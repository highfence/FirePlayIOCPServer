#pragma once

#include "LogBase.h"

namespace FirePlayCommon
{
	class ConsoleLogger : public LogBase
	{
	public :

		ConsoleLogger() {};
		virtual ~ConsoleLogger() {};

	protected :

		void error(const char * pText) override
		{
			consoleOutWithColor(ConsoleColor::RED, "[ERROR] ", pText);
		}

		void warn(const char * pText) override
		{
			consoleOutWithColor(ConsoleColor::YELLOW, "[WARN] ", pText);
		}

		void debug(const char * pText) override
		{
			consoleOutWithColor(ConsoleColor::WHITE, "[DEBUG] ", pText);
		}

		void trace(const char * pText) override
		{
			consoleOutWithColor(ConsoleColor::BLUE, "[TRACE] ", pText);
		}

		void info(const char * pText) override
		{
			consoleOutWithColor(ConsoleColor::GREEN, "[INFO] ", pText);
		}

	};
}