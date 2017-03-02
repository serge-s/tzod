#include "MainMenu.h"
#include "ConfigBinding.h"
#include <loc/Language.h>
#include <ui/Button.h>

static const float c_buttonWidth = 200;
static const float c_buttonHeight = 50;


MainMenuDlg::MainMenuDlg(TextureManager &texman,
                         LangCache &lang,
                         MainMenuCommands commands)
  : _lang(lang)
  , _commands(std::move(commands))
{
	SetFlowDirection(UI::FlowDirection::Vertical);
	SetSpacing(20);

	std::shared_ptr<UI::Button> button;

	button = std::make_shared<UI::Button>(texman);
	button->SetFont("font_default");
	button->SetText(ConfBind(_lang.single_player_btn));
	button->Resize(c_buttonWidth, c_buttonHeight);
	button->eventClick = _commands.newDM;
	AddFront(button);

	button = std::make_shared<UI::Button>(texman);
	button->SetFont("font_default");
	button->SetText(ConfBind(_lang.editor_btn));
	button->Resize(c_buttonWidth, c_buttonHeight);
	button->eventClick = _commands.openMap;
	AddFront(button);

	button = std::make_shared<UI::Button>(texman);
	button->SetFont("font_default");
	button->SetText(ConfBind(_lang.settings_btn));
	button->Resize(c_buttonWidth, c_buttonHeight);
	button->eventClick = _commands.gameSettings;
	AddFront(button);
}

