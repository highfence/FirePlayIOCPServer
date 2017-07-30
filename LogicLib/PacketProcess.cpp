#include "PacketProcess.h"

#include <memory>

#include "../Common/ConsoleLogger.h"
#include "../NetworkLib/PacketQueue.h"

#include "ConnectedUserManager.h"

namespace FirePlayLogic
{
	using LogType = FirePlayCommon::LogType;

	void PacketProcess::Init(ConsoleLogger * logger, PacketQueue * recvQueue, PacketQueue * sendQueue)
	{
		if (logger == nullptr || recvQueue == nullptr || sendQueue == nullptr)
		{
			return;
		}

		_logger = logger;
		_recvQueue = recvQueue;
		_sendQueue = sendQueue;
	}

	void PacketProcess::BroadCast(std::shared_ptr<RecvPacketInfo> packetInfo)
	{
		auto packetId = packetInfo->PacketId;

		// 들어온 패킷에 해당하는 함수목록을 찾는다.
		auto subscribedFuctions = _processMap.find(packetId);

		if (subscribedFuctions == _processMap.end())
		{
			_logger->Write(LogType::LOG_ERROR, "%s | Invalid packed id input!", __FUNCTION__);
			return;
		}

		// 함수목록들을 실행시켜준다.
		for (auto& function : subscribedFuctions->second)
		{
			function(packetInfo);
		}
	}

	void PacketProcess::Subscribe(short interestedPacketId, PacketFunc functor)
	{
		// 해당 패킷아이디로 구독 등록한 다른 친구가 있나 찾아본다.
		if (_processMap.find(interestedPacketId) == _processMap.end())
		{
			// 다른 친구가 없다면, 새로 리스트를 만들어준다.
			PacketFuncList functionList;
			_processMap.emplace(interestedPacketId, std::move(functionList));
		}

		// 해당하는 구독자 리스트에 내 함수를 밀어넣는다.
		_processMap.find(interestedPacketId)->second.emplace_back(std::move(functor));
	}

	void PacketProcess::Update()
	{
		auto recvPacket = _recvQueue->Peek();

		if (recvPacket == nullptr)
		{
			return;
		}

		BroadCast(recvPacket);
	}

	ERROR_CODE PacketProcess::ntfSysConnectSession(std::shared_ptr<RecvPacketInfo> packetInfo)
	{
		_connectedUserManager->SetConnectSession(packetInfo->SessionIndex);
		return ERROR_CODE::NONE;
	}

	ERROR_CODE PacketProcess::ntfSysCloseSession(std::shared_ptr<RecvPacketInfo> packetInfo)
	{
	
	}


}