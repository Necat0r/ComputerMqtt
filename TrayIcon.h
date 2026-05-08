#pragma once

#include <Windows.h>

namespace TrayIcon
{
	static constexpr UINT WM_TRAYICON    = WM_APP + 1;
	static constexpr UINT WM_SETCONNECTED = WM_APP + 2;
	static constexpr UINT ICON_ID        = 1;

	void add(HWND hwnd);
	void update(HWND hwnd, bool connected);
	void remove(HWND hwnd);
}
