@echo off
setlocal

set CFLAGS=-O2 -Wall -Wextra -std=c11 -DUNICODE -D_UNICODE -municode -Isrc
set LIBS=-luser32 -lgdi32 -lshell32 -ld3d11 -ldxgi -lmf -lmfplat -lmfuuid -lmfreadwrite -lwtsapi32

if not exist bin mkdir bin

echo Compiling RawDrive Engine...
gcc %CFLAGS% ^
    src\main.c ^
    src\core\hook_workerw.c ^
    src\ui\tray.c ^
    -o bin\RawDrive.exe %LIBS% -mwindows

if %ERRORLEVEL% equ 0 (
    echo [OK] Build successful: bin\RawDrive.exe
) else (
    echo [ERROR] Build failed.
)

endlocal
