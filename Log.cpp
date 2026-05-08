#include "Log.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <cstdio>
#include <ctime>
#include <cstdarg>
#include <string>
#include <mutex>

namespace
{
	constexpr long long MaxLogBytes = 1 * 1024 * 1024; // 1 MB

	std::mutex  s_mutex;
	bool        s_consoleEnabled = false;
	std::string s_logPath;
	std::string s_logPathBak;

	// Resolve path once on first use (also used by Log::getPath).
	const std::string& logPath()
	{
		if (!s_logPath.empty())
			return s_logPath;

		char localAppData[MAX_PATH]{};
		if (GetEnvironmentVariableA("LOCALAPPDATA", localAppData, MAX_PATH) == 0)
			GetTempPathA(MAX_PATH, localAppData);

		s_logPath    = std::string(localAppData) + "\\ComputerMqtt\\computermqtt.log";
		s_logPathBak = std::string(localAppData) + "\\ComputerMqtt\\computermqtt.log.bak";

		// Ensure the directory exists.
		std::string dir = std::string(localAppData) + "\\ComputerMqtt";
		CreateDirectoryA(dir.c_str(), nullptr); // no-op if already exists

		return s_logPath;
	}

	// Rotate the log file if it has grown past MaxLogBytes.
	void rotateIfNeeded(const std::string& path)
	{
		WIN32_FILE_ATTRIBUTE_DATA info{};
		if (!GetFileAttributesExA(path.c_str(), GetFileExInfoStandard, &info))
			return;

		LARGE_INTEGER size;
		size.HighPart = (LONG)info.nFileSizeHigh;
		size.LowPart  = info.nFileSizeLow;

		if (size.QuadPart >= MaxLogBytes)
		{
			DeleteFileA(s_logPathBak.c_str());
			MoveFileA(path.c_str(), s_logPathBak.c_str());
		}
	}

	void writeRaw(const char* line)
	{
		// OutputDebugString (visible in debugger / Sysinternals DebugView).
		OutputDebugStringA(line);
		OutputDebugStringA("\n");

		// Console (only in --console mode).
		if (s_consoleEnabled)
		{
			fputs(line, stdout);
			fputc('\n', stdout);
		}

		// Log file.
		const std::string& path = logPath();
		rotateIfNeeded(path);

		FILE* f = nullptr;
		if (fopen_s(&f, path.c_str(), "a") == 0 && f)
		{
			fputs(line, f);
			fputc('\n', f);
			fclose(f);
		}
	}
}

namespace Log
{
	void enableConsole()
	{
		s_consoleEnabled = true;
	}

	void write(const char* fmt, ...)
	{
		// Build the formatted message.
		char msgBuf[2048]{};
		va_list args;
		va_start(args, fmt);
		vsnprintf(msgBuf, sizeof(msgBuf), fmt, args);
		va_end(args);

		// Prepend a timestamp.
		std::time_t now = std::time(nullptr);
		std::tm localTime{};
		char timeBuf[24]{};
		if (localtime_s(&localTime, &now) == 0)
			std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", &localTime);
		else
			strncpy_s(timeBuf, "0000-00-00 00:00:00", sizeof(timeBuf));

		char line[2200]{};
		snprintf(line, sizeof(line), "[%s] %s", timeBuf, msgBuf);

		std::lock_guard<std::mutex> lock(s_mutex);
		writeRaw(line);
	}

	const char* getPath()
	{
		return logPath().c_str();
	}
}
