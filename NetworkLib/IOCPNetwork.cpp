#include "IOCPNetwork.h"

#include <memory>
#include <thread>
#include <WinSock2.h>
#include <chrono>
#pragma comment(lib, "ws2_32")

#include "../Common/ConsoleLogger.h"
#include "../Common/Define.h"
#include "../Common/Packet.h"
#include "SessionInfo.h"
#include "PacketQueue.h"

using PktHeader = FirePlayCommon::PktHeader;

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
		// ���õ� ������ listen���ش�.
		auto retval = listen(_serverSocket, _serverInfo->Backlog);
		if (retval != 0)
		{
			_logger->Write(LogType::LOG_ERROR, "%s | Socket Listen Failed.", __FUNCTION__);
			return false;
		}
		
		// listen �����带 Ȱ��ȭ�Ѵ�.
		auto listenThread = std::thread(std::bind(&IOCPNetwork::listenThreadFunc, this));
		listenThread.detach();

		// �ý��� ������ �˾ƿ´�.
		SYSTEM_INFO si;
		GetSystemInfo(&si);
		int threadNum = si.dwNumberOfProcessors * 2;

		// �ھ� ���� �� �� ��ŭ working �����带 Ȱ��ȭ�Ѵ�.
		for (int i = 0; i < threadNum; ++i)
		{
			auto workingThread = std::thread(std::bind(&IOCPNetwork::workerThreadFunc, this));
			workingThread.detach();
		}

		// send �����带 Ȱ��ȭ �Ѵ�.
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

	void IOCPNetwork::workerThreadFunc()
	{
		DWORD transferredByte = 0;
		IO���� * ioInfo		  = nullptr;
		// Key�� �Ѿ� �´ٰ� �ϴµ�, ���� �𸣰ڰ� �Ⱦ�. ���߿� �˻��غ���¡. :)
		SessionInfo * key     = nullptr;

		while (true)
		{
			auto retval = GetQueuedCompletionStatus(_iocpHandle, &transferredByte, (PULONG_PTR)&key, (LPOVERLAPPED*)&ioInfo, INFINITE);
			if (retval == FALSE)
			{
				_logger->Write(LogType::LOG_ERROR, "%s | Iocp GetQueuedCompletionStatus Failed", __FUNCTION__);
				continue;
			}

			auto sessionTag = ioInfo->SessionTag;
			SessionInfo session = _sessionPool[sessionTag];

			if (ioInfo->Status == IOCPInfoStatus::READ)
			{
				// ���� �˻�.
				if (transferredByte == 0)
				{
					session.Clear();
					_sessionPool.ReleaseTag(sessionTag);
					// TODO :: ����� ���� ��Ŷ ������ ��Ŷ ť�� �־��ֱ�.
					_logger->Write(LogType::LOG_INFO, "Session idx %d connect ended", sessionTag);
					continue;
				}

				auto headerPosition = session._recvBuffer;
				auto receivePosition = ioInfo->Wsabuf.buf;

				// ó���ȵ� �������� �� ��
				auto totalDataSize = receivePosition + transferredByte - session._recvBuffer;

				// ��Ŷ���� ��������� ��ٸ��� �������� ������
				auto remainDataSize = totalDataSize;

				const auto packetHeaderSize = FirePlayCommon::packetHeaderSize;
				while (remainDataSize >= packetHeaderSize)
				{
					// ����� �鿩�� ���⿡ ����� �����Ͱ� �ִٸ� ����� �鿩�ٺ���.
					auto header = (PktHeader*)headerPosition;
					auto bodySize = header->BodySize;

					if (packetHeaderSize + bodySize >= remainDataSize)
					{
						// ��Ŷ�� ������ش�.
						auto newPacket = std::make_shared<RecvPacketInfo>();
						newPacket->PacketId = header->Id;
						newPacket->PacketBodySize = bodySize;
						newPacket->pData = headerPosition + packetHeaderSize;
						newPacket->SessionIndex = ioInfo->SessionTag;

						_recvPacketQueue.Push(newPacket);

						// ��Ŷ�� ���� ��, ���� �� ��� �ڸ��� �����ϰ�, ���� ������ ����� �����Ѵ�.
						headerPosition += packetHeaderSize + bodySize;
						remainDataSize -= packetHeaderSize + bodySize;
					}
					else
					{
						break;
					}
				}

				// ���� �����͸� ������ �� ������ ����ش�.
				memcpy_s(session._recvBuffer, _serverInfo->MaxSessionRecvBufferSize, headerPosition, remainDataSize);

				// ���� �� �ִ� ��Ŷ�� �� ��������Ƿ�, Recv�� �Ǵ�.
				ZeroMemory(&ioInfo->Overlapped, sizeof(OVERLAPPED));

				// remainDataSize��ŭ�� ��� �޴´�.
				ioInfo->Wsabuf.buf = session._recvBuffer + remainDataSize;
				ioInfo->Wsabuf.len = _serverInfo->MaxSessionRecvBufferSize - remainDataSize;
				ioInfo->Status = IOCPInfoStatus::READ;

				DWORD recvSize = 0;
				DWORD flags = 0;
				WSARecv(session._socket, &ioInfo->Wsabuf, 1, &recvSize, &flags, &ioInfo->Overlapped, nullptr);
			}
			// TODO 
			else
			{

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

			// TODO :: ������ �ʰ����� ��� ó��.

			// Ǯ���� Session �ϳ��� �޾� ������ �������ش�.
			auto newTag = _sessionPool.GetTag();
			if (newTag < 0)
			{
				_logger->Write(LogType::LOG_WARN, "%s | Client Session Pool", __FUNCTION__);
				continue;
			}

			auto newSession = _sessionPool[newTag];
			newSession._tag = newTag;
			newSession._socket = newClient;
			
			auto newIOCPInfo = new IO����();
			ZeroMemory(&newIOCPInfo->Overlapped, sizeof(OVERLAPPED));
			newIOCPInfo->Wsabuf.buf = newSession._recvBuffer;
			newIOCPInfo->Wsabuf.len = _serverInfo->MaxSessionRecvBufferSize;
			newIOCPInfo->Status = IOCPInfoStatus::READ;
			newIOCPInfo->SessionTag = newTag;

			// IOCP�� ���ο� ������ ������ش�.
			BindSessionToIOCP(&newSession);

			DWORD recvSize = 0;
			DWORD flags = 0;

			// ���ú긦 �ɾ���´�.
			WSARecv(newSession._socket, &newIOCPInfo->Wsabuf, 1, &recvSize, &flags, &newIOCPInfo->Overlapped, NULL);
		}
	}

	void IOCPNetwork::sendThreadFunc()
	{
		while (true)
		{
			if (_sendPacketQueue.IsEmpty())
			{
				// �纸�Ѵ�.
				std::this_thread::sleep_for(std::chrono::milliseconds(0));
				continue;
			}

			auto sendPacket = _sendPacketQueue.Peek();
			auto destSession = _sessionPool[sendPacket->SessionIndex];
			auto sendHeader = PktHeader{ sendPacket->PacketId, sendPacket->PacketBodySize };
			
			send(destSession._socket, (char*)&sendHeader, FirePlayCommon::packetHeaderSize, 0);
			send(destSession._socket, sendPacket->pData, sendPacket->PacketBodySize, 0);
			_sendPacketQueue.Pop();
		}
	}
}
