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

#define DEBUG_PRINT_EXP_STACK   0
#define DEBUG_PRINT_EVAL_RESULT 0

#define COMPARE(x,y) (((x)==(y))?0:(((x)<(y))?-1:1))
#define BOOLVAL(b) (((b)==0)?0:1)

static int jexp_eval_list_inactive(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok);

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
                Realloc(jvvtgt->jvv_val.jvv_val_chars.jvvc_val_chars, char,\
                 jvvtgt->jvv_val.jvv_val_chars.jvvc_size);
    }

    memcpy(jvvtgt->jvv_val.jvv_val_chars.jvvc_val_chars, jchars, jcharslen);
    jvvtgt->jvv_val.jvv_val_chars.jvvc_val_chars[jcharslen] = '\0';
    jvvtgt->jvv_val.jvv_val_chars.jvvc_length = jcharslen;
}
/***************************************************************/
#if 0
static void jvar_store_jvarvalue(
    struct jvarvalue * jvvtgt,
    struct jvarvalue * jvvsrc)
{
/*
** In var.c
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
#endif
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
/***************************************************************/
static int jvarvalue_to_chars(
    struct jrunexec * jx,
    struct jvarvalue * jvvtgt,
    struct jvarvalue * jvvsrc)
{
/*
** See also: jrun_int_tostring()
*/
    int jstat = 0;
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
            jvar_store_chars_len(jvvtgt,
                jvvsrc->jvv_val.jvv_val_chars.jvvc_val_chars,
                jvvsrc->jvv_val.jvv_val_chars.jvvc_length);
            break;
        default:
            break;
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

    jvv1 = GET_RVAL(ajvv1);

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
         struct jvarvalue         * ajvv1)
{
/*
** Logical NOT
*/
    int jstat = 0;
    struct jvarvalue * jvv1;

    jvv1 = GET_RVAL(ajvv1);

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
         struct jvarvalue         * ajvv1)
{
/*
**
*/
    int jstat = 0;
    struct jvarvalue * jvv1;

    jvv1 = GET_RVAL(ajvv1);

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

        default:
            jstat = jrun_set_error(jx, JSERR_OPERATOR_TYPE_MISMATCH,
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

    jvv1 = GET_RVAL(ajvv1);
    jvv2 = GET_RVAL(ajvv2);

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

    jvv1 = GET_RVAL(ajvv1);
    jvv2 = GET_RVAL(ajvv2);

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

    jvv1 = GET_RVAL(ajvv1);
    jvv2 = GET_RVAL(ajvv2);

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
                        jvar_cat_chars(ajvv1,
                            &(jvvchars.jvv_val.jvv_val_chars),
                            &(jvv2->jvv_val.jvv_val_chars));
                        jvar_free_jvarvalue_data(&jvvchars);
                    }
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
                       jvar_cat_chars(ajvv1,
                            &(jvvchars.jvv_val.jvv_val_chars),
                            &(jvv2->jvv_val.jvv_val_chars));
                        jvar_free_jvarvalue_data(&jvvchars);
                    }
                    break;
                default:
                    jstat = jrun_set_error(jx, JSERR_OPERATOR_TYPE_MISMATCH,
                        "Illegal floating addition");
                    break;
            }
            break;

        case JVV_DTYPE_CHARS :
            INIT_JVARVALUE(&(jvvchars));
            jstat = jvarvalue_to_chars(jx, &jvvchars, jvv2);
            if (!jstat) {
                jvar_cat_chars(ajvv1,
                    &(jvv1->jvv_val.jvv_val_chars),
                    &(jvvchars.jvv_val.jvv_val_chars));
                jvar_free_jvarvalue_data(&jvvchars);
                jvar_free_jvarvalue_data(ajvv2);
            }
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
    struct jvarvalue * ajvv1,
    struct jvarvalue * ajvv2)
{
/*
**
*/
    int jstat = 0;
    struct jvarvalue * jvv1;
    struct jvarvalue * jvv2;

    jvv1 = GET_RVAL(ajvv1);
    jvv2 = GET_RVAL(ajvv2);

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
    struct jvarvalue * ajvv1,
    struct jvarvalue * ajvv2)
{
/*
**
*/
    int jstat = 0;
    struct jvarvalue * jvv1;
    struct jvarvalue * jvv2;

    jvv1 = GET_RVAL(ajvv1);
    jvv2 = GET_RVAL(ajvv2);

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
    struct jvarvalue * ajvv1,
    struct jvarvalue * ajvv2)
{
/*
**
*/
    int jstat = 0;
    struct jvarvalue * jvv1;
    struct jvarvalue * jvv2;

    jvv1 = GET_RVAL(ajvv1);
    jvv2 = GET_RVAL(ajvv2);

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

    jvv1 = GET_RVAL(ajvv1);
    jvv2 = GET_RVAL(ajvv2);

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
    } else {
        cmpval = strncmp(jvvc1->jvvc_val_chars, jvvc2->jvvc_val_chars, jvvc2->jvvc_length);
        if (!cmpval && jvvc1->jvvc_length > jvvc2->jvvc_length) cmpval = 1;
    }

    return (cmpval);
}
/***************************************************************/
static int jexp_compare(struct jrunexec * jx,
    struct jvarvalue * ajvv1,
    struct jvarvalue * ajvv2,
    int * cmpval)
{
/*
** 12/08/2020
*/
    int jstat = 0;
    struct jvarvalue * jvv1;
    struct jvarvalue * jvv2;

    jvv1 = GET_RVAL(ajvv1);
    jvv2 = GET_RVAL(ajvv2);

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
                    (*cmpval) = COMPARE((JSFLOAT)jvv1->jvv_val.jvv_val_bool, jvv2->jvv_val.jvv_val_jsfloat);
                    break;
                default:
                    jstat = jrun_set_error(jx, JSERR_OPERATOR_TYPE_MISMATCH,
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
                    (*cmpval) = COMPARE((JSFLOAT)jvv1->jvv_val.jvv_val_jsint, jvv2->jvv_val.jvv_val_jsfloat);
                    break;
                default:
                    jstat = jrun_set_error(jx, JSERR_OPERATOR_TYPE_MISMATCH,
                        "Illegal integer compare");
                    break;
            }
            break;

        case JVV_DTYPE_JSFLOAT:
            switch (jvv2->jvv_dtype) {
                case JVV_DTYPE_BOOL   :
                    (*cmpval) = COMPARE(jvv1->jvv_val.jvv_val_jsfloat, (JSFLOAT)jvv2->jvv_val.jvv_val_bool);
                    break;
                case JVV_DTYPE_JSINT   :
                    (*cmpval) = COMPARE(jvv1->jvv_val.jvv_val_jsfloat, (JSFLOAT)jvv2->jvv_val.jvv_val_jsint);
                    break;
                case JVV_DTYPE_JSFLOAT   :
                    (*cmpval) = COMPARE(jvv1->jvv_val.jvv_val_jsfloat, jvv2->jvv_val.jvv_val_jsfloat);
                    break;
                default:
                    jstat = jrun_set_error(jx, JSERR_OPERATOR_TYPE_MISMATCH,
                        "Illegal floating compare");
                    break;
            }
            break;

        case JVV_DTYPE_CHARS:
            switch (jvv2->jvv_dtype) {
                case JVV_DTYPE_CHARS   :
                    (*cmpval) = jexp_compare_chars(&(jvv1->jvv_val.jvv_val_chars), &(jvv2->jvv_val.jvv_val_chars));
                    break;
                default:
                    jstat = jrun_set_error(jx, JSERR_OPERATOR_TYPE_MISMATCH,
                        "Illegal character compare");
                    break;
            }
            break;

        default:
            jstat = jrun_set_error(jx, JSERR_OPERATOR_TYPE_MISMATCH,
                "Illegal compare");
            break;
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

    jstat = jexp_compare(jx, jvv1, jvv2, &cmpval);
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

    jstat = jexp_compare(jx, jvv1, jvv2, &cmpval);
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

    jstat = jexp_compare(jx, jvv1, jvv2, &cmpval);
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

    jstat = jexp_compare(jx, jvv1, jvv2, &cmpval);
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

    jstat = jexp_compare(jx, jvv1, jvv2, &cmpval);
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

    jstat = jexp_compare(jx, jvv1, jvv2, &cmpval);
    if (!jstat) {
        jvar_store_bool(jvv1, (cmpval >= 0));
    }          

    return (jstat);
}
/***************************************************************/
static int jexp_eval_function_call_ignored(
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
static void free_jvarvalue_funcarg(struct jvarvalue_funcarg * jvvfa)
{
/*
** 02/08/2021
*/
    Free(jvvfa->jvvfa_arg_name);

    Free(jvvfa);
}
/***************************************************************/
void free_jvarvalue_function(struct jvarvalue_function * jvvf)
{
/*
** 02/08/2021
*/
    int ii;

    Free(jvvf->jvvf_func_name);
    for (ii = 0; ii < jvvf->jvvf_nargs; ii++) {
        free_jvarvalue_funcarg(jvvf->jvvf_aargs[ii]);
    }
    Free(jvvf->jvvf_aargs);

    Free(jvvf);
}
/***************************************************************/
static struct jvarvalue_function * new_jvarvalue_function(void)
{
/*
** 02/08/2021
*/
    struct jvarvalue_function * jvvf;

    jvvf = New(struct jvarvalue_function, 1);
    jvvf->jvvf_func_name = NULL;
    jvvf->jvvf_flags = 0;
    jvvf->jvvf_nargs = 0;
    jvvf->jvvf_xargs = 0;
    jvvf->jvvf_aargs = NULL;
    jrun_init_jxcursor(&(jvvf->jvvf_begin_jxc));

    return (jvvf);
}
/***************************************************************/
static struct jvarvalue_funcarg * new_jvarvalue_funcarg(char * argname)
{
/*
** 02/12/2021
*/
    struct jvarvalue_funcarg * jvvfa;

    jvvfa = New(struct jvarvalue_funcarg, 1);
    jvvfa->jvvfa_arg_name = Strdup(argname);

    return (jvvfa);
}
/***************************************************************/
static int jvar_parse_function_body(
                    struct jrunexec * jx,
                    struct jvarvalue_function * jvvf,
                    struct jtoken ** pjtok)
{
/*
** 02/11/2021
*/
    int jstat = 0;

    return (jstat);
}
/***************************************************************/
static int jvar_add_function_argument(
                    struct jrunexec * jx,
                    struct jvarvalue_function * jvvf,
                    struct jtoken ** pjtok)
{
/*
** 02/12/2021
*/
    int jstat = 0;
    struct jvarvalue_funcarg * jvvfa;

    jvvfa = new_jvarvalue_funcarg((*pjtok)->jtok_text);

    if (jvvf->jvvf_nargs == jvvf->jvvf_xargs) {
        if (!jvvf->jvvf_nargs) jvvf->jvvf_xargs = 2;
        else                   jvvf->jvvf_xargs *= 2;
        jvvf->jvvf_aargs = Realloc(jvvf->jvvf_aargs, struct jvarvalue_funcarg *, jvvf->jvvf_xargs);
    }
    jvvf->jvvf_aargs[jvvf->jvvf_nargs] = jvvfa;
    jvvf->jvvf_nargs += 1;

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
    int ii;
    char * argname = (*pjtok)->jtok_text;

    jstat = jvar_validate_varname(jx, (*pjtok));
    if (!jstat) {
        for (ii = 0; ii < jvvf->jvvf_nargs; ii++) {
            if (!strcmp(jvvf->jvvf_aargs[ii]->jvvfa_arg_name, argname)) {
                jstat = jrun_set_error(jx, JSERROBJ_SCRIPT_SYNTAX_ERROR,
                    "Duplicate argument name. Found: '%s'", (*pjtok)->jtok_text);
            }
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
        } else {
            jstat = jvar_validate_function_argname(jx, jvvf, pjtok);
            if (!jstat) {
                jstat = jvar_add_function_argument(jx, jvvf, pjtok);
            }
        }
    }

    return (jstat);
}
/***************************************************************/
static int jvar_parse_function(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    struct jvarvalue * jvv)
{
/*
** 02/08/2021
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
                jstat = jvar_parse_function_body(jx, jvvf, pjtok);
            }
        } else {
            jstat = jrun_set_error(jx, JSERROBJ_SCRIPT_SYNTAX_ERROR,
                "Expecting '(' to begin function arguments. Found: '%s'", (*pjtok)->jtok_text);
        }
    }

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
                jstat = jrun_set_error(jx, JSERR_INVALID_NUMBER,
                    "Invalid number: %s", (*pjtok)->jtok_text);
            } else {
                jstat = jrun_next_token(jx, pjtok);
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
                case JSKW_FUNCTION:
                    jstat = jrun_next_token(jx, pjtok);
                    if (!jstat) jstat = jvar_parse_function(jx, pjtok, jvv);
                    break;
                default:
                    jstat = jrun_set_error(jx, JSERR_UNEXPECTED_KEYWORD,
                        "Unexpected keyword: %s", (*pjtok)->jtok_text);
                break;
            }
            if (!jstat) jstat = jrun_next_token(jx, pjtok);
            break;

        case JSW_ID:
            vjvv = jvar_find_variable(jx, (*pjtok)->jtok_text);
            if (vjvv) {
                jvar_store_lval(jx, jvv, vjvv);
            } else {
                jstat = jrun_set_error(jx, JSERR_UNDEFINED_VARIABLE,
                    "Undefined variable: %s", (*pjtok)->jtok_text);
            }
            if (!jstat) jstat = jrun_next_token(jx, pjtok);
            break;

        default:
            jstat = jrun_set_error(jx, JSERR_INVALID_TOKEN_TYPE,
                "Invalid token type %d: %s", (*pjtok)->jtok_typ, (*pjtok)->jtok_text);
            break;
    }

    return (jstat);
}
/***************************************************************/
static int jexp_eval_primary_ignored(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    struct jvarvalue * jvv)
{
/*
**
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
        case JSW_PU:
            if ((*pjtok)->jtok_kw == JSPU_OPEN_PAREN) {
                jstat = jrun_next_token(jx, pjtok);
            } else {
                jstat = jrun_set_error(jx, JSERR_UNEXPECTED_SPECIAL,
                    "Unexpected special: %s", (*pjtok)->jtok_text);
            }
            break;
        case JSW_KW:
            jstat = jrun_next_token(jx, pjtok);
            break;
        case JSW_ID:
            jrun_peek_token(jx, &next_jtok);
            if (next_jtok.jtok_kw == JSPU_OPEN_PAREN) {
                jstat = jexp_eval_function_call_ignored(jx, pjtok, jvv);
            } else {
                jstat = jrun_next_token(jx, pjtok);
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
static int jexp_method_call(
                    struct jrunexec * jx,
                    struct jvarvalue * mjvv,
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

    if (jvvparms->jvv_dtype == JVV_DTYPE_VALLIST) {
        jvvargs = jvvparms;
    } else {
        jvvl.jvvl_nvals = 1;
        jvvl.jvvl_xvals = 1;
        jvvl.jvvl_avals = jvvparms;
        jvvargrec.jvv_dtype = JVV_DTYPE_VALLIST;
        jvvargrec.jvv_val.jvv_jvvl = &jvvl;
        jvvargs = &jvvargrec;
    }
    INIT_JVARVALUE(rtnjvv);

    if (mjvv->jvv_dtype == JVV_DTYPE_INTERNAL_METHOD) {
        jstat = (mjvv->jvv_val.jvv_int_method.jvvim_method)
            (jx, mjvv->jvv_val.jvv_int_method.jvvim_method_name, jvvargs, rtnjvv);
    } else {
        jstat = jrun_set_error(jx, JSERR_EXP_METHOD,
            "Expecting method name.");
    }
    jvar_free_jvarvalue_data(jvvparms);

    return (jstat);
}
/***************************************************************/
static int jexp_binop_dot(struct jrunexec * jx,
    struct jvarvalue * ajvv1,
    struct jvarvalue * ajvv2)
{
/*
**
*/
    int jstat = 0;
    struct jvarvalue * cjvv;
    struct jvarvalue * mjvv;

    cjvv = GET_RVAL(ajvv1);

    if (cjvv->jvv_dtype != JVV_DTYPE_OBJECT && cjvv->jvv_dtype != JVV_DTYPE_INTERNAL_CLASS) {
        jstat = jrun_set_error(jx, JSERR_EXP_OBJECT_AFTER_DOT,
            "Expecting object after dot.");
    } else if (ajvv2->jvv_dtype != JVV_DTYPE_TOKEN) {
                jstat = jrun_set_error(jx, JSERR_EXP_ID_AFTER_DOT,
                    "Expecting identifier after dot.");
    } else {
        mjvv = jvar_object_member(jx, cjvv, ajvv2->jvv_val.jvv_val_token.jvvt_token);
        if (!mjvv) {
            jstat = jrun_set_error(jx, JSERR_UNDEFINED_CLASS_MEMBER,
                "Undefined class member: %s",
                ajvv2->jvv_val.jvv_val_token.jvvt_token);
        } else {
            jvar_shallow_copy_jvarvalue(ajvv1, mjvv);
        }
        jvar_free_jvarvalue_data(ajvv2);
    }

    return (jstat);
}
/***************************************************************/
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
    { 9, 0, JSPU_DOT               , 68, jexp_binop_dot     , NULL              },
    { 8, 0, JSPU_STAR_STAR         ,  5, jexp_binop_pow     , NULL              },
    { 0, 7, JSPU_BANG              ,  2, NULL               , jexp_unop_lognot  },
    { 6, 0, JSPU_STAR              ,  4, jexp_binop_mul     , NULL              },
    { 6, 0, JSPU_SLASH             ,  4, jexp_binop_div     , NULL              },
    { 5, 7, JSPU_PLUS              ,  6, jexp_binop_add     , jexp_unop_pos     },
    { 5, 7, JSPU_MINUS             ,  6, jexp_binop_sub     , jexp_unop_neg     },
    { 4, 0, JSPU_EQ_EQ             ,  4, jexp_binop_cmp_eq  , NULL              },
    { 4, 0, JSPU_BANG_EQ           ,  4, jexp_binop_cmp_ne  , NULL              },
    { 4, 0, JSPU_LT_EQ             ,  4, jexp_binop_cmp_le  , NULL              },
    { 4, 0, JSPU_LT                ,  4, jexp_binop_cmp_lt  , NULL              },
    { 4, 0, JSPU_GT                ,  4, jexp_binop_cmp_gt  , NULL              },
    { 4, 0, JSPU_GT_EQ             ,  4, jexp_binop_cmp_ge  , NULL              },
    { 3, 0, JSPU_AMP_AMP           , 20, jexp_binop_logand  , NULL              },
    { 2, 0, JSPU_BAR_BAR           , 12, jexp_binop_logor   , NULL              },
    { 0, 0, 0                      ,  0, NULL               , NULL              }
};
#define JXE_OPFLAG_RTOL_ASSOCIATIVITY    1
#define JXE_OPFLAG_UNARY_OPERATION       2
#define JXE_OPFLAG_BINARY_OPERATION      4
#define JXE_OPFLAG_LOGICAL_OR            8  // if 1st op true, do not eval 2nd op */
#define JXE_OPFLAG_LOGICAL_AND          16  // if 1st op false, do not eval 2nd op */
#define JXE_OPFLAGS_LOGICAL             (JXE_OPFLAG_LOGICAL_AND | JXE_OPFLAG_LOGICAL_OR)
#define JXE_OPFLAG_EQ(f,v) (((f) & (v)) == (v))
#define JXE_OPFLAG_CLOSE_PAREN          32
#define JXE_OPFLAG_TOKEN                64
#define JXE_OPFLAG_FUNCTION_CALL        128

//#define HIGHEST_PRECEDENCE      2
#define GET_XLX(tok) \
(((tok)->jtok_typ == JSW_PU || (tok)->jtok_typ == JSW_KW)?jx->jx_aop_xref[(tok)->jtok_kw]:(-1))
/***************************************************************/
#define EVAL_STACK_SZ   16

#define JEXPST_DONE             0
#define JEXPST_FIRST_OPERAND    1
#define JEXPST_NEXT_OPERAND     2
#define JEXPST_BINARY_OPERATOR  3
#define JEXPST_BINARY_IGNORED   4
#define JEXPST_FIRST_IGNORED    5
#define JEXPST_FUNCTION_CALL    6

#define XS_INFO_UNOP            1
#define XS_INFO_OPEN_PAREN      2
#define XS_INFO_OPERAND         3
#define XS_INFO_BINOP           4

struct expession_stack { /* xs_ */
    int xs_xlx;
    short xs_info;
    short xs_ignore;
    struct jvarvalue xs_jvv;
};
#define COPY_XSREC(t,s) (memcpy((t), (s), sizeof(struct expession_stack)))
/***************************************************************/
static int jeval_store_token(struct jrunexec * jx,
                    struct jtoken ** pjtok,
    struct jvarvalue * jvv)
{
/*
** 12/11/2020
*/
    int jstat = 0;

    jvv->jvv_dtype = JVV_DTYPE_TOKEN;
    jvv->jvv_val.jvv_val_token.jvvt_token = Strdup((*pjtok)->jtok_text);

    return (jstat);
}
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
    char nbuf[64];

    switch (jvv->jvv_dtype) {
        case JVV_DTYPE_NONE   :
            if (valstrsz > 0) valstr[0] = '\0';
            break;
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
        case JVV_DTYPE_LVAL:
            sprintf(nbuf, "l-value");
            strmcpy(valstr, nbuf, valstrsz);
            break;
        case JVV_DTYPE_INTERNAL_CLASS:
            sprintf(nbuf, "internal class");
            strmcpy(valstr, nbuf, valstrsz);
            break;
        case JVV_DTYPE_INTERNAL_METHOD:
            Snprintf(nbuf, sizeof(nbuf), "internal method %s()",
                jvv->jvv_val.jvv_int_method.jvvim_method_name);
            strmcpy(valstr, nbuf, valstrsz);
            break;
        case JVV_DTYPE_OBJECT:
            sprintf(nbuf, "object");
            strmcpy(valstr, nbuf, valstrsz);
            break;
        case JVV_DTYPE_VALLIST:
            sprintf(nbuf, "object");
            strmcpy(valstr, nbuf, valstrsz);
            break;
        case JVV_DTYPE_TOKEN:
            sprintf(nbuf, "token<%s>",  jvv->jvv_val.jvv_val_token.jvvt_token);
            strmcpy(valstr, nbuf, valstrsz);
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
        if (opflags & JXE_OPFLAG_CLOSE_PAREN) {
            if (opflagstr[0]) strcat(opflagstr, ",");
            strcat(opflagstr, "close paren");
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
                    jstat = jrun_set_error(jx, JSERR_INTERNAL_ERROR,
                        "Invalid operator on binary op. Found: %d", xlx);
                } else if (jexp_exp_list[xlx].jxe_bin_precedence > precedence ||
                            (jexp_exp_list[xlx].jxe_bin_precedence == precedence &&
            (jexp_exp_list[xlx].jxe_opflags & JXE_OPFLAG_RTOL_ASSOCIATIVITY) == 0)) {
                    binop = jexp_exp_list[xlx].jxe_binop;
                    if (binop)
                        jstat = binop(jx, &(xstack[*depth - 3].xs_jvv),
                            &(xstack[*depth - 1].xs_jvv));
                    /* jvar_free_jvarvalue_data(&(xstack[*depth - 1].xs_jvv));   */
                    /* jvar_free_jvarvalue_data(&(xstack[*depth - 2].xs_jvv));   */
                    (*depth) -= 2;
                } else {
                    done = 1;
                }
            } else if ((*depth) > 1 && xstack[*depth - 2].xs_info == XS_INFO_OPEN_PAREN) {
                if (opflags & JXE_OPFLAG_CLOSE_PAREN) {
                    (*depth)--;
                    COPY_XSREC(&(xstack[*depth - 1]), &(xstack[*depth]));
                    done = 1;
                } else {
                    done = 1;
                }
            } else {
                done = 1;
            }
        } else {
            jstat = jrun_set_error(jx, JSERR_INTERNAL_ERROR,
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

    jstat = jexp_eval_stack(jx, xstack, depth, 0, JXE_OPFLAG_FUNCTION_CALL);
    if (!jstat) jstat = jrun_next_token(jx, pjtok);
    if (!jstat) {
        INIT_JVARVALUE(&jvvparms);
        jstat = jexp_eval(jx, pjtok, &jvvparms);
    }
    /* jexp_print_stack(jx, "At jexp_eval_function_call()", xstack, *depth, 0, JSPU_OPEN_PAREN); */

    if (!jstat) {
        if ((*pjtok)->jtok_typ == JSW_PU && (*pjtok)->jtok_kw == JSPU_CLOSE_PAREN) {
            INIT_JVARVALUE(&rtnjvv);
            mjvv = &(xstack[*depth - 1].xs_jvv);
            jstat = jexp_method_call(jx, mjvv, &jvvparms, &rtnjvv);
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
            jstat = jrun_set_error(jx, JSERR_EXP_CLOSE_PAREN,
                "Expecting close parenthesis. Found: %s", (*pjtok)->jtok_text);
        }
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
                    /* jexp_print_stack(jx, "At JEXPST_FIRST_OPERAND", xstack, depth, 0, JSPU_OPEN_PAREN); */
                    if (depth > 1 && xstack[depth - 1].xs_info == XS_INFO_OPERAND) {
                        jstat = jexp_eval_function_call(jx, pjtok, xstack, &depth);
                        /* jexp_print_stack(jx, "After jexp_eval_function_call()", xstack, depth, 0, 0); */
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
                    jstat = jexp_eval_primary(jx, pjtok, &(xstack[depth].xs_jvv));
                    xstack[depth].xs_xlx  = xlx;
                    xstack[depth].xs_info = XS_INFO_OPERAND;
                    xstack[depth].xs_ignore = 0;
                    depth++;
                    xlx = GET_XLX(*pjtok);
                    //xlx = (((*pjtok)->jtok_typ == JSW_PU || (*pjtok)->jtok_typ == JSW_KW) ? jx->jx_aop_xref[(*pjtok)->jtok_kw] : (-1));
                    state       = JEXPST_BINARY_OPERATOR;
                }
                break;

            case JEXPST_BINARY_OPERATOR:
                if (xlx < 0) {
                    if ((*pjtok)->jtok_typ == JSW_PU && (*pjtok)->jtok_kw == JSPU_CLOSE_PAREN) {
                        jstat = jexp_eval_stack(jx, xstack, &depth, 0, JXE_OPFLAG_CLOSE_PAREN);
                        if (!jstat && depth > 1) {
                            jstat = jrun_next_token(jx, pjtok);
                            xlx = GET_XLX(*pjtok);
                        } else {
                            state = JEXPST_DONE;
                        }
                    } else {
                        state = JEXPST_DONE;
                    }
                } else {
                    jstat = jexp_eval_stack(jx, xstack, &depth,
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
                            if (!jstat && (jexp_exp_list[xlx].jxe_opflags & JXE_OPFLAG_TOKEN)) {
                                jstat = jeval_store_token(jx, pjtok, &(xstack[depth].xs_jvv));
                                xstack[depth].xs_xlx  = xlx;
                                xstack[depth].xs_info = XS_INFO_OPERAND;
                                xstack[depth].xs_ignore = 0;
                                depth++;
                                jstat = jrun_next_token(jx, pjtok);
                            }
                            xlx = GET_XLX(*pjtok);
                            state = JEXPST_FIRST_OPERAND;
                        }
                    }
                }
                break;

            case JEXPST_BINARY_IGNORED:
                if (xlx < 0) {
                    if ((*pjtok)->jtok_typ == JSW_PU && (*pjtok)->jtok_kw == JSPU_CLOSE_PAREN) {
                        //jexp_print_stack(jx, "At JEXPST_BINARY_IGNORED", xstack, depth, 0, JXE_OPFLAG_CLOSE_PAREN);
                        if (depth > 0 && xstack[depth - 1].xs_info == XS_INFO_OPEN_PAREN) {
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
                        } else {
                            jstat = jrun_set_error(jx, JSERR_INTERNAL_ERROR,
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
                    jstat = jexp_eval_primary_ignored(jx, pjtok, &(xstack[depth].xs_jvv));
                    xlx = GET_XLX(*pjtok);
                    state       = JEXPST_BINARY_IGNORED;
                }
                break;
        }
    }

    if (!jstat && depth > 1) {
        jstat = jexp_eval_stack(jx, xstack, &depth, 0, 0);
    }

    if (!jstat && depth > 1) {
        jstat = jrun_set_error(jx, JSERR_INTERNAL_ERROR,
            "Extra items on stack. Found: %d items", depth);
    }

    if (!jstat) {
        if (depth) COPY_JVARVALUE(jvv, &(xstack[0].xs_jvv));
        else INIT_JVARVALUE(jvv);
    }

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
static int jexp_eval_assignment(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    struct jvarvalue * jvv)
{
/*
** 01/12/2021
*/
    int jstat = 0;
    struct jtoken next_jtok;
    struct jvarvalue * vjvv;

    /*
    ** This is done this way to support 'var=val' when var has not been declared.
    ** In strict mode var must be declared so this is allowed under non-strict mode.
    */
    if ((*pjtok)->jtok_typ != JSW_ID) {
        jstat = jexp_eval_value(jx, pjtok, jvv);
    } else {
        jrun_peek_token(jx, &next_jtok);
        if ((*pjtok)->jtok_typ == JSW_ID && next_jtok.jtok_kw == JSPU_EQ) {
            vjvv = jvar_find_variable(jx, (*pjtok)->jtok_text);
            if (!vjvv) {
                if (jrun_get_strict_mode(jx)) {
                    jstat = jrun_set_error(jx, JSERROBJ_SCRIPT_REFERENCE_ERROR,
                        "%s is not defined", (*pjtok)->jtok_text, vjvv);
                } else {    /* not strict */
                    jstat = jvar_validate_varname(jx, (*pjtok));
                    if (!jstat) {
                        vjvv = jvar_new_jvarvalue();
                        jstat = jvar_insert_new_jvarvalue(jx, (*pjtok)->jtok_text, vjvv);
                    }
                }
            }
            if (!jstat) jstat = jrun_next_token(jx, pjtok); /* '=' */
            if (!jstat) jstat = jrun_next_token(jx, pjtok);
            if (!jstat) jstat = jexp_eval_value(jx, pjtok, jvv);
            if (!jstat) {
                jvar_store_jvarvalue(jx, vjvv, jvv);
                jvar_free_jvarvalue_data(jvv);
            }
        } else {
            jstat = jexp_eval_value(jx, pjtok, jvv);
            if (!jstat && (*pjtok)->jtok_kw == JSPU_EQ) {
                if (jvv->jvv_dtype != JVV_DTYPE_LVAL) {
                    jstat = jrun_set_error(jx, JSERROBJ_SCRIPT_SYNTAX_ERROR,
                        "Invalid left-hand side in assignment");
                } else {
                    vjvv = jvv->jvv_val.jvv_lval;
                    jstat = jrun_next_token(jx, pjtok); /* '=' */
                    if (!jstat) jstat = jrun_next_token(jx, pjtok);
                    if (!jstat) {
                        jstat = jexp_eval_value(jx, pjtok, jvv);
                        if (!jstat) {
                            jvar_store_jvarvalue(jx, vjvv, jvv);
                            jvar_free_jvarvalue_data(jvv);
                        }
                    }
                }
            }
        }
    }

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
static int jexp_eval_function_call_inactive(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok)
{
/*
** 01/19/2021
*/
    int jstat;

    jstat = jrun_next_token(jx, pjtok); /* ( */
    if (!jstat) {
        jstat = jexp_eval_list_inactive(jx, pjtok);
    }
    if (!jstat) {
        jstat = jrun_next_token(jx, pjtok); /* ) */
    }
    if (!jstat) {
        jstat = jrun_next_token(jx, pjtok);
    }

    return (jstat);
}
/***************************************************************/
static int jexp_eval_primary_inactive(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok)
{
/*
** 01/19/2021
*/
    int jstat;

    jstat = 0;
    switch ((*pjtok)->jtok_typ) {
        case JSW_STRING:
            jstat = jrun_next_token(jx, pjtok);
            break;
        case JSW_NUMBER:
            jstat = jrun_next_token(jx, pjtok);
            break;

        case JSW_KW:
            switch ((*pjtok)->jtok_kw) {
                case JSKW_FALSE:
                    break;
                case JSKW_TRUE:
                    break;
                case JSKW_NULL:
                    break;
                default:
                    jstat = jrun_set_error(jx, JSERR_UNEXPECTED_KEYWORD,
                        "Unexpected keyword: %s", (*pjtok)->jtok_text);
                break;
            }
            if (!jstat) jstat = jrun_next_token(jx, pjtok);
            break;

        case JSW_ID:
            jstat = jrun_next_token(jx, pjtok);
            if ((*pjtok)->jtok_kw == JSPU_OPEN_PAREN) {
                jstat = jexp_eval_function_call_inactive(jx, pjtok);
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
static int jexp_eval_inactive(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok)
{
/*
** 01/19/2021
*/
    int jstat;
    int parens;
    int done;
    int xlx;

    jstat       = 0;
    parens      = 0;
    done        = 0;

    while (!jstat && !done) {
        xlx = GET_XLX(*pjtok);
        while (!jstat && xlx >= 0 && (jexp_exp_list[xlx].jxe_opflags & JXE_OPFLAG_UNARY_OPERATION)) {
            jstat = jrun_next_token(jx, pjtok);
            xlx = GET_XLX(*pjtok);
        }
        if (!jstat) {
            if ((*pjtok)->jtok_kw == JSPU_OPEN_PAREN) {
                jstat = jrun_next_token(jx, pjtok);
                parens++;
            } else {
                if ((*pjtok)->jtok_kw == JSPU_CLOSE_PAREN) {
                    jstat = jrun_next_token(jx, pjtok);
                    parens--;
                } else {
                    jstat = jexp_eval_primary_inactive(jx, pjtok);
                }
                if (!jstat) {
                    xlx = GET_XLX(*pjtok);
                    if (xlx >= 0 && (jexp_exp_list[xlx].jxe_opflags & JXE_OPFLAG_BINARY_OPERATION)) {
                        jstat = jrun_next_token(jx, pjtok);
                    } else {
                        if (!parens) done = 1;
                    }
                }
            }
        }
    }

    return (jstat);
}
/***************************************************************/
static int jexp_eval_assignment_inactive(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok)
{
/*
** 01/19/2021
*/
    int jstat;

    jstat = jexp_eval_inactive(jx, pjtok);
    while (!jstat &&
           (*pjtok)->jtok_kw == JSPU_EQ ) {
        jstat = jrun_next_token(jx, pjtok);
        if (!jstat) {
            jstat = jexp_eval_inactive(jx, pjtok);
        }
    }

    return (jstat);
}
/***************************************************************/
static int jexp_eval_list_inactive(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok)
{
/*
** 01/19/2021
*/
    int jstat;

    jstat = jexp_eval_assignment_inactive(jx, pjtok);
    while (!jstat &&
           (*pjtok)->jtok_kw == JSPU_COMMA ) {
        jstat = jrun_next_token(jx, pjtok);
        if (!jstat) {
            jstat = jexp_eval_assignment_inactive(jx, pjtok);
        }
    }

    return (jstat);
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

    jstat = jexp_eval_list_inactive(jx, pjtok);

    return (jstat);
}
/***************************************************************/
