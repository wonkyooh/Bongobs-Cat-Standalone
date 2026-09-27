#pragma once

// Maps physical key codes (Windows virtual-key codes, or GLFW key codes) to the
// 0..60 index used by Define::KeyDefine[] / EventManager::KeyEventDown/Up.
//
// No <Windows.h> dependency here: the handful of VK_* values this needs are
// defined locally in KeyMap.cpp, so both functions can be syntax-checked and
// unit-tested on non-Windows hosts.
namespace Input {

// Returns an index into Define::KeyDefine (0..60), or -1 if vk has no mapping.
int VkToKeyIndex(unsigned vk);

// Same index space as VkToKeyIndex, for GLFW key codes (GLFW_KEY_*).
int GlfwKeyToKeyIndex(int glfwKey);

}
