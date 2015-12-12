/**
 * Compiz Anaglyph plugin
 *
 * anaglyph.c
 * Copyright (c) 2007	Patryk Kowalczyk <wodor@wodor.org>
 *
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
 * Author:	Patryk Kowalczyk <wodor@wodor.org>
 **/

#include <compiz-core.h>
#include "anaglyph_options.h"


#define GET_ANAGLYPH_CORE(c) \
	((AnaglyphCore *) (c)->base.privates[corePrivateIndex].ptr)

#define ANAGLYPH_CORE(c) \
	AnaglyphCore *ac = GET_ANAGLYPH_CORE (c)

#define GET_ANAGLYPH_DISPLAY(d) \
	((AnaglyphDisplay *) (d)->base.privates[displayPrivateIndex].ptr)

#define ANAGLYPH_DISPLAY(d)  \
	AnaglyphDisplay *ad = GET_ANAGLYPH_DISPLAY (d)

#define GET_ANAGLYPH_SCREEN(s, ad)  \
	((AnaglyphScreen *) (s)->base.privates[(ad)->screenPrivateIndex].ptr)

#define ANAGLYPH_SCREEN(s)  \
	AnaglyphScreen *as = GET_ANAGLYPH_SCREEN (s, GET_ANAGLYPH_DISPLAY (s->display))

#define GET_ANAGLYPH_WINDOW(w, as) \
	((AnaglyphWindow *) (w)->base.privates[(as)->windowPrivateIndex].ptr)

#define ANAGLYPH_WINDOW(w) \
	AnaglyphWindow *aw = GET_ANAGLYPH_WINDOW (w,  \
				GET_ANAGLYPH_SCREEN (w->screen,  \
				GET_ANAGLYPH_DISPLAY (w->screen->display)))


#define NUM_OPTIONS(s) (sizeof ((s)->opt) / sizeof (CompOption))

/*
 * pointer to display list
 *
 */
static int displayPrivateIndex;
static int corePrivateIndex;


typedef struct _AnaglyphCore {
	ObjectAddProc			objectAdd;
} AnaglyphCore;

typedef struct _AnaglyphDisplay {
	int				screenPrivateIndex;
} AnaglyphDisplay;

typedef struct _AnaglyphScreen {
	int				windowPrivateIndex;
	PaintWindowProc 		paintWindow;
//	PreparePaintScreenProc		preparePaintScreen;
	DonePaintScreenProc		donePaintScreen;
	PaintOutputProc			paintOutput;
//	PaintTransformedOutputProc 	paintTransformedOutput;
	DamageWindowRectProc		damageWindowRect;
	Bool isAnaglyph;
	Bool isDamage;
} AnaglyphScreen;

typedef struct _AnaglyphWindow {
	Bool isAnaglyph;
} AnaglyphWindow;


static void
toggleAnaglyphWindow (CompWindow *w)
{

	ANAGLYPH_WINDOW(w);

	aw->isAnaglyph = !aw->isAnaglyph;

	if (matchEval (anaglyphGetExcludeMatch(w->screen), w))
		aw->isAnaglyph = FALSE;

	if(w->redirected && !aw->isAnaglyph)
		damageScreen(w->screen);

	addWindowDamage(w);
}


static void
toggleAnaglyphScreen (CompScreen *s)
{
	CompWindow *w;

	ANAGLYPH_SCREEN(s);
	//	compLogMessage (s->display, "anaglyph", CompLogLevelInfo, "info");
	as->isAnaglyph = !as->isAnaglyph;

	for (w = s->windows; w; w = w->next)
		if (w)
			toggleAnaglyphWindow (w);

}


static Bool
anaglyphAnaglyphWindow (CompDisplay *d,
			CompAction *action,
			CompActionState state,
			CompOption *option,
			int nOption)
{
	CompWindow *w;
	Window xid;

	xid = getIntOptionNamed(option, nOption, "window", 0);

	w = findWindowAtDisplay(d, xid);


	if (w){
		if (w->attrib.override_redirect)
			return FALSE;

		toggleAnaglyphWindow(w);
	}
	return FALSE;
}

static Bool
anaglyphAnaglyphScreen (CompDisplay *d,
			CompAction *action,
			CompActionState state,
			CompOption *option,
			int nOption)
{
	CompScreen *s;
	Window     xid;

	xid = getIntOptionNamed (option, nOption, "root", 0);

	s = findScreenAtDisplay (d, xid);

	if (s){
		toggleAnaglyphScreen (s);
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------- MASTER FUNCTION

static Bool anaglyphDrawWindow(CompWindow * w,
					 const WindowPaintAttrib  *attrib,
					 const CompTransform *transform,
					 Region	region,
					 unsigned int mask)
{
	Bool status;
	CompScreen* s = w->screen;
	ANAGLYPH_SCREEN(s);
	ANAGLYPH_WINDOW(w);

	//if (((aw->isAnaglyph != as->isAnaglyph) || (as->isAnaglyph && aw->isNew)) && w->texture->pixmap)
	if(aw->isAnaglyph && w->texture->pixmap)
	{
		//if (as->isAnaglyph)
		//	aw->isAnaglyph = FALSE;

		int oldFilter = s->display->textureFilter;

		if (anaglyphGetMipmaps (s))
			s->display->textureFilter = GL_LINEAR_MIPMAP_LINEAR;

		mask |= PAINT_WINDOW_TRANSFORMED_MASK;

		CompTransform sTransform = *transform;
		WindowPaintAttrib wa = *attrib;

		//Window xid;
		//CompWindow *activeWindow;
		//xid = getActiveWindow(s->display, s->root);
		//activeWindow=findWindowAtScreen(s, xid);

		float offset = anaglyphGetOffset (s);
		float desktopOffset = anaglyphGetDesktopOffset (s);

		if (anaglyphGetDesaturate (s))
			wa.saturation = 0.0f;

		UNWRAP(as, s, paintWindow);

		//BLUE and ...
		glColorMask(GL_FALSE,GL_TRUE,GL_TRUE,GL_FALSE);
			if (w->type & CompWindowTypeDesktopMask) //desktop
				matrixTranslate (&sTransform, offset*desktopOffset, 0.0f, 0.0f);
			else if (w->state & CompWindowStateShadedMask)
				matrixTranslate (&sTransform, 0.0f, 0.0f, 0.0f);
			else if ((w->state & CompWindowStateMaximizedHorzMask) || (w->state & CompWindowStateMaximizedVertMask ))
				matrixTranslate (&sTransform, -offset*3.5, 0.0f, 0.0f);
			else if (w->type & CompWindowTypeDockMask) // dock
				matrixTranslate (&sTransform,  0.0f, 0.0f, 0.0f);
			else if (w->state & CompWindowStateStickyMask) // sticky
				matrixTranslate (&sTransform, offset*desktopOffset, 0.0f, 0.0f);
			else if (w->state & CompWindowStateBelowMask) //below
				matrixTranslate (&sTransform, offset, 0.0f, 0.0f);
			else if (w->state & CompWindowStateAboveMask) // above
				matrixTranslate (&sTransform, -offset*4.0, 0.0f, 0.0f);
			else if (w->id == w->screen->display->activeWindow) // active window
				matrixTranslate (&sTransform, -offset*3.0, 0.0f, 0.0f);
			else //other windows
				matrixTranslate (&sTransform, -offset, 0.0f, 0.0f);

		status = (*s->paintWindow) (w, &wa, &sTransform, region, mask);

		//RED
		glColorMask(GL_TRUE,GL_FALSE,GL_FALSE,GL_FALSE);
			if (w->type & CompWindowTypeDesktopMask) //desktop
				matrixTranslate (&sTransform, -offset*2.0*desktopOffset, 0.0f, 0.0f);
			else if (w->state & CompWindowStateShadedMask)
				matrixTranslate (&sTransform, 0.0f, 0.0f, 0.0f);
			else if ((w->state & CompWindowStateMaximizedHorzMask) || (w->state & CompWindowStateMaximizedVertMask ))
				matrixTranslate (&sTransform, offset*3.5, 0.0f, 0.0f);
			else if (w->type & CompWindowTypeDockMask)// dock
				matrixTranslate (&sTransform, 0.0f, 0.0f, 0.0f);
			else if (w->state & CompWindowStateStickyMask) // sticky
				matrixTranslate (&sTransform, -offset*2.0*desktopOffset, 0.0f, 0.0f);
			else if (w->state & CompWindowStateBelowMask) //below
				matrixTranslate (&sTransform, -offset*2.0, 0.0f, 0.0f);
			else if (w->state & CompWindowStateAboveMask) //above
				matrixTranslate (&sTransform, offset*4.0, 0.0f, 0.0f);
			else if (w->id == w->screen->display->activeWindow) // active window
				matrixTranslate (&sTransform, offset*3.0, 0.0f, 0.0f);
			else //other windows
				matrixTranslate (&sTransform, offset*2.0, 0.0f, 0.0f);

		status = (*s->paintWindow) (w, &wa, &sTransform, region, mask);

		WRAP(as, s, paintWindow, anaglyphDrawWindow);

			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		s->display->textureFilter = oldFilter;
	}
	else
	{
		UNWRAP(as, s, paintWindow);
			status = (*s->paintWindow) (w, attrib, transform, region, mask);
		WRAP(as, s, paintWindow, anaglyphDrawWindow);
	}
	return status;
}

//------------------------------------------------------------------------------DAMAGE FUNCTION

static Bool anaglyphDamageWindowRect(CompWindow *w,
					Bool initial,
					BoxPtr rect){

	Bool status = FALSE;

	ANAGLYPH_SCREEN(w->screen);
	ANAGLYPH_WINDOW(w);

/*	if (initial){
		damageScreen(w->screen);
		status = TRUE;
	}
	else*/ if (aw->isAnaglyph || as->isAnaglyph || as->isDamage)
	{
		as->isDamage = TRUE;
		if (!aw->isAnaglyph && !as->isAnaglyph)
			as->isDamage = FALSE;

		damageScreen(w->screen);
		status = TRUE;
	}

	UNWRAP(as, w->screen, damageWindowRect);
		status |= (*w->screen->damageWindowRect)(w, initial, rect);
	WRAP(as, w->screen, damageWindowRect, anaglyphDamageWindowRect);

	return status;
}

//----------------------------------------------------------------------------------- PAINT OUTPUT

static Bool anaglyphPaintOutput(CompScreen *s, const ScreenPaintAttrib *sAttrib,
	const CompTransform *transform, Region region, CompOutput *output, unsigned int mask){

	Bool status;

	ANAGLYPH_SCREEN(s);

	mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;

	UNWRAP(as, s, paintOutput);
		status = (*s->paintOutput)(s, sAttrib, transform, region, output, mask);
	WRAP(as, s, paintOutput, anaglyphPaintOutput);

	return status;
}

//-------------------------------------------------------------------------------

static void
anaglyphWindowAdd (CompScreen *s,
			CompWindow *w)
{
	ANAGLYPH_SCREEN (s);

    if (as->isAnaglyph && matchEval (anaglyphGetAnaglyphMatch (s), w))
	toggleAnaglyphWindow (w);
}

static void
anaglyphScreenOptionChanged (CompScreen       *s,
				CompOption       *opt,
				AnaglyphScreenOptions num)
{
    switch (num)
    {
    case AnaglyphScreenOptionAnaglyphMatch:
    case AnaglyphScreenOptionExcludeMatch:
	{
	    CompWindow *w;
	    ANAGLYPH_SCREEN (s);

	    for (w = s->windows; w; w = w->next)
	    {
		Bool isAnaglyph;
		ANAGLYPH_WINDOW (w);

		isAnaglyph = matchEval (anaglyphGetAnaglyphMatch (s), w);
		isAnaglyph = isAnaglyph && !matchEval (anaglyphGetExcludeMatch (s), w);

		if (isAnaglyph && as->isAnaglyph && !aw->isAnaglyph)
		    toggleAnaglyphWindow (w);
		else if (!isAnaglyph && aw->isAnaglyph)
		    toggleAnaglyphWindow (w);
	    }
	}
	break;
    default:
	break;
    }
}

// ------------------------------------------------------------------------------- OBJECT ADD

static void
anaglyphObjectAdd (CompObject *parent,
			CompObject *object)
{
    static ObjectAddProc dispTab[] = {
	(ObjectAddProc) 0, /* CoreAdd */
        (ObjectAddProc) 0, /* DisplayAdd */
        (ObjectAddProc) 0, /* ScreenAdd */
        (ObjectAddProc) anaglyphWindowAdd
    };

    ANAGLYPH_CORE (&core);

    UNWRAP (ac, &core, objectAdd);
    (*core.objectAdd) (parent, object);
    WRAP (ac, &core, objectAdd, anaglyphObjectAdd);

    DISPATCH (object, dispTab, ARRAY_SIZE (dispTab), (parent, object));
}

// ------------------------------------------------------------------------------- CORE

static Bool
anaglyphInitCore (CompPlugin *p,
	     CompCore   *c)
{
    AnaglyphCore *ac;

    if (!checkPluginABI ("core", CORE_ABIVERSION))
        return FALSE;

    ac = malloc (sizeof (AnaglyphCore));
    if (!ac)
        return FALSE;

    displayPrivateIndex = allocateDisplayPrivateIndex ();
    if (displayPrivateIndex < 0)
    {
        free (ac);
        return FALSE;
    }

    WRAP (ac, c, objectAdd, anaglyphObjectAdd);

    c->base.privates[corePrivateIndex].ptr = ac;

    return TRUE;
}


static void
anaglyphFiniCore (CompPlugin *p,
			CompCore   *c)
{
    ANAGLYPH_CORE (c);

    freeDisplayPrivateIndex (displayPrivateIndex);

    UNWRAP (ac, c, objectAdd);

    free (ac);
}

// ------------------------------------------------------------------------------- DISPLAY

static Bool
anaglyphInitDisplay (CompPlugin *p, CompDisplay *d)
{
    AnaglyphDisplay *ad;

    ad = malloc (sizeof (AnaglyphDisplay));
    if (!ad)
        return FALSE;


	ad->screenPrivateIndex = allocateScreenPrivateIndex (d);
	if (ad->screenPrivateIndex < 0)
	{
		free (ad);
		return FALSE;
	}

	anaglyphSetWindowToggleKeyInitiate (d, anaglyphAnaglyphWindow);
	anaglyphSetScreenToggleKeyInitiate (d, anaglyphAnaglyphScreen);
	anaglyphSetWindowToggleButtonInitiate (d, anaglyphAnaglyphWindow);
	anaglyphSetScreenToggleButtonInitiate (d, anaglyphAnaglyphScreen);

	d->base.privates[displayPrivateIndex].ptr = ad;

	return TRUE;
}

static void
anaglyphFiniDisplay (CompPlugin *p, CompDisplay *d)
{
    ANAGLYPH_DISPLAY (d);

    freeScreenPrivateIndex (d, ad->screenPrivateIndex);

    free (ad);
}

// ------------------------------------------------------------------------------- SCREEN

static Bool
anaglyphInitScreen (CompPlugin *p, CompScreen *s)
{
	AnaglyphScreen *as;

	ANAGLYPH_DISPLAY (s->display);

	as = malloc (sizeof (AnaglyphScreen));
	if (!as)
		return FALSE;

	as->windowPrivateIndex = allocateWindowPrivateIndex(s);
	if (as->windowPrivateIndex < 0)
	{
		free(as);
		return FALSE;
	}

	as->isAnaglyph = FALSE;
	as->isDamage = FALSE;

	anaglyphSetAnaglyphMatchNotify (s, anaglyphScreenOptionChanged);
	anaglyphSetExcludeMatchNotify (s, anaglyphScreenOptionChanged);

	WRAP(as, s, paintOutput, anaglyphPaintOutput);
	WRAP(as, s, paintWindow, anaglyphDrawWindow);
	WRAP(as, s, damageWindowRect, anaglyphDamageWindowRect);

	s->base.privates[ad->screenPrivateIndex].ptr = as;

	return TRUE;
}

static void
anaglyphFiniScreen (CompPlugin *p, CompScreen *s)
{
	ANAGLYPH_SCREEN (s);

	freeWindowPrivateIndex (s, as->windowPrivateIndex);

	UNWRAP(as, s, paintOutput);
	UNWRAP(as, s, paintWindow);
	UNWRAP(as, s, damageWindowRect);

	free (as);
}

// ------------------------------------------------------------------------------- WINDOW

static Bool anaglyphInitWindow(CompPlugin * p, CompWindow * w)
{
	AnaglyphWindow *aw;

	ANAGLYPH_SCREEN(w->screen);

	aw = malloc(sizeof(AnaglyphWindow));
	if (!aw)
		return FALSE;

	aw->isAnaglyph = FALSE;

	w->base.privates[as->windowPrivateIndex].ptr = aw;

	return TRUE;
}

static void anaglyphFiniWindow(CompPlugin * p, CompWindow * w)
{
	ANAGLYPH_WINDOW(w);

	free(aw);
}

// ------------------------------------------------------------------------------- INITFINI

/*
 * anaglyphInit
 *
 */

static Bool
anaglyphInit (CompPlugin *p)
{
	corePrivateIndex = allocateCorePrivateIndex ();
	if (corePrivateIndex < 0)
		return FALSE;

    return TRUE;
}

/*
 * anaglyphFini
 *
 */

static void
anaglyphFini (CompPlugin *p)
{

	if (corePrivateIndex >= 0)
		freeCorePrivateIndex (corePrivateIndex);

}

// ------------------------------------------------------------------------------- OBJECTS

static CompBool
anaglyphInitObjects(CompPlugin *p,
              CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
        (InitPluginObjectProc) anaglyphInitCore,
        (InitPluginObjectProc) anaglyphInitDisplay,
        (InitPluginObjectProc) anaglyphInitScreen,
	(InitPluginObjectProc) anaglyphInitWindow
        };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));

}

static void
anaglyphFiniObjects(CompPlugin *p,
               CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
        (FiniPluginObjectProc) anaglyphFiniCore,
        (FiniPluginObjectProc) anaglyphFiniDisplay,
        (FiniPluginObjectProc) anaglyphFiniScreen,
	(FiniPluginObjectProc) anaglyphFiniWindow
    };

    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

static CompPluginVTable anaglyphVTable = {
	"anaglyph",
	0,
	anaglyphInit,
	anaglyphFini,
	anaglyphInitObjects,
	anaglyphFiniObjects,
	0,
	0
};

CompPluginVTable*
getCompPluginInfo(void)
{
    return &anaglyphVTable;
}
