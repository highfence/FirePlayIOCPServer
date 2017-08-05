
#include "PacketProcess.h"

#include <memory>

#include "../Common/ConsoleLogger.h"
#include "../Common/Define.h"

#include "../NetworkLib/PacketQueue.h"
#include "../NetworkLib/ServerInfo.h"
#include "../NetworkLib/ServerNetErrorCode.h"

#include "UserManager.h"
#include "ConnectedUserManager.h"

namespace FirePlayLogic
{
	using LogType = FirePlayCommon::LogType;

	void PacketProcess::Init(
		ConsoleLogger * logger,
		UserManager   * userManager,
		LobbyManager  * lobbyManager,
		PacketQueue   * recvQueue,
		PacketQueue   * sendQueue,
		ServerInfo    * serverInfo)
	{
		if (logger    == nullptr ||
			recvQueue == nullptr ||
			sendQueue == nullptr)
		{
			return;
		}

		_logger       = logger;
		_userManager  = userManager;
		_lobbyManager = lobbyManager;
		_recvQueue    = recvQueue;
		_sendQueue    = sendQueue;

		_connectedUserManager = std::make_unique<ConnectedUserManager>();
		_connectedUserManager->Init(
			serverInfo->MaxClientCount,
			logger,
			recvQueue,
			sendQueue,
			serverInfo);

		registFunctions();
	}

	void PacketProcess::Update()
	{
		auto recvPacket = _recvQueue->Peek();

		if (recvPacket == nullptr)
		{
			return;
		}

		// ���� ��Ŷ ���̵� �����ϴ� ó�� �Լ��� ���� ���.
		if (_packetFuncArray[recvPacket->PacketId] == nullptr)
		{
			_logger->Write(LogType::LOG_WARN, "%s | There is no function correspond to packet(%d)", __FUNCTION__, recvPacket->PacketId);
			_recvQueue->Pop();
			return;
		}

		// �����ϴ� �Լ� ȣ��.
		_logger->Write(LogType::LOG_DEBUG, "%s | PacketId(%d) Function Called", __FUNCTION__, recvPacket->PacketId);
		_packetFuncArray[recvPacket->PacketId];
		_recvQueue->Pop();
	}

	void PacketProcess::registFunctions()
	{
		// �Լ� �迭 �ʱ�ȭ.
		for (int i = 0; i < (int)PACKET_ID::MAX; ++i)
		{
			_packetFuncArray[i] = nullptr;
		}

		_packetFuncArray[(int)NET_LIB_PACKET_ID::NTF_SYS_CONNECT_SESSION] = &PacketProcess::ntfSysConnectSession;
		_packetFuncArray[(int)PACKET_ID::LOGIN_IN_REQ] = &PacketProcess::login;
	}

	ERROR_CODE PacketProcess::ntfSysConnectSession(std::shared_ptr<RecvPacketInfo> packetInfo)
	{
		_connectedUserManager->SetConnectSession(packetInfo->SessionIndex);
		return ERROR_CODE::NONE;
	}

	ERROR_CODE PacketProcess::ntfSysCloseSession(std::shared_ptr<RecvPacketInfo> packetInfo)
	{
		return ERROR_CODE::NONE;
	}


}