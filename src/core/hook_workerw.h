#ifndef HOOK_WORKERW_H
#define HOOK_WORKERW_H

#include <windows.h>

// Retrieves the handle to the WorkerW window behind the desktop icons.
// This function sends a message to Progman to spawn a WorkerW, then enumerates windows to find it.
HWND GetWorkerW(void);

#endif // HOOK_WORKERW_H
