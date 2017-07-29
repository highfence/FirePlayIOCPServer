#include "IOCPNetwork.h"

#include <thread>
#include <WinSock2.h>
#pragma comment(lib, "ws2_32")

#include "../Common/ConsoleLogger.h"
#include "../Common/Define.h"

namespace FirePlayNetwork
{
	IOCPNetwork::IOCPNetwork(ConsoleLogger * logger, const ServerInfo * serverInfo)
	{
		_logger = logger;
		memcpy_s(_serverInfo, sizeof(ServerInfo), serverInfo, sizeof(ServerInfo));
	}

	void IOCPNetwork::Init()
	{

	}

	void IOCPNetwork::Stop()
	{

	}

	bool IOCPNetwork::startNetwork()
	{
#pragma region Start Network Functions

		auto initWSA = [this]() -> bool
		{
			WSADATA wsaData;
			if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
			{
				_logger->Write(LogType::LOG_ERROR, "%s | WSAStartUp Failed", __FUNCTION__);

				return false;
			}
			return true;
		};

		auto createIOCP = [this]() -> bool
		{
			_iocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
			if (_iocpHandle == INVALID_HANDLE_VALUE)
			{
				_logger->Write(LogType::LOG_ERROR, "%s | Iocp Creation Failed", __FUNCTION__);
				return false;
			}
			return true;
		};

		auto createWorkingThreads = [this]() -> bool
		{
			SYSTEM_INFO si;
			GetSystemInfo(&si);

			for (int i = 0; i < static_cast<int>(si.dwNumberOfProcessors * 2); ++i)
			{
				_threadVec.emplace_back(std::thread([&]() { workingThread(); }));
			}
			return true;
		};

		auto createListenSocket = [this]() -> bool
		{
			_serverSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
			if (_serverSocket == INVALID_SOCKET)
			{
				_logger->Write(LogType::LOG_ERROR, "%s | Listen Socket Initialize Failed", __FUNCTION__);
				return false;
			}
			return true;
		};

		auto bindSocket = [this]() -> bool
		{
			SOCKADDR_IN socketAddr;
			ZeroMemory(&socketAddr, sizeof(socketAddr));
			socketAddr.sin_family = AF_INET;
			socketAddr.sin_addr.s_addr = htonl(INADDR_ANY);
			socketAddr.sin_port = htons(_serverInfo->Port);

			auto retval = bind(_serverSocket, (sockaddr*)&socketAddr, sizeof(socketAddr));
			if (retval != 0)
			{
				_logger->Write(LogType::LOG_ERROR, "%s | Socket Address Bind Failed", __FUNCTION__);
				return false;
			}
			return true;
		};

#pragma endregion

		bool retval = true;

		retval = (initWSA()              && retval);
		retval = (createIOCP()           && retval);
		retval = (createWorkingThreads() && retval);
		retval = (createListenSocket()   && retval);
		retval = (bindSocket()           && retval);

		if (retval == true)
		{
			_logger->Write(LogType::LOG_DEBUG, "%s | IOCPNetwork :: Network start success", __FUNCTION__);
			return true;
		}

		return false;
	}

	bool IOCPNetwork::endNetwork()
	{
#pragma region End Network Functions

		auto endWSA = []() -> bool
		{
			if (WSACleanup() == SOCKET_ERROR)
			{
				return false;
			}
			return true;
		};

#pragma endregion

		endWSA();
	}

	void IOCPNetwork::Run()
	{
	}

	void IOCPNetwork::workingThread()
	{
	}
	
}
