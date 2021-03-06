#include "inc/ui/Button.h"
#include "inc/ui/DataSource.h"
#include "inc/ui/InputContext.h"
#include "inc/ui/LayoutContext.h"
#include "inc/ui/Rectangle.h"
#include "inc/ui/StateContext.h"
#include "inc/ui/Text.h"
#include <plat/Input.h>
#include <video/TextureManager.h>
#include <video/RenderContext.h>
#include <algorithm>

using namespace UI;

ButtonBase::State ButtonBase::GetState(const Plat::Input &input, const LayoutContext &lc, const InputContext &ic, bool hovered) const
{
	if (!lc.GetEnabledCombined() || !eventClick)
		return stateDisabled;

	bool pointerInside = ic.GetPointerType(0) != Plat::PointerType::Unknown && PtInFRect(MakeRectWH(lc.GetPixelSize()), ic.GetPointerPos(0, lc));
	bool pointerPressed = input.GetPointerState(0).pressed;
	if ((pointerInside && pointerPressed && ic.HasCapturedPointers(this)) || ic.GetNavigationSubject(Navigate::Enter).get() == this)
		return statePushed;

	bool focusActive = (ic.GetLastKeyTime() >= ic.GetLastPointerTime()) && lc.GetFocusedCombined();
	bool pointerActive = (ic.GetLastPointerTime() > ic.GetLastKeyTime()) && (ic.HasCapturedPointers(this) || (hovered && !pointerPressed));
	if (focusActive || pointerActive)
		return stateHottrack;

	return stateNormal;
}

bool ButtonBase::OnPointerDown(const Plat::Input &input, const  InputContext &ic, const LayoutContext &lc, TextureManager &texman, PointerInfo pi, int button)
{
	// touch or primary button only
	return !ic.HasCapturedPointers(this) && (Plat::PointerType::Touch == pi.type || 1 == button);
}

void ButtonBase::OnPointerUp(const InputContext &ic, const LayoutContext &lc, TextureManager &texman, PointerInfo pi, int button)
{
	if( PtInFRect(MakeRectWH(lc.GetPixelSize()), pi.position) )
	{
		DoClick();
	}
}

void ButtonBase::OnTap(const InputContext &ic, const LayoutContext &lc, TextureManager &texman, vec2d pointerPosition)
{
	if( !ic.HasCapturedPointers(this))
	{
		DoClick();
	}
}

void ButtonBase::PushState(const Plat::Input& input, StateContext &sc, const LayoutContext &lc, const InputContext &ic, bool hovered) const
{
	switch (GetState(input, lc, ic, hovered))
	{
	case statePushed:
		sc.SetState("Pushed");
		break;
	case stateHottrack:
		sc.SetState("Hover");
		break;
	case stateNormal:
		sc.SetState("Idle");
		break;
	case stateDisabled:
		sc.SetState("Disabled");
		break;
	default:
		assert(false);
	}
}

void ButtonBase::DoClick()
{
	OnClick();
	if (eventClick)
		eventClick();
}

bool ButtonBase::CanNavigate(TextureManager& texman, const LayoutContext& lc, const DataContext& dc, Navigate navigate) const
{
	return Navigate::Enter == navigate && eventClick;
}

void ButtonBase::OnNavigate(TextureManager& texman, const LayoutContext& lc, const DataContext& dc, Navigate navigate, NavigationPhase phase)
{
	if (NavigationPhase::Completed == phase && Navigate::Enter == navigate)
	{
		DoClick();
	}
}

///////////////////////////////////////////////////////////////////////////////

static const auto c_textColor = std::make_shared<StateBinding<SpriteColor>>(0xffeeeeee, // default
	StateBinding<SpriteColor>::MapType{ { "Disabled", 0xaaaaaaaa }, { "Hover", 0xffffffff }, { "Pushed", 0xffffffff } });

static const auto c_backgroundFrame = std::make_shared<StateBinding<unsigned int>>(0, // default
	StateBinding<unsigned int>::MapType{ { "Disabled", 3 }, { "Hover", 1 }, { "Pushed", 2 } });

Button::Button()
{
	_background.SetFrame(c_backgroundFrame);

	_text.SetAlign(alignTextCC);
	_text.SetFontColor(c_textColor);

	SetBackground("ui/button");
	Resize(96, 24);
}

void Button::SetFont(Texture fontTexture)
{
	_text.SetFont(std::move(fontTexture));
}

void Button::SetText(std::shared_ptr<LayoutData<std::string_view>> text)
{
	_text.SetText(std::move(text));
}

void Button::SetBackground(Texture background)
{
	_background.SetTexture(std::move(background));
}

const Texture& Button::GetBackground() const
{
	return _background.GetTexture();
}

void Button::AlignToBackground(TextureManager &texman)
{
	Resize(_background.GetTextureWidth(texman), _background.GetTextureHeight(texman));
}

std::shared_ptr<const Window> Button::GetChild(const std::shared_ptr<const Window>& owner, unsigned int index) const
{
	return { owner, &Button::GetChild(index) };
}

const Window& Button::GetChild(unsigned int index) const
{
	switch (index)
	{
	default:
		assert(false);
	case 0: return _background;
	case 1: return _text;
	}
}

WindowLayout Button::GetChildLayout(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const
{
	vec2d size = lc.GetPixelSize();

	if (&_background == &child)
	{
		return WindowLayout{ MakeRectRB(vec2d{}, size), 1, true };
	}
	if (&_text == &child)
	{
		return WindowLayout{ MakeRectWH(Vec2dFloor(size / 2), vec2d{}), 1, true };
	}

	assert(false);
	return {};
}

///////////////////////////////////////////////////////////////////////////////
// ContentButton

unsigned int ContentButton::GetChildrenCount() const
{
	return !!_content;
}

std::shared_ptr<const Window> ContentButton::GetChild(const std::shared_ptr<const Window>& owner, unsigned int index) const
{
	assert(_content && index == 0);
	return _content;
}

const Window& ContentButton::GetChild(unsigned int index) const
{
	assert(_content && index == 0);
	return *_content;
}

vec2d ContentButton::GetContentSize(TextureManager &texman, const DataContext &dc, float scale, const LayoutConstraints &layoutConstraints) const
{
	return _content ? _content->GetContentSize(texman, dc, scale, layoutConstraints) : vec2d{};
}

WindowLayout ContentButton::GetChildLayout(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const
{
	return WindowLayout{ MakeRectWH(lc.GetPixelSize()), 1, true };
}

///////////////////////////////////////////////////////////////////////////////

CheckBox::CheckBox()
{
	_text.SetFontColor(c_textColor);
}

void CheckBox::SetCheck(bool checked)
{
	_isChecked = checked;
}

void CheckBox::OnClick()
{
	SetCheck(!GetCheck());
}

void CheckBox::SetFont(Texture fontTexture)
{
	_text.SetFont(std::move(fontTexture));
}

void CheckBox::SetText(std::shared_ptr<LayoutData<std::string_view>> text)
{
	_text.SetText(std::move(text));
}

WindowLayout CheckBox::GetChildLayout(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const
{
	if (&_text == &child)
	{
		float pxBoxWidth = _boxPosition == BoxPosition::Left ? ToPx(_boxTexture.GetTextureSize(texman), lc).x : 0;
		vec2d pxTextSize = _text.GetContentSize(texman, dc, lc.GetScaleCombined(), DefaultLayoutConstraints(lc));
		return WindowLayout{ MakeRectWH(vec2d{ pxBoxWidth, std::floor((lc.GetPixelSize().y - pxTextSize.y) / 2) }, pxTextSize), 1, true };
	}
	return ButtonBase::GetChildLayout(texman, lc, dc, child);
}

void CheckBox::Draw(const DataContext &dc, const StateContext &sc, const LayoutContext &lc, const InputContext &ic, RenderContext &rc, TextureManager &texman, const Plat::Input &input, float time, bool hovered) const
{
	State state = GetState(input, lc, ic, hovered);
	unsigned int frame = _isChecked ? state + 4 : state;

	vec2d pxBoxSize = ToPx(_boxTexture.GetTextureSize(texman), lc);
	float boxLeft = _boxPosition == BoxPosition::Right ? lc.GetPixelSize().x - pxBoxSize.x : 0;
	auto box = MakeRectWH(vec2d{ boxLeft, std::floor((lc.GetPixelSize().y - pxBoxSize.y) / 2)}, pxBoxSize);
	rc.DrawSprite(box, _boxTexture.GetTextureId(texman), 0xffffffff, frame);
}

vec2d CheckBox::GetContentSize(TextureManager &texman, const DataContext &dc, float scale, const LayoutConstraints &layoutConstraints) const
{
	vec2d pxTextSize = _text.GetContentSize(texman, dc, scale, layoutConstraints);
	vec2d pxBoxSize = ToPx(texman.GetFrameSize(_boxTexture.GetTextureId(texman)), scale);
	return vec2d{ pxTextSize.x + pxBoxSize.x, std::max(pxTextSize.y, pxBoxSize.y) };
}
