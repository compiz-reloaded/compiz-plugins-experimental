#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>

#ifdef USE_LIBRSVG
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>
#include <librsvg/rsvg.h>
#include <librsvg/rsvg-cairo.h>
#endif

#include <compiz-core.h>
#include <compiz-animation.h>
#include <compiz-animationaddon.h>

extern int animDisplayPrivateIndex;
extern CompMetadata animMetadata;

extern AnimEffect AnimEffectBlinds;
extern AnimEffect AnimEffectBonanza;
extern AnimEffect AnimEffectHelix;
extern AnimEffect AnimEffectShatter;

#define NUM_EFFECTS 4

typedef enum
{
    // Effect settings
    ANIMPLUS_SCREEN_OPTION_BLINDS_HALFTWISTS,
    ANIMPLUS_SCREEN_OPTION_BLINDS_GRIDX,
    ANIMPLUS_SCREEN_OPTION_BLINDS_THICKNESS,
    ANIMPLUS_SCREEN_OPTION_BONANZA_PARTICLES,
    ANIMPLUS_SCREEN_OPTION_BONANZA_SIZE,
    ANIMPLUS_SCREEN_OPTION_BONANZA_LIFE,
    ANIMPLUS_SCREEN_OPTION_BONANZA_COLOR,
    ANIMPLUS_SCREEN_OPTION_BONANZA_MYSTICAL,
    ANIMPLUS_SCREEN_OPTION_HELIX_NUM_TWISTS,
    ANIMPLUS_SCREEN_OPTION_HELIX_GRIDSIZE_Y,
    ANIMPLUS_SCREEN_OPTION_HELIX_THICKNESS,
    ANIMPLUS_SCREEN_OPTION_HELIX_DIRECTION,
    ANIMPLUS_SCREEN_OPTION_HELIX_SPIN_DIRECTION,
    ANIMPLUS_SCREEN_OPTION_SHATTER_NUM_SPOKES,
    ANIMPLUS_SCREEN_OPTION_SHATTER_NUM_TIERS,

    ANIMPLUS_SCREEN_OPTION_NUM
} AnimPlusScreenOptions;

// This must have the value of the first "effect setting" above
// in AnimEgScreenOptions
#define NUM_NONEFFECT_OPTIONS 0

typedef enum _AnimPlusDisplayOptions
{
    ANIMPLUS_DISPLAY_OPTION_ABI = 0,
    ANIMPLUS_DISPLAY_OPTION_INDEX,
    ANIMPLUS_DISPLAY_OPTION_NUM
} AnimPlusDisplayOptions;

typedef struct _AnimPlusDisplay
{
    int screenPrivateIndex;
    AnimBaseFunctions *animBaseFunc;
    AnimAddonFunctions *animAddonFunc;

    CompOption opt[ANIMPLUS_DISPLAY_OPTION_NUM];
} AnimPlusDisplay;

typedef struct _AnimPlusScreen
{
    int windowPrivateIndex;

    CompOutput *output;

    CompOption opt[ANIMPLUS_SCREEN_OPTION_NUM];
} AnimPlusScreen;

typedef struct _AnimPlusWindow
{
    AnimWindowCommon *com;
    AnimWindowEngineData *eng;

    int animFireDirection;

} AnimPlusWindow;

#define GET_ANIMPLUS_DISPLAY(d)						\
    ((AnimPlusDisplay *) (d)->base.privates[animDisplayPrivateIndex].ptr)

#define ANIMPLUS_DISPLAY(d)				\
    AnimPlusDisplay *ad = GET_ANIMPLUS_DISPLAY (d)

#define GET_ANIMPLUS_SCREEN(s, ad)						\
    ((AnimPlusScreen *) (s)->base.privates[(ad)->screenPrivateIndex].ptr)

#define ANIMPLUS_SCREEN(s)							\
    AnimPlusScreen *as = GET_ANIMPLUS_SCREEN (s, GET_ANIMPLUS_DISPLAY (s->display))

#define GET_ANIMPLUS_WINDOW(w, as)						\
    ((AnimPlusWindow *) (w)->base.privates[(as)->windowPrivateIndex].ptr)

#define ANIMPLUS_WINDOW(w)					     \
    AnimPlusWindow *aw = GET_ANIMPLUS_WINDOW (w,                     \
		     GET_ANIMPLUS_SCREEN (w->screen,             \
		     GET_ANIMPLUS_DISPLAY (w->screen->display)))

// ratio of perceived length of animation compared to real duration
// to make it appear to have the same speed with other animation effects

#define EXPLODE_PERCEIVED_T 0.7f

/*
 * Function prototypes
 *
 */

OPTION_GETTERS_HDR

/* blinds.c */

Bool
fxBlindsInit( CompWindow *w );

/* helix.c */

Bool
fxHelixInit( CompWindow *w );

/* bonanza.c */

Bool
fxBonanzaInit (CompWindow * w);

void
fxBonanzaAnimStep (CompWindow *w, float time);


/* shatter.c */
Bool
fxShatterInit (CompWindow *w );
