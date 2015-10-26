#include <iostream>
#include <pthread.h>

#ifndef PASSENGER
#define PASSENGER

class Passenger {
public:
	Passenger( int id );
	~Passenger();
	int getId();
	pthread_cond_t* getNotifier();

private:
	int id;
	pthread_cond_t* notifier;

};

Passenger::Passenger( int id ) {
	this->id = id;
	this->notifier = new pthread_cond_t[1];
	pthread_cond_init( this->notifier, NULL );
}

int Passenger::getId() {
	return this->id;
}

pthread_cond_t* Passenger::getNotifier() {
	return this->notifier;
}

#endif
