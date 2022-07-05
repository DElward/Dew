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
#include "jsenv.h"
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

    for (ii = jvva->jvva_nvals - 1; ii >= 0; ii--) {
        jvar_free_jvarvalue_data(&(jvva->jvva_avals[ii]));
    }
    Free(jvva->jvva_avals);
    Free(jvva);
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
            jstat = jrun_set_error(jx, JSERR_EXP_CLOSE_BRACKET,
                "Expecting close bracket. Found: %s", (*pjtok)->jtok_text);
        }
    }

    return (jstat);
}
/***************************************************************/
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
