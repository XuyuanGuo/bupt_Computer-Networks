#include"tire.h"
TirePtr searchWord(TireNode *root, const char *word)
{
	TirePtr current = (TireNode *)malloc(sizeof(TireNode));
	current = root;
	int len = strlen(word);

	for (int i = 0; i < len; i++) {
		int index = word[i];
		if (current->children[index] == NULL) {
			if (debug_level == 2) {
				printf("searchWord: can't search the word\n");
			}
			return NULL;
		}
		current = current->children[index];
	}
	return current;
}


//由域名寻找IP
char *findIP(char *domain, TirePtr dnsRoot)
{
	if (dnsRoot == NULL)
		return NULL;

	TirePtr curNode = searchWord(dnsRoot, domain);

	if (curNode != NULL) {
		if (debug_level == 2) {
			printf("searchWord: domain is%s, IP_Address is%s\n",
			       domain, curNode->IP_Address);
		}
		return curNode->IP_Address;
	}
	return NULL;
	
}

TirePtr bulidTire(FILE *file)
{
	TirePtr dnsRoot = (TirePtr)malloc(sizeof(TireNode));
	dnsRoot->IP_Address = NULL;
	dnsRoot->children = (TirePtr *)malloc(sizeof(TirePtr) * ASCII_SIZE);
	memset(dnsRoot->children, 0, sizeof(TirePtr) * ASCII_SIZE);
	
	TirePtr curNode = (TireNode *)malloc(sizeof(TireNode));
	char url[URL_MAX_SIZE];
	char ip[IP_MAX_SIZE];
	
	while (fscanf(file, "%s %s", ip, url) > 0) {
		//构建字符串路径
		//curNode = insertWord(dnsRoot, url);
		curNode = dnsRoot;
		int len = strlen(url);

		for (int i = 0; i < len; i++) {
			//这里不能使用url[i] - 'a'，因为有的域名里面含有数字、特定字符等
			//使用字符的ascii码值作为下标
			int index = url[i];
			
			//如果字符串路径不存在就在此基础上继续构建路径
			if (curNode->children[index] == NULL) {
				curNode->children[index] =
					(TirePtr)malloc(sizeof(TireNode));
				curNode->children[index]->IP_Address = NULL;
				curNode->children[index]->children =
					(TirePtr *)malloc(sizeof(TirePtr) *
							  ASCII_SIZE);
				memset(curNode->children[index]
					       ->children,
				       0, sizeof(TirePtr) * ASCII_SIZE);
			}
			curNode = curNode->children[index];
		}

		//给每个字符串路径与IP地址之间建立映射
		if (curNode->IP_Address == NULL) {
			curNode->IP_Address =
				(char *)malloc(sizeof(char) * (strlen(ip) + 1));
			memcpy(curNode->IP_Address, ip, strlen(ip) + 1);
			if (debug_level == 2) {
				printf("Building Tire: URL is %s, ip is %s\n",
				       url, ip);
			}
		} else {
			free(curNode->IP_Address);
			curNode->IP_Address =
				(char *)malloc(sizeof(char) * (strlen(ip) + 1));
			memcpy(curNode->IP_Address, ip, strlen(ip) + 1);
			
			if (debug_level == 2) {
				printf("The URL is: %s, old IP_Address:%s, new IP_Address:%s\n",
				       url, curNode->IP_Address, ip);
				printf("\n\treplace successfully.\n");
			}
		}		
	}

	printf("Bulid tireTree Successfully!\n\n");
	return dnsRoot;
}

TireNode *createNode()
{
	TireNode *node = (TireNode *)malloc(sizeof(TireNode));
	if (node) {
		node->ascii = '\0';
		node->isEndOfWord = false;
		for (int i = 0; i < ASCII_SIZE; i++) {
			node->children[i] = NULL;
		}
	}
	return node;
}

TireNode *insertWord(TireNode *root, const char *word)
{
	TireNode *current = root;
	int len = strlen(word);
	for (int i = 0; i < len; i++) {
		int index = word[i];
		if (current->children[index] == NULL) {
			current->children[index] = createNode();
			current->children[index]->ascii = word[i];
		}
		current = current->children[index];
	}
	current->isEndOfWord = true;
	return current;
}



void freeTrie(TireNode *root)
{
	if (root) {
		for (int i = 0; i < ASCII_SIZE; i++) {
			freeTrie(root->children[i]);
		}
		free(root);
	}
}
