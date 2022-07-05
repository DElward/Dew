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
    INIT_JVARVALUE(&(jvva->jvva_rtn));
    INIT_JVARVALUE(&(jvva->jvva_index));

    return (jvva);
}
/***************************************************************/
void jvar_free_jvarvalue_array(struct jvarvalue_array * jvva)
{
/*
** 09/27/2021
*/
    int ii;
    
    jvva->jvva_nRefs -= 1;
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
int jexp_parse_array_inactive(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok)
{
/*
** 09/28/2021
*/
    int jstat;

    if ((*pjtok)->jtok_kw == JSPU_CLOSE_BRACKET ) {
        jstat = jrun_next_token(jx, pjtok);
        return (jstat);
    }

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
         void              * this_obj,
         struct jvarvalue  * arg,
         struct jvarvalue ** prtnjvv)
{
/*
** 10/25/2021
*/
    int jstat = 0;
    struct jvarvalue_array * jvva = (struct jvarvalue_array *)this_obj;

    jstat = 0;

    jvva->jvva_rtn.jvv_dtype = JVV_DTYPE_JSINT;
    jvva->jvva_rtn.jvv_val.jvv_val_jsint = jvva->jvva_nvals;
    (*prtnjvv) = &(jvva->jvva_rtn);

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
         void              * this_obj,
         struct jvarvalue  * arg,
         struct jvarvalue  * jvv)
{
/*
** 10/25/2021
*/
    int jstat = 0;
    int intval;
    struct jvarvalue_array * jvva = (struct jvarvalue_array *)this_obj;

    jstat = jrun_ensure_int(jx, jvv, &intval, ENSINT_FLAG_INTERR);
    if (!jstat) {
        jstat = jint_set_array_size(jx, jvva, intval);
    }

    return (jstat);
}
/***************************************************************/
#if 0
void jint_free_Array(void * this_obj)
{
/*
** 10/28/2021
*/
    struct jvarvalue_array * jvva = (struct jvarvalue_array *)this_obj;

    jvva->jvva_nRefs -= 1;
}
#endif
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

    if (!this_ptr) {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_NULL_OBJECT,
            "Null object passed to %s()", func_name);
        return (jstat);
    }

    jvvrtn->jvv_dtype = JVV_DTYPE_DYNAMIC;
    jvvrtn->jvv_val.jvv_val_dynamic.jvvy_this_ptr  = this_ptr;
    jvvrtn->jvv_val.jvv_val_dynamic.jvvy_get_proc  = jint_get_Array_length;
    jvvrtn->jvv_val.jvv_val_dynamic.jvvy_set_proc  = jint_set_Array_length;
    jvvrtn->jvv_val.jvv_val_dynamic.jvvy_free_proc = NULL;
    jvvrtn->jvv_val.jvv_val_dynamic.jvvy_arg       = NULL;

    return (jstat);
}
/***************************************************************/
void jint_free_Array_subscript(void * this_obj)
{
/*
** 10/28/2021
*/
    struct jvarvalue_array * jvva = (struct jvarvalue_array *)this_obj;

    jvar_free_jvarvalue_data(&(jvva->jvva_rtn));
}
/***************************************************************/
int jint_get_Array_subscript(struct jrunexec  * jx,
         void              * this_obj,
         struct jvarvalue  * arg,
         struct jvarvalue ** prtnjvv)
{
/*
** 10/25/2021
*/
    int jstat = 0;
    struct jvarvalue_array * jvva = (struct jvarvalue_array *)this_obj;
    int ix;

    if (jvva->jvva_index.jvv_dtype != JVV_DTYPE_JSINT) {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_INVALID_ARRAY_SIZE,
            "Invalid array index type. Type=%s", jvar_get_dtype_name(jvva->jvva_index.jvv_dtype));
    } else {
        ix = jvva->jvva_index.jvv_val.jvv_val_jsint;
        if (ix < 0 || ix >= jvva->jvva_nvals) {
            INIT_JVARVALUE(*prtnjvv);
        } else {
            jstat = jvar_store_jvarvalue(jx, &(jvva->jvva_rtn), &(jvva->jvva_avals[ix]));
            (*prtnjvv) = &(jvva->jvva_rtn);
        }
    }

    return (jstat);
}
/***************************************************************/
int jint_set_Array_subscript(struct jrunexec  * jx,
         void              * this_obj,
         struct jvarvalue  * arg,
         struct jvarvalue  * jvv)
{
/*
** 10/25/2021
*/
    int jstat = 0;
    struct jvarvalue_array * jvva = (struct jvarvalue_array *)this_obj;
    int ix;

    if (jvva->jvva_index.jvv_dtype != JVV_DTYPE_JSINT) {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_INVALID_ARRAY_SIZE,
            "Invalid array index type. Type=%s", jvar_get_dtype_name(jvva->jvva_index.jvv_dtype));
    } else {
        ix = jvva->jvva_index.jvv_val.jvv_val_jsint;
        if (ix < 0) {
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_INVALID_ARRAY_SIZE,
                "Invalid array index. Found: %d", jvva->jvva_index.jvv_val.jvv_val_jsint);
        } else {
            if (ix >= jvva->jvva_nvals) {
                jstat = jint_set_array_size(jx, jvva, ix + 1);
            }
            jstat = jvar_store_jvarvalue(jx, &(jvva->jvva_avals[ix]), jvv);
        }
    }

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

    if (!this_ptr) {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_NULL_OBJECT,
            "Null object passed to %s()", func_name);
        return (jstat);
    }

    jstat = jrun_ensure_int(jx, jvvargs, &intval, ENSINT_FLAG_INTERR);
    if (!jstat) {
        jvvrtn->jvv_dtype = JVV_DTYPE_DYNAMIC;
        jvvrtn->jvv_val.jvv_val_dynamic.jvvy_this_ptr  = this_ptr;
        jvvrtn->jvv_val.jvv_val_dynamic.jvvy_get_proc  = jint_get_Array_subscript;
        jvvrtn->jvv_val.jvv_val_dynamic.jvvy_set_proc  = jint_set_Array_subscript;
        jvvrtn->jvv_val.jvv_val_dynamic.jvvy_free_proc = jint_free_Array_subscript;
        jvvrtn->jvv_val.jvv_val_dynamic.jvvy_arg       = NULL;
        jvva->jvva_index.jvv_dtype = JVV_DTYPE_JSINT;
        jvva->jvva_index.jvv_val.jvv_val_jsint = intval;
    }

    return (jstat);
}
/***************************************************************/
