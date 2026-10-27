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
** Name of File (compilable unit):
**		utl_mem.h
**
** Description:
**		This is the header file for the "utl_mem" module of the UTL
**		subsystem, which contains general-purpose utility functions used
**		by other subsystems to collect memory statistics.
**
** Subsystem:
**      UTL
**
** Version:
**      1.00
**
** Public Functions:
**      UtlMem_Calloc
**      UtlMem_Malloc
**      UtlMem_Realloc
**      UtlMem_DumpInfo
**      UtlMem_Free
**      UtlMem_ResetQueue
**      UtlMem_SetDebugMode

**		
** History:
**		Date		Engineer			Purpose
**		--------	---------------		---------------------------------
**		02/26/96	Duh Jang		 	Initial creation.
*/

/* include guard START */
/* ------------------- */
#ifndef _UTL_MEM_H_INCLUDED

/* include files */
/* ------------- */
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <malloc.h>

/* public definitions */
/* ------------------ */
#define	MEM_DEBUG_OFF		0
#define	MEM_SUMMARY_ON		(1<<0)
#define	MEM_BREAKDOWN_ON	(1<<1)
#define	MEM_TIMESTAMP_ON	(1<<2)
#define	MEM_DEBUG_ON_ALL (MEM_SUMMARY_ON | MEM_BREAKDOWN_ON | MEM_TIMESTAMP_ON)

/* public structures */
/* ----------------- */

/* public global variables */
/* ----------------------- */

/* public prototype definitions */
/* ---------------------------- */
void	*UtlMem_Calloc(
							size_t		nElems,
							size_t		szElem,
							int			srcLine,
							char*		srcFile			);
void	*UtlMem_Malloc(
							size_t		szMemory,
							int			srcLine,
							char*		srcFile			);
void	*UtlMem_Realloc(
							void*		pOrigMemory,
							size_t		szNewMemory,
							int			srcLine,
							char*		srcFile			);
void	UtlMem_DumpInfo(
							char*		pFuncName		);
void	UtlMem_Free(
							void*		pMemory,
							int			srcLine,
							char*		srcFile			);
void	UtlMem_ResetQueue(
														);
void	UtlMem_SetDebugMode(
							int			modeDebug		);  

/* include guard END */
/* ----------------- */
#define _UTL_MEM_H_INCLUDED
#endif
