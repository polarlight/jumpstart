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
**		utl_cmdline.c
**
** Description:
**		This module contains library functions for the UTL subsystem, which
**		is a collection of general-purpose utility functions that are used by
**		all other subsystems.
**
**		Specifically, this module contains library functions that are
**      related to command line processing.  Functions exist to check
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
**		UtlCmdline_GetOptionArg
**		UtlCmdline_GetSubArgument
**		UtlCmdline_MatchArgument
**
** History:
**		Date		Engineer			Purpose
**		--------	----------------	---------------------------------
**		06/21/96	Richard D. Klein	Added UtlCmdline_GetOption.
**		12/06/95	Steven M. Rosen		Initial creation.
*/

/* include files */
/* ------------- */
#include <stdio.h>
#include <string.h>
#include "utl_cmdline.h"
#include "utl_dbg.h"

/* private definitions */
/* ------------------- */
#define	CHR_SEPARATOR	'='			/* the separator character 	*/

/* private structures */
/* ------------------ */

/* private global variables */
/* ------------------------ */

/* private prototype definitions */
/* ----------------------------- */

/*
** Name:
**		UtlCmdline_GetOption
**
** Purpose:
**		Scans a given command line looking for a given option.
**		Returns its position in the command line.
**
** Calling Sequence:
**		int ixOptionParm = UtlCmdline_GetOption(int argc, char* argv[],
**												char* pOptDesignator);
**
** Input:
**		argc				Number of tokens in the command line.
**		argv				Array of ptrs to the tokens in the command
**								line.
**		pOptDesignator		Ptr to option designator (such as '-i')
**								to search for.
**
** Output:
**		NONE
**
** Returns:
**		ixOptionParm			Index, in argv, of the given option
**								designator token on the command line.
**								-1 if option not found.
** Error Conditions:
**		Returns -1 if given option not found in given command line.
**
** Comments:
**		NONE
*/
int UtlCmdline_GetOption(
	int		argc,				/* count of cmdline arguments			*/
	char*	argv[],				/* ptr to ptr of array of cmdline args	*/
	char*	pOptDesignator)		/* ptr to cmdline option designator		*/
{
	int		i;			/* loop counter	*/

/*	For each token in the command line...
	------------------------------------- */
	for (i=0; i<argc; i++)
	{
/*		If the token matches the given option...
		---------------------------------------- */
		if (strcmp(argv[i], pOptDesignator) == 0)
		{
/*			Return to caller.
			----------------- */
			return i;
		}
	}

/*	Return to caller with -1 (option not found).
	-------------------------------------------- */
	return -1;
}

/*
** Name:
**		UtlCmdline_GetOptionArgs
**
** Purpose:
**		Scans a given command line looking for a given option.
**		Returns the index in the command line argument list of 
**		the first argument for the specified option and the
**		number of arguments associated with that option.
**
**		Note that to determine the number of arguments associated 
**		with the specified option designator, this function scans
**		the command line until it finds another option, which it
**		recognizes by finding a "-" as the first character of a
**		command line token.
**
** Calling Sequence:
**		int gotOptionArgs = UtlCmdline_GetOptionArgs(int argc, char* argv[],
**													 char* pOptDesignator,
**													 int* pIdxFirstArg,
**													 int* pNumArgs);
**
** Input:
**		argc				Number of tokens in the command line.
**		argv				Array of ptrs to the tokens in the command
**								line.
**		pOptDesignator		Ptr to option designator (such as '-i')
**								to search for.
**
** Output:
**		pIdxFirstArg		Ptr to the index of the first argument of
**								the specified option designator
**		pNumArgs			Ptr to number of arguments associated
**								with the specified option designator
**
** Returns:
**		gotOptionArgs		Whether or not the specified option designator
**							had any associated arguments:
**								TRUE  => at least one option argument was found
**								FALSE => no option arguments were found
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int UtlCmdline_GetOptionArgs(
	int		argc,				/* count of cmdline arguments			*/
	char*	argv[],				/* ptr to ptr of array of cmdline args	*/
	char*	pOptDesignator,		/* ptr to cmdline option designator		*/
	int*	pIdxFirstArg,		/* ptr to index of first option arg		*/
	int*	pNumArgs)			/* ptr to number of option args			*/
{
	int		idxOption;			/* index of cmdline option designator	*/
	int		gotOptionArgs;		/* got option argument(s) indicator		*/
	int		gotNextOption;		/* got the next option indicator		*/

/*	Assume that we do not find any matches.
	--------------------------------------- */
	gotOptionArgs = FALSE;
	*pIdxFirstArg = -1;
	*pNumArgs     = 0;

/*	Look for a match on the cmdline option designator...
	---------------------------------------------------- */
	if ( (idxOption=UtlCmdline_GetOption(argc, argv, pOptDesignator)) > 0 ) {

/*		If the given option designator has an associated argument...
		------------------------------------------------------------ */
		if ( idxOption < (argc - 1) ) {

/*			We know we have at least one argument.
			-------------------------------------- */
			*pIdxFirstArg = ++idxOption;
			(*pNumArgs)++;
			gotOptionArgs = TRUE;

/*			Total up the number of associated arguments by counting
			any arguments until we either bump into the next option
			(first character is "-") OR we reach the end of the 
			command line arguments.
			------------------------------------------------------- */
			gotNextOption = FALSE;
			while ( (!gotNextOption) && (++idxOption < argc) ) {
				if ( strncmp(argv[idxOption], "-", 1) == 0 ) {
					gotNextOption = TRUE;
				} else {
					(*pNumArgs)++;
				}
			}
		}
	}

/*	Return to caller with the option arguments indicator.
	----------------------------------------------------- */
	return(gotOptionArgs);
}

/*
** Name:
**		UtlCmdline_GetSubArgument
**
** Purpose:
**		This function searches for a match between a specified input
**		substring and the list of command line arguments used to
**		invoke a process.  This function assumes the input substring
**		is part of a command line argument of the following form:
**
**				<strInput>=<strOutput>
**
**		This function searches for <strInput> and if it finds a
**		match on this substring, it returns <strOutput>.
**
** Calling Sequence:
**		gotMatch = UtlCmdline_GetSubArgument(argC, argV, InputStr, 
**											 maxOutputLen, pOutputStr);
**
** Input:
**		argC				the count of arguments to check
**		argV				ptr to array of arguments to check
**		pInputStr			ptr to specified input substring to match
**		maxOutputLen		the max length of the returned output string
**
** Output:
**		pOutputStr			ptr to the returned output string if a match
**							was found
**
** Returns:
**		gotMatch		whether or not the specified input string
**						matches one of the command line arguments
**						used to invoke the process:
**							TRUE  => a match was found
**							FALSE => a match was NOT found
**
** Error Conditions:
**			NONE
**
** Comments:
**			NONE
*/
int UtlCmdline_GetSubArgument(
	int		argC,			/* the count of arguments to check				*/
	char*	argV[],			/* ptr to array of arguments to check			*/
	char*	pInputStr,		/* ptr to specified input substring to match	*/
	int		maxOutputLen,	/* the max length of the returned output string	*/
	char*	pOutputStr)		/* ptr to returned output substring if match ok	*/
{
	int			thisArg;		/* the current argument index	*/
	int			gotMatch;		/* got a match flag				*/
	int			lenOutput;		/* length of the output string	*/
	int			theSeparator;	/* the separator character		*/
	char*		pSeparator;		/* ptr to the separator char	*/

/*	Loop through all the command line arguments, searching for a match
	on the specified input substring.  Note that the command line
	argument MUST contain an "=" character in the string to be a
	potential candidate for a match.
	------------------------------------------------------------------ */
	thisArg = 0;
	gotMatch = FALSE;
	theSeparator = CHR_SEPARATOR;
	while ( (!gotMatch) && (thisArg < argC) ) {
		if ( pSeparator=strchr(argV[thisArg], theSeparator) ) {
			if ( (size_t)(pSeparator - argV[thisArg]) == strlen(pInputStr) ) {
				if (strncmp(pInputStr,argV[thisArg],strlen(pInputStr)) == 0) {
					lenOutput = strlen(argV[thisArg]) - strlen(pInputStr) - 1;
					if (lenOutput < maxOutputLen) {
						strcpy(pOutputStr, pSeparator+1);
						gotMatch = TRUE;
					}
				}
			}
		}
		thisArg++;
	}

/*	Return the results of the search to the caller.
	----------------------------------------------- */
	return(gotMatch);
}

/*
** Name:
**		UtlCmdline_MatchArgument
**
** Purpose:
**		This function searches for a match between a specified 
**		input string and the list of command line arguments used to
**		invoke a process.
**
** Calling Sequence:
**		got_match = UtlCmdline_MatchArgument(argC, argV, pInputStr);
**
** Input:
**		argC				the count of arguments to check
**		argV				ptr to array of arguments to check
**		pInputStr			ptr to the specified input string to match
**
** Output:
**		NONE
**
** Returns:
**		gotMatch		whether or not the specified input string
**						matches one of the command line arguments
**						used to invoke the process:
**							TRUE  => a match was found
**							FALSE => a match was NOT found
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int UtlCmdline_MatchArgument(
	int		argC,			/* the count of arguments to check				*/
	char*	argV[],			/* ptr to array of arguments to check			*/
	char*	pInputStr)		/* ptr to the specified input string to match	*/
{
	int			thisArg;		/* the current argument index	*/
	int			gotMatch;		/* got a match flag				*/

/*	Loop through all the command line arguments, searching for a match
	on the specified input string.
	------------------------------------------------------------------ */
	thisArg = 0;
	gotMatch = FALSE;
	while ( (!gotMatch) && (thisArg < argC) ) {
		if ( strcmp(pInputStr, argV[thisArg]) == 0 )
			gotMatch = TRUE;
		thisArg++;
	}

/*	Return the results of the search to the caller.
	----------------------------------------------- */
	return(gotMatch);
}
