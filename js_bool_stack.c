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
**		js_bool_stack.c
**
** Description:
**		This is the implementation file for the "js_bool_stack" module
**		of the JumpStart library, which is a collection of general-purpose
**		utility functions that are used by all other subsystems.  This module
**		contains library functions that create and manage a simple Boolean
**		push-down stack object.
**
** Subsystem:
**      JumpStart Library
**
** Version:
**      1.00
**
** Public Functions:
**		jsBoolStackAnd
**		jsBoolStackClear
**		jsBoolStackCreate
**		jsBoolStackDelete
**		jsBoolStackNot
**		jsBoolStackOr
**		jsBoolStackPop
**		jsBoolStackPush
**
** History:
**		Date		Engineer			Purpose
**		--------	----------------	---------------------------------
**		07/22/97	Steven M. Rosen		Initial creation.
*/

/* include files */
/* ------------- */
#include "js_mem.h"
#include "js_stack.h"
#include "js_bool_stack.h"

/* private definitions */
/* ------------------- */

/* private structures */
/* ------------------ */

/* private global variables */
/* ------------------------ */
int		m_BooleanTrue  = TRUE;				/* a Boolean TRUE value			*/
int		m_BooleanFalse = FALSE;				/* a Boolean FALSE value		*/

int*	m_pBooleanTrue  = &m_BooleanTrue;	/* ptr to a Boolean TRUE value	*/
int*	m_pBooleanFalse = &m_BooleanFalse;	/* ptr to a Boolean FALSE value	*/

/* private prototype declarations */
/* ------------------------------ */

/*
** Name:
**		jsBoolStackAnd
**
** Purpose:
**		This public function pops the top two nodes from a specified Boolean
**		stack object, performs an AND operation, and then pushes the result
**		back onto the specified Boolean stack object.  A copy of the result
**		of the AND operation is given back to the caller of this function to
**		show the current intermediate value of the Boolean stack object.
**
** Calling Sequence:
**		isOkay = jsBoolStackAnd(hLog, hBoolStack, pBoolValue);
**
** Input:
**		hLog				handle to associated log object
**		hBoolStack			handle to the Boolean stack object
**
** Output:
**		pBoolValue			ptr to returned copy of the value contained
**							in the top node of the specified Boolean
**							stack object
**
** Returns:
**		isOkay				Boolean indicating whether or not
**							the function completed successfully.
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int jsBoolStackAnd(
		HANDLE	hLog,		/* handle to associated log object				*/
		HANDLE	hBoolStack,	/* handle to the Boolean stack object			*/
		int*	pBoolValue)	/* ptr to returned copy of the value contained	*/
							/* in the top node of the specified Boolean		*/
							/* stack object									*/	
{
	int			isOkay = FALSE;		/* Return value of this function	*/
	int*		pBoolStackValue1;	/* ptr to boolean stack value 1		*/
	int*		pBoolStackValue2;	/* ptr to boolean stack value 2		*/
	char*	func="jsBoolStackAnd";	/* Name of this function		*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Pop the first value of the boolean stack object.
	------------------------------------------------ */
	if ( !UtlStack_Pop(stackHandle, (void*)&pBoolStackValue1) ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Could not pop first value off the boolean stack\n");
		goto _errPopStackValue1;
	}

/*	Pop the second value of the boolean stack object.
	------------------------------------------------- */
	if ( !UtlStack_Pop(stackHandle, (void*)&pBoolStackValue2) ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Could not pop second value off the boolean stack\n");
		goto _errPopStackValue2;
	}

/*	Perform an AND operation on the two boolean values.
	--------------------------------------------------- */
	*pBoolValue = (*pBoolStackValue1) & (*pBoolStackValue2);

/*	Push the results of the AND operation onto the boolean stack object.
	-------------------------------------------------------------------- */
	if ( !jsBoolStackPush(stackHandle, *pBoolValue) ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Could not push new value onto the boolean stack\n");
		goto _errPushStack;
	}

/*	Indicate that everything went okay.
	----------------------------------- */
	isOkay = TRUE;
	goto _allOkay;

/*	Perform structured exception handling.
	-------------------------------------- */
_errPushStack:
_errPopStackValue2:
_errPopStackValue1:
	jsBoolStackClear(stackHandle);

_allOkay:

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOkay);
}

/*
** Name:
**		jsBoolStackClear
**
** Purpose:
**		This public function clears an existing Boolean stack object.
**		(NOTE: Only the stack nodes themselves are deleted.  The client
**		data structures pointed to by the stack nodes are NOT deleted --
**		that is the client's responsibility.).
**
** Calling Sequence:
**		isOkay = jsBoolStackClear(hLog, hBoolStack);
**
** Input:
**		hLog				handle to associated log object
**		hBoolStack			handle to the Boolean stack object
**
** Output:
**		NONE
**
** Returns:
**		isOkay				Boolean indicating whether or not
**							the function completed successfully.
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int jsBoolStackClear(
		HANDLE	hLog,		/* handle to associated log object				*/
		HANDLE	hBoolStack)	/* handle to the Boolean stack object			*/
{
	int			isOkay = FALSE;		/* Return value of this function	*/
	char*	func="jsBoolStackClear";		/* Name of this function	*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Clear the boolean stack.
	------------------------ */
	if ( !UtlStack_Clear(stackHandle) ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Could not clear the boolean stack\n");
		goto _errClearStack;
	}

/*	Indicate that everything went okay.
	----------------------------------- */
	isOkay = TRUE;

/*	Perform structured exception handling.
	-------------------------------------- */
_errClearStack:

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOkay);
}

/*
** Name:
**		jsBoolStackCreate
**
** Purpose:
**		This public function creates a new (empty) Boolean stack object.
**
** Calling Sequence:
**		isOkay = jsBoolStackCreate(hLog, phBoolStack);
**
** Input:
**		hLog				handle to associated log object
**
** Output:
**		phBoolStack			ptr to returned handle to the new
**							Boolean stack object
**
** Returns:
**		isOkay				Boolean indicating whether or not
**							the function completed successfully.
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int jsBoolStackCreate(
		HANDLE	hLog,			/* handle to associated log object		*/
		HANDLE*	phBoolStack)	/* ptr to returned handle to the new	*/
								/* Boolean stack object					*/
{
	int			isOkay = FALSE;		/* Return value of this function	*/
	char*	func="jsBoolStackCreate";		/* Name of this function	*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Create the new boolean stack object by deriving it from the
	UtlStack "class".
	----------------------------------------------------------- */
	if ( !UtlStack_Create(pStackHandle) ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Could not create the new boolean stack\n");
		goto _errCreateStack;
	}

/*	Indicate that everything went okay.
	----------------------------------- */
	isOkay = TRUE;

/*	Perform structured exception handling.
	-------------------------------------- */
_errCreateStack:

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOkay);
}

/*
** Name:
**		jsBoolStackDelete
**
** Purpose:
**		This public function deletes an existing Boolean stack object.
**		(NOTE: Only the stack nodes themselves are deleted.  The client
**		data structures pointed to by the stack nodes are NOT deleted --
**		that is the client's responsibility.).
**
** Calling Sequence:
**		isOkay = jsBoolStackDelete(hLog, hBoolStack);
**
** Input:
**		hLog				handle to associated log object
**		hBoolStack			handle to the Boolean stack object
**
** Output:
**		NONE
**
** Returns:
**		isOkay				Boolean indicating whether or not
**							the function completed successfully.
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int jsBoolStackDelete(
		HANDLE	hLog,		/* handle to associated log object				*/
		HANDLE	hBoolStack)	/* handle to the Boolean stack object			*/
{
	int			isOkay = FALSE;		/* Return value of this function	*/
	char*	func="jsBoolStackDelete";		/* Name of this function	*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Delete the boolean stack object that was derived it from the
	UtlStack "class".
	------------------------------------------------------------ */
	if ( !UtlStack_Delete(stackHandle) ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Could not delete the boolean stack\n");
		goto _errDeleteStack;
	}

/*	Indicate that everything went okay.
	----------------------------------- */
	isOkay = TRUE;

/*	Perform structured exception handling.
	-------------------------------------- */
_errDeleteStack:

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOkay);
}

/*
** Name:
**		jsBoolStackNot
**
** Purpose:
**		This public function pops the top node from a specified Boolean
**		stack object, performs a NOT operation, and then pushes the result
**		back onto the specified Boolean stack object.  A copy of the result
**		of the NOT operation is given back to the caller of this function to
**		show the current intermediate value of the Boolean stack object.
**
** Calling Sequence:
**		isOkay = jsBoolStackNot(hLog, hBoolStack, pBoolValue);
**
** Input:
**		hLog				handle to associated log object
**		hBoolStack			handle to the Boolean stack object
**
** Output:
**		pBoolValue			ptr to returned copy of the value contained
**							in the top node of the specified Boolean
**							stack object
**
** Returns:
**		isOkay				Boolean indicating whether or not
**							the function completed successfully.
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int jsBoolStackNot(
		HANDLE	hLog,		/* handle to associated log object				*/
		HANDLE	hBoolStack,	/* handle to the Boolean stack object			*/
		int*	pBoolValue)	/* ptr to returned copy of the value contained	*/
							/* in the top node of the specified Boolean		*/
							/* stack object									*/	
{
	int			isOkay = FALSE;		/* Return value of this function	*/
	int*		pBoolStackValue;	/* ptr to boolean stack value 		*/
	char*	func="jsBoolStackNot";	/* Name of this function		*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Pop the top value of the boolean stack object.
	---------------------------------------------- */
	if ( !UtlStack_Pop(stackHandle, (void*)&pBoolStackValue) ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Could not pop top value off the boolean stack\n");
		goto _errPopStack;
	}

/*	Perform a NOT operation on this boolean value.
	---------------------------------------------- */
	if ( *pBoolStackValue == TRUE ) {
		*pBoolValue = FALSE;
	} else {
		*pBoolValue = TRUE;
	}

/*	Push the results of the NOT operation onto the boolean stack object.
	-------------------------------------------------------------------- */
	if ( !jsBoolStackPush(stackHandle, *pBoolValue) ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Could not push new value onto the boolean stack\n");
		goto _errPushStack;
	}

/*	Indicate that everything went okay.
	----------------------------------- */
	isOkay = TRUE;
	goto _allOkay;

/*	Perform structured exception handling.
	-------------------------------------- */
_errPushStack:
_errPopStack:
	jsBoolStackClear(stackHandle);

_allOkay:

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOkay);
}

/*
** Name:
**		jsBoolStackOr
**
** Purpose:
**		This public function pops the top two nodes from a specified Boolean
**		stack object, performs an OR operation, and then pushes the result
**		back onto the specified Boolean stack object.  A copy of the result
**		of the OR operation is given back to the caller of this function to
**		show the current intermediate value of the Boolean stack object.
**
** Calling Sequence:
**		isOkay = jsBoolStackOr(hLog, hBoolStack, pBoolValue);
**
** Input:
**		hLog				handle to associated log object
**		hBoolStack			handle to the Boolean stack object
**
** Output:
**		pBoolValue			ptr to returned copy of the value contained
**							in the top node of the specified Boolean
**							stack object
**
** Returns:
**		isOkay				Boolean indicating whether or not
**							the function completed successfully.
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int jsBoolStackOr(
		HANDLE	hLog,		/* handle to associated log object				*/
		HANDLE	hBoolStack,	/* handle to the Boolean stack object			*/
		int*	pBoolValue)	/* ptr to returned copy of the value contained	*/
							/* in the top node of the specified Boolean		*/
							/* stack object									*/	
{
	int			isOkay = FALSE;		/* Return value of this function	*/
	int*		pBoolStackValue1;	/* ptr to boolean stack value 1		*/
	int*		pBoolStackValue2;	/* ptr to boolean stack value 2		*/
	char*	func="jsBoolStackOr";		/* Name of this function		*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Pop the first value of the boolean stack object.
	------------------------------------------------ */
	if ( !UtlStack_Pop(stackHandle, (void*)&pBoolStackValue1) ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Could not pop first value off the boolean stack\n");
		goto _errPopStackValue1;
	}

/*	Pop the second value of the boolean stack object.
	------------------------------------------------- */
	if ( !UtlStack_Pop(stackHandle, (void*)&pBoolStackValue2) ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Could not pop second value off the boolean stack\n");
		goto _errPopStackValue2;
	}

/*	Perform an OR operation on the two boolean values.
	-------------------------------------------------- */
	*pBoolValue = (*pBoolStackValue1) | (*pBoolStackValue2);

/*	Push the results of the OR operation onto the boolean stack object.
	------------------------------------------------------------------- */
	if ( !jsBoolStackPush(stackHandle, *pBoolValue) ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Could not push new value onto the boolean stack\n");
		goto _errPushStack;
	}

/*	Indicate that everything went okay.
	----------------------------------- */
	isOkay = TRUE;
	goto _allOkay;

/*	Perform structured exception handling.
	-------------------------------------- */
_errPushStack:
_errPopStackValue2:
_errPopStackValue1:
	jsBoolStackClear(stackHandle);

_allOkay:

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOkay);
}

/*
** Name:
**		jsBoolStackPop
**
** Purpose:
**		This public function pops the top node from a specified Boolean
**		stack object and returns a copy of the value contained in that
**		top node.
**
** Calling Sequence:
**		isOkay = jsBoolStackPop(hLog, hBoolStack, pBoolValue);
**
** Input:
**		hLog				handle to associated log object
**		hBoolStack			handle to the Boolean stack object
**
** Output:
**		pBoolValue			ptr to returned copy of the value contained
**							in the top node of the specified Boolean
**							stack object
**
** Returns:
**		isOkay				Boolean indicating whether or not
**							the function completed successfully.
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int jsBoolStackPop(
		HANDLE	hLog,		/* handle to associated log object				*/
		HANDLE	hBoolStack,	/* handle to the Boolean stack object			*/
		int*	pBoolValue)	/* ptr to returned copy of the value contained	*/
							/* in the top node of the specified Boolean		*/
							/* stack object									*/	
{
	int			isOkay = FALSE;		/* Return value of this function	*/
	int*		pBoolStackValue;	/* ptr to boolean stack value		*/
	char*	func="jsBoolStackPop";	/* Name of this function		*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Pop the top of the boolean stack object.
	---------------------------------------- */
	if ( !UtlStack_Pop(stackHandle, (void*)&pBoolStackValue) ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Could not pop the top off the boolean stack\n");
		goto _errPopStack;
	}

/*	Compare the value of the returned pointer to decide what value to
	return to the caller.
	----------------------------------------------------------------- */
	*pBoolValue = *pBoolStackValue;

/*	Indicate that everything went okay.
	----------------------------------- */
	isOkay = TRUE;
	goto _allOkay;

/*	Perform structured exception handling.
	-------------------------------------- */
_errPopStack:
	jsBoolStackClear(stackHandle);

_allOkay:

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOkay);
}

/*
** Name:
**		jsBoolStackPush
**
** Purpose:
**		This public function pushes a specified Boolean value onto
**		a specified Boolean stack object.
**
** Calling Sequence:
**		isOkay = jsBoolStackPop(hLog, hBoolStack, pBoolValue);
**
** Input:
**		hLog				handle to associated log object
**		hBoolStack			handle to the Boolean stack object
**		boolValue			Boolean value to push onto the Boolean
**							stack object
**
** Output:
**		NONE
**
** Returns:
**		isOkay				Boolean indicating whether or not
**							the function completed successfully.
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int jsBoolStackPush(
		HANDLE	hLog,		/* handle to associated log object				*/
		HANDLE	hBoolStack,	/* handle to the Boolean stack object			*/
		int		boolValue)	/* Boolean value to push onto the Boolean		*/
							/* stack object									*/	
{
	int			isOkay = FALSE;		/* Return value of this function	*/
	int*		pBoolStackValue;	/* ptr to boolean stack value		*/
	char*	func="jsBoolStackPush";		/* Name of this function	*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Compare the value of the input boolean to decide what ptr to push
	onto the top of the boolean stack object.
	----------------------------------------------------------------- */
	if (boolValue == m_BooleanTrue) {
		pBoolStackValue = m_pBooleanTrue;
	} else if (boolValue == m_BooleanFalse) {
		pBoolStackValue = m_pBooleanFalse;
	} else {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Unknown boolean value to push = %d\n", boolValue);
		goto _errUnknownBooleanValue;
	}

/*	Push the ptr onto the top of the boolean stack object.
	------------------------------------------------------ */
	if ( !UtlStack_Push(stackHandle, (void*)pBoolStackValue) ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Could not push onto the top of the boolean stack\n");
		goto _errPushStack;
	}

/*	Indicate that everything went okay.
	----------------------------------- */
	isOkay = TRUE;
	goto _allOkay;

/*	Perform structured exception handling.
	-------------------------------------- */
_errPushStack:
_errUnknownBooleanValue:
	jsBoolStackClear(stackHandle);

_allOkay:

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOkay);
}
