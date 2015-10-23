#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <unistd.h>
#include <cmath>
using namespace std;

GC gc;
Display *display;
Window window;      //initialization for a window
int screen;         //which screen 

void initGraph(int width,int height)
{
	/* open connection with the server */ 
	display = XOpenDisplay(NULL);
	if(display == NULL) {
		fprintf(stderr, "cannot open display\n");
		exit(1);
	}

	screen = DefaultScreen(display);

	/* set window position */
	int x = 0;
	int y = 0;

	/* border width in pixels */
	int border_width = 0;

	/* create window */
	window = XCreateSimpleWindow(display, RootWindow(display, screen), x, y, width, height, border_width, BlackPixel(display, screen), WhitePixel(display, screen));
	
	/* create graph */
	XGCValues values;
	long valuemask = 0;
	
	gc = XCreateGC(display, window, valuemask, &values);
	//XSetBackground (display, gc, WhitePixel (display, screen));
	XSetForeground (display, gc, BlackPixel (display, screen));
	XSetBackground(display, gc, 0X0000FF00);
	XSetLineAttributes (display, gc, 1, LineSolid, CapRound, JoinRound);
	
	/* map(show) the window */
	XMapWindow(display, window);
	XSync(display, 0);

	/* draw rectangle */
	XSetForeground(display,gc,BlackPixel(display,screen));
	XFillRectangle(display,window,gc,0,0,width,height);
	XFlush(display);
}

void draw(int x,int y)
{
	/* draw point */
	XSetForeground(display,gc,WhitePixel(display,screen));
	XDrawPoint (display, window, gc, x, y);
}

const double PI = 3.14159;

int main(int argc,char *argv[])
{
	initGraph(500, 500);
	for( int i = 0; i < 100; i++ )
		draw(int(200*cos(i/100.0*2*PI))+250, 250+int(200*sin(i/100.0*2*PI)));
	for( int i = 0; i < 50; i++)
		draw(int(100*cos(i/50.0*2*PI))+250, 250+int(100*sin(i/50.0*2*PI)));
	XFlush(display);
    
	sleep(3);
	return 0;
}
