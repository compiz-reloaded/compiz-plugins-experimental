#ifndef SCREENSAVER_INTERNAL_H
#define SCREENSAVER_INTERNAL_H

#include <compiz-core.h>
#include <compiz-cube.h>

#include "matrix.h"
#include "vector.h"
#include "screensaver_options.h"

#define GET_SCREENSAVER_DISPLAY(d) \
	((ScreenSaverDisplay *) (d)->base.privates[displayPrivateIndex].ptr)

#define SCREENSAVER_DISPLAY(d) \
	ScreenSaverDisplay *sd = GET_SCREENSAVER_DISPLAY (d)

#define GET_SCREENSAVER_SCREEN(s, hd) \
	((ScreenSaverScreen *) (s)->base.privates[(hd)->screenPrivateIndex].ptr)

#define SCREENSAVER_SCREEN(s) \
	ScreenSaverScreen *ss = GET_SCREENSAVER_SCREEN (s, \
	GET_SCREENSAVER_DISPLAY (s->display))

#define GET_SCREENSAVER_WINDOW(w, ss) \
	((ScreenSaverWindow *) (w)->base.privates[(ss)->windowPrivateIndex].ptr)

#define SCREENSAVER_WINDOW(w) \
	ScreenSaverWindow *sw = GET_SCREENSAVER_WINDOW  (w, \
		GET_SCREENSAVER_SCREEN  (w->screen, \
		GET_SCREENSAVER_DISPLAY (w->screen->display)))

#define WIN_X(w) ( (w)->attrib.x - (w)->input.left )
#define WIN_Y(w) ( (w)->attrib.y - (w)->input.top )
#define WIN_W(w) ( (w)->width + (w)->input.left + (w)->input.right )
#define WIN_H(w) ( (w)->height + (w)->input.top + (w)->input.bottom )

extern int displayPrivateIndex;
extern int cubeDisplayPrivateIndex;

typedef struct _ScreenSaverState {
	// when the screensaver is enabled, running and fadingIn are set to true
	// then, after fadingInDuration, fadingIn is set to false
	// when the screensaver is requested to stop, fadingOut is set to true
	// after fadingOutDuration, running is set to false
	Bool running;
	Bool fadingOut;
	Bool fadingIn;
} ScreenSaverState;

typedef struct _ScreenSaverXSSContext {
	int timeout, interval, prefer_blanking, allow_exposures;
	int first_event;
	Bool init;
} ScreenSaverXSSContext;

class DisplayEffect;
class ScreenEffect;
class WindowEffect;

typedef struct _ScreenSaverDisplay {
	int screenPrivateIndex;
	HandleEventProc handleEvent;

	ScreenSaverState state;
	ScreenSaverXSSContext XSScontext;

	DisplayEffect* effect;
} ScreenSaverDisplay;

typedef struct _ScreenSaverScreen {
	int windowPrivateIndex;
	CubeGetRotationProc			getRotation;
	PreparePaintScreenProc		preparePaintScreen;
	DonePaintScreenProc			donePaintScreen;
	PaintOutputProc				paintOutput;
	PaintWindowProc				paintWindow;
	PaintTransformedOutputProc	paintTransformedOutput;
	PaintScreenProc                 paintScreen;

	// time is stored to calculate progress when fadingIn or fadingOut
	int time;

	float cubeRotX, cubeRotV, cubeProgress, zCamera;
	// FadeOut values are the initial value used to calculate the interpolation when fadingOut
	float cubeRotXFadeOut, cubeRotVFadeOut, zCameraFadeOut;

	// Used with flying windows mode
	// screenCenter is the attraction point
	Point screenCenter;
	Matrix camera, cameraMat;
	float angleCam;

	ScreenEffect* effect;
	GLushort desktopOpacity;

} ScreenSaverScreen;

typedef struct _ScreenSaverWindow {
	WindowEffect* effect;
} ScreenSaverWindow;

void screenSaverHandleEvent( CompDisplay *d, XEvent *event );
void screenSaverGetRotation( CompScreen *s, float *x, float *v, float *progress );
void screenSaverPreparePaintScreen( CompScreen* s, int msSinceLastPaint );
void screenSaverDonePaintScreen( CompScreen* s );
void screenSaverPaintTransformedOutput(	CompScreen* s, const ScreenPaintAttrib* sAttrib, \
										const CompTransform* transform, Region region, \
										CompOutput *output, unsigned int mask);

Bool screenSaverPaintOutput(	CompScreen* s, const ScreenPaintAttrib *sAttrib, \
								const CompTransform* transform, Region region, \
								CompOutput* output, unsigned int mask);

Bool screenSaverPaintWindow(	CompWindow* w, const WindowPaintAttrib* attrib, \
								const CompTransform* transform, Region region, unsigned int mask);

void screenSaverPaintBackground( CompScreen* s, Region region, unsigned int mask );

void screenSaverPaintScreen (CompScreen *s, CompOutput *outputs, int numOutputs, unsigned int mask);

#include "wrapper.h"
#include "effect.h"

#endif
