# SPECIFICATION: RawDrive Engine (v1.0.0)
> Native C Dynamic Desktop & Idle Screen Showcase for Windows 11

---

## 1. Project Overview
**RawDrive** is a zero-runtime-overhead Windows 11 background renderer and cinematic idle engine written entirely in pure **C (C11 standard)**. It injects a hardware-accelerated video rendering canvas behind native desktop icons by interfacing with the Win32 `Progman`/`WorkerW` window tree. It also incorporates an idle screen trigger that simulates an atmospheric lock screen and delegates authentication to the Windows native lock screen (`LockWorkStation`) upon user input.

---

## 2. Technical Stack & Dependencies

* **Language Standard:** C11 / C99 compliant.
* **Target Platform:** Windows 10 (1809+) / Windows 11 (All builds), x64 architecture.
* **Toolchain:** MSVC (`cl.exe` via Visual Studio Build Tools) or Clang/LLVM for Windows.
* **Win32 & DirectX Libraries:**
  * `user32.lib`, `gdi32.lib`, `shell32.lib` (Windowing, inputs, and notification tray).
  * `d3d11.lib`, `dxgi.lib` (Direct3D 11 rendering pipeline and swap chains).
  * `mf.lib`, `mfplat.lib`, `mfreadwrite.lib`, `mfuuid.lib` (Hardware video decoding).
  * `wtsapi32.lib` (Session notification and power management).

### Operational Constraints & Performance Targets
| Metric | Limit / Target |
| :--- | :--- |
| **RAM Usage** | $< 35\text{ MB}$ steady-state footprint |
| **CPU Usage** | $< 1.5\%$ on an 8-thread processor during 1080p60 playback |
| **GPU Engine** | Hardware Video Decode (`DXVA2` / `D3D11VA`) + 3D Copy engine |
| **Frame Timing** | Constant 60 FPS ($16.66\text{ ms}$ interval), V-Sync locked via `DXGI` |

---

## 3. Directory Layout

```text
rawdrive/
├── CMakeLists.txt
├── build.bat
├── config.ini
├── assets/
│   └── garage_loop.mp4
└── src/
    ├── main.c
    ├── core/
    │   ├── hook_workerw.c
    │   ├── hook_workerw.h
    │   ├── idle_engine.c
    │   └── idle_engine.h
    ├── video/
    │   ├── d3d11_renderer.c
    │   ├── d3d11_renderer.h
    │   ├── mf_decoder.c
    │   └── mf_decoder.h
    ├── monitor/
    │   ├── focus_guard.c
    │   ├── focus_guard.h
    │   ├── session_guard.c
    │   └── session_guard.h
    ├── ui/
    │   ├── tray.c
    │   └── tray.h
    └── utils/
        ├── config.c
        ├── config.h
        ├── logger.c
        └── logger.h
4. Architectural Modules & Functional Specifications4.1. Module A: Win32 Desktop Injection (hook_workerw)Objective: Spawn and capture the isolated WorkerW window beneath desktop icons.Execution Logic:Retrieve desktop shell window: HWND hProgman = FindWindowW(L"Progman", NULL);.Send legacy spawn message:CDWORD_PTR result = 0;
SendMessageTimeoutW(hProgman, 0x052C, 0, 0, SMTO_NORMAL, 1000, &result);
Enumerate top-level windows using EnumWindows to detect the newly spawned WorkerW sibling that hosts the wallpaper layer behind SHELLDLL_DefView.Create a borderless host window (WS_POPUP | WS_VISIBLE) matching target screen metrics.Mount host window into the desktop hierarchy: SetParent(hRendererHwnd, hWorkerW);.4.2. Module B: Video Pipeline & Seamless Loop (d3d11_renderer & mf_decoder)Objective: Decode MP4/H.264/H.265 directly into GPU textures without CPU frame copying.Execution Logic:Initialize Media Foundation via MFStartup(MF_VERSION, MFSTARTUP_FULL).Instantiate ID3D11Device and context with D3D11_CREATE_DEVICE_VIDEO_SUPPORT.Create an IMFDXGIDeviceManager and bind it to the D3D11 device via ResetDevice.Create an IMFSourceReader configured with:MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS = TRUEMF_SOURCE_READER_D3D_MANAGER attached to the device manager.Configure IDXGISwapChain1 with DXGI_SWAP_EFFECT_FLIP_DISCARD on the host HWND.Loop Handling: Monitor playback duration. When the remaining stream time is less than $150\text{ ms}$, preload stream index $0$. When end-of-stream is reached, issue IMFSourceReader_SetCurrentPosition to frame zero and present immediately to avoid black-frame flickering.4.3. Module C: Occlusion & Power Manager (focus_guard & session_guard)Objective: Cut GPU/CPU consumption to $0\%$ when the desktop is hidden, games run, or the system sleeps.Execution Logic:Fullscreen Occlusion:Run a lightweight timer thread every $300\text{ ms}$.Query HWND hFore = GetForegroundWindow();.Inspect coordinates with GetWindowRect(hFore, &rect).If the foreground window dimensions match or exceed the monitor resolution and the class name is not Progman/WorkerW, pause the decoding pump.Session / Power Events:Register window with WTSRegisterSessionNotification(hHost, NOTIFY_FOR_THIS_SESSION).Intercept WM_WTSSESSION_CHANGE:On WTS_SESSION_LOCK: Pause rendering and free swap buffers.On WTS_SESSION_UNLOCK: Reallocate buffers and resume stream.Intercept WM_POWERBROADCAST: Pause on PBT_APMSUSPEND, restart on PBT_APMRESUMEAUTOMATIC.4.4. Module D: Idle Display & Mock Lock Screen (idle_engine)Objective: Present a cinematic full-screen car showcase when idle, routing instantly to Windows Lock on input.Execution Logic:Query user activity using LASTINPUTINFO plii; plii.cbSize = sizeof(LASTINPUTINFO); GetLastInputInfo(&plii);.When (GetTickCount() - plii.dwTime) >= (config.idle_sec * 1000):Create an unadorned, topmost overlay window: CreateWindowExW(WS_EX_TOPMOST, ..., WS_POPUP, ...).Reroute the Direct3D presentation target to this overlay.Monitor input via low-level hooks or high-frequency polling (GetAsyncKeyState / GetCursorPos).On input detection:Immediately destroy/hide the overlay window.Call native Win32 API: LockWorkStation().Swap the render context back to the desktop WorkerW host.4.5. Module E: Configuration & System Tray (tray & config)Objective: Background operation without a persistent console window.Execution Logic:Parse config.ini at boot for paths and engine tuning:Ini, TOML[Engine]
VideoPath=assets\garage_loop.mp4
TargetFPS=60
IdleTimeoutSeconds=300
PauseOnBattery=1
Register a taskbar notification icon using Shell_NotifyIconW with NIM_ADD.Context menu triggers: Pause/Resume, Switch Video, Trigger Showcase Now, Quit.5. Development Roadmap for Antigravity AgentsPhase 1: Core Win32 Scaffolding[ ] Implement src/core/hook_workerw.c to handle the Progman message pump and locate WorkerW.[ ] Verify that a simple GDI solid-color fill window stays pinned behind desktop icons and retains right-click functionality.Phase 2: Native D3D11 / Media Foundation Pipeline[ ] Implement src/video/d3d11_renderer.c and src/video/mf_decoder.c.[ ] Implement COM interfaces in pure C using virtual function tables (lpVtbl->...).[ ] Implement video texture streaming using IMFSourceReader directly into a Direct3D 11 texture.[ ] Add the zero-latency loop controller to prevent tearing/black flashes at stream end.Phase 3: Energy & Focus Guards[ ] Implement src/monitor/focus_guard.c to pause playback when non-desktop applications are maximized or focused.[ ] Implement src/monitor/session_guard.c to catch Windows lock/sleep messages.Phase 4: Idle Engine & Windows Lock Handover[ ] Implement src/core/idle_engine.c.[ ] Verify seamless transition from the full-screen idle car showcase to LockWorkStation().Phase 5: Tray UI, Configuration, & Packaging[ ] Implement INI parser in src/utils/config.c.[ ] Implement system tray interface in src/ui/tray.c.[ ] Validate release binary compilation using build.bat.6. Build & Compilation Script (build.bat)Fragmento de código@echo off
setlocal

:: Locate MSVC Environment
if not defined DevEnvDir (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
)

set CFLAGS=/nologo /O2 /W4 /std:c11 /DUNICODE /D_UNICODE /Isrc
set LIBS=user32.lib gdi32.lib shell32.lib d3d11.lib dxgi.lib mf.lib mfplat.lib mfuuid.lib mfreadwrite.lib wtsapi32.lib

if not exist bin mkdir bin

cl.exe %CFLAGS% ^
    src\main.c ^
    src\core\hook_workerw.c ^
    src\core\idle_engine.c ^
    src\video\d3d11_renderer.c ^
    src\video\mf_decoder.c ^
    src\monitor\focus_guard.c ^
    src\monitor\session_guard.c ^
    src\ui\tray.c ^
    src\utils\config.c ^
    src\utils\logger.c ^
    /link %LIBS% /SUBSYSTEM:WINDOWS /OUT:bin\RawDrive.exe

if %ERRORLEVEL% equ 0 (
    echo [OK] Build successful: bin\RawDrive.exe
) else (
    echo [ERROR] Build failed.
)

endlocal