#include "UITestDesktop.h"
#ifdef _WIN32
#include <fswin/FileSystemWin32.h>
using FileSystem = FS::FileSystemWin32;
#else
#include <fsposix/FileSystemPosix.h>
using FileSystem = FS::FileSystemPosix;
#endif // _WIN32
#include <platetc/UIInputRenderingController.h>
#include <platglfw/GlfwAppWindow.h>
#include <platglfw/Timer.h>
#include <video/TextureManager.h>
#include <ui/GuiManager.h>
#include <exception>
#include <string>
#include <iostream>

static void print_what(const std::exception &e, std::string prefix);

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <windef.h>
int APIENTRY WinMain( HINSTANCE, // hInstance
                      HINSTANCE, // hPrevInstance
                      LPSTR, // lpCmdLine
                      int) // nCmdShow
#else
int main(int, const char**)
#endif
try
{
	auto fs = std::make_shared<FileSystem>("data");

	GlfwAppWindow appWindow(
		"LibUI Test App",
		false, // fullscreen
		1024, // width
		768 // height
	);

	ImageCache imageCache;
	TextureManager textureManager;
	textureManager.LoadPackage(*fs, imageCache, ParseDirectory(*fs, "sprites"));

	auto desktop = std::make_shared<UITestDesktop>();

	Timer timer;
	timer.SetMaxDt(0.05f);
	timer.Start();

	UI::TimeStepManager timeStepManager;
	UIInputRenderingController controller(*fs, textureManager, timeStepManager, desktop);

	while (!appWindow.ShouldClose())
	{
		controller.OnRefresh(appWindow, appWindow.GetRender(), appWindow.GetRenderBinding());
		appWindow.Present();
		GlfwAppWindow::PollEvents(controller);
		controller.TimeStep(timer.GetDt(), appWindow.GetInput());
	}

	return 0;
}
catch (const std::runtime_error &e)
{
	print_what(/*s_logger,*/ e, "");
	return 1;
}

static void print_what(const std::exception &e, std::string prefix)
{
#ifdef _WIN32
	OutputDebugStringA((prefix + e.what() + "\n").c_str());
#endif
	try {
		std::rethrow_if_nested(e);
	}
	catch (const std::exception &nested) {
		print_what(nested, prefix + "> ");
	}
}

