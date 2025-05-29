#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <unordered_map>
#include <unordered_set>
#include "RingBuffer.h"

#include "Struct.h"
#include "Logging.h"
#include "NetworkPart.h"
#include "ContentPart.h"
#include "BufferClass.h"
#include "ObjectPoolStack.h"


static SOCKET listenSock;
std::unordered_map<SOCKET, SESSION*> sessionMap;
ObjectPool<CPacket> packetPool;
ObjectPool<SESSION> sessionPool;
std::unordered_set<DWORD> moveSetGroup;


bool NetworkProcess()
{
	static int timer = timeGetTime();
	int chunk = 0;
	bool chunkOk = true;
	int result = 0;
	fd_set readSet;
	fd_set writeSet;
	std::unordered_map<SOCKET, SESSION*>::iterator startIter;
	std::unordered_map<SOCKET, SESSION*>::iterator iter;
	std::unordered_map<SOCKET, SESSION*>::iterator endIter;
	std::unordered_map<SOCKET, SESSION*>::iterator innerIter;
	timeval t = { 0, 0 };
	if (timeGetTime() - timer > 80)
	{
		timer += 40;
		chunkOk = false;
	}
	else if (timeGetTime() - timer > 40)
	{
		timer += 40;
	}

		FD_ZERO(&readSet);
		FD_ZERO(&writeSet);
		int count = 1;
		FD_SET(listenSock, &readSet);


		startIter = sessionMap.begin();
		iter = sessionMap.begin();
		do
		{

			while (count <= 63 && iter != sessionMap.end())
			{
				if (iter->second->isDead)
				{
					++iter;
					continue;
				}
				characterMap[iter->second->sessionId]->isSynced = false;
				FD_SET(iter->first, &readSet);

				if ((iter->second)->sendBuffer.GetUseSize() > 7)
				{
					FD_SET(iter->first, &writeSet);
				}
				count++;
				++iter;
			}

			endIter = iter;
			result = select(NULL, &readSet, &writeSet, NULL, &t);
			// select 결과가 0
			if (result == 0)
			{
				FD_ZERO(&readSet);
				FD_ZERO(&writeSet);
				startIter = iter;
				count = 0;
				continue;
			}
			// select 실패
			else if (result == SOCKET_ERROR)
			{
				LOG(LOG_LEVEL_ERROR, L"[%s:%d] %s", L"ERROR", WSAGetLastError(), L"select Fail");
				return false;
			}
			if (FD_ISSET(listenSock, &readSet))
			{
				AcceptProc();
				result--;
			}
			// 결과값을 모두 처리할 때까지 루프
			while (result > 0)
			{
				if (startIter->second->isDead)
				{
					++startIter;
					continue;
				}
				// send 처리
				if (FD_ISSET(startIter->first, &writeSet))
				{
					SendProc(startIter->second);
					sendCnt++;
					result--;
					chunk++;
					if (chunkOk && chunk > 1000)
					{
						chunk = 0;
						Sleep(8);
					}
				}
				// recv 처리
				if (FD_ISSET(startIter->first, &readSet))
				{
					RecvProc(startIter->second);
					recvCnt++;
					result--;
				}

				++startIter;
			}

			FD_ZERO(&readSet);
			FD_ZERO(&writeSet);
			count = 0;
			startIter = iter;

		} while (iter != sessionMap.end());

	

	for (iter = sessionMap.begin(); iter != sessionMap.end();)
	{
		if (iter->second->isDead)
		{
			closesocket(iter->second->socket);
			
			//delete pSession;
			
			characterPool.Free(characterMap[iter->second->sessionId]);
			//delete pCharacter;
			characterMap.erase(iter->second->sessionId);
			sessionPool.Free(iter->second);
			iter = sessionMap.erase(iter);
			
		}
		else
		{
			iter++;
		}
	}


	return true;
}

bool AcceptProc(void)
{
	//session id로 사용할 인덱스
	static int idIndex = 1;

	SOCKADDR_IN clientAddr;
	int addrLen = sizeof(clientAddr);
	while (true)
	{
		SOCKET clientSock = accept(listenSock, (SOCKADDR*)&clientAddr, &addrLen);

		if (clientSock == INVALID_SOCKET)
		{
			// 모든 연결 수립 완료
			if (WSAGetLastError() == WSAEWOULDBLOCK)
			{
				return true;
			}
			// 연결 수립이 완료된 것이 아닌 다른 에러 발생
			LOG(LOG_LEVEL_ERROR, L"[%s | Error : %d] %s", L"ERROR", WSAGetLastError(), L"accept Fail");

			return false;
		}
		acceptCnt++;
		// 세션 객체 생성
		SESSION* si = sessionPool.Alloc();
		si->socket = clientSock;
		si->sessionId = idIndex++;
		si->ip = inet_ntoa(clientAddr.sin_addr);
		si->port = ntohs(clientAddr.sin_port);
		si->sendBuffer.ClearBuffer();
		si->recvBuffer.ClearBuffer();
		si->lastRecvTime = timeGetTime();
		si->isDead = false;
		// player 객체 생성
		CreatePlayerInfo(si, si->sessionId);
		sessionMap[clientSock] = si;
	}

	return true;
}

bool RecvProc(SESSION* si)
{
	int recvRetval;
	int tailPointerRetval;
	int recvDequeueRetval;
	/*if (si->isDead)
	{
		//printf("죽은 세션(%d)에 패킷 Recv 시도\n", si->socket);
		return true;
	}*/
	//recv
	recvRetval = recv(si->socket, si->recvBuffer.GetTailBufferPtr(), si->recvBuffer.DirectEnqueueSize(), 0);
	if (recvRetval <= 0)
	{
		if (WSAGetLastError() != WSAEWOULDBLOCK)
		{
			LOG(LOG_LEVEL_ERROR, L"[%s | Error : %d SessionId : %d] %s", L"ERROR", WSAGetLastError(), si->sessionId, L"recv Fail");
			
			
			Disconnect(si);
			return false;
		}
		return true;
	}
	// ringbuffer 포인터 조절
	tailPointerRetval = si->recvBuffer.MoveTail(recvRetval);

	if (tailPointerRetval != recvRetval)
	{
		LOG(LOG_LEVEL_ERROR, L"[%s] %s", L"ERROR", L"recvbuffer movetail Fail");
		__debugbreak();
	}
	recvByte += recvRetval;
	CPacket* packetBuffer = packetPool.Alloc();
	while (1)
	{
		if (si->recvBuffer.GetUseSize() < sizeof(PacketHeaderInfo))
		{
			break;
		}

		packetBuffer->ClearPacket();

		PacketHeaderInfo phi;
		// 추가 확인 과정
		recvDequeueRetval = si->recvBuffer.Peek((char*)&phi, sizeof(PacketHeaderInfo));
		if (recvDequeueRetval != sizeof(PacketHeaderInfo))
		{
			LOG(LOG_LEVEL_ERROR, L"[%s] %s", L"ERROR", L"recvbuffer peek Fail");
			__debugbreak();
		}
		//헤더가 잘못됨
		if (phi.byCode != 0x89)
		{
			break;
		}
		// 헤더에서 명시한 길이만큼 충분히 recv했는지 확인
		if (sizeof(PacketHeaderInfo) + phi.bySize > si->recvBuffer.GetUseSize())
		{
			break;
		}
		recvDequeueRetval = si->recvBuffer.MoveHead(sizeof(PacketHeaderInfo));
		if (recvDequeueRetval != sizeof(PacketHeaderInfo))
		{
			LOG(LOG_LEVEL_ERROR, L"[%s] %s", L"ERROR", L"recvbuffer movehead Fail");
			__debugbreak();
		}
		// direct로 데이터를 추출 가능할 경우

		recvDequeueRetval = packetBuffer->MoveInsideStructData(si->recvBuffer.GetHeadBufferPtr(), phi.bySize);
		if (recvDequeueRetval != phi.bySize)
		{
			LOG(LOG_LEVEL_ERROR, L"[%s] %s", L"ERROR", L"recvbuffer dequeue Fail");
			__debugbreak();
		}
		recvDequeueRetval = si->recvBuffer.MoveHead(phi.bySize);
		PacketProc(si, phi.byType, packetBuffer);
		
		
	}
	if (si->recvBuffer.GetUseSize() == 0)
	{
		si->recvBuffer.ClearBuffer();
	}
	si->lastRecvTime = timeGetTime();
	packetPool.Free(packetBuffer);
	return true;
}

bool SendProc(SESSION* si)
{
	int retval;
	
	// 죽은 세션이라면 패스
	if (si->isDead)
	{
//		printf("죽은 세션(%d)에 패킷 Send 시도\n", si->socket);
		return true;
	}
	// send ring buffer 내부의 데이터 모두 send
	while (si->sendBuffer.GetUseSize() > 0)
	{
		int messageLen = si->sendBuffer.DirectDequeueSize();
		//printf("session id : %d send len : %d \n",si->sessionId, messageLen);

		retval = send(si->socket, si->sendBuffer.GetHeadBufferPtr(), messageLen, 0);
		
		//send 실패 시
		if (retval != messageLen)
		{
			if (WSAGetLastError() == WSAECONNRESET || WSAGetLastError() == WSAECONNABORTED)
			{

				LOG(LOG_LEVEL_ERROR, L"[%s | Error : %d SessionId : %d] %s", L"ERROR", WSAGetLastError(),si->sessionId, L"send Fail");
				Disconnect(si);
				return false;
			}
			LOG(LOG_LEVEL_ERROR, L"[%s | Error : %d SessionId : %d] %s", L"ERROR", WSAGetLastError(), si->sessionId, L"send Fail");

			__debugbreak();
		}
		retval = si->sendBuffer.MoveHead(messageLen);
		sendByte += messageLen;
		/*if (retval != messageLen)
		{
			LOG(LOG_LEVEL_ERROR, L"[%s] %s", L"ERROR", L"sendbuffer movehead Fail");
			__debugbreak();
		}*/
	}
	si->sendBuffer.ClearBuffer();
	return true;
}
// 106.245.38.108
bool Disconnect(SESSION* si)
{

	LOG(LOG_LEVEL_ERROR, L"[%s | disconnect : %d SessionId : %d Port : %u]", L"ERROR", WSAGetLastError(), si->sessionId,  si->port);
	CPacket* packetBuffer = packetPool.Alloc();
	packetBuffer->ClearPacket();
	CHARACTER* ci = characterMap[si->sessionId];
	moveSetGroup.erase(si->sessionId);
	sector[ci->curSector.y][ci->curSector.x].remove(ci);
	MakePacketDeleteCharacter(packetBuffer, si->sessionId);
	SendAround(si, packetBuffer, false);
	disconnectCnt++;
	si->isDead = true;
	packetPool.Free(packetBuffer);
	return true;
}

bool PacketProc(SESSION* si, BYTE packetType, CPacket* packetBuffer)
{
	switch (packetType)
	{
	case dfPACKET_CS_MOVE_START:
		csmoveCnt++;
		return PacketProcMoveStart(si, packetBuffer);
		break;
	case dfPACKET_CS_MOVE_STOP:
		csmoveStopCnt++;
		return PacketProcMoveStop(si, packetBuffer);
		break;
	case dfPACKET_CS_ATTACK1:
		csattackCnt++;
		return PacketProcAttack1(si, packetBuffer);
		break;
	case dfPACKET_CS_ATTACK2:
		csattackCnt++;
		return PacketProcAttack2(si, packetBuffer);
		break;
	case dfPACKET_CS_ATTACK3:
		csattackCnt++;
		return PacketProcAttack3(si, packetBuffer);
		break;
	case dfPACKET_CS_ECHO:
		csechoCnt++;
		return PacketProcEcho(si, packetBuffer);
		break;
	default:
		break;
	}
	return true;
}

bool PacketProcMoveStart(SESSION* si, CPacket* packetBuffer)
{
	if (si->isDead)
	{
		return false;
	}
	CHARACTER* ci = characterMap[si->sessionId];
	unsigned char direction;
	unsigned short x;
	unsigned short y;
	*packetBuffer >> direction >> x >> y;

	LOG(LOG_LEVEL_DEBUG, L"[%s] MoveStart > Session ID : %d / Direction : %d / X : %d / Y : %d",
		L"DEBUG", si->sessionId, direction, x, y);
	/*
	if (ci == nullptr)
	{
		LOG(LOG_LEVEL_ERROR, L"[%s] MoveStart > Session ID : %d Not Found",
			L"ERROR", si->sessionId);
		return false;
	}*/

	if ((abs(ci->x - x) > dfERROR_RANGE ||
		abs(ci->y - y) > dfERROR_RANGE))
	{
		printf("start sync error session : %d, direction : %d, now x : %d , now y : %d, packet x : %d, packet y : %d \n"
		, ci->sessionId, ci->direction, ci->x, ci->y, x, y);
		MakePacketSync(packetBuffer, ci->sessionId, ci->x, ci->y);
		
		syncCnt += SendUnicast(si, packetBuffer);
		x = ci->x;
		y = ci->y;
	}

	ci->x = x;
	ci->y = y;
	ci->direction = direction;
	ci->isAction = ACTION_MOVEING;
	moveSetGroup.insert(si->sessionId);

	//얼굴 방향 전환
	if (direction != dfPACKET_MOVE_DIR_DD && direction != dfPACKET_MOVE_DIR_UU)
	{
		ci->faceDirection = direction;
	}

	if (SectorUpdateCharacter(ci))
	{
		CharacterSectorUpdatePacket(ci);
	}

	MakePacketSCMoveStart(packetBuffer, ci->sessionId, ci->direction, ci->x, ci->y);
	scmoveCnt += SendAround(si, packetBuffer, false);

	return true;
}


bool PacketProcMoveStop(SESSION* si, CPacket* packetBuffer)
{
	if (si->isDead)
	{
		return false;
	}
	CHARACTER* ci = characterMap[si->sessionId];
	unsigned char direction;
	unsigned short x;
	unsigned short y;
	*packetBuffer >> direction >> x >> y;

	LOG(LOG_LEVEL_DEBUG, L"[%s] MoveStop > Session ID : %d / Direction : %d / X : %d / Y : %d",
		L"DEBUG", si->sessionId, direction, x, y);

	/*if (ci == nullptr)
	{
		LOG(LOG_LEVEL_ERROR, L"[%s] MoveStop > Session ID : %d Not Found",
			L"ERROR", si->sessionId);
		return false;
	}*/


	if (ci->isSynced == false && (abs(ci->x - x) > dfERROR_RANGE ||
		abs(ci->y - y) > dfERROR_RANGE))
	{
		printf("stop sync error session : %d, direction : %d, now x : %d , now y : %d, packet x : %d, packet y : %d \n"
			, ci->sessionId, ci->direction, ci->x, ci->y, x, y);
		MakePacketSync(packetBuffer, ci->sessionId, ci->x, ci->y);
		syncCnt += SendUnicast(si, packetBuffer); // stop은 어짜피 다 같은 좌표에서 멈출테니 하나만 맞춰도 되지않을까?
		ci->isSynced = true;
		x = ci->x;
		y = ci->y;
	}

	ci->x = x;
	ci->y = y;
	ci->direction = direction;
	ci->isAction = ACTION_MOVE_STOP;
	moveSetGroup.erase(si->sessionId);
	if (SectorUpdateCharacter(ci))
	{
		CharacterSectorUpdatePacket(ci);
	}

	MakePacketSCMoveStop(packetBuffer, ci->sessionId, ci->direction, ci->x, ci->y);
	scmoveStopCnt += SendAround(si, packetBuffer, false);

	return true;
}


bool PacketProcAttack1(SESSION* si, CPacket* packetBuffer)
{
	if (si->isDead)
	{
		return false;
	}
	CHARACTER* ci = characterMap[si->sessionId];
	unsigned char direction;
	unsigned short x;
	unsigned short y;
	*packetBuffer >> direction >> x >> y;

	LOG(LOG_LEVEL_DEBUG, L"[%s] Attack1 > Session ID : %d / Direction : %d / X : %d / Y : %d",
		L"DEBUG", si->sessionId, direction, x, y);
	/*
	if (ci == nullptr)
	{
		LOG(LOG_LEVEL_ERROR, L"[%s] Attack1 > Session ID : %d Not Found",
			L"ERROR", si->sessionId);
		return false;
	}
	*/
	if (ci->isSynced == false && (abs(ci->x - x) > dfERROR_RANGE ||
		abs(ci->y - y) > dfERROR_RANGE))
	{
		printf("attack1 sync error session : %d, direction : %d, now x : %d , now y : %d, packet x : %d, packet y : %d \n"
			, ci->sessionId, ci->direction, ci->x, ci->y, x, y);
		MakePacketSync(packetBuffer, ci->sessionId, ci->x, ci->y);
		syncCnt += SendUnicast(si, packetBuffer);
		ci->isSynced = true;
		x = ci->x;
		y = ci->y;
	}

	ci->x = x;
	ci->y = y;
	ci->direction = direction;
	ci->isAction = ACTION_MOVE_STOP;


	MakePacketSCAttck1(packetBuffer, ci->sessionId, ci->direction, ci->x, ci->y);
	scattackCnt += SendAround(si, packetBuffer, false);

	CheckDamagedPlayer(ci, dfPACKET_SC_ATTACK1);

	return true;
}


bool PacketProcAttack2(SESSION* si, CPacket* packetBuffer)
{
	if (si->isDead)
	{
		return false;
	}
	CHARACTER* ci = characterMap[si->sessionId];
	unsigned char direction;
	unsigned short x;
	unsigned short y;
	*packetBuffer >> direction >> x >> y;

	LOG(LOG_LEVEL_DEBUG, L"[%s] Attack2 > Session ID : %d / Direction : %d / X : %d / Y : %d",
		L"DEBUG", si->sessionId, direction, x, y);
	/*
	if (ci == nullptr)
	{
		LOG(LOG_LEVEL_ERROR, L"[%s] Attack2 > Session ID : %d Not Found",
			L"ERROR", si->sessionId);
		return false;
	}
	*/
	if (ci->isSynced == false && (abs(ci->x - x) > dfERROR_RANGE ||
		abs(ci->y - y) > dfERROR_RANGE))
	{
		printf("attack2 sync error session : %d, direction : %d, now x : %d , now y : %d, packet x : %d, packet y : %d \n"
			, ci->sessionId, ci->direction, ci->x, ci->y, x, y);
		MakePacketSync(packetBuffer, ci->sessionId, ci->x, ci->y);
		syncCnt += SendUnicast(si, packetBuffer);
		ci->isSynced = true;
		x = ci->x;
		y = ci->y;
	}

	ci->x = x;
	ci->y = y;
	ci->direction = direction;
	ci->isAction = ACTION_MOVE_STOP;


	MakePacketSCAttck2(packetBuffer, ci->sessionId, ci->direction, ci->x, ci->y);
	scattackCnt += SendAround(si, packetBuffer, false);

	CheckDamagedPlayer(ci, dfPACKET_SC_ATTACK2);

	return true;
}


bool PacketProcAttack3(SESSION* si, CPacket* packetBuffer)
{
	if (si->isDead)
	{
		return false;
	}
	CHARACTER* ci = characterMap[si->sessionId];
	unsigned char direction;
	unsigned short x;
	unsigned short y;
	*packetBuffer >> direction >> x >> y;

	LOG(LOG_LEVEL_DEBUG, L"[%s] Attack3 > Session ID : %d / Direction : %d / X : %d / Y : %d",
		L"DEBUG", si->sessionId, direction, x, y);
	/*
	if (ci == nullptr)
	{
		LOG(LOG_LEVEL_ERROR, L"[%s] Attack3 > Session ID : %d Not Found",
			L"ERROR", si->sessionId);
		return false;
	}
	*/
	if (ci->isSynced == false && (abs(ci->x - x) > dfERROR_RANGE ||
		abs(ci->y - y) > dfERROR_RANGE))
	{
		printf("attack3 sync error session : %d, direction : %d, now x : %d , now y : %d, packet x : %d, packet y : %d \n"
			, ci->sessionId, ci->direction, ci->x, ci->y, x, y);
		MakePacketSync(packetBuffer, ci->sessionId, ci->x, ci->y);
		syncCnt += SendUnicast(si, packetBuffer);
		ci->isSynced = true;
		x = ci->x;
		y = ci->y;
	}

	ci->x = x;
	ci->y = y;
	ci->direction = direction;
	ci->isAction = ACTION_MOVE_STOP;


	MakePacketSCAttck3(packetBuffer, ci->sessionId, ci->direction, ci->x, ci->y);
	scattackCnt += SendAround(si, packetBuffer, false);

	CheckDamagedPlayer(ci, dfPACKET_SC_ATTACK3);

	return true;
}


bool PacketProcEcho(SESSION* si, CPacket* packetBuffer)
{
	if (si->isDead)
	{
		return false;
	}
	CHARACTER* ci = characterMap[si->sessionId];
	DWORD time;
	*packetBuffer >> time;
	LOG(LOG_LEVEL_DEBUG, L"[%s] Echo Message : %d time : %d",
		L"DEBUG", si->sessionId, time);
	/*
	if (ci == nullptr)
	{
		LOG(LOG_LEVEL_ERROR, L"[%s] Echo Message > Session ID : %d Not Found",
			L"ERROR", si->sessionId);
		return false;
	}
	*/
	MakePacketSCEcho(packetBuffer, time);
	scechoCnt += SendUnicast(si, packetBuffer);

	return true;
}


int SendUnicast(SESSION* session, CPacket* packetBuffer)
{

	int retval;
	int count;
	if (session == NULL || session->isDead)
	{
		return 0;
	}
	retval = session->sendBuffer.Enqueue((char*)packetBuffer->GetBufferPointer(), packetBuffer->GetUsedSize());
	if (retval != packetBuffer->GetUsedSize())
	{
		__debugbreak();
	}
	if (retval != *((char*)packetBuffer->GetBufferPointer() + 1) + 3)
	{
		int v = *((char*)packetBuffer->GetBufferPointer() + 1) + 3;
		__debugbreak();
	}

	return 1;
}

int SendAround(SESSION* session, CPacket* packetBuffer, bool sendMe)
{

	int retval;
	int sendCount = 0;
	SECTOR_AROUND sa;
	if (session == NULL || session->isDead)
	{
		return 0;
	}
	CHARACTER* ci = characterMap[session->sessionId];
	GetSectorAround(ci->curSector.x, ci->curSector.y, &sa);
	std::list<CHARACTER*> sectorList;
	std::list<CHARACTER*>::iterator iter;
	for (int i = 0; i < sa.count; i++)
	{
		sectorList = sector[sa.around[i].y][sa.around[i].x];
		for (iter = sectorList.begin(); iter != sectorList.end(); ++iter)
		{
			if ((*iter)->sessionPointer->isDead)
			{
				continue;
			}
			if (!sendMe && (*iter)->sessionPointer == session)
			{
				continue;
			}
			retval = (*iter)->sessionPointer->sendBuffer.Enqueue((char*)packetBuffer->GetBufferPointer(), packetBuffer->GetUsedSize());
			if (retval != packetBuffer->GetUsedSize())
			{
				__debugbreak();
			}
			sendCount++;
			/*if (retval != *((char*)packetBuffer->GetBufferPointer() + 1) + 3)
			{
				int v = *((char*)packetBuffer->GetBufferPointer() + 1) + 3;
				__debugbreak();
			}*/
		}
	}


	return sendCount;
}


bool SetNetwork()
{
	int retval;
	WSADATA wsa;
	retval = WSAStartup(MAKEWORD(2, 2), &wsa);
	if (retval != 0)
	{
		LOG(LOG_LEVEL_SYSTEM, L"[%s] %s", L"SYSTEM", L"WSAStartup Fail");
		return false;
	}
	LOG(LOG_LEVEL_SYSTEM, L"[%s] %s", L"SYSTEM", L"WSAStartup Success");
	listenSock = socket(AF_INET, SOCK_STREAM, 0);
	if (listenSock == INVALID_SOCKET)
	{
		LOG(LOG_LEVEL_SYSTEM, L"[%s] %s", L"SYSTEM", L"socket Func Fail");
		return false;
	}
	LOG(LOG_LEVEL_SYSTEM, L"[%s] %s", L"SYSTEM", L"socket Func Success");

	SOCKADDR_IN serverAddr;
	memset(&serverAddr, 0x00, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(6000);
	InetPton(AF_INET, L"0.0.0.0", &serverAddr.sin_addr);

	retval = bind(listenSock, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (retval == SOCKET_ERROR)
	{
		LOG(LOG_LEVEL_SYSTEM, L"[%s | Error : %d] %s", L"SYSTEM", WSAGetLastError(), L"bind Fail");
		return false;
	}
	LOG(LOG_LEVEL_SYSTEM, L"[%s] %s", L"SYSTEM", L"bind Success");
	u_long on = 1;
	retval = ioctlsocket(listenSock, FIONBIO, &on);
	if (retval != NO_ERROR)
	{
		LOG(LOG_LEVEL_SYSTEM, L"[%s | Error : %d] %s", L"SYSTEM", WSAGetLastError(), L"asynchronous Fail");
		return false;
	}
	linger li = { 1, 0 };
	retval = setsockopt(listenSock, SOL_SOCKET, SO_LINGER, (const char*) & li, sizeof(li));
	if (retval == SOCKET_ERROR)
	{
		LOG(LOG_LEVEL_SYSTEM, L"[%s | Error : %d] %s", L"SYSTEM", WSAGetLastError(), L"linger opt Fail");
		return false;
	}	
	
	int bufSize = 1000 * 1024;
	retval = setsockopt(listenSock, SOL_SOCKET, SO_SNDBUF, (const char*) &bufSize, sizeof(bufSize));
	if (retval == SOCKET_ERROR)
	{
		LOG(LOG_LEVEL_SYSTEM, L"[%s | Error : %d] %s", L"SYSTEM", WSAGetLastError(), L"sendbuf opt Fail");
		return false;
	}	
	retval = setsockopt(listenSock, SOL_SOCKET, SO_RCVBUF, (const char*) &bufSize, sizeof(bufSize));
	if (retval == SOCKET_ERROR)
	{
		LOG(LOG_LEVEL_SYSTEM, L"[%s | Error : %d] %s", L"SYSTEM", WSAGetLastError(), L"recvbuf opt Fail");
		return false;
	}/*
	retval = setsockopt(listenSock, IPPROTO_TCP, TCP_NODELAY, (const char*)&on, sizeof(on));
	if (retval == SOCKET_ERROR)
	{
		LOG(LOG_LEVEL_SYSTEM, L"[%s | Error : %d] %s", L"SYSTEM", WSAGetLastError(), L"linger opt Fail");
		return false;
	}
	*/
	LOG(LOG_LEVEL_SYSTEM, L"[%s] %s", L"SYSTEM", L"asynchronous Success");
	retval = listen(listenSock, SOMAXCONN_HINT(65535));
	if (retval == SOCKET_ERROR)
	{
		LOG(LOG_LEVEL_SYSTEM, L"[%s | Error : %d] %s", L"SYSTEM", WSAGetLastError(), L"listen Fail");
		return false;
	}
	LOG(LOG_LEVEL_SYSTEM, L"[%s] %s", L"SYSTEM", L"listen Success");
	return true;
}

void QuitNetwork() noexcept
{
	closesocket(listenSock);
	WSACleanup();
}
