//
// Created by 27711 on 2023-06-22.
//
#ifndef HEADER_H
#define HEADER_H

#define u16 unsigned short

struct DNS_Header {
	unsigned id : 16;    /* query identification number */
	unsigned rd : 1;     /* recursion desired */
	unsigned tc : 1;     /* truncated message */
	unsigned aa : 1;     /* authoritive answer */
	unsigned opcode : 4; /* purpose of message */
	unsigned qr : 1;     /* response flag */
	unsigned rcode : 4;  /* response code */
	unsigned cd : 1;     /* checking disabled by resolver */
	unsigned ad : 1;     /* authentic data from named */
	unsigned z : 1;      /* unused bits, must be ZERO */
	unsigned ra : 1;     /* recursion available */
	u16 qdcount;         /* number of question entries */
	u16 ancount;         /* number of answer entries */
	u16 nscount;         /* number of authority entries */
	u16 arcount;         /* number of resource entries */
};

#endif
