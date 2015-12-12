/**
 *
 * Compiz screensaver plugin
 *
 * screensaver.cpp
 *
 * Copyright (c) 2007 Nicolas Viennot <nicolas@viennot.biz>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 **/


#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <X11/extensions/scrnsaver.h>

#include "screensaver_internal.h"
#include "flyingwindows.h"
#include "rotatingcube.h"

int displayPrivateIndex;
int cubeDisplayPrivateIndex;

template< typename _DisplayEffect, typename _ScreenEffect, typename _WindowEffect >
static void screenSaverEffectInstance( CompDisplay* d )
{
	SCREENSAVER_DISPLAY(d);
	delete sd->effect;
	sd->effect = new _DisplayEffect(d);

	for ( CompScreen* s = d->screens; s; s = s->next)
	{
		SCREENSAVER_SCREEN(s);

		delete ss->effect;
		ss->effect = new _ScreenEffect(s);

		for( CompWindow* w = s->windows; w; w = w->next )
		{
			SCREENSAVER_WINDOW(w);
			delete sw->effect;
			sw->effect = new _WindowEffect(w);
		}
	}
}

static void screenSaverEnableEffect( CompDisplay* d )
{
	SCREENSAVER_DISPLAY(d);
	ScreensaverModeEnum mode = (ScreensaverModeEnum)screensaverGetMode(d);

	if( mode == ModeFlyingWindows )
		screenSaverEffectInstance< DisplayFlyingWindows, ScreenFlyingWindows, WindowFlyingWindows >(d);
	else if( mode == ModeRotatingCube )
		screenSaverEffectInstance< DisplayEffect, ScreenRotatingCube, WindowEffect >(d);

	for ( CompScreen* s = d->screens; s; s = s->next)
	{
		SCREENSAVER_SCREEN(s);
		ss->time = 0;
		if( !ss->effect->enable() )
		{
			screenSaverEffectInstance< DisplayEffect, ScreenEffect, WindowEffect >(d);
			return;
		}
	}
	sd->state.fadingOut = False;
	sd->state.fadingIn = True;
	sd->state.running = True;
}

static void screenSaverDisableEffect( CompDisplay* d )
{
	SCREENSAVER_DISPLAY(d);
	for ( CompScreen* s = d->screens; s; s = s->next)
	{
		SCREENSAVER_SCREEN(s);
		ss->effect->disable();
		ss->time = 0;
	}

	sd->state.fadingOut = True;
	sd->state.fadingIn = False;
}

static void screenSaverCleanEffect( CompDisplay* d )
{
	screenSaverEffectInstance< DisplayEffect, ScreenEffect, WindowEffect >(d);
}

static void
screenSaverSetState( CompDisplay *d, Bool enable )
{
	SCREENSAVER_DISPLAY(d);

	if( !sd->state.running && enable )
		sd->effect->loadEffect = true;

	if( sd->state.running && !sd->state.fadingOut && !enable )
		screenSaverDisableEffect(d);
}

static Bool
screenSaverInitiate (CompDisplay     *d,
	            CompAction      *action,
	            CompActionState state,
	            CompOption      *option,
	            int	            nOption)
{
	SCREENSAVER_DISPLAY(d);
	screenSaverSetState(d, !sd->state.running );
	return TRUE;
}

void screenSaverGetRotation( CompScreen *s, float *x, float *v, float *progress )
{
	SCREENSAVER_SCREEN (s);
    ss->effect->getRotation( x, v, progress );
}

Bool screenSaverPaintWindow (CompWindow *w,
	    const WindowPaintAttrib *attrib,
		 const CompTransform	  *transform,
		 Region		          region,
		unsigned int mask)
{
	SCREENSAVER_WINDOW(w);
	return sw->effect->paintWindow( attrib, transform, region, mask );
}

Bool screenSaverPaintOutput (CompScreen		  *s,
		 const ScreenPaintAttrib *sAttrib,
		 const CompTransform	  *transform,
		 Region		          region,
		 CompOutput		  *output,
		 unsigned int		  mask)
{
	SCREENSAVER_SCREEN(s);
	return ss->effect->paintOutput( sAttrib, transform, region, output, mask );
}

void screenSaverPreparePaintScreen (CompScreen *s,
			int	    msSinceLastPaint)
{
	SCREENSAVER_SCREEN (s);
	ss->effect->preparePaintScreen( msSinceLastPaint );
}

void screenSaverPaintTransformedOutput(CompScreen * s,
									   const ScreenPaintAttrib * sAttrib,
									   const CompTransform    *transform,
									   Region region, CompOutput *output,
									   unsigned int mask)
{
	SCREENSAVER_SCREEN(s);
	ss->effect->paintTransformedOutput( sAttrib, transform, region, output, mask);
}

void screenSaverDonePaintScreen( CompScreen* s )
{
	SCREENSAVER_SCREEN (s);
	ss->effect->donePaintScreen();
}

void screenSaverHandleEvent( CompDisplay *d, XEvent *event )
{
	XScreenSaverNotifyEvent* XSSevent;
	SCREENSAVER_DISPLAY (d);

	sd->effect->handleEvent( event );

	switch( (event->type & 0x7F) - sd->XSScontext.first_event )
	{
	case ScreenSaverNotify:
		XSSevent = (XScreenSaverNotifyEvent*) event;
		screenSaverSetState( d, XSSevent->state );
		break;
	}

	if( sd->effect->loadEffect )
	{
		sd->effect->loadEffect = false;
		screenSaverEnableEffect(d);
	}
	else if( sd->effect->cleanEffect )
	{
		sd->effect->cleanEffect = false;
		screenSaverCleanEffect(d);
	}
}

void screenSaverPaintScreen (CompScreen   *s,
			     CompOutput   *outputs,
			     int          numOutputs,
			     unsigned int mask)
{
    SCREENSAVER_SCREEN (s);
    ss->effect->paintScreen (outputs, numOutputs, mask);
}

static void screenSaverSetXScreenSaver( CompDisplay *d, Bool enable )
{
	SCREENSAVER_DISPLAY(d);

	if( enable && !sd->XSScontext.init )
	{

		int dummy;
		if( !XScreenSaverQueryExtension(d->display, &sd->XSScontext.first_event, &dummy ) )
		{
			compLogMessage ("screensaver", CompLogLevelWarn,
					"XScreenSaver Extension not available");
			return;
		}
		sd->XSScontext.init = True;

		XGetScreenSaver( d->display, &sd->XSScontext.timeout, &sd->XSScontext.interval, \
			&sd->XSScontext.prefer_blanking, &sd->XSScontext.allow_exposures);
		XSetScreenSaver( d->display, (int)(screensaverGetAfter(d)*60.0), sd->XSScontext.interval, 0, AllowExposures);

		Window root = DefaultRootWindow (d->display);
		XSetWindowAttributes attr;
		int mask = 0;
		XScreenSaverSetAttributes( d->display,root, -100,-100,1,1,0 ,CopyFromParent,CopyFromParent,CopyFromParent,mask,&attr);
		XScreenSaverSelectInput( d->display, root, ScreenSaverNotifyMask );

	}
	if( !enable && sd->XSScontext.init )
	{
		sd->XSScontext.init = False;

		XSetScreenSaver( d->display, sd->XSScontext.timeout, sd->XSScontext.interval, \
			sd->XSScontext.prefer_blanking, sd->XSScontext.allow_exposures);

		Window root = DefaultRootWindow (d->display);
		XScreenSaverSelectInput( d->display, root, 0 );
		XScreenSaverUnsetAttributes( d->display, root );
	}
}

static void screenSaverSetXScreenSaverNotify( CompDisplay *d, CompOption *opt, ScreensaverDisplayOptions num )
{
	screenSaverSetXScreenSaver( d, False );
	screenSaverSetXScreenSaver( d, screensaverGetStartAutomatically(d) );
}

static Bool
screenSaverInitWindow (CompPlugin *p,
		CompWindow *w)
{
	ScreenSaverWindow *sw;

	SCREENSAVER_SCREEN (w->screen);

	sw = (ScreenSaverWindow*)malloc (sizeof (ScreenSaverWindow));
	if (!sw)
		return FALSE;

	w->base.privates[ss->windowPrivateIndex].ptr = sw;
	SCREENSAVER_DISPLAY(w->screen->display);
	if( sd->state.running && (ScreensaverModeEnum)screensaverGetMode(w->screen->display) == ModeFlyingWindows )
		sw->effect = new WindowFlyingWindows(w);
	else sw->effect = new WindowEffect(w);

	return TRUE;
}

static void
screenSaverFiniWindow (CompPlugin *p,
		CompWindow *w)
{
	SCREENSAVER_WINDOW (w);
	delete sw->effect;
	free (sw);
}

static Bool
screenSaverInitScreen (CompPlugin *p,
		      CompScreen *s)
{
	ScreenSaverScreen *ss;

	SCREENSAVER_DISPLAY (s->display);

	ss = (ScreenSaverScreen*)malloc (sizeof (ScreenSaverScreen));
	if (!ss)
	    return FALSE;

	ss->windowPrivateIndex = allocateWindowPrivateIndex (s);
	if (ss->windowPrivateIndex < 0)
	{
	    free (ss);
	    return FALSE;
	}

	s->base.privates[sd->screenPrivateIndex].ptr = ss;
	ss->effect = new ScreenEffect(s);
	ss->desktopOpacity = OPAQUE;

	WRAP (ss, s, preparePaintScreen, screenSaverPreparePaintScreen);
	WRAP (ss, s, donePaintScreen, screenSaverDonePaintScreen);
	WRAP (ss, s, paintOutput, screenSaverPaintOutput);
	WRAP (ss, s, paintWindow, screenSaverPaintWindow);
	WRAP (ss, s, paintTransformedOutput, screenSaverPaintTransformedOutput);
	WRAP (ss, s, paintScreen, screenSaverPaintScreen);

	return TRUE;
}

static void
screenSaverFiniScreen (CompPlugin *p,
		      CompScreen *s)
{
	SCREENSAVER_SCREEN (s);

	UNWRAP (ss, s, preparePaintScreen);
	UNWRAP (ss, s, donePaintScreen);
	UNWRAP (ss, s, paintOutput);
	UNWRAP (ss, s, paintWindow);
	UNWRAP (ss, s, paintTransformedOutput);
	UNWRAP (ss, s, paintScreen);

	delete ss->effect;
	freeWindowPrivateIndex(s, ss->windowPrivateIndex);
	free (ss);
}

static Bool
screenSaverInitDisplay (CompPlugin  *p,
		   CompDisplay *d)
{
	ScreenSaverDisplay *sd;

	sd = (ScreenSaverDisplay*)malloc (sizeof (ScreenSaverDisplay));
	if (!sd) return FALSE;

	sd->state.running = False;
	sd->state.fadingOut = False;
	sd->state.fadingIn = False;

	sd->screenPrivateIndex = allocateScreenPrivateIndex (d);
	if (sd->screenPrivateIndex < 0)
	{
	    free (sd);
	    return FALSE;
	}
	d->base.privates[displayPrivateIndex].ptr = sd;
	sd->effect = new DisplayEffect(d);

	screensaverSetInitiateKeyInitiate(d,screenSaverInitiate);
	screensaverSetInitiateButtonInitiate(d,screenSaverInitiate);
	screensaverSetInitiateEdgeInitiate(d,screenSaverInitiate);
	screensaverSetStartAutomaticallyNotify( d, screenSaverSetXScreenSaverNotify);
	screensaverSetAfterNotify( d, screenSaverSetXScreenSaverNotify);

	sd->XSScontext.init = False;
	screenSaverSetXScreenSaver(d,screensaverGetStartAutomatically(d));

	WRAP (sd, d, handleEvent, screenSaverHandleEvent);
	return TRUE;
}

static void
screenSaverFiniDisplay (CompPlugin  *p,
		   CompDisplay *d)
{
	SCREENSAVER_DISPLAY (d);

	screenSaverSetXScreenSaver(d,FALSE);

	UNWRAP (sd, d, handleEvent);
	delete sd->effect;
	freeScreenPrivateIndex (d, sd->screenPrivateIndex);
	free (sd);
}

static CompBool
screenSaverInitObject (CompPlugin *p,
		       CompObject *o)
{
	static InitPluginObjectProc dispTab[] = {
		(InitPluginObjectProc) 0, /* InitCore */
		(InitPluginObjectProc) screenSaverInitDisplay,
		(InitPluginObjectProc) screenSaverInitScreen,
		(InitPluginObjectProc) screenSaverInitWindow
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
screenSaverFiniObject (CompPlugin *p,
		       CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
		(FiniPluginObjectProc) 0, /* FiniCore */
		(FiniPluginObjectProc) screenSaverFiniDisplay,
		(FiniPluginObjectProc) screenSaverFiniScreen,
		(FiniPluginObjectProc) screenSaverFiniWindow
    };

    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

static Bool
screenSaverInit (CompPlugin *p)
{
	displayPrivateIndex = allocateDisplayPrivateIndex ();
	if (displayPrivateIndex < 0)
	    return FALSE;

	return TRUE;
}

static void
screenSaverFini (CompPlugin *p)
{
	if (displayPrivateIndex >= 0)
	    freeDisplayPrivateIndex (displayPrivateIndex);
}

static CompPluginVTable testplugVTable = {
	"screensaver",
	0,
	screenSaverInit,
	screenSaverFini,
	screenSaverInitObject,
	screenSaverFiniObject,
	0, /* GetScreenOptions */
	0, /* SetScreenOption */
};

extern "C" {
CompPluginVTable *
getCompPluginInfo (void)
{
	return &testplugVTable;
}
}
