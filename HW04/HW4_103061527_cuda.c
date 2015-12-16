#include <stdio.h>
#include <stdlib.h>
#include <cuda_runtime.h>

#define INF (int) 1 << 30

int ij2ind(int i, int j);

int N;
int* Dist;
int* Dist_cm;

int main(int argc, char* argv[]) {
    FILE *infile = fopen(argv[1], "r");
    int m;
    fscanf(infile, "%d %d", &N, &m);

    Dist = (int*) malloc(sizeof(int) * N * N);

    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            if (i == j) Dist[ij2ind(i, j)] = 0;
            else        Dist[ij2ind(i, j)] = INF;
        }
    }

    while (--m >= 0) {
        int a, b, v;
        fscanf(infile, "%d %d %d", &a, &b, &v);
        --a, --b;
        Dist[ij2ind(a, b)] = v;
    }

    FILE *outfile = fopen(argv[2], "w");
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            if (Dist[ij2ind(i, j)] >= INF) fprintf(outfile, "INF ");
            else                           fprintf(outfile, "%d ", Dist[ij2ind(i, j)]);
        }
        fprintf(outfile, "\n");
    }
    return 0;
}

int ij2ind(int i, int j) {
    return i*N+j;
}
