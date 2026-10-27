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
**		utl_dump.h
**
** Description:
**		This is the header file for the "utl_dump" module of the UTL
**		subsystem, which is a collection of general-purpose utility 
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

/* Include Guard START */
/* ------------------- */
#ifndef _UTL_DUMP_H_INCLUDED

/* Include Files */
/* ------------- */

/* Public Definitions */
/* ------------------ */

/* Public Structures */
/* ----------------- */

/* Public Global Variables */
/* ----------------------- */

/* Public Prototype Definitions */
/* ---------------------------- */
//int UtlDump_File(
//		FILE*	pFile,				/* File ptr of file to dump				*/
//		size_t	fileOffset,			/* File offset of first byte to dump	*/
//		size_t	nBytes,				/* Number of bytes to dump				*/
//		int		loggingMode);		/* Severity of the dump info written to	*/
									/*	the log file.						*/

int UtlDump_Mem(
		char*	pFirstByte,			/* Address of first byte to dump		*/
		size_t	nBytes,				/* Number of bytes to dump				*/
		int		loggingMode);		/* Severity of the dump info written to	*/
									/*	the log file.						*/

/* Include Guard END */
/* ----------------- */
#define _UTL_DUMP_H_INCLUDED
#endif

