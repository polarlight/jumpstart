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
**		utl_stream.c
**
** Description:
**		This is the implementation file for the "utl_stream" module of the
**		UTL subsystem, which is a collection of general-purpose utility 
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

/* include files */
/* ------------- */
#include <sys/types.h>
#include <WINSOCK.H>
#include "utl_dbg.h"
#include "utl_dump.h"
#include "utl_log.h"
#include "utl_stream.h"

/* private definitions */
/* ------------------- */

/* private structures */
/* ------------------- */

/* private global variables */
/* ------------------------ */

/* private prototype declarations */
/* ------------------------------ */
static int	pack_arg_ascii(		char* pArgAscii, int useHostOrder,
								int* pSzStream, char* pStream			);
static int	pack_arg_char(		char argChar, int useHostOrder,
								int* pSzStream,	char* pStream			);
static int	pack_arg_float(		float argFloat, int useHostOrder,
								int* pSzStream,	char* pStream			);
static int	pack_arg_int(		int argInt, int useHostOrder,
								int* pSzStream,	char* pStream			);
static int	pack_arg_short(		short argShort, int useHostOrder,
								int* pSzStream,	char* pStream			);
static int	pack_arg_uchar(		unsigned char argUchar, int useHostOrder,
								int* pSzStream,	char* pStream			);
static int	pack_type(			short argType, int useHostOrder,
								int* pSzStream, char* pStream			);

static int	stream_pack_args_var(
								int useHostOrder, int numArgs,
								short* pArgTypes, int* pSzStream,
								char** ppStream, va_list pArgs			);
static int	stream_pack_length(
								int useHostOrder, short typeLength,
								int theLength, char* pStream			);			 
static int	stream_pack_msg_var(
								int useHostOrder, int numArgs,
								short* pArgTypes, int ndxLengthArg,
								int ndxFirstBodyArg, int* pSzStream,
								char** ppStream, va_list pArgs			);

static int	unpack_arg_ascii(	char* pStream, int useHostOrder,
								int* pCurrByte, int* pNumArgs,
								short* pArgTypes, char** ppArgValues	);
static int	unpack_arg_char(	char* pStream, int useHostOrder,
								int* pCurrByte, int* pNumArgs,
								short* pArgTypes, char** ppArgValues	);
static int	unpack_arg_float(	char* pStream, int useHostOrder,
								int* pCurrByte, int* pNumArgs,
								short* pArgTypes, char** ppArgValues	);
static int	unpack_arg_int(		char* pStream, int useHostOrder,
								int* pCurrByte, int* pNumArgs,
								short* pArgTypes, char** ppArgValues	);
static int	unpack_arg_short(	char* pStream, int useHostOrder,
								int* pCurrByte, int* pNumArgs,
								short* pArgTypes, char** ppArgValues	);
static int	unpack_arg_uchar(	char* pStream, int useHostOrder,
								int* pCurrByte, int* pNumArgs,
								short* pArgTypes, char** ppArgValues	);
static int	unpack_type(		char* pStream, int useHostOrder,
								int* pCurrByte, short* pArgType			);

/*
** Name:
**		UtlStream_DumpPack
**
** Purpose:
**		Dumps the memory associated with a stream that was created
**		using either the "UtlStream_PackArgs", "UtlStream_PackArgsV",
**		"UtlStream_PackMsg", or "UtlStream_PackMsgV" function.
**
** Calling Sequence:
**		isOk = UtlStream_DumpPack(szStream, pStream);
**
** Input:
**		szStream		size of the packed stream to dump
**		pStream			ptr to the packed stream to dump
**
** Output:
**		NONE
**
** Returns:
**		isOk			BOOLEAN indicating whether or not the
**						function executed successfully.
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int UtlStream_DumpPack(
	int		szStream,		/* size of the packed stream to dump	*/
	char*	pStream)		/* ptr to the packed stream to dump		*/
{
	int		isOk = FALSE;			/* return value of this function	*/
	char*	func = "UtlStream_DumpPack"; 	/* name of this function	*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Dump the memory associated with this packed stream.
	--------------------------------------------------- */
	UtlDump_Mem(pStream, szStream, LOG_DEBUG);

/*	Indicate that everything went okay.
	----------------------------------- */
	isOk = TRUE;

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOk);
}

/*
** Name:
**		UtlStream_DumpUnpack
**
** Purpose:
**		Dumps the memory associated with a variable list of arguments
**		and their associated types that was created using either the
**		"UtlStream_UnpackArgs" or "UtlStream_UnpackMsg" function.
**
** Calling Sequence:
**		isOk = UtlStream_DumpUnpack(numArgs, pArgTypes, ppArgValues);
**
** Input:
**		numArgs			number of args in variable arg list
**		pArgTypes		ptr to array of arg types to dump
**		ppArgValues		ptr to array of arg values to dump
**
** Output:
**		NONE
**
** Returns:
**		isOk			BOOLEAN indicating whether or not the
**						function executed successfully.
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int	UtlStream_DumpUnpack(
	int		numArgs,		/* number of args in variable arg list	*/
	short*	pArgTypes,		/* ptr to array of arg types to dump	*/
	char**	ppArgValues)	/* ptr to array of arg values to dump	*/
{
	int				isOk = FALSE;	/* return value of this function	*/
	int				thisArg;		/* the current argument to dump		*/
	char*			pArgAscii;		/* a ptr to an ASCII argument		*/
	char			argChar;		/* a CHAR argument					*/
	unsigned char	argUchar;		/* an UNSIGNED CHAR argument		*/
	short			argShort;		/* a SHORT argument					*/
	int				argInt;			/* an INTEGER argument				*/
	float			argFloat;		/* a FLOATING point argument		*/
	char*	func = "UtlStream_DumpUnpack";	/* name of this function	*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Allocate some temporary space for any ASCII arguments.
	------------------------------------------------------ */
	pArgAscii = (char*)malloc(MAX_SIZE_ASCII);
	if (pArgAscii == NULL) goto _errAllocArgAscii;

/*	Dump the memory associated with the arg types of the unpacked stream.
	--------------------------------------------------------------------- */
	UtlLog_Write(LOG_DEBUG, func, RIGHT_HERE,
		"Dumping %d arguments from an unpacked stream...\n", numArgs);
	for (thisArg=0; thisArg<numArgs; thisArg++) {
		switch ( *(pArgTypes + thisArg) ) {
			case VTYPE_ASCII:
				if ( strlen(*(ppArgValues + thisArg)) > MAX_SIZE_ASCII ) {
					UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
						"ERROR: Size of ASCII string (%d) exceeds max (%d)\n",
							strlen(*(ppArgValues + thisArg)), MAX_SIZE_ASCII);
					goto _errStringTooBig;
				}
				memset(pArgAscii, 0, MAX_SIZE_ASCII);
				memcpy(pArgAscii, *(ppArgValues + thisArg),
					   (strlen(*(ppArgValues + thisArg)) + 1) );
				UtlLog_Write(LOG_DEBUG | LOG_NO_ID, func, RIGHT_HERE,
					"...got an ASCII arg = \"%s\"\n", pArgAscii);
				break;
			case VTYPE_CHAR:
				memcpy(&argChar, *(ppArgValues + thisArg), sizeof(char));
				UtlLog_Write(LOG_DEBUG | LOG_NO_ID, func, RIGHT_HERE,
					"...got a CHAR arg   = \"%c\"\n", argChar);
				break;
			case VTYPE_FLOAT:
				memcpy(&argFloat, *(ppArgValues + thisArg), sizeof(float));
				UtlLog_Write(LOG_DEBUG | LOG_NO_ID, func, RIGHT_HERE,
					"...got a FLOAT arg  = %f\n", argFloat);
				break;
			case VTYPE_INT:
				memcpy(&argInt, *(ppArgValues + thisArg), sizeof(int));
				UtlLog_Write(LOG_DEBUG | LOG_NO_ID, func, RIGHT_HERE,
					"...got an INT arg   = %d\n", argInt);
				break;
			case VTYPE_SHORT:
				memcpy(&argShort, *(ppArgValues + thisArg), sizeof(short));
				UtlLog_Write(LOG_DEBUG | LOG_NO_ID, func, RIGHT_HERE,
					"...got a SHORT arg  = %d\n", argShort);
				break;
			case VTYPE_UCHAR:
				memcpy(&argUchar, *(ppArgValues + thisArg),
						sizeof(unsigned char));
				UtlLog_Write(LOG_DEBUG | LOG_NO_ID, func, RIGHT_HERE,
					"...got a UCHAR arg  = %d\n", argUchar);
				break;
			default:
				UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
					"ERROR: Unknown argument type = %d\n",
						*(pArgTypes + thisArg) );
				goto _errUnknownArgType;
				break;
		}
	}

/*	Indicate that everything went okay.
	----------------------------------- */
	isOk = TRUE;

/*	Free the temporary space allocated for ASCII arguments.
	------------------------------------------------------- */
_errStringTooBig:
	;
_errUnknownArgType:
	free(pArgAscii);
_errAllocArgAscii:
	;

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOk);
}

/*
** Name:
**		UtlStream_ExtractArg
**
** Purpose:
**		Extracts a specified argument that is associated with a variable
**		list of arguments and their associated types that was created
**		using either the "UtlStream_UnpackArgs" or "UtlStream_UnpackMsg"
**		function.
**
** Calling Sequence:
**		isOk = UtlStream_ExtractArg(numArgs, pArgTypes, ppArgValues,
**									ndxArg, argType, ppArgValue);
**
** Input:
**		numArgs			number of args in variable arg list
**		pArgTypes		ptr to array of arg types
**		ppArgValues		ptr to array of arg values
**		ndxArg			index of the argument to extract
**		argType			expected type of the argument to extract
**
** Output:
**		ppArgValue		ptr to the returned argument (ptr)
**
** Returns:
**		isOk			BOOLEAN indicating whether or not the
**						function executed successfully.
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int	UtlStream_ExtractArg(
	int		numArgs,		/* number of args in variable arg list		*/
	short*	pArgTypes,		/* ptr to array of arg types 				*/
	char**	ppArgValues,	/* ptr to array of arg values 				*/
	int		ndxArg,			/* index of the argument to extract			*/
	short	argType,		/* expected type of the argument to extract	*/
	char**	ppArgValue)		/* ptr to the returned argument	(ptr)		*/
{
	int				isOk = FALSE;	/* return value of this function	*/
	char*	func = "UtlStream_ExtractArg";	/* name of this function	*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Perform some sanity checking by verifying that the index of the
	argument to extract and its expected type are correct.
	--------------------------------------------------------------- */
	if ( ndxArg >= numArgs ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: The arg index (%d) is out of bounds of max (%d)\n",
				ndxArg, numArgs);
		goto _errArgIndexOutOfBounds;
	}

	if ( argType != (*(pArgTypes + ndxArg)) ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: The arg type (%d) does not match the actual type (%d)\n",
				argType, (*(pArgTypes + ndxArg)) );
		goto _errArgTypeMismatch;
	}

/*	Extract out the desired argument of the correct type from the array
	of arg values supplied by the caller.
	------------------------------------------------------------------- */
	switch ( *(pArgTypes + ndxArg) ) {
		case VTYPE_ASCII:
			*ppArgValue = *(ppArgValues + ndxArg);
			break;
		case VTYPE_CHAR:
			memcpy(ppArgValue, *(ppArgValues + ndxArg), sizeof(char));
			break;
		case VTYPE_FLOAT:
			memcpy(ppArgValue, *(ppArgValues + ndxArg), sizeof(float));
			break;
		case VTYPE_INT:
			memcpy(ppArgValue, *(ppArgValues + ndxArg), sizeof(int));
			break;
		case VTYPE_SHORT:
			memcpy(ppArgValue, *(ppArgValues + ndxArg), sizeof(short));
			break;
		case VTYPE_UCHAR:
			memcpy(ppArgValue, *(ppArgValues + ndxArg), sizeof(unsigned char));
			break;
		default:
			UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
				"ERROR: Unknown argument type = %d\n", *(pArgTypes + ndxArg));
			goto _errUnknownArgType;
			break;
	}

/*	Indicate that everything went okay.
	----------------------------------- */
	isOk = TRUE;

/*	Problems in extracting the argument
	------------------------------------------------------- */
_errArgTypeMismatch:
	;
_errArgIndexOutOfBounds:
	;
_errUnknownArgType:
	;

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOk);
}

/*
** Name:
**		UtlStream_FreePack
**
** Purpose:
**		Frees the memory associated with a stream that was created
**		using either the "UtlStream_PackArgs", "UtlStream_PackArgsV",
**		"UtlStream_PackMsg", or "UtlStream_PackMsgV" function.
**
** Calling Sequence:
**		isOk = UtlStream_FreePack(szStream, pStream);
**
** Input:
**		szStream		size of the packed stream to free
**		pStream			ptr to the packed stream to free
**
** Output:
**		NONE
**
** Returns:
**		isOk			BOOLEAN indicating whether or not the
**						function executed successfully.
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int UtlStream_FreePack(
	int		szStream,		/* size of the packed stream to free	*/
	char*	pStream)		/* ptr to the packed stream to free		*/
{
	int		isOk = FALSE;			/* return value of this function	*/
	char*	func = "UtlStream_FreePack"; 	/* name of this function	*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Free the memory associated with this packed stream.
	--------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE,
		"Freeing a packed stream of %d bytes at addr %p\n",
			szStream, pStream);
	if (pStream) {
		free(pStream);
	} else {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Could not free invalid ptr to packed stream\n");
		goto _errInvalidPtr;
	}

/*	Indicate that everything went okay.
	----------------------------------- */
	isOk = TRUE;

/*	Invalid memory was trying to be freed.
	-------------------------------------- */
_errInvalidPtr:
	;

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOk);
}

/*
** Name:
**		UtlStream_FreeUnpack
**
** Purpose:
**		Frees the memory associated with a variable list of arguments
**		and their associated types that was created using either the
**		"UtlStream_UnpackArgs" or "UtlStream_UnpackMsg" function.
**
** Calling Sequence:
**		isOk = UtlStream_FreeUnpack(numArgs, pArgTypes, ppArgValues);
**
** Input:
**		numArgs			number of args in variable arg list
**		pArgTypes		ptr to array of arg types to free
**		ppArgValues		ptr to array of arg values to free
**
** Output:
**		NONE
**
** Returns:
**		isOk			BOOLEAN indicating whether or not the
**						function executed successfully.
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int	UtlStream_FreeUnpack(
	int		numArgs,		/* number of args in variable arg list	*/
	short*	pArgTypes,		/* ptr to array of arg types to free	*/
	char**	ppArgValues)	/* ptr to array of arg values to free	*/
{
	int		thisArg;				/* index of this arg in the array	*/
	int		isOk = FALSE;			/* return value of this function	*/
	char*	func = "UtlStream_FreeUnpack";	/* name of this function	*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Free the memory associated with the arg types of the unpacked stream.
	--------------------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE,
		"Freeing ptr to array of types of unpacked stream of %d args at %p\n",
			numArgs, pArgTypes);
	if (pArgTypes) {
		free(pArgTypes);
	} else {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Could not free invalid ptr to array of arg types\n");
		goto _errInvalidPtr;
	}

/*	Free the memory associated with each arg value of the unpacked stream.
	---------------------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE | LOG_NO_ID, func, RIGHT_HERE,
		"Freeing %d arg value ptrs of unpacked stream at addr %p...\n",
			numArgs, ppArgValues);
	if (ppArgValues) {
		for (thisArg=0; thisArg<numArgs; thisArg++) {
			UtlLog_Write(LOG_TRACE | LOG_NO_ID, func, RIGHT_HERE,
				"...freeing arg value %d at addr %p\n",
					thisArg, *(ppArgValues + thisArg) );
			if ( *(ppArgValues + thisArg) ) {
				free( *(ppArgValues + thisArg) );
			} else {
				UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
					"ERROR: Could not free invalid ptr to arg value %d\n",
						thisArg);
				goto _errInvalidPtr;
			}
		}
	} else {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Invalid ptr to array of arg values\n");
		goto _errInvalidPtr;
	}

/*	Free the memory associated with the arg values of the unpacked stream.
	---------------------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE | LOG_NO_ID, func, RIGHT_HERE,
		"Freeing array of arg value ptrs at addr %p\n", ppArgValues);
	if (ppArgValues) {
		free(ppArgValues);
	} else {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Could not free invalid ptr to array of arg values\n");
		goto _errInvalidPtr;
	}

/*	Indicate that everything went okay.
	----------------------------------- */
	isOk = TRUE;

/*	Invalid memory was trying to be freed.
	-------------------------------------- */
_errInvalidPtr:
	;

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOk);
}

/*
** Name:
**		UtlStream_PackArgs
**
** Purpose:
**		Packs a variable list of arguments into a stream that is
**		suitable for sending from one process to another (Typically,
**		a client process to a server process).
**
** Calling Sequence:
**		isOk = UtlStream_PackArgs(useHostOrder, numArgs, pArgTypes,
**								  pSzStream, ppStream, ...);
**
** Input:
**		useHostOrder	use host ordering of bytes indicator
**		numArgs			number of args in variable arg list
**		pArgTypes		ptr to array of argument types
**		...				the variable argument list to pack
**
** Output:
**		pSzStream		ptr to returned size of the stream
**		ppStream		ptr to returned ptr to the stream
**
** Returns:
**		isOk			BOOLEAN indicating whether or not the
**						function executed successfully.
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int	UtlStream_PackArgs(
	int		useHostOrder,	/* use host ordering of bytes indicator	*/
	int		numArgs,		/* number of args in variable arg list	*/
	short*	pArgTypes,		/* ptr to array of argument types		*/
	int*	pSzStream,		/* ptr to returned size of the stream	*/
	char**	ppStream,		/* ptr to returned ptr to the stream	*/
	...)					/* the variable argument list to pack	*/
{
	int		isOk = FALSE;			/* return value of this function	*/
	va_list	pArgs;					/* ptr to the variable arg list		*/
	char*	func = "UtlStream_PackArgs"; 	/* name of this function	*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Indicate we are about to pack this stream.
	------------------------------------------ */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE,
		"Packing a stream of %d arguments...\n", numArgs);

/*	Initialize the variable argument list pointer.
	---------------------------------------------- */
	va_start(pArgs, ppStream);

/*	Get the variable argument list packed into a stream.
	---------------------------------------------------- */
	isOk = stream_pack_args_var(useHostOrder, numArgs, pArgTypes,
								pSzStream, ppStream, pArgs);

/*	Reset the variable argument list pointer.
	----------------------------------------- */
	va_end(pArgs);

/*	Indicate we are done packing this stream.
	----------------------------------------- */
	UtlLog_Write(LOG_TRACE | LOG_NO_ID, func, RIGHT_HERE,
		"Done packing a stream of %d arguments\n", numArgs);

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOk);
}

/*
** Name:
**		UtlStream_PackArgsV
**
** Purpose:
**		Packs a variable list of arguments into a stream that is
**		suitable for sending from one process to another (Typically,
**		a client process to a server process).
**
**		This function is similar to "UtlStream_PackArgs", but the variable
**		list of arguments has ALREADY been put into a "va_list".  In other
**		words, this function is to "UtlStream_PackArgs" as "vsprintf" is
**		to "sprintf".  To make it clearer, this function is intended to
**		be called from another function that accepts a variable list of
**		arguments.  It is assumed that the caller first calls "va_start",
**		followed by a call to this function, and finally a call to "va_end"
**		(This is exactly how "vsprintf" is used).
**
** Calling Sequence:
**		isOk = UtlStream_PackArgsV(useHostOrder, numArgs, pArgTypes,
**								   pSzStream, ppStream, pArgs);
**
** Input:
**		useHostOrder	use host ordering of bytes indicator
**		numArgs			number of args in variable arg list
**		pArgTypes		ptr to array of argument types
**		pArgs			ptr to the variable arg list to pack
**
** Output:
**		pSzStream		ptr to returned size of the stream
**		ppStream		ptr to returned ptr to the stream
**
** Returns:
**		isOk			BOOLEAN indicating whether or not the
**						function executed successfully.
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int	UtlStream_PackArgsV(
	int		useHostOrder,	/* use host ordering of bytes indicator	*/
	int		numArgs,		/* number of args in variable arg list	*/
	short*	pArgTypes,		/* ptr to array of argument types		*/
	int*	pSzStream,		/* ptr to returned size of the stream	*/
	char**	ppStream,		/* ptr to returned ptr to the stream	*/
	va_list	pArgs)			/* ptr to the variable arg list to pack	*/
{
	int		isOk = FALSE;			/* return value of this function	*/
	char*	func = "UtlStream_PackArgsV"; 	/* name of this function	*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Get the variable argument list packed into a stream.
	---------------------------------------------------- */
	isOk = stream_pack_args_var(useHostOrder, numArgs, pArgTypes,
								pSzStream, ppStream, pArgs);

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOk);
}

/*
** Name:
**		UtlStream_PackMsg
**
** Purpose:
**		Packs a complete message (which is passed as a variable list of
**		arguments) into a stream that is suitable for sending from one
**		process to another (Typically, a client process to a server process).
**
** Calling Sequence:
**		isOk = UtlStream_PackMsg(useHostOrder, numArgs, pArgTypes,
**								 ndxLengthArg, ndxFirstBodyArg,
**								 pSzStream, ppStream, ...);
**
** Input:
**		useHostOrder	use host ordering of bytes indicator
**		numArgs			number of args in variable arg list
**		pArgTypes		ptr to array of argument types
**		ndxLengthArg	the array index of the LENGTH argument
**		ndxFirstBodyArg	the array index of the first BODY argument
**		...				the variable argument list to pack
**
** Output:
**		pSzStream		ptr to returned size of the stream
**		ppStream		ptr to returned ptr to the stream
**
** Returns:
**		isOk			BOOLEAN indicating whether or not the
**						function executed successfully.
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int	UtlStream_PackMsg(
	int		useHostOrder,		/* use host ordering of bytes indicator		*/
	int		numArgs,			/* number of args in variable arg list		*/
	short*	pArgTypes,			/* ptr to array of argument types			*/
	int		ndxLengthArg,		/* the array index of the LENGTH arg		*/
	int		ndxFirstBodyArg,	/* the array index of the first BODY arg	*/
	int*	pSzStream,			/* ptr to returned size of the stream		*/
	char**	ppStream,			/* ptr to returned ptr to the stream		*/
	...)						/* the variable argument list to pack		*/
{
	int		isOk = FALSE;			/* return value of this function	*/
	va_list	pArgs;					/* ptr to the variable arg list		*/
	char*	func = "UtlStream_PackMsg"; 	/* name of this function	*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Indicate we are about to pack this message stream.
	-------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE,
		"Packing a message stream of %d arguments...\n", numArgs);

/*	Initialize the variable argument list pointer.
	---------------------------------------------- */
	va_start(pArgs, ppStream);

/*	Get the variable argument list packed into a stream.
	---------------------------------------------------- */
	isOk = stream_pack_msg_var(useHostOrder, numArgs, pArgTypes,
							   ndxLengthArg, ndxFirstBodyArg,
							   pSzStream, ppStream, pArgs);

/*	Reset the variable argument list pointer.
	----------------------------------------- */
	va_end(pArgs);

/*	Indicate we are done packing this message stream.
	------------------------------------------------- */
	UtlLog_Write(LOG_TRACE | LOG_NO_ID, func, RIGHT_HERE,
		"Done packing a message stream of %d arguments\n", numArgs);

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOk);
}

/*
** Name:
**		UtlStream_PackMsgV
**
** Purpose:
**		Packs a complete message (which is passed as a variable list of
**		arguments) into a stream that is suitable for sending from one
**		process to another (Typically, a client process to a server process).
**
**		This function is similar to "UtlStream_PackMsg", but the variable
**		list of arguments has ALREADY been put into a "va_list".  In other
**		words, this function is to "UtlStream_PackMsg" as "vsprintf" is
**		to "sprintf".  To make it clearer, this function is intended to
**		be called from another function that accepts a variable list of
**		arguments.  It is assumed that the caller first calls "va_start",
**		followed by a call to this function, and finally a call to "va_end"
**		(This is exactly how "vsprintf" is used).
**
** Calling Sequence:
**		isOk = UtlStream_PackMsgV(useHostOrder, numArgs, pArgTypes,
**							 	  ndxLengthArg, ndxFirstBodyArg,
**								  pSzStream, ppStream, pArgs);
**
** Input:
**		useHostOrder	use host ordering of bytes indicator
**		numArgs			number of args in variable arg list
**		pArgTypes		ptr to array of argument types
**		ndxLengthArg	the array index of the LENGTH argument
**		ndxFirstBodyArg	the array index of the first BODY argument
**		pArgs			ptr to the variable arg list to pack
**
** Output:
**		pSzStream		ptr to returned size of the stream
**		ppStream		ptr to returned ptr to the stream
**
** Returns:
**		isOk			BOOLEAN indicating whether or not the
**						function executed successfully.
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int	UtlStream_PackMsgV(
	int		useHostOrder,		/* use host ordering of bytes indicator		*/
	int		numArgs,			/* number of args in variable arg list		*/
	short*	pArgTypes,			/* ptr to array of argument types			*/
	int		ndxLengthArg,		/* the array index of the LENGTH arg		*/
	int		ndxFirstBodyArg,	/* the array index of the first BODY arg	*/
	int*	pSzStream,			/* ptr to returned size of the stream		*/
	char**	ppStream,			/* ptr to returned ptr to the stream		*/
	va_list	pArgs)				/* ptr to the variable arg list to pack		*/
{
	int		isOk = FALSE;			/* return value of this function	*/
	char*	func = "UtlStream_PackMsgV"; 	/* name of this function	*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Get the variable argument list packed into a stream.
	---------------------------------------------------- */
	isOk = stream_pack_msg_var(useHostOrder, numArgs, pArgTypes,
							   ndxLengthArg, ndxFirstBodyArg,
							   pSzStream, ppStream, pArgs);

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOk);
}

/*
** Name:
**		UtlStream_UnpackArgs
**
** Purpose:
**		Unpacks a variable number of arguments from a stream into
**		a  list of arguments with a corresponding list of associated
**		types.
**
** Calling Sequence:
**		isOk = UtlStream_UnpackArgs(useHostOrder, szStream, pStream,
**									pNumArgs, ppArgTypes, pppArgValues);
**
** Input:
**		useHostOrder	use host ordering of bytes indicator
**		szStream		size of the stream to unpack
**		pStream			ptr to the stream to unpack
**
** Output:
**		pNumArgs		ptr to returned number of args
**		ppArgTypes		ptr to returned array of arg types
**		pppArgValues	ptr to returned array of arg value ptrs
**
** Returns:
**		isOk			BOOLEAN indicating whether or not the
**						function executed successfully.
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int	UtlStream_UnpackArgs(
	int		useHostOrder,	/* use host ordering of bytes indicator		*/
	int		szStream,		/* size of the stream to unpack				*/
	char*	pStream,		/* ptr to the stream to unpack				*/
	int*	pNumArgs,		/* ptr to returned number of args 			*/
	short**	ppArgTypes,		/* ptr to returned array of arg types		*/
	char***	pppArgValues)	/* ptr to returned array of arg value ptrs	*/
{
	int		isOk = FALSE;			/* return value of this function	*/
	short*	pTempTypes;				/* ptr to temp array of arg types	*/
	char**	ppTempValues;			/* ptr to temp array of arg values	*/
	int		currByte;				/* the current byte in the stream	*/
	short	argType;				/* the type of argument to unpack	*/
	char*	func = "UtlStream_UnpackArgs"; 	/* name of this function	*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Initialize the returned number of arguments unpacked from the stream.
	--------------------------------------------------------------------- */
	*pNumArgs = 0;

/*	Allocate some temporary storage space for the array of arg types
	and array of arg values that we are about to unpack from the stream.
	-------------------------------------------------------------------- */
	pTempTypes = (short*)calloc(MAX_PACKED_ARGS, sizeof(short));
	if (pTempTypes == NULL) goto _errAllocTempTypes;

	ppTempValues = (char**)calloc(MAX_PACKED_ARGS, sizeof(char*));
	if (ppTempValues == NULL) goto _errAllocTempValues;

/*	Indicate we are about to unpack this stream.
	-------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE,
		"Unpacking a stream of %d bytes at addr %p...\n", szStream, pStream);

/*	Walk through the stream and unpack each argument until the
	stream has been completely gobbled up.  To unpack an argument 
	from the stream, the type of the argument is first unpacked
	to determine HOW to unpack the argument that follows the type
	of the argument.  By convention, the TYPE information is
	always packed into a VTYPE_SHORT (2 bytes).
	------------------------------------------------------------- */
	currByte = 0;
	while (currByte < szStream) {
		if ( !unpack_type(pStream, useHostOrder, &currByte, &argType) ) {
			goto _errUnpackType;
		}
		switch ( argType ) {

/*			This variable argument type is ASCII.
			------------------------------------- */
			case VTYPE_ASCII:
				if ( !unpack_arg_ascii(pStream, useHostOrder, &currByte,
									   pNumArgs, pTempTypes, ppTempValues) ) {
					goto _errUnpackArg;
				}
				break;

/*			This variable argument type is CHAR.
			------------------------------------ */
			case VTYPE_CHAR:
				if ( !unpack_arg_char(pStream, useHostOrder, &currByte,
									  pNumArgs, pTempTypes, ppTempValues) ) {
					goto _errUnpackArg;
				}
				break;

/*			This variable argument type is UNSIGNED CHAR.
			--------------------------------------------- */
			case VTYPE_UCHAR:
				if ( !unpack_arg_uchar(pStream, useHostOrder, &currByte,
									   pNumArgs, pTempTypes, ppTempValues) ) {
					goto _errUnpackArg;
				}
				break;

/*			This variable argument type is SHORT.
			------------------------------------- */
			case VTYPE_SHORT:
				if ( !unpack_arg_short(pStream, useHostOrder, &currByte,
									   pNumArgs, pTempTypes, ppTempValues) ) {
					goto _errUnpackArg;
				}
				break;

/*			This variable argument type is INT.
			----------------------------------- */
			case VTYPE_INT:
				if ( !unpack_arg_int(pStream, useHostOrder, &currByte,
								     pNumArgs, pTempTypes, ppTempValues) ) {
					goto _errUnpackArg;
				}
				break;

/*			This variable argument type is FLOAT.
			------------------------------------- */
			case VTYPE_FLOAT:
				if ( !unpack_arg_float(pStream, useHostOrder, &currByte,
									   pNumArgs, pTempTypes, ppTempValues) ) {
					goto _errUnpackArg;
				}
				break;

/*			This variable argument type is UNKNOWN.
			--------------------------------------- */
			default:
				UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
					"ERROR: Got an UNKNOWN argument type to unpack = %d\n",
						argType);
				goto _errUnknownArgType;
				break;
		}
	}

/*	We have finished unpacking the stream into the temporary array
	of argument types and the temporary array of argument values.
	Now get the final result copied into the REAL return output arrays.
	Remember to check for the case where there were NO arguments!
	------------------------------------------------------------------- */
	if (*pNumArgs > 0) {
		*ppArgTypes = (short*)calloc(*pNumArgs, sizeof(short));
		if (*ppArgTypes == NULL) goto _errAllocArgTypes;

		*pppArgValues = (char**)calloc(*pNumArgs, sizeof(char*));
		if (*pppArgValues == NULL) goto _errAllocArgValues;

		memcpy( *ppArgTypes, pTempTypes, (*pNumArgs * sizeof(short)) );
		memcpy( *pppArgValues, ppTempValues, (*pNumArgs * sizeof(char*)) );
	}

/*	Indicate we are done unpacking this stream and everything went okay.
	-------------------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE | LOG_NO_ID, func, RIGHT_HERE,
		"Done unpacking a stream of %d bytes at addr %p...\n",
			szStream, pStream);
	isOk = TRUE;
	goto _doCleanup;

/*	Only get rid of permanent storage if there was a problem.
	--------------------------------------------------------- */
_errAllocArgValues:
	free(*ppArgTypes);
_errAllocArgTypes:
	;

/*	Remember to get rid of the temporary arrays.
	-------------------------------------------- */
_doCleanup:
	;
_errUnknownArgType:
	;
_errUnpackArg:
	;
_errUnpackType:
	free(ppTempValues);
_errAllocTempValues:
	free(pTempTypes);
_errAllocTempTypes:
	;

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOk);
}

/*
** Name:
**		UtlStream_UnpackMsg
**
** Purpose:
**		Unpacks a message that consists of a variable number of arguments
**		from a stream into a list of arguments with a corresponding list
**		of associated types.
**
** Calling Sequence:
**		isOk = UtlStream_UnpackMsg(useHostOrder, szStream, pStream,
**								   ndxLengthArg, ndxFirstBodyArg,
**								   pLengthBody, pLengthHeader,
**								   pNumArgs, ppArgTypes, pppArgValues);
**
** Input:
**		useHostOrder	use host ordering of bytes indicator
**		szStream		size of the stream to unpack
**		pStream			ptr to the stream to unpack
**		ndxLengthArg	the array index of the LENGTH argument
**		ndxFirstBodyArg	the array index of the first BODY argument
**
** Output:
**		pLengthBody		ptr to returned length of msg body
**		pLengthHeader	ptr to returned length of msg header
**		pNumArgs		ptr to returned number of args
**		ppArgTypes		ptr to returned array of arg types
**		pppArgValues	ptr to returned array of arg value ptrs
**
** Returns:
**		isOk			BOOLEAN indicating whether or not the
**						function executed successfully.
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
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
	char***	pppArgValues)		/* ptr to returned array of arg value ptrs	*/
{
	int		isOk = FALSE;			/* return value of this function	*/
	short*	pTempTypes;				/* ptr to temp array of arg types	*/
	char**	ppTempValues;			/* ptr to temp array of arg values	*/
	int		currByte;				/* the current byte in the stream	*/
	short	argType;				/* the type of argument to unpack	*/
	char*	func = "UtlStream_UnpackMsg"; 	/* name of this function	*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Initialize the returned number of arguments unpacked from the stream.
	Also, initialize the returned length of the message body and the 
	message header.
	--------------------------------------------------------------------- */
	*pNumArgs      = 0;
	*pLengthBody   = 0;
	*pLengthHeader = 0;

/*	Allocate some temporary storage space for the array of arg types
	and array of arg values that we are about to unpack from the stream.
	-------------------------------------------------------------------- */
	pTempTypes = (short*)calloc(MAX_PACKED_ARGS, sizeof(short));
	if (pTempTypes == NULL) goto _errAllocTempTypes;

	ppTempValues = (char**)calloc(MAX_PACKED_ARGS, sizeof(char*));
	if (ppTempValues == NULL) goto _errAllocTempValues;

/*	Indicate we are about to unpack this stream.
	-------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE,
		"Unpacking a message stream of %d bytes at addr %p...\n",
			szStream, pStream);

/*	Indicate that the message HEADER is currently being processed.
	-------------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE | LOG_NO_ID, func, RIGHT_HERE,
		"<============== Message HEADER ==============>\n");

/*	Walk through the stream and unpack each argument until the
	stream has been completely gobbled up.  To unpack an argument 
	from the stream, the type of the argument is first unpacked
	to determine HOW to unpack the argument that follows the type
	of the argument.  By convention, the TYPE information is
	always packed into a VTYPE_SHORT (2 bytes).
	------------------------------------------------------------- */
	currByte = 0;
	while (currByte < szStream) {
		if ( !unpack_type(pStream, useHostOrder, &currByte, &argType) ) {
			goto _errUnpackType;
		}

/*		If the current argument that we are unpacking happens to
		be the argument associated with the first BODY field of this
		message, then we need to show the caller that the message
		BODY is now being processed.
		------------------------------------------------------------ */
		if (*pNumArgs == ndxFirstBodyArg) {

/*			Indicate that the message BODY is currently being processed.
			------------------------------------------------------------ */
			UtlLog_Write(LOG_TRACE | LOG_NO_ID, func, RIGHT_HERE,
			"<=============== Message BODY ===============>\n");
		}

		switch ( argType ) {

/*			This variable argument type is ASCII.
			------------------------------------- */
			case VTYPE_ASCII:
				if ( !unpack_arg_ascii(pStream, useHostOrder, &currByte,
									   pNumArgs, pTempTypes, ppTempValues) ) {
					goto _errUnpackArg;
				}
				break;

/*			This variable argument type is CHAR.
			------------------------------------ */
			case VTYPE_CHAR:
				if ( !unpack_arg_char(pStream, useHostOrder, &currByte,
									  pNumArgs, pTempTypes, ppTempValues) ) {
					goto _errUnpackArg;
				}
				break;

/*			This variable argument type is UNSIGNED CHAR.
			--------------------------------------------- */
			case VTYPE_UCHAR:
				if ( !unpack_arg_uchar(pStream, useHostOrder, &currByte,
									   pNumArgs, pTempTypes, ppTempValues) ) {
					goto _errUnpackArg;
				}
				break;

/*			This variable argument type is SHORT.
			------------------------------------- */
			case VTYPE_SHORT:
				if ( !unpack_arg_short(pStream, useHostOrder, &currByte,
									   pNumArgs, pTempTypes, ppTempValues) ) {
					goto _errUnpackArg;
				}
				break;

/*			This variable argument type is INT.
			----------------------------------- */
			case VTYPE_INT:
				if ( !unpack_arg_int(pStream, useHostOrder, &currByte,
								     pNumArgs, pTempTypes, ppTempValues) ) {
					goto _errUnpackArg;
				}
				break;

/*			This variable argument type is FLOAT.
			------------------------------------- */
			case VTYPE_FLOAT:
				if ( !unpack_arg_float(pStream, useHostOrder, &currByte,
									   pNumArgs, pTempTypes, ppTempValues) ) {
					goto _errUnpackArg;
				}
				break;

/*			This variable argument type is UNKNOWN.
			--------------------------------------- */
			default:
				UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
					"ERROR: Got an UNKNOWN argument type to unpack = %d\n",
						argType);
				goto _errUnknownArgType;
				break;
		}
	}

/*	We have finished unpacking the stream into the temporary array
	of argument types and the temporary array of argument values.
	Now get the final result copied into the REAL return output arrays.
	Remember to check for the case where there were NO arguments!
	------------------------------------------------------------------- */
	if (*pNumArgs > 0) {
		*ppArgTypes   = (short*)calloc(*pNumArgs, sizeof(short));
		if (*ppArgTypes == NULL) goto _errAllocArgTypes;

		*pppArgValues = (char**)calloc(*pNumArgs, sizeof(char*));
		if (*pppArgValues == NULL) goto _errAllocArgValues;

		memcpy( *ppArgTypes, pTempTypes, (*pNumArgs * sizeof(short)) );
		memcpy( *pppArgValues, ppTempValues, (*pNumArgs * sizeof(char*)) );
	}

/*	Return to the caller the size of the message body and the size of
	the message header.
	----------------------------------------------------------------- */
	if (ndxFirstBodyArg > 0) {
		*pLengthBody   = (int)(*(*(*pppArgValues + ndxLengthArg)));
		*pLengthHeader = szStream - *pLengthBody;
	} else {
		*pLengthBody   = 0;
		*pLengthHeader = szStream;
	}

/*	Indicate that the message has been processed.
	--------------------------------------------- */
	UtlLog_Write(LOG_TRACE | LOG_NO_ID, func, RIGHT_HERE,
		"<============== End of Message ==============>\n");

/*	Indicate we are done unpacking this stream.
	------------------------------------------- */
	UtlLog_Write(LOG_TRACE | LOG_NO_ID, func, RIGHT_HERE,
		"Done unpacking a message stream of %d bytes at addr %p...\n",
			szStream, pStream);

/*	Indicate that everything went okay.
	----------------------------------- */
	isOk = TRUE;
	goto _doCleanup;

/*	Only get rid of permanent storage if there was a problem.
	--------------------------------------------------------- */
_errAllocArgValues:
	free(*ppArgTypes);
_errAllocArgTypes:
	;

/*	Remember to get rid of the temporary arrays.
	-------------------------------------------- */
_doCleanup:
	;
_errUnknownArgType:
	;
_errUnpackArg:
	;
_errUnpackType:
	free(ppTempValues);
_errAllocTempValues:
	free(pTempTypes);
_errAllocTempTypes:
	;

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOk);
}

/*
** Name:
**		pack_arg_ascii
**
** Purpose:
**		Packs an argument of type VTYPE_ASCII into an output stream.
**
*/
static int pack_arg_ascii(
	char*	pArgAscii,		/* ptr to VTYPE_ASCII variable to pack	*/
	int		useHostOrder,	/* use host ordering of bytes indicator	*/
	int*	pSzStream,		/* ptr to current size of stream		*/
	char*	pStream)		/* ptr to current stream				*/
{
	int		isOk = FALSE;			/* return value of this function	*/
	int		bytesNeeded;			/* bytes needed to pack this arg	*/
	char*	func = "pack_arg_ascii"; 		/* name of this function	*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Validate our input.
	------------------- */
	if ( strlen(pArgAscii) > MAX_SIZE_ASCII ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Size of ASCII string (%d) exceeds max (%d)\n",
				strlen(pArgAscii), MAX_SIZE_ASCII);
		goto _errStringTooBig;
	}

	bytesNeeded = strlen(pArgAscii) + 1 + sizeof(short);
	if ( (*pSzStream + bytesNeeded) > MAX_SIZE_STREAM ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Current size (%d) + bytes needed (%d) exceeds max (%d)\n",
				*pSzStream, bytesNeeded, MAX_SIZE_STREAM);
		goto _errStreamTooBig;
	}

/*	Show what we are about to pack into the output stream.
	------------------------------------------------------ */
	UtlLog_Write(LOG_TRACE | LOG_NO_ID, func, RIGHT_HERE,
		"...packing a VTYPE_ASCII argument = \"%s\"\n", pArgAscii);

/*	Get the type of this argument into the output stream.
	----------------------------------------------------- */
	pack_type(VTYPE_ASCII, useHostOrder, pSzStream, pStream);

/*	Our input argument will fit within the current stream, so
	get it copied into the output stream.
	--------------------------------------------------------- */
	memcpy( (pStream + *pSzStream), pArgAscii, strlen(pArgAscii) + 1 );

/*	Update the size of the current output stream.
	--------------------------------------------- */
	*pSzStream += (strlen(pArgAscii) + 1);

/*	Indicate that everything went okay.
	----------------------------------- */
	isOk = TRUE;

/*	The ASCII string that needs to be packed is too big.
	---------------------------------------------------- */
_errStringTooBig:
	;

/*	The stream being created has gotten too big.
	-------------------------------------------- */
_errStreamTooBig:
	;

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOk);
}

/*
** Name:
**		pack_arg_char
**
** Purpose:
**		Packs an argument of type VTYPE_CHAR into an output stream.
**
*/
static int pack_arg_char(
	char	argChar,		/* the VTYPE_CHAR variable to pack		*/
	int		useHostOrder,	/* use host ordering of bytes indicator	*/
	int*	pSzStream,		/* ptr to current size of stream		*/
	char*	pStream)		/* ptr to current stream				*/
{
	int		isOk = FALSE;			/* return value of this function	*/
	int		bytesNeeded;			/* bytes needed to pack this arg	*/
	char*	func = "pack_arg_char"; 		/* name of this function	*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Validate our input.
	------------------- */
	bytesNeeded = sizeof(char) + sizeof(short);
	if ( (*pSzStream + bytesNeeded) > MAX_SIZE_STREAM ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Current size (%d) + bytes needed (%d) exceeds max (%d)\n",
				*pSzStream, bytesNeeded, MAX_SIZE_STREAM);
		goto _errStreamTooBig;
	}

/*	Show what we are about to pack into the output stream.
	------------------------------------------------------ */
	UtlLog_Write(LOG_TRACE | LOG_NO_ID, func, RIGHT_HERE,
		"...packing a VTYPE_CHAR argument = \"%c\"\n", argChar);

/*	Get the type of this argument into the output stream.
	----------------------------------------------------- */
	pack_type(VTYPE_CHAR, useHostOrder, pSzStream, pStream);

/*	Our input argument will fit within the current stream, so
	get it copied into the output stream.
	--------------------------------------------------------- */
	memcpy( (pStream + *pSzStream), &argChar, sizeof(char) );

/*	Update the size of the current output stream.
	--------------------------------------------- */
	*pSzStream += sizeof(char);

/*	Indicate that everything went okay.
	----------------------------------- */
	isOk = TRUE;

/*	The stream being created has gotten too big.
	-------------------------------------------- */
_errStreamTooBig:
	;

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOk);
}

/*
** Name:
**		pack_arg_float
**
** Purpose:
**		Packs an argument of type VTYPE_FLOAT into an output stream.
**
*/
static int pack_arg_float(
	float	argFloat,		/* the VTYPE_FLOAT variable to pack		*/
	int		useHostOrder,	/* use host ordering of bytes indicator	*/
	int*	pSzStream,		/* ptr to current size of stream		*/
	char*	pStream)		/* ptr to current stream				*/
{
	int		isOk = FALSE;			/* return value of this function	*/
	int		bytesNeeded;			/* bytes needed to pack this arg	*/
	float	theFloat;				/* the FLOAT argument				*/
	char*	func = "pack_arg_float"; 		/* name of this function	*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Validate our input.
	------------------- */
	bytesNeeded = sizeof(float) + sizeof(short);
	if ( (*pSzStream + bytesNeeded) > MAX_SIZE_STREAM ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Current size (%d) + bytes needed (%d) exceeds max (%d)\n",
				*pSzStream, bytesNeeded, MAX_SIZE_STREAM);
		goto _errStreamTooBig;
	}

/*	Show what we are about to pack into the output stream.
	------------------------------------------------------ */
	UtlLog_Write(LOG_TRACE | LOG_NO_ID, func, RIGHT_HERE,
		"...packing a VTYPE_FLOAT argument = %f\n", argFloat);

/*	Get the type of this argument into the output stream.
	----------------------------------------------------- */
	pack_type(VTYPE_FLOAT, useHostOrder, pSzStream, pStream);

/*	Our input argument will fit within the current stream, so
	get it copied into the output stream, after swapping the
	bytes, if required.
	--------------------------------------------------------- */
	theFloat = argFloat;
	if (!useHostOrder) {
		theFloat = (float)htonl((long)argFloat);
	}
	memcpy( (pStream + *pSzStream), &theFloat, sizeof(float) );

/*	Update the size of the current output stream.
	--------------------------------------------- */
	*pSzStream += sizeof(float);

/*	Indicate that everything went okay.
	----------------------------------- */
	isOk = TRUE;

/*	The stream being created has gotten too big.
	-------------------------------------------- */
_errStreamTooBig:
	;

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOk);
}

/*
** Name:
**		pack_arg_int
**
** Purpose:
**		Packs an argument of type VTYPE_INT into an output stream.
**
*/
static int pack_arg_int(
	int		argInt,			/* the VTYPE_INT variable to pack		*/
	int		useHostOrder,	/* use host ordering of bytes indicator	*/
	int*	pSzStream,		/* ptr to current size of stream		*/
	char*	pStream)		/* ptr to current stream				*/
{
	int		isOk = FALSE;			/* return value of this function	*/
	int		bytesNeeded;			/* bytes needed to pack this arg	*/
	int		theInt;					/* the INTEGER argument				*/
	char*	func = "pack_arg_int"; 			/* name of this function	*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Validate our input.
	------------------- */
	bytesNeeded = sizeof(int) + sizeof(short);
	if ( (*pSzStream + bytesNeeded) > MAX_SIZE_STREAM ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Current size (%d) + bytes needed (%d) exceeds max (%d)\n",
				*pSzStream, bytesNeeded, MAX_SIZE_STREAM);
		goto _errStreamTooBig;
	}

/*	Show what we are about to pack into the output stream.
	------------------------------------------------------ */
	UtlLog_Write(LOG_TRACE | LOG_NO_ID, func, RIGHT_HERE,
		"...packing a VTYPE_INT argument = %d\n", argInt);

/*	Get the type of this argument into the output stream.
	----------------------------------------------------- */
	pack_type(VTYPE_INT, useHostOrder, pSzStream, pStream);

/*	Our input argument will fit within the current stream, so
	get it copied into the output stream, after swapping the
	bytes, if required.
	--------------------------------------------------------- */
	theInt = argInt;
	if (!useHostOrder) {
		theInt = htonl(argInt);
	}
	memcpy( (pStream + *pSzStream), &theInt, sizeof(int) );

/*	Update the size of the current output stream.
	--------------------------------------------- */
	*pSzStream += sizeof(int);

/*	Indicate that everything went okay.
	----------------------------------- */
	isOk = TRUE;

/*	The stream being created has gotten too big.
	-------------------------------------------- */
_errStreamTooBig:
	;

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOk);
}

/*
** Name:
**		pack_arg_short
**
** Purpose:
**		Packs an argument of type VTYPE_SHORT into an output stream.
**
*/
static int pack_arg_short(
	short	argShort,		/* the VTYPE_SHORT variable to pack		*/
	int		useHostOrder,	/* use host ordering of bytes indicator	*/
	int*	pSzStream,		/* ptr to current size of stream		*/
	char*	pStream)		/* ptr to current stream				*/
{
	int		isOk = FALSE;			/* return value of this function	*/
	int		bytesNeeded;			/* bytes needed to pack this arg	*/
	short	theShort;				/* the SHORT argument				*/
	char*	func = "pack_arg_short"; 		/* name of this function	*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Validate our input.
	------------------- */
	bytesNeeded = sizeof(short) + sizeof(short);
	if ( (*pSzStream + bytesNeeded) > MAX_SIZE_STREAM ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Current size (%d) + bytes needed (%d) exceeds max (%d)\n",
				*pSzStream, bytesNeeded, MAX_SIZE_STREAM);
		goto _errStreamTooBig;
	}

/*	Show what we are about to pack into the output stream.
	------------------------------------------------------ */
	UtlLog_Write(LOG_TRACE | LOG_NO_ID, func, RIGHT_HERE,
		"...packing a VTYPE_SHORT argument = %d\n", argShort);

/*	Get the type of this argument into the output stream.
	----------------------------------------------------- */
	pack_type(VTYPE_SHORT, useHostOrder, pSzStream, pStream);

/*	Our input argument will fit within the current stream, so
	get it copied into the output stream, after swapping the
	bytes, if required.
	--------------------------------------------------------- */
	theShort = argShort;
	if (!useHostOrder) {
		theShort = htons(argShort);
	}
	memcpy( (pStream + *pSzStream), &theShort, sizeof(short) );

/*	Update the size of the current output stream.
	--------------------------------------------- */
	*pSzStream += sizeof(short);

/*	Indicate that everything went okay.
	----------------------------------- */
	isOk = TRUE;

/*	The stream being created has gotten too big.
	-------------------------------------------- */
_errStreamTooBig:
	;

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOk);
}

/*
** Name:
**		pack_arg_uchar
**
** Purpose:
**		Packs an argument of type VTYPE_UCHAR into an output stream.
**
*/
static int pack_arg_uchar(
	unsigned char	argUchar,		/* the VTYPE_CHAR variable to pack		*/
	int				useHostOrder,	/* use host ordering of bytes indicator	*/
	int*			pSzStream,		/* ptr to current size of stream		*/
	char*			pStream)		/* ptr to current stream				*/
{
	int		isOk = FALSE;			/* return value of this function	*/
	int		bytesNeeded;			/* bytes needed to pack this arg	*/
	char*	func = "pack_arg_uchar"; 		/* name of this function	*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Validate our input.
	------------------- */
	bytesNeeded = sizeof(unsigned char) + sizeof(short);
	if ( (*pSzStream + bytesNeeded) > MAX_SIZE_STREAM ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Current size (%d) + bytes needed (%d) exceeds max (%d)\n",
				*pSzStream, bytesNeeded, MAX_SIZE_STREAM);
		goto _errStreamTooBig;
	}

/*	Show what we are about to pack into the output stream.
	------------------------------------------------------ */
	UtlLog_Write(LOG_TRACE | LOG_NO_ID, func, RIGHT_HERE,
		"...packing a VTYPE_UCHAR argument = %d\n", argUchar);

/*	Get the type of this argument into the output stream.
	----------------------------------------------------- */
	pack_type(VTYPE_UCHAR, useHostOrder, pSzStream, pStream);

/*	Our input argument will fit within the current stream, so
	get it copied into the output stream.
	--------------------------------------------------------- */
	memcpy( (pStream + *pSzStream), &argUchar, sizeof(unsigned char) );

/*	Update the size of the current output stream.
	--------------------------------------------- */
	*pSzStream += sizeof(unsigned char);

/*	Indicate that everything went okay.
	----------------------------------- */
	isOk = TRUE;

/*	The stream being created has gotten too big.
	-------------------------------------------- */
_errStreamTooBig:
	;

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOk);
}

/*
** Name:
**		pack_type
**
** Purpose:
**		Packs an argument type into an output stream.
**
*/
static int pack_type(
	short	argType,		/* the argument type to pack			*/
	int		useHostOrder,	/* use host ordering of bytes indicator	*/
	int*	pSzStream,		/* ptr to current size of stream		*/
	char*	pStream)		/* ptr to current stream				*/
{
	int		isOk = FALSE;		/* return value of this function	*/
	short	theType;			/* the type of the argument			*/
	char*	func = "pack_type"; 		/* name of this function	*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Get the type of this argument into the output stream,
	after swapping the bytes, if required.
	----------------------------------------------------- */
	theType = argType;
	if (!useHostOrder) {
		theType = htons(argType);
	}
	memcpy((pStream + *pSzStream), &theType, sizeof(short));

/*	Update the size of the current output stream.
	--------------------------------------------- */
	*pSzStream += sizeof(short);

/*	Indicate that everything went okay.
	----------------------------------- */
	isOk = TRUE;

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOk);
}

/*
** Name:
**		stream_pack_args_var
**
** Purpose:
**		Packs a variable list of arguments into a stream that is
**		suitable for sending from one process to another (Typically,
**		a client process to a server process).
*/
static int stream_pack_args_var(
	int		useHostOrder,	/* use host ordering of bytes indicator	*/
	int		numArgs,		/* number of args in variable arg list	*/
	short*	pArgTypes,		/* ptr to array of argument types		*/
	int*	pSzStream,		/* ptr to returned size of the stream	*/
	char**	ppStream,		/* ptr to returned ptr to the stream	*/
	va_list	pArgs)			/* ptr to the variable arg list to pack	*/
{
	int			isOk = FALSE;		/* return value of this function	*/
	char*		pTempStream;		/* ptr to a temporary stream area	*/
	int			thisArg;			/* the current argument in the list	*/
	char*		pArgAscii;			/* a VTYPE_ASCII argument			*/
	int			argChar;			/* a VTYPE_CHAR argument			*/
	int			argUchar;			/* a VTYPE_UCHAR argument			*/
	int			argShort;			/* a VTYPE_SHORT argument			*/
	int			argInt;				/* a VTYPE_INT argument				*/
	double		argFloat;			/* a VTYPE_FLOAT argument			*/
	char*	func = "stream_pack_args_var"; 	/* name of this function	*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Initialize the returned size of the stream.
	------------------------------------------- */
	*pSzStream = 0;

/*	Allocate some temporary storage space for the stream we are
	about to build up from a variable argument list.
	----------------------------------------------------------- */
	pTempStream = (char*)calloc(MAX_SIZE_STREAM, 1);
	if (pTempStream == NULL) goto _errAllocTempStream;

/*	Pull off each of the variable arguments that are to be packed.
	Use the type information that is supplied by the caller in the
	"pArgTypes" array to determine what the type of each variable
	argument is SUPPOSED to be.  If the caller makes a mistake, it
	is quite likely that the application will crash with an access
	violation or the application will behave unpredictably.
	-------------------------------------------------------------- */
	thisArg = 0;
	while (thisArg < numArgs) {
		switch ( *(pArgTypes + thisArg) ) {

/*			This variable argument type is ASCII.
			------------------------------------- */
			case VTYPE_ASCII:
				pArgAscii = va_arg(pArgs, char*);
				if ( !pack_arg_ascii(pArgAscii, useHostOrder,
									 pSzStream, pTempStream) ) {
					goto _errPackArg;
				}
				break;

/*			This variable argument type is CHAR.
			------------------------------------ */
			case VTYPE_CHAR:
				argChar = va_arg(pArgs, int);
				if ( !pack_arg_char( (char)argChar, useHostOrder,
									 pSzStream, pTempStream) ) {
					goto _errPackArg;
				}
				break;

/*			This variable argument type is UNSIGNED CHAR.
			--------------------------------------------- */
			case VTYPE_UCHAR:
				argUchar = va_arg(pArgs, int);
				if ( !pack_arg_uchar( (unsigned char)argUchar, useHostOrder,
									 pSzStream, pTempStream ) ) {
					goto _errPackArg;
				}
				break;

/*			This variable argument type is SHORT.
			------------------------------------- */
			case VTYPE_SHORT:
				argShort = va_arg(pArgs, int);
				if ( !pack_arg_short( (short)argShort, useHostOrder,
									  pSzStream, pTempStream ) ) {
					goto _errPackArg;
				}
				break;

/*			This variable argument type is INTEGER.
			--------------------------------------- */
			case VTYPE_INT:
				argInt = va_arg(pArgs, int);
				if ( !pack_arg_int( argInt, useHostOrder,
									pSzStream, pTempStream ) ) {
					goto _errPackArg;
				}
				break;

/*			This variable argument type is FLOAT.
			------------------------------------- */
			case VTYPE_FLOAT:
				argFloat = va_arg(pArgs, double);
				if ( !pack_arg_float( (float)argFloat, useHostOrder,
									  pSzStream, pTempStream ) ) {
					goto _errPackArg;
				}
				break;

/*			This variable argument type is UNKNOWN.
			--------------------------------------- */
			default:
				UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
					"ERROR: Got an UNKNOWN argument type to pack = %d\n",
						*(pArgTypes + thisArg) );
				goto _errUnknownArgType;
				break;
		}

/*		Bump to the next argument in the variable argument list.
		-------------------------------------------------------- */
		thisArg++;
	}

/*	We have finished packing the variable argument list into the
	temporary output stream, so now get the final result copied
	into the REAL return output stream.  Remember to check for
	the case where there were NO variable arguments!
	------------------------------------------------------------ */
	if (*pSzStream > 0) {
		*ppStream = malloc(*pSzStream);
		if (*ppStream == NULL) goto _errAllocStream;
		memcpy(*ppStream, pTempStream, *pSzStream);
	}

/*	Indicate that everything went okay.
	----------------------------------- */
	isOk = TRUE;
	goto _doCleanup;

/*	Only get rid of permanent storage if there was a problem.
	--------------------------------------------------------- */
_errAllocStream:
	;

/*	Remember to get rid of the temporary output stream.
	--------------------------------------------------- */
_doCleanup:
	;
_errPackArg:
	;
_errUnknownArgType:
	free(pTempStream);
_errAllocTempStream:
	;

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOk);
}

/*
** Name:
**		stream_pack_length
**
** Purpose:
**		Packs the length information into a stream.
*/
static int stream_pack_length(
	int		useHostOrder,		/* use host ordering of bytes indicator		*/
	short	typeLength,			/* the type of the message length			*/
	int		theLength,			/* the message length itself				*/
	char*	pStream)			/* ptr to the stream to pack				*/
{
	int			isOk = FALSE;		/* return value of this function	*/
	int			szLength;			/* the size of the msg length		*/
	char*	func = "stream_pack_length"; 	/* name of this function	*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Use the type information that is supplied by the caller in the
	"typeLength" parameter to determine what the type of the length
	argument is SUPPOSED to be.  If the caller makes a mistake, it
	is quite likely that the application will crash with an access
	violation or the application will behave unpredictably.
	--------------------------------------------------------------- */
	szLength = 0;
	switch ( typeLength ) {

/*		This length argument type is CHAR.
		---------------------------------- */
		case VTYPE_CHAR:
			if ( !pack_arg_char( (char)theLength, useHostOrder,
								 &szLength, pStream) ) {
				goto _errPackArg;
			}
			break;

/*		This length argument type is UNSIGNED CHAR.
		------------------------------------------- */
		case VTYPE_UCHAR:
			if ( !pack_arg_uchar( (unsigned char)theLength, useHostOrder,
								  &szLength, pStream) ) {
				goto _errPackArg;
			}
			break;

/*		This length argument type is SHORT.
		----------------------------------- */
		case VTYPE_SHORT:
			if ( !pack_arg_short( (short)theLength, useHostOrder,
								  &szLength, pStream) ) {
				goto _errPackArg;
			}
			break;

/*		This length argument type is INTEGER.
		------------------------------------- */
		case VTYPE_INT:
			if ( !pack_arg_int( theLength, useHostOrder,
								&szLength, pStream) ) {
				goto _errPackArg;
			}
			break;

/*		This variable argument type is UNKNOWN.
		--------------------------------------- */
		default:
			UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
				"ERROR: Got an UNKNOWN argument length type to pack = %d\n",
					typeLength);
			goto _errUnknownArgType;
			break;
	}

/*	We have finished packing the length argument into the input stream.
	------------------------------------------------------------------- */
	if (szLength > 0) {
		UtlLog_Write(LOG_TRACE , func, RIGHT_HERE,
			"Packed a message length of %d bytes at addr %p\n",
				szLength, pStream);
	}

/*	Indicate that everything went okay.
	----------------------------------- */
	isOk = TRUE;

/*	An argument length could not be packed.
	--------------------------------------- */
_errUnknownArgType:
	;
_errPackArg:
	;

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOk);
}

/*
** Name:
**		stream_pack_msg_var
**
** Purpose:
**		Packs a complete message (which is passed as a variable list of
**		arguments) into a stream that is suitable for sending from one
**		process to another (Typically, a client process to a server process).
*/
static int stream_pack_msg_var(
	int		useHostOrder,		/* use host ordering of bytes indicator		*/
	int		numArgs,			/* number of args in variable arg list		*/
	short*	pArgTypes,			/* ptr to array of argument types			*/
	int		ndxLengthArg,		/* the array index of the LENGTH arg		*/
	int		ndxFirstBodyArg,	/* the array index of the first BODY arg	*/
	int*	pSzStream,			/* ptr to returned size of the stream		*/
	char**	ppStream,			/* ptr to returned ptr to the stream		*/
	va_list	pArgs)				/* ptr to the variable arg list to pack		*/
{
	int			isOk = FALSE;		/* return value of this function		*/
	char*		pTempStream;		/* ptr to a temporary stream area		*/
	int			offLengthArg;		/* byte offset of the LENGTH arg		*/
	int			offFirstBodyArg;	/* byte offset of the first BODY arg	*/
	short		typeLength;			/* the type of the message length		*/
	int			theLength;			/* the message length itself			*/
	int			thisArg;			/* the current argument in the list		*/
	char*		pArgAscii;			/* a VTYPE_ASCII argument				*/
	int			argChar;			/* a VTYPE_CHAR argument				*/
	int			argUchar;			/* a VTYPE_UCHAR argument				*/
	int			argShort;			/* a VTYPE_SHORT argument				*/
	int			argInt;				/* a VTYPE_INT argument					*/
	double		argFloat;			/* a VTYPE_FLOAT argument				*/
	char*	func = "stream_pack_msg_var"; 		/* name of this function	*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Initialize the returned size of the stream.
	------------------------------------------- */
	*pSzStream = 0;

/*	Allocate some temporary storage space for the stream we are
	about to build up from a variable argument list.
	----------------------------------------------------------- */
	pTempStream = (char*)calloc(MAX_SIZE_STREAM, 1);
	if (pTempStream == NULL) goto _errAllocTempStream;

/*	Indicate that the message HEADER is currently being processed.
	-------------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE | LOG_NO_ID, func, RIGHT_HERE,
		"<============== Message HEADER ==============>\n");

/*	Process each of the variable arguments that are to be packed.
	------------------------------------------------------------- */
	thisArg = 0;
	while (thisArg < numArgs) {

/*		If the current argument that we are packing up happens to
		be the argument associated with the LENGTH field of this
		message, then we need to remember our current byte offset
		so we can later update this LENGTH field with the correct
		value.
		--------------------------------------------------------- */
		if (thisArg == ndxLengthArg) {
			offLengthArg = *pSzStream;
		}

/*		If the current argument that we are packing up happens to
		be the argument associated with the first BODY field of this
		message, then we need to remember our current byte offset
		so we can later update the LENGTH field with the correct
		value.
		------------------------------------------------------------ */
		if (thisArg == ndxFirstBodyArg) {
			offFirstBodyArg = *pSzStream;

/*			Indicate that the message BODY is currently being processed.
			------------------------------------------------------------ */
			UtlLog_Write(LOG_TRACE | LOG_NO_ID, func, RIGHT_HERE,
			"<=============== Message BODY ===============>\n");
		}

/*		Use the type information that is supplied by the caller in the
		"pArgTypes" array to determine what the type of each variable
		argument is SUPPOSED to be.  If the caller makes a mistake, it
		is quite likely that the application will crash with an access
		violation or the application will behave unpredictably.
		-------------------------------------------------------------- */
		switch ( *(pArgTypes + thisArg) ) {

/*			This variable argument type is ASCII.
			------------------------------------- */
			case VTYPE_ASCII:
				pArgAscii = va_arg(pArgs, char*);
				if ( !pack_arg_ascii(pArgAscii, useHostOrder,
									 pSzStream, pTempStream) ) {
					goto _errPackArg;
				}
				break;

/*			This variable argument type is CHAR.
			------------------------------------ */
			case VTYPE_CHAR:
				argChar = va_arg(pArgs, int);
				if ( !pack_arg_char( (char)argChar, useHostOrder,
									 pSzStream, pTempStream) ) {
					goto _errPackArg;
				}
				break;

/*			This variable argument type is UNSIGNED CHAR.
			--------------------------------------------- */
			case VTYPE_UCHAR:
				argUchar = va_arg(pArgs, int);
				if ( !pack_arg_uchar( (unsigned char)argUchar, useHostOrder,
									 pSzStream, pTempStream ) ) {
					goto _errPackArg;
				}
				break;

/*			This variable argument type is SHORT.
			------------------------------------- */
			case VTYPE_SHORT:
				argShort = va_arg(pArgs, int);
				if ( !pack_arg_short( (short)argShort, useHostOrder,
									  pSzStream, pTempStream ) ) {
					goto _errPackArg;
				}
				break;

/*			This variable argument type is INTEGER.
			--------------------------------------- */
			case VTYPE_INT:
				argInt = va_arg(pArgs, int);
				if ( !pack_arg_int( argInt, useHostOrder,
									pSzStream, pTempStream ) ) {
					goto _errPackArg;
				}
				break;

/*			This variable argument type is FLOAT.
			------------------------------------- */
			case VTYPE_FLOAT:
				argFloat = va_arg(pArgs, double);
				if ( !pack_arg_float( (float)argFloat, useHostOrder,
									  pSzStream, pTempStream ) ) {
					goto _errPackArg;
				}
				break;

/*			This variable argument type is UNKNOWN.
			--------------------------------------- */
			default:
				UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
					"ERROR: Got an UNKNOWN argument type to pack = %d\n",
						*(pArgTypes + thisArg) );
				goto _errUnknownArgType;
				break;
		}

/*		Bump to the next argument in the variable argument list.
		-------------------------------------------------------- */
		thisArg++;
	}

/*	Indicate that the message has been processed.
	--------------------------------------------- */
	UtlLog_Write(LOG_TRACE | LOG_NO_ID, func, RIGHT_HERE,
		"<============== End of Message ==============>\n");

/*	We have finished packing this message with the variable argument
	list into the temporary output stream, so now get the final result
	copied into the REAL return output stream.  Remember to check for
	the case where there were NO variable arguments!
	------------------------------------------------------------------ */
	if (*pSzStream > 0) {
		*ppStream = malloc(*pSzStream);
		if (*ppStream == NULL) goto _errAllocStream;
		memcpy(*ppStream, pTempStream, *pSzStream);

/*		Calculate the correct length information for this message.
		If a first BODY argument exists, then use the starting offset
		of this argument to determine the length of the body of the
		message.  Otherwise, the length of the body of the message
		is just 0 (i.e., the message is nothing more than a header).
		------------------------------------------------------------- */
		if (ndxFirstBodyArg > 0) {
			theLength = *pSzStream - offFirstBodyArg;
		} else {
			theLength = 0;
		}
		typeLength = ( *(pArgTypes + ndxLengthArg) ); 

/*		Get the LENGTH field of this message stream updated with
		the correct value.
		-------------------------------------------------------- */
		if ( !stream_pack_length(useHostOrder, typeLength, theLength,
								 (*ppStream + offLengthArg)) ) {
			goto _errPackLength;
		}
	}

/*	Indicate that everything went okay.
	----------------------------------- */
	isOk = TRUE;
	goto _doCleanup;

/*	Only get rid of permanent storage if there was a problem.
	--------------------------------------------------------- */
_errPackLength:
	free(*ppStream);
_errAllocStream:
	;

/*	Remember to get rid of the temporary output stream.
	--------------------------------------------------- */
_doCleanup:
	;
_errUnknownArgType:
	;
_errPackArg:
	free(pTempStream);
_errAllocTempStream:
	;

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOk);
}

/*
** Name:
**		unpack_arg_ascii
**
** Purpose:
**		Unpacks an argument of type VTYPE_ASCII from an input stream.
**
*/
static int unpack_arg_ascii(
	char*	pStream,		/* ptr to current stream					*/
	int		useHostOrder,	/* use host ordering of bytes indicator		*/
	int*	pCurrByte,		/* ptr to current byte in the stream		*/
	int*	pNumArgs,		/* ptr to current number of arguments		*/
	short*	pArgTypes,		/* ptr to the returned array of arg types	*/
	char**	ppArgValues)	/* ptr to the returned array of arg values	*/
{
	int		isOk = FALSE;			/* return value of this function	*/
	int		lenArg;					/* length (bytes) for this argument	*/
	char*	func = "unpack_arg_ascii"; 		/* name of this function	*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Make sure we are not about to exceed the maximum number of arguments
	that are allowed to be unpacked from this stream.
	-------------------------------------------------------------------- */
	if ( *pNumArgs >= MAX_PACKED_ARGS ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Cannot unpack this arg => exceeds max packed args (%d)\n",
				MAX_PACKED_ARGS);
		goto _errTooManyArgs;
	}

/*	Make sure this ASCII string is a reasonable size.
	------------------------------------------------- */
	lenArg = strlen(pStream + *pCurrByte) + 1;
	if ( lenArg > MAX_SIZE_ASCII ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Size of ASCII string (%d) exceeds max (%d)\n",
				lenArg, MAX_SIZE_ASCII);
		goto _errStringTooBig;
	}

/*	Allocate the memory for this argument and get it copied into the memory.
	------------------------------------------------------------------------ */
	*(ppArgValues + *pNumArgs) = (char*)malloc(lenArg);
	if ( *(ppArgValues + *pNumArgs) == NULL ) goto _errNoMemory;
	memcpy( *(ppArgValues + *pNumArgs), (pStream + *pCurrByte), lenArg);

	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE,
		"Allocated arg %d of unpacked stream at addr %p in array at %p\n",
			*pNumArgs, *(ppArgValues + *pNumArgs), ppArgValues );

/*	Show what we just unpacked from the input stream.
	------------------------------------------------- */
	UtlLog_Write(LOG_TRACE | LOG_NO_ID, func, RIGHT_HERE,
		"...unpacked a VTYPE_ASCII argument = \"%s\"\n",
			(pStream + *pCurrByte));

/*	Indicate the type of this argument.
	----------------------------------- */
	*(pArgTypes + *pNumArgs) = VTYPE_ASCII;

/*	Update the current byte of the input stream and the
	current number or arguments.
	--------------------------------------------------- */
	*pCurrByte += lenArg;
	(*pNumArgs)++;

/*	Indicate that everything went okay.
	----------------------------------- */
	isOk = TRUE;

/*	We had an error condition.
	-------------------------- */
_errNoMemory:
	;
_errStringTooBig:
	;
_errTooManyArgs:
	;

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOk);
}

/*
** Name:
**		unpack_arg_char
**
** Purpose:
**		Unpacks an argument of type VTYPE_CHAR from an input stream.
**
*/
static int unpack_arg_char(
	char*	pStream,		/* ptr to current stream					*/
	int		useHostOrder,	/* use host ordering of bytes indicator		*/
	int*	pCurrByte,		/* ptr to current byte in the stream		*/
	int*	pNumArgs,		/* ptr to current number of arguments		*/
	short*	pArgTypes,		/* ptr to the returned array of arg types	*/
	char**	ppArgValues)	/* ptr to the returned array of arg values	*/
{
	int		isOk = FALSE;			/* return value of this function	*/
	int		lenArg;					/* length (bytes) for this argument	*/
	char*	func = "unpack_arg_char"; 		/* name of this function	*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Make sure we are not about to exceed the maximum number of arguments
	that are allowed to be unpacked from this stream.
	-------------------------------------------------------------------- */
	if ( *pNumArgs >= MAX_PACKED_ARGS ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Cannot unpack this arg => exceeds max packed args (%d)\n",
				MAX_PACKED_ARGS);
		goto _errTooManyArgs;
	}

/*	Allocate the memory for this argument and get it copied into the memory.
	------------------------------------------------------------------------ */
	lenArg = sizeof(char);
	*(ppArgValues + *pNumArgs) = (char*)malloc(lenArg);
	if ( *(ppArgValues + *pNumArgs) == NULL ) goto _errNoMemory;
	memcpy( *(ppArgValues + *pNumArgs), (pStream + *pCurrByte), lenArg);

	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE,
		"Allocated arg %d of unpacked stream at addr %p in array at %p\n",
			*pNumArgs, *(ppArgValues + *pNumArgs), ppArgValues );

/*	Show what we just unpacked from the input stream.
	------------------------------------------------- */
	UtlLog_Write(LOG_TRACE | LOG_NO_ID, func, RIGHT_HERE,
		"...unpacked a VTYPE_CHAR argument = \"%c\"\n",
			*(pStream + *pCurrByte));

/*	Indicate the type of this argument.
	----------------------------------- */
	*(pArgTypes + *pNumArgs) = VTYPE_CHAR;

/*	Update the current byte of the input stream and the
	current number or arguments.
	--------------------------------------------------- */
	*pCurrByte += lenArg;
	(*pNumArgs)++;

/*	Indicate that everything went okay.
	----------------------------------- */
	isOk = TRUE;

/*	We had an error condition.
	-------------------------- */
_errNoMemory:
	;
_errTooManyArgs:
	;

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOk);
}

/*
** Name:
**		unpack_arg_float
**
** Purpose:
**		Unpacks an argument of type VTYPE_FLOAT from an input stream.
**
*/
static int unpack_arg_float(
	char*	pStream,		/* ptr to current stream					*/
	int		useHostOrder,	/* use host ordering of bytes indicator		*/
	int*	pCurrByte,		/* ptr to current byte in the stream		*/
	int*	pNumArgs,		/* ptr to current number of arguments		*/
	short*	pArgTypes,		/* ptr to the returned array of arg types	*/
	char**	ppArgValues)	/* ptr to the returned array of arg values	*/
{
	int		isOk = FALSE;			/* return value of this function	*/
	int		lenArg;					/* length (bytes) for this argument	*/
	float	argFloat;				/* the FLOAT argument				*/
	float	theFloat;				/* the FLOAT argument				*/
	char*	func = "unpack_arg_float"; 		/* name of this function	*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Make sure we are not about to exceed the maximum number of arguments
	that are allowed to be unpacked from this stream.
	-------------------------------------------------------------------- */
	if ( *pNumArgs >= MAX_PACKED_ARGS ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Cannot unpack this arg => exceeds max packed args (%d)\n",
				MAX_PACKED_ARGS);
		goto _errTooManyArgs;
	}

/*	Get this argument copied into a local variable and swap the
	bytes, if required.
	----------------------------------------------------------- */
	lenArg = sizeof(float);
	memcpy(&argFloat, (pStream + *pCurrByte), lenArg);
	theFloat = argFloat;
	if (!useHostOrder) {
		theFloat = (float)ntohl((long)argFloat);
	}

/*	Allocate the memory for this argument and get it copied into the memory.
	------------------------------------------------------------------------ */
	*(ppArgValues + *pNumArgs) = (char*)malloc(lenArg);
	if ( *(ppArgValues + *pNumArgs) == NULL ) goto _errNoMemory;
	memcpy( *(ppArgValues + *pNumArgs), &theFloat, lenArg);

	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE,
		"Allocated arg %d of unpacked stream at addr %p in array at %p\n",
			*pNumArgs, *(ppArgValues + *pNumArgs), ppArgValues );

/*	Show what we just unpacked from the input stream.
	------------------------------------------------- */
	UtlLog_Write(LOG_TRACE | LOG_NO_ID, func, RIGHT_HERE,
		"...unpacked a VTYPE_FLOAT argument = %f\n", theFloat);

/*	Indicate the type of this argument.
	----------------------------------- */
	*(pArgTypes + *pNumArgs) = VTYPE_FLOAT;

/*	Update the current byte of the input stream and the
	current number or arguments.
	--------------------------------------------------- */
	*pCurrByte += lenArg;
	(*pNumArgs)++;

/*	Indicate that everything went okay.
	----------------------------------- */
	isOk = TRUE;

/*	We had an error condition.
	-------------------------- */
_errNoMemory:
	;
_errTooManyArgs:
	;

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOk);
}

/*
** Name:
**		unpack_arg_int
**
** Purpose:
**		Unpacks an argument of type VTYPE_INT from an input stream.
**
*/
static int unpack_arg_int(
	char*	pStream,		/* ptr to current stream					*/
	int		useHostOrder,	/* use host ordering of bytes indicator		*/
	int*	pCurrByte,		/* ptr to current byte in the stream		*/
	int*	pNumArgs,		/* ptr to current number of arguments		*/
	short*	pArgTypes,		/* ptr to the returned array of arg types	*/
	char**	ppArgValues)	/* ptr to the returned array of arg values	*/
{
	int		isOk = FALSE;			/* return value of this function	*/
	int		lenArg;					/* length (bytes) for this argument	*/
	int		argInt;					/* the INTEGER argument				*/
	int		theInt;					/* the INTEGER argument				*/
	char*	func = "unpack_arg_int"; 		/* name of this function	*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Make sure we are not about to exceed the maximum number of arguments
	that are allowed to be unpacked from this stream.
	-------------------------------------------------------------------- */
	if ( *pNumArgs >= MAX_PACKED_ARGS ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Cannot unpack this arg => exceeds max packed args (%d)\n",
				MAX_PACKED_ARGS);
		goto _errTooManyArgs;
	}

/*	Get this argument copied into a local variable and swap the
	bytes, if required.
	----------------------------------------------------------- */
	lenArg = sizeof(int);
	memcpy(&argInt, (pStream + *pCurrByte), lenArg);
	theInt = argInt;
	if (!useHostOrder) {
		theInt = ntohl(argInt);
	}

/*	Allocate the memory for this argument and get it copied into the memory.
	------------------------------------------------------------------------ */
	*(ppArgValues + *pNumArgs) = (char*)malloc(lenArg);
	if ( *(ppArgValues + *pNumArgs) == NULL ) goto _errNoMemory;
	memcpy( *(ppArgValues + *pNumArgs), &theInt, lenArg);

	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE,
		"Allocated arg %d of unpacked stream at addr %p in array at %p\n",
			*pNumArgs, *(ppArgValues + *pNumArgs), ppArgValues );

/*	Show what we just unpacked from the input stream.
	------------------------------------------------- */
	UtlLog_Write(LOG_TRACE | LOG_NO_ID, func, RIGHT_HERE,
		"...unpacked a VTYPE_INT argument = %d\n", theInt);

/*	Indicate the type of this argument.
	----------------------------------- */
	*(pArgTypes + *pNumArgs) = VTYPE_INT;

/*	Update the current byte of the input stream and the
	current number or arguments.
	--------------------------------------------------- */
	*pCurrByte += lenArg;
	(*pNumArgs)++;

/*	Indicate that everything went okay.
	----------------------------------- */
	isOk = TRUE;

/*	We had an error condition.
	-------------------------- */
_errNoMemory:
	;
_errTooManyArgs:
	;

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOk);
}

/*
** Name:
**		unpack_arg_short
**
** Purpose:
**		Unpacks an argument of type VTYPE_SHORT from an input stream.
**
*/
static int unpack_arg_short(
	char*	pStream,		/* ptr to current stream					*/
	int		useHostOrder,	/* use host ordering of bytes indicator		*/
	int*	pCurrByte,		/* ptr to current byte in the stream		*/
	int*	pNumArgs,		/* ptr to current number of arguments		*/
	short*	pArgTypes,		/* ptr to the returned array of arg types	*/
	char**	ppArgValues)	/* ptr to the returned array of arg values	*/
{
	int		isOk = FALSE;			/* return value of this function	*/
	int		lenArg;					/* length (bytes) for this argument	*/
	short	argShort;				/* the SHORT argument				*/
	short	theShort;				/* the SHORT argument				*/
	char*	func = "unpack_arg_short"; 		/* name of this function	*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Make sure we are not about to exceed the maximum number of arguments
	that are allowed to be unpacked from this stream.
	-------------------------------------------------------------------- */
	if ( *pNumArgs >= MAX_PACKED_ARGS ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Cannot unpack this arg => exceeds max packed args (%d)\n",
				MAX_PACKED_ARGS);
		goto _errTooManyArgs;
	}

/*	Get this argument copied into a local variable and swap the
	bytes, if required.
	----------------------------------------------------------- */
	lenArg = sizeof(short);
	memcpy(&argShort, (pStream + *pCurrByte), lenArg);
	theShort = argShort;
	if (!useHostOrder) {
		theShort = ntohs(argShort);
	}

/*	Allocate the memory for this argument and get it copied into the memory.
	------------------------------------------------------------------------ */
	*(ppArgValues + *pNumArgs) = (char*)malloc(lenArg);
	if ( *(ppArgValues + *pNumArgs) == NULL ) goto _errNoMemory;
	memcpy( *(ppArgValues + *pNumArgs), &theShort, lenArg);

	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE,
		"Allocated arg %d of unpacked stream at addr %p in array at %p\n",
			*pNumArgs, *(ppArgValues + *pNumArgs), ppArgValues );

/*	Show what we just unpacked from the input stream.
	------------------------------------------------- */
	UtlLog_Write(LOG_TRACE | LOG_NO_ID, func, RIGHT_HERE,
		"...unpacked a VTYPE_SHORT argument = %d\n", theShort);

/*	Indicate the type of this argument.
	----------------------------------- */
	*(pArgTypes + *pNumArgs) = VTYPE_SHORT;

/*	Update the current byte of the input stream and the
	current number or arguments.
	--------------------------------------------------- */
	*pCurrByte += lenArg;
	(*pNumArgs)++;

/*	Indicate that everything went okay.
	----------------------------------- */
	isOk = TRUE;

/*	We had an error condition.
	-------------------------- */
_errNoMemory:
	;
_errTooManyArgs:
	;

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOk);
}

/*
** Name:
**		unpack_arg_uchar
**
** Purpose:
**		Unpacks an argument of type VTYPE_UCHAR from an input stream.
**
*/
static int unpack_arg_uchar(
	char*	pStream,		/* ptr to current stream					*/
	int		useHostOrder,	/* use host ordering of bytes indicator		*/
	int*	pCurrByte,		/* ptr to current byte in the stream		*/
	int*	pNumArgs,		/* ptr to current number of arguments		*/
	short*	pArgTypes,		/* ptr to the returned array of arg types	*/
	char**	ppArgValues)	/* ptr to the returned array of arg values	*/
{
	int		isOk = FALSE;			/* return value of this function	*/
	int		lenArg;					/* length (bytes) for this argument	*/
	char*	func = "unpack_arg_uchar"; 		/* name of this function	*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Make sure we are not about to exceed the maximum number of arguments
	that are allowed to be unpacked from this stream.
	-------------------------------------------------------------------- */
	if ( *pNumArgs >= MAX_PACKED_ARGS ) {
		UtlLog_Write(LOG_ERROR, func, RIGHT_HERE,
			"ERROR: Cannot unpack this arg => exceeds max packed args (%d)\n",
				MAX_PACKED_ARGS);
		goto _errTooManyArgs;
	}

/*	Validate our input.
	------------------- */
	lenArg = sizeof(unsigned char);

/*	Allocate the memory for this argument and get it copied into the memory.
	------------------------------------------------------------------------ */
	*(ppArgValues + *pNumArgs) = (char*)malloc(lenArg);
	if ( *(ppArgValues + *pNumArgs) == NULL ) goto _errNoMemory;
	memcpy( *(ppArgValues + *pNumArgs), (pStream + *pCurrByte), lenArg);

	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE,
		"Allocated arg %d of unpacked stream at addr %p in array at %p\n",
			*pNumArgs, *(ppArgValues + *pNumArgs), ppArgValues );

/*	Show what we just unpacked from the input stream.
	------------------------------------------------- */
	UtlLog_Write(LOG_TRACE | LOG_NO_ID, func, RIGHT_HERE,
		"...unpacked a VTYPE_UCHAR argument = %d\n", *(pStream + *pCurrByte));

/*	Indicate the type of this argument.
	----------------------------------- */
	*(pArgTypes + *pNumArgs) = VTYPE_UCHAR;

/*	Update the current byte of the input stream and the
	current number or arguments.
	--------------------------------------------------- */
	*pCurrByte += lenArg;
	(*pNumArgs)++;

/*	Indicate that everything went okay.
	----------------------------------- */
	isOk = TRUE;

/*	We had an error condition.
	-------------------------- */
_errNoMemory:
	;
_errTooManyArgs:
	;

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOk);
}

/*
** Name:
**		unpack_type
**
** Purpose:
**		Unpacks an argument type from an input stream.
**
*/
static int unpack_type(
	char*	pStream,		/* ptr to current stream				*/
	int		useHostOrder,	/* use host ordering of bytes indicator	*/
	int*	pCurrByte,		/* ptr to current byte in the stream	*/
	short*	pArgType)		/* ptr to the returned argument type 	*/
{
	int		isOk = FALSE;		/* return value of this function	*/
	short	theType;			/* the type of the argument			*/
	char*	func = "unpack_type"; 		/* name of this function	*/

/*	Log a trace line indicating we are entering this function.
	---------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Unpack the type of this argument from the output stream,
	then swap the bytes, if required.
	-------------------------------------------------------- */
	memcpy(&theType, (pStream + *pCurrByte), sizeof(short));
	*pArgType = theType;
	if (!useHostOrder) {
		*pArgType = ntohs(theType);
	}

/*	Show what we just unpacked from the input stream.
	------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE,
		"Unpacked an argument type = %d\n", *pArgType);

/*	Update the current byte in the input stream.
	-------------------------------------------- */
	*pCurrByte += sizeof(short);

/*	Indicate that everything went okay.
	----------------------------------- */
	isOk = TRUE;

/*	Log a trace line indicating we are leaving this function.
	--------------------------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
	return(isOk);
}
