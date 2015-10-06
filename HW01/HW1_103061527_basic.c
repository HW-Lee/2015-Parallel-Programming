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

inline void initBuffer( int*** buffer, int* cntMap, int cnt );
inline void fill_buffer( int** buffer, int* cntMap, int cnt, int* local_buffer );
inline void operate( int** buffers, int* cntMap, int cnt, int mode );
inline void send( int** buffers, int* cntMap, int cnt );
inline void recv( int** buffers, int* cntMap, int cnt );
inline int compare_array( int* local_buffer, int** buffers, int* cntMap, int cnt );
inline int sync_array( int* local_buffer, int** buffers, int* cntMap, int cnt );
inline void gather_data( int** buffers, int* data_buf, int* resb_proc, int N );

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

    ROOT_DEBUG_DO(
        int j;
        printf( "resb_proc = [ " );
        for (j = 0; j < N; j++) printf( "%d ", resb_proc[j] );
        printf( "]\n" );
    );

    int* recv_cntMap;
    int* send_cntMap;
    int recv_cnt = getCommProcessCount( resb_proc, N, rank, RECV_MODE, &recv_cntMap );
    int send_cnt = getCommProcessCount( resb_proc, N, rank, SEND_MODE, &send_cntMap );
    // recv_cntMap = [ processId1, recv_buffersize1, ... , processIdN, recv_buffersizeN ]
    // send_cntMap = [ processId1, send_buffersize1, ... , processIdN, send_buffersizeN ]

    DEBUG_DO(
        MPI_Barrier( MPI_COMM_WORLD );
        int j;
        for (j = 0; j < size; j++) {
            PROCESS_DO( j,
                int i;

                printf( "process_recv_cntMap_%d = [ ", j );
                for (i = 0; i < recv_cnt; i++) printf( "%d: %d ", recv_cntMap[2*i], recv_cntMap[2*i+1] );
                printf( "]\n" );

                printf( "process_send_cntMap_%d = [ ", j );
                for (i = 0; i < send_cnt; i++) printf( "%d: %d ", send_cntMap[2*i], send_cntMap[2*i+1] );
                printf( "]\n" );

                MPI_Barrier( MPI_COMM_WORLD );
            );
        }
    );

    int** recv_buffer;
    int** send_buffer;

    initBuffer( &recv_buffer, recv_cntMap, recv_cnt );
    initBuffer( &send_buffer, send_cntMap, send_cnt );

    int* data_buf;
    ROOT_DO(
        load_file( &in_file, &data_buf, N );

        int* msg_buffer;
        int* msg_buffersizes = malloc( size * sizeof( int ) );
        int r;
        int i;
        int curIdx;

        for (r = 0; r < size; r++) msg_buffersizes[r] = 0;
        for (r = 0; r < N; r++) msg_buffersizes[ resb_proc[r] ]++;

        for (r = 0; r < size; r++) {
            if ( msg_buffersizes[r] == 0 ) continue;

            msg_buffer = malloc( msg_buffersizes[r] * sizeof( int ) );
            curIdx = 0;
            for (i = r % 2; i < N; i+=2) {
                if ( resb_proc[i] == r )
                    msg_buffer[ curIdx++ ] = data_buf[i];
            }

            // MPI_Send (buffer, count, type, dest, tag, comm)
            if ( msg_buffersizes[r] > 0 )
                MPI_Send( msg_buffer, msg_buffersizes[r], MPI_INT, r, 0, MPI_COMM_WORLD );

            free( msg_buffer );
        }

        free( msg_buffersizes );
    );

    int* local_buffer;
    if ( local_buffersize > 0 ) {
        local_buffer = malloc( local_buffersize * sizeof( int ) );

        // MPI_Recv  (buffer, count, type, source, tag, comm, status)
        MPI_Recv( local_buffer, local_buffersize, MPI_INT, ROOT, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE );
    }

    ROOT_DEBUG_DO(
        printf( "data_buf = [ " );
        int j;
        for (j = 0; j < N; j++) printf( "%d ", data_buf[j] );
        printf( "]\n" );
    );
    DEBUG_DO(
        MPI_Barrier( MPI_COMM_WORLD );
        int j;
        for (j = 0; j < size; j++) {
            PROCESS_DO( j,
                printf( "local_buffer_%d = [ ", j );
                int i;
                for (i = 0; i < local_buffersize; i++) printf( "%d ", local_buffer[i] );
                printf( "]\n" );
            );
            MPI_Barrier( MPI_COMM_WORLD );
        }
    );

    int swapped[2] = { 1, 1 };
    while (swapped[0] > 0 || swapped[1] > 0) {
        if ( size > 1 ) {
            fill_buffer( send_buffer, send_cntMap, send_cnt, local_buffer );

            // odd rank sends values at even-phase
            if ( rank % 2 != phase ) send( send_buffer, send_cntMap, send_cnt );
            else recv( recv_buffer, recv_cntMap, recv_cnt );

            // compare two arrays
            if ( rank % 2 == phase ) {
                swapped[phase] = compare_array( local_buffer, recv_buffer, recv_cntMap, recv_cnt );
                send( recv_buffer, recv_cntMap, recv_cnt );
            } else {
                recv( send_buffer, send_cntMap, send_cnt );
                swapped[phase] = sync_array( local_buffer, send_buffer, send_cntMap, send_cnt );
            }
        } else {
            swapped[phase] = 0;
            int j;
            int swap_temp;
            for (j = phase + 1; j < N; j+=2) {
                if ( compare( data_buf[j-1], data_buf[j] ) == 1 ) {
                    swapped[phase] = 1;
                    swap_temp = data_buf[j-1];
                    data_buf[j-1] = data_buf[j];
                    data_buf[j] = swap_temp;
                }
            }
        }

        togglePhase( &phase );
    }

    // MPI_Bcast (buffer, count, datatype, root, comm)
    int** all_data;

    // MPI_Send (buffer, count, type, dest, tag, comm)
    // MPI_Recv  (buffer, count, type, source, tag, comm, status)
    MPI_Send( local_buffer, local_buffersize, MPI_INT, ROOT, 0, MPI_COMM_WORLD );

    ROOT_DO(
        if ( size > 1 ) {
            all_data = malloc( size * sizeof( int* ) );
            int j;
            for (j = 0; j < size; j++) all_data[j] = malloc( N * sizeof( int ) );
            for (j = 0; j < size; j++) MPI_Recv( all_data[j], N, MPI_INT, j, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE );

            gather_data( all_data, data_buf, resb_proc, N );
        }

        DEBUG_DO(
            int j;
            printf( "data_buf = [ " );
            for (j = 0; j < N; j++) printf( "%d ", data_buf[j] );
            printf( "]\n" );
        );

        check_if_accurate( data_buf, N );
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

void initBuffer( int*** buffer, int* cntMap, int cnt ) {
    if (cnt <= 0) return;
    *buffer = malloc( cnt * sizeof( int* ) );
    int j;
    for (j = 0; j < cnt; j++)
        if ( cntMap[ 2*j+1 ] > 0 )
            ( *buffer )[j] = malloc( cntMap[ 2*j+1 ] * sizeof( int ) );
}

void fill_buffer( int** buffer, int* cntMap, int cnt, int* local_buffer ) {
    int i, j;
    int acc = 0;

    for (i = 0; i < cnt; i++) {
        for (j = 0; j < cntMap[ 2*i + 1 ]; j++) {
            buffer[i][j] = local_buffer[ acc + j ];
        }

        acc += cntMap[ 2*i + 1 ];
    }
}

void operate( int** buffers, int* cntMap, int cnt, int mode ) {
    int j;
    int target;
    int count;

    for (j = 0; j < cnt; j++) {
        target = cntMap[ 2*j ];
        count = cntMap[ 2*j + 1 ];

        if ( target < 0 || count <= 0 ) continue;

        if ( mode == SEND_MODE )
            // MPI_Send (buffer, count, type, dest, tag, comm)
            MPI_Send( buffers[j], count, MPI_INT, target, 0, MPI_COMM_WORLD );
        else
            // MPI_Recv  (buffer, count, type, source, tag, comm, status)
            MPI_Recv( buffers[j], count, MPI_INT, target, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE );
    }
}

void send( int** buffers, int* cntMap, int cnt ) { operate( buffers, cntMap, cnt, SEND_MODE ); }
void recv( int** buffers, int* cntMap, int cnt ) { operate( buffers, cntMap, cnt, RECV_MODE ); }

int compare_array( int* local_buffer, int** buffers, int* cntMap, int cnt ) {
    int offset = 0;
    int j;
    for (j = 0; j < cnt; j++)
        if ( cntMap[ 2*j ] < 0 ) offset += cntMap[ 2*j + 1 ];
        else break;

    int acc = 0;
    int i;
    int cur_count;
    int swap_temp;
    int sth_updated = 0;
    for (i = 0; i < cnt; i++) {
        cur_count = cntMap[ 2*i + 1 ];
        if ( cntMap[ 2*i ] >= 0 ) {
            for (j = 0; j < cur_count; j++) {
                if ( compare( local_buffer[ offset + acc + j ], buffers[i][j] ) == 1 ) {
                    swap_temp = local_buffer[ offset + acc + j ];
                    local_buffer[ offset + acc + j ] = buffers[i][j];
                    buffers[i][j] = swap_temp;
                    sth_updated = 1;
                }
            }
        }
        acc += cur_count;
    }

    return sth_updated;
}

int sync_array( int* local_buffer, int** buffers, int* cntMap, int cnt ) {
    int offset = 0;
    int j;
    for (j = 0; j < cnt; j++)
        if ( cntMap[ 2*j ] < 0 ) offset += cntMap[ 2*j + 1 ];
        else break;

    int acc = 0;
    int i;
    int cur_count;
    int sth_updated = 0;
    for (i = 0; i < cnt; i++) {
        cur_count = cntMap[ 2*i + 1 ];
        if ( cntMap[ 2*i ] >= 0 ) {
            for (j = 0; j < cur_count; j++) {
                if ( sth_updated == 0 && local_buffer[ offset + acc + j ] != buffers[i][j] ) sth_updated = 1;
                local_buffer[ offset + acc + j ] = buffers[i][j];
            }
            acc += cur_count;
        }
    }

    return sth_updated;
}

void gather_data( int** buffers, int* data_buf, int* resb_proc, int N ) {
    int j;
    int size = 0;
    int cur_process;
    for (j = 0; j < N; j++)
        if ( resb_proc[j] > size ) size = resb_proc[j];
    size++;

    int* counters = malloc( size * sizeof( int ) );
    for (j = 0; j < size; j++) counters[j] = 0;
    for (j = 0; j < N; j++) {
        cur_process = resb_proc[j];
        data_buf[j] = buffers[ cur_process ][ counters[ cur_process ]++ ];
    }

    free( counters );
}

void load_file( char** in_file, int** data_buf, int count ) {
    *data_buf = malloc( count * sizeof(int) );
    int i;
    for (i = 0; i < count; i++)
        (*data_buf)[i] = count - i;
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
        if ( compare( series[i-1], series[i] ) != compare( series[i], series[i+1] ) ) {
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

