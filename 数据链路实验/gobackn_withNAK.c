#include <stdio.h>
#include <string.h>

#include "protocol.h"
#include "datalink.h"

typedef enum { false, true } boolean;

#define DATA_TIMER  2000
#define ACK_TIMER 1000
#define MAX_SEQ 31
#define inc(k) if(k<MAX_SEQ) k=k+1;else k=0

struct FRAME {
	unsigned char kind; /* FRAME_DATA */
	unsigned char ack;
	unsigned char seq;
	unsigned char data[PKT_LEN];
	unsigned int  padding;
};

static int phl_ready = 0;
static boolean is_nak = false;

static boolean between(unsigned char a, unsigned char b, unsigned char c)
{
	if ((a <= b) && (b < c) || ((c < a) && (a <= b)) || ((b < c) && (c < a)))
		return true;
	else
		return false;
}

//发送数据帧 
static void put_frame1(unsigned char fk, unsigned char frame_expected)
{
	struct FRAME s;
	s.kind = fk;
	s.ack = (frame_expected + MAX_SEQ) % (MAX_SEQ + 1);
	unsigned char *frame = (unsigned char *)&s;
	int len=2;
	switch (fk){
	case FRAME_ACK:
		*(unsigned int *)(frame + len) = crc32(frame, len);
		send_frame(frame, len + 4);
		dbg_frame("Send ACK %d\n", s.ack);
		break;
	case FRAME_NAK:
		*(unsigned int *)(frame + len) = crc32(frame, len);
		send_frame(frame, len + 4);
		dbg_frame("Send NAK %d\n", s.ack);
		break;
	}
	stop_ack_timer();
	phl_ready = 0;
}

//发送ACK或NAK帧 
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
	unsigned char nbuffered = 0;
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
			if (len < 5 || crc32((unsigned char *)&r, len) != 0) {
				dbg_event("**** Receiver Error, Bad CRC Checksum\n");
				if (is_nak == false){
					put_frame1(FRAME_NAK, frame_expected);
					is_nak = true;
				}
				break;
			}
			while (between(ack_expected, r.ack, next_frame_to_send)){
				nbuffered--;
				stop_timer(ack_expected);
				inc(ack_expected);
			}
			switch (r.kind){
			case FRAME_DATA:
				if (r.seq == frame_expected){
					dbg_frame("RECV DATA %d %d, ID %d\n", r.seq, r.ack, *(short *)r.data);
					put_packet(r.data, len - 7);
					is_nak = false;
					start_ack_timer(ACK_TIMER);
					inc(frame_expected);
				}
				else if (r.seq > frame_expected && is_nak == false){
					put_frame1(FRAME_NAK, frame_expected);
					is_nak = true;
				}
				break;
			case FRAME_ACK:
				dbg_frame("RECV ACK %d\n", r.ack);
				break;
			case FRAME_NAK:
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
			dbg_event("---- DATA %d timeout\n", arg);
			next_frame_to_send = ack_expected;
			for (int i = 1; i <= nbuffered; i++){
				put_frame2(FRAME_DATA, next_frame_to_send, frame_expected, buffer);
				inc(next_frame_to_send);
			}
			break;

		case ACK_TIMEOUT:
			put_frame1(FRAME_ACK, frame_expected);
		}

		if (nbuffered < MAX_SEQ && phl_ready)
			enable_network_layer();
		else
			disable_network_layer();
	}
}
