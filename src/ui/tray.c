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
                HMENU hVideoMenu = CreatePopupMenu();

                // Scan videos directory
                static wchar_t videoFiles[50][MAX_PATH];
                int videoCount = 0;
                WIN32_FIND_DATAW ffd;
                HANDLE hFind = FindFirstFileW(L"videos\\*.mp4", &ffd);
                if (hFind != INVALID_HANDLE_VALUE) {
                    do {
                        if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && videoCount < 50) {
                            lstrcpyW(videoFiles[videoCount], ffd.cFileName);
                            AppendMenuW(hVideoMenu, MF_STRING, 2000 + videoCount, ffd.cFileName);
                            videoCount++;
                        }
                    } while (FindNextFileW(hFind, &ffd) != 0);
                    FindClose(hFind);
                }

                if (videoCount == 0) {
                    AppendMenuW(hVideoMenu, MF_STRING | MF_GRAYED, 0, L"No videos found");
                }

                AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hVideoMenu, L"Select Video");
                AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
                AppendMenuW(hMenu, MF_STRING, TRAY_CMD_QUIT, L"Quit RawDrive");

                SetForegroundWindow(hwnd);

                int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY,
                                         pt.x, pt.y, 0, hwnd, NULL);
                if (cmd == TRAY_CMD_QUIT) {
                    PostMessage(g_hRendererWnd, WM_CLOSE, 0, 0);
                } else if (cmd >= 2000 && cmd < 2000 + videoCount) {
                    int idx = cmd - 2000;
                    static wchar_t selectedPath[MAX_PATH];
                    lstrcpyW(selectedPath, L"videos\\");
                    lstrcatW(selectedPath, videoFiles[idx]);
                    // Send message to main window to change video
                    SendMessageW(g_hRendererWnd, WM_CHANGE_VIDEO, 0, (LPARAM)selectedPath);
                }

                DestroyMenu(hVideoMenu);
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
    nid.hIcon = LoadIconA(hInstance, "IDI_ICON1");
    if (!nid.hIcon) {
        nid.hIcon = LoadIcon(NULL, IDI_APPLICATION); // Fallback
    }
    lstrcpyW(nid.szTip, L"RawDrive Engine");

    Shell_NotifyIconW(NIM_ADD, &nid);
}

void Tray_Cleanup(void) {
    if (g_hTrayWnd) {
        DestroyWindow(g_hTrayWnd);
        g_hTrayWnd = NULL;
    }
}
