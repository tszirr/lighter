#include "filex"
#include <algorithm>
#include <iostream>

#include <sys/types.h>
#include <sys/stat.h>

#ifdef WIN32
	#include <sys/utime.h>

	#define NOMINMAX
	#include <Windows.h>
	#include <cstdlib>

	#define STRICT_TYPED_ITEMIDS
	#include <Shobjidl.h>
	#include <Shlobj.h>
	#include <locale>
	#include <codecvt>

	#pragma comment(linker, "\"/manifestdependency:type='Win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
	#include <utime.h>
	#include <libgen.h>
#endif

namespace stdx
{
	long long file_time(char const* name)
	{
		struct stat buf = { 0 };
		stat(name, &buf);
		return static_cast<long long>(buf.st_mtime);
	}

	bool file_touch(char const* name)
	{
		return utime(name, nullptr) == 0;
	}
	
	namespace detail
	{
		namespace relative_path
		{
			inline bool is_separator(char c) { return c == '\\' || c == '/'; }
			inline bool is_separator_or_null(char c) { return c == 0 || is_separator(c); }

			size_t const backout_size = 3;
			inline char* append_backout(char* cursor) { *cursor++ = '.'; *cursor++ = '.'; *cursor++ = '\\'; return cursor; }

			size_t const separator_size = 1;
			inline char* append_separator(char* cursor) { *cursor++ = '\\'; return cursor; }

			stdx::range<char const*> next_dir(char const* cursor)
			{
				while (is_separator(*cursor)) ++cursor;
				stdx::range<char const*> r(cursor, cursor);
				while (!is_separator_or_null(*r.last)) ++r.last;
				return r;
			}
		}
	}

	std::string dirname(char const* path)
	{
		
#ifdef WIN32
		auto lastSeparator = path;
		for (auto it = path; *it; ++it)
			if (detail::relative_path::is_separator(*it))
				lastSeparator = it;
		return std::string(path, lastSeparator);
#else
		std::string r(path);
		auto n = ::dirname(&r[0]);
		assert (n == &r[0]);
		if (n != &r[0]) strcpy(&r[0], n);
		r.resize(strlen(r.c_str()));
		return r;
#endif
	}

	std::string basename(char const* path)
	{
#ifdef WIN32
		auto lastSeparator = path;
		for (auto it = path; *it; ++it)
			if (detail::relative_path::is_separator(*it))
				lastSeparator = it;
		return std::string(lastSeparator + 1);
#else
		std::string r(path);
		auto n = ::basename(&r[0]);
		assert (n == &r[0]);
		if (n != &r[0]) strcpy(&r[0], n);
		r.resize(strlen(r.c_str()));
		return r;
#endif
	}

	std::string realpath(char const* path)
	{
		struct default_free { void operator ()(void* p) const { free(p); } };
		std::unique_ptr<char, default_free> absolutePath(
#ifdef WIN32
				_fullpath(nullptr, path, 0)
#else
				::realpath(path, nullptr)
#endif
			);
		return absolutePath.get();
	}

	std::string concat_path(char const* tail, char const* head)
	{
		using namespace detail::relative_path;

		auto tailEnd = tail + strlen(tail);
		bool addSeparator = (tail != tailEnd && !is_separator(tailEnd[-1]));

		std::string concat;
		concat.resize(tailEnd - tail + addSeparator + strlen(head));
		auto concatCursor = &concat[0];
		strcpy(concatCursor, tail);
		concatCursor += tailEnd - tail;
		if (addSeparator) concatCursor = append_separator(concatCursor);
		strcpy(concatCursor, head);
		return concat;
	}

	std::string filesys_relative_path(char const* from, char const* to)
	{
		return relative_path(realpath(from).c_str(), realpath(to).c_str());
	}

	std::string relative_path(char const* from, char const* to)
	{
		using namespace detail::relative_path;

		auto fromCursor = from, toCursor = to;

		while (true)
		{
			auto fromDir = next_dir(fromCursor);
			auto toDir = next_dir(toCursor);
			if (!fromDir.empty() && fromDir.size() == toDir.size() && strncmp(fromDir.first, toDir.first, fromDir.size()) == 0)
			{
				fromCursor = fromDir.last;
				toCursor = toDir.last;
				continue;
			}
			break;
		}

		size_t numBackout = 0;

		while (true)
		{
			auto fromDir = next_dir(fromCursor);
			if (!fromDir.empty())
			{
				++numBackout;
				fromCursor = fromDir.last;
				continue;
			}
			break;
		}

		while (is_separator(*toCursor)) ++toCursor;

		std::string relative;
		relative.resize(numBackout * backout_size + strlen(toCursor));
		auto relativeCursor = &relative[0];
		for (size_t i = 0; i < numBackout; ++i)
			relativeCursor = append_backout(relativeCursor);
		strcpy(relativeCursor, toCursor);
		return relative;
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

			typedef win_delete<HANDLE, CloseHandle>::handle_type winhandle;
		}
	}

	mapped_file::mapped_file(char const* name, size_t size, unsigned access, open_mode mode,
	                         unsigned share, unsigned hints)
	{
		typedef detail::generic_file::winhandle winhandle;

		// always give read access
		// note: if we ever remove this, we need differentiated access flags in CreateFileMappingW and MapViewOfFile!
		access |= file_flags::read;

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
			BOOL success = ::SetFilePointerEx(file
				, reinterpret_cast<const LARGE_INTEGER&>(longSize)
				, nullptr
				, FILE_BEGIN)
			&& ::SetEndOfFile(file);
			if (!success)
				throwx(std::runtime_error(name));
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
			#define throw_com_error(x) do { \
					auto winErr = (x); \
					if (FAILED(winErr)) throwx( std::runtime_error(FILE_LINE_PREFIX "COM/Windows Shell") ); \
				} while (false)

			HRESULT com_init_unchecked()
			{
				return CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
			}

			struct COM
			{
				COM()
				{
					throw_com_error(com_init_unchecked());
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

	void init_shell_on_startup()
	{
		char const* msg;
		switch(detail::prompt_file::com_init_unchecked())
		{
		case S_OK:
			msg = "COM explicitly initialized";
			break;
		case S_FALSE:
			msg = "Warning: COM already initialized";
			break;
		case RPC_E_CHANGED_MODE:
			msg = "Warning: COM already initialized IN WRONG MODE";
			break;
		default:
			msg = "Warning: COM could not be initialized";
		}
		std::cerr << msg << std::endl;
	}

	std::vector<std::string> prompt_file(char const* current, char const* extensions
		, dialog::t mode, bool multi)
	{
		std::vector<std::string> result;

		using namespace detail::prompt_file;
		prepareCOM();

		com_handle_t<IFileDialog>::t pfd;
		auto dialogCLSID = (mode != dialog::save) ? CLSID_FileOpenDialog : CLSID_FileSaveDialog;
		throw_com_error(CoCreateInstance(dialogCLSID, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(pfd.rebind())));

		com_handle_t<IFileOpenDialog>::t ofd;
		if (dialogCLSID == CLSID_FileOpenDialog)
			throw_com_error(pfd->QueryInterface(IID_PPV_ARGS(ofd.rebind())));

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

			// Unchecked, as not documented to work, but seems to always select first extension as expected
			if (mode == dialog::save && typeCnt > 0)
				pfd->SetDefaultExtension(L"");
		}

		// Initial folder
		com_handle_t<IShellItem>::t currentFolderItem;
		if (current)
		{
			auto path = utfcvt.from_bytes(current);

			SFGAOF folderAtt = 0;
			{
				struct abs_iid_deleter {
					void operator ()(ITEMIDLIST_ABSOLUTE* ptr) const {
						if (ptr)
							ILFree(ptr);
					}
				};
				stdx::unique_handle<ITEMIDLIST_ABSOLUTE, abs_iid_deleter> iidl;
			
				if (auto pathLen = GetFullPathNameW(path.c_str(), 0, nullptr, nullptr))
				{
					std::wstring fullPath;
					fullPath.resize(pathLen);
					if (GetFullPathNameW(path.c_str(), pathLen, &fullPath[0], nullptr))
						path = std::move(fullPath);
				}

				throw_com_error(SHParseDisplayName(path.c_str(), nullptr, iidl.rebind(), SFGAO_FOLDER, &folderAtt));
				throw_com_error(SHCreateItemFromIDList(iidl, IID_PPV_ARGS(currentFolderItem.rebind())));
			}

			if (~folderAtt & SFGAO_FOLDER || mode == dialog::folder)
			{
				com_handle_t<IShellItem>::t folder;
				throw_com_error(currentFolderItem->GetParent(folder.rebind()));
				currentFolderItem = std::move(folder);

				throw_com_error(pfd->SetFileName(path.c_str()));
			}

			throw_com_error(pfd->SetFolder(currentFolderItem));
		}

		// Show the dialog
		if (SUCCEEDED(pfd->Show(NULL)))
		{
			auto&& getPath = [&utfcvt](IShellItem& item) -> std::string
			{
				struct co_str_deleter {
					void operator ()(PWSTR ptr) const {
						if (ptr)
							CoTaskMemFree(ptr);
					}
				};
				stdx::unique_handle<WCHAR, co_str_deleter> pszFilePath;
				throw_com_error(item.GetDisplayName(SIGDN_FILESYSPATH, pszFilePath.rebind()));
				return utfcvt.to_bytes(pszFilePath);
			};

			if (multi)
			{
				assert (ofd.get() != nullptr);

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
		
	std::vector<std::string> prompt_file_compat(char const* current, char const* extensions
		, dialog::t mode, bool multi)
	{
		std::vector<std::string> result;

		using namespace detail::prompt_file;
		prepareCOM();

		OPENFILENAMEW ofn = { 0 };

		ofn.lStructSize = sizeof(OPENFILENAMEW);
		ofn.hwndOwner = 0;
		
		std::wstring fileData;
		fileData.resize(1024 * 1024, 0);
		ofn.lpstrFile = &fileData[0];
		ofn.nMaxFile = DWORD(fileData.size());
		
		ofn.Flags = OFN_NOCHANGEDIR | OFN_HIDEREADONLY | OFN_EXPLORER | OFN_PATHMUSTEXIST;
		if (mode == dialog::open)
			ofn.Flags |= OFN_FILEMUSTEXIST;
		if (mode == dialog::save)
			ofn.Flags |= OFN_OVERWRITEPROMPT;
		if (multi)
			ofn.Flags |= OFN_ALLOWMULTISELECT;
		
		std::wstring_convert< std::codecvt_utf8_utf16<wchar_t> > utfcvt;

		// Extensions
		std::vector<wchar_t> extensionData;
		if (extensions)
		{
			auto typeStr = utfcvt.from_bytes(extensions);
			auto typeCStr = typeStr.c_str();
			size_t typeCnt = 1 + std::count(typeStr.begin(), typeStr.end(), L'|');
			
			extensionData.resize(2 * (typeStr.size() + 1) + 1);
			auto extensionDataCursor = extensionData.data();

			for (size_t i = 0, off = 0; i < typeCnt; ++i)
			{
				auto nextOff = typeStr.find('|', off);
				if (nextOff != typeStr.npos)
					typeStr[nextOff++] = 0;
				else
					nextOff = typeStr.size() + 1;

				auto ass = typeStr.find('=', off);
				if (ass < nextOff)
					typeStr[ass++] = 0;

				wcscpy(extensionDataCursor, typeCStr + off);
				extensionDataCursor += min_value(nextOff, ass) - off;
				
				if (ass < nextOff)
				{
					wcscpy(extensionDataCursor, typeCStr + ass);
					extensionDataCursor += nextOff - ass;
				}
				else
				{
					wcscpy(extensionDataCursor, typeCStr + off);
					extensionDataCursor += nextOff - off;
				}

				off = nextOff;
			}
			// close w/ double 0
			*extensionDataCursor = 0;

			ofn.lpstrFilter = extensionData.data();
		}

		// Initial folder
		std::wstring initialFolderData;
		if (current)
		{
			auto path = utfcvt.from_bytes(current);
			bool includesFile = false;

			if (auto pathLen = GetFullPathNameW(path.c_str(), 0, nullptr, nullptr))
			{
				std::wstring fullPath;
				LPWSTR filePart = nullptr;
				fullPath.resize(pathLen);
				if (GetFullPathNameW(path.c_str(), pathLen, &fullPath[0], &filePart))
				{
					path = std::move(fullPath);
					includesFile = (filePart != nullptr);
				}
			}

			if (includesFile)
				wcscpy(ofn.lpstrFile, path.c_str());
			else
			{
				// warning: moves path
				initialFolderData = std::move(path);
				ofn.lpstrInitialDir = initialFolderData.data();
			}
		}

		std::wstring currDir(GetCurrentDirectoryW(0, nullptr) + 1, 0);
		auto savedCurrDir = GetCurrentDirectoryW(DWORD(currDir.size()), &currDir[0]);

		BOOL accepted = (mode == dialog::save) ? GetSaveFileNameW(&ofn) : GetOpenFileNameW(&ofn);
		if (accepted && ofn.lpstrFile[0])
		{
			if (multi)
			{
				auto nextCursor = ofn.lpstrFile + wcslen(ofn.lpstrFile) + 1;

				// Multiple selected
				if (*nextCursor)
				{
					std::wstring pathStr = ofn.lpstrFile;
					if (pathStr.back() != '\\' && pathStr.back() != '/')
						pathStr.push_back('\\');

					do
					{
						result.push_back( utfcvt.to_bytes(pathStr + nextCursor) );
						nextCursor += wcslen(nextCursor) + 1;
					}
					while (*nextCursor);
				}
				// Only one file selected
				else
					result.push_back( utfcvt.to_bytes(ofn.lpstrFile) );
			}
			else
			{
				result.push_back( utfcvt.to_bytes(ofn.lpstrFile) );
			}
		}

		if (savedCurrDir)
			SetCurrentDirectoryW(currDir.c_str());

		return result;
	}

	int prompt(char const* message, char const* title, choice::t choice)
	{
		if (!title) title = "Prompt";
		UINT style = MB_TASKMODAL;
		
		if (choice == stdx::choice::yesno)
			style |= MB_ICONQUESTION | MB_YESNO;
		else if (choice == stdx::choice::yesnocancel)
			style |= MB_ICONQUESTION | MB_YESNOCANCEL;
		else // if (choice == stdx::choice::ok)
			style |= MB_ICONINFORMATION | MB_OK;
		
		auto result = ::MessageBoxA(NULL, message, title, style);
		
		if (result == IDYES)
			return 1;
		else if (result == IDNO)
			return 0;
		else
			return -1;
	}

#else

	void init_shell_on_startup()
	{
	}
	
	std::vector<std::string> prompt_file(char const* current, char const* extensions
		, dialog::t mode, bool multi)
	{
		std::vector<std::string> result;

		if (!current) current = "$PWD";

		std::stringstream dialog;
		dialog << "dialog --stdout --fselect \"" << current << "\" 0 0";

		struct process_close {
			void operator ()(FILE* ptr) const {
				if (ptr)
					pclose(ptr);
			}
		};
		stdx::unique_handle<FILE, process_close> pipe( popen(dialog.str().c_str(), "r") );
		if (!pipe) throwx(std::runtime_error("Error opening file dialog"));

		std::string resultPath;
		do
		{
			char buffer[2048];
			while (fgets(buffer, arraylen(buffer), pipe) != nullptr)
				resultPath += buffer;

			if (!resultPath.empty())
				result.push_back(resultPath);
			else
				break;
		}
		while (multi);

		return result;
	}

	std::vector<std::string> prompt_file_compat(char const* current, char const* extensions
		, dialog::t mode, bool multi)
	{
		return prompt_file(current, extensions, mode, multi);
	}

	int prompt(char const* message, char const* title, choice::t choice)
	{
		std::stringstream xmessage;
		xmessage << "xmessage -buttons ";

		enum ButtonValues { OK = 0x10, Yes, No, Cancel };
		if (choice == stdx::choice::yesno || choice == stdx::choice::yesnocancel)
		{
			xmessage << "Yes:" << Yes << ",No:" << No;
			if (choice == stdx::choice::yesnocancel)
				xmessage << ",Cancel:" << Cancel;
		} else // if (choice == stdx::choice::ok)
			xmessage << "OK:" << OK;

		xmessage << " \"" << message << '"';
		auto result = ::system(xmessage.str().c_str());
		
		if (result == Yes)
			return 1;
		else if (result == No)
			return 0;
		else
			return -1;
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
