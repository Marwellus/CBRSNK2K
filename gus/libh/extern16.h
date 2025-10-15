#ifndef __EXTERN16_H__
#define __EXTERN16_H__
/***************************************************************************
*	NAME:  EXTERN16.H
**	COPYRIGHT:
**	"Copyright (c) 1991,1992,1993 by FORTE
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

/* The type field is defined as:
	0 = 16 bit recording is on a daughter card
	1 = 16 bit recording is on board (UltraMax)
*/

#define COMPRESS_NONE		0
#define COMPRESS_ADPCM		1
#define COMPRESS_ULAW		2
#define COMPRESS_ALAW		3

typedef struct {
	unsigned int base_port;
	unsigned int cdrom_base;
	unsigned int play_dma;
	unsigned int rec_dma;
	unsigned int irq_num;
	unsigned int type;
} ULTRA16_CFG;

#endif
