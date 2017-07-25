#pragma once

#include <Windows.h>
#include <iostream>
#include <stdarg.h>
#include <mutex>

namespace FirePlayCommon
{
	const int consoleLogMaxLength = 256;

	enum class LogType : int
	{
		LOG_TRACE = 1,
		LOG_INFO  = 2,
		LOG_ERROR = 3,
		LOG_WARN  = 4,
		LOG_DEBUG = 5,
	};

	class LogBase
	{
	public :

		LogBase() {};
		virtual ~LogBase() {};

		virtual void Write(const LogType type, const char * argFormat, ...)
		{
			char logText[consoleLogMaxLength];

			va_list args;
			va_start(args, argFormat);
			vsprintf_s(logText, consoleLogMaxLength, argFormat, args);
			va_end(args);

			switch (type)
			{
			case LogType::LOG_INFO :
				Info(logText);
				break;

			case LogType::LOG_ERROR :
				Error(logText);
				break;

			case LogType::LOG_WARN :
				Warn(logText);
				break;

			case LogType::LOG_DEBUG :
				Debug(logText);
				break;

			case LogType::LOG_TRACE :
				Info(logText);
				break;

			default:
				break;
			}
		}

	protected :

		enum class ConsoleColor : int
		{
			GREEN  = 2,
			BLUE   = 9,
			RED    = 12,
			YELLOW = 14,
			WHITE  = 15,
		};

		std::mutex _lock;

		virtual void Error(const char * pText) = 0;
		virtual void Warn(const char * pText) = 0;
		virtual void Debug(const char * pText) = 0;
		virtual void Trace(const char * pText) = 0;
		virtual void Info(const char * pText) = 0;

		void consoleOutWithColor(const ConsoleColor color, const char * logText)
		{
			std::lock_guard<std::mutex> _writeLock(_lock);

			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (int)color);
			std::cout << logText << std::endl;
		}
	};

}