#pragma once
#include "PointerInput.h"
#include <math/MyMath.h>
#include <list>
#include <memory>
#include <vector>

class RenderContext;
class TextureManager;

namespace UI
{

class DataContext;
class InputContext;
class LayoutContext;
class TimeStepping;
class StateContext;
class Window;

struct RenderSettings
{
	InputContext &ic;
	RenderContext &rc;
	TextureManager &texman;
	float time;
	std::vector<std::shared_ptr<Window>> hoverPath;
	bool topMostPass;
};

void RenderUIRoot(Window &desktop, RenderSettings &rs, const LayoutContext &lc, const DataContext &dc, const StateContext &sc);

class TimeStepManager
{
public:
	TimeStepManager();

	void TimeStep(std::shared_ptr<Window> desktop, InputContext &ic, float dt);
	float GetTime() const { return _time; }

private:
	friend class TimeStepping;
	std::list<TimeStepping*>::iterator TimeStepRegister(TimeStepping* wnd);
	void TimeStepUnregister(std::list<TimeStepping*>::iterator it);

	std::list<TimeStepping*> _timestep;
	std::list<TimeStepping*>::iterator _tsCurrent;
	bool _tsDeleteCurrent;
	float _time = 0;
};

} // namespace UI
