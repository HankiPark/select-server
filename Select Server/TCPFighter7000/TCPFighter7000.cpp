#pragma comment(lib, "ws2_32")
#include <iostream>

#include <windows.h>
#include "NetworkPart.h"
#include "ContentPart.h"
#include "ControlPart.h"
#pragma comment(lib, "winmm.lib")

bool beShutdown = false;
int networkLoopCnt = 0;

int main()
{
	timeBeginPeriod(1);

	LoadData();

	int montimer = timeGetTime();
	if (!SetNetwork())
	{
		return 0;
	}
	while (!beShutdown)
	{

		if (NetworkProcess())
		{
			networkLoopCnt++;
		}

		UpdateLogic();


		//ServerControl();
		//loopCnt++;
		Monitor();
		//int cnt = 0;


	}

	QuitNetwork();

	return 0;
}
