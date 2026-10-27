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
**		utl_stack.h
**
** Description:
**		This is the header file for the "utl_stack" module of the UTL
**		subsystem, which is a collection of general-purpose utility 
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

/* Include Guard START */
/* ------------------- */
#ifndef _UTL_STACK_H_INCLUDED

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
int UtlStack_Clear(
		int		stackHandle);	/* Stack handle								*/

int UtlStack_Create(
		int*	pStackHandle);	/* Ptr to storage to rcv stack handle		*/

int UtlStack_Delete(
		int		stackHandle);	/* Stack handle								*/

int UtlStack_Pop(
		int		stackHandle,	/* Stack handle								*/
		void**	ppData);		/* Ptr to ptr to data popped off the stack	*/

int UtlStack_Push(
		int		stackHandle,	/* Stack handle								*/
		void*	pData);			/* Ptr to data to push onto the stack		*/

/* Include Guard END */
/* ----------------- */
#define _UTL_STACK_H_INCLUDED
#endif

