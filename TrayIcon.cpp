#include "TrayIcon.h"

#include <shellapi.h>

namespace TrayIcon
{

void add(HWND hwnd)
{
    NOTIFYICONDATA nid{};
    nid.cbSize           = sizeof(nid);
    nid.hWnd             = hwnd;
    nid.uID              = ICON_ID;
    nid.uFlags           = NIF_ICON | NIF_TIP | NIF_SHOWTIP | NIF_MESSAGE;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon            = LoadIcon(nullptr, IDI_WARNING); // disconnected until MQTT connects
    wcscpy_s(nid.szTip, L"ComputerMqtt \u2014 Disconnected");
    Shell_NotifyIcon(NIM_ADD, &nid);
    nid.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIcon(NIM_SETVERSION, &nid);
}

void update(HWND hwnd, bool connected)
{
    NOTIFYICONDATA nid{};
    nid.cbSize = sizeof(nid);
    nid.hWnd   = hwnd;
    nid.uID    = ICON_ID;
    nid.uFlags = NIF_ICON | NIF_TIP | NIF_SHOWTIP;
    nid.hIcon  = LoadIcon(nullptr, connected ? IDI_INFORMATION : IDI_WARNING);
    wcscpy_s(nid.szTip, connected ? L"ComputerMqtt \u2014 Connected" : L"ComputerMqtt \u2014 Disconnected");
    Shell_NotifyIcon(NIM_MODIFY, &nid);
}

void remove(HWND hwnd)
{
    NOTIFYICONDATA nid{};
    nid.cbSize = sizeof(nid);
    nid.hWnd   = hwnd;
    nid.uID    = ICON_ID;
    Shell_NotifyIcon(NIM_DELETE, &nid);
}

}
