# RawDrive Engine

RawDrive Engine is a high-performance, native Windows C application that injects fully hardware-accelerated video backgrounds directly into the desktop. It operates at the core of the Windows UI (WorkerW) to provide a seamless, zero-latency desktop customization experience without compromising system performance.

## Features

- **Hardware Acceleration:** Native D3D11 / DXGI architecture offloads all scaling and rendering to the GPU.
- **Ultra-Lightweight:** Written in pure C (Win32 API) with minimal overhead.
- **Smart Power Management:** Automatically pauses rendering (0% CPU/GPU) when a fullscreen application is detected, or when the system is locked/suspended.
- **Cinematic Idle Screen:** Detects user inactivity and launches a cinematic showcase overlay that seamlessly integrates with the Windows Lock Screen on return.
- **Live System Tray:** Swap background videos instantly from the system tray without restarting the engine.
- **DPI Aware:** Automatically adapts to multi-monitor setups and Windows UI scaling.

## Installation for Users

To install and run RawDrive Engine on your PC:

1. Download or clone this repository.
2. Place your desired .mp4 background videos inside the ideos/ folder.
3. Double-click install.bat.

The installer will automatically package the engine into %LOCALAPPDATA%\RawDrive, register it to launch silently at startup, and place a shortcut on your Desktop.

## Development & Building

### Prerequisites
- MSYS2 with mingw-w64-ucrt-x86_64-gcc toolchain (Native Windows GCC).
- windres for compiling the application icon.

### Build Instructions
Open your terminal in the project root and run:
``bat
.\build.bat
``
This will compile the engine and output the executable to in\RawDrive.exe.

## Architecture

- **Core Injection:** Secures a handle to the hidden WorkerW window situated behind desktop icons.
- **Media Foundation:** Decodes .mp4 files using the IMFSourceReader into raw RGB32 frames, with precise A/V sync via QueryPerformanceCounter.
- **DirectX 11:** Uses an IDXGISwapChain with DXGI_SWAP_EFFECT_DISCARD to present frames to the desktop.

## License
Private / Proprietary.
