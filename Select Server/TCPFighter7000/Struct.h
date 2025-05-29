#ifndef __FILESTRUCT__
#define __FILESTRUCT__

#include "RingBuffer.h"

#define SECTOR_SIZE				128
#define SECTOR_LEN_X			(6400 / SECTOR_SIZE)
#define SECTOR_LEN_Y			(6400 / SECTOR_SIZE)

#define dfRANGE_MOVE_TOP	0
#define dfRANGE_MOVE_LEFT	0
#define dfRANGE_MOVE_RIGHT	6400
#define dfRANGE_MOVE_BOTTOM	6400

#define dfERROR_RANGE		50

#define dfATTACK1_RANGE_X		80
#define dfATTACK2_RANGE_X		90
#define dfATTACK3_RANGE_X		100
#define dfATTACK1_RANGE_Y		10
#define dfATTACK2_RANGE_Y		10
#define dfATTACK3_RANGE_Y		20

#define dfATTACK1_DAMAGE		1
#define dfATTACK2_DAMAGE		2
#define dfATTACK3_DAMAGE		3

#define dfNETWORK_PACKET_RECV_TIMEOUT	30000

#define dfSPEED_PLAYER_X	6	// 3   50fps
#define dfSPEED_PLAYER_Y	4	// 2   50fps

#define ACTION_MOVE_STOP		0
#define ACTION_MOVEING			1

#define DIRECTION_VALUE_LU			0
#define DIRECTION_VALUE_UU			1
#define DIRECTION_VALUE_RU			2
#define DIRECTION_VALUE_LL			3
#define DIRECTION_VALUE_TRASH		4
#define DIRECTION_VALUE_RR			5
#define DIRECTION_VALUE_LD			6
#define DIRECTION_VALUE_DD			7
#define DIRECTION_VALUE_RD			8

#pragma pack(push, 1)
struct PacketHeaderInfo
{
	unsigned char byCode;
	unsigned char bySize;
	unsigned char byType;
};

struct CharacterInfo
{
	unsigned int id;
	unsigned char direction;
	unsigned short x;
	unsigned short y;
	unsigned char hp;
};

struct DeleteInfo {
	unsigned int id;
};

struct CSMoveInfo {
	unsigned char direction;
	unsigned short x;
	unsigned short y;
};

struct SCMoveInfo {
	unsigned int id;
	unsigned char direction;
	unsigned short x;
	unsigned short y;
};

struct CSStopInfo
{
	unsigned char direction;
	unsigned short x;
	unsigned short y;
};

struct SCStopInfo {
	unsigned int id;
	unsigned char direction;
	unsigned short x;
	unsigned short y;
};

struct CSAttackInfo 
{
	unsigned char direction;
	unsigned short x;
	unsigned short y;
};

struct SCAttackInfo
{
	unsigned int id;
	unsigned char direction;
	unsigned short x;
	unsigned short y;
};

struct SCDamageInfo
{
	unsigned int attackId;
	unsigned int damageId;
	unsigned char hp;
};


#pragma pack(pop)

struct SESSION
{
	SOCKET	socket;
	DWORD	sessionId;
	RingBuffer recvBuffer;
	RingBuffer sendBuffer;
	DWORD lastRecvTime;
	char* ip;
	USHORT port;
	bool isDead;
	

	SESSION()
		: socket(0), sessionId(0), recvBuffer(1000), sendBuffer(3000), lastRecvTime(0), isDead(false), port(0) {};
};

struct SECTOR_POS
{
	int x;
	int y;
};

struct SECTOR_AROUND
{
	int count;
	SECTOR_POS around[9];
};

struct CHARACTER
{
	SESSION* sessionPointer;
	DWORD sessionId;

	unsigned short x;
	unsigned short y;
	
	SECTOR_POS curSector;
	SECTOR_POS oldSector;

	BYTE isAction;
	BYTE direction;
	BYTE faceDirection;
	unsigned char hp;
	bool isSynced = false;

	CHARACTER()
		: sessionPointer(nullptr), sessionId(0), x(0), y(0), curSector({ 0, 0 }), oldSector({ 0, 0 }),
		isAction(false), direction(0), faceDirection(0), hp(100), isSynced(false) {};
};

#endif