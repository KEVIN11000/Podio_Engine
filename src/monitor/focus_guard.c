#include "focus_guard.h"

BOOL FocusGuard_IsOccluded(void) {
    HWND hFore = GetForegroundWindow();
    if (!hFore) return FALSE;

    // Ignore desktop-class windows (Progman, WorkerW, Shell_TrayWnd)
    wchar_t className[64] = {0};
    GetClassNameW(hFore, className, 64);
    if (lstrcmpW(className, L"Progman")        == 0 ||
        lstrcmpW(className, L"WorkerW")        == 0 ||
        lstrcmpW(className, L"Shell_TrayWnd")  == 0) {
        return FALSE;
    }

    // Check whether the foreground window covers the entire primary monitor
    RECT rc;
    GetWindowRect(hFore, &rc);

    int scrW = GetSystemMetrics(SM_CXSCREEN);
    int scrH = GetSystemMetrics(SM_CYSCREEN);

    if ((rc.right - rc.left) >= scrW && (rc.bottom - rc.top) >= scrH) {
        return TRUE;   // Fullscreen app detected
    }
    return FALSE;
}
