#pragma once
#include <ui/GuiManager.h>
#include <functional>

class AppState;
class AppController;
class ConfCache;
class LangCache;

namespace FS
{
	class FileSystem;
}

namespace UI
{
	class ConsoleBuffer;
}

class DesktopFactory : public UI::IWindowFactory
{
public:
	DesktopFactory(AppState &appState,
	               AppController &appController,
	               FS::FileSystem &fs,
	               ConfCache &conf,
	               LangCache &lang,
	               UI::ConsoleBuffer &logger);

	// UI::IWindowFactory
	std::shared_ptr<UI::Window> Create(UI::LayoutManager &manager, TextureManager &texman) override;

private:
	AppState &_appState;
	AppController &_appController;
	FS::FileSystem &_fs;
	ConfCache &_conf;
	LangCache &_lang;
	UI::ConsoleBuffer &_logger;
};
