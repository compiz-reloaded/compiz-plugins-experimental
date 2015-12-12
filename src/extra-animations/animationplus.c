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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <limits.h>
#include <GL/glu.h>
#include "animationplus.h"

int animDisplayPrivateIndex;
CompMetadata animMetadata;

AnimEffect animEffects[NUM_EFFECTS];

ExtensionPluginInfo animExtensionPluginInfo = {
    .nEffects		= NUM_EFFECTS,
    .effects		= animEffects,

    .nEffectOptions	= ANIMPLUS_SCREEN_OPTION_NUM,
};

OPTION_GETTERS (GET_ANIMPLUS_DISPLAY (w->screen->display)->animBaseFunc,
		&animExtensionPluginInfo, NUM_NONEFFECT_OPTIONS)

static Bool
animSetScreenOptions (CompPlugin *plugin,
		      CompScreen * screen,
		      const char *name,
		      CompOptionValue * value)
{
    CompOption *o;
    int index;

    ANIMPLUS_SCREEN (screen);

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

static const CompMetadataOptionInfo animPlusScreenOptionInfo[] = {

    { "blinds_num_halftwists", "int", "<min>1</min>", 0, 0 },
    { "blinds_gridx", "int", "<min>1</min>", 0, 0 },
    { "blinds_thickness", "float", 0, 0, 0 },
    { "bonanza_particles", "int", "<min>0</min>", 0, 0 },
    { "bonanza_size", "float", "<min>0.1</min>", 0, 0 },
    { "bonanza_life", "float", "<min>0.1</min>", 0, 0 },
    { "bonanza_color", "color", 0, 0, 0 },
    { "bonanza_mystical", "bool", 0, 0, 0 },
    { "helix_num_twists", "int", "<min>1</min>", 0, 0 },
    { "helix_gridy", "int", "<min>5</min>", 0, 0 },
    { "helix_thickness", "float", 0, 0, 0 },
    { "helix_direction", "bool", 0, 0, 0 },
    { "helix_spin_direction", "int", "<min>0</min>", 0, 0 },
    { "shatter_num_spokes", "int", "<min>0</min>", 0, 0},
    { "shatter_num_tiers", "int", "<min>0</min>", 0, 0}
};

static CompOption *
animGetScreenOptions (CompPlugin *plugin, CompScreen * screen, int *count)
{
    ANIMPLUS_SCREEN (screen);

    *count = NUM_OPTIONS (as);
    return as->opt;
}

/*
// For effects with custom polygon step functions:
AnimExtEffectProperties fxAirplaneExtraProp = {
    .animStepPolygonFunc = fxAirplaneLinearAnimStepPolygon};
*/

AnimEffect AnimEffectBlinds	= &(AnimEffectInfo) {};
AnimEffect AnimEffectHelix	= &(AnimEffectInfo) {};
AnimEffect AnimEffectShatter= &(AnimEffectInfo) {};
AnimEffect AnimEffectBonanza= &(AnimEffectInfo) {};

static void
initEffectProperties (AnimPlusDisplay *ad)
{
    AnimAddonFunctions *addonFunc = ad->animAddonFunc;

    memcpy ((AnimEffectInfo *)AnimEffectBlinds, (&(AnimEffectInfo)
	{"animationplus:Blinds",
	 {TRUE, TRUE, TRUE, FALSE, FALSE},
	 {.prePaintWindowFunc		= addonFunc->polygonsPrePaintWindow,
	  .postPaintWindowFunc		= addonFunc->polygonsPostPaintWindow,
	  .animStepFunc			= addonFunc->polygonsAnimStep,
	  .initFunc			= fxBlindsInit,
	  .addCustomGeometryFunc	= addonFunc->polygonsStoreClips,
	  .drawCustomGeometryFunc	= addonFunc->polygonsDrawCustomGeometry,
	  .updateBBFunc			= addonFunc->polygonsUpdateBB,
	  .prePrepPaintScreenFunc	= addonFunc->polygonsPrePreparePaintScreen,
	  .cleanupFunc			= addonFunc->polygonsCleanup,
	  .refreshFunc			= addonFunc->polygonsRefresh}}),
	  sizeof (AnimEffectInfo));

    memcpy ((AnimEffectInfo *)AnimEffectBonanza, (&(AnimEffectInfo)
	{"animationplus:Bonanza",
	 {TRUE, TRUE, TRUE, FALSE, FALSE},
	 {.postPaintWindowFunc	= addonFunc->drawParticleSystems,
	  .animStepFunc			= fxBonanzaAnimStep,
	  .initFunc			    = fxBonanzaInit,
	  .updateBBFunc			= addonFunc->particlesUpdateBB,
	  .prePrepPaintScreenFunc	= addonFunc->particlesPrePrepPaintScreen,
	  .cleanupFunc			= addonFunc->particlesCleanup}}),
	  sizeof (AnimEffectInfo));

    memcpy ((AnimEffectInfo *)AnimEffectHelix, (&(AnimEffectInfo)
	{"animationplus:Helix",
	 {TRUE, TRUE, TRUE, FALSE, FALSE},
	 {.prePaintWindowFunc		= addonFunc->polygonsPrePaintWindow,
	  .postPaintWindowFunc		= addonFunc->polygonsPostPaintWindow,
	  .animStepFunc			= addonFunc->polygonsAnimStep,
	  .initFunc			= fxHelixInit,
	  .addCustomGeometryFunc	= addonFunc->polygonsStoreClips,
	  .drawCustomGeometryFunc	= addonFunc->polygonsDrawCustomGeometry,
	  .updateBBFunc			= addonFunc->polygonsUpdateBB,
	  .prePrepPaintScreenFunc	= addonFunc->polygonsPrePreparePaintScreen,
	  .cleanupFunc			= addonFunc->polygonsCleanup,
	  .refreshFunc			= addonFunc->polygonsRefresh}}),
	  sizeof (AnimEffectInfo));

    memcpy ((AnimEffectInfo *)AnimEffectShatter, (&(AnimEffectInfo)
	{"animationplus:Shatter",
	 {TRUE, TRUE, TRUE, FALSE, FALSE},
	 {.prePaintWindowFunc		= addonFunc->polygonsPrePaintWindow,
	  .postPaintWindowFunc		= addonFunc->polygonsPostPaintWindow,
	  .animStepFunc			= addonFunc->polygonsAnimStep,
	  .initFunc			= fxShatterInit,
	  .addCustomGeometryFunc	= addonFunc->polygonsStoreClips,
	  .drawCustomGeometryFunc	= addonFunc->polygonsDrawCustomGeometry,
	  .updateBBFunc			= addonFunc->polygonsUpdateBB,
	  .prePrepPaintScreenFunc	= addonFunc->polygonsPrePreparePaintScreen,
	  .cleanupFunc			= addonFunc->polygonsCleanup,
	  .refreshFunc			= addonFunc->polygonsRefresh}}),
	  sizeof (AnimEffectInfo));


    AnimEffect animEffectsTmp[NUM_EFFECTS] =
    {
	    AnimEffectBlinds,
        AnimEffectBonanza,
	    AnimEffectHelix,
        AnimEffectShatter
    };
    memcpy (animEffects,
	    animEffectsTmp,
	    NUM_EFFECTS * sizeof (AnimEffect));
}

static Bool animInitDisplay (CompPlugin * p, CompDisplay * d)
{
    AnimPlusDisplay *ad;
    int animBaseFunctionsIndex;
    int animAddonFunctionsIndex;

    if (!checkPluginABI ("core", CORE_ABIVERSION) ||
        !checkPluginABI ("animation", ANIMATION_ABIVERSION) ||
        !checkPluginABI ("animationaddon", ANIMATIONADDON_ABIVERSION))
	return FALSE;

    if (!getPluginDisplayIndex (d, "animation", &animBaseFunctionsIndex) ||
	!getPluginDisplayIndex (d, "animationaddon", &animAddonFunctionsIndex))
	return FALSE;

    ad = calloc (1, sizeof (AnimPlusDisplay));
    if (!ad)
	return FALSE;

    ad->screenPrivateIndex = allocateScreenPrivateIndex (d);
    if (ad->screenPrivateIndex < 0)
    {
	free (ad);
	return FALSE;
    }

    ad->animBaseFunc = d->base.privates[animBaseFunctionsIndex].ptr;
    ad->animAddonFunc = d->base.privates[animAddonFunctionsIndex].ptr;

    initEffectProperties (ad);

    d->base.privates[animDisplayPrivateIndex].ptr = ad;

    return TRUE;
}

static void animFiniDisplay (CompPlugin * p, CompDisplay * d)
{
    ANIMPLUS_DISPLAY (d);

    freeScreenPrivateIndex (d, ad->screenPrivateIndex);

    free (ad);
}

static Bool animInitScreen (CompPlugin * p, CompScreen * s)
{
    AnimPlusScreen *as;

    ANIMPLUS_DISPLAY (s->display);

    as = calloc (1, sizeof (AnimPlusScreen));
    if (!as)
	return FALSE;

    if (!compInitScreenOptionsFromMetadata (s,
					    &animMetadata,
					    animPlusScreenOptionInfo,
					    as->opt,
					    ANIMPLUS_SCREEN_OPTION_NUM))
    {
	free (as);
	return FALSE;
    }

    as->windowPrivateIndex = allocateWindowPrivateIndex (s);
    if (as->windowPrivateIndex < 0)
    {
	compFiniScreenOptions (s, as->opt, ANIMPLUS_SCREEN_OPTION_NUM);
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
    ANIMPLUS_SCREEN (s);
    ANIMPLUS_DISPLAY (s->display);

    ad->animBaseFunc->removeExtension (s, &animExtensionPluginInfo);

    freeWindowPrivateIndex (s, as->windowPrivateIndex);

    compFiniScreenOptions (s, as->opt, ANIMPLUS_SCREEN_OPTION_NUM);

    free (as);
}

static Bool animInitWindow (CompPlugin * p, CompWindow * w)
{
    CompScreen *s = w->screen;
    AnimPlusWindow *aw;

    ANIMPLUS_DISPLAY (s->display);
    ANIMPLUS_SCREEN (s);

    aw = calloc (1, sizeof (AnimPlusWindow));
    if (!aw)
	return FALSE;

    w->base.privates[as->windowPrivateIndex].ptr = aw;

    aw->com = ad->animBaseFunc->getAnimWindowCommon (w);
    aw->eng = ad->animAddonFunc->getAnimWindowEngineData (w);

    return TRUE;
}

static void animFiniWindow (CompPlugin * p, CompWindow * w)
{
    ANIMPLUS_WINDOW (w);

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
		     (void *) (*count = 0), (plugin, object, count));
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
                                         animPlusScreenOptionInfo,
                                         ANIMPLUS_SCREEN_OPTION_NUM))
        return FALSE;

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
    "animationplus",
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
