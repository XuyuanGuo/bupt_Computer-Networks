#include "cache.h"
#include "windows.h"

extern CRITICAL_SECTION cacheMutex;       // ������
extern CONDITION_VARIABLE cacheCondition; // ��������
extern bool cacheNeedsUpdate;     // �����Ƿ���Ҫ���µı�־
extern HANDLE asyncThread;

// ͬ�����»���ĺ���
void synUpdateCache(char* url, CachePtr dnsCache);

// �첽ˢ�»�����̺߳���
DWORD WINAPI asyncRefreshCache(LPVOID lpParam);

//�����̺߳���
void initThread(void *arg);

//�ر��߳̾�������ٻ���������������
void freeThread();