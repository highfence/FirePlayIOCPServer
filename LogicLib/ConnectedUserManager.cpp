#include "ConnectedUserManager.h"

#include <memory>

namespace FirePlayLogic
{
	void ConnectedUserManager::Init(
		const int		maxSessionCount,
		ConsoleLogger * logger,
		PacketQueue   * recvQueue,
		PacketQueue   * sendQueue,
		ServerInfo    * serverInfo)
	{
		_logger = logger;
		_recvQueue = recvQueue;
		_sendQueue = sendQueue;

		for (int i = 0; i < maxSessionCount; ++i)
		{
			_userList.emplace_back(std::make_shared<ConnectedUser>());
		}

		_isLoginCheckReadyed = serverInfo->IsLoginCheck;
	}

	void ConnectedUserManager::SetConnectSession(const int sessionIdx)
	{
		time(&_userList[sessionIdx]->_connectedTime);
	}
	
	void ConnectedUserManager::SetLogin(const int sessionIdx)
	{
		_userList[sessionIdx]->_isLoginSuccess = true;
	}
}
