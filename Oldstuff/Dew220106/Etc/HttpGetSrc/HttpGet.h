/***************************************************************/
/*  HttpGet.h  --  Header for HttpGet                          */
/***************************************************************/

#include "cmemh.h"

#ifndef New
    #define New(t,n)         ((t*)calloc(sizeof(t),(n)))
    #define Realloc(p,t,n)   ((t*)realloc((p),sizeof(t)*(n)))
    #define Free(p)           free(p)
#endif

#define Strdup(s)       (strcpy(New(char,strlen(s)+1),(s)))
#define IStrlen(s)      ((int)strlen(s))
#define Stricmp(s,t)    (istrcmp((s),(t)))

#define MAJOR_VERSION "0"
#define MINOR_VERSION "0"
#define VERSION MAJOR_VERSION "." MINOR_VERSION
#define PROGRAM_NAME "HttpGet"

#define HG_ERR_IX_DELTA 2000

#define GLB_ERRMSG_SIZE 256

/***************************************************************/
/** Dynamic curl                                              **/
/***************************************************************/

#include "curl/curl.h"

/* curl_easy_XXX typedef's */
typedef CURL *   (*p_curl_easy_init)(void);
typedef CURLcode (*p_curl_easy_setopt)(CURL *curl, CURLoption option, ...);
typedef CURLcode (*p_curl_easy_perform)(CURL *curl);
typedef CURLcode (*p_curl_easy_cleanup)(CURL *curl);
typedef const char * (*p_curl_easy_strerror)(CURLcode curlcode);

/* curl_XXX function typedef's */
typedef struct curl_slist *(*p_curl_slist_append)(struct curl_slist *,
                                                 const char *);
typedef void (*p_curl_slist_free_all)(struct curl_slist *);

/* curl_global_XXX typedef's */
typedef CURLcode (*p_curl_global_init)(long flags);
typedef CURLcode (*p_curl_global_cleanup)(void);

/********************************/

struct dynamic_curl {
    int                      libcurl_resolved;

/* curl_easy_XXX function variables */
    p_curl_easy_init         dcf_curl_easy_init;
    p_curl_easy_setopt       dcf_curl_easy_setopt;
    p_curl_easy_perform      dcf_curl_easy_perform;
    p_curl_easy_cleanup      dcf_curl_easy_cleanup;
    p_curl_easy_strerror     dcf_curl_easy_strerror;

/* curl_XXX function variables */
    p_curl_slist_append      dcf_curl_slist_append;
    p_curl_slist_free_all    dcf_curl_slist_free_all;

/* curl_global_XXX function variables */
    p_curl_global_init       dcf_curl_global_init;
    p_curl_global_cleanup    dcf_curl_global_cleanup;
};
/***************************************************************/

struct hgerr {  /* hgx_ */
    int     hgx_errnum;
    char *  hgx_errmsg;
};

#define HGERREOF        (-2000)
#define HGERRLOADJVM    (-2001)

struct hgenv {  /* hge_ */
    struct dynl_info *      hge_dynl_info;
    char *                  hge_wsdlfile;
    struct frec *           hge_frec_in;
    struct frec *           hge_frec_out;
    int                     hge_done;
    int                     hge_nerrs;
    struct hgerr **         hge_errs;
    struct arginfo *        hge_args;
    struct wsglob *         hge_wsg;
};

/***************************************************************/

int istrcmp(char * b1, char * b2);

int hg_set_error(struct hgenv * hge, int inhgerr, const char *fmt, ...);

void hgSetWsdlFile(struct hgenv * hge, const char * wsdlfile);
char * hgGetWsdlFile(struct hgenv * hge);

/***************************************************************/
