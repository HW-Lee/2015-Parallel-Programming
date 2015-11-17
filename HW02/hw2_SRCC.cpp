#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h> 
#include <time.h>
#include <unistd.h>

#include "RollerCoaster.h"
#include "Passenger.h"

using namespace std;
pthread_mutex_t mtx;
RollerCoaster* rolcoaster;

struct timespec ref_time;

void tic();
double toc();

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

	double** waiting_times = new double*[n];

	for (int i = 0; i < n; i++)
		passengers[i] = new Passenger( i+1 );

	for (int i = 0; i < n; i++)
		pthread_create( &threads[i], NULL, passenger_thread, (void *) passengers[i] );

	for (int i = 0; i < n; i++)
		pthread_join( threads[i], (void **) &waiting_times[i] );

	double avg = 0;
	for (int i = 0; i < n; i++)
		avg += *waiting_times[i] / n;

	printf( "Average waiting time: %lf ms\n", avg );

	exit(0);
	return 0;
}

void* passenger_thread( void* ptr ) {
	int resp;
	Passenger* passenger = (Passenger*) ptr;
	double waiting_time_millis = 0;

	srand( time(NULL) );

	do {
		tic();
		pthread_mutex_lock( &mtx );

		resp = rolcoaster->get_in( passenger );
		if ( resp == 1 ) {
			// printf( "Passenger%d is waiting.\n", passenger->getId() );
			pthread_cond_wait( passenger->getNotifier(), &mtx );
			// printf( "Passenger%d is awake!\n", passenger->getId() );
		}

		pthread_mutex_unlock( &mtx );
		waiting_time_millis += toc();

		if ( rolcoaster->is_available() ) {
			int wander_t = rand() % 5000 + 1;
			printf( "Passenger%d wanders around the park.\n", passenger->getId() );
			usleep( wander_t * 1000 );
			if ( rolcoaster->is_available() )
				printf( "Passenger%d returns for another ride.\n", passenger->getId() );
		}
	} while ( rolcoaster->is_available() );

	printf( "Passenger%d waits %lf ms in total.\n", passenger->getId(), waiting_time_millis );

	double* status = new double[1];
	*status = waiting_time_millis;

	return (void *) status;
}

void tic() {
	clock_gettime( CLOCK_REALTIME, &ref_time );
}

double toc() {
	struct timespec now;
	clock_gettime( CLOCK_REALTIME, &now );
	return (double) ( (now.tv_sec*1e3 + now.tv_nsec*1e-6) - (ref_time.tv_sec*1e3 + ref_time.tv_nsec*1e-6) );
}
