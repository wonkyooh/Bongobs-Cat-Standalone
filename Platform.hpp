#pragma once

#include <string>
#include <vector>

// Small OS-abstraction layer used only by main.cpp/AppConfig/Pal. Every
// function here is safe to call on all platforms; anything that truly needs
// Win32 lives behind #ifdef _WIN32 inside Platform.cpp so the rest of the
// app (and Pal.cpp in particular) never has to #include <Windows.h>.
namespace Platform {

// Absolute, UTF-8, no trailing slash: the directory containing the running
// executable. Used to resolve config.json / Resources / the log file next
// to BongobsCat.exe regardless of the current working directory.
std::string ExeDir();

// Truncates (or creates) the log file at `path` for this run. Call once at
// startup, before any Pal::PrintLog() call that should reach the file.
void InitLog(const std::string &path);

// Appends one line to the log file opened by InitLog (no-op if InitLog was
// never called, or failed to open). Prefixes a monotonic millisecond
// timestamp. Does not also print to stderr -- callers (Pal::PrintLog) do
// that themselves.
void AppendLog(const std::string &line);

// argv, decoded as UTF-8. On Windows this re-derives the argument list from
// GetCommandLineW()/CommandLineToArgvW() so non-ASCII arguments (e.g. a
// --config path under a Korean user folder) survive; elsewhere it just
// copies the given argv (already UTF-8 on macOS/Linux).
std::vector<std::string> GetUtf8Args(int argc, char **argv);

// Windows: AllocConsole() and redirects stdout/stderr/stdin to it, for
// --console. No-op elsewhere (stdio already goes to a terminal there).
void AttachConsole();

// Windows: MessageBoxW with the given UTF-8 title/message. Elsewhere: prints
// the message to stderr as a fallback (there is no message box to show).
void ShowErrorMessageBox(const std::string &title, const std::string &message);

} // namespace Platform
