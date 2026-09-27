/**
  Created by Weng Y on 2020/05/25.
  Copyright © 2020 Weng Y. Under GNU General Public License v2.0.
*/
#include "VtuberFrameWork.hpp"
#include"VtuberDelegate.hpp"

namespace {
    // Standalone app only ever drives a single instance (id 0), so this is
    // just a one-shot init guard now instead of a ref count.
    static bool isLoad = false;

    static bool isInit = false;
    }

void VtuberFrameWork::InitVtuber(int id)
{
    isLoad = VtuberDelegate::GetInstance()->LoadResource(id);
}

bool VtuberFrameWork::InitializeGraphics(int id, GLFWwindow *window)
{
    if (!isLoad)
	    return false;

    if (!isInit)
	    isInit = VtuberDelegate::GetInstance()->Initialize(id, window);

    return isInit;
}

void VtuberFrameWork::RenderFrame(int id)
{
    if (isLoad && isInit)
	    VtuberDelegate::GetInstance()->RenderFrame(id);
}

void VtuberFrameWork::UinitVtuber(int id)
{
    if (isInit) {
	    VtuberDelegate::GetInstance()->ReleaseResource(id);
	    VtuberDelegate::GetInstance()->Release();
	    VtuberDelegate::ReleaseInstance();
	    isInit = false;
    }
    isLoad = false;
}

void VtuberFrameWork::SetWindow(int id, double x, double y, int width, int height, double scale)
{
    VtuberDelegate::GetInstance()->UpdataViewWindow(x, y, width, height, scale, id);
}

void VtuberFrameWork::UpData(int id,double _x, double _y, int width, int height,
			     double sc,double _delayTime, bool _randomMotion,bool _break,
			     bool _eyeBlink,const char *modelPath,bool _tarck,
			     const char *mode, bool _live2d,
			     bool relative_mouse, bool _isMouseHorizontalFlip,
			     bool _isMouseVerticalFlip,bool _isUsemask)
{

	VtuberDelegate::GetInstance()->UpdataViewWindow(_x,_y,width, height,sc, id);
	VtuberDelegate::GetInstance()->updataModelSetting(
		_randomMotion, _delayTime, _break, _eyeBlink, _tarck, _isMouseHorizontalFlip, _isMouseVerticalFlip,id);
	VtuberDelegate::GetInstance()->ChangeModel(modelPath,id);
	VtuberDelegate::GetInstance()->ChangeMode(mode, _live2d, _isUsemask,id);
	VtuberDelegate::GetInstance()->ChangeMouseMovement(relative_mouse);
}

const char** VtuberFrameWork::GetModeDefine(int &_size) {
	return VtuberDelegate::GetInstance()->GetModeDefine(_size);
}

int VtuberFrameWork::GetWidth(int id)
{
	return VtuberDelegate::GetInstance()->getBufferWidth(id);
}

int VtuberFrameWork::GetHeight(int id)
{
	return VtuberDelegate::GetInstance()->getBufferHeight(id);
}
