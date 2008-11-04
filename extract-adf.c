/*
 * extract-adf.c 
 * (C)2008 Michael Steil, http://www.pagetable.com/
 * Do whatever you want with it, but please give credit.
 */

#include <libc.h>
#include <sys/_endian.h>

#define SECTORS 1760
#define FIRST_SECTOR 513 /* for kick10.adf */

#define	T_HEADER 2
#define	T_DATA 8
#define	T_LIST 16

typedef unsigned int uint32_t;

struct sector_raw {
	uint8_t byte[512];
};

struct blkhdr {
	uint32_t type;
	uint32_t header_key;
	uint32_t seq_num;
	uint32_t data_size;
	uint32_t next_data;
	uint32_t chksum;
};

struct fileheader {
	struct blkhdr hdr;
	uint8_t misc[288];
	uint32_t unused1;
	uint16_t uid;
	uint16_t gid;
	uint32_t protect;
	uint32_t byte_size;
	uint8_t comm_len;
	uint8_t comment[79];
	uint8_t unused2[12];
	uint32_t days;
	uint32_t mins;
	uint32_t ticks;
	uint8_t name_len;
	char filename[30];
	uint8_t unused3;
	uint32_t unused4;
	uint32_t real_entry;
	uint32_t next_link;
	uint32_t unused5[5];
	uint32_t hash_chain;
	uint32_t parent;
	uint32_t extension;
	uint32_t sec_type;
};

struct dataheader {
	struct blkhdr hdr;
	uint8_t data[488];
};

union sector {
	struct blkhdr hdr;
	struct fileheader fh;
	struct dataheader dh;
};

#define DATABYTES (sizeof(union sector)-sizeof(struct blkhdr))

union sector sector[SECTORS];

void printfilename(int i) {
	if (ntohl(sector[i].hdr.type) != T_HEADER)
		return;
	if (sector[i].fh.parent) {
		printfilename(ntohl(sector[i].fh.parent));
	}
	printf("/%s", sector[i].fh.filename);
}

int main() {
	FILE *f;
	int i, j;
	uint32_t type, header_key;
	char filename[31];
	
	f = fopen("kick10.adf", "r");
	int r = fread(sector, sizeof(union sector), SECTORS, f);
	printf("sectors: %d\n", r);
	fclose(f);

	for (i=FIRST_SECTOR; i<SECTORS; i++) {
		type = ntohl(sector[i].hdr.type);
		if (type != T_HEADER && type != T_DATA && type != T_LIST)
			continue;
		printf("%x: type       %x\n", i, ntohl(sector[i].hdr.type));
		printf("%x: header_key %x\n", i, ntohl(sector[i].hdr.header_key));
		printf("%x: seq_num    %x\n", i, ntohl(sector[i].hdr.seq_num));
		printf("%x: data_size  %x\n", i, ntohl(sector[i].hdr.data_size));
		printf("%x: next_data  %x\n", i, ntohl(sector[i].hdr.next_data));
		printf("%x: chksum     %x\n", i, ntohl(sector[i].hdr.chksum));
		switch (type) {
			case T_HEADER:
				printf("%x:  filename  \"%s\"\n", i, sector[i].fh.filename);
				printf("%x:  byte_size %d\n", i, ntohl(sector[i].fh.byte_size));
				break;
			case T_DATA:
				header_key = ntohl(sector[i].hdr.header_key);
				if (header_key<SECTORS && ntohl(sector[header_key].hdr.type) == T_HEADER) {
					printf("%x:  filename  \"%s\"\n", i, sector[header_key].fh.filename);
					printf("%x:  byte_size %d\n", i, ntohl(sector[header_key].fh.byte_size));
					strncpy(filename, sector[header_key].fh.filename, sizeof(filename));
				} else {
					snprintf(filename, sizeof(filename), "%04d.txt", header_key);
				}
//				printf("XXX \"%s\" %03d: ", filename, ntohl(sector[i].hdr.seq_num));
				printf("XXX \"");
				printfilename(header_key);
				printf("\" %03d: ", ntohl(sector[i].hdr.seq_num));
				for (j=0; j<16; j++) {
					uint8_t c = sector[i].dh.data[j];
					if ((c>=32 && c<128)||c==9) {
						printf("%c", c);
					} else {
						printf("<0x%02x>", c);
					}
				}
				printf("\n");
				f = fopen(filename, "r+"); /* try to open existing file */
				if (!f) 
					f = fopen(filename, "w"); /* doesn't exist, so create */
				fseek(f, (ntohl(sector[i].hdr.seq_num)-1)*DATABYTES, SEEK_SET);
				printf("seek to %ld\n",  (ntohl(sector[i].hdr.seq_num)-1)*DATABYTES);
				fwrite(sector[i].dh.data, ntohl(sector[i].hdr.data_size), 1, f);
				fclose(f);
		}
		printf("\n");
	}
	
	return 0;
}
