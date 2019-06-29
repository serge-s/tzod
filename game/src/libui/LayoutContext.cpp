#include "inc/ui/LayoutContext.h"
#include "inc/ui/Window.h"

using namespace UI;

LayoutContext::LayoutContext(float opacity, float scale, vec2d pxOffset, vec2d pxSize, bool enabled, bool focused)
	: _pxOffsetCombined(pxOffset)
	, _pxSize(pxSize)
	, _scaleCombined(scale)
	, _opacityCombined(opacity)
	, _enabledCombined(enabled)
	, _focusedCombined(focused)
{
}

LayoutContext::LayoutContext(const InputContext& ic, const Window &parentWindow, const LayoutContext &parentLC, const Window &childWindow, const WindowLayout &childLayout)
	: _pxOffsetCombined(parentLC.GetPixelOffsetCombined() + Offset(childLayout.rect))
	, _pxSize(Size(childLayout.rect))
	, _scaleCombined(parentLC.GetScaleCombined())
	, _opacityCombined(parentLC.GetOpacityCombined() * childLayout.opacity)
	, _enabledCombined(parentLC.GetEnabledCombined() && childLayout.enabled)
	, _focusedCombined(parentLC.GetFocusedCombined() && (parentWindow.GetFocus() == &childWindow))
{
}
