//
// Created by 27711 on 2023-06-23.
//

#include "ID_Transformer.h"
#include "package_Transformer.h"

//初始化ID表
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


//从服务器端收到报文后进行ID转换
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

//从客户端收到报文后进行ID转换并保存
unsigned short save_ID(table_ID_Ptr tableId, char *buf, unsigned short ID,
		       SOCKADDR_IN client_addr)
{
	unsigned short transID = 0;
	
	// 寻找空闲的记录或插入新记录

	int curIndex = tableId->size;

	/*if (curIndex >= ID_TABLE_SIZE) {
		return -1;
	}*/

	do {
		//超时就将ID释放去给其他报文使用，找到一个能放ID的位置就直接放。没必要遍历，在找的时候捎带刷新
		if ((globalTime - tableId->records[curIndex].time + TIME_SIZE) % TIME_SIZE > TTL) {
			tableId->records[curIndex].finished = TRUE;
		}

		if (tableId->records[curIndex].finished) {
			// 更新记录的值(根据RFC文档可知ID为0被视为无效或保留的值，有特殊目的，所以transID必须从1开始)
			transID = (unsigned short)(curIndex + 1);
			
			if (debug_level == 2) {
				printf("Client__save_ID: old ID is:%d, new ID is:%d\n",
				       ID, transID);
			}
			
			// 更新转发包的 ID
			memcpy(buf, &transID, sizeof(unsigned short));
			record_Ptr record_ptr = &tableId->records[curIndex];
			
			//只处理IPv4信息
			if (UrlInDns(buf, record_ptr->url))
			{
				record_ptr->Len =
					strlen(record_ptr->url);
			} else{
				record_ptr->Len = 0;
			}

			//把老旧的信息刷新掉，找到可放的index后就直接退出循环，没必要继续找位置了
			record_ptr->question_Id = ID;
			record_ptr->client_address = client_addr;
			record_ptr->finished = FALSE;
			record_ptr->time = globalTime;
			curIndex = (curIndex + 1 + ID_TABLE_SIZE) % ID_TABLE_SIZE;
			break;
		}

		curIndex = (curIndex + 1 + ID_TABLE_SIZE) % ID_TABLE_SIZE;
	} while (curIndex != tableId->size);
	
	//从当前位置继续给接下来要到来的ID进行ID转换
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

