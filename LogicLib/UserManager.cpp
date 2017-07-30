#include "UserManager.h"

#include "../Common/ObjectPool.h"
#include "../Common/ErrorCode.h"


namespace FirePlayLogic
{
	void UserManager::Init(const int maxUserCount)
	{
		_userPool.Init(maxUserCount);
	}

	ERROR_CODE UserManager::AddUser(const int sessionIdx, const char * id)
	{
		if (findUser(id) != nullptr)
		{
			return ERROR_CODE::USER_MGR_ID_DUPLICATION;
		}

		auto userTag = _userPool.GetTag();
		auto user = _userPool[userTag];

		user.Init(userTag);
		user.Set(sessionIdx, id);

		_userSessionDic.insert({ sessionIdx, std::make_shared<User>(user) });
		_userIDDic.insert({ id, std::make_shared<User>(user) });

		return ERROR_CODE::NONE;
	}

	ERROR_CODE UserManager::RemoveUser(const int sessionIdx)
	{
		auto user = findUser(sessionIdx);

		if (user == nullptr)
		{
			return ERROR_CODE::USER_MGR_REMOVE_INVALID_SESSION;
		}

		auto idx = user->GetIndex();
		auto id = user->GetId();

		_userSessionDic.erase(sessionIdx);
		_userIDDic.erase(id.c_str());

		_userPool.ReleaseTag(idx);

		return ERROR_CODE::NONE;
	}

	std::tuple<ERROR_CODE, std::shared_ptr<User>> UserManager::GetUser(const int sessionIdx)
	{
		auto destUser = findUser(sessionIdx);

		if (destUser == nullptr)
		{
			return { ERROR_CODE::USER_MGR_INVALID_SESSION_INDEX, nullptr };
		}

		if (destUser->IsConfirm() == false)
		{
			return { ERROR_CODE::USER_MGR_NOT_CONFIRM_USER, nullptr };
		}

		return { ERROR_CODE::NONE, destUser };
	}

	std::shared_ptr<User> UserManager::findUser(const int sessionIdx)
	{
		auto findUser = _userSessionDic.find(sessionIdx);

		if (findUser == _userSessionDic.end())
		{
			return nullptr;
		}

		return findUser->second;
	}

	std::shared_ptr<User> UserManager::findUser(const char * id)
	{
		auto findUser = _userIDDic.find(id);

		if (findUser == _userIDDic.end())
		{
			return nullptr;
		}

		return findUser->second;
	}
}
