//
// Created by 27711 on 2023-06-22.
//

#include <cstring>
#include "package_Transformer.h"
#include <stdio.h>

//从DNS报文中提取URL
int UrlInDns(char *DnsInfo, char *UrlInDns)
{
	int i = 12, k = 0;
	//DNF报头长12字节，从第12个字节处开始进行读取域名

	while (DnsInfo[i] != 0) {
		int labelLength = DnsInfo[i];
		memcpy(UrlInDns + k, DnsInfo + i + 1, labelLength);
		k += labelLength;
		UrlInDns[k] = '.';
		k++;
		i += (labelLength + 1);
	}
	//设置字符串结束符
	UrlInDns[k - 1] = 0;
	//协议为IPv4则返回1，为IPv6则返回0。这里可以修改为单独返回各个协议对应的数值
	return (!(DnsInfo[i + 2] - 1)) && (!DnsInfo[i + 1]);
}

//对于含ipv4地址的应答报文，提取其ipv4地址
int IpInDns(char *DnsInfo, char *ipInDns)
{
	int i = 12;
	int QuestionNum = DnsInfo[5];
	int AnswerNum = DnsInfo[7];

	// 跳过问题部分的条目
	for (int queryNum = 0; queryNum < QuestionNum; queryNum++) {
		while (DnsInfo[i] != 0) {
			int length = DnsInfo[i];
			i += length + 1;
		}
		i += 5; // 跳过QTYPE和QCLASS字段
	}
	// 计算IP地址的偏移量
	//12是Name(2字节)+类型(2字节)+类(2字节)+TTL(4字节)+数据长度(2字节)
	for (int answernum = 0; answernum < AnswerNum; answernum++) {
		//如果是ipv4地址
		if ((unsigned short)(DnsInfo[i + 3]) == 1) {
			NumIP_To_CharIp(&DnsInfo[i + 12], ipInDns);
			return 1;
		} else {
			i = i + 12 + DnsInfo[i + 11];
		}
	}
	//回应报文中不包含ipv4地址
	ipInDns = NULL;
	return 0;
}
void NumIP_To_CharIp(char *NumStart, char *ipInDns)
{
	int i, k = 0;
	unsigned char IpAddress;
	for (i = 0; i < 4; i++) {
		IpAddress = NumStart[i];
		//将无符号整数按照十进制输出
		k += sprintf(&ipInDns[k], "%u", IpAddress);
		ipInDns[k] = '.';
		k++;
	}
	ipInDns[k - 1] = 0;
}


int CreateResponse(char *DnsInfo, int DnsLength, char *FindIp,
		   char *DnsResponse)
{
	//按照客户端发来的查询报文去构建响应报文
	int LengthOfDns = GetLengthOfDns(DnsInfo);
	memcpy(DnsResponse, DnsInfo, LengthOfDns);

	//这里设置报文的标志位为0x8180或0x0000，前者代表正确找到IP地址并返回，后者代表没找到IP地址返回一个空回答
	unsigned short a = (DnsLength == LengthOfDns) ? htons(FLAG_MATCH)
						      : htons(FLAG_NO_MATCH);
	memcpy(&DnsResponse[2], &a, sizeof(unsigned short)); // 修改标志位

	//找到的IP为0代表屏蔽，设置回答数为0
	if (strcmp(FindIp, "0.0.0.0") == 0) {
		a = htons(FLAG_NO_MATCH);
	} else {
		a = htons(FLAG_IP_FOUND);
	}
	memcpy(&DnsResponse[6], &a, sizeof(unsigned short));

	int curLen = 0;
	char answer[16];

	//构建host响应的各个字段
	unsigned short Name = htons(NAME_OFFSET);
	unsigned short TypeA = htons(TYPE_A);
	unsigned short ClassA = htons(CLASS_A);
	unsigned long timeLive = htons(TTL);
	unsigned short IPLen = htons(IP_LENGTH);
	unsigned long IP = (unsigned long)inet_addr(FindIp);

	memcpy(answer + curLen, &Name, sizeof(unsigned short));
	curLen += sizeof(unsigned short);
	memcpy(answer + curLen, &TypeA, sizeof(unsigned short));
	curLen += sizeof(unsigned short);
	memcpy(answer + curLen, &ClassA, sizeof(unsigned short));
	curLen += sizeof(unsigned short);
	memcpy(answer + curLen, &timeLive, sizeof(unsigned long));
	curLen += sizeof(unsigned long);
	memcpy(answer + curLen, &IPLen, sizeof(unsigned short));
	curLen += sizeof(unsigned short);
	memcpy(answer + curLen, &IP, sizeof(unsigned long));
	curLen += sizeof(unsigned long);

	memcpy(DnsResponse + LengthOfDns, answer, sizeof(answer));

	if (debug_level == 2) {
		printf("CreateResponse Success---");
		//打印各个字段的值想不到除了挨个printf之外有什么好办法完成自动化打印,索性就不显示名称了
		/*printf("answer: ");
		for (int i = 0; i < sizeof(answer); i++) {
			printf("%02X ", answer[i]);
		}
		printf("\n");*/
	}
	
	return curLen + LengthOfDns;
}



int GetLengthOfDns(char *DnsInfo)
{
	int length = 12;
	while (DnsInfo[length] != 0) {
		length++;
	}
	length = length + 5;
	return length;
}

//实现屏蔽机制
void ShieldIP(char* DnsInfo, char* curIP) {
	if (curIP && !strcmp(curIP, "0.0.0.0")) {
		struct DNS_Header *error_Header = (struct DNS_Header *)DnsInfo;
		error_Header->qr = 1;
		error_Header->rcode = 3;
		if (debug_level) {
			printf("!!!IP is 0.0.0.0. Sent error message to client.!!!\n");
		}
	}
}

//int CreateRequest(char *Domain, char *DnsInfo, unsigned short ID)
//{
//	int i, j, k;
//	unsigned short id = ID;
//	memcpy(DnsInfo, &id, sizeof(unsigned short));
//	//构建报文头的各个字段-Flag、Question、Answer RRs.......
//	DnsInfo[2] = 1;
//	DnsInfo[3] = 0;
//	DnsInfo[4] = 0;
//	DnsInfo[5] = 1;
//	DnsInfo[6] = 0;
//	DnsInfo[7] = 0;
//	DnsInfo[8] = 0;
//	DnsInfo[9] = 0;
//	DnsInfo[10] = 0;
//	DnsInfo[11] = 0;
//	k = 12;
//	//存储域名信息
//	for (i = 0; Domain[i] != 0; i++) {
//		j = 1;
//		while (Domain[i] != '.' && Domain[i] != 0) {
//			DnsInfo[k + j] = Domain[i];
//			i++;
//			j++;
//		}
//		//字符计数法
//		DnsInfo[k] = j - 1;
//		//修正偏移量
//		k = k + j;
//	}
//	DnsInfo[k] = 0;
//	//Type与Class字段
//	DnsInfo[k + 1] = 0;
//	DnsInfo[k + 2] = 1;
//	DnsInfo[k + 3] = 0;
//	DnsInfo[k + 4] = 1;
//	return k + 4 + 1;
//}