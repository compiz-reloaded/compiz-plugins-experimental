/**
 *
 * Beryl useless transparent terminals eyecandy plugin plugin
 *
 * fakeargb.c
 *
 * Copyright (c) 2006 Robert Carr <racarr@beryl-project.org>
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
 **/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <X11/Xatom.h>
#include <X11/extensions/Xrender.h>

#include <compiz.h>
#include "fakeargb_options.h"

static int displayPrivateIndex;

typedef struct _FakeDisplay
{
	int screenPrivateIndex;
} FakeDisplay;

typedef struct _FakeScreen
{
	int windowPrivateIndex;
	DrawWindowTextureProc drawWindowTexture;
	int handle;
	Bool black;
} FakeScreen;

typedef struct _FakeWindow
{
	Bool isFaked;
} FakeWindow;

#define GET_FAKE_DISPLAY(d) ((FakeDisplay *) (d)->privates[displayPrivateIndex].ptr)
#define FAKE_DISPLAY(d) FakeDisplay *fd = GET_FAKE_DISPLAY (d)
#define GET_FAKE_SCREEN(s, nd) ((FakeScreen *) (s)->privates[(nd)->screenPrivateIndex].ptr)
#define FAKE_SCREEN(s) FakeScreen *fs = GET_FAKE_SCREEN (s, GET_FAKE_DISPLAY (s->display))
#define GET_FAKE_WINDOW(w, ns) ((FakeWindow *) (w)->privates[(ns)->windowPrivateIndex].ptr)
#define FAKE_WINDOW(w) FakeWindow *fw = GET_FAKE_WINDOW  (w, GET_FAKE_SCREEN  (w->screen, GET_FAKE_DISPLAY (w->screen->display)))

#define NUM_OPTIONS(s) (sizeof ((s)->opt) / sizeof (CompOption))

static void fakeToggleWindow(CompWindow *w)
{
	FAKE_WINDOW(w);
	FAKE_SCREEN(w->screen);
	if (fs->black)
		fw->isFaked = !fw->isFaked;
	if (fw->isFaked)
	{
		fs->handle = 0;
		fs->black = !fs->black;
	}
	addWindowDamage(w);
}

static Bool
fakeToggle(CompDisplay * d, CompAction * action, CompActionState state,
		  CompOption * option, int nOption)
{
	CompWindow *w;
	Window xid;

	xid = getIntOptionNamed(option, nOption, "window", 0);

	w = findWindowAtDisplay(d, xid);

	if (w)
		fakeToggleWindow(w);

	return TRUE;
}

static int
getFakeFragmentFunction (CompScreen  *s, CompTexture *texture)
{
    CompFunctionData *data;

    FAKE_SCREEN (s);

	int target;

	if (texture->target == GL_TEXTURE_2D)
		target = COMP_FETCH_TARGET_2D;
    else
		target = COMP_FETCH_TARGET_RECT;


	if (fs->handle)
		return fs->handle;

    data = createFunctionData ();
    if (data)
    {
		Bool ok = TRUE;
		int	 handle = 0;

		
		printf("Building shader");
		
			ok &= addTempHeaderOpToFunctionData (data, "neg" );
		ok &= addTempHeaderOpToFunctionData(data, "temp");

		ok &= addFetchOpToFunctionData (data, "output", NULL, target);
 		
 		ok &= addDataOpToFunctionData (data, "RCP neg.a, output.a;");
 		
 		
 		
 		ok &= addDataOpToFunctionData (data, "MUL output.rgb, output.a, output;");
	 		
		ok &= addDataOpToFunctionData(data, "MOV temp, output;");
		
		if (!fs->black)
			ok &= addDataOpToFunctionData(data, "SUB temp.rgb, 1.0, temp;");
		
		ok &= addDataOpToFunctionData(data, "ADD output.a, 0, temp.r;");
	
		ok &= addDataOpToFunctionData(data, "ADD output.a, output.a, temp.g;");
		ok &= addDataOpToFunctionData(data, "ADD output.a, output.a, temp.b;");
		ok &= addDataOpToFunctionData(data, "MUL output.a, output.a, 0.33;");
		ok &= addDataOpToFunctionData (data, "MUL output.rgb, output.a, output;");

		ok &= addColorOpToFunctionData (data, "output", "output");

		if (!ok)
		{
			destroyFunctionData (data);
			return 0;
		}

		handle = createFragmentFunction (s, "fake", data);
		fs->handle = handle;

		

		destroyFunctionData (data);


		return handle;
    }

    return 0;
}

static void fakeDrawWindowTexture(CompWindow * w,
					 CompTexture * texture,
					 const FragmentAttrib *attrib,
					 unsigned int mask)
{
	FAKE_SCREEN(w->screen);
	FAKE_WINDOW(w);
	FragmentAttrib fa = *attrib;

	
	if (fw->isFaked && (texture->name == w->texture->name))
	{

		if (w->screen->fragmentProgram)
		{
			int function;
			function = getFakeFragmentFunction (w->screen, texture);
	    		if (function)
	    		{
				addFragmentFunction (&fa, function);
			}
			UNWRAP(fs, w->screen, drawWindowTexture);
			(*w->screen->drawWindowTexture) (w, texture, &fa, mask);
			WRAP(fs, w->screen, drawWindowTexture, fakeDrawWindowTexture);
		}
	}
	
	else
	{
		UNWRAP(fs, w->screen, drawWindowTexture);
		(*w->screen->drawWindowTexture) (w, texture, &fa, mask);
		WRAP(fs, w->screen, drawWindowTexture, fakeDrawWindowTexture);
	}
}

static Bool fakeInitDisplay(CompPlugin * p, CompDisplay * d)
{
	FakeDisplay *fd;

	fd = malloc(sizeof(FakeDisplay));
	if (!fd)
		return FALSE;

	fd->screenPrivateIndex = allocateScreenPrivateIndex(d);
	if (fd->screenPrivateIndex < 0)
	{
		free(fd);
		return FALSE;
	}

	fakeargbSetWindowToggleInitiate(d, fakeToggle);

	d->privates[displayPrivateIndex].ptr = fd;

	return TRUE;
}

static void fakeFiniDisplay(CompPlugin * p, CompDisplay * d)
{
	FAKE_DISPLAY(d);
	freeScreenPrivateIndex(d, fd->screenPrivateIndex);
	free(fd);
}

static Bool fakeInitScreen(CompPlugin * p, CompScreen * s)
{
	FakeScreen *fs;

	FAKE_DISPLAY(s->display);

	fs = malloc(sizeof(FakeScreen));
	if (!fs)
		return FALSE;

	fs->windowPrivateIndex = allocateWindowPrivateIndex(s);
	if (fs->windowPrivateIndex < 0)
	{
		free(fs);
		return FALSE;
	}

	fs->handle = 0;

	WRAP(fs, s, drawWindowTexture, fakeDrawWindowTexture);

	s->privates[fd->screenPrivateIndex].ptr = fs;
		
	fs->black = TRUE;


	return TRUE;
}

static void fakeFiniScreen(CompPlugin * p, CompScreen * s)
{
	FAKE_SCREEN(s);
	freeWindowPrivateIndex(s, fs->windowPrivateIndex);
	UNWRAP(fs, s, drawWindowTexture);
	
	free(fs);
}

static Bool fakeInitWindow(CompPlugin * p, CompWindow * w)
{
	FakeWindow *fw;

	FAKE_SCREEN(w->screen);

	fw = malloc(sizeof(FakeWindow));
	if (!fw)
		return FALSE;

	fw->isFaked = FALSE;

	w->privates[fs->windowPrivateIndex].ptr = fw;

	return TRUE;
}

static void fakeFiniWindow(CompPlugin * p, CompWindow * w)
{
	FAKE_WINDOW(w);
	free(fw);
}

static Bool fakeInit(CompPlugin * p)
{
	displayPrivateIndex = allocateDisplayPrivateIndex();
	if (displayPrivateIndex < 0)
		return FALSE;

	return TRUE;
}

static void fakeFini(CompPlugin * p)
{
	if (displayPrivateIndex >= 0)
		freeDisplayPrivateIndex(displayPrivateIndex);
}

static int
fakeGetVersion (CompPlugin *plugin, int version)
{
	return ABIVERSION;
}

CompPluginVTable fakeVTable = {
	"fakeargb",
	fakeGetVersion,
	0,
	fakeInit,
	fakeFini,
	fakeInitDisplay,
	fakeFiniDisplay,
	fakeInitScreen,
	fakeFiniScreen,
	fakeInitWindow,
	fakeFiniWindow,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0
};

CompPluginVTable *getCompPluginInfo(void)
{
	return &fakeVTable;
}

