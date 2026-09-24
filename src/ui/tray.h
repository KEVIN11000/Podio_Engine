#ifndef TRAY_H
#define TRAY_H

#include <windows.h>

#define WM_TRAYICON (WM_USER + 1)
#define WM_CHANGE_VIDEO (WM_APP + 1)

// Menu command IDs
#define TRAY_CMD_QUIT 1001
#define TRAY_CMD_AI_GENERATE 1002

// Creates a hidden message-only window and registers the tray icon.
// hInstance: the application instance.
// hRendererWnd: the main renderer window — will receive WM_CLOSE when user clicks Quit.
void Tray_Init(HINSTANCE hInstance, HWND hRendererWnd);

// Removes the tray icon and destroys the hidden window.
void Tray_Cleanup(void);

#endif // TRAY_H
