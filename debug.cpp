#pragma once

#include <iostream>

#ifdef WIN32
	#include <Windows.h>
	#include <DbgHelp.h>
#endif

namespace stdx
{

#ifdef WIN32
	
	bool dump_exception(void* excpt, char const* dumpPath)
	{
		auto hDbgHelp = LoadLibraryA("dbghelp");
		if(!hDbgHelp)
		{
			std::cout << "Cannot load dbghelp to write crash dump" << std::endl;
			return false;
		}
    
		auto pMiniDumpWriteDump = decltype(&MiniDumpWriteDump)(GetProcAddress(hDbgHelp, "MiniDumpWriteDump"));
		if(!pMiniDumpWriteDump)
		{
			std::cout << "Cannot retrieve MiniDumpWriteDump function from dbghelp" << std::endl;
			return false;
		}

		MINIDUMP_EXCEPTION_INFORMATION eifo = { 0 };
		eifo.ThreadId = GetCurrentThreadId();
		eifo.ExceptionPointers = (EXCEPTION_POINTERS*) excpt;
		eifo.ClientPointers = TRUE;

		char dumpName[MAX_PATH * 2];
		if (!dumpPath)
		{
			auto nameEnd = dumpName + GetModuleFileNameA(GetModuleHandleA(0), dumpName, MAX_PATH);
			SYSTEMTIME t;
			GetSystemTime(&t);
			wsprintfA(nameEnd - strlen(".exe"),
				"_%4d%02d%02d_%02d%02d%02d.dmp",
				t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
			dumpPath = dumpName;
		}

		auto hFile = CreateFileA(dumpPath, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if(hFile == INVALID_HANDLE_VALUE)
		{
			std::cout << "Cannot write dump to file " << dumpPath << std::endl;
			return false;
		}

		auto dumped = pMiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile
			, MINIDUMP_TYPE(MiniDumpWithIndirectlyReferencedMemory | MiniDumpScanMemory)
			, &eifo, nullptr, nullptr);
		CloseHandle(hFile);

		if (!dumped)
			std::cout << "Writing memory dump failed (" << dumpPath << ")" << std::endl;

		return (dumped != FALSE);
	}

#endif
	
} // namespace
