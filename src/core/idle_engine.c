#include "idle_engine.h"

BOOL IdleEngine_IsIdle(int idleSeconds) {
    LASTINPUTINFO lii;
    lii.cbSize = sizeof(LASTINPUTINFO);
    if (!GetLastInputInfo(&lii)) return FALSE;

    DWORD elapsed = GetTickCount() - lii.dwTime;
    return elapsed >= (DWORD)(idleSeconds * 1000);
}

/* ── Overlay window procedure ─────────────────────────────────────── */
static LRESULT CALLBACK OverlayProc(HWND hwnd, UINT uMsg,
                                     WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            /* Solid black "cinematic" overlay */
            HBRUSH b = (HBRUSH)GetStockObject(BLACK_BRUSH);
            FillRect(hdc, &ps.rcPaint, b);
            EndPaint(hwnd, &ps);
            return 0;
        }
        /* Any keyboard or mouse input → lock the workstation */
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_MOUSEMOVE: {
            /* Only act on actual movement, not noise */
            static POINT lastPt = {-1, -1};
            if (uMsg == WM_MOUSEMOVE) {
                POINT cur;
                GetCursorPos(&cur);
                if (cur.x == lastPt.x && cur.y == lastPt.y) return 0;
                lastPt = cur;
            }
            DestroyWindow(hwnd);
            LockWorkStation();
            return 0;
        }
        case WM_DESTROY:
            return 0;
        default:
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
}

void IdleEngine_ShowAndLock(HINSTANCE hInstance) {
    static BOOL registered = FALSE;
    const wchar_t CLS[] = L"RawDriveIdleOverlay";

    if (!registered) {
        WNDCLASSW wc = {0};
        wc.lpfnWndProc   = OverlayProc;
        wc.hInstance      = hInstance;
        wc.lpszClassName  = CLS;
        wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
        RegisterClassW(&wc);
        registered = TRUE;
    }

    HWND overlay = CreateWindowExW(
        WS_EX_TOPMOST,
        CLS, L"RawDrive Idle",
        WS_POPUP | WS_VISIBLE,
        0, 0,
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN),
        NULL, NULL, hInstance, NULL);

    if (!overlay) return;

    /* Pump messages for the overlay until it self-destructs */
    MSG msg;
    while (GetMessage(&msg, overlay, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}
