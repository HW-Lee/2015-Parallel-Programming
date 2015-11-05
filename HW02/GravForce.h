#include <iostream>
#include <math.h>
#include "Vec.h"
#include "Body.h"

#ifndef GRAVFORCE
#define GRAVFORCE

using namespace std;

class GravForce {
public:
	GravForce() {};
	GravForce( Body body1, Body body2 );
	double magnitude();
	Vec2(double) force_1to2();
	Vec2(double) force_2to1();

	static const double G;

private:
	Body body1;
	Body body2;
	double F;

};

// const double GravForce::G = 6.67408e-3;
const double GravForce::G = 6.67408e-11;

GravForce::GravForce( Body body1, Body body2 ) {
	this->body1 = body1;
	this->body2 = body2;
	this->F = ( G * body1.mass * body2.mass ) / pow( (body1.position - body2.position).norm<double>(), 2 );
}

double GravForce::magnitude() { return this->F; }

Vec2(double) GravForce::force_1to2() {
	Vec2(double) direction = (body2.position - body1.position);
	return direction * this->F / direction.norm<double>();
}

Vec2(double) GravForce::force_2to1() {
	Vec2(double) direction = (body1.position - body2.position);
	return direction * this->F / direction.norm<double>();
}

#endif
