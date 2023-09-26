#include <stdlib.h>
#include <string.h>
#include "cache.h"
#include "global.h"
#include <stdio.h>

// DNS缓存初始化
Cache *init_cache()
{
	// 分配内存以存储缓存结构
	cache *Cache = (cache *)malloc(sizeof(cache));
	Cache->size = 0;

	// 创建头节点，并将其设置为链表的起始位置
	Cache->head = (cache_record *)malloc(sizeof(cache_record));
	Cache->head->url = NULL;
	Cache->head->ip = NULL;
	Cache->head->time_stamp = 0;
	Cache->head->next = NULL;

	// 初始化尾指针为头节点
	Cache->tail = Cache->head;

	return Cache;
}

// 在DNS缓存中查找指定URL对应的IP地址（采用LRU算法）
char *cache_search(char *url, cache *Cache)
{
	// 刷新缓存，清除过期的记录
	cache_flash(Cache);

	char *ip = NULL;
	cache_record *pre, *cur;
	pre = Cache->head;
	cur = Cache->head->next;
	while (cur != NULL) {
		// 如果URL匹配，则返回对应的IP地址
		if (strcmp(url, cur->url) == 0) {
			ip = cur->ip;

			// 将当前记录移到链表末尾（LRU算法）
			if (cur->next != NULL) {
				pre->next = cur->next;
				Cache->tail->next = cur;
				Cache->tail = Cache->tail->next;
				cur->next = NULL;
			}
			break;
		}
		pre = cur;
		cur = cur->next;
	}
	return ip;
}

// 向DNS缓存中插入新的记录（采用LRU算法）
char *cache_insert(char *url, char *ip, cache *Cache)
{
	// 创建新的缓存记录，并复制URL和IP地址
	cache_record *tmp_record = (cache_record *)malloc(sizeof(cache_record));
	tmp_record->url = (char *)malloc(sizeof(char) * URL_MAX_SIZE);
	memcpy(tmp_record->url, url, strlen(url) + 1);
	tmp_record->ip = (char *)malloc(sizeof(char) * IP_MAX_SIZE);
	memcpy(tmp_record->ip, ip, strlen(ip) + 1);
	tmp_record->time_stamp = globalTime;

	// 将新记录插入到链表末尾，并更新尾指针（LRU算法）
	Cache->tail->next = tmp_record;
	Cache->tail = Cache->tail->next;
	tmp_record->next = NULL;

	//输出调试信息
	if (debug_level == 2) {
		printf("Record is inserted to cache:url = %s, ip = %s, timpstamp = %d\n",
		       tmp_record->url, tmp_record->ip, tmp_record->time_stamp);
	}

	// 增加缓存大小并刷新缓存，以清除过期的记录
	Cache->size++;
	cache_flash(Cache);

	return NULL;
}

// 刷新DNS缓存，清除过期的记录
void cache_flash(cache *Cache)
{
	cache_record *tmp_record;
	while (Cache->size > 0 && (globalTime - Cache->head->next->time_stamp +
				   TIME_SIZE) % TIME_SIZE >
					  TTL) {
		// 移除过期的记录并释放相关内存
		tmp_record = Cache->head->next;
		Cache->head->next = Cache->head->next->next;
		free(tmp_record->url);
		free(tmp_record->ip);
		free(tmp_record);
		Cache->size--;

		// 如果链表变为空，则更新尾指针
		if (Cache->head->next == NULL) {
			Cache->tail = Cache->head;
		}
	}

	int i = 0;
	// 如果缓存大小超过限制，则逐个移除记录直到缓存大小符合要求（LRU算法）
	while (Cache->size > cache_capicity) {
		i++;
		tmp_record = Cache->head->next;
		Cache->head->next = Cache->head->next->next;
		free(tmp_record->url);
		free(tmp_record->ip);
		free(tmp_record);
		Cache->size--;
	}

	// 打印缓存中的记录（用于调试）
	if (debug_level == 2) {
		printf("Cache is flashed:\n");
		printCache(Cache);
	}
}

// 打印DNS缓存中的记录
void printCache(cache *Cache)
{
	cache_record *cur;
	cur = Cache->head->next;
	int i = 0;
	while (cur != NULL) {
		printf("%d: url = %s, ip = %s, timpstamp = %d\n", i + 1,
		       cur->url, cur->ip, cur->time_stamp);
		i++;
		cur = cur->next;
	}
}
