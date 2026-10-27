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
**		utl_bool_eval.h
**
** Description:
**		This is the header file for the "utl_bool_eval" module of the UTL
**		subsystem, which is a collection of general-purpose utility 
**		functions that are used by all other subsystems.  This module
**		contains library functions that create and manage a simple
**		boolean expression evaluation engine.
**
** Subsystem:
**      UTL
**
** Version:
**      1.00
**
** Public Functions:
**		UtlBoolEval_Create
**		UtlBoolEval_Delete
**		UtlBoolEval_Evaluate
**
** History:
**		Date		Engineer			Purpose
**		--------	----------------	---------------------------------
**		07/01/97	Steven M. Rosen		Initial creation.
*/

/* Include Guard START */
/* ------------------- */
#ifndef _UTL_BOOL_EVAL_H_INCLUDED

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
int UtlBoolEval_Create(
		int*	pEvalHandle);	/* ptr to storage to rcv evaluation handle	*/

int UtlBoolEval_Delete(
		int		evalHandle);	/* evaluation handle						*/

int UtlBoolEval_Evaluate(
		int		evalHandle,		/* evaluation handle						*/
		char*	pEvalString,	/* ptr to the boolean evaluation string		*/
		int		countIds,		/* count of identifier strings and values	*/
		char**	ppIdStrings,	/* ptr to array of identifier strings ptrs	*/
		int*	pIdValues,		/* ptr to array of identifier values		*/
		int*	pBoolResult);	/* ptr to returned boolean result of eval	*/

int UtlBoolEval_SetDump(
		int		fDumpStack);	/* dump boolean stack flag					*/

/* Include Guard END */
/* ----------------- */
#define _UTL_BOOL_EVAL_H_INCLUDED
#endif

