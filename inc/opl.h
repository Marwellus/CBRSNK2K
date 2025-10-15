#ifndef OPL_H
#define OPL_H
#include "inc/types.h"

#ifdef __cplusplus
extern "C" {
#endif

// OPL3 base addresses (typically 0x388)
#define OPL3_BASE 0x388
#define OPL3_REGISTER (OPL3_BASE)
#define OPL3_DATA (OPL3_BASE + 1)
#define OPL3_STATUS (OPL3_BASE)

// OPL3 register offsets
#define OPL3_TEST_REG 0x01
#define OPL3_TIMER1 0x02
#define OPL3_TIMER2 0x03
#define OPL3_TIMER_CTRL 0x04
#define OPL3_MODE_REG 0x05

// voice registers (for simple sine wave)
#define VOICE_KSL_TL 0x40			      // key scale level / Total level
#define VOICE_AR_DR 0x60			      // attack rate / Decay rate
#define VOICE_SL_RR 0x80			      // sustain level / Release rate
#define VOICE_FNUM_LOW 0xA0			   // frequency number low byte
#define VOICE_KON_BLOCK_FNUM_HIGH 0xB0 // Key on / Block / F-Num high

extern bool Opl3_Initialized;

bool Opl3_Init();
void Opl3_Shutdown();
void Opl3_Test();

void Opl3_Note_On(int freq, int channel);
void Opl3_Note_Off(int channel);

#ifdef __cplusplus
}
#endif

#endif
