#pragma once
#include <Windows.h>

#include "ConsoleLogger.h"

using ConsoleLogger = FirePlayCommon::ConsoleLogger;
using LogType = FirePlayCommon::LogType;

int wmain(int argc, wchar_t* argv[])
{
	ConsoleLogger logger;

	logger.Write(LogType::LOG_INFO, "%s | Hello!", __FUNCTION__);
	logger.Write(LogType::LOG_DEBUG, "%s | Nice", __FUNCTION__);
	logger.Write(LogType::LOG_ERROR, "%s | To", __FUNCTION__);
	logger.Write(LogType::LOG_TRACE, "%s | Meet", __FUNCTION__);
	logger.Write(LogType::LOG_WARN, "%s | You!", __FUNCTION__);

	return 0;
}
