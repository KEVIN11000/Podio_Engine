@echo off
setlocal

set CFLAGS=-O2 -Wall -Wextra -std=c11 -DUNICODE -D_UNICODE -DCOBJMACROS -municode -Isrc
set LIBS=-ld3d11 -ldxgi -lmf -lmfplat -lmfuuid -lmfreadwrite -luser32 -lgdi32 -lshell32 -lwtsapi32 -lole32 -luuid -lpropsys

if not exist bin mkdir bin

echo Compiling RawDrive Engine...
windres app.rc -O coff -o bin\app.res

gcc %CFLAGS% ^
    src\main.c ^
    src\core\hook_workerw.c ^
    src\core\idle_engine.c ^
    src\monitor\focus_guard.c ^
    src\monitor\session_guard.c ^
    src\ui\tray.c ^
    src\utils\config.c ^
    src\utils\logger.c ^
    src\video\d3d11_renderer.c ^
    src\video\mf_decoder.c ^
    bin\app.res ^
    -o bin\RawDrive.exe %LIBS% -mwindows

if %ERRORLEVEL% equ 0 (
    echo [OK] Build successful: bin\RawDrive.exe
) else (
    echo [ERROR] Build failed.
)

endlocal
