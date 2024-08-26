#ifndef _INCLUDE_NBH_H
#define _INCLUDE_NBH_H_ 

struct HTCIMAGEHEADER {
	char device[32];
	uint32_t sectiontypes[32];
	uint32_t sectionoffsets[32];
	uint32_t sectionlengths[32];
	char CID[32];
	char version[16];
	char language[16];
};

extern struct HTCIMAGEHEADER HTCIMAGEHEADER;

/* nbh.c */
int32_t bufferedReadWrite(FILE *input, FILE *output, uint32_t length);

/* nbhextract.c */
void extractNBH(char *filename);

#endif