#pragma once
#include <winsock2.h>
#pragma comment(lib, "ws2_32")
#include <Windows.h>
#include <thread>
#include <vector>

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

		// Getter, Setter
		HANDLE GetIocpHandle() const { return _iocpHandle; };

	private :

		bool startServer();
		bool initNetwork();
		bool endNetwork();
		void Run();
		void workingThreadFunc();
		void listenThreadFunc();
		void sendThreadFunc();

	private :

		SOCKET _serverSocket = INVALID_SOCKET;
		HANDLE _iocpHandle = INVALID_HANDLE_VALUE;
		ConsoleLogger * _logger = nullptr;
		ServerInfo * _serverInfo = nullptr;

		using ThreadVector = std::vector<std::thread>;
		ThreadVector _threadVec;
	};
}