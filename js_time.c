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
**		utl_time.c
**
** Description:
**		This is the implementation file for the "utl_time" module of the
**		UTL subsystem, which is a collection of general-purpose utility 
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

/* include files */
/* ------------- */
#include "utl_log.h"
#include "utl_dbg.h"
#include "utl_time.h"

/* private definitions */
/* ------------------- */
#define	MAX_SOCKETS		32		/* max number of sockets for select	*/

/* private structures */
/* ------------------- */

/* private global variables */
/* ------------------------ */
int		m_MaxDaysInMonth[] =
				{
					31,		/* max days in January		*/
					28,		/* max days in February		*/
					31,		/* max days in March		*/
					30,		/* max days in April		*/
					31,		/* max days in May			*/
					30,		/* max days in June			*/
					31,		/* max days in July			*/
					31,		/* max days in August		*/
					30,		/* max days in September	*/
					31,		/* max days in October		*/
					30,		/* max days in November		*/
					31		/* max days in December		*/
				};
int		m_SizeMaxDaysInMonth = sizeof(m_MaxDaysInMonth) / sizeof(int);

/* private prototype definitions */
/* ----------------------------- */

/*
** Name:
**		UtlTime_CompareTimes
**
** Purpose:
**		This function compares an input starting and ending pair
**		of times and determines whether or not the starting
**		time is before or equal to the ending time.  In this
**		function, the times are expressed in hours, minutes,
**		and seconds.  A delta time is first added to the start
**		time before the comparison is done.  The delta time
**		(also expressed as hours, minutes, and seconds) can be
**		passed in with all values set to 0 if it is desired to
**		just compare the absolute starting and ending times.
**
** Calling Sequence:
**		okay = UtlTime_CompareTimes(
**					theStartHour, theStartMinute, theStartSecond,
**					theEndHour, theEndMinute, theEndSecond,
**					theDeltaHour, theDeltaMinute, theDeltaSecond	);
**
** Input:
**		theStartHour		the specified starting hour
**		theStartMinute		the specified starting minute
**		theStartSecond		the specified starting second
**		theEndHour			the specified ending hour
**		theEndMinute		the specified ending minute
**		theEndSecond		the specified ending second
**		theDeltaHour		the specified delta hour
**		theDeltaMinute		the specified delta minute
**		theDeltaSecond		the specified delta second
**
** Output:
**		NONE
**
** Returns:
**		okay		whether or not the starting time plus the
**					delta time is greater than the ending time:
**						TRUE  ==> start+delta time >  end time
**						FALSE ==> start+delta time <= end time
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int UtlTime_CompareTimes(
	int		theStartHour,		/* the specified starting hour		*/
	int		theStartMinute,		/* the specified starting minute	*/
	int		theStartSecond,		/* the specified starting second	*/
	int		theEndHour,			/* the specified ending hour		*/
	int		theEndMinute,		/* the specified ending minute		*/
	int		theEndSecond,		/* the specified ending second		*/
	int		theDeltaHour,		/* the specified delta hour			*/
	int		theDeltaMinute,		/* the specified delta minute		*/
	int		theDeltaSecond)		/* the specified delta second		*/
{
	int		secsStartTime;		/* number of secs since midnight for start	*/
	int		secsEndTime;		/* number of secs since midnight for end 	*/
	int		secsDeltaTime;		/* number of secs since midnight for delta	*/
	char*	func="UtlTime_CompareTimes";	/* ptr to name of func	*/

/*	Convert the starting, ending and delta times to numbers of seconds 
	since midnight.
	------------------------------------------------------------------ */
	secsStartTime = (theStartHour * SECS_PER_HOUR) + 
					(theStartMinute * SECS_PER_MIN) +
					(theStartSecond);
	secsEndTime   = (theEndHour * SECS_PER_HOUR) + 
					(theEndMinute * SECS_PER_MIN) +
					(theEndSecond);
	secsDeltaTime = (theDeltaHour * SECS_PER_HOUR) + 
					(theDeltaMinute * SECS_PER_MIN) +
					(theDeltaSecond);

/*	If the starting time is greater that the ending time, then we 
	need to add 1 day's worth of seconds, since the ending time is
	ALWAYS assumed to be ahead in time of the starting time.
	-------------------------------------------------------------- */
	if (secsStartTime > secsEndTime) {
		secsEndTime += SECS_PER_DAY;
	}

/*	Now determine if the starting time plus the delta time is
	greater than the ending time.
	----------------------------------------------------------- */
	if ( (secsStartTime + secsDeltaTime) > secsEndTime ) {
		return(TRUE);
	} else {
		return(FALSE);
	}
}

/*
** Name:
**		UtlTime_CompareTimestamps
**
** Purpose:
**		This function compares an input starting and ending pair
**		of timestamps and determines whether or not the starting
**		timestamp is before or equal to the ending timestamp.
**
** Calling Sequence:
**		okay = UtlTime_CompareTimestamps(
**					theStartYear, theStartMonth, theStartDay,
**					theStartHour, theStartMinute, theStartSecond,
**					theEndYear, theEndMonth, theEndDay,
**					theEndHour, theEndMinute, theEndSecond	);
**
** Input:
**		theStartYear		the specified starting year
**		theStartMonth		the specified starting month
**		theStartDay			the specified starting day
**		theStartHour		the specified starting hour
**		theStartMinute		the specified starting minute
**		theStartSecond		the specified starting second
**		theEndYear			the specified ending year
**		theEndMonth			the specified ending month
**		theEndDay			the specified ending day
**		theEndHour			the specified ending hour
**		theEndMinute		the specified ending minute
**		theEndSecond		the specified ending second
**
** Output:
**		NONE
**
** Returns:
**		okay		whether or not the starting timestamp is before
**					or equal to the ending timestamp:
**						TRUE  ==> starting timestamp <= ending timestamp
**						FALSE ==> starting timestamp >  ending timestamp
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int UtlTime_CompareTimestamps(
	int		theStartYear,		/* the specified starting year		*/
	int		theStartMonth,		/* the specified starting month		*/
	int		theStartDay,		/* the specified starting daya		*/
	int		theStartHour,		/* the specified starting hour		*/
	int		theStartMinute,		/* the specified starting minute	*/
	int		theStartSecond,		/* the specified starting second	*/
	int		theEndYear,			/* the specified ending year		*/
	int		theEndMonth,		/* the specified ending month		*/
	int		theEndDay,			/* the specified ending day			*/
	int		theEndHour,			/* the specified ending hour		*/
	int		theEndMinute,		/* the specified ending minute		*/
	int		theEndSecond)		/* the specified ending second		*/
{
	int			diffTime;				/* the time difference			*/
	struct tm	tmStartTime;			/* the starting time in a TM	*/
	struct tm	tmEndTime;				/* the ending time in a TM		*/
	time_t		calStartTime;			/* the calendar starting time	*/
	time_t		calEndTime;				/* the calendar ending time		*/
	char*	func="UtlTime_CompareTimestamps";	/* ptr to name of func	*/

/*	Get the specified starting time and ending time into their
	respective TM structures so we can determine their equivalent
	calendar times.
	------------------------------------------------------------- */
	tmStartTime.tm_year  = theStartYear - TM_BASE_YEAR;
	tmStartTime.tm_mon   = theStartMonth - 1;
	tmStartTime.tm_mday  = theStartDay;
	tmStartTime.tm_hour  = theStartHour;
	tmStartTime.tm_min   = theStartMinute;
	tmStartTime.tm_sec   = theStartSecond;
	tmStartTime.tm_isdst = -1;

	tmEndTime.tm_year  = theEndYear - TM_BASE_YEAR;
	tmEndTime.tm_mon   = theEndMonth - 1;
	tmEndTime.tm_mday  = theEndDay;
	tmEndTime.tm_hour  = theEndHour;
	tmEndTime.tm_min   = theEndMinute;
	tmEndTime.tm_sec   = theEndSecond;
	tmEndTime.tm_isdst = -1;

/*	Now convert the TM versions of the starting timestamp and the
	ending timestamp into their equivalent calender times (which
	is the number of seconds since January 1, 1970).
	------------------------------------------------------------- */
	calStartTime = mktime(&tmStartTime);
	calEndTime   = mktime(&tmEndTime);

/*	Now determine if the starting calendar time is less than or
	equal to the ending calendar time (Note that we do not need
	to be concerned about the time zone because we are only 
	interested in RELATIVE calendar times).
	----------------------------------------------------------- */
	diffTime = (int)difftime(calStartTime, calEndTime);
	if ( diffTime <= 0 ) {
		return(TRUE);
	} else {
		return(FALSE);
	}
}

/*
** Name:
**		UtlTime_dysize
**
** Purpose:
**		Returns the number of days in the given year (either 365 or 366).
**		This function emulates the UNIX system function "dysize" that is
**		found on some UNIX platforms, but not others.
**
** Calling Sequence:
**		nDays = UtlTime_dysize(int theYear);
**
** Input:
**		theYear				the specified year
**
** Output:
**		NONE
**
** Returns:
**		nDays		the returned number of days for the given year
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int UtlTime_dysize(
	int		theYear)		/* the specified year	*/
{
	int		nDaysInFeb;		/* the number of days in February	*/

/*	Get the number of days in February for the specified year.  If
	the number of days is 29, we return the number of days in a
	non-leap year.  Otherwise, we return the number of days in a 
	leap year.
	--------------------------------------------------------------- */
	nDaysInFeb = UtlTime_GetMaxDay(FEBRUARY_MONTH, theYear);
	return (nDaysInFeb == DAYS_LEAP_FEBRUARY) ?
								DAYS_LEAP_YEAR : DAYS_NORMAL_YEAR;
}

/*
** Name:
**		UtlTime_FormatTime
**
** Purpose:
**		This function formats the current time as specified by
**		an input format string and fills a return buffer with the
**		formatted version of the time.  The caller also specifies
**		whether or not the time should be formatted as either LOCAL
**		or GMT time.
**
** Calling Sequence:
**		okay = UtlTime_FormatTime(fmtAsLocal, pTimeFormat, sizeTimeBuff,
**								  pTimeBuff);
**
** Input:
**		fmtAsLocal			format the time as LOCAL indicator
**		pTimeFormat			ptr to an input time format string
**		sizeTimeBuff		size in bytes of the return time buffer
**
** Output:
**		pTimeBuff			ptr to returned time buffer with the formatted
**							local time
**
** Returns:
**		okay			whether or not the local time was formatted
**						successfully or not:
**							TRUE  => the local time was formatted okay
**							FALSE => the local time was NOT formatted okay
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int UtlTime_FormatTime(
	int			fmtAsLocal,		/* format the time as LOCAL indicator		*/
	char*		pTimeFormat,	/* ptr to an input time format string		*/
	int			sizeTimeBuff,	/* size in bytes of the return time buffer	*/
	char*		pTimeBuff)		/* ptr to returned time buffer 				*/
{
	time_t	currTime;						/* the current timestamp		*/
	char	timeBuff[MAX_TIMESTAMP_LEN];	/* the timestamp as a string	*/
	char*	func="UtlTime_FormatTime";		/* ptr to name of func			*/

/*	Get the current time and format it into a local buffer according
	to the input time format string.
	---------------------------------------------------------------- */
	currTime = time(NULL);
	if (fmtAsLocal) {
		strftime(timeBuff, sizeof(timeBuff), pTimeFormat,
				 localtime(&currTime));
		strcat(timeBuff, " (LOCAL)");
	} else {
		strftime(timeBuff, sizeof(timeBuff), pTimeFormat,
				 gmtime(&currTime));
		strcat(timeBuff, " (GMT)");
	}

/*	If the caller's return buffer is large enough, copy the local
	formatted time buffer to the return buffer and return a good
	status (TRUE). Otherwise, just return a bad status (FALSE).
	------------------------------------------------------------- */
	if (strlen(timeBuff) < (size_t)sizeTimeBuff) {
		strcpy(pTimeBuff, timeBuff);
		return(TRUE);
	} else {
		return(FALSE);
	}
}

/*
** Name:
**		UtlTime_FormattedToBrokenDown
**
** Purpose:
**		Converts a formatted time into the equivalent
**		broken-down time.
**
** Calling Sequence:
**		UtlTime_FormattedToBrokenDown(pTimeFormatted, pFormat, pTimeTm);
**
** Input:
**		pTimeFormatted	A pointer to the formatted time string
**						to be converted.
**		pFormat			A pointer to the time format string.
**
** Output:
**		pTimeTm			A pointer to the returned time in
**						broken-down format.
**
** Returns:
**		NONE
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
void UtlTime_FormattedToBrokenDown(
	char*		pTimeFormatted,	/* Ptr to formatted time to be converted	*/
	char*		pFormat,		/* Ptr to the time format string			*/
	struct tm*	pTimeTm)		/* Ptr to returned time broken-down			*/
{
	char*	func="UtlTime_FormattedToBrokenDown";	/* ptr to name of func	*/

/*	Convert the input timestamp string to its equivalent
	broken down time.  Note that we make the assumption
	that the time format string does NOT include any
	references to the "seconds" information, so we must
	set this value of the TM structure to 0 before we
	return to the caller.
	---------------------------------------------------- */
#if 0
#ifndef _QNX_SOURCE
	strptime(pTimeFormatted, pFormat, pTimeTm);	
	pTimeTm->tm_sec = 0;
#endif
#endif
}

/*
** Name:
**		UtlTime_GetMaxDay
**
** Purpose:
**		This function determines the maximum valid day for a 
**		specified month and year.
**
** Calling Sequence:
**		maxDay = UtlTime_GetMaxDay(theMonth, theYear);
**
** Input:
**		theMonth			the specified month
**		theYear				the specified year
**
** Output:
**		NONE
**
** Returns:
**		maxDay			the maximum valid day for the specified month
**						and year:
**							> 0  ==> the maximum valid day (range 1 to 31)
**							== 0 ==> the input month or year was not valid
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
int UtlTime_GetMaxDay(
	int			theMonth,		/* the specified month	*/
	int			theYear)		/* the specified year	*/
{
	int			maxDay;					/* the maximum day for a month	*/
	char*	func="UtlTime_GetMaxDay";	/* ptr to name of func			*/

/*	Verify the input month and year look reasonable.  If not, just
	return a bad value (0) as the maximum day.
	-------------------------------------------------------------- */
	if ( (theMonth <= 0) || (theMonth > m_SizeMaxDaysInMonth) ||
		 (theYear <= 0) ) {
		return(0);
	}

/*	If the specified month is NOT February, then just return the
	max day for the specified month from our local table.
	------------------------------------------------------------- */
	if ( theMonth != FEBRUARY_MONTH ) {
		return(m_MaxDaysInMonth[theMonth-1]);
	}

/*	The specified month IS February, so we must apply the special
	leap-year logic to determine the ACTUAL max day for this month.
	--------------------------------------------------------------- */
	maxDay = m_MaxDaysInMonth[theMonth-1];
	if ( (theYear % 4) == 0 ) {
		if ( (theYear % 100) != 0 ) {
			maxDay++;
		} else if ( (theYear % 400) == 0 ) {
			maxDay++;
		}
	}
	return(maxDay);
}

/*
** Name:
**		UtlTime_GmtMinusLocalSecs
**
** Purpose:
**		Returns the number of seconds by which local time is west of
**		Greenwich Mean Time (GMT).  That is, returns the difference
**		(gmt - local) in seconds, where 'gmt' and 'local' are the GMT
**		and local representations of a given instant in time.
**
** Calling Sequence:
**		secs = UtlTime_GmtMinusLocalSecs();
**
** Input:
**		NONE
**
** Output:
**		NONE
**
** Returns:
**		The difference (gmt - local) in seconds
**
** Error Conditions:
**		NONE
**
** Comments:
**		Note that if the local time zone is actually EAST of Greenwich,
**		then the returned difference (gmt - local) will be NEGATIVE.
*/
time_t UtlTime_GmtMinusLocalSecs()
{
	time_t		tRef;				/* the reference time					*/
	time_t		tRefPlusTzDiff;		/* reference time + timezone diff		*/
	time_t		tzDiff;				/* the timezone diff					*/
	struct tm	tRefBrokenDown;		/* the reference time as broken-down	*/
	char*		func = "UtlTime_GmtMinusLocalSecs";

/*	Get the current calendar time, expressed as the number of seconds
	elapsed since Jan 1, 1970, GMT.  This is our reference time tRef.
	----------------------------------------------------------------- */
	time(&tRef);

/*	Express the reference time in broken-down format, GMT.  Let's call
	this structure tRefBrokenDown.
	------------------------------------------------------------------ */
	tRefBrokenDown = *gmtime(&tRef);
	tRefBrokenDown.tm_isdst = -1;

/*	Now, what happens if we apply mktime() to tRefBrokenDown?  Well, we
	know that mktime() expects its input param to be in LOCAL time.
	So, mktime(tRefBrokenDown - tzDiff) should give us tRef, since
	(tRefBrokenDown - tzDiff) is tRef expressed in local time. (Here,
	tzDiff is the timezone difference, i.e. GMT - local.)  Therefore,
	mktime(tRefBrokenDown) should give us tRef + tzDiff.
	--------------------------------------------------------------------- */
	tRefPlusTzDiff = mktime(&tRefBrokenDown);

/*	Now, we simply compute tzDiff = (tRef + tzDiff) - tRef.
	------------------------------------------------------- */
#if 0
	tzDiff = difftime(tRefPlusTzDiff, tRef);
#endif

	return tzDiff;
}

/*
** Name:
**		UtlTime_GmtToLocalBrokenDown
**
** Purpose:
**		Converts a broken-down GMT time into the equivalent
**		broken-down local time.
**
** Calling Sequence:
**		UtlTime_GmtToLocalBrokenDown(&gmtTime, &localTime);
**
** Input:
**		pGmtTime		A pointer to the broken-down GMT time
**						to be converted.
**
** Output:
**		pLocalTime		A pointer to a structure that will
**						receive the equivalent broken-down
**						local time.
**
** Returns:
**		NONE
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
void UtlTime_GmtToLocalBrokenDown(
	struct tm* pGmt,	/* Ptr to broken-down GMT time to be converted		*/
	struct tm* pLocal)	/* Ptr to struct to receive equivalent local time	*/
{
	time_t		nSecs;		/* the number of secs since the epoch	*/

/*	Convert the broken-down GMT time to secs since the epoch, GMT time.
	------------------------------------------------------------------- */
	nSecs = UtlTime_timegm(pGmt);

/*	Convert secs since the epoch to broken-down local time.
	------------------------------------------------------- */
	*pLocal = *localtime(&nSecs);
}

/*
** Name:
**		UtlTime_GmtToLocalFormatted
**
** Purpose:
**		Converts a formatted GMT time into the equivalent
**		formatted local time.
**
** Calling Sequence:
**		UtlTime_GmtToLocalFormatted(pGmt, lenTime, pFormat,
**									pLocal, pLocalTm);
**
** Input:
**		pGmt			A pointer to the formatted GMT time string
**						to be converted.
**		lenTime			The length of the formatted time strings.
**		pFormat			A pointer to the time format string.
**
** Output:
**		pLocal			A pointer to the returned string that will
**						receive the equivalent formatted local time.
**		pLocalTm		A pointer to the returned local time in
**						broken down format (for convenience).
**
** Returns:
**		NONE
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
void UtlTime_GmtToLocalFormatted(
	char*		pGmt,		/* Ptr to formatted GMT time to be converted	*/
	int			lenTime,	/* The length of the formatted time strings		*/
	char*		pFormat,	/* Ptr to the time format string				*/
	char*		pLocal,		/* Ptr to returned formatted local time			*/
	struct tm*	pLocalTm)	/* Ptr to returned local time broken down		*/
{
#if 0
	struct tm				tmGmt;			/* the time as a GMT tm		*/
#endif
	char*	func="UtlTime_GmtToLocalFormatted";	/* ptr to name of func	*/

/*	Convert the input GMT timestamp string to its equivalent
	broken down time, convert the GMT broken down time to a
	local broken down time, and then reformat the new local
	broken down time into a new local timestamp string using
	the same format string associated with the original input
	GMT timestamp string.
	--------------------------------------------------------- */
#if 0
#ifndef _QNX_SOURCE
	strptime(pGmt, pFormat, &tmGmt);	
	tmGmt.tm_sec = 0;
	UtlTime_GmtToLocalBrokenDown(&tmGmt, pLocalTm);
	memset(pLocal, 0, lenTime);
	strftime(pLocal, lenTime, pFormat, pLocalTm);
#endif
#endif
}

/*
** Name:
**		UtlTime_GmtToLocalHourMin
**
** Purpose:
**		Converts an hour:min GMT time into the equivalent
**		hour:min LOCAL time.
**
** Calling Sequence:
**		UtlTime_GmtToLocalHourMin(hourGmt, minGmt, &hourLocal, &minLocal);
**
** Input:
**		hourGmt		The hour (0 - 23), GMT time.
**		minGmt		The minute (0 - 59), GMT time.
**
** Output:
**		hourLocal	The hour (0 - 23), Local time.
**		minLocal	The minute (0 - 59), Local time.
**
** Returns:
**		NONE
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
void UtlTime_GmtToLocalHourMin(
	char hourGmt,		/* The hour, GMT time (0 - 23)						*/
	char minGmt,		/* The minute, GMT time (0 - 59)					*/
	char* pHourLocal,	/* Ptr to hour of equivalent local time (0 - 23)	*/
	char* pMinLocal) 	/* Ptr to minute of equivalent local time (0 - 59)	*/
{
	int		secsGmt;		/* number of seconds after midnight, GMT	*/
	int		secsLocal;		/* number of seconds after midnight, local	*/
	int		tzDiff;			/* the timezone difference					*/

/*	Convert input time to seconds after midnight, GMT time.
	------------------------------------------------------- */
	secsGmt = (hourGmt * SECS_PER_HOUR) + (minGmt * SECS_PER_MIN);

/*	Convert again to seconds after midnight, LOCAL time.
	---------------------------------------------------- */
	tzDiff = UtlTime_GmtMinusLocalSecs();
	secsLocal = secsGmt - tzDiff;
	if (secsLocal < 0)
		secsLocal += SECS_PER_DAY;

/*	Convert again to hour:min, LOCAL time.
	-------------------------------------- */
	*pHourLocal = secsLocal / SECS_PER_HOUR;
	*pMinLocal = (secsLocal - (*pHourLocal * SECS_PER_HOUR)) / SECS_PER_MIN;
}

/*
** Name:
**		UtlTime_LocalToGmtBrokenDown
**
** Purpose:
**		Converts a broken-down local time into the equivalent
**		broken-down GMT time.
**
** Calling Sequence:
**		UtlTime_LocalToGmtBrokenDown(&gmtTime, &localTime);
**
** Input:
**		pLocalTime		A pointer to the broken-down local time
**						to be converted.
**
** Output:
**		pGmtTime		A pointer to a structure that will
**						receive the equivalent broken-down
**						GMT time.
**
** Returns:
**		NONE
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
void UtlTime_LocalToGmtBrokenDown(
	struct tm* pLocal,	/* Ptr to broken-down local time to be converted	*/
	struct tm* pGmt)	/* Ptr to struct to receive equivalent GMT time		*/
{
	time_t		tLocal;		/* the local time as number of secs since epoch	*/

/*	First, we convert the local broken down time to the number of 
	seconds since the beginning of time (Jan 1, 1970 at 00:00:00).
	-------------------------------------------------------------- */
	tLocal = mktime(pLocal);

/*	Finally, we convert this tLocal time to a broken-down GMT time.
	--------------------------------------------------------------- */
	*pGmt = *gmtime(&tLocal);
}

/*
** Name:
**		UtlTime_LocalToGmtFormatted
**
** Purpose:
**		Converts a formatted local time into the equivalent
**		formatted GMT time.
**
** Calling Sequence:
**		UtlTime_LocalToGmtFormatted(pLocal, lenTime, pFormat,
**									pGmt, pGmtTm);
**
** Input:
**		pLocal			A pointer to the formatted local time string
**						to be converted.
**		lenTime			The length of the formatted time strings.
**		pFormat			A pointer to the time format string.
**
** Output:
**		pGmt			A pointer to the returned string that will
**						receive the equivalent formatted GMT time.
**		pGmtTm			A pointer to the returned GMT time in
**						broken down format (for convenience).
**
** Returns:
**		NONE
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
void UtlTime_LocalToGmtFormatted(
	char*		pLocal,		/* Ptr to formatted local time to be converted	*/
	int			lenTime,	/* The length of the formatted time strings		*/
	char*		pFormat,	/* Ptr to the time format string				*/
	char*		pGmt,		/* Ptr to returned formatted GMT time			*/
	struct tm*	pGmtTm)		/* Ptr to returned GMT time broken down			*/
{
#if 0
	struct tm				tmLocal;		/* the time as a LOCAL tm	*/
#endif
	char*	func="UtlTime_LocalToGmtFormatted";	/* ptr to name of func	*/

/*	Convert the input local timestamp string to its equivalent
	broken down time, convert the local broken down time to a
	GMT broken down time, and then reformat the new GMT
	broken down time into a new GMT timestamp string using
	the same format string associated with the original input
	local timestamp string.
	---------------------------------------------------------- */
#if 0
#ifndef _QNX_SOURCE
	strptime(pLocal, pFormat, &tmLocal);	
	tmLocal.tm_sec = 0;
	UtlTime_LocalToGmtBrokenDown(&tmLocal, pGmtTm);
	memset(pGmt, 0, lenTime);
	strftime(pGmt, lenTime, pFormat, pGmtTm);
#endif
#endif
}

/*
** Name:
**		UtlTime_LocalToGmtHourMin
**
** Purpose:
**		Converts an hour:min LOCAL time into the equivalent
**		hour:min GMT time.
**
** Calling Sequence:
**		UtlTime_LocalToGmtHourMin(hourLocal, minLocal, &hourGmt, &minGmt);
**
** Input:
**		hourLocal	The hour (0 - 23), Local time.
**		minLocal	The minute (0 - 59), Local time.
**
** Output:
**		hourGmt		The hour (0 - 23), GMT time.
**		minGmt		The minute (0 - 59), GMT time.
**
** Returns:
**		NONE
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
void UtlTime_LocalToGmtHourMin(
	char hourLocal,		/* The hour, local time (0 - 23)					*/
	char minLocal,		/* The minute, local time (0 - 59)					*/
	char* pHourGmt,		/* Ptr to hour of equivalent GMT time (0 - 23)		*/
	char* pMinGmt)		/* Ptr to minute of equivalent GMT time (0 - 59)	*/
{
	int		secsGmt;		/* number of seconds after midnight, GMT	*/
	int		secsLocal;		/* number of seconds after midnight, local	*/
	int		tzDiff;			/* the timezone difference					*/

/*	Convert input time to seconds after midnight, local time.
	--------------------------------------------------------- */
	secsLocal = (hourLocal * SECS_PER_HOUR) + (minLocal * SECS_PER_MIN);

/*	Convert again to seconds after midnight, GMT time.
	-------------------------------------------------- */
	tzDiff = UtlTime_GmtMinusLocalSecs();
	secsGmt = secsLocal + tzDiff;
	if (secsGmt >= SECS_PER_DAY)
		secsGmt -= SECS_PER_DAY;

/*	Convert again to hour:min, GMT time.
	------------------------------------ */
	*pHourGmt = secsGmt / SECS_PER_HOUR;
	*pMinGmt = (secsGmt - (*pHourGmt * SECS_PER_HOUR)) / SECS_PER_MIN;
}

/*
** Name:
**			UtlTime_MonthDayToJulian
**
** Purpose:
**			Given a month (0 - 11), a day of that month (1 - 31),
**			and a year, returns the corresponding Julian date (0 - 365).
*/
int UtlTime_MonthDayToJulian(int month, int day, int year)
{
	int		nDays;			/* number of Julian days (0-based)	*/
	int		i;				/* loop counter						*/

/*	Initialize nDays to zero.
	------------------------- */
	nDays = 0;

/*	For i = 0 to (month - 1)...
	--------------------------- */
	for (i=0; i<month; i++)
	{
/*		Set nDays += (number of days in month #i).
		------------------------------------------ */
		nDays += UtlTime_GetMaxDay(i + 1, year);
	}

/*	Set nDays += day.  (Now nDays contains the 1-based Julian date.)
	---------------------------------------------------------------- */
	nDays += day;

/*	Decrement nDays (so it contains the 0-based Julian date) and
	return it to the caller.
	------------------------------------------------------------ */
	return --nDays;
}

/*
** Name:
**			UtlTime_Sleep
**
** Purpose:
**			This local function sleeps for a specified number
**			of seconds and microseconds, using SELECT to put
**			this process to sleep.
**
** Calling Sequence:
**		UtlTime_Sleep(numSecs, numUsecs);
**
** Input:
**		numSecs			number of seconds to sleep
**		numUsecs		number of microseconds to sleep
**
** Output:
**		NONE
**
** Returns:
**		NONE
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
void UtlTime_Sleep(
	unsigned long	numSecs,		/* number of seconds to sleep		*/
	long			numUsecs)		/* number of microseconds to sleep	*/
{
	char*	func = "UtlTime_Sleep";	/* ptr to name of func			*/

/*	Allow for tracing information on ENTRY.
	--------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "Entering...\n");

/*	Sleep for the desired number of milliseconds.
	--------------------------------------------- */
	Sleep( numSecs*1000 + numUsecs/1000 );

/*	Allow for tracing information on EXIT.
	-------------------------------------- */
	UtlLog_Write(LOG_TRACE, func, RIGHT_HERE, "...leaving\n");
}

/*
** Name:
**			UtlTime_timegm
**
** Purpose:
**			Converts a broken-down GMT time to seconds since Jan. 1,
**			1970 at 00:00:00 GMT time.
**
**			This function emulates the UNIX system function "timegm"
**			that is found on some UNIX platforms, but not others.
**
** Calling Sequence:
**		nSecs = UtlTime_timegm(struct tm* pTmGmt);
**
** Input:
**		pTmGmt			ptr to a broken-down GMT time
**
** Output:
**		NONE
**
** Returns:
**		nSecs		the returned number of seconds since Jan. 1, 1970
**					at 00:00:00 GMT time
**
** Error Conditions:
**		NONE
**
** Comments:
**		NONE
*/
time_t UtlTime_timegm(struct tm* pTmGmt)
{
	int			year;		/* the number of the current year			*/
	int			nDays;		/* the number of days since the epoch		*/
	int			i;			/* loop counter								*/
	time_t		nSecs;		/* the number of seconds since the epoch	*/

/*	Compute the number of days that have elapsed from Jan 1 of 1970
	(start of the "epoch") to Dec 31 of the year prior to the given year.
	--------------------------------------------------------------------- */
	year = TM_BASE_YEAR + pTmGmt->tm_year;
	nDays = 0;
	for (i=EPOCH_BASE_YEAR; i<year; i++)
	{
		nDays += UtlTime_dysize(i);
	}

/*	Add in the number of full days that have elapsed so far in the
	given year.
	-------------------------------------------------------------- */
	pTmGmt->tm_yday = UtlTime_MonthDayToJulian(
		pTmGmt->tm_mon, pTmGmt->tm_mday, year);
	nDays += pTmGmt->tm_yday;

/*	Convert the total number of elapsed days to seconds; then add in
	the seconds that have elapsed so far in the given day.  The result
	is the total number of seconds elapsed since Jan 1, 1970.
	------------------------------------------------------------------ */
	nSecs = (nDays * SECS_PER_DAY) +
			(pTmGmt->tm_hour * SECS_PER_HOUR) +
			(pTmGmt->tm_min * SECS_PER_MIN) +
			(pTmGmt->tm_sec);

	return nSecs;
}
