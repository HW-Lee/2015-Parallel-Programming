#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <omp.h>
#include <math.h>
#include <X11/Xlib.h>
#include <math.h>
#include <unistd.h>
#include <time.h>

#define ITERATION_MAX 65536
bool DRAW_RESULT = false;

typedef struct {
    double real;
    double imag;
} complex;

inline complex mult( complex c1, complex c2 );
inline complex add( complex c1, complex c2 );
inline complex conj( complex c );

inline int mandelbrot_iter( complex c );
inline double sigmoid( double v );

struct timespec ref_time;

/**
 *
 * Time
 *
 */
void tic( struct timespec *ref_time );
double toc( struct timespec *ref_time );


int main( int argc, char* argv[] ) {

    int rank, size;
    double comm_millis = 0;

    MPI_Init( &argc, &argv );
    MPI_Comm_size( MPI_COMM_WORLD, &size );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );

    // ./a.out nthreads real_min real_max imag_min imag_max width height enable/disable
    int    nthreads = atoi( argv[1] );
    double real_min = atof( argv[2] );
    double real_max = atof( argv[3] );
    double imag_min = atof( argv[4] );
    double imag_max = atof( argv[5] );
    int    width    = atoi( argv[6] );
    int    height   = atoi( argv[7] );

    if ( strcmp( argv[8], "enable" ) == 0 ) DRAW_RESULT = true;

    Display *display;
    Window window;      //initialization for a window
    int screen;         //which screen 

    GC gc;
    XGCValues values;
    long valuemask = 0;

    int* glob_buffer;
    int* local_buffer;
    int local_buffer_size = width * height / size;

    if ( rank == 0 ) glob_buffer = (int *) malloc( width * height * sizeof(int) );
    if ( rank < size-1 ) local_buffer = (int *) malloc( local_buffer_size * sizeof(int) );
    else local_buffer = (int *) malloc( (local_buffer_size + (width * height) % size) * sizeof(int) );

    int start_idx = rank * local_buffer_size;
    int end_idx = ( rank < size-1 ) ? start_idx + local_buffer_size : width * height;

    #pragma omp parallel num_threads(nthreads)
    {
        int worker_id = rank * nthreads + omp_get_thread_num();
        double comp_millis = 0;
        double sync_millis = 0;
        struct timespec ref_time_local;

        complex z;
        tic( &ref_time_local );

        #pragma omp for schedule(dynamic) nowait
        for (int i = start_idx; i < end_idx; i++) {
            int x = i / height;
            int y = i % height;
            z.real = (real_max - real_min) * (double) x / width + real_min;
            z.imag = (imag_max - imag_min) * (double) y / height + imag_min;

            local_buffer[i - start_idx] = mandelbrot_iter(z);
        }
        comp_millis += toc( &ref_time_local );

        tic( &ref_time_local );
        #pragma omp barrier
        sync_millis += toc( &ref_time_local );

        printf( "{\n\t\"id\": %d,\n\t\"comp_millis\": %lf,\n\t\"sync_millis\": %lf\n}\n", worker_id, comp_millis, sync_millis );
    }

    tic( &ref_time );
    // MPI_Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm)
    MPI_Gather( local_buffer, local_buffer_size, MPI_INT, glob_buffer, local_buffer_size, MPI_INT, 0, MPI_COMM_WORLD );
    comm_millis += toc( &ref_time );

    if ( (width * height) % size > 0 ) {
        tic( &ref_time );
        if ( rank == size-1 )
            MPI_Send( &( local_buffer[local_buffer_size] ), (width * height) % size, MPI_INT, 0, 0, MPI_COMM_WORLD );
        if ( rank == 0 )
            MPI_Recv( &( glob_buffer[local_buffer_size * size] ), (width * height) % size, MPI_INT, size-1, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE );
        comm_millis += toc( &ref_time );
    }

    if ( rank == 0 && DRAW_RESULT ) {
        display = XOpenDisplay(NULL);
        if (display == NULL) printf( "GG\n" );
        screen = DefaultScreen(display);
        int x = 0, y = 0, border_width = 0;
        window = XCreateSimpleWindow(display, RootWindow(display, screen), x, y, width, height, border_width,
                    BlackPixel(display, screen), WhitePixel(display, screen));

        gc = XCreateGC(display, window, valuemask, &values);
        XSetForeground(display, gc, BlackPixel (display, screen));
        XSetBackground(display, gc, 0X0000FF00);
        XSetLineAttributes (display, gc, 1, LineSolid, CapRound, JoinRound);

        XMapWindow(display, window);
        XSync(display, 0);

        for (int i = 0; i < width; i++) {
            for (int j = 0; j < height; j++) {
                int color = 64 * 1024 * (int) ( (2 * sigmoid( logl(glob_buffer[ i * height + j ]) ) - 1) * 255.0 );
                color |= color >> 8 | color >> 16;
                XSetForeground(display, gc, color );    
                XDrawPoint(display, window, gc, i, j);
            }
        }

        XFlush(display);
        sleep(10);
    }

    if ( rank == 0 ) free(glob_buffer);

    MPI_Barrier( MPI_COMM_WORLD );
    MPI_Finalize();
    return 0;
}

complex mult( complex c1, complex c2 ) {
    complex result;
    result.real = c1.real * c2.real - c1.imag * c2.imag;
    result.imag = c1.real * c2.imag + c1.imag * c2.real;
    return result;
}

complex add( complex c1, complex c2 ) {
    complex result;
    result.real = c1.real + c2.real;
    result.imag = c1.imag + c2.imag;
    return result;
}

complex conj( complex c ) {
    complex result;
    result.real = c.real;
    result.imag = -c.imag;
    return result;
}

int mandelbrot_iter( complex c ) {
    complex z;
    z.real = 0; z.imag = 0;

    int iter = 0;
    double norm_sq = 0;
    while ( iter < ITERATION_MAX && norm_sq < 4 ) {
        z = add( mult( z, z ), c );
        norm_sq = mult( z, conj( z ) ).real;
        iter++;
    }

    return iter;
}

double sigmoid( double v ) {
    return 1 / (1 + pow(2, -v));
}

void tic( struct timespec *ref_time ) {
    clock_gettime( CLOCK_REALTIME, ref_time );
}

double toc( struct timespec *ref_time ) {
    struct timespec now;
    clock_gettime( CLOCK_REALTIME, &now );
    return (double) ( (now.tv_sec*1e3 + now.tv_nsec*1e-6) - (ref_time->tv_sec*1e3 + ref_time->tv_nsec*1e-6) );
}
