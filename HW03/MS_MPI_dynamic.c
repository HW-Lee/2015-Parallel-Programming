#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <math.h>
#include <X11/Xlib.h>
#include <math.h>
#include <unistd.h>
#include <time.h>

#define ITERATION_MAX 65536
#define TAG_DOOPERATION 0
#define TAG_TERMINATE 1
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
void tic();
double toc();


int main( int argc, char* argv[] ) {

	int rank, size;
	double comp_millis = 0;
	double sync_millis = 0;
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
	int    width 	= atoi( argv[6] );
	int    height 	= atoi( argv[7] );

	if ( strcmp( argv[8], "enable" ) == 0 ) DRAW_RESULT = true;

	Display *display;
	Window window;      //initialization for a window
	int screen;         //which screen 

	GC gc;
	XGCValues values;
	long valuemask = 0;

	int* glob_buffer;

	if ( rank == 0 ) glob_buffer = (int *) malloc( width * height * sizeof(int) );

	if ( size == 1 ) {

		tic();
		complex z;
		for (int i = 0; i < width * height; i++) {
			int x = i / height;
			int y = i % height;
			z.real = (real_max - real_min) * (double) x / width + real_min;
			z.imag = (imag_max - imag_min) * (double) y / height + imag_min;

			glob_buffer[i] = mandelbrot_iter(z);
		}
		comp_millis += toc();

	} else {

		// MPI_Send(sendbuf, sendcount, sendtype, dest, tag, comm)
		// MPI_Recv(recvbuf, recvcount, recvtype, src, tag, comm, status)
		// MPI_Iprob(src, tag, comm, flag, status)

		if ( rank == 0 ) {

			// Master
			int num_termination_sent = 0;
			int num_pending = width;
			int flag, src;
			MPI_Status status;
			int colIdx;
			int colIdx_send;
			for (int i = 1; i < size; i++) {
				colIdx_send = width - num_pending;
				num_pending--;
				int tag = ( colIdx_send < 0 ) ? TAG_TERMINATE : TAG_DOOPERATION;

				tic();
				MPI_Send( &colIdx_send, 1, MPI_INT, i, tag, MPI_COMM_WORLD );
				comm_millis += toc();
			}

			while (true) {
				MPI_Iprobe( MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status );

				if ( flag ) {
					sync_millis += toc();

					src = status.MPI_SOURCE;

					tic();
					MPI_Recv( &(glob_buffer[status.MPI_TAG * height]), height, MPI_INT, src, MPI_ANY_TAG, MPI_COMM_WORLD, &status );

					if ( num_pending > 0 ) {
						colIdx_send = width - num_pending;
						num_pending--;
						MPI_Send( &colIdx_send, 1, MPI_INT, src, TAG_DOOPERATION, MPI_COMM_WORLD );
						comm_millis += toc();
					} else {
						MPI_Send( &colIdx_send, 1, MPI_INT, src, TAG_TERMINATE, MPI_COMM_WORLD );
						comm_millis += toc();
						num_termination_sent++;
						if ( num_termination_sent == size-1 ) break;
					}

					tic();
				}
			}

		} else {
			
			// Slave
			int master = 0;
			MPI_Status status;
			int x;
			int* col_iters_send = (int *) malloc( height * sizeof(int) );

			complex z;
			while (true) {
				tic();
				MPI_Recv( &x, 1, MPI_INT, master, MPI_ANY_TAG, MPI_COMM_WORLD, &status );
				comm_millis += toc();

				if ( status.MPI_TAG == TAG_TERMINATE ) break;

				tic();
				for (int y = 0; y < height; y++) {
					z.real = (real_max - real_min) * (double) x / width + real_min;
					z.imag = (imag_max - imag_min) * (double) y / height + imag_min;

					col_iters_send[y] = mandelbrot_iter(z);
				}
				comp_millis += toc();

				tic();
				MPI_Send( col_iters_send, height, MPI_INT, master, x, MPI_COMM_WORLD );
				comm_millis += toc();
			}
			
		}

	}

	tic();
	MPI_Barrier( MPI_COMM_WORLD );
	sync_millis += toc();

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

	printf( "{\n\t\"id\": %d,\n\t\"comp_millis\": %lf,\n\t\"sync_millis\": %lf,\n\t\"comm_millis\": %lf\n}\n", rank, comp_millis, sync_millis, comm_millis );

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

void tic() {
	clock_gettime( CLOCK_REALTIME, &ref_time );
}

double toc() {
	struct timespec now;
	clock_gettime( CLOCK_REALTIME, &now );
	return (double) ( (now.tv_sec*1e3 + now.tv_nsec*1e-6) - (ref_time.tv_sec*1e3 + ref_time.tv_nsec*1e-6) );
}
