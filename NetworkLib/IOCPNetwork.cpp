
#include "IOCPNetwork.h"

#include <memory>
#include <thread>
#include <chrono>

#include "../Common/ConsoleLogger.h"
#include "../Common/Define.h"
#include "../Common/Packet.h"
#include "../Common/ObjectPool.h"
#include "../Common/PacketID.h"
#include "ServerNetErrorCode.h"

#include "ServerInfo.h"
#include "PacketQueue.h"
#include "ServerNetErrorCode.h"

using PktHeader = FirePlayCommon::PktHeader;
using RecvPacketInfo = FirePlayCommon::RecvPacketInfo;
using PACKET_ID = FirePlayCommon::PACKET_ID;

namespace FirePlayNetwork
{
	void IOCPNetwork::Init(
		ConsoleLogger    * logger,
		ServerInfo		 * serverInfo,
		PacketQueue      * recvPacketQueue,
		PacketQueue      * sendPacketQueue)
	{
#pragma region Session Initialize Function

		// Ŭ���̾�Ʈ ���� Ǯ�� �ʱ�ȭ.
		auto sessionPoolInitialize = [this]()
		{
			_sessionPool.Init(_serverInfo.MaxClientCount);

			// ������Ʈ Ǯ�� �Ҵ�� Session���� ����.
			for (int i = 0; i < _serverInfo.MaxClientCount; ++i)
			{
				_sessionPool[i]._tag = i;
				_sessionPool[i]._recvBuffer = new char[_serverInfo.MaxClientRecvBufferSize];
			}
		};

#pragma endregion

		if (logger == nullptr || serverInfo == nullptr || recvPacketQueue == nullptr || sendPacketQueue == nullptr)
		{
			return;
		}

		_logger = logger;
		_recvPacketQueue = recvPacketQueue;
		_sendPacketQueue = sendPacketQueue;
		memcpy(&_serverInfo, serverInfo, sizeof(ServerInfo));

		sessionPoolInitialize();

		_logger->Write(LogType::LOG_INFO, "IOCPNetwork Create :: Port(%d), Backlog(%d)", _serverInfo.Port, _serverInfo.Backlog);

		// ��Ʈ��ũ���� �����Ѵ�.
		if (!initNetwork())
		{
			_logger->Write(LogType::LOG_ERROR, "%s | IOCPNetwork :: Network initialize failed", __FUNCTION__);
			return;
		}
		else
		{
			_logger->Write(LogType::LOG_DEBUG, "%s | IOCPNetwork :: Network initialize success", __FUNCTION__);
		}	

		// ��������� Ȱ��ȭ���ش�.
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

		// ObjectǮ Ŭ���� �ʱ�ȭ.
		_sessionPool.Release();
	}

	void IOCPNetwork::ForcingClose(const int sessionIdx)
	{
		// �������� ������ Ȱ��ȭ �Ǿ� ���� ���� ���¸� �׳� �����Ѵ�.
		if (_sessionPool[sessionIdx].IsConnected() == false)
		{
			return;
		}

		closeSession(FirePlayCommon::SOCKET_CLOSE_CASE::FORCING_CLOSE, static_cast<SOCKET>(_sessionPool[sessionIdx]._socket), sessionIdx);
	}

	bool IOCPNetwork::initNetwork()
	{
#pragma region Start Network Functions

		// WSA ������ Ȱ��ȭ �Ѵ�.
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

		// Io Completion Port�� �����Ѵ�.
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

		// Listen ������ �����Ѵ�.
		auto createListenSocket = [this]() -> bool
		{
			_serverSocket = WSASocket(AF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);
			if (_serverSocket == INVALID_SOCKET)
			{
				_logger->Write(LogType::LOG_ERROR, "%s | Listen Socket Initialize Failed", __FUNCTION__);
				return false;
			}
			return true;
		};

		// Socket�� ������ ���ε��Ѵ�.
		auto bindSocket = [this]() -> bool
		{
			SOCKADDR_IN socketAddr;
			ZeroMemory(&socketAddr, sizeof(socketAddr));
			socketAddr.sin_family = AF_INET;
			socketAddr.sin_addr.s_addr = htonl(INADDR_ANY);
			socketAddr.sin_port = htons(_serverInfo.Port);

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

		retval = (createIOCP()           && retval);
		retval = (initWSA()              && retval);
		retval = (createListenSocket()   && retval);
		retval = (bindSocket()           && retval);

		return retval;
	}

	bool IOCPNetwork::startServer()
	{
		// ���õ� ������ listen���ش�.
		//auto retval = listen(_serverSocket, SOMAXCONN);
		auto retval = listen(_serverSocket, _serverInfo.Backlog);
		if (retval != 0)
		{
			_logger->Write(LogType::LOG_ERROR, "%s | Socket Listen Failed.", __FUNCTION__);
			return false;
		}
		_logger->Write(LogType::LOG_INFO, "%s | Listen. ServerSocketFd(%I64u), BackLog(%d)", __FUNCTION__, _serverSocket, _serverInfo.Backlog);
		
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

	// ������ ������ �ݾ��ִ� �Լ�.
	void IOCPNetwork::closeSession(const FirePlayCommon::SOCKET_CLOSE_CASE closeCase, const SOCKET socket, const int sesseionIdx)
	{
		// ���� Ǯ�� �� �����ϴ� ��쿡�� Session�� ���� ������ �� �ʿ� ���� closesocket�� ȣ��.
		if (closeCase == FirePlayCommon::SOCKET_CLOSE_CASE::SESSION_POOL_EMPTY)
		{
			closesocket(socket);
			return;
		}

		// ������ ������ Ȱ��ȭ ���°� �ƴ϶�� �Է°� ������ ���� �ٷ� ����.
		if (_sessionPool[sesseionIdx].IsConnected() == false)
		{
			return;
		}

		closesocket(socket);
		_sessionPool[sesseionIdx].Clear();
		--_connectedSessionCount;
		_sessionPool.ReleaseTag(sesseionIdx);

		addPacketQueue(sesseionIdx, (short)FirePlayNetwork::NET_ERROR_CODE::NTF_SYS_CLOSE_SESSION, 0, nullptr);
	}

	void IOCPNetwork::addPacketQueue(const int sessionIdx, const short pktId, const short bodySize, char * pDataPos)
	{
		auto packetInfo = std::make_shared<RecvPacketInfo>();
		packetInfo->SessionIndex = sessionIdx;
		packetInfo->PacketId = pktId;
		packetInfo->PacketBodySize = bodySize;
		packetInfo->pData = pDataPos;

		_recvPacketQueue->Push(packetInfo);
	}

	void IOCPNetwork::workerThreadFunc()
	{
		DWORD transferredByte = 0;
		IOInfo * ioInfo		  = nullptr;
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
			auto& session = _sessionPool[sessionTag];
		
			_logger->Write(FirePlayCommon::LogType::LOG_DEBUG, "%s | Socket FD(%I64u), Session(%d) request complete", __FUNCTION__, session._socket, sessionTag);

			if (ioInfo->Status == IOInfoStatus::READ)
			{
				// ���� �˻�.
				if (transferredByte == 0)
				{
					_logger->Write(LogType::LOG_INFO, "Socket FD(%I64u), Session(%d) connect ended", session._socket, sessionTag);
					session.Clear();
					_sessionPool.ReleaseTag(sessionTag);

					std::shared_ptr<RecvPacketInfo> closeSessionInfo = std::make_shared<RecvPacketInfo>();
					closeSessionInfo->PacketId = (short)FirePlayNetwork::NET_ERROR_CODE::NTF_SYS_CLOSE_SESSION;
					closeSessionInfo->SessionIndex = sessionTag;
					_recvPacketQueue->Push(closeSessionInfo);

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
						std::shared_ptr<RecvPacketInfo> newPacket = std::make_shared<RecvPacketInfo>();
						newPacket->PacketId = header->Id;
						newPacket->PacketBodySize = bodySize;
						newPacket->pData = headerPosition + packetHeaderSize;
						newPacket->SessionIndex = ioInfo->SessionTag;

						_recvPacketQueue->Push(newPacket);
						_logger->Write(LogType::LOG_DEBUG, "%s | Making New Packet ID(%d), BodySize(%d), Session(%d)",
							__FUNCTION__,
							header->Id,
							bodySize,
							ioInfo->SessionTag);

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
				memcpy_s(session._recvBuffer, _serverInfo.MaxClientRecvBufferSize, headerPosition, remainDataSize);

				// ���� �� �ִ� ��Ŷ�� �� ��������Ƿ�, Recv�� �Ǵ�.
				ZeroMemory(&ioInfo->Overlapped, sizeof(OVERLAPPED));

				// remainDataSize��ŭ�� ��� �޴´�.
				ioInfo->Wsabuf.buf = session._recvBuffer + remainDataSize;
				ioInfo->Wsabuf.len = _serverInfo.MaxClientRecvBufferSize - remainDataSize;
				ioInfo->Status = IOInfoStatus::READ;

				DWORD recvSize = 0;
				DWORD flags = 0;
				auto retval = WSARecv(session._socket, &ioInfo->Wsabuf, 1, &recvSize, &flags, &ioInfo->Overlapped, nullptr);

				if (SOCKET_ERROR == retval)
				{
					auto error = WSAGetLastError();
					if (error != WSA_IO_PENDING)
					{
						_logger->Write(LogType::LOG_ERROR, "%s | WSARecv Error(%d)", __FUNCTION__, error);
					}
				}
			}
			// TODO :: IOInfoStatus::WRITE�� �� ó��.
			else {}
		}
	}

	// ���ο� �����ڸ� ��ٸ���, �����ڸ� ���ǿ� ������ִ� ������ �Լ�.
	void IOCPNetwork::listenThreadFunc()
	{
#pragma region IOCP Function

		// ������� Io Completion Port�� ���� ������ ������ ���´�.
		auto bindSessionToIOCP = [this](SessionInfo* bindingSession)
		{
			CreateIoCompletionPort((HANDLE)bindingSession->_socket, _iocpHandle, (ULONG_PTR)nullptr, 0);
		};

#pragma endregion

		while (true)
		{
			SOCKADDR_IN clientAddr;
			int addrlen = sizeof(clientAddr);

			_logger->Write(LogType::LOG_DEBUG, "%s | Waiting For Other Client...", __FUNCTION__);

			// ���⼭ Blocking�Ǿ� �ٸ� Ŭ���̾�Ʈ�� accept�� ��ٸ���.
			SOCKET newClient = accept(_serverSocket, (SOCKADDR*)&clientAddr, &addrlen);
			if (newClient == INVALID_SOCKET)
			{
				_logger->Write(LogType::LOG_ERROR, "%s | Client accpet failed", __FUNCTION__);
				continue;
			}

			// Ǯ���� Session �ϳ��� �޾� ������ �������ش�.
			auto newTag = _sessionPool.GetTag();
			if (newTag < 0)
			{
				_logger->Write(LogType::LOG_WARN, "%s | Client Session Pool Full", __FUNCTION__);
				// TODO :: ���⼭ continue���� ������ �ִ��� ��� ó�����־�� ��.
				continue;
			}

			_logger->Write(LogType::LOG_INFO, "%s | Client Accept, Socket FD(%I64u) Session(%d)", __FUNCTION__, _serverSocket, newTag);

			auto& newSession = _sessionPool[newTag];
			newSession._tag = newTag;
			newSession._socket = newClient;
			newSession._socketAddress = clientAddr;
			
			auto newIOCPInfo = new IOInfo();
			ZeroMemory(&newIOCPInfo->Overlapped, sizeof(OVERLAPPED));
			newIOCPInfo->Wsabuf.buf = newSession._recvBuffer;
			newIOCPInfo->Wsabuf.len = _serverInfo.MaxClientRecvBufferSize;
			newIOCPInfo->Status = IOInfoStatus::READ;
			newIOCPInfo->SessionTag = newTag;

			// IOCP�� ���ο� ������ ������ش�.
			bindSessionToIOCP(&newSession);
			++_connectedSessionCount;

			DWORD recvSize = 0;
			DWORD flags = 0;

			// ���ú긦 �ɾ���´�.
			auto retval = WSARecv(
				newSession._socket,
				&newIOCPInfo->Wsabuf,
				1,
				&recvSize, &flags, &newIOCPInfo->Overlapped, nullptr);
			_logger->Write(LogType::LOG_DEBUG, "%s | Waiting for recv massage from socket(%I64u)", __FUNCTION__, newSession._socket);

			if (SOCKET_ERROR == retval)
			{
				auto error = WSAGetLastError();
				if (error != WSA_IO_PENDING)
				{
					_logger->Write(LogType::LOG_ERROR, "%s | WSARecv Error(%d)", __FUNCTION__, error);
				}
			}
		}
	}

	// sendPacketQueue�� �鿩�ٺ��� ���� ��Ŷ�� ������ �����ִ� ������ �Լ�.
	void IOCPNetwork::sendThreadFunc()
	{
		while (true)
		{
			// ���� ��Ŷ�� ���ٸ�,
			if (_sendPacketQueue->IsEmpty())
			{
				// �纸�Ѵ�.
				std::this_thread::sleep_for(std::chrono::milliseconds(0));
				continue;
			}

			_logger->Write(LogType::LOG_DEBUG, "%s | SendThreadFunc Entry", __FUNCTION__);

			std::shared_ptr<RecvPacketInfo> sendPacket = _sendPacketQueue->Peek();
			auto destSession = _sessionPool[sendPacket->SessionIndex];
			auto sendHeader = PktHeader{ sendPacket->PacketId, sendPacket->PacketBodySize };
			
			char* sendChar = (char*)&sendHeader; 
			strcat(sendChar, sendPacket->pData);

			send(destSession._socket, sendChar, FirePlayCommon::packetHeaderSize + sendPacket->PacketBodySize, 0);

			//send(destSession._socket, (char*)&sendHeader, FirePlayCommon::packetHeaderSize, 0);
			//send(destSession._socket, sendPacket->pData, sendPacket->PacketBodySize, 0);

			_sendPacketQueue->Pop();
			
			_logger->Write(LogType::LOG_DEBUG, "%s | Send Packet, To Socket(%I64u), Session(%d), Packet ID(%d)", __FUNCTION__, destSession._socket, destSession._tag, static_cast<int>(sendPacket->PacketId));
		}
	}
}
