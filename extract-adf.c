/*
 * extract-adf.c 
 *
 * (C)2008 Michael Steil, http://www.pagetable.com/
 * Do whatever you want with it, but please give credit.
 *
 * 2011 Sigurbjorn B. Larusson, http://www.dot1q.org/
 *
 * Hack to restore the file path and to pass the adf file and start sector/end sector used as an argument 
 * This makes it possible to use on any OFS adf file and on HD OFS floppies and to tune where to start and end the process
 * for any other purpose
 *
 * Orphaned files still end up where ever you launched the binary, you'll have to manually move them into the structure if you
 * know where the files should be located
 *
 * Also killed all output unless DEBUG is defined as 1 or higher, you can easily enable it (along with even more debugging
 * output from all the crap I added) by defining DEBUG as 1...
 *
 */

#include <libc.h>
#include <sys/_endian.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <inttypes.h>

// These are defaults
#define SECTORS 1760
#define FIRST_SECTOR 513 /* for kick10.adf */

// These are hard coded, no way to set them
#define	T_HEADER 2
#define	T_DATA 8
#define	T_LIST 16

// Set this to 1 (or anything higher) if you want debugging information printed out
#define DEBUG 0

// Maximum AmigaDOS filename length
#define MAX_FILENAME_LENGTH 31
// Maximum path depth, I'm not sure there is an explicit limit, but the max path length is 255 characters, so it can
// never go over that (or even reach it), we'll use 256 just to be safe
#define MAX_PATH_DEPTH 256

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

// Default define of sector union size
union sector sector[SECTORS];

#define DATABYTES (sizeof(union sector)-sizeof(struct blkhdr))


// Print usage information
void usage() {
	fprintf(stderr,"Extract-ADF (C)2008 Michael Steil\n");
        fprintf(stderr,"Later time file path restoration and selectable ADF filename additions by Sigurbjorn B. Larusson\n");
        fprintf(stderr,"\nUsage: extract-adf <filename> [start sector] [end sector]\n");
	fprintf(stderr,"\nThe defaults for start and end sector are 512 and 1760 respectively since this tool was originally\n"),
	fprintf(stderr,"created to salvage lost data from kickstart disks (which contain the kickstart on sectors 0..512)\n");
	fprintf(stderr,"\nTo specify an end sector you must also specify a start sector, but you can specify only a start\n");
	fprintf(stderr,"sector in which case the end sector defaults to 1760 which is correct for a DD floppy\n");
	fprintf(stderr,"\nTo use this tool on a HD floppy, the end sector needs to be 3520\n");
	fprintf(stderr,"\n\nIf you get a Bus error it means that you specificed a non-existing end sector\n");
}

void printfilename(int i) {
	if (ntohl(sector[i].hdr.type) != T_HEADER)
		return;
	if (sector[i].fh.parent) {
		printfilename(ntohl(sector[i].fh.parent));
	}
	fprintf(stdout,"/%s", sector[i].fh.filename);
}

int main(int argc,char **argv) {
	// The Filepointer used to write the file to the disk
	FILE *f;
	// Temporary variables
	int i,j,n;
	uint32_t type, header_key;
	// To store the name of the file
	char filename[31];
	/* Generate a array of strings to store the filepath and each directory name used in it */
	char **filepath;
	/* A integer to store the current working directory */
	int cwd;
	/* A integer to hold the start sector, defaults to the defined value FIRST_SECTOR */
	int startsector = FIRST_SECTOR;
	/* A integer to hold the last sector, defaults to the defined value SECTORS */
	int endsector = SECTORS;
	/* Variable to hold the debugging value */
	int debug = DEBUG;
	/* Allocate space for MAX_PATH_DEPTH rows in filepath[X][] */
	filepath = malloc(MAX_PATH_DEPTH * sizeof(char *));
	if(filepath == NULL) {
		fprintf(stderr, "Out of memory\n");
		return 1;
	}
	for(i = 0; i < MAX_PATH_DEPTH ; i++) {
		/* Allocate space for MAX_FILENAME_LENGTH entries in filepath[X][Y] */
		filepath[i] = malloc(MAX_FILENAME_LENGTH * sizeof(char));
		if(filepath[i] == NULL) {
			fprintf(stderr, "Out of memory\n");
			return 1;
		}
	}

	// We got passed an argument, which is probably the filename
	if(argc > 1) {
		// Open the file
		f = fopen(argv[1],"r");
		if(f == NULL) {
			fprintf(stderr,"Failed to open file %s, exiting\n",argv[1]);
			return 1;
		}
		// Optionally set the start sector?
		if(argc > 2) {
			i=strtoimax(argv[2],NULL,10);
			// Not an integer or value over 3520 (last sector on a HD adf), print usage
			if(i>3520 || i <0 || i>endsector) {
				usage();
				return 1;
			} else {
				startsector = i;
			}
		}
		// Optionally set the end sector?
		if(argc > 3) {
			i=strtoimax(argv[3],NULL,10);
			// Not an integer or value over 3520 (last sector on a HD adf, print usage
			if(i>3520 || i<0 || i <startsector) {
				usage();
				return 1;
			} else {
				endsector = i;
			}
		}
	} else {
		// No arguments, print usage
		usage();
		return 1;
	}
	fprintf(stdout,"Endsector is %d\n",startsector);
	fprintf(stdout,"Endsector is %d\n",endsector);

	// Redefine the sector array based on the endsector
	union sector sector[endsector];
	
	// Read from the file into the sector array
	int r = fread(sector, sizeof(union sector), endsector, f);
	if(debug)
		fprintf(stdout,"sectors: %d\n", r);

	// Not enough sectors read?
	if(r < (endsector-startsector)) {
		fprintf(stderr,"Only managed to read %d sectors out of %d requested, cowardly refusing to continue\n",r,(endsector-startsector));
		return 1;
	}
	// Close the ADF file
	fclose(f);

	// Loop through the esctors we are supposed to read and recover the data
	for (i=startsector; i<endsector; i++) {
		type = ntohl(sector[i].hdr.type);
		if (type != T_HEADER && type != T_DATA && type != T_LIST)
			continue;
		if(debug) {
			fprintf(stdout,"%x: type       %x\n", i, ntohl(sector[i].hdr.type));
			fprintf(stdout,"%x: header_key %x\n", i, ntohl(sector[i].hdr.header_key));
			fprintf(stdout,"%x: seq_num    %x\n", i, ntohl(sector[i].hdr.seq_num));
			fprintf(stdout,"%x: data_size  %x\n", i, ntohl(sector[i].hdr.data_size));
			fprintf(stdout,"%x: next_data  %x\n", i, ntohl(sector[i].hdr.next_data));
			fprintf(stdout,"%x: chksum     %x\n", i, ntohl(sector[i].hdr.chksum));
		}
		switch (type) {
			case T_HEADER:
				if(debug) {
				  fprintf(stdout,"%x:  filename  \"%s\"\n", i, sector[i].fh.filename);
				  fprintf(stdout,"%x:  byte_size %d\n", i, ntohl(sector[i].fh.byte_size));
				}
				break;
			case T_DATA:
				header_key = ntohl(sector[i].hdr.header_key);
				if (header_key<SECTORS && ntohl(sector[header_key].hdr.type) == T_HEADER) {
					if(debug) {
						fprintf(stdout,"%x:  filename  \"%s\"\n", i, sector[header_key].fh.filename);
						fprintf(stdout,"%x:  byte_size %d\n", i, ntohl(sector[header_key].fh.byte_size));
					}
					strncpy(filename, sector[header_key].fh.filename, sizeof(filename));
				} else {
					snprintf(filename, sizeof(filename), "%04d.txt", header_key);
				}
				if(debug)
					fprintf(stdout,"XXX \"%s\" %03d: ", filename, ntohl(sector[i].hdr.seq_num));
				// Find the file path and put it into the filepath array
				j = 0; n=header_key;
				// If this is a regular file (has a regular filename, and a directory structure) find the path
				while( n != 0 && header_key<SECTORS && ntohl(sector[header_key].hdr.type) == T_HEADER) {
					if(sector[n].fh.parent && n != 880)  {
						// Get the path entry name into the filepath array
						strncpy(filepath[j], sector[ntohl(sector[n].fh.parent)].fh.filename,sizeof(filepath[j]));
						//if(debug)
							fprintf(stdout,"In loop iteration %d, found filepath %s\n",j,filepath[j]);
						// Set n as the parent to recurse backwards
					 	n = ntohl(sector[n].fh.parent);
						// If the parent is the root sector we don't need more iterations	
						if(n == 880) {
							if(debug)
								fprintf(stdout,"Parent is root block 880, stopping this loop");
							n=0;
						} else {
							// Increment loop count by 1
							j++;
						}
					} else {
					 	n=0;
					}
				}
				// Open the current directory so we can return to it later
				cwd = open(".",O_RDONLY);
				if(cwd == -1 ) {
					fprintf(stderr,"Can't open current directory, exiting\n");
					return 1;
				}
				// Reverse loop through the filepath to create any directories that we need if there was a 
				// filepath for this file
				if(j>0) {
					for(n=j;n>=0;n--) {
						// If we can't create the directory, and it's not because it already exists 
						// That will happen quite a bit since this code is executed once per block, 
						// and the same file (and directory) can be passed over hundreds of times. 
						// This isn't very efficient, but it does work.
						if(mkdir(filepath[n],0777) < 0 && errno != EEXIST) {
							fprintf(stderr,"Can't create directory %s, exiting\n",filepath[n]);
						} else {
							if(debug)
								fprintf(stdout,"Created directory %s\n",filepath[n]);
							// Change directory to the newly created directory
							if(chdir(filepath[n]) == -1) {
								fprintf(stderr,"Can't change to newly created directory, exiting\n");
								return 1;
							} else {
								if(debug)
									fprintf(stdout,"Changing directory to %s\n",filepath[n]);
							}
						}
					}
				}
				// Open the file for appending (in the current directory with the path intact)
				f = fopen(filename, "r+"); /* try to open existing file */
				if (!f) 
					f = fopen(filename, "w"); /* doesn't exist, so create */
				// Return to the previous working directory
				if(fchdir(cwd) == -1) {
					fprintf(stderr,"Can't return to previous working directory, exiting\n");
					return 1;
				}
				// Close the previously opened working directory
				close(cwd);

				if(debug) {
					fprintf(stdout,"XXX \"");
					printfilename(header_key);
					fprintf(stdout,"\" %03d: ", ntohl(sector[i].hdr.seq_num));
				}

				// Dumper function that dumps out ascii text, isn't really useful, only active if you enable debug
				if(debug) {
					for (j=0; j<16; j++) {
						uint8_t c = sector[i].dh.data[j];
						if ((c>=32 && c<128)||c==9) {
							fprintf(stdout,"%c", c);
						} else {
							fprintf(stdout,"<0x%02x>", c);
						}
					}
					fprintf(stdout,"\n");
				}
				fseek(f, (ntohl(sector[i].hdr.seq_num)-1)*DATABYTES, SEEK_SET);
				if(debug)
					fprintf(stdout,"seek to %ld\n",  (ntohl(sector[i].hdr.seq_num)-1)*DATABYTES);
				fwrite(sector[i].dh.data, ntohl(sector[i].hdr.data_size), 1, f);
				fclose(f);
		}
		if(debug)
			fprintf(stdout,"\n");
	}

	// Free the space allocated for the filepath array, in reverse order to the malloc obviously
	for(i=0; i< MAX_FILENAME_LENGTH; i++) {
		free(filepath[i]);
	}
	free(filepath);

	// Successful run	
	return 0;
}
