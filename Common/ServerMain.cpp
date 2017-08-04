#pragma once
#include <WinSock2.h>
#include <iostream>
#include <thread>
#include <crtdbg.h>
#include "../LogicLib/ServerHost.h"

#ifndef _DEBUG
#define new new(_CLIENT_BLOCK,__FILE__,__LINE)
#endif

using ServerHost = FirePlayLogic::ServerHost;

int main(void)
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	ServerHost host;
	host.Init();

	host.Run();

	std::cout << "press any key to exit...";
	getchar();

	host.Stop();

	return 0;
}
