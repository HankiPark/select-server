#ifndef __CONTROLPART__
#define __CONTROLPART__
#include <unordered_map>
#include <list>
#include "Struct.h"

extern bool beShutdown;
extern std::unordered_map<DWORD, CHARACTER*> characterMap;
extern std::unordered_map<SOCKET, SESSION*> sessionMap;
extern int networkLoopCnt;
extern WCHAR logBuffer[1024];
bool LoadData(); 
void ServerControl();
bool Monitor();

#endif