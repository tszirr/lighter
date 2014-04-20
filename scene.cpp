#include "scene"

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

} // namespace