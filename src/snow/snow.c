/**
 *
 * Beryl snow plugin
 *
 * snow.c
 *
 * Copyright (c) 2006 Eckhart P. <beryl@cornergraf.net>
 * Copyright (c) 2006 Brian Jørgensen <qte@fundanemt.com>
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

/*
 * Many thanks to Atie H. <atie.at.matrix@gmail.com> for providing
 * a clean plugin template
 * Also thanks to the folks from #beryl-dev, especially Quinn_Storm
 * for helping me make this possible
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include <X11/Xatom.h>
#include <X11/extensions/Xrender.h>

#include <compiz.h>
#include "snow_options.h"

#define GET_SNOW_DISPLAY(d)                            \
	((SnowDisplay *) (d)->privates[displayPrivateIndex].ptr)

#define SNOW_DISPLAY(d)                                \
	SnowDisplay *sd = GET_SNOW_DISPLAY (d)

#define GET_SNOW_SCREEN(s, sd)                         \
	((SnowScreen *) (s)->privates[(sd)->screenPrivateIndex].ptr)

#define SNOW_SCREEN(s)                                 \
	SnowScreen *ss = GET_SNOW_SCREEN (s, GET_SNOW_DISPLAY (s->display))

static int displayPrivateIndex = 0;

// ------------------------------------------------------------  STRUCTS -----------------------------------------------------
typedef struct _SnowDisplay
{
	int screenPrivateIndex;
	Bool useTextures;

	int snowTexNFiles;
	CompOptionValue *snowTexFiles;
} SnowDisplay;

typedef struct _SnowScreen SnowScreen;

typedef struct _SnowTexture
{
	CompTexture tex;
	unsigned int width;
	unsigned int height;
	Bool loaded;
	GLuint dList;
} SnowTexture;

typedef struct _SnowFlake
{
	float x;
	float y;
	float z;
	float xs;
	float ys;
	float zs;
	float ra;					//rotation angle
	float rs;					//rotation speed

	SnowTexture *tex;
} SnowFlake;

static void InitiateSnowFlake(SnowScreen * ss, SnowFlake * sf);
static void setSnowflakeTexture(SnowScreen * ss, SnowFlake * sf);
static void beginRendering(SnowScreen * ss, CompScreen * s);
static void setupDisplayList(SnowScreen * ss);

static void snowThink(SnowScreen * ss, SnowFlake * sf);
static void snowMove(CompDisplay *d, SnowFlake * sf);

struct _SnowScreen
{
	CompScreen *s;

	Bool active;

	CompTimeoutHandle timeoutHandle;

	PaintOutputProc paintOutput;
	DrawWindowProc drawWindow;

	SnowTexture *snowTex;
	int snowTexturesLoaded;

	GLuint displayList;

	Bool displayListNeedsUpdate;

	SnowFlake *allSnowFlakes;
};


static void snowThink(SnowScreen * ss, SnowFlake * sf)
{
	int boxing;

	boxing = snowGetScreenBoxing(ss->s->display);

	if (sf->y >= ss->s->height + boxing
		|| sf->x <= -boxing
		|| sf->y >= ss->s->width + boxing
		|| sf->z <= -((float)snowGetScreenDepth(ss->s->display) / 500.0) || sf->z >= 1)

	{
		InitiateSnowFlake(ss, sf);
	}
	snowMove(ss->s->display, sf);
}

static void snowMove(CompDisplay *d, SnowFlake * sf)
{
	float tmp = 1.0f / (100.0f - snowGetSnowSpeed(d));
	int snowUpdateDelay = snowGetSnowUpdateDelay(d);

	sf->x += (sf->xs * (float)snowUpdateDelay) * tmp;
	sf->y += (sf->ys * (float)snowUpdateDelay) * tmp;
	sf->z += (sf->zs * (float)snowUpdateDelay) * tmp;
	sf->ra += ((float)snowUpdateDelay) / (10.0f - sf->rs);
}

static Bool stepSnowPositions(void *sc)
{
	CompScreen *s = sc;
	SNOW_SCREEN(s);

	if (!ss->active)
		return TRUE;

	int i = 0;
	SnowFlake *snowFlake = ss->allSnowFlakes;
	int numFlakes = snowGetNumSnowflakes(s->display);
	Bool onTop = snowGetSnowOverWindows(s->display);

	for (i = 0; i < numFlakes; i++)
		snowThink(ss, snowFlake++);

	if (ss->active && !onTop)
	{
		CompWindow *w;

		for (w = s->windows; w; w = w->next)
		{
			if (w->type & CompWindowTypeDesktopMask)
			{
				addWindowDamage(w);
			}
		}
	}
	else if (ss->active)
		damageScreen(s);

	return True;
}

static Bool
snowToggle(CompDisplay * d, CompAction * action, CompActionState state,
		   CompOption * option, int nOption)
{
	CompScreen *s;
	Window xid;

	xid = getIntOptionNamed(option, nOption, "root", 0);
	s = findScreenAtDisplay(d, xid);

	if (s)
	{
		SNOW_SCREEN(s);
		ss->active = !ss->active;
		if (!ss->active)
			damageScreen(s);
	}

	return TRUE;
}

// -------------------------------------------------  HELPER FUNCTIONS ----------------------------------------------------

int GetRand(int min, int max);
int GetRand(int min, int max)
{
	return (rand() % (max - min + 1) + min);
}

float mmrand(int min, int max, float divisor);
float mmrand(int min, int max, float divisor)
{
	return ((float)GetRand(min, max)) / divisor;
};

// ------------------------------------------------- RENDERING ----------------------------------------------------
static void setupDisplayList(SnowScreen * ss)
{
	float snowSize = snowGetSnowSize(ss->s->display);
	// ----------------- untextured list

	ss->displayList = glGenLists(1);

	glNewList(ss->displayList, GL_COMPILE);
	glBegin(GL_QUADS);

	glColor4f(1.0, 1.0, 1.0, 1.0);
	glVertex3f(0, 0, -0.0);
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glVertex3f(0, snowSize, -0.0);
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glVertex3f(snowSize, snowSize, -0.0);
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glVertex3f(snowSize, 0, -0.0);

	glEnd();
	glEndList();

}

static void beginRendering(SnowScreen * ss, CompScreen * s)
{
	if (snowGetUseBlending(s->display))
		glEnable(GL_BLEND);

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	if (ss->displayListNeedsUpdate)
	{
		setupDisplayList(ss);
		ss->displayListNeedsUpdate = FALSE;
	}

	glColor4f(1.0, 1.0, 1.0, 1.0);
	if (ss->snowTexturesLoaded && snowGetUseTextures(s->display))
	{
		int j = 0;

		for (j = 0; j < ss->snowTexturesLoaded; j++)
		{
			enableTexture(ss->s, &ss->snowTex[j].tex,
						  COMP_TEXTURE_FILTER_GOOD);

			int i = 0;
			SnowFlake *snowFlake = ss->allSnowFlakes;
			int numFlakes = snowGetNumSnowflakes(s->display);
			Bool snowRotate = snowGetSnowRotation(s->display);

			for (i = 0; i < numFlakes; i++)
			{
				if (snowFlake->tex == &ss->snowTex[j])
				{
					glTranslatef(snowFlake->x, snowFlake->y, snowFlake->z);
					if (snowRotate)
						glRotatef(snowFlake->ra, 0, 0, 1);
					glCallList(ss->snowTex[j].dList);
					if (snowRotate)
						glRotatef(-snowFlake->ra, 0, 0, 1);
					glTranslatef(-snowFlake->x, -snowFlake->y, -snowFlake->z);
				}
				snowFlake++;
			}
			disableTexture(ss->s, &ss->snowTex[j].tex);
		}
	}
	else
	{
		int i = 0;
		SnowFlake *snowFlake = ss->allSnowFlakes;
		int numFlakes = snowGetNumSnowflakes(s->display);

		for (i = 0; i < numFlakes; i++)
		{
			glTranslatef(snowFlake->x, snowFlake->y, snowFlake->z);
			glRotatef(snowFlake->ra, 0, 0, 1);
			glCallList(ss->displayList);
			glRotatef(-snowFlake->ra, 0, 0, 1);
			glTranslatef(-snowFlake->x, -snowFlake->y, -snowFlake->z);
			snowFlake++;
		}
	}

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	if (snowGetUseBlending(s->display))
	{
		glDisable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	}
}

// -------------------------------------------------  FUNCTIONS ----------------------------------------------------

static Bool
snowPaintOutput(CompScreen * s,
								const ScreenPaintAttrib * sa,
								const CompTransform		* transform,
								Region region, CompOutput *output, 
								unsigned int mask)
{
	Bool status;

	SNOW_SCREEN(s);

	if (ss->active && !snowGetSnowOverWindows(s->display))
	{
		// This line is essential. Without it the snow will be rendered above (some) other windows.
		mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;
	}

	UNWRAP(ss, s, paintOutput);
	status = (*s->paintOutput) (s, sa, transform, region, output, mask);
	WRAP(ss, s, paintOutput, snowPaintOutput);

	if (ss->active && snowGetSnowOverWindows(s->display))
	{
		CompTransform sTransform = *transform;
		transformToScreenSpace (s, output, -DEFAULT_Z_CAMERA, &sTransform);

		glPushMatrix();
		glLoadMatrixf (sTransform.m);
		beginRendering(ss, s);
		glPopMatrix();
	}

	return status;
}

static Bool
snowDrawWindow(CompWindow * w, const CompTransform *transform, const FragmentAttrib *attrib,
				Region region, unsigned int mask)
{
	int status;

	SNOW_SCREEN(w->screen);

	// First draw Window as usual
	UNWRAP(ss, w->screen, drawWindow);
	status = (*w->screen->drawWindow) (w, transform, attrib, region, mask);
	WRAP(ss, w->screen, drawWindow, snowDrawWindow);

	// Check whether this is the Desktop Window
	if (ss->active && (w->type & CompWindowTypeDesktopMask) && 
		!snowGetSnowOverWindows(w->screen->display))
	{
		beginRendering(ss, w->screen);
	}

	return status;
}

static void InitiateSnowFlake(SnowScreen * ss, SnowFlake * sf)
{
	//TODO: possibly place snowflakes based on FOV, instead of a cube.
	int boxing = snowGetScreenBoxing(ss->s->display);

	switch(snowGetSnowDirection(ss->s->display))
	{
		case SnowDirectionTopToBottom:
			sf->x = mmrand(-boxing, ss->s->width + boxing, 1);
			sf->xs = mmrand(-1, 1, 500);
			sf->y = mmrand(-300, 0, 1);
			sf->ys = mmrand(1, 3, 1);
			break;
		case SnowDirectionBottomToTop:
			sf->x = mmrand(-boxing, ss->s->width + boxing, 1);
			sf->xs = mmrand(-1, 1, 500);
			sf->y = mmrand(ss->s->height, ss->s->height + 300, 1);
			sf->ys = -mmrand(1, 3, 1);
			break;
		case SnowDirectionRightToLeft:
			sf->x = mmrand(ss->s->width, ss->s->width + 300, 1);
			sf->xs = -mmrand(1, 3, 1);
			sf->y = mmrand(-boxing, ss->s->height + boxing, 1);
			sf->ys = mmrand(-1, 1, 500);
			break;
		case SnowDirectionLeftToRight:
			sf->x = mmrand(-300, 0, 1);
			sf->xs = mmrand(1, 3, 1);
			sf->y = mmrand(-boxing, ss->s->height + boxing, 1);
			sf->ys = mmrand(-1, 1, 500);
			break;
		default:
			break;
	}

	sf->z = mmrand(-snowGetScreenDepth(ss->s->display), 0.1, 5000);
	sf->zs = mmrand(-1000, 1000, 500000);
	sf->ra = mmrand(-1000, 1000, 50);
	sf->rs = mmrand(-1000, 1000, 1000);
}

static void setSnowflakeTexture(SnowScreen * ss, SnowFlake * sf)
{
	if (ss->snowTexturesLoaded)
		sf->tex = &ss->snowTex[rand() % ss->snowTexturesLoaded];
}

static void updateSnowTextures(CompScreen * s)
{
	SNOW_SCREEN(s);
	SNOW_DISPLAY(s->display);
	int i = 0;

	for (i = 0; i < ss->snowTexturesLoaded; i++)
	{
		finiTexture(s, &ss->snowTex[i].tex);
		glDeleteLists(ss->snowTex[i].dList, 1);
	}
	if (ss->snowTexturesLoaded)
		free(ss->snowTex);
	ss->snowTexturesLoaded = 0;

	ss->snowTex = calloc(1, sizeof(SnowTexture) * sd->snowTexNFiles);

	int count = 0;
	float snowSize = snowGetSnowSize(s->display);

	for (i = 0; i < sd->snowTexNFiles; i++)
	{
		ss->snowTex[count].loaded =
				readImageToTexture(s, &ss->snowTex[count].tex,
								   sd->snowTexFiles[i].s,
								   &ss->snowTex[count].width,
								   &ss->snowTex[count].height);
		if (!ss->snowTex[count].loaded)
		{
			compLogMessage (s->display, "snow", CompLogLevelWarn,
							"Texture not found : %s", sd->snowTexFiles[i].s);
			continue;
		}
		compLogMessage (s->display, "snow", CompLogLevelInfo,
						"Loaded Texture %s", sd->snowTexFiles[i].s);
		CompMatrix *mat = &ss->snowTex[count].tex.matrix;
		SnowTexture *sTex = &ss->snowTex[count];

		sTex->dList = glGenLists(1);

		glNewList(sTex->dList, GL_COMPILE);

		glBegin(GL_QUADS);

		glTexCoord2f(COMP_TEX_COORD_X(mat, 0), COMP_TEX_COORD_Y(mat, 0));
		glVertex2f(0, 0);
		glTexCoord2f(COMP_TEX_COORD_X(mat, 0),
					 COMP_TEX_COORD_Y(mat, sTex->height));
		glVertex2f(0, snowSize * sTex->height / sTex->width);
		glTexCoord2f(COMP_TEX_COORD_X(mat, sTex->width),
					 COMP_TEX_COORD_Y(mat, sTex->height));
		glVertex2f(snowSize, snowSize * sTex->height / sTex->width);
		glTexCoord2f(COMP_TEX_COORD_X(mat, sTex->width),
					 COMP_TEX_COORD_Y(mat, 0));
		glVertex2f(snowSize, 0);

		glEnd();
		glEndList();

		count++;
	}

	ss->snowTexturesLoaded = count;
	if (count < sd->snowTexNFiles)
		ss->snowTex = realloc(ss->snowTex, sizeof(SnowTexture) * count);

	SnowFlake *snowFlake = ss->allSnowFlakes;
	int numFlakes = snowGetNumSnowflakes(s->display);

	for (i = 0; i < numFlakes; i++)
	{
		setSnowflakeTexture(ss, snowFlake++);
	}
}

static Bool snowInitScreen(CompPlugin * p, CompScreen * s)
{
	SNOW_DISPLAY(s->display);

	SnowScreen *ss = (SnowScreen *) calloc(1, sizeof(SnowScreen));

	ss->s = s;
	s->privates[sd->screenPrivateIndex].ptr = ss;

	ss->snowTexturesLoaded = 0;
	ss->snowTex = NULL;
	ss->active = FALSE;
	ss->displayListNeedsUpdate = FALSE;

	int i = 0;
	int numFlakes = snowGetNumSnowflakes(s->display);

	ss->allSnowFlakes = malloc(numFlakes * sizeof(SnowFlake));
	SnowFlake *snowFlake = ss->allSnowFlakes;

	for (i = 0; i < numFlakes; i++)
	{
		InitiateSnowFlake(ss, snowFlake);
		setSnowflakeTexture(ss, snowFlake);
		snowFlake++;
	}

	updateSnowTextures(s);
	setupDisplayList(ss);

	WRAP(ss, s, paintOutput, snowPaintOutput);
	WRAP(ss, s, drawWindow, snowDrawWindow);

	ss->timeoutHandle = compAddTimeout(snowGetSnowUpdateDelay(s->display), stepSnowPositions, s);

	return TRUE;
}

static void snowFiniScreen(CompPlugin * p, CompScreen * s)
{
	SNOW_SCREEN(s);

	if (ss->timeoutHandle)
		compRemoveTimeout(ss->timeoutHandle);

	int i = 0;

	for (i = 0; i < ss->snowTexturesLoaded; i++)
	{
		finiTexture(s, &ss->snowTex[i].tex);
		glDeleteLists(ss->snowTex[i].dList, 1);
	}
	if (ss->snowTexturesLoaded)
		free(ss->snowTex);

	if (ss->allSnowFlakes)
		free(ss->allSnowFlakes);

	//Restore the original function
	UNWRAP(ss, s, paintOutput);
	UNWRAP(ss, s, drawWindow);

	//Free the pointer
	free(ss);
}

static void snowDisplayOptionChanged(CompDisplay *d, CompOption *opt, SnowDisplayOptions num)
{
	SNOW_DISPLAY(d);

	switch (num)
	{
		case SnowDisplayOptionSnowSize:
			{
				CompScreen *s;

				for (s = d->screens; s; s = s->next)
				{
					SNOW_SCREEN(s);
					ss->displayListNeedsUpdate = TRUE;
					updateSnowTextures(s);
				}
			}
			break;
		case SnowDisplayOptionSnowUpdateDelay:
			{
				CompScreen *s;

				for (s = d->screens; s; s = s->next)
				{
					SNOW_SCREEN(s);
					
					if (ss->timeoutHandle)
						compRemoveTimeout(ss->timeoutHandle);
					ss->timeoutHandle =
						compAddTimeout(snowGetSnowUpdateDelay(d), stepSnowPositions, s);
				}
			}
			break;
		case SnowDisplayOptionNumSnowflakes:
			{
				CompScreen *s;
				int i, numFlakes;
				SnowFlake *snowFlake;

				numFlakes = snowGetNumSnowflakes(d);
				for (s = d->screens; s; s = s->next)
				{
					SNOW_SCREEN(s);
					ss->allSnowFlakes = realloc(ss->allSnowFlakes, numFlakes * sizeof(SnowFlake));
					snowFlake = ss->allSnowFlakes;

					for (i = 0; i < numFlakes; i++)
					{
						InitiateSnowFlake(ss, snowFlake);
						setSnowflakeTexture(ss, snowFlake);
						snowFlake++;
					}
				}
			}
			break;
		case SnowDisplayOptionSnowTextures:
			{
				CompScreen *s;
				CompOption *texOpt;

				texOpt = snowGetSnowTexturesOption(d);

				sd->snowTexFiles = texOpt->value.list.value;
				sd->snowTexNFiles = texOpt->value.list.nValue;

				for (s = d->screens; s; s = s->next)
					updateSnowTextures(s);
			}
			break;
		default:
			break;
	}
}

static Bool snowInitDisplay(CompPlugin * p, CompDisplay * d)
{
	CompOption *texOpt;
	//Generate a snow display
	SnowDisplay *sd = (SnowDisplay *) malloc(sizeof(SnowDisplay));

	//Allocate a private index
	sd->screenPrivateIndex = allocateScreenPrivateIndex(d);

	//Check if its valid
	if (sd->screenPrivateIndex < 0)
	{
		free(sd);
		return FALSE;
	}
	
	snowSetToggleInitiate(d, snowToggle);
	snowSetNumSnowflakesNotify(d, snowDisplayOptionChanged);
	snowSetSnowSizeNotify(d, snowDisplayOptionChanged);
	snowSetSnowUpdateDelayNotify(d, snowDisplayOptionChanged);
	snowSetSnowTexturesNotify(d, snowDisplayOptionChanged);

	texOpt = snowGetSnowTexturesOption(d);
	sd->snowTexFiles = texOpt->value.list.value;
	sd->snowTexNFiles = texOpt->value.list.nValue;

	//Record the display
	d->privates[displayPrivateIndex].ptr = sd;

	return TRUE;
}

static void snowFiniDisplay(CompPlugin * p, CompDisplay * d)
{
	SNOW_DISPLAY(d);

	//Free the private index
	freeScreenPrivateIndex(d, sd->screenPrivateIndex);
	//Free the pointer
	free(sd);
}

static Bool snowInit(CompPlugin * p)
{
	displayPrivateIndex = allocateDisplayPrivateIndex();

	if (displayPrivateIndex < 0)
		return FALSE;

	return TRUE;
}

static void snowFini(CompPlugin * p)
{
	if (displayPrivateIndex >= 0)
		freeDisplayPrivateIndex(displayPrivateIndex);
}

static int snowGetVersion(CompPlugin *p, int version)
{
	return ABIVERSION;
}

CompPluginVTable snowVTable = {
	"snow",
	snowGetVersion,
	0,
	snowInit,
	snowFini,
	snowInitDisplay,
	snowFiniDisplay,
	snowInitScreen,
	snowFiniScreen,
	0,
	0,
	0,
	0,
	0,							/*snowGetScreenOptions */
	0,							/*snowSetScreenOption */
	0,
	0,
	0,
	0
};

CompPluginVTable *getCompPluginInfo(void)
{
	return &snowVTable;
}
