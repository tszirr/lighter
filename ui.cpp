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
