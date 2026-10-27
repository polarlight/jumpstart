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
**		utl_time.h
**
** Description:
**		This is the header file for the "utl_time" module of the UTL
**		subsystem, which is a collection of general-purpose utility 
**		functions that are used by all other subsystems.  This module
**		contains library functions concerned with the conversion of
**		timestamps from one format and/or time zone into another.
**
** Subsystem:
**      UTL
**
** Version:
**      1.00
**
** Public Functions:
**		UtlTime_CompareTimes
**		UtlTime_CompareTimestamps
**		UtlTime_dysize
**		UtlTime_FormatTime
**		UtlTime_FormattedToBrokenDown
**		UtlTime_GetMaxDay
**		UtlTime_GmtMinusLocalSecs
**		UtlTime_GmtToLocalBrokenDown
**		UtlTime_GmtToLocalFormatted
**		UtlTime_GmtToLocalHourMin
**		UtlTime_LocalToGmtBrokenDown
**		UtlTime_LocalToGmtFormatted
**		UtlTime_LocalToGmtHourMin
**		UtlTime_MonthDayToJulian
**		UtlTime_Sleep
**		UtlTime_timegm
**
** History:
**		Date		Engineer			Purpose
**		--------	----------------	---------------------------------
**		03/17/96	Richard D. Klein	Initial creation.
*/

/* include guard START */
/* ------------------- */
#ifndef _UTL_TIME_H_INCLUDED

/* include files */
/* ------------- */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <WINSOCK.H>

/* public definitions */
/* ------------------ */
#define	DAYS_LEAP_YEAR		366			/* days in a leap year				*/
#define	DAYS_NORMAL_YEAR	365			/* days in a normal year			*/
#define	DAYS_LEAP_FEBRUARY	29			/* days in a FEBRUARY leap year		*/
#define	EPOCH_BASE_YEAR		1970		/* base year for beginning of time	*/
#define	FEBRUARY_MONTH		2			/* month of the year for FEBRUARY	*/
#define HOURS_PER_DAY		24			/* number of hours in a day			*/
#define	MAX_TIMESTAMP_LEN	32			/* maximum length of a timestamp	*/
#define	MINS_PER_HOUR		60			/* number of minutes in 1 hour		*/
#define	SECS_PER_MIN		60			/* number of seconds in 1 minute	*/
#define	TM_BASE_YEAR		1900		/* base year for TM-type timestamps	*/

#define SECS_PER_HOUR	(MINS_PER_HOUR * SECS_PER_MIN)	/* seconds per hour	*/
#define SECS_PER_DAY	(HOURS_PER_DAY * SECS_PER_HOUR)	/* seconds per day	*/

/* public structures */
/* ----------------- */

/* public global variables */
/* ----------------------- */

/* public prototype definitions */
/* ---------------------------- */
int		UtlTime_CompareTimes(
							int				theStartHour,
							int				theStartMinute,
							int				theStartSecond,
							int				theEndHour,
							int				theEndMinute,
							int				theEndSecond,
							int				theDeltaHour,
							int				theDeltaMinute,
							int				theDeltaSecond	);

int		UtlTime_CompareTimestamps(
							int				theStartYear,
							int				theStartMonth,
							int				theStartDay,
							int				theStartHour,
							int				theStartMinute,
							int				theStartSecond,
							int				theEndYear,
							int				theEndMonth,
							int				theEndDay,
							int				theEndHour,
							int				theEndMinute,
							int				theEndSecond	);

int		UtlTime_dysize(
							int				theYear			);

int		UtlTime_FormatTime(
							int				fmtAsLocal,
							char*			pTimeFormat,
							int				sizeTimeBuff,
							char*			pTimeBuff		);

void	UtlTime_FormattedToBrokenDown(
							char*			pTimeFormatted,
							char*			pFormat,
							struct tm*		pTimeTm			);

int		UtlTime_GetMaxDay(
							int				theMonth,
							int				theYear			);

time_t	UtlTime_GmtMinusLocalSecs(
															);

void	UtlTime_GmtToLocalBrokenDown(
							struct tm*		pGmt,
							struct tm*		pLocal			);

void	UtlTime_GmtToLocalFormatted(
							char*			pGmt,
							int				lenTime,
							char*			pFormat,
							char*			pLocal,
							struct tm*		pLocalTm			);

void	UtlTime_GmtToLocalHourMin(
							char			hourGmt,
							char			minGmt,
							char*			pHourLocal,
							char*			pMinLocal		);

void	UtlTime_LocalToGmtBrokenDown(
							struct tm*		pLocal,
							struct tm*		pGmt			);

void	UtlTime_LocalToGmtFormatted(
							char*			pLocal,
							int				lenTime,
							char*			pFormat,
							char*			pGmt,
							struct tm*		pGmtTm			);

void	UtlTime_LocalToGmtHourMin(
							char			hourLocal,
							char			minLocal,
							char*			pHourGmt,
							char*			pMinGmt			);

int		UtlTime_MonthDayToJulian(
							int				month,
							int				day,
							int				year			);

void	UtlTime_Sleep(
							unsigned long	numSecs,
							long			numUsecs		);

time_t	UtlTime_timegm(
							struct tm*		pTmGmt			);

/* include guard END */
/* ----------------- */
#define _UTL_TIME_H_INCLUDED
#endif
