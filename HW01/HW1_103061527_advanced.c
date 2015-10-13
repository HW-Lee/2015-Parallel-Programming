#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <mpi.h>

/*
 *
 * Constants
 *
 */
#define ROOT 0
#define DIR_RIGHT 0
#define DIR_LEFT 1

inline int compare( int a, int b );

inline void _merge_sort_recur( int* arr, int* reg, int start_idx, int end_idx );
inline void merge_sort( int* arr, int count );
inline void insert_and_kick( int* arr, int count, int* inserted_n_kicked );
inline void shift_array( int* arr, int count, int dist, int dir );

/*
 *
 * Main functions
 *
 */
int main( int argc, char* argv[] ) {
    clock_t start, end;
    double IO_millis = 0, comm_millis = 0, comp_millis = 0, sync_millis = 0;
    int rank, size;

    MPI_Init( &argc, &argv );
    MPI_Comm_size( MPI_COMM_WORLD, &size );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );

    int N;
    char* in_file;
    char* out_file;

    N = atoi( argv[1] );
    in_file = argv[2];
    out_file = argv[3];
    // Currently, the parameters needed in the project have been set up.
    // N        : the # of integers to be sorted
    // in_file  : the path of the input file which contains all integers to be sorted
    // out_file : the path of the output file where the sorted sequence is printed

    int red_process_num = 0;
    if ( N < 20 ) {
        red_process_num = size - 1;
        size = 1;
    } else if ( size > N/4 ) {
        red_process_num = size - N/4;
        size = N/4;
    }

    int data_bufsize = ( N % size > 0 ) ? ( N + size - ( N % size ) ) : N;
    int* data_buf = malloc( ( size + red_process_num ) * ( data_bufsize / size ) * sizeof( int ) );
    MPI_File f;

    start = clock();
    MPI_Barrier( MPI_COMM_WORLD );
    end = clock();
    sync_millis += ((double) (end - start)) * 1000 / CLOCKS_PER_SEC;

    start = clock();
    // MPI_File_open (MPI_Comm comm, char *filename, int amode, MPI_Info info, MPI_File *fh)
    MPI_File_open( MPI_COMM_WORLD, in_file, MPI_MODE_RDONLY, MPI_INFO_NULL, &f );

    // MPI_File_read (MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status)
    MPI_File_read( f, data_buf, N, MPI_INT, MPI_STATUS_IGNORE );
    end = clock();
    IO_millis += ((double) (end - start)) * 1000 / CLOCKS_PER_SEC;

    start = clock();
    int i;
    for (i = 0; i < data_bufsize - N; i++) data_buf[ N + i ] = INT_MAX;

    int local_bufsize = data_bufsize / size;
    int* local_buf = malloc( local_bufsize * sizeof( int ) );

    if ( rank < size ) {
        for (i = 0; i < local_bufsize; i++) local_buf[i] = data_buf[ rank * local_bufsize + i ];
        merge_sort( local_buf, local_bufsize );
    }

    if ( rank != ROOT ) free( data_buf );
    end = clock();
    comp_millis += ((double) (end - start)) * 1000 / CLOCKS_PER_SEC;

    int updated = 1;
    int local_updated;
    int send_msg, recv_msg;

    while ( updated > 0 && size > 1 ) {

        local_updated = 0;

        start = clock();
        MPI_Barrier( MPI_COMM_WORLD );
        end = clock();
        sync_millis += ((double) (end - start)) * 1000 / CLOCKS_PER_SEC;

        start = clock();
        if ( rank < size ) {
            if ( rank > ROOT ) {
                send_msg = local_buf[0];
                MPI_Send( &send_msg, 1, MPI_INT, rank-1, 0, MPI_COMM_WORLD );
            }
            if ( rank < size-1 ) {
                MPI_Recv( &recv_msg, 1, MPI_INT, rank+1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE );
            }
        }
        end = clock();
        comm_millis += ((double) (end - start)) * 1000 / CLOCKS_PER_SEC;

        start = clock();
        MPI_Barrier( MPI_COMM_WORLD );
        end = clock();
        sync_millis += ((double) (end - start)) * 1000 / CLOCKS_PER_SEC;

        start = clock();
        if ( rank < size && rank < size-1 ) {
            send_msg = recv_msg;
            if ( compare( recv_msg, local_buf[ local_bufsize-1 ] ) < 1 ) {
                local_updated = 1;
                send_msg = local_buf[ local_bufsize-1 ];
                local_buf[ local_bufsize-1 ] = recv_msg;
            }
        }
        end = clock();
        comp_millis += ((double) (end - start)) * 1000 / CLOCKS_PER_SEC;

        start = clock();
        MPI_Barrier( MPI_COMM_WORLD );
        end = clock();
        sync_millis += ((double) (end - start)) * 1000 / CLOCKS_PER_SEC;

        start = clock();
        if ( rank < size ) {
            if ( rank < size-1 ) MPI_Send( &send_msg, 1, MPI_INT, rank+1, 0, MPI_COMM_WORLD );
            if ( rank > ROOT ) MPI_Recv( &recv_msg, 1, MPI_INT, rank-1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE );
        }
        end = clock();
        comm_millis += ((double) (end - start)) * 1000 / CLOCKS_PER_SEC;

        start = clock();
        if ( rank < size && rank > ROOT ) {
            shift_array( local_buf, local_bufsize, 1, DIR_LEFT );
            local_buf[ local_bufsize-1 ] = recv_msg;
        }

        if ( rank < size ) {
            send_msg = local_buf[ local_bufsize-1 ];
            recv_msg = local_buf[ local_bufsize-2 ];
            local_buf[ local_bufsize-1 ] = INT_MAX;
            local_buf[ local_bufsize-2 ] = INT_MAX;
            insert_and_kick( local_buf, local_bufsize, &send_msg );
            insert_and_kick( local_buf, local_bufsize, &recv_msg );
        }
        end = clock();
        comp_millis += ((double) (end - start)) * 1000 / CLOCKS_PER_SEC;;

        start = clock();
        MPI_Barrier( MPI_COMM_WORLD );
        end = clock();
        sync_millis += ((double) (end - start)) * 1000 / CLOCKS_PER_SEC;

        start = clock();
        MPI_Allreduce( &local_updated, &updated, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD );
        end = clock();
        comm_millis += ((double) (end - start)) * 1000 / CLOCKS_PER_SEC;
    }

    start = clock();
    MPI_Barrier( MPI_COMM_WORLD );
    end = clock();
    sync_millis += ((double) (end - start)) * 1000 / CLOCKS_PER_SEC;

    start = clock();
    // MPI_Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm)
    MPI_Gather( local_buf, local_bufsize, MPI_INT, data_buf, local_bufsize, MPI_INT, ROOT, MPI_COMM_WORLD );
    end = clock();
    comm_millis += ((double) (end - start)) * 1000 / CLOCKS_PER_SEC;

    start = clock();
    if ( rank == ROOT ) {
        FILE* f = fopen( out_file, "wb" );
        fwrite( data_buf, sizeof( int ), N, f );
    }
    end = clock();
    IO_millis += ((double) (end - start)) * 1000 / CLOCKS_PER_SEC;

    printf( "{\n\t\"id\": %d,\n\t\"i/o\": %.3f,\n\t\"comm\": %.3f,\n\t\"sync\": %.3f\n\t\"comp\": %.3f\n}\n", 
        rank, IO_millis, comm_millis, sync_millis, comp_millis );

    MPI_Barrier( MPI_COMM_WORLD );
    MPI_Finalize();
    return 0;
}

int compare( int a, int b ) {
    if ( a > b ) return 1;
    if ( b > a ) return -1;
    return 0;
}

void shift_array( int* arr, int count, int dist, int dir ) {
    int j;

    if ( dir == DIR_RIGHT ) for (j = count-1; j >= dist; j--) arr[j] = arr[ j-dist ];
    else for (j = 0; j < count-dist; j++) arr[j] = arr[ j+dist ];
}

void _merge_sort_recur( int* arr, int* reg, int start_idx, int end_idx ) {
    if ( start_idx >= end_idx ) return;

    int mid = ( ( end_idx - start_idx ) >> 1 ) + start_idx;
    int start1 = start_idx, end1 = mid;
    int start2 = mid + 1, end2 = end_idx;

    _merge_sort_recur( arr, reg, start1, end1 );
    _merge_sort_recur( arr, reg, start2, end2 );

    int k = start_idx;
    while ( start1 <= end1 && start2 <= end2 )
        reg[ k++ ] = ( compare( arr[ start1 ], arr[ start2 ] ) == -1 ) ? arr[ start1++ ] : arr[ start2++ ];

    while ( start1 <= end1 ) reg[ k++ ] = arr[ start1++ ];
    while ( start2 <= end2 ) reg[ k++ ] = arr[ start2++ ];

    for (k = start_idx; k <= end_idx; k++) arr[k] = reg[k];
}

void merge_sort( int* arr, int count ) {
    int* reg = malloc( count * sizeof( int ) );
    _merge_sort_recur( arr, reg, 0, count-1 );
}

void insert_and_kick( int* arr, int count, int* inserted_n_kicked ) {
    int start_idx = 0, end_idx = count - 1;
    int mid, comp_result;
    int swap_temp, j;

    if ( compare( *inserted_n_kicked, arr[ end_idx ] ) >= 0 ) return;

    if ( compare( *inserted_n_kicked, arr[ start_idx ] ) <= 0 ) {
        swap_temp = *inserted_n_kicked;
        *inserted_n_kicked = arr[ end_idx ];
        for (j = end_idx; j > 0; j--) arr[ j ] = arr[ j-1 ];
        arr[ start_idx ] = swap_temp;
        return;
    }

    while ( end_idx - start_idx > 1 ) {
        mid = ( start_idx + end_idx ) >> 1;
        comp_result = compare( *inserted_n_kicked, arr[mid] );
        if ( comp_result == 1 ) start_idx = mid;
        else if ( comp_result == -1 ) end_idx = mid;
        else {
            start_idx = mid;
            end_idx = mid;
            break;
        }
    }

    swap_temp = *inserted_n_kicked;
    *inserted_n_kicked = arr[ count - 1 ];
    for (j = count-1; j > end_idx; j--) arr[ j ] = arr[ j-1 ];
    arr[ end_idx ] = swap_temp;
}
