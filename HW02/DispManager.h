#include <iostream>
#include <X11/Xlib.h>
#include <math.h>
#include <unistd.h>
#include "Vec.h"

#ifndef DISPLAYMANAGER
#define DISPLAYMANAGER

using namespace std;

class DispManager {
public:
	static void init( double x_min, double y_min, double length, int length_px );
	static void draw( double x, double y );
	static void draw( Vec2(double) point );
	static void drawline( double x1, double y1, double x2, double y2 );
	static void drawline( Vec2(double) v1, Vec2(double) v2 );
	static void drawrect( Vec2(double) v1, Vec2(double) v2 );
	static void clear();
	static void flush();

	static GC gc;
	static Window window;
	static Display* display;
	static int screen;
	static double x_min;
	static double y_min;
	static double length;
	static int width_px;
	static int height_px;
	static int length_per_px;

private:
	static void _draw( int x, int y );
	static void _drawline( int x1, int y1, int x2, int y2 );
	static void _init( int width_px, int height_px );
	static void _init( int length_px );

};

GC DispManager::gc = (GC) NULL;
Window DispManager::window = (Window) NULL;
Display* DispManager::display = (Display*) NULL;
int DispManager::screen = 0;
double DispManager::x_min = 0;
double DispManager::y_min = 0;
double DispManager::length = 0;
int DispManager::width_px = 0;
int DispManager::height_px = 0;
int DispManager::length_per_px = 0;

void DispManager::init( double x_min, double y_min, double length, int length_px ) {
	DispManager::x_min = x_min;
	DispManager::y_min = y_min;
	DispManager::length = length;
	DispManager::width_px = length_px;
	DispManager::height_px = length_px;
	DispManager::length_per_px = (int) (length_px / length);
	DispManager::_init( length_px );
}

void DispManager::_init( int width_px, int height_px ) {
	DispManager::display = XOpenDisplay( NULL );
	if ( DispManager::display == NULL ) {
		cout << "Failed to open the display." << endl;
		return;
	}
	DispManager::screen = DefaultScreen( DispManager::display );
	// set window position
	int x = 0, y = 0;
	// border width in pixels
	int border_width = 0;
	// create the window
	DispManager::window = 
		XCreateSimpleWindow( 
							DispManager::display,
							RootWindow( DispManager::display, DispManager::screen ), 
							x, y, width_px, height_px, border_width, 
							BlackPixel( DispManager::display, DispManager::screen ),
							WhitePixel( DispManager::display, DispManager::screen )
							);
	DispManager::clear();
}

void DispManager::_init( int length_px ) { DispManager::_init( length_px, length_px ); }

void DispManager::clear() {
	// create the graph
	XGCValues values;
	long valuemask = 0;

	DispManager::gc = XCreateGC( DispManager::display, DispManager::window, valuemask, &values );

	XSetForeground( DispManager::display, DispManager::gc, BlackPixel( DispManager::display, DispManager::screen ) );
	XSetBackground( DispManager::display, DispManager::gc, 0X0000FF00 );
	XSetLineAttributes( DispManager::display, DispManager::gc, 1, LineSolid, CapRound, JoinRound );
	
	/* map(show) the window */
	XMapWindow( DispManager::display, DispManager::window );
	XSync( DispManager::display, 0 );

	/* draw rectangle */
	XSetForeground( DispManager::display, DispManager::gc, BlackPixel( DispManager::display, DispManager::screen ) );
	XFillRectangle( DispManager::display, DispManager::window, DispManager::gc, 0, 0, DispManager::width_px, DispManager::height_px );
	XFlush( DispManager::display );
}

void DispManager::_draw( int x, int y ) {
	XSetForeground( DispManager::display, DispManager::gc, WhitePixel( DispManager::display, DispManager::screen ) );
	XDrawPoint( DispManager::display, DispManager::window, DispManager::gc, x, y );
}

void DispManager::_drawline( int x1, int y1, int x2, int y2 ) {
	XSetForeground( DispManager::display, DispManager::gc, WhitePixel( DispManager::display, DispManager::screen ) );
	XDrawLine( DispManager::display, DispManager::window, DispManager::gc, x1, y1, x2, y2 );
}

void DispManager::drawline( double x1, double y1, double x2, double y2 ) {
	int x1_px = (int) ( (x1 - DispManager::x_min) * DispManager::length_per_px );
	int y1_px = (int) ( (y1 - DispManager::y_min) * DispManager::length_per_px );
	int x2_px = (int) ( (x2 - DispManager::x_min) * DispManager::length_per_px );
	int y2_px = (int) ( (y2 - DispManager::y_min) * DispManager::length_per_px );

	if ( x1_px < 0 || y1_px < 0 || x2_px < 0 || y2_px < 0 ) return;
	DispManager::_drawline( x1_px, y1_px, x2_px, y2_px );
}

void DispManager::draw( double x, double y ) {
	int x_px = (int) ( (x - DispManager::x_min) * DispManager::length_per_px );
	int y_px = (int) ( (y - DispManager::y_min) * DispManager::length_per_px );

	if (x_px < 0 || y_px < 0) return;
	DispManager::_draw( x_px, y_px );
}

void DispManager::drawline( Vec2(double) v1, Vec2(double) v2 ) {
	DispManager::drawline( v1[0], v1[1], v2[0], v2[1] );
}

void DispManager::drawrect( Vec2(double) v1, Vec2(double) v2 ) {
	Vec2(double) vertexes[4];
	vertexes[0][0] = v1[0];
	vertexes[0][1] = v1[1];
	vertexes[1][0] = v2[0];
	vertexes[1][1] = v1[1];
	vertexes[2][0] = v2[0];
	vertexes[2][1] = v2[1];
	vertexes[3][0] = v1[0];
	vertexes[3][1] = v2[1];
	for (int i = 0; i < 4; i++)
		DispManager::drawline( vertexes[i], vertexes[ (i+1) % 4 ] );
}

void DispManager::draw( Vec2(double) point ) { DispManager::draw( point[0], point[1] ); }

void DispManager::flush() { XFlush( DispManager::display ); }

#endif
