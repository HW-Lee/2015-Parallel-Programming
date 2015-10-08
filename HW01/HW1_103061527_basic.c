#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

/*
 *
 * Commands
 *
 */
#define DO( cond, cmd ) if ( cond ) { cmd; }
#define DEBUG_DO(cmd) DO( DEBUG, cmd )
#define PROCESS_DO( r, cmd ) DO( rank == r, cmd )
#define PROCESS_DEBUG_DO( r, cmd ) DO( rank == r && DEBUG, cmd )
#define ROOT_DO(cmd) PROCESS_DO( ROOT, cmd )
#define ROOT_DEBUG_DO(cmd) PROCESS_DEBUG_DO( ROOT, cmd )


/*
 *
 * Constants
 *
 */
#define ROOT 0
#define DEBUG 0
#define EVEN_PHASE 0
#define ODD_PHASE 1
#define RECV_MODE 0
#define SEND_MODE 1


/*
 *
 * Functions
 *
 */
inline void init( int* argc, char** argv[], int* size, int* rank );
inline void finalize();

inline void processParameters( char* argv[], int* N, char** in_file, char** out_file );
inline int getResponsibility( int** resb_proc, int Neven, int size_even, int Nodd, int size_odd, int processId );
inline int getCommProcessCount( int* resb_proc, int N, int processId, int mode, int** cntMap );
inline void scatter( int* data_buf, int* resb_proc, int N, int** root_local_data );
inline int compare_arrays( int* recv_arr, int* orig_arr, int count );
inline int sync_arrays( int* recv_arr, int* orig_arr, int count ); 

inline void op( int* buffer, int* cntMap, int cnt, int mode );
inline void send( int* buffer, int* cntMap, int cnt );
inline void recv( int* buffer, int* cntMap, int cnt );

inline void load_file( char** in_file, int** data_buf, int count );
inline void togglePhase( char* phase );
inline int compare( int a, int b );
inline void check_if_accurate( int* series, int count );


/*
 *
 * Main functions
 *
 */
int main( int argc, char* argv[] ) {
    int rank, size;

    init( &argc, &argv, &size, &rank );

    int N;
    char* in_file;
    char* out_file;
    char phase = EVEN_PHASE;

    processParameters( argv, &N, &in_file, &out_file );
    // Currently, the parameters needed in the project have been set up.
    // N        : the # of integers to be sorted
    // in_file  : the path of the input file which contains all integers to be sorted
    // out_file : the path of the output file where the sorted sequence is printed

    if ( size >= N ) size = N - 1;

    int Nodd = N / 2; // the # of elements in odd index: floor( N/2 )
    int Neven = N - Nodd; // the # of elements in even index: round( N/2 )
    int size_odd = size / 2;
    int size_even = size - size_odd;


    // each element refers to the process which takes resbonsibility of processing
    int* resb_proc;
    int local_buffersize;
    if ( size > 1 ) {
        local_buffersize = getResponsibility( &resb_proc, Neven, size_even, Nodd, size_odd, rank );
    } else {
        local_buffersize = N;
        resb_proc = malloc( N * sizeof( int ) );
        int j;
        for (j = 0; j < N; j++) resb_proc[j] = 0;
    }

    int* recv_cntMap;
    int* send_cntMap;
    int recv_cnt = getCommProcessCount( resb_proc, N, rank, RECV_MODE, &recv_cntMap );
    int send_cnt = getCommProcessCount( resb_proc, N, rank, SEND_MODE, &send_cntMap );
    // recv_cntMap = [ processId1, recv_buffersize1, ... , processIdN, recv_buffersizeN ]
    // send_cntMap = [ processId1, send_buffersize1, ... , processIdN, send_buffersizeN ]

    int* data_buf;
    int* local_buffer;

    if ( local_buffersize > 0 ) local_buffer = malloc( local_buffersize * sizeof( int ) );

    ROOT_DO(
        load_file( &in_file, &data_buf, N );
        scatter( data_buf, resb_proc, N, &local_buffer );
    );

    int swapped[2] = { 1, 1 };
    int* comm_buffer;

    if ( local_buffersize > 0 ) {
        // MPI_Recv ( buf, count, datatype, source, tag, comm, status )
        if ( rank != ROOT ) MPI_Recv( local_buffer, local_buffersize, MPI_INT, ROOT, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE );
        comm_buffer = malloc( local_buffersize * sizeof( int ) );

        int j;
        for (j = 0; j < local_buffersize; j++) comm_buffer[j] = local_buffer[j];

    }

    while ( swapped[0] == 1 || swapped[1] == 1 ) {
        if ( size > 1 ) {
            if ( rank % 2 != phase ) {
                int j;
                for (j = 0; j < local_buffersize; j++) comm_buffer[j] = local_buffer[j];
            }
            if ( rank % 2 != phase ) send( comm_buffer, send_cntMap, send_cnt );
            else recv( comm_buffer, recv_cntMap, recv_cnt );

            int offset, compare_len;

            if ( rank % 2 != phase ) {
                offset = ( send_cntMap[0] < 0 ) ? 1:0;
                compare_len = local_buffersize - offset;
                if ( send_cntMap[ 2*(send_cnt-1) ] < 0 ) compare_len--;

                recv( comm_buffer, send_cntMap, send_cnt );
                swapped[phase] = sync_arrays( comm_buffer, &( local_buffer[offset] ), compare_len );
            } else {
                offset = ( recv_cntMap[0] < 0 ) ? 1:0;
                compare_len = local_buffersize - offset;
                if ( recv_cntMap[ 2*(recv_cnt-1) ] < 0 ) compare_len--;

                swapped[phase] = compare_arrays( comm_buffer, &( local_buffer[offset] ), compare_len );
                send( comm_buffer, recv_cntMap, recv_cnt );
            }

        } else {
            swapped[phase] = 0;
            int j;
            int swap_temp;
            for (j = phase+1; j < N; j+=2) {
                if ( compare( data_buf[j-1], data_buf[j] ) == 1 ) {
                    swapped[phase] = 1;
                    swap_temp = data_buf[j];
                    data_buf[j] = data_buf[j-1];
                    data_buf[j-1] = swap_temp;
                }
            }
        }

        togglePhase( &phase );
    }

    int** all_data;
    MPI_Request req;

    if ( rank != ROOT ) {
        // MPI_Isend ( buf, count, datatype, dest, tag, comm, request )
        MPI_Isend( local_buffer, local_buffersize, MPI_INT, ROOT, 0, MPI_COMM_WORLD, &req );
    }

    ROOT_DO(
        // MPI_Recv ( buf, count, datatype, source, tag, comm, status )
        all_data = malloc( size * sizeof( int* ) );
        int j;
        for (j = 1; j < size; j++) all_data[j] = malloc( N * sizeof( int ) );
        for (j = 1; j < size; j++) MPI_Recv( all_data[j], N, MPI_INT, j, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE );

        int* counters = malloc( size * sizeof( int ) );
        int target;
        for (j = 0; j < size; j++) counters[j] = 0;
        for (j = 0; j < N; j++) {
            target = resb_proc[j];
            if ( target > 0 ) data_buf[j] = all_data[target][ counters[target]++ ];
            else data_buf[j] = local_buffer[ counters[target]++ ];
        }

        printf( "data_buf = [ " );
        for (j = 0; j < N; j++) printf( "%d ", data_buf[j] );
        printf( "]\n" );
    );

    finalize();
    return 0;
}


/*
 *
 * Functions implementation
 *
 */
void processParameters( char* argv[], int* N, char** in_file, char** out_file ) {
    *N = atoi( argv[1] );
    *in_file = argv[2];
    *out_file = argv[3];
}

int getResponsibility( int** resb_proc, int Neven, int size_even, int Nodd, int size_odd, int processId ) {
    int N = Neven + Nodd;

    *resb_proc = malloc( N * sizeof( int ) );

    int j;
    int* num_even = malloc( (size_even+1) * sizeof(int) );
    int* num_odd = malloc( (size_odd+1) * sizeof(int) );

    // Initiate
    for (j = 1; j < size_even+1; j++) num_even[j] = Neven / size_even;
    for (j = 1; j < size_odd+1; j++) num_odd[j] = Nodd / size_odd;

    // Adding modulo
    for (j = 0; j < Neven % size_even; j++) num_even[ j+1 ]++;
    for (j = 0; j < Nodd % size_odd; j++) num_odd[ j+1 ]++;

    // Accumulated
    num_even[0] = 0; num_odd[0] = 0;
    for (j = 1; j < size_even+1; j++) num_even[j] += num_even[j-1];
    for (j = 1; j < size_odd+1; j++) num_odd[j] += num_odd[j-1];

    // Assign processes
    for (j = 0; j < size_even; j++) {
        int i;
        for (i = num_even[j]; i < num_even[j+1]; i++) ( *resb_proc )[ i*2 ] = j * 2;
    }
    for (j = 0; j < size_odd; j++) {
        int i;
        for (i = num_odd[j]; i < num_odd[j+1]; i++) ( *resb_proc )[ i*2+1 ] = j * 2 + 1;
    }

    int cnt = 0;
    for (j = 0; j < N; j++) {
        if ( ( *resb_proc )[j] == processId ) cnt++;
    }

    free( num_even );
    free( num_odd );

    return cnt;
}

int getCommProcessCount( int* resb_proc, int N, int processId, int mode, int** cntMap ) {
    int cnt = 0;
    int* temp_cntMap = malloc( 2 * N * sizeof( int ) );
    int unique_num = N;
    int j;
    int adjacent_proc = -1;
    if ( mode == RECV_MODE ) {
        for (j = processId % 2; j < N; j+=2) {
            if ( resb_proc[j] > processId ) break;
            if ( resb_proc[j] != processId ) continue;

            if ( j < N-1 ) adjacent_proc = resb_proc[ j+1 ];
            else adjacent_proc = -1;

            if ( adjacent_proc != unique_num || adjacent_proc < 0 ) {
                unique_num = adjacent_proc;
                temp_cntMap[ 2*cnt ] = unique_num;
                temp_cntMap[ 2*cnt + 1 ] = 1;
                cnt++;
            } else if ( adjacent_proc == unique_num ) {
                temp_cntMap[ 2*cnt - 1 ]++;
            }
        }
    } else {
        for (j = processId % 2; j < N; j+=2) {
            if ( resb_proc[j] > processId ) break;
            if ( resb_proc[j] != processId ) continue;

            if ( j > 0 ) adjacent_proc = resb_proc[ j-1 ];
            else adjacent_proc = -1;

            if ( adjacent_proc != unique_num || adjacent_proc < 0 ) {
                unique_num = adjacent_proc;
                temp_cntMap[ 2*cnt ] = unique_num;
                temp_cntMap[ 2*cnt + 1 ] = 1;
                cnt++;
            } else if ( adjacent_proc == unique_num ) {
                temp_cntMap[ 2*cnt - 1 ]++;
            }
        }
    }

    *cntMap = malloc( 2 * cnt * sizeof( int ) );
    for (j = 0; j < 2*cnt; j++) ( *cntMap )[j] = temp_cntMap[j];

    free( temp_cntMap );

    return cnt;
}

void scatter( int* data_buf, int* resb_proc, int N, int** root_local_data ) {
    // MPI_Isend ( buf, count, datatype, dest, tag, comm, request )
    int size = 0;
    int j;

    for (j = 0; j < N; j++)
        if ( resb_proc[j] > size ) size = resb_proc[j];
    size++;

    int* counters = malloc( size * sizeof( int ) );
    int** buffers = malloc( size * sizeof( int* ) );

    for (j = 0; j < size; j++) {
        counters[j] = 0;
        buffers[j] = malloc( N * sizeof( int ) );
    }

    int target;
    for (j = 0; j < N; j++) {
        target = resb_proc[j];
        if ( target > 0 )
            buffers[ target ][ counters[target]++ ] = data_buf[j];
        else
            ( *root_local_data )[ counters[target]++ ] = data_buf[j];
    }

    MPI_Request* req = malloc( size * sizeof( MPI_Request ) );
    for (j = 1; j < size; j++) {
        if ( counters[j] > 0 )
            MPI_Isend( buffers[j], counters[j], MPI_INT, j, 0, MPI_COMM_WORLD, &( req[j] ) );
    }

    for (j = 1; j < size; j++) MPI_Wait( &( req[j] ), MPI_STATUS_IGNORE );

    for (j = 0; j < size; j++) free( buffers[j] );
    free( buffers );
    free( counters );
}

int compare_arrays( int* recv_arr, int* orig_arr, int count ) {
    int j;
    int swapped = 0;
    int swap_temp;
    for (j = 0; j < count; j++) {
        if ( compare( orig_arr[j], recv_arr[j] ) == 1 ) {
            swapped = 1;
            swap_temp = orig_arr[j];
            orig_arr[j] = recv_arr[j];
            recv_arr[j] = swap_temp;
        }
    }

    return swapped;
}

int sync_arrays( int* recv_arr, int* orig_arr, int count ) {
    int j;
    int updated = 0;
    for (j = 0; j < count; j++) {
        if ( recv_arr[j] != orig_arr[j] ) {
            updated = 1;
            orig_arr[j] = recv_arr[j];
        }
    }

    return updated;
}

void send( int* buffer, int* cntMap, int cnt ) { op( buffer, cntMap, cnt, SEND_MODE ); }
void recv( int* buffer, int* cntMap, int cnt ) { op( buffer, cntMap, cnt, RECV_MODE ); }

void op( int* buffer, int* cntMap, int cnt, int mode ) {
    // MPI_Isend ( buf, count, datatype, dest, tag, comm, request )
    // MPI_Recv ( buf, count, datatype, source, tag, comm, status )

    int offset = 0;
    int j;
    int target, count;
    MPI_Request* req = malloc( cnt * sizeof( MPI_Request ) );
    for (j = 0; j < cnt; j++) {
        target = cntMap[ 2*j ];
        count = cntMap[ 2*j+1 ];

        if ( target >= 0 ) {
            if ( mode == SEND_MODE ) {
                MPI_Isend( &( buffer[ offset ] ), count, MPI_INT, target, 0, MPI_COMM_WORLD, &( req[j] ) );
            } else {
                MPI_Recv( &( buffer[ offset ] ), count, MPI_INT, target, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE );
            }
        } else if ( mode == RECV_MODE ) offset -= count;

        offset += count;
    }
}

void load_file( char** in_file, int** data_buf, int count ) {
    *data_buf = malloc( count * sizeof(int) );
    int i;
    for (i = 0; i < count; i++)
        (*data_buf)[i] = (count - i);
}

void togglePhase( char* phase ) { *phase = (*phase + 1) % 2; }

int compare( int a, int b ) {
    if ( a > b ) return 1;
    if ( b > a ) return -1;
    return 0;
}

void check_if_accurate( int* series, int count ) {
    int i;
    for (i = 1; i < count-1; i++) {
        if ( compare( series[i-1], series[i] ) * compare( series[i], series[i+1] ) < 0 ) {
            printf( "Something wrong!!\n" );
            return;
        }
    }
    printf( "The function works well!!\n" );
}

void init( int* argc, char** argv[], int* size, int* rank ) {
    MPI_Init( argc, argv );
    MPI_Comm_size( MPI_COMM_WORLD, size );
    MPI_Comm_rank( MPI_COMM_WORLD, rank );
}

void finalize() {
    MPI_Barrier( MPI_COMM_WORLD );
    MPI_Finalize();
}

