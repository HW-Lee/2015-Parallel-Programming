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
#define DEBUG 1
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
        int j;
        for (j = 0; j < size; j++) {
            MPI_Barrier( MPI_COMM_WORLD );
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

