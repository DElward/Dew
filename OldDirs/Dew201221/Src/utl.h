#ifndef UTLH_INCLUDED
#define UTLH_INCLUDED
/***************************************************************/
/*  utl.h  --  Utilities                                       */
/***************************************************************/

struct dynl_info {
    int                     dynl_ndynprocs;
    struct dynprocinfo **   dynl_dynprocs;

    struct dynamic_curl *   dcf;
};
typedef int     d_daynum;    /* Days since Day 0 - Friday December 31, 1999 */

/***************************************************************/

#define HTMLCEOF             ((HTMLC)EOF)
#define HTMLCOVFL            254
#define HTMLC_BYTES(nc) ((nc) * HTMLCSZ)

#define HTTPCMAXSZ           4 /* Maximum number of bytes in raw char */
#define HTMLCSZ              2 /* Number of bytes in a translated char */
/***************************************************************/
/* URLs                                                        */
/***************************************************************/
#define URI_SCHEME_NONE     0
#define URI_SCHEME_FILE     4101
#define URI_SCHEME_HTTP     4102
#define URI_SCHEME_HTTPS    4103
#define URI_SCHEME_FTP      4104
#define URI_SCHEME_MAILTO   4105
#define URI_SCHEME_URN      4106

#define URI_ERR_NO_URI              4201
#define URI_ERR_EXP_SLASH_SLASH     4202
#define URI_ERR_BAD_PORT            4203

#define URI_TOKEN_MAX       254
#define URI_SCHEME_MAX      10

struct uri {        /* ftp://user:password@host:port/path  */
    int             uri_scheme;
    int             uri_errnum;
    char *          uri_user;
    char *          uri_pass;
    char *          uri_host;
    int             uri_port;
    char *          uri_raw_path;   /* May have %xx's */
};

/***************************************************************/

char * quick_strcpy(char * tgtbuf, int tgtmax, const char * src, int * need_free);
void strmcpy(char * tgt, const char * src, size_t tmax);
void append_chars(char ** cstr,
                    int * slen,
                    int * smax,
                    const char * val,
                    int vallen);

char * os_get_message_str(long errCode);

int htmlctochar(char * tgt,
            const HTMLC * src,
            int srclen,
             int tgtmax);
void qhtmlctostr(char * tgt, const HTMLC * src, int tgtlen);
int htmlcstrbytes(const HTMLC * hbuf);
int htmlclen(const HTMLC * hbuf);
void strtohtmlc(HTMLC * tgt, const char * src, int tgtlen);
HTMLC * htmlcstrdup(const HTMLC * hbuf);

int uri_get_scheme(const char * uri_str, int * uix);

struct dynl_info * dynl_dynloadnew(void);
void dynl_dynloadfree(struct dynl_info * dynl);
void * dynl_dynloadproc(struct dynl_info * dynl,
                    char * libpath,
                    char * libname,
                    char * procname,
                    int * err,
                    char* errmsg,
                    int errmsgmax);
void dynl_dyntraceentry(struct dynl_info * dynl,
                   char * dynproc,
                   int    flags,
                   char * libhome,      /* flags & 8 */
                   char * libpath,      /* flags & 4 */
                   char * libname,      /* flags & 2 */
                   char * procname);    /* flags & 1 */

void calcpathlib(char * libhome,    /* in  */
                 char * libpath,    /* in  */
                 char * libname,    /* in  */
                 char * dflthome,   /* in  */
                 char * dfltpath,   /* in  */
                 char * dfltname,   /* in  */
                 char * lpath,      /* out */
                 char * lname);     /* out */
/***************************************************************/
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

void start_cpu_timer(struct proctimes * pt_start, int use_hi_p);
void stop_cpu_timer(struct proctimes * pt_start,
                    struct proctimes * pt_end,
                    struct proctimes * pt_result);
/***************************************************************/
#endif /* UTLH_INCLUDED */
