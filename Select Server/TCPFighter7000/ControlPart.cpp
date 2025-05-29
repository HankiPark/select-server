#include <iostream>
#include <conio.h>
#include <windows.h>
#include <unordered_map>
#include "ControlPart.h"
#include "Struct.h"
#include "Logging.h"

int acceptCnt = 0;
int disconnectCnt = 0;
int csmoveCnt = 0;
int csmoveStopCnt = 0;
int csattackCnt = 0;
int csechoCnt = 0;
int scmyCharacterCnt = 0;
int scotherCharacterCnt = 0;
int scdeleteCharacterCnt = 0;
int scmoveCnt = 0;
int scmoveStopCnt = 0;
int scattackCnt = 0;
int scechoCnt = 0;
int recvByte = 0;
int recvCnt = 0;
int sendByte = 0;
int sendCnt = 0;
int logicCnt = 0;
int syncCnt = 0;
int scdamagedCnt = 0;
bool LoadData()
{
	sessionMap.reserve(10000);
	characterMap.reserve(10000);
	return true;
}

void ServerControl()
{
	static bool bControlMode = false;

	if (!_kbhit())
	{
		return;
	}

	WCHAR ControlKey = _getwch();
	if (L'u' == ControlKey || L'U' == ControlKey)
	{
		bControlMode = true;
		wprintf(L"Control Mode \n");
		wprintf(L" Press Q - Quit \n");
		wprintf(L" Press L - Key Lock \n");
	}

	if (bControlMode && (L'l' == ControlKey || L'L' == ControlKey))
	{
		wprintf(L"Key Lock ..  Press U - Key UnLock\n");
		bControlMode = false;
	}

	if (bControlMode && (L'q' == ControlKey || L'Q' == ControlKey))
	{
		beShutdown = true;
	}
	return;
}
bool Monitor()
{
	static int montimer = timeGetTime();
	//int cnt = 0;
	if (timeGetTime() - montimer >= 1000)
	{
		GAMELOG(L"--------------------------------------------------------\n");
		GAMELOG(L"Network Loop : %d       Logic Loop : %d\n", networkLoopCnt, logicCnt);
		GAMELOG(L"Accept : %d       Disconnect : %d       Sync : %d\n", acceptCnt, disconnectCnt, syncCnt);
		GAMELOG(L"recv Byte : %d       recv Count : %d\n", recvByte, recvCnt);
		GAMELOG(L"CSMoveStart Packet : %d       CSMoveStop Packet : %d\n", csmoveCnt, csmoveStopCnt);
		GAMELOG(L"CSAttack Packet : %d       CSEcho Packet : %d\n", csattackCnt, csechoCnt);
		GAMELOG(L"send Byte : %d       send Count : %d\n", sendByte, sendCnt);
		GAMELOG(L"SCCreateMyCharacter Packet : %d       SCCreateOtherCharacter Packet : %d       SCDeleteCharacter Packet : %d\n",
			scmyCharacterCnt, scotherCharacterCnt, scdeleteCharacterCnt);
		GAMELOG(L"SCMoveStart Packet : %d       SCMoveStop Packet : %d\n", scmoveCnt, scmoveStopCnt);
		GAMELOG(L"SCAttack Packet : %d       ScDamaged Packet : %d       SCEcho Packet : %d\n", scattackCnt, scdamagedCnt, scechoCnt);
		GAMELOG(L"--------------------------------------------------------\n");
		networkLoopCnt = 0;
		acceptCnt = 0;
		disconnectCnt = 0;
		recvByte = 0;
		recvCnt = 0;
		sendByte = 0;
		sendCnt = 0;
		logicCnt = 0;
		scmoveCnt = 0;
		csmoveCnt = 0;
		scmoveStopCnt = 0;
		csmoveStopCnt = 0;
		scattackCnt = 0;
		csattackCnt = 0;
		scechoCnt = 0;
		csechoCnt = 0;
		scdamagedCnt = 0;
		scmyCharacterCnt = 0;
		scotherCharacterCnt = 0;
		scdeleteCharacterCnt = 0;
		
		montimer += 1000;
	}



	return true;
}