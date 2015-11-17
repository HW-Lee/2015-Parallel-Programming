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
	Vec2(double) vector();

	static const double G;

private:
	Vec2(double) F;

};

// const double GravForce::G = 6.67408e-3;
const double GravForce::G = 6.67408e-11;

GravForce::GravForce( Body body1, Body body2 ) {
	Vec2(double) direction = (body2.position - body1.position);
	double mag = ( G * body1.mass * body2.mass ) / pow( direction.norm<double>(), 3 );
	this->F = direction * mag;
}

double GravForce::magnitude() { return this->F.norm<double>(); }

Vec2(double) GravForce::vector() { return this->F; }

#endif
