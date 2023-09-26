//
// Created by 27711 on 2023-06-23.
//

#ifndef DNS_SERVER_C_ID_TRANSFORMER_H
#define DNS_SERVER_C_ID_TRANSFORMER_H

#include <unordered_map>
#include "global.h"


typedef struct {
	char *url; //����
	int Len;
	unsigned short question_Id; //Transaction ID
	SOCKADDR_IN client_address;
	int time;      //��ID����¼��ʱ���
	BOOL finished; //�������Ƿ����
} record, *record_Ptr;

typedef struct {
	record_Ptr records;
	int size;
} table_ID, *table_ID_Ptr;

void init_Table(table_ID_Ptr tableId);
record_Ptr search_ID(table_ID_Ptr tableId, unsigned short ID);
unsigned short save_ID(table_ID_Ptr tableId, char *buf, unsigned short ID,
		       SOCKADDR_IN client_addr);
//void free_table_ID(table_ID_Ptr tableId);
//void out_Of_Time(table_ID_Ptr tableId);
#endif //DNS_SERVER_C_ID_TRANSFORMER_H
