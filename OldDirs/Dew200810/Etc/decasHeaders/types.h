/***************************************************************/
/*  types.h - Copy program                                     */
/***************************************************************/
#include <limits.h>
#include <sys/types.h>
/***************************************************************/
/* Things needed for directory reading                         */
#include <sys/stat.h>

#ifdef WIN32
    #define IS_WINDOWS 1
    typedef wchar_t tWCHAR; /* Added 04/30/2015 */
#else
    #define IS_WINDOWS 1
    typedef unsigned short tWCHAR; /* Added 04/30/2015 */
#endif

#ifdef _DEBUG
    #define IS_DEBUG    1
#else
    #define IS_DEBUG    0
#endif

#ifdef _x64
    #define IS_64_BITS  1
#else
    #define IS_64_BITS  0
#endif
/***************************************************************/
typedef unsigned char uchar;
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

#if IS_64_BITS
    typedef unsigned long long pint;
#else
    typedef unsigned int pint;
#endif

#if IS_WINDOWS
typedef __int64 int64;
typedef unsigned __int64 uint64;
#else
typedef long long int64;
typedef unsigned long long uint64;
#endif

#if IS_DEBUG
    #define USE_CMEM    1
#else
    #define USE_CMEM    0
#endif
/***************************************************************/

#if USE_CMEM
#include "cmemh.h"
#else
#define New(t,n)         ((t*)calloc(sizeof(t),(n)))
#define Realloc(p,t,n)   ((t*)realloc((p),sizeof(t)*(n)))
#define Free(p)           free(p)
#endif

#define Strdup(s)        (strcpy(New(char,strlen(s)+1),(s)))
#define MAX(n,m)         ( (n)>(m) ? (n) : (m) )
#define MIN(n,m)         ( (n)<(m) ? (n) : (m) )
#define Stricmp         istrcmp
#define Memicmp         imemicmp
#define IStrlen(s)      ((int)(strlen(s)))

/* #define TMax(x,y) ((x) > (y) ? (x) : (y)) */
/* #define TMin(x,y) ((x) < (y) ? (x) : (y)) */

#ifndef PATH_MAX
    #ifdef MAX_PATH
       #define PATH_MAX MAX_PATH
    #else
       #ifdef _MAX_PATH
         #define PATH_MAX _MAX_PATH
       #else
         #define PATH_MAX 255
       #endif
    #endif
#endif

/***************************************************************/

