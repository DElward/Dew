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
#include "error.h"
#include "jstok.h"
#include "jsexp.h"
#include "dbtreeh.h"
#include "snprfh.h"
#include "utl.h"
#include "var.h"
#include "print.h"

#define TABWIDTH 4

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
int jpr_jvarvalue_array_tostring(
    struct jrunexec * jx,
    char ** prbuf,
    int * prlen,
    int * prmax,
    struct jvarvalue_array * jvva,
    int vflags)
{
/*
** 10/19/2021
*/
    int jstat = 0;
    char * prbuf2;
    int prlen2;
    int prmax2;
    int ii;
    int nprint;

    if (vflags & JPR_FLAG_LOG) {
        append_printf(prbuf, prlen, prmax, "(%d) ", jvva->jvva_nvals);
    }

    nprint = 0;
    append_charval(prbuf, prlen, prmax, "[");
    ii = 0;
    while (ii < jvva->jvva_nvals) {
        if (nprint) append_charval(prbuf, prlen, prmax, ", ");
        if ((vflags & JPR_FLAG_LOG) && jvva->jvva_avals[ii].jvv_dtype == JVV_DTYPE_NONE) {
            append_charval(prbuf, prlen, prmax, "…");
            do {
                ii++;
            } while (ii < jvva->jvva_nvals && jvva->jvva_avals[ii].jvv_dtype == JVV_DTYPE_NONE);
        } else {
            prbuf2 = NULL;
            prlen2 = 0;
            prmax2 = 0;
            jstat = jpr_jvarvalue_tostring(jx, &prbuf2, &prlen2, &prmax2,
                &(jvva->jvva_avals[ii]), vflags | JPR_FLAGS_SHOW_QUOTES);
            if (!jstat) {
                append_charval(prbuf, prlen, prmax, prbuf2);
                Free(prbuf2);
            }
            nprint++;
            ii++;
        }
    }
    append_charval(prbuf, prlen, prmax, "]");

    return (jstat);
}
/***************************************************************/
int jpr_jvarvalue_tostring(
    struct jrunexec * jx,
    char ** prbuf,
    int * prlen,
    int * prmax,
    struct jvarvalue * jvv,
    int vflags)
{
/*
** 03/11/2021 - 09/27/21 All to strings go through here
*/
    int jstat = 0;
    struct jvarvalue * jvvtmp;

    if (!jvv) {
        append_charval(prbuf, prlen, prmax, "*null var*");
        return (0);
    }

    switch (jvv->jvv_dtype) {
        case JVV_DTYPE_NONE   :
            if (vflags & JPR_FLAG_SHOW_TYPE)     append_charval(prbuf, prlen, prmax, "NONE: ");
            if (vflags & JPR_FLAG_SHOW_VARS) {
                append_charval(prbuf, prlen, prmax, "undefined");
            }
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
                if (vflags & JPR_FLAGS_SHOW_QUOTES) {
                    if (vflags & JPR_FLAG_LOG) {
                        append_printf(prbuf, prlen, prmax, "\'%s\'", jvv->jvv_val.jvv_val_chars.jvvc_val_chars);
                    } else {
                        append_printf(prbuf, prlen, prmax, "\"%s\"", jvv->jvv_val.jvv_val_chars.jvvc_val_chars);
                    }
                } else {
                    append_printf(prbuf, prlen, prmax, "%s", jvv->jvv_val.jvv_val_chars.jvvc_val_chars);
                }
            }
            break;

        case JVV_DTYPE_LVAL:
            if (vflags & JPR_FLAG_SHOW_TYPE) append_charval(prbuf, prlen, prmax, "LVAL: ");
            if (vflags & JPR_FLAG_SHOW_LVALUE) {
                jstat = jexp_eval_rval(jx, jvv, &jvvtmp);
                if (!jstat) {
                    jstat = jpr_jvarvalue_tostring(jx, prbuf, prlen, prmax, jvvtmp, vflags);
                }
            }
            break;

        case JVV_DTYPE_INTERNAL_CLASS:
            if (vflags & JPR_FLAG_SHOW_TYPE) append_charval(prbuf, prlen, prmax, "INT CLASS: ");
            if (vflags & JPR_FLAG_SHOW_CLASSES) {
                append_charval(prbuf, prlen, prmax, "class value");
            }
            break;

        case JVV_DTYPE_INTERNAL_METHOD:
            if (vflags & JPR_FLAG_SHOW_TYPE) append_charval(prbuf, prlen, prmax, "INT METHOD: ");
            if (vflags & JPR_FLAG_SHOW_FUNCTIONS) {
                append_charval(prbuf, prlen, prmax, "method value");
            }
            break;

        case JVV_DTYPE_INTERNAL_OBJECT:
            if (vflags & JPR_FLAG_SHOW_TYPE) append_charval(prbuf, prlen, prmax, "INT OBJ: ");
            if (vflags & JPR_FLAG_SHOW_OBJECTS) {
                append_charval(prbuf, prlen, prmax, "int obj value");
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
            if (vflags & JPR_FLAG_SHOW_TYPE) {
#if IS_DEBUG
                append_charval(prbuf, prlen, prmax, "FUNCTION[");
                append_chars(prbuf, prlen, prmax, jvv->jvv_val.jvv_val_function->jvvf_sn, 4);
                append_charval(prbuf, prlen, prmax, "]: ");
#else
                append_charval(prbuf, prlen, prmax, "FUNCTION: ");
#endif
            }
            if (vflags & JPR_FLAG_SHOW_FUNCTIONS) {
                append_printf(prbuf, prlen, prmax, "\"%s\"", jvv->jvv_val.jvv_val_function->jvvf_func_name);
            }
            break;

        case JVV_DTYPE_FUNCVAR   :
            if (vflags & JPR_FLAG_SHOW_TYPE) append_charval(prbuf, prlen, prmax, "FUNCVAR: ");
            if (vflags & JPR_FLAG_SHOW_LVALUE) {
                append_printf(prbuf, prlen, prmax, "f %s",
                    jvv->jvv_val.jvv_val_funcvar.jvvfv_jvvf->jvvf_func_name);
            }
            break;

            case JVV_DTYPE_IMETHVAR   :
            if (vflags & JPR_FLAG_SHOW_TYPE) append_charval(prbuf, prlen, prmax, "IMETHVAR: ");
            if (vflags & JPR_FLAG_SHOW_LVALUE) {
                append_printf(prbuf, prlen, prmax, "f %s",
                    jvv->jvv_val.jvv_val_imethvar.jvvimv_jvvim->jvvim_method_name);
            }
            break;
        
        case JVV_DTYPE_ARRAY:
            if (vflags & JPR_FLAG_SHOW_TYPE) append_printf(prbuf, prlen, prmax, "ARRAY(%d): ",
                jvv->jvv_val.jvv_val_array->jvva_nRefs);
            jstat = jpr_jvarvalue_array_tostring(jx, prbuf, prlen, prmax, jvv->jvv_val.jvv_val_array, vflags);
            break;
        
        case JVV_DTYPE_OBJPTR:
            if (vflags & JPR_FLAG_SHOW_TYPE) append_charval(prbuf, prlen, prmax, "OBJPTR: ");
            switch (jvv->jvv_val.jvv_objptr.jvvp_dtype) {
                case JVV_DTYPE_ARRAY   :
                    if (vflags & JPR_FLAG_SHOW_TYPE) append_printf(prbuf, prlen, prmax, "ARRAY(%d): ",
                        jvv->jvv_val.jvv_objptr.jvvp_val.jvvp_val_array->jvva_nRefs);
                    jstat = jpr_jvarvalue_array_tostring(jx, prbuf, prlen, prmax,
                        jvv->jvv_val.jvv_objptr.jvvp_val.jvvp_val_array, vflags);
                    break;
                default:
                    append_printf(prbuf, prlen, prmax, "*Unknown object pointer type=%d*",
                        jvv->jvv_val.jvv_objptr.jvvp_dtype);
                    break;
            }
            break;

        case JVV_DTYPE_DYNAMIC:
            if (vflags & JPR_FLAG_SHOW_TYPE) append_charval(prbuf, prlen, prmax, "DYNAMIC: ");
            if (vflags & JPR_FLAG_SHOW_LVALUE) {
                jstat = jexp_eval_rval(jx, jvv, &jvvtmp);
                if (!jstat) {
                    jstat = jpr_jvarvalue_tostring(jx, prbuf, prlen, prmax, jvvtmp, vflags);
                }
            }
            break;

        default:
            append_printf(prbuf, prlen, prmax, "*Unknown type=%s*", jvar_get_dtype_name(jvv->jvv_dtype));
            break;
    }

    return (jstat);
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
#if 0
void jpr_jvvflags_char(char * jvvfbuf, int jvvfbufsz, int jvvflags)
{
/*
** 09/23/2021
*/
    jvvfbuf[0] = '\0';
    if (jvvflags & JVV_FLAG_CONST) {
        if (jvvfbuf[0]) strcat(jvvfbuf, ",");
        strcat(jvvfbuf, "CONST");
    }
}
#endif
/***************************************************************/
void jpr_jcxflags_char(char * jvvfbuf, int jvvfbufsz, int jcxflags)
{
/*
** 10/06/2021
*/
    jvvfbuf[0] = '\0';
    if (!jcxflags) {
        if (jvvfbuf[0]) strcat(jvvfbuf, ",");
        strcat(jvvfbuf, "None");
    }
    if (jcxflags & JCX_FLAG_FUNCTION) {
        if (jvvfbuf[0]) strcat(jvvfbuf, ",");
        strcat(jvvfbuf, "Func");
    }
    if (jcxflags & JCX_FLAG_PROPERTY) {
        if (jvvfbuf[0]) strcat(jvvfbuf, ",");
        strcat(jvvfbuf, "Prop");
    }
    if (jcxflags & JCX_FLAG_CONST) {
        if (jvvfbuf[0]) strcat(jvvfbuf, ",");
        strcat(jvvfbuf, "Const");
    }
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
    char jvvfbuf[64];

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
#if IS_DEBUG
                append_chars(&prbuf, &prlen, &prmax, jcx->jcx_sn, 4);
                append_printf(&prbuf, &prlen, &prmax, " %2d ", (*pvix));
#else
                append_printf(&prbuf, &prlen, &prmax, "%2d ", (*pvix));
#endif
#if IS_DEBUG
                append_chars(&prbuf, &prlen, &prmax, jcx->jcx_sn, 4);
                append_chars(&prbuf, &prlen, &prmax, jcx->jcx_jvar->jvar_sn, 4);
                append_chars(&prbuf, &prlen, &prmax, " ", 1);
                if (jcx->jcx_ajvv[*pvix].jvv_sn[0])
                    append_chars(&prbuf, &prlen, &prmax, jcx->jcx_ajvv[*pvix].jvv_sn, 4);
                else
                    append_chars(&prbuf, &prlen, &prmax, "nosn", 4);
#endif
                /* jpr_jvvflags_char(jvvfbuf, sizeof(jvvfbuf), jcx->jcx_ajvv[*pvix]->jvv_flags); */
                jpr_jcxflags_char(jvvfbuf, sizeof(jvvfbuf), jcx->jcx_flags[*pvix]);
                append_printf(&prbuf, &prlen, &prmax, " %-10s %-8s - ", kval, jvvfbuf);
                if (jpr_vix_is_const(*pvix, jcx->jcx_jvar))
                    append_charval(&prbuf, &prlen, &prmax, "const: " );
                else
                    append_charval(&prbuf, &prlen, &prmax, "     : ");
                jstat = jpr_jvarvalue_tostring(jx, &prbuf, &prlen, &prmax, jcx->jcx_ajvv + (*pvix), vflags);
                printf("%s\n", prbuf);
                prlen = 0;
            }
        }
    } while (!jstat && pvix);
    dbtsi_close(vcs);

    Free(prbuf);

    return (jstat);
}
/***************************************************************/
int jpr_exec_debug_show_allvars(
                    struct jrunexec * jx)
{
/*
** 02/22/2021
*/
    int jstat = 0;
    struct jcontext * jcx;
    struct jvarvalue_function * jvvf;

    jcx = jvar_get_head_jcontext(jx);
    if (!jcx) {
        printf("No variables.\n");
    } else {
        printf("All variables:\n");
        jvvf = jcx->jcx_jvvf;
        if (jvvf) {
            if (jvvf->jvvf_func_name) {
                printf("Function: %s\n", jvvf->jvvf_func_name);
            } else {
                printf("Function: %s\n", NO_FUNCTION_NAME);
            }
        }

        while (!jstat && jcx) {
            jstat = jpr_debug_show_jcontext(jx, jcx, 1, JPR_FLAGS_ALL_TYPES | JPR_FLAG_SHOW_TYPE);
            jcx = jcx->jcx_outer_jcx;
            if (jcx) printf("\n");
        }
    }

    return (jstat);
}
/***************************************************************/
void append_function_args(struct jrunexec * jx,
                    char ** argbuf,
                    int * arglen,
                    int * argmax,
                    struct jvarvalue_function * jvvf,
                    struct jcontext * jcx,
                    int appflags)
{
/*
** 07/14/2021
**      appflags : 1 - Show argument name
*/
    int jstat = 0;
    int argix;
    int vix;
    struct jvarvalue * jvv;
    int vflags;

    vflags = JPR_FLAGS_ALL_TYPES;

    append_chars(argbuf, arglen, argmax, "(", 1);
    for (argix = 0; argix < jvvf->jvvf_nargs; argix++) {
        if (argix) append_chars(argbuf, arglen, argmax, ",", 1);
        if (appflags & 1) {
            append_charval(argbuf, arglen, argmax, jvvf->jvvf_aargs[argix]->jvvfa_arg_name);
            append_chars(argbuf, arglen, argmax, "=", 1);
        }
        vix = jvvf->jvvf_aargs[argix]->jvvfa_vix;
        if (vix < 0 || vix > jcx->jcx_njvv) {
            append_charval(argbuf, arglen, argmax, "*invalid var index*");
        } else {
            jvv = jcx->jcx_ajvv + vix;
            jstat = jpr_jvarvalue_tostring(jx, argbuf, arglen, argmax, jvv, vflags);
        }
    }
    append_chars(argbuf, arglen, argmax, ")", 1);
}
/***************************************************************/
int jpr_exec_debug_show_stacktrace(
                    struct jrunexec * jx)
{
/*
** 07/14/2021
*/
    int jstat = 0;
    int ix;
    struct jfuncstate * jxfs;
    struct jvarvalue_function * jvvf;
    struct jcontext * jcx;
    char * argbuf = NULL;
    int    arglen = 0;
    int    argmax = 0;
    char * func_name;

    printf("Stacktrace:\n");
    ix = jx->jx_njfs - 1;
    while (ix >= 0) {
        jxfs = jx->jx_jfs[ix];
        jcx = jxfs->jfs_jcx;
        jvvf = jxfs->jfs_jvvf;
        append_function_args(jx, &argbuf, &arglen, &argmax, jvvf, jcx, 1);
        if (!jvvf->jvvf_func_name) func_name = NO_FUNCTION_NAME;
        else                       func_name = jvvf->jvvf_func_name;
        printf("%d %s%s\n", ix, func_name, argbuf);

        arglen = 0;
        ix--;
    }
    Free(argbuf);

    return (jstat);
}
/***************************************************************/
void jpr_xstack_char(char * xbuf, int xstate)
{
    switch (xstate) {
        case XSTAT_NONE                : strcpy(xbuf, "NONE");                  break;
        case XSTAT_TRUE_IF             : strcpy(xbuf, "TRUE_IF");               break;
        case XSTAT_TRUE_ELSE           : strcpy(xbuf, "TRUE_ELSE");             break;
        case XSTAT_TRUE_WHILE          : strcpy(xbuf, "TRUE_WHILE");            break;
        case XSTAT_ACTIVE_BLOCK        : strcpy(xbuf, "ACTIVE_BLOCK");          break;
        case XSTAT_INCLUDE_FILE        : strcpy(xbuf, "INCLUDE_FILE");          break;
        case XSTAT_FUNCTION            : strcpy(xbuf, "FUNCTION");              break;
        case XSTAT_BEGIN               : strcpy(xbuf, "BEGIN");                 break;
        case XSTAT_BEGIN_THREAD        : strcpy(xbuf, "BEGIN_THREAD");          break;
        case XSTAT_TRY                 : strcpy(xbuf, "TRY");                   break;
        case XSTAT_CATCH               : strcpy(xbuf, "CATCH");                 break;
        case XSTAT_FINALLY             : strcpy(xbuf, "FINALLY");               break;

        case XSTAT_FALSE_IF            : strcpy(xbuf, "FALSE_IF");              break;
        case XSTAT_FALSE_ELSE          : strcpy(xbuf, "FALSE_ELSE");            break;
        case XSTAT_INACTIVE_IF         : strcpy(xbuf, "INACTIVE_IF");           break;
        case XSTAT_INACTIVE_ELSE       : strcpy(xbuf, "INACTIVE_ELSE");         break;
        case XSTAT_INACTIVE_WHILE      : strcpy(xbuf, "INACTIVE_WHILE");        break;
        case XSTAT_INACTIVE_BLOCK      : strcpy(xbuf, "INACTIVE_BLOCK");        break;
        case XSTAT_DEFINE_FUNCTION     : strcpy(xbuf, "DEFINE_FUNCTION");       break;

        case XSTAT_TRUE_IF_COMPLETE    : strcpy(xbuf, "TRUE_IF_COMPLETE");      break;
        case XSTAT_FALSE_IF_COMPLETE   : strcpy(xbuf, "FALSE_IF_COMPLETE");     break;
        case XSTAT_INACTIVE_IF_COMPLETE: strcpy(xbuf, "INACTIVE_IF_COMPLETE");  break;

        default:
            sprintf(xbuf, "(?=%d)", xstate);
            break;
    }
}
/***************************************************************/
void jpr_xstack_flags_char(char * fbuf, int fbufsz, int xsflags)
{
/*
** 09/20/2021
*/
    fbuf[0] = '\0';
    if (xsflags & JXS_FLAGS_BEGIN_FUNC) {
        if (fbuf[0]) strcat(fbuf, ",");
        strcat(fbuf, "BEGIN_FUNC");
    }
}
/***************************************************************/
void jpr_get_varlist(struct jcontext * jcx, char * vlist, int vlistsz)
{
/*
** 11/04/2021
*/
    void * vcs;
    int * pvix;
    int klen;
    char * kval;
    int ktot;

    ktot = 0;

    vcs = dbtsi_rewind(jcx->jcx_jvar->jvar_jvarvalue_dbt, DBTREE_READ_FORWARD);
    do {
        pvix = dbtsi_next(vcs);
        if (pvix) {
            kval = dbtsi_get_key(vcs, &klen);
            if (ktot && ktot + 1 < vlistsz) vlist[ktot++] = ',';
            if (ktot + klen < vlistsz) {
                memcpy(vlist + ktot, kval, klen - 1);
                ktot += klen - 1;
            }
        }
    } while (pvix);
    dbtsi_close(vcs);
    vlist[ktot] = '\0';
}
/***************************************************************/
int jpr_exec_debug_show_runstack(struct jrunexec * jx)
{
/*
** 09/20/2021
** See also: jrun_print_xstack()
*/
    int jstat = 0;
    int sp;
    char xbuf[32];
    char fbuf[64];
    char vlist[128];

    printf("Runstack:\n");
    for (sp = jx->jx_njs - 1; sp >= 0; sp--) {

        jpr_xstack_char(xbuf, jx->jx_js[sp]->jxs_xstate);
        jpr_xstack_flags_char(fbuf, sizeof(fbuf), jx->jx_js[sp]->jxs_flags);
        if (jx->jx_js[sp]->jxs_jcx) {
            jpr_get_varlist(jx->jx_js[sp]->jxs_jcx, vlist, sizeof(vlist));
            printf("    %d. %-20s %-20s %s\n", sp, xbuf, fbuf, vlist);
        } else {
            printf("    %d. %-20s %-20s\n", sp, xbuf, fbuf);
        }
    }

    return (jstat);
}
/***************************************************************/
int jpr_exec_debug_show_sysinfo(
                    struct jrunexec * jx)
{
/*
** 10/18/2021
*/
    int jstat = 0;

    printf("sizeof(struct jvarvalue)=%d\n", sizeof(struct jvarvalue));

    return (jstat);
}
/***************************************************************/
int jpr_exec_debug_show_help(struct jrunexec * jx)
{
/*
** 10/18/2021
*/
    int jstat = 0;

    printf("'debug show' syntax:\n");
    printf("    'debug show allvars'\n");
    printf("    'debug show help'\n");
    printf("    'debug show runstack'\n");
    printf("    'debug show stacktrace'\n");
    printf("    'debug show sysinfo'\n");

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
    char toke[TOKSZ];

    get_toke_char(dbgstr, ix, toke, TOKSZ, GETTOKE_FLAGS_DEFAULT);
    if (!strcmp(toke, "allvars")) {
        jstat = jpr_exec_debug_show_allvars(jx);
    } else if (!strcmp(toke, "stacktrace")) {
        jstat = jpr_exec_debug_show_stacktrace(jx);
    } else if (!strcmp(toke, "runstack")) {
        jstat = jpr_exec_debug_show_runstack(jx);
    } else if (!strcmp(toke, "sysinfo")) {
        jstat = jpr_exec_debug_show_sysinfo(jx);
    } else if (!strcmp(toke, "help") || !strcmp(toke, "?")) {
        jstat = jpr_exec_debug_show_help(jx);
    }

    return (jstat);
}
/***************************************************************/
