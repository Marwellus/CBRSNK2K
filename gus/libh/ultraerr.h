#ifndef __ULTRAERR_H__
#define __ULTRAERR_H__

/***************************************************************************
*	NAME:  ULTRAERR.H
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
***************************************************************************/

/* 0 WIll not be used for ANY error. AN OK return will use ULTRA_OK ... */
#define ULTRA_OK			1
#define BAD_NUM_OF_VOICES	2
#define NO_MEMORY			3
#define CORRUPT_MEM			4
#define NO_ULTRA			5
#define DMA_BUSY			6
#define BAD_DMA_ADDR		7	/* auto init across page boundaries */
#define VOICE_OUT_OF_RANGE	8	/* allocate a voice past # active */
#define VOICE_NOT_FREE		9	/* voice has already been allocated */
#define NO_FREE_VOICES		10	/* not any voices free */
#define NO_MIXER			11  /* ICS Mixer 2101 not present */

#endif

