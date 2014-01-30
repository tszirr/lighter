#pragma once

#include "ogl"

#include <map>

namespace ogl
{
	GLFW*& GLFW::instance()
	{
		static GLFW* i;
		return i;
	}

	void GLFW::errorCallback(int error, const char* description)
	{
		std::cout << "GLFW error #" << error << ": " << description << std::endl;
	}

	void APIENTRY GLFW::debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, GLvoid* userParam)
	{
		bool Error = false;
		bool Info = false;

		char const* srcS = "Unknown Source";
		if(source == GL_DEBUG_SOURCE_API_ARB)
			srcS = "OpenGL";
		else if(source == GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB)
			srcS = "Windows";
		else if(source == GL_DEBUG_SOURCE_SHADER_COMPILER_ARB)
			srcS = "Shader Compiler";
		else if(source == GL_DEBUG_SOURCE_THIRD_PARTY_ARB)
			srcS = "Third Party";
		else if(source == GL_DEBUG_SOURCE_APPLICATION_ARB)
			srcS = "Application";
		else if(source == GL_DEBUG_SOURCE_OTHER_ARB)
			srcS = "Other";

		char const* typeS = "unknown error";
		if(type == GL_DEBUG_TYPE_ERROR_ARB)
			typeS = "error";
		else if(type == GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB)
			typeS = "deprecated behavior";
		else if(type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB)
			typeS = "undefined behavior";
		else if(type == GL_DEBUG_TYPE_PORTABILITY_ARB)
			typeS = "portability";
		else if(type == GL_DEBUG_TYPE_PERFORMANCE_ARB)
			typeS = "performance";
		else if(type == GL_DEBUG_TYPE_OTHER_ARB)
		{
			typeS = "message";
			Info = (strstr(message, "info:") != nullptr);
		}

		char const* severityS= "unknown severity";
		if(severity == GL_DEBUG_SEVERITY_HIGH_ARB)
		{
			severityS = "high";
			Error = true;
		}
		else if(severity == GL_DEBUG_SEVERITY_MEDIUM_ARB)
			severityS = "medium";
		else if(severity == GL_DEBUG_SEVERITY_LOW_ARB)
			severityS = "low";

		if (Error || !Info)
			std::cout << srcS << ": " << typeS << "(" << severityS << ") " << id << ": " << message << std::endl;
	}

	namespace
	{
		std::map<GLFWwindow*, GLFWWindow*>& windows()
		{
			static std::map<GLFWwindow*, GLFWWindow*> w;
			return w;
		}
	}

	void GLFWWindow::registerThis() throw()
	{
		if (window.valid())
			windows()[window] = this;
	}
	void GLFWWindow::unregisterThis() throw()
	{
		if (window.valid())
			windows().erase(window);
	}

	void GLFWWindow::resizeCallback(GLFWwindow *window, int w, int h)
	{
		auto* self = windows()[window];
		if (self && self->resize)
			self->resize((unsigned) w, (unsigned) h);
	}

	void GLFWWindow::keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
	{
		auto* self = windows()[window];
		if (self && self->keyboard)
			self->keyboard(key, action != GLFW_RELEASE, action == GLFW_REPEAT);
	}

	void GLFWWindow::textCallback(GLFWwindow *window, unsigned chr)
	{
		auto* self = windows()[window];
		if (self && self->text)
			self->text(chr);
	}

	void GLFWWindow::mouseCallback(GLFWwindow *window, double x, double y)
	{
		auto* self = windows()[window];
		if (self && self->mouse)
			self->mouse((int) x, (int) y);
	}

	void GLFWWindow::buttonsCallback(GLFWwindow *window, int button, int action, int mods)
	{
		auto* self = windows()[window];
		if (self && self->buttons)
			self->buttons(button, action == GLFW_PRESS);
	}

} // namespace
