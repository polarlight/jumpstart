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
**		utl_queue.c
**
** Description:
**		This is the implementation file for the "utl_queue" module of the
**		UTL subsystem, which is a collection of general-purpose utility 
**		functions that are used by all other subsystems.  This module
**		contains library functions that create and manage a doubly-
**		linked queue.
**
** Subsystem:
**      UTL
**
** Version:
**      1.00
**
** Public Functions:
**		UtlQueue_AddAfterCurr
**		UtlQueue_AddBeforeCurr
**		UtlQueue_AddHead
**		UtlQueue_AddTail
**		UtlQueue_Create
**		UtlQueue_Delete
**		UtlQueue_DelCurr
**		UtlQueue_DelHead
**		UtlQueue_DelTail
**		UtlQueue_GetHead
**		UtlQueue_GetNext
**		UtlQueue_GetPrev
**		UtlQueue_GetTail
**
** History:
**		Date		Engineer			Purpose
**		--------	----------------	---------------------------------
**		08/22/96	Richard D. Klein	Initial creation.
**		07/02/97	Steven M. Rosen		Added UtlQueue_AddAfterCurr,
**										UtlQueue_AddBeforeCurr, and
**										UtlQueue_GetPrev.
*/

/* include files */
/* ------------- */
#include "utl_dbg.h"
#include "utl_queue.h"

/* private definitions */
/* ------------------- */

/**********************/
/* private structures */
/**********************/

/*	Queue node data structure.
	-------------------------- */
typedef struct node {
	struct node*		pNext;			/* Ptr to next queue node			*/
	struct node*		pPrev;			/* Ptr to previous queue node		*/
	void*				pData;			/* Ptr to client data				*/
} UTL_QUEUE_NODE;

/*	Queue data structure.
	--------------------- */
typedef struct {
	UTL_QUEUE_NODE*		pHead;			/* Ptr to head						*/
	UTL_QUEUE_NODE*		pTail;			/* Ptr to tail						*/
	UTL_QUEUE_NODE*		pCurr;			/* Ptr to current node				*/
} UTL_QUEUE;

/* private global variables */
/* ------------------------ */

/* private prototype declarations */
/* ------------------------------ */

/*
** Name:
**		UtlQueue_AddAfterCurr
**
** Purpose:
**		To the given queue, add a node after the current node
**		containing (i.e. pointing to) the given client data.
**
** Calling Sequence:
**		isOk = UtlQueue_AddAfterCurr(queueHandle, pData);
**
** Input:
**		queueHandle			Handle of the queue affected.
**		pData				Ptr to the client data to be added.
**
** Output:
**		NONE
**
** Returns:
**		isOk				BOOLEAN indicating whether or not the
**							function executed successfully.
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int UtlQueue_AddAfterCurr(
		int		queueHandle,		/* Queue handle							*/
		void*	pData)				/* Ptr to data to add after queue curr	*/
{
	int		isOk = TRUE;			/* Return value of this function		*/
	UTL_QUEUE_NODE*
			pNewNode = NULL;		/* Ptr to new head node of queue		*/
	UTL_QUEUE*
			pQueue = NULL;			/* Queue handle recast as a queue ptr	*/
	char*	func = "UtlQueue_AddAfterCurr";
									/* Name of this function				*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	(If no node is current and we have at least one node in the
	queue, just return with an error.)
	----------------------------------------------------------- */
	pQueue = (UTL_QUEUE*)queueHandle;
	if ( (pQueue->pCurr == NULL) && (pQueue->pHead != NULL) )
	{
		UtlLog_Write(LOG_TRACE, func, RIGHT_HERE,
			"Can't add after the current node -- no node is current!\n");
		isOk = FALSE;
		goto _errNoCleanup;
	}

/*	If the queue is currently empty, then add as the new tail.
	---------------------------------------------------------- */
	if (pQueue->pTail == NULL)
		UtlQueue_AddTail(queueHandle, pData);

/*	If the current node is the head, then add after the current tail.
	----------------------------------------------------------------- */
	else if (pQueue->pCurr == pQueue->pTail)
		UtlQueue_AddTail(queueHandle, pData);

/*	Else (the queue contains at least two nodes, and the current
	node is NOT the tail node)...
	------------------------------------------------------------ */
	else
	{
/*		Allocate a new queue node.  Make its data ptr point to the
		client's data.  Make its prev ptr point to the current node.
		Make its next ptr point to the current node's next ptr.
		------------------------------------------------------------ */
		pNewNode = (UTL_QUEUE_NODE*)malloc(sizeof(UTL_QUEUE_NODE));
		if (pNewNode == NULL)
		{
			UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
				"Unable to allocate new queue node\n");
			isOk = FALSE;
			goto _errNoCleanup;
		}
		pNewNode->pData = pData;
		pNewNode->pPrev = pQueue->pCurr;
		pNewNode->pNext = pQueue->pCurr->pNext;

/*		Make the current node's next's prev ptr point to the new queue
		node and the current node's next ptr point to the new queue node.
		----------------------------------------------------------------- */
		pQueue->pCurr->pNext->pPrev = pNewNode;
		pQueue->pCurr->pNext        = pNewNode;

/*		Set the queue's curr ptr to NULL.
		--------------------------------- */
		pQueue->pCurr = NULL;
	}

_errNoCleanup:

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

	return isOk;
}

/*
** Name:
**		UtlQueue_AddBeforeCurr
**
** Purpose:
**		To the given queue, add a node before the current node
**		containing (i.e. pointing to) the given client data.
**
** Calling Sequence:
**		isOk = UtlQueue_AddBeforeCurr(queueHandle, pData);
**
** Input:
**		queueHandle			Handle of the queue affected.
**		pData				Ptr to the client data to be added.
**
** Output:
**		NONE
**
** Returns:
**		isOk				BOOLEAN indicating whether or not the
**							function executed successfully.
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int UtlQueue_AddBeforeCurr(
		int		queueHandle,		/* Queue handle							*/
		void*	pData)				/* Ptr to data to add before queue curr	*/
{
	int		isOk = TRUE;			/* Return value of this function		*/
	UTL_QUEUE_NODE*
			pNewNode = NULL;		/* Ptr to new head node of queue		*/
	UTL_QUEUE*
			pQueue = NULL;			/* Queue handle recast as a queue ptr	*/
	char*	func = "UtlQueue_AddBeforeCurr";
									/* Name of this function				*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	(If no node is current and we have at least one node in the
	queue, just return with an error.)
	----------------------------------------------------------- */
	pQueue = (UTL_QUEUE*)queueHandle;
	if ( (pQueue->pCurr == NULL) && (pQueue->pHead != NULL) )
	{
		UtlLog_Write(LOG_TRACE, func, RIGHT_HERE,
			"Can't add before the current node -- no node is current!\n");
		isOk = FALSE;
		goto _errNoCleanup;
	}

/*	If the queue is currently empty, then add as the new head.
	---------------------------------------------------------- */
	if (pQueue->pHead == NULL)
		UtlQueue_AddHead(queueHandle, pData);

/*	If the current node is the head, then add before the current head.
	------------------------------------------------------------------ */
	else if (pQueue->pCurr == pQueue->pHead)
		UtlQueue_AddHead(queueHandle, pData);

/*	Else (the queue contains at least two nodes, and the current
	node is NOT the head node)...
	------------------------------------------------------------ */
	else
	{
/*		Allocate a new queue node.  Make its data ptr point to the
		client's data.  Make its next ptr point to the current node.
		Make its prev ptr point to the current node's prev ptr.
		------------------------------------------------------------ */
		pNewNode = (UTL_QUEUE_NODE*)malloc(sizeof(UTL_QUEUE_NODE));
		if (pNewNode == NULL)
		{
			UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
				"Unable to allocate new queue node\n");
			isOk = FALSE;
			goto _errNoCleanup;
		}
		pNewNode->pData = pData;
		pNewNode->pNext = pQueue->pCurr;
		pNewNode->pPrev = pQueue->pCurr->pPrev;

/*		Make the current node's prev's next ptr point to the new queue
		node and the current node's prev ptr point to the new queue node.
		----------------------------------------------------------------- */
		pQueue->pCurr->pPrev->pNext = pNewNode;
		pQueue->pCurr->pPrev        = pNewNode;

/*		Set the queue's curr ptr to NULL.
		--------------------------------- */
		pQueue->pCurr = NULL;
	}

_errNoCleanup:

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

	return isOk;
}

/*
** Name:
**		UtlQueue_AddHead
**
** Purpose:
**		To the head of the given queue, add a node containing (i.e.
**		pointing to) the given client data.
**
** Calling Sequence:
**		isOk = UtlQueue_AddHead(queueHandle, pData);
**
** Input:
**		queueHandle			Handle of the queue affected.
**		pData				Ptr to the client data to be added.
**
** Output:
**		NONE
**
** Returns:
**		isOk				BOOLEAN indicating whether or not the
**							function executed successfully.
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int UtlQueue_AddHead(
		int		queueHandle,		/* Queue handle							*/
		void*	pData)				/* Ptr to data to add to queue head		*/
{
	int		isOk = TRUE;			/* Return value of this function		*/
	UTL_QUEUE_NODE*
			pNewNode = NULL;		/* Ptr to new head node of queue		*/
	UTL_QUEUE*
			pQueue = NULL;			/* Queue handle recast as a queue ptr	*/
	char*	func = "UtlQueue_AddHead";
									/* Name of this function				*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Set the queue's curr node ptr to NULL.
	-------------------------------------- */
	pQueue = (UTL_QUEUE*)queueHandle;
	pQueue->pCurr = NULL;

/*	Allocate a new queue node.  Make its data ptr point to the client's
	data.  Make its next ptr point to the old head.  Make its prev ptr
	NULL.
	------------------------------------------------------------------- */
	pNewNode = (UTL_QUEUE_NODE*)malloc(sizeof(UTL_QUEUE_NODE));
	if (pNewNode == NULL)
	{
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"Unable to allocate new queue node\n");
		isOk = FALSE;
		goto _errNoCleanup;
	}
	pNewNode->pData = pData;
	pNewNode->pNext = pQueue->pHead;
	pNewNode->pPrev = NULL;

/*	Make the old head's prev ptr point to the new node.
	--------------------------------------------------- */
	if (pQueue->pHead)
		pQueue->pHead->pPrev = pNewNode;

/*	Make the queue's head ptr point to the new node.
	------------------------------------------------ */
	pQueue->pHead = pNewNode;

/*	If the queue's tail ptr is currently NULL (i.e. if the
	queue was empty before this function was called), then
	make it point to the new node.
	------------------------------------------------------ */
	if (pQueue->pTail == NULL)
		pQueue->pTail = pNewNode;

_errNoCleanup:

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

	return isOk;
}

/*
** Name:
**		UtlQueue_AddTail
**
** Purpose:
**		To the tail of the given queue, add a node containing (i.e.
**		pointing to) the given client data.
**
** Calling Sequence:
**		isOk = UtlQueue_AddTail(queueHandle, pData);
**
** Input:
**		queueHandle			Handle of the queue affected.
**		pData				Ptr to the client data to be added.
**
** Output:
**		NONE
**
** Returns:
**		isOk				BOOLEAN indicating whether or not the
**							function executed successfully.
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int UtlQueue_AddTail(
		int		queueHandle,		/* Queue handle							*/
		void*	pData)				/* Ptr to data to add to queue tail		*/
{
	int		isOk = TRUE;			/* Return value of this function		*/
	UTL_QUEUE_NODE*
			pNewNode = NULL;		/* Ptr to new tail node of queue		*/
	UTL_QUEUE*
			pQueue = NULL;			/* Queue handle recast as a queue ptr	*/
	char*	func = "UtlQueue_AddTail";
									/* Name of this function				*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Set the queue's curr node ptr to NULL.
	-------------------------------------- */
	pQueue = (UTL_QUEUE*)queueHandle;
	pQueue->pCurr = NULL;

/*	Allocate a new queue node.  Make its data ptr point to the client's
	data.  Make its next ptr NULL.  Make its prev ptr point to the old
	tail.
	------------------------------------------------------------------- */
	pNewNode = (UTL_QUEUE_NODE*)malloc(sizeof(UTL_QUEUE_NODE));
	if (pNewNode == NULL)
	{
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"Unable to allocate new queue node\n");
		isOk = FALSE;
		goto _errNoCleanup;
	}
	pNewNode->pData = pData;
	pNewNode->pNext = NULL;
	pNewNode->pPrev = pQueue->pTail;

/*	Make the old tail's next ptr point to the new node.
	--------------------------------------------------- */
	if (pQueue->pTail)
		pQueue->pTail->pNext = pNewNode;

/*	Make the queue's tail ptr point to the new node.
	------------------------------------------------ */
	pQueue->pTail = pNewNode;

/*	If the queue's head ptr is currently NULL (i.e. if the
	queue was empty before this function was called), then
	make it point to the new node.
	------------------------------------------------------ */
	if (pQueue->pHead == NULL)
		pQueue->pHead = pNewNode;

_errNoCleanup:

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

	return isOk;
}

/*
** Name:
**		UtlQueue_Create
**
** Purpose:
**		Create a new (empty) queue.
**
** Calling Sequence:
**		isOk = UtlQueue_Create(&queueHandle);
**
** Input:
**		NONE
**
** Output:
**		&queueHandle		Ptr to storage that is to receive the
**							handle of the new queue.
**
** Returns:
**		isOk				BOOLEAN indicating whether or not the
**							function executed successfully.
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int UtlQueue_Create(
		int*	pQueueHandle)		/* Ptr to storage to rcv queue handle	*/
{
	int		isOk = TRUE;			/* Return value of this function		*/
	UTL_QUEUE*
			pQueue = NULL;			/* Ptr to new queue						*/
	char*	func = "UtlQueue_Create";
									/* Name of this function				*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Allocate a queue data structure.  Set its head, tail, and curr ptrs
	to NULL.  Cast the queue's address as an int and write it into the
	storage provided by the caller.
	------------------------------------------------------------------- */
	pQueue = (UTL_QUEUE*)malloc(sizeof(UTL_QUEUE));
	if (pQueue == NULL)
	{
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"Could not allocate space for new queue\n");
		*pQueueHandle = 0;
		isOk = FALSE;
		goto _errNoCleanup;
	}

	pQueue->pHead = NULL;
	pQueue->pTail = NULL;
	pQueue->pCurr = NULL;

	*pQueueHandle = (int)pQueue;

_errNoCleanup:

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

	return isOk;
}

/*
** Name:
**		UtlQueue_Delete
**
** Purpose:
**		Delete an existing queue.  (NOTE: Only the queue nodes
**		and the queue data structure itself are deleted.  The
**		client data structures pointed to by the queue nodes
**		are NOT deleted -- that is the client's responsibility.)
**		
** Calling Sequence:
**		isOk = UtlQueue_Delete(queueHandle);
**
** Input:
**		queueHandle					Handle of the queue to be deleted.
**
** Output:
**		NONE
**
** Returns:
**		isOk						BOOLEAN indicating whether or not
**									the function completed successfully.
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int UtlQueue_Delete(
		int		queueHandle)		/* Queue handle							*/
{
	int		isOk = TRUE;			/* Return value of this function		*/
	char*	func = "UtlQueue_Delete";
									/* Name of this function				*/

/*	Log a trace line indicating we're entering this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Free all the queue nodes.
	------------------------- */
	while (UtlQueue_DelHead(queueHandle))
		;

/*	Free the queue itself.
	---------------------- */
	free((UTL_QUEUE*)queueHandle);

/*	Log a trace line indicating we're leaving this function.
	-------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

	return isOk;
}

/*
** Name:
**		UtlQueue_DelCurr
**
** Purpose:
**		Delete the "current" node of a queue.  (NOTE: Only the current
**		node itself is deleted.  The client data pointed to by the current
**		node is NOT deleted -- that is the client's responsibility.)
**
** Calling Sequence:
**		isOk = UtlQueue_DelCurr(queueHandle);
**
** Input:
**		queueHandle					Handle of the affected queue.
**
** Output:
**		NONE
**
** Returns:
**		isOk						BOOLEAN indicating whether or not
**									the function completed successfully.
**									(NOTE: If the specified queue is
**									empty, 'isOk' will be FALSE.)
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int UtlQueue_DelCurr(
		int		queueHandle)		/* Queue handle							*/
{
	int		isOk = TRUE;			/* Return value of this function		*/
	UTL_QUEUE*
			pQueue = NULL;			/* Queue handle recast as a queue ptr	*/
	char*	func = "UtlQueue_DelCurr";
									/* Name of thie function				*/

/*	Log a trace line indicating we're entering this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	(If no node is current, just return with an error.)
	--------------------------------------------------- */
	pQueue = (UTL_QUEUE*)queueHandle;
	if (pQueue->pCurr == NULL)
	{
		UtlLog_Write(LOG_TRACE, func, RIGHT_HERE,
			"Can't delete current node -- no node is current!\n");
		isOk = FALSE;
		goto _errNoCleanup;
	}

/*	If the current node is the head, then delete the head.
	------------------------------------------------------ */
	if (pQueue->pCurr == pQueue->pHead)
		UtlQueue_DelHead(queueHandle);

/*	Else if the current node is the tail, then delete the tail.
	----------------------------------------------------------- */
	else if (pQueue->pCurr == pQueue->pTail)
		UtlQueue_DelTail(queueHandle);

/*	Else (the queue contains at least three nodes, and the current
	node is an interior node)...
	-------------------------------------------------------------- */
	else
	{
/*		Make the node before the current node point to the
		node after the current node; make the node after the
		current node point to the node before the current node.
		------------------------------------------------------- */
		pQueue->pCurr->pPrev->pNext = pQueue->pCurr->pNext;
		pQueue->pCurr->pNext->pPrev = pQueue->pCurr->pPrev;

/*		Free the current node.
		---------------------- */
		free(pQueue->pCurr);

/*		Set the queue's curr ptr to NULL.
		--------------------------------- */
		pQueue->pCurr = NULL;
	}

_errNoCleanup:

/*	Log a trace line indicating we're leaving this function.
	-------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

	return isOk;
}

/*
** Name:
**		UtlQueue_DelHead
**
** Purpose:
**		Delete the head of a queue.  (NOTE: Only the head node itself
**		is deleted.  The client data pointed to by the head node is
**		NOT deleted -- that is the client's responsibility.)
**
** Calling Sequence:
**		isOk = UtlQueue_DelHead(queueHandle);
**
** Input:
**		queueHandle					Handle of the affected queue.
**
** Output:
**		NONE
**
** Returns:
**		isOk						BOOLEAN indicating whether or not
**									the function completed successfully.
**									(NOTE: If the specified queue is
**									empty, 'isOk' will be FALSE.)
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int UtlQueue_DelHead(
		int		queueHandle)		/* Queue handle							*/
{
	int		isOk = TRUE;			/* Return value of this function		*/
	UTL_QUEUE_NODE*
			pNewHead = NULL;		/* Ptr to new head node					*/
	UTL_QUEUE*
			pQueue = NULL;			/* Queue handle recast as a queue ptr	*/
	char*	func = "UtlQueue_DelHead";
									/* Name of thie function				*/

/*	Log a trace line indicating we're entering this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Set the queue's curr node ptr to NULL.
	-------------------------------------- */
	pQueue = (UTL_QUEUE*)queueHandle;
	pQueue->pCurr = NULL;

/*	(If the queue is empty, just return with an error.)
	--------------------------------------------------- */
	if (pQueue->pHead == NULL)
	{
		UtlLog_Write(LOG_TRACE, func, RIGHT_HERE,
			"Can't delete head -- queue is empty\n");
		isOk = FALSE;
		goto _errNoCleanup;
	}

/*	Save a ptr to the node after the head.
	-------------------------------------- */
	pNewHead = pQueue->pHead->pNext;

/*	Free the head node.
	------------------- */
	free(pQueue->pHead);

/*	(If the queue is now empty, just set the queue's
	head and tail ptrs to NULL and return.)
	------------------------------------------------ */
	if (pNewHead == NULL)
	{
		pQueue->pHead = NULL;
		pQueue->pTail = NULL;
		goto _errNoCleanup;
	}

/*	Make the queue's head ptr point to the new head.
	------------------------------------------------ */
	pQueue->pHead = pNewHead;

/*	Make the new head's prev ptr NULL.
	---------------------------------- */
	pNewHead->pPrev = NULL;

_errNoCleanup:

/*	Log a trace line indicating we're leaving this function.
	-------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

	return isOk;
}

/*
** Name:
**		UtlQueue_DelTail
**
** Purpose:
**		Delete the tail of a queue.  (NOTE: Only the tail node itself
**		is deleted.  The client data pointed to by the tail node is
**		NOT deleted -- that is the client's responsibility.)
**
** Calling Sequence:
**		isOk = UtlQueue_DelTail(queueHandle);
**
** Input:
**		queueHandle					Handle of the affected queue.
**
** Output:
**		NONE
**
** Returns:
**		isOk						BOOLEAN indicating whether or not
**									the function completed successfully.
**									(NOTE: If the specified queue is
**									empty, 'isOk' will be FALSE.)
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int UtlQueue_DelTail(
		int		queueHandle)		/* Queue handle							*/
{
	int		isOk = TRUE;			/* Return value of this function		*/
	UTL_QUEUE_NODE*
			pNewTail = NULL;		/* Ptr to new tail node					*/
	UTL_QUEUE*
			pQueue = NULL;			/* Queue handle recast as a queue ptr	*/
	char*	func = "UtlQueue_DelTail";
									/* Name of thie function				*/

/*	Log a trace line indicating we're entering this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Set the queue's curr node ptr to NULL.
	-------------------------------------- */
	pQueue = (UTL_QUEUE*)queueHandle;
	pQueue->pCurr = NULL;

/*	(If the queue is empty, just return with an error.)
	--------------------------------------------------- */
	if (pQueue->pTail == NULL)
	{
		UtlLog_Write(LOG_TRACE, func, RIGHT_HERE,
			"Can't delete tail -- queue is empty\n");
		isOk = FALSE;
		goto _errNoCleanup;
	}

/*	Save a ptr to the node before the tail.
	-------------------------------------- */
	pNewTail = pQueue->pTail->pPrev;

/*	Free the tail node.
	------------------- */
	free(pQueue->pTail);

/*	(If the queue is now empty, set the queue's
	head and tail ptrs to NULL and return.)
	------------------------------------------ */
	if (pNewTail == NULL)
	{
		pQueue->pHead = NULL;
		pQueue->pTail = NULL;
		goto _errNoCleanup;
	}

/*	Make the queue's tail ptr point to the new tail.
	------------------------------------------------ */
	pQueue->pTail = pNewTail;

/*	Make the new tail's next ptr NULL.
	---------------------------------- */
	pNewTail->pNext = NULL;

_errNoCleanup:

/*	Log a trace line indicating we're leaving this function.
	-------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

	return isOk;
}

/*
** Name:
**		UtlQueue_GetHead
**
** Purpose:
**		Get the client data ptr contained in the head node of
**		the given queue.
**
** Calling Sequence:
**		isOk = UtlQueue_GetHead(queueHandle, &pData);
**
** Input:
**		queueHandle					Handle to the queue of interest.
**
** Output:
**		&pData						Ptr to storage that is to receive
**									a copy of the client data ptr con-
**									tained in the head node of the
**									given queue.
**
** Returns:
**		isOk						BOOLEAN indicating whether or not
**									the function completed successfully.
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int UtlQueue_GetHead(
		int		queueHandle,		/* Queue handle							*/
		void**	ppData)				/* Ptr to storage to rcv ptr to data	*/
{
	int		isOk = TRUE;			/* Return value of this function		*/
	UTL_QUEUE*
			pQueue = NULL;			/* Queue handle recast as a queue ptr	*/
	char*	func = "UtlQueue_GetHead";
									/* Name of this function				*/

/*	Log a trace line indicating we're entering this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	(If the queue is empty, return immediately with an error.)
	---------------------------------------------------------- */
	pQueue = (UTL_QUEUE*)queueHandle;
	if (pQueue->pHead == NULL)
	{
		UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Queue is empty\n");
		*ppData = NULL;
		isOk = FALSE;
		goto _errNoCleanup;
	}

/*	Make the queue's curr node ptr point to the head.
	------------------------------------------------- */
	pQueue->pCurr = pQueue->pHead;

/*	Fetch the client data ptr from the curr node and write it to the
	storage provided by the caller.
	---------------------------------------------------------------- */
	*ppData = pQueue->pCurr->pData;

_errNoCleanup:

/*	Log a trace line indicating we're leaving this function.
	-------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

	return isOk;
}

/*
** Name:
**		UtlQueue_GetNext
**
** Purpose:
**		Get the client data ptr contained in the next node of
**		the given queue, i.e. the node following the current
**		node.
**
** Calling Sequence:
**		isOk = UtlQueue_GetNext(queueHandle, &pData);
**
** Input:
**		queueHandle					Handle to the queue of interest.
**
** Output:
**		&pData						Ptr to storage that is to receive
**									a copy of the client data ptr con-
**									tained in the next node of the
**									given queue.
**
** Returns:
**		isOk						BOOLEAN indicating whether or not
**									the function completed successfully.
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int UtlQueue_GetNext(
		int		queueHandle,		/* Queue handle							*/
		void**	ppData)				/* Ptr to storage to rcv ptr to data	*/
{
	int		isOk = TRUE;			/* Return value of this function		*/
	UTL_QUEUE*
			pQueue = NULL;			/* Queue handle recast as a queue ptr	*/
	char*	func = "UtlQueue_GetNext";
									/* Name of this function				*/

/*	Log a trace line indicating we're entering this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	(If there is no current node, return immediately with an error.)
	---------------------------------------------------------------- */
	pQueue = (UTL_QUEUE*)queueHandle;
	if (pQueue->pCurr == NULL)
	{
		UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "No current node\n");
		*ppData = NULL;
		isOk = FALSE;
		goto _errNoCleanup;
	}

/*	(If there is no next node, return immediately with an error.)
	------------------------------------------------------------- */
	if (pQueue->pCurr->pNext == NULL)
	{
		UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "No next node\n");
		*ppData = NULL;
		isOk = FALSE;
		goto _errNoCleanup;
	}

/*	Advance the queue's curr node ptr to the next node.
	--------------------------------------------------- */
	pQueue->pCurr = pQueue->pCurr->pNext;

/*	Fetch the client data ptr from the new curr node and write it to
	the storage provided by the caller.
	---------------------------------------------------------------- */
	*ppData = pQueue->pCurr->pData;

_errNoCleanup:

/*	Log a trace line indicating we're leaving this function.
	-------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

	return isOk;
}

/*
** Name:
**		UtlQueue_GetPrev
**
** Purpose:
**		Get the client data ptr contained in the prev node of
**		the given queue, i.e. the node before the current node.
**
** Calling Sequence:
**		isOk = UtlQueue_GetPrev(queueHandle, &pData);
**
** Input:
**		queueHandle					Handle to the queue of interest.
**
** Output:
**		&pData						Ptr to storage that is to receive
**									a copy of the client data ptr con-
**									tained in the prev node of the
**									given queue.
**
** Returns:
**		isOk						BOOLEAN indicating whether or not
**									the function completed successfully.
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int UtlQueue_GetPrev(
		int		queueHandle,		/* Queue handle							*/
		void**	ppData)				/* Ptr to storage to rcv ptr to data	*/
{
	int		isOk = TRUE;			/* Return value of this function		*/
	UTL_QUEUE*
			pQueue = NULL;			/* Queue handle recast as a queue ptr	*/
	char*	func = "UtlQueue_GetPrev";
									/* Name of this function				*/

/*	Log a trace line indicating we're entering this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	(If there is no current node, return immediately with an error.)
	---------------------------------------------------------------- */
	pQueue = (UTL_QUEUE*)queueHandle;
	if (pQueue->pCurr == NULL)
	{
		UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "No current node\n");
		*ppData = NULL;
		isOk = FALSE;
		goto _errNoCleanup;
	}

/*	(If there is no prev node, return immediately with an error.)
	------------------------------------------------------------- */
	if (pQueue->pCurr->pPrev == NULL)
	{
		UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "No previous node\n");
		*ppData = NULL;
		isOk = FALSE;
		goto _errNoCleanup;
	}

/*	Roll back the queue's curr node ptr to the previous node.
	--------------------------------------------------------- */
	pQueue->pCurr = pQueue->pCurr->pPrev;

/*	Fetch the client data ptr from the new curr node and write it to
	the storage provided by the caller.
	---------------------------------------------------------------- */
	*ppData = pQueue->pCurr->pData;

_errNoCleanup:

/*	Log a trace line indicating we're leaving this function.
	-------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

	return isOk;
}

/*
** Name:
**		UtlQueue_GetTail
**
** Purpose:
**		Get the client data ptr contained in the tail node of
**		the given queue.
**
** Calling Sequence:
**		isOk = UtlQueue_GetTail(queueHandle, &pData);
**
** Input:
**		queueHandle					Handle to the queue of interest.
**
** Output:
**		&pData						Ptr to storage that is to receive
**									a copy of the client data ptr con-
**									tained in the tail node of the
**									given queue.
**
** Returns:
**		isOk						BOOLEAN indicating whether or not
**									the function completed successfully.
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int UtlQueue_GetTail(
		int		queueHandle,		/* Queue handle							*/
		void**	ppData)				/* Ptr to storage to rcv ptr to data	*/
{
	int		isOk = TRUE;			/* Return value of this function		*/
	UTL_QUEUE*
			pQueue = NULL;			/* Queue handle recast as a queue ptr	*/
	char*	func = "UtlQueue_GetTail";
									/* Name of this function				*/

/*	Log a trace line indicating we're entering this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	(If the queue is empty, return immediately with an error.)
	---------------------------------------------------------- */
	pQueue = (UTL_QUEUE*)queueHandle;
	if (pQueue->pTail == NULL)
	{
		UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Queue is empty\n");
		*ppData = NULL;
		isOk = FALSE;
		goto _errNoCleanup;
	}

/*	Make the queue's curr node ptr point to the tail.
	------------------------------------------------- */
	pQueue->pCurr = pQueue->pTail;

/*	Fetch the client data ptr from the curr node and write it to the
	storage provided by the caller.
	---------------------------------------------------------------- */
	*ppData = pQueue->pCurr->pData;

_errNoCleanup:

/*	Log a trace line indicating we're leaving this function.
	-------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

	return isOk;
}

