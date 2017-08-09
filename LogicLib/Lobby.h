#pragma once

#include <memory>
#include <vector>
#include <unordered_map>

#include "../Common/Define.h"

#include "../NetworkLib/PacketQueue.h"

#include "User.h"

namespace FirePlayCommon
{
	enum class ERROR_CODE : short;
	class ConsoleLogger;
}

namespace FirePlayNetwork
{
	class IOCPNetwork;
}

namespace FirePlayLogic
{
	using ERROR_CODE     = FirePlayCommon::ERROR_CODE;
	using ConsoleLogger  = FirePlayCommon::ConsoleLogger;
	using PACKET_ID      = FirePlayCommon::PACKET_ID;
	using RecvPacketInfo = FirePlayCommon::RecvPacketInfo;

	using PacketQueue    = FirePlayNetwork::PacketQueue;

	class Room;

	struct LobbyUser
	{
		short idx = 0;
		User * user = nullptr;
	};

	class Lobby
	{
	public :

		Lobby() {};
		virtual ~Lobby() {};

		void Init(
			const short lobbyIdx,
			const short maxLobbyUserCount,
			const short maxRoomCountByLobby,
			const short maxRoomUserCount);

		void Release();

		void SetNetwork(PacketQueue * sendQueue, ConsoleLogger * logger);

		short GetIndex() { return _index; }

		ERROR_CODE EnterUser(User* user);
		ERROR_CODE LeaveUser(const int userIdx);

		short GetUserCount();

		void NotifyLobbyEnterUserInfo(User* user);
		void NotifyLobbyLeaveUserInfo(User* user);

		//Room* CreateRoom();
		//Room* GetRoom();
		//void NotifyChangedRoomInfo(const short roomIndex);

		short GetMaxUserCount() { return (short)_maxUserCount; }
		short GetMaxRoomCount() { return (short)_roomList.size(); }

		void NotifyChat(const int sessionIdx, const char * userId, const wchar_t* msg);

	protected :

		User * FindUser(const int userIdx);

		ERROR_CODE AddUser(User* user);

		void RemoveUser(const int userIdx);

	private :

		void sendToAllUser(const short packetId, const short dataSize, char * pData, const int passUserIdx = -1);

	private :

		PacketQueue * _sendQueue = nullptr;
		ConsoleLogger * _logger = nullptr;

		short _index = 0;
		short _maxUserCount = 0;
		
		std::vector<LobbyUser> _userList;
		std::unordered_map<int, User*> _userIdxDic;
		std::unordered_map<const char*, User*> _userIdDic;

		std::vector<Room*> _roomList;
	};
}
