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
**		utl_comm.h
**
** Description:
**		This is the header file for the "utl_comm" module of the UTL
**		subsystem, which is a collection of general-purpose utility 
**		functions that are used by all other subsystems.  This module
**		contains library functions related to performing basic
**		communications between processes using either TCP/IP or UDP.
**
** Subsystem:
**      UTL
**
** Version:
**      1.00
**
** Public Functions:
**		UtlComm_Accept
**		UtlComm_Close
**		UtlComm_ConnectPending
**		UtlComm_GetSockIO
**		UtlComm_Initialize
**		UtlComm_InputPending
**		UtlComm_Open
**		UtlComm_Read
**		UtlComm_ReadBlock
**		UtlComm_Terminate
**		UtlComm_Write
**
** History:
**		Date		Engineer			Purpose
**		--------	----------------	---------------------------------
**		06/02/97	Richard D. Klein	Added UtlComm_GetSockIO.
**		02/26/96	Steven M. Rosen		Initial creation.
*/

/* Include Guard START */
/* ------------------- */
#ifndef _UTL_COMM_H_INCLUDED

/* Special Definitions */
/* ------------------- */

/* Include Files */
/* ------------- */
#include <sys/types.h>
#include <time.h>
#include <WINSOCK.H>
#include "utl_err.h"

/* Public Definitions */
/* ------------------ */

/* the different types of protocol for communication */
/* ------------------------------------------------- */
#define	UDP_COMM		(1 << 0)	/* comm protocol is UDP */
#define	TCP_COMM		(1 << 1)	/* comm protocol is TCP */

/* the different types of applications for communication */
/* ----------------------------------------------------- */
#define CLIENT_APP		(1 << 2) 	/* the application is a CLIENT */
#define SERVER_APP		(1 << 3)	/* the application is a SERVER */

/* various maximum and minimum constants */
/* ------------------------------------- */
#define	MAX_CHANNELS		256		/* max number of communication channels	*/
#define	MAX_CONNECTIONS		32		/* max number of concurrent connections	*/
#define	MAX_NAME_LEN		32		/* max length of a name					*/
#define	MAX_RECV_TIMEOUTS	3		/* max number of timeouts for RECV		*/
#define	MAX_SEND_TIMEOUTS	3		/* max number of timeouts for SEND		*/

/* Public Structures */
/* ----------------- */

/* the message header descriptor */
/* ----------------------------- */
typedef struct {
	int		sizeMsgHead;	/* the size (in bytes) of the msg header	*/
	int		offBodyLen;		/* the offset (in bytes) to the body length */
	int		sizeBodyLen;	/* the size (in bytes) of the body length	*/
	int		useHostOrder;	/* the use host-ordering of bytes flag		*/
} UTLCOMM_HDR_DESC;

/* the all-purpose communication channel ID descriptor */
/* --------------------------------------------------- */
typedef struct {
	int					typeComm;		/* the type of communication		*/
	int					typeApp;		/* the type of application			*/
	char*				pNameHost;		/* ptr to name of the host			*/
	char*				pNameService;	/* ptr to name of the service		*/
	UTLCOMM_HDR_DESC*	pHeaderInfo;	/* ptr to the msg header info		*/
	int					sockIO;			/* the socket for I/O				*/
	int					sockListen;		/* the socket for listening			*/
	int					isMaster;		/* this comm channel is a MASTER	*/
	int					maxConnections;	/* the max number of connections	*/
	struct sockaddr_in	addrSockServer;	/* the SERVER address info			*/
	struct sockaddr_in	addrSockClient;	/* the CLIENT address info			*/
} UTLCOMM_CHAN_DESC;

/* the block read communication channel IDs descriptor */
/* --------------------------------------------------- */
typedef struct {
	int					sizeList;				/* size of the list		*/
	UTLCOMM_CHAN_DESC*	theList[MAX_CHANNELS];	/* comm channel ID list	*/
} UTLCOMM_CHANLIST_DESC;

/* Public Global Variables */
/* ----------------------- */

/* Public Prototype Definitions */
/* ---------------------------- */

long	UtlComm_Accept(
							UTLCOMM_CHAN_DESC*		pMasterChanId,
							UTLCOMM_CHAN_DESC**		ppChanId		);
long	UtlComm_Close(
							UTLCOMM_CHAN_DESC*		pChanId			);
long	UtlComm_ConnectPending(
							UTLCOMM_CHAN_DESC*		pChanId,
							struct timeval*			pTimeOut,
							int*					pConnectPending	);
long	UtlComm_GetSockIO(
							UTLCOMM_CHAN_DESC*		pChanId,
							int*					pSockIO			);
long	UtlComm_Initialize(
																	);
long	UtlComm_InputPending(
							UTLCOMM_CHANLIST_DESC*	pListChanIds,
							struct timeval*			pTimeOut,
							int*					pInputPending,
							UTLCOMM_CHAN_DESC**		ppChanId		);
long	UtlComm_Open(
							int						typeComm,
							int						typeApp,
							int						maxConnections,
							struct timeval*			pConnTimeout,
							char*					nameHost,
							char*					nameService,
							UTLCOMM_HDR_DESC*		pHeaderInfo,
							UTLCOMM_CHAN_DESC**		ppChanId		);
long	UtlComm_Read(
							UTLCOMM_CHAN_DESC*		pChanId,
							int						sizeMsgMax,
							struct timeval*			pTimeOut,
							int						(*pFuncCB)(),
							void*					pDataCB,
							struct timeval*			pTimeOutCB,
							char*					pMsg,
							int*					pSizeMsg		);
long	UtlComm_ReadBlock(
							UTLCOMM_CHANLIST_DESC*	pListChanIds,
							int						sizeMsgMax,
							struct timeval*			pTimeOut,
							int						(*pFuncCB)(),
							void*					pDataCB,
							struct timeval*			pTimeOutCB,
							char*					pMsg,
							int*					pSizeMsg,
							UTLCOMM_CHAN_DESC**		ppChanId		);
long	UtlComm_Terminate(
																	);
long	UtlComm_Write(
							UTLCOMM_CHAN_DESC*		pChanId,
							int						sizeMsgTotal,
							struct timeval*			pTimeOut,
							int						(*pFuncCB)(),
							void*					pDataCB,
							struct timeval*			pTimeOutCB,
							char*					pMsg,
							int*					pSizeMsg			);

/* Include Guard END */
/* ----------------- */
#define _UTL_COMM_H_INCLUDED
#endif
