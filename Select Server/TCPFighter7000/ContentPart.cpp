#include <iostream>
#include <windows.h>
#include <unordered_map>
#include <unordered_set>
#include <list>
#include "PacketDefine.h"
#include "Struct.h"

#include "BufferClass.h"
#include "NetworkPart.h" 
#include "ContentPart.h"
#include "Logging.h"
#include "ObjectPoolStack.h"

std::unordered_map<DWORD, CHARACTER*> characterMap;
ObjectPool<CHARACTER> characterPool;
std::list<CHARACTER*> sector[SECTOR_LEN_Y][SECTOR_LEN_X];

bool CreatePlayerInfo(SESSION* si, int sessionId)
{
	// 플레이어 객체 생성 후 데이터 작성
	CHARACTER* ci = characterPool.Alloc();
	ci->sessionPointer = si;
	ci->sessionId = sessionId;
	ci->isAction = ACTION_MOVE_STOP;
	ci->x = rand() % 6300 + 1;
	ci->y = rand() % 6300 + 1;
	ci->curSector = { ci->x / SECTOR_SIZE, ci->y / SECTOR_SIZE };
	ci->oldSector = { ci->curSector.x, ci->curSector.y };
	ci->hp = 100;
	ci->isSynced = false;
	if (ci->x >= 3150)
	{
		ci->direction = dfPACKET_MOVE_DIR_LL;
		ci->faceDirection = dfPACKET_MOVE_DIR_LL;
	}
	else
	{
		ci->direction = dfPACKET_MOVE_DIR_RR;
		ci->faceDirection = dfPACKET_MOVE_DIR_RR;
	}
	// 플레이어 생성 패킷 전달
	CPacket* packetBuffer = packetPool.Alloc();
	packetBuffer->ClearPacket();
	MakePacketCreateMyCharacter(packetBuffer, ci->sessionId, ci->direction, ci->x, ci->y, ci->hp);
	SendUnicast(si, packetBuffer);
	scmyCharacterCnt++;
	packetPool.Free(packetBuffer);
	//sector와 characterMap에 등록
	sector[ci->curSector.y][ci->curSector.x].push_back(ci);
	characterMap[sessionId] = ci;

	// sector내의 캐릭터 생성
	CharacterSectorCreatePacket(ci);

	return true;
}

// 섹터가 바뀌었는지 판단
bool SectorUpdateCharacter(CHARACTER* ci)
{
	ci->oldSector.x = ci->curSector.x;
	ci->oldSector.y = ci->curSector.y;

	ci->curSector.x = ci->x / SECTOR_SIZE;
	ci->curSector.y = ci->y / SECTOR_SIZE;

	if (ci->oldSector.x == ci->curSector.x && ci->oldSector.y == ci->curSector.y)
	{
		return false;
	}
	return true;
}

// player 객체 생성 시 sector내의 캐릭터 생성
bool CharacterSectorCreatePacket(CHARACTER* ci)
{
	SECTOR_AROUND sa;
	CPacket* packetBuffer = packetPool.Alloc();
	packetBuffer->ClearPacket();
	MakePacketCreateOtherCharacter(packetBuffer, ci->sessionId, ci->direction, ci->x, ci->y, ci->hp);
	scotherCharacterCnt += SendAround(ci->sessionPointer, packetBuffer, false);

	GetSectorAround(ci->curSector.x, ci->curSector.y, &sa);
	std::list<CHARACTER*> sectorList;
	std::list<CHARACTER*>::iterator iter;
	for (int i = 0; i < sa.count; i++)
	{

		sectorList = sector[sa.around[i].y][sa.around[i].x];
		for (iter = sectorList.begin(); iter != sectorList.end(); ++iter)
		{	
			if ((*iter)->sessionId == ci->sessionId)
			{
				continue;
			}
			packetBuffer->ClearPacket();
			MakePacketCreateOtherCharacter(packetBuffer, 
				(*iter)->sessionId, (*iter)->direction, (*iter)->x, (*iter)->y, (*iter)->hp);
			scotherCharacterCnt += SendUnicast(ci->sessionPointer, packetBuffer);
			
			if ((*iter)->isAction == ACTION_MOVEING)
			{
				packetBuffer->ClearPacket();
				MakePacketSCMoveStart(packetBuffer, (*iter)->sessionId, (*iter)->direction, (*iter)->x, (*iter)->y);
				scmoveCnt += SendUnicast(ci->sessionPointer, packetBuffer);
				
			}

		}
	}
	packetPool.Free(packetBuffer);
	return true;
}

// 캐릭터 이동에 따른 섹터 업데이트
bool CharacterSectorUpdatePacket(CHARACTER* ci)
{	
	CPacket* CreatePacketBuffer = packetPool.Alloc();
	CPacket* MoveUserPacketBuffer = packetPool.Alloc();
	CPacket* CreateUserPacketBuffer = packetPool.Alloc();
	CPacket* DeleteUserPacketBuffer = packetPool.Alloc();
	CreatePacketBuffer->ClearPacket();
	MoveUserPacketBuffer->ClearPacket();
	CreateUserPacketBuffer->ClearPacket();
	DeleteUserPacketBuffer->ClearPacket();
	SECTOR_AROUND removeSector;
	SECTOR_AROUND addSector;
	bool userMove = false;
	int removeCnt = 0;
	int addCnt = 0;
	int oldX = ci->oldSector.x;
	int oldY = ci->oldSector.y;
	int curX = ci->curSector.x;
	int curY = ci->curSector.y;
	std::list<CHARACTER*> sectorList;
	std::list<CHARACTER*>::iterator iter;
	int i;
	// 캐릭터 이동 방향에 따른 8가지 방향값
	int directionVal = (curX - oldX) + (curY - oldY) * 3 + 4;
	// 섹터 이동에 따른 해당 세션의 캐릭터 생성 패킷 미리 생성
	MakePacketCreateOtherCharacter(CreateUserPacketBuffer, ci->sessionId, ci->direction, ci->x, ci->y, ci->hp);
	// 섹터 이동에 따른 해당 세션의 캐릭터 삭제 패킷 미리 생성
	MakePacketDeleteCharacter(DeleteUserPacketBuffer, ci->sessionId);
	// 세션의 캐릭터가 이동중이라면 이동중 패킷도 생성
	if (ci->isAction == ACTION_MOVEING)
	{
		MakePacketSCMoveStart(MoveUserPacketBuffer, ci->sessionId, ci->direction, ci->x, ci->y);
		userMove = true;
	}
	//방향에 따른 삭제와 생성 sector 판단
	switch (directionVal)
	{
	case DIRECTION_VALUE_LU:
	{
		if (oldX + 1 < SECTOR_LEN_X)
		{
			removeSector.around[removeCnt++] = { oldX + 1, oldY };
			if (oldY + 1 < SECTOR_LEN_Y)
			{
				removeSector.around[removeCnt++] = { oldX + 1, oldY + 1 };
			}
			if (oldY - 1 >= 0)
			{
				removeSector.around[removeCnt++] = { oldX + 1, oldY - 1 };
			}
		}

		if (oldY + 1 < SECTOR_LEN_Y)
		{
			removeSector.around[removeCnt++] = { oldX, oldY + 1 };
			if (oldX - 1 >= 0)
			{
				removeSector.around[removeCnt++] = { oldX - 1, oldY + 1 };
			}
		}



		if (curX - 1 >= 0)
		{
			addSector.around[addCnt++] = { curX - 1, curY };
			if (curY - 1 >= 0)
			{
				addSector.around[addCnt++] = { curX - 1, curY - 1 };
			}
			if (curY + 1 < SECTOR_LEN_Y)
			{
				addSector.around[addCnt++] = { curX - 1, curY + 1 };
			}
		}
		if (curY - 1 >= 0)
		{
			addSector.around[addCnt++] = { curX, curY - 1 };
			if (curX + 1 < SECTOR_LEN_X)
			{
				addSector.around[addCnt++] = { curX + 1, curY - 1 };
			}
		}

	}
		break;
	case DIRECTION_VALUE_UU:
	{
		if (oldY + 1 < SECTOR_LEN_Y)
		{
			removeSector.around[removeCnt++] = { oldX, oldY + 1 };
			if (oldX - 1 >= 0)
			{
				removeSector.around[removeCnt++] = { oldX - 1, oldY + 1 };
			}
			if (oldX + 1 < SECTOR_LEN_X)
			{
				removeSector.around[removeCnt++] = { oldX + 1, oldY + 1 };
			}
		}

		if (curY - 1 >= 0)
		{
			addSector.around[addCnt++] = { curX, curY - 1 };
			if (curX - 1 >= 0)
			{
				addSector.around[addCnt++] = { curX - 1, curY - 1 };
			}
			if (curX + 1 < SECTOR_LEN_X)
			{
				addSector.around[addCnt++] = { curX + 1, curY - 1 };
			}
		}
	}
		break;
	case DIRECTION_VALUE_RU:
	{
		if (oldX - 1 >= 0)
		{
			removeSector.around[removeCnt++] = { oldX - 1, oldY };
			if (oldY - 1 >= 0)
			{
				removeSector.around[removeCnt++] = { oldX - 1, oldY - 1 };
			}
			if (oldY + 1 < SECTOR_LEN_Y)
			{
				removeSector.around[removeCnt++] = { oldX - 1, oldY + 1 };
			}
		}

		if (oldY + 1 < SECTOR_LEN_Y)
		{
			removeSector.around[removeCnt++] = { oldX, oldY + 1 };
			if (oldX + 1 < SECTOR_LEN_X)
			{
				removeSector.around[removeCnt++] = { oldX + 1, oldY + 1 };
			}
		}

		if (curX + 1 < SECTOR_LEN_X)
		{
			addSector.around[addCnt++] = { curX + 1, curY };
			if (curY + 1 < SECTOR_LEN_Y)
			{
				addSector.around[addCnt++] = { curX + 1, curY + 1 };
			}
			if (curY - 1 >= 0)
			{
				addSector.around[addCnt++] = { curX + 1, curY - 1 };
			}
		}

		if (curY - 1 >= 0)
		{
			addSector.around[addCnt++] = { curX, curY - 1 };
			if (curX - 1 >= 0)
			{
				addSector.around[addCnt++] = { curX - 1, curY - 1 };
			}
		}

	}
		break;
	case DIRECTION_VALUE_LL:
	{
		if (oldX + 1 < SECTOR_LEN_X)
		{
			removeSector.around[removeCnt++] = { oldX + 1, oldY };
			if (oldY - 1 >= 0)
			{
				removeSector.around[removeCnt++] = { oldX + 1 , oldY - 1 };
			}
			if (oldY + 1 < SECTOR_LEN_Y)
			{
				removeSector.around[removeCnt++] = { oldX + 1 , oldY + 1 };
			}
		}

		if (curX - 1 >= 0)
		{
			addSector.around[addCnt++] = { curX - 1 , curY };

			if (curY - 1 >= 0)
			{
				addSector.around[addCnt++] = { curX - 1 , curY - 1 };
			}
			if (curY + 1 < SECTOR_LEN_Y)
			{
				addSector.around[addCnt++] = { curX - 1 , curY + 1 };
			}
		}
	}
		break;	
	case DIRECTION_VALUE_TRASH:
		break;
	case DIRECTION_VALUE_RR:
	{
		if (oldX - 1 >= 0)
		{
			removeSector.around[removeCnt++] = { oldX - 1 , oldY };

			if (oldY - 1 >= 0)
			{
				removeSector.around[removeCnt++] = { oldX - 1 , oldY - 1 };
			}
			if (oldY + 1 < SECTOR_LEN_Y)
			{
				removeSector.around[removeCnt++] = { oldX - 1 , oldY + 1 };
			}
		}

		if (curX + 1 < SECTOR_LEN_X)
		{
			addSector.around[addCnt++] = { curX + 1, curY };
			if (curY - 1 >= 0)
			{
				addSector.around[addCnt++] = { curX + 1 , curY - 1 };
			}
			if (curY + 1 < SECTOR_LEN_Y)
			{
				addSector.around[addCnt++] = { curX + 1 , curY + 1 };
			}
		}
	}
		break;	
	case DIRECTION_VALUE_LD:
	{
		if (oldX + 1 < SECTOR_LEN_X)
		{
			removeSector.around[removeCnt++] = { oldX + 1, oldY };
			if (oldY + 1 < SECTOR_LEN_Y)
			{
				removeSector.around[removeCnt++] = { oldX + 1, oldY + 1 };
			}
			if (oldY - 1 >= 0)
			{
				removeSector.around[removeCnt++] = { oldX + 1, oldY - 1 };
			}
		}

		if (oldY - 1 >= 0)
		{
			removeSector.around[removeCnt++] = { oldX, oldY - 1 };
			if (oldX - 1 >= 0)
			{
				removeSector.around[removeCnt++] = { oldX - 1, oldY - 1 };
			}
		}


		if (curX - 1 >= 0)
		{
			addSector.around[addCnt++] = { curX - 1, curY };
			if (curY - 1 >= 0)
			{
				addSector.around[addCnt++] = { curX - 1, curY - 1 };
			}
			if (curY + 1 < SECTOR_LEN_Y)
			{
				addSector.around[addCnt++] = { curX - 1, curY + 1 };
			}
		}

		if (curY + 1 < SECTOR_LEN_Y)
		{
			addSector.around[addCnt++] = { curX, curY + 1 };
			if (curX + 1 < SECTOR_LEN_X)
			{
				addSector.around[addCnt++] = { curX + 1, curY + 1 };
			}
		}
	}
		break;
	case DIRECTION_VALUE_DD:
	{
		if (oldY - 1 >= 0)
		{
			removeSector.around[removeCnt++] = { oldX, oldY - 1 };
			if (oldX - 1 >= 0)
			{
				removeSector.around[removeCnt++] = { oldX - 1, oldY - 1 };
			}
			if (oldX + 1 < SECTOR_LEN_X)
			{
				removeSector.around[removeCnt++] = { oldX + 1, oldY - 1 };
			}
		}

		if (curY + 1 < SECTOR_LEN_Y)
		{
			addSector.around[addCnt++] = { curX, curY + 1 };
			if (curX - 1 >= 0)
			{
				addSector.around[addCnt++] = { curX - 1, curY + 1 };
			}
			if (curX + 1 < SECTOR_LEN_X)
			{
				addSector.around[addCnt++] = { curX + 1, curY + 1 };
			}
		}
	}
		break;
	case DIRECTION_VALUE_RD:
	{
		if (oldX - 1 >= 0)
		{
			removeSector.around[removeCnt++] = { oldX - 1, oldY };
			if (oldY - 1 >= 0)
			{
				removeSector.around[removeCnt++] = { oldX - 1, oldY - 1 };
			}
			if (oldY + 1 < SECTOR_LEN_Y)
			{
				removeSector.around[removeCnt++] = { oldX - 1, oldY + 1 };
			}
		}
		if (oldY - 1 >= 0)
		{
			removeSector.around[removeCnt++] = { oldX, oldY - 1 };
			if (oldX + 1 < SECTOR_LEN_X)
			{
				removeSector.around[removeCnt++] = { oldX + 1, oldY - 1 };
			}
		}

		if (curX + 1 < SECTOR_LEN_X)
		{
			addSector.around[addCnt++] = { curX + 1, curY };
			if (curY + 1 < SECTOR_LEN_Y)
			{
				addSector.around[addCnt++] = { curX + 1, curY + 1 };
			}
			if (curY - 1 >= 0)
			{
				addSector.around[addCnt++] = { curX + 1, curY - 1 };
			}
		}

		if (curY + 1 < SECTOR_LEN_Y)
		{
			addSector.around[addCnt++] = { curX, curY + 1 };
			if (curX - 1 >= 0)
			{
				addSector.around[addCnt++] = { curX - 1, curY + 1 };
			}
		}

	}
		break;	
	}

	addSector.count = addCnt;
	removeSector.count = removeCnt;
	for (i = 0; i < addCnt; i++)
	{
		sectorList = sector[addSector.around[i].y][addSector.around[i].x];
		for (iter = sectorList.begin(); iter != sectorList.end(); ++iter)
		{
			scotherCharacterCnt += SendUnicast((*iter)->sessionPointer, CreateUserPacketBuffer);

			if (userMove)
			{
				scmoveCnt += SendUnicast((*iter)->sessionPointer, MoveUserPacketBuffer);
			}
			CreatePacketBuffer->ClearPacket();
			MakePacketCreateOtherCharacter(CreatePacketBuffer, (*iter)->sessionId, (*iter)->direction, (*iter)->x, (*iter)->y, (*iter)->hp);
			scotherCharacterCnt += SendUnicast(ci->sessionPointer, CreatePacketBuffer);

			if ((*iter)->isAction == ACTION_MOVEING)
			{
				CreatePacketBuffer->ClearPacket();
				MakePacketSCMoveStart(CreatePacketBuffer, (*iter)->sessionId, (*iter)->direction, (*iter)->x, (*iter)->y);
				scmoveCnt += SendUnicast(ci->sessionPointer, CreatePacketBuffer);

			}
		}
	}	

	for (i = 0; i < removeCnt; i++)
	{
		sectorList = sector[removeSector.around[i].y][removeSector.around[i].x];
		for (iter = sectorList.begin(); iter != sectorList.end(); ++iter)
		{
			scdeleteCharacterCnt += SendUnicast((*iter)->sessionPointer, DeleteUserPacketBuffer);

			CreatePacketBuffer->ClearPacket();
			MakePacketDeleteCharacter(CreatePacketBuffer, (*iter)->sessionId);
			scdeleteCharacterCnt += SendUnicast(ci->sessionPointer, CreatePacketBuffer);

		}
	}

	sector[ci->curSector.y][ci->curSector.x].push_back(ci);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          
	sector[ci->oldSector.y][ci->oldSector.x].remove(ci);

	packetPool.Free(CreatePacketBuffer);
	packetPool.Free(MoveUserPacketBuffer);
	packetPool.Free(CreateUserPacketBuffer);
	packetPool.Free(DeleteUserPacketBuffer);


	return true;
}

void GetSectorAround(int sectorX, int sectorY, SECTOR_AROUND* sectorAround) noexcept
{
	int checkBox[9][2] = { {-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, {0, 0}, {0, 1}, {1, -1}, {1, 0}, {1, 1} };
	int i;
	sectorAround->count = 0;
	for (i = 0; i < 9; i++)
	{
		int tempX = sectorX;
		int tempY = sectorY;
		tempX += checkBox[i][0];
		tempY += checkBox[i][1];

		if (tempX < 0 || tempY < 0 || tempX >= SECTOR_LEN_X || tempY >= SECTOR_LEN_Y)
		{
			continue;
		}
		sectorAround->around[sectorAround->count] = {tempX, tempY};
		sectorAround->count++;
	}
	return;
}
// 공격 판정을 위한 함수
bool CheckDamagedPlayer(CHARACTER* ci, int attackType)
{
	/*if (ci->sessionPointer->isDead)
	{
		return true;
	}*/
	int xMin;
	int xMax;
	int yMin;
	int yMax;
	int damage;
	bool isLeft;
	std::list<CHARACTER*> sectorList;
	std::list<CHARACTER*>::iterator iter;

	switch (ci->faceDirection)
	{
	case dfPACKET_MOVE_DIR_LL:
	case dfPACKET_MOVE_DIR_LU:
	case dfPACKET_MOVE_DIR_LD:
		isLeft = true;
		break;
	case dfPACKET_MOVE_DIR_RU:
	case dfPACKET_MOVE_DIR_RR:
	case dfPACKET_MOVE_DIR_RD:
		isLeft = false;
		break;
	case dfPACKET_MOVE_DIR_UU:
	case dfPACKET_MOVE_DIR_DD:
	default:
		return false;
		break;
	}

	switch (attackType)
	{
	case dfPACKET_SC_ATTACK1:
	{
		SEARCH_ATTACK_RANGE(isLeft, xMax, xMin, yMax, yMin, dfATTACK1_RANGE_X, dfATTACK1_RANGE_Y);
		damage = dfATTACK1_DAMAGE;
	}
	break;
	case dfPACKET_SC_ATTACK2:
	{
		SEARCH_ATTACK_RANGE(isLeft, xMax, xMin, yMax, yMin, dfATTACK2_RANGE_X, dfATTACK2_RANGE_Y);
		damage = dfATTACK2_DAMAGE;
	}
	break;
	case dfPACKET_SC_ATTACK3:
	{
		SEARCH_ATTACK_RANGE(isLeft, xMax, xMin, yMax, yMin, dfATTACK3_RANGE_X, dfATTACK3_RANGE_Y);
		damage = dfATTACK3_DAMAGE;
	}
	break;
	default:
		return false;
		break;
	}

	// 공격 시 내 섹터와 내 정면의 섹터만 범위로 한정하도록 변경
	sectorList = sector[ci->curSector.y][ci->curSector.x];
	for (iter = sectorList.begin(); iter != sectorList.end(); ++iter)
	{
		if ((*iter)->sessionId == ci->sessionId)
		{
			continue;
		}
		if ((*iter)->x < xMin || (*iter)->x > xMax ||
			(*iter)->y < yMin || (*iter)->y > yMax)
		{
			continue;
		}
		if ((*iter)->hp < damage)
		{
			(*iter)->hp = 0;
		}
		else
		{
			(*iter)->hp -= damage;
		}

		LOG(LOG_LEVEL_DEBUG, L"[%s] Attacked > Session ID : %d / left hp : %d / X : %d / Y : %d",
			L"DEBUG", (*iter)->sessionId, (*iter)->hp, (*iter)->x, (*iter)->y);

		CPacket* packetBuffer = packetPool.Alloc();
		packetBuffer->ClearPacket();
		MakePacketSCDamage(packetBuffer, ci->sessionId, (*iter)->sessionId, (*iter)->hp);
		scdamagedCnt += SendAround((*iter)->sessionPointer, packetBuffer, true);
		packetPool.Free(packetBuffer);
		return true;
	}
		
	if (isLeft && ci->curSector.x != 0)
	{
		sectorList = sector[ci->curSector.y][ci->curSector.x - 1];
		for (iter = sectorList.begin(); iter != sectorList.end(); ++iter)
		{
			if ((*iter)->sessionId == ci->sessionId)
			{
				continue;
			}
			if ((*iter)->x < xMin || (*iter)->x > xMax ||
				(*iter)->y < yMin || (*iter)->y > yMax)
			{
				continue;
			}
			if ((*iter)->hp < damage)
			{
				(*iter)->hp = 0;
			}
			else
			{
				(*iter)->hp -= damage;
			}

			LOG(LOG_LEVEL_DEBUG, L"[%s] Attacked > Session ID : %d / left hp : %d / X : %d / Y : %d",
				L"DEBUG", (*iter)->sessionId, (*iter)->hp, (*iter)->x, (*iter)->y);

			CPacket* packetBuffer = packetPool.Alloc();
			packetBuffer->ClearPacket();
			MakePacketSCDamage(packetBuffer, ci->sessionId, (*iter)->sessionId, (*iter)->hp);
			scdamagedCnt += SendAround((*iter)->sessionPointer, packetBuffer, true);
			packetPool.Free(packetBuffer);
			return true;
		}
	}
	else if (!isLeft && ci->curSector.x != SECTOR_LEN_X - 1)
	{
		sectorList = sector[ci->curSector.y][ci->curSector.x + 1];
		for (iter = sectorList.begin(); iter != sectorList.end(); ++iter)
		{
			if ((*iter)->sessionId == ci->sessionId)
			{
				continue;
			}
			if ((*iter)->x < xMin || (*iter)->x > xMax ||
				(*iter)->y < yMin || (*iter)->y > yMax)
			{
				continue;
			}
			if ((*iter)->hp < damage)
			{
				(*iter)->hp = 0;
			}
			else
			{
				(*iter)->hp -= damage;
			}

			LOG(LOG_LEVEL_DEBUG, L"[%s] Attacked > Session ID : %d / left hp : %d / X : %d / Y : %d",
				L"DEBUG", (*iter)->sessionId, (*iter)->hp, (*iter)->x, (*iter)->y);

			CPacket* packetBuffer = packetPool.Alloc();
			packetBuffer->ClearPacket();
			MakePacketSCDamage(packetBuffer, ci->sessionId, (*iter)->sessionId, (*iter)->hp);
			scdamagedCnt += SendAround((*iter)->sessionPointer, packetBuffer, true);
			packetPool.Free(packetBuffer);
			return true;
		}
	}

	return true;
}


bool UpdateLogic()
{
	static unsigned int timer = timeGetTime();
	CHARACTER* pCharacter = nullptr;
	SESSION* pSession = nullptr;
	std::unordered_map<DWORD, CHARACTER*>::iterator iter;
	std::unordered_set<DWORD>::iterator moveIter;
	int timeVal = timeGetTime() - timer;
	while (timeVal >= 40)
	{
		logicCnt++;
		timer += 40;
		timeVal -= 40;

		for (iter = characterMap.begin(); iter != characterMap.end(); )
		{
			pCharacter = iter->second;
			pSession = pCharacter->sessionPointer;
			if ((timer > pSession->lastRecvTime 
				&& timer - pSession->lastRecvTime > dfNETWORK_PACKET_RECV_TIMEOUT) ||
				pCharacter->hp <= 0)
			{
				Disconnect(pSession);
				closesocket(pSession->socket);
				sessionMap.erase(pSession->socket);
				sessionPool.Free(pSession);
				characterPool.Free(pCharacter);
				//delete pSession;
				//delete pCharacter;
				iter = characterMap.erase(iter);
				
			}
			else
			{
				++iter;
			}
		}

		
		for (moveIter = moveSetGroup.begin(); moveIter != moveSetGroup.end();)
		{
			pCharacter = characterMap[*moveIter];
			++moveIter;
			/*if (pCharacter->sessionPointer->isDead)
			{
				continue;
			}
			if (pCharacter->isAction == ACTION_MOVE_STOP)
			{
				continue;
			}*/

			switch (pCharacter->direction)
			{
			case dfPACKET_MOVE_DIR_LL:
			{
				UPDATE_MOVE_CHARACTER(pCharacter, pCharacter->x - dfSPEED_PLAYER_X, pCharacter->y);
			}
			break;
			case dfPACKET_MOVE_DIR_LU:
			{
				UPDATE_MOVE_CHARACTER(pCharacter, pCharacter->x - dfSPEED_PLAYER_X, pCharacter->y - dfSPEED_PLAYER_Y);
			}
			break;
			case dfPACKET_MOVE_DIR_UU:
			{
				UPDATE_MOVE_CHARACTER(pCharacter, pCharacter->x, pCharacter->y - dfSPEED_PLAYER_Y);
			}
			break;
			case dfPACKET_MOVE_DIR_RU:
			{
				UPDATE_MOVE_CHARACTER(pCharacter, pCharacter->x + dfSPEED_PLAYER_X, pCharacter->y - dfSPEED_PLAYER_Y);
			}
			break;
			case dfPACKET_MOVE_DIR_RR:
			{
				UPDATE_MOVE_CHARACTER(pCharacter, pCharacter->x + dfSPEED_PLAYER_X, pCharacter->y);
			}
			break;
			case dfPACKET_MOVE_DIR_RD:
			{
				UPDATE_MOVE_CHARACTER(pCharacter, pCharacter->x + dfSPEED_PLAYER_X, pCharacter->y + dfSPEED_PLAYER_Y);
			}
			break;
			case dfPACKET_MOVE_DIR_DD:
			{
				UPDATE_MOVE_CHARACTER(pCharacter, pCharacter->x, pCharacter->y + dfSPEED_PLAYER_Y);
			}
			break;
			case dfPACKET_MOVE_DIR_LD:
			{
				UPDATE_MOVE_CHARACTER(pCharacter, pCharacter->x - dfSPEED_PLAYER_X, pCharacter->y + dfSPEED_PLAYER_Y);
			}
			break;
			default:
				break;
			}


			if (SectorUpdateCharacter(pCharacter))
			{
				CharacterSectorUpdatePacket(pCharacter);
			}
		}
	}
	

	return true;
}