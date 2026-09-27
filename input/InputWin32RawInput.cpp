// Backend 2: pure Raw Input (keyboard + mouse), no WH_*_LL hooks. Useful when
// an overlay or anti-cheat starves/blocks low-level hooks; Raw Input delivery
// goes through the normal window message queue instead.
#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <hidusage.h>

#include "Input.hpp"
#include "KeyMap.hpp"
#include "../EventManager.hpp"
#include "../Pal.hpp"

#include <future>
#include <thread>

#ifndef HID_USAGE_PAGE_GENERIC
#define HID_USAGE_PAGE_GENERIC ((USHORT)0x01)
#endif
#ifndef HID_USAGE_GENERIC_MOUSE
#define HID_USAGE_GENERIC_MOUSE ((USHORT)0x02)
#endif
#ifndef HID_USAGE_GENERIC_KEYBOARD
#define HID_USAGE_GENERIC_KEYBOARD ((USHORT)0x06)
#endif

namespace {

const wchar_t* const kClassName = L"BongobsCatInputRawInput";

EventManager* g_sink = nullptr;
std::thread g_thread;
DWORD g_threadId = 0;

void HandleMouse(const RAWMOUSE& mouse)
{
	if (!g_sink) {
		return;
	}
	if (!(mouse.usFlags & MOUSE_MOVE_ABSOLUTE)) {
		g_sink->SetRelativeMouse(mouse.lLastX, mouse.lLastY);
	}
	if (mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN) g_sink->LeftButtonDown();
	if (mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP) g_sink->LeftButtonUp();
	if (mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN) g_sink->RightButtonDown();
	if (mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP) g_sink->RightButtonUp();
}

void HandleKeyboard(const RAWKEYBOARD& kb)
{
	if (!g_sink || kb.VKey == 0xFF) {
		// 0xFF: "fake" key some keyboards send as part of an escaped sequence
		// (e.g. Pause/Break), not a real key.
		return;
	}

	unsigned vk = kb.VKey;
	if (vk == VK_SHIFT) {
		// VK_SHIFT is undifferentiated; disambiguate via scan code.
		vk = (kb.MakeCode == 0x36) ? VK_RSHIFT : VK_LSHIFT;
	} else if (vk == VK_CONTROL) {
		// Likewise for VK_CONTROL, via the E0 extended-key flag.
		vk = (kb.Flags & RI_KEY_E0) ? VK_RCONTROL : VK_LCONTROL;
	}
	// (Menu/Alt keys aren't in Define::KeyDefine; VkToKeyIndex naturally
	// returns -1 for them below, so no special-casing needed.)

	const int idx = Input::VkToKeyIndex(vk);
	if (idx < 0) {
		return;
	}

	if (kb.Flags & RI_KEY_BREAK) {
		g_sink->KeyEventUp(idx);
	} else {
		g_sink->KeyEventDown(idx);
	}
}

LRESULT CALLBACK HiddenWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_INPUT) {
		UINT size = 0;
		GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER));

		// We only ever register keyboard/mouse, so this always fits; skip
		// anything that doesn't rather than risk overrunning the stack buffer.
		if (size > 0 && size <= sizeof(RAWINPUT)) {
			RAWINPUT raw;
			UINT copied = size;
			if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &raw, &copied, sizeof(RAWINPUTHEADER)) == size) {
				if (raw.header.dwType == RIM_TYPEMOUSE) {
					HandleMouse(raw.data.mouse);
				} else if (raw.header.dwType == RIM_TYPEKEYBOARD) {
					HandleKeyboard(raw.data.keyboard);
				}
			}
		}
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void ThreadMain(std::promise<bool> startResult)
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

	const HINSTANCE hInst = GetModuleHandle(nullptr);
	WNDCLASSEXW wc{};
	wc.cbSize = sizeof(wc);
	wc.lpfnWndProc = HiddenWndProc;
	wc.hInstance = hInst;
	wc.lpszClassName = kClassName;
	if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
		startResult.set_value(false);
		return;
	}

	// Hidden top-level window used only as a Raw Input target: WS_OVERLAPPED,
	// never shown. Deliberately NOT HWND_MESSAGE -- message-only windows do
	// not reliably receive WM_INPUT together with RIDEV_INPUTSINK.
	const HWND hiddenWnd = CreateWindowExW(0, kClassName, L"", WS_OVERLAPPED, 0, 0, 0, 0, nullptr, nullptr, hInst, nullptr);
	if (!hiddenWnd) {
		UnregisterClassW(kClassName, hInst);
		startResult.set_value(false);
		return;
	}

	RAWINPUTDEVICE rid[2];
	rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
	rid[0].usUsage = HID_USAGE_GENERIC_MOUSE;
	rid[0].dwFlags = RIDEV_INPUTSINK;
	rid[0].hwndTarget = hiddenWnd;
	rid[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
	rid[1].usUsage = HID_USAGE_GENERIC_KEYBOARD;
	rid[1].dwFlags = RIDEV_INPUTSINK;
	rid[1].hwndTarget = hiddenWnd;
	if (!RegisterRawInputDevices(rid, 2, sizeof(RAWINPUTDEVICE))) {
		DestroyWindow(hiddenWnd);
		UnregisterClassW(kClassName, hInst);
		startResult.set_value(false);
		return;
	}

	g_threadId = GetCurrentThreadId();
	startResult.set_value(true);

	MSG msg;
	BOOL ret;
	while ((ret = GetMessage(&msg, nullptr, 0, 0)) != 0) {
		if (ret == -1) {
			break;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// Cleanup happens on this (the hook) thread, before it exits.
	DestroyWindow(hiddenWnd);
	UnregisterClassW(kClassName, hInst);
}

} // namespace

namespace Input {

bool Win32Raw_Start(EventManager* sink)
{
	if (g_threadId != 0) {
		Pal::PrintLog("Input(rawinput): Start called while already running");
		return false;
	}

	g_sink = sink;

	std::promise<bool> startResult;
	std::future<bool> future = startResult.get_future();
	g_thread = std::thread(ThreadMain, std::move(startResult));

	const bool ok = future.get();
	if (!ok) {
		g_thread.join();
		g_sink = nullptr;
	}
	return ok;
}

void Win32Raw_Stop()
{
	if (g_threadId == 0) {
		return;
	}

	PostThreadMessage(g_threadId, WM_QUIT, 0, 0);
	if (g_thread.joinable()) {
		g_thread.join();
	}
	g_threadId = 0;
	g_sink = nullptr;
}

} // namespace Input

#endif // _WIN32
