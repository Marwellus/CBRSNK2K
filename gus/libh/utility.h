#ifndef __UTILITY_H__
#define __UTILITY_H__

/***************************************************************************
*	NAME:  UTILITY.H
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
*  CREATION DATE: 04/01/94
*--------------------------------------------------------------------------*
*     VERSION	DATE	   NAME		DESCRIPTION
*>	1.0	04/01/94		Original
***************************************************************************/

#include "extern.h"
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


#ifdef __cplusplus
extern "C" {
#endif

int _Cdecl UltraGetCfg(ULTRA_CFG *);
void _Cdecl OutChar(int, unsigned char);
unsigned char _Cdecl data_ready(int);
unsigned char _Cdecl xmit_busy(int);
unsigned char _Cdecl InChar(int);
void _Cdecl init_port(int);
int _Cdecl fast_kbhit(void);
void _Cdecl read_joystick(unsigned int *, unsigned int *, unsigned int *, unsigned int *, unsigned int *);
int _Cdecl GetUltra16Cfg(ULTRA_CFG *, ULTRA16_CFG *);
void far * _Cdecl MallocAlignedBuff(unsigned int);

#ifdef __cplusplus
}
#endif

#endif

