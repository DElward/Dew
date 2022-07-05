/***************************************************************/
/*
**  var.c --  Variable classes
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

/***************************************************************/
struct jvarvalue * jvar_new_jvarvalue(void)
{
    struct jvarvalue * jvv;
#if IS_DEBUG
    static int next_jvv_sn = 0;
#endif

    Free(0);
    jvv = New(struct jvarvalue, 1);
#if IS_DEBUG
    jvv->jvv_sn[0] = 'V';
    jvv->jvv_sn[1] = (next_jvv_sn / 100) % 10 + '0';
    jvv->jvv_sn[2] = (next_jvv_sn /  10) % 10 + '0';
    jvv->jvv_sn[3] = (next_jvv_sn      ) % 10 + '0';
    next_jvv_sn++;
#endif

    INIT_JVARVALUE(jvv);

    return (jvv);
}
/***************************************************************/
struct jvarrec * jvar_new_jvarrec(void)
{
    struct jvarrec * jvar;
#if IS_DEBUG
    static int next_jvar_sn = 0;
#endif

    jvar = New(struct jvarrec, 1);
#if IS_DEBUG
    jvar->jvar_sn[0] = 'A';
    jvar->jvar_sn[1] = (next_jvar_sn / 100) % 10 + '0';
    jvar->jvar_sn[2] = (next_jvar_sn /  10) % 10 + '0';
    jvar->jvar_sn[3] = (next_jvar_sn      ) % 10 + '0';
    next_jvar_sn++;
#endif

    jvar->jvar_jvarvalue_dbt    = dbtsi_new();
    /*jvar->jvar_prev             = NULL;*/
    jvar->jvar_nvars            = 0;
    jvar->jvar_nconsts          = 0;
    jvar->jvar_xconsts          = 0;
    jvar->jvar_avix             = NULL;
    jvar->jvar_aconsts          = NULL;

    return (jvar);
}
/***************************************************************/
int * jvar_find_in_jvarrec(struct jvarrec * jvar, const char * varname)
{
/*
** 03/03/2021
*/

    int * pvix;

    pvix = dbtsi_find(jvar->jvar_jvarvalue_dbt, varname);

    return (pvix);
}
/***************************************************************/
void jvar_new_local_const(struct jvarrec * jvar, struct jvarvalue * jvv, int vix)
{
/*
** 03/03/2021
*/
    if (jvar->jvar_nconsts == jvar->jvar_xconsts) {
        if (!jvar->jvar_nconsts) jvar->jvar_xconsts = 2;
        else                     jvar->jvar_xconsts *= 2;
        jvar->jvar_avix =
            Realloc(jvar->jvar_avix, int, jvar->jvar_xconsts);
        jvar->jvar_aconsts =
            Realloc(jvar->jvar_aconsts, struct jvarvalue*, jvar->jvar_xconsts);
    }
    jvar->jvar_avix[jvar->jvar_nconsts] = vix;
    jvar->jvar_aconsts[jvar->jvar_nconsts] = jvv;
    jvar->jvar_nconsts += 1;
}
/***************************************************************/
#if 0
int jvar_is_const_jvv(struct jvarvalue * jvv)
{
/*
** 03/05/2021
*/

    int is_const;

    is_const = 0;
    if (jvv) {
        if (jvv->jvv_dtype == JVV_DTYPE_INTERNAL_METHOD) {
            is_const = 1;
        }
    }

    return (is_const);
}
#endif
/***************************************************************/
int jvar_insert_into_jvarrec(struct jvarrec * jvar, const char * varname)
{
/*
** 03/05/2021
*/

    int is_dup;
    int vix;

    vix = jvar->jvar_nvars;

    is_dup = dbtsi_insert(jvar->jvar_jvarvalue_dbt, varname, jvar->jvar_nvars);
    if (is_dup) {
        vix = VINVALID;
    } else {
        jvar->jvar_nvars += 1;
    }

    return (vix);
}
/***************************************************************/
char * jvar_get_dtype_name(int dtype)
{
/*
**
*/
    char * out;

    switch (dtype) {
        case JVV_DTYPE_NONE            : out = "None";          break;
        case JVV_DTYPE_BOOL            : out = "Bool";          break;
        case JVV_DTYPE_JSINT           : out = "JSInt";         break;
        case JVV_DTYPE_JSFLOAT         : out = "JSFloat";       break;
        case JVV_DTYPE_CHARS           : out = "Chars";         break;
        case JVV_DTYPE_NUMBER          : out = "Number";        break;
        case JVV_DTYPE_NULL            : out = "Null";          break;
        case JVV_DTYPE_LVAL            : out = "LVal";          break;
        case JVV_DTYPE_INTERNAL_CLASS  : out = "InteralClass";  break;
        case JVV_DTYPE_INTERNAL_METHOD : out = "InteralMethod"; break;
        case JVV_DTYPE_OBJECT          : out = "Object";        break;
        case JVV_DTYPE_VALLIST         : out = "ValueList";     break;
        case JVV_DTYPE_TOKEN           : out = "Token";         break;
        case JVV_DTYPE_FUNCTION        : out = "Function";      break;
        default                        : out = "*Undefined*";   break;
    }

    return (out);
}
/***************************************************************/
void jvar_free_jvarvalue_internal_class(struct jvarvalue_internal_class * jvvi)
{
/*
**
*/
    Free(jvvi->jvvi_class_name);
    jvar_free_jvarrec_consts(jvvi->jvvi_jvar);
    jvar_free_jvarrec(jvvi->jvvi_jvar);
    Free(jvvi);
}
/***************************************************************/
void jvar_free_jvarvalue_object_data(struct jvarvalue_object * jvvo)
{
/*
**
*/
    jvar_free_jcontext(jvvo->jvvo_jcx);
}
/***************************************************************/
static void jvvl_free_jvarvalue_vallist(struct jvarvalue_vallist * jvvlargs)
{
/*
**
*/
    int ii;

    for (ii = jvvlargs->jvvl_nvals - 1; ii >= 0; ii--) {
        jvar_free_jvarvalue_data(&(jvvlargs->jvvl_avals[ii]));
    }
    Free(jvvlargs->jvvl_avals);
    Free(jvvlargs);
}
/***************************************************************/
void jvar_free_jvarvalue_data(struct jvarvalue * jvv)
{
/*
**
*/
    if (!jvv) return;

    switch (jvv->jvv_dtype) {
        case JVV_DTYPE_NONE  :
        case JVV_DTYPE_BOOL  :
        case JVV_DTYPE_JSINT   :
        case JVV_DTYPE_JSFLOAT:
        case JVV_DTYPE_LVAL :
            break;
        case JVV_DTYPE_CHARS :
            Free(jvv->jvv_val.jvv_val_chars.jvvc_val_chars);
            break;
        case JVV_DTYPE_INTERNAL_CLASS :
            jvar_free_jvarvalue_internal_class(jvv->jvv_val.jvv_jvvi);
            break;
        case JVV_DTYPE_OBJECT :
            jvar_free_jvarvalue_object_data(&(jvv->jvv_val.jvv_jvvo));
            break;
        case JVV_DTYPE_INTERNAL_METHOD:
            Free(jvv->jvv_val.jvv_int_method.jvvim_method_name);
            break;
        case JVV_DTYPE_TOKEN :
            Free(jvv->jvv_val.jvv_val_token.jvvt_token);
            break;
        case JVV_DTYPE_VALLIST:
            jvvl_free_jvarvalue_vallist(jvv->jvv_val.jvv_jvvl);
            break;
        case JVV_DTYPE_FUNCTION:
            jvar_free_jvarvalue_function(jvv->jvv_val.jvv_val_function);
            break;
        default:
            printf("jvar_free_jvarvalue_data(%s)\n",
                jvar_get_dtype_name(jvv->jvv_dtype));
            break;
    }
    jvv->jvv_dtype = JVV_DTYPE_NONE;
}
/***************************************************************/
void jvar_free_jvarvalue(struct jvarvalue * jvv)
{
/*
** 02/19/2021
*/
    jvar_free_jvarvalue_data(jvv);
    Free(jvv);
}
/***************************************************************/
void jvar_store_null(struct jvarvalue * jvv)
{
/*
**
*/
    jvar_free_jvarvalue_data(jvv);
    jvv->jvv_dtype = JVV_DTYPE_NULL;
}
/***************************************************************/
void jvar_store_bool(
    struct jvarvalue * jvv,
    int val_jsbool)
{
/*
**
*/
    jvar_free_jvarvalue_data(jvv);
    jvv->jvv_dtype = JVV_DTYPE_BOOL;
    jvv->jvv_val.jvv_val_bool = val_jsbool?1:0;
}
/***************************************************************/
void jvar_store_jsint(
    struct jvarvalue * jvv,
    JSINT val_jsint)
{
/*
**
*/
    jvar_free_jvarvalue_data(jvv);
    jvv->jvv_dtype = JVV_DTYPE_JSINT;
    jvv->jvv_val.jvv_val_jsint = val_jsint;
}
/***************************************************************/
void jvar_store_jsfloat(
    struct jvarvalue * jvv,
    JSFLOAT val_jsfloat)
{
/*
**
*/
    jvar_free_jvarvalue_data(jvv);
    jvv->jvv_dtype = JVV_DTYPE_JSFLOAT;
    jvv->jvv_val.jvv_val_jsfloat = val_jsfloat;
}
/***************************************************************/
void jvar_store_jvarvalue(
    struct jrunexec * jx,
    struct jvarvalue * jvvtgt,
    struct jvarvalue * jvvsrc)
{
/*
**
    JVV_DTYPE_BOOL              ,
    JVV_DTYPE_JSINT             ,
    JVV_DTYPE_JSFLOAT           ,
    JVV_DTYPE_CHARS             ,
    JVV_DTYPE_NUMBER            ,
    JVV_DTYPE_NULL              ,
    JVV_DTYPE_LVAL              ,
    JVV_DTYPE_INTERNAL_CLASS    ,
    JVV_DTYPE_INTERNAL_METHOD   ,
    JVV_DTYPE_OBJECT            ,
*/
    switch (jvvsrc->jvv_dtype) {
        case JVV_DTYPE_NULL   :
            jvar_store_null(jvvtgt);
            break;
        case JVV_DTYPE_BOOL   :
            jvar_store_bool(jvvtgt, jvvsrc->jvv_val.jvv_val_bool);
            break;
        case JVV_DTYPE_JSINT   :
            jvar_store_jsint(jvvtgt, jvvsrc->jvv_val.jvv_val_jsint);
            break;
        case JVV_DTYPE_JSFLOAT   :
            jvar_store_jsfloat(jvvtgt, jvvsrc->jvv_val.jvv_val_jsfloat);
            break;
        case JVV_DTYPE_CHARS   :
            jvar_store_chars_len(jvvtgt,
                jvvsrc->jvv_val.jvv_val_chars.jvvc_val_chars,
                jvvsrc->jvv_val.jvv_val_chars.jvvc_length);
            break;
        case JVV_DTYPE_INTERNAL_METHOD   :
            jvvtgt->jvv_dtype = jvvsrc->jvv_dtype;
            jvvtgt->jvv_val.jvv_int_method.jvvim_method = jvvsrc->jvv_val.jvv_int_method.jvvim_method;
            jvvtgt->jvv_val.jvv_int_method.jvvim_method_name = 
                Strdup(jvvsrc->jvv_val.jvv_int_method.jvvim_method_name);
            break;

        case JVV_DTYPE_INTERNAL_CLASS   :
            jvvtgt->jvv_val.jvv_jvvi = jvvsrc->jvv_val.jvv_jvvi;
            break;
        default:
            break;
    }
}
/***************************************************************/
struct jvarvalue * jvv_dup_jvarvalue(struct jrunexec * jx, struct jvarvalue * jvv)
{
/*
**
*/
    struct jvarvalue * njvv;

    njvv = jvar_new_jvarvalue();
    jvar_store_jvarvalue(jx, njvv, jvv);

    return (njvv);
}
/***************************************************************/
void jvv_free_jvarvalue(struct jvarvalue * jvv)
{
    jvar_free_jvarvalue_data(jvv);
    Free(jvv);
}
/***************************************************************/
static void jvv_free_vjvarvalue(void * vjvv)
{
    jvv_free_jvarvalue((struct jvarvalue*)vjvv);
}
/***************************************************************/
void jvar_free_jvarrec(struct jvarrec * jvar)
{
    dbtsi_free(jvar->jvar_jvarvalue_dbt);
    Free(jvar);
}
/***************************************************************/
void jvar_free_jvarrec_consts(struct jvarrec * jvar)
{
/*
** 03/15/2021
*/
    int ii;

    for (ii = 0; ii < jvar->jvar_nconsts; ii++) {
        jvar_free_jvarvalue(jvar->jvar_aconsts[ii]);
    }
    Free(jvar->jvar_avix);
    Free(jvar->jvar_aconsts);
}
/***************************************************************/
#if 0
void jvar_push_vars(struct jrunexec * jx, struct jvarrec * jvar)
{
/*
**
*/
    if (jx->jx_nvars == jx->jx_xvars) {
        jx->jx_xvars += 4;
        jx->jx_avars = Realloc(jx->jx_avars,struct jvarrec *,jx->jx_xvars);
    }
    jx->jx_avars[jx->jx_nvars] = jvar;
    jx->jx_nvars += 1;
}
/***************************************************************/
struct jvarrec * jvar_pop_vars(struct jrunexec * jx)
{
/*
**
*/
    struct jvarrec * jvar;

    if (jx->jx_nvars > 0) {
        jx->jx_nvars -= 1;
        jvar_free_jvarrec(jx->jx_avars[jx->jx_nvars]);
    }

    jvar = NULL;
    if (jx->jx_nvars > 0) {
        jvar = jx->jx_avars[jx->jx_nvars - 1];
    }

    return (jvar);
}
#endif
/***************************************************************/
struct jvarvalue * jvar_find_class_member(struct jrunexec * jx,
    struct jvarvalue * cjvv,
    const char * mbrname)
{
/*
**
*/
    struct jvarvalue *  jvv = NULL;
//    struct jvarvalue ** pjvv;
//    int mnlen;
//
//    jvv = NULL;
//    mnlen = IStrlen(mbrname) + 1;
//
//    /* search class variables */
//    if (cjvv) {
//        if (cjvv->jvv_dtype == JVV_DTYPE_OBJECT) {
//            pjvv = (struct jvarvalue **)dbtree_find(
//                cjvv->jvv_val.jvv_jvvo.jvvo_vars->jvar_jvarvalue_dbt,
//                mbrname, mnlen);
//        } else if (cjvv->jvv_dtype == JVV_DTYPE_INTERNAL_CLASS) {
//            pjvv = (struct jvarvalue **)dbtree_find(
//                cjvv->jvv_val.jvv_jvvi->jvvi_jvar->jvar_jvarvalue_dbt,
//                mbrname, mnlen);
//        }
//        if (pjvv) {
//            jvv = (*pjvv);
//        }
//    }

    return (jvv);
}
/***************************************************************/
#ifdef OLD_WAY
struct jvarvalue * xxxjvar_object_member(struct jrunexec * jx,
    struct jvarvalue * cjvv,
    const char * mbrname)
{
/*
**
*/
    struct jvarvalue *  jvv = NULL;
    struct jvarvalue ** pjvv;
    int mnlen;
    int is_dup;

    jvv = NULL;
    mnlen = IStrlen(mbrname) + 1;

    if (cjvv) {
        if (cjvv->jvv_dtype == JVV_DTYPE_OBJECT) {
            /* look in object */
            pjvv = (struct jvarvalue **)dbtree_find(
                cjvv->jvv_val.jvv_jvvo.jvvo_vars->jvar_jvarvalue_dbt,
                mbrname, mnlen);
            if (pjvv) {
                jvv = (*pjvv);
            } else {
                /* if cannot find in object, look in class */
                jvv = jvar_find_class_member(jx, cjvv->jvv_val.jvv_jvvo.jvvo_class, mbrname);
                if (jvv) {
                    /* if found in class, duplicate and put in object */
                    jvv = jvv_dup_jvarvalue(jx, jvv);
                    is_dup = dbtree_insert(
                        cjvv->jvv_val.jvv_jvvo.jvvo_vars->jvar_jvarvalue_dbt,
                        mbrname, mnlen, jvv);
                }
            }
        }
    }

    return (jvv);
}
#endif
/***************************************************************/
struct jvarvalue * jvar_get_jvv_from_vix(struct jrunexec * jx,
    struct jcontext * jcx,
    int vix)
{
/*
** 03/04/2021
*/
    struct jvarvalue * jvv;

#if IS_DEBUG
    if (vix < 0 || vix >= jcx->jcx_njvv) {
        printf("**DEBUG** Cannot find variable index: %d", vix);
    }
#endif

    if (vix < jcx->jcx_njvv) {
        jvv = jcx->jcx_ajvv[vix];
    } else {
        jvv = NULL;
    }

    return (jvv);
}
/***************************************************************/
struct jcontext * jvar_get_object_jcontext(struct jrunexec * jx,
    struct jvarvalue_object * jvvo)
{
/*
** 03/04/2021
*/
    struct jcontext * jcx = NULL;

    if (jvvo && jvvo->jvvo_class) {
        if (jvvo->jvvo_class->jvv_dtype == JVV_DTYPE_INTERNAL_CLASS) {
            /* (*pjvar) = jvvo->jvvo_class->jvv_val.jvv_jvvi->jvvi_jvar; */
            jcx = jvvo->jvvo_jcx;
        }
    }

    return (jcx);
}
/***************************************************************/
struct jvarvalue * jvar_object_member(struct jrunexec * jx,
    struct jvarvalue * cjvv,
    const char * mbrname)
{
/*
**
*/
    struct jvarvalue *  jvv = NULL;
    struct jcontext * jcx;
    int * pvix;

    jvv = NULL;

    if (cjvv) {
        if (cjvv->jvv_dtype == JVV_DTYPE_OBJECT) {
            jcx = jvar_get_object_jcontext(jx, &(cjvv->jvv_val.jvv_jvvo));
            if (jcx) {
                pvix = jvar_find_in_jvarrec(jcx->jcx_jvar, mbrname);
                if (pvix) {
                    jvv = jvar_get_jvv_from_vix(jx, jcx, *pvix);
                }
            }
        }
    }

    return (jvv);
}
/***************************************************************/
#ifdef OLD_WAY
struct jvarvalue * xxxjvar_find_global_variable(struct jrunexec * jx, const char * varname)
{
/*
**
*/
    struct jvarvalue ** pjvv;
    struct jvarvalue *  jvv;
    struct jvarrec   *  jvar;
    int vnlen;

    jvv = NULL;
    vnlen = IStrlen(varname) + 1;

    /* search global variables */
    jvar = jx->jx_globals;
    while (!jvv && jvar) {
        pjvv = (struct jvarvalue **)dbtree_find(
            jvar->jvar_jvarvalue_dbt, varname, vnlen);
        if (pjvv) {
            jvv = (*pjvv);
        } else {
            jvar = jvar->jvar_prev;
        }
    }

    return (jvv);
}
#endif
/***************************************************************/
int jvar_find_global_varid(struct jrunexec * jx, const char * varname)
{
/*
** 03/02/2021
*/
    int vix;

    vix = VINVALID;

    return (vix);
}
/***************************************************************/
int jvar_find_varid(struct jrunexec * jx, const char * varname)
{
/*
** 03/02/2021
*/
    int varid;

    varid = VINVALID;

    return (varid);
}
// /***************************************************************/
// struct jvarvalue * jvar_get_variable_from_varid(struct jrunexec * jx, int varid)
// {
// /*
// ** 03/02/2021
// */
//     struct jvarvalue *  jvv;
// 
//     jvv = NULL;
// 
//     return (jvv);
// }
/***************************************************************/
#ifdef OLD_WAY
int xxxjvar_insert_new_global_variable(struct jrunexec * jx,
    const char * varname,
    struct jvarvalue * jvv)
{
    int jstat = 0;
    int is_dup;
    struct jvarrec * jvar;

    jvar = jx->jx_globals;
    is_dup = dbtree_insert(jvar->jvar_jvarvalue_dbt,
        varname, IStrlen(varname) + 1, jvv);
    if (is_dup) {
        if (jvv->jvv_dtype == JVV_DTYPE_INTERNAL_CLASS) {
            jstat = jrun_set_error(jx, JSERR_DUP_INTERNAL_CLASS,
                "Duplicate internal class: %s", varname);
        } else {
            jstat = jrun_set_error(jx, JSERR_DUPLICATE_VARIABLE,
                "Duplicate variable: %s", varname);
        }
    }

    return (jstat);
}
#endif
/***************************************************************/
void jvar_free_function_context(struct jcontext * jcx)
{
/*
** 03/07/2021
*/
    int ii;
    int vix;

    /* Initialize constants so they don't get freed */
    for (ii = 0; ii < jcx->jcx_jvar->jvar_nconsts; ii++) {
        vix = jcx->jcx_jvar->jvar_avix[ii];
        jcx->jcx_ajvv[vix] = NULL;
    }

    for (ii = 0; ii < jcx->jcx_jvar->jvar_nvars; ii++) {
        jvar_free_jvarvalue(jcx->jcx_ajvv[ii]);
    }
}
/***************************************************************/
void jvar_free_jcontext(struct jcontext * jcx)
{
/*
** 03/03/2021
*/
    jvar_free_function_context(jcx);

    if (jcx) {
        Free(jcx->jcx_ajvv);
        Free(jcx);
    }
}
/***************************************************************/
struct jcontext * jvar_new_jcontext(struct jrunexec * jx,
    struct jvarrec * jvar,
    struct jcontext * outer_jcx)
{
/*
** 03/03/2021
*/
    struct jcontext * jcx;
    int ii;
    int vix;
#if IS_DEBUG
    static int next_jcx_sn = 0;
#endif

    jcx = New(struct jcontext, 1);
#if IS_DEBUG
    jcx->jcx_sn[0] = 'X';
    jcx->jcx_sn[1] = (next_jcx_sn / 100) % 10 + '0';
    jcx->jcx_sn[2] = (next_jcx_sn /  10) % 10 + '0';
    jcx->jcx_sn[3] = (next_jcx_sn      ) % 10 + '0';
    next_jcx_sn++;
#endif
    jcx->jcx_outer_jcx = outer_jcx;
    jcx->jcx_njvv = jvar->jvar_nvars;
    jcx->jcx_xjvv = jvar->jvar_nvars;
    jcx->jcx_ajvv = NULL;
    jcx->jcx_jvar = jvar;
    if (jcx->jcx_xjvv) jcx->jcx_ajvv = New(struct jvarvalue*, jcx->jcx_xjvv);

    /* Copy constants into jcx->jcx_ajvv */
    for (ii = 0; ii < jvar->jvar_nconsts; ii++) {
        vix = jvar->jvar_avix[ii];
        jcx->jcx_ajvv[vix] = jvar->jvar_aconsts[ii];
    }

    return (jcx);
}
/***************************************************************/
struct jvarrec * jvar_get_global_jvarrec(struct jrunexec * jx)
{
/*
** 03/04/2021
*/

    return (jx->jx_globals);
}
/***************************************************************/
struct jcontext * jvar_get_global_jcontext(struct jrunexec * jx)
{
/*
** 03/03/2021
*/
    struct jcontext * jcx;

    if (!jx->jx_global_jcx) {
        jx->jx_global_jcx = jvar_new_jcontext(jx, jvar_get_global_jvarrec(jx), NULL);
    }
    jcx = jx->jx_global_jcx;

    return (jcx);
}
/***************************************************************/
int jvar_insert_new_global_variable(struct jrunexec * jx,
    const char * varname,
    struct jvarvalue * jvv)
{
    int jstat = 0;
    struct jcontext * jcx;

    jcx = jvar_get_global_jcontext(jx);

    jstat = jvar_new_ad_hoc_variable(jx, jcx, varname, jvv);

    return (jstat);
}
/***************************************************************/
int jvar_add_class_method(
    struct jrunexec * jx,
    struct jvarvalue * cjvv,
    const char * member_name,
    struct jvarvalue * mjvv)

{
/*
**
*/
    int jstat = 0;
    int vix;

    vix = jvar_insert_into_jvarrec(cjvv->jvv_val.jvv_jvvi->jvvi_jvar, member_name);
    if (vix < 0) {
        if (mjvv->jvv_dtype == JVV_DTYPE_INTERNAL_METHOD) {
            jstat = jrun_set_error(jx, JSERR_DUP_INTERNAL_METHOD,
                "Duplicate internal class method: %s", member_name);
        } else {
            jstat = jrun_set_error(jx, JSERR_DUP_CLASS_MEMBER,
                "Duplicate class member: %s", member_name);
        }
        return (jstat);
    } else {
        jvar_new_local_const(cjvv->jvv_val.jvv_jvvi->jvvi_jvar, mjvv, vix);
    }

    return (jstat);
}
/***************************************************************/
#if 0
static int jvar_check_varname(
    struct jrunexec * jx,
    const char * varname)
{
/*
** 01/12/2021
*/
    int jstat = 0;
    int ii;

    if (JTOK_FIRST_ID_CHAR(varname[0])) {
        for (ii = 1; !jstat && varname[ii]; ii++) {
            if (JTOK_ID_CHAR(varname[ii])) {
                /* ok */
            } else {
                jstat = jrun_set_error(jx, JSERR_INVALID_VARNAME,
                    "Invalid character in variable name \'%s\'", varname);
            }
        }
    } else {
        jstat = jrun_set_error(jx, JSERR_INVALID_VARNAME,
            "Invalid first character in variable name \'%s\'", varname);
    }

    return (jstat);
}
#endif
/***************************************************************/
int jvar_is_valid_varname(
    struct jrunexec * jx,
    struct jtoken * jtok)
{
/*
** 02/26/2021
**
** Returns:
**  0 - jtok->jtok_text is valid variable name
**  1 - jtok->jtok_text is an invalid variable name
**  2 - jtok->jtok_text is a reserved word
*/
    int is_valid = 0;

    if (jtok->jtok_typ != JSW_ID) {
        if (jtok->jtok_typ != JSW_KW) {
            is_valid = 1;
        } else {
            if ((jtok->jtok_flags & JSKWI_VAR_OK) == 0) {
                is_valid = 2;
            }
        }
    }

    return (is_valid);
}
/***************************************************************/
int jvar_validate_varname(
    struct jrunexec * jx,
    struct jtoken * jtok)
{
/*
** 01/12/2021
*/
    int jstat = 0;
    int is_valid ;

    is_valid = jvar_is_valid_varname(jx, jtok);

    if (is_valid) {
        if (is_valid == 2) {
            jstat = jrun_set_error(jx, JSERR_INVALID_VARNAME,
                "Reserved word used as identifier. Found \'%s\'", jtok->jtok_text);
        } else {
            jstat = jrun_set_error(jx, JSERR_INVALID_VARNAME,
                "Expecting identifier. Found \'%s\'", jtok->jtok_text);
        }
    }

    return (jstat);
}
/***************************************************************/
int jvar_new_local_variable(struct jrunexec * jx,
    struct jvarvalue_function * jvvf,
    const char * varname)
{
/*
** 02/26/2021
*/
    int vix;

    vix = jvar_insert_into_jvarrec(jvvf->jvvf_vars, varname);
#if IS_DEBUG
    if (vix < 0) {
        printf("**DEBUG** Duplicate variable name: %s", varname);
    }
#endif

    return (vix);
}
/***************************************************************/
#if 0
void xxxjvar_new_local_function(struct jrunexec * jx,
    struct jvarvalue_function * jvvf_parent,
    struct jvarvalue_function * jvvf_child,
    int vix_parent)
{
/*
** 02/26/2021
*/
    char * funcname = jvvf_child->jvvf_func_name;

    if (jvvf_parent->jvvf_nfuncs == jvvf_parent->jvvf_xfuncs) {
        if (!jvvf_parent->jvvf_nfuncs) jvvf_parent->jvvf_xfuncs = 2;
        else                   jvvf_parent->jvvf_xfuncs *= 2;
        jvvf_parent->jvvf_afuncs =
            Realloc(jvvf_parent->jvvf_afuncs, struct jvarvalue_function *, jvvf_parent->jvvf_xfuncs);
    }
    jvvf_parent->jvvf_afuncs[jvvf_parent->jvvf_nfuncs] = jvvf_child;
    jvvf_parent->jvvf_nfuncs += 1;

    jvvf_child->jvvf_vix = vix_parent;
}
#endif
/***************************************************************/
int jvar_new_local_function(struct jrunexec * jx,
    struct jvarvalue_function * jvvf_parent,
    struct jvarvalue          * fjvv_child)
{
/*
** 02/26/2021
*/
    int jstat = 0;
    int vix_parent;

    if (fjvv_child->jvv_dtype != JVV_DTYPE_FUNCTION ||
        !fjvv_child->jvv_val.jvv_val_function->jvvf_func_name) {
        jstat = jrun_set_error(jx, JSERROBJ_SCRIPT_SYNTAX_ERROR,
            "Expecting function name.");
    } else {
        vix_parent = jvar_insert_into_jvarrec(jvvf_parent->jvvf_vars,
                fjvv_child->jvv_val.jvv_val_function->jvvf_func_name);
        if (vix_parent < 0) {
            jstat = jrun_set_error(jx, JSERROBJ_SCRIPT_SYNTAX_ERROR,
                "Duplicate function name. Found: '%s'",
                    fjvv_child->jvv_val.jvv_val_function->jvvf_func_name);
        } else {
            fjvv_child->jvv_val.jvv_val_function->jvvf_vix = vix_parent;

            jvar_new_local_const(jvvf_parent->jvvf_vars, fjvv_child, vix_parent);
        }
    }

    return (jstat);
}
/***************************************************************/
int jvar_find_local_variable_index(struct jrunexec * jx,
    struct jvarvalue_function * jvvf,
    const char * varname)
{
/*
** 02/26/2021
*/
    int vix;

    vix = VINVALID;

    return (vix);
}
/***************************************************************/
struct jvarvalue ** jvar_get_jvv_with_expand(struct jrunexec * jx,
    struct jcontext * jcx,
    int vix)
{
/*
** 03/04/2021
*/
    struct jvarvalue ** pjvv;

    if (vix >= jcx->jcx_xjvv) {
        if (!jcx->jcx_xjvv) jcx->jcx_xjvv = 4;
        while (vix >= jcx->jcx_xjvv) jcx->jcx_xjvv *= 2;
        jcx->jcx_ajvv = Realloc(jcx->jcx_ajvv, struct jvarvalue*, jcx->jcx_xjvv);
        memset(jcx->jcx_ajvv + jcx->jcx_njvv, 0, (jcx->jcx_xjvv - jcx->jcx_njvv) * sizeof(struct jvarvalue*));
    }
    jcx->jcx_njvv = vix + 1;
    pjvv = &(jcx->jcx_ajvv[vix]);

    return (pjvv);
}
/***************************************************************/
int jvar_is_jvv_const(struct jvarvalue * jvv)
{
/*
** 03/16/2021
*/
    int is_const = 0;

    if ((jvv->jvv_dtype == JVV_DTYPE_FUNCTION)          ||
        (jvv->jvv_dtype == JVV_DTYPE_INTERNAL_METHOD)   ||
        (jvv->jvv_dtype == JVV_DTYPE_INTERNAL_CLASS)    ) {
        is_const = 1;
    }

    return (is_const);
}
/***************************************************************/
int jvar_new_ad_hoc_variable(struct jrunexec * jx,
    struct jcontext * jcx,
    const char * varname,
    struct jvarvalue * jvv)
{
/*
** 03/01/2021
*/
    int jstat = 0;
    int * pvix;
    int vix;
    struct jvarvalue ** tgt_jvv;

    pvix = jvar_find_in_jvarrec(jcx->jcx_jvar, varname);
    if (pvix) {
        vix = *pvix;
    } else {
        vix = jvar_insert_into_jvarrec(jcx->jcx_jvar, varname);
    }
    tgt_jvv = jvar_get_jvv_with_expand(jx, jcx, vix);
    if (tgt_jvv) {
        *tgt_jvv = jvv;
        if (jvar_is_jvv_const(jvv)) {
            jvar_new_local_const(jcx->jcx_jvar, jvv, vix);
        }
    }

    return (jstat);
}
/***************************************************************/
struct jvarvalue * jvar_find_local_variable(
    struct jrunexec * jx,
    struct jcontext * local_jcx,
    const char * varname)
{
/*
** 03/02/2021
*/
    struct jvarvalue *  jvv;
    int * pvix;

    jvv = NULL;

    pvix = jvar_find_in_jvarrec(local_jcx->jcx_jvar, varname);
    if (pvix) {
        jvv = jvar_get_jvv_from_vix(jx, local_jcx, *pvix);
    }

    return (jvv);
}
/***************************************************************/
struct jcontext * jvar_get_current_jcontext(struct jrunexec * jx)
{
/*
** 03/04/2021
*/
    struct jfuncstate * jxfs;
    struct jcontext * jcx;

    if (jx->jx_njfs) {
        jxfs = jx->jx_jfs[jx->jx_njfs - 1];
        jcx = jxfs->jfs_jcx;
    } else {
        jcx = jvar_get_global_jcontext(jx);
    }

    return (jcx);
}
/***************************************************************/
struct jfuncstate * jvar_get_current_jfuncstate(struct jrunexec * jx)
{
/*
** 03/18/2021
*/
    struct jfuncstate * jxfs = NULL;

    if (jx->jx_njfs) {
        jxfs = jx->jx_jfs[jx->jx_njfs - 1];
    }

    return (jxfs);
}
/***************************************************************/
struct jvarvalue * jvar_new_jvv_at_vix(struct jrunexec * jx,
    struct jcontext * jcx,
    int vix)
{
/*
** 03/18/2021
*/
    struct jvarvalue * jvv;

#if IS_DEBUG
    if (vix < 0 || vix >= jcx->jcx_njvv) {
        printf("**DEBUG** Cannot find new variable index: %d", vix);
    }
#endif

    if (vix < jcx->jcx_njvv) {
        jvv = jvar_new_jvarvalue();
        jcx->jcx_ajvv[vix] = jvv;
    } else {
        jvv = NULL;
    }

    return (jvv);
}
/***************************************************************/
struct jvarvalue * jvar_find_variable(
    struct jrunexec * jx,
    struct jtoken * jtok,
    struct jcontext ** pvar_jcx)
{
/*
** 03/05/2021
*/
    struct jvarvalue *  jvv = NULL;
    struct jcontext * jcx;
    int * pvix;

    (*pvar_jcx) = NULL;

    jcx = jvar_get_current_jcontext(jx);
    while (jcx && !jvv) {
        pvix = jvar_find_in_jvarrec(jcx->jcx_jvar, jtok->jtok_text);
        if (!pvix) {
            jcx = jcx->jcx_outer_jcx;
        } else {
            (*pvar_jcx) = jcx;
            jvv = jvar_get_jvv_from_vix(jx, jcx, *pvix);
            if (!jvv) {
                jvv = jvar_new_jvv_at_vix(jx, jcx, *pvix);
            }
        }
    }

    return (jvv);
}
/***************************************************************/
