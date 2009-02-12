#include "compiz-elements.h"
#include "elements_options.h"

#define GET_ELEMENTS_DISPLAY(d)                            \
	((ElementsDisplay *) (d)->base.privates[displayPrivateIndex].ptr)

#define ELEMENTS_DISPLAY(d)                                \
	ElementsDisplay *ed = GET_ELEMENTS_DISPLAY (d)

#define GET_ELEMENTS_SCREEN(s, ed)                         \
	((ElementsScreen *) (s)->base.privates[(ed)->privateIndex].ptr)

#define ELEMENTS_SCREEN(s)                                 \
	ElementsScreen *es = GET_ELEMENTS_SCREEN (s, GET_ELEMENTS_DISPLAY (s->display))

#define GLOW_STAGES		5
#define MAX_AUTUMN_AGE		100

extern int displayPrivateIndex;

typedef struct _ElementsDisplay
{
	int privateIndex;
	Bool textAvailable;
	ElementTypeInfo *elementTypes;
} ElementsDisplay;

typedef struct _ElementsScreen
{
	CompTimeoutHandle timeoutHandle;
	PaintOutputProc paintOutput;
	DrawWindowProc  drawWindow;
	/* text display support */
	CompTexture textTexture;
	Pixmap      textPixmap;
	int         textWidth;
	int         textHeight;
	Bool        renderText;
	/* Information texture rendering */
	ElementTexture *eTexture;
	int            ntTextures;
	int	       nTexture;
	Bool	    renderTexture;
	CompTimeoutHandle renderTimeout;
	CompTimeoutHandle switchTimeout;
	/* position in list */
	int listIter;
	/* animation number according to user set option */
	int animIter;
	GLuint displayList;
	Bool   needUpdate;
	ElementAnimation *animations;
} ElementsScreen;

/* autumn.c */
typedef struct _AutumnElement
{
	float autumnFloat[2][MAX_AUTUMN_AGE];
	int   autumnAge[2];
	int   autumnChange;
} AutumnElement;

void
initiateAutumnElement (CompScreen *s,
		       Element *e);
void
autumnMove (CompScreen *s,
	    ElementAnimation *anim,
	    Element *e,
	    int     updateDelay);

void
autumnFini (CompScreen *s,
	    Element    *e);

/* fireflies.c */
typedef struct _FireflyElement
{
	float lifespan;
	float age;
	float lifecycle;
	float dx[4], dy[4], dz[4];
} FireflyElement;

void
initiateFireflyElement (CompScreen *s,
		       Element *e);

void
fireflyMove (CompScreen *s,
	    ElementAnimation *anim,
	    Element *e,
	    int     updateDelay);

void
fireflyFini (CompScreen *s,
	     Element    *e);

/* snow.c */
void
initiateSnowElement (CompScreen *s,
		     Element *e);

void
snowMove   (CompScreen *s,
	    ElementAnimation *anim,
	    Element *e,
	    int     updateDelay);

void
snowFini (CompScreen *s,
	  Element    *e);

/* stars.c */
void
initiateStarElement (CompScreen *s,
		     Element *e);

void
starMove   (CompScreen *s,
	    ElementAnimation *anim,
	    Element *e,
	    int     updateDelay);

void
starFini (CompScreen *s,
	  Element    *e);

/* bubbles.c */
typedef struct _BubbleElement
{
	float bubbleFloat[2][MAX_AUTUMN_AGE];
	int   bubbleAge[2];
	int   bubbleChange;
} BubbleElement;

void
initiateBubbleElement (CompScreen *s,
		       Element *e);

void
bubbleMove (CompScreen *s,
	    ElementAnimation *anim,
	    Element *e,
	    int     updateDelay);

void
bubbleFini (CompScreen *s,
	    Element *e);
