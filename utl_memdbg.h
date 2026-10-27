/*
**						 Copyright 1994-1997 Mesh, Inc.
**
**								  CONFIDENTIAL
**
**								Copyright Notice
**								----------------
**
** Copyright 1994-1997 Mesh, Inc. All rights reserved.  Reproduction in
** whole or in part without the express written consent of Mesh, Inc.
** is prohibited. 
*/

/*
** Name of File:
**		js_memdbg.h
**
** Description:
**		This is a header file associated with the "js_mem" module of the
**		JumpStart Library, which is a collection of general-purpose utility
**		functions for developing robust, well-layered, and well-partitioned
**		software.
**
**		Specifically, this header file needs to be included in the list
**		of include files for ANY module ('.c') that contains references
**		to the memory allocation or de-allocation functions that follow:
**
**					calloc
**					malloc
**					realloc
**					free
**
**		The inclusion of this header file will cause the above memory
**		allocation and de-allocation functions to be replaced with
**		calls to appropriate jsMemXxx functions that will allow
**		memory debugging to be done for that module.  For more details,
**		see the documentation in the "js_mem" module of the JumpStart
**		Library.
**
** Subsystem:
**      JumpStart Library
**
** Version:
**      1.00
**
** Public Functions:
**
**		Function Name				Function Description
**		--------------------------	------------------------------------------
**
** History:
**		Date		Engineer			Purpose
**		--------	---------------		--------------------------------------
**		07/28/94	Steven M. Rosen		Initial creation.
*/

/* Include Guard START */
/* ------------------- */
#ifndef _JS_MEMDBG_H_INCLUDED

/* Include Files */
/* ------------- */
#include "js_log.h"
#include "js_mem.h"

/* Public Definitions */
/* ------------------ */
#define calloc(n, size) 	jsMemCalloc(NULL, n, size, RIGHT_HERE, NULL) 
#define malloc(size) 		jsMemMalloc(NULL, size, RIGHT_HERE, NULL)
#define realloc(ptr, size)	jsMemRealloc(NULL, ptr, size, RIGHT_HERE, NULL)
#define free(ptr)			jsMemFree(NULL, ptr, RIGHT_HERE, NULL)

/* Public Structures */
/* ----------------- */

/* Public Global Variables */
/* ----------------------- */

/* Public Prototype Definitions */
/* ---------------------------- */


/* Include Guard END */
/* ----------------- */
#define _JS_MEMDBG_H_INCLUDED
#endif

