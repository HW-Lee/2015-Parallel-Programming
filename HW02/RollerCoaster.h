#include <iostream>
#include <vector>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>

#include "Passenger.h"

using namespace std;

#ifndef ROLLERCOASTER
#define ROLLERCOASTER

class RollerCoaster {
public:
	RollerCoaster( int cap, pthread_mutex_t* mtx_ref );
	~RollerCoaster();
	pthread_cond_t* getNotifier();
	int get_in( Passenger* passenger );
	void set_tracktime_millis( int tracktime_millis );
	void set_run_times( int N );
	bool is_full();
	bool is_available();

	void go_track();

	static long get_current_time_millis();

private:
	pthread_cond_t* notifier;
	pthread_mutex_t* mtx_ref;
	long init_time;
	int cap;
	int N;
	int cur_cap;
	int tracktime_millis;
	std::vector<Passenger*> passengers;

};

RollerCoaster::RollerCoaster( int cap, pthread_mutex_t* mtx_ref ) {
	this->mtx_ref = mtx_ref;
	this->cap = cap;
	this->cur_cap = 0;
	this->init_time = RollerCoaster::get_current_time_millis();
	this->notifier = new pthread_cond_t[1];
	pthread_cond_init( this->notifier, NULL );
}

pthread_cond_t* RollerCoaster::getNotifier() {
	return this->notifier;
}

int RollerCoaster::get_in( Passenger* passenger ) {
	int resp = 0;
	if ( this->is_available() ) {
		resp = 1;
		this->passengers.push_back( passenger );
		this->cur_cap++;

		printf( "Passenger%d gets in!\n", passenger->getId() );
		if ( this->is_full() ) {
			printf( "The car is full.\n" );
			resp = 0;
			this->go_track();
		}
	}
	return resp;
}

void RollerCoaster::set_tracktime_millis( int tracktime_millis ) {
	this->tracktime_millis = tracktime_millis;
}

void RollerCoaster::set_run_times( int N ) {
	this->N = N;
}

bool RollerCoaster::is_full() {
	return this->cur_cap == this->cap;
}

bool RollerCoaster::is_available() {
	return this->N > 0;
}

void RollerCoaster::go_track() {
	cout << "car departures at " << RollerCoaster::get_current_time_millis() - this->init_time << " millisec. ";
	cout << "Passengers";
	for (int i = 0; i < this->passengers.size()-1; i++)
		cout << passengers[i]->getId() << ", ";
	cout << "and " << passengers[this->cap-1]->getId() << " are in the car." << endl;

	usleep( this->tracktime_millis * 1e3 );

	cout << "car arrives at " << RollerCoaster::get_current_time_millis() - this->init_time << " millisec. ";

	for (int i = 0; i < this->passengers.size()-1; i++)
		cout << passengers[i]->getId() << ", ";
	cout << "and " << passengers[this->cap-1]->getId() << " passengers get off." << endl;

	this->N--;
	this->cur_cap = 0;

	for (int i = 0; i < this->passengers.size()-1; i++) { 
		pthread_cond_signal( this->passengers[i]->getNotifier() );
		// cout << "Wake up! Passenger" << this->passengers[i]->getId() << endl;
	}
	this->passengers.clear();
}

long RollerCoaster::get_current_time_millis() {
	struct timeval init_time;
	gettimeofday( &init_time, NULL );

	return init_time.tv_sec * 1000 + init_time.tv_usec / 1000;
}

#endif
