/***************************************************************/
/* cmd.h                                                       */
/***************************************************************/

#define IS_DEBUG_PARM0		0

struct parmlist {
    int     pl_nparms;
    int     pl_mxparms;
    int     pl_redir_flags;
    int     pl_parm1_subbed;
    int     pl_xeqflags;
    char ** pl_cparmlist;
    int *   pl_cparmlens;
    int *   pl_cparmmaxs;
    char *  pl_stdin_fnam;
    char *  pl_stdout_fnam;
    char *  pl_stderr_fnam;
    struct cenv * pl_ce;
    struct tempfile * pl_stdin_tf;
    /* these are to close unused Unix pipes between the fork() and exec() */
    int     pl_piped_flags;
    int     pl_unused_in;
    int     pl_unused_out;
    void *  pl_vrs;
#if IS_DEBUG_PARM0
    char * pl_dbg_parm0;
#endif

    struct parmlist *   pl_piped_parmlist;
};
#define PIPEFLAG_UNUSED_IN	1
#define PIPEFLAG_UNUSED_OUT 2

#define REDIR_NONE          0
#define REDIR_ERROR         0x0001
#define REDIR_PIPE          0x0002

#define REDIR_STDIN_FILE        0x0010
#define REDIR_STDIN_HERE        0x0020
#define REDIR_STDIN_HERE_NOSUB  0x0060
#define REDIR_STDIN_MASK        0x00F0

#define REDIR_STDOUT_FILE       0x0100
#define REDIR_STDOUT_APPEND     0x0200
#define REDIR_STDOUT_MASK       0x0F00

#define REDIR_STDERR_FILE       0x1000
#define REDIR_STDERR_APPEND     0x2000
#define REDIR_STDERR_MASK       0xF000

#define SRCHFLAG_SEACH_XEQ      1
#define SRCHFLAG_SEACH_READ     4
#define SRCHFLAG_SEACH_DOT      8

#define SRCHFLAG_TO_FACC(s)     ((s) & 7)

#if IS_WINDOWS
    #define SRCHFLAG_SEACH_EXEC SRCHFLAG_SEACH_READ
#else
    #define SRCHFLAG_SEACH_EXEC SRCHFLAG_SEACH_XEQ
#endif


#define PFLAG_SLASH_DELIMITS    1
#define PFLAG_NO_SUBS           2
#define PFLAG_EXP               4
#define PFLAG_QUOTE             8
#define PFLAG_FOR_PARSE         16

#define MAX_INLINE_SUBS         100

#define SUBPARM_NONE            0
#define SUBPARM_QUOTE           1
#define SUBPARM_QUOTE_IF_NEEDED 2

/***************************************************************/
void copy_char_str(struct str * tgtstr, const char * srcchar);
void copy_char_str_len(struct str * tgtstr, const char * srcchar, int slen);

int sub_str(struct cenv * ce, struct str * linstr);

int find_exe(struct cenv * ce,
                const char * cmdchar,
                int search_flags,
                struct str * cmdstr);

int get_opt_flag(char oparm);

int get_parm(struct cenv * ce,
            struct str * cmdstr,
            int * cmdix,
            int parmflags,
            struct str * parmstr);

int parse_parmlist(struct cenv * ce,
                    struct str * cmdstr,
                int * cmdix,
                    struct parmlist * pl);
int free_parmlist_buf(struct cenv * ce, struct parmlist * pl);

int handle_redirs(struct cenv * ce,
                    struct parmlist * pl);

void set_globalvar_ix(struct glob * gg,
                  int ix,
                  const char * vnam,
                  int vnlen,
                  const char * vval,
                  int vvlen);
char * find_globalvar(struct glob * gg,
                  const char * vnam,
                  int vnlen);
char * find_globalvar_ex(struct cenv * ce,
                  const char * vnam,
                  int vnlen);
struct var * find_var(struct cenv * ce,
                  const char * vnam,
                  int vnlen);

int call_usrfunc(struct cenv * ce,
                 struct usrfuncinfo * ufi,
                 struct parmlist * pl,
                 struct var * vparmlist,
                 int nparms,
                 struct var * result);
int idle_command_handler(struct cenv * ce,
                struct str * cmdstr,
                int * cmdix);

int chndlr_generic(struct cenv * ce,
                struct str * rawparm0,
                struct str * parm0,
                struct str * cmdstr,
                int * cmdix,
                int xeqflags);
int sub_parm(struct cenv * ce,
            const char * cmdbuf,
            int * pix,
            struct str * parmstr,
            int subflags);
int sub_parm_inline(struct cenv * ce,
                struct str * cmdstr,
                int * pix,
                int subflags);

int get_parmstr(struct cenv * ce,
                struct str * cmdstr,
                int * cmdix,
                int parmflags,
                struct str * parmstr);
void clean_nest_mess(struct cenv * ce, int end_typ, int alt_end_typ);


#define XEQFLAG_NOWAIT      1
#define XEQFLAG_VERBOSE     2

int execute_command_handler(struct cenv * ce, 
                        struct str * cmdstr,
                        int * cmdix,
                        int xeqflags);
int execute_command_string(struct cenv * ce,
                        const char * cmdbuf,
                        struct str * cmdout);

void glob_init(struct glob * gg, int gflags);
/***************************************************************/
