#ifndef FOCUS_GUARD_H
#define FOCUS_GUARD_H

#include <windows.h>

// Returns TRUE if a fullscreen non-desktop application is covering the screen.
BOOL FocusGuard_IsOccluded(void);

#endif // FOCUS_GUARD_H
