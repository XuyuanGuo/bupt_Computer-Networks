//
// Created by 27711 on 2023-06-23.
//
#ifndef DNS_SERVER_C_TIRE_H
#define DNS_SERVER_C_TIRE_H
#include "global.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
typedef struct TireNode {
	char ascii;
	char *IP_Address;
	bool isEndOfWord;
	struct TireNode **children;
} TireNode , *TirePtr;

TireNode *createNode();
char *findIP(char* domin ,TirePtr root);
TireNode* insertWord(TireNode *root, const char* word);
TirePtr searchWord(TireNode *root, const char* word);
TirePtr bulidTire(FILE *file);
#endif