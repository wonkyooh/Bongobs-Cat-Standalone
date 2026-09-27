# Bongobs Cat Standalone

Bongobs Cat Standalone is a standalone Windows executable port of the Bongobs Cat OBS plugin. It renders a Live2D bongo-cat that reacts to your keyboard, and optionally your mouse, while you play a game. It's built for streamers who use OBS Studio as well as those on SOOP 프릭샷, which cannot load OBS plugins at all. Rather than drawing as an OBS-native overlay, the cat renders into its own window that you capture with a Window Capture source and key out with a Chroma Key filter — a method that works the same way in OBS and 프릭샷.

## Screenshot

<!-- TODO: add a screenshot once available
![Bongobs Cat overlay in OBS](docs/screenshot.png)
-->

## Download

Download `BongobsCat-windows-x64.zip` from the [Releases](../../releases/latest) page. The zip contains:

- `BongobsCat.exe`
- `config.json`
- `Resources/Bango Cat/...`
- `README.md`
- `LICENSE`

It's portable — there's no installer, and no Visual C++ Redistributable is required, since the executable is statically linked against the CRT. Requires Windows 10 version 1903 or later, or Windows 11, 64-bit.

## Quick start

1. Unzip the archive anywhere.
2. Run `BongobsCat.exe`.

A `BongobsCat.log` file is written next to the executable. The window itself is a fixed-size (1280x768 by default), opaque window filled with a solid chroma-key color (`#00FF00` by default). It isn't meant to be watched directly — it exists to be captured by a Window Capture source with a Chroma Key filter applied on top.

Borderless mode is available: there's no title bar in that mode, so move the window by dragging its body, and close it with Alt+F4. Always-on-top is also available. Both are off by default; see [Configuration](#configuration).

## Add to your stream

### OBS Studio

1. In Sources, click **+** and add **Window Capture**.
2. Select `[BongobsCat.exe]: Bongobs Cat` as the window (the title shown is whatever `window.title` is set to).
3. Set **Capture Method** to **Windows 10 (1903 and up)** (Windows Graphics Capture). The older BitBlt-based method can render as solid black for OpenGL windows like this one.
4. Tick **Client Area** to exclude the title bar and border, or leave it unticked and set `window.borderless: true` in `config.json` instead.
5. Right-click the source, choose **Filters**, click **+** under Effect Filters, and add **Chroma Key**. Set **Key Color Type** to Green (or Custom Color, matching `background.color` if you changed it), then adjust Similarity, Smoothness, and Spill to taste.

Don't minimize the Bongobs Cat window while it's being captured.

### 프릭샷 (SOOP)

1. Add a 창 캡처 (window capture) source and target the Bongobs Cat window.
2. Apply a 크로마키 (chroma key) filter set to the background color.
3. If the captured image shows solid black, look for a Windows Graphics Capture or "Windows 10"-style capture option — the exact wording differs from OBS and hasn't been verified for this document, but the concept is the same as in the OBS section: Windows Graphics Capture rather than the older BitBlt-style capture.
4. Turn on `window.borderless: true` so no title bar ends up in the captured frame.

Don't minimize the window while capturing. If the labels above don't match your version of 프릭샷, the underlying idea is what matters: capture the window, key out the background color, prefer Windows Graphics Capture over BitBlt-style capture, and don't minimize.

### Game tips

- Prefer Borderless Windowed over Exclusive Fullscreen if the cat capture freezes or stops updating.
- The Bongobs Cat window doesn't need to stay visible on top of your game. Window capture reads a window's contents regardless of its stacking order or on-screen visibility, as long as it isn't minimized, so it's fine to let the game cover it completely.

## Configuration

Settings live in `config.json` next to `BongobsCat.exe` and are loaded on startup. The default file looks like this:

```json
{
  "mode": "standard",
  "window": { "width": 1280, "height": 768, "x": null, "y": null, "borderless": false, "always_on_top": false, "prevent_minimize": true, "title": "Bongobs Cat" },
  "background": { "color": "#00FF00", "transparent": false },
  "cat": { "live2d": true, "mask": false, "scale": 1.83, "x": 0.0, "y": 0.02, "speed": 1.0, "random_motion": true, "breath": true, "eyeblink": true, "track": true },
  "input": { "backend": "hook", "relative_mouse": false, "mouse_horizontal_flip": true, "mouse_vertical_flip": true },
  "render": { "vsync": true, "max_fps": 60 },
  "log": { "level": "info" }
}
```

| Key | Type | Default | Description |
| --- | --- | --- | --- |
| `mode` | string | `"standard"` | Which model and animation set to load. Must be one of the names listed in `Resources/Bango Cat/mode/config.json` (`standard`, `keyboard`, `feixue`, `bilibiliduo`, `mania`). Can be overridden for a single launch with `--mode`. |
| `window.width` | integer | `1280` | Window width, in pixels. The scene is always laid out on a 1280x768 canvas and scaled uniformly to fit the window, so any size works. |
| `window.height` | integer | `768` | Window height, in pixels. If the aspect ratio differs from 1280x768, the unused strips are filled with the background color (and keyed out with it). |
| `window.x` | integer or null | `null` | Window horizontal position. When `null`, Windows chooses where to place the window. |
| `window.y` | integer or null | `null` | Window vertical position. When `null`, Windows chooses where to place the window. |
| `window.borderless` | boolean | `false` | Removes the title bar and border. With this on, move the window by dragging its body, and close it with Alt+F4. |
| `window.always_on_top` | boolean | `false` | Keeps the window above other windows. |
| `window.prevent_minimize` | boolean | `true` | Blocks the window from being minimized. On by default because Windows Graphics Capture cannot capture a minimized window. |
| `window.title` | string | `"Bongobs Cat"` | The window's title text. This is what you look for when picking a capture source in OBS or 프릭샷. |
| `background.color` | string | `"#00FF00"` | The solid chroma-key background color, as a hex code. |
| `background.transparent` | boolean | `false` | Experimental true-transparency mode used instead of a solid background color. Some capture software renders this as solid black, so the opaque-background-plus-chroma-key approach above is the one to rely on. |
| `cat.live2d` | boolean | `true` | Renders the Live2D model. |
| `cat.mask` | boolean | `false` | Enables the face-overlay images from `Resources/Bango Cat/face`, toggled at runtime with F1-F4. |
| `cat.scale` | float | `1.83` | Model scale. |
| `cat.x` | float | `0.0` | Model horizontal position offset. |
| `cat.y` | float | `0.02` | Model vertical position offset. |
| `cat.speed` | float | `1.0` | Animation speed multiplier. |
| `cat.random_motion` | boolean | `true` | Plays idle random motions. |
| `cat.breath` | boolean | `true` | Plays the breathing animation. |
| `cat.eyeblink` | boolean | `true` | Plays the eye-blink animation. |
| `cat.track` | boolean | `true` | Makes the cat's head and eyes follow the mouse cursor. |
| `input.backend` | string | `"hook"` | Which input-capture mechanism to use: `"hook"` or `"rawinput"`. See [Input capture & games](#input-capture--games) for details. |
| `input.relative_mouse` | boolean | `false` | When `true`, drives the paw from raw relative mouse deltas instead of the absolute cursor position. Useful for games that lock or hide the cursor. |
| `input.mouse_horizontal_flip` | boolean | `true` | Flips the horizontal axis used for mouse tracking. |
| `input.mouse_vertical_flip` | boolean | `true` | Flips the vertical axis used for mouse tracking. |
| `render.vsync` | boolean | `true` | Enables vertical sync. |
| `render.max_fps` | integer | `60` | Caps the frame rate. |
| `log.level` | string | `"info"` | Log verbosity written to `BongobsCat.log`. `--verbose` on the command line forces verbose output regardless of this setting. |

### Command-line flags

| Flag | Description |
| --- | --- |
| `--config <path>` | Use a config file at a custom path instead of the default next to the executable. |
| `--mode <name>` | Override `mode` for this launch only. |
| `--console` | Attach a console window so you can see log output live. |
| `--verbose` | Force verbose logging. |
| `--help` | Print usage and exit. |

## Input capture & games

Bongobs Cat captures keyboard and mouse input using one of two backends, selected with `input.backend`.

- **`hook` (default)** — Windows low-level keyboard and mouse hooks. This works system-wide, including while a game has focus, and is the same underlying mechanism OBS hotkeys and AutoHotkey use. It does not inject a DLL into any other process, so anti-cheat systems generally tolerate it.
- **`rawinput`** — the Raw Input API, registered with `RIDEV_INPUTSINK`. Also focus-independent. Try this backend if an overlay or a game's anti-cheat setup is starving the low-level hooks, or if keys intermittently stop registering with the default backend.

**Elevation and UIPI**: Windows User Interface Privilege Isolation (UIPI) blocks a normal-privilege process from observing input delivered to an elevated ("Run as administrator") window. If keys only fail to register in one specific game, that game is probably running elevated — run `BongobsCat.exe` as administrator too.

**Privacy**: neither backend records, logs, or transmits keystrokes or mouse movement anywhere. They're only used to flip which on-screen paw or key state the cat displays.

## Troubleshooting

- **Keys aren't detected in a specific game** -> that game is likely running elevated; run `BongobsCat.exe` as administrator. Also try setting `input.backend` to `"rawinput"`. Check `BongobsCat.log` (or relaunch with `--console`) to confirm which input backend started and whether it reported an error — individual keystrokes are never logged.
- **Capture shows solid black** -> switch the capture method to Windows Graphics Capture / "Windows 10 (1903 and up)" in OBS (or the equivalent option in 프릭샷). Also make sure `background.transparent` is `false`.
- **The window isn't listed in the capture-source picker, or the capture is frozen** -> make sure `BongobsCat.exe` is running and not minimized (`window.prevent_minimize` defaults to on, but double-check). Alt-Tab to confirm the window exists.
- **"Resources folder not found" or a similar startup error** -> keep the `Resources` folder next to `BongobsCat.exe`; don't unzip only the executable.
- **Windows SmartScreen warns "Windows protected your PC"** -> expected for an unsigned executable. Click "More info", then "Run anyway" — or build from source yourself if you'd rather not.

## Building from source

### Windows

Requires Visual Studio 2022 (with the "Desktop development with C++" workload) and CMake 3.16 or later.

```
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
cmake --install build --config Release --prefix dist
```

### macOS (development only)

The macOS build is x86_64 only, because the bundled Cubism Core static library is x86_64-only. Input handling also falls back to just the focused window (there's no system-wide hook) since this target is meant for development and iteration, not distribution.

```
cmake -S . -B build-mac -G Ninja -DCMAKE_OSX_ARCHITECTURES=x86_64
cmake --build build-mac
arch -x86_64 ./build-mac/BongobsCat
```

## Differences from the OBS plugin

- Same renderer, assets, and Live2D model as upstream.
- OBS glue code removed.
- Renders directly to its own window instead of `glReadPixels`-ing into an OBS texture.
- Settings moved from the OBS Properties UI to `config.json`.
- Input capture rewritten with two selectable backends, plus bug fixes (mouse button state tracking).
- Support for non-ASCII paths.

## Credits & License

Bongobs Cat Standalone is licensed under the GPL-2.0 license — see [`LICENSE`](LICENSE).

This project is a derivative of [a1928370421/Bongobs-Cat-Plugin](https://github.com/a1928370421/Bongobs-Cat-Plugin) by Weng Y, credited accordingly.

It bundles the following third-party components:

- **Live2D Cubism SDK** — the Core is proprietary, under the [Live2D Proprietary Software License](https://www.live2d.com/eula/live2d-proprietary-software-license-agreement_en.html); the Framework is under the [Live2D Open Software License](https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html). Redistribution follows those license terms, the same way the upstream plugin does.
- **GLFW** — zlib license.
- **GLEW** — BSD/MIT.
- **stb_image** — public domain / MIT (dual).
- **JsonCpp** — MIT / public domain (dual).

## 한국어 빠른 시작

Bongobs Cat Standalone은 OBS 플러그인이었던 Bongobs Cat을 독립 실행 파일로 옮긴 프로그램입니다. 키보드(그리고 선택적으로 마우스) 입력에 반응하는 Live2D 봉고캣을 화면에 띄워주는데, OBS Studio는 물론이고 플러그인을 설치할 수 없는 SOOP 프릭샷에서도 쓸 수 있게 만들어졌습니다.

- **다운로드**: [Releases](../../releases/latest) 페이지에서 `BongobsCat-windows-x64.zip`을 받으세요.
- **압축 해제**: 아무 폴더에나 풀면 됩니다. 설치 프로그램이 아니라 실행 파일 하나라서, `Resources` 폴더를 꼭 `BongobsCat.exe`와 같은 위치에 둬야 합니다.
- **실행**: `BongobsCat.exe`를 실행하세요. 초록색(`#00FF00`) 배경으로 꽉 찬 창이 뜨는데, 이건 직접 보라고 띄우는 창이 아니라 크로마키로 배경을 지우고 캡처하기 위한 창입니다.
- **OBS**: 소스에서 창 캡처를 추가하고 `[BongobsCat.exe]: Bongobs Cat` 창을 선택한 다음, 캡처 방법을 "Windows 10 (1903 이상)"으로 설정하세요 (예전 방식인 BitBlt로 두면 화면이 검게 나올 수 있습니다). 그 다음 필터에서 크로마키를 추가하고 배경색에 맞게 조정하면 됩니다.
- **프릭샷**: 마찬가지로 창 캡처 소스를 추가하고 크로마키 필터를 적용하면 되는데, 프릭샷의 정확한 메뉴 이름은 버전마다 다를 수 있고 저희도 따로 확인해보지는 못했습니다. "창 캡처 + 크로마키"라는 개념만 기억해두고 비슷한 옵션을 찾아보시면 됩니다.
- **게임 중 키 입력이 안 될 때**: 대부분 그 게임이 관리자 권한으로 실행되고 있어서 그렇습니다. `BongobsCat.exe`도 관리자 권한으로 실행해 보세요.
