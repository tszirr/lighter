#pragma once

#include "filex"
#include <algorithm>

#include <sys/types.h>
#include <sys/stat.h>

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
