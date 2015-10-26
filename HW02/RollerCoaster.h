#include <iostream>
#include <vector>
#include <sys/time.h>
#include <pthread.h>

#include "Passenger.h"

using namespace std;

#ifndef ROLLERCOASTER
#define ROLLERCOASTER

class RollerCoaster {
public:
	RollerCoaster( int cap, pthread_mutex_t* mtx_ref );
	~RollerCoaster();
	void get_in( Passenger* passenger );
	void set_tracktime_millis( int tracktime_millis );
	void set_run_times( int N );
	bool is_full();
	bool is_available();

	void go_track();

	static long get_current_time_millis();

private:
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
}

void RollerCoaster::get_in( Passenger* passenger ) {
	// pthread_mutex_lock( this->mtx_ref );

	if ( this->is_available() ) {
		if ( this->is_full() ) this->go_track();
		
		this->passengers.push_back( passenger );
		this->cur_cap++;

		cout << "Passenger" << passenger->getId() << " gets in!" << endl;
	}

	// pthread_mutex_unlock( this->mtx_ref );
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
	for (int i = 0; i < this->passengers.size()-1; i++)
		cout << passengers[i]->getId() << ", ";
	cout << "and " << passengers[this->cap-1]->getId() << " passengers are in the car." << endl;

	usleep( this->tracktime_millis * 1000 );

	cout << "car arrives at " << RollerCoaster::get_current_time_millis() - this->init_time << " millisec. ";

	for (int i = 0; i < this->passengers.size(); i++) pthread_cond_signal( this->passengers[i]->getNotifier() );

	for (int i = 0; i < this->passengers.size()-1; i++)
		cout << passengers[i]->getId() << ", ";
	cout << "and " << passengers[this->cap-1]->getId() << " passengers get off." << endl;

	this->N--;
	this->cur_cap = 0;
	this->passengers.clear();
}

long RollerCoaster::get_current_time_millis() {
	struct timeval init_time;
	gettimeofday( &init_time, NULL );

	return init_time.tv_sec * 1000 + init_time.tv_usec / 1000;
}

#endif
