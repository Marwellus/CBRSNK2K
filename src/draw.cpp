#include <dos.h>
#include <bios.h>
#include <math.h>
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include "inc/types.h"
#include "inc/logger.h"
#include "inc/timer.h"
#include "inc/memory.h"
#include "inc/scroller.h"
#include "inc/loadgif.h"
#include "inc/video.hpp"
#include "inc/font.hpp"
#include "inc/draw.hpp"

Font Draw::_font("ESCHATA.F08");

bool Draw::_renderMode = VGAMEM;
bool Draw::_sfxActive  = false;
addr* Draw::_pcxBuffer = NULL;
/* used in sinusEffect */
int Draw::_lineOffset[200] = {0};
/* in-game assets */
GameAsset Draw::GameAssets[MAX_ASSETS] = {0};
/* assets & switches gfx (tiles) */
GameAsset* Draw::_asset	= NULL;
byte* Draw::_switchOn	= NULL;
byte* Draw::_switchOff	= NULL;
/* color cycles */
ColorCycle Draw::_exitLight = { 0 };
ColorCycle Draw::_cycles[MAX_CYCLES] = { 0 };
/* measurement */
uint Draw::TicksPassed = 0;

// ============================================================================
// out-game
// ============================================================================

void Draw::Init() {
	_pcxBuffer = Bit_Mask;
	// load in-game assets
	LoadGameAssets("snkasts.pcx");
	// prepare own assets
	_asset = GetGameAsset("draw");
	_switchOn  = _asset->tiles[0].data;
	_switchOff = _asset->tiles[1].data;

	for (int n = 0; n < MAX_CYCLES;  n++)
		_cycles[n].color = NULL;

	byte color = 62; // red door indicator
	RGB rgb = Video::GetColorRGB(color);
   _exitLight.color 		= color;
   _exitLight.original 	= rgb;
	_exitLight.count		= 0;
   _exitLight.step 		= 0;
	_exitLight.dir 		= 1;
   _exitLight.red   		= 50;
   _exitLight.green 		= 12;
   _exitLight.blue  		= 12;
}

void Draw::Dispose() {
	Log_Info("Draw:: ticks passed: %d", TicksPassed);
}

void Draw::verticalTitle(CbParams* p) {
	Draw_Target = p->target;
   PrintV("CyberSnake 2000 Turbo", 305, 8, 124);
	PrintV("CyberSnake 2000 Turbo", 304, 7, 38);
	Draw_Target = VGA_Address;
}

void Draw::GameIntro() {
   ClearScreen(true);

	Print("        In the year 2000", 10, 81, 64);
	delay(1000);
	Print("     cybernetic snakes have", 10, 91, 64);
	delay(1000);
	Print("  taken over the entire world!", 10, 101, 64);
	delay(2000);
	Print("Only ONE snake can save humanity!", 10, 111, 48);
	delay(2500);

	CbParams p; p.target = VGA_Address;
	ImageTransition("cbrsnktt.pcx", FadeOut, verticalTitle, &p);
	delay(5000);
}

void Draw::hallOfFame(CbParams* p) {
	Draw_Target = p->target;
	Print("<== Hall Of Shame ==>", 7, 104, 126);
	Print("<== Hall Of Shame ==>", 6, 103, 28);

	char hs_tbl_row[32];
	for (int row = 0; row < 8; row++) {
		sprintf(
			hs_tbl_row,
			"%s . . . . . . . %s",
			p->hs.entry[row].score, // "023"
			p->hs.entry[row].handle // "KNG"
		);
		Print(hs_tbl_row, 6, 115 + (row * 10), row == p->hspos ? 191 : 113);
	}

	verticalTitle(p);
}

void Draw::HallOfFame(HighScores hs, int hspos, TransitionType type) {
   CbParams p; p.hs = hs; p.hspos = hspos; 
	p.target = type == FadeOut ? VGA_Address : VGA_Image;
	Video::RestorePalette();
	ImageTransition("hiscore.pcx", type, hallOfFame, &p);
}

void Draw::HowtoControl(TransitionType type) {
	CbParams p; p.target = type == FadeOut ? VGA_Address : VGA_Image;
	ImageTransition("controls.pcx", type, verticalTitle, &p);
}

// ============================================================================
// pre-game gfx
// ============================================================================

void Draw::horizontalTitle(CbParams* p) {
	Draw_Target = p->target;
   Print("CbrSnk 2000 Turbo", 165, 191, 124);
	Print("CbrSnk 2000 Turbo", 164, 191, 28);
	Draw_Target = VGA_Address;
}

void Draw::InitRound(char* filename, TransitionType type) {
	memset(Max_Buffer, 0, MAX_BUFFER_SIZE);

   // load and setup map
   SNKMap map = { VGA_Image, VGA_Palette, Bit_Mask, Level_Map };
   IOStatus s = Load_Level(filename, &map);
	Video::StoreVGAPalette(map.palette, true);

	CbParams p; p.target = VGA_Image;
	horizontalTitle(&p); LevelTransition(type);
}

void Draw::HowtoPlay() {
	CbParams p; p.target = VGA_Image;
   ImageTransition("explain.pcx", Drop, horizontalTitle, &p);
	SNKStats stats = { 1, 3, 0 };
	ShowStats(stats, false);
}

void Draw::SwitchRenderMode() {
   _renderMode = _renderMode ? 0 : 1;
   Draw_Target = _renderMode == 1 ? VGA_Buffer : VGA_Address;
}

// ============================================================================
// in-game gfx
// ============================================================================

void Draw::ShowStats(SNKStats stats, bool extra_life) {
   char text[16];
	static word pulse = 0;
   static word counter = 0;
   static bool toggle = true;

   sprintf(text, "ROUND: %d", stats.round);
	Print(text, 4, 3, 50);
	Print(text, 5, 3, 48);

	text[0] = 0;
	for (int i = 0; i < stats.lifes; i++) sprintf(text+i, "\2");
	Print("\2\2\2\2\2\2", 146, 3, 2);
	
	Print(":", 209, 3, 2); Print(":", 215, 3, 2);
	if (pulse >= 16) { pulse = 0; Print(".", 208, 3, 10); }
	else if (pulse >= 12) Print(".", 214, 3, 10);
	else if (pulse >= 8)  Print("/", 214, 3, 10);
	else if (pulse >= 4)  Print("/", 208, 3, 10);
	else if (pulse >= 0)  Print(".", 208, 3, 10);
	pulse++;

   if (extra_life)  {
		if (counter < 2) Print(text, 145, 3, 10);
      counter = (counter >= 4) ? 0 : counter + 1;
   } else {
      Print(text, 145, 3, 10);   
      counter = 0;
   }

	sprintf(text, "SCORE:%03d", stats.score);
	Print(text, 233, 3, 2);
	sprintf(text, stats.score > 99 ? "SCORE:%d" : 
					  stats.score > 9 ? "SCORE: %d" : "SCORE:  %d",
					  stats.score);
	Print(text, 232, 3, 10);
}

void Draw::NitroGauge(int max_nitro, int used) {
	int max_x = 157; // (max_x / max_nitro) ~ 8
	int width = int((max_x / max_nitro) * used);
	int move_x = max_x - width;

	Draw_Target = FLD_Buffer;
   Fill(move_x, 191, width, 7, 116);
	Draw_Target = VGA_Buffer;
}

void Draw::ToggleEffect() {
   _sfxActive = _sfxActive ? false : true;
   MemCopy32(SFX_Buffer, VGA_Buffer, SCREEN_SIZE);
}

void Draw::updateOffsets() {
	static int phase = 0; phase++;
	for (int y = 14; y < 186; y++) {
		_lineOffset[y] = (int)(5 * sin((phase + y) * 0.1));
	}
}

void Draw::sinusEffect() {
	int playfield = 0; 
	updateOffsets();

	// keep ui (top/bottom) updated
   MemCopy32(SFX_Buffer, VGA_Buffer, (14*320));
	MemCopy32(SFX_Buffer+(186*320), VGA_Buffer+(186*320), (14*320));

	// render distorted img into sfx_buffer
	for (int y = 14; y < 186; y++) {
      playfield = (y * 320) + 10;
		MemCopy32(SFX_Buffer + playfield + _lineOffset[y], VGA_Buffer+playfield, 300);
	}
}

void Draw::UpdateScreen(SNKStats stats, bool extra_life, bool update_buffer) {
	// update only on change
	if (update_buffer) {
		MemCopy32(VGA_Buffer, VGA_Image, SCREEN_SIZE);	// static image
		TransCopy(VGA_Buffer, FLD_Buffer, SCREEN_SIZE); // level items, snake etc
		Draw::ShowStats(stats, extra_life);
		MemSet32(FLD_Buffer, 0, SCREEN_SIZE);
	}

	if (_sfxActive) sinusEffect(); // renders SFX_Buffer image

	MemCopy32(VGA_Address, _sfxActive ? SFX_Buffer : VGA_Buffer, SCREEN_SIZE);
	if (update_buffer) TicksPassed = t_measure_tick;
}

void Draw::DoorIndicator(bool reset) {
	_exitLight.count = 0; // not requried
	ColorPulse(&_exitLight, reset);
}

void Draw::ColorPulse(ColorCycle* cyc, bool reset) {
	int steps = 16;

   cyc->step  += cyc->dir;
   cyc->red   -= (cyc->red > 0   && cyc->red < 63)   ? cyc->dir : 0;
   cyc->green -= (cyc->green > 0 && cyc->green < 63) ? cyc->dir : 0;
   cyc->blue  -= (cyc->blue > 0  && cyc->blue < 63)  ? cyc->dir : 0;   
   Video::SetColorRGB(cyc->color, cyc->red, cyc->green, cyc->blue);

	cyc->dir = (cyc->step >= steps) ? -1 : (cyc->step <= 0) ? 1 : cyc->dir;

   if (reset) {
		Video::SetColorRGB(
			cyc->color,
			cyc->original.red, 
			cyc->original.green, 
			cyc->original.blue
		);
	}
}

ColorCycle* Draw::GetOrAddCycle(byte color) {
	static int idx = 0; int n = -1;

	for (int i = 0; i < MAX_CYCLES; i++)
		if (_cycles[i].color == color) { n = i; break; }
	
	if  (n != -1) return &_cycles[n];

	_cycles[idx].original = Video::GetColorRGB(color);
	_cycles[idx].color 	 = color;
	_cycles[idx].count 	 = 0;	// external control
	_cycles[idx].step 	 = 0;
	_cycles[idx].dir 		 = 1;
	_cycles[idx].red		 = 50;
	_cycles[idx].green 	 = 12;
	_cycles[idx].blue		 = 12;

	idx += idx < MAX_CYCLES ? 1 : -(MAX_CYCLES-1);
	return &_cycles[idx];
}

// ============================================================================
// Transition effect methods / exit game scroller
// ============================================================================

/* transitions keep UI untouched */
void Draw::LevelTransition(TransitionType type) {
   bool run = true, last_run = false;
   // effects skip top & bottom of the screen (UI parts)
   int x = 0, size_x = 1, y = 14, show_part, offset, i; 
   float acc = 0;

	switch (type) {
   case Drop:
      offset = SCREEN_SIZE-(14*320);
      while (run) {
         run = !last_run; show_part = (offset - (y * 320));
         while (inp(0x3DA) & 0x08); while (!(inp(0x3DA) & 0x08));
         MemCopy32(VGA_Address+(14*320), (VGA_Image+show_part), y * 320);
         y += (1 + acc); acc += 0.18;
         if (y >= 172) { y = 172; last_run = true; };
      }
      break;
   case Slide:
      MemCopy32(SFX_Buffer, VGA_Address, SCREEN_SIZE);
      while (run) {
         run = !last_run;
         for (y = 14; y <= 186; y++) {
            offset = (y*320)+(319-size_x);
            MemCopy32(SFX_Buffer+offset, VGA_Image+(y*320), size_x);
         }
         while (inp(0x3DA) & 0x08); while (!(inp(0x3DA) & 0x08));
         MemCopy32(VGA_Address, SFX_Buffer, SCREEN_SIZE);

         size_x += (1 + acc); acc += 0.18;
         if (size_x >= 319) { size_x = 319; last_run = true; };
      }
      break;
   case Interlaced:
      for (y = 12; y < 186; y += 2) {
         MemCopy32(VGA_Address+(y*320), VGA_Image+(y*320), 320); 
			delay(5);
      }
      for (y = 187; y > 13; y -= 2) {
         MemCopy32(VGA_Address+(y*320), VGA_Image+(y*320), 320); 
			delay(5);
      }      
      break;
	case Rotate:
		MemCopy32(VGA_Buffer, VGA_Address, SCREEN_SIZE);
		Backflip(VGA_Buffer, VGA_Image, false);
		break;
   default: // FadeOut
      Video::FadeOut(500);
   	Video::ClearScreen();
	   MemCopy32(VGA_Address, VGA_Image, SCREEN_SIZE);
      Video::FadeIn(500);
      break;
   }
}

void Draw::ImageTransition(char* filename, TransitionType type) {
   ImageTransition(filename, type, NULL, NULL);
}

/* full image transitions */
void Draw::ImageTransition(
   char* filename, TransitionType type,
   void(*callback)(CbParams*), CbParams* params
) {
   bool run = true, last_run = false;
   int y = 1, offset = SCREEN_SIZE, i;
	float acc = 0;

   switch (type) {
   case Drop:
      PCXImage(filename, VGA_Image);
      if (callback) callback(params);
      while (run) {
         run = !last_run;
         while (inp(0x3DA) & 0x08); while (!(inp(0x3DA) & 0x08));
         offset = (SCREEN_SIZE - (y * 320));
         MemCopy32(VGA_Address, (VGA_Image + offset), y * 320);
         y += (1 + acc); acc += 0.18;
         if (y >= 200) { y = 200; last_run = true; };
      }
      break; 
   case Interlaced:
      PCXImage(filename, VGA_Image);
      if (callback) callback(params);
      for (y = 0; y < 200; y += 2) {
         MemCopy32(VGA_Address + (y * 320), VGA_Image + (y * 320), 320);
         delay(5);
      }
      for (y = 199; y > 0; y -= 2) {
         MemCopy32(VGA_Address + (y * 320), VGA_Image + (y * 320), 320);
         delay(5);
      }
      break;
	case Rotate:
		MemCopy32(VGA_Buffer, VGA_Address, SCREEN_SIZE);
      PCXImage(filename, VGA_Image);
      if (callback) callback(params);
		Backflip(VGA_Buffer, VGA_Image, true);
		break;
   default: // = FadeOut
      Video::FadeOut(250);
   	Video::ClearScreen();
      PCXImage(filename, VGA_Address);
	   if (callback) callback(params);
      Video::FadeIn(250);
      break;
   }
}

void Draw::Backflip(addr* frontside, addr* backside, bool full) {
	const float PI = 3.14159f;
	const float CENTER_Y = 100.0f;
	int sy, ey, new_y, src_y; addr* source;
	float cos_angle; float angle = 0.0f;

	MemCopy32(SFX_Buffer, frontside, SCREEN_SIZE);
	// rotate entire screen or keep UI untouched (ingame)
	if (full) { sy=0; ey=200; } else { sy=14; ey=200-sy; }

	while (angle <= PI) {
		MemSet32(SFX_Buffer+(sy*320), 0, SCREEN_SIZE-(sy*320));
		cos_angle = cos(angle);
		source = (angle <= PI/2) ? frontside : backside;

		for (int y = sy; y < ey; y++) {
			new_y = (int)((y - CENTER_Y) * cos_angle + CENTER_Y);
			if (angle > PI/2) src_y = 199-y; else src_y = y;
			if (new_y < sy || new_y > ey) continue;
			MemCopy32(SFX_Buffer+(new_y*320), source+(src_y*320), 320);
		}

		MemCopy32(VGA_Address, SFX_Buffer, SCREEN_SIZE-(sy*320));
		angle += 0.01f;
	}
	// last tick
	MemCopy32(VGA_Address+(sy*320), backside+(sy*320), SCREEN_SIZE-(sy*320));
}

void Draw::ScrollCredits() {
   _font.Load("mindset.f08");
	MemSet32(Level_Map, 0, SCREEN_SIZE);
   ImageTransition("SCROLLER.PCX", FadeOut);
	// save image from pcx loader buffer for scroller
	MemCopy32(VGA_Image, _pcxBuffer, SCREEN_SIZE);
	// load inline image for scroller
	PCXImage("HACKER.PCX", Level_Map);
	// call scroller
   Roll_Credits("outro.txt", "shrtprse.wav", _font.Table);
}

// ============================================================================
// specific methods related to Field class
// ============================================================================

void Draw::ClearFieldBuffer() {
   memset(FLD_Buffer, 0, SCREEN_SIZE);
}

void Draw::ToggleSwitch(int offset, bool state) {
   byte* tile = state ? _switchOn : _switchOff;
   for(int y = 0; y < 3; y++) {
		memcpy(VGA_Image+offset+(y*320), tile+(y*5), 5);
   }
}

// ============================================================================
// supportive methods
// ============================================================================

void Draw::SaveCurrentScreen() {
   memcpy(VGA_Buffer, VGA_Address, SCREEN_SIZE);
}

void Draw::RestoreCurrentScreen() {
   memcpy(VGA_Address, VGA_Buffer, SCREEN_SIZE);
}

void Draw::ClearScreen(bool set_pal) {
   Video::FadeOut(250);
	Video::ClearScreen();
   if (set_pal) Video::LoadVGAPalette("CBRPAL.PCX"); 
   Video::FadeIn(250);
}

IOStatus Draw::PCXImage(char* filename, addr* target) {
	PCXHeader header;
	Shizzlabang shizzle;
	addr* image_ptr;
	int width, height;
	byte vga_byte, count;
	int vga_idx = 0, src_idx = 0;

	shizzle = Asset_Manager(filename, GRAPHICS);
	if (shizzle.status != Awesome) return shizzle.status;
	memcpy(&header, shizzle.asset.location, sizeof(PCXHeader));

	width = header.xMax + 1; height = header.yMax + 1;
	if (height * width > SCREEN_SIZE) return Image_To_Big;
	image_ptr = shizzle.asset.location + sizeof(PCXHeader);

	// RLE decoder
	while (vga_idx < width * height) {
		vga_byte = image_ptr[src_idx++];
		if ((vga_byte & 0xC0) == 0xC0) {
			count = vga_byte & 0x3F;
			vga_byte = image_ptr[src_idx++];
			for (int k = 0; k < count; k++) {
				_pcxBuffer[vga_idx++] = vga_byte;
			}
		} else {
			_pcxBuffer[vga_idx++] = vga_byte;
		}
	}

	byte pcx_palette[768];
	memcpy(pcx_palette, (addr*)shizzle.asset.location + (shizzle.asset.size - 768), 768);
	Video::StoreVGAPalette(pcx_palette, false);
   
	while (inp(0x3da) & 0x08); while (!(inp(0x3da) & 0x08));
	MemCopy32(target, _pcxBuffer, width * height);

	return Awesome;
}

void Draw::LoadGameAssets(char* filename) {
	static bool once = false;
	addr* asset_ptr; byte color_byte = 0; 
	char* asset_name = NULL; 
	byte asset_id = -1, tile_id = 0;
	byte current, width, height;
	int off, idx = 0; 
	bool error = false;

	char color2id[16][16] = {
		{ "void" }, { "field" }, { "draw" }, { "snake" },
		{ "cater" }, { "boss" }, { "unused" }, { "unused" },
		{ "unused" }, { "unused" }, { "unused" }, { "unused" },
		{ "unused" }, { "unused" }, { "unused" }, { "unused" },
	};

	for (int i = 0; i < MAX_ASSETS; i++) {
		GameAssets[i].name[0] = 0;
		for (int k = 0; k < MAX_TILES; k++) {
			GameAssets[i].tiles[k].width = NULL; 
			GameAssets[i].tiles[k].height = NULL;
			GameAssets[i].tiles[k].data[0] = 0;
		}
	}

	if (PCXImage(filename, VGA_Buffer) != Awesome) {
		Log_Info("Draw: in-game asset file not found", filename);
		return;
	}

	asset_ptr = VGA_Buffer; current = 0;
	while (!error && idx < MAX_16K && color_byte != READ_STOP) {
		color_byte = asset_ptr[idx];

		if (!once) Log_Info("Draw: reading assets, first byte: 0x%02X [%d]",
								  color_byte, color_byte);

		// skip 0 (nothing) & 15 (reserved)
		if (color_byte > 0 && color_byte < 15) {
			if (current == color_byte) 
				{ tile_id++; } else 
				{ asset_id++; tile_id = 0; }
			if (tile_id >= MAX_TILES || asset_id >= MAX_ASSETS) 
				{ error = true; continue; }

			// set name
			if (!GameAssets[asset_id].name[0]) {
				strcpy(GameAssets[asset_id].name, color2id[color_byte]);
				Log_Info("Field: [%d] byte: %d, asset '%s' identified",
							asset_id, color_byte, GameAssets[asset_id].name);
			}

			// identifier is followed by 2 bytes defining the tile size
			width  = GameAssets[asset_id].tiles[tile_id].width  = asset_ptr[idx+1];
			height = GameAssets[asset_id].tiles[tile_id].height = asset_ptr[idx+2];

			if ((width * height) <= 1 || (width * height) > 64) {
				Log_Info("Field: [%d] tile [%d] to big: %d",
							asset_id, tile_id, (width * height));
				error = true;
			}
	
			off = idx + 321; // => tile 1st byte
			byte* data = GameAssets[asset_id].tiles[tile_id].data;
			for (word y = 0; y < height; y++) {
				memcpy(data+(y*width), asset_ptr+(off+(y*320)), width);
			}

			current = color_byte;
			idx += width + 1;
		} else {
			idx++;
		}

		once = true;
	}

	if (error) {
		Log_Info("Draw: loading in-game assets failed");
	}
}

GameAsset* Draw::GetGameAsset(char* name) {
	int a_id = -1; 
	for (a_id = 0; a_id < MAX_ASSETS; a_id++) {
		if (!strcmp(GameAssets[a_id].name, name)) break;
	}
	if (a_id == -1) {
		Log_Info("Draw: asset [%s] not found", name);
		return NULL;
	}

	return &GameAssets[a_id];
}

void Draw::GameAssetTo(char* name, int t_id, int x, int y, addr* buffer) {
	GameAsset* asset;
	if (!(asset = GetGameAsset(name))) {
		Log_Info("No game asset '%s' found", name);
		return;
	}

	int width  = asset->tiles[t_id].width;
	int height = asset->tiles[t_id].height;
	byte* data = asset->tiles[t_id].data;
	int offset = 0;

	for (int ay = 0; ay < height; ay++) {
		offset = (y+ay)*320+x;
		memcpy(buffer+offset, data+(ay*width), width);
	}
}

void Draw::Print(char* text, int x, int y, byte color) {
	for (int i = 0; text[i] != '\0'; i++) {
		Letter(x + (i * 9), y, text[i], color);
	}
}

void Draw::PrintV(char* text, int x, int y, byte color) {
	for (int i = 0; text[i] != '\0'; i++) {
		Letter(x, y + (i * 9), text[i], color);
	}
}

void Draw::Letter(int x, int y, char chr, byte color) {
	int idx = (int)chr; byte* letter = _font.Table[idx];
	DrawLetter(x, y, color, letter, Draw_Target);
}

void Draw::BorderedBox(int sx, int sy, int ex, int ey) {
	Fill(sx, sy, ex - sx, ey - sy, 96);
	Line(sx, ey, ex, ey, 8);
	Line(ex, sy, ex, ey, 8);
	Line(sx, sy, ex, sy, 7);
	Line(sx, sy, sx, ey, 7);
}

void Draw::MessageBox(char* message) {
	int width = strlen(message) * 9;
	int x = 160 - (width / 2);
	int y = 98;

	int sx = x - 10;
	int ex = x + width + 10;
	int sy = y - 4;
	int ey = y + 10;

	BorderedBox(sx, sy, ex, ey);
	Print(message, x, y, 4);
}

void Draw::GetMultiPxl(int x, int y, addr* buffer) {
	Pxl pxl; pxl.color = 0; int idx = 0;

	for (int i = 0; i < PXL_SIZE; i++) {
		for (int k = 0; k < PXL_SIZE; k++) {
			pxl.x = x; pxl.offx = i;
			pxl.y = y; pxl.offy = k;
			buffer[idx++] = Video::Read(pxl);
		}
	}
}

void Draw::Pixel(int x, int y, byte color) {
	Pixel(x, y, color, VGA_Address);
}
void Draw::Pixel(int x, int y, byte color, addr* buffer) {
	for (int row = 0; row < PXL_SIZE; row++) {
		for (int col = 0; col < PXL_SIZE; col++) {
			Pxl pxl = {x, y, row, col, color};
			Video::Write(pxl, buffer);
		}
	}
}

void Draw::MultiPxl(int x, int y, byte* colors) {
	MultiPxl(x, y, colors, VGA_Address);
}
void Draw::MultiPxl(int x, int y, byte* colors, addr* buffer) {
	int idx = 0;
	for (int row = 0; row < PXL_SIZE; row++) {
		for (int col = 0; col < PXL_SIZE; col++) {
         if (colors[idx]) { // 0/black = transparent
            Pxl pxl = {x, y, row, col, colors[idx]};
            Video::Write(pxl, buffer);
         }
         idx++;
		}
	}
}

void Draw::Fill(int x, int y, int width, int height, byte color) {
	for (int row = 0; row < height; row++) {
		for (int col = 0; col < width; col++) {
			Pxl pxl = {x, y, col, row, color};
			Video::Write(pxl);
		}
	}
}

void Draw::Line(int sx, int sy, int ex, int ey, byte color) {
	int dx = abs(ex - sx);
	int dy = abs(ey - sy);
	int sxStep = (sx < ex) ? 1 : -1;
	int syStep = (sy < ey) ? 1 : -1;
	int err = dx - dy;

	while (sx != ex || sy != ey) {
		Pxl pxl = {sx, sy, 0, 0, color};
		Video::Write(pxl);
		if ((err * 2) > -dy) {
			err -= dy;
			sx += sxStep;
		}
		if ((err * 2) < dx) {
			err += dx;
			sy += syStep;
		}
	}
	Pxl pxl = {sx, sy, 0, 0, color};
	Video::Write(pxl);
}

void Draw::Rect(int x, int y, int w, int h, byte color) {
	Line(x, y, x + w, y, color);
	Line(x + w, y, x + w, y + h, color);
	Line(x, y + h, x + w, y + h, color);
	Line(x, y, x, y + h, color);
}
