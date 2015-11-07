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

using namespace std;

vector<Body>* bodySet;

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

	int num_bodies;

	ifstream f( in_file );
	string line;

	getline( f, line );
	num_bodies = atoi( line.c_str() );

	Vec2(double) pos;
	Vec2(double) vel;

	bodySet = new vector<Body>[2];

	while( getline( f, line ) ) {
		istringstream iss(line);
		iss >> pos[0] >> pos[1] >> vel[0] >> vel[1];
		Body b( pos, vel, m );
		bodySet[0].push_back( b );
		bodySet[1].push_back( b );
	}

	if ( strcmp( xwin_en, "enable" ) == 0 )
		DispManager::init( x_min, y_min, l_coor, l_xwin );

	int switch_idx = 0;

	for (int iter = 0; iter < T; iter++) {
		#pragma omp parallel num_threads(num_threads) firstprivate(switch_idx, num_bodies, t)
		{
			#pragma omp for
			for (int i = 0; i < num_bodies; i++) {
				Vec2(double) F;
				for (int j = 0; j < num_bodies; j++) {
					if ( i != j ) {
						GravForce force( bodySet[switch_idx][i], bodySet[switch_idx][j] );
						F += force.vector();
					}
				}
				bodySet[ (switch_idx + 1) % 2 ][i].velocity = bodySet[switch_idx][i].velocity + F / bodySet[switch_idx][i].mass * t;
				bodySet[ (switch_idx + 1) % 2 ][i].position = bodySet[switch_idx][i].position + bodySet[ (switch_idx + 1) % 2 ][i].velocity * t;
			}
		}

		switch_idx = (switch_idx + 1) % 2;

		cout << "iter = " << iter << endl;
		if ( strcmp( xwin_en, "enable" ) == 0 ) {
			DispManager::clear();

			for (int i = 0; i < num_bodies; i++)
				DispManager::draw( bodySet[switch_idx][i].position );

			DispManager::flush();
		}
	}

	exit(0);
	return 0;
}
