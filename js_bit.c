/*
* 			Copyright 1997 JumpStart, Inc.
* 
* 						CONFIDENTIAL
* 
* 					  Copyright Notice
* 					  ----------------
* 
* Copyright 1997 JumpStart, Inc. All rights reserved.
* Reproduction in whole or in part without the express written consent
* of JumpStart Inc. is expressly prohibited. 
* 
*/

/*
** Name of File:
**		js_bit.c
**
** Description:
**		This is the implementation file for the "js_bit" module of the
**		JumpStart library, which is a collection of general-purpose utility 
**		functions that are used by all other subsystems.  This module
**		contains library functions that operate on any individual bit
**		in a block of memory.
**
** Subsystem:
**      JumpStart Library
**
** Version:
**      1.00
**
** Public Functions:
**		jsBitClear
**		jsBitSet
**		jsBitTest
**
** History:
**		Date		Engineer			Purpose
**		--------	----------------	---------------------------------
**		07/22/97	Richard D. Klein	Initial creation.
*/

/* include files */
/* ------------- */
#include "js_bit.h"
#include "js_mem.h"

/* private definitions */
/* ------------------- */
#define	BITS_PER_BYTE		8			/* number of bits per byte	*/

/* private structures */
/* ------------------ */

/* private global variables */
/* ------------------------ */

/* private prototype definitions */
/* ----------------------------- */

/*
** Name:
**		jsBitClear
**
** Purpose:
**		Clears the given bit in the given block of memory.
**
** Calling Sequence:
**		jsBitClear(pBlock, nBit, fBigEndian);
**
** Input:
**		pBlock			Ptr to a block of memory.
**		nBit			Number of the bit to be cleared.
**		fBigEndian		Flag indicating whether the bits within a byte
**						are numbered from left to right (TRUE) or
**						from right to left (FALSE).
**
** Output:
**		NONE
**
** Returns:
**		NONE
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
void jsBitClear(
	unsigned char*	pBlock,		/* Ptr to a block of memory				*/
	int				nBit,		/* Number of the bit to be cleared		*/
	int				fBigEndian)	/* Flag indicating whether the bits		*/
								/* within a byte are numbered from left	*/
								/* to right (TRUE) or from right to		*/
								/* left (FALSE).						*/
{
	int				whichByte;		/* the byte containing the bit	*/
	int				whichBit;		/* the bit within that byte		*/
	unsigned char	mask;			/* mask used to clear the bit	*/

/*	Determine which byte, and which bit within that byte, contain
	the bit of interest.
	------------------------------------------------------------- */
	whichByte = nBit / BITS_PER_BYTE;
	whichBit = nBit % BITS_PER_BYTE;

/*	Construct a mask which, when ANDed into the byte containing the
	bit of interest, will clear the bit of interest.
	--------------------------------------------------------------- */
	mask = (fBigEndian) ? ~(0x80 >> whichBit) : ~(0x01 << whichBit);

/*	Perform the AND operation.
	-------------------------- */
	pBlock[whichByte] &= mask;
}

/*
** Name:
**		jsBitSet
**
** Purpose:
**		Sets the given bit in the given block of memory.
**
** Calling Sequence:
**		jsBitSet(pBlock, nBit, fBigEndian);
**
** Input:
**		pBlock			Ptr to a block of memory.
**		nBit			Number of the bit to be set.
**		fBigEndian		Flag indicating whether the bits within a byte
**						are numbered from left to right (TRUE) or
**						from right to left (FALSE).
**
** Output:
**		NONE
**
** Returns:
**		NONE
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
void jsBitSet(
	unsigned char*	pBlock,		/* Ptr to a block of memory				*/
	int				nBit,		/* Number of the bit to be set.			*/
	int				fBigEndian)	/* Flag indicating whether the bits		*/
								/* within a byte are numbered from left	*/
								/* to right (TRUE) or from right to		*/
								/* left (FALSE).						*/
{
	int				whichByte;		/* the byte containing the bit	*/
	int				whichBit;		/* the bit within that byte		*/
	unsigned char	mask;			/* mask used to set the bit		*/

/*	Determine which byte, and which bit within that byte, contain
	the bit of interest.
	------------------------------------------------------------- */
	whichByte = nBit / BITS_PER_BYTE;
	whichBit = nBit % BITS_PER_BYTE;

/*	Construct a mask which, when ORed into the byte containing the
	bit of interest, will set the bit of interest.
	-------------------------------------------------------------- */
	mask = (fBigEndian) ? (0x80 >> whichBit) : (0x01 << whichBit);

/*	Perform the OR operation.
	------------------------- */
	pBlock[whichByte] |= mask;
}

/*
** Name:
**		jsBitTest
**
** Purpose:
**		Tests the given bit in the given block of memory.
**
** Calling Sequence:
**		isSet = jsBitSet(pBlock, nBit, fBigEndian);
**
** Input:
**		pBlock			Ptr to a block of memory.
**		nBit			Number of the bit to be tested.
**		fBigEndian		Flag indicating whether the bits within a byte
**						are numbered from left to right (TRUE) or
**						from right to left (FALSE).
**
** Output:
**		NONE
**
** Returns:
**		isSet		flag indicating whether or not the given bit is
**					set:
**							TRUE  => the bit IS set
**							FALSE => the bit is NOT set
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
short jsBitTest(
	unsigned char*	pBlock,		/* Ptr to a block of memory				*/
	int				nBit,		/* Number of the bit to be tested.		*/
	int				fBigEndian)	/* Flag indicating whether the bits		*/
								/* within a byte are numbered from left	*/
								/* to right (TRUE) or from right to		*/
								/* left (FALSE).						*/
{
	int				whichByte;		/* the byte containing the bit	*/
	int				whichBit;		/* the bit within that byte		*/
	unsigned char	mask;			/* mask used to test the bit	*/
	unsigned char	theByte;		/* copy of the byte of interest	*/

/*	Determine which byte, and which bit within that byte, contain
	the bit of interest.
	------------------------------------------------------------- */
	whichByte = nBit / BITS_PER_BYTE;
	whichBit = nBit % BITS_PER_BYTE;

/*	Construct a mask which, when ANDed into the byte containing the
	bit of interest, will zero out all the other bits while leaving
	the bit of interest alone.
	-------------------------------------------------------------- */
	mask = (fBigEndian) ? (0x80 >> whichBit) : (0x01 << whichBit);

/*	Perform the AND operation on a COPY of the byte containing the
	bit of interest.
	-------------------------------------------------------------- */
	theByte = pBlock[whichByte];
	theByte &= mask;

/*	The bit of interest is set if and only if the result of the AND
	operation is non-zero.
	--------------------------------------------------------------- */
	return(theByte);
}

