#pragma once

#include <time.h>
#include <chrono>
#include <vector>
#include <memory>
#include "../Common/Define.h"
#include "../NetworkLib/IOCPNetwork.h"

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

	// TODO :: 추가 구현 필요
	class ConnectedUserManager
	{
	public :
		
		ConnectedUserManager() {};
		~ConnectedUserManager() {};

		void SetConnectSession(const int sessionIdx);
		void SetLogin(const int sessionIdx);

	private :

		ConsoleLogger * _logger;
		std::vector<std::shared_ptr<ConnectedUser>> _userList;
	};
}