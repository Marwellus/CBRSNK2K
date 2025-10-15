#ifndef __CODEC_H__
#define __CODEC_H__
/***************************************************************************
*	NAME:  CODEC.H
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
*  CREATION DATE: 01/01/94
*--------------------------------------------------------------------------*
*     VERSION	DATE	   NAME		DESCRIPTION
*>	1.0	01/01/94		Original
***************************************************************************/

#define CODEC_BASE		0x32C

#define CODEC_ADDR		0		/* register select */
#define CODEC_DATA		1		/* loc to read/write data */
#define CODEC_STATUS	2
#define CODEC_PIO		3

/************************************************************************/
/* Definitions for CODEC_ADDR register */
/* Bits 0-3. Select an internal register to read/write */
#define LEFT_INPUT			0x00		/* Left input control register */
#define RIGHT_INPUT			0x01		/* RIght input control register */
#define GF1_LEFT_INPUT		0x02		/* Left Aux #1 input control */
#define GF1_RIGHT_INPUT		0x03		/* Right Aux #1 input control */
#define CD_LEFT_INPUT		0x04		/* Left Aux #2 input control */
#define CD_RIGHT_INPUT		0x05		/* Right Aux #2 input control */
#define LEFT_OUTPUT			0x06		/* Left output control */
#define RIGHT_OUTPUT		0x07		/* Right output control */
#define PLAYBK_FORMAT		0x08		/* Clock and data format */
#define IFACE_CTRL			0x09		/* Interface control */
#define PIN_CTRL			0x0a		/* Pin control */
#define TEST_INIT			0x0b		/* Test and initialization */
#define MISC_INFO			0x0c		/* Miscellaneaous information */
#define LOOPBACK			0x0d		/* Digital Mix */
#define PLY_UPR_CNT			0x0e		/* Playback Upper Base Count */
#define PLY_LWR_CNT			0x0f		/* Playback Lower Base Count */
#define ALT_FEATURE_1		0x10		/* alternate #1 feature enable */
#define ALT_FEATURE_2		0x11		/* alternate #2 feature enable */
#define LEFT_LINE_IN		0x12		/* left line input control */
#define RIGHT_LINE_IN		0x13		/* right line input control */
#define TIMER_LOW			0x14		/* timer low byte */
#define TIMER_HIGH			0x15		/* timer high byte */
#define IRQ_STATUS			0x18		/* irq status register */
#define MONO_IO_CTRL		0x1a		/* mono input/output control */
#define REC_FORMAT			0x1c		/* record format */
#define REC_UPR_CNT			0x1e		/* record upper count */
#define REC_LWR_CNT			0x1f		/* record lower count */

/************************************************************************/
/* Definitions for CODEC_ADDR register */
#define CODEC_INIT		0x80		/* CODEC is initializing */
#define CODEC_MCE		0x40		/* Mode change enable */
#define CODEC_TRD		0x20		/* Transfer Request Disable */
/* bits 3-0 are indirect register address (0-15) */
/************************************************************************/

/************************************************************************/
/* Definitions for CODEC_STATUS register */
#define CODEC_CUL		0x80		/* Capture data upper/lower byte */
#define CODEC_CLR		0x40		/* Capture left/right sample */
#define CODDEC_CRDY		0x20		/* Capture data read */
#define CODEC_SOUR		0x10		/* Playback over/under run error */
#define CODEC_PUL		0x08		/* Playback upper/lower byte */
#define CODEC_PLR		0x04		/* Playback left/right sample */
#define CODEC_PRDY		0x02		/* Playback data register read */
#define CODEC_INT		0x01		/* interrupt status */
/************************************************************************/

/************************************************************************/
/* Definitions for Both Left and Right input level register */
#define LINE_SOURCE			0x00		/* Line source selected */
#define GF1_SOURCE			0x40		/* Auxiliary 1 source selected */
#define MIC_SOURCE			0x80		/* Microphone source selected */
#define MUX_OUTPUT_SOURCE	0xC0		/* Post-mixed DAC output selected */
#define MIC_GAIN			0x20		/* microphone gain enable */
/* bits 3-0 are left input gain select (0-15) */
/* bits 3-0 are right input gain select (0-15) */
/************************************************************************/

/************************************************************************/
/* Definitions for Left AUX #1 input level register */
#define MUTE_INPUT			0x80		/* Mute this input's source */

/* bits 3-0 are left AUX #1 (GF1) input atten select (0-15) */
/* bits 3-0 are right AUX #1 (GF1) input atten select (0-15) */
/* bits 3-0 are left AUX #2 (CD) input atten select (0-15) */
/* bits 3-0 are right AUX #2 (CD) input atten select (0-15) */
/************************************************************************/

/************************************************************************/
/* Definitions for Left output level register */
#define MUTE_OUTPUT		0x80		/* Mute this output source */
/* bits 5-0 are left output attenuation select (0-63) */
/* bits 5-0 are right output attenuation select (0-63) */
/************************************************************************/

/************************************************************************/
/* Definitions for clock and data format register */
#define BIT_8_ALAW				0x60	/* 8 bit A-law companded */
#define BIT_16_LINEAR			0x40	/* 16 bit twos complement data */
#define BIT_8_ULAW				0x20	/* 8 bit U-law companded */
#define BIT_8_LINEAR			0x00	/* 8 bit unsigned data */
#define TYPE_STEREO				0x10	/* stero mode */
/* Bits 3-1 define frequency divisor */
#define XTAL1					0x00	/* 24.576 crystal */
#define XTAL2					0x01	/* 16.9344 crystal */
/************************************************************************/

/************************************************************************/
/* Definitions for interface control register */
#define CAPTURE_PIO				0x80	/* Capture PIO enable */
#define PLAYBACK_PIO			0x40	/* Playback PIO enable */
#define AUTOCALIB				0x08	/* auto calibrate */
#define SINGLE_DMA				0x04	/* Use single DMA channel */
#define CAPTURE_ENABLE			0x02	/* Capture enable */
#define PLAYBACK_ENABLE			0x01	/* playback enable */
/************************************************************************/

/************************************************************************/
/* Definitions for Pin control register */
#define IRQ_ENABLE				0x02	/* interrupt enable */
#define XCTL1					0x40	/* external control #1 */
#define XCTL0					0x80	/* external control #0 */
/************************************************************************/

/************************************************************************/
/* Definitions for MISC control register */
#define CODEC_MODE2				0x40	/* MODE 2 */

/************************************************************************/
/* Definitions for Alternate feature 1 register */
#define CODEC_DACZ				0x01	/* zero DAC when under run */
#define TIMER_ENABLE			0x40	/* Codec timer enable */
#define CODEC_OLB				0x80	/* output level bit */
										/* 0=2.0 (-3db), 1=2.8 (0db) */

/************************************************************************/
/* Definitions for irq status (alternate feature status bits) */
#define PLAYBACK_IRQ			0x10
#define CAPTURE_IRQ				0x20
#define TIMER_IRQ				0x40
/************************************************************************/

/************************************************************************/
/* Definitions for Test and init register */
#define CALIB_IN_PROGRESS		0x20	/* auto calibrate in progress */
/************************************************************************/

/************************************************************************/
/* Definitions for Digital mix control register */
/* bits 7-2 are for the digital mix attenuation */
#define DIGITAL_MIX_ENABLE		0x01	/* enable digital mix (monitor) */
/************************************************************************/

/************************************************************************/
/* Definitions for Left input level register */
/************************************************************************/

/************************************************************************/
/* Definitions for Left input level register */
/************************************************************************/

/************************************************************************/
/* Definitions for Left input level register */
/************************************************************************/

/* Image registers of codec registers */

typedef struct {
	unsigned int playfreq;		/* freqency in hertz ... */
	unsigned int recfreq;		/* freqency in hertz ... */
	unsigned char lic;
	unsigned char ric;
	unsigned char la1ic;
	unsigned char ra1ic;
	unsigned char la2ic;
	unsigned char ra2ic;
	unsigned char loc;
	unsigned char roc;
	unsigned char pdfr;
	unsigned char ic;
	unsigned char pc;
	unsigned char ti;
	unsigned char mi;
	unsigned char lbc;
	unsigned char pbru;
	unsigned char pbrl;
	unsigned char afei;
	unsigned char afeii;
	unsigned char llic;
	unsigned char rlic;
	unsigned char tlb;
	unsigned char thb;
	unsigned char afs;
	unsigned char mioc;
	unsigned char cdfr;
	unsigned char cbru;
	unsigned char cbrl;
} IMAGE16;

typedef struct {
	unsigned int hertz;
	unsigned char bits;
} CODEC_FREQ;

#endif
