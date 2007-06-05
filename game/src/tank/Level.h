// Level.h
#pragma once

////////////////////////////////////////////////////

#include "gc/Object.h" // FIXME!


class GC_RigidBodyStatic;

class Field;
class FieldCell
{
	friend class Field;

	static unsigned long _sessionId;    // ������������� ������� ������
	//-----------------------------
	short _x, _y;                       // ���������� ������
	//-----------------------------
	unsigned long _mySession;        // ������, � ������� ������ ������ ���� ���������
	//-----------------------------
	GC_RigidBodyStatic **_ppObjects;
	unsigned  char _objCount;
	unsigned  char _prop;            // 0 - ��������, 1 - ����������, 0xFF - ������������
	//-----------------------------
	void UpdateProperties();
	FieldCell()
	{
        _ppObjects   = NULL;
		_objCount    = 0;
		_prop        = 0;     // ��������
		//------------------
		_mySession   = -1;
	}
public:

	const FieldCell *_prevCell; // ���������� ������ ����
	float _pathBefore;          // ��������� ���� �� ������ ������
	float _pathAfter;           // ������ ��������� ���� �� ������ ������ �� ������ ����������

	inline int GetObjectsCount() const { return _objCount; }
	inline GC_RigidBodyStatic* GetObject(int index) const { return _ppObjects[index]; }

	inline bool IsChecked() const { return _mySession == _sessionId; }
	inline void Check()           { _mySession = _sessionId;         }

	inline void UpdatePath(short x, short y)
	{
		int dx = abs(x - GetX());
		int dy = abs(y - GetY());
		_pathAfter = (float) __max(dx, dy) + (float) __min(dx, dy) * 0.4142f;
	}

	void AddObject(GC_RigidBodyStatic *object);
	void RemoveObject(GC_RigidBodyStatic *object);

	inline short GetX()               const { return _x;    }
	inline short GetY()               const { return _y;    }
	inline unsigned char Properties() const { return _prop; }

	inline float Rate() const { return _pathBefore + _pathAfter; }

	inline bool operator > (const FieldCell &cell) const
	{
		_ASSERT(_mySession == cell._mySession);
		return Rate() > cell.Rate();
	}
};

class RefFieldCell
{
	FieldCell *_cell;
public:
	RefFieldCell(FieldCell &cell) { _cell = &cell; }
	operator FieldCell& () const { return *_cell; }
	bool operator > (const RefFieldCell &cell) const
	{
		return *_cell > *cell._cell;
	}
};


class Field
{
	FieldCell _edgeCell;
    FieldCell **_cells;
	int _cx;
	int _cy;

	void Clear();

public:
	static void NewSession() { ++FieldCell::_sessionId; }

    Field();
	~Field();

	void Resize(int cx, int cy);
	void ProcessObject(GC_RigidBodyStatic *object, bool oper);

#ifdef _DEBUG
	FieldCell& operator() (int x, int y);
	void Dump();
#else
	inline FieldCell& operator() (int x, int y)
	{
		return (x >= 0 && x < _cx && y >= 0 && y < _cy) ? _cells[y][x] : _edgeCell;
	}
#endif

};

///////////////////////////////////////////////////////////////////////////////

#define ED(name, desc, layer, width, height, align, offset)   \
	Level::RegisterType<__this_class>(                        \
	(name), (desc), (layer), (width), (height), (align), (offset))

#define ED_ITEM(name, desc) ED(name, desc, 1, 2, 2, CELL_SIZE/2, 0)
#define ED_LAND(name, desc, layer) ED(name, desc, layer, CELL_SIZE, CELL_SIZE, CELL_SIZE, CELL_SIZE/2)
#define ED_TURRET(name, desc) ED(name, desc, 0, CELL_SIZE*2, CELL_SIZE*2, CELL_SIZE, CELL_SIZE)


////////////////////////////////////////////////////

class GC_Object;
class GC_Sound;
class GC_Wood;
class GC_RigidBodyStatic;
class GC_Wall;
class GC_Vehicle;
class GC_PickUp;
class GC_2dSprite;

class GC_Text;

//////////////////////

class TankServer;
class TankClient;

class Level
{
#ifdef _DEBUG
	BOOL _bInitialized;
#endif

	friend class GC_Object;

	std::map<const GC_Object*, string_t>  _objectToNameMap;
	std::map<string_t, const GC_Object*>  _nameToObjectMap;

public:

#ifdef NETWORK_DEBUG
	DWORD _dwChecksum;
#endif

	// logics
	OBJECT_LIST     objects;     // global list
	OBJECT_LIST     respawns;
	OBJECT_LIST     projectiles;
	OBJECT_LIST     players;
	OBJECT_LIST     sounds;
	OBJECT_LIST     indicators;
	OBJECT_LIST     vehicles;
	OBJECT_LIST     pickups;
	OBJECT_LIST     cameras;

	OBJECT_GRIDSET  grid_rigid_s;
	OBJECT_GRIDSET  grid_walls;
	OBJECT_GRIDSET  grid_wood;
	OBJECT_GRIDSET  grid_water;
	OBJECT_GRIDSET  grid_pickup;

	OBJECT_LIST     ts_floating;
	OBJECT_LIST     ts_fixed;
	OBJECT_LIST     endframe;

	OBJECT_LIST     lights;

	// graphics
	OBJECT_LIST     z_globals[Z_COUNT];
	OBJECT_GRIDSET  z_grids[Z_COUNT];

	SafePtr<GC_Text>   _temporaryText;
	void DrawText(const char *string, const vec2d &position, enumAlignText align = alignTextLT);

/////////////////////////////////////
//settings
	bool    _paused;
	bool    _modeEditor;
	bool    _limitHit;  // ��������� fraglimit ��� timelimit
	float   _sx, _sy;   // ������ ������

	int _locations_x;
	int _locations_y;

	DWORD    _gameType;

	string_t _infoAuthor;
	string_t _infoEmail;
	string_t _infoUrl;
	string_t _infoDesc;
	string_t _infoTheme;


/////////////////////////////////////////////////////
//network

	int _seed;

	TankServer *_server;
	TankClient *_client;

/////////////////////////////////////
public:
	float _time;
	float _timeBuffer;

    Field _field;

/////////////////////////////////////////////////////
	Level();
	~Level();

	void Init(int X, int Y);
	void Resize(int X, int Y);


	BOOL init_emptymap();
	BOOL init_import_and_edit(char *mapName);

	BOOL init_newdm(const char *mapName);
	BOOL init_load(const char *fileName);

/////////////////////////////////////////////////////
// load / save
public:
	bool RestoreObject(ObjectType otType, HANDLE file);

	bool Unserialize(const char *fileName);
	bool Serialize(const char *fileName);

	bool Export(const char *fileName);
	bool Import(const char *fileName, bool bForEditor);

	void Pause(bool pause);

	void TimeStep(float dt);

	GC_Object* FindObject(const char *name) const;



	//
	// tracing
	//

public:
	GC_RigidBodyStatic* agTrace( GridSet<OBJECT_LIST> &list,
	                             GC_RigidBodyStatic* pIgnore,
	                             const vec2d &x0,      // ���������� ������
	                             const vec2d &a,       // �����������
	                             vec2d *ht   = NULL,
	                             vec2d *norm = NULL) const;



	//
	// editing interface
	//

private:
	typedef GC_Object* (*CreateProc) (float, float);
	template<class T> static GC_Object* CreateObject(float x, float y)
	{
		return new T(x, y);
	}

	struct EdItem
	{
		CreateProc    Create;
		int           layer;
		float         align;
		float         offset;
		vec2d         size;
		const char*   name;
		const char*   desc;
	};

	typedef std::map<ObjectType, EdItem> type2item;
	typedef std::map<string_t, ObjectType> name2type;
	typedef std::vector<ObjectType> index2type;

	static type2item& get_t2i()
	{
		static type2item t2i;
		return t2i;
	}
	static name2type& get_n2t()
	{
		static name2type n2t;
		return n2t;
	}
	static index2type& get_i2t()
	{
		static index2type i2t; // sort by desc
		return i2t;
	}
	
public:
	GC_2dSprite* PickEdObject(const vec2d &pt);

	static int GetTypeCount()
	{
		return get_i2t().size();
	}
	static int GetLayerByTypeIndex(int typeIndex)
	{
		return get_t2i()[get_i2t()[typeIndex]].layer;
	}
	static ObjectType GetType(int typeIndex)
	{
		return get_i2t()[typeIndex];
	}
	static const char* GetTypeDesc(int typeIndex)
	{
		return get_t2i()[get_i2t()[typeIndex]].desc;
	}
	static const char* GetTypeName(ObjectType type)
	{
		return get_t2i()[type].name;
	}
	template<class T> static void RegisterType(const char *name, const char *desc, int layer, float width, float height, float align, float offset)
	{
		EdItem ei;
		ei.Create = (CreateProc) CreateObject<T>;
		ei.desc   = desc;
		ei.name   = name;
		ei.layer  = layer;
		ei.size.Set(width, height);
		ei.align  = align;
		ei.offset = offset;
		get_t2i()[T::this_type] = ei;
		get_n2t()[name] = T::this_type;
		get_i2t().push_back(T::this_type);
	}
	static bool IsRegistered(ObjectType type)
	{
		return (get_t2i().find(type) != get_t2i().end());
	}


	//
	// config callback handlers
	//

protected:
	void OnChangeSoundVolume();

};

///////////////////////////////////////////////////////////////////////////////
// end of file
