#include "ConnectedUserManager.h"

namespace FirePlayLogic
{
	void ConnectedUserManager::SetConnectSession(const int sessionIdx)
	{
		time(&_userList[sessionIdx]->_connectedTime);
	}
}
