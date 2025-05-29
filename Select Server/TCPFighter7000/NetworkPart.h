#ifndef __NETWORKPART__
#define __NETWORKPART__
#include "Struct.h"
#include "BufferClass.h"
#include "PacketDefine.h"
#include "ObjectPoolStack.h"
#include <unordered_map>

extern int logLevel;
extern int acceptCnt;
extern int disconnectCnt;
extern int scmoveCnt;
extern int scmoveStopCnt;
extern int scattackCnt;
extern int scechoCnt;
extern int csmoveCnt;
extern int csmoveStopCnt;
extern int csattackCnt;
extern int csechoCnt;
extern int syncCnt;
extern int recvByte;
extern int recvCnt;
extern int sendByte;
extern int sendCnt;
extern int scmyCharacterCnt;
extern int scotherCharacterCnt;
extern int scdeleteCharacterCnt;
extern int scdamagedCnt;
extern WCHAR logBuffer[1024];
extern std::unordered_map<DWORD, CHARACTER*> characterMap;
extern ObjectPool<CHARACTER> characterPool;
extern std::list<CHARACTER*> sector[SECTOR_LEN_Y][SECTOR_LEN_X];

bool SetNetwork();

void QuitNetwork() noexcept;

bool NetworkProcess();

bool AcceptProc();

int SendUnicast(SESSION* session, CPacket* packetBuffer);

int SendAround(SESSION* session, CPacket* packetBuffer, bool sendMe = false);

__forceinline bool MakePacketCreateMyCharacter(CPacket* packetBuffer,
	DWORD id, BYTE direction, unsigned short x, unsigned short y, unsigned char hp)
{
	PacketHeaderInfo header;
	header.byCode = 0x89;
	header.bySize = 10;
	header.byType = dfPACKET_SC_CREATE_MY_CHARACTER;
	packetBuffer->ClearPacket();
	packetBuffer->MoveInsideStructData((char*)&header, sizeof(PacketHeaderInfo));
	*packetBuffer << id << direction << x << y << hp;

	return true;
}

__forceinline bool MakePacketCreateOtherCharacter(CPacket* packetBuffer,
	DWORD id, BYTE direction, unsigned short x, unsigned short y, unsigned char hp)
{
	PacketHeaderInfo header;
	header.byCode = 0x89;
	header.bySize = 10;
	header.byType = dfPACKET_SC_CREATE_OTHER_CHARACTER;
	packetBuffer->ClearPacket();
	packetBuffer->MoveInsideStructData((char*)&header, sizeof(PacketHeaderInfo));
	*packetBuffer << id << direction << x << y << hp;

	return true;
}

__forceinline bool MakePacketSCMoveStart(CPacket* packetBuffer,
	DWORD id, BYTE direction, unsigned short x, unsigned short y)
{
	PacketHeaderInfo header;
	header.byCode = 0x89;
	header.bySize = 9;
	header.byType = dfPACKET_SC_MOVE_START;
	packetBuffer->ClearPacket();
	packetBuffer->MoveInsideStructData((char*)&header, sizeof(PacketHeaderInfo));
	*packetBuffer << id << direction << x << y;

	return true;
}

__forceinline bool MakePacketDeleteCharacter(CPacket* packetBuffer, DWORD id)
{
	PacketHeaderInfo header;
	header.byCode = 0x89;
	header.bySize = 4;
	header.byType = dfPACKET_SC_DELETE_CHARACTER;
	packetBuffer->ClearPacket();
	packetBuffer->MoveInsideStructData((char*)&header, sizeof(PacketHeaderInfo));
	*packetBuffer << id;
	return true;
}

__forceinline bool MakePacketSync(CPacket* packetBuffer, DWORD id, unsigned short x, unsigned short y)
{
	PacketHeaderInfo header;
	header.byCode = 0x89;
	header.bySize = 8;
	header.byType = dfPACKET_SC_SYNC;
	packetBuffer->ClearPacket();
	packetBuffer->MoveInsideStructData((char*)&header, sizeof(PacketHeaderInfo));
	*packetBuffer << id << x << y;
	
	return true;
}

__forceinline bool MakePacketSCMoveStop(CPacket* packetBuffer,
	DWORD id, BYTE direction, unsigned short x, unsigned short y)
{
	PacketHeaderInfo header;
	header.byCode = 0x89;
	header.bySize = 9;
	header.byType = dfPACKET_SC_MOVE_STOP;
	packetBuffer->ClearPacket();
	packetBuffer->MoveInsideStructData((char*)&header, sizeof(PacketHeaderInfo));
	*packetBuffer << id << direction << x << y;

	return true;
}

__forceinline bool MakePacketSCAttck1(CPacket* packetBuffer,
	DWORD id, BYTE direction, unsigned short x, unsigned short y)
{
	PacketHeaderInfo header;
	header.byCode = 0x89;
	header.bySize = 9;
	header.byType = dfPACKET_SC_ATTACK1;
	packetBuffer->ClearPacket();
	packetBuffer->MoveInsideStructData((char*)&header, sizeof(PacketHeaderInfo));
	*packetBuffer << id << direction << x << y;
	return true;
}

__forceinline bool MakePacketSCAttck2(CPacket* packetBuffer,
	DWORD id, BYTE direction, unsigned short x, unsigned short y)
{
	PacketHeaderInfo header;
	header.byCode = 0x89;
	header.bySize = 9;
	header.byType = dfPACKET_SC_ATTACK2;
	packetBuffer->ClearPacket();
	packetBuffer->MoveInsideStructData((char*)&header, sizeof(PacketHeaderInfo));
	*packetBuffer << id << direction << x << y;
	return true;
}

__forceinline bool MakePacketSCAttck3(CPacket* packetBuffer,
	DWORD id, BYTE direction, unsigned short x, unsigned short y)
{
	PacketHeaderInfo header;
	header.byCode = 0x89;
	header.bySize = 9;
	header.byType = dfPACKET_SC_ATTACK3;
	packetBuffer->ClearPacket();
	packetBuffer->MoveInsideStructData((char*)&header, sizeof(PacketHeaderInfo));
	*packetBuffer << id << direction << x << y;
	return true;
}

__forceinline bool MakePacketSCDamage(CPacket* packetBuffer,
	DWORD attackId, DWORD damageId, unsigned char hp)
{
	PacketHeaderInfo header;
	header.byCode = 0x89;
	header.bySize = 9;
	header.byType = dfPACKET_SC_DAMAGE;
	packetBuffer->ClearPacket();
	packetBuffer->MoveInsideStructData((char*)&header, sizeof(PacketHeaderInfo));
	*packetBuffer << attackId << damageId << hp;

	return true;
}

__forceinline bool MakePacketSCEcho(CPacket* packetBuffer,
	DWORD time)
{
	PacketHeaderInfo header;
	header.byCode = 0x89;
	header.bySize = 4;
	header.byType = dfPACKET_SC_ECHO;
	packetBuffer->ClearPacket();
	packetBuffer->MoveInsideStructData((char*)&header, sizeof(PacketHeaderInfo));
	*packetBuffer << time;

	return true;
}

bool RecvProc(SESSION* si);

bool SendProc(SESSION* si);

bool Disconnect(SESSION* si);

bool PacketProc(SESSION* si, BYTE packetType, CPacket* packetBuffer);

bool PacketProcMoveStart(SESSION* si, CPacket* packetBuffer);

bool PacketProcMoveStop(SESSION* si, CPacket* packetBuffer);

bool PacketProcAttack1(SESSION* si, CPacket* packetBuffer);

bool PacketProcAttack2(SESSION* si, CPacket* packetBuffer);

bool PacketProcAttack3(SESSION* si, CPacket* packetBuffer);

bool PacketProcEcho(SESSION* si, CPacket* packetBuffer);

#endif