#include <windows.h>
#include <objbase.h>
#include "core/hook_workerw.h"
#include "core/idle_engine.h"
#include "monitor/focus_guard.h"
#include "monitor/session_guard.h"
#include "ui/tray.h"
#include "utils/config.h"
#include "utils/logger.h"
#include "video/d3d11_renderer.h"
#include "video/mf_decoder.h"

/* ── Globals ──────────────────────────────────────────────────────── */
static AppConfig g_config;
static Renderer  g_renderer;
static Decoder   g_decoder;
static BOOL      g_running   = TRUE;
static BOOL      g_paused    = FALSE;   /* session lock / power suspend */
static BOOL      decoderReady = FALSE;
static HINSTANCE g_hInstance  = NULL;

/* ── Window procedure for the injected background window ──────────  */
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg,
                             WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            EndPaint(hwnd, &ps);   /* D3D11 owns the surface now */
            return 0;
        }
        case WM_CHANGE_VIDEO: {
            const wchar_t* newPath = (const wchar_t*)lParam;
            Logger_Log(LOG_INFO, "Switching video to: %ls", newPath);

            /* Convert back to UTF-8 for config */
            char utf8Path[MAX_PATH_LEN];
            WideCharToMultiByte(CP_UTF8, 0, newPath, -1, utf8Path, MAX_PATH_LEN, NULL, NULL);
            lstrcpyA(g_config.video_path, utf8Path);

            /* Persist to config.ini */
            WritePrivateProfileStringA("Engine", "VideoPath", utf8Path, ".\\config.ini");

            /* Reboot the decoder */
            decoderReady = FALSE;
            Decoder_Shutdown(&g_decoder);

            if (g_renderer.device && Decoder_Init(&g_decoder, newPath) == 0) {
                decoderReady = TRUE;
                Logger_Log(LOG_INFO, "Video decoder successfully restarted.");
            } else {
                Logger_Log(LOG_ERROR, "Failed to load new video.");
            }
            return 0;
        }
        case WM_CLOSE:
            g_running = FALSE;
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            SessionGuard_Unregister(hwnd);
            Tray_Cleanup();
            Logger_Log(LOG_INFO, "RawDrive Engine shutting down.");
            Logger_Shutdown();
            PostQuitMessage(0);
            return 0;
        default: {
            /* Let the session guard inspect WTS / power messages */
            BOOL shouldPause = g_paused;
            if (SessionGuard_HandleMessage(uMsg, wParam, lParam, &shouldPause)) {
                if (shouldPause != g_paused) {
                    g_paused = shouldPause;
                    Logger_Log(LOG_INFO, "Session guard → %s",
                               g_paused ? "PAUSED" : "RESUMED");
                }
                return 0;
            }
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
        }
    }
}

/* ── Entry point ──────────────────────────────────────────────────── */
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    PWSTR pCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)pCmdLine;
    (void)nCmdShow;

    /* 0. COM + Config + Logger */
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    Config_Load(&g_config, "config.ini");
    Logger_Init("rawdrive.log");
    Logger_Log(LOG_INFO, "RawDrive Engine starting...");

    /* 1. Locate WorkerW */
    HWND workerw = GetWorkerW();
    if (!workerw) {
        Logger_Log(LOG_ERROR, "Failed to locate WorkerW.");
        MessageBoxW(NULL, L"Failed to locate WorkerW.", L"Error",
                    MB_OK | MB_ICONERROR);
        Logger_Shutdown();
        CoUninitialize();
        return 1;
    }
    Logger_Log(LOG_INFO, "WorkerW found: %p", (void*)workerw);

    /* 2. Register & create the background window */
    const wchar_t CLASS_NAME[] = L"RawDriveWindow";
    WNDCLASSW wc = {0};
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance      = hInstance;
    wc.lpszClassName  = CLASS_NAME;
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(
        0, CLASS_NAME, L"RawDrive Background",
        WS_POPUP | WS_VISIBLE,
        0, 0,
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN),
        workerw, NULL, hInstance, NULL);

    if (!hwnd) {
        Logger_Log(LOG_ERROR, "CreateWindowExW failed.");
        Logger_Shutdown();
        CoUninitialize();
        return 1;
    }
    SetParent(hwnd, workerw);
    g_hInstance = hInstance;

    /* 3. System tray + session guard */
    Tray_Init(hInstance, hwnd);
    SessionGuard_Register(hwnd);

    /* 4. Initialise D3D11 renderer */
    if (Renderer_Init(&g_renderer, hwnd) != 0) {
        Logger_Log(LOG_ERROR, "Renderer init failed — falling back to GDI.");
        /* Keep running with the black GDI background */
    }

    /* 5. Initialise Media Foundation decoder */
    wchar_t wVideoPath[MAX_PATH_LEN];
    MultiByteToWideChar(CP_UTF8, 0,
                        g_config.video_path, -1,
                        wVideoPath, MAX_PATH_LEN);

    if (g_renderer.device && Decoder_Init(&g_decoder, wVideoPath) == 0) {
        decoderReady = TRUE;
        Logger_Log(LOG_INFO, "Video pipeline ready — entering render loop.");
    } else {
        Logger_Log(LOG_WARN, "Decoder init failed — desktop will stay black.");
    }

    /* 6. Main loop (PeekMessage + frame decode/present) */
    MSG msg = {0};
    while (g_running) {
        /* Drain the Win32 message queue */
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                g_running = FALSE;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (!g_running) break;

        /* Check for Idle Engine trigger */
        if (g_config.idle_timeout_sec > 0 && IdleEngine_IsIdle(g_config.idle_timeout_sec)) {
            Logger_Log(LOG_INFO, "Idle timeout reached. Triggering cinematic overlay.");
            IdleEngine_ShowAndLock(g_hInstance);
            /* Return to normal state after lock */
            continue;
        }

        /* Check for focus occlusion or session/power pause */
        BOOL occluded = FocusGuard_IsOccluded();
        if (g_paused || occluded || !decoderReady) {
            Sleep(100);
            continue;
        }

        /* Decode and present frame */
        const BYTE* pixels = NULL;
        LONG pitch = 0;
        if (Decoder_ReadFrame(&g_decoder, &pixels, &pitch) == 0) {
            Renderer_UploadAndPresent(&g_renderer, pixels, (UINT)pitch);
        }
    }

    /* 7. Cleanup */
    if (decoderReady)       Decoder_Shutdown(&g_decoder);
    if (g_renderer.device)  Renderer_Shutdown(&g_renderer);
    CoUninitialize();
    return 0;
}
