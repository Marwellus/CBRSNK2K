#ifndef __FORTE_H__
#define __FORTE_H__

/***************************************************************************
*	NAME:  FORTE.H
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
*  CREATION DATE: 07/30/92
*--------------------------------------------------------------------------*
*     VERSION	DATE	   NAME		DESCRIPTION
*>	1.0	07/30/92		Original
***************************************************************************/

/* Either define 1 of these here or on the compiler command line */
//#define BORLANDC
//#define MSOFTC
//#define WATCOMC
//#define METAWARE

#ifdef __HIGHC__
#define METAWARE
#endif

#ifdef __BORLANDC__
#define BORLANDC
#endif

#ifdef _MSC_VER
#define MSOFTC
#endif

#ifdef __WATCOMC__
#define WATCOMC
#endif

/***************************************************************************
 *
 *	BORLANDC Compilers
 *
 **************************************************************************/
#ifdef BORLANDC
#define ENTER_CRITICAL	asm pushf; asm cli;
#define ENTER_CRITICAL_ON	asm pushf; asm sti;
#define LEAVE_CRITICAL		asm popf;
#define LEAVE_CRITICAL_ON LEAVE_CRITICAL
#define FARFUNC			far
#undef FLAT_MODEL
#endif

/***************************************************************************
 *
 *	MICROSOFT Compilers
 *
 **************************************************************************/
#ifdef MSOFTC
#define asm	_asm
#define interrupt _interrupt
#define ENTER_CRITICAL	_asm pushf; _asm cli;
#define ENTER_CRITICAL_ON	_asm pushf; _asm sti;
#define LEAVE_CRITICAL		_asm popf;
#define LEAVE_CRITICAL_ON LEAVE_CRITICAL
#define FARFUNC			far
#undef FLAT_MODEL
#endif

/***************************************************************************
 *
 *	WATCOM Compiler
 *
 **************************************************************************/
#ifdef WATCOMC
#undef far
#define far
#define FARFUNC			__far
#define FLAT_MODEL

#define ENTER_CRITICAL IRQ_PUSH_OFF()
extern void IRQ_PUSH_OFF (void);
#pragma aux IRQ_PUSH_OFF =	\
    "pushfd",			\
    "cli";

#define ENTER_CRITICAL_ON IRQ_PUSH_ON()
extern void IRQ_PUSH_ON (void);
#pragma aux IRQ_PUSH_ON =	\
    "pushfd",			\
    "sti";

#define LEAVE_CRITICAL IRQ_POP()
extern void IRQ_POP (void);
#pragma aux IRQ_POP =	\
    "popfd";
#define LEAVE_CRITICAL_ON LEAVE_CRITICAL
#endif

/***************************************************************************
 *
 *	METAWARE HIGHC Compiler
 *
 **************************************************************************/
#ifdef METAWARE

#define inp _inb
#define inpw _inpw
#define outp _outb
#define outpw _outw

#pragma On (Globals_volatile)

#undef far
#define far	_Far
#define FARFUNC
#define FLAT_MODEL
#ifdef NEVER
#define OPCODE_NOP		0x90
#define OPCODE_PUSHF	0x9c
#define OPCODE_CLI		0xfa
#define OPCODE_STI		0xfb
#define OPCODE_POPF		0x9d
#define ENTER_CRITICAL	_inline(OPCODE_PUSHF);\
  _inline(OPCODE_CLI);
#define ENTER_CRITICAL_ON	_inline(OPCODE_PUSHF);\
  _inline(OPCODE_STI);
#define LEAVE_CRITICAL		_inline(OPCODE_POPF);
#endif

#define ENTER_CRITICAL	_gf1_data.gf1_sema4++;
extern void leave_critical();
#define LEAVE_CRITICAL	leave_critical();
#define ENTER_CRITICAL_ON
#define LEAVE_CRITICAL_ON
#endif

/*****************************************************************/

#define		TRUE	1
#define		FALSE	0

#define ON			1
#define OFF			0

typedef 	void 	(*PFV)();
typedef 	int 	(*PFI)();
#ifdef METAWARE
typedef 	_Far _INTERRPT void (*PVI)();
#else
#ifdef __TINY__
typedef 	void 	(interrupt *PVI)();
#else
typedef 	void 	(interrupt FARFUNC *PVI)();
#endif
#endif

#endif

