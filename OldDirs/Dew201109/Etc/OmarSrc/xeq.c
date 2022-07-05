/*
**   xeq.c      -- XEQ processing
*/
/***************************************************************/
#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>
/***************************************************************/
#include "util.h"
#include "omar.h"
#include "dbtreeh.h"
#include "snprfh.h"
#include "json.h"
#include "onn.h"
#include "xeq.h"
#include "bid.h"
#include "handrec.h"
#include "crehands.h"
#include "intclas.h"
#include "thread.h"

#define DEBUG_XSTACK 0

#define MAX_TOKEN_STACK     32  /* 11/06/2019 Changed from 16 */

/***************************************************************/
static int svare_exec_active_stmt(struct svarexec * svx, struct svartoken * svt);
static int svare_exec_inactive_stmt(struct svarexec * svx, struct svartoken * svt);
static int svare_eval(struct svarexec * svx,
                        struct svarvalue * csvv,
                        struct svartoken * svt,
                        struct svarvalue * svv);
static int svare_inactive_binop(struct svarexec * svx, struct svartoken * svt);
static int svare_inactive_and(struct svarexec * svx, struct svartoken * svt);
static int svare_inactive_or(struct svarexec * svx, struct svartoken * svt);
static int svare_inactive_eval(struct svarexec * svx, struct svartoken * svt);
/***************************************************************/

/***************************************************************/
#if IS_MULTITHREADED
    #if IS_WINDOWS
        CRITSECTYP * xeq_crit_section = NULL;
    #else
        CRITSECTYP * xeq_crit_section = NULL;
    #endif /* _Windows */
    int xeq_crit_section_nrefs = 0;
#endif

/***************************************************************/
#if IS_MULTITHREADED
void init_xeq_crit_section(void)
{
    if (!xeq_crit_section_nrefs) {
        xeq_crit_section = New(CRITSECTYP, 1);
        CRIT_INIT(xeq_crit_section);
    }
    xeq_crit_section_nrefs++;
}
/***************************************************************/
void shut_xeq_crit_section(void)
{
    xeq_crit_section_nrefs--;
    if (!xeq_crit_section_nrefs) {
        CRIT_DEL(xeq_crit_section);
        Free(xeq_crit_section);
    }
}
/***************************************************************/
void lock_xeq_crit_section(void)
{
    if (xeq_crit_section) {
        CRIT_LOCK(xeq_crit_section);
    }
}
/***************************************************************/
void unlock_xeq_crit_section(void)
{
    if (xeq_crit_section) {
        CRIT_ULOCK(xeq_crit_section);
    }
}
#endif
/***************************************************************/
static void xstack_char(char * xbuf, int xstate)
{
    switch (xstate) {
        case XSTAT_TRUE_IF                 : strcpy(xbuf, "TRUE_IF");                 break;
        case XSTAT_TRUE_ELSE               : strcpy(xbuf, "TRUE_ELSE");               break;
        case XSTAT_TRUE_WHILE              : strcpy(xbuf, "TRUE_WHILE");              break;

        case XSTAT_FALSE_IF                : strcpy(xbuf, "FALSE_IF");                break;
        case XSTAT_FALSE_ELSE              : strcpy(xbuf, "FALSE_ELSE");              break;
        case XSTAT_INACTIVE_IF             : strcpy(xbuf, "INACTIVE_IF");             break;
        case XSTAT_INACTIVE_ELSE           : strcpy(xbuf, "INACTIVE_ELSE");           break;
        case XSTAT_INACTIVE_WHILE          : strcpy(xbuf, "INACTIVE_WHILE");          break;
        case XSTAT_RETURN                  : strcpy(xbuf, "RETURN");                  break;

        case XSTAT_TRUE_IF_COMPLETE        : strcpy(xbuf, "TRUE_IF_COMPLETE");        break;
        case XSTAT_FALSE_IF_COMPLETE       : strcpy(xbuf, "FALSE_IF_COMPLETE");       break;
        /* case XSTAT_TRUE_ELSE_COMPLETE      : strcpy(xbuf, "TRUE_ELSE_COMPLETE");      break; */
        /* case XSTAT_FALSE_ELSE_COMPLETE     : strcpy(xbuf, "FALSE_ELSE_COMPLETE");     break; */
        case XSTAT_INACTIVE_IF_COMPLETE    : strcpy(xbuf, "INACTIVE_IF_COMPLETE");    break;
        /* case XSTAT_INACTIVE_ELSE_COMPLETE  : strcpy(xbuf, "INACTIVE_ELSE_COMPLETE");  break; */

        case XSTAT_BEGIN_THREAD            : strcpy(xbuf, "BEGIN_THREAD");            break;
        case XSTAT_BEGIN                   : strcpy(xbuf, "BEGIN");                   break;
        case XSTAT_INCLUDE_FILE            : strcpy(xbuf, "INCLUDE_FILE");            break;
        case XSTAT_FUNCTION                : strcpy(xbuf, "FUNCTION");                break;
        case XSTAT_ACTIVE_BLOCK            : strcpy(xbuf, "ACTIVE_BLOCK");            break;
        case XSTAT_INACTIVE_BLOCK          : strcpy(xbuf, "INACTIVE_BLOCK");          break;
        case XSTAT_DEFINE_FUNCTION         : strcpy(xbuf, "DEFINE_FUNCTION");         break;
        default:
            sprintf(xbuf, "(?=%d)", xstate);
            break;
    }
}
/***************************************************************/
static void printf_xstate(int xstate)
{
    char xbuf[32];

    xstack_char(xbuf, xstate);
    printf("%s", xbuf);
}
/***************************************************************/
#if DEBUG_XSTACK & 255
static void svare_print_xstack(struct svarexec * svx)
{
    int sp;

    printf("[");
    for (sp = 1; sp < svx->svx_nsvs; sp++) {
        if (sp > 1) printf(",");

        printf_xstate(svx->svx_svs[sp]->svs_xstate);
    }
    printf("]\n");
}
#endif
/***************************************************************/
static void svare_init_svarcursor(
    struct svarcursor * svc_tgt)
{
    svc_tgt->svc_str_ix  = 0;
    svc_tgt->svc_char_ix = 0;
    svc_tgt->svc_adv     = 0;
}
/***************************************************************/
static struct svarinput * new_svarinput(void)
{
    struct svarinput * svi;

    svi = New(struct svarinput, 1);
    svi->svi_strl = NULL;
    svare_init_svarcursor(&(svi->svi_pc));
    svi->svi_strl_needs_free = 0;

    return (svi);
}
/***************************************************************/
static void free_svarinput(struct svarinput * svi)
{
    if (svi->svi_strl_needs_free) strlist_free(svi->svi_strl);

    Free(svi);
}
/***************************************************************/
static struct svarformalparms * new_svarformalparms(void)
{
    struct svarformalparms * svfp;

    svfp = New(struct svarformalparms, 1);
    svfp->svfp_afp = NULL;
    svfp->svfp_nfp = 0;

    return (svfp);
}
/***************************************************************/
static void free_svarformalparms(struct svarformalparms * svfp)
{
    int ii;

    if (!svfp) return;

    for (ii = svfp->svfp_nfp - 1; ii >= 0; ii--) {
        Free(svfp->svfp_afp[ii]);
    }
    Free(svfp->svfp_afp);
    Free(svfp);
}
/***************************************************************/
static struct svarfunction * new_svarfunction(void)
{
    struct svarfunction * svf;

    svf = New(struct svarfunction, 1);
    svf->svf_formals    = NULL;
    svf->svf_body       = NULL;
    svf->svf_funcname   = NULL;
    svf->svf_nrefs      = 0;

    return (svf);
}
/***************************************************************/
static void free_svarfunction(struct svarfunction * svf)
{
    svf->svf_nrefs -= 1;
    if (!svf->svf_nrefs) {
        free_svarformalparms(svf->svf_formals);
        Free(svf->svf_funcname);
        Free(svf);
    }
}
/***************************************************************/
/* svar functions */
/***************************************************************/
void svare_get_svarglob(struct svarexec * svx)
{
    if (!svx->svx_lock_count) {
#if IS_DEBUG
        if (svx->svx_locked_svg) {
            printf("******** Locked global pointer not null.\n");
        }
#endif
#if IS_MULTITHREADED
        lock_xeq_crit_section();
#endif
        svx->svx_locked_svg = (struct svarglob *)(svx->svx_vsvg);
    }

    svx->svx_lock_count += 1;
}
/***************************************************************/
void svare_release_svarglob(struct svarexec * svx)
{
#if IS_DEBUG
    if (!svx->svx_locked_svg || !svx->svx_lock_count) {
        printf("******** Global pointer not Locked.\n");
    }
#endif
    svx->svx_lock_count -= 1;

    if (!svx->svx_lock_count) {
        svx->svx_locked_svg = NULL;
#if IS_MULTITHREADED
        unlock_xeq_crit_section();
#endif
    }
}
/***************************************************************/
static void svare_free_svg_tokensave(struct svarglob * svg)
{
    while (svg->svg_xsvts) {
        svg->svg_xsvts -= 1;
        strlist_free(svg->svg_asvts[svg->svg_xsvts]->svts_strl);
        Free(svg->svg_asvts[svg->svg_xsvts]);
    }
    Free(svg->svg_asvts);
}
/***************************************************************/
static struct svarglob * svar_new_svarglob(void)
{
    struct svarglob * svg;

    svg = New(struct svarglob, 1);
    svg->svg_nsvts = 0;
    svg->svg_xsvts = 0;
    svg->svg_asvts = NULL;
    svg->svg_svt_depth = 0;
#if IS_MULTITHREADED
    svg->svg_thl = threadlist_new();
#endif

    return (svg);
}
/***************************************************************/
static void svar_free_svarglob(struct svarglob * svg)
{
    svare_free_svg_tokensave(svg);
#if IS_MULTITHREADED
    threadlist_free(svg->svg_thl);
#endif

    Free(svg);
}
/***************************************************************/
void svar_wait_for_all_threads(struct svarglob * svg)
{
#if IS_MULTITHREADED
    threadlist_wait_for_all_threads(svg->svg_thl);
#endif
}
/***************************************************************/
static struct svarvalue * svar_new_svarvalue_chars(const char * svarval,
    int svarvallen)
{
    struct svarvalue * svv;

    svv = New(struct svarvalue, 1);
    svv->svv_val.svv_chars.svvc_size   = svarvallen + 1;
    svv->svv_val.svv_chars.svvc_val_chars =
        New(char, svv->svv_val.svv_chars.svvc_size);
    memcpy(svv->svv_val.svv_chars.svvc_val_chars, svarval, svarvallen);
    svv->svv_val.svv_chars.svvc_val_chars[svarvallen] = '\0';

    svv->svv_dtype = SVV_DTYPE_CHARS;
    svv->svv_flags = 0;
    svv->svv_val.svv_chars.svvc_length = svarvallen;

    return (svv);
}
/***************************************************************/
struct svarvalue * svare_find_int_member(
    struct svarvalue_int_class * svnc,
    const char * vnam)
{
    struct svarvalue ** psvv;
    struct svarvalue *  svv;
    int vnlen;

    svv = NULL;
    vnlen = IStrlen(vnam) + 1;
    psvv = (struct svarvalue **)dbtree_find(svnc->svnc_svrec->svar_svarvalue_dbt,
        vnam, vnlen);
    if (psvv) {
        svv = (*psvv);
    }

    return (svv);
}
/***************************************************************/
#if 0
struct svarvalue * svare_find_member(
    struct svarvalue * csvv,
    const char * vnam)
{
    struct svarvalue ** psvv;
    struct svarvalue *  svv;
    int vnlen;

    psvv = NULL;
    svv = NULL;
    vnlen = IStrlen(vnam) + 1;
    if (csvv->svv_dtype == SVV_DTYPE_CLASS) {
        psvv = (struct svarvalue **)dbtree_find(csvv->svv_val.svv_svic.svic_svrec->svar_svarvalue_dbt,
            vnam, vnlen);
        if (psvv) {
            svv = (*psvv);
        }
    } else if (csvv->svv_dtype == SVV_DTYPE_INT_CLASS) {
        svv = svare_find_int_member(csvv->svv_val.svv_svnc, vnam);
    }

    return (svv);
}
#endif
/***************************************************************/
static void svar_vfree_class_svarvalue(void * vsvv)
{
    struct svarvalue * svv = (struct svarvalue *)vsvv;

    svar_free_svarvalue(svv);
}
/***************************************************************/
static void svar_free_svarvalue_int_class(struct svarvalue_int_class * svnc)
{
    svnc->svnc_nrefs -= 1;

    if (!svnc->svnc_nrefs) {
        dbtree_free(svnc->svnc_svrec->svar_svarvalue_dbt, svar_vfree_class_svarvalue);
        Free(svnc->svnc_svrec);
        Free(svnc);
    }
}
/***************************************************************/
static void svar_free_svarvalue_method(struct svarvalue * svv)
{
}
/***************************************************************/
static void svar_free_svarvalue_function(struct svarvalue * svv)
{
    free_svarfunction(svv->svv_val.svv_svf);
}
/***************************************************************/
static void svar_free_array(struct svarvalue_array * sva)
{
    int ii;

    sva->sva_nrefs -= 1;

    if (!sva->sva_nrefs) {
        for (ii = 0; ii < sva->sva_nelement; ii++) {
            svar_free_svarvalue(sva->sva_aelement[ii]);
        }
        Free(sva->sva_aelement);
        Free(sva);
    }
}
/***************************************************************/
static void svar_free_svarvalue_array(struct svarvalue * svv)
{
    svar_free_array(svv->svv_val.svv_array);
}
/***************************************************************/
static void svar_free_svarvalue_int_object(struct svarvalue * svv)
{
    int svstat = 0;
    struct svarvalue_int_class * svnc;
    struct svarvalue * msvv;
    struct svarvalue * asvv[1];
    SVAR_INTERNAL_FUNCTION ifuncptr;
    struct svarvalue_int_object_value * svviv;

    svviv = svv->svv_val.svv_svvi.svvi_svviv;
    svviv->svviv_nrefs -= 1;
    svnc = svv->svv_val.svv_svvi.svvi_int_class;

    if (!svviv->svviv_nrefs) {
        msvv = svare_find_int_member(svnc, ".delete");
        if (msvv && msvv->svv_dtype == SVV_DTYPE_METHOD) {
            ifuncptr = msvv->svv_val.svv_method;
            asvv[0] = svv;
            svstat = (ifuncptr)(NULL, NULL, 1, asvv, NULL);
        }
        Free(svviv);
    }
    svar_free_svarvalue_int_class(svnc);
}
/***************************************************************/
static void svar_free_svarvalue_data(struct svarvalue * svv)
{
    switch (svv->svv_dtype) {
        case SVV_DTYPE_NONE  :
        case SVV_DTYPE_BOOL  :
        case SVV_DTYPE_INT   :
        case SVV_DTYPE_LONG  :
        case SVV_DTYPE_DOUBLE:
            break;
        case SVV_DTYPE_CHARS :
            Free(svv->svv_val.svv_chars.svvc_val_chars);
            break;
        case SVV_DTYPE_PTR   :
            if (svv->svv_val.svv_ptr.svvp_destructor)
                (svv->svv_val.svv_ptr.svvp_destructor)
                    (svv->svv_val.svv_ptr.svvp_val_ptr);
            break;
        case SVV_DTYPE_METHOD   :
            svar_free_svarvalue_method(svv);
            break;
        case SVV_DTYPE_USER_FUNCTION   :
            svar_free_svarvalue_function(svv);
            break;
        case SVV_DTYPE_ARRAY   :
            svar_free_svarvalue_array(svv);
            break;
        case SVV_DTYPE_INT_CLASS   :
            svar_free_svarvalue_int_class(svv->svv_val.svv_svnc);
            break;
        case SVV_DTYPE_INT_OBJECT   :
            svar_free_svarvalue_int_object(svv);
            break;
        case SVV_DTYPE_INT_FUNCTION:
            break;
        default:
            break;
    }
    svv->svv_dtype = 0;
}
/***************************************************************/
void svar_free_svarvalue(struct svarvalue * svv)
{
    svar_free_svarvalue_data(svv);
    Free(svv);
}
/***************************************************************/
static void svar_vfree_svarvalue(void * vsvv)
{
    svar_free_svarvalue((struct svarvalue *)vsvv);
}
/***************************************************************/
struct svarrec * svar_new(void)
{
    struct svarrec * svrec;

    svrec = New(struct svarrec, 1);
    svrec->svar_svarvalue_dbt    = dbtree_new();

    return (svrec);
}
/***************************************************************/
void svar_free(struct svarrec * svrec)
{
    dbtree_free(svrec->svar_svarvalue_dbt, svar_vfree_svarvalue);
    Free(svrec);
}
/***************************************************************/
static void svare_free_xstat(struct svarexec * svx)
{
    while (svx->svx_xsvs) {
        svx->svx_xsvs -= 1;
        Free(svx->svx_svs[svx->svx_xsvs]);
    }
    Free(svx->svx_svs);
}
/***************************************************************/
int svare_get_svstat(struct svarexec * svx) {

    return (svx->svx_svstat);
}
/***************************************************************/
int svare_set_error(struct svarexec * svx, int svstat, char *fmt, ...) {

    va_list args;

    va_start (args, fmt);
    svx->svx_errmsg = Vsmprintf (fmt, args);
    va_end (args);
    svx->svx_svstat = svstat;

    return (svstat);
}
/***************************************************************/
int svare_clear_error(struct svarexec * svx, int in_svstat) {

    int out_svstat;

    out_svstat = 0;
    Free(svx->svx_errmsg);
    svx->svx_errmsg = NULL;
    svx->svx_svstat = out_svstat;

    return (out_svstat);
}
/***************************************************************/
void svare_svarvalue_bool(
    struct svarvalue * svv,
    int val_bool)
{
    svar_free_svarvalue_data(svv);
    svv->svv_dtype = SVV_DTYPE_BOOL;
    svv->svv_val.svv_val_bool = val_bool;
}
/***************************************************************/
void svare_svarvalue_int(
    struct svarvalue * svv,
    int32 val_int)
{
    svar_free_svarvalue_data(svv);
    svv->svv_dtype = SVV_DTYPE_INT;
    svv->svv_val.svv_val_int = val_int;
}
/***************************************************************/
static void svare_svarvalue_long(
    struct svarvalue * svv,
    int64 val_long)
{
    svar_free_svarvalue_data(svv);
    svv->svv_dtype = SVV_DTYPE_LONG;
    svv->svv_val.svv_val_long = val_long;
}
/***************************************************************/
void svare_svarvalue_double(
    struct svarvalue * svv,
    double val_double)
{
    svar_free_svarvalue_data(svv);
    svv->svv_dtype = SVV_DTYPE_DOUBLE;
    svv->svv_val.svv_val_double = val_double;
}
/***************************************************************/
void svare_store_string(
    struct svarvalue * svv,
    const char * bufval,
    int          buflen)
{
    if (svv->svv_dtype == SVV_DTYPE_CHARS) {
        if (buflen >= svv->svv_val.svv_chars.svvc_size) {
            svv->svv_val.svv_chars.svvc_size   = buflen + 1;
            svv->svv_val.svv_chars.svvc_val_chars =
        Realloc(svv->svv_val.svv_chars.svvc_val_chars, char, buflen + 1);
        }
    } else {
        svar_free_svarvalue_data(svv);
        svv->svv_val.svv_chars.svvc_size   = buflen + 1;
        svv->svv_val.svv_chars.svvc_val_chars =
            New(char, svv->svv_val.svv_chars.svvc_size);
        svv->svv_dtype = SVV_DTYPE_CHARS;
    }
    svv->svv_val.svv_chars.svvc_length   = buflen;
    memcpy(svv->svv_val.svv_chars.svvc_val_chars, bufval, buflen);
    svv->svv_val.svv_chars.svvc_val_chars[buflen] = '\0';
}
/***************************************************************/
static void svare_store_binary(
    struct svarvalue * svv,
    const char * bufval,
    int          buflen)
{
    svare_store_string(svv, bufval, buflen);
}
/***************************************************************/
void svare_store_string_pointer(
    struct svarvalue * svv,
    char * bufval,
    int buflen)
{
    if (svv->svv_dtype == SVV_DTYPE_CHARS) {
        if (buflen < svv->svv_val.svv_chars.svvc_size) {
            strcpy(svv->svv_val.svv_chars.svvc_val_chars, bufval);
            Free(bufval);
        } else {
            Free(svv->svv_val.svv_chars.svvc_val_chars);
            svv->svv_val.svv_chars.svvc_val_chars = bufval;
            svv->svv_val.svv_chars.svvc_size = buflen + 1;

        }
    } else {
        svar_free_svarvalue_data(svv);
        svv->svv_dtype = SVV_DTYPE_CHARS;
        svv->svv_val.svv_chars.svvc_val_chars = bufval;
        svv->svv_val.svv_chars.svvc_size = buflen + 1;
    }
    svv->svv_val.svv_chars.svvc_length   = buflen;
}
/***************************************************************/
static void svare_store_ptr(
    struct svarvalue * svv,
    void * val_ptr,
    int    class_id,
    void (*p_destructor)(void*),
    void * (*p_copy)(void*) )
{
    svar_free_svarvalue_data(svv);
    svv->svv_dtype = SVV_DTYPE_PTR;
    svv->svv_val.svv_ptr.svvp_val_ptr       = val_ptr;
    svv->svv_val.svv_ptr.svvp_class_id      = class_id;
    svv->svv_val.svv_ptr.svvp_destructor    = p_destructor;
    svv->svv_val.svv_ptr.svvp_copy          = p_copy;
}
/***************************************************************/
static void svar_jsontree_destroy(void * vjt) {

    free_jsontree((struct jsontree *)vjt);
}
/***************************************************************/
void svare_store_int_object(
    struct svarexec * svx,
    struct svarvalue_int_class * svnc,
    struct svarvalue * svv,
    void * val_ptr)
{
    struct svarvalue_int_object_value * svviv;

    svar_free_svarvalue_data(svv);

    svviv = New(struct svarvalue_int_object_value, 1);
    svviv->svviv_val_ptr = val_ptr;
    svviv->svviv_nrefs   = 1;
    svv->svv_dtype = SVV_DTYPE_INT_OBJECT;
    svv->svv_val.svv_svvi.svvi_int_class = svnc;
    svv->svv_val.svv_svvi.svvi_svviv = svviv;
    svnc->svnc_nrefs += 1;
}
/***************************************************************/
static int svare_store_number(
    struct svarexec * svx,
    struct svarvalue * svv,
    const char * strval)
{
    int svstat = 0;
    int64 nlong;
    double ndouble;
    double pten;
    int ii;

    nlong = 0;
    ii = 0;
    while (isdigit(strval[ii])) {
        nlong = (nlong * 10) + (strval[ii] - '0');
        ii++;
    }

    if (strval[ii] == '.') {
        ndouble = (double)nlong;
        pten = 1.0;
        ii++;
        while (isdigit(strval[ii])) {
            pten /= 10.0;
            ndouble += pten * (double)(strval[ii] - '0');
            ii++;
        }
        svare_svarvalue_double(svv, ndouble);
    } else {
        if (strval[ii] == 'l' || strval[ii] == 'L') {
            svare_svarvalue_long(svv, nlong);
            ii++;
        } else {
            svare_svarvalue_int(svv, (int)nlong);
        }
    }

    if (strval[ii]) {
        svstat = svare_set_error(svx, SVARERR_BAD_NUMBER,
            "Illegal number: %s", strval);
    }

    return (svstat);
}
/***************************************************************/
static int svare_svarvalue_concat(
    struct svarexec * svx,
    struct svarvalue * svv1,
    struct svarvalue * svv2)
{
    int svstat = 0;
    int tlen;

    if (svv1->svv_dtype != SVV_DTYPE_CHARS ||
        svv2->svv_dtype != SVV_DTYPE_CHARS) {
        svstat = svare_set_error(svx, SVARERR_OPERATOR_TYPE_MISMATCH,
            "Illegal string concatenation");
    } else {
        tlen = svv1->svv_val.svv_chars.svvc_length +
               svv2->svv_val.svv_chars.svvc_length + 1;
        if (tlen > svv1->svv_val.svv_chars.svvc_size) {
            svv1->svv_val.svv_chars.svvc_size = tlen;
            svv1->svv_val.svv_chars.svvc_val_chars =
                Realloc(svv1->svv_val.svv_chars.svvc_val_chars, char, tlen);
        }
        memcpy(svv1->svv_val.svv_chars.svvc_val_chars +
               svv1->svv_val.svv_chars.svvc_length,
            svv2->svv_val.svv_chars.svvc_val_chars,
            svv2->svv_val.svv_chars.svvc_length);
        svv1->svv_val.svv_chars.svvc_length   = tlen - 1;
        svv1->svv_val.svv_chars.svvc_val_chars[tlen - 1] = '\0';
    }

    return (svstat);
}
/***************************************************************/
int svar_get_int_variable(struct svarexec * svx,
        struct svarvalue * svv,
        int * p_num)
{
/* 12/05/2017 */
    int svstat = 0;

    (*p_num) = 0;
    switch (svv->svv_dtype) {
        case SVV_DTYPE_BOOL  :
            if (svv->svv_val.svv_val_bool) {
                (*p_num) = 1;
            } else {
                (*p_num) = 0;
            }
            break;
        case SVV_DTYPE_INT   :
            (*p_num) = svv->svv_val.svv_val_int;
            break;
        case SVV_DTYPE_LONG  :
            (*p_num) = (int)svv->svv_val.svv_val_long;
            break;
        case SVV_DTYPE_DOUBLE:
            (*p_num) = (int)svv->svv_val.svv_val_double;
            break;
        default:
            svstat = svare_set_error(svx, SVARERR_BAD_NUMBER,
                    "Expecting integer variable");
            break;
    }

    return (svstat);
}
/***************************************************************/
int svar_get_double_variable(struct svarexec * svx,
        struct svarvalue * svv,
        double * p_num)
{
    int svstat = 0;

    (*p_num) = 0.0;
    switch (svv->svv_dtype) {
        case SVV_DTYPE_BOOL  :
            if (svv->svv_val.svv_val_bool) {
                (*p_num) = 1.0;
            } else {
                (*p_num) = 0.0;
            }
            break;
        case SVV_DTYPE_INT   :
            (*p_num) = (double)svv->svv_val.svv_val_int;
            break;
        case SVV_DTYPE_LONG  :
            (*p_num) = (double)svv->svv_val.svv_val_long;
            break;
        case SVV_DTYPE_DOUBLE:
            (*p_num) = svv->svv_val.svv_val_double;
            break;
        default:
            svstat = svare_set_error(svx, SVARERR_BAD_NUMBER,
                    "Expecting double variable");
            break;
    }

    return (svstat);
}
/***************************************************************/
struct svarvalue * svare_new_variable(void)
{
    struct svarvalue * svv;

    svv = New(struct svarvalue, 1);
    memset(svv, 0, sizeof(struct svarvalue));

    return (svv);
}
/***************************************************************/
struct svarvalue * svare_cascading_find_variable(struct svarexec * svx,
    struct svarvalue * csvv,
    const char * vnam)
{
    struct svarvalue ** psvv;
    struct svarvalue *  svv;
    int vnlen;
#if IS_MULTITHREADED
    int crit_locked = 0;
#endif

    svv = NULL;
    vnlen = IStrlen(vnam) + 1;

    /* search class variables */
    if (csvv) {
        if (csvv->svv_dtype == SVV_DTYPE_INT_CLASS) {
            psvv = (struct svarvalue **)dbtree_find(
                csvv->svv_val.svv_svnc->svnc_svrec->svar_svarvalue_dbt,
                vnam, vnlen);
        } else if (csvv->svv_dtype == SVV_DTYPE_INT_OBJECT) {
            psvv = (struct svarvalue **)dbtree_find(
                csvv->svv_val.svv_svvi.svvi_int_class->svnc_svrec->svar_svarvalue_dbt,
                vnam, vnlen);
        } else {
            psvv = NULL;
        }
        if (psvv) {
            svv = (*psvv);
        }
    }

    /* search current variables */
    if (!svv && svx->svx_nvars > 2) {
        psvv = (struct svarvalue **)dbtree_find(
            svx->svx_avars[svx->svx_nvars-1]->svar_svarvalue_dbt,
            vnam, vnlen);
        if (psvv) {
            svv = (*psvv);
        }
    }

    /* search global variables - after execution */
    if (!svv && svx->svx_nvars > 1) {
#if IS_MULTITHREADED
    if (!crit_locked) { lock_xeq_crit_section(); crit_locked = 1; }
#endif
        psvv = (struct svarvalue **)dbtree_find(
            svx->svx_avars[1]->svar_svarvalue_dbt,
            vnam, vnlen);
        if (psvv) {
            svv = (*psvv);
        }
    }

    /* search global variables - before execution */
    if (!svv && svx->svx_nvars > 0) {
#if IS_MULTITHREADED
    if (!crit_locked) { lock_xeq_crit_section(); crit_locked = 1; }
#endif
        psvv = (struct svarvalue **)dbtree_find(
            svx->svx_avars[0]->svar_svarvalue_dbt,
            vnam, vnlen);
        if (psvv) {
            svv = (*psvv);
        }
    }

#if IS_MULTITHREADED
    if (crit_locked) { unlock_xeq_crit_section(); crit_locked = 0; }
#endif

    return (svv);
}
/***************************************************************/
struct svarvalue * svare_find_variable(struct svarexec * svx,
    struct svarvalue * csvv,
    const char * vnam)
{
    struct svarvalue ** psvv;
    struct svarvalue *  svv;
    int vnlen;

    svv = NULL;
    vnlen = IStrlen(vnam) + 1;

    /* search current variables */
    if (svx->svx_nvars > 1) {
        psvv = (struct svarvalue **)dbtree_find(
            svx->svx_avars[svx->svx_nvars-1]->svar_svarvalue_dbt,
            vnam, vnlen);
        if (psvv) {
            svv = (*psvv);
        }
    }

    return (svv);
}
/***************************************************************/
int svare_store_svarvalue(struct svarexec * svx,
    struct svarvalue * tsvv,
    struct svarvalue * ssvv)
{
    int svstat = 0;

    switch (ssvv->svv_dtype) {
        case SVV_DTYPE_NONE  :
            /* Function return of function with no return */
            break;
        case SVV_DTYPE_BOOL  :
            svare_svarvalue_bool(tsvv, ssvv->svv_val.svv_val_bool);
            break;
        case SVV_DTYPE_INT   :
            svare_svarvalue_int(tsvv, ssvv->svv_val.svv_val_int);
            break;
        case SVV_DTYPE_LONG  :
            svare_svarvalue_long(tsvv, ssvv->svv_val.svv_val_long);
            break;
        case SVV_DTYPE_DOUBLE:
            svare_svarvalue_double(tsvv, ssvv->svv_val.svv_val_double);
            break;
        case SVV_DTYPE_CHARS :
            svare_store_string(tsvv,
                ssvv->svv_val.svv_chars.svvc_val_chars,
                ssvv->svv_val.svv_chars.svvc_length);
            break;
        case SVV_DTYPE_PTR   :
            if (ssvv->svv_val.svv_ptr.svvp_copy) {
                svare_store_ptr(tsvv,
                    (ssvv->svv_val.svv_ptr.svvp_copy)
                                (ssvv->svv_val.svv_ptr.svvp_val_ptr),
                    ssvv->svv_val.svv_ptr.svvp_class_id,
                    ssvv->svv_val.svv_ptr.svvp_destructor,
                    ssvv->svv_val.svv_ptr.svvp_copy);
            } else if (!(ssvv->svv_val.svv_ptr.svvp_val_ptr)) {
                svare_store_ptr(tsvv, NULL, 0, NULL, NULL);
            } else {
                svstat = svare_set_error(svx, SVARERR_NO_COPY_AVAILABLE,
                        "Illegal pointer store");
            }
        case SVV_DTYPE_METHOD:
            svstat = svare_set_error(svx, SVARERR_NO_COPY_AVAILABLE,
                    "Illegal pointer store to method");
            break;
        case SVV_DTYPE_USER_FUNCTION:
            tsvv->svv_dtype = ssvv->svv_dtype;
            tsvv->svv_flags = ssvv->svv_flags;
            tsvv->svv_val.svv_svf = ssvv->svv_val.svv_svf;
            tsvv->svv_val.svv_svf->svf_nrefs += 1;
            break;
        case SVV_DTYPE_ARRAY:
            tsvv->svv_dtype = ssvv->svv_dtype;
            tsvv->svv_flags = ssvv->svv_flags;
            tsvv->svv_val.svv_array = ssvv->svv_val.svv_array;
            tsvv->svv_val.svv_array->sva_nrefs += 1;
            break;
        case SVV_DTYPE_INT_CLASS:
            tsvv->svv_dtype = ssvv->svv_dtype;
            tsvv->svv_flags = ssvv->svv_flags;
            tsvv->svv_val.svv_svnc = ssvv->svv_val.svv_svnc;
            tsvv->svv_val.svv_svnc->svnc_nrefs += 1;
            break;
        case SVV_DTYPE_INT_OBJECT:
            tsvv->svv_dtype = ssvv->svv_dtype;
            tsvv->svv_flags = ssvv->svv_flags;
            tsvv->svv_val.svv_svvi.svvi_int_class = ssvv->svv_val.svv_svvi.svvi_int_class;
            tsvv->svv_val.svv_svvi.svvi_svviv = ssvv->svv_val.svv_svvi.svvi_svviv;
            tsvv->svv_val.svv_svvi.svvi_svviv->svviv_nrefs += 1;
            tsvv->svv_val.svv_svvi.svvi_int_class->svnc_nrefs += 1;
            break;
        default:
            svstat = svare_set_error(svx, SVARERR_NO_COPY_AVAILABLE,
                    "Illegal pointer store to unknown(%d)", ssvv->svv_dtype);
            break;
    }

    return (svstat);
}
/***************************************************************/
static void svare_copy_svarvalue(
    struct svarvalue * tsvv,
    struct svarvalue * ssvv)
{
    svar_free_svarvalue_data(tsvv);
    memcpy(tsvv, ssvv, sizeof(struct svarvalue));
    memset(ssvv, 0, sizeof(struct svarvalue));
}
/***************************************************************/
static int svare_add_svarvalue(struct svarexec * svx,
    const char * vnam,
    struct svarvalue * svv)
{
    int svstat = 0;
    int is_dup;
#if IS_MULTITHREADED
    int crit_locked = 0;
#endif

    if (svx->svx_nvars) {
#if IS_MULTITHREADED
    if (!crit_locked && svx->svx_nvars <= 2)
        { lock_xeq_crit_section(); crit_locked = 1; }
#endif
        is_dup = dbtree_insert(
            svx->svx_avars[svx->svx_nvars-1]->svar_svarvalue_dbt,
            vnam, IStrlen(vnam) + 1, svv);
            if (is_dup) {
                svstat = SVARERR_DUPLICATE_VAR;
                svstat = svare_set_error(svx, SVARERR_DUPLICATE_VAR,
                    "Duplicate variable: %s", vnam);
            }
    }

#if IS_MULTITHREADED
    if (crit_locked) { unlock_xeq_crit_section(); crit_locked = 0; }
#endif

    return (svstat);
}
/***************************************************************/
static struct svarvalue * svare_get_svarvalue(struct svarexec * svx, const char * vnam)
{
    struct svarvalue * tsvv;
    int is_dup;

    tsvv = svare_find_variable(svx, NULL, vnam);
    if (!tsvv) {
        tsvv = svare_new_variable();
        if (svx->svx_nvars) {
            is_dup = dbtree_insert(
                svx->svx_avars[svx->svx_nvars-1]->svar_svarvalue_dbt,
                vnam, IStrlen(vnam) + 1, tsvv);
        }
    }

    return (tsvv);
}
/***************************************************************/
static int svare_set_svarvalue(struct svarexec * svx,
    const char * vnam,
    struct svarvalue * svv)
{
    int svstat = 0;
    struct svarvalue * tsvv;

    tsvv = svare_get_svarvalue(svx, vnam);

    /* - old way svstat = svare_store_svarvalue(svx, tsvv, svv); */
    svare_copy_svarvalue(tsvv, svv);

    return (svstat);
}
/***************************************************************/
static void svare_push_vars(struct svarexec * svx, struct svarrec * svar)
{
    if (svx->svx_nvars == svx->svx_mvars) {
        svx->svx_mvars += 4;
        svx->svx_avars = Realloc(svx->svx_avars,struct svarrec *,svx->svx_mvars);
    }
    svx->svx_avars[svx->svx_nvars] = svar;
    svx->svx_nvars += 1;
}
/***************************************************************/
static struct svarrec * svare_pop_vars(struct svarexec * svx)
{
    struct svarrec * svar;

    if (svx->svx_nvars > 0) {
        svx->svx_nvars -= 1;
        svar_free(svx->svx_avars[svx->svx_nvars]);
    }

    svar = NULL;
    if (svx->svx_nvars > 0) {
        svar = svx->svx_avars[svx->svx_nvars - 1];
    }

    return (svar);
}
/***************************************************************/
static void svare_copy_svarcursor(
    struct svarcursor * svc_tgt,
    struct svarcursor * svc_src)
{
    svc_tgt->svc_str_ix  = svc_src->svc_str_ix;
    svc_tgt->svc_char_ix = svc_src->svc_char_ix;
    svc_tgt->svc_adv     = svc_src->svc_adv;
}
/***************************************************************/
static void svare_get_current_svc(struct svarexec * svx, struct svarcursor * curr_svc)
{
    svare_copy_svarcursor(curr_svc, &(svx->svx_token_pc));
}
/***************************************************************/
static int svare_push_include(struct svarexec * svx, struct strlist * strl, int needs_free)
{
    int svstat = 0;

    if (svx->svx_nsvi == svx->svx_xsvi) {
        int nsvi = svx->svx_nsvi;
        if (!svx->svx_xsvi) svx->svx_xsvi = 4;
        else svx->svx_xsvi *= 2;
        if (svx->svx_xsvi >= MAX_INCLUDE_STACK) svx->svx_xsvi = MAX_INCLUDE_STACK;
        if (svx->svx_nsvi == MAX_INCLUDE_STACK) {
            svx->svx_xsvi = svx->svx_nsvi;  /* To prevent Free() errors */
            svstat = svare_set_error(svx, SVARERR_INCLUDE_STACK_OVERFLOW,
                "Include stack overflow.");
            return (svstat);
        }
        svx->svx_asvi = Realloc(svx->svx_asvi, struct svarinput*, svx->svx_xsvi);
        while (nsvi < svx->svx_xsvi) {
            svx->svx_asvi[nsvi++] = new_svarinput();
        }
    }

    svx->svx_asvi[svx->svx_nsvi]->svi_strl = svx->svx_svi->svi_strl;
    svx->svx_asvi[svx->svx_nsvi]->svi_strl_needs_free = svx->svx_svi->svi_strl_needs_free;
    svare_copy_svarcursor(&(svx->svx_asvi[svx->svx_nsvi]->svi_pc), &(svx->svx_svi->svi_pc));
    svx->svx_nsvi += 1;

    svx->svx_svi->svi_strl = strl;
    svx->svx_svi->svi_strl_needs_free = needs_free;
    svare_init_svarcursor(&(svx->svx_svi->svi_pc));

    return (svstat);
}
/***************************************************************/
static int svare_pop_include(struct svarexec * svx)
{
    int svstat = 0;

    if (!svx->svx_nsvi) {
        svstat = svare_set_error(svx, SVARERR_INCLUDE_FILE_UNDERFLOW,
            "Include stack underflow.");
        return (svstat);
    }

    if (svx->svx_svi->svi_strl_needs_free) {
        strlist_free(svx->svx_svi->svi_strl);
    }

    svx->svx_nsvi -= 1;
    svx->svx_svi->svi_strl = svx->svx_asvi[svx->svx_nsvi]->svi_strl;
    svx->svx_svi->svi_strl_needs_free = svx->svx_asvi[svx->svx_nsvi]->svi_strl_needs_free;
    svare_copy_svarcursor(&(svx->svx_svi->svi_pc), &(svx->svx_asvi[svx->svx_nsvi]->svi_pc));

    return (svstat);
}
/***************************************************************/
static void svare_free_includes(struct svarexec * svx)
{
    int svstat = 0;

    while (svx->svx_nsvi) {
        svx->svx_nsvi -= 1;
        if (svx->svx_asvi[svx->svx_nsvi]->svi_strl_needs_free) {
            strlist_free(svx->svx_asvi[svx->svx_nsvi]->svi_strl);
        }
    }

    while (svx->svx_xsvi) {
        svx->svx_xsvi -= 1;
        Free(svx->svx_asvi[svx->svx_xsvi]);
    }
    Free(svx->svx_asvi);
}
/***************************************************************/
struct svarexec * svare_init(
        struct svarglob * svg,
        struct svarrec * svrec,
        struct strlist * strl,
        void * gdata)
{
    struct svarexec * svx;

#if IS_MULTITHREADED
    init_xeq_crit_section();
#endif

    svx = New(struct svarexec, 1);
    svx->svx_svi = new_svarinput();
    svx->svx_svi->svi_strl = strl;
    svx->svx_svi->svi_strl_needs_free = 1;
    svare_init_svarcursor(&(svx->svx_token_pc));
    svx->svx_errmsg = NULL;
    svx->svx_nvars = 0;
    svx->svx_mvars = 0;
    svx->svx_avars = NULL;
    /* svx->svx_class_svar = svar_new(); */
    svx->svx_gdata = gdata;
    svx->svx_nsvs  = 0;
    svx->svx_xsvs  = 0;
    svx->svx_svs   = NULL;
    svx->svx_nsvi  = 0;
    svx->svx_xsvi  = 0;
    svx->svx_asvi  = NULL;

    svare_push_vars(svx, svrec);

    svare_push_vars(svx, svar_new()); /* create new variable spave */

    svare_init_internal_classes(svx);
    svare_init_internal_functions(svx);

    svx->svx_locked_svg = NULL;
    svx->svx_lock_count = 0;
    svx->svx_vsvg       = svg;

    return (svx);
}
/***************************************************************/
static void svare_free(struct svarexec * svx)
{
    svare_free_xstat(svx);
    svare_free_includes(svx);

    while (svx->svx_nvars > 1) {
        svare_pop_vars(svx);
    }

    Free(svx->svx_avars);
    Free(svx->svx_errmsg);

    free_svarinput(svx->svx_svi);

    Free(svx);
}
/***************************************************************/
static void svare_shut(struct svarexec * svx)
{
    while (svx->svx_nvars > 1) svare_pop_vars(svx);
    svare_free(svx);

#if IS_MULTITHREADED
    shut_xeq_crit_section();
#endif
}
/***************************************************************/
struct svarexec * svare_dup_svarexec(struct svarexec * svx)
{
    struct svarexec * n_svx;

    n_svx = New(struct svarexec, 1);

    n_svx->svx_svi = new_svarinput();
    svare_init_svarcursor(&(n_svx->svx_token_pc));
    n_svx->svx_errmsg = NULL;
    n_svx->svx_nvars = 0;
    n_svx->svx_mvars = 0;
    n_svx->svx_avars = NULL;
    /* n_svx->svx_class_svar   = svx->svx_class_svar; */
    n_svx->svx_gdata        = svx->svx_gdata; /* Fix */
    n_svx->svx_nsvs  = 0;
    n_svx->svx_xsvs  = 0;
    n_svx->svx_svs   = NULL;
    n_svx->svx_nsvi  = 0;
    n_svx->svx_xsvi  = 0;
    n_svx->svx_asvi  = NULL;

    if (svx->svx_nvars > 0) {
        svare_push_vars(n_svx, svx->svx_avars[0]);
    }

    if (svx->svx_nvars > 1) {
        svare_push_vars(n_svx, svx->svx_avars[1]);
    }

    n_svx->svx_locked_svg = NULL;
    n_svx->svx_lock_count = 0;
    n_svx->svx_vsvg       = svx->svx_vsvg;

    return (n_svx);
}
/***************************************************************/
void svare_release_svarexec(struct svarexec * svx)
{
    svare_free_xstat(svx);
    svare_free_includes(svx);

    while (svx->svx_nvars > 2) {
        svare_pop_vars(svx);
    }
    Free(svx->svx_avars);
    Free(svx->svx_errmsg);
    free_svarinput(svx->svx_svi);

    Free(svx);
}
/***************************************************************/
static char svare_current_char(struct svarexec * svx)
{
    char svchar;

    svchar = 0;
    if (svx->svx_svi->svi_pc.svc_str_ix < svx->svx_svi->svi_strl->strl_len) {
        svchar = svx->svx_svi->svi_strl->strl_strs[svx->svx_svi->svi_pc.svc_str_ix]
                                         [svx->svx_svi->svi_pc.svc_char_ix];
    }

    return (svchar);
}
/***************************************************************/
static char svare_next_char(struct svarexec * svx)
{
/*
** Returns 0 on end of line
*/
    char svchar;

    svchar = 0;
    if (svx->svx_svi->svi_pc.svc_str_ix < svx->svx_svi->svi_strl->strl_len) {
        svchar = svx->svx_svi->svi_strl->strl_strs[svx->svx_svi->svi_pc.svc_str_ix]
                                         [svx->svx_svi->svi_pc.svc_char_ix + 1];
    }

    return (svchar);
}
/***************************************************************/
#define SVAR_TOKE_CHAR(c) (isalnum(c) || (c) == '_')

#define SVAR_VAR_CHAR0(c) (isalpha(c) || (c) == '_')
#define SVAR_VAR_CHAR(c) (isalnum(c) || (c) == '_')

#define SVAR_STMT_TERM_CHAR(c) ((c) == ';' || (c) == '}')
#define SVAR_BLOCK_BEGIN_CHAR(c) ((c) == '{')
#define SVAR_BLOCK_END_CHAR(c) ((c) == '}')
#define SVAR_STMT_END_CHAR(c) ((c) == ';')
/***************************************************************/
static char svare_advance_char(struct svarexec * svx, int skip_eol)
{
    char svchar;

    svchar = svare_current_char(svx);
    if (svchar) {
        svx->svx_svi->svi_pc.svc_char_ix += 1;
    } else {
        svx->svx_svi->svi_pc.svc_char_ix = 0;
        svx->svx_svi->svi_pc.svc_str_ix += 1;
    }
    svchar = svare_current_char(svx);

    if (!svchar && skip_eol) {
        while (!svchar && (svx->svx_svi->svi_pc.svc_str_ix < svx->svx_svi->svi_strl->strl_len)) {
            svx->svx_svi->svi_pc.svc_char_ix = 0;
            svx->svx_svi->svi_pc.svc_str_ix += 1;
            svchar = svare_current_char(svx);
        }
    }

    return (svchar);
}
/***************************************************************/
static char svare_get_char(struct svarexec * svx, int skip_eol)
{
/*
** Skips over end of line
*/
    char svchar;

    if (svx->svx_svi->svi_pc.svc_adv) {
        svchar = svare_advance_char(svx, skip_eol);
    } else {
        svchar = svare_current_char(svx);
        if (!svchar && skip_eol) {
            svchar = svare_advance_char(svx, skip_eol);
        }
    }
    if (svchar) svx->svx_svi->svi_pc.svc_adv = 1;
    else svx->svx_svi->svi_pc.svc_adv = 0;

    return (svchar);
}
/***************************************************************/
static void svare_advance_to_eol(struct svarexec * svx)
{
    char svchar;

    do {
        svchar = svare_get_char(svx, 0);
    } while (svchar);
}
/***************************************************************/
static char svare_skip_spaces(struct svarexec * svx)
{
    char svchar;
    int done;

    done = 0;
    while (!done) {
        do {
            svchar = svare_get_char(svx, 1);
        } while (isspace(svchar));
        svx->svx_svi->svi_pc.svc_adv = 0;
        if (svchar != '/') {
            done = 1;
        } else {
            if (svare_next_char(svx) != '/') {
                done = 1;
            } else {
                svchar = svare_get_char(svx, 0);
                svare_advance_to_eol(svx);
            }
        }
    }

    return (svchar);
}
/***************************************************************/
static char svare_xlate_backslash(struct svarexec * svx) {
/*
** Translate a backslashed character
*/

    char svchar;
    char out;
    unsigned char h1;
    unsigned char h2;

    svchar = svare_get_char(svx, 0);

    switch (svchar) {
        case '0':               out = '\0'; break; /* null */
        case 'a': case 'A':     out = '\a'; break; /* bell */
        case 'b': case 'B':     out = '\b'; break; /* backspace */
        case 'n': case 'N':     out = '\n'; break; /* newline */
        case 'r': case 'R':     out = '\r'; break; /* return */
        case 't': case 'T':     out = '\t'; break; /* tab */
        case 'x': case 'X':                        /* Hexadecimal */

            h1 = svare_get_char(svx, 0);
            if (h1) h2 = svare_get_char(svx, 0);
            else return (0);

            if (h1 >= '0' && h1 <= '9')      out = (h1 - '0') << 4;
            else if (h1 >= 'A' && h1 <= 'F') out = ((h1 - 'A') + 10) << 4;
            else if (h1 >= 'a' && h1 <= 'f') out = ((h1 - 'a') + 10) << 4;
            else return (0);

            if (h2 >= '0' && h2 <= '9')      out |= (h2 - '0');
            else if (h2 >= 'A' && h2 <= 'F') out |= (h2 - 'A') + 10;
            else if (h2 >= 'a' && h2 <= 'f') out |= (h2 - 'a') + 10;
            else return (0);

            break;

        default:                out = svchar;   break;
    }

    return (out);
}
/***************************************************************/
static int svare_get_token_quote_f(struct svarexec * svx,
    char qchar,
    struct svartoken * svt)
{
    int svstat = 0;
    char svchar;
    int done;

    done = 0;
    svt->svt_tlen = 0;

    if (svt->svt_tlen == svt->svt_tmax) {
        svt->svt_tmax += 64;
        svt->svt_text = Realloc(svt->svt_text, char, svt->svt_tmax);
    }
    svt->svt_text[svt->svt_tlen] = qchar;
    svt->svt_tlen += 1;

    while (!done) {
        svchar = svare_get_char(svx, 0);
        if (!svchar) {
            done = 1;
            svstat = svare_set_error(svx, SVARERR_UNMATCHED_QUOTE,
                    "Unmatched quotation");
        } else {
            if (svchar == qchar) {
                done = 1;
            }

            if (svt->svt_tlen == svt->svt_tmax) {
                svt->svt_tmax += 64;
                svt->svt_text = Realloc(svt->svt_text, char, svt->svt_tmax);
            }
            svt->svt_text[svt->svt_tlen] = svchar;
            svt->svt_tlen += 1;
        }
    }

    if (svt->svt_tlen == svt->svt_tmax) {
        svt->svt_tmax += 64;
        svt->svt_text = Realloc(svt->svt_text, char, svt->svt_tmax);
    }
    svt->svt_text[svt->svt_tlen] = '\0';

    return (svstat);
}
/***************************************************************/
static int svare_get_single_token_quote(struct svarexec * svx,
    char qchar,
    struct svartoken * svt)
{
    int svstat = 0;
    char svchar;
    int done;

    done = 0;

    while (!done) {
        svchar = svare_get_char(svx, 0);
        if (!svchar) {
            done = 1;
            svstat = svare_set_error(svx, SVARERR_UNMATCHED_QUOTE,
                    "Unmatched quotation");
        } else {
            if (svchar == qchar) {
                done = 1;
            } else if (svchar == '\\') {
                svchar = svare_xlate_backslash(svx);
            }

            if (svt->svt_tlen == svt->svt_tmax) {
                svt->svt_tmax += 64;
                svt->svt_text = Realloc(svt->svt_text, char, svt->svt_tmax);
            }
            svt->svt_text[svt->svt_tlen] = svchar;
            svt->svt_tlen += 1;
        }
    }

    if (svt->svt_tlen == svt->svt_tmax) {
        svt->svt_tmax += 64;
        svt->svt_text = Realloc(svt->svt_text, char, svt->svt_tmax);
    }
    svt->svt_text[svt->svt_tlen] = '\0';

    return (svstat);
}
/***************************************************************/
static int svare_get_token_quote(struct svarexec * svx,
    char qchar,
    struct svartoken * svt)
{
    int svstat = 0;
    int done;
    char svchar;

    done = 0;
    svt->svt_tlen = 0;
    svchar = qchar;

    if (svt->svt_tlen == svt->svt_tmax) {
        svt->svt_tmax += 64;
        svt->svt_text = Realloc(svt->svt_text, char, svt->svt_tmax);
    }
    svt->svt_text[svt->svt_tlen] = qchar;
    svt->svt_tlen += 1;

    while (!done) {
        done = 1;
        svstat = svare_get_single_token_quote(svx, svchar, svt);
        if (!svstat) {
            svchar = svare_skip_spaces(svx);
            if (svchar == '\"') {
                svt->svt_tlen -= 1; /* remove trailing quote */
                svchar = svare_get_char(svx, 0);
                done = 0;
            }
        }
    }

    return (svstat);
}
/***************************************************************/
static void svare_append_token(struct svartoken * svt, char svchar)
{
    if (svt->svt_tlen == svt->svt_tmax) {
        svt->svt_tmax += 64;
        svt->svt_text = Realloc(svt->svt_text, char, svt->svt_tmax);
    }
    svt->svt_text[svt->svt_tlen] = svchar;
    if (svchar) svt->svt_tlen += 1;
}
/***************************************************************/
static void svare_save_current_svc(struct svarexec * svx)
{
    svare_copy_svarcursor(&(svx->svx_token_pc), &(svx->svx_svi->svi_pc));
}
/***************************************************************/
static int svare_push_save_tokens(struct svarexec * svx, struct strlist * strl)
{
    int svstat = 0;

    svare_get_svarglob(svx);    /**** Lock globals ****/

    if (svx->svx_locked_svg->svg_nsvts == svx->svx_locked_svg->svg_xsvts) {
        int nsvts = svx->svx_locked_svg->svg_nsvts;
        if (!svx->svx_locked_svg->svg_xsvts) svx->svx_locked_svg->svg_xsvts = 2;
        else svx->svx_locked_svg->svg_xsvts *= 2;
        if (svx->svx_locked_svg->svg_xsvts > MAX_TOKEN_STACK) {
            svstat = svare_set_error(svx, SVARERR_TOKEN_STACK_OVERFLOW,
                "Token stack overflow.");
            return (svstat);
        }
        svx->svx_locked_svg->svg_asvts = Realloc(svx->svx_locked_svg->svg_asvts, struct svartokensave*, svx->svx_locked_svg->svg_xsvts);
        while (nsvts < svx->svx_locked_svg->svg_xsvts) {
            svx->svx_locked_svg->svg_asvts[nsvts] = New(struct svartokensave, 1);
            svx->svx_locked_svg->svg_asvts[nsvts]->svts_strl = NULL;
            nsvts++;
        }
    }
    svx->svx_locked_svg->svg_asvts[svx->svx_locked_svg->svg_nsvts]->svts_strl = strl;
    svx->svx_locked_svg->svg_nsvts += 1;
    svx->svx_locked_svg->svg_svt_depth += 1;

    svare_release_svarglob(svx);    /**** Unock globals ****/

    return (svstat);
}
/***************************************************************/
static int svare_pop_save_tokens(struct svarexec * svx)
{
    int svstat = 0;

    svare_get_svarglob(svx);    /**** Lock globals ****/

    if (svx->svx_locked_svg->svg_svt_depth == 0) {
        svstat = svare_set_error(svx, SVARERR_TOKEN_STACK_UNDERFLOW,
            "Token stack underflow.");
        return (svstat);
    }
    svx->svx_locked_svg->svg_svt_depth -= 1;

    svare_release_svarglob(svx);    /**** Unock globals ****/

    return (svstat);
}
/***************************************************************/
static void svare_save_token(struct svarexec * svx, struct svartoken * svt)
{
    char * etext;

    svare_get_svarglob(svx);    /**** Lock globals ****/

    if (svx->svx_locked_svg->svg_svt_depth) {
        if (any_escapes(svt->svt_text)) {
            etext = escape_text(svt->svt_text);
            strlist_append_str_to_last(svx->svx_locked_svg->svg_asvts[svx->svx_locked_svg->svg_nsvts - 1]->svts_strl,
                etext, MAX_TOKEN_LINE_LENGTH);
            Free(etext);
        } else {
            strlist_append_str_to_last(svx->svx_locked_svg->svg_asvts[svx->svx_locked_svg->svg_nsvts - 1]->svts_strl,
                svt->svt_text, MAX_TOKEN_LINE_LENGTH);
        }
    }

    svare_release_svarglob(svx);    /**** Unock globals ****/
}
/***************************************************************/
static int svare_get_token_eof(struct svarexec * svx, char * p_svchar)
{
    int svstat = 0;

    (*p_svchar) = 0;

    while (!svstat && !(*p_svchar)) {
        if (svx->svx_nsvi) {
            if (svx->svx_svs[svx->svx_nsvs - 1]->svs_xstate == XSTAT_INCLUDE_FILE) {
                svstat = svare_pop_xstat(svx, XSTAT_INCLUDE_FILE);
                if (!svstat) (*p_svchar) = svare_skip_spaces(svx);
            } else if (svx->svx_svs[svx->svx_nsvs - 1]->svs_xstate == XSTAT_FUNCTION) {
                svstat = svare_push_xstat(svx, XSTAT_RETURN);
                svstat = SVARERR_RETURN;
            } else {
                svstat = SVARERR_EOF;
            }
        } else {
            svstat = SVARERR_EOF;
        }
    }

    return (svstat);
}
/***************************************************************/
static void svare_unget_token(struct svarexec * svx)
{
    svare_copy_svarcursor(&(svx->svx_svi->svi_pc), &(svx->svx_token_pc));
}
/***************************************************************/
static int svare_get_token_no_save(struct svarexec * svx, struct svartoken * svt)
{
    int svstat = 0;
    char svchar;
    int all_digits;
    int done;

    done = 0;
    all_digits = 1;
    svt->svt_tlen = 0;

    svchar = svare_skip_spaces(svx);

    if (!svchar) {
        svstat = svare_get_token_eof(svx, &svchar);
    }

    svare_save_current_svc(svx);

    if (svstat || !svchar) {
        if (!svstat) svstat = SVARERR_EOF;
    } else if (svchar == '\"') {
        svchar = svare_get_char(svx, 0);
        svstat = svare_get_token_quote(svx, svchar, svt);
    } else {
        svchar = svare_get_char(svx, 0);
        while (!done) {
            svare_append_token(svt, svchar);
            if (!SVAR_TOKE_CHAR(svchar)) {
                svchar = svare_next_char(svx);
                if (svt->svt_tlen == 1 &&
                       ((svt->svt_text[0] == '<' && svchar == '=') ||
                        (svt->svt_text[0] == '>' && svchar == '=') ||
                        (svt->svt_text[0] == '<' && svchar == '>') )) {
                    svchar = svare_get_char(svx, 0);
                } else {
                    done = 1;
                }
            } else {
                if (all_digits) all_digits = isdigit(svchar);
                svchar = svare_get_char(svx, 0);
                if (!SVAR_TOKE_CHAR(svchar)) {
                    if (svchar == '.' && all_digits) {
                        all_digits = 0;
                        svare_append_token(svt, svchar);
                        svchar = svare_get_char(svx, 0);
                        if (!SVAR_TOKE_CHAR(svchar)) {
                            done = 1;
                            svx->svx_svi->svi_pc.svc_adv = 0;
                        }
                    } else if (svt->svt_tlen == 1 &&
                           ((svt->svt_text[0] == 'f' && svchar == '\"') ||
                            (svt->svt_text[0] == 'F' && svchar == '\"') )) {
                        svstat = svare_get_token_quote_f(svx, svchar, svt);
                        done = 1;
                    } else {
                        done = 1;
                        svx->svx_svi->svi_pc.svc_adv = 0;
                    }
                }
            }
        }
        svare_append_token(svt, '\0');
    }

    return (svstat);
}
/***************************************************************/
static int svare_get_token(struct svarexec * svx, struct svartoken * svt)
{
    int svstat;

    svstat = svare_get_token_no_save(svx, svt);

    /* Old: if (svx->svx_nsvts > 0 && !svstat && svt->svt_tlen) { */
    if (!svstat) {
        if (SVAR_BLOCK_END_CHAR(svt->svt_text[0]) &&
            svx->svx_svs[svx->svx_nsvs - 1]->svs_xstate == XSTAT_DEFINE_FUNCTION) {
            /* end of function -- do not save */
        } else {
            svare_save_token(svx, svt);
        }
    }
    /* printf("svare_get_token(\"%s\")=%d\n", svt->svt_text, svstat); */

    return (svstat);
}
/***************************************************************/
static struct svartoken * svar_new_svartoken(void)
{
    struct svartoken * svt;

    svt = New(struct svartoken, 1);
    svt->svt_tlen = 0;
    svt->svt_tmax = 0;
    svt->svt_text = NULL;

    return (svt);
}
/***************************************************************/
static void svar_free_svartoken(struct svartoken * svt)
{
    Free(svt->svt_text);
    Free(svt);
}
/***************************************************************/
static int svare_good_varname(const char * vname)
{
    int okname;
    int ii;

    if (!vname || !vname[0] || !SVAR_VAR_CHAR0(vname[0])) {
        okname = 0;
    } else {
        okname = 1;
        for (ii = 1; okname && vname[ii]; ii++) {
            if (!SVAR_VAR_CHAR(vname[0])) okname = 0;
        }
    }

    return (okname);
}
/***************************************************************/
static int svare_binop_mul(struct svarexec * svx,
    struct svarvalue * svv1,
    struct svarvalue * svv2)
{
    int svstat = 0;

    switch (svv1->svv_dtype) {
        case SVV_DTYPE_INT   :
            if (svv2->svv_dtype == SVV_DTYPE_INT) {
                svare_svarvalue_int(svv1,
                    svv1->svv_val.svv_val_int *
                    svv2->svv_val.svv_val_int);
            } else if (svv2->svv_dtype == SVV_DTYPE_LONG) {
                svare_svarvalue_long(svv1,
                    (int64)svv1->svv_val.svv_val_int *
                    svv2->svv_val.svv_val_long);
            } else if (svv2->svv_dtype == SVV_DTYPE_DOUBLE) {
                svare_svarvalue_double(svv1,
                    (double)svv1->svv_val.svv_val_int *
                    svv2->svv_val.svv_val_double);
            } else {
                svstat = svare_set_error(svx, SVARERR_OPERATOR_TYPE_MISMATCH,
                    "Illegal integer multiplication");
            }
            break;
        case SVV_DTYPE_LONG  :
            if (svv2->svv_dtype == SVV_DTYPE_INT) {
                svare_svarvalue_long(svv1,
                    svv1->svv_val.svv_val_long *
                    (int64)svv2->svv_val.svv_val_int);
            } else if (svv2->svv_dtype == SVV_DTYPE_LONG) {
                svare_svarvalue_long(svv1,
                    svv1->svv_val.svv_val_long *
                    svv2->svv_val.svv_val_long);
            } else if (svv2->svv_dtype == SVV_DTYPE_DOUBLE) {
                svare_svarvalue_double(svv1,
                    (double)svv1->svv_val.svv_val_long *
                    svv2->svv_val.svv_val_double);
            } else {
                svstat = svare_set_error(svx, SVARERR_OPERATOR_TYPE_MISMATCH,
                    "Illegal long multiplication");
            }
            break;
        case SVV_DTYPE_DOUBLE:
            if (svv2->svv_dtype == SVV_DTYPE_INT) {
                svare_svarvalue_double(svv1,
                    svv1->svv_val.svv_val_double *
                    (double)svv2->svv_val.svv_val_int);
            } else if (svv2->svv_dtype == SVV_DTYPE_LONG) {
                svare_svarvalue_double(svv1,
                    svv1->svv_val.svv_val_double *
                    (double)svv2->svv_val.svv_val_long);
            } else if (svv2->svv_dtype == SVV_DTYPE_DOUBLE) {
                svare_svarvalue_double(svv1,
                    svv1->svv_val.svv_val_double *
                    svv2->svv_val.svv_val_double);
            } else {
                svstat = svare_set_error(svx, SVARERR_OPERATOR_TYPE_MISMATCH,
                    "Illegal double multiplication");
            }
            break;

        case SVV_DTYPE_CHARS :
            svstat = svare_set_error(svx, SVARERR_OPERATOR_TYPE_MISMATCH,
                    "Illegal string multiplication");
            break;

        case SVV_DTYPE_PTR   :
            svstat = svare_set_error(svx, SVARERR_OPERATOR_TYPE_MISMATCH,
                    "Illegal object multiplication");
            break;

        default:
             svstat = svare_set_error(svx, SVARERR_OPERATOR_TYPE_MISMATCH,
                    "Illegal object multiplication");
           break;
    }


    return (svstat);
}
/***************************************************************/
static int svare_binop_div(struct svarexec * svx,
    struct svarvalue * svv1,
    struct svarvalue * svv2)
{
    int svstat = 0;

    switch (svv1->svv_dtype) {
        case SVV_DTYPE_INT   :
            if (svv2->svv_dtype == SVV_DTYPE_INT) {
                svare_svarvalue_int(svv1,
                    svv1->svv_val.svv_val_int / svv2->svv_val.svv_val_int);
            } else if (svv2->svv_dtype == SVV_DTYPE_LONG) {
                svare_svarvalue_long(svv1,
                    (int64)svv1->svv_val.svv_val_int /
                    svv2->svv_val.svv_val_long);
            } else if (svv2->svv_dtype == SVV_DTYPE_DOUBLE) {
                svare_svarvalue_double(svv1,
                    (double)svv1->svv_val.svv_val_int /
                    svv2->svv_val.svv_val_double);
            } else {
                svstat = svare_set_error(svx, SVARERR_OPERATOR_TYPE_MISMATCH,
                    "Illegal integer division");
            }
            break;
        case SVV_DTYPE_LONG  :
            if (svv2->svv_dtype == SVV_DTYPE_INT) {
                svare_svarvalue_long(svv1,
                    svv1->svv_val.svv_val_long /
                    (int64)svv2->svv_val.svv_val_int);
            } else if (svv2->svv_dtype == SVV_DTYPE_LONG) {
                svare_svarvalue_long(svv1,
                    svv1->svv_val.svv_val_long /
                    svv2->svv_val.svv_val_long);
            } else if (svv2->svv_dtype == SVV_DTYPE_DOUBLE) {
                svare_svarvalue_double(svv1,
                    (double)svv1->svv_val.svv_val_long /
                    svv2->svv_val.svv_val_double);
            } else {
                svstat = svare_set_error(svx, SVARERR_OPERATOR_TYPE_MISMATCH,
                    "Illegal long division");
            }
            break;
        case SVV_DTYPE_DOUBLE:
            if (svv2->svv_dtype == SVV_DTYPE_INT) {
                svare_svarvalue_double(svv1,
                    svv1->svv_val.svv_val_double /
                    (double)svv2->svv_val.svv_val_int);
            } else if (svv2->svv_dtype == SVV_DTYPE_LONG) {
                svare_svarvalue_double(svv1,
                    svv1->svv_val.svv_val_double /
                    (double)svv2->svv_val.svv_val_long);
            } else if (svv2->svv_dtype == SVV_DTYPE_DOUBLE) {
                svare_svarvalue_double(svv1,
                    svv1->svv_val.svv_val_double /
                    svv2->svv_val.svv_val_double);
            } else {
                svstat = svare_set_error(svx, SVARERR_OPERATOR_TYPE_MISMATCH,
                    "Illegal double division");
            }
            break;

        case SVV_DTYPE_CHARS :
            svstat = svare_set_error(svx, SVARERR_OPERATOR_TYPE_MISMATCH,
                    "Illegal string division");
            break;

        case SVV_DTYPE_PTR   :
            svstat = svare_set_error(svx, SVARERR_OPERATOR_TYPE_MISMATCH,
                    "Illegal pointer division");
            break;

        default:
            svstat = svare_set_error(svx, SVARERR_OPERATOR_TYPE_MISMATCH,
                    "Illegal division");
            break;
    }


    return (svstat);
}
/***************************************************************/
static int svare_binop_add(struct svarexec * svx,
    struct svarvalue * svv1,
    struct svarvalue * svv2)
{
    int svstat = 0;

    switch (svv1->svv_dtype) {
        case SVV_DTYPE_INT   :
            if (svv2->svv_dtype == SVV_DTYPE_INT) {
                svare_svarvalue_int(svv1,
                    svv1->svv_val.svv_val_int +
                    svv2->svv_val.svv_val_int);
            } else if (svv2->svv_dtype == SVV_DTYPE_LONG) {
                svare_svarvalue_long(svv1,
                    (int64)svv1->svv_val.svv_val_int +
                    svv2->svv_val.svv_val_long);
            } else if (svv2->svv_dtype == SVV_DTYPE_DOUBLE) {
                svare_svarvalue_double(svv1,
                    (double)svv1->svv_val.svv_val_int +
                    svv2->svv_val.svv_val_double);
            } else {
                svstat = svare_set_error(svx, SVARERR_OPERATOR_TYPE_MISMATCH,
                    "Illegal integer addition");
            }
            break;
        case SVV_DTYPE_LONG  :
            if (svv2->svv_dtype == SVV_DTYPE_INT) {
                svare_svarvalue_long(svv1,
                    svv1->svv_val.svv_val_long +
                    (int64)svv2->svv_val.svv_val_int);
            } else if (svv2->svv_dtype == SVV_DTYPE_LONG) {
                svare_svarvalue_long(svv1,
                    svv1->svv_val.svv_val_long +
                    svv2->svv_val.svv_val_long);
            } else if (svv2->svv_dtype == SVV_DTYPE_DOUBLE) {
                svare_svarvalue_double(svv1,
                    (double)svv1->svv_val.svv_val_long +
                    svv2->svv_val.svv_val_double);
            } else {
                svstat = svare_set_error(svx, SVARERR_OPERATOR_TYPE_MISMATCH,
                    "Illegal long addition");
            }
            break;
        case SVV_DTYPE_DOUBLE:
            if (svv2->svv_dtype == SVV_DTYPE_INT) {
                svare_svarvalue_double(svv1,
                    svv1->svv_val.svv_val_double +
                    (double)svv2->svv_val.svv_val_int);
            } else if (svv2->svv_dtype == SVV_DTYPE_LONG) {
                svare_svarvalue_double(svv1,
                    svv1->svv_val.svv_val_double +
                    (double)svv2->svv_val.svv_val_long);
            } else if (svv2->svv_dtype == SVV_DTYPE_DOUBLE) {
                svare_svarvalue_double(svv1,
                    svv1->svv_val.svv_val_double +
                    svv2->svv_val.svv_val_double);
            } else {
                svstat = svare_set_error(svx, SVARERR_OPERATOR_TYPE_MISMATCH,
                    "Illegal double addition");
            }
            break;

        case SVV_DTYPE_CHARS :
            svstat = svare_set_error(svx, SVARERR_OPERATOR_TYPE_MISMATCH,
                    "Illegal string addition");
            break;

        case SVV_DTYPE_PTR   :
            svstat = svare_set_error(svx, SVARERR_OPERATOR_TYPE_MISMATCH,
                    "Illegal object addition");
            break;

        default:
            break;
    }


    return (svstat);
}
/***************************************************************/
static int svare_binop_sub(struct svarexec * svx,
    struct svarvalue * svv1,
    struct svarvalue * svv2)
{
    int svstat = 0;

    switch (svv1->svv_dtype) {
        case SVV_DTYPE_INT   :
            if (svv2->svv_dtype == SVV_DTYPE_INT) {
                svare_svarvalue_int(svv1,
                    svv1->svv_val.svv_val_int -
                    svv2->svv_val.svv_val_int);
            } else if (svv2->svv_dtype == SVV_DTYPE_LONG) {
                svare_svarvalue_long(svv1,
                    (int64)svv1->svv_val.svv_val_int -
                    svv2->svv_val.svv_val_long);
            } else if (svv2->svv_dtype == SVV_DTYPE_DOUBLE) {
                svare_svarvalue_double(svv1,
                    (double)svv1->svv_val.svv_val_int -
                    svv2->svv_val.svv_val_double);
            } else {
                svstat = svare_set_error(svx, SVARERR_OPERATOR_TYPE_MISMATCH,
                    "Illegal integer subtraction");
            }
            break;
        case SVV_DTYPE_LONG  :
            if (svv2->svv_dtype == SVV_DTYPE_INT) {
                svare_svarvalue_long(svv1,
                    svv1->svv_val.svv_val_long -
                    (int64)svv2->svv_val.svv_val_int);
            } else if (svv2->svv_dtype == SVV_DTYPE_LONG) {
                svare_svarvalue_long(svv1,
                    svv1->svv_val.svv_val_long -
                    svv2->svv_val.svv_val_long);
            } else if (svv2->svv_dtype == SVV_DTYPE_DOUBLE) {
                svare_svarvalue_double(svv1,
                    (double)svv1->svv_val.svv_val_long -
                    svv2->svv_val.svv_val_double);
            } else {
                svstat = svare_set_error(svx, SVARERR_OPERATOR_TYPE_MISMATCH,
                    "Illegal long subtraction");
            }
            break;
        case SVV_DTYPE_DOUBLE:
            if (svv2->svv_dtype == SVV_DTYPE_INT) {
                svare_svarvalue_double(svv1,
                    svv1->svv_val.svv_val_double -
                    (double)svv2->svv_val.svv_val_int);
            } else if (svv2->svv_dtype == SVV_DTYPE_LONG) {
                svare_svarvalue_double(svv1,
                    svv1->svv_val.svv_val_double -
                    (double)svv2->svv_val.svv_val_long);
            } else if (svv2->svv_dtype == SVV_DTYPE_DOUBLE) {
                svare_svarvalue_double(svv1,
                    svv1->svv_val.svv_val_double -
                    svv2->svv_val.svv_val_double);
            } else {
                svstat = svare_set_error(svx, SVARERR_OPERATOR_TYPE_MISMATCH,
                    "Illegal double subtraction");
            }
            break;

        case SVV_DTYPE_CHARS :
            svstat = svare_set_error(svx, SVARERR_OPERATOR_TYPE_MISMATCH,
                    "Illegal string subtraction");
            break;

        case SVV_DTYPE_PTR   :
            svstat = svare_set_error(svx, SVARERR_OPERATOR_TYPE_MISMATCH,
                    "Illegal pointer subtraction");
            break;

        default:
            svstat = svare_set_error(svx, SVARERR_OPERATOR_TYPE_MISMATCH,
                    "Illegal subtraction");
            break;
    }

    return (svstat);
}
/***************************************************************/
static int svare_unop_neg(struct svarexec * svx,
    struct svarvalue * svv1)
{
    int svstat = 0;

    switch (svv1->svv_dtype) {
        case SVV_DTYPE_BOOL   :
            svstat = svare_set_error(svx, SVARERR_OPERATOR_TYPE_MISMATCH,
                    "Illegal boolean negative");
            break;

        case SVV_DTYPE_INT   :
            svare_svarvalue_int(svv1,
                - svv1->svv_val.svv_val_int);
            break;

        case SVV_DTYPE_LONG  :
                svare_svarvalue_long(svv1,
                    - svv1->svv_val.svv_val_long);
            break;
        case SVV_DTYPE_DOUBLE:
                svare_svarvalue_double(svv1,
                    - svv1->svv_val.svv_val_double);
            break;

        case SVV_DTYPE_CHARS :
            svstat = svare_set_error(svx, SVARERR_OPERATOR_TYPE_MISMATCH,
                    "Illegal string negative");
            break;

        case SVV_DTYPE_PTR   :
            svstat = svare_set_error(svx, SVARERR_OPERATOR_TYPE_MISMATCH,
                    "Illegal pointer negative");
            break;

        default:
            svstat = svare_set_error(svx, SVARERR_OPERATOR_TYPE_MISMATCH,
                    "Illegal negative");
            break;
    }

    return (svstat);
}
/***************************************************************/
static int svare_unop_not(struct svarexec * svx,
    struct svarvalue * svv1)
{
    int svstat = 0;

    switch (svv1->svv_dtype) {
        case SVV_DTYPE_BOOL   :
            if (svv1->svv_val.svv_val_bool) {
                svv1->svv_val.svv_val_bool = 0;
            } else {
                svv1->svv_val.svv_val_bool = 1;
            }
            break;

        case SVV_DTYPE_INT   :
            svstat = svare_set_error(svx, SVARERR_OPERATOR_TYPE_MISMATCH,
                    "Illegal integer not");
            break;

        case SVV_DTYPE_LONG  :
            svstat = svare_set_error(svx, SVARERR_OPERATOR_TYPE_MISMATCH,
                    "Illegal long not");
            break;

        case SVV_DTYPE_DOUBLE:
            svstat = svare_set_error(svx, SVARERR_OPERATOR_TYPE_MISMATCH,
                    "Illegal string not");
            break;

        case SVV_DTYPE_CHARS :
            svstat = svare_set_error(svx, SVARERR_OPERATOR_TYPE_MISMATCH,
                    "Illegal string not");
            break;

        case SVV_DTYPE_PTR   :
            svstat = svare_set_error(svx, SVARERR_OPERATOR_TYPE_MISMATCH,
                    "Illegal pointer not");
            break;

        default:
            svstat = svare_set_error(svx, SVARERR_OPERATOR_TYPE_MISMATCH,
                    "Illegal object not");
            break;
    }

    return (svstat);
}
/***************************************************************/
static int svare_eval_function(struct svarexec * svx,
    struct svarvalue * csvv,    /* Class pointerr -- May be NULL */
    struct svartoken * svt,
    struct svarvalue * msvv,    /* Method pointerr -- May be NULL */
    struct svarvalue * svv);

/***************************************************************/
struct svarvalue * svare_find_class(struct svarexec * svx,
    const char * cnam)
{
    struct svarvalue *  svv;

    svv = svare_cascading_find_variable(svx, NULL, cnam);
    if (svv) {
        if (svv->svv_dtype != SVV_DTYPE_INT_CLASS) {
            svv = NULL;
        }
    }

    return (svv);
}
/***************************************************************/
static struct svarvalue_array * svar_new_svarvalue_array(void)
{
    struct svarvalue_array * sva;

    sva = New(struct svarvalue_array, 1);
    sva->sva_nelement = 0;
    sva->sva_xelement = 0;
    sva->sva_aelement = NULL;
    sva->sva_nrefs    = 0;

    return (sva);
}
/***************************************************************/
static int svare_unop_new_array(struct svarexec * svx,
    struct svartoken * svt,
    struct svarvalue * svv)
{
    int svstat = 0;

    svv->svv_dtype = SVV_DTYPE_ARRAY;
    svv->svv_val.svv_array = svar_new_svarvalue_array();
    svv->svv_val.svv_array->sva_nrefs += 1;
    svstat = svare_get_token(svx, svt);
    if (!svstat) {
        if (svt->svt_text[0] != '(') {
            svstat = svare_set_error(svx, SVARERR_EXP_OPEN_PAREN,
                "Expecting open parenthesis. Found: %s", svt->svt_text);
        } else {
            svstat = svare_get_token(svx, svt);
        }
    }
    if (!svstat) {
        if (svt->svt_text[0] != ')') {
            svstat = svare_set_error(svx, SVARERR_EXP_CLOSE_PAREN,
                "Expecting closing parenthesis. Found: %s", svt->svt_text);
        } else {
            svstat = svare_get_token(svx, svt);
        }
    }

    return (svstat);
}
/***************************************************************/
static int svare_unop_new(struct svarexec * svx,
    struct svarvalue * pcsvv,
    struct svartoken * svt,
    struct svarvalue * svv)
{
    int svstat = 0;
    struct svarvalue * csvv;
    struct svarvalue * msvv;
    struct svarvalue svv2;
 
    csvv = svare_find_class(svx, svt->svt_text);
    if (!csvv) {
        if (!pcsvv && !strcmp(svt->svt_text, "Array")) {
            svstat = svare_unop_new_array(svx, svt, svv);
        } else {
            svstat = svare_set_error(svx, SVARERR_UNDEFINED_CLASS,
                "Undefined class: %s", svt->svt_text);
        }
    } else if (csvv->svv_dtype == SVV_DTYPE_INT_CLASS) {
        INIT_SVARVALUE(&svv2);
        svstat = svare_store_svarvalue(svx, &svv2, csvv);
        msvv = svare_find_int_member(csvv->svv_val.svv_svnc, ".new");
        if (!msvv) {
            svstat = svare_set_error(svx, SVARERR_CANNOT_FIND_MEMBER,
                "Cannot find new method");
        } else if (msvv->svv_dtype != SVV_DTYPE_METHOD) {
            svstat = svare_set_error(svx, SVARERR_MEMBER_NOT_METHOD,
                "New is not a method");
        } else {
            svstat = svare_eval_function(svx, &svv2, svt, msvv, svv);
        }
    }

    return (svstat);
}
/***************************************************************/
static int svare_binop_concat(struct svarexec * svx,
    struct svarvalue * svv1,
    struct svarvalue * svv2)
{
    int svstat = 0;
    char * str1;
    char * str2;
    char * cstr;
    int len1;
    int len2;
    int clen;

    str1 = svare_svarvalue_to_string(svx, svv1, 0);
    str2 = svare_svarvalue_to_string(svx, svv2, 0);

    if (!str1 || !str2) {
        svstat = svare_set_error(svx, SVARERR_OPERATOR_TYPE_MISMATCH,
            "Illegal string concatenation");
    } else {
        len1 = IStrlen(str1);
        len2 = IStrlen(str2);
        clen = len1 + len2;
        cstr = New(char, clen + 1);
        memcpy(cstr, str1, len1);
        memcpy(cstr + len1, str2, len2);
        cstr[clen] = '\0';
        svare_store_string_pointer(svv1, cstr, clen);
        svar_free_svarvalue_data(svv2);
        Free(str1);
        Free(str2);
    }

    return (svstat);
}
/***************************************************************/
#ifdef OLD_WAY
static int svare_binop_concat(struct svarexec * svx,
    struct svarvalue * svv1,
    struct svarvalue * svv2)
{
    int svstat = 0;

    switch (svv1->svv_dtype) {
        case SVV_DTYPE_INT   :
            svstat = svare_set_error(svx, SVARERR_OPERATOR_TYPE_MISMATCH,
                "Illegal integer concatenation");
            break;
        case SVV_DTYPE_LONG  :
            svstat = svare_set_error(svx, SVARERR_OPERATOR_TYPE_MISMATCH,
                "Illegal long concatenation");
            break;
        case SVV_DTYPE_DOUBLE:
            svstat = svare_set_error(svx, SVARERR_OPERATOR_TYPE_MISMATCH,
                "Illegal double concatenation");
            break;

        case SVV_DTYPE_CHARS :
            if (svv2->svv_dtype == SVV_DTYPE_CHARS) {
                svstat = svare_svarvalue_concat(svx, svv1, svv2);
                svar_free_svarvalue_data(svv2);
            } else {
                svstat = svare_set_error(svx, SVARERR_OPERATOR_TYPE_MISMATCH,
                        "Illegal string concatenation");
            }
            break;

        case SVV_DTYPE_PTR   :
            svstat = svare_set_error(svx, SVARERR_OPERATOR_TYPE_MISMATCH,
                    "Illegal pointer concatenation");
            break;

        default:
            svstat = svare_set_error(svx, SVARERR_OPERATOR_TYPE_MISMATCH,
                    "Illegal concatenation");
            break;
    }

    return (svstat);
}
#endif
/***************************************************************/
SVAR_INTERNAL_FUNCTION svare_find_function(struct svarexec * svx,
    struct svarvalue * csvv,    /* Object pointer -- May be NULL */
    struct svarvalue * msvv,    /* Method pointer -- May be NULL if csvv is NULL */
    const char * funcnam)
{
    SVAR_INTERNAL_FUNCTION ifuncptr;
    struct svarvalue * fsvv;

    ifuncptr = NULL;

    if (csvv) {
        if (msvv) {
            ifuncptr = msvv->svv_val.svv_method;
        } else {
            switch (csvv->svv_dtype) {
                case SVV_DTYPE_INT_OBJECT: 
                    fsvv = svare_find_int_member(csvv->svv_val.svv_svvi.svvi_int_class, funcnam);
                    break;
                case SVV_DTYPE_INT_CLASS: 
                    fsvv = svare_find_int_member(csvv->svv_val.svv_svnc, funcnam);
                    break;
                default: 
                    break;
            }
            if (fsvv && fsvv->svv_dtype == SVV_DTYPE_METHOD) {
                ifuncptr = fsvv->svv_val.svv_method;
            }
        }
    } else {
        fsvv = svare_cascading_find_variable(svx, csvv, funcnam);
        if (fsvv && fsvv->svv_dtype == SVV_DTYPE_INT_FUNCTION) {
            ifuncptr = fsvv->svv_val.svv_int_func;
        }
    }

    return (ifuncptr);
}
/***************************************************************/
struct svarfunction * svare_find_user_function(
    struct svarexec * svx,
    struct svarvalue * csvv,    /* Object pointer -- May be NULL */
    struct svarvalue * msvv,    /* Method pointer -- May be NULL if csvv is NULL */
    const char * funcnam)
{
    struct svarfunction * svf;
    struct svarvalue * fsvv;

    svf = NULL;

    fsvv = svare_cascading_find_variable(svx, csvv, funcnam);
    if (fsvv && fsvv->svv_dtype == SVV_DTYPE_USER_FUNCTION) {
        svf = fsvv->svv_val.svv_svf;
    }

    return (svf);
}
/***************************************************************/
static int svare_exec_block(struct svarexec * svx, struct svartoken * svt);

int svare_call_user_function(struct svarexec * svx,
        struct svarfunction * svf,
        int nsvv,
        struct svarvalue ** asvv,
        struct svarvalue *  rsvv)
{
    int svstat = 0;
    struct svarrec      * svrec;
    int ii;
    struct svartoken * svt;
    struct svarvalue * rtn_svv;

    svrec      = svar_new();
    svare_push_vars(svx, svrec);

    /* set parameters */
    if (!svf->svf_formals) {
        if (nsvv) {
            svstat = svare_set_error(svx, SVARERR_TOO_MANY_PARMS,
                "Function %s requires %d arguments, but %d were supplied.",
                svf->svf_funcname, 0, nsvv);
        }
    } else if (!svstat && nsvv < svf->svf_formals->svfp_nfp) {
        svstat = svare_set_error(svx, SVARERR_TOO_MANY_PARMS,
            "Function %s requires %d arguments, but only %d were supplied.",
            svf->svf_funcname, svf->svf_formals->svfp_nfp, nsvv);
    } else {
        ii = 0;
        while (!svstat && ii < nsvv) {
            if (ii < svf->svf_formals->svfp_nfp) {
                svstat = svare_set_svarvalue(svx,
                    svf->svf_formals->svfp_afp[ii], asvv[ii]);
            } else {
                svstat = svare_set_error(svx, SVARERR_TOO_MANY_PARMS,
                    "Function %s requires %d arguments, but %d were supplied.",
                    svf->svf_funcname, svf->svf_formals->svfp_nfp, nsvv);
            }
            ii++;
        }
    }

    /* strlist_print(svf->svf_body->svts_strl); */

    if (!svstat) svstat = svare_push_include(svx, svf->svf_body->svts_strl, 0);
    if (!svstat) svstat = svare_push_xstat(svx, XSTAT_FUNCTION);

    svt = svar_new_svartoken();
    if (!svstat) svstat = svare_get_token(svx, svt);

    if (!svstat) {
        svstat = svare_exec_block(svx, svt);
        if (svstat == SVARERR_RETURN) {
            svstat = 0;
            if (svx->svx_svs[svx->svx_nsvs - 1]->svs_xstate == XSTAT_RETURN) {
                svstat = svare_pop_xstat(svx, 0);
            }
            if (rsvv->svv_dtype != SVV_DTYPE_NO_VALUE) {    /* if not a call statement */
                rtn_svv = svare_find_variable(svx, NULL, RETURN_VARIABLE_NAME);
                if (rtn_svv) {
                    svstat = svare_store_svarvalue(svx, rsvv, rtn_svv);
                }
            }
        }

        if (svx->svx_svs[svx->svx_nsvs - 1]->svs_xstate == XSTAT_FUNCTION) {
            if (!svstat) svstat = svare_pop_xstat(svx, XSTAT_FUNCTION);
        }
        if (!svstat) svstat = svare_pop_include(svx);
        svare_pop_vars(svx);
    }
    svar_free_svartoken(svt);

    return (svstat);
}
/***************************************************************/
static int svare_eval_function(struct svarexec * svx,
    struct svarvalue * csvv,    /* Object pointer -- May be NULL */
    struct svartoken * svt,
    struct svarvalue * msvv,    /* NULL unless csvv is a class */
    struct svarvalue * svv)
{
    int svstat = 0;
    int done;
    int nsvv;
    char * funcnam;
    struct svarvalue ** asvv;
    SVAR_INTERNAL_FUNCTION ifuncptr;
    struct svarfunction * svf;

    funcnam = Strdup(svt->svt_text);
    svstat = svare_get_token(svx, svt);  /* always ( */
    nsvv = 0;
    asvv = NULL;
    ifuncptr = NULL;
    svf = NULL;

    if (csvv) {
        asvv = Realloc(asvv, struct svarvalue *, nsvv + 1);
        /* csvv is an object or class */
        asvv[nsvv] = New(struct svarvalue, 1);
        memcpy(asvv[nsvv], csvv, sizeof(struct svarvalue));
        nsvv++;
    }

    done = 0;
    svstat = svare_get_token(svx, svt);
    if (svt->svt_text[0] == ')') {
        done = 1;
        svstat = svare_get_token(svx, svt); /* 09/28/2017 */
    }

    while (!svstat && !done) {
        asvv = Realloc(asvv, struct svarvalue *, nsvv + 1);
        asvv[nsvv] = New(struct svarvalue, 1);
        svstat = svare_eval(svx, csvv, svt, asvv[nsvv]);
        if (!svstat) {
            nsvv++;
            if (svt->svt_text[0] == ',') {
                svstat = svare_get_token(svx, svt);
            } else if (svt->svt_text[0] == ')') {
                done = 1;
                svstat = svare_get_token(svx, svt);
            } else if (! SVAR_STMT_TERM_CHAR(svt->svt_text[0])) {
                svstat = svare_set_error(svx, SVARERR_BAD_DELIMITER,
                    "Illegal delimiter in function call: \'%s\'",
                    svt->svt_text);
            }
        }
    }

    if (!svstat) {
        svf = svare_find_user_function(svx, csvv, msvv, funcnam);
        if (svf) {
            svstat = svare_call_user_function(svx, svf, nsvv, asvv, svv);
        } else {
            ifuncptr = svare_find_function(svx, csvv, msvv, funcnam);
            if (ifuncptr) {
                svstat = (ifuncptr)(svx, funcnam, nsvv, asvv, svv);
            } else {
                svstat = svare_set_error(svx, SVARERR_BAD_FUNCTION,
                    "Unrecognized function: \'%s\'", funcnam);
            }
        }
    }

    while (nsvv > 0) {
        nsvv--;
        svar_free_svarvalue(asvv[nsvv]); /* funcfree */
    }
    Free(asvv);

    Free(funcnam);

    return (svstat);
}
/***************************************************************/
static int svare_predefined_constant(struct svarexec * svx,
    const char * vnam,
    struct svarvalue * svv)
{
    int is_predef = 0;

    if (!strcmp(vnam, "false")) {
        svare_svarvalue_bool(svv, 0);
        is_predef = 1;
    } else if (!strcmp(vnam, "true")) {
        svare_svarvalue_bool(svv, 1);
        is_predef = 1;
    } else if (!strcmp(vnam, "null")) {
        svare_store_ptr(svv, NULL, 0, NULL, NULL);
        is_predef = 1;
    }

    return (is_predef);
}
/***************************************************************/
static int svare_eval_primary(struct svarexec * svx,
    struct svarvalue * csvv,
    struct svartoken * svt,
    struct svarvalue * svv)
{
    int svstat = 0;
    struct svarvalue * vsvv;
    struct svarvalue * msvv;
    char svchar;

    if (svt->svt_text[0] == '\"' ) {
        svare_store_string(svv,
            svt->svt_text + 1, svt->svt_tlen - 2);
        svstat = svare_get_token(svx, svt);
    } else if (isdigit(svt->svt_text[0]) ||
               (svt->svt_text[0] == '.' && isdigit(svt->svt_text[1]))) {
        svstat = svare_store_number(svx, svv, svt->svt_text);
        if (!svstat) {
            svstat = svare_get_token(svx, svt);
        }
    } else if (svt->svt_text[0] == '(') {
        svstat = svare_get_token(svx, svt);
        if (!svstat) {
            svstat = svare_eval(svx, csvv, svt, svv);
        }
        if (!svstat) {
            if (svt->svt_text[0] == ')') {
                svstat = svare_get_token(svx, svt);
            } else {
                svstat = svare_set_error(svx, SVARERR_EXP_CLOSE_PAREN,
                    "Expecting closing parenthesis. Found: %s", svt->svt_text);
            }
        } else if (svstat == SVARERR_EOF) {
            svstat = svare_set_error(svx, SVARERR_EXP_CLOSE_PAREN,
                "Expecting closing parenthesis. Found end");
        }
    } else {
        svchar = svare_skip_spaces(svx);
        if (svchar == '(') {
            msvv = NULL;
            if (!svstat) {
                svstat = svare_eval_function(svx, csvv, svt, msvv, svv);
            }
        } else {
            svx->svx_svi->svi_pc.svc_adv = 0;
            if (svare_predefined_constant(svx, svt->svt_text, svv)) {
                svstat = svare_get_token(svx, svt);
            } else {
                vsvv = svare_find_variable(svx, csvv, svt->svt_text);
                if (!vsvv) vsvv = svare_find_class(svx, svt->svt_text);
                if (!vsvv) {
                    svstat = svare_set_error(svx, SVARERR_UNDEFINED_VARIABLE,
                        "Undefined variable: %s", svt->svt_text);
                } else {
                    svstat = svare_store_svarvalue(svx, svv, vsvv);
                    if (!svstat) {
                        svstat = svare_get_token(svx, svt);
                    }
                }
            }
        }
    }

    return (svstat);
}
/***************************************************************/
static int svare_eval_object(struct svarexec * svx,
    struct svarvalue * csvv,
    struct svartoken * svt,
    struct svarvalue * svv)
{
    int svstat = 0;
    struct svarvalue svv2;

    if (svv->svv_dtype == SVV_DTYPE_INT_OBJECT) {
        INIT_SVARVALUE(&svv2);
        svstat = svare_get_token(svx, svt);
        if (!svstat) {
            svstat = svare_eval_primary(svx, svv, svt, &svv2);
            if (!svstat) {
                COPY_SVARVALUE(svv, &svv2);
            }
        }
    } else if (svv->svv_dtype == SVV_DTYPE_INT_CLASS) {
        INIT_SVARVALUE(&svv2);
        svstat = svare_get_token(svx, svt);
        if (!svstat) {
            svstat = svare_eval_primary(svx, svv, svt, &svv2);
            if (!svstat) {
                COPY_SVARVALUE(svv, &svv2);
            }
        }
    } else {
        svstat = svare_set_error(svx, SVARERR_BAD_DOT,
            "Dot may only be used with an object");
    }

    return (svstat);
}
/***************************************************************/
static int svar_get_array_variable(struct svarexec * svx,
        struct svarvalue_array * sva,
        int aindex,
        struct svarvalue * asvv)
{
    int svstat = 0;

    if (aindex < 0 || aindex >= sva->sva_nelement) {
        svstat = svare_set_error(svx, SVARERR_ARRAY_INDEX_BOUNDS,
            "Array index %d is lout of bounds. Maximum is %d",
                aindex, sva->sva_nelement - 1);
    } else {
        svstat = svare_store_svarvalue(svx, asvv, sva->sva_aelement[aindex]);
        sva->sva_nrefs -= 1;
    }

    return (svstat);
}
/***************************************************************/
static int svare_eval_array(struct svarexec * svx,
    struct svarvalue * csvv,
    struct svartoken * svt,
    struct svarvalue * svv)
{
    int svstat = 0;
    struct svarvalue svv2;
    int ixval;
    struct svarvalue isvv;

    if (svv->svv_dtype != SVV_DTYPE_ARRAY) {
        svstat = svare_set_error(svx, SVARERR_BAD_BRACKET,
            "Bracket may only be used with an array");
    } else {
        svstat = svare_get_token(svx, svt);
        if (!svstat) {
            INIT_SVARVALUE(&isvv);
            svstat = svare_eval(svx, csvv, svt, &isvv);
        }
        if (!svstat) {
             svstat = svar_get_int_variable(svx, &isvv, &ixval);
            if (!svstat) {
                INIT_SVARVALUE(&svv2);
                svstat = svar_get_array_variable(svx, svv->svv_val.svv_array, ixval, &svv2);
            }
            if (!svstat) {
                COPY_SVARVALUE(svv, &svv2);
            }
        }
        if (!svstat) {
            if (svt->svt_text[0] != ']') {
                svstat = svare_set_error(svx, SVARERR_EXP_CLOSE_BRACKET,
                    "Expecting close bracket (]). Found: %s", svt->svt_text);
            } else {
                svstat = svare_get_token(svx, svt);
            }
        }
    }

    return (svstat);
}
/***************************************************************/
static int svare_eval_dot(struct svarexec * svx,
    struct svarvalue * csvv,
    struct svartoken * svt,
    struct svarvalue * svv)
{
    int svstat = 0;

    svstat = svare_eval_primary(svx, csvv, svt, svv);

    while (!svstat &&
        (svt->svt_text[0] == '['  || !strcmp(svt->svt_text, "."))) {
        if (svt->svt_text[0] == '[') {
            svstat = svare_eval_array(svx, csvv, svt, svv);
        } else {
            svstat = svare_eval_object(svx, csvv, svt, svv);
        }
    }

    return (svstat);
}
/***************************************************************/
static int svare_eval_unop(struct svarexec * svx,
    struct svarvalue * csvv,
    struct svartoken * svt,
    struct svarvalue * svv)
{
    int svstat = 0;
    char op[4];

    if (svt->svt_text[0] == '-'  ||
        !strcmp(svt->svt_text, "not")) {
        op[0] = svt->svt_text[0];
        op[1] = '\0';
        strcpy(op, svt->svt_text);
        svstat = svare_get_token(svx, svt);
        if (!svstat) {
            svstat = svare_eval_unop(svx, csvv, svt, svv);
        }
        if (!svstat) {
            if (op[0] == '-') {
                svstat = svare_unop_neg(svx, svv);
            } else if (!strcmp(op, "not")) {
                svstat = svare_unop_not(svx, svv);
            }
        }
    } else if (!strcmp(svt->svt_text, "new")) {
        svstat = svare_get_token(svx, svt);
        if (!svstat) {
            svstat = svare_unop_new(svx, csvv, svt, svv);
        }
    } else {
        svstat = svare_eval_dot(svx, csvv, svt, svv);
    }

    return (svstat);
}
/***************************************************************/
static int svare_eval_mulop(struct svarexec * svx,
    struct svarvalue * csvv,
    struct svartoken * svt,
    struct svarvalue * svv)
{
    int svstat = 0;
    struct svarvalue svv2;
    char op[4];

    svstat = svare_eval_unop(svx, csvv, svt, svv);
    while (!svstat &&
        (svt->svt_text[0] == '*' ||
         svt->svt_text[0] == '/' ) ) {
        memset(&svv2, 0, sizeof(struct svarvalue));
        op[0] = svt->svt_text[0];
        op[1] = '\0';
        svstat = svare_get_token(svx, svt);
        if (!svstat) {
            svstat = svare_eval_unop(svx, csvv, svt, &svv2);
        }
        if (!svstat) {
            if (op[0] == '/') {
                svstat = svare_binop_div(svx, svv, &svv2);
            } else {
                svstat = svare_binop_mul(svx, svv, &svv2);
            }
        }
    }

    return (svstat);
}
/***************************************************************/
static int svare_eval_addop(struct svarexec * svx,
    struct svarvalue * csvv,
    struct svartoken * svt,
    struct svarvalue * svv)
{
    int svstat = 0;
    struct svarvalue svv2;
    char op[4];

    svstat = svare_eval_mulop(svx, csvv, svt, svv);
    while (!svstat &&
        (svt->svt_text[0] == '+' ||
         svt->svt_text[0] == '-' ||
         svt->svt_text[0] == '&' ) ) {
        memset(&svv2, 0, sizeof(struct svarvalue));
        op[0] = svt->svt_text[0];
        op[1] = '\0';
        svstat = svare_get_token(svx, svt);
        if (!svstat) {
            svstat = svare_eval_mulop(svx, csvv, svt, &svv2);
        }
        if (!svstat) {
            if (op[0] == '-') {
                svstat = svare_binop_sub(svx, svv, &svv2);
            } else if (op[0] == '&') {
                svstat = svare_binop_concat(svx, svv, &svv2);
            } else {
                svstat = svare_binop_add(svx, svv, &svv2);
            }
        }
    }

    return (svstat);
}
/***************************************************************/
static int svare_compare(struct svarexec * svx,
    struct svarvalue * svv1,
    struct svarvalue * svv2,
    int * p_ans)
{
    int svstat = 0;
    int32   val2_int;
    int64   val2_long;
    double  val2_double;

    (*p_ans) = 0;

    switch (svv1->svv_dtype) {
        case SVV_DTYPE_BOOL  :
            if (svv2->svv_dtype != SVV_DTYPE_BOOL) {
                svstat = svare_set_error(svx, SVARERR_OPERATOR_TYPE_MISMATCH,
                    "Illegal boolean compare");
            } else {
                if (svv1->svv_val.svv_val_bool <
                    svv2->svv_val.svv_val_bool) {
                    (*p_ans) = -1;
                } else if (svv1->svv_val.svv_val_bool >
                    svv2->svv_val.svv_val_bool) {
                    (*p_ans) = 1;
                }
            }
            break;
        case SVV_DTYPE_INT   :
            if (svv2->svv_dtype == SVV_DTYPE_INT) {
                val2_int = svv2->svv_val.svv_val_int;
            } else if (svv2->svv_dtype == SVV_DTYPE_LONG) {
                val2_int = (int32)svv2->svv_val.svv_val_long;
            } else if (svv2->svv_dtype == SVV_DTYPE_DOUBLE) {
                val2_int = (int32)floor(svv2->svv_val.svv_val_double);
            } else {
                svstat = svare_set_error(svx, SVARERR_OPERATOR_TYPE_MISMATCH,
                    "Illegal integer compare");
            }

            if (!svstat) {
                if (svv1->svv_val.svv_val_int < val2_int) {
                    (*p_ans) = -1;
                } else if (svv1->svv_val.svv_val_int > val2_int) {
                    (*p_ans) = 1;
                }
            }
            break;
        case SVV_DTYPE_LONG  :
            if (svv2->svv_dtype == SVV_DTYPE_INT) {
                val2_long = (int64)svv2->svv_val.svv_val_int;
            } else if (svv2->svv_dtype == SVV_DTYPE_LONG) {
                val2_long = svv2->svv_val.svv_val_long;
            } else if (svv2->svv_dtype == SVV_DTYPE_DOUBLE) {
                val2_long = (int64)floor(svv2->svv_val.svv_val_double);
            } else {
                svstat = svare_set_error(svx, SVARERR_OPERATOR_TYPE_MISMATCH,
                    "Illegal long compare");
            }

            if (!svstat) {
                if (svv1->svv_val.svv_val_long < val2_long) {
                    (*p_ans) = -1;
                } else if (svv1->svv_val.svv_val_long > val2_long) {
                    (*p_ans) = 1;
                }
            }
            break;
        case SVV_DTYPE_DOUBLE:
            if (svv2->svv_dtype == SVV_DTYPE_INT) {
                val2_double = (double)svv2->svv_val.svv_val_int;
            } else if (svv2->svv_dtype == SVV_DTYPE_LONG) {
                val2_double = (double)svv2->svv_val.svv_val_long;
            } else if (svv2->svv_dtype == SVV_DTYPE_DOUBLE) {
                val2_double = svv2->svv_val.svv_val_double;
            } else {
                svstat = svare_set_error(svx, SVARERR_OPERATOR_TYPE_MISMATCH,
                    "Illegal double compare");
            }

            if (!svstat) {
                if (svv1->svv_val.svv_val_double < val2_double) {
                    (*p_ans) = -1;
                } else if (svv1->svv_val.svv_val_double > val2_double) {
                    (*p_ans) = 1;
                }
            }
            break;

        case SVV_DTYPE_CHARS :
            if (svv2->svv_dtype != SVV_DTYPE_CHARS) {
                svstat = svare_set_error(svx, SVARERR_OPERATOR_TYPE_MISMATCH,
                    "Illegal string compare");
            } else {
                (*p_ans) = strcmp(
                    svv1->svv_val.svv_chars.svvc_val_chars,
                    svv2->svv_val.svv_chars.svvc_val_chars);
            }
            break;

        case SVV_DTYPE_PTR   :
            if (svv2->svv_dtype != SVV_DTYPE_PTR) {
                svstat = svare_set_error(svx, SVARERR_OPERATOR_TYPE_MISMATCH,
                    "Illegal object compare");
            } else {
                if (svv1->svv_val.svv_ptr.svvp_val_ptr) {
                    if (svv2->svv_val.svv_ptr.svvp_val_ptr) {
                        svstat = svare_set_error(svx,
                            SVARERR_OPERATOR_TYPE_MISMATCH,
                            "Object compare must be against null");
                    } else {
                        (*p_ans) = -1;
                    }
                } else {
                    if (svv2->svv_val.svv_ptr.svvp_val_ptr) {
                        (*p_ans) = 1;
                    }
                }
            }
            break;

        default:
            svstat = svare_set_error(svx, SVARERR_OPERATOR_TYPE_MISMATCH,
                "Illegal compare");
            break;
    }


    return (svstat);
}
/***************************************************************/
static int svare_eval_relop(struct svarexec * svx,
    struct svarvalue * csvv,
    struct svartoken * svt,
    struct svarvalue * svv)
{
    int svstat = 0;
    struct svarvalue svv2;
    int ans;
    char op[4];

    svstat = svare_eval_addop(svx, csvv, svt, svv);
    while (!svstat &&
        (svt->svt_text[0] == '=' ||
         svt->svt_text[0] == '>' ||
         svt->svt_text[0] == '<' ) ) {
        memset(&svv2, 0, sizeof(struct svarvalue));
        op[0] = svt->svt_text[0];
        op[1] = svt->svt_text[1];
        op[2] = '\0';
        svstat = svare_get_token(svx, svt);
        if (!svstat) {
            svstat = svare_eval_addop(svx, csvv, svt, &svv2);
        }
        if (!svstat) {
            svstat = svare_compare(svx, svv, &svv2, &ans);
            svar_free_svarvalue_data(&svv2);
        }
        if (!svstat) {
            switch (op[0]) {
                case '=':
                    if (!ans) svare_svarvalue_bool(svv, 1);
                    else svare_svarvalue_bool(svv, 0);
                    break;
                case '<':
                    if (ans < 0 || (!ans && op[1] == '=')) {
                        svare_svarvalue_bool(svv, 1);
                    } else if (ans > 0 && op[1] == '>') {
                        svare_svarvalue_bool(svv, 1);
                    } else {
                        svare_svarvalue_bool(svv, 0);
                    }
                    break;
                case '>':
                    if (ans > 0 || (!ans && op[1] == '0')) {
                        svare_svarvalue_bool(svv, 1);
                    } else {
                        svare_svarvalue_bool(svv, 0);
                    }
                    break;
                default:
                    break;
            }
        }
    }

    return (svstat);
}
/***************************************************************/
static int svare_eval_and(struct svarexec * svx,
    struct svarvalue * csvv,
    struct svartoken * svt,
    struct svarvalue * svv)
{
    int svstat = 0;
    struct svarvalue svv2;
    int is_false;

    is_false = 0;
    svstat = svare_eval_relop(svx, csvv, svt, svv);

    while (!svstat &&
        !strcmp(svt->svt_text, "and")) {
        if (!svstat && !is_false) {
            if (svv->svv_dtype != SVV_DTYPE_BOOL) {
                svstat = svare_set_error(svx, SVARERR_EXPECT_BOOLEAN,
                    "Expecting boolean value for and operand 1.");
            } else {
                is_false = ! svv->svv_val.svv_val_bool;
            }
        }

        if (!svstat) {
            if (is_false) {
                svstat = svare_get_token(svx, svt);
                if (!svstat) svstat = svare_inactive_binop(svx, svt);
            } else {
                INIT_SVARVALUE(&svv2);
                svstat = svare_get_token(svx, svt);
                if (!svstat) {
                    svstat = svare_eval_relop(svx, csvv, svt, &svv2);
                    if (!svstat) {
                        svare_copy_svarvalue(svv, &svv2);
                        if (svv->svv_dtype != SVV_DTYPE_BOOL) {
                            svstat = svare_set_error(svx, SVARERR_EXPECT_BOOLEAN,
                            "Expecting boolean value for and operand 2.");
                        } else {
                            is_false = ! svv->svv_val.svv_val_bool;
                        }
                    }
                }
            }
        }
    }

    return (svstat);
}
/***************************************************************/
static int svare_eval_or(struct svarexec * svx,
    struct svarvalue * csvv,
    struct svartoken * svt,
    struct svarvalue * svv)
{
    int svstat = 0;
    struct svarvalue svv2;
    int is_true;

    is_true = 0;
    svstat = svare_eval_and(svx, csvv, svt, svv);

    while (!svstat &&
        !strcmp(svt->svt_text, "or")) {
        if (!svstat && !is_true) {
            if (svv->svv_dtype != SVV_DTYPE_BOOL) {
                svstat = svare_set_error(svx, SVARERR_EXPECT_BOOLEAN,
                    "Expecting boolean value for or operand 1.");
            } else {
                is_true = svv->svv_val.svv_val_bool;
            }
        }

        if (!svstat) {
            if (is_true) {
                svstat = svare_get_token(svx, svt);
                if (!svstat) svstat = svare_inactive_and(svx, svt);
            } else {
                INIT_SVARVALUE(&svv2);
                svstat = svare_get_token(svx, svt);
                if (!svstat) {
                    svstat = svare_eval_and(svx, csvv, svt, &svv2);
                    if (!svstat) {
                        svare_copy_svarvalue(svv, &svv2);
                        if (svv->svv_dtype != SVV_DTYPE_BOOL) {
                            svstat = svare_set_error(svx, SVARERR_EXPECT_BOOLEAN,
                            "Expecting boolean value for or operand 2.");
                        } else {
                            is_true = svv->svv_val.svv_val_bool;
                        }
                    }
                }
            }
        }
    }

    return (svstat);
}
/***************************************************************/
static int svare_eval(struct svarexec * svx,
    struct svarvalue * csvv,
    struct svartoken * svt,
    struct svarvalue * svv)
{
    int svstat = 0;

    svstat = svare_eval_or(svx, csvv, svt, svv);

    return (svstat);
}
/***************************************************************/
int svare_evaluate(struct svarexec * svx,
    struct svartoken * svt,
    struct svarvalue * svv)
{
    int svstat = 0;

    INIT_SVARVALUE(svv);
    svstat = svare_eval(svx, NULL, svt, svv);

    return (svstat);
}
/***************************************************************/
static int svare_inactive_eval_function(struct svarexec * svx,
    struct svartoken * svt)
{
    int svstat = 0;
    int done;

    svstat = svare_get_token(svx, svt);  /* always ( */

    done = 0;
    svstat = svare_get_token(svx, svt);
    if (svt->svt_text[0] == ')') {
        done = 1;
        svstat = svare_get_token(svx, svt); 
    }

    while (!svstat && !done) {
        svstat = svare_inactive_eval(svx, svt);
        if (!svstat) {
            if (svt->svt_text[0] == ',') {
                svstat = svare_get_token(svx, svt);
            } else if (svt->svt_text[0] == ')') {
                done = 1;
                svstat = svare_get_token(svx, svt);
            } else if (! SVAR_STMT_TERM_CHAR(svt->svt_text[0])) {
                svstat = svare_set_error(svx, SVARERR_BAD_DELIMITER,
                    "Illegal delimiter in function call: \'%s\'",
                    svt->svt_text);
            }
        }
    }

    return (svstat);
}
/***************************************************************/
static int svare_inactive_primary(struct svarexec * svx,
    struct svartoken * svt)
{
    int svstat = 0;
    char svchar;

    if (svt->svt_text[0] == '\"' ) {
        svstat = svare_get_token(svx, svt);
    } else if (isdigit(svt->svt_text[0]) ||
               (svt->svt_text[0] == '.' && isdigit(svt->svt_text[1]))) {
        svstat = svare_get_token(svx, svt);
    } else if (svt->svt_text[0] == '(') {
        svstat = svare_get_token(svx, svt);
        if (!svstat) {
            svstat = svare_inactive_eval(svx, svt);
        }
        if (!svstat) {
            if (svt->svt_text[0] == ')') {
                svstat = svare_get_token(svx, svt);
            } else {
                svstat = svare_set_error(svx, SVARERR_EXP_CLOSE_PAREN,
                    "Expecting closing parenthesis. Found: %s", svt->svt_text);
            }
        } else if (svstat == SVARERR_EOF) {
            svstat = svare_set_error(svx, SVARERR_EXP_CLOSE_PAREN,
                "Expecting closing parenthesis. Found end");
        }
    } else {
        svchar = svare_skip_spaces(svx);
        if (svchar == '(') {
            svstat = svare_inactive_eval_function(svx, svt);
        } else {
            svstat = svare_get_token(svx, svt);
        }
    }

    return (svstat);
}
/***************************************************************/
static int svare_inactive_array(struct svarexec * svx,
    struct svartoken * svt)
{
    int svstat = 0;

    svstat = svare_get_token(svx, svt);
    if (!svstat) {
        svstat = svare_inactive_eval(svx, svt);
    }
    if (!svstat) {
        if (svt->svt_text[0] != ']') {
            svstat = svare_set_error(svx, SVARERR_EXP_CLOSE_BRACKET,
                "Expecting close bracket (]). Found: %s", svt->svt_text);
        } else {
            svstat = svare_get_token(svx, svt);
        }
    }

    return (svstat);
}
/***************************************************************/
static int svare_inactive_dot(struct svarexec * svx,
    struct svartoken * svt)
{
    int svstat = 0;

    svstat = svare_inactive_primary(svx, svt);

    while (!svstat &&
        (svt->svt_text[0] == '['  || !strcmp(svt->svt_text, "."))) {
        if (svt->svt_text[0] == '[') {
            svstat = svare_inactive_array(svx, svt);
        } else {
            svstat = svare_get_token(svx, svt);
            if (!svstat) {
                svstat = svare_inactive_primary(svx, svt);
            }
        }
    }

    return (svstat);
}
/***************************************************************/
static int svare_inactive_unop(struct svarexec * svx,
    struct svartoken * svt)
{
    int svstat = 0;

    if (svt->svt_text[0] == '-'  ||
        !strcmp(svt->svt_text, "not")) {
        svstat = svare_get_token(svx, svt);
        if (!svstat) {
            svstat = svare_inactive_unop(svx, svt);
        }
    } else if (!strcmp(svt->svt_text, "new")) {
        svstat = svare_get_token(svx, svt);
    } else {
        svstat = svare_inactive_dot(svx, svt);
    }

    return (svstat);
}
/***************************************************************/
static int svare_inactive_binop(struct svarexec * svx,
    struct svartoken * svt)
{
    int svstat = 0;

    svstat = svare_inactive_unop(svx, svt);

    while (!svstat &&
        svt->svt_text[0] == '='         ||
        svt->svt_text[0] == '>'         ||
        svt->svt_text[0] == '<'         ||
        svt->svt_text[0] == '+'         ||
        svt->svt_text[0] == '-'         ||
        svt->svt_text[0] == '&'         ||
        svt->svt_text[0] == '*'         ||
        svt->svt_text[0] == '/'         ) {

        svstat = svare_get_token(svx, svt);
        if (!svstat) {
            svstat = svare_inactive_unop(svx, svt);
        }
    }

    return (svstat);
}
/***************************************************************/
static int svare_inactive_and(struct svarexec * svx,
    struct svartoken * svt)
{
    int svstat = 0;

    svstat = svare_inactive_binop(svx, svt);

    while (!svstat && !strcmp(svt->svt_text, "and")) {

        svstat = svare_get_token(svx, svt);
        if (!svstat) {
            svstat = svare_inactive_binop(svx, svt);
        }
    }

    return (svstat);
}
/***************************************************************/
static int svare_inactive_or(struct svarexec * svx,
    struct svartoken * svt)
{
    int svstat = 0;

    svstat = svare_inactive_and(svx, svt);

    while (!svstat && !strcmp(svt->svt_text, "or")) {

        svstat = svare_get_token(svx, svt);
        if (!svstat) {
            svstat = svare_inactive_and(svx, svt);
        }
    }

    return (svstat);
}
/***************************************************************/
static int svare_inactive_eval(struct svarexec * svx,
    struct svartoken * svt)
{
    int svstat = 0;

    svstat = svare_inactive_or(svx, svt);

    return (svstat);
}
/***************************************************************/
static int svare_evaluate_inactive(struct svarexec * svx,
    struct svartoken * svt)
{
    int svstat = 0;

#if 1
    svstat = svare_inactive_eval(svx, svt);
#else
    struct svarvalue svv;
    INIT_SVARVALUE(&svv);
    svstat = svare_eval(svx, NULL, svt, &svv, 1);
#endif

    return (svstat);
}
/***************************************************************/
static char * svare_int_object_to_string(struct svarexec * svx,
                           struct svarvalue * svv,
                                int dispflags)
{
    int svstat = 0;
    char * strbuf;
    struct svarvalue * msvv;
    struct svarvalue   rsvv;
    struct svarvalue * asvv[1];
    char funcnam[16];

    strbuf = NULL;
    strcpy(funcnam, "toString");

    msvv = svare_find_int_member(svv->svv_val.svv_svvi.svvi_int_class, funcnam);
    if (!msvv) {
        svstat = svare_set_error(svx, SVARERR_CANNOT_FIND_MEMBER,
            "Cannot find toString method");
    } else if (msvv->svv_dtype != SVV_DTYPE_METHOD) {
        svstat = svare_set_error(svx, SVARERR_MEMBER_NOT_METHOD,
            "toString is not a method");
    } else {
        INIT_SVARVALUE(&rsvv);
        asvv[0] = svv;
        svstat = (msvv->svv_val.svv_method)(svx, funcnam, 1, asvv, &rsvv);
        if (rsvv.svv_dtype == SVV_DTYPE_CHARS) {
            strbuf = Strdup(rsvv.svv_val.svv_chars.svvc_val_chars);
        }
        svar_free_svarvalue_data(&rsvv);
    }

    return (strbuf);
}
/***************************************************************/
static char * svare_array_to_string(struct svarexec * svx,
                           struct svarvalue * svv,
                                int dispflags)
{
    int svstat = 0;
    char * buf;
    int buflen;
    int bufmax;
    int ii;
    char * vbuf;
    int vlen;
    int edispflags;
    struct svarvalue_array * sva;

    buf    = NULL;
    buflen = 0;
    bufmax = 0;
    edispflags = dispflags | DISPFLAG_QUOTES | DISPFLAG_ESCAPE;
    sva = svv->svv_val.svv_array;

    append_chars(&buf, &buflen, &bufmax, "[", 1);
    for (ii = 0; ii < sva->sva_nelement; ii++) {
        if (ii) append_chars(&buf, &buflen, &bufmax, ",", 1);
        vbuf = svare_svarvalue_to_string(svx, sva->sva_aelement[ii], edispflags);
        if (vbuf) {
            vlen = IStrlen(vbuf);
            append_chars(&buf, &buflen, &bufmax, vbuf,vlen);
            Free(vbuf);
        }
    }
    append_chars(&buf, &buflen, &bufmax, "]", 1);

    return (buf);
}
/***************************************************************/
char * svare_svarvalue_to_string(struct svarexec * svx,
                                struct svarvalue * svv,
                                int dispflags)
{
    char * strbuf;
    char * tmpbuf;

    switch (svv->svv_dtype) {
        case SVV_DTYPE_BOOL  :
            if (svv->svv_val.svv_val_bool) strbuf = Strdup("true");
            else                           strbuf = Strdup("false");
            break;
        case SVV_DTYPE_INT   :
            strbuf = Smprintf("%d", svv->svv_val.svv_val_int);
            break;
        case SVV_DTYPE_LONG  :
            strbuf = Smprintf("%Ld", svv->svv_val.svv_val_long);
            break;
        case SVV_DTYPE_DOUBLE:
            strbuf = Smprintf("%g", svv->svv_val.svv_val_double);
            break;
        case SVV_DTYPE_CHARS :
            if (dispflags & DISPFLAG_QUOTES) {
                strbuf = Smprintf("\"%s\"", svv->svv_val.svv_chars.svvc_val_chars);
            } else {
                strbuf = Smprintf("%s", svv->svv_val.svv_chars.svvc_val_chars);
            }
            if ((dispflags & DISPFLAG_ESCAPE) && any_escapes(strbuf)) {
                tmpbuf = escape_text(strbuf);
                Free(strbuf);
                strbuf = tmpbuf;
            }
            break;
        case SVV_DTYPE_PTR   :
            if (!svv->svv_val.svv_ptr.svvp_val_ptr) {
                strbuf = Strdup("null");
            } else {
                strbuf = Smprintf("0x08x",
                    (int)svv->svv_val.svv_ptr.svvp_val_ptr);
            }
            break;
        case SVV_DTYPE_ARRAY :
            strbuf = svare_array_to_string(svx, svv, dispflags);
            break;
        case SVV_DTYPE_INT_OBJECT :
            strbuf = svare_int_object_to_string(svx, svv, dispflags);
            break;
        default:
            strbuf = NULL;
            break;
    }

    return (strbuf);
}
/***************************************************************/
static int svare_print_nl(struct svarexec * svx)
{
    int svstat = 0;

    printf("\n");

    return (svstat);
}
/***************************************************************/
static int svare_exec_call(struct svarexec * svx, struct svartoken * svt)
{
    int svstat = 0;
    struct svarvalue svv;
    struct svarfunction * svf;

    svv.svv_dtype = SVV_DTYPE_NO_VALUE;

    svstat = svare_eval(svx, NULL, svt, &svv);
    if (!svstat) {
        if (svv.svv_dtype == SVV_DTYPE_USER_FUNCTION) {
            svf = svv.svv_val.svv_svf;
            svstat = svare_set_error(svx, SVARERR_FUNCTION_CALL_NEEDS_PARENS,
                "Function call to %s requires parentheses.", svf->svf_funcname);
        }
    }

    return (svstat);
}
/***************************************************************/
#if 0
static int svare_exec_else(struct svarexec * svx, struct svartoken * svt)
{
    int svstat = 0;

    return (svstat);
}
#endif
/***************************************************************/
static int svare_exec_exit(struct svarexec * svx, struct svartoken * svt)
{
    int svstat;

    if (!SVAR_STMT_TERM_CHAR(svt->svt_text[0])) {
        svstat = svare_set_error(svx, SVARERR_EXP_STMT_TERMINATOR,
            "Expecting statement separator on %s. Found: %s",
            "exit", svt->svt_text);
    } else {
        svstat = SVARERR_EXIT;
    }

    return (svstat);
}
/***************************************************************/
static int read_include_file(struct frec * include_fr,
    struct strlist * strl)
{
    int is_err = 0;
    int ok;
    int done;
    int ix;
    int linenum;
    char includebuf[1024];

    linenum = 0;
    done = 0;
    while (!is_err && !done) {
        ok = frec_gets(includebuf, sizeof(includebuf), include_fr);
        if (ok < 0) {
            done = 1;
            if (!include_fr->feof) {
                is_err = 1;
            }
        } else {
            ix = 0;
            if (!linenum) {
                /* Check for UTF-8 */
                if (includebuf[ix]     == '\xef' &&
                    includebuf[ix + 1] == '\xbb' &&
                    includebuf[ix + 2] == '\xbf') {
                    ix += 3;
                }
            }
            linenum++;
            while (isspace(includebuf[ix])) ix++;
            if (includebuf[ix] && (includebuf[ix] != '/' || includebuf[ix + 1] != '/')) {
                if (!strcmp(includebuf,":EOF:")) {
                    done = 1;   /* Simulate EOF with :EOF: */
                } else {
                    strlist_append_str(strl, includebuf + ix);
                }
            }
        }
    }

    frec_close(include_fr);

    return (is_err);
}
/***************************************************************/
static int svare_read_include_file(struct svarexec * svx,
    const char * include_fname,
    struct strlist * strl)
{
    int svstat = 0;
    int is_err;
    struct frec * include_fr;

    include_fr = frec_open(include_fname, "r", "include file");
    if (!include_fr) {
        svstat = svare_set_error(svx, SVARERR_OPEN_INCLUDE,
            "Error opening include file for input. File: %s",
            include_fname);
        return (svstat);
    }

    is_err = read_include_file(include_fr, strl);
    if (is_err) {
        svstat = svare_set_error(svx, SVARERR_READ_INCLUDE,
            "Error reading from include file. File: %s",
            include_fname);
    }

    return (svstat);
}
/***************************************************************/
static int svare_check_context(struct svarexec * svx, const char * cmdname)
{
    int svstat = 0;

    if (XSTAT_IS_COMPOUND(svx)) {
        svstat = svare_set_error(svx, SVARERR_NOT_PERMITTED_HERE,
            "%s statement not permitted in this context,",
            cmdname);
    }

    return (svstat);
}
/***************************************************************/
static int svare_exec_include(struct svarexec * svx, struct svartoken * svt)
{
    int svstat;
    struct svarvalue svv;
    char * include_fname;
    struct strlist * include_strl;

    svstat = svare_check_context(svx, "include");

    if (!svstat) svstat = svare_evaluate(svx, svt, &svv);
    if (!svstat) {
        include_fname = svare_svarvalue_to_string(svx, &svv, 0);
        svstat = svare_get_svstat(svx);
        if (!svstat) {
            include_strl = strlist_new();
            svstat = svare_read_include_file(svx, include_fname, include_strl);
            if (!svstat) svstat = svare_push_include(svx, include_strl, 1);
            if (!svstat) svstat = svare_push_xstat(svx, XSTAT_INCLUDE_FILE);
            Free(include_fname);
            svar_free_svarvalue_data(&svv);
        }
    }

    if (!svstat) {
        if (!SVAR_STMT_TERM_CHAR(svt->svt_text[0])) {
            svstat = svare_set_error(svx, SVARERR_EXP_STMT_TERMINATOR,
            "Expecting statement separator on %s. Found: %s",
            "include", svt->svt_text);
        }
    }

    return (svstat);
}
/***************************************************************/
static int svare_exec_fail(struct svarexec * svx, struct svartoken * svt)
{
    int svstat = 0;
    struct svarvalue svv;
    char * expstr;

    svstat = svare_evaluate(svx, svt, &svv);
    if (!svstat) {
        expstr = svare_svarvalue_to_string(svx, &svv, 0);
        svstat = svare_get_svstat(svx);
        if (!svstat) {
            svstat = svare_set_error(svx, SVARERR_FAIL,
                "Failure: %s", expstr);
            Free(expstr);
            svar_free_svarvalue_data(&svv);
        }
    }

    return (svstat);
}
/***************************************************************/
static int svare_parse_func_formals(struct svarexec * svx,
        struct svartoken * svt,
        struct svarformalparms * svfp)
{
    int svstat = 0;
    int ii;
    int found;

    while (!svstat && svt->svt_text[0] != ')') {
        if (!svare_good_varname(svt->svt_text)) {
            svstat = svare_set_error(svx, SVARERR_BAD_VARNAME,
                "Illegal variable name: %s", svt->svt_text);
        } else if (svfp) {
            found = 0;
            ii = svfp->svfp_nfp - 1;
            while (!found && ii >= 0) {
                if (!strcmp(svt->svt_text, svfp->svfp_afp[ii])) found = 1;
                else ii--;
            }
            if (found) {
                svstat = svare_set_error(svx, SVARERR_DUPLICATE_FORMAL,
                    "Duplicate formal parameter: %s", svt->svt_text);
            } else {
                svfp->svfp_afp = Realloc(svfp->svfp_afp, char*, svfp->svfp_nfp + 1);
                svfp->svfp_afp[svfp->svfp_nfp] = Strdup(svt->svt_text);
                svfp->svfp_nfp += 1;
            }
        }
        if (!svstat) svstat = svare_get_token_no_save(svx, svt);
        if (!svstat) {
            if (svt->svt_text[0] == ',') {
                svstat = svare_get_token_no_save(svx, svt);
            } else if (svt->svt_text[0] == ')') {
                /* done */
            } else {
                svstat = svare_set_error(svx, SVARERR_EXPECT_COMMA,
                    "Expecting comma. Found: \"%s\"", svt->svt_text);
            }
        }
    }

    if (svstat) {
        free_svarformalparms(svfp);
    }

    return (svstat);
}
/***************************************************************/
static int svare_exec_func(struct svarexec * svx, struct svartoken * svt)
{
    int svstat = 0;
    struct svarvalue * svv;
    struct svarvalue * fsvv;
    char * funcnam;
    struct svarformalparms * svfp;
    struct svarfunction * svf;
    struct strlist * func_body;

    svstat = svare_check_context(svx, "func");  /* not within if or while */
    if (svstat) return (svstat);

    if (!svare_good_varname(svt->svt_text)) {
        svstat = svare_set_error(svx, SVARERR_BAD_VARNAME,
            "Illegal variable name: %s", svt->svt_text);
    } else {
        funcnam = Strdup(svt->svt_text);
        svv = svare_find_variable(svx, NULL, funcnam);
        if (svv) {
            svstat = svare_set_error(svx, SVARERR_NOT_UNIQUE,
                "Function name %s is not unique.", funcnam);
        } else {
            svstat = svare_get_token_no_save(svx, svt);
            if (svt->svt_text[0] != '(') {
                svfp = NULL;
            } else {
                svfp = new_svarformalparms();
                svstat = svare_get_token_no_save(svx, svt);
                if (!svstat) svstat = svare_parse_func_formals(svx, svt, svfp);
                if (!svstat) svstat = svare_get_token_no_save(svx, svt);
            }
            if (!svstat) {
                if (SVAR_BLOCK_BEGIN_CHAR(svt->svt_text[0])) {
                    svare_get_svarglob(svx);    /**** Lock globals ****/
                    svf = new_svarfunction();
                    svf->svf_funcname = funcnam;
                    svf->svf_formals = svfp;
                    func_body = strlist_new();
                    svf->svf_nrefs = 1;
                    fsvv = svare_new_variable();
                    fsvv->svv_dtype = SVV_DTYPE_USER_FUNCTION;
                    fsvv->svv_val.svv_svf = svf;
                    if (!svstat) svstat = svare_add_svarvalue(svx, funcnam, fsvv);
                    if (!svstat) svstat = svare_push_xstat(svx, XSTAT_DEFINE_FUNCTION);
                    if (!svstat) svstat = svare_push_save_tokens(svx, func_body);
                    svf->svf_body = svx->svx_locked_svg->svg_asvts[svx->svx_locked_svg->svg_nsvts - 1];
                    if (!svstat) svstat = svare_get_token(svx, svt);
                    svare_release_svarglob(svx);    /**** Unock globals ****/
                } else if (SVAR_STMT_END_CHAR(svt->svt_text[0])) {
                    /* forward declaration */
                } else {
                    svstat = svare_set_error(svx, SVARERR_EXPECT_BEGIN,
                        "Expecting { to begin function. Found: \"%s\"",
                        svt->svt_text);
                }
            }
        }
    }

    return (svstat);
}
/***************************************************************/
static int svare_exec_print(struct svarexec * svx, struct svartoken * svt)
{
    int svstat = 0;
    struct svarvalue svv;
    char * expstr;
    int nl;

    nl = 1;
#if DEBUG_XSTACK & 2
    printf("svare_exec_print() toke=%-6s, stack=", svt->svt_text);
    svare_print_xstack(svx);
#endif

    while (!svstat && !SVAR_STMT_TERM_CHAR(svt->svt_text[0])) {
        nl = 1;
        svstat = svare_evaluate(svx, svt, &svv);
        if (!svstat) {
            expstr = svare_svarvalue_to_string(svx, &svv, 0);
            svstat = svare_get_svstat(svx);
            if (!svstat) {
                printf("%s", expstr);
                Free(expstr);
                svar_free_svarvalue_data(&svv);
            }
        }
        if (!svstat) {
            if (svt->svt_text[0] == ',') {
                printf(" ");
                svstat = svare_get_token(svx, svt);
                nl = 0;
            } else if (! SVAR_STMT_TERM_CHAR(svt->svt_text[0])) {
                svstat = svare_set_error(svx, SVARERR_BAD_DELIMITER,
                    "Illegal delimiter: \'%s\'", svt->svt_text);
            }
        }
    }
    if (!svstat && nl) svstat = svare_print_nl(svx);

    return (svstat);
}
/***************************************************************/
static int svare_exec_return(struct svarexec * svx, struct svartoken * svt)
{
    int svstat;
    struct svarvalue svv;

    svstat = svare_evaluate(svx, svt, &svv);
    if (!svstat) {
        if (!SVAR_STMT_TERM_CHAR(svt->svt_text[0])) {
            svstat = svare_set_error(svx, SVARERR_EXP_STMT_TERMINATOR,
                "Expecting statement separator on %s. Found: %s",
                "return", svt->svt_text);
        } else {
            svstat = svare_set_svarvalue(svx, RETURN_VARIABLE_NAME, &svv);
            while (svx->svx_nsvs > 1 &&
                   svx->svx_svs[svx->svx_nsvs - 1]->svs_xstate != XSTAT_FUNCTION) {
                svstat = svare_pop_xstat(svx, 0);
            }
            if (!svstat) {
                svstat = svare_push_xstat(svx, XSTAT_RETURN);
                svstat = SVARERR_RETURN;
            }
        }
    }

    return (svstat);
}
/***************************************************************/
static int svar_set_array_size(struct svarexec * svx,
        struct svarvalue_array * sva,
        int array_size)
{
    int svstat = 0;
    int ii;

    if (array_size < 0) {
        svstat = svare_set_error(svx, SVARERR_NEGATIVE_ARRAY_INDEX,
            "Array size is less than zero: %d", array_size);
    } else if (array_size >= MAX_ARRAY_INDEX) {
        svstat = svare_set_error(svx, SVARERR_NEGATIVE_ARRAY_INDEX,
            "Array index of %d exceeds maximum of %d", array_size, MAX_ARRAY_INDEX - 1);
    } else if (sva->sva_nelement > array_size) {
        ii = sva->sva_nelement;
        while (ii > array_size) {
            svar_free_svarvalue(sva->sva_aelement[ii]);
            ii--;
        }
        sva->sva_nelement = array_size;
    } else {
        if (sva->sva_xelement < array_size) {
            if (!sva->sva_xelement) sva->sva_xelement = 8;
            else sva->sva_xelement *= 2;
            if (sva->sva_xelement < array_size) sva->sva_xelement = array_size + 8;
            sva->sva_aelement = Realloc(sva->sva_aelement, struct svarvalue*, sva->sva_xelement);
        }

        if (sva->sva_nelement < array_size) {
            for (ii = sva->sva_nelement; ii < array_size; ii++) {
                sva->sva_aelement[ii] = svare_new_variable();
                sva->sva_aelement[ii]->svv_dtype = SVV_DTYPE_NO_VALUE;
            }
            sva->sva_nelement = array_size;
        }
    }

    return (svstat);
}
/***************************************************************/
static int svar_get_new_array_variable(struct svarexec * svx,
        struct svarvalue_array * sva,
        int aindex,
        struct svarvalue ** asvv)
{
    int svstat = 0;

    (*asvv) = NULL;

    if (aindex < 0) {
        svstat = svare_set_error(svx, SVARERR_NEGATIVE_ARRAY_INDEX,
            "Array index is less than zero: %d", aindex);
    } else if (aindex >= MAX_ARRAY_INDEX) {
        svstat = svare_set_error(svx, SVARERR_NEGATIVE_ARRAY_INDEX,
            "Array index of %d exceeds maximum of %d", aindex, MAX_ARRAY_INDEX - 1);
    } else {
        if (aindex >= sva->sva_nelement) {
            svstat = svar_set_array_size(svx, sva, aindex + 1);
        }
        if (!svstat) (*asvv) = sva->sva_aelement[aindex];
    }

    return (svstat);
}
/***************************************************************/
static int svare_get_lval(struct svarexec * svx, struct svartoken * svt, struct svarvalue ** lsvv)
{
    int svstat = 0;
    char * vnam;
    int ixval;
    struct svarvalue isvv;

    (*lsvv) = NULL;
    vnam    = NULL;

    if (!svare_good_varname(svt->svt_text)) {
        svstat = svare_set_error(svx, SVARERR_BAD_VARNAME,
            "Illegal variable name: %s", svt->svt_text);
    } else {
        vnam = Strdup(svt->svt_text);
        svstat = svare_get_token(svx, svt);
    }

    if (!svstat) {
        (*lsvv) = svare_get_svarvalue(svx, vnam);
    }

    while (!svstat && svt->svt_text[0] == '[') {
        if ((*lsvv)->svv_dtype && (*lsvv)->svv_dtype != SVV_DTYPE_ARRAY) {
            svar_free_svarvalue_data(*lsvv);
        }
        if ((*lsvv)->svv_dtype != SVV_DTYPE_ARRAY) {
            (*lsvv)->svv_dtype = SVV_DTYPE_ARRAY;
            (*lsvv)->svv_val.svv_array = svar_new_svarvalue_array();
            (*lsvv)->svv_val.svv_array->sva_nrefs += 1;
        }
        svstat = svare_get_token(svx, svt);
        if (!svstat) {
            INIT_SVARVALUE(&isvv);
            svstat = svare_evaluate(svx, svt, &isvv);
        }
        if (!svstat) {
            svstat = svar_get_int_variable(svx, &isvv, &ixval);
        }
        if (!svstat) {
            svstat = svar_get_new_array_variable(svx, (*lsvv)->svv_val.svv_array, ixval, lsvv);
        }
        if (!svstat) {
            if (svt->svt_text[0] != ']') {
                svstat = svare_set_error(svx, SVARERR_EXP_CLOSE_BRACKET,
                    "Expecting close bracket (]). Found: %s", svt->svt_text);
            } else {
                svstat = svare_get_token(svx, svt);
            }
        }
    }

    if (!svstat) {
        if (svt->svt_text[0] != '=') {
            svstat = svare_set_error(svx, SVARERR_EXPECT_EQUAL,
                "Expecting equal sign in set. Found: %s", svt->svt_text);
        } else {
            if (!(*lsvv)) (*lsvv) = svare_get_svarvalue(svx, vnam);
            svstat = svare_get_token(svx, svt);
        }
    }
    Free(vnam);

    return (svstat);
}
/***************************************************************/
static int svare_exec_set(struct svarexec * svx, struct svartoken * svt)
{
    int svstat = 0;
    struct svarvalue * lsvv;
    struct svarvalue svv;

    svstat = svare_get_lval(svx, svt, &lsvv);

    if (!svstat) {
        svstat = svare_evaluate(svx, svt, &svv);
    }
    if (!svstat) {
        svare_copy_svarvalue(lsvv, &svv);
    }

    return (svstat);
}
/***************************************************************/
int svare_push_xstat(struct svarexec * svx, int xstat)
{
    int svstat = 0;

    if (svx->svx_nsvs == svx->svx_xsvs) {
        int nsvs = svx->svx_nsvs;
        if (!svx->svx_xsvs) svx->svx_xsvs = 16;
        else svx->svx_xsvs *= 2;
        if (svx->svx_xsvs >= MAX_XSTAT_STACK) svx->svx_xsvs = MAX_XSTAT_STACK;
        if (svx->svx_nsvs == MAX_XSTAT_STACK) {
            svx->svx_xsvs = svx->svx_nsvs;  /* To prevent Free() errors */
            svstat = svare_set_error(svx, SVARERR_RUNTIME_STACK_OVERFLOW,
                "Runtime stack overflow.");
            return (svstat);
        }
        svx->svx_svs = Realloc(svx->svx_svs, struct svarstate*, svx->svx_xsvs);
        while (nsvs < svx->svx_xsvs) {
            svx->svx_svs[nsvs++] = New(struct svarstate, 1);
        }
    }
    svx->svx_svs[svx->svx_nsvs]->svs_xstate = xstat;
    svx->svx_nsvs += 1;

    return (svstat);
}
/***************************************************************/
int svare_pop_xstat(struct svarexec * svx, int old_xstat)
{
    int svstat = 0;

    if (svx->svx_nsvs < 2) {
        svstat = svare_set_error(svx, SVARERR_RUNTIME_STACK_UNDERFLOW,
            "Runtime stack underflow.");
        return (svstat);
    }
    if (svx->svx_svs[svx->svx_nsvs - 1]->svs_xstate == XSTAT_INCLUDE_FILE) {
        svstat = svare_pop_include(svx);
    }
    svx->svx_nsvs -= 1;

    return (svstat);
}
/***************************************************************/
static int svare_pop_xstat_with_restore(struct svarexec * svx, int old_xstat)
{
    int svstat;

    svare_copy_svarcursor(&(svx->svx_svi->svi_pc), &(svx->svx_svs[svx->svx_nsvs - 1]->svs_svc));

    svstat = svare_pop_xstat(svx, old_xstat);

    return (svstat);
}
/***************************************************************/
static int svare_pop_push_xstat(struct svarexec * svx, int old_xstat, int new_xstat)
{
    int svstat = 0;

    if (!svx->svx_nsvs) {
        svstat = svare_set_error(svx, SVARERR_RUNTIME_STACK_UNDERFLOW,
            "Runtime stack underflow.");
        return (svstat);
    }
    svx->svx_svs[svx->svx_nsvs - 1]->svs_xstate = new_xstat;

    return (svstat);
}
/***************************************************************/
static void svare_set_xstat_svc(struct svarexec * svx, struct svarcursor * svc)
{
    svare_copy_svarcursor(&(svx->svx_svs[svx->svx_nsvs - 1]->svs_svc), svc);
}
/***************************************************************/
static int svare_update_xstat(struct svarexec * svx)
{
    int svstat = 0;

    switch (svx->svx_svs[svx->svx_nsvs - 1]->svs_xstate) {
        case XSTAT_TRUE_IF:
            svstat = svare_pop_push_xstat(svx, XSTAT_TRUE_IF, XSTAT_TRUE_IF_COMPLETE);
            break;
        case  XSTAT_FALSE_IF:
            svstat = svare_pop_push_xstat(svx, XSTAT_FALSE_IF, XSTAT_FALSE_IF_COMPLETE);
            break;
        case  XSTAT_TRUE_ELSE:
            svstat = svare_pop_xstat(svx, XSTAT_TRUE_ELSE);
            //svstat = svare_pop_xstat(svx, XSTAT_TRUE_ELSE_COMPLETE); /* 11/05/2019 */
            break;
        case  XSTAT_INACTIVE_IF:
            svstat = svare_pop_push_xstat(svx, XSTAT_INACTIVE_IF, XSTAT_INACTIVE_IF_COMPLETE);
            break;
        case  XSTAT_FALSE_ELSE:
            svstat = svare_pop_xstat(svx, XSTAT_FALSE_ELSE);
            //svstat = svare_pop_xstat(svx, XSTAT_FALSE_ELSE_COMPLETE); /* 11/05/2019 */
            break;
        case  XSTAT_INACTIVE_ELSE:
            svstat = svare_pop_xstat(svx, XSTAT_INACTIVE_ELSE);
            //svstat = svare_pop_push_xstat(svx, XSTAT_INACTIVE_IF, XSTAT_INACTIVE_ELSE_COMPLETE); /* 11/05/2019 */
            break;

        /* while */
        case XSTAT_TRUE_WHILE:
            svstat = svare_pop_xstat_with_restore(svx, XSTAT_TRUE_WHILE);
            break;
        case  XSTAT_INACTIVE_WHILE:
            svstat = svare_pop_xstat(svx, XSTAT_INACTIVE_WHILE);
            break;

        /* only active else */
        case  XSTAT_TRUE_IF_COMPLETE:
            svstat = svare_pop_push_xstat(svx, XSTAT_TRUE_IF_COMPLETE, XSTAT_FALSE_ELSE);
            break;
        case  XSTAT_FALSE_IF_COMPLETE:
            svstat = svare_pop_push_xstat(svx, XSTAT_FALSE_IF_COMPLETE, XSTAT_TRUE_ELSE);
            break;
        case  XSTAT_INACTIVE_IF_COMPLETE:
            svstat = svare_pop_push_xstat(svx, XSTAT_INACTIVE_IF_COMPLETE, XSTAT_INACTIVE_ELSE);
            break;
        default:
            break;
    }

    return (svstat);
}
/***************************************************************/
static int svare_exec_block(struct svarexec * svx, struct svartoken * svt)
{
/*
** Expects: svt->svt_text == '{' if nostart == 0
*/
    int svstat = 0;
    int done;
    int is_active;

    done = 0;

    while (!svstat && !done) {
        is_active = XSTAT_IS_ACTIVE(svx);
        if (SVAR_BLOCK_BEGIN_CHAR(svt->svt_text[0])) {
#if DEBUG_XSTACK & 4
    printf("svare_exec_block('{') %-8s toke=%-6s, stack=",(is_active?"active":"inactive"), svt->svt_text);
    svare_print_xstack(svx);
#endif
            while (XSTAT_IS_COMPLETE(svx)) {
                svstat = svare_pop_xstat(svx, 0);
            }

            if (is_active) {
                svstat = svare_push_xstat(svx, XSTAT_ACTIVE_BLOCK);
            } else {
                svstat = svare_push_xstat(svx, XSTAT_INACTIVE_BLOCK);
            }
            if (!svstat) svstat = svare_get_token(svx, svt);
        } else if (SVAR_BLOCK_END_CHAR(svt->svt_text[0])) {
#if DEBUG_XSTACK & 4
            printf("svare_exec_block('}') %-8s toke=%-6s, stack=",(is_active?"active":"inactive"), svt->svt_text);
    svare_print_xstack(svx);
#endif
            while (XSTAT_IS_COMPLETE(svx)) {         /* 11/05/2019 -- moved here */
                svstat = svare_pop_xstat(svx, 0);    /* 11/05/2019 -- moved here */
            }                                        /* 11/05/2019 -- moved here */

            if (svx->svx_svs[svx->svx_nsvs - 1]->svs_xstate == XSTAT_FUNCTION) {    /* 11/06/2019 */
                svstat = svare_push_xstat(svx, XSTAT_RETURN);                       /* 11/06/2019 */
                svstat = SVARERR_RETURN;                                            /* 11/06/2019 */
            } else {                                                                /* 11/06/2019 */
                if (svx->svx_svs[svx->svx_nsvs - 1]->svs_xstate == XSTAT_DEFINE_FUNCTION) {
                    if (!svstat) svstat = svare_pop_save_tokens(svx);
                }
                svstat = svare_pop_xstat(svx, 0);

                if (!svstat) svstat = svare_update_xstat(svx);
                if (!svstat) svstat = svare_get_token(svx, svt);
            }
        } else {
            if (is_active) {
                svstat = svare_exec_active_stmt(svx, svt);
            } else {
                svstat = svare_exec_inactive_stmt(svx, svt);
            }
        }
    }

    return (svstat);
}
/***************************************************************/
static int svare_skip_block(struct svarexec * svx, struct svartoken * svt)
{
/*
** Expects: svt->svt_text == '{'
*/
    int svstat = 0;
    int depth;

    depth = 0;
    do {
        if (SVAR_BLOCK_BEGIN_CHAR(svt->svt_text[0])) {
            depth++;
        } else if (SVAR_BLOCK_END_CHAR(svt->svt_text[0])) {
            depth--;
        }
        svstat = svare_get_token(svx, svt);
    } while (!svstat && depth);

    return (svstat);
}
/***************************************************************/
static int svare_skip_statement(struct svarexec * svx, struct svartoken * svt)
{
    int svstat = 0;
    int done;

    done = 0;
    do {
        if (SVAR_STMT_TERM_CHAR(svt->svt_text[0])) {
            done = 1;
        }
        svstat = svare_get_token(svx, svt);
        if (SVAR_BLOCK_BEGIN_CHAR(svt->svt_text[0])) {
            done = 1;
        }
    } while (!svstat && !done);

    return (svstat);
}
/***************************************************************/
#ifdef OLD_WAY
static int svare_exec_if(struct svarexec * svx, struct svartoken * svt)
{
    int svstat = 0;
    struct svarvalue svv;
    int ifstat;

/*
** ifstat
**      0 - done with if
**      1 - looking for active if
**      2 - if was false
**      3 - completed true if
*/
    ifstat = 1;

    while (!svstat && ifstat) {
        switch (ifstat) {
            case 1:
                svstat = svare_evaluate(svx, svt, &svv);
                if (!svstat) {
                    if (svv.svv_dtype != SVV_DTYPE_BOOL) {
                        svstat = svare_set_error(svx, SVARERR_EXPECT_BOOLEAN,
                            "Expecting boolean value for if.");
                    } else {
                        if (svv.svv_val.svv_val_bool) {
                            svstat = svare_exec_block(svx, svt, 0);
                            ifstat = 3;
                        } else {
                            svstat = svare_skip_block(svx, svt);
                            ifstat = 2;
                        }
                    }
                }
                break;

            case 2:
                if (SVAR_STMT_TERM_CHAR(svt->svt_text[0])) {
                    ifstat = 0;
                } else if (!strcmp(svt->svt_text, "else")) {
                    svstat = svare_get_token(svx, svt);
                    if (SVAR_STMT_TERM_CHAR(svt->svt_text[0])) {
                        ifstat = 0;
                    } else if (SVAR_BLOCK_BEGIN_CHAR(svt->svt_text[0])) {
                        svstat = svare_exec_block(svx, svt, 0);
                        ifstat = 0;
                    } else {
                        if (!svstat) {
                            if (!strcmp(svt->svt_text, "if")) {
                                ifstat = 1;
                                svstat = svare_get_token(svx, svt);
                            } else {
                                svstat = svare_set_error(svx, SVARERR_EXPECT_IF,
                                    "Expecting 'if' after 'else'. Found: %s",
                                    svt->svt_text);
                            }
                        }
                    }
                } else {
                    svstat = svare_set_error(svx, SVARERR_EXP_STMT_TERMINATOR,
                    "Expecting else statement separator in if else. Found: %s",
                    svt->svt_text);
                }
                break;

            case 3:
                if (SVAR_STMT_TERM_CHAR(svt->svt_text[0])) {
                    ifstat = 0;
                } else if (!strcmp(svt->svt_text, "else")) {
                    svstat = svare_get_token(svx, svt);
                    if (SVAR_STMT_TERM_CHAR(svt->svt_text[0])) {
                        ifstat = 0;
                    } else if (SVAR_BLOCK_BEGIN_CHAR(svt->svt_text[0])) {
                        svstat = svare_skip_block(svx, svt);
                        ifstat = 0;
                    } else {
                        svstat = svare_skip_statement(svx, svt);
                        if (!svstat) {
                            if (SVAR_STMT_TERM_CHAR(svt->svt_text[0])) {
                                ifstat = 0;
                            } else
                                if (SVAR_BLOCK_BEGIN_CHAR(svt->svt_text[0])) {
                                svstat = svare_skip_block(svx, svt);
                            }
                        }
                    }
                } else {
                    svstat = svare_set_error(svx, SVARERR_EXP_STMT_TERMINATOR,
                        "Expecting else statement separator in if. Found: %s",
                        svt->svt_text);
                }
                break;

            default:
                break;
        }
    }

    return (svstat);
}
#endif
/***************************************************************/
static int svare_exec_if(struct svarexec * svx, struct svartoken * svt)
{
    int svstat = 0;
    struct svarvalue svv;

/*
** ifstat
*/
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

    return (svstat);
}
/***************************************************************/
static int svare_exec_while(struct svarexec * svx, struct svartoken * svt)
{
    int svstat = 0;
    struct svarvalue svv;

/*
** ifstat
*/
    svstat = svare_evaluate(svx, svt, &svv);
    if (!svstat) {
        if (svv.svv_dtype != SVV_DTYPE_BOOL) {
            svstat = svare_set_error(svx, SVARERR_EXPECT_BOOLEAN,
                "Expecting boolean value for if.");
        } else {
            if (svv.svv_val.svv_val_bool) {
                svstat = svare_push_xstat(svx, XSTAT_TRUE_WHILE);
            } else {
                svstat = svare_push_xstat(svx, XSTAT_INACTIVE_WHILE);
            }
        }
    }

    return (svstat);
}
/***************************************************************/
static int svare_exec_inactive_if(struct svarexec * svx, struct svartoken * svt)
{
    int svstat = 0;
/*
** ifstat
*/
    svstat = svare_evaluate_inactive(svx, svt);
    if (!svstat) {
        svstat = svare_push_xstat(svx, XSTAT_INACTIVE_IF);
    }

    return (svstat);
}
/***************************************************************/
static int svare_exec_inactive_while(struct svarexec * svx, struct svartoken * svt)
{
    int svstat = 0;
/*
** ifstat
*/
    svstat = svare_evaluate_inactive(svx, svt);
    if (svstat) svstat = svare_clear_error(svx, svstat);
    if (!svstat) {
        svstat = svare_push_xstat(svx, XSTAT_INACTIVE_WHILE);
    }

    return (svstat);
}
/***************************************************************/
static int svare_exec_inactive_func(struct svarexec * svx, struct svartoken * svt)
{
    int svstat = 0;
/*
** ifstat
*/
    svstat = svare_exec_func(svx, svt);

    return (svstat);
}
/***************************************************************/
#ifdef OLD_WAY_191105
static int svare_exec_active_stmt(struct svarexec * svx, struct svartoken * svt)
{
/*
**
**
*/
    int svstat = 0;
    int is_active;
    struct svarcursor current_svc;

#if DEBUG_XSTACK & 1
    printf("svare_exec_active_stmt()  toke=%-6s, stack=", svt->svt_text);
    svare_print_xstack(svx);
#endif

    if (!strcmp(svt->svt_text, "else")) {
        svstat = svare_update_xstat(svx);
        if (!svstat) svstat = svare_get_token(svx, svt);
    } else if (!strcmp(svt->svt_text, "if")) {
        svstat = svare_update_xstat(svx);
        if (!svstat) svstat = svare_get_token(svx, svt);
        if (!svstat) {
            svstat = svare_exec_if(svx, svt);
        }
    } else if (!strcmp(svt->svt_text, "while")) {
        svare_get_current_svc(svx, &current_svc);
        svstat = svare_update_xstat(svx);
        if (!svstat) svstat = svare_get_token(svx, svt);
        if (!svstat) {
            svstat = svare_exec_while(svx, svt);
            if (!svstat && svx->svx_svs[svx->svx_nsvs - 1]->svs_xstate == XSTAT_TRUE_WHILE) {
                svare_set_xstat_svc(svx, &current_svc);
            }

        }
    } else if (!strcmp(svt->svt_text, "func")) {
        svare_unget_token(svx);
        svstat = svare_get_token_no_save(svx, svt);
        if (!svstat) svstat = svare_get_token_no_save(svx, svt);
        if (!svstat) {
            svstat = svare_exec_func(svx, svt);
        }
    } else {
        while (XSTAT_IS_COMPLETE(svx)) {
            svstat = svare_pop_xstat(svx, 0);
        }
        is_active = XSTAT_IS_ACTIVE(svx);

        if (!is_active) {                                   /* 08/13/2019 */
            svstat = svare_exec_inactive_stmt(svx, svt);    /* 08/13/2019 */
        } else {                                            /* 08/13/2019 */
            if (!strcmp(svt->svt_text, "call")) {
                svstat = svare_get_token(svx, svt);
                if (!svstat) {
                    svstat = svare_exec_call(svx, svt);
                }
            } else if (!strcmp(svt->svt_text, "exit")) {
                svstat = svare_get_token(svx, svt);
                if (!svstat) {
                    svstat = svare_exec_exit(svx, svt);
                }
            } else if (!strcmp(svt->svt_text, "fail")) {
                svstat = svare_get_token(svx, svt);
                if (!svstat) {
                    svstat = svare_exec_fail(svx, svt);
                }
            } else if (!strcmp(svt->svt_text, "include")) {
                svstat = svare_get_token(svx, svt);
                if (!svstat) {
                    svstat = svare_exec_include(svx, svt);
                }
            } else if (!strcmp(svt->svt_text, "print")) {
                svstat = svare_get_token(svx, svt);
                if (!svstat) {
                    svstat = svare_exec_print(svx, svt);
                }
            } else if (!strcmp(svt->svt_text, "return")) {
                svstat = svare_get_token(svx, svt);
                if (!svstat) {
                    svstat = svare_exec_return(svx, svt);
                }
            } else if (!strcmp(svt->svt_text, "set")) {
                svstat = svare_get_token(svx, svt);
                if (!svstat) {
                    svstat = svare_exec_set(svx, svt);
                }
            } else if (SVAR_STMT_END_CHAR(svt->svt_text[0])) {
                /* null statement - do nothing */
            } else {
                svstat = svare_set_error(svx, SVARERR_UNRECOGNIZED_STMT,
                    "Unrecognized statement: %s", svt->svt_text);
            }

            if (!svstat) svstat = svare_update_xstat(svx);

            if (!svstat) {
                if (SVAR_STMT_END_CHAR(svt->svt_text[0])) {
                    svstat = svare_get_token(svx, svt);
                } else {
                    svstat = svare_set_error(svx, SVARERR_EXP_SEMICOLON_TERMINATOR,
                        "Expecting semicolon terminator. Found: \"%s\"", svt->svt_text);
                }
            } else if (svstat == SVARERR_RETURN) {
                if (! SVAR_STMT_END_CHAR(svt->svt_text[0])) {
                    svstat = svare_set_error(svx, SVARERR_EXP_SEMICOLON_TERMINATOR,
                        "Expecting semicolon terminator. Found: \"%s\"", svt->svt_text);
                }
            }
        }
    }

#ifdef UNUSED
    if (svstat == SVARERR_EOF) {
        svstat = svare_check_eof(svx, svt);
    }
#endif

    return (svstat);
}
#else
static int svare_exec_active_stmt(struct svarexec * svx, struct svartoken * svt)
{
/*
**
**
*/
    int svstat = 0;
    int is_active;
    struct svarcursor current_svc;

#if DEBUG_XSTACK & 1
    printf("svare_exec_active_stmt()  toke=%-6s, stack=", svt->svt_text);
    svare_print_xstack(svx);
#endif

    if (!strcmp(svt->svt_text, "else")) {
        if (!XSTAT_IS_COMPLETE(svx)) {                                                      /* 11/07/2019 */
            svstat = svare_set_error(svx, SVARERR_EXP_IF,                                   /* 11/07/2019 */
                "else expects previous statement to be if. Found: \"%s\"", svt->svt_text);  /* 11/07/2019 */
        } else {                                                                            /* 11/07/2019 */
            svstat = svare_update_xstat(svx);
            if (!svstat) svstat = svare_get_token(svx, svt);
        }
    } else if (!strcmp(svt->svt_text, "if")) {
        while (XSTAT_IS_COMPLETE(svx)) { svstat = svare_pop_xstat(svx, 0); } /* 11/04/2019 */
        svstat = svare_update_xstat(svx);
        is_active = XSTAT_IS_ACTIVE(svx);                       /* 11/06/2019 */
        if (!svstat) svstat = svare_get_token(svx, svt);
        if (!svstat) {
            if (!is_active) {                                   /* 11/06/2019 */
                svstat = svare_exec_inactive_if(svx, svt);      /* 11/06/2019 */
            } else {
                svstat = svare_exec_if(svx, svt);
            }
        }
    } else if (!strcmp(svt->svt_text, "while")) {
        while (XSTAT_IS_COMPLETE(svx)) { svstat = svare_pop_xstat(svx, 0); } /* 11/04/2019 */
        svare_get_current_svc(svx, &current_svc);
        svstat = svare_update_xstat(svx);
        if (!svstat) svstat = svare_get_token(svx, svt);
        if (!svstat) {
            svstat = svare_exec_while(svx, svt);
            if (!svstat && svx->svx_svs[svx->svx_nsvs - 1]->svs_xstate == XSTAT_TRUE_WHILE) {
                svare_set_xstat_svc(svx, &current_svc);
            }

        }
    } else if (!strcmp(svt->svt_text, "func")) {
        while (XSTAT_IS_COMPLETE(svx)) { svstat = svare_pop_xstat(svx, 0); } /* 11/04/2019 */
        svare_unget_token(svx);
        svstat = svare_get_token_no_save(svx, svt);
        if (!svstat) svstat = svare_get_token_no_save(svx, svt);
        if (!svstat) {
            svstat = svare_exec_func(svx, svt);
        }
    } else {
        while (XSTAT_IS_COMPLETE(svx)) { svstat = svare_pop_xstat(svx, 0); }
        is_active = XSTAT_IS_ACTIVE(svx);

        if (!is_active) {                                   /* 08/13/2019 */
            svstat = svare_exec_inactive_stmt(svx, svt);    /* 08/13/2019 */
        } else {                                            /* 08/13/2019 */
            if (!strcmp(svt->svt_text, "call")) {
                svstat = svare_get_token(svx, svt);
                if (!svstat) {
                    svstat = svare_exec_call(svx, svt);
                }
            } else if (!strcmp(svt->svt_text, "exit")) {
                svstat = svare_get_token(svx, svt);
                if (!svstat) {
                    svstat = svare_exec_exit(svx, svt);
                }
            } else if (!strcmp(svt->svt_text, "fail")) {
                svstat = svare_get_token(svx, svt);
                if (!svstat) {
                    svstat = svare_exec_fail(svx, svt);
                }
            } else if (!strcmp(svt->svt_text, "include")) {
                svstat = svare_get_token(svx, svt);
                if (!svstat) {
                    svstat = svare_exec_include(svx, svt);
                }
            } else if (!strcmp(svt->svt_text, "print")) {
                svstat = svare_get_token(svx, svt);
                if (!svstat) {
                    svstat = svare_exec_print(svx, svt);
                }
            } else if (!strcmp(svt->svt_text, "return")) {
                svstat = svare_get_token(svx, svt);
                if (!svstat) {
                    svstat = svare_exec_return(svx, svt);
                }
            } else if (!strcmp(svt->svt_text, "set")) {
                svstat = svare_get_token(svx, svt);
                if (!svstat) {
                    svstat = svare_exec_set(svx, svt);
                }
            } else if (SVAR_STMT_END_CHAR(svt->svt_text[0])) {
                /* null statement - do nothing */
            } else {
                svstat = svare_set_error(svx, SVARERR_UNRECOGNIZED_STMT,
                    "Unrecognized statement: %s", svt->svt_text);
            }

            if (!svstat) svstat = svare_update_xstat(svx);

            if (!svstat) {
                if (SVAR_STMT_END_CHAR(svt->svt_text[0])) {
                    svstat = svare_get_token(svx, svt);
                } else {
                    svstat = svare_set_error(svx, SVARERR_EXP_SEMICOLON_TERMINATOR,
                        "Expecting semicolon terminator. Found: \"%s\"", svt->svt_text);
                }
            } else if (svstat == SVARERR_RETURN) {
                if (! SVAR_STMT_END_CHAR(svt->svt_text[0])) {
                    svstat = svare_set_error(svx, SVARERR_EXP_SEMICOLON_TERMINATOR,
                        "Expecting semicolon terminator. Found: \"%s\"", svt->svt_text);
                }
            }
        }
    }

#ifdef UNUSED
    if (svstat == SVARERR_EOF) {
        svstat = svare_check_eof(svx, svt);
    }
#endif

    return (svstat);
}
#endif
/***************************************************************/
static int svare_exec_inactive_stmt(struct svarexec * svx, struct svartoken * svt)
{
    int svstat = 0;
    int done;

#if DEBUG_XSTACK & 1
    printf("svare_exec_inactive_stmt() toke=%-6s, stack=", svt->svt_text);
    svare_print_xstack(svx);
#endif

    if (!strcmp(svt->svt_text, "else")) {
        svstat = svare_get_token(svx, svt);
        if (svx->svx_svs[svx->svx_nsvs - 1]->svs_xstate == XSTAT_FALSE_IF) {
            svstat = svare_pop_push_xstat(svx, XSTAT_FALSE_IF, XSTAT_TRUE_ELSE);
        } else if (svx->svx_svs[svx->svx_nsvs - 1]->svs_xstate == XSTAT_INACTIVE_IF) {
            svstat = svare_pop_push_xstat(svx, XSTAT_INACTIVE_IF, XSTAT_INACTIVE_ELSE);
        }
    } else if (!strcmp(svt->svt_text, "if")) {
        svstat = svare_update_xstat(svx);
        if (!svstat) svstat = svare_get_token(svx, svt);
        if (!svstat) {
            svstat = svare_exec_inactive_if(svx, svt);
        }
    } else if (!strcmp(svt->svt_text, "while")) {
        svstat = svare_update_xstat(svx);
        if (!svstat) svstat = svare_get_token(svx, svt);
        if (!svstat) {
            svstat = svare_exec_inactive_while(svx, svt);
        }
    } else if (!strcmp(svt->svt_text, "func")) {
        svare_unget_token(svx);
        svstat = svare_get_token_no_save(svx, svt);
        if (!svstat) svstat = svare_get_token_no_save(svx, svt);
        if (!svstat) {
            svstat = svare_exec_inactive_func(svx, svt);
        }
    } else if (!strcmp(svt->svt_text, ";")) {       /* 08/20/2019 */
        while (XSTAT_IS_COMPLETE(svx)) {            /* 08/20/2019 */
            svstat = svare_pop_xstat(svx, 0);       /* 08/20/2019 */
        }                                           /* 08/20/2019 */
        if (!svstat) {                              /* 08/20/2019 */
            svstat = svare_update_xstat(svx);       /* 08/20/2019 */
        }                                           /* 08/20/2019 */
        if (!svstat) {                              /* 08/20/2019 */
            svstat = svare_get_token(svx, svt);     /* 08/20/2019 */
        }                                           /* 08/20/2019 */
    } else {                                        /* 08/20/2019 */
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
    }

    return (svstat);
}
/***************************************************************/
static int svare_pop_final_xstat(struct svarexec * svx)
{
    int svstat = 0;

    if (svx->svx_nsvs < 1) {
        svstat = svare_set_error(svx, SVARERR_RUNTIME_STACK_UNDERFLOW,
            "Runtime stack underflow.");
    } else if (svx->svx_nsvs > 1) {
#if DEBUG_XSTACK & 2
    printf("SVARERR_RUNTIME_STACK_ITEMS_REMAIN\n");
    svare_print_xstack(svx);
#endif
        svstat = svare_set_error(svx, SVARERR_RUNTIME_STACK_ITEMS_REMAIN,
            "Items remain on runtime stack.");
    } else {
        svx->svx_nsvs -= 1;
    }

    return (svstat);
}
/***************************************************************/
static int svare_exec_svarexec(struct svarexec * svx)
{
    int svstat = 0;
    struct svartoken * svt;

    svt = svar_new_svartoken();

    svstat = svare_push_xstat(svx, XSTAT_BEGIN);
    if (!svstat) svstat = svare_get_token(svx, svt);
    if (!svstat) svstat = svare_exec_block(svx, svt);

    if (svstat == SVARERR_EXIT || svstat == SVARERR_EOF) svstat = 0;

    if (!svstat) {
        svstat = svare_pop_final_xstat(svx);
    }

    svar_free_svartoken(svt);

    return (svstat);
}
/***************************************************************/
int svar_exec(struct svarexec * svx)
{
    int svstat = 0;

    svstat = svare_exec_svarexec(svx);

    return (svstat);
}
/***************************************************************/
char * svar_get_error_message(struct svarexec * svx, int esvstat)
{
    int svstat = 0;
    char * emsg;

    if (svx->svx_errmsg) {
        emsg = Smprintf("%s (SVERR %d)", svx->svx_errmsg, esvstat);
    } else {
        emsg = Smprintf("(SVERR %d)", esvstat);
    }

    return (emsg);
}
/***************************************************************/
#ifdef WHCODE
/*@*/void do_runcon (STMT * stmt, EBLK ** err) {
/*
** parms is static so that every set command does not
** create a new entry in the symbol table.
**
** Example in WHCONFIL
**
**      #################################################################
**      ## NetSuite authentication
**      [RUN#TEST1]
**      set NSDEVKEY="C9C6FC46-C297-460A-B6DA-86E9916E49E2";
**      set NSEK=EnBase64(Mask(NSDEVKEY, 520));
**      if NSDEVKEY <> Mask(DeBase64(NSEK), 520) {
**          print "Test value mismatch.";
**      };
**      print "NSEK="&NSEK;
**      #################################################################
**
**      To turn on SOAP debug in MSG991 (WHCONFIL):
**
**          call SetXMLDebug("PARMS,REQUEST,RESPONSE,HEADERS,WSDLTRACE");
**              -or-
**          call SetXMLDebug(
**              "PARMS,REQUEST,RESPONSE,HEADERS,WSDLTRACE,RESPONSEHEADERS");
**
**      Warehouse script to run example:
**          RUNCON TEST1
*/
    static SYM *parms=0;
    SYM *ptr=0;
    char parm[100];
    struct strlist *      run_strlist;
    struct svarexec * svx;
    int svstat = 0;
    char * emsg;
    struct svarrec      * svarrec;
    int pn;
    char pbuf[16];

    read_token_c(stmt, parm);
    svarrec      = svar_new();
    run_strlist = get_auth_strlist("RUN", NULL, parm, err);
    if (*err) return;
    if (!run_strlist) {
        *err = set_error(19138);
        set_error_text(*err, parm);
        return;
    }

    pn = 0;
    do {
        read_token_c(stmt, parm);
        if ((parm[0])) {
            sprintf(pbuf, "P%d", ++pn);
            svar_set(svarrec, pbuf, parm,
                SVAR_FLAG_REMOVE_OPTIONAL_DOUBLE_QUOTES |
                SVAR_FLAG_NO_DUPLICATE_VAR);
        }
    } while (parm[0]);

    svx = svare_init(svarrec, run_strlist);

    svstat = svar_exec(svx);
    if (svstat) {
        *err = set_error(19127);
        set_error_long(*err, "%d", svstat);
        emsg = svar_get_error_message(svx, svstat);
        set_error_text(*err, emsg);
        Free(emsg);
    }
    svare_shut(svx);
    svar_free(svarrec);
}
/***************************************************************/
void file_fileread(
    char * file_name,
    char * file_mode,
    struct wh_str *dest,
    EBLK ** err) {

    int nread;
    int done;
    int fopenflags;
    int default_mode;
    WHFILE *fptr;
    char * fmode_str;
    char buf[1024];

    str_setlen(0, dest, err);
    default_mode = WH_OPEN_READ;

    parse_wh_file_parms(file_mode, &(fmode_str), &(fopenflags),
                    default_mode, err);
    if (*err) return;

    fptr = wh_fopen(file_name, fmode_str, fopenflags, err);
    if (*err) return;

    done = 0;
    while (!done) {
        nread = (int)wh_fread(fptr, buf, sizeof(buf), err);
        if (*err) {
            done = 1;
        }
        if (!nread) {
            done = 1;
        } else {
            str_append(buf, nread, dest, err);
        }
    }

    wh_fclose(fptr, err);
    Free(fmode_str);
}
/***************************************************************/
#endif
/***************************************************************/
int xeq_strlist(struct omarglobals * og,
    struct svarglob * svg,
    struct strlist * xeq_strl)
{
    int estat = 0;
    struct svarexec * svx;
    int svstat = 0;
    char * emsg;
    struct svarrec      * svarrec;

    svarrec      = svar_new();
    svx = svare_init(svg, svarrec, xeq_strl, og);

    svstat = svar_exec(svx);
    if (svstat) {
        emsg = svar_get_error_message(svx, svstat);
        estat = set_error(og, "XEQ error: %s", emsg);
        Free(emsg);
    }
    svar_wait_for_all_threads(svg);

    svare_shut(svx);
    svar_free(svarrec);

    return (estat);
}
/***************************************************************/
int strl_fileread(struct omarglobals * og,
    const char * file_name,
    struct strlist * strl)
{
    int estat = 0;
    int is_err;
    struct frec * xeq_fr;

    xeq_fr = frec_open(file_name, "r", "XEQ file");
    if (!xeq_fr) {
        estat = set_error(og, "Error opening XEQ file for input. File: %s",
            file_name);
        return (estat);
    }

    is_err = read_include_file(xeq_fr, strl);
    if (is_err) {
        estat = set_error(og, "Error reading XEQ file.");
    }

    return (estat);
}
/***************************************************************/
int process_xeq(struct omarglobals * og, const char * xeqfame)
{
/*
**
*/
    int estat = 0;
    struct strlist * xeq_strl;
    struct svarglob * svg;

    xeq_strl = strlist_new();

    estat = strl_fileread(og, xeqfame, xeq_strl);

    if (!estat) {
        svg = svar_new_svarglob();
        estat = xeq_strlist(og, svg, xeq_strl);
        svar_free_svarglob(svg);
    }

    /* strlist_free(xeq_strl); */

    return (estat);
}
/***************************************************************/
