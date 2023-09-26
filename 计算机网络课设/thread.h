#include "cache.h"
#include "windows.h"

extern CRITICAL_SECTION cacheMutex;       // 互斥锁
extern CONDITION_VARIABLE cacheCondition; // 条件变量
extern bool cacheNeedsUpdate;     // 缓存是否需要更新的标志
extern HANDLE asyncThread;

// 同步更新缓存的函数
void synUpdateCache(char* url, CachePtr dnsCache);

// 异步刷新缓存的线程函数
DWORD WINAPI asyncRefreshCache(LPVOID lpParam);

//建立线程函数
void initThread(void *arg);

//关闭线程句柄与销毁互斥锁和条件变量
void freeThread();