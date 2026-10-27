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
**		utl_config_multi.c
**
** Description:
**		To read configuration information from one of a series of
**		configuration files and set global variables that are either
**		strings or numeric values or arrays of these types of variables.
**
**		Note: This module contains functions that are similar to those
**		found in "utl_config", but improves on the original "utl_config"
**		API calls ("UtlConfig_XXX") by allowing an application to open
**		up and load one or MORE configuration files.  The original
**		"utl_config" module made the assumption that an application 
**		would only need to open a single configuration file.  This
**		new module provides greater flexibility in that an application
**		can now open many configuration files.  This is especially 
**		useful in the case where an application consists of several
**		processes that want to each load a "common" configuration file,
**		yet also load their own process-specific configuration file.
**
**		Since the code in this module is based entirely on the original
**		"utl_config" module, an attempt was made to retain most of the
**		original logic.  The main enhancement has been the addition of
**		a "handle" for each of the API calls.  This "handle" allows this
**		new module to maintain a context for each configuration file that
**		is opened and loaded by an application.
**
** Subsystem:
**		UTL
**
** Version:
**		1.00
**
** Public Functions:
**		UtlConfigMulti_Close
**		UtlConfigMulti_DefineItem
**		UtlConfigMulti_GetItemSize
**		UtlConfigMulti_Initialize
**		UtlConfigMulti_Load
**		UtlConfigMulti_Open
**		UtlConfigMulti_Terminate
**		UtlConfigMulti_Unload
**
** History:
**		Date		Engineer			Purpose
**		--------	---------------		-------------------------------
**		05/13/97	Steven M. Rosen		Initial creation.
*/

/* include files */
/* ------------- */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ctype.h>
#include "utl_log.h"
#include "utl_queue.h"
#include "utl_config_multi.h"
#include "utl_dbg.h"

/* private definitions */
/* ------------------- */
#define	MAX_ARRAY_PTRS			1024	/* max number of array pointers		*/
#define	MAX_ID_LEN				80		/* max length of an identifier		*/
#define	MAX_INT_LEN				9		/* max length of an integer			*/
#define	MAX_LINE_LEN			256		/* max length of an input line		*/
#define	MAX_STR_LEN				1024	/* max length of a value's string	*/

#define	READ_ID_STATE			5000	/* reading an identifier state		*/
#define	READ_INT_STATE			5001	/* reading an integer state			*/
#define	READ_STR_STATE			5002	/* reading a string state			*/
#define	SEEK_ID_STATE			5003	/* seeking an identifier state		*/
#define	SEEK_VAL_STATE			5004	/* seeking a value state			*/

#define	CFG_INT_TYPE			6000	/* type flag for an integer			*/
#define	CFG_STR_TYPE			6001	/* type flag for a string			*/

#define	CFG_MODE_CONFIGURE		7000	/* configure flag					*/
#define	CFG_MODE_RECONFIGURE	7001	/* reconfigure flag					*/

#define	CHR_BEGIN_ARRAY			'{'		/* the BEGIN ARRAY character		*/
#define	CHR_COMMENT				'#'		/* the COMMENT character			*/
#define	CHR_END_ARRAY			'}'		/* the END ARRAY character			*/
#define	CHR_END_OF_FILE			0		/* the "END-OF-FILE" character		*/
#define	CHR_ESCAPE				92		/* the ESCAPE character ('\')		*/
#define	CHR_MINUS				'-'		/* the MINUS character				*/
#define	CHR_NEWLINE				'\n'	/* the NEWLINE character			*/
#define	CHR_PLUS				'+'		/* the PLUS character				*/
#define	CHR_STR_DELIMITER		'"'		/* the string DELIMITER character	*/
#define	CHR_UNDERSCORE			'_'		/* the UNDERSCORE character			*/

/* private structures */
/* ------------------ */
typedef char	ID_STR		[MAX_ID_LEN + 1];		/* an identifier string	*/
typedef char	LINE_STR	[MAX_LINE_LEN + 1];		/* an input line string	*/
typedef char	STR_STR		[MAX_STR_LEN + 1];		/* a string string		*/

typedef struct cfg_item {
	char*				pItemID;		/* ptr to config item identifier	*/
	char**				ppItemVal;		/* ptr to ptr to config item value	*/
	int					typeItem;		/* the type of the config item		*/
	int					isArray;		/* the config item is an array flag	*/
	int					countArray;		/* count of config items in array	*/
	struct cfg_item*	pNextItem;		/* ptr to the next config item		*/
} CFG_ITEM;

typedef struct cfg_state_info {
	FILE*		theConfigFileDescr;		/* ptr to config file descriptor	*/
	CFG_ITEM*	theConfigListHead;		/* ptr to head of linked list		*/
	CFG_ITEM*	theConfigListTail;		/* ptr to tail of linked list		*/
	int			theConfigMode;			/* the configuration mode			*/

	char**		theArrayPtrs;			/* ptr to temporary array ptrs		*/
	int			theArrayCount;			/* number of array items			*/
	int			theArrayType;			/* the type of array items			*/

	int			theInputCurrPos;		/* the current input position		*/
	int			theInputLastCh;			/* the last input character			*/
	int			theInputLastPos;		/* the last input position			*/
	LINE_STR	theInputStr;			/* the current input string			*/
} CFG_STATE_INFO;

/* private global variables */
/* ------------------------ */
static int		m_ConfigFileList  = 0;	/* handle to list of config files	*/		
static int		m_ConfigFileCount = 0;	/* count in list of config files	*/		

/* private prototype definitions */
/* ----------------------------- */
static void array_process_init(
	HANDLE	hConfigFile);		/* handle to configuration file				*/

static void array_process_term(
	HANDLE	hConfigFile);		/* handle to configuration file				*/

static char get_next_char(
	HANDLE	hConfigFile,		/* handle to configuration file				*/
	int*	pThisLine);			/* ptr to returned line number				*/
static int load_array_item(
	HANDLE	hConfigFile,		/* handle to configuration file				*/
	char*	pItemID,			/* ptr to config item identifier			*/
	int		typeItem,			/* the type of the config item				*/
	int		thisLine);			/* current line number in config file		*/
static int load_item(
	HANDLE	hConfigFile,		/* handle to configuration file				*/
	char*	pItemID,			/* ptr to config item identifier			*/
	char*	pItemVal,			/* ptr to the config item value				*/
	int		typeItem,			/* the type of the config item				*/
	int		thisLine);			/* current line number in config file		*/
static int process_READ_ID_STATE(
	HANDLE	hConfigFile,		/* handle to configuration file				*/
	char	ch,					/* the character being processed			*/
	int		thisLine,			/* current line number in config file		*/
	int*	pPosID,				/* ptr to counter for identifier			*/
	char*	pStrID,				/* ptr to a config item identifier string	*/
	int*	pCurrState);		/* ptr to current state of state machine	*/
static int process_READ_INT_STATE(
	HANDLE	hConfigFile,		/* handle to configuration file				*/
	char	ch,					/* the character being processed			*/
	int		thisLine,			/* current line number in config file		*/
	int*	pTypeArray,			/* ptr to the type of array being processed	*/
	int*	pDoingArray,		/* ptr to doing an array flag				*/
	int*	pPosID,				/* ptr to counter for identifier			*/
	char*	pStrID,				/* ptr to a config item identifier string	*/
	int*	pPosVal,			/* ptr to counter for value					*/
	char*	pStrVal,			/* ptr to a config item value string		*/
	int*	pCurrState);		/* ptr to current state of state machine	*/
static int process_READ_STR_STATE(
	HANDLE	hConfigFile,		/* handle to configuration file				*/
	char	ch,					/* the character being processed			*/
	int		thisLine,			/* current line number in config file		*/
	int*	pTypeArray,			/* ptr to the type of array being processed	*/
	int*	pDoingArray,		/* ptr to doing an array flag				*/
	int*	pGotEscape,			/* ptr to got an escape character active	*/ 
	int*	pPosID,				/* ptr to counter for identifier			*/
	char*	pStrID,				/* ptr to a config item identifier string	*/
	int*	pPosVal,			/* ptr to counter for value					*/
	char*	pStrVal,			/* ptr to a config item value string		*/
	int*	pCurrState);		/* ptr to current state of state machine	*/
static int process_SEEK_ID_STATE(
	HANDLE	hConfigFile,		/* handle to configuration file				*/
	char	ch,					/* the character being processed			*/
	int		thisLine,			/* current line number in config file		*/
	int*	pPosID,				/* ptr to counter for identifier			*/
	char*	pStrID,				/* ptr to a config item identifier string	*/
	int*	pCurrState);		/* ptr to current state of state machine	*/
static int process_SEEK_VAL_STATE(
	HANDLE	hConfigFile,		/* handle to configuration file				*/
	char	ch,					/* the character being processed			*/
	int		thisLine,			/* current line number in config file		*/
	int*	pTypeArray,			/* ptr to the type of array being processed	*/
	int*	pDoingArray,		/* ptr to doing an array flag				*/
	int*	pPosID,				/* ptr to counter for identifier			*/
	char*	pStrID,				/* ptr to a config item identifier string	*/
	int*	pPosVal,			/* ptr to counter for value					*/
	char*	pStrVal,			/* ptr to a config item value string		*/
	int*	pCurrState);		/* ptr to current state of state machine	*/
static int save_array_value(
	HANDLE	hConfigFile,		/* handle to configuration file				*/
	char*	pItemVal,			/* ptr to the config item value				*/
	int		typeItem);			/* the type of the config item				*/
static int validate_handle(
	HANDLE	hConfigFile);		/* handle to configuration file				*/

/*
** Name:
**		UtlConfigMulti_Close
**
** Purpose:
**		This public function is used to close a specified configuration file.
**		If the configuration file has not been unloaded, it will be unloaded
**		at this time.
**
** Calling Sequence:
**		isOkay = UtlConfigMulti_Close(hConfigFile);
**
** Input:
**		hConfigFile		handle to configuration file
**
** Output:
**		NONE
**
** Returns:
**		isOkay		whether or not the configuration file was
**					closed properly:
**						TRUE  => everything was closed okay
**						FALSE => everything was NOT closed okay
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int	UtlConfigMulti_Close(
		HANDLE	hConfigFile)	/* handle to configuration file				*/
{
	int				isOkay = FALSE;			/* return value 				*/
	int				gotEntry;				/* got an entry in queue flag	*/
	int				gotMatchingEntry;		/* got a matching entry flag	*/
	HANDLE			hThisFile;				/* handle to this config file	*/
	CFG_STATE_INFO*	pConfigInfo;			/* ptr to config file state info*/
	char*	func="UtlConfigMulti_Close";	/* ptr to name of function		*/

/*	Validate this configuration file handle.
	---------------------------------------- */
	if ( !validate_handle(hConfigFile) ) {
		goto _errValidateHandle;
	}
	pConfigInfo = (CFG_STATE_INFO*)hConfigFile;

/*	Make sure we have at least one element in the Configuration File List.
	---------------------------------------------------------------------- */
	if (m_ConfigFileCount <= 0) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: There are no configuration files currently open.\n");
		goto _errNoConfigFilesOpen;
	}

/*	Find the matching entry in the Configuration File List.
	------------------------------------------------------- */
	gotMatchingEntry = FALSE;
	gotEntry = UtlQueue_GetHead(m_ConfigFileList, (void*)&hThisFile);

    while (gotEntry && !gotMatchingEntry) {
		if (hThisFile == hConfigFile) {

/*			Free up the memory associated with this configuration file.
			----------------------------------------------------------- */
			free(pConfigInfo);

/*			Free up this entry in the Configuration File List.
			-------------------------------------------------- */
			if ( !UtlQueue_DelCurr(m_ConfigFileList) ) {
				UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
					"ERROR: Could not delete a config file from the list\n");
				goto _errQueueDelCurr;
			}

/*			Bump down the number of currently open configuration files.
			----------------------------------------------------------- */
			m_ConfigFileCount--;
			gotMatchingEntry = TRUE;

/*		No match was found for this entry, so try the next one.
		------------------------------------------------------- */
		} else {
			gotEntry = UtlQueue_GetNext(m_ConfigFileList, (void*)&hThisFile);
		}
    }

/*	If we did not find a matching entry in The Registry, we have a problem!
	----------------------------------------------------------------------- */
	if ( !gotMatchingEntry ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Could not find a match in the Configuration File List\n");
			goto _errNoMatchingEntry;
	}

/*	Everything went okay, so indicate a good return status.
	------------------------------------------------------- */
	isOkay = TRUE;

/*	Error conditions were detected, so indicate a bad return status.
	---------------------------------------------------------------- */
_errNoMatchingEntry:
_errQueueDelCurr:
_errNoConfigFilesOpen:
_errValidateHandle:

/*	Return the status to the caller.
	-------------------------------- */
	return(isOkay);
}

/*
** Name:
**		UtlConfigMulti_DefineItem
**
** Purpose:
**		This public function is used to define one global variable for a
**		specified configuration file.  It should be called for each global
**		variable that is found in the configuration file prior to calling
**		the load configuration file function (UtlConfigMulti_Load).
**
** Calling Sequence:
**		isOkay = UtlConfigMulti_DefineItem(hConfigFile, ppItemVal, pItemID);
**
** Input:
**		hConfigFile		handle to configuration file
**		ppItemVal		ptr to ptr to the config item value
**		pItemID			ptr to identifier string for config item
**
** Output:
**		ppItemVal		ptr to the ptr to the config item value
**
** Returns:
**		isOkay		whether or not the configuration file item was
**					defined properly:
**						TRUE  => everything was defined okay
**						FALSE => everything was NOT defined okay
**
** Error Conditions:
**		NONE
**
** Comments:
**		To use this function for a global configurable item, the item
**		must be defined in advance, either in the ".h" file if it is
**		a global variable or the ".c" file if it is a module variable
**		(global to just the module).
**
**		For example, if the variables were to be module variables, there
**		would be entries like this at the top of the ".c" file:
**
**			char*	m_MY_STRING;		# my generic string 
**			int		m_MY_INTEGER;		# my generic integer 
**
**			char**	m_MY_STRINGS;		# my generic array of strings
**			int**	m_MY_INTEGERS;		# my generic array of integers
**
**		Then, this function would be called like this during the 
**		initialization of the application:
**
**			UtlConfigMulti_DefineItem(hConfigFile,
**							&m_MY_STRING,			"MY_STRING"		);
**			UtlConfigMulti_DefineItem(hConfigFile,
**							(char**)&m_MY_INTEGER,	"MY_INTEGER"	);
**
**			UtlConfigMulti_DefineItem(hConfigFile,
**							(char**)&m_MY_STRINGS,	"MY_STRINGS"	);
**			UtlConfigMulti_DefineItem(hConfigFile,
**							(char**)&m_MY_INTEGERS,	"MY_INTEGERS"	);
*/
int	UtlConfigMulti_DefineItem(
		HANDLE	hConfigFile,	/* handle to configuration file				*/
		char**	ppItemVal,		/* ptr to ptr to the config item value		*/
		char*	pItemID)		/* ptr to identifier string for config item	*/
{
	int					isOkay = FALSE;	/* the returned status				*/
	CFG_STATE_INFO*		pConfigInfo;	/* ptr to config file state info	*/
	CFG_ITEM*			pNewItem;		/* ptr to a new config item			*/
	char*	func="UtlConfigMulti_DefineItem";	/* ptr to name of function	*/

/*	Validate this configuration file handle.
	---------------------------------------- */
	if ( !validate_handle(hConfigFile) ) {
		goto _errValidateHandle;
 	}
	pConfigInfo = (CFG_STATE_INFO*)hConfigFile;

/*	If this is a reconfiguration, unload the old configuration.
	----------------------------------------------------------- */
	if (pConfigInfo->theConfigMode == CFG_MODE_RECONFIGURE) {
		UtlLog_Write(LOG_DEBUG, func, RIGHT_HERE,
			"Unloading configuration...\n");
		UtlConfigMulti_Unload(hConfigFile);
		pConfigInfo->theConfigMode = CFG_MODE_CONFIGURE;
	}
	
/*	Allocate memory for this new configuration item and then 
	initialize it with the caller's information.
	-------------------------------------------------------- */
	pNewItem = (CFG_ITEM*)calloc(1, sizeof(CFG_ITEM));
	if (pNewItem == NULL) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Could not allocate memory for config item \"%s\"\n",
				pItemID);
		goto _errNoMoreMemory;
	}
	pNewItem->ppItemVal = ppItemVal;
	pNewItem->pItemID   = malloc(strlen(pItemID) + 1);
	if (pNewItem->pItemID == NULL) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Could not allocate memory for config item \"%s\"\n",
				pItemID);
		goto _errNoMoreMemory;
	}
	strcpy(pNewItem->pItemID, pItemID);
	
/*	Add this new configuration item to our list of configuration items,
	remembering to special case the first item to be added to our
	linked list.
	------------------------------------------------------------------- */
	if (pConfigInfo->theConfigListTail == NULL) {
		pConfigInfo->theConfigListHead = pNewItem;
	} else {
		pConfigInfo->theConfigListTail->pNextItem = pNewItem;
	}
	pConfigInfo->theConfigListTail = pNewItem;
	
/*	We are all done defining this new configuration item.
	----------------------------------------------------- */

/*	Everything went okay, so indicate a good return status.
	------------------------------------------------------- */
	isOkay = TRUE;

/*	Error conditions were detected, so indicate a bad return status.
	---------------------------------------------------------------- */
_errNoMoreMemory:
_errValidateHandle:

/*	Return the status to the caller.
	-------------------------------- */
	return(isOkay);
}
	
/*
** Name:
**		UtlConfigMulti_GetItemSize
**
** Purpose
**		This public function gets the size of a config item for a specified
**		configuration item identifier.  This is only meaningful for a
**		configuration item that is an array.
**
** Calling Sequence:
**		szItem = UtlConfigMulti_GetItemSize(hConfigFile, pItemID);
**
** Input:
**		hConfigFile		handle to configuration file
**		pItemID			ptr to the config item identifier string
**
** Output:
**		NONE
**
** Returns:
**		szItem		the size of the config item
**						 > 0 => the size of the config item
**						== 0 => the config item identifier was NOT found
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int UtlConfigMulti_GetItemSize(
		HANDLE	hConfigFile,	/* handle to configuration file				*/
		char*	pItemID)		/* ptr to the config item identifier string	*/
{
	CFG_STATE_INFO*	pConfigInfo;	/* ptr to config file state info		*/
	CFG_ITEM*		pThisItem;		/* ptr to config item in linked list	*/
	int				wasFound;		/* config item was found flag			*/
	char*	func="UtlConfigMulti_GetItemSize";	/* ptr to name of function	*/

/*	Validate this configuration file handle.
	---------------------------------------- */
	if ( !validate_handle(hConfigFile) ) {
		goto _errValidateHandle;
	}
	pConfigInfo = (CFG_STATE_INFO*)hConfigFile;
																			
/*	Assume the config item has NOT been found in our linked list of
	config items.
	--------------------------------------------------------------- */
	wasFound = FALSE;
	
/*	Start at the beginning of the linked list and search for a match
	with the config item identifier string that has been passed into
	this function.
	---------------------------------------------------------------- */
	pThisItem = pConfigInfo->theConfigListHead;
	while (pThisItem != NULL) {
	
/*		If the identifier read from the config file matches the config item
		identifier of the current config item, then return the size of this
		config item.
		------------------------------------------------------------------- */
		if ( strcmp(pItemID, pThisItem->pItemID) == 0 ) {
			return(pThisItem->countArray);
		}	
		
/*		Bump to the next config item in the linked list.
		------------------------------------------------ */
		pThisItem = pThisItem->pNextItem;
	}

/*	Error conditions were detected, so indicate a bad return status.
	---------------------------------------------------------------- */
_errValidateHandle:
	
/*	Return that we did not find a match for this config item.
	--------------------------------------------------------- */
	return(0);
}

/*
** Name:
**		UtlConfigMulti_Initialize
**
** Purpose:
**		This public function is used to initialize the "utl_config_multi"
**		API functions.
**
** Calling Sequence:
**		isOkay = UtlConfigMulti_Initialize();
**
** Input:
**		NONE
**
** Output:
**		NONE
**
** Returns:
**		isOkay		whether or not the multiple configuration file
**					module was initialized properly:
**						TRUE  => everything was initialized okay
**						FALSE => everything was NOT initialized okay
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int	UtlConfigMulti_Initialize()
{
	int			isOkay = FALSE;					/* the returned status		*/
	char*	func="UtlConfigMulti_Initialize";	/* ptr to name of function	*/

/*	Create a queue for the Configuration File List.
	----------------------------------------------- */
	if ( !UtlQueue_Create(&m_ConfigFileList) ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Could not create the Configuration File List queue\n");
		goto _errQueueCreate;
	}

/*	Everything went okay, so indicate a good return status.
	------------------------------------------------------- */
	isOkay = TRUE;

/*	Error conditions were detected, so indicate a bad return status.
	---------------------------------------------------------------- */
_errQueueCreate:

/*	Return the status to the caller.
	-------------------------------- */
	return(isOkay);
}

/*
** Name:
**		UtlConfigMulti_Load
**
** Purpose
**		This public function is used to load configuration information from
**		a specified configuration file into a caller's global string and/or
**		numeric items that were established as a result of previous calls to
**		UtlConfigMulti_DefineItem.
**
** Calling Sequence:
**		isOkay = UtlConfigMulti_Load(hConfigFile);
**
** Input:
**		hConfigFile		handle to configuration file
**
** Output:
**		NONE
**
** Returns:
**		isOkay		whether or not the configuration file was
**					loaded properly:
**						TRUE  => everything was loaded okay
**						FALSE => everything was NOT loaded okay
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int UtlConfigMulti_Load(
		HANDLE	hConfigFile)	/* handle to configuration file				*/
{
	CFG_STATE_INFO*	pConfigInfo;	/* ptr to config file state info		*/
	int			isOkay = FALSE;		/* the returned status					*/
	int			posID;				/* counter for identifiers				*/
	int			posVal;				/* counter for values					*/
	int			doneParsing;		/* the done parsing config file flag	*/
	int			doingArray;			/* we are processing an array flag		*/
	int			typeArray;			/* the type of array being processed	*/
	char		ch;					/* the character being processed		*/
	int			thisLine;			/* current line number in config file	*/
	int			gotEscape;			/* got an escape character active 		*/
	ID_STR		strID;				/* buffer for a config item identifier	*/
	STR_STR		strVal;				/* buffer for a config item value		*/
	int			currState;			/* current state of the state machine	*/
	char*	func="UtlConfigMulti_Load";		/* ptr to name of function		*/

/*	Validate this configuration file handle.
	---------------------------------------- */
	if ( !validate_handle(hConfigFile) ) {
		goto _errValidateHandle;
	}
	pConfigInfo = (CFG_STATE_INFO*)hConfigFile;

/*	Initialize some information for the state machine processing.
	------------------------------------------------------------- */
	posID		= 0;
	posVal		= 0;
	doneParsing	= FALSE;
	doingArray	= FALSE;
	gotEscape	= FALSE;
	thisLine	= 0;
	currState	= SEEK_ID_STATE;

/*	Indicate that we have not read any input from the configuration
	file at this time.
	--------------------------------------------------------------- */
	pConfigInfo->theInputCurrPos = -1;
	pConfigInfo->theInputLastCh  = CHR_NEWLINE;
	
/*	Loop through the entire file, one character at a time and parse
	the entire file as a state machine.  We will decide what we are
	currently doing as we examine each character.  After we read in
	a character, we are doing one of the following:
	
		1) processing a config item identifier
		2) processing a config item value(s)
	
	If we are processing the value of a config item, we currently 
	support the following types:
	
		a) a string value		(delimited by double quotes '"')
		b) a numeric value		(contains '0' -> '9' with an optional
								 '+' or '-' as the first character)
	------------------------------------------------------------------ */							  
	while ( !doneParsing ) {
	
/*		Get the next character from the file.
		------------------------------------- */
		ch = get_next_char(hConfigFile, &thisLine);
		
/*		Process this character according to our current state.
		------------------------------------------------------ */
		switch (currState) {
		
/*			We are seeking a config item identifier.
			---------------------------------------- */
			case SEEK_ID_STATE:
				if ( !process_SEEK_ID_STATE(hConfigFile, ch, thisLine,
											&posID, strID, &currState) ) {
					goto _errProcessSeekIdState;
				}
				break;
			
/*			We are reading a config item identifier.
			---------------------------------------- */
			case READ_ID_STATE:
				if ( !process_READ_ID_STATE(hConfigFile, ch, thisLine,
											&posID, strID, &currState) ) {
					goto _errProcessReadIdState;
				}
				break;
				
/*			We are seeking a config item value.
			----------------------------------- */
			case SEEK_VAL_STATE:
				if ( !process_SEEK_VAL_STATE(hConfigFile,
											 ch, thisLine, &typeArray,
											 &doingArray, &posID, strID,
											 &posVal, strVal, &currState) ) {
					goto _errProcessSeekValState;
				}
				break;
				
/*			We are reading an integer config item value.
			-------------------------------------------- */
			case READ_INT_STATE:
				if ( !process_READ_INT_STATE(hConfigFile,
											 ch, thisLine, &typeArray,
											 &doingArray, &posID, strID,
											 &posVal, strVal, &currState) ) {
					goto _errProcessReadIntState;
				}
				break;
				
/*			We are reading a string config item value.
			------------------------------------------ */
			case READ_STR_STATE:
				if ( !process_READ_STR_STATE(hConfigFile,
											 ch, thisLine, &typeArray,
											 &doingArray, &gotEscape,
											 &posID, strID, &posVal, strVal,
											 &currState) ) {
					goto _errProcessReadStrState;
				}
				break;
					
/*			We have an unknown state if we fall through to here.
			---------------------------------------------------- */
			default:
				UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
					"ERROR: Unknown state in state machine = %d.\n", currState);
				goto _errUnknownState;
				break;
		} /* end "switch (..." */
			
/*		If we have arrived at the end of the file, we are all done.
		----------------------------------------------------------- */
		if (ch == CHR_END_OF_FILE) {
			doneParsing = TRUE;
		}

	} /* end "while (..." */									
	  
/*	Close the file and return a good status to the caller after first
	indicating that the NEXT call to this function will result in a 
	RECONFIGURATION (i.e., an UNLOAD will be done before the LOAD).
	----------------------------------------------------------------- */
	fclose(pConfigInfo->theConfigFileDescr);
	pConfigInfo->theConfigMode = CFG_MODE_RECONFIGURE;

/*	Everything went okay, so indicate a good return status.
	------------------------------------------------------- */
	isOkay = TRUE;

/*	Error conditions were detected, so indicate a bad return status.
	---------------------------------------------------------------- */
_errUnknownState:
_errProcessReadStrState:
_errProcessReadIntState:
_errProcessSeekValState:
_errProcessReadIdState:
_errProcessSeekIdState:
_errValidateHandle:

/*	Return the status to the caller.
	-------------------------------- */
	return(isOkay);
}

/*
** Name:
**		UtlConfigMulti_Open
**
** Purpose:
**		This public function is used to open a specified configuration file.
**
** Calling Sequence:
**		isOkay = UtlConfigMulti_Open(pConfigFileName, phConfigFile);
**
** Input:
**		pConfigFileName		ptr to name of the configuration file
**
** Output:
**		phConfigFile		ptr to returned configuration file handle
**
** Returns:
**		isOkay		whether or not the configuration file was
**					opened properly:
**						TRUE  => everything was opened okay
**						FALSE => everything was NOT opened okay
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int	UtlConfigMulti_Open(
		char*	pConfigFileName,/* ptr to name of the config file		*/
		HANDLE*	phConfigFile)	/* ptr to returned config file handle	*/
{
	CFG_STATE_INFO*	pConfigInfo;	/* ptr to config file state info	*/
	int			isOkay = FALSE;		/* the returned status				*/
	char*	func="UtlConfigMulti_Open";		/* ptr to name of function	*/

/*	Allocate memory for some context and state information about this
	configuration file.
	----------------------------------------------------------------- */
	pConfigInfo = (CFG_STATE_INFO *)calloc(1, sizeof(CFG_STATE_INFO));
	if (pConfigInfo == NULL) {
		goto _errNoMemForConfigInfo;
	}

	/*	Open the configuration input file.
	---------------------------------- */
	pConfigInfo->theConfigFileDescr = fopen(pConfigFileName, "r");
	if (pConfigInfo->theConfigFileDescr == NULL) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Could not open configuration file \"%s\".\n",
				pConfigFileName);
		goto _errOpenConfigFile;
	}

/*	Initialize any other context and state information for this
	configuration file.
	----------------------------------------------------------- */
	pConfigInfo->theConfigMode = CFG_MODE_CONFIGURE;

/*	Add this context and state information structure to the Configuration
	File List queue.
	--------------------------------------------------------------------- */
	if(!UtlQueue_AddHead(m_ConfigFileList, pConfigInfo)) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Unable to add new configuration file to the list\n");
		goto _errAddNewConfigFile;
	}

/*	Bump up the number of open configuration files.
	----------------------------------------------- */
	m_ConfigFileCount++;

/*	Return the handle to this configuration file.
	--------------------------------------------- */
	*phConfigFile = (HANDLE*)pConfigInfo;

/*	Everything went okay, so indicate a good return status.
	------------------------------------------------------- */
	isOkay = TRUE;
	goto _allOkay;

/*	Error conditions were detected, so indicate a bad return status.
	---------------------------------------------------------------- */
_errAddNewConfigFile:
_errOpenConfigFile:
	if (pConfigInfo != NULL) {
		free(pConfigInfo);
	}
_errNoMemForConfigInfo:

/*	Return the status to the caller.
	-------------------------------- */
_allOkay:
	return(isOkay);
}

/*
** Name:
**		UtlConfigMulti_Terminate
**
** Purpose:
**		This public function is used to terminate the "utl_config_multi"
**		API functions.
**
** Calling Sequence:
**		isOkay = UtlConfigMulti_Terminate();
**
** Input:
**		NONE
**
** Output:
**		NONE
**
** Returns:
**		isOkay		whether or not the multiple configuration file
**					module was terminated properly:
**						TRUE  => everything was terminated okay
**						FALSE => everything was NOT terminated okay
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int	UtlConfigMulti_Terminate()
{
	int			isOkay = FALSE;					/* the returned status		*/
	char*	func="UtlConfigMulti_Terminate";	/* ptr to name of function	*/

/*	Delete the queue for the Configuration File List.
	------------------------------------------------- */
	if ( !UtlQueue_Delete(m_ConfigFileList) ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Could not delete the Configuration File List queue\n");
		goto _errQueueDelete;
	}

/*	Everything went okay, so indicate a good return status.
	------------------------------------------------------- */
	isOkay = TRUE;

/*	Error conditions were detected, so indicate a bad return status.
	---------------------------------------------------------------- */
_errQueueDelete:

/*	Return the status to the caller.
	-------------------------------- */
	return(isOkay);
}
		
/*
** Name:
**		UtlConfigMulti_Unload
**
** Purpose
**		This public function is used to unload a previous configuration file
**		that was loaded by freeing all the memory allocated for global
**		configuration items and then making sure the linked list of the
**		configuration items is forced into an empty state.
**
** Calling Sequence:
**		isOkay = UtlConfigMulti_Unload(hConfigFile);
**
** Input:
**		hConfigFile		handle to configuration file
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
int UtlConfigMulti_Unload(
		HANDLE	hConfigFile)	/* handle to configuration file				*/
{
	CFG_STATE_INFO*	pConfigInfo;	/* ptr to config file state info	*/
	int			isOkay = FALSE;		/* the returned status				*/
	CFG_ITEM*	pThisItem;			/* ptr to the current config item	*/
	CFG_ITEM*	pNextItem;			/* ptr to the next config item		*/
	char**		ppThisItemVal;		/* ptr to ptr to config item value	*/
	int			ndx;				/* loop counter						*/
	char*	func="UtlConfigMulti_Unload";	/* ptr to name of this func	*/

/*	Validate this configuration file handle.
	---------------------------------------- */
	if ( !validate_handle(hConfigFile) ) {
		goto _errValidateHandle;
	}
	pConfigInfo = (CFG_STATE_INFO*)hConfigFile;

/*	Get the first config item from the linked list.
	----------------------------------------------- */
	pThisItem = pConfigInfo->theConfigListHead;
	
/*	Run through the linked list and free up all the memory for each
	config item.
	--------------------------------------------------------------- */
	while (pThisItem != NULL) {
		pNextItem = pThisItem->pNextItem;
		
/*		If the config item is actually an array of values, then 
		remember to free the array's memory before freeing the
		config item's memory.
		------------------------------------------------------- */
		if (pThisItem->isArray) {
			ppThisItemVal = (char**)*pThisItem->ppItemVal;
			for (ndx=0; ndx<pThisItem->countArray; ndx++) {
				free(*ppThisItemVal);
				ppThisItemVal++;
			}
			free(*(pThisItem->ppItemVal));
			
/*		If the config item was of type CFG_STR_TYPE (which is a string),
		then remember to free the string's memory before freeing the
		config item's memory.
		---------------------------------------------------------------- */
		} else if (pThisItem->typeItem == CFG_STR_TYPE) {
			free(*(pThisItem->ppItemVal));
		}
		free(pThisItem->pItemID);
		free(pThisItem);
		pThisItem = pNextItem;
	}
	
/*	Now that all the memory has been freed up, make sure the linked
	list is forced to show an empty state.
	--------------------------------------------------------------- */
	pConfigInfo->theConfigListHead = pConfigInfo->theConfigListTail = NULL;

/*	Everything went okay, so indicate a good return status.
	------------------------------------------------------- */
	isOkay = TRUE;

/*	Error conditions were detected, so indicate a bad return status.
	---------------------------------------------------------------- */
_errValidateHandle:

/*	Return the status to the caller.
	-------------------------------- */
	return(isOkay);
}		

/*
** Name:
**		array_process_init
**
** Purpose
**		This local function initializes the state machine for processing
**		of a config item that is being defined as an array of values.
**
** Calling Sequence:
**		array_process_init(hConfigFile);
**
** Input:
**		hConfigFile		handle to configuration file
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
static void array_process_init(
	HANDLE	hConfigFile)		/* handle to configuration file				*/
{
	CFG_STATE_INFO*		pConfigInfo;	/* ptr to config file state info	*/
	char*	func="array_process_init";	/* ptr to name of function			*/

/*	Get the pointer to the REAL configuration file context information.
	------------------------------------------------------------------- */
	pConfigInfo = (CFG_STATE_INFO*)hConfigFile;
	
/*	Allocate a large chunk of memory to store the temporary array pointers
	and then indicate that there are no array pointers at this time.
	---------------------------------------------------------------------- */
	pConfigInfo->theArrayPtrs =
					(char**)calloc( 1, (MAX_ARRAY_PTRS * sizeof(char*)) );
	if (pConfigInfo->theArrayPtrs == NULL) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Could not allocate memory for array processing\n");
		return;
	}
	pConfigInfo->theArrayCount = 0;

/*	Return to the caller.
	--------------------- */
	return;
}

/*
** Name:
**		array_process_term
**
** Purpose
**		This local function terminates the state machine for processing
**		of a config item that is being defined as an array of values.
**
** Calling Sequence:
**		array_process_term(hConfigFile);
**
** Input:
**		hConfigFile		handle to configuration file
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
static void array_process_term(
	HANDLE	hConfigFile)		/* handle to configuration file				*/
{
	CFG_STATE_INFO*		pConfigInfo;	/* ptr to config file state info	*/
	int					ndx;			/* the loop counter					*/
	char*	func="array_process_term";	/* ptr to name of function			*/

/*	Get the pointer to the REAL configuration file context information.
	------------------------------------------------------------------- */
	pConfigInfo = (CFG_STATE_INFO*)hConfigFile;

/*	If the temporary array pointers were never loaded into an array item
	(because no match was found, for example, in the configuration file),
	then free them at this time.
	--------------------------------------------------------------------- */
	if (pConfigInfo->theArrayCount > 0) {
		for (ndx=0; ndx<pConfigInfo->theArrayCount; ndx++) {
			if ( *(pConfigInfo->theArrayPtrs + ndx) != NULL ) {
				free( *(pConfigInfo->theArrayPtrs + ndx) );
			}
		}
	}
	
/*	Free the chunk of memory that was used to store the temporary array
	pointers and indicate that there are no array pointers at this time.
	-------------------------------------------------------------------- */
	free(pConfigInfo->theArrayPtrs);
	pConfigInfo->theArrayCount = 0;
}
	
/*
** Name:
**		get_next_char
**
** Purpose
**		This local function gets the next character from a
**		specified configuration file.  If a COMMENT character
**		is found that does NOT have an ESCAPE character in
**		front of it, the rest of the current input line is
**		discarded and the next input line is read from the
**		configuration file.
**
** Calling Sequence:
**		ch = get_next_char(hConfigFile, pThisLine);
**
** Input:
**		hConfigFile			handle to configuration file
**
** Output:
**		pThisLine			ptr to the returned line number of
**							the configuration file where the
**							returned character was found
**
** Returns:
**		ch			the returned character from the configuration file
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
static char get_next_char(
	HANDLE	hConfigFile,		/* handle to configuration file				*/
	int*	pThisLine)			/* ptr to returned line number				*/
{
	CFG_STATE_INFO*		pConfigInfo;	/* ptr to config file state info	*/
	char				nextCh;			/* the next character				*/						
	int					gotNextCh;		/* got the next char flag			*/
	char*	func="get_next_char";		/* ptr to name of function			*/

/*	Get the pointer to the REAL configuration file context information.
	------------------------------------------------------------------- */
	pConfigInfo = (CFG_STATE_INFO*)hConfigFile;

/*	Loop through the characters of the input configuration file, 
	looking for the first character that can be returned to the
	caller.  Any COMMENT character that is not preceeded by an
	ESCAPE character and all the characters AFTER that COMMENT
	character are considered non-returnable (they are swallowed
	by this function).
	------------------------------------------------------------ */
	gotNextCh = FALSE;
	while ( !gotNextCh ) {

/*		Read a new input string into our buffer if necessary.  This will
		be true if either our current Input Position (pConfigInfo->theInputCurrPos) is 
		at the end of the current Input String (pConfigInfo->theInputStr) OR this is the
		special case where the configuration file has just been opened and
		we need to read in the initial input string into our buffer.  After
		reading in the next input string, remember to bump the current line
		number.
		------------------------------------------------------------------- */
		if ( (pConfigInfo->theInputCurrPos >= pConfigInfo->theInputLastPos) ||
			 (pConfigInfo->theInputCurrPos < 0) ) {
			if (fgets(pConfigInfo->theInputStr,
					  sizeof(pConfigInfo->theInputStr),
					  pConfigInfo->theConfigFileDescr) != NULL) {
				pConfigInfo->theInputCurrPos = 0;
				pConfigInfo->theInputLastPos = strlen(pConfigInfo->theInputStr);
				(*pThisLine)++;
			} else {
				if ( feof(pConfigInfo->theConfigFileDescr) ) {
					return(CHR_END_OF_FILE);
				} else {
					UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
						"ERROR: Could not read from config file, err = %d.\n",
							ferror(pConfigInfo->theConfigFileDescr));
					return(CHR_END_OF_FILE);
				}
			}
		}

/*		If the current character is a NEWLINE character, we just
		need to bump the current Input Position (theInputCurrPos),
		which will ARTIFICIALLY cause the next input string to be
		read from the configuration file.
		--------------------------------------------------------- */
		if (pConfigInfo->theInputStr[pConfigInfo->theInputCurrPos] ==
															CHR_NEWLINE) {
			pConfigInfo->theInputCurrPos++;

/*		If the current character in our input string IS NOT a COMMENT
		character, then it can be returned to the caller.  We need to
		remember this character in case it happens to be an ESCAPE
		character when we examine the next returnable character.
		------------------------------------------------------------- */
		} else if (pConfigInfo->theInputStr[pConfigInfo->theInputCurrPos] !=
															CHR_COMMENT) {
			nextCh = pConfigInfo->theInputStr[pConfigInfo->theInputCurrPos];
			gotNextCh = TRUE;
			pConfigInfo->theInputLastCh = nextCh;
			pConfigInfo->theInputCurrPos++;

/*		The current character IS a COMMENT character, so we need to
		determine if there was a previous ESCAPE character in our
		input string.  If so, this is not really a COMMENT character
		and is, therefore, returnable to the caller.
		------------------------------------------------------------ */
		} else if (pConfigInfo->theInputLastCh == CHR_ESCAPE) {
			nextCh = pConfigInfo->theInputStr[pConfigInfo->theInputCurrPos];
			gotNextCh = TRUE;
			pConfigInfo->theInputLastCh = nextCh;
			pConfigInfo->theInputCurrPos++;

/*		We have a real COMMENT character and need to make sure we ignore
		this character and all its following characters in the current
		input string.  The easiest way to do this is to ARTIFICIALLY
		set the current Input Position (theInputCurrPos) to be the
		last Input Position (theInputLastPos), which will force the
		next line of the input configuration file to be read.
		---------------------------------------------------------------- */
		} else {
			pConfigInfo->theInputCurrPos = pConfigInfo->theInputLastPos;
		}
	}

/*	We have the next character, so return it!
	----------------------------------------- */
	return(nextCh);
}

/*
** Name:
**		load_array_item
**
** Purpose
**		This local function loads an array of config item values for a 
**		config item identifier of a specified type.
**
** Calling Sequence:
**		wasFound = load_array_item(hConfigFile, pItemID, typeItem, thisLine);
**
** Input:
**		hConfigFile		handle to configuration file
**		pItemID			ptr to the config item identifier string
**		typeItem		the type of the config item
**		thisLine		current line number in config file
**
** Output:
**		NONE
**
** Returns:
**		wasFound	whether or not the config item identifier was found:
**						TRUE  => the config item identifier was found
**						FALSE => the config item identifier was NOT found
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
static int load_array_item(
	HANDLE	hConfigFile,		/* handle to configuration file				*/
	char*	pItemID,			/* ptr to config item identifier			*/
	int		typeItem,			/* the type of the config item				*/
	int		thisLine)			/* current line number in config file		*/
{
	CFG_STATE_INFO*		pConfigInfo;	/* ptr to config file state info	*/
	CFG_ITEM*			pThisItem;		/* ptr to config item in linked list*/
	int					wasFound;		/* config item was found flag		*/
	char*				c_temp;			/* ptr to temp char buffer			*/	  
	char*	func="load_array_item";		/* ptr to name of function			*/

/*	Get the pointer to the REAL configuration file context information.
	------------------------------------------------------------------- */
	pConfigInfo = (CFG_STATE_INFO*)hConfigFile;
	
/*	Assume the config item has NOT been found in our linked list of
	config items.
	--------------------------------------------------------------- */
	wasFound = FALSE;
	
/*	Start at the beginning of the linked list and search for a match
	with the config item identifier string that has been passed into
	this function.
	---------------------------------------------------------------- */
	pThisItem = pConfigInfo->theConfigListHead;
	while (pThisItem != NULL) {
	
/*		If the identifier read from the config file matches the config item
		identifier of the current config item, then load this item.
		------------------------------------------------------------------- */
		if ( strcmp(pItemID, pThisItem->pItemID) == 0 ) {

/*			If the count of array values is 0, the user forgot to define
			any values, so report this as an error.
			------------------------------------------------------------ */
			if (pConfigInfo->theArrayCount <= 0) {
				UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
					"ERROR: Config item ID \"%s\" has no array values.\n",
						pItemID);
					return(FALSE);
			}
		
/*			Save the type info for unloading at a later time.  Also save
			information about whether or not this is an array item and how
			many items are in the array.
			-------------------------------------------------------------- */
			pThisItem->typeItem   = typeItem;
			pThisItem->isArray	  = TRUE;
			pThisItem->countArray = pConfigInfo->theArrayCount;
			
/*			Since this is an array of items, a temporary array of ptrs has
			been created for either CFG_STR_TYPE or CFG_INT_TYPE.  So, just
			copy the array of ptrs to a newly allocated chunk of memory and
			then save off the pointer to the newly allocated memory.
			--------------------------------------------------------------- */
			if ( (typeItem == CFG_STR_TYPE) || (typeItem == CFG_INT_TYPE) ) {
				c_temp = (char*)calloc(
							1, (pConfigInfo->theArrayCount * sizeof(char*)) );
				if (c_temp == NULL) {
					UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
						"ERROR: No more memory for config item \"%s\"\n",
							pThisItem->pItemID);
					return(FALSE);
				}
				memcpy(c_temp, pConfigInfo->theArrayPtrs,
							(pConfigInfo->theArrayCount * sizeof(char*)));
				*(pThisItem->ppItemVal) = c_temp;

				memset(pConfigInfo->theArrayPtrs, 0,
							(pConfigInfo->theArrayCount * sizeof(char*)));
				wasFound = TRUE;
				
/*			Otherwise, this config item type is unknown, so we need to
			report an error and then move on.
			---------------------------------------------------------- */
			} else {
				UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
					"ERROR: Unknown configuration type = %d.\n", typeItem);
				return(FALSE);
			}
		}
		
/*		Bump to the next config item in the linked list.
		------------------------------------------------ */
		pThisItem = pThisItem->pNextItem;
	}

/*	If a match was not found, put out a warning to the user and then
	artificially make it look like we DID find a match.  This allows
	a configuration file to have entries that are NOT actually loaded
	by an application.
	----------------------------------------------------------------- */
	if (!wasFound) {
		UtlLog_Write(LOG_WARN | LOG_NO_ID, func, RIGHT_HERE,
			"NOTE: Ignoring unknown config ID \"%s\" (line %d).\n",
				pItemID, thisLine);
		wasFound = TRUE;
	}
	
/*	Return whether or not we have found a match for this config item.
	----------------------------------------------------------------- */
	return(wasFound);
}
		
/*
** Name:
**		load_item
**
** Purpose
**		This local function loads a config item value for a config
**		item identifier of a specified type.
**
** Calling Sequence:
**		wasFound = load_item(hConfigFile, pItemID, pItemVal,
**							 typeItem, thisLine);
**
** Input:
**		hConfigFile		handle to configuration file
**		pItemID			ptr to the config item identifier string
**		pItemVal		ptr to the config item value
**		typeItem		the type of the config item
**		thisLine		current line number in config file
**
** Output:
**		NONE
**
** Returns:
**		wasFound	whether or not the config item identifier was found:
**						TRUE  => the config item identifier was found
**						FALSE => the config item identifier was NOT found
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
static int load_item(
	HANDLE	hConfigFile,		/* handle to configuration file				*/
	char*	pItemID,			/* ptr to config item identifier			*/
	char*	pItemVal,			/* ptr to the config item value				*/
	int		typeItem,			/* the type of the config item				*/
	int		thisLine)			/* current line number in config file		*/
{
	CFG_STATE_INFO*		pConfigInfo;	/* ptr to config file state info	*/
	CFG_ITEM*			pThisItem;		/* ptr to config item in linked list*/
	int					wasFound;		/* config item was found flag		*/
	char*				c_temp;			/* ptr to temp char buffer			*/
	char*	func="load_item";			/* ptr to name of function			*/

/*	Get the pointer to the REAL configuration file context information.
	------------------------------------------------------------------- */
	pConfigInfo = (CFG_STATE_INFO*)hConfigFile;
	
/*	Assume the config item has NOT been found in our linked list of
	config items.
	--------------------------------------------------------------- */
	wasFound = FALSE;
	
/*	Start at the beginning of the linked list and search for a match
	with the config item identifier string that has been passed into
	this function.
	---------------------------------------------------------------- */
	pThisItem = pConfigInfo->theConfigListHead;
	while (pThisItem != NULL) {
	
/*		If the identifier read from the config file matches the config item
		identifier of the current config item, then load this item.
		------------------------------------------------------------------- */
		if ( strcmp(pItemID, pThisItem->pItemID) == 0 ) {
		
/*			Save the type info for unloading at a later time.  Also save
			information about whether or not this is an array item and how
			many items are in the array.
			-------------------------------------------------------------- */
			pThisItem->typeItem   = typeItem;
			pThisItem->isArray	= FALSE;
			pThisItem->countArray = 1;
			
/*			If this config item type is a CFG_STR_TYPE (which is a string),
			then we need to allocate some memory for it and save off the
			pointer to the newly allocated memory.
			--------------------------------------------------------------- */
			if (typeItem == CFG_STR_TYPE) {
				c_temp = (char*)calloc(1, strlen(pItemVal) + 1);
				if (c_temp == NULL) {
					UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
						"ERROR: No more memory for config item \"%s\"\n",
							pThisItem->pItemID);
					return(FALSE);
				}
				strcpy(c_temp, pItemVal);
				*(pThisItem->ppItemVal) = c_temp;
				wasFound = TRUE;
				
/*			If this config item type is a CFG_INT_TYPE (which is an
			integer), we just need to convert the input string to a
			nice number.
			------------------------------------------------------- */
			} else if (typeItem == CFG_INT_TYPE) {
				*(pThisItem->ppItemVal) = (char*)atoi(pItemVal);
				wasFound = TRUE;
				
/*			Otherwise, this config item type is unknown, so we need to
			report an error and then move on.
			---------------------------------------------------------- */
			} else {
				UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
					"ERROR: Unknown configuration type = %d (line %d).\n",
						typeItem, thisLine);
				return(FALSE);
			}
		}
		
/*		Bump to the next config item in the linked list.
		------------------------------------------------ */
		pThisItem = pThisItem->pNextItem;
	}

/*	If a match was not found, put out a warning to the user and then
	artificially make it look like we DID find a match.  This allows
	a configuration file to have entries that are NOT actually loaded
	by an application.
	----------------------------------------------------------------- */
	if (!wasFound) {
		UtlLog_Write(LOG_WARN | LOG_NO_ID, func, RIGHT_HERE,
			"NOTE: Ignoring unknown config ID \"%s\" (line %d).\n",
				pItemID, thisLine);
		wasFound = TRUE;
	}
	
/*	Return whether or not we have found a match for this config item.
	----------------------------------------------------------------- */
	return(wasFound);
}		

/*
** Name:
**		process_READ_ID_STATE
**
** Purpose
**		This local function processes the READ_ID_STATE of the
**		state machine that is parsing the current configuration file.
**
** Calling Sequence:
**		isOkay = process_READ_ID_STATE(hConfigFile, ch, thisLine, pPosID,
**									   pStrID, pCurrState);
**
** Input:
**		hConfigFile		handle to configuration file
**		ch				the character being processed
**		thisLine		current line number in config file
**
** Output:
**		pPosID			ptr to counter for identifier
**		pStrID			ptr to a config item identifier string
**		pCurrState		ptr to current state of state machine
*
** Returns:
**		isOkay		whether or not the READ_ID_STATE was processed
**					properly:
**						TRUE  => everything was processed okay
**						FALSE => everything was NOT processed okay
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
static int process_READ_ID_STATE(
	HANDLE	hConfigFile,		/* handle to configuration file				*/
	char	ch,					/* the character being processed			*/
	int		thisLine,			/* current line number in config file		*/
	int*	pPosID,				/* ptr to counter for identifier			*/
	char*	pStrID,				/* ptr to a config item identifier string	*/
	int*	pCurrState)			/* ptr to current state of state machine	*/
{
	CFG_STATE_INFO*		pConfigInfo;	/* ptr to config file state info	*/
	char*	func="process_READ_ID_STATE";	/* ptr to name of function		*/

/*	Get the pointer to the REAL configuration file context information.
	------------------------------------------------------------------- */
	pConfigInfo = (CFG_STATE_INFO*)hConfigFile;
			
/*	We are reading a config item identifier, so make sure we
	add this character to the current identifier string if
	it is valid (anything ALPHANUMERIC or an UNDERSCORE).
	-------------------------------------------------------- */
	if ( (ch == CHR_UNDERSCORE) || isalnum(ch) ) {
		if (*pPosID < MAX_ID_LEN) {
			*(pStrID + (*pPosID)++) = ch;
		} else {
			*(pStrID + *pPosID) = '\0';
			UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
				"ERROR: Config item ID name is too long (line %d):\n",
					thisLine);
			UtlLog_Write(LOG_ERROR | LOG_NO_ID, func, RIGHT_HERE,
				"%s\n", pStrID);
			return(FALSE);
		}

/*	If we have reached the end of the file, then this config
	item identifier does not have an associated value.
	-------------------------------------------------------- */
	} else if (ch == CHR_END_OF_FILE) {
		*(pStrID + *pPosID) = '\0';
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Config item ID \"%s\" has no value (line %d).\n",
				pStrID, thisLine);
		return(FALSE);

/*	If this character is NOT white space, we have an invalid
	config item identifier.
	-------------------------------------------------------- */
	} else if ( !isspace(ch) ) {
		*(pStrID + *pPosID) = '\0';
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Invalid Config item ID: \"%s\" (line %d).\n",
				pStrID, thisLine);
		return(FALSE);

/*	This character IS white space, so we have completed defining
	a valid config item identifier.  It is now time to set the
	state of our state machine to seek an associated value for
	the config item identifier.
	------------------------------------------------------------ */
	} else if (*pPosID > 0) {
		*pCurrState = SEEK_VAL_STATE;
		*(pStrID + *pPosID) = '\0';
	} else {
		*pCurrState = SEEK_ID_STATE;
	}

/*	Return a good status to the caller.
	----------------------------------- */
	return(TRUE);
}

/*
** Name:
**		process_READ_INT_STATE
**
** Purpose
**		This local function processes the READ_INT_STATE of the
**		state machine that is parsing the current configuration file.
**
** Calling Sequence:
**		isOkay = process_READ_INT_STATE(hConfigFile,
**										ch, thisLine, pTypeArray, pDoingArray,
**										pPosID, pStrID, pPosVal, pStrVal,
**									    pCurrState);
**
** Input:
**		hConfigFile		handle to configuration file
**		ch				the character being processed
**		thisLine		current line number in config file
**
** Output:
**		pTypeArray		ptr to the type of array being processed
**		pDoingArray		ptr to doing an array flag
**		pPosID			ptr to counter for identifier
**		pStrID			ptr to a config item identifier string
**		pPosVal			ptr to counter for value
**		pStrVal			ptr to a config item value string
**		pCurrState		ptr to current state of state machine
*
** Returns:
**		isOkay		whether or not the READ_INT_STATE was processed
**					properly:
**						TRUE  => everything was processed okay
**						FALSE => everything was NOT processed okay
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
static int process_READ_INT_STATE(
	HANDLE	hConfigFile,		/* handle to configuration file				*/
	char	ch,					/* the character being processed			*/
	int		thisLine,			/* current line number in config file		*/
	int*	pTypeArray,			/* ptr to the type of array being processed	*/
	int*	pDoingArray,		/* ptr to doing an array flag				*/
	int*	pPosID,				/* ptr to counter for identifier			*/
	char*	pStrID,				/* ptr to a config item identifier string	*/
	int*	pPosVal,			/* ptr to counter for value					*/
	char*	pStrVal,			/* ptr to a config item value string		*/
	int*	pCurrState)			/* ptr to current state of state machine	*/
{
	CFG_STATE_INFO*		pConfigInfo;	/* ptr to config file state info	*/
	char*	func="process_READ_INT_STATE";	/* ptr to name of function		*/

/*	Get the pointer to the REAL configuration file context information.
	------------------------------------------------------------------- */
	pConfigInfo = (CFG_STATE_INFO*)hConfigFile;
				
/*	We are reading config item integer value, so make sure we add
	this character to the current value string if it is valid (this
	would be any digit).
	--------------------------------------------------------------- */
	if ( isdigit(ch) ) {
		if (*pPosVal < MAX_INT_LEN) {
			*(pStrVal + (*pPosVal)++) = ch;
		} else {
			*(pStrID + *pPosID) = '\0';
			UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
				"ERROR: Config item ID \"%s\" value too long (line %d).\n",
					pStrID, thisLine);
			return(FALSE);
		}
			
/*	If this character IS NOT white space, we have an invalid
	config item integer value.
	-------------------------------------------------------- */
	} else if ( !isspace(ch) ) {
		*(pStrID + *pPosID) = '\0';
		*(pStrVal + (*pPosVal)++) = ch;
		*(pStrVal + (*pPosVal)) = '\0';
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Config item ID \"%s\" has a bad value=\"%s\" (line %d).\n",
				pStrID, pStrVal, thisLine);
		return(FALSE);

/*	This character IS white space, so we have completed defining
	a valid config item integer value.
	------------------------------------------------------------ */
	} else {
		*(pStrVal + (*pPosVal)) = '\0';
		if ( !(*pDoingArray) ) {

/*			We are NOT doing an array, so we can now load this config
			item integer value and then set the state of our state
			machine to seek the next config item identifier.
			--------------------------------------------------------- */
			if ( !load_item(hConfigFile, pStrID, pStrVal, CFG_INT_TYPE,
							thisLine) ) {
				return(FALSE);
			}
			*pCurrState = SEEK_ID_STATE;
		} else {

/*			We ARE doing an array, so we should just save this config
			item integer value and then set the state of our state
			machine to seek the next config item integer value.
			--------------------------------------------------------- */
			if ( !save_array_value(hConfigFile, pStrVal, CFG_INT_TYPE) ) {
				*(pStrID + *pPosID) = '\0';
				UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
					"ERROR: Could not save value (line %d).\n", thisLine);
				return(FALSE);
			}
			*pTypeArray = CFG_INT_TYPE;
			*pCurrState = SEEK_VAL_STATE;	
		}	
	}

/*	Return a good status to the caller.
	----------------------------------- */
	return(TRUE);
}

/*
** Name:
**		process_READ_STR_STATE
**
** Purpose
**		This local function processes the READ_STR_STATE of the
**		state machine that is parsing the current configuration file.
**
** Calling Sequence:
**		isOkay = process_READ_STR_STATE(hConfigFile,
**										ch, thisLine, pTypeArray,
**										pDoingArray, pGotEscape,
**										pPosID, pStrID, pPosVal, pStrVal,
**									    pCurrState);
**
** Input:
**		hConfigFile		handle to configuration file
**		ch				the character being processed
**		thisLine		current line number in config file
**
** Output:
**		pTypeArray		ptr to the type of array being processed
**		pDoingArray		ptr to doing an array flag
**		pGotEscape		ptr to got an escape character active 
**		pPosID			ptr to counter for identifier
**		pStrID			ptr to a config item identifier string
**		pPosVal			ptr to counter for value
**		pStrVal			ptr to a config item value string
**		pCurrState		ptr to current state of state machine
*
** Returns:
**		isOkay		whether or not the READ_STR_STATE was processed
**					properly:
**						TRUE  => everything was processed okay
**						FALSE => everything was NOT processed okay
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
static int process_READ_STR_STATE(
	HANDLE	hConfigFile,		/* handle to configuration file				*/
	char	ch,					/* the character being processed			*/
	int		thisLine,			/* current line number in config file		*/
	int*	pTypeArray,			/* ptr to the type of array being processed	*/
	int*	pDoingArray,		/* ptr to doing an array flag				*/
	int*	pGotEscape,			/* ptr to got an escape character active	*/ 
	int*	pPosID,				/* ptr to counter for identifier			*/
	char*	pStrID,				/* ptr to a config item identifier string	*/
	int*	pPosVal,			/* ptr to counter for value					*/
	char*	pStrVal,			/* ptr to a config item value string		*/
	int*	pCurrState)			/* ptr to current state of state machine	*/
{
	CFG_STATE_INFO*		pConfigInfo;	/* ptr to config file state info	*/
	char*	func="process_READ_STR_STATE";	/* ptr to name of function		*/

/*	Get the pointer to the REAL configuration file context information.
	------------------------------------------------------------------- */
	pConfigInfo = (CFG_STATE_INFO*)hConfigFile;
				
/*	We are reading a config item string value, so make sure we
	add this character to the current value string if it is valid.
	(anything other than the STR_DELIMITER character, unless we
	have the ESCAPE character active).  Make sure we have not
	exceeded the maximum length for a string value.
	-------------------------------------------------------------- */
	if ( ((ch != CHR_STR_DELIMITER) || *pGotEscape) &&
		 (ch != CHR_END_OF_FILE) ) {
		if (*pPosVal < MAX_STR_LEN) {
			if (*pGotEscape) {
				*(pStrVal + (*pPosVal)++) = ch;
				*pGotEscape = FALSE;
			} else if (ch == CHR_ESCAPE) {
				*pGotEscape = TRUE;
			} else {
				*(pStrVal + (*pPosVal)++) = ch;
			}
		} else {
			*(pStrID + *pPosID) = '\0';
			UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
				"ERROR: Config item ID \"%s\" value is too long (line %d):\n",
					pStrID, thisLine);
			return(FALSE);
		}	

/*	If we have reached the end of the file, then this config
	item identifier does not have an associated value.
	-------------------------------------------------------- */
	} else if (ch == CHR_END_OF_FILE) {
		*(pStrID + *pPosID) = '\0';
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Config item ID \"%s\" has no value (line %d).\n",
				pStrID, thisLine);
		return(FALSE);
	} else {
				
/*	This is the STR_DELIMITER character, so we have completed defining
	a valid config item string value.
	------------------------------------------------------------------ */
		*(pStrVal + (*pPosVal)) = '\0';
		if ( !(*pDoingArray) ) {

/*			We are NOT doing an array, so we can now load this config
			item string value and then set the state of our state
			machine to seek the next config item identifier.
			--------------------------------------------------------- */
			if ( !load_item(hConfigFile, pStrID, pStrVal, CFG_STR_TYPE,
							thisLine) ) {
				return(FALSE);
			}
			*pCurrState = SEEK_ID_STATE;
		} else {

/*			We ARE doing an array, so we should just save this config
			item string value and then set the state of our state
			machine to seek the next config item string value.
			--------------------------------------------------------- */
			if ( !save_array_value(hConfigFile, pStrVal, CFG_STR_TYPE) ) {
				*(pStrID + *pPosID) = '\0';
				UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
					"ERROR: Could not save value (line %d).\n", thisLine);
				return(FALSE);
			}
			*pTypeArray = CFG_STR_TYPE;
			*pCurrState = SEEK_VAL_STATE;	
	   }	
	}

/*	Return a good status to the caller.
	----------------------------------- */
	return(TRUE);
}

/*
** Name:
**		process_SEEK_ID_STATE
**
** Purpose
**		This local function processes the SEEK_ID_STATE of the
**		state machine that is parsing the current configuration file.
**
** Calling Sequence:
**		isOkay = process_SEEK_ID_STATE(hConfigFile,
**									   ch, thisLine, pPosID, pStrID,
**									   pCurrState);
**
** Input:
**		hConfigFile		handle to configuration file
**		ch				the character being processed
**		thisLine		current line number in config file
**
** Output:
**		pPosID			ptr to counter for identifier
**		pStrID			ptr to a config item identifier string
**		pCurrState		ptr to current state of state machine
*
** Returns:
**		isOkay		whether or not the SEEK_ID_STATE was processed
**					properly:
**						TRUE  => everything was processed okay
**						FALSE => everything was NOT processed okay
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
static int process_SEEK_ID_STATE(
	HANDLE	hConfigFile,		/* handle to configuration file				*/
	char	ch,					/* the character being processed			*/
	int		thisLine,			/* current line number in config file		*/
	int*	pPosID,				/* ptr to counter for identifier			*/
	char*	pStrID,				/* ptr to a config item identifier string	*/
	int*	pCurrState)			/* ptr to current state of state machine	*/
{
	CFG_STATE_INFO*		pConfigInfo;	/* ptr to config file state info	*/
	char*	func="process_SEEK_ID_STATE";	/* ptr to name of function		*/

/*	Get the pointer to the REAL configuration file context information.
	------------------------------------------------------------------- */
	pConfigInfo = (CFG_STATE_INFO*)hConfigFile;

/*	We are seeking a config item identifier, so make sure we add
	this character to the current identifier string if it is valid
	(anything the is ALPHANUMERIC or the UNDERSCORE character).
	-------------------------------------------------------------- */
	if ( !(isspace(ch) || (ch == CHR_END_OF_FILE)) ) {
		if ( (ch == CHR_UNDERSCORE) || isalpha(ch) ) {
			*pCurrState = READ_ID_STATE;
			*pPosID = 0;
			*(pStrID + (*pPosID)++) = ch;
		} else {
			UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
				"ERROR: Invalid character (line %d): \"%c\"\n", thisLine, ch);
			return(FALSE);
		}
	}

/*	Return a good status to the caller.
	----------------------------------- */
	return(TRUE);
}

/*
** Name:
**		process_SEEK_VAL_STATE
**
** Purpose
**		This local function processes the SEEK_VAL_STATE of the
**		state machine that is parsing the current configuration file.
**
** Calling Sequence:
**		isOkay = process_SEEK_VAL_STATE(hConfigFile,
**										ch, thisLine, pTypeArray, pDoingArray,
**										pPosID, pStrID, pPosVal, pStrVal,
**									    pCurrState);
**
** Input:
**		hConfigFile		handle to configuration file
**		ch				the character being processed
**		thisLine		current line number in config file
**
** Output:
**		pTypeArray		ptr to the type of array being processed
**		pDoingArray		ptr to doing an array flag
**		pPosID			ptr to counter for identifier
**		pStrID			ptr to a config item identifier string
**		pPosVal			ptr to counter for value
**		pStrVal			ptr to a config item value string
**		pCurrState		ptr to current state of state machine
*
** Returns:
**		isOkay		whether or not the SEEK_ID_STATE was processed
**					properly:
**						TRUE  => everything was processed okay
**						FALSE => everything was NOT processed okay
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
static int process_SEEK_VAL_STATE(
	HANDLE	hConfigFile,		/* handle to configuration file				*/
	char	ch,					/* the character being processed			*/
	int		thisLine,			/* current line number in config file		*/
	int*	pTypeArray,			/* ptr to the type of array being processed	*/
	int*	pDoingArray,		/* ptr to doing an array flag				*/
	int*	pPosID,				/* ptr to counter for identifier			*/
	char*	pStrID,				/* ptr to a config item identifier string	*/
	int*	pPosVal,			/* ptr to counter for value					*/
	char*	pStrVal,			/* ptr to a config item value string		*/
	int*	pCurrState)			/* ptr to current state of state machine	*/
{
	CFG_STATE_INFO*		pConfigInfo;	/* ptr to config file state info	*/
	char*	func="process_SEEK_VAL_STATE";	/* ptr to name of function		*/

/*	Get the pointer to the REAL configuration file context information.
	------------------------------------------------------------------- */
	pConfigInfo = (CFG_STATE_INFO*)hConfigFile;
				
/*	We are seeking a config item value, so if we have reached the
	end of the configuration file, we have a config item identifier
	with no associated value.
	--------------------------------------------------------------- */
	if (ch == CHR_END_OF_FILE) {
		*(pStrID + *pPosID) = '\0';
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Config item ID \"%s\" has no value (line %d)\n",
				pStrID, thisLine);
		return(FALSE);

/*	If this is the BEGIN_ARRAY character, we need to initialize
	ourselves for processing an array of values and then set a
	flag to remember that we are doing an array of values for
	the current config item identifier.
	----------------------------------------------------------- */
	} else if (ch == CHR_BEGIN_ARRAY) {
		if (*pDoingArray) {
			UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
				"ERROR: Config item ID \"%s\" already has a '{' (line %d)\n",
					pStrID, thisLine);
			return(FALSE);
		} else {
			array_process_init(hConfigFile);
			*pDoingArray = TRUE;
		}

/*	If this is the END_ARRAY character, we need to verify that we
	are actually doing an array of config item values and then
	load these values for our current config item identifier.  Then
	we need to terminate our array processing environment and set
	the next state of this state machine to seek the next config
	item identifier.
	--------------------------------------------------------------- */
	} else if (ch == CHR_END_ARRAY) {
		if (*pDoingArray) {
			if ( !load_array_item(hConfigFile, pStrID, *pTypeArray,
								  thisLine) ) {
				*(pStrID + *pPosID) = '\0';
				UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
					"ERROR: Config item ID \"%s\" not defined (line %d)\n",
						pStrID, thisLine);
				return(FALSE);
			}
			*pDoingArray = FALSE;
			array_process_term(hConfigFile);
			*pCurrState = SEEK_ID_STATE;
		} else {
			*(pStrID + *pPosID) = '\0';
			UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
				"ERROR: Config item ID \"%s\" has no matching '{' (line %d)\n",
					pStrID, thisLine);
			return(FALSE);
		}

/*	If this is the STR_DELIMITER character, we are about to 
	process a config item string value, so we just need to set
	the next state of this state machine to read a string value.
	------------------------------------------------------------ */
	} else if (ch == CHR_STR_DELIMITER) {
		*pCurrState = READ_STR_STATE;
		*pPosVal = 0;

/*	If this character is a digit or the MINUS character or the PLUS
	character, we are about to process a config item integer value,
	so we just need to set the next state of this state machine to
	read an integer value.
	--------------------------------------------------------------- */
	} else if ( isdigit(ch) || (ch == CHR_MINUS) || (ch == CHR_PLUS) ) {
		*pCurrState = READ_INT_STATE;
		*pPosVal = 0;
		*(pStrVal + (*pPosVal)++) = ch;

/*	If this character is NOT white space, our current config item
	identifier has no associated value.
	------------------------------------------------------------- */
	} else if ( !isspace(ch) ) {
		*(pStrID + *pPosID) = '\0';
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Config item ID \"%s\" has no value (line %d).\n",
				pStrID, thisLine);
		return(FALSE);
	}

/*	Return a good status to the caller.
	----------------------------------- */
	return(TRUE);
}

/*
** Name:
**		save_array_value
**
** Purpose
**		This local function saves a config item value for a config
**		item identifier of a specified type.
**
** Calling Sequence:
**		isOkay = save_array_value(hConfigFile, pItemVal, typeItem);
**
** Input:
**		hConfigFile		handle to configuration file
**		pItemVal		ptr to the config item value
**		typeItem		the type of the config item
**
** Output:
**		NONE
**
** Returns:
**		isOkay		whether or not the config item value was saved okay:
**						TRUE  => the config item value was saved okay
**						FALSE => the config item value was NOT saved okay
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
static int save_array_value(
	HANDLE	hConfigFile,		/* handle to configuration file				*/
	char*	pItemVal,			/* ptr to the config item value				*/
	int		typeItem)			/* the type of the config item				*/
{
	CFG_STATE_INFO*		pConfigInfo;	/* ptr to config file state info	*/
	char**				pThisArrayItem;	/* ptr to this array item			*/
	char*				c_temp;			/* ptr to temp char buffer			*/
	int					thisInt;		/* this integer						*/
	char*	func="save_array_value";	/* ptr to name of function			*/

/*	Get the pointer to the REAL configuration file context information.
	------------------------------------------------------------------- */
	pConfigInfo = (CFG_STATE_INFO*)hConfigFile;
	
/*	Make sure we have room in our temporary memory of array pointers
	for this new array item.
	---------------------------------------------------------------- */
	if (pConfigInfo->theArrayCount >= MAX_ARRAY_PTRS) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Exceeded the maximum number of array items (%d).\n",
				MAX_ARRAY_PTRS);
		return(FALSE);
	}

/*	If this is NOT the first config item value of the array to be
	saved AND the type of this config item value is DIFFERENT than
	the FIRST config item value, we have an error condition.
	-------------------------------------------------------------- */
	if (pConfigInfo->theArrayCount >= 1) {
		if (typeItem != pConfigInfo->theArrayType) {
			if (pConfigInfo->theArrayType == CFG_INT_TYPE) {
				UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
					"ERROR: The value \"%s\" must be an INTEGER.\n",
						pItemVal);
			} else if (pConfigInfo->theArrayType == CFG_STR_TYPE) {
				UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
					"ERROR: The value \"%s\" must be a STRING.\n",
						pItemVal);
			} else {
				UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
					"ERROR: The value \"%s\" is an unknown type %d.\n",
						pItemVal, pConfigInfo->theArrayType);
			}
			return(FALSE);
		}
	}
			
/*	If this config item type is a CFG_STR_TYPE (which is a string),
	then we need to allocate some memory for it and copy the value
	into the new memory.
	--------------------------------------------------------------- */
	if (typeItem == CFG_STR_TYPE) {
		c_temp = (char*)calloc(1, strlen(pItemVal) + 1);
		if (c_temp == NULL) {
			UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
				"ERROR: Could not allocate memory for config value \"%s\"\n",
					pItemVal);
			return(FALSE);
		}
		strcpy(c_temp, pItemVal);
				
/*	If this config item type is a CFG_INT_TYPE (which is an integer),
	then we need to allocate some memory for an integer, convert the
	string value to an integer and copy the value into the new memory.
	------------------------------------------------------------------ */
	} else if (typeItem == CFG_INT_TYPE) {
		c_temp = (char*)calloc(1, sizeof(int));
		if (c_temp == NULL) {
			UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
				"ERROR: Could not allocate memory for config value \"%s\"\n",
					pItemVal);
			return(FALSE);
		}
		thisInt = atoi(pItemVal);
		memcpy(c_temp, &thisInt, sizeof(int));
				
/*	Otherwise, this config item type is unknown, so we need to
	report an error and then move on.
	---------------------------------------------------------- */
	} else {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Unknown configuration type = %d.\n", typeItem);
		return(FALSE);
	}
	
/*	Save off the pointer to the newly allocated memory and bump the 
	count of array items that have been saved.
	--------------------------------------------------------------- */
	pThisArrayItem = pConfigInfo->theArrayPtrs + pConfigInfo->theArrayCount;
	*pThisArrayItem = c_temp;
	pConfigInfo->theArrayCount++;

/*	Remember the type of the config item that was just saved if this 
	is the first one so we can verify that all subsequent config item
	elements of the array are of the same type.
	----------------------------------------------------------------- */
	if (pConfigInfo->theArrayCount == 1) {
		pConfigInfo->theArrayType = typeItem;
	}
	
/*	Return a good status to the caller.
	----------------------------------- */
	return(TRUE);
}

/*
** Name:
**		validate_handle
**
** Purpose:
**		This local function is used to validate a specified configuration
**		file handle.
**
** Calling Sequence:
**		isOkay = validate_handle(hConfigFile);
**
** Input:
**		hConfigFile		handle to configuration file
**
** Output:
**		NONE
**
** Returns:
**		isOkay		whether or not the configuration file handle
**					is valid:
**						TRUE  => the handle is okay
**						FALSE => the handle is NOT okay
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
static int validate_handle(
	HANDLE	hConfigFile)		/* handle to configuration file				*/
{
	int			gotMatch = FALSE;	/* got a match on the handle	*/
	int			gotEntry;			/* got an entry in queue flag	*/
	HANDLE		hThisFile;			/* handle to this config file	*/
	char*	func="validate_handle";	/* ptr to name of function		*/

/*	Validate this configuration file handle by running through the
	Configuration File List, looking for a match on the handle.
	--------------------------------------------------------------- */
	gotEntry = UtlQueue_GetHead(m_ConfigFileList, (void*)&hThisFile);
    while ( (gotEntry) && ( !gotMatch) ) {
		if (hThisFile == hConfigFile) {
			gotMatch = TRUE;
		}
		gotEntry = UtlQueue_GetNext(m_ConfigFileList, (void*)&hThisFile);
    }

/*	Return whether or not we found a match to the caller.
	----------------------------------------------------- */
	return(gotMatch);
}
