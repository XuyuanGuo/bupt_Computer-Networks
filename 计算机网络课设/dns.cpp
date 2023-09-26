#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <process.h>
#include <stdexcept>
#include <cstring>
#include<Ws2tcpip.h>
#include "header.h"
#include "tire.h"
#include "ID_Transformer.h"
#include "package_Transformer.h"
#include "global.h"
#include "thread.h"
#define _WINSOCK_DEPRECATED_NO_WARNINGS

void set_Command_Option(int argc, char* argv[], char* server_IP, char* file_name);
void init_Socket(char *server_IP);

// socket variables -------------------------------------------------------------------------------
int my_socket;              // �׽���
SOCKADDR_IN client_address; // �ͻ����׽��ֵ�ַ
SOCKADDR_IN server_address; // �������׽��ֵ�ַ
SOCKADDR_IN tmp_sock_address; // �����ݴ��ַ����Ϊ���յ�ʱ��֪���ǿͻ��˻��Ƿ�����
int sockIn_Size = sizeof(SOCKADDR_IN);

int main(int argc, char *argv[])
{
	char server_IP[IP_MAX_SIZE] = "10.3.9.44";
	struct DNS_Header *dns_header = NULL;
	debug_level = 0;
	
	//ID_Table��ʼ��
	table_ID_Ptr tableId = (table_ID_Ptr)malloc(sizeof(table_ID));
	init_Table(tableId);
	
	//TimeSet
	SYSTEMTIME system_time;
	GetSystemTime(&system_time);

	int current_hour, current_minute, current_second;
	int current_millisecond = system_time.wMilliseconds;

	current_hour = system_time.wHour;
	current_minute = system_time.wMinute;
	current_second = system_time.wSecond;

	

	
	// argc-���������ַ����ĸ��� argv-ָ������ַ���
	printf("\n*****************************************************\n");
	printf("*                   DNS Relay Server                *\n");
	printf("*****************************************************\n");
	printf("command format: dnsrelay [-d|-dd] [dns-server-ipaddr] [filename]\n");
	printf("Start Time:%02d:%02d:%02d.%03d\n", current_hour, current_minute,
	       current_second, current_millisecond);

	//ʹ����ʦ�ṩ��dnsrelay.txt
	char *file_Name = "dnsrelay.txt";

	//���������в���ѡ����Եȼ�����������ַ�빹���ֵ������ļ�
	set_Command_Option(argc, argv, server_IP, file_Name);

	//�ֵ�������
	FILE *dnsFile = fopen(file_Name, "r");
	TirePtr dnsRoot = (TirePtr)malloc(sizeof(TireNode *));
	if (dnsFile == NULL) {
		printf("Failed to open file: %s\n", file_Name);
	} else {
		printf("Open file is success!\n");
		dnsRoot = bulidTire(dnsFile);
		if (dnsRoot != NULL) {
			printf("bulidTire Success!\n");
		} else {
			printf("bulidTire Fail!!!\n");
		}
		fclose(dnsFile); // �ر��ļ�
	}

	//cache��ʼ��
	CachePtr dnsCache;
	dnsCache = init_cache();

	//�̴߳���
	HANDLE hThread =
		(HANDLE)_beginthreadex(NULL, 0, timeRun, NULL, 0, NULL);
	//initThread(dnsCache);
	//printf("Thread Create OK\n");

	//socket���� 
	WSADATA wsadata; // �洢Socket���ʼ����Ϣ
	WSAStartup(MAKEWORD(2, 2),
		   &wsadata); // ���ݰ汾֪ͨ����ϵͳ������SOCKET�Ķ�̬���ӿ�
	printf("Start initialize socket.\n");
	init_Socket(server_IP); // ��ʼ�� ����

	//�������ݳ�ʼ��
	char buffer[LEN] = {0};
	char response_Buffer[LEN] = {0};
	char url[URL_MAX_SIZE];
	char ip[IP_MAX_SIZE];
	char *curIP = NULL;

	while (TRUE) {
		memset(buffer, '\0', LEN);
		
		//���ձ���
		int recvLen = recvfrom(my_socket, buffer, LEN, 0,
				       (SOCKADDR *)&tmp_sock_address,
				       &sockIn_Size);
		

		dns_header = (struct DNS_Header *)buffer;
		//printf("--------recvLen:%d--------\n", recvLen);
		
		//qrΪ1������Ӧ���ģ�Ϊ0�����ѯ����
		if (dns_header->qr == 1) {
			int sendLen;
			printf("--------- Received from Server,Length:%d---------\n",recvLen);

			if (UrlInDns(buffer, url) == 1) {
				printf("The package ----- URL:%s \n",
				       url);
			} 
			
			//idת��
			unsigned short id = dns_header->id;
			record_Ptr recordPtr = search_ID(tableId, id);
			
			//���ֵ����в���������Ӧ��IP
			//curIP = findIP(url, dnsRoot); 
			//IpInDns(buffer, ip);
			if(recordPtr != NULL)
			{
				//id = recordPtr->question_Id;
				memcpy(buffer, &recordPtr->question_Id
				       , sizeof(unsigned short));
						
				//IP���Σ����ɣ��Ƿ����ڷ��������صĵ�ַ������DNS�����������أ�
				//ShieldIP(buffer, curIP);
				
				sendLen = sendto(
					my_socket, buffer, recvLen, 0,
					(struct sockaddr *)&recordPtr
						->client_address,
					sizeof(recordPtr
							    ->client_address));
				
				//�ҵ���Ӧ��ID��Ϣ�󣬴��䷢�ؿͻ��˺�finished��ΪTRUE����ʾID���и�λ�ÿ�������ʹ��
				recordPtr->finished = TRUE;

				if (IpInDns(buffer, ip)) {
					//��cache�м����������ip��Ϣ
					cache_insert(url, ip, dnsCache);
				}
				
				if (debug_level >= 1) {
					GetSystemTime(&system_time);
					current_hour = system_time.wHour;
					current_minute = system_time.wMinute;
					current_second = system_time.wSecond;
					current_millisecond =
						system_time.wMilliseconds;
					printf("Time:%02d:%02d:%02d.%03d---",
					       current_hour, current_minute,
					       current_second,
					       current_millisecond);
					
					//�õ��ͻ���IP��ַ��Ϣ
					struct sockaddr_in clientAddress =
						recordPtr->client_address;
					char ipAddress[INET_ADDRSTRLEN];
					inet_ntop(AF_INET,
						  &(clientAddress.sin_addr),
						  ipAddress, INET_ADDRSTRLEN);
					
					printf("Transformer ID to %d to Send Client Address %s,the URL:%s in ID_Table\n",
					       id, ipAddress, url);
					if (ip != NULL) {
						printf("IP is %s\n", ip);
					}
				} else {
					printf("Transformer ID to Send Client\n");
				}
				
				if (debug_level == 2) {
					printf("SendLen = %d---", sendLen);
					for (int i = 0; i < sendLen; i++) {
						printf("%02x ", (unsigned char)buffer[i]);
					}
					printf("\n");

					printf("\tID is %02x, transId is %d, QR is %u ",
					       recordPtr->question_Id,id,dns_header->qr);
					printf("OPCODE is %u, AA is %u, TC is %u ",
					       dns_header->opcode,
					       dns_header->aa, dns_header->tc);
					printf("RD is %u, RA is %u, Z is %u, RCODE is %u\n",
					       dns_header->rd, dns_header->ra,
					       dns_header->z,
					       dns_header->rcode);
					printf("\tQDCOUNT is %u, ANCOUNT is %u, NSCOUNT is %u, ARCOUNT is %u\n",
					       htons(dns_header->qdcount),
					       htons(dns_header->ancount),
					       htons(dns_header->nscount),
					       htons(dns_header->arcount));
				}

			} else {
				printf("The ID of package is not found\n");
			}
			
		} else if (dns_header->qr == 0) {
			printf("--------- Received from Client,Length:%d---------\n",recvLen);
			
			client_address = tmp_sock_address;
			
			//��ôIPv4Э��,ҪôIPv6Э��
			if (UrlInDns(buffer, url) == 1) {
				printf("The package ----- URL:%s \n",
				       url);
			}
			bool notSearchFlag = TRUE;
			//��cache������в���
			if (notSearchFlag) {
				if (debug_level) {
					printf("1-Search in Cache!\n");
				}
				curIP = cache_search(url, dnsCache);
				if (curIP) {
					if (debug_level) {
						printf("In cache find Success\n");
					}
					notSearchFlag = false;
				} else {
					if (debug_level) {
						printf("Not find in cache\n");
					}
				}
			}

			if (notSearchFlag) {
				// ���ֵ����������
				if (debug_level) {
					printf("2-Search in Tire!\n");
				}
				curIP = findIP(url, dnsRoot);
				if (curIP != NULL) {
					notSearchFlag = FALSE;
					//IP����
					ShieldIP(buffer, curIP);
					
					//��cache�м����������ip��Ϣ
					//���ip��0.0.0.0��Ҳ����cache������ʹ����������������ʱ���ٷ���0.0.0.0
					cache_insert(url, curIP, dnsCache);
					if (debug_level) {
						printf("In tire find Success\n");
						printf("add Info to Cache-URL:%s,IP:%s\n",
						       url, curIP);
					}	
				} else {
					if (debug_level) {
						printf("Not find in tire\n");
					}
				}
			}

			//��cache���ֵ������ҵ�����˹�����Ӧ���ķ����ͻ���
			if (!notSearchFlag) {
				
				printf("Find the IP:%s, send response to Client\n",
				       curIP);
				
				int responseLen =
					CreateResponse(buffer, recvLen, curIP,
						       response_Buffer);
				sendto(my_socket, response_Buffer, responseLen,
				       0, (struct sockaddr *)&client_address,
				       sizeof(client_address));

				if (debug_level == 2) {
					printf("ResponseLen = %d---", responseLen);
					for (int i = 0; i < responseLen; i++) {
						printf("%02x ", (unsigned char)buffer[i]);
					}
				}
			}

			//��cache���ֵ�����û�ҵ�
			if (notSearchFlag) {
				//�ѿͻ��˷����ı��Ľ���IDת�������浽ID����
				unsigned short transID =
					save_ID(tableId, buffer, dns_header->id,
						client_address);
				if (transID == -1) {
					printf("Can't find space in ID_Table, throw away the package-URL:%s\n",
					       url);
				} else {
					//ȷ��ID�����пռ��ź󽫲�ѯ����ת������������
					int sendLen = sendto(
						my_socket, buffer, recvLen, 0,
						(struct sockaddr
							 *)&server_address,
						sizeof(server_address));
	
					
					if (debug_level >= 1) {
						GetSystemTime(&system_time);
						current_hour =
							system_time.wHour;
						current_minute =
							system_time.wMinute;
						current_second =
							system_time.wSecond;
						current_millisecond =
							system_time
								.wMilliseconds;

						printf("Time:%02d:%02d:%02d.%03d---",
						       current_hour,
						       current_minute,
						       current_second,
						       current_millisecond);

						//�õ��ͻ���IP��ַ��Ϣ
						struct sockaddr_in clientAddress =
							client_address;
						char ipAddress[INET_ADDRSTRLEN];
						inet_ntop(AF_INET,
							  &(clientAddress
								    .sin_addr),
							  ipAddress,
							  INET_ADDRSTRLEN);

						printf("The transID is:%d,client-ip:%s send to server-ip:%s, The URL is:%s\n",
						       transID, ipAddress,
						       server_IP, url);
					}

					if (debug_level == 2) {
						printf("SendLen = %d---",
						       sendLen);
						for (int i = 0; i < sendLen;
						     i++) {
							printf("%02x ",
							       (unsigned char)buffer[i]);
						}

						printf("\tID is %02x, transId is %d, QR is %u ",
						       dns_header->id,
						       transID, dns_header->qr);
						printf("OPCODE is %u, AA is %u, TC is %u ",
						       dns_header->opcode,
						       dns_header->aa,
						       dns_header->tc);
						printf("RD is %u, RA is %u, Z is %u, RCODE is %u\n",
						       dns_header->rd,
						       dns_header->ra,
						       dns_header->z,
						       dns_header->rcode);
						printf("\tQDCOUNT is %u, ANCOUNT is %u, NSCOUNT is %u, ARCOUNT is %u\n",
						       htons(dns_header->qdcount),
						       htons(dns_header->ancount),
						       htons(dns_header->nscount),
						       htons(dns_header->arcount));
					}
				}
			}
			
		} else {
			printf("Not recv any package!!!!!\n");
			break;
		}
		printf("\n");
		
	}
	return 0;
}

//�����׽���
int create_socket()
{
	int socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (socket_fd < 0) {
		printf("Failed to create socket.");
	}
	return socket_fd;
}

//���׽��ֵ�ָ����ַ��˿ں�
int bind_socket(int socket_fd, const char *ip, int port,
		struct sockaddr_in *address)
{
	memset(address, 0, sizeof(struct sockaddr_in));
	address->sin_family = AF_INET;
	address->sin_addr.s_addr = htonl(INADDR_ANY); // �󶨵�������õ�ַ
	address->sin_port = htons(port);

	// �����׽���ѡ���ֹ��ַ����
	int reuse = 0;
	if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR,
		       (const char *)&reuse, sizeof(reuse)) < 0) {
		printf("Failed to set socket options.\n");
		return -1;
	}

	// ���׽��ֵ�ָ����ַ��˿�
	if (bind(socket_fd, (struct sockaddr *)address,
		 sizeof(struct sockaddr_in)) < 0) {
		printf("Failed to bind socket port.\n");
		return -1;
	}

	return 0;
}

void init_Socket(char *server_ip)
{
	// �����׽���
	my_socket = create_socket();

	// ���׽��ֵ����ص�ַ��˿�
	int bind_result = bind_socket(my_socket, NULL, 53, &client_address);

	// ���÷�������ַ��Ϣ
	server_address.sin_family = AF_INET;
	inet_pton(AF_INET, server_ip, &(server_address.sin_addr));
	server_address.sin_port = htons(53);

	if (my_socket >= 0 && bind_result == 0) {
		printf("Socket build and bind successfully!\n");
	}
}

void set_Command_Option(int argc, char *argv[], char *server_IP,
			char *file_name)
{

	// ���������в���
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-d") == 0) {
			debug_level = 1;
		} else if (strcmp(argv[i], "-dd") == 0) {
			debug_level = 2;
		} else if (i == 2) {
			strcpy(server_IP, argv[i]);
		} else if (i == 3) {
			strcpy(file_name, argv[i]);
		}
	}

	// ������ý��
	printf("Set debug_level: %d\n", debug_level);
	if (server_IP != NULL) {
		printf("Set DNS server: %s\n", server_IP);
	}
	if (file_name != NULL) {
		printf("Set File Name: %s\n", file_name);
	}

}