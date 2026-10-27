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
**		utl_timer_queue.h
**
** Description:
**		This is the header file for the "utl_timer_queue" module of the UTL
**		subsystem, which is a collection of general-purpose utility 
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

/* Include Guard START */
/* ------------------- */
#ifndef _UTL_TIMER_QUEUE_H_INCLUDED

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
int UtlTimerQueue_Add(
		int		queueHandle,	/* timer queue handle						*/
		int		timeUnits,		/* number of time units for added node		*/
		void*	pData);			/* ptr to user data to add onto timer queue	*/

int UtlTimerQueue_Create(
		int*	pQueueHandle);	/* ptr to storage to rcv timer queue handle	*/

int UtlTimerQueue_Decrement(
		int		queueHandle,	/* timer queue handle						*/
		int		timeUnits,		/* number of time units to decrement queue	*/
		int*	pNumDataPtrs,	/* ptr to returned number of user data ptrs	*/
		void**	ppData);		/* ptr to user data ptrs that have timed out*/

int UtlTimerQueue_Delete(
		int		queueHandle);	/* timer queue handle						*/

int UtlTimerQueue_SetDump(
		int		fDumpQueue);	/* dump timer queue flag					*/

/* Include Guard END */
/* ----------------- */
#define _UTL_TIMER_QUEUE_H_INCLUDED
#endif

