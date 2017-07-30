#include "PacketProcess.h"

#include "../NetworkLib/IOCPNetwork.h"
#include "../NetworkLib/PacketQueue.h"

#include "../Common/ErrorCode.h"
#include "../Common/PacketID.h"
#include "../Common/Packet.h"

#include "ConnectedUserManager.h"
#include "UserManager.h"

using PACKET_ID = FirePlayCommon::PACKET_ID;

namespace FirePlayLogic
{
	ERROR_CODE PacketProcess::login(std::shared_ptr<RecvPacketInfo> packetInfo)
	{
		FirePlayCommon::PktLogInRes resPkt;
		auto reqPkt = (FirePlayCommon::PktLogInReq*)packetInfo->pData;

		auto addRet = _userManager->AddUser(packetInfo->SessionIndex, reqPkt->szID);

		if (addRet != ERROR_CODE::NONE)
		{
			// TODO :: Check error
		}

		_connectedUserManager->SetLogin(packetInfo->SessionIndex);

		resPkt.ErrorCode = (short)addRet;

		RecvPacketInfo sendPacket;
		sendPacket.SessionIndex = packetInfo->SessionIndex;
		sendPacket.PacketId = (short)PACKET_ID::LOGIN_IN_RES;
		sendPacket.PacketBodySize = sizeof(FirePlayCommon::PktLogInRes);
		sendPacket.pData = (char*)&resPkt;

		_sendQueue->Push(std::make_shared<RecvPacketInfo>(sendPacket));

		return ERROR_CODE::NONE;
	}

	ERROR_CODE PacketProcess::lobbyList(std::shared_ptr<RecvPacketInfo> packetInfo)
	{
		return ERROR_CODE();
	}

}