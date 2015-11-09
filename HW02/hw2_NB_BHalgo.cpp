#include <iostream>      /* Input/Output */
#include <stdlib.h>     /* General Utilities */
#include <pthread.h>    /* POSIX Threads */
#include <vector>
#include <fstream>
#include <sstream>
#include <string.h>
#include "Vec.h"
#include "DispManager.h"
#include "Body.h"
#include "GravForce.h"
#include "BHTree.h"

using namespace std;

class Params {
public:
	Params() {}

	int tid;
	int num_threads;
	int chidx;
	BHTree* node;
};

void init_tree();
void build_tree( BHTree* node );

pthread_mutex_t lock[4];
pthread_t* threads;

BHTree tree;
vector<Body>* bodySet = new vector<Body>[2];
int switch_idx = 0;

int main( int argc, char* argv[] ) {

	// ./hw2_NB_BHalgo #threads m T t FILE theta enable/disable x_min y_min length Length
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

	while( getline( f, line ) ) {
		istringstream iss(line);
		iss >> pos[0] >> pos[1] >> vel[0] >> vel[1];
		Body b( pos, vel, m );
		bodySet[0].push_back( b );
		bodySet[1].push_back( b );
	}

	if ( strcmp( xwin_en, "enable" ) == 0 )
		DispManager::init( x_min, y_min, l_coor, l_xwin );

	for (int i = 0; i < 4; i++)
		pthread_mutex_init( &lock[i], NULL );
	
	threads = new pthread_t[num_threads];

	for (int iter = 0; iter < T; iter++) {
		// cout << "iter = " << iter << endl;
		init_tree();
		build_tree( &tree );
	}

	for (int i = 0; i < 4; i++)
		pthread_mutex_destroy( &lock[i] );
	free( threads );

	exit(0);
	return 0;
}

void init_tree() {
	tree.clear();

	tree.layer = 0;

	Vec2(double) region[2];
	for (int i = 0; i < bodySet[0].size(); i++) {
		tree.member_register.push_back( &bodySet[switch_idx][i] );

		if ( bodySet[switch_idx][i].position[0] < region[0][0] || i == 0 )
			region[0][0] = bodySet[switch_idx][i].position[0];

		if ( bodySet[switch_idx][i].position[1] < region[0][1] || i == 0 )
			region[0][1] = bodySet[switch_idx][i].position[1];

		if ( bodySet[switch_idx][i].position[0] > region[1][0] || i == 0 )
			region[1][0] = bodySet[switch_idx][i].position[0];

		if ( bodySet[switch_idx][i].position[1] > region[1][1] || i == 0 )
			region[1][1] = bodySet[switch_idx][i].position[1];
	}

	tree.region[0] = region[0]; tree.region[1] = region[1];
	// cout << "Root region: (" << region[0][0] << ", " << region[0][1] << ") to (" << region[1][0] << ", " << region[1][1] << ")" << endl;
}

void build_tree( BHTree* node ) {
	// for (int i = 0; i < node->layer; i++) cout << "\t";
	// cout << "[" << node->layer << "] member_register.size() = " << node->member_register.size() << endl;
	if ( node->member_register.size() == 0 ) return;
	if ( node->member_register.size() == 1 ) {
		node->setMass( node->member_register[0]->mass );
		node->setMassCenter( node->member_register[0]->position );
		node->member_register.clear();
		return;
	}

	Vec2(double) mass_center;
	double mass = 0;

	for (int i = 0; i < node->member_register.size(); i++) {
		mass_center += node->member_register[i]->position;
		mass += node->member_register[i]->mass;
	}
	mass_center /= node->member_register.size();
	mass /= node->member_register.size();

	node->setMass( mass );
	node->setMassCenter( mass_center );

	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 2; j++) {
			BHTree child_node;
			child_node.layer = node->layer + 1;
			if ( i == 0 ) {
				child_node.region[0][0] = (node->region[0][0] + node->region[1][0]) / 2;
				child_node.region[1][0] = node->region[1][0];
			} else {
				child_node.region[0][0] = node->region[0][0];
				child_node.region[1][0] = (node->region[0][0] + node->region[1][0]) / 2;
			}
			if ( j == 0 ) {
				child_node.region[0][1] = (node->region[0][1] + node->region[1][1]) / 2;
				child_node.region[1][1] = node->region[1][1];
			} else {
				child_node.region[0][1] = node->region[0][1];
				child_node.region[1][1] = (node->region[0][1] + node->region[1][1]) / 2;
			}

			node->children.push_back( child_node );
		}
	}

	int i, j;
	Vec2(double) position;
	Vec2(double) center = ( node->region[0] + node->region[1] ) / 2;
	for (int idx = 0; idx < node->member_register.size(); idx++) {
		position = node->member_register[idx]->position;
		i = ( position[0] > center[0] ) ? 0 : 1;
		j = ( position[1] > center[1] ) ? 0 : 1;

		node->children[ 2*i+j ].member_register.push_back( node->member_register[idx] );
	}

	for (int chidx = 0; chidx < node->children.size(); chidx++)
		build_tree( &(node->children[chidx]) );

	node->member_register.clear();
}
