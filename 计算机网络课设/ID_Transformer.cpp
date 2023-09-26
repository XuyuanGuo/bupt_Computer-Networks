//
// Created by 27711 on 2023-06-23.
//

#include "ID_Transformer.h"
#include "package_Transformer.h"

//��ʼ��ID��
void init_Table(table_ID_Ptr tableId) {
    tableId->records = (record_Ptr)malloc(sizeof(record) * ID_TABLE_SIZE);
    tableId->size = 0;
    for (int i = 0; i < ID_TABLE_SIZE; i++) {
        tableId->records[i].url = (char*)malloc(sizeof(char) * URL_MAX_SIZE);
        tableId->records[i].Len = 0;
        tableId->records[i].question_Id = 0;
        tableId->records[i].finished = TRUE;
        tableId->records[i].time = -1;
        memset(&(tableId->records[i].client_address), 0, sizeof(SOCKADDR_IN));
    }
}


//�ӷ��������յ����ĺ����IDת��
record_Ptr search_ID(table_ID_Ptr tableId, unsigned short ID)
{
	//out_Of_Time(tableId);

	if (tableId->records[ID - 1].finished) {
		return NULL;
	} else {
		if (debug_level == 2) {
			printf("search_ID: ID is %d\n", ID);
		}
		
		tableId->records[ID - 1].finished = TRUE;
		return &tableId->records[ID - 1];
	}
}

//�ӿͻ����յ����ĺ����IDת��������
unsigned short save_ID(table_ID_Ptr tableId, char *buf, unsigned short ID,
		       SOCKADDR_IN client_addr)
{
	unsigned short transID = 0;
	
	// Ѱ�ҿ��еļ�¼������¼�¼

	int curIndex = tableId->size;

	/*if (curIndex >= ID_TABLE_SIZE) {
		return -1;
	}*/

	do {
		//��ʱ�ͽ�ID�ͷ�ȥ����������ʹ�ã��ҵ�һ���ܷ�ID��λ�þ�ֱ�ӷš�û��Ҫ���������ҵ�ʱ���Ӵ�ˢ��
		if ((globalTime - tableId->records[curIndex].time + TIME_SIZE) % TIME_SIZE > TTL) {
			tableId->records[curIndex].finished = TRUE;
		}

		if (tableId->records[curIndex].finished) {
			// ���¼�¼��ֵ(����RFC�ĵ���֪IDΪ0����Ϊ��Ч������ֵ��������Ŀ�ģ�����transID�����1��ʼ)
			transID = (unsigned short)(curIndex + 1);
			
			if (debug_level == 2) {
				printf("Client__save_ID: old ID is:%d, new ID is:%d\n",
				       ID, transID);
			}
			
			// ����ת������ ID
			memcpy(buf, &transID, sizeof(unsigned short));
			record_Ptr record_ptr = &tableId->records[curIndex];
			
			//ֻ����IPv4��Ϣ
			if (UrlInDns(buf, record_ptr->url))
			{
				record_ptr->Len =
					strlen(record_ptr->url);
			} else{
				record_ptr->Len = 0;
			}

			//���Ͼɵ���Ϣˢ�µ����ҵ��ɷŵ�index���ֱ���˳�ѭ����û��Ҫ������λ����
			record_ptr->question_Id = ID;
			record_ptr->client_address = client_addr;
			record_ptr->finished = FALSE;
			record_ptr->time = globalTime;
			curIndex = (curIndex + 1 + ID_TABLE_SIZE) % ID_TABLE_SIZE;
			break;
		}

		curIndex = (curIndex + 1 + ID_TABLE_SIZE) % ID_TABLE_SIZE;
	} while (curIndex != tableId->size);
	
	//�ӵ�ǰλ�ü�����������Ҫ������ID����IDת��
	tableId->size = curIndex;

	return transID;
}

//void free_table_ID(table_ID tableId)
//{
//	for (auto &item : tableId) {
//		free(item.second.url);
//	}
//	tableId.clear();
//}

