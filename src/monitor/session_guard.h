#ifndef SESSION_GUARD_H
#define SESSION_GUARD_H

#include <windows.h>

// Call once after the host window is created.
void SessionGuard_Register(HWND hwnd);

// Call from the window procedure.  Returns TRUE if the message was
// handled (caller should return 0), FALSE otherwise.
// Sets *pShouldPause = TRUE when playback should pause,
//      *pShouldPause = FALSE when playback should resume.
BOOL SessionGuard_HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam,
                                BOOL* pShouldPause);

// Unregisters session notifications.
void SessionGuard_Unregister(HWND hwnd);

#endif // SESSION_GUARD_H
