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

		// TODO :: RoomList 네트워크 설정.
		for (auto room : _roomList)
		{
		}
	}

	ERROR_CODE Lobby::EnterUser(User * user)
	{
		// 받을 수 있는 유저 자리가 남았는지 확인.
		if (_userIdDic.size() >= _maxUserCount)
		{
			return ERROR_CODE::LOBBY_ENTER_MAX_USER_COUNT;
		}

		// 중복 접속인지 확인.
		if (FindUser(user->GetIndex()) != nullptr)
		{
			return ERROR_CODE::LOBBY_ENTER_USER_DUPLICATION;
		}

		// 유저 목록에 추가해준다.
		auto retval = AddUser(user);
		if (retval != ERROR_CODE::NONE)
		{
			return retval;
		}

		// 유저 상태를 로비로 입장.
		user->EnterLobby(_index);

		// 유저 정보를 딕셔너리에 저장.
		_userIdDic.insert({ user->GetId().c_str(), user });
		_userIdxDic.insert({ user->GetIndex(), user });
	}

	ERROR_CODE Lobby::LeaveUser(const int userIdx)
	{
		// 유저 정보 삭제.
		RemoveUser(userIdx);

		// 떠날 유저 정보 확인.
		auto user = FindUser(userIdx);

		// 만약 떠나려는 유저 정보가 VALID하지 않다면(내가 가지고 있지 않았던 유저) 
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
		// 유저의 ID 얻어오기.
		strncpy_s(pkt.UserID, _countof(pkt.UserID), user->GetId().c_str(), FirePlayCommon::MAX_USER_ID_SIZE);

		// 다른 접속한 유저들에게 알림.
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

		// 원래 코드에서는 (User*)의 형태로 캐스팅해주고 있음. 주의 필요.
		return destUser->second;
	}

	ERROR_CODE Lobby::AddUser(User * user)
	{
		// 비어있는 유저 자리를 찾는다.
		auto emptyUserSpace = std::find_if(std::begin(_userList), std::end(_userList),
			[](auto& lobbyUser) { return lobbyUser.user == nullptr; });

		// 비어있는 자리가 없다면 에러 반환.
		if (emptyUserSpace == std::end(_userList))
		{
			return ERROR_CODE::LOBBY_ENTER_EMPTY_USER_LIST;
		}

		emptyUserSpace->user = user;

		return ERROR_CODE::NONE;
	}

	void Lobby::RemoveUser(const int userIdx)
	{
		// 제거하려는 user가 있나 확인.
		auto findUser = std::find_if(std::begin(_userList), std::end(_userList),
			[userIdx](LobbyUser& lobbyUser) { return (lobbyUser.user != nullptr) && (lobbyUser.user->GetIndex() == userIdx); });

		// 없다면 그냥 리턴.
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

			// passUserIdx인지 확인.
			if (user->GetIndex() == passUserIdx)
			{
				continue;
			}

			// 현재 로비에 있는 상태인지 확인.
			if (user->IsCurStateIsLobby() == false)
			{
				continue;
			}

			// 조건이 맞다면 패킷 제조.
			std::shared_ptr<RecvPacketInfo> sendPacket = std::make_shared<RecvPacketInfo>();
			sendPacket->PacketId = packetId;
			sendPacket->SessionIndex = user->GetSessionIdx();
			sendPacket->PacketBodySize = dataSize;
			sendPacket->pData = pData;
			
			_sendQueue->Push(sendPacket);
		}
	}
}

