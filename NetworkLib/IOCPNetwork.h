#pragma once
#include <winsock2.h>
#pragma comment(lib, "ws2_32")
#include <Windows.h>
#include <thread>
#include <vector>
#include <memory>

#include "../Common/ObjectPool.h"
#include "SessionInfo.h"

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

		IOCPNetwork(ConsoleLogger * logger, const ServerInfo * serverInfo);
		IOCPNetwork() = delete;
		~IOCPNetwork() {};

		void Init();
		void Stop();
		void Run();

		// Getter, Setter
		HANDLE GetIocpHandle() const { return _iocpHandle; };

	private :

		bool initNetwork();
		bool startServer();
		bool endNetwork();

		void workingThreadFunc();
		void listenThreadFunc();
		void sendThreadFunc();

	private :

		SOCKET          _serverSocket = INVALID_SOCKET;
		HANDLE          _iocpHandle   = INVALID_HANDLE_VALUE;
		ConsoleLogger * _logger       = nullptr;
		ServerInfo    * _serverInfo   = nullptr;

		using SessionPool = FirePlayCommon::ObjectPool<SessionInfo>;
		SessionPool _sessionPool;
	};
}