#pragma once
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <stdexcept>

///////////////////////////////////////////////////////////////////////////////

#define MAP_EXCHANGE_FLOAT(name, value, def_val)            \
	if( f.loading() )                                       \
	{                                                       \
	    if( !f.getObjectAttribute(#name, (float&) value) )  \
	        value = def_val;                                \
	}                                                       \
	else                                                    \
	{                                                       \
	    f.setObjectAttribute(#name, (float) value);         \
	}

#define MAP_EXCHANGE_INT(name, value, def_val)              \
	if( f.loading() )                                       \
	{                                                       \
	    if( !f.getObjectAttribute(#name, (int&) value) )    \
	        value = def_val;                                \
	}                                                       \
	else                                                    \
	{                                                       \
	    f.setObjectAttribute(#name, (int) value);           \
	}

#define MAP_EXCHANGE_STRING(name, value, def_val)           \
	if( f.loading() )                                       \
	{                                                       \
	    if( !f.getObjectAttribute(#name, value) )           \
	        value = def_val;                                \
	}                                                       \
	else                                                    \
	{                                                       \
	    f.setObjectAttribute(#name, value);                 \
	}


////////////////////////////////////////////////////////////

namespace detail
{
	inline constexpr uint32_t MakeSignature(char const s[5])
	{
		return s[0] | (s[1] << 8) | (s[2] << 16) | (s[3] << 24);
	}
}

namespace FS
{
	struct Stream;
}


class MapFile
{
#pragma warning(push)
#ifdef __INTEL_COMPILER
# pragma warning(disable: 1899)
#endif
	enum enumChunkTypes : uint32_t
	{
		CHUNK_HEADER_OPEN  = detail::MakeSignature("hdr{"),
		CHUNK_ATTRIB       = detail::MakeSignature("attr"),
		CHUNK_HEADER_CLOSE = detail::MakeSignature("}hdr"),
		CHUNK_OBJDEF       = detail::MakeSignature("dfn:"),
		CHUNK_OBJECT       = detail::MakeSignature("obj:"),
		CHUNK_TEXTURE      = detail::MakeSignature("tex:"),
	};
#pragma warning(pop)

	enum enumDataTypes : uint32_t
	{
		DATATYPE_INT    = 1,
		DATATYPE_FLOAT  = 2,
		DATATYPE_STRING = 3,  // max length is 0xffff
	};

	struct ChunkHeader
	{
		enumChunkTypes chunkType;
		uint32_t       chunkSize;
	};

	struct AttributeSet
	{
		std::map<std::string, int, std::less<>> attrs_int;
		std::map<std::string, float, std::less<>> attrs_float;
		std::map<std::string, std::string, std::less<>> attrs_str;

		void clear()
		{
			attrs_int.clear();
			attrs_float.clear();
			attrs_str.clear();
		}
	};

	struct ObjectDefinition
	{
		struct Property
		{
			Property(Property &&other)
				: _type(other._type)
				, name(std::move(other.name))
			{
				switch (_type)
				{
					case DATATYPE_INT:
						value_int = other.value_int;
						break;
					case DATATYPE_FLOAT:
						value_float = other.value_float;
						break;
					case DATATYPE_STRING:
						new (&value_string) std::string(std::move(other.value_string));
						break;
				}
			}
			Property(enumDataTypes type)
				: _type(type)
			{
				switch (_type)
				{
					case DATATYPE_INT:
						value_int = 0;
						break;
					case DATATYPE_FLOAT:
						value_float = 0;
						break;
					case DATATYPE_STRING:
						new (&value_string) std::string();
						break;
					default:
						throw std::runtime_error("Unknown data type");
				}
			}
			~Property()
			{
				switch (_type)
				{
					case DATATYPE_INT:
					case DATATYPE_FLOAT:
						break;
					case DATATYPE_STRING:
						value_string.~basic_string();
						break;
				}
			}
			const enumDataTypes _type;
			std::string   name;
			union
			{
				int value_int;
				float value_float;
				std::string value_string;
			};
		};

		std::string           className;
		std::vector<Property> propertyDefinitions;
	};

private:
	FS::Stream &_file;
	std::unique_ptr<char[]> _buffer;
	unsigned int _bufferSize = 0;
	bool _modeWrite;
	bool _headerWritten = false;
	bool _isNewClass = false;

	AttributeSet _mapAttrs;

	std::vector<ObjectDefinition> _managed_classes;
	std::map<std::string, int, std::less<>> _name_to_index; // map classname to index in _managed_classes

	int _objType; // index in _managed_classes
	int _numProperties; // in current object

	bool _read_chunk_header(ChunkHeader &chdr);
	void _skip_block(size_t size);

	void WriteHeader();

	void WriteInt(int value);
	void WriteFloat(float value);
	void WriteString(std::string_view value);
	void WriteData(const void *data, size_t size);
	inline void* GetWriteBuffer(size_t size);
	template<class T> T& GetWriteBuffer()
	{
		return *reinterpret_cast<T*>(GetWriteBuffer(sizeof(T)));
	}

	void ReadInt(int &value);
	void ReadFloat(float &value);
	void ReadString(std::string &value);

public:
	MapFile(FS::Stream &stream, bool write);
	MapFile(const MapFile&) = delete;
	MapFile& operator=(const MapFile&) = delete;
	~MapFile();

	bool loading() const;

	bool ReadNextObject();
	std::string_view GetCurrentClassName() const;

	void BeginObject(std::string_view classname);
	void WriteCurrentObject();
	void WriteEndOfFile();

	bool getMapAttribute(std::string_view name, int &value) const;
	bool getMapAttribute(std::string_view name, float &value) const;
	bool getMapAttribute(std::string_view name, std::string &value) const;

	void setMapAttribute(std::string name, int value);
	void setMapAttribute(std::string name, float value);
	void setMapAttribute(std::string name, std::string value);

	bool getObjectAttribute(std::string_view name, int &value) const;
	bool getObjectAttribute(std::string_view name, float &value) const;
	bool getObjectAttribute(std::string_view name, std::string &value) const;

	void setObjectAttribute(std::string_view name, int value);
	void setObjectAttribute(std::string_view name, float value);
	void setObjectAttribute(std::string_view name, std::string_view value);

	template<class T>
	void Exchange(std::string_view name, T *value, T defaultValue)
	{
		assert(value);
		if( loading() )
		{
			if( !getObjectAttribute(name, *value) )
				value = defaultValue;
		}
		else
		{
			setObjectAttribute(name, *value);
		}
	}
};
