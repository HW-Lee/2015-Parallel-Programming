#include <stdio.h>
#include <stdlib.h>
#include <cuda_runtime.h>

#define NUM_THREADS 500
#define INF (int) 1 << 29

int* Dist;
int* Dist_d;

__host__ __device__ int ij2ind(int i, int j, int N) {
    return i*N+j;
}

__global__ void updateList(int* list, int N, int k) {
    int idx = threadIdx.x + blockIdx.x * blockDim.x;
    int i = idx / N;
    int j = idx % N;
    if (i < N && j < N && list[ij2ind(i, j, N)] > list[ij2ind(i, k, N)] + list[ij2ind(k, j, N)])
        list[ij2ind(i, j, N)] = list[ij2ind(i, k, N)] + list[ij2ind(k, j, N)];
}

int main(int argc, char* argv[]) {
    FILE *infile = fopen(argv[1], "r");
    int m;
    int N;
    fscanf(infile, "%d %d", &N, &m);

    // TODO: Allocate memory (pinned/unpinned)
    // Dist = (int*) malloc(sizeof(int) * N*N);
    cudaMallocHost((void**) &Dist, sizeof(int) * N*N);
    cudaMalloc((void**) &Dist_d, sizeof(int) * N*N);

    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            if (i == j) Dist[ij2ind(i, j, N)] = 0;
            else        Dist[ij2ind(i, j, N)] = INF;
        }
    }

    while (--m >= 0) {
        int a, b, v;
        fscanf(infile, "%d %d %d", &a, &b, &v);
        --a, --b;
        Dist[ij2ind(a, b, N)] = v;
    }

    // TODO: Copy values loaded from the file
    cudaMemcpy((void*) Dist_d, (void*) Dist, sizeof(int) * N*N, cudaMemcpyHostToDevice);

    // TODO: Updating list
    int num_blocks = N*N / NUM_THREADS + 1;
    for (int k = 0; k < N; k++) {
        updateList<<< num_blocks, NUM_THREADS >>>(Dist_d, N, k);
    }

    // TODO: Copy final values
    cudaMemcpy((void*) Dist, (void*) Dist_d, sizeof(int) * N*N, cudaMemcpyDeviceToHost);

    FILE *outfile = fopen(argv[2], "w");
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            if (Dist[ij2ind(i, j, N)] >= INF) fprintf(outfile, "INF ");
            else                           fprintf(outfile, "%d ", Dist[ij2ind(i, j, N)]);
        }
        fprintf(outfile, "\n");
    }

    // TODO: Free memory
    cudaFreeHost(Dist);
    cudaFree(Dist_d);

    return 0;
}
