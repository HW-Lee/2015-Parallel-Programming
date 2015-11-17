#include <iostream>
#include "Vec.h"

#ifndef BODY
#define BODY

class Body {
public:
	Body() {};
	Body( const Vec2(double) pos, const Vec2(double) vel, const double m );
	void operator=( const Body& body );

	Vec2(double) position;
	Vec2(double) velocity;
	double mass;

};

Body::Body( const Vec2(double) pos, const Vec2(double) vel, const double m ) {
	this->position = pos;
	this->velocity = vel;
	this->mass = m;
}

void Body::operator=( const Body& body ) {
	this->position = body.position;
	this->velocity = body.velocity;
	this->mass = body.mass;
}

#endif