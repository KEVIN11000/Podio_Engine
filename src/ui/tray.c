#include "tray.h"
#include <shellapi.h>

static NOTIFYICONDATAW nid = {0};

void Tray_Init(HWND hwnd) {
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    // Load a default system icon for now
    nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    lstrcpyW(nid.szTip, L"RawDrive Engine");

    Shell_NotifyIconW(NIM_ADD, &nid);
}

void Tray_Cleanup(void) {
    Shell_NotifyIconW(NIM_DELETE, &nid);
}

void Tray_HandleMessage(HWND hwnd, WPARAM wParam, LPARAM lParam) {
    (void)wParam;
    if (lParam == WM_RBUTTONUP || lParam == WM_LBUTTONUP) {
        POINT pt;
        GetCursorPos(&pt);
        
        HMENU hMenu = CreatePopupMenu();
        AppendMenuW(hMenu, MF_STRING, TRAY_CMD_QUIT, L"Quit RawDrive");
        
        // Required for the menu to disappear if the user clicks outside
        SetForegroundWindow(hwnd);
        
        int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hwnd, NULL);
        if (cmd == TRAY_CMD_QUIT) {
            PostMessage(hwnd, WM_CLOSE, 0, 0);
        }
        
        DestroyMenu(hMenu);
    }
}
