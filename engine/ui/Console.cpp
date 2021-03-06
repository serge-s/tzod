#include "inc/ui/Console.h"
#include "inc/ui/Edit.h"
#include "inc/ui/EditableText.h"
#include "inc/ui/ScrollBar.h"
#include "inc/ui/Rectangle.h"
#include "inc/ui/GuiManager.h"
#include "inc/ui/LayoutContext.h"
#include <plat/ConsoleBuffer.h>
#include <plat/Keys.h>
#include <video/TextureManager.h>
#include <video/RenderContext.h>
#include <algorithm>

using namespace UI;

ConsoleHistoryDefault::ConsoleHistoryDefault(size_t maxSize)
	: _maxSize(maxSize)
{
}

void ConsoleHistoryDefault::Enter(std::string str)
{
	_buf.push_back(std::move(str));
	if( _buf.size() > _maxSize )
	{
		_buf.pop_front();
	}
}

size_t ConsoleHistoryDefault::GetItemCount() const
{
	return _buf.size();
}

std::string_view ConsoleHistoryDefault::GetItem(size_t index) const
{
	return _buf[index];
}

///////////////////////////////////////////////////////////////////////////////

Console::Console(TimeStepManager &manager, TextureManager &texman)
  : TimeStepping(manager)
  , _cmdIndex(0)
  , _font(texman.FindSprite("font_small"))
  , _buf(nullptr)
  , _history(nullptr)
  , _echo(true)
  , _autoScroll(true)
{
	_background = std::make_shared<Rectangle>();
	_background->SetTexture("ui/console");
	_background->SetDrawBorder(true);

	_input = std::make_shared<Edit>();
	AddFront(_input);

	_scroll = std::make_shared<ScrollBarVertical>(texman);
	_scroll->eventScroll = std::bind(&Console::OnScrollBar, this, std::placeholders::_1);
	AddFront(_scroll);
	SetTimeStep(true); // FIXME: workaround
}

void Console::SetColors(const SpriteColor *colors, size_t count)
{
	_colors.assign(colors, colors + count);
}

void Console::SetHistory(IConsoleHistory *history)
{
	_history = history;
	_cmdIndex = _history ? _history->GetItemCount() : 0;
}

void Console::SetBuffer(Plat::ConsoleBuffer *buf)
{
	_buf = buf;
	_scroll->SetDocumentSize(_buf ? (float) _buf->GetLineCount() + _scroll->GetPageSize() : 0);
}

void Console::SetEcho(bool echo)
{
	_echo = echo;
}

bool Console::OnKeyPressed(const Plat::Input &input, const InputContext &ic, Plat::Key key)
{
	switch(key)
	{
	case Plat::Key::Up:
//		if( GetAsyncKeyState(VK_CONTROL) & 0x8000 ) // FIXME: workaround
//		{
//			_scroll->SetPos(_scroll->GetPos() - 1);
//			_autoScroll = _scroll->GetPos() + _scroll->GetPageSize() >= _scroll->GetDocumentSize();
//		}
//		else
		if( _history )
		{
			_cmdIndex = std::min(_cmdIndex, _history->GetItemCount());
			if( _cmdIndex > 0 )
			{
				--_cmdIndex;
				_input->GetEditable().SetText(std::string(_history->GetItem(_cmdIndex)));
			}
		}
		break;
	case Plat::Key::Down:
//		if( GetAsyncKeyState(VK_CONTROL) & 0x8000 ) // FIXME: workaround
//		{
//			_scroll->SetPos(_scroll->GetPos() + 1);
//			_autoScroll = _scroll->GetPos() + _scroll->GetPageSize() >= _scroll->GetDocumentSize();
//		}
//		else
		if( _history )
		{
			++_cmdIndex;
			if( _cmdIndex < _history->GetItemCount() )
			{
				_input->GetEditable().SetText(std::string(_history->GetItem(_cmdIndex)));
			}
			else
			{
				_input->GetEditable().SetText(std::string());
				_cmdIndex = _history->GetItemCount();
			}
		}
		break;
	case Plat::Key::Enter:
	{
		auto cmd = _input->GetEditable().GetText();
		if( cmd.empty() )
		{
			_buf->WriteLine(0, std::string(">"));
		}
		else
		{
			if( _history )
			{
				if( _history->GetItemCount() == 0 || cmd != _history->GetItem(_history->GetItemCount() - 1) )
				{
					_history->Enter(std::string(cmd));
				}
				_cmdIndex = _history->GetItemCount();
			}

			if( _echo )
			{
				_buf->Format(0) << "> " << cmd;       // echo to the console
			}
			if( eventOnSendCommand )
				eventOnSendCommand(cmd);
			_input->GetEditable().SetText(std::string());
		}
		_scroll->SetPos(_scroll->GetDocumentSize());
		_autoScroll = true;
		break;
	}
	case Plat::Key::PageUp:
		_scroll->SetPos(_scroll->GetPos() - _scroll->GetPageSize());
		_autoScroll = _scroll->GetPos() + _scroll->GetPageSize() >= _scroll->GetDocumentSize();
		break;
	case Plat::Key::PageDown:
		_scroll->SetPos(_scroll->GetPos() + _scroll->GetPageSize());
		_autoScroll = _scroll->GetPos() + _scroll->GetPageSize() >= _scroll->GetDocumentSize();
		break;
//	case Plat::Key::Home:
//		break;
//	case Plat::Key::End:
//		break;
	case Plat::Key::Escape:
		if( _input->GetEditable().GetText().empty() )
			return false;
		_input->GetEditable().SetText(std::string());
		break;
	case Plat::Key::Tab:
		if( eventOnRequestCompleteCommand )
		{
			std::string result;
			int pos = _input->GetEditable().GetSelEnd();
			if( eventOnRequestCompleteCommand(_input->GetEditable().GetText(), pos, result) )
			{
				_input->GetEditable().SetText(result);
				_input->GetEditable().SetSel(pos, pos);
			}
			_scroll->SetPos(_scroll->GetDocumentSize());
			_autoScroll = true;
		}
		break;
	default:
		return false;
	}
	return true;
}

void Console::OnScroll(TextureManager &texman, const InputContext &ic, const LayoutContext &lc, const DataContext &dc, vec2d scrollOffset, bool precise)
{
	_scroll->SetPos(_scroll->GetPos() - scrollOffset.y * 3);
	_autoScroll = _scroll->GetPos() + _scroll->GetPageSize() >= _scroll->GetDocumentSize();
}

void Console::OnTimeStep(const Plat::Input &input, bool focused, float dt)
{
	// FIXME: workaround
	_scroll->SetLineSize(1);
	_scroll->SetPageSize(20); // textAreaHeight / _texman.GetFrameHeight(_font, 0));
	_scroll->SetDocumentSize(_buf ? (float) _buf->GetLineCount() + _scroll->GetPageSize() - 1 : 0);
	if( _autoScroll )
		_scroll->SetPos(_scroll->GetDocumentSize());
}

void Console::Draw(const DataContext &dc, const StateContext &sc, const LayoutContext &lc, const InputContext &ic, RenderContext &rc, TextureManager &texman, const Plat::Input &input, float time, bool hovered) const
{
	_background->Draw(dc, sc, lc, ic, rc, texman, input, time, hovered);

	if( _buf )
	{
		_buf->Lock();

		FRECT inputRect = GetChildLayout(texman, lc, dc, *_input).rect;
		float textAreaHeight = inputRect.top;

		float h = std::floor(texman.GetFrameHeight(_font, 0) * lc.GetScaleCombined());
		size_t visibleLineCount = size_t(textAreaHeight / h);
		size_t scroll  = std::min(size_t(_scroll->GetDocumentSize() - _scroll->GetPos() - _scroll->GetPageSize()), _buf->GetLineCount());
		size_t lineMax = _buf->GetLineCount() - scroll;
		size_t count   = std::min(lineMax, visibleLineCount);

		float y = -fmod(textAreaHeight, h) + (float) (visibleLineCount - count) * h;

		for( size_t line = lineMax - count; line < lineMax; ++line )
		{
			unsigned int sev = _buf->GetSeverity(line);
			SpriteColor color = sev < _colors.size() ? _colors[sev] : 0xffffffff;
			rc.DrawBitmapText(vec2d{ 4, y }, lc.GetScaleCombined(), _font, color, _buf->GetLine(line));
			y += h;
		}

		_buf->Unlock();
	}
}

WindowLayout Console::GetChildLayout(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const
{
	if (_background.get() == &child)
	{
		return WindowLayout{ MakeRectWH(lc.GetPixelSize()), 1, true };
	}

	float inputHeight = _input->GetContentSize(texman, dc, lc.GetScaleCombined(), DefaultLayoutConstraints(lc)).y;
	if (_input.get() == &child)
	{
		return WindowLayout{ MakeRectRB(vec2d{ 0, lc.GetPixelSize().y - inputHeight }, lc.GetPixelSize()), 1, true };
	}
	if (_scroll.get() == &child)
	{
		float scrollWidth = std::floor(_scroll->GetWidth() * lc.GetScaleCombined());
		return WindowLayout{ MakeRectWH(vec2d{ lc.GetPixelSize().x - scrollWidth }, vec2d{ scrollWidth, lc.GetPixelSize().y - inputHeight }), 1, true };
	}

	assert(false);
	return {};
}

std::shared_ptr<const Window> Console::GetFocus(const std::shared_ptr<const Window>& owner) const
{
	return _input;
}

const Window* Console::GetFocus() const
{
	return _input.get();
}

void Console::OnScrollBar(float pos)
{
	_autoScroll = pos + _scroll->GetPageSize() >= _scroll->GetDocumentSize();
}

