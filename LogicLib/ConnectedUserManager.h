#pragma once

#include <time.h>
#include <chrono>
#include <vector>
#include <memory>
#include "../Common/Define.h"
#include "../NetworkLib/IOCPNetwork.h"
#include "../NetworkLib/PacketQueue.h"

namespace FirePlayLogic
{
	struct ConnectedUser
	{
		void Clear()
		{
			_isLoginSuccess = false;
			_connectedTime = 0;
		}

		bool _isLoginSuccess = false;
		time_t _connectedTime = 0;
	};

	using ConsoleLogger = FirePlayCommon::ConsoleLogger;
	using ServerInfo = FirePlayCommon::ServerInfo;
	using PacketQueue = FirePlayNetwork::PacketQueue;

	// TODO :: 추가 구현 필요
	class ConnectedUserManager
	{
	public :
		
		ConnectedUserManager() {};
		~ConnectedUserManager() {};

		void Init(const int maxSessionCount, ConsoleLogger * logger, PacketQueue * recvQueue, PacketQueue * sendQueue, ServerInfo * config);

		void SetConnectSession(const int sessionIdx);
		void SetLogin(const int sessionIdx);

	private :

		ConsoleLogger * _logger;
		PacketQueue * _recvQueue;
		PacketQueue * _sendQueue;
		std::vector<std::shared_ptr<ConnectedUser>> _userList;

		bool _isLoginCheckReadyed = false;
	};
}