
#include "PacketProcess.h"

#include "../NetworkLib/IOCPNetwork.h"
#include "../NetworkLib/PacketQueue.h"

#include "../Common/Define.h"
#include "../Common/ErrorCode.h"
#include "../Common/PacketID.h"
#include "../Common/Packet.h"
#include "../Common/ConsoleLogger.h"

#include "ConnectedUserManager.h"
#include "UserManager.h"
#include "LobbyManager.h"

using PACKET_ID = FirePlayCommon::PACKET_ID;
using LogType = FirePlayCommon::LogType;

namespace FirePlayLogic
{
	ERROR_CODE PacketProcess::login(std::shared_ptr<RecvPacketInfo> packetInfo)
	{
		_logger->Write(LogType::LOG_DEBUG, "%s | Login Func Entry", __FUNCTION__);

		auto reqPkt = (FirePlayCommon::PktLogInReq*)packetInfo->pData;

		auto addRet = _userManager->AddUser(packetInfo->SessionIndex, reqPkt->szID);
		_logger->Write(LogType::LOG_DEBUG, "%s | Add User Session(%d), Id(%s)", __FUNCTION__, packetInfo->SessionIndex, reqPkt->szID);

		if (addRet != ERROR_CODE::NONE)
		{
			// TODO :: Check error
			_logger->Write(LogType::LOG_ERROR, "%s | AddUser Failed, ERROR_CODE(%d)", __FUNCTION__, static_cast<int>(addRet));
		}

		_connectedUserManager->SetLogin(packetInfo->SessionIndex);

		FirePlayCommon::PktLogInRes resPkt;
		resPkt.ErrorCode = (short)addRet;
		_logger->Write(LogType::LOG_DEBUG, "%s | AddUser ERROR_CODE(%d)", __FUNCTION__, static_cast<int>(addRet));

		std::shared_ptr<RecvPacketInfo> sendPacket = std::make_shared<RecvPacketInfo>();
		sendPacket->SessionIndex = packetInfo->SessionIndex;
		sendPacket->PacketId = (short)PACKET_ID::LOGIN_IN_RES;
		sendPacket->PacketBodySize = sizeof(FirePlayCommon::PktLogInRes);
		sendPacket->pData = (char*)&resPkt;

		_logger->Write(LogType::LOG_DEBUG, "%s | Make Send Packet(shared_ptr)", __FUNCTION__);

		_sendQueue->Push(sendPacket);
		_logger->Write(LogType::LOG_DEBUG, "%s | Push Packet(%d) Session(%d) to SendQueue", __FUNCTION__, sendPacket->PacketId, sendPacket->SessionIndex);

		return ERROR_CODE::NONE;
	}

	ERROR_CODE PacketProcess::lobbyList(std::shared_ptr<RecvPacketInfo> packetInfo)
	{
		auto reqUserRet = _userManager->GetUser(packetInfo->SessionIndex);
		auto errorCode = std::get<0>(reqUserRet);

		if (errorCode != ERROR_CODE::NONE)
		{
			return errorCode;
		}

		auto reqUser = std::get<1>(reqUserRet);

		if (reqUser->IsCurStateIsLogin() == false)
		{
			return ERROR_CODE::LOBBY_LIST_INVALID_DOMAIN;
		}

		_lobbyManager->SendLobbyListInfo(packetInfo->SessionIndex);

		return ERROR_CODE::NONE;
	}
}