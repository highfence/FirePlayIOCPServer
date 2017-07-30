#include "ServerHost.h"

#include <thread>
#include <chrono>
#include <memory>

#include "../Common/ConsoleLogger.h"

#include "../NetworkLib/IOCPNetwork.h"
#include "../NetworkLib/PacketQueue.h"

#include "PacketProcess.h"
#include "UserManager.h"
#include "LobbyManager.h"

namespace FirePlayLogic
{
	ERROR_CODE ServerHost::Init()
	{
		// 로깅 클래스 생성.
		_logger = std::make_unique<FirePlayCommon::ConsoleLogger>();

		// 서버 정보 불러들이기.
		loadConfig();

		// 패킷 큐 생성.
		_recvQueue = new PacketQueue();
		_sendQueue = new PacketQueue();

		// 네트워크 클래스 생성.
		//auto iocpInstance = IOCPNetwork::Factory::Create(_logger.get(), _serverInfo.get(), _recvQueue.get(), _sendQueue.get());
		//_iocpNetwork = std::make_unique<IOCPNetwork>(std::move(iocpInstance));
		_iocpNetwork = std::make_unique<IOCPNetwork>();
		_iocpNetwork->Init(_logger.get(), _serverInfo.get(), _recvQueue, _sendQueue);

		// 유저 매니저 생성.
		_userManager = std::make_unique<UserManager>();
		_userManager->Init(_serverInfo->MaxClientCount);

		// 패킷 프로세스 생성.
		//_packetProcess = std::make_unique<PacketProcess>(PacketProcess::Factory::Create(_logger.get(), _userManager.get(), _lobbyManager.get(), _recvQueue.get(), _sendQueue.get()));
		_packetProcess = std::make_unique<PacketProcess>();
		_packetProcess->Init(_logger.get(), _userManager.get(), _lobbyManager.get(), _recvQueue, _sendQueue);

		return ERROR_CODE::NONE;
	}

	void ServerHost::Run()
	{
		while (_isRun)
		{
			_packetProcess->Update();
		}
	}

	void ServerHost::Stop()
	{
		_isRun = false;
	}

	ERROR_CODE ServerHost::loadConfig()
	{
		_serverInfo = std::make_unique<ServerInfo>();

		wchar_t sPath[MAX_PATH] = { 0, };
		::GetCurrentDirectory(MAX_PATH, sPath);

		wchar_t inipath[MAX_PATH] = { 0, };
		_snwprintf_s(inipath, _countof(inipath), _TRUNCATE, L"%s\\ServerConfig.ini", sPath);

		_serverInfo->Port = (unsigned short)GetPrivateProfileInt(L"Config", L"Port", 0, inipath);
		_serverInfo->Backlog = GetPrivateProfileInt(L"Config", L"BackLogCount", 0, inipath);
		_serverInfo->MaxClientCount = GetPrivateProfileInt(L"Config", L"MaxClientCount", 0, inipath);

		_serverInfo->MaxClientSockOptRecvBufferSize = (short)GetPrivateProfileInt(L"Config", L"MaxClientSockOptRecvBufferSize", 0, inipath);
		_serverInfo->MaxClientSockOptSendBufferSize = (short)GetPrivateProfileInt(L"Config", L"MaxClientSockOptSendBufferSize", 0, inipath);
		_serverInfo->MaxClientRecvBufferSize = (short)GetPrivateProfileInt(L"Config", L"MaxClientRecvBufferSize", 0, inipath);
		_serverInfo->MaxClientSendBufferSize = (short)GetPrivateProfileInt(L"Config", L"MaxClientSendBufferSize", 0, inipath);

		_serverInfo->IsLoginCheck = GetPrivateProfileInt(L"Config", L"IsLoginCheck", 0, inipath) == 1 ? true : false;

		_serverInfo->ExtraClientCount = GetPrivateProfileInt(L"Config", L"ExtraClientCount", 0, inipath);
		_serverInfo->MaxLobbyCount = GetPrivateProfileInt(L"Config", L"MaxLobbyCount", 0, inipath);
		_serverInfo->MaxLobbyUserCount = GetPrivateProfileInt(L"Config", L"MaxLobbyUserCount", 0, inipath);
		_serverInfo->MaxRoomCountByLobby = GetPrivateProfileInt(L"Config", L"MaxRoomCountByLobby", 0, inipath);
		_serverInfo->MaxRoomUserCount = GetPrivateProfileInt(L"Config", L"MaxRoomUserCount", 0, inipath);

		_logger->Write(FirePlayNetwork::LogType::LOG_INFO, "%s | Port(%d), Backlog(%d)", __FUNCTION__, _serverInfo->Port, _serverInfo->Backlog);
		_logger->Write(FirePlayNetwork::LogType::LOG_INFO, "%s | IsLoginCheck(%d)", __FUNCTION__, _serverInfo->IsLoginCheck);
		return ERROR_CODE::NONE;
	}

	void ServerHost::release()
	{
		if (_iocpNetwork != nullptr)
		{
			_iocpNetwork->Stop();
		}
	}
}
