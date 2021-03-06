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

void UiTextRenderer::flushText()
{
	renderer.flushText();
}

namespace
{
	struct DefaultTextRenderer : UiTextRenderer
	{
		text::TextRenderer textRenderer;
		
		DefaultTextRenderer(text::FreeTypeRef* freeTypeLib, text::FaceRef* font, ogl::ProgramRef* textProgram)
			: UiTextRenderer(*freeTypeLib, *font, textRenderer)
			, textRenderer(*freeTypeLib, textProgram, 10000) // todo: initialize with adaptive tile size?
		{ }
	};
}

std::unique_ptr<UiTextRenderer> UiTextRenderer::create(text::FreeTypeRef* freeTypeLib, text::FaceRef* font, ogl::ProgramRef* textProgram)
{
	return std::unique_ptr<UiTextRenderer>(new DefaultTextRenderer(freeTypeLib, font, textProgram));
}

std::unique_ptr<UserInterface> UserInterface::create(text::FreeTypeRef* freeTypeLib, text::FaceRef* font, ogl::ProgramRef* textProgram
		, ogl::ProgramRef* widgetProgram)
{
	struct DefaultUserInterfaceDeps
	{
		UiRenderer widgetRenderer;
		DefaultTextRenderer uiTextRenderer;
		
		DefaultUserInterfaceDeps(text::FreeTypeRef* freeTypeLib, text::FaceRef* font, ogl::ProgramRef* textProgram
				, ogl::ProgramRef* widgetProgram)
			: widgetRenderer(widgetProgram, 1024)
			, uiTextRenderer(freeTypeLib, font, textProgram)
		{ }
	};
	struct DefaultUserInterface : DefaultUserInterfaceDeps, ui::TextUi
	{
		DefaultUserInterface(text::FreeTypeRef* freeTypeLib, text::FaceRef* font, ogl::ProgramRef* textProgram
				, ogl::ProgramRef* widgetProgram)
			: DefaultUserInterfaceDeps(freeTypeLib, font, textProgram, widgetProgram)
			, TextUi(&this->DefaultUserInterfaceDeps::widgetRenderer, &this->DefaultUserInterfaceDeps::uiTextRenderer)
		{ }
		// todo: replace by proper render methods
		void flushWidgets() override { DefaultUserInterfaceDeps::widgetRenderer.flushWidgets(); }
		void flushText() override { DefaultUserInterfaceDeps::uiTextRenderer.textRenderer.flushText(); }
	};
	return std::unique_ptr<UserInterface>(new DefaultUserInterface(freeTypeLib, font, textProgram, widgetProgram));
}

void preset_user_interface(UniversalInterface& ui, stdx::fun_ref<void(UniversalInterface&)> target
                         , char const* defaultFile, char const* activeFilePath, ui::UniversalInterface::InteractionParam<char const*> activeFileChange
                         , char const* groupName, UniqueElementIdentifier id)
{
	if (!id.value) id = UEI(preset_user_interface);

	if (auto presetGroup = ui::Group(ui, id))
	{
		uintptr_t controlIdentifier = 0;

		ui.addText(controlIdentifier++, (groupName) ? groupName : "Preset", "", nullptr);

		auto&& loadPreset = [&](char const* file)
		{
			try
			{
				activeFilePath = defaultFile = nullptr; // warning: everything may change
				if (activeFileChange) activeFileChange->updateValue(file);
				load_ini_file(file, target);
			}
			catch(...) { stdx::prompt(appx::exception_string().c_str(), "Error applying preset"); }
		};
		if (activeFilePath)
		{
			auto hiddenIdentifier = controlIdentifier++;
			if (activeFilePath[0]) ui.addHidden(hiddenIdentifier, "active", activeFilePath, loadPreset);
		}
		ui.addButton(controlIdentifier++, (activeFilePath && activeFilePath[0] && defaultFile && defaultFile[0]) ? defaultFile : "load", [&]()
		{
			auto mf = stdx::prompt_file_compat(defaultFile, "Ini files=*.ini|All files=*.*", stdx::dialog::open, true);
			for (auto&&f : mf) loadPreset(f.c_str());
		});
		

		ui.addButton(controlIdentifier++, "save", [&]()
		{
			auto mf = stdx::prompt_file_compat(defaultFile, "Ini files=*.ini|All files=*.*", stdx::dialog::save);
			for (auto&&f : mf)
			{
				try
				{
					activeFilePath = defaultFile = nullptr; // warning: everything may change
					if (activeFileChange) activeFileChange->updateValue(f.c_str());
					save_ini_file(f.c_str(), target);
				}
				catch(...) { stdx::prompt(appx::exception_string().c_str(), "Error saving preset"); }
			}
		});
	}
}

} // namespace
