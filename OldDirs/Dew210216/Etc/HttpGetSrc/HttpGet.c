/*
/*  HttpGet.c  --  Functions for HttpGet
*/
/***************************************************************/
/*
History:
10/27/15 DTE  0.0 Began work
*/
/***************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>

#define ARG_FLAG_PAUSE       1
#define ARG_FLAG_HELP        2
#define ARG_FLAG_DONE        4
#define ARG_FLAG_IN          8
#define ARG_FLAG_OUT         16
#define ARG_FLAG_VERSION     32
#define ARG_FLAG_VERSION_NUM 64

struct arginfo {
    int     arg_flags;
    char *  arg_infname;
    char *  arg_outfname;
};

/***************************************************************/

#include "config.h"

#include "HttpGet.h"
#include "osc.h"
#include "snprfh.h"
#include "wsdl.h"
#include "wsproc.h"

/***************************************************************/
int hg_set_error(struct hgenv * hge, int inhgerr, const char *fmt, ...) {

    va_list args;
    int eix;
    char * emsg;
    struct hgerr * hgx;

    va_start (args, fmt);
    emsg = Vsmprintf (fmt, args);
    va_end (args);

    eix = 0;
    while (eix < hge->hge_nerrs && hge->hge_errs[eix]) eix++;

    if (eix == hge->hge_nerrs) {
        hge->hge_errs = Realloc(hge->hge_errs, struct hgerr*, hge->hge_nerrs + 1);
        eix = hge->hge_nerrs;
        hge->hge_nerrs += 1;
    }

    hgx = New(struct hgerr, 1);
    hge->hge_errs[eix] = hgx;

    hgx->hgx_errmsg = emsg;
    hgx->hgx_errnum = inhgerr;

    return (eix + HG_ERR_IX_DELTA);
}
/***************************************************************/
static int hg_clear_error(struct hgenv * hge, int eref) {

    int eix;

    if (eref < 0) return (0);

    eix = eref - HG_ERR_IX_DELTA;
    if (eix < 0 || eix >= hge->hge_nerrs) return (-1);

    Free(hge->hge_errs[eix]->hgx_errmsg);
    Free(hge->hge_errs[eix]);
    hge->hge_errs[eix] = NULL;

    while (hge->hge_nerrs > 0 && hge->hge_errs[hge->hge_nerrs - 1] == NULL)
        hge->hge_nerrs -= 1;

    return (0);
}
/***************************************************************/
static void hg_clear_all_errors(struct hgenv * hge) {

    int eix;

    while (hge->hge_nerrs > 0) {
        eix = hge->hge_nerrs;
        while (eix > 0 && hge->hge_errs[eix - 1] == NULL) eix--;
        if (eix) {
            hg_clear_error(hge, eix - 1 + HG_ERR_IX_DELTA);
        } else {
            hge->hge_nerrs = 0; /* stop loop */
        }
    }

    Free(hge->hge_errs);
    hge->hge_errs = NULL;
}
/***************************************************************/
static char * hg_get_error_msg(struct hgenv * hge, int eref) {

    int eix;
    char * emsg;

    if (eref < 0) {
        switch (eref) {
            case HGERREOF:
                emsg = Smprintf("%s HGKERR %d",
                    "End of file", eref);
                break;
            default:
                emsg = Smprintf("%s HGKERR %d",
                    "Unknown error number", eref);
                break;
        }
    } else {
        eix = eref - HG_ERR_IX_DELTA;
        if (eix < 0 || eix >= hge->hge_nerrs) return NULL;
        if (!hge->hge_errs[eix]) return NULL;

        emsg = Smprintf("%s HGERR %d",
            hge->hge_errs[eix]->hgx_errmsg, hge->hge_errs[eix]->hgx_errnum);
    }

    return (emsg);
}
/***************************************************************/
static int hg_show_error(struct hgenv * hge, FILE * outf, int eref) {

    char * emsg;

    emsg = hg_get_error_msg(hge, eref);
    if (!emsg) return (-1);

    fprintf(outf, "%s\n", emsg);

    hg_clear_error(hge, eref);
    Free(emsg);

    return (0);
}
/***************************************************************/
static void hg_show_all_errors(struct hgenv * hge, FILE * outf) {

    int eix;

    while (hge->hge_nerrs > 0) {
        eix = hge->hge_nerrs;
        while (eix > 0 && hge->hge_errs[eix - 1] == NULL) eix--;
        if (eix) {
            hg_show_error(hge, outf, eix - 1 + HG_ERR_IX_DELTA);
        } else {
            hge->hge_nerrs = 0; /* stop loop */
        }
    }

    hg_clear_all_errors(hge);
}
/***************************************************************/
static int hg_get_error(struct hgenv * hge) {

    int eref;

    eref = 0;
    if (hge->hge_nerrs > 0) {
        eref = hge->hge_nerrs - 1 + HG_ERR_IX_DELTA;
    }

    return (eref);
}
/***************************************************************/
struct arginfo * arg_init(void) {

    struct arginfo * args;

    args = New(struct arginfo, 1);
    args->arg_flags     = 0;
    args->arg_infname   = NULL;
    args->arg_outfname  = NULL;

    return (args);
}
/***************************************************************/
void arg_shut(struct arginfo * args) {

    Free(args->arg_infname);
    Free(args->arg_outfname);
    Free(args);
}
/***************************************************************/
struct hgenv * hgNewEnv(void)
{
    struct hgenv * hge;

    hge = New(struct hgenv, 1);
    if (!hge) return (NULL);

    hge->hge_dynl_info          = NULL;
    hge->hge_wsdlfile           = NULL;
    hge->hge_nerrs              = 0;
    hge->hge_errs               = NULL;
    hge->hge_done               = 0;
    hge->hge_frec_in            = NULL;
    hge->hge_frec_out           = NULL;
    hge->hge_wsg                = NULL;

    hge->hge_args               = arg_init();

    return (hge);
}
/***************************************************************/
void hgFreeEnv(struct hgenv * hge)
{
    if (hge->hge_dynl_info) dynl_dynloadfree(hge->hge_dynl_info);

    hg_clear_all_errors(hge);
    Free(hge->hge_wsdlfile);
    free_wsglob(hge->hge_wsg);

    arg_shut(hge->hge_args);

    Free(hge);
}
/***************************************************************/
void hgSetWsdlFile(struct hgenv * hge, const char * wsdlfile)
{
    if (hge->hge_wsdlfile) Free(hge->hge_wsdlfile);

    hge->hge_wsdlfile = Strdup(wsdlfile);
}
/***************************************************************/
char * hgGetWsdlFile(struct hgenv * hge)
{
    return (hge->hge_wsdlfile);
}
/***************************************************************/
struct frec {
    FILE * fref;
    unsigned char * fbuf;
    char * fname;
    int    fbufsize;
    int    fbuflen;
    int    flinenum;
    int    feof;
};
#define LINE_MAX_SIZE   1024
#define LINE_MAX_LEN    (LINE_MAX_SIZE - 1)

#define TOKE_MAX_SIZE   PATH_MAXIMUM
#define TOKE_MAX_LEN    (TOKE_MAX_SIZE - 1)
/***************************************************************/
void get_toke_quote (const char * bbuf, int * bbufix, char *toke, int tokemax) {
/*
** Parse quotation
*/
    char quote;
    int tokelen;
    int done;
    char ch;

    tokelen = 0;
    toke[0] = '\0';

    ch = bbuf[*bbufix];
    (*bbufix)++;
    quote = ch;
    toke[tokelen++] = ch;
    done = 0;

    while (!done) {
        ch = bbuf[*bbufix];

        if (!ch) {
            done = 1;
            if (tokelen + 1 < tokemax) toke[tokelen++] = '<';
            if (tokelen + 1 < tokemax) toke[tokelen++] = 'e';
            if (tokelen + 1 < tokemax) toke[tokelen++] = 'n';
            if (tokelen + 1 < tokemax) toke[tokelen++] = 'd';
            if (tokelen + 1 < tokemax) toke[tokelen++] = '>';
        } else if (ch == quote) {
            if (tokelen + 1 < tokemax) toke[tokelen++] = ch;
            (*bbufix)++;
            if (bbuf[*bbufix] == quote) (*bbufix) += 1;
            else done = 1;
        } else {
            if (tokelen + 1 < tokemax) toke[tokelen++] = ch;
            (*bbufix)++;
        }
    }
    toke[tokelen] = '\0';
}
/***************************************************************/
void gettoke (const char * bbuf, int * bbufix, char *toke, int tokemax) {
/*
** Reads next token from bbuf, starting from bbufix.
** upshifted.
**
** Tokens are alphanumeric and: '@', '.', '$', '_'
*/
    int tokelen;
    int done;
    char ch;

    toke[0] = '\0';

    while (bbuf[*bbufix] &&
           isspace(bbuf[*bbufix])) (*bbufix)++;
    if (!bbuf[*bbufix]) return;

    if (bbuf[*bbufix] == '\"') {
        get_toke_quote(bbuf, bbufix, toke, tokemax);
        return;
    }

    tokelen = 0;
    done = 0;

    while (!done) {
        ch = bbuf[*bbufix];

        if (isalpha(ch) || isdigit(ch)) {
            if (tokelen + 1 < tokemax) toke[tokelen++] = ch;
            (*bbufix)++;
        } else if (ch == '@'  ||
                   ch == '.'  ||
                   ch == '$'  ||
                   ch == '_') {
            if (tokelen + 1 < tokemax) toke[tokelen++] = ch;
            (*bbufix)++;
        } else {
            if (!tokelen) {
                if (tokelen + 1 < tokemax) toke[tokelen++] = ch;
                (*bbufix)++;
            }
            done = 1;
        }
    }
    toke[tokelen] = '\0';
}
/***************************************************************/
char * unquote_mem(char * tok) {

	char * out;
	int len;

	len = strlen(tok);
	if (len > 2 && tok[0] == '\"' && tok[len-1] == tok[0]) {
		out = New(char, len - 1);
		memcpy(out, tok + 1, len - 2);
		out[len - 2] = '\0';
	} else {
		out = New(char, len + 1);
		memcpy(out, tok, len + 1);
	}

	return (out);
}
/***************************************************************/
void unquote_inline(char * tok) {

	int len;

	len = strlen(tok);
	if (len > 2 && tok[0] == '\"' && tok[len-1] == tok[0]) {
		memmove(tok, tok + 1, len - 2);
		tok[len - 2] = '\0';
	}
}
/***************************************************************/
int frec_read_line(struct hgenv * hge,
                    char * line,
                    int linemax,
                    struct frec * fr) {

    int ii;
    int lix;
    int done;
    int hgstat;

    lix = 0;
    done = 0;

    while (!done) {
        if (!fgets(fr->fbuf, fr->fbufsize, fr->fref)) {
            int en = ERRNO;
            done = 1;
            if (ferror(fr->fref) || !feof(fr->fref)) {
                hgstat = hg_set_error(hge, HGE_EREADIN,
                            "Error #%d reading file: %s\n%s",
                                    en, fr->fname, strerror(en));
                lix = (-1);
            } else {
                fr->feof = 1;
                lix = (-1);
            }
        } else {
            fr->flinenum += 1;
            ii = strlen(fr->fbuf);
            while (ii > 0 && isspace(fr->fbuf[ii - 1])) ii--;
            if (linemax - lix <= ii) {
                hgstat = hg_set_error(hge, HGE_EREADIN,
                            "Buffer overflow reading file");
                lix = (-1);
                done = 1;
            } else {
                done = 1;
                memcpy(line + lix, fr->fbuf, ii);
                lix += ii;
                line[lix] = '\0';
            }
        }
    }

    return (lix);
}
/***************************************************************/
int frec_printf(struct hgenv * hge, struct frec * fr, const char *fmt, ...) {

    va_list args;
    char * msg;
    int hgstat = 0;

    va_start (args, fmt);
    msg = Vsmprintf (fmt, args);
    va_end (args);

    if (fputs(msg, fr->fref) == EOF) {
        int en = ERRNO;
        hgstat = hg_set_error(hge, HGE_EWRITEOUT,
                    "Error #%d writing to file: %s\n%s",
                            en, fr->fname, strerror(en));
    }
    Free(msg);

    return (hgstat);
}
/***************************************************************/
int frec_fflush(struct hgenv * hge, struct frec * fr) {

    int hgstat = 0;

    if (fflush(fr->fref) == EOF) {
        int en = ERRNO;
        hgstat = hg_set_error(hge, HGE_EWRITEOUT,
                    "Error #%d writing to file: %s\n%s",
                            en, fr->fname, strerror(en));
    }

    return (hgstat);
}
/***************************************************************/
void frec_free(struct frec * fr) {

    if (fr) {
        Free(fr->fname);
        Free(fr->fbuf);
        Free(fr);
     }
}
/***************************************************************/
int frec_close(struct frec * fr) {

    if (fr) {
        fclose(fr->fref);
        frec_free(fr);
     }

    return (0);
}
/***************************************************************/
int frec_is_eof(struct frec * fr) {

    return (fr->feof);
}
/***************************************************************/
struct frec * frec_dup(struct hgenv * hge,
                   FILE * openfref,
                   const char * fname,
                   const char * fmode) {

    FILE * fref;
    struct frec * fr;
    int hgstat;

    fref = FDOPEN(FILENO(openfref), fmode);
    if (!fref) {
        int ern = ERRNO;
        hgstat = hg_set_error(hge, HGE_EDUPOPEN,
                            "Error #%d duplicating: %s\n%s",
                            ern, fname, strerror(ern));
        fr = NULL;
    } else {
        fr = New(struct frec, 1);
        fr->fname = Strdup(fname);
        fr->fbufsize = LINE_MAX_SIZE + 2;
        fr->fbuf = New(char, fr->fbufsize);
        fr->flinenum = 0;
        fr->fbuflen = 0;
        fr->feof = 0;
        fr->fref = fref;
    }

    return (fr);
}
/***************************************************************/
struct frec * frec_open(struct hgenv * hge,
                   const char * fname,
                   const char * fmode) {

    FILE * fref;
    struct frec * fr;
    int hgstat;

    fref = fopen(fname, fmode);
    if (!fref) {
        int ern = ERRNO;
        hgstat = hg_set_error(hge, HGE_EFOPEN,
                            "Error #%d opening: %s\n%s",
                            ern, fname, strerror(ern));
        fr = NULL;
    } else {
        fr = New(struct frec, 1);
        fr->fname = Strdup(fname);
        fr->fbufsize = LINE_MAX_SIZE + 2;
        fr->fbuf = New(char, fr->fbufsize);
        fr->flinenum = 0;
        fr->fbuflen = 0;
        fr->feof = 0;
        fr->fref = fref;
    }

    return (fr);
}
/***************************************************************/
void * dynloadlibcurlproc(struct dynl_info * dynl,
                            char * libhome,
                            char * libpath,
                            char * libname,
                            char * procname,
                            int * err,
                            char* errmsg,
                            int errmsgmax) {
/*
** libhome should be getenv("LIBCURL_HOME")
** libname should be getenv("WHLIBCURLLIB")
**
This application has failed to start because libidn-11.dll was not found.
This application has failed to start because LIBEAY32.dll was not found.
This application has failed to start because SSLEAY32.dll was not found.
*/
    char lpath[256];
    char lname[256];
    void * plabel;

    dynl_dyntraceentry(dynl, "dynloadlibcurlproc",
                    0x0B, libhome, 0, libname, procname);

    calcpathlib(
        libhome         , libpath           , libname           ,
        "."             , LIBCURL_LIB_PATH  , LIBCURL_LIB_NAME   ,
        lpath           , lname);

    plabel = dynl_dynloadproc(dynl, lpath, lname, procname,
                    err, errmsg, sizeof(errmsg));

    return (plabel);
}
/***************************************************************/
static int hg_load_libcurl(struct hgenv * hge, struct dynl_info * dynl) {

    int dynerr;
    int hgerr;
    char * libcurlhome;
    char * libcurlpath;
    char * libcurllib;
    char errmsg[256];

    hgerr = 0;

    if (dynl->dcf) return (0);

    dynl->dcf = New(struct dynamic_curl, 1);
    dynl->dcf->libcurl_resolved = 0;

    libcurlhome = getenv("LIBCURL_HOME");
    libcurlpath = getenv("WHLIBCURLLIBPATH");
    libcurllib  = getenv("WHLIBCURLLIB");

    /* curl_easy_init */
    dynl->dcf->dcf_curl_easy_init =
        (p_curl_easy_init)dynloadlibcurlproc(dynl,
            libcurlhome, libcurlpath, libcurllib, "curl_easy_init",
            &dynerr, errmsg, sizeof(errmsg));
    if (!dynl->dcf->dcf_curl_easy_init) dynl->dcf->libcurl_resolved = -1;

    /* curl_easy_setopt */
    if (!dynl->dcf->libcurl_resolved) {
        dynl->dcf->dcf_curl_easy_setopt =
            (p_curl_easy_setopt)dynloadlibcurlproc(dynl,
                libcurlhome, libcurlpath, libcurllib, "curl_easy_setopt",
                &dynerr, errmsg, sizeof(errmsg));
        if (!dynl->dcf->dcf_curl_easy_setopt) dynl->dcf->libcurl_resolved = -1;
    }

    /* curl_easy_perform */
    if (!dynl->dcf->libcurl_resolved) {
        dynl->dcf->dcf_curl_easy_perform =
            (p_curl_easy_perform)dynloadlibcurlproc(dynl,
                libcurlhome, libcurlpath, libcurllib, "curl_easy_perform",
                &dynerr, errmsg, sizeof(errmsg));
        if (!dynl->dcf->dcf_curl_easy_perform) dynl->dcf->libcurl_resolved = -1;
    }

    /* curl_easy_cleanup */
    if (!dynl->dcf->libcurl_resolved) {
        dynl->dcf->dcf_curl_easy_cleanup =
            (p_curl_easy_cleanup)dynloadlibcurlproc(dynl,
                libcurlhome, libcurlpath, libcurllib, "curl_easy_cleanup",
                &dynerr, errmsg, sizeof(errmsg));
        if (!dynl->dcf->dcf_curl_easy_cleanup) dynl->dcf->libcurl_resolved = -1;
    }

    /* curl_easy_strerror */
    if (!dynl->dcf->libcurl_resolved) {
        dynl->dcf->dcf_curl_easy_strerror =
            (p_curl_easy_strerror)dynloadlibcurlproc(dynl,
                libcurlhome, libcurlpath, libcurllib, "curl_easy_strerror",
                &dynerr, errmsg, sizeof(errmsg));
        if (!dynl->dcf->dcf_curl_easy_strerror) dynl->dcf->libcurl_resolved = -1;
    }

    /* curl_global_init */
    if (!dynl->dcf->libcurl_resolved) {
        dynl->dcf->dcf_curl_global_init =
            (p_curl_global_init)dynloadlibcurlproc(dynl,
                libcurlhome, libcurlpath, libcurllib, "curl_global_init",
                &dynerr, errmsg, sizeof(errmsg));
        if (!dynl->dcf->dcf_curl_global_init) dynl->dcf->libcurl_resolved = -1;
    }

    /* curl_global_cleanup */
    if (!dynl->dcf->libcurl_resolved) {
        dynl->dcf->dcf_curl_global_cleanup =
            (p_curl_global_cleanup)dynloadlibcurlproc(dynl,
                libcurlhome, libcurlpath, libcurllib, "curl_global_cleanup",
                &dynerr, errmsg, sizeof(errmsg));
        if (!dynl->dcf->dcf_curl_global_cleanup) dynl->dcf->libcurl_resolved = -1;
    }

    /* curl_slist_append */
    if (!dynl->dcf->libcurl_resolved) {
        dynl->dcf->dcf_curl_slist_append =
            (p_curl_slist_append)dynloadlibcurlproc(dynl,
                libcurlhome, libcurlpath, libcurllib, "curl_slist_append",
                &dynerr, errmsg, sizeof(errmsg));
        if (!dynl->dcf->dcf_curl_slist_append) dynl->dcf->libcurl_resolved = -1;
    }

    /* curl_slist_free_all */
    if (!dynl->dcf->libcurl_resolved) {
        dynl->dcf->dcf_curl_slist_free_all =
            (p_curl_slist_free_all)dynloadlibcurlproc(dynl,
                libcurlhome, libcurlpath, libcurllib, "curl_slist_free_all",
                &dynerr, errmsg, sizeof(errmsg));
        if (!dynl->dcf->dcf_curl_slist_free_all) dynl->dcf->libcurl_resolved = -1;
    }

    if (!dynl->dcf->libcurl_resolved) {
        dynl->dcf->libcurl_resolved = 1;
    } else {
        hgerr = hg_set_error(hge, HGERRLOADJVM,
                        "Error %d loading runtime library: %s",
                        dynerr, errmsg);
    }

    return (hgerr);
}
/***************************************************************/
int hgShowError(struct hgenv * hge, FILE * outf, int eref) {

    return hg_show_error(hge, outf, eref);
}
/***************************************************************/
struct MemoryStruct {
  char *memory;
  size_t size;
};

static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;
  int bufsz;
  char buf[128];

  mem->memory = realloc(mem->memory, mem->size + realsize + 1);
  if(mem->memory == NULL) {
    /* out of memory! */
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }

    if (!mem->size && realsize) {
        bufsz = realsize;
        if (bufsz > 64) bufsz = 64;
        memcpy(buf, contents, bufsz);
        buf[bufsz] = '\0';
        fprintf(stdout, "1 len=%d %s\n", realsize, buf);
    }


  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

int getinmemory_example(struct dynl_info * dynl)
{
  CURL *curl_handle;
  CURLcode res;

  struct MemoryStruct chunk;

  chunk.memory = malloc(1);  /* will be grown as needed by the realloc above */
  chunk.size = 0;    /* no data at this point */

  dynl->dcf->dcf_curl_global_init(CURL_GLOBAL_ALL);

  /* init the curl session */
  curl_handle = dynl->dcf->dcf_curl_easy_init();

  /* specify URL to get */
  //f_curl_easy_setopt(curl_handle, CURLOPT_URL, "http://www.example.com/");
  dynl->dcf->dcf_curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, FALSE);

  //f_curl_easy_setopt(curl_handle, CURLOPT_URL, "https://www.google.com/");
  //f_curl_easy_setopt(curl_handle, CURLOPT_URL, "https://www.ssllabs.com/");

  dynl->dcf->dcf_curl_easy_setopt(curl_handle, CURLOPT_URL, 
        "https://api.channeladvisor.com/ChannelAdvisorAPI/v7/InventoryService.asmx?WSDL");

    /*  https Error: Peer certificate cannot be authenticated with given CA certificates */
    /*      Solution: curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, FALSE); */

  /* send all data to this function  */
  dynl->dcf->dcf_curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

  /* we pass our 'chunk' struct to the callback function */
  dynl->dcf->dcf_curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

  /* some servers don't like requests that are made without a user-agent
     field, so we provide one */
  dynl->dcf->dcf_curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

  /* get it! */
  res = dynl->dcf->dcf_curl_easy_perform(curl_handle);

  /* check for errors */
  if(res != CURLE_OK) {
    fprintf(stderr, "curl_easy_perform() failed: %s\n",
            dynl->dcf->dcf_curl_easy_strerror(res));
  }
  else {
    /*
     * Now, our chunk.memory points to a memory block that is chunk.size
     * bytes big and contains the remote file.
     *
     * Do something nice with it!
     */

    printf("%lu bytes retrieved\n", (long)chunk.size);
  }

  /* cleanup curl stuff */
  dynl->dcf->dcf_curl_easy_cleanup(curl_handle);

  free(chunk.memory);

  /* we're done with libcurl, so clean it up */
  dynl->dcf->dcf_curl_global_cleanup();

  return 0;
}
/*************************************************************/
int simple_example(struct dynl_info * dynl)
{
  CURL *curl;
  CURLcode res;
 
  curl = dynl->dcf->dcf_curl_easy_init();
  if(curl) {
    dynl->dcf->dcf_curl_easy_setopt(curl, CURLOPT_URL, "http://taurus.com");
    /* example.com is redirected, so we tell libcurl to follow redirection */ 
    dynl->dcf->dcf_curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
 
    /* Perform the request, res will get the return code */ 
    res = dynl->dcf->dcf_curl_easy_perform(curl);
    /* Check for errors */ 
    if(res != CURLE_OK)
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              dynl->dcf->dcf_curl_easy_strerror(res));
 
    /* always cleanup */ 
    dynl->dcf->dcf_curl_easy_cleanup(curl);
  }
  return 0;
}
/*************************************************************/
int hg_main(int (*main_proc)(void))
{
    struct hgenv * hge;
    int hgerr;

    hge = hgNewEnv();

    hge->hge_dynl_info = dynl_dynloadnew();
    hgerr = hg_load_libcurl(hge, hge->hge_dynl_info);
    if (!hgerr) {
        hgerr = main_proc();
    }
    if (hgerr) {
        hgShowError(hge, stdout, hgerr);
    }

    hgFreeEnv(hge);

    return (hgerr);
}
/*************************************************************/
int hg_read_wsdl_file(struct hgenv * hge, const char * wsdl_fname)
{
    int hgerr;

    hgerr = read_xml_wsdl_file(hge, wsdl_fname,
            WSDL_DEFINITIONS, hge->hge_wsg);

    return (hgerr);
}
/*************************************************************/
int hg_read_wsdl(struct hgenv * hge, const char * wsdl_fname)
{
    int hgerr;

    if (!hge->hge_dynl_info) hge->hge_dynl_info = dynl_dynloadnew();
    hgerr = hg_load_libcurl(hge, hge->hge_dynl_info);
    if (!hgerr) {
        hge->hge_wsg = new_wsglob();
        hgerr = hg_read_wsdl_file(hge, wsdl_fname);
    }

    return (hgerr);
}
/***************************************************************/
void showusage(char * prognam) {

    printf("%s - v%s\n", PROGRAM_NAME, VERSION);
    printf("\n");
    printf("Usage: %s [options] <config file name>\n", prognam);
    printf("\n");
    printf("Options:\n");
    printf("  -help             -?      Show extended help.\n");
    printf("  -in <fname>       -i      Read input from <fname>\n");
    printf("  -out <fname>      -o      Write output to <fname>\n");
    printf("  -pause            -p      Pause when done.\n");
    printf("\n");
    printf("Commands:\n");
    printf("  call <operation>(<parms>) Call WSDL operation\n");
    printf("  list                      List WSDL operations\n");
    printf("  wsdl <uri>                Use WSDL <uri>\n");
    printf("  exit                      Exit\n");
    printf("  #<any text>               Comment\n");
    printf("\n");
}
/***************************************************************/
void free_stringlist(char ** slist)
{
    int ii;
    if (slist) {
        for (ii = 0; slist[ii]; ii++) {
            Free(slist[ii]);
        }
        Free(slist);
    }
}
/***************************************************************/
/*@*/int istrcmp(char * b1, char * b2) {
/*
** Case insensitive strcmp
*/
    int ii;

    for (ii = 0; b1[ii]; ii++) {
             if (tolower(b1[ii]) < tolower(b2[ii])) return (-1);
        else if (tolower(b1[ii]) > tolower(b2[ii])) return (1);
    }
    if (b2[ii]) return (-1);
    else        return (0);
}
/***************************************************************/
int process_cmd_list(struct hgenv * hge,
                const char * line,
                int * linix,
                char * toke,
                int tokemax) {

    int hgstat = 0;
    int wstat = 0;
    int ii;
    char ** service_list;
    const char * emsg;

    wstat = wsdl_get_operations(hge->hge_wsg, WSB_BINDING_TYPE_SOAP);
    if (wstat) {
        emsg = ws_get_error_msg(hge->hge_wsg, wstat);
        hgstat = hg_set_error(hge, HGE_EWSDLERR,
            "WSDL error: %s", emsg);
        ws_clear_error(hge->hge_wsg, wstat);
    } else {
        service_list = wsdl_get_service_list(hge->hge_wsg);
        if (service_list) {
            for (ii = 0; service_list[ii]; ii++) {
                frec_printf(hge, hge->hge_frec_out, "    %s\n", service_list[ii]);
            }
            frec_fflush(hge, hge->hge_frec_out);
            /* free service_list */
            free_stringlist(service_list);
        }
    }

    if (!hgstat) gettoke(line, linix, toke, tokemax);

    return (hgstat);
}
/***************************************************************/
int process_cmd_call(struct hgenv * hge,
                const char * line,
                int * linix,
                char * toke,
                int tokemax) {

    int hgstat = 0;
    int wstat = 0;
    char paren[4];
    const char * emsg;

    wstat = wsdl_get_operations(hge->hge_wsg, WSB_BINDING_TYPE_SOAP);
    if (wstat) {
        emsg = ws_get_error_msg(hge->hge_wsg, wstat);
        hgstat = hg_set_error(hge, HGE_EWSDLERR,
            "WSDL error: %s", emsg);
        ws_clear_error(hge->hge_wsg, wstat);
    } else {
        gettoke(line, linix, paren, sizeof(paren));
        if (strcmp(paren, "(")) {
            hgstat = hg_set_error(hge, HGE_EPAREN,
                "CALL expecting open parenthesis. Found: %s", toke);
        } else {
            wstat = wsdl_call_operation(hge->hge_wsg, hge->hge_dynl_info,
                        toke, line, linix, stdout);
            if (wstat) {
                emsg = ws_get_error_msg(hge->hge_wsg, wstat);
                hgstat = hg_set_error(hge, HGE_EOPER,
                    "WSDL operation error: %s", emsg);
                ws_clear_error(hge->hge_wsg, wstat);
            } else {
                gettoke(line, linix, toke, tokemax);
                if (strcmp(toke, ")")) {
                    hgstat = hg_set_error(hge, HGE_EPAREN,
                        "CALL expecting close parenthesis. Found: %s", toke);
                }
            }
        }
    }

    if (!hgstat) gettoke(line, linix, toke, tokemax);

    return (hgstat);
}
/***************************************************************/
int process_cmd_wsdl(struct hgenv * hge,
                const char * line,
                int * linix,
                char * toke,
                int tokemax) {

    int hgstat = 0;
    struct proctimes pt_start;
    struct proctimes pt_end;
    struct proctimes pt;

    if (hge->hge_wsg) free_wsglob(hge->hge_wsg);

    unquote_inline(toke);
    start_cpu_timer(&pt_start, 0);
    hgstat = hg_read_wsdl(hge, toke);
    if (!hgstat) {
        stop_cpu_timer(&pt_start, &pt_end, &pt);
        gettoke(line, linix, toke, tokemax);
        printf("%d WSDL schemas processed in: %d.%03ds CPU seconds, %d.%03ds wall seconds\n",
                get_num_wsschema(hge->hge_wsg->wsg_wsd->wsd_wstypes),
                pt.pt_cpu_time.ts_second % 60,
                pt.pt_cpu_time.ts_microsec / 1000,
                pt.pt_wall_time.ts_second % 60,
                pt.pt_wall_time.ts_microsec / 1000);
    }

    return (hgstat);
}
/***************************************************************/
int process_cmd(struct hgenv * hge,
                const char * line,
                int * linix,
                char * toke,
                int tokemax) {

    int hgstat = 0;

    if (!Stricmp(toke, "exit")) {
        hge->hge_done = 1;
        gettoke(line, linix, toke, tokemax);

    } else if (!Stricmp(toke, "list")) {
        gettoke(line, linix, toke, tokemax);
        hgstat = process_cmd_list(hge, line, linix, toke, TOKE_MAX_LEN);

    } else if (!Stricmp(toke, "call")) {
        gettoke(line, linix, toke, tokemax);
        hgstat = process_cmd_call(hge, line, linix, toke, TOKE_MAX_LEN);

    } else if (!Stricmp(toke, "wsdl")) {
        gettoke(line, linix, toke, tokemax);
        hgstat = process_cmd_wsdl(hge, line, linix, toke, TOKE_MAX_LEN);

    } else {
        hgstat = hg_set_error(hge, HGE_BADCMD,
            "Unrecognized command. Found \"%s\"", toke);
    }

    return (hgstat);
}
/***************************************************************/
int process_line(struct hgenv * hge, const char * line) {

    int hgstat;
    int linix;
    char toke[TOKE_MAX_SIZE];

    hgstat = 0;
    linix = 0;
    while (line[linix] && isspace(line[linix])) linix++;

    if (line[linix] && line[linix] != '#') {
        gettoke(line, &linix, toke, TOKE_MAX_LEN);
        hgstat = process_cmd(hge, line, &linix, toke, TOKE_MAX_LEN);

        if (!hgstat) {
            if (toke[0]) {
                hgstat = hg_set_error(hge, HGE_EXPEOL,
                    "Expecting end of line. Found \"%s\"", toke);
            } else if (!hge->hge_done) {
                while (line[linix] && isspace(line[linix])) linix++;
                if (line[linix]) {
                    hgstat = hg_set_error(hge, HGE_EXPEOL,
                        "Expecting end of line. Found \"%s\"", &(line[linix]));
                }
            }
        }
    }

    return (hgstat);
}
/***************************************************************/
int process(struct hgenv * hge) {

    int hgstat = 0;
    int done;
    int llen;
    char line[LINE_MAX_SIZE];

    done = 0;
    if (hge->hge_args->arg_flags & ARG_FLAG_IN) {
        hge->hge_frec_in = frec_open(hge, hge->hge_args->arg_infname, "r");
    } else {
        hge->hge_frec_in = frec_dup(hge, stdin, "stdin", "r");
    }

    if (hge->hge_args->arg_flags & ARG_FLAG_OUT) {
        hge->hge_frec_out = frec_open(hge, hge->hge_args->arg_outfname, "w");
    } else {
        hge->hge_frec_out = frec_dup(hge, stdout, "stdout", "w");
    }

    if (!hge->hge_frec_in || !hge->hge_frec_out) {
        done = 1;
    }

    while (!done) {
        llen = frec_read_line(hge, line, LINE_MAX_LEN, hge->hge_frec_in);
        if (llen < 0) {
            done = 1;
        } else {
            hgstat = process_line(hge, line);
            if (hgstat) {
                done = 1;
            } else {
                done =  hge->hge_done;
            }
        }
    }

    if (!hgstat) hgstat = hg_get_error(hge);

    if (hgstat) {
        hg_show_all_errors(hge, stderr);
    }

    if (hge->hge_args->arg_flags & ARG_FLAG_IN) {
        frec_close(hge->hge_frec_in);
    } else {
        frec_free(hge->hge_frec_in);
    }
    if (hge->hge_args->arg_flags & ARG_FLAG_OUT) {
        frec_close(hge->hge_frec_out);
    } else {
        frec_free(hge->hge_frec_out);
    }

    return (hgstat);
}
/***************************************************************/
int getargs(struct hgenv * hge,
            int argc,
            char *argv[],
            struct arginfo * args) {

    int ii;
    int hgstat = 0;

    for (ii = 1; !hgstat && ii < argc; ii++) {
        if (argv[ii][0] == '-') {
            if (!Stricmp(argv[ii], "-p") || !Stricmp(argv[ii], "-pause")) {
                args->arg_flags |= ARG_FLAG_PAUSE;
            } else if (!Stricmp(argv[ii], "-?") || !Stricmp(argv[ii], "-help")) {
                args->arg_flags |= ARG_FLAG_HELP | ARG_FLAG_DONE;
            } else if (!Stricmp(argv[ii], "-version")) {
                args->arg_flags |= ARG_FLAG_VERSION | ARG_FLAG_DONE;
            } else if (!Stricmp(argv[ii], "-versnum")) {
                args->arg_flags |= ARG_FLAG_VERSION_NUM | ARG_FLAG_DONE;

            } else if (!Stricmp(argv[ii], "-i") || !Stricmp(argv[ii], "-in")) {
                ii++;
                if (ii < argc && argv[ii][0] != '-') {
                    args->arg_infname = Strdup(argv[ii]);
                    args->arg_flags |= ARG_FLAG_IN;
                }

            } else if (!Stricmp(argv[ii], "-o") || !Stricmp(argv[ii], "-out")) {
                ii++;
                if (ii < argc && argv[ii][0] != '-') {
                    args->arg_outfname = Strdup(argv[ii]);
                    args->arg_flags |= ARG_FLAG_OUT;
                }

            } else {
                hgstat = hg_set_error(hge, HGE_EBADOPT,
                    "Unrecognized option %s", argv[ii]);
            }
        }
    }

    return (hgstat);
}
/***************************************************************/
int main(int argc, char * argv[])
{
    int hgstat;
    int do_pause; 
    struct hgenv * hge;

    do_pause = 0;

    if (argc <= 1) {
        showusage(argv[0]);
        return (0);
    }

    hge = hgNewEnv();

    hgstat = getargs(hge, argc, argv, hge->hge_args);

    if (hge->hge_args->arg_flags & ARG_FLAG_HELP) {
        showusage(argv[0]);
    }

    if (hge->hge_args->arg_flags & ARG_FLAG_VERSION) {
        printf("%s - %s\n", PROGRAM_NAME, VERSION);
    } else if (hge->hge_args->arg_flags & ARG_FLAG_VERSION_NUM) {
        printf("%s%s\n", MAJOR_VERSION, MINOR_VERSION);
    }

    if (!hgstat && !(hge->hge_args->arg_flags & ARG_FLAG_DONE)) {
        hgstat = process(hge);
    }

    /* hgstat = hg_main(getinmemory_example); */
    /* hgstat = hg_read_wsdl(); */

    do_pause = hge->hge_args->arg_flags & ARG_FLAG_PAUSE;

    hgFreeEnv(hge);

    if (mem_not_freed() > 0) {
        print_mem_stats("CON");
    }

    if (do_pause) {
        char inbuf[100];
        printf("Press ENTER to continue...");
        fgets(inbuf, sizeof(inbuf), stdin);
    }

    return (0);
}
/***************************************************************/



