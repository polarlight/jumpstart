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
**		utl_timer_queue.c
**
** Description:
**		This is the implementation file for the "utl_timer_queue" module of
**		the UTL subsystem, which is a collection of general-purpose utility 
**		functions that are used by all other subsystems.  This module
**		contains library functions that create and manage a timer queue.
**
** Subsystem:
**      UTL
**
** Version:
**      1.00
**
** Public Functions:
**		UtlTimerQueue_Add
**		UtlTimerQueue_Create
**		UtlTimerQueue_Decrement
**		UtlTimerQueue_Delete
**
** History:
**		Date		Engineer			Purpose
**		--------	----------------	---------------------------------
**		07/02/97	Steven M. Rosen		Initial creation.
*/

/* include files */
/* ------------- */
#include "utl_dbg.h"
#include "utl_queue.h"
#include "utl_timer_queue.h"

/* private definitions */
/* ------------------- */

/* private structures */
/* ------------------ */
typedef struct {
	int		timeUnits;			/* number of time units left for this node	*/
	void*	pData;				/* ptr to the associated user data			*/
} UTL_TIMER_QUEUE;

/* private global variables */
/* ------------------------ */
static int	m_DumpQueue = FALSE;		/* dump timer queue flag			*/

/* private prototype declarations */
/* ------------------------------ */
static int dump_timer_queue(
		int		queueHandle);	/* timer queue handle						*/

/*
** Name:
**		UtlTimerQueue_Add
**
** Purpose:
**		This public function adds a new node to a specified timer queue
**		with a specified number of time units that represent the "time
**		out" value for the new node.
**
** Calling Sequence:
**		isOkay = UtlTimerQueue_Add(queueHandle, timeUnits, pData);
**
** Input:
**		queueHandle			timer queue handle
**		timeUnits			number of time units for added node
**		pData				ptr to user data to add onto timer queue
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
int UtlTimerQueue_Add(
		int		queueHandle,	/* timer queue handle						*/
		int		timeUnits,		/* number of time units for added node		*/
		void*	pData)			/* ptr to user data to add onto timer queue	*/
{
	int					isOkay = FALSE;	/* return value of this func	*/
	UTL_TIMER_QUEUE*	pNewTimerQueue;	/* ptr to new timer queue node	*/
	UTL_TIMER_QUEUE*	pTimerQueue;	/* ptr to a timer queue node	*/
	int					addedNewNode;	/* added new node flag			*/
	int					gotNode;		/* got another queue node flag	*/
	int					gotHead;		/* got a timer queue head		*/
	int					totalTimeUnits;	/* running total of time units	*/
	char*	func="UtlTimerQueue_Add";	/* Name of this function		*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Allocate a timer queue node data structure. 
	------------------------------------------- */
	pNewTimerQueue = (UTL_TIMER_QUEUE*)malloc(sizeof(UTL_TIMER_QUEUE));
	if (pNewTimerQueue == NULL) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"Could not allocate space for new timer queue node\n");
		goto _errAllocateMemory;
	}

/*	Initialize this timer queue node with the input information.
	------------------------------------------------------------ */
	pNewTimerQueue->timeUnits = timeUnits;
	pNewTimerQueue->pData     = pData;

/*	Insert this time queue node into its proper place in the timer
	queue.  This is done by walking through the queue, looking for
	the two nodes that it will sit on either side of this node.
	Remember that any node in the timer queue has a "relative" time
	unit that relates it to the node just in front of it (in time).
	A special case is made for the case where the queue is now empty.
	----------------------------------------------------------------- */
	addedNewNode   = FALSE;
	totalTimeUnits = 0;

/*	Start walking through the queue (starting from the head, of
	course) and keep track of the running total of time units.
	As soon as we reach a running total that exceeds our new timer
	queue node, we need to add the new timer queue node at this
	spot in the timer queue and adjust the time units for both the
	new timer queue node and the timer queue node that is directly
	after the new timer queue node.
	--------------------------------------------------------------- */
	gotHead = gotNode = UtlQueue_GetHead(queueHandle, (void*)&pTimerQueue);
    while ( gotNode && (!addedNewNode) ) {
		totalTimeUnits += pTimerQueue->timeUnits;

		if (totalTimeUnits >= timeUnits) {
			pNewTimerQueue->timeUnits = timeUnits -
							(totalTimeUnits - pTimerQueue->timeUnits);
			pTimerQueue->timeUnits -= pNewTimerQueue->timeUnits;

			if ( !UtlQueue_AddBeforeCurr(queueHandle,
										 (void*)pNewTimerQueue) ) {
				goto _errAddBeforeCurr;
			}
			addedNewNode = TRUE;
		} else {
			gotNode = UtlQueue_GetNext(queueHandle, (void*)&pTimerQueue);
		}
    }

/*	If we did not add a new node, there are two possibilities: One,
	the timer queue is empty (no head), which means we can just add
	the new timer queue node to the timer queue as the new head.  The
	second possibility is that we have run through the entire timer
	queue (i.e., there IS at least a head) and the total time units
	never exceeded our new time units value.  For this case, we can
	add the new timer queue node as the tail of our timer queue, but
	we need to adjust its time units by the total time units that we
	have accumulated so far (remember, everything is stored in the
	timer queue as relative offsets from each other).
	----------------------------------------------------------------- */
	if ( !addedNewNode ) {
		if ( !gotHead ) {
			if ( !UtlQueue_AddHead(queueHandle, pNewTimerQueue) ) {
				UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
					"ERROR: Could not add the head of the timer queue\n");
				goto _errAddHead;
			}
		} else {
			pNewTimerQueue->timeUnits -= totalTimeUnits;
			if ( !UtlQueue_AddTail(queueHandle, pNewTimerQueue) ) {
				UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
					"ERROR: Could not add the tail of the timer queue\n");
				goto _errAddTail;
			}
		}
	}

/*	For debugging, dump the modified timer queue.
	--------------------------------------------- */
	dump_timer_queue(queueHandle);

/*	Indicate that everything went okay.
	----------------------------------- */
	isOkay = TRUE;

/*	Perform structured exception handling.
	-------------------------------------- */
_errAddTail:
_errAddHead:
_errAddBeforeCurr:
_errAllocateMemory:

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOkay);
}

/*
** Name:
**		UtlTimerQueue_Create
**
** Purpose:
**		Create a new (empty) timer queue.
**
** Calling Sequence:
**		isOkay = UtlTimerQueue_Create(pQueueHandle);
**
** Input:
**		NONE
**
** Output:
**		pQueueHandle		ptr to storage that is to receive the
**							handle of the new timer queue.
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
int UtlTimerQueue_Create(
		int*	pQueueHandle)	/* ptr to storage to rcv timer queue handle	*/
{
	int			isOkay = FALSE;			/* return value of this function	*/
	char*	func="UtlTimerQueue_Create";	/* name of this function		*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Create the new timer queue object by deriving it from the UtlQueue
	"class".
	------------------------------------------------------------------ */
	*pQueueHandle = 0;
	if ( !UtlQueue_Create(pQueueHandle) ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Could not create the new timer queue\n");
		goto _errCreateTimerQueue;
	}

/*	Indicate that everything went okay.
	----------------------------------- */
	isOkay = TRUE;

/*	Perform structured exception handling.
	-------------------------------------- */
_errCreateTimerQueue:

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOkay);
}

/*
** Name:
**		UtlTimerQueue_Decrement
**
** Purpose:
**		This public function decrements a specified timer queue by
**		the specified number of time units.  Any nodes of the timer
**		queue that have "timed out" as a result of the decrement
**		operation will be removed from the timer queue.  A list of
**		pointers to the user data for each "timed out" node is 
**		returned to the caller.
**
** Calling Sequence:
**		isOkay = UtlTimerQueue_Decrement(queueHandle, timeUnits,
**										 pNumDataPtrs, ppData);
**
** Input:
**		queueHandle			timer queue handle
**		timeUnits			number of time units to decrement queue
**
** Output:
**		pNumDataPtrs		ptr to returned number of user data ptrs
**		ppData				ptr to storage that is to receive a list
**							of user data ptrs, one for each "timed out"
**							node
**
** Returns:
**		isOkay				BOOLEAN indicating whether or not
**							the function completed successfully.
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int UtlTimerQueue_Decrement(
		int		queueHandle,	/* timer queue handle						*/
		int		timeUnits,		/* number of time units to decrement queue	*/
		int*	pNumDataPtrs,	/* ptr to returned number of user data ptrs	*/
		void**	ppData)			/* ptr to user data ptrs that have timed out*/
{
	int					isOkay = FALSE;		/* return value of this func	*/
	UTL_TIMER_QUEUE*	pTimerQueue;		/* ptr to a timer queue node	*/
	int					gotNode;			/* got another queue node flag	*/
	int					origTimeUnits;		/* the original time units		*/
	int					totalTimeUnits;		/* running total of time units	*/
	int					nDataPtrs;			/* the number of user data ptrs	*/
	int**				ppDataPtrs;			/* ptr to user data ptrs		*/
	char*	func="UtlTimerQueue_Decrement";	/* Name of this function		*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Assume that no user data ptrs are returned.
	------------------------------------------- */
	*pNumDataPtrs = 0;
	nDataPtrs = 0;

/*	Look at the head of the timer queue (if it exists) and remove
	that node if the running total of time units is less than or 
	equal to the input decrement time units.  If a head node does
	not need to be removed, then decrement its time unit by the
	amount specified by the caller.
	------------------------------------------------------------- */
	totalTimeUnits = timeUnits;

	gotNode = UtlQueue_GetHead(queueHandle, (void*)&pTimerQueue);
    while ( gotNode && (totalTimeUnits > 0) ) {

		origTimeUnits = pTimerQueue->timeUnits;
		if (pTimerQueue->timeUnits <= totalTimeUnits) {
			pTimerQueue->timeUnits = 0;
		} else {
			pTimerQueue->timeUnits -= totalTimeUnits;
		}
		totalTimeUnits -= origTimeUnits;

		if (pTimerQueue->timeUnits <= 0) {

			nDataPtrs++;
			if (nDataPtrs == 1) {
				ppDataPtrs = (int**)malloc(sizeof(int**));
			} else {
				ppDataPtrs = (int**)realloc(ppDataPtrs,
											sizeof(int**) * nDataPtrs);
			}
			if ( ppDataPtrs == NULL ) {
				UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
					"ERROR: Could not allocate memory for user data ptrs\n");
				goto _errAllocateMemory;
			}
			*(ppDataPtrs + nDataPtrs - 1) = (int*)pTimerQueue->pData;
			*ppData = (void**)ppDataPtrs;

			free(pTimerQueue);
			if ( !UtlQueue_DelHead(queueHandle) ) {
				goto _errDelHead;
			}
		}

		gotNode = UtlQueue_GetHead(queueHandle, (void*)&pTimerQueue);
    }
	*pNumDataPtrs = nDataPtrs;

/*	For debugging, dump the modified timer queue.
	--------------------------------------------- */
	dump_timer_queue(queueHandle);

/*	Indicate that everything went okay.
	----------------------------------- */
	isOkay = TRUE;

/*	Perform structured exception handling.
	-------------------------------------- */
_errDelHead:
_errAllocateMemory:

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOkay);
}

/*
** Name:
**		UtlTimerQueue_Delete
**
** Purpose:
**		Delete an existing timer queue.  (NOTE: Only the queue nodes
**		themselves are deleted.  The client data structures pointed
**		to by the queue nodes are NOT deleted -- that is the client's
**		responsibility.)
**		
** Calling Sequence:
**		isOkay = UtlTimerQueue_Delete(queueHandle);
**
** Input:
**		queueHandle			Handle of the timer queue to be deleted.
**
** Output:
**		NONE
**
** Returns:
**		isOkay				BOOLEAN indicating whether or not
**							the function completed successfully.
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int UtlTimerQueue_Delete(
		int		queueHandle)	/* timer queue handle						*/
{
	int					isOkay = FALSE;	/* Return value of this function	*/
	UTL_TIMER_QUEUE*	pTimerQueue;	/* ptr to a timer queue node		*/
	int					gotNode;		/* got another queue node flag		*/
	char*	func="UtlTimerQueue_Delete";	/* Name of this function		*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Free any remaining timer queue nodes in the timer queue.
	-------------------------------------------------------- */
	gotNode = UtlQueue_GetHead(queueHandle, (void*)&pTimerQueue);
    while ( gotNode ) {
		free(pTimerQueue);
		gotNode = UtlQueue_GetNext(queueHandle, (void*)&pTimerQueue);
    }

/*	Delete the timer queue object that was derived it from the UtlQueue
	"class".
	------------------------------------------------------------------- */
	if ( !UtlQueue_Delete(queueHandle) ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Could not delete the timer queue\n");
		goto _errDeleteTimerQueue;
	}

/*	Indicate that everything went okay.
	----------------------------------- */
	isOkay = TRUE;

/*	Perform structured exception handling.
	-------------------------------------- */
_errDeleteTimerQueue:

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOkay);
}

/*
** Name:
**		UtlTimerQueue_SetDump
**
** Purpose:
**		This public function sets the dump timer queue flag, which applies
**		globally for ALL timer queues.
**
** Calling Sequence:
**		isOkay = UtlTimerQueue_SetDump(fDumpQueue);
**
** Input:
**		fDumpQueue			dump timer queue flag
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
int UtlTimerQueue_SetDump(
		int		fDumpQueue)		/* dump timer queue flag					*/
{
	int					isOkay = FALSE;		/* return value of this func	*/
	char*	func="UtlTimerQueue_SetDump";	/* Name of this function		*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Set the global dump timer queue flag. 
	------------------------------------- */
	m_DumpQueue = fDumpQueue;

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
**		dump_timer_queue
**
** Purpose:
**		Dumps a specified timer queue.
**
*/
static int dump_timer_queue(
		int		queueHandle)	/* timer queue handle						*/
{
	int					isOkay = FALSE;	/* return value of this func	*/
	UTL_TIMER_QUEUE*	pTimerQueue;	/* ptr to a timer queue node	*/
	int					gotNode;		/* got another queue node flag	*/
	char*	func="dump_timer_queue";	/* name of this function		*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Dump out the entire timer queue, showing the time units for each node,
	but only if the global dump timer queue flag is set.
	---------------------------------------------------------------------- */
	if (m_DumpQueue) {
		UtlLog_Write(LOG_DEBUG, func, RIGHT_HERE,
			"Dumping timer queue:\n");

		gotNode = UtlQueue_GetHead(queueHandle, (void*)&pTimerQueue);
		while ( gotNode ) {
			UtlLog_Write(LOG_DEBUG | LOG_NO_ID, func, RIGHT_HERE,
				"	for pTimerQueue=%p, userData=%d, timeUnits=%d\n",
					pTimerQueue, (int)pTimerQueue->pData,
					pTimerQueue->timeUnits);
			gotNode = UtlQueue_GetNext(queueHandle, (void*)&pTimerQueue);
		}
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
