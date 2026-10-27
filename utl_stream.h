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
**		utl_stream.h
**
** Description:
**		This is the header file for the "utl_stream" module of the UTL
**		subsystem, which is a collection of general-purpose utility 
**		functions that are used by all other subsystems.  This module
**		contains library functions related to packing and unpacking a
**		variable list of arguments of different types into a stream
**		of bytes.  An application might want to invoke these functions
**		as an aid in sending variable arguments from one process to
**		another, where the communication of the information is done
**		using either TCP/IP or UDP (using, of course, the UtlComm_XXX
**		functions of the UTL subsystem).
**
** Subsystem:
**      UTL
**
** Version:
**      1.00
**
** Public Functions:
**		UtlStream_DumpPack
**		UtlStream_DumpUnpack
**		UtlStream_ExtractArg
**		UtlStream_FreePack
**		UtlStream_FreeUnpack
**		UtlStream_PackArgs
**		UtlStream_PackArgsV
**		UtlStream_PackMsg
**		UtlStream_PackMsgV
**		UtlStream_UnpackArgs
**		UtlStream_UnpackMsg
**
** History:
**		Date		Engineer			Purpose
**		--------	----------------	---------------------------------
**		09/23/96	Steven M. Rosen		Initial creation.
*/

/* Include Guard START */
/* ------------------- */
#ifndef _UTL_STREAM_H_INCLUDED

/* Include Files */
/* ------------- */
#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <WINSOCK.H>

/* Public Definitions */
/* ------------------ */
#define	MAX_PACKED_ARGS		100			/* max number of packed arguments	*/
#define	MAX_SIZE_STREAM		4096		/* max size of a dynamic stream		*/
#define	MAX_SIZE_ASCII		256			/* max size of an ASCII string		*/

/* Types of Variables */
/* ------------------ */
typedef enum {
	VTYPE_ASCII,		/* type of variable is an ASCII string		*/
	VTYPE_CHAR,			/* type of variable is a CHAR				*/
	VTYPE_UCHAR,		/* type of variable is an UNSIGNED CHAR		*/
	VTYPE_SHORT,		/* type of variable is a SHORT				*/
	VTYPE_INT,			/* type of variable is an INTEGER			*/
	VTYPE_FLOAT			/* type of variable is a FLOAT				*/
} VAR_TYPES;

/* Public Structures */
/* ----------------- */

/* Public Global Variables */
/* ----------------------- */

/* Public Prototype Definitions */
/* ---------------------------- */
int UtlStream_DumpPack(
	int		szStream,			/* size of the packed stream to dump		*/
	char*	pStream);			/* ptr to the packed stream to dump			*/

int	UtlStream_DumpUnpack(
	int		numArgs,			/* number of args in variable arg list		*/
	short*	pArgTypes,			/* ptr to array of arg types to dump		*/
	char**	ppArgValues);		/* ptr to array of arg value ptrs to dump	*/

int	UtlStream_ExtractArg(
	int		numArgs,		/* number of args in variable arg list		*/
	short*	pArgTypes,		/* ptr to array of arg types 				*/
	char**	ppArgValues,	/* ptr to array of arg values 				*/
	int		ndxArg,			/* index of the argument to extract			*/
	short	argType,		/* expected type of the argument to extract	*/
	char**	ppArgValue);	/* ptr to the returned argument	(ptr)		*/

int UtlStream_FreePack(
	int		szStream,			/* size of the packed stream to free		*/
	char*	pStream);			/* ptr to the packed stream to free			*/

int	UtlStream_FreeUnpack(
	int		numArgs,			/* number of args in variable arg list		*/
	short*	pArgTypes,			/* ptr to array of arg types to free		*/
	char**	ppArgValues);		/* ptr to array of arg values to free		*/

int	UtlStream_PackArgs(
	int		useHostOrder,		/* use host ordering of bytes indicator		*/
	int		numArgs,			/* number of args in variable arg list		*/
	short*	pArgTypes,			/* ptr to array of argument types			*/
	int*	pSzStream,			/* ptr to returned size of the stream		*/
	char**	ppStream,			/* ptr to returned ptr to the stream		*/
	...);						/* the variable argument list to pack		*/

int	UtlStream_PackArgsV(
	int		useHostOrder,		/* use host ordering of bytes indicator		*/
	int		numArgs,			/* number of args in variable arg list		*/
	short*	pArgTypes,			/* ptr to array of argument types			*/
	int*	pSzStream,			/* ptr to returned size of the stream		*/
	char**	ppStream,			/* ptr to returned ptr to the stream		*/
	va_list	pArgs);				/* the variable argument list to pack		*/

int	UtlStream_PackMsg(
	int		useHostOrder,		/* use host ordering of bytes indicator		*/
	int		numArgs,			/* number of args in variable arg list		*/
	short*	pArgTypes,			/* ptr to array of argument types			*/
	int		ndxLengthArg,		/* the array index of the LENGTH arg		*/
	int		ndxFirstBodyArg,	/* the array index of the first BODY arg	*/
	int*	pSzStream,			/* ptr to returned size of the stream		*/
	char**	ppStream,			/* ptr to returned ptr to the stream		*/
	...);						/* the variable argument list to pack		*/
int	UtlStream_PackMsgV(
	int		useHostOrder,		/* use host ordering of bytes indicator		*/
	int		numArgs,			/* number of args in variable arg list		*/
	short*	pArgTypes,			/* ptr to array of argument types			*/
	int		ndxLengthArg,		/* the array index of the LENGTH arg		*/
	int		ndxFirstBodyArg,	/* the array index of the first BODY arg	*/
	int*	pSzStream,			/* ptr to returned size of the stream		*/
	char**	ppStream,			/* ptr to returned ptr to the stream		*/
	va_list	pArgs);				/* ptr to the variable arg list to pack		*/

int	UtlStream_UnpackArgs(
	int		useHostOrder,		/* use host ordering of bytes indicator		*/
	int		szStream,			/* size of the stream to unpack				*/
	char*	pStream,			/* ptr to the stream to unpack				*/
	int*	pNumArgs,			/* ptr to returned number of args			*/
	short**	ppArgTypes,			/* ptr to returned array of arg types		*/
	char***	pppArgValues);		/* ptr to returned array of arg value ptrs	*/

int	UtlStream_UnpackMsg(
	int		useHostOrder,		/* use host ordering of bytes indicator		*/
	int		szStream,			/* size of the stream to unpack				*/
	char*	pStream,			/* ptr to the stream to unpack				*/
	int		ndxLengthArg,		/* the array index of the LENGTH arg		*/
	int		ndxFirstBodyArg,	/* the array index of the first BODY arg	*/
	int*	pLengthBody,		/* ptr to returned length of msg body		*/
	int*	pLengthHeader,		/* ptr to returned length of msg header		*/
	int*	pNumArgs,			/* ptr to returned number of args 			*/
	short**	ppArgTypes,			/* ptr to returned array of arg types		*/
	char***	pppArgValues);		/* ptr to returned array of arg value ptrs	*/

/* Include Guard END */
/* ----------------- */
#define _UTL_STREAM_H_INCLUDED
#endif
