/*
* 			Copyright 1993-1997 Sony Electronics, Inc.
* 
* 						CONFIDENTIAL
* 
* 					  Copyright Notice
* 					  ----------------
* 
* Copyright 1993-1997 Sony Electronics Inc. All rights reserved.
* Reproduction in whole or in part without the express written consent
* of Sony Electronics Inc. is expressly prohibited. 
* 
*/

/*
** Name of File:
**		utl_bit.h
**
** Description:
**		This is the header file for the "utl_bit" module of the UTL
**		subsystem, which is a collection of general-purpose utility 
**		functions that are used by all other subsystems.  This module
**		contains library functions that operate on any individual bit
**		in a block of memory.
**
** Subsystem:
**      UTL
**
** Version:
**      1.00
**
** Public Functions:
**		UtlBit_Clear
**		UtlBit_Set
**		UtlBit_Test
**
** History:
**		Date		Engineer			Purpose
**		--------	----------------	---------------------------------
**		03/21/96	Richard D. Klein	Initial creation.
*/

/* Include Guard START */
/* ------------------- */
#ifndef _UTL_BIT_H_INCLUDED

/* Include Files */
/* ------------- */

/* Public Definitions */
/* ------------------ */

/* Public Structures */
/* ----------------- */

/* Public Global Variables */
/* ----------------------- */

/* Public Prototype Definitions */
/* ---------------------------- */
void	UtlBit_Clear(unsigned char* pBlock, int nBit, int fBigEndian);
void	UtlBit_Set(unsigned char* pBlock, int nBit, int fBigEndian);
short	UtlBit_Test(unsigned char* pBlock, int nBit, int fBigEndian);

/* Include Guard END */
/* ----------------- */
#define _UTL_BIT_H_INCLUDED
#endif

