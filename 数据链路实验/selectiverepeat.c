#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "protocol.h"
#include "datalink.h"

#define MAX_DATA 256
#define MAX_SEQ 15 
#define MAX_DATA_TIME 4000
#define MAX_ACK_TIME 280
#define NR_BUFS ((MAX_SEQ + 1) / 2)
#define inc(seq) if (seq < MAX_SEQ) seq = seq + 1; else seq = 0;

typedef unsigned int seq_nr;
typedef unsigned char frame_kind;
typedef unsigned char seq_ack;
typedef unsigned char Packet[MAX_DATA];

bool no_nak = true;
bool phl_ready = false;



typedef struct {
	frame_kind kind;   // ֡������
	seq_ack ack; // ʹ���Ӵ�ȷ�ϻ��ƣ����һ����ȷ�ϵ�֡�����
	unsigned char seq;           // ֡���
	unsigned char data[MAX_DATA]; // ���ݲ���
	unsigned char padding[4];    // 4�ֽ�-CRC32У���ֶ�
} Frame;




// �ж��Ƿ����ڴ�����
static bool between(seq_nr a, seq_nr b, seq_nr c)
{
	return ((a <= b && b < c) || (c < a && a <= b) || (c < a && b < c));
}

static void put_frame(unsigned char* frame, int len)
{
	*(unsigned int*)(frame + len) = crc32(frame, len);
	send_frame(frame, len + 4);
	phl_ready = false;
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
	}
	else if (kind == FRAME_NAK) {
		//send_f.ack = frame_expected;
		no_nak = false;
		dbg_frame("Send NAK %d\n", (send_f.ack + 1) % (MAX_SEQ + 1));
		send_frame((unsigned char*)&send_f, 3);
	}
	else
	{
		dbg_frame("Send ACK %d\n", send_f.ack);
		send_frame((unsigned char*)&send_f, 3);
	}
	stop_ack_timer();
}

int main(int argc, char** argv)
{
	int len;
	int event, arg;

	seq_nr ack_expected = 0; // �����յ�ACK��֡��ţ����ʹ��ڵ��½�
	seq_nr next_frame_to_send = 0; // ��һ��������֡��ţ����ʹ��ڵ��Ͻ�+1
	seq_nr frame_expected = 0; // �����յ���֡���,���մ����½�
	seq_nr too_far = NR_BUFS;//���մ����Ͻ�
	seq_nr count_frame = 0; // ���յ���֡������
	Frame receiver_frame; // ���յ���֡     

	bool arrived[NR_BUFS];//���ÿ��֡�Ƿ񱻽����߽���
	Packet send_buf[NR_BUFS];// ���ʹ���
	Packet rece_buf[NR_BUFS];//���մ���

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
		case NETWORK_LAYER_READY: // ��������
			count_frame++;
			get_packet(send_buf[next_frame_to_send % NR_BUFS]);
			send_any_frame(FRAME_DATA, next_frame_to_send, frame_expected, send_buf); // ����DATA֡
			inc(next_frame_to_send);
			break;

		case PHYSICAL_LAYER_READY:
			phl_ready = 1;
			break;

		case FRAME_RECEIVED:
			len = recv_frame((unsigned char*)&receiver_frame, sizeof receiver_frame);
			if (len >= 6 && crc32((unsigned char*)&receiver_frame, len) != 0) // У�����
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
					//inc(frame_expected);
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

			if (receiver_frame.kind == FRAME_ACK)
				dbg_frame("Recv ACK %d\n", receiver_frame.ack);


			if (receiver_frame.kind == FRAME_NAK && between(ack_expected, (receiver_frame.ack + 1) % (MAX_SEQ + 1), next_frame_to_send)) {
				send_any_frame(FRAME_DATA, (receiver_frame.ack + 1) % (MAX_SEQ + 1), frame_expected, send_buf);
				dbg_frame("Recv NAK  %d\n", (receiver_frame.ack + 1) % (MAX_SEQ + 1), len);
			}

			while (between(ack_expected, receiver_frame.ack, next_frame_to_send)) // �ۼ�ȷ��
			{
				count_frame--;
				stop_timer(ack_expected);
				inc(ack_expected);
			}
			break;

		case DATA_TIMEOUT:
			dbg_event("---- DATA %d timeout ----\n", arg);
			if (!between(ack_expected, arg, next_frame_to_send))
				arg += NR_BUFS;
			send_any_frame(FRAME_DATA, arg, frame_expected, send_buf);
			break;

		case ACK_TIMEOUT:
			dbg_event("---- ACK %d timeout ----\n", frame_expected);
			send_any_frame(FRAME_ACK, 0, frame_expected, send_buf);
			break;
		}


	}
}
