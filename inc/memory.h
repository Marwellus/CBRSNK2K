#ifndef MEMORY_H
#define MEMORY_H
#include "inc/types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_BUFFER_SIZE 384000
#define STD_BUFFER_SIZE 64000 // 320 x 200

extern addr* Max_Buffer;	// 448kb, containing all other buffers ...
extern addr* VGA_Buffer;	// back buffer
extern addr* SFX_Buffer;	// sfx buffer
extern addr* FLD_Buffer;	// field render buffer
extern addr* VGA_Image;		// original level image
/* non-gfx */
extern addr* Level_Map;		// collision map
extern addr* Bit_Mask;		// 64kb bit mask
/* small seperate buffer */
extern addr* VGA_Palette; // 768b VGA palette

extern addr* VGA_Address; // actual vga ram address
extern addr* Draw_Target; // current draw target for game

typedef struct {
	addr* data; 	// 32bit flat pointer
	addr* real;  	// physical address
	uint size;
	uint selector;
	uint segment;
	uint page;
	uint offset;
} Real_Segment;

bool Mem_Init(); // dbl check shit
void Mem_Dispose(void); // clean shit up
/* DMA shit needs the real shit */
bool Real_Malloc(Real_Segment* segment);
void Real_Free(int selector);
/* some ASM shit */
void MemCopy32(addr* dest, addr* src, int size);
void TransCopy(addr* dest, addr* src, int size);
void MemSet32(addr* dest, dword value, uint count);
void DrawTile(int x, int y, byte* tile, int width, int height, addr* target);
void DrawLetter(int x, int y, byte color, byte* letter, addr* target);
void ColorLetter(int x, int y, byte color, byte* letter, addr* target);

#ifdef __cplusplus
}
#endif

#endif
