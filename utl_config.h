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
**      utl_config_multi.h
**
** Description:
**      This is the header file for "utl_config_multi.c", which is used
**		to read configuration information from one of a series of
**		configuration files and set global variables that are either
**		strings or numeric values or arrays of these types of variables.
**
** Subsystem:
**      UTL
**
** Version:
**      1.00
**
** Public Functions:
**      UtlConfigMulti_Close
**      UtlConfigMulti_DefineItem
**      UtlConfigMulti_GetItemSize
**      UtlConfigMulti_Initialize
**      UtlConfigMulti_Load
**      UtlConfigMulti_Open
**      UtlConfigMulti_Terminate
**      UtlConfigMulti_Unload
**
** History:
**      Date        Engineer            Purpose
**      ----        ------------        -------------------------------
**      06/01/96    Steven M. Rosen     Initial creation.
*/

/* include guard END */
/* ----------------- */
#ifndef _UTL_CONFIG_MULTI_H_INCLUDED

/* include files */
/* ------------- */
#include <windows.h>

/* public definitions */
/* ------------------ */
#define TRUE        1               /* the value is TRUE    */
#define FALSE       0               /* the value is FALSE   */

/* public structures */
/* ----------------- */

/* public global variables */
/* ----------------------- */

/* public prototype definitions */
/* ---------------------------- */
int	UtlConfigMulti_Close(
		HANDLE	hConfigFile);	/* handle to configuration file				*/

int	UtlConfigMulti_DefineItem(
		HANDLE	hConfigFile,	/* handle to configuration file				*/
		char**	ppItemVal,		/* ptr to ptr to the config item value		*/
		char*	pItemID);		/* ptr to identifier string for config item	*/

int UtlConfigMulti_GetItemSize(
		HANDLE	hConfigFile,	/* handle to configuration file				*/
		char*	pItemID);		/* ptr to the config item identifier string	*/

int	UtlConfigMulti_Initialize();

int UtlConfigMulti_Load(
		HANDLE	hConfigFile);	/* handle to configuration file				*/

int	UtlConfigMulti_Open(
		char*	pConfigFileName,/* ptr to name of the configuration file	*/
		HANDLE*	phConfigFile);	/* ptr to returned configuration file handle*/

int	UtlConfigMulti_Terminate();

int UtlConfigMulti_Unload(
		HANDLE	hConfigFile);	/* handle to configuration file				*/

/* include guard END */
/* ----------------- */
#define _UTL_CONFIG_MULTI_H_INCLUDED
#endif
                                    
