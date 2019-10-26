#pragma once
#include <ui/ScrollView.h>

namespace FS
{
	class FileSystem;
}

namespace UI
{
	class ScanlineLayout;
}

class ShellConfig;
class LangCache;
class MapCollection;
class WorldCache;
class WorldView;

class SelectMapDlg final
	: public UI::ScrollView
{
public:
	SelectMapDlg(WorldView &worldView, FS::FileSystem &fsRoot, ShellConfig &conf, LangCache &lang, WorldCache &worldCache, MapCollection &mapCollection);

	std::function<void(unsigned int)> eventMapSelected;

private:
	WorldView &_worldView;
	ShellConfig &_conf;
	WorldCache &_worldCache;
	MapCollection &_mapCollection;
	std::shared_ptr<UI::ScanlineLayout> _mapTiles;
};