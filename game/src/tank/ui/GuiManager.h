// GuiManager.h

#pragma once

#include "Window.h"
#include <list>

class TextureManager;

namespace UI
{

class LayoutManager;

struct IWindowFactory
{
    virtual Window* Create(LayoutManager *pManager) = 0;
};

enum Msg
{
    MSGKEYUP,
    MSGKEYDOWN,
    MSGCHAR,
	MSGLBUTTONDOWN,
	MSGRBUTTONDOWN,
	MSGMBUTTONDOWN,
	MSGLBUTTONUP,
	MSGRBUTTONUP,
	MSGMBUTTONUP,
	MSGMOUSEMOVE,
	MSGMOUSEWHEEL,
};

class LayoutManager
{
public:
	LayoutManager(TextureManager &texman, IWindowFactory &&desktopFactory);
	~LayoutManager();

	void TimeStep(float dt);
	void Render() const;

	bool ProcessMouse(float x, float y, float z, Msg msg);
	bool ProcessKeys(Msg msg, int c);

	TextureManager& GetTextureManager() const { return _texman; }
	Window* GetDesktop() const;

	Window* GetCapture() const;
	void SetCapture(Window* wnd);

	bool SetFocusWnd(Window* wnd);  // always resets previous focus
	Window* GetFocusWnd() const;
//	bool ResetFocus(Window* wnd);   // remove focus from wnd or any of its children

	bool IsMainWindowActive() const { return _isAppActive; }
    vec2d GetMousePos() const { return _lastMousePos; }
    
private:
	friend class Window;
	void AddTopMost(Window* wnd, bool add);
	void ResetWindow(Window* wnd);
    std::list<Window*>::iterator TimeStepRegister(Window* wnd);
	void TimeStepUnregister(std::list<Window*>::iterator it);

	bool ProcessMouseInternal(Window* wnd, float x, float y, float z, Msg msg);

    TextureManager &_texman;
    std::list<Window*> _topmost;
    std::list<Window*> _timestep;
    std::list<Window*>::iterator _tsCurrent;
    bool _tsDeleteCurrent;
    

	unsigned int _captureCountSystem;
	unsigned int _captureCount;

	WindowWeakPtr _focusWnd;
	WindowWeakPtr _hotTrackWnd;
	WindowWeakPtr _captureWnd;

	WindowWeakPtr _desktop;
    
    vec2d _lastMousePos;

	bool _isAppActive;
#ifndef NDEBUG
	bool _dbgFocusIsChanging;
#endif
};

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI
// end of file
