#include <iostream>
#include "GravForce.h"

#ifndef COMPUTEPARAMS
#define COMPUTEPARAMS

using namespace std;

class ComputeParams {
public:
	ComputeParams() {};
	ComputeParams( int thread_id, int num_threads, double del_t, GravForce** shared_memory );
	void operator=( ComputeParams p );
	int getId();
	double getTimeInterval();
	int getThreadCount();
	GravForce** getSharedMemory();

private:
	int thread_id;
	int num_threads;
	GravForce** shared_memory;
	double del_t;

};

ComputeParams::ComputeParams( int thread_id, int num_threads, double del_t, GravForce** shared_memory ) {
	this->thread_id = thread_id;
	this->del_t = del_t;
	this->num_threads = num_threads;
	this->shared_memory = shared_memory;
}

void ComputeParams::operator=( ComputeParams p ) {
	this->thread_id = p.getId();
	this->del_t = p.getThreadCount();
	this->num_threads = p.getThreadCount();
	this->shared_memory = p.getSharedMemory();
}

int ComputeParams::getId() { return this->thread_id; }
double ComputeParams::getTimeInterval() { return this->del_t; }
int ComputeParams::getThreadCount() { return this->num_threads; }
GravForce** ComputeParams::getSharedMemory() { return this->shared_memory; }

#endif
