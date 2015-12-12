#ifndef ROTATINGCUBE_H
#define ROTATINGCUBE_H

#include "screensaver_internal.h"

class ScreenRotatingCube : public ScreenEffect
{
public:
	ScreenRotatingCube( CompScreen* s ) : ScreenEffect(s) {}
	virtual ~ScreenRotatingCube() {}

	virtual bool enable();
	virtual void disable();
	virtual void getRotation( float* x, float* v, float *progress );
	virtual void preparePaintScreen( int msSinceLastPaint );
	virtual void donePaintScreen();
	virtual Bool paintOutput(	const ScreenPaintAttrib *sAttrib, \
								const CompTransform* transform, Region region, \
								CompOutput* output, unsigned int mask);
protected:
	virtual void clean();

private:
	bool loadCubePlugin();
};

#endif
