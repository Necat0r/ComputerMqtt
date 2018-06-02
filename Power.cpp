#include "Power.h"

#include <Windows.h>

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
