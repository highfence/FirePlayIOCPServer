
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

		// 클라이언트 세션 풀을 초기화.
		auto sessionPoolInitialize = [this]()
		{
			_sessionPool.Init(_serverInfo.MaxClientCount);

			// 오브젝트 풀에 할당된 Session마다 설정.
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

		// 네트워크단을 세팅한다.
		if (!initNetwork())
		{
			_logger->Write(LogType::LOG_ERROR, "%s | IOCPNetwork :: Network initialize failed", __FUNCTION__);
			return;
		}
		else
		{
			_logger->Write(LogType::LOG_DEBUG, "%s | IOCPNetwork :: Network initialize success", __FUNCTION__);
		}	

		// 쓰레드들을 활성화해준다.
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

		// Object풀 클래스 초기화.
		_sessionPool.Release();
	}

	void IOCPNetwork::ForcingClose(const int sessionIdx)
	{
		// 닫으려는 세션이 활성화 되어 있지 않은 상태면 그냥 리턴한다.
		if (_sessionPool[sessionIdx].IsConnected() == false)
		{
			return;
		}

		closeSession(FirePlayCommon::SOCKET_CLOSE_CASE::FORCING_CLOSE, static_cast<SOCKET>(_sessionPool[sessionIdx]._socket), sessionIdx);
	}

	bool IOCPNetwork::initNetwork()
	{
#pragma region Start Network Functions

		// WSA 소켓을 활성화 한다.
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

		// Io Completion Port를 생성한다.
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

		// Listen 소켓을 생성한다.
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

		// Socket에 설정을 바인딩한다.
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
		// 세팅된 소켓을 listen해준다.
		//auto retval = listen(_serverSocket, SOMAXCONN);
		auto retval = listen(_serverSocket, _serverInfo.Backlog);
		if (retval != 0)
		{
			_logger->Write(LogType::LOG_ERROR, "%s | Socket Listen Failed.", __FUNCTION__);
			return false;
		}
		_logger->Write(LogType::LOG_INFO, "%s | Listen. ServerSocketFd(%I64u), BackLog(%d)", __FUNCTION__, _serverSocket, _serverInfo.Backlog);
		
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
			auto workingThread = std::thread(std::bind(&IOCPNetwork::workerThreadFunc, this));
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

	// 지정한 세션을 닫아주는 함수.
	void IOCPNetwork::closeSession(const FirePlayCommon::SOCKET_CLOSE_CASE closeCase, const SOCKET socket, const int sesseionIdx)
	{
		// 세션 풀이 비어서 종료하는 경우에는 Session을 따로 설정해 줄 필요 없이 closesocket만 호출.
		if (closeCase == FirePlayCommon::SOCKET_CLOSE_CASE::SESSION_POOL_EMPTY)
		{
			closesocket(socket);
			return;
		}

		// 지정한 세션이 활성화 상태가 아니라면 입력값 오류로 보고 바로 리턴.
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
		// Key가 넘어 온다고 하는데, 뭔지 모르겠고 안씀. 나중에 검색해봐야징. :)
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
				// 종료 검사.
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

				// 처리안된 데이터의 총 량
				auto totalDataSize = receivePosition + transferredByte - session._recvBuffer;

				// 패킷으로 만들어지길 기다리는 데이터의 사이즈
				auto remainDataSize = totalDataSize;

				const auto packetHeaderSize = FirePlayCommon::packetHeaderSize;
				while (remainDataSize >= packetHeaderSize)
				{
					// 헤더를 들여다 보기에 충분한 데이터가 있다면 헤더를 들여다본다.
					auto header = (PktHeader*)headerPosition;
					auto bodySize = header->BodySize;

					if (packetHeaderSize + bodySize >= remainDataSize)
					{
						// 패킷을 만들어준다.
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

						// 패킷을 만든 후, 다음 번 헤더 자리를 지정하고, 남은 데이터 사이즈를 갱신한다.
						headerPosition += packetHeaderSize + bodySize;
						remainDataSize -= packetHeaderSize + bodySize;
					}
					else
					{
						break;
					}
				}

				// 남은 데이터를 버퍼의 맨 앞으로 당겨준다.
				memcpy_s(session._recvBuffer, _serverInfo.MaxClientRecvBufferSize, headerPosition, remainDataSize);

				// 만들 수 있는 패킷은 다 만들었으므로, Recv를 건다.
				ZeroMemory(&ioInfo->Overlapped, sizeof(OVERLAPPED));

				// remainDataSize만큼은 띄고 받는다.
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
			// TODO :: IOInfoStatus::WRITE일 때 처리.
			else {}
		}
	}

	// 새로운 접속자를 기다리고, 접속자를 세션에 등록해주는 쓰레드 함수.
	void IOCPNetwork::listenThreadFunc()
	{
#pragma region IOCP Function

		// 만들어진 Io Completion Port에 들어온 소켓을 정보에 묶는다.
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

			// 여기서 Blocking되어 다른 클라이언트의 accept를 기다린다.
			SOCKET newClient = accept(_serverSocket, (SOCKADDR*)&clientAddr, &addrlen);
			if (newClient == INVALID_SOCKET)
			{
				_logger->Write(LogType::LOG_ERROR, "%s | Client accpet failed", __FUNCTION__);
				continue;
			}

			// 풀에서 Session 하나를 받아 정보를 기입해준다.
			auto newTag = _sessionPool.GetTag();
			if (newTag < 0)
			{
				_logger->Write(LogType::LOG_WARN, "%s | Client Session Pool Full", __FUNCTION__);
				// TODO :: 여기서 continue말고 동접자 최대일 경우 처리해주어야 함.
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

			// IOCP에 새로운 세션을 등록해준다.
			bindSessionToIOCP(&newSession);
			++_connectedSessionCount;

			DWORD recvSize = 0;
			DWORD flags = 0;

			// 리시브를 걸어놓는다.
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

	// sendPacketQueue를 들여다보며 보낼 패킷이 있으면 보내주는 쓰레드 함수.
	void IOCPNetwork::sendThreadFunc()
	{
		while (true)
		{
			// 보낼 패킷이 없다면,
			if (_sendPacketQueue->IsEmpty())
			{
				// 양보한다.
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
