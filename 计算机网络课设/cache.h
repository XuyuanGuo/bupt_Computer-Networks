//
// Created by 27711 on 2023-06-25.
//

#define cache_capicity 30

typedef struct Cache_record {
	char *url; //”Ú√˚
	char *ip;
	int time_stamp;
	struct Cache_record *next;
} cache_record;

typedef struct Cache {
	cache_record *head;
	cache_record *tail;
	int size;
} cache, *CachePtr;

Cache *init_cache();
char *cache_search(char *url, cache *Cache);
char *cache_insert(char *url, char *ip, cache *Cache);
void cache_flash(cache *Cache);
void printCache(cache *Cache);
