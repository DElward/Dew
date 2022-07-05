/***************************************************************/
/* dir.h                                                       */
/***************************************************************/

#if IS_WINDOWS
    #define DIR_DELIM       '\\'
    #define DIR_DELIM_STR   "\\"
    #define IS_DIR_DELIM(c) ((c) == '\\')
    #define PATH_MAXIMUM    (_MAX_PATH + 4)
    #define FILE_MAXIMUM    (256 + 4)
    #define EXT_MAXIMUM     _MAX_EXT
#else
	#ifndef PATH_MAX
		#define PATH_MAX	1024	/* Guess ?? */
	#endif

    #define DIR_DELIM       '/'
    #define DIR_DELIM_STR   "/"
    #define IS_DIR_DELIM(c) ((c) == '/')
    #define PATH_MAXIMUM    (PATH_MAX + 4)
    #define FILE_MAXIMUM    (256 + 4)
    #define EXT_MAXIMUM     FILE_MAXIMUM
#endif
/***************************************************************/
/* stuff for stat()                                            */

#define FTYP_NONE     0
#define FTYP_FILE     1
#define FTYP_DIR      2
#define FTYP_LINK     3
#define FTYP_PIPE     4
#define FTYP_OTHR     15
#define FTYP_DOT_DIR  18 /* either "." or ".." */
/***************************************************************/

/* F_S_ISDIR */
#ifdef S_ISDIR
    #define F_S_ISDIR S_ISDIR
#else
    #define F_S_ISDIR(m) (((m) & FSTAT_S_IFMT) == FSTAT_S_IFDIR)
#endif

/* F_S_ISREG */
#ifdef S_ISREG
    #define F_S_ISREG S_ISREG
#else
    #define F_S_ISREG(m) (((m) & FSTAT_S_IFMT) == FSTAT_S_IFREG)
#endif

/* F_S_ISLNK */
#ifdef S_ISLNK
    #define F_S_ISLNK S_ISLNK
#else
    #define F_S_ISLNK(m) (((m) & FSTAT_S_IFMT) == FSTAT_S_IFLNK)
#endif
/***************************************************************/
/* Status and Permissions                                      */


/* S_IxUSR */
#ifndef S_IRUSR
    #define S_IRUSR     _S_IREAD
#endif
#ifndef S_IWUSR
    #define S_IWUSR     _S_IWRITE
#endif
#ifndef S_IXUSR
    #define S_IXUSR     _S_IEXEC
#endif

/* S_IxGRP */
#ifndef S_IRGRP
    #define S_IRGRP     0000040
#endif
#ifndef S_IWGRP
    #define S_IWGRP     0000020
#endif
#ifndef S_IXGRP
    #define S_IXGRP     0000010
#endif

/* S_IxOTH */
#ifndef S_IROTH
    #define S_IROTH     0000004
#endif
#ifndef S_IWOTH
    #define S_IWOTH     0000002
#endif
#ifndef S_IXOTH
    #define S_IXOTH     0000001
#endif

/****************/

/* S_IREAD */
#ifdef _S_IREAD
    #define FSTAT_S_IREAD _S_IREAD
#else
    #define FSTAT_S_IREAD (S_IRUSR | S_IRGRP | S_IROTH)
#endif
#define FSTAT_S_IREAD_ALL (S_IRUSR | S_IRGRP | S_IROTH)

/* S_IWRITE */
#ifdef _S_IWRITE
    #define FSTAT_S_IWRITE _S_IWRITE
#else
    #define FSTAT_S_IWRITE (S_IWUSR | S_IWGRP | S_IWOTH)
#endif
#define FSTAT_S_IWRITE_ALL (S_IWUSR | S_IWGRP | S_IWOTH)

/* S_IEXEC */
#ifdef _S_IEXEC
    #define FSTAT_S_IEXEC _S_IEXEC
#else
    #define FSTAT_S_IEXEC (S_IXUSR | S_IXGRP | S_IXOTH)
#endif
#define FSTAT_S_IEXEC_ALL (S_IXUSR | S_IXGRP | S_IXOTH)

/* S_IFREG */
#ifndef _S_IFREG
    #define _S_IFREG S_IFREG
#endif
#define FSTAT_S_IFREG _S_IFREG

/* S_IFDIR */
#ifndef _S_IFDIR
    #define _S_IFDIR S_IFDIR
#endif
#define FSTAT_S_IFDIR _S_IFDIR

/* S_IFLNK */
#ifdef S_IFLNK
    #define FSTAT_S_IFLNK S_IFLNK
#else
    #define FSTAT_S_IFLNK 0
#endif

/* S_IFLNK */
#ifdef S_IFLNK
    #define FSTAT_S_IFLNK S_IFLNK
#else
    #define FSTAT_S_IFLNK 0
#endif

/* S_IFIFO */
#ifndef _S_IFIFO
    #define _S_IFIFO S_IFIFO
#endif
#define FSTAT_S_IFIFO _S_IFIFO

/* S_IFCHR */
#ifndef _S_IFCHR
    #define _S_IFCHR S_IFCHR
#endif
#define FSTAT_S_IFCHR _S_IFCHR

/* FSTAT_S_IFMT */
#ifdef S_IFMT
    #define FSTAT_S_IFMT S_IFMT
#else
    #ifdef _S_IFMT
        #define FSTAT_S_IFMT _S_IFMT
    #else
        #define FSTAT_S_IFMT (FSTAT_S_IFREG | FSTAT_S_IFDIR | FSTAT_S_IFIFO | FSTAT_S_IFCHR)
    #endif
#endif

/***************************************************************/

#define DIRFLAG_WIN     1
#define DIRFLAG_CASE    2

#define  CECHEQ(c,s,t) ((c) ? (toupper(s) == toupper(t)) : ((s) == (t)))

#if IS_WINDOWS
    char * GetWinMessage(long errCode);
    time_t filetime_to_time(const struct _FILETIME * filtim);
#endif

/* int read_directory functions */
int open_directory(void * vrdr, const char * dname);
int read_directory(void * vrdr, char * buf, int bufsize, int * eof);
void close_directory(void * vrdr);
void * new_read_directory(void);
void close_read_directory(void * vrdr);
int get_errstat_directory(void * vrdr);
char * get_errmsg_directory(void * vrdr);

int remove_directory(struct cenv * ce,
                int cmdflags,
                const char * dirname);

void os_makepath(const char * dirname,
		const char * fname,
		char * fullname,
		int fnamemax,
        char ddelim);
int has_drive_name(const char * dirname);

/*  ********  */


void get_cwd(char *cwd, int cwdmax);
void set_time_globals(struct glob * gg);
int patmatches(int casein,
                const char * pat,
                const char * val);

int get_filenames(const char * dfname, char *** fnamelist);

int chndlr_cd(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix);
int chndlr_chmod(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix);
int chndlr_rmdir(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix);

int chndlr_mkdir(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix);
int chndlr_dir(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix);
int chndlr_copy(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix);
int chndlr_move(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix);
int chndlr_remove(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix);
int chndlr_removedir(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix);
int chndlr_type(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix);
/***************************************************************/
