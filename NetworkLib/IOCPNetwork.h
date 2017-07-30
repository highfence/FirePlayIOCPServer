#pragma once
#include <winsock2.h>
#pragma comment(lib, "ws2_32")
#include <Windows.h>
#include <thread>
#include <vector>
#include <memory>

#include "../Common/ObjectPool.h"
#include "../Common/Define.h"

namespace FirePlayCommon
{
	enum class LogType;
	struct ServerInfo;
	class ConsoleLogger;
}

namespace FirePlayNetwork
{
	using ConsoleLogger = FirePlayCommon::ConsoleLogger;
	using LogType = FirePlayCommon::LogType;
	using ServerInfo = FirePlayCommon::ServerInfo;
	class PacketQueue;

	class IOCPNetwork
	{
	public:

		IOCPNetwork() {}
		~IOCPNetwork() {}

		void Init(
			ConsoleLogger    * logger,
			const ServerInfo * serverInfo,
			PacketQueue      * recvPacketQueue,
			PacketQueue      * sendPacketQueue);
		void Stop();

		void ForcingClose(const int sessionIdx);

		// Getter, Setter
		HANDLE GetIocpHandle() const { return _iocpHandle; };
		int GetSessionPoolSize() { return _sessionPool.GetSize(); }

	private :

		bool initNetwork();
		bool startServer();
		bool endNetwork();

		void closeSession(const FirePlayCommon::SOCKET_CLOSE_CASE closeCase, const SOCKET socket, const int sesseionIdx);
		void addPacketQueue(const int sessionIdx, const short pktId, const short bodySize, char * pDataPos);

		void workerThreadFunc();
		void listenThreadFunc();
		void sendThreadFunc();

	private :

		SOCKET          _serverSocket = INVALID_SOCKET;
		HANDLE          _iocpHandle   = INVALID_HANDLE_VALUE;
		ConsoleLogger * _logger       = nullptr;
		ServerInfo    * _serverInfo   = nullptr;

		using SessionPool = FirePlayCommon::ObjectPool<SessionInfo>;
		SessionPool _sessionPool;
		PacketQueue * _recvPacketQueue;
		PacketQueue * _sendPacketQueue;

		size_t _connectedSessionCount = 0;

	public :

		static class Factory
		{
		public:
			static IOCPNetwork * Create(
				ConsoleLogger    * logger,
				const ServerInfo * serverInfo,
				PacketQueue      * recvPacketQueue,
				PacketQueue      * sendPacketQueue)
			{
				auto product = new IOCPNetwork();
				if (product == nullptr)
				{
					return nullptr;
				}

				product->Init(logger, serverInfo, recvPacketQueue, sendPacketQueue);
				return product;
			}
		};
	};
}