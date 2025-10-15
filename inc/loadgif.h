#ifndef LOADGIF_H
#define LOADGIF_H
#include "inc/types.h"
#include "inc/fileio.h"

#ifdef __cplusplus
extern "C" {
#endif

#define POOL_SIZE 0x20000 // 128kb

typedef struct {
    byte signature[3];    // "GIF"
    byte version[3];      // "87a" oder "89a"
} GIFHeader;

#pragma pack(1)
typedef struct {
    word width;
    word height;
    byte packed;          // Global Color Table Info
    byte bg_color_index;
    byte pixel_aspect;
} GIFLogicalScreen;

typedef struct {
    byte separator;       // 0x2C
    word left;
    word top;
    word width;
    word height;
    byte packed;
} GIFImageDescriptor;
#pragma pack()

// LZW Code-Tabelle Eintrag
typedef struct {
    word code;
    word length;
    addr* pixels;
} LZWEntry;

typedef struct {
   addr* graphics;
   addr* palette;
   addr* bit_mask;
   addr* level_data;
} SNKMap;


IOStatus Load_Level(char* filename, SNKMap* lvldata);

#ifdef __cplusplus
}
#endif

#endif
