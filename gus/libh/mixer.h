#ifndef __MIXER_H__
#define __MIXER_H__

/***************************************************************************
*	NAME:  MIXER.H
**	COPYRIGHT:
**	"Copyright (c) 1993, by FORTE
**
**       "This software is furnished under a license and may be used,
**       copied, or disclosed only in accordance with the terms of such
**       license and with the inclusion of the above copyright notice.
**       This software or any other copies thereof may not be provided or
**       otherwise made available to any other person. No title to and
**       ownership of the software is hereby transfered."
****************************************************************************
*  CREATION DATE: 10/04/93
*--------------------------------------------------------------------------*
*     VERSION	DATE	   NAME		DESCRIPTION
*>	1.0	10/04/93		Original
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

#ifdef __cplusplus
extern "C" {
#endif

int _Cdecl UltraMixProbe(unsigned int);
void _Cdecl UltraMixAttn(int,int,int,int);
void _Cdecl UltraMixXparent(int);
void _Cdecl UltraMixMute(int,int);

#ifdef __cplusplus
}
#endif

#define MIX_SEL_PORT  0x506		/* Offset from base port */
#define MIX_DATA_PORT 0x106		/* Offset from base port */

#define MIX_CHAN_0	0
#define MIX_CHAN_1	1
#define MIX_CHAN_2	2
#define MIX_CHAN_3	3
#define MIX_CHAN_4	4
#define MIX_CHAN_5	5

#define MIX_LEFT 0
#define MIX_RIGHT 1

/* Mixer channels used on GUS */
/* Channel #4 is NOT used */
#define MIX_MIKE_IN	MIX_CHAN_0
#define MIX_LINE_IN MIX_CHAN_1
#define MIX_CD_IN   MIX_CHAN_2
#define MIX_GF1_OUT MIX_CHAN_3
#define MIX_MASTER  MIX_CHAN_5

#define MIX_CTRL_LEFT  0x00
#define MIX_CTRL_RIGHT 0x01
#define MIX_ATTN_LEFT  0x02
#define MIX_ATTN_RIGHT 0x03
#define MIX_PAN        0x04


typedef struct {
	unsigned int mixer_addr;
	unsigned int mixer_data;
	unsigned int mute[6];
} ULTRA_MIXER;

#endif

