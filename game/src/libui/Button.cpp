#include "inc/ui/Button.h"
#include "inc/ui/GuiManager.h"
#include <video/TextureManager.h>
#include <video/DrawingContext.h>
#include <algorithm>

using namespace UI;

ButtonBase::ButtonBase(LayoutManager &manager)
  : Window(manager)
  , _state(stateNormal)
{
}

void ButtonBase::SetState(State s)
{
	if( _state != s )
	{
		_state = s;
		OnChangeState(s);
	}
}

bool ButtonBase::OnPointerMove(float x, float y, PointerType pointerType, unsigned int pointerID)
{
	if( GetManager().HasCapturedPointers(this) )
	{
		if (GetManager().GetCapture(pointerID).get() == this)
		{
			bool push = x < GetWidth() && y < GetHeight() && x > 0 && y > 0;
			SetState(push ? statePushed : stateNormal);
		}
	}
	else
	{
		SetState(stateHottrack);
	}
	if( eventMouseMove )
		eventMouseMove(x, y);
	return true;
}

bool ButtonBase::OnPointerDown(float x, float y, int button, PointerType pointerType, unsigned int pointerID)
{
	if( !GetManager().HasCapturedPointers(this) && 1 == button ) // primary button only
	{
		GetManager().SetCapture(pointerID, shared_from_this());
		SetState(statePushed);
		if( eventMouseDown )
			eventMouseDown(x, y);
	}
	return true;
}

bool ButtonBase::OnPointerUp(float x, float y, int button, PointerType pointerType, unsigned int pointerID)
{
	if( GetManager().GetCapture(pointerID).get() == this && 1 == button )
	{
		GetManager().SetCapture(pointerID, nullptr);
		bool click = (GetState() == statePushed);
		if( eventMouseUp )
			eventMouseUp(x, y);
		if( click )
		{
			OnClick();
			if( eventClick )
				eventClick();
			if( GetEnabled() )  // handler may have disabled this button
				SetState(stateHottrack);
		}
	}
	return true;
}

bool ButtonBase::OnMouseLeave()
{
	SetState(stateNormal);
	return true;
}

bool ButtonBase::OnTap(float x, float y)
{
    if( !GetManager().HasCapturedPointers(this))
    {
        OnClick();
        if( eventClick )
            eventClick();
    }
    return true;
}

void ButtonBase::OnClick()
{
}

void ButtonBase::OnChangeState(State state)
{
}

void ButtonBase::OnEnabledChange(bool enable, bool inherited)
{
	if( enable )
	{
		SetState(stateNormal);
	}
	else
	{
		SetState(stateDisabled);
	}
}


///////////////////////////////////////////////////////////////////////////////
// button class implementation

std::shared_ptr<Button> Button::Create(Window *parent, TextureManager &texman, const std::string &text, float x, float y, float w, float h)
{
	auto res = std::make_shared<Button>(parent->GetManager(), texman);
	res->Move(x, y);
	res->SetText(text);
	if( w >= 0 && h >= 0 )
	{
		res->Resize(w, h);
	}

	parent->AddFront(res);

	return res;
}

Button::Button(LayoutManager &manager, TextureManager &texman)
  : ButtonBase(manager)
  , _font((size_t)-1)
  , _icon((size_t)-1)
{
	SetTexture(texman, "ui/button", true);
	SetFont(texman, "font_default");
	OnChangeState(stateNormal);
}

void Button::SetFont(TextureManager &texman, const char *fontName)
{
	_font = texman.FindSprite(fontName);
}

void Button::SetIcon(TextureManager &texman, const char *spriteName)
{
	_icon = spriteName ? texman.FindSprite(spriteName) : (size_t)-1;
}

void Button::OnChangeState(State state)
{
	SetFrame(state);
}

void Button::Draw(bool focused, bool enabled, vec2d size, DrawingContext &dc, TextureManager &texman) const
{
	ButtonBase::Draw(focused, enabled, size, dc, texman);

	SpriteColor c = 0;

	switch( GetState() )
	{
	case statePushed:
		c = 0xffffffff;
		break;
	case stateHottrack:
		c = 0xffffffff;
		break;
	case stateNormal:
		c = 0xffffffff;
		break;
	case stateDisabled:
		c = 0xbbbbbbbb;
		break;
	default:
		assert(false);
	}

	if (_icon != -1)
	{
		float iconHeight = texman.GetFrameHeight(_icon, 0);
		float textHeight = texman.GetFrameHeight(_font, 0);

		float x = size.x / 2;
		float y = (size.y - iconHeight - textHeight) / 2 + iconHeight;

		dc.DrawSprite(_icon, 0, c, x, y - iconHeight/2, vec2d(1, 0));
		dc.DrawBitmapText(x, y, _font, c, GetText(), alignTextCT);
	}
	else
	{
		float x = size.x / 2;
		float y = size.y / 2;
		dc.DrawBitmapText(x, y, _font, c, GetText(), alignTextCC);
	}
}


///////////////////////////////////////////////////////////////////////////////
// TextButton

TextButton::TextButton(LayoutManager &manager, TextureManager &texman)
  : ButtonBase(manager)
  , _fontTexture((size_t) -1)
  , _drawShadow(true)
{
	SetTexture(texman, nullptr, false);
	OnChangeState(stateNormal);
}

void TextButton::AlignSizeToContent()
{
	if( -1 != _fontTexture )
	{
		float w = GetManager().GetTextureManager().GetFrameWidth(_fontTexture, 0);
		float h = GetManager().GetTextureManager().GetFrameHeight(_fontTexture, 0);
		Resize((w - 1) * (float) GetText().length(), h + 1);
	}
}

void TextButton::SetDrawShadow(bool drawShadow)
{
	_drawShadow = drawShadow;
}

bool TextButton::GetDrawShadow() const
{
	return _drawShadow;
}

void TextButton::SetFont(const char *fontName)
{
	_fontTexture = GetManager().GetTextureManager().FindSprite(fontName);
	AlignSizeToContent();
}

void TextButton::OnTextChange()
{
	AlignSizeToContent();
}

void TextButton::Draw(bool focused, bool enabled, vec2d size, DrawingContext &dc, TextureManager &texman) const
{
	ButtonBase::Draw(focused, enabled, size, dc, texman);

	// grep 'enum State'
	SpriteColor colors[] =
	{
		SpriteColor(0xffffffff), // normal
		SpriteColor(0xffccccff), // hottrack
		SpriteColor(0xffccccff), // pushed
		SpriteColor(0xAAAAAAAA), // disabled
	};
	if( _drawShadow && stateDisabled != GetState() )
	{
		dc.DrawBitmapText(1, 1, _fontTexture, 0xff000000, GetText());
	}
	dc.DrawBitmapText(0, 0, _fontTexture, colors[GetState()], GetText());
}

///////////////////////////////////////////////////////////////////////////////

ImageButton::ImageButton(LayoutManager &manager)
  : ButtonBase(manager)
{
}

void ImageButton::OnChangeState(State state)
{
	SetFrame(state);
}

///////////////////////////////////////////////////////////////////////////////

std::shared_ptr<CheckBox> CheckBox::Create(Window *parent, TextureManager &texman, float x, float y, const std::string &text)
{
	auto res = std::make_shared<CheckBox>(parent->GetManager(), texman);
	res->Move(x, y);
	res->SetText(text);
	parent->AddFront(res);
	return res;
}

CheckBox::CheckBox(LayoutManager &manager, TextureManager &texman)
  : ButtonBase(manager)
  , _fontTexture(texman.FindSprite("font_small"))
  , _boxTexture(texman.FindSprite("ui/checkbox"))
  , _drawShadow(true)
  , _isChecked(false)
{
	SetTexture(texman, nullptr, false);
	AlignSizeToContent();
}

void CheckBox::AlignSizeToContent()
{
	const TextureManager &tm = GetManager().GetTextureManager();
	float th = tm.GetFrameHeight(_fontTexture, 0);
	float tw = tm.GetFrameWidth(_fontTexture, 0);
	float bh = tm.GetFrameHeight(_boxTexture, GetFrame());
	float bw = tm.GetFrameWidth(_boxTexture, GetFrame());
	Resize(bw + (tw - 1) * (float) GetText().length(), std::max(th + 1, bh));
}

void CheckBox::SetCheck(bool checked)
{
	_isChecked = checked;
	SetFrame(_isChecked ? GetState()+4 : GetState());
}

void CheckBox::OnClick()
{
	SetCheck(!GetCheck());
}

void CheckBox::OnTextChange()
{
	AlignSizeToContent();
}

void CheckBox::OnChangeState(State state)
{
	SetFrame(_isChecked ? state+4 : state);
}

void CheckBox::Draw(bool focused, bool enabled, vec2d size, DrawingContext &dc, TextureManager &texman) const
{
	ButtonBase::Draw(focused, enabled, size, dc, texman);

	float bh = texman.GetFrameHeight(_boxTexture, GetFrame());
	float bw = texman.GetFrameWidth(_boxTexture, GetFrame());
	float th = texman.GetFrameHeight(_fontTexture, 0);

	FRECT box = {0, (size.y - bh) / 2, bw, (size.y - bh) / 2 + bh};
	dc.DrawSprite(&box, _boxTexture, GetBackColor(), GetFrame());

	// grep 'enum State'
	SpriteColor colors[] =
	{
		SpriteColor(0xffffffff), // Normal
		SpriteColor(0xffffffff), // Hottrack
		SpriteColor(0xffffffff), // Pushed
		SpriteColor(0xffffffff), // Disabled
	};
	if( _drawShadow && stateDisabled != GetState() )
	{
		dc.DrawBitmapText(bw + 1, (size.y - th) / 2 + 1, _fontTexture, 0xff000000, GetText());
	}
	dc.DrawBitmapText(bw, (size.y - th) / 2, _fontTexture, colors[GetState()], GetText());
}
