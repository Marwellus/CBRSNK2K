#ifndef VIDEO_HPP
#define VIDEO_HPP
#include "inc/types.h"

#define SCREEN_SIZE 64000
#define MDDOS 1 // DOS TEXT MODE
#define MD13H 2 // VGA MODE 13H
#define MODEX 3 // VGA MODE X

struct RGB {
	byte red;
	byte green;
	byte blue;
};

struct Pxl {
	int x,y,offx,offy;
	byte color;
};

class Video {
	public:
		static int Mode;
		static RGB VGAPalette[256];

		static bool Init();
		static void Dispose();
		static void SetScreen(int mode);
		static void SetRefreshRate(int hz);
		static byte Read(Pxl pxl);
		static void Write(Pxl pxl);
      static void Write(Pxl pxl, addr* buffer);
		static void FlashColor(byte color);
		static void SetColorRGB(int index, byte r, byte g, byte b);
		static RGB GetColorRGB(byte color);
      static void FadeOut(int duration_ms);
      static void FadeIn(int duration_ms);
      static void LoadVGAPalette(char* filename);
		static void StoreVGAPalette(byte* pcx_pal, bool set);
      static void StoreCurrentPalette();
		static void RestorePalette();		
		static void ColorSpread(RGB fromColor, RGB toColor, int fromIdx, int toIdx);
		static void ClearScreen();
		static void PrintC(int x, int y, char *text, byte color);
		static void SplashScreen();
};

#endif
