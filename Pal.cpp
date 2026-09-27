#include "Pal.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstdarg>
#include <cstdio>
#include <system_error>
#include "Define.hpp"
#include "Platform.hpp"
#include "input/Input.hpp"

using std::endl;
using namespace Csm;
using namespace std;
using namespace Define;

namespace fs = std::filesystem;

csmByte* Pal::LoadFileAsBytes(const string filePath, csmSizeInt* outSize)
{
    *outSize = 0;

    // u8path() so non-ASCII (e.g. Korean) paths round-trip correctly on every
    // platform; on Windows this is what lets std::ifstream open a wide path.
    fs::path path = fs::u8path(filePath);

    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open())
    {
	PrintLog("[APP]file not found: %s", filePath.c_str());
	return NULL;
    }

    std::error_code ec;
    uintmax_t size = fs::file_size(path, ec);
    if (ec)
    {
	PrintLog("[APP]failed to stat file: %s", filePath.c_str());
	return NULL;
    }

    char* buf = new char[size];
    if (size > 0 && !file.read(buf, static_cast<std::streamsize>(size)))
    {
	PrintLog("[APP]failed to read file: %s", filePath.c_str());
	delete[] buf;
	return NULL;
    }

    *outSize = static_cast<csmSizeInt>(size);
    return reinterpret_cast<csmByte*>(buf);
}

void Pal::ReleaseBytes(csmByte* byteData)
{
    delete[] byteData;
}

void Pal::PrintLog(const csmChar *format, ...)
{
    va_list args;
    csmChar buf[2048];
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

#ifdef CSM_DEBUG_MEMORY_LEAKING
// メモリリークチェック時は大量の標準出力がはしり重いのでprintfを利用する
    std::printf("%s", buf);
#else
    std::cerr << buf << std::endl;
#endif
    Platform::AppendLog(buf);
}

void Pal::PrintMessage(const csmChar* message)
{
    PrintLog("%s", message);
}

bool Pal::IsFileExist(const Csm::csmChar *csDir)
{
	std::error_code ec;
	return fs::exists(fs::u8path(std::string(csDir)), ec);
}

int Pal::GetAllDirName(const Csm::csmChar *csDir, Csm::csmChar **Files)
{
	return 0;
}

std::string Pal::GetModelName(const std::string &filePath)
{
	std::error_code ec;
	fs::path dir = fs::u8path(filePath);
	if (!fs::exists(dir, ec) || !fs::is_directory(dir, ec))
		return "";

	const std::string suffix = ".model3.json";
	for (const auto &entry : fs::directory_iterator(dir, ec))
	{
		if (ec)
			break;
		if (!entry.is_regular_file())
			continue;

		std::string name = entry.path().filename().u8string();
		if (name.size() >= suffix.size() &&
		    name.compare(name.size() - suffix.size(), suffix.size(), suffix) == 0)
			return name;
	}

	return "";
}

void Pal::GetDesktopResolution(int &horizontal, int &vertical) {
	Input::GetDesktopSize(horizontal, vertical);
}
