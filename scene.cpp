#include "scenex"

#include "file"
#include <algorithm>

namespace scene
{

unsigned DataHeader::make_id(char const* id)
{
	union
	{
		unsigned n;
		char c[4];
	} did;
	
	assert (id != nullptr);
	
	did.n = 0;
	for (int i = 0; auto c = id[i]; ++i)
	{
		did.c[i] = c;
		assert (i < arraylen(did.c));
	}

	return did.n;
}

DataHeader DataHeader::make(char const* id, size_t count, unsigned version, size_t elementSize)
{
	DataHeader header;

	header.id = make_id(id);
	header.version = version;

	assert (elementSize > 0);
	assert (elementSize * count <= ~0U);
	header.elementSize = unsigned(elementSize);
	header.size = unsigned(elementSize * count);

	return header;
}

std::string scenecvt::cmd() const
{
	std::string result;
	auto toolExe = this->toolExe;
	if (!toolExe)
		toolExe = "scenecvt";
	result.reserve(strlen(toolExe) + 1024);
	result.append(toolExe);
	result.append(" scene");
	if (!normals) result.append(" /VDn");
	if (vertexColors) result.append(" /Vc");
	if (!texcoords) result.append(" /VDt");
	if (forceTexcoords) result.append(" /VFt");
	if (regenerateAllNormals) result.append(" /Vsn");
	if (maxSmoothingAngle >= 0.0f) { result.append(" /Vsna "); result.append(std::to_string(maxSmoothingAngle) ); }
	if (optimize) result.append(" /Mo");
	if (mergeAllGeometry) result.append(" /Sg");
	if (mergeEqualMaterials) result.append(" /Sm");
	if (pretransform) result.append(" /Sp");
	if (scaleFactor > 0.0f) { result.append(" /Ssf "); result.append(std::to_string(scaleFactor) ); }
	return result;
}

int scenecvt::run(stdx::data_range_param<char const *const> inputs, char const* output) const
{
	auto cmd = this->cmd();
	if (inputs.size() > 1)
		cmd.append(" /S+");
	for (auto i : inputs)
	{
		cmd.append(" \"");
		cmd.append(i);
		cmd.append("\"");
	}
	cmd.append(" \"");
	cmd.append(output);
	cmd.append("\"");

	return system(cmd.c_str());
}

std::string scenecvt::locateOrRun(char const* srcFile, bool skipIfUpToDate) const
{
	std::string result = srcFile;

	auto srcExtBegin = strrchr(srcFile, '.');
	char const sceneExt[] = ".scene";
	if (srcExtBegin && !stdx::strieq(srcExtBegin, sceneExt))
	{
		std::string outFile;
		outFile.reserve((srcExtBegin - srcFile) + arraylen(sceneExt));
		outFile.append(srcFile, srcExtBegin);
		outFile.append(sceneExt);

		// Try to locate (exists > 0 + newer)
		bool located = skipIfUpToDate && stdx::file_time(outFile.c_str()) > stdx::file_time(srcFile);

		// Try to convert
		if (!located)
			if (run(stdx::make_range_n(&srcFile, 1), outFile.c_str()) != 0)
				throwx( stdx::file_error("unable to convert file") );

		result = outFile;
	}

	return result;
}

} // namespace