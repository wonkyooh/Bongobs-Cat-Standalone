#pragma once

#include <string>
#include <vector>
#include <CubismFramework.hpp>

// config.json schema (every key optional; unknown keys ignored; missing
// file -> defaults + a warning in the log). See AppConfig.cpp for parsing
// and validation details.
struct AppConfig {
	struct WindowConfig {
		int width = 1280;
		int height = 768;
		bool hasX = false;
		int x = 0;
		bool hasY = false;
		int y = 0;
		bool borderless = false;
		bool alwaysOnTop = false;
		bool preventMinimize = true;
		std::string title = "Bongobs Cat";
	};

	struct BackgroundConfig {
		float r = 0.0f;
		float g = 1.0f;
		float b = 0.0f;
		bool transparent = false;
	};

	struct CatConfig {
		bool live2d = true;
		bool mask = false;
		double scale = 1.83;
		double x = 0.0;
		double y = 0.02;
		double speed = 1.0;
		bool randomMotion = true;
		bool breath = true;
		bool eyeBlink = true;
		bool track = true;
	};

	struct InputConfig {
		std::string backend = "hook";
		bool relativeMouse = false;
		bool mouseHorizontalFlip = true;
		bool mouseVerticalFlip = true;
		// Relaunch elevated (UAC) on startup if not already elevated. Works
		// around Windows UIPI blocking our hooks/raw input from seeing
		// keystrokes delivered to an elevated (anti-cheat) game window.
		bool runAsAdmin = false;
	};

	struct RenderConfig {
		bool vsync = true;
		int maxFps = 60;
	};

	std::string mode = "standard";
	WindowConfig window;
	BackgroundConfig background;
	CatConfig cat;
	InputConfig input;
	RenderConfig render;
	Csm::CubismFramework::Option::LogLevel logLevel = Csm::CubismFramework::Option::LogLevel_Info;

	// Loads config.json from `path`. On any problem (missing file, unreadable,
	// invalid JSON) logs a warning via Pal::PrintLog and returns defaults;
	// individual bad/missing keys inside an otherwise-valid file also just
	// warn-and-default rather than failing the whole load.
	static AppConfig LoadFromFile(const std::string &path);
};

// Parsed command line, already CLI-only (argv[0] excluded).
struct CliOptions {
	std::string configPath;   // resolved absolute path (may not exist yet)
	std::string modeOverride; // only meaningful if hasModeOverride
	bool hasModeOverride = false;
	bool console = false;
	bool verbose = false;
	bool help = false;
	bool elevate = false; // relaunch elevated (UAC) if not already elevated
};

// `args` excludes argv[0]. `exeDir` resolves the default --config location.
CliOptions ParseCliOptions(const std::vector<std::string> &args, const std::string &exeDir);

void PrintUsage();
