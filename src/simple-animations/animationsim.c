/**
 * Example Animation extension plugin for compiz
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 **/

#include "animationsim.h"

int animDisplayPrivateIndex;
CompMetadata animMetadata;

AnimEffect animEffects[NUM_EFFECTS];

ExtensionPluginInfo animExtensionPluginInfo = {
    .nEffects		= NUM_EFFECTS,
    .effects		= animEffects,

    .nEffectOptions	= ANIMSIM_SCREEN_OPTION_NUM,
};

OPTION_GETTERS (GET_ANIMSIM_DISPLAY (w->screen->display)->animBaseFunc,
		&animExtensionPluginInfo, NUM_NONEFFECT_OPTIONS)

static Bool
animSetScreenOptions (CompPlugin *plugin,
		      CompScreen * screen,
		      const char *name,
		      CompOptionValue * value)
{
    CompOption *o;
    int index;

    ANIMSIM_SCREEN (screen);

    o = compFindOption (as->opt, NUM_OPTIONS (as), name, &index);
    if (!o)
	return FALSE;

    switch (index)
    {
    default:
	return compSetScreenOption (screen, o, value);
	break;
    }

    return FALSE;
}

static const CompMetadataOptionInfo animEgScreenOptionInfo[] = {
    { "bounce_max_size", "float", "<min>1.0</min>", 0, 0 },
    { "bounce_min_size", "float", "<max>1.0</max>", 0, 0 },
    { "bounce_number", "int", 0, 0, 0			},
    { "bounce_fade", "bool", 0, 0, 0			},
    { "flyin_direction", "int", 0, 0, 0			},
    { "flyin_direction_x", "float", 0, 0, 0		},
    { "flyin_direction_y", "float", 0, 0, 0		},
    { "flyin_fade", "bool", 0, 0, 0			},
    { "flyin_distance", "float", 0, 0, 0		},
    { "rotatein_angle", "float", 0, 0, 0		},
    { "rotatein_direction", "int", 0, 0, 0		},
    { "sheet_start_percent", "float", 0, 0, 0   	},
    { "expandpw_horiz_first", "bool", 0, 0, 0   	},
    { "expandpw_initial_horiz", "int", 0, 0, 0   	},
    { "expandpw_initial_vert", "int", 0, 0, 0   	},
    { "expandpw_delay", "float", 0, 0, 0   		}
};

static CompOption *
animGetScreenOptions (CompPlugin *plugin, CompScreen * screen, int *count)
{
    ANIMSIM_SCREEN (screen);

    *count = NUM_OPTIONS (as);
    return as->opt;
}

/*
// For effects with custom polygon step functions:
AnimExtEffectProperties fxAirplaneExtraProp = {
    .animStepPolygonFunc = fxAirplaneLinearAnimStepPolygon};
*/

AnimEffect AnimEffectFlyIn	= &(AnimEffectInfo) {};
AnimEffect AnimEffectBounce	= &(AnimEffectInfo) {};
AnimEffect AnimEffectRotateIn	= &(AnimEffectInfo) {};
AnimEffect AnimEffectSheets	= &(AnimEffectInfo) {};
AnimEffect AnimEffectExpand	= &(AnimEffectInfo) {};
AnimEffect AnimEffectExpandPW	= &(AnimEffectInfo) {};

static void
initEffectProperties (AnimSimDisplay *ad)
{
    AnimBaseFunctions  *baseFunc  = ad->animBaseFunc;

    memcpy ((AnimEffectInfo *)AnimEffectFlyIn, (&(AnimEffectInfo)
	{"animationsim:Fly In",
	 {TRUE, TRUE, TRUE, FALSE, FALSE},
         {.updateWindowAttribFunc	= fxFlyinUpdateWindowAttrib,
          .animStepFunc		= fxFlyinAnimStep,
          .initFunc			= fxFlyinInit,
          .letOthersDrawGeomsFunc	= baseFunc->returnTrue,
          .updateWinTransformFunc	= fxFlyinUpdateWindowTransform,
          .updateBBFunc		= baseFunc->compTransformUpdateBB}}),
	  sizeof (AnimEffectInfo));

    memcpy ((AnimEffectInfo *)AnimEffectBounce, (&(AnimEffectInfo)
	{"animationsim:Bounce",
	 {TRUE, TRUE, TRUE, FALSE, FALSE},
         {.updateWindowAttribFunc	= fxBounceUpdateWindowAttrib,
          .animStepFunc		= fxBounceAnimStep,
          .initFunc			= fxBounceInit,
          .letOthersDrawGeomsFunc	= baseFunc->returnTrue,
          .updateWinTransformFunc	= fxBounceUpdateWindowTransform,
          .updateBBFunc		= baseFunc->compTransformUpdateBB}}),
	  sizeof (AnimEffectInfo));
    memcpy ((AnimEffectInfo *)AnimEffectRotateIn, (&(AnimEffectInfo)
	{"animationsim:Rotate In",
	 {TRUE, TRUE, TRUE, FALSE, FALSE},
	 {.updateWindowAttribFunc	= fxRotateinUpdateWindowAttrib,
          .prePaintWindowFunc	= fxRotateinPrePaintWindow,
          .postPaintWindowFunc	= fxRotateinPostPaintWindow,
          .animStepFunc		= fxRotateinAnimStep,
          .initFunc			= fxRotateinInit,
          .letOthersDrawGeomsFunc	= baseFunc->returnTrue,
          .updateWinTransformFunc	= fxRotateinUpdateWindowTransform,
          .updateBBFunc		= baseFunc->compTransformUpdateBB,
          .zoomToIconFunc		= fxRotateinZoomToIcon}}),
	  sizeof (AnimEffectInfo));
    memcpy ((AnimEffectInfo *)AnimEffectSheets, (&(AnimEffectInfo)
	{"animationsim:Sheet",
	 {TRUE, TRUE, TRUE, FALSE, FALSE},
         {.animStepFunc		= fxSheetsModelStep,
          .initFunc		= fxSheetsInit,
          .initGridFunc		= fxSheetsInitGrid,
          .updateBBFunc		= baseFunc->modelUpdateBB,
          .useQTexCoord		= TRUE}}),
	  sizeof (AnimEffectInfo));
    memcpy ((AnimEffectInfo *)AnimEffectExpand, (&(AnimEffectInfo)
	{"animationsim:Expand",
	 {TRUE, TRUE, TRUE, FALSE, FALSE},
	 {.updateWindowAttribFunc	= fxExpandUpdateWindowAttrib,
          .animStepFunc		= fxExpandAnimStep,
          .initFunc			= fxExpandInit,
          .letOthersDrawGeomsFunc	= baseFunc->returnTrue,
          .updateWinTransformFunc	= fxExpandUpdateWindowTransform,
          .updateBBFunc		= baseFunc->compTransformUpdateBB}}),
	  sizeof (AnimEffectInfo));
    memcpy ((AnimEffectInfo *)AnimEffectExpandPW, (&(AnimEffectInfo)
	{"animationsim:Expand Piecewise",
	 {TRUE, TRUE, TRUE, FALSE, FALSE},
	 {.updateWindowAttribFunc	= fxExpandPWUpdateWindowAttrib,
          .animStepFunc		= fxExpandPWAnimStep,
          .initFunc			= fxExpandPWInit,
          .letOthersDrawGeomsFunc	= baseFunc->returnTrue,
          .updateWinTransformFunc	= fxExpandPWUpdateWindowTransform,
          .updateBBFunc		= baseFunc->compTransformUpdateBB}}),
	  sizeof (AnimEffectInfo));

    AnimEffect animEffectsTmp[NUM_EFFECTS] =
    {
	AnimEffectFlyIn,
	AnimEffectBounce,
	AnimEffectRotateIn,
	AnimEffectSheets,
	AnimEffectExpand,
	AnimEffectExpandPW
    };
    memcpy (animEffects,
	    animEffectsTmp,
	    NUM_EFFECTS * sizeof (AnimEffect));
}

static Bool animInitDisplay (CompPlugin * p, CompDisplay * d)
{
    AnimSimDisplay *ad;
    int animBaseFunctionsIndex;

    if (!checkPluginABI ("core", CORE_ABIVERSION) ||
        !checkPluginABI ("animation", ANIMATION_ABIVERSION))
    {
	compLogMessage ("animationsim", CompLogLevelError, "ABI Versions between CORE, ANIMATION and ANIMATIONSIM are not in sync. Please recompile animationsim\n");
	return FALSE;
    }

    if (!getPluginDisplayIndex (d, "animation", &animBaseFunctionsIndex))
    {
	return FALSE;
    }

    ad = calloc (1, sizeof (AnimSimDisplay));
    if (!ad)
	return FALSE;

    ad->screenPrivateIndex = allocateScreenPrivateIndex (d);
    if (ad->screenPrivateIndex < 0)
    {
	free (ad);
	return FALSE;
    }

    ad->animBaseFunc = d->base.privates[animBaseFunctionsIndex].ptr;

    initEffectProperties (ad);

    d->base.privates[animDisplayPrivateIndex].ptr = ad;

    return TRUE;
}

static void animFiniDisplay (CompPlugin * p, CompDisplay * d)
{
    ANIMSIM_DISPLAY (d);

    freeScreenPrivateIndex (d, ad->screenPrivateIndex);

    free (ad);
}

static Bool animInitScreen (CompPlugin * p, CompScreen * s)
{
    AnimSimScreen *as;

    ANIMSIM_DISPLAY (s->display);

    as = calloc (1, sizeof (AnimSimScreen));
    if (!as)
	return FALSE;

    if (!compInitScreenOptionsFromMetadata (s,
					    &animMetadata,
					    animEgScreenOptionInfo,
					    as->opt,
					    ANIMSIM_SCREEN_OPTION_NUM))
    {
	free (as);
	fprintf(stderr, "unable metadata\n");
	return FALSE;
    }

    as->windowPrivateIndex = allocateWindowPrivateIndex (s);
    if (as->windowPrivateIndex < 0)
    {
	compFiniScreenOptions (s, as->opt, ANIMSIM_SCREEN_OPTION_NUM);
	free (as);
	return FALSE;
    }

    as->output = &s->fullscreenOutput;

    animExtensionPluginInfo.effectOptions = &as->opt[NUM_NONEFFECT_OPTIONS];

    ad->animBaseFunc->addExtension (s, &animExtensionPluginInfo);

    s->base.privates[ad->screenPrivateIndex].ptr = as;

    return TRUE;
}

static void animFiniScreen (CompPlugin * p, CompScreen * s)
{
    ANIMSIM_SCREEN (s);
    ANIMSIM_DISPLAY (s->display);

    ad->animBaseFunc->removeExtension (s, &animExtensionPluginInfo);

    freeWindowPrivateIndex (s, as->windowPrivateIndex);

    compFiniScreenOptions (s, as->opt, ANIMSIM_SCREEN_OPTION_NUM);

    free (as);
}

static Bool animInitWindow (CompPlugin * p, CompWindow * w)
{
    CompScreen *s = w->screen;
    AnimSimWindow *aw;

    ANIMSIM_DISPLAY (s->display);
    ANIMSIM_SCREEN (s);

    aw = calloc (1, sizeof (AnimSimWindow));
    if (!aw)
	return FALSE;

    w->base.privates[as->windowPrivateIndex].ptr = aw;

    aw->com = ad->animBaseFunc->getAnimWindowCommon (w);

    return TRUE;
}

static void animFiniWindow (CompPlugin * p, CompWindow * w)
{
    ANIMSIM_WINDOW (w);

    free (aw);
}

static CompBool
animInitObject (CompPlugin *p,
		CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0, /* InitCore */
	(InitPluginObjectProc) animInitDisplay,
	(InitPluginObjectProc) animInitScreen,
	(InitPluginObjectProc) animInitWindow
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
animFiniObject (CompPlugin *p,
		CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0, /* FiniCore */
	(FiniPluginObjectProc) animFiniDisplay,
	(FiniPluginObjectProc) animFiniScreen,
	(FiniPluginObjectProc) animFiniWindow
    };

    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

static CompOption *
animGetObjectOptions (CompPlugin *plugin,
		      CompObject *object,
		      int	   *count)
{
    static GetPluginObjectOptionsProc dispTab[] = {
	(GetPluginObjectOptionsProc) 0, /* GetCoreOptions */
	(GetPluginObjectOptionsProc) 0, /* GetDisplayOptions */
	(GetPluginObjectOptionsProc) animGetScreenOptions
    };

    RETURN_DISPATCH (object, dispTab, ARRAY_SIZE (dispTab),
		     (void *) (long int)(*count = 0), (plugin, object, count));
}

static CompBool
animSetObjectOption (CompPlugin      *plugin,
		     CompObject      *object,
		     const char      *name,
		     CompOptionValue *value)
{
    static SetPluginObjectOptionProc dispTab[] = {
	(SetPluginObjectOptionProc) 0, /* SetCoreOption */
	(SetPluginObjectOptionProc) 0, /* SetDisplayOption */
	(SetPluginObjectOptionProc) animSetScreenOptions
    };

    RETURN_DISPATCH (object, dispTab, ARRAY_SIZE (dispTab), FALSE,
		     (plugin, object, name, value));
}

static Bool animInit (CompPlugin * p)
{
    if (!compInitPluginMetadataFromInfo (&animMetadata,
					 p->vTable->name,
					 0, 0,
					 animEgScreenOptionInfo,
					 ANIMSIM_SCREEN_OPTION_NUM))
     {
	fprintf(stderr, "init no metadata\n");
	return FALSE;
     }

    animDisplayPrivateIndex = allocateDisplayPrivateIndex ();
    if (animDisplayPrivateIndex < 0)
    {
	compFiniMetadata (&animMetadata);
	return FALSE;
    }

    compAddMetadataFromFile (&animMetadata, p->vTable->name);

    return TRUE;
}

static void animFini (CompPlugin * p)
{
    freeDisplayPrivateIndex (animDisplayPrivateIndex);
    compFiniMetadata (&animMetadata);
}

static CompMetadata *
animGetMetadata (CompPlugin *plugin)
{
    return &animMetadata;
}

CompPluginVTable animVTable = {
    "animationsim",
    animGetMetadata,
    animInit,
    animFini,
    animInitObject,
    animFiniObject,
    animGetObjectOptions,
    animSetObjectOption,
};

CompPluginVTable*
getCompPluginInfo20070830 (void)
{
    return &animVTable;
}
