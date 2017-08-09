#pragma once
#include <vector>
#include <unordered_map>

#include "../NetworkLib/PacketQueue.h"

namespace FirePlayCommon
{
	class ConsoleLogger;
}

namespace FirePlayLogic
{
	struct LobbyManagerConfig
	{
		int MaxLobbyCount;
		int MaxLobbyUserCount;
		int MaxRoomCountByLobby;
		int MaxRoomUserCount;
	};

	struct LobbySmallInfo
	{
		short Num;
		short UserCount;
	};

	using PacketQueue = FirePlayNetwork::PacketQueue;
	using ConsoleLogger = FirePlayCommon::ConsoleLogger;

	class Lobby;

	class LobbyManager
	{
	public :

		LobbyManager() {};
		~LobbyManager() {};

		void Init(const LobbyManagerConfig config, PacketQueue * sendQueue, ConsoleLogger * logger);

		Lobby * GetLobby(short lobbyId);

		void SendLobbyListInfo(const int sessionIdx);

	private :

		PacketQueue * _sendQueue;
		ConsoleLogger * _logger;

		std::vector<Lobby> _lobbyList;
	};
}


