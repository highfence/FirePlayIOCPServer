#pragma once
#include <memory>

#include "../Common/Packet.h"
#include "../Common/ErrorCode.h"
#include "../Common/ConsoleLogger.h"
#include "../Common/Define.h"

#include "../NetworkLib/PacketQueue.h"
#include "../NetworkLib/IOCPNetwork.h"

#include "UserManager.h"
#include "LobbyManager.h"
#include "PacketProcess.h"


namespace FirePlayLogic
{
	using IOCPNetwork = FirePlayNetwork::IOCPNetwork;
	using PacketQueue = FirePlayNetwork::PacketQueue;
	using ConsoleLogger = FirePlayCommon::ConsoleLogger;
	using ServerInfo = FirePlayCommon::ServerInfo;
	using ERROR_CODE = FirePlayCommon::ERROR_CODE;

	class ServerHost
	{
	public :

		ServerHost() {};
		~ServerHost() {};

		ERROR_CODE Init();

		void Run();

		void Stop();

	private :

		ERROR_CODE loadConfig();

		void release();

	private :

		bool _isRun = false;

		std::unique_ptr<IOCPNetwork> _iocpNetwork;
		std::unique_ptr<ConsoleLogger> _logger;
		std::unique_ptr<ServerInfo> _serverInfo;

		std::unique_ptr<UserManager> _userManager;
		std::unique_ptr<LobbyManager> _lobbyManager;
		std::unique_ptr<PacketProcess> _packetProcess;

		PacketQueue * _recvQueue;
		PacketQueue * _sendQueue;
	};
}
