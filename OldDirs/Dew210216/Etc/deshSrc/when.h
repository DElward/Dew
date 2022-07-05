/***************************************************************/
/* When.h                                                      */
/***************************************************************/
#ifndef WHENH_INCLUDED
#define WHENH_INCLUDED
/***************************************************************/
typedef int64 DATINT;

typedef int     d_daynum;    /* Days since Day 0 - Friday December 31, 1999 */
typedef int     d_sec;      /* Second within day */
typedef DATINT  d_date;     /* Daynum * SECS_IN_DAY + secs */
typedef int     d_ymd;      /* Date in YYYYMMDD format */
typedef int     d_doy;      /* Day of year 1-366 */
typedef DATINT  d_interval; /* Interval in seconds */
#define         SECS_IN_DAY         86400L

/*
Unit             Number     Bits
years             8,000     12.97
days          2,922,000     21.48
hours        70,128,000     26.06
mins      4,207,680,000     31.97
secs    252,460,800,000     37.88
*/

#define DOW_SUNDAY      0
#define DOW_MONDAY      1
#define DOW_TUESDAY     2
#define DOW_WEDNESDAY   3
#define DOW_THURSDAY    4
#define DOW_FRIDAY      5
#define DOW_SATURDAY    6

#define SS_DAYNUM_DIFF     36523        /* SQL Server adjustment */
#define SS_TICKS_IN_DAY    25920000L    /* SQL Server adjustment */

#define FTM_YEAR(yy) ((yy) + 1900)
#define FTM_MONTH(mm) ((mm) + 1)
#define FTM_DAY(dd) (dd)

/* Unix Time  0h Jan 1, 1970  Julian Day - 2440587 � 86400 = 210866716800  */
#define TIME_D_DATE_DAYS_DIFF       	(2440587)
#if IS_WINDOWS
    #define TIME_D_DATE_DIFF_NO_TZ       (210866716800i64)
#else
    #define TIME_D_DATE_DIFF_NO_TZ       (210866716800ll)
#endif

/*
** time_t day 0 = Thursday January 1, 1970 (Julian day 2440588)
** date_t day 0 = Friday December 31, 1999
*/
/*#define TIME_D_DATE_DIFF    946623600L  -- when in PDT */
#define TIME_D_DAYS_DIFF        2440588L   /* 10956 * SECS_IN_DAY = 946598400 */
/* #define TIME_D_DATE_DIFF_NO_TZ  946598400L */
#define TIME_D_DATE_DIFF         (TIME_D_DATE_DIFF_NO_TZ - ((TIMEZONE) - 3600 * (DAYLIGHT)))
#define TIME_D_DATE_DIFF_DST(d)  (TIME_D_DATE_DIFF_NO_TZ - ((TIMEZONE) - 3600 * (d)))


#define JULIAN_EXCEL_DIFF       (2415018L)

#define ADJUST_YEARS    32          /* must be multiple of 4 */
#define ADJUST_DAYS     11688       /* ADJUST_YEARS / 4 * 1461 */
#define ADJUST_SECONDS  1009843200L /* ADJUST_DAYS * SECS_IN_DAY */

#if IS_WINDOWS
    #define TIMEZONE _timezone
    #define DAYLIGHT _daylight
#else
    extern long timezone;
    extern int daylight;

    #define TIMEZONE (int)timezone
    #define DAYLIGHT daylight
#endif

#define EPD_DBADCHAR        101
#define EPD_DMATCH          102
#define EPD_DMONNAM         103
#define EPD_DDAY            104
#define EPD_DMONTH          105
#define EPD_DYY             106
#define EPD_DYYYY           107
#define EPD_DDATE           108
#define EPD_DHOUR           109
#define EPD_DMINUTE         110
#define EPD_DSECOND         111
#define EPD_DEOD            112
#define EPD_DWDNAME         113
#define EPD_EXPEOD          114
#define EPD_BADUNIT         115
#define EPD_BADRESULT       116
#define EPD_DAMPM           117
#define EPD_DHOUR12         118
#define EPD_DWDCONFLICT     119
#define EPD_DDAYOFYEAR      120
#define EPD_DJULDATE        121
#define EPD_DJULCONFLICT    122
#define EPD_DJDOYCONFLICT   123
#define EPD_DJULDOY         124
#define EPD_DBCAD           125
#define EPD_DEXDATE         126
#define EPD_DEXCONFLICT     127
#define EPD_DJULFRAC        128
#define EPD_DJULFCONFLICT   129
#define EPD_DEXFRAC         130
#define EPD_DEXFCONFLICT    131
#define EPD_DNDAYS          132
#define EPD_DNHOURS         133
#define EPD_DNMINUTES       134
#define EPD_DNSECONDS       135
#define EPD_DNMONTHS        136
#define EPD_DNYEARS         137
#define EPD_DINTCONFLICT    138
#define EPD_DINTMONTH       139

/***************************************************************/
struct timestamp {
    int ts_daynum;
    int ts_second;
    int ts_microsec;
};
struct proctimes {
    struct timestamp pt_wall_time;
    struct timestamp pt_cpu_time;
    struct timestamp pt_sys_time;
    int              pt_use_hi_p;
    int64            pt_hi_p;
    double           pt_hi_elap_secs;
};


void glob_init_time(void);
void get_cpu_timer(struct proctimes * pt);

void start_cpu_timer(struct proctimes * pt_start, int use_hi_p);
void stop_cpu_timer(struct proctimes * pt_start,
                    struct proctimes * pt_end,
                    struct proctimes * pt_result);

int fmt_d_date(d_date ddt, const char * fmt, char *buf, int bufsize);
d_date time_t_to_d_time(time_t tt, int is_dst);
d_date time_t_to_d_time_dst(time_t tt, int dst);
d_date get_time_now(int is_dst);
int ymd_to_dayofweek(int year, int month, int day);

void daynum_to_ymd(d_daynum daynum, int * year, int * month, int * day);
d_daynum ymd_to_daynum(int year, int month, int day);

void get_pd_errmsg(int pderr, char * errmsg, int errmsgmax);
int parsedate(const char* dbuf,
                int *ix,
                const char * fmt,
                d_date * date);
int dateaddsub(const char * fromdate,
              const char * fromfmt,
              const char * interval,
              const char * intervalfmt,
              int          subbing,
              const char * destfmt,
              char  *      outbuf,
              int          outbuflen);
int subdates( const char*  dbuf1,
              const char * fmt1,
              const char*  dbuf2,
              const char * fmt2,
              const char * outfmt,
              char  *      outbuf,
              int          outbuflen);

#endif /* WHENH_INCLUDED */
/***************************************************************/
