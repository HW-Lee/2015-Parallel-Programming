#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cuda_runtime.h>

#define OUTFILE 1
#define INF (int) 1 << 24

__device__ __host__ int ij2ind(int i, int j, int N) {
    return i*N+j;
}

int* Dist;
int* Dist_d;

__global__ void updateList(int* D, int blocksize, int N, int r, int blockDimWidth, int phase) {
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
            bi = (r + blockIdx.y + 1) % (int) ceil((double) N/blocksize);
            bj = (r + blockIdx.x + 1) % (int) ceil((double) N/blocksize);
            break;
    }
    extern __shared__ int DS[];

    int offset_i = blocksize * bi;
    int offset_j = blocksize * bj;
    int offset_r = blocksize * r;

    // DS[0:bibs-1][:] = B[bi][bj] = D[bibs:(bi+1)bs-1][bjbs:(bj+1)bs-1]
    // DS[bibs:2bibs-1][:] = B[bi][r] = D[bibs:(bi+1)bs-1][rbs:(r+1)bs-1]
    // DS[2bibs:3bibs-1][:] = B[r][bi] = D[rbs:(r+1)bs-1][bjbs:(bj+1)bs-1]
    for (int i = threadIdx.y; i < blocksize && i+offset_i < N; i+=blockDimWidth) {
        for (int j = threadIdx.x; j < blocksize && j+offset_j < N; j+=blockDimWidth) {
            DS[ij2ind(i, j, blocksize)] = D[ij2ind(i+offset_i, j+offset_j, N)];
            DS[ij2ind(i+blocksize, j, blocksize)] = D[ij2ind(i+offset_i, j+offset_r, N)];
            DS[ij2ind(i+2*blocksize, j, blocksize)] = D[ij2ind(i+offset_r, j+offset_j, N)];
        }
    }
    __syncthreads();

    // DS[i][j] = min{ DS[i][j], DS[i+bs][k] + DS[k+2bs][j] }
    for (int k = 0; k < blocksize; k++) {
        for (int i = threadIdx.y; i < blocksize && i+offset_i < N; i+=blockDimWidth) {
            for (int j = threadIdx.x; j < blocksize && j+offset_j < N; j+=blockDimWidth) {
                if (DS[ij2ind(i, j, blocksize)] > DS[ij2ind(i+blocksize, k, blocksize)] + DS[ij2ind(k+2*blocksize, j, blocksize)]) {
                    DS[ij2ind(i, j, blocksize)] = DS[ij2ind(i+blocksize, k, blocksize)] + DS[ij2ind(k+2*blocksize, j, blocksize)];
                    if (r == bi) DS[ij2ind(i+2*blocksize, j, blocksize)] = DS[ij2ind(i, j, blocksize)];
                    if (r == bj) DS[ij2ind(i+blocksize, j, blocksize)] = DS[ij2ind(i, j, blocksize)];
                }
            }
        }
        __syncthreads();
    }

    for (int i = threadIdx.y; i < blocksize && i+offset_i < N; i+=blockDimWidth) {
        for (int j = threadIdx.x; j < blocksize && j+offset_j < N; j+=blockDimWidth) {
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

    // printf("dp.sharedMemPerBlock = %d\n", dp.sharedMemPerBlock);
    // printf("dp.maxThreadsPerBlock = %d\n", dp.maxThreadsPerBlock);
    // printf("dp.maxThreadsDim = (%d, %d, %d)\n", dp.maxThreadsDim[0], dp.maxThreadsDim[1], dp.maxThreadsDim[2]);
    // printf("dp.maxGridSize = (%d, %d, %d)\n", dp.maxGridSize[0], dp.maxGridSize[1], dp.maxGridSize[2]);

    int blockDimWidth = (int) sqrt(dp.maxThreadsPerBlock);
    dim3 block(blockDimWidth, blockDimWidth);

    if (argc < 3) {
        printf("not enough arguments.\n");
        return 0;
    }

    int blocksize;
    int MAX_BLOCKSIZE = (int) sqrt(dp.sharedMemPerBlock/3.0/sizeof(int));
    if (argc >= 4) blocksize = atoi(argv[3]);
    else blocksize = blockDimWidth;

    if (blocksize > MAX_BLOCKSIZE) blocksize = MAX_BLOCKSIZE;

    printf("Blocking factor: %d\n", blocksize);

    // TODO: Read file and get meta data
    FILE *infile = fopen(argv[1], "r");
    int m;
    int N;
    fscanf(infile, "%d %d", &N, &m);

    if (blocksize > N) blocksize = N;
    int N_ext = N + (blocksize - N % blocksize);

    // TODO: Allocate memory (pinned/unpinned)
    cudaMallocHost((void**) &Dist, sizeof(int) * N_ext*N_ext);
    cudaMalloc((void**) &Dist_d, sizeof(int) * N_ext*N_ext);

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

    // TODO: Copy values loaded from the file
    cudaMemcpy((void*) Dist_d, (void*) Dist, sizeof(int) * N_ext*N_ext, cudaMemcpyHostToDevice);

    // TODO: Updating list
    int num_blocks_per_column = (int) ceil((double) N_ext/blocksize);
    dim3 grid_1(2, num_blocks_per_column-1);
    dim3 grid_2(num_blocks_per_column-1, num_blocks_per_column-1);
    for (int r = 0; r < num_blocks_per_column; r++) {
        printf("\rCompute progress: %.2f%%", (float) r/num_blocks_per_column*100);
        updateList<<< 1, block, sizeof(int) * 3*blocksize*blocksize >>>(Dist_d, blocksize, N_ext, r, blockDimWidth, 0);
        updateList<<< grid_1, block, sizeof(int) * 3*blocksize*blocksize >>>(Dist_d, blocksize, N_ext, r, blockDimWidth, 1);
        updateList<<< grid_2, block, sizeof(int) * 3*blocksize*blocksize >>>(Dist_d, blocksize, N_ext, r, blockDimWidth, 2);
        cudaMemcpy((void*) Dist, (void*) Dist_d, sizeof(int), cudaMemcpyDeviceToHost);
        printf("\rCompute progress: %.2f%%", (float) (r+1)/num_blocks_per_column*100);
    }
    printf("\n");

    // TODO: Copy final values
    cudaMemcpy((void*) Dist, (void*) Dist_d, sizeof(int) * N_ext*N_ext, cudaMemcpyDeviceToHost);

    // TODO: Write file
    if (OUTFILE == 1) {
        printf("Writing the file...\n");
        FILE *outfile = fopen(argv[2], "w");
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < N; ++j) {
                if (Dist[ij2ind(i, j, N_ext)] >= INF) fprintf(outfile, "INF ");
                else                                  fprintf(outfile, "%d ", Dist[ij2ind(i, j, N_ext)]);
            }
            fprintf(outfile, "\n");
        }
    }

    // TODO: Free memory
    cudaFreeHost(Dist);
    cudaFree(Dist_d);

    return 0;
}
