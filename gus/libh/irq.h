#ifndef __IRQ_H__
#define __IRQ_H__

/***************************************************************************
*	NAME:  IRQ.H
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

#define MAX_IRQ		16

#define IRQ_UNAVAIL		0x0000
#define IRQ_AVAIL		0x0001
#define IRQ_USED		0x0002

#define OCR1	0x20			/* 8259-1 Operation control register */
#define IMR1	0x21			/* 8259-1 Mask register */

#define OCR2	0xA0			/* 8259-2 Operation control register */
#define IMR2	0xA1			/* 8259-2 Mask register */

typedef struct {
	unsigned char	latch;
	unsigned char	mask;
	unsigned char	spec_eoi;
	unsigned char	ocr;
	unsigned char	imr;
} IRQ_ENTRY;

#endif

