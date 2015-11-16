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

typedef struct Tree {
	Body** elements;
	int num_elements;
	int layer;
	struct Tree** children;
	bool hasChild;
	double mass;
	Vec2 position;
	Vec2 region[2];
} BHTree;

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
bool* checklist;
BHTree* root;
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
Vec2 Force_getForce( int idx1, BHTree* node, double theta, double d );

/**
 *
 *	Tree
 *
 */
 void Tree_init();
 void* Tree_build_layer( void* ptr );
 void* Tree_build( void* ptr );
 void Tree_free( BHTree* node );


typedef struct {
	int num_threads;
	int del_t;
	int tid;
	int layer;
	double theta;
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
		p[i].layer = 0;
		p[i].theta = theta;
	}

	int init_layer = 0;
	while ( pow( 4, init_layer ) < num_threads && init_layer < 1 ) init_layer++;

	int num_nodes = pow( 4, init_layer );
	BHTree** nodes = (BHTree**) malloc( num_nodes * sizeof(BHTree*) );

	for (int iter = 0; iter < T; iter++) {
		Tree_init();

		for (int i = 0; i < num_nodes; i++)
			nodes[i] = root;

		for (int i = 0; i < init_layer; i++) {
			for (int j = 0; j < num_threads; j++) {
				p[j].layer = i;
				if ( j < pow( 4, i ) )
					pthread_create( &threads[j], NULL, Tree_build_layer, (void*) &p[j] );
			}
			for (int j = 0; j < num_threads; j++) {
				if ( j < pow( 4, i ) )
					pthread_join( threads[j], NULL );
			}
			for (int j = 0; j < num_nodes; j++) {
				nodes[j] = nodes[j]->children[ j / (num_nodes / (int)pow( 4, i+1 )) % 4 ];
			}
		}

		int offset = 0;
		while ( offset < num_nodes ) {
			for (int i = 0; i < num_threads; i++)
				if ( i + offset < num_nodes )
					pthread_create( &threads[i], NULL, Tree_build, (void*) nodes[ i+offset ] );

			for (int i = 0; i < num_threads; i++)
				if ( i + offset < num_nodes )
					pthread_join( threads[i], NULL );

			offset += num_threads;
		}
		
		// printf( "iter = %d\n", iter );

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

		Tree_free( root );
	}

	return 0;
}

void* move_bodies( void* ptr ) {
	Params* p = (Params*) ptr;
	int tid = p->tid;
	int num_threads = p->num_threads;
	double del_t = p->del_t;
	double theta = p->theta;

	double width = root->region[1].x - root->region[0].x;
	double height = root->region[1].y - root->region[0].y;

	double d = ( width > height ) ? width : height;

	if ( num_threads > num_bodies ) num_threads = num_bodies;
	if ( tid < num_threads ) {
		int start_idx = tid * ( num_bodies / num_threads );
		int end_idx = ( tid < num_threads - 1 ) ? start_idx + ( num_bodies / num_threads ) : num_bodies;

		for (int i = start_idx; i < end_idx; i++) {
			Vec2 F = Force_getForce( i, root, theta, d );
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

Vec2 Force_getForce( int idx1, BHTree* node, double theta, double d ) {
	Vec2 vec;
	vec.x = 0; vec.y = 0;

	Vec2 direction;
	direction.x = ( node->position.x - bodySet[idx1].position.x );
	direction.y = ( node->position.y - bodySet[idx1].position.y );
	double norm = Vec_norm( direction );

	if ( !node->hasChild || d/norm < theta ) {
		if ( norm < 1e-2 ) norm = 1e-2;
		double mag = 6.67408e-11 * bodySet[idx1].mass * node->mass / pow( norm, 3 );
		vec.x = direction.x * mag;
		vec.y = direction.y * mag;

		return vec;
	}

	for (int i = 0; i < 4; i++) {
		Vec2 f = Force_getForce( idx1, node->children[i], theta, d/2 );
		vec.x += f.x;
		vec.y += f.y;
	}

	return vec;
}

void Tree_init() {
	root = (BHTree*) malloc( sizeof(BHTree) );

	root->mass = 0;
	root->position.x = 0;
	root->position.y = 0;

	root->elements = (Body**) malloc( sizeof(Body*) * num_bodies );

	for (int i = 0; i < num_bodies; i++) {
		root->mass += bodySet[i].mass;
		root->position.x += bodySet[i].position.x * bodySet[i].mass;
		root->position.y += bodySet[i].position.y * bodySet[i].mass;
		root->elements[i] = &bodySet[i];
		if ( i == 0 ||  bodySet[i].position.x < root->region[0].x )
			root->region[0].x = bodySet[i].position.x;

		if ( i == 0 ||  bodySet[i].position.y < root->region[0].y )
			root->region[0].y = bodySet[i].position.y;

		if ( i == 0 ||  bodySet[i].position.x > root->region[1].x )
			root->region[1].x = bodySet[i].position.x;

		if ( i == 0 ||  bodySet[i].position.y > root->region[1].y )
			root->region[1].y = bodySet[i].position.y;
	}

	root->position.x /= root->mass;
	root->position.y /= root->mass;

	root->num_elements = num_bodies;
	root->layer = 0;
	root->hasChild = false;
}

void* Tree_build_layer( void* ptr ) {
	Params* p = (Params*) ptr;
	int tid = p->tid;
	int num_threads = p->num_threads;
	int layer = p->layer;

	int* idx;
	idx = (int*) malloc( layer * sizeof(int) );

	int dummy = tid;
	BHTree* node = root;
	for (int i = 0; i < layer; i++) {
		idx[ layer-i-1 ] = ( dummy % 4 );
		dummy /= 4;
	}
	for (int i = 0; i < layer; i++) {
		node = node->children[ idx[i] ];
	}

	node->mass = 0;
	node->position.x = 0;
	node->position.y = 0;

	if ( node->num_elements < 2 ) {
		if ( node->num_elements == 1 ) {
			node->mass = node->elements[0]->mass;
			node->position.x = node->elements[0]->position.x;
			node->position.y = node->elements[0]->position.y;
		}
		node->hasChild = false;
		free( node->elements );
		return 0;
	}

	node->children = (BHTree**) malloc( 4 * sizeof( BHTree* ) );
	int* counters = (int*) malloc( 4 * sizeof(int) );

	for (int i = 0; i < 4; i++) {
		node->children[i] = (BHTree*) malloc( sizeof(BHTree) );
		node->children[i]->elements = (Body**) malloc( node->num_elements * sizeof( Body* ) );
		counters[i] = 0;
	}
	node->hasChild = true;

	for (int i = 0; i < node->num_elements; i++) {
		node->mass += node->elements[i]->mass;
		node->position.x += node->elements[i]->position.x;
		node->position.y += node->elements[i]->position.y;
	}

	node->position.x /= node->num_elements;
	node->position.y /= node->num_elements;

	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 2; j++) {
			BHTree* child_node = node->children[ 2*i+j ];
			if ( i == 0 ) {
				child_node->region[0].x = (node->region[0].x + node->region[1].x) / 2;
				child_node->region[1].x = node->region[1].x;
			} else {
				child_node->region[0].x = node->region[0].x;
				child_node->region[1].x = (node->region[0].x + node->region[1].x) / 2;
			}
				if ( j == 0 ) {
				child_node->region[0].y = (node->region[0].y + node->region[1].y) / 2;
				child_node->region[1].y = node->region[1].y;
			} else {
				child_node->region[0].y = node->region[0].y;
				child_node->region[1].y = (node->region[0].y + node->region[1].y) / 2;
			}
		}
	}

	int i, j;
	Vec2 position;
	Vec2 center;
	center.x = (node->region[0].x + node->region[1].x) / 2;
	center.y = (node->region[0].y + node->region[1].y) / 2;
	for (int ii = 0; ii < node->num_elements; ii++) {
		position.x = node->elements[ii]->position.x;
		position.y = node->elements[ii]->position.y;
		i = ( position.x > center.x ) ? 0 : 1;
		j = ( position.y > center.y ) ? 0 : 1;

		node->children[ 2*i+j ]->elements[ counters[ 2*i+j ]++ ] = node->elements[ii];
	}

	for (int ii = 0; ii < 4; ii++) {
		node->children[ii]->num_elements = counters[ii];
	}

	free( counters );
	free( idx );
}

void* Tree_build( void* ptr ) {
	BHTree* node = (BHTree*) ptr;

	node->mass = 0;
	node->position.x = 0;
	node->position.y = 0;

	if ( node->num_elements < 2 ) {
		if ( node->num_elements == 1 ) {
			node->mass = node->elements[0]->mass;
			node->position.x = node->elements[0]->position.x;
			node->position.y = node->elements[0]->position.y;
		}
		node->hasChild = false;
		free( node->elements );
		return 0;
	}

	node->children = (BHTree**) malloc( 4 * sizeof( BHTree* ) );
	int* counters = (int*) malloc( 4 * sizeof(int) );

	for (int i = 0; i < 4; i++)
		node->children[i] = (BHTree*) malloc( sizeof(BHTree) );

	for (int i = 0; i < 4; i++) {
		node->children[i]->elements = (Body**) malloc( node->num_elements * sizeof( Body* ) );
		counters[i] = 0;
	}
	node->hasChild = true;

	for (int i = 0; i < node->num_elements; i++) {
		node->mass += node->elements[i]->mass;
		node->position.x += node->elements[i]->position.x;
		node->position.y += node->elements[i]->position.y;
	}

	node->position.x /= node->num_elements;
	node->position.y /= node->num_elements;

	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 2; j++) {
			BHTree* child_node = node->children[ 2*i+j ];
			if ( i == 0 ) {
				child_node->region[0].x = (node->region[0].x + node->region[1].x) / 2;
				child_node->region[1].x = node->region[1].x;
			} else {
				child_node->region[0].x = node->region[0].x;
				child_node->region[1].x = (node->region[0].x + node->region[1].x) / 2;
			}
				if ( j == 0 ) {
				child_node->region[0].y = (node->region[0].y + node->region[1].y) / 2;
				child_node->region[1].y = node->region[1].y;
			} else {
				child_node->region[0].y = node->region[0].y;
				child_node->region[1].y = (node->region[0].y + node->region[1].y) / 2;
			}
		}
	}

	int i, j;
	Vec2 position;
	Vec2 center;
	center.x = (node->region[0].x + node->region[1].x) / 2;
	center.y = (node->region[0].y + node->region[1].y) / 2;
	for (int ii = 0; ii < node->num_elements; ii++) {
		position.x = node->elements[ii]->position.x;
		position.y = node->elements[ii]->position.y;
		i = ( position.x > center.x ) ? 0 : 1;
		j = ( position.y > center.y ) ? 0 : 1;

		node->children[ 2*i+j ]->elements[ counters[ 2*i+j ]++ ] = node->elements[ii];
	}

	for (int ii = 0; ii < 4; ii++) {
		node->children[ii]->num_elements = counters[ii];
		Tree_build( (void*) node->children[ii] );
	}

	free( counters );
}

void Tree_free( BHTree* node ) {
	if ( node->hasChild ) {
		for (int i = 0; i < 4; i++)
			Tree_free( node->children[i] );
		free( node->children );
	}
}
