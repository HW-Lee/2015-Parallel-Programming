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


/*
 *
 * Functions
 *
 */
inline void init( int* argc, char** argv[], int* size, int* rank );
inline void finalize();

inline void processParameters( char* argv[], int* N, char** in_file, char** out_file );
inline void load_file( char** in_file, int** data_buf, int count );
inline void togglePhase( char* phase );
inline int pairIdx2arrayIdx( char phase, int pairIdx );
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

    DEBUG_DO( printf( "Hello, world! from process_%d\n", rank ); finalize(); return 0; );

    processParameters( argv, &N, &in_file, &out_file );

    int* data_buf;
    ROOT_DO( load_file( &in_file, &data_buf, N ) );

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

void load_file( char** in_file, int** data_buf, int count ) {
    *data_buf = malloc( count * sizeof(int) );
    int i;
    for (i = 0; i < count; i++)
        (*data_buf)[i] = count - i;
}

int pairIdx2arrayIdx( char phase, int pairIdx ) {
    return pairIdx * 2 + (int)phase;
}

void togglePhase( char* phase ) {
    *phase = (*phase + 1) % 2;
}

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

