#include "LobbyManager.h"

#include <memory>

#include "../Common/ConsoleLogger.h"
#include "../Common/Packet.h"

#include "Lobby.h"

namespace FirePlayLogic
{
	void LobbyManager::Init(const LobbyManagerConfig config, PacketQueue * sendQueue, ConsoleLogger * logger)
	{
		_sendQueue = sendQueue;
		_logger = logger;

		for (int i = 0; i < config.MaxLobbyCount; ++i)
		{
			Lobby lobby;
			lobby.Init((short)i, (short)config.MaxLobbyUserCount, (short)config.MaxRoomCountByLobby, (short)config.MaxRoomUserCount);
			lobby.SetNetwork(_sendQueue, _logger);

			_lobbyList.emplace_back(std::move(lobby));
		}
	}

	Lobby * LobbyManager::GetLobby(short lobbyId)
	{
		if (lobbyId < 0 || lobbyId >= (short)_lobbyList.size())
		{
			return nullptr;
		}

		return &_lobbyList[lobbyId];
	}

	void LobbyManager::SendLobbyListInfo(const int sessionIdx)
	{
		FirePlayCommon::PktLobbyListRes resPkt;
		resPkt.ErrorCode = (short)ERROR_CODE::NONE;
		resPkt.LobbyCount = static_cast<short>(_lobbyList.size());

		int idx = 0;
		for (auto& lobby : _lobbyList)
		{
			resPkt.LobbyList[idx].LobbyId = lobby.GetIndex();
			resPkt.LobbyList[idx].LobbyUserCount = lobby.GetUserCount();

			++idx;
		}

		std::shared_ptr<RecvPacketInfo> sendPacket = std::make_shared<RecvPacketInfo>();
		sendPacket->SessionIndex = sessionIdx;
		sendPacket->PacketId = (short)PACKET_ID::LOBBY_LIST_RES;
		sendPacket->PacketBodySize = sizeof(resPkt);
		sendPacket->pData = (char*)&resPkt;

		_sendQueue->Push(sendPacket);
	}
}
