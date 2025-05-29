#ifndef __CONTENTPART__
#define __CONTENTPART__
#include <unordered_set>
#include "Struct.h"
#include "BufferClass.h"
#include "ObjectPoolStack.h"

extern std::unordered_map<SOCKET, SESSION*> sessionMap;
extern std::unordered_set<DWORD> moveSetGroup;
extern int logicCnt;
extern ObjectPool<CPacket> packetPool;
extern ObjectPool<SESSION> sessionPool;
#define SEARCH_ATTACK_RANGE(isLeft, xMax, xMin, yMax, yMin, X_RANGE, Y_RANGE)					\
do {																							\
		if (isLeft)																				\
		{																						\
			xMax = 	ci->x;																		\
			xMin = xMax - X_RANGE;																\
		}																						\
		else																					\
		{																						\
			xMin = ci->x;																		\
			xMax = xMin + X_RANGE;																\
		}																						\
		yMax = ci->y + Y_RANGE;																	\
		yMin = ci->y - Y_RANGE;																	\
} while (0)																						\

#define UPDATE_MOVE_CHARACTER(pCHARACTER, LocX, LocY)										\
do {																						\
	if (LocX >= 0 && LocY >= 0 && LocX < dfRANGE_MOVE_RIGHT && LocY < dfRANGE_MOVE_BOTTOM)	\
	{																						\
		pCHARACTER->x = LocX;																\
		pCHARACTER->y = LocY;																\
	}																						\
} while (0)																					\

bool CreatePlayerInfo(SESSION* si, int sessionId);

bool SectorUpdateCharacter(CHARACTER* ci);

bool CharacterSectorUpdatePacket(CHARACTER* ci);

bool CharacterSectorCreatePacket(CHARACTER* ci);

void GetSectorAround(int sectorX, int sectorY, SECTOR_AROUND* sectorAround) noexcept;

bool CheckDamagedPlayer(CHARACTER* ci, int attackType);

bool UpdateLogic();

#endif