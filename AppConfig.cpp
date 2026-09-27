#include "AppConfig.hpp"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include "Pal.hpp"
#include "json.h"

namespace {

bool GetBool(const Json::Value &obj, const char *key, bool def)
{
	if (!obj.isObject() || !obj.isMember(key) || !obj[key].isBool())
		return def;
	return obj[key].asBool();
}

int GetInt(const Json::Value &obj, const char *key, int def)
{
	if (!obj.isObject() || !obj.isMember(key) || !obj[key].isNumeric())
		return def;
	return obj[key].asInt();
}

double GetDouble(const Json::Value &obj, const char *key, double def)
{
	if (!obj.isObject() || !obj.isMember(key) || !obj[key].isNumeric())
		return def;
	return obj[key].asDouble();
}

std::string GetString(const Json::Value &obj, const char *key, const std::string &def)
{
	if (!obj.isObject() || !obj.isMember(key) || !obj[key].isString())
		return def;
	return obj[key].asString();
}

const Json::Value &GetObject(const Json::Value &root, const char *key)
{
	static const Json::Value emptyObject{Json::objectValue};
	if (!root.isObject() || !root.isMember(key) || !root[key].isObject())
		return emptyObject;
	return root[key];
}

// Accepts "#RRGGBB" or "RRGGBB".
bool ParseHexColor(const std::string &in, float &r, float &g, float &b)
{
	std::string hex = in;
	if (!hex.empty() && hex[0] == '#')
		hex = hex.substr(1);

	if (hex.size() != 6)
		return false;

	for (char c : hex) {
		if (!std::isxdigit(static_cast<unsigned char>(c)))
			return false;
	}

	unsigned long value = std::strtoul(hex.c_str(), nullptr, 16);
	r = static_cast<float>((value >> 16) & 0xFF) / 255.0f;
	g = static_cast<float>((value >> 8) & 0xFF) / 255.0f;
	b = static_cast<float>(value & 0xFF) / 255.0f;
	return true;
}

bool ParseLogLevel(const std::string &s, Csm::CubismFramework::Option::LogLevel &level)
{
	using Csm::CubismFramework;
	if (s == "verbose")
		level = CubismFramework::Option::LogLevel_Verbose;
	else if (s == "debug")
		level = CubismFramework::Option::LogLevel_Debug;
	else if (s == "info")
		level = CubismFramework::Option::LogLevel_Info;
	else if (s == "warning")
		level = CubismFramework::Option::LogLevel_Warning;
	else if (s == "error")
		level = CubismFramework::Option::LogLevel_Error;
	else
		return false;
	return true;
}

} // namespace

AppConfig AppConfig::LoadFromFile(const std::string &path)
{
	AppConfig cfg;

	if (!Pal::IsFileExist(path.c_str())) {
		Pal::PrintLog("[APP]config file not found at %s, using built-in defaults", path.c_str());
		return cfg;
	}

	Csm::csmSizeInt size = 0;
	Csm::csmByte *buffer = Pal::LoadFileAsBytes(path, &size);
	if (buffer == NULL) {
		Pal::PrintLog("[APP]config file could not be read at %s, using built-in defaults", path.c_str());
		return cfg;
	}
	if (size == 0) {
		Pal::PrintLog("[APP]config file at %s is empty, using built-in defaults", path.c_str());
		Pal::ReleaseBytes(buffer);
		return cfg;
	}

	Json::Value root;
	JSONCPP_STRING err;
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	bool ok = reader->parse(reinterpret_cast<char *>(buffer), reinterpret_cast<char *>(buffer) + size, &root, &err);
	Pal::ReleaseBytes(buffer);

	if (!ok || !root.isObject()) {
		Pal::PrintLog("[APP]config file at %s has invalid JSON (%s), using built-in defaults", path.c_str(),
			      err.c_str());
		return cfg;
	}

	cfg.mode = GetString(root, "mode", cfg.mode);

	const Json::Value &windowObj = GetObject(root, "window");
	cfg.window.width = GetInt(windowObj, "width", cfg.window.width);
	cfg.window.height = GetInt(windowObj, "height", cfg.window.height);
	if (windowObj.isMember("x") && windowObj["x"].isNumeric()) {
		cfg.window.hasX = true;
		cfg.window.x = windowObj["x"].asInt();
	}
	if (windowObj.isMember("y") && windowObj["y"].isNumeric()) {
		cfg.window.hasY = true;
		cfg.window.y = windowObj["y"].asInt();
	}
	cfg.window.borderless = GetBool(windowObj, "borderless", cfg.window.borderless);
	cfg.window.alwaysOnTop = GetBool(windowObj, "always_on_top", cfg.window.alwaysOnTop);
	cfg.window.preventMinimize = GetBool(windowObj, "prevent_minimize", cfg.window.preventMinimize);
	cfg.window.title = GetString(windowObj, "title", cfg.window.title);

	const Json::Value &bgObj = GetObject(root, "background");
	std::string colorStr = GetString(bgObj, "color", "");
	if (!colorStr.empty()) {
		float r, g, b;
		if (ParseHexColor(colorStr, r, g, b)) {
			cfg.background.r = r;
			cfg.background.g = g;
			cfg.background.b = b;
		} else {
			Pal::PrintLog("[APP]config background.color '%s' is invalid, using default green",
				      colorStr.c_str());
		}
	}
	cfg.background.transparent = GetBool(bgObj, "transparent", cfg.background.transparent);

	const Json::Value &catObj = GetObject(root, "cat");
	cfg.cat.live2d = GetBool(catObj, "live2d", cfg.cat.live2d);
	cfg.cat.mask = GetBool(catObj, "mask", cfg.cat.mask);
	cfg.cat.scale = GetDouble(catObj, "scale", cfg.cat.scale);
	cfg.cat.x = GetDouble(catObj, "x", cfg.cat.x);
	cfg.cat.y = GetDouble(catObj, "y", cfg.cat.y);
	cfg.cat.speed = GetDouble(catObj, "speed", cfg.cat.speed);
	cfg.cat.randomMotion = GetBool(catObj, "random_motion", cfg.cat.randomMotion);
	cfg.cat.breath = GetBool(catObj, "breath", cfg.cat.breath);
	cfg.cat.eyeBlink = GetBool(catObj, "eyeblink", cfg.cat.eyeBlink);
	cfg.cat.track = GetBool(catObj, "track", cfg.cat.track);

	const Json::Value &inputObj = GetObject(root, "input");
	cfg.input.backend = GetString(inputObj, "backend", cfg.input.backend);
	cfg.input.relativeMouse = GetBool(inputObj, "relative_mouse", cfg.input.relativeMouse);
	cfg.input.mouseHorizontalFlip = GetBool(inputObj, "mouse_horizontal_flip", cfg.input.mouseHorizontalFlip);
	cfg.input.mouseVerticalFlip = GetBool(inputObj, "mouse_vertical_flip", cfg.input.mouseVerticalFlip);

	const Json::Value &renderObj = GetObject(root, "render");
	cfg.render.vsync = GetBool(renderObj, "vsync", cfg.render.vsync);
	cfg.render.maxFps = GetInt(renderObj, "max_fps", cfg.render.maxFps);

	const Json::Value &logObj = GetObject(root, "log");
	std::string levelStr = GetString(logObj, "level", "info");
	Csm::CubismFramework::Option::LogLevel level;
	if (ParseLogLevel(levelStr, level)) {
		cfg.logLevel = level;
	} else {
		Pal::PrintLog("[APP]config log.level '%s' is invalid, using 'info'", levelStr.c_str());
	}

	return cfg;
}

CliOptions ParseCliOptions(const std::vector<std::string> &args, const std::string &exeDir)
{
	CliOptions opts;
	opts.configPath = exeDir + "/config.json";

	for (size_t i = 0; i < args.size(); i++) {
		const std::string &arg = args[i];
		if (arg == "--config" && i + 1 < args.size()) {
			opts.configPath = args[++i];
		} else if (arg == "--mode" && i + 1 < args.size()) {
			opts.modeOverride = args[++i];
			opts.hasModeOverride = true;
		} else if (arg == "--console") {
			opts.console = true;
		} else if (arg == "--verbose") {
			opts.verbose = true;
		} else if (arg == "--help" || arg == "-h" || arg == "/?") {
			opts.help = true;
		}
	}

	return opts;
}

void PrintUsage()
{
	std::printf(
		"Bongobs Cat - standalone Live2D bongo-cat overlay\n"
		"\n"
		"Usage: BongobsCat [options]\n"
		"\n"
		"Options:\n"
		"  --config <path>   Path to config.json (default: <exe dir>/config.json)\n"
		"  --mode <name>     Overrides config.json's \"mode\"\n"
		"  --console         Open a console window and mirror the log there (Windows)\n"
		"  --verbose         Force Cubism log level to verbose\n"
		"  --help            Show this help and exit\n");
}
