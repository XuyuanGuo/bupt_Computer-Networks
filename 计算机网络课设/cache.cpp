#include <stdlib.h>
#include <string.h>
#include "cache.h"
#include "global.h"
#include <stdio.h>

// DNS�����ʼ��
Cache *init_cache()
{
	// �����ڴ��Դ洢����ṹ
	cache *Cache = (cache *)malloc(sizeof(cache));
	Cache->size = 0;

	// ����ͷ�ڵ㣬����������Ϊ�������ʼλ��
	Cache->head = (cache_record *)malloc(sizeof(cache_record));
	Cache->head->url = NULL;
	Cache->head->ip = NULL;
	Cache->head->time_stamp = 0;
	Cache->head->next = NULL;

	// ��ʼ��βָ��Ϊͷ�ڵ�
	Cache->tail = Cache->head;

	return Cache;
}

// ��DNS�����в���ָ��URL��Ӧ��IP��ַ������LRU�㷨��
char *cache_search(char *url, cache *Cache)
{
	// ˢ�»��棬������ڵļ�¼
	cache_flash(Cache);

	char *ip = NULL;
	cache_record *pre, *cur;
	pre = Cache->head;
	cur = Cache->head->next;
	while (cur != NULL) {
		// ���URLƥ�䣬�򷵻ض�Ӧ��IP��ַ
		if (strcmp(url, cur->url) == 0) {
			ip = cur->ip;

			// ����ǰ��¼�Ƶ�����ĩβ��LRU�㷨��
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

// ��DNS�����в����µļ�¼������LRU�㷨��
char *cache_insert(char *url, char *ip, cache *Cache)
{
	// �����µĻ����¼��������URL��IP��ַ
	cache_record *tmp_record = (cache_record *)malloc(sizeof(cache_record));
	tmp_record->url = (char *)malloc(sizeof(char) * URL_MAX_SIZE);
	memcpy(tmp_record->url, url, strlen(url) + 1);
	tmp_record->ip = (char *)malloc(sizeof(char) * IP_MAX_SIZE);
	memcpy(tmp_record->ip, ip, strlen(ip) + 1);
	tmp_record->time_stamp = globalTime;

	// ���¼�¼���뵽����ĩβ��������βָ�루LRU�㷨��
	Cache->tail->next = tmp_record;
	Cache->tail = Cache->tail->next;
	tmp_record->next = NULL;

	//���������Ϣ
	if (debug_level == 2) {
		printf("Record is inserted to cache:url = %s, ip = %s, timpstamp = %d\n",
		       tmp_record->url, tmp_record->ip, tmp_record->time_stamp);
	}

	// ���ӻ����С��ˢ�»��棬��������ڵļ�¼
	Cache->size++;
	cache_flash(Cache);

	return NULL;
}

// ˢ��DNS���棬������ڵļ�¼
void cache_flash(cache *Cache)
{
	cache_record *tmp_record;
	while (Cache->size > 0 && (globalTime - Cache->head->next->time_stamp +
				   TIME_SIZE) % TIME_SIZE >
					  TTL) {
		// �Ƴ����ڵļ�¼���ͷ�����ڴ�
		tmp_record = Cache->head->next;
		Cache->head->next = Cache->head->next->next;
		free(tmp_record->url);
		free(tmp_record->ip);
		free(tmp_record);
		Cache->size--;

		// ��������Ϊ�գ������βָ��
		if (Cache->head->next == NULL) {
			Cache->tail = Cache->head;
		}
	}

	int i = 0;
	// ��������С�������ƣ�������Ƴ���¼ֱ�������С����Ҫ��LRU�㷨��
	while (Cache->size > cache_capicity) {
		i++;
		tmp_record = Cache->head->next;
		Cache->head->next = Cache->head->next->next;
		free(tmp_record->url);
		free(tmp_record->ip);
		free(tmp_record);
		Cache->size--;
	}

	// ��ӡ�����еļ�¼�����ڵ��ԣ�
	if (debug_level == 2) {
		printf("Cache is flashed:\n");
		printCache(Cache);
	}
}

// ��ӡDNS�����еļ�¼
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
