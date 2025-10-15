#ifndef FILEIO_H
#define FILEIO_H
#include "inc/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	Awesome = 0,
	No_File = 1,
	File_Max = 2,
	No_Cache = 3,
	Cache_Full = 4,
	Cache_Fail = 5,
	Image_To_Big = 6,
	Horse_Shit = 97,
	Cluster_Fuck = 98,
	OH_MY_GOSH_WHAT_WERE_YOU_THINKING = 99
} IOStatus;

typedef enum { 
   GRAPHICS = 0,
	SAMPLES = 1,
	MISC = 2 
} Asset_Type;

typedef struct {
	char filename[32];	// id
	addr* location;		// pointer address
	unsigned long size;  // actual size
	Asset_Type type;
   uint checksum;
} Asset;

typedef struct {
	IOStatus status;
	Asset asset;
} Shizzlabang;

typedef struct {
	char score[4];
   char handle[4];
} ScoreEntry;

typedef struct {
   ScoreEntry entry[8];
   bool loaded;
	bool saved;
} HighScores;

/*
 * caches raw file data in pools of Asset_Type
 * adds 4 zero bytes between data blocks as terminator internally
 * returns meta-data and pointer to cached file data, if successful
 * types: GRAPHICS, SAMPLES, MISC
*/
Shizzlabang Asset_Manager(char* filename, Asset_Type type);
bool Init_Assets();
void Dispose_Assets();

/*
 * loads large sample files in chunks of given size
 * returns loaded chunk and file_size
*/
uint Stream_Read_Chunk(char* filename, byte* buffer, ulong chunk_size);
IOStatus Load_Palette_PCX(char* filename);

HighScores Load_High_Scores(void);
IOStatus Save_High_Scores(HighScores sores);

/* debug */
bool Validate_Cache(char* filename, Asset_Type type);
int Validate_PCX(char* filename);

#ifdef __cplusplus
}
#endif

#endif
