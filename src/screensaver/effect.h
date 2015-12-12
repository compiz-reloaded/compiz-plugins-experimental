#ifndef EFFECT_H
#define EFFECT_H

#include "screensaver_internal.h"

class DisplayEffect : public DisplayWrapper
{
public:
	DisplayEffect( CompDisplay* w );
	virtual ~DisplayEffect() {}

	bool cleanEffect;
	bool loadEffect;
};

class ScreenEffect : public ScreenWrapper
{
public:
	ScreenEffect( CompScreen* s ) : ScreenWrapper(s), progress(0)  {}
	virtual ~ScreenEffect() {}
	float getProgress() { return progress; }

	virtual bool enable();
	virtual void disable() {}
	virtual void preparePaintScreen( int msSinceLastPaint );

protected:
	virtual void clean() {}

private:
	float progress;
};

class WindowEffect : public WindowWrapper
{
public:
	WindowEffect( CompWindow* w ) : WindowWrapper(w) {}
	virtual ~WindowEffect() {}
};

#endif
