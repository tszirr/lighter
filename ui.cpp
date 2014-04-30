#include "uix"
#include "textx"

namespace ui
{

glm::aabb<glm::ivec2> UiTextRenderer::selectChar(glm::ivec2 cursorPos, size_t& charIdx, glm::ivec2 insertPos, char const* text, size_t maxChars, CharType type)
{
	switch (type)
	{
	case UTF8: default: return renderer.selectChar(lib, font, cursorPos, charIdx, insertPos, text, maxChars);
	case UTF16: return renderer.selectChar(lib, font, cursorPos, charIdx, insertPos, reinterpret_cast<FT_Int16 const*>(text), maxChars);
	case UTF32: return renderer.selectChar(lib, font, cursorPos, charIdx, insertPos, reinterpret_cast<FT_Int32 const*>(text), maxChars);
	}
}

glm::aabb<glm::ivec2> UiTextRenderer::boundText(char const* text, size_t maxChars, glm::ivec2 insertPos, CharType type)
{
	switch (type)
	{
	case UTF8: default: return renderer.boundText(lib, font, insertPos, text, maxChars);
	case UTF16: return renderer.boundText(lib, font, insertPos, reinterpret_cast<FT_Int16 const*>(text), maxChars);
	case UTF32: return renderer.boundText(lib, font, insertPos, reinterpret_cast<FT_Int32 const*>(text), maxChars);
	}
}

glm::aabb<glm::ivec2> UiTextRenderer::drawText(glm::ivec2 insertPos, char const* text, size_t maxChars, CharType type)
{
	switch (type)
	{
	case UTF8: default: return renderer.drawText(lib, font, insertPos, text, maxChars);
	case UTF16: return renderer.drawText(lib, font, insertPos, reinterpret_cast<FT_Int16 const*>(text), maxChars);
	case UTF32: return renderer.drawText(lib, font, insertPos, reinterpret_cast<FT_Int32 const*>(text), maxChars);
	}
}

std::unique_ptr<UserInterface> UserInterface::create(text::FreeType* freeTypeLib, text::Face* font, ogl::Program* textProgram
		, ogl::Program* widgetProgram)
{
	struct DefaultUserInterfaceDeps
	{
		UiRenderer widgetRenderer;
		text::TextRenderer textRenderer;
		UiTextRenderer uiTextRenderer;
		
		DefaultUserInterfaceDeps(text::FreeType* freeTypeLib, text::Face* font, ogl::Program* textProgram
				, ogl::Program* widgetProgram)
			: widgetRenderer(widgetProgram, 1024)
			, textRenderer(*freeTypeLib, textProgram, 10000) // todo: initialize with adaptive tile size?
			, uiTextRenderer(*freeTypeLib, *font, textRenderer)
		{
		}
	};
	struct DefaultUserInterface : DefaultUserInterfaceDeps, ui::TextUi
	{
		DefaultUserInterface(text::FreeType* freeTypeLib, text::Face* font, ogl::Program* textProgram
				, ogl::Program* widgetProgram)
			: DefaultUserInterfaceDeps(freeTypeLib, font, textProgram, widgetProgram)
			, TextUi(&widgetRenderer, &uiTextRenderer)
		{
		}
		// todo: replace by proper render methods
		void flushWidgets() override { widgetRenderer.flushWidgets(); }
		void flushText() override { DefaultUserInterfaceDeps::textRenderer.flushText(); }
	};
	return std::unique_ptr<UserInterface>(new DefaultUserInterface(freeTypeLib, font, textProgram, widgetProgram));
}

} // namespace

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

	void pushGroup(UniqueElementIdentifier id) override
	{
		++groupLabelPending;
	}
	void popGroup(UniqueElementIdentifier id) override
	{
		if (groupLabelPending > 0)
			--groupLabelPending;
		else
		{
			groupPrefixLen.pop_back();
			assert (!groupPrefixLen.empty()); // Always keep 0 sentinel
			groupPrefix.resize(groupPrefixLen.back());
		}
	}
	
	char const* keyFromLabel(char const* label)
	{
		if (groupLabelPending > 0)
		{
			groupPrefix.push_back('.');
			groupPrefix.append(label);
			groupPrefixLen.push_back(groupPrefix.size());
			--groupLabelPending;
			stream->changeSection(groupPrefix.c_str());
			return "$";
		}
		else
			return label;
	}
	void addLabel(char const* label) override
	{
		// Labels are not stored, but may name groups
		keyFromLabel(label);
	}

	void beginUnion() override
	{
	}

	void endUnion() override
	{
	}

	void addText(UniqueElementIdentifier id, char const* label, char const* text, InteractionParam<char const*> interact) override
	{
		stream->addItem(keyFromLabel(label), text);
	}

	void addHidden(UniqueElementIdentifier id, char const* label, char const* text, InteractionParam<char const*> interact) override
	{
		stream->addItem(keyFromLabel(label), text);
	}

	void addButton(UniqueElementIdentifier id, char const* text, bool value, InteractionParam<ButtonEvent::T> interact, InteractionParam<Button> control = nullptr) override
	{
	}

	void addButton(UniqueElementIdentifier id, char const* text, InteractionParam<void> interact, char const* fullText = nullptr, InteractionParam<Button> control = nullptr) override
	{
//		auto key = keyFromLabel(label);
//		(fullText) ? fullText : text;
	}

	void addOption(UniqueElementIdentifier id, char const* text, bool value, InteractionParam<bool> interact, InteractionParam<Button> control = nullptr) override
	{
		stream->addItem(keyFromLabel(text), (value) ? "true" : "false");
	}

	void addSlider(UniqueElementIdentifier id, char const* label, float value, float range, InteractionParam<float> interact, float ticks = 0.0f, InteractionParam<Slider> control = nullptr) override
	{
		char buf[1024];
		sprintf(buf, "%f", value);
		stream->addItem(keyFromLabel(label), buf);
	}
};

void write(KeyValueStream& out, std::function<void(UniversalInterface&)> const& ui)
{
	UiToText uitt(out);
	ui(uitt);
}

} // namespace
