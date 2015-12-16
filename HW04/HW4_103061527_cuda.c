#include <stdio.h>
#include <stdlib.h>
#include <cuda_runtime.h>

#define INF MAX_INT

int index(int i, int j);

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
            if (i == j) Dist[index(i, j)] = 0;
            else        Dist[index(i, j)] = INF;
        }
    }

    while (--m >= 0) {
        int a, b, v;
        fscanf(infile, "%d %d %d", &a, &b, &v);
        --a, --b;
        Dist[index(i, j)] = v;
    }

    FILE *outfile = fopen(argv[2], "w");
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            if (Dist[index(i, j)] >= INF) fprintf(outfile, "INF ");
            else                          fprintf(outfile, "%d ", Dist[index(i, j)]);
        }
        fprintf(outfile, "\n");
    }
    return 0;
}

int index(int i, int j) {
    return i*N+j;
}
