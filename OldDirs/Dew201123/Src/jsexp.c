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
#include "jstok.h"
#include "jsexp.h"
#include "dbtreeh.h"
#include "snprfh.h"
#include "utl.h"
#include "var.h"
#include "internal.h"

/***************************************************************/
/* forard */
static int jexp_eval(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    struct jvarvalue * jvv);

/***************************************************************/
int convert_string_to_number(const char * numbuf, int * intans, double * dblans)
{
/*
**  Returns:
**      -1  : Number contains invalid characters
**      -2  : Number has no digits
**       1  : Number is an integer in intans
**       2  : Number is a double in dblans
*/
    int cstat;
    int neg;
    int ix;
    int ndigits;
    int ival;
    int exp;
    int eneg;
    double nval;
    double dec;

    cstat = 0;
    (*intans) = 0;
    (*dblans) = 0.0;
    ival = 0;
    neg = 0;
    ix = 0;
    ndigits = 0;

    if (numbuf[ix] == '-') {
        neg = 1;
        ix++;
    } else if (numbuf[ix] == '+') {
        ix++;
    }

    while (ndigits < 9 && numbuf[ix] >= '0' && numbuf[ix] <= '9') {
        ndigits++;
        ival = (ival * 10) + (numbuf[ix] - '0');
        ix++;
    }

    if (!numbuf[ix]) {
        cstat = 1; /* int */
    } else {
        cstat = 2; /* double */
        nval = (double)ival;
        while (numbuf[ix] >= '0' && numbuf[ix] <= '9') {
            ndigits++;
            nval = (nval * 10.0) + (numbuf[ix] - '0');
            ix++;
        }
    }

    if (numbuf[ix] == '.') {
        ix++;
        dec = 1.0;
        while (numbuf[ix] >= '0' && numbuf[ix] <= '9') {
            ndigits++;
            dec /= 10.0;
            nval += dec * (double)(numbuf[ix] - '0');
            ix++;
        }
    }

    if (numbuf[ix] == 'e' || numbuf[ix] == 'E') {
        ix++;
        exp = 0;
        eneg = 0;
        if (numbuf[ix] == '-') {
            eneg = 1;
            ix++;
        } else if (numbuf[ix] == '+') {
            ix++;
        }

        while (ndigits < 9 && numbuf[ix] >= '0' && numbuf[ix] <= '9') {
            exp = (exp * 10) + (numbuf[ix] - '0');
            ix++;
        }

        if (!numbuf[ix]) {
            if (eneg) {
                while (exp) {
                    nval /= 10.0;
                    exp--;
                }
            } else {
                while (exp) {
                    nval *= 10.0;
                    exp--;
                }
            }
        }
    }

    if (numbuf[ix]) {
        cstat = -1;
    } else if (!ndigits) {
        cstat = -2;
    } else if (cstat == 1) {
        if (neg) ival = -ival;
        (*intans) = ival;
    } else if (cstat == 2) {
        if (neg) nval = -nval;
        (*dblans) = nval;
    }
    
    return (cstat);
}
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
            Realloc(jvvtgt->jvv_val.jvv_val_chars.jvvc_val_chars, char, jvvtgt->jvv_val.jvv_val_chars.jvvc_size);
    }

    memcpy(jvvtgt->jvv_val.jvv_val_chars.jvvc_val_chars, jchars, jcharslen);
    jvvtgt->jvv_val.jvv_val_chars.jvvc_val_chars[jcharslen] = '\0';
    jvvtgt->jvv_val.jvv_val_chars.jvvc_length = jcharslen;
}
/***************************************************************/
static void jvar_store_jvarvalue(
    struct jvarvalue * jvvtgt,
    struct jvarvalue * jvvsrc)
{
/*
**
*/
    switch (jvvsrc->jvv_dtype) {
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
        default:
            break;
    }
}
/***************************************************************/
static void jvar_store_lval(
    struct jrunexec * jx,
    struct jvarvalue * jvvtgt,
    struct jvarvalue * jvvsrc)
{
/*
**
*/
    jvvtgt->jvv_dtype = JVV_DTYPE_LVAL;
    jvvtgt->jvv_val.jvv_lval = jvvsrc;
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
            Realloc(jvvtgt->jvv_val.jvv_val_chars.jvvc_val_chars, char, jvvtgt->jvv_val.jvv_val_chars.jvvc_size);
    }

    memcpy(jvvtgt->jvv_val.jvv_val_chars.jvvc_val_chars,
           jvv_chars1->jvvc_val_chars, jvv_chars1->jvvc_length);
    memcpy(jvvtgt->jvv_val.jvv_val_chars.jvvc_val_chars + jvv_chars1->jvvc_length,
           jvv_chars2->jvvc_val_chars, jvv_chars2->jvvc_length);
    jvvtgt->jvv_val.jvv_val_chars.jvvc_val_chars[tlen] = '\0';
    jvvtgt->jvv_val.jvv_val_chars.jvvc_length = tlen;
}
/***************************************************************/
static void jvarvalue_to_chars(
    struct jvarvalue * jvvtgt,
    struct jvarvalue * jvvsrc)
{
/*
** See also: jrun_int_tostring()
*/
    char jchars[32];

    jvar_free_jvarvalue_data(jvvtgt);
    switch (jvvsrc->jvv_dtype) {
        case JVV_DTYPE_JSINT   :
            Snprintf(jchars, sizeof(jchars), "%d", jvvsrc->jvv_val.jvv_val_jsint);
            jvar_store_chars(jvvtgt, jchars);
            break;
        case JVV_DTYPE_JSFLOAT   :
            Snprintf(jchars, sizeof(jchars), "%g",
                (double)jvvsrc->jvv_val.jvv_val_jsfloat);
            jvar_store_chars(jvvtgt, jchars);
            break;
        case JVV_DTYPE_CHARS   :
            break;
        default:
            break;
    }
}
/***************************************************************/
static int jexp_unop_neg(struct jrunexec * jx,
         struct jvarvalue         * jvvans,
         struct jvarvalue         * jvv1)
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

    jvv1 = GET_RVAL(jvv1);

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
            jstat = jrun_set_error(jx, JSERR_OPERATOR_TYPE_MISMATCH,
                "Illegal negation");
            break;
    }

    return (jstat);
}
/***************************************************************/
static int jexp_unop_lognot(struct jrunexec * jx,
         struct jvarvalue         * jvvans,
         struct jvarvalue         * jvv1)
{
/*
** Logical NOT
*/
    int jstat = 0;

    jvv1 = GET_RVAL(jvv1);

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
            jstat = jrun_set_error(jx, JSERR_OPERATOR_TYPE_MISMATCH,
                "Illegal positive");
            break;
    }

    return (jstat);
}
/***************************************************************/
static int jexp_unop_pos(struct jrunexec * jx,
         struct jvarvalue         * jvvans,
         struct jvarvalue         * jvv1)
{
/*
**
*/
    int jstat = 0;

    jvv1 = GET_RVAL(jvv1);

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
            jstat = jrun_set_error(jx, JSERR_OPERATOR_TYPE_MISMATCH,
                "Illegal positive");
            break;
    }

    return (jstat);
}
/***************************************************************/
static int jexp_istrue(struct jrunexec * jx,
         struct jvarvalue         * jvv1,
         int * istrue)
{
/*
** Logical NOT
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

        default:
            jstat = jrun_set_error(jx, JSERR_OPERATOR_TYPE_MISMATCH,
                "Illegal boolean");
            break;
    }

    return (jstat);
}
/***************************************************************/
static int jexp_binop_logor(struct jrunexec * jx,
    struct jvarvalue * jvv1,
    struct jvarvalue * jvv2)
{
/*
**
*/
    int jstat = 0;
    int istrue;

    jvv1 = GET_RVAL(jvv1);
    jvv2 = GET_RVAL(jvv2);

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
    struct jvarvalue * jvv1,
    struct jvarvalue * jvv2)
{
/*
**
*/
    int jstat = 0;
    int istrue;

    jvv1 = GET_RVAL(jvv1);
    jvv2 = GET_RVAL(jvv2);

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
    struct jvarvalue * jvv1,
    struct jvarvalue * jvv2)
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

    jvv1 = GET_RVAL(jvv1);
    jvv2 = GET_RVAL(jvv2);

    switch (jvv1->jvv_dtype) {
        case JVV_DTYPE_JSINT   :
            switch (jvv2->jvv_dtype) {
                case JVV_DTYPE_JSINT   :
                    jvar_store_jsint(jvv1,
                        jvv1->jvv_val.jvv_val_jsint +
                        jvv2->jvv_val.jvv_val_jsint);
                    break;
                case JVV_DTYPE_JSFLOAT   :
                    jvar_store_jsfloat(jvv1,
                        (JSFLOAT)jvv1->jvv_val.jvv_val_jsint +
                        jvv2->jvv_val.jvv_val_jsfloat);
                    break;
                case JVV_DTYPE_CHARS   :
                    jvarvalue_to_chars(&jvvchars, jvv1);
                    jvar_cat_chars(jvv1,
                        &(jvvchars.jvv_val.jvv_val_chars),
                        &(jvv2->jvv_val.jvv_val_chars));
                    jvar_free_jvarvalue_data(&jvvchars);
                    break;
                default:
                    jstat = jrun_set_error(jx, JSERR_OPERATOR_TYPE_MISMATCH,
                        "Illegal integer addition");
                    break;
            }
            break;

        case JVV_DTYPE_JSFLOAT:
            switch (jvv2->jvv_dtype) {
                case JVV_DTYPE_JSINT   :
                    jvar_store_jsfloat(jvv2,
                        jvv1->jvv_val.jvv_val_jsfloat +
                        (JSFLOAT)jvv2->jvv_val.jvv_val_jsint);
                    break;
                case JVV_DTYPE_JSFLOAT   :
                    jvar_store_jsfloat(jvv2,
                        jvv1->jvv_val.jvv_val_jsfloat +
                        jvv2->jvv_val.jvv_val_jsfloat);
                    break;
                case JVV_DTYPE_CHARS   :
                    jvarvalue_to_chars(&jvvchars, jvv1);
                    jvar_cat_chars(jvv2,
                        &(jvvchars.jvv_val.jvv_val_chars),
                        &(jvv2->jvv_val.jvv_val_chars));
                    jvar_free_jvarvalue_data(&jvvchars);
                    break;
                default:
                    jstat = jrun_set_error(jx, JSERR_OPERATOR_TYPE_MISMATCH,
                        "Illegal floating addition");
                    break;
            }
            break;

        case JVV_DTYPE_CHARS :
            jvarvalue_to_chars(&jvvchars, jvv2);
            jvar_cat_chars(jvv2,
                &(jvv1->jvv_val.jvv_val_chars),
                &(jvvchars.jvv_val.jvv_val_chars));
            jvar_free_jvarvalue_data(&jvvchars);
            break;

        default:
            jstat = jrun_set_error(jx, JSERR_OPERATOR_TYPE_MISMATCH,
                "Illegal addition");
            break;
    }

    return (jstat);
}
/***************************************************************/
static int jexp_binop_sub(struct jrunexec * jx,
    struct jvarvalue * jvv1,
    struct jvarvalue * jvv2)
{
/*
**
*/
    int jstat = 0;

    jvv1 = GET_RVAL(jvv1);
    jvv2 = GET_RVAL(jvv2);

    switch (jvv1->jvv_dtype) {
        case JVV_DTYPE_JSINT   :
            switch (jvv2->jvv_dtype) {
                case JVV_DTYPE_JSINT   :
                    jvar_store_jsint(jvv1,
                        jvv1->jvv_val.jvv_val_jsint -
                        jvv2->jvv_val.jvv_val_jsint);
                    break;
                case JVV_DTYPE_JSFLOAT   :
                    jvar_store_jsfloat(jvv1,
                        (JSFLOAT)jvv1->jvv_val.jvv_val_jsint -
                        jvv2->jvv_val.jvv_val_jsfloat);
                    break;
                default:
                    jstat = jrun_set_error(jx, JSERR_OPERATOR_TYPE_MISMATCH,
                        "Illegal integer subtraction");
                    break;
            }
            break;

        case JVV_DTYPE_JSFLOAT:
            switch (jvv2->jvv_dtype) {
                case JVV_DTYPE_JSINT   :
                    jvar_store_jsfloat(jvv2,
                        jvv1->jvv_val.jvv_val_jsfloat -
                        (JSFLOAT)jvv2->jvv_val.jvv_val_jsint);
                    break;
                case JVV_DTYPE_JSFLOAT   :
                    jvar_store_jsfloat(jvv2,
                        jvv1->jvv_val.jvv_val_jsfloat -
                        jvv2->jvv_val.jvv_val_jsfloat);
                    break;
                default:
                    jstat = jrun_set_error(jx, JSERR_OPERATOR_TYPE_MISMATCH,
                        "Illegal floating subtraction");
                    break;
            }
            break;

        default:
            jstat = jrun_set_error(jx, JSERR_OPERATOR_TYPE_MISMATCH,
                "Illegal subtraction");
            break;
    }

    return (jstat);
}
/***************************************************************/
static int jexp_binop_mul(struct jrunexec * jx,
    struct jvarvalue * jvv1,
    struct jvarvalue * jvv2)
{
/*
**
*/
    int jstat = 0;

    jvv1 = GET_RVAL(jvv1);
    jvv2 = GET_RVAL(jvv2);

    switch (jvv1->jvv_dtype) {
        case JVV_DTYPE_JSINT   :
            switch (jvv2->jvv_dtype) {
                case JVV_DTYPE_JSINT   :
                    jvar_store_jsint(jvv1,
                        jvv1->jvv_val.jvv_val_jsint *
                        jvv2->jvv_val.jvv_val_jsint);
                    break;
                case JVV_DTYPE_JSFLOAT   :
                    jvar_store_jsfloat(jvv1,
                        (JSFLOAT)jvv1->jvv_val.jvv_val_jsint *
                        jvv2->jvv_val.jvv_val_jsfloat);
                    break;
                default:
                    jstat = jrun_set_error(jx, JSERR_OPERATOR_TYPE_MISMATCH,
                        "Illegal integer multiplication");
                    break;
            }
            break;

        case JVV_DTYPE_JSFLOAT:
            switch (jvv2->jvv_dtype) {
                case JVV_DTYPE_JSINT   :
                    jvar_store_jsfloat(jvv2,
                        jvv1->jvv_val.jvv_val_jsfloat *
                        (JSFLOAT)jvv2->jvv_val.jvv_val_jsint);
                    break;
                case JVV_DTYPE_JSFLOAT   :
                    jvar_store_jsfloat(jvv2,
                        jvv1->jvv_val.jvv_val_jsfloat *
                        jvv2->jvv_val.jvv_val_jsfloat);
                    break;
                default:
                    jstat = jrun_set_error(jx, JSERR_OPERATOR_TYPE_MISMATCH,
                        "Illegal floating multiplication");
                    break;
            }
            break;

        default:
            jstat = jrun_set_error(jx, JSERR_OPERATOR_TYPE_MISMATCH,
                "Illegal multiplication");
            break;
    }

    return (jstat);
}
/***************************************************************/
static int jexp_binop_div(struct jrunexec * jx,
    struct jvarvalue * jvv1,
    struct jvarvalue * jvv2)
{
/*
**
*/
    int jstat = 0;

    jvv1 = GET_RVAL(jvv1);
    jvv2 = GET_RVAL(jvv2);

    switch (jvv1->jvv_dtype) {
        case JVV_DTYPE_JSINT   :
            switch (jvv2->jvv_dtype) {
                case JVV_DTYPE_JSINT   :
                    jvar_store_jsint(jvv1,
                        jvv1->jvv_val.jvv_val_jsint /
                        jvv2->jvv_val.jvv_val_jsint);
                    break;
                case JVV_DTYPE_JSFLOAT   :
                    jvar_store_jsfloat(jvv1,
                        (JSFLOAT)jvv1->jvv_val.jvv_val_jsint /
                        jvv2->jvv_val.jvv_val_jsfloat);
                    break;
                default:
                    jstat = jrun_set_error(jx, JSERR_OPERATOR_TYPE_MISMATCH,
                        "Illegal integer division");
                    break;
            }
            break;

        case JVV_DTYPE_JSFLOAT:
            switch (jvv2->jvv_dtype) {
                case JVV_DTYPE_JSINT   :
                    jvar_store_jsfloat(jvv2,
                        jvv1->jvv_val.jvv_val_jsfloat /
                        (JSFLOAT)jvv2->jvv_val.jvv_val_jsint);
                    break;
                case JVV_DTYPE_JSFLOAT   :
                    jvar_store_jsfloat(jvv2,
                        jvv1->jvv_val.jvv_val_jsfloat /
                        jvv2->jvv_val.jvv_val_jsfloat);
                    break;
                default:
                    jstat = jrun_set_error(jx, JSERR_OPERATOR_TYPE_MISMATCH,
                        "Illegal floating division");
                    break;
            }
            break;

        default:
            jstat = jrun_set_error(jx, JSERR_OPERATOR_TYPE_MISMATCH,
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
    struct jvarvalue * jvv1,
    struct jvarvalue * jvv2)
{
/*
** hat = exponentiation
*/
    int jstat = 0;
    int merr;
    int nint;
    JSFLOAT nflt;

    jvv1 = GET_RVAL(jvv1);
    jvv2 = GET_RVAL(jvv2);

    switch (jvv1->jvv_dtype) {
        case JVV_DTYPE_JSINT   :
            switch (jvv2->jvv_dtype) {
                case JVV_DTYPE_JSINT   :
                    merr = pow_int(jvv1->jvv_val.jvv_val_jsint,
                        jvv2->jvv_val.jvv_val_jsint, &nint);
                    jvar_store_jsint(jvv1, nint);
                    if (merr) {
                        jstat = jrun_set_error(jx, JSERR_ILLEGAL_EXPONENTIATION,
                            "Error on integer exponentiation");
                    }
                    break;
                case JVV_DTYPE_JSFLOAT   :
                    merr = pow_flt((JSFLOAT)jvv1->jvv_val.jvv_val_jsint,
                        jvv2->jvv_val.jvv_val_jsfloat, &nflt);
                    jvar_store_jsfloat(jvv1, nflt);
                    if (merr) {
                        jstat = jrun_set_error(jx, JSERR_ILLEGAL_EXPONENTIATION,
                            "Error on integer exponentiation");
                    }
                    break;
                default:
                    jstat = jrun_set_error(jx, JSERR_OPERATOR_TYPE_MISMATCH,
                        "Illegal integer exponentiation");
                    break;
            }
            break;

        case JVV_DTYPE_JSFLOAT:
            switch (jvv2->jvv_dtype) {
                case JVV_DTYPE_JSINT   :
                    merr = pow_flt_int(jvv1->jvv_val.jvv_val_jsfloat,
                        jvv2->jvv_val.jvv_val_jsint, &nflt);
                    jvar_store_jsfloat(jvv1, nflt);
                    if (merr) {
                        jstat = jrun_set_error(jx, JSERR_ILLEGAL_EXPONENTIATION,
                            "Error on floating exponentiation");
                    }
                    break;
                case JVV_DTYPE_JSFLOAT   :
                    merr = pow_flt(jvv1->jvv_val.jvv_val_jsfloat,
                        jvv2->jvv_val.jvv_val_jsfloat, &nflt);
                    jvar_store_jsfloat(jvv1, nflt);
                    if (merr) {
                        jstat = jrun_set_error(jx, JSERR_ILLEGAL_EXPONENTIATION,
                            "Error on floating exponentiation");
                    }
                    break;
                default:
                    jstat = jrun_set_error(jx, JSERR_OPERATOR_TYPE_MISMATCH,
                        "Illegal floating exponentiation");
                    break;
            }
            break;

        default:
            jstat = jrun_set_error(jx, JSERR_OPERATOR_TYPE_MISMATCH,
                "Illegal exponentiation");
            break;
    }

    return (jstat);
}
/***************************************************************/
static int jexp_eval_function_call(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    struct jvarvalue * jvv)
{
/*
**
*/
    int jstat = 0;

    //jstat = jexp_eval_primary(jx, pjtok, jvv);

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
    struct jtoken next_jtok;

    INIT_JVARVALUE(jvv);
    switch ((*pjtok)->jtok_typ) {
        case JSW_STRING:
            /* jtok_text has beginning and ending quatation marks */
            jvar_store_chars_len(jvv, (*pjtok)->jtok_text + 1, (*pjtok)->jtok_tlen - 2);
            break;
        case JSW_NUMBER:
            cstat = jvar_chars_to_number(jvv, (*pjtok)->jtok_text);
            if (cstat) {
                jstat = jrun_set_error(jx, JSERR_INVALID_NUMBER,
                    "Invalid number: %s", (*pjtok)->jtok_text);
            } else {
                jstat = jrun_next_token(jx, pjtok);
            }
            break;
        case JSW_PU:
            if ((*pjtok)->jtok_kw == JSPU_OPEN_PAREN) {
                jstat = jrun_next_token(jx, pjtok);
                if (!jstat) {
                    jstat = jexp_eval(jx, pjtok, jvv);
                }
                if (!jstat) {
                    if ((*pjtok)->jtok_kw == JSPU_CLOSE_PAREN) {
                        jstat = jrun_next_token(jx, pjtok);
                    } else {
                        jstat = jrun_set_error(jx, JSERR_EXP_CLOSE_PAREN,
                            "Expecting close parenthesis. Found: %s", (*pjtok)->jtok_text);
                    }
                } else if (jstat == JERR_EOF) {
                    jstat = jrun_set_error(jx, JSERR_EXP_CLOSE_PAREN,
                        "Expecting close parenthesis. Found end.");
                }
            } else {
                jstat = jrun_set_error(jx, JSERR_UNEXPECTED_SPECIAL,
                    "Unexpected special: %s", (*pjtok)->jtok_text);
            }
            break;
        case JSW_KW:
            switch ((*pjtok)->jtok_kw) {
                case JSKW_FALSE:
                    jvar_store_bool(jvv, 0);
                    break;
                case JSKW_TRUE:
                    jvar_store_bool(jvv, 1);
                    break;
                case JSKW_NULL:
                    jvar_store_null(jvv);
                    break;
                default:
                    jstat = jrun_set_error(jx, JSERR_UNEXPECTED_KEYWORD,
                        "Unexpected keyword: %s", (*pjtok)->jtok_text);
                break;
            }
            if (!jstat) jstat = jrun_next_token(jx, pjtok);
            break;
        case JSW_ID:
            jrun_peek_token(jx, &next_jtok);
            if (next_jtok.jtok_kw == JSPU_OPEN_PAREN) {
                jstat = jexp_eval_function_call(jx, pjtok, jvv);
            } else {
                vjvv = jvar_find_variable(jx, (*pjtok)->jtok_text);
                if (vjvv) {
                    jvar_store_lval(jx, jvv, vjvv);
                } else {
                    jstat = jrun_set_error(jx, JSERR_UNDEFINED_VARIABLE,
                        "Undefined variable: %s", (*pjtok)->jtok_text);
                }
            }
            break;
        default:
            jstat = jrun_set_error(jx, JSERR_INVALID_TOKEN_TYPE,
                "Invalid token type %d: %s", (*pjtok)->jtok_typ, (*pjtok)->jtok_text);
            break;
    }

    return (jstat);
}
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
static void jvar_shallow_copy_jvarvalue(
    struct jvarvalue * jvvtgt,
    struct jvarvalue * jvvsrc)
{
/*
**
*/
    memcpy(jvvtgt, jvvsrc, sizeof(struct jvarvalue));
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
    jvar_shallow_copy_jvarvalue(&(jvvl->jvvl_avals[jvvl->jvvl_nvals]), jvv);
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
static int jexp_method_call(
                    struct jrunexec * jx,
                    struct jvarvalue * mjvv,
                    const char * func_name,
                    struct jvarvalue * jvvparms,
                    struct jvarvalue * jvv)
{
/*
**
*/
    int jstat = 0;
    int free_parms;
    struct jvarvalue_vallist * jvvlargs;
    struct jvarvalue_vallist jvvl;
    struct jvarvalue jvvarray[1];

    if (jvvparms->jvv_dtype == JVV_DTYPE_VALLIST) {
        free_parms = 1;
        jvvlargs = jvvparms->jvv_val.jvv_jvvl;
    } else {
        free_parms = 0;
        jvar_shallow_copy_jvarvalue(&(jvvarray[0]), jvvparms);
        jvvl.jvvl_avals = jvvarray;
        jvvl.jvvl_nvals = 1;
        jvvlargs        = &jvvl;
    }
    INIT_JVARVALUE(jvv);

    if (mjvv->jvv_dtype == JVV_DTYPE_INTERNAL_METHOD) {
        jstat = (mjvv->jvv_val.jvv_int_method)(jx, func_name, jvvlargs, jvv);
    } else {
        jstat = jrun_set_error(jx, JSERR_EXP_METHOD,
            "Expecting method name. Found: %s", func_name);
    }

    if (free_parms) {
        jvvl_free_jvarvalue_vallist(jvvlargs);
    } else {
        jvar_free_jvarvalue_data(&(jvvarray[0]));
    }

    return (jstat);
}
/***************************************************************/
static int jexp_eval_method_call(
                    struct jrunexec * jx,
                    struct jvarvalue * mjvv,
                    const char * func_name,
                    struct jtoken ** pjtok,
                    struct jvarvalue * jvv)
{
/*
**
*/
    int jstat = 0;
    struct jtoken next_jtok;
    struct jvarvalue jvvparms;

    INIT_JVARVALUE(&jvvparms);

    jstat = jrun_next_token(jx, pjtok);  /* '(' - cannot fail */
    jrun_peek_token(jx, &next_jtok);
    if (next_jtok.jtok_kw != JSPU_CLOSE_PAREN) {
        jstat = jrun_next_token(jx, pjtok);
        if (!jstat) {
            jstat = jexp_eval(jx, pjtok, &jvvparms);
        }
        if (!jstat) {
            if ((*pjtok)->jtok_kw == JSPU_CLOSE_PAREN) {
                jstat = jexp_method_call(jx, mjvv, func_name, &jvvparms, jvv);
                if (!jstat) {
                    jstat = jrun_next_token(jx, pjtok);
                }
            } else {
                jstat = jrun_set_error(jx, JSERR_EXP_CLOSE_PAREN,
                    "Expecting close parenthesis in method call. Found: %s",
                    (*pjtok)->jtok_text);
            }
        } else if (jstat == JERR_EOF) {
            jstat = jrun_set_error(jx, JSERR_EXP_CLOSE_PAREN,
                "Expecting close parenthesis in method call. Found end.");
        }
    }

    return (jstat);
}
/***************************************************************/
static int jexp_binop_dot(
                    struct jrunexec * jx,
                    struct jvarvalue * cjvv,
                    struct jtoken ** pjtok,
                    struct jvarvalue * jvv)
{
/*
**
*/
    int jstat = 0;
    struct jtoken next_jtok;
    struct jvarvalue * mjvv;
    int need_free;
    char * func_name;
    char func_name_buf[FUNC_NAME_MAX];

    cjvv = GET_RVAL(cjvv);

    if (cjvv->jvv_dtype != JVV_DTYPE_OBJECT) {
        jstat = jrun_set_error(jx, JSERR_EXP_OBJECT_AFTER_DOT,
            "Expecting object after dot. Found: %s", (*pjtok)->jtok_text);
    } else {
        mjvv = jvar_object_member(jx, cjvv, (*pjtok)->jtok_text);
        if (!mjvv) {
            jstat = jrun_set_error(jx, JSERR_UNDEFINED_CLASS_MEMBER,
                "Undefined class member: %s", (*pjtok)->jtok_text);
        } else {
            jrun_peek_token(jx, &next_jtok);
            if (next_jtok.jtok_kw == JSPU_OPEN_PAREN) {
                func_name = quick_strcpy(
                    func_name_buf, FUNC_NAME_MAX, (*pjtok)->jtok_text, &need_free);
                jstat = jexp_eval_method_call(jx, mjvv, func_name, pjtok, jvv);
                if (!jstat) {
                    jstat = jrun_next_token(jx, pjtok);
                }
                if (need_free) Free(func_name);
            }
        }
    }

    return (jstat);
}
/***************************************************************/
static int jexp_eval_dot(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    struct jvarvalue * jvv)
{
/*
**
*/
    int jstat = 0;
    struct jvarvalue jvv2;

    jstat = jexp_eval_primary(jx, pjtok, jvv);
    while (!jstat &&
        ((*pjtok)->jtok_kw == JSPU_DOT)) {
        INIT_JVARVALUE(&jvv2);
        jstat = jrun_next_token(jx, pjtok);
        if (!jstat) {
            if ((*pjtok)->jtok_typ != JSW_ID) {
                jstat = jrun_set_error(jx, JSERR_EXP_ID_AFTER_DOT,
                    "Expecting identifier after dot. Found: %s", (*pjtok)->jtok_text);
            } else {
                jstat = jexp_binop_dot(jx, jvv, pjtok, &jvv2);
                jvar_shallow_copy_jvarvalue(jvv, &jvv2);
            }
        }
    }

    return (jstat);
}
/***************************************************************/
static int jexp_eval_mul_div(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    struct jvarvalue * jvv)
{
/*
**
*/
    int jstat;
    short opkw;
    struct jvarvalue jvv2;

    jstat = jexp_eval_dot(jx, pjtok, jvv);
    while (!jstat &&
        ((*pjtok)->jtok_kw == JSPU_STAR ||
         (*pjtok)->jtok_kw == JSPU_SLASH) ) {
        INIT_JVARVALUE(&jvv2);
        opkw = (*pjtok)->jtok_kw;
        jstat = jrun_next_token(jx, pjtok);
        if (!jstat) {
            jstat = jexp_eval_dot(jx, pjtok, &jvv2);
        }
        if (!jstat) {
            if (opkw == JSPU_SLASH) {
                jstat = jexp_binop_div(jx, jvv, &jvv2);
            } else {
                jstat = jexp_binop_mul(jx, jvv, &jvv2);
            }
        }
    }

    return (jstat);
}
/***************************************************************/
static int jexp_eval_add_sub(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    struct jvarvalue * jvv)
{
/*
**
*/
    int jstat;
    short opkw;
    struct jvarvalue jvv2;

    jstat = jexp_eval_mul_div(jx, pjtok, jvv);
    while (!jstat &&
        ((*pjtok)->jtok_kw == JSPU_PLUS ||
         (*pjtok)->jtok_kw == JSPU_MINUS) ) {
        INIT_JVARVALUE(&jvv2);
        opkw = (*pjtok)->jtok_kw;
        jstat = jrun_next_token(jx, pjtok);
        if (!jstat) {
            jstat = jexp_eval_mul_div(jx, pjtok, &jvv2);
        }
        if (!jstat) {
            if (opkw == JSPU_MINUS) {
                jstat = jexp_binop_sub(jx, jvv, &jvv2);
            } else {
                jstat = jexp_binop_add(jx, jvv, &jvv2);
            }
        }
    }

    return (jstat);
}
/***************************************************************/
static int jexp_eval_assignment(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    struct jvarvalue * jvv)
{
/*
**
*/
    int jstat;

    jstat = jexp_eval_add_sub(jx, pjtok, jvv);

    return (jstat);
}
/***************************************************************/
static int jexp_eval_list(
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
            jstat = jexp_eval_assignment(jx, pjtok, &jvv2);
        }
        if (!jstat) {
            jvar_add_jvarvalue_vallist(jvvl, &jvv2);
        }
    }

    if (!jstat && jvvl) {
        jvv->jvv_dtype = JVV_DTYPE_VALLIST;
        jvv->jvv_val.jvv_jvvl = jvvl;
    }

    return (jstat);
}
/***************************************************************/
#if USE_TABLE_EVAL
typedef int (*JXE_BINARY_OPERATOR_FUNCTION)
        (struct jrunexec          * jx,
         struct jvarvalue         * jvv1,
         struct jvarvalue         * jvv2);
typedef int (*JXE_UNARY_OPERATOR_FUNCTION)
        (struct jrunexec          * jx,
         struct jvarvalue         * jvvans,
         struct jvarvalue         * jvv1);

struct jexp_exp_rec { /* jxe_ */
    short jxe_bin_precedence;
    short jxe_unop_precedence;
    int jxe_pu;
    int jxe_opflags;
    JXE_BINARY_OPERATOR_FUNCTION jxe_binop;
    JXE_UNARY_OPERATOR_FUNCTION  jxe_unop;
} jexp_exp_list[] = {
    { 7, 0, JSPU_STAR_STAR         ,  5, jexp_binop_pow     , NULL              },
    { 0, 6, JSPU_BANG              ,  2, NULL               , jexp_unop_lognot  },
    { 5, 0, JSPU_STAR              ,  4, jexp_binop_mul     , NULL              },
    { 5, 0, JSPU_SLASH             ,  4, jexp_binop_div     , NULL              },
    { 4, 6, JSPU_PLUS              ,  6, jexp_binop_add     , jexp_unop_pos     },
    { 4, 6, JSPU_MINUS             ,  6, jexp_binop_sub     , jexp_unop_neg     },
    { 2, 0, JSPU_AMP_AMP           , 20, jexp_binop_logand  , NULL              },
    { 1, 0, JSPU_BAR_BAR           , 12, jexp_binop_logor   , NULL              },
    { 0, 0, 0                      ,  0, NULL               , NULL              }
};
#define JXE_OPFLAG_RTOL_ASSOCIATIVITY    1
#define JXE_OPFLAG_UNARY_OPERATION       2
#define JXE_OPFLAG_BINARY_OPERATION      4
#define JXE_OPFLAG_LOGICAL_OR            8  // if 1st op true, do not eval 2nd op */
#define JXE_OPFLAG_LOGICAL_AND          16  // if 1st op false, do not eval 2nd op */
#define JXE_OPFLAGS_LOGICAL             (JXE_OPFLAG_LOGICAL_AND | JXE_OPFLAG_LOGICAL_OR)
//#define HIGHEST_PRECEDENCE      2
#define GET_XLX(tok) (((tok)->jtok_typ == JSW_PU || (tok)->jtok_typ == JSW_KW)?jx->jx_aop_xref[(tok)->jtok_kw]:(-1))
/***************************************************************/
#define EVAL_STACK_SZ   16
#define JEXPST_DONE         0
#define JEXPST_FIRST_OPERAND    1
#define JEXPST_NEXT_OPERAND     2
#define JEXPST_BINARY_OPERATOR  3
//#define JEXPST_3            3
//#define JEXPST_4            4
//#define JEXPST_5            5
//#define JEXPST_6            6
//#define JEXPST_7            7
//#define JEXPST_8            8
#define XS_INFO_UNOP            1
#define XS_INFO_OPEN_PAREN      2
#define XS_INFO_OPERAND         3
//#define XS_INFO_BINOP_OPERAND   4
#define XS_INFO_BINOP           4
struct expession_stack { /* xs_ */
    int xs_xlx;
    short xs_info;
    short xs_ignore;
    struct jvarvalue xs_jvv;
};
#define COPY_XSREC(t,s) (memcpy((t), (s), sizeof(struct expession_stack)))
/***************************************************************/
static void jexp_quick_tostring(
                    struct jrunexec * jx,
                    struct jvarvalue * jvv,
                    char * valstr,
                    int valstrsz)
{
/*
**
*/
    char nbuf[32];

    switch (jvv->jvv_dtype) {
        case JVV_DTYPE_BOOL   :
            sprintf(nbuf, "%s", (jvv->jvv_val.jvv_val_bool==0)?"false":"true");
            strmcpy(valstr, nbuf, valstrsz);
            break;
        case JVV_DTYPE_JSINT   :
            sprintf(nbuf, "%d", jvv->jvv_val.jvv_val_jsint);
            strmcpy(valstr, nbuf, valstrsz);
            break;
        case JVV_DTYPE_JSFLOAT   :
            sprintf(nbuf, "%g", jvv->jvv_val.jvv_val_jsfloat);
            strmcpy(valstr, nbuf, valstrsz);
            break;
        case JVV_DTYPE_CHARS   :
            if (jvv->jvv_val.jvv_val_chars.jvvc_length < valstrsz) {
                memcpy(valstr, jvv->jvv_val.jvv_val_chars.jvvc_val_chars,
                               jvv->jvv_val.jvv_val_chars.jvvc_length);
                valstr[jvv->jvv_val.jvv_val_chars.jvvc_length] = '\0';
            } else {
                memcpy(valstr, jvv->jvv_val.jvv_val_chars.jvvc_val_chars,
                               valstrsz - 1);
                valstr[valstrsz - 1] = '\0';
            }
            break;
        default:
            sprintf(nbuf, "*Unknown type=%d*", jvv->jvv_dtype);
            strmcpy(valstr, nbuf, valstrsz);
            break;
    }
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
    char valstr[32];
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
        if (xstack[ii].xs_ignore) strcpy(ignstr, "Ignore");

        printf("%2d %-8s %2s %-6s %-10s %s\n", ii, punam, prestr, ignstr, info, valstr);
    }
}
/***************************************************************/
static int jexp_eval_stack(
                    struct jrunexec * jx,
                    struct expession_stack * xstack,
                    int * depth,
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

    while (!jstat && !done) {
        jexp_print_stack(jx, "Stack pre-eval", xstack, (*depth), precedence, opflags);
        if (*depth <= 1) {
            if (*depth == 1 && (opflags && JXE_OPFLAGS_LOGICAL)) {
                jstat = jexp_istrue(jx, &(xstack[*depth - 1].xs_jvv), &istrue);
                if (!jstat) {
                    if ((istrue && (opflags && JXE_OPFLAG_LOGICAL_OR))   ||
                        (!istrue && (opflags && JXE_OPFLAG_LOGICAL_AND)) ) {
                        xstack[*depth - 1].xs_ignore = 1;
                    }
                }
            }
            done = 1;
        } else {
            switch (xstack[*depth - 1].xs_info) {
                case XS_INFO_OPEN_PAREN:
                    (*depth)--;
                    done = 1;
                    break;
 
                case XS_INFO_UNOP:
                    jstat = jrun_set_error(jx, JSERR_INTERNAL_ERROR,
                        "Unary operator with no operand");
                    break;

                case XS_INFO_OPERAND:
                    done = 1;
                    xlx = xstack[*depth - 2].xs_xlx;
                    while (!jstat && xlx >= 0 && (*depth) > 1 &&
                           xstack[*depth - 2].xs_info == XS_INFO_UNOP &&
                           jexp_exp_list[xlx].jxe_unop_precedence > precedence) {
                        (*depth)--;
                        unop = jexp_exp_list[xlx].jxe_unop;
                        if (unop) jstat = unop(jx, &(xstack[*depth - 1].xs_jvv), &(xstack[*depth].xs_jvv));
                        xstack[*depth - 1].xs_info = XS_INFO_OPERAND;
                        if (*depth > 1) xlx = xstack[*depth - 2].xs_xlx;
                        else xlx = -1;
                    }
                    if (!jstat && (*depth) > 2 &&
                           xstack[*depth - 2].xs_info == XS_INFO_BINOP) {
                        xlx = xstack[*depth - 2].xs_xlx;
                        if (xlx < 0) {
                            jstat = jrun_set_error(jx, JSERR_INTERNAL_ERROR,
                                "Invalid operator on binary op. Found: %d", xlx);
                        } else if (jexp_exp_list[xlx].jxe_bin_precedence > precedence ||
                                   (jexp_exp_list[xlx].jxe_bin_precedence == precedence &&
                                    (jexp_exp_list[xlx].jxe_opflags & JXE_OPFLAG_RTOL_ASSOCIATIVITY) == 0)) {
                            binop = jexp_exp_list[xlx].jxe_binop;
                            if (binop) jstat = binop(jx, &(xstack[*depth - 3].xs_jvv), &(xstack[*depth - 1].xs_jvv));
                            (*depth) -= 2;
                            done = 0;
                        }
                    }
                    if (!jstat && !precedence && (*depth) > 1 &&
                           xstack[*depth - 2].xs_info == XS_INFO_OPEN_PAREN) {
                        (*depth)--;
                        COPY_XSREC(&(xstack[*depth - 1]), &(xstack[*depth]));
                        done = 1;
                    }
                    break;

                case XS_INFO_BINOP:
                    jstat = jrun_set_error(jx, JSERR_INTERNAL_ERROR,
                        "Binary operator with no operand");
                    break;

                default:
                    jstat = jrun_set_error(jx, JSERR_INTERNAL_ERROR,
                        "Invalid expression info. Found: %d", xstack[*depth].xs_info);
                    break;
            }
        }
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
**
*/
    int jstat;
    int state;
    int depth;
    int xlx;
    struct expession_stack xstack[EVAL_STACK_SZ];

    jstat       = 0;
    depth       = 0;
    state       = JEXPST_FIRST_OPERAND;
    xlx         = GET_XLX(*pjtok);

    while (!jstat && state != JEXPST_DONE) {
        switch (state) {
            case JEXPST_FIRST_OPERAND:
                if (xlx >= 0 && jexp_exp_list[xlx].jxe_unop) {
                    xstack[depth].xs_xlx  = xlx;
                    xstack[depth].xs_info = XS_INFO_UNOP;
                    xstack[depth].xs_ignore = 0;
                    INIT_JVARVALUE(&(xstack[depth].xs_jvv));
                    depth++;
                    jstat = jrun_next_token(jx, pjtok);
                    xlx = GET_XLX(*pjtok);
                } else if ((*pjtok)->jtok_typ == JSW_PU && (*pjtok)->jtok_kw == JSPU_OPEN_PAREN) {
                    xstack[depth].xs_xlx  = -1;
                    xstack[depth].xs_info = XS_INFO_OPEN_PAREN;
                    xstack[depth].xs_ignore = 0;
                    INIT_JVARVALUE(&(xstack[depth].xs_jvv));
                    depth++;
                    jstat = jrun_next_token(jx, pjtok);
                    xlx = GET_XLX(*pjtok);
                } else {
                    INIT_JVARVALUE(&(xstack[depth].xs_jvv));
                    jstat = jexp_eval_primary(jx, pjtok, &(xstack[depth].xs_jvv));
                    xstack[depth].xs_xlx  = xlx;
                    xstack[depth].xs_info = XS_INFO_OPERAND;
                    xstack[depth].xs_ignore = 0;
                    depth++;
                    xlx = GET_XLX(*pjtok);
                    state       = JEXPST_BINARY_OPERATOR;
                }
                break;

            case JEXPST_BINARY_OPERATOR:
                if (xlx < 0) {
                    if ((*pjtok)->jtok_typ == JSW_PU && (*pjtok)->jtok_kw == JSPU_CLOSE_PAREN) {
                        jstat = jexp_eval_stack(jx, xstack, &depth, 0, 0);
                        if (!jstat) {
                            //jexp_print_stack(jx, "After stack eval", xstack, depth, 0);
                            jstat = jrun_next_token(jx, pjtok);
                            xlx = GET_XLX(*pjtok);
                        }
                    } else {
                        state = JEXPST_DONE;
                    }
                } else {
                    jstat = jexp_eval_stack(jx, xstack, &depth,
                        jexp_exp_list[xlx].jxe_bin_precedence, jexp_exp_list[xlx].jxe_opflags);
                    xstack[depth].xs_xlx  = xlx;
                    xstack[depth].xs_info = XS_INFO_BINOP;
                    xstack[depth].xs_ignore = xstack[depth-1].xs_ignore;
                    depth++;
                    jstat = jrun_next_token(jx, pjtok);
                    xlx = GET_XLX(*pjtok);
                    state = JEXPST_FIRST_OPERAND;
                }
                break;
        }
    }

    /* Operators are [0..depth-1], operands are [0..depth] */
    while (!jstat && depth > 1) {
        jstat = jexp_eval_stack(jx, xstack, &depth, 0, 0);
    }

    if (!jstat) COPY_JVARVALUE(jvv, &(xstack[0].xs_jvv));

    return (jstat);
}
#endif
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
#if ! USE_TABLE_EVAL
static int jexp_eval(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    struct jvarvalue * jvv)
{
    int jstat;

    jstat = jexp_eval_list(jx, pjtok, jvv);

    return (jstat);
}
#endif
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
**     21     Grouping                        n/a             (  )
**     20     Member Access                   left-to-right    . 
**            Computed Member Access          left-to-right    [  ]
**            new (with argument list)        n/a             new  (  )
**            Function Call                   left-to-right    (  )
**            Optional chaining               left-to-right   ?.
**     19     new (without argument list)     right-to-left   new 
**     18     Postfix Increment               n/a              ++
**            Postfix Decrement                                --
**     17     Logical NOT                     right-to-left   ! 
**            Bitwise NOT                                     ~ 
**            Unary Plus                                      + 
**            Unary Negation                                  - 
**            Prefix Increment                                ++ 
**            Prefix Decrement                                -- 
**            typeof                                          typeof 
**            void                                            void 
**            delete                                          delete 
**            await                                           await 
**     16     Exponentiation                  right-to-left    ** 
**     15     Multiplication                  left-to-right    * 
**            Division                                         / 
**            Remainder                                        % 
**     14     Addition                        left-to-right    + 
**            Subtraction                                      - 
**     13     Bitwise Left Shift              left-to-right    << 
**            Bitwise Right Shift                              >> 
**            Bitwise Unsigned Right Shift                     >>> 
**     12     Less Than                       left-to-right    < 
**            Less Than Or Equal                               <= 
**            Greater Than                                     > 
**            Greater Than Or Equal                            >= 
**            in                                               in 
**            instanceof                                       instanceof 
**     11     Equality                        left-to-right    == 
**            Inequality                                       != 
**            Strict Equality                                  === 
**            Strict Inequality                                !== 
**     10     Bitwise AND                     left-to-right    & 
**     9      Bitwise XOR                     left-to-right    ^ 
**     8      Bitwise OR                      left-to-right    | 
**     7      Logical AND                     left-to-right    && 
**     6      Logical OR                      left-to-right    || 
**     5      Nullish coalescing operator     left-to-right    ?? 
**     4      Conditional                     right-to-left    ?  : 
**     3      Assignment                      right-to-left    = 
**                                                             += 
**                                                             -= 
**                                                             **= 
**                                                             *= 
**                                                             /= 
**                                                             %= 
**                                                             <<= 
**                                                             >>= 
**                                                             >>>= 
**                                                             &= 
**                                                             ^= 
**                                                             |= 
**                                                             &&= 
**                                                             ||= 
**                                                             ??= 
**     2      yield                           right-to-left   yield 
**            yield*                                          yield* 
**     1      Comma / Sequence                left-to-right    , 
*/
    int jstat = 0;
    char outstr[128];

    INIT_JVARVALUE(jvv);
    jstat = jexp_eval(jx, pjtok, jvv);
    if (!jstat) {
        jexp_quick_tostring(jx, jvv, outstr, sizeof(outstr));
        /* jstat = jrun_int_tostring(jx, jvv, &outstr); */
        if (!jstat) {
            printf("jexp_evaluate=<%s>\n", outstr);
            /* Free(outstr); */
        }
    }

    return (jstat);
}
/***************************************************************/
