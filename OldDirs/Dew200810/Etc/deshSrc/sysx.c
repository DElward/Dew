/*
**  Name: sysx.c
**
*/
/*
** History **
** Date     Vers Description
** 10/15/10 0.00 Begin
** 10/19/10 0.01 First version
** 10/20/10 0.02 Fixes

int execve(const char *filename, char *const argv [], char *const envp[]);
    filename must be either a binary executable, or a script starting
    with a line of the form "#! interpreter [arg].

The functions below are front-ends for the function execve()

    int execl(const char *path, const char *arg, ...); 

    int execlp(const char *file, const char *arg, ...); 
        duplicate the actions of the shell in searching for an executable
        file if the specified file name does not contain a slash (/) character.

    int execle(const char *path, const char *arg , ..., char * const envp[]); 

    int execv(const char *path, char *const argv[]); 

    int execvp(const char *file, char *const argv[]);   
        duplicate the actions of the shell in searching for an executable
        file if the specified file name does not contain a slash (/) character.

    If PATH this variable isn't specified, the default path ``:/bin:/usr/bin'' is used. 

*/
#define PROGRAM_NAME    "sysx"
#define PROGRAM_VERSION "0.02"

#define COMPILE_MAIN 0

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>
#include <stdarg.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/stat.h>

#if IS_WINDOWS
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <winsock.h>
    #include <sql.h>
    #include <sqlext.h>
    #include <odbcss.h>
#endif  /* IS_WINDOWS */

#if IS_WINDOWS
    #include <process.h>
    #include <io.h>

    int fork(void) { return (-1); }
    int waitpid(int pid, int *status, int options) { return (-1); }
    #define WIFEXITED(n) (0)
    #define WEXITSTATUS(n) (0)
    #define GETERRNO       GetLastError()

    #define THREAD_RTYP void
    #define THREAD_TTYP HANDLE
    #define THREAD_BEGIN(t,f,p) (!((t)=(THREAD_TTYP)_beginthread((f),4096,(p)))?errno:0)
    #define THREAD_END _endthreadex(0)
#else
    #include <unistd.h>
    #include <pwd.h>
    #include <sys/wait.h>
    #include <pthread.h>

    #define GETERRNO       errno

    #define THREAD_RTYP void*
    #define THREAD_TTYP pthread_t
    #define THREAD_BEGIN(t,f,p) pthread_create(&t,NULL,(f),(p))
    #define THREAD_END { pthread_exit(NULL); return (NULL); }
#endif  /* IS_WINDOWS */

#ifdef _XOPEN_SOURCE
    #include <sys/select.h>
#endif

#include "desh.h"
#include "sysx.h"
#include "cmd.h"

#define DECLARE_BLOCK_SIGNAL sigset_t newMask; sigset_t oldMask
#define BLOCK_SIGNAL(sig)    \
		{sigemptyset(&newMask);sigemptyset(&oldMask);sigaddset(&newMask,(sig));sigprocmask(SIG_BLOCK, &newMask, &oldMask);}
#define UNBLOCK_SIGNAL { sigprocmask(SIG_SETMASK, &oldMask, NULL); }

#define PID_EVENT_TRACKING      0

#if PID_EVENT_TRACKING
#define PID_EVENT_FORK          1
#define PID_EVENT_WAIT_SIGNAL	2
#define PID_EVENT_TIMEOUT       3
#define PID_EVENT_END           4
#define PID_EVENT_CHILD         5
#define PID_EVENT_WAIT_CHILD	6
#define PID_EVENT_ALARM         7

#define PID_EVENT_FILE_NAME	"event_list"

    struct pid_event {
    	int pevt_event;
    	int pevt_pid;
    	int pevt_info;
    };
	#define MAX_PID_EVENTS 8000
    struct pid_event pid_event_list[MAX_PID_EVENTS];
    int g_num_pid_events = 0;
    void add_pid_event(int evt, int evt_pid, int evt_info)
    {
    	if (g_num_pid_events < MAX_PID_EVENTS) {
    		pid_event_list[g_num_pid_events].pevt_event = evt;
    		pid_event_list[g_num_pid_events].pevt_pid   = evt_pid;
    		pid_event_list[g_num_pid_events].pevt_info  = evt_info;
    		g_num_pid_events++;
    	}
    }
    void write_pid_event_list(const char * pelfname)
    {
    	FILE * fref;
    	int ii;

    	fref = fopen(pelfname, "w");
    	if (fref) {
    		for (ii = 0; ii < g_num_pid_events; ii++) {
    			switch (pid_event_list[ii].pevt_event) {
					case PID_EVENT_FORK:
			    		fprintf(fref, "fork %5d\n", pid_event_list[ii].pevt_pid);
						break;
					case PID_EVENT_WAIT_SIGNAL:
			    		fprintf(fref, "wait %5d %d %d\n",
			    				pid_event_list[ii].pevt_pid,
			    				pid_event_list[ii].pevt_info >> 8,
			    				pid_event_list[ii].pevt_info &0xFF);
						break;
					case PID_EVENT_TIMEOUT:
			    		fprintf(fref, "timeout %d\n", pid_event_list[ii].pevt_info);
						break;
					case PID_EVENT_END:
			    		fprintf(fref, "END  %d\n", pid_event_list[ii].pevt_info);
						break;
					case PID_EVENT_CHILD:
			    		fprintf(fref, "child %5d\n", pid_event_list[ii].pevt_pid);
						break;
					case PID_EVENT_WAIT_CHILD:
			    		fprintf(fref, "wait %5d %d\n",
			    				pid_event_list[ii].pevt_pid,
			    				pid_event_list[ii].pevt_info);
						break;
					case PID_EVENT_ALARM:
			    		fprintf(fref, "alarm\n");
						break;
					default:
			    		fprintf(fref, "?%d %d %d\n",
							pid_event_list[ii].pevt_event,
							pid_event_list[ii].pevt_pid,
							pid_event_list[ii].pevt_info);
						break;
    			}
    		}
    		fprintf(fref, "End of event list.\n");
    		fclose(fref);
    	}
    }
#endif

#define CLOSEX(fd) CLOSE(fd)
#define DUPX(fd) DUP(fd)
#define DUP2X(f,g)   DUP2(f,g)

#if IS_WINDOWS
    #define DUP2        _dup2
    #define SPAWNX win_spawnx
#else
    #define DUP2        dup2
    #define SPAWNX ux_spawnx
#endif  /* IS_WINDOWS */

/***************************************************************/
/* stuff for stat()                                            */
#define OSFTYP_NONE     0
#define OSFTYP_FILE     1
#define OSFTYP_DIR      2
#define OSFTYP_OTHR     3

#ifndef S_IFREG
    #define S_IFREG S_ISREG
#endif

#ifndef _S_IFREG
    #define _S_IFREG S_IFREG
#endif

#ifndef _S_IFDIR
    #define _S_IFDIR S_IFDIR
#endif

#ifndef __S_IFDIR
    #define __S_IFDIR _S_IFDIR
#endif

#define TMAX(x,y) ((x) > (y) ? (x) : (y))
#ifndef TRUE
    #define TRUE (1)
#endif
#ifndef FALSE
    #define FALSE (0)
#endif
#ifndef SIGCHLD
    #define SIGCHLD (0)
#endif

typedef int (STDIN_CALLBACK) (void * pInfo, char * buf, size_t len);
typedef int (STDOUT_CALLBACK) (void * pInfo, const char *  buf, size_t len);
typedef int (STDERR_CALLBACK) (void * pInfo, const char *  buf, size_t len);

#define STDIN       0
#define STDOUT      1
#define STDERR      2

/***************************************************************/
void append_quoted (struct str * sbuf,
                    const char * cbuf)
{
    int needsq;
    int ii;
    int jj;
    
    needsq = 0;
    if (!cbuf[0]) needsq = 1; /* zero length parm needs to be "" */
    
    for (ii = 0; !needsq && cbuf[ii]; ii++) {
        if (isspace(cbuf[ii])) needsq = 1;
    }
    if (needsq) APPCH(sbuf, '\"');
    
    for (ii = 0; cbuf[ii]; ii++) {
        if (cbuf[ii] == '\"') {
            jj = ii - 1;
            while (jj >= 0 && cbuf[jj] == '\\') {
                APPCH(sbuf, '\\');
                jj--;
            }
            APPCH(sbuf, '\\');
            APPCH(sbuf, cbuf[ii]);
        } else {
            APPCH(sbuf, cbuf[ii]);
        }
    }
    if (needsq) APPCH(sbuf, '\"');
    APPCH(sbuf, '\0');
}

/***************************************************************/
/****   Windows only                                        ****/
/***************************************************************/
#if IS_WINDOWS

static int g_child_done = 0;

struct CInWinInfo {
    char * mProgramName;
    //char * mProgramParameters;
    void* mpInfo;
    STDIN_CALLBACK  * mStdinProc;
    STDOUT_CALLBACK * mStdoutProc;
    STDERR_CALLBACK * mStderrProc;
    HANDLE mhChild;
    HANDLE mhPipe;
};

struct winInfo {
    int    wi_stat;
    int    wi_flags;
    int    wi_exitcode;
    PID_T  wi_childpid;
    HANDLE wi_hchild;
    char * wi_errmsg;
};
int gwi_npids = 0;
int gwi_mxpid = 0;
struct winInfo ** gwi_winInfo = NULL;

/***************************************************************/
static struct CInWinInfo* new_CInWinInfo(const char * programName,
                                     void * pInfo,
                                     STDIN_CALLBACK  * stdinProc,
                                     STDOUT_CALLBACK * stdoutProc,
                                     STDERR_CALLBACK * stderrProc)
{
    struct CInWinInfo* rri;

    rri = New(struct CInWinInfo, 1);
    rri->mProgramName = Strdup(programName);
    rri->mpInfo         = pInfo;
    rri->mStdinProc     = stdinProc;
    rri->mStdoutProc    = stdoutProc;
    rri->mStderrProc    = stderrProc;
    rri->mhChild        = NULL;
    rri->mhPipe         = NULL;

    return (rri);
}
/***************************************************************/
static void free_CInWinInfo(struct CInWinInfo* rri)
{
    Free(rri->mProgramName);
    Free(rri);
}
/***************************************************************/
static struct winInfo* new_winInfo(void)
{
    struct winInfo* rri;

    rri = New(struct winInfo, 1);
    rri->wi_stat          = 0;
    rri->wi_flags         = 0;
    rri->wi_exitcode      = 0;
    rri->wi_childpid      = 0;
    rri->wi_hchild        = NULL;
    rri->wi_errmsg        = NULL;

    return (rri);
}
/***************************************************************/
static void free_winInfo(struct winInfo* rri)
{

    if (rri->wi_errmsg) Free(rri->wi_errmsg);

    Free(rri);
}
/***************************************************************/
static int find_gwi_pids(PID_T pid)
{
	int pidix;
	int found;

	found = 0;
	pidix = 0;

	while (!found && pidix < gwi_npids) {
		if (gwi_winInfo[pidix]->wi_childpid == pid) found = 1;
		else pidix++;
	}
	if (!found) pidix = -1;

	return (pidix);
}
/***************************************************************/
void add_gwi_pid(struct winInfo* pWinInfo)
{
	if (gwi_npids == gwi_mxpid) {
		if (!gwi_mxpid) gwi_mxpid = 4;
		else gwi_mxpid *= 2;

		gwi_winInfo  = Realloc(gwi_winInfo, struct winInfo*, gwi_mxpid);
	}
	gwi_winInfo[gwi_npids] = pWinInfo;
	gwi_npids++;
}
/***************************************************************/
static int del_gwi_pid(int pidix)
{
	int ii;
    int rtn;

    if (gwi_winInfo[pidix]->wi_hchild) {
        rtn = CloseHandle(gwi_winInfo[pidix]->wi_hchild);
    }

	free_winInfo(gwi_winInfo[pidix]);

	for (ii = pidix + 1; ii < gwi_npids; ii++) {
		gwi_winInfo[ii - 1] = gwi_winInfo[ii];
	}
	gwi_npids--;

	return (0);
}
/***************************************************************/
static int del_all_gwi_completed_pids(void)
{
	int pidix;

	pidix = gwi_npids - 1;
	while (pidix >= 0) {
		if (pidix < gwi_npids && (gwi_winInfo[pidix]->wi_flags & RXFLAG_DELETE)) {
			del_gwi_pid(pidix);
		} else {
			pidix--;
		}
	}

	return (0);
}
/***************************************************************/
static int GetErrorMsg (DWORD ern, const char * proc_name, char * errmsg, int errmsgmax)
{
    LPVOID lpvMessageBuffer;

    FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
            NULL, ern,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&lpvMessageBuffer, 0, NULL);

    strcpy(errmsg, proc_name);
    strncpy(errmsg, (char *)lpvMessageBuffer, errmsgmax - strlen(proc_name));

    LocalFree(lpvMessageBuffer);

    return (RUXERR_FAIL);
}
/***************************************************************/
static char * GetErrorMsgStr(DWORD ern, const char * proc_name)
{
    char * emsg;
    char errmsg[ERRMSG_MAX + 1];

    GetErrorMsg(ern, proc_name, errmsg, ERRMSG_MAX);
    emsg = Strdup(errmsg);

    return (emsg);
}
/***************************************************************/
int rx_free_processes(void) {
    
    int estat;
	int pidix;
    
    estat = 0;

	pidix = gwi_npids - 1;
	while (pidix >= 0) {
		del_gwi_pid(pidix);
		pidix--;
	}
    Free(gwi_winInfo);
    
    return (estat);
}
/***************************************************************/
#ifdef UNUSED
    static int PrepAndLaunchRedirectedChild(struct CInWinInfo* pInWinInfo,
                                        HANDLE hChildStdIn,
                                        HANDLE hChildStdOut,
                                        HANDLE hChildStdErr,
                                        char * errmsg,
                                        int errmsgmax)
    {
        int stat = 0;
        int ern;
        PROCESS_INFORMATION pi;
        STARTUPINFO si;
        char commandLine[256];

        strcpy(commandLine, pInWinInfo->mProgramName);

        // Set up the start up info struct.
        ZeroMemory(&si,sizeof(STARTUPINFO));
        si.cb = sizeof(STARTUPINFO);
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdOutput = hChildStdOut;
        si.hStdInput  = hChildStdIn;
        si.hStdError  = hChildStdErr;
        // Use this if you want to hide the child:
        //     si.wShowWindow = SW_HIDE;
        // Note that dwFlags must include STARTF_USESHOWWINDOW if you want to
        // use the wShowWindow flags.


        // Launch the process that you want to redirect (in this case,
        // Child.exe). Make sure Child.exe is in the same directory as
        // redirect.c launch redirect from a command line to prevent location
        // confusion.
        if (!CreateProcess(
                    NULL,                        // application name
                    commandLine,                 // command line
                    NULL,                        // process security attributes
                    NULL,                        // primary thread security attributes
                    TRUE,                        // handles are inherited
                    CREATE_NEW_PROCESS_GROUP,    // creation flags
                    NULL,                        // use parents environment
                    NULL,                        // use parents current directory
                    &si,                         // STARTUPINFO pointer
                    &pi)) {                      // receives PROCESS_INFORMATION
            ern = GETERRNO;
            stat = GetErrorMsg(ern, "CreateProcess()", errmsg, errmsgmax);
        } else {
            // Close any unnecessary handles.
            if (!CloseHandle(pi.hThread)) {
                ern = GETERRNO;
                stat = GetErrorMsg(ern, "CloseHandle()", errmsg, errmsgmax);
            }
        }

        pInWinInfo->mhChild = pi.hProcess;

        return (stat);
    }
#endif
/***************************************************************/
static DWORD ReadAndHandleOutput(struct CInWinInfo* pInWinInfo,
                                    HANDLE hPipeOutput,
                                    HANDLE hPipeError,
                                    char * errmsg,
                                    int errmsgmax)
{
    CHAR lpBuffer[256];
    DWORD nBytesRead;
    int done;
    DWORD exitCode = 0;
    DWORD wRtn;
    int len;
    int ern;
    HANDLE hList[2];

    done = 0;
    while ((done & 3) != 3) {
        hList[0] = hPipeOutput;
        hList[1] = hPipeError;
        wRtn = WaitForMultipleObjects(2, hList, FALSE, INFINITE);
        if (wRtn == WAIT_FAILED) {
            ern = GETERRNO;
            done = 7;
            exitCode = GetErrorMsg(ern, "WaitForMultipleObjects()", errmsg, errmsgmax);
        } else if (wRtn - WAIT_OBJECT_0 == 0) {
            if (!ReadFile(hPipeOutput,lpBuffer,sizeof(lpBuffer), &nBytesRead,NULL) || !nBytesRead) {
                ern = GETERRNO;
                if (ern == ERROR_BROKEN_PIPE) {
                    done |= 1;
                } else {
                    done = GetErrorMsg(ern, "ReadFile()", errmsg, errmsgmax);
                }
            } else {
                if (!g_child_done) {
                    if (pInWinInfo->mStdoutProc != NULL) {
                        len = pInWinInfo->mStdoutProc(pInWinInfo->mpInfo, lpBuffer, nBytesRead);
                    }
                } else {
                    done = 7;
                    exitCode = PROGRTN_ERR;
                }
            }
        } else if (wRtn - WAIT_OBJECT_0 == 1) {
            if (!ReadFile(hPipeError,lpBuffer,sizeof(lpBuffer), &nBytesRead,NULL) || !nBytesRead) {
                ern = GETERRNO;
                if (ern == ERROR_BROKEN_PIPE) {
                    done |= 2;
                } else {
                    done = GetErrorMsg(ern, "ReadFile()", errmsg, errmsgmax);
                }
            } else {
                if (!g_child_done) {
                    if (pInWinInfo->mStderrProc != NULL) {
                        len = pInWinInfo->mStderrProc(pInWinInfo->mpInfo, lpBuffer, nBytesRead);
                    }
                } else {
                    done = 7;
                    exitCode = PROGRTN_ERR;
                }
            }
        }
    }

    if (done & 4) {
        exitCode = PROGRTN_ERR;
    } else {
        if (! GetExitCodeProcess(pInWinInfo->mhChild, &exitCode)) {
            exitCode = PROGRTN_ERR;
        }
    }

    return (exitCode);
}
/***************************************************************/
static DWORD WINAPI GetAndSendInputThread(LPVOID lpvThreadParam)
{
    struct CInWinInfo* pInWinInfo = (struct CInWinInfo*)lpvThreadParam;
    HANDLE hPipeWrite = pInWinInfo->mhPipe;
    CHAR read_buff[256];
    DWORD nBytesRead;
    DWORD nBytesWrote;
    int done;
    DWORD cStrix;
    int stat = 0;

    done = g_child_done;
    while (!done) {
        nBytesRead = pInWinInfo->mStdinProc(pInWinInfo->mpInfo, read_buff, sizeof(read_buff));
        if (nBytesRead <= 0) {
            done = 1;
        } else {
            cStrix = 0;
            while (!done && cStrix < nBytesRead) {
                if (!WriteFile(hPipeWrite,read_buff + cStrix,nBytesRead - cStrix,&nBytesWrote,NULL)) {
                    done = 1;
                    if (GetLastError() == ERROR_NO_DATA) {
                    // Pipe was closed (normal exit path).
                    } else {
                        // DisplayError("WriteFile");
                    }
                } else {
                    cStrix += nBytesWrote;
                }
                done = g_child_done;
            }
        }
    }

    return (stat);
}
/***************************************************************/
char * copy_envp(char ** envp)
{
    int tlen;
    int ix;
    int ii;
    int jj;
    char * fenv;

    if (!envp) {
        fenv = NULL;
    } else {
        tlen = 0;
        for (ii = 0; envp[ii]; ii++) {
            tlen += Istrlen(envp[ii]) + 1;
        }

        fenv = New(char, tlen + 1);
        ix = 0;
        for (ii = 0; envp[ii]; ii++) {
            for (jj = 0; envp[ii][jj]; jj++) fenv[ix++] = envp[ii][jj];
            fenv[ix++] = '\0';
        }
        fenv[ix] = '\0';
    }

    return (fenv);
}
/***************************************************************/
static void PrepAndLaunchChild(struct winInfo* pWinInfo,
                                 char * cmdfile,
                                 char * cmdparms,
                                    int  run_flags,
                                    char ** envp,
                                    HANDLE hChildStdIn,
                                    HANDLE hChildStdOut,
                                    HANDLE hChildStdErr)
{
    int ern;
    PROCESS_INFORMATION pi;
    STARTUPINFO si;
    char * fenv;
    int cpstat;

    fenv = copy_envp(envp); /* takes 0.011 milliseconds on Caesar release build  */

    // Set up the start up info struct.
    ZeroMemory(&si,sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hChildStdOut;
    si.hStdInput  = hChildStdIn;
    si.hStdError  = hChildStdErr;
    // Use this if you want to hide the child:
    //     si.wShowWindow = SW_HIDE;
    // Note that dwFlags must include STARTF_USESHOWWINDOW if you want to
    // use the wShowWindow flags.


    // Launch the process that you want to redirect (in this case,
    // Child.exe). Make sure Child.exe is in the same directory as
    // redirect.c launch redirect from a command line to prevent location
    // confusion.
    cpstat = CreateProcess(
                cmdfile,                     // application name
                cmdparms,                    // command line
                NULL,                        // process security attributes
                NULL,                        // primary thread security attributes
                TRUE,                        // handles are inherited
                CREATE_NEW_PROCESS_GROUP,    // creation flags
                fenv,                        // use parents environment
                NULL,                        // use parents current directory
                &si,                         // STARTUPINFO pointer
                &pi);
    if (!cpstat) {                      // receives PROCESS_INFORMATION
        ern = GETERRNO;
        pWinInfo->wi_errmsg = GetErrorMsgStr(ern, "CreateProcess()");
        pWinInfo->wi_stat   = RUXERR_FAIL;
    } else {
        if (run_flags & RUNFLAG_DEBUG) {
            fprintf(stderr, "%s: pid=%ld\n", cmdfile, pi.dwProcessId);
        }
        pWinInfo->wi_flags |= RXFLAG_RUNNING;
        pWinInfo->wi_childpid = (PID_T)pi.dwProcessId;
        pWinInfo->wi_hchild   = pi.hProcess;
        // Close any unnecessary handles.
        if (!CloseHandle(pi.hThread)) {
            ern = GETERRNO;
            pWinInfo->wi_errmsg = GetErrorMsgStr(ern, "CloseHandle()");
            pWinInfo->wi_stat   = RUXERR_FAIL;
        }
    }
    Free(fenv);
}
/***************************************************************/
void append_unquoted (struct str * sbuf,
                        const char * cbuf,
                        int * cix)
{
/*
** See: http://msdn.microsoft.com/en-us/library/a1y7w461.aspx
**  
**  Microsoft C startup code uses the following rules when interpreting arguments
**  given on the operating system command line: 
**  
**      Arguments are delimited by white space, which is either a space or a tab.
**  
**      A string surrounded by double quotation marks is interpreted as a single argument,
**      regardless of white space contained within. A quoted string can be embedded in an
**      argument. Note that the caret (^) is not recognized as an escape character or delimiter. 
**  
**      A double quotation mark preceded by a backslash, \", is interpreted as a literal
**      double quotation mark (").
**  
**      Backslashes are interpreted literally, unless they immediately precede a double
**      quotation mark.
**  
**      If an even number of backslashes is followed by a double quotation mark,
**      then one backslash (\) is placed in the argv array for every pair of backslashes (\\),
**      and the double quotation mark (") is interpreted as a string delimiter.
**  
**      If an odd number of backslashes is followed by a double quotation mark,
**      then one backslash (\) is placed in the argv array for every pair of backslashes (\\)
**      and the double quotation mark is interpreted as an escape sequence by the
**      remaining backslash, causing a literal double quotation mark (") to be placed in argv.
**
**
**  Examples:
**      Command-Line Input      argv[1]     argv[2]     argv[3]     desh
**
**      "a b c" d e             a b c       d           e           "a b c" d e         (same)
**
**      "ab\"c" "\\" d          ab"c        \           d           ab""""c \ d
**
**      a\\\b d"e f"g h         a\\\b       de fg       h           a\\\b "de fg" h     (same)
**
**      a\\\"b c d              a\"b        c           d           a\""""b c d
** 
**      a\\\\"b c" d e          a\\b c      d           e           a\\"b c" d e
**  
**
**
**  Bash Examples:
**      Command-Line Input      argv[1]     argv[2]     argv[3]     Same as Windows
**
**      "a b c" d e             a b c       d           e           Yes
**
**      "ab\"c" "\\" d          ab"c        \           d           Yes
**
**      a\\\b d"e f"g h         a\b         de fg       h           No
**
**      a\\\"b c d              a\"b        c           d           Yes
** 
**      a\\\\"b c" d e          a\\b c      d           e           Yes
**  
*/
    int eol;
    int nbs;

    while (isspace(cbuf[*cix])) (*cix)++;
    if (cbuf[*cix] == '\"') {
        (*cix)++;
        eol = 0;
        while (!eol) {
            if (!cbuf[*cix]) {
                eol = 1;
            } else if (cbuf[*cix] == '\\') {
                nbs = 0;
                while (cbuf[*cix] == '\\') { nbs++; (*cix)++; }

                if (cbuf[*cix] == '\"') {
                    if (nbs & 1) {
                        nbs = (nbs - 1) >> 1;
                        while (nbs) { nbs--; APPCH(sbuf, '\\'); }
                        APPCH(sbuf, cbuf[*cix]);
                        (*cix)++;
                    } else {
                        nbs >>= 1;
                        while (nbs) { nbs--; APPCH(sbuf, '\\'); }
                        eol = 1;
                        (*cix)++;
                    }
                } else {
                    while (nbs) { nbs--; APPCH(sbuf, '\\'); }
                }
            } else if (cbuf[*cix] == '\"') {
                eol = 1;
                (*cix)++;
            } else {
                APPCH(sbuf, cbuf[*cix]);
                (*cix)++;
            }
        }
    } else {
        eol = 0;
        while (!eol) {
            if (!cbuf[*cix] || isspace(cbuf[*cix])) {
                eol = 1;
            } else if (cbuf[*cix] == '\\') {
                nbs = 0;
                while (cbuf[*cix] == '\\') { nbs++; (*cix)++; }

                if (cbuf[*cix] == '\"') {
                    if (nbs & 1) {
                        nbs = (nbs - 1) >> 1;
                        while (nbs) { nbs--; APPCH(sbuf, '\\'); }
                    }
                    while (nbs) { nbs--; APPCH(sbuf, '\\'); }
                    APPCH(sbuf, cbuf[*cix]);
                    (*cix)++;
                } else {
                    while (nbs) { nbs--; APPCH(sbuf, '\\'); }
                }
            } else {
                APPCH(sbuf, cbuf[*cix]);
                (*cix)++;
            }
        }
    }
    APPCH(sbuf, '\0');
}
/***************************************************************/
void win_proc_args (const char * syscmd,    // In : Command from FTYPE after sustitution of %1
                    char *const argv [],    // In : Parsed command line
                    int  run_flags,         // In : Run flags
                    struct str * cmdstr,    // Out: First parm of call to CreateProcess()
                    struct str * parmstr)   // Out: Second parm of call to CreateProcess()
{
/*
**  cmd: decat dec.txt
**      IN:
**          syscmd : C:\D\bin\decat.EXE
**          argv[0]: decat
**          argv[1]: dec.txt
**      OUT:
**          cmdstr : C:\D\bin\decat.EXE
**          parmstr: decat dec.txt
**
**  cmd: autoruns           <-- Assumes .chm is in PATHEXT
**      IN:
**          syscmd : "C:\WINDOWS\hh.exe" C:\D\bin\autoruns.chm"
**          argv[0]: autoruns
**      OUT:
**          cmdstr : "C:\WINDOWS\hh.exe"
**          parmstr: autoruns C:\D\bin\autoruns.chm"
**
**  cmd: KSJB_Logo          <-- Assumes .jpg is in PATHEXT
**      IN:
**          syscmd : C:\WINDOWS\system32\rundll32.exe C:\WINDOWS\system32\shimgvw.dll,ImageView_Fullscreen C:\D\bin\KSJB_Logo.jpg
**          argv[0]: KSJB_Logo
**      OUT:
**          cmdstr : C:\WINDOWS\system32\rundll32.exe
**          parmstr: KSJB_Logo C:\WINDOWS\system32\shimgvw.dll,ImageView_Fullscreen C:\D\bin\KSJB_Logo.jpg
*/
    int ix;
    int ii;
    char * cargv;
    char ** scargv;
    int argc;

    init_str(cmdstr, 0);
    init_str(parmstr, 0);

    ix = 0;
    cargv = parse_command_line(syscmd + ix, &argc, NULL, 0);
    scargv = (char**)cargv;
    if (argc) {
        copy_char_str(cmdstr, scargv[0]);
        if (run_flags & RUNFLAG_SKIP_PARM1) {
            append_quoted(parmstr, argv[0]);
        } else {
            append_quoted(parmstr, scargv[0]);
        }
    }

    for (ii = 1; ii < argc; ii++) {
        APPCH(parmstr, ' ');
        append_quoted(parmstr, scargv[ii]);
    }

    for (ii = 1; argv[ii]; ii++) {
        APPCH(parmstr, ' ');
        append_quoted(parmstr, argv[ii]);
    }

    Free(cargv);
    APPCH(parmstr, '\0');
}
/***************************************************************/
void win_runx (const char * syscmd,
					int argc,
                    char *const argv [],
                    int  run_flags,
                    char ** envp,
                    int  stdin_f,
                    int  stdout_f,
                    int  stderr_f,
                    struct winInfo* pWinInfo)
{
/*
** See MSDN help under: "Creating a Child Process with Redirected Input and Output"
*/
    HANDLE hOutputWrite;
    HANDLE hInputRead;
    HANDLE hErrorWrite;
    struct str cmdstr;
    struct str parmstr;
    int  i_stdin_f;
    int  i_stdout_f;
    int  i_stderr_f;

    /* make inheritable files */
    i_stdin_f  = DUPX(stdin_f);
    i_stdout_f = DUPX(stdout_f);
    i_stderr_f = DUPX(stderr_f);

    hInputRead   = (HANDLE)_get_osfhandle(i_stdin_f);
    hOutputWrite = (HANDLE)_get_osfhandle(i_stdout_f);
    hErrorWrite  = (HANDLE)_get_osfhandle(i_stderr_f);

    win_proc_args(syscmd, argv, run_flags, &cmdstr, &parmstr);

    if (run_flags & RUNFLAG_DEBUG) {
        fprintf(stderr, "%s| %s\n", cmdstr.str_buf, parmstr.str_buf);
    }
    PrepAndLaunchChild(pWinInfo, cmdstr.str_buf, parmstr.str_buf, run_flags, envp,
                        hInputRead, hOutputWrite, hErrorWrite);
/*
    if (!stat) {
        if (WaitForSingleObject(pWinInfo->wi_hchild,INFINITE) == WAIT_FAILED) {
            if (! GetExitCodeProcess(pWinInfo->wi_hchild, &exitCode)) {
                exitCode = PROGRTN_ERR;
            }
        }
    }
*/

    CLOSEX(i_stdin_f);
    CLOSEX(i_stdout_f);
    CLOSEX(i_stderr_f);

    free_str_buf(&cmdstr);
    free_str_buf(&parmstr);
}
/***************************************************************/
void * rx_runx (const char * syscmd,
					int argc,
                    char *const argv [],
                    int  run_flags,
                    char ** envp,
                    int  stdin_f,
                    int  stdout_f,
                    int  stderr_f,
                    void (*post_fork)(void * pfvp),
                    void * pfvp)
{
    struct winInfo* pWinInfo;
    void * vrs;

#if IS_DEBUG_PARM0
    if (post_fork) post_fork(pfvp);
#endif

    pWinInfo = new_winInfo();

    win_runx(syscmd,
    		argc,
            argv,
            run_flags,
            envp,
            stdin_f,
            stdout_f,
            stderr_f,
            pWinInfo);

    vrs = pWinInfo;

    return (vrs);
}
/***************************************************************/
int rx_stat(void * vrs) {

    struct winInfo* pWinInfo = (struct winInfo*)vrs;

    return (pWinInfo->wi_stat);
}
/***************************************************************/
int rx_exitcode(void ** pidlist, int npids) {

    struct winInfo* pWinInfo;
    int ii;
    int exit_code;

    exit_code = 0;
    for (ii = 0; ii < npids; ii++) {
        pWinInfo = (struct winInfo*)(pidlist[ii]);
        exit_code |= pWinInfo->wi_exitcode; /* What to do? */
    }

    return (exit_code);
}
/***************************************************************/
char * rx_errmsg(void * vrs) {

    struct winInfo* pWinInfo = (struct winInfo*)vrs;

    return (pWinInfo->wi_errmsg);
}
/***************************************************************/
char * rx_errmsg_list(void ** pidlist, int npids) {

    struct winInfo* pWinInfo;
    int ii;
    char * err_msg;

    err_msg = NULL;

    for (ii = 0; ii < npids; ii++) {
        pWinInfo = (struct winInfo*)(pidlist[ii]);
        if (!err_msg) err_msg = pWinInfo->wi_errmsg;
    }

    return (err_msg);
}
/***************************************************************/
void rx_free(void ** pidlist, int npids) {

    struct winInfo* pWinInfo;
    int ii;

    for (ii = 0; ii < npids; ii++) {
        pWinInfo = (struct winInfo*)(pidlist[ii]);
        free_winInfo(pWinInfo);
    }
}
/***************************************************************/
int rx_wait(void ** pidlist, int npids, int rxwaitflags) {

    struct winInfo* pWinInfo;
    HANDLE *lpHandles;
    DWORD exitCode = 0;
    int ii;
    int np;
    int ern;
    int rtn;
    int rxstat;

    if (!npids) {
        return (RUXERR_NOPIDS);
    }

    rxstat = 0;

    if (npids == 1) {
        pWinInfo = (struct winInfo*)(pidlist[0]);

        if (pWinInfo->wi_flags & RXFLAG_RUNNING) {
            if (rxwaitflags & RXWAIT_VERBOSE) {
                fprintf(stderr, "Waiting for single pid: %d\n", (int)pWinInfo->wi_childpid);
                fflush(stderr);
            }

            rtn = WaitForSingleObject(pWinInfo->wi_hchild,INFINITE);
            if (rtn == WAIT_FAILED) {
                ern = GETERRNO;
                pWinInfo->wi_errmsg = GetErrorMsgStr(ern, "WaitForSingleObject()");
                pWinInfo->wi_stat   = RUXERR_FAIL;
                if (rxwaitflags & RXWAIT_VERBOSE) {
                    fprintf(stderr, "Single wait failed with error %d\n", ern);
                    fflush(stderr);
                }
            } else {
                if (! GetExitCodeProcess(pWinInfo->wi_hchild, &exitCode)) {
                    ern = GETERRNO;
                    pWinInfo->wi_errmsg = GetErrorMsgStr(ern, "GetExitCodeProcess()");
                    pWinInfo->wi_stat   = RUXERR_FAIL;
                    exitCode = PROGRTN_ERR;
                } else {
                     pWinInfo->wi_exitcode = exitCode;
                }
            }
            pWinInfo->wi_hchild = NULL;
            pWinInfo->wi_flags ^= RXFLAG_RUNNING; /* Clear RXFLAG_RUNNING */
            CloseHandle(pWinInfo->wi_hchild);
            rxstat = pWinInfo->wi_stat;
        }
    } else {
        np = 0;
        lpHandles = New(HANDLE, npids);
        for (ii = 0; ii < npids; ii++) {
            pWinInfo = (struct winInfo*)(pidlist[ii]);
            if (pWinInfo->wi_flags & RXFLAG_RUNNING) {
                if (rxwaitflags & RXWAIT_VERBOSE) {
                    if (!np) {
                        fprintf(stderr, "Waiting for multiple pids:");
                    }
                    fprintf(stderr, " %d", (int)pWinInfo->wi_childpid);
                }
                lpHandles[np] = pWinInfo->wi_hchild;
                np++;
            }
        }

        if (rxwaitflags & RXWAIT_VERBOSE) {
            fprintf(stderr, "\n");
            fflush(stderr);
        }

        if (np) {
            rtn = WaitForMultipleObjects(np, lpHandles, 1, INFINITE);
            if (rtn == WAIT_FAILED) {
                ern = GETERRNO;
                pWinInfo = (struct winInfo*)(pidlist[0]);
                pWinInfo->wi_errmsg = GetErrorMsgStr(ern, "WaitForMultipleObjects()");
                pWinInfo->wi_stat   = RUXERR_FAIL;
                if (rxwaitflags & RXWAIT_VERBOSE) {
                    fprintf(stderr, "Multiple wait failed with error %d\n", ern);
                    fflush(stderr);
                }
            } else {
                for (ii = 0; ii < npids; ii++) {
                    pWinInfo = (struct winInfo*)(pidlist[ii]);
                    if (pWinInfo->wi_flags & RXFLAG_RUNNING) {
                        if (! GetExitCodeProcess(pWinInfo->wi_hchild, &exitCode)) {
                            ern = GETERRNO;
                            pWinInfo->wi_errmsg = GetErrorMsgStr(ern, "GetExitCodeProcess()");
                            pWinInfo->wi_stat   = RUXERR_FAIL;
                            exitCode            = PROGRTN_ERR;
                        } else {
                            pWinInfo->wi_exitcode = exitCode;
                        }
                        pWinInfo->wi_flags ^= RXFLAG_RUNNING; /* Clear RXFLAG_RUNNING */
                        CloseHandle(pWinInfo->wi_hchild);
                        pWinInfo->wi_hchild = NULL;
                    }
                    if (!rxstat) rxstat = pWinInfo->wi_stat;
                }
            }
        }
        Free(lpHandles);
    }

    return (rxstat);
}
/***************************************************************/
int rx_wait_pidlist(PID_T* pidlist, int npids, int * exit_code, int rxwaitflags) {

    struct winInfo* pWinInfo;
    int pix;
    int ii;
    int rxstat;
    int piderr;
    int l_npids;
    int l_mxpid;
    struct winInfo ** l_winInfo;

    rxstat  = 0;
    piderr  = 0;
    (*exit_code) = 0;
    l_npids = 0;
    l_mxpid = 0;
    l_winInfo = NULL;

    for (ii = 0; ii < npids; ii++) {
        pix = find_gwi_pids(pidlist[ii]);
        if (pix < 0) {
            piderr = 1;
        } else {
            pWinInfo = gwi_winInfo[pix];
            if (pWinInfo->wi_flags & RXFLAG_RUNNING) {
	            if (l_npids == l_mxpid) {
		            if (!l_mxpid) l_mxpid = 4;
		            else l_mxpid *= 2;

		            l_winInfo  = Realloc(l_winInfo, struct winInfo*, l_mxpid);
	            }
	            l_winInfo[l_npids] = pWinInfo;
	            l_npids++;
            }
        }
    }

    if (l_npids) {
        rxstat = rx_wait(l_winInfo, l_npids, rxwaitflags);

        for (ii = 0; ii < l_npids; ii++) {
            if (!(l_winInfo[ii]->wi_flags & RXFLAG_RUNNING)) {
                (*exit_code) |= l_winInfo[ii]->wi_exitcode; /* What to do? */
                l_winInfo[ii]->wi_flags |= RXFLAG_DELETE;
            }
        }
        Free(l_winInfo);
    }
    del_all_gwi_completed_pids();

    if (piderr && !rxstat && !(rxwaitflags & RXWAIT_IGNERR)) rxstat = RUXERR_BADPID;

    return (rxstat);
}
/***************************************************************/
int rx_check_proclist(void ** proclist, int nprocs, int rxwaitflags)
{
/*
 * Called after START command.
 */

	int rxstat;
	int pix;
	struct winInfo* pWinInfo;

	rxstat = 0;
	pix = 0;

	while (pix < nprocs) {
		pWinInfo = (struct winInfo*)(proclist[pix]);
		if (find_gwi_pids(pWinInfo->wi_childpid) < 0) {
			add_gwi_pid(pWinInfo);
		}
		pix++;
	}

	return (rxstat);
}
/***************************************************************/
void rx_get_pidlist(PID_T ** pidlist, int * npids) {
/*
** Assumes pidlist & npids have been initialized.
*/

    int ii;
    int mxpid;

    mxpid = (*npids);

	for (ii = 0; ii < gwi_npids; ii++) {
        if (gwi_winInfo[ii]->wi_flags & RXFLAG_RUNNING) {
	        if ((*npids) == mxpid) {
		        if (!mxpid) mxpid = 4;
		        else mxpid *= 2;

		        (*pidlist)  = Realloc((*pidlist), PID_T, mxpid);
	        }
	        (*pidlist)[(*npids)] = gwi_winInfo[ii]->wi_childpid;
	        (*npids)++;
        }
	}
}
/***************************************************************/
int rx_get_pid(void) {

    return GetCurrentProcessId();
}
/***************************************************************/
void rx_get_pid_str(void * pidptr, char * pidstr, int pidstrmax) {

    struct winInfo* pWinInfo;
    char pidbuf[16];

    pWinInfo = (struct winInfo*)pidptr;
    sprintf(pidbuf, "%d", pWinInfo->wi_childpid);
    strncpy(pidstr, pidbuf, pidstrmax);
}
/***************************************************************/
#else  /* IS_WINDOWS */
/***************************************************************/
/***************************************************************/
struct uxInfo {
    int    ui_stat;
    int	   ui_flags;
    int    ui_exitcode;
    PID_T  ui_childpid;
    char * ui_errmsg;
};
int gux_npids = 0;
int gux_mxpid = 0;
struct uxInfo ** gux_uxInfo = NULL;
#ifdef USE_SIGUSR1
int g_exec_problems = 0;
#endif

#define SIGALRM_CHECK_SECONDS   2

#define MOVEFDX(fromfd, tofd) (DUP2X(fromfd, tofd) >= 0  &&  CLOSEX(fromfd) == 0)

/***************************************************************/
static struct uxInfo* new_uxInfo(void)
{
    struct uxInfo* pUxInfo;

    pUxInfo = New(struct uxInfo, 1);
    pUxInfo->ui_stat          = 0;
    pUxInfo->ui_flags         = 0;
    pUxInfo->ui_exitcode      = 0;
    pUxInfo->ui_childpid      = 0;
    pUxInfo->ui_errmsg        = NULL;

    return (pUxInfo);
}
/***************************************************************/
static void free_uxInfo(struct uxInfo* pUxInfo)
{
    if (pUxInfo->ui_errmsg) Free(pUxInfo->ui_errmsg);

    Free(pUxInfo);
}
/***************************************************************/
static int find_gux_pids(PID_T pid)
{
	int pidix;
	int found;

	found = 0;
	pidix = 0;

	while (!found && pidix < gux_npids) {
		if (gux_uxInfo[pidix]->ui_childpid == pid) found = 1;
		else pidix++;
	}
	if (!found) pidix = -1;

	return (pidix);
}
/***************************************************************/
static struct uxInfo* add_gux_pids(void)
{
	struct uxInfo* pUxInfo;

    pUxInfo = new_uxInfo();

    if (gux_npids == gux_mxpid) {
        if (!gux_mxpid) gux_mxpid = 4;
        else gux_mxpid *= 2;

        gux_uxInfo  = Realloc(gux_uxInfo, struct uxInfo*, gux_mxpid);
    }
    gux_uxInfo[gux_npids] = pUxInfo;
    gux_npids++;

	return (pUxInfo);
}
/***************************************************************/
static int del_gux_pids(int pidix)
{
	int ii;

	free_uxInfo(gux_uxInfo[pidix]);

	for (ii = pidix + 1; ii < gux_npids; ii++) {
		gux_uxInfo[ii - 1] = gux_uxInfo[ii];
	}
	gux_npids--;
    gux_uxInfo[gux_npids] = NULL;

	return (0);
}
/***************************************************************/
static void del_all_gux_completed_pids(void)
{
	/*
	 * Not sure if this needs BLOCK_SIGNAL(SIGCHLD) or not.  It seems to work
	 * well without it, but I'll leave it because I think you need it if you
	 * wait for some pids while others are still running.
	 */

	int pidix;
    DECLARE_BLOCK_SIGNAL;

	BLOCK_SIGNAL(SIGCHLD);
	pidix = gux_npids - 1;
	while (pidix >= 0) {
		if (pidix < gux_npids && (gux_uxInfo[pidix]->ui_flags & RXFLAG_DELETE)) {
			del_gux_pids(pidix);
		} else {
			pidix--;
		}
	}
	UNBLOCK_SIGNAL;
}
/***************************************************************/
#ifdef USE_SIGUSR1
static char * GetErrorMsgStr(int ern, const char * proc_name)
{
    char * emsg;
    char errmsg[ERRMSG_MAX + 1];

    sprintf(errmsg, "%s: %s", proc_name, strerror(ern));
    emsg = Strdup(errmsg);

    return (emsg);
}
/***************************************************************/
static void set_exec_error(struct uxInfo* pUxInfo, int exec_error)
{
    pUxInfo->ui_errmsg = GetErrorMsgStr(exec_error, "exec()");
    pUxInfo->ui_stat   = RUXERR_FAIL;
}
#endif
/***************************************************************/
static struct uxInfo* set_exitcode_gux_pids(PID_T pid, int exitcode)
{
    /*
     * Called after receiving SIGCHLD
     */
	int pidix;
	struct uxInfo* pUxInfo;

    pidix = find_gux_pids(pid);
    if (pidix < 0) {
        /* forked process completed before pid was added to gux_uxInfo */
        pidix = find_gux_pids(0);
        if (pidix >= 0) {
            gux_uxInfo[pidix]->ui_childpid = pid;
            gux_uxInfo[pidix]->ui_flags    = RXFLAG_RUNNING;
        }
    }

	if (pidix < 0) {
		pUxInfo = NULL;
    } else {
		pUxInfo = gux_uxInfo[pidix];
		if (pUxInfo->ui_flags & RXFLAG_RUNNING) {
            pUxInfo->ui_flags ^= RXFLAG_RUNNING;
            pUxInfo->ui_exitcode = exitcode;
        }

#ifdef USE_SIGUSR1
        if (g_exec_problems & (exitcode & 128)) {
            set_exec_error(pUxInfo, (exitcode & 0x7f));
            g_exec_problems--;
        }
#endif
	}
    
	return (pUxInfo);
}
/***************************************************************/
int rx_free_processes(void) {
    
    int estat;
	int pidix;
    
    estat = 0;

	pidix = gux_npids - 1;
	while (pidix >= 0) {
		del_gux_pids(pidix);
		pidix--;
	}
    Free(gux_uxInfo);
#if PID_EVENT_TRACKING
	add_pid_event(PID_EVENT_END, 0, 0);
   	write_pid_event_list(PID_EVENT_FILE_NAME);
#endif
    
    return (estat);
}
/***************************************************************/
int remap_pipe_stdin_stdout_stderr(int rpipe, int wpipe, int epipe)
{
    int msame = 0;
    int mdiff = 0;
    int mconf = 0;
    int result = 0;
    int ern = 0;

    if (rpipe == STDIN)      msame |= 1;
    else if (rpipe > STDERR) mdiff |= 1;
    else                     mconf |= 1;

    if (wpipe == STDOUT)                        msame |= 2;
    else if (wpipe == STDIN || wpipe == STDERR) mconf |= 2;
    else                                        mdiff |= 2;

    if (epipe == STDERR)     msame |= 4;
    else if (epipe < STDERR) mconf |= 4;
    else                     mdiff |= 4;

    result = TRUE;

    if (mconf & 1) result = result && DUP2X(rpipe, STDIN);
    if (mconf & 2) result = result && DUP2X(wpipe, STDOUT);
    if (mconf & 4) result = result && DUP2X(epipe, STDERR);

    if (mdiff & 1) result = result && MOVEFDX(rpipe, STDIN);

    if (mdiff & 2) {
    	if (wpipe == epipe) {	/* 1>&2 */
    		result = result && MOVEFDX(epipe, STDOUT); /* (mdiff & 4) must be true */
    	} else {
    		result = result && MOVEFDX(wpipe, STDOUT);
    	}
    }

    if (mdiff & 4) {
    	if (wpipe == epipe) {	/* 2>&1 */
    		result = result && DUP2X(STDOUT, STDERR);
    	} else {
    		result = result && MOVEFDX(epipe, STDERR);
    	}
    }

    return  ern * 1000 + result;
}
/***************************************************************/
static void child_signal(int sig) {
    /*
     ** If more than one child terminates, it is possible only one SIGCHLD is generated.
     */
	PID_T pid;
	int exitcode;
	int wcount;

	wcount = 0;
    if (g_show_signals) { fprintf(stderr, "signal child_signal()\n"); fflush(stderr); }

	do {
		wcount++;
		pid = waitpid(-1, &exitcode, WNOHANG);
		if (pid > 0) {
#if PID_EVENT_TRACKING
    add_pid_event(PID_EVENT_WAIT_SIGNAL, pid, (wcount << 8) | (WEXITSTATUS(exitcode) & 0xFF));
#endif
			set_exitcode_gux_pids(pid, WEXITSTATUS(exitcode));
		}
	} while (pid > 0);
}
/***************************************************************/
#ifdef USE_SIGUSR1
static void user1_signal(int sig)
/*
*/
{
    g_exec_problems += 1;
}
#endif
/***************************************************************/
static void arm_child_signal(void) {

    struct sigaction old_action;
    struct sigaction new_action;

    /* Set up the structure to specify the new action. */
    new_action.sa_handler = child_signal;
    sigemptyset (&new_action.sa_mask);
    new_action.sa_flags = 0;

    /* signal(SIGCHLD , child_signal); */
    sigaction (SIGCHLD, &new_action, &old_action);
}
/***************************************************************/
struct uxInfo* ux_runx (const char * syscmd,
					int argc,
                    char *const argv [],
                    int  run_flags,
                    char ** envp,
                    int  stdin_f,
                    int  stdout_f,
                    int  stderr_f,
                    void (*post_fork)(void * pfvp),
                    void * pfvp)
{
	int rstat;
	int chldpid;
    struct uxInfo* pUxInfo;
    DECLARE_BLOCK_SIGNAL;

    rstat = 0;
    pUxInfo = add_gux_pids();

    arm_child_signal();

#ifdef USE_SIGUSR1
    signal(SIGUSR1 , user1_signal);
#endif
    chldpid = fork();

    if (!chldpid) {
        int remaprtn;
        int rtn;
        int excd;
        int ern;

        remaprtn = remap_pipe_stdin_stdout_stderr(stdin_f, stdout_f, stderr_f);
        if (post_fork) post_fork(pfvp);
        rtn = execve(syscmd, argv, envp);

        /* Only gets here if execve() fails */
        ern = errno;
#ifdef USE_SIGUSR1
        kill(getppid(), SIGUSR1);
#endif

        excd = 128;
        if (!(ern & 0xff80)) excd |= (ern & 0x7f);

        exit(excd);
    }
#if PID_EVENT_TRACKING
    add_pid_event(PID_EVENT_FORK, chldpid, 0);
#endif

    BLOCK_SIGNAL(SIGCHLD);
    if (!pUxInfo->ui_childpid) {
		pUxInfo->ui_flags    = RXFLAG_RUNNING;
		pUxInfo->ui_childpid = chldpid;
    }
#if PID_EVENT_TRACKING
    else {
          add_pid_event(PID_EVENT_CHILD, chldpid, 0);
	}
#endif
    UNBLOCK_SIGNAL;

    return (pUxInfo);
}
/***************************************************************/
void * rx_runx (const char * syscmd,
				int argc,
                char *const argv [],
                int  run_flags,
                char ** envp,
                int  stdin_f,
                int  stdout_f,
                int  stderr_f,
                void (*post_fork)(void * pfvp),
                void * pfvp)
{
    struct uxInfo* pUxInfo;
    char msg[256];

    sprintf(msg, "rx_runx: %s\n", syscmd);

    pUxInfo = ux_runx(
    		syscmd,
    		argc,
            argv,
            run_flags,
            envp,
            stdin_f,
            stdout_f,
            stderr_f,
            post_fork,
            pfvp);

    return (pUxInfo);
}
/***************************************************************/
int rx_stat(void * vrs) {
    
	struct uxInfo* pUxInfo = (struct uxInfo*)vrs;

    return (pUxInfo->ui_stat);
}
/***************************************************************/
int rx_exitcode(void ** proclist, int nprocs) {
    
	struct uxInfo* pUxInfo;
    int ii;
    int exit_code;

    exit_code = 0;
    for (ii = 0; ii < nprocs; ii++) {
    	pUxInfo = (struct uxInfo*)(proclist[ii]);
        exit_code |= pUxInfo->ui_exitcode; /* What to do? */
    }

    return (exit_code);
}
/***************************************************************/
char * rx_errmsg(void * vrs) {
    
	struct uxInfo* pUxInfo = (struct uxInfo*)vrs;

    return (pUxInfo->ui_errmsg);
}
/***************************************************************/
char * rx_errmsg_list(void ** proclist, int nprocs) {
    
	struct uxInfo* pUxInfo;
    int ii;
    char * err_msg;

    err_msg = NULL;

    for (ii = 0; ii < nprocs; ii++) {
    	pUxInfo = (struct uxInfo*)(proclist[ii]);
        if (!err_msg) err_msg = pUxInfo->ui_errmsg;
    }

    return (err_msg);
}
/***************************************************************/
void rx_free(void ** proclist, int nprocs) {

    int ii;
	struct uxInfo* pUxInfo;
    
    for (ii = 0; ii < nprocs; ii++) {
        pUxInfo = (struct uxInfo*)(proclist[ii]);
        if (!(pUxInfo->ui_flags & RXFLAG_RUNNING)) {
            pUxInfo->ui_flags |= RXFLAG_DELETE;
        }
    }
    
    del_all_gux_completed_pids();
}
/***************************************************************/
#ifdef OLD_WAY
int rx_wait(void ** proclist, int nprocs, int rxwaitflags) {
    
    int rxstat;
    int nrun;
    int ii;
    int rtn;
    int rxfail;
	struct uxInfo* pUxInfo;
#if IS_MACINTOSH
    PID_T pid;
    int exitcode;
#endif

    if (!nprocs) {
        return(RUXERR_NOPIDS);
    }
    
    rxstat = 0;

    do {
    	nrun = 0;
        rxfail = 0;
        for (ii = 0; ii < nprocs; ii++) {
        	pUxInfo = (struct uxInfo*)(proclist[ii]);
            if (pUxInfo->ui_flags & RXFLAG_RUNNING) nrun++;
            if (!rxfail) rxfail = pUxInfo->ui_stat;
        }
#if IS_MACINTOSH
        if (nrun) {
            pid = waitpid(-1, &exitcode, WNOHANG); /* Appears to be necessary */
            if (pid > 0) {
                pUxInfo = set_exitcode_gux_pids(pid, WEXITSTATUS(exitcode));
                if (!rxfail) rxfail = pUxInfo->ui_stat;
            } else {
        	    rtn = sleep(8); /* Wait for SIGCHLD */
            }
        }
#else
        if (nrun) {
       	    rtn = pause(); /* Wait for SIGCHLD */
#if PID_EVENT_TRACKING
       	    if (!rtn) {
       	    	add_pid_event(PID_EVENT_TIMEOUT, 0, 4);
       	    	write_pid_event_list(PID_EVENT_FILE_NAME);
       	    }
#endif
        }
#endif
    } while (!rxstat && nrun);

    if (!rxstat) rxstat = rxfail;
    
    return (rxstat);
}
#endif
/***************************************************************/
#ifdef USE_ALARM
static void arm_alarm_signal(void) {

    struct sigaction old_action;
    struct sigaction new_action;

    void alarm_signal(int sig);

    /* Set up the structure to specify the new action. */
    new_action.sa_handler = alarm_signal;
    sigemptyset (&new_action.sa_mask);
    new_action.sa_flags = 0;

    /* signal(SIGALRM , alarm_signal); */
    sigaction (SIGALRM, &new_action, &old_action);

    alarm(SIGALRM_CHECK_SECONDS);
}
/***************************************************************/
void alarm_signal(int sig) {

#if PID_EVENT_TRACKING
    add_pid_event(PID_EVENT_ALARM, 0, 0);
/*
	add_pid_event(PID_EVENT_END, 0, 0);
   	write_pid_event_list(PID_EVENT_FILE_NAME);
    exit(0);
*/
#endif
    if (g_show_signals) { fprintf(stderr, "signal alarm_signal()\n"); fflush(stderr); }
    arm_alarm_signal();
}
/***************************************************************/
int rx_wait(void ** proclist, int nprocs, int rxwaitflags) {

    int rxstat;
    int nrun;
    int ii;
    int ern;
    int gnrun;
    int rxfail;
    int alarmed;
	struct uxInfo* pUxInfo;
    PID_T pid;
    int exitcode;

    if (!nprocs) {
        return(RUXERR_NOPIDS);
    }

    rxstat = 0;
    rxfail = 0;
    alarmed = 0;

    do {
    	nrun = 0;
         for (ii = 0; ii < nprocs; ii++) {
        	pUxInfo = (struct uxInfo*)(proclist[ii]);
            if (pUxInfo->ui_flags & RXFLAG_RUNNING) nrun++;
            if (!rxfail) rxfail = pUxInfo->ui_stat;
        }

        if (nrun) {
            gnrun = 0;
            for (ii = 0; ii < gux_npids; ii++) {
                if (gux_uxInfo[ii]->ui_flags & RXFLAG_RUNNING) gnrun++;
            }
            if (nrun < gnrun) {
                arm_alarm_signal();
                alarmed = 1;
            }
            pid = waitpid(-1, &exitcode, 0);
#if PID_EVENT_TRACKING
    add_pid_event(PID_EVENT_WAIT_CHILD, pid, (pid<0)?errno:(WEXITSTATUS(exitcode) & 0xFF));
#endif
            if (pid > 0) {
                pUxInfo = set_exitcode_gux_pids(pid, WEXITSTATUS(exitcode));
                if (!rxfail) rxfail = pUxInfo->ui_stat;
            } else if (pid < 0) {
            	ern = errno;
            	if (ern == ECHILD) nrun = 0; /* no children */
            	else if (ern == EINTR) {
                    pid = 0;
                } else {
                    rxstat = -1;
                }
            } else {
            	rxstat = -2; /* should never happen - waitpid() does not return 0 */
            }
        }
    } while (!rxstat && nrun);

    if (alarmed) alarm(0); /* cancel alarm */

    if (!rxstat) rxstat = rxfail;

    return (rxstat);
}
#endif /* USE_ALARM */
/***************************************************************/
int rx_wait(void ** proclist, int nprocs, int rxwaitflags) {

    int rxstat;
    int ii;
    PID_T wpid;
    int exitcode;
	struct uxInfo* pUxInfo;

    if (!nprocs) {
        return(RUXERR_NOPIDS);
    }

    rxstat = 0;

    for (ii = 0; ii < nprocs; ii++) {
        pUxInfo = (struct uxInfo*)(proclist[ii]);
        /*
         * waitpid() could end with EINTR for another pid here so we need to loop
         * as long as our pid is running.
        */
        while (pUxInfo->ui_flags & RXFLAG_RUNNING) {
            wpid = waitpid(pUxInfo->ui_childpid, &exitcode, 0);
            if (wpid > 0) {
                pUxInfo = set_exitcode_gux_pids(pUxInfo->ui_childpid, WEXITSTATUS(exitcode));
                if (!rxstat) rxstat = pUxInfo->ui_stat;
            }
        }
    }

    return (rxstat);
}
/***************************************************************/
int rx_check_proclist(void ** proclist, int nprocs, int rxwaitflags) {
/*
 * Called after START command.
*/

    int rxstat;
    int pix;
    struct uxInfo* pUxInfo;

    rxstat = 0;
    pix = 0;

    while (pix < nprocs) {
    	pUxInfo = (struct uxInfo*)(proclist[pix]);
    	pUxInfo->ui_flags |= RXFLAG_STARTED;
        pix++;
    }

    return (rxstat);
}
/***************************************************************/
int rx_get_pid(void) {
    
    return (int)getpid();
}
/***************************************************************/
void rx_get_pidlist(PID_T ** pidlist, int * npids) {
    /*
     ** Assumes pidlist & npids have been initialized.
     */
    
    int ii;
    int mxpid;
    
    mxpid = (*npids);
    
	for (ii = 0; ii < gux_npids; ii++) {
  		if (gux_uxInfo[ii]->ui_flags & RXFLAG_RUNNING) {
	        if ((*npids) == mxpid) {
		        if (!mxpid) mxpid = 4;
		        else mxpid *= 2;
                
		        (*pidlist)  = Realloc((*pidlist), PID_T, mxpid);
	        }
	        (*pidlist)[(*npids)] = gux_uxInfo[ii]->ui_childpid;
	        (*npids)++;
        }
	}
}
/***************************************************************/
void rx_get_pid_str(void * procptr, char * pidstr, int pidstrmax) {
    
    struct uxInfo* pUxInfo;
    char pidbuf[16];
    
    pUxInfo = (struct uxInfo*)procptr;
    sprintf(pidbuf, "%d", pUxInfo->ui_childpid);
    strncpy(pidstr, pidbuf, pidstrmax);
}
/***************************************************************/
int rx_wait_pidlist(PID_T* pidlist, int npids, int * exit_code, int rxwaitflags) {
    
    struct uxInfo* pUxInfo;
    int pix;
    int ii;
    int rxstat;
    int piderr;
    int l_npids;
    int l_mxpid;
	struct uxInfo ** l_uxInfo;

    rxstat  = 0;
    piderr  = 0;
    (*exit_code) = 0;
    l_npids = 0;
    l_mxpid = 0;
    l_uxInfo = NULL;
    
    for (ii = 0; ii < npids; ii++) {
        pix = find_gux_pids(pidlist[ii]);
        if (pix < 0) {
            piderr = 1;
        } else {
            pUxInfo = gux_uxInfo[pix];
            if (pUxInfo->ui_flags & RXFLAG_RUNNING) {
	            if (l_npids == l_mxpid) {
		            if (!l_mxpid) l_mxpid = 4;
		            else l_mxpid *= 2;
                    
		            l_uxInfo  = Realloc(l_uxInfo, struct uxInfo*, l_mxpid);
	            }
	            l_uxInfo[l_npids] = pUxInfo;
	            l_npids++;
            }
        }
    }
    
    if (l_npids) {
        rxstat = rx_wait((void**)l_uxInfo, l_npids, rxwaitflags);
        (*exit_code) = rx_exitcode((void**)l_uxInfo, l_npids);
        rx_free((void**)l_uxInfo, l_npids); /* Sets RXFLAG_DELETE */
        Free(l_uxInfo);
    }
    del_all_gux_completed_pids();
    
    if (piderr && !rxstat && !(rxwaitflags & RXWAIT_IGNERR)) rxstat = RUXERR_BADPID;
    
    return (rxstat);
}
/***************************************************************/
#endif  /* ! IS_WINDOWS */
/***************************************************************/
/***************************************************************/
char * parse_command_line(const char * cmd, int * pargc, const char * arg0, int flags) {

/*
**  flags   0 - treat backslashes like any character
**          1 - treat backslashes as an escape character
*/
    /* see http://msdn.microsoft.com/en-us/library/a1y7w461.aspx */
    int ix;
    int qix;
    int done;
    int nparms;
    int parmix;
    int tlen;
    int size;
    int tix;
    int arg0len;
    char * cargv;
    char ** argv;

    /* pass 1: count parms */
    arg0len = 0;
    nparms = 0;
    tlen = 0;
    ix = 0;
    while (cmd[ix]) {
        while (isspace(cmd[ix])) ix++;
        if (cmd[ix]) {
            qix = -1;
            done = 0;
            while (!done) {
                if (!cmd[ix]) done = 1;
                else if (cmd[ix] == '\"') {
                    if (qix < 0) qix = ix;
                    else qix = -1;
                    ix++;
                } else if (isspace(cmd[ix]) && (qix < 0)) {
                    done = 1;
                } else {
                    if (cmd[ix] == '\\' && cmd[ix+1] && (flags & 1)) ix++;
                    tlen++;
                    ix++;
                }
            }
            tlen++; /* for '\0' terminator */
            nparms++;
        }
    }

    if (arg0) {
        nparms++;
        arg0len = Istrlen(arg0) + 1;
        tlen += arg0len;
    }

    tix = sizeof(char*) * (nparms + 1);
    size = tix + tlen;
    cargv = New(char, size);
    argv = (char**)cargv;
    parmix = 0;

    if (arg0) {
        argv[parmix] = cargv + tix;
        memcpy(cargv + tix, arg0, arg0len);
        tix += arg0len;
        parmix++;
    }

    /* pass 2: move parms */
    ix = 0;
    while (cmd[ix]) {
        while (isspace(cmd[ix])) ix++;
        if (cmd[ix]) {
            qix = -1;
            argv[parmix] = NULL;
            done = 0;
            while (!done) {
                if (!cmd[ix]) done = 1;
                else if (cmd[ix] == '\"') {
                    if (qix < 0) qix = ix;
                    else qix = -1;
                    ix++;
                } else if (isspace(cmd[ix]) && (qix < 0)) {
                    done = 1;
                } else {
                    if (cmd[ix] == '\\' && cmd[ix+1] && (flags & 1)) ix++;
                    if (!argv[parmix]) argv[parmix] = cargv + tix;
                    cargv[tix++] = cmd[ix];
                    ix++;
                }
            }
            if (!argv[parmix]) argv[parmix] = cargv + tix;
            cargv[tix++] = '\0';
            parmix++;
        }
    }
    argv[nparms] = NULL;

    (*pargc) = nparms;

    return (cargv);
}
/***************************************************************/
