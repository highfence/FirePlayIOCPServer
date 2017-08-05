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
		int ExtraClientCount; // �����ϸ� �α��ο��� ¥������ MaxClientCount + �������� �غ��Ѵ�.

		short MaxClientSockOptRecvBufferSize;
		short MaxClientSockOptSendBufferSize;
		short MaxClientRecvBufferSize = 0;
		short MaxClientSendBufferSize;

		bool IsLoginCheck;	// ���� �� Ư�� �ð� �̳��� �α��� �Ϸ� ���� ����

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