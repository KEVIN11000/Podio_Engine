#ifndef IDLE_ENGINE_H
#define IDLE_ENGINE_H

#include <windows.h>

// Returns TRUE if the user has been idle for >= idleSeconds.
BOOL IdleEngine_IsIdle(int idleSeconds);

// Shows a topmost overlay, waits for any user input, then calls
// LockWorkStation() and destroys the overlay.
void IdleEngine_ShowAndLock(HINSTANCE hInstance);

#endif // IDLE_ENGINE_H
