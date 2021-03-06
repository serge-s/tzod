#pragma once
#include "Rectangle.h"

namespace UI
{

class Button;

class ScrollBarBase : public WindowContainer
{
public:
	ScrollBarBase();

	void SetShowButtons(bool showButtons);
	bool GetShowButtons() const;

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

	// Window
	WindowLayout GetChildLayout(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const override;

protected:
	virtual float Select(float x, float y) const = 0;
	float GetScrollPaneLength(const LayoutContext &lc) const;

	float _tmpBoxPos;
	std::shared_ptr<Rectangle> _background;
	std::shared_ptr<Button> _btnBox;
	std::shared_ptr<Button> _btnUpLeft;
	std::shared_ptr<Button> _btnDownRight;

private:
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
	explicit ScrollBarVertical(TextureManager &texman);

	// Window
	WindowLayout GetChildLayout(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const override;

protected:
	float Select(float x, float y) const override { return y; }
};

class ScrollBarHorizontal final : public ScrollBarBase
{
public:
	explicit ScrollBarHorizontal(TextureManager &texman);

	// Window
	WindowLayout GetChildLayout(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const override;

private:
	float Select(float x, float y) const override { return x; }
};

} // namespace UI
