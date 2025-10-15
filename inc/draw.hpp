#ifndef DRAW_HPP
#define DRAW_HPP
#include "inc/types.h"
#include "inc/fileio.h"
#include "inc/video.hpp"
#include "inc/font.hpp"

#define PXL_SIZE  2
#define GFX_MASK_0  0x0F // render mask
#define GFX_MASK_1  0x0E // render mask cater
#define BUFFER    1
#define VGAMEM    0

#define MAX_16K		16000
#define READ_STOP		0xFE
#define MAX_ASSETS 	16
#define MAX_TILES		8
#define MAX_CYCLES	8

struct CbParams {
	addr* target;
   HighScores hs; 
   int hspos; 
};

struct SNKStats {
	int round;
	int lifes;
	int score;
};

struct SmallTile {
	word width;
	word height;
	byte data[64];
};

struct GameAsset {
	char name[16];
	SmallTile tiles[8];
};

struct ColorCycle {
	RGB original;		// save original color
	byte color;			// color to cycle
	int count;			// external control
	int step, dir;		// internal control
	byte red, green, blue; // rgb values
};

struct PCXHeader {
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
};

enum TransitionType {
   FadeOut     = 1,
   Interlaced  = 2,
   Drop        = 3,
   Slide       = 4,
	Rotate		= 5
};

class Draw {
	private:
		static Font _font;
		static addr* _pcxBuffer;
		static GameAsset* _asset;
		static int _lineOffset[200];      
      static bool _renderMode; 
      static bool _sfxActive;
      static byte* _switchOn;
      static byte* _switchOff;
		static ColorCycle _exitLight;
		static ColorCycle _cycles[MAX_CYCLES];
		static void updateOffsets();
      static void verticalTitle(CbParams* empty);
      static void horizontalTitle(CbParams* empty);
      static void hallOfFame(CbParams* params);
		static void sinusEffect();
	public:
		static uint TicksPassed;
		static int PxlSize;
		static GameAsset GameAssets[16];
		static void Init();
		static void Dispose();
		/* out-game gfx */
		static void GameIntro();
		static void HowtoControl(TransitionType type);
      static void HallOfFame(HighScores hs, int hspos, TransitionType type);
      /* pre-/after game gfx */
      static void InitRound(char* name, TransitionType type);
      static void HowtoPlay();
      static void SwitchRenderMode();
      /* in-game gfx */
      static void ShowStats(SNKStats stats, bool extra_life);
		static void NitroGauge(int max_nitro, int used);
      static void ToggleEffect();
      static void DoorIndicator(bool reset);
		static ColorCycle* GetOrAddCycle(byte color);
		static void ColorPulse(ColorCycle* cyc, bool reset);
		static void UpdateScreen(SNKStats stats, bool extra_life, bool update_buffer);
      /* transition effects / exit game scroller */
		static void Backflip(addr* front, addr* back, bool partially);
      static void LevelTransition(TransitionType type);
      static void ImageTransition(char *filename, TransitionType type);
		static void ImageTransition(char* filename, TransitionType type,
                                  void(*callback)(CbParams*), CbParams* params);
      static void ScrollCredits();
      /* Field methods */
      static void ClearFieldBuffer();
		static void ToggleSwitch(int offset, bool state);
      /* supportive methods */
      static void SaveCurrentScreen();
      static void RestoreCurrentScreen();
      static void ClearScreen(bool set_pal);
		static IOStatus PCXImage(char* filename, addr* target);
		static void LoadGameAssets(char* filename);
		static GameAsset* GetGameAsset(char* name);
		static void GameAssetTo(char* name, int tile, int x, int y, addr* buffer);
		static void MessageBox(char* text);
		static void BorderedBox(int sx, int sy, int ex, int ey);
		static void Print(char* text, int x, int y, byte color);
      static void PrintV(char* text, int x, int y, byte color);
		static void Letter(int x, int y, char c, byte color);
		static void Pixel(int x, int y, byte color);
		static void Pixel(int x, int y, byte color, addr* buffer);
		static void MultiPxl(int x, int y, byte* colors);
		static void MultiPxl(int x, int y, byte* colors, addr* buffer);
		static void GetMultiPxl(int x, int y, addr* buffer);
		static void Fill(int sx, int ex, int sy, int ey, byte color);
		static void Rect(int x, int y, int w, int h, byte color);
		static void Line(int sx, int sy, int ex, int ey, byte color);

      /* debug */
		static void ShowPalette();
		static int CheckPCX(char *filename);
};

#endif