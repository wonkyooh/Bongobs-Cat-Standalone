// Backend 3: focused-window-only fallback built on GLFW's own callbacks.
// Used as the sole backend on non-Windows, and optionally on Windows when
// Backend::Glfw is explicitly requested (e.g. for dev/testing without hooks).
// Unlike the Win32 backends this only sees input while the app window has
// focus -- there is no system-wide capture without platform-specific APIs.
#include "Input.hpp"
#include "KeyMap.hpp"
#include "../EventManager.hpp"

#include <GLFW/glfw3.h>

namespace {
EventManager* g_sink = nullptr;
GLFWwindow* g_window = nullptr;
} // namespace

namespace Input {

void Glfw_Start(EventManager* sink, GLFWwindow* window)
{
	g_sink = sink;
	g_window = window;
}

void Glfw_Stop()
{
	g_sink = nullptr;
	g_window = nullptr;
}

void Glfw_OnKey(int key, int /*scancode*/, int action, int /*mods*/)
{
	if (!g_sink || action == GLFW_REPEAT) {
		return;
	}

	const int idx = GlfwKeyToKeyIndex(key);
	if (idx < 0) {
		return;
	}

	if (action == GLFW_PRESS) {
		g_sink->KeyEventDown(idx);
	} else if (action == GLFW_RELEASE) {
		g_sink->KeyEventUp(idx);
	}
}

void Glfw_OnMouseButton(int button, int action, int /*mods*/)
{
	if (!g_sink) {
		return;
	}

	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		if (action == GLFW_PRESS) g_sink->LeftButtonDown();
		else if (action == GLFW_RELEASE) g_sink->LeftButtonUp();
	} else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
		if (action == GLFW_PRESS) g_sink->RightButtonDown();
		else if (action == GLFW_RELEASE) g_sink->RightButtonUp();
	}
}

void Glfw_GetCursorPosition(int& x, int& y)
{
	if (!g_window) {
		x = 0;
		y = 0;
		return;
	}

	double cx = 0.0, cy = 0.0;
	glfwGetCursorPos(g_window, &cx, &cy);

	int wx = 0, wy = 0;
	glfwGetWindowPos(g_window, &wx, &wy);

	x = wx + static_cast<int>(cx);
	y = wy + static_cast<int>(cy);
}

void Glfw_GetDesktopSize(int& w, int& h)
{
	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = monitor ? glfwGetVideoMode(monitor) : nullptr;
	if (!mode) {
		w = 0;
		h = 0;
		return;
	}

	w = mode->width;
	h = mode->height;
}

} // namespace Input
