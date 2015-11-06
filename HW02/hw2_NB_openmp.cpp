#include <iostream>      /* Input/Output */
#include <stdlib.h>     /* General Utilities */
#include <omp.h>
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

vector<Body> bodySet;

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

	// printf( "./hw2_NB_openmp %d %lf %d %lf %s %f %s", num_threads, m, T, t, in_file, theta, xwin_en );

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

	Vec2(double) pos;
	Vec2(double) vel;

	while( getline( f, line ) ) {
		istringstream iss(line);
		iss >> pos[0] >> pos[1] >> vel[0] >> vel[1];
		Body b( pos, vel, m );
		bodySet.push_back( b );
	}

	int num_pair = num_bodies * (num_bodies-1) / 2;

	GravForce** pair = new GravForce*[num_bodies-1];
	for (int i = 0; i < num_bodies-1; i++) {
		pair[i] = new GravForce[ num_bodies-1 - i ];
	}

	pthread_t* threads = new pthread_t[num_threads];

	if ( strcmp( xwin_en, "enable" ) == 0 )
		DispManager::init( x_min, y_min, l_coor, l_xwin );

	ComputeParams* params = new ComputeParams[num_threads];
	for (int i = 0; i < num_threads; i++) {
		ComputeParams p( i, num_threads, t, pair );
		params[i] = p;
	}

	for (int iter = 0; iter < T; iter++) {
		#pragma opm parallel num_thread(num_threads) private(i, j, num_bodies, del_t)
		{
			int num_bodies = bodySet.size();
			#pragma omp for schedule(dynamic, 1) nowait
				for (int i = 0; i < num_bodies-1; i++) {
					for (int j = 0; j < num_bodies-1-i; j++) {
						// printf( "thread%d process (%d, %d)\n", omp_get_thread_num(), i, i+j+1 );
						GravForce force( bodySet[i], bodySet[i+j+1] );
						pair[i][j] = force;
					}
				}

			Vec2(double) F;
			double del_t = t;
			#pragma omp for schedule(dynamic, 1) nowait
				for (int i = 0; i < num_bodies; i++) {
					F *= 0;
					for (int j = 0; j < num_bodies; j++) {
						if ( i > j ) F += pair[j][i-j-1].force_2to1();
						else if ( i < j ) F += pair[i][j-i-1].force_1to2();
					}

					bodySet[i].velocity += F / bodySet[i].mass * del_t;
					bodySet[i].position += bodySet[i].velocity * del_t;
				}
		}

		cout << "iter = " << iter << endl;
		if ( strcmp( xwin_en, "enable" ) == 0 ) {
			DispManager::clear();

			for (int i = 0; i < num_bodies; i++)
				DispManager::draw( bodySet[i].position );

			// #pragma omp parallel num_threads(num_threads) shared(bodySet) private(i, num_bodies)
			// {
			// 	int num_bodies = bodySet.size();
			// 	#pragma omp for schedule(dynamic, 1)
			// 		for (int i = 0; i < num_bodies; i++)
			// 			DispManager::draw( bodySet[i].position );
			// }

			DispManager::flush();
		}
	}

	exit(0);
	return 0;
}
