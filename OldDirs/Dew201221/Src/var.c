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

    jvv = New(struct jvarvalue, 1);
    memset(jvv, 0, sizeof(struct jvarvalue));

    return (jvv);
}
/***************************************************************/
struct jvarrec * jvar_new_jvarrec(void)
{
    struct jvarrec * jvar;

    jvar = New(struct jvarrec, 1);
    jvar->jvar_jvarvalue_dbt    = dbtree_new();
    jvar->jvar_prev             = NULL;

    return (jvar);
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
    jvar_free_jvarrec(jvvi->jvvi_jvar);
    Free(jvvi);
}
/***************************************************************/
void jvar_free_jvarvalue_object_data(struct jvarvalue_object * jvvo)
{
/*
**
*/
    jvar_free_jvarrec(jvvo->jvvo_vars);
}
/***************************************************************/
void jvar_free_jvarvalue_data(struct jvarvalue * jvv)
{
/*
**
*/
    switch (jvv->jvv_dtype) {
        case JVV_DTYPE_NONE  :
        case JVV_DTYPE_BOOL  :
        case JVV_DTYPE_JSINT   :
        case JVV_DTYPE_JSFLOAT:
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
            break;
        case JVV_DTYPE_TOKEN :
            Free(jvv->jvv_val.jvv_val_token.jvvt_token);
            break;
        default:
            printf("jvar_free_jvarvalue_data(%s)\n",
                jvar_get_dtype_name(jvv->jvv_dtype));
            break;
    }
    jvv->jvv_dtype = JVV_DTYPE_NONE;
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
static void jvar_store_jvarvalue(
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
            jvvtgt->jvv_val.jvv_int_method = jvvsrc->jvv_val.jvv_int_method;
            break;
        default:
            break;
    }
}
/***************************************************************/
struct jvarvalue * jvv_dup_jvarvalue(struct jvarvalue * jvv)
{
/*
**
*/
    struct jvarvalue * njvv;

    njvv = jvar_new_jvarvalue();
    jvar_store_jvarvalue(njvv, jvv);

    return (njvv);
}
/***************************************************************/
static void jvv_free_jvarvalue(struct jvarvalue * jvv)
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
    dbtree_free(jvar->jvar_jvarvalue_dbt, jvv_free_vjvarvalue);
    Free(jvar);
}
/***************************************************************/
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
static struct jvarrec * jvar_pop_vars(struct jrunexec * jx)
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
/***************************************************************/
struct jvarvalue * jvar_find_class_member(struct jrunexec * jx,
    struct jvarvalue * cjvv,
    const char * mbrname)
{
/*
**
*/
    struct jvarvalue ** pjvv;
    struct jvarvalue *  jvv;
    int mnlen;

    jvv = NULL;
    mnlen = IStrlen(mbrname) + 1;

    /* search class variables */
    if (cjvv) {
        if (cjvv->jvv_dtype == JVV_DTYPE_OBJECT) {
            pjvv = (struct jvarvalue **)dbtree_find(
                cjvv->jvv_val.jvv_jvvo.jvvo_vars->jvar_jvarvalue_dbt,
                mbrname, mnlen);
        } else if (cjvv->jvv_dtype == JVV_DTYPE_INTERNAL_CLASS) {
            pjvv = (struct jvarvalue **)dbtree_find(
                cjvv->jvv_val.jvv_jvvi->jvvi_jvar->jvar_jvarvalue_dbt,
                mbrname, mnlen);
        }
        if (pjvv) {
            jvv = (*pjvv);
        }
    }

    return (jvv);
}
/***************************************************************/
struct jvarvalue * jvar_object_member(struct jrunexec * jx,
    struct jvarvalue * cjvv,
    const char * mbrname)
{
/*
**
*/
    struct jvarvalue ** pjvv;
    struct jvarvalue *  jvv;
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
                    jvv = jvv_dup_jvarvalue(jvv);
                    is_dup = dbtree_insert(
                        cjvv->jvv_val.jvv_jvvo.jvvo_vars->jvar_jvarvalue_dbt,
                        mbrname, mnlen, jvv);
                }
            }
        }
    }

    return (jvv);
}
/***************************************************************/
struct jvarvalue * jvar_find_variable_cascading(struct jrunexec * jx,
    const char * varname)
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

    /* search current variables */
    if (!jvv && jx->jx_nvars > 1) {
        jvar = jx->jx_avars[jx->jx_nvars - 1];
        while (!jvv && jvar) {
            pjvv = (struct jvarvalue **)dbtree_find(
                jvar->jvar_jvarvalue_dbt, varname, vnlen);
            if (pjvv) {
                jvv = (*pjvv);
            } else {
                jvar = jvar->jvar_prev;
            }
        }
    }

    /* search global variables - before execution */
    if (!jvv && jx->jx_nvars > JX_GLOBALS_IX) {
        jvar = jx->jx_avars[JX_GLOBALS_IX];
        while (!jvv && jvar) {
            pjvv = (struct jvarvalue **)dbtree_find(
                jvar->jvar_jvarvalue_dbt, varname, vnlen);
            if (pjvv) {
                jvv = (*pjvv);
            } else {
                jvar = jvar->jvar_prev;
            }
        }
    }

    return (jvv);
}
/***************************************************************/
struct jvarvalue * jvar_find_variable(struct jrunexec * jx,
    const char * varname)
{
/*
**
*/
    struct jvarvalue *  jvv;

    jvv = jvar_find_variable_cascading(jx, varname);

    return (jvv);
}
/***************************************************************/
int jvar_add_jvarvalue(struct jrunexec * jx,
    const char * varname,
    struct jvarvalue * jvv)
{
    int jstat = 0;
    int is_dup;
    struct jvarrec * jvar;

    if (jx->jx_nvars) {
        jvar = jx->jx_avars[jx->jx_nvars - 1];
        is_dup = dbtree_insert(
            jvar->jvar_jvarvalue_dbt, varname, IStrlen(varname) + 1, jvv);
            if (is_dup) {
                if (jvv->jvv_dtype == JVV_DTYPE_INTERNAL_CLASS) {
                    jstat = jrun_set_error(jx, JSERR_DUP_INTERNAL_CLASS,
                        "Duplicate internal class: %s", varname);
                } else {
                    jstat = jrun_set_error(jx, JSERR_DUPLICATE_VARIABLE,
                        "Duplicate variable: %s", varname);
                }
            }
    }

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
    int is_dup;
    int mnlen;

    mnlen = IStrlen(member_name) + 1;

    is_dup = dbtree_insert(cjvv->jvv_val.jvv_jvvi->jvvi_jvar->jvar_jvarvalue_dbt,
        member_name, mnlen, mjvv);
    if (is_dup) {
        if (mjvv->jvv_dtype == JVV_DTYPE_INTERNAL_METHOD) {
            jstat = jrun_set_error(jx, JSERR_DUP_INTERNAL_METHOD,
                "Duplicate internal class method: %s", member_name);
        } else {
            jstat = jrun_set_error(jx, JSERR_DUP_CLASS_MEMBER,
                "Duplicate class member: %s", member_name);
        }
        return (jstat);
    }

    return (jstat);
}
/***************************************************************/
