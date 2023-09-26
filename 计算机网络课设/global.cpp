//
// Created by 27711 on 2023-06-23.
//
#include "global.h"
#include <windows.h>

int globalTime;
int debug_level;

//使全局时间变量跑起来，以此去检测报文信息是否过期
unsigned int __stdcall timeRun(void *param)
{
	globalTime = 0;
	while (TRUE) {
		Sleep(1000);
		globalTime = (globalTime + 1) % TIME_SIZE;
	}
	return 0;
}