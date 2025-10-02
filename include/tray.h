#pragma once

// Windows system tray integration
bool trayInitialize();
void trayShutdown();
void trayShowNotification(const char* title, const char* message);

// Optional: set callback invoked when user activates tray icon (click/double-click)
typedef void (*TrayActivateCallback)();
void traySetOnActivate(TrayActivateCallback cb);


