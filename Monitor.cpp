
#include "Monitor.h"

#include <iostream>

#include <Windows.h>
#include <powrprof.h>


using HookFunction = std::function<LPARAM(int code, WPARAM wParam, LPARAM lParam)>;
static HookFunction s_instanceHook;
HHOOK s_mainHook;

static LRESULT WINAPI hookProc(int code, WPARAM wParam, LPARAM lParam)
{
	const CWPRETSTRUCT* payload = (const CWPRETSTRUCT*)lParam;

	s_instanceHook(payload->message, payload->wParam, payload->lParam);

	return CallNextHookEx(s_mainHook, code, wParam, lParam);
}

Monitor::Monitor(PowerCallback&& callback)
: m_powerCallback(std::move(callback))
, m_monitorOn(true)
{
	HINSTANCE hInstance = GetModuleHandle(NULL);
	s_mainHook = SetWindowsHookEx(WH_CALLWNDPROCRET, ::hookProc, hInstance, GetCurrentThreadId());
	if (s_mainHook == 0)
	{
		std::cout << "Hook failed with error: " << GetLastError() << std::endl;
		return;
	}

	s_instanceHook = [this](int message, WPARAM wParam, LPARAM lParam) -> LRESULT {
		switch (message)
		{
		case WM_SYSCOMMAND:
			if (wParam == SC_MONITORPOWER && lParam == 2)
			{
				// Something shut off the monitor power
				changeStatus(false);
			}
			break;

		case WM_KEYDOWN:
		case WM_MOUSEMOVE:
			// Predict this will wake us up.
			changeStatus(true);
			break;

		default:
			break;
		}

		return NULL;
	};
}

Monitor::~Monitor()
{
	UnhookWindowsHookEx(s_mainHook);
}

void Monitor::setPower(bool value)
{
	if (value)
	{
		// Turning the display on via WM_SYSCOMMAND seems to be broken...

		mouse_event(MOUSEEVENTF_MOVE, 0, 1, 0, NULL);
		Sleep(40);
		mouse_event(MOUSEEVENTF_MOVE, 0, (DWORD)-1, 0, NULL);

		// Predict we're on
		changeStatus(true);
	}
	else
	{
		const LPARAM Off = 2;
		PostMessage(HWND_BROADCAST, WM_SYSCOMMAND, SC_MONITORPOWER, Off);

		// Predict we're off
		changeStatus(false);
	}
}

void Monitor::changeStatus(bool value)
{
	if (m_monitorOn != value)
	{
		std::cout << "Monitor switching " << (value ? "ON" : "OFF") << std::endl;
		m_monitorOn = value;
		m_powerCallback(m_monitorOn);
	}
}