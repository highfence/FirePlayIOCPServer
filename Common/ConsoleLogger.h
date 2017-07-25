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
			consoleOutWithColor(ConsoleColor::RED, pText);
		}

		void warn(const char * pText) override
		{
			consoleOutWithColor(ConsoleColor::YELLOW, pText);
		}

		void debug(const char * pText) override
		{
			consoleOutWithColor(ConsoleColor::WHITE, pText);
		}

		void trace(const char * pText) override
		{
			consoleOutWithColor(ConsoleColor::BLUE, pText);
		}

		void info(const char * pText) override
		{
			consoleOutWithColor(ConsoleColor::GREEN, pText);
		}

	};
}