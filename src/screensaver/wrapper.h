#ifndef WRAPPER_H
#define WRAPPER_H

#include "screensaver_internal.h"

class DisplayWrapper
{
public:
	DisplayWrapper( CompDisplay* d );
	virtual ~DisplayWrapper() {}

	virtual void handleEvent( XEvent *event );

protected:
	CompDisplay* d;
	ScreenSaverDisplay* sd;
};

class ScreenWrapper
{
public:
	ScreenWrapper( CompScreen* s );
	virtual ~ScreenWrapper() {}

	virtual void getRotation( float* x, float* v, float *progress );
	virtual void preparePaintScreen( int msSinceLastPaint );
	virtual void donePaintScreen();
	virtual void paintTransformedOutput(	const ScreenPaintAttrib* sAttrib, \
											const CompTransform* transform, Region region, \
											CompOutput *output, unsigned int mask);
	virtual Bool paintOutput(	const ScreenPaintAttrib *sAttrib, \
								const CompTransform* transform, Region region, \
								CompOutput* output, unsigned int mask);
	virtual void paintScreen (CompOutput   *outputs,
				  int          numOutputs,
				  unsigned int mask);

protected:
	CompScreen* s;
	ScreenSaverScreen* ss;
};

class WindowWrapper
{
public:
	WindowWrapper( CompWindow* w );
	virtual ~WindowWrapper() {}
	virtual Bool paintWindow(	const WindowPaintAttrib* attrib, \
								const CompTransform* transform, Region region, unsigned int mask);
protected:
	CompWindow* w;
	ScreenSaverWindow* sw;
};

#endif
