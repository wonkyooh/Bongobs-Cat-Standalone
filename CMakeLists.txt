cmake_minimum_required(VERSION 3.16)

# Vendored subprojects declare old cmake_minimum_required() floors (GLFW 3.1,
# GLEW 2.8.12). CMake 4.x refuses anything below 3.5 outright; this variable
# tells it to treat those as 3.5 instead, so nobody has to pass
# -DCMAKE_POLICY_VERSION_MINIMUM=3.5 by hand. Must be set before the
# add_subdirectory() calls below.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

project(BongobsCat LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Static CRT everywhere (no VC++ redist needed on the target machine). Must be
# set before any add_subdirectory()/add_library()/add_executable() call that
# should honor it -- GLEW, GLFW, Framework and BongobsCat all need to agree,
# or MSVC's linker fails with LNK2038 (mismatched _ITERATOR_DEBUG_LEVEL/RuntimeLibrary).
cmake_policy(SET CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(FATAL_ERROR "[BongobsCat] Only 64-bit builds are supported.")
endif()

set(BONGOBSCAT_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(CORE_PATH ${BONGOBSCAT_ROOT}/CubismSdk/Core)
set(FRAMEWORK_PATH ${BONGOBSCAT_ROOT}/CubismSdk/Framework)
set(THIRD_PARTY_PATH ${BONGOBSCAT_ROOT}/thirdParty)
set(STB_PATH ${THIRD_PARTY_PATH}/stb)
set(GLEW_PATH ${THIRD_PARTY_PATH}/glew)
set(GLFW_PATH ${THIRD_PARTY_PATH}/glfw)
set(RES_PATH ${BONGOBSCAT_ROOT}/Resources)

# ---------------------------------------------------------------------------
# Cubism Core (prebuilt static library, not built from source)
# ---------------------------------------------------------------------------
add_library(Live2DCubismCore STATIC IMPORTED)

if(WIN32)
  if(MSVC_VERSION GREATER_EQUAL 1920)
    set(CUBISM_MSVC_TOOLSET 142) # VS2019/2022/2026; the 14.x toolsets are ABI-compatible and no newer libs are shipped
  elseif(MSVC_VERSION GREATER_EQUAL 1910)
    set(CUBISM_MSVC_TOOLSET 141) # VS2017
  else()
    message(FATAL_ERROR "[BongobsCat] Unsupported MSVC toolset: need VS2017 (1910) or newer.")
  endif()

  set(CUBISM_CORE_LIB_DIR ${CORE_PATH}/lib/windows/x86_64/${CUBISM_MSVC_TOOLSET})
  set_target_properties(Live2DCubismCore PROPERTIES
    IMPORTED_LOCATION_DEBUG          ${CUBISM_CORE_LIB_DIR}/Live2DCubismCore_MTd.lib
    IMPORTED_LOCATION_RELEASE        ${CUBISM_CORE_LIB_DIR}/Live2DCubismCore_MT.lib
    IMPORTED_LOCATION_RELWITHDEBINFO ${CUBISM_CORE_LIB_DIR}/Live2DCubismCore_MT.lib
    IMPORTED_LOCATION_MINSIZEREL     ${CUBISM_CORE_LIB_DIR}/Live2DCubismCore_MT.lib
    INTERFACE_INCLUDE_DIRECTORIES    ${CORE_PATH}/include
  )
elseif(APPLE)
  if(NOT CMAKE_OSX_ARCHITECTURES STREQUAL "x86_64")
    message(FATAL_ERROR
      "[BongobsCat] The vendored macOS Cubism Core static library is x86_64-only. "
      "Configure with -DCMAKE_OSX_ARCHITECTURES=x86_64 and run the result under "
      "Rosetta on Apple Silicon.")
  endif()
  set_target_properties(Live2DCubismCore PROPERTIES
    IMPORTED_LOCATION                ${CORE_PATH}/lib/macos/libLive2DCubismCore.a
    INTERFACE_INCLUDE_DIRECTORIES    ${CORE_PATH}/include
  )
else()
  message(FATAL_ERROR "[BongobsCat] Unsupported platform: only Windows x64 and macOS x86_64 are supported.")
endif()

# ---------------------------------------------------------------------------
# GLEW / GLFW (built from the vendored sources)
# ---------------------------------------------------------------------------
set(BUILD_UTILS OFF CACHE BOOL "" FORCE)
add_subdirectory(${GLEW_PATH}/build/cmake ${CMAKE_CURRENT_BINARY_DIR}/glew)

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
# GLFW force-rewrites its own flags to /MD when this is ON; keep it OFF so
# GLFW/GLEW/Framework/BongobsCat all end up on the same static CRT.
set(USE_MSVC_RUNTIME_LIBRARY_DLL OFF CACHE BOOL "" FORCE)
add_subdirectory(${GLFW_PATH} ${CMAKE_CURRENT_BINARY_DIR}/glfw)

# ---------------------------------------------------------------------------
# Cubism Native Framework (static lib, built from the vendored sources)
# ---------------------------------------------------------------------------
set(FRAMEWORK_SOURCE OpenGL)
add_subdirectory(${FRAMEWORK_PATH} ${CMAKE_CURRENT_BINARY_DIR}/Framework)

if(WIN32)
  target_compile_definitions(Framework PUBLIC CSM_TARGET_WIN_GL GLEW_STATIC)
elseif(APPLE)
  target_compile_definitions(Framework PUBLIC CSM_TARGET_MAC_GL GLEW_STATIC)
endif()
target_include_directories(Framework PUBLIC ${GLEW_PATH}/include)
target_link_libraries(Framework PUBLIC Live2DCubismCore glew_s)

find_package(OpenGL REQUIRED)

# ---------------------------------------------------------------------------
# BongobsCat executable
# ---------------------------------------------------------------------------
set(BONGOBSCAT_SOURCES
  main.cpp
  AppConfig.cpp
  Platform.cpp
  VtuberFrameWork.cpp
  VtuberDelegate.cpp
  View.cpp
  Live2DManager.cpp
  Model.cpp
  Pal.cpp
  LAppTextureManager.cpp
  LAppAllocator.cpp
  Sprite.cpp
  EventManager.cpp
  Define.cpp
  jsoncpp.cpp
  InfoReader.cpp
  input/Input.cpp
  input/KeyMap.cpp
  input/InputGlfw.cpp
)

if(WIN32)
  list(APPEND BONGOBSCAT_SOURCES
    input/InputWin32Hook.cpp
    input/InputWin32RawInput.cpp
    app.manifest
  )
endif()

add_executable(BongobsCat WIN32 ${BONGOBSCAT_SOURCES})

set_target_properties(BongobsCat PROPERTIES OUTPUT_NAME BongobsCat)

target_include_directories(BongobsCat PRIVATE ${BONGOBSCAT_ROOT} ${STB_PATH})

target_link_libraries(BongobsCat PRIVATE Framework glfw glew_s ${OPENGL_LIBRARIES})

if(MSVC)
  target_compile_definitions(BongobsCat PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX WIN32_LEAN_AND_MEAN UNICODE _UNICODE)
  target_compile_options(BongobsCat PRIVATE /utf-8)
  # WIN32 subsystem (no console flash on launch) but a plain int main() entry point.
  target_link_options(BongobsCat PRIVATE /ENTRY:mainCRTStartup)
  # CommandLineToArgvW (Platform::GetUtf8Args) lives in shell32.
  target_link_libraries(BongobsCat PRIVATE shell32)
elseif(APPLE)
  target_compile_definitions(BongobsCat PRIVATE GL_SILENCE_DEPRECATION)
  target_link_libraries(BongobsCat PRIVATE "-framework OpenGL")
endif()

# ---------------------------------------------------------------------------
# Ship Resources/ and config.json next to the exe, both for running straight
# out of the build directory and for `cmake --install`.
# ---------------------------------------------------------------------------
add_custom_command(TARGET BongobsCat POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          "${RES_PATH}" "$<TARGET_FILE_DIR:BongobsCat>/Resources"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${BONGOBSCAT_ROOT}/config.json" "$<TARGET_FILE_DIR:BongobsCat>/config.json"
  COMMENT "[BongobsCat] copying Resources/ and config.json next to the executable"
)

install(TARGETS BongobsCat DESTINATION .)
install(DIRECTORY ${RES_PATH} DESTINATION .)
install(FILES config.json README.md LICENSE DESTINATION .)
