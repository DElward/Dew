/***************************************************************/
/* thread.c                                                    */
/***************************************************************/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#if IS_WINDOWS
    #define WIN32_LEAN_AND_MEAN
    #include <Windows.h>
    //#include <time.h>
    #include <process.h>
    #include <io.h>
    #include <fcntl.h>
    #include <Winsock2.h>
#else   /* Unix */
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <signal.h>
    #include <fcntl.h>
    #include <sys/time.h>
    #include <sys/times.h>
    #include <unistd.h>
    #include <pthread.h>
#endif

#include "omar.h"
#include "xeq.h"
#include "thread.h"
#include "intclas.h"

/***************************************************************/
/**  struct threadlist                                         */
/***************************************************************/
struct threadlist * threadlist_new(void)
{
    struct threadlist * thl;

    thl = New(struct threadlist, 1);

    thl->thl_crit_section = New(CRITSECTYP, 1);
    CRIT_INIT(thl->thl_crit_section);

    thl->thl_crit_section_nlocks = 0;
    thl->thl_nthreads = 0;
    thl->thl_xthreads = 0;
    thl->thl_athreads = NULL;
    thl->thl_athreadids = NULL;

    return (thl);
}
/***************************************************************/
void threadlist_free(struct threadlist * thl)
{
    Free(thl->thl_athreads);
    Free(thl->thl_athreadids);

    CRIT_DEL(thl->thl_crit_section);
    Free(thl->thl_crit_section);

    Free(thl);
}
/***************************************************************/
void threadlist_lock_crit_section(struct threadlist * thl)
{
    if (!thl->thl_crit_section_nlocks) CRIT_LOCK(thl->thl_crit_section);
    thl->thl_crit_section_nlocks += 1;
}
/***************************************************************/
void threadlist_unlock_crit_section(struct threadlist * thl)
{
    thl->thl_crit_section_nlocks -= 1;
    if (!thl->thl_crit_section_nlocks) CRIT_ULOCK(thl->thl_crit_section);
}
/***************************************************************/
#if 0
void threadlist_print(struct threadlist * thl)
{
    int ii;

    printf("THREADLIST: Num threads=%d\n", thl->thl_nthreads); fflush(stdout);
    for (ii = 0; ii < thl->thl_nthreads; ii++) {
        printf("THREADLIST: %d. %d (0x%lx)\n", 
            ii, (long)thl->thl_athreadids[ii], (long)thl->thl_athreads[ii]); fflush(stdout);
    }
}
#endif
/***************************************************************/
void threadlist_remove_thread_index(struct threadlist * thl, int thix)
{
    int ii;

    if (!thl->thl_crit_section_nlocks) return;

    if (thix < 0 || thix >= thl->thl_nthreads) {
#if 0
        printf("******** Thread index %d out of range. Num threads=%d\n",
            thix, thl->thl_nthreads); fflush(stdout);
#endif
        return;
    }

    ii = thix;

#if 0
    printf("******** Removing thread %d (0x%lx)\n",
        (long)thl->thl_athreadids[ii], (long)thl->thl_athreads[ii]); fflush(stdout);
#endif

    while (ii + 1 < thl->thl_nthreads) {
        thl->thl_athreads[ii] = thl->thl_athreads[ii + 1];
        thl->thl_athreadids[ii] = thl->thl_athreadids[ii + 1];
        ii++;
    }
    thl->thl_nthreads -= 1;
    thl->thl_athreads[thl->thl_nthreads] = 0;
    thl->thl_athreadids[thl->thl_nthreads] = 0;
}
/***************************************************************/
int threadlist_find_thread_by_hdl(struct threadlist * thl, THREAD_TTYP thhdl)
{
    int thix;
    int found;

    if (!thl->thl_crit_section_nlocks) return (-1);

    found = 0;
    thix  = 0;

    while (!found && thix < thl->thl_nthreads) {
        if (thhdl == thl->thl_athreads[thix]) found = 1;
        else thix++;
    }
    if (!found) thix = -1;

    return (thix);
}
/***************************************************************/
void threadlist_remove_thread_by_hdl(struct threadlist * thl, THREAD_TTYP thhdl)
{
    int thix;

    threadlist_lock_crit_section(thl);  /**** Lock threadlist ****/

    thix = threadlist_find_thread_by_hdl(thl, thhdl);
#if 0
    if (thix < 0) {
        printf("******** Thread handle (0x%lx) not found.\n", (long)thhdl); fflush(stdout);
        threadlist_print(thl);
    }
#endif
    if (thix >= 0) {
        threadlist_remove_thread_index(thl, thix);
    }

    threadlist_unlock_crit_section(thl);  /**** Unlock threadlist ****/
}
/***************************************************************/
int threadlist_find_thread_by_id(struct threadlist * thl, THREAD_ITYP thid)
{
    int thix;
    int found;

    if (!thl->thl_crit_section_nlocks) return (-1);

    found = 0;
    thix  = 0;

    while (!found && thix < thl->thl_nthreads) {
        if (thid == thl->thl_athreadids[thix]) found = 1;
        else thix++;
    }
    if (!found) thix = -1;

    return (thix);
}
/***************************************************************/
void threadlist_remove_thread_by_id(struct threadlist * thl, THREAD_ITYP thid)
{
    int thix;

    threadlist_lock_crit_section(thl);  /**** Lock threadlist ****/

    thix = threadlist_find_thread_by_id(thl, thid);
#if 0
    if (thix < 0) {
        printf("******** Thread id %ld not found.\n", (long)thid); fflush(stdout);
        threadlist_print(thl);
    }
#endif
    if (thix >= 0) {
        threadlist_remove_thread_index(thl, thix);
    }

    threadlist_unlock_crit_section(thl);  /**** Unlock threadlist ****/
}
/***************************************************************/
void threadlist_add_thread(struct threadlist * thl, THREAD_TTYP thhdl, THREAD_ITYP thid)
{
    threadlist_lock_crit_section(thl);  /**** Lock threadlist ****/

    if (thl->thl_nthreads == thl->thl_xthreads) {
        if (!thl->thl_xthreads) thl->thl_xthreads = 4;
        else thl->thl_xthreads *= 2;
        thl->thl_athreads = Realloc(thl->thl_athreads, THREAD_TTYP, thl->thl_xthreads);
        thl->thl_athreadids = Realloc(thl->thl_athreadids, THREAD_ITYP, thl->thl_xthreads);
    }
    thl->thl_athreads[thl->thl_nthreads] = thhdl;
    thl->thl_athreadids[thl->thl_nthreads] = thid;
    thl->thl_nthreads += 1;

#if 0
    printf("******** Added thread %d (0x%lx)\n", (long)thid, (long)thhdl); fflush(stdout);
#endif

    threadlist_unlock_crit_section(thl);  /**** Unlock threadlist ****/
}
/***************************************************************/
THREAD_TTYP threadlist_get_last_thread_hdl(struct threadlist * thl)
{
    THREAD_TTYP thhdl;

    threadlist_lock_crit_section(thl);  /**** Lock threadlist ****/

    thhdl = 0;
    if (thl->thl_nthreads > 0) {
        thhdl = thl->thl_athreads[thl->thl_nthreads - 1];
    }

    threadlist_unlock_crit_section(thl);  /**** Unlock threadlist ****/

    return (thhdl);
}
/***************************************************************/
void threadlist_wait_for_all_threads(struct threadlist * thl)
{
    THREAD_TTYP thhdl;

#if 0
    threadlist_print(thl);
#endif

    thhdl = threadlist_get_last_thread_hdl(thl);

    while (thhdl) {
        THREAD_COMPLETE_WAIT(thhdl);
        threadlist_remove_thread_by_hdl(thl, thhdl);
        thhdl = threadlist_get_last_thread_hdl(thl);
    }
}
/***************************************************************/
/**  Thread -  struct neural_network                           */
/***************************************************************/
static struct thread_info * new_thread_info(void)
{
    struct thread_info * thi;

    thi = New(struct thread_info, 1);
    thi->thi_filler = 777;

    return (thi);
}
/***************************************************************/
static void free_thread_info(struct thread_info * thi)
{
    Free(thi);
}
/***************************************************************/
static struct thread_handle * new_thread_handle(struct thread_info * thi)
{
    struct thread_handle * thh;

    thh = New(struct thread_handle, 1);
    thh->thh_parent = thi;
    thh->thh_svf    = NULL;
    thh->thh_nsvv   = 0;
    thh->thh_asvv   = NULL;
    thh->thh_os_thread = 0;
    thh->thh_stat   = 0;

    return (thh);
}
/***************************************************************/
static void free_thread_handle(struct thread_handle * thh)
{
    int ii;

#if IS_MULTITHREADED
    if (thh->thh_os_thread && thh->thh_stat == THHSTAT_STARTED) {
        THREAD_COMPLETE_WAIT(thh->thh_os_thread);
        thh->thh_stat = THHSTAT_TERMINATED;
        svare_get_svarglob(thh->thh_svx);    /**** Lock globals ****/
        threadlist_remove_thread_by_hdl(thh->thh_svx->svx_locked_svg->svg_thl, thh->thh_os_thread);
        svare_release_svarglob(thh->thh_svx);    /**** Unock globals ****/
    }
#endif
    svare_release_svarexec(thh->thh_svx);

    for (ii = 0; ii < thh->thh_nsvv; ii++) {
        svar_free_svarvalue(thh->thh_asvv[ii]);
    }
    Free(thh->thh_asvv);
    Free(thh);
}
/***************************************************************/
static int svare_method_ThreadNew(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)
{
    int svstat = 0;
    struct thread_info * thi;

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "<" SVAR_CLASS_NAME_THREAD ">");

    if (!svstat) {
        thi = new_thread_info();
        svare_store_int_object(svx, asvv[0]->svv_val.svv_svnc, rsvv, thi);
    }

    return (svstat);
}
/***************************************************************/
static int svare_method_ThreadDelete(struct svarexec * svx,   /* NULL for destructor */
    const char * func_name,     /* NULL for destructor */
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)    /* NULL for destructor */
{
    int svstat = 0;
    struct thread_info * thi;

    thi = (struct thread_info *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
    free_thread_info(thi);

    return (svstat);
}
/***************************************************************/
#if IS_MULTITHREADED
static int svare_thread_main(struct thread_handle * thh)
{
    int svstat = 0;
    struct svarvalue rsvv;

    INIT_SVARVALUE(&rsvv);

    svstat = svare_push_xstat(thh->thh_svx, XSTAT_BEGIN_THREAD);

    svstat = svare_call_user_function(
        thh->thh_svx, thh->thh_svf, thh->thh_nsvv, thh->thh_asvv, &rsvv);

    /* svstat = svare_pop_xstat(thh->thh_svx, XSTAT_BEGIN_THREAD); */

    svare_get_svarglob(thh->thh_svx);    /**** Lock globals ****/
    threadlist_remove_thread_by_id(thh->thh_svx->svx_locked_svg->svg_thl, THREAD_SELF);
    svare_release_svarglob(thh->thh_svx);    /**** Unock globals ****/

    THREAD_END;

    return (svstat);
}
#endif
/***************************************************************/
#if IS_MULTITHREADED
static int svare_call_thread(struct svarexec * svx, struct thread_handle * thh)
{
    int svstat = 0;

    THREAD_BEGIN(thh->thh_os_thread, svare_thread_main, thh);

    if ((long)(thh->thh_os_thread) == THREAD_BEGIN_ERROR) {
        thh->thh_stat = THHSTAT_START_ERROR;
        svstat = svare_set_error(thh->thh_svx, SVARERR_THREAD_START_ERROR,
            "Error starting thread");
    } else {
        thh->thh_stat = THHSTAT_STARTED;
        svare_get_svarglob(svx);    /**** Lock globals ****/
        threadlist_add_thread(svx->svx_locked_svg->svg_thl,
            thh->thh_os_thread, THREAD_ID(thh->thh_os_thread));
        svare_release_svarglob(svx);    /**** Unock globals ****/
    }

    return (svstat);
}
#else
static int svare_call_thread(struct thread_handle * thh)
{
    int svstat = 0;
    struct svarvalue rsvv;

    INIT_SVARVALUE(&rsvv);
    svstat = svare_push_xstat(thh->thh_svx, XSTAT_BEGIN_THREAD);

    svstat = svare_call_user_function(
        thh->thh_svx, thh->thh_svf, thh->thh_nsvv, thh->thh_asvv, &rsvv);

    /* svstat = svare_pop_xstat(thh->thh_svx, XSTAT_BEGIN_THREAD); */

    return (svstat);
}
#endif
/***************************************************************/
static int svare_method_ThreadStart(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)
{
    int svstat = 0;
    struct thread_info * thi;
    struct thread_handle * thh;
    struct svarvalue * csvv;
    int ii;

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "{" SVAR_CLASS_NAME_THREAD "}p.");

    if (!svstat) {
        thi = (struct thread_info *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        csvv = svare_find_class(svx, SVAR_CLASS_NAME_THREAD_HANDLE);
        if (!csvv) {
            svstat = svare_set_error(svx, SVARERR_UNDEFINED_CLASS,
                "Undefined variable: %s", SVAR_CLASS_NAME_THREAD_HANDLE);
        } else {
            thh = new_thread_handle(thi);
            thh->thh_svx = svare_dup_svarexec(svx);
            thh->thh_nsvv   = nsvv - 2;
            thh->thh_svf    = asvv[1]->svv_val.svv_svf;
            if (thh->thh_nsvv > 0) {
                thh->thh_asvv = New(struct svarvalue *, thh->thh_nsvv);
                for (ii = 0; !svstat && ii < thh->thh_nsvv; ii++) {
                    thh->thh_asvv[ii] = New(struct svarvalue, 1);
                    svstat = svare_store_svarvalue(svx,
                        thh->thh_asvv[ii], asvv[ii + 2]);
                }
            }
            if (!svstat) svstat = svare_call_thread(svx, thh);
            if (csvv->svv_dtype == SVV_DTYPE_INT_CLASS) {
                if (!svstat) svare_store_int_object(svx, csvv->svv_val.svv_svnc, rsvv, thh);
            }
        }
    }

    return (svstat);
}
/***************************************************************/
static int svare_method_ThreadHandleDelete(struct svarexec * svx,   /* NULL for destructor */
    const char * func_name,     /* NULL for destructor */
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)    /* NULL for destructor */
{
    int svstat = 0;
    struct thread_handle * thh;

    thh = (struct thread_handle *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
    free_thread_handle(thh);

    return (svstat);
}
/***************************************************************/
static int svare_thread_wait(struct svarexec * svx,
    int wait_millisecs, /* unimplemented */
    int nthreads,
    struct thread_handle ** thh_list)
{
    int svstat = 0;
    int nstarted;
    int ii;

    do {
        nstarted = 0;
        for (ii = 0; ii < nthreads; ii++) {
            if (thh_list[ii]->thh_stat == THHSTAT_STARTED) {
                nstarted++;
                THREAD_COMPLETE_WAIT(thh_list[ii]->thh_os_thread);
                thh_list[ii]->thh_stat = THHSTAT_TERMINATED;
                svare_get_svarglob(svx);    /**** Lock globals ****/
                threadlist_remove_thread_by_hdl(svx->svx_locked_svg->svg_thl, thh_list[ii]->thh_os_thread);
                svare_release_svarglob(svx);    /**** Unock globals ****/
            }
        }
    } while (!svstat && nstarted);

    return (svstat);
}
/***************************************************************/
static int svare_method_ThreadWait(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)
{
    int svstat = 0;
    struct thread_info * thi;
    struct thread_handle * thh;
    int wait_millisecs;
    int nthreads;
    int ii;
    struct thread_handle ** thh_list;

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "{" SVAR_CLASS_NAME_THREAD "}i.");

    if (!svstat) {
        thi = (struct thread_info *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        wait_millisecs = asvv[0]->svv_val.svv_val_int;
        nthreads = nsvv - 2;
        thh_list = NULL;

        if (nthreads) {
            thh_list = New(struct thread_handle *, nthreads);
            for (ii = 0; !svstat && ii < nthreads; ii++) {
                if (asvv[ii + 2]->svv_dtype != SVV_DTYPE_INT_OBJECT) {
                    svstat = svare_set_error(svx, SVARERR_INVALID_PARM_TYPE,
                            "Expecting object as parameter #%d to %s.", ii + 2, func_name);
                } else if (asvv[ii + 2]->svv_val.svv_svvi.svvi_int_class->svnc_class_id != SVAR_CLASS_ID_THREAD_HANDLE) {
                    svstat = svare_set_error(svx, SVARERR_INVALID_PARM_TYPE,
                            "Expecting ThreadHandle as parameter #%d to %s.", ii + 2, func_name);
                } else {
                    thh = (struct thread_handle *)(asvv[ii + 2]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
                    thh_list[ii] = thh;
                }
            }
            if (!svstat) {
                svstat = svare_thread_wait(svx, wait_millisecs, nthreads, thh_list);
            }
            Free(thh_list);
        }
    }

    return (svstat);
}
/***************************************************************/
void svare_init_internal_thread_class(struct svarexec * svx)
{
    struct svarvalue * svv;

    /* Thread */
    svv = svare_new_int_class(svx, SVAR_CLASS_ID_THREAD);
    svare_new_int_class_method(svv, ".new"              , svare_method_ThreadNew);
    svare_new_int_class_method(svv, ".delete"           , svare_method_ThreadDelete);
    svare_new_int_class_method(svv, "start"             , svare_method_ThreadStart);
    svare_new_int_class_method(svv, "wait"              , svare_method_ThreadWait);
    svare_add_int_class(svx, SVAR_CLASS_NAME_THREAD, svv);

    /* ThreadHandle */
    svv = svare_new_int_class(svx, SVAR_CLASS_ID_THREAD_HANDLE);
    svare_new_int_class_method(svv, ".delete"           , svare_method_ThreadHandleDelete);
    svare_add_int_class(svx, SVAR_CLASS_NAME_THREAD_HANDLE, svv);
}
/***************************************************************/
