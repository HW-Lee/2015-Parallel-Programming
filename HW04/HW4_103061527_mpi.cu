#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cuda_runtime.h>
#include <mpi.h>

#define OUTFILE 1
#define INF (int) 1 << 24

__device__ __host__ int ij2ind(int i, int j, int N) {
    return i*N+j;
}

int* Dist;
int* Dist_d;

__global__ void updateList(int* D, int blocksize, int N, int r, int blockDimWidth, int phase, int offset) {
    int bi, bj;
    switch(phase) {
        case 0:
            bi = r;
            bj = r;
            break;
        case 1:
            if (blockIdx.x == 1) {
                bj = r;
                bi = (r + blockIdx.y + 1) % (int) ceil((double) N/blocksize);
            } else {
                bi = r;
                bj = (r + blockIdx.y + 1) % (int) ceil((double) N/blocksize);
            }
            break;
        case 2:
            bi = blockIdx.x + offset;
            bj = blockIdx.y;
            break;
    }
    extern __shared__ int DS[];

    int offset_i = blocksize * bi;
    int offset_j = blocksize * bj;
    int offset_r = blocksize * r;

    // DS[0:bibs-1][:] = B[bi][bj] = D[bibs:(bi+1)bs-1][bjbs:(bj+1)bs-1]
    // DS[bibs:2bibs-1][:] = B[bi][r] = D[bibs:(bi+1)bs-1][rbs:(r+1)bs-1]
    // DS[2bibs:3bibs-1][:] = B[r][bi] = D[rbs:(r+1)bs-1][bjbs:(bj+1)bs-1]
    for (int i = threadIdx.y; i < blocksize; i+=blockDimWidth) {
        for (int j = threadIdx.x; j < blocksize; j+=blockDimWidth) {
            DS[ij2ind(i, j, blocksize)] = D[ij2ind(i+offset_i, j+offset_j, N)];
            DS[ij2ind(i+blocksize, j, blocksize)] = D[ij2ind(i+offset_i, j+offset_r, N)];
            DS[ij2ind(i+2*blocksize, j, blocksize)] = D[ij2ind(i+offset_r, j+offset_j, N)];
        }
    }
    __syncthreads();

    // DS[i][j] = min{ DS[i][j], DS[i+bs][k] + DS[k+2bs][j] }
    for (int k = 0; k < blocksize; k++) {
        for (int i = threadIdx.y; i < blocksize; i+=blockDimWidth) {
            for (int j = threadIdx.x; j < blocksize; j+=blockDimWidth) {
                if (DS[ij2ind(i, j, blocksize)] > DS[ij2ind(i+blocksize, k, blocksize)] + DS[ij2ind(k+2*blocksize, j, blocksize)]) {
                    DS[ij2ind(i, j, blocksize)] = DS[ij2ind(i+blocksize, k, blocksize)] + DS[ij2ind(k+2*blocksize, j, blocksize)];
                    if (r == bi) DS[ij2ind(i+2*blocksize, j, blocksize)] = DS[ij2ind(i, j, blocksize)];
                    if (r == bj) DS[ij2ind(i+blocksize, j, blocksize)] = DS[ij2ind(i, j, blocksize)];
                }
            }
        }
        __syncthreads();
    }

    for (int i = threadIdx.y; i < blocksize; i+=blockDimWidth) {
        for (int j = threadIdx.x; j < blocksize; j+=blockDimWidth) {
            // DS[i][j] = D[i+bsbi][j+bsbj]
            D[ij2ind(i+offset_i, j+offset_j, N)] = DS[ij2ind(i, j, blocksize)];
        }
    }
}

int main(int argc, char* argv[]) {
    int dev = 0;
    cudaSetDevice(dev);
    cudaDeviceProp dp;
    cudaGetDeviceProperties(&dp, dev);

    int size, rank;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    cudaSetDevice(rank);
    int gpu_id = -1;
    cudaGetDevice(&gpu_id);
    printf("Thread%d gets GPU%d\n", rank, gpu_id);

    int blockDimWidth = (int) sqrt(dp.maxThreadsPerBlock);

    if (argc < 3) {
        printf("not enough arguments.\n");
        return 0;
    }

    int blocksize;
    int MAX_BLOCKSIZE = (int) sqrt(dp.sharedMemPerBlock/3.0/sizeof(int));

    if (argc >= 4) blocksize = atoi(argv[3]);
    else blocksize = blockDimWidth;

    if (blocksize > MAX_BLOCKSIZE) blocksize = MAX_BLOCKSIZE;

    // TODO: Read file and get meta 
    FILE *infile;
    int m;
    int N;
    int N_ext;

    if (rank == 0) {
        infile = fopen(argv[1], "r");
        fscanf(infile, "%d %d", &N, &m);
        // Send N: MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
        MPI_Send(&N, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
    } else {
         // Recv N: MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status)
        MPI_Recv(&N, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    if (blocksize > N) blocksize = N;
    if (blockDimWidth > blocksize) blockDimWidth = blocksize;
    N_ext = N + (blocksize - ((N-1) % blocksize + 1));

    dim3 block(blockDimWidth, blockDimWidth);
    if (rank == 0) printf("Blocking factor: %d\n", blocksize);

    // TODO: Allocate memory (pinned/unpinned)
    cudaMallocHost((void**) &Dist, sizeof(int) * N_ext*N_ext);
    cudaMalloc((void**) &Dist_d, sizeof(int) * N_ext*N_ext);

    if (rank == 0) {
        for (int i = 0; i < N_ext; ++i) {
            for (int j = 0; j < N_ext; ++j) {
                if (i == j) Dist[ij2ind(i, j, N_ext)] = 0;
                else        Dist[ij2ind(i, j, N_ext)] = INF;
            }
        }

        while (--m >= 0) {
            int a, b, v;
            fscanf(infile, "%d %d %d", &a, &b, &v);
            --a, --b;
            Dist[ij2ind(a, b, N_ext)] = v;
        }
        fclose(infile);
        // Send Dist: MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
        MPI_Send(Dist, N_ext*N_ext, MPI_INT, 1, 0, MPI_COMM_WORLD);
    } else {
        // Recv Dist: MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status)
        MPI_Recv(Dist, N_ext*N_ext, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    // TODO: Updating list
    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);

    float phase1elapsed_millis = 0;
    float phase2elapsed_millis = 0;
    float phase3elapsed_millis = 0;
    float t;

    int num_blocks_per_column = (int) ceil((double) N_ext/blocksize);
    dim3 grid_1(2, num_blocks_per_column-1);

    int num_blocks_per_thread = num_blocks_per_column / size;
    int row_offset = num_blocks_per_thread * rank * blocksize;
    if (rank == size-1)
        num_blocks_per_thread += num_blocks_per_column % size;

    dim3 grid_2(num_blocks_per_thread, num_blocks_per_column);

    int cpy_idx = ij2ind(row_offset, 0, N_ext);
    cudaMemcpy((void*) &Dist_d[cpy_idx], (void*) &Dist[cpy_idx], sizeof(int) * N_ext*blocksize*num_blocks_per_thread, cudaMemcpyDeviceToHost);

    for (int r = 0; r < num_blocks_per_column; r++) {
        if (rank == 0) printf("\rCompute progress: %.2f%%", (float) r/num_blocks_per_column*100);

        int r_idx = ij2ind(r * blocksize, 0, N_ext);
        cudaMemcpy((void*) &Dist_d[r_idx], (void*) &Dist[r_idx], sizeof(int) * N_ext * blocksize, cudaMemcpyHostToDevice);

        if (rank == 0) cudaEventRecord(start);
        updateList<<< 1, block, sizeof(int) * 3*blocksize*blocksize >>>(Dist_d, blocksize, N_ext, r, blockDimWidth, 0, -1);
        if (rank == 0) {
            cudaEventRecord(stop);
            cudaEventSynchronize(stop);
            cudaEventElapsedTime(&t, start, stop);
            phase1elapsed_millis += t;
        }

        if (rank == 0) cudaEventRecord(start);
        updateList<<< grid_1, block, sizeof(int) * 3*blocksize*blocksize >>>(Dist_d, blocksize, N_ext, r, blockDimWidth, 1, -1);
        if (rank == 0) {
            cudaEventRecord(stop);
            cudaEventSynchronize(stop);
            cudaEventElapsedTime(&t, start, stop);
            phase2elapsed_millis += t;
        }

        if (rank == 0) cudaEventRecord(start);

        updateList<<< grid_2, block, sizeof(int) * 3*blocksize*blocksize >>>(Dist_d, blocksize, N_ext, r, blockDimWidth, 2, row_offset/blocksize);

        if (rank == 0) {
            cudaEventRecord(stop);
            cudaEventSynchronize(stop);
            cudaEventElapsedTime(&t, start, stop);
            phase3elapsed_millis += t;
        }

        cudaMemcpy((void*) &Dist[cpy_idx], (void*) &Dist_d[cpy_idx], sizeof(int) * N_ext*blocksize*num_blocks_per_thread, cudaMemcpyDeviceToHost);

        if (rank == 0) printf("\rCompute progress: %.2f%%", (float) (r+1)/num_blocks_per_column*100);

        // Exchange the results
        // MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
        // MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status)
        if (rank == 0) {
            int send_idx = 0;
            int send_cnt = N_ext*blocksize*num_blocks_per_thread;
            int recv_idx = ij2ind(num_blocks_per_thread * blocksize, 0, N_ext);
            int recv_cnt = N_ext*blocksize*(num_blocks_per_thread + num_blocks_per_column % size);
            MPI_Send(&Dist[send_idx], send_cnt, MPI_INT, 1, 0, MPI_COMM_WORLD);
            MPI_Recv(&Dist[recv_idx], recv_cnt, MPI_INT, 1, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        } else {
            int recv_idx = 0;
            int recv_cnt = N_ext*blocksize*(num_blocks_per_thread - num_blocks_per_column % size);
            int send_idx = ij2ind((num_blocks_per_thread - num_blocks_per_column % size) * blocksize, 0, N_ext);
            int send_cnt = N_ext*blocksize*num_blocks_per_thread;
            MPI_Recv(&Dist[recv_idx], recv_cnt, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Send(&Dist[send_idx], send_cnt, MPI_INT, 0, 0, MPI_COMM_WORLD);
        }

        MPI_Barrier(MPI_COMM_WORLD);
    }
    printf("\n");

    // TODO: Write file
    if (OUTFILE == 1 && rank == 0 && argc <= 4) {
        printf("Writing the file...\n");
        FILE *outfile = fopen(argv[2], "w");
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < N; ++j) {
                if (Dist[ij2ind(i, j, N_ext)] >= INF) fprintf(outfile, "INF ");
                else                                  fprintf(outfile, "%d ", Dist[ij2ind(i, j, N_ext)]);
            }
            fprintf(outfile, "\n");
        }
        fclose(outfile);
    }

    printf("Process%d: phase_elapsed = (%.2f, %.2f, %.2f) ms\n", rank, phase1elapsed_millis, phase2elapsed_millis, phase3elapsed_millis);

    // TODO: Free memory
    cudaFreeHost(Dist);
    cudaFree(Dist_d);

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
    return 0;
}
