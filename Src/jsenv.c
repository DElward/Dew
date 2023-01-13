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
#include "error.h"
#include "jstok.h"
#include "jsexp.h"
#include "dbtreeh.h"
#include "snprfh.h"
#include "internal.h"
#include "var.h"
#include "prep.h"
#include "stmt.h"
#include "obj.h"

/***************************************************************/
/* 02/03/2022                                                  */
/*      Should compile and work correctly with following       */
/*      #defines set to either 1 or 0:                         */
/*         USE_COMMAND_TABLE            stmt.h                 */
/*         PREP_INACTIVE_EXPRESSIONS    jsexp.h                */
/*                                                             */
/*      The preferred value is:                                */
/*         USE_COMMAND_TABLE            = 3                    */
/*         PREP_INACTIVE_EXPRESSIONS    = 1                    */
/*                                                             */
/*       These #defines should be removed and the unused       */
/*       code deleted.                                         */
/***************************************************************/

#define DEBUG_TOKENS    0

// Call C dll from javascript Node.js
//      https://nodeaddons.com/calling-native-c-dlls-from-a-node-js-web-app/

/***************************************************************/
static struct jrunexec * jrun_new_jrunexec(int jxflags)
{
/*
**
*/
    struct jrunexec * jx;

    jx = New(struct jrunexec, 1);
    jx->jx_jxc.jxc_jtl      = NULL;
    jx->jx_jxc.jxc_toke_ix  = 0;
    jx->jx_flags            = jxflags;
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
#if 0
    jx->jx_njtls            = 0;
    jx->jx_xjtls            = 0;
    jx->jx_ajtls            = NULL;
#endif
    jx->jx_nop_xref         = 0;
    jx->jx_aop_xref         = NULL;
    jx->jx_head_jcx         = NULL;    /* 09/22/2021 */
    jx->jx_module_dbt       = dbtree_new(); /* 02/07/2022 */
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
    /* jvar_free_jvarrec_consts(jx->jx_globals); */
    jint_free_modules(jx);
    jvar_free_jvarrec(jx->jx_globals);

#if 0
    ii = jx->jx_njtls;
    while (ii > 0) {
        ii--;
        jtok_free_jtokenlist(jx->jx_ajtls[ii]);
    }
    Free(jx->jx_ajtls);
#endif

    jrun_clear_all_errors(jx);
    Free(jx->jx_aerrs);
    Free(jx->jx_aop_xref);

    Free(jx);
}
/***************************************************************/
int jenflags_to_jxflags(int jenflags)
{
    int jxflags;

    jxflags = 0;
    if (jenflags & JENFLAG_NODEJS) {
        jxflags |= JX_FLAG_NODEJS;
    }
    if (jenflags & JENFLAG_DEBUGFEATURES) {
        jxflags |= JX_FLAG_DEBUGFEATURES;
    }

    return (jxflags);
}
/***************************************************************/
struct jsenv * new_jsenv(int jenflags)
{
    struct jsenv * jse;

    jse = New(struct jsenv, 1);
    jse->jse_flags = jenflags;
    jse->jse_jx    = jrun_new_jrunexec(jenflags_to_jxflags(jenflags));

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

    if (jrun_get_cmd_rec_list_size(jse->jse_jx) != JSPU_ZZ_LAST - 1) {
        printf("******** WARNING: Command table is incorrect size ********\n\n");
    }

    return (jstat);
}
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
static struct jcontext * jrun_create_function_context(
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
        //jcx->jcx_flags = Realloc(jcx->jcx_flags, int, jvar->jvar_nvars);
        jcx->jcx_xjvv = jvar->jvar_nvars;
    }
    jcx->jcx_njvv = jvar->jvar_nvars;
    /* if (jcx->jcx_njvv) {                                                         */
    /*     memset(jcx->jcx_ajvv, 0, jcx->jcx_njvv * sizeof(struct jvarvalue*));     */
    /*     memset(jcx->jcx_flags, 0, jcx->jcx_njvv * sizeof(int));                  */
    /* }                                                                            */

    for (ii = 0; ii < jvar->jvar_nconsts; ii++) {
        vix = jvar->jvar_avix[ii];
        COPY_JVARVALUE(jcx->jcx_ajvv + vix, jvar->jvar_aconsts + ii);
    }

    return (jcx);
}
/***************************************************************/
int jrun_push_jfuncstate(struct jrunexec * jx,
    struct jvarvalue_function * jvvf,
    struct jvarvalue * this_jvv,
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
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_RUNTIME_STACK_OVERFLOW,
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
    jx->jx_jfs[jx->jx_njfs]->jfs_this_jvv = this_jvv;
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
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_RUNTIME_STACK_UNDERFLOW,
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
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_RUNTIME_STACK_OVERFLOW,
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
    jx->jx_js[jx->jx_njs]->jxs_finally_jstat    = 0;
    jx->jx_js[jx->jx_njs]->jxs_jti              = NULL;
    jx->jx_njs += 1;

    return (jstat);
}
/***************************************************************/
int jrun_pop_xstat(struct jrunexec * jx)
{
    int jstat = 0;
    struct jcontext * jcx;

    if (jx->jx_njs < 2) {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_RUNTIME_STACK_UNDERFLOW,
            "Runtime stack underflow on pop.");
        return (jstat);
    }
    /*
    if (jx->jx_js[jx->jx_njs - 1]->jxs_xstate == XSTAT_INCLUDE_FILE) {
        jstat = jrun_pop_include(jx);
    }
    */

    jcx = XSTAT_JCX(jx);
    if (jcx) {
        jvar_set_head_jcontext(jx, jcx->jcx_outer_jcx);
        /* jvar_free_jvarrec_consts(jcx->jcx_jvar); */
        jvar_free_jcontext(jcx);
    }

    jx->jx_njs -= 1;

    return (jstat);
}
/***************************************************************/
int jrun_pop_push_xstat(struct jrunexec * jx, int xstat)
{
    int jstat = 0;
    struct jcontext * jcx;

    if (!jx->jx_njs) {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_RUNTIME_STACK_UNDERFLOW,
            "Runtime stack underflow on pop/push.");
        return (jstat);
    }

    jcx = XSTAT_JCX(jx);
    if (jcx) {
        jvar_set_head_jcontext(jx, jcx->jcx_outer_jcx);
        /* jvar_free_jvarrec_consts(jcx->jcx_jvar); */
        jvar_free_jcontext(jcx);
    }

    jx->jx_js[jx->jx_njs - 1]->jxs_xstate           = xstat;
    jx->jx_js[jx->jx_njs - 1]->jxs_jxc.jxc_jtl      = NULL;
    jx->jx_js[jx->jx_njs - 1]->jxs_jxc.jxc_toke_ix  = 0;
    jx->jx_js[jx->jx_njs - 1]->jxs_flags            = 0;
    jx->jx_js[jx->jx_njs - 1]->jxs_jcx              = NULL;
    jx->jx_js[jx->jx_njs - 1]->jxs_finally_jstat    = 0;
    jx->jx_js[jx->jx_njs - 1]->jxs_jti              = NULL;

    return (jstat);
}
/***************************************************************/
int jrun_set_xstat(struct jrunexec * jx, int xstat)
{
/*
** 01/16/2022
*/
    int jstat = 0;

    if (!jx->jx_njs) {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_RUNTIME_STACK_UNDERFLOW,
            "Runtime stack underflow on set.");
        return (jstat);
    }

    jx->jx_js[jx->jx_njs - 1]->jxs_xstate           = xstat;

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
struct jxcursor * jrun_new_jxcursor(void)
{
/*
** 01/28/2022
*/
    struct jxcursor * jxc;

    jxc = New(struct jxcursor, 1);
    jxc->jxc_jtl     = NULL;
    jxc->jxc_toke_ix = 0;

    return (jxc);
}
/***************************************************************/
void jrun_free_jxcursor(struct jxcursor * jtifo)
{
/*
** 01/28/2022
*/
    Free(jtifo);
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

        /* try/catch */
        case XSTAT_TRY:
            jstat = jrun_set_xstat(jx, XSTAT_TRY_COMPLETE);
            break;
        case XSTAT_CATCH:
            jstat = jrun_set_xstat(jx, XSTAT_CATCH_COMPLETE);
            break;
        case XSTAT_FINALLY:
            jstat = jrun_set_xstat(jx, XSTAT_FINALLY_COMPLETE);
            break;
        case XSTAT_INACTIVE_CATCH:
            jstat = jrun_set_xstat(jx, XSTAT_CATCH_COMPLETE);
            break;

        default:
            break;
    }

    return (jstat);
}
/***************************************************************/
#ifdef UNUSED
const char * jrun_next_token_string(struct jrunexec * jx)
{
/*
** 01/11/2022
*/
    if (jx->jx_jxc.jxc_toke_ix == jx->jx_jxc.jxc_jtl->jtl_ntoks) {
        return ("*EOF*");
    } else {
        return (jx->jx_jxc.jxc_jtl->jtl_atoks[jx->jx_jxc.jxc_toke_ix]->jtok_text);
    }
}
#endif
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
#if DEBUG_TOKENS & 1 || DEBUG_TOKENS & 2
        printf("EOF\n");
#endif
    } else {
        (*pjtok) = jx->jx_jxc.jxc_jtl->jtl_atoks[jx->jx_jxc.jxc_toke_ix];
        jx->jx_jxc.jxc_toke_ix += 1;
#if DEBUG_TOKENS & 2
        if (jx->jx_flags & JX_FLAG_PREPROCESSING) {
            printf(" <%s> ", (*pjtok)->jtok_text);
        }
#endif
#if DEBUG_TOKENS & 1
        if (!(jx->jx_flags & JX_FLAG_PREPROCESSING)) {
            if ((jx)->jx_njs > 0 && XSTAT_IS_ACTIVE(jx)) printf(" <%s> ", (*pjtok)->jtok_text);
            else printf("*<%s> ", (*pjtok)->jtok_text);
        }
#endif
    }

    return (jstat);
}
/***************************************************************/
#ifdef UNUSED
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
#endif
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
#if 0
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
    struct jtokenlist * jtl;

    if (!jx->jx_njs) {
        jtl = NULL;
    } else {
        jx->jx_njtls -= 1;
        jtl = jx->jx_ajtls[jx->jx_njtls];
    }

    return (jtl);
}
#endif
/***************************************************************/
int jrun_set_strict_mode(struct jrunexec * jx)
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
static int jrun_call_global_function_by_name(struct jrunexec * jx,
    const char * funcname,
    struct jvarvalue * this_jvv,
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
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_INVALID_FUNCTION,
            "Cannot find function: %s", funcname);
    } else if (fjvv->jvv_dtype == JVV_DTYPE_FUNCTION) {
        jstat = jexp_call_function(jx, &jtok, fjvv->jvv_val.jvv_val_function, this_jvv, jvvparms, global_jcx, &rtnjvv);
    } else {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_INVALID_FUNCTION,
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
    struct jvarvalue * main_obj_jvv;

    jstat = jprep_preprocess_main(jx, &jvvf_main);
    if (!jstat) {
        jvar_init_jvarvalue(&fjvv_main);
        fjvv_main.jvv_dtype = JVV_DTYPE_FUNCTION;
        fjvv_main.jvv_val.jvv_val_function = jvvf_main;
        jstat = jvar_insert_new_global_variable(jx, jvvf_main->jvvf_func_name, &fjvv_main);
    }

    if (!jstat) jstat = jrun_push_xstat(jx, XSTAT_BEGIN);

    if (!jstat) {
        jvvparms = jrun_get_main_parms(jx);
        main_obj_jvv = jvar_new_jvarvalue();
        main_obj_jvv->jvv_dtype = JVV_DTYPE_OBJECT;
        main_obj_jvv->jvv_val.jvv_val_object = job_new_jvarvalue_object(jx);
        jstat = jrun_call_global_function_by_name(jx, jvvf_main->jvvf_func_name, main_obj_jvv, jvvparms);
        jvar_free_jvarvalue(main_obj_jvv);
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
    int tflags;
    int tcharslen;
    struct jtokenlist * jtl;

    tflags = (JTOK_CRE_FLAG_ADD_OPEN | JTOK_CRE_FLAG_ADD_CLOSE);
    jstat = jtok_create_tokenlist(jse->jse_jx, jsstr, &jtl, &tcharslen, tflags);

    if (!jstat) {
        /* jrun_push_jtl(jse->jse_jx, jtl); */
        jrun_setcursor(jse->jse_jx, jtl, 0);
        jstat = jrun_exec_jrunexec(jse->jse_jx);
    }

    /* jtl = jrun_pop_jtl(jse->jse_jx); */
    jtok_free_jtokenlist(jtl);

    return (jstat);
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
