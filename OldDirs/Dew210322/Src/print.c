/***************************************************************/
/*
**  print.c --  Debug/print functions
*/
/***************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>

#include "dew.h"
#include "jsenv.h"
#include "jstok.h"
#include "jsexp.h"
#include "dbtreeh.h"
#include "snprfh.h"
#include "utl.h"
#include "var.h"
#include "print.h"

#define TABWIDTH 4

#define JPR_FLAG_SHOW_VARS          1
#define JPR_FLAG_SHOW_CLASSES       2
#define JPR_FLAG_SHOW_FUNCTIONS     4
#define JPR_FLAG_SHOW_LVALUE        8
#define JPR_FLAG_SHOW_OBJECTS       16
#define JPR_FLAGS_ALL_TYPES         0xff
#define JPR_FLAG_SHOW_TYPE          0x100

/***************************************************************/
void append_printf(char ** cstr,
                    int * slen,
                    int * smax,
                    const char * fmt,
                    ...)
{
/*
** 03/11/2021
*/
    va_list args;
    char * sbuf;

    va_start(args, fmt);
    sbuf = Vsmprintf(fmt, args);
    va_end(args);

    append_charval(cstr, slen, smax, sbuf);
    Free(sbuf);
}
/***************************************************************/
static void jpr_goto_tab(
    char ** prbuf,
    int * prlen,
    int * prmax,
    int tabno,
    int tabwidth)
{
/*
** 03/11/2021
*/
    int col;

    col = tabno * tabwidth;
    if (col < (*prlen)) {
        col = ((*prlen) + tabwidth) / tabwidth;
        col *= tabwidth;
    }
    if ((*prmax) <= col) {
        (*prmax) = col + 64;
        (*prbuf) = Realloc((*prbuf), char, (*prmax));
    }
    memset((*prbuf) + (*prlen), ' ', col - (*prlen));
    (*prlen) = col;
    (*prbuf)[col] = '\0';
}
/***************************************************************/
void jpr_jvarvalue_tostring(
    char ** prbuf,
    int * prlen,
    int * prmax,
    struct jvarvalue * jvv,
    int vflags)
{
/*
** 03/11/2021
*/
    switch (jvv->jvv_dtype) {
        case JVV_DTYPE_NONE   :
            if (vflags & JPR_FLAG_SHOW_TYPE)     append_charval(prbuf, prlen, prmax, "NONE: ");
            break;
        case JVV_DTYPE_BOOL   :
            if (vflags & JPR_FLAG_SHOW_VARS) {
                if (vflags & JPR_FLAG_SHOW_TYPE) append_charval(prbuf, prlen, prmax, "BOOL: ");
                if (jvv->jvv_val.jvv_val_bool==0) append_charval(prbuf, prlen, prmax, "false");
                else append_charval(prbuf, prlen, prmax, "true");
            }
            break;
        case JVV_DTYPE_JSINT   :
            if (vflags & JPR_FLAG_SHOW_VARS) {
                if (vflags & JPR_FLAG_SHOW_TYPE) append_charval(prbuf, prlen, prmax, "INT: ");
                append_printf(prbuf, prlen, prmax, "%d", jvv->jvv_val.jvv_val_jsint);
            }
            break;
        case JVV_DTYPE_JSFLOAT   :
            if (vflags & JPR_FLAG_SHOW_VARS) {
                if (vflags & JPR_FLAG_SHOW_TYPE) append_charval(prbuf, prlen, prmax, "FLOAT: ");
                append_printf(prbuf, prlen, prmax, "%g", jvv->jvv_val.jvv_val_jsfloat);
            }
            break;
        case JVV_DTYPE_CHARS   :
            if (vflags & JPR_FLAG_SHOW_VARS) {
                if (vflags & JPR_FLAG_SHOW_TYPE) append_charval(prbuf, prlen, prmax, "CHARS: ");
                append_printf(prbuf, prlen, prmax, "\"%s\"", jvv->jvv_val.jvv_val_chars.jvvc_val_chars);
            }
            break;
        case JVV_DTYPE_LVAL:
            if (vflags & JPR_FLAG_SHOW_TYPE) append_charval(prbuf, prlen, prmax, "LVAL: ");
            if (vflags & JPR_FLAG_SHOW_LVALUE) {
                jpr_jvarvalue_tostring(prbuf, prlen, prmax, jvv->jvv_val.jvv_lval, vflags);
            }
            break;
        case JVV_DTYPE_INTERNAL_CLASS:
            if (vflags & JPR_FLAG_SHOW_TYPE) append_charval(prbuf, prlen, prmax, "CLASS: ");
            if (vflags & JPR_FLAG_SHOW_CLASSES) {
                append_charval(prbuf, prlen, prmax, "class value");
            }
            break;
        case JVV_DTYPE_INTERNAL_METHOD:
            if (vflags & JPR_FLAG_SHOW_TYPE) append_charval(prbuf, prlen, prmax, "METHOD: ");
            if (vflags & JPR_FLAG_SHOW_FUNCTIONS) {
                append_charval(prbuf, prlen, prmax, "method value");
            }
            break;
        case JVV_DTYPE_OBJECT:
            if (vflags & JPR_FLAG_SHOW_TYPE) append_charval(prbuf, prlen, prmax, "OBJECT: ");
            if (vflags & JPR_FLAG_SHOW_OBJECTS) {
                append_charval(prbuf, prlen, prmax, "object value");
            }
            break;
        case JVV_DTYPE_VALLIST:
            if (vflags & JPR_FLAG_SHOW_TYPE) append_charval(prbuf, prlen, prmax, "VALLIST: ");
            if (vflags & JPR_FLAG_SHOW_OBJECTS) {
                append_charval(prbuf, prlen, prmax, "value list");
            }
            break;
        case JVV_DTYPE_TOKEN:
            if (vflags & JPR_FLAG_SHOW_TYPE) append_charval(prbuf, prlen, prmax, "TOKEN: ");
            if (vflags & JPR_FLAG_SHOW_OBJECTS) {
                append_printf(prbuf, prlen, prmax, "\"%s\"", jvv->jvv_val.jvv_val_token.jvvt_token);
            }
            break;
        case JVV_DTYPE_FUNCTION:
            if (vflags & JPR_FLAG_SHOW_TYPE) append_charval(prbuf, prlen, prmax, "FUNCTION: ");
            if (vflags & JPR_FLAG_SHOW_FUNCTIONS) {
                append_printf(prbuf, prlen, prmax, "\"%s\"", jvv->jvv_val.jvv_val_function->jvvf_func_name);
            }
            break;
        default:
            append_printf(prbuf, prlen, prmax, "*Unknown type=%d*", jvv->jvv_dtype);
            break;
    }
}
/***************************************************************/
static int jpr_vix_is_const(int vix, struct jvarrec * jvar)
{
/*
** 03/12/2021
*/
    int is_const = 0;
    int ii;

    for (ii = 0; !is_const && ii < jvar->jvar_nconsts; ii++) {
        if (vix == jvar->jvar_avix[ii]) is_const = 1;
    }

    return (is_const);
}
/***************************************************************/
static int jpr_debug_show_jcontext(
                    struct jrunexec * jx,
                    struct jcontext * jcx,
                    int tab,
                    int vflags)
{
/*
** 03/11/2021
*/
    int jstat = 0;
    void * vcs;
    int * pvix;
    int klen;
    char * kval;
    char * prbuf = NULL;
    int prlen = 0;
    int prmax = 0;

    vcs = dbtsi_rewind(jcx->jcx_jvar->jvar_jvarvalue_dbt, DBTREE_READ_FORWARD);
    do {
        pvix = dbtsi_next(vcs);
        if (pvix) {
            if ((*pvix) < 0 || (*pvix) >= jcx->jcx_njvv) {
                printf("**** variable index %d is out of range[0-%d]",
                    (*pvix), jcx->jcx_njvv);
            } else {
                jpr_goto_tab(&prbuf, &prlen, &prmax, tab, TABWIDTH);
                kval = dbtsi_get_key(vcs, &klen);
                append_printf(&prbuf, &prlen, &prmax, "%2d %-16s - ", (*pvix), kval);
                if (jpr_vix_is_const(*pvix, jcx->jcx_jvar))
                    append_charval(&prbuf, &prlen, &prmax, "const: " );
                else
                    append_charval(&prbuf, &prlen, &prmax, "     : ");
                jpr_jvarvalue_tostring(&prbuf, &prlen, &prmax, jcx->jcx_ajvv[*pvix], vflags);
                printf("%s\n", prbuf);
                prlen = 0;
            }
        }
    } while (pvix);
    dbtsi_close(vcs);

    Free(prbuf);

    return (jstat);
}
/***************************************************************/
int jpr_exec_debug_show(
                    struct jrunexec * jx,
                    char * dbgstr,
                    int * ix)
{
/*
** 02/22/2021
*/
    int jstat = 0;
    struct jfuncstate * jxfs;
    struct jcontext * jcx = NULL;
    struct jvarvalue * fjvv;
    char toke[TOKSZ];

    get_toke_char(dbgstr, ix, toke, TOKSZ, GETTOKE_FLAGS_DEFAULT);
    if (!strcmp(toke, "allvars")) {
        printf("All variables:\n");
        jxfs = jvar_get_current_jfuncstate(jx);
        if (!jxfs) {
            printf("Not in a function.\n");
            jcx = jvar_get_global_jcontext(jx);
            jstat = jpr_debug_show_jcontext(jx, jcx, 1, JPR_FLAGS_ALL_TYPES | JPR_FLAG_SHOW_TYPE);
        } else {
            jcx = jxfs->jfs_jcx;
            fjvv = jxfs->jfs_fjvv;
            if (fjvv->jvv_dtype == JVV_DTYPE_INTERNAL_METHOD) {
                printf("Internal method: %s\n", fjvv->jvv_val.jvv_int_method.jvvim_method_name);
            } else if (fjvv->jvv_dtype == JVV_DTYPE_FUNCTION) {
                printf("Function: %s\n", fjvv->jvv_val.jvv_val_function->jvvf_func_name);
            } else {
                printf("Invalid function type: %d\n", fjvv->jvv_dtype);
            }

            while (!jstat && jcx) {
                jstat = jpr_debug_show_jcontext(jx, jcx, 1, JPR_FLAGS_ALL_TYPES | JPR_FLAG_SHOW_TYPE);
                jcx = jcx->jcx_outer_jcx;
                if (jcx) printf("\n");
            }
        }
    }

    return (jstat);
}
/***************************************************************/
