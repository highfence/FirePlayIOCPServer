#pragma once
#include <WinSock2.h>

namespace FirePlayNetwork
{
	const int maxIpLen = 32; // IP 문자열 최대 길이
	const int maxPacketBodySize = 1024; // 최대 패킷 보디 크기

	enum class IOCPInfoStatus : int
	{
		NONE = 0,
		READ = 1,
		WRITE = 2
	};

	struct IOCPInfo
	{
		OVERLAPPED Overlapped;
		WSABUF Wsabuf;
		IOCPInfoStatus Status = IOCPInfoStatus::NONE;
		int SessionTag = -1;
	};

	class SessionInfo
	{
	public :

		SessionInfo() {};
		~SessionInfo() {};

		bool IsConnected() { return _socket != 0 ? true : false; }

		void Clear()
		{
			_socket = 0;
			_ip[0] = '\0';
			_reServerHostingDataSize = 0;
			_prevReadPosInRecvBuffer = 0;
			_sendSize = 0;
		}

		int       _tag = 0;
		SOCKET    _socket = INVALID_SOCKET;
		char      _ip[maxIpLen] = { 0, };

		char *    _recvBuffer = nullptr;
		int       _reServerHostingDataSize = 0;
		int       _prevReadPosInRecvBuffer = 0;

		char *    _sendBuffer = nullptr;
		int       _sendSize = 0;
	};

	struct RecvPacketInfo
	{
		int SessionIndex = 0;
		short PacketId = 0;
		short PacketBodySize = 0;
		char * pData = 0;
	};
}
