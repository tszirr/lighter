#include "uii"

#include <cstdio>

namespace ui
{

struct UiToText : UniversalInterface
{
	typedef std::pair<std::string, std::string> tuple;
	
	KeyValueStream* stream;

	std::vector<std::string> groupPath;
	size_t groupCount;
	size_t groupLabelPending;

	UiToText(KeyValueStream& stream)
		: stream(&stream)
		, groupCount(0)
		, groupLabelPending(0) { }
	
	bool pushContext(UniqueElementIdentifier id) override { return true; }
	void popContext(UniqueElementIdentifier id) override { }

	bool pushGroup(UniqueElementIdentifier id) override
	{
		++groupLabelPending;
		return true;
	}
	void popGroup(UniqueElementIdentifier id) override
	{
		if (groupLabelPending > 0)
			--groupLabelPending;
		else
		{
			stream->leaveSection(groupPath.back().c_str());
			--groupCount;
		}
	}
	
	void addItemOrGroup(char const* key, char const* value)
	{
		if (groupLabelPending > 0)
		{
			if (groupCount >= groupPath.size())
				groupPath.resize(1 + groupCount * 3 / 2);
			groupPath[groupCount] = key;
			++groupCount;
			--groupLabelPending;
			stream->enterSection(key, value);
		}
		else if (value)
			stream->addItem(key, value);
	}

	void addText(UniqueElementIdentifier id, char const* label, char const* text, InteractionParam<char const*> interact) override
	{
		addItemOrGroup(label
			  // Always store something for interactive text
			, (interact) ? (text ? text : "")
			  // Otherwise store nothing, but maybe add group
			  : nullptr);
	}

	void addHidden(UniqueElementIdentifier id, char const* label, char const* text, InteractionParam<char const*> interact, InteractionParam<std::string&> onDemand = nullptr) override
	{
		std::string onDemandStorage;
		if (onDemand) { onDemandStorage = text; onDemand->updateValue(onDemandStorage); text = onDemandStorage.c_str(); }
		stream->addItem(label, text);
	}

	void addInteractiveButton(UniqueElementIdentifier id, char const* text, bool value, InteractionParam<ButtonEvent::T> interact, InteractionParam<Button> control = nullptr) override
	{
	}

	void addButton(UniqueElementIdentifier id, char const* text, InteractionParam<void> interact, char const* fullText = nullptr, InteractionParam<Button> control = nullptr) override
	{
//		auto key = keyFromLabel(label);
//		(fullText) ? fullText : text;
	}

	void addOption(UniqueElementIdentifier id, char const* text, bool value, InteractionParam<bool> interact, InteractionParam<Button> control = nullptr) override
	{
		addItemOrGroup(text, (interact) ? (value ? "true" : "false") : nullptr);
	}

	void addSlider(UniqueElementIdentifier id, char const* label, float value, float range, InteractionParam<float> interact, float ticks = 0.0f, float fullBarThreshold = 0.0f
		, InteractionParam<Slider> control = nullptr) override
	{
		char const* storedVal = nullptr;
		char buf[1024];
		if (interact)
		{
			sprintf(buf, "%f", value);
			storedVal = buf;
		}
		addItemOrGroup(label, storedVal);
	}

	void addColor(UniqueElementIdentifier id, char const* label, glm::vec3 const& value, float valueRange, InteractionParam<glm::vec3> interact, float valueTicks = 0.0f) override
	{
		char const* storedVal = nullptr;
		char buf[1024];
		if (interact)
		{
			sprintf(buf, "%f %f %f", value.x, value.y, value.z);
			storedVal = buf;
		}
		addItemOrGroup(label, storedVal);
	}
};

void write(KeyValueStream& out, stdx::fun_ref<void(UniversalInterface&)> ui)
{
	UiToText uitt(out);
	ui.dispatch(ui, uitt);
}

struct TextToUi : UniversalInterface
{
	typedef std::pair<std::string, std::string> tuple;
	
	KeyValueStore* stream;
	std::string nullterminatedStore;

	size_t groupLabelPending;

	TextToUi(KeyValueStore& stream)
		: stream(&stream)
		, groupLabelPending(0) { }

	bool pushContext(UniqueElementIdentifier id) override { return true; }
	void popContext(UniqueElementIdentifier id) override { }

	bool pushGroup(UniqueElementIdentifier id) override
	{
		++groupLabelPending;
		return true;
	}
	void popGroup(UniqueElementIdentifier id) override
	{
		if (groupLabelPending > 0)
			--groupLabelPending;
		else
			stream->leaveSection();
	}
	
	struct ItemSource : UniversalInterface::ItemSource
	{
		struct Data
		{
			char itemSourceID[4];
			KeyValueStore::Item const* item;

			#define ItemSource_ID(_0, _1, _2, _3) _0 1 _1 5 _2 6 _3 4
			#define ItemSource_ID_Expr(x, e, d) ItemSource_ID(x[0] e, d x[1] e, d x[2] e, d x[3] e)

			Data()
			{
				ItemSource_ID_Expr(itemSourceID, =, ;);
			}
			static KeyValueStore::Item const* getItem(char const* label)
			{
				return (ItemSource_ID_Expr(label, ==, &&)) 
					? reinterpret_cast<Data const*>(label)->item
					: nullptr;
			}

			#undef ItemSource_ID_Expr
			#undef ItemSource_ID
		} data;

		KeyValueStore* stream;
		KeyValueStore::Item const* nextItem;

		size_t estimateCount() const override
		{
			return stream->estimateCount(data.item);
		}
		char const* next() override
		{
			data.item = nextItem;
			nextItem = stream->nextItem(nextItem);
			return (data.item) ? data.itemSourceID : nullptr;
		}
	};

	void addItems(stdx::fun_ref<void(UniversalInterface&, UniversalInterface::ItemSource&)> addItem, char const* filterLabel = nullptr) override
	{
		ItemSource is;
		is.stream = stream;
		is.nextItem = stream->findItem(filterLabel);
		addItem.dispatch(addItem, *this, is);
	}
	
	KeyValueStore::Item const* getItem(char const* label)
	{
		auto item = ItemSource::Data::getItem(label);
		if (!item)
			item = stream->findItem(label);
		return item;
	}

	stdx::range<char const*> getValueAndMaybeEnter(char const* label)
	{
		auto item = getItem(label);
		if (groupLabelPending > 0)
		{
			stream->enterSection(item);
			--groupLabelPending;
		}
		return stream->getValue(item);
	}

	void addText(UniqueElementIdentifier id, char const* label, char const* text, InteractionParam<char const*> interact) override
	{
		auto val = getValueAndMaybeEnter(label);
		if (interact)
			if (val.first) // check if value explicitly given
			{
				nullterminatedStore.assign(val.first, val.last);
				interact->updateValue(nullterminatedStore.c_str());
			}
	}

	void addHidden(UniqueElementIdentifier id, char const* label, char const* text, InteractionParam<char const*> interact, InteractionParam<std::string&> onDemand = nullptr) override
	{
		if (interact)
		{
			auto val = stream->getValue(getItem(label));
			if (val.first) // check if value explicitly given
			{
				nullterminatedStore.assign(val.first, val.last);
				interact->updateValue(nullterminatedStore.c_str());
			}
		}
	}

	void addInteractiveButton(UniqueElementIdentifier id, char const* text, bool value, InteractionParam<ButtonEvent::T> interact, InteractionParam<Button> control = nullptr) override
	{
	}

	void addButton(UniqueElementIdentifier id, char const* text, InteractionParam<void> interact, char const* fullText = nullptr, InteractionParam<Button> control = nullptr) override
	{
//		auto key = keyFromLabel(label);
//		(fullText) ? fullText : text;
	}

	void addOption(UniqueElementIdentifier id, char const* text, bool value, InteractionParam<bool> interact, InteractionParam<Button> control = nullptr) override
	{
		auto val = getValueAndMaybeEnter(text);
		if (interact)
			if (val.first)
				interact->updateValue( !stdx::memeq(val, stdx::strlit_range("false")) && !(val[0] == '0' && val[1] == 0) );
	}

	void addSlider(UniqueElementIdentifier id, char const* label, float value, float range, InteractionParam<float> interact, float ticks = 0.0f, float fullBarThreshold = 0.0f
		, InteractionParam<Slider> control = nullptr) override
	{
		auto val = getValueAndMaybeEnter(label);
		if (interact)
			if (val.first)
			{
				float newVal = value;
				if (sscanf(val.first, "%f", &newVal) == 1)
					interact->updateValue(newVal);
			}
	}

	void addColor(UniqueElementIdentifier id, char const* label, glm::vec3 const& value, float valueRange, InteractionParam<glm::vec3> interact, float valueTicks = 0.0f) override
	{
		auto val = getValueAndMaybeEnter(label);
		if (interact)
			if (val.first)
			{
				glm::vec3 newVal = value;
				if (sscanf(val.first, "%f %f %f", &newVal.x, &newVal.y, &newVal.z) == 3)
					interact->updateValue(newVal);
			}
	}
};

void read(KeyValueStore& in, stdx::fun_ref<void(UniversalInterface&)> ui)
{
	TextToUi ttui(in);
	ui.dispatch(ui, ttui);
}

} // namespace

#include "uiix"
#include "filex"

namespace ui
{

void load_ini_file(char const* file, stdx::fun_ref<void(UniversalInterface&)> ui)
{
	auto presetFile = stdx::load_file(file);
	auto presets = stdx::parse_ini_file<char>(stdx::data_range(presetFile));
	presets[0].sort();
	auto valStore = make_SortedKeyValueStore(presets.get());
	read(valStore, ui);
}

void save_ini_file(char const* file, stdx::fun_ref<void(UniversalInterface&)> ui)
{
	auto iniFile = stdx::write_file(file);
	IniStream iniStream(iniFile);
	write(iniStream, ui);
}

} // namespace
