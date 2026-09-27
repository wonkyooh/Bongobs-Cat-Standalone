/**
  Bongobs Cat -- standalone executable entry point.

  Owns the window, the main loop, and config/resource-path wiring. The
  Cubism/OpenGL side lives behind VtuberFrameWork/VtuberDelegate; the global
  keyboard/mouse capture lives behind input/Input.hpp (owned by a different
  agent -- this file only calls into it).
*/
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <chrono>
#include <cstdio>
#include <exception>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "AppConfig.hpp"
#include "Define.hpp"
#include "Pal.hpp"
#include "Platform.hpp"
#include "View.hpp"
#include "VtuberDelegate.hpp"
#include "VtuberFrameWork.hpp"
#include "input/Input.hpp"

namespace {

constexpr int kCatId = 0;

// State the GLFW callbacks need, reached via glfwGetWindowUserPointer. main()
// owns the only instance; it outlives every callback invocation (all of
// which happen inside RunApp()'s glfwPollEvents() calls).
struct AppState {
	bool borderless = false;
	bool preventMinimize = true;

	bool dragging = false;
	double dragCursorStartX = 0.0;
	double dragCursorStartY = 0.0;
	int dragWindowStartX = 0;
	int dragWindowStartY = 0;
};

void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	(void)window;
	Input::OnGlfwKey(key, scancode, action, mods);
}

void MouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
	Input::OnGlfwMouseButton(button, action, mods);

	AppState *state = static_cast<AppState *>(glfwGetWindowUserPointer(window));
	if (!state || !state->borderless || button != GLFW_MOUSE_BUTTON_LEFT)
		return;

	if (action == GLFW_PRESS) {
		state->dragging = true;
		glfwGetCursorPos(window, &state->dragCursorStartX, &state->dragCursorStartY);
		glfwGetWindowPos(window, &state->dragWindowStartX, &state->dragWindowStartY);
	} else if (action == GLFW_RELEASE) {
		state->dragging = false;
	}
}

// Borderless drag-to-move: while LMB is held on a borderless window, follow
// the cursor. Windows Graphics Capture / OBS window capture only sees the
// client area, so there's no titlebar to drag by otherwise.
void CursorPosCallback(GLFWwindow *window, double x, double y)
{
	AppState *state = static_cast<AppState *>(glfwGetWindowUserPointer(window));
	if (!state || !state->dragging)
		return;

	double dx = x - state->dragCursorStartX;
	double dy = y - state->dragCursorStartY;
	glfwSetWindowPos(window, static_cast<int>(state->dragWindowStartX + dx),
			  static_cast<int>(state->dragWindowStartY + dy));
}

// Window/screen capture backends (Windows Graphics Capture in particular)
// cannot capture a minimized window, so bounce back out of iconified state
// when configured to.
void IconifyCallback(GLFWwindow *window, int iconified)
{
	AppState *state = static_cast<AppState *>(glfwGetWindowUserPointer(window));
	if (state && state->preventMinimize && iconified)
		glfwRestoreWindow(window);
}

#ifndef NDEBUG
void DrainGlErrors(const char *where)
{
	static std::set<GLenum> reported;
	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR) {
		if (reported.insert(err).second)
			Pal::PrintLog("[APP]GL error 0x%04x at %s (further occurrences of this code are suppressed)",
				      static_cast<unsigned int>(err), where);
	}
}
#endif

int RunApp(int argc, char **argv)
{
	std::vector<std::string> allArgs = Platform::GetUtf8Args(argc, argv);
	std::string exeDir = Platform::ExeDir();

	std::vector<std::string> cliArgs;
	if (allArgs.size() > 1)
		cliArgs.assign(allArgs.begin() + 1, allArgs.end());

	CliOptions cli = ParseCliOptions(cliArgs, exeDir);

	if (cli.help) {
		// Best effort: on Windows a GUI-subsystem exe isn't attached to
		// whatever console launched it, so without this the text would
		// otherwise vanish.
		Platform::AttachConsole();
		PrintUsage();
		return 0;
	}

	if (cli.console)
		Platform::AttachConsole();

	Platform::InitLog(exeDir + "/BongobsCat.log");

	AppConfig config = AppConfig::LoadFromFile(cli.configPath);
	if (cli.hasModeOverride)
		config.mode = cli.modeOverride;
	if (cli.verbose)
		config.logLevel = Csm::CubismFramework::Option::LogLevel_Verbose;

	Define::SetLogLevel(config.logLevel);

	// A title bar can't be transparent (and would get captured), so a
	// transparent background implies borderless regardless of what was
	// configured. always_on_top is independent and still honored either way.
	if (config.background.transparent && !config.window.borderless) {
		config.window.borderless = true;
		Pal::PrintLog("[APP]transparent background: forcing borderless");
	}

	Pal::PrintLog("[APP]Bongobs Cat starting; exeDir=%s configPath=%s", exeDir.c_str(), cli.configPath.c_str());

	// Elevation: some games run elevated (anti-cheat), and Windows UIPI then
	// blocks our non-elevated hooks/raw input from seeing their keystrokes.
	// If configured/requested, relaunch elevated before creating any window.
	// IsElevated() is true in the relaunched copy, so this can't loop.
	if ((config.input.runAsAdmin || cli.elevate) && !Platform::IsElevated()) {
		if (Platform::RelaunchElevated(cliArgs)) {
			Pal::PrintLog("[APP]relaunching elevated");
			return 0;
		}
		Pal::PrintLog(
			"[APP]WARNING: elevation was declined; continuing without admin -- input may not register in elevated games");
	}

	std::string resourcesRoot = exeDir + "/Resources/Bango Cat/";
	Define::SetResourcesRoot(resourcesRoot);

	std::string modeConfigPath = resourcesRoot + "mode/config.json";
	if (!Pal::IsFileExist(modeConfigPath.c_str())) {
		std::string msg = "Resources folder not found next to BongobsCat.exe\n(" + resourcesRoot + ")";
		Pal::PrintLog("[APP]FATAL: %s", msg.c_str());
		Platform::ShowErrorMessageBox("Bongobs Cat", msg);
		return 1;
	}

	if (glfwInit() == GLFW_FALSE) {
		Pal::PrintLog("[APP]FATAL: glfwInit failed");
		Platform::ShowErrorMessageBox("Bongobs Cat", "Failed to initialize GLFW.");
		return 1;
	}

	glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	glfwWindowHint(GLFW_DECORATED, config.window.borderless ? GLFW_FALSE : GLFW_TRUE);
	glfwWindowHint(GLFW_FLOATING, config.window.alwaysOnTop ? GLFW_TRUE : GLFW_FALSE);
	glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, config.background.transparent ? GLFW_TRUE : GLFW_FALSE);
	glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_FALSE);
	glfwWindowHint(GLFW_SAMPLES, 4);

	GLFWwindow *window = glfwCreateWindow(config.window.width, config.window.height, config.window.title.c_str(),
					       NULL, NULL);
	if (!window) {
		Pal::PrintLog("[APP]window creation with 4x MSAA failed, retrying with MSAA disabled");
		glfwWindowHint(GLFW_SAMPLES, 0);
		window = glfwCreateWindow(config.window.width, config.window.height, config.window.title.c_str(),
					   NULL, NULL);
	}
	if (!window) {
		Pal::PrintLog("[APP]FATAL: glfwCreateWindow failed");
		Platform::ShowErrorMessageBox("Bongobs Cat", "Failed to create the application window.");
		glfwTerminate();
		return 1;
	}

	if (config.window.hasX || config.window.hasY) {
		int x, y;
		glfwGetWindowPos(window, &x, &y);
		glfwSetWindowPos(window, config.window.hasX ? config.window.x : x,
				  config.window.hasY ? config.window.y : y);
	}

	AppState state;
	state.borderless = config.window.borderless;
	state.preventMinimize = config.window.preventMinimize;
	glfwSetWindowUserPointer(window, &state);

	glfwMakeContextCurrent(window);
	glfwSwapInterval(config.render.vsync ? 1 : 0);

	// The scene is laid out on a fixed logical canvas -- the plugin's 1280x768
	// render target. Sprites and the Live2D view matrix are all computed
	// against it, and each frame the canvas is scaled uniformly onto the real
	// framebuffer (letterboxed in the background color if the aspect differs),
	// so the picture matches the OBS plugin regardless of window size or DPI.
	// Feeding the raw framebuffer size in here instead shifts the cat off the
	// canvas on HiDPI displays: View::UpdataViewData() offsets the model by the
	// difference to RenderTargetWidth/Height.
	const int canvasWidth = Define::RenderTargetWidth;
	const int canvasHeight = Define::RenderTargetHeight;

	// Order matters: the view's layout math needs a non-zero buffer size
	// before View::Initialize() runs, so set it before touching VtuberDelegate
	// any other way (InitVtuber() below is what lazily constructs it).
	VtuberFrameWork::SetWindow(kCatId, config.cat.x, config.cat.y, canvasWidth, canvasHeight, config.cat.scale);
	VtuberFrameWork::InitVtuber(kCatId);

	{
		int modeCount = 0;
		const char **modes = VtuberFrameWork::GetModeDefine(modeCount);
		bool found = false;
		for (int i = 0; i < modeCount; i++) {
			if (config.mode == modes[i]) {
				found = true;
				break;
			}
		}
		if (!found && modeCount > 0) {
			Pal::PrintLog("[APP]mode '%s' not found in Resources, using '%s'", config.mode.c_str(),
				      modes[0]);
			config.mode = modes[0];
		}
	}

	if (!VtuberFrameWork::InitializeGraphics(kCatId, window)) {
		Pal::PrintLog("[APP]FATAL: failed to initialize graphics/Cubism");
		Platform::ShowErrorMessageBox("Bongobs Cat", "Failed to initialize the renderer.");
		VtuberFrameWork::UinitVtuber(kCatId);
		glfwDestroyWindow(window);
		glfwTerminate();
		return 1;
	}

	VtuberFrameWork::UpData(kCatId, config.cat.x, config.cat.y, canvasWidth, canvasHeight, config.cat.scale,
				config.cat.speed, config.cat.randomMotion, config.cat.breath, config.cat.eyeBlink,
				NULL, config.cat.track, config.mode.c_str(), config.cat.live2d,
				config.input.relativeMouse, config.input.mouseHorizontalFlip,
				config.input.mouseVerticalFlip, config.cat.mask);

	Input::Config inputConfig;
	inputConfig.backend = Input::ParseBackend(config.input.backend);
	EventManager *eventManager = VtuberDelegate::GetInstance()->GetView()->GetEventManager();
	if (Input::Start(inputConfig, eventManager, window)) {
		Pal::PrintLog("[APP]input backend active: %s", Input::ActiveBackendName());
	} else {
		Pal::PrintLog(
			"[APP]WARNING: input backend '%s' failed to start; the cat will render but won't react to keyboard/mouse",
			config.input.backend.c_str());
	}

	glfwSetKeyCallback(window, KeyCallback);
	glfwSetMouseButtonCallback(window, MouseButtonCallback);
	glfwSetCursorPosCallback(window, CursorPosCallback);
	glfwSetWindowIconifyCallback(window, IconifyCallback);

	using Clock = std::chrono::steady_clock;
	const bool capFps = config.render.maxFps > 0;
	const Clock::duration frameDuration =
		capFps ? std::chrono::duration_cast<Clock::duration>(
				 std::chrono::duration<double>(1.0 / config.render.maxFps))
		       : Clock::duration::zero();
	Clock::time_point nextFrame = Clock::now() + frameDuration;

	// Window-size watchdog state: true while recovering from an externally
	// forced resize, so the fix-up below and its log line fire once per
	// episode instead of every single frame until it clears.
	bool sizeRestorePending = false;

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		// An exclusive-fullscreen game can change the desktop resolution,
		// which makes Windows force-shrink this (non-resizable) window and
		// leaves the capture region stale. Put the size (and, if the user
		// pinned one, the position) back once the desktop can fit it again.
		// Borderless windows are skipped: borderless games never change the
		// desktop resolution, and those users place the window themselves
		// (dragging only ever changes position, never size, so this is
		// drag-safe for the non-borderless case too).
		if (!config.window.borderless) {
			int curWidth = 0, curHeight = 0;
			glfwGetWindowSize(window, &curWidth, &curHeight);

			if (curWidth == config.window.width && curHeight == config.window.height) {
				sizeRestorePending = false;
			} else {
				// Zero-initialized so a failed query (e.g. null monitor)
				// leaves them 0, which fails the fit test below -> no resize.
				int workX = 0, workY = 0, workWidth = 0, workHeight = 0;
				GLFWmonitor *primary = glfwGetPrimaryMonitor();
				if (primary)
					glfwGetMonitorWorkarea(primary, &workX, &workY, &workWidth, &workHeight);
				if (workWidth >= config.window.width && workHeight >= config.window.height) {
					glfwSetWindowSize(window, config.window.width, config.window.height);
					if (config.window.hasX || config.window.hasY) {
						int curX, curY;
						glfwGetWindowPos(window, &curX, &curY);
						glfwSetWindowPos(window, config.window.hasX ? config.window.x : curX,
								  config.window.hasY ? config.window.y : curY);
					}
					if (!sizeRestorePending)
						Pal::PrintLog(
							"[APP]window was resized externally (%dx%d), restoring to %dx%d",
							curWidth, curHeight, config.window.width,
							config.window.height);
					sizeRestorePending = true;
				}
				// else: the desktop is still too small to fit the configured
				// size (mid resolution-change) -- retry next frame.
			}
		}

		int fbWidth = 0, fbHeight = 0;
		glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
		if (fbWidth <= 0 || fbHeight <= 0) {
			// Minimized / zero-size framebuffer: nothing to draw into.
			glfwWaitEventsTimeout(0.1);
			continue;
		}

		// Clear the whole framebuffer (letterbox bars included) ...
		glViewport(0, 0, fbWidth, fbHeight);
		if (config.background.transparent) {
			// The OS compositor (DWM / macOS) treats the framebuffer as
			// PREMULTIPLIED alpha, so a "transparent" pixel must be all
			// zero -- clearing to (green, 0) would leave the green added
			// on top and tint the whole window green. Use (0,0,0,0).
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		} else {
			glClearColor(config.background.r, config.background.g, config.background.b, 1.0f);
		}
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// ... then draw the fixed-aspect canvas into the largest centered
		// viewport that fits.
		int vpWidth = fbWidth;
		int vpHeight = static_cast<int>(static_cast<long long>(fbWidth) * canvasHeight / canvasWidth);
		if (vpHeight > fbHeight) {
			vpHeight = fbHeight;
			vpWidth = static_cast<int>(static_cast<long long>(fbHeight) * canvasWidth / canvasHeight);
		}
		glViewport((fbWidth - vpWidth) / 2, (fbHeight - vpHeight) / 2, vpWidth, vpHeight);

		VtuberFrameWork::RenderFrame(kCatId);

#ifndef NDEBUG
		DrainGlErrors("main loop");
#endif

		glfwSwapBuffers(window);

		if (capFps) {
			std::this_thread::sleep_until(nextFrame);
			Clock::time_point now = Clock::now();
			// Resync instead of accumulating a backlog if a frame (or a
			// stall, e.g. window drag) ran long -- avoids a burst of
			// zero-wait catch-up frames.
			nextFrame = (now > nextFrame + frameDuration) ? now + frameDuration
								       : nextFrame + frameDuration;
		}
	}

	Input::Stop();
	VtuberFrameWork::UinitVtuber(kCatId);
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}

} // namespace

int main(int argc, char **argv)
{
	try {
		return RunApp(argc, argv);
	} catch (const std::exception &e) {
		Pal::PrintLog("[APP]FATAL: unhandled exception: %s", e.what());
		Platform::ShowErrorMessageBox("Bongobs Cat", std::string("Unexpected error: ") + e.what());
		return 1;
	}
}
