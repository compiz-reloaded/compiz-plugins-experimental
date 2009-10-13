/* Insert copyright */
#ifndef _COMPIZ_ELEMENTS_H
#define _COMPIZ_ELEMENTS_H

#include <compiz-core.h>

/* This is the number used to make sure that elements don't get all
   created in one place. Bigger number, more distance.
   Possibly fixable later. -> s->width? */
#define BIG_NUMBER		20000

#define ELEMENTS_ABIVERSION     20081215

typedef struct _ElementTypeInfo  ElementTypeInfo;
typedef struct _Element          Element;
typedef struct _ElementAnimation ElementAnimation;
typedef struct _ElementTexture   ElementTexture;

typedef void (*ElementInitiateProc) (CompScreen *s,
				     Element    *e);

typedef void (*ElementMoveProc) (CompScreen       *s,
				 ElementAnimation *anim,
				 Element          *e,
				 int              updateDelay);

typedef void (*ElementFiniProc) (CompScreen *s,
				 Element    *e);

/* Element Type management, extension plugins should call this to create
   a new 'type' of element in the core of elements on initDisplay.
   What is provided:
   * CompDisplay *
   * Element Name
   * Function pointer to element initialization function
   * Function pointer to element movement function
   * Fuction  pointer to fini function
 */

typedef struct _ElementsBaseFunctions
{
    Bool (*newElementType) (CompDisplay         *d,
			    char                *name,
			    char                *desc,
			    ElementInitiateProc initiate,
			    ElementMoveProc	move,
			    ElementFiniProc     fini);

    void (*removeElementType) (CompScreen *s,
			       char       *name);

    int (*getRand) (int min, int max);
    float (*mmRand) (int min, int max, float divisor);

    int (*boxing) (CompScreen *s);
    int (*depth) (CompScreen *s);
} ElementsBaseFunctions;

struct _ElementTypeInfo
{
    char *name;
    char *desc;

    ElementInitiateProc initiate;
    ElementMoveProc     move;
    ElementFiniProc     fini;

    struct _ElementTypeInfo *next;
};

struct _ElementTexture
{
    CompTexture tex;

    unsigned int width;
    unsigned int height;

    Bool   loaded;
    GLuint dList;		
};

struct _Element
{
    float x, y, z;
    float dx, dy, dz;
    float rSpeed;
    int   rDirection;
    int   rAngle;

    float opacity;
    float glowAlpha;	/* Needs to be painted */
    int   nTexture;

    void *ptr;		/* Extensions latch on to this */
};

struct _ElementAnimation
{
    char *type;
    char *desc;

    int nElement;
    int size;
    int speed;
    int id;		/* Uniquely idenitifies this animation */

    Bool active;

    ElementTexture *texture;
    int            nTextures;

    Element         *elements;
    ElementTypeInfo *properties;

    struct _ElementAnimation *next;
};	

/* Resets an animation to defaults */
void initiateElement (CompScreen       *s,
		      ElementAnimation *anim,
		      Element          *ele);

/* Calls the movement function pointer for an element in an element animation */
void elementMove (CompScreen       *s,
		  Element          *e,
		  ElementAnimation *anim,
		  int              updateDelay);

/* Used to update all element textures and settings */
void updateElementTextures (CompScreen *s,
			    Bool       changeTextures);

/* Utility Functions */
int elementsGetRand (int min, int max);
float elementsMmRand(int  min, int max, float divisor);
int elementsGetEBoxing (CompScreen *s);
int elementsGetEDepth (CompScreen *s);

/* Management of ElementAnimations */
ElementAnimation* elementsCreateAnimation (CompScreen *s,
					   char       *name);
void elementsDeleteAnimation (CompScreen       *s,
			      ElementAnimation *anim);

#endif
