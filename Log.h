#pragma once

namespace Log
{
	// Enable writing to stdout in addition to the log file and OutputDebugString.
	// Call once at startup when running in --console mode.
	void enableConsole();

	// Write a formatted log line. Always goes to file + OutputDebugString.
	void write(const char* fmt, ...);
}
