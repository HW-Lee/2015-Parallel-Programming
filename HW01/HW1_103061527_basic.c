#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <mpi.h>

/*
 *
 * Constants
 *
 */
#define ROOT 0
#define DEBUG 1
#define EVEN_PHASE 0
#define ODD_PHASE 1
#define RECV_MODE 0
#define SEND_MODE 1


inline void check_if_accurate( int* buf, int count );
inline void toggle_phase( char* phase );
inline int compare( int a, int b );

/*
 *
 * Main functions
 *
 */
int main( int argc, char* argv[] ) {
    int rank, size;

    MPI_Init( &argc, &argv );
    MPI_Comm_size( MPI_COMM_WORLD, &size );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );

    int N;
    char* in_file;
    char* out_file;
    char phase = EVEN_PHASE;

    N = atoi( argv[1] );
    in_file = argv[2];
    out_file = argv[3];
    // Currently, the parameters needed in the project have been set up.
    // N        : the # of integers to be sorted
    // in_file  : the path of the input file which contains all integers to be sorted
    // out_file : the path of the output file where the sorted sequence is printed

    int red_process_num = 0;
    if ( size > N ) {
        red_process_num = size - N;
        size = N;
    }

    int data_bufsize = ( N % size > 0 ) ? ( N + size - ( N % size ) ) : N;
    int* data_buf = malloc( ( size + red_process_num ) * ( data_bufsize / size ) * sizeof( int ) );
    MPI_File f;

    // MPI_File_open (MPI_Comm comm, char *filename, int amode, MPI_Info info, MPI_File *fh)
    MPI_File_open( MPI_COMM_WORLD, in_file, MPI_MODE_RDONLY, MPI_INFO_NULL, &f );

    // MPI_File_read (MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status)
    MPI_File_read( f, data_buf, N, MPI_INT, MPI_STATUS_IGNORE );

    int i;
    for (i = 0; i < data_bufsize - N; i++) data_buf[ N + i ] = INT_MAX;

    // if ( rank == ROOT ) {
    //     for (i = 0; i < data_bufsize; i++)
    //         printf( "%d ", data_buf[i] );
    //     printf( "\n" );
    // }

    // MPI_Barrier( MPI_COMM_WORLD );

    int local_bufsize = data_bufsize / size;
    int* local_buf = malloc( local_bufsize * sizeof( int ) );

    if ( rank < size )
        for (i = 0; i < local_bufsize; i++) local_buf[i] = data_buf[ rank * local_bufsize + i ];

    if ( rank != ROOT ) free( data_buf );

    // int j;
    // for (j = 0; j < size; j++) {
    //     for (i = 0; i < local_bufsize; i++) {
    //         if ( rank == j ) { printf( "%d ", local_buf[i] ); if ( i == local_bufsize-1 ) printf( "\n" ); }
    //         MPI_Barrier( MPI_COMM_WORLD );
    //     }
    // }

    int msg_buf;
    int msg_recved, msg_sent;
    int local_buf_pos = rank * local_bufsize; // it means local_buf[0] == data_buf[local_buf_pos]

    int swapped[2] = { 1, 1 };
    int sync_swapped[2] = { 1, 1 };

    while ( sync_swapped[0] > 0 || sync_swapped[1] > 0 ) {
        msg_recved = 0;
        msg_sent = 0;
        swapped[phase] = 0;

        if ( rank < size ) {
            // Send: even-phase when pos is odd; odd-phase when pos is even
            if ( rank > 0 && local_buf_pos % 2 != phase ) {
                // printf( "process%d sent data_buf[%d]: %d\n", rank, local_buf_pos, local_buf[0] );
                MPI_Send( &( local_buf[0] ), 1, MPI_INT, rank-1, 0, MPI_COMM_WORLD );
                msg_sent = 1;
            }

            // Recv: even-phase when pos+bufsize-1 is even, odd-phase when pos+bufsize-1 is odd
            if ( rank < size-1 && ( local_buf_pos + local_bufsize - 1 ) % 2 == phase ) {
                MPI_Recv( &msg_buf, 1, MPI_INT, rank+1, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE );
                // printf( "process%d received %d\n", rank, msg_buf );
                msg_recved = 1;
            }

            int j;
            int swap_temp;
            // Compare:
            //      even-phase starts at local_buf_pos+1/local_buf_pos+2 (even/odd)
            //       odd-phase starts at local_buf_pos/local_buf_pos+1 (even/odd)
            for (j = phase + 1; j < N; j+=2) {
                if ( j >= local_buf_pos + local_bufsize ) break;
                if ( j <= local_buf_pos ) continue;
                // printf( "compare( %d, %d ) by process%d\n", local_buf[ j - local_buf_pos - 1 ], local_buf[ j - local_buf_pos ], rank );
                if ( compare( local_buf[ j - local_buf_pos - 1 ], local_buf[ j - local_buf_pos ] ) == 1 ) {
                    swapped[phase] = 1;
                    swap_temp = local_buf[ j - local_buf_pos - 1 ];
                    local_buf[ j - local_buf_pos - 1 ] = local_buf[ j - local_buf_pos ];
                    local_buf[ j - local_buf_pos ] = swap_temp;
                }
            }

            if ( msg_recved == 1 ) {
                // printf( "compare( %d, %d ) by process%d\n", local_buf[ local_bufsize-1 ], msg_buf, rank );
                if ( compare( local_buf[ local_bufsize-1 ], msg_buf ) == 1 ) {
                    swapped[phase] = 1;
                    swap_temp = local_buf[ local_bufsize-1 ];
                    local_buf[ local_bufsize-1 ] = msg_buf;
                    msg_buf = swap_temp;
                }
                MPI_Send( &msg_buf, 1, MPI_INT, rank+1, 0, MPI_COMM_WORLD );
            }

            if ( msg_sent == 1 ) {
                MPI_Recv( &msg_buf, 1, MPI_INT, rank-1, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE );
                if ( msg_buf != local_buf[0] ) swapped[phase] = 1;
                local_buf[0] = msg_buf;
            }
        } else {
            printf( "process%d is redundant\n", rank );
        }

        // MPI_Allreduce (send_buf, recv_buf, count, data_type, op, comm)
        MPI_Allreduce( &( swapped[phase] ), &( sync_swapped[phase] ), 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD );

        toggle_phase( &phase );
    }

    // MPI_Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm)
    MPI_Gather( local_buf, local_bufsize, MPI_INT, data_buf, local_bufsize, MPI_INT, ROOT, MPI_COMM_WORLD );

    if ( rank == ROOT ) {
        // check_if_accurate( data_buf, N );
        FILE* f = fopen( out_file, "wb" );
        fwrite( data_buf, sizeof( int ), N, f );
    }

    MPI_Barrier( MPI_COMM_WORLD );
    MPI_Finalize();
    return 0;
}

void toggle_phase( char* phase ) {
    *phase = ( *phase + 1 ) % 2;
}

int compare( int a, int b ) {
    if ( a > b ) return 1;
    if ( b > a ) return -1;
    return 0;
}

void check_if_accurate( int* buf, int count ) {
    int j;
    for (j = 0; j < count-1; j++) {
        if ( compare( buf[j], buf[j+1] ) == 1 ) {
            printf( "Something wrong!\n" );
            return;
        }
    }
    printf( "The function works well!!\n" );
}
