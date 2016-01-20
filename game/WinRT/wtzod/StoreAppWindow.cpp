#include "pch.h"
#include "StoreAppWindow.h"
#include "DeviceResources.h"
#include "DisplayOrientation.h"
#include "WinStoreKeys.h"

#include <video/RenderBase.h>
#include <video/RenderD3D11.h>
#include <ui/GuiManager.h>
#include <ui/Window.h>

using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input;
using namespace Windows::System;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;
using namespace Windows::Devices::Input;

static ::DisplayOrientation DOFromDegrees(int degrees)
{
	switch (degrees)
	{
	default: assert(false);
	case 0: return DO_0;
	case 90: return DO_90;
	case 180: return DO_180;
	case 270: return DO_270;
	}
}

static float PixelsFromDips(float dips, float dpi)
{
	const float defaultDpi = 96.0f;
	return dips * dpi / defaultDpi;
}

static bool DispatchPointerMessage(UI::LayoutManager &inputSink, PointerEventArgs ^args, float dpi, UI::Msg msgHint)
{
	UI::PointerType pointerType;
	switch (args->CurrentPoint->PointerDevice->PointerDeviceType)
	{
	case PointerDeviceType::Mouse:
	case PointerDeviceType::Pen:
		pointerType = UI::PointerType::Mouse;
		break;

	case PointerDeviceType::Touch:
		pointerType = UI::PointerType::Touch;
		break;

	default:
		pointerType = UI::PointerType::Unknown;
		break;
	}

	int button;
	switch (args->CurrentPoint->Properties->PointerUpdateKind)
	{
	case PointerUpdateKind::LeftButtonPressed:
	case PointerUpdateKind::LeftButtonReleased:
		button = 1;
		break;

	case PointerUpdateKind::RightButtonPressed:
	case PointerUpdateKind::RightButtonReleased:
		button = 2;
		break;

	case PointerUpdateKind::MiddleButtonPressed:
	case PointerUpdateKind::MiddleButtonReleased:
		button = 3;
		break;

	default:
		button = 0;
		break;
	}

	UI::Msg msg;
	switch (args->CurrentPoint->Properties->PointerUpdateKind)
	{
	case PointerUpdateKind::LeftButtonPressed:
	case PointerUpdateKind::RightButtonPressed:
	case PointerUpdateKind::MiddleButtonPressed:
		msg = UI::Msg::PointerDown;
		break;

	case PointerUpdateKind::LeftButtonReleased:
	case PointerUpdateKind::RightButtonReleased:
	case PointerUpdateKind::MiddleButtonReleased:
		msg = UI::Msg::PointerUp;
		break;

	default:
		switch (msgHint)
		{
		case UI::Msg::MOUSEWHEEL:
		case UI::Msg::PointerMove:
			msg = msgHint;
			break;
		default:
			return false; // unknown button or something else we do not handle
		}
		break;
	}

	int delta = args->CurrentPoint->Properties->MouseWheelDelta;

	return inputSink.ProcessPointer(
		PixelsFromDips(args->CurrentPoint->Position.X, dpi),
		PixelsFromDips(args->CurrentPoint->Position.Y, dpi),
		(float)delta / 120.f,
		msg,
		button,
		pointerType,
		args->CurrentPoint->PointerId);
}

StoreAppWindow::StoreAppWindow(CoreWindow^ coreWindow, DX::DeviceResources &deviceResources, DX::SwapChainResources &swapChainResources)
	: _gestureRecognizer(ref new GestureRecognizer())
	, _displayInformation(DisplayInformation::GetForCurrentView())
	, _coreWindow(coreWindow)
	, _deviceResources(deviceResources)
	, _input(coreWindow)
	, _render(RenderCreateD3D11(deviceResources.GetD3DDeviceContext(), nullptr/*swapChainResources.GetBackBufferRenderTargetView()*/))
	, _inputSink(std::make_shared<UI::LayoutManager*>())
{
	_render->SetDisplayOrientation(DOFromDegrees(ComputeDisplayRotation(_displayInformation->NativeOrientation, _displayInformation->CurrentOrientation)));
	_regOrientationChanged = _displayInformation->OrientationChanged += ref new TypedEventHandler<DisplayInformation^, Platform::Object^>(
		[this](DisplayInformation^ sender, Platform::Object^)
	{
		_render->SetDisplayOrientation(DOFromDegrees(ComputeDisplayRotation(sender->NativeOrientation, sender->CurrentOrientation)));
	});

	_regSizeChanged = _coreWindow->SizeChanged += ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(
		[inputSink = _inputSink, displayInformation = _displayInformation](CoreWindow^ sender, WindowSizeChangedEventArgs^ args)
	{
		if (*inputSink)
		{
			float dpi = displayInformation->LogicalDpi;
			(*inputSink)->GetDesktop()->Resize(PixelsFromDips(sender->Bounds.Width, dpi), PixelsFromDips(sender->Bounds.Height, dpi));
		}
	});

	_regPointerMoved = _coreWindow->PointerMoved += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(
		[inputSink = _inputSink, displayInformation = _displayInformation, gestureRecognizer = _gestureRecognizer](CoreWindow^ sender, PointerEventArgs^ args)
	{
		gestureRecognizer->ProcessMoveEvents(args->GetIntermediatePoints());

		if (*inputSink)
		{
			args->Handled = DispatchPointerMessage(**inputSink, args, displayInformation->LogicalDpi, UI::Msg::PointerMove);
		}
	});

	_regPointerPressed = _coreWindow->PointerPressed += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(
		[inputSink = _inputSink, displayInformation = _displayInformation, gestureRecognizer = _gestureRecognizer](CoreWindow^ sender, PointerEventArgs^ args)
	{
		gestureRecognizer->ProcessDownEvent(args->CurrentPoint);

		if (*inputSink)
		{
			args->Handled = DispatchPointerMessage(**inputSink, args, displayInformation->LogicalDpi, UI::Msg::PointerDown);
		}
	});

	_regPointerReleased = _coreWindow->PointerReleased += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(
		[inputSink = _inputSink, displayInformation = _displayInformation, gestureRecognizer = _gestureRecognizer](CoreWindow^ sender, PointerEventArgs^ args)
	{
		gestureRecognizer->ProcessUpEvent(args->CurrentPoint);
		gestureRecognizer->CompleteGesture();

		if (*inputSink)
		{
			args->Handled = DispatchPointerMessage(**inputSink, args, displayInformation->LogicalDpi, UI::Msg::PointerUp);
		}
	});

	_coreWindow->PointerWheelChanged += ref new TypedEventHandler<CoreWindow ^, PointerEventArgs ^>(
		[inputSink = _inputSink, displayInformation = _displayInformation, gestureRecognizer = _gestureRecognizer](CoreWindow^ sender, PointerEventArgs^ args)
	{
		gestureRecognizer->ProcessMouseWheelEvent(args->CurrentPoint,
			(args->KeyModifiers & VirtualKeyModifiers::Shift) != VirtualKeyModifiers::None,
			(args->KeyModifiers & VirtualKeyModifiers::Control) != VirtualKeyModifiers::None);

		if (*inputSink)
		{
			args->Handled = DispatchPointerMessage(**inputSink, args, displayInformation->LogicalDpi, UI::Msg::MOUSEWHEEL);
		}
	});

	_gestureRecognizer->GestureSettings = GestureSettings::Tap;
	_gestureRecognizer->Tapped += ref new TypedEventHandler<GestureRecognizer ^, TappedEventArgs ^>(
		[inputSink = _inputSink, displayInformation = _displayInformation](GestureRecognizer ^sender, TappedEventArgs ^args)
	{
		if (*inputSink)
		{
			float dpi = displayInformation->LogicalDpi;
			unsigned int pointerID = 111; // should be unique enough :)
			(*inputSink)->ProcessPointer(
				PixelsFromDips(args->Position.X, dpi),
				PixelsFromDips(args->Position.Y, dpi),
				0, // z delta
				UI::Msg::TAP,
				1,
				UI::PointerType::Touch,
				pointerID);
		}
	});

	_regKeyDown = _coreWindow->KeyDown += ref new TypedEventHandler<CoreWindow ^, KeyEventArgs ^>(
		[inputSink = _inputSink](CoreWindow^ sender, KeyEventArgs^ args)
	{
		if (*inputSink)
		{
			args->Handled = (*inputSink)->ProcessKeys(UI::Msg::KEYDOWN, MapWinStoreKeyCode(args->VirtualKey, args->KeyStatus.IsExtendedKey));
		}
	});

	_regKeyUp = _coreWindow->KeyUp += ref new TypedEventHandler<CoreWindow ^, KeyEventArgs ^>(
		[inputSink = _inputSink](CoreWindow^ sender, KeyEventArgs^ args)
	{
		if (*inputSink)
		{
			args->Handled = (*inputSink)->ProcessKeys(UI::Msg::KEYUP, MapWinStoreKeyCode(args->VirtualKey, args->KeyStatus.IsExtendedKey));
		}
	});

	_regCharacterReceived = _coreWindow->CharacterReceived += ref new TypedEventHandler<CoreWindow ^, CharacterReceivedEventArgs ^>(
		[inputSink = _inputSink](CoreWindow^ sender, CharacterReceivedEventArgs^ args)
	{
		if (*inputSink)
		{
			args->Handled = (*inputSink)->ProcessText(args->KeyCode);
		}
	});
}

StoreAppWindow::~StoreAppWindow()
{
	_coreWindow->CharacterReceived -= _regCharacterReceived;
	_coreWindow->KeyUp -= _regKeyUp;
	_coreWindow->KeyDown -= _regKeyDown;
	_coreWindow->PointerReleased -= _regPointerReleased;
	_coreWindow->PointerPressed -= _regPointerMoved;
	_coreWindow->PointerMoved -= _regPointerMoved;
	_coreWindow->SizeChanged -= _regSizeChanged;

	_displayInformation->OrientationChanged -= _regOrientationChanged;

	// Events may still fire after the event handler is unregistered.
	// Remove the sink so that handlers could no-op.
	*_inputSink = nullptr;
}

UI::IClipboard& StoreAppWindow::GetClipboard()
{
	return _clipboard;
}

UI::IInput& StoreAppWindow::GetInput()
{
	return _input;
}

IRender& StoreAppWindow::GetRender()
{
	return *_render;
}

unsigned int StoreAppWindow::GetPixelWidth()
{
	float dpi = _displayInformation->LogicalDpi;
	return (unsigned int)PixelsFromDips(_coreWindow->Bounds.Width, dpi);
}

unsigned int StoreAppWindow::GetPixelHeight()
{
	float dpi = _displayInformation->LogicalDpi;
	return (unsigned int)PixelsFromDips(_coreWindow->Bounds.Height, dpi);
}

void StoreAppWindow::SetInputSink(UI::LayoutManager *inputSink)
{
	*_inputSink = inputSink;
}

