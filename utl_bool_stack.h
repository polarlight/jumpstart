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
**		utl_bool_stack.h
**
** Description:
**		This is the header file for the "utl_bool_stack" module of the UTL
**		subsystem, which is a collection of general-purpose utility 
**		functions that are used by all other subsystems.  This module
**		contains library functions that create and manage a simple
**		boolean push-down stack.
**
** Subsystem:
**      UTL
**
** Version:
**      1.00
**
** Public Functions:
**		UtlBoolStack_And
**		UtlBoolStack_Clear
**		UtlBoolStack_Create
**		UtlBoolStack_Delete
**		UtlBoolStack_Not
**		UtlBoolStack_Or
**		UtlBoolStack_Pop
**		UtlBoolStack_Push
**
** History:
**		Date		Engineer			Purpose
**		--------	----------------	---------------------------------
**		06/30/97	Steven M. Rosen		Initial creation.
*/

/* Include Guard START */
/* ------------------- */
#ifndef _UTL_BOOL_STACK_H_INCLUDED

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
int UtlBoolStack_And(
		int		stackHandle,	/* Boolean stack handle						*/
		int*	pBoolValue);	/* Ptr to current boolean value on stack	*/

int UtlBoolStack_Clear(
		int		stackHandle);	/* Boolean stack handle						*/

int UtlBoolStack_Create(
		int*	pStackHandle);	/* Ptr to returned boolean stack handle		*/

int UtlBoolStack_Delete(
		int		stackHandle);	/* Boolean stack handle						*/

int UtlBoolStack_Not(
		int		stackHandle,	/* Boolean stack handle						*/
		int*	pBoolValue);	/* Ptr to current boolean value on stack	*/

int UtlBoolStack_Or(
		int		stackHandle,	/* Boolean stack handle						*/
		int*	pBoolValue);	/* Ptr to current boolean value on stack	*/

int UtlBoolStack_Pop(
		int		stackHandle,	/* Boolean stack handle						*/
		int*	pBoolValue);	/* Ptr to boolean value popped off stack	*/

int UtlBoolStack_Push(
		int		stackHandle,	/* Boolean stack handle						*/
		int		boolValue);		/* Boolean value to push onto stack			*/

/* Include Guard END */
/* ----------------- */
#define _UTL_BOOL_STACK_H_INCLUDED
#endif

