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
#include "prep.h"

#define DEBUG_XSTACK            0

static int jrun_exec_inactive_stmt(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok);

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
    jx->jx_njfs             = 0;
    jx->jx_xjfs             = 0;
    jx->jx_jfs              = NULL;
    jx->jx_nerrs            = 0;
    jx->jx_xerrs            = 0;
    jx->jx_aerrs            = NULL;
    jx->jx_globals          = NULL;
    jx->jx_global_jcx       = NULL;
    jx->jx_njtls            = 0;
    jx->jx_xjtls            = 0;
    jx->jx_ajtls            = NULL;
    jx->jx_nop_xref         = 0;
    jx->jx_aop_xref         = NULL;
    jx->jx_head_jcx         = NULL;    /* 09/22/2021 */
    memset(jx->jx_type_objs, 0, sizeof(jx->jx_type_objs));

    jexp_init_op_xref(jx);

    return (jx);
}
/***************************************************************/
static void jrun_free_jxstate(struct jxstate * jxs)
{
    Free(jxs);
}
/***************************************************************/
static void jrun_free_jfuncstate(struct jfuncstate * jxfs)
{
    Free(jxfs);
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

    ii = jx->jx_xjfs;
    while (ii > 0) {
        ii--;
        jrun_free_jfuncstate(jx->jx_jfs[ii]);
    }
    Free(jx->jx_jfs);

    for (ii = 0; ii < JVV_NUM_DTYPES; ii++ ) {
        if (jx->jx_type_objs[ii]) {
            jvar_free_jvarvalue(jx->jx_type_objs[ii]);
        }
    }

    jvar_free_jcontext(jx->jx_global_jcx);
    jvar_free_jvarrec_consts(jx->jx_globals);
    jvar_free_jvarrec(jx->jx_globals);

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
    if (!jse) return;

    jrun_free_jrunexec(jse->jse_jx);
    Free(jse);
}
/***************************************************************/
int jenv_initialize(struct jsenv * jse)
{
    int jstat = 0;
    struct jvarrec * jvar;
    struct jcontext * global_jcx;
    
    jvar = jvar_new_jvarrec();
    jse->jse_jx->jx_globals = jvar;
    INCVARRECREFS(jvar);
    global_jcx = jvar_get_global_jcontext(jse->jse_jx); /* Creates jx->jx_global_jcx */

    //jvar_push_vars(jse->jse_jx, jvar); /* create new variable space */

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
#if DEBUG_XSTACK & 255
/***************************************************************/
static void printf_xstate(int xstate)
{
    char xbuf[32];

    xstack_char(xbuf, xstate);
    printf("%s", xbuf);
}
/***************************************************************/
static void jrun_print_xstack(struct jrunexec * jx)
{
    int sp;

    printf("[");
    for (sp = 1; sp < jx->jx_njs; sp++) {
        if (sp > 1) printf(",");

        printf_xstate(jx->jx_js[sp]->jxs_xstate);
    }
    printf("]\n");
}
#endif
/***************************************************************/
struct jvarvalue * jrun_get_return_jvv(struct jrunexec * jx)
{
/*
** 02/19/2021
*/
    struct jvarvalue * rtnjvv = NULL;

    if (jx->jx_njfs) {
        rtnjvv = &(jx->jx_jfs[jx->jx_njfs - 1]->jfs_rtnjvv);
    }

    return (rtnjvv);
}
/***************************************************************/
struct jcontext * jrun_create_function_context(
    struct jrunexec * jx,
    struct jvarrec * jvar,
    struct jcontext * outer_jcx)
{
/*
** 03/08/2021
*/
    struct jcontext * jcx;
    int ii;
    int vix;

    jcx = jvar_new_jcontext(jx, jvar, outer_jcx);
    if (jvar->jvar_nvars && jvar->jvar_nvars >= jcx->jcx_xjvv) {
        jcx->jcx_ajvv = Realloc(jcx->jcx_ajvv, struct jvarvalue, jvar->jvar_nvars);
        jcx->jcx_flags = Realloc(jcx->jcx_flags, int, jvar->jvar_nvars);
        jcx->jcx_xjvv = jvar->jvar_nvars;
    }
    jcx->jcx_njvv = jvar->jvar_nvars;
    /* if (jcx->jcx_njvv) {                                                         */
    /*     memset(jcx->jcx_ajvv, 0, jcx->jcx_njvv * sizeof(struct jvarvalue*));     */
    /*     memset(jcx->jcx_flags, 0, jcx->jcx_njvv * sizeof(int));                  */
    /* }                                                                            */

    for (ii = 0; ii < jvar->jvar_nconsts; ii++) {
        vix = jvar->jvar_avix[ii];
        /* jcx->jcx_ajvv[vix] = jvar->jvar_aconsts[ii]; */
        COPY_JVARVALUE(jcx->jcx_ajvv + vix, jvar->jvar_aconsts + ii);
        jcx->jcx_flags[vix] = jvar->jvar_const_jcxflags[ii];
    }

    return (jcx);
}
/***************************************************************/
int jrun_push_jfuncstate(struct jrunexec * jx,
    struct jvarvalue_function * jvvf,
    struct jcontext * outer_jcx)
{
/*
** 01/12/2021 - Called whenever a function is entered
*/
    int jstat = 0;
    struct jcontext * jcx;

#if DEBUG_FUNCTION_CALLS
    if (jvvf->jvv_dtype == JVV_DTYPE_FUNCTION) {
        printf("Function call: %s()\n", jvvf->jvv_val.jvv_val_function->jvvf_func_name);
    } else if (jvvf && jvvf->jvv_dtype == JVV_DTYPE_INTERNAL_METHOD) {
        printf("Internal method call: %s.%s()\n",
            jvvf->jvv_val.jvv_int_method.jvvim_cjvv->jvv_val.jvv_jvvi->jvvi_class_name,
            jvvf->jvv_val.jvv_int_method.jvvim_method_name);
    } else {
        printf("Function call ?\n");
    }
#endif

    if (jx->jx_njfs == jx->jx_xjfs) {
        int njfs = jx->jx_njfs;
        if (!jx->jx_xjfs) jx->jx_xjfs = 1;
        else jx->jx_xjfs *= 2;
        if (jx->jx_xjfs >= JMAX_FUNCTION_STACK) jx->jx_xjfs = JMAX_FUNCTION_STACK;
        if (jx->jx_njfs == JMAX_FUNCTION_STACK) {
            jx->jx_xjfs = jx->jx_njfs;  /* To prevent Free() errors */
            jstat = jrun_set_error(jx, JSERR_RUNTIME_STACK_OVERFLOW,
                "Runtime function stack overflow.");
            return (jstat);
        }
        jx->jx_jfs = Realloc(jx->jx_jfs, struct jfuncstate*, jx->jx_xjfs);
        while (njfs < jx->jx_xjfs) {
            jx->jx_jfs[njfs] = New(struct jfuncstate, 1);
            njfs++;
        }
    }
    if (!jx->jx_njfs) {
        jx->jx_jfs[jx->jx_njfs]->jfs_flags = 0;
    } else {
        jx->jx_jfs[jx->jx_njfs]->jfs_flags = jx->jx_jfs[jx->jx_njfs - 1]->jfs_flags;
    }
    jx->jx_jfs[jx->jx_njfs]->jfs_jvvf = jvvf;

    jcx = jrun_create_function_context(jx, jvvf->jvvf_vars, outer_jcx);
    jx->jx_jfs[jx->jx_njfs]->jfs_jcx = jcx;        
    jx->jx_jfs[jx->jx_njfs]->jfs_prev_jcx = jvar_get_head_jcontext(jx);
    jcx->jcx_jvvf = jvvf;
    jvar_set_head_jcontext(jx, jcx);
    jrun_set_current_jxc(jx, &(jvvf->jvvf_begin_jxc));

    INIT_JVARVALUE(&(jx->jx_jfs[jx->jx_njfs]->jfs_rtnjvv));
    jx->jx_njfs += 1;

    return (jstat);
}
/***************************************************************/
int jrun_pop_jfuncstate(struct jrunexec * jx)
{
    int jstat = 0;
    struct jvarvalue_function * jvvf;
    struct jcontext * jcx;
    struct jcontext * jcx_new;

#if DEBUG_FUNCTION_CALLS
    if (jx->jx_njfs >= 0) {
        jvvf = jx->jx_jfs[jx->jx_njfs - 1]->jfs_jvvf;
        if (jvvf->jvv_dtype == JVV_DTYPE_FUNCTION) {
            printf("Function exit: %s()\n", jvvf->jvv_val.jvv_val_function->jvvf_func_name);
        } else if (jvvf && jvvf->jvv_dtype == JVV_DTYPE_INTERNAL_METHOD) {
            printf("Internal method exit: %s.%s()\n",
                jvvf->jvv_val.jvv_int_method.jvvim_cjvv->jvv_val.jvv_jvvi->jvvi_class_name,
                jvvf->jvv_val.jvv_int_method.jvvim_method_name);
        } else {
            printf("Function exit ?\n");
        }
    }
#endif

    if (jx->jx_njfs <= 0) {
        jstat = jrun_set_error(jx, JSERR_RUNTIME_STACK_UNDERFLOW,
            "Runtime function stack underflow.");
        return (jstat);
    }

    jvvf = jx->jx_jfs[jx->jx_njfs - 1]->jfs_jvvf;
    if (jvvf) {
        jcx = jx->jx_jfs[jx->jx_njfs - 1]->jfs_jcx;
        jcx_new = jx->jx_jfs[jx->jx_njfs - 1]->jfs_prev_jcx;
        jvar_free_jcontext(jcx);
        jvar_set_head_jcontext(jx, jcx_new);
    }
    jx->jx_njfs -= 1;

    return (jstat);
}
/***************************************************************/
struct jvarvalue_function * jrun_get_current_function(struct jrunexec * jx)
{
    struct jvarvalue_function * jvvf;

    jvvf = NULL;

    return (jvvf);
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
    jx->jx_js[jx->jx_njs]->jxs_xstate           = xstat;
    jx->jx_js[jx->jx_njs]->jxs_jxc.jxc_jtl      = NULL;
    jx->jx_js[jx->jx_njs]->jxs_jxc.jxc_toke_ix  = 0;
    jx->jx_js[jx->jx_njs]->jxs_flags            = 0;
    jx->jx_js[jx->jx_njs]->jxs_jcx              = NULL;
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

    if (XSTAT_JCX(jx)) {
        jvar_set_head_jcontext(jx, XSTAT_JCX(jx)->jcx_outer_jcx);
        jvar_free_jcontext(XSTAT_JCX(jx));
    }

    jx->jx_njs -= 1;

    return (jstat);
}
/***************************************************************/
void jrun_init_jxcursor(struct jxcursor * tgtjxc)
{
/*
** 02/08/2021
*/
    tgtjxc->jxc_jtl     = NULL;
    tgtjxc->jxc_toke_ix = 0;
}
/***************************************************************/
void jrun_copy_current_jxc(struct jrunexec * jx, struct jxcursor * current_jxc)
{
/*
** 03/25/2021
**
** See also: jrun_get_current_jxc()
*/
    current_jxc->jxc_jtl        = jx->jx_jxc.jxc_jtl;
    current_jxc->jxc_toke_ix    = jx->jx_jxc.jxc_toke_ix;
}
/***************************************************************/
void jrun_copy_jxcursor(struct jxcursor * tgtjxc, struct jxcursor * srcjxc)
{
/*
** 02/01/2021
**
    struct jtokenlist *     jxc_jtl;
    int                     jxc_toke_ix;
*/
    tgtjxc->jxc_jtl     = srcjxc->jxc_jtl;
    tgtjxc->jxc_toke_ix = srcjxc->jxc_toke_ix;
}
/***************************************************************/
void jrun_get_current_jxc(struct jrunexec * jx, struct jxcursor * current_jxc)
{
/*
** 02/01/2021
**
** See also: jrun_copy_current_jxc()
** Copies cursor of current token to current_jxc
**
** Omar
    svare_copy_svarcursor(curr_svc, &(svx->svx_token_pc));
**
    jrun_copy_jxcursor(current_jxc, &(jx->jx_jxc));
*/
    current_jxc->jxc_jtl        = jx->jx_jxc.jxc_jtl;
    current_jxc->jxc_toke_ix    = jx->jx_jxc.jxc_toke_ix - 1;
}
/***************************************************************/
void jrun_set_current_jxc(struct jrunexec * jx, struct jxcursor * current_jxc)
{
/*
** 02/18/2021
**
** Copies current_jxc to current token
**
*/
    jx->jx_jxc.jxc_jtl        = current_jxc->jxc_jtl;
    jx->jx_jxc.jxc_toke_ix    = current_jxc->jxc_toke_ix;
}
/***************************************************************/
void jrun_set_xstat_jxc(struct jrunexec * jx, struct jxcursor * src_jxc)
{
/*
** 02/01/2021
**
** Omar
    svare_copy_svarcursor(&(svx->svx_svs[svx->svx_nsvs - 1]->svs_svc), svc);
*/
    jrun_copy_jxcursor(&(jx->jx_js[jx->jx_njs - 1]->jxs_jxc), src_jxc);
}
/***************************************************************/
int jrun_pop_xstat_with_restore(struct jrunexec * jx)
{
/*
** 02/01/2021
**
Omar
static int svare_pop_xstat_with_restore(struct svarexec * svx, int old_xstat)
{
    int svstat;

    svare_copy_svarcursor(&(svx->svx_svi->svi_pc),
                          &(svx->svx_svs[svx->svx_nsvs - 1]->svs_svc));

    svstat = svare_pop_xstat(svx, old_xstat);

    return (svstat);
}
*/
    int jstat = 0;

    jrun_copy_jxcursor(&(jx->jx_jxc), &(jx->jx_js[jx->jx_njs - 1]->jxs_jxc));

    jstat = jrun_pop_xstat(jx);

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
    jx->jx_js[jx->jx_njs - 1]->jxs_xstate           = xstat;
    jx->jx_js[jx->jx_njs - 1]->jxs_jxc.jxc_jtl      = NULL;
    jx->jx_js[jx->jx_njs - 1]->jxs_jxc.jxc_toke_ix  = 0;

    return (jstat);
}
/***************************************************************/
int jrun_update_xstat(struct jrunexec * jx)
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
            jstat = jrun_pop_xstat_with_restore(jx);
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
 
        case XSTAT_FUNCTION:
            jstat = jrun_pop_xstat_with_restore(jx);
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
        (*pjtok) = NULL;
    } else {
        (*pjtok) = jx->jx_jxc.jxc_jtl->jtl_atoks[jx->jx_jxc.jxc_toke_ix];
        jx->jx_jxc.jxc_toke_ix += 1;
    }

    return (jstat);
}
/***************************************************************/
void jrun_unget_token(struct jrunexec * jx, struct jtoken ** pjtok)
{
/*
** 02/01/2021
*/
    if (jx->jx_jxc.jxc_toke_ix >= 0) {
        jx->jx_jxc.jxc_toke_ix -= 1;
        (*pjtok) = jx->jx_jxc.jxc_jtl->jtl_atoks[jx->jx_jxc.jxc_toke_ix];
    }
}
/***************************************************************/
void jrun_peek_token(struct jrunexec * jx, struct jtoken * jtok)
{
/*
**
*/
    memset(jtok, 0, sizeof(struct jtoken));
    if (jx->jx_jxc.jxc_toke_ix + 1 < jx->jx_jxc.jxc_jtl->jtl_ntoks) {
        jtok->jtok_text  = jx->jx_jxc.jxc_jtl->jtl_atoks[jx->jx_jxc.jxc_toke_ix]->jtok_text;
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
static int jrun_set_strict_mode(struct jrunexec * jx)
{
/*
** 01/12/2021
*/
    int jstat = 0;

    if (jx->jx_njfs) {
        jx->jx_jfs[jx->jx_njfs - 1]->jfs_flags |= JFSFLAG_STRICT_MODE;
    }

    return (jstat);
}
/***************************************************************/
int jrun_get_strict_mode(struct jrunexec * jx)
{
/*
** 01/12/2021
*/
    int strict_mode = 0;

    if (jx->jx_njfs) {
        if (jx->jx_jfs[jx->jx_njfs - 1]->jfs_flags & JFSFLAG_STRICT_MODE) strict_mode = 1;
    }

    return (strict_mode);
}
/***************************************************************/
int jrun_get_stricter_mode(struct jrunexec * jx)
{
/*
** 01/20/2021 - Used for dubious things that strict allows
*/
    int stricter_mode = 0;

    if (jx->jx_njfs) {
        if (jx->jx_jfs[jx->jx_njfs - 1]->jfs_flags & JFSFLAG_STRICTER_MODE) stricter_mode = 1;
    }

    return (stricter_mode);
}
/***************************************************************/
static int jrun_process_use_string(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    int * pskip_eval)
{
/*
** 01/12/2021
*/
    int jstat = 0;
    int len;
    char * dbgstr;

    (*pskip_eval) = 0;
    if ((*pjtok)->jtok_tlen > 5 && !strncmp((*pjtok)->jtok_text + 1, "use ", 4)) {
        if ((*pjtok)->jtok_tlen == 12 && !strncmp((*pjtok)->jtok_text + 5, "strict", 6)) {
            jstat = jrun_set_strict_mode(jx);
            (*pskip_eval) = 1;
        }
#if IS_DEBUG
    } else if ((*pjtok)->jtok_tlen > 5 && !strncmp((*pjtok)->jtok_text + 1, "debug ", 5)) {
        len = IStrlen((*pjtok)->jtok_text);
        dbgstr = New(char, len - 1);
        memcpy(dbgstr, (*pjtok)->jtok_text + 1, len - 2);
        dbgstr[len - 2] = '\0';
        //dbgstr = Strdup((*pjtok)->jtok_text);
        jstat = jrun_debug_chars(jx, dbgstr);
        Free(dbgstr);
        (*pskip_eval) = 1;
#endif
    }

    return (jstat);
}
/***************************************************************/
static int jrun_exec_implicit_stmt(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok)
{
    int jstat = 0;
    int skip_eval = 0;
    struct jvarvalue jvv;

    if ((*pjtok)->jtok_typ == JSW_STRING) {
        jstat = jrun_process_use_string(jx, pjtok, &skip_eval);
    }
    if (skip_eval) {
        jstat = jrun_next_token(jx, pjtok);
    } else  {
        jstat = jexp_evaluate(jx, pjtok, &jvv);
        //jvar_free_jvarvalue_data(&jvv);   -- Removed 07/21/21
    }

    if (!jstat) {
        if ((*pjtok)->jtok_kw != JSPU_SEMICOLON) {
                    jstat = jrun_set_error(jx, JSERROBJ_SCRIPT_SYNTAX_ERROR,
                "Expecting semicolon. Found \'%s\'", (*pjtok)->jtok_text);
        }
    }

    return (jstat);
}
/***************************************************************/
static int jrun_exec_if_stmt(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok)
{
/*
** 01/17/2021
** Omar
    svstat = svare_evaluate(svx, svt, &svv);
    if (!svstat) {
        if (svv.svv_dtype != SVV_DTYPE_BOOL) {
            svstat = svare_set_error(svx, SVARERR_EXPECT_BOOLEAN,
                "Expecting boolean value for if.");
        } else {
            if (svv.svv_val.svv_val_bool) {
                svstat = svare_push_xstat(svx, XSTAT_TRUE_IF);
            } else {
                svstat = svare_push_xstat(svx, XSTAT_FALSE_IF);
            }
        }
    }
*/
    int jstat = 0;
    int istrue;
    struct jvarvalue jvv;

    if ((*pjtok)->jtok_kw != JSPU_OPEN_PAREN) {
        jstat = jrun_set_error(jx, JSERROBJ_SCRIPT_SYNTAX_ERROR,
            "Expecting open parenthesis. Found \'%s\'", (*pjtok)->jtok_text);
    } else {
        jstat = jrun_next_token(jx, pjtok);
        if (!jstat) {
            jstat = jexp_evaluate(jx, pjtok, &jvv);
            if (!jstat) {
                jstat = jexp_istrue(jx, &jvv, &istrue);
                if (!jstat) {
                    if (istrue) {
                        jstat = jrun_push_xstat(jx, XSTAT_TRUE_IF);
                    } else {
                        jstat = jrun_push_xstat(jx, XSTAT_FALSE_IF);
                    }
                    if ((*pjtok)->jtok_kw != JSPU_CLOSE_PAREN) {
                        jstat = jrun_set_error(jx, JSERROBJ_SCRIPT_SYNTAX_ERROR,
                            "Expecting close parenthesis. Found \'%s\'", (*pjtok)->jtok_text);
                    } else {
                        jstat = jrun_next_token(jx, pjtok);
                    }
                }
            }
        }
    }

    return (jstat);
}
/***************************************************************/
static int jrun_exec_inactive_if(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok)
{
/*
** 01/17/2021
** Omar
    svstat = svare_evaluate_inactive(svx, svt);
    if (!svstat) {
        svstat = svare_push_xstat(svx, XSTAT_INACTIVE_IF);
    }
*/
    int jstat = 0;

    if ((*pjtok)->jtok_kw != JSPU_OPEN_PAREN) {
        jstat = jrun_set_error(jx, JSERROBJ_SCRIPT_SYNTAX_ERROR,
            "Expecting open parenthesis. Found \'%s\'", (*pjtok)->jtok_text);
    } else {
        jstat = jrun_next_token(jx, pjtok);
        if (!jstat) {
            jstat = jexp_evaluate_inactive(jx, pjtok);
            if (!jstat) jstat = jrun_push_xstat(jx, XSTAT_INACTIVE_IF);
            if (!jstat)  {
                if ((*pjtok)->jtok_kw != JSPU_CLOSE_PAREN) {
                    jstat = jrun_set_error(jx, JSERROBJ_SCRIPT_SYNTAX_ERROR,
                        "Expecting close parenthesis. Found \'%s\'", (*pjtok)->jtok_text);
                } else {
                    jstat = jrun_next_token(jx, pjtok);
                }
            }
        }
    }

    return (jstat);
}
/***************************************************************/
static int jrun_exec_while_stmt(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok)
{
/*
** 02/01/2021
** Omar
    svstat = svare_evaluate(svx, svt, &svv);
    if (!svstat) {
        if (svv.svv_dtype != SVV_DTYPE_BOOL) {
            svstat = svare_set_error(svx, SVARERR_EXPECT_BOOLEAN,
                "Expecting boolean value for while.");
        } else {
            if (svv.svv_val.svv_val_bool) {
                svstat = svare_push_xstat(svx, XSTAT_TRUE_WHILE);
            } else {
                svstat = svare_push_xstat(svx, XSTAT_INACTIVE_WHILE);
            }
        }
    }
*/
    int jstat = 0;
    int istrue;
    struct jvarvalue jvv;

    if ((*pjtok)->jtok_kw != JSPU_OPEN_PAREN) {
        jstat = jrun_set_error(jx, JSERROBJ_SCRIPT_SYNTAX_ERROR,
            "Expecting open parenthesis. Found \'%s\'", (*pjtok)->jtok_text);
    } else {
        jstat = jrun_next_token(jx, pjtok);
        if (!jstat) {
            jstat = jexp_evaluate(jx, pjtok, &jvv);
            if (!jstat) {
                jstat = jexp_istrue(jx, &jvv, &istrue);
                if (!jstat) {
                    if (istrue) {
                        jstat = jrun_push_xstat(jx, XSTAT_TRUE_WHILE);
                    } else {
                        jstat = jrun_push_xstat(jx, XSTAT_INACTIVE_WHILE);
                    }
                    if ((*pjtok)->jtok_kw != JSPU_CLOSE_PAREN) {
                        jstat = jrun_set_error(jx, JSERROBJ_SCRIPT_SYNTAX_ERROR,
                            "Expecting close parenthesis. Found \'%s\'", (*pjtok)->jtok_text);
                    } else {
                        jstat = jrun_next_token(jx, pjtok);
                    }
                }
            }
        }
    }

    return (jstat);
}
/***************************************************************/
static int jrun_exec_inactive_while(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok)
{
/*
** 02/01/2021
** Omar
    svstat = svare_evaluate_inactive(svx, svt);
    if (svstat) svstat = svare_clear_error(svx, svstat);
    if (!svstat) {
        svstat = svare_push_xstat(svx, XSTAT_INACTIVE_WHILE);
    }
*/
    int jstat = 0;

    if ((*pjtok)->jtok_kw != JSPU_OPEN_PAREN) {
        jstat = jrun_set_error(jx, JSERROBJ_SCRIPT_SYNTAX_ERROR,
            "Expecting open parenthesis. Found \'%s\'", (*pjtok)->jtok_text);
    } else {
        jstat = jrun_next_token(jx, pjtok);
        if (!jstat) {
            jstat = jexp_evaluate_inactive(jx, pjtok);
            if (!jstat) jstat = jrun_push_xstat(jx, XSTAT_INACTIVE_WHILE);
            if (!jstat)  {
                if ((*pjtok)->jtok_kw != JSPU_CLOSE_PAREN) {
                    jstat = jrun_set_error(jx, JSERROBJ_SCRIPT_SYNTAX_ERROR,
                        "Expecting close parenthesis. Found \'%s\'", (*pjtok)->jtok_text);
                }
            }
        }
    }

    return (jstat);
}
/***************************************************************/
#if 0
// Non-hoisted var statement
static int jrun_exec_var_stmt(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok)
{
/*
** 01/03/2021
**
** Syntax: var varname1 [= value1] [, varname2 [= value2] ... [, varnameN [= valueN]]];
*/
    int jstat = 0;
    int done;
    struct jvarvalue * jvv;
    struct jtoken next_jtok;
    struct jvarvalue   vjvv;

    done = 0;
    while (!jstat && !done) {
        jstat = jvar_validate_varname(jx, (*pjtok));
        if (!jstat) {
            jvv = jvar_find_variable(jx, (*pjtok)->jtok_text);
            jrun_peek_token(jx, &next_jtok);
            if (next_jtok.jtok_kw == JSPU_EQ) {
                if (!jvv) {
                    jvv = jvar_new_jvarvalue();
                    jstat = jvar_insert_new_global_variable(jx, (*pjtok)->jtok_text, jvv);
                }
                if (!jstat) jstat = jrun_next_token(jx, pjtok); /* '=' */
                if (!jstat) jstat = jrun_next_token(jx, pjtok);
                if (!jstat) jstat = jexp_eval_value(jx, pjtok, &vjvv);
                if (!jstat) {
                    jvar_store_jvarvalue(jx, jvv, &vjvv);
                    jvar_free_jvarvalue_data(&vjvv);
                }
            } else {    /* No '=' */
                if (jvv) {
                    jstat = jrun_next_token(jx, pjtok);
                } else {
                    jvv = jvar_new_jvarvalue();
                    jstat = jvar_insert_new_global_variable(jx, (*pjtok)->jtok_text, jvv);
                    if (!jstat) jstat = jrun_next_token(jx, pjtok);
                }
            }
            if (!jstat) {
                if ((*pjtok)->jtok_kw == JSPU_SEMICOLON) {
                    jstat = jrun_next_token(jx, pjtok);
                    done = 1;
                } else if ((*pjtok)->jtok_kw == JSPU_COMMA) {
                    jstat = jrun_next_token(jx, pjtok);
                } else {
                    jstat = jrun_set_error(jx, JSERROBJ_SCRIPT_SYNTAX_ERROR,
                        "Unexpected token \'%s\'", (*pjtok)->jtok_text);
                }
            }
        }
    }

    return (jstat);
}
#endif
/***************************************************************/
static int jrun_exec_newvar_stmt(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    int jcxflags)
{
/*
** 09/21/2021 - let/const statement for new vars
**
** Syntax: let/const varname1 [= value1] [, varname2 [= value2] ... [, varnameN [= valueN]]];
*/

    int jstat = 0;
    int done;
    int vix;
    int injcxflags;
    struct jcontext * jcx;
    struct jvarrec * jvar;
    int * pvix;
    struct jfuncstate * jxfs;
    struct jvarvalue * jvv;
    struct jvarvalue * njvv;
    struct jtoken next_jtok;
    struct jvarvalue   vjvv;

    jcx = XSTAT_JCX(jx);
    if (!jcx) {
        if (XSTAT_XSTATE(jx) != XSTAT_ACTIVE_BLOCK) {
            jstat = jrun_set_error(jx, JSERR_MUST_BE_BLOCK,
                "Variable \'%s\' must be declared in block.", (*pjtok)->jtok_text);
        } else {
            jvar = jvar_new_jvarrec();
            jcx = jvar_new_jcontext(jx, jvar, jvar_get_head_jcontext(jx));
            jvar_set_head_jcontext(jx, jcx);
            XSTAT_JCX(jx) = jcx;
        }
    }

    done = 0;
    while (!jstat && !done) {
        pvix = jvar_find_in_jvarrec(jcx->jcx_jvar, (*pjtok)->jtok_text);
        if (!pvix) {
            if (XSTAT_FLAGS(jx) & JXS_FLAGS_BEGIN_FUNC) {
                /* Check if variable was also var */
                jxfs = jvar_get_current_jfuncstate(jx);
                if (jxfs) {
                    pvix = jvar_find_in_jvarrec(jxfs->jfs_jcx->jcx_jvar, (*pjtok)->jtok_text);
                    if (pvix) {
                        jstat = jrun_set_error(jx, JSERR_DUPLICATE_VARIABLE,
                            "Variable \'%s\' is duplicate of var.", (*pjtok)->jtok_text);
                    }
                }
            }
            jstat = jvar_validate_varname(jx, (*pjtok));
            if (!jstat) {
                jvv = jvar_new_jvarvalue();
                vix = jvar_insert_into_jvarrec(jcx->jcx_jvar, (*pjtok)->jtok_text);
                njvv = jvar_get_jvv_with_expand(jx, jcx, vix, &jcxflags);
            }
        } else {
            jvv = jvar_get_jvv_from_vix(jx, jcx, *pvix, &injcxflags);
        }

        if (!jstat) {
            jrun_peek_token(jx, &next_jtok);
            if (next_jtok.jtok_kw == JSPU_EQ) {
                if (!jstat) jstat = jrun_next_token(jx, pjtok); /* '=' */
                if (!jstat) jstat = jrun_next_token(jx, pjtok);
                if (!jstat) jstat = jexp_eval_value(jx, pjtok, &vjvv);
                if (!jstat) {
                    jstat = jvar_assign_jvarvalue(jx, jvv, &vjvv);
                    if (!jstat) {
                        /* jvar_set_jvvflags(jvv, jvvflags); */
                        jvar_free_jvarvalue_data(&vjvv);
                    }
                }
            } else if (jcxflags & JCX_FLAG_CONST) {
                jstat = jrun_set_error(jx, JSERR_EXPECT_EQUALS,
                    "Const statement requires value for \'%s\'", (*pjtok)->jtok_text);
            }
        }
        if (!jstat) {
            if ((*pjtok)->jtok_kw == JSPU_SEMICOLON) {
                jstat = jrun_next_token(jx, pjtok);
                done = 1;
            } else if ((*pjtok)->jtok_kw == JSPU_COMMA) {
                jstat = jrun_next_token(jx, pjtok);
            } else {
                jstat = jrun_set_error(jx, JSERROBJ_SCRIPT_SYNTAX_ERROR,
                    "Unexpected token \'%s\'", (*pjtok)->jtok_text);
            }
        }
    }

    return (jstat);
}
/***************************************************************/
static int jrun_exec_const_stmt(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok)
{
/*
** 09/23/2021 - const statement for new vars
**
** Syntax: const varname1 [= value1] [, varname2 [= value2] ... [, varnameN [= valueN]]];
*/

    int jstat = 0;

    if (!jstat) jstat = jrun_exec_newvar_stmt(jx, pjtok, JCX_FLAG_CONST);

    return (jstat);
}
/***************************************************************/
static int jrun_exec_let_stmt(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok)
{
/*
** 09/21/2021 - let statement for new vars
**
** Syntax: let varname1 [= value1] [, varname2 [= value2] ... [, varnameN [= valueN]]];
*/

    int jstat = 0;

    if (!jstat) jstat = jrun_exec_newvar_stmt(jx, pjtok, 0);

    return (jstat);
}
/***************************************************************/
static int jrun_exec_var_stmt(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok)
{
/*
** 03/02/2021 - var statement for previously hoisted vars
**
** Syntax: var varname1 [= value1] [, varname2 [= value2] ... [, varnameN [= valueN]]];
*/
    int jstat = 0;
    int done;
    int jcxflags = 0;
    struct jvarvalue * jvv;
    struct jtoken next_jtok;
    struct jvarvalue   vjvv;
    struct jcontext * var_jcx;

    done = 0;
    while (!jstat && !done) {
        jvv = jvar_find_variable(jx, (*pjtok), &var_jcx, &jcxflags);
        if (!jvv) {
            jstat = jrun_set_error(jx, JSERR_INTERNAL_ERROR,
                "Cannot find hoisted variable \'%s\'", (*pjtok)->jtok_text);
        } else {
            jrun_peek_token(jx, &next_jtok);
            if (next_jtok.jtok_kw == JSPU_EQ) {
                if (!jvv) {
                    jvv = jvar_new_jvarvalue();
                    jstat = jvar_insert_new_global_variable(jx, (*pjtok)->jtok_text, jvv, 0);
                }
                if (!jstat) jstat = jrun_next_token(jx, pjtok); /* '=' */
                if (!jstat) jstat = jrun_next_token(jx, pjtok);
                if (!jstat) jstat = jexp_eval_value(jx, pjtok, &vjvv);
                if (!jstat) {
                    jstat = jvar_assign_jvarvalue(jx, jvv, &vjvv);
                    if (!jstat) jvar_free_jvarvalue_data(&vjvv);
                }
            } else {    /* No '=' */
                if (jvv) {
                    jstat = jrun_next_token(jx, pjtok);
                } else {
                    jvv = jvar_new_jvarvalue();
                    jstat = jvar_insert_new_global_variable(jx, (*pjtok)->jtok_text, jvv, 0);
                    if (!jstat) jstat = jrun_next_token(jx, pjtok);
                }
            }
            if (!jstat) {
                if ((*pjtok)->jtok_kw == JSPU_SEMICOLON) {
                    jstat = jrun_next_token(jx, pjtok);
                    done = 1;
                } else if ((*pjtok)->jtok_kw == JSPU_COMMA) {
                    jstat = jrun_next_token(jx, pjtok);
                } else {
                    jstat = jrun_set_error(jx, JSERROBJ_SCRIPT_SYNTAX_ERROR,
                        "Unexpected token \'%s\'", (*pjtok)->jtok_text);
                }
            }
        }
    }

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
    int is_active;
    int do_update;
    struct jxcursor current_jxc;

    do_update = 1;
    if ((*pjtok)->jtok_typ == JSW_KW) do_update = !((*pjtok)->jtok_flags & JSKWI_NO_UPDATE);
    
    switch ((*pjtok)->jtok_kw) {
#if IS_DEBUG
        case JSPU_AT_AT:
            jstat = JERR_EXIT;
            break;
#endif

        case JSKW_CONST:
            jstat = jrun_next_token(jx, pjtok);
            if (!jstat) jstat = jrun_exec_const_stmt(jx, pjtok);
            break;

        case JSKW_ELSE:
/*
** Omar
        if (!XSTAT_IS_COMPLETE(svx)) {                                                    
            svstat = svare_set_error(svx, SVARERR_EXP_IF,                                 
                "else expects previous statement to be if. Found: \"%s\"", svt->svt_text);
        } else {                                                                          
            svstat = svare_update_xstat(svx);
            if (!svstat) svstat = svare_get_token(svx, svt);
        }
*/
            if (!XSTAT_IS_COMPLETE(jx)) {                                                    
                jstat = jrun_set_error(jx, JSERROBJ_SCRIPT_SYNTAX_ERROR,
                    "Unexpected token \'%s\'", (*pjtok)->jtok_text);
            } else {                                                                          
                jstat = jrun_update_xstat(jx);
                if (!jstat) jstat = jrun_next_token(jx, pjtok);
            }
            break;

        case JSKW_FUNCTION:
            jstat = jrun_next_token(jx, pjtok);
            if (!jstat) jstat = jrun_exec_function_stmt(jx, pjtok);
            break;

        case JSKW_IF:
/*
** Omar
        while (XSTAT_IS_COMPLETE(svx)) { svstat = svare_pop_xstat(svx, 0); }
        svstat = svare_update_xstat(svx);
        is_active = XSTAT_IS_ACTIVE(svx);                   
        if (!svstat) svstat = svare_get_token(svx, svt);
        if (!svstat) {
            if (!is_active) {                               
                svstat = svare_exec_inactive_if(svx, svt);  
            } else {
                svstat = svare_exec_if(svx, svt);
            }
        }

*/
            while (XSTAT_IS_COMPLETE(jx)) { jstat = jrun_pop_xstat(jx); }
            if (!jstat) jstat = jrun_update_xstat(jx);
            is_active = XSTAT_IS_ACTIVE(jx);
            if (!jstat) jstat = jrun_next_token(jx, pjtok);
            if (!jstat) {
                if (!is_active) {                               
                    jstat = jrun_exec_inactive_if(jx, pjtok);
                } else {
                    jstat = jrun_exec_if_stmt(jx, pjtok);
                }
            }
            break;

        case JSKW_LET:
            jstat = jrun_next_token(jx, pjtok);
            if (!jstat) jstat = jrun_exec_let_stmt(jx, pjtok);
            break;

        case JSKW_RETURN:
            jstat = jrun_next_token(jx, pjtok);
            if (!jstat) jstat = jrun_exec_return_stmt(jx, pjtok);
            break;

        case JSKW_WHILE:
/*
** Omar
        while (XSTAT_IS_COMPLETE(svx)) { svstat = svare_pop_xstat(svx, 0); }
        svare_get_current_svc(svx, &current_svc);
        svstat = svare_update_xstat(svx);
        if (!svstat) svstat = svare_get_token(svx, svt);
        if (!svstat) {
            svstat = svare_exec_while(svx, svt);
            if (!svstat && svx->svx_svs[svx->svx_nsvs - 1]->svs_xstate == XSTAT_TRUE_WHILE) {
                svare_set_xstat_svc(svx, &current_svc);
            }

        }
*/
            while (XSTAT_IS_COMPLETE(jx)) { jstat = jrun_pop_xstat(jx); }
            if (!jstat) jstat = jrun_update_xstat(jx);
            if (!jstat) {
                jrun_get_current_jxc(jx, &current_jxc);
                jstat = jrun_next_token(jx, pjtok);
            }
            if (!jstat) jstat = jrun_exec_while_stmt(jx, pjtok);
            if (!jstat && jx->jx_js[jx->jx_njs - 1]->jxs_xstate == XSTAT_TRUE_WHILE) {
                jrun_set_xstat_jxc(jx, &current_jxc);
            }
            break;

        case JSKW_VAR:
            jstat = jrun_next_token(jx, pjtok);
            if (!jstat) jstat = jrun_exec_var_stmt(jx, pjtok);
            break;

        case JSPU_SEMICOLON:
            jstat = jrun_next_token(jx, pjtok);
            break;

        default:
/*
** Omar
        while (XSTAT_IS_COMPLETE(svx)) { svstat = svare_pop_xstat(svx, 0); }
        is_active = XSTAT_IS_ACTIVE(svx);

        if (!is_active) {                               
            svstat = svare_exec_inactive_stmt(svx, svt);
        } else {                                        
*/
            while (XSTAT_IS_COMPLETE(jx)) { jstat = jrun_pop_xstat(jx); }
            is_active = XSTAT_IS_ACTIVE(jx);
            if (!is_active) {                               
                jstat = jrun_exec_inactive_stmt(jx, pjtok);
            } else {                                        
                jstat = jrun_exec_implicit_stmt(jx, pjtok);
                if (!jstat && do_update) jstat = jrun_update_xstat(jx);
                if (!jstat) jstat = jrun_next_token(jx, pjtok);
            }
            /* jstat = jrun_set_error(jx, JSERR_UNSUPPORTED_STMT,     */
            /*     "Unsupported statement: %s", (*pjtok)->jtok_text); */
            break;
    }

    return (jstat);
}
/***************************************************************/
static int jrun_exec_generic_inactive_stmt(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok)
{
/*
**
**
** See also: jprep_skip_generic_statement()
*/
    int jstat = 0;
    int done;
    int braces;
/*
** Omar
        int braces;

        while (XSTAT_IS_COMPLETE(svx)) {
            svstat = svare_pop_xstat(svx, 0);
        }

        done = 0;
        braces = 0;
        while (!svstat && !done) {
            svstat = svare_get_token(svx, svt);
            if (SVAR_BLOCK_BEGIN_CHAR(svt->svt_text[0])) {
                braces++;
            } else if (SVAR_BLOCK_END_CHAR(svt->svt_text[0])) {
                braces--;
                if (!braces) done = 1;
            } else if (SVAR_STMT_TERM_CHAR(svt->svt_text[0])) {
                if (!braces) done = 1;
            }
        }

        if (!svstat) {
            svstat = svare_update_xstat(svx);
        }

        if (SVAR_STMT_END_CHAR(svt->svt_text[0])) {
            svstat = svare_get_token(svx, svt);
        }
*/

    while (XSTAT_IS_COMPLETE(jx)) { jstat = jrun_pop_xstat(jx); }

    done = 0;
    braces = 0;
    while (!jstat && !done) {
        jstat = jrun_next_token(jx, pjtok);
        if ((*pjtok)->jtok_kw == JSPU_OPEN_BRACE) {
            braces++;
        } else if ((*pjtok)->jtok_kw == JSPU_CLOSE_BRACE) {
            braces--;
            if (!braces) done = 1;
        } else if ((*pjtok)->jtok_kw == JSPU_SEMICOLON) {
            //jrun_unget_token(jx, pjtok);
            if (!braces) {
                done = 1;
                jstat = jrun_update_xstat(jx);
                if (!jstat) jstat = jrun_next_token(jx, pjtok);
            }
        }
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

    switch ((*pjtok)->jtok_kw) {
#if IS_DEBUG
        case JSPU_AT_AT:
            jstat = JERR_EXIT;
            break;
#endif

        case JSKW_ELSE:
/*
** Omar
        svstat = svare_get_token(svx, svt);
        if (svx->svx_svs[svx->svx_nsvs - 1]->svs_xstate == XSTAT_FALSE_IF) {
            svstat = svare_pop_push_xstat(svx, XSTAT_FALSE_IF, XSTAT_TRUE_ELSE);
        } else if (svx->svx_svs[svx->svx_nsvs - 1]->svs_xstate == XSTAT_INACTIVE_IF) {
            svstat = svare_pop_push_xstat(svx, XSTAT_INACTIVE_IF, XSTAT_INACTIVE_ELSE);
        }
*/
            jstat = jrun_next_token(jx, pjtok);
            if (jx->jx_js[jx->jx_njs - 1]->jxs_xstate == XSTAT_FALSE_IF) {
                jstat = jrun_pop_push_xstat(jx, XSTAT_TRUE_ELSE);
            } else if (jx->jx_js[jx->jx_njs - 1]->jxs_xstate == XSTAT_INACTIVE_IF) {
                jstat = jrun_pop_push_xstat(jx, XSTAT_INACTIVE_ELSE);
            }
            break;

        case JSKW_IF:
/*
** Omar
        svstat = svare_update_xstat(svx);
        if (!svstat) svstat = svare_get_token(svx, svt);
        if (!svstat) {
            svstat = svare_exec_inactive_if(svx, svt);
        }
*/
            jstat = jrun_update_xstat(jx);
            if (!jstat) jstat = jrun_next_token(jx, pjtok);
            if (!jstat) jstat = jrun_exec_inactive_if(jx, pjtok);
            break;

        case JSKW_WHILE:
/*
** Omar
        svstat = svare_update_xstat(svx);
        if (!svstat) svstat = svare_get_token(svx, svt);
        if (!svstat) {
            svstat = svare_exec_inactive_while(svx, svt);
        }
*/
            jstat = jrun_update_xstat(jx);
            if (!jstat) jstat = jrun_next_token(jx, pjtok);
            if (!jstat) jstat = jrun_exec_inactive_while(jx, pjtok);
            break;

        case JSPU_SEMICOLON:
/*
** Omar
    } else if (!strcmp(svt->svt_text, ";")) {  
        while (XSTAT_IS_COMPLETE(svx)) {       
            svstat = svare_pop_xstat(svx, 0);  
        }                                      
        if (!svstat) {                         
            svstat = svare_update_xstat(svx);  
        }                                      
        if (!svstat) {                         
            svstat = svare_get_token(svx, svt);
        }                                      
*/
            while (XSTAT_IS_COMPLETE(jx)) { jstat = jrun_pop_xstat(jx); }
            if (!jstat) jstat = jrun_update_xstat(jx);
            if (!jstat) jstat = jrun_next_token(jx, pjtok);
            break;

        default:
            jstat = jrun_exec_generic_inactive_stmt(jx, pjtok);
            break;
    }

    return (jstat);
}
/***************************************************************/
int jrun_exec_block(struct jrunexec * jx, struct jtoken ** pjtok)
{
/*
**
*/
    int jstat = 0;
    int is_active;
    //struct jcontext * jcx;

    while (!jstat) {
        is_active = XSTAT_IS_ACTIVE(jx);
        if ((*pjtok)->jtok_kw == JSPU_OPEN_BRACE) {
#if DEBUG_XSTACK & 4
            printf("================================================================\n");
            printf("4 jrun_exec_block() %-8s toke=%-6s, stack=",
                   (is_active?"active":"inactive"), (*pjtok)->jtok_text);
            jrun_print_xstack(jx);
#endif
            while (XSTAT_IS_COMPLETE(jx)) { jstat = jrun_pop_xstat(jx); }

            if (is_active) {
                if (XSTAT_XSTATE(jx) == XSTAT_FUNCTION) {
                    jstat = jrun_push_xstat(jx, XSTAT_ACTIVE_BLOCK);
                    XSTAT_FLAGS(jx) |= JXS_FLAGS_BEGIN_FUNC;
                } else {
                    jstat = jrun_push_xstat(jx, XSTAT_ACTIVE_BLOCK);
                }
            } else {
                jstat = jrun_push_xstat(jx, XSTAT_INACTIVE_BLOCK);
            }
            if (!jstat) jstat = jrun_next_token(jx, pjtok);
        } else if ((*pjtok)->jtok_kw == JSPU_CLOSE_BRACE) {
#if DEBUG_XSTACK & 4
            printf("================================================================\n");
            printf("4 jrun_exec_block() %-8s toke=%-6s, stack=",
                   (is_active?"active":"inactive"), (*pjtok)->jtok_text);
            jrun_print_xstack(jx);
#endif
            //jcx = XSTAT_JCX(jx);
            //if (jcx) {
            //    jvar_set_head_jcontext(jx, jcx->jcx_outer_jcx);
            //    jvar_free_jcontext(jcx);
            //}

            while (XSTAT_IS_COMPLETE(jx)) { jstat = jrun_pop_xstat(jx); }

            jstat = jrun_pop_xstat(jx);

            if (XSTAT_XSTATE(jx) == XSTAT_FUNCTION) {
                //jstat = jrun_push_xstat(jx, XSTAT_RETURN);
                jstat = JERR_RETURN;
            } else {
                //  if (jx->jx_js[jx->jx_njs - 1]->jxs_xstate == XSTAT_DEFINE_FUNCTION) {
                //      if (!jstat) jstat = jrun_pop_save_tokens(jx);
                //  }

                if (!jstat) jstat = jrun_update_xstat(jx);
                if (!jstat) jstat = jrun_next_token(jx, pjtok);
            }
        } else {
#if DEBUG_XSTACK & 8
            printf("================================================================\n");
            printf("8 jrun_exec_block() %-8s toke=%-6s, stack=",
                   (is_active?"active":"inactive"), (*pjtok)->jtok_text);
            jrun_print_xstack(jx);
#endif
            if (is_active) {
                jstat = jrun_exec_active_stmt(jx, pjtok);
            } else {
                jstat = jrun_exec_inactive_stmt(jx, pjtok);
            }
        }
    }

    return (jstat);
}
/***************************************************************/
static int jrun_call_global_function_by_name(struct jrunexec * jx,
    const char * funcname,
    struct jvarvalue * jvvparms)
{
/*
** 03/01/2021
**         jstat = jexp_call_function(jx, fjvv, jvvargs, rtnjvv);
*/
    int jstat = 0;
    struct jtoken * jtok;
    struct jvarvalue * fjvv;
    struct jvarvalue  rtnjvv;
    struct jcontext * global_jcx;

    jtok = NULL;

    global_jcx = jvar_get_global_jcontext(jx);

    fjvv = jvar_find_local_variable(jx, global_jcx, funcname);
    if (!fjvv) {
        jstat = jrun_set_error(jx, JSERR_INVALID_FUNCTION,
            "Cannot find function: %s", funcname);
    } else if (fjvv->jvv_dtype == JVV_DTYPE_FUNCTION) {
        jstat = jexp_call_function(jx, &jtok, fjvv->jvv_val.jvv_val_function, NULL, jvvparms, global_jcx, &rtnjvv);
    } else {
        jstat = jrun_set_error(jx, JSERR_INVALID_FUNCTION,
            "Expecting a function. Found: %s", funcname);
    }

    return (jstat);
}
/***************************************************************/
static struct jvarvalue * jrun_get_main_parms(struct jrunexec * jx)
{
/*
** 04/03/21
*/
    struct jvarvalue * jvvparms;

    jvvparms = New(struct jvarvalue, 1);
    jvvparms->jvv_dtype = JVV_DTYPE_VALLIST;
    jvvparms->jvv_val.jvv_jvvl = New(struct jvarvalue_vallist, 1);
    jvvparms->jvv_val.jvv_jvvl->jvvl_nvals = 0;

    return (jvvparms);
}
/***************************************************************/
static int jrun_exec_jrunexec(struct jrunexec * jx)
{
/*
**
*/
    int jstat = 0;
    //  struct jtoken * jtok;
    struct jvarvalue_function * jvvf_main;
    struct jvarvalue fjvv_main;
    struct jvarvalue * jvvparms;

#if IS_DEBUG
    /* struct jtoken ** pjtok = &jtok; */
    printf("sizeof(struct jvarvalue)=%d\n", sizeof(struct jvarvalue));
#endif

    jstat = jprep_preprocess_main(jx, &jvvf_main);
    if (!jstat) {
        jvar_init_jvarvalue(&fjvv_main);
        fjvv_main.jvv_dtype = JVV_DTYPE_FUNCTION;
        fjvv_main.jvv_val.jvv_val_function = jvvf_main;
        jstat = jvar_insert_new_global_variable(jx, jvvf_main->jvvf_func_name, &fjvv_main, 0);
    }

    if (!jstat) jstat = jrun_push_xstat(jx, XSTAT_BEGIN);

    if (!jstat) {
        jvvparms = jrun_get_main_parms(jx);
        jstat = jrun_call_global_function_by_name(jx, jvvf_main->jvvf_func_name, jvvparms);
        jvar_free_jvarvalue(jvvparms);
    }

    //if (!jstat) jstat = jrun_push_jfuncstate(jx, fjvv_main);
    //
    //if (!jstat) jstat = jrun_push_xstat(jx, XSTAT_BEGIN);
    //if (!jstat) jstat = jrun_next_token(jx, &jtok);
    //
    //if (!jstat) jstat = jrun_exec_block(jx, &jtok);

    while (!jstat && XSTAT_IS_COMPLETE(jx)) { jstat = jrun_pop_xstat(jx); }

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
