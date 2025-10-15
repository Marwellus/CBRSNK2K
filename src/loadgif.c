#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "inc/types.h"
#include "inc/logger.h"
#include "inc/fileio.h"
#include "inc/loadgif.h"

// LZW decompressor
static LZWEntry *code_table;
static word initial_code_size;
static word code_size;
static word clear_code;
static word end_code;
static word next_code;
static word max_code;
static int code_count = 0;

// pxl pool
static addr* pixel_pool = NULL;
static int pool_position = 0;
static int pool_size = 0;

static void init_pixel_pool();
static void dispose_pixel_pool();
static byte* alloc_pixels(int length);

// bit stream reader
static addr* data_ptr;
static word bits_in_buffer;
static word bit_buffer;

static bool lzw_decoder(byte initial_code_size);
static void set_data_source(byte* data);
static void initialize_table(byte initial_code_size);
static bool decode_to_buffer(byte* output_buffer, int expected_pixels);
static void lzw_dispose();

/*
 * This loader is designed to load specifically prepared GIF images
 * and nothing else. They've been made and saved with Graf2X:
 * Non-Interlaced, 3 layers, 256 color global palette
 * Any extension is skipped, only 0x2C matters
*/
IOStatus Load_Level(char* filename, SNKMap* map_data) {
	GIFHeader header;
	GIFLogicalScreen screen;
	GIFImageDescriptor image_desc;
	Shizzlabang shizzle;
   IOStatus status;
	addr* gif_ptr;
   addr* lzw_data;
   addr* temp_buffer;
   addr* target_buffer;
   byte ext_size, ext_type;
   int src_offset, dst_offset, row;
   int lzw_code_size;
   int block_size;
   int layer;
   int expected_pixels;
   int color_table_size = 0;
   int lzw_data_pos = 0;
   int block_count = 0;

	shizzle = Asset_Manager(filename, GRAPHICS);
	if (shizzle.status != Awesome) return shizzle.status;
	gif_ptr = (byte*)shizzle.asset.location;

	memcpy(&header, gif_ptr, sizeof(GIFHeader));
	gif_ptr += sizeof(GIFHeader);
	memcpy(&screen, gif_ptr, sizeof(GIFLogicalScreen));
	gif_ptr += sizeof(GIFLogicalScreen);

   // Log_Info("GIF: LSD size: %d x %d", screen.width, screen.height);

	// get global color table
	if (screen.packed & 0x80) {
		color_table_size = 3 * (1 << ((screen.packed & 0x07) + 1));
		memcpy(map_data->palette, gif_ptr, 768);
		gif_ptr += color_table_size;
	} else {
		Log_Info("GIF: No global color table!");
	}

   init_pixel_pool(); // pixel_pool
	lzw_data = (byte*)malloc(32768);
   temp_buffer = (byte*)malloc(64000);
   if (!temp_buffer || !lzw_data || !pixel_pool) return Cache_Fail;

	// non-interlaced, 3 layers, 256 global colors
	for (layer = 0; layer < 3; layer++) {
      // target buffer
		switch (layer) {
         case 0: target_buffer = map_data->graphics; break;
         case 1: target_buffer = map_data->level_data; break;
         case 2: target_buffer = map_data->bit_mask; break;
		}

      // Log_Info("GIF: Layer %d: Block type 0x%02X", layer, *gif_ptr);

		// File Termination - must not be reached at this point
		if (*gif_ptr == 0x3B) return Horse_Shit;
      // Extension Block, skip everything since it doesn't matter here
      if (*gif_ptr == 0x21) {
         gif_ptr++; ext_type = *gif_ptr++;
         do { ext_size = *gif_ptr++; gif_ptr += ext_size; } 
         while (ext_size != 0);
      }
      // Local Image Descriptor - must be there or death
		if (*gif_ptr != 0x2C) return Horse_Shit;
      
		memcpy(&image_desc, gif_ptr, sizeof(GIFImageDescriptor));
		gif_ptr += sizeof(GIFImageDescriptor);

      // Log_Info("GIF: ID pos-x: %d pos-y: %d", image_desc.left, image_desc.top);
      // Log_Info("GIF: ID size-x: %d size-y: %d", image_desc.width, image_desc.height);

		// should not be there, skip
		if (image_desc.packed & 0x80) {
         Log_Info("GIF: Local table detected!");
			color_table_size = 3 * (1 << ((image_desc.packed & 0x07) + 1));
			gif_ptr += color_table_size;
		}

      // init decoder
		lzw_data_pos = 0; lzw_code_size = *gif_ptr++;
		if (!lzw_decoder(lzw_code_size)) return Cache_Fail;

		do {
			block_size = *gif_ptr++;
			if (block_size > 0) {
				memcpy(lzw_data + lzw_data_pos, gif_ptr, block_size);
				lzw_data_pos += block_size;
				gif_ptr += block_size;
			}
         block_count++;
		} while (block_size != 0);


      // Log_Info("LZW: data collected: %d bytes from %d sub-blocks", lzw_data_pos, block_count);
      // Log_Info("LZW: start decoding, data size: %d bytes", lzw_data_pos);
		set_data_source(lzw_data);
		expected_pixels = image_desc.width * image_desc.height;

      // clean up any mess
      memset(temp_buffer, 0, 64000);
      memset(target_buffer, 0, 64000);

      // decode ...
		if (!decode_to_buffer(temp_buffer, expected_pixels)) {
         // ... and fail :-(
			free(lzw_data); lzw_dispose();
			return Cluster_Fuck; 
		} else {
         // ... or recreate image :-)
         for (row = 0; row < image_desc.height; row++) {
            src_offset = row * image_desc.width;
            dst_offset = (image_desc.top + row) * 320 + image_desc.left;
            memcpy(target_buffer + dst_offset, 
                   temp_buffer + src_offset, 
                   image_desc.width);
         }
      }

      lzw_dispose();
	}

   dispose_pixel_pool();
   free(lzw_data); free(temp_buffer);
	// Log_Info("GIF loaded: %s (3 layers, %dx%d)", filename, screen.width, screen.height);

	return Awesome;
}

/* PIXEL POOL */
static void init_pixel_pool() {
	pixel_pool = (byte*)malloc(POOL_SIZE);
   if (!pixel_pool) Log_Info("GIF: Failed to allocate pixel pool!");
	pool_position = 0;
}

static void dispose_pixel_pool() {
	if (pixel_pool) {
      free(pixel_pool);
	   pixel_pool = NULL;
	   pool_position = 0;
   }
}

static byte* alloc_pixels(int length) {
   byte* result;
	if (pool_position + length > POOL_SIZE)
		return NULL;

	result = pixel_pool + pool_position;
	pool_position += length;
	return result;
}

/* DECODER */
static bool lzw_decoder(byte init_code_size) {
	code_table = (LZWEntry*)malloc((4096 * sizeof(LZWEntry))); // 32kb w/o pxl data
	if (!code_table) return false;

   initial_code_size = init_code_size;

   // set initial values
	clear_code = 1 << initial_code_size;
	end_code = clear_code + 1;

   // Log_Info("LZW: Code  size=%d, Clear=%d, End=%d", code_size, clear_code, end_code);
	initialize_table(initial_code_size);

   return true;
}

static void lzw_dispose() {
   if (code_table) {
      free(code_table);
      code_table = NULL;
   }
}

static void initialize_table(byte initial_code_size) {
   word i; word num_colors = 1 << initial_code_size;
   memset(code_table, 0, (4096 * sizeof(LZWEntry)));
   
   pool_position = 0;
   code_size = initial_code_size + 1;
	next_code = end_code + 1;
	max_code = (1 << code_size) - 1;

	// read colors
	for (i = 0; i < num_colors; i++) {
      code_table[i].pixels = alloc_pixels(1);
		code_table[i].code = i;
		code_table[i].length = 1;
		code_table[i].pixels[0] = (byte)i;
	}

	code_table[clear_code].length = 0;
	code_table[end_code].length = 0;
}

static void set_data_source(byte* data) {
	data_ptr = data;
	bit_buffer = 0;
	bits_in_buffer = 0;
}

static word read_code() {
   word code = 0; 
   word take_bits = 0; 
   word bits_needed = code_size;
   word bits_taken = 0;
   
   // stolen from by grafx2 decoder
   while (bits_needed) {
      if (!bits_in_buffer) {
         bit_buffer = *data_ptr++;
         bits_in_buffer = 8;
      }

      take_bits = (bits_needed <= bits_in_buffer) ? bits_needed : bits_in_buffer;
      code |= (bit_buffer & ((1 << take_bits) - 1)) << bits_taken;
        
      bit_buffer >>= take_bits;
      bits_in_buffer -= take_bits;
      bits_taken += take_bits;
      bits_needed -= take_bits;
   }
   
   code_count++;
   if (code >= 4095) Log_Info("LZW: warning! code #%d, code: 0x%02X", code_count, code);
   return code;
}

static bool check_next_code()  {
   // set code size as needed
   if (next_code > max_code && code_size < 12) {
      max_code = ((1 << (++code_size)) - 1);
      // Log_Info("LZW: code #%d, new code_size: %d, max_code: %d", code_count, code_size, max_code);
   }
   // <insert case here, return false will break decoder-loop>
   return true;
}

static bool decode_to_buffer(byte* output_buffer, int expected_pixels) {
	int pixels_written = 0;
	word code, old_code = 0;
   word old_len = 0, new_len = 0;
	bool first_code = true;

	while (pixels_written < expected_pixels) {
		code = read_code();

		// end of information
		if (code == end_code) break;
		// clear code || out of space => table reset
		if (code == clear_code || next_code >= 4095)
      {
			initialize_table(
            (clear_code == 256) 
            ? 8 : (clear_code == 128)
            ? 7 : (clear_code == 64)
            ? 6 : (clear_code == 32)
            ? 5 : (clear_code == 16)
            ? 4 : (clear_code == 8)
            ? 3 : 2
         );
			first_code = true;
         code_count = 0;
			continue;
		}

		if (first_code) 
      {
			if (code < next_code && code_table[code].length > 0) 
         {
            memcpy(output_buffer + pixels_written,
                   code_table[code].pixels,
                   code_table[code].length);

            pixels_written += code_table[code].length;            
			}
			first_code = false;
		}
      else 
      {
         old_len = code_table[old_code].length; new_len = old_len + 1;

			if (code < next_code) {
            memcpy(output_buffer + pixels_written,
                   code_table[code].pixels,
                   code_table[code].length);
            pixels_written += code_table[code].length;

            // create new entry
            if (next_code < 4096) {
               code_table[next_code].pixels = alloc_pixels(new_len);
               // old code + first pixel from new code
               memcpy(code_table[next_code].pixels,
                      code_table[old_code].pixels,
                      code_table[old_code].length);
               
               code_table[next_code].pixels[old_len] = code_table[code].pixels[0];
               code_table[next_code].length = new_len;
               code_table[next_code].code = next_code;
               next_code++;
	         }               			
			} 
         else if (code == next_code) 
         {
				// special case
            if (next_code < 4096) {
               code_table[next_code].pixels = alloc_pixels(new_len);

               memcpy(code_table[next_code].pixels,
                      code_table[old_code].pixels,
                      code_table[old_code].length);

               code_table[next_code].pixels[old_len] = code_table[old_code].pixels[0];
               code_table[next_code].length = new_len;
               code_table[next_code].code = next_code;

               memcpy(output_buffer + pixels_written,
                      code_table[next_code].pixels,
                      code_table[next_code].length);

               pixels_written += code_table[next_code].length;
               next_code++;
            }
			}
		}

		old_code = code;
      if (!check_next_code()) break;
	}
   
   // Log_Info("LZW: Pixels written: %d/%d", pixels_written, expected_pixels);
	return pixels_written == expected_pixels;
}

