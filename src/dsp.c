#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <i86.h>
#include "inc/types.h"
#include "inc/memory.h"
#include "inc/logger.h"
#include "inc/fileio.h"
#include "inc/dsp.h"

#define SB_RESET 0x6
#define SB_READ 0xA
#define SB_READ_STATUS 0xE
#define SB_WRITE 0xC

#define SB_ENABLE 0xD1
#define SB_DISABLE 0xD3
#define SB_SET_FREQUENCY 0x40
#define SB_SINGLE_CYCLE 0x14

#define MASK_REGISTER 0x0A
#define MODE_REGISTER 0x0B
#define MSB_LSB 0x0C
#define DMA_CHANNEL_0 0x87
#define DMA_CHANNEL_1 0x83
#define DMA_CHANNEL_3 0x82

#define BUFFER_SIZE 32768

bool _dspEnabled = false;

static Real_Segment dma_buffer = {0};

word _sbIRQ;  /* detected from BLASTER */
word _sbDMA;  /* default 1 */
word _sbBase; /* default 220h */

volatile int _playing = 0;
volatile int _currentChunk = 0;
volatile int _totalChunks = 0;
volatile int _numberOfSamples;
volatile int _lastChunkSize = 0;

char* _currentFilename;
byte* _cachedSampleData = NULL;
uint _sampleDataSize = 0;
uint _sampleDataOffset = 0;
bool _longPlay;
int _distortionLevel = 0; // for some effects

void (__interrupt __far *old_irq)();

static int  dsp_detect();
static void dsp_init();
static word reset_dsp(word port);
static void write_dsp(byte command);
static int  is_dsp_busy();
static void dsp_playback();
static void init_irq_dpmi();
static void deinit_irq_dpmi();
static void dsp_set_master_volume(byte volume);

// public

bool DSP_Test() {
	if (!dsp_detect()) return false;

	dsp_init();
	_dspEnabled = true;
	return _dspEnabled;
}

void DSP_PlayLeft(char* filename) {
	DSP_Play(filename, false);
	
	// outp(_sbBase + 0x4, 0x04);
   // outp(_sbBase + 0x5, 0xE7);

	// DSP_Play(filename, false);

   // outp(_sbBase + 0x4, 0x04);
   // outp(_sbBase + 0x5, 0xEE);
}

void DSP_PlayRight(char* filename) {
	DSP_Play(filename, false);

	// outp(_sbBase + 0x4, 0x04);
	// outp(_sbBase + 0x5, 0x7E);

	// DSP_Play(filename, false);

   // outp(_sbBase + 0x4, 0x04);
   // outp(_sbBase + 0x5, 0xEE);
}

bool DSP_Play(char* filename, bool long_play) {
	Shizzlabang shizzle;
	ulong file_size = 0;
   _currentFilename = filename;
   _longPlay = long_play;   

   if (!_dspEnabled) return false;
	if (_playing) DSP_Stop();

	reset_dsp(_sbBase);
	write_dsp(SB_ENABLE);

   if (!long_play)  {
      // use cache for smaller samples
      shizzle = Asset_Manager(filename, SAMPLES);
      if (shizzle.status != Awesome) {
         // Log_Info("Nope @ file: %s, %d", filename, shizzle.status);
         return 0;
      }

      // point to cached data and skip WAV header
      _sampleDataSize   = shizzle.asset.size - 44;
      _cachedSampleData = shizzle.asset.location + 44;
      _sampleDataOffset = 0;
   } else {
      // samples over 256kb will be streamed
      file_size = Stream_Read_Chunk(filename, dma_buffer.data, BUFFER_SIZE);
      if (file_size > 0) {
         // wav header has already been cut
         _sampleDataSize   = file_size;
         _cachedSampleData = dma_buffer.data;
         _sampleDataOffset = 0;
      }
   }

   _lastChunkSize = _sampleDataSize % BUFFER_SIZE;
	_totalChunks = (int)(_sampleDataSize / BUFFER_SIZE);
	_totalChunks += _lastChunkSize > 0 ? 1 : 0;

	_currentChunk = 1;
	_numberOfSamples = (_totalChunks == 1 && _lastChunkSize > 0) ? _lastChunkSize : BUFFER_SIZE;

	write_dsp(SB_SET_FREQUENCY);
	write_dsp(256 - 1000000 / 11000);

	// trigger play, the rest does the irq handler
	dsp_playback();
	return 1;
}

void DSP_Stop() {
	if (_playing) {
		_playing = 0;

		outp(MASK_REGISTER, 4 | _sbDMA);

		// Clear cached data reference (don't free - cache owns it)
		_cachedSampleData = NULL;
      _currentChunk = 0; _lastChunkSize =  0;
      _numberOfSamples = 0; _sampleDataOffset = 0;
		_sampleDataSize = 0;

      memset(dma_buffer.data, 0x80, BUFFER_SIZE);
	}
}

void DSP_Dispose() {
	if (!_dspEnabled) return;
	DSP_Stop();
	
	_cachedSampleData = NULL;
   _numberOfSamples = 0;
	_sampleDataOffset = 0;
	_sampleDataSize = 0;

	deinit_irq_dpmi();

	write_dsp(SB_DISABLE);
	reset_dsp(_sbBase);

	Real_Free(dma_buffer.selector);
}

bool DSP_Is_Playing() {
	return _playing;
}

void DSP_Set_Volume(int percent) {
    unsigned char volume;
    
    if (percent > 100) percent = 100;
    if (percent < 0) percent = 0;
    
    // convert % to levels 1 - 15
    volume = (percent * 15) / 100;
    
    dsp_set_master_volume(volume);
}

void DSP_Set_Distortion(int percent) {
   _distortionLevel = percent;
}

// internal

void __interrupt __far dsp_irq_handler() {
	static int safe_exit = 0;
	static int irq_count = 0;

	inp(_sbBase + SB_READ_STATUS);
	inp(_sbBase + SB_READ);

	_disable();
	if (++safe_exit > 1000) {
		DSP_Stop(); safe_exit = 0;
		_chain_intr(old_irq);
		return;
	}

	outp(0x20, 0x20); if (_sbIRQ > 7) outp(0xA0, 0x20);

	if (_playing) {
      _currentChunk++;
      if (_currentChunk < _totalChunks) {
         _numberOfSamples = (_currentChunk == _totalChunks && _lastChunkSize > 0)
                           ? _lastChunkSize : BUFFER_SIZE;
         _sampleDataOffset += _numberOfSamples;
         dsp_playback();
      } else {
         DSP_Stop();
      }
   }

	_enable();
}

static int dsp_detect() {
	word n; char* BLASTER;
	word dma[7] = { 0x210, 0x220, 0x230, 0x240, 0x250, 0x260, 0x280 };
	_sbBase = 0;

	for (n = 0; n < 7; n++) { if ((_sbBase = reset_dsp(dma[n]))) break; }

	BLASTER = getenv("BLASTER");
	if (!BLASTER) { Log_Info("DSP: no BLASTER env defined"); return 0; }

	_sbDMA = 1; // default
	_sbIRQ = 7; // default

	// parse BLASTER variable
	for (n = 0; n < strlen(BLASTER); n++) {
		if ((BLASTER[n] | 32) == 'd') _sbDMA = BLASTER[n + 1] - '0';
		if ((BLASTER[n] | 32) == 'i') {
			_sbIRQ = BLASTER[n + 1] - '0';
			if (BLASTER[n + 2] != ' ' && BLASTER[n + 2] != 0) {
				_sbIRQ = _sbIRQ * 10 + BLASTER[n + 2] - '0';
			}
		}
	}

	Log_Info("DSP: base: 0x%02X, irq: %d, dma: %d", _sbBase, _sbIRQ, _sbDMA);
	return _sbBase != 0;
}

static void dsp_init() {
	dma_buffer.data = 0;
	dma_buffer.size = BUFFER_SIZE;

	if (!Real_Malloc(&dma_buffer))
		return;

	init_irq_dpmi();
	reset_dsp(_sbBase);
	write_dsp(SB_ENABLE);
}

static word reset_dsp(word port) {
	byte status = 0, ready = 0, n;

	outp(port + SB_RESET, 1); delay(5);
	outp(port + SB_RESET, 0); delay(5);

	for (n = 0; n < 10; n++) {
		status = inp(port + SB_READ_STATUS);
		ready  = inp(port + SB_READ);
		if (status != 0xFF && ready == 0xAA) return port;
	}

	return 0;
}

static void write_dsp(byte command) {
	while ((inp(_sbBase + SB_WRITE) & 0x80) == 0x80);
	outp(_sbBase + SB_WRITE, command);
}

static int is_dsp_busy() {
	// write status - bit 7 is set when busy
	return (inp(_sbBase + SB_WRITE) & 0x80) ? 1 : 0;
}

static void init_irq_dpmi() {
    union REGS regs;
    struct SREGS sregs;

    // we simply ignore these, just fuck'em
    if (_sbIRQ == 2 || _sbIRQ == 10 || _sbIRQ == 11)
        return;
    
    // dpmi 0204h - get protected mode interrupt
    memset(&regs, 0, sizeof(regs));
    regs.w.ax = 0x0204;
    regs.h.bl = _sbIRQ + 8;  // hardware IRQ => interrupt number
    int386(0x31, &regs, &regs);
    
    // save old handler (CX:EDX)
    old_irq = (void(__interrupt __far*)())MK_FP(regs.w.cx, regs.x.edx);
    
    // dpmi 0205h - set protected mode interrupt
    segread(&sregs);
    regs.w.ax = 0x0205;
    regs.h.bl = _sbIRQ + 8;
    regs.w.cx = sregs.cs;
    regs.x.edx = (ulong)dsp_irq_handler;
    int386(0x31, &regs, &regs);
    
    // enable IRQ at PIC
    outp(0x21, inp(0x21) & ~(1 << _sbIRQ));	 
}

static void deinit_irq_dpmi() {
    union REGS regs;

    // we simply ignore these, just fuck'em
    if (_sbIRQ == 2 || _sbIRQ == 10 || _sbIRQ == 11)
        return;
    
    // disable IRQ at PIC first
    outp(0x21, inp(0x21) | (1 << _sbIRQ));
    
    // dpmi 0205h - restore old interrupt
    memset(&regs, 0, sizeof(regs));
    regs.w.ax = 0x0205;
    regs.h.bl = _sbIRQ + 8;
    
    // extract segment:offset from old_irq far pointer
    regs.w.cx = FP_SEG(old_irq);
    regs.x.edx = FP_OFF(old_irq);
    
    int386(0x31, &regs, &regs);
}

static void dsp_playback() {
   int i, dist_start, dist_stop;
   if (_longPlay) {
      _sampleDataSize = Stream_Read_Chunk(_currentFilename, dma_buffer.data, BUFFER_SIZE);
   } else {
   	memset(dma_buffer.data, 0x80, BUFFER_SIZE);
   	memcpy(dma_buffer.data, _cachedSampleData + _sampleDataOffset, _numberOfSamples);
   }

	if (_numberOfSamples <= 0 || _sampleDataSize <= 0) {
		_playing = 0; return;
	}

   if (_distortionLevel > 0) {
      for (i = 0; i < _numberOfSamples; i++) {
         if (rand() % 100 < _distortionLevel) dma_buffer.data[i] ^= rand() & 0xFF;
      }
      if(_distortionLevel < 50) {
         dist_start = rand() % (_numberOfSamples - 12000);
         dist_stop = dist_start + (rand() % 5500) + 5500;
         for (i = dist_start; i < dist_stop; i++) {
            if (rand() % 50 > _distortionLevel) dma_buffer.data[i] ^= rand() & 0xFF;
         }         
      }
   }

	/* program DMA controller */
	outp(MASK_REGISTER, 4 | _sbDMA);
	outp(MSB_LSB, 0);
	outp(MODE_REGISTER, 0x48 | _sbDMA);

	outp(_sbDMA << 1, dma_buffer.offset & 0xFF);
	outp(_sbDMA << 1, dma_buffer.offset >> 8);

	switch (_sbDMA) {
	case 0:
		outp(DMA_CHANNEL_0, dma_buffer.page);
		break;
	case 1:
		outp(DMA_CHANNEL_1, dma_buffer.page);
		break;
	case 3:
		outp(DMA_CHANNEL_3, dma_buffer.page);
		break;
	};

	outp((_sbDMA << 1) + 1, (_numberOfSamples - 1) & 0xFF);
	outp((_sbDMA << 1) + 1, (_numberOfSamples - 1) >> 8);
	outp(MASK_REGISTER, _sbDMA);

	write_dsp(SB_SINGLE_CYCLE);
	write_dsp((_numberOfSamples - 1) & 0xFF);
	write_dsp((_numberOfSamples - 1) >> 8);

	inp(_sbBase + SB_READ_STATUS);
	_playing = 1;
}

static void dsp_set_master_volume(byte volume) {
    // volume: 0-15 (0 = silent, 15 = max)
    byte vol_byte;
    
    if (volume > 15) volume = 15;
    
    // left & right same volume
    vol_byte = (volume << 4) | volume;
    
    // master volume
    outp(_sbBase + 0x4, 0x22);
    outp(_sbBase + 0x5, vol_byte);
    
    // voice Volume (DSP)
    outp(_sbBase + 0x4, 0x04);
    outp(_sbBase + 0x5, vol_byte);
}
