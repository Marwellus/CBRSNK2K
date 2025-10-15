#include <dos.h>
#include <mem.h>
#include <malloc.h>
#include "inc/types.h"
#include "inc/logger.h"
#include "inc/memory.h"

/* ATTN:
 *	take care with these buffers and be always
 *	cautious from were you access them and why
*/

addr* VGA_Address = NULL; // actual vga ram address
addr* Draw_Target = NULL; // current draw target for game
addr* Max_Buffer  = NULL; // mem block

addr* VGA_Buffer  = NULL; // back buffer
addr* SFX_Buffer  = NULL; // effects buffer
addr* FLD_Buffer  = NULL; // field render buffer
addr* VGA_Image   = NULL; // original level image
/* non-gfx */
addr* Level_Map	= NULL; // collision map
addr* Bit_Mask    = NULL; // bit mask
/* small separate buffer */
addr* VGA_Palette = NULL; // VGA palette data

bool Mem_Init() {
	VGA_Address = (addr*)0xA0000;
   VGA_Palette = (byte*)malloc(768);
   Max_Buffer  = (byte*)malloc(MAX_BUFFER_SIZE);
	if (!Max_Buffer || !VGA_Palette) return false;

	// setup pointers
   VGA_Buffer  = Max_Buffer;
   SFX_Buffer  = Max_Buffer + (STD_BUFFER_SIZE);
   FLD_Buffer  = Max_Buffer + (STD_BUFFER_SIZE * 2);
   VGA_Image   = Max_Buffer + (STD_BUFFER_SIZE * 3);
	Level_Map   = Max_Buffer + (STD_BUFFER_SIZE * 4);
   Bit_Mask    = Max_Buffer + (STD_BUFFER_SIZE * 5);
	Draw_Target = VGA_Address;
	return true;
}

void Mem_Dispose(void) {
   if (Max_Buffer)  { free(Max_Buffer); Max_Buffer = NULL; }
   if (VGA_Palette) { free(VGA_Palette); VGA_Palette = NULL; }
}

/* allocate real mode segment (max ~60kb) */
bool Real_Malloc(Real_Segment* segment) {
	ulong linear_address;
	ulong real_address;
	int page1, page2;
	int temp_segment;
	int paragraphs;
	union REGS regs;
	
	memset(&regs, 0, sizeof(regs));
	// dpmi 0100H, allocate DOS segment, 1st try
	paragraphs = (segment->size + 15) / 16; // (16-byte blocks)
	regs.w.ax = 0x0100; regs.w.bx = paragraphs;
	int386(0x31, &regs, &regs);
	
	if (regs.w.cflag) return false;

	// physical address for DMA controller
	temp_segment = regs.w.ax;
	real_address = ((ulong)temp_segment) << 4;
	page1 = (int)(real_address >> 16);
	page2 = (int)((real_address + segment->size - 1) >> 16);

	// crossing segment boundary?
	if (page1 != page2) {
		Real_Free(segment->selector);
		real_address = 0; temp_segment = 0;
		
		// 2nd try
		memset(&regs, 0, sizeof(regs));
		regs.w.ax = 0x0100; regs.w.bx = paragraphs;
		int386(0x31, &regs, &regs);

		if (regs.w.cflag) {
			// if it fails two times, give up
         Log_Info("MEM: failed to allocate real mode segment");
			Real_Free(segment->selector);
			return false;
		}
	}

	segment->segment  = regs.w.ax; // real-mode segment
	segment->selector = regs.w.dx; // protected-mode selector

   memset(&regs, 0, sizeof(regs));
	// dpmi 0006H, get segment base address
   regs.w.ax = 0x0006; 
	regs.w.bx = segment->selector;
   int386(0x31, &regs, &regs);

   if (regs.w.cflag) { 
		Log_Info("MEM: failed, code: 0x%04X", regs.w.cflag);
		Real_Free(segment->selector); return false; 
	}
   
   // build 32-bit linear address from CX:DX
   linear_address = ((ulong)regs.w.cx << 16) | regs.w.dx;
	segment->data = (addr*)linear_address;

	real_address = ((ulong)segment->segment) << 4;
	segment->real = (addr*)real_address;
	segment->page = (byte)(real_address >> 16);
	segment->offset = (int)(real_address & 0xFFFF);
	
	return true;
}

/* free real mode segment */
void Real_Free(int selector) {
	union REGS regs;
	if (selector != NULL) {
		// dpmi 101H, free DOS block
		memset(&regs, 0, sizeof(regs));
		regs.w.ax = 0x0101;
		regs.w.dx = selector;
		int386(0x31, &regs, &regs);			
	}
}

/* special asm routines to speed a few things up a (tiny) bit */

void MemCopy32(addr* dest, addr* src, int size) {
   _asm {
      push esi
      push edi
      push ecx
        
      mov esi, src         ; source ptr
      mov edi, dest        ; destination ptr  
      mov ecx, size        ; byte count
      shr ecx, 2           ; dword count
      
      cld                  ; forward copy
      rep movsd            ; copy from [ESI] to [EDI]
        
      pop ecx
      pop edi
      pop esi
   }
}

/* copy none zero bytes only (transparency) */
void TransCopy(addr* dest, addr* src, int size) {
   _asm {
		pushad
		
      mov esi, src       ; source ptr
      mov edi, dest      ; destination ptr  
      mov ecx, size      ; byte count
      shr ecx, 2         ; dword count
        
   copy_loop:
      mov eax, [esi]     ; load 4 bytes from source
      test eax, eax      ; check if all are zero
      jz skip_dword      ; if yes, skip
      
      mov ebx, eax       ; else check each byte
      test bl, bl        ; first byte
      jz check_2nd
      mov [edi], bl
        
   check_2nd:
      test bh, bh        ; second byte
      jz check_3rd  
      mov [edi+1], bh
        
   check_3rd:
		shr ebx, 16        ; get upper 2 bytes
      test bl, bl        ; third byte
      jz check_4th
      mov [edi+2], bl
        
   check_4th:
      test bh, bh        ; fourth byte
      jz skip_dword
      mov [edi+3], bh
        
   skip_dword:
      add esi, 4         ; next source dword
      add edi, 4         ; next dest dword
      dec ecx
      jnz copy_loop
      
		popad
   }
}

void MemSet32(addr* dest, dword value, uint count) {
	_asm {
		pushad

		mov edi, dest
		mov ecx, count
		mov eax, value
		
		mov edx, ecx
		shr ecx, 2				; count / 4 = number of DWORDs
		jz do_bytes				; skip if less than 4 bytes
		
		test edi, 3				; Check if aligned
		jz aligned_already
		
	align_loop:
		mov [edi], al
		inc edi
		dec edx
		test edi, 3
		jnz align_loop
		
		mov ecx, edx
		shr ecx, 2				; recalc DWORD count after alignment
		
	aligned_already:
		rep stosd
		
	do_bytes:
		mov ecx, edx
		and ecx, 3          ; remainder = count & 3
		jz done
		rep stosb           ; remaining 0-3 bytes
		
	done:
		popad
	}
}

void DrawTile(int x, int y, byte* tile, int width, int height, addr* target) {
	_asm {
		pushad

      mov edi, target
      mov eax, y
      mov ebx, 320
      mul ebx 				
      add eax, x              
      add edi, eax 			; target + (y * 320 + x)

		mov esi, tile
		xor edx, edx

	row_loop:
		push edi					; save pos
		xor ecx, ecx

	col_loop:
		mov al, [esi]			; fetch byte
		cmp al, 255				; is 255?
		je skip_byte			; then skip it
		mov [edi], al			; else write to target
		
   skip_byte:					
		inc esi
		inc edi		
		inc ecx
		cmp ecx, width
		jl col_loop
		
		pop edi
		add edi, 320 			; next row in buffer

		inc edx
      cmp edx, height
		jl row_loop

		popad
	}
}

void DrawLetter(int x, int y, byte color, byte* letter, addr* target) {
	_asm {
      pushad
						  
      mov edi, target
      mov eax, y
      mov ebx, 320
      mul ebx 				
      add eax, x              
      add edi, eax 			; target + (y * 320 + x)
      
      mov esi, letter
      movzx edx, color
      xor ecx, ecx
      
	row_loop:
      push edi 				; save pos
      mov al, [esi+ecx]		; fetch byte
      mov ah, 0x80 			; bit mask
      push ecx 				; safe row counter
      mov ecx, 8 				; = 8 columns
      
   col_loop:
      test al, ah				; bit set?
      jz skip_pixel 			; nope, skip      
      mov [edi], dl 			; else write to target
      
   skip_pixel:
      inc edi					; next column
      shr ah, 1 				; shift bit mask right
      dec ecx
      jnz col_loop
      
      pop ecx 					; get row counter
      pop edi 					; back to row start
      add edi, 320 			; next row
		
      cmp edx, 16				; rainbow colors
      jl no_color_change
      cmp edx, 37
      jg check_steel
      inc edx
      jmp no_color_change
        
   check_steel:
		cmp edx, 38				; steel colors
      jl no_color_change
      cmp edx, 45
      jg no_color_change
      inc edx

   no_color_change:
      inc ecx
      cmp ecx, 8
      jl row_loop
      
      popad
	}
}

void ColorLetter(int x, int y, byte color, byte* letter, addr* target) {
	_asm {
      pushad
						  
      mov edi, target
      mov eax, y
      mov ebx, 320
      mul ebx 				
      add eax, x              
      add edi, eax 			; target + (y * 320 + x)
      
      mov esi, letter
      movzx edx, color
      xor ecx, ecx
      
	row_loop:
      push edi 				; save pos
      mov al, [esi+ecx]		; fetch byte
      mov ah, 0x80 			; bit mask
      push ecx 				; safe row counter
      mov ecx, 8 				; = 8 columns
      
   col_loop:
      test al, ah				; bit set?
      jz skip_pixel 			; nope, skip      
      mov [edi], dl 			; else write to target
      
   skip_pixel:
      inc edi					; next column
      shr ah, 1 				; shift bit mask right
      dec ecx
      jnz col_loop
      
      pop ecx 					; get row counter
      pop edi 					; back to row start
      add edi, 320 			; next row
		
      inc edx					; inc color value
	   inc ecx
      cmp ecx, 8
      jl row_loop
      
      popad
	}
}
