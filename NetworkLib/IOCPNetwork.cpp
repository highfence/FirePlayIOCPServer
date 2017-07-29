#include "IOCPNetwork.h"

#include <thread>
#include <WinSock2.h>
#pragma comment(lib, "ws2_32")

#include "../Common/ConsoleLogger.h"
#include "../Common/Define.h"
#include "SessionInfo.h"
#include "PacketQueue.h"

namespace FirePlayNetwork
{
	IOCPNetwork::IOCPNetwork(ConsoleLogger * logger, const ServerInfo * serverInfo)
	{
		_logger = logger;
		memcpy_s(_serverInfo, sizeof(ServerInfo), serverInfo, sizeof(ServerInfo));
		_sessionPool.Init(_serverInfo->Backlog);
		_logger->Write(LogType::LOG_INFO, "IOCPNetwork Create :: Port -> %d, Backlog -> %d", _serverInfo->Port, _serverInfo->Backlog);
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

	void IOCPNetwork::Run()
	{
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
		retval = (createListenSocket()   && retval);
		retval = (bindSocket()           && retval);

		return retval;
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

	void IOCPNetwork::workingThreadFunc()
	{
		DWORD transferredByte = 0;
		IOCPInfo * iocpInfo   = nullptr;
		SessionInfo * session = nullptr;

		while (true)
		{
			auto retval = GetQueuedCompletionStatus(_iocpHandle, &transferredByte, (PULONG_PTR)&session, (LPOVERLAPPED*)&iocpInfo, INFINITE);
			if (retval == FALSE)
			{
				_logger->Write(LogType::LOG_ERROR, "%s | Iocp GetQueuedCompletionStatus Failed", __FUNCTION__);
				continue;
			}

			auto sessionTag = iocpInfo->SessionTag;
			SessionInfo session = _sessionPool[sessionTag];

			// 종료 검사.
			if (transferredByte == 0)
			{
				session.Clear();
				_sessionPool.ReleaseTag(sessionTag);
				// TODO :: 사용자 종료 패킷 조제후 패킷 큐에 넣어주기.
				_logger->Write(LogType::LOG_INFO, "Session idx %d connect ended", sessionTag);
				continue;
			}

			

		}
	}

	void IOCPNetwork::listenThreadFunc()
	{
#pragma region IOCP Function

		auto BindSessionToIOCP = [this](SessionInfo* bindingSession)
		{
			CreateIoCompletionPort((HANDLE)bindingSession->_socket, _iocpHandle, (ULONG_PTR)nullptr, 0);
		};

#pragma endregion

		while (true)
		{
			SOCKADDR_IN clientAddr;
			ZeroMemory(&clientAddr, sizeof(clientAddr));
			int addrlen = sizeof(clientAddr);

			auto newClient = accept(_serverSocket, (SOCKADDR*)&clientAddr, &addrlen);
			if (newClient == INVALID_SOCKET)
			{
				_logger->Write(LogType::LOG_ERROR, "%s | Client accpet failed", __FUNCTION__);
				continue;
			}

			// TODO :: 동접자 초과했을 경우 처리.

			// 풀에서 Session 하나를 받아 정보를 기입해준다.
			auto newTag = _sessionPool.GetTag();
			if (newTag < 0)
			{
				_logger->Write(LogType::LOG_WARN, "%s | Client Session Pool", __FUNCTION__);
				continue;
			}

			auto newSession = _sessionPool[newTag];
			newSession._tag = newTag;
			newSession._socket = newClient;
			
			auto newIOCPInfo = new IOCPInfo();
			ZeroMemory(&newIOCPInfo->Overlapped, sizeof(OVERLAPPED));
			newIOCPInfo->Wsabuf.buf = newSession._recvBuffer;
			newIOCPInfo->Wsabuf.len = _serverInfo->MaxSessionRecvBufferSize;
			newIOCPInfo->Status = IOCPInfoStatus::READ;
			newIOCPInfo->SessionTag = newTag;

			// IOCP에 새로운 세션을 등록해준다.
			BindSessionToIOCP(&newSession);

			DWORD recvSize = 0;
			DWORD flags = 0;

			// 리시브를 걸어놓는다.
			WSARecv(newSession._socket, &newIOCPInfo->Wsabuf, 1, &recvSize, &flags, &newIOCPInfo->Overlapped, NULL);
		}
	}

	void IOCPNetwork::sendThreadFunc()
	{

	}
	
}
