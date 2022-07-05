/***************************************************************/
/* assoc.h                                                     */
/***************************************************************/


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

#define CHECK_OPENWITH 1

/***************************************************************/
/* stuff for stat()                                            */

#define FTYP_NONE     0
#define FTYP_FILE     1
#define FTYP_DIR      2
#define FTYP_LINK     3
#define FTYP_PIPE     4
#define FTYP_OTHR     15
#define FTYP_DOT_DIR  18 /* either "." or ".." */
/********************************/

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

/********************************/
/***************************************************************/
void to_widestr(char * dest, char * src);
int resolve_assoc( const char * pszSrc,
		char * pszDst,
        int pszDstSize,
		char * errmsg,
        int errmsgmax);
/*@*/int file_stat(const char * fname);
/***************************************************************/

struct tempfile {
    FILE * tf_fref;
    char * tf_fname;
};
/***************************************************************/
struct tempfile * tmpfile_open(void);
void tmpfile_close(struct tempfile * tf);
int tmpfile_filno(struct tempfile * tf);
const char * tmpfile_filnam(struct tempfile * tf);
int tmpfile_reset(struct tempfile * tf);
int tmpfile_write(struct tempfile * tf, const char * buf, int blen);

int check_file_access(const char * fname, int facc);

void get_modbuf(int fs_mode, char * modbuf);

struct file_stat_rec {
                    int     fsr_mode;
                    int     fsr_uid;
                    int     fsr_gid;
                    d_date  fsr_accdate;
                    d_date  fsr_crdate;
                    d_date  fsr_moddate;
                    int64   fsr_size;
};

int get_file_stat(const char * fname,
                    struct file_stat_rec * fsr,
                    char   * errmsg,
                    int      errmsgmax);
int get_file_owner(char * fname,
                    int fs_uid,
                    char * ownnam,
                    int ownsiz,
                    char * grpnam,
                    int grpsiz,
                    char * errmsg,
                    int errmsgmax);

int get_home_directory(struct cenv * ce, char * dirname, int dnmax);
int get_host_name(struct cenv * ce, char * hostname, int hnmax);
void get_timezone_info(int * tzone, int * is_dst);
/***************************************************************/
