/***************************************************************/
/*
**  jsenv.c --  JavaScript Environment/main
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
#include "internal.h"
#include "var.h"

/***************************************************************/
static struct jrunexec * jrun_new_jrunexec(void)
{
/*
**
*/
    struct jrunexec * jx;

    jx = New(struct jrunexec, 1);
    jx->jx_jxc.jxc_jtl      = NULL;
    jx->jx_jxc.jxc_toke_ix  = 0;
    jx->jx_njs              = 0;
    jx->jx_xjs              = 0;
    jx->jx_js               = NULL;
    jx->jx_nerrs            = 0;
    jx->jx_xerrs            = 0;
    jx->jx_aerrs            = NULL;
    jx->jx_nvars            = 0;
    jx->jx_xvars            = 0;
    jx->jx_avars            = NULL;
    jx->jx_njtls            = 0;
    jx->jx_xjtls            = 0;
    jx->jx_ajtls            = NULL;
    jx->jx_nop_xref         = 0;
    jx->jx_aop_xref         = NULL;

    //memset(&(jx->jx_prev_token), 0, sizeof(struct jxcursor));
#if USE_TABLE_EVAL
    jexp_init_op_xref(jx);
#endif

    return (jx);
}
/***************************************************************/
static void jrun_free_jxstate(struct jxstate * jxs)
{
    Free(jxs);
}
/***************************************************************/
void jrun_free_jerror(struct jerror * jerr)
{
    Free(jerr->jerr_msg);
    Free(jerr);
}
/***************************************************************/
static void jrun_clear_all_errors(struct jrunexec * jx)
{
    int ii;

    ii = jx->jx_nerrs;
    while (ii > 0) {
        ii--;
        jrun_free_jerror(jx->jx_aerrs[ii]);
    }
    jx->jx_nerrs = 0;
}
/***************************************************************/
static void jrun_free_jrunexec(struct jrunexec * jx)
{
    int ii;

    ii = jx->jx_xjs;
    while (ii > 0) {
        ii--;
        jrun_free_jxstate(jx->jx_js[ii]);
    }
    Free(jx->jx_js);

    ii = jx->jx_nvars;
    while (ii > 0) {
        ii--;
        jvar_free_jvarrec(jx->jx_avars[ii]);
    }
    Free(jx->jx_avars);

    ii = jx->jx_njtls;
    while (ii > 0) {
        ii--;
        jtok_free_jtokenlist(jx->jx_ajtls[ii]);
    }
    Free(jx->jx_ajtls);

    jrun_clear_all_errors(jx);
    Free(jx->jx_aerrs);
    Free(jx->jx_aop_xref);

    Free(jx);
}
/***************************************************************/
struct jsenv * new_jsenv(int jenflags)
{
    struct jsenv * jse;

    jse = New(struct jsenv, 1);
    jse->jse_flags = jenflags;
    jse->jse_jx    = jrun_new_jrunexec();

    return (jse);
}
/***************************************************************/
void free_jsenv(struct jsenv * jse)
{
    jrun_free_jrunexec(jse->jse_jx);
    Free(jse);
}
/***************************************************************/
int jenv_initialize(struct jsenv * jse)
{
    int jstat = 0;

    
    jvar_push_vars(jse->jse_jx, jvar_new_jvarrec()); /* create new variable spave */

    jstat = jenv_load_internal_classes(jse);

    return (jstat);
}
/***************************************************************/
struct jerror * jrun_new_jerror(int jstat, char * errmsgptr)
{
    struct jerror * jerr;

    jerr = New(struct jerror, 1);
    jerr->jerr_stat = jstat;
    jerr->jerr_msg  = errmsgptr;

    return (jerr);
}
/***************************************************************/
static int jrun_add_error(struct jrunexec * jx,
    int jstat, char *errmsgptr) {

    int jerrix;
    struct jerror * jerr;

    jerr = jrun_new_jerror(jstat, errmsgptr);

    if (jx->jx_nerrs == MAXIMUM_JERRORS) {
        int ii;
        jrun_free_jerror(jx->jx_aerrs[0]);
        for (ii = 0; ii < MAXIMUM_JERRORS - 1; ii++) {
            jx->jx_aerrs[ii] = jx->jx_aerrs[ii + 1];
        }
        jx->jx_nerrs -= 1;
    } else {
        if (jx->jx_nerrs == jx->jx_xerrs) {
            if (!jx->jx_xerrs) jx->jx_xerrs = 1;
            else jx->jx_xerrs *= 2;
            jx->jx_aerrs = Realloc(jx->jx_aerrs, struct jerror * , jx->jx_xerrs);
        }
    }
    jx->jx_aerrs[jx->jx_nerrs] = jerr;
    jx->jx_nerrs += 1;

    jerrix = jx->jx_nerrs + JERROR_OFFSET;

    return (jerrix);
}
/***************************************************************/
int jrun_set_error(struct jrunexec * jx, int jstat, char *fmt, ...) {

    va_list args;
    char *errmsgptr1;
    char *errmsgptr2;
    int jerrix;

    va_start (args, fmt);
    errmsgptr1 = Vsmprintf (fmt, args);
    va_end (args);
    errmsgptr2 = Smprintf("%s (JSERR %d)", errmsgptr1, jstat);
    Free(errmsgptr1);

    jerrix = jrun_add_error(jx, jstat, errmsgptr2);

    return (jerrix);
}
/***************************************************************/
int jrun_push_xstat(struct jrunexec * jx, int xstat)
{
/*
**
*/
    int jstat = 0;

    if (jx->jx_njs == jx->jx_xjs) {
        int njs = jx->jx_njs;
        if (!jx->jx_xjs) jx->jx_xjs = 16;
        else jx->jx_xjs *= 2;
        if (jx->jx_xjs >= JMAX_XSTAT_STACK) jx->jx_xjs = JMAX_XSTAT_STACK;
        if (jx->jx_njs == JMAX_XSTAT_STACK) {
            jx->jx_xjs = jx->jx_njs;  /* To prevent Free() errors */
            jstat = jrun_set_error(jx, JSERR_RUNTIME_STACK_OVERFLOW,
                "Runtime stack overflow.");
            return (jstat);
        }
        jx->jx_js = Realloc(jx->jx_js, struct jxstate*, jx->jx_xjs);
        while (njs < jx->jx_xjs) {
            jx->jx_js[njs++] = New(struct jxstate, 1);
        }
    }
    jx->jx_js[jx->jx_njs]->jxs_xstate = xstat;
    jx->jx_njs += 1;

    return (jstat);
}
/***************************************************************/
int jrun_pop_xstat(struct jrunexec * jx)
{
    int jstat = 0;

    if (jx->jx_njs < 2) {
        jstat = jrun_set_error(jx, JSERR_RUNTIME_STACK_UNDERFLOW,
            "Runtime stack underflow.");
        return (jstat);
    }
    /*
    if (jx->jx_js[jx->jx_njs - 1]->jxs_xstate == XSTAT_INCLUDE_FILE) {
        jstat = jrun_pop_include(jx);
    }
    */
    jx->jx_njs -= 1;

    return (jstat);
}
/***************************************************************/
int jrun_pop_push_xstat(struct jrunexec * jx, int xstat)
{
    int jstat = 0;

    if (!jx->jx_njs) {
        jstat = jrun_set_error(jx, JSERR_RUNTIME_STACK_UNDERFLOW,
            "Runtime stack underflow.");
        return (jstat);
    }
    jx->jx_js[jx->jx_njs]->jxs_xstate = xstat;

    return (jstat);
}
/***************************************************************/
static int jrun_update_xstat(struct jrunexec * jx)
{
    int jstat = 0;

    switch (jx->jx_js[jx->jx_njs - 1]->jxs_xstate) {
        case XSTAT_TRUE_IF:
            jstat = jrun_pop_push_xstat(jx, XSTAT_TRUE_IF_COMPLETE);
            break;
        case  XSTAT_FALSE_IF:
            jstat = jrun_pop_push_xstat(jx, XSTAT_FALSE_IF_COMPLETE);
            break;
        case  XSTAT_TRUE_ELSE:
            jstat = jrun_pop_xstat(jx);
            break;
        case  XSTAT_INACTIVE_IF:
            jstat = jrun_pop_push_xstat(jx, XSTAT_INACTIVE_IF_COMPLETE);
            break;
        case  XSTAT_FALSE_ELSE:
            jstat = jrun_pop_xstat(jx);
            break;
        case  XSTAT_INACTIVE_ELSE:
            jstat = jrun_pop_xstat(jx);
            break;

        /* while */
        case XSTAT_TRUE_WHILE:
            //  jstat = jrun_pop_xstat_with_restore(jx, XSTAT_TRUE_WHILE);
            break;
        case  XSTAT_INACTIVE_WHILE:
            jstat = jrun_pop_xstat(jx);
            break;

        /* only active else */
        case  XSTAT_TRUE_IF_COMPLETE:
            jstat = jrun_pop_push_xstat(jx, XSTAT_FALSE_ELSE);
            break;
        case  XSTAT_FALSE_IF_COMPLETE:
            jstat = jrun_pop_push_xstat(jx, XSTAT_TRUE_ELSE);
            break;
        case  XSTAT_INACTIVE_IF_COMPLETE:
            jstat = jrun_pop_push_xstat(jx, XSTAT_INACTIVE_ELSE);
            break;
        default:
            break;
    }

    return (jstat);
}
/***************************************************************/
int jrun_next_token(struct jrunexec * jx, struct jtoken ** pjtok)
{
/*
**
*/
    int jstat = 0;

    if (jx->jx_jxc.jxc_toke_ix == jx->jx_jxc.jxc_jtl->jtl_ntoks) {
        jstat = JERR_EOF;
    } else {
        (*pjtok) = jx->jx_jxc.jxc_jtl->jtl_atoks[jx->jx_jxc.jxc_toke_ix];
        jx->jx_jxc.jxc_toke_ix += 1;
    }

    return (jstat);
}
/***************************************************************/
void jrun_peek_token(struct jrunexec * jx, struct jtoken * jtok)
{
/*
**
*/
    memset(jtok, 0, sizeof(struct jtoken));
    if (jx->jx_jxc.jxc_toke_ix + 1 < jx->jx_jxc.jxc_jtl->jtl_ntoks) {
        jtok->jtok_typ   = jx->jx_jxc.jxc_jtl->jtl_atoks[jx->jx_jxc.jxc_toke_ix]->jtok_typ;
        jtok->jtok_flags = jx->jx_jxc.jxc_jtl->jtl_atoks[jx->jx_jxc.jxc_toke_ix]->jtok_flags;
        jtok->jtok_kw    = jx->jx_jxc.jxc_jtl->jtl_atoks[jx->jx_jxc.jxc_toke_ix]->jtok_kw;
    }
}
/***************************************************************/
void jrun_push_jtl(struct jrunexec * jx, struct jtokenlist * jtl)
{
/*
**
*/
    if (jx->jx_njtls == jx->jx_xjtls) {
        if (!jx->jx_xjtls) jx->jx_xjtls = 4;
        else jx->jx_xjtls *= 2;
        jx->jx_ajtls = Realloc(jx->jx_ajtls, struct jtokenlist*, jx->jx_xjtls);
    }
    jx->jx_ajtls[jx->jx_njtls] = jtl;
    jx->jx_njtls += 1;
}
/***************************************************************/
struct jtokenlist * jrun_pop_jtl(struct jrunexec * jx)
{
    int jstat = 0;
    struct jtokenlist * jtl;

    if (!jx->jx_njs) {
        jtl = NULL;
    } else {
        jx->jx_njtls -= 1;
        jtl = jx->jx_ajtls[jx->jx_njtls];
    }

    return (jtl);
}
/***************************************************************/
static int jrun_exec_implicit_stmt(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok)
{
    int jstat = 0;
    struct jvarvalue jvv;

    jstat = jexp_evaluate(jx, pjtok, &jvv);

#if 0
    struct jarfunction * jf;

    jv.jv_dtype = SVV_DTYPE_NO_VALUE;

    jstat = jare_eval(jx, NULL, jt, &jv);
    if (!jstat) {
        if (jv.jv_dtype == SVV_DTYPE_USER_FUNCTION) {
            jf = jv.jv_val.jv_jf;
            jstat = jare_set_error(jx, SVARERR_FUNCTION_CALL_NEEDS_PARENS,
                "Function call to %s requires parentheses.", jf->jf_funcname);
        }
    }
#endif
    jvar_free_jvarvalue_data(&jvv);

    return (jstat);
}
/***************************************************************/
static int jrun_exec_active_stmt(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok)
{
/*
**
*/
    int jstat = 0;

    switch ((*pjtok)->jtok_kw) {
        case JSKW_NONE:
            jstat = jrun_exec_implicit_stmt(jx, pjtok);
            break;
#if IS_DEBUG
        case JSPU_AT_AT:
            jstat = JERR_EXIT;
            break;
#endif
        case JSPU_SEMICOLON:
            jstat = jrun_next_token(jx, pjtok);
            break;
        default:
            jstat = jrun_set_error(jx, JSERR_UNSUPPORTED_STMT,
                "Unsupported statement: %s", (*pjtok)->jtok_text);
            break;
    }

    return (jstat);
}
/***************************************************************/
static int jrun_exec_inactive_stmt(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok)
{
/*
**
*/
    int jstat = 0;

    return (jstat);
}
/***************************************************************/
static int jrun_exec_jrunexec(struct jrunexec * jx)
{
/*
**
*/
    int jstat = 0;
    int is_active;
    struct jtoken * jtok;

    jstat = jrun_push_xstat(jx, XSTAT_BEGIN);
    if (!jstat) jstat = jrun_next_token(jx, &jtok);

    while (!jstat) {
        is_active = XSTAT_IS_ACTIVE(jx);
        if (jtok->jtok_kw == JSPU_OPEN_BRACE) {
            while (XSTAT_IS_COMPLETE(jx)) {
                jstat = jrun_pop_xstat(jx);
            }

            if (is_active) {
                jstat = jrun_push_xstat(jx, XSTAT_ACTIVE_BLOCK);
            } else {
                jstat = jrun_push_xstat(jx, XSTAT_INACTIVE_BLOCK);
            }
            if (!jstat) jstat = jrun_next_token(jx, &jtok);
        } else if (jtok->jtok_kw == JSPU_CLOSE_BRACE) {
            while (XSTAT_IS_COMPLETE(jx)) {
                jstat = jrun_pop_xstat(jx);
            }

            if (jx->jx_js[jx->jx_njs - 1]->jxs_xstate == XSTAT_FUNCTION) {
                jstat = jrun_push_xstat(jx, XSTAT_RETURN);
                jstat = JERR_RETURN;
            } else {
                //  if (jx->jx_js[jx->jx_njs - 1]->jxs_xstate == XSTAT_DEFINE_FUNCTION) {
                //      if (!jstat) jstat = jrun_pop_save_tokens(jx);
                //  }
                jstat = jrun_pop_xstat(jx);

                if (!jstat) jstat = jrun_update_xstat(jx);
                if (!jstat) jstat = jrun_next_token(jx, &jtok);
            }
        } else {
            if (is_active) {
                jstat = jrun_exec_active_stmt(jx, &jtok);
            } else {
                jstat = jrun_exec_inactive_stmt(jx, &jtok);
            }
        }
    }

    if (jstat == JERR_EOF) {
        if (jx->jx_njs == 1 &&
            jx->jx_js[jx->jx_njs - 1]->jxs_xstate == XSTAT_BEGIN) {
            jstat = 0;
        }
    }

    return (jstat);
}
/***************************************************************/
void jrun_setcursor(struct jrunexec * jx, struct jtokenlist * jtl, int tokix)
{
/*
**
*/
    jx->jx_jxc.jxc_jtl      = jtl;
    jx->jx_jxc.jxc_toke_ix  = tokix;
}
/***************************************************************/
int jenv_xeq_string(struct jsenv * jse, const char * jsstr)
{
/*
**
*/
    int jstat = 0;
    struct jtokenlist * jtl;

    jstat = jtok_create_tokenlist(jse->jse_jx, jsstr, &jtl);

    if (!jstat) {
        jrun_push_jtl(jse->jse_jx, jtl);
        jrun_setcursor(jse->jse_jx, jtl, 0);
        jstat = jrun_exec_jrunexec(jse->jse_jx);
    }

    jtl = jrun_pop_jtl(jse->jse_jx);
    jtok_free_jtokenlist(jtl);

    return (jstat);
}
/***************************************************************/
char * jrun_get_errmsg(struct jrunexec * jx,
    int emflags,
    int jstat,
    int *jerrnum)
{
    struct jerror * jerr;
    char *jerrmsg;

    if (!jx->jx_nerrs) {
        jerrmsg = NULL;
    } else {
        jerr = jx->jx_aerrs[jx->jx_nerrs - 1];
        jerrmsg = Strdup(jerr->jerr_msg);
    }

    return (jerrmsg);
}
/***************************************************************/
char * jenv_get_errmsg(struct jsenv * jse,
    int emflags,
    int jstat,
    int *jerrnum)
{
    return jrun_get_errmsg(jse->jse_jx, emflags, jstat, jerrnum);
}
/***************************************************************/
