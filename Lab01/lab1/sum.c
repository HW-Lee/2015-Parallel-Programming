// Sample MPI Program for Parallel Programming 2015 Lab1
// You can reuse this code as a template for homework 1.
// Or you can write your own from scratch!
// It's up to you.

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define ROOT 0

int main (int argc, char *argv[]) {
	int rank, size;

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	if (argc < 3) {
		if (rank == ROOT) {
			fprintf(stderr, "Insufficient args\n");
			fprintf(stderr, "Usage: %s N input_file", argv[0]);
		}
		MPI_Barrier(MPI_COMM_WORLD);
		MPI_Finalize();
		return 0;
	}

	const int N = atoi(argv[1]);
	const char *inName = argv[2];

	int *root_arr; // for root process (which rank == 0) only

	// Part 1: Read file
	/* Note: You should deal with cases where (N < size) in Homework 1 */
	if (rank == ROOT) {
		root_arr = malloc(N * sizeof(int));
		FILE *f = fopen(inName, "rb");
		fread(root_arr, sizeof(int), N, f);
		fclose(f);
	}

	// Part 2: Scatter
	/* Note: You should deal with cases where (N % size != 0) in Homework 1 */
	int num_per_node = N / size;
	int *local_arr = malloc (num_per_node * sizeof(int));
	// Ref:
	// MPI_Scatter (send_buf, send_count, send_type, recv_buf, recv_count, recv_type, root, comm)
	MPI_Scatter    (root_arr, num_per_node, MPI_INT, local_arr, num_per_node, MPI_INT, ROOT, MPI_COMM_WORLD);

	if (rank == ROOT) {
		free (root_arr);
	}

	// Part 3: Calculate the sum of local_arr
	int sum = 0;
	int i;
	for (i = 0; i < num_per_node; i++) {
		sum += local_arr[i];
	}

	free (local_arr);

	// Part 4: Accumulate the result to root process
	int accumulated;
	// Ref:
	// MPI_Reduce (send_buf, recv_buf, count, data_type, op, root, comm)
	MPI_Reduce    (&sum, &accumulated, 1, MPI_INT, MPI_SUM, ROOT, MPI_COMM_WORLD);

	if (rank == ROOT) {
		printf("SUM: %d\n", accumulated);
	}

	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Finalize();

	return 0;
}
