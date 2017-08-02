
#include "PacketProcess.h"

#include "../NetworkLib/IOCPNetwork.h"
#include "../NetworkLib/PacketQueue.h"

#include "../Common/ErrorCode.h"
#include "../Common/PacketID.h"
#include "../Common/Packet.h"
#include "../Common/ConsoleLogger.h"

#include "ConnectedUserManager.h"
#include "UserManager.h"

using PACKET_ID = FirePlayCommon::PACKET_ID;
using LogType = FirePlayCommon::LogType;

namespace FirePlayLogic
{
	ERROR_CODE PacketProcess::login(std::shared_ptr<RecvPacketInfo> packetInfo)
	{
		_logger->Write(LogType::LOG_DEBUG, "%s | Login Func Entry", __FUNCTION__);

		FirePlayCommon::PktLogInRes resPkt;
		auto reqPkt = (FirePlayCommon::PktLogInReq*)packetInfo->pData;

		auto addRet = _userManager->AddUser(packetInfo->SessionIndex, reqPkt->szID);
		_logger->Write(LogType::LOG_DEBUG, "%s | Add User Session(%d), Id(%s)", __FUNCTION__, packetInfo->SessionIndex, reqPkt->szID);

		if (addRet != ERROR_CODE::NONE)
		{
			// TODO :: Check error
			_logger->Write(LogType::LOG_ERROR, "%s | AddUser Failed, ERROR_CODE(%d)", __FUNCTION__, static_cast<int>(addRet));
		}

		// ERROR :: 이 이상 진행 불가. 데드락?
		_connectedUserManager->SetLogin(packetInfo->SessionIndex);

		resPkt.ErrorCode = (short)addRet;
		_logger->Write(LogType::LOG_DEBUG, "%s | AddUser ERROR_CODE(%d)", __FUNCTION__, static_cast<int>(addRet));

		RecvPacketInfo sendPacket;
		sendPacket.SessionIndex = packetInfo->SessionIndex;
		sendPacket.PacketId = (short)PACKET_ID::LOGIN_IN_RES;
		sendPacket.PacketBodySize = sizeof(FirePlayCommon::PktLogInRes);
		sendPacket.pData = (char*)&resPkt;

		_logger->Write(LogType::LOG_DEBUG, "%s | Push Packet(%d) to SendQueue", __FUNCTION__, sendPacket.PacketId);
		_sendQueue->Push(std::make_shared<RecvPacketInfo>(sendPacket));

		return ERROR_CODE::NONE;
	}

	ERROR_CODE PacketProcess::lobbyList(std::shared_ptr<RecvPacketInfo> packetInfo)
	{
		return ERROR_CODE();
	}
}