#ifndef HTTPIOH_INCLUDED
#define HTTPIOH_INCLUDED
/*
**  httpio.h  --  Header file for httpio.c
*/
/***************************************************************/
#define HTE_ELOADLIBCURL    (-6200)
#define HTE_ECURLINIT       (-6201)
#define HTE_ECURLPROCESS    (-6202)
#define HTE_ENOTWSDL        (-6203)
#define HTE_EPARSE          (-6204)
#define HTE_EWSDLERR        (-6205)
#define HTE_ECURLPREPCURL   (-6206)
#define HTE_EBADREQUEST     (-6207) /* 03/12/2020 */
#define HTE_EBADURL         (-6208) /* 04/29/2020 */
/***************************************************************/

#define HTE_DEBUG_SHOW_HEADERS          1
#define HTE_DEBUG_SHOW_RESPONSE_HEADERS 2

#define WH_CURL_AGENT "libcurl-agent/1.0"
#define WH_DATA_PAGE_INCREMENT  16384
#define WH_DATA_PAGE_MASK       (~(WH_DATA_PAGE_INCREMENT - 1))

#if POINTERS_64_BIT
    #define WH_DYNLOAD_LIBRARY    "lib64"
#else
    #define WH_DYNLOAD_LIBRARY    "lib32"
#endif

#if ! LIBCURL_ACCESS
    #define DEFAULT_LIBCURL_LIBRARY  ""
#else
  #if IS_WINDOWS
    #define DEFAULT_LIBCURL_LIBRARY  WH_DYNLOAD_LIBRARY # "\\" # "libcurl.dll"
  #else
    #define DEFAULT_LIBCURL_LIBRARY  WH_DYNLOAD_LIBRARY # "/" # "libcurl.sl"
  #endif
#endif /* LIBCURL_ACCESS */

#if IS_WINDOWS
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

#define HTX_ERR_IX_DELTA    2000

/***************************************************************/

struct htenv;
/***************************************************************/

struct htenv * htNewEnv(void);
void htFreeEnv(struct htenv * hte);
char * ht_get_error_msg(struct htenv * hte, int eref);
int ht_get_url(struct htenv * hte,
                const char * url,
                const char * httphdrs,
                char ** httpresponse,
                int   * httplength);
/***************************************************************/
#endif /* HTTPIOH_INCLUDED */
