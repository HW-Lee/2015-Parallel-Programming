#include <iostream>      /* Input/Output */
#include <stdlib.h>     /* General Utilities */
#include <pthread.h>    /* POSIX Threads */
#include <sys/time.h>

#include "RollerCoaster.h"
#include "Passenger.h"

using namespace std;
pthread_mutex_t mtx;
RollerCoaster* rolcoaster;

void* simulate( void* ptr );

int main( int argc, char* argv[] ) {

	// hw2_SRCC n C T N
	// int n = atoi( argv[0] );
	// int C = atoi( argv[1] );
	// int T = atoi( argv[2] );
	// int N = atoi( argv[3] );
	int n = 5;
	int C = 3;
	int T = 1000;
	int N = 10;

	// Initialization
	pthread_t* threads = new pthread_t[n];
	Passenger** passengers = new Passenger*[n];

	pthread_mutex_init( &mtx, NULL );

	rolcoaster = new RollerCoaster( C, &mtx );
	rolcoaster->set_tracktime_millis( T );
	rolcoaster->set_run_times( N );

	for (int i = 0; i < n; i++)
		passengers[i] = new Passenger( i+1 );

	for (int i = 0; i < n; i++)
		pthread_create( &threads[i], NULL, simulate, (void *) passengers[i] );

	for (int i = 0; i < n; i++)
		pthread_join( threads[i], NULL );

	exit(0);
	return 0;
}

void* simulate( void* ptr ) {
	Passenger* passenger = (Passenger*) ptr;

	while ( rolcoaster->is_available() ) {
		pthread_mutex_lock( &mtx );

		rolcoaster->get_in( passenger );
		printf( "Passenger%d is waiting.\n", passenger->getId() );
		if ( rolcoaster->is_available() )
			pthread_cond_wait( passenger->getNotifier(), &mtx );
		printf( "Passenger%d walks around.\n", passenger->getId() );
		usleep( 1000 * 1000 );

		pthread_mutex_unlock( &mtx );
	}

	return 0;
}
