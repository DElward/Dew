#ifndef THREADH_INCLUDED
#define THREADH_INCLUDED
/***************************************************************/
/* thread.h                                                    */
/***************************************************************/
/***************************************************************/
/* thread helpers                                              */
/***************************************************************/
#if IS_MULTITHREADED
    #if IS_WINDOWS
        #define WIN32_LEAN_AND_MEAN
        #include <Windows.h>
        #define CRIT_INIT(cs)  (InitializeCriticalSection(cs))
        #define CRIT_LOCK(cs)  (EnterCriticalSection(cs))
        #define CRIT_ULOCK(cs) (LeaveCriticalSection(cs))
        #define CRIT_DEL(cs)   (DeleteCriticalSection(cs))
        #define CRIT_TRY(cs)   (TryEnterCriticalSection(cs))
        #define CRITSECTYP CRITICAL_SECTION

        #define THREAD_TTYP HANDLE
        #define THREAD_ITYP long
        #define THREAD_RTYP void
        #define THREAD_PIPE(p) _pipe((p), 256, _O_BINARY )

        #define THREAD_BEGIN(t,f,p) (!((t)=(THREAD_TTYP)_beginthread((f),4096,(p)))?errno:0)
        #define THREAD_BEGIN_ERROR  (-1)
        #define THREAD_END _endthreadex(0)
        #define THREAD_EXISTS(t,s) (GetExitCodeThread((t),&(s))==STILL_ACTIVE)
        #define THERRNO   GetLastError()
        #define THREAD_SELF ((THREAD_ITYP)GetCurrentThreadId())
        #define THREAD_COMPLETE_WAIT(t) (WaitForSingleObject((t), INFINITE))
        #define THREAD_ID(h) ((THREAD_ITYP)GetThreadId(h))
    #else
        #include <pthread.h>
        #define CRIT_INIT(cs)  (pthread_mutex_init(cs,NULL))
        #define CRIT_LOCK(cs)  (pthread_mutex_lock(cs))
        #define CRIT_ULOCK(cs) (pthread_mutex_unlock(cs))
        #define CRIT_DEL(cs)   (pthread_mutex_destroy(cs))
        #define CRIT_TRY(cs)   (!pthread_mutex_trylock(cs))
        #define CRITSECTYP pthread_mutex_t

        #define THREAD_TTYP pthread_t
        #define THREAD_ITYP long
        #define THREAD_RTYP void*
        #define THREAD_PIPE(p) pipe(p)

        #define THREAD_BEGIN(t,f,p) pthread_create(&t,NULL,(f),(p))
        #define THREAD_BEGIN_ERROR  (-1)
        #define THREAD_END pthread_exit(NULL)
        #define THREAD_EXISTS(t,s) (pthread_join((t),&(s))==STILL_ACTIVE)
        #define THERRNO   errno
        #define THREAD_SELF ((THREAD_ITYP)pthread_self())
        #define THREAD_COMPLETE_WAIT(t) (pthread_join((t),NULL))
        #define THREAD_ID(h) ((THREAD_ITYP)(h))
    #endif /* _Windows */
#else
        #define THREAD_TTYP int
#endif

struct threadlist { /* thl_ */
    CRITSECTYP *            thl_crit_section;
    int                     thl_crit_section_nlocks;
    int                     thl_nthreads;
    int                     thl_xthreads;
    THREAD_TTYP *           thl_athreads;
    THREAD_ITYP *           thl_athreadids;
};

struct thread_info {    /* thi_ */
    int thi_filler;
};

#define THHSTAT_NONE        0
#define THHSTAT_STARTED     1
#define THHSTAT_TERMINATED  2
#define THHSTAT_START_ERROR 3

struct thread_handle {    /* thh_ */
    struct thread_info *    thh_parent;
    int                     thh_stat;
    struct svarfunction *   thh_svf;
    THREAD_TTYP             thh_os_thread;
    struct svarexec *       thh_svx;
    int                     thh_nsvv;
    struct svarvalue **     thh_asvv;
};

/***************************************************************/
void svare_init_internal_thread_class(struct svarexec * svx);
struct threadlist * threadlist_new(void);
void threadlist_free(struct threadlist * thl);
void threadlist_wait_for_all_threads(struct threadlist * thl);

/***************************************************************/
#endif /* THREADH_INCLUDED */
