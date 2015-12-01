#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

int main( int argc, char* argv[] ) {
	double start, end;
	double sq_io_millis = 0, mpi_io_millis = 0;
    int rank, size;

    MPI_Init( &argc, &argv );
    MPI_Comm_size( MPI_COMM_WORLD, &size );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );

    int N;
    char* in_file;
    char* out_file;

    N = atoi( argv[1] );
    in_file = argv[2];

    int* data_buf = malloc( N * sizeof( int ) );

    MPI_Barrier( MPI_COMM_WORLD );

    start = MPI_Wtime();
    FILE* f = fopen( in_file, "rb" );
    fread( data_buf, N, sizeof( int ), f );
    fclose( f );
    end = MPI_Wtime();
    sq_io_millis += ( end - start ) * 1000;

    MPI_Barrier( MPI_COMM_WORLD );

    start = MPI_Wtime();
    MPI_File mpi_f;
    MPI_File_open( MPI_COMM_WORLD, in_file, MPI_MODE_RDONLY, MPI_INFO_NULL, &mpi_f );
    MPI_File_read( mpi_f, data_buf, N, MPI_INT, MPI_STATUS_IGNORE );
    end = MPI_Wtime();
    mpi_io_millis += ( end - start ) * 1000;

    printf( "{\n\t\"id\": %d,\n\t\"seq_io\": %.3f,\n\t\"mpi_io\": %.3f\n}\n", rank, sq_io_millis, mpi_io_millis );

    MPI_Barrier( MPI_COMM_WORLD );
    MPI_Finalize();
    return 0;
}
