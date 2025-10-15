#ifndef __GF1OS_H__
#define __GF1OS_H__

/***************************************************************************
*	NAME:  GF1OS.H
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

#define NEXT_OFFSET		0L
#define PREV_OFFSET		4L
#define SIZE_OFFSET		8L
#define MEM_HEADER_SIZE SIZE_OFFSET+4L

#define ULTRA_PRESENT	0x0001		/* show this card is present */
#define	DRAM_DMA_BUSY	0x0002		/* show this channels dram dma is busy */
#define ADC_DMA_BUSY	0x0004		/* show we are busy recording */
#define DRAM_DMA_NOWAIT	0x0008		/* show we didn't wait last time */
#define ADC_DMA_NOWAIT	0x0010		/* show we didn't wait last time */

#define READ_DMA		1
#define WRITE_DMA		2
#define INDEF_READ		3	/* auto init record */
#define INDEF_WRITE		4	/* auto init record */

typedef struct {
	unsigned int flags;
	unsigned int base_port;
	unsigned int dram_dma_chan;
	unsigned int adc_dma_chan;
	unsigned int gf1_irq_num;
	unsigned int midi_irq_num;
	PVI		old_gf1_vec;
	PVI		old_midi_vec;
	PFV		midi_xmit_func;
	PFV		midi_recv_func;
	PFV		timer1_func;
	PFV		timer2_func;
	PFV		wavetable_func;
	PFV		volume_func;
	PFV		dram_dma_tc_func;
	PFV		record_dma_tc_func;
	PFV		aux_irq_func;
	unsigned char	mix_image;
	unsigned char	voices;
	unsigned char	image_midi;
	unsigned long	used_voices;
	unsigned long 	reserved_dram;
	unsigned long 	free_mem;
	unsigned char	timer_ctrl;
	unsigned char	timer_mask;
	int		midi_data;
	int		midi_control;
	int		voice_select;
	int		reg_select;
	int		data_low;
	int		data_hi;
	int		irq_status;
	int		dram_data;
	int		mix_control;
	int		irq_control;
	int		timer_control;
	int		timer_data;
	int		ultra_errno;
	int		gf1_sema4;
	int		irq_pending;
} ULTRA_DATA;

#endif

