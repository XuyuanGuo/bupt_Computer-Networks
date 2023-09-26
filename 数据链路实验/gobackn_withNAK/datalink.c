#include <stdio.h>
#include <string.h>

#include "protocol.h"
#include "datalink.h"

typedef enum { false, true } boolean;

#define DATA_TIMER  2000	//�ݼ��㣬DATA_TIMERӦ����1066+ACK_TIMER
#define ACK_TIMER 1000	//ACK_TIMERӦ���ڴ�������ȡһ������ʱ�䣬������1000ms����
#define MAX_SEQ 31		//�����㣬MAX_SEQ��>=5,���ǵ�MAX_SEQ=2^n-1���˴�ȡ31
#define inc(k) if(k<MAX_SEQ) k=k+1;else k=0

struct FRAME {
	unsigned char kind; /* FRAME_DATA�� */
	unsigned char ack;
	unsigned char seq;
	unsigned char data[PKT_LEN];
	unsigned int  padding;	//Ԥ����DATA֡��CRCУ�����ֶ�
};

/*
DATA Frame
+=========+========+========+===============+========+
| KIND(1) | SEQ(1) | ACK(1) | DATA(240~256) | CRC(4) |
+=========+========+========+===============+========+

ACK Frame
+=========+========+========+
| KIND(1) | ACK(1) | CRC(4) |
+=========+========+========+

NAK Frame
+=========+========+========+
| KIND(1) | ACK(1) | CRC(4) |
+=========+========+========+
*/

static int phl_ready = 0;	//������Ƿ��������δ������disable�����
static boolean is_nak = false;	//��ǰ�Ƿ����NAK��ֻ�ܴ���1��NAK������ǰ����NAK�򲻻��ٷ���NAK

//�ж�b�Ƿ���a��c֮��
static boolean between(unsigned char a, unsigned char b, unsigned char c)
{
	if ((a <= b) && (b < c) || ((c < a) && (a <= b)) || ((b < c) && (c < a)))
		return true;
	else
		return false;
}

//����ACK��NAK֡
static void put_frame1(unsigned char fk, unsigned char frame_expected)
{
	struct FRAME s;
	s.kind = fk;
	s.ack = (frame_expected + MAX_SEQ) % (MAX_SEQ + 1);	//�൱�ڽ���frame_expected-1��ģMAX_SEQ����
	unsigned char *frame = (unsigned char *)&s;
	int len = 2;
	switch (fk){
	case FRAME_ACK:
		*(unsigned int *)(frame + len) = crc32(frame, len);	//��֡β������CRCУ����
		send_frame(frame, len + 4);
		dbg_frame("Send ACK %d\n", s.ack);
		break;
	case FRAME_NAK:
		*(unsigned int *)(frame + len) = crc32(frame, len);
		send_frame(frame, len + 4);
		dbg_frame("Send NAK %d\n", s.ack);
		break;
	}
	stop_ack_timer();	//�����������з��������Ӵ�ACK�ˣ���ȡ��ACK����TIMER
	phl_ready = 0;
}

//��������֡ 
static void put_frame2(unsigned char fk, unsigned char frame_nr, unsigned char frame_expected, unsigned char buffer[][PKT_LEN])
{
	struct FRAME s;
	s.kind = fk;
	s.ack = (frame_expected + MAX_SEQ) % (MAX_SEQ + 1);
	unsigned char *frame = (unsigned char *)&s;
	int len = 3 + PKT_LEN;
	s.seq = frame_nr;
	memcpy(s.data, buffer[frame_nr], PKT_LEN);
	*(unsigned int *)(frame + len) = crc32(frame, len);
	send_frame(frame, len + 4);
	dbg_frame("Send DATA %d %d, ID %d\n", s.seq, s.ack, *(short *)s.data);
	start_timer(frame_nr, DATA_TIMER);
	stop_ack_timer();
	phl_ready = 0;
}

int main(int argc, char **argv)
{
	unsigned char next_frame_to_send = 0;
	unsigned char frame_expected = 0, ack_expected = 0;
	struct FRAME r;
	unsigned char nbuffered = 0;	//���ʹ��ڴ�С��δ��ȷ�ϵİ������ڷ��ʹ�����
	unsigned char buffer[MAX_SEQ + 1][PKT_LEN];
	int event, arg;
	int len = 0;

	protocol_init(argc, argv);
	lprintf("Designed by Guo Xuyuan, build: " __DATE__"  "__TIME__"\n");

	disable_network_layer();

	while (true) {
		event = wait_for_event(&arg);

		switch (event){
		case NETWORK_LAYER_READY:
			get_packet(buffer[next_frame_to_send]);
			nbuffered++;
			put_frame2(FRAME_DATA, next_frame_to_send, frame_expected, buffer);
			inc(next_frame_to_send);
			break;

		case PHYSICAL_LAYER_READY:
			phl_ready = 1;
			break;

		case FRAME_RECEIVED:
			len = recv_frame((unsigned char *)&r, sizeof r);
			//��CRCУ������򲻽��ܸ�֡����û��NAK���ڵ�����·���NAK
			if (len < 5 || crc32((unsigned char *)&r, len) != 0) {
				dbg_event("**** Receiver Error, Bad CRC Checksum\n");
				if (is_nak == false){
					put_frame1(FRAME_NAK, frame_expected);
					is_nak = true;
				}
				break;
			}
			//�ۼ�ȷ�ϻ��ƣ��յ�NAK n��ʾn��n֮ǰ����֡����ȷ��
			//ͬʱ�ڷ���NAK֮ǰ��Ҫ��ִ�д˶Σ��ͷŷ��ʹ��ڣ������������յ�����ȷ��֡
			while (between(ack_expected, r.ack, next_frame_to_send)){
				nbuffered--;
				stop_timer(ack_expected);
				inc(ack_expected);
			}
			switch (r.kind){
			case FRAME_DATA:
				//������㴫���
				if (r.seq == frame_expected){
					dbg_frame("RECV DATA %d %d, ID %d\n", r.seq, r.ack, *(short *)r.data);
					put_packet(r.data, len - 7);
					is_nak = false;
					start_ack_timer(ACK_TIMER);
					inc(frame_expected);
				}
				//�ʼ����Ϊr.seq != frame_expected��������ַ����ش����⣬���Ǻ���r.seq < frame_expected��֡
				else if (r.seq > frame_expected && is_nak == false){
					put_frame1(FRAME_NAK, frame_expected);
					is_nak = true;
				}
				break;
			case FRAME_ACK:
				dbg_frame("RECV ACK %d\n", r.ack);
				break;
			case FRAME_NAK:
				//�յ�NAK�����N�����ͷ��ʹ����л��������֡
				dbg_frame("RECV NAK %d\n", r.ack);
				if (between(ack_expected, (r.ack + 1) % (MAX_SEQ + 1), next_frame_to_send)){
					next_frame_to_send = (r.ack + 1) % (MAX_SEQ + 1);
					for (int i = 1; i <= nbuffered; i++){
						put_frame2(FRAME_DATA, next_frame_to_send, frame_expected, buffer);
						inc(next_frame_to_send);
					}
				}
				break;
			}
			break;

		case DATA_TIMEOUT:
			//��ʱ�����N�����ͷ��ʹ����л��������֡
			dbg_event("---- DATA %d timeout\n", arg);
			next_frame_to_send = ack_expected;
			for (int i = 1; i <= nbuffered; i++){
				put_frame2(FRAME_DATA, next_frame_to_send, frame_expected, buffer);
				inc(next_frame_to_send);
			}
			break;

		case ACK_TIMEOUT:
			//Ϊ��ʱ���ַ��������Ӵ�ȷ��֡�����Ƿ��͵���ȷ��֡
			put_frame1(FRAME_ACK, frame_expected);
		}

		if (nbuffered < MAX_SEQ && phl_ready)
			enable_network_layer();
		else
			disable_network_layer();
	}
}