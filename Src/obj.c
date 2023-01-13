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
#include "utl.h"
#include "dbtreeh.h"
#include "prep.h"
#include "print.h"

/***************************************************************/
#if IS_DEBUG
void jvar_inc_object_refs(struct jvarvalue_object * jvvb, int val)
{
/*
** 03/08/2022
*/
    jvvb->jvvb_nRefs += val;
    if (jvvb->jvvb_nRefs < 0) {
        printf("******** jvvb->jvvb_nRefs < 0 ********\n");
    } else if (jvvb->jvvb_nRefs > 10000) {
        printf("******** jvvb->jvvb_nRefs > 10000 = %d ********\n", jvvb->jvvb_nRefs);
    }
}
#endif
/***************************************************************/
struct jvarvalue_object * job_new_jvarvalue_object(struct jrunexec * jx)
{
/*
** 10/12/2021
*/
    struct jvarvalue_object * jvvb;
#if IS_DEBUG
    static int next_object_sn = 0;
#endif

    jvvb = New(struct jvarvalue_object, 1);
    jvvb->jvvb_nRefs = 1;
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
    
    DECOBJREFS(jvvb);
    if (!jvvb->jvvb_nRefs) {
        jvar_free_jcontext(jvvb->jvvb_jcx);
        Free(jvvb);
    }
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
    int * pvix;
    struct jvarvalue * njvv;

    pvix = jvar_find_in_jvarrec(jvvb->jvvb_vars, field_name);
    if (pvix) {
        if (jrun_get_strict_mode(jx)) {
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERROBJ_SCRIPT_SYNTAX_ERROR,
                "Duplicate field name not permitted in strict mode. Found: '%s'", field_name);
        } else {
            njvv = jvar_get_jvv_from_vix(jx, jvvb->jvvb_jcx, (*pvix));
            jvar_free_jvarvalue_data(njvv);
        }
    } else {
        vix = jvar_insert_into_jvarrec(jvvb->jvvb_vars, field_name);
        njvv = jvar_get_jvv_with_expand(jx, jvvb->jvvb_jcx, vix);
        COPY_JVARVALUE(njvv, jvv);
    }

    return (jstat);
}
/***************************************************************/
int jexp_parse_object_field(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    char ** field_name,
                    int * fnlen,
                    int * fnmax,
                    struct jvarvalue * jvv)
{
/*
** 10/12/2021
                append_chars(prbuf, prlen, prmax, jvv->jvv_val.jvv_val_function->jvvf_sn, 4);

*/
    int jstat = 0;

    (*fnlen) = 0;
    INIT_JVARVALUE(jvv);

    if ((*pjtok)->jtok_typ == JSW_STRING) {
        append_chars(field_name, fnlen, fnmax, (*pjtok)->jtok_text + 1, (*pjtok)->jtok_tlen - 2);
    } else if ((*pjtok)->jtok_typ == JSW_ID) {
        jstat = jvar_validate_varname(jx, (*pjtok));
        if (!jstat) {
            append_chars(field_name, fnlen, fnmax, (*pjtok)->jtok_text, (*pjtok)->jtok_tlen);
        } else {
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_OBJ_TOKEN_TYPE,
                "Unexpected token type in field name: %s", (*pjtok)->jtok_text);
        }
    } else {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_OBJ_TOKEN_TYPE,
            "Expecting field name. Found: %s", (*pjtok)->jtok_text);
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
        jstat = jexp_eval_assignment(jx, pjtok, jvv);
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
**
** See also: jprep_parse_object_inactive()
*/
    int jstat = 0;
    struct jvarvalue_object * jvvb;
    struct jvarvalue jvv2;
    char * field_name;
    int fnlen;
    int fnmax;

    field_name = NULL;
    fnlen = 0;
    fnmax = 0;

    jvvb = job_new_jvarvalue_object(jx);

    if ((*pjtok)->jtok_kw == JSPU_CLOSE_BRACE ) {
        jvv->jvv_dtype = JVV_DTYPE_OBJECT;
        jvv->jvv_val.jvv_val_object = jvvb;
        jstat = jrun_next_token(jx, pjtok);
        return (jstat);
    }

    jstat = jexp_parse_object_field(jx, pjtok, &field_name, &fnlen, &fnmax, &jvv2);
    if (!jstat) jstat = jvar_add_object_field(jx, jvvb, field_name, &jvv2);

    while (!jstat &&
           (*pjtok)->jtok_kw == JSPU_COMMA ) {
        jstat = jrun_next_token(jx, pjtok);
        if (!jstat) {
            if ((*pjtok)->jtok_kw == JSPU_CLOSE_BRACE &&
                !jrun_get_stricter_mode(jx)) {
                /* ok - allows: { fruit: "apple", size: "medium", } */
            } else {
                jstat = jexp_parse_object_field(jx, pjtok, &field_name, &fnlen, &fnmax, &jvv2);
                if (!jstat) jstat = jvar_add_object_field(jx, jvvb, field_name, &jvv2);
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
    Free(field_name);

    return (jstat);
}
/***************************************************************/
#if ! PREP_INACTIVE_EXPRESSIONS
int jexp_parse_object_field_inactive(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok)
{
/*
** 10/12/2021
**
** See also: jexp_parse_object_field()
*/
    int jstat = 0;

    if ((*pjtok)->jtok_kw == JSPU_CLOSE_BRACE ) {
        jstat = jrun_next_token(jx, pjtok);
        return (jstat);
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
**
** See also: jexp_parse_object()
** See also: jprep_parse_object_inactive()
*/
    int jstat = 0;

    if ((*pjtok)->jtok_kw == JSPU_CLOSE_BRACE ) {
        jstat = jrun_next_token(jx, pjtok);
        return (jstat);
    }

    jstat = jexp_parse_object_field_inactive(jx, pjtok);

    while (!jstat && (*pjtok)->jtok_kw == JSPU_COMMA ) {
        jstat = jrun_next_token(jx, pjtok);
    }

    return (jstat);
}
#endif
/***************************************************************/
#ifdef UNUSED
int job_dup_jvarvalue_object(
        struct jrunexec * jx,
        struct jvarvalue_object ** pjvvbtgt,
        struct jvarvalue_object * jvvbsrc)
{
/*
** 10/13/2021
*/
    int jstat = 0;
    void * vcs;
    int * pvix;
    int vixtgt;
    struct jvarvalue * jvvsrc;
    struct jvarvalue * jvvtgt;
    int klen;
    char * field_name;

    (*pjvvbtgt) = job_new_jvarvalue_object(jx);

    vcs = dbtsi_rewind(jvvbsrc->jvvb_vars->jvar_jvarvalue_dbt, DBTREE_READ_FORWARD);
    do {
        pvix = dbtsi_next(vcs);
        if (pvix) {
            field_name = dbtsi_get_key(vcs, &klen);
            jvvsrc = jvar_get_jvv_from_vix(jx, jvvbsrc->jvvb_jcx, (*pvix));
            vixtgt = jvar_insert_into_jvarrec((*pjvvbtgt)->jvvb_vars, field_name);
            jvvtgt = jvar_get_jvv_with_expand(jx, (*pjvvbtgt)->jvvb_jcx, vixtgt);
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
#endif
/***************************************************************/
int jprep_parse_object_field_inactive(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    int skip_flags)
{
/*
** 02/02/2022
*/
    int jstat = 0;

    if ((*pjtok)->jtok_typ == JSW_STRING) {
        /* ok */
    } else if ((*pjtok)->jtok_typ == JSW_ID) {
        /* ok */
    } else {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_OBJ_TOKEN_TYPE,
            "Expecting field name. Found: %s", (*pjtok)->jtok_text);
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
        jstat = jprep_eval_assignment(jx, pjtok, skip_flags);
    }

    return (jstat);
}
/***************************************************************/
int jprep_parse_object_inactive(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    int skip_flags)
{
/*
** 12/17/2021
*/
    int jstat = 0;

    if ((*pjtok)->jtok_kw == JSPU_CLOSE_BRACE ) {
        jstat = jrun_next_token(jx, pjtok);
        return (jstat);
    }

    jstat = jprep_parse_object_field_inactive(jx, pjtok, skip_flags);

    while (!jstat &&
           (*pjtok)->jtok_kw == JSPU_COMMA ) {
        jstat = jrun_next_token(jx, pjtok);
        if (!jstat) {
            if ((*pjtok)->jtok_kw == JSPU_CLOSE_BRACE &&
                !jrun_get_stricter_mode(jx)) {
                /* ok - allows: { fruit: "apple", size: "medium", } */
            } else {
                jstat = jprep_parse_object_field_inactive(jx, pjtok, skip_flags);
            }
        }
    }

    if (!jstat) {
        if ((*pjtok)->jtok_kw == JSPU_CLOSE_BRACE ) {
            jstat = jrun_next_token(jx, pjtok);
        } else {
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_EXP_CLOSE_BRACE,
                "Expecting close brace. Found: %s", (*pjtok)->jtok_text);
        }
    }

    return (jstat);
}
/***************************************************************/
int jpr_object_tostring(
    struct jrunexec * jx,
    char ** prbuf,
    int * prlen,
    int * prmax,
    struct jvarvalue_object * jvvb,
    int jfmtflags)
{
/*
** 03/02/2022
*/
    int jstat = 0;
    void * vcs;
    char * prbuf2;
    int prlen2;
    int prmax2;
    int * pvix;
    int jfmtorigin;
    int exfmt;
    int nprint;
    int njfmtflags;
    int klen;
    char * field_name;
    struct jvarvalue * jvvsrc;
    char beginstr[4];
    char endstr[4];
    char commastr[4];
    char colonstr[4];

    exfmt      = EXTRACT_JFMT_FMT(jfmtflags);
    jfmtorigin = EXTRACT_JFMT_ORIGIN(jfmtflags);
    nprint     = 0;
    init_charbuf(&prbuf2, &prlen2, &prmax2);

    switch (exfmt) {
        case JFMT_FMT_j:
            strcpy(beginstr, "{");
            strcpy(endstr  , "}");
            strcpy(commastr, ",");
            strcpy(colonstr, ":");
            njfmtflags = jfmtflags | JFMT_FLAG_NESTED | JFMT_FLAG_DOUBLE_QUOTES;
            break;
        
        default:
            strcpy(beginstr, "{");
            strcpy(endstr  , "}");
            strcpy(commastr, ", ");
            strcpy(colonstr, ": ");
            njfmtflags = jfmtflags | JFMT_FLAG_NESTED | JFMT_FLAG_SINGLE_QUOTES;
            break;
    }

    append_charval(prbuf, prlen, prmax, beginstr);

    vcs = dbtsi_rewind(jvvb->jvvb_vars->jvar_jvarvalue_dbt, DBTREE_READ_FORWARD);
    do {
        pvix = dbtsi_next(vcs);
        if (pvix) {
            if (nprint) append_charval(prbuf, prlen, prmax, commastr);
            field_name = dbtsi_get_key(vcs, &klen);
            if (field_name) {
                if (exfmt == JFMT_FMT_j) {
                    append_printf(prbuf, prlen, prmax, "\"%s\"", field_name);
                } else {
                    append_charval(prbuf, prlen, prmax, field_name);
                }
            }
            append_charval(prbuf, prlen, prmax, colonstr);

            jvvsrc = jvar_get_jvv_from_vix(jx, jvvb->jvvb_jcx, (*pvix));

            jstat = jpr_jvarvalue_tostring(jx, &prbuf2, &prlen2, &prmax2, jvvsrc, njfmtflags);
            if (!jstat && prbuf2) {
                append_charval(prbuf, prlen, prmax, prbuf2);
                erase_charbuf(&prbuf2, &prlen2, &prmax2);
            }
            nprint++;
        }
    } while (!jstat && pvix);
    dbtsi_close(vcs);

    append_charval(prbuf, prlen, prmax, endstr);
    Free(prbuf2);

    return (jstat);
}
/***************************************************************/
int jvar_insert_new_object_member(struct jrunexec * jx,
    const char * varname,
    struct jcontext * jcx,
    struct jvarvalue ** newjvv)
{
/*
** 09/19/2022
*/
    int jstat = 0;
    int vix;
    struct jvarvalue * tgt_jvv;

    tgt_jvv = NULL;
    vix = jvar_insert_into_jvarrec(jcx->jcx_jvar, varname);
    if (vix >= 0) tgt_jvv = jvar_get_jvv_with_expand(jx, jcx, vix);
#if IS_DEBUG
    if (!tgt_jvv) {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_INTERNAL_ERROR,
            "Internal error: Cannot find new variable: %s", varname);
        return (jstat);
    }
#endif
    (*newjvv) = tgt_jvv;

    return (jstat);
}
/***************************************************************/
int jexp_binop_dot_object(struct jrunexec * jx,
    struct jvarvalue * jvvobject,
    struct jvarvalue * jvvparent,
    struct jvarvalue * jvvfield)
{
/*
** 03/03/2022
*/
    int jstat = 0;
    int * pvix;
    struct jvarvalue * jvvsrc;
    struct jvarvalue_object * jvvb = jvvparent->jvv_val.jvv_val_object;

#if IS_DEBUG
    /* jvv_dtype should be checked before getting here. */
    if (jvvfield->jvv_dtype != JVV_DTYPE_TOKEN) {
        jstat = jrun_set_error(jx, errtyp_InternalError, JSERR_INTERNAL_ERROR,
            "Internal error: Expecting identifier after dot.");
        return (jstat);
    }
#endif

    pvix = jvar_find_in_jvarrec(jvvb->jvvb_vars, jvvfield->jvv_val.jvv_val_token.jvvt_token);
    if (!pvix) {
        jstat = jvar_insert_new_object_member(jx, jvvfield->jvv_val.jvv_val_token.jvvt_token, jvvb->jvvb_jcx, &jvvsrc);
    } else {
        jvvsrc = jvar_get_jvv_from_vix(jx, jvvb->jvvb_jcx, *pvix);
    }
    
    if (!jvvsrc) {
        if (!jstat) {
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_INTERNAL_ERROR,
                "Object member '%s' is null.", 
                jvvfield->jvv_val.jvv_val_token.jvvt_token);
        }
    } else {
        INCOBJREFS(jvvb);
        /* jvar_free_jvarvalue_data(jvvobject);  -- 09/26/2022 -- Moved to jvar_store_lval() */
        jvar_store_lval(jx, jvvobject, jvvsrc, NULL);
        if (jvvobject->jvv_dtype == JVV_DTYPE_LVAL) {
            jvvobject->jvv_val.jvv_lval.jvvv_parent = jvar_new_jvarvalue();
            jvvobject->jvv_val.jvv_lval.jvvv_parent->jvv_dtype = JVV_DTYPE_OBJECT;
            jvvobject->jvv_val.jvv_lval.jvvv_parent->jvv_val.jvv_val_object = jvvb;
            if (jvvobject->jvv_val.jvv_lval.jvvv_lval->jvv_dtype == JVV_DTYPE_FUNCVAR) {
                INCOBJREFS(jvvb);
                jvvobject->jvv_val.jvv_lval.jvvv_lval->jvv_val.jvv_val_funcvar.jvvfv_this_obj = jvvb;
            }
        }
    }

    return (jstat);
}
/***************************************************************/
int jexp_binop_dot_prototype(struct jrunexec * jx,
    struct jvarvalue * jvvobject,
    struct jprototype * jpt,
    struct jvarvalue * jvvfield)
{
/*
** 10/04/2022
*/
    int jstat = 0;
    int * pvix;
    struct jvarvalue * jvvsrc;

#if IS_DEBUG
    /* jvv_dtype should be checked before getting here. */
    if (jvvfield->jvv_dtype != JVV_DTYPE_TOKEN) {
        jstat = jrun_set_error(jx, errtyp_InternalError, JSERR_INTERNAL_ERROR,
            "Internal error: Expecting identifier after dot.");
        return (jstat);
    }
#endif

    pvix = jvar_find_in_jvarrec(jpt->jpt_jvar, jvvfield->jvv_val.jvv_val_token.jvvt_token);
    if (!pvix) {
        jstat = jvar_insert_new_object_member(jx, jvvfield->jvv_val.jvv_val_token.jvvt_token, jpt->jpt_jcx, &jvvsrc);
    } else {
        jvvsrc = jvar_get_jvv_from_vix(jx, jpt->jpt_jcx, *pvix);
    }
    
    if (!jvvsrc) {
        if (!jstat) {
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_INTERNAL_ERROR,
                "Object member '%s' is null.", 
                jvvfield->jvv_val.jvv_val_token.jvvt_token);
        }
    } else {
        jvar_store_lval(jx, jvvobject, jvvsrc, NULL);
        if (jvvobject->jvv_dtype == JVV_DTYPE_LVAL) {
            jvvobject->jvv_val.jvv_lval.jvvv_parent = jvar_new_jvarvalue();
            jvvobject->jvv_val.jvv_lval.jvvv_parent->jvv_dtype = JVV_DTYPE_PROTOTYPE;
            jvvobject->jvv_val.jvv_lval.jvvv_parent->jvv_val.jvv_prototype.jvpt_jpt = jpt;
        }
    }

    return (jstat);
}
/***************************************************************/
