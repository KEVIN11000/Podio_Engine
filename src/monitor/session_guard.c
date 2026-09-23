#include "session_guard.h"
#include <wtsapi32.h>

void SessionGuard_Register(HWND hwnd) {
    WTSRegisterSessionNotification(hwnd, NOTIFY_FOR_THIS_SESSION);
}

BOOL SessionGuard_HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam,
                                BOOL* pShouldPause) {
    (void)lParam;

    if (uMsg == WM_WTSSESSION_CHANGE) {
        switch (wParam) {
            case WTS_SESSION_LOCK:
                *pShouldPause = TRUE;
                return TRUE;
            case WTS_SESSION_UNLOCK:
                *pShouldPause = FALSE;
                return TRUE;
        }
    }

    if (uMsg == WM_POWERBROADCAST) {
        switch (wParam) {
            case PBT_APMSUSPEND:
                *pShouldPause = TRUE;
                return TRUE;
            case PBT_APMRESUMEAUTOMATIC:
                *pShouldPause = FALSE;
                return TRUE;
        }
    }

    return FALSE;
}

void SessionGuard_Unregister(HWND hwnd) {
    WTSUnRegisterSessionNotification(hwnd);
}
