#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <compiz-core.h>
#include <compiz-animation.h>

extern int animDisplayPrivateIndex;
extern CompMetadata animMetadata;

extern AnimEffect AnimEffectFlyIn;
extern AnimEffect AnimEffectBounce;
extern AnimEffect AnimEffectRotateIn;
extern AnimEffect AnimEffectSheets;
extern AnimEffect AnimEffectExpand;
extern AnimEffect AnimEffectExpandPW;

#define NUM_EFFECTS 6

typedef enum
{
    // Effect settings
    ANIMSIM_SCREEN_OPTION_BOUNCE_MAX_SIZE,
    ANIMSIM_SCREEN_OPTION_BOUNCE_MIN_SIZE,
    ANIMSIM_SCREEN_OPTION_BOUNCE_NUMBER,
    ANIMSIM_SCREEN_OPTION_BOUNCE_FADE,
    ANIMSIM_SCREEN_OPTION_FLYIN_DIRECTION,
    ANIMSIM_SCREEN_OPTION_FLYIN_DIRECTION_X,
    ANIMSIM_SCREEN_OPTION_FLYIN_DIRECTION_Y,
    ANIMSIM_SCREEN_OPTION_FLYIN_FADE,
    ANIMSIM_SCREEN_OPTION_FLYIN_DISTANCE,
    ANIMSIM_SCREEN_OPTION_ROTATEIN_ANGLE,
    ANIMSIM_SCREEN_OPTION_ROTATEIN_DIRECTION,
    ANIMSIM_SCREEN_OPTION_SHEET_START_PERCENT,
    ANIMSIM_SCREEN_OPTION_EXPANDPW_HORIZ_FIRST,
    ANIMSIM_SCREEN_OPTION_EXPANDPW_INITIAL_HORIZ,
    ANIMSIM_SCREEN_OPTION_EXPANDPW_INITIAL_VERT,
    ANIMSIM_SCREEN_OPTION_EXPANDPW_DELAY,

    ANIMSIM_SCREEN_OPTION_NUM
} AnimSimScreenOptions;

// This must have the value of the first "effect setting" above
// in AnimSimScreenOptions
#define NUM_NONEFFECT_OPTIONS 0

typedef enum _AnimSimDisplayOptions
{
    ANIMSIM_DISPLAY_OPTION_ABI = 0,
    ANIMSIM_DISPLAY_OPTION_INDEX,
    ANIMSIM_DISPLAY_OPTION_NUM
} AnimSimDisplayOptions;

typedef struct _WaveParam
{
    float halfWidth;
    float amp;
    float pos;
} WaveParam;

typedef struct _AnimSimDisplay
{
    int screenPrivateIndex;
    AnimBaseFunctions *animBaseFunc;

    CompOption opt[ANIMSIM_DISPLAY_OPTION_NUM];
} AnimSimDisplay;

typedef struct _AnimSimScreen
{
    int windowPrivateIndex;

    CompOutput *output;

    CompOption opt[ANIMSIM_SCREEN_OPTION_NUM];
} AnimSimScreen;

typedef struct _AnimSimWindow
{
    AnimWindowCommon *com;
    /* bounce props */
    int bounceCount;
    int nBounce;
    float currBounceProgress;
    float targetScale;
    float currentScale;
    float lastProgressMax;
    Bool bounceNeg;
    /* rotatein props */
    int rotatinModAngle;
    int currentCull;
    /* sheets props */
    int sheetsWaveCount;
    WaveParam *sheetsWaves;

} AnimSimWindow;

#define GET_ANIMSIM_DISPLAY(d)						\
    ((AnimSimDisplay *) (d)->base.privates[animDisplayPrivateIndex].ptr)

#define ANIMSIM_DISPLAY(d)				\
    AnimSimDisplay *ad = GET_ANIMSIM_DISPLAY (d)

#define GET_ANIMSIM_SCREEN(s, ad)						\
    ((AnimSimScreen *) (s)->base.privates[(ad)->screenPrivateIndex].ptr)

#define ANIMSIM_SCREEN(s)							\
    AnimSimScreen *as = GET_ANIMSIM_SCREEN (s, GET_ANIMSIM_DISPLAY (s->display))

#define GET_ANIMSIM_WINDOW(w, as)						\
    ((AnimSimWindow *) (w)->base.privates[(as)->windowPrivateIndex].ptr)

#define ANIMSIM_WINDOW(w)					     \
    AnimSimWindow *aw = GET_ANIMSIM_WINDOW (w,                     \
		     GET_ANIMSIM_SCREEN (w->screen,             \
		     GET_ANIMSIM_DISPLAY (w->screen->display)))

// ratio of perceived length of animation compared to real duration
// to make it appear to have the same speed with other animation effects

#define EXPLODE_PERCEIVED_T 0.7f

/*
 * Function prototypes
 *
 */

OPTION_GETTERS_HDR

/* flyin.c */

Bool
fxFlyinInit (CompWindow *w);

void
fxFlyinUpdateWindowAttrib (CompWindow * w,
			   WindowPaintAttrib *wAttrib);

void
fxFlyinAnimStep (CompWindow *w,
		 float time);

float
fxFlyinAnimProgress (CompWindow *w);

void
fxFlyinUpdateWindowTransform (CompWindow *w,
			      CompTransform *wTransform);

/* rotatein.c */

Bool
fxRotateinInit (CompWindow *w);

void
fxRotateinUpdateWindowAttrib (CompWindow * w,
			   WindowPaintAttrib *wAttrib);

void
fxRotateinAnimStep (CompWindow *w,
		 float time);

float
fxRotateinAnimProgress (CompWindow *w);

void
fxRotateinUpdateWindowTransform (CompWindow *w,
			      CompTransform *wTransform);

void
fxRotateinPrePaintWindow (CompWindow * w);

void
fxRotateinPostPaintWindow (CompWindow * w);

Bool
fxRotateinZoomToIcon (CompWindow *w);

/* bounce.c */

Bool
fxBounceInit (CompWindow *w);

void
fxBounceUpdateWindowAttrib (CompWindow * w,
			   WindowPaintAttrib *wAttrib);

void
fxBounceAnimStep (CompWindow *w,
		 float time);

float
fxBounceAnimProgress (CompWindow *w);

void
fxBounceUpdateWindowTransform (CompWindow *w,
			      CompTransform *wTransform);

/* sheets.c */

void
fxSheetsInitGrid (CompWindow *w,
		     int *gridWidth,
		     int *gridHeight);

Bool
fxSheetsInit (CompWindow * w);

void
fxSheetsModelStep (CompWindow * w,
		      float time);

/* expand.c */

Bool
fxExpandInit (CompWindow *w);

void
fxExpandUpdateWindowAttrib (CompWindow * w,
			   WindowPaintAttrib *wAttrib);

void
fxExpandAnimStep (CompWindow *w,
		 float time);

float
fxExpandAnimProgress (CompWindow *w);

void
fxExpandUpdateWindowTransform (CompWindow *w,
			      CompTransform *wTransform);

/* expandPW.c */

Bool
fxExpandPWInit (CompWindow *w);

void
fxExpandPWUpdateWindowAttrib (CompWindow * w,
			   WindowPaintAttrib *wAttrib);

void
fxExpandPWAnimStep (CompWindow *w,
		 float time);

float
fxExpandPWAnimProgress (CompWindow *w);

void
fxExpandPWUpdateWindowTransform (CompWindow *w,
			      CompTransform *wTransform);
