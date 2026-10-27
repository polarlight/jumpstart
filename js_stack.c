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
**		utl_stack.c
**
** Description:
**		This is the implementation file for the "utl_stack" module of the
**		UTL subsystem, which is a collection of general-purpose utility 
**		functions that are used by all other subsystems.  This module
**		contains library functions that create and manage a simple
**		push-down stack.
**
** Subsystem:
**      UTL
**
** Version:
**      1.00
**
** Public Functions:
**		UtlStack_Clear
**		UtlStack_Create
**		UtlStack_Delete
**		UtlStack_Pop
**		UtlStack_Push
**
** History:
**		Date		Engineer			Purpose
**		--------	----------------	---------------------------------
**		06/30/97	Steven M. Rosen		Initial creation.
*/

/* include files */
/* ------------- */
#include "utl_dbg.h"
#include "utl_queue.h"
#include "utl_stack.h"

/* private definitions */
/* ------------------- */

/* private structures */
/* ------------------ */

/* private global variables */
/* ------------------------ */

/* private prototype declarations */
/* ------------------------------ */

/*
** Name:
**		UtlStack_Clear
**
** Purpose:
**		Clears an existing stack.  (NOTE: Only the stack nodes
**		themselves are deleted.  The client data structures pointed
**		to by the stack nodes are NOT deleted -- that is the client's
**		responsibility.)
**		
** Calling Sequence:
**		isOkay = UtlStack_Clear(stackHandle);
**
** Input:
**		stackHandle					Handle of the stack to be cleared.
**
** Output:
**		NONE
**
** Returns:
**		isOkay						BOOLEAN indicating whether or not
**									the function completed successfully.
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int UtlStack_Clear(
		int		stackHandle)	/* Stack handle								*/
{
	int			isOkay = FALSE;		/* Return value of this function	*/
	char*	func="UtlStack_Clear";	/* Name of this function			*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Free all the stack nodes.
	------------------------- */
	while (UtlQueue_DelHead(stackHandle)) {
		;
	}

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
**		UtlStack_Create
**
** Purpose:
**		Create a new (empty) stack.
**
** Calling Sequence:
**		isOkay = UtlStack_Create(&stackHandle);
**
** Input:
**		NONE
**
** Output:
**		&stackHandle		Ptr to storage that is to receive the
**							handle of the new stack.
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
int UtlStack_Create(
		int*	pStackHandle)	/* Ptr to storage to rcv stack handle		*/
{
	int			isOkay = FALSE;		/* Return value of this function	*/
	char*	func="UtlStack_Create";	/* Name of this function			*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Create the new stack object by deriving it from the UtlQueue "class".
	--------------------------------------------------------------------- */
	*pStackHandle = 0;
	if ( !UtlQueue_Create(pStackHandle) ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Could not create the new stack\n");
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
**		UtlStack_Delete
**
** Purpose:
**		Delete an existing stack.  (NOTE: Only the stack nodes
**		themselves are deleted.  The client data structures pointed
**		to by the stack nodes are NOT deleted -- that is the client's
**		responsibility.)
**		
** Calling Sequence:
**		isOkay = UtlStack_Delete(stackHandle);
**
** Input:
**		stackHandle					Handle of the stack to be deleted.
**
** Output:
**		NONE
**
** Returns:
**		isOkay						BOOLEAN indicating whether or not
**									the function completed successfully.
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int UtlStack_Delete(
		int		stackHandle)	/* Stack handle								*/
{
	int			isOkay = FALSE;		/* Return value of this function	*/
	char*	func="UtlStack_Delete";	/* Name of this function			*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Delete the stack object that was derived it from the UtlQueue "class".
	---------------------------------------------------------------------- */
	if ( !UtlQueue_Delete(stackHandle) ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Could not delete the stack\n");
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
**		UtlStack_Pop
**
** Purpose:
**		Pops the client data ptr contained in the top node of
**		the given stack.
**
** Calling Sequence:
**		isOkay = UtlStack_Pop(stackHandle, &pData);
**
** Input:
**		stackHandle					Handle to the stack of interest.
**
** Output:
**		&pData						Ptr to storage that is to receive
**									a copy of the client data ptr con-
**									tained in the top node of the
**									given stack.
**
** Returns:
**		isOkay						BOOLEAN indicating whether or not
**									the function completed successfully.
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int UtlStack_Pop(
		int		stackHandle,	/* Stack handle								*/
		void**	ppData)			/* Ptr to ptr to data popped off the stack	*/
{
	int			isOkay = FALSE;		/* Return value of this function	*/
	char*	func="UtlStack_Pop";	/* Name of this function			*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Pop the top of the stack object by getting the head of the underlying
	queue object and then deleting current head of this queue object.
	---------------------------------------------------------------------- */
	if ( !UtlQueue_GetHead(stackHandle, ppData) ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Could not pop the top off the stack\n");
		goto _errGetTopOfStack;
	}
	if ( !UtlQueue_DelHead(stackHandle) ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Could not delete the top of the stack\n");
		goto _errDeleteTopOfStack;
	}

/*	Indicate that everything went okay.
	----------------------------------- */
	isOkay = TRUE;

/*	Perform structured exception handling.
	-------------------------------------- */
_errDeleteTopOfStack:
_errGetTopOfStack:

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOkay);
}

/*
** Name:
**		UtlStack_Push
**
** Purpose:
**		To the top of the given stack, pushes a node containing (i.e.
**		pointing to) the given client data.
**
** Calling Sequence:
**		isOkay = UtlStack_Push(stackHandle, pData);
**
** Input:
**		stackHandle			Stack handle
**		pData				Ptr to data to push onto the stack.
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
int UtlStack_Push(
		int		stackHandle,	/* Stack handle								*/
		void*	pData)			/* Ptr to data to push onto the stack		*/
{
	int			isOkay = FALSE;		/* Return value of this function	*/
	char*	func="UtlStack_Push";	/* Name of this function			*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Push the data onto the top of the stack object by adding it to the head
	of the underlying queue object.
	----------------------------------------------------------------------- */
	if ( !UtlQueue_AddHead(stackHandle, pData) ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Could not push onto the top of the stack\n");
		goto _errPushTopOfStack;
	}

/*	Indicate that everything went okay.
	----------------------------------- */
	isOkay = TRUE;

/*	Perform structured exception handling.
	-------------------------------------- */
_errPushTopOfStack:

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOkay);
}
