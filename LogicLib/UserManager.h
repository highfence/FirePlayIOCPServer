#pragma once
#include <unordered_map>
#include <deque>
#include <memory>
#include <string>

#include "../Common/ObjectPool.h"

#include "User.h"

namespace FirePlayCommon
{
	enum class ERROR_CODE : short;
}

namespace FirePlayLogic
{
	using ERROR_CODE = FirePlayCommon::ERROR_CODE;
	using UserPool = FirePlayCommon::ObjectPool<User>;

	class UserManager
	{
	public:

		UserManager() {};
		virtual ~UserManager() {};

		void Init(const int maxUserCount);

		ERROR_CODE AddUser(const int sessionIdx, const char * id);
		ERROR_CODE RemoveUser(const int sessionIdx);

		std::tuple<ERROR_CODE, std::shared_ptr<User>> GetUser(const int sessionIdx);

	private :

		std::shared_ptr<User> findUser(const int sessionIdx);
		std::shared_ptr<User> findUser(const char * id);
		
	private :

		UserPool _userPool;
		std::unordered_map<int, std::shared_ptr<User>> _userSessionDic;
		std::unordered_map<const char*, std::shared_ptr<User>> _userIDDic;
	};
}