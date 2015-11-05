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

void* compute_force( void* ptr );
void* move_bodies( void* ptr );

vector<Body> bodySet;

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

	// printf( "./hw2_NB_pthread %d %lf %d %lf %s %f %s", num_threads, m, T, t, in_file, theta, xwin_en );

	if ( strcmp( xwin_en, "enable" ) == 0 ) { // Needs to be modified to c version string comparison
		   x_min 	   = atof( argv[8] );
		   y_min 	   = atof( argv[9] );
		   l_coor 	   = atof( argv[10] );
		   l_xwin 	   = atoi( argv[11] );

		   // printf( " %f %f %f %d", x_min, y_min, l_coor, l_xwin );
	}

	// cout << endl;

	int num_bodies;

	ifstream f( in_file );
	string line;

	getline( f, line );
	num_bodies = atoi( line.c_str() );

	// cout << num_bodies << endl;

	Vec2(double) pos;
	Vec2(double) vel;

	while( getline( f, line ) ) {
		istringstream iss(line);
		iss >> pos[0] >> pos[1] >> vel[0] >> vel[1];
		// cout << "(" << pos[0] << ", " << pos[1] << ", " << vel[0] << ", " << vel[1] << ")" << endl;
		Body b( pos, vel, m );
		bodySet.push_back( b );
	}

	int num_pair = num_bodies * (num_bodies-1) / 2;
	int num_chunks = (num_bodies % 2 == 0) ? num_bodies / 2 : (num_bodies+1) / 2;

	GravForce** pair = new GravForce*[num_bodies-1];
	for (int i = 0; i < num_bodies-1; i++) {
		pair[i] = new GravForce[ num_bodies-1 - i ];
	}

	pthread_t* threads = new pthread_t[num_threads];

	if ( strcmp( xwin_en, "enable" ) == 0 )
		DispManager::init( x_min, y_min, l_coor, l_xwin );

	ComputeParams* params = new ComputeParams[num_threads];

	// T = 5;
	for (int iter = 0; iter < T; iter++) {
		for (int i = 0; i < num_threads; i++) {
			ComputeParams p( i, num_threads, t, pair );
			params[i] = p;
			pthread_create( &threads[i], NULL, compute_force, (void *) &params[i] );
		}

		for (int i = 0; i < num_threads; i++)
			pthread_join( threads[i], NULL );
	
		for (int i = 0; i < num_threads; i++)
			pthread_create( &threads[i], NULL, move_bodies, (void *) &params[i] );

		for (int i = 0; i < num_threads; i++)
			pthread_join( threads[i], NULL );

		// cout << "-----------------------------------" << endl << endl;
		// for (int i = 0; i < 10; i++) {
		// 	cout << "Body" << i+1 << ":" << endl;
		// 	cout << "\tmass    :  " << bodySet[i].mass << endl;
		// 	cout << "\tposition: (" << bodySet[i].position[0] << ", " << bodySet[i].position[1] << ")" << endl;
		// 	cout << "\tvelocity: (" << bodySet[i].velocity[0] << ", " << bodySet[i].velocity[1] << ")" << endl;
		// }
		// cout << "-----------------------------------" << endl << endl;

		cout << "iteration " << iter << endl;
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

void* compute_force( void* ptr ) {
	ComputeParams* params = (ComputeParams*) ptr;
	int id = params->getId();
	int num_threads = params->getThreadCount();
	int num_bodies = bodySet.size();
	GravForce** pair = params->getSharedMemory();

	int num_chunks = (num_bodies % 2 == 0) ? num_bodies / 2 : (num_bodies+1) / 2;

	if ( num_threads > num_chunks ) num_threads = num_chunks;

	if ( id < num_threads ) {
		int chunk_start_idx;
		int chunk_end_idx;
		if ( id < num_chunks % num_threads ) {
			chunk_start_idx = id * (num_chunks / num_threads + 1);
			chunk_end_idx = (id + 1) * (num_chunks / num_threads + 1);
		} else {
			chunk_start_idx = id * (num_chunks / num_threads) + (num_chunks % num_threads);
			chunk_end_idx = (id + 1) * (num_chunks / num_threads) + (num_chunks % num_threads);
		}

		// printf( "[compute_force] Process%d processing from %d to %d...\n", id, chunk_start_idx, chunk_end_idx-1 );

		for (int i = chunk_start_idx; i < chunk_end_idx; i++) {
			for (int j = 0; j < num_bodies-i-1; j++) {
				// printf( "(%d, %d)\n", i, j );
				GravForce force( bodySet[i], bodySet[i+j+1] );
				pair[i][j] = force;
			}
			for (int j = 0; j < i+1; j++) {
				// printf( "(%d, %d)\n", num_bodies-2-i, j );
				GravForce force( bodySet[num_bodies-2-i], bodySet[num_bodies-2-i+j+1] );
				pair[num_bodies-2-i][j] = force;
			}
		}
	}

	return 0;
}

void* move_bodies( void* ptr ) {
	ComputeParams* params = (ComputeParams*) ptr;
	int id = params->getId();
	int num_threads = params->getThreadCount();
	int num_bodies = bodySet.size();
	GravForce** pair = params->getSharedMemory();
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

		// printf( "[move_bodies] Process%d processing from %d to %d...\n", id, body_start_idx, body_end_idx-1 );

		Vec2(double) F;
		for (int i = body_start_idx; i < body_end_idx; i++) {
			F *= 0;
			for (int j = 0; j < num_bodies; j++) {
				if ( i > j ) F += pair[j][i-j-1].force_2to1();
				else if ( i < j ) F += pair[i][j-i-1].force_1to2();
			}
			// printf( "Body%d is pushed with force (%e, %e)\n", i, F[0], F[1] );
			bodySet[i].velocity += F / bodySet[i].mass * del_t;
			bodySet[i].position += bodySet[i].velocity * del_t;
		}
	}

	return 0;
}
