#pragma once
#include <memory>
#include <unordered_map>
#include <functional>
#include <list>

#include "../Common/PacketID.h"
#include "../Common/ErrorCode.h"
#include "../Common/Define.h"

namespace FirePlayCommon
{
	class ConsoleLogger;
}

namespace FirePlayNetwork
{
	class PacketQueue;
}

namespace FirePlayLogic
{
	using ConsoleLogger = FirePlayCommon::ConsoleLogger;
	using RecvPacketInfo = FirePlayCommon::RecvPacketInfo;
	using PACKET_ID = FirePlayCommon::PACKET_ID;
	using ERROR_CODE = FirePlayCommon::ERROR_CODE;

	using PacketQueue = FirePlayNetwork::PacketQueue;

	using PacketFunc = std::function<ERROR_CODE(std::shared_ptr<RecvPacketInfo>)>;
	using PacketFuncList = std::list<PacketFunc>;

	class ConnectedUserManager;


	class PacketProcess
	{
	public :

		PacketProcess() {};
		~PacketProcess() {};

		void Init(ConsoleLogger * logger, PacketQueue * recvQueue, PacketQueue * sendQueue);
		void BroadCast(std::shared_ptr<RecvPacketInfo> packetInfo);
		void Subscribe(short interestedPacketId, PacketFunc functor);

		void Update();

	private :
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

		std::unordered_map<short, PacketFuncList> _processMap;

	public :

		static class Factory
		{
		public:
			static PacketProcess * Create(ConsoleLogger * logger, PacketQueue * recvQueue, PacketQueue * sendQueue)
			{
				auto product = new PacketProcess();
				if (product == nullptr)
				{
					return nullptr;
				}

				product->Init(logger, recvQueue, sendQueue);
				return product;
			}
		};
	};
}