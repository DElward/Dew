/***************************************************************/
/* config.h                                                    */
/***************************************************************/
#include <limits.h>
#include <sys/types.h>
/***************************************************************/

#define FUNCTRACE 0

#ifdef WIN32
    #define IS_WINDOWS  	1
    #define IS_MACINTOSH	0
    #define IS_LINUX        0
    #if defined(_x64)
        #define IS_64_BIT  1
    #else
        #define IS_64_BIT  0
    #endif
#elif defined(LINUX_X86)
    #define IS_WINDOWS  	0
    #define IS_MACINTOSH	0
    #define IS_LINUX        1
    #if defined(LINUX_X86_64)
        #define IS_64_BIT  1
    #else
        #define IS_64_BIT  0
    #endif
#elif defined(MACOSX)
    #define IS_WINDOWS  	0
    #define IS_MACINTOSH	1
    #define IS_LINUX        0
    #define IS_64_BIT  		1
#else
    #define IS_WINDOWS  	0
    #define IS_MACINTOSH	0
    #define IS_LINUX        0
    #define IS_64_BIT  		0
#endif

#ifndef IS_DEBUG
    #if defined(_DEBUG) ||  defined(DEBUG)
        #define IS_DEBUG    1
    #else
        #define IS_DEBUG    0
    #endif
#endif

#if IS_DEBUG
    #define INTERNAL_HELP   0
#else
    #define INTERNAL_HELP   1
#endif

/***************************************************************/
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
    #define INT64_T __int64
    typedef __int64 int64;
    typedef unsigned __int64 uint64;
#else
    #define INT64_T long long
    typedef long long int64;
    typedef unsigned long long uint64;
#endif

typedef unsigned char uchar;

#if IS_WINDOWS
    #if defined(_x64)
        typedef __int64 intr;
        typedef unsigned __int64 uintr;
    #else
        typedef long intr; /* true long */
        typedef unsigned long uintr; /* true long */
    #endif
#else
    typedef long intr; /* true long */
    typedef unsigned long uintr; /* true long */
#endif

/***************************************************************/

#define ERRNO           errno
#define Istrlen(s) ((int)strlen(s))
#define Int             int
/***************************************************************/
