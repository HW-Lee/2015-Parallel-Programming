#include <stdio.h>
#include <stdlib.h>
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


/*
 *
 * Main functions
 *
 */
int main( int argc, char* argv[] ) {
    int rank, size;

    MPI_Init( argc, argv );
    MPI_Comm_size( MPI_COMM_WORLD, size );
    MPI_Comm_rank( MPI_COMM_WORLD, rank );

    int N;
    char* in_file;
    char* out_file;
    char phase = EVEN_PHASE;

    *N = atoi( argv[1] );
    *in_file = argv[2];
    *out_file = argv[3];
    // Currently, the parameters needed in the project have been set up.
    // N        : the # of integers to be sorted
    // in_file  : the path of the input file which contains all integers to be sorted
    // out_file : the path of the output file where the sorted sequence is printed

    MPI_Barrier( MPI_COMM_WORLD );
    MPI_Finalize();
    return 0;
}
