#include "../include/tray.h"
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <string>

static NOTIFYICONDATAA g_nid = {};
static HWND g_hwnd = nullptr;
static UINT WM_TRAYICON = WM_USER + 1;
static TrayActivateCallback g_onActivate = nullptr;

static LRESULT CALLBACK TrayWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_CREATE)
    {
        return 0;
    }
    else if (msg == WM_TRAYICON)
    {
        if (lParam == WM_LBUTTONDBLCLK || lParam == WM_LBUTTONUP)
        {
            if (g_onActivate) g_onActivate();
        }
        return 0;
    }
    else if (msg == WM_DESTROY)
    {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

bool trayInitialize()
{
    HINSTANCE hInstance = GetModuleHandleA(nullptr);
    WNDCLASSA wc = {};
    wc.lpfnWndProc = TrayWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "XmlConfigTrayWindow";
    if (!RegisterClassA(&wc))
        return false;

    g_hwnd = CreateWindowExA(0, wc.lpszClassName, "XmlConfigTray", WS_OVERLAPPEDWINDOW,
                             CW_USEDEFAULT, CW_USEDEFAULT, 300, 200,
                             nullptr, nullptr, hInstance, nullptr);
    if (!g_hwnd)
        return false;

    g_nid = {};
    g_nid.cbSize = sizeof(g_nid);
    g_nid.hWnd = g_hwnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_MESSAGE | NIF_TIP | NIF_ICON;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    strcpy_s(g_nid.szTip, "XML Config");
    Shell_NotifyIconA(NIM_ADD, &g_nid);
    return true;
}

void trayShutdown()
{
    if (g_nid.hWnd)
    {
        Shell_NotifyIconA(NIM_DELETE, &g_nid);
    }
    if (g_hwnd)
    {
        DestroyWindow(g_hwnd);
        g_hwnd = nullptr;
    }
}

void trayShowNotification(const char* title, const char* message)
{
    if (!g_hwnd) return;
    NOTIFYICONDATAA nid = g_nid;
    nid.uFlags |= NIF_INFO;
    strncpy_s(nid.szInfoTitle, title, _TRUNCATE);
    strncpy_s(nid.szInfo, message, _TRUNCATE);
    nid.dwInfoFlags = NIIF_INFO;
    Shell_NotifyIconA(NIM_MODIFY, &nid);
}

void traySetOnActivate(TrayActivateCallback cb)
{
    g_onActivate = cb;
}

#else
bool trayInitialize() { return false; }
void trayShutdown() {}
void trayShowNotification(const char*, const char*) {}
#endif


