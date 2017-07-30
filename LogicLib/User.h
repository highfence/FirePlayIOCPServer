#pragma once
#include <string>

namespace FirePlayLogic
{
	class User
	{
	public :

		enum class UserState : short
		{
			NONE = 0,
			LOGIN = 1,
			LOBBY = 2,
			ROOM = 3
		};

		User() {};
		virtual ~User() {};

		void Init(const short index);

		void Clear();

		void Set(const int sessionIdx, const char * id);

		short GetIndex();

		int GetSessionIdx();

		std::string GetId();

		bool IsConfirm();

		short GetLobbyIndex();

		void EnterLobby(const short lobbyIdx);

		void LeaveLobby();

		short GetRoomIndex();

		void EnterRoom(const short lobbyIdx, const short roomIdx);

		bool IsCurStateIsLogin();

		bool IsCurStateIsLobby();

		bool IsCurStateIsRoom();

	private :

		short _idx = -1;

		int _sessionIdx = -1;

		std::string _id;

		bool _isConfirm = false;

		UserState _currentState = UserState::NONE;

		short _lobbyIdx = -1;

		short _roomIdx = -1;
	};

}