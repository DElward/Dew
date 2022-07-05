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
#include "jsexp.h"
#include "error.h"
#include "snprfh.h"
#include "internal.h"

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
    if (jerr) {
        Free(jerr->jerr_msg);
        Free(jerr);
    }
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
static struct jerror * jrun_get_and_clear_error(struct jrunexec * jx, int injstat)
{
/*
** 11/08/2021
*/
    int jstat = 0;;
    int jerrix;
    struct jerror * jerr = NULL;

    jerrix = injstat - JERROR_OFFSET - 1;
    if (jerrix >= 0 && jerrix < jx->jx_nerrs) {
        jerr = jx->jx_aerrs[jerrix];
        jx->jx_aerrs[jerrix] = NULL;
        while (jx->jx_nerrs > 0 && jx->jx_aerrs[jx->jx_nerrs - 1] == NULL) jx->jx_nerrs -= 1;
    }

    return (jerr);
}
/***************************************************************/
int jrun_clear_error(struct jrunexec * jx, int injstat)
{
/*
** 11/08/2021
*/
    int jstat = 0;;
    struct jerror  * jerr;

    jerr = jrun_get_and_clear_error(jx, injstat);
    jrun_free_jerror(jerr);

    return (jstat);
}
/***************************************************************/
char * jerr_get_errtyp_name(enum errTypes errtyp)
{
/*
** 11/02/2021
*/
    char * out;

    switch (errtyp) {
        case errtyp_Error               : out = "Error";                break;
        case errtyp_EvalError           : out = "EvalError";            break;
        case errtyp_InternalError       : out = "InternalError";        break;
        case errtyp_NativeError         : out = "NativeError";          break;
        case errtyp_RangeError          : out = "RangeError";           break;
        case errtyp_ReferenceError      : out = "ReferenceError";       break;
        case errtyp_SyntaxError         : out = "SyntaxError";          break;
        case errtyp_TypeError           : out = "TypeError";            break;
        case errtyp_URIError            : out = "URIError";             break;
        case errtyp_UnimplementedError  : out = "UnimplementedError";   break;
        default                         : out = "*UndefinedError*";     break;
    }

    return (out);
}
/***************************************************************/
struct jerror * jrun_new_jerror(enum errTypes errtyp, int jstat, char * errmsgptr)
{
    struct jerror * jerr;

    jerr = New(struct jerror, 1);
    jerr->jerr_stat     = jstat;
    jerr->jerr_errtyp   = errtyp;
    jerr->jerr_msg      = errmsgptr;

    return (jerr);
}
/***************************************************************/
static int jrun_add_error(struct jrunexec * jx,
    enum errTypes errtyp,
    int jstat,
    char *errmsgptr)
{
    int jerrix;
    struct jerror * jerr;

    jerr = jrun_new_jerror(errtyp, jstat, errmsgptr);

    if (jx->jx_nerrs == MAXIMUM_JERRORS) {
#if IS_DEBUG
    printf("**DEBUG: Maximum number of errors (%d) reached.\n", MAXIMUM_JERRORS);
    *((char*)0) = 0;  /* Cause blowout */
#endif
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
    if (errtyp == errtyp_Error) {
        errmsgptr2 = Smprintf("%s", errmsgptr1);
    } else {
        etypnam = jerr_get_errtyp_name(errtyp);
        errmsgptr2 = Smprintf("%s: %s (JSERR %d)", etypnam, errmsgptr1, jstat);
    }
    Free(errmsgptr1);

    jerrix = jrun_add_error(jx, errtyp, jstat, errmsgptr2);

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
void jint_init_jintobj_Error(struct jintobj_Error * jioe,
    enum errTypes           errtyp,
    const char            * errmsg)
{
/*
** 11/11/2021
*/
#if IS_DEBUG
    static int next_jioe_sn = 0;
#endif

#if IS_DEBUG
    jioe->jioe_sn[0] = 'E';
    jioe->jioe_sn[1] = (next_jioe_sn / 100) % 10 + '0';
    jioe->jioe_sn[2] = (next_jioe_sn /  10) % 10 + '0';
    jioe->jioe_sn[3] = (next_jioe_sn      ) % 10 + '0';
    next_jioe_sn++;
#endif

    jioe->jioe_errtyp = errtyp;
    jioe->jioe_msglen = IStrlen(errmsg);
    jioe->jioe_msg    = New(char, jioe->jioe_msglen + 1);
    memcpy(jioe->jioe_msg, errmsg, jioe->jioe_msglen + 1);
}
/***************************************************************/
int jint_Error_toString(
        struct jrunexec  * jx,
        const char       * func_name,
        void             * this_obj,
        struct jvarvalue * jvvlargs,
        struct jvarvalue * jvvrtn)
{
/*
** 11/10/2021
*/
    int jstat = 0;
    struct jintobj_Error * jioe = (struct jintobj_Error *)this_obj;

    jvar_store_chars_len(jvvrtn, jioe->jioe_msg, jioe->jioe_msglen);

    return (jstat);
}
/***************************************************************/
int jrun_create_Error(struct jrunexec * jx,
    enum errTypes           errtyp,
    const char            * errmsg,
    struct jvarvalue * ejvv)
{
/*
** 11/11/2021
struct jintobj_AggregateError { ** jioe_ae **
    struct jintobj_Error        jioe_ae_err;
};

struct jintobj_InternalError { ** jioe_ie **
    struct jintobj_Error        jioe_ie_err;
};

struct jintobj_NativeError { ** jioe_ne **
    struct jintobj_Error        jioe_ne_err;
};

struct jintobj_RangeError { ** jioe_ge **
    struct jintobj_Error        jioe_ge_err;
};

struct jintobj_ReferenceError { ** jioe_re **
    struct jintobj_Error        jioe_re_err;
};

struct jintobj_SyntaxError { ** jioe_se **
    struct jintobj_Error        jioe_se_err;
};

struct jintobj_TypeError { ** jioe_te **
    struct jintobj_Error        jioe_te_err;
};

struct jintobj_URIError { ** jioe_ue **
    struct jintobj_Error        jioe_ue_err;
};

struct jintobj_UnimplementedError { ** jioe_xe **
    struct jintobj_Error        jioe_xe_err;
};

*/
    int jstat = 0;
    void * jioe;
    char * etypnam;

    switch (errtyp) {
        case errtyp_EvalError           : jioe = New(struct jintobj_AggregateError, 1);     break;
        case errtyp_InternalError       : jioe = New(struct jintobj_InternalError, 1);      break;
        case errtyp_NativeError         : jioe = New(struct jintobj_NativeError, 1);        break;
        case errtyp_RangeError          : jioe = New(struct jintobj_RangeError, 1);         break;
        case errtyp_ReferenceError      : jioe = New(struct jintobj_ReferenceError, 1);     break;
        case errtyp_SyntaxError         : jioe = New(struct jintobj_SyntaxError, 1);        break;
        case errtyp_TypeError           : jioe = New(struct jintobj_TypeError, 1);          break;
        case errtyp_URIError            : jioe = New(struct jintobj_NativeError, 1);        break;
        case errtyp_UnimplementedError  : jioe = New(struct jintobj_UnimplementedError, 1); break;
        default                         : jioe = New(struct jintobj_UnimplementedError, 1); break;
    }

    etypnam = jerr_get_errtyp_name(errtyp);

    jint_init_jintobj_Error(jioe, errtyp, errmsg);

    ejvv->jvv_dtype = JVV_DTYPE_INTERNAL_OBJECT;
    ejvv->jvv_val.jvv_jvvo = jvar_new_jvarvalue_int_object_classname(jx, etypnam, jioe);

    return (jstat);
}
/***************************************************************/
int jrun_convert_error_to_jvv_error(struct jrunexec * jx,
    int injstat,
    struct jvarvalue * ejvv)
{
/*
** 11/10/2021
*/
    int jstat = 0;;
    struct jerror  * jerr;

    jerr = jrun_get_and_clear_error(jx, injstat);

    if (jerr) {
        jstat = jrun_create_Error(jx, jerr->jerr_errtyp, jerr->jerr_msg, ejvv);
    } else {
        jstat = jrun_create_Error(jx, errtyp_InternalError, "Invalid error reference", ejvv);
    }

    /* switch (jerr->jerr_errtyp) {                 */
    /*     case errtyp_EvalError           :        */
    /*         break;                               */
    /*     case errtyp_InternalError       :        */
    /*         break;                               */
    /*     case errtyp_RangeError          :        */
    /*         break;                               */
    /*     case errtyp_ReferenceError      :        */
    /*         break;                               */
    /*     case errtyp_SyntaxError         :        */
    /*         break;                               */
    /*     case errtyp_TypeError           :        */
    /*         break;                               */
    /*     case errtyp_URIError            :        */
    /*         break;                               */
    /*     case errtyp_NativeError         :        */
    /*         break;                               */
    /*     case errtyp_UnimplementedError  :        */
    /*         break;                               */
    /*     default                         :        */
    /*         break;                               */
    /* }                                            */

    jrun_free_jerror(jerr);

    return (jstat);
}
/***************************************************************/
