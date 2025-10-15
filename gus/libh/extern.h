#ifndef __EXTERN_H__
#define __EXTERN_H__

/***************************************************************************
*	NAME:  EXTERN.H
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

/*******************************************************************
 * This file contains things specific to an application's use of the
 * UltraSound card.
 *******************************************************************/

#define DMA_AUTO_INIT		0x01		/* Used by app to specify autoinit */

#define DMA_16		0x40
#define DMA_8		0x00

#define DMA_CVT_2	0x80
#define DMA_NO_CVT	0x00

#define USE_ROLLOVER	0x01

typedef struct {
	unsigned int base_port;
	unsigned int dram_dma_chan;
	unsigned int adc_dma_chan;
	unsigned int gf1_irq_num;
	unsigned int midi_irq_num;
	unsigned int dram_size;
	char ultrapath[64];
} ULTRA_CFG;

#endif
