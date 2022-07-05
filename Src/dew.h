#ifndef DEWH_INCLUDED
#define DEWH_INCLUDED
/***************************************************************/
/* dew.h                                                       */
/***************************************************************/

#define MAJOR_VERSION "0"
#define MINOR_VERSION "0"
#define VERSION MAJOR_VERSION "." MINOR_VERSION
#define PROGRAM_NAME "dew"

/*
Version Date     Description
0.00    07-28-20 Began work

Date     Enhancement request
*/
/***************************************************************/
#define ERRNO       (errno)
#ifdef _Windows
    #define GETERRNO    (GetLastError())
#else
    #define GETERRNO    ERRNO
#endif
/***************************************************************/
#define Strdup(s)       (strcpy(New(char,strlen(s)+1),(s)))
#define Stricmp         _stricmp
#define Memicmp         _memicmp
#define Max(n,m)        ( (n)>(m) ? (n) : (m) )
#define Min(n,m)        ( (n)<(m) ? (n) : (m) )
#define IStrlen(s)      (int)strlen(s)

#define CMEM 1

#if CMEM
    #include "cmemh.h"
#else
    #define New(t,n)         ((t*)calloc(sizeof(t),(n)))
    #define Realloc(p,t,n)   ((t*)realloc((p),sizeof(t)*(n)))
    #define Free(p)           free(p)
#endif

/***************************************************************/
/*
** Config
*/
/***************************************************************/
#include <limits.h>
#include <sys/types.h>
/***************************************************************/

#if defined(_Windows) || defined (WIN32)
    #define DIRDELIM '\\'
    #define IS_WINDOWS  1
    typedef wchar_t HTMLC;
#else
    #define DIRDELIM '/'
    #define IS_WINDOWS  0
    typedef unsigned short HTMLC;
#endif

#if (INT_MAX == 32767)
    typedef int int16;
    typedef unsigned int uint16;
#elif (SHRT_MAX == 32767)
    typedef short int int16;
    typedef unsigned short int uint16;
#else
    #error No int16 value
#endif

#if (INT_MAX == 2147483647L)
    typedef int int32;
    typedef unsigned int uint32;
#elif (LONG_MAX == 2147483647L)
    typedef long int int32;
    typedef unsigned long int uint32;
#else
    #error No int32 value
#endif

#if IS_WINDOWS
    typedef __int64 int64;
    typedef unsigned __int64 uint64;
#else
    typedef long long int64;
    typedef unsigned long long uint64;
#endif

#if IS_WINDOWS
    #if defined(_x64)
        #define POINTERS_64_BIT (1)
    #else
        #define POINTERS_64_BIT (0)
    #endif
#elif defined(__hpux)
    #if defined(__LP64__)
        #define POINTERS_64_BIT (1)
    #else
        #define POINTERS_64_BIT (0)
    #endif
#elif defined(GENERIC_64BIT)
    #define POINTERS_64_BIT (1)
#elif defined(LINUX_X86_64)
    #define POINTERS_64_BIT (1)
#else
    #define POINTERS_64_BIT (0)
#endif

#if POINTERS_64_BIT
    typedef int64 intr;
    typedef uint64 uintr;
#else
    typedef int32 intr;
    typedef uint32 uintr;
#endif

#ifndef IS_DEBUG
    #if defined(_DEBUG) || defined(DEBUG)
        #define IS_DEBUG    1
    #else
        #define IS_DEBUG    0
    #endif
#endif


/***************************************************************/
/*
** Globals
*/
/***************************************************************/

#define LINE_MAX_SIZE   1024
#define LINE_MAX_LEN    (LINE_MAX_SIZE - 1)

#define TOKE_MAX_SIZE   PATH_MAXIMUM
#define TOKE_MAX_LEN    (TOKE_MAX_SIZE - 1)

#define STOKE_MAX_SIZE  16  /* small token */

#define MSGOUT_SIZE       LINE_MAX_SIZE
#define ERRMSG_MAX      255
#define JERRMSG_MAX     ERRMSG_MAX
#define FERRMSG_MAX     94  /* See strerror() documentation */
#define FBUFFER_SIZE    2048
#define DEWMEM_FNAME    "DewMem.txt"

/***************************************************************/
#define ESTAT_PARMEXPFNAME          (-3401)
#define ESTAT_PARMBAD               (-3402)
#define ESTAT_PARM                  (-3403)
#define ESTAT_PARMFGETS             (-3404)
#define ESTAT_PARMDEPTH             (-3405)
#define ESTAT_PARMFILE              (-3406)
#define ESTAT_FIND_LIBCURL          (-3407)
#define ESTAT_GET_URL               (-3408)
#define ESTAT_PROCESS_URL           (-3409)
#define ESTAT_OPEN_FILE             (-3410)
#define ESTAT_READ_FILE             (-3411)
#define ESTAT_BAD_JSUSE             (-3412)
#define ESTAT_UNSUPPORTED_FILTYP    (-3413)
#define ESTAT_EXECUTE_STRING        (-3414)

/***************************************************************/

#define GLB_FLAG_PAUSE          1
#define GLB_FLAG_VERSION        2
#define GLB_FLAG_HELP           4
#define GLB_FLAG_DONE           8
#define GLB_FLAG_SHOWMEM        16

#define GLB_JSFLAG_NODEJS           1
#define GLB_JSFLAG_DEBUGFEATURES    2


struct globals { /* glb_ */
    int                 glb_nRefs;
	int                 glb_flags;
	int                 glb_jsflags;
    struct htenv *      glb_hte;
    struct jsenv *      glb_jse;
	int                 glb_nparms;
	char              **glb_parms;
	FILE               *glb_outfile;
	FILE               *glb_errfile;
};
/***************************************************************/
int set_error(int estat, const char * fmt, ...);
int set_error_f(int estat, int ern, const char * fmt, ...);
void printout (char * fmt, ...);
char * get_dewvar(const char * vname);

void release_globals(struct globals * glb);
struct globals * get_globals(void);

/***************************************************************/
#endif /* DEWH_INCLUDED */

