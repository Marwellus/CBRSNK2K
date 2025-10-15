#include <dos.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <mem.h>
#include <malloc.h>
#include <i86.h>
#include "inc/types.h"
#include "inc/timer.h"
#include "inc/logger.h"
#include "inc/memory.h"
#include "inc/fileio.h"
#include "inc/video.hpp"

int Video::Mode = 0;
RGB Video::VGAPalette[256];

struct VGA_REG {
   word port; 
	byte reg; 
	byte val;
};

bool Video::Init() {
	if (!Mem_Init()) {
		Log_Info("Video: failed to alloc video buffers");
		return false;
	}

	return true;
}

void Video::Dispose() {
	Mem_Dispose(); // vidmem
}

void Video::SetScreen(int mode) {
	Video::Mode = mode;

	union REGS regs; 
	memset(&regs, 0, sizeof(regs));

	regs.h.ah = 0x00;
	switch (mode) {
		case MDDOS:
			regs.h.al = 0x03; // dos std
			break;
		case MD13H: // 320x200x256
			regs.h.al = 0x13;
			break;
		default:
			regs.h.al = 0x03;
	}
	int386(0x10, &regs, &regs);
}

void Video::SetRefreshRate(int hz) {
	byte overflow, v_total, v_retrace_end;
	
	// Unlock CRTC registers
	outp(0x3D4, 0x11);
	byte temp = inp(0x3D5);
	outp(0x3D5, temp & 0x7F);
	
	switch(hz) {
		case 60:
			v_total = 0x0B;
			overflow = 0x3E;
			v_retrace_end = 0x8C;
			break;		
		case 70:
		default:
			v_total = 0xBF;
			overflow = 0x1F;
			v_retrace_end = 0x8E;
			break;
	}
	
	// set rate
	outp(0x3D4, 0x06); outp(0x3D5, v_total);
	outp(0x3D4, 0x07); outp(0x3D5, overflow);
	outp(0x3D4, 0x11); outp(0x3D5, v_retrace_end);
}

byte Video::Read(Pxl pxl) {
	static byte color = 0;
	int offset = (pxl.y + pxl.offy) * 320 + (pxl.x + pxl.offx);
	color = Draw_Target[offset];
	return color;
}

// write into current Draw_Target
void Video::Write(Pxl pxl) {
	int offset = (pxl.y + pxl.offy) * 320 + (pxl.x + pxl.offx);
	*(Draw_Target + offset) = pxl.color;
}

// write into given buffer
void Video::Write(Pxl pxl, addr* buffer) {
	int offset = (pxl.y + pxl.offy) * 320 + (pxl.x + pxl.offx);
	*(buffer + offset) = pxl.color;
}

void Video::FlashColor(byte color) {
	RGB rgb = GetColorRGB(color);

	outp(0x3C8, color);
	outp(0x3C9, 63);
	outp(0x3C9, 63);
	outp(0x3C9, 63);
	delay(100);
	outp(0x3C9, rgb.red);
	outp(0x3C9, rgb.green);
	outp(0x3C9, rgb.blue);
}

RGB Video::GetColorRGB(byte color) {
	outp(0x3C7, color);

	static RGB rgb;
	rgb.red = inp(0x3C9);
	rgb.green = inp(0x3C9);
	rgb.blue = inp(0x3C9);

	return rgb;
}

void Video::SetColorRGB(int index, byte r, byte g, byte b) {
	outp(0x3c8, index);
	outp(0x3c9, r);
	outp(0x3c9, g);
	outp(0x3c9, b);
}

void Video::LoadVGAPalette(char* filename) {
	if (Load_Palette_PCX(filename) == No_File) 
		Log_Info("Palette-File not found");
	Video::StoreCurrentPalette();
}

void Video::StoreVGAPalette(byte* pcx_pal, bool set) {
	for (int i = 0; i < 256; i++) {
		RGB color = { 0, 0, 0 };
		color.red 	= pcx_pal[i * 3]  >> 2;
		color.green = pcx_pal[i * 3 + 1] >> 2;
		color.blue 	= pcx_pal[i * 3 + 2] >> 2;
		VGAPalette[i] = color;
	}
	if (set) RestorePalette();
}

void Video::StoreCurrentPalette() {
	for (int i = 0; i < 256; i++) {
		VGAPalette[i] = GetColorRGB(i);
	}
}

void Video::RestorePalette() {
	for (int i = 0; i < 256; i++) {
		SetColorRGB(i, VGAPalette[i].red, VGAPalette[i].green, VGAPalette[i].blue);
	}
}

void Video::FadeOut(int duration_ms) {
	int steps = 32; // fade steps
	int delay_per_step = duration_ms / steps;

	for (int step = 0; step < steps; step++) {
		int fade_factor = steps - step - 1; // 31 down to 0

		// fade each color in the palette
		for (int color = 0; color < 256; color++) {
			RGB orig = VGAPalette[color];

			byte new_red = (orig.red * fade_factor) / (steps - 1);
			byte new_green = (orig.green * fade_factor) / (steps - 1);
			byte new_blue = (orig.blue * fade_factor) / (steps - 1);

			SetColorRGB(color, new_red, new_green, new_blue);
		}

		delay(delay_per_step);
	}
}

void Video::FadeIn(int duration_ms) {
	int steps = 32; // fade steps
	int delay_per_step = duration_ms / steps;

	for (int step = 0; step < steps; step++) {
		int fade_factor = step; // 0 up to 31

		// fade each color in the palette
		for (int color = 0; color < 256; color++) {
			RGB orig = VGAPalette[color];

			byte new_red = (orig.red * fade_factor) / (steps - 1);
			byte new_green = (orig.green * fade_factor) / (steps - 1);
			byte new_blue = (orig.blue * fade_factor) / (steps - 1);

			SetColorRGB(color, new_red, new_green, new_blue);
		}

		delay(delay_per_step);
	}
}

void Video::ClearScreen() {
	memset(VGA_Address, 0, SCREEN_SIZE);
}

void Video::ColorSpread(RGB fromColor, RGB toColor, int fromIdx, int toIdx) {
	SetColorRGB(fromIdx, fromColor.red, fromColor.green, fromColor.blue);
	SetColorRGB(toIdx, toColor.red, toColor.green, toColor.blue);

	int delta = toIdx - fromIdx;
	int stepR = (toColor.red - fromColor.red) / delta;
	int stepG = (toColor.green - fromColor.green) / delta;
	int stepB = (toColor.blue - fromColor.blue) / delta;

	int red = fromColor.red, green = fromColor.green, blue = fromColor.blue;

	for (int i = fromIdx + 1; i < toIdx - 1; i++) {
		red += red + stepR < 63 ? stepR : 0;
		green += green + stepG < 63 ? stepG : 0;
		blue += blue + stepB < 63 ? stepB : 0;
		SetColorRGB(i, red, green, blue);
	}
}

void Video::PrintC(int x, int y, char *text, byte color) {
	word* video = (word*)0xB8000;
	int pos = y * 80 + x;
	while (*text)
		video[pos++] = (*text++) | (color << 8);
}

void Video::SplashScreen() {
   int row = 0;
	PrintC(0, row++, "ษอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป", 0x1E);
	PrintC(0, row++, "บ                 *** Cybersnake 2000 Turbo ***                 บ", 0x1E);
	PrintC(0, row++, "บ                 [2025] Retronomitron(tm) Ltd.                 บ", 0x1E);
	PrintC(0, row++, "ฬอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออน", 0x1E);
	PrintC(0, row,   "บ                                                               บ", 0x1E);
	PrintC(3, row++, "Thanks for playing my game! I hope, you enjoyed it!", 0x1F);
	PrintC(0, row,   "บ                                                               บ", 0x1E);
	PrintC(3, row++, "If you wish to contact me, drop me an", 0x1F);
	PrintC(0, row,   "บ                                                               บ", 0x1E);
	PrintC(3, row++, "e-mail: <marwellus@disruption.global>", 0x1E);
	PrintC(0, row++, "บ                                                               บ", 0x1E);
	PrintC(0, row,   "บ                                                               บ", 0x1E);
	PrintC(3, row++, "'I have allocated DOS memory in protected mode!'", 0x1C);
	PrintC(0, row,   "บ                                                               บ", 0x1E);
	PrintC(3, row++, "- Chuck Noland, 'Cast away'", 0x1C);
	PrintC(0, row++, "ศอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออผ", 0x1E);
   for (int i = 0; i < row; i++) printf("\n");
}