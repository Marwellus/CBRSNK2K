#pragma pack(1)
#ifndef __PATCH_H__
#define __PATCH_H__

/***************************************************************************
*	NAME:  PATCH.H
**	COPYRIGHT:
**	"Copyright (c) 1991,1992, by FORTE
**
**       "This software is furnished under a license and may be used,
**       copied, or disclosed only in accordance with the terms of such
**       license and with the inclusion of the above copyright notice.
**       This software or any other copies thereof may not be provided or
**       otherwise made available to any other person. No title to and
**       ownership of the software is hereby transfered."
****************************************************************************
*  CREATION DATE: 07/01/92
*--------------------------------------------------------------------------*
*     VERSION	DATE	   NAME		DESCRIPTION
*>	1.0	07/01/92		Original
***************************************************************************/

#define ENVELOPES	6	

/* This is the definition for what FORTE's patch format is. All .PAT */
/* files will have this format. */

#define HEADER_SIZE	12
#define ID_SIZE		10
#define DESC_SIZE 	60
#define RESERVED_SIZE	40
#define PATCH_HEADER_RESERVED_SIZE 36
#define LAYER_RESERVED_SIZE	40
#define PATCH_DATA_RESERVED_SIZE	36
#define GF1_HEADER_TEXT	"GF1PATCH110"

#define byte	unsigned char
#define word	unsigned short
#define dword	unsigned long

typedef struct
{
	char	header[HEADER_SIZE];	
	char	gravis_id[ID_SIZE];	/* Id = "ID#000002" */
	char	description[DESC_SIZE];
	byte	instruments;
	byte	voices;
	byte	channels;
	word	wave_forms;
	word	master_volume;
	dword data_size;
	byte	reserved[PATCH_HEADER_RESERVED_SIZE];
} PATCHHEADER;

typedef struct
{
	word	instrument;
	char	instrument_name[16];
	dword instrument_size;
	byte	layers;
	byte	reserved[RESERVED_SIZE];	
} INSTRUMENTDATA;

typedef struct
{
	byte		layer_duplicate;
	byte		layer;
	dword		layer_size;
	byte		samples;
	byte		reserved[LAYER_RESERVED_SIZE];	
} LAYERDATA;

typedef struct
{
	char		wave_name[7];
	byte		fractions;
	long		wave_size;
	long		start_loop;
	long		end_loop;
	word		sample_rate;
	long		low_frequency;
	long		high_frequency;
	long		root_frequency;
	short		tune;
	byte		balance;
	byte		envelope_rate[ENVELOPES];
	byte		envelope_offset[ENVELOPES];	
	byte		tremolo_sweep;
	byte		tremolo_rate;
	byte		tremolo_depth;
	byte		vibrato_sweep;
	byte		vibrato_rate;
	byte		vibrato_depth;
	byte		modes;
	short		scale_frequency;
	word		scale_factor;		/* from 0 to 2048 or 0 to 2 */
	byte		reserved[PATCH_DATA_RESERVED_SIZE];
} PATCHDATA;

#endif
#pragma pack()
