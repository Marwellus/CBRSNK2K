#include <string.h>
#include <stdlib.h>
#include <conio.h>
#include <stdio.h>
#include <malloc.h>
#include "inc/types.h"
#include "inc/logger.h"
#include "inc/memory.h"
#include "inc/dsp.h"
#include "inc/scroller.h"

// ============================================================================
// INFINITY SCROLLER DELUXE PRO(c) V17.1 by Redonkulous Corp.
// ============================================================================

#define SCREEN_SIZE 64000
#define BUFFER_SIZE 128000
#define MAX_LINES 256

static char _text[MAX_LINES][42];
static addr* Roll_Buffer = NULL;
static addr* Inline_Image	 = NULL;
static addr* Background_Image	= NULL;

static bool prep_outro(char* filename, int* rows);
static bool glitch_party(int glitch_amt, int write_y);
static void render_text(char* text, int y, byte (*font)[8], byte color);
static bool render_image(int y, int x);

void Roll_Credits(char* text_file, char* sample_file, byte (*font)[8]) {   
   DSP_Set_Volume(80);
   DSP_Play(sample_file, true);
   Text_Scroller(text_file, font);
}

void Text_Scroller(char* filename, byte (*font)[8]) {
	int  bytes_left; char key; char ctrl;
	int  read_pos = 0, frame = 1, 
        speed = 4, line_count = 0, 
        scroll_y = 1, write_y = 200, 
        i = 0, row = 0, glitch_amt = 1;
   bool exit = false, 
        special_case = true,
		  img_content = false;
   byte color = 58; char c_str[4] = { 0, 0, 0, 0 };
	
	// repurpose existing buffers
	Roll_Buffer			= FLD_Buffer;
	Inline_Image		= Level_Map;	
	Background_Image	= VGA_Image;

	if (!prep_outro(filename, &line_count)) return;
   MemSet32(VGA_Buffer, 0, BUFFER_SIZE);
   
	// render entire first page
	while (row < line_count && write_y < 400) {
		// get color from "%123%\n"
		if (_text[row][0] == '%') {
			memcpy(c_str, _text[row] + 1, 3);
			color = atoi(c_str); row++;
		}
		render_text(_text[row], write_y, font, color);
		write_y += 10; row++;
	}

	while (!exit) {
		while (inp(0x3da) & 0x08); 
		while (!(inp(0x3da) & 0x08));

      // infinity scroller
		if (!(frame % speed)) {
			MemCopy32(Roll_Buffer, Background_Image, SCREEN_SIZE);
			bytes_left = BUFFER_SIZE - read_pos;
			if (bytes_left >= SCREEN_SIZE) {
				TransCopy(Roll_Buffer, VGA_Buffer + read_pos, SCREEN_SIZE);
			} else {
				TransCopy(Roll_Buffer, VGA_Buffer + read_pos, bytes_left);
				TransCopy(Roll_Buffer + bytes_left, VGA_Buffer, SCREEN_SIZE - bytes_left);
			}
			MemCopy32(VGA_Address, Roll_Buffer, SCREEN_SIZE);
			read_pos = ((read_pos + 320) % BUFFER_SIZE);
		}

		// render 2 text lines until none is left
		if (frame < 6000 && !(frame % 80)) {
			write_y = ((read_pos / 320) - 20) % 400; // 20px behind viewport
			if (write_y < 0) write_y += special_case ? 20 : 400;
			special_case = false;

			MemSet32(VGA_Buffer + (write_y * 320), 0, 6400);

			for (i = 0; i < 2 && row < line_count; i++) {
				ctrl = _text[row][0];
				// get color code from "%123%\n"
				if (ctrl == '%') {
					memcpy(c_str, _text[row] + 1, 3);
					color = atoi(c_str); row++;
				}
				// check, if image should be drawn
				if (ctrl == '$') { img_content = true; row++; }
				if (img_content) {
					img_content = render_image(write_y, 80);
					if (img_content) write_y += 10;
					continue;
				}
				// print line
				render_text(_text[row], write_y, font, color);
				write_y += 10; row++;
			}
		}

		// glitch party at the end
		if (frame > 5500 && !(frame % 420)) {
			glitch_amt += glitch_amt < 9 ? 1 : 0;
			speed -= speed > 2 ? 1 : 0;
		}
		if (frame > 5600 && !(frame % (80 / glitch_amt))) {
			write_y = ((read_pos / 320) - 20) % 400;
			if (write_y < 0) write_y += 400;
         exit = glitch_party(glitch_amt, write_y);
		}
      
		if (kbhit()) {
			key = getch();
			if (key == 27) exit = true;
		}

		frame++;
	}
}

static bool glitch_party(int glitch_amt, int write_y) {
   byte pxl_row[320]; byte glitch[32]; byte len;
   int  h, i, k; int offset = 0;
   static byte* raw_code = (byte*)0x0700; 
   MemSet32(pxl_row, 0, 320);

   for (h  = 0; h < glitch_amt; h++) {
      MemSet32(pxl_row, 0, 320);
      if (glitch_amt >= 7) {
         return true;
      } else if (glitch_amt >= 5) {
         if (glitch_amt == 6) DSP_Set_Distortion(80);
         MemCopy32(pxl_row, raw_code, 320);
         raw_code += 320;
      } else if (glitch_amt < 5) {
         if (glitch_amt > 2) DSP_Set_Distortion(glitch_amt * 4);
         for (i = 0; i < glitch_amt; i++)	{
            MemSet32(glitch, 0, 32);
            len = rand() % 32;
            for (k = 0; k < len; k++) {
               glitch[k] = rand() % 255;
            }
            offset = rand() % 300;
            MemCopy32(pxl_row+offset, glitch, len);
         }
      }
      MemCopy32(VGA_Buffer+(write_y*320), pxl_row, 320);
      write_y += 8 / glitch_amt;
   }

   return false;
}

static bool prep_outro(char* filename, int* rows) {
   FILE* file; byte *buffer;
   int len; long size;
   char filepath[256] = "assets\\";
	strcat(filepath, filename);

	file = fopen(filepath, "rb");
	if (!file) return false;

   fseek(file, 0, SEEK_END); 
	size = ftell(file); rewind(file); 
   buffer = (byte*)malloc(size + 1);
   fread(buffer, size, 1, file); 
   
   buffer[size] = '\0';
   fclose(file);

	len = 0; *rows = 0;
	while (*buffer != '\0' && *rows < MAX_LINES) {      
		if (*buffer == '\n') {
			_text[*rows][len] = '\0';
			len = 0; (*rows)++; buffer++;
         continue;
		}
		_text[*rows][len] = *buffer;
		len += len < 40 ? 1 : 0;
		buffer++;
	}

   free(buffer);
   return true;
}

static void render_text(char* text, int y, byte (*font)[8], byte color) {
	int pixel_x, pixel_y, ascii, char_x;
   int i = 0, row = 0, col = 0;
   int text_width = strlen(text) * 9;
	int start_x = 160 - (text_width / 2);
	byte* letter;

	for (i = 0; text[i] != '\0'; i++) {
		char_x = start_x + (i * 9);
		ascii  = (int)text[i]; 
		letter = font[ascii];
		DrawLetter(char_x+1, y+1, 173, letter, VGA_Buffer);
		ColorLetter(char_x,  y, color, letter, VGA_Buffer);
	}
}

static bool render_image(int y, int x) {
	static int img_row; int line;
	if (img_row < 130) {
		for (line = 0; line < 10; line++) {
			TransCopy(
				VGA_Buffer+((y+line)*320)+x, 
				Inline_Image+((img_row+line)*160), 
				160
			);
		}
		img_row += 10; 
		return true;
	}
	return false;
}
