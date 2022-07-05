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
                    jvar_store_boolean(jvv, 0);
                    break;
                case JSKW_TRUE:
                    jvar_store_boolean(jvv, 1);
                    break;
                case JSKW_NULL:
                    jvar_store_null(jvv);
                    break;
                default:
                    jstat = jrun_set_error(jx, JSERR_UNEXPECTED_KEYWORD,
                        "Unexpected keyword: %s", (*pjtok)->jtok_text);
                break;
            }
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
         struct jvarvalue         * jvv1);

struct jexp_exp_rec { /* jxe_ */
    int jxe_precedence;
    int jxe_pu;
    int jxe_rtol_associativity;
    JXE_BINARY_OPERATOR_FUNCTION jxe_binop;
    JXE_UNARY_OPERATOR_FUNCTION  jxe_unop;
} jexp_exp_list[] = {
    { 2, JSPU_SLASH             , 0, jexp_binop_div     , NULL              },
    { 2, JSPU_STAR              , 0, jexp_binop_mul     , NULL              },
    { 1, JSPU_PLUS              , 0, jexp_binop_add     , NULL              },
    { 1, JSPU_MINUS             , 0, jexp_binop_sub     , NULL              },
    { 0, 0                      , 0, NULL               , NULL              }
};
#define HIGHEST_PRECEDENCE      2
//#define MAX_EXP_DEPTH           (HIGHEST_PRECEDENCE + 4)
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

    for (ii = 0; jexp_exp_list[ii].jxe_precedence; ii++) {
        jx->jx_aop_xref[jexp_exp_list[ii].jxe_pu] = ii;
    }
}
/***************************************************************/
#if 0
static int jexp_eval(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    struct jvarvalue * jvv)
{
    int jstat;
    int done;
    int precdone;
    int prec;
    int xlx;
    struct jvarvalue jvva[HIGHEST_PRECEDENCE + 1];
    int xlxa[HIGHEST_PRECEDENCE + 1];

    jstat = 0;
    done = 0;
    prec = 2;

    while (!done && !jstat) {
        jstat = jexp_eval_primary(jx, pjtok, jvv);
        precdone = 0;
        while (!precdone && !jstat) {
            xlx = -1;
            jstat = jrun_next_token(jx, pjtok); /* gets operator */
            if (!jstat) {
                if ((*pjtok)->jtok_typ == JSW_PU || (*pjtok)->jtok_typ == JSW_KW) {
                    xlx = jx->jx_aop_xref[(*pjtok)->jtok_kw];
                }
            }
            if (xlx < 0) {
                done = 1;
            } if (jexp_exp_list[xlx].jxe_precedence == prec) {
                INIT_JVARVALUE(&(jvva[prec]));
                jstat = jrun_next_token(jx, pjtok); /* gets operand */
                if (!jstat) {
                    jstat = jexp_eval_primary(jx, pjtok, &(jvva[prec]));
                    if (!jstat) {
                        jstat = (jexp_exp_list[xlx].jxe_binop)(jx, jvv, &(jvva[prec]));
                    }
                }
            } else {
                xlxa[prec] = xlx;
                precdone = 1;
                prec--;
                jstat = jrun_next_token(jx, pjtok); /* gets operator */
            }
        }
    }

    return (jstat);
}
/*
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
*/
#endif
/***************************************************************/
#if 1
static int jexp_eval(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    struct jvarvalue * jvv)
{
/*
**
*/
    //  int jstat;
    //  int depth;
    //  int done;
    //  int prec;
    //  int opxlx[HIGHEST_PRECEDENCE + 1];
    //  int opprec;
    //  int isend;
    //  int xlx;
    //  JXE_BINARY_OPERATOR_FUNCTION xbinop;
    //  //JXE_UNARY_OPERATOR_FUNCTION  xunop;
    //  int pstate[HIGHEST_PRECEDENCE + 1];
    //  struct jvarvalue jvva[HIGHEST_PRECEDENCE + 1];
    //  
    //  jstat = 0;
    //  done = 0;
    //  depth = 0;
    //  isend = 0;
    //  prec = HIGHEST_PRECEDENCE;
    //  pstate[prec] = 1;
/*
**
** For each level:
**      Evaluate upper level into jvv
**      While operator matches:
**          get next token
**          Evaluate upper level into jvva[prec]
**          Do operation
**
**  pstate[prec]:
**      1 - Evaluate level prec
**      2 - Do operation at level prec
*/
    int jstat;
    int done;
    int exstate;
    int xlx;
    int prec;
    int opprec;
    int current_prec;
    struct jvarvalue jvva[HIGHEST_PRECEDENCE + 1];

#define EXSTAT_PRIMARY_OP1  1
#define EXSTAT_PRIMARY_OP2  2
#define EXSTAT_GET_OPIX     3
#define EXSTAT_CHECK_PREC   4

    /* do unary operations here */
    jstat = 0;
    done = 0;
    prec = 0;
    current_prec = 2;
    exstate = EXSTAT_PRIMARY_OP1;

    while (!done && !jstat) {
        switch (exstate) {
            case EXSTAT_PRIMARY_OP1:
                jstat = jexp_eval_primary(jx, pjtok, jvv);
                exstate = EXSTAT_GET_OPIX;
                break;
            case EXSTAT_PRIMARY_OP2:
                jstat = jexp_eval_primary(jx, pjtok, &(jvva[prec]));
                break;
            case EXSTAT_GET_OPIX:
                xlx = -1;
                jstat = jrun_next_token(jx, pjtok); /* gets operator */
                if (!jstat) {
                    if ((*pjtok)->jtok_typ == JSW_PU || (*pjtok)->jtok_typ == JSW_KW) {
                        xlx = jx->jx_aop_xref[(*pjtok)->jtok_kw];
                    }
                }
                if (xlx < 0) {
                    done = 1;
                } else {
                    opprec = jexp_exp_list[xlx].jxe_precedence;
                }
                exstate = EXSTAT_CHECK_PREC;
                break;
            case EXSTAT_CHECK_PREC:
                if (opprec == current_prec) {
                    jstat = (jexp_exp_list[xlx].jxe_binop)(jx, jvv, &(jvva[prec]));
                } else {
                }
                break;
            default:
                break;
        }
    }

/*
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
*/

#if 0
    jstat = jexp_eval_primary(jx, pjtok, jvv);

    /* pjtok has operand */
    while (!done && !jstat) {

        if (pstate[prec] == 1) {
            jstat = jrun_next_token(jx, pjtok); /* gets operator */
            xlx = -1;
            if (!jstat) {
                opprec = 0;
                if ((*pjtok)->jtok_typ == JSW_PU || (*pjtok)->jtok_typ == JSW_KW) {
                    xlx = jx->jx_aop_xref[(*pjtok)->jtok_kw];
                }
            }
            if (xlx < 0) {
                done = 1;
            } else {
                opxlx[prec] = xlx;
                pstate[prec] = 2;
            }
        } else if (pstate[prec] == 2) {
            opprec = jexp_exp_list[opxlx[prec]].jxe_precedence;
            if (opprec < prec) {
                prec--;
                pstate[prec] = 2;
            } else if (opprec == prec) {
                jstat = jrun_next_token(jx, pjtok); /* gets operand */
                if (!jstat) {
                    pstate[prec] = 3;
                }
            }
        } else if (pstate[prec] == 3) {
            jstat = jexp_eval_primary(jx, pjtok, &(jvva[prec]));
            if (!jstat) {
                pstate[prec] = 4;
            }
        } else {    /* pstate[prec] == 4 */
            xbinop = jexp_exp_list[opxlx[prec]].jxe_binop;
            jstat = xbinop(jx, jvv, &(jvva[prec]));
            pstate[prec] = 1;
        }
    }
#endif

    return (jstat);
}
/***************************************************************/
#endif
#if 0
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
#endif  /* #if USE_TABLE_EVAL */
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
    char * outstr;

    INIT_JVARVALUE(jvv);
    jstat = jexp_eval(jx, pjtok, jvv);
    if (!jstat) {
        jstat = jrun_int_tostring(jx, jvv, &outstr);
        if (!jstat) {
            printf("jexp_evaluate=<%s>\n", outstr);
            Free(outstr);
        }
    }

    return (jstat);
}
/***************************************************************/
