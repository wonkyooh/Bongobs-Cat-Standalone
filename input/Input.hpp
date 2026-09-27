#pragma once
#include <string>
struct GLFWwindow;
class EventManager;
namespace Input {
enum class Backend { Hook, RawInput, Glfw };
Backend ParseBackend(const std::string& s);            // "hook"|"rawinput"|"glfw"; unknown -> Hook (Glfw on non-Windows)
struct Config { Backend backend = Backend::Hook; };
bool Start(const Config& cfg, EventManager* sink, GLFWwindow* window); // window only used by Glfw backend
void Stop();
const char* ActiveBackendName();
void OnGlfwKey(int key, int scancode, int action, int mods);          // forward from your GLFW key callback
void OnGlfwMouseButton(int button, int action, int mods);             // forward from your GLFW mouse-button callback
void GetCursorPosition(int& x, int& y);   // desktop pixel coords
void GetDesktopSize(int& w, int& h);      // primary monitor size
}
