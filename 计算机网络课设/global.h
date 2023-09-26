//
// Created by 27711 on 2023-06-22.
//
#ifndef DNS_SERVER_C_GLOBAL_H
#define DNS_SERVER_C_GLOBAL_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "wsock32.lib")
#endif

#define FLAG_MATCH 0x8180
#define FLAG_NO_MATCH 0x0000
#define FLAG_IP_FOUND 0x0001
#define NAME_OFFSET 0xc00c
#define TYPE_A 0x0001
#define CLASS_A 0x0001
#define TTL 0x10
#define IP_LENGTH 0x0004
#define IP_MAX_SIZE 16
#define URL_MAX_SIZE 200
#define ASCII_SIZE 128
#define LEN 512
#define ID_TABLE_SIZE 128
#define TIME_SIZE 1200
extern int globalTime;
extern int debug_level;
//extern int current_hour, current_minute, current_second£¬current_millisecond;
//extern SYSTEMTIME system_time;
//#include <time.h>
unsigned int __stdcall timeRun(void* );
#endif //DNS_SERVER_C_GLOBAL_H
