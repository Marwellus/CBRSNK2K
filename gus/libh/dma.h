#ifndef DMA_INC
#define DMA_INC
/***************************************************************************
*	NAME:  DMA.H
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

/* DMA Controler #1 (8-bit controller) */
#define DMA1_STAT	0x08		/* read status register */
#define DMA1_WCMD	0x08		/* write command register */
#define DMA1_WREQ	0x09		/* write request register */
#define DMA1_SNGL	0x0A		/* write single bit register */
#define DMA1_MODE	0x0B		/* write mode register */
#define DMA1_CLRFF	0x0C		/* clear byte ptr flip/flop */
#define DMA1_MCLR	0x0D		/* master clear register */
#define DMA1_CLRM	0x0E		/* clear mask register */
#define DMA1_WRTALL	0x0F		/* write all mask register */

/* DMA Controler #2 (16-bit controller) */
#define DMA2_STAT	0xD0		/* read status register */
#define DMA2_WCMD	0xD0		/* write command register */
#define DMA2_WREQ	0xD2		/* write request register */
#define DMA2_SNGL	0xD4		/* write single bit register */
#define DMA2_MODE	0xD6		/* write mode register */
#define DMA2_CLRFF	0xD8		/* clear byte ptr flip/flop */
#define DMA2_MCLR	0xDA		/* master clear register */
#define DMA2_CLRM	0xDC		/* clear mask register */
#define DMA2_WRTALL	0xDE		/* write all mask register */

#define DMA0_ADDR	0x00		/* chan 0 base adddress */
#define DMA0_CNT	0x01		/* chan 0 base count */
#define DMA1_ADDR	0x02		/* chan 1 base adddress */
#define DMA1_CNT	0x03		/* chan 1 base count */
#define DMA2_ADDR	0x04		/* chan 2 base adddress */
#define DMA2_CNT	0x05		/* chan 2 base count */
#define DMA3_ADDR	0x06		/* chan 3 base adddress */
#define DMA3_CNT	0x07		/* chan 3 base count */
#define DMA4_ADDR	0xC0		/* chan 4 base adddress */
#define DMA4_CNT	0xC2		/* chan 4 base count */
#define DMA5_ADDR	0xC4		/* chan 5 base adddress */
#define DMA5_CNT	0xC6		/* chan 5 base count */
#define DMA6_ADDR	0xC8		/* chan 6 base adddress */
#define DMA6_CNT	0xCA		/* chan 6 base count */
#define DMA7_ADDR	0xCC		/* chan 7 base adddress */
#define DMA7_CNT	0xCE		/* chan 7 base count */

#define DMA0_PAGE	0x87		/* chan 0 page register (refresh)*/
#define DMA1_PAGE	0x83		/* chan 1 page register */
#define DMA2_PAGE	0x81		/* chan 2 page register */
#define DMA3_PAGE	0x82		/* chan 3 page register */
#define DMA4_PAGE	0x8F		/* chan 4 page register (unuseable)*/
#define DMA5_PAGE	0x8B		/* chan 5 page register */
#define DMA6_PAGE	0x89		/* chan 6 page register */
#define DMA7_PAGE	0x8A		/* chan 7 page register */

/**********************************************************/

#define MAX_DMA		7

#define DMA_DECREMENT	0x20	/* mask to make DMA hardware go backwards */

/* Bits for dma flags location */
#define DMA_USED	0x0001
#define DMA_PENDING 0x0002
#define TWO_FLAG	0x0004
#define REV_FLAG	0x0008
#define CALIB_COUNT	0x0010

typedef struct {
	unsigned int	flags;
	unsigned int	latch;
	unsigned int	dma_disable;	/* bits to disable dma channel */
	unsigned int	dma_enable;		/* bits to enable dma channel */
	unsigned int	page;			/* page port location */
	unsigned int	addr;			/* addr port location */
	unsigned int	count;			/* count port location */
	unsigned int	single;			/* single mode port location */
	unsigned int	mode;			/* mode port location */
	unsigned int	clear_ff;		/* clear flip-flop port location */
	unsigned int	write;			/* bits for write transfer */
	unsigned int	read;			/* bits for read transfer */
	unsigned char	cur_mode;		/* current mode */
	unsigned int	cur_page;		/* current page of transfer */
	unsigned int	cur_addr;		/* current transfer address */
	unsigned int	amnt_sent;		/* current amnt sent */
	unsigned int	cur_size;		/* current size of transfer */
	unsigned int	nxt_page;		/* next page */
	unsigned int	nxt_addr;		/* next address */
	unsigned int	nxt_size;		/* size of next buffer */
	unsigned char	cur_control;
} DMA_ENTRY;

#endif
