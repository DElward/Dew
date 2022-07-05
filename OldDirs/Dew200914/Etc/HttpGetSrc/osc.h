/***************************************************************/
/*  osc.h  --  OS Specific and utilities                       */
/***************************************************************/

#define ERRNO           errno

#define OSFTYP_NONE     0
#define OSFTYP_FILE     1
#define OSFTYP_DIR      2
#define OSFTYP_OTHR     3

struct dynl_info {
    int                     dynl_ndynprocs;
    struct dynprocinfo **   dynl_dynprocs;

    struct dynamic_curl *   dcf;
};
/***************************************************************/

void Strmcpy(char * tgt, const char * src, size_t tmax);

int os_ftype(const char * fname);
int os_get_registry_jvm(char * reg_val, int reg_val_sz);
void os_get_message(long errCode, char * errMsg, long errMsgSize);
char * os_get_message_str(long errCode);

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
#if defined(_Windows)
    #if POINTERS_64_BIT
        #define LIBCURL_LIB_HOME ""
        #define LIBCURL_LIB_PATH "LIB64"
        #define LIBCURL_LIB_NAME "LIBCURL.DLL"
    #else
        #define LIBCURL_LIB_HOME ""
        #define LIBCURL_LIB_PATH "LIB32"
        #define LIBCURL_LIB_NAME "LIBCURL.DLL"
    #endif
#elif defined(__hpux)
    #if POINTERS_64_BIT
        #define LIBCURL_LIB_HOME ""
        #define LIBCURL_LIB_PATH "lib64"
        #define LIBCURL_LIB_NAME "libcurl.sl"
    #else
        #define LIBCURL_LIB_HOME ""
        #define LIBCURL_LIB_PATH "lib32"
        #define LIBCURL_LIB_NAME "libcurl.sl"
    #endif
#else
    #if POINTERS_64_BIT
        #define LIBCURL_LIB_HOME ""
        #define LIBCURL_LIB_PATH "lib64"
        #define LIBCURL_LIB_NAME "libcurl.so"
    #else
        #define LIBCURL_LIB_HOME ""
        #define LIBCURL_LIB_PATH "lib32"
        #define LIBCURL_LIB_NAME "libcurl.so"
    #endif
#endif

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
