#include "Install.h"
#include "Log.h"

#include <Windows.h>
#include <string>

int installAutoStart(const char* host, const char* portStr)
{
	wchar_t exePath[MAX_PATH] = {};
	GetModuleFileNameW(nullptr, exePath, MAX_PATH);

	// Build value: "<exe path>" <host> <port>
	std::wstring value = L"\"";
	value += exePath;
	value += L"\" ";

	auto narrowToWide = [](const char* s) -> std::wstring
	{
		int n = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
		std::wstring w(n, L'\0');
		MultiByteToWideChar(CP_UTF8, 0, s, -1, w.data(), n);
		w.resize(n - 1); // drop null terminator
		return w;
	};

	value += narrowToWide(host) + L" " + narrowToWide(portStr);

	HKEY key = nullptr;
	LONG err = RegOpenKeyExW(HKEY_CURRENT_USER,
		L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
		0, KEY_SET_VALUE, &key);
	if (err != ERROR_SUCCESS)
	{
		Log::write("--install: RegOpenKeyEx failed (%ld)", err);
		return -1;
	}

	err = RegSetValueExW(key, L"ComputerMqtt", 0, REG_SZ,
		reinterpret_cast<const BYTE*>(value.c_str()),
		static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t)));
	RegCloseKey(key);

	if (err != ERROR_SUCCESS)
	{
		Log::write("--install: RegSetValueEx failed (%ld)", err);
		return -1;
	}

	Log::write("Auto-start installed");
	return 0;
}

int uninstallAutoStart()
{
	HKEY key = nullptr;
	LONG err = RegOpenKeyExW(HKEY_CURRENT_USER,
		L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
		0, KEY_SET_VALUE, &key);
	if (err != ERROR_SUCCESS)
	{
		Log::write("--uninstall: RegOpenKeyEx failed (%ld)", err);
		return -1;
	}

	err = RegDeleteValueW(key, L"ComputerMqtt");
	RegCloseKey(key);

	if (err == ERROR_FILE_NOT_FOUND)
	{
		Log::write("Auto-start was not installed");
		return 0;
	}

	if (err != ERROR_SUCCESS)
	{
		Log::write("--uninstall: RegDeleteValue failed (%ld)", err);
		return -1;
	}

	Log::write("Auto-start uninstalled");
	return 0;
}
