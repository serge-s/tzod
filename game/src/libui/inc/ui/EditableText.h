#pragma once
#include "Window.h"

class TextureManager;

namespace UI
{
	class EditableText
		: public Window
		, private PointerSink
		, private KeyboardSink
		, private TextSink
	{
	public:
		EditableText(LayoutManager &manager, TextureManager &texman);

		int GetTextLength() const;

		const std::string& GetText() const;
		void SetText(TextureManager &texman, const std::string &text);

		void SetInt(int value);
		int  GetInt() const;

		void  SetFloat(float value);
		float GetFloat() const;

		void SetSel(int begin, int end, LayoutContext *optionalLC = nullptr); // -1 means end of string
		int GetSelStart() const;
		int GetSelEnd() const;
		int GetSelMin() const;
		int GetSelMax() const;
		int GetSelLength() const;

		void Paste(TextureManager &texman, InputContext &ic);
		void Copy(InputContext &ic) const;

		std::function<void()> eventChange;

		// Window
		void Draw(const StateContext &sc, const LayoutContext &lc, const InputContext &ic, DrawingContext &dc, TextureManager &texman) const override;
		PointerSink* GetPointerSink() override { return this; }
		KeyboardSink *GetKeyboardSink() override;
		TextSink* GetTextSink() override { return this; }
		vec2d GetContentSize(TextureManager &texman, const StateContext &sc, float scale) const override;

	private:
		std::string _text;
		int   _selStart;
		int   _selEnd;
		int   _offset;
		float _lastCursortime;
		size_t _font;
		size_t _cursor;
		size_t _selection;

		void OnTextChange(TextureManager &texman);
		int HitTest(TextureManager &texman, vec2d px, float scale) const;

		// TextSink
		bool OnChar(int c) override;

		// KeyboardSink
		bool OnKeyPressed(InputContext &ic, Key key) override;

		// PointerSink
		bool OnPointerDown(InputContext &ic, LayoutContext &lc, TextureManager &texman, vec2d pointerPosition, int button, PointerType pointerType, unsigned int pointerID) override;
		void OnPointerMove(InputContext &ic, LayoutContext &lc, TextureManager &texman, vec2d pointerPosition, PointerType pointerType, unsigned int pointerID, bool captured) override;
		void OnTap(InputContext &ic, LayoutContext &lc, TextureManager &texman, vec2d pointerPosition) override;
	};

} // namespace UI