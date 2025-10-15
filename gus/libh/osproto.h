#ifndef __OSPROTO_H__
#define __OSPROTO_H__

/***************************************************************************
*	NAME:  OSPROTO.H
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
*  CREATION DATE: 06/01/93
*--------------------------------------------------------------------------*
*     VERSION	DATE	   NAME		DESCRIPTION
*>	1.0	06/01/93		Original
***************************************************************************/

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

#include "dma.h"

#ifdef __cplusplus
extern "C" {
#endif

int _Cdecl UltraDmaDram_64K(void far *,unsigned int,unsigned long,unsigned char,int);
void _Cdecl UltraDmaNext(DMA_ENTRY *, int);
unsigned long _Cdecl UltraPeekLong(unsigned long);
void _Cdecl UltraPokeLong(unsigned long, unsigned long);
void _Cdecl UltraSetInterface(int,int,int,int);
void _Cdecl UltraStartDramDma(unsigned char);
int _Cdecl PrimeDma(void far *, int, unsigned int, unsigned int);
unsigned long _Cdecl convert_to_16bit(unsigned long);
void _Cdecl SetIrqs(int,int);
void _Cdecl ResetIrqs(int,int);
void _Cdecl SetIrqHandlers(int,int);
void _Cdecl ReSetIrqHandlers(int,int);
void _Cdecl default_func(void);
void _Cdecl handle_dma_tc(void);
void _Cdecl gf1_delay(void);
#ifdef METAWARE
void _Cdecl gf1_setvect(int, PVI);
#else
void _Cdecl gf1_setvect(int, void (far interrupt *)());
#endif
unsigned long _Cdecl make_physical_address(unsigned int, unsigned int, unsigned char);

#ifdef __cplusplus
extern "C" {
#endif
#endif
