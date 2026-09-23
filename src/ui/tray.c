#include "tray.h"
#include <shellapi.h>

static NOTIFYICONDATAW nid = {0};
static HWND g_hTrayWnd = NULL;
static HWND g_hRendererWnd = NULL;

// Window procedure for the hidden tray-only window
static LRESULT CALLBACK TrayWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_TRAYICON:
            if (lParam == WM_RBUTTONUP || lParam == WM_LBUTTONUP) {
                POINT pt;
                GetCursorPos(&pt);

                HMENU hMenu = CreatePopupMenu();
                AppendMenuW(hMenu, MF_STRING, TRAY_CMD_QUIT, L"Quit RawDrive");

                // Required for the menu to disappear if the user clicks outside
                SetForegroundWindow(hwnd);

                int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY,
                                         pt.x, pt.y, 0, hwnd, NULL);
                if (cmd == TRAY_CMD_QUIT) {
                    // Signal the renderer window to close
                    PostMessage(g_hRendererWnd, WM_CLOSE, 0, 0);
                }

                DestroyMenu(hMenu);
            }
            return 0;

        case WM_DESTROY:
            Shell_NotifyIconW(NIM_DELETE, &nid);
            return 0;

        default:
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
}

void Tray_Init(HINSTANCE hInstance, HWND hRendererWnd) {
    g_hRendererWnd = hRendererWnd;

    // Register a dedicated window class for the tray
    const wchar_t TRAY_CLASS[] = L"RawDriveTrayClass";
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = TrayWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = TRAY_CLASS;
    RegisterClassW(&wc);

    // Create a hidden, message-only window
    g_hTrayWnd = CreateWindowExW(
        0, TRAY_CLASS, L"RawDrive Tray", 0,
        0, 0, 0, 0,
        HWND_MESSAGE,   // Message-only window — invisible, no parent in desktop tree
        NULL, hInstance, NULL
    );

    // Register the notification icon
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = g_hTrayWnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    lstrcpyW(nid.szTip, L"RawDrive Engine");

    Shell_NotifyIconW(NIM_ADD, &nid);
}

void Tray_Cleanup(void) {
    if (g_hTrayWnd) {
        DestroyWindow(g_hTrayWnd);
        g_hTrayWnd = NULL;
    }
}
