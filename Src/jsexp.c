/***************************************************************/
/*
**  jsexp.c --  JavaScript expressions
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
#include "stmt.h"
#include "dbtreeh.h"
#include "jstok.h"
#include "jsexp.h"
#include "dbtreeh.h"
#include "snprfh.h"
#include "utl.h"
#include "var.h"
#include "internal.h"
#include "prep.h"
#include "print.h"
#include "array.h"
#include "obj.h"

#define DEBUG_PRINT_EXP_STACK   0
#define DEBUG_PRINT_EVAL_RESULT 0
#define DEBUG_ASSIGN            0

#define COMPARE(x,y) (((x)==(y))?0:(((x)<(y))?-1:1))
#define BOOLVAL(b) (((b)==0)?0:1)

/***************************************************************/
/* forard */
static int jexp_eval(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    struct jvarvalue * jvv);

/***************************************************************/
static int jvar_chars_to_number(struct jvarvalue * jvvtgt, const char * jchars)
{
/*
**
*/
    int cstat = 0;
    int intval;
    double dblval;

    cstat = convert_string_to_number(jchars, &intval, &dblval);
    if (cstat == 1) {
        jvar_store_jsint(jvvtgt, intval);
        cstat = 0;
    } else if (cstat == 2) {
        jvar_store_jsfloat(jvvtgt, dblval);
        cstat = 0;
    } else if (cstat >= 0) {
        cstat = -3;
    }

    return (cstat);
}
/***************************************************************/
#if USE_JVV_CHARS_POINTER
void jvar_store_chars_len(
    struct jvarvalue * jvvtgt,
    const char * jchars,
    int jcharslen)
{
/*
** 03/15/2022
*/
    if (jvvtgt->jvv_dtype != JVV_DTYPE_CHARS) {
        jvar_free_jvarvalue_data(jvvtgt);
        jvvtgt->jvv_dtype = JVV_DTYPE_CHARS;
        jvvtgt->jvv_val.jvv_val_chars = jvar_new_jvarvalue_chars();
        jvvtgt->jvv_val.jvv_val_chars->jvvc_size = jcharslen + JVARVALUE_CHARS_MIN;
        jvvtgt->jvv_val.jvv_val_chars->jvvc_val_chars =
            New(char, jvvtgt->jvv_val.jvv_val_chars->jvvc_size);
    } else if (jcharslen + 1 >= jvvtgt->jvv_val.jvv_val_chars->jvvc_size) {
        jvvtgt->jvv_val.jvv_val_chars->jvvc_size = jcharslen + JVARVALUE_CHARS_MIN;
        jvvtgt->jvv_val.jvv_val_chars->jvvc_val_chars =
                Realloc(jvvtgt->jvv_val.jvv_val_chars->jvvc_val_chars, char,\
                 jvvtgt->jvv_val.jvv_val_chars->jvvc_size);
    }

    memcpy(jvvtgt->jvv_val.jvv_val_chars->jvvc_val_chars, jchars, jcharslen);
    jvvtgt->jvv_val.jvv_val_chars->jvvc_val_chars[jcharslen] = '\0';
    jvvtgt->jvv_val.jvv_val_chars->jvvc_length = jcharslen;
}
#else
void jvar_store_chars_len(
    struct jvarvalue * jvvtgt,
    const char * jchars,
    int jcharslen)
{
/*
**
*/
    if (jvvtgt->jvv_dtype != JVV_DTYPE_CHARS) {
        jvar_free_jvarvalue_data(jvvtgt);
        jvvtgt->jvv_dtype = JVV_DTYPE_CHARS;
        jvvtgt->jvv_val.jvv_val_chars.jvvc_size = jcharslen + JVARVALUE_CHARS_MIN;
        jvvtgt->jvv_val.jvv_val_chars.jvvc_val_chars =
            New(char, jvvtgt->jvv_val.jvv_val_chars.jvvc_size);
    } else if (jcharslen + 1 >= jvvtgt->jvv_val.jvv_val_chars.jvvc_size) {
        jvvtgt->jvv_val.jvv_val_chars.jvvc_size = jcharslen + JVARVALUE_CHARS_MIN;
        jvvtgt->jvv_val.jvv_val_chars.jvvc_val_chars =
                Realloc(jvvtgt->jvv_val.jvv_val_chars.jvvc_val_chars, char,\
                 jvvtgt->jvv_val.jvv_val_chars.jvvc_size);
    }

    memcpy(jvvtgt->jvv_val.jvv_val_chars.jvvc_val_chars, jchars, jcharslen);
    jvvtgt->jvv_val.jvv_val_chars.jvvc_val_chars[jcharslen] = '\0';
    jvvtgt->jvv_val.jvv_val_chars.jvvc_length = jcharslen;
}
#endif
/***************************************************************/
void jvar_store_lval(
    struct jrunexec * jx,
    struct jvarvalue * jvvtgt,
    struct jvarvalue * jvvsrc,
    struct jcontext  * var_jcx)
{
/*
**
*/
    if (jvvsrc->jvv_dtype == JVV_DTYPE_FUNCTION) {
        jvvtgt->jvv_dtype = JVV_DTYPE_FUNCVAR;
        jvvtgt->jvv_val.jvv_val_funcvar.jvvfv_jvvf = jvvsrc->jvv_val.jvv_val_function;
        jvvtgt->jvv_val.jvv_val_funcvar.jvvfv_var_jcx = var_jcx;
        INCFUNCREFS(jvvsrc->jvv_val.jvv_val_function);
    } else if (jvvsrc->jvv_dtype == JVV_DTYPE_OBJPTR) {
            switch (jvvsrc->jvv_val.jvv_objptr.jvvp_pval->jvv_dtype) {
                case JVV_DTYPE_ARRAY   :
                    jvvtgt->jvv_dtype = JVV_DTYPE_LVAL;
                    jvvtgt->jvv_val.jvv_lval.jvvv_lval    = jvvsrc;
                    jvvtgt->jvv_val.jvv_lval.jvvv_var_jcx = NULL;
#if LVAL_PARENT
                    jvvtgt->jvv_val.jvv_lval.jvvv_parent  = NULL;
#endif
#if FIX_220406
                    jvvtgt->jvv_val.jvv_lval.jvvv_jvvb    = NULL;
#endif
#if FIX_220714
                    jvvtgt->jvv_val.jvv_lval.jvvv_jvvb    = NULL;
#endif
                    break;
#if FIX_220714
                case JVV_DTYPE_OBJECT   :
                    jvvtgt->jvv_dtype = JVV_DTYPE_LVAL;
                    jvvtgt->jvv_val.jvv_objptr.jvvp_pval  = jvvsrc;
                    jvvtgt->jvv_val.jvv_lval.jvvv_var_jcx = NULL;
                    jvvtgt->jvv_val.jvv_lval.jvvv_jvvb    = NULL;
                    break;
#else
                case JVV_DTYPE_OBJECT   :
                    jvvtgt->jvv_dtype = JVV_DTYPE_LVAL;
                    jvvtgt->jvv_val.jvv_lval.jvvv_lval    = jvvsrc;
                    jvvtgt->jvv_val.jvv_lval.jvvv_var_jcx = NULL;
#if LVAL_PARENT
                    jvvtgt->jvv_val.jvv_lval.jvvv_parent  = NULL;
#endif
#if FIX_220406
                    jvvtgt->jvv_val.jvv_lval.jvvv_jvvb    = NULL;
#endif
                    break;
#endif
                default:
                    break;
            }
    } else {
        jvvtgt->jvv_dtype = JVV_DTYPE_LVAL;
        jvvtgt->jvv_val.jvv_lval.jvvv_lval    = jvvsrc;
        jvvtgt->jvv_val.jvv_lval.jvvv_var_jcx = NULL;
#if LVAL_PARENT
        jvvtgt->jvv_val.jvv_lval.jvvv_parent  = NULL;
#endif
#if FIX_220406
        jvvtgt->jvv_val.jvv_lval.jvvv_jvvb    = NULL;
#endif
#if FIX_220714
        jvvtgt->jvv_val.jvv_lval.jvvv_jvvb    = NULL;
#endif
    }
}
/***************************************************************/
static void jvar_store_chars(
    struct jvarvalue * jvvtgt,
    const char * jchars)
{
/*
**
*/
    jvar_store_chars_len(jvvtgt, jchars, IStrlen(jchars));
}
/***************************************************************/
#if USE_JVV_CHARS_POINTER
static void jvar_cat_chars(
    struct jvarvalue * jvvtgt,
    struct jvarvalue_chars * jvv_chars1,
    struct jvarvalue_chars * jvv_chars2)
{
/*
** 03/15/2022
*/
    int tlen;

    tlen = jvv_chars1->jvvc_length + jvv_chars2->jvvc_length;
    if (jvvtgt->jvv_dtype != JVV_DTYPE_CHARS) {
        jvar_free_jvarvalue_data(jvvtgt);
        jvvtgt->jvv_dtype = JVV_DTYPE_CHARS;
        jvvtgt->jvv_val.jvv_val_chars->jvvc_size = tlen + JVARVALUE_CHARS_MIN;
        jvvtgt->jvv_val.jvv_val_chars->jvvc_val_chars =
            New(char, jvvtgt->jvv_val.jvv_val_chars->jvvc_size);
    } else if (tlen + 1 >= jvvtgt->jvv_val.jvv_val_chars->jvvc_size) {
        jvvtgt->jvv_val.jvv_val_chars->jvvc_size = tlen + JVARVALUE_CHARS_MIN;
        jvvtgt->jvv_val.jvv_val_chars->jvvc_val_chars =
            Realloc(jvvtgt->jvv_val.jvv_val_chars->jvvc_val_chars, char, \
                jvvtgt->jvv_val.jvv_val_chars->jvvc_size);
    }

    memcpy(jvvtgt->jvv_val.jvv_val_chars->jvvc_val_chars,
           jvv_chars1->jvvc_val_chars, jvv_chars1->jvvc_length);
    memcpy(jvvtgt->jvv_val.jvv_val_chars->jvvc_val_chars + jvv_chars1->jvvc_length,
           jvv_chars2->jvvc_val_chars, jvv_chars2->jvvc_length);
    jvvtgt->jvv_val.jvv_val_chars->jvvc_val_chars[tlen] = '\0';
    jvvtgt->jvv_val.jvv_val_chars->jvvc_length = tlen;
}
#else
static void jvar_cat_chars(
    struct jvarvalue * jvvtgt,
    struct jvarvalue_chars * jvv_chars1,
    struct jvarvalue_chars * jvv_chars2)
{
/*
**
*/
    int tlen;

    tlen = jvv_chars1->jvvc_length + jvv_chars2->jvvc_length;
    if (jvvtgt->jvv_dtype != JVV_DTYPE_CHARS) {
        jvar_free_jvarvalue_data(jvvtgt);
        jvvtgt->jvv_dtype = JVV_DTYPE_CHARS;
        jvvtgt->jvv_val.jvv_val_chars.jvvc_size = tlen + JVARVALUE_CHARS_MIN;
        jvvtgt->jvv_val.jvv_val_chars.jvvc_val_chars =
            New(char, jvvtgt->jvv_val.jvv_val_chars.jvvc_size);
    } else if (tlen + 1 >= jvvtgt->jvv_val.jvv_val_chars.jvvc_size) {
        jvvtgt->jvv_val.jvv_val_chars.jvvc_size = tlen + JVARVALUE_CHARS_MIN;
        jvvtgt->jvv_val.jvv_val_chars.jvvc_val_chars =
            Realloc(jvvtgt->jvv_val.jvv_val_chars.jvvc_val_chars, char, \
                jvvtgt->jvv_val.jvv_val_chars.jvvc_size);
    }

    memcpy(jvvtgt->jvv_val.jvv_val_chars.jvvc_val_chars,
           jvv_chars1->jvvc_val_chars, jvv_chars1->jvvc_length);
    memcpy(jvvtgt->jvv_val.jvv_val_chars.jvvc_val_chars + jvv_chars1->jvvc_length,
           jvv_chars2->jvvc_val_chars, jvv_chars2->jvvc_length);
    jvvtgt->jvv_val.jvv_val_chars.jvvc_val_chars[tlen] = '\0';
    jvvtgt->jvv_val.jvv_val_chars.jvvc_length = tlen;
}
#endif
/***************************************************************/
static int jvarvalue_to_chars(
    struct jrunexec * jx,
    struct jvarvalue * jvvtgt,
    struct jvarvalue * jvvsrc)
{
/*
**
*/
    int jstat = 0;
    int jfmtflags;
    char * prbuf = NULL;
    int prlen = 0;
    int prmax = 0;

    prbuf = NULL;
    prlen = 0;
    prmax = 0;
    jfmtflags = JFMT_ORIGIN_TOCHARS;

    jstat = jpr_jvarvalue_tostring(jx, &prbuf, &prlen, &prmax, jvvsrc, jfmtflags);
    if (!jstat) {
        jvar_store_chars_len(jvvtgt, prbuf, prlen);
        Free(prbuf);
    }

    return (jstat);
}
/***************************************************************/
static int jexp_pre_increment(struct jrunexec * jx,
         struct jvarvalue         * jvvans,
         struct jvarvalue         * ajvv1)
{
/*
** 02/22/2022
**
** Pre increment (e.g. ++ii)
*/
    int jstat = 0;
    int ans;
    JSFLOAT fans;
    struct jvarvalue * jvv1;

    if (ajvv1->jvv_dtype != JVV_DTYPE_LVAL) {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_OPERATOR_REQUIRES_LVAL,
            "Increment operator requires L-value");
    } else {
        jvv1 = ajvv1->jvv_val.jvv_lval.jvvv_lval;
        switch (jvv1->jvv_dtype) {
            case JVV_DTYPE_BOOL  :
                ans = (jvv1->jvv_val.jvv_val_bool == 0) ? 1 : 2;
                jvar_store_jsint(jvvans, ans);
                jvar_store_jsint(jvv1, ans);
                break;

            case JVV_DTYPE_JSINT   :
                ans = jvv1->jvv_val.jvv_val_jsint + 1;
                jvar_store_jsint(jvvans, ans);
                jvar_store_jsint(jvv1, ans);
                break;

            case JVV_DTYPE_JSFLOAT:
                fans = jvv1->jvv_val.jvv_val_jsfloat + 1.0;
                jvar_store_jsfloat(jvvans, fans);
                jvar_store_jsfloat(jvv1, fans);
                break;

            default:
                jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_OPERATOR_TYPE_MISMATCH,
                    "Illegal prefix increment");
                break;
        }
    }

    return (jstat);
}
/***************************************************************/
static int jexp_post_increment(struct jrunexec * jx, struct jvarvalue * ajvv1)
{
/*
** 02/23/2022
**
** Post increment (e.g. ii++)
*/
    int jstat = 0;
    int ans;
    JSFLOAT fans;
    struct jvarvalue * jvv1;

    if (ajvv1->jvv_dtype != JVV_DTYPE_LVAL) {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_OPERATOR_REQUIRES_LVAL,
            "Increment operator requires L-value");
    } else {
        jvv1 = ajvv1->jvv_val.jvv_lval.jvvv_lval;
        switch (jvv1->jvv_dtype) {
            case JVV_DTYPE_BOOL  :
                ans = (jvv1->jvv_val.jvv_val_bool == 0) ? 0 : 1;
                jvar_store_jsint(ajvv1, ans);
                ans += 1;
                jvar_store_jsint(jvv1, ans);
                break;

            case JVV_DTYPE_JSINT   :
                ans = jvv1->jvv_val.jvv_val_jsint;
                jvar_store_jsint(ajvv1, ans);
                ans += 1;
                jvar_store_jsint(jvv1, ans);
                break;

            case JVV_DTYPE_JSFLOAT:
                fans = jvv1->jvv_val.jvv_val_jsfloat;
                jvar_store_jsfloat(ajvv1, fans);
                fans += 1.0;
                jvar_store_jsfloat(jvv1, fans);
                break;

            default:
                jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_OPERATOR_TYPE_MISMATCH,
                    "Illegal postfix increment");
                break;
        }
    }

    return (jstat);
}
/***************************************************************/
static int jexp_unop_neg(struct jrunexec * jx,
         struct jvarvalue         * jvvans,
         struct jvarvalue         * ajvv1)
{
/*
**
enum e_jvv_type {
    JVV_DTYPE_NONE = 0          ,
    JVV_DTYPE_BOOL              ,
    JVV_DTYPE_JSINT             ,
    JVV_DTYPE_JSFLOAT           ,
    JVV_DTYPE_CHARS             ,
    JVV_DTYPE_NUMBER            ,
    JVV_ZZZZ     
};
struct jvarvalue {
    enum e_jvv_type    jvv_dtype;
    union {
        int     jvv_val_bool;
        JSINT   jvv_val_jsint;
        JSFLOAT jvv_val_jsfloat;
        void *  jvv_void;
        struct jvarvalue_chars jvv_val_chars;
        struct jvarvalue_chars jvv_val_number;
    } jv_val;
};
*/
    int jstat = 0;
    struct jvarvalue * jvv1;

    jstat = jexp_get_rval(jx, &jvv1, ajvv1);
    if (jstat) return (jstat);

    switch (jvv1->jvv_dtype) {
        case JVV_DTYPE_JSINT   :
            jvar_store_jsint(jvvans,
                - jvv1->jvv_val.jvv_val_jsint);
            break;

        case JVV_DTYPE_JSFLOAT:
            jvar_store_jsfloat(jvvans,
                - jvv1->jvv_val.jvv_val_jsfloat);
            break;

        default:
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_OPERATOR_TYPE_MISMATCH,
                "Illegal negation");
            break;
    }

    return (jstat);
}
/***************************************************************/
static int jexp_unop_lognot(struct jrunexec * jx,
         struct jvarvalue         * jvvans,
         struct jvarvalue         * ajvv1)
{
/*
** Logical NOT
*/
    int jstat = 0;
    struct jvarvalue * jvv1;

    jstat = jexp_get_rval(jx, &jvv1, ajvv1);
    if (jstat) return (jstat);

    switch (jvv1->jvv_dtype) {
        case JVV_DTYPE_BOOL  :
            jvar_store_bool(jvvans,
                (jvv1->jvv_val.jvv_val_bool==0)?1:0);
            break;

        case JVV_DTYPE_JSINT   :
            jvar_store_jsint(jvvans,
                (jvv1->jvv_val.jvv_val_jsint==0)?1:0);
            break;

        case JVV_DTYPE_JSFLOAT:
            jvar_store_jsfloat(jvvans,
                (jvv1->jvv_val.jvv_val_jsfloat==0)?1:0);
            break;

        default:
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_OPERATOR_TYPE_MISMATCH,
                "Illegal positive");
            break;
    }

    return (jstat);
}
/***************************************************************/
static int jexp_unop_pos(struct jrunexec * jx,
         struct jvarvalue         * jvvans,
         struct jvarvalue         * ajvv1)
{
/*
**
*/
    int jstat = 0;
    struct jvarvalue * jvv1;

    jstat = jexp_get_rval(jx, &jvv1, ajvv1);
    if (jstat) return (jstat);

    switch (jvv1->jvv_dtype) {
        case JVV_DTYPE_JSINT   :
            jvar_store_jsint(jvvans,
                + jvv1->jvv_val.jvv_val_jsint);
            break;

        case JVV_DTYPE_JSFLOAT:
            jvar_store_jsfloat(jvvans,
                + jvv1->jvv_val.jvv_val_jsfloat);
            break;

        default:
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_OPERATOR_TYPE_MISMATCH,
                "Illegal positive");
            break;
    }

    return (jstat);
}
/***************************************************************/
int jexp_istrue(struct jrunexec * jx,
         struct jvarvalue         * jvv1,
         int * istrue)
{
/*
**
*/
    int jstat = 0;

    (*istrue) = 0;

    switch (jvv1->jvv_dtype) {
        case JVV_DTYPE_BOOL  :
            if (jvv1->jvv_val.jvv_val_bool != 0) (*istrue) = 1;
            break;

        case JVV_DTYPE_JSINT   :
            if (jvv1->jvv_val.jvv_val_jsint != 0) (*istrue) = 1;
            break;

        case JVV_DTYPE_JSFLOAT:
            if (jvv1->jvv_val.jvv_val_jsfloat != 0) (*istrue) = 1;
            break;

        case JVV_DTYPE_LVAL:
            jstat = jexp_istrue(jx, jvv1->jvv_val.jvv_lval.jvvv_lval, istrue);
            break;

        default:
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_OPERATOR_TYPE_MISMATCH,
                "Illegal boolean");
            break;
    }

    return (jstat);
}
/***************************************************************/
static int jexp_binop_logor(struct jrunexec * jx,
    struct jvarvalue * ajvv1,
    struct jvarvalue * ajvv2)
{
/*
**
*/
    int jstat = 0;
    int istrue;
    struct jvarvalue * jvv1;
    struct jvarvalue * jvv2;

    jstat = jexp_get_rval(jx, &jvv1, ajvv1);
    if (jstat) return (jstat);
    jstat = jexp_get_rval(jx, &jvv2, ajvv2);
    if (jstat) return (jstat);

    jstat = jexp_istrue(jx, jvv1, &istrue);
    if (!jstat) {
        if (!istrue) {
            jstat = jexp_istrue(jx, jvv2, &istrue);
            if (!jstat) {
                jvar_store_bool(jvv1, (istrue)?1:0);
            }
        } else {
            jvar_store_bool(jvv1, 1);
        }
    }

    return (jstat);
}
/***************************************************************/
static int jexp_binop_logand(struct jrunexec * jx,
    struct jvarvalue * ajvv1,
    struct jvarvalue * ajvv2)
{
/*
**
*/
    int jstat = 0;
    int istrue;
    struct jvarvalue * jvv1;
    struct jvarvalue * jvv2;

    jstat = jexp_get_rval(jx, &jvv1, ajvv1);
    if (jstat) return (jstat);
    jstat = jexp_get_rval(jx, &jvv2, ajvv2);
    if (jstat) return (jstat);

    jstat = jexp_istrue(jx, jvv1, &istrue);
    if (!jstat) {
        if (istrue) {
            jstat = jexp_istrue(jx, jvv2, &istrue);
            if (!jstat) {
                jvar_store_bool(jvv1, (istrue)?1:0);
            }
        } else {
            jvar_store_bool(jvv1, 0);
        }
    }

    return (jstat);
}
/***************************************************************/
static int jexp_binop_add(struct jrunexec * jx,
    struct jvarvalue * ajvv1,
    struct jvarvalue * ajvv2)
{
/*
**
enum e_jvv_type {
    JVV_DTYPE_NONE = 0          ,
    JVV_DTYPE_BOOL              ,
    JVV_DTYPE_JSINT             ,
    JVV_DTYPE_JSFLOAT           ,
    JVV_DTYPE_CHARS             ,
    JVV_DTYPE_NUMBER            ,
    JVV_ZZZZ     
};
struct jvarvalue {
    enum e_jvv_type    jvv_dtype;
    union {
        int     jvv_val_bool;
        JSINT   jvv_val_jsint;
        JSFLOAT jvv_val_jsfloat;
        void *  jvv_void;
        struct jvarvalue_chars jvv_val_chars;
        struct jvarvalue_chars jvv_val_number;
    } jv_val;
};
*/
    int jstat = 0;
    struct jvarvalue jvvchars;
    struct jvarvalue * jvv1;
    struct jvarvalue * jvv2;

    jstat = jexp_get_rval(jx, &jvv1, ajvv1);
    if (jstat) return (jstat);
    jstat = jexp_get_rval(jx, &jvv2, ajvv2);
    if (jstat) return (jstat);

    switch (jvv1->jvv_dtype) {
        case JVV_DTYPE_JSINT   :
            switch (jvv2->jvv_dtype) {
                case JVV_DTYPE_JSINT   :
                    jvar_store_jsint(ajvv1,
                        jvv1->jvv_val.jvv_val_jsint +
                        jvv2->jvv_val.jvv_val_jsint);
                    break;
                case JVV_DTYPE_JSFLOAT   :
                    jvar_store_jsfloat(ajvv1,
                        (JSFLOAT)jvv1->jvv_val.jvv_val_jsint +
                        jvv2->jvv_val.jvv_val_jsfloat);
                    break;
                case JVV_DTYPE_CHARS   :
                    INIT_JVARVALUE(&(jvvchars));
                    jstat = jvarvalue_to_chars(jx, &jvvchars, jvv1);
                    if (!jstat) {
#if USE_JVV_CHARS_POINTER
                        jvar_cat_chars(ajvv1,
                            jvvchars.jvv_val.jvv_val_chars,
                            jvv2->jvv_val.jvv_val_chars);
#else
                        jvar_cat_chars(ajvv1,
                            &(jvvchars.jvv_val.jvv_val_chars),
                            &(jvv2->jvv_val.jvv_val_chars));
#endif
                        jvar_free_jvarvalue_data(&jvvchars);
                    }
                    break;
                default:
                    jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_OPERATOR_TYPE_MISMATCH,
                        "Illegal integer addition");
                    break;
            }
            break;

        case JVV_DTYPE_JSFLOAT:
            switch (jvv2->jvv_dtype) {
                case JVV_DTYPE_JSINT   :
                    jvar_store_jsfloat(ajvv1,
                        jvv1->jvv_val.jvv_val_jsfloat +
                        (JSFLOAT)jvv2->jvv_val.jvv_val_jsint);
                    break;
                case JVV_DTYPE_JSFLOAT   :
                    jvar_store_jsfloat(ajvv1,
                        jvv1->jvv_val.jvv_val_jsfloat +
                        jvv2->jvv_val.jvv_val_jsfloat);
                    break;
                case JVV_DTYPE_CHARS   :
                    INIT_JVARVALUE(&(jvvchars));
                    jstat = jvarvalue_to_chars(jx, &jvvchars, jvv1);
                    if (!jstat) {
#if USE_JVV_CHARS_POINTER
                        jvar_cat_chars(ajvv1,
                            jvvchars.jvv_val.jvv_val_chars,
                            jvv2->jvv_val.jvv_val_chars);
#else
                        jvar_cat_chars(ajvv1,
                            &(jvvchars.jvv_val.jvv_val_chars),
                            &(jvv2->jvv_val.jvv_val_chars));
#endif
                        jvar_free_jvarvalue_data(&jvvchars);
                    }
                    break;
                default:
                    jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_OPERATOR_TYPE_MISMATCH,
                        "Illegal floating addition");
                    break;
            }
            break;

        case JVV_DTYPE_CHARS :
            INIT_JVARVALUE(&(jvvchars));
            jstat = jvarvalue_to_chars(jx, &jvvchars, jvv2);
            if (!jstat) {
#if USE_JVV_CHARS_POINTER
                jvar_cat_chars(ajvv1,
                    jvv1->jvv_val.jvv_val_chars,
                    jvvchars.jvv_val.jvv_val_chars);
#else
                jvar_cat_chars(ajvv1,
                    &(jvv1->jvv_val.jvv_val_chars),
                    &(jvvchars.jvv_val.jvv_val_chars));
#endif
                jvar_free_jvarvalue_data(&jvvchars);
                jvar_free_jvarvalue_data(ajvv2);
            }
            break;

        default:
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_OPERATOR_TYPE_MISMATCH,
                "Illegal addition");
            break;
    }

    return (jstat);
}
/***************************************************************/
static int jexp_binop_sub(struct jrunexec * jx,
    struct jvarvalue * ajvv1,
    struct jvarvalue * ajvv2)
{
/*
**
*/
    int jstat = 0;
    struct jvarvalue * jvv1;
    struct jvarvalue * jvv2;

    jstat = jexp_get_rval(jx, &jvv1, ajvv1);
    if (jstat) return (jstat);
    jstat = jexp_get_rval(jx, &jvv2, ajvv2);
    if (jstat) return (jstat);

    switch (jvv1->jvv_dtype) {
        case JVV_DTYPE_JSINT   :
            switch (jvv2->jvv_dtype) {
                case JVV_DTYPE_JSINT   :
                    jvar_store_jsint(ajvv1,
                        jvv1->jvv_val.jvv_val_jsint -
                        jvv2->jvv_val.jvv_val_jsint);
                    break;
                case JVV_DTYPE_JSFLOAT   :
                    jvar_store_jsfloat(ajvv1,
                        (JSFLOAT)jvv1->jvv_val.jvv_val_jsint -
                        jvv2->jvv_val.jvv_val_jsfloat);
                    break;
                default:
                    jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_OPERATOR_TYPE_MISMATCH,
                        "Illegal integer subtraction");
                    break;
            }
            break;

        case JVV_DTYPE_JSFLOAT:
            switch (jvv2->jvv_dtype) {
                case JVV_DTYPE_JSINT   :
                    jvar_store_jsfloat(ajvv1,
                        jvv1->jvv_val.jvv_val_jsfloat -
                        (JSFLOAT)jvv2->jvv_val.jvv_val_jsint);
                    break;
                case JVV_DTYPE_JSFLOAT   :
                    jvar_store_jsfloat(ajvv1,
                        jvv1->jvv_val.jvv_val_jsfloat -
                        jvv2->jvv_val.jvv_val_jsfloat);
                    break;
                default:
                    jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_OPERATOR_TYPE_MISMATCH,
                        "Illegal floating subtraction");
                    break;
            }
            break;

        default:
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_OPERATOR_TYPE_MISMATCH,
                "Illegal subtraction");
            break;
    }

    return (jstat);
}
/***************************************************************/
static int jexp_binop_mul(struct jrunexec * jx,
    struct jvarvalue * ajvv1,
    struct jvarvalue * ajvv2)
{
/*
**
*/
    int jstat = 0;
    struct jvarvalue * jvv1;
    struct jvarvalue * jvv2;

    jstat = jexp_get_rval(jx, &jvv1, ajvv1);
    if (jstat) return (jstat);
    jstat = jexp_get_rval(jx, &jvv2, ajvv2);
    if (jstat) return (jstat);

    switch (jvv1->jvv_dtype) {
        case JVV_DTYPE_JSINT   :
            switch (jvv2->jvv_dtype) {
                case JVV_DTYPE_JSINT   :
                    jvar_store_jsint(ajvv1,
                        jvv1->jvv_val.jvv_val_jsint *
                        jvv2->jvv_val.jvv_val_jsint);
                    break;
                case JVV_DTYPE_JSFLOAT   :
                    jvar_store_jsfloat(ajvv1,
                        (JSFLOAT)jvv1->jvv_val.jvv_val_jsint *
                        jvv2->jvv_val.jvv_val_jsfloat);
                    break;
                default:
                    jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_OPERATOR_TYPE_MISMATCH,
                        "Illegal integer multiplication");
                    break;
            }
            break;

        case JVV_DTYPE_JSFLOAT:
            switch (jvv2->jvv_dtype) {
                case JVV_DTYPE_JSINT   :
                    jvar_store_jsfloat(ajvv1,
                        jvv1->jvv_val.jvv_val_jsfloat *
                        (JSFLOAT)jvv2->jvv_val.jvv_val_jsint);
                    break;
                case JVV_DTYPE_JSFLOAT   :
                    jvar_store_jsfloat(ajvv1,
                        jvv1->jvv_val.jvv_val_jsfloat *
                        jvv2->jvv_val.jvv_val_jsfloat);
                    break;
                default:
                    jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_OPERATOR_TYPE_MISMATCH,
                        "Illegal floating multiplication");
                    break;
            }
            break;

        default:
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_OPERATOR_TYPE_MISMATCH,
                "Illegal multiplication");
            break;
    }

    return (jstat);
}
/***************************************************************/
static int jexp_binop_div(struct jrunexec * jx,
    struct jvarvalue * ajvv1,
    struct jvarvalue * ajvv2)
{
/*
**
*/
    int jstat = 0;
    struct jvarvalue * jvv1;
    struct jvarvalue * jvv2;

    jstat = jexp_get_rval(jx, &jvv1, ajvv1);
    if (jstat) return (jstat);
    jstat = jexp_get_rval(jx, &jvv2, ajvv2);
    if (jstat) return (jstat);

    switch (jvv1->jvv_dtype) {
        case JVV_DTYPE_JSINT   :
            switch (jvv2->jvv_dtype) {
                case JVV_DTYPE_JSINT   :
                    jvar_store_jsint(ajvv1,
                        jvv1->jvv_val.jvv_val_jsint /
                        jvv2->jvv_val.jvv_val_jsint);
                    break;
                case JVV_DTYPE_JSFLOAT   :
                    jvar_store_jsfloat(ajvv1,
                        (JSFLOAT)jvv1->jvv_val.jvv_val_jsint /
                        jvv2->jvv_val.jvv_val_jsfloat);
                    break;
                default:
                    jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_OPERATOR_TYPE_MISMATCH,
                        "Illegal integer division");
                    break;
            }
            break;

        case JVV_DTYPE_JSFLOAT:
            switch (jvv2->jvv_dtype) {
                case JVV_DTYPE_JSINT   :
                    jvar_store_jsfloat(ajvv1,
                        jvv1->jvv_val.jvv_val_jsfloat /
                        (JSFLOAT)jvv2->jvv_val.jvv_val_jsint);
                    break;
                case JVV_DTYPE_JSFLOAT   :
                    jvar_store_jsfloat(ajvv1,
                        jvv1->jvv_val.jvv_val_jsfloat /
                        jvv2->jvv_val.jvv_val_jsfloat);
                    break;
                default:
                    jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_OPERATOR_TYPE_MISMATCH,
                        "Illegal floating division");
                    break;
            }
            break;

        default:
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_OPERATOR_TYPE_MISMATCH,
                "Illegal division");
            break;
    }

    return (jstat);
}
/***************************************************************/
int pow_int(int base, int exponent, int * ans)
{
/*
** 10/07/2020 - integer exponentiation
**
** Returns:
**      0 - OK
**      1 - Base is zero and exponent is not positive
*/
    int lans;
    int lbase;
    int lexponent;
    int merr;

    merr = 0;
    lbase = base;
    lexponent  = exponent;
    if (exponent < 0) lexponent = -exponent;
    if (base == 0 && exponent <= 0) merr = 1;

    lans = 1;
    while (lexponent) {
        if (lexponent & 1) lans = lans * lbase;
        lexponent = lexponent >> 1;
        lbase = lbase * lbase;
    }

    if (exponent < 0 && lans != 0) lans = 1 / lans;
    (*ans) = lans;

    return (merr);
}
/***************************************************************/
int pow_flt_int(JSFLOAT base, int exponent, JSFLOAT * ans)
{
/*
** 10/07/2020 - float to integer exponentiation
**
** Returns:
**      0 - OK
**      1 - Base is zero and exponent is not positive
*/
    JSFLOAT lans;
    JSFLOAT lbase;
    int lexponent;
    int merr;

    merr = 0;
    lbase = base;
    lexponent  = exponent;
    if (exponent < 0) lexponent = -exponent;
    if (base == 0.0 && exponent <= 0) merr = 1;

    lans = 1;
    while (lexponent) {
        if (lexponent & 1) lans = lans * lbase;
        lexponent = lexponent >> 1;
        lbase = lbase * lbase;
    }

    if (exponent < 0 && lans != 0) lans = 1.0 / lans;
    (*ans) = lans;

    return (merr);
}
/***************************************************************/
int pow_flt(JSFLOAT base, JSFLOAT exponent, JSFLOAT * ans)
{
/*
** 10/07/2020 - floating exponentiation
**
** Returns:
**      0 - OK
**      1 - Base is zero and exponent is not positive
**      2 - Base is negative and exponent not an integer
*/
    int merr;
    JSFLOAT lnbase;
    JSFLOAT lexponent;

    merr = 0;
    if (base < 0.0) {
        if (exponent < 0) lexponent = -exponent;
        lexponent = (JSFLOAT)((int)exponent);
        if (exponent != lexponent) {
            merr = 2;
        } else {
            merr = pow_flt_int(base, (int)exponent, ans);
        }
    } else if (base > 0.0) {
        lnbase = log(base);
        (*ans) = exp(exponent * lnbase);
    } else {    /* base == 0.0) */
        (*ans) = 0.0;
        if (exponent <= 0.0) merr = 1;
    }

    return (merr);
}
/***************************************************************/
static int jexp_binop_pow(struct jrunexec * jx,
    struct jvarvalue * ajvv1,
    struct jvarvalue * ajvv2)
{
/*
** hat = exponentiation
*/
    int jstat = 0;
    int merr;
    int nint;
    JSFLOAT nflt;
    struct jvarvalue * jvv1;
    struct jvarvalue * jvv2;

    jstat = jexp_get_rval(jx, &jvv1, ajvv1);
    if (jstat) return (jstat);
    jstat = jexp_get_rval(jx, &jvv2, ajvv2);
    if (jstat) return (jstat);

    switch (jvv1->jvv_dtype) {
        case JVV_DTYPE_JSINT   :
            switch (jvv2->jvv_dtype) {
                case JVV_DTYPE_JSINT   :
                    merr = pow_int(jvv1->jvv_val.jvv_val_jsint,
                        jvv2->jvv_val.jvv_val_jsint, &nint);
                    jvar_store_jsint(ajvv1, nint);
                    if (merr) {
                        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_ILLEGAL_EXPONENTIATION,
                            "Error on integer exponentiation");
                    }
                    break;
                case JVV_DTYPE_JSFLOAT   :
                    merr = pow_flt((JSFLOAT)jvv1->jvv_val.jvv_val_jsint,
                        jvv2->jvv_val.jvv_val_jsfloat, &nflt);
                    jvar_store_jsfloat(ajvv1, nflt);
                    if (merr) {
                        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_ILLEGAL_EXPONENTIATION,
                            "Error on integer exponentiation");
                    }
                    break;
                default:
                    jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_OPERATOR_TYPE_MISMATCH,
                        "Illegal integer exponentiation");
                    break;
            }
            break;

        case JVV_DTYPE_JSFLOAT:
            switch (jvv2->jvv_dtype) {
                case JVV_DTYPE_JSINT   :
                    merr = pow_flt_int(jvv1->jvv_val.jvv_val_jsfloat,
                        jvv2->jvv_val.jvv_val_jsint, &nflt);
                    jvar_store_jsfloat(ajvv1, nflt);
                    if (merr) {
                        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_ILLEGAL_EXPONENTIATION,
                            "Error on floating exponentiation");
                    }
                    break;
                case JVV_DTYPE_JSFLOAT   :
                    merr = pow_flt(jvv1->jvv_val.jvv_val_jsfloat,
                        jvv2->jvv_val.jvv_val_jsfloat, &nflt);
                    jvar_store_jsfloat(ajvv1, nflt);
                    if (merr) {
                        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_ILLEGAL_EXPONENTIATION,
                            "Error on floating exponentiation");
                    }
                    break;
                default:
                    jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_OPERATOR_TYPE_MISMATCH,
                        "Illegal floating exponentiation");
                    break;
            }
            break;

        default:
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_OPERATOR_TYPE_MISMATCH,
                "Illegal exponentiation");
            break;
    }

    return (jstat);
}
/***************************************************************/
static int jexp_binop_bitand(struct jrunexec * jx,
    struct jvarvalue * ajvv1,
    struct jvarvalue * ajvv2)
{
/*
**
*/
    int jstat = 0;
    int intval1;
    int intval2;
    struct jvarvalue * jvv1;
    struct jvarvalue * jvv2;

    jstat = jexp_get_rval(jx, &jvv1, ajvv1);
    if (jstat) return (jstat);
    jstat = jexp_get_rval(jx, &jvv2, ajvv2);
    if (jstat) return (jstat);

    jstat = jrun_ensure_int(jx, jvv1, &intval1, ENSINT_FLAG_INTERR);
    if (!jstat) {
        jstat = jrun_ensure_int(jx, jvv2, &intval2, ENSINT_FLAG_INTERR);
        if (!jstat) {
            jvar_store_jsint(ajvv1, intval1 & intval2);
        }
    }

    return (jstat);
}
/***************************************************************/
static int jexp_binop_bitxor(struct jrunexec * jx,
    struct jvarvalue * ajvv1,
    struct jvarvalue * ajvv2)
{
/*
**
*/
    int jstat = 0;
    int intval1;
    int intval2;
    struct jvarvalue * jvv1;
    struct jvarvalue * jvv2;

    jstat = jexp_get_rval(jx, &jvv1, ajvv1);
    if (jstat) return (jstat);
    jstat = jexp_get_rval(jx, &jvv2, ajvv2);
    if (jstat) return (jstat);

    jstat = jrun_ensure_int(jx, jvv1, &intval1, ENSINT_FLAG_INTERR);
    if (!jstat) {
        jstat = jrun_ensure_int(jx, jvv2, &intval2, ENSINT_FLAG_INTERR);
        if (!jstat) {
            jvar_store_jsint(ajvv1, intval1 ^ intval2);
        }
    }

    return (jstat);
}
/***************************************************************/
static int jexp_binop_bitor(struct jrunexec * jx,
    struct jvarvalue * ajvv1,
    struct jvarvalue * ajvv2)
{
/*
**
*/
    int jstat = 0;
    int intval1;
    int intval2;
    struct jvarvalue * jvv1;
    struct jvarvalue * jvv2;

    jstat = jexp_get_rval(jx, &jvv1, ajvv1);
    if (jstat) return (jstat);
    jstat = jexp_get_rval(jx, &jvv2, ajvv2);
    if (jstat) return (jstat);

    jstat = jrun_ensure_int(jx, jvv1, &intval1, ENSINT_FLAG_INTERR);
    if (!jstat) {
        jstat = jrun_ensure_int(jx, jvv2, &intval2, ENSINT_FLAG_INTERR);
        if (!jstat) {
            jvar_store_jsint(ajvv1, intval1 | intval2);
        }
    }

    return (jstat);
}
/***************************************************************/
static int jexp_compare_chars(
    struct jvarvalue_chars * jvvc1,
    struct jvarvalue_chars * jvvc2)
{
/*
** 12/09/2020
*/
    int cmpval;

    if (jvvc1->jvvc_length < jvvc2->jvvc_length) {
        cmpval = strncmp(jvvc1->jvvc_val_chars, jvvc2->jvvc_val_chars, jvvc1->jvvc_length);
        if (!cmpval && jvvc1->jvvc_length != jvvc2->jvvc_length) cmpval = 2;
    } else {
        cmpval = strncmp(jvvc1->jvvc_val_chars, jvvc2->jvvc_val_chars, jvvc2->jvvc_length);
        if (!cmpval && jvvc1->jvvc_length != jvvc2->jvvc_length) cmpval = 1;
    }

    return (cmpval);
}
/***************************************************************/
static int jexp_compare(struct jrunexec * jx,
    struct jvarvalue * ajvv1,
    struct jvarvalue * ajvv2,
    int * cmpval,
    int cmp_equality)
{
/*
** 12/08/2020
**
** See: https://www.ecma-international.org/ecma-262/5.1/#sec-11.9.3
*/
    int jstat = 0;
    struct jvarvalue * jvv1;
    struct jvarvalue * jvv2;

    jstat = jexp_get_rval(jx, &jvv1, ajvv1);
    if (jstat) return (jstat);
    jstat = jexp_get_rval(jx, &jvv2, ajvv2);
    if (jstat) return (jstat);

    switch (jvv1->jvv_dtype) {
        case JVV_DTYPE_BOOL   :
            switch (jvv2->jvv_dtype) {
                case JVV_DTYPE_BOOL   :
                    (*cmpval) = COMPARE(jvv1->jvv_val.jvv_val_bool, jvv2->jvv_val.jvv_val_bool);
                    break;
                case JVV_DTYPE_JSINT   :
                    (*cmpval) = COMPARE(jvv1->jvv_val.jvv_val_bool, jvv2->jvv_val.jvv_val_bool);
                    break;
                case JVV_DTYPE_JSFLOAT   :
                    (*cmpval) =
                    COMPARE((JSFLOAT)jvv1->jvv_val.jvv_val_bool, jvv2->jvv_val.jvv_val_jsfloat);
                    break;
                default:
                    jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_INVALID_COMPARE,
                        "Illegal integer compare");
                    break;
            }
            break;

        case JVV_DTYPE_JSINT   :
            switch (jvv2->jvv_dtype) {
                case JVV_DTYPE_BOOL   :
                    (*cmpval) = COMPARE(jvv1->jvv_val.jvv_val_jsint, jvv2->jvv_val.jvv_val_bool);
                    break;
                case JVV_DTYPE_JSINT   :
                    (*cmpval) = COMPARE(jvv1->jvv_val.jvv_val_jsint, jvv2->jvv_val.jvv_val_jsint);
                    break;
                case JVV_DTYPE_JSFLOAT   :
                    (*cmpval) =
                    COMPARE((JSFLOAT)jvv1->jvv_val.jvv_val_jsint, jvv2->jvv_val.jvv_val_jsfloat);
                    break;
                default:
                    jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_INVALID_COMPARE,
                        "Illegal integer compare");
                    break;
            }
            break;

        case JVV_DTYPE_JSFLOAT:
            switch (jvv2->jvv_dtype) {
                case JVV_DTYPE_BOOL   :
                    (*cmpval) =
                    COMPARE(jvv1->jvv_val.jvv_val_jsfloat, (JSFLOAT)jvv2->jvv_val.jvv_val_bool);
                    break;
                case JVV_DTYPE_JSINT   :
                    (*cmpval) =
                    COMPARE(jvv1->jvv_val.jvv_val_jsfloat, (JSFLOAT)jvv2->jvv_val.jvv_val_jsint);
                    break;
                case JVV_DTYPE_JSFLOAT   :
                    (*cmpval) =
                    COMPARE(jvv1->jvv_val.jvv_val_jsfloat, jvv2->jvv_val.jvv_val_jsfloat);
                    break;
                default:
                    jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_INVALID_COMPARE,
                        "Illegal floating compare");
                    break;
            }
            break;

        case JVV_DTYPE_CHARS:
            switch (jvv2->jvv_dtype) {
                case JVV_DTYPE_CHARS   :
#if USE_JVV_CHARS_POINTER
                    (*cmpval) = jexp_compare_chars(jvv1->jvv_val.jvv_val_chars,
                                                   jvv2->jvv_val.jvv_val_chars);
#else
                    (*cmpval) = jexp_compare_chars(&(jvv1->jvv_val.jvv_val_chars),
                                                   &(jvv2->jvv_val.jvv_val_chars));
#endif
                    break;
                default:
                    jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_INVALID_COMPARE,
                        "Illegal character compare");
                    break;
            }
            jvar_free_jvarvalue_data(ajvv1);     /* 07/15/2021 - 02/15/22 changed jvv1 --> ajvv1 */
            jvar_free_jvarvalue_data(ajvv2);     /* 07/15/2021 - 02/15/22 changed jvv2 --> ajvv2 */
            break;

        case JVV_DTYPE_ARRAY:
            switch (jvv2->jvv_dtype) {
                case JVV_DTYPE_ARRAY   :
                    (*cmpval) = jexp_compare_arrays(jvv1->jvv_val.jvv_val_array,
                                                    jvv2->jvv_val.jvv_val_array);
                    break;
                default:
                    jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_INVALID_COMPARE,
                        "Illegal array compare");
                    break;
            }
            jvar_free_jvarvalue_data(ajvv1);     /* 07/15/2021 - 02/15/22 changed jvv1 --> ajvv1 */
            jvar_free_jvarvalue_data(ajvv2);     /* 07/15/2021 - 02/15/22 changed jvv2 --> ajvv2 */
            break;
        
        case JVV_DTYPE_OBJPTR:
            switch (jvv1->jvv_val.jvv_objptr.jvvp_pval->jvv_dtype) {
                case JVV_DTYPE_ARRAY   :
                    if (jvv2->jvv_dtype == JVV_DTYPE_OBJPTR &&
                        jvv2->jvv_val.jvv_objptr.jvvp_pval->jvv_dtype == JVV_DTYPE_ARRAY) {
                        if (jvv1->jvv_val.jvv_objptr.jvvp_pval->jvv_val.jvv_val_array == /* pointer compare */
                            jvv2->jvv_val.jvv_objptr.jvvp_pval->jvv_val.jvv_val_array) {
                            (*cmpval) = 0;
                        } else {
                            (*cmpval) = 1;
                        }
                    } else {
                        (*cmpval) = 1;
                    }
                    break;
                default:
                    jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_INVALID_STORE,
                        "Illegal object pointer compare %s",
                        jvar_get_dtype_name(jvv1->jvv_dtype));
                    break;
            }
            break;

        default:
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_INVALID_COMPARE,
                "Illegal compare");
            break;
    }

    if (jstat) {
        jvar_free_jvarvalue_data(jvv1);
        jvar_free_jvarvalue_data(jvv2);
    }

    return (jstat);
}
/***************************************************************/
static int jexp_binop_cmp_eq(struct jrunexec * jx,
    struct jvarvalue * jvv1,
    struct jvarvalue * jvv2)
{
/*
** 12/09/2020
*/
    int jstat;;
    int cmpval;

    jstat = jexp_compare(jx, jvv1, jvv2, &cmpval, 1);
    if (!jstat) {
        jvar_store_bool(jvv1, (cmpval == 0));
    }          

    return (jstat);
}
/***************************************************************/
static int jexp_binop_cmp_ne(struct jrunexec * jx,
    struct jvarvalue * jvv1,
    struct jvarvalue * jvv2)
{
/*
** 12/09/2020
*/
    int jstat;;
    int cmpval;

    jstat = jexp_compare(jx, jvv1, jvv2, &cmpval, 1);
    if (!jstat) {
        jvar_store_bool(jvv1, (cmpval != 0));
    }          

    return (jstat);
}
/***************************************************************/
static int jexp_binop_cmp_le(struct jrunexec * jx,
    struct jvarvalue * jvv1,
    struct jvarvalue * jvv2)
{
/*
** 12/09/2020
*/
    int jstat;;
    int cmpval;

    jstat = jexp_compare(jx, jvv1, jvv2, &cmpval, 0);
    if (!jstat) {
        jvar_store_bool(jvv1, (cmpval <= 0));
    }          

    return (jstat);
}
/***************************************************************/
static int jexp_binop_cmp_lt(struct jrunexec * jx,
    struct jvarvalue * jvv1,
    struct jvarvalue * jvv2)
{
/*
** 12/09/2020
*/
    int jstat;;
    int cmpval;

    jstat = jexp_compare(jx, jvv1, jvv2, &cmpval, 0);
    if (!jstat) {
        jvar_store_bool(jvv1, (cmpval < 0));
    }          

    return (jstat);
}
/***************************************************************/
static int jexp_binop_cmp_gt(struct jrunexec * jx,
    struct jvarvalue * jvv1,
    struct jvarvalue * jvv2)
{
/*
** 12/09/2020
*/
    int jstat;;
    int cmpval;

    jstat = jexp_compare(jx, jvv1, jvv2, &cmpval, 0);
    if (!jstat) {
        jvar_store_bool(jvv1, (cmpval > 0));
    }          

    return (jstat);
}
/***************************************************************/
static int jexp_binop_cmp_ge(struct jrunexec * jx,
    struct jvarvalue * jvv1,
    struct jvarvalue * jvv2)
{
/*
** 12/09/2020
*/
    int jstat;;
    int cmpval;

    jstat = jexp_compare(jx, jvv1, jvv2, &cmpval, 0);
    if (!jstat) {
        jvar_store_bool(jvv1, (cmpval >= 0));
    }          

    return (jstat);
}
/***************************************************************/
int jvar_assign_jvarvalue(
    struct jrunexec * jx,
    struct jvarvalue * jvvtgt,
    struct jvarvalue * jvvsrc)
{
/*
** 10/11/2021
*/
    int jstat = 0;
    struct jvarvalue * jvv;

#if DEBUG_ASSIGN
    jexp_quick_print(jx, jvvsrc, "Assign ");
#endif

    jstat = jexp_get_rval(jx, &jvv, jvvsrc);
    if (jstat) return (jstat);

    jstat = jvar_store_jvarvalue(jx, jvvtgt, jvv);

#if DEBUG_ASSIGN
    jexp_quick_print(jx, jvvtgt, "    -> ");
#endif

    return (jstat);
}
/***************************************************************/
static int jvar_assign_to_reference(struct jrunexec * jx,
    struct jvarvalue * jvvtgt,
    struct jvarvalue * jvvsrc)
{
/*
**
*/
    int jstat = 0;
    struct jvarvalue * vjvvtgt;

    if (jvvtgt->jvv_dtype == JVV_DTYPE_LVAL) {
        vjvvtgt = jvvtgt->jvv_val.jvv_lval.jvvv_lval;
        /*
        ** The following is needed to prevent the following from working:
        **      var abc = "Abcdefghijklmn";
        **      var subs = abc.substring;
        **      var val=subs(3, 8);
        */
        if (vjvvtgt->jvv_dtype == JVV_DTYPE_FUNCVAR) {
            vjvvtgt->jvv_val.jvv_val_funcvar.jvvfv_this_obj = NULL;
        } else if (vjvvtgt->jvv_dtype == JVV_DTYPE_IMETHVAR) {
#if USE_JVARVALUE_IMETHVAR
            vjvvtgt->jvv_val.jvv_val_imethvar.jvvimv_this_jvv = NULL;
#endif
            vjvvtgt->jvv_val.jvv_val_imethvar.jvvimv_this_ptr = NULL;
        }
        jstat = jvar_assign_jvarvalue(jx, vjvvtgt, jvvsrc);
        if (!jstat) jvar_free_jvarvalue_data(jvvsrc);
    } else if (jvvtgt->jvv_dtype == JVV_DTYPE_DYNAMIC) {
        if (!jvvtgt->jvv_val.jvv_val_dynamic->jvvy_set_proc) {
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_PROP_READ_ONLY,
                "Property is read-only and may not be set");
        } else {
            jstat = (jvvtgt->jvv_val.jvv_val_dynamic->jvvy_set_proc)(jx,
                jvvtgt->jvv_val.jvv_val_dynamic, NULL, jvvsrc);
        }
    } else {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERROBJ_SCRIPT_SYNTAX_ERROR,
            "Invalid left-hand side in assignment");
    }

    return (jstat);
}
/***************************************************************/
static int jexp_binop_assign(struct jrunexec * jx,
    struct jvarvalue * jvvtgt,
    struct jvarvalue * jvvsrc)
{
/*
** 04/04/2022
*/
    int jstat;

    jstat = jvar_assign_to_reference(jx, jvvtgt, jvvsrc);

    return (jstat);
}
/***************************************************************/
static void free_jvarvalue_funcarg(struct jvarvalue_funcarg * jvvfa)
{
/*
** 02/08/2021
*/
    Free(jvvfa->jvvfa_arg_name);

    Free(jvvfa);
}
/***************************************************************/
void jvar_free_jvarvalue_function(struct jvarvalue_function * jvvf)
{
/*
** 02/08/2021
*/
    int ii;

    if (!jvvf) return;

    DECFUNCREFS(jvvf);
    if (jvvf->jvvf_nRefs == 0) {
        Free(jvvf->jvvf_func_name);
        for (ii = 0; ii < jvvf->jvvf_nargs; ii++) {
            free_jvarvalue_funcarg(jvvf->jvvf_aargs[ii]);
        }
        Free(jvvf->jvvf_aargs);
        /* jvar_free_jvarrec_consts(jvvf->jvvf_vars); */
        jvar_free_jvarrec(jvvf->jvvf_vars);

        Free(jvvf);
    }
}
/***************************************************************/
struct jvarvalue_function * new_jvarvalue_function(void)
{
/*
** 02/08/2021
*/
    struct jvarvalue_function * jvvf;
#if IS_DEBUG
    static int next_jvvf_sn = 0;
#endif

    jvvf = New(struct jvarvalue_function, 1);
#if IS_DEBUG
    jvvf->jvvf_sn[0] = 'F';
    jvvf->jvvf_sn[1] = (next_jvvf_sn / 100) % 10 + '0';
    jvvf->jvvf_sn[2] = (next_jvvf_sn /  10) % 10 + '0';
    jvvf->jvvf_sn[3] = (next_jvvf_sn      ) % 10 + '0';
    next_jvvf_sn++;
#endif
    jvvf->jvvf_func_name = NULL;
    jvvf->jvvf_flags    = 0;
    jvvf->jvvf_nRefs    = 0;
    jvvf->jvvf_nargs    = 0;
    jvvf->jvvf_xargs    = 0;
    jvvf->jvvf_aargs    = NULL;
    jvvf->jvvf_vix      = VINVALID;
    jrun_init_jxcursor(&(jvvf->jvvf_begin_jxc));
    jvvf->jvvf_vars = jvar_new_jvarrec();
    INCVARRECREFS(jvvf->jvvf_vars);

    return (jvvf);
}
/***************************************************************/
static struct jvarvalue_funcarg * new_jvarvalue_funcarg(char * argname,
                    int arg_flags)
{
/*
** 02/12/2021
*/
    struct jvarvalue_funcarg * jvvfa;

    jvvfa = New(struct jvarvalue_funcarg, 1);
    jvvfa->jvvfa_arg_name = Strdup(argname);
    jvvfa->jvvfa_flags = arg_flags;
    jvvfa->jvvfa_vix = VINVALID;

    return (jvvfa);
}
/***************************************************************/
#if IS_DEBUG
struct jvscanrec {  /* jvsr */
    struct jrunexec   * jvsr_jx;
    int                 jvsr_vflags;
};
/***************************************************************/
#if 0
int jrun_debug_show_jvarrec(
                    void * vjvsr,
                    void * vvarname,
                    void * vjvv)
{
/*
** 02/23/2021
*/
    int jstat = 0;
    int done = 0;
    struct jvscanrec * jvsr = (struct jvscanrec *)vjvsr;
    struct jvarvalue * jvv  = (struct jvarvalue *)vjvv;
    char * varname = (char *)vvarname;
    char vbuf[64];

    jexp_quick_tostring(jvsr->jvsr_jx, jvv, vbuf, sizeof(vbuf));
    printf("%s=%s\n", varname, vbuf);

    return (done);
}
#endif
/***************************************************************/
int jpr_exec_debug_help(
                    struct jrunexec * jx)
{
/*
** 10/18/2021
*/
    int jstat = 0;
    
    jstat = jpr_exec_debug_show_help(jx);

    return (jstat);
}
/***************************************************************/
int jrun_debug_chars(
                    struct jrunexec * jx,
                    char * dbgstr)
{
/*
** 02/22/2021
**
**  Syntax:
**      'debug show stacktrace';
**      'debug show allvars';
*/
    int jstat = 0;
    int ix;
    char toke[TOKSZ];

    ix = 0;
    get_toke_char(dbgstr, &ix, toke, TOKSZ, GETTOKE_FLAGS_DEFAULT);
    if (!strcmp(toke, "debug")) {
        while (!jstat && dbgstr[ix]) {
            get_toke_char(dbgstr, &ix, toke, TOKSZ, GETTOKE_FLAGS_DEFAULT);
            if (!strcmp(toke, "show")) {
                jstat = jpr_exec_debug_show(jx, dbgstr, &ix);
            } else if (!strcmp(toke, "help") || !strcmp(toke, "?")) {
                jstat = jpr_exec_debug_help(jx);
            }
            if (!jstat) {
                while (isspace(dbgstr[ix]) || dbgstr[ix] == ',') ix++;
            }
        }
    }

    return (jstat);
}
#endif
/***************************************************************/
static int jvar_parse_function_body(
                    struct jrunexec * jx,
                    struct jvarvalue_function * jvvf,
                    struct jtoken ** pjtok,
                    int skip_flags)
{
/*
** 02/11/2021
*/
    int jstat = 0;

    if ((*pjtok)->jtok_kw == JSPU_OPEN_BRACE) {
        jrun_get_current_jxc(jx, &(jvvf->jvvf_begin_jxc));
        jstat = jprep_preprocess_block(jx, jvvf, pjtok, skip_flags);
    } else {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERROBJ_SCRIPT_SYNTAX_ERROR,
            "Expecting '{' to begin function. Found: '%s'", (*pjtok)->jtok_text);
    }

    return (jstat);
}
/***************************************************************/
static int jvar_add_function_argument(
                    struct jrunexec * jx,
                    struct jvarvalue_function * jvvf,
                    int arg_flags,
                    struct jtoken ** pjtok)
{
/*
** 02/12/2021
*/
    int jstat = 0;
    int vix;
    struct jvarvalue_funcarg * jvvfa;

    jvvfa = new_jvarvalue_funcarg((*pjtok)->jtok_text, arg_flags);

    if (jvvf->jvvf_nargs == jvvf->jvvf_xargs) {
        if (!jvvf->jvvf_nargs) jvvf->jvvf_xargs = 2;
        else                   jvvf->jvvf_xargs *= 2;
        jvvf->jvvf_aargs = Realloc(jvvf->jvvf_aargs, struct jvarvalue_funcarg *, jvvf->jvvf_xargs);
    }
    jvvf->jvvf_aargs[jvvf->jvvf_nargs] = jvvfa;
    jvvf->jvvf_nargs += 1;
    vix = jvar_new_local_variable(jx, jvvf, (*pjtok)->jtok_text);
    jvvfa->jvvfa_vix = vix;

    return (jstat);
}
/***************************************************************/
static int jvar_validate_function_argname(
                    struct jrunexec * jx,
                    struct jvarvalue_function * jvvf,
                    struct jtoken ** pjtok)
{
/*
** 02/12/2021
*/
    int jstat = 0;
    int * pvix;
    char * argname = (*pjtok)->jtok_text;

    jstat = jvar_validate_varname(jx, (*pjtok));
    if (!jstat) {
        pvix = jvar_find_in_jvarrec(jvvf->jvvf_vars, (*pjtok)->jtok_text);
        if (pvix) {
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERROBJ_SCRIPT_SYNTAX_ERROR,
                "Duplicate argument name. Found: '%s'", (*pjtok)->jtok_text);
        }
    }

    return (jstat);
}
/***************************************************************/
static int jvar_parse_function_arguments(
                    struct jrunexec * jx,
                    struct jvarvalue_function * jvvf,
                    struct jtoken ** pjtok)
{
/*
** 02/11/2021
*/
    int jstat = 0;
    int done;

    done = 0;
    while (!jstat && !done) {
        if ((*pjtok)->jtok_typ == JSW_PU && (*pjtok)->jtok_kw == JSPU_CLOSE_PAREN) {
            done = 1;
        } else if ((*pjtok)->jtok_typ == JSW_PU && (*pjtok)->jtok_kw == JSPU_DOT_DOT_DOT) {
            jstat = jrun_next_token(jx, pjtok);
            if (!jstat) {
                jstat = jvar_validate_function_argname(jx, jvvf, pjtok);
            }
            if (!jstat) {
                jstat = jvar_add_function_argument(jx, jvvf, JVVFA_FLAG_REST_PARAM, pjtok);
            }
            if (!jstat) {
                jstat = jrun_next_token(jx, pjtok);
            }
            if (!jstat) {
                if ((*pjtok)->jtok_typ == JSW_PU && (*pjtok)->jtok_kw == JSPU_CLOSE_PAREN) {
                    done = 1;
                } else if ((*pjtok)->jtok_typ == JSW_PU && (*pjtok)->jtok_kw == JSPU_COMMA) {
                    jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERROBJ_SCRIPT_SYNTAX_ERROR,
                        "Rest argument must be last argument list. Found: '%s'", (*pjtok)->jtok_text);
                } else {
                    jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERROBJ_SCRIPT_SYNTAX_ERROR,
                        "Expecting ')' to end argument list. Found: '%s'", (*pjtok)->jtok_text);
                }
            }
        } else {
            jstat = jvar_validate_function_argname(jx, jvvf, pjtok);
            if (!jstat) {
                jstat = jvar_add_function_argument(jx, jvvf, 0, pjtok);
            }
            if (!jstat) {
                jstat = jrun_next_token(jx, pjtok);
            }
            if (!jstat) {
                if ((*pjtok)->jtok_typ == JSW_PU && (*pjtok)->jtok_kw == JSPU_CLOSE_PAREN) {
                    done = 1;
                } else if ((*pjtok)->jtok_typ == JSW_PU && (*pjtok)->jtok_kw == JSPU_COMMA) {
                    jstat = jrun_next_token(jx, pjtok);
                } else {
                    jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERROBJ_SCRIPT_SYNTAX_ERROR,
                        "Expecting ',' or ')' in argument list. Found: '%s'", (*pjtok)->jtok_text);
                }
            }
        }
    }
 
    if (!jstat) {
        jstat = jrun_next_token(jx, pjtok);
    }

    return (jstat);
}
/***************************************************************/
int jvar_parse_function(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    struct jvarvalue * jvv,
                    int skip_flags)
{
/*
** 02/08/2021
**
** Syntax:
**  function name([param[, param,[..., param]]]) {
**     [statements]
**  }
**  function* name([param[, param[, ... param]]]) {
**     statements
**  }
*/
    int jstat = 0;
    struct jvarvalue_function * jvvf;

    jvvf = new_jvarvalue_function();
    if ((*pjtok)->jtok_kw == JSPU_STAR) {
        jvvf->jvvf_flags |= JVVF_FLAG_GENERATOR;
        jstat = jrun_next_token(jx, pjtok);
        if (jstat) return (jstat);
    }

    if ((*pjtok)->jtok_typ == JSW_ID || (*pjtok)->jtok_typ == JSW_KW) {
        jstat = jvar_validate_varname(jx, (*pjtok));
        if (!jstat) {
            jvvf->jvvf_func_name = Strdup((*pjtok)->jtok_text);
            jstat = jrun_next_token(jx, pjtok);
        }
    }

    if (!jstat) {
        if ((*pjtok)->jtok_typ == JSW_PU && (*pjtok)->jtok_kw == JSPU_OPEN_PAREN) {
            jstat = jrun_next_token(jx, pjtok);
            if (jstat) return (jstat);
            jstat = jvar_parse_function_arguments(jx, jvvf, pjtok);
            if (!jstat) {
                jstat = jvar_parse_function_body(jx, jvvf, pjtok, skip_flags);
            }
            if (!jstat) {
                jvv->jvv_dtype = JVV_DTYPE_FUNCTION;
                jvv->jvv_val.jvv_val_function = jvvf;
            }
        } else {
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERROBJ_SCRIPT_SYNTAX_ERROR,
                "Expecting '(' to begin function arguments. Found: '%s'", (*pjtok)->jtok_text);
        }
    }

    if (jstat) {
        jvar_free_jvarvalue_function(jvvf);
    } else {
        INCFUNCREFS(jvvf);
    }

    return (jstat);
}
/***************************************************************/
#ifdef OLD_WAY
int jvar_parse_function_arguments_inactive(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok)
{
/*
** 02/11/2021
*/
    int jstat = 0;
    int done;

    done = 0;
    while (!jstat && !done) {
        if ((*pjtok)->jtok_typ == JSW_PU && (*pjtok)->jtok_kw == JSPU_CLOSE_PAREN) {
            done = 1;
        } else if ((*pjtok)->jtok_typ == JSW_PU && (*pjtok)->jtok_kw == JSPU_DOT_DOT_DOT) {
            jstat = jrun_next_token(jx, pjtok);
            if (!jstat) {
                jstat = jrun_next_token(jx, pjtok);
            }
            if (!jstat) {
                if ((*pjtok)->jtok_typ == JSW_PU && (*pjtok)->jtok_kw == JSPU_CLOSE_PAREN) {
                    done = 1;
                } else {
                    jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERROBJ_SCRIPT_SYNTAX_ERROR,
                        "Expecting ')' to end argument list. Found: '%s'", (*pjtok)->jtok_text);
                }
            }
        } else {
            jstat = jrun_next_token(jx, pjtok);
            if (!jstat) {
                if ((*pjtok)->jtok_typ == JSW_PU && (*pjtok)->jtok_kw == JSPU_CLOSE_PAREN) {
                    done = 1;
                } else if ((*pjtok)->jtok_typ == JSW_PU && (*pjtok)->jtok_kw == JSPU_COMMA) {
                    jstat = jrun_next_token(jx, pjtok);
                } else {
                    jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERROBJ_SCRIPT_SYNTAX_ERROR,
                        "Expecting ',' or ')' in argument list. Found: '%s'", (*pjtok)->jtok_text);
                }
            }
        }
    }

    if (!jstat) jstat = jrun_next_token(jx, pjtok);

    return (jstat);
}
#endif
/***************************************************************/
#ifdef OLD_WAY
int jexp_eval_function_stmt_inactive(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok)
{
/*
** 01/19/2021
**
**  On Entry: (*pjtok)->jtok_text = "("
*/
    int jstat;

    jstat = jrun_next_token(jx, pjtok);
    if (!jstat) {
        jstat = jvar_parse_function_arguments_inactive(jx, pjtok);
    }
    if (!jstat) {
        jstat = jrun_exec_inactive_block(jx, pjtok); /* in: "{"	out:  */
    }

    return (jstat);
}
#endif
/***************************************************************/
#ifdef OLD_WAY
int xxxjrun_exec_function_stmt(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    int jcmdflags)
{
/*
** 03/22/2021
*/
    int jstat = 0;
    struct jcontext * jcx;

    if ((*pjtok)->jtok_kw == JSPU_STAR) {
        jstat = jrun_next_token(jx, pjtok);
        if (jstat) return (jstat);
    }

    if ((*pjtok)->jtok_typ == JSW_ID || (*pjtok)->jtok_typ == JSW_KW) {
        if (XSTAT_IS_ACTIVE(jx)) {
            jstat = jvar_get_block_jcontext(jx, (*pjtok)->jtok_text, &jcx); /* 12/29/2021 */
        }
        jstat = jrun_next_token(jx, pjtok);
        if (jstat) return (jstat);
    }

    if (!jstat) {
        jstat = jexp_eval_function_stmt_inactive(jx, pjtok);
    }

    return (jstat);
}
#endif
/***************************************************************/
int jrun_exec_function_stmt(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok)
{
/*
** 03/22/2021
**
** On entry (*pjtok)->jtok_text = "function"
*/
    int jstat = 0;
    struct jcontext * jcx;
    struct jtokeninfo * jti_func;

    jti_func = (*pjtok)->jtok_jti;
#if IS_DEBUG
    if (!jti_func ||
        jti_func->jti_type != JTI_TYPE_FUNCTION ||
        !jti_func->jti_val.jtifu_end_jxc || 
        !jti_func->jti_val.jtifu_end_jxc->jxc_jtl) {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_INTERNAL_ERROR,
            "Internal error: Invalid jtokeninfo on function.");
        return (jstat);
    }
#endif

    jrun_set_current_jxc(jx, jti_func->jti_val.jtifu_end_jxc);
    jstat = jrun_next_token(jx, pjtok);
    if (!jstat) {
        if (XSTAT_IS_ACTIVE(jx)) {
            jstat = jvar_get_block_jcontext(jx, (*pjtok)->jtok_text, &jcx); /* 12/29/2021 */
        }
    }

    return (jstat);
}
/***************************************************************/
int jrun_exec_return_stmt(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok)
{
/*
** 02/19/2021
**
** Omar
{
    int svstat;
    struct svarvalue svv;

    svstat = svare_evaluate(svx, svt, &svv);
    if (!svstat) {
        if (!SVAR_STMT_TERM_CHAR(svt->svt_text[0])) {
            svstat = svare_set_error(svx, SVARERR_EXP_STMT_TERMINATOR,
                "Expecting statement separator on %s. Found: %s",
                "return", svt->svt_text);
        } else {
            svstat = svare_set_svarvalue(svx, RETURN_VARIABLE_NAME, &svv);
            while (svx->svx_nsvs > 1 &&
                   svx->svx_svs[svx->svx_nsvs - 1]->svs_xstate != XSTAT_FUNCTION) {
                svstat = svare_pop_xstat(svx, 0);
            }
            if (!svstat) {
                svstat = svare_push_xstat(svx, XSTAT_RETURN);
                svstat = SVARERR_RETURN;
            }
        }
    }

    return (svstat);
}
*/
    int jstat = 0;
    struct jvarvalue * vjvv;
#if STORE_ON_RETURN == 11
    struct jvarvalue   rtnjvv;
    struct jvarvalue * rjvv;
#elif STORE_ON_RETURN == 1
    struct jvarvalue   rtnjvv;
#elif STORE_ON_RETURN == 2
    struct jvarvalue   rtnjvv;
#endif

    if ((*pjtok)->jtok_kw != JSPU_SEMICOLON) {
        vjvv = jrun_get_return_jvv(jx);
#if STORE_ON_RETURN == 11   /* Old one - passes tests */
        jstat = jexp_evaluate(jx, pjtok, &rtnjvv);
        if (!jstat) {
            jstat = jexp_get_rval(jx, &rjvv, &rtnjvv);
            if (!jstat) COPY_JVARVALUE(vjvv, rjvv);
        }
        //jvar_free_jvarvalue_data(&rtnjvv);
#elif STORE_ON_RETURN == 1
        jstat = jexp_evaluate(jx, pjtok, &rtnjvv);
        if (!jstat) {
            COPY_JVARVALUE(vjvv, &rtnjvv);
        }
#elif STORE_ON_RETURN == 2
        jstat = jexp_evaluate(jx, pjtok, &rtnjvv);
        if (!jstat) {
            //jstat = jvar_store_jvarvalue(jx, vjvv, &rtnjvv);
            COPY_JVARVALUE(vjvv, &rtnjvv);
        }
#else
        jstat = jexp_evaluate(jx, pjtok, vjvv);
#endif
    }

    if (!jstat) {
        if ((*pjtok)->jtok_kw != JSPU_SEMICOLON) {
            jstat = jrun_set_error(jx, errtyp_SyntaxError, JSERR_EXP_SEMICOLON,
                "Expecting semicolon. Found: '%s'", (*pjtok)->jtok_text);
        } else {
            jstat = JERR_RETURN;
        }
    }

    return (jstat);
}
/***************************************************************/
#ifdef OLD_WAY
int jexp_eval_rval(struct jrunexec * jx,
    struct jvarvalue * jvv,
    struct jvarvalue ** prtnjvv)
{
/*
** 10/25/2021
*/
    int jstat = 0;

    if (jvv->jvv_dtype == JVV_DTYPE_LVAL) {
        (*prtnjvv) = jvv->jvv_val.jvv_lval.jvvv_lval;
    } else if (jvv->jvv_dtype == JVV_DTYPE_DYNAMIC) {
        jstat = (jvv->jvv_val.jvv_val_dynamic->jvvy_get_proc)(jx,
            jvv->jvv_val.jvv_val_dynamic, NULL, prtnjvv);
        /* free_jvarvalue_dynamic(jvv->jvv_val.jvv_val_dynamic);    */
        /* INIT_JVARVALUE(jvv);                                     */
    } else {
        (*prtnjvv) = jvv;
    }

    return (jstat);
}
#endif
/***************************************************************/
int jexp_get_rval(struct jrunexec * jx,
    struct jvarvalue ** ptgtjvv,
    struct jvarvalue * srcjvv)
{
/*
** 10/25/2021 - 03/31/2022
*/
    int jstat = 0;

    if (srcjvv->jvv_dtype == JVV_DTYPE_LVAL) {
        (*ptgtjvv) = srcjvv->jvv_val.jvv_lval.jvvv_lval;
    } else if (srcjvv->jvv_dtype == JVV_DTYPE_DYNAMIC) {
        jstat = (srcjvv->jvv_val.jvv_val_dynamic->jvvy_get_proc)(jx,
            srcjvv->jvv_val.jvv_val_dynamic, NULL, ptgtjvv);
    } else {
        (*ptgtjvv) = srcjvv;
    }

    return (jstat);
}
/***************************************************************/
#ifdef OLD_WAY
static int jvar_parse_function_operand(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    struct jvarvalue * jvv)
{
/*
** 01/29/2022
*/
    int jstat = 0;
    struct jvarvalue tjvv;

    jstat = jrun_next_token(jx, pjtok);
    INIT_JVARVALUE(&tjvv);
    if (!jstat) jstat = jvar_parse_function(jx, pjtok, &tjvv, SKIP_FLAG_HOIST);
    if (!jstat && tjvv.jvv_dtype == JVV_DTYPE_FUNCTION) {
        jvv->jvv_dtype = JVV_DTYPE_FUNCVAR;
        jvv->jvv_val.jvv_val_funcvar.jvvfv_jvvf = tjvv.jvv_val.jvv_val_function;
        jvv->jvv_val.jvv_val_funcvar.jvvfv_var_jcx = jvar_get_head_jcontext(jx);
    }

    return (jstat);
}
#endif
static int jvar_parse_function_operand(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    struct jvarvalue * jvv)
{
/*
** 01/29/2022
*/
    int jstat = 0;
    struct jtokeninfo * jti_func_oper;

    jti_func_oper = (*pjtok)->jtok_jti;
#if IS_DEBUG
    if (!jti_func_oper ||
         jti_func_oper->jti_type != JTI_TYPE_FUNCTION_OPERAND ||
        !jti_func_oper->jti_val.jtifo || 
         jti_func_oper->jti_val.jtifo->jtifo_fjvv.jvv_dtype != JVV_DTYPE_FUNCTION || 
        !jti_func_oper->jti_val.jtifo->jtifo_end_jxc.jxc_jtl) {
        jstat = jrun_set_error(jx, errtyp_InternalError, JSERR_INTERNAL_ERROR,
            "Internal error: Invalid jtokeninfo on function operand.");
        return (jstat);
    }
#endif
    jvv->jvv_dtype = JVV_DTYPE_FUNCVAR;
    jvv->jvv_val.jvv_val_funcvar.jvvfv_jvvf =
        jti_func_oper->jti_val.jtifo->jtifo_fjvv.jvv_val.jvv_val_function;
    jvv->jvv_val.jvv_val_funcvar.jvvfv_var_jcx = jvar_get_head_jcontext(jx);

    jrun_set_current_jxc(jx, &(jti_func_oper->jti_val.jtifo->jtifo_end_jxc));
    jstat = jrun_next_token(jx, pjtok);

    return (jstat);
}
/***************************************************************/
static int jexp_eval_primary(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    struct jvarvalue * jvv)
{
/*
**
#define JSW_UNKNOWN     0
#define JSW_KW          1
#define JSW_PU          2
#define JSW_NUMBER      3
#define JSW_ID          4
#define JSW_STRING      5
#define JSW_ERROR       6
*/
    int jstat = 0;
    int cstat;
    struct jvarvalue * vjvv;
    struct jcontext * var_jcx;

    INIT_JVARVALUE(jvv);
    switch ((*pjtok)->jtok_typ) {
        case JSW_STRING:
            /* jtok_text has beginning and ending quatation marks */
            jvar_store_chars_len(jvv, (*pjtok)->jtok_text + 1, (*pjtok)->jtok_tlen - 2);
            jstat = jrun_next_token(jx, pjtok);
            break;
        case JSW_NUMBER:
            cstat = jvar_chars_to_number(jvv, (*pjtok)->jtok_text);
            if (cstat) {
                jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_INVALID_NUMBER,
                    "Invalid number: %s", (*pjtok)->jtok_text);
            } else {
                jstat = jrun_next_token(jx, pjtok);
            }
            break;

        case JSW_KW:
            switch ((*pjtok)->jtok_kw) {
                case JSKW_FALSE:
                    jvar_store_bool(jvv, 0);
                    jstat = jrun_next_token(jx, pjtok);
                    break;
                case JSKW_TRUE:
                    jvar_store_bool(jvv, 1);
                    jstat = jrun_next_token(jx, pjtok);
                    break;
                case JSKW_NULL:
                    jvar_store_null(jvv);
                    jstat = jrun_next_token(jx, pjtok);
                    break;
                case JSKW_FUNCTION:
                    jstat = jvar_parse_function_operand(jx, pjtok, jvv);
                    break;
                default:
                    jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_UNEXPECTED_KEYWORD,
                        "Unexpected keyword: %s", (*pjtok)->jtok_text);
                break;
            }
            break;

        case JSW_ID:
            vjvv = jvar_find_variable(jx, (*pjtok), &var_jcx);
            if (vjvv) {
                jvar_store_lval(jx, jvv, vjvv, var_jcx);
            } else {
                jstat = jrun_set_error(jx, errtyp_ReferenceError, JSERR_UNDEFINED_VARIABLE,
                    "Undefined variable: %s", (*pjtok)->jtok_text);
            }
            if (!jstat) jstat = jrun_next_token(jx, pjtok);
            break;

        case JSW_PU:
            switch ((*pjtok)->jtok_kw) {
                case JSPU_OPEN_BRACKET:
                    jstat = jrun_next_token(jx, pjtok);
                    if (!jstat) jstat = jexp_parse_array(jx, pjtok, jvv);
                    break;

                case JSPU_OPEN_BRACE:
                    jstat = jrun_next_token(jx, pjtok);
                    if (!jstat) jstat = jexp_parse_object(jx, pjtok, jvv);
                    break;

                default:
                    jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_UNEXPECTED_PUNCTUATOR,
                        "Unexpected punctuator: %s", (*pjtok)->jtok_text);
                    break;
            }
            break;

        default:
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_INVALID_TOKEN_TYPE,
                "Invalid token type %d: %s", (*pjtok)->jtok_typ, (*pjtok)->jtok_text);
            break;
    }

    return (jstat);
}
/***************************************************************/
#if ! PREP_INACTIVE_EXPRESSIONS
static int jexp_eval_primary_inactive(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok)
{
/*
**
**
** See also: jprep_eval_primary()
*/
    int jstat = 0;
    struct jtoken next_jtok;

    switch ((*pjtok)->jtok_typ) {
        case JSW_STRING:
            jstat = jrun_next_token(jx, pjtok);
            break;

        case JSW_NUMBER:
            jstat = jrun_next_token(jx, pjtok);
            break;

        case JSW_KW:
            if ((*pjtok)->jtok_kw == JSKW_FUNCTION) {
                //jstat = jrun_next_token(jx, pjtok);
                if (!jstat) jstat = jrun_exec_function_stmt(jx, pjtok);
            } else {
                jstat = jrun_next_token(jx, pjtok);
            }
            break;

        case JSW_ID:
            jrun_peek_token(jx, &next_jtok);
            if (next_jtok.jtok_kw == JSPU_OPEN_PAREN) {
                jstat = jprep_eval_function_call(jx, pjtok, 0);
            } else {
                jstat = jrun_next_token(jx, pjtok);
            }
            break;

        case JSW_PU:
            switch ((*pjtok)->jtok_kw) {
                case JSPU_OPEN_PAREN:
                    jstat = jrun_next_token(jx, pjtok);
                    break;

                case JSPU_OPEN_BRACKET:
                    jstat = jrun_next_token(jx, pjtok);
                    if (!jstat) jstat = jexp_parse_array_inactive(jx, pjtok);
                    break;

                case JSPU_OPEN_BRACE:
                    jstat = jrun_next_token(jx, pjtok);
                    if (!jstat) jstat = jexp_parse_object_inactive(jx, pjtok);
                    break;

                default:
                    jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_UNEXPECTED_PUNCTUATOR,
                        "Unexpected 1 punctuator: %s", (*pjtok)->jtok_text);
                    break;
            }
            break;

        default:
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_INVALID_TOKEN_TYPE,
                "Invalid token type %d: %s", (*pjtok)->jtok_typ, (*pjtok)->jtok_text);
            break;
    }

    return (jstat);
}
/***************************************************************/
#else
static int jexp_eval_primary_inactive(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok)
{
/*
** 02/02/2022
*/
    int jstat = 0;

    jstat = jprep_eval_primary(jx, pjtok, 0);

    return (jstat);
}
#endif
/***************************************************************/
static struct jvarvalue_vallist * jva_new_jvarvalue_vallist(void)
{
/*
**
*/
    struct jvarvalue_vallist * jvvl;

    jvvl = New(struct jvarvalue_vallist, 1);
    jvvl->jvvl_nvals = 0;
    jvvl->jvvl_xvals = 0;
    jvvl->jvvl_avals = NULL;

    return (jvvl);
}
/***************************************************************/
static void jvar_add_jvarvalue_vallist(
    struct jvarvalue_vallist * jvvl,
    struct jvarvalue * jvv)
{
/*
**
*/
    if (jvvl->jvvl_nvals == jvvl->jvvl_xvals) {
        if (!jvvl->jvvl_xvals) jvvl->jvvl_xvals = 4;
        else jvvl->jvvl_xvals *= 2;
        jvvl->jvvl_avals = Realloc(jvvl->jvvl_avals, struct jvarvalue, jvvl->jvvl_xvals);
    }
    COPY_JVARVALUE(&(jvvl->jvvl_avals[jvvl->jvvl_nvals]), jvv);
    jvvl->jvvl_nvals += 1;
}
/***************************************************************/
#if 0
static void jvvl_quick_free_jvarvalue_vallist(struct jvarvalue_vallist * jvvlargs)
{
/*
**
*/
    Free(jvvlargs->jvvl_avals);
    Free(jvvlargs);
}
#endif
/***************************************************************/
int jexp_assign_parameters(
                    struct jrunexec * jx,
                    struct jvarvalue_function * jvvf,
                    struct jvarvalue * jvvparms)
{
/*
** 07/13/2021
*/
    int jstat = 0;
    struct jfuncstate * jxfs;
    struct jvarvalue * jvvsrc;
    int parmix;
    int formix;

    jxfs = jx->jx_jfs[jx->jx_njfs - 1]; /* See: jvar_get_current_jfuncstate(jx); */
    if (jvvparms->jvv_dtype != JVV_DTYPE_VALLIST) {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_INTERNAL_ERROR,
            "Expecting vallist for function arguments.");
    } else {
        parmix = 0;
        while (parmix < jvvf->jvvf_nargs) {
            formix = jvvf->jvvf_aargs[parmix]->jvvfa_vix;
            if (formix < jvvparms->jvv_val.jvv_jvvl->jvvl_nvals) {
                jvvsrc = &(jvvparms->jvv_val.jvv_jvvl->jvvl_avals[parmix]);
                if (jvvsrc->jvv_dtype == JVV_DTYPE_LVAL) {
                    jvvsrc = jvvsrc->jvv_val.jvv_lval.jvvv_lval;
                }
                /* jstat = jvar_store_jvarvalue_n(jx, &(jxfs->jfs_jcx->jcx_ajvv[formix]), jvvsrc); */
                jstat = jvar_store_jvarvalue(jx, jxfs->jfs_jcx->jcx_ajvv + formix, jvvsrc);
            }
            parmix++;
        }
    }

    return (jstat);
}
/***************************************************************/
int jexp_call_function(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    struct jvarvalue_function * jvvf,
                    struct jvarvalue_int_object * this_obj,
                    struct jvarvalue * jvvparms,
                    struct jcontext * outer_jcx,
                    struct jvarvalue * rtnjvv)
{
/*
** 02/17/2021
*/
    int jstat = 0;
    int jstat2 = 0;
    int is_eof = 0;
    struct jxcursor current_jxc;
    struct jvarvalue * vjvv;

    jrun_get_current_jxc(jx, &current_jxc);

    jrun_push_xstat(jx, XSTAT_FUNCTION);

    jrun_set_xstat_jxc(jx, &current_jxc);

    if (!jstat && (jvvf->jvvf_flags & JVVF_FLAG_RUN_TO_EOF)) {
        jstat = jrun_push_xstat(jx, XSTAT_ACTIVE_BLOCK);
        XSTAT_FLAGS(jx) |= JXS_FLAGS_BEGIN_FUNC;
    }

    jstat = jrun_push_jfuncstate(jx, jvvf, outer_jcx);  /* sets cursor */

    if (!jstat) jstat = jexp_assign_parameters(jx, jvvf, jvvparms);

    if (!jstat) jstat = jrun_next_token(jx, pjtok);

    if (!jstat) jstat = jrun_exec_block(jx, pjtok);
    
//  #if DEBUG_FUNCTION_CALLS
//      if (fjvv->jvv_dtype == JVV_DTYPE_FUNCTION) {
//          printf("Function exit: %s() stat=%d\n",
//              fjvv->jvv_val.jvv_val_function->jvvf_func_name, jstat);
//      } else {
//          printf("Function exit ? stat=%d\n", jstat);
//      }
//  #endif

    if ((jstat == JERR_RETURN) || (jstat == JERR_EXIT)) {
        if (jstat == JERR_EXIT) is_eof = 1;
        vjvv = jrun_get_return_jvv(jx);
#if 0
        COPY_JVARVALUE(rtnjvv, vjvv);
#elif STORE_ON_RETURN == 2
        jstat = jvar_assign_jvarvalue(jx, rtnjvv, vjvv);  /* 03/10/2022 */
        jvar_free_jvarvalue_data(vjvv);
#else
        jstat = jvar_assign_jvarvalue(jx, rtnjvv, vjvv);  /* 03/10/2022 */
#endif
    } else if (jstat == JERR_EOF && (jvvf->jvvf_flags & JVVF_FLAG_RUN_TO_EOF)) {
        if (jstat == JERR_EOF) is_eof = 1;
        jstat = 0;
    }

    while (!jstat2 && !XSTAT_IS_RETURNABLE(jx)) {
        jstat2 = jrun_pop_xstat(jx);
    }

    //jvar_pop_vars(jx);

    if (!jstat2) jstat2 = jrun_pop_jfuncstate(jx);

    if (!jstat2) jstat2 = jrun_update_xstat(jx);

    if (!jstat2 && !is_eof) jstat2 = jrun_next_token(jx, pjtok);

    if (!jstat) jstat = jstat2;

    return (jstat);
}
/***************************************************************/
static int jexp_method_call(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    struct jvarvalue * fjvv,
                    struct jvarvalue * jvvparms,
                    struct jvarvalue * rtnjvv)
{
/*
**
*/
    int jstat = 0;
    struct jvarvalue * jvvargs;
    struct jvarvalue   jvvargrec;
    struct jvarvalue_vallist jvvl;
    struct jcontext * jcx;
    struct jvarvalue_function * jvvf;
    struct jvarvalue_int_object   * this_obj;
    struct jvarvalue  * jvvfunc;
    struct jvarvalue  * mjvv;

    if (jvvparms->jvv_dtype == JVV_DTYPE_VALLIST) {
        jvvargs = jvvparms;
    } else {
        if (jvvparms->jvv_dtype == JVV_DTYPE_NONE) {
            jvvl.jvvl_nvals = 0;
            jvvl.jvvl_xvals = 0;
            jvvl.jvvl_avals = NULL;
        } else {
            jvvl.jvvl_nvals = 1;
            jvvl.jvvl_xvals = 1;
            jvvl.jvvl_avals = jvvparms;
        }
        jvvargrec.jvv_dtype = JVV_DTYPE_VALLIST;
        jvvargrec.jvv_val.jvv_jvvl = &jvvl;
        jvvargs = &jvvargrec;
    }
    INIT_JVARVALUE(rtnjvv);

    jcx = NULL;

    if (fjvv->jvv_dtype == JVV_DTYPE_LVAL) {
        mjvv = fjvv->jvv_val.jvv_lval.jvvv_lval;
        jcx = fjvv->jvv_val.jvv_lval.jvvv_var_jcx;
    } else {
        mjvv = fjvv;
    }

#ifdef OLD_WAY
    if (mjvv->jvv_dtype == JVV_DTYPE_FUNCVAR) {
        jvvf = mjvv->jvv_val.jvv_val_funcvar.jvvfv_jvvf;
        jcx = mjvv->jvv_val.jvv_val_funcvar.jvvfv_var_jcx;
        this_obj = mjvv->jvv_val.jvv_val_funcvar.jvvfv_this_obj;
        jstat = jexp_call_function(jx, pjtok, jvvf, this_obj, jvvargs, jcx, rtnjvv);
    } else if (mjvv->jvv_dtype == JVV_DTYPE_IMETHVAR) {
        jstat = jexp_internal_method_call(jx, mjvv->jvv_val.jvv_val_imethvar.jvvimv_jvvim,
            mjvv->jvv_val.jvv_val_imethvar.jvvimv_this_ptr, jvvargs, rtnjvv);
    } else if (mjvv->jvv_dtype == JVV_DTYPE_INTERNAL_METHOD) {
        jstat = jexp_internal_method_call(jx, mjvv->jvv_val.jvv_int_method, NULL, jvvargs, rtnjvv);
    } else if (mjvv->jvv_dtype == JVV_DTYPE_FUNCTION) {
        if (!jcx) jcx = jvar_get_head_jcontext(jx);
        jstat = jexp_call_function(jx, pjtok, mjvv->jvv_val.jvv_val_function, NULL, jvvargs, jcx, rtnjvv);
    } else if (mjvv->jvv_dtype == JVV_DTYPE_DYNAMIC) {
        jstat = (mjvv->jvv_val.jvv_val_dynamic->jvvy_get_proc)(jx,
                    mjvv->jvv_val.jvv_val_dynamic, NULL, &jvvfunc);
        if (!jstat) {
            //jstat = jexp_call_function(jx, pjtok, mjvv->jvv_val.jvv_val_function, NULL, jvvargs, jcx, rtnjvv);
            jstat = jexp_method_call(jx, pjtok, jvvfunc, jvvparms, rtnjvv);
        }
    } else if (mjvv->jvv_dtype == JVV_DTYPE_LVAL) {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_INTERNAL_ERROR,
            "Found lval for function or method.");
    } else {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_EXP_METHOD,
            "Expecting function or method. Found: %s",
                jvar_get_dtype_name(mjvv->jvv_dtype));
    }
#else
    switch (mjvv->jvv_dtype) {
        case JVV_DTYPE_FUNCVAR:
            jvvf = mjvv->jvv_val.jvv_val_funcvar.jvvfv_jvvf;
            jcx = mjvv->jvv_val.jvv_val_funcvar.jvvfv_var_jcx;
            this_obj = mjvv->jvv_val.jvv_val_funcvar.jvvfv_this_obj;
            jstat = jexp_call_function(jx, pjtok, jvvf, this_obj, jvvargs, jcx, rtnjvv);
#if FIX_220311
            jvar_free_jvarvalue_data(fjvv);
#endif
            break;

        case JVV_DTYPE_IMETHVAR:
            jstat = jexp_internal_method_call(jx, mjvv->jvv_val.jvv_val_imethvar.jvvimv_jvvim,
                mjvv->jvv_val.jvv_val_imethvar.jvvimv_this_ptr, jvvargs, rtnjvv);
#if FIX_220311
            jvar_free_jvarvalue_data(fjvv);
#endif
            break;

        case JVV_DTYPE_INTERNAL_METHOD:
            jstat = jexp_internal_method_call(jx, mjvv->jvv_val.jvv_int_method, NULL, jvvargs, rtnjvv);
#if FIX_220311
            jvar_free_jvarvalue_data(fjvv);
#endif
            break;

        case JVV_DTYPE_FUNCTION:
            if (!jcx) jcx = jvar_get_head_jcontext(jx);
            jstat = jexp_call_function(jx, pjtok, mjvv->jvv_val.jvv_val_function, NULL, jvvargs, jcx, rtnjvv);
#if FIX_220311
            jvar_free_jvarvalue_data(fjvv);
#endif
            break;

        case JVV_DTYPE_DYNAMIC:
            jstat = (mjvv->jvv_val.jvv_val_dynamic->jvvy_get_proc)(jx,
                        mjvv->jvv_val.jvv_val_dynamic, NULL, &jvvfunc);
            if (!jstat) {
                jstat = jexp_method_call(jx, pjtok, jvvfunc, jvvparms, rtnjvv);
            }
#if FIX_220311
            jvar_free_jvarvalue_data(fjvv);
#endif
            break;

        case JVV_DTYPE_LVAL:
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_INTERNAL_ERROR,
                "Found lval for function or method.");
            break;

        default:
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_EXP_METHOD,
                "Expecting function or method. Found: %s",
                    jvar_get_dtype_name(mjvv->jvv_dtype));
            break;
    }
#endif

    jvar_free_jvarvalue_data(jvvparms);

    return (jstat);
}
/***************************************************************/
static int jexp_binop_call(struct jrunexec * jx,
    struct jvarvalue * jvv,
    void             * this_ptr,
    struct jvarvalue * jvvargs,
    char * method_name,
    struct jvarvalue * rtnjvv)
{
/*
** 10/06/2021
*/
    int jstat = 0;
    struct jvarvalue * mjvv;

    switch (jvv->jvv_dtype) {
        case JVV_DTYPE_INTERNAL_CLASS   :
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_UNSUPPORTED_OPERATION,
                "Dot on internal class not supported.");
            break;

        case JVV_DTYPE_INTERNAL_OBJECT   :
            mjvv = jvar_int_object_member(jx, jvv, method_name);
            if (!mjvv) {
                jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_UNDEFINED_CLASS_MEMBER,
                    "Undefined class member: %s", method_name);
            } else if (mjvv->jvv_dtype == JVV_DTYPE_INTERNAL_METHOD) {
                if (mjvv->jvv_flags & (JCX_FLAG_PROPERTY | JCX_FLAG_OPERATION)) {
                    jstat = jexp_internal_method_call(jx, mjvv->jvv_val.jvv_int_method, this_ptr, jvvargs, rtnjvv);
                    if (!jstat) {
                        jvar_free_jvarvalue_data(jvvargs);
                    }
                } else {
                    rtnjvv->jvv_dtype = JVV_DTYPE_IMETHVAR;
                    rtnjvv->jvv_val.jvv_val_imethvar.jvvimv_jvvim = mjvv->jvv_val.jvv_int_method;
#if USE_JVARVALUE_IMETHVAR
                    rtnjvv->jvv_val.jvv_val_imethvar.jvvimv_this_jvv = NULL;
#endif
                    rtnjvv->jvv_val.jvv_val_imethvar.jvvimv_this_ptr = this_ptr;
                    INCIMETHREFS(mjvv->jvv_val.jvv_int_method);
                }
            } else {
                jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_UNSUPPORTED_OPERATION,
                    "Unsupported class member type: %s", method_name);
            }
            break;

        default:
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_EXP_OBJECT_AFTER_DOT,
                "Expecting object after dot.");
            break;
    }

    return (jstat);
}
/***************************************************************/
static int jexp_binop_type_bracket(struct jrunexec * jx,
    int dtype,
    void * this_ptr,
    struct jvarvalue * jvvparms,
    struct jvarvalue * rtnjvv)
{
/*
** 10/26/2021
*/
    int jstat = 0;
    int tix;
    struct jvarvalue * jvv;

    tix = JVV_DTYPE_INDEX(dtype);

    if (tix < 0 || tix >= JVV_NUM_DTYPES) {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_INTERNAL_ERROR,
            "Invalid type index. Type=%d", dtype);
    } else {
        jvv = jx->jx_type_objs[tix];
        if (!jvv) {
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_INVALID_TYPE,
                "Type has no elements. Type=%s", jvar_get_dtype_name(dtype));
        } else {
            jstat = jexp_binop_call(jx, jvv, this_ptr, jvvparms, JVV_BRACKET_OPERATION, rtnjvv);
        }
    }

    return (jstat);
}
/***************************************************************/
static int jexp_eval_subscript(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    struct jvarvalue * mjvv,
                    struct jvarvalue * jvvparms,
                    struct jvarvalue * rtnjvv)
{
/*
** 10/21/2021
        case JVV_DTYPE_OBJPTR   :
            switch (cjvv->jvv_val.jvv_objptr.jvvp_dtype) {
                case JVV_DTYPE_ARRAY   :
                    jstat = jexp_binop_type_dot(jx, JVV_DTYPE_ARRAY,
                        cjvv->jvv_val.jvv_objptr.jvvp_val.jvvp_val_array, ajvv1, ajvv2);
*/
    int jstat = 0;
    struct jvarvalue * cjvv;

    jstat = jexp_get_rval(jx, &cjvv, mjvv);
    if (jstat) return (jstat);

    if (cjvv->jvv_dtype == JVV_DTYPE_OBJPTR) {
        switch (cjvv->jvv_val.jvv_objptr.jvvp_pval->jvv_dtype) {
            case JVV_DTYPE_ARRAY   :
                jstat = jexp_binop_type_bracket(jx, JVV_DTYPE_ARRAY,
                    cjvv->jvv_val.jvv_objptr.jvvp_pval->jvv_val.jvv_val_array, jvvparms, rtnjvv);
                break;
            default:
                jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_INVALID_STORE,
                    "Illegal object pointer subscript %s",
                    jvar_get_dtype_name(cjvv->jvv_val.jvv_objptr.jvvp_pval->jvv_dtype));
                break;
        }
    } else if (cjvv->jvv_dtype == JVV_DTYPE_ARRAY) {
        jstat = jexp_binop_type_bracket(jx, JVV_DTYPE_ARRAY,
            cjvv->jvv_val.jvv_val_array, jvvparms, rtnjvv);
    } else {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_INVALID_TYPE,
            "No bracket support. Type=%s", jvar_get_dtype_name(cjvv->jvv_dtype));
    }
    //if (!jstat) jstat = jrun_next_token(jx, pjtok);

    return (jstat);
}
/***************************************************************/
static int jexp_binop_type_dot(struct jrunexec * jx,
    int dtype,
    void * this_ptr,
    struct jvarvalue * ajvv1,
    struct jvarvalue * ajvv2)
{
/*
** 10/06/2021
*/
    int jstat = 0;
    int tix;
    struct jvarvalue * jvv;
    struct jvarvalue   rtnjvv;

    tix = JVV_DTYPE_INDEX(dtype);

    if (tix < 0 || tix >= JVV_NUM_DTYPES) {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_INTERNAL_ERROR,
            "Invalid type index. Type=%d", dtype);
    } else {
        jvv = jx->jx_type_objs[tix];
        if (!jvv) {
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_INVALID_TYPE,
                "Type has no elements. Type=%s", jvar_get_dtype_name(dtype));
        } else {
            INIT_JVARVALUE(&rtnjvv);
            jstat = jexp_binop_call(jx, jvv, this_ptr, ajvv1, ajvv2->jvv_val.jvv_val_token.jvvt_token, &rtnjvv);
#if USE_JVARVALUE_IMETHVAR
            if (rtnjvv.jvv_dtype == JVV_DTYPE_IMETHVAR) {
                rtnjvv.jvv_val.jvv_val_imethvar.jvvimv_this_jvv = jvar_new_jvarvalue();
                //jstat = jvar_store_jvarvalue(jx, rtnjvv.jvv_val.jvv_val_imethvar.jvvimv_this_jvv, ajvv1);
                COPY_JVARVALUE(rtnjvv.jvv_val.jvv_val_imethvar.jvvimv_this_jvv, ajvv1);
            }
#else
            jvar_free_jvarvalue_data(ajvv1);    /* 03/15/2022 */
#endif
            COPY_JVARVALUE(ajvv1, &rtnjvv);
            jvar_free_jvarvalue_data(ajvv2);
        }
    }

    return (jstat);
}
/***************************************************************/
static int jexp_binop_dot(struct jrunexec * jx,
    struct jvarvalue * ajvv1,
    struct jvarvalue * ajvv2)
{
/*
** 09/30/2021
*/
    int jstat = 0;
    struct jvarvalue * cjvv;
    struct jvarvalue * mjvv;

    if (ajvv2->jvv_dtype != JVV_DTYPE_TOKEN) {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_EXP_ID_AFTER_DOT,
            "Expecting identifier after dot.");
        return (jstat);
    }

    jstat = jexp_get_rval(jx, &cjvv, ajvv1);
    if (jstat) return (jstat);

    switch (cjvv->jvv_dtype) {
        case JVV_DTYPE_INTERNAL_CLASS   :
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_UNSUPPORTED_OPERATION,
                "Dot on internal class not supported.");
            break;

        case JVV_DTYPE_INTERNAL_OBJECT   :
            mjvv = jvar_int_object_member(jx, cjvv, ajvv2->jvv_val.jvv_val_token.jvvt_token);
            if (!mjvv) {
                jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_UNDEFINED_CLASS_MEMBER,
                    "Undefined class member: %s",
                    ajvv2->jvv_val.jvv_val_token.jvvt_token);
            } else if (mjvv->jvv_dtype == JVV_DTYPE_INTERNAL_METHOD) {
                ajvv1->jvv_dtype = JVV_DTYPE_IMETHVAR;
                ajvv1->jvv_val.jvv_val_imethvar.jvvimv_jvvim = mjvv->jvv_val.jvv_int_method;
#if USE_JVARVALUE_IMETHVAR
                ajvv1->jvv_val.jvv_val_imethvar.jvvimv_this_jvv = NULL;
#endif
                ajvv1->jvv_val.jvv_val_imethvar.jvvimv_this_ptr = cjvv->jvv_val.jvv_jvvo->jvvo_this_ptr;
                INCIMETHREFS(mjvv->jvv_val.jvv_int_method);
            } else {
                jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_UNSUPPORTED_OPERATION,
                    "Unsupported class member type: %s",
                    ajvv2->jvv_val.jvv_val_token.jvvt_token);
            }
            jvar_free_jvarvalue_data(ajvv2);
            break;

        case JVV_DTYPE_CHARS   :
#if USE_JVV_CHARS_POINTER
            jstat = jexp_binop_type_dot(jx, cjvv->jvv_dtype, cjvv->jvv_val.jvv_val_chars, ajvv1, ajvv2);
#else
            jstat = jexp_binop_type_dot(jx, cjvv->jvv_dtype, &(cjvv->jvv_val.jvv_val_chars), ajvv1, ajvv2);
#endif
            break;

        case JVV_DTYPE_OBJPTR   :
            switch (cjvv->jvv_val.jvv_objptr.jvvp_pval->jvv_dtype) {
                case JVV_DTYPE_ARRAY   :
                    jstat = jexp_binop_type_dot(jx, JVV_DTYPE_ARRAY,
                        cjvv->jvv_val.jvv_objptr.jvvp_pval->jvv_val.jvv_val_array, ajvv1, ajvv2);
                    break;
                case JVV_DTYPE_OBJECT   :
                    // old way - jstat = jexp_binop_dot_object(jx, cjvv, cjvv->jvv_val.jvv_objptr.jvvp_pval->jvv_val.jvv_val_object, ajvv2);
                    jstat = jexp_binop_dot_object(jx, ajvv1, cjvv->jvv_val.jvv_objptr.jvvp_pval->jvv_val.jvv_val_object, ajvv2);
                    break;
                default:
                    jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_INVALID_STORE,
                        "Illegal object pointer dot %s",
                        jvar_get_dtype_name(cjvv->jvv_val.jvv_objptr.jvvp_pval->jvv_dtype));
                    break;
            }
            break;

        case JVV_DTYPE_OBJECT   :
            // old way - jstat = jexp_binop_dot_object(jx, cjvv, cjvv->jvv_val.jvv_val_object, ajvv2);
            jstat = jexp_binop_dot_object(jx, ajvv1, cjvv->jvv_val.jvv_val_object, ajvv2);
            break;

        default:
#if USE_JVV_CHARS_POINTER
            jstat = jexp_binop_type_dot(jx, cjvv->jvv_dtype, cjvv->jvv_val.jvv_val_chars, ajvv1, ajvv2);
#else
            jstat = jexp_binop_type_dot(jx, cjvv->jvv_dtype, &(cjvv->jvv_val.jvv_val_chars), ajvv1, ajvv2);
#endif
            /*
            ** 2/15/2022 - Replaced with jexp_binop_type_dot()
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_EXP_OBJECT_AFTER_DOT,
                "Expecting object after dot.");
            */
            break;
    }

    return (jstat);
}
/***************************************************************/
/* operator list                                               */
struct jexp_exp_rec jexp_exp_list[] = {
    {  JX_DOT_PREC, 0, JSPU_DOT      , 68, jexp_binop_dot     , NULL                },
    {  0, 12, JSPU_OPEN_BRACKET      ,512, NULL               , NULL                },    /* 02/18/2022 */
    {  0, 12, JSPU_PLUS_PLUS         ,514, NULL               , jexp_pre_increment  },    /* 02/22/2022 */
    { 11,  0, JSPU_STAR_STAR         ,  5, jexp_binop_pow     , NULL                },
    {  0, 10, JSPU_BANG              ,  2, NULL               , jexp_unop_lognot    },
    {  9,  0, JSPU_STAR              ,  4, jexp_binop_mul     , NULL                },
    {  9,  0, JSPU_SLASH             ,  4, jexp_binop_div     , NULL                },
    {  8, 10, JSPU_PLUS              ,  6, jexp_binop_add     , jexp_unop_pos       },
    {  8, 10, JSPU_MINUS             ,  6, jexp_binop_sub     , jexp_unop_neg       },
    {  7,  0, JSPU_EQ_EQ             ,  4, jexp_binop_cmp_eq  , NULL                },
    {  7,  0, JSPU_BANG_EQ           ,  4, jexp_binop_cmp_ne  , NULL                },
    {  7,  0, JSPU_LT_EQ             ,  4, jexp_binop_cmp_le  , NULL                },
    {  7,  0, JSPU_LT                ,  4, jexp_binop_cmp_lt  , NULL                },
    {  7,  0, JSPU_GT                ,  4, jexp_binop_cmp_gt  , NULL                },
    {  7,  0, JSPU_GT_EQ             ,  4, jexp_binop_cmp_ge  , NULL                },
    {  6,  0, JSPU_AMP               ,  4, jexp_binop_bitand  , NULL                },
    {  5,  0, JSPU_HAT               ,  4, jexp_binop_bitxor  , NULL                },
    {  4,  0, JSPU_BAR               ,  4, jexp_binop_bitor   , NULL                },
    {  3,  0, JSPU_AMP_AMP           , 20, jexp_binop_logand  , NULL                },
    {  2,  0, JSPU_BAR_BAR           , 12, jexp_binop_logor   , NULL                },
#if ! DEBUG_OLD_ASSIGN
    {  1,  0, JSPU_EQ                ,  5, jexp_binop_assign   , NULL               },
#endif
    {  0,  0, 0                      ,  0, NULL               , NULL                }
};
/***************************************************************/
#define EVAL_STACK_SZ   32

#define JEXPST_DONE             0
#define JEXPST_FIRST_OPERAND    1
#define JEXPST_NEXT_OPERAND     2
#define JEXPST_BINARY_OPERATOR  3
#define JEXPST_BINARY_IGNORED   4
#define JEXPST_FIRST_IGNORED    5
#define JEXPST_FUNCTION_CALL    6

enum exs_info {
    XS_INFO_NONE = 0,
    XS_INFO_UNOP = 1,
    XS_INFO_OPEN_PAREN = 2,
    XS_INFO_OPERAND = 3,
    XS_INFO_BINOP = 4
};

struct expession_stack { /* xs_ */
    int xs_xlx;
    enum exs_info xs_info;
    int xs_ignore;
    struct jvarvalue xs_jvv;
};
#define COPY_XSREC(t,s) (memcpy((t), (s), sizeof(struct expession_stack)))
/***************************************************************/
struct jexp_exp_rec * jeval_get_exp_list(void) {

    return (jexp_exp_list);
}
/***************************************************************/
static void jeval_store_token(struct jrunexec * jx,
                    struct jtoken ** pjtok,
    struct jvarvalue * jvv)
{
/*
** 12/11/2020
*/
    jvv->jvv_dtype = JVV_DTYPE_TOKEN;
    jvv->jvv_val.jvv_val_token.jvvt_token = Strdup((*pjtok)->jtok_text);
}
/***************************************************************/
void jexp_quick_tostring(
                    struct jrunexec * jx,
                    struct jvarvalue * jvv,
                    char * valstr,
                    int valstrsz)
{
/*
**
*/
    int jstat = 0;
    int jfmtflags;
    char * prbuf = NULL;
    int prlen = 0;
    int prmax = 0;

    prbuf = NULL;
    prlen = 0;
    prmax = 0;
    jfmtflags = JFMT_ORIGIN_TOCHARS | JFMT_FLAG_SHOW_TYPE;

    jstat = jpr_jvarvalue_tostring(jx, &prbuf, &prlen, &prmax, jvv, jfmtflags);
    if (jstat) {
        strmcpy(valstr, "*Error*", valstrsz);
    } else if (prlen) {
        strmcpy(valstr, prbuf, valstrsz);
        Free(prbuf);
    } else {
        valstr[0] = 0;
    }
}
/***************************************************************/
void jexp_quick_print(
                    struct jrunexec * jx,
                    struct jvarvalue * jvv,
                    char * msg)
{
/*
**
*/
    int jstat = 0;
    int jfmtflags;
    char * prbuf = NULL;
    int prlen = 0;
    int prmax = 0;

    prbuf = NULL;
    prlen = 0;
    prmax = 0;
    jfmtflags = JFMT_ORIGIN_TOCHARS | JFMT_FLAG_SHOW_TYPE;

    jstat = jpr_jvarvalue_tostring(jx, &prbuf, &prlen, &prmax, jvv, jfmtflags);
    if (jstat) {
        append_charval(&prbuf, &prlen, &prmax, "*Error*");
    } else if (!prlen) {
        append_charval(&prbuf, &prlen, &prmax, "");
    }
    printf("%s%s\n", msg, prbuf);
    Free(prbuf);
}
/***************************************************************/
static void jexp_print_stack(
                    struct jrunexec * jx,
                    const char * msg,
                    struct expession_stack * xstack,
                    int depth,
                    short bin_precedence,
                    short opflags)
{
/*
**
*/
    int ii;
    int xlx;
    char prestr[8];
    char punam[16];
    char info[32];
    char valstr[64];
    char ignstr[32];
    char opflagstr[128];

    if (!opflags) {
        strcpy(opflagstr, "0");
    } else {
        opflagstr[0] = '\0';
        if (opflags & JXE_OPFLAG_RTOL_ASSOCIATIVITY) {
            if (opflagstr[0]) strcat(opflagstr, ",");
            strcat(opflagstr, "rtol associativity");
        }
        if (opflags & JXE_OPFLAG_UNARY_OPERATION) {
            if (opflagstr[0]) strcat(opflagstr, ",");
            strcat(opflagstr, "unary operator");
        }
        if (opflags & JXE_OPFLAG_BINARY_OPERATION) {
            if (opflagstr[0]) strcat(opflagstr, ",");
            strcat(opflagstr, "binary operator");
        }
        if (opflags & JXE_OPFLAG_LOGICAL_OR) {
            if (opflagstr[0]) strcat(opflagstr, ",");
            strcat(opflagstr, "logical or");
        }
        if (opflags & JXE_OPFLAG_LOGICAL_AND) {
            if (opflagstr[0]) strcat(opflagstr, ",");
            strcat(opflagstr, "logical and");
        }
        if (opflags & JXE_OPFLAG_CLOSE_PAREN) {
            if (opflagstr[0]) strcat(opflagstr, ",");
            strcat(opflagstr, "close paren");
        }
        if (opflags & JXE_OPFLAG_TOKEN) {
            if (opflagstr[0]) strcat(opflagstr, ",");
            strcat(opflagstr, "token");
        }
        if (opflags & JXE_OPFLAG_FUNCTION_CALL) {
            if (opflagstr[0]) strcat(opflagstr, ",");
            strcat(opflagstr, "func call");
        }
        //if (opflags & JXE_OPFLAG_OPEN_BRACKET) {
        //    if (opflagstr[0]) strcat(opflagstr, ",");
        //    strcat(opflagstr, "open bracket");
        //}
        if (opflags & JXE_OPFLAG_POST_UNARY_OPERATION) {
            if (opflagstr[0]) strcat(opflagstr, ",");
            strcat(opflagstr, "post unary op");
        }
    }

    printf("%s depth=%d precedence=%d flags=%s\n",
        msg, depth, bin_precedence, opflagstr);

    for (ii = depth - 1; ii >= 0; ii--) {
        prestr[0] = '\0';
        valstr[0] = '\0';

        xlx = xstack[ii].xs_xlx;
        if (xlx < 0) {
            punam[0] = '\0';
        } else {
            jrun_find_punctuator_name_by_pu(jexp_exp_list[xlx].jxe_pu, punam);
            if (xstack[ii].xs_info == XS_INFO_UNOP) {
                 sprintf(prestr, "%d", jexp_exp_list[xlx].jxe_unop_precedence);
           } else {
                sprintf(prestr, "%d", jexp_exp_list[xlx].jxe_bin_precedence);
            }
        }

        switch (xstack[ii].xs_info) {
            case XS_INFO_OPEN_PAREN:
                strcpy(info, "Open paren");
                strcpy(punam, "(");
                break;
 
            case XS_INFO_UNOP:
                strcpy(info, "Unary operator");
                break;
 
            case XS_INFO_BINOP:
                strcpy(info, "Binary operator");
                break;
 
            case XS_INFO_OPERAND:
                strcpy(info, "Operand");
                jexp_quick_tostring(jx, &(xstack[ii].xs_jvv), valstr, sizeof(valstr));
                break;

            default:
                sprintf(info, "Unknown info #%d", xstack[ii].xs_info);
                break;
        }

        ignstr[0] = '\0';
        if (xstack[ii].xs_ignore) sprintf(ignstr, "Ignore=%d", xstack[ii].xs_ignore);

        printf("%2d %-8s %2s %-8s %-10s %s\n", ii, punam, prestr, ignstr, info, valstr);
    }
}
/***************************************************************/
static int jexp_eval_stack(
                    struct jrunexec * jx,
                    struct expession_stack * xstack,
                    int * depth,
                    int * rtnflags,     /* 01/11/2022 */
                    short precedence,
                    short opflags)
{
/*
**
*/
    int jstat;
    int done;
    int xlx;
    int istrue;
    JXE_BINARY_OPERATOR_FUNCTION binop;
    JXE_UNARY_OPERATOR_FUNCTION  unop;

    jstat = 0;
    done  = 0;
    (*rtnflags) = 0;    /* 01/11/2022 */

    while (!jstat && !done) {
#if DEBUG_PRINT_EXP_STACK
        jexp_print_stack(jx, "Stack pre-eval", xstack, (*depth), precedence, opflags);
#endif
        if (*depth <= 1) {
            done = 1;
        } else if (xstack[*depth - 1].xs_info == XS_INFO_OPERAND) {
            /* do unary operations if higher precedence */
            xlx = xstack[*depth - 2].xs_xlx;
            while (!jstat && xlx >= 0 &&
                    xstack[*depth - 2].xs_info == XS_INFO_UNOP &&
                    jexp_exp_list[xlx].jxe_unop_precedence > precedence) {
                (*depth)--;
                unop = jexp_exp_list[xlx].jxe_unop;
                if (unop) jstat =
                    unop(jx, &(xstack[*depth - 1].xs_jvv), &(xstack[*depth].xs_jvv));
                xstack[*depth - 1].xs_info = XS_INFO_OPERAND;
                if (*depth > 1) xlx = xstack[*depth - 2].xs_xlx;
                else xlx = -1;
            }

            if ((*depth) > 2 && xstack[*depth - 2].xs_info == XS_INFO_BINOP) {
                xlx = xstack[*depth - 2].xs_xlx;
                if (xlx < 0) {
                    jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_INTERNAL_ERROR,
                        "Invalid operator on binary op. Found: %d", xlx);
                } else if (jexp_exp_list[xlx].jxe_bin_precedence > precedence ||
                            (jexp_exp_list[xlx].jxe_bin_precedence == precedence &&
            (jexp_exp_list[xlx].jxe_opflags & JXE_OPFLAG_RTOL_ASSOCIATIVITY) == 0)) {
                    binop = jexp_exp_list[xlx].jxe_binop;
                    if (binop)
                        jstat = binop(jx, &(xstack[*depth - 3].xs_jvv),
                            &(xstack[*depth - 1].xs_jvv));
                    jvar_free_jvarvalue_data(&(xstack[*depth - 1].xs_jvv)); /* 02/22/2022 */
                    /* jvar_free_jvarvalue_data(&(xstack[*depth - 2].xs_jvv));   */
                    (*depth) -= 2;
                    xstack[*depth - 1].xs_ignore = 0;   /* 01/10/2022 */
        /* if (!jstat) jexp_print_stack(jx, "Stack post-op", xstack, (*depth), precedence, opflags); */       
                } else {
                    done = 1;
                }
            } else if ((*depth) > 1 && xstack[*depth - 2].xs_info == XS_INFO_OPEN_PAREN) {
                if (opflags & JXE_OPFLAG_CLOSE_PAREN) {
                    (*depth)--;
                    (*rtnflags) |= 1;       /* 01/11/2022 */
                    COPY_XSREC(&(xstack[*depth - 1]), &(xstack[*depth]));
                    done = 1;
                } else {
                    done = 1;
                }
            } else {
                done = 1;
            }
        } else {
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_INTERNAL_ERROR,
                "Invalid expression info. Found: %d", xstack[*depth].xs_info);
        }
    }

    if (!jstat && (*depth) > 0 && (opflags & JXE_OPFLAGS_LOGICAL) &&
            !xstack[*depth - 1].xs_ignore) {
        jstat = jexp_istrue(jx, &(xstack[*depth - 1].xs_jvv), &istrue);
        if (!jstat) {
            if ((istrue && JXE_OPFLAG_EQ(opflags, JXE_OPFLAG_LOGICAL_OR))   ||
                (!istrue && JXE_OPFLAG_EQ(opflags, JXE_OPFLAG_LOGICAL_AND)) ) {
                xstack[*depth - 1].xs_ignore = precedence;
            }
        }
    }

    return (jstat);
}
/***************************************************************/
static int jexp_eval_function_call(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    struct expession_stack * xstack,
                    int * depth)
{
/*
** 12/14/2020
**
** See: jexp_eval_method_call()
static int jexp_method_call(
                    struct jrunexec * jx,
                    struct jvarvalue * mjvv,
                    const char * func_name,
                    struct jvarvalue * jvvparms,
                    struct jvarvalue * jvv)

*/
    int jstat;
    struct jvarvalue jvvparms;
    struct jvarvalue rtnjvv;
    struct jvarvalue * mjvv;
    int rtnflags;   /* 01/11/2022 */

    /* jexp_print_stack(jx, "At jexp_eval_function_call()", xstack, *depth, JX_DOT_PREC, JXE_OPFLAG_FUNCTION_CALL); */
    jstat = jexp_eval_stack(jx, xstack, depth, &rtnflags, JX_DOT_PREC, JXE_OPFLAG_FUNCTION_CALL);
    if (!jstat) jstat = jrun_next_token(jx, pjtok);
    INIT_JVARVALUE(&jvvparms);
    if (!jstat) {
        if ((*pjtok)->jtok_kw != JSPU_CLOSE_PAREN) {
            jstat = jexp_eval(jx, pjtok, &jvvparms);
        }
    }
    /* jexp_print_stack(jx, "At jexp_eval_function_call()", xstack, *depth, 0, JSPU_OPEN_PAREN); */

    if (!jstat) {
        if ((*pjtok)->jtok_typ == JSW_PU && (*pjtok)->jtok_kw == JSPU_CLOSE_PAREN) {
            INIT_JVARVALUE(&rtnjvv);
            mjvv = &(xstack[*depth - 1].xs_jvv);
            jstat = jexp_method_call(jx, pjtok, mjvv, &jvvparms, &rtnjvv);
#if ! FIX_220311
            jvar_free_jvarvalue_data(mjvv);
#endif
            if (!jstat) {
                if (rtnjvv.jvv_dtype) {
                    xstack[*depth - 1].xs_xlx  = 0;
                    xstack[*depth - 1].xs_info = XS_INFO_OPERAND;
                    xstack[*depth - 1].xs_ignore = 0;
                    COPY_JVARVALUE(&(xstack[*depth - 1].xs_jvv), &rtnjvv);
                } else {
                    *depth -= 1;
                }
                jstat = jrun_next_token(jx, pjtok);
            }
        } else {
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_EXP_CLOSE_PAREN,
                "Expecting close parenthesis. Found: %s", (*pjtok)->jtok_text);
        }
    }

    return (jstat);
}
/***************************************************************/
static int jexp_eval_open_bracket(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    struct expession_stack * xstack,
                    int * depth)
{
/*
** 10/21/2021
**
*/
    int jstat;
    struct jvarvalue jvvparms;
    struct jvarvalue rtnjvv;
    struct jvarvalue * mjvv;
    int rtnflags;   /* 01/11/2022 */

    jstat = jexp_eval_stack(jx, xstack, depth, &rtnflags, JX_DOT_PREC, JXE_OPFLAG_POST_UNARY_OPERATION);
    if (!jstat) jstat = jrun_next_token(jx, pjtok);
    INIT_JVARVALUE(&jvvparms);
    if (!jstat) {
        jstat = jexp_eval(jx, pjtok, &jvvparms);
    }
    /* jexp_print_stack(jx, "At jexp_eval_function_call()", xstack, *depth, 0, JSPU_OPEN_PAREN); */

    if (!jstat) {
        if ((*pjtok)->jtok_typ == JSW_PU && (*pjtok)->jtok_kw == JSPU_CLOSE_BRACKET) {
            INIT_JVARVALUE(&rtnjvv);
            mjvv = &(xstack[*depth - 1].xs_jvv);
            jstat = jexp_eval_subscript(jx, pjtok, mjvv, &jvvparms, &rtnjvv);
            if (!jstat) {
                jvar_free_jvarvalue_data(mjvv);  /* var: jvva_nRefs = ?, const: jvva_nRefs = 2 */
                if (rtnjvv.jvv_dtype) {
                    xstack[*depth - 1].xs_xlx  = 0;
                    xstack[*depth - 1].xs_info = XS_INFO_OPERAND;
                    xstack[*depth - 1].xs_ignore = 0;
                    COPY_JVARVALUE(&(xstack[*depth - 1].xs_jvv), &rtnjvv);
                } else {
                    *depth -= 1;
                }
                jstat = jrun_next_token(jx, pjtok);
            }
        } else {
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_EXP_CLOSE_BRACKET,
                "Expecting close bracket for index. Found: %s", (*pjtok)->jtok_text);
        }
    }

    return (jstat);
}
/***************************************************************/
static void jexp_skip_error_exp(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    struct expession_stack * xstack,
                    int * depth)
{
/*
** 11/03/2021
*/
    int jstat = 0;
    int done;
    int xlx;

    /* jexp_print_stack(jx, "Stack jexp_skip_error_exp", xstack, (*depth), 0, 0); */
#if 0 && IS_DEBUG
    printf("Enter jexp_skip_error_exp() -- function needs fixing.\n");
#endif

    done = 0;

    while (!done) {
        xlx = GET_XLX(*pjtok);
        while (!done && xlx >= 0 &&
               (jexp_exp_list[xlx].jxe_opflags & (JXE_OPFLAG_BINARY_OPERATION | JXE_OPFLAG_UNARY_OPERATION))) {
            jstat = jrun_next_token(jx, pjtok);
            if (jstat) done = 1;
            xlx = GET_XLX(*pjtok);
        }
        if (!done) {
            if ((*pjtok)->jtok_kw == JSPU_OPEN_PAREN) {
                jstat = jrun_next_token(jx, pjtok);
            } else {
                if ((*pjtok)->jtok_kw == JSPU_CLOSE_PAREN) {
                    jstat = jrun_next_token(jx, pjtok);
                } else if ((*pjtok)->jtok_kw == JSPU_SEMICOLON      ||
                           (*pjtok)->jtok_kw == JSPU_OPEN_BRACE     ||
                           (*pjtok)->jtok_kw == JSPU_CLOSE_BRACKET  ||
                           (*pjtok)->jtok_kw == JSPU_CLOSE_BRACE) {
                    done = 1;
                } else {
                    jstat = jexp_eval_primary_inactive(jx, pjtok);
                }
                if (!jstat && !done) {
                    xlx = GET_XLX(*pjtok);
                    if (xlx >= 0 && (jexp_exp_list[xlx].jxe_opflags & JXE_OPFLAG_BINARY_OPERATION)) {
                        jstat = jrun_next_token(jx, pjtok);
                        if (jstat) done = 1;
                    }
                }
            }
        }
    }
}
/***************************************************************/
static int jexp_generic_postfix_unary(
                    struct jrunexec * jx,
                    int (*postfix_unary_op)(struct jrunexec * jx, struct jvarvalue * ajvv1),
                    struct jtoken ** pjtok,
                    struct expession_stack * xstack,
                    int * depth)
{
/*
** 02/23/2022
**
*/
    int jstat = 0;
    int rtnflags;

    /* jexp_print_stack(jx, "At jexp_postfix_unary_operator()", xstack, *depth, 0, 0); */

    jstat = jexp_eval_stack(jx, xstack, depth, &rtnflags, JX_DOT_PREC, JXE_OPFLAG_POST_UNARY_OPERATION);
    if (!jstat) jstat = jrun_next_token(jx, pjtok);
    if (!jstat) jstat = (*postfix_unary_op)(jx, &(xstack[*depth - 1].xs_jvv));

    return (jstat);
}
/***************************************************************/
static int jexp_postfix_unary_operator(
                    struct jrunexec * jx,
                    int oppu,
                    struct jtoken ** pjtok,
                    struct expession_stack * xstack,
                    int * depth)
{
/*
** 02/23/2022
**
**  Postfix unary operators do not go on xstack, they are just done.
*/
    int jstat = 0;

    switch (oppu) {
        case JSPU_PLUS_PLUS:
            jstat = jexp_generic_postfix_unary(jx, jexp_post_increment, pjtok, xstack, depth);
            break;
        case JSPU_OPEN_BRACKET:
            jstat = jexp_eval_open_bracket(jx, pjtok, xstack, depth);
            break;
        default:
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_UNSUPPORTED_OPERATION,
                "Unsupported postfix unary operator: %s", (*pjtok)->jtok_text);
            break;
    }

    return (jstat);
}
/***************************************************************/
int jexp_eval_value(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    struct jvarvalue * jvv)
{
/*
**
*/
    int jstat;
    int state;
    int depth;
    int xlx;
    int rtnflags;   /* 01/11/2022 */
    struct expession_stack xstack[EVAL_STACK_SZ];

    jstat       = 0;
    depth       = 0;
    state       = JEXPST_FIRST_OPERAND;
    xlx         = GET_XLX(*pjtok);

    while (!jstat && state != JEXPST_DONE) {
        switch (state) {
            case JEXPST_FIRST_OPERAND:
                if (xlx >= 0 && (jexp_exp_list[xlx].jxe_opflags & JXE_OPFLAG_UNARY_OPERATION)) {
                    xstack[depth].xs_xlx  = xlx;
                    xstack[depth].xs_info = XS_INFO_UNOP;
                    xstack[depth].xs_ignore = 0;
                    INIT_JVARVALUE(&(xstack[depth].xs_jvv));
                    depth++;
                    jstat = jrun_next_token(jx, pjtok);
                    xlx = GET_XLX(*pjtok);
                } else if ((*pjtok)->jtok_typ == JSW_PU && (*pjtok)->jtok_kw == JSPU_OPEN_PAREN) {
                    /* jexp_print_stack(jx, "At ", xstack, depth, 0, JSPU_OPEN_PAREN); */
                    if (depth > 1 && xstack[depth - 1].xs_info == XS_INFO_OPERAND) {
                        jstat = jexp_eval_function_call(jx, pjtok, xstack, &depth);
                        /* jexp_print_stack(jx, "After ()", xstack, depth, 0, 0); */
                        xlx = GET_XLX(*pjtok);
                        if (depth) state       = JEXPST_BINARY_OPERATOR;
                        else state = JEXPST_DONE;
                    } else {
                        xstack[depth].xs_xlx  = -1;
                        xstack[depth].xs_info = XS_INFO_OPEN_PAREN;
                        xstack[depth].xs_ignore = 0;
                        INIT_JVARVALUE(&(xstack[depth].xs_jvv));
                        depth++;
                        jstat = jrun_next_token(jx, pjtok);
                        xlx = GET_XLX(*pjtok);
                    }
                } else {
                    INIT_JVARVALUE(&(xstack[depth].xs_jvv));
                    if (depth > 0 && (xstack[depth-1].xs_xlx >= 0 && 
                                      jexp_exp_list[xstack[depth-1].xs_xlx].jxe_opflags & JXE_OPFLAG_TOKEN)) {
                        jeval_store_token(jx, pjtok, &(xstack[depth].xs_jvv));
                        jstat = jrun_next_token(jx, pjtok);
                    } else {
                        jstat = jexp_eval_primary(jx, pjtok, &(xstack[depth].xs_jvv));
                    }
                    if (!jstat) {
                        xstack[depth].xs_xlx  = xlx;
                        xstack[depth].xs_info = XS_INFO_OPERAND;
                        xstack[depth].xs_ignore = 0;
                        depth++;
                        /* jexp_print_stack(jx, "JEXPST_FIRST_OPERAND ", xstack, depth, 0, 0); */
                        xlx = GET_XLX(*pjtok);
                        state       = JEXPST_BINARY_OPERATOR;
                    }
                }
                break;

            case JEXPST_BINARY_OPERATOR:
                if (xlx < 0) {
                    if ((*pjtok)->jtok_typ == JSW_PU) {
                        if ((*pjtok)->jtok_kw == JSPU_CLOSE_PAREN) {
                            /* jexp_print_stack(jx, "JSPU_CLOSE_PAREN", xstack, depth, 0, 0); */
                            jstat = jexp_eval_stack(jx, xstack, &depth, &rtnflags, 0, JXE_OPFLAG_CLOSE_PAREN);
                            if (!jstat) {
                                if (depth > 1 || (rtnflags & 1)) {
                                    jstat = jrun_next_token(jx, pjtok);
                                    xlx = GET_XLX(*pjtok);
                                } else {
                                    state = JEXPST_DONE;
                                }
                            }
                        } else if ((*pjtok)->jtok_kw == JSPU_OPEN_PAREN) {
                            jstat = jexp_eval_function_call(jx, pjtok, xstack, &depth);
                            xlx = GET_XLX(*pjtok);
                        } else {
                            state = JEXPST_DONE;
                        }
                    } else {
                        state = JEXPST_DONE;
                    }
                } else if (jexp_exp_list[xlx].jxe_opflags & JXE_OPFLAG_POST_UNARY_OPERATION) {
                    jstat = jexp_postfix_unary_operator(jx, jexp_exp_list[xlx].jxe_pu, pjtok, xstack, &depth);
                    xlx = GET_XLX(*pjtok);
                } else {
                    jstat = jexp_eval_stack(jx, xstack, &depth, &rtnflags,
                        jexp_exp_list[xlx].jxe_bin_precedence, jexp_exp_list[xlx].jxe_opflags);
                    if (!jstat) {
                        if (xstack[depth-1].xs_ignore) {
                            jstat = jrun_next_token(jx, pjtok);
                            xlx = GET_XLX(*pjtok);
                            state = JEXPST_FIRST_IGNORED;
                        } else {
                            xstack[depth].xs_xlx  = xlx;
                            xstack[depth].xs_info = XS_INFO_BINOP;
                            xstack[depth].xs_ignore = xstack[depth-1].xs_ignore;
                            INIT_JVARVALUE(&(xstack[depth].xs_jvv));
                            depth++;
                            jstat = jrun_next_token(jx, pjtok);
                            xlx = GET_XLX(*pjtok);
                            state = JEXPST_FIRST_OPERAND;
                        }
                    }
                }
                break;

            case JEXPST_BINARY_IGNORED:
                if (xlx < 0) {
                    if ((*pjtok)->jtok_typ == JSW_PU && (*pjtok)->jtok_kw == JSPU_CLOSE_PAREN) {
                        //jexp_print_stack(jx, "At ", xstack, depth, 0, JXE_OPFLAG_CLOSE_PAREN);
                        /*  if (ii101_15 == 1 || ii101_15 == 3) {           */
                        /*  At  depth=1 precedence=0 flags=close paren      */
                        /*   0             Ignore=2 Operand    true         */
                        if (depth <= 1) {
                            state = JEXPST_DONE;
                        } else if (xstack[depth - 1].xs_info == XS_INFO_OPEN_PAREN) {
                            depth--;
                            jstat = jrun_next_token(jx, pjtok);
                            xlx = GET_XLX(*pjtok);
                            if (!xstack[depth - 1].xs_ignore) state = JEXPST_BINARY_OPERATOR;
                        } else if (depth > 1 && xstack[depth - 2].xs_info == XS_INFO_OPEN_PAREN) {
                            depth--;
                            xstack[depth - 1].xs_xlx    = xstack[depth].xs_xlx;
                            xstack[depth - 1].xs_info   = xstack[depth].xs_info;
                            xstack[depth - 1].xs_jvv    = xstack[depth].xs_jvv;
                            /* xstack[depth - 1].xs_ignore = xstack[depth].xs_ignore; */
                            /* COPY_XSREC(&(xstack[depth - 1]), &(xstack[depth])); */
                            jstat = jrun_next_token(jx, pjtok);
                            xlx = GET_XLX(*pjtok);
                            if (!xstack[depth - 1].xs_ignore) state = JEXPST_BINARY_OPERATOR;
                        } else if (xstack[depth - 2].xs_info == XS_INFO_BINOP) {
                            /* jexp_print_stack(jx, "XS_INFO_BINOP ", xstack, depth, 0, 0); */
                            /*  **Error  depth=3 precedence=0 flags=0       */
                            /*   2             Ignore=3 Operand    false    */
                            /*   1 ||        2          Binary operator     */
                            /*   0             Ignore=3 Operand    false    */
                            depth -= 2;
                        } else {
#if IS_DEBUG
                            jexp_print_stack(jx, "**Error ", xstack, depth, 0, 0);
#endif
                            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_INTERNAL_ERROR,
                                "Expecting open parenthesis. Found: %d", xstack[depth].xs_info);
                        }
                    } else {
                        state = JEXPST_DONE;
                    }
                } else if (xstack[depth-1].xs_ignore > jexp_exp_list[xlx].jxe_bin_precedence) {
                    //jexp_print_stack(jx, "At JEXPST_BINARY_IGNORED", xstack, depth, 0, 0);
                    xstack[depth].xs_xlx  = xlx;
                    xstack[depth].xs_info = XS_INFO_BINOP;
                    xstack[depth].xs_ignore = 0;
                    INIT_JVARVALUE(&(xstack[depth].xs_jvv));
                    depth++;
                    jstat = jrun_next_token(jx, pjtok);
                    xlx = GET_XLX(*pjtok);
                    state = JEXPST_FIRST_OPERAND;
                } else {
                    jstat = jrun_next_token(jx, pjtok);
                    xlx = GET_XLX(*pjtok);
                    state = JEXPST_FIRST_IGNORED;
                }
                break;

            case JEXPST_FIRST_IGNORED:
                if (xlx >= 0 && jexp_exp_list[xlx].jxe_unop) {
                    jstat = jrun_next_token(jx, pjtok);
                    xlx = GET_XLX(*pjtok);
                } else if ((*pjtok)->jtok_typ == JSW_PU && (*pjtok)->jtok_kw == JSPU_OPEN_PAREN) {
                    xstack[depth].xs_xlx  = -1;
                    xstack[depth].xs_info = XS_INFO_OPEN_PAREN;
                    if (xstack[depth-1].xs_ignore) xstack[depth].xs_ignore = 1;
                    else xstack[depth].xs_ignore = 0;
                    INIT_JVARVALUE(&(xstack[depth].xs_jvv));
                    depth++;
                    jstat = jrun_next_token(jx, pjtok);
                    xlx = GET_XLX(*pjtok);
                } else {
                    jstat = jexp_eval_primary_inactive(jx, pjtok);
                    xlx = GET_XLX(*pjtok);
                    state       = JEXPST_BINARY_IGNORED;
                }
                break;
        }
    }

    if (!jstat && depth > 1) {
        jstat = jexp_eval_stack(jx, xstack, &depth, &rtnflags, 0, 0);
    }

    if (!jstat && depth > 1) {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_INTERNAL_ERROR,
            "Extra items on stack. Found: %d items", depth);
    }

    if (jstat) {
        jexp_skip_error_exp(jx, pjtok, xstack, &depth);
        INIT_JVARVALUE(jvv);
    } else {
        if (depth) {
            COPY_JVARVALUE(jvv, &(xstack[0].xs_jvv));
        } else {
            INIT_JVARVALUE(jvv);
        }
    }

    return (jstat);
}
/***************************************************************/
int jexp_eval_assignment(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    struct jvarvalue * jvv)
{
/*
** 09/28/2021
*/
    int jstat;

    jstat = jexp_eval_value(jx, pjtok, jvv);

    return (jstat);
}
/***************************************************************/
int jexp_eval_list(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    struct jvarvalue * jvv)
{
/*
**
*/
    int jstat;
    struct jvarvalue_vallist * jvvl;
    struct jvarvalue jvv2;

    jvvl = NULL;

    jstat = jexp_eval_assignment(jx, pjtok, jvv);
    while (!jstat &&
           (*pjtok)->jtok_kw == JSPU_COMMA ) {
        if (!jvvl) {
            jvvl = jva_new_jvarvalue_vallist();
            jvar_add_jvarvalue_vallist(jvvl, jvv);
        }
        INIT_JVARVALUE(&jvv2);
        jstat = jrun_next_token(jx, pjtok);
        if (!jstat) {
            if ((*pjtok)->jtok_kw == JSPU_CLOSE_PAREN &&
                !jrun_get_stricter_mode(jx)) {
                /* ok - allows: console.log( "Good",); */
            } else {
                jstat = jexp_eval_assignment(jx, pjtok, &jvv2);
                if (!jstat) {
                    jvar_add_jvarvalue_vallist(jvvl, &jvv2);
                }
            }
        }
    }

    if (!jstat && jvvl) {
        jvv->jvv_dtype = JVV_DTYPE_VALLIST;
        jvv->jvv_val.jvv_jvvl = jvvl;
    }

    return (jstat);
}
/***************************************************************/
static int jexp_eval(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    struct jvarvalue * jvv)
{
/*
** 01/12/2021
**
**      Evaluates expression with commas and assignments
*/
    int jstat;

    jstat = jexp_eval_list(jx, pjtok, jvv);

    return (jstat);
}
/***************************************************************/
int jexp_evaluate(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    struct jvarvalue * jvv)
{
/*
** From: https://developer.mozilla.org/
**       en-US/docs/Web/JavaScript/Reference/Operators/Operator_Precedence
**
** Precedence Operator type                   Associativity   Individual operators
**     21     Grouping                        n/a             ( … )
**     20     Member Access                   left-to-right   … . …
**            Computed Member Access          left-to-right   … [ … ]
**            new (with argument list)        n/a             new … ( … )
**            Function Call                   left-to-right   … ( … )
**            Optional chaining               left-to-right   ?.
**     19     new (without argument list)     right-to-left   new …
**     18     Postfix Increment               n/a             … ++
**            Postfix Decrement                               … --
**     17     Logical NOT                     right-to-left   ! …
**            Bitwise NOT                                     ~ …
**            Unary Plus                                      + …
**            Unary Negation                                  - …
**            Prefix Increment                                ++ …
**            Prefix Decrement                                -- …
**            typeof                                          typeof …
**            void                                            void …
**            delete                                          delete …
**            await                                           await …
**     16     Exponentiation                  right-to-left   … ** …
**     15     Multiplication                  left-to-right   … * …
**            Division                                        … / …
**            Remainder                                       … % …
**     14     Addition                        left-to-right   … + …
**            Subtraction                                     … - …
**     13     Bitwise Left Shift              left-to-right   … << …
**            Bitwise Right Shift                             … >> …
**            Bitwise Unsigned Right Shift                    … >>> …
**     12     Less Than                       left-to-right   … < …
**            Less Than Or Equal                              … <= …
**            Greater Than                                    … > …
**            Greater Than Or Equal                           … >= …
**            in                                              … in …
**            instanceof                                      … instanceof …
**     11     Equality                        left-to-right   … == …
**            Inequality                                      … != …
**            Strict Equality                                 … === …
**            Strict Inequality                               … !== …
**     10     Bitwise AND                     left-to-right   … & …
**     9      Bitwise XOR                     left-to-right   … ^ …
**     8      Bitwise OR                      left-to-right   … | …
**     7      Logical AND                     left-to-right   … && …
**     6      Logical OR                      left-to-right   … || …
**     5      Nullish coalescing operator     left-to-right   … ?? …
**     4      Conditional                     right-to-left   … ? … : …
**     3      Assignment                      right-to-left   … = …
**                                                            … += …
**                                                            … -= …
**                                                            … **= …
**                                                            … *= …
**                                                            … /= …
**                                                            … %= …
**                                                            … <<= …
**                                                            … >>= …
**                                                            … >>>= …
**                                                            … &= …
**                                                            … ^= …
**                                                            … |= …
**                                                            … &&= …
**                                                            … ||= …
**                                                            … ??= …
**     2      yield                           right-to-left   yield …
**            yield*                                          yield* …
**     1      Comma / Sequence                left-to-right   … , …
*/
    int jstat = 0;
#if DEBUG_PRINT_EVAL_RESULT
    char outstr[128];
#endif

    INIT_JVARVALUE(jvv);
    jstat = jexp_eval(jx, pjtok, jvv);

#if DEBUG_PRINT_EVAL_RESULT
    if (!jstat) {
        if (strcmp((*pjtok)->jtok_text, ";")) {
            printf("jexp_evaluate end token=<%s>\n", (*pjtok)->jtok_text);
        }
        jexp_quick_tostring(jx, jvv, outstr, sizeof(outstr));
        /* jstat = jrun_int_tostring(jx, jvv, &outstr); */
        if (!jstat) {
            printf("jexp_evaluate=<%s>\n", outstr);
            /* Free(outstr); */
        }
    }
#endif

    return (jstat);
}
/***************************************************************/
void jexp_init_op_xref(struct jrunexec * jx)
{
/*
**
*/
    int ii;

    jx->jx_nop_xref = JSPU_ZZ_LAST;
    jx->jx_aop_xref = New(short, jx->jx_nop_xref);
    memset(jx->jx_aop_xref, -1, sizeof(short) * jx->jx_nop_xref);

    for (ii = 0; jexp_exp_list[ii].jxe_pu; ii++) {
        jx->jx_aop_xref[jexp_exp_list[ii].jxe_pu] = ii;
    }
}
/***************************************************************/
int jexp_evaluate_inactive(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok)
{
/*
** 01/19/2021
*/
    int jstat;

#if PREP_INACTIVE_EXPRESSIONS
    jstat = jprep_expression(jx, pjtok, 0);
#else
    jstat = jexp_eval_list_inactive(jx, pjtok);
#endif

    return (jstat);
}
/***************************************************************/
