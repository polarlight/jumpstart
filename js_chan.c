/*
* 			Copyright 1997 JumpStart, Inc.
* 
* 						CONFIDENTIAL
* 
* 					  Copyright Notice
* 					  ----------------
* 
* Copyright 1997 JumpStart, Inc. All rights reserved.
* Reproduction in whole or in part without the express written consent
* of JumpStart Inc. is expressly prohibited. 
* 
*/

/*
** Name of File:
**		js_chan.c
**
** Description:
**		This is the implementation file for the "js_chan" module of the
**		JumpStart library, which is a collection of general-purpose utility 
**		functions that are used by all other subsystems.  This module
**      contains library functions related to performing basic communications
**		using either TCP/IP, UDP, or RS232 serial communication.
**
** Subsystem:
**      JumpStart Library
**
** Version:
**      1.00
**
** Public Functions:
**		jsChanAccept
**		jsChanCreate
**		jsChanDelete
**		jsChanHasConnect
**		jsChanHasInput
**		jsChanRead
**		jsChanWrite
**
** History:
**		Date		Engineer			Purpose
**		--------	----------------	---------------------------------
**		07/22/97	Steven M. Rosen		Initial creation.
*/

/* Special Definitions */
/* ------------------- */
#define	FD_SETSIZE			256		/* max number of socket FDs	*/

/* Include Files */
/* ------------- */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <WINSOCK.H>
//#include <netinet/in.h>
//#include <netdb.h>
//#include <sys/ioctl.h>
//#include <time.h>
#include "js_bit.h"
#include "js_log.h"
#include "js_mem.h"
#include "js_dump.h"
#include "js_time.h"
#include "js_chan.h"

/* Private Definitions */
/* ------------------- */

/* miscellaneous constants */
/* ----------------------- */
#define	BODY_SIZE_CHAR		sizeof(char)	/* size of body in a CHAR	*/
#define	BODY_SIZE_SHORT		sizeof(short)	/* size of body in a SHORT	*/
#define	BODY_SIZE_LONG		sizeof(long)	/* size of body in a LONG	*/

#define	TYPE_SOCKET_IO			0			/* an I/O socket type		*/
#define	TYPE_SOCKET_LISTEN		1			/* a listening socket type	*/
#define	MASK_SOCKET_IO		(1 << TYPE_SOCKET_IO)
											/* an I/O socket mask		*/
#define	MASK_SOCKET_LISTEN	(1 << TYPE_SOCKET_LISTEN)
											/* a listening socket mask	*/

#define	BLOCKING_ON			0				/* socket is blocking		*/
#define	BLOCKING_OFF		1				/* socket is non-blocking	*/

#define	REASONABLE_CONN_TIMEOUT		3		/* number of seconds for a 	*/
											/* reasonable connection	*/
											/* timeout value			*/

#define	USECS_PER_SEC		1000000			/* number of usecs in a sec	*/

/* Private Structures */
/* ------------------ */

/* Private Global Variables */
/* ------------------------ */

/* Private Prototype Definitions */
/* ----------------------------- */

static int comm_calc_max_timeouts(
				struct timeval*		pTimeOut,
				struct timeval*		pTimeOutCB,
				int*				pMaxTimeOutsCB	);

static long comm_create_chan(
				char* pNameHost, char* pNameService,
				UTLCOMM_HDR_DESC* pHeadInfo,
				UTLCOMM_CHAN_DESC** ppChanId	);

static long comm_destroy_chan(
				UTLCOMM_CHAN_DESC* pChanId	);

static long comm_get_body_size(
				UTLCOMM_HDR_DESC* pHeaderInfo, char* pBuff	);

static long comm_load_chan(
				UTLCOMM_CHAN_DESC* pChanId, int isMaster,
				int typeComm, int typeApp, 
				int maxConnections, int sockListen,
				char* pNameHost, char* pNameService,
				UTLCOMM_HDR_DESC* pHeaderInfo	);

static long comm_accept_chan(
				UTLCOMM_CHAN_DESC* pChanId	);
static long comm_accept_chan_client_udp(
				UTLCOMM_CHAN_DESC* pChanId	);
static long comm_accept_chan_client_tcp(
				UTLCOMM_CHAN_DESC* pChanId	);
static long comm_accept_chan_server_udp(
				UTLCOMM_CHAN_DESC* pChanId	);
static long comm_accept_chan_server_tcp(
				UTLCOMM_CHAN_DESC* pChanId	);

static long comm_open_chan(
				UTLCOMM_CHAN_DESC* pChanId,
				struct timeval* pConnTimeout	);
static long comm_open_chan_client_udp(
				UTLCOMM_CHAN_DESC* pChanId,
				struct timeval* pConnTimeout	);
static long comm_open_chan_client_tcp(
				UTLCOMM_CHAN_DESC* pChanId,
				struct timeval* pConnTimeout	);
static long comm_open_chan_server_udp(
				UTLCOMM_CHAN_DESC* pChanId	);
static long comm_open_chan_server_tcp(
				UTLCOMM_CHAN_DESC* pChanId	);

static long comm_read_chan(
				UTLCOMM_CHAN_DESC* pChanId,
				int sizeMsgMax, struct timeval* pTimeOut,
				int (*pFuncCB)(), void* pDataCB, struct timeval* pTimeOutCB,
				char* pMsg, int* pSizeMsg	);
static long comm_read_chan_udp(
				UTLCOMM_CHAN_DESC* pChanId,
				int sizeMsgMax, struct timeval* pTimeOut,
				int (*pFuncCB)(), void* pDataCB, struct timeval* pTimeOutCB,
				char* pMsg, int* pSizeMsg	);
static long comm_read_chan_tcp(
				UTLCOMM_CHAN_DESC* pChanId,
				int sizeMsgMax, struct timeval* pTimeOut,
				int (*pFuncCB)(), void* pDataCB, struct timeval* pTimeOutCB,
				char* pMsg, int* pSizeMsg	);
static long comm_read_from_chan(
				UTLCOMM_CHAN_DESC* pChanId,
				int sizeBuff, char* pBuff	);

static long comm_read_block_chan(
				UTLCOMM_CHANLIST_DESC* pListChanIds,
				int sizeMsgMax, struct timeval* pTimeOut,
				int (*pFuncCB)(), void* pDataCB, struct timeval* pTimeOutCB,
				char* pMsg, int* pSizeMsg, UTLCOMM_CHAN_DESC** ppChanId	);
static long comm_read_block_chan_udp(
				UTLCOMM_CHANLIST_DESC* pListChanIds,
				int sizeMsgMax, struct timeval* pTimeOut,
				int (*pFuncCB)(), void* pDataCB, struct timeval* pTimeOutCB,
				char* pMsg, int* pSizeMsg, UTLCOMM_CHAN_DESC** ppChanId	);
static long comm_read_block_chan_tcp(
				UTLCOMM_CHANLIST_DESC* pListChanIds,
				int sizeMsgMax, struct timeval* pTimeOut,
				int (*pFuncCB)(), void* pDataCB,
				struct timeval* pTimeOutCB,
				char* pMsg, int* pSizeMsg, UTLCOMM_CHAN_DESC** ppChanId	);

static int comm_read_select(
				int						typeSocket,
				UTLCOMM_CHANLIST_DESC*	pListChanIds,
				fd_set*					pMaskRead,
				fd_set*					pMaskWrite,
				fd_set*					pMaskExcept,
				struct timeval*			pTimeOut,
				int						(*pFuncCB)(),
				void*					pDataCB,
				struct timeval*			pTimeOutCB,
				int*					pUserAbort		);

static long comm_write_chan(
				UTLCOMM_CHAN_DESC* pChanId,
				int sizeMsgTotal, struct timeval* pTimeOut,
				int (*pFuncCB)(), void* pDataCB, struct timeval* pTimeOutCB,
				char* pMsg, int* pSizeMsg	);
static long comm_write_chan_udp(
				UTLCOMM_CHAN_DESC* pChanId,
				int sizeMsgTotal, struct timeval* pTimeOut,
				int (*pFuncCB)(), void* pDataCB, struct timeval* pTimeOutCB,
				char* pMsg, int* pSizeMsg	);
static long comm_write_chan_tcp(
				UTLCOMM_CHAN_DESC* pChanId,
				int sizeMsgTotal, struct timeval* pTimeOut,
				int (*pFuncCB)(), void* pDataCB, struct timeval* pTimeOutCB,
				char* pMsg, int* pSizeMsg	);
static long comm_write_to_chan(
				UTLCOMM_CHAN_DESC* pChanId,
				int sizeBuff, char* pBuff	);

static long comm_validate_accept_parms(
				UTLCOMM_CHAN_DESC*	pMasterChanId,
				UTLCOMM_CHAN_DESC**	ppChanId	);
static long comm_validate_close_parms(
				UTLCOMM_CHAN_DESC* pChanId	);
static long comm_validate_connect_pending_parms(
				UTLCOMM_CHAN_DESC* pChanId,
				struct timeval*	pTimeOut	);
static long comm_validate_input_pending_parms(
				UTLCOMM_CHANLIST_DESC* pListChanIds,
				struct timeval* pTimeOut,
				UTLCOMM_CHAN_DESC** ppChanId	);
static long comm_validate_open_parms(
				int typeComm, int typeApp,
				int maxConnections, struct timeval* pConnTimeout,
				char* pNameHost, char* nameHeader,
				UTLCOMM_HDR_DESC* pHeaderInfo,
				UTLCOMM_CHAN_DESC** ppChanId	);
static long comm_validate_read_parms(
				UTLCOMM_CHAN_DESC* pChanId,
				int sizeMsgMax, struct timeval* pTimeOut,
				int (*pFuncCB)(), void* pDataCB, struct timeval* pTimeOutCB,
				char* pMsg, int* pSizeMsg	);
static long comm_validate_read_block_parms(
				UTLCOMM_CHANLIST_DESC* pListChanIds,
				int sizeMsgMax, struct timeval* pTimeOut,
				int (*pFuncCB)(), void* pDataCB, struct timeval* pTimeOutCB,
				char* pMsg, int* pSizeMsg, UTLCOMM_CHAN_DESC** ppChanId	);
static long comm_validate_write_parms(
				UTLCOMM_CHAN_DESC* pChanId,
				int sizeMsgMax, struct timeval* pTimeOut,
				int (*pFuncCB)(), void* pDataCB, struct timeval* pTimeOutCB,
				char* pMsg, int* pSizeMsg	);

/*
** Name:
**		jsChanAccept
**
** Purpose:
**		This function is used to ACCEPT a new communication channel
**		object from a specified Server communication channel object.
**
** Calling Sequence:
**		isOkay = jsChanAccept(hLog, hServerChan, phChan, pFailReason);
**
** Input:
**		hLog			handle to associated log object
**		hServerChan		handle to Server channel object
**
** Output:
**		phChan			ptr to the returned handle to the new
**						channel that was accepted from the
**						Server channel
**		pFailReason		ptr to returned reason for any failure
**						condition
**
** Returns:
**		isOkay			Boolean indicating whether or not the
**						function executed successfully
**
** Error Conditions:
**
**		Error Code						Meaning
**		----------						-------
**		JS_SUCCESS						Success
**		JS_CHAN_ERR_ACCEPT				Could not accept on the listen socket
**		JS_CHAN_ERR_INVALID_HANDLE		Invalid handle
**		JS_CHAN_ERR_INVALID_PTR			Invalid pointer
**		JS_CHAN_ERR_NOT_ALLOWED			Accept not allowed for CLIENT_APP
**		JS_CHAN_ERR_OPEN_TCP_SOCKET		Could not open a TCP/IP socket
**		JS_CHAN_ERR_OUT_OF_MEMORY		Could not obtain enough memory
**
** Comments:
**		NONE
*/
int jsChanAccept(
	HANDLE		hLog,			/* handle to associated log object			*/
	HANDLE		hServerChan,	/* handle to Server channel object			*/
	HANDLE*		phChan,			/* ptr to the returned handle to the new	*/
								/* channel that was accepted from the		*/
								/* Server channel							*/
 	int*		pFailReason)	/* ptr to returned reason for any failure	*/
								/* condition								*/
{
	UTLCOMM_CHAN_DESC*	pNewChanId;		/* ptr to a new comm channel ID	*/
	long	err;						/* the returned error code	*/
	char*	func = "jsChanAccept";	/* ptr to name of function	*/

/*	Allow for tracing information on ENTRY.
	--------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Validate the input parameters.
	------------------------------ */
	err = comm_validate_accept_parms(pMasterChanId, ppChanId);
	if (err != UTL_SUCCESS) {
		return(err);
	}

/*	Allocate a new comm channel ID.
	------------------------------- */
	err = comm_create_chan(	pMasterChanId->pNameHost,
							pMasterChanId->pNameService,
							pMasterChanId->pHeaderInfo,
							&pNewChanId	);
	if (err != UTL_SUCCESS) {
		comm_destroy_chan(pNewChanId);
		return(err);
	}

/*	Load the new comm channel ID with the input parameters.
	------------------------------------------------------- */
	err = comm_load_chan(	pNewChanId,
							FALSE,
							pMasterChanId->typeComm,
							pMasterChanId->typeApp,
							pMasterChanId->maxConnections,
							pMasterChanId->sockListen,
							pMasterChanId->pNameHost,
							pMasterChanId->pNameService,
							pMasterChanId->pHeaderInfo	);
	if (err != UTL_SUCCESS) {
		comm_destroy_chan(pNewChanId);
		return(err);
	}

/*	Accept the comm channel that has been specified by the caller.
	-------------------------------------------------------------- */
	err = comm_accept_chan(pNewChanId);
	if (err != UTL_SUCCESS) {
		comm_destroy_chan(pNewChanId);
		return(err);
	}

/*	Allow for tracing information on EXIT.
	-------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

/*	Everything went well, so load the new channel ID into the
	caller's space and return a successful status.
	---------------------------------------------------------- */
	*ppChanId = pNewChanId;
	return(UTL_SUCCESS);
}

/*
** Name:
**		jsChanDelete
**
** Purpose:
**		This function is used to delete a communication channel
**		object.
**
** Calling Sequence:
**		isOkay = jsChanDelete(hLog, hChan, pFailReason);
**
** Input:
**		hLog			handle to associated log object
**		hChan			handle to channel object
**
** Output:
**		pFailReason		ptr to returned reason for any failure
**						condition
**
** Returns:
**		isOkay			Boolean indicating whether or not the
**						function executed successfully
**
** Error Conditions:
**
**		Error Code						Meaning
**		----------						-------
**		JS_SUCCESS						Success
**		JS_CHAN_ERR_INVALID_HANDLE		Invalid handle
**		JS_CHAN_ERR_INVALID_PTR			Invalid pointer
**
** Comments:
**		NONE
*/
int jsChanDelete(
	HANDLE		hLog,			/* handle to associated log object			*/
	HANDLE		hChan,			/* handle to channel object					*/
 	int*		pFailReason)	/* ptr to returned reason for any failure	*/
								/* condition								*/
{
	long	err;						/* the returned error code	*/
	char*	func="jsChanDelete";		/* ptr to name of function	*/

/*	Allow for tracing information on ENTRY.
	--------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Validate the input parameters.
	------------------------------ */
	err = comm_validate_close_parms(pChanId);
	if (err != UTL_SUCCESS) {
		return(err);
	}

/*	Close the comm channel by using the local destroy function.
	----------------------------------------------------------- */
	err = comm_destroy_chan(pChanId);

/*	Allow for tracing information on EXIT.
	-------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(err);
}

/*
** Name:
**		jsChanHasConnect
**
** Purpose:
**		This function is used to determine if a connection is
**		pending for a specified communication channel.  Note
**		that a check is made to verify that the communication
**		channel is associated with an application that is acting
**		as a Server (JS_APP_TYPE_SERVER).
**
** Calling Sequence:
**		isOkay = jsChanHasConnect(hLog, hChan, pTimeout,
**								  pHasConnect, pFailReason);
**
** Input:
**		hLog			handle to associated log object
**		hChan			handle to channel object
**		pTimeout		pointer to the timeout to apply to the check
**						for a pending connection before returning to
**						the caller
**
**						Note that a NULL value for this input parameter
**						is valid and will cause the process to block
**						until a pending connection has been detected.
**						A non-NULL value means that the caller is supplying
**						a pointer to a timeout structure of type "timeval",
**						which has the following two members:
**
**				->tv_sec		the seconds to wait before timing out
**				->tv_usec		the micro-seconds to wait before timing out
**
**						Note that it is perfectly valid to pass in 
**						a pointer to this structure with each of the
**						members set to 0.  In this case, if a connection
**						is not pending at the time of the check, there
**						will be an immediate return with the error code
**						JS_CHAN_ERR_TIMEOUT.
**
** Output:
**		pHasConnect		ptr to the returned flag that indicates whether
**						or not a connection is pending:
**							TRUE  => a connection IS pending
**							FALSE => a connection IS NOT pending
**
**		pFailReason		ptr to returned reason for any failure
**						condition
**
** Returns:
**		isOkay			Boolean indicating whether or not the
**						function executed successfully
**
** Error Conditions:
**
**		Error Code						Meaning
**		----------						-------
**		JS_SUCCESS						Success
**		JS_CHAN_ERR_APP_NOT_SERVER		This application is not of type Server
**		JS_CHAN_ERR_INVALID_HANDLE		Invalid handle
**		JS_CHAN_ERR_INVALID_PTR			Invalid pointer
**		JS_CHAN_ERR_SELECT				Could not SELECT on the comm channel
**		JS_CHAN_ERR_TIMEOUT				The comm channel has timed out
**
** Comments:
**		NONE
*/
int jsChanHasConnect(
	HANDLE			hLog,		/* handle to associated log object			*/
	HANDLE			hChan,		/* handle to channel object					*/
	struct timeval*	pTimeOut,	/* ptr to the timeout to apply to the 		*/
								/* check before returning to the caller		*/
	int*			pHasConnect,/* ptr to connection pending flag			*/
 	int*			pFailReason)/* ptr to returned reason for any failure	*/
								/* condition								*/
{
	long					err;		/* the returned error code			*/
	fd_set					maskRead;	/* the SELECT read mask				*/
	fd_set					maskWrite;	/* the SELECT write mask			*/
	fd_set					maskExcept;	/* the SELECT exception mask		*/
	int						numReady;	/* number sockets ready for read	*/
	int						userAbort;	/* the user has aborted indicator	*/
	UTLCOMM_CHANLIST_DESC	theChanIds;	/* list of comm channel IDs			*/
	char*	func = "jsChanHasConnect";	/* ptr to name of function			*/

/*	Allow for tracing information on ENTRY.
	--------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Assume that there is no connection pending.
	------------------------------------------- */
	*pConnectPending = FALSE;

/*	Validate the input parameters.
	------------------------------ */
	err = comm_validate_connect_pending_parms(pChanId, pTimeOut);
	if (err != UTL_SUCCESS) {
		return(err);
	}

/*	Create an artificial "list" of comm channel IDs, where the "list"
	has just one entry and then use SELECT to determine if there is
	any pending connection.
	----------------------------------------------------------------- */
	theChanIds.theList[0]  = pChanId;	
	theChanIds.sizeList    = 1;	

	numReady = comm_read_select(MASK_SOCKET_LISTEN, &theChanIds,
								&maskRead, &maskWrite, &maskExcept, pTimeOut,
								NULL, NULL, NULL, &userAbort);

/*	There was a serious problem (like a broken pipe, etc.),
	so return the error condition to the caller (the caller
	is responsible for closing down the comm channel).
	------------------------------------------------------- */
	if (numReady < 0) {
		UtlErr_Write(UTLCOMM_ERR_SELECT, func, RIGHT_HERE);
		err = UTLCOMM_ERR_SELECT;
	} else if (numReady == 0) {

/*		There is no connection pending on the comm channel, or the 
		comm channel has timed out, so return this information to the
		caller after making sure the returned count is set to 0.
		-------------------------------------------------------------- */
		if (pTimeOut == NULL) {
			err = UTL_SUCCESS;
		} else {
			err = UTLCOMM_ERR_TIMEOUT;
		}
	} else {

/*		We appear to have a connection pending on the comm channel,
		so verify this fact before indicating this to the caller.
		----------------------------------------------------------- */
		if (FD_ISSET(pChanId->sockListen, &maskRead)) {
			*pConnectPending = TRUE;
		}
		err = UTL_SUCCESS;
	}

/*	Allow for tracing information on EXIT.
	-------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(err);
}

/*
** Name:
**		jsChanInitialize
**
** Purpose:
**		This function is used to initialize the communications layer.
**
** Calling Sequence:
**		err = jsChanInitialize();
**
** Input:
**		NONE
**
** Output:
**		NONE
**
** Returns:
**		err		Error status.  Possible values include:
**
**		Error Code						Meaning
**		----------						-------
**		UTL_SUCCESS						Success
**		UTLCOMM_ERR_INITIALIZE			Failed to initialize
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
long jsChanInitialize()
{
	WSADATA				wsadata;		/* winsock initialization data	*/
	WORD				wVer;			/* the winsock version			*/
	int					iRc;			/* return code					*/
	char*	func = "jsChanInitialize";/* ptr to name of function		*/

/*	Allow for tracing information on ENTRY.
	--------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Initialize the use of WINSOCK.DLL and show the returned version number.
	----------------------------------------------------------------------- */
	wVer = MAKEWORD(1,1);
	iRc = WSAStartup(wVer, &wsadata);
	if (iRc != 0) {
		UtlLog_Write(LOG_DEBUG, func, RIGHT_HERE,
			"ERROR: WSAStartup failed, iRc=%d\n", iRc);
		return(UTLCOMM_ERR_INITIALIZE);
	}

	if ( (LOBYTE(wsadata.wVersion) != 1) ||
		 (HIBYTE(wsadata.wVersion) != 1) ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Wrong WINSOCK.DLL version = %d, %d\n",
			LOBYTE(wsadata.wVersion), HIBYTE(wsadata.wVersion));
		return(UTLCOMM_ERR_INITIALIZE);
	} else {
		UtlLog_Write(LOG_DEBUG, func, RIGHT_HERE,
			"WSAStartup success! WINSOCK.DLL version = %d, %d\n",
			LOBYTE(wsadata.wVersion), HIBYTE(wsadata.wVersion));
	}

/*	Allow for tracing information on EXIT.
	-------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(UTL_SUCCESS);
}

/*
** Name:
**		jsChanHasInput
**
** Purpose:
**		This function is used to determine if any input is pending
**		for a specified list of comm channels.  If one of the comm
**		channels is a listening channel, the check is made for a
**		pending connection.  Otherwise, the check is made for a
**		pending read.  Note that a check is made to verify that each
**		comm channel is associated with an application that is acting
**		as a server (JS_APP_TYPE_SERVER).
**
** Calling Sequence:
**		err = jsChanHasInput(hLog, phChanList, szChanList, pTimeout,
**							 pHasInput, phReadyChan, pFailReason);
**
** Input:
**		hLog			handle to associated log object
**		phChanList		ptr to list of channel objects
**		szChanList		the size of the list of channel objects
**		pTimeout		pointer to the timeout to apply to the check
**						for any pending input before returning to
**						the caller
**
**						Note that a NULL value for this input parameter
**						is valid and will cause the process to block
**						until a pending connection has been detected.
**						A non-NULL value means that the caller is supplying
**						a pointer to a timeout structure of type "timeval",
**						which has the following two members:
**
**				->tv_sec		the seconds to wait before timing out
**				->tv_usec		the micro-seconds to wait before timing out
**
**						Note that it is perfectly valid to pass in 
**						a pointer to this structure with each of the
**						members set to 0.  In this case, if there is no
**						input pending at the time of the check, there
**						will be an immediate return with the error code
**						JS_CHAN_ERR_TIMEOUT.
**
** Output:
**		pHasInput		ptr to the returned flag that indicates whether
**						or not any input is pending:
**							TRUE  => an input IS pending
**							FALSE => an input IS NOT pending
**
**		phReadyChan		ptr to returned handle of communication
**						channel that has input pending
**
**		pFailReason		ptr to returned reason for any failure
**						condition
**
** Error Conditions:
**
**		Error Code						Meaning
**		----------						-------
**		JS_SUCCESS						Success
**		JS_CHAN_ERR_APP_NOT_SERVER		This application is not of type Server
**		JS_CHAN_ERR_INVALID_HANDLE		Invalid handle
**		JS_CHAN_ERR_INVALID_PTR			Invalid pointer
**		JS_CHAN_ERR_SELECT				Could not SELECT on the comm channel
**		JS_CHAN_ERR_TIMEOUT				The comm channel has timed out
**
** Comments:
**		NONE
*/
int jsChanHasInput(
	HANDLE			hLog,		/* handle to associated log object			*/
	HANDLE*			phChanList,	/* ptr to list of channel objects			*/
	int				szChanList,	/* the size of the list of channel objects	*/
	struct timeval*	pTimeout,	/* ptr to the timeout to apply to the 		*/
								/* check before returning to the caller		*/
	int*			pHasInput,	/* ptr to returned input pending flag		*/
	HANDLE*			phReadyChan,/* ptr to returned handle of communication	*/
								/* channel that has input pending			*/
 	int*			pFailReason)/* ptr to returned reason for any failure	*/
								/* condition								*/
{
	long				err;			/* the returned error code			*/
	int					thisChanId;		/* this comm channel ID				*/
	fd_set				maskRead;		/* the SELECT read mask				*/
	fd_set				maskWrite;		/* the SELECT write mask			*/
	fd_set				maskExcept;		/* the SELECT exception mask		*/
	int					searchDone;		/* search is done flag				*/
	int					numReady;		/* number sockets ready for read	*/
	int					userAbort;		/* the user has aborted indicator	*/
	UTLCOMM_CHAN_DESC*	pChanId;		/* ptr to the comm channel ID		*/
	char*	func = "jsChanHasInput";			/* ptr to name of function	*/

/*	Allow for tracing information on ENTRY.
	--------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Assume that there is no input pending.
	-------------------------------------- */
	*pInputPending = FALSE;

/*	Validate the input parameters.
	------------------------------ */
	err = comm_validate_input_pending_parms(pListChanIds, pTimeOut, ppChanId);
	if (err != UTL_SUCCESS) {
		return(err);
	}

/*	Use SELECT to determine if there is any pending input for one
	of the comm channels.
	------------------------------------------------------------- */
	numReady = comm_read_select(MASK_SOCKET_LISTEN | MASK_SOCKET_IO,
								pListChanIds,
								&maskRead, &maskWrite, &maskExcept,
								pTimeOut, NULL, NULL, NULL, &userAbort);

/*	If the user has aborted the operation, return immediately with
	the appropriate return status.
	-------------------------------------------------------------- */
	if (userAbort) {
		return(UTL_USER_ABORT);
	}

/*	There was a serious problem (like a broken pipe, etc.),
	so return the error condition to the caller (the caller
	is responsible for closing down the comm channel).
	------------------------------------------------------- */
	if (numReady < 0) {
		UtlErr_Write(UTLCOMM_ERR_SELECT, func, RIGHT_HERE);
		return(UTLCOMM_ERR_SELECT);
	}

/*	There is nothing to read from the comm channel, or the comm
	channel has timed out, so return this information to the caller.
	---------------------------------------------------------------- */
	if (numReady == 0) {
		if (pTimeOut == NULL) {
			return(UTL_SUCCESS);
		} else {
			return(UTLCOMM_ERR_TIMEOUT);
		}
	}

/*	We have something to read from the comm channel, so try
	to determine which socket was selected.
	------------------------------------------------------- */

/*	First, check to see if any of the ready sockets happen to
	belong to a Master comm channel (in other words, a connection
	is now pending and needs to be serviced quickly).
	------------------------------------------------------------- */
	thisChanId = 0;
	searchDone = FALSE;
	while ( (!searchDone) && (thisChanId < pListChanIds->sizeList) ) {
		pChanId = pListChanIds->theList[thisChanId];
		if ( (pChanId->isMaster) &&
			 FD_ISSET(pChanId->sockListen, &maskRead) ) {
			searchDone = TRUE;
		} else {
			thisChanId++;
		}
	}

/*	If we did not find any Master comm channels that are ready
	to accept a pending connection, then let's check to see if
	there are any regular I/O sockets ready to read.
	---------------------------------------------------------- */
	if ( !searchDone) {
		thisChanId = 0;
		while ( (!searchDone) && (thisChanId < pListChanIds->sizeList) ) {
			pChanId = pListChanIds->theList[thisChanId];
			if ( (!pChanId->isMaster) &&
				 FD_ISSET(pChanId->sockIO, &maskRead) ) {
				searchDone = TRUE;
			} else {
				thisChanId++;
			}
		}
	}

/*	Make sure that we actually found a socket in the select list.
	------------------------------------------------------------- */
	if (!searchDone) {
		UtlErr_Write(UTLCOMM_ERR_SOCK_NOT_FOUND, func, RIGHT_HERE);
		return(UTLCOMM_ERR_SOCK_NOT_FOUND);
	}

/*	Show the caller that a comm channel was found with something
	to read on it.
	------------------------------------------------------------ */
	*pInputPending = TRUE;
	*ppChanId = pChanId;

/*	Allow for tracing information on EXIT.
	-------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(UTL_SUCCESS);
}

/*
** Name:
**		jsChanCreate
**
** Purpose:
**		This function is used to create a new communication channel
**		object.
**
** Calling Sequence:
**		isOkay = jsChanCreate(	hLog, int typeComm, int typeApp,
**								pChanDesc, pHeaderDesc, phChan,
**								pFailReason	);
**							int maxConnections, struct timeval* pConnTimeout,
**							char* pNameHost, char* pNameService,
**							JS_CHAN_HDR_DESC* pHeaderInfo,
**							UTLCOMM_CHAN_DESC** ppChanId	);
**
** Input:
**		typeComm		The type of communication associated with
**						the comm channel:
**
**						JS_COMM_TYPE_TCP	The type of protocol for the
**											comm channel is TCP/IP
**						JS_COMM_TYPE_UDP	The type of protocol for the
**											comm channel is UDP
**						JS_COMM_TYPE_SERIAL	The type of protocol for the
**											comm channel is SERIAL
**
**		typeApp			The type of application associated with
**						the comm channel:
**
**						JS_APP_TYPE_CLIENT	The type of application for
**											the comm channel is a CLIENT
**						JS_APP_TYPE_SERVER	The type of application for
**											the comm channel is a SERVER
**
**		pChanDesc		ptr to a channel descriptor that contains
**						detailed information describing how this
**						channel is to be created, which has the
**						following members:
** 
**			->maxConnections	The maximum number of concurrent connections
**								allowed on the channel (This is only relevant
**								if the "typeApp" is JS_APP_TYPE_SERVER)
**
**			->pConnTimeout		Pointer to the timeout to apply to the connect
**								before returning to the caller.  Note that
**								a NULL value for this input parameter is
**								valid and will cause the process to block
**								until the connect has completed.  A non-NULL
**								value means the caller is supplying a pointer
**								to a timeout structure of type "timeval",
**								which has the following two members:
**
**					->tv_sec		the seconds to wait before timing out
**					->tv_usec		the micro-seconds to wait before timing out
**
**								Note that it is perfectly valid to pass in 
**								a pointer to this structure with each of the
**								members set to 0.  In this case, if the connect
**								is not there at the time after the wait, there
**								will be an immediate return with the error code
**								JS_CHAN_ERR_CONNECT.
**
**			->pNameHost			Pointer to the name of the host associated
**								with the comm channel.  Note that if the
**								"typeApp" parameter is JS_APP_TYPE_SERVER then
**								this pointer can be set to the value NULL.
**
**			->pNameService		Pointer to the name of the service (in the 
**								"/etc/services" file) associated with the
**								comm channel.
**
**		pHeaderInfo		Pointer to a structure describing what the
**						header of a message looks like:
**
**			->sizeMsgHead	the size (in bytes) of the msg header
**			->offBodyLen	the offset (in bytes) to the body length
**			->sizeBodyLen	the size (in bytes) of the body length
**
** Output:
**		ppChanId		Pointer to the returned pointer to the comm 
**						channel ID that was opened.
**
** Returns:
**		err		Error status.  Possible values include:
**
**		Error Code						Meaning
**		----------						-------
**		UTL_SUCCESS						Success
**		UTLCOMM_ERR_BIND				Could not bind the socket
**		UTLCOMM_ERR_CONNECT				Could not connect to a server
**		UTLCOMM_ERR_GETSERVBYNAME		Could not get the service by name
**		UTLCOMM_ERR_GETSOCKNAME			Could not get the socket name
**		UTLCOMM_ERR_INVALID_APP_COMM	Invalid application or communication
**		UTLCOMM_ERR_INVALID_APP_TYPE	Invalid type of application
**		UTLCOMM_ERR_INVALID_COMM_TYPE	Invalid type of communication
**		UTLCOMM_ERR_INVALID_HDR_INFO	Invalid msg header information
**		UTLCOMM_ERR_INVALID_HOST		Invalid host name
**		UTLCOMM_ERR_INVALID_PTR			Invalid pointer
**		UTLCOMM_ERR_INVALID_SERVICE		Invalid service name
**		UTLCOMM_ERR_INVALID_TIMEOUT		Invalid timeout value
**		UTLCOMM_ERR_LISTEN				Could not listen on a socket
**		UTLCOMM_ERR_MAX_CONNECTIONS		Bad number of max connections
**		UTLCOMM_ERR_NOT_IMPLEMENTED		This feature is NOT YET IMPLEMENTED
**		UTLCOMM_ERR_OPEN_TCP_SOCKET		Could not open a TCP/IP socket
**		UTLCOMM_ERR_OUT_OF_MEMORY		Could not obtain enough memory
**		UTLCOMM_ERR_SETSOCKOPT			Could not set the socket options
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
long jsChanOpen(
	int					typeComm,		/* the type of communication 		*/
										/* associated with the comm channel	*/
	int					typeApp,		/* the type of application 			*/
										/* associated with the comm channel	*/
	int					maxConnections,	/* maximum number of connections	*/
										/* allowed on the comm channel		*/
	struct timeval*		pConnTimeout,	/* ptr to the timeout to apply to 	*/
										/* the connect before returning to 	*/
										/* the caller						*/
	char*				pNameHost,		/* ptr to the name of the host 		*/
										/* associated with the comm channel	*/
	char*				pNameService,	/* ptr to the name of the service	*/
										/* associated with the comm channel	*/
	UTLCOMM_HDR_DESC*	pHeaderInfo,	/* ptr to a structure describing	*/
										/* what the header of a message		*/
										/* looks like						*/
	UTLCOMM_CHAN_DESC**	ppChanId)		/* ptr to the returned ptr to the	*/
										/* comm channel ID that was opened	*/
{
	UTLCOMM_CHAN_DESC*	pNewChanId;		/* ptr to a new comm channel ID	*/
	long	err;						/* the returned error code		*/
	char*	func = "jsChanOpen";		/* ptr to name of the function	*/

/*	Allow for tracing information on ENTRY.
	--------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Validate the input parameters.
	------------------------------ */
	err = comm_validate_open_parms(	typeComm, typeApp,
									maxConnections, pConnTimeout,
									pNameHost, pNameService, pHeaderInfo,
								 	ppChanId	);
	if (err != UTL_SUCCESS) {
		return(err);
	}

/*	Allocate a new comm channel ID.
	------------------------------- */
	err = comm_create_chan(pNameHost, pNameService, pHeaderInfo,
						   &pNewChanId);
	if (err != UTL_SUCCESS) {
		comm_destroy_chan(pNewChanId);
		return(err);
	}

/*	Load the new comm channel ID with the input parameters.
	------------------------------------------------------- */
	err = comm_load_chan(pNewChanId, TRUE, typeComm, typeApp,
						 maxConnections, -1,
						 pNameHost, pNameService, pHeaderInfo);
	if (err != UTL_SUCCESS) {
		comm_destroy_chan(pNewChanId);
		return(err);
	}

/*	Open the comm channel that has been specified by the caller.
	------------------------------------------------------------ */
	err = comm_open_chan(pNewChanId, pConnTimeout);
	if (err != UTL_SUCCESS) {
		comm_destroy_chan(pNewChanId);
		return(err);
	}

/*	Allow for tracing information on EXIT.
	-------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

/*	Everything went well, so load the new channel ID into the
	caller's space and return a successful status.
	---------------------------------------------------------- */
	*ppChanId = pNewChanId;
	return(UTL_SUCCESS);
}

/*
** Name:
**		jsChanRead
**
** Purpose:
**		This function is used to READ from a comm channel.
**
** Calling Sequence:
**		err = jsChanRead(
**							UTLCOMM_CHAN_DESC*	pChanId,
**							int					sizeMsgMax, 
**							struct timeval*		pTimeOut,
**							int					(*pFuncCB)(),
**							void*				pDataCB,
**							struct timeval*		pTimeOutCB,
**							char*				pMsg,
**							int*				pSizeMsg		);
**
** Input:
**
**		pChanId			Pointer to the comm channel ID to read from.
**
**		sizeMsgMax		The maximum size of the message buffer (in
**						bytes) that has been allocated by the caller.
**
**		pTimeOut		Pointer to the timeout to apply to the read
**						before returning to the caller.  Note that
**						a NULL value for this input parameter is
**						valid and will cause the process to block
**						until the read has completed.  A non-NULL
**						value means the caller is supplying a pointer
**						to a timeout structure of type "timeval",
**						which has the following two members:
**
**			->tv_sec		the seconds to wait before timing out
**			->tv_usec		the micro-seconds to wait before timing out
**
**						Note that it is perfectly valid to pass in 
**						a pointer to this structure with each of the
**						members set to 0.  In this case, if the data
**						is not there at the time of the read, there
**						will be an immediate return with the error code
**						UTLCOMM_ERR_TIMEOUT.
**
**		pFuncCB			Pointer to an optional callback function that
**						will be invoked according to the value of
**						its associated timeout value, "pTimeOutCB".
**						If this pointer is NULL, then no callback
**						will be performed.  However, if this pointer
**						is non_NULL, it will be assumed to be a function
**						that is invoked with the "pDataCB" pointer
**						as its one and only argument.
**
**		pDataCB			Pointer to an optional structure that is always
**						passed as the one and only argument to "pFuncCB"
**						if the callback function is to be invoked.
**
**		pTimeOutCB		Pointer to an optional timeout to apply before 
**						calling the associated callback function specified 
**						in "pFuncCB".  Note that a NULL value for this
**						input parameter is ONLY valid if "pFuncCB" is
**						also NULL.  The "timeval" structure is the
**						same as for "pTimeOut".
**
** Output:
**
**		pMsg			Pointer to a message buffer that will contain
**						the bytes read from the comm channel.  Note
**						that this is a buffer that is allocated by the
**						caller BEFORE calling this read function.  Since
**						the caller does not know how big the message 
**						will be, this buffer should be allocated to be
**						the size of the largest EXPECTED message.  Of 
**						course, if a message was recieved that could not
**						be copied to this buffer (because the buffer was
**						not large enough), the caller would have the 
**						error code UTLCOMM_ERR_MSG_TOO_LARGE returned.
**
**		pSizeMsg		Pointer to the size of the message (in bytes)
**						actually read from the comm channel.  This
**						size information will be less than or equal to
**						"sizeMsgMax".
**
** Returns:
**		err		Error status.  Possible values include:
**
**		Error Code						Meaning
**		----------						-------
**		UTL_SUCCESS						Success
**		UTL_USER_ABORT					The user has aborted the operation
**		UTLCOMM_ERR_BODY_TOO_LARGE		Message body is too large
**		UTLCOMM_ERR_GET_BODY_SIZE		Could not get the message body size
**		UTLCOMM_ERR_INVALID_BODY_SIZE	Invalid body size for message
**		UTLCOMM_ERR_INVALID_PTR			Invalid pointer
**		UTLCOMM_ERR_MSG_TOO_LARGE		Message is too large
**		UTLCOMM_ERR_NOTHING_TO_READ		Nothing to read from the comm channel
**		UTLCOMM_ERR_NOT_IMPLEMENTED		This feature is NOT YET IMPLEMENTED
**		UTLCOMM_ERR_OUT_OF_MEMORY		Could not obtain enough memory
**		UTLCOMM_ERR_READING_BODY		Could not read body from comm channel
**		UTLCOMM_ERR_READING_HDR			Could not read header from comm channel
**		UTLCOMM_ERR_RECV				Could not RECV on the comm channel
**		UTLCOMM_ERR_SELECT				Could not SELECT on the comm channel
**		UTLCOMM_ERR_TIMEOUT				The comm channel has timed out
**		UTLCOMM_ERR_INVALID_CB_TIMEOUT	The callback timeout is invalid 
**		UTLCOMM_ERR_INVALID_SOCK_IO		The comm channel I/O socket is invalid 
**		UTLCOMM_ERR_REMOTE_CHAN_CLOSED	The remote comm channel has closed
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
long jsChanRead(
	UTLCOMM_CHAN_DESC*	pChanId,		/* ptr to the comm channel ID to 	*/
										/* read from 						*/
	int					sizeMsgMax,		/* the maximum size of the message 	*/
										/* buffer (in bytes) that has been	*/
										/* allocated by the caller			*/
	struct timeval*		pTimeOut,		/* ptr to the timeout to apply to 	*/
										/* the read before returning to the	*/
										/* caller							*/
	int					(*pFuncCB)(),	/* ptr to an optional callback func	*/
										/* to invoke for the caller			*/
	void*				pDataCB,		/* ptr to an optional chunk of data	*/
										/* that is passed to the callback	*/
										/* func as its only argument		*/
	struct timeval*		pTimeOutCB,		/* ptr to an optional timeout to	*/
										/* apply before calling the	assoc	*/
										/* callback function				*/
	char*				pMsg,			/* ptr to a message buffer that		*/
										/* will contain the bytes that were	*/
										/* read from the comm channel		*/
	int*				pSizeMsg)		/* ptr to the size of the message	*/
										/* (in bytes) actually read from 	*/
										/* the comm channel					*/
{
	int		err = UTL_SUCCESS;		/* the returned error code		*/
	char*	func = "jsChanRead";	/* ptr to name of the function	*/

/*	Allow for tracing information on ENTRY.
	--------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Validate the input parameters.
	------------------------------ */
	err = comm_validate_read_parms(	pChanId, sizeMsgMax, pTimeOut,
									pFuncCB, pDataCB, pTimeOutCB,
									pMsg, pSizeMsg					);
	if (err != UTL_SUCCESS) {
		return(err);
	}

/*	Read from the comm channel that has been specified by the caller.
	----------------------------------------------------------------- */
	err = comm_read_chan(	pChanId, sizeMsgMax, pTimeOut,
							pFuncCB, pDataCB, pTimeOutCB,
							pMsg, pSizeMsg					);
	if (err != UTL_SUCCESS) {
		return(err);
	}

/*	Allow for tracing information on EXIT.
	-------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

/*	Everything went well, so return a successful status.
	---------------------------------------------------- */
	return(UTL_SUCCESS);
}

/*
** Name:
**		jsChanTerminate
**
** Purpose:
**		This function is used to terminate the communications layer.
**
** Calling Sequence:
**		err = jsChanTerminate();
**
** Input:
**		NONE
**
** Output:
**		NONE
**
** Returns:
**		err		Error status.  Possible values include:
**
**		Error Code						Meaning
**		----------						-------
**		UTL_SUCCESS						Success
**		UTLCOMM_ERR_TERMINATE			Failed to terminate
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
long jsChanTerminate()
{
	char*	func = "jsChanTerminate"	;/* ptr to name of function	*/

/*	Allow for tracing information on ENTRY.
	--------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Terminate the use of WINSOCK.DLL.
	--------------------------------- */
	WSACleanup();

/*	Allow for tracing information on EXIT.
	-------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(UTL_SUCCESS);
}

/*
** Name:
**		jsChanWrite
**
** Purpose:
**		This function is used to WRITE to a comm channel.
**
** Calling Sequence:
**		err = jsChanWrite(
**							UTLCOMM_CHAN_DESC*	pChanId,
**							int					sizeMsgTotal, 
**							struct timeval*		pTimeOut,
**							int					(*pFuncCB)(),
**							void*				pDataCB,
**							struct timeval*		pTimeOutCB,
**							char*				pMsg,
**							int*				pSizeMsg		);
**
** Input:
**
**		pChanId			Pointer to the comm channel ID to write to.
**
**		sizeMsgTotal	The total size of the message buffer (in
**						bytes) to be written to the comm channel.
**
**		pTimeOut		Pointer to the timeout to apply to the write
**						before returning to the caller.  Note that
**						a NULL value for this input parameter is
**						valid and will cause the process to return
**						immediately if the write is NOT completed.  A 
**						non-NULL value means the caller is supplying a 
**						pointer to a timeout structure of type "timeval",
**						which has the following two members:
**
**			->tv_sec		the seconds to wait before timing out
**			->tv_usec		the micro-seconds to wait before timing out
**
**						Note that it is perfectly valid to pass in 
**						a pointer to this structure with each of the
**						members set to 0.  In this case, if the data
**						is not written completely at the time of the
**						write, there will be an immediatee return with
**						the error code UTLCOMM_ERR_TIMEOUT.
**
**		pFuncCB			Pointer to an optional callback function that
**						will be invoked according to the value of
**						its associated timeout value, "pTimeOutCB".
**						If this pointer is NULL, then no callback
**						will be performed.  However, if this pointer
**						is non_NULL, it will be assumed to be a function
**						that is invoked with the "pDataCB" pointer
**						as its one and only argument.
**
**		pDataCB			Pointer to an optional structure that is always
**						passed as the one and only argument to "pFuncCB"
**						if the callback function is to be invoked.
**
**		pTimeOutCB		Pointer to an optional timeout to apply before 
**						calling the associated callback function specified 
**						in "pFuncCB".  Note that a NULL value for this
**						input parameter is ONLY valid if "pFuncCB" is
**						also NULL.  The "timeval" structure is the
**						same as for "pTimeOut".
**
**		pMsg			Pointer to a message buffer to be written
**						to the comm channel.
**
** Output:
**
**		pSizeMsg		Pointer to the size of the message (in bytes)
**						actually written to the comm channel.  This
**						size will be equal to or less than the value
**						of "sizeMsgTotal".
**
** Returns:
**		err		Error status.  Possible values include:
**
**		Error Code						Meaning
**		----------						-------
**		UTL_SUCCESS						Success
**		UTLCOMM_ERR_INVALID_MSG_SIZE	Invalid size of message
**		UTLCOMM_ERR_INVALID_PTR			Invalid pointer
**		UTLCOMM_ERR_SEND				Could not SEND on the comm channel
**		UTLCOMM_ERR_TIMEOUT				The comm channel has timed out
**		UTLCOMM_ERR_WRITE_MISMATCH		Write mismatch on the comm channel
**		UTLCOMM_ERR_INVALID_CB_TIMEOUT	The callback timeout is invalid 
**		UTLCOMM_ERR_INVALID_SOCK_IO		The comm channel I/O socket is invalid 
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
long jsChanWrite(
	UTLCOMM_CHAN_DESC*	pChanId,		/* ptr to the comm channel ID to be	*/
										/* written to						*/
	int					sizeMsgTotal,	/* the total size of the message 	*/
										/* buffer (in bytes) to be written	*/
										/* to the comm channel				*/
	struct timeval*		pTimeOut,		/* ptr to the timeout to apply to 	*/
										/* the write before returning to 	*/
										/* the caller						*/
	int					(*pFuncCB)(),	/* ptr to an optional callback func	*/
										/* to invoke for the caller			*/
	void*				pDataCB,		/* ptr to an optional chunk of data	*/
										/* that is passed to the callback	*/
										/* func as its only argument		*/
	struct timeval*		pTimeOutCB,		/* ptr to an optional timeout to	*/
										/* apply before calling the	assoc	*/
										/* callback function				*/
	char*				pMsg,			/* ptr to a message buffer to be	*/
										/* written to the comm channel		*/
	int*				pSizeMsg)		/* ptr to the size of the message	*/
										/* (in bytes) actually written to	*/
										/* the comm channel					*/
{
	int		err = UTL_SUCCESS;			/* the returned error code		*/
	char*	func = "jsChanWrite";	/* ptr to name of the function	*/

/*	Allow for tracing information on ENTRY.
	--------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Validate the input parameters.
	------------------------------ */
	err = comm_validate_write_parms(
							pChanId, sizeMsgTotal, pTimeOut,
							pFuncCB, pDataCB, pTimeOutCB,
						    pMsg, pSizeMsg					);
	if (err != UTL_SUCCESS) {
		return(err);
	}

/*	Write to the comm channel that has been specified by the caller.
	---------------------------------------------------------------- */
	err = comm_write_chan(	pChanId, sizeMsgTotal, pTimeOut,
							pFuncCB, pDataCB, pTimeOutCB,
							pMsg, pSizeMsg);
	if (err != UTL_SUCCESS) {
		return(err);
	}

/*	Allow for tracing information on EXIT.
	-------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

/*	Everything went well, so return a successful status.
	---------------------------------------------------- */
	return(UTL_SUCCESS);
}

/*
** Name:
**			comm_calc_max_timeouts
**
** Purpose:
**			This local function calculates the maximum number of
**			callback timeouts that should be invoked to add up to
**			a user-specified maximum timeout value for a select
**			call.  This function will use both the seconds and
**			the microseconds of the timeout structure.  Both timeout
**			values will be converted to a total number of microseconds
**			before the appropriate division is done to calculate the
**			maximum number of callback timeouts.
*/
static int comm_calc_max_timeouts(
	struct timeval*		pTimeOut,		/* ptr to timeout information	*/
	struct timeval*		pTimeOutCB,		/* ptr to callback timeout info	*/
	int*				pMaxTimeOutsCB)	/* ptr to max callback timeouts	*/
{
	int			numUsecs;		/* number of usecs in timeout			*/
	int			numUsecsCB;		/* number of usecs in callback timeout	*/
	char*	func = "comm_calc_max_timeouts";	/* ptr to name of func	*/

/*	Allow for tracing information on ENTRY.
	--------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Determine the maximum number of callback timeouts by first converting
	both timeout structures to a total number of microseconds.
	--------------------------------------------------------------------- */
	numUsecs   = (pTimeOut->tv_sec   * USECS_PER_SEC) + pTimeOut->tv_usec;
	numUsecsCB = (pTimeOutCB->tv_sec * USECS_PER_SEC) + pTimeOutCB->tv_usec;

/*	Now calculate the maximum number of callback timeouts.
	------------------------------------------------------ */
	*pMaxTimeOutsCB = 0;
	if (numUsecsCB > 0) {
		*pMaxTimeOutsCB = numUsecs / numUsecsCB;
	} 

/*	Allow for tracing information on EXIT.
	-------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

/*	If we have some positive value for the maximum number of callback
	timeouts, then return a good value (TRUE).  Otherwise, return a
	bad value (FALSE).
	----------------------------------------------------------------- */
	if (*pMaxTimeOutsCB > 0) {
		return(TRUE);
	} else {
		return(FALSE);
	}
}

/*
** Name:
**			comm_create_chan
**
** Purpose:
**			This local function allocates the memory that is 
**			required for creating a new comm channel ID.
*/
static long comm_create_chan(
	char*				pNameHost,		/* ptr to name of the host		*/
	char*				pNameService,	/* ptr to name of the service	*/
	UTLCOMM_HDR_DESC*	pHeaderInfo,	/* ptr to the msg header		*/
	UTLCOMM_CHAN_DESC**	ppChanId)		/* ptr to ptr to the new chan 	*/
{
	char*		func = "comm_create_chan";		/* ptr to name of func	*/

/*	Allow for tracing information on ENTRY.
	--------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Allocate the memory for the basic comm channel ID structure.
	------------------------------------------------------------ */
	*ppChanId = (UTLCOMM_CHAN_DESC*)calloc(1, sizeof(UTLCOMM_CHAN_DESC));
	if ( *ppChanId == NULL ) {
		UtlErr_Write(UTLCOMM_ERR_OUT_OF_MEMORY, func, RIGHT_HERE);
		return(UTLCOMM_ERR_OUT_OF_MEMORY);
	}

/*	Allocate the memory for the service name and attach it to the
	newly created comm channel ID.
	-------------------------------------------------------------- */
	if (pNameService != NULL) {
		(*ppChanId)->pNameService =
				(char*)calloc(1, strlen(pNameService)+1);
		if ( (*ppChanId)->pNameService == NULL ) {
			UtlErr_Write(UTLCOMM_ERR_OUT_OF_MEMORY, func, RIGHT_HERE);
			return(UTLCOMM_ERR_OUT_OF_MEMORY);
		}
	}

/*	Allocate the memory for the host name and attach it to the
	newly created comm channel ID.
	------------------------------------------------------------- */
	if (pNameHost != NULL) {
		(*ppChanId)->pNameHost =
				(char*)calloc(1, strlen(pNameHost)+1);
		if ( (*ppChanId)->pNameHost == NULL ) {
			UtlErr_Write(UTLCOMM_ERR_OUT_OF_MEMORY, func, RIGHT_HERE);
			return(UTLCOMM_ERR_OUT_OF_MEMORY);
		}
	}

/*	Allocate the memory for the message header info and attach it to
	the newly created comm channel ID, if a host name is to be saved.
	----------------------------------------------------------------- */
	if (pHeaderInfo != NULL) {
		(*ppChanId)->pHeaderInfo =
				(UTLCOMM_HDR_DESC*)calloc(1, sizeof(UTLCOMM_HDR_DESC));
		if ( (*ppChanId)->pHeaderInfo == NULL ) {
			UtlErr_Write(UTLCOMM_ERR_OUT_OF_MEMORY, func, RIGHT_HERE);
			return(UTLCOMM_ERR_OUT_OF_MEMORY);
		}
	}

/*	Allow for tracing information on EXIT.
	-------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

/*	Return a successful status to the caller.
	----------------------------------------- */
	return(UTL_SUCCESS);
}

/*
** Name:
**			comm_destroy_chan
**
** Purpose:
**			This local function deallocates the memory that was
**			required for creating a new comm channel ID.
*/
static long comm_destroy_chan(
	UTLCOMM_CHAN_DESC*	pChanId)	/* ptr to the comm channel ID */
{
	char*		func = "comm_destroy_chan";		/* ptr to name of func	*/

/*	Allow for tracing information on ENTRY.
	--------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Make sure there is a valid pointer to the comm channel ID.
	--------------------------------------------------------- */
	if (pChanId == NULL) {
		UtlErr_Write(UTLCOMM_ERR_INVALID_PTR, func, RIGHT_HERE);
		return(UTLCOMM_ERR_INVALID_PTR);
	}

/*	Deallocate the memory for the host name, service name and
	the message header information.
	--------------------------------------------------------- */
	if (pChanId->pNameHost) {
		free(pChanId->pNameHost);
	}
	if (pChanId->pNameService) {
		free(pChanId->pNameService);
	}
	if (pChanId->pHeaderInfo) {
		free(pChanId->pHeaderInfo);
	}

/*	Close the sockets that were created for this channel.
	----------------------------------------------------- */
	if (pChanId->sockIO > 0) {
		closesocket(pChanId->sockIO);
	}
	if (pChanId->isMaster) {
		if (pChanId->sockListen > 0) {
			closesocket(pChanId->sockListen);
		}
	}

/*	Deallocate the memory for the basic comm channel ID structure.
	-------------------------------------------------------------- */
	free(pChanId);

/*	Allow for tracing information on EXIT.
	-------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

/*	Return a successful status to the caller.
	----------------------------------------- */
	return(UTL_SUCCESS);
}

/*
** Name:
**			comm_get_body_size
**
** Purpose:
**			This local function gets the size of a message
**			body out of the header for the message, using
**			the descriptor information found in the comm 
**			channel ID descriptor.
*/
static long comm_get_body_size(
	UTLCOMM_HDR_DESC*	pHeaderInfo,	/* ptr to msg header info	*/
	char*				pMsgHead)		/* ptr to msg header buffer	*/
{
	char		sizeBodyChar;				/* size of the body as a CHAR	*/
	short		sizeBodyShort;				/* size of the body as a SHORT	*/
	long		sizeBodyLong;				/* size of the body as a LONG	*/
	char*		func = "comm_get_body_size";		/* ptr to name of func	*/

/*	Allow for tracing information on ENTRY.
	--------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Extract out the message body size from the message header.
	---------------------------------------------------------- */
	switch (pHeaderInfo->sizeBodyLen) {
		case BODY_SIZE_CHAR:
			memcpy(&sizeBodyChar,  (pMsgHead + pHeaderInfo->offBodyLen),
				   pHeaderInfo->sizeBodyLen);
			return((long)sizeBodyChar);
			break;
		case BODY_SIZE_SHORT:
			memcpy(&sizeBodyShort, (pMsgHead + pHeaderInfo->offBodyLen),
				   pHeaderInfo->sizeBodyLen);
			if (!pHeaderInfo->useHostOrder) {
				sizeBodyShort = ntohs(sizeBodyShort);
			}
			return((long)(sizeBodyShort));
			break;
		case BODY_SIZE_LONG:
			memcpy(&sizeBodyLong, (pMsgHead + pHeaderInfo->offBodyLen),
				   pHeaderInfo->sizeBodyLen);
			if (!pHeaderInfo->useHostOrder) {
				sizeBodyLong = ntohl(sizeBodyLong);
			}
			return(sizeBodyLong);
			break;
		default:
			UtlErr_Write(UTLCOMM_ERR_INVALID_BODY_SIZE, func, RIGHT_HERE);
			return(UTLCOMM_ERR_INVALID_BODY_SIZE);
			break;
	}

/*	Allow for tracing information on EXIT.
	-------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
}

/*
** Name:
**			comm_load_chan
**
** Purpose:
**			This local function loads the validated input parameters
**			for the "jsChanAccept" and "jsChanOpen" functions into
**			a newly created comm channel ID.
*/
static long comm_load_chan(
	UTLCOMM_CHAN_DESC*	pChanId,		/* ptr to a comm channel ID		*/
	int					isMaster,		/* comm channel is a MASTER		*/
	int 				typeComm,		/* type of comm (UDP/TCP)		*/
	int 				typeApp,		/* type of app (CLIENT/SERVER)	*/
	int 				maxConnections,	/* max number of connections	*/
	int					sockListen,		/* the listening socket			*/
	char*				pNameHost,		/* ptr to name of the host		*/
	char*				pNameService,	/* ptr to name of the service	*/
	UTLCOMM_HDR_DESC*	pHeaderInfo)	/* ptr to the msg header info	*/
{
	char*		func = "comm_load_chan";		/* ptr to name of func	*/

/*	Allow for tracing information on ENTRY.
	--------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Load the communication and the application type information.
	------------------------------------------------------------ */
	pChanId->typeComm = typeComm;
	pChanId->typeApp  = typeApp;

/*	Load the maximum number of connections allowed.
	----------------------------------------------- */
	pChanId->maxConnections = maxConnections;

/*	Load the listening socket information.
	-------------------------------------- */
	pChanId->sockListen = sockListen;

/*	Load whether or not this is a MASTER comm channel.
	-------------------------------------------------- */
	pChanId->isMaster = isMaster;

/*	Load the host name and service name information.
	------------------------------------------------ */
	if (pNameHost    != NULL) strcpy(pChanId->pNameHost,    pNameHost);
	if (pNameService != NULL) strcpy(pChanId->pNameService, pNameService);

/*	Load the message header information.
	------------------------------------ */
	*(pChanId->pHeaderInfo) = *pHeaderInfo;

/*	Make sure the I/O socket is initialized to a "not created yet" state.
	--------------------------------------------------------------------- */
	pChanId->sockIO = -1;

/*	Allow for tracing information on EXIT.
	-------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

/*	Everything loaded out just fine, so indicated a success
	to the caller.
	------------------------------------------------------- */
	return(UTL_SUCCESS);
}

/*
** Name:
**			comm_accept_chan
**
** Purpose:
**			This local function accepts a comm channel as either
**			a CLIENT or a SERVER application in support of the
**			"jsChanAccept" function.
*/
static long comm_accept_chan(
	UTLCOMM_CHAN_DESC*	pChanId)	/* ptr to the comm channel ID */
{
	char*		func = "comm_accept_chan";		/* ptr to name of func	*/

/*	Allow for tracing information on ENTRY.
	--------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Look at the application type and communication type to decide what
	kind of open needs to be done for this invocation of "jsChanAccept".
	---------------------------------------------------------------------- */
	switch (pChanId->typeApp | pChanId->typeComm) {

/*		This is a CLIENT application using UDP.
		--------------------------------------- */
		case (CLIENT_APP | UDP_COMM):
			return(comm_accept_chan_client_udp(pChanId));
			break;

/*		This is a CLIENT application using TCP/IP.
		------------------------------------------ */
		case (CLIENT_APP | TCP_COMM):
			return(comm_accept_chan_client_tcp(pChanId));
			break;

/*		This is a SERVER application using UDP.
		--------------------------------------- */
		case (SERVER_APP | UDP_COMM):
			return(comm_accept_chan_server_udp(pChanId));
			break;

/*		This is a SERVER application using TCP/IP.
		------------------------------------------ */
		case (SERVER_APP | TCP_COMM):
			return(comm_accept_chan_server_tcp(pChanId));
			break;

/*		This is an UNKNOWN application type and / or communication type.
		---------------------------------------------------------------- */
		default:
			UtlErr_Write(UTLCOMM_ERR_INVALID_APP_COMM, func, RIGHT_HERE);
			return(UTLCOMM_ERR_INVALID_APP_COMM);
			break;
	}	

/*	Allow for tracing information on EXIT.
	-------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

/*	Return a successful status to the caller.
	----------------------------------------- */
	return(UTL_SUCCESS);
}

/*
** Name:
**			comm_accept_chan_client_udp
**
** Purpose:
**			This local function accepts a comm channel as a
**			CLIENT application using UDP as the communication
**			protocol in support of the "jsChanAccept" function.
*/
static long comm_accept_chan_client_udp(
	UTLCOMM_CHAN_DESC*	pChanId)	/* ptr to the comm channel ID */
{
	char*	func = "comm_accept_chan_client_udp";	/* ptr to name of func	*/

/*	Allow for tracing information on ENTRY.
	--------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Allow for tracing information on EXIT.
	-------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

/*	Do not allow an ACCEPT to be done for a CLIENT_APP.
	--------------------------------------------------- */
	UtlErr_Write(UTLCOMM_ERR_NOT_ALLOWED, func, RIGHT_HERE);
	return(UTLCOMM_ERR_NOT_ALLOWED);
}

/*
** Name:
**			comm_accept_chan_client_tcp
**
** Purpose:
**			This local function accepts a comm channel as a
**			CLIENT application using TCP/IP as the communication
**			protocol in support of the "jsChanAccept" function.
*/
static long comm_accept_chan_client_tcp(
	UTLCOMM_CHAN_DESC*	pChanId)	/* ptr to the comm channel ID */
{
	char*	func = "comm_accept_chan_client_tcp";	/* ptr to name of func	*/

/*	Allow for tracing information on ENTRY.
	--------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Allow for tracing information on EXIT.
	-------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

/*	Do not allow an ACCEPT to be done for a CLIENT_APP.
	--------------------------------------------------- */
	UtlErr_Write(UTLCOMM_ERR_NOT_ALLOWED, func, RIGHT_HERE);
	return(UTLCOMM_ERR_NOT_ALLOWED);
}

/*
** Name:
**			comm_accept_chan_server_udp
**
** Purpose:
**			This local function accepts a comm channel as a
**			SERVER application using UDP as the communication
**			protocol in support of the "jsChanAccept" function.
*/
static long comm_accept_chan_server_udp(
	UTLCOMM_CHAN_DESC*	pChanId)	/* ptr to the comm channel ID */
{
	char*	func = "comm_accept_chan_server_udp";	/* ptr to name of func	*/

/*	Allow for tracing information on ENTRY.
	--------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Allow for tracing information on EXIT.
	-------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

/*	Look at the communication type to decide what kind of open
	needs to be done for this invocation of "jsChanOpen".
	-------------------------------------------------------- */
	UtlErr_Write(UTLCOMM_ERR_NOT_IMPLEMENTED, func, RIGHT_HERE);
	return(UTLCOMM_ERR_NOT_IMPLEMENTED);

/*	Return a successful status to the caller.
	return(UTL_SUCCESS);
	----------------------------------------- */
}

/*
** Name:
**			comm_accept_chan_server_tcp
**
** Purpose:
**			This local function accepts a comm channel as a
**			SERVER application using TCP/IP as the communication
**			protocol in support of the "jsChanAccept" function.
*/
static long comm_accept_chan_server_tcp(
	UTLCOMM_CHAN_DESC*	pChanId)	/* ptr to the comm channel ID */
{
	int					gotAccept;			/* got a good accept flag		*/
	char*	func = "comm_accept_chan_server_tcp";	/* ptr to name of func	*/

/*	Allow for tracing information on ENTRY.
	--------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Wait to accept a request from some client process.
	-------------------------------------------------- */
	gotAccept = FALSE;
	while ( !gotAccept ) {
		pChanId->sockIO = accept(pChanId->sockListen, 0, 0);
		if (pChanId->sockIO == -1) {
			if (WSAGetLastError() == WSAEINTR) goto _errSystemInterrupt;
			UtlErr_Write(UTLCOMM_ERR_ACCEPT, func, RIGHT_HERE);
			return(UTLCOMM_ERR_ACCEPT);
		} else {
			gotAccept = TRUE;
		}

/*		System interrupts should NOT affect our wait for the accept...
		-------------------------------------------------------------- */
_errSystemInterrupt:
		;
	}

/*	Allow for tracing information on EXIT.
	-------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

/*	Return a successful status to the caller.
	----------------------------------------- */
	return(UTL_SUCCESS);
}

/*
** Name:
**			comm_open_chan
**
** Purpose:
**			This local function opens a comm channel as either
**			a CLIENT or a SERVER application in support of the
**			"jsChanOpen" function.
*/
static long comm_open_chan(
	UTLCOMM_CHAN_DESC*	pChanId,		/* ptr to the comm channel ID	*/
	struct timeval*		pConnTimeout)	/* ptr to connection timeout	*/
{
	char*		func = "comm_open_chan";		/* ptr to name of func	*/

/*	Allow for tracing information on ENTRY.
	--------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Look at the application type and communication type to decide what
	kind of open needs to be done for this invocation of "jsChanOpen".
	-------------------------------------------------------------------- */
	switch (pChanId->typeApp | pChanId->typeComm) {

/*		This is a CLIENT application using UDP.
		--------------------------------------- */
		case (CLIENT_APP | UDP_COMM):
			return(comm_open_chan_client_udp(pChanId, pConnTimeout));
			break;

/*		This is a CLIENT application using TCP/IP.
		------------------------------------------ */
		case (CLIENT_APP | TCP_COMM):
			return(comm_open_chan_client_tcp(pChanId, pConnTimeout));
			break;

/*		This is a SERVER application using UDP.
		--------------------------------------- */
		case (SERVER_APP | UDP_COMM):
			return(comm_open_chan_server_udp(pChanId));
			break;

/*		This is a SERVER application using TCP/IP.
		------------------------------------------ */
		case (SERVER_APP | TCP_COMM):
			return(comm_open_chan_server_tcp(pChanId));
			break;

/*		This is an UNKNOWN application type and / or communication type.
		---------------------------------------------------------------- */
		default:
			UtlErr_Write(UTLCOMM_ERR_INVALID_APP_COMM, func, RIGHT_HERE);
			return(UTLCOMM_ERR_INVALID_APP_COMM);
			break;
	}	

/*	Allow for tracing information on EXIT.
	-------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

/*	Return a successful status to the caller.
	----------------------------------------- */
	return(UTL_SUCCESS);
}

/*
** Name:
**			comm_open_chan_client_udp
**
** Purpose:
**			This local function opens a comm channel as a
**			CLIENT application using UDP as the communication
**			protocol in support of the "jsChanOpen" function.
*/
static long comm_open_chan_client_udp(
	UTLCOMM_CHAN_DESC*	pChanId,		/* ptr to the comm channel ID	*/
	struct timeval*		pConnTimeout)	/* ptr to connection timeout	*/
{
	char*	func = "comm_open_chan_client_udp";	/* ptr to name of func	*/

/*	Allow for tracing information on ENTRY.
	--------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Allow for tracing information on EXIT.
	-------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

/*	Look at the communication type to decide what kind of open
	needs to be done for this invocation of "jsChanOpen".
	-------------------------------------------------------- */
	UtlErr_Write(UTLCOMM_ERR_NOT_IMPLEMENTED, func, RIGHT_HERE);
	return(UTLCOMM_ERR_NOT_IMPLEMENTED);

/*	Return a successful status to the caller.
	return(UTL_SUCCESS);
	----------------------------------------- */
}

/*
** Name:
**			comm_open_chan_client_tcp
**
** Purpose:
**			This local function opens a comm channel as a
**			CLIENT application using TCP/IP as the communication
**			protocol in support of the "jsChanOpen" function.
*/
static long comm_open_chan_client_tcp(
	UTLCOMM_CHAN_DESC*	pChanId,		/* ptr to the comm channel ID	*/
	struct timeval*		pConnTimeout)	/* ptr to connection timeout	*/
{
	unsigned long		modeBlocking;	/* the socket blocking mode		*/
	struct hostent*		pHostEntry;		/* ptr to host entry			*/
	struct servent*		pServiceEntry;	/* ptr to service entry			*/
	char*	func = "comm_open_chan_client_tcp";	/* ptr to name of func	*/

/*	Allow for tracing information on ENTRY.
	--------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Open a TCP/IP socket for this CLIENT application.
	------------------------------------------------- */
	pChanId->sockIO = socket(AF_INET, SOCK_STREAM, 0);
	if (pChanId->sockIO < 0) {
		UtlErr_Write(UTLCOMM_ERR_OPEN_TCP_SOCKET, func, RIGHT_HERE);
		return(UTLCOMM_ERR_OPEN_TCP_SOCKET);
	}

/*	Save the address format in the comm channel ID.
	----------------------------------------------- */
	pChanId->addrSockClient.sin_family = AF_INET;

/*	Get the host information.
	------------------------- */
	pHostEntry = gethostbyname(pChanId->pNameHost);
	if (pHostEntry == NULL) {
		UtlErr_Write(UTLCOMM_ERR_GETHOSTBYNAME, func, RIGHT_HERE);
		return(UTLCOMM_ERR_GETHOSTBYNAME);
	}

/*	Get the service information.
	---------------------------- */
	pServiceEntry = getservbyname(pChanId->pNameService, "tcp");
	if (pServiceEntry == NULL) {
		UtlErr_Write(UTLCOMM_ERR_GETSERVBYNAME, func, RIGHT_HERE);
		return(UTLCOMM_ERR_GETSERVBYNAME);
	}

/*	Get the address in the INET address.
	------------------------------------ */
	memcpy( (char*)&(pChanId->addrSockClient.sin_addr),
			 pHostEntry->h_addr, pHostEntry->h_length );

/*	Bind to the port.
	----------------- */
	pChanId->addrSockClient.sin_port = htons(pServiceEntry->s_port);

/*	If this application has indicated it wants a timeout
	associated with the connection, temporarily set the socket
	to NON-BLOCKING, in case the end-point we are trying to
	connect to is not there at this time (machine down, etc.).
	---------------------------------------------------------- */
	if (pConnTimeout != NULL) {
		modeBlocking = BLOCKING_OFF;
		if (ioctlsocket(pChanId->sockIO, FIONBIO,
						&modeBlocking) < 0) {
			UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
				"ERROR: Could not set connect socket to NON-BLOCKING, err=%d\n",
					WSAGetLastError());
			return(UTLCOMM_ERR_CONNECT);
		}
	}

/*	Now, try to solicit the connect.
	-------------------------------- */
	if (connect(pChanId->sockIO,
				(struct sockaddr *)&pChanId->addrSockClient,
				sizeof(pChanId->addrSockClient)) < 0) {
		if (pConnTimeout != NULL) {
			if ( (WSAGetLastError() != WSAEINPROGRESS) &&
				 (WSAGetLastError() != WSAEWOULDBLOCK) ) {
				UtlLog_Write(LOG_DEBUG, func, RIGHT_HERE,
					"Note: This attempt to connect failed, errno=%d\n", WSAGetLastError());
				return(UTLCOMM_ERR_CONNECT);
			}
			UtlTime_Sleep(pConnTimeout->tv_sec, pConnTimeout->tv_usec);
			if (connect(pChanId->sockIO,
						(struct sockaddr *)&pChanId->addrSockClient,
						sizeof(pChanId->addrSockClient)) < 0) {
				if (WSAGetLastError() != WSAEISCONN) {
					UtlLog_Write(LOG_DEBUG, func, RIGHT_HERE,
						"Note: This attempt to connect failed, errno=%d\n",
							WSAGetLastError());
					return(UTLCOMM_ERR_CONNECT);
				}
			}
		} else {
			UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
				"ERROR: Could not connect to \"%s\" on \"%s\", err=%d\n",
					pChanId->pNameService, pChanId->pNameHost, WSAGetLastError());
			return(UTLCOMM_ERR_CONNECT);
		}
	}

/*	If necessary, set the socket back to its default mode of BLOCKING.
	------------------------------------------------------------------ */
	if (pConnTimeout != NULL) {
		modeBlocking = BLOCKING_ON;
		if (ioctlsocket(pChanId->sockIO, FIONBIO,
						&modeBlocking) < 0) {
			UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
				"ERROR: Could not reset connect socket to BLOCKING, err=%d\n",
					WSAGetLastError());
			return(UTLCOMM_ERR_CONNECT);
		}
	}

/*	Allow for tracing information on EXIT.
	-------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

/*	Return a successful status to the caller.
	----------------------------------------- */
	return(UTL_SUCCESS);
}

/*
** Name:
**			comm_open_chan_server_udp
**
** Purpose:
**			This local function opens a comm channel as a
**			SERVER application using UDP as the communication
**			protocol in support of the "jsChanOpen" function.
*/
static long comm_open_chan_server_udp(
	UTLCOMM_CHAN_DESC*	pChanId)	/* ptr to the comm channel ID */
{
	char*	func = "comm_open_chan_server_udp";	/* ptr to name of func	*/

/*	Allow for tracing information on ENTRY.
	--------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Allow for tracing information on EXIT.
	-------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

/*	Look at the communication type to decide what kind of open
	needs to be done for this invocation of "jsChanOpen".
	-------------------------------------------------------- */
	UtlErr_Write(UTLCOMM_ERR_NOT_IMPLEMENTED, func, RIGHT_HERE);
	return(UTLCOMM_ERR_NOT_IMPLEMENTED);

/*	Return a successful status to the caller.
	return(UTL_SUCCESS);
	----------------------------------------- */
}

/*
** Name:
**			comm_open_chan_server_tcp
**
** Purpose:
**			This local function opens a comm channel as a
**			SERVER application using TCP/IP as the communication
**			protocol in support of the "jsChanOpen" function.
*/
static long comm_open_chan_server_tcp(
	UTLCOMM_CHAN_DESC*	pChanId)	/* ptr to the comm channel ID */
{
	int					optSock;		/* the socket options			*/
	int					lenSockName;	/* length of the socket name	*/
	struct servent*		pServiceEntry;	/* ptr to service entry			*/
	char*	func = "comm_open_chan_server_tcp";	/* ptr to name of func	*/

/*	Allow for tracing information on ENTRY.
	--------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Open a TCP/IP listening socket for this SERVER application.
	----------------------------------------------------------- */
	pChanId->sockListen = socket(AF_INET, SOCK_STREAM, 0);
	if (pChanId->sockListen < 0) {
		UtlErr_Write(UTLCOMM_ERR_OPEN_TCP_SOCKET, func, RIGHT_HERE);
		return(UTLCOMM_ERR_OPEN_TCP_SOCKET);
	}

/*	Get the service information.
	---------------------------- */
	pServiceEntry = getservbyname(pChanId->pNameService, "tcp");
	if (pServiceEntry == NULL) {
		UtlErr_Write(UTLCOMM_ERR_GETSERVBYNAME, func, RIGHT_HERE);
		return(UTLCOMM_ERR_GETSERVBYNAME);
	}

/*	Bind the service information to this SERVER application.
	-------------------------------------------------------- */
	pChanId->addrSockServer.sin_family      = AF_INET;
	pChanId->addrSockServer.sin_addr.s_addr = INADDR_ANY;
	pChanId->addrSockServer.sin_port        = htons(pServiceEntry->s_port);

/*	Ensure that the upcoming bind will succeed.
	------------------------------------------- */
	optSock = 1;
	optSock = setsockopt(pChanId->sockListen, SOL_SOCKET,
						 SO_REUSEADDR, (const char *)&optSock,
						 sizeof(optSock));
	if (optSock == -1) {
		UtlErr_Write(UTLCOMM_ERR_SETSOCKOPT, func, RIGHT_HERE);
		return(UTLCOMM_ERR_SETSOCKOPT);
	}

/*	Bind the listening socket to this SERVER application.
	----------------------------------------------------- */
	if (bind(pChanId->sockListen, (struct sockaddr*)&pChanId->addrSockServer,
			 sizeof(pChanId->addrSockServer))) {
		UtlErr_Write(UTLCOMM_ERR_BIND, func, RIGHT_HERE);
		return(UTLCOMM_ERR_BIND);
	}

/*	Get the name of the listening socket for this SERVER application.
	----------------------------------------------------------------- */
	lenSockName = sizeof(pChanId->addrSockServer);
	if (getsockname(pChanId->sockListen,
					(struct sockaddr*)&pChanId->addrSockServer,
					&lenSockName)) {
		UtlErr_Write(UTLCOMM_ERR_GETSOCKNAME, func, RIGHT_HERE);
		return(UTLCOMM_ERR_GETSOCKNAME);
	}

/*	Setup to listen for a request from some client process.
	------------------------------------------------------- */
	if (listen(pChanId->sockListen, pChanId->maxConnections)) {
		UtlErr_Write(UTLCOMM_ERR_LISTEN, func, RIGHT_HERE);
		return(UTLCOMM_ERR_LISTEN);
	}

/*	Allow for tracing information on EXIT.
	-------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

/*	Return a successful status to the caller.
	----------------------------------------- */
	return(UTL_SUCCESS);
}

/*
** Name:
**			comm_read_chan
**
** Purpose:
**			This local function reads from a comm channel as either
**			a CLIENT or a SERVER application in support of the
**			"jsChanRead" function.
*/
static long comm_read_chan(
	UTLCOMM_CHAN_DESC*	pChanId,		/* ptr to the comm channel ID	*/
	int 				sizeMsgMax,		/* max size of message buffer	*/
	struct timeval*		pTimeOut,		/* ptr to timeout info			*/
	int					(*pFuncCB)(),	/* ptr to callback function		*/
	void*				pDataCB,		/* ptr to callback data			*/
	struct timeval*		pTimeOutCB,		/* ptr to callback timeout info	*/
	char*				pMsg,			/* ptr to returned msg buffer	*/
	int* 				pSizeMsg)		/* ptr to returned size of msg	*/
{
	char*		func = "comm_read_chan";		/* ptr to name of func	*/

/*	Allow for tracing information on ENTRY.
	--------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Look at the communication type to decide what kind of read needs
	to be done for this invocation of "jsChanRead".
	---------------------------------------------------------------- */
	switch (pChanId->typeComm) {

/*		This application is using UDP.
		------------------------------ */
		case (UDP_COMM):
			return(comm_read_chan_udp(	pChanId, sizeMsgMax, pTimeOut,
										pFuncCB, pDataCB, pTimeOutCB,
										pMsg, pSizeMsg));
			break;

/*		This application is using TCP/IP.
		--------------------------------- */
		case (TCP_COMM):
			return(comm_read_chan_tcp(	pChanId, sizeMsgMax, pTimeOut,
										pFuncCB, pDataCB, pTimeOutCB,
										pMsg, pSizeMsg));
			break;

/*		This is an UNKNOWN communication type.
		---------------------------------------------------------------- */
		default:
			UtlErr_Write(UTLCOMM_ERR_INVALID_APP_TYPE, func, RIGHT_HERE);
			return(UTLCOMM_ERR_INVALID_APP_TYPE);
			break;
	}	

/*	Allow for tracing information on EXIT.
	-------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

/*	Return a successful status to the caller.
	----------------------------------------- */
	return(UTL_SUCCESS);
}

/*
** Name:
**			comm_read_chan_udp
**
** Purpose:
**			This local function reads from a comm channel as 
**			an application using UDP as the communication
**			protocol in support of the "jsChanRead" function.
*/
static long comm_read_chan_udp(
	UTLCOMM_CHAN_DESC*	pChanId,		/* ptr to the comm channel ID	*/
	int 				sizeMsgMax,		/* max size of message buffer	*/
	struct timeval*		pTimeOut,		/* ptr to timeout information 	*/
	int					(*pFuncCB)(),	/* ptr to callback function		*/
	void*				pDataCB,		/* ptr to callback data			*/
	struct timeval*		pTimeOutCB,		/* ptr to callback timeout info	*/
	char*				pMsg,			/* ptr to returned msg buffer	*/
	int* 				pSizeMsg)		/* ptr to returned size of msg	*/
{
	char*	func = "comm_read_chan_udp";		/* ptr to name of func	*/

/*	Allow for tracing information on ENTRY.
	--------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Allow for tracing information on EXIT.
	-------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

/*	Read from the comm channel for this application using UDP.
	---------------------------------------------------------- */
	UtlErr_Write(UTLCOMM_ERR_NOT_IMPLEMENTED, func, RIGHT_HERE);
	return(UTLCOMM_ERR_NOT_IMPLEMENTED);

/*	Return a successful status to the caller.
	return(UTL_SUCCESS);
	----------------------------------------- */
}

/*
** Name:
**			comm_read_chan_tcp
**
** Purpose:
**			This local function reads from a comm channel as 
**			an application using TCP/IP as the communication
**			protocol in support of the "jsChanRead" function.
*/
static long comm_read_chan_tcp(
	UTLCOMM_CHAN_DESC*	pChanId,		/* ptr to the comm channel ID	*/
	int 				sizeMsgMax,		/* max size of message buffer	*/
	struct timeval*		pTimeOut,		/* ptr to timeout information 	*/
	int					(*pFuncCB)(),	/* ptr to callback function		*/
	void*				pDataCB,		/* ptr to callback data			*/
	struct timeval*		pTimeOutCB,		/* ptr to callback timeout info	*/
	char*				pMsg,			/* ptr to returned msg buffer	*/
	int* 				pSizeMsg)		/* ptr to returned size of msg	*/
{
	long					retStatus;		/* the return status			*/
	UTLCOMM_CHANLIST_DESC	theChanIds;		/* list of comm channel IDs		*/
	UTLCOMM_CHAN_DESC*		pReturnChanId;	/* ptr to returned comm chan ID	*/
	char*	func = "comm_read_chan_tcp";			/* ptr to name of func	*/

/*	Allow for tracing information on ENTRY.
	--------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Create an artificial "list" of comm channel IDs, where the "list"
	has just one entry and then call the "comm_read_block_chan_tcp"
	function to do the real work.
	----------------------------------------------------------------- */
	theChanIds.theList[0]  = pChanId;	
	theChanIds.sizeList    = 1;	

	retStatus = comm_read_block_chan_tcp(&theChanIds, sizeMsgMax, pTimeOut,
										 pFuncCB, pDataCB, pTimeOutCB,
										 pMsg, pSizeMsg, &pReturnChanId);

/*	Allow for tracing information on EXIT.
	-------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

/*	Return the status to the caller.
	-------------------------------- */
	return(retStatus);
}

/*
** Name:
**			comm_read_from_chan
**
** Purpose:
**			This local function reads a specified number of bytes
**			from a specified channel into a specified buffer that
**			has been allocated by the caller.
*/
static long comm_read_from_chan(
	UTLCOMM_CHAN_DESC*	pChanId,	/* ptr to the comm channel ID		*/
	int			sizeToRead,			/* the size (in bytes) to read		*/
	char*		pBuff)				/* ptr to the buffer to read into	*/
{
	int			optRecv;			/* the options on the RECV				*/
	int			numTries;			/* the number of attempts to try 		*/
	int			sizeReadThisTime;	/* the number of bytes read this time	*/
	int			sizeLeftToRead;		/* the number of bytes left to read		*/
	int			sizeReadSoFar;		/* the number of bytes read so far		*/
	char*		func = "comm_read_from_chan";	/* ptr to name of func		*/

/*	Allow for tracing information on ENTRY.
	--------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Get ready to read all the requested bytes from the pipe.
	-------------------------------------------------------- */
	optRecv        = 0;
	numTries       = 0;
	sizeReadSoFar  = 0;
	sizeLeftToRead = sizeToRead;

/*	Loop until we have gathered up all the bytes requested by the caller.
	--------------------------------------------------------------------- */
	while ( (sizeReadSoFar != sizeToRead) &&
			(numTries < MAX_RECV_TIMEOUTS) ) {
		sizeReadThisTime = recv(pChanId->sockIO, (pBuff + sizeReadSoFar),
								sizeLeftToRead, optRecv);
		if (sizeReadThisTime < 0) {
			if (WSAGetLastError() == WSAEINTR) goto _errSystemInterrupt;
			UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
				"Error from RECV, errno=%d\n", WSAGetLastError());
			UtlErr_Write(UTLCOMM_ERR_RECV, func, RIGHT_HERE);
			UtlLog_Write(LOG_DEBUG | LOG_NO_ID, func, RIGHT_HERE,
				"Expected number of bytes to read = %d\n", sizeToRead);
			if (sizeReadSoFar > 0) {
				UtlDump_Mem(pBuff, sizeReadSoFar, LOG_DEBUG);
			}
			return(UTLCOMM_ERR_RECV);
		} else {
			sizeLeftToRead -= sizeReadThisTime;
			sizeReadSoFar  += sizeReadThisTime;
			if (sizeReadSoFar != sizeToRead) {
				Sleep(1000);
			}
		}

/*		Bump the number of tries to get all the bytes.
		---------------------------------------------- */
		numTries++;

/*		System interrupts should NOT affect our number of tries...
		---------------------------------------------------------- */
_errSystemInterrupt:
		;
	}

/*	Check to see if we read the number of bytes that were expected.
	--------------------------------------------------------------- */
	if (sizeReadSoFar != sizeToRead) {
		if (sizeReadSoFar == 0) {
			UtlLog_Write(LOG_DEBUG, func, RIGHT_HERE,
				"Note: Got 0 bytes ==> Remote channel has closed.\n");
			return(UTLCOMM_ERR_REMOTE_CHAN_CLOSED);
		}
		UtlErr_Write(UTLCOMM_ERR_TOO_FEW_BYTES, func, RIGHT_HERE);
		UtlLog_Write(LOG_DEBUG, func, RIGHT_HERE,
			"Expected %d bytes, only got %d\n", sizeToRead, sizeReadSoFar);
		if (sizeReadSoFar > 0) {
			UtlDump_Mem(pBuff, sizeReadSoFar, LOG_DEBUG);
		}
		return(UTLCOMM_ERR_TOO_FEW_BYTES);
	}

/*	Allow for tracing information on EXIT.
	-------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

/*	Return a successful status to the caller.
	----------------------------------------- */
	return(UTL_SUCCESS);
}

/*
** Name:
**			comm_read_block_chan
**
** Purpose:
**			This local function reads from a block of comm channels as 
**			either a CLIENT or a SERVER application in support of the
**			"jsChanReadBlock" function.
*/
static long comm_read_block_chan(
	UTLCOMM_CHANLIST_DESC*	pListChanIds,	/* ptr to list of comm chan IDs	*/
	int 					sizeMsgMax,		/* max size of message buffer	*/
	struct timeval*			pTimeOut,		/* ptr to timeout information 	*/
	int						(*pFuncCB)(),	/* ptr to callback function		*/
	void*					pDataCB,		/* ptr to callback data			*/
	struct timeval*			pTimeOutCB,		/* ptr to callback timeout info	*/
	char*					pMsg,			/* ptr to returned msg buffer	*/
	int* 					pSizeMsg,		/* ptr to returned size of msg	*/
	UTLCOMM_CHAN_DESC**		ppChanId)		/* ptr to returned comm channel */
{
	char*		func = "comm_read_block_chan";	/* ptr to name of func	*/

/*	Allow for tracing information on ENTRY.
	--------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Look at the communication type to decide what kind of read needs
	to be done for this invocation of "jsChanReadBlock".
	---------------------------------------------------------------- */
	switch (pListChanIds->theList[0]->typeComm) {

/*		This application is using UDP.
		------------------------------ */
		case (UDP_COMM):
			return(comm_read_block_chan_udp(pListChanIds, sizeMsgMax, pTimeOut,
											pFuncCB, pDataCB, pTimeOutCB,
											pMsg, pSizeMsg, ppChanId));
			break;

/*		This application is using TCP/IP.
		--------------------------------- */
		case (TCP_COMM):
			return(comm_read_block_chan_tcp(pListChanIds, sizeMsgMax, pTimeOut,
											pFuncCB, pDataCB, pTimeOutCB,
											pMsg, pSizeMsg, ppChanId));
			break;

/*		This is an UNKNOWN communication type.
		---------------------------------------------------------------- */
		default:
			UtlErr_Write(UTLCOMM_ERR_INVALID_APP_TYPE, func, RIGHT_HERE);
			return(UTLCOMM_ERR_INVALID_APP_TYPE);
			break;
	}	

/*	Allow for tracing information on EXIT.
	-------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

/*	Return a successful status to the caller.
	----------------------------------------- */
	return(UTL_SUCCESS);
}

/*
** Name:
**			comm_read_block_chan_udp
**
** Purpose:
**			This local function reads from a block of comm channels
**			as an application using UDP as the communication
**			protocol in support of the "jsChanReadBlock" function.
*/
static long comm_read_block_chan_udp(
	UTLCOMM_CHANLIST_DESC*	pListChanIds,	/* ptr to list of comm chan IDs	*/
	int 					sizeMsgMax,		/* max size of message buffer	*/
	struct timeval*			pTimeOut,		/* ptr to timeout information 	*/
	int						(*pFuncCB)(),	/* ptr to callback function		*/
	void*					pDataCB,		/* ptr to callback data			*/
	struct timeval*			pTimeOutCB,		/* ptr to callback timeout info	*/
	char*					pMsg,			/* ptr to returned msg buffer	*/
	int* 					pSizeMsg,		/* ptr to returned size of msg	*/
	UTLCOMM_CHAN_DESC**		ppChanId)		/* ptr to the comm channel IDs	*/
{
	char*	func = "comm_read_block_chan_udp";	/* ptr to name of func	*/

/*	Allow for tracing information on ENTRY.
	--------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Allow for tracing information on EXIT.
	-------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

/*	Read from the comm channels for this application using UDP.
	---------------------------------------------------------- */
	UtlErr_Write(UTLCOMM_ERR_NOT_IMPLEMENTED, func, RIGHT_HERE);
	return(UTLCOMM_ERR_NOT_IMPLEMENTED);

/*	Return a successful status to the caller.
	return(UTL_SUCCESS);
	----------------------------------------- */
}

/*
** Name:
**			comm_read_block_chan_tcp
**
** Purpose:
**			This local function reads from a comm channel as 
**			an application using TCP/IP as the communication
**			protocol in support of the "jsChanRead" function.
*/
static long comm_read_block_chan_tcp(
	UTLCOMM_CHANLIST_DESC*	pListChanIds,	/* ptr to list of comm chan IDs	*/
	int 					sizeMsgMax,		/* max size of message buffer	*/
	struct timeval*			pTimeOut,		/* ptr to timeout information 	*/
	int						(*pFuncCB)(),	/* ptr to callback function		*/
	void*					pDataCB,		/* ptr to callback data			*/
	struct timeval*			pTimeOutCB,		/* ptr to callback timeout info	*/
	char*					pMsg,			/* ptr to returned msg buffer	*/
	int* 					pSizeMsg,		/* ptr to returned size of msg	*/
	UTLCOMM_CHAN_DESC**		ppChanId)		/* ptr to the comm chan ID ptr	*/
{
	int					err;			/* returned status code				*/
	fd_set				maskRead;		/* the SELECT read mask				*/
	fd_set				maskWrite;		/* the SELECT write mask			*/
	fd_set				maskExcept;		/* the SELECT exception mask		*/
	int					thisChanId;		/* this comm channel ID				*/
	int					searchDone;		/* search is done flag				*/
	int					numReady;		/* number sockets ready for read	*/
	int					szMsgBody;		/* size of the message body			*/
	int					szMsgHead;		/* size of the message header		*/
	int					userAbort;		/* the user has aborted indicator	*/
	UTLCOMM_CHAN_DESC*	pChanId;		/* ptr to the comm channel ID		*/
	char*	func = "comm_read_block_chan_tcp";	/* ptr to name of func		*/

/*	Allow for tracing information on ENTRY.
	--------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Indicate that nothing has been read at this time.
	------------------------------------------------- */
	*pSizeMsg = 0;

/*	See if there is anything to read on the comm channel.
	----------------------------------------------------- */
	numReady = comm_read_select(MASK_SOCKET_IO, pListChanIds,
								&maskRead, &maskWrite, &maskExcept, pTimeOut,
								pFuncCB, pDataCB, pTimeOutCB, &userAbort);

/*	If the user has aborted the operation, return immediately with
	the appropriate return status.
	-------------------------------------------------------------- */
	if (userAbort) {
		return(UTL_USER_ABORT);
	}

/*	There was a serious problem (like a broken pipe, etc.),
	so return the error condition to the caller (the caller
	is responsible for closing down the comm channel).
	------------------------------------------------------- */
	if (numReady < 0) {
		UtlErr_Write(UTLCOMM_ERR_SELECT, func, RIGHT_HERE);
		return(UTLCOMM_ERR_SELECT);
	}

/*	There is nothing to read from the comm channel, or the comm
	channel has timed out, so return this information to the caller.
	---------------------------------------------------------------- */
	if (numReady == 0) {
		if (pTimeOut == NULL) {
			return(UTLCOMM_ERR_NOTHING_TO_READ);
		} else {
			UtlErr_Write(UTLCOMM_ERR_TIMEOUT, func, RIGHT_HERE);
			return(UTLCOMM_ERR_TIMEOUT);
		}
	}

/*	We have something to read from the comm channel, so try
	to determine which socket was selected.
	------------------------------------------------------- */
	thisChanId = 0;
	searchDone = FALSE;
	while ( (!searchDone) && (thisChanId < pListChanIds->sizeList) ) {
		pChanId = pListChanIds->theList[thisChanId];
		if (FD_ISSET(pChanId->sockIO, &maskRead)) {
			searchDone = TRUE;
		} else {
			thisChanId++;
		}
	}

/*	Make sure that we actually found a socket in the select list.
	------------------------------------------------------------- */
	if (!searchDone) {
		UtlErr_Write(UTLCOMM_ERR_SOCK_NOT_FOUND, func, RIGHT_HERE);
		return(UTLCOMM_ERR_SOCK_NOT_FOUND);
	}

/*	Show the caller that a comm channel was found with something
	to read on it.
	------------------------------------------------------------ */
	*ppChanId = pChanId;
	
/*	Make sure that the caller has allocated enough space to receive
	the header of the message.
	--------------------------------------------------------------- */
	szMsgHead = pChanId->pHeaderInfo->sizeMsgHead;
	if ( szMsgHead  > sizeMsgMax ) {
		UtlErr_Write(UTLCOMM_ERR_MSG_TOO_LARGE, func, RIGHT_HERE);
		UtlLog_Write(LOG_ERROR | LOG_NO_ID, func, RIGHT_HERE,
			"ERROR: Message header (%d bytes) is too large (max=%d bytes)\n",
				szMsgHead, sizeMsgMax);
		return(UTLCOMM_ERR_MSG_TOO_LARGE);
	}

/*	We have something to read from the comm channel, so first
	read the number of bytes found in the expected header of
	the incoming message.
	--------------------------------------------------------- */
	err = comm_read_from_chan(pChanId, szMsgHead, pMsg);
	if (err != UTL_SUCCESS) {
		*pSizeMsg = 0;
		return(err);
	}

/*	Now that we have read in the message header, decode the header
	to determine how many more bytes need to be read to get the 
	rest of the incoming message.
	-------------------------------------------------------------- */
	szMsgBody = (int)comm_get_body_size(pChanId->pHeaderInfo, pMsg);
	if (szMsgBody < 0) {
		UtlErr_Write(UTLCOMM_ERR_GET_BODY_SIZE, func, RIGHT_HERE);
		return(UTLCOMM_ERR_GET_BODY_SIZE);
	}

/*	If there is only a message header and no body, then we are done.
	---------------------------------------------------------------- */
	if (szMsgBody == 0) {
		*pSizeMsg = szMsgHead;
		return(UTL_SUCCESS);
	}

/*	Make sure that the body of the message has a reasonable length.
	--------------------------------------------------------------- */
	if (szMsgBody > sizeMsgMax) {
		UtlErr_Write(UTLCOMM_ERR_BODY_TOO_LARGE, func, RIGHT_HERE);
		UtlLog_Write(LOG_ERROR | LOG_NO_ID, func, RIGHT_HERE,
			"ERROR: Message body (%d bytes) is too large (max=%d bytes)\n",
				szMsgBody, sizeMsgMax);
		return(UTLCOMM_ERR_BODY_TOO_LARGE);
	}
	
/*	Make sure that the caller has allocated enough space to receive
	the body of the message.
	--------------------------------------------------------------- */
	if ( (szMsgHead + szMsgBody) > sizeMsgMax ) {
		UtlErr_Write(UTLCOMM_ERR_MSG_TOO_LARGE, func, RIGHT_HERE);
		UtlLog_Write(LOG_ERROR | LOG_NO_ID, func, RIGHT_HERE,
			"ERROR: Message (%d bytes) is too large (max=%d bytes)\n",
				(szMsgHead + szMsgBody), sizeMsgMax);
		return(UTLCOMM_ERR_MSG_TOO_LARGE);
	}

/*	Now read the body of the message into the caller's data space.
	-------------------------------------------------------------- */
	err = comm_read_from_chan(pChanId, szMsgBody, (pMsg + szMsgHead));
	if (err != UTL_SUCCESS) {
		return(err);
	}
	*pSizeMsg = szMsgHead + szMsgBody;

/*	Allow for tracing information on EXIT.
	-------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

/*	Return a successful status to the caller.
	----------------------------------------- */
	return(UTL_SUCCESS);
}

/*
** Name:
**			comm_read_select
**
** Purpose:
**			This local function breaks a user-specified select call
**			up into several select calls of a short duration if
**			the caller has specified a callback function.  The
**			callback function is invoked if the select falls through
**			and no data is ready to be read.  After the callback
**			function returns, if an ABORT status is returned, then
**			this read function will promptly terminate.
*/
static int comm_read_select(
	int						typeSocket,		/* the type of socket to check	*/
	UTLCOMM_CHANLIST_DESC*	pListChanIds,	/* ptr to list of comm chan IDs	*/
	fd_set*					pMaskRead,		/* ptr to SELECT read mask		*/
	fd_set*					pMaskWrite,		/* ptr to SELECT write mask		*/
	fd_set*					pMaskExcept,	/* ptr to SELECT except mask	*/
	struct timeval*			pTimeOut,		/* ptr to timeout information	*/
	int						(*pFuncCB)(),	/* ptr to callback function		*/
	void*					pDataCB,		/* ptr to callback data			*/
	struct timeval*			pTimeOutCB,		/* ptr to callback timeout info	*/
	int*					pUserAbort)		/* ptr to user abort indicator	*/
{
	struct timeval*		pThisTimeOut;	/* ptr to this timeout structure	*/
	int					thisChanId;		/* this comm channel ID				*/
	int					maxTimeOuts;	/* max number of timeouts			*/
	int					maxTimeOutsCB;	/* max number of callback timeouts	*/
	int					numTimeOuts;	/* the number of timeouts so far	*/
	int					numReady;		/* number sockets ready for read	*/
	int					abortRead;		/* indication to abort the read		*/
	UTLCOMM_CHAN_DESC*	pChanId;		/* ptr to the comm channel ID		*/
	char*	func = "comm_read_select";	/* ptr to name of func				*/

/*	Allow for tracing information on ENTRY.
	--------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Assume that the user has NOT aborted the operation.
	--------------------------------------------------- */
	*pUserAbort = FALSE;

/*	Determine which timeout structure to use for the select.  Basically,
	if the caller has specified a callback function, then we need to
	use the callback timeout structure N times, where N is about equal
	to the number of "callback timeouts" in the main user-specified    
	timeout.  To determine N, just use the seconds information from the
	timeout structures.  This is not totally accurate, but it is probably
	good enough for most applications.  If this is not true, a more precise
	algorithm can easily be constructed.
	----------------------------------------------------------------------- */
	maxTimeOuts = 1;
	pThisTimeOut = pTimeOut;
	if (pFuncCB != NULL) {
		if ( (pTimeOutCB != NULL) && (pTimeOut != NULL) ) {
			if (comm_calc_max_timeouts(	pTimeOut, pTimeOutCB,
										&maxTimeOutsCB			)) {
				maxTimeOuts = maxTimeOutsCB;
				pThisTimeOut = pTimeOutCB;
			}
		}
	}

/*	Loop for up to the maximum number of timeouts.
	---------------------------------------------- */
	numTimeOuts = 0;
	while (numTimeOuts < maxTimeOuts) {

/*		Start by setting up the SELECT masks for read, write, and except.
		For each comm channel in the list, we need to set the read mask
		if the comm channel is NOT a Master SERVER comm channel and the
		desired type of socket is a regular I/O socket.  We also need to
		set the read mask if comm channel IS a Master SERVER comm channel
		and the desired type of socket is a listening socket.
		----------------------------------------------------------------- */
		FD_ZERO(pMaskRead); FD_ZERO(pMaskWrite); FD_ZERO(pMaskExcept);
		for (thisChanId=0; thisChanId<pListChanIds->sizeList; thisChanId++) {
			pChanId = pListChanIds->theList[thisChanId];
			if ( UtlBit_Test((void*)&typeSocket, TYPE_SOCKET_IO, FALSE) &&
				 !(pChanId->isMaster &&
				   pChanId->typeApp == SERVER_APP) ) {
				FD_SET(pChanId->sockIO, pMaskRead);
			} else if ( UtlBit_Test((void*)&typeSocket, TYPE_SOCKET_LISTEN,
									FALSE) &&
						(pChanId->isMaster &&
						 pChanId->typeApp == SERVER_APP) ) {
				FD_SET(pChanId->sockListen, pMaskRead);
			}
		}

/*		See if there is anything to read on the comm channel.
		----------------------------------------------------- */
		numReady = select(FD_SETSIZE, pMaskRead, pMaskWrite, pMaskExcept,
						  pThisTimeOut);

/*		There appears to something ready to read, so return to our
		caller and let him decide what to do next.
		---------------------------------------------------------- */
		if (numReady > 0) {
			return(numReady);
		}

/*		There was a serious problem (like a broken pipe, etc.), so
		return to our caller and let him decide what to do next.
		Remember to check for a system interrupt - this kind of 
		error is acceptable and means we just continue waiting to
		see if there is anything to read on the comm channel.
		---------------------------------------------------------- */
		if (numReady < 0) {
			if (WSAGetLastError() == WSAEINTR) goto _errSystemInterrupt;
			return(numReady);
		}

/*		There is nothing to read from the comm channel, or the comm
		channel has timed out, so check to see if we need to invoke
		a specified callback function before trying to wait again.
		If the callback function returns and indicates that the user
		wants to abort, accomodate the user by prematurely ending
		our looping logic and return immediately to our caller and
		let him decide what to do next (which will be to return with
		a USER ABORT error condition).
		------------------------------------------------------------ */
		if (pFuncCB != NULL) {
			abortRead = (*pFuncCB)(pDataCB);
			if (abortRead) {
				*pUserAbort = TRUE;
				return(numReady);
			}
		}

/*		Bump the number of timeouts and try again.
		------------------------------------------ */
		numTimeOuts++;

/*		System interrupts should NOT affect our number of timeouts...
		------------------------------------------------------------- */
_errSystemInterrupt:
		;
	}

/*	Allow for tracing information on ENTRY.
	--------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

	return(numReady);
}

/*
** Name:
**			comm_write_chan
**
** Purpose:
**			This local function writes to a comm channel as either
**			a CLIENT or a SERVER application in support of the
**			"jsChanWrite" function.
*/
static long comm_write_chan(
	UTLCOMM_CHAN_DESC*	pChanId,		/* ptr to the comm channel ID	*/
	int 				sizeMsgTotal,	/* total size of message buffer	*/
	struct timeval*		pTimeOut,		/* ptr to timeout information 	*/
	int					(*pFuncCB)(),	/* ptr to callback function		*/
	void*				pDataCB,		/* ptr to callback data			*/
	struct timeval*		pTimeOutCB,		/* ptr to callback timeout info	*/
	char*				pMsg,			/* ptr to msg buffer to write	*/
	int* 				pSizeMsg)		/* ptr to returned size of msg	*/
{
	char*		func = "comm_write_chan";		/* ptr to name of func	*/

/*	Allow for tracing information on ENTRY.
	--------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Look at the communication type to decide what kind of write needs
	to be done for this invocation of "jsChanWrite".
	-------------------------------------------------------------------- */
	switch (pChanId->typeComm) {

/*		This is an application using UDP.
		--------------------------------- */
		case (UDP_COMM):
			return(comm_write_chan_udp(	pChanId, sizeMsgTotal, pTimeOut,
										pFuncCB, pDataCB, pTimeOutCB,
										pMsg, pSizeMsg));
			break;

/*		This is an application using TCP/IP.
		------------------------------------ */
		case (TCP_COMM):
			return(comm_write_chan_tcp(	pChanId, sizeMsgTotal, pTimeOut,
										pFuncCB, pDataCB, pTimeOutCB,
										pMsg, pSizeMsg));
			break;

/*		This is an UNKNOWN communication type.
		-------------------------------------- */
		default:
			UtlErr_Write(UTLCOMM_ERR_INVALID_APP_TYPE, func, RIGHT_HERE);
			return(UTLCOMM_ERR_INVALID_APP_TYPE);
			break;
	}	

/*	Allow for tracing information on EXIT.
	-------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

/*	Return a successful status to the caller.
	----------------------------------------- */
	return(UTL_SUCCESS);
}

/*
** Name:
**			comm_write_chan_udp
**
** Purpose:
**			This local function writes to a comm channel as 
**			an application using UDP as the communication
**			protocol in support of the "jsChanWrite" function.
*/
static long comm_write_chan_udp(
	UTLCOMM_CHAN_DESC*	pChanId,		/* ptr to the comm channel ID	*/
	int 				sizeMsgTotal,	/* total size of message buffer	*/
	struct timeval*		pTimeOut,		/* ptr to timeout information 	*/
	int					(*pFuncCB)(),	/* ptr to callback function		*/
	void*				pDataCB,		/* ptr to callback data			*/
	struct timeval*		pTimeOutCB,		/* ptr to callback timeout info	*/
	char*				pMsg,			/* ptr to msg buffer to write	*/
	int* 				pSizeMsg)		/* ptr to returned size of msg	*/
{
	char*	func = "comm_write_chan_udp";		/* ptr to name of func	*/

/*	Allow for tracing information on ENTRY.
	--------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Allow for tracing information on EXIT.
	-------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

/*	Write to the comm channel for this application using UDP.
	--------------------------------------------------------- */
	UtlErr_Write(UTLCOMM_ERR_NOT_IMPLEMENTED, func, RIGHT_HERE);
	return(UTLCOMM_ERR_NOT_IMPLEMENTED);

/*	Return a successful status to the caller.
	return(UTL_SUCCESS);
	----------------------------------------- */
}

/*
** Name:
**			comm_write_chan_tcp
**
** Purpose:
**			This local function writes to a comm channel as 
**			an application using TCP/IP as the communication
**			protocol in support of the "jsChanWrite" function.
*/
static long comm_write_chan_tcp(
	UTLCOMM_CHAN_DESC*	pChanId,		/* ptr to the comm channel ID	*/
	int 				sizeMsgTotal,	/* total size of message buffer	*/
	struct timeval*		pTimeOut,		/* ptr to timeout information 	*/
	int					(*pFuncCB)(),	/* ptr to callback function		*/
	void*				pDataCB,		/* ptr to callback data			*/
	struct timeval*		pTimeOutCB,		/* ptr to callback timeout info	*/
	char*				pMsg,			/* ptr to msg buffer to write	*/
	int* 				pSizeMsg)		/* ptr to returned size of msg	*/
{
	int			err;				/* the error return status 				*/
	char*	func = "comm_write_chan_tcp";			/* ptr to name of func	*/

/*	Allow for tracing information on ENTRY.
	--------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Get the entire message written out to the comm channel.
	------------------------------------------------------- */
	err = comm_write_to_chan(	pChanId,
								sizeMsgTotal, pMsg	);
	if (err != UTL_SUCCESS) {
		*pSizeMsg = 0;
		return(err);
	} else {
		*pSizeMsg = sizeMsgTotal;
	}

/*	Allow for tracing information on EXIT.
	-------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

/*	Return a successful status to the caller.
	----------------------------------------- */
	return(UTL_SUCCESS);
}

/*
** Name:
**			comm_write_to_chan
**
** Purpose:
**			This local function writes a specified number of bytes
**			to a specified socket from a specified buffer that
**			has been allocated by the caller.
*/
static long comm_write_to_chan(
	UTLCOMM_CHAN_DESC*	pChanId,	/* ptr to the comm channel ID		*/
	int			sizeToWrite,		/* the size (in bytes) to write		*/
	char*		pBuff)				/* ptr to the buffer to write from	*/
{
	int		optSend;				/* the options on the SEND				*/
	int		numTries;				/* the number of attempts to try 		*/
	int		sizeWrittenThisTime;	/* the number of bytes written this time*/
	int		sizeLeftToWrite;		/* the number of bytes left to write 	*/
	int		sizeWrittenSoFar;		/* the number of bytes written so far	*/
	char*	func = "comm_write_to_chan";		/* ptr to name of func		*/

/*	Allow for tracing information on ENTRY.
	--------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Get ready to write all the requested bytes to the pipe.
	------------------------------------------------------- */
	optSend          = 0;
	numTries         = 0;
	sizeWrittenSoFar = 0;
	sizeLeftToWrite  = sizeToWrite;

/*	Loop until we have sent out all the bytes requested by the caller.
	------------------------------------------------------------------ */
	while ( (sizeWrittenSoFar != sizeToWrite) &&
			(numTries < MAX_SEND_TIMEOUTS) ) {
		sizeWrittenThisTime = send(pChanId->sockIO, (pBuff + sizeWrittenSoFar),
								   sizeLeftToWrite, optSend);
		if (sizeWrittenThisTime < 0) {
			if (WSAGetLastError() == WSAEINTR) goto _errSystemInterrupt;
			UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
				"Error from SEND, errno=%d\n", WSAGetLastError());
			UtlErr_Write(UTLCOMM_ERR_SEND, func, RIGHT_HERE);
			UtlLog_Write(LOG_DEBUG | LOG_NO_ID, func, RIGHT_HERE,
				"Expected number of bytes to write = %d\n", sizeToWrite);
			if (sizeWrittenSoFar > 0) {
				UtlDump_Mem(pBuff, sizeWrittenSoFar, LOG_DEBUG);
			}
			return(UTLCOMM_ERR_SEND);
		} else {
			sizeLeftToWrite  -= sizeWrittenThisTime;
			sizeWrittenSoFar += sizeWrittenThisTime;
			if (sizeWrittenSoFar != sizeToWrite) {
				Sleep(1000);
			}
		}

/*		Bump the number of tries to send all the bytes.
		----------------------------------------------- */
		numTries++;

/*		System interrupts should NOT affect our number of tries...
		---------------------------------------------------------- */
_errSystemInterrupt:
		;
	}

/*	If the number of bytes written do not match the intended
	number indicated by the caller, we had a problem.
	-------------------------------------------------------- */
	if (sizeWrittenSoFar != sizeToWrite) {
		UtlErr_Write(UTLCOMM_ERR_WRITE_MISMATCH, func, RIGHT_HERE);
		UtlLog_Write(LOG_DEBUG, func, RIGHT_HERE,
			"Tried to send %d bytes, only sent %d\n", sizeToWrite,
				sizeWrittenSoFar);
		if (sizeWrittenSoFar > 0) {
			UtlDump_Mem(pBuff, sizeWrittenSoFar, LOG_DEBUG);
		}
		return(UTLCOMM_ERR_WRITE_MISMATCH);
	}

/*	Allow for tracing information on EXIT.
	-------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

/*	Return a successful status to the caller.
	----------------------------------------- */
	return(UTL_SUCCESS);
}

/*
** Name:
**			comm_validate_accept_parms
**
** Purpose:
**			This local function validates the input parameters
**			for the "jsChanAccept" function.
*/
static long comm_validate_accept_parms(
	UTLCOMM_CHAN_DESC*	pMasterChanId,	/* ptr to master comm channel ID	*/
	UTLCOMM_CHAN_DESC**	ppChanId)		/* ptr to returned ptr to the comm	*/
										/* channel ID that was accepted		*/
{
	char*	func = "comm_validate_accept_parms";	/* ptr to name of func	*/

/*	Allow for tracing information on ENTRY.
	--------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Make sure the ptr to the master comm channel ID is valid.
	--------------------------------------------------------- */
	if ( pMasterChanId == NULL ) {
		UtlErr_Write(UTLCOMM_ERR_INVALID_PTR, func, RIGHT_HERE);
		return(UTLCOMM_ERR_INVALID_PTR);
	}

/*	Make sure the ptr to the comm channel ID ptr is valid.
	------------------------------------------------------ */
	if ( ppChanId == NULL ) {
		UtlErr_Write(UTLCOMM_ERR_INVALID_PTR, func, RIGHT_HERE);
		return(UTLCOMM_ERR_INVALID_PTR);
	}

/*	Allow for tracing information on EXIT.
	-------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

/*	Everything checked out just fine, so indicated a success
	to the caller.
	-------------------------------------------------------- */
	return(UTL_SUCCESS);
}

/*
** Name:
**			comm_validate_close_parms
**
** Purpose:
**			This local function validates the input parameters
**			for the "jsChanClose" function.
*/
static long comm_validate_close_parms(
	UTLCOMM_CHAN_DESC*	pChanId)		/* ptr to the comm channel ID	*/
{
	char*	func = "comm_validate_close_parms";	/* ptr to name of func	*/

/*	Allow for tracing information on ENTRY.
	--------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Make sure the ptr to the comm channel ID is valid.
	-------------------------------------------------- */
	if (pChanId == NULL) {
		UtlErr_Write(UTLCOMM_ERR_INVALID_PTR, func, RIGHT_HERE);
		return(UTLCOMM_ERR_INVALID_PTR);
	}

/*	Allow for tracing information on EXIT.
	-------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

/*	Everything checked out just fine, so indicated a success
	to the caller.
	-------------------------------------------------------- */
	return(UTL_SUCCESS);
}

/*
** Name:
**			comm_validate_connect_pending_parms
**
** Purpose:
**			This local function validates the input parameters
**			for the "jsChanConnectPending" function.
*/
static long comm_validate_connect_pending_parms(
	UTLCOMM_CHAN_DESC*	pChanId,		/* ptr to the comm channel ID	*/
	struct timeval*		pTimeOut)		/* ptr to timeout information	*/
{
	char*	func = "comm_validate_connect_pending_parms";
										/* ptr to name of func	*/

/*	Allow for tracing information on ENTRY.
	--------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Make sure the ptr to the comm channel ID is valid.
	-------------------------------------------------- */
	if (pChanId == NULL) {
		UtlErr_Write(UTLCOMM_ERR_INVALID_PTR, func, RIGHT_HERE);
		return(UTLCOMM_ERR_INVALID_PTR);
	}

/*	Make sure the timeout information is valid (nothing to do for now).
	------------------------------------------------------------------- */

/*	Make sure that this application is a server type.
	------------------------------------------------- */
	if (pChanId->typeApp != SERVER_APP) {
		UtlErr_Write(UTLCOMM_ERR_APP_NOT_SERVER, func, RIGHT_HERE);
		return(UTLCOMM_ERR_APP_NOT_SERVER);
	}

/*	Allow for tracing information on EXIT.
	-------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

/*	Everything checked out just fine, so indicated a success
	to the caller.
	-------------------------------------------------------- */
	return(UTL_SUCCESS);
}

/*
** Name:
**			comm_validate_input_pending_parms
**
** Purpose:
**			This local function validates the input parameters
**			for the "jsChanInputPending" function.
*/
static long comm_validate_input_pending_parms(
	UTLCOMM_CHANLIST_DESC*	pListChanIds,	/* ptr to the comm channel IDs	*/
	struct timeval*			pTimeOut,		/* ptr to timeout information 	*/
	UTLCOMM_CHAN_DESC**		ppChanId)		/* ptr to the comm channel ptr	*/
{
	int					thisChanId;		/* this comm channel ID				*/
	UTLCOMM_CHAN_DESC*	pChanId;		/* ptr to the comm channel ptr		*/
	char*	func="comm_validate_input_pending_parms"; /* ptr to name of func*/

/*	Allow for tracing information on ENTRY.
	--------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Make sure the ptr to the comm channel IDs is valid.
	--------------------------------------------------- */
	if (pListChanIds == NULL) {
		UtlErr_Write(UTLCOMM_ERR_INVALID_PTR, func, RIGHT_HERE);
		return(UTLCOMM_ERR_INVALID_PTR);
	}

/*	Make sure the number of comm channel IDs is valid.
	-------------------------------------------------- */
	if ( (pListChanIds->sizeList <= 0) ||
		 (pListChanIds->sizeList > MAX_CHANNELS) ) {
		UtlErr_Write(UTLCOMM_ERR_INVALID_NUM_CHANS, func, RIGHT_HERE);
		return(UTLCOMM_ERR_INVALID_NUM_CHANS);
	}

/*	Make sure the comm channel IDs in the list are all valid.
	--------------------------------------------------------- */
	for (thisChanId=0; thisChanId<pListChanIds->sizeList; thisChanId++) {
		if (pListChanIds->theList[thisChanId] == NULL) {
			UtlErr_Write(UTLCOMM_ERR_INVALID_PTR, func, RIGHT_HERE);
			return(UTLCOMM_ERR_INVALID_PTR);
		}
	}

/*	Make sure the application type for each of the comm channel IDs
	is a SERVER_APP type.
	--------------------------------------------------------------- */
	for (thisChanId=0; thisChanId<pListChanIds->sizeList; thisChanId++) {
		pChanId = pListChanIds->theList[thisChanId];
		if (pChanId->typeApp != SERVER_APP) {
			UtlErr_Write(UTLCOMM_ERR_APP_NOT_SERVER, func, RIGHT_HERE);
			return(UTLCOMM_ERR_APP_NOT_SERVER);
		}
	}

/*	Make sure the timeout information is valid (nothing to do for now).
	------------------------------------------------------------------- */

/*	Make sure the ptr to the comm channel ID ptr is valid.
	------------------------------------------------------ */
	if (ppChanId == NULL) {
		UtlErr_Write(UTLCOMM_ERR_INVALID_PTR, func, RIGHT_HERE);
		return(UTLCOMM_ERR_INVALID_PTR);
	}

/*	Allow for tracing information on EXIT.
	-------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

/*	Everything checked out just fine, so indicated a success
	to the caller.
	-------------------------------------------------------- */
	return(UTL_SUCCESS);
}

/*
** Name:
**			comm_validate_open_parms
**
** Purpose:
**			This local function validates the input parameters
**			for the "jsChanOpen" function.
*/
static long comm_validate_open_parms(
	int 				typeComm,		/* type of comm (UDP/TCP)		*/
	int 				typeApp,		/* type of app (CLIENT/SERVER)	*/
	int					maxConnections,	/* max number of connections	*/
	struct timeval*		pConnTimeout,	/* ptr to the connect timeout	*/
	char*				pNameHost,		/* ptr to name of the host		*/
	char*				pNameService,	/* ptr to name of the service	*/
	UTLCOMM_HDR_DESC*	pHeaderInfo,	/* ptr to msg header info		*/
	UTLCOMM_CHAN_DESC**	ppChanId)		/* ptr to comm channel ID ptr	*/
{
	char*	func = "comm_validate_open_parms";	/* ptr to name of func	*/

/*	Allow for tracing information on ENTRY.
	--------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Make sure the type of communication is valid.
	-------------------------------------------- */
	if ( ( typeComm != UDP_COMM) && (typeComm != TCP_COMM) ) {
		UtlErr_Write(UTLCOMM_ERR_INVALID_COMM_TYPE, func, RIGHT_HERE);
		return(UTLCOMM_ERR_INVALID_COMM_TYPE);
	}

/*	Make sure the type of application is valid.
	------------------------------------------- */
	if ( ( typeApp != CLIENT_APP) && (typeApp != SERVER_APP) ) {
		UtlErr_Write(UTLCOMM_ERR_INVALID_APP_TYPE, func, RIGHT_HERE);
		return(UTLCOMM_ERR_INVALID_APP_TYPE);
	}

/*	Make sure the maximum number of concurrent connections is valid
	if this application is SERVER_APP.
	---------------------------------------------------------------- */
	if (typeApp == SERVER_APP) {
		if ( (maxConnections <= 0) || (maxConnections > MAX_CONNECTIONS) ) {
			UtlErr_Write(UTLCOMM_ERR_MAX_CONNECTIONS, func, RIGHT_HERE);
			return(UTLCOMM_ERR_MAX_CONNECTIONS);
		}
	}

/*	Make sure the connection timeout, if supplied, is valid and
	looks like a reasonable size if this application is CLIENT_APP.
	If the timeout is valid but not a reasonable size, just put
	out a warning message. 
	--------------------------------------------------------------- */
	if (typeApp == CLIENT_APP) {
		if (pConnTimeout != NULL) {
			if ((pConnTimeout->tv_sec < 0) || (pConnTimeout->tv_usec < 0)) {
				UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
					"ERROR: Invalid connection timeout, sec=%d, usec=%d\n",
						pConnTimeout->tv_sec, pConnTimeout->tv_usec);
				return(UTLCOMM_ERR_INVALID_TIMEOUT);
			} else if (pConnTimeout->tv_sec > REASONABLE_CONN_TIMEOUT) {
				UtlLog_Write(LOG_WARN, func, RIGHT_HERE,
					"WARNING: Connect timeout (%d) > reasonable limit (%d)\n",
						pConnTimeout->tv_sec, REASONABLE_CONN_TIMEOUT);
			}
		}
	}

/*	Make sure the name of the host is valid.  Note that if the
	application type is SERVER_APP, a NULL pointer is okay (since
	a SERVER_APP does not need the host name information).
	------------------------------------------------------------- */
	if (pNameHost == NULL) {
		if (typeApp == CLIENT_APP) {
			UtlErr_Write(UTLCOMM_ERR_INVALID_HOST, func, RIGHT_HERE);
			return(UTLCOMM_ERR_INVALID_HOST);
		}
	} else if (strlen(pNameHost) > MAX_NAME_LEN) {
		UtlErr_Write(UTLCOMM_ERR_INVALID_HOST, func, RIGHT_HERE);
		return(UTLCOMM_ERR_INVALID_HOST);
	}

/*	Make sure the name of the service is valid.
	------------------------------------------- */
	if (pNameService == NULL) {
		UtlErr_Write(UTLCOMM_ERR_INVALID_SERVICE, func, RIGHT_HERE);
		return(UTLCOMM_ERR_INVALID_SERVICE);
	} else if (strlen(pNameService) > MAX_NAME_LEN) {
		UtlErr_Write(UTLCOMM_ERR_INVALID_SERVICE, func, RIGHT_HERE);
		return(UTLCOMM_ERR_INVALID_SERVICE);
	}

/*	Make sure the message header information is valid.
	-------------------------------------------------- */
	if ( (pHeaderInfo->sizeBodyLen != BODY_SIZE_CHAR) &&
		 (pHeaderInfo->sizeBodyLen != BODY_SIZE_SHORT) &&
		 (pHeaderInfo->sizeBodyLen != BODY_SIZE_LONG) ) {
		UtlErr_Write(UTLCOMM_ERR_INVALID_BODY_SIZE, func, RIGHT_HERE);
		return(UTLCOMM_ERR_INVALID_BODY_SIZE);
	}

	if ( (pHeaderInfo->offBodyLen + pHeaderInfo->sizeBodyLen) >
		 pHeaderInfo->sizeMsgHead ) {
		UtlErr_Write(UTLCOMM_ERR_INVALID_HDR_INFO, func, RIGHT_HERE);
		return(UTLCOMM_ERR_INVALID_HDR_INFO);
	}

/*	Make sure the ptr to the comm channel ID ptr is valid.
	------------------------------------------------------ */
	if ( ppChanId == NULL ) {
		UtlErr_Write(UTLCOMM_ERR_INVALID_PTR, func, RIGHT_HERE);
		return(UTLCOMM_ERR_INVALID_PTR);
	}

/*	Allow for tracing information on EXIT.
	-------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

/*	Everything checked out just fine, so indicated a success
	to the caller.
	-------------------------------------------------------- */
	return(UTL_SUCCESS);
}

/*
** Name:
**			comm_validate_read_parms
**
** Purpose:
**			This local function validates the input parameters
**			for the "jsChanRead" function.
*/
static long comm_validate_read_parms(
	UTLCOMM_CHAN_DESC*	pChanId,		/* ptr to the comm channel ID	*/
	int 				sizeMsgMax,		/* max size of message buffer	*/
	struct timeval*		pTimeOut,		/* ptr to timeout information 	*/
	int					(*pFuncCB)(),	/* ptr to callback function		*/
	void*				pDataCB,		/* ptr to callback data			*/
	struct timeval*		pTimeOutCB,		/* ptr to callback timeout info	*/
	char*				pMsg,			/* ptr to returned msg buffer	*/
	int* 				pSizeMsg)		/* ptr to returned size of msg	*/
{
	char*	func = "comm_validate_read_parms";	/* ptr to name of func	*/

/*	Allow for tracing information on ENTRY.
	--------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Make sure the ptr to the comm channel ID is valid.
	-------------------------------------------------- */
	if (pChanId == NULL) {
		UtlErr_Write(UTLCOMM_ERR_INVALID_PTR, func, RIGHT_HERE);
		return(UTLCOMM_ERR_INVALID_PTR);
	}

/*	Make sure the comm channel ID has a valid I/O socket (i.e., if
	this is a listening socket, then the I/O socket will still be
	uninitialized since I/O cannot be done on the listening socket
	of a SERVER_APP).
	-------------------------------------------------------------- */
	if (pChanId->sockIO < 0) {
		UtlErr_Write(UTLCOMM_ERR_INVALID_SOCK_IO, func, RIGHT_HERE);
		return(UTLCOMM_ERR_INVALID_SOCK_IO);
	}

/*	Make sure the max size of the message is valid.
	----------------------------------------------- */
	if (sizeMsgMax < pChanId->pHeaderInfo->sizeMsgHead) {
		UtlErr_Write(UTLCOMM_ERR_INVALID_MSG_SIZE, func, RIGHT_HERE);
		return(UTLCOMM_ERR_INVALID_MSG_SIZE);
	}

/*	Make sure the timeout information is valid (nothing to do for now).
	------------------------------------------------------------------- */

/*	Make sure the ptr to the returned message is valid.
	--------------------------------------------------- */
	if (pMsg == NULL) {
		UtlErr_Write(UTLCOMM_ERR_INVALID_PTR, func, RIGHT_HERE);
		return(UTLCOMM_ERR_INVALID_PTR);
	}

/*	Make sure the ptr to the returned message size is valid.
	-------------------------------------------------------- */
	if (pSizeMsg == NULL) {
		UtlErr_Write(UTLCOMM_ERR_INVALID_PTR, func, RIGHT_HERE);
		return(UTLCOMM_ERR_INVALID_PTR);
	}

/*	Allow for tracing information on EXIT.
	-------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

/*	Everything checked out just fine, so indicated a success
	to the caller.
	-------------------------------------------------------- */
	return(UTL_SUCCESS);
}

/*
** Name:
**			comm_validate_read_block_parms
**
** Purpose:
**			This local function validates the input parameters
**			for the "jsChanReadBlock" function.
*/
static long comm_validate_read_block_parms(
	UTLCOMM_CHANLIST_DESC*	pListChanIds,	/* ptr to the comm channel IDs	*/
	int 					sizeMsgMax,		/* max size of message buffer	*/
	struct timeval*			pTimeOut,		/* ptr to timeout information 	*/
	int						(*pFuncCB)(),	/* ptr to callback function		*/
	void*					pDataCB,		/* ptr to callback data			*/
	struct timeval*			pTimeOutCB,		/* ptr to callback timeout info	*/
	char*					pMsg,			/* ptr to returned msg buffer	*/
	int* 					pSizeMsg,		/* ptr to returned size of msg	*/
	UTLCOMM_CHAN_DESC**		ppChanId)		/* ptr to the comm channel ptr	*/
{
	int					thisChanId;		/* this comm channel ID				*/
	UTLCOMM_CHAN_DESC*	pChanId;		/* ptr to the comm channel ptr		*/
	char*	func = "comm_validate_read_block_parms";/* ptr to name of func	*/

/*	Allow for tracing information on ENTRY.
	--------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Make sure the ptr to the comm channel IDs is valid.
	--------------------------------------------------- */
	if (pListChanIds == NULL) {
		UtlErr_Write(UTLCOMM_ERR_INVALID_PTR, func, RIGHT_HERE);
		return(UTLCOMM_ERR_INVALID_PTR);
	}

/*	Make sure the number of comm channel IDs is valid.
	-------------------------------------------------- */
	if ( (pListChanIds->sizeList <= 0) ||
		 (pListChanIds->sizeList > MAX_CHANNELS) ) {
		UtlErr_Write(UTLCOMM_ERR_INVALID_NUM_CHANS, func, RIGHT_HERE);
		return(UTLCOMM_ERR_INVALID_NUM_CHANS);
	}

/*	Make sure the comm channel IDs in the list are all valid.
	--------------------------------------------------------- */
	for (thisChanId=0; thisChanId<pListChanIds->sizeList; thisChanId++) {
		if (pListChanIds->theList[thisChanId] == NULL) {
			UtlErr_Write(UTLCOMM_ERR_INVALID_PTR, func, RIGHT_HERE);
			return(UTLCOMM_ERR_INVALID_PTR);
		}
	}

/*	Make sure the comm channel IDs each has a valid I/O socket (i.e., 
	if this is a listening socket, then the I/O socket will still be
	uninitialized since I/O cannot be done on the listening socket
	of a SERVER_APP).
	----------------------------------------------------------------- */
	for (thisChanId=0; thisChanId<pListChanIds->sizeList; thisChanId++) {
		pChanId = pListChanIds->theList[thisChanId];
		if (pChanId->sockIO < 0) {
			UtlErr_Write(UTLCOMM_ERR_INVALID_SOCK_IO, func, RIGHT_HERE);
			return(UTLCOMM_ERR_INVALID_SOCK_IO);
		}
	}

/*	Make sure the max size of the message is valid.
	----------------------------------------------- */
	pChanId = pListChanIds->theList[0];
	if (sizeMsgMax < pChanId->pHeaderInfo->sizeMsgHead) {
		UtlErr_Write(UTLCOMM_ERR_INVALID_MSG_SIZE, func, RIGHT_HERE);
		return(UTLCOMM_ERR_INVALID_MSG_SIZE);
	}

/*	Make sure the timeout information is valid (nothing to do for now).
	------------------------------------------------------------------- */

/*	Make sure the ptr to the returned message is valid.
	--------------------------------------------------- */
	if (pMsg == NULL) {
		UtlErr_Write(UTLCOMM_ERR_INVALID_PTR, func, RIGHT_HERE);
		return(UTLCOMM_ERR_INVALID_PTR);
	}

/*	Make sure the ptr to the returned message size is valid.
	-------------------------------------------------------- */
	if (pSizeMsg == NULL) {
		UtlErr_Write(UTLCOMM_ERR_INVALID_PTR, func, RIGHT_HERE);
		return(UTLCOMM_ERR_INVALID_PTR);
	}

/*	Make sure the ptr to the comm channel ID ptr is valid.
	------------------------------------------------------ */
	if (ppChanId == NULL) {
		UtlErr_Write(UTLCOMM_ERR_INVALID_PTR, func, RIGHT_HERE);
		return(UTLCOMM_ERR_INVALID_PTR);
	}

/*	Allow for tracing information on EXIT.
	-------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

/*	Everything checked out just fine, so indicated a success
	to the caller.
	-------------------------------------------------------- */
	return(UTL_SUCCESS);
}

/*
** Name:
**			comm_validate_write_parms
**
** Purpose:
**			This local function validates the input parameters
**			for the "jsChanWrite" function.
*/
static long comm_validate_write_parms(
	UTLCOMM_CHAN_DESC*	pChanId,		/* ptr to the comm channel ID	*/
	int 				sizeMsgTotal,	/* total size of message buffer	*/
	struct timeval*		pTimeOut,		/* ptr to timeout information 	*/
	int					(*pFuncCB)(),	/* ptr to callback function		*/
	void*				pDataCB,		/* ptr to callback data			*/
	struct timeval*		pTimeOutCB,		/* ptr to callback timeout info	*/
	char*				pMsg,			/* ptr to returned msg buffer	*/
	int* 				pSizeMsg)		/* ptr to returned size of msg	*/
{
	char*	func = "comm_validate_write_parms";	/* ptr to name of func	*/

/*	Allow for tracing information on ENTRY.
	--------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Make sure the ptr to the comm channel ID is valid.
	-------------------------------------------------- */
	if (pChanId == NULL) {
		UtlErr_Write(UTLCOMM_ERR_INVALID_PTR, func, RIGHT_HERE);
		return(UTLCOMM_ERR_INVALID_PTR);
	}

/*	Make sure the comm channel ID has a valid I/O socket (i.e., if
	this is a listening socket, then the I/O socket will still be
	uninitialized since I/O cannot be done on the listening socket
	of a SERVER_APP).
	-------------------------------------------------------------- */
	if (pChanId->sockIO < 0) {
		UtlErr_Write(UTLCOMM_ERR_INVALID_SOCK_IO, func, RIGHT_HERE);
		return(UTLCOMM_ERR_INVALID_SOCK_IO);
	}

/*	Make sure the total size of the message is valid.
	------------------------------------------------- */
	if (sizeMsgTotal <= 0) {
		UtlErr_Write(UTLCOMM_ERR_INVALID_MSG_SIZE, func, RIGHT_HERE);
		return(UTLCOMM_ERR_INVALID_MSG_SIZE);
	}

/*	Make sure the timeout information is valid (nothing to do for now).
	------------------------------------------------------------------- */

/*	Make sure the ptr to the returned message is valid.
	--------------------------------------------------- */
	if (pMsg == NULL) {
		UtlErr_Write(UTLCOMM_ERR_INVALID_PTR, func, RIGHT_HERE);
		return(UTLCOMM_ERR_INVALID_PTR);
	}

/*	Make sure the ptr to the returned message size is valid.
	-------------------------------------------------------- */
	if (pSizeMsg == NULL) {
		UtlErr_Write(UTLCOMM_ERR_INVALID_PTR, func, RIGHT_HERE);
		return(UTLCOMM_ERR_INVALID_PTR);
	}

/*	Allow for tracing information on EXIT.
	-------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");

/*	Everything checked out just fine, so indicated a success
	to the caller.
	-------------------------------------------------------- */
	return(UTL_SUCCESS);
}
