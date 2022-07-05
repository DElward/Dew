/***************************************************************/
/*
**  error.c --  JavaScript errors
*/
/***************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>
#include <math.h>

#include "dew.h"
#include "jsenv.h"
#include "error.h"
#include "snprfh.h"

void printerr(char * fmt, ...);

/***************************************************************/
#if 0
int set_jserror(enum errTypes errtyp, int estat, const char * fmt, ...)
{

	va_list args;
    char msgbuf[JSERRMSG_SIZE];

	va_start(args, fmt);
	Vsnprintf(msgbuf, sizeof(msgbuf), fmt, args);
	va_end(args);

    printerr("%s (JSERR %d)\n", msgbuf, estat);

	return (estat);
}
#endif
/***************************************************************/
void jrun_free_jerror(struct jerror * jerr)
{
    Free(jerr->jerr_msg);
    Free(jerr);
}
/***************************************************************/
void jrun_clear_all_errors(struct jrunexec * jx)
{
    int ii;

    ii = jx->jx_nerrs;
    while (ii > 0) {
        ii--;
        jrun_free_jerror(jx->jx_aerrs[ii]);
    }
    jx->jx_nerrs = 0;
}
/***************************************************************/
char * jerr_get_errtyp_name(enum errTypes errtyp)
{
/*
** 11/02/2021
*/
    char * out;

    switch (errtyp) {
        case errtyp_EvalError           : out = "EvalError";            break;
        case errtyp_RangeError          : out = "RangeError";           break;
        case errtyp_ReferenceError      : out = "ReferenceError";       break;
        case errtyp_SyntaxError         : out = "SyntaxError";          break;
        case errtyp_TypeError           : out = "TypeError";            break;
        case errtyp_URIError            : out = "URIError";             break;
        case errtyp_NativeError         : out = "NativeError";          break;
        case errtyp_UnimplementedError  : out = "UnimplementedError";   break;
        default                         : out = "*UndefinedError*";     break;
    }

    return (out);
}
/***************************************************************/
struct jerror * jrun_new_jerror(int jstat, char * errmsgptr)
{
    struct jerror * jerr;

    jerr = New(struct jerror, 1);
    jerr->jerr_stat = jstat;
    jerr->jerr_msg  = errmsgptr;

    return (jerr);
}
/***************************************************************/
static int jrun_add_error(struct jrunexec * jx,
    int jstat, char *errmsgptr) {

    int jerrix;
    struct jerror * jerr;

    jerr = jrun_new_jerror(jstat, errmsgptr);

    if (jx->jx_nerrs == MAXIMUM_JERRORS) {
        int ii;
        jrun_free_jerror(jx->jx_aerrs[0]);
        for (ii = 0; ii < MAXIMUM_JERRORS - 1; ii++) {
            jx->jx_aerrs[ii] = jx->jx_aerrs[ii + 1];
        }
        jx->jx_nerrs -= 1;
    } else {
        if (jx->jx_nerrs == jx->jx_xerrs) {
            if (!jx->jx_xerrs) jx->jx_xerrs = 1;
            else jx->jx_xerrs *= 2;
            jx->jx_aerrs = Realloc(jx->jx_aerrs, struct jerror * , jx->jx_xerrs);
        }
    }
    jx->jx_aerrs[jx->jx_nerrs] = jerr;
    jx->jx_nerrs += 1;

    jerrix = jx->jx_nerrs + JERROR_OFFSET;

    return (jerrix);
}
/***************************************************************/
int jrun_set_error(struct jrunexec * jx, enum errTypes errtyp, int jstat, char *fmt, ...) {

    va_list args;
    char *errmsgptr1;
    char *errmsgptr2;
    char *etypnam;
    int jerrix;

    va_start (args, fmt);
    errmsgptr1 = Vsmprintf (fmt, args);
    va_end (args);
    etypnam = jerr_get_errtyp_name(errtyp);
    errmsgptr2 = Smprintf("%s: %s (JSERR %d)", etypnam, errmsgptr1, jstat);
    Free(errmsgptr1);

    jerrix = jrun_add_error(jx, jstat, errmsgptr2);

    return (jerrix);
}
/***************************************************************/
char * jrun_get_errmsg(struct jrunexec * jx,
    int emflags,
    int jstat,
    int *jerrnum)
{
    struct jerror * jerr;
    char *jerrmsg;

    if (!jx->jx_nerrs) {
        jerrmsg = NULL;
    } else {
        jerr = jx->jx_aerrs[jx->jx_nerrs - 1];
        jerrmsg = Strdup(jerr->jerr_msg);
    }

    return (jerrmsg);
}
/***************************************************************/
