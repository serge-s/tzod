// Scroll.h

#pragma once

#include "Window.h"

namespace UI
{

class ImageButton;

class ScrollBarBase : public Window
{
public:
	ScrollBarBase(LayoutManager &manager);

	void SetShowButtons(bool showButtons);
	bool GetShowButtons() const;

	virtual void SetSize(float size) = 0;
	virtual float GetSize() const = 0;

	virtual void SetPos(float pos);
	float GetPos() const;

	void  SetDocumentSize(float limit);
	float GetDocumentSize() const;

	void  SetLineSize(float ls);
	float GetLineSize() const;

	void  SetPageSize(float ps);
	float GetPageSize() const;

	void SetElementTextures(TextureManager &texman, const char *slider, const char *upleft, const char *downright);

	std::function<void(float)> eventScroll;

protected:
	virtual void OnEnabledChange(bool enable, bool inherited);
	virtual float Select(float x, float y) const = 0;
	float GetScrollPaneLength() const;

	float _tmpBoxPos;
	std::shared_ptr<ImageButton> _btnBox;
	std::shared_ptr<ImageButton> _btnUpLeft;
	std::shared_ptr<ImageButton> _btnDownRight;

private:
	virtual void OnSize(float width, float height);

	void OnBoxMouseDown(float x, float y);
	void OnBoxMouseUp(float x, float y);
	void OnBoxMouseMove(float x, float y);

	void OnUpLeft();
	void OnDownRight();

	void OnLimitsChanged();

	float _pos;
	float _lineSize;
	float _pageSize;
	float _documentSize;

	bool _showButtons;
};

class ScrollBarVertical final : public ScrollBarBase
{
public:
	ScrollBarVertical(LayoutManager &manager, TextureManager &texman);

	void SetSize(float size) override;
	float GetSize() const override;

	void SetPos(float pos) override;

protected:
	float Select(float x, float y) const override { return y; }
};

class ScrollBarHorizontal final : public ScrollBarBase
{
public:
	ScrollBarHorizontal(LayoutManager &manager, TextureManager &texman);

	void SetSize(float size) override;
	float GetSize() const override;

	void SetPos(float pos) override;

private:
	float Select(float x, float y) const override { return x; }
};

} // namespace UI
