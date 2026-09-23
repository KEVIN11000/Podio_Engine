#include <windows.h>
#include "core/hook_workerw.h"
#include "ui/tray.h"
#include "utils/config.h"
#include "utils/logger.h"

// Global application config
static AppConfig g_config;

// Window procedure for our injected background window
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            // Render a black background until D3D11 is ready
            HBRUSH brush = (HBRUSH)GetStockObject(BLACK_BRUSH);
            FillRect(hdc, &ps.rcPaint, brush);
            
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_DESTROY:
            Tray_Cleanup();
            Logger_Log(LOG_INFO, "RawDrive Engine shutting down.");
            Logger_Shutdown();
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)pCmdLine;
    (void)nCmdShow;

    // 0. Load configuration and start logger
    Config_Load(&g_config, "config.ini");
    Logger_Init("rawdrive.log");
    Logger_Log(LOG_INFO, "RawDrive Engine starting...");

    // 1. Get the background WorkerW
    HWND workerw = GetWorkerW();
    if (!workerw) {
        Logger_Log(LOG_ERROR, "Failed to locate WorkerW.");
        MessageBoxW(NULL, L"Failed to locate WorkerW.", L"Error", MB_OK | MB_ICONERROR);
        Logger_Shutdown();
        return 1;
    }
    Logger_Log(LOG_INFO, "WorkerW found: %p", (void*)workerw);

    // 2. Register the window class
    const wchar_t CLASS_NAME[] = L"RawDriveWindow";
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClassW(&wc);

    // 3. Create the window
    HWND hwnd = CreateWindowExW(
        0,                              // Optional window styles
        CLASS_NAME,                     // Window class
        L"RawDrive Background",         // Window text
        WS_POPUP | WS_VISIBLE,          // Window style (borderless)
        0, 0,                           // X, Y
        GetSystemMetrics(SM_CXSCREEN),  // Width
        GetSystemMetrics(SM_CYSCREEN),  // Height
        workerw,                        // Parent window (the WorkerW)
        NULL,                           // Menu
        hInstance,                      // Instance handle
        NULL                            // Additional application data
    );

    if (hwnd == NULL) {
        return 0;
    }

    // Set the parent explicitly to inject it behind icons
    SetParent(hwnd, workerw);

    // 4. Init the system tray icon on a separate hidden window
    Tray_Init(hInstance, hwnd);
    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
