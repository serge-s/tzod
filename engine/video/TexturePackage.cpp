#include "inc/video/TexturePackage.h"
#include <fs/FileSystem.h>
#include <luaetc/LuaDeleter.h>
#include <stdexcept>

extern "C"
{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

static int getchar(lua_State* L, int tblidx, const char* field, int def)
{
	lua_getfield(L, tblidx, field);
	if (lua_isstring(L, -1) && lua_objlen(L, -1) > 0)
		def = static_cast<int>((unsigned char)lua_tostring(L, -1)[0]);
	lua_pop(L, 1); // pop result of getfield
	return def;
}

static int getint(lua_State* L, int tblidx, const char* field, int def)
{
	lua_getfield(L, tblidx, field);
	if (lua_isnumber(L, -1))
		def = static_cast<int>(lua_tointeger(L, -1));
	lua_pop(L, 1); // pop result of getfield
	return def;
}

static bool trygetfloat(lua_State * L, int tblidx, const char* field, float def, float* outValue)
{
	bool result;
	lua_getfield(L, tblidx, field);
	if (lua_isnumber(L, -1))
	{
		*outValue = (float)lua_tonumber(L, -1);
		result = true;
	}
	else
	{
		*outValue = def;
		result = false;
	}
	lua_pop(L, 1); // pop result of getfield
	return result;
}

static bool getbool(lua_State * L, int tblidx, const char* field, bool def)
{
	lua_getfield(L, tblidx, field);
	if (!lua_isnil(L, -1))
		def = !!lua_toboolean(L, -1);
	lua_pop(L, 1); // pop result of getfield
	return def;
}

static void getspritedef(lua_State * L, int idx, PackageSpriteDesc* outSpriteDef)
{
	// texture bounds
	trygetfloat(L, idx, "left", 0, &outSpriteDef->atlasOffset.x);
	trygetfloat(L, idx, "top", 0, &outSpriteDef->atlasOffset.y);
	outSpriteDef->hasSizeX = trygetfloat(L, idx, "right", 0, &outSpriteDef->atlasSize.x);
	outSpriteDef->hasSizeY = trygetfloat(L, idx, "bottom", 0, &outSpriteDef->atlasSize.y);
	outSpriteDef->atlasSize -= outSpriteDef->atlasOffset;

	// border
	trygetfloat(L, idx, "border", 0, &outSpriteDef->border);

	// scale
	trygetfloat(L, idx, "xscale", 1, &outSpriteDef->scale.x);
	trygetfloat(L, idx, "yscale", 1, &outSpriteDef->scale.y);

	// frames count
	outSpriteDef->xframes = getint(L, idx, "xframes", 1);
	outSpriteDef->yframes = getint(L, idx, "yframes", 1);

	// pivot position
	outSpriteDef->hasPivotX = trygetfloat(L, idx, "xpivot", 0, &outSpriteDef->pivot.x);
	outSpriteDef->hasPivotY = trygetfloat(L, idx, "ypivot", 0, &outSpriteDef->pivot.y);

	// font
	outSpriteDef->leadChar = getchar(L, idx, "leadchar", ' ');

	// flags
	outSpriteDef->magFilter = getbool(L, idx, "magfilter", false);
	outSpriteDef->wrappable = getbool(L, idx, "wrappable", false);
}


std::vector<PackageSpriteDesc> ParsePackage(const std::string& packageName, std::shared_ptr<FS::MemMap> file, FS::FileSystem& fs)
{
	std::vector<PackageSpriteDesc> result;

	std::unique_ptr<lua_State, LuaStateDeleter> luaState(lua_open());
	if (!luaState)
		throw std::bad_alloc();

	lua_State* L = luaState.get();

	if (0 != (luaL_loadbuffer(L, static_cast<const char*>(file->GetData()), file->GetSize(), packageName.c_str()) ||
		lua_pcall(L, 0, 1, 0)))
	{
		std::runtime_error e(lua_tostring(L, -1));
		lua_close(L);
		throw e;
	}

	if (lua_istable(L, -1))
	{
		// loop over files
		for (lua_pushnil(L); lua_next(L, -2); lua_pop(L, 1))
		{
			// now 'key' is at index -2 and 'value' at index -1
			if (!lua_istable(L, -1))
				continue;

			lua_getfield(L, -1, "file");
			std::string fileName = lua_tostring(L, -1);
			lua_pop(L, 1); // pop result of lua_getfield

			lua_getfield(L, -1, "content");
			if (lua_istable(L, -1))
			{
				// loop over textures in 'content' table
				for (lua_pushnil(L); lua_next(L, -2); lua_pop(L, 1))
				{
					if (!lua_istable(L, -1))
						continue;

					// make the key copy because lua_tostring may change its type
					lua_pushvalue(L, -2);
					if (const char* texname = lua_tostring(L, -1))
					{
						// now 'value' at index -2
						PackageSpriteDesc sd = {};
						getspritedef(L, -2, &sd);
						sd.textureFilePath = fileName;
						sd.spriteName = texname;

						result.push_back(std::move(sd));
					}
					lua_pop(L, 1); // pop key copy
				}
			}
			lua_pop(L, 1); // pop content
		}
	}

	return result;
}

static bool TryReadMetadata(std::shared_ptr<FS::FileSystem> dir, std::string fileName, PackageSpriteDesc &result)
{
	auto file = dir->Open(fileName, FS::ModeRead, true /*nothrow*/);
	if (!file)
		return false;
	auto mappedFile = file->QueryMap();

	std::unique_ptr<lua_State, LuaStateDeleter> luaState(lua_open());
	if (!luaState)
		throw std::bad_alloc();
	lua_State* L = luaState.get();

	if (0 != (luaL_loadbuffer(L, static_cast<const char*>(mappedFile->GetData()), mappedFile->GetSize(), fileName.c_str()) ||
		lua_pcall(L, 0, 1, 0)))
	{
		std::runtime_error e(lua_tostring(L, -1));
		lua_close(L);
		throw e;
	}

	getspritedef(L, LUA_GLOBALSINDEX, &result);

	return true;
}

std::vector<PackageSpriteDesc> ParseDirectory(FS::FileSystem& fs, std::string_view dirName, std::string_view texPrefix)
{
	std::vector<PackageSpriteDesc> result;

	std::shared_ptr<FS::FileSystem> dir = fs.GetFileSystem(dirName);

	if (auto dirsFile = dir->Open("!dirs", FS::ModeRead, true /*nothrow*/))
	{
		auto mappedFile = dirsFile->QueryMap();
		auto stringView = std::string_view(reinterpret_cast<const char*>(mappedFile->GetData()), mappedFile->GetSize());
		auto constexpr delimiters = "\n\r";
		auto lineBegin = stringView.find_first_not_of(delimiters);
		while (lineBegin != std::string_view::npos)
		{
			auto lineEnd = stringView.find_first_of(delimiters, lineBegin);
			if (lineEnd == std::string_view::npos)
				lineEnd = stringView.length();

			auto subdirName = stringView.substr(lineBegin, lineEnd - lineBegin);
			auto subdirContents = ParseDirectory(fs, FS::PathCombine(dirName, subdirName), std::string(texPrefix).append(subdirName).append("/"));
			result.insert(result.end(), subdirContents.begin(), subdirContents.end());

			lineBegin = stringView.find_first_not_of(delimiters, lineEnd);
		}
	}

	auto files = dir->EnumAllFiles("*.tga");
	for (auto fileName: files)
	{
		auto fileNameNoExt = fileName;
		fileNameNoExt.erase(fileNameNoExt.length() - 4); // remove .tga extension

		PackageSpriteDesc sd = {};
		if (!TryReadMetadata(dir, fileNameNoExt + ".lua", sd))
		{
			sd.scale = vec2d{ 1, 1 };
			sd.xframes = 1;
			sd.yframes = 1;
			sd.leadChar = ' ';
		}
		sd.textureFilePath = FS::PathCombine(dirName, fileName);
		sd.spriteName = std::string(texPrefix).append(fileNameNoExt);
		result.push_back(std::move(sd));
	}

	return result;
}
