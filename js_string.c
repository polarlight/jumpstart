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
**		utl_string.c
**
** Description:
**		This is the implementation file for the "utl_string" module of the
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
**		06/18/97	Steven M. Rosen		Initial creation.
*/

/* include files */
/* ------------- */
#include <string.h>
#include "utl_dbg.h"
#include "utl_log.h"
#include "utl_string.h"

/* private definitions */
/* ------------------- */

/* private structures */
/* ------------------- */

/* private global variables */
/* ------------------------ */

/* private prototype declarations */
/* ------------------------------ */

/****************************************************/
/* NOTE: Please keep the following static functions */
/* ====  in alphabetical order.  Thank you!         */
/****************************************************/

static int new_string_size_okay(
	char*	pStr1,			/* ptr to the first string fragment				*/
	char*	pStr2,			/* ptr to the second string fragment			*/
	int		countNewChars);	/* the number of new characters being added		*/

static int validate_substitute_params(
	int		countTokens,	/* the count of both input and output tokens	*/
	char**	ppInTokens,		/* ptr to array of ptrs to the input tokens		*/
	char**	ppOutTokens,	/* ptr to array of ptrs to the output tokens	*/
	char*	pInputStr);		/* ptr to string with the imbedded input tokens	*/

static int validate_tokenize_and_convert_ints_params(
	char*	pInputStr,		/* ptr to string to be tokenized and converted	*/
	char*	pDelimitersStr,	/* ptr to string of delimiter characters		*/
	int*	pSizeArrayInts);/* ptr to the returned size of the returned		*/
							/* array of converted integers					*/

/*
** Name:
**		UtlString_Substitute
**
** Purpose:
**		This public function substitutes a specified number of "input"
**		string tokens within a specified string with an alternate set of
**		"output" string tokens.  The size of the "input" and "output"
**		token lists MUST be the same.
**
** Calling Sequence:
**		pOutputStr = UtlString_Substitute(	countTokens,
**											ppInTokens,
**											ppOutTokens,
**											pInputStr	);
**
** Input:
**		countTokens		the count of both input and output tokens
**		ppInTokens		ptr to array of ptrs to the input tokens
**		ppOutTokens		ptr to array of ptrs to the output tokens
**		pInputStr		ptr to string with the imbedded input tokens
**
** Output:
**		NONE
**
** Returns:
**		pOutputStr		ptr to the returned output string
**						(a value of NULL indicates there was an error)
**
**						IMPORTANT NOTE: the application is responsible
**						for freeing up the memory that was allocated by
**						this function for this returned output string
**
** Error Conditions:
**		NONE
**
** Comments:
**		Following are some examples of various output strings that would
**		be created from an input string using the specified input and
**		output tokens:
**
**		1)	Input String = "The point %N at location %L went into alarm
**							at time %T."
**
**				Input Tokens		Output Tokens
**				------------		-------------
**					"%L"			"Rack 5, Chassis A3"
**					"%N"			"# 14"
**					"%T"			"06/19/1997 13:00"
**
**			Output String = "The point #14 at location Rack 5, Chassis A3
**							 went into alarm at time 06/19/1997 13:00."
**
**
**		2)	Input String = "I will meet ^PERSON^ at the (place) in $Time
**							minutes.  ^PERSON^ is always very prompt."
**
**				Input Tokens		Output Tokens
**				------------		-------------
**					"(place)"		"Roller Palace"
**					"$Time"			"45"
**					"^PERSON^"		"Becky"
**
**			Output String = "I will meet Becky at the Roller Palace in 45
**							 minutes.  Becky is always very prompt."
**
**
**		3)	Input String = "This input string will look just like the output
**							string because NO substitution take place."
**
**				Input Tokens		Output Tokens
**				------------		-------------
**					"YES"			"Absolutely, Sir!!!"
**					"%xxx"			"A dumb name for an input token"
**
**			Output String = "This input string will look just like the output
**							 string because NO substitution take place."
**
*/
char* UtlString_Substitute(
	int		countTokens,	/* the count of both input and output tokens	*/
	char**	ppInTokens,		/* ptr to array of ptrs to the input tokens		*/
	char**	ppOutTokens,	/* ptr to array of ptrs to the output tokens	*/
	char*	pInputStr)		/* ptr to string with the imbedded input tokens	*/
{
	char*		pOutputStr = NULL;	/* ptr to the returned output string	*/
	char*		pSubStr;			/* ptr to substring we are looking for	*/
	char*		pTempStr;			/* ptr to a temporary working area		*/
	char*		pNewInputStr;		/* ptr to the new input string			*/
	char*		pThisInToken;		/* ptr to this input token				*/
	char*		pThisOutToken;		/* ptr to this output token				*/
	int			lenThisInToken;		/* length of this input token			*/
	int			lenThisOutToken;	/* length of this output token			*/
	int			charsBefore;		/* number of chars BEFORE input token	*/
	int			charsAfter;			/* number of chars AFTER input token	*/
	int			ndx;				/* loop counter							*/
	char*	func="UtlString_Substitute"; 		/* name of this func		*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Validate the input parameters.
	------------------------------ */
	if ( !validate_substitute_params(countTokens, ppInTokens,
									 ppOutTokens, pInputStr) ) {
		goto _errValidateParams;
	}

/*	Allocate a pair of nice large temporary working areas.
	------------------------------------------------------ */
	pTempStr = (char*)malloc(MAX_STRING_SIZE);
	if ( pTempStr == NULL ) {
		goto _errAllocatingTempStr;
	}
	pNewInputStr = (char*)malloc(MAX_STRING_SIZE);
	if ( pNewInputStr == NULL ) {
		goto _errAllocatingNewInputStr;
	}

/*	Initialize our NewInputString that will eventually contain the fully
	substituted input string.
	-------------------------------------------------------------------- */
	if ( !new_string_size_okay(pInputStr, NULL, 0) ) {
		goto _errStringTooBig;
	}
	strcpy(pNewInputStr, pInputStr);

/*	Run through each of the input tokens, substituting the corresponding
	output token for every occurrence of each input token in the 
	specified input string.
	-------------------------------------------------------------------- */
	for (ndx=0; ndx<countTokens; ndx++) {
		memset(pTempStr, 0, MAX_STRING_SIZE);

		pThisInToken    = *(ppInTokens + ndx);
		pThisOutToken   = *(ppOutTokens + ndx);

		lenThisInToken  = strlen(pThisInToken);
		lenThisOutToken = strlen(pThisOutToken);

		while ( (pSubStr=strstr(pNewInputStr, pThisInToken)) != NULL ) {

/*			We found an input token to substitute, so first copy
			everything BEFORE the input token that we just found to
			our temporary working area.
			------------------------------------------------------- */
			charsBefore = pSubStr - pNewInputStr;
			if (charsBefore > 0) {
				if ( !new_string_size_okay(NULL, NULL, charsBefore) ) {
					goto _errStringTooBig;
				}
				strncpy(pTempStr, pNewInputStr, charsBefore);
			}

/*			Now append the subtitute output token for the input token
			that we are trying to replace.
			--------------------------------------------------------- */
			if (lenThisOutToken > 0) {
				if ( !new_string_size_okay(pTempStr, pThisOutToken, 0) ) {
					goto _errStringTooBig;
				}
				strcat(pTempStr, pThisOutToken);
			}

/*			Finally, copy the rest of the input string (AFTER the input
			token) to our temporary working area.
			----------------------------------------------------------- */
			charsAfter = strlen(pNewInputStr) -
									(charsBefore + lenThisInToken);
			if (charsAfter > 0) {
				if ( !new_string_size_okay(pTempStr,
						(pNewInputStr + charsBefore + lenThisInToken), 0) ) {
					goto _errStringTooBig;
				}
				strcat(pTempStr,
					   (pNewInputStr + charsBefore + lenThisInToken));
			}

/*			Now that we have substituted this input token into our
			temporary working area, get the temporary working area
			copied back as our newest NewInputString and try to look
			for another input token that can be substituted.
			-------------------------------------------------------- */
			strcpy(pNewInputStr, pTempStr);
			memset(pTempStr, 0, MAX_STRING_SIZE);
		}
	}

/*	Now that the input string has been completely substituted, allocate
	some memory for the returned output string and then copy the new
	input string to the returned output string.
	------------------------------------------------------------------- */
	pOutputStr = (char*)malloc(strlen(pNewInputStr) + 1);
	if ( pOutputStr == NULL ) {
		goto _errAllocatingOutputStr;
	}
	strcpy(pOutputStr, pNewInputStr);

/*	Perform structured exception handling.
	-------------------------------------- */
_errStringTooBig:
_errAllocatingOutputStr:
	if (pNewInputStr) free(pNewInputStr);

_errAllocatingNewInputStr:
	if (pTempStr) free(pTempStr);

_errAllocatingTempStr:
_errValidateParams:

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(pOutputStr);
}

/*
** Name:
**		UtlString_TokenizeAndConvertInts
**
** Purpose:
**		This public function tokenizes a specified string using a
**		specified set of delimiters and then converts each of the
**		tokens into their binary integer equivalents.
**
** Calling Sequence:
**		pArrayInts = UtlString_TokenizeAndConvertInts(	pInputStr,
**														pDelimitersStr,
**														pSizeArrayInts);
**
** Input:
**		pInputStr		ptr to string to be tokenized and converted
**		pDelimitersStr	ptr to string of delimiter characters
**
** Output:
**		pSizeArrayInts	ptr to the returned size of the returned array
**						of converted integers
**
** Returns:
**		pArrayInts		ptr to the returned array of converted integers
**						(a value of NULL indicates there was an error)
**
**						IMPORTANT NOTE: the application is responsible
**						for freeing up the memory that was allocated by
**						this function for this returned array of integers
**
** Error Conditions:
**		NONE
**
** Comments:
**		Following are some examples of various output arrays of integers
**		that would be created from a specified input string and specified
**		string of delimiters:
**
**		1)	Input String      = " -7  13,66;75"
**			Delimiters String = " ,;"
**
**			Returned Array of Ints = -7 13 66 75
**
**		2)	Input String      = " -7  13,66;75"
**			Delimiters String = ";"
**
**			Returned Array of Ints = -7 75
**
**		3)	Input String      = " -7a  13,66;75"
**			Delimiters String = " ,;"
**
**			Returned Array of Ints = -7 13 66 75
**
**
*/
int* UtlString_TokenizeAndConvertInts(
	char*	pInputStr,		/* ptr to string to be tokenized and converted	*/
	char*	pDelimitersStr,	/* ptr to string of delimiter characters		*/
	int*	pSizeArrayInts)	/* ptr to the returned size of the returned		*/
							/* array of converted integers					*/
{
	int*		pArrayInts = NULL;		/* ptr to returned array of ints	*/
	int*		pTempArrayInts = NULL;	/* ptr to temporary array of ints	*/
	char*		pToken;					/* ptr to a token of the input str	*/
	char*		pInputStrCopy = NULL;	/* ptr to a copy of the input str	*/
	char*	func="UtlString_TokenizeAndConvertInts"; /* name of this func	*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Validate the input parameters.
	------------------------------ */
	if ( !validate_tokenize_and_convert_ints_params(pInputStr,
													pDelimitersStr,
													pSizeArrayInts) ) {
		goto _errValidateParams;
	}

/*	Indicate that we have no returned integers at this time.
	-------------------------------------------------------- */
	*pSizeArrayInts = 0;

/*	Make a copy of the original input string, since "strtok" will
	modify the string that it tokenizes.
	------------------------------------------------------------- */
	pInputStrCopy = (char*)malloc(strlen(pInputStr) + 1);
	if (pInputStrCopy == NULL) {
		goto _errAllocatingInputStrCopy;
	}
	strcpy(pInputStrCopy, pInputStr);

/*	Allocate a nice large temporary working area.
	--------------------------------------------- */
	pTempArrayInts = (int*)malloc(MAX_ARRAY_INTS * sizeof(int));
	if ( pTempArrayInts == NULL ) {
		goto _errAllocatingTempArrayInts;
	}

/*	Establish the string to be tokenized and get the first token.
	------------------------------------------------------------- */
	pToken = strtok( pInputStrCopy, pDelimitersStr );
	
/*	While there are tokens in the input string...
	--------------------------------------------- */
	while( pToken != NULL ) {

/*		Make sure that we have room to add this next converted token.
		------------------------------------------------------------- */
		if ( (*pSizeArrayInts) >= MAX_ARRAY_INTS ) {
			UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
				"ERROR: Too many converted integers, max = %d\n",
					MAX_ARRAY_INTS);
			goto _errTooManyInts;
		}

/*		We have a token, so lets convert it to its binary equivalent and
		add it to the array of temporar integers that will eventually be
		returned to the caller.
		---------------------------------------------------------------- */
		*( pTempArrayInts + (*pSizeArrayInts) ) = atoi(pToken);

/*		Bump the number of returned converted integers.
		----------------------------------------------- */
		(*pSizeArrayInts)++;

/*		Get the next token from the input string.
		----------------------------------------- */
		pToken = strtok( NULL, pDelimitersStr );
	}

/*	Now that the input string has been completely tokenized and converted
	to integers, allocate some memory for the returned array of converted
	integers and then copy the temporary array of converted integers to
	the returned array of converted integers.
	--------------------------------------------------------------------- */
	if ( (*pSizeArrayInts) > 0 ) {
		pArrayInts = (int*)malloc( (*pSizeArrayInts) * sizeof(int) );
		if ( pArrayInts == NULL ) {
			goto _errAllocatingArrayInts;
		}
		memcpy(	pArrayInts, pTempArrayInts,
				((*pSizeArrayInts) * sizeof(int)) );
	}

/*	Perform structured exception handling.
	-------------------------------------- */
_errAllocatingArrayInts:
_errTooManyInts:
	if (pTempArrayInts) free(pTempArrayInts);

_errAllocatingTempArrayInts:
	if (pInputStrCopy) free(pInputStrCopy);

_errAllocatingInputStrCopy:
_errValidateParams:

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(pArrayInts);
}

/****************************************************/
/* NOTE: Please keep the following static functions */
/* ====  in alphabetical order.  Thank you!         */
/****************************************************/

/*
** Name:
**		new_string_size_okay
**
** Purpose:
**		This local function verifies that the new string we are about to
**		create as a result of a strcpy or strncpy or strcat will be within
**		the allowed maximum size.
*/
static int new_string_size_okay(
	char*	pStr1,			/* ptr to the first string fragment				*/
	char*	pStr2,			/* ptr to the second string fragment			*/
	int		countNewChars)	/* the number of new characters being added		*/
{
	int				isOkay = FALSE;			/* the return status		*/
	int				lenStr1;				/* length of string 1		*/
	int				lenStr2;				/* length of string 2		*/
	char*	func="new_string_size_okay"; 	/* name of this function	*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Make sure the size of the two string fragments and the count of
	new characters will be within the maximum size of a string.
	--------------------------------------------------------------- */
	lenStr1 = 0;
	if (pStr1 != NULL) lenStr1 = strlen(pStr1);

	lenStr2 = 0;
	if (pStr2 != NULL) lenStr2 = strlen(pStr2);

	if ( (lenStr1 + lenStr2 + countNewChars) >= MAX_STRING_SIZE ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: The new output string will be too big (%d), max=%d\n",
				(lenStr1 + lenStr2 + countNewChars), MAX_STRING_SIZE);
		goto _errStringTooBig;
	}

/*	Indicate that everything went fine.
	----------------------------------- */
	isOkay = TRUE;

/*	Perform structured exception handling.
	-------------------------------------- */
_errStringTooBig:

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOkay);
}

/*
** Name:
**		validate_substitute_params
**
** Purpose:
**		This local function validates the input parameters to the
**		the UtlString_Substitute public function.
*/
static int validate_substitute_params(
	int		countTokens,	/* the count of both input and output tokens	*/
	char**	ppInTokens,		/* ptr to array of ptrs to the input tokens		*/
	char**	ppOutTokens,	/* ptr to array of ptrs to the output tokens	*/
	char*	pInputStr)		/* ptr to string with the imbedded input tokens	*/
{
	int					isOkay = FALSE;			/* the return status		*/
	char*	func="validate_substitute_params"; 	/* name of this function	*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Validate the input parameters.
	------------------------------ */
	if ( countTokens <= 0 ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: The count of tokens (%d) must be greater than 0\n",
				countTokens);
		goto _errInputParams;
	}

	if ( ppInTokens == NULL ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Invalid ptr to array of ptrs to the input tokens\n");
		goto _errInputParams;
	}
	if ( *ppInTokens == NULL ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Invalid array of ptrs to the input tokens\n");
		goto _errInputParams;
	}

	if ( ppOutTokens == NULL ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Invalid ptr to array of ptrs to the output tokens\n");
		goto _errInputParams;
	}
	if ( *ppOutTokens == NULL ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Invalid array of ptrs to the output tokens\n");
		goto _errInputParams;
	}

	if ( pInputStr == NULL ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Invalid ptr to string with the imbedded input tokens\n");
		goto _errInputParams;
	}

/*	Indicate that everything went fine.
	----------------------------------- */
	isOkay = TRUE;

/*	Perform structured exception handling.
	-------------------------------------- */
_errInputParams:

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOkay);
}

/*
** Name:
**		validate_tokenize_and_convert_ints_params
**
** Purpose:
**		This local function validates the input parameters to the
**		the UtlString_TokenizeAndConvertInts public function.
*/
static int validate_tokenize_and_convert_ints_params(
	char*	pInputStr,		/* ptr to string to be tokenized and converted	*/
	char*	pDelimitersStr,	/* ptr to string of delimiter characters		*/
	int*	pSizeArrayInts)	/* ptr to the returned size of the returned		*/
							/* array of converted integers					*/
{
	int					isOkay = FALSE;			/* the return status		*/
	char*	func="validate_tokenize_and_convert_ints_params";
												/* name of this function	*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Validate the input parameters.
	------------------------------ */
	if ( pInputStr == NULL ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Invalid ptr to string to be tokenized and converted\n");
		goto _errInputParams;
	}
	if ( pDelimitersStr == NULL ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Invalid ptr to string of delimiter characters\n");
		goto _errInputParams;
	}
	if ( pSizeArrayInts == NULL ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Invalid ptr to the returned size of returned array\n");
		goto _errInputParams;
	}

/*	Indicate that everything went fine.
	----------------------------------- */
	isOkay = TRUE;

/*	Perform structured exception handling.
	-------------------------------------- */
_errInputParams:

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOkay);
}
