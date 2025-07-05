#include "logger.h"

TRACELOGGING_DEFINE_PROVIDER(
	g_hServiceProvider,
	"RANskrilServiceLogger",
	(0x4b8d3d0e, 0x4a4e, 0x4405, 0xa1, 0x69, 0x70, 0x64, 0x62, 0x0b, 0x63, 0x2a));

Logger::Logger()
{
	TraceLoggingRegister(g_hServiceProvider);
}

Logger::~Logger()
{
	TraceLoggingUnregister(g_hServiceProvider);
}

Logger& Logger::GetInstance()
{
	static Logger instance;
	return instance;
}

void Logger::Write(std::wstring subcomponent, std::wstring message, int level)
{
	switch (level) {
	case WINEVENT_LEVEL_INFO:
		TraceLoggingWrite(
			g_hServiceProvider,
			"RANskril CORE",
			TraceLoggingLevel(WINEVENT_LEVEL_INFO),
			TraceLoggingWideString(subcomponent.c_str(), "Subcomponent"),
			TraceLoggingWideString(message.c_str(), "Message")
		);
		break;
	case WINEVENT_LEVEL_WARNING:
		TraceLoggingWrite(
			g_hServiceProvider,
			"RANskril CORE",
			TraceLoggingLevel(WINEVENT_LEVEL_WARNING),
			TraceLoggingValue(subcomponent.c_str(), "Subcomponent"),
			TraceLoggingValue(message.c_str(), "Message")
		);
		break;
	case WINEVENT_LEVEL_ERROR:
		TraceLoggingWrite(
			g_hServiceProvider,
			"RANskril CORE",
			TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
			TraceLoggingValue(subcomponent.c_str(), "Subcomponent"),
			TraceLoggingValue(message.c_str(), "Message")
		);
		break;
	case WINEVENT_LEVEL_CRITICAL:
		TraceLoggingWrite(
			g_hServiceProvider,
			"RANskril CORE",
			TraceLoggingLevel(WINEVENT_LEVEL_CRITICAL),
			TraceLoggingValue(subcomponent.c_str(), "Subcomponent"),
			TraceLoggingValue(message.c_str(), "Message")
		);
		break;
	default:
		TraceLoggingWrite(
			g_hServiceProvider,
			"RANskril CORE",
			TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
			TraceLoggingValue(subcomponent.c_str(), "Subcomponent"),
			TraceLoggingValue(message.c_str(), "Message")
		);
		break;
	}
}
