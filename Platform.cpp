#include "Platform.hpp"

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <mutex>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <shellapi.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <climits>
#else
#include <climits>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace {

std::mutex g_logMutex;
std::ofstream g_logFile;
std::chrono::steady_clock::time_point g_logStart;

std::string NormalizeExeDir(const std::string &exePathUtf8)
{
	std::error_code ec;
	fs::path p = fs::u8path(exePathUtf8);
	fs::path canon = fs::canonical(p, ec);
	if (ec)
		canon = p;
	return canon.parent_path().u8string();
}

#if defined(_WIN32)
std::string WideToUtf8(const wchar_t *wide)
{
	if (!wide)
		return std::string();
	int size = WideCharToMultiByte(CP_UTF8, 0, wide, -1, NULL, 0, NULL, NULL);
	if (size <= 0)
		return std::string();
	std::string out(static_cast<size_t>(size - 1), '\0');
	WideCharToMultiByte(CP_UTF8, 0, wide, -1, out.data(), size, NULL, NULL);
	return out;
}

std::wstring Utf8ToWide(const std::string &utf8)
{
	if (utf8.empty())
		return std::wstring();
	int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, NULL, 0);
	if (size <= 0)
		return std::wstring();
	std::wstring out(static_cast<size_t>(size - 1), L'\0');
	MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, out.data(), size);
	return out;
}

// Quotes/escapes one argument per the Win32 argv convention (the same rules
// CommandLineToArgvW parses by), so args containing spaces or quotes survive
// being joined into a single command-line string for ShellExecuteW.
std::wstring QuoteArgForWindows(const std::wstring &arg)
{
	if (!arg.empty() && arg.find_first_of(L" \t\"") == std::wstring::npos)
		return arg;

	std::wstring out = L"\"";
	for (size_t i = 0; i < arg.size();) {
		size_t backslashes = 0;
		while (i < arg.size() && arg[i] == L'\\') {
			backslashes++;
			i++;
		}
		if (i == arg.size()) {
			// Trailing backslashes: double them since the closing quote follows.
			out.append(backslashes * 2, L'\\');
			break;
		} else if (arg[i] == L'"') {
			// Backslashes before a literal quote: double them, then escape the quote.
			out.append(backslashes * 2 + 1, L'\\');
			out.push_back(L'"');
			i++;
		} else {
			out.append(backslashes, L'\\');
			out.push_back(arg[i]);
			i++;
		}
	}
	out.push_back(L'"');
	return out;
}
#endif

} // namespace

namespace Platform {

std::string ExeDir()
{
#if defined(_WIN32)
	std::vector<wchar_t> buf(MAX_PATH);
	for (;;) {
		DWORD len = GetModuleFileNameW(NULL, buf.data(), static_cast<DWORD>(buf.size()));
		if (len == 0)
			return std::string();
		if (len < buf.size()) {
			return NormalizeExeDir(WideToUtf8(buf.data()));
		}
		buf.resize(buf.size() * 2);
	}
#elif defined(__APPLE__)
	uint32_t size = 0;
	_NSGetExecutablePath(NULL, &size);
	std::vector<char> buf(size + 1, '\0');
	if (_NSGetExecutablePath(buf.data(), &size) != 0)
		return std::string();
	return NormalizeExeDir(std::string(buf.data()));
#else
	char buf[PATH_MAX] = {0};
	ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
	if (len <= 0)
		return std::string();
	buf[len] = '\0';
	return NormalizeExeDir(std::string(buf));
#endif
}

void InitLog(const std::string &path)
{
	std::lock_guard<std::mutex> lock(g_logMutex);
	g_logFile.open(fs::u8path(path), std::ios::out | std::ios::trunc);
	g_logStart = std::chrono::steady_clock::now();
}

void AppendLog(const std::string &line)
{
	std::lock_guard<std::mutex> lock(g_logMutex);
	if (!g_logFile.is_open())
		return;

	auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now() - g_logStart)
			       .count();
	g_logFile << "[+" << elapsed << "ms] " << line << std::endl;
}

std::vector<std::string> GetUtf8Args(int argc, char **argv)
{
	std::vector<std::string> result;

#if defined(_WIN32)
	int wargc = 0;
	LPWSTR *wargv = CommandLineToArgvW(GetCommandLineW(), &wargc);
	if (wargv) {
		result.reserve(static_cast<size_t>(wargc));
		for (int i = 0; i < wargc; i++)
			result.push_back(WideToUtf8(wargv[i]));
		LocalFree(wargv);
		return result;
	}
	// Fall through to argv if CommandLineToArgvW somehow failed.
#endif

	result.reserve(static_cast<size_t>(argc));
	for (int i = 0; i < argc; i++)
		result.push_back(argv[i] ? argv[i] : "");
	return result;
}

void AttachConsole()
{
#if defined(_WIN32)
	if (!::AllocConsole())
		return;

	FILE *fp = nullptr;
	freopen_s(&fp, "CONOUT$", "w", stdout);
	freopen_s(&fp, "CONOUT$", "w", stderr);
	freopen_s(&fp, "CONIN$", "r", stdin);
	SetConsoleOutputCP(CP_UTF8);
#endif
	// No-op on macOS/Linux: launched from a terminal, stdio already visible.
}

void ShowErrorMessageBox(const std::string &title, const std::string &message)
{
#if defined(_WIN32)
	MessageBoxW(NULL, Utf8ToWide(message).c_str(), Utf8ToWide(title).c_str(), MB_OK | MB_ICONERROR);
#else
	std::fprintf(stderr, "%s: %s\n", title.c_str(), message.c_str());
#endif
}

bool IsElevated()
{
#if defined(_WIN32)
	HANDLE token = NULL;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
		return false;

	TOKEN_ELEVATION elevation;
	DWORD size = sizeof(elevation);
	bool elevated = false;
	if (GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size))
		elevated = elevation.TokenIsElevated != 0;

	CloseHandle(token);
	return elevated;
#else
	return true;
#endif
}

bool RelaunchElevated(const std::vector<std::string> &args)
{
#if defined(_WIN32)
	std::vector<wchar_t> buf(MAX_PATH);
	for (;;) {
		DWORD len = GetModuleFileNameW(NULL, buf.data(), static_cast<DWORD>(buf.size()));
		if (len == 0)
			return false;
		if (len < buf.size())
			break;
		buf.resize(buf.size() * 2);
	}
	std::wstring exePath(buf.data());

	std::wstring cmdLine;
	for (size_t i = 0; i < args.size(); i++) {
		if (i > 0)
			cmdLine += L' ';
		cmdLine += QuoteArgForWindows(Utf8ToWide(args[i]));
	}

	HINSTANCE result = ShellExecuteW(nullptr, L"runas", exePath.c_str(), cmdLine.c_str(), nullptr, SW_SHOWNORMAL);
	return reinterpret_cast<INT_PTR>(result) > 32;
#else
	(void)args;
	return false;
#endif
}

} // namespace Platform
