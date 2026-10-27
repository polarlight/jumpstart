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
**		utl_string.h
**
** Description:
**		This is the header file for the "utl_string" module of the
**		UTL subsystem, which is a collection of general-purpose utility 
**		functions that are used by all other subsystems.  This module
**		contains library functions related to operations involving strings.
**
** Subsystem:
**      UTL
**
** Version:
**      1.00
**
** Public Functions:
**		UtlString_Substitute
**		UtlString_TokenizeAndConvertInts
**
** History:
**		Date		Engineer			Purpose
**		--------	----------------	---------------------------------
**		06/19/97	Steven M. Rosen		Initial creation.
*/

/* Include Guard START */
/* ------------------- */
#ifndef _UTL_STRING_H_INCLUDED

/* Include Files */
/* ------------- */

/* Public Definitions */
/* ------------------ */
#define	MAX_ARRAY_INTS		64			/* the maximum array of integers	*/
#define	MAX_STRING_SIZE		1024		/* the maximum string size			*/

/* Public Structures */
/* ----------------- */

/* Public Global Variables */
/* ----------------------- */

/* Public Prototype Definitions */
/* ---------------------------- */
char* UtlString_Substitute(
	int		countTokens,	/* the count of both input and output tokens	*/
	char**	ppInTokens,		/* ptr to array of ptrs to the input tokens		*/
	char**	ppOutTokens,	/* ptr to array of ptrs to the output tokens	*/
	char*	pInputStr);		/* ptr to string with the imbedded input tokens	*/

int* UtlString_TokenizeAndConvertInts(
	char*	pInputStr,		/* ptr to string to be tokenized and converted	*/
	char*	pDelimitersStr,	/* ptr to string of delimiter characters		*/
	int*	pSizeArrayInts);/* ptr to the returned size of the returned		*/
							/* array of converted integers					*/

/* Include Guard END */
/* ----------------- */
#define _UTL_STRING_H_INCLUDED
#endif
