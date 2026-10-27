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
**		utl_comm_serial.h
**
** Description:
**		This is the header file for the "utl_comm_serial" module of the
**		UTL subsystem, which is a collection of general-purpose utility 
**		functions that are used by all other subsystems.  This module
**		contains library functions related to performing basic serial
**		communications between a process and (typically) some hardware
**		device.
**
** Subsystem:
**      UTL
**
** Version:
**      1.00
**
** Public Functions:
**		UtlCommSerial_Close
**		UtlCommSerial_Open
**		UtlCommSerial_Purge
**		UtlCommSerial_Read
**		UtlCommSerial_Write
**
** History:
**		Date		Engineer			Purpose
**		--------	----------------	---------------------------------
**		05/07/97	Steven M. Rosen		Initial creation.
*/

/* Include Guard START */
/* ------------------- */
#ifndef _UTL_COMM_SERIAL_H_INCLUDED

/* Special Definitions */
/* ------------------- */

/* Include Files */
/* ------------- */
#include <windows.h>

/* Public Definitions */
/* ------------------ */

/* various maximum and minimum constants */
/* ------------------------------------- */
#define	MAX_SERIAL_CHANNELS	256		/* max number of serial comm channels	*/
#define	MAX_SERIAL_MSG_SIZE	8196	/* max size (in bytes) of a message		*/

/* Public Structures */
/* ----------------- */

/* the message header descriptor */
/* ----------------------------- */
typedef struct {
	int		sizeMsgHead;		/* size (in bytes) of the msg header	*/
	int		offBodyLen;			/* offset (in bytes) to the body length */
	int		sizeBodyLen;		/* size (in bytes) of the body length	*/
	int		(*funcBodyCalc)();	/* ptr to the body calculation function	*/
} UTLCOMMSER_HDR_DESC;

/* the device characteristics descriptor */
/* ------------------------------------- */
typedef struct {
	int		devBaudRate;		/* the comm device baud rate				*/
	int		devDataBits;		/* the comm device data bits				*/
	int		devParity;			/* the comm device parity					*/
	int		devStopBits;		/* the comm device stop bits				*/
	int		devReadTimeout;		/* the comm device read timeout (msecs)		*/
	int		devWriteTimeout;	/* the comm device write timeout (msecs)	*/
} UTLCOMMSER_DEVICE_DESC;

/* Public Global Variables */
/* ----------------------- */

/* Public Prototype Definitions */
/* ---------------------------- */

int UtlCommSerial_Close(
		HANDLE	hCommChannel);	/* the comm channel handle				*/

int UtlCommSerial_Open(
		char*	pCommPortStr,	/* ptr to the comm port string			*/
		UTLCOMMSER_DEVICE_DESC*
				pDeviceInfo,	/* ptr to the device info desc			*/
		UTLCOMMSER_HDR_DESC*
				pHeaderInfo,	/* ptr to the msg header info desc		*/
		HANDLE*	phCommChannel);	/* ptr to returned comm channel	handle	*/

int UtlCommSerial_Purge(
		HANDLE	hCommChannel);	/* the comm channel handle				*/

int UtlCommSerial_Read(
		HANDLE	hCommChannel,	/* the comm channel handle				*/
		int		sizeMsgMax,		/* the maximum size of the message 		*/
								/* buffer (in bytes) that has been		*/
								/* allocated by the caller				*/
		char*	pMsg,			/* ptr to a message buffer that			*/
								/* will contain the bytes that were		*/
								/* read from the comm channel			*/
		int*	pSizeMsg);		/* ptr to the size of the message		*/
								/* (in bytes) actually read from 		*/
								/* the comm channel						*/

int UtlCommSerial_Write(
		HANDLE	hCommChannel,	/* the comm channel handle				*/
		int		sizeMsgTotal,	/* the total size of the message 		*/
								/* buffer (in bytes) to be written		*/
								/* to the comm channel					*/
		char*	pMsg,			/* ptr to a message buffer to be		*/
								/* written to the comm channel			*/
		int*	pSizeMsg);		/* ptr to the size of the message		*/
								/* (in bytes) actually written to		*/

/* Include Guard END */
/* ----------------- */
#define _UTL_COMM_SERIAL_H_INCLUDED
#endif
