//
// Created by 27711 on 2023-06-23.
//

#ifndef DNS_SERVER_C_PACKAGE_TRANSFORMER_H
#define DNS_SERVER_C_PACKAGE_TRANSFORMER_H
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "global.h"
#include "header.h"
void NumIP_To_CharIp(char *NumStart, char *ipInDns);
int GetLengthOfDns(char *DnsInfo);

int IpInDns(char *DnsInfo, char *ipInDns);
int CreateResponse(char *DnsInfo, int DnsLength, char *FindIp,
			  char *DnsResponse);
int CreateRequest(char *Domain, char *DnsInfo, unsigned short ID);
int UrlInDns(char *DnsInfo, char *UrlInDns);
void ShieldIP(char *DnsInfo, char* curIP);
#endif //DNS_SERVER_C_PACKAGE_TRANSFORMER_H
