#pragma once
#include <ui/Window.h>
#include <functional>

namespace FS
{
	class FileSystem;
}

namespace UI
{
	class ConsoleBuffer;
	class Text;
}

class ConfCache;
class GetFileNameDlg;

struct MainMenuCommands
{
	std::function<void()> newCampaign;
	std::function<void()> newDM;
	std::function<void()> newMap;
	std::function<void(std::string)> openMap;
	std::function<void(std::string)> exportMap;
	std::function<void()> exit;
};

class MainMenuDlg : public UI::Window
{
	void OnSinglePlayer();
	void OnSaveGame();
	void OnSaveGameSelect(int result);
	void OnLoadGame();
	void OnLoadGameSelect(int result);

	void OnMultiPlayer();
	void OnHost();
	void OnJoin();
	void OnInternet();
	void OnNetworkProfile();

	void OnEditor();
	void OnMapSettings();
	void OnImportMap();
	void OnImportMapSelect(int result);
	void OnExportMap();
	void OnExportMapSelect(int result);

	void OnSettings();


	enum PanelType
	{
		PT_NONE,
		PT_SINGLEPLAYER,
		PT_MULTIPLAYER,
		PT_EDITOR,
	};

	enum PanelState
	{
		PS_NONE,
		PS_APPEARING,
		PS_DISAPPEARING,
	};

	UI::Window    *_panel = nullptr;
	UI::Window    *_panelFrame = nullptr;
	UI::Text      *_panelTitle = nullptr;
	PanelType  _ptype;
	PanelState _pstate;

	GetFileNameDlg *_fileDlg;
	FS::FileSystem &_fs;
	ConfCache &_conf;
	UI::ConsoleBuffer &_logger;
	MainMenuCommands _commands;

public:
	MainMenuDlg(Window *parent,
	            FS::FileSystem &fs,
	            ConfCache &conf,
	            UI::ConsoleBuffer &logger,
	            MainMenuCommands commands);
	virtual ~MainMenuDlg();
	virtual void OnParentSize(float width, float height) override;
	virtual bool OnRawChar(int c) override;
	virtual bool OnFocus(bool) override { return true; }

protected:
	void OnTimeStep(float dt);
	void OnCloseChild(int result);
	void CreatePanel(); // create panel of current _ptype and go to PS_APPEARING state
	void SwitchPanel(PanelType newtype);
};