#ifndef SNPRFH_INCLUDED
#define SNPRFH_INCLUDED
/***************************************************************/
/* snprf.h                                                     */
/***************************************************************/

#define PRINTF_I64 1

#include <stdarg.h>

#if PRINTF_I64
    #ifdef WIN32
        #define INT64_T __int64
        #define CON_10_64           (10i64)
        #define CON_MIN_64          (0x8000000000000000i64) /* fixed 4/1/2012 */
        #define CON_MAX_64          (0x7fff7fffffffffffi64) /* fixed 4/1/2012 */
    #else
        #define INT64_T long long
        #define CON_10_64           (10ll)
        #define CON_MIN_64          (0x8000000000000000ll) /* fixed 4/1/2012 */
        #define CON_MAX_64          (0x7fffffffffffffffll) /* fixed 4/1/2012 */
    #endif
#endif

/***************************************************************/
char * Smprintf(const char *fmt, ...);
char * Vsmprintf(const char *fmt, va_list args);
int Vsnprintf(char *buf, size_t size, const char *fmt, va_list args);
int Snprintf(char *buf, size_t size, const char *fmt, ...);
int Fmprintf(FILE * fref, const char *fmt, ...);
int Vfmprintf(FILE * fref, const char *fmt, va_list args);
char * Vsmprintf(const char *fmt, va_list args);
/***************************************************************/
#endif /* SNPRFH_INCLUDED */

