#include "uix"
#include "textx"

#include "file"
#include "appx"

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

std::unique_ptr<UserInterface> UserInterface::create(text::FreeTypeRef* freeTypeLib, text::FaceRef* font, ogl::ProgramRef* textProgram
		, ogl::ProgramRef* widgetProgram)
{
	struct DefaultUserInterfaceDeps
	{
		UiRenderer widgetRenderer;
		text::TextRenderer textRenderer;
		UiTextRenderer uiTextRenderer;
		
		DefaultUserInterfaceDeps(text::FreeTypeRef* freeTypeLib, text::FaceRef* font, ogl::ProgramRef* textProgram
				, ogl::ProgramRef* widgetProgram)
			: widgetRenderer(widgetProgram, 1024)
			, textRenderer(*freeTypeLib, textProgram, 10000) // todo: initialize with adaptive tile size?
			, uiTextRenderer(*freeTypeLib, *font, textRenderer)
		{
		}
	};
	struct DefaultUserInterface : DefaultUserInterfaceDeps, ui::TextUi
	{
		DefaultUserInterface(text::FreeTypeRef* freeTypeLib, text::FaceRef* font, ogl::ProgramRef* textProgram
				, ogl::ProgramRef* widgetProgram)
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

void preset_user_interface(UniversalInterface& ui, stdx::fun_ref<void(UniversalInterface&)> target, char const* defaultFile, std::string* activeFile, char const* groupName)
{
	std::string activeFileName;
	if (activeFile)
	{
		activeFileName = stdx::basename(activeFile->c_str());
		if (!defaultFile) defaultFile = activeFileName.c_str();
	}

	if (!groupName) groupName = "Preset";

	if (auto presetGroup = ui::Group(ui, UEI(preset_user_interface)))
	{
		uintptr_t controlIdentifier = 0;

		ui.addText(controlIdentifier++, groupName, "", nullptr);

		auto&& loadPreset = [&](char const* file)
		{
			try { load_ini_file(file, target); *activeFile = file; }
			catch(...) { stdx::prompt(appx::exception_string().c_str(), "Error applying preset"); }
		};
		ui.addButton(controlIdentifier++, (!activeFileName.empty()) ? activeFileName.c_str() : "load", [&]()
		{
			auto mf = stdx::prompt_file_compat(defaultFile, "Ini files=*.ini|All files=*.*", stdx::dialog::open, true);
			for (auto&&f : mf) loadPreset(f.c_str());
		});
		if (activeFile)
		{
			auto hiddenIdentifier = controlIdentifier++;
			if (!activeFile->empty()) ui.addHidden(hiddenIdentifier, "active", activeFile->c_str(), loadPreset);
		}

		ui.addButton(controlIdentifier++, "save", [&]()
		{
			auto mf = stdx::prompt_file_compat(defaultFile, "Ini files=*.ini|All files=*.*", stdx::dialog::save);
			for (auto&&f : mf)
			{
				try { save_ini_file(f.c_str(), target); }
				catch(...) { stdx::prompt(appx::exception_string().c_str(), "Error saving preset"); }
			}
		});
	}
}

} // namespace
