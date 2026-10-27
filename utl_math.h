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
**		utl_math.h
**
** Description:
**		This is the header file for the "utl_math" module of the UTL
**		subsystem, which is a collection of general-purpose utility 
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

/* Include Guard START */
/* ------------------- */
#ifndef _UTL_MATH_H_INCLUDED

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
int UtlMath_Rand(
		int		lowerLim,			/* Lower limit of range in which the	*/
									/*	random number is to fall.			*/
		int		upperLim);			/* Upper limit of range in which the	*/
									/*	random number is to fall.			*/

/* Include Guard END */
/* ----------------- */
#define _UTL_MATH_H_INCLUDED
#endif

