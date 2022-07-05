/***************************************************************/
/* When.c                                                      */
/***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <math.h>

#include "types.h"
#include "decas.h"
#include "when.h"

#if IS_WINDOWS
    #include <time.h>
    #define WIN32_LEAN_AND_MEAN
    #include <Windows.h>
#else
    #include <unistd.h>
    #include <utime.h>
    #include <time.h>
    #include <sys/time.h>
    #include <sys/times.h>
    #include <sys/resource.h>
#endif

#if IS_MACINTOSH
    #include <mach/clock.h>
    #include <mach/mach.h>
#endif

struct d_interval {
    int     in_years;       /* difference in year only */
    int     in_months;      /* difference in month only. Range -11 to 11 */
    DATINT  in_seconds;     /* total seconds in interval */
};

#define CAL_TESTING     0
#define CAL_TESTING_BIG 1
#define CAL_TEST_TIME   1

#if CAL_TESTING
    void testcal(void);

    #if CAL_TESTING_BIG
        #define CAL_TEST_MIN_YEAR       (-5100)
        #define CAL_TEST_MAX_YEAR       (2200)
        #define CAL_TEST_FIRST_MONTH    1
        #define CAL_TEST_FIRST_DAY      1
        #define CAL_TEST_LAST_MONTH     12
        #define CAL_TEST_LAST_DAY       31
    #else
        #define CAL_TEST_MIN_YEAR       (-5100)
        #define CAL_TEST_MAX_YEAR       (CAL_TEST_MIN_YEAR + 0)
        #define CAL_TEST_FIRST_MONTH    1
        #define CAL_TEST_FIRST_DAY      1
        #define CAL_TEST_LAST_MONTH     12
        #define CAL_TEST_LAST_DAY       31
    #endif

    #define CAL_TEST_MAX_ERRS           10

#if CAL_TEST_TIME
    #define CAL_TEST_HOUR               9
    #define CAL_TEST_MINUTE             10
    #define CAL_TEST_SECOND             12
#else
    #define CAL_TEST_HOUR               0
    #define CAL_TEST_MINUTE             0
    #define CAL_TEST_SECOND             0
#endif

#endif

#define CAL_REVISION_YEAR           1752
#define CAL_REVISION_MONTH          9
#define CAL_REVISION_MONTH_LAST_DAY 30
#define CAL_REVISION_DAY            2
#define CAL_REVISION_JDAY           246
#define CAL_REVISION_NDAYS          11
#define CAL_MIN_YEAR                (-6000)
#define CAL_MAX_YEAR                9999
#define CAL_YEAR_BIAS               6400
#define CAL_YEAR_NULL               (CAL_MIN_YEAR - 1)

#define CAL_NDY1                    365L      /* Days in 1 year    */
#define CAL_NDY4                    1461L     /* Days in 4 years   */
#define CAL_NDY100                  36524L    /* Days in 100 years */
#define CAL_NDY400                  146097L   /* Days in 400 years */
#define CAL_NDY100_LY4              36525L    /* Days in 100 years with leap year every 4 years */
#define CAL_NDY400_LY4              146100L   /* Days in 400 years with leap year every 4 years */
#define CAL_NDY1600_LY4  (4L * CAL_NDY400_LY4) /* Days in 1600 years with leap year every 4 years */
#define CAL_NDY4_NLY                1460L     /* Days in 4 years with no leap year */

#define CAL_YEAR_BIAS_DAYS          ((CAL_YEAR_BIAS / 400L) * CAL_NDY400_LY4)
#define CAL_YEAR_BIAS_SECS          ((d_date)CAL_YEAR_BIAS_DAYS * (d_date)SECS_IN_DAY)
#define CAL_REVISION_DAY_NUMBER     (CAL_YEAR_BIAS_DAYS + 640164L)
#define CAL_JULIAN_DIFF             (CAL_YEAR_BIAS_DAYS - 1721056L)

#ifdef UNNEEDED
/* #define CAL_JULIAN_DIFF             (0) */

/*
**  Notes:
**      5 B.C. - a leap year starting on Saturday of the Proleptic Julian calendar
**      4 B.C. - a common year starting on Monday of the Proleptic Julian calendar
**      1 B.C. - a leap year starting on Thursday of the Proleptic Julian calendar
**      0      - Year zero does not exist 
**      1 A.D. - a common year starting on Saturday of the Proleptic Julian calendar
**
**      The Julian Day Number starts at noon on January 1, 4713 BC proleptic Julian calendar.
**      The Julian day number for 13 September 2012 is 2456184.
**      The Julian day number for 2007-02-10 CE 18:53 GMT is 2,454,142.2867 AJD
**      See: http://en.wikipedia.org/wiki/Julian_day
*/

/*
**      http://en.wikipedia.org/wiki/Modulo_operation

#define IDIVMO(q,r,a,b) idivmo(&q,&r,a,b)

int idivfloor(int a, int b) {

    int q;

    if (!b) return;

    q = a / b;

    if (a < 0) {
    } else

}

*/
/***************************************************************/
#define DIVMO(q,r,a,b) {q=a/b;r=a%b;if(r&&((a<0&&b>0)||(a>0&&b<0))){q--;r+=b;}}
void idivmo(int * q, int * r, int a, int b) {
/*
** Integer division and modulus (correct regardless of sign of a and b)
**
**      Does nothing at all if b is 0
**      C operators / and % are correct if sign(a) == sign(b)
**      If there is a remainder and sign(a) != sign(b), then b needs to be added to
**      the remainder and the quotient needs to be decremented.        
**      See: http://en.wikipedia.org/wiki/Modulo_operation
**
** Returns:
**  (*q) = floor(a / b)
**  (*r) = a - b * floor(a / b)     <-- Always same sign as b
*/

    if (b > 0) {
        (*q) = a / b;
        (*r) = a % b;
        if ((*r) && a < 0) {
            (*r) += b;
            (*q)--;
        }
    } else if (b < 0) {
        (*q) = a / b;
        (*r) = a % b;
        if ((*r) && a > 0) {
            (*r) += b;
            (*q)--;
        }
    }
}
/***************************************************************/
void ii64divmo(int64 * q, int * r, int64 a, int b) {
/*
** Returns:
**  (*q) = floor(a / b)
**  (*r) = a - b * floor(a / b)
*/

    if (b > 0) {
        (*q) = (int)(a / (int64)b);
        (*r) = (int)(a % (int64)b);
        if ((*r) && a < 0) {
            (*q)--;
            (*r) += b;
        }
    } else if (b < 0) {
        (*q) = (int)(a / (int64)b);
        (*r) = (int)(a % (int64)b);
        if ((*r) && a > 0) {
            (*q)--;
            (*r) += b;
        }
    }
}
/***************************************************************/
void i64divmo(int64 * q, int64 * r, int64 a, int64 b) {
/*
** Returns:
**  (*q) = floor(a / b)
**  (*r) = a - b * floor(a / b)
*/

    if (b > 0) {
        (*q) = (int)(a / (int64)b);
        (*r) = (int)(a % (int64)b);
        if ((*r) && a < 0) {
            (*q)--;
            (*r) += b;
        }
    } else if (b < 0) {
        (*q) = (int)(a / (int64)b);
        (*r) = (int)(a % (int64)b);
        if ((*r) && a > 0) {
            (*q)--;
            (*r) += b;
        }
    }
}
/***************************************************************/
int get_year(void) {

    static int g_start_year = 0;
    struct tm * lt;
    time_t ct;

    if (!g_start_year) {
        ct = time(NULL);
        lt = localtime(&ct);
        g_start_year = lt->tm_year;
    }

    return g_start_year;
}
/***************************************************************/
void glob_init_time(void) {

    TZSET();   /* Appears to do nothing at all on Windows */
}
/***************************************************************/
int gooddate(int year, int month, int day) {
/*
                               1752                               

       January               February                 March       
Su Mo Tu We Th Fr Sa   Su Mo Tu We Th Fr Sa   Su Mo Tu We Th Fr Sa
          1  2  3  4                      1    1  2  3  4  5  6  7
 5  6  7  8  9 10 11    2  3  4  5  6  7  8    8  9 10 11 12 13 14
12 13 14 15 16 17 18    9 10 11 12 13 14 15   15 16 17 18 19 20 21
19 20 21 22 23 24 25   16 17 18 19 20 21 22   22 23 24 25 26 27 28
26 27 28 29 30 31      23 24 25 26 27 28 29   29 30 31

        April                   May                   June        
Su Mo Tu We Th Fr Sa   Su Mo Tu We Th Fr Sa   Su Mo Tu We Th Fr Sa
          1  2  3  4                   1  2       1  2  3  4  5  6
 5  6  7  8  9 10 11    3  4  5  6  7  8  9    7  8  9 10 11 12 13
12 13 14 15 16 17 18   10 11 12 13 14 15 16   14 15 16 17 18 19 20
19 20 21 22 23 24 25   17 18 19 20 21 22 23   21 22 23 24 25 26 27
26 27 28 29 30         24 25 26 27 28 29 30   28 29 30
                       31
        July                  August                September     
Su Mo Tu We Th Fr Sa   Su Mo Tu We Th Fr Sa   Su Mo Tu We Th Fr Sa
          1  2  3  4                      1          1  2 14 15 16
 5  6  7  8  9 10 11    2  3  4  5  6  7  8   17 18 19 20 21 22 23
12 13 14 15 16 17 18    9 10 11 12 13 14 15   24 25 26 27 28 29 30
19 20 21 22 23 24 25   16 17 18 19 20 21 22
26 27 28 29 30 31      23 24 25 26 27 28 29
                       30 31
       October               November               December      
Su Mo Tu We Th Fr Sa   Su Mo Tu We Th Fr Sa   Su Mo Tu We Th Fr Sa
 1  2  3  4  5  6  7             1  2  3  4                   1  2
 8  9 10 11 12 13 14    5  6  7  8  9 10 11    3  4  5  6  7  8  9
15 16 17 18 19 20 21   12 13 14 15 16 17 18   10 11 12 13 14 15 16
22 23 24 25 26 27 28   19 20 21 22 23 24 25   17 18 19 20 21 22 23
29 30 31               26 27 28 29 30         24 25 26 27 28 29 30
                                              31
*/

    int out;

    if (!year || year < CAL_MIN_YEAR || year > CAL_MAX_YEAR) {
        out = 0;
    } else if (month < 1 || month > 12) {
        out = 0;
    } else if (day < 1 || day > 31) {
        out = 0;

    } else if (year < CAL_REVISION_YEAR) {
        if (year < 0) year++; /* no year 0, years 1 BC, 5 BC, etc. are leap years */
        if (day > 30 &&
                   (month == 4 || month == 6 || month == 9 || month == 11)) {
            out = 0;
        } else if (day > 29 && month == 2) {
            out = 0;
        } else if (day > 28 && month == 2 && (year & 3)) {
            out = 0;
        } else {
            out = 1;
        }

    } else if (year == CAL_REVISION_YEAR) {
        if (year == CAL_REVISION_MONTH) {
            if (day <= CAL_REVISION_DAY) {
                out = 1;
            } else if (day > CAL_REVISION_DAY + CAL_REVISION_NDAYS && 
                       day <= CAL_REVISION_MONTH_LAST_DAY) {
                out = 1;
            } else {
                out = 0;
            }
        } else if (day > 30 &&
                   (month == 4 || month == 6 || month == 9 || month == 11)) {
            out = 0;
        } else if (day > 29 && month == 2) {
            out = 0;
        } else if (day > 28 && month == 2 && (year & 3)) {
            out = 0;
        } else {
            out = 1;
        }

    } else {    /* (year > CAL_REVISION_YEAR) */
        if (day > 30 &&
                   (month == 4 || month == 6 || month == 9 || month == 11)) {
            out = 0;
        } else if (day > 29 && month == 2) {
            out = 0;
        } else if (day > 28 && month == 2) {
            if (year & 3) {
                out = 0;
            } else {
                if (!(year % 100) && (year % 400)) {
                    out = 0;
                } else {
                    out = 1;
                }
            }
        } else {
            out = 1;
        }
    }

    return (out);
}
/***************************************************************/
int goodtime(int hour, int minute, int second) {

    int out;

    if (hour < 0 || hour > 23) {
        out = 0;
    } else if (minute < 0 || minute > 59) {
        out = 0;
    } else if (second < 0 || second > 59) {
        out = 0;
    } else {
        out = 1;
    }

    return (out);
}
#endif /* UNNEEDED */
/***************************************************************/
void timestamp_diff(struct timestamp * ts1,
                    struct timestamp * ts2,
                    struct timestamp * tsr) {
    
    tsr->ts_microsec = ts1->ts_microsec - ts2->ts_microsec;
    tsr->ts_second   = ts1->ts_second   - ts2->ts_second;
    tsr->ts_daynum   = ts1->ts_daynum   - ts2->ts_daynum;
    
    while (tsr->ts_microsec < 0) {
        tsr->ts_microsec += 1000000L;
        tsr->ts_second   -= 1;
    }
    
    while (tsr->ts_second < 0) {
        tsr->ts_second += 86400L;
        tsr->ts_daynum -= 1;
    }
}
/***************************************************************/
/*
    Get CPU Time
        Windows:
            void WINAPI GetLocalTime(
              __out  LPSYSTEMTIME lpSystemTime
            );
            BOOL WINAPI GetSystemTimes(
              __out_opt  LPFILETIME lpIdleTime,
              __out_opt  LPFILETIME lpKernelTime,
              __out_opt  LPFILETIME lpUserTime
            );
        Posix:
          int clock_gettime(clockid_t clk_id, struct timespect *tp);
                //  include time.h and to link to librt.a
          int  getrusage(int who, struct rusage *r_usage);
*/
/***************************************************************/
#if IS_WINDOWS
/***************************************************************/
void systemtime_to_timestamp(const SYSTEMTIME * systim, struct timestamp * ts) {

	ts->ts_daynum   = (int)ymd_to_daynum(systim->wYear, systim->wMonth, systim->wDay);
	ts->ts_second   = systim->wHour * 3600  + systim->wMinute * 60 + systim->wSecond;
    ts->ts_microsec = systim->wMilliseconds * 1000;
}
/***************************************************************/
void filetime_to_timestamp(const FILETIME * filtim, struct timestamp * ts) {

	SYSTEMTIME systim;
    FILETIME locfiltim;

    if (!FileTimeToLocalFileTime(filtim, &locfiltim)) {
        systim.wYear         = 1988;
        systim.wMonth        = 8;
        systim.wDay          = 8;
        systim.wHour         = 0;
        systim.wMinute       = 0;
        systim.wSecond       = 0;
        systim.wMilliseconds = 0;
    } else {
	    if (!FileTimeToSystemTime(&locfiltim, &systim)) {
            systim.wYear         = 1977;
            systim.wMonth        = 7;
            systim.wDay          = 7;
            systim.wHour         = 0;
            systim.wMinute       = 0;
            systim.wSecond       = 0;
            systim.wMilliseconds = 0;
	    }
    }
    systemtime_to_timestamp(&systim, ts);
}
/***************************************************************/
void filetimeticks_to_timestamp(const FILETIME * filtim, struct timestamp * ts) {

    int64 ft;

    ft = (int64)(filtim->dwHighDateTime) << 32 | (int64)(filtim->dwLowDateTime);
    ft /= 10; /* convert 100ns to microsecs */

    ts->ts_microsec = (int)(ft % 1000000L);
    ft /= 1000000L; /* convert microsecs to secs */

    ts->ts_second   = (int)(ft % 86400L);
    ts->ts_daynum   = (int)(ft / 86400L);
}
/***************************************************************/
void get_cpu_timer(struct proctimes * pt) {

	SYSTEMTIME locsystim;
    FILETIME creationTime;
    FILETIME exitTime;
    FILETIME kernelTime;
    FILETIME userTime;
/*
BOOL WINAPI GetProcessTimes(
  __in   HANDLE hProcess,
  __out  LPFILETIME lpCreationTime,
  __out  LPFILETIME lpExitTime,
  __out  LPFILETIME lpKernelTime,
  __out  LPFILETIME lpUserTime
);
HANDLE WINAPI GetCurrentProcess(void);
*/
    GetLocalTime(&locsystim);
    systemtime_to_timestamp(&locsystim, &(pt->pt_wall_time));

    if (!GetProcessTimes(GetCurrentProcess(),
            &creationTime, &exitTime, &kernelTime, &userTime)) {
        memset(&(pt->pt_cpu_time), 0, sizeof(struct timestamp));
        memset(&(pt->pt_sys_time), 0, sizeof(struct timestamp));
    } else {
        filetimeticks_to_timestamp(&userTime, &(pt->pt_cpu_time));
        filetimeticks_to_timestamp(&kernelTime, &(pt->pt_sys_time));
    }

    if (pt->pt_use_hi_p) {
        LARGE_INTEGER hiprec_counter;
        if (!QueryPerformanceCounter(&hiprec_counter)) {
            pt->pt_use_hi_p = 0;
        } else {
            memcpy(&(pt->pt_hi_p), &(hiprec_counter.QuadPart), sizeof(hiprec_counter.QuadPart));
        }
#if IS_DEBUG
    if (sizeof(hiprec_counter.QuadPart) != sizeof(int64)) {
        fprintf(stderr, "** Timer size mismatch **\n"); fflush(stderr);
    }
#endif
    }

}
/***************************************************************/
int get_frequency_hi_timer(int64 * hi_freq) {

	int rtn;
    LARGE_INTEGER hiprec_freq;

	rtn = 0;

    if (!QueryPerformanceFrequency(&hiprec_freq)) {
        rtn = -1;
    } else {
    	(*hi_freq) = hiprec_freq.QuadPart;
   }

	return (rtn);
}
#else
/***************************************************************/
#if IS_MACINTOSH
/***************************************************************/
int get_hi_timespec(struct timespec * ts) {

    clock_serv_t cclock;
    mach_timespec_t mts;

    host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);
    ts->tv_sec = mts.tv_sec;
    ts->tv_nsec = mts.tv_nsec;
    
    return (0);
}
#else
int get_hi_timespec(struct timespec * ts) {

    return (clock_gettime(CLOCK_MONOTONIC, ts));
}
#endif
/***************************************************************/
void timeval_to_timestamp(const struct timeval * tv, struct timestamp * ts) {
    
	ts->ts_daynum   = (Int)((tv->tv_sec / SECS_IN_DAY) + TIME_D_DATE_DAYS_DIFF);
	ts->ts_second   = tv->tv_sec % SECS_IN_DAY;
    ts->ts_microsec = tv->tv_usec;
}
/***************************************************************/
void get_cpu_timer(struct proctimes * pt) {

	struct timeval tv;
	struct rusage pusage;

	if (!gettimeofday(&tv, NULL)) {
		timeval_to_timestamp(&tv, &(pt->pt_wall_time));
	} else {
		memset(&(pt->pt_wall_time), 0, sizeof(struct timestamp));
	}

	if (!getrusage(RUSAGE_SELF, &pusage)) {
		timeval_to_timestamp(&(pusage.ru_utime), &(pt->pt_cpu_time));
		timeval_to_timestamp(&(pusage.ru_stime), &(pt->pt_sys_time));
	} else {
		memset(&(pt->pt_cpu_time), 0, sizeof(struct timestamp));
		memset(&(pt->pt_sys_time), 0, sizeof(struct timestamp));
	}

    if (pt->pt_use_hi_p) {
    	 struct timespec ts;
    	 /* clock_gettime(CLOCK_REALTIME, &ts); */
    	 if (!get_hi_timespec(&ts)) {
    	     pt->pt_hi_p = (int64)(1000000000L) * (int64)(ts.tv_sec) + (int64)(ts.tv_nsec);
    	 } else {
    		 pt->pt_hi_p = 0;
    	 }
    }
}
/***************************************************************/
int get_frequency_hi_timer(int64 * hi_freq) {
/*
 * To check precision of timer
 * :
 *     struct timespec getres_freq;
 *     clock_getres(CLOCK_MONOTONIC, &getres_freq);
 */

    (*hi_freq) = 1000000000LL; /* clock_gettime(CLOCK_MONOTONIC) is always nanosecs */

	return (0);
}
#endif /* IS_WINDOWS */
/***************************************************************/
void start_cpu_timer(struct proctimes * pt_start, int use_hi_p) {

    pt_start->pt_use_hi_p = use_hi_p;

    get_cpu_timer(pt_start);
}
/***************************************************************/
void stop_cpu_timer(struct proctimes * pt_start,
                    struct proctimes * pt_end,
                    struct proctimes * pt_result) {

	int64 hi_freq;

	pt_end->pt_use_hi_p = pt_start->pt_use_hi_p;
    get_cpu_timer(pt_end);

    timestamp_diff(&(pt_end->pt_wall_time), &(pt_start->pt_wall_time), &(pt_result->pt_wall_time));
    timestamp_diff(&(pt_end->pt_cpu_time) , &(pt_start->pt_cpu_time) , &(pt_result->pt_cpu_time));
    timestamp_diff(&(pt_end->pt_sys_time) , &(pt_start->pt_sys_time) , &(pt_result->pt_sys_time));

    pt_result->pt_use_hi_p = 0;
    if (pt_start->pt_use_hi_p && pt_end->pt_use_hi_p) {
        pt_result->pt_hi_p = pt_end->pt_hi_p - pt_start->pt_hi_p;
        if (get_frequency_hi_timer(&hi_freq)) {
            pt_result->pt_use_hi_p = 0;
        } else {
            pt_result->pt_use_hi_p = 1;

            pt_result->pt_hi_elap_secs =
                (double)(pt_result->pt_hi_p) / (double)(hi_freq);
        }
    }
}
/***************************************************************/
d_daynum ymd_to_daynum(int year, int month, int day) {
/*
** Day 0 - Friday December 31, 1999
**
**      #define CAL_YEAR_BIAS               6400
**      09/02/1752 --> 2977764
**      09/14/1752 --> 2977765
**      12/31/1752 --> 2977873
**      12/31/1755 --> 2978968
**      12/31/1799 --> 2995039
**      12/31/1999 --> 3068087
*/
    int dn;
    int tt;
    int adj;
    int byear;
    int quad;
    int yquad;
    int cent;
    int ycent;
    d_daynum daynum;

    adj = 0;

    if (year <= CAL_REVISION_YEAR) {
        byear = year + CAL_YEAR_BIAS;
        if (year < 0) byear++;

        dn = (byear / 4) * CAL_NDY4;
        tt = byear & 3;
        dn += (tt * 365);
        if (tt || month > 2) dn++;
        if (year == CAL_REVISION_YEAR) {
            if (month > CAL_REVISION_MONTH ||
                (month == CAL_REVISION_MONTH && day > CAL_REVISION_DAY)) {
                adj += CAL_REVISION_NDAYS;
            }
        }
    } else {
        byear = year - 1600;
        adj += CAL_REVISION_NDAYS;

        quad  = byear / 400;
        yquad = byear - (quad * 400);
        cent  = yquad / 100;
        ycent = yquad - (cent * 100);
        tt    = ycent & 3;

        if (!cent) {
            dn  =  (ycent / 4) * CAL_NDY4;
            dn += (tt * 365);
            if (tt || month > 2) dn++;
        } else if (ycent < 4) {
            dn  = 365 * ycent + 1;
        } else {
            dn  =  (ycent / 4) * CAL_NDY4;
            dn += (tt * 365);
            if (tt || month > 2) dn++;
        }

        dn += (quad * CAL_NDY400);
        dn += (cent * CAL_NDY100);
        dn += CAL_NDY1600_LY4;
        dn += CAL_YEAR_BIAS_DAYS;
        dn += 1;    /* for Feb 29, 1700 */
   }

    switch (month) {
        case  1:            break;  case  2: dn +=  31; break;
        case  3: dn +=  59; break;  case  4: dn +=  90; break;
        case  5: dn += 120; break;  case  6: dn += 151; break;
        case  7: dn += 181; break;  case  8: dn += 212; break;
        case  9: dn += 243; break;  case 10: dn += 273; break;
        case 11: dn += 304; break;  case 12: dn += 334; break;
        default:;
    }

    daynum = (dn + day - adj);

    return (daynum - CAL_JULIAN_DIFF);
}
/***************************************************************/
#ifdef UNNEEDED
d_sec hms_to_sec(int hour, int minute, int second) {
/*
** Second 0 is at midnight
*/
    d_sec sec;

    sec = (hour * 3600) + (minute * 60) + second;

    return (sec);
}
/***************************************************************/
void sec_to_hms(d_sec sec, int * hour, int * minute, int * second) {

    *hour   = (int)(sec / 3600);
    *minute = (int)((sec / 60  ) - (*hour * 60));
    *second = (int)(sec - (*hour * 3600) - (*minute * 60));
}
/***************************************************************/
void daynum_to_ymd(d_daynum daynum, int * year, int * month, int * day) {

    int yy;
    int mm;
    int dd;
    int leap;
    int quad;
    int dquad;
    int cent;
    int dcent;
    d_daynum adaynum;

    adaynum  = daynum + CAL_JULIAN_DIFF;

    if (adaynum <= CAL_REVISION_DAY_NUMBER) {
        leap = 0;
        yy = ((adaynum-1) / CAL_NDY4);
        dd = (adaynum) - (yy * CAL_NDY4);
        yy = (yy * 4) - CAL_YEAR_BIAS;
             if (dd > 1096) { yy += 3; dd -= 1096; }
        else if (dd > 731)  { yy += 2; dd -= 731;  }
        else if (dd > 366)  { yy += 1; dd -= 366;  }
        else leap = 1;

             if (dd > 334 + leap) { mm = 12; dd -= (334 + leap); }
        else if (dd > 304 + leap) { mm = 11; dd -= (304 + leap); }
        else if (dd > 273 + leap) { mm = 10; dd -= (273 + leap); }
        else if (dd > 243 + leap) { mm =  9; dd -= (243 + leap); }
        else if (dd > 212 + leap) { mm =  8; dd -= (212 + leap); }
        else if (dd > 181 + leap) { mm =  7; dd -= (181 + leap); }
        else if (dd > 151 + leap) { mm =  6; dd -= (151 + leap); }
        else if (dd > 120 + leap) { mm =  5; dd -= (120 + leap); }
        else if (dd > 90  + leap) { mm =  4; dd -= (90  + leap); }
        else if (dd > 59  + leap) { mm =  3; dd -= (59  + leap); }
        else if (dd > 31        ) { mm =  2; dd -= (31        ); }
        else                      { mm =  1;                     }
    } else {
        adaynum -= CAL_YEAR_BIAS_DAYS;
        adaynum += CAL_REVISION_NDAYS;
        adaynum -= CAL_NDY1600_LY4;
        adaynum--;    /* for Feb 29, 1700 */
        quad   = (adaynum-1) / CAL_NDY400;
        dquad  = adaynum - (quad * CAL_NDY400);
        if (dquad <= CAL_NDY100_LY4) {
            leap = 0;
            yy = ((dquad-1) / CAL_NDY4);
            dd = (dquad) - (yy * CAL_NDY4);
            yy *= 4;
                 if (dd > 1096) { yy += 3; dd -= 1096; }
            else if (dd > 731)  { yy += 2; dd -= 731;  }
            else if (dd > 366)  { yy += 1; dd -= 366;  }
            else leap = 1;
            yy += 1600 + (400 * quad);
        } else {
            dquad  -= CAL_NDY100_LY4;
            cent    = (dquad - 1) / CAL_NDY100;
            dcent   = dquad - (cent * CAL_NDY100);
            leap    = 0;

            if (dcent <= CAL_NDY4_NLY) {
                yy = (dcent-1) / CAL_NDY1;
                dd = dcent - (yy * CAL_NDY1);
                yy += 1700 + (100 * cent) + (400 * quad);
            } else {
                dcent  -= CAL_NDY4_NLY;
                yy = (dcent-1) / CAL_NDY4;
                dd = dcent - (yy * CAL_NDY4);
                yy = 1704 + (yy * 4) + (100 * cent) + (400 * quad);
                     if (dd > 1096) { yy += 3; dd -= 1096; }
                else if (dd > 731 ) { yy += 2; dd -= 731 ; }
                else if (dd > 366 ) { yy += 1; dd -= 366 ; }
                else leap = 1;
            }
        }

             if (dd > 334 + leap) { mm = 12; dd -= (334 + leap); }
        else if (dd > 304 + leap) { mm = 11; dd -= (304 + leap); }
        else if (dd > 273 + leap) { mm = 10; dd -= (273 + leap); }
        else if (dd > 243 + leap) { mm =  9; dd -= (243 + leap); }
        else if (dd > 212 + leap) { mm =  8; dd -= (212 + leap); }
        else if (dd > 181 + leap) { mm =  7; dd -= (181 + leap); }
        else if (dd > 151 + leap) { mm =  6; dd -= (151 + leap); }
        else if (dd > 120 + leap) { mm =  5; dd -= (120 + leap); }
        else if (dd > 90  + leap) { mm =  4; dd -= (90  + leap); }
        else if (dd > 59  + leap) { mm =  3; dd -= (59  + leap); }
        else if (dd > 31        ) { mm =  2; dd -= (31        ); }
        else                      { mm =  1;                     }
    }

    *year  = yy;
    if (*year <= 0) (*year)--;
    *month = mm;
    *day   = dd;
}
/***************************************************************/
void d_date_to_ymdhms(d_date ddt,
            int * year, int * month, int * day,
            int * hour, int * minute, int * second) {

    d_daynum daynum;
    int64    idaynum;
    d_sec    sec;

    ii64divmo(&idaynum, &sec, ddt, SECS_IN_DAY);
    daynum = (d_daynum)idaynum;

    daynum_to_ymd(daynum, year, month, day);
    sec_to_hms(sec, hour, minute, second);
}
/***************************************************************/
void d_date_to_ymd(d_date ddt, int * year, int * month, int * day) {

    int hh;
    int mi;
    int ss;

    d_date_to_ymdhms(ddt, year, month, day, &hh, &mi, &ss);
}
/***************************************************************/
void d_date_to_hms(d_date ddt,
            int * hour, int * minute, int * second) {

    int yy;
    int mm;
    int dd;

    d_date_to_ymdhms(ddt, &yy, &mm, &dd, hour, minute, second);
}
/***************************************************************/
int to_ymd(int year, int month, int day) {
/*
*/
    int yyyymmdd;

    yyyymmdd = (year * 10000) + (month * 100) + day;

    return (yyyymmdd);
}
/***************************************************************/
void from_ymd(int yyyymmdd, int * year, int * month, int * day) {

    *year   = (yyyymmdd / 10000);
    *month  = (yyyymmdd / 100  ) - (*year * 100);
    *day    = yyyymmdd - (*year * 10000) - (*month * 100);
}
/***************************************************************/
int ymd_to_dayofweek(int year, int month, int day) {
/*
*/
    d_daynum daynum;
    int dow;

    daynum = ymd_to_daynum(year, month, day);
    dow = (daynum + CAL_JULIAN_DIFF + 4) % 7;

    return (dow);
}
/***************************************************************/
d_date ymd_sec_to_d_date(d_ymd ymd, d_sec sec) {

    int yy;
    int mm;
    int dd;
    d_daynum daynum;

    from_ymd(ymd, &yy, &mm, &dd);
    daynum = ymd_to_daynum(yy, mm, dd);

    return (daynum * SECS_IN_DAY + sec);
}
/***************************************************************/
d_date tm_to_d_time(struct tm * lt) {

    d_daynum daynum;

    daynum = ymd_to_daynum(FTM_YEAR(lt->tm_year), FTM_MONTH(lt->tm_mon), FTM_DAY(lt->tm_mday));

    return (daynum * SECS_IN_DAY + hms_to_sec(lt->tm_hour, lt->tm_min, lt->tm_sec));
}
/***************************************************************/
d_date make_d_date(int year, int month, int day, int hour, int minute, int second) {
/*
*/
    d_daynum daynum;

    daynum = ymd_to_daynum(year, month, day);

    return ((d_date)daynum * SECS_IN_DAY + hms_to_sec(hour, minute, second));
}
/***************************************************************/
d_date make_d_date_ymd(int year, int month, int day) {
/*
*/
    d_daynum daynum;
    d_date   daynumsecs;

    daynum = ymd_to_daynum(year, month, day);
    daynumsecs = (d_date)daynum * SECS_IN_DAY;

    return (daynumsecs);
}
/***************************************************************/
#ifdef OLD_WAY
d_date time_t_to_d_time(time_t tt) {
/*
**  #define TIMEZONE _timezone
**  #define DAYLIGHT _daylight
**  
**  #define TIME_D_DATE_DIFF         (TIME_D_DATE_DIFF_NO_TZ - ((TIMEZONE) - 3600 * (DAYLIGHT)))
**  #define TIME_D_DATE_DIFF_DST(d)  (TIME_D_DATE_DIFF_NO_TZ - ((TIMEZONE) - 3600 * (d)))
**
** On 4/1/2013 (Daylight in effect) values on Windows XP are:
**      tz      = 28800
**      dl      = 1
**      tzn0    = "Pacific Standard Time"
**      tzn1    = "Pacific Daylight Time"
**   	tzi.Bias            = 480
**   	tzi.DaylightBias    = -60
**      tzi rtn             = 2 (TIME_ZONE_ID_DAYLIGHT)
**
** On 4/1/2013 backdated system to 3/1/2013(Daylight not in effect)
**  and got same values on Windows XP:
**      tz      = 28800
**      dl      = 1
**      tzn0    = "Pacific Standard Time"
**      tzn1    = "Pacific Daylight Time"
**   	tzi.Bias            = 480
**   	tzi.DaylightBias    = -60
**      tzi rtn             = 1 (TIME_ZONE_ID_STANDARD)
**
** was until 11/2012:
**     return (d_date)(tt + TIME_D_DATE_DIFF);
**
** was until 4/1/2013:
**     return (d_date)(tt + TIME_D_DATE_DIFF_DST(0));
**
** on 4/1/2013 changed back to:
**     return (d_date)(tt + TIME_D_DATE_DIFF);
*/

    return (d_date)(tt + TIME_D_DATE_DIFF_DST(0));
}
#endif
d_date time_t_to_d_time(time_t tt, int is_dst) {

    return (d_date)(tt + TIME_D_DATE_DIFF_DST(is_dst));
}
/***************************************************************/
d_date time_t_to_d_time_dst(time_t tt, int dst) {

    return (d_date)(tt + TIME_D_DATE_DIFF_DST(dst));
}
/***************************************************************/
d_date get_time_now(int is_dst) {

    return (time_t_to_d_time(time(NULL), is_dst));
}
/***************************************************************/
time_t d_date_to_time_t(d_date dd) {

    return (time_t)(dd + (d_date)TIME_D_DATE_DIFF);
}
/***************************************************************/
time_t d_date_to_time_t_no_tz(d_date dd) {

    return (time_t)(dd + (d_date)TIME_D_DATE_DIFF_NO_TZ);
}
/***************************************************************/
d_doy to_day_of_year(int year, int month, int day) {
/*
** Returns 1 - 366
*/
    d_doy dn;
    int tdays[]   = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

    if (year < 0) year++;
    if (month >= 1 && month <= 12) dn = tdays[month-1] + day;
    else dn = 0;
    if (!(year & 3) &&
        ((year < CAL_REVISION_YEAR) || (year % 100) || !(year % 400)) && month > 2) dn++;

    if (year == CAL_REVISION_YEAR) {
        if (month > CAL_REVISION_MONTH ||
            (month == CAL_REVISION_MONTH && day > CAL_REVISION_DAY)) {
            dn -= CAL_REVISION_NDAYS;
        }
    }

    return (dn);
}
/***************************************************************/
void from_day_of_year(int year, d_doy jday, int * month, int * day) {

    int ndays[]   = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int nlydays[] = {0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    int mm;
    int jj;

    jj = jday;
    mm = 1;
    if (year < 0) year++;

    if (year == CAL_REVISION_YEAR) {
        if (jj > CAL_REVISION_JDAY) {
            jj += CAL_REVISION_NDAYS;
        }
    }

    if (!(year & 3) && ((year < CAL_REVISION_YEAR) || !(year % 400) || (year % 100))) {
        while (jj > nlydays[mm]  && mm < 12) { jj -= nlydays[mm]; mm++; }
    } else {
        while (jj > ndays[mm]  && mm < 12) { jj -= ndays[mm]; mm++; }
    }

    *month = mm;
    *day   = jj;
}
/***************************************************************/
int fmt_d_date(d_date ddt, const char * fmt, char *buf, int bufsize) {
/*
** Similar to strftime(), fmt_d_interval()
**
** See also: parsedate(), parse_interval()
**
** Supported formats:
    %%  Percent sign
    %a  Abbreviated weekday name
    %b  Abbreviated month name
    %C  BC/AD indicator for year                            -- not in strftime()
    %d  Day of month as decimal number (01 - 31)
*    %E Day number from Jan 1, 1900 as in Microsoft Excel**   -- not in strftime()
    %H  Hour in 24-hour format (00 - 23)
    %I  Hour in 12-hour format (01 - 12)
    %j  Day of year as decimal number (001 � 366)
    %J  Julian day beginning January 1, 4713 BC             -- not in strftime()
    %m  Month as decimal number (01 - 12)
    %H  Hour in 24-hour format (00 - 23)
    %p  AM/PM indicator for 12-hour clock
    %S  Second as decimal number (00 - 59)
    %te Time of day from 12:00 AM as decimal (Excel format) -- not in strftime()
    %tj Time of day from 12:00 PM as decimal (Julian time)  -- not in strftime()
    %y  Year without century, as decimal number (00 - 99)
    %Y  Year with century, as decimal number

** Unupported formats:
    %A  Full weekday name
    %B  Full month name
    %c  Date and time representation appropriate for locale
    %U  Week of year as decimal number, with Sunday as first day of week (00 � 53)
    %w  Weekday as decimal number (0 � 6; Sunday is 0)
    %W  Week of year as decimal number, with Monday as first day of week (00 � 53)
    %x  Date representation for current locale
    %X  Time representation for current locale
    %z  Time zone abbreviation; no characters if time zone is unknown
    %Z  Time-zone name; no characters if time zone is unknown

* - Unimplemented
** - Microsoft Excel thinks 2/29/1900 is a date

*/
    int ii;
    int ix;
    int bx;
    int yy;
    int mm;
    int dd;
    int hh;
    int mi;
    int ss;
    int day;
    int tsec;
    double t_x;
    double fl;
    int fmterr;
    char tb[32];

    d_date_to_ymdhms(ddt, &yy, &mm, &dd, &hh, &mi, &ss);

    fmterr = 0;
    ix = 0;
    bx = 0;
    while (fmt[ix]) {
        if (fmt[ix] == '%' && fmt[ix+1]) {
            ix++;
            switch (fmt[ix]) {
                case '%':
                    if (bx + 1 >= bufsize) {
                        fmterr |= 2; /* overflow */
                    } else {
                        buf[bx++] = '%';
                    }
                    break;

                case 'A':
                    if (bx + 9 >= bufsize) {
                        fmterr |= 2; /* overflow */
                    } else {
                        day = ymd_to_dayofweek(yy, mm, dd);
                        switch (day) {
                            case DOW_SUNDAY     : strcpy(tb, "Sunday");     break;
                            case DOW_MONDAY     : strcpy(tb, "Monday");     break;
                            case DOW_TUESDAY    : strcpy(tb, "Tuesday");    break;
                            case DOW_WEDNESDAY  : strcpy(tb, "Wednesday");  break;
                            case DOW_THURSDAY   : strcpy(tb, "Thursday");   break;
                            case DOW_FRIDAY     : strcpy(tb, "Friday");     break;
                            case DOW_SATURDAY   : strcpy(tb, "Saturday");   break;
                            default : ;
                        }
                        for (ii = 0; tb[ii]; ii++) {
                            buf[bx] = tb[ii];
                            bx++;
                        }
                    }
                    break;

                case 'a':
                    if (bx + 3 >= bufsize) {
                        fmterr |= 2; /* overflow */
                    } else {
                        day = ymd_to_dayofweek(yy, mm, dd);
                        switch (day) {
                            case DOW_SUNDAY     : strcpy(tb, "Sun"); break;
                            case DOW_MONDAY     : strcpy(tb, "Mon"); break;
                            case DOW_TUESDAY    : strcpy(tb, "Tue"); break;
                            case DOW_WEDNESDAY  : strcpy(tb, "Wed"); break;
                            case DOW_THURSDAY   : strcpy(tb, "Thu"); break;
                            case DOW_FRIDAY     : strcpy(tb, "Fri"); break;
                            case DOW_SATURDAY   : strcpy(tb, "Sat"); break;
                            default : ;
                        }
                        for (ii = 0; tb[ii]; ii++) {
                            buf[bx] = tb[ii];
                            bx++;
                        }
                    }
                    break;

                case 'B':
                    if (bx + 8 >= bufsize) {
                        fmterr |= 2; /* overflow */
                    } else {
                        switch (mm) {
                            case  1 : strcpy(tb, "January");    break;
                            case  2 : strcpy(tb, "February");   break;
                            case  3 : strcpy(tb, "March");      break;
                            case  4 : strcpy(tb, "April") ;     break;
                            case  5 : strcpy(tb, "May");        break;
                            case  6 : strcpy(tb, "June");       break;
                            case  7 : strcpy(tb, "July");       break;
                            case  8 : strcpy(tb, "August");     break;
                            case  9 : strcpy(tb, "Sepember");   break;
                            case 10 : strcpy(tb, "October");    break;
                            case 11 : strcpy(tb, "November");   break;
                            case 12 : strcpy(tb, "December");   break;
                            default : ;
                        }
                        for (ii = 0; tb[ii]; ii++) {
                            buf[bx] = tb[ii];
                            bx++;
                        }
                    }
                    break;

                case 'b':
                    if (bx + 3 >= bufsize) {
                        fmterr |= 2; /* overflow */
                    } else {
                        switch (mm) {
                            case  1 : strcpy(tb, "Jan"); break;
                            case  2 : strcpy(tb, "Feb"); break;
                            case  3 : strcpy(tb, "Mar"); break;
                            case  4 : strcpy(tb, "Apr"); break;
                            case  5 : strcpy(tb, "May"); break;
                            case  6 : strcpy(tb, "Jun"); break;
                            case  7 : strcpy(tb, "Jul"); break;
                            case  8 : strcpy(tb, "Aug"); break;
                            case  9 : strcpy(tb, "Sep"); break;
                            case 10 : strcpy(tb, "Oct"); break;
                            case 11 : strcpy(tb, "Nov"); break;
                            case 12 : strcpy(tb, "Dec"); break;
                            default : ;
                        }
                        for (ii = 0; tb[ii]; ii++) {
                            buf[bx] = tb[ii];
                            bx++;
                        }
                    }
                    break;

                case 'C':
                    if (bx + 2 >= bufsize) {
                        fmterr |= 2; /* overflow */
                    } else {
                        if (yy < 0) {
                            buf[bx++] = 'B';
                            buf[bx++] = 'C';
                        } else {
                            buf[bx++] = 'A';
                            buf[bx++] = 'D';
                        }
                    }
                    break;

                case 'd':
                    if (bx + 2 >= bufsize) {
                        fmterr |= 2; /* overflow */
                    } else {
                        buf[bx++] = (dd / 10) + '0';
                        buf[bx++] = (dd % 10) + '0';
                    }
                    break;

                case 'E':
                    if (bx + 5 >= bufsize) {
                        fmterr |= 2; /* overflow */
                    } else {
                        day = ymd_to_daynum(yy, mm, dd) - JULIAN_EXCEL_DIFF;
                        sprintf(tb, "%05d", day);
                        for (ii = 0; tb[ii]; ii++) {
                            buf[bx] = tb[ii];
                            bx++;
                        }
                    }
                    break;

                case 'j':
                    if (bx + 3 >= bufsize) {
                        fmterr |= 2; /* overflow */
                    } else {
                        day = ymd_to_daynum(yy, mm, dd) - ymd_to_daynum(yy, 1, 1) + 1;
                        sprintf(tb, "%03d", day);
                        for (ii = 0; tb[ii]; ii++) {
                            buf[bx] = tb[ii];
                            bx++;
                        }
                    }
                    break;

                case 'J':
                    if (bx + 7 >= bufsize) {
                        fmterr |= 2; /* overflow */
                    } else {
                        day = ymd_to_daynum(yy, mm, dd);
                        if (day < 0 || hh >= 12) day++; /* Julian day starts at 12:00 PM */
                        sprintf(tb, "%07d", day);
                        for (ii = 0; tb[ii]; ii++) {
                            buf[bx] = tb[ii];
                            bx++;
                        }
                    }
                    break;

                case 'm':
                    if (bx + 2 >= bufsize) {
                        fmterr |= 2; /* overflow */
                    } else {
                        buf[bx++] = (mm / 10) + '0';
                        buf[bx++] = (mm % 10) + '0';
                    }
                    break;

                case 't':
                    ix++;
                    switch (fmt[ix]) {
                        case 'e':
                            if (bx + 6 >= bufsize) {
                                fmterr |= 2; /* overflow */
                            } else {
                                tsec = (hh * 3600) + (mi * 60) + ss;
                                t_x = (double) tsec / 86400.0;
                                for (ii = 0; ii < 6; ii++) {
                                    t_x *= 10.0;
                                    fl = floor(t_x);
                                    buf[bx++] = ((int)fl) + '0';
                                    t_x -= fl;
                                }
                            }
                            break;
                        case 'j':
                            if (bx + 6 >= bufsize) {
                                fmterr |= 2; /* overflow */
                            } else {
                                tsec = (hh * 3600) + (mi * 60) + ss;
                                if (tsec < 43200) tsec += 43200; /* 43200 = 12 hours */
                                else              tsec -= 43200;
                                t_x = (double) tsec / 86400.0;
                                for (ii = 0; ii < 6; ii++) {
                                    t_x *= 10.0;
                                    fl = floor(t_x);
                                    buf[bx++] = ((int)fl) + '0';
                                    t_x -= fl;
                                }
                            }
                            break;
                        default: ;
                            fmterr |= 1; /* bad % */
                            if (bx + 1 >= bufsize) {
                                fmterr |= 2; /* overflow */
                            } else {
                                buf[bx++] = fmt[ix];
                            }
                            break;
                    }
                    break;

                case 'y':
                    if (bx + 2 >= bufsize) {
                        fmterr |= 2; /* overflow */
                    } else {
                        ii = abs(yy) % 100;
                        buf[bx++] = (ii / 10) + '0';
                        buf[bx++] = (ii % 10) + '0';
                    }
                    break;

                case 'Y':
                    if (bx + 4 >= bufsize) {
                        fmterr |= 2; /* overflow */
                    } else {
                        ii = abs(yy);
                        buf[bx++] = (ii / 1000    ) + '0';
                        buf[bx++] = (ii / 100 % 10) + '0';
                        buf[bx++] = (ii / 10  % 10) + '0';
                        buf[bx++] = (ii       % 10) + '0';
                    }
                    break;

                case 'H':
                    if (bx + 2 >= bufsize) {
                        fmterr |= 2; /* overflow */
                    } else {
                        buf[bx++] = (hh / 10) + '0';
                        buf[bx++] = (hh % 10) + '0';
                    }
                    break;

                case 'I':
                    if (bx + 2 >= bufsize) {
                        fmterr |= 2; /* overflow */
                    } else {
                        if (hh == 0) {
                            buf[bx++] = '1';
                            buf[bx++] = '2';
                        } else if (hh <= 12) {
                            buf[bx++] = (hh / 10) + '0';
                            buf[bx++] = (hh % 10) + '0';
                        } else {
                            buf[bx++] = ((hh - 12) / 10) + '0';
                            buf[bx++] = ((hh - 12) % 10) + '0';
                        }
                    }
                    break;

                case 'M':
                    if (bx + 2 >= bufsize) {
                        fmterr |= 2; /* overflow */
                    } else {
                        buf[bx++] = (mi / 10) + '0';
                        buf[bx++] = (mi % 10) + '0';
                    }
                    break;

                case 'p':
                    if (bx + 2 >= bufsize) {
                        fmterr |= 2; /* overflow */
                    } else {
                        if (hh <= 11) {
                            buf[bx++] = 'A';
                            buf[bx++] = 'M';
                        } else {
                            buf[bx++] = 'P';
                            buf[bx++] = 'M';
                        }
                    }
                    break;

                case 'S':
                    if (bx + 2 >= bufsize) {
                        fmterr |= 2; /* overflow */
                    } else {
                        buf[bx++] = (ss / 10) + '0';
                        buf[bx++] = (ss % 10) + '0';
                    }
                    break;

                default: ;
                    fmterr |= 1; /* bad % */
                    if (bx + 1 >= bufsize) {
                        fmterr |= 2; /* overflow */
                    } else {
                        buf[bx++] = fmt[ix];
                    }
                    break;
            }
            ix++;
        } else {
            if (bx + 1 >= bufsize) {
                fmterr |= 2; /* overflow */
            } else {
                buf[bx++] = fmt[ix++];
            }
        }
    }
    if (bx >= bufsize) buf[bufsize - 1] = '\0';
    else               buf[bx] = '\0';

    return (fmterr);
}
/***************************************************************/
int fmt_d_interval(struct d_interval * din, const char * fmt, char *buf, int bufsize) {
/*
** Similar to strftime(), fmt_d_date()
**
** See also: parsedate(), parse_interval()
**
** Supported formats:
**
**    %%  Percent sign
**    %H  Hour in 24-hour format (00 - 23)
**    %m  Months in interval (0 - 11)
**    %nd Total number of days in interval, preceded by '-' if negative
**    %nH Total number of hours in interval, preceded by '-' if negative
**    %nm Total number of months in interval, preceded by '-' if negative
**    %nM Total number of minutes in interval, preceded by '-' if negative
**    %nS Total number of seconds in interval, preceded by '-' if negative
**    %ny 2-digit Total number of years in interval, preceded by '-' if negative
**    %nY 4-digit Total number of years in interval, preceded by '-' if negative
**    %H  Hour in 24-hour format (00 - 23)
**    %S  Second as decimal number (00 - 59)
**    %y  Last 2 digits of year, preceded by '-' if negative
**    %Y  Last 4 digits of year, preceded by '-' if negative
*/
    int ii;
    int ix;
    int bx;
    int hh;
    int mi;
    int ss;
    int years;
    int months;
    int64 tmi;
    int64 thh;
    int64 td;
    int fmterr;
    char tb[32];

    ii64divmo(&tmi, &ss, din->in_seconds, 60);
    ii64divmo(&thh, &mi, tmi, 60);
    ii64divmo(&td , &hh, thh, 24);

    years = din->in_years;
    if (years == CAL_YEAR_NULL) years = 0;

    months = din->in_months;

    fmterr = 0;
    ix = 0;
    bx = 0;
    while (fmt[ix]) {
        if (fmt[ix] == '%' && fmt[ix+1]) {
            ix++;
            switch (fmt[ix]) {
                case '%':
                    if (bx + 1 >= bufsize) {
                        fmterr |= 2; /* overflow */
                    } else {
                        buf[bx++] = '%';
                    }
                    break;

                case 'H':
                    if (bx + 2 >= bufsize) {
                        fmterr |= 2; /* overflow */
                    } else {
                        buf[bx++] = (hh / 10) + '0';
                        buf[bx++] = (hh % 10) + '0';
                    }
                    break;

                case 'm':
                    if (bx + 2 >= bufsize) {
                        fmterr |= 2; /* overflow */
                    } else {
                        ii = din->in_months;
                        if (ii < 0) ii += 12;
                        buf[bx++] = (ii / 10) + '0';
                        buf[bx++] = (ii % 10) + '0';
                    }
                    break;

                case 'M':
                    if (bx + 2 >= bufsize) {
                        fmterr |= 2; /* overflow */
                    } else {
                        buf[bx++] = (mi / 10) + '0';
                        buf[bx++] = (mi % 10) + '0';
                    }
                    break;

                case 'n':
                    ix++;
                    switch (fmt[ix]) {
                        case 'd':
                            Snprintf(tb, sizeof(tb), "%6Ld", td);
                            break;
                        case 'H':
                            Snprintf(tb, sizeof(tb), "%6Ld", thh);
                            break;
                        case 'm':
                            Snprintf(tb, sizeof(tb), "%4d", years * months);
                            break;
                        case 'M':
                            Snprintf(tb, sizeof(tb), "%6Ld", tmi);
                            break;
                        case 'S':
                            Snprintf(tb, sizeof(tb), "%6Ld", din->in_seconds);
                            break;
                        case 'y':
                            Snprintf(tb, sizeof(tb), "%2d", years);
                            break;
                        case 'Y':
                            Snprintf(tb, sizeof(tb), "%4d", years);
                            break;
                        default: ;
                            fmterr |= 1; /* bad % */
                            if (bx + 1 >= bufsize) {
                                fmterr |= 2; /* overflow */
                            } else {
                                buf[bx++] = fmt[ix];
                            }
                            break;
                        }
                        for (ii = 0; !fmterr && tb[ii]; ii++) {
                            if (bx + 1 >= bufsize) {
                                fmterr |= 2; /* overflow */
                            } else {
                                buf[bx++] = tb[ii];
                        }
                    }
                    break;

                case 'S':
                    if (bx + 2 >= bufsize) {
                        fmterr |= 2; /* overflow */
                    } else {
                        buf[bx++] = (ss / 10) + '0';
                        buf[bx++] = (ss % 10) + '0';
                    }
                    break;

                case 'y':
                    ii = years;
                    if (din->in_months < 0) ii -= 1;
                    if (bx + 2 + ((ii<0)?1:0) >= bufsize) {
                        fmterr |= 2; /* overflow */
                    } else {
                        if (ii < 0) {
                            buf[bx++] = '-';
                            ii = -ii;
                        }
                        buf[bx++] = (ii / 10) + '0';
                        buf[bx++] = (ii % 10) + '0';
                    }
                    break;

                case 'Y':
                    ii = years;
                    if (din->in_months < 0) ii -= 1;
                    if (bx + 4 + ((ii<0)?1:0) >= bufsize) {
                        fmterr |= 2; /* overflow */
                    } else {
                        if (ii < 0) {
                            buf[bx++] = '-';
                            ii = -ii;
                        }
                        buf[bx++] = (ii / 1000    ) + '0';
                        buf[bx++] = (ii / 100 % 10) + '0';
                        buf[bx++] = (ii / 10  % 10) + '0';
                        buf[bx++] = (ii       % 10) + '0';
                    }
                    break;

                default: ;
                    fmterr |= 1; /* bad % */
                    if (bx + 1 >= bufsize) {
                        fmterr |= 2; /* overflow */
                    } else {
                        buf[bx++] = fmt[ix];
                    }
                    break;
            }
            ix++;
        } else {
            if (bx + 1 >= bufsize) {
                fmterr |= 2; /* overflow */
            } else {
                buf[bx++] = fmt[ix++];
            }
        }
    }
    if (bx >= bufsize) buf[bufsize - 1] = '\0';
    else               buf[bx] = '\0';

    return (fmterr);
}
/***************************************************************/
int parse_interval(const char* dbuf,
                int *ix,
                const char * fmt,
                struct d_interval * din) {
/*
** Similar to parsedate()
**
** See also: fmt_d_interval(), fmt_d_date()
**
** Supported formats:
**    %%  Percent sign
**    %H  Hour in 24-hour format (00 - 23)
**    %m  Months in interval (0 - 11)
**    %nd Total number of days in interval, preceded by '-' if negative
**    %nH Total number of hours in interval, preceded by '-' if negative
**    %nm Total number of months in interval, preceded by '-' if negative
**    %nM Total number of minutes in interval, preceded by '-' if negative
**    %nS Total number of seconds in interval, preceded by '-' if negative
**    %ny 2-digit Total number of years in interval, preceded by '-' if negative
**    %nY 4-digit Total number of years in interval, preceded by '-' if negative
**    %H  Hour in 24-hour format (00 - 23)
**    %S  Second as decimal number (00 - 59)
**    %y  Last 2 digits of year, preceded by '-' if negative
**    %Y  Last 4 digits of year, preceded by '-' if negative
*/
    int bx;
    int fx;
    int errn;
    int n_yy;
    int n_mm;
    int n_d;
    int n_H;
    int n_M;
    int n_S;
    int yy;
    int mm;
    int hh;
    int mi;
    int ss;
    int neg;
    char fdone;
    char fdel;
    int tol;    /* tolerant - %H %d. ok with 1 digit */
    char buf[64];

    errn    = 0;

    din->in_months  = 0;
    din->in_years   = CAL_YEAR_NULL;
    din->in_seconds = 0;

    fx      = 0;
    n_d     = 0;
    n_H     = 0;
    n_M     = 0;
    n_S     = 0;
    n_mm    = 0;
    n_yy    = 0;
    mm      = 0;
    yy      = CAL_YEAR_NULL;
    hh      = 0;
    mi      = 0;
    ss      = 0;
    tol     = 1;

    while (!errn && fmt[fx] && dbuf[*ix]) {
        if (fmt[fx] == '%' && fmt[fx+1]) {
            fx++;
            switch (fmt[fx]) {
                case '%':
                    buf[bx++] = '%';
                    break;

                case 'H':
                    if (dbuf[*ix] == ' ') {
                        (*ix)++;
                        hh = 0;
                    } else if (dbuf[*ix] >= '0' && dbuf[*ix] <= '9') {
                        hh = dbuf[*ix] - '0';
                        (*ix)++;
                    } else errn = EPD_DHOUR; /* bad hour */
                    if (!errn && dbuf[*ix] != fmt[fx + 1]) {
                        if (dbuf[*ix] >= '0' && dbuf[*ix] <= '9') {
                            hh = (hh * 10) + dbuf[*ix] - '0';
                            (*ix)++;
                        } else if (!tol) errn = EPD_DHOUR; /* bad hour */
                    }
                    break;

                case 'm':
                    if (dbuf[*ix] == ' ') {
                        (*ix)++;
                    } else if (dbuf[*ix] >= '0' && dbuf[*ix] <= '9') {
                        mm = dbuf[*ix] - '0';
                        (*ix)++;
                    } else errn = EPD_DMONTH; /* bad month */
                    if (!errn && dbuf[*ix] != fmt[fx + 1]) {
                        if (dbuf[*ix] >= '0' && dbuf[*ix] <= '9') {
                            mm = (mm * 10) + dbuf[*ix] - '0';
                            (*ix)++;
                        } else errn = EPD_DMONTH; /* bad month */
                    }
                    break;

                case 'M':
                    if (dbuf[*ix] == ' ') {
                        (*ix)++;
                        mi = 0;
                    } else if (dbuf[*ix] >= '0' && dbuf[*ix] <= '9') {
                        mi = dbuf[*ix] - '0';
                        (*ix)++;
                    } else errn = EPD_DMINUTE; /* bad minute */
                    if (!errn && dbuf[*ix] != fmt[fx + 1]) {
                        if (dbuf[*ix] >= '0' && dbuf[*ix] <= '9') {
                            mi = (mi * 10) + dbuf[*ix] - '0';
                            (*ix)++;
                        } else errn = EPD_DMINUTE; /* bad minute */
                    }
                    break;

                case 'n':
                    fx++;
                    fdel = fmt[fx + 1];
                    if (fdel == '%' || isdigit(fdel)) errn = EPD_DNDAYS;
                    fdone = 0;
                    switch (fmt[fx]) {
                        case 'd':
                            bx = 0;
                            neg = 0;
                            n_d = 0;
                            while (dbuf[*ix] == ' ') (*ix)++;
                            if (dbuf[*ix] == '-') { neg = 1; (*ix)++; }
                            while (!errn && dbuf[*ix] && !fdone) {
                                if (dbuf[*ix] >= '0' && dbuf[*ix] <= '9') {
                                    n_d = (n_d * 10) + dbuf[*ix] - '0';
                                    (*ix)++;
                                    bx++;
                                } else if (dbuf[*ix] == fdel) {
                                    fdone = 1;
                                } else {
                                    errn = EPD_DNDAYS; /* bad num days */
                                }
                            }
                            if (!errn && neg) n_d = -n_d;
                            break;

                        case 'H':
                            bx = 0;
                            neg = 0;
                            n_H = 0;
                            while (dbuf[*ix] == ' ') (*ix)++;
                            if (dbuf[*ix] == '-') { neg = 1; (*ix)++; }
                            while (!errn && dbuf[*ix] && !fdone) {
                                if (dbuf[*ix] >= '0' && dbuf[*ix] <= '9') {
                                    n_H = (n_H * 10) + dbuf[*ix] - '0';
                                    (*ix)++;
                                    bx++;
                                } else if (dbuf[*ix] == fdel) {
                                    fdone = 1;
                                } else {
                                    errn = EPD_DNHOURS; /* bad num hours */
                                }
                            }
                            if (!errn && neg) n_H = -n_H;
                            break;

                        case 'm':
                            bx = 0;
                            neg = 0;
                            n_mm = 0;
                            while (dbuf[*ix] == ' ') (*ix)++;
                            if (dbuf[*ix] == '-') { neg = 1; (*ix)++; }
                            while (!errn && dbuf[*ix] && !fdone) {
                                if (dbuf[*ix] >= '0' && dbuf[*ix] <= '9') {
                                    n_mm = (n_mm * 10) + dbuf[*ix] - '0';
                                    (*ix)++;
                                    bx++;
                                } else if (dbuf[*ix] == fdel) {
                                    fdone = 1;
                                } else {
                                    errn = EPD_DNMONTHS; /* bad num months */
                                }
                            }
                            if (!errn && neg) n_mm = -n_mm;
                            break;

                        case 'M':
                            bx = 0;
                            neg = 0;
                            n_M = 0;
                            while (dbuf[*ix] == ' ') (*ix)++;
                            if (dbuf[*ix] == '-') { neg = 1; (*ix)++; }
                            while (!errn && dbuf[*ix] && !fdone) {
                                if (dbuf[*ix] >= '0' && dbuf[*ix] <= '9') {
                                    n_M = (n_M * 10) + dbuf[*ix] - '0';
                                    (*ix)++;
                                    bx++;
                                } else if (dbuf[*ix] == fdel) {
                                    fdone = 1;
                                } else {
                                    errn = EPD_DNMINUTES; /* bad num minutes */
                                }
                            }
                            if (!errn && neg) n_M = -n_M;
                            break;

                        case 'S':
                            bx = 0;
                            neg = 0;
                            n_S = 0;
                            while (dbuf[*ix] == ' ') (*ix)++;
                            if (dbuf[*ix] == '-') { neg = 1; (*ix)++; }
                            while (!errn && dbuf[*ix] && !fdone) {
                                if (dbuf[*ix] >= '0' && dbuf[*ix] <= '9') {
                                    n_S = (n_S * 10) + dbuf[*ix] - '0';
                                    (*ix)++;
                                    bx++;
                                } else if (dbuf[*ix] == fdel) {
                                    fdone = 1;
                                } else {
                                    errn = EPD_DNSECONDS; /* bad num seconds */
                                }
                            }
                            if (!errn && neg) n_S = -n_S;
                            break;

                        case 'y':
                        case 'Y':
                            bx = 0;
                            neg = 0;
                            n_yy = 0;
                            while (dbuf[*ix] == ' ') (*ix)++;
                            if (dbuf[*ix] == '-') { neg = 1; (*ix)++; }
                            while (!errn && dbuf[*ix] && !fdone) {
                                if (dbuf[*ix] >= '0' && dbuf[*ix] <= '9') {
                                    n_yy = (n_yy * 10) + dbuf[*ix] - '0';
                                    (*ix)++;
                                    bx++;
                                } else if (dbuf[*ix] == fdel) {
                                    fdone = 1;
                                } else {
                                    errn = EPD_DNYEARS; /* bad num years */
                                }
                            }
                            if (!errn && neg) n_yy = -n_yy;
                            break;
                    }
                    break;

                case 'S':
                    if (dbuf[*ix] == ' ') {
                        ss = 0;
                        (*ix)++;
                    } else if (dbuf[*ix] >= '0' && dbuf[*ix] <= '9') {
                        ss = dbuf[*ix] - '0';
                        (*ix)++;
                    } else errn = EPD_DSECOND; /* bad second */
                    if (!errn && dbuf[*ix] != fmt[fx + 1]) {
                        if (dbuf[*ix] >= '0' && dbuf[*ix] <= '9') {
                            ss = (ss * 10) + dbuf[*ix] - '0';
                            (*ix)++;
                        } else errn = EPD_DSECOND; /* bad second */
                    }
                    break;

                case 'y':
                    if (dbuf[*ix] == ' ') {
                        yy = 0;
                        (*ix)++;
                    } else if (dbuf[*ix] >= '0' && dbuf[*ix] <= '9') {
                        yy = dbuf[*ix] - '0';
                        (*ix)++;
                    } else errn = EPD_DYY; /* bad 2 digit year */
                    if (!errn) {
                        if (dbuf[*ix] >= '0' && dbuf[*ix] <= '9') {
                            yy = (yy * 10) + dbuf[*ix] - '0';
                            (*ix)++;
                        } else errn = EPD_DYY; /* bad 2 digit year */
                    }
                    if (!errn) yy += 2000;
                    break;

                case 'Y':
                    bx = 0;
                    yy = 0;
                    while (!errn && dbuf[*ix] && bx < 4) {
                        if (dbuf[*ix] >= '0' && dbuf[*ix] <= '9') {
                            yy = (yy * 10) + dbuf[*ix] - '0';
                            (*ix)++;
                            bx++;
                        } else {
                            errn = EPD_DYYYY; /* bad 4 digit year */
                        }
                    }
                    break;

                default:
                    errn = EPD_DBADCHAR;  /* bad % character */
                    break;
            }
            fx++;
        } else {
            if (fmt[fx] == dbuf[*ix]) {
                fx++;
                (*ix)++;
            } else {
                errn = EPD_DMATCH; /* character doesn't match */
            }
        }
    }

    if (!errn && fmt[fx]) {
        errn = EPD_DEOD; /* end of dbuf */
    }

/****************************************************************
 ****************************************************************
 ********              CALCULATIONS HERE                 ********
 ****************************************************************
 ****************************************************************/

    /* Handle year and month */
    if (n_yy) {
        if (n_mm || yy != CAL_YEAR_NULL) {
            errn = EPD_DINTCONFLICT;
        } else {
            din->in_years = n_yy;
        }
    } else if (n_mm) {
        if (mm || yy != CAL_YEAR_NULL) {
            errn = EPD_DINTCONFLICT;
        } else {
            idivmo(&(din->in_years), &(din->in_months), n_mm, 12);
        }
    }

    if (!errn) {
        if (mm) {
            if (mm > 11) {
                errn = EPD_DINTCONFLICT;
            } else {
                din->in_months += mm;
            }
        }
    }

    /* Handle days, hours, minutes, seconds */
    if (n_d) {
        if (n_H || n_M || n_S) {
            errn = EPD_DINTCONFLICT;
        } else {
            din->in_seconds = n_d * SECS_IN_DAY;
        }
    } else if (n_H) {
        if (n_M || n_S) {
            errn = EPD_DINTCONFLICT;
        } else {
            din->in_seconds = n_H * 3600L;
        }
    } else if (n_M) {
        if (n_S) {
            errn = EPD_DINTCONFLICT;
        } else {
            din->in_seconds = n_M * 60L;
        }
    } else if (n_S) {
        din->in_seconds = n_S;
    }

    if (!errn) {
        if (hh) {
            if (hh > 23) {
                errn = EPD_DHOUR;
            } else {
                din->in_seconds += hh * 3600L;
            }
        }
    }

    if (!errn) {
        if (mi) {
            if (mi > 59) {
                errn = EPD_DMINUTE;
            } else {
                din->in_seconds += mi * 60L;
            }
        }
    }

    if (!errn) {
        if (ss) {
            if (ss > 59) {
                errn = EPD_DSECOND;
            } else {
                din->in_seconds += ss;
            }
        }
    }

    return (errn);
}
/***************************************************************/
void get_pd_errmsg(int pderr, char * errmsg, int errmsgmax) {

    extern void strmcpy(char * tgt, const char * src, size_t tmax);

    switch (pderr) {
        case EPD_DBADCHAR   :
            strmcpy(errmsg, "Unsupported date % directive", errmsgmax);
            break;

        case EPD_DMATCH   :
            strmcpy(errmsg, "Single character mismatch in date", errmsgmax);
            break;

        case EPD_DMONNAM   :
            strmcpy(errmsg, "Invalid 3 letter abreviation of month in date", errmsgmax);
            break;

        case EPD_DDAY   :
            strmcpy(errmsg, "Invalid day within month (1-31) in date", errmsgmax);
            break;

        case EPD_DMONTH   :
            strmcpy(errmsg, "Invalid month within year (1-12) in date", errmsgmax);
            break;

        case EPD_DYY   :
            strmcpy(errmsg, "Invalid 2 digit year (00-99) in date", errmsgmax);
            break;

        case EPD_DYYYY   :
            strmcpy(errmsg, "Invalid 4 digit year (0000-9999) in date", errmsgmax);
            break;

        case EPD_DDATE   :
            strmcpy(errmsg, "Invalid date", errmsgmax);
            break;

        case EPD_DHOUR   :
            strmcpy(errmsg, "Invalid hour within day (0-23) in date", errmsgmax);
            break;

        case EPD_DMINUTE   :
            strmcpy(errmsg, "Invalid minute within hour (0-59) in date", errmsgmax);
            break;

        case EPD_DSECOND   :
            strmcpy(errmsg, "Invalid second within minute (0-59) in date", errmsgmax);
            break;

        case EPD_DEOD   :
            strmcpy(errmsg, "Unexpected end of date value", errmsgmax);
            break;

        case EPD_DWDNAME   :
            strmcpy(errmsg, "Invalid 3 letter abreviation of day of week in date", errmsgmax);
            break;

        case EPD_EXPEOD   :
            strmcpy(errmsg, "Expected end of date value", errmsgmax);
            break;

        case EPD_BADUNIT   :
            strmcpy(errmsg, "Invalid unit for date arithmetic", errmsgmax);
            break;

        case EPD_BADRESULT   :
            strmcpy(errmsg, "Invalid result in date arithmetic", errmsgmax);
            break;

        case EPD_DAMPM   :
            strmcpy(errmsg, "Invalid 2 letter AM/PM", errmsgmax);
            break;

        case EPD_DHOUR12   :
            strmcpy(errmsg, "Invalid AM/PM hour within day (1-12) in date", errmsgmax);
            break;

        case EPD_DWDCONFLICT   :
            strmcpy(errmsg, "Day of week conflicts with date", errmsgmax);
            break;

        case EPD_DDAYOFYEAR:
            strmcpy(errmsg, "Invalid 3 digit day of year (001-366) in date", errmsgmax);
            break;

        case EPD_DJULDATE:
            strmcpy(errmsg, "Invalid 7 digit Julian day number in date", errmsgmax);
            break;

        case EPD_DJULCONFLICT:
            strmcpy(errmsg, "Julian day number conflicts with date", errmsgmax);
            break;

        case EPD_DJDOYCONFLICT:
            strmcpy(errmsg, "Day of year conflicts with date", errmsgmax);
            break;

        case EPD_DJULDOY:
            strmcpy(errmsg, "Invalid 3 digit day of year in date", errmsgmax);
            break;

        case EPD_DBCAD   :
            strmcpy(errmsg, "Invalid 2 letter BC/AD", errmsgmax);
            break;

        case EPD_DEXDATE:
            strmcpy(errmsg, "Invalid 5 digit Microsoft Excel day number in date", errmsgmax);
            break;

        case EPD_DEXCONFLICT:
            strmcpy(errmsg, "Microsoft Excel day number conflicts with date", errmsgmax);
            break;

        case EPD_DJULFRAC:
            strmcpy(errmsg, "Invalid 6 digit Julian time fraction", errmsgmax);
            break;

        case EPD_DJULFCONFLICT:
            strmcpy(errmsg, "Julian time fraction conflicts with time", errmsgmax);
            break;

        case EPD_DEXFRAC:
            strmcpy(errmsg, "Invalid 6 digit Microsoft Excel time fraction", errmsgmax);
            break;

        case EPD_DEXFCONFLICT:
            strmcpy(errmsg, "Microsoft Excel time fraction conflicts with time", errmsgmax);
            break;

        case EPD_DNDAYS:
            strmcpy(errmsg, "Invalid number of days", errmsgmax);
            break;

        case EPD_DNHOURS:
            strmcpy(errmsg, "Invalid number of hours", errmsgmax);
            break;

        case EPD_DNMINUTES:
            strmcpy(errmsg, "Invalid number of minutes", errmsgmax);
            break;

        case EPD_DNSECONDS:
            strmcpy(errmsg, "Invalid number of seconds", errmsgmax);
            break;

        case EPD_DNMONTHS  :
            strmcpy(errmsg, "Invalid number of months", errmsgmax);
            break;

        case EPD_DNYEARS  :
            strmcpy(errmsg, "Invalid number of years", errmsgmax);
            break;

        case EPD_DINTCONFLICT   :
            strmcpy(errmsg, "Items specified in interval are conflicting", errmsgmax);
            break;

        case EPD_DINTMONTH   :
            strmcpy(errmsg, "Invalid number of months within (0-11) in interval", errmsgmax);
            break;

        default:
            Snprintf(errmsg, errmsgmax, "Unrecognized date error number %d", pderr);
            break;
    }
}
/***************************************************************/
int parsedate(const char* dbuf,
                int *ix,
                const char * fmt,
                d_date * date) {
/*
** Similar to parse_interval()
**
** See also: fmt_d_date(), fmt_d_interval()
**
** Supported formats:
    %%  Percent sign
    %a  Abbreviated weekday name
    %b  Abbreviated month name
    %C  BC/AD indicator for year                            -- not in strftime()
    %d  Day of month as decimal number (01 - 31)
*    %E Day number from Jan 1, 1900 as in Microsoft Excel**   -- not in strftime()
    %H  Hour in 24-hour format (00 - 23)
    %I  Hour in 12-hour format (01 - 12)
    %j  Day of year as decimal number (001 - 366)
    %J  Julian day beginning January 1, 4713 BC              -- not in strftime()
    %m  Month as decimal number (01 - 12)
    %M  Minute as decimal number (00 - 59)
*    %nd Total number of days in interval                    -- not in strftime()
*    %nH Total number of hours in interval                   -- not in strftime()
*    %nM Total number of minutes in interval                 -- not in strftime()
*    %nS Total number of seconds in interval                 -- not in strftime()
    %S  Second as decimal number (00 - 59)
    %te Time of day from 12:00 AM as decimal (Excel format) -- not in strftime()
    %tj Time of day from 12:00 PM as decimal (Julian time)  -- not in strftime()
    %y  Year without century, as decimal number (00 - 99)
    %Y  Year with century, as decimal number
    %p  Current locales AM/PM indicator for 12-hour clock

** Unsupported formats
    %A  Full weekday name
    %B  Full month name
    %c  Date and time representation appropriate for locale
    %U  Week of year as decimal number, with Sunday as first day of week (00 - 53)
    %w  Weekday as decimal number (0 - 6; Sunday is 0)
    %W  Week of year as decimal number, with Monday as first day of week (00 - 53)
    %x  Date representation for current locale
    %X  Time representation for current locale
    %z, %Z  Time-zone name; no characters if time zone is unknown

* - Unimplemented
** - Microsoft Excel thinks 2/29/1900 is a date

* Errors
     1  Bad character in date
     2  Date doesn't match format
     3  Bad month name
     4  Bad day
     5  Bad month
     6  Bad 2 digit year
     7  Bad 4 digit year
     8  Not a valid date
     9  Bad hour
    10  Bad minute
    11  Bad second
    12  Unexpected end

*/
    int bx;
    int fx;
    int errn;
    int yy;
    int mm;
    int dd;
    int tyy;
    int tmm;
    int tdd;
    int hh;
    int mi;
    int ss;
    int thh;
    int tmi;
    int tss;
    int dow;
    int julian;
    int exdate;
    int t_e;
    int t_j;
    double fl;
    int jdoy;
    int hh12;   /* 0 -  No %I, 1=AM, 2=PM */
    int bcad;   /* 0 -  No %C, 1=BC, 2=AD */
    int tol;    /* tolerant - %H %d. ok with 1 digit */
    char buf[64];

    errn    = 0;
    *date   = 0;
    fx      = 0;
    yy      = CAL_YEAR_NULL;
    mm      = 0;
    dd      = 0;
    hh      = -1;
    mi      = -1;
    ss      = -1;
    dow     = -1;
    jdoy    = -1;
    julian  = -1;
    exdate  = -1;
    t_e     = -1;
    t_j     = -1;
    tol     = 1;
    hh12    = 0;
    bcad    = 0;

    while (!errn && fmt[fx] && dbuf[*ix]) {
        if (fmt[fx] == '%' && fmt[fx+1]) {
            fx++;
            switch (fmt[fx]) {
                case '%':
                    buf[bx++] = '%';
                    break;

                case 'A':
                    for (bx = 0; bx < 32 && isalpha(dbuf[*ix]); bx++) {
                        buf[bx] = toupper(dbuf[*ix]);
                        (*ix)++;
                    }
                    buf[bx] = '\0';
                         if (!strcmp(buf, "SUNDAY"))    dow = DOW_SUNDAY;
                    else if (!strcmp(buf, "MONDAY"))    dow = DOW_MONDAY;
                    else if (!strcmp(buf, "TUESDAY"))   dow = DOW_TUESDAY;
                    else if (!strcmp(buf, "WEDNESDAY")) dow = DOW_WEDNESDAY;
                    else if (!strcmp(buf, "THURSDAY"))  dow = DOW_THURSDAY;
                    else if (!strcmp(buf, "FRIDAY"))    dow = DOW_FRIDAY;
                    else if (!strcmp(buf, "SATURDAY"))  dow = DOW_SATURDAY;
                    else errn = EPD_DWDNAME;  /* bad weekday name */
                    break;

                case 'a':
                    for (bx = 0; bx < 3 && dbuf[*ix]; bx++) {
                        buf[bx] = toupper(dbuf[*ix]);
                        (*ix)++;
                    }
                    buf[bx] = '\0';
                         if (!strcmp(buf, "SUN")) dow = DOW_SUNDAY;
                    else if (!strcmp(buf, "MON")) dow = DOW_MONDAY;
                    else if (!strcmp(buf, "TUE")) dow = DOW_TUESDAY;
                    else if (!strcmp(buf, "WED")) dow = DOW_WEDNESDAY;
                    else if (!strcmp(buf, "THU")) dow = DOW_THURSDAY;
                    else if (!strcmp(buf, "FRI")) dow = DOW_FRIDAY;
                    else if (!strcmp(buf, "SAT")) dow = DOW_SATURDAY;
                    else errn = EPD_DWDNAME;  /* bad weekday name */
                    break;

                case 'B':
                    for (bx = 0; bx < 32 && isalpha(dbuf[*ix]); bx++) {
                        buf[bx] = toupper(dbuf[*ix]);
                        (*ix)++;
                    }
                    buf[bx] = '\0';
                         if (!strcmp(buf, "JANUARY"))   mm =  1;
                    else if (!strcmp(buf, "FEBRUARY"))  mm =  2;
                    else if (!strcmp(buf, "MARCG"))     mm =  3;
                    else if (!strcmp(buf, "APRIL"))     mm =  4;
                    else if (!strcmp(buf, "MAY"))       mm =  5;
                    else if (!strcmp(buf, "JUNE"))      mm =  6;
                    else if (!strcmp(buf, "JULY"))      mm =  7;
                    else if (!strcmp(buf, "AUGUST"))    mm =  8;
                    else if (!strcmp(buf, "SEPTEMBER")) mm =  9;
                    else if (!strcmp(buf, "OCTOBER"))   mm = 10;
                    else if (!strcmp(buf, "NOVEMBER"))  mm = 11;
                    else if (!strcmp(buf, "DECEMBER"))  mm = 12;
                    else errn = EPD_DMONNAM;  /* bad month name */
                    break;

                case 'b':
                    for (bx = 0; bx < 3 && dbuf[*ix]; bx++) {
                        buf[bx] = toupper(dbuf[*ix]);
                        (*ix)++;
                    }
                    buf[bx] = '\0';
                         if (!strcmp(buf, "JAN")) mm =  1;
                    else if (!strcmp(buf, "FEB")) mm =  2;
                    else if (!strcmp(buf, "MAR")) mm =  3;
                    else if (!strcmp(buf, "APR")) mm =  4;
                    else if (!strcmp(buf, "MAY")) mm =  5;
                    else if (!strcmp(buf, "JUN")) mm =  6;
                    else if (!strcmp(buf, "JUL")) mm =  7;
                    else if (!strcmp(buf, "AUG")) mm =  8;
                    else if (!strcmp(buf, "SEP")) mm =  9;
                    else if (!strcmp(buf, "OCT")) mm = 10;
                    else if (!strcmp(buf, "NOV")) mm = 11;
                    else if (!strcmp(buf, "DEC")) mm = 12;
                    else errn = EPD_DMONNAM;  /* bad month name */
                    break;

                case 'C':
                    for (bx = 0; bx < 2 && dbuf[*ix]; bx++) {
                        buf[bx] = toupper(dbuf[*ix]);
                        (*ix)++;
                    }
                    buf[bx] = '\0';

                         if (!strcmp(buf, "BC")) bcad =  1;
                    else if (!strcmp(buf, "AD")) bcad =  2;
                    else errn = EPD_DBCAD;  /* bad BC/AC */
                    break;

                case 'd':
                    if (dbuf[*ix] == ' ') {
                        (*ix)++;
                    } else if (dbuf[*ix] >= '0' && dbuf[*ix] <= '9') {
                        dd = dbuf[*ix] - '0';
                        (*ix)++;
                    } else errn = EPD_DDAY; /* bad day */
                    if (!errn) {
                        if (dbuf[*ix] >= '0' && dbuf[*ix] <= '9') {
                            dd = (dd * 10) + dbuf[*ix] - '0';
                            (*ix)++;
                        } else if (!tol) errn = EPD_DDAY; /* bad day */
                    }
                    break;

                case 'E':
                    bx = 0;
                    exdate = 0;
                    while (dbuf[*ix] == ' ' && bx < 5) bx++;
                    while (!errn && dbuf[*ix] && bx < 5) {
                        if (dbuf[*ix] >= '0' && dbuf[*ix] <= '9') {
                            exdate = (exdate * 10) + dbuf[*ix] - '0';
                            (*ix)++;
                            bx++;
                        } else {
                            errn = EPD_DEXDATE; /* bad Excel day */
                        }
                    }
                    break;

                case 'H':
                    if (dbuf[*ix] == ' ') {
                        (*ix)++;
                        hh = 0;
                    } else if (dbuf[*ix] >= '0' && dbuf[*ix] <= '9') {
                        hh = dbuf[*ix] - '0';
                        (*ix)++;
                    } else errn = EPD_DHOUR; /* bad hour */
                    if (!errn) {
                        if (dbuf[*ix] >= '0' && dbuf[*ix] <= '9') {
                            hh = (hh * 10) + dbuf[*ix] - '0';
                            (*ix)++;
                        } else if (!tol) errn = EPD_DHOUR; /* bad hour */
                    }
                    break;

                case 'I':
                    if (!hh12) hh12 = 1;
                    if (dbuf[*ix] == ' ') {
                        (*ix)++;
                        hh = 0;
                    } else if (dbuf[*ix] >= '0' && dbuf[*ix] <= '9') {
                        hh = dbuf[*ix] - '0';
                        (*ix)++;
                    } else errn = EPD_DHOUR; /* bad hour */
                    if (!errn) {
                        if (dbuf[*ix] >= '0' && dbuf[*ix] <= '9') {
                            hh = (hh * 10) + dbuf[*ix] - '0';
                            (*ix)++;
                        } else if (!tol) errn = EPD_DHOUR; /* bad hour */
                    }
                    break;

                case 'j':
                    bx = 0;
                    jdoy = 0;
                    while (dbuf[*ix] == ' ' && bx < 3) bx++;
                    while (!errn && dbuf[*ix] && bx < 3) {
                        if (dbuf[*ix] >= '0' && dbuf[*ix] <= '9') {
                            jdoy = (jdoy * 10) + dbuf[*ix] - '0';
                            (*ix)++;
                            bx++;
                        } else {
                            errn = EPD_DJULDOY; /* bad day of year */
                        }
                    }
                    break;

                case 'J':
                    bx = 0;
                    julian = 0;
                    while (dbuf[*ix] == ' ' && bx < 7) bx++;
                    while (!errn && dbuf[*ix] && bx < 7) {
                        if (dbuf[*ix] >= '0' && dbuf[*ix] <= '9') {
                            julian = (julian * 10) + dbuf[*ix] - '0';
                            (*ix)++;
                            bx++;
                        } else {
                            errn = EPD_DJULDATE; /* bad Julian day */
                        }
                    }
                    break;

                case 'p':
                    for (bx = 0; bx < 2 && dbuf[*ix]; bx++) {
                        buf[bx] = toupper(dbuf[*ix]);
                        (*ix)++;
                    }
                    buf[bx] = '\0';

                         if (!strcmp(buf, "AM")) hh12 =  1;
                    else if (!strcmp(buf, "PM")) hh12 =  2;
                    else errn = EPD_DAMPM;  /* bad AM/PM */
                    break;

                case 'm':
                    if (dbuf[*ix] == ' ') {
                        (*ix)++;
                    } else if (dbuf[*ix] >= '0' && dbuf[*ix] <= '9') {
                        mm = dbuf[*ix] - '0';
                        (*ix)++;
                    } else errn = EPD_DMONTH; /* bad month */
                    if (!errn) {
                        if (dbuf[*ix] >= '0' && dbuf[*ix] <= '9') {
                            mm = (mm * 10) + dbuf[*ix] - '0';
                            (*ix)++;
                        } else errn = EPD_DMONTH; /* bad month */
                    }
                    break;

                case 'M':
                    if (dbuf[*ix] == ' ') {
                        (*ix)++;
                        mi = 0;
                    } else if (dbuf[*ix] >= '0' && dbuf[*ix] <= '9') {
                        mi = dbuf[*ix] - '0';
                        (*ix)++;
                    } else errn = EPD_DMINUTE; /* bad minute */
                    if (!errn) {
                        if (dbuf[*ix] >= '0' && dbuf[*ix] <= '9') {
                            mi = (mi * 10) + dbuf[*ix] - '0';
                            (*ix)++;
                        } else errn = EPD_DMINUTE; /* bad minute */
                    }
                    break;

                case 'S':
                    if (dbuf[*ix] == ' ') {
                        ss = 0;
                        (*ix)++;
                    } else if (dbuf[*ix] >= '0' && dbuf[*ix] <= '9') {
                        ss = dbuf[*ix] - '0';
                        (*ix)++;
                    } else errn = EPD_DSECOND; /* bad second */
                    if (!errn) {
                        if (dbuf[*ix] >= '0' && dbuf[*ix] <= '9') {
                            ss = (ss * 10) + dbuf[*ix] - '0';
                            (*ix)++;
                        } else errn = EPD_DSECOND; /* bad second */
                    }
                    break;

                case 't':
                    fx++;
                    switch (fmt[fx]) {
                        case 'e':
                            bx = 0;
                            t_e = 0;
                            while (dbuf[*ix] == ' ' && bx < 6) bx++;
                            while (!errn && dbuf[*ix] && bx < 6) {
                                if (dbuf[*ix] >= '0' && dbuf[*ix] <= '9') {
                                    t_e = (t_e * 10) + dbuf[*ix] - '0';
                                    (*ix)++;
                                    bx++;
                                } else {
                                    errn = EPD_DEXFRAC; /* bad Excel time */
                                }
                            }
                            break;
                        case 'j':
                            bx = 0;
                            t_j = 0;
                            while (dbuf[*ix] == ' ' && bx < 6) bx++;
                            while (!errn && dbuf[*ix] && bx < 6) {
                                if (dbuf[*ix] >= '0' && dbuf[*ix] <= '9') {
                                    t_j = (t_j * 10) + dbuf[*ix] - '0';
                                    (*ix)++;
                                    bx++;
                                } else {
                                    errn = EPD_DEXFRAC; /* bad Excel time */
                                }
                            }
                            break;
                        default:
                            errn = EPD_DBADCHAR;  /* bad % character */
                            break;
                    }
                    break;

                case 'y':
                    if (dbuf[*ix] == ' ') {
                        yy = 0;
                        (*ix)++;
                    } else if (dbuf[*ix] >= '0' && dbuf[*ix] <= '9') {
                        yy = dbuf[*ix] - '0';
                        (*ix)++;
                    } else errn = EPD_DYY; /* bad 2 digit year */
                    if (!errn) {
                        if (dbuf[*ix] >= '0' && dbuf[*ix] <= '9') {
                            yy = (yy * 10) + dbuf[*ix] - '0';
                            (*ix)++;
                        } else errn = EPD_DYY; /* bad 2 digit year */
                    }
                    if (!errn) yy += 2000;
                    break;

                case 'Y':
                    bx = 0;
                    yy = 0;
                    while (!errn && dbuf[*ix] && bx < 4) {
                        if (dbuf[*ix] >= '0' && dbuf[*ix] <= '9') {
                            yy = (yy * 10) + dbuf[*ix] - '0';
                            (*ix)++;
                            bx++;
                        } else {
                            errn = EPD_DYYYY; /* bad 4 digit year */
                        }
                    }
                    break;

                default:
                    errn = EPD_DBADCHAR;  /* bad % character */
                    break;
            }
            fx++;
        } else {
            if (fmt[fx] == dbuf[*ix]) {
                fx++;
                (*ix)++;
            } else {
                errn = EPD_DMATCH; /* character doesn't match */
            }
        }
    }

    if (!errn && fmt[fx]) {
        errn = EPD_DEOD; /* end of dbuf */
    }

    if (!errn) {
        if (mm >= 0 || dd >= 0) {
            if (yy == CAL_YEAR_NULL) {
                yy = get_year();
            } else if (bcad == 1) {
                yy = -yy;
            }
            if (mm < 0) mm = 1;
            if (dd < 0) dd = 1;
            if (gooddate(yy, mm, dd)) {
                *date = make_d_date_ymd(yy, mm, dd);
                if (dow >= 0) {
                    if (ymd_to_dayofweek(yy, mm, dd) != dow) {
                        errn = EPD_DWDCONFLICT;
                    }
                }
            } else errn = EPD_DDATE; /* bad date */
        }
    }

    if (!errn && jdoy >= 0) {
        if (jdoy < 1 || jdoy > 366) {
            errn = EPD_DJULDOY;
        } else {
            if (yy == CAL_YEAR_NULL) {
                yy = get_year();
            }
            from_day_of_year(yy, jdoy, &tmm, &tdd);
            if (!gooddate(yy, tmm, tdd)) {
                errn = EPD_DWDCONFLICT;
            } else {
                if ((mm > 0 && mm != tmm) || (dd > 0 && dd != tdd)) {
                    errn = EPD_DJDOYCONFLICT;
                } else {
                    mm = tmm;
                    dd = tdd;
                }
            }
        }
    }

    if (!errn && hh12) {
        if (hh < 1 || hh > 12) {
            errn = EPD_DHOUR12;
        } else if (hh12 == 2) {
            if (hh != 12) hh += 12;
        } else if (hh == 12) {
            hh = 0;
        }
    }

    if (!errn) {
        if (hh >= 0 || mi >= 0 || ss >= 0) {
            if (hh < 0) hh = 0;
            else if (hh > 23) errn =  EPD_DHOUR; /* bad hour */
            if (!errn) {
                if (mi < 0) {
                    if (ss >= 0) *date += (hh * 3600) + ss;
                    else         *date += (hh * 3600);
                } else {
                    if (mi > 59) errn = EPD_DMINUTE; /* bad minute */
                    else {
                        if (ss < 0) {
                            *date += (hh * 3600) + (mi * 60);
                        } else {
                            if (ss > 59) errn = EPD_DSECOND; /* bad minute */
                            else {
                                *date += (hh * 3600) + (mi * 60) + ss;
                            }
                        }
                    }
                }
            }
        }
    }

    if (!errn && t_e >= 0) {
        fl = (double)t_e / 1000000.0;
        if (fl < 0.0 || fl > 1.0) {
            errn = EPD_DEXFRAC;
        } else {
            fl = floor((fl * 86400.0) + 0.5);
            sec_to_hms((int)fl, &thh, &tmi, &tss);
            if (hh < 0) hh = thh;
            if (mi < 0) mi = tmi;
            if (ss < 0) ss = tss;
            if (abs(hms_to_sec(hh, mi, ss) - (int)fl) > 1) {
                errn = EPD_DEXFCONFLICT;
            }
        }
    }

    if (!errn && t_j >= 0) {
        fl = (double)t_j / 1000000.0;
        if (fl < 0.0 || fl > 1.0) {
            errn = EPD_DJULFRAC;
        } else {
            if (fl < 0.5) fl += 0.5;
            else          fl -= 0.5;
            fl = floor((fl * 86400.0) + 0.5);
            sec_to_hms((int)fl, &thh, &tmi, &tss);
            if (hh < 0) hh = thh;
            if (mi < 0) mi = tmi;
            if (ss < 0) ss = tss;
            if (abs(hms_to_sec(hh, mi, ss) - (int)fl) > 1) {
                errn = EPD_DJULFCONFLICT;
            }
        }
    }

    if (!errn && julian >= 0) {
        if (hh >= 12) julian--;  /* Julian day starts at 12:00 PM */
        daynum_to_ymd(julian, &tyy, &tmm, &tdd);
        if (!gooddate(tyy, tmm, tdd)) {
            errn = EPD_DJULDATE;
        } else {
            if ((yy != CAL_YEAR_NULL && yy != tyy) ||
                (mm > 0 && mm != tmm) || (dd > 0 && dd != tdd)) {
                errn = EPD_DJULCONFLICT;
            } else {
                yy = tyy;
                mm = tmm;
                ss = tdd;
            }
        }
    }

    if (!errn && exdate >= 0) {
        daynum_to_ymd(exdate + JULIAN_EXCEL_DIFF, &tyy, &tmm, &tdd);
        if (!gooddate(tyy, tmm, tdd)) {
            errn = EPD_DJULDATE;
        } else {
            if ((yy != CAL_YEAR_NULL && yy != tyy) ||
                (mm > 0 && mm != tmm) || (dd > 0 && dd != tdd)) {
                errn = EPD_DEXCONFLICT;
            } else {
                yy = tyy;
                mm = tmm;
                ss = tdd;
            }
        }
    }

    return (errn);
}
/***************************************************************/
int add_sub_to_date(d_date              fdate,
                    struct d_interval * fdin,
                    int                 subbing,
                    const char *        outfmt,
                    char  *             outbuf,
                    int                 outbuflen) {

    int errn;
    int yy;
    int mm;
    int dd;
    int hh;
    int mi;
    int ss;
    d_date rdate;

    errn = 0;

    if (fdin->in_years != CAL_YEAR_NULL || fdin->in_months != 0) {
        d_date_to_ymdhms(fdate, &yy, &mm, &dd, &hh, &mi, &ss);

        if (fdin->in_years != CAL_YEAR_NULL) {
            if (subbing) yy -= fdin->in_years;
            else         yy += fdin->in_years;
        }
        if (subbing) mm -= fdin->in_months;
        else         mm += fdin->in_months;
        while (mm > 12) { mm -= 12; yy++; }
        while (mm <  1) { mm += 12; yy--; }

        rdate = make_d_date(yy, mm, dd, hh, mi, ss);
    } else {
        rdate = fdate;
    }

    if (subbing) rdate -= fdin->in_seconds;
    else         rdate += fdin->in_seconds;

    errn = fmt_d_date(rdate, outfmt, outbuf, outbuflen);

    return (errn);
}
/***************************************************************/
int dateaddsub(const char * fromdate,
              const char * fromfmt,
              const char * interval,
              const char * intervalfmt,
              int          subbing,
              const char * destfmt,
              char  *      outbuf,
              int          outbuflen) {

    int errn;
    int ix;
    d_date fdate;
    struct d_interval odin;

    ix = 0;
    errn = parsedate(fromdate, &ix, fromfmt, &fdate);
    if (!errn) {
        if (fromdate[ix]) {
            errn = EPD_EXPEOD;
        } else if (interval[0] || intervalfmt[0]) {
            ix = 0;
            errn = parse_interval(interval, &ix, intervalfmt, &odin);
            if (!errn) {
                if (interval[ix]) {
                    errn = EPD_EXPEOD;
                }
            }
        }
    }

    if (!errn) {
        if (interval[0] || intervalfmt[0]) {
            errn = add_sub_to_date(fdate, &odin, subbing, destfmt, outbuf, outbuflen);
        } else {
            errn = fmt_d_date(fdate, destfmt, outbuf, outbuflen);
        }
    }

    return (errn);
}
/***************************************************************/
int subtract_dates( d_date       date1,
                    d_date       date2,
                    const char * outfmt,
                    char  *      outbuf,
                    int          outbuflen) {

    int errn;
    int yy1;
    int yy2;
    int mm1;
    int mm2;
    int dd1;
    int dd2;
    struct d_interval din;

    errn = 0;

    d_date_to_ymd(date1, &yy1, &mm1, &dd1);
    d_date_to_ymd(date2, &yy2, &mm2, &dd2);

    din.in_years   = yy1 - yy2;
    din.in_months  = mm1 - mm2;
    din.in_seconds = date1 - date2;

    errn = fmt_d_interval(&din, outfmt, outbuf, outbuflen);

    return (errn);
}
/***************************************************************/
int subdates( const char*  dbuf1,
              const char * fmt1,
              const char*  dbuf2,
              const char * fmt2,
              const char * outfmt,
              char  *      outbuf,
              int          outbuflen) {

    int errn;
    int ix;
    d_date date1;
    d_date date2;

    outbuf[0] = '\0';
    ix = 0;
    errn = parsedate(dbuf1, &ix, fmt1, &date1);
    if (!errn) {
        if (dbuf1[ix]) {
            errn = EPD_EXPEOD;
        } else {
            ix = 0;
            errn = parsedate(dbuf2, &ix, fmt2, &date2);
            if (!errn) {
                if (dbuf2[ix]) {
                    errn = EPD_EXPEOD;
                }
            }
        }
    }

    if (!errn) {
        errn = subtract_dates(date1, date2, outfmt, outbuf, outbuflen);
    }

    return (errn);
}
/**************************************************************/
#endif /* UNNEEDED */




