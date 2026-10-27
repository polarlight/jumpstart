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
**		utl_hash.c
**
** Description:
**		This is the implementation file for the "utl_hash" module of the
**		UTL subsystem, which is a collection of general-purpose utility 
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
**		UtlHash_Dump
**		UtlHash_Lookup
**
** History:
**		Date		Engineer			Purpose
**		--------	----------------	---------------------------------
**		06/24/97	Steven M. Rosen		Initial creation.
*/

/* include files */
/* ------------- */
#include <string.h>
#include "utl_dbg.h"
#include "utl_log.h"
#include "utl_queue.h"
#include "utl_hash.h"

/* private definitions */
/* ------------------- */
#define	DEFAULT_SPARSENESS_FACTOR	3	/* the default sparseness factor	*/
#define	HASH_ID_INVALID				-1	/* hash ID is invalid				*/

#define	HASH_ID_TYPE_BUCKET			1	/* hash ID is a BUCKET type			*/
#define	HASH_ID_TYPE_INDEX			2	/* hash ID is an INDEX type			*/
#define	HASH_ID_TYPE_UNUSED			-1	/* hash ID is an UNUSED type		*/

/* private structures */
/* ------------------ */

/*	Hash table entry data structure.
	-------------------------------- */
typedef struct {
	int				typeHashID;			/* type of the hash identifier		*/
	int				hashID;				/* hash identifier					*/
	char*			pHashString;		/* ptr to hash string				*/
} UTL_HASH_TABLE_ENTRY;

typedef struct {
	int				szHashTable;			/* the size of the hash table	*/
	int				(*pTheHashingFunc)();	/* ptr to hashing function		*/
	UTL_HASH_TABLE_ENTRY*	pHashTblEntries;/* ptr to hash table entries	*/
} UTL_HASH_TABLE;

/* private global variables */
/* ------------------------ */

/* private prototype declarations */
/* ------------------------------ */

/****************************************************/
/* NOTE: Please keep the following static functions */
/* ====  in alphabetical order.  Thank you!         */
/****************************************************/

static UTL_HASH_TABLE_ENTRY* bucket_add(
		UTL_HASH_TABLE_ENTRY* pHashTblEntry);	/* ptr to hash tbl entry	*/

static UTL_HASH_TABLE_ENTRY* bucket_create(
		UTL_HASH_TABLE_ENTRY* pHashTblEntry);	/* ptr to hash tbl entry	*/

static UTL_HASH_TABLE_ENTRY* bucket_delete(
		UTL_HASH_TABLE_ENTRY* pHashTblEntry);	/* ptr to hash tbl entry	*/

static UTL_HASH_TABLE_ENTRY* bucket_dump(
		UTL_HASH_TABLE_ENTRY* pHashTblEntry);	/* ptr to hash tbl entry	*/

static UTL_HASH_TABLE_ENTRY* bucket_find(
		UTL_HASH_TABLE_ENTRY* pHashTblEntry,	/* ptr to hash tbl entry	*/
		char*	pHashString);			/* ptr to hash string				*/

static int copy_string(
		char**	ppDestStr,				/* ptr to ptr to destination string	*/
		char**	ppSrceStr,				/* ptr to ptr to source string		*/
		int		fDiscardSrceStr);		/* discard the source string flag	*/

static int default_hashing_func(
		char*	pHashString,			/* ptr to hash string				*/
		int		szHashTable);			/* the size of the hash table		*/

static int dump_hash_table_entry(
		UTL_HASH_TABLE_ENTRY* pHashTblEntry,	/* ptr to hash tbl entry	*/
		char*	pTextDescription);		/* ptr to textual description		*/

static int insert_hash_table_entry(
		UTL_HASH_TABLE_ENTRY* pHashTblEntries,	/* ptr to hash tbl entries	*/
		int		szHashTable,			/* the size of the hash table		*/
		char*	pHashString,			/* ptr to hash string				*/
		int		hashID,					/* hash string identifier			*/
		int		(*pTheHashingFunc)());	/* ptr to hashing function			*/

static int lookup_hash_table_entry(
		UTL_HASH_TABLE_ENTRY* pHashTblEntries,	/* ptr to hash tbl entries	*/
		int		szHashTable,			/* the size of the hash table		*/
		char*	pHashString,			/* ptr hash string					*/
		int		(*pTheHashingFunc)(),	/* ptr to hashing function			*/
		int*	pHashID);				/* ptr to hash string identifier	*/

static int validate_create_params(
		char**	ppHashStrings,			/* ptr to array of hash strings		*/
		int		nHashStrings,			/* number of hash strings			*/
		int		szSparsenessFactor,		/* size of the sparseness factor	*/
		int		(*pHashingFunc)(),		/* ptr to hashing function			*/
		int*	phHashTable);			/* ptr to returned hash tbl handle	*/

/*
** Name:
**		UtlHash_Create
**
** Purpose:
**		This public function creates a new hash table from a specified
**		array of strings.  An optional Sparseness Factor (positive number)
**		can be supplied to override the default Sparseness Factor for the
**		hash table.  Also, an optional hashing function (non-NULL) can be
**		supplied to override the default hashing function.
**
** Calling Sequence:
**		isOkay = UtlHash_Create(	ppHashStrings,
**									nHashStrings,
**									szSparsenessFactor,
**									pHashingFunc,
**									phHashTable	);
**
** Input:
**		ppHashStrings		ptr to array of hash strings
**		nHashStrings		number of hash strings
**		szSparsenessFactor	the size of the sparseness factor
**		pHashingFunc		ptr to the hash table function
**
** Output:
**		phHashTable			ptr to the returned hash table handle
**
** Returns:
**		isOkay				BOOLEAN indicating whether or not the
**							function executed successfully.
**
** Error Conditions:
**		NONE
**
** Comments:
**		If the "szSparsenessFactor" is set to a non-positive number then
**		a default Sparseness Factor will automatically be used.
**
**		If the "pHashFunc" is set to NULL then a default hash table function
**		will automatically be used.
*/
int UtlHash_Create(
		char**	ppHashStrings,			/* ptr to array of hash strings		*/
		int		nHashStrings,			/* number of hash strings			*/
		int		szSparsenessFactor,		/* size of the sparseness factor	*/
		int		(*pHashingFunc)(),		/* ptr to hashing function			*/
		int*	phHashTable)			/* ptr to returned hash tbl handle	*/
{
	int			isOkay = FALSE;			/* return value of this function	*/
	int			szHashTable;			/* the size of the hash table		*/
	int			(*pTheHashingFunc)();	/* ptr to THE hashing function		*/
	int			ndx;					/* loop counter						*/
	UTL_HASH_TABLE*			pHashTable;		/* ptr to the new hash table	*/
	UTL_HASH_TABLE_ENTRY*	pHashTblEntries;/* ptr to hash table entries	*/
	char*	func="UtlHash_Create";			/* name of this function		*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Assume that we are going to fail until proven otherwise.
	-------------------------------------------------------- */
	*phHashTable = 0;

/*	Validate the input parameters.
	------------------------------ */
	if ( !validate_create_params(ppHashStrings, nHashStrings,
								 szSparsenessFactor, pHashingFunc,
								 phHashTable) ) {
		goto _errValidateParams;
	}

/*	Determine the size of the hash table - use a default Sparseness
	Factor if the supplied Sparseness Factor is not a positive number.
	------------------------------------------------------------------ */
	szHashTable = szSparsenessFactor * nHashStrings;
	if (szHashTable <= 0) {
		szHashTable = DEFAULT_SPARSENESS_FACTOR * nHashStrings;
	}

/*	Determine if we use a supplied hashing function or a default
	hashing function when adding the hash table strings to the
	hash table.
	------------------------------------------------------------ */
	pTheHashingFunc = pHashingFunc;
	if (pTheHashingFunc == NULL) {
		pTheHashingFunc = default_hashing_func;
	}

/*	Allocate a hash table data structure.
	------------------------------------- */
	pHashTable = (UTL_HASH_TABLE*)malloc(sizeof(UTL_HASH_TABLE));
 	if (pHashTable == NULL) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"Could not allocate space for the new hash table\n");
		goto _errAllocatingHashTable;
	}

/*	Allocate the entries for the hash table.
	---------------------------------------- */
	pHashTblEntries = (UTL_HASH_TABLE_ENTRY*)calloc(
												szHashTable,
												sizeof(UTL_HASH_TABLE_ENTRY));
	if (pHashTblEntries == NULL) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"Could not allocate space for new hash table entries\n");
		goto _errAllocatingHashTableEntries;
	}

/*	Initialize all the entries of this new hash table.
	-------------------------------------------------- */
	for (ndx=0; ndx<szHashTable; ndx++) {
		(pHashTblEntries + ndx)->typeHashID  = HASH_ID_TYPE_UNUSED;
		(pHashTblEntries + ndx)->hashID      = HASH_ID_INVALID;
		(pHashTblEntries + ndx)->pHashString = NULL;
	}

/*	Save the size of the hash table, the hash table entries, and the
	hashing function in the allocated hash table structure.  Then, make
	sure we give the caller a handle to this new hash table.
	------------------------------------------------------------------- */
	pHashTable->szHashTable     = szHashTable;
	pHashTable->pHashTblEntries = pHashTblEntries;
	pHashTable->pTheHashingFunc = pTheHashingFunc;

	*phHashTable = (int)pHashTable;

/*	Run through the specified list of hash table strings and insert
	each one to the hash table.
	--------------------------------------------------------------- */
	for (ndx=0; ndx<nHashStrings; ndx++) {
		if ( !insert_hash_table_entry(pHashTblEntries, szHashTable,
									  *(ppHashStrings+ndx), ndx,
									  pTheHashingFunc) ) {
			goto _errInsertHashTableEntry;
		}
	}

/*	Indicate that everything went okay.
	----------------------------------- */
	isOkay = TRUE;
	goto _allOkay;

/*	Perform structured exception handling.
	-------------------------------------- */
_errInsertHashTableEntry:
	if (pHashTblEntries) free(pHashTblEntries);

_errAllocatingHashTableEntries:
	if (pHashTable) free(pHashTable);

_errAllocatingHashTable:
_errValidateParams:

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
_allOkay:
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOkay);
}

/*
** Name:
**		UtlHash_Delete
**
** Purpose:
**		This public function deletes an existing hash table.
**		
** Calling Sequence:
**		isOkay = UtlHash_Delete(hHashTable);
**
** Input:
**		hHashTable			handle of the hash table to be deleted.
**
** Output:
**		NONE
**
** Returns:
**		isOkay				BOOLEAN indicating whether or not
**							the function completed successfully.
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int UtlHash_Delete(
		int		hHashTable)				/* handle of hash tbl to be deleted	*/
{
	int						isOkay = FALSE;	/* return value of this func	*/
	int						szHashTable;	/* the size of the hash table	*/
	UTL_HASH_TABLE*			pHashTable;		/* ptr to the new hash table	*/
	UTL_HASH_TABLE_ENTRY*	pHashTblEntries;/* ptr to hash table entries	*/
	int						ndx;			/* loop counter					*/
	char*	func="UtlHash_Delete";			/* name of this function		*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Turn the handle into something we can use internally.
	----------------------------------------------------- */
	pHashTable = (UTL_HASH_TABLE*)hHashTable;

	szHashTable     = pHashTable->szHashTable;
	pHashTblEntries = pHashTable->pHashTblEntries;

/*	De-allocate any entries associated with any buckets that are
	found in the hash table.
	------------------------------------------------------------ */
	for (ndx=0; ndx<szHashTable; ndx++) {
		if ( (pHashTblEntries + ndx)->typeHashID == HASH_ID_TYPE_BUCKET ) {
			bucket_delete(pHashTblEntries + ndx);
		}
	}

/*	De-allocate any hash strings in the hash table.
	----------------------------------------------- */
	for (ndx=0; ndx<szHashTable; ndx++) {
		if ( (pHashTblEntries + ndx)->pHashString != NULL ) {
			free((pHashTblEntries + ndx)->pHashString);
			(pHashTblEntries + ndx)->pHashString = NULL;
		}
	}

/*	De-allocate the entries for the hash table.
	------------------------------------------- */
	if (pHashTable->pHashTblEntries != NULL) {
		free(pHashTable->pHashTblEntries);
		pHashTable->pHashTblEntries = NULL;
	}

/*	De-allocate the hash table data structure.
	------------------------------------------ */
	if (pHashTable != NULL) {
		free(pHashTable);
		pHashTable = NULL;
	}

/*	Indicate that everything went okay.
	----------------------------------- */
	isOkay = TRUE;

/*	Perform structured exception handling.
	-------------------------------------- */

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOkay);
}

/*
** Name:
**		UtlHash_Dump
**
** Purpose:
**		This public function dumps an existing hash table.
**		
** Calling Sequence:
**		isOkay = UtlHash_Dump(hHashTable);
**
** Input:
**		hHashTable			handle of the hash table to be dumped.
**
** Output:
**		NONE
**
** Returns:
**		isOkay				BOOLEAN indicating whether or not
**							the function completed successfully.
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int UtlHash_Dump(
		int		hHashTable)				/* handle of hash tbl to be dumped	*/
{
	int						isOkay = FALSE;	/* return value of this func	*/
	int						szHashTable;	/* the size of the hash table	*/
	UTL_HASH_TABLE*			pHashTable;		/* ptr to the new hash table	*/
	UTL_HASH_TABLE_ENTRY*	pHashTblEntries;/* ptr to hash table entries	*/
	int						ndx;			/* loop counter					*/
	char*	func="UtlHash_Dump";			/* name of this function		*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Turn the handle into something we can use internally.
	----------------------------------------------------- */
	pHashTable = (UTL_HASH_TABLE*)hHashTable;

	szHashTable     = pHashTable->szHashTable;
	pHashTblEntries = pHashTable->pHashTblEntries;

/*	Dump all the entries in the hash table.
	--------------------------------------- */
	for (ndx=0; ndx<szHashTable; ndx++) {

/*		Dump the hash information at this entry in the hash table
		according to the type of this entry.
		----------------------------------------------------------- */
		switch ( (pHashTblEntries + ndx)->typeHashID ) {
			case HASH_ID_TYPE_BUCKET:
				bucket_dump(pHashTblEntries + ndx);
				break;
		
			case HASH_ID_TYPE_INDEX:
				dump_hash_table_entry(pHashTblEntries + ndx, "normal slot");
				break;

			case HASH_ID_TYPE_UNUSED:
				dump_hash_table_entry(pHashTblEntries + ndx, "unused slot");
				break;

			default:
				break;
		}
	}

/*	Indicate that everything went okay.
	----------------------------------- */
	isOkay = TRUE;

/*	Perform structured exception handling.
	-------------------------------------- */

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOkay);
}

/*
** Name:
**		UtlHash_Lookup
**
** Purpose:
**		This public function looks for a match on a specified hash table
**		string in a specified hash table and returns the identifier for
**		the hash string.
**		
** Calling Sequence:
**		hashID = UtlHash_Lookup(hHashTable, pHashTableStr);
**
** Input:
**		hHashTable			handle of the specified hash table.
**		pHashTableStr		ptr to the hash string to lookup
**
** Output:
**		NONE
**
** Returns:
**		hashID				the returned identifier for the specified
**							hash table string - a negative value means
**							there was an error
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int UtlHash_Lookup(
		int		hHashTable,				/* handle of specified hash tbl		*/
		char*	pHashString)			/* ptr to hash string				*/
{
	int			szHashTable;			/* the size of the hash table		*/
	int			(*pTheHashingFunc)();	/* ptr to THE hashing function		*/
	int			hashID;					/* the hash identifier				*/
	UTL_HASH_TABLE*			pHashTable;		/* ptr to the new hash table	*/
	UTL_HASH_TABLE_ENTRY*	pHashTblEntries;/* ptr to hash table entries	*/
	char*	func="UtlHash_Lookup";			/* name of this function		*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Assume that we fail to find the hash string identifier.
	------------------------------------------------------- */
	hashID = -1;

/*	Turn the handle into something we can use internally.
	----------------------------------------------------- */
	pHashTable = (UTL_HASH_TABLE*)hHashTable;

	szHashTable     = pHashTable->szHashTable;
	pHashTblEntries = pHashTable->pHashTblEntries;
	pTheHashingFunc = pHashTable->pTheHashingFunc;

/*	Use the specified hash table string with the hash table function
	to find its hash identifier.
	---------------------------------------------------------------- */
	if ( !lookup_hash_table_entry(pHashTblEntries, szHashTable,
								  pHashString, pTheHashingFunc, &hashID) ) {
		goto _errLookupHashTableEntry;
	}

/*	Perform structured exception handling.
	-------------------------------------- */
_errLookupHashTableEntry:

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(hashID);
}

/****************************************************/
/* NOTE: Please keep the following static functions */
/* ====  in alphabetical order.  Thank you!         */
/****************************************************/

/*
** Name:
**		bucket_add
**
** Purpose:
**		This private function adds to a bucket for a specified entry
**		of a hash table.
*/
static UTL_HASH_TABLE_ENTRY* bucket_add(
		UTL_HASH_TABLE_ENTRY* pHashTblEntry)	/* ptr to hash tbl entry	*/
{
	UTL_HASH_TABLE_ENTRY* pNewEntry;	/* ptr to new hash tbl entry	*/
	char*	func="bucket_add";			/* name of this function		*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Allocate a new entry for the hash table.
	---------------------------------------- */
	pNewEntry = (UTL_HASH_TABLE_ENTRY*)calloc(
											1,
											sizeof(UTL_HASH_TABLE_ENTRY));
	if (pNewEntry == NULL) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"Could not allocate space for a new hash table entry\n");
		goto _errAllocatingNewEntry;
	}

/*	Add the new hash table entry as the next entry in the new
	bucket that has now been created.
	--------------------------------------------------------- */
	if ( !UtlQueue_AddHead(pHashTblEntry->hashID, (void*)pNewEntry) ) {
		UtlQueue_Delete(pHashTblEntry->hashID);
		goto _errAddToBucket;
	}
	goto _allOkay;

/*	Perform structured exception handling.
	-------------------------------------- */
_errAddToBucket:
	if (pNewEntry) free(pNewEntry);

_errAllocatingNewEntry:

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
_allOkay:
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(pNewEntry);
}

/*
** Name:
**		bucket_create
**
** Purpose:
**		This private function creates a new bucket for a specified entry
**		of a hash table.
*/
static UTL_HASH_TABLE_ENTRY* bucket_create(
		UTL_HASH_TABLE_ENTRY* pHashTblEntry)	/* ptr to hash tbl entry	*/
{
	UTL_HASH_TABLE_ENTRY* pThisEntry;	/* ptr to new hash tbl entry	*/
	UTL_HASH_TABLE_ENTRY* pNewEntry;	/* ptr to new hash tbl entry	*/
	char*	func="bucket_create";		/* name of this function		*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Allocate space for this entry of the hash table.
	------------------------------------------------ */
	pThisEntry = (UTL_HASH_TABLE_ENTRY*)calloc(
									1, sizeof(UTL_HASH_TABLE_ENTRY));
	if (pThisEntry == NULL) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"Could not allocate space for this hash table entry\n");
		goto _errAllocatingThisEntry;
	}

/*	Get the data for this entry copied from the original slot in the
	hash table to its new home as the first entry in the bucket now
	found at this slot in the hash table.  Always make a copy of the
	hash string for the new entry and discard the original hash string.
	------------------------------------------------------------------- */
	pThisEntry->typeHashID  = pHashTblEntry->typeHashID;
	pThisEntry->hashID      = pHashTblEntry->hashID;

	if ( !copy_string(&pThisEntry->pHashString,
					  &pHashTblEntry->pHashString, TRUE) ) {
		goto _errCopyingHashString;
	}

/*	Allocate a new entry for the hash table.
	---------------------------------------- */
	pNewEntry = (UTL_HASH_TABLE_ENTRY*)calloc(
									1, sizeof(UTL_HASH_TABLE_ENTRY));
	if (pNewEntry == NULL) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"Could not allocate space for a new hash table entry\n");
		goto _errAllocatingNewEntry;
	}

/*	Create a new queue that will be used to hold the linked list of
	hash table entries for the new bucket at this hash table entry.
	Get the data for the original hash table entry modified to show
	that we now have a BUCKET at this location in the hash table.
	--------------------------------------------------------------- */
	if ( !UtlQueue_Create(&pHashTblEntry->hashID) ) {
		goto _errCreateNewBucket;
	}
	pHashTblEntry->typeHashID  = HASH_ID_TYPE_BUCKET;
	if (pHashTblEntry->pHashString != NULL) {
		free(pHashTblEntry->pHashString);
		pHashTblEntry->pHashString = NULL;
	}

/*	Add the original hash table entry as the first entry in the new
	bucket that has now been created.
	--------------------------------------------------------------- */
	if ( !UtlQueue_AddHead(pHashTblEntry->hashID, (void*)pThisEntry) ) {
		UtlQueue_Delete(pHashTblEntry->hashID);
		goto _errAddToBucket;
	}

/*	Add the new hash table entry as the next entry in the new
	bucket that has now been created.
	--------------------------------------------------------- */
	if ( !UtlQueue_AddHead(pHashTblEntry->hashID, (void*)pNewEntry) ) {
		UtlQueue_Delete(pHashTblEntry->hashID);
		goto _errAddToBucket;
	}
	goto _allOkay;

/*	Perform structured exception handling.
	-------------------------------------- */
_errAddToBucket:
_errCreateNewBucket:
	if (pNewEntry) free(pNewEntry);

_errAllocatingNewEntry:
_errCopyingHashString:
	if (pThisEntry) free(pThisEntry);

_errAllocatingThisEntry:

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
_allOkay:
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(pNewEntry);
}

/*
** Name:
**		bucket_delete
**
** Purpose:
**		This private function deletes the bucket for a specified entry
**		of a hash table.
*/
static UTL_HASH_TABLE_ENTRY* bucket_delete(
		UTL_HASH_TABLE_ENTRY* pHashTblEntry)	/* ptr to hash tbl entry	*/
{
	int						gotEntry;		/* got an entry in bucket flag	*/
	UTL_HASH_TABLE_ENTRY*	pThisEntry;		/* ptr to new hash tbl entry	*/
	char*	func="bucket_delete";			/* name of this function		*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	De-allocate any hash table entries that were put into this bucket.
	Remember to de-allocate any memory associated with any hash strings
	found in the bucket.
	------------------------------------------------------------------- */
	gotEntry = UtlQueue_GetHead(pHashTblEntry->hashID, (void*)&pThisEntry);
	while (gotEntry) {
		if (pThisEntry != NULL) {
			if ( pThisEntry->pHashString != NULL ) {
				free(pThisEntry->pHashString);
				pThisEntry->pHashString = NULL;
			}
			free(pThisEntry);
			pThisEntry = NULL;
		}
		gotEntry = UtlQueue_GetNext(pHashTblEntry->hashID,
									(void*)&pThisEntry);
	}

/*	Finally, delete the queue that was used to manage this bucket of
	hash table entries.
	---------------------------------------------------------------- */
	UtlQueue_Delete(pHashTblEntry->hashID);

/*	Perform structured exception handling.
	-------------------------------------- */

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(NULL);
}

/*
** Name:
**		bucket_dump
**
** Purpose:
**		This private function dumps the bucket for a specified entry
**		of a hash table.
*/
static UTL_HASH_TABLE_ENTRY* bucket_dump(
		UTL_HASH_TABLE_ENTRY* pHashTblEntry)	/* ptr to hash tbl entry	*/
{
	int						gotEntry;		/* got an entry in bucket flag	*/
	UTL_HASH_TABLE_ENTRY*	pThisEntry;		/* ptr to new hash tbl entry	*/
	char*	func="bucket_dump";				/* name of this function		*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Dump the specified hash table entry (it is actually a bucket).
	-------------------------------------------------------------- */
	dump_hash_table_entry(pHashTblEntry, "bucket slot");

/*	Dump any hash table entries that were put into this bucket.
	----------------------------------------------------------- */
	gotEntry = UtlQueue_GetHead(pHashTblEntry->hashID, (void*)&pThisEntry);
	while (gotEntry) {
		dump_hash_table_entry(pThisEntry, "	bucket member");
		gotEntry = UtlQueue_GetNext(pHashTblEntry->hashID,
									(void*)&pThisEntry);
	}

/*	Perform structured exception handling.
	-------------------------------------- */

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(NULL);
}

/*
** Name:
**		bucket_find
**
** Purpose:
**		This private function finds an entry of a hash table in the
**		specified bucket for a specified hash string.
*/
static UTL_HASH_TABLE_ENTRY* bucket_find(
		UTL_HASH_TABLE_ENTRY* pHashTblEntry,	/* ptr to hash tbl entry	*/
		char*	pHashString)			/* ptr to hash string				*/
{
	int					gotMatch = FALSE;	/* got a match on hash string	*/
	int					gotEntry;			/* got an entry in bucket flag	*/
	UTL_HASH_TABLE_ENTRY*	pThisEntry;		/* ptr to new hash tbl entry	*/
	char*	func="bucket_find";				/* name of this function		*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Look for a match on the hash string with any hash table entries
	that were put into this bucket.
	--------------------------------------------------------------- */
	gotEntry = UtlQueue_GetHead(pHashTblEntry->hashID, (void*)&pThisEntry);
	while (gotEntry && !gotMatch) {
		if ( strcmp(pThisEntry->pHashString, pHashString) == 0 ) {
			gotMatch = TRUE;
		} else {
			gotEntry = UtlQueue_GetNext(pHashTblEntry->hashID,
										(void*)&pThisEntry);
		}
	}

/*	Perform structured exception handling.
	-------------------------------------- */

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(pThisEntry);
}

/*
** Name:
**		copy_string
**
** Purpose:
**		This private function makes a copy of a string and optionally
**		discards the original source string.
*/
static int copy_string(
		char**	ppDestStr,				/* ptr to ptr to destination string	*/
		char**	ppSrceStr,				/* ptr to ptr to source string		*/
		int		fDiscardSrceStr)		/* discard the source string flag	*/
{
	int			isOkay = FALSE;		/* return value of this function	*/
	char*	func="copy_string";		/* name of this function			*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Make sure the we have a valid source string before we do anything.
	------------------------------------------------------------------ */
	if ( *ppSrceStr != NULL) {
		*ppDestStr = (char*)calloc(1, strlen(*ppSrceStr) + 1);
		if (*ppDestStr == NULL) {
			UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
				"Could not allocate space for copying string \"%s\"\n",
					*ppSrceStr);
			goto _errAllocatingDestString;
		}
		strcpy(*ppDestStr, *ppSrceStr);

/*		Discard the original source string, if requested by the caller.
		--------------------------------------------------------------- */
		if (fDiscardSrceStr) {
			free(*ppSrceStr);
			*ppSrceStr = NULL;
		}
	}

/*	Indicate that everything went okay.
	----------------------------------- */
	isOkay = TRUE;

/*	Perform structured exception handling.
	-------------------------------------- */
_errAllocatingDestString:

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOkay);
}

/*
** Name:
**		default_hashing_func
**
** Purpose:
**		This private function is the default hashing function that is used
**		if a caller of the UtlHash_Create public function does not provide
**		a hashing function.
*/
static int default_hashing_func(
		char*	pHashString,			/* ptr to hash string				*/
		int		szHashTable)			/* the size of the hash table		*/
{
	int			theHashNum;				/* the calculated hash number	*/
	int			lenHashString;			/* length of the hash string	*/
	int			ndx;					/* loop counter					*/
	char*	func="default_hashing_func";	/* name of this function	*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	The default hashing algorithm is quite simple.  Just add up the binary
	value of each character in the hash string and then take the modular
	of the size of the hash table.  That's it! (Note that the length of
	the hash string has already been validated).
	---------------------------------------------------------------------- */
	theHashNum    = 0;
	lenHashString = strlen(pHashString);
	for (ndx=0; ndx<lenHashString; ndx++) {
		theHashNum += (int)(*(pHashString + ndx));
	}
	theHashNum = theHashNum % szHashTable;

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(theHashNum);
}

/*
** Name:
**		dump_hash_table_entry
**
** Purpose:
**		This private function dumps a specified hash table entry.
*/
static int dump_hash_table_entry(
		UTL_HASH_TABLE_ENTRY* pHashTblEntry,	/* ptr to hash tbl entry	*/
		char*	pTextDescription)		/* ptr to textual description		*/
{
	int			isOkay = FALSE;			/* return value of this function	*/
	char*	func="dump_hash_table_entry";	/* name of this function		*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Dump the information in this hash table entry, using the textual
	description provided by the caller.
	---------------------------------------------------------------- */
	UtlLog_Write(LOG_DEBUG | LOG_NO_ID, func, RIGHT_HERE,
		"%s\n", pTextDescription);
	UtlLog_Write(LOG_DEBUG | LOG_NO_ID, func, RIGHT_HERE,
		"		typeHashID  = %d\n", pHashTblEntry->typeHashID);
	UtlLog_Write(LOG_DEBUG | LOG_NO_ID, func, RIGHT_HERE,
		"		hashID      = %d\n", pHashTblEntry->hashID);
	if (pHashTblEntry->pHashString != NULL) {
		UtlLog_Write(LOG_DEBUG | LOG_NO_ID, func, RIGHT_HERE,
		"		pHashString = \"%s\"\n", pHashTblEntry->pHashString);
	}

/*	Indicate that everything went okay.
	----------------------------------- */
	isOkay = TRUE;

/*	Perform structured exception handling.
	-------------------------------------- */

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOkay);
}

/*
** Name:
**		insert_hash_table_entry
**
** Purpose:
**		This private function inserts a hash string into a hash table
**		using a specified hashing function and a specified hash identifier.
*/
static int insert_hash_table_entry(
		UTL_HASH_TABLE_ENTRY* pHashTblEntries,	/* ptr to hash tbl entries	*/
		int		szHashTable,			/* the size of the hash table		*/
		char*	pHashString,			/* ptr to hash string				*/
		int		hashID,					/* hash string identifier			*/
		int		(*pTheHashingFunc)())	/* ptr to hashing function			*/
{
	int			isOkay = FALSE;			/* return value of this function	*/
	int			theHashNum;				/* the hash number of hash string	*/
	int			theHashID;				/* the hash identifier				*/
	UTL_HASH_TABLE_ENTRY*	pThisEntry;	/* ptr to this hash table entry		*/
	char*	func="insert_hash_table_entry";	/* name of this function		*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Check to make sure that the hash string we are about to insert into
	the hash table is unique.
	------------------------------------------------------------------- */
	if ( lookup_hash_table_entry(pHashTblEntries, szHashTable,
								 pHashString, pTheHashingFunc, &theHashID) ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Duplicate hash string \"%s\" in the hash table\n",
				pHashString);
		goto _errDuplicateHashTableEntry;
	}

/*	Get the hash number for the specified hash string.
	-------------------------------------------------- */
	theHashNum = (*pTheHashingFunc)(pHashString, szHashTable);
	UtlLog_Write(LOG_DEBUG | LOG_NO_ID, func, RIGHT_HERE,
		"For hash string \"%20.20s\" the hash number = %d\n",
			pHashString, theHashNum);

/*	Insert the hash information at this entry in the hash table
	according to the type of this entry.
	----------------------------------------------------------- */
	switch ( (pHashTblEntries + theHashNum)->typeHashID ) {
		case HASH_ID_TYPE_BUCKET:
			pThisEntry = bucket_add(pHashTblEntries + theHashNum);
			break;

		case HASH_ID_TYPE_INDEX:
			pThisEntry = bucket_create(pHashTblEntries + theHashNum);
			break;

		case HASH_ID_TYPE_UNUSED:
			pThisEntry = (pHashTblEntries + theHashNum);
			break;

		default:
			break;
	}

/*	Fill in the hash information for this entry in the hash table.
	-------------------------------------------------------------- */
	if ( pThisEntry == NULL ) {
		goto _errInsertNewEntry;
	}

	pThisEntry->typeHashID  = HASH_ID_TYPE_INDEX;
	pThisEntry->hashID      = hashID;

	if ( !copy_string(&pThisEntry->pHashString,
					  &pHashString, FALSE) ) {
		goto _errCopyingHashString;
	}

/*	Indicate that everything went okay.
	----------------------------------- */
	isOkay = TRUE;

/*	Perform structured exception handling.
	-------------------------------------- */
_errCopyingHashString:
_errInsertNewEntry:
_errDuplicateHashTableEntry:

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOkay);
}

/*
** Name:
**		lookup_hash_table_entry
**
** Purpose:
**		This private function uses the specified hash table string
**		with the hash table function to find its hash identifier
*/
static int lookup_hash_table_entry(
		UTL_HASH_TABLE_ENTRY* pHashTblEntries,	/* ptr to hash tbl entries	*/
		int		szHashTable,			/* the size of the hash table		*/
		char*	pHashString,			/* ptr hash string					*/
		int		(*pTheHashingFunc)(),	/* ptr to hashing function			*/
		int*	pHashID)				/* ptr to hash string identifier	*/
{
	int			isOkay = FALSE;			/* return value of this function	*/
	int			theHashNum;				/* the hash number of hash string	*/
	UTL_HASH_TABLE_ENTRY*	pThisEntry;	/* ptr to this hash table entry		*/
	char*	func="lookup_hash_table_entry";	/* name of this function		*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Get the hash number for the specified hash string.
	-------------------------------------------------- */
	theHashNum = (*pTheHashingFunc)(pHashString, szHashTable);

/*	Lookup the hash information for this entry in the hash table
	according to the type of this entry.
	------------------------------------------------------------ */
	switch ( (pHashTblEntries + theHashNum)->typeHashID ) {
		case HASH_ID_TYPE_BUCKET:
			pThisEntry = bucket_find(pHashTblEntries + theHashNum,
									 pHashString);
			break;

		case HASH_ID_TYPE_INDEX:
			pThisEntry = (pHashTblEntries + theHashNum);
			break;

		case HASH_ID_TYPE_UNUSED:
			pThisEntry = NULL;
			break;

		default:
			break;
	}

/*	Make sure we have a valid entry and if so, see if the hash strings
	match to determine if we have been successful.
	------------------------------------------------------------------ */
	if ( pThisEntry == NULL ) {
		goto _errFindEntry;
	}
	if ( !(strcmp(pThisEntry->pHashString, pHashString) == 0) ) {
		goto _errNoMatchFound;
	}

/*	Retrieve the hash identifier from this entry in the hash table.
	--------------------------------------------------------------- */
	*pHashID = pThisEntry->hashID;

/*	Indicate that everything went okay.
	----------------------------------- */
	isOkay = TRUE;

/*	Perform structured exception handling.
	-------------------------------------- */
_errNoMatchFound:
_errFindEntry:

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOkay);
}

/*
** Name:
**		validate_create_params
**
** Purpose:
**		This private function validates the input parameters to the
**		public function UtlHash_Create.
*/
static int validate_create_params(
		char**	ppHashStrings,			/* ptr to array of hash strings		*/
		int		nHashStrings,			/* number of hash strings			*/
		int		szSparsenessFactor,		/* size of the sparseness factor	*/
		int		(*pHashingFunc)(),		/* ptr to hashing function			*/
		int*	phHashTable)			/* ptr to returned hash tbl handle	*/
{
	int			isOkay = FALSE;			/* return value of this function	*/
	int			ndx;					/* loop counter						*/
	char*	func="validate_create_params";	/* name of this function		*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Validate the ptr to the array of hash strings.
	---------------------------------------------- */
	if ( ppHashStrings == NULL ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Invalid ptr to the array of hash strings\n");
		goto _errInvalidParam;
	}

/*	Validate the number of hash strings.
	------------------------------------ */
	if ( (nHashStrings <= 0) || (nHashStrings > MAX_HASH_STRINGS) ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Invalid number of hash strings (%d), min=1, max=%d\n",
				nHashStrings, MAX_HASH_STRINGS);
		goto _errInvalidParam;
	}

/*	Validate each hash string in the array (both the ptr to the hash
	string as well as its length.
	---------------------------------------------------------------- */
	for (ndx=0; ndx<nHashStrings; ndx++) {
		if ( *(ppHashStrings + ndx) == NULL ) {
			UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
				"ERROR: Invalid ptr to hash string #%d\n", ndx);
			goto _errInvalidParam;
		}
		if ( strlen(*(ppHashStrings + ndx)) > MAX_HASH_STRING_SIZE ) {
			UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
				"ERROR: Hash string #%d \"%s\" is too long (%d), maxlen=%d\n",
					ndx, *(ppHashStrings + ndx),
					strlen(*(ppHashStrings + ndx)), MAX_HASH_STRING_SIZE);
			goto _errInvalidParam;
		}
	}

/*	Validate the sparseness factor, if it has been supplied.
	-------------------------------------------------------- */
	if ( szSparsenessFactor > 0 ) {
		if ( szSparsenessFactor > MAX_SPARSENESS_FACTOR ) {
			UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
				"ERROR: Invalid sparseness factor (%d), max=%d\n",
					szSparsenessFactor, MAX_SPARSENESS_FACTOR);
			goto _errInvalidParam;
		}
	}

/*	Validate the hashing function, if it has been supplied.
	------------------------------------------------------- */
	if ( pHashingFunc != NULL ) {
		;
	}

/*	Validate the ptr to the returned hash table handle.
	--------------------------------------------------- */
	if ( phHashTable == NULL ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Invalid ptr to the returned hash table handle\n");
		goto _errInvalidParam;
	}

/*	Indicate that everything went okay.
	----------------------------------- */
	isOkay = TRUE;

/*	Perform structured exception handling.
	-------------------------------------- */
_errInvalidParam:

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOkay);
}
