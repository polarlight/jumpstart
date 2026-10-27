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
**		utl_log.c
**
** Description:
**		This module contains library functions for the UTL subsystem, which
**		is a collection of general-purpose utility functions that are used by
**		all other subsystems.  This module contains library functions
**		related to logging of error messages and debug information and
**		is used to standardize the logging strategy.
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
**		UtlLog_SetXvtFuncs
**		UtlLog_Write
**
** History:
**		Date		Engineer			Purpose
**		--------	---------------		---------------------------------
**		12/06/95	Steven M. Rosen		Initial creation.
*/

/* include files */
/* ------------- */
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "utl_dbg.h"
#include "utl_log.h"

/* private definitions */
/* ------------------- */
#define	SIZE_LOG_LINE_STR	256			/* max log file line size			*/
#define	SIZE_LOG_NUM_STR	2			/* max log file number size			*/
#define	SIZE_NAME_STR		128			/* max length for a name			*/
#define	SIZE_TIMESTAMP_STR	32			/* max timestamp size				*/

#define	DFLT_LOG_FILES		7			/* dflt number of log files			*/
#define	DFLT_LOG_FILE_BYTES	1000000		/* dflt max bytes in a log file		*/ 
#define	DFLT_LOG_MODE	(LOG_ERROR | LOG_WARN)	/* default logging mode		*/

#define	LOG_EXTENSION		".log"		/* log file extension				*/
#define	LOG_SEPARATOR		"_"			/* log file separator				*/

#define	OPEN_MODE_APPEND	"a"			/* open log file mode as append		*/
#define	OPEN_MODE_WRITE		"w"			/* open log file mode as write		*/

#define	TIMESTAMP_FORMAT	"%c"		/* timestamp format					*/
#define	LOG_NUM_FORMAT		"%02.2d"	/* log file number format			*/
#define	LOG_ID_FORMAT	" %s ==> %s (line %d in %s):\n"	/* log ID format	*/

/* private structures */
/* ------------------ */
typedef char	LOG_LINE_STR	[SIZE_LOG_LINE_STR + 1];
typedef char	LOG_NUM_STR		[SIZE_LOG_NUM_STR + 1];
typedef char	NAME_STR		[SIZE_NAME_STR + 1];
typedef char	TIMESTAMP_STR	[SIZE_TIMESTAMP_STR + 1];

/* private global variables */
/* ------------------------ */
static NAME_STR		m_pLogFileBaseName;		/* ptr to log file base name	*/
static NAME_STR		m_pLogFileRootPath;		/* ptr to log file root path	*/
static NAME_STR		m_pLogFileMode;			/* ptr to log file mode			*/
static NAME_STR		m_pLogFileFullName;		/* ptr to log file full name	*/
static FILE			*m_pLogFileHndl = NULL;	/* ptr to log file handle		*/

static int			m_UsingSetFilePtr = FALSE;
											/* using "UtlLog_SetFilePtr"	*/

static int			m_LogModeMask = DFLT_LOG_MODE;
											/* the logging mode	mask		*/
static int			m_szMaxLogFileBytes = DFLT_LOG_FILE_BYTES;
											/* max size of a log file		*/
static int			m_maxLogFiles = DFLT_LOG_FILES;
											/* max number of log files		*/

static void			(*m_XvtInfoFunc)()  = NULL;	/* XVT informational func	*/
static void			(*m_XvtWarnFunc)()  = NULL;	/* XVT warning func			*/
static void			(*m_XvtErrorFunc)() = NULL;	/* XVT error func			*/

/* private prototype definitions */
/* ----------------------------- */

static int	find_log_file(
	char*	pLogFileName,		/* ptr to the debug log file name 			*/
	int*	pAllFilesFilled);	/* ptr to ALL debug log files filled flag	*/

static int	log_file_is_full();

static int	open_next_log_file();

static int	validate_log_open(
	char*	pLogFileRootPath,	/* ptr to the debug log file root path	*/
	char*	pLogFileBaseName,	/* ptr to the debug log file base name	*/
	char*	pLogFileMode,		/* ptr to the mode of the open			*/
	int		szMaxLogFileBytes,	/* max size of a log file (in bytes)	*/
	int		maxLogFiles);		/* max number of log files				*/

/*
** Name:
**		UtlLog_Close
**
** Purpose:
**		This function is used to close the debug log file that was
**		opened by an application using "UtlLog_Open".
**
** Calling Sequence:
**		isOkay = UtlLog_Close();
**
** Input:
**		NONE
**
** Output:
**		NONE
**
** Returns:
**		isOkay			whether or not the debug log file was
**						closed okay:
**							TRUE  => the log file was closed okay
**							FALSE => the log file was NOT closed okay
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int UtlLog_Close()
{
	int		isOk = FALSE;			/* return status of this function	*/
	char*	func = "UtlLog_Close";	/* ptr to name of func				*/

/*	Reset the names associated with this debug log file.
	---------------------------------------------------- */
	strcpy(m_pLogFileRootPath, "");
	strcpy(m_pLogFileBaseName, "");
	strcpy(m_pLogFileMode, "");
	strcpy(m_pLogFileFullName, "");

/*	Close the debug log file and reset the log file handle.
	------------------------------------------------------- */
	if (m_pLogFileHndl) {
		fclose(m_pLogFileHndl);
	}
	m_pLogFileHndl = NULL;

/*	Indicate that everything went okay.
	----------------------------------- */
	isOk = TRUE;

/*	Return the status to the caller.
	-------------------------------- */
	return(isOk);
}

/*
** Name:
**		UtlLog_Open
**
** Purpose:
**		This function is used to open the debug log file for an
**		application.  Using the "UtlLog" functions to open an
**		application's debug log file allows for a standardized
**		method for writing information to the debug log file and
**		provides a standardized method for managing the number
**		and maximum size of the debug log files.
**
** Calling Sequence:
**		isOkay = UtlLog_Open(pLogFileRootPath, pLogFileBaseName,
**							 pLogFileMode, szMaxLogFileBytes,
**							 maxLogFiles);
**
** Input:
**		pLogFileRootPath	ptr to the debug log file root path
**		pLogFileBaseName	ptr to the debug log file base name
**		pLogFileMode		ptr to the mode of the open (either "a" or "w")
**		szMaxLogFileBytes	max size of a log file (in bytes)
**		maxLogFiles			max number of log files
**
** Output:
**		NONE
**
** Returns:
**		isOkay			whether or not the debug log file was
**						opened okay:
**							TRUE  => the log file was opened okay
**							FALSE => the log file was NOT opened okay
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int UtlLog_Open(
	char*	pLogFileRootPath,	/* ptr to the debug log file root path	*/
	char*	pLogFileBaseName,	/* ptr to the debug log file base name	*/
	char*	pLogFileMode,		/* ptr to the mode of the open			*/
	int		szMaxLogFileBytes,	/* max size of a log file (in bytes)	*/
	int		maxLogFiles)		/* max number of log files				*/
{
	int			isOk = FALSE;		/* return status of this function	*/
	int			allFilesFilled;		/* all debug log files filled flag	*/
	NAME_STR	pLogFileName;		/* ptr to full debug log file name	*/

/*	Validate the input parameters used to open the debug log file
	for this application.
	------------------------------------------------------------- */
	if ( !validate_log_open(pLogFileRootPath, pLogFileBaseName,
							pLogFileMode, szMaxLogFileBytes, maxLogFiles) ) {
		goto _errValidateLogOpen;
	}

/*	Find the name of the next debug log file that is to be opened.
	-------------------------------------------------------------- */
	if ( !find_log_file(pLogFileName, &allFilesFilled) ) {
		goto _errFindLogFile;
	}

/*	Finally, open the debug log file.
	--------------------------------- */
	if (allFilesFilled) {
		m_pLogFileHndl = fopen(pLogFileName, "w");
	} else {
		m_pLogFileHndl = fopen(pLogFileName, pLogFileMode);
	}
	if (m_pLogFileHndl == NULL) {
		printf("ERROR: Could not open log file \"%s\"\n", pLogFileName);
		goto _errOpeningLogFile;
	}

/*	Save the name of the fully qualified debug log file name.
	--------------------------------------------------------- */
	strcpy(m_pLogFileFullName, pLogFileName);

/*	Indicate that everything went okay.
	----------------------------------- */
	isOk = TRUE;
	goto _allOk;

/*	Perform structured exception handling.
	-------------------------------------- */
_errOpeningLogFile:
	;
_errFindLogFile:
	;
_errValidateLogOpen:
	;
_allOk:
	;

/*	Return the status to the caller.
	-------------------------------- */
	return(isOk);
}

/*
** Name:
**		UtlLog_SetFilePtr
**
** Purpose:
**		This function is used to set the debug logging file handle
**		that will be used by other UtlLog_XXX functions to access the
**		debug log file.
**
** Calling Sequence:
**		UtlLog_SetFilePtr(pLogFileHndl);
**
** Input:
**		pLogFileHndl	ptr to the debug log file handle:
**							NULL	 => use standard output
**							non-NULL => use this as a FILE handle
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
void UtlLog_SetFilePtr(
	FILE*	pLogFileHndl)	/* ptr to the debug log file handle */
{
/*	Set the specified debug log file handle into the global
	module variable for use by other functions in this module
	that need to access the debug log file.
	--------------------------------------------------------- */
	m_pLogFileHndl = pLogFileHndl;
	m_UsingSetFilePtr = TRUE;
}

/*
** Name:
**		UtlLog_SetMode
**
** Purpose:
**		This function is used to set the logging mode bits that will
**		be used by other UtlLog_XXX functions to decide whether or
**		not to output messages to the log file.
**
** Calling Sequence:
**		UtlLog_SetMode(logModeMask);
**
** Input:
**		logModeMask		the logging mode mask:
**							LOG_ERROR ==> log ERROR messages
**							LOG_WARN  ==> log WARNING messages
**							LOG_INFO  ==> log INFORMATIONAL messages
**							LOG_DEBUG ==> log DEBUG messages
**							LOG_TRACE ==> log TRACE messages
**							LOG_XVT   ==> log XVT messages
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
void UtlLog_SetMode(
	int		logModeMask)	/* the logging mode mask */
{
/*	Set the specified logging mode mask into the global module
	variable for use by other functions in this module that need
	to access the current logging mode.
	------------------------------------------------------------ */
	m_LogModeMask = logModeMask;
}

/*
** Name:
**		UtlLog_SetXvtFuncs
**
** Purpose:
**		This function is used to set the XVT informational, warning,
**		and error functions that might be used by the UtlLog_Write
**		function when and XVT application wants to output an XVT 
**		informational, warning, or error message.
**
** Calling Sequence:
**		UtlLog_SetXvtFuncs(xvtInfoFunc, xvtWarnFunc, xvtErrorFunc);
**
** Input:
**		xvtInfoFunc			ptr to the XVT informational function
**		xvtWarnFunc			ptr to the XVT warning function
**		xvtErrorFunc		ptr to the XVT error function
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
void UtlLog_SetXvtFuncs(
	void	(*xvtInfoFunc)(),		/* ptr to the XVT informational func	*/
	void	(*xvtWarnFunc)(),		/* ptr to the XVT warning func			*/
	void	(*xvtErrorFunc)())		/* ptr to the XVT error func			*/
{
/*	Set the specified XVT informational, warning, and error functions 
	into the global module variables for use by other functions in this
	module that need to access these XVT functions.
	------------------------------------------------------------------- */
	m_XvtInfoFunc  = xvtInfoFunc;
	m_XvtWarnFunc  = xvtWarnFunc;
	m_XvtErrorFunc = xvtErrorFunc;
}

/*
** Name:
**		UtlLog_Write
**
** Purpose:
**		This function is used to write to the debug log file 
**		that the caller has opened up using a previous call to
**		"UtlLog_Open".  If the caller has forgotten to make
**		make this call (or, intentionally not made the call
**		because they do NOT want a debug log file), then this 
**		function will just write the specified output message 
**		to standard out using "printf" instead of "fprintf". 
**		This function will automatically timestamp the message
**		that is sent out to the debug log file.
**
** Calling Sequence:
**		bytesWritten = UtlLog_Write(logModeMask, pNameFunc, srcLine,
**									srcFile, pFormat, ...);
**
** Input:
**		logModeMask		the logging mode mask of this message
**		pNameFunc		ptr to the name of the calling function
**		srcLine			the line number of the source file
**		srcFile			ptr to the name of the source file
**		pFormat			ptr to the output format statement
**		...				the variable argument list (just like
**						"printf" or "fprintf")
**
** Output:
**		NONE
**
** Returns:
**		bytesWritten		the number of bytes actually output
**							(excluding the NULL termination character),
**							which is just like "printf" or "fprintf" 
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int UtlLog_Write(
	int		logModeMask,		/* the logging mode mask of this message	*/
	char*	pNameFunc,			/* ptr to the name of the calling function	*/
	int		srcLine,			/* the line number of the source file		*/
	char*	srcFile,			/* ptr to the name of the source file		*/
	char*	pFormat,			/* ptr to the output format statement		*/
	...)						/* the variable argument list				*/
{
	va_list			args;			/* the variable argument list	*/
	LOG_LINE_STR	msgXvt;			/* the XVT message				*/
	LOG_LINE_STR	msgId;			/* the logging ID message		*/
	LOG_LINE_STR	msgUser;		/* the user message				*/
	TIMESTAMP_STR	theTime;		/* the timestamp as a string	*/
	time_t			currTime;		/* the current timestamp		*/
	int				bytesWritten;	/* the returned byte count		*/

/*	Assume that we have written nothing.
	------------------------------------ */
	bytesWritten = 0;

/*	If the current logging mode and if turned off, we have nothing
	to do, so just return to the caller.
	-------------------------------------------------------------- */
	if (!m_LogModeMask) {
		return(bytesWritten);
	}

/*	Use the format statement (if supplied) to format the variable
	argument list supplied by the caller.
	------------------------------------------------------------- */
	if (pFormat!= NULL) {
		va_start(args, pFormat);
		bytesWritten = vsprintf(msgUser, pFormat, args);
		va_end(args);
	}

/*	If the caller has not suppressed the log message identifier,
	then create it, which will include the timestamp string, the
	function name, the line number of the source file, and the
	name of the source file.
	------------------------------------------------------------ */
	strcpy(msgXvt, "");
	if ( ( logModeMask & m_LogModeMask ) &&
	     ( !(logModeMask & LOG_NO_ID) ) ) {
		currTime = time(NULL);
		strftime(theTime, sizeof(theTime), TIMESTAMP_FORMAT,
				 localtime(&currTime));
		sprintf(msgId, LOG_ID_FORMAT, theTime, pNameFunc, srcLine, srcFile);

/*		If this is a LOG_ERROR message and the current logging mode
		says we are in an XVT application, start setting up the XVT 
		error message.
		-------------------------------------------------------- */
		if ( (logModeMask & LOG_ERROR) && (m_LogModeMask & LOG_XVT) ) {
			strcat(msgXvt, msgId);
		} 

/*		If a log file has been opened (meaning, we have a non-NULL
		log file pointer), then output the log message identifier to
		the log file.  Optionally, output the log message identifier 
		to the standard output.
		------------------------------------------------------------ */
		if (m_pLogFileHndl) {

/*			Check to see if we need to close the current log file and
			then open a new log file (which happens when our current
			log file has filled itself to capacity).
			--------------------------------------------------------- */
			if ( log_file_is_full() ) {
				if ( !open_next_log_file() ) {
					return(0);
				}
			}
			bytesWritten = fprintf(m_pLogFileHndl, "%s", msgId);
			fflush(m_pLogFileHndl);

/*			If the current logging mode says to output all log
			messages twice (usually for debugging a crash) then
			output this message again.
			--------------------------------------------------- */
			if (m_LogModeMask & LOG_TWICE) {
				bytesWritten = fprintf(m_pLogFileHndl, "%s", msgId);
				fflush(m_pLogFileHndl);
			}
		}

		if ( (!m_pLogFileHndl) || (m_LogModeMask & LOG_CONSOLE) ) {
			bytesWritten = printf("%s", msgId);
		}
	}

/*	If the current logging mode says we are in an XVT application,
	then we might need to also output the message using an XVT
	function to show either an ERROR, WARNING, or INFO condition.
	-------------------------------------------------------------- */
	if ( m_LogModeMask & LOG_XVT ) {

/*		We have an ERROR message to output as an XVT alert box.
		------------------------------------------------------- */
		if (logModeMask & LOG_ERROR) {
			strcat(msgXvt, msgUser);
			if (m_XvtErrorFunc) {
				(*m_XvtErrorFunc)(msgXvt);
			}

/*		We have a WARNING message to output as an XVT alert box.
		-------------------------------------------------------- */
		} else if (logModeMask & LOG_WARN) {
			strcat(msgXvt, msgUser);
			if (m_XvtWarnFunc) {
				(*m_XvtWarnFunc)(msgXvt);
			}

/*		We have an INFOrmational message to output as an XVT alert box.
		--------------------------------------------------------------- */
		} else if (logModeMask & LOG_INFO) {
			strcat(msgXvt, msgUser);
			if (m_XvtInfoFunc) {
				(*m_XvtInfoFunc)(msgXvt);
			}
		}
	}

/*	If the log mode of this message is compatible with the current
	logging mode, then output the log message to either the log file
	and/or the standard output.
	---------------------------------------------------------------- */
	if ( (logModeMask & m_LogModeMask) && (pFormat != NULL) ) {
		if (m_pLogFileHndl) {
			bytesWritten = fprintf(m_pLogFileHndl, "	%s", msgUser);
			fflush(m_pLogFileHndl);

/*			If the current logging mode says to output all log
			messages twice (usually for debugging a crash) then 
			output this message again.
			--------------------------------------------------- */
			if (m_LogModeMask & LOG_TWICE) {
				bytesWritten = fprintf(m_pLogFileHndl, "	%s", msgUser);
				fflush(m_pLogFileHndl);
			}
		}

		if ( (!m_pLogFileHndl) || (m_LogModeMask & LOG_CONSOLE) ) {
			bytesWritten = printf("	%s", msgUser);
		}
	} else {
		bytesWritten = 0;
	}

/*	We are done processing this log message, so return the number
	of bytes actually written to the caller.
	------------------------------------------------------------- */
	return(bytesWritten);
}

/*
** Name:
**		find_log_file
**
** Purpose:
**		This local function is used to find a fully qualified debug log
**		file name, using a specified root path and base name to find the
**		FIRST debug log file of the form "<root path><base name>_<num>.log"
**		that is smaller than the maximum size for a debug file, where <num>
**		is the ASCII equivalent of a number between "00" and "maxLogFiles".
*/
static int find_log_file(
	char*	pLogFileName,		/* ptr to the debug log file name ptr		*/
	int*	pAllFilesFilled)	/* ptr to ALL debug log files filled flag	*/
{
	int				isOk = FALSE;			/* return status of this func	*/
	struct stat		fileStatus;				/* info about the log file		*/
	int				err;					/* returned error code			*/
	int				lenLogFileName;			/* len of the log file name		*/
	int				foundLogFile = FALSE;	/* found a qualifying log file	*/
	int				thisLogFile = 0;		/* current log file number		*/
	time_t			oldest_st_mtime;		/* oldest modify log file time	*/
	NAME_STR		pOldestLogFileName;		/* ptr to oldest log file		*/
	NAME_STR		pLogFileNum;			/* ptr to log file number		*/
	char*	func = "find_log_file";			/* ptr to name of func			*/

/*	Initialize the oldest modify log file time.
	------------------------------------------- */
	oldest_st_mtime = time(NULL);
	*pAllFilesFilled = FALSE;

/*	Make sure the fully qualified debug log file name is not too big.
	----------------------------------------------------------------- */
	lenLogFileName = strlen(m_pLogFileRootPath) + 
					 strlen(m_pLogFileBaseName) +
					 strlen(LOG_SEPARATOR) + SIZE_LOG_NUM_STR +
					 strlen(LOG_EXTENSION);
	if (lenLogFileName > SIZE_NAME_STR) {
		printf("ERROR: Length of debug log file (%d) exceeds max size (%d)\n",
				lenLogFileName, SIZE_NAME_STR);
		goto _errLogFileNameTooBig;
	}

/*	Loop until we either find a qualifying debug log file or we have
	exceeded the max number of log files.
	---------------------------------------------------------------- */
	while ( (!foundLogFile) && (thisLogFile < m_maxLogFiles) ) {

/*		Create the name of the fully qualified debug log file name.
		----------------------------------------------------------- */
		sprintf(pLogFileNum, LOG_NUM_FORMAT, thisLogFile);
		strcpy(pLogFileName, m_pLogFileRootPath);
		strcat(pLogFileName, m_pLogFileBaseName);
		strcat(pLogFileName, LOG_SEPARATOR);
		strcat(pLogFileName, pLogFileNum);
		strcat(pLogFileName, LOG_EXTENSION);

/*		Get some information about this debug log file.
		----------------------------------------------- */
		err = stat(pLogFileName, &fileStatus);
		if (err < 0) {

/*			If this file does not exist at this time, we are all done with
			our search.  Otherwise, we have a true problem to report.
			-------------------------------------------------------------- */
			if (errno == ENOENT) {
				foundLogFile = TRUE;
			} else {
				printf("ERROR: Could not get log file info, error = %d\n",
						errno);
				goto _errGettingLogFileInfo;
			}

/*		This file exists, so see if its size is reasonable (under the max
		log file size in bytes).  If it is reasonable, we are done with our
		search.  Otherwise, just save its name if the log file's last
		modification date is the oldest date so far.
		------------------------------------------------------------------- */
		} else {
			if (fileStatus.st_size < m_szMaxLogFileBytes) {
				foundLogFile = TRUE;
			} else {
				if (fileStatus.st_mtime < oldest_st_mtime) {
					strcpy(pOldestLogFileName, pLogFileName);
					oldest_st_mtime = fileStatus.st_mtime;
				}
			}
		}

/*		Bump to the next log file number.
		--------------------------------- */
		thisLogFile++;
	}

/*	If we have NOT found a qualifying debug log file, there is only
	one reason: We have the max number of log files and they are ALL
	filled up to capacity.  In this case, we use the oldest debug
	log file name as the fully qualified name.
	----------------------------------------------------------------- */
	if ( !foundLogFile ) {
		strcpy(pLogFileName, pOldestLogFileName);
		*pAllFilesFilled = TRUE;
	}

/*	Indicate that everything went okay.
	----------------------------------- */
	isOk = TRUE;
	goto _allOk;

/*	Perform structured exception handling.
	-------------------------------------- */
_errGettingLogFileInfo:
	;
_errLogFileNameTooBig:
	;
_allOk:
	;

/*	Return the status to the caller.
	-------------------------------- */
	return(isOk);
}

/*
** Name:
**		log_file_is_full
**
** Purpose:
**		This local function is used to check to see if the current
**		debug log file has filled itself to its maximum capacity.
*/
static int log_file_is_full()
{
	int			isFull = FALSE;			/* return status of this func	*/
	struct stat	fileStatus;				/* info about the log file		*/
	int			err;					/* returned error code			*/
	char*	func = "log_file_is_full";	/* ptr to name of func			*/

/*	Only check the size of the current debug log file if we have
	one currently opened.
	------------------------------------------------------------ */
	if ( ( m_pLogFileHndl != NULL ) && ( !m_UsingSetFilePtr ) ) {

/*		Get some information about the current debug log file.
		------------------------------------------------------ */
		err = stat(m_pLogFileFullName, &fileStatus);
		if (err < 0) {
			printf("ERROR: Could not get log file info, error = %d\n", errno);
		} else {

/*			If the size of our currently open debug log file has exceeded
			the maximum allowed, then indicate this to the caller.
			------------------------------------------------------------- */
			if (fileStatus.st_size > m_szMaxLogFileBytes) {
				isFull = TRUE;
			}
		}
	}

/*	Return the status to the caller.
	-------------------------------- */
	return(isFull);
}

/*
** Name:
**		open_next_log_file
**
** Purpose:
**		This local function is used to open the next debug log
**		file associated with an application.
*/
static int open_next_log_file()
{
	int			isOk = FALSE;		/* return status of this func			*/
	NAME_STR	pLogFileRootPath;	/* ptr to the debug log file root path	*/
	NAME_STR	pLogFileBaseName;	/* ptr to the debug log file base name	*/
	NAME_STR	pLogFileMode;		/* ptr to the mode of the open			*/
	int			szMaxLogFileBytes;	/* max size of a log file (in bytes)	*/
	int			maxLogFiles;		/* max number of log files				*/
	char*	func = "open_next_log_file";	/* ptr to name of func			*/

/*	If we do NOT have a currently open debug log file, there is
	nothing to do.
	----------------------------------------------------------- */
	if ( m_pLogFileHndl == NULL ) {
		isOk = TRUE;
		goto _allOk;
	}

/*	Otherwise, we need to close the log file and then open the next
	log file.  First, save the relevant information before we actually
	close the existing debug log file.
	------------------------------------------------------------------ */
	strcpy(pLogFileRootPath, m_pLogFileRootPath);
	strcpy(pLogFileBaseName, m_pLogFileBaseName);
	strcpy(pLogFileMode, m_pLogFileMode);

	szMaxLogFileBytes = m_szMaxLogFileBytes;
	maxLogFiles       = m_maxLogFiles;

/*	We need to close the log file and then open the next log file.
	-------------------------------------------------------------- */
	if ( !UtlLog_Close() ) {
		goto _errLogClose;
	}
	if ( !UtlLog_Open(	pLogFileRootPath,
						pLogFileBaseName,
						pLogFileMode,
						szMaxLogFileBytes,
						maxLogFiles	) ) {
		goto _errLogOpen;
	}

/*	Indicate that everything went okay.
	----------------------------------- */
	isOk = TRUE;
	goto _allOk;

/*	Perform structured exception handling.
	-------------------------------------- */
_errLogOpen:
	;
_errLogClose:
	;
_allOk:
	;

/*	Return the status to the caller.
	-------------------------------- */
	return(isOk);
}

/*
** Name:
**		validate_log_open
**
** Purpose:
**		This function is used to validate the parameters used to
**		open the debug log file for an application.
*/
static int validate_log_open(
	char*	pLogFileRootPath,	/* ptr to the debug log file root path	*/
	char*	pLogFileBaseName,	/* ptr to the debug log file base name	*/
	char*	pLogFileMode,		/* ptr to the mode of the open			*/
	int		szMaxLogFileBytes,	/* max size of a log file (in bytes)	*/
	int		maxLogFiles)		/* max number of log files				*/
{
	int		isOk = FALSE;			/* return status of this function	*/

/*	Make sure we have valid pointers for the debug log file root path,
	base name, and mode.
	------------------------------------------------------------------ */
	if (pLogFileRootPath == NULL) {
		printf("ERROR: Invalid debug log file root path ptr\n");
		goto _errRootPathPtr;
	}
	if (pLogFileBaseName == NULL) {
		printf("ERROR: Invalid debug log file base name ptr\n");
		goto _errBaseNamePtr;
	}
	if (pLogFileMode == NULL) {
		printf("ERROR: Invalid debug log file mode ptr\n");
		goto _errModePtr;
	}

/*	Make sure the input strings are reasonable sizes.
	------------------------------------------------- */
	if ( strlen(pLogFileRootPath) > SIZE_NAME_STR ) {
		printf("ERROR: Log file root path\"%s\" is too long (%d), max = %d\n",
				pLogFileRootPath, strlen(pLogFileRootPath), SIZE_NAME_STR);
		goto _errRootPathTooLong;
	}
	if ( strlen(pLogFileBaseName) > SIZE_NAME_STR ) {
		printf("ERROR: Log file base name\"%s\" is too long (%d), max = %d\n",
				pLogFileBaseName, strlen(pLogFileBaseName), SIZE_NAME_STR);
		goto _errBaseNameTooLong;
	}
	if ( strlen(pLogFileMode) > SIZE_NAME_STR ) {
		printf("ERROR: Log file mode\"%s\" is too long (%d), max = %d\n",
				pLogFileMode, strlen(pLogFileMode), SIZE_NAME_STR);
		goto _errModeTooLong;
	}

/*	Make sure the debug log file is opened as either "append" or "write".
	--------------------------------------------------------------------- */
	if ( (strcmp(pLogFileMode, OPEN_MODE_APPEND) != 0) &&
		 (strcmp(pLogFileMode, OPEN_MODE_WRITE) != 0) ) {
		printf("ERROR: Log file mode (\"%s\") must be \"%s\" or \"%s\"\n",
			pLogFileMode, OPEN_MODE_APPEND, OPEN_MODE_WRITE);
		goto _errModeUnknown;
	}

/*	Save the names associated with this debug log file.
	--------------------------------------------------- */
	strcpy(m_pLogFileRootPath, pLogFileRootPath);
	strcpy(m_pLogFileBaseName, pLogFileBaseName);
	strcpy(m_pLogFileMode, pLogFileMode);

/*	Save the maximum size in bytes for a debug log file and the
	maximum number of debug log files.
	----------------------------------------------------------- */
	if ( (szMaxLogFileBytes < 0) || (szMaxLogFileBytes > MAX_LOG_FILE_BYTES) ) {
		printf("WARNING: Max log file bytes (%d) out of range, using %d\n",
			szMaxLogFileBytes, m_szMaxLogFileBytes);
	} else {
		m_szMaxLogFileBytes = szMaxLogFileBytes;
	}
	if ( (maxLogFiles <= 0) || (maxLogFiles > MAX_LOG_FILES) ) {
		printf("WARNING: Max log files (%d) out of range, using %d\n",
			maxLogFiles, m_maxLogFiles);
	} else {
		m_maxLogFiles = maxLogFiles;
	}

/*	Indicate that everything went okay.
	----------------------------------- */
	isOk = TRUE;
	goto _allOk;

/*	Perform structured exception handling.
	-------------------------------------- */
_errModeUnknown:
	;
_errModeTooLong:
	;
_errBaseNameTooLong:
	;
_errRootPathTooLong:
	;
_errModePtr:
	;
_errBaseNamePtr:
	;
_errRootPathPtr:
	;
_allOk:
	;

/*	Return the status to the caller.
	-------------------------------- */
	return(isOk);
}
