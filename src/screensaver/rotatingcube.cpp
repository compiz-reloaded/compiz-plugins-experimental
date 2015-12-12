#include "rotatingcube.h"

bool ScreenRotatingCube::loadCubePlugin()
{
	CompDisplay* d = s->display;

	if (!checkPluginABI ("core", CORE_ABIVERSION))
		return false;

	if (!checkPluginABI ("cube", CUBE_ABIVERSION))
		return false;

	if (!getPluginDisplayIndex (d, "cube", &cubeDisplayPrivateIndex))
		return false;

	return cubeDisplayPrivateIndex >= 0;
}

bool ScreenRotatingCube::enable()
{
	if( !loadCubePlugin() )
		return false;

	CUBE_SCREEN(s);

	ss->zCamera = 0.0;
	ss->cubeRotX = 0.0;
	ss->cubeRotV = 0.0;
	cs->rotationState = RotationManual;
	WRAP( ss, cs, getRotation, screenSaverGetRotation );

	return ScreenEffect::enable();
}

void ScreenRotatingCube::disable()
{
	ss->zCameraFadeOut = ss->zCamera;
	ss->cubeRotXFadeOut = ss->cubeRotX;
	ss->cubeRotVFadeOut = ss->cubeRotV;

	ScreenEffect::disable();
}

void ScreenRotatingCube::clean()
{
	CUBE_SCREEN(s);
	cs->rotationState = RotationNone;
	UNWRAP (ss, cs, getRotation);
}

void ScreenRotatingCube::getRotation( float* x, float* v, float *progress )
{
	ScreenEffect::getRotation( x, v, progress );

	*x += ss->cubeRotX;
	*v += ss->cubeRotV;
	*progress = MAX (*progress, ss->cubeProgress);
}

void ScreenRotatingCube::preparePaintScreen( int msSinceLastPaint )
{

	ScreenEffect::preparePaintScreen( msSinceLastPaint );

	float rotX = screensaverGetCubeRotationSpeed(s->display)/100.0;
	float rotV = 0.0;

	SCREENSAVER_DISPLAY(s->display);

	if( sd->state.fadingIn )
	{
		rotX *= getProgress();
		ss->zCamera = -screensaverGetCubeZoom(s->display) * getProgress();
		ss->cubeProgress = getProgress();
	}
	else if( sd->state.fadingOut )
	{
		ss->zCamera = ss->zCameraFadeOut * (1-getProgress());
		ss->cubeRotX = ss->cubeRotXFadeOut * (1-getProgress());
		ss->cubeRotV = ss->cubeRotVFadeOut * (1-getProgress());
		ss->cubeProgress = 1 - getProgress();
	}

	if( !sd->state.fadingOut )
	{
		ss->cubeRotX += rotX * msSinceLastPaint;
		ss->cubeRotV += rotV * msSinceLastPaint;
	}
	if( ss->cubeRotX > 180.0 ) ss->cubeRotX -= 360.0;
	if( ss->cubeRotX < -180.0 ) ss->cubeRotX += 360.0;
}

void ScreenRotatingCube::donePaintScreen()
{
	damageScreen(s);
	ScreenEffect::donePaintScreen();
}

Bool ScreenRotatingCube::paintOutput(	const ScreenPaintAttrib *sAttrib, \
										const CompTransform* transform, Region region, \
										CompOutput* output, unsigned int mask)
{
	ScreenPaintAttrib sA = *sAttrib;
	sA.zCamera += ss->zCamera;

	mask &= ~PAINT_SCREEN_REGION_MASK;
	mask |= PAINT_SCREEN_TRANSFORMED_MASK;
	return ScreenEffect::paintOutput( &sA, transform, region, output, mask );
}
