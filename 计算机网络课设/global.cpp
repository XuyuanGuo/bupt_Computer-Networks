//
// Created by 27711 on 2023-06-23.
//
#include "global.h"
#include <windows.h>

int globalTime;
int debug_level;

//ʹȫ��ʱ��������������Դ�ȥ��ⱨ����Ϣ�Ƿ����
unsigned int __stdcall timeRun(void *param)
{
	globalTime = 0;
	while (TRUE) {
		Sleep(1000);
		globalTime = (globalTime + 1) % TIME_SIZE;
	}
	return 0;
}