#include "thread.h"
#include <stdio.h>

CRITICAL_SECTION cacheMutex;       // 互斥锁
CONDITION_VARIABLE cacheCondition; // 条件变量
bool cacheNeedsUpdate = false;     // 缓存是否需要更新的标志
HANDLE asyncThread;

// 同步更新缓存的函数
void syncUpdateCache(char* url, CachePtr dnsCache)
{
	// 加锁
	EnterCriticalSection(&cacheMutex);

	// 执行同步更新缓存的操作
	//findAndRefresh(url, dnsCache);

	// 更新完成后设置标志
	cacheNeedsUpdate = true;

	// 发送信号通知等待的线程
	WakeConditionVariable(&cacheCondition);

	// 解锁
	LeaveCriticalSection(&cacheMutex);
}

// 异步刷新缓存的线程函数
DWORD WINAPI asyncRefreshCache(LPVOID lpParam)
{
	Sleep(1000);
	// 加锁
	EnterCriticalSection(&cacheMutex);

	// 等待缓存需要更新的信号
	while (!cacheNeedsUpdate) {
		SleepConditionVariableCS(&cacheCondition, &cacheMutex,
					 INFINITE);
	}

	// 执行异步刷新缓存的操作
	//asyRefresh((CachePtr)lpParam);

	// 刷新完成后重置标志
	cacheNeedsUpdate = false;

	// 解锁
	LeaveCriticalSection(&cacheMutex);

	return 0;
}

void initThread(void *arg)
{
	// 初始化互斥锁和条件变量
	InitializeCriticalSection(&cacheMutex);
	InitializeConditionVariable(&cacheCondition);

	// 创建异步刷新缓存的线程
	asyncThread = CreateThread(NULL, 0, asyncRefreshCache,
					  arg, 0, NULL);
	if (asyncThread) {
		printf("asyThread create Success!\n");
	}

	// 执行同步更新缓存的操作
	//syncUpdateCache();

	// 等待异步线程结束
	WaitForSingleObject(asyncThread, INFINITE);

}

void freeThread() {
	// 关闭线程句柄
	if (CloseHandle(asyncThread)) {
		printf("CloseHandle Success!\n");
	}

	// 销毁互斥锁和条件变量
	DeleteCriticalSection(&cacheMutex);
}