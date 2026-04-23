#include "Power.h"

#include <Windows.h>
#include <powrprof.h>
#include <functional>

namespace
{
	std::function<void(bool)> s_powerCallback;
	HPOWERNOTIFY s_powerNotificationHandle = nullptr;

	ULONG CALLBACK PowerNotificationCallback(PVOID context, ULONG type, PVOID setting)
	{
		UNREFERENCED_PARAMETER(context);
		UNREFERENCED_PARAMETER(setting);

		if (!s_powerCallback)
			return ERROR_SUCCESS;

		switch (type)
		{
		case PBT_APMSUSPEND:
			s_powerCallback(false);
			break;

		case PBT_APMRESUMEAUTOMATIC:
		case PBT_APMRESUMESUSPEND:
			s_powerCallback(true);
			break;

		default:
			break;
		}

		return ERROR_SUCCESS;
	}
}

void initPowerNotifications(std::function<void(bool)>&& callback)
{
	s_powerCallback = std::move(callback);

	if (s_powerNotificationHandle)
		return;

	DEVICE_NOTIFY_SUBSCRIBE_PARAMETERS parameters = {};
	parameters.Callback = PowerNotificationCallback;
	parameters.Context = nullptr;

	HPOWERNOTIFY notificationHandle = nullptr;
	const DWORD result = PowerRegisterSuspendResumeNotification(
		DEVICE_NOTIFY_CALLBACK,
		&parameters,
		&notificationHandle);

	if (result == ERROR_SUCCESS)
		s_powerNotificationHandle = notificationHandle;
}

void standby()
{
	HANDLE processHandle = GetCurrentProcess();
	HANDLE tokenHandle;

	// Get user's access token for this process
	if (OpenProcessToken(processHandle, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &tokenHandle) != TRUE)
		return;

	// Lookup LUID for Shutdown privilege
	LUID privilegeId;
	if (LookupPrivilegeValue(nullptr, SE_SHUTDOWN_NAME, &privilegeId) != TRUE)
		return;

	TOKEN_PRIVILEGES privileges;
	privileges.Privileges[0].Luid = privilegeId;
	privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	privileges.PrivilegeCount = 1;

	BOOL result = AdjustTokenPrivileges(
		tokenHandle,
		FALSE,
		&privileges,
		0,
		nullptr,
		nullptr);

	if (result != TRUE)
		return;

	SetSystemPowerState(TRUE, TRUE);
}
