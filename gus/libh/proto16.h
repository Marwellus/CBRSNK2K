#ifndef __PROTO16_H__
#define __PROTO16_H__

/***************************************************************************
*	NAME:  PROTO16.H
**	COPYRIGHT:
**	"Copyright (c) 1994, by FORTE
**
**       "This software is furnished under a license and may be used,
**       copied, or disclosed only in accordance with the terms of such
**       license and with the inclusion of the above copyright notice.
**       This software or any other copies thereof may not be provided or
**       otherwise made available to any other person. No title to and
**       ownership of the software is hereby transfered."
****************************************************************************
*  CREATION DATE: 01/01/94
*--------------------------------------------------------------------------*
*     VERSION	DATE	   NAME		DESCRIPTION
*>	1.0	01/01/94		Original
***************************************************************************/

#include "extern16.h"

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


unsigned char _Cdecl Left_CD_Input_Level(unsigned char);
unsigned char _Cdecl Left_CD_Input_Mute(unsigned char);
unsigned char _Cdecl Left_GF1_Input_Level(unsigned char);
unsigned char _Cdecl Left_GF1_Input_Mute(unsigned char);
unsigned char _Cdecl Left_Input_Gain_Select(unsigned char);
unsigned char _Cdecl Left_Input_Source(unsigned char);
unsigned char _Cdecl Left_Line_Input_Level(unsigned char);
unsigned char _Cdecl Left_Line_Input_Mute(unsigned char);
unsigned char _Cdecl Left_Mic_Gain_Enable(unsigned char);
unsigned char _Cdecl Left_Output_Attn_Select(unsigned char);
unsigned char _Cdecl Left_Output_Mute(unsigned char);
void (_Cdecl *Ultra16CaptureHandler(void (*)()))();
int _Cdecl Ultra16Close(void);
void _Cdecl Ultra16DisableIrqs(void);
void _Cdecl Ultra16EnableIrqs(void);
int _Cdecl Ultra16GoPlay(unsigned char);
int _Cdecl Ultra16GoRecord(unsigned char);
int _Cdecl Ultra16Open(int, ULTRA16_CFG *);
int _Cdecl Ultra16PlayData(void far *, unsigned char, unsigned int, int, int);
int _Cdecl Ultra16PlayDmaBusy(void);
unsigned int _Cdecl Ultra16PlayFormat(int, int, int);
void (_Cdecl *Ultra16PlaybackHandler(void (*)()))();
int _Cdecl Ultra16PrimePlay(void far *, unsigned int, int);
int _Cdecl Ultra16PrimeRecord(void far *, unsigned int, int);
int _Cdecl Ultra16Probe(int, ULTRA16_CFG *);
void _Cdecl Ultra16ProgPlayCnt(unsigned int);
void _Cdecl Ultra16ProgRecCnt(unsigned int);
unsigned int _Cdecl Ultra16ReadPlayPosition(void);
unsigned int _Cdecl Ultra16ReadRecordPosition(void);
unsigned int _Cdecl Ultra16RecFormat(int, int, int);
int _Cdecl Ultra16RecordDmaBusy(void);
int _Cdecl Ultra16RecordData(void far *, unsigned char, unsigned int, int, int);
unsigned char _Cdecl Ultra16Revision(void);
unsigned int _Cdecl Ultra16SetFreq(unsigned int);
void _Cdecl Ultra16StartPlayDma(unsigned char);
void _Cdecl Ultra16StartRecordDma(unsigned char);
void _Cdecl Ultra16StartTimer(void);
void _Cdecl Ultra16StopPlayDma(void);
void _Cdecl Ultra16StopRecordDma(void);
void _Cdecl Ultra16StopTimer(void);
void _Cdecl Ultra16SetTimer(unsigned int);
void (_Cdecl *Ultra16TimerHandler(void (*)()))();
void _Cdecl Ultra16Version(unsigned int *, unsigned int *);
void _Cdecl Ultra16WaitPlayDma(void);
void _Cdecl Ultra16WaitRecordDma(void);
void _Cdecl Ultra16Xparent(void);
void _Cdecl UltraMaxHandler(void);
void _Cdecl AutoCalibrate(void);
void _Cdecl ReSet16IrqHandlers(int);
void _Cdecl Reset16Irqs(int);
void _Cdecl Start_Play(void far *, unsigned int);
int _Cdecl Start_Recording(void far *, unsigned int);
void _Cdecl Set16IrqHandlers(int);
void _Cdecl Set16Irqs(int);
unsigned char _Cdecl Right_CD_Input_Level(unsigned char);
unsigned char _Cdecl Right_CD_Input_Mute(unsigned char);
unsigned char _Cdecl Right_GF1_Input_Level(unsigned char);
unsigned char _Cdecl Right_GF1_Input_Mute(unsigned char);
unsigned char _Cdecl Right_Input_Gain_Select(unsigned char);
unsigned char _Cdecl Right_Input_Source(unsigned char);
unsigned char _Cdecl Right_Line_Input_Level(unsigned char);
unsigned char _Cdecl Right_Line_Input_Mute(unsigned char);
unsigned char _Cdecl Right_Mic_Gain_Enable(unsigned char);
unsigned char _Cdecl Right_Output_Attn_Select(unsigned char);
unsigned char _Cdecl Right_Output_Mute(unsigned char);
unsigned char _Cdecl Mono_Input_Mute(unsigned char);
unsigned char _Cdecl Mono_Input_Level(unsigned char);
unsigned char _Cdecl Mono_Output_Mute(unsigned char);

#endif

