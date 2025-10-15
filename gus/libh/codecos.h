#ifndef __CODECOS_H__
#define __CODECOS_H__

/***************************************************************************
*	NAME:  CODECOS.H
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
*  CREATION DATE: 08/01/92
*--------------------------------------------------------------------------*
*     VERSION	DATE	   NAME		DESCRIPTION
*>	1.0	08/01/92		Original
***************************************************************************/

typedef struct {
	unsigned int flags;
	unsigned int type;
	unsigned int base_port;
	unsigned int addr;
	unsigned int data;
	unsigned int status;
	unsigned int pio;
	unsigned int play_chan;
	unsigned int rec_chan;
	unsigned int irq_num;
	unsigned int setup;
	PVI		old_codec_vec;
	PFV		playback_func;
	PFV		capture_func;
	PFV		timer_func;
} ULTRA16_DATA;

#endif
