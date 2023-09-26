#include "thread.h"
#include <stdio.h>

CRITICAL_SECTION cacheMutex;       // ������
CONDITION_VARIABLE cacheCondition; // ��������
bool cacheNeedsUpdate = false;     // �����Ƿ���Ҫ���µı�־
HANDLE asyncThread;

// ͬ�����»���ĺ���
void syncUpdateCache(char* url, CachePtr dnsCache)
{
	// ����
	EnterCriticalSection(&cacheMutex);

	// ִ��ͬ�����»���Ĳ���
	//findAndRefresh(url, dnsCache);

	// ������ɺ����ñ�־
	cacheNeedsUpdate = true;

	// �����ź�֪ͨ�ȴ����߳�
	WakeConditionVariable(&cacheCondition);

	// ����
	LeaveCriticalSection(&cacheMutex);
}

// �첽ˢ�»�����̺߳���
DWORD WINAPI asyncRefreshCache(LPVOID lpParam)
{
	Sleep(1000);
	// ����
	EnterCriticalSection(&cacheMutex);

	// �ȴ�������Ҫ���µ��ź�
	while (!cacheNeedsUpdate) {
		SleepConditionVariableCS(&cacheCondition, &cacheMutex,
					 INFINITE);
	}

	// ִ���첽ˢ�»���Ĳ���
	//asyRefresh((CachePtr)lpParam);

	// ˢ����ɺ����ñ�־
	cacheNeedsUpdate = false;

	// ����
	LeaveCriticalSection(&cacheMutex);

	return 0;
}

void initThread(void *arg)
{
	// ��ʼ������������������
	InitializeCriticalSection(&cacheMutex);
	InitializeConditionVariable(&cacheCondition);

	// �����첽ˢ�»�����߳�
	asyncThread = CreateThread(NULL, 0, asyncRefreshCache,
					  arg, 0, NULL);
	if (asyncThread) {
		printf("asyThread create Success!\n");
	}

	// ִ��ͬ�����»���Ĳ���
	//syncUpdateCache();

	// �ȴ��첽�߳̽���
	WaitForSingleObject(asyncThread, INFINITE);

}

void freeThread() {
	// �ر��߳̾��
	if (CloseHandle(asyncThread)) {
		printf("CloseHandle Success!\n");
	}

	// ���ٻ���������������
	DeleteCriticalSection(&cacheMutex);
}