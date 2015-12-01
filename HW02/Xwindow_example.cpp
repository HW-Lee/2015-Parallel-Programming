#include <iostream>
#include <cmath>
#include "DispManager.h"

using namespace std;

const double PI = 3.14159265358979323;

int main( int argc, char* argv[] ) {
	DispManager::init( -2.0, -2.0, 4.0, 400 );

	for (int sec = 1; sec <= 5; sec++) {
		DispManager::clear();

		for (int i = 0; i < 100; i++)
			DispManager::draw( (1 + (double)sec/10)*cos(2*PI*i/100.0), (1 + (double)sec/10)*sin(2*PI*i/100.0) );

		DispManager::flush();
		sleep(1);
	}
	return 0;
}
