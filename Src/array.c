/***************************************************************/
/*
**  array.c --  JavaScript arrays
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
#include "jstok.h"
#include "jsexp.h"
#include "var.h"
#include "array.h"
#include "prep.h"
#include "print.h"
#include "utl.h"

/***************************************************************/
#if IS_DEBUG
void jvar_inc_array_refs(struct jvarvalue_array * jvva, int val)
{
/*
** 03/08/2022
*/
    jvva->jvva_nRefs += val;
    if (jvva->jvva_nRefs < 0) {
        printf("******** jvva->jvva_nRefs < 0 ********\n");
    } else if (jvva->jvva_nRefs > 10000) {
        printf("******** jvva->jvva_nRefs > 10000 = %d ********\n", jvva->jvva_nRefs);
    }
}
#endif
/***************************************************************/
static struct jvarvalue_array * jva_new_jvarvalue_array(int nElements)
{
/*
** 09/24/2021
*/
    struct jvarvalue_array * jvva;

    jvva = New(struct jvarvalue_array, 1);
    jvva->jvva_nRefs = 1;
    jvva->jvva_nvals = nElements;
    jvva->jvva_xvals = nElements;
    jvva->jvva_avals = NULL;
    if (nElements) jvva->jvva_avals = New(struct jvarvalue, nElements);

    return (jvva);
}
/***************************************************************/
void jvar_free_jvarvalue_array(struct jvarvalue_array * jvva)
{
/*
** 09/27/2021
*/
    int ii;
    
    DECARRAYREFS(jvva);
    if (!jvva->jvva_nRefs) {
        for (ii = jvva->jvva_nvals - 1; ii >= 0; ii--) {
            jvar_free_jvarvalue_data(&(jvva->jvva_avals[ii]));
        }
        Free(jvva->jvva_avals);
        Free(jvva);
    }
}
/***************************************************************/
static int jvar_add_jvarvalue_array(
    struct jrunexec * jx,
    struct jvarvalue_array * jvva,
    struct jvarvalue * jvv)
{
/*
** 09/24/2021
*/
    int jstat = 0;
    int ii;

    if (jvva->jvva_nvals == jvva->jvva_xvals) {
        if (!jvva->jvva_xvals) jvva->jvva_xvals = 4;
        else jvva->jvva_xvals *= 2;
        jvva->jvva_avals = Realloc(jvva->jvva_avals, struct jvarvalue, jvva->jvva_xvals);
        for (ii = jvva->jvva_nvals; ii < jvva->jvva_xvals; ii++) {
            jvva->jvva_avals[ii].jvv_dtype = JVV_DTYPE_NONE;
        }
    }
    if (jvv->jvv_dtype == JVV_DTYPE_LVAL) {
        jvv = jvv->jvv_val.jvv_lval.jvvv_lval;
        jstat = jvar_store_jvarvalue(jx, &(jvva->jvva_avals[jvva->jvva_nvals]), jvv);
    } else {
        COPY_JVARVALUE(&(jvva->jvva_avals[jvva->jvva_nvals]), jvv);
    }
    jvva->jvva_nvals += 1;

    return (jstat);
}
/***************************************************************/
int jexp_parse_array(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    struct jvarvalue * jvv)
{
/*
** 09/23/2021
*/
    int jstat = 0;
    struct jvarvalue_array * jvva;
    struct jvarvalue jvv2;

    jvva = jva_new_jvarvalue_array(0);

    if ((*pjtok)->jtok_kw == JSPU_CLOSE_BRACKET ) {
        jvv->jvv_dtype = JVV_DTYPE_ARRAY;
        jvv->jvv_val.jvv_val_array = jvva;
        jstat = jrun_next_token(jx, pjtok);
        return (jstat);
    }

    jstat = jexp_eval_assignment(jx, pjtok, jvv);
    if (!jstat) jstat = jvar_add_jvarvalue_array(jx, jvva, jvv);

    while (!jstat &&
           (*pjtok)->jtok_kw == JSPU_COMMA ) {
        INIT_JVARVALUE(&jvv2);
        jstat = jrun_next_token(jx, pjtok);
        if (!jstat) {
            if ((*pjtok)->jtok_kw == JSPU_CLOSE_BRACKET &&
                !jrun_get_stricter_mode(jx)) {
                /* ok - allows: [aa , bb,] */
            } else {
                jstat = jexp_eval_assignment(jx, pjtok, &jvv2);
                if (!jstat) jstat = jvar_add_jvarvalue_array(jx, jvva, &jvv2);
            }
        }
    }

    if (!jstat) {
        if ((*pjtok)->jtok_kw == JSPU_CLOSE_BRACKET ) {
            jvv->jvv_dtype = JVV_DTYPE_ARRAY;
            jvv->jvv_val.jvv_val_array = jvva;
            jstat = jrun_next_token(jx, pjtok);
        } else {
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_EXP_CLOSE_BRACKET,
                "Expecting close bracket. Found: %s", (*pjtok)->jtok_text);
        }
    }

    return (jstat);
}
/***************************************************************/
#if 0
int jvar_store_array(
    struct jrunexec * jx,
    struct jvarvalue * jvvtgt,
    struct jvarvalue * jvvsrc)
{
/*
** 09/24/2021
*/
    int jstat = 0;
    int ix;
    struct jvarvalue * jvv;

    jvar_free_jvarvalue_data(jvvtgt);
    jvvtgt->jvv_dtype = JVV_DTYPE_ARRAY;
    jvvtgt->jvv_val.jvv_val_array =
        jva_new_jvarvalue_array(jvvsrc->jvv_val.jvv_val_array->jvva_nvals);

    for (ix = 0; ix < jvvsrc->jvv_val.jvv_val_array->jvva_nvals; ix++) {
        jvv = &(jvvsrc->jvv_val.jvv_val_array->jvva_avals[ix]);
        if (jvv->jvv_dtype == JVV_DTYPE_LVAL) {
            jvv = jvv->jvv_val.jvv_lval.jvvv_lval;
        }
        jstat = jvar_store_jvarvalue(jx, &(jvvtgt->jvv_val.jvv_val_array->jvva_avals[ix]), jvv);
    }

    return (jstat);
}
#endif
/***************************************************************/
int jprep_parse_array_inactive(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    int skip_flags)
{
/*
** 12/17/2021
**
** See also: jexp_parse_array_inactive()
*/
    int jstat;

    if ((*pjtok)->jtok_kw == JSPU_CLOSE_BRACKET ) {
        jstat = jrun_next_token(jx, pjtok);
        return (jstat);
    }

    jstat = jprep_eval_assignment(jx, pjtok, skip_flags);
    while (!jstat &&
           (*pjtok)->jtok_kw == JSPU_COMMA ) {
        jstat = jrun_next_token(jx, pjtok);
        if (!jstat) {
            jstat = jprep_eval_assignment(jx, pjtok, skip_flags);
        }
    }

    if (!jstat && (*pjtok)->jtok_kw == JSPU_CLOSE_BRACKET ) {
        jstat = jrun_next_token(jx, pjtok);
    }

    return (jstat);
}
/***************************************************************/
int jexp_compare_arrays(
    struct jvarvalue_array * jvva1,
    struct jvarvalue_array * jvva2)
{
/*
** 10/18/2021
*/
    int cmpval;

    cmpval = 0;

    return (cmpval);
}
/***************************************************************/
int jint_get_Array_length(struct jrunexec  * jx,
         struct jvarvalue_dynamic * jvvy,
         struct jvarvalue  * arg,
         struct jvarvalue ** prtnjvv)
{
/*
** 10/25/2021
*/
    int jstat = 0;
    struct jvarvalue_array * jvva;

    jvva = (struct jvarvalue_array *)jvvy->jvvy_this_ptr;
    jvvy->jvvy_rtn->jvv_dtype = JVV_DTYPE_JSINT;
    jvvy->jvvy_rtn->jvv_val.jvv_val_jsint = jvva->jvva_nvals;
    (*prtnjvv) = jvvy->jvvy_rtn;
    //DECARRAYREFS(jvva);

    return (jstat);
}
/***************************************************************/
int jint_set_array_size(struct jrunexec  * jx,
        struct jvarvalue_array * jvva,
        int new_size)
{
/*
** 10/26/2021
*/
    int jstat = 0;
    int log2;
    int nn;

    if (new_size < 0) {
        jstat = jrun_set_error(jx, errtyp_RangeError, JSERR_INVALID_ARRAY_SIZE,
            "Invalid array size. Found: %d", new_size);
    } else if (new_size < jvva->jvva_xvals) {
        if (new_size > jvva->jvva_nvals) {
            for (nn = jvva->jvva_nvals; nn < new_size; nn++) {
                jvva->jvva_avals[nn].jvv_dtype = JVV_DTYPE_NONE;
            }
        }
        jvva->jvva_nvals = new_size;
    } else {    /* new_size >= jvva->jvva_xvals */
        if (!new_size) {
            jvva->jvva_xvals = 4;
        } else {
            log2 = 0;
            nn = new_size;
            while (nn) { nn >>= 1; log2++; }
            nn = 1;
            while (log2) { nn <<= 1; log2--; }
            jvva->jvva_xvals = nn;
        }
        jvva->jvva_avals = Realloc(jvva->jvva_avals, struct jvarvalue, jvva->jvva_xvals);
        for (nn = jvva->jvva_nvals; nn < new_size; nn++) {
            jvva->jvva_avals[nn].jvv_dtype = JVV_DTYPE_NONE;
        }
        jvva->jvva_nvals = new_size;
    }

    return (jstat);
}
/***************************************************************/
int jint_set_Array_length(struct jrunexec  * jx,
         struct jvarvalue_dynamic * jvvy,
         struct jvarvalue  * arg,
         struct jvarvalue  * jvv)
{
/*
** 10/25/2021
*/
    int jstat = 0;
    int intval;
    struct jvarvalue_array * jvva;

    jvva = (struct jvarvalue_array *)jvvy->jvvy_this_ptr;
    jstat = jrun_ensure_int(jx, jvv, &intval, ENSURE_FLAG_ERROR);
    if (!jstat) {
        jstat = jint_set_array_size(jx, jvva, intval);
    }
    //DECARRAYREFS(jvva);

    return (jstat);
}
/***************************************************************/
int jint_Array_length(
        struct jrunexec  * jx,
        const char       * func_name,
        void             * this_ptr,
        struct jvarvalue * jvvlargs,
        struct jvarvalue * jvvrtn)
{
/*
** 10/21/2021
*/
    int jstat = 0;
    struct jvarvalue_dynamic * jvvy;

    if (!this_ptr) {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_NULL_OBJECT,
            "Null object passed to %s()", func_name);
        return (jstat);
    }

    jvvy = new_jvarvalue_dynamic();
    jvvy->jvvy_this_ptr  = this_ptr;
    jvvy->jvvy_get_proc  = jint_get_Array_length;
    jvvy->jvvy_set_proc  = jint_set_Array_length;
    jvvy->jvvy_free_proc = NULL;
    jvvy->jvvy_arg       = NULL;
    jvvy->jvvy_rtn = jvar_new_jvarvalue();

    jvvrtn->jvv_dtype = JVV_DTYPE_DYNAMIC;
    jvvrtn->jvv_val.jvv_val_dynamic = jvvy;

    return (jstat);
}
/***************************************************************/
void jint_free_Array_subscript(struct jvarvalue_dynamic * jvvy)
{
/*
** 10/28/2021
*/
    struct jvarvalue_array * jvva;

    jvva = (struct jvarvalue_array *)jvvy->jvvy_this_ptr;
    jvar_free_jvarvalue_array(jvva); /* var: jvva_nRefs = 3, const: jvva_nRefs = 1 */
    jvar_free_jvarvalue(jvvy->jvvy_index);
}
/***************************************************************/
int jint_get_Array_subscript(struct jrunexec  * jx,
         struct jvarvalue_dynamic * jvvy,
         struct jvarvalue  * arg,
         struct jvarvalue ** prtnjvv)
{
/*
** 10/25/2021
*/
    int jstat = 0;
    struct jvarvalue_array * jvva;
    int ix;

    jvva = (struct jvarvalue_array *)jvvy->jvvy_this_ptr;
    if (jvvy->jvvy_index->jvv_dtype != JVV_DTYPE_JSINT) {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_INVALID_ARRAY_SIZE,
            "Invalid array index type. Type=%s", jvar_get_dtype_name(jvvy->jvvy_index->jvv_dtype));
    } else {
        ix = jvvy->jvvy_index->jvv_val.jvv_val_jsint;
        if (ix < 0 || ix >= jvva->jvva_nvals) {
            INIT_JVARVALUE(*prtnjvv);
        } else {
            jstat = jvar_store_jvarvalue(jx, jvvy->jvvy_rtn, &(jvva->jvva_avals[ix]));
            (*prtnjvv) = jvvy->jvvy_rtn;
        }
    }
    //DECARRAYREFS(jvva);

    return (jstat);
}
/***************************************************************/
int jint_set_Array_subscript(struct jrunexec  * jx,
         struct jvarvalue_dynamic * jvvy,
         struct jvarvalue  * arg,
         struct jvarvalue  * jvv)
{
/*
** 10/25/2021
*/
    int jstat = 0;
    struct jvarvalue_array * jvva;
    int ix;

    jvva = (struct jvarvalue_array *)jvvy->jvvy_this_ptr;
    if (jvvy->jvvy_index->jvv_dtype != JVV_DTYPE_JSINT) {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_INVALID_ARRAY_SIZE,
            "Invalid array index type. Type=%s", jvar_get_dtype_name(jvvy->jvvy_index->jvv_dtype));
    } else {
        ix = jvvy->jvvy_index->jvv_val.jvv_val_jsint;
        if (ix < 0) {
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_INVALID_ARRAY_SIZE,
                "Invalid array index. Found: %d", ix);
        } else {
            if (ix >= jvva->jvva_nvals) {
                jstat = jint_set_array_size(jx, jvva, ix + 1);
            }
            jstat = jvar_store_jvarvalue(jx, &(jvva->jvva_avals[ix]), jvv);
            jvar_free_jvarvalue_data(jvv);  /* 02/17/2022 */
        }
    }
    //DECARRAYREFS(jvva);

    return (jstat);
}
/***************************************************************/
int jint_Array_subscript(
        struct jrunexec  * jx,
        const char       * func_name,
        void             * this_ptr,
        struct jvarvalue * jvvargs,
        struct jvarvalue * jvvrtn)
{
/*
** 10/26/2021
*/
    int jstat = 0;
    int intval;
    struct jvarvalue_array * jvva = (struct jvarvalue_array *)this_ptr;
    struct jvarvalue_dynamic * jvvy;

    if (!this_ptr) {    /* var: jvva_nRefs = 2, const: jvva_nRefs = 1 */
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_NULL_OBJECT,
            "Null object passed to %s()", func_name);
        return (jstat);
    }

    jstat = jrun_ensure_int(jx, jvvargs, &intval, ENSURE_FLAG_ERROR);
    if (!jstat) {
        INCARRAYREFS(jvva);
        jvvy = new_jvarvalue_dynamic();
        jvvy->jvvy_this_ptr  = this_ptr;
        jvvy->jvvy_get_proc  = jint_get_Array_subscript;
        jvvy->jvvy_set_proc  = jint_set_Array_subscript;
        jvvy->jvvy_free_proc = jint_free_Array_subscript;
        jvvy->jvvy_arg       = NULL;
        jvvy->jvvy_index = jvar_new_jvarvalue();
        jvvy->jvvy_index->jvv_dtype = JVV_DTYPE_JSINT;
        jvvy->jvvy_index->jvv_val.jvv_val_jsint = intval;
        jvvy->jvvy_rtn = jvar_new_jvarvalue();

        jvvrtn->jvv_dtype = JVV_DTYPE_DYNAMIC;
        jvvrtn->jvv_val.jvv_val_dynamic = jvvy;
    }

    return (jstat);
}
/***************************************************************/
int jpr_array_tostring(
    struct jrunexec * jx,
    char ** prbuf,
    int * prlen,
    int * prmax,
    struct jvarvalue_array * jvva,
    int jfmtflags)
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
    int njfmtflags;
    int jfmtorigin;
    int exfmt;
    int nvals;
    int nempty;
    char beginstr[4];
    char endstr[4];
    char commastr[4];

    exfmt      = EXTRACT_JFMT_FMT(jfmtflags);
    jfmtorigin = EXTRACT_JFMT_ORIGIN(jfmtflags);
    nvals      = jvva->jvva_nvals;
    nprint     = 0;
    njfmtflags = 0;
    commastr[0] = '\0';
    init_charbuf(&prbuf2, &prlen2, &prmax2);

#if 0
    if (exfmt == JFMT_FMT_d) {
        beginstr[0] = '\0';
        endstr[0]   = '\0';
        if (nvals == 0) {
            append_charval(prbuf, prlen, prmax, "0");
        } else if (nvals > 1) {
            nvals = 0;
            append_charval(prbuf, prlen, prmax, NAN_DISPLAY_STRING);
        }
        njfmtflags = jfmtflags | JFMT_FLAG_NESTED;
    } else if (exfmt == JFMT_FMT_f || exfmt == JFMT_FMT_i) {
        beginstr[0] = '\0';
        endstr[0]   = '\0';
        commastr[0] = '\0';
        if (nvals == 0) {
            append_charval(prbuf, prlen, prmax, NAN_DISPLAY_STRING);
        } else {
            nvals       = 1;
            njfmtflags = jfmtflags | JFMT_FLAG_NESTED;
        }
    } else if (jfmtorigin == JFMT_ORIGIN_TOSTRING) {
        beginstr[0] = '\0';
        endstr[0]   = '\0';
        strcpy(commastr, ",");
        njfmtflags = jfmtflags | JFMT_FLAG_NESTED;
    } else if (exfmt == JFMT_FMT_s && (jfmtflags & JFMT_FLAG_NESTED)) {
        strcpy(beginstr, "[");
        strcpy(endstr  , "]");
        strcpy(commastr, ",");
        nvals = 0;
        njfmtflags = JFMT_FLAG_USE_ONLY_TYPE_NAME;
    } else if (nvals == 0 && (exfmt != JFMT_FMT_o)) {
        strcpy(beginstr, "[");
        strcpy(endstr  , "]");
    } else if (jfmtorigin == JFMT_ORIGIN_FORMAT && (exfmt != JFMT_FMT_j)) {
        strcpy(beginstr, "[ ");
        strcpy(endstr  , " ]");
        strcpy(commastr, ", ");
        njfmtflags = jfmtflags | JFMT_FLAG_NESTED | JFMT_FLAG_SINGLE_QUOTES;
    } else {    /* (exfmt == JFMT_FMT_j) && nvals == 0 */
        strcpy(beginstr, "[");
        strcpy(endstr  , "]");
        strcpy(commastr, ",");
        njfmtflags = jfmtflags | JFMT_FLAG_NESTED | JFMT_FLAG_SINGLE_QUOTES;
    }
#else
    if (jfmtorigin == JFMT_ORIGIN_TOSTRING) {
        beginstr[0] = '\0';
        endstr[0]   = '\0';
        strcpy(commastr, ",");
        njfmtflags = jfmtflags | JFMT_FLAG_NESTED | JFMT_FLAG_USE_BLANK_FOR_NULL;
    } else {
        switch (exfmt) {
            case JFMT_FMT_d:
                beginstr[0] = '\0';
                endstr[0]   = '\0';
                if (nvals == 0) {
                    append_charval(prbuf, prlen, prmax, "0");
                } else if (nvals > 1) {
                    nvals = 0;
                    append_charval(prbuf, prlen, prmax, NAN_DISPLAY_STRING);
                }
                njfmtflags = jfmtflags | JFMT_FLAG_NESTED;
                break;
        
            case JFMT_FMT_f:
            case JFMT_FMT_i:
                beginstr[0] = '\0';
                endstr[0]   = '\0';
                commastr[0] = '\0';
                if (nvals == 0) {
                    append_charval(prbuf, prlen, prmax, NAN_DISPLAY_STRING);
                } else {
                    nvals       = 1;
                    njfmtflags = jfmtflags | JFMT_FLAG_NESTED;
                }
                break;
        
            case JFMT_FMT_j:
                strcpy(beginstr, "[");
                strcpy(endstr  , "]");
                strcpy(commastr, ",");
                njfmtflags = jfmtflags |
                    JFMT_FLAG_NESTED | JFMT_FLAG_SINGLE_QUOTES | JFMT_FLAG_USE_NULL_FOR_NULL;
                break;
        
            case JFMT_FMT_o:
                strcpy(beginstr, "[ ");
                strcpy(endstr  , " ]");
                strcpy(commastr, ", ");
                njfmtflags = jfmtflags | JFMT_FLAG_NESTED | JFMT_FLAG_SINGLE_QUOTES;
                break;
        
            case JFMT_FMT_O:
                if (nvals == 0) {
                    strcpy(beginstr, "[");
                    strcpy(endstr  , "]");
                } else {
                    strcpy(beginstr, "[ ");
                    strcpy(endstr  , " ]");
                    strcpy(commastr, ", ");
                    njfmtflags = jfmtflags | JFMT_FLAG_NESTED | JFMT_FLAG_SINGLE_QUOTES;
                }
                break;
        
            case JFMT_FMT_s:
                if (jfmtflags & JFMT_FLAG_NESTED) {
                    strcpy(beginstr, "[");
                    strcpy(endstr  , "]");
                    strcpy(commastr, ",");
                    nvals = 0;
                    njfmtflags = JFMT_FLAG_USE_ONLY_TYPE_NAME;
                } else if (nvals == 0) {
                    strcpy(beginstr, "[");
                    strcpy(endstr  , "]");
                } else {
                    strcpy(beginstr, "[ ");
                    strcpy(endstr  , " ]");
                    strcpy(commastr, ", ");
                    njfmtflags = jfmtflags | JFMT_FLAG_NESTED | JFMT_FLAG_SINGLE_QUOTES;
                }
                break;
        
            default:
                if (nvals == 0) {
                    strcpy(beginstr, "[");
                    strcpy(endstr  , "]");
                } else {
                    strcpy(beginstr, "[ ");
                    strcpy(endstr  , " ]");
                    strcpy(commastr, ", ");
                    njfmtflags = jfmtflags | JFMT_FLAG_NESTED | JFMT_FLAG_SINGLE_QUOTES;
                }
                break;
        }
    }
#endif

    if (jfmtorigin == JFMT_ORIGIN_LOG) {
        append_printf(prbuf, prlen, prmax, "(%d) ", jvva->jvva_nvals);
    }

    append_charval(prbuf, prlen, prmax, beginstr);

    if (njfmtflags & JFMT_FLAG_USE_ONLY_TYPE_NAME) {
        if (jfmtorigin == JFMT_ORIGIN_LOG) {
            append_printf(prbuf, prlen, prmax, "Array(%d) ", jvva->jvva_nvals);
        } else {
            append_charval(prbuf, prlen, prmax, "Array");
        }
    }

    ii = 0;
    while (ii < nvals) {
        if (nprint) append_charval(prbuf, prlen, prmax, commastr);
        nempty = 0;
        if ((exfmt != JFMT_FMT_j) && (jfmtorigin != JFMT_ORIGIN_TOSTRING)) {
            while (ii < nvals && jvva->jvva_avals[ii].jvv_dtype == JVV_DTYPE_NONE) {
                ii++;
                nempty++;
            }
        }
        if (nempty) {
            if (jfmtorigin == JFMT_ORIGIN_LOG) {
                append_charval(prbuf, prlen, prmax, EMPTY_DISPLAY_STRING);
            //} else if (exfmt == JFMT_FMT_o) {
            } else {
                if (nempty == 1) {
                    append_printf(prbuf, prlen, prmax, "<%d empty item>", nempty);
                } else {
                    append_printf(prbuf, prlen, prmax, "<%d empty items>", nempty);
                }
            }
        } else {
            jstat = jpr_jvarvalue_tostring(jx, &prbuf2, &prlen2, &prmax2,
                &(jvva->jvva_avals[ii]), njfmtflags);
            if (!jstat && prbuf2) {
                append_charval(prbuf, prlen, prmax, prbuf2);
                erase_charbuf(&prbuf2, &prlen2, &prmax2);
            }
            nprint++;
            ii++;
        }
    }

    if (exfmt == JFMT_FMT_o) {
        if (nprint) append_charval(prbuf, prlen, prmax, commastr);
        append_printf(prbuf, prlen, prmax, "[length]: %d", jvva->jvva_nvals);
    }

    append_charval(prbuf, prlen, prmax, endstr);
    Free(prbuf2);

    return (jstat);
}
/***************************************************************/
int jint_Array_toString(
        struct jrunexec  * jx,
        const char       * func_name,
        void             * this_ptr,
        struct jvarvalue * jvvlargs,
        struct jvarvalue * jvvrtn)
{
/*
** 11/10/2021
*/
    int jstat = 0;
    char * prbuf;
    int prlen;
    int prmax;
    int jfmtflags;
    struct jvarvalue_array * jvva = (struct jvarvalue_array *)this_ptr;

    prbuf = NULL;
    prlen = 0;
    prmax = 0;
    jfmtflags = JFMT_ORIGIN_TOSTRING;

    jstat = jpr_array_tostring(jx, &prbuf, &prlen, &prmax, jvva, jfmtflags);
    if (jstat) {
        jvar_store_chars_len(jvvrtn, "", 0);
    } else {
        jvar_store_chars_len(jvvrtn, prbuf, prlen);
    }
    Free(prbuf);

    return (jstat);
}
/***************************************************************/
int jint_Array_append_jvarvalue(
        struct jrunexec  * jx,
        struct jvarvalue_array * jvva,
        struct jvarvalue * jvvlargs)
{
/*
** 03/22/2022
*/
    int jstat = 0;
    int ix;
    struct jvarvalue * jvv;
    struct jvv_iterator jvvit;

    jvv = jvar_begin_iterate_jvv(0, &jvvit, jvvlargs);
    while (!jstat && jvv) {
        ix = jvva->jvva_nvals;
        jstat = jint_set_array_size(jx, jvva, ix + 1);
        if (!jstat) jstat = jvar_store_jvarvalue(jx, &(jvva->jvva_avals[ix]), jvv);
        jvv = jvar_next_iterate_jvv(&jvvit);
    }
    jvar_close_iterate_jvv_data(&jvvit);

    return (jstat);
}
/***************************************************************/
int jint_Array_push(
        struct jrunexec  * jx,
        const char       * func_name,
        void             * this_ptr,
        struct jvarvalue * jvvlargs,
        struct jvarvalue * jvvrtn)
{
/*
** 03/22/2022
*/
    int jstat = 0;
    struct jvarvalue_array * jvva = (struct jvarvalue_array *)this_ptr;

    if (!this_ptr) {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_NULL_OBJECT,
            "Null object passed to %s()", func_name);
        return (jstat);
    }

    jstat = jint_Array_append_jvarvalue(jx, jvva, jvvlargs);

    jvvrtn->jvv_dtype = JVV_DTYPE_JSINT;
    jvvrtn->jvv_val.jvv_val_jsint = jvva->jvva_nvals;

    return (jstat);
}
/***************************************************************/
