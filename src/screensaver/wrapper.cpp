#include "screensaver_internal.h"

DisplayWrapper::DisplayWrapper( CompDisplay* d )
{
	SCREENSAVER_DISPLAY(d);
	this->d = d;
	this->sd = sd;
}

void DisplayWrapper::handleEvent( XEvent *event )
{
	UNWRAP (sd, d, handleEvent);
	d->handleEvent(d, event);
	WRAP (sd, d, handleEvent, screenSaverHandleEvent);
}

ScreenWrapper::ScreenWrapper( CompScreen* s )
{
	SCREENSAVER_SCREEN(s);
	this->s = s;
	this->ss = ss;
}

void ScreenWrapper::getRotation( float* x, float* v, float *progress )
{
	CUBE_SCREEN(s);
	UNWRAP( ss, cs, getRotation );
	cs->getRotation( s, x, v, progress );
	WRAP( ss, cs, getRotation, screenSaverGetRotation );
}

void ScreenWrapper::preparePaintScreen( int msSinceLastPaint )
{
	UNWRAP (ss, s, preparePaintScreen);
	s->preparePaintScreen(s, msSinceLastPaint);
	WRAP (ss, s, preparePaintScreen, screenSaverPreparePaintScreen);
}

void ScreenWrapper::donePaintScreen()
{
	UNWRAP (ss, s, donePaintScreen);
	s->donePaintScreen(s);
	WRAP (ss, s, donePaintScreen, screenSaverDonePaintScreen);
}

void ScreenWrapper::paintTransformedOutput(	const ScreenPaintAttrib* sAttrib, \
											const CompTransform* transform, Region region, \
											CompOutput *output, unsigned int mask)
{
	UNWRAP(ss, s, paintTransformedOutput);
	s->paintTransformedOutput(s, sAttrib, transform, region, output, mask);
	WRAP( ss, s, paintTransformedOutput, screenSaverPaintTransformedOutput );
}

Bool ScreenWrapper::paintOutput(	const ScreenPaintAttrib *sAttrib, \
									const CompTransform* transform, Region region, \
									CompOutput* output, unsigned int mask)
{
	UNWRAP (ss, s, paintOutput);
	Bool status = s->paintOutput(s, sAttrib, transform, region, output, mask);
	WRAP (ss, s, paintOutput, screenSaverPaintOutput);
	return status;
}

void ScreenWrapper::paintScreen (CompOutput *outputs, int numOutputs, unsigned int mask)
{
    UNWRAP (ss, s, paintScreen);
    (*s->paintScreen) (s, outputs, numOutputs, mask);
    WRAP (ss, s, paintScreen, screenSaverPaintScreen);
}

WindowWrapper::WindowWrapper( CompWindow* w )
{
	SCREENSAVER_WINDOW(w);
	this->w = w;
	this->sw = sw;
}

Bool WindowWrapper::paintWindow(	const WindowPaintAttrib* attrib, \
									const CompTransform* transform, Region region, unsigned int mask)
{
	CompScreen* s = w->screen;
	SCREENSAVER_SCREEN(s);

	UNWRAP (ss, s, paintWindow);
	Bool status = s->paintWindow(w, attrib, transform, region, mask);
	WRAP (ss, s, paintWindow, screenSaverPaintWindow);
	return status;
}
