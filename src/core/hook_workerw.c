#include "hook_workerw.h"

// Callback for EnumWindows to find the WorkerW window.
// The target WorkerW is a sibling of the window hosting SHELLDLL_DefView.
static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    HWND p = FindWindowExW(hwnd, NULL, L"SHELLDLL_DefView", NULL);
    if (p != NULL) {
        // Find the next sibling WorkerW
        HWND* workerw = (HWND*)lParam;
        *workerw = FindWindowExW(NULL, hwnd, L"WorkerW", NULL);
    }
    return TRUE; // Continue enumerating
}

HWND GetWorkerW(void) {
    // 1. Locate Progman
    HWND progman = FindWindowW(L"Progman", NULL);
    if (!progman) {
        return NULL;
    }

    // 2. Send the undocumented message 0x052C to spawn the background WorkerW
    DWORD_PTR result = 0;
    SendMessageTimeoutW(progman, 0x052C, 0, 0, SMTO_NORMAL, 1000, &result);

    // 3. Locate the spawned WorkerW
    HWND workerw = NULL;
    EnumWindows(EnumWindowsProc, (LPARAM)&workerw);

    return workerw;
}
