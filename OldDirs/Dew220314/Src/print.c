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
#include "internal.h"
#include "array.h"
#include "obj.h"

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
int jpr_int_object_call_method(
    struct jrunexec  * jx,
    void             * this_obj,
    struct jvarvalue * mjvv,
    struct jvarvalue * rtnjvv)
{
/*
** 11/22/2021
*/
    int jstat = 0;
    struct jvarvalue * jvvargs;
    struct jvarvalue   jvvargrec;
    struct jvarvalue_vallist jvvl;

    INIT_JVARVALUE(rtnjvv);

    jvvl.jvvl_nvals = 0;
    jvvl.jvvl_xvals = 0;
    jvvl.jvvl_avals = NULL;
    jvvargrec.jvv_dtype = JVV_DTYPE_VALLIST;
    jvvargrec.jvv_val.jvv_jvvl = &jvvl;
    jvvargs = &jvvargrec;

    jstat = jexp_internal_method_call(jx, mjvv->jvv_val.jvv_int_method, this_obj, jvvargs, rtnjvv);

    return (jstat);
}
/***************************************************************/
int jpr_int_object_tostring(
    struct jrunexec * jx,
    char ** prbuf,
    int * prlen,
    int * prmax,
    struct jvarvalue * cjvv,
    int * did_cvt,
    int jfmtflags)
{
/*
** 11/22/2021
*/
    int jstat = 0;
    struct jvarvalue * mjvv;
    struct jvarvalue   rtnjvv;

    (*did_cvt) = 0;

    if (cjvv->jvv_dtype == JVV_DTYPE_INTERNAL_OBJECT) {
        mjvv = jvar_int_object_member(jx, cjvv, JVV_INTERNAL_METHOD_toString);
        if (mjvv && mjvv->jvv_dtype == JVV_DTYPE_INTERNAL_METHOD) {
            jstat = jpr_int_object_call_method(jx, cjvv->jvv_val.jvv_jvvo->jvvo_this_ptr, mjvv, &rtnjvv);
            if (!jstat) {
                if (rtnjvv.jvv_dtype == JVV_DTYPE_CHARS) {
                    (*did_cvt) = 1;
                    append_chars(prbuf, prlen, prmax,
                        rtnjvv.jvv_val.jvv_val_chars.jvvc_val_chars,
                        rtnjvv.jvv_val.jvv_val_chars.jvvc_length);
                    
                }
                jvar_free_jvarvalue_data(&rtnjvv);
            }
        }
    }

    return (jstat);
}
/***************************************************************/
int jpr_jsint_tostring(
    struct jrunexec * jx,
    char ** prbuf,
    int * prlen,
    int * prmax,
    JSINT val,
    int jfmtflags)
{
/*
** 02/14/2022
*/
    int jstat = 0;

    if (jfmtflags & JFMT_FLAG_SHOW_TYPE) append_charval(prbuf, prlen, prmax, "INT: ");

    switch  (EXTRACT_JFMT_FMT(jfmtflags)) {
        case JFMT_FMT_none:
            append_printf(prbuf, prlen, prmax, "%d", val);
            break;
        case JFMT_FMT_d:
            append_printf(prbuf, prlen, prmax, "%d", val);
            break;
        case JFMT_FMT_s:
            append_printf(prbuf, prlen, prmax, "%d", val);
            break;

        case JFMT_FMT_f:
        case JFMT_FMT_i:
        case JFMT_FMT_j:
        case JFMT_FMT_o:
        case JFMT_FMT_O:
            append_printf(prbuf, prlen, prmax, "%d", val);
            break;

        default:
            append_printf(prbuf, prlen, prmax, "%d", val);
            break;
    }

    return (jstat);
}
/***************************************************************/
int jpr_chars_tostring(
    struct jrunexec * jx,
    char ** prbuf,
    int * prlen,
    int * prmax,
    char * charval,
    int jfmtflags)
{
/*
** 02/14/2022
*/
    int jstat = 0;
    char * val;
    int cstat;
    int intnum;
    double dblnum;

    val = charval;
    if (!val) val = "";

    if (jfmtflags & JFMT_FLAG_SHOW_TYPE) append_charval(prbuf, prlen, prmax, "CHARS: ");

    switch  (EXTRACT_JFMT_FMT(jfmtflags)) {
        case JFMT_FMT_none:
            if (jfmtflags & JFMT_FLAG_SINGLE_QUOTES) {
                append_printf(prbuf, prlen, prmax, "\'%s\'", val);
            } else if (jfmtflags & JFMT_FLAG_DOUBLE_QUOTES) {
                append_printf(prbuf, prlen, prmax, "\"%s\"", val);
            } else {
                append_charval(prbuf, prlen, prmax, val);
            }
            break;
        case JFMT_FMT_d:
        case JFMT_FMT_f:
        case JFMT_FMT_i:
            cstat = convert_string_to_number(val, &intnum, &dblnum);
            if (cstat == 1) {
                append_printf(prbuf, prlen, prmax, "%d", intnum);
            } else if (cstat == 2) {
                append_printf(prbuf, prlen, prmax, "%g", dblnum);
            } else {
                append_charval(prbuf, prlen, prmax, NAN_DISPLAY_STRING);
            }
            break;
        case JFMT_FMT_s:
            if (jfmtflags & JFMT_FLAG_SINGLE_QUOTES) {
                append_printf(prbuf, prlen, prmax, "\'%s\'", val);
            } else {
                append_charval(prbuf, prlen, prmax, val);
            }
            break;

        case JFMT_FMT_j:
            append_printf(prbuf, prlen, prmax, "\"%s\"", val);
            break;

        case JFMT_FMT_o:
        case JFMT_FMT_O:
            if (EXTRACT_JFMT_ORIGIN(jfmtflags) == JFMT_ORIGIN_FORMAT ||
                (jfmtflags & JFMT_FLAG_SINGLE_QUOTES)) {
                append_printf(prbuf, prlen, prmax, "\'%s\'", val);
            } else {
                append_printf(prbuf, prlen, prmax, "%s", val);
            }
            break;

        default:
            append_charval(prbuf, prlen, prmax, val);
            break;
    }

    return (jstat);
}
/***************************************************************/
int jpr_jvarvalue_tostring(
    struct jrunexec * jx,
    char ** prbuf,
    int * prlen,
    int * prmax,
    struct jvarvalue * jvv,
    int jfmtflags)
{
/*
** 03/11/2021 - 09/27/21 All to strings go through here
*/
    int jstat = 0;
    int did_cvt;
    struct jvarvalue * jvvtmp;

    if (!jvv) {
        append_charval(prbuf, prlen, prmax, "*null var*");
        return (0);
    }

    switch (jvv->jvv_dtype) {
        case JVV_DTYPE_NONE   :
            if (jfmtflags & JFMT_FLAG_SHOW_TYPE)     append_charval(prbuf, prlen, prmax, "NONE: ");
            if (jfmtflags & JFMT_FLAG_USE_NULL_FOR_NULL) {
                append_charval(prbuf, prlen, prmax, NULL_DISPLAY_STRING);
            } else if (jfmtflags & JFMT_FLAG_USE_BLANK_FOR_NULL) {
                /* don't do anything */
            } else {
                append_charval(prbuf, prlen, prmax, UNDEFINED_DISPLAY_STRING);
            }
            break;
        
        case JVV_DTYPE_BOOL   :
            if (jfmtflags & JFMT_FLAG_SHOW_TYPE) append_charval(prbuf, prlen, prmax, "BOOL: ");
            if (jvv->jvv_val.jvv_val_bool==0) append_charval(prbuf, prlen, prmax, "false");
            else append_charval(prbuf, prlen, prmax, "true");
            break;

        case JVV_DTYPE_JSINT   :
            jstat = jpr_jsint_tostring(jx, prbuf, prlen, prmax, jvv->jvv_val.jvv_val_jsint, jfmtflags);
            break;

        case JVV_DTYPE_JSFLOAT   :
            if (jfmtflags & JFMT_FLAG_SHOW_TYPE) append_charval(prbuf, prlen, prmax, "FLOAT: ");
            append_printf(prbuf, prlen, prmax, "%g", jvv->jvv_val.jvv_val_jsfloat);
            break;

        case JVV_DTYPE_CHARS   :
            jstat = jpr_chars_tostring(jx, prbuf, prlen, prmax, jvv->jvv_val.jvv_val_chars.jvvc_val_chars, jfmtflags);
            break;

        case JVV_DTYPE_LVAL:
            if (jfmtflags & JFMT_FLAG_SHOW_TYPE) append_charval(prbuf, prlen, prmax, "LVAL: ");
            jstat = jexp_eval_rval(jx, jvv, &jvvtmp);
            if (!jstat) {
                jstat = jpr_jvarvalue_tostring(jx, prbuf, prlen, prmax, jvvtmp, jfmtflags);
            }
            break;

        case JVV_DTYPE_INTERNAL_CLASS:
            if (jfmtflags & JFMT_FLAG_SHOW_TYPE) append_charval(prbuf, prlen, prmax, "INT CLASS: ");
            append_charval(prbuf, prlen, prmax, "class value");
            break;

        case JVV_DTYPE_INTERNAL_METHOD:
            if (jfmtflags & JFMT_FLAG_SHOW_TYPE)
                append_printf(prbuf, prlen, prmax, "INT METHOD<%d>: ", jvv->jvv_val.jvv_int_method->jvvim_nRefs);
            append_charval(prbuf, prlen, prmax, "method");
            break;

        case JVV_DTYPE_INTERNAL_OBJECT:
            if (jfmtflags & JFMT_FLAG_SHOW_TYPE)
                append_printf(prbuf, prlen, prmax, "INT OBJ<%d>: ", jvv->jvv_val.jvv_jvvo->jvvo_nRefs);
            did_cvt = 0;
            jstat = jpr_int_object_tostring(jx, prbuf, prlen, prmax, jvv, &did_cvt, jfmtflags);
            if (!did_cvt) {
#if IS_DEBUG
                append_charval(prbuf, prlen, prmax, "[");
                if (jvv->jvv_val.jvv_jvvo->jvvo_sn[0])
                    append_chars(prbuf, prlen, prmax, jvv->jvv_val.jvv_jvvo->jvvo_sn, 4);
                append_charval(prbuf, prlen, prmax, "]");
                if (!jvv->jvv_val.jvv_jvvo->jvvo_jcx) append_charval(prbuf, prlen, prmax, " ** no jcx **");
#else
                append_charval(prbuf, prlen, prmax, "internal object");
#endif
            }
            break;

        case JVV_DTYPE_VALLIST:
            if (jfmtflags & JFMT_FLAG_SHOW_TYPE) append_charval(prbuf, prlen, prmax, "VALLIST: ");
            append_charval(prbuf, prlen, prmax, "value list");
            break;

        case JVV_DTYPE_TOKEN:
            if (jfmtflags & JFMT_FLAG_SHOW_TYPE) append_charval(prbuf, prlen, prmax, "TOKEN: ");
            append_printf(prbuf, prlen, prmax, "\"%s\"", jvv->jvv_val.jvv_val_token.jvvt_token);
            break;

        case JVV_DTYPE_FUNCTION:
            if (jfmtflags & JFMT_FLAG_SHOW_TYPE) {
#if IS_DEBUG
                append_charval(prbuf, prlen, prmax, "FUNCTION[");
                append_chars(prbuf, prlen, prmax, jvv->jvv_val.jvv_val_function->jvvf_sn, 4);
                append_printf(prbuf, prlen, prmax, "<%d>]: ", jvv->jvv_val.jvv_val_function->jvvf_nRefs);
#else
                append_charval(prbuf, prlen, prmax, "FUNCTION: ");
#endif
            }
            append_printf(prbuf, prlen, prmax, "\"%s\"", jvv->jvv_val.jvv_val_function->jvvf_func_name);
            break;

        case JVV_DTYPE_FUNCVAR   :
            if (jfmtflags & JFMT_FLAG_SHOW_TYPE) append_printf(prbuf, prlen, prmax, "FUNCVAR<%d>: ",
                jvv->jvv_val.jvv_val_funcvar.jvvfv_jvvf->jvvf_nRefs);
            append_printf(prbuf, prlen, prmax, "f %s",
                jvv->jvv_val.jvv_val_funcvar.jvvfv_jvvf->jvvf_func_name);
            break;

        case JVV_DTYPE_IMETHVAR   :
            if (jfmtflags & JFMT_FLAG_SHOW_TYPE) append_charval(prbuf, prlen, prmax, "IMETHVAR: ");
            append_printf(prbuf, prlen, prmax, "f %s",
                jvv->jvv_val.jvv_val_imethvar.jvvimv_jvvim->jvvim_method_name);
            break;
        
        case JVV_DTYPE_ARRAY:
            if (jfmtflags & JFMT_FLAG_SHOW_TYPE) append_printf(prbuf, prlen, prmax, "ARRAY<%d>: ",
                jvv->jvv_val.jvv_val_array->jvva_nRefs);
            jstat = jpr_array_tostring(jx, prbuf, prlen, prmax, jvv->jvv_val.jvv_val_array, jfmtflags);
            break;
        
        case JVV_DTYPE_OBJPTR:
            if (jfmtflags & JFMT_FLAG_SHOW_TYPE) append_charval(prbuf, prlen, prmax, "OBJPTR: ");
            switch (jvv->jvv_val.jvv_objptr.jvvp_pval->jvv_dtype) {
                case JVV_DTYPE_ARRAY   :
                    if (jfmtflags & JFMT_FLAG_SHOW_TYPE) append_printf(prbuf, prlen, prmax, "ARRAY<%d>: ",
                        jvv->jvv_val.jvv_objptr.jvvp_pval->jvv_val.jvv_val_array->jvva_nRefs);
                    jstat = jpr_array_tostring(jx, prbuf, prlen, prmax,
                        jvv->jvv_val.jvv_objptr.jvvp_pval->jvv_val.jvv_val_array, jfmtflags);
                    break;
                case JVV_DTYPE_OBJECT   :
                    if (jfmtflags & JFMT_FLAG_SHOW_TYPE) append_printf(prbuf, prlen, prmax, "OBJECT<%d>: ",
                        jvv->jvv_val.jvv_objptr.jvvp_pval->jvv_val.jvv_val_object->jvvb_nRefs);
                    jstat = jpr_object_tostring(jx, prbuf, prlen, prmax,
                        jvv->jvv_val.jvv_objptr.jvvp_pval->jvv_val.jvv_val_object, jfmtflags);
                    break;
                default:
                    append_printf(prbuf, prlen, prmax, "*Unknown object pointer tostring type=%s*",
                        jvar_get_dtype_name(jvv->jvv_val.jvv_objptr.jvvp_pval->jvv_dtype));
                    break;
            }
            break;

        case JVV_DTYPE_OBJECT   :
            if (jfmtflags & JFMT_FLAG_SHOW_TYPE) append_printf(prbuf, prlen, prmax, "OBJECT<%d>: ",
                jvv->jvv_val.jvv_val_object->jvvb_nRefs);
            jstat = jpr_object_tostring(jx, prbuf, prlen, prmax, jvv->jvv_val.jvv_val_object, jfmtflags);
            break;

        case JVV_DTYPE_DYNAMIC:
            if (jfmtflags & JFMT_FLAG_SHOW_TYPE) append_charval(prbuf, prlen, prmax, "DYNAMIC: ");
            jstat = jexp_eval_rval(jx, jvv, &jvvtmp);
            if (!jstat) {
                jstat = jpr_jvarvalue_tostring(jx, prbuf, prlen, prmax, jvvtmp, jfmtflags);
            }
            break;

        default:
            append_printf(prbuf, prlen, prmax, "*Unknown tostring type=%s*", jvar_get_dtype_name(jvv->jvv_dtype));
            break;
    }

    return (jstat);
}
/***************************************************************/
int jpr_jvarvalue_tocharstar(
    struct jrunexec * jx,
    char ** charstar,
    struct jvarvalue * jvv,
    int jfmtflags)
{
/*
** 11/16/2021
*/
    int jstat;
    int cslen;
    int csmax;

    (*charstar) = NULL;
    cslen = 0;
    csmax = 0;

    jstat = jpr_jvarvalue_tostring(jx, charstar, &cslen, &csmax, jvv, jfmtflags);

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
void jpr_jvvflags_char(char * jvvfbuf, int jvvfbufsz, int jvvflags)
{
/*
** 10/06/2021
*/
    jvvfbuf[0] = '\0';
    if (!jvvflags) {
        if (jvvfbuf[0]) strcat(jvvfbuf, ",");
        strcat(jvvfbuf, "None");
    }
    if (jvvflags & JCX_FLAG_METHOD) {
        if (jvvfbuf[0]) strcat(jvvfbuf, ",");
        strcat(jvvfbuf, "Meth");
    }
    if (jvvflags & JCX_FLAG_PROPERTY) {
        if (jvvfbuf[0]) strcat(jvvfbuf, ",");
        strcat(jvvfbuf, "Prop");
    }
    if (jvvflags & JCX_FLAG_CONST) {
        if (jvvfbuf[0]) strcat(jvvfbuf, ",");
        strcat(jvvfbuf, "Const");
    }
    if (jvvflags & JCX_FLAG_FUNCTION) {
        if (jvvfbuf[0]) strcat(jvvfbuf, ",");
        strcat(jvvfbuf, "Func");
    }
}
/***************************************************************/
static int jpr_vartype_matches(
                    struct jrunexec * jx,
                    struct jvarvalue * jvv,
                    int jfmtflags)
{
/*
** 12/03/2021
*/
    int matches = 0;

    switch (jvv->jvv_dtype){
        case JVV_DTYPE_INTERNAL_CLASS :
            if (jfmtflags & JFMT_FILTER_CLASSES) matches = 1;
            break;

        default:
            if (jfmtflags & JFMT_FILTER_VARS) matches = 1;
            break;
    }

    return (matches);
}
/***************************************************************/
static int jpr_debug_show_jcontext(
                    struct jrunexec * jx,
                    struct jcontext * jcx,
                    int tab,
                    int jfmtflags)
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
            } else if (!jpr_vartype_matches(jx, jcx->jcx_ajvv + (*pvix), jfmtflags)) {
                /* do nothing */
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
                append_chars(&prbuf, &prlen, &prmax, jcx->jcx_jvar->jvar_sn, 4);
                append_printf(&prbuf, &prlen, &prmax, " %2d ", jcx->jcx_jvar->jvar_nRefs);
                if (jcx->jcx_ajvv[*pvix].jvv_sn[0])
                    append_chars(&prbuf, &prlen, &prmax, jcx->jcx_ajvv[*pvix].jvv_sn, 4);
                else
                    append_chars(&prbuf, &prlen, &prmax, "nosn", 4);
#endif
                jpr_jvvflags_char(jvvfbuf, sizeof(jvvfbuf), jcx->jcx_ajvv[*pvix].jvv_flags);
                append_printf(&prbuf, &prlen, &prmax, " %-10s %-8s - ", kval, jvvfbuf);
                if (jpr_vix_is_const(*pvix, jcx->jcx_jvar))
                    append_charval(&prbuf, &prlen, &prmax, "const: " );
                else
                    append_charval(&prbuf, &prlen, &prmax, "     : ");
                jstat = jpr_jvarvalue_tostring(jx, &prbuf, &prlen, &prmax, jcx->jcx_ajvv + (*pvix), jfmtflags);
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
                    struct jrunexec * jx, int jfmtflags)
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
            jstat = jpr_debug_show_jcontext(jx, jcx, 1, jfmtflags | JFMT_FLAG_SHOW_TYPE);
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
                    int jfmtflags)
{
/*
** 07/14/2021
*/
    int jstat = 0;
    int argix;
    int vix;
    struct jvarvalue * jvv;

    append_chars(argbuf, arglen, argmax, "(", 1);
    for (argix = 0; argix < jvvf->jvvf_nargs; argix++) {
        if (argix) append_chars(argbuf, arglen, argmax, ",", 1);
        if (jfmtflags & JFMT_FLAG_SHOW_NAME) {
            append_charval(argbuf, arglen, argmax, jvvf->jvvf_aargs[argix]->jvvfa_arg_name);
            append_chars(argbuf, arglen, argmax, "=", 1);
        }
        vix = jvvf->jvvf_aargs[argix]->jvvfa_vix;
        if (vix < 0 || vix > jcx->jcx_njvv) {
            append_charval(argbuf, arglen, argmax, "*invalid var index*");
        } else {
            jvv = jcx->jcx_ajvv + vix;
            jstat = jpr_jvarvalue_tostring(jx, argbuf, arglen, argmax, jvv, jfmtflags | JFMT_FMT_j);
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
        append_function_args(jx, &argbuf, &arglen, &argmax, jvvf, jcx, JFMT_FLAG_SHOW_NAME);
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

        case XSTAT_TRY_COMPLETE        : strcpy(xbuf, "TRY_COMPLETE");          break;
        case XSTAT_INACTIVE_TRY        : strcpy(xbuf, "INACTIVE_TRY");          break;
        case XSTAT_CATCH_COMPLETE      : strcpy(xbuf, "CATCH_COMPLETE");        break;
        case XSTAT_FINALLY_COMPLETE    : strcpy(xbuf, "FINALLY_COMPLETE");      break;
        case XSTAT_FAILED_CATCH        : strcpy(xbuf, "FAILED_CATCH");          break;
        case XSTAT_INACTIVE_CATCH      : strcpy(xbuf, "INACTIVE_CATCH");        break;
        case XSTAT_INACTIVE_TRY_CATCH  : strcpy(xbuf, "INACTIVE_TRY_CATCH");    break;
        case XSTAT_INACTIVE_FINALLY    : strcpy(xbuf, "INACTIVE_FINALLY");      break;

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
    char ebuf[32];
    char xbuf[32];
    char fbuf[64];
    char vlist[128];

    printf("Runstack:\n");
    for (sp = jx->jx_njs - 1; sp >= 0; sp--) {

        jpr_xstack_char(xbuf, jx->jx_js[sp]->jxs_xstate);
        jpr_xstack_flags_char(fbuf, sizeof(fbuf), jx->jx_js[sp]->jxs_flags);
        ebuf[0] = '\0';
        if (jx->jx_js[sp]->jxs_finally_jstat) strcpy(ebuf, "*Err");
        if (jx->jx_js[sp]->jxs_jcx) {
            jpr_get_varlist(jx->jx_js[sp]->jxs_jcx, vlist, sizeof(vlist));
            printf("    %d. %-15s %-4s %-20s %s\n", sp, xbuf, ebuf, fbuf, vlist);
        } else {
            printf("    %d. %-15s %-4s %-20s\n", sp, xbuf, ebuf, fbuf);
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

    printf("sizeof(struct jvarvalue)=%d\n", (int)sizeof(struct jvarvalue));

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
    printf("    'debug show allclasses'\n");
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
**
**  e.g. 'debug show allvars'
*/
    int jstat = 0;
    char toke[TOKSZ];

    get_toke_char(dbgstr, ix, toke, TOKSZ, GETTOKE_FLAGS_DEFAULT);
    if (!strcmp(toke, "allclasses")) {
        jstat = jpr_exec_debug_show_allvars(jx, JFMT_FILTER_CLASSES);
    } else if (!strcmp(toke, "allvars")) {
        jstat = jpr_exec_debug_show_allvars(jx, JFMT_FILTER_VARS);
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
