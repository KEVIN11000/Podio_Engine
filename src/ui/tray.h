#ifndef TRAY_H
#define TRAY_H

#include <windows.h>

#define WM_TRAYICON (WM_USER + 1)

// Menu command IDs
#define TRAY_CMD_QUIT 1001

// Initializes and displays the system tray icon
void Tray_Init(HWND hwnd);

// Removes the system tray icon
void Tray_Cleanup(void);

// Handles window messages related to the tray icon
void Tray_HandleMessage(HWND hwnd, WPARAM wParam, LPARAM lParam);

#endif // TRAY_H
