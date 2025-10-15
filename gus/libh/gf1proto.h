#ifndef __GF1PROTO_H__
#define __GF1PROTO_H__

/***************************************************************************
*	NAME:  GF1PROTO.H
**	COPYRIGHT:
**	"Copyright (c) 1992, by FORTE
**
**       "This software is furnished under a license and may be used,
**       copied, or disclosed only in accordance with the terms of such
**       license and with the inclusion of the above copyright notice.
**       This software or any other copies thereof may not be provided or
**       otherwise made available to any other person. No title to and
**       ownership of the software is hereby transfered."
****************************************************************************
*  CREATION DATE: 11/18/92
*--------------------------------------------------------------------------*
*     VERSION	DATE	   NAME		DESCRIPTION
*>	1.0	11/18/92		Original
*>	1.1	03/01/93		Added a few protos
***************************************************************************/

#include "forte.h"
#include "extern.h"

#ifdef MSOFTC
#define _Cdecl  _cdecl
#else
#if __STDC__
#undef _Cdecl
#define _Cdecl
#else
#define _Cdecl  cdecl
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif
/**********************************************************************
 *
 * General control prototypes 
 *
 *********************************************************************/
unsigned char _Cdecl UltraCalcRate(unsigned int, unsigned int, unsigned long);
int _Cdecl UltraClose(void);
int _Cdecl UltraOpen(ULTRA_CFG far *, int);
unsigned char _Cdecl UltraPeekData(unsigned int, unsigned long);
unsigned long _Cdecl UltraPeekLong(unsigned long);
int _Cdecl UltraPing(unsigned int);
void _Cdecl UltraPokeData(unsigned int, unsigned long, unsigned char);
int _Cdecl UltraProbe(unsigned int);
int _Cdecl UltraReset(int);
int _Cdecl UltraSizeDram(void);
void _Cdecl UltraVersion(int *, int *);
/**********************************************************************
 *
 * Recording control prototypes 
 *
 *********************************************************************/
unsigned int _Cdecl GetRecordDmaPos(int);
int _Cdecl UltraGoRecord(unsigned char);
int _Cdecl UltraPrimeRecord(void far *, unsigned int, int);
int _Cdecl UltraRecordData(void far *, unsigned char, unsigned int, int, int);
int _Cdecl UltraRecordDmaBusy(void);
unsigned int _Cdecl UltraReadRecordPosition(void);
void _Cdecl UltraSetRecordFrequency(unsigned long);
void _Cdecl UltraStartRecordDma(unsigned char);
void _Cdecl UltraWaitRecordDma(void);
/**********************************************************************
 *
 * Volume control prototypes 
 *
 *********************************************************************/
void _Cdecl UltraRampLinearVolume(int, unsigned int, unsigned int, unsigned long, unsigned char);
void _Cdecl UltraRampVolume(int, unsigned int, unsigned int, unsigned char, unsigned char);
unsigned int _Cdecl UltraReadVolume(int);
void _Cdecl UltraSetLinearVolume(int, int);
void _Cdecl UltraSetVolume(int, unsigned int);
void _Cdecl UltraStopVolume(int);
void _Cdecl UltraVectorVolume(int, unsigned int, unsigned char, unsigned char);
void _Cdecl UltraVectorLinearVolume(int, unsigned int, unsigned char, unsigned char);
int _Cdecl UltraVolumeStopped(int);
/**********************************************************************
 *
 * Voice control prototypes 
 *
 *********************************************************************/
int _Cdecl UltraAllocVoice(int, int far *);
void _Cdecl UltraClearVoices(void);
void _Cdecl UltraFreeVoice(int);
void _Cdecl UltraGoVoice(int, unsigned char);
unsigned char _Cdecl UltraPrimeVoice(int, unsigned long, unsigned long, unsigned long, unsigned char);
unsigned long _Cdecl UltraReadVoice(int);
void _Cdecl UltraSetLoopMode(int, unsigned char);
void _Cdecl UltraSetFrequency(int, unsigned long);
void _Cdecl UltraSetBalance(int, int);
void _Cdecl UltraSetVoice(int, unsigned long);
void _Cdecl UltraSetVoiceEnd(int, unsigned long);
void _Cdecl UltraStartVoice(int, unsigned long, unsigned long, unsigned long, unsigned char);
void _Cdecl UltraStopVoice(int);
void _Cdecl UltraVoiceOff(int, int);
void _Cdecl UltraVoiceOn(int, unsigned long, unsigned long, unsigned long, unsigned char, unsigned long);
int _Cdecl UltraVoiceStopped(int);
/**********************************************************************
 *
 * Mixer  control prototypes 
 *
 *********************************************************************/
void _Cdecl UltraDisableLineIn(void);
void _Cdecl UltraDisableMicIn(void);
void _Cdecl UltraDisableOutput(void);
void _Cdecl UltraEnableLineIn(void);
void _Cdecl UltraEnableMicIn(void);
void _Cdecl UltraEnableOutput(void);
int _Cdecl UltraGetLineIn(void);
int _Cdecl UltraGetMicIn(void);
int _Cdecl UltraGetOutput(void);
/**********************************************************************
 *
 * Memory control prototypes 
 *
 *********************************************************************/
unsigned long _Cdecl UltraMaxAlloc(void);
int _Cdecl UltraMemAlloc(unsigned int, unsigned int far *);
int _Cdecl UltraMemFree(unsigned int, unsigned int);
int _Cdecl UltraMemInit(void);
/**********************************************************************
 *
 * MIDI Control prototypes 
 *
 *********************************************************************/
unsigned char _Cdecl UltraMidiRecv(void);
void _Cdecl UltraMidiDisableRecv(void);
void _Cdecl UltraMidiDisableXmit(void);
void _Cdecl UltraMidiEnableRecv(void);
void _Cdecl UltraMidiEnableXmit(void);
void _Cdecl UltraMidiXmit(unsigned char);
unsigned char _Cdecl UltraMidiStatus(void);
void _Cdecl UltraMidiReset(void);
/**********************************************************************
 *
 * Timer Control prototypes 
 *
 *********************************************************************/
void _Cdecl UltraStopTimer(int);
void _Cdecl UltraStartTimer(int, unsigned char);
int _Cdecl UltraTimerStopped(int);
/**********************************************************************
 *
 * Dram Dma prototypes 
 *
 *********************************************************************/
int _Cdecl UltraDownload(void far *, unsigned char, unsigned int, unsigned int, int);
int _Cdecl UltraDramDmaBusy(void);
int _Cdecl UltraUpload(void far *, unsigned char, unsigned int, unsigned int, int);
void _Cdecl UltraStartDramDma(unsigned char);
void _Cdecl UltraWaitDramDma(void);
void _Cdecl UltraStopRecordDma(void);
void _Cdecl UltraStopPlayDma(void);
/**********************************************************************
 *
 * Joystick prototypes 
 *
 *********************************************************************/
void _Cdecl UltraTrimJoystick(unsigned char);

/**********************************************************************
 *
 * CallBack prototypes 
 *
 *********************************************************************/
#ifdef NEVER
void (_Cdecl *UltraDramTcHandler(void (*)()))();
void (_Cdecl *UltraMidiXmitHandler(void (*)()))();
void (_Cdecl *UltraMidiRecvHandler(void (*)()))();
void (_Cdecl *UltraTimer1Handler(void (*)()))();
void (_Cdecl *UltraTimer2Handler(void (*)()))();
void (_Cdecl *UltraWaveHandler(void (*)()))();
void (_Cdecl *UltraVolumeHandler(void (*)()))();
void (_Cdecl *UltraRecordHandler(void (*)()))();
void (_Cdecl *UltraAuxHandler(void (*)()))();
#endif

PFV UltraDramTcHandler(PFV);
PFV UltraMidiXmitHandler(PFV);
PFV UltraMidiRecvHandler(PFV);
PFV UltraTimer1Handler(PFV);
PFV UltraTimer2Handler(PFV);
PFV UltraWaveHandler(PFV);
PFV UltraVolumeHandler(PFV);
PFV UltraRecordHandler(PFV);
PFV UltraAuxHandler(PFV);

#ifdef __cplusplus
}
#endif
#endif

