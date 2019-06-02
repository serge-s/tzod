#pragma once
#include <video/RenderBase.h>
#include <cassert>
#include <functional>
#include <list>
#include <memory>
#include <string>
#include <deque>

class RenderContext;
class TextureManager;

namespace Plat
{
	enum class Key;
	struct Input;
}

namespace UI
{

class DataContext;
class InputContext;
class LayoutContext;
class StateContext;
struct LayoutConstraints;
struct NavigationSink;
struct PointerSink;
template <class> struct LayoutData;

enum class StretchMode
{
	Stretch,
	Fill,
};

enum class FlowDirection
{
	Vertical,
	Horizontal
};

struct ScrollSink
{
	virtual void OnScroll(TextureManager &texman, const InputContext &ic, const LayoutContext &lc, const DataContext &dc, vec2d scrollOffset, bool precise) = 0;
	virtual void EnsureVisible(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, FRECT pxFocusRect) = 0;
};

struct KeyboardSink
{
	virtual bool OnKeyPressed(const InputContext &ic, Plat::Key key) { return false; }
	virtual void OnKeyReleased(const InputContext &ic, Plat::Key key) {}
};

struct TextSink
{
	virtual bool OnChar(int c) = 0;
	virtual void OnPaste(std::string_view text) = 0;
	virtual std::string_view OnCopy() const = 0;
	virtual std::string OnCut() = 0;
};

class StateContext;

struct StateGen
{
	virtual void PushState(StateContext &sc, const LayoutContext &lc, const InputContext &ic, bool hovered) const = 0;
};

struct WindowLayout
{
	FRECT rect;
	float opacity;
	bool enabled;
};

class Window
{
public:
	Window();
	virtual ~Window() = default;

	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;

	// Subtree
	virtual unsigned int GetChildrenCount() const { return 0; }
	virtual std::shared_ptr<const Window> GetChild(const std::shared_ptr<const Window>& owner, unsigned int index) const { assert(false); return nullptr; }
	virtual const Window& GetChild(unsigned int index) const { assert(false); return *this; }
	virtual WindowLayout GetChildLayout(TextureManager& texman, const LayoutContext& lc, const DataContext& dc, const Window& child) const { assert(false); return {}; }
	virtual void SetFocus(Window* child) { assert(false); }
	virtual std::shared_ptr<const Window> GetFocus(const std::shared_ptr<const Window>& owner) const { return nullptr; }
	virtual const Window* GetFocus() const { return nullptr; }

	// Input
	virtual NavigationSink* GetNavigationSink() { return nullptr; }
	virtual ScrollSink* GetScrollSink() { return nullptr; }
	virtual PointerSink* GetPointerSink() { return nullptr; }
	virtual KeyboardSink* GetKeyboardSink() { return nullptr; }
	virtual TextSink* GetTextSink() { return nullptr; }

	// State
	virtual const StateGen* GetStateGen() const { return nullptr; }

	//
	// Appearance
	//

	void SetVisible(bool show);
	bool GetVisible() const { return _isVisible; }

	void SetTopMost(bool topmost);
	bool GetTopMost() const { return _isTopMost; }

	void SetClipChildren(bool clip) { _clipChildren = clip; }
	bool GetClipChildren() const { return _clipChildren; }


	//
	// size & position
	//

	virtual vec2d GetContentSize(TextureManager &texman, const DataContext &dc, float scale, const LayoutConstraints &layoutConstraints) const { return Vec2dFloor(GetSize() *scale); }

	void Move(float x, float y);
	vec2d GetOffset() const { return vec2d{_x, _y}; }

	void Resize(float width, float height);
	void SetHeight(float height) { Resize(GetWidth(), height); }
	void SetWidth(float width) { Resize(width, GetHeight()); }
	float GetWidth() const { return _width; }
	float GetHeight() const { return _height; }
	vec2d GetSize() const { return vec2d{GetWidth(), GetHeight()}; }

	// rendering
	virtual void Draw(const DataContext &dc, const StateContext &sc, const LayoutContext &lc, const InputContext &ic, RenderContext &rc, TextureManager &texman, float time, bool hovered) const {}

	// const utils
	bool HasNavigationSink() const { return !!const_cast<Window*>(this)->GetNavigationSink(); }
	bool HasScrollSink() const { return !!const_cast<Window*>(this)->GetScrollSink(); }
	bool HasPointerSink() const { return !!const_cast<Window*>(this)->GetPointerSink(); }
	bool HasKeyboardSink() const { return !!const_cast<Window*>(this)->GetKeyboardSink(); }
	bool HasTextSink() const { return !!const_cast<Window*>(this)->GetTextSink(); }
	std::shared_ptr<Window> GetChild(const std::shared_ptr<const Window>& owner, unsigned int index)
	{
		return std::const_pointer_cast<Window>(static_cast<const Window*>(this)->GetChild(owner, index));
	}
	Window& GetChild(unsigned int index)
	{
		return const_cast<Window&>(static_cast<const Window*>(this)->GetChild(index));
	}
	std::shared_ptr<Window> GetFocus(const std::shared_ptr<const Window>& owner)
	{
		return std::const_pointer_cast<Window>(static_cast<const Window*>(this)->GetFocus(owner));
	}
	Window* GetFocus()
	{
		return const_cast<Window*>(static_cast<const Window*>(this)->GetFocus());
	}

private:
	// size and position
	float _x = 0;
	float _y = 0;
	float _width = 0;
	float _height = 0;

	// attributes
	struct
	{
		bool _isVisible : 1;
		bool _isTopMost : 1;
		bool _clipChildren : 1;
	};
};

class WindowContainer : public Window
{
public:
	~WindowContainer() override;

	void UnlinkAllChildren();
	void UnlinkChild(Window& child);
	void AddFront(std::shared_ptr<Window> child);
	void AddBack(std::shared_ptr<Window> child);

	using Window::GetChild;

	// Window
	unsigned int GetChildrenCount() const override final;
	std::shared_ptr<const Window> GetChild(const std::shared_ptr<const Window>& owner, unsigned int index) const override final;
	const Window& GetChild(unsigned int index) const override final;
	WindowLayout GetChildLayout(TextureManager& texman, const LayoutContext& lc, const DataContext& dc, const Window& child) const override;
	void SetFocus(Window* child) override final;
	std::shared_ptr<const Window> GetFocus(const std::shared_ptr<const Window>& owner) const override;
	const Window* GetFocus() const override;

protected:
	const std::deque<std::shared_ptr<Window>>& GetChildren() const { return _children; }

private:
	Window* _focusChild = nullptr;
	std::deque<std::shared_ptr<Window>> _children;
};

bool NeedsFocus(TextureManager& texman, const InputContext& ic, const Window& wnd, const LayoutContext& lc, const DataContext& dc);
FRECT CanvasLayout(vec2d offset, vec2d size, float scale);

//////////////////////// to remove ////////////////////
class TimeStepManager;
class Managerful
{
protected:
	explicit Managerful(TimeStepManager &manager) : _manager(manager) {}

	TimeStepManager& GetTimeStepManager() const { return _manager; }

private:
	TimeStepManager &_manager;
};

class TimeStepping : public Managerful
{
public:
	explicit TimeStepping(TimeStepManager &manager) : Managerful(manager) {}
	~TimeStepping();

	void SetTimeStep(bool enable);

	virtual void OnTimeStep(Plat::Input &input, bool focused, float dt) {}

private:
	std::list<TimeStepping*>::iterator _timeStepReg;
	bool _isTimeStep = false;
};

} // namespace UI
