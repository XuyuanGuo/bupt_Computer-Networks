#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "protocol.h"
#include "datalink.h"

#define MAX_DATA 256
#define MAX_SEQ 15 
#define MAX_DATA_TIME 4000 //270*4+263(256+7)*3(data+ack+data)+7(nak)=1856 捎带确认
#define MAX_ACK_TIME 1000 //实验报告里面写的280ms是基于高误码率设置的，设置短的ACKTIME可以使得帧多次重传以确保接收方收到正确的帧，但可能会带来网络拥塞
//1000 = 263 + 270*2    RTT的1.5到2倍之间
#define NR_BUFS ((MAX_SEQ + 1) / 2)
#define inc(seq) if (seq < MAX_SEQ) seq = seq + 1; else seq = 0;

//0 1 2 3 4 5 6 7
//0 1 2 3

typedef unsigned int seq_nr;
typedef unsigned char frame_kind;
typedef unsigned char seq_ack;
typedef unsigned char Packet[MAX_DATA];

bool no_nak = true;
bool phl_ready = false;



typedef struct {
	frame_kind kind;   // 帧的类型
	seq_ack ack; // 使用捎带确认机制，最后一个被确认的帧的序号
	unsigned char seq;           // 帧序号
	unsigned char data[MAX_DATA]; // 数据部分
	unsigned char padding[4];    // 4字节-CRC32校验字段
} Frame;




// 判断是否落在窗口中
static bool between(seq_nr a, seq_nr b, seq_nr c)
{
	return ((a <= b && b < c) || (c < a && a <= b) || (c < a && b < c));
}

static void put_frame(unsigned char* frame, int len)//尾部加上32位的CRC校验码
{
	*(unsigned int*)(frame + len) = crc32(frame, len);
	send_frame(frame, len + 4);//winsock函数
	phl_ready = false;//通过在发送帧后将其设置为“false”，表明物理层不再准备发送，确保帧按顺序发送和处理，避免并发帧传输可能产生问题或冲突。
}

static void send_any_frame(frame_kind kind, seq_nr next_send_frame, seq_nr frame_expected, Packet buffer[])
{
	Frame send_f;
	send_f.kind = kind;
	send_f.ack = (frame_expected + MAX_SEQ) % (MAX_SEQ + 1);
	if (kind == FRAME_DATA) {
		send_f.seq = next_send_frame;
		memcpy(send_f.data, buffer[next_send_frame % NR_BUFS], MAX_DATA);
		dbg_frame("Send DATA %d %d, ID %d\n", send_f.seq, send_f.ack, *(short*)send_f.data);
		put_frame((unsigned char*)&send_f, 3 + MAX_DATA);
		start_timer(next_send_frame % NR_BUFS, MAX_DATA_TIME);
		//next_send_frame % NR_BUFS计算出发送方缓冲区中启动定时器的帧的索引。用“%NR_BUFS”确保索引保持在发送方缓冲区内。
		//'NR_BUFS'的值表示发送方缓冲区的大小，'next_send_frame'与'NR_BUFS'取模确保索引在有效范围内。
	}
	else if (kind == FRAME_NAK) {

		no_nak = false;
		dbg_frame("Send NAK %d\n", (send_f.ack + 1) % (MAX_SEQ + 1));
		send_frame((unsigned char*)&send_f, 3);
	}
	else
	{
		dbg_frame("Send ACK %d\n", send_f.ack);
		send_frame((unsigned char*)&send_f, 3);
	}
	stop_ack_timer();//确保发送方只等待最近发送帧的ACK
}

int main(int argc, char** argv)
{
	int len;
	int event, arg;

	seq_nr ack_expected = 0; // 期望收到ACK的帧序号，发送窗口的下界
	seq_nr next_frame_to_send = 0; // 下一个发出的帧序号，发送窗口的上界+1
	seq_nr frame_expected = 0; // 期望收到的帧序号,接收窗口下界
	seq_nr too_far = NR_BUFS;//接收窗口上界
	seq_nr count_frame = 0; // 接收到的帧的数量
	Frame receiver_frame; // 接收到的帧     

	bool arrived[NR_BUFS];//标记每个帧是否被接收者接收
	Packet send_buf[NR_BUFS];// 发送窗口
	Packet rece_buf[NR_BUFS];//接收窗口

	for (int i = 0; i < NR_BUFS; i++)
	{
		arrived[i] = false;
	}

	protocol_init(argc, argv);
	lprintf("Designed by WHR, build: " __DATE__ "  "__TIME__"\n");
	disable_network_layer();

	while (true) {

		if (count_frame < NR_BUFS && phl_ready) {
			enable_network_layer();
		}
		else
		{
			disable_network_layer();
		}

		event = wait_for_event(&arg);

		switch (event) {
		case NETWORK_LAYER_READY: // 网络层就绪
			count_frame++;
			get_packet(send_buf[next_frame_to_send % NR_BUFS]);
			send_any_frame(FRAME_DATA, next_frame_to_send, frame_expected, send_buf); // 发送DATA帧
			inc(next_frame_to_send);
			break;

		case PHYSICAL_LAYER_READY:
			phl_ready = 1;
			break;

		case FRAME_RECEIVED:
			len = recv_frame((unsigned char*)&receiver_frame, sizeof receiver_frame);
			if (len >= 6 && crc32((unsigned char*)&receiver_frame, len) != 0) // 校验错误
			{
				dbg_event("**** Receiver Error, Bad CRC Checksum ****\n");
				if (no_nak) {
					send_any_frame(FRAME_NAK, 0, frame_expected, send_buf);
					no_nak = false;
				}
				break;
			}

			if (receiver_frame.kind == FRAME_DATA) {
				dbg_frame("Recv DATA %d %d, ID %d\n", receiver_frame.seq, receiver_frame.ack, *(short*)receiver_frame.data);
				if (receiver_frame.seq != frame_expected && no_nak) {
					dbg_frame("NAK send back\n");
					send_any_frame(FRAME_NAK, 0, frame_expected, send_buf);
					no_nak = false;
				}
				else
				{

					start_ack_timer(MAX_ACK_TIME);
				}

				if (between(frame_expected, receiver_frame.seq, too_far) && arrived[receiver_frame.seq % NR_BUFS] == false) {
					arrived[receiver_frame.seq % NR_BUFS] = true;
					memcpy(rece_buf[receiver_frame.seq % NR_BUFS], receiver_frame.data, MAX_DATA);
					while (arrived[frame_expected % NR_BUFS])
					{
						put_packet(rece_buf[frame_expected % NR_BUFS], MAX_DATA);
						no_nak = true;
						arrived[frame_expected % NR_BUFS] = false;
						inc(frame_expected);
						inc(too_far);
						start_ack_timer(MAX_ACK_TIME);
					}
				}
			}

			if (receiver_frame.kind == FRAME_ACK) {
				dbg_frame("Recv ACK %d\n", receiver_frame.ack);
				while (between(ack_expected, receiver_frame.ack, next_frame_to_send)) // 累计确认
				{
					count_frame--;
					stop_timer(ack_expected);
					inc(ack_expected);
				}
			}



			if (receiver_frame.kind == FRAME_NAK && between(ack_expected, (receiver_frame.ack + 1) % (MAX_SEQ + 1), next_frame_to_send)) {
				send_any_frame(FRAME_DATA, (receiver_frame.ack + 1) % (MAX_SEQ + 1), frame_expected, send_buf);
				dbg_frame("Recv NAK  %d\n", (receiver_frame.ack + 1) % (MAX_SEQ + 1), len);
			}


			break;

		case DATA_TIMEOUT:
			dbg_event("---- DATA %d timeout ----\n", arg);
			if (!between(ack_expected, arg, next_frame_to_send)) {
				arg += NR_BUFS;//如果超时数据帧不在ack_expected与Next_frame_to_send之间，arg的值将调整为下一个缓冲区的索引，以确保使用'send_any_frame'重新发送正确的帧。
			}

			send_any_frame(FRAME_DATA, arg, frame_expected, send_buf);
			break;

		case ACK_TIMEOUT:
			dbg_event("---- ACK %d timeout ----\n", frame_expected);
			send_any_frame(FRAME_ACK, 0, frame_expected, send_buf);
			break;
		}


	}
}