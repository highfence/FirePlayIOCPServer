#pragma once
#include <memory>
#include <deque>
#include <mutex>

#include "../Common/PacketID.h"
#include "../Common/Define.h"

using RecvPacketInfo = FirePlayCommon::RecvPacketInfo;

namespace FirePlayNetwork
{
	/*
		PacketQueue
		멀티쓰레드 환경에서 돌아가는 큐를 지원하기 위하여 만든 클래스.
	*/
	class PacketQueue
	{
	public :

		PacketQueue() = default;
		~PacketQueue() = default;

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
			_packetDeque.pop_front();
		}

		void Push(std::shared_ptr<RecvPacketInfo> recvPacket)
		{
			_packetDeque.emplace_back(recvPacket);
		}

		bool IsEmpty() { return _packetDeque.empty(); };

	private :

		std::deque<std::shared_ptr<RecvPacketInfo>> _packetDeque;
		std::mutex _mutex;
	};

}
