/***************************************************************/
/* config.h                                                    */
/***************************************************************/
#include <limits.h>
#include <sys/types.h>
/***************************************************************/

#ifdef WIN32
    #define IS_WINDOWS  1
#else
    #define IS_WINDOWS  0
#endif

#ifndef IS_DEBUG
    #ifdef _DEBUG
        #define IS_DEBUG    1
    #else
        #define IS_DEBUG    0
    #endif
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

#if (LONG_MAX == 2147483647L)
    #define LONGS_64_BIT    0
#else
    #define LONGS_64_BIT    1  /* Hopefully */
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

/* Guess */
#if IS_WINDOWS
    #if defined(_x64)
        #define POINTERS_64_BIT 1
        typedef uint32 REG_T;
    #else
        #define POINTERS_64_BIT 0
        typedef uint32 REG_T;
    #endif
#else
    #if (LONG_MAX == 2147483647L)
        #define POINTERS_64_BIT 0
        typedef uint32 REG_T;
    #else
        #define POINTERS_64_BIT 1
        typedef uint64 REG_T;
    #endif
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

#if IS_WINDOWS
    #define DIR_DELIM       '\\'
    #define PATH_MAXIMUM    (MAX_PATH + 4)

    #define DUP             _dup
    #define FDOPEN          _fdopen
    #define FILENO          _fileno
#else
    #define DIR_DELIM       '/'
    #define PATH_MAXIMUM    (PATH_MAX + 4)

    #define DUP             dup
    #define FDOPEN          fdopen
    #define FILENO          _fileno
#endif


/***************************************************************/
