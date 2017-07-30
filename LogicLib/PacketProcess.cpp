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

		// ���� ��Ŷ�� �ش��ϴ� �Լ������ ã�´�.
		auto subscribedFuctions = _processMap.find(packetId);

		if (subscribedFuctions == _processMap.end())
		{
			_logger->Write(LogType::LOG_ERROR, "%s | Invalid packed id input!", __FUNCTION__);
			return;
		}

		// �Լ���ϵ��� ��������ش�.
		for (auto& function : subscribedFuctions->second)
		{
			function(packetInfo);
		}
	}

	void PacketProcess::Subscribe(short interestedPacketId, PacketFunc functor)
	{
		// �ش� ��Ŷ���̵�� ���� ����� �ٸ� ģ���� �ֳ� ã�ƺ���.
		if (_processMap.find(interestedPacketId) == _processMap.end())
		{
			// �ٸ� ģ���� ���ٸ�, ���� ����Ʈ�� ������ش�.
			PacketFuncList functionList;
			_processMap.emplace(interestedPacketId, std::move(functionList));
		}

		// �ش��ϴ� ������ ����Ʈ�� �� �Լ��� �о�ִ´�.
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