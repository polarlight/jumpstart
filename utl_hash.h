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
**		utl_hash.h
**
** Description:
**		This is the header file for the "utl_hash" module of the UTL
**		subsystem, which is a collection of general-purpose utility 
**		functions that are used by all other subsystems.  This module
**		contains library functions that create and manage a hash table.
**
** Subsystem:
**      UTL
**
** Version:
**      1.00
**
** Public Functions:
**		UtlHash_Create
**		UtlHash_Delete
**		UtlHash_Lookup
**
** History:
**		Date		Engineer			Purpose
**		--------	----------------	---------------------------------
**		06/24/97	Steven M. Rosen		Initial creation.
*/

/* Include Guard START */
/* ------------------- */
#ifndef _UTL_HASH_H_INCLUDED

/* Include Files */
/* ------------- */

/* Public Definitions */
/* ------------------ */
#define	MAX_HASH_STRINGS		32767	/* the max number of hash strings	*/
#define	MAX_HASH_STRING_SIZE	255		/* the max size of a hash string	*/
#define	MAX_SPARSENESS_FACTOR	10		/* the max sparseness factor		*/

/* Public Structures */
/* ----------------- */

/* Public Global Variables */
/* ----------------------- */

/* Public Prototype Definitions */
/* ---------------------------- */
int UtlHash_Create(
		char**	ppHashStrings,			/* ptr to array of hash strings		*/
		int		nHashStrings,			/* number of hash strings			*/
		int		szSparsenessFactor,		/* size of the sparseness factor	*/
		int		(*pHashingFunc)(),		/* ptr to hashing function			*/
		int*	phHashTable);			/* ptr to returned hash tbl handle	*/

int UtlHash_Delete(
		int		hHashTable);			/* handle of hash tbl to be deleted	*/

int UtlHash_Dump(
		int		hHashTable);			/* handle of hash tbl to be dumped	*/

int UtlHash_Lookup(
		int		hHashTable,				/* handle of specified hash tbl		*/
		char*	pHashString);			/* ptr to hash string				*/

/* Include Guard END */
/* ----------------- */
#define _UTL_HASH_H_INCLUDED
#endif

