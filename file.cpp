#pragma once

#include "filex"
#include <algorithm>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utime.h>

#ifdef WIN32
	#include <Windows.h>
	#include <cstdlib>

	#include <Shobjidl.h>
	#include <Shlobj.h>
	#include <locale>
	#include <codecvt>

#else
	#include <libgen.h>
#endif

namespace stdx
{
	long long file_time(char const* name)
	{
		struct stat buf;
		stat(name, &buf);
		return static_cast<long long>(buf.st_mtime);
	}

	bool file_touch(char const* name)
	{
		return utime(name, nullptr) == 0;
	}

	std::string dirname(char const* path)
	{
		std::string r(path);
#ifdef WIN32
		_splitpath(path, nullptr, &r[0], nullptr, nullptr);
#else
		auto n = ::dirname(&r[0]);
		assert (n == &r[0]);
		if (n != &r[0]) strcpy(&r[0], n);
#endif
		r.resize(strlen(r.data()));
		return r;
	}

	std::string basename(char const* path)
	{
		std::string r(path);
		std::string e(path);
#ifdef WIN32
		_splitpath(path, nullptr, nullptr, &r[0], &e[0]);
		strcpy(&r[strlen(r.data())], e.data());
#else
		auto n = ::basename(&r[0]);
		assert (n == &r[0]);
		if (n != &r[0]) strcpy(&r[0], n);
#endif
		r.resize(strlen(r.data()));
		return r;
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

	namespace detail
	{
		namespace prompt_file
		{
			#define throw_com_error(x) if (FAILED(x)) throwx( std::runtime_error("COM/Windows Shell") );

			struct COM
			{
				COM()
				{
					throw_com_error(CoInitializeEx(NULL, COINIT_MULTITHREADED));
				}
				~COM()
				{
					CoUninitialize();
				}
			};

			void prepareCOM()
			{
				static COM com;
			}

			struct com_delete
			{
				void operator ()(IUnknown* ptr) const
				{
					if (ptr)
						ptr->Release();
				}
			};
			
			template <class T>
			struct com_handle_t
			{
				typedef stdx::unique_handle<T, com_delete> t;
			};
		}

	} // namespace

	std::vector<std::string> prompt_file(char const* current, char const* extensions
		, dialog::t mode, bool multi)
	{
		std::vector<std::string> result;

		using namespace detail::prompt_file;
		prepareCOM();

		com_handle_t<IFileDialog>::t pfd;
		auto dialogCLSID = (mode != dialog::save) ? CLSID_FileOpenDialog : CLSID_FileSaveDialog;
		throw_com_error(CoCreateInstance(dialogCLSID, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(pfd.rebind())));

		// Options
		{
			DWORD dwFlags = 0;
			pfd->GetOptions(&dwFlags);
			dwFlags |= FOS_FORCEFILESYSTEM | FOS_NOCHANGEDIR;
			if (mode == dialog::folder)
				dwFlags |= FOS_PICKFOLDERS;
			if (multi)
				dwFlags |= FOS_ALLOWMULTISELECT;
			throw_com_error(pfd->SetOptions(dwFlags));
		}

		std::wstring_convert< std::codecvt_utf8_utf16<wchar_t> > utfcvt;

		// Extensions
		if (extensions)
		{
			auto typeStr = utfcvt.from_bytes(extensions);
			auto typeCStr = typeStr.c_str();
			size_t typeCnt = 1 + std::count(typeStr.begin(), typeStr.end(), L'|');
			std::vector<COMDLG_FILTERSPEC> types(typeCnt);

			for (size_t i = 0, off = 0; i < typeCnt; ++i)
			{
				size_t nextOff = typeStr.find('|', off);
				if (nextOff != typeStr.npos)
					typeStr[nextOff++] = 0;

				auto& type = types[i];
				type.pszName = type.pszSpec = typeCStr + off;

				auto ass = typeStr.find('=', off);
				if (ass < nextOff)
				{
					typeStr[ass++] = 0;
					type.pszSpec = typeCStr + ass;
				}

				off = nextOff;
			}

			throw_com_error(pfd->SetFileTypes(UINT(typeCnt), types.data()));
		}

		// Initial folder
		if (current)
		{
			SFGAOF folderAtt;
			com_handle_t<IShellItem>::t item;
			{
				struct abs_iid_deleter
				{
					void operator ()(ITEMIDLIST* ptr) const
					{
						if (ptr)
							ILFree(ptr);
					}
				};
				stdx::unique_handle<ITEMIDLIST, abs_iid_deleter> iidl;
			
				throw_com_error(SHParseDisplayName(utfcvt.from_bytes(current).c_str(), nullptr, iidl.rebind(), SFGAO_FOLDER, &folderAtt));
				throw_com_error(SHCreateItemFromIDList(iidl, IID_PPV_ARGS(item.rebind())));
			}

			if (~folderAtt & SFGAO_FOLDER)
			{
				com_handle_t<IShellItem>::t folder;
				item->GetParent(folder.rebind());
				item = std::move(folder);
			}

			throw_com_error(pfd->SetFolder(item));
		}

		// Show the dialog
		if (SUCCEEDED(pfd->Show(NULL)))
		{
			auto&& getPath = [&utfcvt](IShellItem& item) -> std::string
			{
				PWSTR pszFilePath = NULL;
				throw_com_error(item.GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath));
				return utfcvt.to_bytes(pszFilePath);
			};

			if (multi)
			{
				com_handle_t<IFileOpenDialog>::t ofd;
				throw_com_error(pfd->QueryInterface(IID_PPV_ARGS(ofd.rebind())));

				com_handle_t<IShellItemArray>::t psiResults;
				throw_com_error(ofd->GetResults(psiResults.rebind()));

				DWORD resultCount = 0;
				psiResults->GetCount(&resultCount);
				for (DWORD i = 0; i < resultCount; ++i)
				{
					com_handle_t<IShellItem>::t psiResult;
					throw_com_error(psiResults->GetItemAt(i, psiResult.rebind()));
					result.push_back( getPath(*psiResult) );
				}
			}
			else
			{
				com_handle_t<IShellItem>::t psiResult;
				throw_com_error(pfd->GetResult(psiResult.rebind()));
				result.push_back( getPath(*psiResult) );
			}
		}

		return result;
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
