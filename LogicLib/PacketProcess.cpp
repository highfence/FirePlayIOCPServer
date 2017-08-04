
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

	void PacketProcess::BroadCast(std::shared_ptr<RecvPacketInfo> packetInfo)
	{
		auto packetId = packetInfo->PacketId;

		// ���� ��Ŷ�� �ش��ϴ� �Լ������ ã�´�.
		auto subscribedFuctions = _processMap.find(packetId);

		if (subscribedFuctions == _processMap.end())
		{
			_logger->Write(LogType::LOG_ERROR, "%s | Invalid packed id input!", __FUNCTION__);
			return;
		}

		_logger->Write(LogType::LOG_DEBUG, "%s | PacketId(%d) is BroadCast", __FUNCTION__, static_cast<int>(packetId));

		// �Լ���ϵ��� ��������ش�.
		for (auto& function : subscribedFuctions->second)
		{
			function(packetInfo);
		}
	}

	void PacketProcess::Subscribe(short interestedPacketId, PacketFunc functor)
	{
		// �ش� ��Ŷ���̵�� ���� ����� �ٸ� ģ���� �ֳ� ã�ƺ���.
		if (_processMap.find(interestedPacketId) == _processMap.end())
		{
			// �ٸ� ģ���� ���ٸ�, ���� ����Ʈ�� ������ش�.
			PacketFuncList functionList;
			_processMap.emplace(interestedPacketId, std::move(functionList));
		}

		// �ش��ϴ� ������ ����Ʈ�� �� �Լ��� �о�ִ´�.
		_processMap.find(interestedPacketId)->second.emplace_back(std::move(functor));
	}

	void PacketProcess::Update()
	{
		auto recvPacket = _recvQueue->Peek();

		if (recvPacket == nullptr)
		{
			return;
		}

		_logger->Write(LogType::LOG_DEBUG, "%s | BroadCast %d packet", __FUNCTION__, recvPacket->PacketId);
		BroadCast(recvPacket);
	}

	void PacketProcess::registFunctions()
	{
		Subscribe((short)FirePlayNetwork::NET_ERROR_CODE::NTF_SYS_CONNECT_SESSION,
			std::bind(&PacketProcess::ntfSysConnectSession, this, std::placeholders::_1));

		Subscribe((short)FirePlayCommon::PACKET_ID::LOGIN_IN_REQ,
			std::bind(&PacketProcess::login, this, std::placeholders::_1));
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