#pragma once
#include <string>

namespace FirePlayCommon
{
	typedef struct ServerInfo
	{
		int Port = 0;
		int Backlog = 0;
		std::string ServerIp;
	};
}