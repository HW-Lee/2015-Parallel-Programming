#include <iostream>      /* Input/Output */
#include <stdlib.h>     /* General Utilities */
#include <pthread.h>    /* POSIX Threads */
#include <stdio.h>
#include <string.h>
#include <sstream>
#include <fstream>
#include <vector>

#include "Vec.h"
#include "Body.h"
#include "GravForce.h"
#include "DispManager.h"
#include "ComputeParams.h"

using namespace std;

void* move_bodies( void* ptr );

vector<Body> bodySet;
pthread_barrier_t bar;

int main( int argc, char* argv[] ) {

	// ./hw2_NB_pthread #threads m T t FILE theta enable/disable x_min y_min length Length
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

	int num_bodies;

	ifstream f( in_file );
	string line;

	getline( f, line );
	num_bodies = atoi( line.c_str() );

	Vec2(double) pos;
	Vec2(double) vel;

	while( getline( f, line ) ) {
		istringstream iss(line);
		iss >> pos[0] >> pos[1] >> vel[0] >> vel[1];
		Body b( pos, vel, m );
		bodySet.push_back( b );
	}

	pthread_t* threads = new pthread_t[num_threads];

	if ( strcmp( xwin_en, "enable" ) == 0 )
		DispManager::init( x_min, y_min, l_coor, l_xwin );

	ComputeParams* params = new ComputeParams[num_threads];

	for (int i = 0; i < num_threads; i++) {
		ComputeParams p( i, num_threads, t, 0 );
		params[i] = p;
	}

	for (int iter = 0; iter < T; iter++) {
		for (int i = 0; i < num_threads; i++)
			pthread_create( &threads[i], NULL, move_bodies, (void *) &params[i] );

		for (int i = 0; i < num_threads; i++)
			pthread_join( threads[i], NULL );

		// cout << "iteration " << iter << endl;
		if ( strcmp( xwin_en, "enable" ) == 0 ) {
			DispManager::clear();

			for (int i = 0; i < bodySet.size(); i++)
				DispManager::draw( bodySet[i].position );
			DispManager::flush();
		}
	}

	exit(0);
	return 0;
}

void* move_bodies( void* ptr ) {
	ComputeParams* params = (ComputeParams*) ptr;
	int id = params->getId();
	int num_threads = params->getThreadCount();
	int num_bodies = bodySet.size();
	double del_t = params->getTimeInterval();

	if ( num_threads > num_bodies ) num_threads = num_bodies;

	if ( id < num_bodies ) {
		int body_start_idx;
		int body_end_idx;

		if ( id < num_bodies % num_threads ) {
			body_start_idx = id * (num_bodies / num_threads + 1);
			body_end_idx = (id + 1) * (num_bodies / num_threads + 1);
		} else {
			body_start_idx = id * (num_bodies / num_threads) + (num_bodies % num_threads);
			body_end_idx = (id + 1) * (num_bodies / num_threads) + (num_bodies % num_threads);
		}

		Vec2(double) F;
		for (int i = body_start_idx; i < body_end_idx; i++) {
			F *= 0;
			for (int j = 0; j < num_bodies; j++) {
				if ( i != j ) {
					GravForce force( bodySet[i], bodySet[j] );
					F += force.vector();
				}
			}

			bodySet[i].velocity += F / bodySet[i].mass * del_t;
		}

		pthread_barrier_wait( &bar );

		for (int i = body_start_idx; i < body_end_idx; i++)
			bodySet[i].position += bodySet[i].velocity * del_t;
	}

	return 0;
}
