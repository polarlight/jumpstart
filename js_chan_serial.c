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
**		utl_comm_serial.c
**
** Description:
**      This module contains library functions for the UTL subsystem, which
**      is a collection of general-purpose utility functions that are used 
**      by all other subsystems.  This module contains library functions
**      related to performing basic serial communications between a process
**		and (typically) some hardware device.
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

/* Include Files */
/* ------------- */
#include <windows.h>
#include "utl_log.h"
#include "utl_dbg.h"
#include "utl_dump.h"
#include "utl_comm_serial.h"

/* Private Definitions */
/* ------------------- */

/* various maximum and minimum constants */
/* ------------------------------------- */
#define	MAX_READ_TIMEOUTS	1				/* max number of read timeouts	*/
#define	MAX_WRITE_TIMEOUTS	1				/* max number of write timeouts	*/

/* miscellaneous constants */
/* ----------------------- */
#define	BODY_SIZE_CHAR		sizeof(char)	/* size of body in a CHAR		*/
#define	BODY_SIZE_SHORT		sizeof(short)	/* size of body in a SHORT		*/
#define	BODY_SIZE_LONG		sizeof(long)	/* size of body in a LONG		*/
#define	BODY_SIZE_INVALID	-1				/* size of body is invalid		*/

/* Private Structures */
/* ------------------ */

/* the all-purpose serial communication channel ID descriptor */
/* ---------------------------------------------------------- */
typedef struct {
	char*					pCommPortStr;	/* ptr to the comm port string	*/
	UTLCOMMSER_DEVICE_DESC	deviceInfo;		/* the device info				*/
	UTLCOMMSER_HDR_DESC		headerInfo;		/* the msg header info			*/
	HANDLE					hCommChannel;	/* handle to the comm channel	*/
} UTLCOMMSER_CHAN_DESC;

/* Private Global Variables */
/* ------------------------ */

/* Private Prototype Definitions */
/* ----------------------------- */
static int comm_create_chan_serial(
	char*		pCommPortStr,			/* ptr to the comm port string		*/
	UTLCOMMSER_CHAN_DESC**	ppChanId);	/* ptr to ptr to the new chan		*/

static int comm_destroy_chan_serial(
	UTLCOMMSER_CHAN_DESC*	pChanId);	/* ptr to the comm channel desc		*/

static int comm_get_body_size_serial(
	UTLCOMMSER_CHAN_DESC*	pChanId,	/* ptr to the comm channel desc		*/
	char*					pMsgHead);	/* ptr to msg header buffer			*/

static int comm_get_header_size_serial(
	UTLCOMMSER_CHAN_DESC*	pChanId);	/* ptr to the comm channel desc		*/

static int comm_load_chan_serial(
	UTLCOMMSER_CHAN_DESC*	pChanId,	/* ptr to the comm channel desc		*/
	char*				pCommPortStr,	/* ptr to the comm port string		*/
	UTLCOMMSER_DEVICE_DESC*
						pDeviceInfo,	/* ptr to the device info desc		*/
	UTLCOMMSER_HDR_DESC*
						pHeaderInfo);	/* ptr to the msg header info desc	*/

static int comm_read_from_chan_serial(
	UTLCOMMSER_CHAN_DESC*	pChanId,	/* ptr to the comm channel desc		*/
	int					sizeToRead,		/* the size (in bytes) to read		*/
	char*				pBuff,			/* ptr to the buffer to read into	*/
	int					fReportErrs);	/* report errors indicator flag		*/

static int comm_write_to_chan_serial(
	UTLCOMMSER_CHAN_DESC*	pChanId,	/* ptr to the comm channel desc		*/
	int					sizeToWrite,	/* the size (in bytes) to write		*/
	char*				pBuff,			/* ptr to the buffer to write from	*/
	int					fReportErrs);	/* report errors indicator flag		*/

/*
** Name:
**		UtlCommSerial_Close
**
** Purpose:
**		This public function is used to close a specified serial
**		communications channel.
**
** Calling Sequence:
**		isOkay = UtlCommSerial_Close(HANDLE hCommChannel);
**
** Input:
**
**		hCommChannel		Handle to the serial comm channel to close
**
** Output:
**		NONE
**
** Returns:
**		isOkay				Whether or not this function succeeded:
**								TRUE  => Everything was okay
**								FALSE => Everything was NOT okay
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int UtlCommSerial_Close(
		HANDLE	hCommChannel)	/* the comm channel handle				*/
{
	int				isOkay = FALSE;		/* return value of this funct	*/
	DWORD			dwError;			/* last error code				*/
	BOOL			fSuccess;			/* returned function status		*/
	UTLCOMMSER_CHAN_DESC*	pChanId;	/* ptr to the comm channel desc	*/
	char*	func="UtlCommSerial_Close";	/* ptr to name of the func		*/

/*	Trace entering this function...
	------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Get the REAL comm channel pointer from the comm channel handle.
	--------------------------------------------------------------- */
	pChanId = (UTLCOMMSER_CHAN_DESC*)hCommChannel;

/*	Close down the comm device.
	--------------------------- */
	fSuccess = CloseHandle(pChanId->hCommChannel);
	if (!fSuccess) {
		dwError = GetLastError();
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Could not close comm port \"%s\", error=%d\n",
				pChanId->pCommPortStr, dwError);
		goto _errCloseHandle;
	} else {
		UtlLog_Write(LOG_DEBUG | LOG_NO_ID, func, RIGHT_HERE,
			"Closed comm port\"%s\"...\n",
				pChanId->pCommPortStr);
	}
 
/*	Everything went okay, so indicate a good return status.
	------------------------------------------------------- */
	isOkay = TRUE;

/*	Error conditions were detected, so indicate a bad return status.
	---------------------------------------------------------------- */
_errCloseHandle:

/*	Trace leaving this function...
	------------------------------ */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...Leaving\n");
	return(isOkay);
}

/*
** Name:
**		UtlCommSerial_Open
**
** Purpose:
**		This public function is used to open a serial communication
**		channel for a specified device on a specified comm port.
**
** Calling Sequence:
**		err = UtlCommSerial_Open(
**						char*						pCommPortStr,
**						UTLCOMMSER_DEVICE_DESC*		pDeviceInfo,
**						UTLCOMMSER_HDR_DESC*		pHeaderInfo,
**						HANDLE*						phCommChannel	);
**
** Input:
**		pCommPortStr		Ptr to the communications port string, which
**							is typically of the form "COMxx".  The "xx"
**							in the "COMxx" string is the comm port number.
**							Common examples of this string would be "COM1",
**							"COM2", "COM7", "COM13", etc.
**
**		pDeviceInfo			Ptr to the device information descriptor, which
**							contains device-related communications parameters.
**							This descriptor contains the following fields:
**
**			->devBaudRate		the comm device baud rate
**			->devDataBits		the comm device data bits
**			->devParity			the comm device parity
**			->devStopBits		the comm device stop bits
**			->devReadTimeout	the comm device read timeout (msecs)
**			->devWriteTimeout	the comm device write timeout (msecs)
**
**		pHeaderInfo			Pointer to a structure describing what the
**							header of a message looks like:
**
**			->sizeMsgHead		the size (in bytes) of the msg header
**			->offBodyLen		the offset (in bytes) to the body length
**			->sizeBodyLen		the size (in bytes) of the body length
**			->funcBodyCalc		ptr to the body calculation function
**
** Output:
**		phCommChannel		Pointer to the returned comm channel handle
**							that was opened
**
** Returns:
**		isOkay				Whether or not this function succeeded:
**								TRUE  => Everything was okay
**								FALSE => Everything was NOT okay
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/

int UtlCommSerial_Open(
		char*	pCommPortStr,	/* ptr to the comm port string			*/
		UTLCOMMSER_DEVICE_DESC*
				pDeviceInfo,	/* ptr to the device info desc			*/
		UTLCOMMSER_HDR_DESC*
				pHeaderInfo,	/* ptr to the msg header info desc		*/
		HANDLE*	phCommChannel)	/* ptr to returned comm channel	handle	*/
{
	int				isOkay = FALSE;		/* return value of this funct	*/
	DCB				dcb;				/* device control block			*/
	DWORD			dwError;			/* last error code				*/
	COMMTIMEOUTS	commTimeouts;		/* the communication timeouts	*/
	BOOL			fSuccess;			/* returned function status		*/
	UTLCOMMSER_CHAN_DESC*	pChanId;	/* ptr to the comm channel desc	*/
 	char*	func="UtlCommSerial_Open";	/* ptr to name of the func		*/

/*	Trace entering this function...
	------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Allocate a new comm channel descriptor.
	--------------------------------------- */
	if ( !comm_create_chan_serial(pCommPortStr, &pChanId) ) {
		goto _errCreateChanSerial;
	}

/*	Load the new comm channel descriptor with the input parameters.
	--------------------------------------------------------------- */
	if ( !comm_load_chan_serial(pChanId, pCommPortStr, pDeviceInfo,
								pHeaderInfo) ) {
		comm_destroy_chan_serial(pChanId);
		goto _errLoadChanSerial;
	}

/*	Open up the communications serial port for the specified comm channel.
	---------------------------------------------------------------------- */
	pChanId->hCommChannel = CreateFile(
				pChanId->pCommPortStr,
				GENERIC_READ | GENERIC_WRITE,
				0,				/* must be opened w/exclusive-access		*/
				NULL,			/* no security attrs						*/
				OPEN_EXISTING,	/* comm devices must use OPEN_EXISTING		*/
				0,				/* not overlapped I/O						*/
				NULL			/* hTemplate must be NULL for comm devices	*/
			);

	if (pChanId->hCommChannel == INVALID_HANDLE_VALUE) {
		dwError = GetLastError();
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Could not open comm port \"%s\", error=%d\n",
				pChanId->pCommPortStr, dwError);
		comm_destroy_chan_serial(pChanId);
		goto _errCreateFile;
	} else {
		UtlLog_Write(LOG_DEBUG | LOG_NO_ID, func, RIGHT_HERE,
			"Opened comm port \"%s\"...\n",
				pChanId->pCommPortStr);
	}

/*	Get the current configuration for the comm device.
	-------------------------------------------------- */
	fSuccess = GetCommState(pChanId->hCommChannel, &dcb);
	if (!fSuccess) {
		dwError = GetLastError();
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Could not get comm port \"%s\" configuration, error=%d\n",
				pChanId->pCommPortStr, dwError);
		comm_destroy_chan_serial(pChanId);
		goto _errGetCommState;
	} else {
		UtlLog_Write(LOG_DEBUG | LOG_NO_ID, func, RIGHT_HERE,
			"Got comm port \"%s\" configuration...\n",
				pChanId->pCommPortStr);
	}

/*	Fill in the DCB (Device Control Block) with the device baud rate,
	the device data bits, the device parity, and the device stop bits.
	------------------------------------------------------------------ */
	dcb.BaudRate = pChanId->deviceInfo.devBaudRate;
	dcb.ByteSize = pChanId->deviceInfo.devDataBits;
	dcb.Parity   = pChanId->deviceInfo.devParity;
	dcb.StopBits = pChanId->deviceInfo.devStopBits;

/*	Set the new configuration for the comm device.
	---------------------------------------------- */
	fSuccess = SetCommState(pChanId->hCommChannel, &dcb);
	if (!fSuccess) {
		dwError = GetLastError();
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Could not set comm port \"%s\" configuration, error=%d\n",
				pChanId->pCommPortStr, dwError);
		comm_destroy_chan_serial(pChanId);
		goto _errSetCommState;
	} else {
		UtlLog_Write(LOG_DEBUG | LOG_NO_ID, func, RIGHT_HERE,
			"Set comm port \"%s\" configuration...\n",
				pChanId->pCommPortStr);
	}

/*	Clear the input and output queues of the comm device.
	----------------------------------------------------- */
	fSuccess = PurgeComm(pChanId->hCommChannel,
						 PURGE_TXCLEAR | PURGE_RXCLEAR);
	if (!fSuccess) {
		dwError = GetLastError();
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Could not clear comm port \"%s\" I/O queues, error=%d\n",
				pChanId->pCommPortStr, dwError);
		comm_destroy_chan_serial(pChanId);
		goto _errPurgeComm;
	} else {
		UtlLog_Write(LOG_DEBUG | LOG_NO_ID, func, RIGHT_HERE,
			"Cleared comm port \"%s\" I/O queues...\n",
				pChanId->pCommPortStr);
	}

/*	Find out what the current timeout information is for the comm device.
	--------------------------------------------------------------------- */
	fSuccess = GetCommTimeouts(pChanId->hCommChannel, &commTimeouts);
	if (!fSuccess) {
		dwError = GetLastError();
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Could not get comm port \"%s\" timeouts, error=%d\n",
				pChanId->pCommPortStr, dwError);
		comm_destroy_chan_serial(pChanId);
		goto _errGetCommTimeouts;
	} else {
		UtlLog_Write(LOG_DEBUG | LOG_NO_ID, func, RIGHT_HERE,
			"Got comm port \"%s\" timeouts...\n",
				pChanId->pCommPortStr);
	}
	UtlLog_Write(LOG_DEBUG | LOG_NO_ID, func, RIGHT_HERE,
		"	ReadIntervalTimeout         = %d\n",
				commTimeouts.ReadIntervalTimeout);
	UtlLog_Write(LOG_DEBUG | LOG_NO_ID, func, RIGHT_HERE,
		"	ReadTotalTimeoutMultiplier  = %d\n",
				commTimeouts.ReadTotalTimeoutMultiplier);
	UtlLog_Write(LOG_DEBUG | LOG_NO_ID, func, RIGHT_HERE,
		"	ReadTotalTimeoutConstant    = %d\n",
				commTimeouts.ReadTotalTimeoutConstant);
	UtlLog_Write(LOG_DEBUG | LOG_NO_ID, func, RIGHT_HERE,
		"	WriteTotalTimeoutMultiplier = %d\n",
				commTimeouts.WriteTotalTimeoutMultiplier);
	UtlLog_Write(LOG_DEBUG | LOG_NO_ID, func, RIGHT_HERE,
		"	WriteTotalTimeoutConstant   = %d\n",
				commTimeouts.WriteTotalTimeoutConstant);

/*	Set the new timeout information for the comm device.
	---------------------------------------------------- */
	commTimeouts.ReadTotalTimeoutConstant  =
								pChanId->deviceInfo.devReadTimeout;
	commTimeouts.WriteTotalTimeoutConstant =
								pChanId->deviceInfo.devWriteTimeout;

	fSuccess = SetCommTimeouts(pChanId->hCommChannel, &commTimeouts);
	if (!fSuccess) {
		dwError = GetLastError();
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Could not set comm port \"%s\" timeouts, error=%d\n",
				pChanId->pCommPortStr, dwError);
		comm_destroy_chan_serial(pChanId);
		goto _errSetCommTimeouts;
	} else {
		UtlLog_Write(LOG_DEBUG | LOG_NO_ID, func, RIGHT_HERE,
			"Set comm port \"%s\" timeouts...\n",
				pChanId->pCommPortStr);
	}

/*	Verify the new timeout information for the comm device.
	------------------------------------------------------- */
	fSuccess = GetCommTimeouts(pChanId->hCommChannel, &commTimeouts);
	if (!fSuccess) {
		dwError = GetLastError();
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Could not get comm port \"%s\" timeouts, error=%d\n",
				pChanId->pCommPortStr, dwError);
		comm_destroy_chan_serial(pChanId);
		goto _errGetCommTimeouts;
	} else {
		UtlLog_Write(LOG_DEBUG | LOG_NO_ID, func, RIGHT_HERE,
			"Got comm port \"%s\" timeouts...\n",
				pChanId->pCommPortStr);
	}
	UtlLog_Write(LOG_DEBUG | LOG_NO_ID, func, RIGHT_HERE,
		"	ReadIntervalTimeout         = %d\n",
				commTimeouts.ReadIntervalTimeout);
	UtlLog_Write(LOG_DEBUG | LOG_NO_ID, func, RIGHT_HERE,
		"	ReadTotalTimeoutMultiplier  = %d\n",
				commTimeouts.ReadTotalTimeoutMultiplier);
	UtlLog_Write(LOG_DEBUG | LOG_NO_ID, func, RIGHT_HERE,
		"	ReadTotalTimeoutConstant    = %d\n",
				commTimeouts.ReadTotalTimeoutConstant);
	UtlLog_Write(LOG_DEBUG | LOG_NO_ID, func, RIGHT_HERE,
		"	WriteTotalTimeoutMultiplier = %d\n",
				commTimeouts.WriteTotalTimeoutMultiplier);
	UtlLog_Write(LOG_DEBUG | LOG_NO_ID, func, RIGHT_HERE,
		"	WriteTotalTimeoutConstant   = %d\n",
				commTimeouts.WriteTotalTimeoutConstant);

/*	Everything went okay, so return the communications handle
	and indicate a good return status.
	--------------------------------------------------------- */
	*phCommChannel = (HANDLE*)pChanId;
	isOkay = TRUE;

/*	Error conditions were detected, so indicate a bad return status.
	---------------------------------------------------------------- */
_errSetCommTimeouts:
_errGetCommTimeouts:
_errPurgeComm:
_errSetCommState:
_errGetCommState:
_errCreateFile:
_errLoadChanSerial:
_errCreateChanSerial:

/*	Trace leaving this function...
	------------------------------ */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...Leaving\n");
	return(isOkay);
}

/*
** Name:
**		UtlCommSerial_Purge
**
** Purpose:
**		This public function is used to purge a specified serial
**		communications channel.
**
** Calling Sequence:
**		isOkay = UtlCommSerial_Purge(HANDLE hCommChannel);
**
** Input:
**
**		hCommChannel		Handle to the serial comm channel to purge
**
** Output:
**		NONE
**
** Returns:
**		isOkay				Whether or not this function succeeded:
**								TRUE  => Everything was okay
**								FALSE => Everything was NOT okay
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int UtlCommSerial_Purge(
		HANDLE	hCommChannel)	/* the comm channel handle				*/
{
	int				isOkay = FALSE;		/* return value of this funct	*/
	DWORD			dwError;			/* last error code				*/
	BOOL			fSuccess;			/* returned function status		*/
	char			nextByte;			/* next garbage byte to swallow	*/
	UTLCOMMSER_CHAN_DESC*	pChanId;	/* ptr to the comm channel desc	*/
	char*	func="UtlCommSerial_Purge";	/* ptr to name of the func		*/

/*	Trace entering this function...
	------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Get the REAL comm channel pointer from the comm channel handle.
	--------------------------------------------------------------- */
	pChanId = (UTLCOMMSER_CHAN_DESC*)hCommChannel;

/*	Clear the input and output queues of the comm device.
	----------------------------------------------------- */
	fSuccess = PurgeComm(pChanId->hCommChannel,
						 PURGE_TXCLEAR | PURGE_RXCLEAR);
	if (!fSuccess) {
		dwError = GetLastError();
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Could not clear comm port \"%s\" I/O queues, error=%d\n",
				pChanId->pCommPortStr, dwError);
		comm_destroy_chan_serial(pChanId);
		goto _errPurgeComm;
	} else {
		UtlLog_Write(LOG_DEBUG | LOG_NO_ID, func, RIGHT_HERE,
			"Cleared comm port \"%s\" I/O queues...\n",
				pChanId->pCommPortStr);
	}

/*	Since the "PurgeComm" function does not appear to do its job
	in a complete fashion, we are going to swallow any garbage that
	still remains in the input queue of the comm device.
	--------------------------------------------------------------- */
	UtlLog_Write(LOG_DEBUG | LOG_NO_ID, func, RIGHT_HERE,
		"Swallowing garbage on comm port \"%s\"...",
				pChanId->pCommPortStr);
	while ( comm_read_from_chan_serial(pChanId, sizeof(char),
									   &nextByte, FALSE) ) {
		UtlLog_Write(LOG_DEBUG | LOG_NO_ID, func, RIGHT_HERE,
			" %02.2x", nextByte);
	}
	UtlLog_Write(LOG_DEBUG | LOG_NO_ID, func, RIGHT_HERE,
		" ...done\n");
 
/*	Everything went okay, so indicate a good return status.
	------------------------------------------------------- */
	isOkay = TRUE;

/*	Error conditions were detected, so indicate a bad return status.
	---------------------------------------------------------------- */
_errPurgeComm:

/*	Trace leaving this function...
	------------------------------ */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...Leaving\n");
	return(isOkay);
}

/*
** Name:
**		UtlCommSerial_Read
**
** Purpose:
**		This public function is used to read a message from a specified
**		serial communication channel.
**
** Calling Sequence:
**		err = UtlCommSerial_Read(
**									HANDLE		hCommChannel,
**									int			sizeMsgMax, 
**									char*		pMsg,
**									int*		pSizeMsg	);
**
** Input:
**
**		hCommChannel		Handle to the serial comm channel to read
**							the message from.
**
**		sizeMsgMax			The maximum size of the message buffer (in
**							bytes) that has been allocated by the caller.
**
** Output:
**
**		pMsg				Pointer to a message buffer that will contain
**							the bytes read from the comm channel.  Note
**							that this is a buffer that is allocated by the
**							caller BEFORE calling this read function.  Since
**							the caller does not know how big the message 
**							will be, this buffer should be allocated to be
**							the size of the largest EXPECTED message.  Of 
**							course, if a message was recieved that could not
**							be copied to this buffer (because the buffer was
**							not large enough), the caller would have an error
**							condition returned.
**
**		pSizeMsg			Pointer to the size of the message (in bytes)
**							actually read from the comm channel.  This
**							size information will be less than or equal to
**							"sizeMsgMax".
**
** Returns:
**		isOkay				Whether or not this function succeeded:
**								TRUE  => Everything was okay
**								FALSE => Everything was NOT okay
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int UtlCommSerial_Read(
	HANDLE	hCommChannel,	/* the comm channel handle			*/
	int		sizeMsgMax,		/* the maximum size of the message 	*/
							/* buffer (in bytes) that has been	*/
							/* allocated by the caller			*/
	char*	pMsg,			/* ptr to a message buffer that		*/
							/* will contain the bytes that were	*/
							/* read from the comm channel		*/
	int*	pSizeMsg)		/* ptr to the size of the message	*/
							/* (in bytes) actually read from 	*/
							/* the comm channel					*/
{
	int				isOkay = FALSE;		/* return value of this funct	*/
	int				szMsgBody;			/* size of the message body		*/
	int				szMsgHead;			/* size of the message header	*/
	UTLCOMMSER_CHAN_DESC*	pChanId;	/* ptr to the comm channel desc	*/
	char*	func="UtlCommSerial_Read";	/* ptr to name of the func		*/

/*	Trace entering this function...
	------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Get the REAL comm channel pointer from the comm channel handle.
	--------------------------------------------------------------- */
	pChanId = (UTLCOMMSER_CHAN_DESC*)hCommChannel;

/*	Indicate that nothing has been read at this time.
	------------------------------------------------- */
	*pSizeMsg = 0;
	
/*	Make sure that the caller has allocated enough space to receive
	the header of the message.
	--------------------------------------------------------------- */
	szMsgHead = comm_get_header_size_serial(pChanId);
	if ( szMsgHead  > sizeMsgMax ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Message header (%d bytes) is too large (max=%d bytes)\n",
				szMsgHead, sizeMsgMax);
		goto _errHeaderTooLarge;
	}

/*	First read the number of bytes found in the expected header of
	the incoming message.
	-------------------------------------------------------------- */
	if ( !comm_read_from_chan_serial(pChanId, szMsgHead, pMsg, TRUE) ) {
		*pSizeMsg = 0;
		goto _errHeaderNotRead;
	}

/*	Now that we have read in the message header, decode the header
	to determine how many more bytes need to be read to get the 
	rest of the incoming message.
	-------------------------------------------------------------- */
	szMsgBody = comm_get_body_size_serial(pChanId, pMsg);
	if (szMsgBody < 0) {
		goto _errGetBodySize;
	}

/*	If there is only a message header and no body, then we are done.
	---------------------------------------------------------------- */
	if (szMsgBody == 0) {
		*pSizeMsg = szMsgHead;
		goto _allOkay;
	}

/*	Make sure that the body of the message has a reasonable length.
	--------------------------------------------------------------- */
	if (szMsgBody > sizeMsgMax) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Message body (%d bytes) is too large (max=%d bytes)\n",
				szMsgBody, sizeMsgMax);
		goto _errBodyTooLarge;
	}
	
/*	Make sure that the caller has allocated enough space to receive
	the body of the message.
	--------------------------------------------------------------- */
	if ( (szMsgHead + szMsgBody) > sizeMsgMax ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Message (%d bytes) is too large (max=%d bytes)\n",
				(szMsgHead + szMsgBody), sizeMsgMax);
		goto _errMsgTooLarge;
	}

/*	Now read the body of the message into the caller's data space.
	-------------------------------------------------------------- */
	if ( !comm_read_from_chan_serial(pChanId, szMsgBody,
									 (pMsg + szMsgHead), TRUE) ) {
		goto _errReadingBody;
	}
	*pSizeMsg = szMsgHead + szMsgBody;
 
/*	Everything went okay, so indicate a good return status.
	------------------------------------------------------- */
_allOkay:
	isOkay = TRUE;

/*	Error conditions were detected, so indicate a bad return status.
	---------------------------------------------------------------- */
_errReadingBody:
_errMsgTooLarge:
_errBodyTooLarge:
_errGetBodySize:
_errHeaderNotRead:
_errHeaderTooLarge:

/*	Trace leaving this function...
	------------------------------ */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...Leaving\n");
	return(isOkay);
}

/*
** Name:
**		UtlCommSerial_Write
**
** Purpose:
**		This public function is used to write a message to a specified
**		serial communication channel.
**
** Calling Sequence:
**		err = UtlCommSerial_Write(
**									HANDLE		hCommChannel,
**									int			sizeMsgTotal, 
**									char*		pMsg,
**									int*		pSizeMsg	);
**
** Input:
**
**		hCommChannel		Handle to the serial comm channel to write
**							the message to.
**
**		sizeMsgTotal		The total size of the message buffer (in
**							bytes) to be written to the comm channel.
**
**		pMsg				Pointer to a message buffer to be written
**							to the comm channel.
**
** Output:
**
**		pSizeMsg			Pointer to the size of the message (in bytes)
**							actually written to the comm channel.  This
**							size information will be less than or equal to
**							"sizeMsgTotal".
**
** Returns:
**		isOkay				Whether or not this function succeeded:
**								TRUE  => Everything was okay
**								FALSE => Everything was NOT okay
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int UtlCommSerial_Write(
	HANDLE	hCommChannel,	/* the comm channel handle			*/
	int		sizeMsgTotal,	/* the total size of the message 	*/
							/* buffer (in bytes) to be written	*/
							/* to the comm channel				*/
	char*	pMsg,			/* ptr to a message buffer to be	*/
							/* written to the comm channel		*/
	int*	pSizeMsg)		/* ptr to the size of the message	*/
							/* (in bytes) actually written to	*/
{
	int				isOkay = FALSE;		/* return value of this funct	*/
	UTLCOMMSER_CHAN_DESC*	pChanId;	/* ptr to the comm channel desc	*/
	char*	func="UtlCommSerial_Write";	/* ptr to name of the func		*/

/*	Trace entering this function...
	------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Get the REAL comm channel pointer from the comm channel handle.
	--------------------------------------------------------------- */
	pChanId = (UTLCOMMSER_CHAN_DESC*)hCommChannel;

/*	Indicate that nothing has been read at this time.
	------------------------------------------------- */
	*pSizeMsg = 0;

/*	Get the entire message written out to the comm channel.
	------------------------------------------------------- */
	if ( !comm_write_to_chan_serial(pChanId, sizeMsgTotal, pMsg, TRUE) ) {
		goto _errWritingMsg;
	}
	*pSizeMsg = sizeMsgTotal;
 
/*	Everything went okay, so indicate a good return status.
	------------------------------------------------------- */
	isOkay = TRUE;

/*	Error conditions were detected, so indicate a bad return status.
	---------------------------------------------------------------- */
_errWritingMsg:

/*	Trace leaving this function...
	------------------------------ */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...Leaving\n");
	return(isOkay);
}

/*
** Name:
**			comm_create_chan_serial
**
** Purpose:
**			This local function allocates the memory that is 
**			required for creating a new serial comm channel descriptor.
*/
static int comm_create_chan_serial(
	char*		pCommPortStr,			/* ptr to the comm port string	*/
	UTLCOMMSER_CHAN_DESC**	ppChanId)	/* ptr to ptr to the new chan	*/
{
	int				isOkay = FALSE;		/* return value of this funct	*/
	char*	func="comm_create_chan_serial";		/* ptr to name of func	*/

/*	Trace entering this function...
	------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Allocate the memory for the basic comm channel descriptor structure.
	-------------------------------------------------------------------- */
	*ppChanId = (UTLCOMMSER_CHAN_DESC*)calloc(1,
											  sizeof(UTLCOMMSER_CHAN_DESC));
	if ( *ppChanId == NULL ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: No more memory available\n");
		goto _errOutOfMemory;
	}

/*	Allocate the memory for the comm port string and attach it to the
	newly created comm channel descriptor.
	------------------------------------------------------------------ */
	if (pCommPortStr != NULL) {
		(*ppChanId)->pCommPortStr =
				(char*)calloc(1, strlen(pCommPortStr)+1);
		if ( (*ppChanId)->pCommPortStr == NULL ) {
			UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
				"ERROR: No more memory available\n");
			goto _errOutOfMemory;
		}
	}
 
/*	Everything went okay, so indicate a good return status.
	------------------------------------------------------- */
	isOkay = TRUE;

/*	Error conditions were detected, so indicate a bad return status.
	---------------------------------------------------------------- */
_errOutOfMemory:

/*	Trace leaving this function...
	------------------------------ */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...Leaving\n");
	return(isOkay);
}

/*
** Name:
**			comm_destroy_chan_serial
**
** Purpose:
**			This local function deallocates the memory that was
**			required for creating a new comm channel descriptor.
*/
static int comm_destroy_chan_serial(
	UTLCOMMSER_CHAN_DESC*	pChanId)	/* ptr to the comm channel desc	*/
{
	int				isOkay = FALSE;		/* return value of this funct	*/
	char*	func="comm_destroy_chan_serial";	/* ptr to name of func	*/

/*	Trace entering this function...
	------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Make sure there is a valid pointer to the comm channel ID.
	--------------------------------------------------------- */
	if (pChanId == NULL) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Invalid comm channel descriptor\n");
		goto _errInvalidDescriptor;
	}

/*	Deallocate the memory for the comm port string.
	----------------------------------------------- */
	if (pChanId->pCommPortStr) {
		free(pChanId->pCommPortStr);
	}

/*	Deallocate the memory for the basic comm channel descriptor.
	------------------------------------------------------------ */
	free(pChanId);
 
/*	Everything went okay, so indicate a good return status.
	------------------------------------------------------- */
	isOkay = TRUE;

/*	Error conditions were detected, so indicate a bad return status.
	---------------------------------------------------------------- */
_errInvalidDescriptor:

/*	Trace leaving this function...
	------------------------------ */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...Leaving\n");
	return(isOkay);
}

/*
** Name:
**			comm_get_body_size_serial
**
** Purpose:
**			This local function gets the size of a message
**			body out of the header for the message, using
**			the header information descriptor.
*/
static int comm_get_body_size_serial(
	UTLCOMMSER_CHAN_DESC*	pChanId,	/* ptr to the comm channel desc		*/
	char*					pMsgHead)	/* ptr to msg header buffer			*/
{
	char		sizeBodyChar;		/* size of the body as a CHAR		*/
	short		sizeBodyShort;		/* size of the body as a SHORT		*/
	long		sizeBodyLong;		/* size of the body as a LONG		*/
	int			numBodyBytes;		/* number of bytes for the body		*/
	char*	func="comm_get_body_size_serial";	/* ptr to name of func	*/

/*	Trace entering this function...
	------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Extract out the message body size from the message header, but
	only if the application has NOT supplied a special "Body Calculation"
	function for determining the ACTUAL size of the body (in bytes).
	--------------------------------------------------------------------- */
	if (pChanId->headerInfo.funcBodyCalc == NULL) {
		switch (pChanId->headerInfo.sizeBodyLen) {
			case BODY_SIZE_CHAR:
				memcpy(	&sizeBodyChar,
						(pMsgHead + pChanId->headerInfo.offBodyLen),
						pChanId->headerInfo.sizeBodyLen	);
				numBodyBytes = (int)sizeBodyChar;
				break;
			case BODY_SIZE_SHORT:
				memcpy(	&sizeBodyShort,
						(pMsgHead + pChanId->headerInfo.offBodyLen),
						pChanId->headerInfo.sizeBodyLen	);
				numBodyBytes = (int)sizeBodyShort;
				break;
			case BODY_SIZE_LONG:
				memcpy(	&sizeBodyLong,
						(pMsgHead + pChanId->headerInfo.offBodyLen),
						pChanId->headerInfo.sizeBodyLen	);
				numBodyBytes = (int)sizeBodyLong;
				break;
			default:
				UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
					"ERROR: Invalid body size in header descriptor = %d\n",
						pChanId->headerInfo.sizeBodyLen	);
				numBodyBytes = BODY_SIZE_INVALID;
				break;
		}
	} else {

/*		The application HAS supplied a callback function to calculate
		the number of bytes that need to be read for the body.  So,
		just call this function and let the application tell us how
		many bytes need to be read.  This application's callback function
		always needs to be given back a pointer to the message header.
		------------------------------------------------------------------ */
		numBodyBytes = (*(pChanId->headerInfo.funcBodyCalc))(pMsgHead);
	}

/*	Trace leaving this function...
	------------------------------ */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...Leaving\n");
	return(numBodyBytes);
}

/*
** Name:
**			comm_get_header_size_serial
**
** Purpose:
**			This local function gets the header size for a specified
**			serial comm channel.
*/
static int comm_get_header_size_serial(
	UTLCOMMSER_CHAN_DESC*	pChanId)	/* ptr to the comm channel desc		*/
{
	int			numHeaderBytes;		/* number of bytes for the header	*/
	char*	func="comm_get_header_size_serial";/* ptr to name of func	*/

/*	Trace entering this function...
	------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");
 
/*	Determine the body size from the header that has been read.
	----------------------------------------------------------- */
	numHeaderBytes = pChanId->headerInfo.sizeMsgHead;
	
/*	Trace leaving this function...
	------------------------------ */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...Leaving\n");
	return(numHeaderBytes);
}

/*
** Name:
**			comm_load_chan_serial
**
** Purpose:
**			This local function loads the validated input parameters
**			for the "UtlCommSerial_Open" functions into a newly
**			created comm channel descriptor.
*/
static int comm_load_chan_serial(
	UTLCOMMSER_CHAN_DESC*	pChanId,	/* ptr to the comm channel desc		*/
	char*				pCommPortStr,	/* ptr to the comm port string		*/
	UTLCOMMSER_DEVICE_DESC*
						pDeviceInfo,	/* ptr to the device info desc		*/
	UTLCOMMSER_HDR_DESC*
						pHeaderInfo)	/* ptr to the msg header info desc	*/
{
	int				isOkay = FALSE;		/* return value of this funct	*/
	char*	func="comm_load_chan_serial";		/* ptr to name of func	*/

/*	Trace entering this function...
	------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Load the comm port string.
	-------------------------- */
	if (pCommPortStr != NULL) {
		strcpy(pChanId->pCommPortStr, pCommPortStr);
	}

/*	Load the device information.
	---------------------------- */
	pChanId->deviceInfo = *pDeviceInfo;

/*	Load the message header information.
	------------------------------------ */
	pChanId->headerInfo = *pHeaderInfo;
 
/*	Everything went okay, so indicate a good return status.
	------------------------------------------------------- */
	isOkay = TRUE;

/*	Trace leaving this function...
	------------------------------ */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...Leaving\n");
	return(isOkay);
}

/*
** Name:
**			comm_read_from_chan_serial
**
** Purpose:
**			This local function reads a specified number of bytes
**			from a specified serial comm channel into a specified
**			buffer that has been allocated by the caller.
*/
static int comm_read_from_chan_serial(
	UTLCOMMSER_CHAN_DESC*	pChanId,	/* ptr to the comm channel desc		*/
	int					sizeToRead,		/* the size (in bytes) to read		*/
	char*				pBuff,			/* ptr to the buffer to read into	*/
	int					fReportErrs)	/* report errors indicator flag		*/
{
	int			isOkay = FALSE;		/* return value of this funct			*/
	int			numTries;			/* the number of attempts to try 		*/
	int			sizeReadThisTime;	/* the number of bytes read this time	*/
	int			sizeLeftToRead;		/* the number of bytes left to read		*/
	int			sizeReadSoFar;		/* the number of bytes read so far		*/
	BOOL		fSuccess;			/* returned function status				*/
	DWORD		dwError;			/* the last error code					*/
	LPVOID		lpMsgBuf;			/* ptr to error message buffer			*/
	char*	func="comm_read_from_chan_serial";	/* ptr to name of func		*/

/*	Trace entering this function...
	------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Get ready to read all the requested bytes from the comm channel.
	---------------------------------------------------------------- */
	numTries       = 0;
	sizeReadSoFar  = 0;
	sizeLeftToRead = sizeToRead;

/*	Loop until we have gathered up all the bytes requested by the caller.
	--------------------------------------------------------------------- */
	while ( (sizeReadSoFar != sizeToRead) &&
			(numTries < MAX_READ_TIMEOUTS) ) {
		fSuccess = ReadFile(	pChanId->hCommChannel,
								(LPVOID)(pBuff + sizeReadSoFar),
								(DWORD)sizeLeftToRead,
								(LPDWORD)&sizeReadThisTime,
								NULL
							);
		if (!fSuccess) {
			if (fReportErrs) {
				dwError = GetLastError();
				UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
					"ERROR: Could not read from \"%s\", error=%d\n",
						pChanId->pCommPortStr, dwError);
				FormatMessage(	FORMAT_MESSAGE_ALLOCATE_BUFFER |
								FORMAT_MESSAGE_FROM_SYSTEM,
								NULL,
								GetLastError(),
								MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
								(LPTSTR) &lpMsgBuf,
								0,
								NULL	);
				UtlLog_Write(LOG_ERROR | LOG_NO_ID, func, RIGHT_HERE,
					"FormatMessage = \"%s\"", lpMsgBuf);
				LocalFree( lpMsgBuf );
			}
			goto _errReadFile;
		}
		sizeLeftToRead -= sizeReadThisTime;
		sizeReadSoFar  += sizeReadThisTime;

/*		Bump the number of tries to get all the bytes.
		---------------------------------------------- */
		numTries++;
	}

/*	Check to see if we read the number of bytes that were expected.
	--------------------------------------------------------------- */
	if (sizeReadSoFar != sizeToRead) {
		if (fReportErrs) {
			UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
				"Tried to read %d bytes, only read %d\n", sizeToRead,
					sizeReadSoFar);
			if (sizeReadSoFar > 0) {
				UtlDump_Mem(pBuff, sizeReadSoFar, LOG_ERROR | LOG_NO_ID);
			}
		}
		goto _errReadTooFewBytes;
	}
 
/*	Everything went okay, so indicate a good return status.
	------------------------------------------------------- */
	isOkay = TRUE;

/*	Error conditions were detected, so indicate a bad return status.
	---------------------------------------------------------------- */
_errReadTooFewBytes:
_errReadFile:

/*	Trace leaving this function...
	------------------------------ */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...Leaving\n");
	return(isOkay);
}

/*
** Name:
**			comm_write_to_chan_serial
**
** Purpose:
**			This local function writes a specified number of bytes
**			to a specified serial comm channel from a specified buffer
**			that has been allocated by the caller.
*/
static int comm_write_to_chan_serial(
	UTLCOMMSER_CHAN_DESC*	pChanId,	/* ptr to the comm channel desc		*/
	int					sizeToWrite,	/* the size (in bytes) to write		*/
	char*				pBuff,			/* ptr to the buffer to write from	*/
	int					fReportErrs)	/* report errors indicator flag		*/
{
	int			isOkay = FALSE;			/* return value of this funct		*/
	int			numTries;				/* number of attempts to try 		*/
	int			sizeWrittenThisTime;	/* number of bytes written this time*/
	int			sizeLeftToWrite;		/* number of bytes left to write 	*/
	int			sizeWrittenSoFar;		/* number of bytes written so far	*/
	BOOL		fSuccess;				/* returned function status			*/
	DWORD		dwError;				/* the last error code				*/
	LPVOID		lpMsgBuf;				/* ptr to error message buffer		*/
	char*	func="comm_write_to_chan_serial";	/* ptr to name of func		*/

/*	Trace entering this function...
	------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Get ready to write all the requested bytes to the comm channel.
	--------------------------------------------------------------- */
	numTries         = 0;
	sizeWrittenSoFar = 0;
	sizeLeftToWrite  = sizeToWrite;

/*	Loop until we have sent out all the bytes requested by the caller.
	------------------------------------------------------------------ */
	while ( (sizeWrittenSoFar != sizeToWrite) &&
			(numTries < MAX_WRITE_TIMEOUTS) ) {
		fSuccess = WriteFile(	pChanId->hCommChannel,
								(LPVOID)(pBuff + sizeWrittenSoFar),
								(DWORD)sizeLeftToWrite,
								(LPDWORD)&sizeWrittenThisTime,
								NULL
							);
		if (!fSuccess) {
			if (fReportErrs) {
				dwError = GetLastError();
				UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
					"ERROR: Could not write to \"%s\", error=%d\n",
						pChanId->pCommPortStr, dwError);
				FormatMessage(	FORMAT_MESSAGE_ALLOCATE_BUFFER |
								FORMAT_MESSAGE_FROM_SYSTEM,
								NULL,
								GetLastError(),
								MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
								(LPTSTR) &lpMsgBuf,
								0,
								NULL	);
				UtlLog_Write(LOG_ERROR | LOG_NO_ID, func, RIGHT_HERE,
					"FormatMessage = \"%s\"", lpMsgBuf);
				LocalFree( lpMsgBuf );
			}
			goto _errWriteFile;
		}
		sizeLeftToWrite  -= sizeWrittenThisTime;
		sizeWrittenSoFar += sizeWrittenThisTime;

/*		Bump the number of tries to send all the bytes.
		----------------------------------------------- */
		numTries++;
	}

/*	Check to see if we wrote the number of bytes that were expected.
	---------------------------------------------------------------- */
	if (sizeWrittenSoFar != sizeToWrite) {
		if (fReportErrs) {
			UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
				"Tried to send %d bytes, only sent %d\n", sizeToWrite,
					sizeWrittenSoFar);
			if (sizeWrittenSoFar > 0) {
				UtlDump_Mem(pBuff, sizeWrittenSoFar, LOG_ERROR | LOG_NO_ID);
			}
		}
		goto _errWroteTooFewBytes;
	}
 
/*	Everything went okay, so indicate a good return status.
	------------------------------------------------------- */
	isOkay = TRUE;

/*	Error conditions were detected, so indicate a bad return status.
	---------------------------------------------------------------- */
_errWroteTooFewBytes:
_errWriteFile:

/*	Trace leaving this function...
	------------------------------ */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...Leaving\n");
	return(isOkay);
}
