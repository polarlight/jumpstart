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
**		utl_log.h
**
** Description:
**		This is the header file for the "utl_log" module of the UTL
**		subsystem, which is a collection of general-purpose utility 
**		functions that are used by all other subsystems.  This module
**		contains library functions related to logging of error messages
**		and debug information and is used to standardize the logging
**		strategy.
**
**
** Subsystem:
**      UTL
**
** Version:
**      1.00
**
** Public Functions:
**		UtlLog_Close
**		UtlLog_Open
**		UtlLog_SetFilePtr
**		UtlLog_SetMode
**		UtlLog_Write
**
** History:
**		Date		Engineer			Purpose
**		--------	---------------		---------------------------------
**		12/06/95	Steven M. Rosen		Initial creation.
*/

/* include guard START */
/* ------------------- */
#ifndef _UTL_LOG_H_INCLUDED

/* include files */
/* ------------- */
#include <stdio.h>
#include <time.h>
#include <stdarg.h>

/* public definitions */
/* ------------------ */
#define	FALSE				0			/* the value is FALSE				*/
#define	TRUE				1			/* the value is TRUE  				*/

#define	MAX_LOG_FILES		100			/* max number of log files			*/
#define	MAX_LOG_FILE_BYTES	20000000	/* max bytes in a log file			*/

#define	LOG_ERROR			(1 << 0)	/* log ERROR mask					*/
#define	LOG_WARN			(1 << 1)	/* log WARNING mask					*/
#define	LOG_INFO			(1 << 2)	/* log INFORMATION mask				*/
#define	LOG_DEBUG			(1 << 3)	/* log DEBUG mask					*/
#define	LOG_TRACE			(1 << 4)	/* log TRACE mask					*/
#define	LOG_NO_ID			(1 << 5)	/* log NO IDENTIFIER mask			*/
#define	LOG_XVT				(1 << 6)	/* log XVT mask						*/
#define	LOG_TWICE			(1 << 7)	/* log TWICE mask					*/
#define	LOG_CONSOLE			(1 << 8)	/* log CONSOLE mask					*/

#define	RIGHT_HERE	(int)__LINE__,(char *)__FILE__	/* right here in source	*/

/* public structures */
/* ----------------- */

/* public global variables */
/*------ ----------------- */

/* public prototype definitions */
/* ---------------------------- */
int		UtlLog_Close(
														);
int		UtlLog_Open(
							char*	pLogFileRootPath,
							char*	pLogFileBaseName,
							char*	pLogFileMode,
							int		szMaxLogFileBytes,	
							int		maxLogFiles			);
void	UtlLog_SetFilePtr(
							FILE*	pLogFileHndl		);
void	UtlLog_SetMode(
							int		logModeMask);
void	UtlLog_SetXvtFuncs(
							void	(*xvtInfoFunc)(),
							void	(*xvtWarnFunc)(),
							void	(*xvtErrorFunc)()	);
int		UtlLog_Write(
							int		logModeMask,
							char*	pNameFunc,
							int		srcLine,
							char*	srcFile,
							char*	pFormat, ...		);

/* include guard END */
/* ----------------- */
#define _UTL_LOG_H_INCLUDED
#endif

