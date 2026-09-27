/**
  Created by Weng Y on 2020/05/25.
  Copyright © 2020 Weng Y. Under GNU General Public License v2.0.
*/
#pragma once

struct GLFWwindow;

class VtuberFrameWork {
public:
	static void InitVtuber(int id);

	// Finishes bringing up OpenGL/Cubism for `id` on `window`'s current
	// context (glewInit + Cubism startup + sprite/model load). Call after
	// SetWindow() so View::Initialize() sees a non-zero buffer size, and
	// after InitVtuber(). Returns false if resources failed to load/init.
	static bool InitializeGraphics(int id, GLFWwindow *window);

	// Renders the current frame into the currently bound framebuffer. Does
	// NOT clear -- the caller's render loop owns glClear/glViewport/glfwSwapBuffers.
	static void RenderFrame(int id);

	static void UinitVtuber(int id);

	// Sets the render target placement/size/scale used by View's layout math.
	// Safe to call again later (e.g. on window resize) with the same x/y/scale
	// and a new width/height.
	static void SetWindow(int id, double x, double y, int width, int height, double scale);

	static void UpData(int id,double _x, double _y, int width, int height,
			   double scale,double delayTime, bool randomMotion,
			   bool breath,bool eyeBlink, const char *ModelName,
			   bool track, const char *modelPath, bool _live2d,
			   bool relative_mouse, bool _isMouseHorizontalFlip,
			   bool _isMouseVerticalFlip, bool _isUsemask);

	static const char **GetModeDefine(int &_size);

	static int GetWidth(int id);

	static int GetHeight(int id);
};
