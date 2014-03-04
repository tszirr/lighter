#pragma once

#include "filex"
#include <algorithm>

#include <sys/types.h>
#include <sys/stat.h>

#ifdef WIN32
	#include <Windows.h>
#endif

namespace stdx
{
	long long file_time(char const* name)
	{
		struct stat buf;
		stat(name, &buf);
		return static_cast<long long>(buf.st_mtime);
	}

	std::string load_file(char const* name)
	{
		std::string str;

		auto t = read_file(name);
		t.seekg(0, std::ios::end);   
		str.reserve((size_t) t.tellg());
		t.seekg(0, std::ios::beg);

		str.assign(std::istreambuf_iterator<char>(t), std::istreambuf_iterator<char>());
		return str;
	}
	
	std::vector<char> load_binary_file(char const* name, bool nullterminated)
	{
		std::vector<char> data;

		auto t = read_binary_file(name);
		t.seekg(0, std::ios::end);
		size_t fileSize = (size_t) t.tellg();
		data.resize(fileSize + (size_t) nullterminated);
		t.seekg(0, std::ios::beg);

		t.read(data.data(), fileSize);
		return data;
	}

#ifdef WIN32
	namespace detail
	{
		namespace generic_file
		{
			inline DWORD get_windows_access_flags(unsigned access)
			{
				DWORD winAccess = 0;

				if (access & file_flags::read) winAccess |= GENERIC_READ;
				if (access & file_flags::write) winAccess |= GENERIC_WRITE;

				// Always require some kind of access
				if (!winAccess) winAccess = GENERIC_READ;

				return winAccess;
			}

			inline DWORD get_windows_sharing_flags(unsigned share, unsigned access)
			{
				DWORD winShare = 0;
				if (share & file_flags::read) winShare |= FILE_SHARE_READ;
				if (share & file_flags::write) winShare |= FILE_SHARE_WRITE;
				return winShare;
			}

			inline DWORD get_windows_open_mode(file_flags::open_mode mode, unsigned access)
			{
				if (access & file_flags::write)
					switch (mode)
					{
						case file_flags::nonexisting: return CREATE_NEW;
						case file_flags::existing: return OPEN_EXISTING;
						case file_flags::new_overwrite: return CREATE_ALWAYS;
						default: case file_flags::open_or_new: return OPEN_ALWAYS;
					}
				else
					return OPEN_EXISTING;
			}

			inline DWORD get_windows_optimization_flags(unsigned hints)
			{
				if (hints & file_flags::sequential) return FILE_FLAG_SEQUENTIAL_SCAN;
				else if (hints & file_flags::random) return FILE_FLAG_RANDOM_ACCESS;
				else return 0;
			}

			typedef BOOL (WINAPI* PrefetchVirtualMemoryPtr)(HANDLE hProcess, ULONG_PTR NumberOfEntries, PWIN32_MEMORY_RANGE_ENTRY VirtualAddresses, ULONG Flags);
			inline PrefetchVirtualMemoryPtr get_prefetch_function()
			{
				DWORD dwVersion = GetVersion();
				DWORD dwMajor = LOBYTE(LOWORD(dwVersion));
				DWORD dwMinor = HIBYTE(LOWORD(dwVersion));

				// supported from Win 8 onwards
				return (dwMajor >= 6 && dwMinor >= 2)
					? (PrefetchVirtualMemoryPtr) GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "PrefetchVirtualMemory")
					: nullptr;
			}

			template <class Pointer, BOOL (WINAPI* Deleter)(Pointer)>
			struct win_delete
			{
				typedef Pointer pointer;
				void operator ()(pointer ptr) const
				{
					if (ptr)
						(*Deleter)(ptr);
				}

				typedef std::unique_ptr< Pointer, win_delete<Pointer, Deleter> > pointer_type;
				typedef stdx::unique_handle< Pointer, win_delete<Pointer, Deleter> > handle_type;
			};
		}
	}

	mapped_file::mapped_file(char const* name, size_t size, unsigned access, open_mode mode,
	                         unsigned share, unsigned hints)
	{
		typedef detail::generic_file::win_delete<HANDLE, CloseHandle>::handle_type winhandle;

		winhandle file( ::CreateFileA(
			  name // todo: from utf8?
			, detail::generic_file::get_windows_access_flags(access)
			, detail::generic_file::get_windows_sharing_flags(share, access)
			, nullptr
			, detail::generic_file::get_windows_open_mode(mode, access)
			, detail::generic_file::get_windows_optimization_flags(hints)
			, NULL
			) );
		if (file.get() == INVALID_HANDLE_VALUE)
			throwx(std::runtime_error(name));

		// Resize first to avoid creating the mapping twice
		if ((access & file_flags::write) && size != 0)
		{
			LONGLONG longSize = size;
			::SetFilePointerEx(file
				, reinterpret_cast<const LARGE_INTEGER&>(static_cast<const LONGLONG&>(0))
				, reinterpret_cast<LARGE_INTEGER*>(&longSize)
				, FILE_BEGIN);
		}

		// Handles size of 0 equal to current file size
		winhandle mapping( ::CreateFileMappingW(file
			, nullptr
			, (access & file_flags::write) ? PAGE_READWRITE : PAGE_READONLY
			, 0
			, 0
			, nullptr
			) );
		if (mapping.get() == NULL)
			throwx(std::runtime_error(name));

		LONGLONG longSize;
		if (!::GetFileSizeEx(file, reinterpret_cast<LARGE_INTEGER*>(&longSize)))
			throwx(std::runtime_error(name));

        // Handles size of 0 equal to end of file
		this->data = (char*) ::MapViewOfFile(mapping
			, (access & file_flags::write) ? (FILE_MAP_READ | FILE_MAP_WRITE) : FILE_MAP_READ
			, 0
			, 0
			, 0
			);
		if (!this->data)
			throwx(std::runtime_error(name));

		this->size = static_cast<size_t>(longSize);
	}

	mapped_file::~mapped_file()
	{
		if (data)
			::UnmapViewOfFile(data);
	}

	void mapped_file::prefetchAll()
	{
		static auto PrefetchVirtualMemory = detail::generic_file::get_prefetch_function();
		if (PrefetchVirtualMemory)
		{
			static HANDLE process = GetCurrentProcess();
			WIN32_MEMORY_RANGE_ENTRY prefetchRange = { data, size };
			(*PrefetchVirtualMemory)(process, 1, &prefetchRange, 0);
		}
	}

#endif
	
	namespace detail
	{
		namespace process_includes
		{
			inline char const* string_find(stdx::data_range_param<char const> str, char const val, char const* cursor)
			{
				return std::find(cursor, str.end(), val);
			}

			inline char const* string_find(stdx::data_range_param<char const> str, char const* val, char const* cursor)
			{
				return std::search(cursor, str.end(), val, val + strlen(val));
			}

			template <size_t Size>
			inline char const* string_find(stdx::data_range_param<char const> str, char const (&val)[Size], char const* cursor)
			{
				return std::search(cursor, str.end(), val, val + Size - 1);
			}
		}
	}

	std::string process_includes_(stdx::data_range_param<char const> src, char const* filename
		, include_resolver const& resolve_include, stdx::data_range_param<char const> preamble
		, bool int_file_id)
	{
		using namespace detail::process_includes;

		std::stringstream result;
		if (!preamble.empty())
			result << preamble << std::endl;

		char const *srcBegin = src.data(), *srcEnd = srcBegin + src.size();
		auto nextSrcCursor = srcBegin;
		size_t nextSrcLine = 1;

		while (nextSrcCursor < srcEnd)
		{
			auto nextIncludeCursor = string_find(src, "#include", nextSrcCursor);

			result << "#line " << nextSrcLine;
			if (!int_file_id)
				result << " \"" << filename << "\"\n";
			else
				result << " " << (int) filename[0] << "\n";
			result.write(nextSrcCursor, nextIncludeCursor - nextSrcCursor);

			nextSrcLine += std::count(nextSrcCursor, nextIncludeCursor, '\n');
			nextSrcCursor = nextIncludeCursor;

			if (nextIncludeCursor < srcEnd)
			{
				auto nextIncludeEnd = string_find(src, '\n', nextIncludeCursor);

				auto localInclueStart = string_find(src, '"', nextIncludeCursor);
				auto systemInclueStart = string_find(src, '<', nextIncludeCursor);

				bool localInclude = true;
				std::string includeName;

				if (localInclueStart < nextIncludeEnd)
				{
					includeName.assign(localInclueStart + 1, string_find(src, '"', localInclueStart + 1));
					localInclude = true;
				}
				else if (systemInclueStart < nextIncludeEnd)
				{
					includeName.assign(systemInclueStart + 1, string_find(src, '>', systemInclueStart + 1));
					localInclude = false;
				}

				result << resolve_include.resolve(includeName.c_str(), localInclude) << '\n';

				// Skip #include directive
				nextSrcCursor = nextIncludeEnd;
				if (nextSrcCursor < srcEnd)
					++nextSrcCursor;
				++nextSrcLine;
			}
		}

		return result.str();
	}

} // namespace
