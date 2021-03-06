#include "inc/config/ConfigBase.h"
#include <fs/FileSystem.h>
#include <luaetc/LuaDeleter.h>

extern "C"
{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

#include <cassert>
#include <ostream>
#include <stdexcept>

///////////////////////////////////////////////////////////////////////////////

static ConfVar* GetVarIfTypeMatch(ConfVarTable *parent, const char *key, ConfVar::Type type)
{
	if( ConfVar *v = parent->Find(key) )
	{
		return v->GetType() == type ? v : nullptr;
	}
	else if( parent->IsFrozen() )
	{
		return nullptr;
	}
	return parent->GetVar(key, type).first;
}

static ConfVar* TableElementFromLua(lua_State *L, ConfVarTable *parent, const char *key)
{
	ConfVar* result = nullptr;
	int valueType = lua_type(L, -1);
	switch( valueType )
	{
	case LUA_TSTRING:
		result = GetVarIfTypeMatch(parent, key, ConfVar::typeString);
		break;
	case LUA_TBOOLEAN:
		result = GetVarIfTypeMatch(parent, key, ConfVar::typeBoolean);
		break;
	case LUA_TNUMBER:
		result = GetVarIfTypeMatch(parent, key, ConfVar::typeNumber);
		break;
	case LUA_TTABLE:
		result = GetVarIfTypeMatch(parent, key, lua_objlen(L,-1) ? ConfVar::typeArray : ConfVar::typeTable);
		break;
	default:
		return nullptr;
	}

	return result;
}

static ConfVar* ArrayElementFromLua(lua_State *L, ConfVarArray *parent, size_t key)
{
	ConfVar* result = nullptr;
	int valueType = lua_type(L, -1);
	switch( valueType )
	{
	case LUA_TSTRING:
		result = parent->GetVar(key, ConfVar::typeString).first;
		break;
	case LUA_TBOOLEAN:
		result = parent->GetVar(key, ConfVar::typeBoolean).first;
		break;
	case LUA_TNUMBER:
		result = parent->GetVar(key, ConfVar::typeNumber).first;
		break;
	case LUA_TTABLE:
		result = parent->GetVar(key, lua_objlen(L,-1) ? ConfVar::typeArray : ConfVar::typeTable).first;
		break;
	default:
//		_logger.Format(1) << "Unknown lua type - " << lua_typename(L, valueType);
		return nullptr;
	}

	return result;
}

///////////////////////////////////////////////////////////////////////////////

ConfVar::ConfVar()
  : _type(typeNil)
  , _frozen(false)
{
}

ConfVar::~ConfVar()
{
	assert(typeNil == _type);
}

void ConfVar::FireValueUpdate(ConfVar *pVar)
{
	if( eventChange )
		eventChange();
}

const char* ConfVar::GetTypeName() const
{
	return "nil";
}

void ConfVar::SetType(Type type)
{
	assert(!_frozen);
	if( type != _type )
	{
		this->~ConfVar(); // manually call the destructor
		switch( type )
		{
#pragma push_macro("new")
#undef new
			case typeNil:     new(this) ConfVar();        break;
			case typeNumber:  new(this) ConfVarNumber();  break;
			case typeBoolean: new(this) ConfVarBool();    break;
			case typeString:  new(this) ConfVarString();  break;
			case typeArray:   new(this) ConfVarArray();   break;
			case typeTable:   new(this) ConfVarTable();   break;
			default: assert(!"unknown ConfVar type");
#pragma pop_macro("new")
		}
		assert( _type == type );
	}
}

ConfVarNumber& ConfVar::AsNum()
{
	assert(typeNumber == _type);
	return static_cast<ConfVarNumber&>(*this);
}

ConfVarBool& ConfVar::AsBool()
{
	assert(typeBoolean == _type);
	return static_cast<ConfVarBool&>(*this);
}

ConfVarString& ConfVar::AsStr()
{
	assert(typeString == _type);
	return static_cast<ConfVarString&>(*this);
}

ConfVarArray& ConfVar::AsArray()
{
	assert(typeArray == _type);
	return static_cast<ConfVarArray&>(*this);
}

ConfVarTable& ConfVar::AsTable()
{
	assert(typeTable == _type);
	return static_cast<ConfVarTable&>(*this);
}

void ConfVar::Write(std::ostream &o, int indent) const
{
	o << "nil";
}

bool ConfVar::Assign(lua_State *)
{
	assert(false);
	return false;
}

void ConfVar::Push(lua_State *) const
{
	assert(false);
}

void ConfVar::Freeze(bool freeze)
{
	_frozen = freeze;
}


///////////////////////////////////////////////////////////////////////////////
// number

ConfVarNumber::ConfVarNumber()
{
	_type = typeNumber;
	_val.asNumber = 0;
}

ConfVarNumber::~ConfVarNumber()
{
	_type = typeNil;
}

const char* ConfVarNumber::GetTypeName() const
{
	return "number";
}

double ConfVarNumber::GetRawNumber() const
{
	assert(typeNumber == _type);
	return _val.asNumber;
}

void ConfVarNumber::SetRawNumber(double value)
{
	assert(typeNumber == _type);
	if( _val.asNumber != value )
	{
		_val.asNumber = value;
		FireValueUpdate(this);
	}
}

float ConfVarNumber::GetFloat() const
{
	assert(typeNumber == _type);
	return (float) _val.asNumber;
}
void ConfVarNumber::SetFloat(float value)
{
	assert(typeNumber == _type);
	if( _val.asNumber != value )
	{
		_val.asNumber = value;
		FireValueUpdate(this);
	}
}

int ConfVarNumber::GetInt() const
{
	assert(typeNumber == _type);
	return (int) _val.asNumber;
}
void ConfVarNumber::SetInt(int value)
{
	assert(typeNumber == _type);
	if( _val.asNumber != value )
	{
		_val.asNumber = value;
		FireValueUpdate(this);
	}
}

void ConfVarNumber::Write(std::ostream &o, int indent) const
{
	o << _val.asNumber;
}

bool ConfVarNumber::Assign(lua_State *L)
{
	SetFloat( (float) lua_tonumber(L, -1) );
	return true;
}

void ConfVarNumber::Push(lua_State *L) const
{
	lua_pushnumber(L, GetFloat());
}

///////////////////////////////////////////////////////////////////////////////
// boolean

ConfVarBool::ConfVarBool()
{
	_type = typeBoolean;
	_val.asBool = false;
}

ConfVarBool::~ConfVarBool()
{
	_type = typeNil;
}

const char* ConfVarBool::GetTypeName() const
{
	return "boolean";
}

bool ConfVarBool::Get() const
{
	assert(typeBoolean == _type);
	return _val.asBool;
}
void ConfVarBool::Set(bool value)
{
	assert(typeBoolean == _type);
	_val.asBool = value;
	FireValueUpdate(this);
}

void ConfVarBool::Write(std::ostream &o, int indent) const
{
	o << (Get() ? "true" : "false");
}

bool ConfVarBool::Assign(lua_State *L)
{
	Set( 0 != lua_toboolean(L, -1) );
	return true;
}

void ConfVarBool::Push(lua_State *L) const
{
	lua_pushboolean(L, Get());
}

///////////////////////////////////////////////////////////////////////////////
// string

ConfVarString::ConfVarString()
{
	_type = typeString;
	_val.asString = new std::string();
}

ConfVarString::~ConfVarString()
{
	assert( typeString == _type );
	delete _val.asString;
	_type = typeNil;
}

const char* ConfVarString::GetTypeName() const
{
	return "string";
}

std::string_view ConfVarString::Get() const
{
	assert(typeString == _type);
	return *_val.asString;
}

void ConfVarString::Set(std::string value)
{
	assert(typeString == _type);
	*_val.asString = std::move(value);
	FireValueUpdate(this);
}

void ConfVarString::Set(std::string_view value)
{
	assert(typeString == _type);
	*_val.asString = value;
	FireValueUpdate(this);
}

void ConfVarString::Set(const char *value)
{
	assert(typeString == _type);
	*_val.asString = value;
	FireValueUpdate(this);
}

void ConfVarString::Write(std::ostream &o, int indent) const
{
	o << '"';
	for( size_t i = 0; i < _val.asString->size(); ++i )
	{
		const char c = (*_val.asString)[i];
		switch(c)
		{
		case '\\':
			o << "\\\\";
			break;
		case '\r':
			o << "\\r";
			break;
		case '\n':
			o << "\\n";
			break;
		case '"':
			o << "\\\"";
			break;
		case '\t':
			o << "\\t";
			break;
		default:
			o << c;
		}
	}
	o << '"';
}

bool ConfVarString::Assign(lua_State *L)
{
	Set(lua_tostring(L, -1));
	return true;
}

void ConfVarString::Push(lua_State *L) const
{
	lua_pushlstring(L, Get().data(), Get().length());
}


///////////////////////////////////////////////////////////////////////////////
// array

ConfVarArray::ConfVarArray()
{
	_type = typeArray;
	_val.asArray = new std::deque<std::unique_ptr<ConfVar>>();
}

ConfVarArray::~ConfVarArray()
{
	assert(typeArray == _type);
	delete _val.asArray;
	_type = typeNil;
}

const char* ConfVarArray::GetTypeName() const
{
	return "array";
}

std::pair<ConfVar*, bool> ConfVarArray::GetVar(size_t index, ConfVar::Type type)
{
	assert( index < GetSize() );
	std::pair<ConfVar*, bool> result( (*_val.asArray)[index].get(), true);

	if( result.first->GetType() != type )
	{
		result.first->SetType(type);
		result.second = false;
		FireValueUpdate(this);
		assert(result.first->GetType() == type);
	}

	return result;
}

ConfVarNumber& ConfVarArray::GetNum(size_t index, float def)
{
	std::pair<ConfVar*, bool> p = GetVar(index, ConfVar::typeNumber);
	if( !p.second )
		p.first->AsNum().SetFloat(def);
	return p.first->AsNum();
}

ConfVarNumber& ConfVarArray::SetNum(size_t index, float value)
{
	ConfVarNumber &v = GetVar(index, ConfVar::typeNumber).first->AsNum();
	v.SetFloat(value);
	return v;
}

ConfVarNumber& ConfVarArray::GetNum(size_t index, int def)
{
	std::pair<ConfVar*, bool> p = GetVar(index, ConfVar::typeNumber);
	if( !p.second )
		p.first->AsNum().SetInt(def);
	return p.first->AsNum();
}

ConfVarNumber& ConfVarArray::SetNum(size_t index, int value)
{
	ConfVarNumber &v = GetVar(index, ConfVar::typeNumber).first->AsNum();
	v.SetInt(value);
	return v;
}

ConfVarBool& ConfVarArray::GetBool(size_t index, bool def)
{
	std::pair<ConfVar*, bool> p = GetVar(index, ConfVar::typeBoolean);
	if( !p.second )
		p.first->AsBool().Set(def);
	return p.first->AsBool();
}

ConfVarBool& ConfVarArray::SetBool(size_t index, bool value)
{
	ConfVarBool &v = GetVar(index, ConfVar::typeBoolean).first->AsBool();
	v.Set(value);
	return v;
}

ConfVarString& ConfVarArray::GetStr(size_t index)
{
	return GetVar(index, ConfVar::typeString).first->AsStr();
}

ConfVarString& ConfVarArray::GetStr(size_t index, std::string def)
{
	std::pair<ConfVar*, bool> p = GetVar(index, ConfVar::typeString);
	if (!p.second)
		p.first->AsStr().Set(std::move(def));
	return p.first->AsStr();
}

ConfVarString& ConfVarArray::SetStr(size_t index, std::string value)
{
	ConfVarString &v = GetVar(index, ConfVar::typeString).first->AsStr();
	v.Set(std::move(value));
	return v;
}

ConfVarArray& ConfVarArray::GetArray(size_t index)
{
	return GetVar(index, ConfVar::typeArray).first->AsArray();
}

ConfVarTable& ConfVarArray::GetTable(size_t index)
{
	return GetVar(index, ConfVar::typeTable).first->AsTable();
}

void ConfVarArray::EnsureIndex(size_t index)
{
	if (index + 1 > GetSize())
		Resize(index + 1);
}

void ConfVarArray::Resize(size_t newSize)
{
	assert(typeArray == _type);
	size_t oldSize = _val.asArray->size();
	if (newSize != oldSize)
	{
		_val.asArray->resize(newSize);
		if (newSize > oldSize)
		{
			for (size_t i = oldSize; i < newSize; ++i)
			{
				(*_val.asArray)[i].reset(new ConfVar());
			}
		}
		FireValueUpdate(this);
	}
}

size_t ConfVarArray::GetSize() const
{
	assert(typeArray == _type);
	return _val.asArray->size();
}

ConfVar& ConfVarArray::GetAt(size_t index) const
{
	assert(typeArray == _type);
	return *(*_val.asArray)[index];
}

void ConfVarArray::RemoveAt(size_t index)
{
	assert(typeArray == _type);
	_val.asArray->erase(_val.asArray->begin() + index);
	FireValueUpdate(this);
}

void ConfVarArray::PopFront()
{
	assert(typeArray == _type);
	_val.asArray->pop_front();
	FireValueUpdate(this);
}

ConfVar& ConfVarArray::PushBack(Type type)
{
	assert(typeArray == _type);
	std::unique_ptr<ConfVar> result(new ConfVar());
	result->SetType(type);
	_val.asArray->push_back(std::move(result));
	FireValueUpdate(this);
	return *_val.asArray->back();
}

void ConfVarArray::PopBack()
{
	assert(typeArray == _type);
	_val.asArray->pop_back();
	FireValueUpdate(this);
}

ConfVar& ConfVarArray::PushFront(Type type)
{
	assert(typeArray == _type);
	std::unique_ptr<ConfVar> result(new ConfVar());
	result->SetType(type);
	_val.asArray->push_front(std::move(result));
	FireValueUpdate(this);
	return *_val.asArray->front();
}

void ConfVarArray::Write(std::ostream &o, int indent) const
{
	o << "{\n";
	bool delim = false;
	for( size_t i = 0; i < GetSize(); ++i )
	{
		if( delim )
			o << ",\n";
		for( int n = 0; n < indent; ++n )
			o << "  ";
		delim = true;
		GetAt(i).Write(o, indent+1);
	}
	o << '\n';
	for (int n = 0; n < indent - 1; ++n)
		o << "  ";
	o << '}';
}

bool ConfVarArray::Assign(lua_State *L)
{
	Resize( lua_objlen(L, -1) );

	for( size_t i = 0; i < GetSize(); ++i )
	{
		lua_pushinteger(L, i+1); // push the key
		lua_gettable(L, -2);   // pop the key, push the value

		if( ConfVar *v = ArrayElementFromLua(L, this, i) )
		{
			if( !v->Assign(L) )
				return false;
		}

		lua_pop(L, 1);         // pop the value
	}

	return true;
}

void ConfVarArray::Push(lua_State *L) const
{
	*reinterpret_cast<ConfVarArray const**>( lua_newuserdata(L, sizeof(this)) ) = this;
	luaL_getmetatable(L, "conf_array");  // metatable for config
	lua_setmetatable(L, -2);
}


///////////////////////////////////////////////////////////////////////////////
// table

ConfVarTable::ConfVarTable()
{
	_type = typeTable;
	_val.asTable = new TableType();
}

ConfVarTable::~ConfVarTable()
{
	assert(typeTable == _type);
	delete _val.asTable;
	_type = typeNil;
}

const char* ConfVarTable::GetTypeName() const
{
	return "table";
}

ConfVar* ConfVarTable::Find(std::string_view name)  // returns nullptr if variable not found
{
	auto it = _val.asTable->find(name);
	return _val.asTable->end() != it ? it->second.get() : nullptr;
}

size_t ConfVarTable::GetSize() const
{
	return _val.asTable->size();
}

std::vector<std::string> ConfVarTable::GetKeys() const
{
	std::vector<std::string> out;
	for( auto it = _val.asTable->begin(); _val.asTable->end() != it; ++it )
	{
		out.push_back(it->first);
	}
	return out;
}

std::pair<ConfVar*, bool> ConfVarTable::GetVar(std::string_view name, ConfVar::Type type)
{
	std::pair<ConfVar*, bool> result(nullptr, true);

	assert(ConfVar::typeNil != type);
	auto it = _val.asTable->find(name);
	if( _val.asTable->end() == it )
	{
		// create new item
		assert(!_frozen);
		result.first = _val.asTable->emplace(std::move(name), std::unique_ptr<ConfVar>(new ConfVar())).first->second.get();
		FireValueUpdate(this);
	}
	else
	{
		result.first = it->second.get();
	}

	if( result.first->GetType() != type )
	{
		// change type of the existing item
		assert(!_frozen);
		result.first->SetType(type);
		result.second = false;
		FireValueUpdate(this);
	}
	assert(result.first->GetType() == type);

	return result;
}

ConfVarNumber& ConfVarTable::GetNum(std::string_view name, float def)
{
	std::pair<ConfVar*, bool> p = GetVar(name, ConfVar::typeNumber);
	if( !p.second )
		p.first->AsNum().SetFloat(def);
	return p.first->AsNum();
}

ConfVarNumber& ConfVarTable::SetNum(std::string name, float value)
{
	ConfVarNumber &v = GetVar(std::move(name), ConfVar::typeNumber).first->AsNum();
	v.SetFloat(value);
	return v;
}

ConfVarNumber& ConfVarTable::GetNum(std::string_view name, int def)
{
	std::pair<ConfVar*, bool> p = GetVar(name, ConfVar::typeNumber);
	if( !p.second )
		p.first->AsNum().SetInt(def);
	return p.first->AsNum();
}

ConfVarNumber& ConfVarTable::SetNum(std::string name, int value)
{
	ConfVarNumber &v = GetVar(std::move(name), ConfVar::typeNumber).first->AsNum();
	v.SetInt(value);
	return v;
}

ConfVarBool& ConfVarTable::GetBool(std::string_view name, bool def)
{
	std::pair<ConfVar*, bool> p = GetVar(name, ConfVar::typeBoolean);
	if( !p.second )
		p.first->AsBool().Set(def);
	return p.first->AsBool();
}

ConfVarBool& ConfVarTable::SetBool(std::string name, bool value)
{
	ConfVarBool &v = GetVar(std::move(name), ConfVar::typeBoolean).first->AsBool();
	v.Set(value);
	return v;
}

ConfVarString& ConfVarTable::GetStr(std::string_view name)
{
	return GetVar(name, ConfVar::typeString).first->AsStr();
}

ConfVarString& ConfVarTable::GetStr(std::string_view name, std::string_view def)
{
	std::pair<ConfVar*, bool> p = GetVar(name, ConfVar::typeString);
	if (!p.second)
		p.first->AsStr().Set(std::move(def));
	return p.first->AsStr();
}

ConfVarString& ConfVarTable::SetStr(std::string name, std::string value)
{
	ConfVarString &v = GetVar(std::move(name), ConfVar::typeString).first->AsStr();
	v.Set(std::move(value));
	return v;
}

ConfVarArray& ConfVarTable::GetArray(std::string_view name, void (*init)(ConfVarArray&))
{
	std::pair<ConfVar*, bool> p = GetVar(name, ConfVar::typeArray);
	if( !p.second && init )
		init(p.first->AsArray());
	return p.first->AsArray();
}

ConfVarTable& ConfVarTable::GetTable(std::string_view name, void (*init)(ConfVarTable&))
{
	std::pair<ConfVar*, bool> p = GetVar(name, ConfVar::typeTable);
	if( !p.second && init )
		init(p.first->AsTable());
	return p.first->AsTable();
}

void ConfVarTable::Clear()
{
	assert( typeTable == _type );
	assert( !_frozen );
	_val.asTable->clear();
	FireValueUpdate(this);
}

bool ConfVarTable::Remove(const ConfVar &value)
{
	assert( typeTable == _type );
	assert( !_frozen );
	for( auto it = _val.asTable->begin(); _val.asTable->end() != it; ++it )
	{
		if( &value == it->second.get() )
		{
			_val.asTable->erase(it);
			FireValueUpdate(this);
			return true;
		}
	}
	return false;
}

bool ConfVarTable::Remove(std::string_view name)
{
	assert( typeTable == _type );
	assert( !_frozen );
	auto it = _val.asTable->find(name);
	if( _val.asTable->end() != it )
	{
		_val.asTable->erase(it);
		FireValueUpdate(this);
		return true;
	}
	return false;
}

bool ConfVarTable::Rename(const ConfVar &value, std::string newName)
{
	assert( typeTable == _type );
	assert( !_frozen );

	auto it = _val.asTable->begin();
	while( _val.asTable->end() != it )
	{
		if( &value == it->second.get() )
		{
			break;
		}
		++it;
	}
	if( _val.asTable->end() == it )
	{
		return false; // old name not found
	}

	if( it->first == newName )
	{
		return true;
	}

	if( _val.asTable->count(newName) )
	{
		return false; // new name is already in use
	}

	_val.asTable->emplace(std::move(newName), std::move(it->second));
	_val.asTable->erase(it);

	FireValueUpdate(this);

	return true;
}

bool ConfVarTable::Rename(std::string_view oldName, std::string newName)
{
	assert( typeTable == _type );
	assert( !_frozen );

	auto it = _val.asTable->find(oldName);
	if( _val.asTable->end() == it )
	{
		return false; // old name not found
	}

	if( it->first == newName )
	{
		return true;
	}

	if( _val.asTable->count(newName) )
	{
		return false; // new name is already in use
	}

	_val.asTable->emplace(std::move(newName), std::move(it->second));
	_val.asTable->erase(it);

	FireValueUpdate(this);

	return true;
}

void ConfVarTable::Write(std::ostream &o, int indent) const
{
	if( indent )
	{
		o << '{' << std::endl;
	}

	for( auto it = _val.asTable->begin(); _val.asTable->end() != it; ++it )
	{
		for( int i = 0; i < indent; ++i )
		{
			o << "  ";
		}

		bool safe = true;
		for( size_t i = 0; i < it->first.size(); ++i )
		{
			unsigned char c = it->first[i];
			if( !isalpha(c) && '_' != c )
			{
				if( 0 == i || !isdigit(c) )
				{
					safe = false;
					break;
				}
			}
		}

		if( safe )
		{
			o << it->first;
		}
		else
		{
			o << "[\"";
			for( size_t i = 0; i < it->first.size(); ++i )
			{
				const char c = it->first[i];
				switch(c)
				{
				case '\\':
					o << "\\\\";
					break;
				case '\n':
					o << "\\n";
					break;
				case '"':
					o << "\\\"";
					break;
				case '\t':
					o << "\\t";
					break;
				default:
					o << c;
				}
			}
			o << "\"]";
		}

		o << " = ";
		it->second->Write(o, indent + 1);
		o << (indent ? ',' : ';');
		o << std::endl;
	}
	if( indent )
	{
		for( int i = 0; i < indent-1; ++i )
		{
			o << "  ";
		}
		o << '}';
	}
}

bool ConfVarTable::Assign(lua_State *L)
{
	// enumerate all fields of the table
	for( lua_pushnil(L); lua_next(L, -2); lua_pop(L, 1) )
	{
		// now 'key' is at index -2 and 'value' at index -1
		// use only string keys
		if( LUA_TSTRING == lua_type(L, -2) )
		{
			const char *key = lua_tostring(L, -2);
			if( ConfVar *v = TableElementFromLua(L, this, key) )
			{
				if( !v->Assign(L) )
					return false;
			}
			else
			{
//				_logger.Printf(1, "variable '%s' was dropped", key);
			}
		}
	}

	return true;
}

void ConfVarTable::Save(std::ostream &o) const
{
	o << "-- config file was automatically generated by application" << std::endl;
	Write(o, 0);
}

bool ConfVarTable::Load(FS::MemMap &data, const char *name)
{
	std::unique_ptr<lua_State, LuaStateDeleter> L(luaL_newstate());

	// try to read and execute the file
	if( luaL_loadbuffer(L.get(), static_cast<const char *>(data.GetData()), data.GetSize(), name) ||
		lua_pcall(L.get(), 0, 0, 0) )
	{
		throw std::runtime_error(lua_tostring(L.get(), -1));
	}

	// get global table
	lua_pushvalue(L.get(), LUA_GLOBALSINDEX);
	bool result = Assign(L.get());

	FireValueUpdate(this);

	return result;
}

void ConfVarTable::Push(lua_State *L) const
{
	*reinterpret_cast<ConfVarTable const**>(lua_newuserdata(L, sizeof(this))) = this;
	luaL_getmetatable(L, "conf_table");  // metatable for config
	lua_setmetatable(L, -2);
}


///////////////////////////////////////////////////////////////////////////////
// lua binding

// returns type string for arrays and tables
static int luaT_conftostring(lua_State *L)
{
	assert(lua_type(L, 1) == LUA_TUSERDATA);
	ConfVar *v = *reinterpret_cast<ConfVar **>( lua_touserdata(L, 1) );
	switch( v->GetType() )
	{
	case ConfVar::typeArray:
		lua_pushfstring(L, "conf_array: %p", v);
		break;
	case ConfVar::typeTable:
		lua_pushfstring(L, "conf_table: %p", v);
		break;
	default:
		assert(false);
	}
	return 1;
}

// retrieving an array element
static int luaT_getconfarray(lua_State *L)
{
	lua_settop(L, 2);

	ConfVarArray *v = *reinterpret_cast<ConfVarArray **>(luaL_checkudata(L, 1, "conf_array"));
	assert( ConfVar::typeArray == v->GetType() );

	size_t index = luaL_checkinteger(L, 2);
	if( index >= v->AsArray().GetSize() )
	{
		return luaL_error(L, "array index is out of range");
	}
	v->GetAt(index).Push(L);
	return 1;
}

// retrieving a table element
static int luaT_getconftable(lua_State *L)
{
	lua_settop(L, 2);

	ConfVarTable *v = *reinterpret_cast<ConfVarTable **>(luaL_checkudata(L, 1, "conf_table"));
	assert( ConfVar::typeTable == v->GetType() );

	const char *key = luaL_checkstring(L, 2);

	if( ConfVar *result = v->Find(key) )
	{
		result->Push(L);
	}
	else
	{
		return luaL_error(L, "variable '%s' not found", key);
	}

	return 1;
}

// assigning a value to an array element
static int luaT_setconfarray(lua_State *L)
{
	lua_settop(L, 3);

	ConfVarArray *v = *reinterpret_cast<ConfVarArray **>(luaL_checkudata(L, 1, "conf_array"));
	assert( ConfVar::typeArray == v->GetType() );

	size_t index = luaL_checkinteger(L, 2);

	if( index >= v->AsArray().GetSize() )
	{
		return luaL_error(L, "array index is out of range");
	}

	ConfVar &val = v->GetAt(index);
	switch( val.GetType() )
	{
	case ConfVar::typeBoolean:
		val.AsBool().Set( 0 != lua_toboolean(L, 3) );
		break;
	case ConfVar::typeNumber:
		val.AsNum().SetFloat( (float) luaL_checknumber(L, 3) );
		break;
	case ConfVar::typeString:
		val.AsStr().Set(luaL_checkstring(L, 3));
		break;
	case ConfVar::typeArray:
		return luaL_error(L, "attempt to modify conf_array");
	case ConfVar::typeTable:
		return luaL_error(L, "attempt to modify conf_table");
    case ConfVar::typeNil:
        break;
	}

	return 0;
}

// assigning a value to a table element
static int luaT_setconftable(lua_State *L)
{
	lua_settop(L, 3);

	ConfVarTable *v = *reinterpret_cast<ConfVarTable **>(luaL_checkudata(L, 1, "conf_table"));
	assert( ConfVar::typeTable == v->GetType() );

	const char *key = luaL_checkstring(L, 2);

	if( ConfVar *val = v->Find(key) )
	{
		switch( val->GetType() )
		{
		case ConfVar::typeBoolean:
			val->AsBool().Set(!!lua_toboolean(L, 3));
			break;
		case ConfVar::typeNumber:
			val->AsNum().SetFloat((float) luaL_checknumber(L, 3));
			break;
		case ConfVar::typeString:
			val->AsStr().Set(luaL_checkstring(L, 3));
			break;
		case ConfVar::typeArray:
			luaL_checktype(L, 3, LUA_TTABLE);
			return val->Assign(L);
		case ConfVar::typeTable:
			luaL_checktype(L, 3, LUA_TTABLE);
			return luaL_error(L, "attempt to modify conf_table");
		case ConfVar::typeNil:
			break;
		}
	}
	else
	{
		return luaL_error(L, "variable '%s' not found", key);
	}
	return 0;
}

// generic __next support for tables
int ConfVarTable::luaT_conftablenext(lua_State *L)
{
	lua_settop(L, 2);

	ConfVarTable *v = *reinterpret_cast<ConfVarTable **>(luaL_checkudata(L, 1, "conf_table"));
	assert( ConfVar::typeTable == v->GetType() );

	if( v->_val.asTable->empty() )
	{
		lua_pushnil(L);
		return 1;
	}

	if( lua_isnil(L, 2) )
	{
		// begin iteration
		lua_pushstring(L, v->_val.asTable->begin()->first.c_str()); // key
		v->_val.asTable->begin()->second->Push(L);                  // value
		return 2;
	}

	const char *key = luaL_checkstring(L, 2);
	auto it = v->_val.asTable->find(key);
	if( v->_val.asTable->end() == it )
	{
		return luaL_error(L, "invalid key to 'next'");
	}

	++it;

	if( v->_val.asTable->end() == it )
	{
		// end of list
		lua_pushnil(L);
		return 1;
	}

	// return next pair
	lua_pushstring(L, it->first.c_str());   // key
	it->second->Push(L);                    // value
	return 2;
}


// map config to the conf lua variable
void ConfVarTable::InitConfigLuaBinding(lua_State *L, const char *globName)
{
#ifndef NDEBUG
	int top = lua_gettop(L);
#endif

	luaL_newmetatable(L, "conf_table");  // metatable for tables
	 lua_pushcfunction(L, luaT_setconftable);
	  lua_setfield(L, -2, "__newindex");
	 lua_pushcfunction(L, luaT_getconftable);
	  lua_setfield(L, -2, "__index");
	 lua_pushcfunction(L, luaT_conftostring);
	  lua_setfield(L, -2, "__tostring");
	 lua_pushcfunction(L, luaT_conftablenext);
	  lua_setfield(L, -2, "__next");
	 lua_pop(L, 1); // pop the metatable

	luaL_newmetatable(L, "conf_array");  // metatable for arrays
	 lua_pushcfunction(L, luaT_setconfarray);  // push handler function
	  lua_setfield(L, -2, "__newindex");        // this also pops function from the stack
	 lua_pushcfunction(L, luaT_getconfarray);  // push handler function
	  lua_setfield(L, -2, "__index");           // this also pops function from the stack
	 lua_pushcfunction(L, luaT_conftostring);  // push handler function
	  lua_setfield(L, -2, "__tostring");        // this also pops function from the stack
	 lua_pop(L, 1); // pop the metatable

	Push(L);
	 lua_setglobal(L, globName);    // set global and pop one element from stack

	assert(lua_gettop(L) == top);
}

///////////////////////////////////////////////////////////////////////////////

LuaConfigCacheBase::LuaConfigCacheBase()
{
}

LuaConfigCacheBase::~LuaConfigCacheBase()
{
}

LuaConfigCacheBase::helper* LuaConfigCacheBase::operator ->()
{
	return &_helper;
}

///////////////////////////////////////////////////////////////////////////////
// end of file
