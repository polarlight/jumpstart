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
**		utl_dbg.h
**
** Description:
**		This is the header file for the "utl_dbg" module of the UTL
**		subsystem, which contains general-purpose utility functions used
**		by other subsystems to collect memory statistics.
**
**		Specifically, this header file needs to be included in the list
**		of include files for ANY module ('.c') that contains references
**		to the memory allocation or de-allocation functions that 
**		follow:
**					calloc
**					malloc
**					realloc
**					free
**
**					xvt_malloc
**					xvt_free
**
**		The inclusion of this header file will cause the above memory
**		allocation and de-allocation functions to be replaced with
**		calls to appropriate UtlMem_xxx functions that will allow
**		memory debugging to be done for that module.  For more details,
**		see the documentation in the "utl_mem" module of the UTL
**		subsystem.
**
** Subsystem:
**      UTL
**
** Version:
**      1.00
**
** Public Functions:
**
**		
** History:
**		Date		Engineer			Purpose
**		--------	---------------		---------------------------------
**		03/06/96	Duh Jang		 	Initial creation.
*/

/* include guard START */
/* ------------------- */
#ifndef _UTL_DBG_H_INCLUDED

/* include files */
/* ------------- */
#include "utl_log.h"
#include "utl_mem.h"

/* public definitions */
/* ------------------ */
#define calloc(n, size) 		UtlMem_Calloc(n, size, RIGHT_HERE) 
#define malloc(size) 			UtlMem_Malloc(size, RIGHT_HERE)
#define realloc(ptr, size)		UtlMem_Realloc(ptr, size, RIGHT_HERE)
#define free(ptr)				UtlMem_Free(ptr, RIGHT_HERE)

#define xvt_malloc(size) 		UtlMem_Malloc(size, RIGHT_HERE)
#define xvt_realloc(ptr, size)	UtlMem_Realloc(ptr, size, RIGHT_HERE)
#define xvt_free(ptr)			UtlMem_Free(ptr, RIGHT_HERE)


/* public structures */
/* ----------------- */

/* public global variables */
/* ----------------------- */

/* public prototype definitions */
/* ---------------------------- */

/* include guard END */
/* ----------------- */
#define _UTL_DBG_H_INCLUDED
#endif
