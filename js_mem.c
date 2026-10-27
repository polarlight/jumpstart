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
**		utl_mem.c
**
** Description:
**		This module contains library functions for the UTL subsystem, which
**		is a collection of general-purpose utility functions that are used by
**		all other subsystems to collect memory statistics.
**
**      This module contains functions to create a very powerful memory
**		debugger for an application.  An application only needs to 
**		include "utl_dbg.h" in any module that contains the following
**		memory-related functions:
**
**					calloc
**					malloc
**					realloc
**					free
**
**					xvt_malloc
**					xvt_free
**
**		The inclusion of "utl_dbg.h" will automatically provide a set
**		of MACRO substitutions for these memory-related functions so
**		they call the corresponding "UtlMem_xxx" functions.  As each
**		call is made to either allocate or de-allocate memory, the
**		modules that include "utl_dbg.h" will automatically have memory
**		debugging available to them.  To actually activate the memory
**		debugging features of this module, a call must be made to 
**		"UtlMem_SetDebugMode" to turn on the appropriate level of 
**		memory debugging.  See the function header for "UtlMem_SetDebugMode" 
**		for additional details about this important function.
**
**		After "UtlMem_SetDebugMode" has been called to set some level
**		of memory debugging, calls to "UtlMem_DumpInfo" can be made
**		from selected logic areas of the application code to determine
**		how memory is being managed by the application.  A typical
**		usage of this call would be to invoke it one time before the
**		initialization logic of an application and then one additional 
**		time after the termination logic of an application and see if
**		the memory dump shows no outstanding memory nodes.  If there
**		ARE some memory nodes after executing the termination logic
**		of an application, it is clear that there is a memory leak
**		and if MEM_BREAKDOWN_ON is set, a nice breakdown of each
**		memory node will be provided at the time of the memory dump.
**		This information can be used to show exactly where and when
**		the memory was allocated by the application.
**
** Subsystem:
**      UTL
**
** Version:
**      1.00
**
** Public Functions:
**		UtlMem_Calloc
**		UtlMem_Malloc
**		UtlMem_Realloc
**		UtlMem_DumpInfo
**		UtlMem_Free
**		UtlMem_ResetQueue
**		UtlMem_SetDebugMode
**
**
** History:
**		Date		Engineer			Purpose
**		--------	---------------		---------------------------------
**		02/26/96	Duh Jang			Initial creation.
**		05/22/96	Steven M. Rosen		Replaced UtlMem_Alloc with
**										UtlMem_Calloc, UtlMem_Malloc,
**										and UtlMem_Realloc.  Also added
**										more documentation.
*/

/* include files */
/* ------------- */
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <malloc.h>
#include "utl_log.h"
#include "utl_mem.h"

/* private definitions */
/* ------------------- */
#define	FALSE				0				/* the value is FALSE		*/
#define	TRUE				1				/* the value is TRUE		*/
#define	MAX_LOG_LINE_LEN	256				/* max log file line length	*/
#define	MAX_TIMESTAMP_LEN	32				/* max timestamp length		*/
#define	TIMESTAMP_FORMAT	"%D %T"			/* timestamp format			*/

#define	LOG_ALLOC_FORMAT	" %s => %5d alloc %4d/%s"
#define	LOG_FREED_FORMAT	" %s => %5d freed %4d/%14.14s for %s" 

/* private structures */
/* ------------------ */

/* 
** The NODE_TYPE structure contains information to help monitor the
** allocation of memory.  The members of this structure includes a
** timestamp associated with the creation of the memory, the size of
** the allocated memory, the pointer to the allocated memory, and the
** originating source line and source file that made the call to create
** the memory chunk.  Another pair of members are the used to maintain
** a doubly-linked list of these memory queue nodes (the "prev" and 
** "next" members).
*/
typedef struct node_tag{
	time_t				timeMemory;	/* the time this memory was allocated	*/
    size_t				szMemory;	/* size of the allocated memory			*/ 
    void*				pMemory;	/* ptr to the allocated memory			*/
	int					srcLine;	/* original line of the source file		*/
	char*				srcFile;	/* original name of the source file 	*/
    struct node_tag*	prev;		/* ptr to the previous node in queue	*/
    struct node_tag*	next;		/* ptr to the next node in queue		*/
} NODE_TYPE;
 
/*
** The memory queue structure consists of a pointer to the first
** node in the memory queue and a count of the total number of 
** memory queue nodes.
*/
typedef struct queue_tag{
	NODE_TYPE*	top;		/* ptr to first memory node in memory queue	*/
	int			count;		/* count of memory nodes					*/
} QUEUE_TYPE;

/* private global variables */
/* ------------------------ */

static QUEUE_TYPE	m_Queue = {NULL,0};			/* memory queue			*/
static QUEUE_TYPE*	m_MemoryQueue = &m_Queue;	/* queue pointer		*/
static int			m_DebugOn = MEM_DEBUG_OFF;	/* memory debug flag	*/ 

/* private prototype definitions */
/* ----------------------------- */

static void freenode(
    NODE_TYPE*	pMemNode,	/* ptr to the memory queue node to be freed	*/
	int			srcLine,	/* the originating source line number		*/
	char*		srcFile);	/* the originating source file				*/

static size_t get_memory_size(
	void*		pMemory);	/* ptr to memory to search for				*/

static NODE_TYPE *makenode(
    void*		pMemory,	/* ptr to the allocated memory				*/
    size_t		szMemory,	/* the size of the allocated memory			*/
	time_t		timeMemory,	/* creation time for the allocated memory	*/
	int			srcLine,	/* the originating source line number		*/
	char*		srcFile);	/* the originating source file				*/

static void pushnode(
	NODE_TYPE*	pMemNode);	/* ptr to a memory node to be pushed onto queue	*/

static void queue_memory(
	void*		pMemory,	/* ptr to the allocated memory 				*/
	size_t		nElems,		/* number of elements of allocated memory	*/ 
	size_t		szElem,		/* the size of an allocated memory element 	*/
	int			srcLine,	/* the originating source line number		*/
	char*		srcFile);	/* the originating source file				*/

/*
** Name:
**		UtlMem_Calloc
**
** Purpose:
**		This function replaces a call to "calloc" and is used
**		to monitory the memory usage in any module that includes
**		"utl_dbg.h" to perform the MACRO substitution of all
**		"calloc" calls with calls to this function instead.
**
** Calling Sequence:
**		pMemory = UtlMem_Calloc(size_t nElems, size_t szElem,
**								int srcLine, char* srcFile);
** Input:
**		nElems			number of elements of memory to allocate
**		szElem			the size of a memory element to allocate
**		srcLine			the originating source line number
**		srcFile			the originating source file
**
** Output:
**		NONE
**
** Returns:
**		pMemory			the returned ptr to the allocated memory
**
** Error Conditions:
**		NONE
**
** Comments: 
**		NONE
*/
void *UtlMem_Calloc(
	size_t	nElems,			/* number of elements of memory	to allocate	*/ 
	size_t	szElem,			/* the size of a memory element to allocate	*/
	int		srcLine,		/* the originating source line number		*/
	char*	srcFile)		/* the originating source file				*/
{
	void*		pMemory; 	 /* generic pointer for holding dynamic memory	*/ 
	char*	func_name="UtlMem_Calloc";		/* ptr to name of the function	*/

/*	Allocate the memory for the user using "calloc".
	------------------------------------------------ */
	pMemory = calloc(nElems, szElem);
	if (pMemory == NULL) {
		UtlLog_Write(LOG_ERROR, func_name, RIGHT_HERE,
			"ERROR: Could not calloc %d bytes of memory\n", (nElems * szElem));
		return(NULL);
	}

/*	Get the memory that was just allocated placed in our memory
	queue, if necessary.
	----------------------------------------------------------- */
	queue_memory(pMemory, nElems, szElem, srcLine, srcFile);

/*	Return the pointer to the chunk of memory that was allocated
	to the caller.
	------------------------------------------------------------ */
	return(pMemory);
}

/*
** Name:
**		UtlMem_Malloc
**
** Purpose:
**		This function replaces a call to "malloc" and is used
**		to monitory the memory usage in any module that includes
**		"utl_dbg.h" to perform the MACRO substitution of all
**		"malloc" calls with calls to this function instead.
**
** Calling Sequence:
**		pMemory = UtlMem_Malloc(size_t szMemory,
**								int srcLine, char* srcFile);
** Input:
**		szMemory		the size of memory to allocate
**		srcLine			the originating source line number
**		srcFile			the originating source file
**
** Output:
**		NONE
**
** Returns:
**		pMemory			the returned ptr to the allocated memory
**
** Error Conditions:
**		NONE
**
** Comments: 
**		NONE
*/
void *UtlMem_Malloc(
	size_t	szMemory,		/* the size of memory to allocate		*/
	int		srcLine,		/* the originating source line number	*/
	char*	srcFile)		/* the originating source file			*/
{
	void*		pMemory; 	 				/* ptr to the allocated memory 	*/ 
	char*	func_name="UtlMem_Malloc";		/* ptr to name of the function	*/

/*	Allocate the memory for the user using "malloc".
	------------------------------------------------ */
	pMemory = malloc(szMemory);
	if (pMemory == NULL) {
		UtlLog_Write(LOG_ERROR, func_name, RIGHT_HERE,
			"ERROR: Could not malloc %d bytes of memory\n", szMemory);
		return(NULL);
	}

/*	Get the memory that was just allocated placed in our memory
	queue, if necessary.
	----------------------------------------------------------- */
	queue_memory(pMemory, 1, szMemory, srcLine, srcFile);

/*	Return the pointer to the chunk of memory that was allocated
	to the caller.
	------------------------------------------------------------ */
	return(pMemory);
}

/*
** Name:
**		UtlMem_Realloc
**
** Purpose:
**		This function replaces a call to "realloc" and is used
**		to monitory the memory usage in any module that includes
**		"utl_dbg.h" to perform the MACRO substitution of all
**		"realloc" calls with calls to this function instead.
**
** Calling Sequence:
**		pMemory = UtlMem_Realloc(void* pOrigMemory, size_t szNewMemory,
**								 int srcLine, char* srcFile);
** Input:
**		pOrigMemory		ptr to the original memory
**		szNewMemory		the size of the new memory to allocate
**		srcLine			the originating source line number
**		srcFile			the originating source file
**
** Output:
**		NONE
**
** Returns:
**		pMemory			the returned ptr to the allocated memory
**
** Error Conditions:
**		NONE
**
** Comments: 
**		NONE
*/
void *UtlMem_Realloc(
	void*	pOrigMemory,	/* ptr to the original memory				*/
	size_t	szNewMemory,	/* the size of the new memory to allocate	*/
	int		srcLine,		/* the originating source line number		*/
	char*	srcFile)		/* the originating source file				*/
{
	size_t		szOrigMemory;			/* the size of the original memory	*/
	void*		pMemory; 	 			/* ptr to the allocated memory 		*/ 
	char*	func_name="UtlMem_Realloc";			/* ptr to name of func		*/

/*	If memory debugging is turned off then just realloc the memory 
	and return to the caller.
	-------------------------------------------------------------- */
	if (m_DebugOn == MEM_DEBUG_OFF) {
		pMemory = realloc(pOrigMemory, szNewMemory);
		if (pMemory == NULL) {
			UtlLog_Write(LOG_ERROR, func_name, RIGHT_HERE,
				"ERROR: Could not realloc %d bytes of memory\n", szNewMemory);
			return(NULL);
		}
		return(pMemory);
	}

/*	Allocate the memory for the user using the UtlMem_XXX functions
	to simulate "realloc".  We cannot use "realloc" if memory debug
	is turned on because we are monitoring memory usage and it is 
	assumed that the original memory being reallocated first used a 
	UtlMem_XXX allocation routine.
	--------------------------------------------------------------- */

/*	Get the original size of the memory that was allocated.
	------------------------------------------------------- */
	szOrigMemory = get_memory_size(pOrigMemory);

/*	If we found the size of the original memory, then use UtlMem_Malloc
	to create the new memory.  After allocating the new memory, copy
	the original memory to the new memory.  In case the caller has
	done a "realloc" with a smaller size for the memory, only copy
	as much of the original memory as possible.
	-------------------------------------------------------------------- */
	if (szOrigMemory > 0) {
		pMemory = UtlMem_Malloc(szNewMemory, srcLine, srcFile);
		if (pMemory != NULL) {
			if (szOrigMemory <= szNewMemory) {
				memcpy(pMemory, pOrigMemory, szOrigMemory);
			} else {
				memcpy(pMemory, pOrigMemory, szNewMemory);
			}
			UtlMem_Free(pOrigMemory, srcLine, srcFile);
		}
	} else {
		UtlLog_Write(LOG_ERROR, func_name, RIGHT_HERE,
			"Could not find size of memory with ptr = %p\n", pOrigMemory);
	}

/*	Return the pointer to the chunk of memory that was allocated
	to the caller.
	------------------------------------------------------------ */
	return(pMemory);
}

/*
** Name:
**		UtlMem_DumpInfo
**
** Purpose:
**		This function dumps the current memory usage into
**		either a debug log file (if logging has been enabled
**		or to the standard output (if logging has NOT been
**		enabled.  See the description of "UtlMem_SetDebugMode"
**		to see more details about what information is dumped
**		out when this function is called.
**
** Calling Sequence:
** 		UtlMem_DumpInfo(char *function_name);
**
** Input:
**		function_name	ptr to the name of the calling function 
**						that has requested the memory dump
**
** Output:
**		NONE	
**
** Returns:
**		NONE	
**
** Error Conditions:
**		NONE
**
** Comments: 
**		NONE
*/
void UtlMem_DumpInfo(
	char*	function_name)	/* ptr to the name of the calling function	*/
{
    size_t  user_node_sum    = 0;	/* sum of user's node memory size	*/
    size_t  node_type_sum    = 0;	/* sum of NODE_TYPE node's size		*/
    size_t  total_sum        = 0;	/* sum of the above two				*/
	int		num_nonzero_node = 0;	/* number of memory nodes allocated	*/

	NODE_TYPE*	pMemNode;						/* ptr to next memory node	*/
	char		time_buff2[MAX_TIMESTAMP_LEN];	/* timestamp as a string	*/
	char*	func_name="UtlMem_DumpInfo";		/* ptr to name of function	*/

/*	Check the validity of the input arguments.
	------------------------------------------ */
	if (function_name == NULL) {
		UtlLog_Write(LOG_ERROR, func_name, RIGHT_HERE,
					"Invalid function name was detected.\n");
		return;
	}

/*	If memory debugging is turned off, then there are no statistics
	to output.
	--------------------------------------------------------------- */
	if (m_DebugOn == MEM_DEBUG_OFF) {
		UtlLog_Write(LOG_WARN, func_name, RIGHT_HERE,
			"There are no statistics being collected for \"%s\".\n",
			function_name);
		return;
	}

/*	If memory MEM_BREAKDOWN_ON is true, output a message announcing
	that we are going to dump the breakdown of any memory that
	has leaked.
	--------------------------------------------------------------- */
	if (MEM_BREAKDOWN_ON & m_DebugOn) {
		UtlLog_Write(LOG_DEBUG, func_name, RIGHT_HERE,
			"The memory breakdown for \"%s\" follows:\n", function_name);
	}

/*	Initialize ourselves for traversing through the queue by starting
	at the top of our queue.
	----------------------------------------------------------------- */
	pMemNode = m_MemoryQueue->top;

/*	Traverse through our queue of memory nodes and output any existing
	nodes to the log file if MEM_BREAKDOWN_ON is true.  Whether or not
	we need to output any existing nodes, sum the total number of nodes
	and the total amount of memory associated with these nodes for a
	summary report later on.
	------------------------------------------------------------------- */
	for (;pMemNode;pMemNode = pMemNode->next) {
		if (MEM_BREAKDOWN_ON & m_DebugOn) {
            strftime(time_buff2, sizeof(time_buff2), TIMESTAMP_FORMAT,
                 localtime(&pMemNode->timeMemory));

			UtlLog_Write(LOG_DEBUG|LOG_NO_ID, func_name, RIGHT_HERE,
				"    %5d  alloc  %4d/%s for %s\n",
					pMemNode->szMemory, pMemNode->srcLine,
					pMemNode->srcFile, time_buff2);
		}
		user_node_sum += pMemNode->szMemory;
		num_nonzero_node++;	
	}

/*	Get the sum of the tracking node memory size and add it to the
	user node memory size to get the total memory still allocated.
	-------------------------------------------------------------- */
	node_type_sum = (size_t)(m_MemoryQueue->count * sizeof(NODE_TYPE));
	total_sum = user_node_sum + node_type_sum;

/*	Output the debug memory summary information.
	-------------------------------------------- */
	UtlLog_Write(LOG_DEBUG, func_name, RIGHT_HERE,
		"The memory summary for \"%s\" follows:\n", function_name); 

	UtlLog_Write(LOG_DEBUG|LOG_NO_ID, func_name, RIGHT_HERE,
		"	The allocated memory = %10u bytes (total of %5d nodes)\n",
			user_node_sum, num_nonzero_node);

	UtlLog_Write(LOG_DEBUG|LOG_NO_ID, func_name, RIGHT_HERE,
		"	The overhead memory  = %10u bytes\n", node_type_sum);

	UtlLog_Write(LOG_DEBUG|LOG_NO_ID, func_name, RIGHT_HERE,
		"	The total memory     = %10u bytes\n", total_sum);
}

/*
** Name:
**		UtlMem_Free
**
** Purpose:
**		This function replaces a call to "free" and is used
**		to monitory the memory usage in any module that includes
**		"utl_dbg.h" to perform the MACRO substitution of all
**		"free" calls with calls to this function instead.
**
** Calling Sequence:
**		UtlMem_Free(void* pMemory, srcLine, srcFile);
**
** Input:
**		pMemory			ptr to the allocated memory to be freed
**		srcLine			the originating source line number
**		srcFile			the originating source file
**
** Output:
**		NONE
**
** Returns:
**		NONE
**
** Error Conditions:
**		NONE
**
** Comments: 
**		NONE
*/
void UtlMem_Free(
	void*	pMemory,	/* ptr to the allocated memory to be freed	*/ 
	int		srcLine,	/* the originating source line number		*/
	char*	srcFile)	/* the originating source file				*/
{
	NODE_TYPE*	pMemNode;				/* ptr to a memory node		*/	
	char*	func_name="UtlMem_Free";	/* ptr to name of function	*/

/*	If memory debugging is turned off then just free the pointer
	and return to the caller.
	------------------------------------------------------------ */
	if (m_DebugOn == MEM_DEBUG_OFF) {
		free(pMemory);
		return;
	}

/*	If there is nothing in our queue, then just report that all the
	allocated memory nodes are free already and return to the caller.
	----------------------------------------------------------------- */
	if (m_MemoryQueue->top == NULL){
		UtlLog_Write(LOG_ERROR, func_name, RIGHT_HERE,
			"The memory queue is empty.  No match can be found.\n");
		return;
	}

/*	Search for a matching node in our queue of memory nodes.  If a 
	match is found, then free the memory and the node that is 
	associated  with it.
	-------------------------------------------------------------- */
	for (pMemNode=m_MemoryQueue->top; pMemNode ; pMemNode=pMemNode->next) {
		if (pMemNode->pMemory == pMemory) {
			free(pMemory);
			freenode(pMemNode, srcLine, srcFile);
			return;
   	    }
	}
}

/*
** Name:
**		UtlMem_ResetQueue
**
** Purpose:
**		This function will reset the Memory Queue that is used
**		to manage the linked list of allocated memory nodes for
**		a process.  This function should only be called by a
**		child process that has just been forked from its parent
**		and wants to track its own memory allocation properly
**		(without having to worry about freeing up any memory 
**		that was allocated by the parent process BEFORE the fork
**		call was made).
**
** Calling Sequence:
**		UtlMem_ResetQueue();
**
** Input:
**		NONE
**
** Output:
**		NONE
**
** Returns:
**		NONE
**
** Error Conditions:
**		NONE
**
** Comments: 
**		NONE
*/
void UtlMem_ResetQueue()
{
	char*	func_name="UtlMem_ResetQueue";	/* ptr to name of function	*/

/*	Reset the Memory Queue for this process.
	---------------------------------------- */
	m_Queue.top   = NULL;
	m_Queue.count = 0;
}


/*
** Name:
**		UtlMem_SetDebugMode
**
** Purpose:
**		This function sets the memory debugging level for an
**		application.  A call to this function MUST be made
**		to activate the memory debugging features.  The following
**		possible modes can be set with this function:
**
**			MEM_DEBUG_OFF		turns memory debugging off
**			MEM_SUMMARY_ON		turns memory debug summary on
**			MEM_BREAKDOWN_ON	turns memory debug breakdown on
**			MEM_TIMESTAMP_ON	turns memory debug timestamp on
**			MEM_DEBUG_ALL		turns all memory debugging features on
**
**		The mode MEM_DEBUG_OFF turns memory debugging off.  This would
**		normally be done after the last call in the application to
**		the memory dumping function, "UtlMem_DumpInfo".
**
**		The mode MEM_SUMMARY_ON turns memory debug summary information
**		on.  If this mode is on when "UtlMem_DumpInfo" is called, only
**		summary information will be dumped out.  The summary information
**		includes the total USER memory still allocated (in bytes) and
**		the number of memory nodes (chunks) that still exist in the
**		memory queue maintained by this module.  Also included is the 
**		amount of memory used by this module to track the memory usage
**		(this is the "overhead" of memory debugging).  Finally, the
**		summary information includes the total amount of memory that
**		is allocated at the time of the call to "UtlMem_DumpInfo".
**
**		The mode MEM_BREAKDOWN_ON turns memory debug breakdown on.
**		If this mode is on when "UtlMem_DumpInfo" is called, a
**		complete breakdown of all the memory nodes that still exist
**		will be dumped out.  The breakdown information that is 
**		dumped out includes the number of bytes of memory for the
**		node, the source line number and source file name that shows
**		where the memory was allocated, and a timestamp showing the
**		time that the memory was originally allocated.  THIS IS ONE
**		OF THE MOST POWERFUL FEATURES OF THIS MEMORY DEBUGGING TOOL.
**
**		The mode MEM_TIMESTAMP_ON turns memory debug timestamp on.
**		If this mode is on, all memory that is either allocated
**		(via calls to "calloc", "malloc", "realloc", or "xvt_malloc")
**		or de-allocated (via calls to either "free" or "xvt_free")
**		will result in a debug line getting dumped out.  The timestamp
**		information that is dumped out includes the time that the
**		the information is dumped, the number of bytes being either
**		allocated or freed, whether or not the memory is being allocated
**		or freed, the source line number and source file name that
**		is either allocating or freeing the memory, and in the case
**		of memory that is being freed, the timestamp of the creation
**		of that node (chunk) of memory.  THIS IS ANOTHER VERY POWERFUL
**		FEATURE OF THIS MEMORY DEBUGGING TOOL.
**
**		The modes MEM_DEBUG_ON, MEM_BREAKDOWN_ON, and MEM_TIMESTAMP_ON
**		can be ORed independently.  The mode MEM_DEBUG_ALL is the same
**		as "(MEM_DEBUG_ON | MEM_BREAKDOWN_ON | MEM_TIMESTAMP_ON)".
**
** Calling Sequence:
**		UtlMem_SetDebugMode(mem_debug_mode);
**
** Input:
**		mem_debug_mode		the mode of memory debug logging:
**
**			MEM_DEBUG_OFF		turns memory debugging off
**			MEM_SUMMARY_ON		turns memory debug summary on
**			MEM_BREAKDOWN_ON	turns memory debug breakdown on
**			MEM_TIMESTAMP_ON	turns memory debug timestamp on
**			MEM_DEBUG_ALL		turns all memory debugging features on
**
** Output:
**		NONE
**
** Returns:
**		NONE
**
** Error Conditions:
**		NONE	
**
** Comments: 
**		NONE
*/
void UtlMem_SetDebugMode(
	int 	mem_debug_mode)		/* the mode of memory debug */
{
/*	Set the specified memory debug mode into the global module
	variable for use by other functions in this module that need
	to access the current memory debug mode.
	------------------------------------------------------------ */
	m_DebugOn = mem_debug_mode;
}

/*
** Name:
**		freenode	
**
** Purpose:
**		This function frees a memory node from the memory queue.
**
** Calling Sequence:
**		freenode(pMemNode, srcLine, srcFile);
**
** Input:
**		pMemNode		ptr to memory queue node to be freed
**		srcLine			the originating source line number
**		srcFile			the originating source file
**
** Output:
**		NONE
**
** Returns:
**		NONE
**
** Error Conditions:
**		NONE
**
** Comments: 
**		NONE
*/
static void freenode(
	NODE_TYPE*	pMemNode,	/* ptr to memory queue node to be freed	*/
    int			srcLine,	/* the originating source line number	*/
    char*		srcFile)	/* the originating source file			*/
{
	time_t    timeCurr; 	  					/* the current timestamp	*/
	char      timeCurrStr[MAX_TIMESTAMP_LEN];	/* curr timestamp as str 	*/
	char      timeMemoryStr[MAX_TIMESTAMP_LEN];	/* memory timestamp as str	*/
	char      id_msg[MAX_LOG_LINE_LEN];			/* logging ID message		*/
	char*	func_name="freenode";				/* ptr to name of function	*/

/*	Print a timestamp message for the freed memory if the user
	has MEM_TIMESTAMP_ON mode set to true.
	---------------------------------------------------------- */
	if (m_DebugOn & MEM_TIMESTAMP_ON) {
    	timeCurr = time(NULL);
		strftime(timeCurrStr, sizeof(timeCurrStr), TIMESTAMP_FORMAT,
                 localtime(&timeCurr));
		strftime(timeMemoryStr, sizeof(timeMemoryStr), TIMESTAMP_FORMAT,
                 localtime(&pMemNode->timeMemory));

		sprintf(id_msg, LOG_FREED_FORMAT, timeCurrStr, pMemNode->szMemory,
				srcLine, srcFile, timeMemoryStr);   

		UtlLog_Write(LOG_DEBUG | LOG_NO_ID, func_name, RIGHT_HERE,
					"%s\n", id_msg); 
    }

/*	Free the memory node.  There are 4 possibilites:
		1. the matched node is the first node and also the last node
		2. the matched node is the first node and but not the last
		3. the matched node is a middle node
		4. the matched node is the last node 
	---------------------------------------------------------------- */

/*	Case 1 - this is the only node on the list.
	------------------------------------------- */
    if ((pMemNode->prev==NULL) && (pMemNode->next==NULL)) {
	    free((void *)pMemNode);
	    m_MemoryQueue->count--;
	    m_MemoryQueue->top =NULL;

/*	Case 2 - this is the first node but not the last one.
	----------------------------------------------------- */
    } else if ((pMemNode->prev==NULL) && (pMemNode->next !=NULL)) {
		m_MemoryQueue->top = pMemNode->next;
		pMemNode->next->prev = NULL;
		pMemNode->next = NULL;/* breaks all the links before free*/
		free((void *)pMemNode);
		m_MemoryQueue->count--;

/*	Case 3 - this is a middle node so 4 links need to be broken.
	------------------------------------------------------------ */
	} else if ((pMemNode->prev !=NULL) && (pMemNode->next !=NULL)) {
		pMemNode->prev->next = pMemNode->next;
		pMemNode->next->prev = pMemNode->prev;
		pMemNode->next = pMemNode->prev = NULL;
		free((void *)pMemNode);
		m_MemoryQueue->count--;

/*	Case 4 - this is the last node.
	------------------------------- */
	} else if ((pMemNode->prev !=NULL) && (pMemNode->next == NULL)) {
		pMemNode->prev->next = NULL;
		pMemNode->prev = NULL;
		free((void *)pMemNode);
		m_MemoryQueue->count--;
	}
}

/*
** Name:
**		get_memory_size
**
** Purpose:
**		This functions gets the size of a chunk of memory that
**		presumably resides in our memory queue.  If no match is
**		found for the ptr to the memory, then a value of 0 is
**		returned for the size of the memory.
**
** Calling Sequence:
**		szMemory = get_memory_size(pMemory)
**
** Input:
**		pMemory			ptr to the allocated memory to match
**
** Output:
**		NONE
**
** Returns:
**		szMemory		the size of the allocated memory:
**							> 0  => the memory ptr was found
**							== 0 => the memoyr ptr was NOT found
**
** Error Conditions:
**		NONE
**
** Comments: 
**		NONE
*/
static size_t get_memory_size(
	void*	pMemory)	/* ptr to the allocated memory to match	*/
{
	NODE_TYPE*		pMemNode;				/* ptr to a memory node		*/	
	char*	func_name="get_memory_size";	/* ptr to name of function	*/

/*	If there is nothing in our queue, then just report that all the
	allocated memory nodes are free already and return to the caller.
	----------------------------------------------------------------- */
	if (m_MemoryQueue->top == NULL){
		UtlLog_Write(LOG_ERROR, func_name, RIGHT_HERE,
			"The memory queue is empty.  No match can be found.\n");
		return(0);
	}

/*	Search for a matching node in our queue of memory nodes.  If a 
	match is found, then return the size of the memory in the node.
	--------------------------------------------------------------- */
	for (pMemNode=m_MemoryQueue->top; pMemNode ; pMemNode=pMemNode->next) {
		if (pMemNode->pMemory == pMemory) {
			return(pMemNode->szMemory);
   	    }
	}

/*	Return an indication that no match was found in our queue.
	---------------------------------------------------------- */
	return(0);
}

/*
** Name:
**		makenode	
**
** Purpose:
**		This local function makes a new memory node for
**		the memory queue.
**
** Calling Sequence:
**		pMemNode = makenode(pMemory, szMemory, timeMemory,
**							srcLine, srcFile);
**
** Input:
**		pMemory			ptr to the allocated memory
**		szMemory		the size of the allocated memory
**		timeMemory		creation time for the allocated memory
**		srcLine			the originating source line number
**		srcFile			the originating source file
**
** Output:
**		NONE
**
** Returns:
**		pMemNode		ptr to the new memory queue node:
**							!= NULL ==> node was created okay
**							== NULL ==> node was NOT created okay	
**
** Error Conditions
**		NONE
**
** Comments: 
**		NONE
*/
static NODE_TYPE *makenode(
	void*		pMemory,		/* ptr to the allocated memory				*/	
	size_t		szMemory,		/* the size of the allocated memory			*/
	time_t		timeMemory,		/* creation time for the allocated memory	*/
	int			srcLine,		/* the originating source line number		*/
	char*		srcFile)		/* the originating source file				*/
{
	NODE_TYPE*		pMemNode;			/* tracking node holding place	*/ 
	char*	func_name="makenode";		/* ptr to name of function		*/

/*	Allocate a memory node for the memory that has been allocated on
	behalf of the user.
	---------------------------------------------------------------- */
	pMemNode = (NODE_TYPE *)calloc(1, sizeof(NODE_TYPE));
	if ( pMemNode == (NODE_TYPE*) NULL ) {
		UtlLog_Write(LOG_ERROR, func_name, RIGHT_HERE,
			"All memory has been exhausted.\n");
		return((NODE_TYPE*)NULL);
	} else {

/*		Save the memory pointer, the size of the memory that was allocated,
		the source line and source file associated with this chunk of 
		memory, and the time of this memory's creation.
		------------------------------------------------------------------- */
		pMemNode->pMemory    = pMemory;
		pMemNode->szMemory   = szMemory;
		pMemNode->srcLine    = srcLine;
		pMemNode->srcFile    = srcFile;
		pMemNode->timeMemory = timeMemory;
	}

/*	Return the pointer to the allocated memory node.
	------------------------------------------------ */
	return(pMemNode);
}

/*
** Name:
**		pushnode	
**
** Purpose:
**		This local function pushes a new memory node onto the
**		memory queue for future monitoring.
**
** Calling Sequence:
**		pushnode(pMemNode);
**
** Input:
**		pMemNode		ptr to a new node to push onto the memory queue
**
** Output:
**		NONE
**
** Returns:
**		NONE
**
** Error Conditions:
**		NONE
**
** Comments: 
**		NONE
*/
static void pushnode(
	NODE_TYPE*	pMemNode)	/* ptr to a node be push onto the memory queue	*/
{
	char*	func_name="pushnode";		/* ptr to name of function	*/

/*	Make sure we have a valid input memory node to push onto our queue.
	------------------------------------------------------------------- */
	if (pMemNode == NULL) {
		UtlLog_Write(LOG_ERROR, func_name, RIGHT_HERE,
			"Attempted to push a nonexistent node.\n");
		return;
	}

/*	If this is the first node to push onto the queue, then we need to
	adjust our queue pointer information.
	----------------------------------------------------------------- */
	if (m_MemoryQueue->top == NULL) {
		pMemNode->next = NULL;
		pMemNode->prev = NULL;
		m_MemoryQueue->top = pMemNode;
		m_MemoryQueue->count++;
	} else {

/*		Insert this memory node at the top of our memory queue.
		------------------------------------------------------- */
		pMemNode->next = m_MemoryQueue->top;
		pMemNode->prev = NULL;
		m_MemoryQueue->top->prev = pMemNode;
		m_MemoryQueue->top = pMemNode;
		m_MemoryQueue->count++;
	}
}

/*
** Name:
**		queue_memory
**
** Purpose:
**		This local function is the main work horse for any of the
**		allocation routines (UtlMem_Calloc, UtlMem_Malloc, and 
**		UtlMem_Realloc).  This function will monitor the memory
**		usage by placing all allocated memory onto a double-linked
**		list queue.
**
** Calling Sequence:
**		queue_memory(void* pMemory, size_t nElems, size_t szElem,
**					 int srcLine, char* srcFile);
**
** Input:
**		pMemory			ptr to the allocated memory 
**		nElems			number of elements of allocated	memory 
**		szElem			the size of an allocated memory element 
**		srcLine			the originating source line number
**		srcFile			the originating source file
**
** Output:
**		NONE
**
** Returns:
**		NONE
**
** Error Conditions:
**		NONE
**
** Comments: 
**		NONE
*/
static void queue_memory(
	void*	pMemory,		/* ptr to the allocated memory 				*/
	size_t	nElems,			/* number of elements of allocated memory	*/ 
	size_t	szElem,			/* the size of an allocated memory element 	*/
	int		srcLine,		/* the originating source line number		*/
	char*	srcFile)		/* the originating source file				*/
{
	NODE_TYPE*	pMemNode; 	 			/* ptr to a memory queue node		*/
	time_t		timeMemory; 	 		/* the current timestamp			*/
	char		id_msg[MAX_LOG_LINE_LEN];		/* logging ID message		*/
	char		time_buff[MAX_TIMESTAMP_LEN];	/* timestamp as a string	*/
	char*	func_name="queue_memory";			/* ptr to name of function	*/

/*	Get the timestamp for the memory that was just allocated.
	-------------------------------------------------------- */
	timeMemory = time(NULL);

/*	Print a timestamp message for the allocated memory if the user
	has MEM_TIMESTAMP_ON mode set to true.
	-------------------------------------------------------------- */
	if (pMemory != NULL) {
		if (m_DebugOn & MEM_TIMESTAMP_ON) {
       		strftime(time_buff, sizeof(time_buff), TIMESTAMP_FORMAT,
                 localtime(&timeMemory));

        	sprintf(id_msg,LOG_ALLOC_FORMAT,time_buff, (nElems * szElem),
					srcLine, srcFile);  

			UtlLog_Write(LOG_DEBUG | LOG_NO_ID, func_name, RIGHT_HERE,
						 "%s\n",id_msg); 
    	} 	

/*		If memory MEM_SUMMARY mode is set to true, we need to add this
		allocated chunk of memory to our list of memory nodes.
		-------------------------------------------------------------- */
		if (m_DebugOn & MEM_SUMMARY_ON) {
			pMemNode = makenode(pMemory, (nElems * szElem), timeMemory,
								srcLine, srcFile);
			pushnode(pMemNode);
	  	}
	}
}
