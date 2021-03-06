#pragma once
#include "Texture.h"
#include "Window.h"
#include "video/RenderContext.h"

namespace UI
{
template<class T> struct LayoutData;
template<class T> struct RenderData;

class Text : public Window
{
public:
	void SetAlign(enumAlignText align) { _align = align; }
	void SetFont(Texture fontTexture) { _fontTexture = std::move(fontTexture); }
	void SetFontColor(std::shared_ptr<RenderData<SpriteColor>> color);
	void SetText(std::shared_ptr<LayoutData<std::string_view>> text);

	// Window
	void Draw(const DataContext &dc, const StateContext &sc, const LayoutContext &lc, const InputContext &ic, RenderContext &rc, TextureManager &texman, const Plat::Input &input, float time, bool hovered) const override;
	vec2d GetContentSize(TextureManager &texman, const DataContext &dc, float scale, const LayoutConstraints &layoutConstraints) const override;

protected:
	enumAlignText _align = alignTextLT;
	Texture _fontTexture = "font_small";
	std::shared_ptr<RenderData<SpriteColor>> _fontColor;
	std::shared_ptr<LayoutData<std::string_view>> _text;
};

class TextWithUnderline : public Text
{
public:
	void SetUnderline(std::shared_ptr<RenderData<bool>> underline) { _underline = underline; }
	void Draw(const DataContext& dc, const StateContext& sc, const LayoutContext& lc, const InputContext& ic, RenderContext& rc, TextureManager& texman, const Plat::Input &input, float time, bool hovered) const override;
private:
	Texture _underlineTexture = "ui/editsel";
	std::shared_ptr<RenderData<bool>> _underline;
};

} // namespace UI
