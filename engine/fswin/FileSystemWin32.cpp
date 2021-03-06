#include "inc/fswin/FileSystemWin32.h"
#include <utf8.h>
#include <algorithm>
#include <cassert>
#include <sstream>
#include <string_view>

using namespace FS;

static std::string StrFromErr(DWORD dwMessageId)
{
	WCHAR msgBuf[1024];
	DWORD msgSize = FormatMessageW(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,
		dwMessageId,
		0,
		msgBuf,
		1024,
		nullptr);
	while (msgSize)
	{
		if (msgBuf[msgSize - 1] == L'\n' || msgBuf[msgSize - 1] == L'\r')
		{
			--msgSize;
		}
		else
		{
			break;
		}
	}

	if (msgSize)
	{
		std::string result;
		utf8::utf16to8(msgBuf, msgBuf + msgSize, std::back_inserter(result));
		return result;
	}
	else
	{
		std::ostringstream ss;
		ss << "Unknown error (" << dwMessageId << ")";
		return ss.str();
	}
}

static AutoHandle OpenFileWin32(std::wstring fileName, FileMode mode)
{
	AutoHandle file;

	std::replace(fileName.begin(), fileName.end(), L'/', L'\\');

	DWORD dwDesiredAccess = 0;
	DWORD dwShareMode = FILE_SHARE_READ;
	DWORD dwCreationDisposition = 0;

	assert(mode);
	if (mode & ModeWrite)
	{
		dwDesiredAccess |= FILE_WRITE_DATA;
		dwShareMode = 0;
		dwCreationDisposition = CREATE_ALWAYS;
	}
	if (mode & ModeRead)
	{
		dwDesiredAccess |= FILE_READ_DATA;
		dwCreationDisposition = (mode & ModeWrite) ? OPEN_ALWAYS : OPEN_EXISTING;
	}

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
	file.h = ::CreateFileW(fileName.c_str(),
		dwDesiredAccess,
		dwShareMode,
		nullptr,                        // lpSecurityAttributes
		dwCreationDisposition,
		FILE_FLAG_SEQUENTIAL_SCAN,      // dwFlagsAndAttributes
		nullptr);                       // hTemplateFile
#elif WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_PC_APP)
	file.h = ::CreateFile2(fileName.c_str(),
		dwDesiredAccess,
		dwShareMode,
		dwCreationDisposition,
		nullptr);
#endif

	return file;
}

FileWin32::FileWin32(AutoHandle file, FileMode mode)
	: _mode(mode)
	, _file(std::move(file))
	, _mapped(false)
	, _streamed(false)
{
	assert(_mode);
	assert(INVALID_HANDLE_VALUE != _file.h);
}

FileWin32::~FileWin32()
{
}

std::shared_ptr<MemMap> FileWin32::QueryMap()
{
	assert(!_mapped && !_streamed);
	std::shared_ptr<MemMap> result = std::make_shared<MemMapWin32>(shared_from_this(), _file.h);
	_mapped = true;
	return result;
}

std::shared_ptr<Stream> FileWin32::QueryStream()
{
	assert(!_mapped && !_streamed);
	if (_mode == ModeRead)
	{
		_mapped = true;
		return std::make_shared<MemMapWin32>(shared_from_this(), _file.h);
	}
	else
	{
		_streamed = true;
		return std::make_shared<StreamWin32>(shared_from_this(), _file.h);
	}
}

void FileWin32::Unmap()
{
	assert(_mapped && !_streamed);
	_mapped = false;
}

void FileWin32::Unstream()
{
	assert(_streamed && !_mapped);
	_streamed = false;
}

///////////////////////////////////////////////////////////////////////////////

StreamWin32::StreamWin32(std::shared_ptr<FileWin32> parent, HANDLE hFile)
	: _file(parent)
	, _hFile(hFile)
{
	Seek(0, SEEK_SET);
}

StreamWin32::~StreamWin32()
{
	_file->Unstream();
}

size_t StreamWin32::Read(void *dst, size_t size, size_t count)
{
	DWORD bytesRead;
	if( !ReadFile(_hFile, dst, size * count, &bytesRead, nullptr) )
	{
		throw std::runtime_error(StrFromErr(GetLastError()));
	}
	if( bytesRead % size )
	{
		throw std::runtime_error("unexpected end of file");
	}
	return bytesRead / size;
}

void StreamWin32::Write(const void *src, size_t size)
{
	DWORD written;
	BOOL result = WriteFile(_hFile, src, size, &written, nullptr);
	if( !result || written != size )
	{
		throw std::runtime_error(StrFromErr(GetLastError()));
	}
}

void StreamWin32::Seek(long long amount, unsigned int origin)
{
	DWORD dwMoveMethod;
	switch( origin )
	{
	case SEEK_SET: dwMoveMethod = FILE_BEGIN; break;
	case SEEK_CUR: dwMoveMethod = FILE_CURRENT; break;
	case SEEK_END: dwMoveMethod = FILE_END; break;
	default:
		assert(false);
	}
	LARGE_INTEGER result;
	LARGE_INTEGER liAmount;
	liAmount.QuadPart = amount;
	if( !SetFilePointerEx(_hFile, liAmount, &result, dwMoveMethod) )
	{
		throw std::runtime_error(StrFromErr(GetLastError()));
	}
}

long long StreamWin32::Tell() const
{
	LARGE_INTEGER zero = { 0 };
	LARGE_INTEGER current = { 0 };
	SetFilePointerEx(_hFile, zero, &current, FILE_CURRENT);
	return current.QuadPart;
}

///////////////////////////////////////////////////////////////////////////////

MemMapWin32::MemMapWin32(std::shared_ptr<FileWin32> parent, HANDLE hFile)
	: _file(std::move(parent))
	, _hFile(hFile)
	, _data(nullptr)
	, _size(0)
{
	SetupMapping();
}

MemMapWin32::~MemMapWin32()
{
	if( _data )
	{
		UnmapViewOfFile(_data);
	}
	_file->Unmap();
}

void MemMapWin32::SetupMapping()
{
	LARGE_INTEGER size;
	if( !GetFileSizeEx(_hFile, &size) )
	{
		throw std::runtime_error(StrFromErr(GetLastError()));
	}

	if (size.HighPart > 0)
	{
		throw std::runtime_error("File is too large");
	}

	_size = size.LowPart;

	_map.h = CreateFileMapping(_hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
	if( nullptr == _map.h )
	{
		throw std::runtime_error(StrFromErr(GetLastError()));
	}

	_data = MapViewOfFile(_map.h, FILE_MAP_READ, 0, 0, 0);
	if( !_data )
	{
		throw std::runtime_error(StrFromErr(GetLastError()));
	}
}

const void* MemMapWin32::GetData() const
{
	return _data;
}

unsigned long MemMapWin32::GetSize() const
{
	return _size;
}

void MemMapWin32::SetSize(unsigned long size)
{
	BOOL bUnmapped = UnmapViewOfFile(_data);
	_data = nullptr;
	_size = 0;
	if( !bUnmapped )
	{
		throw std::runtime_error(StrFromErr(GetLastError()));
	}

	CloseHandle(_map.h);

	LARGE_INTEGER distance = { size };
	if( !SetFilePointerEx(_hFile, distance, nullptr, FILE_BEGIN) )
	{
		throw std::runtime_error(StrFromErr(GetLastError()));
	}
	if( !SetEndOfFile(_hFile) )
	{
		throw std::runtime_error(StrFromErr(GetLastError()));
	}

	SetupMapping();
	assert(_size == size);
}

size_t MemMapWin32::Read(void *dst, size_t size, size_t count)
{
	size_t bytes = size * count;
	if (_pos + bytes > _size)
	{
		return 0;
	}
	memcpy(dst, (const char*)_data + _pos, bytes);
	_pos += bytes;
	return count;
}

void MemMapWin32::Write(const void *src, size_t size)
{
	assert(false);
}

void MemMapWin32::Seek(long long amount, unsigned int origin)
{
	switch (origin)
	{
	case SEEK_SET:
		assert(amount >= 0 && amount <= _size);
		_pos = static_cast<size_t>(amount);
		break;
	case SEEK_CUR:
		assert((long long)_pos + amount >= 0 && (long long)_pos + amount <= _size);
		_pos += static_cast<size_t>(amount);
		break;
	case SEEK_END:
		assert(amount <= 0 && (long long)_pos + amount >= 0);
		_pos = _size + static_cast<size_t>(amount);
		break;
	default:
		assert(false);
	}
}

///////////////////////////////////////////////////////////////////////////////

static std::wstring WinPathCombine(std::wstring_view first, std::string_view second)
{
	// result = first + '\\' + second
	std::wstring result;
	result.reserve(first.size() + second.size() + 1);
	result.append(first);
	result.append(1, L'\\');
	utf8::utf8to16(second.begin(), second.end(), std::back_inserter(result));
	return result;
}

static std::string w2s(std::wstring_view w)
{
	std::string s;
	s.reserve(w.size());
	utf8::utf16to8(w.begin(), w.end(), std::back_inserter(s));
	return s;
}

FileSystemWin32::FileSystemWin32(std::wstring rootDirectory)
	: _rootDirectory(std::move(rootDirectory))
{
}

FileSystemWin32::FileSystemWin32(std::string_view rootDirectory)
{
	_rootDirectory.reserve(rootDirectory.size());
	utf8::utf8to16(rootDirectory.begin(), rootDirectory.end(), std::back_inserter(_rootDirectory));
}

std::vector<std::string> FileSystemWin32::EnumAllFiles(std::string_view mask)
{
	std::wstring query = WinPathCombine(_rootDirectory, mask);

	WIN32_FIND_DATAW fd;
	HANDLE hSearch = FindFirstFileExW(
		query.c_str(),
		FindExInfoStandard,
		&fd,
		FindExSearchNameMatch,
		nullptr,
		0);
	if( INVALID_HANDLE_VALUE == hSearch )
	{
		if( ERROR_FILE_NOT_FOUND == GetLastError() )
		{
			return std::vector<std::string>(); // nothing matches
		}
		throw std::runtime_error(StrFromErr(GetLastError()));
	}

	std::vector<std::string> files;
	do
	{
		if( 0 == (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
		{
			files.push_back(w2s(fd.cFileName));
		}
	}
	while( FindNextFileW(hSearch, &fd) );
	FindClose(hSearch);
	return files;
}

std::shared_ptr<File> FileSystemWin32::RawOpen(std::string_view fileName, FileMode mode, bool nothrow)
{
	// combine with the root path
	auto file = OpenFileWin32(WinPathCombine(_rootDirectory, fileName), mode);
	if (INVALID_HANDLE_VALUE == file.h)
	{
		if (nothrow)
			return nullptr;
		else
			throw std::runtime_error(StrFromErr(GetLastError()));
	}
	return std::make_shared<FileWin32>(std::move(file), mode);
}

std::shared_ptr<FileSystem> FileSystemWin32::GetFileSystem(std::string_view path, bool create, bool nothrow)
try
{
	if (auto fs = FileSystem::GetFileSystem(path, create, true))
	{
		return fs;
	}

	assert(!path.empty());

	// skip delimiters at the beginning
	std::string::size_type offset = path.find_first_not_of('/');
	assert(std::string::npos != offset);

	auto p = path.find('/', offset);
	auto dirName = path.substr(offset, std::string::npos != p ? p - offset : p);
	auto tmpDir = WinPathCombine(_rootDirectory, dirName);

	// try to find directory
	WIN32_FIND_DATAW fd = {0};
	HANDLE search = FindFirstFileExW(
		tmpDir.c_str(),
		FindExInfoStandard,
		&fd,
		FindExSearchNameMatch,
		nullptr,
		0);

	if (INVALID_HANDLE_VALUE != search)
	{
		FindClose(search);
	}
	else
	{
		if( create )
		{
			if (!CreateDirectoryW(tmpDir.c_str(), nullptr))
			{
				// creation failed
				if( nothrow )
					return nullptr;
				else
					throw std::runtime_error(StrFromErr(GetLastError()));
			}
			else
			{
				// try searching again to get attributes
				HANDLE search2 = FindFirstFileExW(
					tmpDir.c_str(),
					FindExInfoStandard,
					&fd,
					FindExSearchNameMatch,
					nullptr,
					0);
				FindClose(search2);
				if (INVALID_HANDLE_VALUE == search2)
				{
					if (nothrow)
						return nullptr;
					else
						throw std::runtime_error(StrFromErr(GetLastError()));
				}
			}
		}
		else
		{
			// directory not found
			if( nothrow )
				return nullptr;
			else
				throw std::runtime_error(StrFromErr(GetLastError()));
		}
	}

	if( 0 == (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
		throw std::runtime_error("object is not a directory");

	auto child = std::make_shared<FileSystemWin32>(tmpDir);
	Mount(dirName, child);

	if( std::string::npos != p )
		return child->GetFileSystem(path.substr(p), create, nothrow); // process the rest of the path
	return child; // last path node was processed
}
catch (const std::exception&)
{
	std::ostringstream ss;
	ss << "Failed to open directory '" << path << "'";
	std::throw_with_nested(std::runtime_error(ss.str()));
}
