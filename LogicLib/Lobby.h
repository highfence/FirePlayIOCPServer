#pragma once

#include <memory>
#include <vector>
#include <unordered_map>

namespace FirePlayCommon
{
	enum class ERROR_CODE : short;
	class ConsoleLogger;
}

namespace FirePlayNetwork
{
	class IOCPNetwork;
}

namespace FirePlayLogic
{
	using ERROR_CODE = FirePlayCommon::ERROR_CODE;
	using IOCPNetwork = FirePlayNetwork::IOCPNetwork;
	using ConsoleLogger = FirePlayCommon::ConsoleLogger;

	class User;

	struct LobbyUser
	{
		short idx = 0;
		std::shared_ptr<User> user = nullptr;
	};

	class Lobby
	{
	public :

		Lobby() {};
		virtual ~Lobby() {};

		void Init(const short lobbyIdx, const short maxLobbyUserCount, const short maxRoomCountByLobby, const short maxRoomUserCount);

		void Release();

		void SetNetwork(IOCPNetwork * network, ConsoleLogger * logger);


	private :

		void sendToAllUser(const short packetId, const short dataSize, char * pData, const int passUserIdx = -1);

	private :

	};
}
