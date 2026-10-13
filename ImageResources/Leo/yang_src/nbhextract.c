/*
 * HTCFlasher - http://htc-flasher.googlecode.com
 *
 * Copyright (C) 2007-2008 Pau Oliva Fora - pof @ <eslack.org>
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
#include "nbh.h"

int32_t DEBUG=0;

static unsigned char bmphead1[18] = {
	0x42, 0x4d, 		/* signature */
	0x36, 0x84, 0x03, 0x00, /* size of BMP file in bytes (unreliable) */
	0x00, 0x00, 0x00, 0x00, /* reserved, must be zero */
	0x36, 0x00, 0x00, 0x00, /* offset to start of image data in bytes */
	0x28, 0x00, 0x00, 0x00, /* size of BITMAP INFO HEADER structure, must be 0x28 */
};

unsigned char bmpheadW[4];
unsigned char bmpheadH[4];
//	0xf0, 0x00, 0x00, 0x00, /* image width in pixels */
//	0x40, 0x01, 0x00, 0x00, /* image height in pixels */

static unsigned char bmphead2[8] = {
	0x01, 0x00,		/* number of planes in the image, must be 1 */
	0x18, 0x00, 		/* number of bits per pixel (1, 4, 8 or 24) */
	0x00, 0x00, 0x00, 0x00, /* compression type (0=none, 1=RLE-8, 2=RLE-4) */
};

//unsigned char bmpheadS[4];
//	0x00, 0x84, 0x03, 0x00, /* size of image data in bytes (including padding) 0x38400 for 320x240*/ 

static unsigned char bmphead3[16] = {
	0x00, 0x00, 0x00, 0x00, /* horizontal resolution in pixels per meter (unreliable) */
	0x00, 0x00, 0x00, 0x00, /* vertical resolution in pixels per meter (unreliable) */
	0x00, 0x00, 0x00, 0x00, /* number of colors in image, or zero */
	0x00, 0x00, 0x00, 0x00  /* number of important colors, or zero */
};



/* getSectionName - returns the name based on ID */
const char *getSectionName(uint32_t section)
{
	if (section == 0x100)
		return "IPL";
	if (section == 0x101)
		return "G3IPL";
	if (section == 0x102)
		return "G4IPL";
	if (section == 0x200)
		return "SPL";
	if (section == 0x201)
		return "G3SPL";
	if (section == 0x202)
		return "G4SPL";
	if (section == 0x300 || section == 0x301)
		return "GSM";
	if (section == 0x400 || section == 0x401)
		return "OS";
	if (section == 0x500)
		return "DIAG";
	if (section == 0x600)
		return "MainSplash";
	if (section == 0x601)
		return "SubSplash";
	if (section == 0x700 || section == 0x900)
		return "ExtROM";
	if (section == 0xB05)
		return "Recovery";
	if (section == 0xB06)
		return "UserData";
	if (section == 0xB04)
		return "LinuxBoot";
	if (section == 0xB02)
		return "LinuxSystem";
	if (section == 0xA00)
		return "MCPLD";
	return "unknown";
}

/* convertNB2BMP - converts a NB splash screen to a bitmap */
int32_t convertNB2BMP(FILE *input, char *filename, int32_t biWidth, int32_t biHeight, uint32_t dataLen, uint32_t start)
{
	FILE *output;
	int32_t y,x;
	char filename2[1024];
	unsigned char colors[3];
	unsigned short encoded;
	uint32_t biSize;
	unsigned char data[dataLen];

	sprintf(filename2, "%s.bmp", filename);
	printf("[] Encoding: %s\n", filename2);

	if (fseek(input, start, SEEK_SET) != 0) {
		fprintf(stderr, "[!!] Could not move to start of image\n");
		return 0;
	}

	if (fread(data, 1, dataLen, input) != dataLen) {
		fprintf(stderr, "[!!] Could not read full image\n");
		return 1;
	}

	output=fopen(filename2,"wb");
	if (output == NULL) {
		fprintf(stderr, "[!!] Could not open '%s'\n", filename);
		return 1;
	}

	if (fwrite(bmphead1, 1, sizeof(bmphead1), output) != sizeof(bmphead1)) {
		fprintf(stderr, "[!!] Could not write bitmap header 1\n");
		fclose(output);
		return 1;
	}

	fwrite(&biWidth, 1, sizeof(biWidth), output);
	fwrite(&biHeight, 1, sizeof(biHeight), output);

	if (fwrite(bmphead2, 1, sizeof(bmphead2), output) != sizeof(bmphead2)) {
		fprintf(stderr, "[!!] Could not write bitmap header 2\n");
		fclose(output);
		return 1;
	}

	biSize = biWidth*biHeight*3;
	fwrite(&biSize, 1, sizeof(biSize), output);

	if (fwrite(bmphead3, 1, sizeof(bmphead3), output) != sizeof(bmphead3)) {
		fprintf(stderr, "[!!] Could not write bitmap header 3\n");
		fclose(output);
		return 1;
	}

	for (y=0; y < biHeight; y++) {
		for (x=0; x < biWidth; x++) {
			encoded = ((unsigned short *)data)[((biHeight-(y+1))*biWidth)+x];
			colors[0] = (encoded << 3) & 0xF8; // 11111000b  take only 5 bytes
			colors[1] = (encoded >> 3) & 0xFC; // 11111100b  take only 6 bytes
			colors[2] = (encoded >> 8) & 0xF8; // 11111000b  take only 5 bytes
			fwrite(colors, 1, 3, output);
		}
	}

	fclose(output);
	return 0;
}

/* isSectionImage - returns true if section type is splash screen */
int32_t isSectionImage(uint32_t type)
{
	if (type == 0x600)
		return 1;
	if (type == 0x601)
		return 1;
	return 0;
}

/* extractNB - extract NB files from NBH */
int32_t extractNB(FILE *input, int32_t index, uint32_t type, uint32_t start, uint32_t len)
{
	int32_t retval=0;
	int32_t biWidth;
	int32_t biHeight;
	uint32_t dataLen;
	char filename[1024];
	FILE *output;

	sprintf(filename, "%02d_%s_0x%03lx.nb", index, getSectionName(type), type);
	printf("[] Extracting: %s\n", filename);

	output = fopen(filename, "wb");
	if (output == NULL)
		fprintf(stderr, "[!!] Could not open '%s'\n", filename);

	if (fseek(input, start, SEEK_SET) != 0)
		fprintf(stderr, "[!!] Could not seek to position %08lX\n", start);

	if (bufferedReadWrite(input, output, len))
		retval = 1;
	else
		fprintf(stderr, "[!!] Could not read complete file\n");

	fclose(output);

	if (isSectionImage(type)) {

		switch (len) {
			case 262144: 
			case 153600: 
				biWidth = 240;
				biHeight = 320;
			break;
			case 655360:
			case 614912:
				biWidth = 480;
				biHeight = 640;
			break;
			default:
				fprintf(stderr, "[!!] Could not determine NB image Width and Height.\n");
				return 0;
			break;
		}

		dataLen = biWidth * biHeight * 2;
		convertNB2BMP(input, filename, biWidth, biHeight, dataLen, start);
	}

	return retval;
}

/* extractNBH - Extract all ROM parts from NBH file */
void extractNBH(char *filename)
{
	int32_t i;
	FILE *input;
	FILE *output;
	FILE *tmpfile;
	char magic[5000];
	char magicHeader2[]={'R','0','0','0','F','F','\n'};
	unsigned char signature[500];
	uint32_t blockIndex = 0;
	uint32_t offset = 0;
	uint32_t blockLen;
	uint32_t signLen;
	uint32_t initsign;
	uint32_t magicHeader[]={'H','T','C','I','M','A','G','E'};
	unsigned char flag;
	unsigned char blockSign[5000];
	struct HTCIMAGEHEADER header;

	/* NBH2DBH */
	input = fopen(filename,"rb");
	if (input == NULL) {
		fprintf(stderr, "[!!] Could not open '%s'\n", filename);
		exit(1);
	}

	if (fread(magic, 1, sizeof(magicHeader2), input) != sizeof(magicHeader2)) {
		fprintf(stderr, "[!!] '%s' is not a valid NBH file1\n",filename);
		fclose(input);
		exit(1);
	}

	if (memcmp(magicHeader2, magic, sizeof(magicHeader2)) != 0) {
		//fprintf(stderr, "[!!] '%s' is not a valid NBH file2\n",filename);
		//fclose(input);
		//exit(1);
		// NEW FORMAT
		initsign = 240;
	} else {
		// OLD format
		initsign = 16;
	}
	if (DEBUG) printf("initsign: %ld\n",initsign);

	if (fread(signature, 1, initsign, input) != initsign) {
		fprintf(stderr, "[!!] Could not read initial signature\n");
		fclose(input);
		exit(1);
	}

	if (DEBUG) {
		printf("Initial signature: ");
		for (i = 0; i < initsign; i++)
			printf("%02X ", signature[i]);
		printf("\n");
	}

	output=fopen("tempfile.dbh","wb");
	if (output == NULL) {
		fprintf(stderr, "[!!] Could not open 'tempfile.dbh'\n");
		fclose(output);
		exit(1);
	}

	blockIndex = 0;
	offset = 0;
	

	while (!feof(input)) {

		if (fread(&blockLen, 4, 1, input) && fread(&signLen, 4, 1, input) && fread(&flag, 1, 1, input)) {

			if (DEBUG) printf("blockLen: %08lX\nsignLen: %08lX\nflag: %i\n", blockLen, signLen, flag);

			if (initsign!=16 && blockIndex==0) {
				printf("[] Android NBH detected, adjusting first block length.\n");
				blockLen=999999999;
			}
			if (!bufferedReadWrite(input, output, blockLen)) {
				if (initsign!=16 && blockIndex!=0) fprintf(stderr, "[!!] Error in block %ld (%08lX - %08lX)\n", blockIndex, offset, offset + blockLen);
				break;
			}

			if (fread(blockSign, 1, signLen, input) != signLen) {
				fprintf(stderr, "Error in block signature %ld\n",blockIndex);
				memset(blockSign, 0, sizeof(blockSign));
				break;
			}

			if (DEBUG) {
				printf("Signature for block %04ld (%08lX - %08lX): ", blockIndex, offset, offset + blockLen);
				for (i = 0; i < signLen; i++)
					printf("%02X", blockSign[i]);
				printf("\n");
			}

		} else {
			fprintf(stderr, "[!!] Error in block %ld\n", blockIndex);
			break;
		}

		blockIndex++;
		offset += blockLen;

		/* end */
		if (flag == 0x02)
			break;
	}
	fclose(output);
	fclose(input);

	/* DBHExtract */
	tmpfile = fopen("tempfile.dbh", "rb");
	if (tmpfile == NULL) {
		fprintf(stderr, "[!!] Could not open 'tempfile.dbh'\n");
		fclose(tmpfile);
		exit(1);
	}

	if (fread(magic,1,sizeof(magicHeader),tmpfile)!=sizeof(magicHeader)) {
		fprintf(stderr, "[!!] tempfile.dbh is not a valid .dbh file\n");
		fclose(tmpfile);
		exit(1);
	}

	if (memcmp(magicHeader,magic,sizeof(magicHeader))!=0) {
		fprintf(stderr, "[!!] tempfile.dbh is not a valid .dbh file\n");
		fclose(tmpfile);
		exit(1);
	}

	if (fread(&header,1,sizeof(HTCIMAGEHEADER),tmpfile)!=sizeof(HTCIMAGEHEADER)) {
		fprintf(stderr,"[!!] Could not read header\n");
		fclose(tmpfile);
		exit(1);
	}

	printf("\n");
	printf("Device:   %s\n", header.device);
	printf("CID:      %s\n", header.CID);
	printf("Version:  %s\n", header.version);
	printf("Language: %s\n", header.language);
	printf("\n");

	for (i = 0; i < (sizeof(header.sectiontypes) / sizeof(header.sectiontypes[0])); i++) {
		if (i+50 < 95)
		if (header.sectiontypes[i]!=0 && i!=31) {
			if (!extractNB(tmpfile, i, header.sectiontypes[i], header.sectionoffsets[i], header.sectionlengths[i]))
				fprintf(stderr,"[!!] Error while extracting file %d\n", i);
		}
	}

	printf ("[] Done!\n");

	fclose(tmpfile);
	if (!DEBUG) unlink("tempfile.dbh");
}