#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

complex mult( complex c1, complex c2 );
complex add( complex c1, complex c2 );
complex conj( complex c );

int mandelbort_iter( complex c );

struct timespec ref_time;

/**
 *
 * Time
 *
 */
void tic();
double toc();

int main( int argc, char* argv[] ) {

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

	glob_buffer = (int *) malloc( width * height * sizeof(int) );

	#pragma omp parallel num_threads(nthreads)
	{
		complex z;
		#pragma omp for schedule(dynamic, 1) nowait
		for (int x = 0; x < width; x++) {
			printf( "Thread%d processes column %d\n", omp_get_thread_num(), x );
			for (int y = 0; y < height; y++) {
				z.real = (real_max - real_min) * (double) x / width + real_min;
				z.imag = (imag_max - imag_min) * (double) y / height + imag_min;

				glob_buffer[ x * height + y ] = mandelbort_iter(z);
			}
		}
	}

	if ( DRAW_RESULT ) {
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
				int color = 1024 * 1024 * (int) (glob_buffer[i * height + j] % 256);
				color |= color >> 8 | color >> 16;
				XSetForeground(display, gc, color );	
				XDrawPoint(display, window, gc, i, j);
			}
		}

		XFlush(display);
		sleep(10);
	}

	free(glob_buffer);
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

int mandelbort_iter( complex c ) {
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

void tic() {
	clock_gettime( CLOCK_REALTIME, &ref_time );
}

double toc() {
	struct timespec now;
	clock_gettime( CLOCK_REALTIME, &now );
	return (double) ( (now.tv_sec*1e3 + now.tv_nsec*1e-6) - (ref_time.tv_sec*1e3 + ref_time.tv_nsec*1e-6) );
}

