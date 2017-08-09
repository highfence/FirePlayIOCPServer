#include "PacketProcess.h"

#include "../Common/Packet.h"
#include "../Common/ErrorCode.h"

#include "../NetworkLib/PacketQueue.h"

#include "User.h"
#include "UserManager.h"
#include "LobbyManager.h"
#include "Lobby.h"

namespace FirePlayLogic
{
	ERROR_CODE PacketProcess::lobbyEnter(std::shared_ptr<RecvPacketInfo> packetInfo)
	{
		auto reqPkt = (FirePlayCommon::PktLobbyEnterReq*)packetInfo->pData;
		FirePlayCommon::PktLobbyEnterRes resPkt;

		// 유저리스트에서 유저를 찾아본다.
		auto enterUserRet = _userManager->GetUser(packetInfo->SessionIndex);
		auto errorCode = std::get<0>(enterUserRet);

		// 유저가 존재하는지 확인.
		if (errorCode != ERROR_CODE::NONE)
		{
			return errorCode;
		}

		auto enterUser = std::get<1>(enterUserRet);

		// 로그인 상태가 아니면 에러 반환.
		if (enterUser->IsCurStateIsLogin() == false)
		{
			return ERROR_CODE::LOBBY_ENTER_INVALID_DOMAIN;
		}

		// 선택된 로비를 찾는다.
		auto selectedLobby = _lobbyManager->GetLobby(reqPkt->LobbyId);

		// 선택된 로비가 존재하지 않는다면 에러 반환.
		if (selectedLobby == nullptr)
		{
			return ERROR_CODE::LOBBY_ENTER_INVALID_LOBBY_INDEX;
		}

		// 유저를 선택한 로비에 넣어준다.
		auto enterRet = selectedLobby->EnterUser(enterUser);
		
		// 에러 검사.
		if (enterRet != ERROR_CODE::NONE)
		{
			return enterRet;
		}

		// 다른 사람들에게 새로운 유저가 들어왔음을 알린다.
		selectedLobby->NotifyLobbyEnterUserInfo(enterUser);

		// 성공 패킷을 보내준다.
		resPkt.MaxUserCount = selectedLobby->GetMaxUserCount();
		resPkt.MaxRoomCount = selectedLobby->GetMaxRoomCount();

		std::shared_ptr<RecvPacketInfo> sendPacket = std::make_shared<RecvPacketInfo>();
		sendPacket->PacketId = (short)PACKET_ID::LOBBY_ENTER_RES;
		sendPacket->SessionIndex = enterUser->GetSessionIdx();
		sendPacket->PacketBodySize = sizeof(resPkt);
		//sendPacket->pData = (char*)&resPkt;
		sendPacket->pData = new char[sendPacket->PacketBodySize];
		memcpy(sendPacket->pData, (char*)&resPkt, sendPacket->PacketBodySize);

		_sendQueue->Push(sendPacket);

		return ERROR_CODE::NONE;
	}

	ERROR_CODE PacketProcess::lobbyRoomList(std::shared_ptr<RecvPacketInfo> packetInfo)
	{
		return ERROR_CODE();
	}

	ERROR_CODE PacketProcess::lobbyUserList(std::shared_ptr<RecvPacketInfo> packetInfo)
	{
		return ERROR_CODE();
	}

	ERROR_CODE PacketProcess::lobbyChat(std::shared_ptr<RecvPacketInfo> packetInfo)
	{
		return ERROR_CODE();
	}

	ERROR_CODE PacketProcess::lobbyWisper(std::shared_ptr<RecvPacketInfo> packetInfo)
	{
		return ERROR_CODE();
	}

	ERROR_CODE PacketProcess::lobbyLeave(std::shared_ptr<RecvPacketInfo> packetInfo)
	{
		return ERROR_CODE();
	}

}