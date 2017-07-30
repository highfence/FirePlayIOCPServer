#include "User.h"

namespace FirePlayLogic
{
	void User::Init(const short index)
	{
		_idx = index;
	}

	void User::Clear()
	{
		_sessionIdx = 0;
		_id = "";
		_isConfirm = false;
		_currentState = UserState::NONE;
		_lobbyIdx = -1;
		_roomIdx = -1;
	}

	void User::Set(const int sessionIdx, const char * id)
	{
		_isConfirm = true;
		_currentState = UserState::LOGIN;

		_sessionIdx = sessionIdx;
		_id = id;
	}

	short User::GetIndex()
	{
		return _idx;
	}

	int User::GetSessionIdx()
	{
		return _sessionIdx;
	}

	std::string User::GetId()
	{
		return _id;
	}

	bool User::IsConfirm()
	{
		return _isConfirm;
	}

	short User::GetLobbyIndex()
	{
		return _lobbyIdx;
	}

	void User::EnterLobby(const short lobbyIdx)
	{
		_lobbyIdx = lobbyIdx;
		_currentState = UserState::LOBBY;
	}

	void User::LeaveLobby()
	{
		_currentState = UserState::LOGIN;
	}

	short User::GetRoomIndex()
	{
		return _roomIdx;
	}

	void User::EnterRoom(const short lobbyIdx, const short roomIdx)
	{
		_lobbyIdx = lobbyIdx;
		_roomIdx = roomIdx;
		_currentState = UserState::ROOM;
	}

	bool User::IsCurStateIsLogin()
	{
		return _currentState == UserState::LOGIN ? true : false;
	}

	bool User::IsCurStateIsLobby()
	{
		return _currentState == UserState::LOBBY ? true : false;
	}

	bool User::IsCurStateIsRoom()
	{
		return _currentState == UserState::ROOM ? true : false;
	}

}