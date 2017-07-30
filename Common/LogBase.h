#pragma once

#include <WinSock2.h>
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
				info(logText);
				break;

			case LogType::LOG_ERROR :
				error(logText);
				break;

			case LogType::LOG_WARN :
				warn(logText);
				break;

			case LogType::LOG_DEBUG :
				debug(logText);
				break;

			case LogType::LOG_TRACE :
				info(logText);
				break;

			default:
				break;
			}
		}

	protected :

		enum class ConsoleColor : WORD 
		{
			GREEN  = 2,
			BLUE   = 9,
			RED    = 12,
			YELLOW = 14,
			WHITE  = 15,
		};

		std::mutex _lock;

		virtual void error(const char * pText) = 0;
		virtual void warn(const char * pText) = 0;
		virtual void debug(const char * pText) = 0;
		virtual void trace(const char * pText) = 0;
		virtual void info(const char * pText) = 0;

		void consoleOutWithColor(const ConsoleColor color, const char* logHead, const char * logText)
		{
			std::lock_guard<std::mutex> _writeLock(_lock);

			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (WORD)color);
			std::cout << logText << std::endl;
		}
	};

}