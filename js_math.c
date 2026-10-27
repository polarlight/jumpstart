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
**		utl_math.c
**
** Description:
**		This is the implementation file for the "utl_math" module of the
**		UTL subsystem, which is a collection of general-purpose utility 
**		functions that are used by all other subsystems.  This module
**		contains library functions that perform miscellaneous, generally
**		useful mathematical operations.
**
** Subsystem:
**      UTL
**
** Version:
**      1.00
**
** Public Functions:
**		UtlMath_Rand
**
** History:
**		Date		Engineer			Purpose
**		--------	----------------	---------------------------------
**		09/06/96	Richard D. Klein	Initial creation.
*/

/* include files */
/* ------------- */
#include <stdlib.h>
#include "utl_dbg.h"
#include "utl_log.h"
#include "utl_math.h"

/* private definitions */
/* ------------------- */

/* private structures */
/* ------------------ */

/* private global variables */
/* ------------------------ */

/**********************************/
/* private prototype declarations */
/**********************************/

/*---------------------------------------------------+
 | NOTE: Please keep the following static function   |
 | ====  declarations in alphabetical order. Thanks! |
 +---------------------------------------------------*/

/* <no static functions at the present time> */


/*
** Name:
**		UtlMath_Rand
**
** Purpose:
**		Returns a random integer lying in the range specified
**		by the caller.
**
** Calling Sequence:
**		randInt = UtlMath_Rand(int lowerLim, int upperLim);
**
** Input:
**		lowerLim			Lower limit of the range in which the
**							random integer is to lie.
**
**		upperLim			Upper limit of the range in which the
**							random integer is to lie.
**
** Output:
**		NONE
**
** Returns:
**		randInt				Random integer lying in the range
**							extending from lowerLim to upperLim.
*/
int UtlMath_Rand(
	int			lowerLim,			/* Lower limit of range.				*/
	int			upperLim)			/* Upper limit of range.				*/
{
	int			min, max;			/* The minimum and maximum values of	*/
									/*	the range specified by the caller.	*/
	int			randInt;			/* The generated random integer.		*/
	char*		func = "UtlMath_Rand";
									/* Name of this function.				*/

/*	If tracing is turned on, log a message indicating we've just
	entered this function.
	------------------------------------------------------------ */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	If the caller accidentally reversed the lower and upper limits,
	let's un-reverse them.
	--------------------------------------------------------------- */
	min = (lowerLim <= upperLim) ? lowerLim : upperLim;
	max = (lowerLim <= upperLim) ? upperLim : lowerLim;

/*	Generate a random integer, then scale and offset it so it lies
	in the range specified by the caller.
	-------------------------------------------------------------- */
	randInt = min + (rand() % (max - min + 1));

/*	If tracing is turned on, log a message indicating we're about
	to leave this function.
	------------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...Leaving\n");

	return randInt;
}

