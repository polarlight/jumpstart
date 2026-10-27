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
**		js_bool_expr.c
**
** Description:
**		This is the implementation file for the "js_bool_expr" module of the
**		JumpStart library, which is a collection of general-purpose utility 
**		functions that are used by all other subsystems.  This module
**		contains library functions that create and manage a simple
**		Boolean expression evaluation object.
**
** Subsystem:
**      JumpStart Library
**
** Version:
**      1.00
**
** Public Functions:
**		jsBoolExprCreate
**		jsBoolExprDelete
**		jsBoolExprEvaluate
**		jsBoolExprSetDump
**
** History:
**		Date		Engineer			Purpose
**		--------	----------------	---------------------------------
**		07/22/97	Steven M. Rosen		Initial creation.
*/

/* include files */
/* ------------- */
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "js_mem.h"
#include "js_log.h"
#include "js_bool_stack.h"
#include "js_bool_expr.h"

/* private definitions */
/* ------------------- */
#define	BSIZE	64
#define	NONE	-1
#define	EOS		'\0'

#define	NUM		256
#define	AND		257
#define	OR		258
#define	NOT		259
#define	ID		260
#define	DONE	261

/* private structures */
/* ------------------ */

/*	Boolean Evaluation data structure.
	---------------------------------- */
typedef struct {
	int			currCharPos;	/* the current character position			*/
	int			theLookAhead;	/* the next type of token					*/
	int			tokenValue;		/* the value of the current token			*/
	int			theBoolStack;	/* the boolean stack						*/
	int			currBoolValue;	/* the current boolean value				*/
	char		lexBuff[BSIZE];	/* buffer for lexical analysis				*/
	char*		pEvalString;	/* ptr to the boolean evaluation string		*/
	int			countIds;		/* count of identifier strings and values	*/
	char**		ppIdStrings;	/* ptr to array of identifier strings ptrs	*/
	int*		pIdValues;		/* ptr to array of identifier values		*/
} UTL_BOOL_EVAL;

/* private global variables */
/* ------------------------ */
static int	m_DumpStack = FALSE;		/* dump boolean stack flag			*/

/* private prototype declarations */
/* ------------------------------ */
static int emit(
	UTL_BOOL_EVAL*	pBoolEval,		/* ptr to boolean evaluation context	*/
	int				thisTokenChar,	/* the token character to emit			*/
	int				tokenValue);	/* the token value						*/

static int expr(
	UTL_BOOL_EVAL*	pBoolEval);		/* ptr to boolean evaluation context	*/

static int factor(
	UTL_BOOL_EVAL*	pBoolEval);		/* ptr to boolean evaluation context	*/

static int init_context(
	UTL_BOOL_EVAL*	pBoolEval);		/* ptr to boolean evaluation context	*/

static int lexan(
	UTL_BOOL_EVAL*	pBoolEval,		/* ptr to boolean evaluation context	*/
	int*			pNextToken);	/* ptr to returned next token			*/

static int lookup(
	UTL_BOOL_EVAL*	pBoolEval,		/* ptr to boolean evaluation context	*/
	int*			pTokenValue);	/* ptr to returned token value			*/

static int match(
	UTL_BOOL_EVAL*	pBoolEval,		/* ptr to boolean evaluation context	*/
	int				thisTokenChar);	/* the token to match					*/

static int parse(
	UTL_BOOL_EVAL*	pBoolEval,		/* ptr to boolean evaluation context	*/
	int*			pBoolResult);	/* ptr to returned boolean result		*/

static int term(
	UTL_BOOL_EVAL*	pBoolEval);		/* ptr to boolean evaluation context	*/

static int term_context(
	UTL_BOOL_EVAL*	pBoolEval);		/* ptr to boolean evaluation context	*/

/*
** Name:
**		jsBoolExprCreate
**
** Purpose:
**		This public function creates a new Boolean expression object.
**
** Calling Sequence:
**		isOkay = jsBoolExprCreate(hLog, phBoolExpr);
**
** Input:
**		hLog				handle to associated log object
**
** Output:
**		phBoolExpr			ptr to returned Boolean expression
**							object handle
**
** Returns:
**		isOkay				Boolean indicating whether or not the
**							function executed successfully.
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int jsBoolExprCreate(
		HANDLE	hLog,		/* handle to associated log object		*/
		HANDLE*	phBoolExpr)	/* ptr to returned Boolean expression	*/
							/* object handle						*/
{
	int				isOkay = FALSE;		/* return value of this function	*/
	UTL_BOOL_EVAL*	pBoolEval;			/* ptr to boolean evaluation data	*/
	char*	func="jsBoolExprCreate";	/* name of this function			*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Assume that something is going to go wrong until proven otherwise.
	------------------------------------------------------------------ */
	*pEvalHandle = 0;

/*	Allocate a boolean evaluation data structure. 
	--------------------------------------------- */
	pBoolEval = (UTL_BOOL_EVAL*)malloc(sizeof(UTL_BOOL_EVAL));
	if (pBoolEval == NULL) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"Could not allocate space for new boolean evaluation\n");
		goto _errAllocateMemory;
	}

/*	Cast the address as an int and write it into the storage provided
	by the caller.
	----------------------------------------------------------------- */
	*pEvalHandle = (int)pBoolEval;

/*	Indicate that everything went okay.
	----------------------------------- */
	isOkay = TRUE;

/*	Perform structured exception handling.
	-------------------------------------- */
_errAllocateMemory:

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOkay);
}

/*
** Name:
**		jsBoolExprDelete
**
** Purpose:
**		This public function deletes a Boolean expression evaluation object.
**
** Calling Sequence:
**		isOkay = jsBoolExprDelete(hLog, hBoolExpr);
**
** Input:
**		hLog				handle to associated log object
**		hBoolExpr			handle to Boolean expression object
**
** Output:
**		NONE
**
** Returns:
**		isOkay				Boolean indicating whether or not the
**							function executed successfully.
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int jsBoolExprDelete(
		HANDLE	hLog,		/* handle to associated log object			*/
		HANDLE	hBoolExpr)	/* handle to Boolean expression	object		*/
{
	int			isOkay = FALSE;			/* Return value of this function	*/
	char*	func="jsBoolExprDelete";	/* Name of this function			*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Free the boolean evaluation engine.
	----------------------------------- */
	free((UTL_BOOL_EVAL*)evalHandle);

/*	Indicate that everything went okay.
	----------------------------------- */
	isOkay = TRUE;

/*	Perform structured exception handling.
	-------------------------------------- */

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOkay);
}

/*
** Name:
**		jsBoolExprEvaluate
**
** Purpose:
**		This public function evaluates an input Boolean expression
**		object, using a set of input identifier strings and their
**		respective values.
**
** Calling Sequence:
**		isOkay = jsBoolExprEvaluate(	hLog,
**										hBoolExpr,
**										pExprString,
**										countIds,
**										ppIdStrings,
**										pIdValues,
**										pBoolResult	);
**
** Input:
**		hLog				handle to associated log object
**		hBoolExpr			handle to Boolean expression object
**		pExprString			ptr to the Boolean expression string
**		countIds			count of identifier strings and values
**		ppIdStrings			ptr to array of identifier string ptrs
**		pIdValues			ptr to array of identifier values
**
** Output:
**		pBoolResult			ptr to returned Boolean result of the
**							evaluation
**
** Returns:
**		isOkay				Boolean indicating whether or not
**							the function completed successfully.
**
** Error Conditions:
**		NONE
**
** Comments:
**		The algorithm used for this Boolean expression evaluation engine is
**		taken directly from the "Red Dragon" book, known formally as
**		"Compilers - Principles, Techniques, and Tools" by Alfred V. Aho,
**		Ravi Sethi, and Jeffrey D. Ullman.  In particular, a simple compiler
**		is described in Chapter 2 and the code is found at the end of the
**		chapter.  Any similarity between the following code and the code
**		found in this book is entirely deliberate, with only a few small
**		modifications that were made to adapt it to evaluating Boolean 
**		expressions.  Also, instead of using a symbol table, the caller of
**		this function provides a "symbol table" that is used to establish
**		the values of identifiers that are found in the Boolean expression.
*/
int jsBoolExprEvaluate(
		HANDLE	hLog,			/* handle to associated log object			*/
		HANDLE	hBoolExpr,		/* handle to Boolean expression object		*/
		char*	pExprString,	/* ptr to the Boolean expression string		*/
		int		countIds,		/* count of identifier strings and values	*/
		char**	ppIdStrings,	/* ptr to array of identifier strings ptrs	*/
		int*	pIdValues,		/* ptr to array of identifier values		*/
		int*	pBoolResult)	/* ptr to returned Boolean result			*/
{
	int				isOkay = FALSE;	/* return value of this function		*/
	UTL_BOOL_EVAL*	pBoolEval;		/* ptr to boolean evaluation context	*/
	char*	func="jsBoolExprEvaluate";		/* Name of this function		*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Save the input information for later reference in the boolean
	evaluation context data structure.
	------------------------------------------------------------- */
	pBoolEval = (UTL_BOOL_EVAL*)evalHandle;

	pBoolEval->pEvalString = pEvalString;
	pBoolEval->countIds    = countIds;
	pBoolEval->ppIdStrings = ppIdStrings;
	pBoolEval->pIdValues   = pIdValues;

/*	Initialize the working context information for this boolean evaluation
	data structure.
	---------------------------------------------------------------------- */
	pBoolEval->currCharPos   = 0;
	pBoolEval->theLookAhead  = DONE;
	pBoolEval->tokenValue    = 0;
	pBoolEval->theBoolStack  = 0;
	pBoolEval->currBoolValue = 0;
	memset(pBoolEval->lexBuff, 0, BSIZE);

/*	Initialize the context for the boolean expression evaluation.
	------------------------------------------------------------- */
	if ( !init_context(pBoolEval) ) {
		goto _errInitContext;
	}

/*	Parsee and evaluate the boolean expression evaluation.
	------------------------------------------------------ */
	if ( !parse(pBoolEval, pBoolResult) ) {
		goto _errParse;
	}

/*	Terminate the context for the boolean expression evaluation.
	------------------------------------------------------------ */
	if ( !term_context(pBoolEval) ) {
		goto _errTermContext;
	}

/*	Indicate that everything went okay.
	----------------------------------- */
	isOkay = TRUE;

/*	Perform structured exception handling.
	-------------------------------------- */
_errTermContext:
_errParse:
_errInitContext:

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOkay);
}

/*
** Name:
**		jsBoolExprSetDump
**
** Purpose:
**		This public function sets the dump Boolean stack flag, which applies
**		globally for ALL Boolean expression objects.
**
** Calling Sequence:
**		isOkay = jsBoolExprSetDump(hLog, fDumpStack);
**
** Input:
**		hLog				handle to associated log object
**		fDumpStack			dump Boolean stack flag
**
** Output:
**		NONE
**
** Returns:
**		isOkay				BOOLEAN indicating whether or not the
**							function executed successfully.
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int jsBoolExprSetDump(
		HANDLE	hLog,			/* handle to associated log object			*/
		int		fDumpStack)		/* dump Boolean stack flag					*/
{
	int					isOkay = FALSE;		/* return value of this func	*/
	char*	func="jsBoolExprSetDump";		/* Name of this function		*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Set the global dump boolean stack flag. 
	--------------------------------------- */
	m_DumpStack = fDumpStack;

/*	Indicate that everything went okay.
	----------------------------------- */
	isOkay = TRUE;

/*	Perform structured exception handling.
	-------------------------------------- */

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOkay);
}

/*
** Name:
**		emit
**
** Purpose:
**		Emits a token and its value for a boolean expression.
**
*/
static int emit(
	UTL_BOOL_EVAL*	pBoolEval,		/* ptr to boolean evaluation context	*/
	int				thisTokenChar,	/* the token character to emit			*/
	int				tokenValue)		/* the token value						*/
{
	int		isOkay = FALSE;		/* return value of this function	*/
	char*	func="emit";		/* name of this function			*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

	switch(thisTokenChar) {
		case '*':
			if ( !UtlBoolStack_And(pBoolEval->theBoolStack,
								   &pBoolEval->currBoolValue) ) {
				goto _errBoolStackAnd;
			}
			if (m_DumpStack) {
				UtlLog_Write(LOG_DEBUG | LOG_NO_ID, func, RIGHT_HERE,
				"*     , currBoolValue=%d\n", pBoolEval->currBoolValue);
			}
			break;
		case '+':
			if ( !UtlBoolStack_Or(pBoolEval->theBoolStack,
								  &pBoolEval->currBoolValue) ) {
				goto _errBoolStackOr;
			}
			if (m_DumpStack) {
				UtlLog_Write(LOG_DEBUG | LOG_NO_ID, func, RIGHT_HERE,
				"+     , currBoolValue=%d\n", pBoolEval->currBoolValue);
			}
			break;
		case '!':
			if ( !UtlBoolStack_Not(pBoolEval->theBoolStack,
								   &pBoolEval->currBoolValue) ) {
				goto _errBoolStackNot;
			}
			if (m_DumpStack) {
				UtlLog_Write(LOG_DEBUG | LOG_NO_ID, func, RIGHT_HERE,
				"!     , currBoolValue=%d\n", pBoolEval->currBoolValue);
			}
			break;
		case NUM:
			if ( !UtlBoolStack_Push(pBoolEval->theBoolStack,
									tokenValue) ) {
				goto _errBoolStackPush;
			}
			if (m_DumpStack) {
				UtlLog_Write(LOG_DEBUG | LOG_NO_ID, func, RIGHT_HERE,
				"PUSH %d\n", tokenValue);
			}
			break;
		case ID:
			if ( !UtlBoolStack_Push(pBoolEval->theBoolStack,
									tokenValue) ) {
				goto _errBoolStackPush;
			}
			if (m_DumpStack) {
				UtlLog_Write(LOG_DEBUG | LOG_NO_ID, func, RIGHT_HERE,
				"PUSH %d\n", tokenValue);
			}
			break;
		default:
			if (m_DumpStack) {
				UtlLog_Write(LOG_DEBUG | LOG_NO_ID, func, RIGHT_HERE,
				"thisTokenChar=%d, tokenValue=%d\n",
					thisTokenChar, tokenValue);
			}
			break;
	}

/*	Indicate that everything went okay.
	----------------------------------- */
	isOkay = TRUE;
	goto _allOkay;

/*	Perform structured exception handling.
	-------------------------------------- */
_errBoolStackPush:
_errBoolStackNot:
_errBoolStackOr:
_errBoolStackAnd:
	UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
		"ERROR: Boolean stack operation failed, character position = %d\n",
			pBoolEval->currCharPos - 1);

_allOkay:

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOkay);
}

/*
** Name:
**		expr
**
** Purpose:
**		Parses an "expression" for a boolean expression.
**
*/
static int expr(
	UTL_BOOL_EVAL*	pBoolEval)		/* ptr to boolean evaluation context	*/
{
	int		isOkay = FALSE;		/* return value of this function	*/
	int		thisTokenChar;		/* this token character				*/
	int		allDone = FALSE;	/* all done processing flag			*/
	char*	func="expr";		/* name of this function			*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	An "expr" consists of a "term" followed by an optional list of
	more "term"s that are separated by "+" (OR).
	-------------------------------------------------------------- */
	if ( !term(pBoolEval) ) {
		goto _errTerm;
	}
	while( !allDone ) {
		switch (pBoolEval->theLookAhead) {
			case '+':
				thisTokenChar = pBoolEval->theLookAhead;
				if ( !match(pBoolEval, pBoolEval->theLookAhead) ) {
					goto _errMatch;
				}
				if ( !term(pBoolEval) ) {
					goto _errTerm;
				}
				if ( !emit(pBoolEval, thisTokenChar, NONE) ) {
					goto _errEmit;
				}
				break;
			default:
				allDone = TRUE;
				break;
		}
	}

/*	Indicate that everything went okay.
	----------------------------------- */
	isOkay = TRUE;

/*	Perform structured exception handling.
	-------------------------------------- */
_errEmit:
_errTerm:
_errMatch:

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOkay);
}

/*
** Name:
**		factor
**
** Purpose:
**		Parses a "factor" for a boolean expression.
**
*/
static int factor(
	UTL_BOOL_EVAL*	pBoolEval)		/* ptr to boolean evaluation context	*/
{
	int		isOkay = FALSE;		/* return value of this function	*/
	char*	func="factor";		/* name of this function			*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	A "factor" consists of either an "expr" surrounded by "(" and ")"
	OR a "!factor" (NOT factor) OR a NUM (number) or and ID (identifier
	that has an associated value).
	------------------------------------------------------------------- */
	switch (pBoolEval->theLookAhead) {
		case '(':
			if ( !match(pBoolEval, '(') ) {
				goto _errMatch;
			}
			if ( !expr(pBoolEval) ) {
				goto _errExpr;
			}
			if ( !match(pBoolEval, ')') ) {
				goto _errMatch;
			}
			break;
		case '!':
			if ( !match(pBoolEval, '!') ) {
				goto _errMatch;
			}
			if ( !factor(pBoolEval) ) {
				goto _errFactor;
			}
			if ( !emit(pBoolEval, '!', pBoolEval->tokenValue) ) {
				goto _errEmit;
			}
			break;
		case NUM:
			if ( !emit(pBoolEval, NUM, pBoolEval->tokenValue) ) {
				goto _errEmit;
			}
			if ( !match(pBoolEval, NUM) ) {
				goto _errMatch;
			}
			break;
		case ID:
			if ( !emit(pBoolEval, ID, pBoolEval->tokenValue) ) {
				goto _errEmit;
			}
			if ( !match(pBoolEval, ID) ) {
				goto _errMatch;
			}
			break;
		default:
			UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
				"ERROR: Syntax error at character position %d, char=%c\n",
					pBoolEval->currCharPos - 1,
					*(pBoolEval->pEvalString + pBoolEval->currCharPos - 1));
			goto _errSyntax;
			break;
	}

/*	Indicate that everything went okay.
	----------------------------------- */
	isOkay = TRUE;

/*	Perform structured exception handling.
	-------------------------------------- */
_errSyntax:
_errMatch:
_errEmit:
_errFactor:
_errExpr:

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOkay);
}

/*
** Name:
**		init_context
**
** Purpose:
**		Initializes the context for a boolean expression.
**
*/
static int init_context(
	UTL_BOOL_EVAL*	pBoolEval)		/* ptr to boolean evaluation context	*/
{
	int		isOkay = FALSE;			/* return value of this function	*/
	char*	func="init_context";	/* name of this function			*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Create a boolean stack object to use as the stack engine for
	evaluating this boolean expression.
	------------------------------------------------------------ */
	if ( !UtlBoolStack_Create(&pBoolEval->theBoolStack) ) {
		goto _errBoolStackCreate;
	}

/*	Indicate that everything went okay.
	----------------------------------- */
	isOkay = TRUE;

/*	Perform structured exception handling.
	-------------------------------------- */
_errBoolStackCreate:

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOkay);
}

/*
** Name:
**		lexan
**
** Purpose:
**		Performs lexical analysis for a boolean expression.
**
*/
static int lexan(
	UTL_BOOL_EVAL*	pBoolEval,		/* ptr to boolean evaluation context	*/
	int*			pNextToken)		/* ptr to returned next token			*/
{
	int		isOkay = FALSE;		/* return value of this function	*/
	int		thisTokenChar;		/* this token character				*/
	char*	pCurrChar;			/* ptr to current character			*/
	int		ndx;				/* loop counter						*/
	int		gotToken = FALSE;	/* got next token flag				*/
	char*	func="lexan";		/* name of this function			*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Assume we are all done with the lexical analysis.
	------------------------------------------------- */
	*pNextToken = DONE;

/*	March down the evaluation string from the current character
	position until we have isolated the next token in the string.
	------------------------------------------------------------- */
	while ( !gotToken ) {

/*		Get the current character and then bump the current character
		position.
		------------------------------------------------------------- */
		pCurrChar = pBoolEval->pEvalString + pBoolEval->currCharPos;
		thisTokenChar = (int)(*pCurrChar);
		pBoolEval->currCharPos++;

/*		Skip over any white space.
		-------------------------- */
		if ( thisTokenChar == ' ' || thisTokenChar == '\t' ) {
			;
		} else if (thisTokenChar == '\n') {
			;

/*		If we found a digit then we want to convert this numeric
		token to its binary value.  Remember to verify that we
		have only found a single digit (either "0" or "1", in fact).
		------------------------------------------------------------ */
		} else if ( isdigit(thisTokenChar) ) {
			sscanf(pCurrChar, "%d", &pBoolEval->tokenValue);
			if ( (pBoolEval->tokenValue != 0) &&
				 (pBoolEval->tokenValue != 1) ) {
				UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
				"ERROR: Invalid constant %d at character position %d\n",
					pBoolEval->tokenValue, pBoolEval->currCharPos - 1);
				goto _errInvalidConstant;
			}
			*pNextToken = NUM;
			gotToken = TRUE;

/*		If we have found an alphabetic character, then this is (hopefully)
		an identifier, so extract out the entire identifier and then try
		to lookup its binary value.
		------------------------------------------------------------------ */
		} else if ( isalpha(thisTokenChar) ) {
			pBoolEval->currCharPos--;
			ndx = 0;
			while ( isalnum(thisTokenChar) ) {
				pBoolEval->lexBuff[ndx] = (char)thisTokenChar;;
				pBoolEval->currCharPos++;
				ndx++;
				if ( ndx >= BSIZE ) {
					UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
					"ERROR: Token at character position %d too big, max=%d\n",
						pBoolEval->currCharPos - 1, BSIZE - 1);
					*pNextToken = DONE;
					goto _errTokenTooBig;
				}
				pCurrChar = pBoolEval->pEvalString + pBoolEval->currCharPos;
				thisTokenChar = (int)(*pCurrChar);
			}
			pBoolEval->lexBuff[ndx] = EOS;
			if ( !lookup(pBoolEval, &pBoolEval->tokenValue) ) {
				goto _errLookup;
			}
			*pNextToken = ID;
			gotToken = TRUE;

/*		If we have found an EOS character then we have reached the end of
		this boolean expression.
		----------------------------------------------------------------- */
		} else if ( thisTokenChar == EOS ) {
			*pNextToken = DONE;
			gotToken = TRUE;

/*		There is really no other case that should be normally found, but
		if we find something else, just ignore it.
		---------------------------------------------------------------- */
		} else {
			pBoolEval->tokenValue = NONE;
			*pNextToken = thisTokenChar;
			gotToken = TRUE;
		}
	}

/*	Indicate that everything went okay.
	----------------------------------- */
	isOkay = TRUE;

/*	Perform structured exception handling.
	-------------------------------------- */
_errLookup:
_errTokenTooBig:
_errInvalidConstant:

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOkay);
}

/*
** Name:
**		lookup
**
** Purpose:
**		Looks up the value of the current token for a boolean expression.
**
*/
static int lookup(
	UTL_BOOL_EVAL*	pBoolEval,		/* ptr to boolean evaluation context	*/
	int*			pTokenValue)	/* ptr to returned token value			*/
{
	int		isOkay = FALSE;		/* return value of this function	*/
	int		tokenValue;			/* the value of the current token	*/
	int		gotMatch = FALSE;	/* got a match flag					*/
	int		thisId   = 0;		/* this identifier in the list		*/
	char*	func="lookup";		/* name of this function			*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Assume that nothing is found.
	----------------------------- */
	tokenValue = -1;

/*	Search through the input list of identifiers, looking for a match
	on the current token in this boolean expression.
	----------------------------------------------------------------- */
	while ( (!gotMatch) && (thisId < pBoolEval->countIds) ) {
		if ( strcmp( (*(pBoolEval->ppIdStrings + thisId)),
					 pBoolEval->lexBuff) == 0 ) {
			gotMatch = TRUE;
			*pTokenValue = *(pBoolEval->pIdValues + thisId);
		} else {
			thisId++;
		}
	}

/*	If no match was found, then we have a problem.
	---------------------------------------------- */
	if ( !gotMatch ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
		"ERROR: No match found for \"%s\" in id strings list, char pos=%d\n",
			pBoolEval->lexBuff, pBoolEval->currCharPos - 1);
		goto _errNoMatchFound;
	}

/*	Indicate that everything went okay.
	----------------------------------- */
	isOkay = TRUE;

/*	Perform structured exception handling.
	-------------------------------------- */
_errNoMatchFound:

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOkay);
}

/*
** Name:
**		match
**
** Purpose:
**		Matches a token for a boolean expression.
**
*/
static int match(
	UTL_BOOL_EVAL*	pBoolEval,		/* ptr to boolean evaluation context	*/
	int				thisTokenChar)	/* the token to match					*/
{
	int		isOkay = FALSE;		/* return value of this function	*/
	char*	func="match";		/* name of this function			*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	If the token to match is the same as the current lookahead
	token, we can go look for the next token.  Otherwise, we
	have a syntax error.
	---------------------------------------------------------- */
	if (pBoolEval->theLookAhead == thisTokenChar) {
		if ( !lexan(pBoolEval, &pBoolEval->theLookAhead) ) {
			goto _errLexan;
		}
	} else {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
		"ERROR: Syntax error at character position %d, expected a \"%c\"\n",
			pBoolEval->currCharPos - 1, thisTokenChar);
		goto _errSyntax;
	}

/*	Indicate that everything went okay.
	----------------------------------- */
	isOkay = TRUE;

/*	Perform structured exception handling.
	-------------------------------------- */
_errSyntax:
_errLexan:

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOkay);
}

/*
** Name:
**		parse
**
** Purpose:
**		Parses a boolean expression.
**
*/
static int parse(
	UTL_BOOL_EVAL*	pBoolEval,		/* ptr to boolean evaluation context	*/
	int*			pBoolResult)	/* ptr to returned boolean result		*/
{
	int		isOkay = FALSE;		/* return value of this function	*/
	char*	func="parse";		/* name of this function			*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	To parse the boolean expression, get a starting token from the
	lexical analyzer and then start recursively parsing the boolean
	expression until we have reached the end of the input string.
	--------------------------------------------------------------- */
	if ( !lexan(pBoolEval, &pBoolEval->theLookAhead) ) {
		goto _errLexan;
	}
	while (pBoolEval->theLookAhead != DONE) {
		if ( !expr(pBoolEval) ) {
			goto _errExpr;
		}

/*		The boolean expression has been completely parsed, so pop off
		the last node from our boolean stack engine to get the result
		of the boolean expression to return to the caller.
		------------------------------------------------------------- */
		if ( !UtlBoolStack_Pop(pBoolEval->theBoolStack,
							   &pBoolEval->currBoolValue) ) {
			goto _errBoolStackPop;
		}

		if (m_DumpStack) {
			UtlLog_Write(LOG_DEBUG | LOG_NO_ID, func, RIGHT_HERE,
				"RESULT, currBoolValue=%d\n", pBoolEval->currBoolValue);
		}
		*pBoolResult = pBoolEval->currBoolValue;
	}

/*	Indicate that everything went okay.
	----------------------------------- */
	isOkay = TRUE;

/*	Perform structured exception handling.
	-------------------------------------- */
_errBoolStackPop:
_errExpr:
_errLexan:

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOkay);
}

/*
** Name:
**		term
**
** Purpose:
**		Parses a "term" for a boolean expression.
**
*/
static int term(
	UTL_BOOL_EVAL*	pBoolEval)		/* ptr to boolean evaluation context	*/
{
	int		isOkay = FALSE;		/* return value of this function	*/
	int		thisTokenChar;		/* this token character				*/
	int		allDone = FALSE;	/* all done processing flag			*/
	char*	func="term";		/* name of this function			*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	A "term" consists of a "factor" followed by an optional list of
	more "factor"s that are separated by "*" (AND).
	--------------------------------------------------------------- */
	if ( !factor(pBoolEval) ) {
		goto _errFactor;
	}

	while ( !allDone ) {
		switch (pBoolEval->theLookAhead) {
			case '*':
				thisTokenChar = pBoolEval->theLookAhead;
				if ( !match(pBoolEval, pBoolEval->theLookAhead) ) {
					goto _errMatch;
				}
				if ( !factor(pBoolEval) ) {
					goto _errFactor;
				}
				if ( !emit(pBoolEval, thisTokenChar, NONE) ) {
					goto _errEmit;
				}
				continue;
			default:
				allDone = TRUE;
				break;
		}
	}

/*	Indicate that everything went okay.
	----------------------------------- */
	isOkay = TRUE;

/*	Perform structured exception handling.
	-------------------------------------- */
_errEmit:
_errFactor:
_errMatch:

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOkay);
}

/*
** Name:
**		term_context
**
** Purpose:
**		Terminates the context for a boolean expression.
**
*/
static int term_context(
	UTL_BOOL_EVAL*	pBoolEval)		/* ptr to boolean evaluation context	*/
{
	int		isOkay = FALSE;			/* return value of this function	*/
	char*	func="term_context";	/* name of this function			*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Delete the boolean stack object that was used as the stack engine for
	evaluating this boolean expression.
	--------------------------------------------------------------------- */
	if ( !UtlBoolStack_Delete(pBoolEval->theBoolStack) ) {
		goto _errBoolStackDelete;
	}

/*	Indicate that everything went okay.
	----------------------------------- */
	isOkay = TRUE;

/*	Perform structured exception handling.
	-------------------------------------- */
_errBoolStackDelete:

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOkay);
}
