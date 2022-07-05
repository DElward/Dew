/***************************************************************/
/*
**  obj.c --  JavaScript objects
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
#include "obj.h"
#include "dbtreeh.h"

/***************************************************************/
static struct jvarvalue_object * job_new_jvarvalue_object(struct jrunexec * jx)
{
/*
** 10/12/2021
*/
    struct jvarvalue_object * jvvb;
#if IS_DEBUG
    static int next_object_sn = 0;
#endif

    jvvb = New(struct jvarvalue_object, 1);
#if IS_DEBUG
    jvvb->jvvb_sn[0] = 'B';
    jvvb->jvvb_sn[1] = (next_object_sn / 100) % 10 + '0';
    jvvb->jvvb_sn[2] = (next_object_sn /  10) % 10 + '0';
    jvvb->jvvb_sn[3] = (next_object_sn      ) % 10 + '0';
    next_object_sn++;
#endif
    jvvb->jvvb_vars   = jvar_new_jvarrec();
    jvvb->jvvb_jcx    = jvar_new_jcontext(jx, jvvb->jvvb_vars, NULL);

    return (jvvb);
}
/***************************************************************/
void job_free_jvarvalue_object(struct jvarvalue_object * jvvb)
{
    jvar_free_jvarrec(jvvb->jvvb_vars);
    jvar_free_jcontext(jvvb->jvvb_jcx);
    Free(jvvb);
}
/***************************************************************/
static int jvar_add_object_field(
    struct jrunexec * jx,
    struct jvarvalue_object * jvvb,
    char * field_name,
    struct jvarvalue * jvv)
{
/*
** 10/12/2021
*/
    int jstat = 0;
    int vix;
    int injcxflags;
    int * pvix;
    struct jvarvalue * njvv;

    pvix = jvar_find_in_jvarrec(jvvb->jvvb_vars, field_name);
    if (pvix) {
        if (jrun_get_strict_mode(jx)) {
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERROBJ_SCRIPT_SYNTAX_ERROR,
                "Duplicate field name not permitted in strict mode. Found: '%s'", field_name);
        } else {
            njvv = jvar_get_jvv_from_vix(jx, jvvb->jvvb_jcx, (*pvix), &injcxflags);
            jvar_free_jvarvalue_data(njvv);
        }
    } else {
        vix = jvar_insert_into_jvarrec(jvvb->jvvb_vars, field_name);
        njvv = jvar_get_jvv_with_expand(jx, jvvb->jvvb_jcx, vix, &injcxflags);
    }

    return (jstat);
}
/***************************************************************/
int jexp_parse_object_field(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    char ** field_name,
                    struct jvarvalue ** pjvv)
{
/*
** 10/12/2021
*/
    int jstat = 0;
    struct jvarvalue jvv2;

    (*field_name) = NULL;
    (*pjvv) = NULL;
    INIT_JVARVALUE(&jvv2);

    if ((*pjtok)->jtok_typ == JSW_STRING) {
        (*field_name) = New(char, (*pjtok)->jtok_tlen - 1);
        memcpy((*field_name), (*pjtok)->jtok_text + 1, (*pjtok)->jtok_tlen - 2);
        (*field_name)[(*pjtok)->jtok_tlen - 2] = '\0';
    } else if ((*pjtok)->jtok_typ == JSW_ID) {
        jstat = jvar_validate_varname(jx, (*pjtok));
        if (!jstat) {
            (*field_name) = New(char, (*pjtok)->jtok_tlen + 1);
            memcpy((*field_name), (*pjtok)->jtok_text, (*pjtok)->jtok_tlen);
            (*field_name)[(*pjtok)->jtok_tlen] = '\0';
        } else {
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_OBJ_TOKEN_TYPE,
                "Unexpected token type in field name: %s", (*pjtok)->jtok_text);
        }
    }

    if (!jstat) jstat = jrun_next_token(jx, pjtok);

    if (!jstat) {
        if ((*pjtok)->jtok_kw != JSPU_COLON ) {
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_OBJ_EXP_COLON,
                "Expecting colon after field name: %s", (*pjtok)->jtok_text);
        } else {
            jstat = jrun_next_token(jx, pjtok);
        }
    }

    if (!jstat) {
        jstat = jexp_eval_assignment(jx, pjtok, &jvv2);
        if (!jstat) {
            (*pjvv) = jvar_new_jvarvalue();
            COPY_JVARVALUE((*pjvv), &jvv2);
        }
    }

    return (jstat);
}
/***************************************************************/
int jexp_parse_object(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    struct jvarvalue * jvv)
{
/*
** 10/12/2021
*/
    int jstat = 0;
    struct jvarvalue_object * jvvb;
    struct jvarvalue * jvv2;
    char * field_name;

    jvvb = job_new_jvarvalue_object(jx);

    if ((*pjtok)->jtok_kw == JSPU_CLOSE_BRACKET ) {
        jvv->jvv_dtype = JVV_DTYPE_OBJECT;
        jvv->jvv_val.jvv_val_object = jvvb;
        jstat = jrun_next_token(jx, pjtok);
        return (jstat);
    }

    jstat = jexp_parse_object_field(jx, pjtok, &field_name, &jvv2);
    if (!jstat) jstat = jvar_add_object_field(jx, jvvb, field_name, jvv2);

    while (!jstat &&
           (*pjtok)->jtok_kw == JSPU_COMMA ) {
        jstat = jrun_next_token(jx, pjtok);
        if (!jstat) {
            if ((*pjtok)->jtok_kw == JSPU_CLOSE_BRACKET &&
                !jrun_get_stricter_mode(jx)) {
                /* ok - allows: { fruit: "apple", size: "medium", } */
            } else {
                jstat = jexp_parse_object_field(jx, pjtok, &field_name, &jvv2);
                if (!jstat) jstat = jvar_add_object_field(jx, jvvb, field_name, jvv2);
            }
        }
    }

    if (!jstat) {
        if ((*pjtok)->jtok_kw == JSPU_CLOSE_BRACE ) {
            jvv->jvv_dtype = JVV_DTYPE_OBJECT;
            jvv->jvv_val.jvv_val_object = jvvb;
            jstat = jrun_next_token(jx, pjtok);
        } else {
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_EXP_CLOSE_BRACE,
                "Expecting close brace. Found: %s", (*pjtok)->jtok_text);
        }
    }

    return (jstat);
}
/***************************************************************/
int jexp_parse_object_inactive(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok)
{
/*
** 10/12/2021
*/
    int jstat = 0;

    if ((*pjtok)->jtok_kw == JSPU_CLOSE_BRACE ) {
        jstat = jrun_next_token(jx, pjtok);
        return (jstat);
    }

    return (jstat);
}
/***************************************************************/
int job_dup_jvarvalue_object(
        struct jrunexec * jx,
        struct jvarvalue_object ** pjvvbtgt,
        struct jvarvalue_object * jvvbsrc)
{
/*
** 10/13/2021
*/
    int jstat = 0;
    ;
    void * vcs;
    int * pvix;
    int vixtgt;
    struct jvarvalue * jvvsrc;
    struct jvarvalue * jvvtgt;
    int injcxflags;
    int klen;
    char * field_name;

    (*pjvvbtgt) = job_new_jvarvalue_object(jx);

    vcs = dbtsi_rewind(jvvbsrc->jvvb_vars->jvar_jvarvalue_dbt, DBTREE_READ_FORWARD);
    do {
        pvix = dbtsi_next(vcs);
        if (pvix) {
            field_name = dbtsi_get_key(vcs, &klen);
            jvvsrc = jvar_get_jvv_from_vix(jx, jvvbsrc->jvvb_jcx, (*pvix), &injcxflags);
            vixtgt = jvar_insert_into_jvarrec((*pjvvbtgt)->jvvb_vars, field_name);
            jvvtgt = jvar_get_jvv_with_expand(jx, (*pjvvbtgt)->jvvb_jcx, vixtgt, &injcxflags);
            jstat = jvar_store_jvarvalue(jx, jvvtgt, jvvsrc);
        }
    } while (!jstat && pvix);
    dbtsi_close(vcs);

    return (jstat);
}
/***************************************************************/
int jvar_store_object(
    struct jrunexec * jx,
    struct jvarvalue * jvvtgt,
    struct jvarvalue * jvvsrc)
{
/*
** 10/13/2021
*/
    int jstat = 0;

    jvar_free_jvarvalue_data(jvvtgt);
    jstat = job_dup_jvarvalue_object(jx, &(jvvtgt->jvv_val.jvv_val_object), jvvsrc->jvv_val.jvv_val_object);
    if (!jstat) {
        jvvtgt->jvv_dtype = JVV_DTYPE_OBJECT;
    }

    return (jstat);
}
/***************************************************************/
