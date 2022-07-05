#ifndef SYSXH_INCLUDED
#define SYSXH_INCLUDED
/***************************************************************/
/* sysx.h                                                      */
/***************************************************************/

#if IS_WINDOWS
    typedef int PID_T;
#else
    typedef pid_t PID_T;
#endif

char * parse_command_line(const char * cmd, int * pargc, const char * arg0, int flags);

#define RUNFLAG_STDIN       1
#define RUNFLAG_STDOUT      2
#define RUNFLAG_STDERR      4
#define RUNFLAG_DEBUG       8
#define RUNFLAG_SKIP_PARM1  16

/***************************************************************/
int spawnX(const char * syscmd,
            void * f_info,
            int  (stdin_f) (void *, char * buf, size_t len),
            int  (stdout_f)(void *, const char * buf, size_t len),
            int  (stderr_f)(void *, const char * buf, size_t len),
            char * errmsg,
            int errmsgmax);
/***************************************************************/
int systemX(const char * syscmd,
            void * f_info,
            int  (stdin_f) (void *, char * buf, size_t len),
            int  (stdout_f)(void *, const char * buf, size_t len),
            int  (stderr_f)(void *, const char * buf, size_t len),
            char * errmsg,
            int errmsgmax);
/***************************************************************/

void append_quoted (struct str * sbuf,  const char * cbuf);
int rx_free_processes(void);

void * rx_runx (const char * syscmd,
					int argc,
                    char *const argv [],
                    int  run_flags,
                    char ** envp,
                    int  stdin_f,
                    int  stdout_f,
                    int  stderr_f,
                    void (*post_fork)(void * pfvp),
                    void * pfvp);
int rx_stat(void * vrs);
int rx_exitcode(void ** proclist, int nprocs);
char * rx_errmsg(void * vrs);
char * rx_errmsg_list(void ** proclist, int nprocs);
int rx_wait( void ** proclist, int nprocs, int rxwaitflags);
int rx_check_proclist(void ** proclist, int nprocs, int rxwaitflags);
void rx_free(void ** proclist, int nprocs);
int rx_get_pid(void);
void rx_get_pid_str(void * procptr, char * pidstr, int pidstrmax);
int rx_wait_pidlist(PID_T* pidlist, int npids, int * exit_code, int rxwaitflags);
void rx_get_pidlist(PID_T ** pidlist, int * npids);

#define RUXERR_FAIL         	(-8400)
#define RUXERR_ADDDUP			(-8401)
#define RUXERR_NOPID			(-8402)
#define RUXERR_PIDRUNNING		(-8403)
#define RUXERR_PIDNOTRUNNING	(-8404)
#define RUXERR_NOPIDS			(-8405)
#define RUXERR_BADPID			(-8406)

#define RXWAIT_IGNERR           1
#define RXWAIT_VERBOSE          2

#define RXFLAG_RUNNING          1
#define RXFLAG_STARTED          2
#define RXFLAG_DELETE           4

#if IS_WINDOWS
    #define RUNX    win_runx
    #define EXECVP  _execvp
#else
    #define RUNX    ux_runx
    #define EXECVP  execvp
#endif

#endif /* SYSXH_INCLUDED */

/***************************************************************/
