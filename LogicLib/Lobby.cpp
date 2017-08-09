#include "Lobby.h"

#include <memory>
#include <string>
#include <algorithm>

#include "../Common/ErrorCode.h"
#include "../Common/Packet.h"

#include "../Common/Define.h"

#include "Room.h"
#include "User.h"

namespace FirePlayLogic
{
	void Lobby::Init(
		const short lobbyIdx,
		const short maxLobbyUserCount,
		const short maxRoomCountByLobby,
		const short maxRoomUserCount)
	{
		_index = lobbyIdx;
		_maxUserCount = maxLobbyUserCount;

		for (int i = 0; i < maxLobbyUserCount; ++i)
		{
			LobbyUser lobbyUser;
			lobbyUser.idx = (short)i;
			lobbyUser.user = nullptr;

			_userList.emplace_back(std::move(lobbyUser));
		}

		for (int i = 0; i < maxRoomCountByLobby; ++i)
		{
			_roomList.emplace_back(new Room());
			_roomList[i]->init((short)i, maxRoomUserCount);
		}
	}

	void Lobby::Release()
	{
		for (int i = 0; i < (int)_roomList.size(); ++i)
		{
			delete _roomList[i];
		}
		_roomList.clear();
	}

	void Lobby::SetNetwork(PacketQueue * sendQueue, ConsoleLogger * logger)
	{
		_sendQueue = sendQueue;
		_logger = logger;

		// TODO :: RoomList ��Ʈ��ũ ����.
		for (auto room : _roomList)
		{
		}
	}

	ERROR_CODE Lobby::EnterUser(User * user)
	{
		// ���� �� �ִ� ���� �ڸ��� ���Ҵ��� Ȯ��.
		if (_userIdDic.size() >= _maxUserCount)
		{
			return ERROR_CODE::LOBBY_ENTER_MAX_USER_COUNT;
		}

		// �ߺ� �������� Ȯ��.
		if (FindUser(user->GetIndex()) != nullptr)
		{
			return ERROR_CODE::LOBBY_ENTER_USER_DUPLICATION;
		}

		// ���� ��Ͽ� �߰����ش�.
		auto retval = AddUser(user);
		if (retval != ERROR_CODE::NONE)
		{
			return retval;
		}

		// ���� ���¸� �κ�� ����.
		user->EnterLobby(_index);

		// ���� ������ ��ųʸ��� ����.
		_userIdDic.insert({ user->GetId().c_str(), user });
		_userIdxDic.insert({ user->GetIndex(), user });
	}

	ERROR_CODE Lobby::LeaveUser(const int userIdx)
	{
		// ���� ���� ����.
		RemoveUser(userIdx);

		// ���� ���� ���� Ȯ��.
		auto user = FindUser(userIdx);

		// ���� �������� ���� ������ VALID���� �ʴٸ�(���� ������ ���� �ʾҴ� ����) 
		if (user == nullptr)
		{
			return ERROR_CODE::LOBBY_LEAVE_USER_NVALID_UNIQUEINDEX;
		}

		user->LeaveLobby();

		_userIdDic.erase(user->GetId().c_str());
		_userIdxDic.erase(user->GetIndex());

		return ERROR_CODE::NONE;
	}

	short Lobby::GetUserCount()
	{
		return static_cast<short>(_userIdxDic.size());
	}

	void Lobby::NotifyLobbyEnterUserInfo(User * user)
	{
		FirePlayCommon::PktLobbyNewUserInfoNtf pkt;
		// ������ ID ������.
		strncpy_s(pkt.UserID, _countof(pkt.UserID), user->GetId().c_str(), FirePlayCommon::MAX_USER_ID_SIZE);

		// �ٸ� ������ �����鿡�� �˸�.
		sendToAllUser((short)PACKET_ID::LOBBY_ENTER_USER_NTF, sizeof(pkt), (char*)&pkt, user->GetIndex());
	}

	void Lobby::NotifyLobbyLeaveUserInfo(User * user)
	{
		FirePlayCommon::PktLobbyLeaveUserInfoNtf pkt;
		strncpy_s(pkt.UserID, _countof(pkt.UserID), user->GetId().c_str(), FirePlayCommon::MAX_USER_ID_SIZE);
		
		sendToAllUser((short)PACKET_ID::LOBBY_LEAVE_USER_NTF, sizeof(pkt), (char*)&pkt, user->GetIndex());
	}

	void Lobby::NotifyChat(const int sessionIdx, const char * userId, const wchar_t * msg)
	{
		FirePlayCommon::PktLobbyChatNtf pkt;
		strncpy_s(pkt.UserID, _countof(pkt.UserID), userId, FirePlayCommon::MAX_USER_ID_SIZE);
		wcsncpy_s(pkt.Msg, FirePlayCommon::MAX_LOBBY_CHAT_MSG_SIZE + 1, msg, FirePlayCommon::MAX_LOBBY_CHAT_MSG_SIZE);

		sendToAllUser((short)PACKET_ID::LOBBY_CHAT_NTF, sizeof(pkt), (char*)&pkt, sessionIdx);
	}

	User * Lobby::FindUser(const int userIdx)
	{
		auto destUser = _userIdxDic.find(userIdx);

		if (destUser == _userIdxDic.end())
		{
			return nullptr;
		}

		// ���� �ڵ忡���� (User*)�� ���·� ĳ�������ְ� ����. ���� �ʿ�.
		return destUser->second;
	}

	ERROR_CODE Lobby::AddUser(User * user)
	{
		// ����ִ� ���� �ڸ��� ã�´�.
		auto emptyUserSpace = std::find_if(std::begin(_userList), std::end(_userList),
			[](auto& lobbyUser) { return lobbyUser.user == nullptr; });

		// ����ִ� �ڸ��� ���ٸ� ���� ��ȯ.
		if (emptyUserSpace == std::end(_userList))
		{
			return ERROR_CODE::LOBBY_ENTER_EMPTY_USER_LIST;
		}

		emptyUserSpace->user = user;

		return ERROR_CODE::NONE;
	}

	void Lobby::RemoveUser(const int userIdx)
	{
		// �����Ϸ��� user�� �ֳ� Ȯ��.
		auto findUser = std::find_if(std::begin(_userList), std::end(_userList),
			[userIdx](LobbyUser& lobbyUser) { return (lobbyUser.user != nullptr) && (lobbyUser.user->GetIndex() == userIdx); });

		// ���ٸ� �׳� ����.
		if (findUser == std::end(_userList))
		{
			return;
		}

		findUser->user = nullptr;
	}


	void Lobby::sendToAllUser(const short packetId, const short dataSize, char * pData, const int passUserIdx)
	{
		for (auto& userDictionary : _userIdxDic)
		{
			auto& user = userDictionary.second;

			// passUserIdx���� Ȯ��.
			if (user->GetIndex() == passUserIdx)
			{
				continue;
			}

			// ���� �κ� �ִ� �������� Ȯ��.
			if (user->IsCurStateIsLobby() == false)
			{
				continue;
			}

			// ������ �´ٸ� ��Ŷ ����.
			std::shared_ptr<RecvPacketInfo> sendPacket = std::make_shared<RecvPacketInfo>();
			sendPacket->PacketId = packetId;
			sendPacket->SessionIndex = user->GetSessionIdx();
			sendPacket->PacketBodySize = dataSize;
			sendPacket->pData = pData;
			
			_sendQueue->Push(sendPacket);
		}
	}
}

