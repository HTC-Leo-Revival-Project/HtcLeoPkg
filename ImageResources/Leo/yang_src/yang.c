/* 
 * HTCFlasher - http://htc-flasher.googlecode.com
 *
 * YANG: Yet Another NBH Generator
 * Copyright (C) 2008 Pau Oliva Fora - pof @ <eslack.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * opinion) any later version. See <http://www.gnu.org/licenses/gpl.html>
 *
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include "nbh.h"

void help_show_message()
{
	fprintf(stderr, "Usage: yang [options]\n\n");
	fprintf(stderr, " Exctract NBH options:\n");
	fprintf(stderr, "    -X [filename]       Extract NBH file contents\n\n");
	fprintf(stderr, " Generate NBH options:\n");
	fprintf(stderr, "    -F [filename]       Output NBH filename\n");
	fprintf(stderr, "    -f [file1,file2...] Comma separated list of input NB files\n");
	fprintf(stderr, "    -t [type1,type2...] Comma separated list of input NB types\n");
	fprintf(stderr, "    -s [chunksize]      SignMaxChunkSize (64 or 1024)\n");
	fprintf(stderr, "    -d [device]         Device name (Model ID)\n");
	fprintf(stderr, "    -c [CID]            CID field\n");
	fprintf(stderr, "    -v [version]        Version number\n");
	fprintf(stderr, "    -l [language]       Language\n\n");
	//fprintf(stderr, "Example:\n");
	//fprintf(stderr, " yang -F RUU_signed.nbh -f MainSplash.nb,OS.nb -t 0x600,0x400 -s 64 -d KAIS***** -c SuperCID -v 1.0 -l WWE\n");
#ifdef WIN32
	// GUI idiot proof ;)
	fprintf(stderr, "Press ENTER to quit\n\n");
	getchar();
#endif
}

int32_t main(int32_t argc, char **argv)
{
	int32_t c,i;
	int32_t totalnbfiles;
	int32_t totaltypes;
	int32_t SignMaxChunkSize;

	char *filename;
	char *types;
	char *type;
	char *nbfiles;
	char *tempnbfiles;
	char *nbfile;
	char magicHeader2[]={'R','0','0','0','F','F','\n'};
	char initialsignature[]={'X' ,'D' ,'A' ,'-' ,'D' ,'e' ,'v' ,'e' ,'l' ,'o' ,'p' ,'e' ,'r' ,'s' ,'!' ,' ' };

	FILE *nbh;
	FILE *dbh;
	FILE *nb;

	uint32_t offset;
	uint32_t totalsize;
	uint32_t blockLen;
	uint32_t sectionlen;
	uint32_t signLen;
	uint32_t totalwrite;
	uint32_t lps;
	uint32_t magicHeader[]={'H','T','C','I','M','A','G','E'};

	unsigned char signature[5000];
	unsigned char flag;

	struct HTCIMAGEHEADER header;

	printf ("=== yang v1.1: Yet Another NBH Generator\n");
	printf ("=== (c) 2008 Pau Oliva Fora - pof @ XDA-Developers\n\n");

#ifdef WIN32
	// drag & drop support for win32
	if (argc == 2) {
		printf ("[] Extract NBH file '%s'\n",argv[1]);
		extractNBH(argv[1]);
		fprintf(stderr, "Press ENTER to quit\n\n");
		getchar();
		return 0;
	}
#endif

	if ((argc == 3) && !strcmp(argv[1], "-X")) {
		printf ("[] Extract NBH file '%s'\n",argv[2]);
		extractNBH(argv[2]);
		return 0;
	}

	if (argc < 17) {
		help_show_message();
		return 1;
	}

	while ((c = getopt(argc, argv, "F:d:c:v:l:s:f:t:")) != -1) {
		switch(c) {
		case 'F':
			filename = optarg;
			printf("[] Output NBH file: %s\n", filename);
			break;
		case 'd':
			strncpy(header.device, optarg, 32);
			printf("[] Device: %s\n", header.device);
			break;
		case 'c':
			strncpy(header.CID, optarg, 32);
			printf("[] CID: %s\n", header.CID);
			break;
		case 'v':
			strncpy(header.version, optarg, 16);
			printf("[] Version: %s\n", header.version);
			break;
		case 'l':
			strncpy(header.language, optarg, 16);
			printf("[] Language: %s\n", header.language);
			break;
		case 's':
			SignMaxChunkSize = atoi(optarg);
			printf("[] SignMaxChunkSize: %d\n", SignMaxChunkSize);
			break;
		case 'f':
			nbfiles = optarg;
			printf("[] Input files: %s\n", nbfiles);
			break;
		case 't':
			types = optarg;
			printf("[] Input types: %s\n", types);
			break;
		default:
			help_show_message();
			return 0;
			break;

		}
	}

	printf ("\n");

	switch (SignMaxChunkSize) {
		case 64:
			blockLen = 0x10000;
			signLen = 128;
			break;
		case 1024:
			blockLen = 0x100000;
			signLen = 128;
			break;
		default:
			fprintf(stderr, "[!!] SignMaxChunkSize must be 64 or 1024\n");
			return 1;
			break;
	}

	memset(header.sectiontypes, 0, sizeof(header.sectiontypes));
	memset(header.sectionoffsets, 0, sizeof(header.sectionoffsets));
	memset(header.sectionlengths, 0, sizeof(header.sectionlengths));
	memset(signature, 0, sizeof(signature));

	/* get all section types from command line and write them in the header */
	for (i=0; i<32; i++) {
		if (i == 0) 
			type = strtok(types, ",");
		else
			type = strtok(NULL, ",");
		if (type == NULL) {
			totaltypes = i--;
			break;
		}

		//printf("\n[] Section type: %s\n", type);
		sscanf(type, "0x%lx", &header.sectiontypes[i]);
	}


	/* calculate offset for first section type */
	offset = sizeof(magicHeader) + sizeof(HTCIMAGEHEADER);
	//printf ("[] First section offset: %lx\n", offset);

	tempnbfiles = strdup(nbfiles);

	/* get all sizes and offset position for all nb files and write them in the header*/
	for (i=0; i<32; i++) {
		if (i == 0)
			nbfile = strtok(nbfiles, ",");
		else
			nbfile = strtok(NULL, ",");
		if (nbfile == NULL) {
			totalnbfiles = i--;
			break;
		}

		nb = fopen(nbfile, "rb");
		if (nb == NULL) {
			fprintf(stderr, "[!!] Could not open '%s'\n", nbfile);
			fclose(dbh);
			return 1;
		}
	
		fseek(nb, 0, SEEK_END);
		sectionlen = ftell(nb);
		fclose(nb);

		header.sectionoffsets[i]=offset;
		header.sectionlengths[i]=sectionlen;

		//printf ("header.sectionoffsets[%d]: %lx\n",i, header.sectionoffsets[i]);
		//printf ("header.sectionlengths[%d]: %lx\n",i, header.sectionlengths[i]);
		offset += sectionlen;
	}

	strcpy(nbfiles, tempnbfiles);
	free(tempnbfiles);

	/* sanity check, header types and nb files must match */
	if (totaltypes != totalnbfiles) {
		fprintf(stderr, "[!!] Total section types does not match total input NB files\n");
		return 1;
	}

	/* we have the header, now write the DBH file */
        dbh = fopen("tempfile.dbh","wb");
        if (dbh == NULL) {
                fprintf(stderr, "[!!] Could not open 'tempfile.dbh'\n");
                return 1;
        }

	/* write the DBH header */
	fwrite(magicHeader, 1, sizeof(magicHeader), dbh);
	fwrite(&header, 1, sizeof(HTCIMAGEHEADER), dbh);

	/* create the DBH file including all NB files */
	for (i=0; i<totalnbfiles; i++) {
		if (i == 0) 
			nbfile = strtok(nbfiles, ",");
		else
			nbfile = strtok(NULL, ",");

		if (nbfile == NULL) break;

		printf("[] 0x%lx --> %s\n", header.sectiontypes[i],nbfile);

		nb = fopen(nbfile, "rb");
		if (nb == NULL) {
			fprintf(stderr, "[!!] Could not open '%s'\n", nbfile);
			fclose(dbh);
			return 1;
		}
	
		fseek(nb, 0, SEEK_END);
		sectionlen = ftell(nb);
		fseek(nb, 0, SEEK_SET);

		bufferedReadWrite(nb, dbh, sectionlen);	

		fclose(nb);
	}

	fclose(dbh);

	/* we have the DBH, now write the NBH */

	nbh = fopen(filename, "wb");
	if (nbh == NULL) {
		fprintf(stderr, "[!!] Could not open '%s'\n", filename);
		return 1;
	}

	fwrite(magicHeader2, 1, sizeof(magicHeader2), nbh);
	fwrite(initialsignature, 1, sizeof(initialsignature), nbh);

	dbh = fopen("tempfile.dbh", "rb");
	if (dbh == NULL) {
		fprintf(stderr, "[!!] Could not open 'tempfile.dbh'\n");
		fclose(nbh);
		return 1;
	}

	fseek(dbh, 0, SEEK_END);
	totalsize = ftell(dbh);
	fseek(dbh, 0, SEEK_SET);
	totalwrite = 0;
	flag = 1;

	do {
		if ((totalsize - totalwrite) < blockLen)
			lps = totalsize - totalwrite;
		else 
			lps = blockLen;
		//printf ("blockLen: %lx - totalsize: %lx - totalwrite: %lx\n", lps, totalsize, totalwrite);
		fwrite(&lps, 4, 1, nbh);
		fwrite(&signLen, 4, 1, nbh);
		fwrite(&flag, 1, 1, nbh);
		bufferedReadWrite(dbh, nbh, lps);
		fwrite(signature, 1, signLen, nbh);
		totalwrite += lps;
	} while (totalwrite < totalsize);
	
	fclose(dbh);
	unlink("tempfile.dbh");

	/* write the last block */
	blockLen = 0;
	flag = 2;
	fwrite(&blockLen, 4, 1, nbh);
	fwrite(&signLen, 4, 1, nbh);
	fwrite(&flag, 1, 1, nbh);
	fwrite(signature, 1, signLen, nbh);

	fclose(nbh);
	printf("\nDone!\n");
	return 0;
}