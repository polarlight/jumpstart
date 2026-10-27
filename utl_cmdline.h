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
**		utl_cmdline.h
**
** Description:
**		This is the header file for the "utl_cmdline" module of the UTL
**		subsystem, which contains general-purpose utility functions used
**		by other subsystems.
**
**		Specifically, this module contains library functions that are
**		related to command line processing.  Functions exist to check
**		for the presence of specified options, parse command line
**		parameters, and other command line related tasks.
**
** Subsystem:
**      UTL
**
** Version:
**      1.00
**
** Public Functions:
**		UtlCmdline_GetOption
**		UtlCmdline_GetOptionArgs
**		UtlCmdline_GetSubArgument
**		UtlCmdline_MatchArgument
**
** History:
**		Date		Engineer			Purpose
**		--------	---------------		---------------------------------
**		06/21/96	Richard D. Klein	Added UtlCmdline_GetOption.
**		12/06/95	Steven M. Rosen		Initial creation.
*/

/* include guard START */
/* ------------------- */
#ifndef _UTL_CMDLINE_H_INCLUDED

/* include files */
/* ------------- */

/* public definitions */
/* ------------------- */
#define	FALSE		0		/* the value is FALSE */
#define	TRUE		1		/* the value is TRUE  */

/* public structures */
/* ----------------- */

/* public global variables */
/* ----------------------- */

/* public prototype definitions */
/* ---------------------------- */

int UtlCmdline_GetOption(
	int		argc,			/* number of arguments in command line			*/
	char*	argv[],			/* array of ptrs to command line tokens			*/
	char*	pOptDesignator);/* ptr to cmdline option designator				*/

int UtlCmdline_GetOptionArgs(
	int		argc,			/* count of cmdline arguments					*/
	char*	argv[],			/* ptr to ptr of array of cmdline args			*/
	char*	pOptDesignator,	/* ptr to cmdline option designator				*/
	int*	pIdxFirstArg,	/* ptr to index of first option arg				*/
	int*	pNumArgs);		/* ptr to number of option args					*/

int UtlCmdline_GetSubArgument(
	int		argC,			/* the count of arguments to check				*/
	char*	argV[],			/* ptr to array of arguments to check			*/
	char*	pInputStr,		/* ptr to specified input substring to match	*/
	int		maxOutputLen,	/* the max length of the returned output string	*/
	char*	pOutputStr);	/* ptr to returned output substring if match ok	*/

int UtlCmdline_MatchArgument(
	int		argC,			/* the count of arguments to check				*/
	char*	argV[],			/* ptr to array of arguments to check			*/
	char*	pInputStr);		/* ptr to the specified input string to match	*/

/* include guard END */
/* ----------------- */
#define _UTL_CMDLINE_H_INCLUDED
#endif
