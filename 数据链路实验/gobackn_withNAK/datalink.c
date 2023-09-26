#include <stdio.h>
#include <string.h>

#include "protocol.h"
#include "datalink.h"

typedef enum { false, true } boolean;

#define DATA_TIMER  2000	//据计算，DATA_TIMER应大于1066+ACK_TIMER
#define ACK_TIMER 1000	//ACK_TIMER应大于从网络层获取一个包的时间，经测试1000ms可行
#define MAX_SEQ 31		//经计算，MAX_SEQ需>=5,考虑到MAX_SEQ=2^n-1，此处取31
#define inc(k) if(k<MAX_SEQ) k=k+1;else k=0

struct FRAME {
	unsigned char kind; /* FRAME_DATA等 */
	unsigned char ack;
	unsigned char seq;
	unsigned char data[PKT_LEN];
	unsigned int  padding;	//预留给DATA帧的CRC校验码字段
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

static int phl_ready = 0;	//物理层是否就绪，若未就绪则disable网络层
static boolean is_nak = false;	//当前是否存在NAK，只能存在1个NAK，若当前存在NAK则不会再发送NAK

//判断b是否在a、c之间
static boolean between(unsigned char a, unsigned char b, unsigned char c)
{
	if ((a <= b) && (b < c) || ((c < a) && (a <= b)) || ((b < c) && (c < a)))
		return true;
	else
		return false;
}

//发送ACK或NAK帧
static void put_frame1(unsigned char fk, unsigned char frame_expected)
{
	struct FRAME s;
	s.kind = fk;
	s.ack = (frame_expected + MAX_SEQ) % (MAX_SEQ + 1);	//相当于进行frame_expected-1的模MAX_SEQ减法
	unsigned char *frame = (unsigned char *)&s;
	int len = 2;
	switch (fk){
	case FRAME_ACK:
		*(unsigned int *)(frame + len) = crc32(frame, len);	//在帧尾部附加CRC校验码
		send_frame(frame, len + 4);
		dbg_frame("Send ACK %d\n", s.ack);
		break;
	case FRAME_NAK:
		*(unsigned int *)(frame + len) = crc32(frame, len);
		send_frame(frame, len + 4);
		dbg_frame("Send NAK %d\n", s.ack);
		break;
	}
	stop_ack_timer();	//不管怎样，有反向流量捎带ACK了，故取消ACK——TIMER
	phl_ready = 0;
}

//发送数据帧 
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
	unsigned char nbuffered = 0;	//发送窗口大小，未被确认的包缓存在发送窗口中
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
			//若CRC校验出错，则不接受该帧，在没有NAK存在的情况下发送NAK
			if (len < 5 || crc32((unsigned char *)&r, len) != 0) {
				dbg_event("**** Receiver Error, Bad CRC Checksum\n");
				if (is_nak == false){
					put_frame1(FRAME_NAK, frame_expected);
					is_nak = true;
				}
				break;
			}
			//累计确认机制，收到NAK n表示n及n之前所有帧都被确认
			//同时在发送NAK之前需要先执行此段，释放发送窗口，否则网络层会收到不正确的帧
			while (between(ack_expected, r.ack, next_frame_to_send)){
				nbuffered--;
				stop_timer(ack_expected);
				inc(ack_expected);
			}
			switch (r.kind){
			case FRAME_DATA:
				//向网络层传输包
				if (r.seq == frame_expected){
					dbg_frame("RECV DATA %d %d, ID %d\n", r.seq, r.ack, *(short *)r.data);
					put_packet(r.data, len - 7);
					is_nak = false;
					start_ack_timer(ACK_TIMER);
					inc(frame_expected);
				}
				//最开始设置为r.seq != frame_expected，但会出现反复重传问题，于是忽略r.seq < frame_expected的帧
				else if (r.seq > frame_expected && is_nak == false){
					put_frame1(FRAME_NAK, frame_expected);
					is_nak = true;
				}
				break;
			case FRAME_ACK:
				dbg_frame("RECV ACK %d\n", r.ack);
				break;
			case FRAME_NAK:
				//收到NAK后回退N，发送发送窗口中缓存的所有帧
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
			//超时后回退N，发送发送窗口中缓存的所有帧
			dbg_event("---- DATA %d timeout\n", arg);
			next_frame_to_send = ack_expected;
			for (int i = 1; i <= nbuffered; i++){
				put_frame2(FRAME_DATA, next_frame_to_send, frame_expected, buffer);
				inc(next_frame_to_send);
			}
			break;

		case ACK_TIMEOUT:
			//为及时出现反向流量捎带确认帧，于是发送单独确认帧
			put_frame1(FRAME_ACK, frame_expected);
		}

		if (nbuffered < MAX_SEQ && phl_ready)
			enable_network_layer();
		else
			disable_network_layer();
	}
}