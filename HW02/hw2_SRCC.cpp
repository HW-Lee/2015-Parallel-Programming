#include <iostream>      /* Input/Output */
#include <stdlib.h>     /* General Utilities */
#include <pthread.h>    /* POSIX Threads */
#include <sys/time.h>

#include "RollerCoaster.h"
#include "Passenger.h"

using namespace std;
pthread_mutex_t mtx;
RollerCoaster* rolcoaster;

void* passenger_thread( void* ptr );

int main( int argc, char* argv[] ) {

	// hw2_SRCC n C T N
	int n = atoi( argv[1] );
	int C = atoi( argv[2] );
	int T = atoi( argv[3] );
	int N = atoi( argv[4] );

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
		pthread_create( &threads[i], NULL, passenger_thread, (void *) passengers[i] );

	for (int i = 0; i < n; i++)
		pthread_join( threads[i], NULL );

	exit(0);
	return 0;
}

void* passenger_thread( void* ptr ) {
	int resp;
	Passenger* passenger = (Passenger*) ptr;

	srand( time(NULL) );

	do {
		pthread_mutex_lock( &mtx );

		resp = rolcoaster->get_in( passenger );
		if ( resp == 1 ) {
			// printf( "Passenger%d is waiting.\n", passenger->getId() );
			pthread_cond_wait( passenger->getNotifier(), &mtx );
			// printf( "Passenger%d is awake!\n", passenger->getId() );
		}

		pthread_mutex_unlock( &mtx );

		if ( rolcoaster->is_available() ) {
			int wander_t = rand() % 5000 + 1;
			printf( "Passenger%d wanders around the park.\n", passenger->getId() );
			usleep( wander_t * 1000 );
			if ( rolcoaster->is_available() )
				printf( "Passenger%d returns for another ride.\n", passenger->getId() );
		}
	} while ( rolcoaster->is_available() );

	return 0;
}
