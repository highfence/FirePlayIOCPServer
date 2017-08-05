#pragma once

namespace FirePlayCommon
{
	class ServerInfo
	{
	public :

		ServerInfo() {};
		~ServerInfo() {};

		unsigned short Port = 0;
		int Backlog = 0;

		int MaxClientCount;
		int ExtraClientCount; // 가능하면 로그인에서 짜르도록 MaxClientCount + 여유분을 준비한다.

		short MaxClientSockOptRecvBufferSize;
		short MaxClientSockOptSendBufferSize;
		short MaxClientRecvBufferSize = 0;
		short MaxClientSendBufferSize;

		bool IsLoginCheck;	// 연결 후 특정 시간 이내에 로그인 완료 여부 조사

		int MaxLobbyCount;
		int MaxLobbyUserCount;
		int MaxRoomCountByLobby;
		int MaxRoomUserCount;
	};

	enum class SOCKET_CLOSE_CASE : short
	{
		SESSION_POOL_EMPTY = 1,
		SELECT_ERROR = 2,
		SOCKET_RECV_ERROR = 3,
		SOCKET_RECV_BUFFER_PROCESS_ERROR = 4,
		SOCKET_SEND_ERROR = 5,
		FORCING_CLOSE = 6,
	};

	struct RecvPacketInfo
	{
		RecvPacketInfo() = default;
		~RecvPacketInfo() {};

		int SessionIndex = 0;
		short PacketId = 0;
		short PacketBodySize = 0;
		char * pData = nullptr;
	};

}