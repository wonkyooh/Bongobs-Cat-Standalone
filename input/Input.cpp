#include "Input.hpp"
#include "../Pal.hpp"

#include <cctype>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

// Internal per-backend entry points. Each backend .cpp defines the functions
// declared for its platform; there is no shared header for these because
// only this file calls them (the linker matches on namespace + signature).
namespace Input {

#ifdef _WIN32
// Implemented in InputWin32Hook.cpp.
bool Win32Hook_Start(EventManager* sink);
void Win32Hook_Stop();
// Implemented in InputWin32RawInput.cpp.
bool Win32Raw_Start(EventManager* sink);
void Win32Raw_Stop();
#endif

// Implemented in InputGlfw.cpp. Compiled on every platform: reachable even on
// Windows when Backend::Glfw is explicitly requested, and it is the only
// backend available at all on non-Windows.
void Glfw_Start(EventManager* sink, GLFWwindow* window);
void Glfw_Stop();
void Glfw_OnKey(int key, int scancode, int action, int mods);
void Glfw_OnMouseButton(int button, int action, int mods);
void Glfw_GetCursorPosition(int& x, int& y);
void Glfw_GetDesktopSize(int& w, int& h);

} // namespace Input

namespace {

Input::Backend g_active = Input::Backend::Hook;
bool g_running = false;

const char* BackendName(Input::Backend b)
{
	switch (b) {
	case Input::Backend::Hook: return "hook";
	case Input::Backend::RawInput: return "rawinput";
	case Input::Backend::Glfw: return "glfw";
	}
	return "unknown";
}

} // namespace

Input::Backend Input::ParseBackend(const std::string& s)
{
	std::string lower;
	lower.reserve(s.size());
	for (char c : s) {
		lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
	}

	if (lower == "hook") return Backend::Hook;
	if (lower == "rawinput") return Backend::RawInput;
	if (lower == "glfw") return Backend::Glfw;

#ifdef _WIN32
	return Backend::Hook;
#else
	return Backend::Glfw;
#endif
}

bool Input::Start(const Config& cfg, EventManager* sink, GLFWwindow* window)
{
	if (g_running) {
		Pal::PrintLog("Input::Start called while backend '%s' is already running; ignoring", ActiveBackendName());
		return false;
	}

	Backend backend = cfg.backend;
	bool ok;

#ifdef _WIN32
	if (backend == Backend::Hook) {
		ok = Win32Hook_Start(sink);
	} else if (backend == Backend::RawInput) {
		ok = Win32Raw_Start(sink);
	} else {
		Glfw_Start(sink, window);
		ok = true;
	}
#else
	if (backend != Backend::Glfw) {
		Pal::PrintLog("Input: backend '%s' is not available on this platform, falling back to glfw", BackendName(backend));
		backend = Backend::Glfw;
	}
	Glfw_Start(sink, window);
	ok = true;
#endif

	if (ok) {
		g_active = backend;
		g_running = true;
	} else {
		Pal::PrintLog("Input: failed to start backend '%s'", BackendName(backend));
	}
	return ok;
}

void Input::Stop()
{
	if (!g_running) {
		return;
	}

#ifdef _WIN32
	if (g_active == Backend::Hook) {
		Win32Hook_Stop();
	} else if (g_active == Backend::RawInput) {
		Win32Raw_Stop();
	} else {
		Glfw_Stop();
	}
#else
	Glfw_Stop();
#endif

	g_running = false;
}

const char* Input::ActiveBackendName()
{
	return g_running ? BackendName(g_active) : "none";
}

void Input::OnGlfwKey(int key, int scancode, int action, int mods)
{
	if (g_running && g_active == Backend::Glfw) {
		Glfw_OnKey(key, scancode, action, mods);
	}
}

void Input::OnGlfwMouseButton(int button, int action, int mods)
{
	if (g_running && g_active == Backend::Glfw) {
		Glfw_OnMouseButton(button, action, mods);
	}
}

#ifdef _WIN32

void Input::GetCursorPosition(int& x, int& y)
{
	POINT p;
	GetCursorPos(&p);
	x = p.x;
	y = p.y;
}

void Input::GetDesktopSize(int& w, int& h)
{
	w = GetSystemMetrics(SM_CXSCREEN);
	h = GetSystemMetrics(SM_CYSCREEN);
}

#else

void Input::GetCursorPosition(int& x, int& y)
{
	Glfw_GetCursorPosition(x, y);
}

void Input::GetDesktopSize(int& w, int& h)
{
	Glfw_GetDesktopSize(w, h);
}

#endif
