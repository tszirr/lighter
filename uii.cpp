#include "uii"

#include <cstdio>

namespace ui
{

struct UiToText : UniversalInterface
{
	typedef std::pair<std::string, std::string> tuple;
	
	KeyValueStream* stream;

	std::string groupPrefix;
	std::vector<size_t> groupPrefixLen;
	size_t groupLabelPending;

	UiToText(KeyValueStream& stream)
		: stream(&stream)
		, groupPrefixLen(1, 0)
		, groupLabelPending(0) { }

	bool pushGroup(UniqueElementIdentifier id, bool openByDefault = true) override
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
			stream->leaveSection();
			groupPrefixLen.pop_back();
			assert (!groupPrefixLen.empty()); // Always keep 0 sentinel
			groupPrefix.resize(groupPrefixLen.back());
		}
	}
	
	char const* keyFromLabel(char const* label)
	{
		if (groupLabelPending > 0)
		{
			if (!groupPrefix.empty())
				groupPrefix.push_back('.');
			groupPrefix.append(label);
			groupPrefixLen.push_back(groupPrefix.size());
			--groupLabelPending;
			stream->enterSection(label, groupPrefix.c_str());
			return "$";
		}
		else
			return label;
	}

	void beginUnion() override
	{
	}

	void endUnion() override
	{
	}

	void addText(UniqueElementIdentifier id, char const* label, char const* text, InteractionParam<char const*> interact) override
	{
		auto key = keyFromLabel(label);
		if (interact)
			stream->addItem(key, text);
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
		auto key = keyFromLabel(text);
		if (interact)
			stream->addItem(key, (value) ? "true" : "false");
	}

	void addSlider(UniqueElementIdentifier id, char const* label, float value, float range, InteractionParam<float> interact, float ticks = 0.0f, InteractionParam<Slider> control = nullptr) override
	{
		auto key = keyFromLabel(label);
		if (interact)
		{
			char buf[1024];
			sprintf(buf, "%f", value);
			stream->addItem(key, buf);
		}
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

	size_t groupLabelPending;

	TextToUi(KeyValueStore& stream)
		: stream(&stream)
		, groupLabelPending(0) { }

	bool pushGroup(UniqueElementIdentifier id, bool openByDefault = true) override
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
	
	char const* keyFromLabel(char const* label)
	{
		if (groupLabelPending > 0)
		{
			stream->enterSection(label);
			--groupLabelPending;
			return "$";
		}
		else
			return label;
	}

	void beginUnion() override
	{
	}

	void endUnion() override
	{
	}

	void addText(UniqueElementIdentifier id, char const* label, char const* text, InteractionParam<char const*> interact) override
	{
		auto key = keyFromLabel(label);
		if (interact)
			if (auto val = stream->getValue(key))
				interact->updateValue(val);
	}

	void addHidden(UniqueElementIdentifier id, char const* label, char const* text, InteractionParam<char const*> interact, InteractionParam<std::string&> onDemand = nullptr) override
	{
		if (interact)
			if (auto val = stream->getValue(label))
				interact->updateValue(val);
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
		auto key = keyFromLabel(text);
		if (interact)
			if (auto val = stream->getValue(key))
				interact->updateValue( !stdx::strieq(val, "false") && !(val[0] == '0' && val[1] == 0) );
	}

	void addSlider(UniqueElementIdentifier id, char const* label, float value, float range, InteractionParam<float> interact, float ticks = 0.0f, InteractionParam<Slider> control = nullptr) override
	{
		auto key = keyFromLabel(label);
		if (interact)
			if (auto val = stream->getValue(key))
			{
				float newVal = value;
				if (sscanf(val, "%f", &newVal) == 1)
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
	auto presets = stdx::parse_ini_file(stdx::load_file(file).c_str());
	auto valStore = make_SortedKeyValueStore(presets);
	read(valStore, ui);
}

void save_ini_file(char const* file, stdx::fun_ref<void(UniversalInterface&)> ui)
{
	auto iniFile = stdx::write_file(file);
	IniStream iniStream(iniFile);
	write(iniStream, ui);
}

} // namespace
