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
		if (!initNetwork())
		{
			_logger->Write(LogType::LOG_ERROR, "%s | IOCPNetwork :: Network initialize failed", __FUNCTION__);
			return;
		}
		else
		{
			_logger->Write(LogType::LOG_DEBUG, "%s | IOCPNetwork :: Network initialize success", __FUNCTION__);
		}

		if (!startServer())
		{
			_logger->Write(LogType::LOG_ERROR, "%s | IOCPNetwork :: Network start failed", __FUNCTION__);
			return;
		}
		else
		{
			_logger->Write(LogType::LOG_DEBUG, "%s | IOCPNetwork :: Network start success", __FUNCTION__);
		}
	}

	void IOCPNetwork::Stop()
	{
		if (!endNetwork())
		{
			_logger->Write(LogType::LOG_ERROR, "%s | IOCPNetwork :: Network end failed", __FUNCTION__);
			return;
		}
		else
		{
			_logger->Write(LogType::LOG_DEBUG, "%s | IOCPNetwork :: Network end success", __FUNCTION__);
		}
	}

	bool IOCPNetwork::startServer()
	{
		// 세팅된 소켓을 listen해준다.
		auto retval = listen(_serverSocket, _serverInfo->Backlog);
		if (retval != 0)
		{
			_logger->Write(LogType::LOG_ERROR, "%s | Socket Listen Failed.", __FUNCTION__);
			return false;
		}
		
		// listen 쓰레드를 활성화한다.
		auto listenThread = std::thread(std::bind(&IOCPNetwork::listenThreadFunc, this));
		listenThread.detach();

		// 시스템 정보를 알아온다.
		SYSTEM_INFO si;
		GetSystemInfo(&si);
		int threadNum = si.dwNumberOfProcessors * 2;

		// 코어 수의 두 배 만큼 working 쓰레드를 활성화한다.
		for (int i = 0; i < threadNum; ++i)
		{
			auto workingThread = std::thread(std::bind(&IOCPNetwork::workingThreadFunc, this));
			workingThread.detach();
		}

		// send 쓰레드를 활성화 한다.
		auto sendThread = std::thread(std::bind(&IOCPNetwork::sendThreadFunc, this));
		sendThread.detach();

		return true;
	}

	bool IOCPNetwork::initNetwork()
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
				_threadVec.emplace_back(std::thread([&]() { workingThreadFunc(); }));
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

		return retval;
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

		bool retval = true;

		retval = (endWSA() && retval);

		return retval;
	}

	void IOCPNetwork::Run()
	{
	}

	void IOCPNetwork::workingThreadFunc()
	{

	}

	void IOCPNetwork::listenThreadFunc()
	{

	}

	void IOCPNetwork::sendThreadFunc()
	{

	}
	
}
