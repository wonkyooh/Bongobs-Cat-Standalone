// Backend 1 (default on Windows): WH_KEYBOARD_LL + WH_MOUSE_LL low-level hooks
// for keys/buttons, plus Raw Input on a hidden window for relative mouse
// motion. This is the path the original OBS plugin shipped to 1M+ users.
//
// Known limitation (UIPI): a non-elevated process does not receive input
// directed at an elevated (run-as-administrator) foreground window/hook
// chain. If the game runs elevated, BongobsCat must also run elevated.
#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

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

namespace {

const wchar_t* const kClassName = L"BongobsCatInputHook";

EventManager* g_sink = nullptr;
std::thread g_thread;
DWORD g_threadId = 0;

// --- Hook procedures -------------------------------------------------------
// These must stay trivial: no allocation, no locks, no logging. Windows
// silently drops a WH_KEYBOARD_LL/WH_MOUSE_LL hook whose procedure takes
// longer than LowLevelHooksTimeout (default 300 ms) to return.

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION) {
		const KBDLLHOOKSTRUCT* p = reinterpret_cast<const KBDLLHOOKSTRUCT*>(lParam);
		// Treat Alt-held variants (WM_SYSKEY*) as ordinary key events too --
		// common in games (e.g. Alt+key binds) and ignored by the old plugin.
		if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
			const int idx = Input::VkToKeyIndex(static_cast<unsigned>(p->vkCode));
			if (idx >= 0 && g_sink) {
				g_sink->KeyEventDown(idx);
			}
		} else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
			const int idx = Input::VkToKeyIndex(static_cast<unsigned>(p->vkCode));
			if (idx >= 0 && g_sink) {
				g_sink->KeyEventUp(idx);
			}
		}
	}
	return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION && g_sink) {
		switch (wParam) {
		case WM_LBUTTONDOWN:
			g_sink->LeftButtonDown();
			break;
		case WM_LBUTTONUP:
			g_sink->LeftButtonUp();
			break;
		case WM_RBUTTONDOWN:
			g_sink->RightButtonDown();
			break;
		case WM_RBUTTONUP:
			g_sink->RightButtonUp();
			break;
		default:
			break;
		}
	}
	return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

LRESULT CALLBACK HiddenWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_INPUT) {
		UINT size = 0;
		GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER));

		// We only ever register the mouse, so this always fits; skip anything
		// that doesn't rather than risk overrunning the stack buffer.
		if (size > 0 && size <= sizeof(RAWINPUT)) {
			RAWINPUT raw;
			UINT copied = size;
			if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &raw, &copied, sizeof(RAWINPUTHEADER)) == size &&
			    raw.header.dwType == RIM_TYPEMOUSE &&
			    !(raw.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) &&
			    g_sink) {
				g_sink->SetRelativeMouse(raw.data.mouse.lLastX, raw.data.mouse.lLastY);
			}
		}
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void ThreadMain(std::promise<bool> startResult)
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

	const HHOOK hKeyboard = SetWindowsHookExW(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(nullptr), 0);
	const HHOOK hMouse = SetWindowsHookExW(WH_MOUSE_LL, MouseProc, GetModuleHandle(nullptr), 0);
	if (!hKeyboard || !hMouse) {
		if (hKeyboard) UnhookWindowsHookEx(hKeyboard);
		if (hMouse) UnhookWindowsHookEx(hMouse);
		startResult.set_value(false);
		return;
	}

	const HINSTANCE hInst = GetModuleHandle(nullptr);
	WNDCLASSEXW wc{};
	wc.cbSize = sizeof(wc);
	wc.lpfnWndProc = HiddenWndProc;
	wc.hInstance = hInst;
	wc.lpszClassName = kClassName;
	if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
		UnhookWindowsHookEx(hKeyboard);
		UnhookWindowsHookEx(hMouse);
		startResult.set_value(false);
		return;
	}

	// Hidden top-level window used only as a Raw Input target: WS_OVERLAPPED,
	// never shown. Deliberately NOT HWND_MESSAGE -- message-only windows do
	// not reliably receive WM_INPUT together with RIDEV_INPUTSINK.
	const HWND hiddenWnd = CreateWindowExW(0, kClassName, L"", WS_OVERLAPPED, 0, 0, 0, 0, nullptr, nullptr, hInst, nullptr);
	if (!hiddenWnd) {
		UnregisterClassW(kClassName, hInst);
		UnhookWindowsHookEx(hKeyboard);
		UnhookWindowsHookEx(hMouse);
		startResult.set_value(false);
		return;
	}

	RAWINPUTDEVICE rid{};
	rid.usUsagePage = HID_USAGE_PAGE_GENERIC;
	rid.usUsage = HID_USAGE_GENERIC_MOUSE;
	rid.dwFlags = RIDEV_INPUTSINK;
	rid.hwndTarget = hiddenWnd;
	if (!RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
		// Raw mouse motion only feeds the optional relative_mouse mode; keys and
		// buttons come from the hooks, so keep the backend alive without it.
		Pal::PrintLog("Input(hook): RegisterRawInputDevices failed (error %lu); relative mouse motion disabled",
			      static_cast<unsigned long>(GetLastError()));
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
	UnhookWindowsHookEx(hKeyboard);
	UnhookWindowsHookEx(hMouse);
}

} // namespace

namespace Input {

bool Win32Hook_Start(EventManager* sink)
{
	if (g_threadId != 0) {
		Pal::PrintLog("Input(hook): Start called while already running");
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

void Win32Hook_Stop()
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
