/***************************************************************/
/* desh.h                                                      */
/***************************************************************/

#define WHMEM 1

#if WHMEM
    #include "cmemh.h"
#else
    #define New(t,n)         ((t*)calloc(sizeof(t),(n)))
    #define Realloc(p,t,n)   ((t*)realloc((p),sizeof(t)*(n)))
    #define Free(p)           free(p)
#endif
#define Strdup(s)        (strcpy(New(char,strlen(s)+1),(s)))

/***************************************************************/
/* Empty structs to reduce warnings IN XCode 5                 */
struct cenv;
struct parmlist;
struct cmdinfo;
struct var;
/***************************************************************/

extern int global_user_break; /* desh.c -- ctl-c pressed */
extern int g_show_signals;

#define PROGRTN_SUCC    0
#define PROGRTN_FAIL    1
#define PROGRTN_ERR     (-1)

#define DESHPROMPT1_NAME    "DESHPROMPT1"
#define DESHPROMPT2_NAME    "DESHPROMPT2"

#if IS_WINDOWS
    #define CHDIR       _chdir
    #define CHMOD       _chmod
    #define DUP         _dup
    #define CLOSE       _close
    #define OPEN        _open
    #define OPEN3       _open
    #define PIPE(f)     _pipe(f, 1, _O_NOINHERIT)
    #define REG_FOPEN(f,m)
    #define REG_FCLOSE(f)
    #define GETCWD      _getcwd
    #define GETENV      getenv
    #define ISATTY      _isatty
    #define LSEEK       _lseek
    #define MKDIR       _mkdir
    #define READ        _read
    #define REMOVE      remove
    #define RENAME      rename
    #define RMDIR       _rmdir
    #define LSTAT        stat
    #define STAT         stat
	#define USE_TMPFILE 0
    #define TEMPNAM     _tempnam
    #define WRITE       _write
    #define UTIME       _utime
    #define FILENO      _fileno
    #define TZSET      _tzset
    #define FCHOWN      win_fchown
#else
    #define CHDIR       chdir
    #define CHMOD       chmod
    #define GETCWD      getcwd
    #define GETENV      getenv
    #define ISATTY      isatty
    #define LSEEK       lseek
    #define MKDIR(d)    mkdir((d),0755)
    #define READ        read
    #define REMOVE      remove
    #define RENAME      rename
    #define RMDIR       rmdir
    #define LSTAT       lstat
    #define STAT        stat
	#define USE_TMPFILE 1
    #define TEMPNAM     tempnam
    #define WRITE       write
    #define UTIME       utime
    #define FILENO      fileno
    #define TZSET       tzset
    #define FCHOWN      fchown

    #define DUP         dup
    #define CLOSE       close
    #define OPEN        open
    #define OPEN3       open
    #define PIPE(f)     pipe(f)
    #define REG_FOPEN(f,m)
    #define REG_FCLOSE(f)
#endif

#define RESET(f) LSEEK((f),0,SEEK_SET)

#ifndef _O_RDONLY
#define _O_RDONLY O_RDONLY
#endif

#ifndef _O_WRONLY
#define _O_WRONLY O_WRONLY
#endif

#ifndef _O_CREAT
#define _O_CREAT O_CREAT
#endif

#ifndef _O_TRUNC
#define _O_TRUNC O_TRUNC
#endif

#ifndef _O_APPEND
#define _O_APPEND O_APPEND
#endif

#ifndef _O_BINARY
#define _O_BINARY (0)
#endif
/***************************************************************/

#define ERRMSG_MAX          255
#define ASSOC_MAX           PATH_MAXIMUM

#if IS_DEBUG
    #define BASIC_CHUNK         4
#else
	#define BASIC_CHUNK         64
#endif

#define CHARLIST_CHUNK      BASIC_CHUNK     /* Used by sub_parm_fileset() */
#define PARMLIST_CHUNK      4               /* Number of function or cmd parms */
#define COMMAND_CHUNK       BASIC_CHUNK     /* Used by save_cmd() for for,function,while */
#define CMD_LINEMAX         BASIC_CHUNK     /* File buffer size */
#define STR_CHUNK_SIZE      BASIC_CHUNK     /* Used by append_str_ch(), others */
#define VAR_CHUNK_SIZE      STR_CHUNK_SIZE
#define INIT_STR_CHUNK_SIZE BASIC_CHUNK     /* Used by init_str() */

#define COMMENT_CHAR        '#'
#define CONTINUATION_CHAR   '^'

#define TMPFIL_LINEMAX      256

typedef int FILET;
typedef int64 VITYP; /* true long */
#define NUMBUF_SIZE         24 /* needs to hold largest VITYP */
#define BI_FUNCNAM_SIZE     12 /* needs to hold largest name of built-in function - "subfromdate" */

#define ONE_MEGABYTE 1048576L    /* Max length of rept() */

#define FREECE_STDIN        1
#define FREECE_STDOUT       2
#define FREECE_STDERR       4
#define FREECE_ARGS         8
#define FREECE_STMTS        16
#define FREECE_VARS         32
#define FREECE_FUNCS        64
#define FREECE_NEST         128

/*
2^57 = 144,115,188,075,855,872
2^58 = 288,230,376,151,711,744
2^59 = 576,460,752,303,423,488
2^60 = 1,152,921,504,606,846,976
2^61 = 2,305,843,009,213,693,952
2^62 = 4,611,686,018,427,387,904
2^63 = 9,223,372,036,854,775,808    (19 digits)
2^64 = 18,446,744,073,709,551,616
*/
#define IS_FIRST_VARCHAR(c) ((isalpha(c) || (c) == '_')?1:0)
#define IS_VARCHAR(c)       ((isalnum(c) || (c) == '_')?1:0)

#define ESTAT_OK            0

#define CEFLAG_VERSNO       1       /* -a flag */
#define CEFLAG_CMD          2       /* -c flag */
#define CEFLAG_DEBUG        4       /* -d flag */
#define CEFLAG_SUBFSET      8       /* -f flag - fileset substitution, e.g. f* */
#define CEFLAG_CASE         16      /* -i flag */
#define CEFLAG_NORC         32      /* -n flag - do not execute rc file */
#define CEFLAG_PAUSE        64      /* -p flag */
#define CEFLAG_OPT_PARMS    128     /* -r flag - Undocumented */
#define CEFLAG_VERBOSE      256     /* -v flag */
#define CEFLAG_WIN          512     /* -w flag - file extensions and % substitution */
#define CEFLAG_EXIT         1024    /* -x flag */

#define CEFLAG_ALL_FLAGS    0x0FFF

#define DESHRC_NAME         "deshrc"
#define DESHRCINI_NAME      "deshrc.ini"
#define DESHRCDOT_NAME      ".deshrc"

/* command flags */
#define CMDFLAG_VERBOSE     1   /* -v flag */
#define CMDFLAG_BARE        2
#define CMDFLAG_RECURSIVE   4
#define CMDFLAG_UX_STYLE    8
#define CMDFLAG_SHOW_ALL    16
#define CMDFLAG_GROUP_DIRS  32
#define CMDFLAG_SORT_TIME   64
#define CMDFLAG_SORT_SIZE   128
#define CMDFLAG_DESC_SORT   256
#define CMDFLAG_YES         512
#define CMDFLAG_DIR_REQ     1024
#define CMDFLAG_FORCE       2048
#define CMDFLAG_READ_ONLY   4096
#define CMDFLAG_NUMBERED    8192
#define CMDFLAG_DEBUG       16384

#define CPFLAG_CHDATE       1
#define CPFLAG_CHOWN        2


#define IS_VERBOSE(c,f)     (((c)->ce_flags & CEFLAG_VERBOSE) || ((f) & CMDFLAG_VERBOSE))
#define IS_SEARCH_DOT(c)    (((c)->ce_flags & CEFLAG_WIN)?SRCHFLAG_SEACH_DOT:0)

#if IS_WINDOWS
    #define DEFAULT_FLAGS           (CEFLAG_CASE | CEFLAG_WIN)
    #define DEFAULT_PATH_DELIM      ';'
    #define UPSHIFT_GLOBAL_VARNAMES 1
#else
    #define DEFAULT_FLAGS           (CEFLAG_SUBFSET)
    #define DEFAULT_PATH_DELIM      ':'
    #define UPSHIFT_GLOBAL_VARNAMES 0
#endif

struct str {
    int         str_len;
    int         str_max;
    char *      str_buf;
};

#define PROMPT_NONE     0 /* no prompt */
#define PROMPT_CMD      1 /* standard command prompt - DESHPROMPT1 */
#define PROMPT_CMD_CONT 2 /* continuation command prompt - DESHPROMPT2 */
#define PROMPT_GT       3 /* '>' prompt */
#define PROMPT_STR      4 /* Use prompt_string */
#define PROMPT_DASH     5 /* '-' prompt */

#define PROMPT_MAX      128 /* maximum prompt length */


#define APPCH(s,c) {if((s)->str_len==(s)->str_max){\
(s)->str_max+=STR_CHUNK_SIZE;(s)->str_buf=Realloc((s)->str_buf,char,(s)->str_max);}\
(s)->str_buf[(s)->str_len]=(c);if(c)(s)->str_len+=1;}

typedef int (*cmd_handler)(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix);

struct cmdinfo {
    cmd_handler     ci_cmd_handler;
    char *          ci_cmdpath;
};

struct filref {
    char *          fr_cmdpath;
    int             fr_parm_subbed;
};

struct extinf {
    char *          xi_cmdpath;
};

typedef int (*bifunc_handler)(struct cenv * ce,
            int parmc,
            struct var ** parmv,
            struct var * result);

struct bifuncinfo {
    unsigned int        bfi_parm_mask;
    bifunc_handler      bfi_bifunc_handler;
    char                bfi_funcnam[BI_FUNCNAM_SIZE];
};

/*
typedef int (*eisetenv_handler)(struct cenv * ce,
            const char * varnam,
            const char * varval);

struct envinfo {
    eisetenv_handler      ei_setenv_handler;
};
*/

#define ER_ERRMSG_MAX           (ERRMSG_MAX + 1)
#define ER_STATIC_ERRMSG_MAX    128

struct errrec {
    int                 er_in_use;
    int                 er_next_avail;
    int                 er_next_errrec_index;
    int                 er_errnum;
    int                 er_index;
    char                er_errmsg[ER_ERRMSG_MAX];
};

struct glob {
    char **             gg_envp;
    char                gg_path_delim;
    struct dbtree *     gg_bi_funcs;    /* tree of struct bifuncinfo - built-in functions */
    struct dbtree *     gg_cmds;        /* tree of struct cmdinfo */
    struct dbtree *     gg_idle_cmds;   /* tree of struct cmdinfo */
    struct dbtree *     gg_genv;        /* tree of int - indexes into gg_envp */
    int                 gg_nenv;
    struct dbtree *     gg_extinf;      /* tree of struct extinf */
    struct dbtree *     gg_filref;      /* tree of struct filref */
#if IS_DEBUG
    char *              gg_mem_fname;
#endif
    time_t              gg_time_now;
    int                 gg_is_dst;
    int                 gg_timezone;
    int                 gg_next_avail_errrec;
    int                 gg_max_errrec;
    struct errrec **    gg_errrec;
};

#define NETYP_NONE          0
#define NETYP_FOR_IDLE      1
#define NETYP_FOR           2
#define NETYP_IDLE_FUNC     3
#define NETYP_EXEC_FUNC     4
#define NETYP_IF            5
#define NETYP_ELSE          6
#define NETYP_WHILE         7
#define NETYP_TRY           8
#define NETYP_CATCH         9
#define NETYP_FILE          10
#define NETYP_CMD           11
#define NETYP_CMD_LIND_CMD  12

#define NEFORTYP_NONE       0
#define NEFORTYP_CSVPARSE   1
#define NEFORTYP_IN         2
#define NEFORTYP_TO         3
#define NEFORTYP_DOWNTO     4
#define NEFORTYP_PARSE      5
#define NEFORTYP_READ       6
#define NEFORTYP_READDIR    7

#define NEIDLE_NONE         0
#define NEIDLE_SKIP         1
#define NEIDLE_ERROR        2

#define NEFLAG_NONE         0
#define NEFLAG_NOERRMSG     1
#define NEFLAG_IN_FUNC      2
#define NEFLAG_BREAK        4

struct nest {
    int             ne_typ;
    short           ne_for_typ;
    short           ne_for_done;
    char *          ne_vnam;
    int             ne_vnlen;
    int             ne_vnmax;
    int             ne_loopback;
    int             ne_is_saved;
    int             ne_ix;
    int             ne_idle;
    int             ne_flags;
    int             ne_label;
    int             ne_saved_stmt_pc;
    int             ne_saved_stmt_len;
    struct usrfuncinfo * ne_ufi;
    void *          ne_data;
    VITYP           ne_from_val;
    VITYP           ne_to_val;
    struct nest *   ne_prev;
};

#define UFIFLAG_OPT_PARMS   1

struct usrfuncinfo {
    int             ufi_nparms;
    struct str **   ufi_sparmlist;
    char          * ufi_func_name;
    int             ufi_flags;
    int             ufi_stmt_len;
    char **         ufi_stmt_list;
};

struct funclib {
    struct dbtree * fl_funcs;                   /* tree of struct usrfuncinfo */
};

struct term;

struct filinf {
    FILET           fi_fileno;
    char *          fi_inbuf;
    char *          fi_infname;
    int             fi_inbufix;
    int             fi_inbuflen;
    int             fi_inbufsz;
    int             fi_stdin_tty;
    int             fi_flags;
    int             fi_eof;
    char *          fi_outfname;
    struct term *   fi_tty;
};

#define FIFLAG_NOCLOSE      1

#define FPR_CSVPARM_SIZE 128

struct forparserec {
    char *          fpr_csvparms;
    struct str      fpr_srcstr;
    int             fpr_ix;
    struct str      fpr_parmstr;
};

struct forreadrec {
    struct filinf * frr_fi;
    struct str      frr_str;
};

struct forreaddirrec {
    void          * frdr_rdr;
    struct str      frdr_str;
};

#define ISIDLE(c) ((c)->ce_nest->ne_idle)

struct cenv {
    struct nest *   ce_nest;
    FILET           ce_stdin_f;
    FILET           ce_stdout_f;
    FILET           ce_stderr_f;
    struct filinf * ce_filinf;
    int             ce_close_flags;
    int             ce_argc;
    char **         ce_argv;
    struct dbtree *     ce_vars;                /* tree of struct var */
    struct funclib *    ce_funclib;
    int             ce_flags;
    int             ce_rtn;
    struct var *    ce_rtn_var;
    int             ce_reading_saved;
    struct str      ce_cmd_input_str;
    int             ce_stmt_max;
    int             ce_stmt_len;
    int             ce_stmt_pc;
    char **         ce_stmt_list;
    int             ce_errnum;
    char *          ce_errmsg;
    char *          ce_errcmd;
    struct glob *   ce_gg;
    struct cenv *   ce_file_ce;
    struct cenv *   ce_prev_ce;
};

#define  CESTRCMP(c,s,t) (((c)->ce_flags & CEFLAG_CASE) ? Stricmp((s),(t)) : strcmp((s),(t)))
#define  CEMEMCMP(c,s,t,l) (((c)->ce_flags & CEFLAG_CASE) ? Memicmp((s),(t),(l)) : memcmp((s),(t),(l)))
#define  CEDWNS(c,s) (((c)->ce_flags & CEFLAG_CASE) ? tolower(s) : (s))
#define  CECHEQ(c,s,t) ((c) ? (toupper(s) == toupper(t)) : ((s) == (t)))

/***************************************************************/
int Stricmp(const char * b1, const char * b2);
int Memicmp(const char * b1, const char * b2, size_t blen);

char * get_global_argv0(void);

void free_charlist(char ** charlist);
void append_charlist(char *** p_charlist, char * buf, int * cllen, int * clmax);
void free_filref(void * vfr);

struct errrec * get_avail_errrec(struct glob * gg);
void release_errrec(struct glob * gg, int errrec_index);
struct errrec * get_errrec(struct glob * gg, int errrec_index);

int is_good_vname(struct cenv * ce, const char * vnam);

int handle_ctl_c(struct cenv * ce);

int get_var_int(struct cenv * ce,
                const char * vnam,
                int * vnum);

void copy_vnam(struct cenv * ce,
        char ** pvnamtgt,
        int * pvnamlen,
        int * pvnammax,
        struct str * srcstr);

struct cenv * new_cenv( FILET       stdin_f,
                        FILET       stdout_t,
                        FILET       stderr_f,
                        struct filinf * fi,
                        int         close_flags,
                        int argc,
                        char *argv[],
                        int flags,
                        struct funclib * fl,
                        struct glob * gg,
                        struct cenv * file_ce,
                        struct cenv * prev_ce);
struct nest * new_nest(struct cenv * ce, int netyp);
struct funclib * new_funclib(void);
void set_cmd_pc(struct cenv * ce, int cpc);
void free_nest(struct cenv * ce, struct nest * ne);
void free_cenv(struct cenv * curr_ce, struct cenv * ce);
int read_line(struct cenv * ce,
                struct filinf * fi,
                struct str * linstr,
                int ptyp,
                const char * prompt_string);
int read_and_execute_commands_from_file(struct cenv * ce,
                        const char * cmd_file_name);
struct filinf * new_filinf(FILET fi_fileno, const char * infname, int inbufsz);
int read_filinf(struct cenv * ce,
                struct filinf * fi,
                int ptyp,
                const char * prompt_string);
void free_filinf(struct filinf * fi);
int filinf_pair(struct cenv * ce,
                struct filinf * fi,
                FILET fi_outfileno,
                const char * outfname);
int read_tty_prompt(struct cenv * ce, struct filinf * fi, struct str * promptstr);

void free_forparserec(struct forparserec * fpr);
void free_forreadrec(struct forreadrec * frr);
void free_forreaddirrec(struct forreaddirrec * frdr);
struct forparserec * new_forparserec(const char * srcstr, const char * csvparms);
struct forreadrec * new_forreadrec(FILET frr_fileno, const char * infname);
struct forreaddirrec * new_forreaddirrec(void * vrdr, const char * infname);

int saving_cmds(struct cenv * ce);
int is_saved_cmd(struct cenv * ce);
void save_cmd(struct cenv * ce,
                    struct str * cmdstr,
                    int * cmdix);
char * get_next_saved_cmd(struct cenv * ce);
void delete_cmds(struct cenv * ce);

int execute_saved_commands(struct cenv * ce);
int execute_saved_command(struct cenv * ce, struct str * cmdstr);
int execute_command_list(struct cenv * ce, 
                        struct str * cmdstr,
                        int * cmdix,
                        int xeqflags);

void init_str(struct str * ss, int smax);
void init_str_char(struct str * ss, const char * buf);
void free_str_buf(struct str * ss);
void set_str_len(struct str * ss, const char * buf, int blen);
void set_str_char(struct str * ss, const char * buf);
void set_str_char_f(struct str * ss, char *fmt, ...);

void append_str_len(struct str * ss, const char * buf, int blen);
void append_str_len_upper(struct str * ss, const char * buf, int blen);
void append_str(struct str * ss, const char * buf);
void append_str_ch(struct str * ss, char ch);
void append_str_char(struct str * ss, const char * buf);
void append_str_len_at(struct str * ss, int ssix, const char * buf, int blen);
void append_str_str(struct str * tgt, struct str * src);
void get_desh_version(int vfmt, char * vers, int versmax);

void set_filinf_flags(struct filinf * fi, int fiflags);

/***************************************************************/
