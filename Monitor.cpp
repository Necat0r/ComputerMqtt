#include "Monitor.h"

#include <Windows.h>
#include <powrprof.h>

void setMonitorPower(bool value)
{
	if (value)
	{
		// Turning the display on via WM_SYSCOMMAND seems to be broken...

		mouse_event(MOUSEEVENTF_MOVE, 0, 1, 0, NULL);
		Sleep(40);
		mouse_event(MOUSEEVENTF_MOVE, 0, (DWORD)-1, 0, NULL);
	}
	else
	{
		const LPARAM Off = 2;
		PostMessage(HWND_BROADCAST, WM_SYSCOMMAND, SC_MONITORPOWER, Off);
	}
}
