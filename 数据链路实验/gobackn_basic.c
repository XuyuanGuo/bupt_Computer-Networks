#include <stdio.h>
#include <string.h>
#include "protocol.h"
#include "datalink.h"

typedef enum { false, true } boolean;

#define DATA_TIMER  2000
#define MAX_SEQ 7
#define inc(k) if(k<MAX_SEQ) k=k+1;else k=0

struct FRAME {
	unsigned char kind; /* FRAME_DATA */
	unsigned char ack;
	unsigned char seq;
	unsigned char data[PKT_LEN];
	unsigned int  padding;
};

static int phl_ready = 0;

static boolean between(unsigned char a, unsigned char b, unsigned char c){
	if ((a <= b) && (b < c) || ((c < a) && (a <= b)) || ((b < c) && (c < a)))
		return true;
	else
		return false;
}

static void put_frame(unsigned char frame_nr, unsigned char frame_expected, unsigned char buffer[][PKT_LEN])
{

	struct FRAME s;
	s.kind = FRAME_DATA;
	s.ack = (frame_expected + MAX_SEQ) % (MAX_SEQ + 1);
	s.seq = frame_nr;
	memcpy(s.data, buffer[frame_nr], PKT_LEN);
	
	unsigned char *frame = (unsigned char *)&s;
	int len = 3 + PKT_LEN;
	*(unsigned int *)(frame + len) = crc32(frame, len);
	send_frame(frame, len + 4);
	dbg_frame("Send DATA %d %d, ID %d\n", s.seq, s.ack, *(short *)s.data);

	phl_ready = 0;
	start_timer(frame_nr, DATA_TIMER);
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
			put_frame(next_frame_to_send, frame_expected, buffer);
			inc(next_frame_to_send);
			break;

		case PHYSICAL_LAYER_READY:
			phl_ready = 1;
			break;

		case FRAME_RECEIVED:
			len = recv_frame((unsigned char *)&r, sizeof r);
			dbg_frame("Recv DATA %d %d, ID %d\n", r.seq, r.ack, *(short *)r.data);
			if (len < 5 || crc32((unsigned char *)&r, len) != 0) {
				dbg_event("**** Receiver Error, Bad CRC Checksum\n");
				break;
			}
			if (r.seq == frame_expected){
				put_packet(r.data, len - 7);
				inc(frame_expected);
			}
			while (between(ack_expected, r.ack, next_frame_to_send)){
				nbuffered--;
				stop_timer(ack_expected);
				inc(ack_expected);
			}
			break;

		case DATA_TIMEOUT:
			dbg_event("---- DATA %d timeout\n", arg);
			next_frame_to_send = ack_expected;
			for (int i = 1; i <= nbuffered; i++){
				put_frame(next_frame_to_send, frame_expected, buffer);
				inc(next_frame_to_send);
			}
			break;
		}

		if (nbuffered < MAX_SEQ && phl_ready)
			enable_network_layer();
		else
			disable_network_layer();
	}
}


