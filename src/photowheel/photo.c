/******************************
Copyright stuff goes here =)
...
Based on Gears (see below)
Joel Bosveld (Joel.Bosveld@gmail.com)
******************************/

/*
 * Compiz cube gears plugin
 *
 * gears.c
 *
 * This is an example plugin to show how to render something inside
 * of the transparent cube
 *
 * Copyright : (C) 2007 by Dennis Kasprzyk
 * E-mail    : onestone@compiz.org
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
 * Based on glxgears.c:
 *    http://cvsweb.xfree86.org/cvsweb/xc/programs/glxgears/glxgears.c
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <math.h>

#include <compiz-core.h>
#include <compiz-cube.h>

#include "photo_options.h"

#define PI 3.14159

static int displayPrivateIndex;

static int cubeDisplayPrivateIndex;

typedef struct _PhotoTexture
{
	CompTexture tex;

	unsigned height;
	unsigned width;

	GLuint dList;
}
PhotoTexture;

typedef struct _PhotoDisplay
{
    int screenPrivateIndex;

}
PhotoDisplay;

typedef struct _PhotoScreen
{
    DonePaintScreenProc    donePaintScreen;
    PreparePaintScreenProc preparePaintScreen;

    CubeClearTargetOutputProc clearTargetOutput;
    CubePaintInsideProc       paintInside;

    Bool damage;

    Bool firsttime;

    Bool transformOrder;

    Bool enablecull;

	PhotoTexture *photoTexture;
	int n; /* how many sides on prism */
	GLfloat l; /* how big is the prism */
	GLfloat h; /* how high is the prism */
	float rspeed; /* speed of rotation */

	float xpos;
	float ypos;
	float zpos;

	GLuint dList; /* caps */

	CompListValue *photoTexFiles;

    float angle;

}
PhotoScreen;

#define GET_PHOTO_DISPLAY(d) \
    ((PhotoDisplay *) (d)->base.privates[displayPrivateIndex].ptr)
#define PHOTO_DISPLAY(d) \
    PhotoDisplay *gd = GET_PHOTO_DISPLAY(d);                            /* Macros to get the gd pointer to PhotoDisplay struct */

#define GET_PHOTO_SCREEN(s, gd) \
    ((PhotoScreen *) (s)->base.privates[(gd)->screenPrivateIndex].ptr)
#define PHOTO_SCREEN(s) \
    PhotoScreen *gs = GET_PHOTO_SCREEN(s, GET_PHOTO_DISPLAY(s->display))     /* Macros to get the gs pointer to PhotoScreen struct */


static void
photoParamChange (CompScreen *s,
				  CompOption *opt,
				  PhotoScreenOptions num)
	{
	PHOTO_SCREEN (s);

    gs->h= photoGetHeight (s);
    gs->l= photoGetWidth (s);
    gs->rspeed = photoGetSpeed (s);
    gs->xpos = photoGetXpos (s);
    gs->ypos = photoGetYpos (s);
    gs->zpos = photoGetZpos (s);
    gs->transformOrder = photoGetOrder (s);
    gs->enablecull = photoGetCull (s);
	}


static void
photoCapChange (CompScreen *s,
				CompOption *opt,
				PhotoScreenOptions num)

	{

	PHOTO_SCREEN (s);
	int i;
	unsigned short *top;
	unsigned short *bot;

	top = photoGetTopColour (s);
	bot = photoGetBottomColour (s);


	if(!(gs->firsttime))
	{
		glDeleteLists (gs->dList, 1);
	}

    gs->dList = glGenLists (1);
    glNewList (gs->dList, GL_COMPILE);
	glBegin(GL_POLYGON);

			glColor4us(top[0],top[1],top[2],top[3]);

		for(i=gs->n-1;i>-1;i--)
			{
			glVertex3f( cos(i*2*PI/(gs->n)), 1,  sin(i*2*PI/(gs->n)) );
			}

	glEnd();

	glBegin(GL_POLYGON);

		glColor4us(bot[0],bot[1],bot[2],bot[3]);

		for(i=0;i<(gs->n);i++)
			{
			glVertex3f( cos(i*2*PI/(gs->n)), -1,  sin(i*2*PI/(gs->n)) );
			}

	glEnd();
    glEndList();
	}

static void
photoTextureChange (CompScreen *s,
				  CompOption *opt,
				  PhotoScreenOptions num)
	{
    PHOTO_SCREEN (s);

    int i;
    int i2;

    if(!(gs->firsttime))
	{
	for(i=0;i<(gs->n);i++)
			{
			finiTexture (s, &gs->photoTexture[i].tex);
			glDeleteLists (gs->photoTexture[i].dList, 1);
			}

		glDeleteLists (gs->dList, 1);
	}

    CompMatrix *mat;


    gs->photoTexFiles = photoGetPhotoTextures (s);
    gs->n=gs->photoTexFiles->nValue;

    gs->photoTexture=malloc(sizeof (PhotoTexture) * gs-> n);

    for(i=0;i<(gs->n);i++)
	{
	initTexture(s, &(gs->photoTexture[i].tex));
	if(!(readImageToTexture (s, &(gs->photoTexture[i]).tex,  		/*texture pointer*/
						 gs->photoTexFiles->value[i].s,                       		/*file*/
						 &(gs->photoTexture[i]).width,                        /*pointer for width*/
						 &(gs->photoTexture[i]).height)))				/*pointer for height*/
               {
               compLogMessage ("photo", CompLogLevelWarn,
			"Failed to load image: %s",
			gs->photoTexFiles->value[i].s);

               finiTexture(s, &(gs->photoTexture[i].tex));
               initTexture(s, &(gs->photoTexture[i].tex));
               }

         mat = &gs->photoTexture[i].tex.matrix;
         gs->photoTexture[i].dList = glGenLists (1);

         i2=i+1;
         if((i2==gs->n))i2=0;

         glNewList (gs->photoTexture[i].dList, GL_COMPILE);
         glBegin(GL_QUADS);

		glTexCoord2f (COMP_TEX_COORD_X (mat, (gs->photoTexture[i]).width),
			COMP_TEX_COORD_Y (mat, 0));
				glVertex3f( cos(i*2*PI/(gs->n)), 1.0f, sin(i*2*PI/(gs->n)) );	// Top Left Of The Texture and Quad

		glTexCoord2f (COMP_TEX_COORD_X (mat, 0),
			 COMP_TEX_COORD_Y (mat, 0));
				glVertex3f( cos(i2*2*PI/(gs->n)), 1.0f, sin(i2*2*PI/(gs->n)) );	// Top Right Of The Texture and Quad

		glTexCoord2f (COMP_TEX_COORD_X (mat, 0),
			COMP_TEX_COORD_Y (mat, (gs->photoTexture[i]).height));
				glVertex3f( cos(i2*2*PI/(gs->n)), -1.0f, sin(i2*2*PI/(gs->n)) );	// Bot Right Of The Texture and Quad

		glTexCoord2f (COMP_TEX_COORD_X (mat, (gs->photoTexture[i]).width),
			COMP_TEX_COORD_Y (mat, (gs->photoTexture[i]).height));
				glVertex3f(  cos(i*2*PI/(gs->n)), -1.0f, sin(i*2*PI/(gs->n)) );	// Bot Left Of The Texture and Quad

	glEnd();
	glEndList ();
	}

	photoCapChange (s, NULL, 0);

	}

static void
photoClearTargetOutput (CompScreen *s,    /* Doesn't do anything except glClear(GL_DEPTH_BUFFER_BIT)? - cube must change it? */
			float      xRotate,
			float      vRotate)
{
    PHOTO_SCREEN (s);
    CUBE_SCREEN (s);

    UNWRAP (gs, cs, clearTargetOutput);
    (*cs->clearTargetOutput) (s, xRotate, vRotate);
    WRAP (gs, cs, clearTargetOutput, photoClearTargetOutput);

    glClear (GL_DEPTH_BUFFER_BIT);
}

static void                       /* draws the photo, then calls the cube function */
photoPaintInside (CompScreen              *s,
		  const ScreenPaintAttrib *sAttrib,
		  const CompTransform     *transform,
		  CompOutput              *output,
		  int                     size)
{
    PHOTO_SCREEN (s);
    CUBE_SCREEN (s);

	int i;
	Bool enabled;

    ScreenPaintAttrib sA = *sAttrib;

	sA.yRotate += cs->invert * (360.0f / size) *
		(cs->xRotations - (s->x * cs->nOutput)); /*?*/

    CompTransform mT = *transform;

    (*s->applyScreenTransform) (s, &sA, output, &mT);

    glPushMatrix();
    glLoadMatrixf (mT.m);
    glTranslatef (cs->outputXOffset, -cs->outputYOffset, 0.0f);   /* OpenGL stuff to set correct transformation due to cube rotation? */
    glScalef (cs->outputXScale, cs->outputYScale, 1.0f);

    glPushAttrib (GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT);

    glEnable (GL_BLEND);

	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	enabled = glIsEnabled (GL_CULL_FACE);

	if(gs->enablecull)glEnable(GL_CULL_FACE);
	else glDisable(GL_CULL_FACE);


	glPushMatrix();

	glDisable (GL_LIGHTING);
	glDisable (GL_COLOR_MATERIAL);
	glEnable (GL_DEPTH_TEST);


/******************************/

        glScalef (0.25f, 0.25f, 0.25f);

        if(gs->transformOrder)glTranslatef(gs->xpos,gs->ypos,gs->zpos);

        glRotatef((gs->angle),0.0f,1.0f,0.0f);

        if(!(gs->transformOrder))glTranslatef(gs->xpos,gs->ypos,gs->zpos);

                (gs->angle)+=(gs->rspeed);
			if((gs->angle>=360.0f)) (gs->angle)=0.0f;


        glScalef (gs->l, gs->h, gs->l);

		for(i=0; i<(gs->n);i++)
			{
		glEnable (gs->photoTexture[i].tex.target);

		enableTexture (s, &(gs->photoTexture[i].tex),
			COMP_TEXTURE_FILTER_GOOD);
		glCallList (gs->photoTexture[i].dList);

			disableTexture (s, &(gs->photoTexture[i].tex));
			}

		glCallList(gs->dList);



/****************************************/

    glPopMatrix();

    glEnable (GL_COLOR_MATERIAL);

    if (s->lighting)
	glEnable (GL_LIGHTING);

    glDisable (GL_DEPTH_TEST);
    glDisable (GL_BLEND);

    if (enabled) glEnable (GL_CULL_FACE);
    else glDisable (GL_CULL_FACE);    /* resets settings to original */

    glPopMatrix();
    glPopAttrib();

    gs->damage = TRUE;

    UNWRAP (gs, cs, paintInside);
    (*cs->paintInside) (s, sAttrib, transform, output, size);
    WRAP (gs, cs, paintInside, photoPaintInside);
}

static void            /* Rotates the various photo ... */
photoPreparePaintScreen (CompScreen *s,
			 int        ms)
{
    PHOTO_SCREEN (s);



    UNWRAP (gs, s, preparePaintScreen);
    (*s->preparePaintScreen) (s, ms);
    WRAP (gs, s, preparePaintScreen, photoPreparePaintScreen);
}

static void /* Calls damageScreen ? */
photoDonePaintScreen (CompScreen * s)
{
    PHOTO_SCREEN (s);

    if (gs->damage)
    {
	damageScreen (s);
	gs->damage = FALSE;
    }

    UNWRAP (gs, s, donePaintScreen);
    (*s->donePaintScreen) (s);
    WRAP (gs, s, donePaintScreen, photoDonePaintScreen);
}

/* Inits display */

static Bool
photoInitDisplay (CompPlugin  *p,
		  CompDisplay *d)
{
    PhotoDisplay *gd;

    if (!checkPluginABI ("core", CORE_ABIVERSION) ||
	!checkPluginABI ("cube", CUBE_ABIVERSION))
	return FALSE;

    if (!getPluginDisplayIndex (d, "cube", &cubeDisplayPrivateIndex))
	return FALSE;

    gd = malloc (sizeof (PhotoDisplay));    /* creates data struct - remainder is error checking */

    if (!gd)
	return FALSE;

    gd->screenPrivateIndex = allocateScreenPrivateIndex (d);

    if (gd->screenPrivateIndex < 0)
    {
	free (gd);
	return FALSE;
    }

    d->base.privates[displayPrivateIndex].ptr = gd;  /* allows use of PHOTO_DISPLAY macro */

    return TRUE;
}

static void
photoFiniDisplay (CompPlugin  *p,
		  CompDisplay *d)
{
    PHOTO_DISPLAY (d);

    freeScreenPrivateIndex (d, gd->screenPrivateIndex);
    free (gd);
}


/* Function which is run when screen is initialized */

static Bool
photoInitScreen (CompPlugin *p,
		 CompScreen *s)
{
    PhotoScreen *gs;

    PHOTO_DISPLAY (s->display);    /* sets gd pointer used below */

    CUBE_SCREEN (s);

    gs = malloc (sizeof (PhotoScreen) );            /* creates gearScreen struct */

    if (!gs)
	return FALSE;

    s->base.privates[gd->screenPrivateIndex].ptr = gs;           /* For the PHOTO_SCREEN macro to work  */

	/*** My Code ***/
	gs->firsttime=TRUE;
	gs->rspeed=photoGetSpeed (s);
	gs->xpos=photoGetXpos (s);
	gs->ypos=photoGetYpos (s);
	gs->zpos=photoGetZpos (s);
	gs->transformOrder = photoGetOrder (s);

	photoSetPhotoTexturesNotify (s, photoTextureChange);
	photoSetHeightNotify (s, photoParamChange);
	photoSetWidthNotify (s, photoParamChange);
	photoSetSpeedNotify (s, photoParamChange);
	photoSetXposNotify (s, photoParamChange);
	photoSetYposNotify (s, photoParamChange);
	photoSetZposNotify (s, photoParamChange);
	photoSetOrderNotify (s, photoParamChange);

	photoSetTopColourNotify (s, photoCapChange);
	photoSetBottomColourNotify (s, photoCapChange);
	photoSetCullNotify (s, photoParamChange);

	photoTextureChange (s, NULL, 0);
	photoParamChange (s, NULL, 0);


	/*** End of my Code ***/

    WRAP (gs, s, donePaintScreen, photoDonePaintScreen);                 /*Functions version of ... is called instead of cores (which then also calls core function) */
    WRAP (gs, s, preparePaintScreen, photoPreparePaintScreen);
    WRAP (gs, cs, clearTargetOutput, photoClearTargetOutput);
    WRAP (gs, cs, paintInside, photoPaintInside);

    return TRUE;

}

/* Fuction is run when screen is uninitialized */

static void
photoFiniScreen (CompPlugin *p,
		 CompScreen *s)
{
    PHOTO_SCREEN (s);                     /*Sets the pointers cs gs */
    CUBE_SCREEN (s);
    int i;

	for(i=0;i<(gs->n);i++)
		{
		finiTexture (s, &gs->photoTexture[i].tex);
		glDeleteLists (gs->photoTexture[i].dList, 1);
		}

	glDeleteLists (gs->dList, 1);

	free(gs->photoTexture);



    UNWRAP (gs, s, donePaintScreen);                /* gs is pointer to structure GearScreen, s is pointer to structure screen? - these functions are core functions*/
    UNWRAP (gs, s, preparePaintScreen);

    UNWRAP (gs, cs, clearTargetOutput);            /* ... cs is pointer to structure CubeScreen? - these two functions are part of the Cube plugin*/
    UNWRAP (gs, cs, paintInside);



    free (gs);
}

/* Load and Unload plugin */

static Bool
photoInit (CompPlugin * p)
{
    displayPrivateIndex = allocateDisplayPrivateIndex();

    if (displayPrivateIndex < 0)
	return FALSE;

    return TRUE;
}

static void
photoFini (CompPlugin * p)
{
    if (displayPrivateIndex >= 0)
	freeDisplayPrivateIndex (displayPrivateIndex);
}

/* Init Object - lists functions for Core, Display and Screen */

static CompBool
photoInitObject (CompPlugin *p,
		 CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0, /* InitCore */
	(InitPluginObjectProc) photoInitDisplay,
	(InitPluginObjectProc) photoInitScreen
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
photoFiniObject (CompPlugin *p,
		 CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0, /* FiniCore */
	(FiniPluginObjectProc) photoFiniDisplay,
	(FiniPluginObjectProc) photoFiniScreen
    };

    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

/* VTable */

CompPluginVTable photoVTable = {
    "photo",
    0,          /* (?) */
    photoInit,
    photoFini,
    photoInitObject,
    photoFiniObject,
    0,         /* Get(?) Object Options */
    0          /* Set(?) Object Options */
};

CompPluginVTable *
getCompPluginInfo (void)
{
    return &photoVTable;
}
