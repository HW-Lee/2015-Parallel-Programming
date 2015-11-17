#include <iostream>
#include <vector>
#include "Vec.h"
#include "Body.h"

#ifndef BHTREE
#define BHTREE

using namespace std;

class BHTree {
public:
	BHTree() { this->body = 0; this->mass = 0; }
	Vec2(double) getMassCenter();
	double getMass();
	Body getBody();
	bool isGroup();
	void setBodyRef( Body* body );
	void setMassCenter( Vec2(double) mass_center );
	void setMass( double mass );
	void clear();
	void setIsGroup( bool is_group );

	Vec2(double) region[2];
	vector<BHTree> children;
	vector<Body*> member_register;

	int layer;

private:
	Body* body;
	Vec2(double) mass_center;
	double mass;
	bool is_group;

};

Vec2(double) BHTree::getMassCenter() { return this->mass_center; }
double BHTree::getMass() { return this->mass; }
Body BHTree::getBody() { return *( this->body ); }
bool BHTree::isGroup() { return this->is_group; }
void BHTree::setBodyRef( Body* body ) { this->body = body; }
void BHTree::setMassCenter( Vec2(double) mass_center ) { this->mass_center = mass_center; }
void BHTree::setMass( double mass ) { this->mass = mass; }
void BHTree::setIsGroup( bool is_group ) { this->is_group = is_group; }

void BHTree::clear() {
	for (int i = 0; i < this->children.size(); i++) {
		this->children[i].clear();
		this->children[i].member_register.clear();
	}
	this->member_register.clear();
	this->children.clear();
}

#endif