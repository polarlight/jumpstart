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
**		utl_queue.h
**
** Description:
**		This is the header file for the "utl_queue" module of the UTL
**		subsystem, which is a collection of general-purpose utility 
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

/* Include Guard START */
/* ------------------- */
#ifndef _UTL_QUEUE_H_INCLUDED

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
int UtlQueue_AddAfterCurr(
		int		queueHandle,		/* Queue handle							*/
		void*	pData);				/* Ptr to data to add after queue curr	*/

int UtlQueue_AddBeforeCurr(
		int		queueHandle,		/* Queue handle							*/
		void*	pData);				/* Ptr to data to add before queue curr	*/

int UtlQueue_AddHead(
		int		queueHandle,		/* Queue handle							*/
		void*	pData);				/* Ptr to data to add to queue head		*/

int UtlQueue_AddTail(
		int		queueHandle,		/* Queue handle							*/
		void*	pData);				/* Ptr to data to add to queue tail		*/

int UtlQueue_Create(
		int*	pQueueHandle);		/* Ptr to storage to rcv queue handle	*/

int UtlQueue_Delete(
		int		queueHandle);		/* Queue handle							*/

int UtlQueue_DelCurr(
		int		queueHandle);		/* Queue handle							*/

int UtlQueue_DelHead(
		int		queueHandle);		/* Queue handle							*/

int UtlQueue_DelTail(
		int		queueHandle);		/* Queue handle							*/

int UtlQueue_GetHead(
		int		queueHandle,		/* Queue handle							*/
		void**	ppData);			/* Ptr to storage to rcv ptr to data	*/

int UtlQueue_GetNext(
		int		queueHandle,		/* Queue handle							*/
		void**	ppData);			/* Ptr to storage to rcv ptr to data	*/

int UtlQueue_GetPrev(
		int		queueHandle,		/* Queue handle							*/
		void**	ppData);			/* Ptr to storage to rcv ptr to data	*/

int UtlQueue_GetTail(
		int		queueHandle,		/* Queue handle							*/
		void**	ppData);			/* Ptr to storage to rcv ptr to data	*/

/* Include Guard END */
/* ----------------- */
#define _UTL_QUEUE_H_INCLUDED
#endif

