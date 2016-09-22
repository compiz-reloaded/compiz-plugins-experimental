    ////////////////////////////////////////////////////////////////////////
   //////                  Elements for Compiz Fusion                //////
  ////////////////////////////////////////////////////////////////////////
/*                                                                       *
 * 									 *
 * 		     Copyleft (c) 2008 Patrick Fisher			 *
 *			   pat@elementsplugin.com			 *
 *		      http://www.elementsplugin.com/  			 *
 *		        Based on the Snow Plugin			 *
 * 		    By Eckhart P. and Brian Jørgensen			 *
	

 *         This program is free software; you can redistribute           *
 *           it and/or modify it under the terms of the GNU              *
 *                 General Public License version 2                      *

 
 *          This program is distributed without ANY warranty		 *
 *		WHATSOEVER. Not even an IMPLIED warranty		 *
 *	     of MERCHANTABILITY or FITNESS for ANY PURPOSE.		 *
 *	  See the GNU General Public License for more details. 		 *


 *	     Elements was written from the ground-up, but is 
 *	    both inspired and based on the Snow, Stars, Autumn		 *
 *	    and Fireflies Plugins, written by various authors 		 *
 *	     listed below. I (Pat Fisher) would like to thank		 *
 * 	     each and every one of them for their help and		 *
 *			       inspiration.				 *

 *			Snow and Fireflies Plugins: 
 * 	  Copyright (c) 2006 Eckhart P. <beryl@cornergraf.net>		 *
 *	  Copyright (c) 2006 Brian Jørgensen <qte@fundanemt.com>	 *
 * 	 								 *
 * 			       Stars Plugin:				 *
 * 	  Copyright (c) 2007 Kyle Mallory <kyle.mallory@utah.edu>	 *
 * 	 								 *
 * 			       Autumn Plugin:				 *
 * 	  Copyright (c) 2007 Pat Fisher <pat@elementsplugin.com>	 *

 *			     Special thanks to:				 *
 *	   Simon Strumse who was willing to sacrifice his time,		 *
 *	     and effort to make sure that Elements was ready.		 *
 *	     This guy even had to re-install his OS because of		 *
 *		  one of the Elements Alphas. Thanks!			 *
 *									 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */



#include <compiz-core.h>
#include "elements_options.h"
#define GET_DISPLAY(d)                            \
	((eDisplay *) (d)->base.privates[displayPrivateIndex].ptr)

#define E_DISPLAY(d)                                \
	eDisplay *ed = GET_DISPLAY (d)

#define GET_SCREEN(s, ed)                         \
	((screen *) (s)->base.privates[(ed)->privateIndex].ptr)

#define E_SCREEN(s)                                 \
	screen *eScreen = GET_SCREEN (s, GET_DISPLAY (s->display))

#define GLOW_STAGES		5
#define MAX_AUTUMN_AGE		100
#define BIG_NUMBER		20000			/* This is the number used to make sure that elements don't get all created in one place. 									Bigger number, more distance. Possibly fixable later.*/

static float glowCurve[GLOW_STAGES][4] = { { 0.0, 0.5, 0.5, 1.0 }, { 1.0, 1.0, 0.5, 0.75 }, { 0.75, 0.3, 1.2, 1.0 }, { 1.0, 0.7, 1.5, 1.0 }, { 1.0, 0.5, 0.5, 0.0 } };
static int displayPrivateIndex = 0;

typedef struct _eDisplay 			//This structure holds all the textures selected from the Compiz window
{
	int privateIndex;
	int numTex[5];
	CompOptionValue *texFiles[5];
} eDisplay;

typedef struct _texture
{
	CompTexture tex;
	unsigned int width;
	unsigned int height;
	Bool loaded;
	GLuint dList;
} texture;


typedef struct _element
{
	int type;				//Follows the usual pattern, alphabetic except for bubbles, which is 4.
	float x, y, z;
	float dx[4], dy[4], dz[4];		//matrix only used for Fireflies
	int autumnAge[2];			//Used by both Autumn and Bubbles. Determines which part of the autumnFloat matrix the Element is in
	float rSpeed;
	int rDirection;
	int rAngle;
	float autumnFloat[2][MAX_AUTUMN_AGE];	//Determines how much side-to-side the Bubbles and Leaves go. Fairly complicated. See initiateElement
	int autumnChange;
	float lifespan;
	float age;
	float lifecycle;
	float glowAlpha;
	texture *eTex;
} element;

typedef struct _screen
{
	CompScreen *cScreen;
	Bool isActive[5];
	Bool useKeys;
	CompTimeoutHandle timeoutHandle;
	PaintOutputProc paintOutput;
	DrawWindowProc  drawWindow;
	texture *textu;
	int numElements;
	int numTexLoaded[5];
	GLuint displayList;
	Bool   needUpdate;
	element *allElements;
} screen;

static void initiateElement (screen *eScreen, element *ele);
static void elementMove (CompDisplay *d, element *ele);
static void setElementTexture (screen *eScreen, element  *ele);
static void updateElementTextures (CompScreen *s, Bool changeTextures);
static inline float mmRand(int  min, int max, float divisor);
static void elementsDisplayOptionChanged (CompDisplay *d, CompOption *opt, ElementsDisplayOptions num);
float bezierCurve(float p[4], float time, int type);
static void createAll(CompDisplay *d);

static inline int
getRand (int min,
	 int max)
{
	return (rand() % (max - min + 1) + min);
}

static inline float
mmRand (int   min,
	int   max,
	float divisor)
{
	return ((float) getRand(min, max)) / divisor;
};


float bezierCurve(float p[4], float time, int type) {				//Used for Fireflies and Stars to move.
	float out = 0.0f;
	if (type == 3)
		out = p[0] * (time+0.01) * 10;
	else if (type == 1)
	{
		float a = p[0];
		float b = p[0] + p[1];
		float c = p[3] + p[2];
		float d = p[3];
		float e = 1.0 - time; 
		out = (a*(e*e*e)) + (3.0*b*(e*e)*time) + (3.0*c*e*(time*time)) + (d*(time*time*time));
	}
    return out;
}

static Bool
elementActive (CompScreen *s)
{
	int i;

	E_SCREEN (s);

	for (i = 0; i <= 4; i++)
		if (eScreen->isActive[i])
			return TRUE;

	return FALSE;
}

static void
elementTestCreate ( screen *currentScreen, element *ele)		//If the Element is outside of screen boxing, it is recreated. Else, it's moved.
{
	if (ele->y >= currentScreen->cScreen->height + 200|| ele->x <= -200|| ele->x >= currentScreen->cScreen->width + 200||
	ele->y >= currentScreen->cScreen->height + 200||
	ele->z <= -((float) elementsGetScreenDepth (currentScreen->cScreen->display) / 500.0) ||
	ele->z >= 1 || ele->y <= -200)			// Screen boxing has been replaced by a hard-coded number. It is 200, in this case.
	{
		initiateElement(currentScreen, ele);
	}

		elementMove(currentScreen->cScreen->display, ele);
}

static void
elementMove (CompDisplay *display, element *ele)
{

	float autumnSpeed = elementsGetAutumnSpeed (display)/30.0f;
	float ffSpeed = elementsGetFireflySpeed (display) / 700.0f;
	float snowSpeed = elementsGetSnowSpeed (display) / 500.0f;
	float starsSpeed = elementsGetStarsSpeed (display) / 500.0f;
	float bubblesSpeed = (100.0 - elementsGetViscosity (display))/30.0f;
	int   updateDelay = elementsGetUpdateDelay (display);

	if (ele->type == 0)
	{
		ele->x += (ele->autumnFloat[0][ele->autumnAge[0]] * (float) updateDelay) * 0.0125;
		ele->y += (ele->autumnFloat[1][ele->autumnAge[1]] * (float) updateDelay) * 0.0125 + autumnSpeed;
		ele->z += (ele->dz[0] * (float) updateDelay) * autumnSpeed / 100.0;
		ele->rAngle += ((float) updateDelay) / (10.1f - ele->rSpeed);
		ele->autumnAge[0] += ele->autumnChange;
		ele->autumnAge[1] += 1;
		if (ele->autumnAge[1] >= MAX_AUTUMN_AGE)
		{
			ele->autumnAge[1] = 0;
		}
		if (ele->autumnAge[0] >= MAX_AUTUMN_AGE)
		{
			ele->autumnAge[0] = MAX_AUTUMN_AGE - 1;
			ele->autumnChange = -1;
		}
		if (ele->autumnAge[0] <= -1)
		{
			ele->autumnAge[0] = 0;
			ele->autumnChange = 1;
		}

	}
	else if (ele->type == 1)
	{
	  	ele->age += 0.01;
		ele->lifecycle = (ele->age / 10) / ele->lifespan * (ffSpeed * 70);
		int glowStage = (ele->lifecycle * GLOW_STAGES);
		ele->glowAlpha = bezierCurve(glowCurve[glowStage], ele->lifecycle, ele->type);
		float xs = bezierCurve(ele->dx, ele->lifecycle, ele->type); 
		float ys = bezierCurve(ele->dy, ele->lifecycle, ele->type); 
		float zs = bezierCurve(ele->dz, ele->lifecycle, ele->type); 
		ele->x += (float)(xs * (double)updateDelay) * ffSpeed;
		ele->y += (float)(ys * (double)updateDelay) * ffSpeed;
		ele->z += (float)(zs * (double)updateDelay) * ffSpeed;
	}
	else if (ele->type == 2)
	{
		ele->x += (ele->dx[0] * (float) updateDelay) * snowSpeed;
		ele->y += (ele->dy[0] * (float) updateDelay) * snowSpeed;
		ele->z += (ele->dz[0] * (float) updateDelay) * snowSpeed;
		ele->rAngle += ((float) updateDelay) / (10.1f - ele->rSpeed);
	}
	else if (ele->type == 3)
	{
		float tmp = 1.0f / (100.0f - starsSpeed);
		float xs = bezierCurve(ele->dx, tmp, ele->type); 
		float ys = bezierCurve(ele->dy, tmp, ele->type); 
		float zs = bezierCurve(ele->dz, tmp, ele->type); 
		ele->x += (float)(xs * (double)updateDelay) * starsSpeed;
		ele->y += (float)(ys * (double)updateDelay) * starsSpeed;
		ele->z += (float)(zs * (double)updateDelay) * starsSpeed;
	}
	else if (ele->type == 4)
	{
		ele->x += (ele->autumnFloat[0][ele->autumnAge[0]] * (float) updateDelay) * 0.125;
		ele->y += (ele->dy[0] * (float) updateDelay) * bubblesSpeed;
		ele->z += (ele->dz[0] * (float) updateDelay) * bubblesSpeed / 100.0;
		ele->rAngle += ((float) updateDelay) / (10.1f - ele->rSpeed);
		ele->autumnAge[0] += ele->autumnChange;
		if (ele->autumnAge[0] >= MAX_AUTUMN_AGE)
		{
			ele->autumnAge[0] = MAX_AUTUMN_AGE - 1;
			ele->autumnChange = -9;
		}
		if (ele->autumnAge[0] <= -1)
		{
			ele->autumnAge[0] = 0;
			ele->autumnChange = 9;
		}

	}
	else
	{
		compLogMessage ("Elements", CompLogLevelWarn,
			    "Not a valid element type");
	}
}

static Bool
stepPositions(void *closure)
{
	CompScreen *s = closure;
	int i, numSnow, numAutumn, numStars, numFf, numBubbles, numTmp;
	element *ele;
	Bool onTopOfWindows;
	Bool active = elementActive(s);

	E_SCREEN(s);

	if (!active)
		return TRUE;

	ele = eScreen->allElements;

	if (eScreen->isActive[0])
		numAutumn = elementsGetNumLeaves (s->display);
	else
		numAutumn = 0;
	if (eScreen->isActive[1])
		numFf = elementsGetNumFireflies (s->display);
	else
		numFf = 0;
	if (eScreen->isActive[2])
		numSnow = elementsGetNumSnowflakes (s->display);
	else
		numSnow = 0;
	if (eScreen->isActive[3])
		numStars = elementsGetNumStars (s->display);
	else
		numStars = 0;
	if (eScreen->isActive[4])
		numBubbles = elementsGetNumBubbles (s->display);
	else
		numBubbles = 0;

	numTmp = numAutumn + numFf + numSnow + numStars + numBubbles;
	onTopOfWindows = elementsGetOverWindows (s->display);
	for (i = 0; i < numTmp; i++)
		elementTestCreate(eScreen, ele++);
	if (active && !onTopOfWindows )
	{
		CompWindow *w;
		for (w = s->windows; w; w = w->next)
		{
			if (w->type & CompWindowTypeDesktopMask)
				addWindowDamage (w);
		}
	}
	else if (active)
		damageScreen (s);

	return TRUE;
}

static Bool
elementsAutumnToggle (CompDisplay     *d,
	    CompAction      *action,
	    CompActionState state,
	    CompOption      *option,
	    int             nOption)
{
	Bool useKeys = FALSE;
	CompScreen *s;
	for (s = d->screens; s; s = s->next)
	{
		E_SCREEN (s);
		useKeys = eScreen->useKeys;
		if (useKeys)
		{
			eScreen->isActive[0] = !eScreen->isActive[0];
			damageScreen (s);
			eScreen->needUpdate = TRUE;
		}
	}
	if (useKeys)
	{
		createAll( d );
	}
	return TRUE;
}

static Bool
elementsFirefliesToggle (CompDisplay     *d,
	    CompAction      *action,
	    CompActionState state,
	    CompOption      *option,
	    int             nOption)
{
	Bool useKeys = FALSE;
	CompScreen *s;
	for (s = d->screens; s; s = s->next)
 	{
		E_SCREEN (s);
		useKeys = eScreen->useKeys;
		if (useKeys)
		{
			eScreen->isActive[1] = !eScreen->isActive[1];
			damageScreen (s);
			eScreen->needUpdate = TRUE;
		}
	}
	if (useKeys)
	{
		createAll( d );
	}
	return TRUE;
}
static Bool
elementsSnowToggle (CompDisplay     *d,
	    CompAction      *action,
	    CompActionState state,
	    CompOption      *option,
	    int             nOption)
{
	Bool useKeys = FALSE;
	CompScreen *s;
	for (s = d->screens; s; s = s->next)
	{
		E_SCREEN (s);
		useKeys = eScreen->useKeys;
		if (useKeys)
		{
			eScreen->isActive[2] = !eScreen->isActive[2];
			damageScreen (s);
			eScreen->needUpdate = TRUE;
		}
	}
	if (useKeys)
	{
		createAll( d );
	}
	return TRUE;
}
static Bool
elementsStarsToggle (CompDisplay     *d,
	    CompAction      *action,
	    CompActionState state,
	    CompOption      *option,
	    int             nOption)
{
	Bool useKeys = FALSE;
	CompScreen *s;
	for (s = d->screens; s; s = s->next)
	{
		E_SCREEN (s);
		useKeys = eScreen->useKeys;
		if (useKeys)
		{
			eScreen->isActive[3] = !eScreen->isActive[3];
			damageScreen (s);
			eScreen->needUpdate = TRUE;
		}
	}
	if (useKeys)
	{
		createAll( d );
	}
	return TRUE;
}
static Bool
elementsBubblesToggle (CompDisplay     *d,
	    CompAction      *action,
	    CompActionState state,
	    CompOption      *option,
	    int             nOption)
{
	Bool useKeys = FALSE;
	CompScreen *s;
	for (s = d->screens; s; s = s->next)
	{
		E_SCREEN (s);
		useKeys = eScreen->useKeys;
		if (useKeys)
		{
			eScreen->isActive[4] = !eScreen->isActive[4];
			damageScreen (s);
			eScreen->needUpdate = TRUE;
		}
	}
	if (useKeys)
	{
		createAll( d );
	}
	return TRUE;
}

static void
setupDisplayList (screen *eScreen)
{
	eScreen->displayList = glGenLists (1);
	glNewList (eScreen->displayList, GL_COMPILE);
	glBegin (GL_QUADS);
	glColor4f (1.0, 1.0, 1.0, 1.0);
	glVertex3f (0, 0, -0.0);
	glColor4f (1.0, 1.0, 1.0, 1.0);
	glVertex3f (0, 1.0, -0.0);
	glColor4f (1.0, 1.0, 1.0, 1.0);
	glVertex3f (1.0, 1.0, -0.0);
	glColor4f (1.0, 1.0, 1.0, 1.0);
	glVertex3f (1.0, 0, -0.0);
	glEnd ();
	glEndList ();
}

static void
beginRendering (screen *eScreen,
		CompScreen *s)
{
	int j;
	glEnable (GL_BLEND);
	glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	if (eScreen->needUpdate)
	{
		setupDisplayList (eScreen);
		eScreen->needUpdate = FALSE;
	}

	glColor4f (1.0, 1.0, 1.0, 1.0);
	for (j = 0; j < eScreen->numElements; j++)
		{
			element *ele = eScreen->allElements;
			int i, numAutumn, numFf, numSnow, numStars, numBubbles;
			if (eScreen->isActive[0])
				numAutumn = elementsGetNumLeaves (s->display);
			else
				numAutumn = 0;
			if (eScreen->isActive[1])
				numFf = elementsGetNumFireflies (s->display);
			else
				numFf = 0;
			if (eScreen->isActive[2])
				numSnow = elementsGetNumSnowflakes (s->display);
			else
				numSnow = 0;
			if (eScreen->isActive[3])
				numStars = elementsGetNumStars (s->display);
			else
				numStars = 0;
			if (eScreen->isActive[4])
				numBubbles = elementsGetNumBubbles (s->display);
			else
				numBubbles = 0;

			int numTmp = numAutumn + numFf + numSnow + numStars + numBubbles;
			Bool autumnRotate = elementsGetAutumnRotate (s->display);
			Bool snowRotate = elementsGetSnowRotate (s->display);
			Bool ffRotate = elementsGetFirefliesRotate (s->display);
			Bool starsRotate = elementsGetStarsRotate (s->display);
			Bool bubblesRotate = elementsGetBubblesRotate (s->display);

			enableTexture (eScreen->cScreen, &eScreen->textu[j].tex,
			   COMP_TEXTURE_FILTER_GOOD);

			for (i = 0; i < numTmp; i++)
			{
				if (ele->eTex == &eScreen->textu[j])
				{
					glTranslatef (ele->x, ele->y, ele->z);
					if (autumnRotate && ele->type == 0)
						glRotatef (ele->rAngle, 0, 0, 1);
					if (ffRotate && ele->type == 1)
						glRotatef (ele->rAngle, 0, 0, 1);
					if (snowRotate && ele->type == 2)
						glRotatef (ele->rAngle, 0, 0, 1);
					if (starsRotate && ele->type == 3)
						glRotatef (ele->rAngle, 0, 0, 1);
					if (bubblesRotate && ele->type == 4)
						glRotatef (ele->rAngle, 0, 0, 1);
					glCallList (eScreen->textu[j].dList);
					if (autumnRotate && ele->type == 0)
						glRotatef (-ele->rAngle, 0, 0, 1);
					if (ffRotate && ele->type == 1)
						glRotatef (-ele->rAngle, 0, 0, 1);
					if (snowRotate && ele->type == 2)
						glRotatef (-ele->rAngle, 0, 0, 1);
					if (starsRotate && ele->type == 3)
						glRotatef (-ele->rAngle, 0, 0, 1);
					if (bubblesRotate && ele->type == 4)
						glRotatef (-ele->rAngle, 0, 0, 1);
					glTranslatef (-ele->x, -ele->y, -ele->z);
				}
				ele++;
			}
			disableTexture (eScreen->cScreen, &eScreen->textu[j].tex);
	}
	glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glDisable (GL_BLEND);
	glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
}


static Bool
elementsPaintOutput (CompScreen              *s,
		 const ScreenPaintAttrib *sa,
		 const CompTransform	 *transform,
		 Region                  region,
		 CompOutput              *output,
		 unsigned int            mask)
{
	Bool status;
	Bool active = elementActive(s);

	E_SCREEN (s);

	UNWRAP (eScreen, s, paintOutput);
	status = (*s->paintOutput) (s, sa, transform, region, output, mask);
	WRAP (eScreen, s, paintOutput, elementsPaintOutput);

	if (active && elementsGetOverWindows (s->display))
	{
		CompTransform sTransform = *transform;
		transformToScreenSpace (s, output, -DEFAULT_Z_CAMERA, &sTransform);
		glPushMatrix ();
		glLoadMatrixf (sTransform.m);
		beginRendering (eScreen, s);
		glPopMatrix ();
	}

    return status;
}


static Bool
elementsDrawWindow (CompWindow           *w,
		const CompTransform  *transform,
		const FragmentAttrib *attrib,
		Region               region,
		unsigned int         mask)
{
	CompScreen *s = w->screen;
	Bool status;
	Bool active = elementActive(s);

	E_SCREEN (s);

	UNWRAP (eScreen, w->screen, drawWindow);
	status = (*w->screen->drawWindow) (w, transform, attrib, region, mask);
	WRAP (eScreen, w->screen, drawWindow, elementsDrawWindow);

	if (active && (w->type & CompWindowTypeDesktopMask) &&
		!elementsGetOverWindows (s->display))
	{
		beginRendering (eScreen, s);
	}
	return status;
}

static void
initiateElement (screen *eScreen,
		   element  *ele)
{
	int i, iii;

	if (ele->type == 4)
	{
		float temp = mmRand(elementsGetViscosity( eScreen->cScreen->display)/2.0,elementsGetViscosity( eScreen->cScreen->display),50.0);
		float xSway = 1.0 - temp*temp * 1.0 / 4.0;
	
		for (iii=0; iii<MAX_AUTUMN_AGE; iii++)
		{
			ele->autumnFloat[0][iii] = -xSway + (iii * ((2 * xSway)/(MAX_AUTUMN_AGE-1)));
		}

		ele->autumnAge[0] = getRand(0,MAX_AUTUMN_AGE-1);
		ele->autumnAge[1] = ele->autumnAge[0];
		ele->x  = mmRand (0, eScreen->cScreen->width, 1);
		ele->autumnChange = 1;
		ele->y = mmRand (eScreen->cScreen->height+100, eScreen->cScreen->height+ BIG_NUMBER,1);
		ele->dy[0] = mmRand (-2, -1, 5);
	}
	if (ele->type == 0)
	{
		float xSway = mmRand(elementsGetAutumnSway( eScreen->cScreen->display)/2,elementsGetAutumnSway( eScreen->cScreen->display),2.0);
		float ySway = elementsGetAutumnSpeed(eScreen->cScreen->display) / 20.0;

		for (iii=0; iii<MAX_AUTUMN_AGE; iii++)
		{
			ele->autumnFloat[0][iii] = -xSway + (iii * ((2 * xSway)/(MAX_AUTUMN_AGE-1)));
		}
		for (iii=0; iii<(MAX_AUTUMN_AGE / 2); iii++)
		{
			ele->autumnFloat[1][iii] = -ySway + (iii * ((2 * ySway)/(MAX_AUTUMN_AGE-1)));
		}
		for (; iii<MAX_AUTUMN_AGE; iii++)
		{
			ele->autumnFloat[1][iii] = ySway - (iii * ((2 * ySway)/(MAX_AUTUMN_AGE-1)));
		}
		ele->autumnAge[0] = getRand(0,MAX_AUTUMN_AGE-1);
		ele->autumnAge[1] = getRand(0,(MAX_AUTUMN_AGE/2)-1);
		ele->x  = mmRand (0, eScreen->cScreen->width, 1);
		ele->autumnChange = 1;
		ele->y  = -mmRand (100, BIG_NUMBER, 1);
		ele->dy[0] = mmRand (-2, -1, 5);
	}



	if (ele->type == 2)
	{
		int snowSway = elementsGetSnowSway (eScreen->cScreen->display);
		switch (elementsGetWindDirection (eScreen->cScreen->display))
		{
			case WindDirectionNone:
				ele->x  = mmRand (0, eScreen->cScreen->width, 1);
				ele->dx[0] = mmRand (-snowSway, snowSway, 1.0);
				ele->y  = mmRand (-BIG_NUMBER, -100, 1);
				ele->dy[0] = mmRand (1, 3, 1);
			break;
			case WindDirectionUp:
				ele->x  = mmRand (0, eScreen->cScreen->width, 1);
				ele->dx[0] = mmRand (-snowSway, snowSway, 1.0);
				ele->y  = mmRand (eScreen->cScreen->height + 100, eScreen->cScreen->height + BIG_NUMBER, 1);
				ele->dy[0] = -mmRand (1, 3, 1);
			break;
			case WindDirectionLeft:
				ele->x  = mmRand (eScreen->cScreen->width+100, eScreen->cScreen->width+BIG_NUMBER, 1);
				ele->dx[0] = -mmRand (1, 3, 1);
				ele->y  = mmRand (0, eScreen->cScreen->height, 1);
				ele->dy[0] = mmRand (-snowSway, snowSway, 1.5);
			break;
			case WindDirectionRight:
				ele->x  = mmRand (-BIG_NUMBER, -100, 1);
				ele->dx[0] = mmRand (1, 3, 1);
				ele->y  = mmRand (0, eScreen->cScreen->height, 1);
				ele->dy[0] = mmRand (-snowSway, snowSway, 1.5);
			break;
			default:
			break;
		}
	}

	ele->z  = mmRand (-elementsGetScreenDepth (eScreen->cScreen->display), 0.1, 5000);
	ele->dz[0] = mmRand (-500, 500, 500000);
	ele->rAngle = mmRand (-1000, 1000, 50);
	ele->rSpeed = mmRand (-2100, 2100, 700);

	if (ele->type == 1)
	{
		ele->x = mmRand(0, eScreen->cScreen->width, 1);
		ele->y = mmRand(0, eScreen->cScreen->height, 1);
		ele->lifespan = mmRand(50,1000, 100);
		ele->age = 0.0;
		for (i = 0; i < 4; i++) 
		{
			ele->dx[i] = mmRand(-3000, 3000, 200);
			ele->dy[i] = mmRand(-2000, 2000, 200);
			ele->dz[i] = mmRand(-1000, 1000, 500000);
		}
	}
	if (ele->type == 3)
	{
	   	float init;
		ele->dx[0] = mmRand(-50000, 50000, 5000);
		ele->dy[0] = mmRand(-50000, 50000, 5000);
		ele->dz[0] = mmRand(000, 200, 2000);
		ele->x = eScreen->cScreen->width * .5 + elementsGetStarOffsetX (eScreen->cScreen->display); // X Offset
		ele->y = eScreen->cScreen->height * .5 + elementsGetStarOffsetY (eScreen->cScreen->display); // Y Offset
		ele->z = mmRand(000, 0.1, 5000);
		init = mmRand(0,100, 1);
		ele->x += init * ele->dx[0];
		ele->y += init * ele->dy[0];
	}
}

static void
setElementTexture (screen *eScreen,
		     element  *ele)
{
	if (eScreen->numTexLoaded[0] && ele->type == 0)
		ele->eTex = &eScreen->textu[rand () % eScreen->numTexLoaded[0]];
	else if (eScreen->numTexLoaded[1] && ele->type == 1)
		ele->eTex = &eScreen->textu[eScreen->numTexLoaded[0] + (rand () % eScreen->numTexLoaded[1])];
	else if (eScreen->numTexLoaded[2] && ele->type == 2)
		ele->eTex = &eScreen->textu[eScreen->numTexLoaded[0] + eScreen->numTexLoaded[1] + (rand () % eScreen->numTexLoaded[2])];
	else if (eScreen->numTexLoaded[3] && ele->type == 3)
		ele->eTex = &eScreen->textu[eScreen->numTexLoaded[0] + eScreen->numTexLoaded[1] + eScreen->numTexLoaded[2] + (rand () % eScreen->numTexLoaded[3])];
	else if (eScreen->numTexLoaded[4] && ele->type == 4)
		ele->eTex = &eScreen->textu[eScreen->numTexLoaded[0] + eScreen->numTexLoaded[1] + eScreen->numTexLoaded[2] + eScreen->numTexLoaded[3] + (rand () % eScreen->numTexLoaded[4])];
	else
		ele->eTex = NULL;
	
}

static void
updateElementTextures (CompScreen *s, Bool changeTextures)
{
	int       i, count = 0;
	float     autumnSize = elementsGetLeafSize(s->display);
	float     ffSize = elementsGetFireflySize(s->display);
	float     snowSize = elementsGetSnowSize(s->display);
	float     starsSize = elementsGetStarsSize(s->display);
	float     bubblesSize = elementsGetBubblesSize(s->display);
	element *ele;

	E_SCREEN (s);
	E_DISPLAY (s->display);
	int numAutumn, numFf, numSnow, numStars, numBubbles;

	if (eScreen->isActive[0])
		numAutumn = elementsGetNumLeaves (s->display);
	else
		numAutumn = 0;
	if (eScreen->isActive[1])
		numFf = elementsGetNumFireflies (s->display);
	else
		numFf = 0;
	if (eScreen->isActive[2])
		numSnow = elementsGetNumSnowflakes (s->display);
	else
		numSnow = 0;
	if (eScreen->isActive[3])
		numStars = elementsGetNumStars (s->display);
	else
		numStars = 0;
	if (eScreen->isActive[4])
		numBubbles = elementsGetNumBubbles (s->display);
	else
		numBubbles = 0;
	ele = eScreen->allElements;
	if (changeTextures)
	{
	for (i = 0; i < eScreen->numElements; i++)
	{
		finiTexture (s, &eScreen->textu[i].tex);
		glDeleteLists (eScreen->textu[i].dList, 1);
	}

	if (eScreen->textu)
		free (eScreen->textu);
	eScreen->numElements = 0;
	eScreen->numTexLoaded[0] = 0;
	eScreen->numTexLoaded[1] = 0;
	eScreen->numTexLoaded[2] = 0;
	eScreen->numTexLoaded[3] = 0;
	eScreen->numTexLoaded[4] = 0;
	eScreen->textu = calloc (1, sizeof (texture) * (ed->numTex[0] + ed->numTex[1] + ed->numTex[2] + ed->numTex[3] + ed->numTex[4]));
	}
	for (i = 0; i < ed->numTex[0]; i++)
	{
		CompMatrix  *mat;
		texture *aTex;
	if (changeTextures)
	{
		eScreen->textu[count].loaded =
		    readImageToTexture (s, &eScreen->textu[count].tex,
					ed->texFiles[0][i].s,
					&eScreen->textu[count].width,
					&eScreen->textu[count].height);
		if (!eScreen->textu[count].loaded)
		{
		    compLogMessage ("Elements", CompLogLevelWarn,
			    "Texture (Autumn) not found : %s", ed->texFiles[0][i].s);
		    continue;
		}
		compLogMessage ("Elements", CompLogLevelInfo,
			"Loaded Texture (Autumn)%s", ed->texFiles[0][i].s);
		}
		mat = &eScreen->textu[count].tex.matrix;
		aTex = &eScreen->textu[count];
		aTex->dList = glGenLists (1);
		glNewList (aTex->dList, GL_COMPILE);

		glBegin (GL_QUADS);

		glTexCoord2f (COMP_TEX_COORD_X (mat, 0), COMP_TEX_COORD_Y (mat, 0));
		glVertex2f (0, 0);
		glTexCoord2f (COMP_TEX_COORD_X (mat, 0),
			      COMP_TEX_COORD_Y (mat, aTex->height));
		glVertex2f (0, autumnSize * aTex->height / aTex->width);
		glTexCoord2f (COMP_TEX_COORD_X (mat, aTex->width),
			      COMP_TEX_COORD_Y (mat, aTex->height));
		glVertex2f (autumnSize, autumnSize * aTex->height / aTex->width);
		glTexCoord2f (COMP_TEX_COORD_X (mat, aTex->width),
			      COMP_TEX_COORD_Y (mat, 0));
		glVertex2f (autumnSize, 0);

		glEnd ();
		glEndList ();

		count++;
	}
	if (changeTextures)
		eScreen->numTexLoaded[0] = count;
	for (i = 0; i < ed->numTex[1]; i++)
	{
		CompMatrix  *mat;
		texture *aTex;
		if (changeTextures)
		{
		eScreen->textu[count].loaded =
		readImageToTexture (s, &eScreen->textu[count].tex,
				ed->texFiles[1][i].s,
				&eScreen->textu[count].width,
				&eScreen->textu[count].height);
		if (!eScreen->textu[count].loaded)
		{
		    compLogMessage ("Elements", CompLogLevelWarn,
			    "Texture (Firefly) not found : %s", ed->texFiles[1][i].s);
		    continue;
		}
		compLogMessage ("Elements", CompLogLevelInfo,
			"Loaded Texture (Firefly) %s", ed->texFiles[1][i].s);
		}
		mat = &eScreen->textu[count].tex.matrix;
		aTex = &eScreen->textu[count];

		aTex->dList = glGenLists (1);
		glNewList (aTex->dList, GL_COMPILE);

		glBegin (GL_QUADS);

		glTexCoord2f (COMP_TEX_COORD_X (mat, 0), COMP_TEX_COORD_Y (mat, 0));
		glVertex2f (0, 0);
		glTexCoord2f (COMP_TEX_COORD_X (mat, 0),
		      COMP_TEX_COORD_Y (mat, aTex->height));
		glVertex2f (0, ffSize * aTex->height / aTex->width);
		glTexCoord2f (COMP_TEX_COORD_X (mat, aTex->width),
			      COMP_TEX_COORD_Y (mat, aTex->height));
		glVertex2f (ffSize, ffSize * aTex->height / aTex->width);
		glTexCoord2f (COMP_TEX_COORD_X (mat, aTex->width),
			      COMP_TEX_COORD_Y (mat, 0));
		glVertex2f (ffSize, 0);

		glEnd ();
		glEndList ();

		count++;
	}
	if (changeTextures)
	eScreen->numTexLoaded[1] = count - eScreen->numTexLoaded[0];
	for (i = 0; i < ed->numTex[2]; i++)
	{
		CompMatrix  *mat;
		texture *aTex;
	if (changeTextures)
	{
		eScreen->textu[count].loaded =
		readImageToTexture (s, &eScreen->textu[count].tex,
				ed->texFiles[2][i].s,
				&eScreen->textu[count].width,
				&eScreen->textu[count].height);
		if (!eScreen->textu[count].loaded)
		{
			compLogMessage ("Elements", CompLogLevelWarn,
			    "Texture (snow) not found : %s", ed->texFiles[2][i].s);
			continue;
		}
		compLogMessage ("Elements", CompLogLevelInfo,
			"Loaded Texture (snow) %s", ed->texFiles[2][i].s);
	}
		mat = &eScreen->textu[count].tex.matrix;
		aTex = &eScreen->textu[count];

		aTex->dList = glGenLists (1);
		glNewList (aTex->dList, GL_COMPILE);

		glBegin (GL_QUADS);

		glTexCoord2f (COMP_TEX_COORD_X (mat, 0), COMP_TEX_COORD_Y (mat, 0));
		glVertex2f (0, 0);
		glTexCoord2f (COMP_TEX_COORD_X (mat, 0),
		      COMP_TEX_COORD_Y (mat, aTex->height));
		glVertex2f (0, snowSize * aTex->height / aTex->width);
		glTexCoord2f (COMP_TEX_COORD_X (mat, aTex->width),
		      COMP_TEX_COORD_Y (mat, aTex->height));
		glVertex2f (snowSize, snowSize * aTex->height / aTex->width);
		glTexCoord2f (COMP_TEX_COORD_X (mat, aTex->width),
		      COMP_TEX_COORD_Y (mat, 0));
		glVertex2f (snowSize, 0);

		glEnd ();
		glEndList ();

		count++;
	}
	if (changeTextures)
	eScreen->numTexLoaded[2] = count - eScreen->numTexLoaded[0] -eScreen->numTexLoaded[1];
	for (i = 0; i < ed->numTex[3]; i++)
	{
		CompMatrix  *mat;
		texture *aTex;
	if (changeTextures)
	{
		eScreen->textu[count].loaded =
		readImageToTexture (s, &eScreen->textu[count].tex,
				ed->texFiles[3][i].s,
				&eScreen->textu[count].width,
				&eScreen->textu[count].height);
		if (!eScreen->textu[count].loaded)
		{
			compLogMessage ("Elements", CompLogLevelWarn,
			    "Texture (stars) not found : %s", ed->texFiles[3][i].s);
			continue;
		}
		compLogMessage ("Elements", CompLogLevelInfo,
			"Loaded Texture (stars)%s", ed->texFiles[3][i].s);
	}
		mat = &eScreen->textu[count].tex.matrix;
		aTex = &eScreen->textu[count];

		aTex->dList = glGenLists (1);
		glNewList (aTex->dList, GL_COMPILE);

		glBegin (GL_QUADS);

		glTexCoord2f (COMP_TEX_COORD_X (mat, 0), COMP_TEX_COORD_Y (mat, 0));
		glVertex2f (0, 0);
		glTexCoord2f (COMP_TEX_COORD_X (mat, 0),
		      COMP_TEX_COORD_Y (mat, aTex->height));
		glVertex2f (0, starsSize * aTex->height / aTex->width);
		glTexCoord2f (COMP_TEX_COORD_X (mat, aTex->width),
		      COMP_TEX_COORD_Y (mat, aTex->height));
		glVertex2f (starsSize, starsSize * aTex->height / aTex->width);
		glTexCoord2f (COMP_TEX_COORD_X (mat, aTex->width),
		      COMP_TEX_COORD_Y (mat, 0));
		glVertex2f (starsSize, 0);

		glEnd ();
		glEndList ();

		count++;
	}
	if (changeTextures)
	eScreen->numTexLoaded[3] = count - eScreen->numTexLoaded[0] - eScreen->numTexLoaded[1] - eScreen->numTexLoaded[2];
	for (i = 0; i < ed->numTex[4]; i++)
	{
		CompMatrix  *mat;
		texture *aTex;
	if (changeTextures)
	{
		eScreen->textu[count].loaded =
		readImageToTexture (s, &eScreen->textu[count].tex,
				ed->texFiles[4][i].s,
				&eScreen->textu[count].width,
				&eScreen->textu[count].height);
		if (!eScreen->textu[count].loaded)
		{
			compLogMessage ("Elements", CompLogLevelWarn,
			    "Texture (bubbles) not found : %s", ed->texFiles[4][i].s);
			continue;
		}
		compLogMessage ("Elements", CompLogLevelInfo,
			"Loaded Texture (bubbles)%s", ed->texFiles[4][i].s);
	}
		mat = &eScreen->textu[count].tex.matrix;
		aTex = &eScreen->textu[count];

		aTex->dList = glGenLists (1);
		glNewList (aTex->dList, GL_COMPILE);

		glBegin (GL_QUADS);

		glTexCoord2f (COMP_TEX_COORD_X (mat, 0), COMP_TEX_COORD_Y (mat, 0));
		glVertex2f (0, 0);
		glTexCoord2f (COMP_TEX_COORD_X (mat, 0),
		      COMP_TEX_COORD_Y (mat, aTex->height));
		glVertex2f (0, bubblesSize * aTex->height / aTex->width);
		glTexCoord2f (COMP_TEX_COORD_X (mat, aTex->width),
		      COMP_TEX_COORD_Y (mat, aTex->height));
		glVertex2f (bubblesSize, bubblesSize * aTex->height / aTex->width);
		glTexCoord2f (COMP_TEX_COORD_X (mat, aTex->width),
		      COMP_TEX_COORD_Y (mat, 0));
		glVertex2f (bubblesSize, 0);

		glEnd ();
		glEndList ();

		count++;
	}
	if (changeTextures)
	{
	eScreen->numTexLoaded[4] = count - eScreen->numTexLoaded[0] - eScreen->numTexLoaded[1] - eScreen->numTexLoaded[2] - eScreen->numTexLoaded[3];

//	if (count < (ed->numTex[0] + ed->numTex[1] + ed->numTex[2] + ed->numTex[3] + ed->numTex[4]))
		eScreen->textu = realloc (eScreen->textu, sizeof (texture) * count);

	eScreen->numElements = count;

	for (i = 0; i < (numAutumn + numFf + numSnow + numStars + numBubbles); i++)
		setElementTexture (eScreen, ele++);
	}
}

static Bool
elementsInitScreen (CompPlugin *p,
		CompScreen *s)
{
	screen *eScreen;
	E_DISPLAY (s->display);

	eScreen = calloc (1, sizeof(screen));
	s->base.privates[ed->privateIndex].ptr = eScreen;
	eScreen->cScreen = s;
	eScreen->numElements = 0;
	eScreen->textu = NULL;
	eScreen->needUpdate = FALSE;
	eScreen->useKeys = elementsGetToggle (s->display);

	if (!eScreen->useKeys)
	{
	eScreen->isActive[0] = elementsGetToggleAutumnCheck (s->display);
	eScreen->isActive[1] = elementsGetToggleFirefliesCheck (s->display);
	eScreen->isActive[2] = elementsGetToggleSnowCheck (s->display);
	eScreen->isActive[3] = elementsGetToggleStarsCheck (s->display);
	eScreen->isActive[4] = elementsGetToggleBubblesCheck (s->display);
	}
	else
	{
	eScreen->isActive[0] = FALSE;
	eScreen->isActive[1] = FALSE;
	eScreen->isActive[2] = FALSE;
	eScreen->isActive[3] = FALSE;
	eScreen->isActive[4] = FALSE;
	}

	createAll( s->display);
	updateElementTextures (s, TRUE);
	setupDisplayList (eScreen);
	WRAP (eScreen, s, paintOutput, elementsPaintOutput);
	WRAP (eScreen, s, drawWindow, elementsDrawWindow);
	eScreen->timeoutHandle = compAddTimeout (elementsGetUpdateDelay (s->display),
							elementsGetUpdateDelay (s->display),
							stepPositions, s);

	return TRUE;
}

static void
elementsFiniScreen (CompPlugin *p,
		CompScreen *s)
{
	int i;

	E_SCREEN (s);

	if (eScreen->timeoutHandle)
		compRemoveTimeout (eScreen->timeoutHandle);

	for (i = 0; i < eScreen->numElements; i++)
	{
		finiTexture (s, &eScreen->textu[i].tex);
		glDeleteLists (eScreen->textu[i].dList, 1);
	}

	if (eScreen->textu)
		free (eScreen->textu);

	UNWRAP (eScreen, s, paintOutput);
	UNWRAP (eScreen, s, drawWindow);
	free (eScreen);
}

static void
createAll( CompDisplay *d )
{
	CompScreen *s;
	int  i, ii, iii, iv ,v, numAutumn, numFf, numSnow, numStars, numBubbles, numTmp;
	element  *ele;

	for (s = d->screens; s; s = s->next)
	{
		E_SCREEN (s);
		if (eScreen->isActive[0])
			numAutumn = elementsGetNumLeaves (s->display);
		else
			numAutumn = 0;
		if (eScreen->isActive[1])
			numFf = elementsGetNumFireflies (s->display);
		else
			numFf = 0;
		if (eScreen->isActive[2])
			numSnow = elementsGetNumSnowflakes (s->display);
		else
			numSnow = 0;
		if (eScreen->isActive[3])
			numStars = elementsGetNumStars (s->display);
		else
			numStars = 0;
		if (eScreen->isActive[4])
			numBubbles = elementsGetNumBubbles (s->display);
		else
			numBubbles = 0;

		numTmp = (numAutumn + numFf + numSnow + numStars + numBubbles);
		eScreen->allElements = ele = realloc (eScreen->allElements,
				 numTmp * sizeof (element));
		ele = eScreen->allElements;

		for (i = 0; i < numAutumn; i++)
		{
			ele->type = 0;
			initiateElement (eScreen, ele);
				setElementTexture (eScreen, ele);
			ele++;
		}
		for (ii = 0; ii < numFf; ii++)
		{
			ele->type = 1;
			initiateElement (eScreen, ele);
			setElementTexture (eScreen, ele);
			ele++;
		}
		for (iii = 0; iii < numSnow; iii++)
		{
			ele->type = 2;
			initiateElement (eScreen, ele);
			setElementTexture (eScreen, ele);
			ele++;
		}
		for (iv = 0; iv < numStars; iv++)
		{
			ele->type = 3;
			initiateElement (eScreen, ele);
			setElementTexture (eScreen, ele);
			ele++;
		}
		for (v = 0; v < numBubbles; v++)
		{
			ele->type = 4;
			initiateElement (eScreen, ele);
			setElementTexture (eScreen, ele);
			ele++;
		}
	}
}

static void
elementsDisplayOptionChanged (CompDisplay        *d,
			  CompOption         *opt,
			  ElementsDisplayOptions num)
{
	E_DISPLAY (d);
	switch (num)
	{
		case ElementsDisplayOptionToggleAutumnCheck:
		{
			Bool useKeys = FALSE;
			CompScreen *s;
			for (s = d->screens; s; s = s->next)
			{
				E_SCREEN (s);
				useKeys = eScreen->useKeys;
				if (!useKeys)
				{
					eScreen->isActive[0] = elementsGetToggleAutumnCheck(s->display);
					damageScreen (s);
					eScreen->needUpdate = TRUE;
				}
			}
			if (!useKeys)
				createAll( d );
		}
		break;
		case ElementsDisplayOptionToggleFirefliesCheck:
		{
			Bool useKeys = FALSE;
			CompScreen *s;
			for (s = d->screens; s; s = s->next)
			{
				E_SCREEN (s);
				useKeys = eScreen->useKeys;
				if (!useKeys)
				{
					eScreen->isActive[1] = elementsGetToggleFirefliesCheck(s->display);
					damageScreen (s);
					eScreen->needUpdate = TRUE;
				}
			}
			if (!useKeys)
				createAll( d );
		}
		break;
		case ElementsDisplayOptionToggleSnowCheck:
		{
			Bool useKeys = FALSE;
			CompScreen *s;
			for (s = d->screens; s; s = s->next)
			{
				E_SCREEN (s);
				useKeys = eScreen->useKeys;
				if (!useKeys)
				{
					eScreen->isActive[2] = elementsGetToggleSnowCheck(s->display);
					damageScreen (s);
					eScreen->needUpdate = TRUE;
				}
			}
			if (!useKeys)
			createAll( d );
		}
		break;
		case ElementsDisplayOptionToggleStarsCheck:
		{
			Bool useKeys = FALSE;
			CompScreen *s;
			for (s = d->screens; s; s = s->next)
			{
				E_SCREEN (s);
				useKeys = eScreen->useKeys;
				if (!useKeys)
				{
					eScreen->isActive[3] = elementsGetToggleStarsCheck(s->display);
					damageScreen (s);
					eScreen->needUpdate = TRUE;
				}
			}
			if (!useKeys)
				createAll( d );
		}
		break;
		case ElementsDisplayOptionToggleBubblesCheck:
		{
			Bool useKeys = FALSE;
			CompScreen *s;
			for (s = d->screens; s; s = s->next)
			{
				E_SCREEN (s);
				useKeys = eScreen->useKeys;
				if (!useKeys)
				{
					eScreen->isActive[4] = elementsGetToggleBubblesCheck(s->display);
					damageScreen (s);
					eScreen->needUpdate = TRUE;
				}
			}
			if (!useKeys)
			createAll( d );
		}
		break;
		case ElementsDisplayOptionToggle:
		{
			CompScreen *s;

			for (s = d->screens; s; s = s->next)
			{
				E_SCREEN (s);
				eScreen->useKeys = elementsGetToggle(d);
				if (!eScreen->useKeys)
				{
					eScreen->isActive[0] = elementsGetToggleAutumnCheck(s->display);
					eScreen->isActive[1] = elementsGetToggleFirefliesCheck(s->display);
					eScreen->isActive[2] = elementsGetToggleSnowCheck(s->display);
					eScreen->isActive[3] = elementsGetToggleStarsCheck(s->display);
					eScreen->isActive[4] = elementsGetToggleBubblesCheck(s->display);
					createAll( d);
				}
				damageScreen (s);
			}
		}
		break;
		case ElementsDisplayOptionLeafSize:
		{
			CompScreen *s;

			for (s = d->screens; s; s = s->next)
			{
				E_SCREEN (s);
				eScreen->needUpdate = TRUE;
				updateElementTextures (s, FALSE);
			}
		}
		break;
		case ElementsDisplayOptionBubblesSize:
		{
			CompScreen *s;

			for (s = d->screens; s; s = s->next)
			{
				E_SCREEN (s);
				eScreen->needUpdate = TRUE;
				updateElementTextures (s, FALSE);
			}
		}

		break;
		case ElementsDisplayOptionSnowSize:
		{
			CompScreen *s;

			for (s = d->screens; s; s = s->next)
			{
				E_SCREEN (s);
				eScreen->needUpdate = TRUE;
				updateElementTextures (s, FALSE);
			}
		}
		break;
		case ElementsDisplayOptionSnowSway:
		{
			createAll( d );
		}
		break;
		case ElementsDisplayOptionStarsSize:
		{
			CompScreen *s;

			for (s = d->screens; s; s = s->next)
			{
				E_SCREEN (s);
				eScreen->needUpdate = TRUE;
				updateElementTextures (s, FALSE);
			}
		}
		break;
		case ElementsDisplayOptionFireflySize:
		{
			CompScreen *s;
			for (s = d->screens; s; s = s->next)
			{
				E_SCREEN (s);
				eScreen->needUpdate = TRUE;
				updateElementTextures (s, FALSE);
			}
		}
		break;
		case ElementsDisplayOptionUpdateDelay:
		{
			CompScreen *s;

			for (s = d->screens; s; s = s->next)
			{
				E_SCREEN (s);

				if (eScreen->timeoutHandle)
					compRemoveTimeout (eScreen->timeoutHandle);
					eScreen->timeoutHandle =
						compAddTimeout (elementsGetUpdateDelay (d),
								elementsGetUpdateDelay (d),
								stepPositions, s);
			}
		}
		break;
		case ElementsDisplayOptionNumLeaves:
		{
			createAll( d );
		}
		break;
		case ElementsDisplayOptionNumBubbles:
		{
			createAll( d );
		}
		break;

		case ElementsDisplayOptionAutumnSway:
		{
			createAll( d );
		}
		break;
		case ElementsDisplayOptionNumFireflies:
		{
			createAll( d );
		}
		break;
		case ElementsDisplayOptionNumSnowflakes:
		{
			createAll( d );
		}
		break;
		case ElementsDisplayOptionNumStars:
		{
			createAll( d );
		}
		break;
		case ElementsDisplayOptionLeafTextures:
		{
			CompScreen *s;
			CompOption *texAutOpt;
			texAutOpt = elementsGetLeafTexturesOption (d);
			ed->texFiles[0] = texAutOpt->value.list.value;
			ed->numTex[0] = texAutOpt->value.list.nValue;
			for (s = d->screens; s; s = s->next)
				updateElementTextures (s, TRUE);
		}
		break;
		case ElementsDisplayOptionBubblesTextures:
		{
			CompScreen *s;
			CompOption *texBubblesOpt;
			texBubblesOpt = elementsGetBubblesTexturesOption (d);
			ed->texFiles[4] = texBubblesOpt->value.list.value;
			ed->numTex[4] = texBubblesOpt->value.list.nValue;
			for (s = d->screens; s; s = s->next)
				updateElementTextures (s, TRUE);
		}
		break;
		case ElementsDisplayOptionFirefliesTextures:
		{
			CompScreen *s;
			CompOption *texFfOpt;

			texFfOpt = elementsGetFirefliesTexturesOption (d);

			ed->texFiles[1] = texFfOpt->value.list.value;
			ed->numTex[1] = texFfOpt->value.list.nValue;

			for (s = d->screens; s; s = s->next)
				updateElementTextures (s, TRUE);
		}
		break;
		case ElementsDisplayOptionSnowTextures:
		{
			CompScreen *s;
			CompOption *texSnowOpt;

			texSnowOpt = elementsGetSnowTexturesOption (d);

			ed->texFiles[2] = texSnowOpt->value.list.value;
			ed->numTex[2] = texSnowOpt->value.list.nValue;

			for (s = d->screens; s; s = s->next)
				updateElementTextures (s, TRUE);
		}
		break;
		case ElementsDisplayOptionStarsTextures:
		{
			CompScreen *s;
			CompOption *texStarsOpt;

			texStarsOpt = elementsGetStarsTexturesOption (d);
			ed->texFiles[3] = texStarsOpt->value.list.value;
			ed->numTex[3] = texStarsOpt->value.list.nValue;

			for (s = d->screens; s; s = s->next)
				updateElementTextures (s, TRUE);

		}
		break;
		default:
		break;
	}
}

static Bool
elementsInitDisplay (CompPlugin  *p,
		 CompDisplay *d)
{
	CompOption  *texAutOpt, *texFfOpt, *texSnowOpt, *texStarsOpt;
	CompOption  *texBubblesOpt;
	eDisplay *ed;
        if (!checkPluginABI ("core", CORE_ABIVERSION))
	return FALSE;    

	ed = malloc (sizeof (eDisplay));

	ed->privateIndex = allocateScreenPrivateIndex (d);
	if (ed->privateIndex < 0)
	{
		free (ed);
		return FALSE;
	}
	elementsSetToggleAutumnKeyInitiate (d, elementsAutumnToggle);
	elementsSetToggleFirefliesKeyInitiate (d, elementsFirefliesToggle);
	elementsSetToggleSnowKeyInitiate (d, elementsSnowToggle);
	elementsSetToggleStarsKeyInitiate (d, elementsStarsToggle);
	elementsSetToggleBubblesKeyInitiate (d, elementsBubblesToggle);
	elementsSetToggleNotify (d, elementsDisplayOptionChanged);
	elementsSetToggleSnowCheckNotify (d, elementsDisplayOptionChanged);
	elementsSetToggleAutumnCheckNotify (d, elementsDisplayOptionChanged);
	elementsSetToggleStarsCheckNotify (d, elementsDisplayOptionChanged);
	elementsSetToggleFirefliesCheckNotify (d, elementsDisplayOptionChanged);
	elementsSetToggleBubblesCheckNotify (d, elementsDisplayOptionChanged);
	elementsSetNumSnowflakesNotify (d, elementsDisplayOptionChanged);
	elementsSetNumBubblesNotify (d, elementsDisplayOptionChanged);
	elementsSetAutumnSwayNotify (d, elementsDisplayOptionChanged);
	elementsSetNumLeavesNotify (d, elementsDisplayOptionChanged);
	elementsSetNumFirefliesNotify (d, elementsDisplayOptionChanged);
	elementsSetNumStarsNotify (d, elementsDisplayOptionChanged);
	elementsSetLeafSizeNotify (d, elementsDisplayOptionChanged);
	elementsSetBubblesSizeNotify (d, elementsDisplayOptionChanged);
	elementsSetFireflySizeNotify (d, elementsDisplayOptionChanged);
	elementsSetSnowSizeNotify (d, elementsDisplayOptionChanged);
	elementsSetSnowSwayNotify (d, elementsDisplayOptionChanged);
	elementsSetStarsSizeNotify (d, elementsDisplayOptionChanged);
	elementsSetUpdateDelayNotify (d, elementsDisplayOptionChanged);
	elementsSetLeafTexturesNotify (d, elementsDisplayOptionChanged);
	elementsSetFirefliesTexturesNotify (d, elementsDisplayOptionChanged);
	elementsSetSnowTexturesNotify (d, elementsDisplayOptionChanged);
	elementsSetStarsTexturesNotify (d, elementsDisplayOptionChanged);
	elementsSetBubblesTexturesNotify (d, elementsDisplayOptionChanged);

	texAutOpt = elementsGetLeafTexturesOption (d);
	texFfOpt = elementsGetFirefliesTexturesOption (d);
	texSnowOpt = elementsGetSnowTexturesOption (d);
	texStarsOpt = elementsGetStarsTexturesOption (d);
	texBubblesOpt = elementsGetBubblesTexturesOption (d);
	ed->texFiles[0] = texAutOpt->value.list.value;
	ed->texFiles[1] = texFfOpt->value.list.value;
	ed->texFiles[2] = texSnowOpt->value.list.value;
	ed->texFiles[3] = texStarsOpt->value.list.value;
	ed->texFiles[4] = texBubblesOpt->value.list.value;
	ed->numTex[0] = texAutOpt->value.list.nValue;
	ed->numTex[1] = texFfOpt->value.list.nValue;
	ed->numTex[2] = texSnowOpt->value.list.nValue;
	ed->numTex[3] = texStarsOpt->value.list.nValue;
	ed->numTex[4] = texBubblesOpt->value.list.nValue;

	d->base.privates[displayPrivateIndex].ptr = ed;
	return TRUE;
}


static void
elementsFiniDisplay (CompPlugin  *p,
		 CompDisplay *d)
{
	E_DISPLAY (d);

	freeScreenPrivateIndex (d, ed->privateIndex);
	free (ed);
}

static Bool
elementsInit (CompPlugin *p)
{
	displayPrivateIndex = allocateDisplayPrivateIndex ();
	if (displayPrivateIndex < 0)
		return FALSE;

	return TRUE;
}

static void
elementsFini (CompPlugin *p)
{
	freeDisplayPrivateIndex (displayPrivateIndex);
}


static CompBool elementsInitObject(CompPlugin *p, CompObject *o)
{
	static InitPluginObjectProc dispTab[] = {
		(InitPluginObjectProc) 0, // InitCore
		(InitPluginObjectProc) elementsInitDisplay,
		(InitPluginObjectProc) elementsInitScreen
	};

	RETURN_DISPATCH(o, dispTab, ARRAY_SIZE(dispTab), TRUE, (p, o));
}

static void elementsFiniObject(CompPlugin *p, CompObject *o)
{
	static FiniPluginObjectProc dispTab[] = {
		(FiniPluginObjectProc) 0, // FiniCore
		(FiniPluginObjectProc) elementsFiniDisplay,
		(FiniPluginObjectProc) elementsFiniScreen
	};

	DISPATCH(o, dispTab, ARRAY_SIZE(dispTab), (p, o));
}

CompPluginVTable elementsVTable = {
	"elements",
	0,
	elementsInit,
	elementsFini,
	elementsInitObject,
	elementsFiniObject,
	0,
	0
};

CompPluginVTable*
getCompPluginInfo (void)
{
    return &elementsVTable;
}

// Software is like sex. Sure, you can pay for it, but why would you?! //
