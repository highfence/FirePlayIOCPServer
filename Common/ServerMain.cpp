#pragma once
#include <WinSock2.h>
#include <iostream>
#include <thread>
#include "../LogicLib/ServerHost.h"

using ServerHost = FirePlayLogic::ServerHost;

int main(void)
{
	ServerHost host;
	host.Init();

	host.Run();

	std::cout << "press any key to exit...";
	getchar();

	host.Stop();

	return 0;
}
