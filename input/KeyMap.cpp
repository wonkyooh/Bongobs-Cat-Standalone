#include "KeyMap.hpp"
#include <GLFW/glfw3.h>

// Plain numeric VK_* values from WinUser.h, for the keys BongobsCat maps.
// Defined locally (see KeyMap.hpp) instead of including <Windows.h>.
namespace {
namespace Vk {
constexpr unsigned Space = 0x20;
constexpr unsigned LShift = 0xA0;
constexpr unsigned RShift = 0xA1;
constexpr unsigned LControl = 0xA2;
constexpr unsigned RControl = 0xA3;
constexpr unsigned F1 = 0x70;      // F2..F12 follow contiguously
constexpr unsigned Up = 0x26;
constexpr unsigned Down = 0x28;
constexpr unsigned Left = 0x25;
constexpr unsigned Right = 0x27;
constexpr unsigned OemComma = 0xBC;  // <
constexpr unsigned OemPeriod = 0xBE; // >
constexpr unsigned Oem4 = 0xDB;      // [
constexpr unsigned Oem6 = 0xDD;      // ]
constexpr unsigned Numpad0 = 0x60;   // Numpad1..9 follow contiguously
} // namespace Vk
} // namespace

int Input::VkToKeyIndex(unsigned vk)
{
	// Verbatim from the original Bongobs Cat Hook.cpp's HookCode(), which
	// shipped this exact table to 1M+ users. Do not "clean up" entries
	// without re-checking against Define::KeyDefine[61] in Define.cpp first.
	switch (vk) {
	// a..z
	case 0x41: return 0;
	case 0x42: return 1;
	case 0x43: return 2;
	case 0x44: return 3;
	case 0x45: return 4;
	case 0x46: return 5;
	case 0x47: return 6;
	case 0x48: return 7;
	case 0x49: return 8;
	case 0x4A: return 9;
	case 0x4B: return 10;
	case 0x4C: return 11;
	case 0x4D: return 12;
	case 0x4E: return 13;
	case 0x4F: return 14;
	case 0x50: return 15;
	case 0x51: return 16;
	case 0x52: return 17;
	case 0x53: return 18;
	case 0x54: return 19;
	case 0x55: return 20;
	case 0x56: return 21;
	case 0x57: return 22;
	case 0x58: return 23;
	case 0x59: return 24;
	case 0x5A: return 25;

	// 0..9 (top-row digits)
	case 0x30: return 26;
	case 0x31: return 27;
	case 0x32: return 28;
	case 0x33: return 29;
	case 0x34: return 30;
	case 0x35: return 31;
	case 0x36: return 32;
	case 0x37: return 33;
	case 0x38: return 34;
	case 0x39: return 35;

	case Vk::Space: return 36;
	case Vk::LShift: return 37;
	case Vk::LControl: return 38;

	case Vk::F1 + 0: return 39;
	case Vk::F1 + 1: return 40;
	case Vk::F1 + 2: return 41;
	case Vk::F1 + 3: return 42;
	case Vk::F1 + 4: return 43;
	case Vk::F1 + 5: return 44;
	case Vk::F1 + 6: return 45;
	case Vk::F1 + 7: return 46;
	case Vk::F1 + 8: return 47;
	case Vk::F1 + 9: return 48;
	case Vk::F1 + 10: return 49;
	case Vk::F1 + 11: return 50;

	case Vk::Up: return 51;
	case Vk::Down: return 52;
	// NOTE: Hook.cpp's original table maps VK_RIGHT->53 and VK_LEFT->54, which
	// is swapped relative to Define::KeyDefine's label order at those two
	// slots ("...,left,right" would suggest left=53/right=54). Preserved
	// verbatim per spec so existing behavior/configs don't change.
	// GlfwKeyToKeyIndex mirrors this swap so both backends stay consistent.
	case Vk::Right: return 53;
	case Vk::Left: return 54;

	case Vk::OemComma: return 55;  // <
	case Vk::OemPeriod: return 56; // >
	case Vk::Oem4: return 57;      // [
	case Vk::Oem6: return 58;      // ]
	case Vk::RShift: return 59;
	case Vk::RControl: return 60;

	// Numpad digits: absent from the original Hook.cpp table. Added per spec
	// so NumLock-on numpad digit presses drive the same paws as the top-row
	// digits (0x60..0x69 = VK_NUMPAD0..VK_NUMPAD9).
	case Vk::Numpad0 + 0: return 26;
	case Vk::Numpad0 + 1: return 27;
	case Vk::Numpad0 + 2: return 28;
	case Vk::Numpad0 + 3: return 29;
	case Vk::Numpad0 + 4: return 30;
	case Vk::Numpad0 + 5: return 31;
	case Vk::Numpad0 + 6: return 32;
	case Vk::Numpad0 + 7: return 33;
	case Vk::Numpad0 + 8: return 34;
	case Vk::Numpad0 + 9: return 35;

	default: return -1;
	}
}

int Input::GlfwKeyToKeyIndex(int glfwKey)
{
	if (glfwKey >= GLFW_KEY_A && glfwKey <= GLFW_KEY_Z) {
		return glfwKey - GLFW_KEY_A; // 0..25
	}
	if (glfwKey >= GLFW_KEY_0 && glfwKey <= GLFW_KEY_9) {
		return 26 + (glfwKey - GLFW_KEY_0); // 26..35
	}
	if (glfwKey >= GLFW_KEY_KP_0 && glfwKey <= GLFW_KEY_KP_9) {
		return 26 + (glfwKey - GLFW_KEY_KP_0); // numpad shares 26..35
	}
	if (glfwKey >= GLFW_KEY_F1 && glfwKey <= GLFW_KEY_F12) {
		return 39 + (glfwKey - GLFW_KEY_F1); // 39..50
	}

	switch (glfwKey) {
	case GLFW_KEY_SPACE: return 36;
	case GLFW_KEY_LEFT_SHIFT: return 37;
	case GLFW_KEY_LEFT_CONTROL: return 38;
	case GLFW_KEY_UP: return 51;
	case GLFW_KEY_DOWN: return 52;
	// Mirrors VkToKeyIndex's swap (see comment there) so the same physical
	// key produces the same index on every backend.
	case GLFW_KEY_RIGHT: return 53;
	case GLFW_KEY_LEFT: return 54;
	case GLFW_KEY_COMMA: return 55;         // <
	case GLFW_KEY_PERIOD: return 56;        // >
	case GLFW_KEY_LEFT_BRACKET: return 57;  // [
	case GLFW_KEY_RIGHT_BRACKET: return 58; // ]
	case GLFW_KEY_RIGHT_SHIFT: return 59;
	case GLFW_KEY_RIGHT_CONTROL: return 60;
	default: return -1;
	}
}
