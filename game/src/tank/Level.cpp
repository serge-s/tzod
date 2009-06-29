// Level.cpp
////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "pluto.h"


#include "Level.h"
#include "macros.h"
#include "functions.h"
#include "script.h"

#include "core/debug.h"
#include "core/Console.h"
#include "core/Application.h"

#include "config/Config.h"
#include "config/Language.h"

#include "network/TankClient.h"
#include "network/TankServer.h"

#include "video/RenderBase.h"
#include "video/TextureManager.h" // for ThemeManager

#include "fs/SaveFile.h"
#include "fs/MapFile.h"

#include "ui/GuiManager.h"
#include "ui/gui_desktop.h"
#include "ui/gui.h"
#include "ui/gui_widgets.h"

#include "gc/GameClasses.h"
#include "gc/RigidBodyDinamic.h"
#include "gc/Player.h"
#include "gc/Sound.h"
#include "gc/Camera.h"

//#ifdef _DEBUG
#include "gc/ai.h"
//#endif


////////////////////////////////////////////////////////////
unsigned long FieldCell::_sessionId;

void FieldCell::UpdateProperties()
{
	_prop = 0;
	for( int i = 0; i < _objCount; i++ )
	{
		assert(_ppObjects[i]->GetPassability() > 0);
		if( _ppObjects[i]->GetPassability() > _prop )
			_prop = _ppObjects[i]->GetPassability();
	}
}

void FieldCell::AddObject(GC_RigidBodyStatic *object)
{
	assert(object);
	assert(!object->IsKilled());
	assert(_objCount < 255);

#ifdef _DEBUG
	for( int i = 0; i < _objCount; ++i )
	{
		assert(object != _ppObjects[i]);
	}
#endif

	GC_RigidBodyStatic **tmp = new GC_RigidBodyStatic* [_objCount + 1];

	if( _ppObjects )
	{
		assert(_objCount > 0);
		memcpy(tmp, _ppObjects, sizeof(GC_RigidBodyStatic*) * _objCount);
		delete[] _ppObjects;
	}

	_ppObjects = tmp;
	_ppObjects[_objCount++] = object;

	UpdateProperties();
}

void FieldCell::RemoveObject(GC_RigidBodyStatic *object)
{
	assert(object);
	assert(_objCount > 0);

	if( 1 == _objCount )
	{
		assert(object == _ppObjects[0]);
		_objCount = 0;
		delete[] _ppObjects;
		_ppObjects = NULL;
	}
	else
	{
		GC_RigidBodyStatic **tmp = new GC_RigidBodyStatic* [_objCount - 1];
		int j = 0;
		for( int i = 0; i < _objCount; ++i )
		{
			if( object == _ppObjects[i] ) continue;
			tmp[j++] = _ppObjects[i];
		}
		_objCount--;
		assert(j == _objCount);
		delete[] _ppObjects;
		_ppObjects = tmp;
	}

	UpdateProperties();
}

////////////////////////////////////////////////////////////

Field::Field()
{
	_cells = NULL;
	_cx    = 0;
	_cy    = 0;

	_edgeCell._prop = 0xFF;
	_edgeCell._x    = -1;
	_edgeCell._y    = -1;
}

Field::~Field()
{
	Clear();
}

void Field::Clear()
{
	if( _cells )
	{
		for( int i = 0; i < _cy; i++ )
			delete[] _cells[i];
		delete[] _cells;
		//-----------
		_cells = NULL;
		_cx    = 0;
		_cy    = 0;
	}
	assert(0 == _cx && 0 == _cy);
}

void Field::Resize(int cx, int cy)
{
	assert(cx > 0 && cy > 0);
	Clear();
	_cx = cx;
	_cy = cy;
	_cells = new FieldCell* [_cy];
	for( int y = 0; y < _cy; y++ )
	{
		_cells[y] = new FieldCell[_cx];
		for( int x = 0; x < _cx; x++ )
		{
			(*this)(x, y)._x = x;
			(*this)(x, y)._y = y;
			if( 0 == x || 0 == y || _cx-1 == x || _cy-1 == y )
				(*this)(x, y)._prop = 0xFF;
		}
	}
	FieldCell::_sessionId = 0;
}

void Field::ProcessObject(GC_RigidBodyStatic *object, bool add)
{
	float r = object->GetRadius() / CELL_SIZE;
	vec2d p = object->GetPos() / CELL_SIZE;

	assert(r > 0);

	int xmin = __max(0,       int(p.x - r + 0.5f));
	int xmax = __min(_cx - 1, int(p.x + r + 0.5f));
	int ymin = __max(0,       int(p.y - r + 0.5f));
	int ymax = __min(_cy - 1, int(p.y + r + 0.5f));

	for( int x = xmin; x <= xmax; x++ )
	for( int y = ymin; y <= ymax; y++ )
	{
		if( add )
		{
			(*this)(x, y).AddObject(object);
		}
		else
		{
			(*this)(x, y).RemoveObject(object);
			if( 0 == x || 0 == y || _cx-1 == x || _cy-1 == y )
				(*this)(x, y)._prop = 0xFF;
		}
	}
}

#ifdef _DEBUG
FieldCell& Field::operator() (int x, int y)
{
	assert(NULL != _cells);
	return (x >= 0 && x < _cx && y >= 0 && y < _cy) ? _cells[y][x] : _edgeCell;
}

void Field::Dump()
{
	TRACE("==== Field dump ====\n");

	for( int y = 0; y < _cy; y++ )
	{
		char buf[1024] = {0};
		for( int x = 0; x < _cx; x++ )
		{
			switch( (*this)(x, y).Properties() )
			{
			case 0:
				strcat(buf, " ");
				break;
			case 1:
				strcat(buf, "-");
				break;
			case 0xFF:
				strcat(buf, "#");
				break;
			}
		}
		TRACE("%s\n", buf);
	}

	TRACE("=== end of dump ====\n");
}

#endif

////////////////////////////////////////////////////////////

// � ������������ ������ ��������� ������� �������
Level::Level()
  : _modeEditor(false)
  , _steps(0)
  , _pause(0)
  , _time(0)
  , _timeBuffer(0)
  , _dropedFrames(0)
  , _limitHit(false)
  , _frozen(false)
  , _safeMode(true)
  , _ctrlSentCount(0)
  , _locationsX(0)
  , _locationsY(0)
  , _sx(0)
  , _sy(0)
  , _seed(1)
  , _gameType(-1)
  , _serviceListener(NULL)
  , _texBack(g_texman->FindSprite("background"))
  , _texGrid(g_texman->FindSprite("grid"))
#ifdef NETWORK_DEBUG
  , _checksum(0)
  , _frame(0)
  , _dump(NULL)
#endif
{
	TRACE("Constructing the level\n");

	// register config handlers
	g_conf->s_volume->eventChange.bind(&Level::OnChangeSoundVolume, this);
	g_conf->sv_nightmode->eventChange.bind(&Level::OnChangeNightMode, this);
}

bool Level::IsEmpty() const
{
	return GetList(LIST_objects).empty();
}

void Level::Resize(int X, int Y)
{
	assert(IsEmpty());


	//
	// Resize
	//

	_locationsX  = (X * CELL_SIZE / LOCATION_SIZE + ((X * CELL_SIZE) % LOCATION_SIZE != 0 ? 1 : 0));
	_locationsY  = (Y * CELL_SIZE / LOCATION_SIZE + ((Y * CELL_SIZE) % LOCATION_SIZE != 0 ? 1 : 0));
	_sx           = (float) X * CELL_SIZE;
	_sy           = (float) Y * CELL_SIZE;

	for( int i = 0; i < Z_COUNT; i++ )
		z_grids[i].resize(_locationsX, _locationsY);

	grid_rigid_s.resize(_locationsX, _locationsY);
	grid_walls.resize(_locationsX, _locationsY);
	grid_wood.resize(_locationsX, _locationsY);
	grid_water.resize(_locationsX, _locationsY);
	grid_pickup.resize(_locationsX, _locationsY);

	_field.Resize(X + 1, Y + 1);


	//
	// FIXME: default objects
	//

	new GC_Camera(SafePtr<GC_Player>()); // no player
}

void Level::Clear()
{
	assert(IsSafeMode());

	if( g_gui )  // FIXME: dependence on GUI
		static_cast<UI::Desktop*>(g_gui->GetDesktop())->ShowEditor(false);

//	ObjectList::safe_iterator it = GetList(LIST_objects).safe_begin();
//	while( GetList(LIST_objects).end() != it )
	FOREACH_SAFE(GetList(LIST_objects), GC_Object, obj)
	{
//		GC_Object* obj = *it;
		assert(!obj->IsKilled());
		obj->Kill();
//		++it;
	}
	assert(IsEmpty());

	// reset variables
	_ctrlSentCount = 0;
	_modeEditor = false;
	_pause = 0;
	_time = 0;
	_timeBuffer = 0;
	_limitHit = false;
	_frozen = false;
	_gameType = -1;
#ifdef NETWORK_DEBUG
	_checksum = 0;
	_frame = 0;
	if( _dump )
	{
		fclose(_dump);
		_dump = NULL;
	}
#endif

	_dropedFrames = 0;
	_lag.clear();

	_abstractClient.Reset();
}

void Level::HitLimit()
{
	assert(!_limitHit);
	PauseLocal(true);
	_limitHit = true;
	PLAY(SND_Limit, vec2d(0,0));
}

bool Level::init_emptymap(int X, int Y)
{
	assert(IsSafeMode());
	assert(IsEmpty());

	Resize(X, Y);

	_gameType   = GT_EDITOR;

	_ThemeManager::Inst().ApplyTheme(0);

	ToggleEditorMode();
	assert(_modeEditor);

	g_conf->sv_nightmode->Set(false);

	return true;
}

bool Level::init_import_and_edit(const char *mapName)
{
	assert(IsSafeMode());
	assert(IsEmpty());

	_gameType = GT_EDITOR;
	g_conf->sv_nightmode->Set(false);

	if( !Import(mapName, false) )
		return false;

	ToggleEditorMode();
	assert(_modeEditor);

	return true;
}

bool Level::init_newdm(const string_t &mapName, unsigned long seed)
{
	assert(IsSafeMode());
	assert(IsEmpty());

	_gameType   = GT_DEATHMATCH;
	_modeEditor = false;
	_seed       = seed;

	return Import(mapName.c_str(), true);
}

bool Level::init_load(const char *fileName)
{
	assert(IsSafeMode());
	assert(IsEmpty());
	_modeEditor = false;
	return Unserialize(fileName);
}

Level::~Level()
{
	assert(IsSafeMode());
	TRACE("Destroying the level\n");

	Clear();

	// unregister config handlers
	g_conf->s_volume->eventChange.clear();
	g_conf->sv_nightmode->eventChange.clear();

	//-------------------------------------------
	assert(!g_env.nNeedCursor);
}

bool Level::Unserialize(const char *fileName)
{
	assert(IsEmpty());
	assert(IsSafeMode());

	TRACE("Loading saved game from file '%s'\n", fileName);

	SaveFile f;
	f._load = true;
	f._file = CreateFile(fileName,
	                     GENERIC_READ,
	                     0,
	                     NULL,
	                     OPEN_EXISTING,
	                     FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
	                     NULL);

	if( INVALID_HANDLE_VALUE == f._file )
	{
		TRACE("ERROR: couldn't open file\n");
		return false;
	}

	bool result = true;
	try
	{
		DWORD bytesRead = 0;

		SaveHeader sh = {0};
		ReadFile(f._file, &sh, sizeof(SaveHeader), &bytesRead, NULL);
		if( sizeof(SaveHeader) != bytesRead )
			throw "ERROR: unexpected end of file";

		if( VERSION != sh.dwVersion )
			throw "ERROR: invalid version";


		//
		// restoring lua user environment
		//
		struct ReadHelper
		{
			static const char* r(lua_State *L, void* data, size_t *sz)
			{
				static char buf[1];
				DWORD bytesRead = 0;
				ReadFile((HANDLE) data, buf, sizeof(buf), &bytesRead, NULL);
				*sz = bytesRead;
				return bytesRead ? buf : NULL;
			}
			static int read_user(lua_State *L)
			{
				void *ud = lua_touserdata(L, 1);
				lua_settop(L, 0);
				lua_newtable(g_env.L);       // permanent objects
				pluto_unpersist(L, &r, ud);
				lua_setglobal(L, "user");    // unpersisted object
				return 0;
			}
			static int read_queue(lua_State *L)
			{
				void *ud = lua_touserdata(L, 1);
				lua_settop(L, 0);
				lua_newtable(g_env.L);       // permanent objects
				pluto_unpersist(L, &r, ud);
				lua_getglobal(L, "pushcmd");
				assert(LUA_TFUNCTION == lua_type(L, -1));
				lua_pushvalue(L, -2);
				lua_setupvalue(L, -2, 1);    // unpersisted object
				return 0;
			}
		};
		if( lua_cpcall(g_env.L, &ReadHelper::read_user, f._file) )
		{
			const char *err = lua_tostring(g_env.L, -1);
			TRACE("%s\n", err);
			lua_pop(g_env.L, 1);
			throw "ERROR: pluto user";
		}
		if( lua_cpcall(g_env.L, &ReadHelper::read_queue, f._file) )
		{
			const char *err = lua_tostring(g_env.L, -1);
			TRACE("%s\n", err);
			lua_pop(g_env.L, 1);
			throw "ERROR: pluto queue";
		}


		_gameType = sh.dwGameType;

		g_conf->sv_timelimit->SetFloat(sh.timelimit);
		g_conf->sv_fraglimit->SetInt(sh.fraglimit);
		g_conf->sv_nightmode->Set(sh.nightmode);

		_time = sh.time;
		Resize(sh.width, sh.height);

		while( sh.nObjects > 0 )
		{
			GC_Object::CreateFromFile(f);
			sh.nObjects--;
		}

		// restore links
		if( !f.RestoreAllLinks() )
			throw "ERROR: invalid links";

		// apply the theme
		_infoTheme = sh.theme;
		_ThemeManager::Inst().ApplyTheme(_ThemeManager::Inst().FindTheme(sh.theme));

		// update skins
		FOREACH( GetList(LIST_players), GC_Player, pPlayer )
		{
			pPlayer->UpdateSkin();
		}

		GC_Camera::SwitchEditor();
	}
	catch (const char *msg)
	{
		TRACE("%s\n", msg);
		result = false;
		Clear();
	}

	CloseHandle(f._file);
	return result;
}

bool Level::Serialize(const char *fileName)
{
	assert(!IsEmpty());
	assert(IsSafeMode());

	TRACE("Saving game to file '%s'\n", fileName);

	DWORD bytesWritten = 0;

	SaveFile f;
	f._load = false;
	f._file = CreateFile(fileName,
	                     GENERIC_WRITE,
	                     0,
	                     NULL,
	                     CREATE_ALWAYS,
	                     FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
	                     NULL);
	if( INVALID_HANDLE_VALUE == f._file )
	{
		TRACE("ERROR: couldn't open file for writing\n");
		return false;
	}

	bool result = true;
    try
	{
		SaveHeader sh = {0};
		strcpy(sh.theme, _infoTheme.c_str());
		sh.dwVersion    = VERSION;
		sh.dwGameType   = _gameType;
		sh.fraglimit    = g_conf->sv_fraglimit->GetInt();
		sh.timelimit    = g_conf->sv_timelimit->GetFloat();
		sh.nightmode    = g_conf->sv_nightmode->Get();
		sh.time         = _time;
		sh.width        = (int) _sx / CELL_SIZE;
		sh.height       = (int) _sy / CELL_SIZE;
		sh.nObjects     = 0; // ����� ����������� �� ���� ������

		WriteFile(f._file, &sh, sizeof(SaveHeader), &bytesWritten, NULL);
		if( bytesWritten != sizeof(SaveHeader) )
			throw "ERROR: couldn't write file. check disk space";


		//
		// writing lua user environment
		//
		struct WriteHelper
		{
			static int w(lua_State *L, const void* p, size_t sz, void* ud)
			{
				DWORD written = 0;
				WriteFile((HANDLE) ud, p, sz, &written, NULL);
				return sz - written;
			}
			static int write_user(lua_State *L)
			{
				void *ud = lua_touserdata(L, 1);
				lua_settop(L, 0);
				lua_newtable(g_env.L);       // permanent objects
				lua_getglobal(L, "user");    // object to persist
				pluto_persist(L, &w, ud);
				return 0;
			}
			static int write_queue(lua_State *L)
			{
				void *ud = lua_touserdata(L, 1);
				lua_settop(L, 0);
				lua_newtable(g_env.L);       // permanent objects
				lua_getglobal(L, "pushcmd");
				assert(LUA_TFUNCTION == lua_type(L, -1));
				lua_getupvalue(L, -1, 1);    // object to persist
				lua_remove(L, -2);
				pluto_persist(L, &w, ud);
				return 0;
			}
		};
		if( lua_cpcall(g_env.L, &WriteHelper::write_user, f._file) )
		{
			const char *err = lua_tostring(g_env.L, -1);
			TRACE("%s\n", err);
			lua_pop(g_env.L, 1);
			throw "ERROR: pluto user";
		}
		if( lua_cpcall(g_env.L, &WriteHelper::write_queue, f._file) )
		{
			const char *err = lua_tostring(g_env.L, -1);
			TRACE("%s\n", err);
			lua_pop(g_env.L, 1);
			throw "ERROR: pluto queue";
		}


		//���������� ��� �������. ���� ����� - ���������
		ObjectList::reverse_iterator it = GetList(LIST_objects).rbegin();
		for( ; it != GetList(LIST_objects).rend(); ++it )
		{
			GC_Object *object = *it;
			if( object->IsSaved() )
			{
				ObjectType type = object->GetType();
				WriteFile(f._file, &type, sizeof(type), &bytesWritten, NULL);
				try
				{
					SafePtr<void> tmp;
					SetRawPtr(tmp, object);
					f.Serialize(tmp);
					SetRawPtr(tmp, NULL);

					object->Serialize(f);
				}
				catch (...)
				{
					throw "ERROR: serialize object failed\n";
				}
				sh.nObjects++;
			}
		}

		// return to begin of file and write count of saved objects
		SetFilePointer(f._file, 0, NULL, FILE_BEGIN);
		WriteFile(f._file, &sh, sizeof(SaveHeader), &bytesWritten, NULL);
		if( bytesWritten != sizeof(SaveHeader) )
			throw "ERROR: couldn't write file. check disk space";
	}
	catch(const char *msg)
	{
		TRACE("%s\n", msg);
		result = false;
	}
	CloseHandle(f._file);

	return result;
}

bool Level::Import(const char *fileName, bool execInitScript)
{
	assert(IsEmpty());
	assert(IsSafeMode());

	MapFile file;
	if( !file.Open(fileName, false) )
		return false;

	int width, height;
	if( !file.getMapAttribute("width", width) ||
		!file.getMapAttribute("height", height) )
	{
		return false;
	}

	file.getMapAttribute("theme", _infoTheme);
	_ThemeManager::Inst().ApplyTheme(_ThemeManager::Inst().FindTheme(_infoTheme));

	file.getMapAttribute("author",   _infoAuthor);
	file.getMapAttribute("desc",     _infoDesc);
	file.getMapAttribute("link-url", _infoUrl);
	file.getMapAttribute("e-mail",   _infoEmail);
	file.getMapAttribute("on_init",  _infoOnInit);

	Resize(width, height);

	while( file.NextObject() )
	{
		float x = 0;
		float y = 0;
		file.getObjectAttribute("x", x);
		file.getObjectAttribute("y", y);
		name2type::iterator it = get_n2t().find(file.GetCurrentClassName());
		if( get_n2t().end() == it ) continue;
		GC_Object *object = get_t2i()[it->second].Create(x, y);
		object->MapExchange(file);
	}
	GC_Camera::SwitchEditor();

	if( execInitScript && !script_exec(g_env.L, _infoOnInit.c_str()) )
	{
		Clear();
		return false;
	}

	return true;
}

bool Level::Export(const char *fileName)
{
	assert(!IsEmpty());
	assert(IsSafeMode());

	MapFile file;

	//
	// map info
	//

	file.setMapAttribute("type", "deathmatch");

	std::ostringstream str;
	str << VERSION;
	file.setMapAttribute("version", str.str());

	file.setMapAttribute("width",  (int) _sx / CELL_SIZE);
	file.setMapAttribute("height", (int) _sy / CELL_SIZE);

	file.setMapAttribute("author",   _infoAuthor);
	file.setMapAttribute("desc",     _infoDesc);
	file.setMapAttribute("link-url", _infoUrl);
	file.setMapAttribute("e-mail",   _infoEmail);

	file.setMapAttribute("theme",    _infoTheme);

	file.setMapAttribute("on_init",  _infoOnInit);

	//
	// objects
	//

	if( !file.Open(fileName, true) )
		return false;

	FOREACH( GetList(LIST_objects), GC_Object, object )
	{
		if( object->IsKilled() ) continue;
		if( IsRegistered(object->GetType()) )
		{
			file.BeginObject(GetTypeName(object->GetType()));
			object->MapExchange(file);
			if( !file.WriteCurrentObject() ) return false;
		}
	}

	return true;
}

void Level::PauseLocal(bool pause)
{
	if( pause )
	{
		if( 0 == _pause + g_env.pause )
		{
			PauseSound(true);
		}
		++_pause;
	}
	else
	{
		assert(_pause + g_env.pause > 0);
		--_pause;
		if( 0 == _pause + g_env.pause )
		{
			PauseSound(false);
		}
	}
}

void Level::PauseSound(bool pause)
{
	FOREACH( GetList(LIST_sounds), GC_Sound, pSound )
	{
		pSound->Freeze(pause);
	}
}

void Level::ToggleEditorMode()
{
	if( GT_INTRO == _gameType )
	{
		return;
	}

	if( _modeEditor )
	{
		_modeEditor = false;
		PauseLocal(false);
	}
	else
	{
		_modeEditor = true;
		PauseLocal(true);
	}
	GC_Camera::SwitchEditor();
}

GC_Object* Level::CreateObject(ObjectType type, float x, float y)
{
	assert(IsRegistered(type));
	return get_t2i()[type].Create(x, y);
}

GC_2dSprite* Level::PickEdObject(const vec2d &pt, int layer)
{
	for( int i = Z_COUNT; i--; )
	{
		PtrList<ObjectList> receive;
		z_grids[i].OverlapPoint(receive, pt / LOCATION_SIZE);

		PtrList<ObjectList>::iterator rit = receive.begin();
		for( ; rit != receive.end(); rit++ )
		{
			ObjectList::iterator it = (*rit)->begin();
			for( ; it != (*rit)->end(); ++it )
			{
				GC_2dSprite *object = static_cast<GC_2dSprite*>(*it);

				FRECT frect;
				object->GetGlobalRect(frect);

				if( PtInFRect(frect, pt) )
				{
					for( int i = 0; i < GetTypeCount(); ++i )
					{
						if( object->GetType() == GetTypeByIndex(i)
						    && (-1 == layer || GetTypeInfoByIndex(i).layer == layer) )
						{
							return object;
						}
					}
				}
			}
		}
	}

	return NULL;
}

int Level::net_rand()
{
	return ((_seed = _seed * 214013L + 2531011L) >> 16) & RAND_MAX;
}

float Level::net_frand(float max)
{
	return (float) net_rand() / (float) RAND_MAX * max;
}

vec2d Level::net_vrand(float len)
{
	return vec2d(net_frand(PI2)) * len;
}

void Level::CalcOutstrip( const vec2d &fp, // fire point
                          float vp,        // speed of the projectile
                          const vec2d &tx, // target position
                          const vec2d &tv, // target velocity
                          vec2d &out_fake) // out: fake target position
{
	float vt = tv.len();

	if( vt >= vp || vt < 1e-7 )
	{
		out_fake = tx;
	}
	else
	{
		float cg = tv.x / vt;
		float sg = tv.y / vt;

		float x   = (tx.x - fp.x) * cg + (tx.y - fp.y) * sg;
		float y   = (tx.y - fp.y) * cg - (tx.x - fp.x) * sg;
		float tmp = vp*vp - vt*vt;

		float fx = x + vt * (x*vt + sqrtf(x*x * vp*vp + y*y * tmp)) / tmp;

		out_fake.x = __max(0, __min(_sx, fp.x + fx*cg - y*sg));
		out_fake.y = __max(0, __min(_sy, fp.y + fx*sg + y*cg));
	}
}

GC_RigidBodyStatic* Level::agTrace( Grid<ObjectList> &list,
	                                const GC_RigidBodyStatic* ignore,
	                                const vec2d &x0,  // origin
	                                const vec2d &a,   // direction
	                                vec2d *ht,
	                                vec2d *norm ) const
{
	vec2d hit;
	float nx, ny;

	GC_RigidBodyStatic *pBestObject = NULL;
	float minLen = 1.0e8;


	//
	// overlap line
	//

	vec2d begin(x0), end(x0 + a), delta(a);
	begin /= LOCATION_SIZE;
	end   /= LOCATION_SIZE;
	delta /= LOCATION_SIZE;

	const int halfBeginX = int(floorf(begin.x - 0.5f));
	const int halfBeginY = int(floorf(begin.y - 0.5f));

	const int halfEndX = int(floorf(end.x - 0.5f));
	const int halfEndY = int(floorf(end.y - 0.5f));

	const int jitX[4] = {0,1,0,1};
	const int jitY[4] = {0,0,1,1};

	const int stepx = delta.x > 0 ? 2 : -2;
	const int stepy = delta.y > 0 ? 2 : -2;

	float p = delta.y * (begin.x - 0.5f) - delta.x * (begin.y - 0.5f);
	float tx = p - delta.y * (float) stepx;
	float ty = p + delta.x * (float) stepy;


	float la = a.len();
	float la_tmp = la / LOCATION_SIZE;

	for( int i = 0; i < 4; i++ )
	{
		int cx = halfBeginX + jitX[i];
		int cy = halfBeginY + jitY[i];

		int count = (abs(cx-halfEndX - (cx<halfEndX))>>1) + (abs(cy-halfEndY - (cy<halfEndY))>>1);
		assert(count >= 0);

		do
		{
			//
			// check current cell
			//
			if( cx >= 0 && cx < _locationsX && cy >= 0 && cy < _locationsY )
			{
				ObjectList &tmp_list = list.element(cx, cy);
				for( ObjectList::iterator it = tmp_list.begin(); it != tmp_list.end(); ++it )
				{
					GC_RigidBodyStatic *object = (GC_RigidBodyStatic *) *it;
					if( object->GetTrace0() || ignore == object
						|| object->CheckFlags(GC_FLAG_RBSTATIC_PHANTOM|GC_FLAG_OBJECT_KILLED) )
					{
						continue;
					}

					//�����
					bool bHasHit = true;
					// FIXME! TODO: estimate


					// �����
					if( bHasHit )
					{
						vec2d m[4];

						for( int i = 0; i < 4; ++i )
						{
							m[i] = object->GetVertex(i);
						}

						for( int n = 0; n < 4; ++n )
						{
							float xb = m[n].x;
							float yb = m[n].y;

							float bx = m[(n+1)&3].x - xb;
							float by = m[(n+1)&3].y - yb;

							float delta = a.y*bx - a.x*by;
							if( delta <= 0 ) continue;

							float tb = (a.x*(yb - x0.y) - a.y*(xb - x0.x)) / delta;

							if( tb <= 1 && tb >= 0 )
							{
								float len = (bx*(yb - x0.y) - by*(xb - x0.x)) / delta;

								if( len >= 0 && len < minLen )
								{
									minLen = len;
									pBestObject = object;
									nx =  by;
									ny = -bx;
									hit.Set(xb + bx * tb, yb + by * tb);
								}
							}
						}
					}
				}
			}


			// step to the next cell
			float t = delta.x * (float) cy - delta.y * (float) cx;
			if( fabsf(t + tx) < fabsf(t + ty) )
				cx += stepx;
			else
				cy += stepy;

		} while( count-- );
	}

	if( pBestObject )
	{
		if( ( ((x0.x <= hit.x) && (hit.x <= x0.x + a.x))
		      ||((x0.x + a.x <= hit.x) && (hit.x <= x0.x)) ) &&
		    ( ((x0.y <= hit.y) && (hit.y <= x0.y + a.y))
		      ||((x0.y + a.y <= hit.y) && (hit.y <= x0.y)) ) )
		{
			if( norm )
			{
				float l = sqrtf(nx*nx + ny*ny);
				norm->Set(nx / l, ny / l);
			}
			if( ht ) *ht = hit;
			return pBestObject;
		}
	}

	return NULL;
}

void Level::DrawBackground(size_t tex) const
{
	const LogicalTexture &lt = g_texman->Get(tex);
	g_render->TexBind(lt.dev_texture);

	MyVertex *v = g_render->DrawQuad();
	v[0].color = 0xffffffff;
	v[0].u = 0;
	v[0].v = 0;
	v[0].x = 0;
	v[0].y = 0;
	v[1].color = 0xffffffff;
	v[1].u = _sx / lt.pxFrameWidth;
	v[1].v = 0;
	v[1].x = _sx;
	v[1].y = 0;
	v[2].color = 0xffffffff;
	v[2].u = _sx / lt.pxFrameWidth;
	v[2].v = _sy / lt.pxFrameHeight;
	v[2].x = _sx;
	v[2].y = _sy;
	v[3].color = 0xffffffff;
	v[3].u = 0;
	v[3].v = _sy / lt.pxFrameHeight;
	v[3].x = 0;
	v[3].y = _sy;
}

/*
ControlPacket Level::GetControlPacket(GC_Object *player)
{
	ASSERT_TYPE(player, GC_Player);
	const ControlPacket &cp = *(_ctrlPtr++);
	if( MISC_YOUARETHELAST & cp.wControlState )
	{
		_lag = static_cast<GC_Player*>(player)->GetNick();
	}
	return cp;
}
*/

void Level::Step(const ControlPacketVector &ctrl, float dt)
{
	++_steps;

	_time        += dt;
	_timeBuffer  -= dt;
	_dropedFrames = __max(0, _dropedFrames - dt); // it's allowed one dropped frame per second

	if( !_frozen )
	{
		ControlPacketVector::const_iterator ctrlIt = ctrl.begin();
		FOREACH( GetList(LIST_players), GC_Player, p )
		{
			if( GC_PlayerHuman *ph = dynamic_cast<GC_PlayerHuman *>(p) )
			{
				VehicleState vs;
				(ctrlIt++)->tovs(vs);
				ph->SetControllerState(vs);
			}
		}
		assert(ctrlIt == ctrl.end());


//		_ctrlPtr = ctrl.begin();
		_safeMode = false;
		for( ObjectList::safe_iterator it = ts_fixed.safe_begin(); it != ts_fixed.end(); ++it )
		{
			assert(!(*it)->IsKilled());
			(*it)->TimeStepFixed(dt);
		}
		GC_RigidBodyDynamic::ProcessResponse(dt);
		_safeMode = true;
//		assert(_ctrlPtr == ctrl.end());
	}

	RunCmdQueue(dt);


	//
	// detect sync lost error
	//

#ifdef NETWORK_DEBUG
	if( g_client )
	{
		if( !_dump )
		{
			char fn[MAX_PATH];
			sprintf_s(fn, "network_dump_%u_%u.txt", GetTickCount(), GetCurrentProcessId());
			_dump = fopen(fn, "w");
			assert(_dump);
		}
		++_frame;
		fprintf(_dump, "\n### frame %04d ###\n", _frame);

		DWORD dwCheckSum = 0;
		for( ObjectList::safe_iterator it = ts_fixed.safe_begin(); it != ts_fixed.end(); ++it )
		{
			if( DWORD cs = (*it)->checksum() )
			{
				dwCheckSum = dwCheckSum ^ cs ^ 0xD202EF8D;
				dwCheckSum = (dwCheckSum >> 1) | ((dwCheckSum & 0x00000001) << 31);
				fprintf(_dump, "0x%08x -> local 0x%08x, global 0x%08x  (%s)\n", (*it), cs, dwCheckSum, typeid(**it).name());
			}
		}
		_checksum = dwCheckSum;
		fflush(_dump);
	}
#endif
}

void Level::TimeStep(float dt)
{
	FOREACH( GetList(LIST_cameras), GC_Camera, pCamera )
	{
		pCamera->HandleFreeMovement();
	}

	FOREACH_SAFE( GetList(LIST_sounds), GC_Sound, pSound )
	{
		pSound->KillWhenFinished();
	}


	if( g_env.pause + _pause > 0 /*&& !g_client*/ && _gameType != GT_INTRO || _limitHit )
		return;


	dt *= g_conf->sv_speed->GetFloat() / 100.0f;
	assert(dt >= 0);
	assert(!_modeEditor);


	//
	// apply dt filter
	//

	if( g_conf->cl_dtwindow->GetInt() > 1 )
	{
		_dt.push_back(dt);
		while( (signed) _dt.size() > g_conf->cl_dtwindow->GetInt() )
		{
			_dt.pop_front();
		}

		dt = 0;
		for( std::deque<float>::const_iterator it = _dt.begin(); it != _dt.end(); ++it )
		{
			dt += (*it);
		}
		dt /= (float) _dt.size();
	}

	float dt_fixed = g_client ? 1.0f / g_conf->sv_fps->GetFloat() : dt;


	if( _ctrlSentCount <= g_conf->cl_latency->GetInt() )
	{
		//
		// read controller state for local players
		//
		std::vector<VehicleState> ctrl;
		FOREACH( GetList(LIST_players), GC_Player, p )
		{
			if( GC_PlayerLocal *pl = dynamic_cast<GC_PlayerLocal *>(p) )
			{
				VehicleState vs;
				pl->ReadControllerStateAndStepPredicted(vs, dt_fixed);
				ctrl.push_back(vs);
			}
		}


		//
		// send ctrl
		//
		_abstractClient.Send(ctrl
#ifdef NETWORK_DEBUG
			, _checksum, _frame
#endif
		);
		++_ctrlSentCount;
	}


	_timeBuffer = std::min(_timeBuffer + dt * g_conf->cl_boost->GetFloat(), 2 / g_conf->sv_fps->GetFloat());

	if( _timeBuffer > 0 )
	{
		ControlPacketVector cpv;
		if( _abstractClient.Recv(cpv) )
		{
			Step(cpv, dt_fixed);
			--_ctrlSentCount;
		}
	}



	if( !_frozen )
	{
		_safeMode = false;
		ObjectList::safe_iterator it = ts_floating.safe_begin();
		while( it != ts_floating.end() )
		{
			assert(!(*it)->IsKilled());
			(*it)->TimeStepFloat(dt);
			++it;
		}
		_safeMode = true;
	}



#if 0
#ifdef _DEBUG
	// check for dead objects
	OBJECT_LIST::safe_iterator it = GetList(LIST_objects).safe_begin();
	while( GetList(LIST_objects).end() != it )
	{
		assert(!(*it)->IsKilled());
		++it;
	}
#endif
#endif

	if( g_conf->sv_timelimit->GetInt() && g_conf->sv_timelimit->GetInt() * 60 <= _time )
	{
		HitLimit();
	}

	static_cast<UI::Desktop*>(g_gui->GetDesktop())->GetOscilloscope()->Push(_timeBuffer);
	static_cast<UI::Desktop*>(g_gui->GetDesktop())->GetOscilloscope2()->Push(dt);
	static_cast<UI::Desktop*>(g_gui->GetDesktop())->GetOscilloscope3()->Push((float) _steps);
	static_cast<UI::Desktop*>(g_gui->GetDesktop())->GetOscilloscope4()->Push((float) _ctrlSentCount);

	_steps = 0;
}

void Level::RunCmdQueue(float dt)
{
	assert(_safeMode);

	lua_State * const L = g_env.L;

	lua_getglobal(L, "pushcmd");
	assert(LUA_TFUNCTION == lua_type(L, -1));
	lua_getupvalue(L, -1, 1);
	int queueidx = lua_gettop(L);

	for( lua_pushnil(L); lua_next(L, queueidx); lua_pop(L, 1) )
	{
		// -2 -> key; -1 -> value(table)

		lua_rawgeti(L, -1, 2);
		lua_Number time = lua_tonumber(L, -1) - dt;
		lua_pop(L, 1);

		if( time <= 0 )
		{
			// call function and remove it from queue
			lua_rawgeti(L, -1, 1);
			if( lua_pcall(L, 0, 0, 0) )
			{
				TRACE("%s\n", lua_tostring(g_env.L, -1));
				lua_pop(g_env.L, 1); // pop the error message
			}
			lua_pushvalue(L, -2); // push copy of the key
			lua_pushnil(L);
			lua_settable(L, queueidx);
		}
		else
		{
			// update time value
			lua_pushnumber(L, time);
			lua_rawseti(L, -2, 2);
		}
	}

	assert(lua_gettop(L) == queueidx);
	lua_pop(L, 2); // pop results of lua_getglobal and lua_getupvalue
}

void Level::Render() const
{
	//
	// ����������� �������� ����� � ����� ������������ �������
	//

	GC_Camera *pMaxShake = NULL;

	if( g_render->GetWidth() >= (int) _sx &&
		g_render->GetHeight() >= (int) _sy )
	{
		float max_shake = 0;
		FOREACH( GetList(LIST_cameras), GC_Camera, pCamera )
		{
			if( !pCamera->IsActive() ) continue;
			if( pCamera->GetShake() >= max_shake )
			{
				pMaxShake = pCamera;
				max_shake = pCamera->GetShake();
			}
		}
	}

	g_render->SetAmbient( g_conf->sv_nightmode->Get() ? 0.0f : 1.0f );

	int count = 0;
	FOREACH( GetList(LIST_cameras), GC_Camera, pCamera )
	{
		if( pMaxShake ) pCamera = pMaxShake;
		if( !pCamera->IsActive() ) continue;


		//
		// draw lights to alpha channel
		//

		g_render->SetMode(RM_LIGHT);
		pCamera->Select();
		if( g_conf->sv_nightmode->Get() )
		{
			float xmin = (float) __max(0, g_env.camera_x );
			float ymin = (float) __max(0, g_env.camera_y );
			float xmax = __min(_sx, (float) g_env.camera_x +
				(float) g_render->GetViewportWidth() / pCamera->_zoom );
			float ymax = __min(_sy, (float) g_env.camera_y +
				(float) g_render->GetViewportHeight() / pCamera->_zoom );

			FOREACH( GetList(LIST_lights), GC_Light, pLight )
			{
				assert(!pLight->IsKilled());
				if( pLight->IsActive() &&
					pLight->GetPos().x + pLight->GetRenderRadius() > xmin &&
					pLight->GetPos().x - pLight->GetRenderRadius() < xmax &&
					pLight->GetPos().y + pLight->GetRenderRadius() > ymin &&
					pLight->GetPos().y - pLight->GetRenderRadius() < ymax )
				{
					pLight->Shine();
				}
			}
		}


		//
		// draw world to rgb
		//

		g_render->SetMode(RM_WORLD);

		// background texture
		DrawBackground(_texBack);
		if( g_level->_modeEditor && g_conf->ed_drawgrid->Get() )
			DrawBackground(_texGrid);


		int xmin = __max(0, g_env.camera_x / LOCATION_SIZE);
		int ymin = __max(0, g_env.camera_y / LOCATION_SIZE);
		int xmax = __min(_locationsX - 1,
			(g_env.camera_x + int((float) g_render->GetViewportWidth() / pCamera->_zoom)) / LOCATION_SIZE);
		int ymax = __min(_locationsY - 1,
			(g_env.camera_y + int((float) g_render->GetViewportHeight() / pCamera->_zoom)) / LOCATION_SIZE + 1);

		for( int z = 0; z < Z_COUNT; ++z )
		{
			for( int x = xmin; x <= xmax; ++x )
			for( int y = ymin; y <= ymax; ++y )
			{
				// FIXME: using of global g_level
				FOREACH(g_level->z_grids[z].element(x,y), GC_2dSprite, object)
				{
					assert(!object->IsKilled());
					object->Draw();
					assert(!object->IsKilled());
				}
			}


			// loop over globals
			// FIXME: using of global g_level
			FOREACH( g_level->z_globals[z], GC_2dSprite, object )
			{
				assert(!object->IsKilled());
				object->Draw();
				assert(!object->IsKilled());
			}
		}

		if( pMaxShake ) break;
	} // cameras

#ifdef _DEBUG
	FOREACH( g_level->GetList(LIST_players), GC_Player, p )
	{
		if( GC_PlayerAI *pp = dynamic_cast<GC_PlayerAI *>(p) )
		{
			pp->debug_draw();
		}
	}
#endif

	if( !_dbgLineBuffer.empty() )
	{
		g_render->DrawLines(&*_dbgLineBuffer.begin(), _dbgLineBuffer.size());
		_dbgLineBuffer.clear();
	}
}

void Level::DbgLine(const vec2d &v1, const vec2d &v2, SpriteColor color)
{
#ifdef _DEBUG
	_dbgLineBuffer.push_back(MyLine());
	MyLine &line = _dbgLineBuffer.back();
	line.begin = v1;
	line.end = v2;
	line.color = color;
#endif
}

GC_Object* Level::FindObject(const string_t &name) const
{
	std::map<string_t, const GC_Object*>::const_iterator it = _nameToObjectMap.find(name);
	return _nameToObjectMap.end() != it ? const_cast<GC_Object*>(it->second) : NULL;
}

void Level::OnChangeSoundVolume()
{
	FOREACH( GetList(LIST_sounds), GC_Sound, pSound )
	{
		pSound->UpdateVolume();
	}
}

void Level::OnChangeNightMode()
{
	FOREACH( GetList(LIST_lights), GC_Light, pLight )
	{
		assert(!pLight->IsKilled());
		pLight->Update();
	}
}

///////////////////////////////////////////////////////////////////////////////

void Level::AbstractClient::Send(std::vector<VehicleState> &ctrl
#ifdef NETWORK_DEBUG
		  , DWORD cs, int frame
#endif
		  )
{
	if( g_client )
	{
		assert(1 == ctrl.size());
		ControlPacket cp;
		cp.fromvs(ctrl[0]);
#ifdef NETWORK_DEBUG
		cp.checksum = cs;
		cp.frame = frame;
#endif
		g_client->SendControl(cp);
	}
	else
	{
		_cpv.push_back(ControlPacketVector());
		_cpv.back().resize(ctrl.size());
		for( size_t i = 0; i < ctrl.size(); ++i )
		{
			_cpv.back()[i].fromvs(ctrl[i]);
		}
	}
}

bool Level::AbstractClient::Recv(ControlPacketVector &result)
{
	if( g_client )
	{
		return g_client->RecvControl(result);
	}
	if( !_cpv.empty() )
	{
		result.swap(_cpv.front());
		_cpv.pop_front();
		return true;
	}
	return false;
}


///////////////////////////////////////////////////////////////////////////////
// end of file
