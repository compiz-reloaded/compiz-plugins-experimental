#ifndef FLYINGWINDOWS_H
#define FLYINGWINDOWS_H

#include "screensaver_internal.h"

class DisplayFlyingWindows : public DisplayEffect
{
public:
	DisplayFlyingWindows( CompDisplay* d ) : DisplayEffect(d) {}
	virtual ~DisplayFlyingWindows() {}

	virtual void handleEvent( XEvent *event );
};

class ScreenFlyingWindows : public ScreenEffect
{
public:
	ScreenFlyingWindows( CompScreen* s ) : ScreenEffect(s) {}
	virtual ~ScreenFlyingWindows() {}

	virtual bool enable();
	virtual void disable();
	virtual void preparePaintScreen( int msSinceLastPaint );
	virtual void donePaintScreen();
	virtual void paintScreen (CompOutput   *outputs,
				  int          numOutputs,
				  unsigned int mask);
	virtual Bool paintOutput(	const ScreenPaintAttrib *sAttrib, \
								const CompTransform* transform, Region region, \
								CompOutput* output, unsigned int mask);
	virtual void paintTransformedOutput(	const ScreenPaintAttrib* sAttrib, \
											const CompTransform* transform, Region region, \
											CompOutput *output, unsigned int mask);
private:
	void initWindow( CompWindow* _w );
	void recalcVertices( CompWindow* _w );
	void addForce( const Point& p1, const Point& p2, const Point& center, Vector& resultante, Vector& couple, float w, Bool attract );
};

class WindowFlyingWindows : public WindowEffect
{
	friend class ScreenFlyingWindows;

public:
	WindowFlyingWindows( CompWindow* w );
	virtual ~WindowFlyingWindows() {}
	void initWindow();
	virtual Bool paintWindow(	const WindowPaintAttrib* attrib, \
								const CompTransform* transform, Region region, unsigned int mask);
	void recalcVertices();

	static WindowFlyingWindows& getInstance( CompWindow* w )
		{ SCREENSAVER_WINDOW(w); return ( WindowFlyingWindows& )( *sw->effect ); }

private:
	bool isActiveWin();

	// isScreenSaverWin()
	bool active;

	// used for non-active window like the desktop
	GLushort opacity;
	GLushort opacityFadeOut;
	GLushort opacityOld;
	int steps;

	// used for active window
	// translate matrix
	Matrix transformTrans;

	// rotation matrix
	Matrix centerTrans, transformRot, centerTransInv;

	// precomputed transform matrix
	Matrix transform;

	Matrix transformFadeOut;

	// 5 normalized vertices are stored, the four corners and the center
	Point vertex[5];

	// normalized speed vectors
	Vector speed;
	Vector speedrot;
};

#endif
