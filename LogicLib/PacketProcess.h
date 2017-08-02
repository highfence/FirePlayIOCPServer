#pragma once
#include <memory>
#include <unordered_map>
#include <functional>
#include <list>

#include "../Common/PacketID.h"
#include "../Common/ErrorCode.h"
#include "../Common/Define.h"

#include "../NetworkLib/PacketQueue.h"

#include "ConnectedUserManager.h"

namespace FirePlayCommon
{
	class ConsoleLogger;
}

namespace FirePlayLogic
{
	using ConsoleLogger = FirePlayCommon::ConsoleLogger;
	using RecvPacketInfo = FirePlayCommon::RecvPacketInfo;
	using PACKET_ID = FirePlayCommon::PACKET_ID;
	using ERROR_CODE = FirePlayCommon::ERROR_CODE;
	using ServerInfo = FirePlayCommon::ServerInfo;

	using PacketQueue = FirePlayNetwork::PacketQueue;

	using PacketFunc = std::function<ERROR_CODE(std::shared_ptr<RecvPacketInfo>)>;
	using PacketFuncList = std::list<PacketFunc>;

	class UserManager;
	class LobbyManager;

	class PacketProcess
	{
	public :

		PacketProcess() {};
		~PacketProcess() {};

		void Init(
			ConsoleLogger * logger,
			UserManager   * userManager,
			LobbyManager  * lobbyManager,
			PacketQueue   * recvQueue,
			PacketQueue   * sendQueue,
			ServerInfo    * serverInfo);
		void BroadCast(std::shared_ptr<RecvPacketInfo> packetInfo);
		void Subscribe(short interestedPacketId, PacketFunc functor);

		void Update();

	private :

		void registFunctions();

		ERROR_CODE ntfSysConnectSession(std::shared_ptr<RecvPacketInfo> packetInfo);
		ERROR_CODE ntfSysCloseSession(std::shared_ptr<RecvPacketInfo> packetInfo);

		ERROR_CODE login(std::shared_ptr<RecvPacketInfo> packetInfo);

		ERROR_CODE lobbyList(std::shared_ptr<RecvPacketInfo> packetInfo);

		ERROR_CODE lobbyEnter(std::shared_ptr<RecvPacketInfo> packetInfo);

		ERROR_CODE lobbyRoomList(std::shared_ptr<RecvPacketInfo> packetInfo);

		ERROR_CODE lobbyUserList(std::shared_ptr<RecvPacketInfo> packetInfo);

		ERROR_CODE lobbyChat(std::shared_ptr<RecvPacketInfo> packetInfo);

		ERROR_CODE lobbyWisper(std::shared_ptr<RecvPacketInfo> packetInfo);

		ERROR_CODE lobbyLeave(std::shared_ptr<RecvPacketInfo> packetInfo);

		ERROR_CODE roomEnter(std::shared_ptr<RecvPacketInfo> packetInfo);

		ERROR_CODE roomLeave(std::shared_ptr<RecvPacketInfo> packetInfo);

		ERROR_CODE roomChat(std::shared_ptr<RecvPacketInfo> packetInfo);

		ERROR_CODE roomMasterGameStart(std::shared_ptr<RecvPacketInfo> packetInfo);

		ERROR_CODE roomGameStart(std::shared_ptr<RecvPacketInfo> packetInfo);

	private :

		ConsoleLogger * _logger = nullptr;
		PacketQueue * _recvQueue = nullptr;
		PacketQueue * _sendQueue = nullptr;
		std::unique_ptr<ConnectedUserManager> _connectedUserManager = nullptr;
		UserManager * _userManager = nullptr;
		LobbyManager * _lobbyManager = nullptr;

		std::unordered_map<short, PacketFuncList> _processMap;

	public :

		static class Factory
		{
		public:
			static PacketProcess * Create(
				ConsoleLogger * logger,
				UserManager   * userManager,
				LobbyManager  * lobbyManager,
				PacketQueue   * recvQueue,
				PacketQueue   * sendQueue)
			{
				auto product = new PacketProcess();
				if (product == nullptr)
				{
					return nullptr;
				}

				//product->Init(logger, userManager, lobbyManager, recvQueue, sendQueue);
				return product;
			}
		};
	};
}