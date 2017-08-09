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

		// ��������Ʈ���� ������ ã�ƺ���.
		auto enterUserRet = _userManager->GetUser(packetInfo->SessionIndex);
		auto errorCode = std::get<0>(enterUserRet);

		// ������ �����ϴ��� Ȯ��.
		if (errorCode != ERROR_CODE::NONE)
		{
			return errorCode;
		}

		auto enterUser = std::get<1>(enterUserRet);

		// �α��� ���°� �ƴϸ� ���� ��ȯ.
		if (enterUser->IsCurStateIsLogin() == false)
		{
			return ERROR_CODE::LOBBY_ENTER_INVALID_DOMAIN;
		}

		// ���õ� �κ� ã�´�.
		auto selectedLobby = _lobbyManager->GetLobby(reqPkt->LobbyId);

		// ���õ� �κ� �������� �ʴ´ٸ� ���� ��ȯ.
		if (selectedLobby == nullptr)
		{
			return ERROR_CODE::LOBBY_ENTER_INVALID_LOBBY_INDEX;
		}

		// ������ ������ �κ� �־��ش�.
		auto enterRet = selectedLobby->EnterUser(enterUser);
		
		// ���� �˻�.
		if (enterRet != ERROR_CODE::NONE)
		{
			return enterRet;
		}

		// �ٸ� ����鿡�� ���ο� ������ �������� �˸���.
		selectedLobby->NotifyLobbyEnterUserInfo(enterUser);

		// ���� ��Ŷ�� �����ش�.
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