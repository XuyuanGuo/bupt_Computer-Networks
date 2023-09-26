//
// Created by 27711 on 2023-06-22.
//

#include <cstring>
#include "package_Transformer.h"
#include <stdio.h>

//��DNS��������ȡURL
int UrlInDns(char *DnsInfo, char *UrlInDns)
{
	int i = 12, k = 0;
	//DNF��ͷ��12�ֽڣ��ӵ�12���ֽڴ���ʼ���ж�ȡ����

	while (DnsInfo[i] != 0) {
		int labelLength = DnsInfo[i];
		memcpy(UrlInDns + k, DnsInfo + i + 1, labelLength);
		k += labelLength;
		UrlInDns[k] = '.';
		k++;
		i += (labelLength + 1);
	}
	//�����ַ���������
	UrlInDns[k - 1] = 0;
	//Э��ΪIPv4�򷵻�1��ΪIPv6�򷵻�0����������޸�Ϊ�������ظ���Э���Ӧ����ֵ
	return (!(DnsInfo[i + 2] - 1)) && (!DnsInfo[i + 1]);
}

//���ں�ipv4��ַ��Ӧ���ģ���ȡ��ipv4��ַ
int IpInDns(char *DnsInfo, char *ipInDns)
{
	int i = 12;
	int QuestionNum = DnsInfo[5];
	int AnswerNum = DnsInfo[7];

	// �������ⲿ�ֵ���Ŀ
	for (int queryNum = 0; queryNum < QuestionNum; queryNum++) {
		while (DnsInfo[i] != 0) {
			int length = DnsInfo[i];
			i += length + 1;
		}
		i += 5; // ����QTYPE��QCLASS�ֶ�
	}
	// ����IP��ַ��ƫ����
	//12��Name(2�ֽ�)+����(2�ֽ�)+��(2�ֽ�)+TTL(4�ֽ�)+���ݳ���(2�ֽ�)
	for (int answernum = 0; answernum < AnswerNum; answernum++) {
		//�����ipv4��ַ
		if ((unsigned short)(DnsInfo[i + 3]) == 1) {
			NumIP_To_CharIp(&DnsInfo[i + 12], ipInDns);
			return 1;
		} else {
			i = i + 12 + DnsInfo[i + 11];
		}
	}
	//��Ӧ�����в�����ipv4��ַ
	ipInDns = NULL;
	return 0;
}
void NumIP_To_CharIp(char *NumStart, char *ipInDns)
{
	int i, k = 0;
	unsigned char IpAddress;
	for (i = 0; i < 4; i++) {
		IpAddress = NumStart[i];
		//���޷�����������ʮ�������
		k += sprintf(&ipInDns[k], "%u", IpAddress);
		ipInDns[k] = '.';
		k++;
	}
	ipInDns[k - 1] = 0;
}


int CreateResponse(char *DnsInfo, int DnsLength, char *FindIp,
		   char *DnsResponse)
{
	//���տͻ��˷����Ĳ�ѯ����ȥ������Ӧ����
	int LengthOfDns = GetLengthOfDns(DnsInfo);
	memcpy(DnsResponse, DnsInfo, LengthOfDns);

	//�������ñ��ĵı�־λΪ0x8180��0x0000��ǰ�ߴ�����ȷ�ҵ�IP��ַ�����أ����ߴ���û�ҵ�IP��ַ����һ���ջش�
	unsigned short a = (DnsLength == LengthOfDns) ? htons(FLAG_MATCH)
						      : htons(FLAG_NO_MATCH);
	memcpy(&DnsResponse[2], &a, sizeof(unsigned short)); // �޸ı�־λ

	//�ҵ���IPΪ0�������Σ����ûش���Ϊ0
	if (strcmp(FindIp, "0.0.0.0") == 0) {
		a = htons(FLAG_NO_MATCH);
	} else {
		a = htons(FLAG_IP_FOUND);
	}
	memcpy(&DnsResponse[6], &a, sizeof(unsigned short));

	int curLen = 0;
	char answer[16];

	//����host��Ӧ�ĸ����ֶ�
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
		//��ӡ�����ֶε�ֵ�벻�����˰���printf֮����ʲô�ð취����Զ�����ӡ,���ԾͲ���ʾ������
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

//ʵ�����λ���
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
//	//��������ͷ�ĸ����ֶ�-Flag��Question��Answer RRs.......
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
//	//�洢������Ϣ
//	for (i = 0; Domain[i] != 0; i++) {
//		j = 1;
//		while (Domain[i] != '.' && Domain[i] != 0) {
//			DnsInfo[k + j] = Domain[i];
//			i++;
//			j++;
//		}
//		//�ַ�������
//		DnsInfo[k] = j - 1;
//		//����ƫ����
//		k = k + j;
//	}
//	DnsInfo[k] = 0;
//	//Type��Class�ֶ�
//	DnsInfo[k + 1] = 0;
//	DnsInfo[k + 2] = 1;
//	DnsInfo[k + 3] = 0;
//	DnsInfo[k + 4] = 1;
//	return k + 4 + 1;
//}