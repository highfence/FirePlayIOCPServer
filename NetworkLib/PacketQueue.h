#pragma once
#include <memory>
#include <deque>
#include <mutex>

#include "../Common/PacketID.h"
#include "../Common/Define.h"

namespace FirePlayNetwork
{
	using RecvPacketInfo = FirePlayCommon::RecvPacketInfo;

	/*
		PacketQueue
		��Ƽ������ ȯ�濡�� ���ư��� ť�� �����ϱ� ���Ͽ� ���� Ŭ����.
	*/
	class PacketQueue
	{
	public :

		PacketQueue() = default;
		~PacketQueue()
		{
			std::lock_guard<std::mutex> lock(_mutex);
			_packetDeque.clear();
			_packetDeque.shrink_to_fit();
		}

		std::shared_ptr<RecvPacketInfo> Peek()
		{
			std::lock_guard<std::mutex> lock(_mutex);
			if (_packetDeque.empty())
			{
				return nullptr;
			}
			return _packetDeque.front();
		}

		void Pop()
		{
			std::lock_guard<std::mutex> lock(_mutex);
			_packetDeque.pop_front();
		}

		void Push(std::shared_ptr<RecvPacketInfo> recvPacket)
		{
			std::lock_guard<std::mutex> lock(_mutex);
			_packetDeque.emplace_back(recvPacket);
		}

		bool IsEmpty() { return _packetDeque.empty(); };

	private :

		std::deque<std::shared_ptr<RecvPacketInfo>> _packetDeque;
		std::mutex _mutex;
	};

}
