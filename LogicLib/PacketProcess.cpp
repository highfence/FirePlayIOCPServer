
#include "PacketProcess.h"

#include <memory>

#include "../Common/ConsoleLogger.h"
#include "../Common/Define.h"

#include "../NetworkLib/PacketQueue.h"
#include "../NetworkLib/ServerInfo.h"
#include "../NetworkLib/ServerNetErrorCode.h"

#include "UserManager.h"
#include "ConnectedUserManager.h"
#include "Lobby.h"
#include "LobbyManager.h"

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

		// 받은 패킷 아이디에 대응하는 처리 함수가 없는 경우.
		if (_packetFuncArray[recvPacket->PacketId] == nullptr)
		{
			_logger->Write(LogType::LOG_WARN, "%s | There is no function correspond to packet(%d)", __FUNCTION__, recvPacket->PacketId);
			_recvQueue->Pop();
			return;
		}

		// 대응하는 함수 호출.
		_logger->Write(LogType::LOG_DEBUG, "%s | PacketId(%d) Function Called", __FUNCTION__, recvPacket->PacketId);
		(this->*_packetFuncArray[recvPacket->PacketId])(recvPacket);
		_recvQueue->Pop();
	}

	void PacketProcess::registFunctions()
	{
		// 함수 배열 초기화.
		for (int i = 0; i < (int)PACKET_ID::MAX; ++i)
		{
			_packetFuncArray[i] = nullptr;
		}

		_packetFuncArray[(int)NET_LIB_PACKET_ID::NTF_SYS_CONNECT_SESSION  ] = &PacketProcess::ntfSysConnectSession;
		_packetFuncArray[(int)NET_LIB_PACKET_ID::NTF_SYS_CLOSE_SESSION    ] = &PacketProcess::ntfSysCloseSession;
		_packetFuncArray[(int)        PACKET_ID::LOGIN_IN_REQ             ] = &PacketProcess::login;
		_packetFuncArray[(int)        PACKET_ID::LOBBY_LIST_REQ           ] = &PacketProcess::lobbyList;
		_packetFuncArray[(int)        PACKET_ID::LOBBY_ENTER_REQ          ] = &PacketProcess::lobbyEnter;
		_packetFuncArray[(int)        PACKET_ID::LOBBY_ENTER_ROOM_LIST_REQ] = &PacketProcess::lobbyRoomList;
		_packetFuncArray[(int)        PACKET_ID::LOBBY_ENTER_USER_LIST_REQ] = &PacketProcess::lobbyUserList;
		_packetFuncArray[(int)        PACKET_ID::LOBBY_CHAT_REQ           ] = &PacketProcess::lobbyChat;
		_packetFuncArray[(int)        PACKET_ID::LOBBY_LEAVE_REQ          ] = &PacketProcess::lobbyLeave;
		_packetFuncArray[(int)        PACKET_ID::ROOM_ENTER_REQ           ] = &PacketProcess::roomEnter;
		_packetFuncArray[(int)        PACKET_ID::ROOM_LEAVE_REQ           ] = &PacketProcess::roomLeave;
	}

	ERROR_CODE PacketProcess::ntfSysConnectSession(std::shared_ptr<RecvPacketInfo> packetInfo)
	{
		_connectedUserManager->SetConnectSession(packetInfo->SessionIndex);
		return ERROR_CODE::NONE;
	}

	ERROR_CODE PacketProcess::ntfSysCloseSession(std::shared_ptr<RecvPacketInfo> packetInfo)
	{
		auto closeUser = std::get<1>(_userManager->GetUser(packetInfo->SessionIndex));

		if (closeUser != nullptr)
		{
			auto closeLobby = _lobbyManager->

		}
	}


}