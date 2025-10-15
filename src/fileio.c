#include <dos.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <mem.h>
#include "inc/types.h"
#include "inc/timer.h"
#include "inc/logger.h"
#include "inc/memory.h"
#include "inc/fileio.h"

/* vidmem:
 * VGA_Buffer  : used as image data buffer while loading
 * Draw_Target : loaded image will be copied to
 */

// ============================================================================
// TURBO CACHE MASTER 2000(c) by Redonkulous Corp. (declaration)
// ============================================================================

// files will be cached at first load
static addr* _graphicsCache = NULL;
static addr* _samplesCache = NULL;
static addr* _miscCache = NULL;
static bool _cacheInitialized = false;

static Asset _assetCache[50];

static IOStatus disk_to_cache(char* filename, Asset_Type type);
static Asset fetch_asset(char* filename);

// ============================================================================
// Load
// ============================================================================

IOStatus Load_Palette_PCX(char* filename) {
	Asset *asset; FILE *file; int i;
	byte palette[768];
	char filepath[256] = "assets\\";
	strcat(filepath, filename);

	file = fopen(filepath, "rb");
	if (!file) return No_File;

	fseek(file, -768, SEEK_END);
	fread(palette, 1, 768, file);

	outp(0x3c8, 0);
	for (i = 0; i < 768; i++) {
		outp(0x3c9, palette[i] >> 2);
	}

	fclose(file);
	return Awesome;
}

HighScores Load_High_Scores(void) {
   FILE *file; HighScores table;

   table.loaded = false;
	file = fopen("hscores.snk", "rb");
	if (!file) return table;

	fread((char*)&table.entry[0], sizeof(ScoreEntry), 8, file);
   
   table.loaded = true;
	fclose(file);
	return table;
}

IOStatus Save_High_Scores(HighScores table) {
	FILE *file;

	// Save to file
	file = fopen("hscores.snk", "wb");
	if (!file) return No_File;

	fwrite((char*)&table.entry[0], sizeof(ScoreEntry), 8, file);

	table.saved = true;
	fclose(file);
	return Awesome;
}

// read long play samples from disk, skipping cache
uint Stream_Read_Chunk(char* filename, byte* chunk, ulong chunk_size) {
	static FILE* current_file = NULL;
	static char current_filename[256] = {0};
   static uint current_file_size = 0;
	static long file_position = 0;
	ulong bytes_read = 0;
	char filepath[256] = "assets\\";

	// check, if new file (you just never know)
	if (current_file == NULL || strcmp(current_filename, filename) != 0) {
		if (current_file) {
			fclose(current_file);
			current_file = NULL;
		}

		strcat(filepath, filename);
		current_file = fopen(filepath, "rb");
		if (!current_file) return 0;

      fseek(current_file, 0, SEEK_END);
      current_file_size = ftell(current_file);
      rewind(current_file);

		strcpy(current_filename, filename);
		file_position = 0;

		// skip header
		if (strstr(filename, ".wav") || strstr(filename, ".WAV")) {
			fseek(current_file, 44, SEEK_SET);
			file_position = 44; current_file_size -= 44;
		}
	}

	bytes_read = fread(chunk, 1, chunk_size, current_file);
   // Log_Info("Bytes read: %d", bytes_read);

	if (bytes_read == 0) {
		fclose(current_file);
		current_filename[0] = '\0';
		current_file = NULL;
      current_file_size = 0;
      file_position = 0;
		return 0;
	}

	// add silence, if necessary
	if (bytes_read < chunk_size) {
		memset(chunk + bytes_read, 0x80, chunk_size - bytes_read);
	}

	file_position += bytes_read;
	return current_file_size;
}

// ============================================================================
// TURBO CACHE MASTER 2000(c) by Redonkulous Corp.  (implementation)
// ============================================================================

/*
 * caches raw file data in pools of Asset_Type
 * adds 4 zero bytes between data blocks as terminator internally
 * returns meta-data and pointer to cached file data, if successful
 * types: GRAPHICS, SAMPLES, MISC
*/
Shizzlabang Asset_Manager(char* filename, Asset_Type type) {
	Shizzlabang shizzle;
	shizzle.status = No_Cache;

	if (_cacheInitialized) {
		shizzle.asset = fetch_asset(filename);
		if (shizzle.asset.size == 0) {
			shizzle.status = disk_to_cache(filename, type);			
			if (shizzle.status == Awesome) {
				shizzle.asset = fetch_asset(filename);
				return shizzle;
			}
		} else {
			shizzle.status = Awesome;
			return shizzle;
		}
	}

	return shizzle;
}

bool Init_Assets() {
	if (_graphicsCache == NULL) _graphicsCache = (byte*)malloc(0x80000); // 512KB
	if (_samplesCache == NULL) _samplesCache = (byte*)malloc(0x80000);	// 512KB
	if (_miscCache == NULL) _miscCache = (byte*)malloc(0x20000);		   // 128KB

	if (_graphicsCache == NULL || _samplesCache == NULL || _miscCache == NULL)
		return false;

   _cacheInitialized = true;
	return true;
}

void Dispose_Assets() {
	if (_graphicsCache) { free(_graphicsCache); _graphicsCache = NULL; }
	if (_samplesCache) { free(_samplesCache); _samplesCache = NULL; }
	if (_miscCache) { free(_miscCache); _miscCache = NULL; }
   _cacheInitialized = false;
}

static Asset fetch_asset(char* filename) {
	Asset asset;
	int slot = -1;
	int i = 0;
	asset.size = 0;

	for (i = 0; i < 50; i++) {
		if (strcmp(_assetCache[i].filename, filename) == 0) {
			slot = i; break;
		}
	}

	if (slot != -1) return _assetCache[slot];
	return asset;
}

static IOStatus disk_to_cache(char* filename, Asset_Type type) {
	FILE *file; 
   addr* target_cache;
	int slot = -1, i = 0;
   const byte appendix = 4;      // keep 4 zero bytes space between cached data
	uint max_size = 0;            // size of specific cache poool
	uint file_size = 0;
	uint used_space = 0;
   uint checksum = 0;

	char filepath[256] = "assets\\";
	strcat(filepath, filename);

	// find free slot
	for (i = 0; i < 50; i++) {
		if (_assetCache[i].filename[0] == '\0') { // empty slot
			slot = i; break;
		}
	}
	if (slot == -1) return File_Max;

	file = fopen(filepath, "rb");
	if (!file) return No_File;

	fseek(file, 0, SEEK_END);
	file_size = ftell(file);
	rewind(file);

	switch (type) {
	case GRAPHICS:
		target_cache = _graphicsCache;
		max_size = 0x80000; // 512kb
		break;
	case SAMPLES:
		target_cache = _samplesCache;
		max_size = 0x80000; // 512kb
		break;
	case MISC:
		target_cache = _miscCache;
		max_size = 0x20000; // 128kb
		break;
	default:
		fclose(file);
		return Horse_Shit;
	}

	// space left?
	for (i = 0; i < 50; i++) {
		if (_assetCache[i].type == type && _assetCache[i].filename[0] != '\0') {         
			used_space += _assetCache[i].size + appendix;
		}
	}
	if ((used_space + file_size + appendix) > max_size) {
		fclose(file); return Cache_Full;
	}

	// save asset to cache
	fread(target_cache + used_space, file_size, 1, file);
	fclose(file);

   // append 4 zero bytes at the end of cached block
   memset(target_cache + used_space + (file_size - 1), 0, appendix);
   strcpy(_assetCache[slot].filename, filename);
	_assetCache[slot].location = (byte *)target_cache + used_space;
	_assetCache[slot].size = file_size;
	_assetCache[slot].type = type;

   for (i = 0; i < file_size; i++) checksum ^= target_cache[used_space + i];
   _assetCache[slot].checksum = checksum;

   // Log_Info("Loaded: %s", filepath);
	return Awesome;
}

// ========================================================
// Debug
// ========================================================

bool Validate_Cache(char* filename, Asset_Type type) {
   Shizzlabang shizzle; uint checksum = 0, i;

   shizzle = Asset_Manager(filename, type);

   for (i = 0; i < shizzle.asset.size; i++) {
      checksum ^= shizzle.asset.location[i];
   }

   if (checksum != shizzle.asset.checksum) {
      Log_Info("FileIO: file: %s, expected: %lX, got: %lX", 
               shizzle.asset.filename, shizzle.asset.checksum, checksum);
      return false;
   }

   return true;
}

typedef struct {
	char manufacturer;
	char version;
	char encoding;
	char bitsPerPixel;
	word xMin, yMin, xMax, yMax;
	word hRes, vRes;
	char palette[48];
	char reserved;
	char numPlanes;
	word bytesPerLine;
	word paletteType;
	char filler[58];
} PCXTestHeader;

int Validate_PCX(char* filename) {
	PCXTestHeader header;
	word width;
	word height;
	byte palette[768];
	byte palette_sig;

	FILE *file = fopen(filename, "rb");
	if (!file) {
		printf("ERROR: Cannot open PCX file: %s\n", filename);
		return 0;
	}
	// Read header
	if (fread(&header, sizeof(PCXTestHeader), 1, file) != 1) {
		printf("ERROR: Cannot read PCX header from %s\n", filename);
	}
	// Check PCX signature
	if (header.manufacturer != 10) {
		printf("ERROR: %s is not a valid PCX file (manufacturer = %d)\n",
			   filename, header.manufacturer);
	}
	// Check version (should be 5 for 256-color PCX)
	if (header.version != 5) {
		printf("ERROR: %s wrong PCX version (got %d, need 5)\n", filename,
			   header.version);
	}
	// Check encoding (should be 1 for RLE)
	if (header.encoding != 1) {
		printf("ERROR: %s wrong encoding (got %d, need 1)\n", filename,
			   header.encoding);
		fclose(file);
		return 0;
	}
	// Check bits per pixel (should be 8 for 256-color)
	if (header.bitsPerPixel != 8) {
		printf("ERROR: %s wrong bits per pixel (got %d, need 8)\n", filename,
			   header.bitsPerPixel);
	}
	// Calculate dimensions
	width = header.xMax - header.xMin + 1;
	height = header.yMax - header.yMin + 1;
	// Check exact dimensions for Mode 13h
	if (width != 320) {
		printf("ERROR: %s wrong width (got %u, need 320)\n", filename, width);
	}
	if (height != 200) {
		printf("ERROR: %s wrong height (got %u, need 200)\n", filename, height);
	}
	// Check number of planes (should be 1 for 256-color)
	if (header.numPlanes != 1) {
		printf("ERROR: %s wrong number of planes (got %d, need 1)\n", filename,
			   header.numPlanes);
	}
	// Check bytes per line (should be 320 for our case)
	if (header.bytesPerLine != 320) {
		printf("ERROR: %s wrong bytes per line (got %u, need 320)\n", filename,
			   header.bytesPerLine);
	}
	// Check if palette exists (seek to palette position)
	if (fseek(file, -769, SEEK_END) != 0) {
		printf("ERROR: %s file too small for palette\n", filename);
	}
	// Check palette signature (should be 0x0C)
	if (fread(&palette_sig, 1, 1, file) != 1) {
		printf("ERROR: %s cannot read palette signature\n", filename);
	}
	if (palette_sig != 0x0C) {
		printf("ERROR: %s invalid palette signature (got 0x%02X, need 0x0C)\n",
			   filename, palette_sig);
	}
	// Read and validate palette (768 bytes = 256 colors * 3 components)
	if (fread(palette, 1, 768, file) != 768) {
		printf("ERROR: %s cannot read complete palette\n", filename);
	}
	// Check if palette values are in valid range (0-63 for VGA)
	/*
	for (int i = 0; i < 768; i++) {
	  if (palette[i] > 63) {
			printf("ERROR: %s palette value out of range (color %d,
	component %d = %d)\n", filename, i/3, i%3, palette[i]);
	  }
	}
	*/
	fclose(file);

	return 1;
}
