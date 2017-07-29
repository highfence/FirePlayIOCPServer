#pragma once
#include <winsock2.h>
#pragma comment(lib, "ws2_32")
#include <Windows.h>
#include <thread>
#include <vector>
#include <memory>

#include "../Common/ObjectPool.h"
#include "SessionInfo.h"
#include "PacketQueue.h"

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

		// Getter, Setter
		HANDLE GetIocpHandle() const { return _iocpHandle; };

	private :

		bool initNetwork();
		bool startServer();
		bool endNetwork();

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
	};

	static class NetworkFactory
	{
	public :
		IOCPNetwork * Create(
			ConsoleLogger    * logger,
			const ServerInfo * serverInfo,
			PacketQueue      * recvPacketQueue,
			PacketQueue      * sendPacketQueue)
		{
			auto product = new IOCPNetwork();
			product->Init(logger, serverInfo, recvPacketQueue, sendPacketQueue);

			return product;
		}
	};
}