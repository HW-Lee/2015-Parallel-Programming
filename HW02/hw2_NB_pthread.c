#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <X11/Xlib.h>
#include <math.h>
#include <unistd.h>

typedef struct {
	double x;
	double y;
} Vec2;

typedef struct {
	double mass;
	Vec2 position;
	Vec2 velocity;
} Body;

typedef struct {
	GC gc;
	Window window;
	Display* display;
	int screen;
	double x_min;
	double y_min;
	double length;
	int width_px;
	int height_px;
	int length_per_px;
} DispManager;

DispManager manager;
Body* bodySet;
int num_bodies;
pthread_barrier_t bar;

/**
 *
 *	Display Manager
 *
 */
void DispManager_init( double x_min, double y_min, double length, int length_px );
void DispManager_int_init( int width_px, int height_px );
void DispManager_clear();
void DispManager_draw( double x, double y );
void DispManager_int_draw( int x, int y );
void DispManager_drawline( double x1, double y1, double x2, double y2 );
void DispManager_int_drawline( int x1, int y1, int x2, int y2 );
void DispManager_flush();

/**
 *
 *	Vec
 *
 */
double Vec_norm( Vec2 vec );

/**
 *
 *	Force
 *
 */
Vec2 Force_getForce( int idx1, int idx2 );


typedef struct {
	int num_threads;
	int del_t;
	int tid;
} Params;

void* move_bodies( void* ptr );

int main( int argc, char* argv[] ) {

	// ./hw2_NB_openmp #threads m T t FILE theta enable/disable x_min y_min length Length
	int    num_threads = atoi( argv[1] );
	double m 		   = atof( argv[2] );
	int    T 		   = atoi( argv[3] );
	double t 		   = atof( argv[4] );
	char*  in_file	   = argv[5];
	float  theta 	   = atof( argv[6] );
	char*  xwin_en	   = argv[7];
	float  x_min;
	float  y_min;
	float  l_coor;
	int    l_xwin;

	if ( strcmp( xwin_en, "enable" ) == 0 ) {
		   x_min 	   = atof( argv[8] );
		   y_min 	   = atof( argv[9] );
		   l_coor 	   = atof( argv[10] );
		   l_xwin 	   = atoi( argv[11] );
	}

	pthread_barrier_init( &bar, NULL, num_threads );

	FILE* f = fopen( in_file, "rt" );
	char line[100];

	fgets( line, 100, f );
	sscanf( line, "%d", &num_bodies );

	bodySet = (Body*) malloc( sizeof(Body) * num_bodies );
	for (int i = 0; i < num_bodies; i++) {
		fgets( line, 100, f );
		sscanf( line, "%lf %lf %lf %lf", &bodySet[i].position.x, &bodySet[i].position.y, &bodySet[i].velocity.x, &bodySet[i].velocity.y );
		bodySet[i].mass = m;
	}
	fclose( f );

	if ( strcmp( xwin_en, "enable" ) == 0 )
		DispManager_init( x_min, y_min, l_coor, l_xwin );

	pthread_t* threads = (pthread_t*) malloc( sizeof( pthread_t ) * num_threads );

	Params* p = (Params*) malloc( sizeof( Params ) * num_threads );
	for (int i = 0; i < num_threads; i++) {
		p[i].num_threads = num_threads;
		p[i].del_t = t;
		p[i].tid = i;
	}

	for (int iter = 0; iter < T; iter++) {
		printf( "iter = %d\n", iter );

		for (int i = 0; i < num_threads; i++)
			pthread_create( &threads[i], NULL, move_bodies, (void*) &p[i] );

		for (int i = 0; i < num_threads; i++)
			pthread_join( threads[i], NULL );

		if ( strcmp( xwin_en, "enable" ) == 0 ) {
			DispManager_clear();

			for (int i = 0; i < num_bodies; i++)
				DispManager_draw( bodySet[i].position.x, bodySet[i].position.y );

			DispManager_flush();
		}
	}

	return 0;
}

void* move_bodies( void* ptr ) {
	Params* p = (Params*) ptr;
	int tid = p->tid;
	int num_threads = p->num_threads;
	double del_t = p->del_t;

	if ( num_threads > num_bodies ) num_threads = num_bodies;
	if ( tid < num_threads ) {
		int start_idx = tid * ( num_bodies / num_threads );
		int end_idx = ( tid < num_threads - 1 ) ? start_idx + ( num_bodies / num_threads ) : num_bodies;

		for (int i = start_idx; i < end_idx; i++) {
			Vec2 F;
			F.x = 0; F.y = 0;
			for (int j = 0; j < num_bodies; j++) {
				if ( i != j ) {
					Vec2 force = Force_getForce( i, j );
					F.x += force.x;
					F.y += force.y;
				}
			}
			bodySet[i].velocity.x += F.x / bodySet[i].mass * del_t;
			bodySet[i].velocity.y += F.y / bodySet[i].mass * del_t;
		}

		pthread_barrier_wait( &bar );

		for (int i = start_idx; i < end_idx; i++) {
			bodySet[i].position.x += bodySet[i].velocity.x * del_t;
			bodySet[i].position.y += bodySet[i].velocity.y * del_t;
		}
	}
}



void DispManager_init( double x_min, double y_min, double length, int length_px ) {
	manager.x_min = x_min;
	manager.y_min = y_min;
	manager.length = length;
	manager.width_px = length_px;
	manager.height_px = length_px;
	manager.length_per_px = (int) (length_px / length);
	DispManager_int_init( length_px, length_px );
}

void DispManager_int_init( int width_px, int height_px ) {
	manager.display = XOpenDisplay( NULL );
	if ( manager.display == NULL ) {
		printf( "Failed to open the display.\n" );
		return;
	}
	manager.screen = DefaultScreen( manager.display );
	// set window position
	int x = 0, y = 0;
	// border width in pixels
	int border_width = 0;
	// create the window
	manager.window = 
		XCreateSimpleWindow( 
							manager.display,
							RootWindow( manager.display, manager.screen ), 
							x, y, width_px, height_px, border_width, 
							BlackPixel( manager.display, manager.screen ),
							WhitePixel( manager.display, manager.screen )
							);
	DispManager_clear();
}

void DispManager_clear() {
	// create the graph
	XGCValues values;
	long valuemask = 0;

	manager.gc = XCreateGC( manager.display, manager.window, valuemask, &values );

	XSetForeground( manager.display, manager.gc, BlackPixel( manager.display, manager.screen ) );
	XSetBackground( manager.display, manager.gc, 0X0000FF00 );
	XSetLineAttributes( manager.display, manager.gc, 1, LineSolid, CapRound, JoinRound );
	
	/* map(show) the window */
	XMapWindow( manager.display, manager.window );
	XSync( manager.display, 0 );

	/* draw rectangle */
	XSetForeground( manager.display, manager.gc, BlackPixel( manager.display, manager.screen ) );
	XFillRectangle( manager.display, manager.window, manager.gc, 0, 0, manager.width_px, manager.height_px );
	XFlush( manager.display );
}

void DispManager_draw( double x, double y ) {
	int x_px = (int) ( (x - manager.x_min) * manager.length_per_px );
	int y_px = (int) ( (y - manager.y_min) * manager.length_per_px );

	if (x_px < 0 || y_px < 0) return;
	DispManager_int_draw( x_px, y_px );
}

void DispManager_int_draw( int x, int y ) {
	XSetForeground( manager.display, manager.gc, WhitePixel( manager.display, manager.screen ) );
	XDrawPoint( manager.display, manager.window, manager.gc, x, y );
}

void DispManager_drawline( double x1, double y1, double x2, double y2 ) {
	int x1_px = (int) ( (x1 - manager.x_min) * manager.length_per_px );
	int y1_px = (int) ( (y1 - manager.y_min) * manager.length_per_px );
	int x2_px = (int) ( (x2 - manager.x_min) * manager.length_per_px );
	int y2_px = (int) ( (y2 - manager.y_min) * manager.length_per_px );

	if ( x1_px < 0 || y1_px < 0 || x2_px < 0 || y2_px < 0 ) return;
	DispManager_int_drawline( x1_px, y1_px, x2_px, y2_px );
}

void DispManager_int_drawline( int x1, int y1, int x2, int y2 ) {
	XSetForeground( manager.display, manager.gc, WhitePixel( manager.display, manager.screen ) );
	XDrawLine( manager.display, manager.window, manager.gc, x1, y1, x2, y2 );
}

void DispManager_flush() {
	XFlush( manager.display );
}

double Vec_norm( Vec2 vec ) {
	return sqrt( pow( vec.x, 2 ) + pow( vec.y, 2 ) );
}

Vec2 Force_getForce( int idx1, int idx2 ) {
	Vec2 vec;
	Vec2 direction;
	direction.x = ( bodySet[idx2].position.x - bodySet[idx1].position.x );
	direction.y = ( bodySet[idx2].position.y - bodySet[idx1].position.y );

	double norm = Vec_norm( direction );
	if ( norm < 1e-2 ) norm = 1e-2;
	double mag = 6.67408e-11 * bodySet[idx1].mass * bodySet[idx2].mass / pow( norm, 3 );
	vec.x = direction.x * mag;
	vec.y = direction.y * mag;

	return vec;
}
