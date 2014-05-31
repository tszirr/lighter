#include "pool"

#include <algorithm>

namespace appx
{

namespace detail
{

namespace resource_pool
{

pool_item* find(std::unique_ptr<pool_item> const* begin, std::unique_ptr<pool_item> const* end, void const* desc
	, bool (*lessThanDesc)(std::unique_ptr<pool_item> const&, void const*)
	, bool (*greaterThanDesc)(void const*, std::unique_ptr<pool_item> const&))
{
	auto first = std::lower_bound(begin, end, desc, lessThanDesc);
	auto last = std::upper_bound(begin, end, desc, greaterThanDesc);
	
	auto it = first;
	for (; it != last; ++it)
		if ((*it)->ref_count == 0)
			break;

	return (it != last)
		? it->get()
		: nullptr;
}

pool_item* insert(std::vector< std::unique_ptr<pool_item> >& items
	, pool_item* newItemPtr, void const* desc
	, bool (*lessThanDesc)(std::unique_ptr<pool_item> const&, void const*))
{
	std::unique_ptr<pool_item> newItem(newItemPtr);
	auto where = std::lower_bound(items.begin(), items.end(), desc, lessThanDesc);
	items.insert(where, MOVE(newItem));
	return newItemPtr;
}

void erase_unused(std::vector< std::unique_ptr<pool_item> >& items, unsigned short usage_frame, unsigned short min_unused_frames)
{
	items.erase(
		  std::remove_if(items.begin(), items.end(), [=](std::unique_ptr<pool_item> const& i)
			{ return i->ref_count == 0 && usage_frame - i->last_use >= min_unused_frames; }
		  )
		, items.end()
		);
}

} // namespace

} // namespace

} // namespace
