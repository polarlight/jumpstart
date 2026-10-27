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
**		utl_dump.c
**
** Description:
**		This is the implementation file for the "utl_dump" module of the
**		UTL subsystem, which is a collection of general-purpose utility 
**		functions that are used by all other subsystems.  This module
**		contains library functions that read a portion of memory or
**		a portion of a file, and dump out that portion to the currently
**		open log file.
**
** Subsystem:
**      UTL
**
** Version:
**      1.00
**
** Public Functions:
**		UtlDump_File
**		UtlDump_Mem
**
** History:
**		Date		Engineer			Purpose
**		--------	----------------	---------------------------------
**		08/21/96	Richard D. Klein	Initial creation.
*/

/* include files */
/* ------------- */
#include <ctype.h>
#include <string.h>
#include "utl_dbg.h"
#include "utl_dump.h"
#include "utl_log.h"

/* private definitions */
/* ------------------- */
#define BYTES_PER_LINE		24		/* Number of bytes in one line of dump	*/

/* private structures */
/* ------------------ */

/* private global variables */
/* ------------------------ */

/**********************************/
/* private prototype declarations */
/**********************************/

/*---------------------------------------------------+
 | NOTE: Please keep the following static function   |
 | ====  declarations in alphabetical order. Thanks! |
 +---------------------------------------------------*/

void byte_to_ascii(char aByte, char* asciiEquiv);
int dump_line(size_t loc, int nBytesInLine, char* pFirstByteInLine,
			  int loggingMode);

/*
** Name:
**		UtlDump_File
**
** Purpose:
**		To the currently open log file, dumps out a specified
**		block of a specified disk file, in both hex and ascii.
**
** Calling Sequence:
**		isOk = UtlDump_File(pFile, fileOffset, nBytes, loggingMode);
**
** Input:
**		pFile				File ptr of the file to dump.
**
**		fileOffset			File offset of the first byte to dump.
**
**		nBytes				Number of bytes to dump.
**
**		loggingMode			Severity of the dump info written to the
**							log file.
**
** Output:
**		NONE
**
** Returns:
**		isOk				BOOLEAN indicating whether or not the
**							function executed successfully.
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int UtlDump_File(
		FILE*	pFile,				/* File ptr of file to dump				*/
		size_t	fileOffset,			/* File offset of first byte to dump	*/
		size_t	nBytes,				/* Number of bytes to dump				*/
		int		loggingMode)		/* Severity of the dump info written to	*/
									/*	the log file.						*/
{
	int		isOk = TRUE;			/* Return value of this function		*/
	int		nLines;					/* Number of lines dump consists of		*/
	int		nBytesInLastLine;		/* Number of bytes in the last line of	*/
									/*	the dump.							*/
	int		nBytesInIthLine;		/* Number of bytes in the ith line of	*/
									/*	the dump.							*/
	int		i;						/* Loop variable						*/
	size_t	offLastByteToDump;		/* File offset of the last byte to dump	*/
	size_t	offLastByteInFile;		/* File offset of the last byte in the	*/
									/*	file being dumped.					*/
	size_t	fileSize;				/* Total number of bytes in file being	*/
									/*	dumped.								*/
	size_t	loc;					/* The offset used to label each line	*/
									/*	of the dump.						*/
	char*	pBuf;					/* Ptr to a buffer into which the file	*/
									/*	bytes to be dumped are read.		*/
	char*	pIthLine;				/* Ptr to the first byte in the 'i'th	*/
									/*	line of the dump.					*/
	char*	func = "UtlDump_File";	/* Name of this function				*/

/*	Log a trace statement indicating we've entered this function.
	------------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	(If the specified block extends beyond the end of the file,
	then return immmediately with an error.)
	----------------------------------------------------------- */
	offLastByteToDump = fileOffset + nBytes - 1;
	fseek(pFile, 0L, 2);
	fileSize = ftell(pFile);
	offLastByteInFile = fileSize - 1;
	if (offLastByteToDump > offLastByteInFile)
	{
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"Specified block extends beyond end of file\n");
		isOk = FALSE;
		goto _errNoCleanup;
	}

/*	Allocate a buffer just large enough to hold the specified
	block of file bytes.
	--------------------------------------------------------- */
	pBuf = malloc(nBytes);
	if (pBuf == NULL)
	{
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"Unable to malloc file buffer\n");
		isOk = FALSE;
		goto _errNoCleanup;
	}
	
/*	Read the specified block of file bytes into the buffer
	allocated above.
	------------------------------------------------------ */
	fseek(pFile, fileOffset, 0);
	fread(pBuf, 1, nBytes, pFile);

/*	Determine how many lines the dump will consist of.
	-------------------------------------------------- */
	nLines = nBytes / BYTES_PER_LINE;
	nBytesInLastLine = nBytes % BYTES_PER_LINE;

	if (nBytesInLastLine != 0)
		nLines++;

/*	Dump out a header to the currently open log file.
	------------------------------------------------- */
	UtlLog_Write(loggingMode, func, RIGHT_HERE, "\n");

/*	For i = first to last line...
	----------------------------- */
	for (i = 0, pIthLine = pBuf, loc = fileOffset;
		 i < nLines;
		 i++, pIthLine += BYTES_PER_LINE, loc += BYTES_PER_LINE)
	{
/*		Dump the ith line out to the currently open log file.
		----------------------------------------------------- */
		nBytesInIthLine = (i==nLines-1) ? nBytesInLastLine : BYTES_PER_LINE;
		isOk = dump_line(loc, nBytesInIthLine, pIthLine, loggingMode);
	}

_errNoCleanup:

/*	Log a trace statement indicating we're about to leave this function.
	-------------------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

	return isOk;
}

/*
** Name:
**		UtlDump_Mem
**
** Purpose:
**		To the currently open log file, dumps out a specified
**		block of memory, in both hex and ascii.
**
** Calling Sequence:
**		isOk = UtlDump_Mem(pFirstByte, nBytes, loggingMode);
**
** Input:
**		pFirstByte			Ptr to the first byte of the memory block
**							to be dumped.
**
**		nBytes				Number of bytes in the memory block to be
**							dumped.
**
**		loggingMode			Severity of the dump info written to the
**							log file.
**
** Output:
**		NONE
**
** Returns:
**		isOk				BOOLEAN indicating whether or not the
**							function executed successfully.
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int UtlDump_Mem(
		char*	pFirstByte,			/* Address of first byte to dump		*/
		size_t	nBytes,				/* Number of bytes to dump				*/
		int		loggingMode)		/* Severity of the dump info written to	*/
									/*	the log file.						*/
{
	int		isOk = TRUE;			/* Return value of this function		*/
	int		nLines;					/* Number of lines dump consists of		*/
	int		nBytesInLastLine;		/* Number of bytes in the last line of	*/
									/*	the dump.							*/
	int		nBytesInIthLine;		/* Number of bytes in the ith line of	*/
									/*	the dump.							*/
	int		i;						/* Loop variable						*/
	char*	pIthLine;				/* Ptr to the first byte in the 'i'th	*/
									/*	line of the dump.					*/
	char*	func = "UtlDump_Mem";	/* Name of this function				*/

/*	Log a trace statement indicating we've entered this function.
	------------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Determine how many lines the dump will consist of.
	-------------------------------------------------- */
	nLines = nBytes / BYTES_PER_LINE;
	nBytesInLastLine = nBytes % BYTES_PER_LINE;

	if (nBytesInLastLine == 0)
		nBytesInLastLine = BYTES_PER_LINE;
	else
		nLines++;

/*	Dump out a header to the currently open log file.
	------------------------------------------------- */
	UtlLog_Write(loggingMode, func, RIGHT_HERE, "\n");

/*	For i = first to last line...
	----------------------------- */
	for (i=0,pIthLine=pFirstByte; i<nLines; i++,pIthLine+=BYTES_PER_LINE)
	{
/*		Dump the ith line out to the currently open log file.
		----------------------------------------------------- */
		nBytesInIthLine = (i==nLines-1) ? nBytesInLastLine : BYTES_PER_LINE;
		isOk = dump_line( (size_t)pIthLine, nBytesInIthLine,
						  pIthLine, loggingMode );
	}

/*	Log a trace statement indicating we're about to leave this function.
	-------------------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

	return isOk;
}

/*--------------------------------------------------+
 | NOTE: Please keep the following static function  |
 | ====  definitions in alphabetical order. Thanks! |
 +--------------------------------------------------*/

/*
** Name:
**		byte_to_ascii
**
** Purpose:
**		Given a byte, determine its two-character ascii equivalent.
**		Two characters are needed in order to represent byte values
**		such as '\n'.  Simple printable characters, such as '@',
**		will be represented as " @", i.e. the first char will be a
**		space.
*/
void byte_to_ascii(
		char	aByte,					/* Byte whose ascii equivalent is to*/
										/*	be determined.					*/
		char*	asciiEquiv)				/* Ptr to storage that is to receive*/
										/*	the two-character ascii equiv-	*/
										/*	alent of the given byte,followed*/
										/*	by a '\0'.  (Therefore this		*/
										/*	storage should be at least three*/
										/*	bytes in length.)				*/
{
	switch (aByte)
	{
		case 0x00:	asciiEquiv[0] = '\\'; asciiEquiv[1] = '0'; break;
		case 0x08:	asciiEquiv[0] = '\\'; asciiEquiv[1] = 'b'; break;
		case 0x0c:	asciiEquiv[0] = '\\'; asciiEquiv[1] = 'f'; break;
		case 0x0a:	asciiEquiv[0] = '\\'; asciiEquiv[1] = 'n'; break;
		case 0x0d:	asciiEquiv[0] = '\\'; asciiEquiv[1] = 'r'; break;
		case 0x09:	asciiEquiv[0] = '\\'; asciiEquiv[1] = 't'; break;

		default:

			if (isprint(aByte))
				sprintf(asciiEquiv, " %c", aByte);
			else
				sprintf(asciiEquiv, "  ");
	}

/*	Terminate the string with a null char.
	-------------------------------------- */
	asciiEquiv[2] = '\0';
}

/*
** Name:
**		dump_line
**
** Purpose:
**		To the currently open log file, dumps out the given array
**		of bytes as a single line.  Actually, this "single line"
**		consists of two lines:  a hex dump, and an ascii dump just
**		below it.  Also, the hex dump line is preceded by the hex
**		representation of the given 'loc' parameter, which is
**		meant to be the address (if we're dumping a memory block)
**		or offset (if we're dumping a disk file) of the first byte
**		in the line.
*/
int dump_line(
		size_t	loc,				/* Address or offset of 1st byte in line*/
		int		nBytesInLine,		/* Number of bytes in the given line	*/
		char*	pFirstByteInLine,	/* Ptr to the first byte in the line	*/
		int		loggingMode)		/* Severity of the dump info written to	*/
									/*	the log file.						*/
{
	int		isOk = TRUE;			/* Return value of this function		*/
	int		i;						/* Loop variable						*/
	char	ithByte;				/* The ith byte in the given line		*/
	char	ithByteInHex[3];		/* The ith byte in the given line,		*/
									/*	expressed as a hex string with a	*/
									/*	terminating null char.				*/
	char	ithByteInAscii[3];		/* The ith byte in the given line,		*/
									/*	expressed as an ascii string con-	*/
									/*	sisting of two chars (in order to	*/
									/*	express bytes such as '\n'),followed*/
									/*	by a terminating null char.			*/
	char	hexLine[80];			/* The hex component of the given line	*/
	char	asciiLine[80];			/* The ascii component of the given line*/
	char*	func = "dump_line";		/* Name of this function				*/


/*	Log a trace line indicating we're entering this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Initialize our hex line to the given 'loc', written in hex, and
	followed by a colon and a space.
	--------------------------------------------------------------- */
	memset(hexLine, '\0', sizeof(hexLine));
	sprintf(hexLine, "%8.8lX: ", loc);

/*	For i = first to last short byte in the line...
	----------------------------------------------- */
	for (i=0; i<nBytesInLine; i++)
	{
/*		If i is even, append a space char to the hex line.
		-------------------------------------------------- */
		if (i % 2 == 0)
			strcat(hexLine, " ");

/*		Fetch the ith byte from memory.
		------------------------------- */
		ithByte = *(pFirstByteInLine + i);

/*		Create a little two-char string containing the hex
		representation of the ith byte.
		-------------------------------------------------- */
		sprintf(ithByteInHex, "%2.2X", ithByte);

/*		Append this little two-char string to the hex line.
		--------------------------------------------------- */
		strcat(hexLine, ithByteInHex);
	}

/*	Append a \n to the hex line, and write the hex line to the
	log file.
	---------------------------------------------------------- */
	strcat(hexLine, "\n");
	UtlLog_Write(loggingMode|LOG_NO_ID, func, RIGHT_HERE, hexLine);

/*	Initialize our ascii line to ten spaces.
	---------------------------------------- */
	memset(asciiLine, '\0', sizeof(asciiLine));
	sprintf(asciiLine, "          ");

/*	For i = first to last byte in the line...
	----------------------------------------- */
	for (i=0; i<nBytesInLine; i++)
	{
/*		If i is even, append a space char to the hex line.
		-------------------------------------------------- */
		if (i % 2 == 0)
			strcat(asciiLine, " ");

/*		Fetch the ith byte from memory.
		------------------------------- */
		ithByte = *(pFirstByteInLine + i);

/*		Create a little two-char string containing the ascii
		representation of the ith byte.
		---------------------------------------------------- */
		byte_to_ascii(ithByte, ithByteInAscii);

/*		Append this little two-char string to the ascii line.
		----------------------------------------------------- */
		strcat(asciiLine, ithByteInAscii);
	}

/*	Append two linefeeds to the ascii line, and write the ascii
	line to the log file.
	----------------------------------------------------------- */
	strcat(asciiLine, "\n\n");
	UtlLog_Write(loggingMode|LOG_NO_ID, func, RIGHT_HERE, asciiLine);

/*	Log a trace line indicating we're leaving this function.
	-------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

	return isOk;
}

