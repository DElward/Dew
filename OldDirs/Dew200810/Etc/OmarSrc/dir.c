/***************************************************************/
/* dir.c                                                       */
/***************************************************************/


#include "config.h"

#if IS_WINDOWS
#include <io.h>
#include <direct.h>
#include <sys/utime.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>

#include "omar.h"
#include "dir.h"
#include "util.h"

int global_user_break = 0; /* unused */

/***************************************************************/
/* Things needed for directory reading                         */
/***************************************************************/

#if IS_WINDOWS
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#else
    #include <dirent.h>
    #include <unistd.h>
    #include <utime.h>
#endif

#define MKINT64(hi,lo) ((int64)(hi) << 32 | (int64)(lo))

#if defined(MACOSX)
#define STAT_CTIME(sb)  (sb.st_ctimespec.tv_sec)
#define STAT_MTIME(sb)  (sb.st_mtimespec.tv_sec)
#else
#define STAT_CTIME(sb)  (sb.st_ctime)
#define STAT_MTIME(sb)  (sb.st_mtime)
#endif

#define STAT_FSIZE_LO(sb) ((sb.st_size) & 0xFFFFFFFFL)
#define STAT_FSIZE_HI(sb) (((int64)(sb.st_size) >> 32) & 0xFFFFFFFFL)


/*
**
**  removed `aa/go'
**  removed `aa/bb'
**  removed directory: `aa'
**  
**  mkdir: created directory `aa'
**  
*/
#define DIRERR_TOO_DEEP     (-3001)
#define DIRERR_OPEN_DIR     (-3002)
#define DIRERR_READ_DIR     (-3003)
#define DIRERR_FILE_ATTR    (-3004)
#define DIRERR_CALLBACK_ERR (-3005)
#define DIRERR_FIND_FILE    (-3006)
#define DIRERR_NOT_DIR      (-3007)
#define DIRERR_CTL_C        (-3008)

#define DIROPT_RECURSIVE    1
#define DIROPT_POST_ORDER   2
#define DIROPT_DOT_DOTDOT   4
#define DIROPT_CASE         8
#define DIROPT_ONE_LEVEL    16
#define DIROPT_ALL_DIRS     32
#define DIROPT_DOT_HIDES    64

#define SWFLAG_DOT_STAR     1


#define MAX_DIR_RECUR_DEPTH 100

struct dirInfo {
    int    di_stat;
    int    di_oserr;
    char * di_errmsg;
};

/*
*       typedef struct _WIN32_FIND_DATA {
*         DWORD    dwFileAttributes; 
*         FILETIME ftCreationTime; 
*         FILETIME ftLastAccessTime; 
*         FILETIME ftLastWriteTime; 
*         DWORD    nFileSizeHigh; 
*         DWORD    nFileSizeLow; 
*         DWORD    dwReserved0; 
*         DWORD    dwReserved1; 
*         TCHAR    cFileName[ MAX_PATH ]; 
*         TCHAR    cAlternateFileName[ 14 ]; 
*       } WIN32_FIND_DATA, *PWIN32_FIND_DATA; 
*
*       struct stat {
*           dev_t         st_dev;      // device
*           ino_t         st_ino;      // inode
*           mode_t        st_mode;     // protection
*           nlink_t       st_nlink;    // number of hard links
*           uid_t         st_uid;      // user ID of owner
*           gid_t         st_gid;      // group ID of owner
*           dev_t         st_rdev;     // device type (if inode device)
*           off_t         st_size;     // total size, in bytes
*           blksize_t     st_blksize;  // blocksize for filesystem I/O
*           blkcnt_t      st_blocks;   // number of blocks allocated
*           time_t        st_atime;    // time of last access
*           time_t        st_mtime;    // time of last modification
*           time_t        st_ctime;    // time of last status change
*              
*/

#define FATTR_IS_DIR        1
#define FATTR_READ_ONLY     2
#define FATTR_HIDDEN        4
#define FATTR_FILE          8

struct filattrrec {
    int     fa_fil_attr;
    uint32  fa_file_size_hi;
    uint32  fa_file_size_lo;
    time_t  fa_mod_date;
    time_t  fa_cr_date;
    int     fa_name_ix;
    char    fa_filepath[PATH_MAXIMUM];
};

struct fstats {
    int    fst_nfiles;
    int    fst_ndirs;
    int64  fst_total;
    char   fst_dir_name[PATH_MAXIMUM];
};

struct dir_work {
    struct fstats dw_dir_fst;
    struct fstats dw_tot_fst;
    int     dw_npath_ext;
    char ** dw_path_ext_list;
};

int binary_find(const char * srchstr, char ** srclist, int nlist);

#define IS_WILD(c) ((c) == '*' || (c) == '?' || (c) == '[')

#define COPY_FBUF_SIZE  24576
/***************************************************************/
#if IS_WINDOWS
/*@*/int winfile_stat(const char * fname) {

	int fAttr;
    int result = 0;
    int fnlen;

    if (fname[0] == '.' && (fname[1] == '\0' ||
                           (fname[1] == '.' && fname[2] == '\0'))) {
        result = FTYP_DOT_DIR;
    } else {
	    fAttr = GetFileAttributes((LPCTSTR)fname);
	    if (fAttr <  0) {
            result = FTYP_NONE;
	    } else {
            if (fAttr & FILE_ATTRIBUTE_DIRECTORY)   result = FTYP_DIR;
            else {
                fnlen = IStrlen(fname);
                if (fnlen > 4 && fname[fnlen-4] == '.'  &&
                    (fname[fnlen-3] == 'l' || fname[fnlen-3] == 'L') &&
                    (fname[fnlen-2] == 'n' || fname[fnlen-2] == 'N') &&
                    (fname[fnlen-1] == 'k' || fname[fnlen-1] == 'K')) {
                    result = FTYP_LINK;
                } else  {
                    result = FTYP_FILE;
                }
            }
        }
    }

	return (result);
}
#endif
/***************************************************************/
/*@*/int uxfile_stat(const char * fname) {
/*
** Figures out file type.
**  Sets result:
**      FTYP_NONE       (0) - No such file or cannot access
**      FTYP_FILE       (1) - Regular file
**      FTYP_DIR        (2) - Directory
**      FTYP_LINK       (3) - Special file
**      FTYP_OTHR       (4) - Other file
**      FTYP_DOT_DIR    (5) -  either "." or ".."
**
**   From Newton (Linux x64)
**   stat.h:#define S_IFMT  00170000
**   stat.h:#define S_IFREG  0100000
**   stat.h:#define S_IFDIR  0040000
**   stat.h:#define S_IFCHR  0020000
**   stat.h:#define S_IFIFO  0010000
*/

    int result = 0;
    struct stat stbuf;

    if (fname[0] == '.' && (fname[1] == '\0' ||
                           (fname[1] == '.' && fname[2] == '\0'))) {
        result = FTYP_DOT_DIR;
    } else {
        if (LSTAT(fname, &stbuf)) {
            result = FTYP_NONE;
        } else {
                 if (F_S_ISDIR(stbuf.st_mode))  result = FTYP_DIR;
            else if (F_S_ISREG(stbuf.st_mode))  result = FTYP_FILE;
            else if (F_S_ISLNK(stbuf.st_mode))  result = FTYP_LINK;
            else                                result = FTYP_OTHR;
        }
    }

    return (result);
}
/***************************************************************/
/*@*/int file_stat(const char * fname) {

#if IS_WINDOWS
    return winfile_stat(fname);
#else
    return uxfile_stat(fname);
#endif
}
/***************************************************************/
int patbracketmatches(int casein,
                const char * pat,
                char val) {

    int matches;
    int patix;

    matches = 0;
    patix   = 0;
    while (!matches && pat[patix] && pat[patix] != ']') {
        if (pat[patix + 1] == '-') {
            patix += 2;
            if (pat[patix] && pat[patix] != ']') {
                if (casein) {
                    if (toupper(val) >= toupper(pat[patix - 2]) &&
                        toupper(val) <= toupper(pat[patix])) matches = 1;
                } else {
                    if (val >= pat[patix - 2] &&
                        val <= pat[patix]) matches = 1;
                }
            }
        } else {
            matches = CECHEQ(casein, pat[patix], val);
        }
        patix++;
    }

    return (matches);
}
/***************************************************************/
int patmatches(int casein,
                const char * pat,
                const char * val) {

    int matches;
    int brend;
    int patix;
    int valix;

    if (!pat || !pat[0]) return (1);

    patix   = 0;
    valix   = 0;
    matches = 1;
    while (matches && pat[patix] && val[valix]) {
        if (pat[patix] == '?') {
            patix++;
            valix++;
        } else if (pat[patix] == '*') {
            patix++;
            if (!pat[patix]) matches = 1;
            else             matches = 0;
            while (!matches && val[valix]) {
                matches = patmatches(casein, pat + patix, val + valix);
                if (!matches) valix++;
            }
            if (matches) return (1);
        } else if (pat[patix] == '[') {
            brend = patix + 1;
            while (pat[patix] && pat[brend] != ']') brend++;
            if (pat[patix]) {
                matches = patbracketmatches(casein, pat + patix + 1, val[valix]);
                if (matches) {
                    patix = brend + 1;
                    valix++;
                }
            } else matches = 0;
        } else if (CECHEQ(casein, pat[patix], val[valix])) {
            patix++;
            valix++;
        } else matches = 0;
    }

    if (matches) {
        if (val[valix]) matches = 0;
        else if (pat[patix]) {
            if (pat[patix] == '*') {
                matches = patmatches(casein, pat + patix + 1, val); /* Fixed 2/9/2014 */
            } else matches = 0;
        }
    }

    return (matches);
}
/***************************************************************/
void ux_splitpath(
		const char * fullname,
		char * dirname,
		char * fname,
		int fnamemax)
{
	int ii;

	ii = IStrlen(fullname);

	while (ii > 0 && !IS_DIR_DELIM(fullname[ii - 1])) ii--;
	strmcpy(fname, fullname + ii, fnamemax);

	memcpy(dirname, fullname, ii);
	dirname[ii] = '\0';
}
/***************************************************************/
void ux_splitpath_d(
		const char * fullname,
		char * dirname,
		char * fname,
		int fnamemax)
{
/*
** Same as ux_splitpath(), but always puts last part in fname.
**   e.g.    /aaa/bbb/ccc/ gives dirname=/aaa/bbb and fname=ccc
*/
	int ii;
    int jj;

	ii = IStrlen(fullname);
    jj = 0;

	if (ii > 0 && IS_DIR_DELIM(fullname[ii - 1])) ii--;
	while (ii > 0 && !IS_DIR_DELIM(fullname[ii - 1])) { ii--; jj++; }
	memcpy(fname, fullname + ii, jj);
    fname[jj] = '\0';

	memcpy(dirname, fullname, ii);
	dirname[ii] = '\0';
}
/***************************************************************/
void ux_splitpathext(
		const char * fullname,
		char * dirname,
		char * fname,
		char * fext,
		int fnamemax,
        int extmax)
{
	int ii;
	int nn;

	ii = IStrlen(fullname);

	while (ii > 0 &&
			(fullname[ii - 1] != '.') &&
			(!IS_DIR_DELIM(fullname[ii - 1]))) ii--;

	if (ii > 0 && (fullname[ii - 1] == '.')) {
		strmcpy(fext, fullname + ii, extmax);
		ii--;
		nn = 0;
		while (ii > 0 && !IS_DIR_DELIM(fullname[ii - 1])) { ii--; nn++; }
		if (nn >= fnamemax - 1) nn = fnamemax - 2;
		memcpy(fname, fullname + ii, nn);
		fname[nn] = '\0';
	} else {
		fext[0] = '\0';
		strmcpy(fname, fullname + ii, fnamemax);
	}

	memcpy(dirname, fullname, ii);
	dirname[ii] = '\0';
}
/***************************************************************/
void ux_makepath(const char * dirname,
		const char * fname,
		char * fullname,
		size_t fnamemax,
        char ddelim)
{
	int dnlen;
	int fnlen;

	dnlen = IStrlen(dirname);
	fnlen = IStrlen(fname);

	if ((size_t)(dnlen + fnlen + 2) > fnamemax) {
		if (fnamemax > 0) fullname[0] = '\0';
		return;
	}

	if (dnlen) {
		memcpy(fullname, dirname, dnlen);
		if (fullname[dnlen-1] != ddelim) {
			fullname[dnlen++] = ddelim;
		}
	}
	memcpy(fullname + dnlen, fname, fnlen + 1);
}
/***************************************************************/
static void ux_makepathext(const char * dirname,
		const char * fname,
		const char * fext,
		char * fullname,
		size_t fnamemax,
        char ddelim)
{
	int dnlen;
	int fnlen;
	int exlen;
	int fulllen;

	dnlen = IStrlen(dirname);
	fnlen = IStrlen(fname);
	exlen = IStrlen(fext);
	fulllen = 0;

	if ((size_t)(dnlen + fnlen + exlen + 3) > fnamemax) {
		if (fnamemax > 0) fullname[0] = '\0';
		return;
	}

	if (dnlen) {
		memcpy(fullname, dirname, dnlen);
		fulllen += dnlen;
		if (fullname[fulllen-1] != ddelim) {
			fullname[fulllen++] = ddelim;
		}
	}
	memcpy(fullname + fulllen, fname, fnlen);
	fulllen += fnlen;

	if (exlen) {
		fullname[fulllen++] = '.';
		memcpy(fullname + fulllen, fext, exlen);
		fulllen += exlen;
	}
	fullname[fulllen] = '\0';
}
/***************************************************************/
void os_makepath(const char * dirname,
		const char * fname,
		char * fullname,
		int fnamemax,
        char ddelim)
{
	ux_makepath(dirname, fname, fullname, fnamemax, ddelim);
}
/***************************************************************/
void os_makepathext(const char * dirname,
		const char * fname,
		const char * fext,
		char * fullname,
		int fnamemax,
        char ddelim)
{
	ux_makepathext(dirname, fname, fext, fullname, fnamemax, ddelim);
}
/***************************************************************/
void os_splitpath(
		const char * fullname,
		char * dirname,
		char * fname,
		int fnamemax)
{
	ux_splitpath(fullname, dirname, fname, fnamemax);
}
/***************************************************************/
void os_splitpath_d(
		const char * fullname,
		char * dirname,
		char * fname,
		int fnamemax)
{
	ux_splitpath_d(fullname, dirname, fname, fnamemax);
}
/***************************************************************/
void os_splitpathext(
		const char * fullname,
		char * dirname,
		char * fname,
		char * fext,
		int fnamemax,
        int extmax)
{
	ux_splitpathext(fullname, dirname, fname, fext, fnamemax, extmax);
}
/***************************************************************/
static struct dirInfo* new_dirInfo(void)
{
    struct dirInfo* di;

    di = New(struct dirInfo, 1);
    di->di_stat         = 0;
    di->di_oserr        = 0;
    di->di_errmsg       = NULL;

    return (di);
}
/***************************************************************/
static void free_dirInfo(struct dirInfo* di)
{

    if (di->di_errmsg) Free(di->di_errmsg);

    Free(di);
}
/***************************************************************/
void dx_free(void * vdi) {

    struct dirInfo* di = (struct dirInfo*)vdi;

    free_dirInfo(di);
}
/***************************************************************/
int dx_stat(void * vdi) {

    struct dirInfo* di = (struct dirInfo*)vdi;

    return (di->di_stat);
}
/***************************************************************/
char * dx_errmsg(void * vdi) {

    struct dirInfo* di = (struct dirInfo*)vdi;

    return (di->di_errmsg);
}
#if IS_WINDOWS
/***************************************************************/
/* Windows directory functions                                 */
/***************************************************************/
struct dirdata {
    HANDLE hFind;
    WIN32_FIND_DATA FindFileData;
    int    adv;
    int    fil_attr;
};
/***************************************************************/
int get_windows_fil_attr(int win_fil_attr) {

    int fil_attr;

    fil_attr = 0;

    if (win_fil_attr & FILE_ATTRIBUTE_DIRECTORY ) fil_attr |= FATTR_IS_DIR;
    else                                          fil_attr |= FATTR_FILE;

    if (win_fil_attr & FILE_ATTRIBUTE_READONLY  ) fil_attr |= FATTR_READ_ONLY;
    if (win_fil_attr & FILE_ATTRIBUTE_HIDDEN    ) fil_attr |= FATTR_HIDDEN;

    return (fil_attr);
}
/***************************************************************/
char * GetWinMessage(long errCode) {

    LPVOID lpMsgBuf;
    int ii;
    char * cpMsgBuf;
    char * winmsg;

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        errCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (LPTSTR) &lpMsgBuf,
        0,
        NULL);

    cpMsgBuf = (char *)lpMsgBuf;

    ii = IStrlen(cpMsgBuf);
    while (ii > 0 && isspace(cpMsgBuf[ii-1])) ii--;
    winmsg = New(char, ii + 1);

    memcpy(winmsg, cpMsgBuf, ii);
    winmsg[ii] = '\0';

    LocalFree(lpMsgBuf);

    return (winmsg);
}
/***************************************************************/
time_t filetime_to_time(const FILETIME * filtim) {

	time_t outtim;
	SYSTEMTIME systim;
	struct tm stm;
    FILETIME locfiltim;

	memset(&stm, 0, sizeof(struct tm));

    if (!FileTimeToLocalFileTime(filtim, &locfiltim)) {
		stm.tm_year = 88;
		stm.tm_mon  = 8;
		stm.tm_mday = 8;
		stm.tm_hour = 8;
		stm.tm_min  = 8;
		stm.tm_sec  = 8;
    } else {
	    if (!FileTimeToSystemTime(&locfiltim, &systim)) {
		    stm.tm_year = 77;
		    stm.tm_mon  = 7;
		    stm.tm_mday = 7;
		    stm.tm_hour = 7;
		    stm.tm_min  = 7;
		    stm.tm_sec  = 7;
	    } else {
		    stm.tm_year = systim.wYear - 1900;
		    stm.tm_mon  = systim.wMonth - 1;
		    stm.tm_mday = systim.wDay;
		    stm.tm_hour = systim.wHour;
		    stm.tm_min  = systim.wMinute;
		    stm.tm_sec  = systim.wSecond;
	    }
    }
	outtim = mktime(&stm);

	return (outtim);
}
/***************************************************************/
void cvt_windows_fil_attr_rec(struct filattrrec * fa,
                                WIN32_FIND_DATA * pFindFileData) {

    fa->fa_fil_attr     = get_windows_fil_attr(pFindFileData->dwFileAttributes);
    fa->fa_mod_date     = filetime_to_time(&(pFindFileData->ftLastWriteTime));
    fa->fa_cr_date      = filetime_to_time(&(pFindFileData->ftCreationTime));
    fa->fa_file_size_hi = pFindFileData->nFileSizeHigh;
    fa->fa_file_size_lo = pFindFileData->nFileSizeLow;
}
/***************************************************************/
int get_fil_attr_rec(struct dirInfo* di,
                        const char * fname,
                        struct filattrrec * fa) {
    int result = 0;
    HANDLE hFind;
    int namix;
    int dir_req;
    WIN32_FIND_DATA FindFileData;

    namix = IStrlen(fname);
    if (!namix) {
        strcpy(fa->fa_filepath, "."); /* get_cwd */
        namix = 1;
    } else {
        memcpy(fa->fa_filepath, fname, namix + 1);
    }

    if (namix && fa->fa_filepath[namix - 1] == DIR_DELIM) {
#if IS_WINDOWS
        if (namix != 3 || fa->fa_filepath[namix - 2] != ':') {
            fa->fa_filepath[namix - 1] = '\0';
        } else {
            fa->fa_filepath[namix    ] = '*';
            fa->fa_filepath[namix + 1] = '\0'; /*  Need to C:\ --> C:\*  */
        }
#else
        fa->fa_filepath[namix - 1] = '\0';
#endif
        dir_req = 1;
    } else {
        while (namix > 0 && fa->fa_filepath[namix - 1] != DIR_DELIM) namix--;
        dir_req = 0;
    }

    hFind = FindFirstFile((LPCTSTR)fa->fa_filepath, &FindFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        di->di_oserr  = GetLastError();
        di->di_errmsg = GetWinMessage(di->di_oserr);
        di->di_stat   = DIRERR_FIND_FILE;
        result = di->di_stat;
    } else {
        if (dir_req) fa->fa_filepath[namix] = '\0';  /*  In case of C:\ --> C:\*  */
        fa->fa_name_ix = namix;
        cvt_windows_fil_attr_rec(fa, &FindFileData);

        FindClose(hFind);
    }

    if (!di->di_stat && dir_req && !(fa->fa_fil_attr & FATTR_IS_DIR)) {
        di->di_stat = DIRERR_NOT_DIR;
        di->di_errmsg = Strdup("not a directory");
    }

	return (result);
}
/***************************************************************/
int cvt_fil_attr_rec(struct dirInfo* di,
                        void * dbuf,
                        struct filattrrec * fa) {
 
    struct dirdata * dd = (struct dirdata*)dbuf;

    cvt_windows_fil_attr_rec(fa, &(dd->FindFileData));

	return (0);
}
/***************************************************************/
int open_dir(struct dirInfo* di, const char * dname, void ** pdbuf) {

    int err = 0;
    struct dirdata * dd;
    char fdname[MAX_PATH];
    int  fdnamlen;

    strcpy(fdname, dname);
    fdnamlen = IStrlen(fdname);
    if (fdnamlen && fdname[fdnamlen-1] != DIR_DELIM) {
        fdname[fdnamlen++] = DIR_DELIM;
        fdname[fdnamlen]   = '\0';
    }

    fdname[fdnamlen++] = '*';
    fdname[fdnamlen]   = '\0';

    *pdbuf = 0;
    dd = New(struct dirdata, 1);
    dd->adv = 0;

    dd->hFind = FindFirstFile((LPCTSTR)fdname, &(dd->FindFileData));

    if (dd->hFind == INVALID_HANDLE_VALUE) {
        di->di_oserr  = GetLastError();
        di->di_errmsg = GetWinMessage(di->di_oserr);
        di->di_stat   = DIRERR_OPEN_DIR;
        Free(dd);
        return (di->di_stat);
    }

    (*pdbuf) = dd;

    return (err);
}
/***************************************************************/
int read_dir(struct dirInfo* di, void * dbuf, char * buf, int bufsize, int * eof) {

    int err = 0;
    int ern;
    struct dirdata * dd = (struct dirdata*)dbuf;
    BOOL bResult;

    *eof = 0;
    if (dd->adv) {
        bResult = FindNextFile(dd->hFind, &(dd->FindFileData));
        if (!bResult) {
            ern = GetLastError();
            *eof = 1;
            if (ern != ERROR_NO_MORE_FILES) {
                di->di_oserr  = ern;
                di->di_errmsg = GetWinMessage(di->di_oserr);
                di->di_stat   = DIRERR_READ_DIR;
                err = di->di_stat;
            }
        }
    }

    if (!(*eof)) {
        strcpy(buf, (char*)dd->FindFileData.cFileName);
        dd->adv = 1;
        dd->fil_attr = get_windows_fil_attr(dd->FindFileData.dwFileAttributes);
    }

    return (err);
}
/***************************************************************/
int close_dir(struct dirInfo* di, void * dbuf) {

    int err = 0;
    struct dirdata * dd = (struct dirdata*)dbuf;

    FindClose(dd->hFind);
    Free(dd);

    return (err);
}
/***************************************************************/
#else
/***************************************************************/
/* Unix directory functions                                    */
/***************************************************************/
struct dirdata {
    DIR * dir;
};
/***************************************************************/
int open_dir(struct dirInfo* di, const char * dname, void ** pdbuf) {

    int err = 0;
    struct dirdata * dd;

    *pdbuf = 0;
    dd = New(struct dirdata, 1);

    dd->dir = opendir(dname);
    if (!dd->dir) {
        Free(dd);
        return (DIRERR_OPEN_DIR);
    }

    (*pdbuf) = dd;

    return (err);
}
/***************************************************************/
int read_dir(struct dirInfo* di, void * dbuf, char * buf, int bufsize, int * eof) {

    int err = 0;
    struct dirdata * dd = (struct dirdata*)dbuf;
    struct dirent * de;

    *eof = 0;
    de = readdir(dd->dir);
    if (!de) {
        *eof = 1;
    } else {
        strcpy(buf, de->d_name);
    }

    return (err);
}
/***************************************************************/
int close_dir(struct dirInfo* di, void * dbuf) {

    int err = 0;
    struct dirdata * dd = (struct dirdata*)dbuf;

    closedir(dd->dir);
    Free(dd);

    return (err);
}
#endif
/***************************************************************/
struct rdrec {
    struct dirInfo * rdr_di;
    void           * rdr_vd;
};
/***************************************************************/
int open_directory(void * vrdr, const char * dname) {

    struct rdrec * rdr = (struct rdrec *)vrdr;

    return open_dir(rdr->rdr_di, dname, &(rdr->rdr_vd));
}
/***************************************************************/
int read_directory(void * vrdr, char * buf, int bufsize, int * eof) {

    struct rdrec * rdr = (struct rdrec *)vrdr;

    return read_dir(rdr->rdr_di, rdr->rdr_vd, buf, bufsize, eof);
}
/***************************************************************/
void close_directory(void * vrdr) {

    struct rdrec * rdr = (struct rdrec *)vrdr;

    if (rdr->rdr_vd) {
        close_dir(rdr->rdr_di, rdr->rdr_vd);
        rdr->rdr_vd    = NULL;
    }
}
/***************************************************************/
void * new_read_directory(void) {

    struct rdrec * rdr;

    rdr = New(struct rdrec, 1);

    rdr->rdr_di    = new_dirInfo();
    rdr->rdr_vd    = NULL;

    return (rdr);
}
/***************************************************************/
void close_read_directory(void * vrdr) {

    struct rdrec * rdr = (struct rdrec *)vrdr;

    if (rdr) {
        if (rdr->rdr_di) {
            if (rdr->rdr_vd) {
                close_dir(rdr->rdr_di, rdr->rdr_vd);
            }
            free_dirInfo(rdr->rdr_di);
        }
        Free(rdr);
    }        
}
/***************************************************************/
int get_errstat_directory(void * vrdr) {

    struct rdrec * rdr = (struct rdrec *)vrdr;

    return (dx_stat(rdr->rdr_di));
}
/***************************************************************/
char * get_errmsg_directory(void * vrdr) {

    struct rdrec * rdr = (struct rdrec *)vrdr;

    return (dx_errmsg(rdr->rdr_di));
}
/***************************************************************/
/***************************************************************/
/***************************************************************/
int get_fil_attr(void * dbuf) {

    /* struct dirdata * dd = (struct dirdata*)dbuf; */

    /* return (dd->fil_attr); XFIX */
    return (0);
}
/***************************************************************/
int dir_scan(struct dirInfo* di,
                struct filattrrec * fa,
                char ** dirpats,
                const char * pfilpat,
                int dir_opts,
                int (*dirent_callback)(void * cbi, struct filattrrec * fa),
                void * cbi,
                int depth) {

    int err = 0;
    void * vp;
    int done;
    int eof;
    struct filattrrec n_farec;
    char filename[FILE_MAXIMUM];

    if (depth > MAX_DIR_RECUR_DEPTH) {
        return (DIRERR_TOO_DEEP);
    }

    if ((dir_opts & DIROPT_RECURSIVE) && !(dir_opts & DIROPT_POST_ORDER)) {
        if (!pfilpat || !pfilpat[0]) {
            if (dirent_callback(cbi, fa)) {
                return (DIRERR_CALLBACK_ERR);
            }
        }
    }

    err = open_dir(di, fa->fa_filepath, &vp);
    if (err) return (err);

    strcpy(n_farec.fa_filepath, fa->fa_filepath);
    n_farec.fa_name_ix = IStrlen(n_farec.fa_filepath);

    if (n_farec.fa_name_ix && n_farec.fa_filepath[n_farec.fa_name_ix-1] != DIR_DELIM) {
        n_farec.fa_filepath[n_farec.fa_name_ix++] = DIR_DELIM;
        n_farec.fa_filepath[n_farec.fa_name_ix]   = '\0';
    }

    done = 0;
    while (!done && !err) {
        if (global_user_break)
            err = DIRERR_CTL_C;
        else
            err = read_dir(di, vp, filename, FILE_MAXIMUM, &eof);
        if (err || eof) {
            done = 1;
        } else if ((dir_opts & DIROPT_DOT_DOTDOT) ||
                    (strcmp(filename, ".") && strcmp(filename, ".."))) {
            strcpy(n_farec.fa_filepath + n_farec.fa_name_ix, filename);

            err = cvt_fil_attr_rec(di, vp, &n_farec);
            if (filename[0] == '.' &&
            	(!(dir_opts & DIROPT_DOT_DOTDOT) ||
                    (strcmp(filename, ".") && strcmp(filename, ".."))))
                n_farec.fa_fil_attr |= FATTR_HIDDEN;

            if (!err) {
                if (n_farec.fa_fil_attr & FATTR_IS_DIR) {
                    if (!dirpats || !dirpats[0]) {
                        if (dir_opts & DIROPT_RECURSIVE) {
                            err = dir_scan(di, &n_farec, NULL, pfilpat, dir_opts,
                                            dirent_callback, cbi, depth + 1);
                        } else {
                            if (!pfilpat || !pfilpat[0] || (dir_opts & DIROPT_ALL_DIRS) ||
                                patmatches((dir_opts & DIROPT_CASE)?1:0, pfilpat, filename)) {
                                if (dirent_callback(cbi, &n_farec)) {
                                    err = DIRERR_CALLBACK_ERR;
                                }
                            }
                        }
                    } else if (patmatches((dir_opts & DIROPT_CASE)?1:0, dirpats[0], filename)) {
                        if ((!pfilpat || !pfilpat[0]) && (dir_opts & DIROPT_ONE_LEVEL)) {
                            if (dirent_callback(cbi, &n_farec)) {
                                err = DIRERR_CALLBACK_ERR;
                            }
                        }
                        if (!err && !(dir_opts & DIROPT_ONE_LEVEL)) {
                            err = dir_scan(di, &n_farec, dirpats + 1, pfilpat, dir_opts,
                                         dirent_callback, cbi, depth + 1);
                        }
                    }
                } else { /* !(n_farec.fa_fil_attr & FATTR_IS_DIR) */
                    if (!dirpats || !dirpats[0]) {
                        if (!pfilpat || !pfilpat[0] ||
                            patmatches((dir_opts & DIROPT_CASE)?1:0, pfilpat, filename)) {
                            if (dirent_callback(cbi, &n_farec)) {
                                err = DIRERR_CALLBACK_ERR;
                            }
                        }
                    }
                }
            }
        }
    }

    if (err) {
        close_dir(di, vp);
    } else {
        err = close_dir(di, vp);
        
        if (!err && (dir_opts & DIROPT_RECURSIVE) && (dir_opts & DIROPT_POST_ORDER)) {
            if (!pfilpat || !pfilpat[0]) {
                if (dirent_callback(cbi, fa)) {
                    return (DIRERR_CALLBACK_ERR);
                }
            }
        }
    }

    return (err);
}
/***************************************************************/
void * scan_directory(
                    const char * dirnam,
                    char ** dirpats,
                    const char * pfilpat,
                    int dir_opts,
                    int (*dirent_callback)(void * cbi, struct filattrrec * fa),
                    void * cbi) {

    struct dirInfo* di;
    struct filattrrec farec;

    di = new_dirInfo();

    di->di_stat = get_fil_attr_rec(di, dirnam, &farec);

    if (!di->di_stat) {
        if (farec.fa_fil_attr & FATTR_IS_DIR) {
            di->di_stat = dir_scan(di, &farec, dirpats, pfilpat, dir_opts,
                                     dirent_callback, cbi, 1);
        } else {
            if (dirent_callback(cbi, &farec)) {
                di->di_stat = DIRERR_CALLBACK_ERR;
            }
        }
    }

    return (di);
}
/***************************************************************/
void get_cwd(char *cwd, int cwdmax) {

    if (!GETCWD(cwd, cwdmax)) {
        cwd[0] = '\0';
    }
}
/***************************************************************/
struct dedirent {
    /* char                de_filename[FILE_MAXIMUM]; */
    struct filattrrec   de_fattr;
};
struct dirfiles {
    char *              df_dirname;
    int                 df_dnlen;
    int                 df_nents;
    int                 df_mxents;
    int                 df_cmdflags;
    struct dedirent **  df_dirent;
};
struct scanrec {
    struct cenv *       sr_ce;
    int                 sr_cmdflags;
    int                 sr_estat;
    int                 sr_namix;
    struct dirfiles *   sr_df;
};
/***************************************************************/
static struct dirfiles* new_dirfiles(int cmdflags)
{
    struct dirfiles * df;

    df = New(struct dirfiles, 1);

    df->df_dirname  = NULL;
    df->df_dnlen    = 0;
    df->df_mxents   = 0;
    df->df_nents    = 0;
    df->df_cmdflags = cmdflags;
    df->df_dirent   = NULL;

    return (df);
}
/***************************************************************/
static void free_dirfiles(struct dirfiles * df)
{
    int ii;

    for (ii = 0; ii < df->df_mxents; ii++) Free(df->df_dirent[ii]);
    Free(df->df_dirent);

    Free(df->df_dirname);
    Free(df);
}
/***************************************************************/
static void add_dirfile(struct dirfiles * df, const char * dfnam, struct filattrrec * fa)
{
    int ii;

    if (df->df_nents == df->df_mxents) {
        if (df->df_mxents) df->df_mxents *= 2; /* chunk */
        else df->df_mxents = 8;
        df->df_dirent = Realloc(df->df_dirent, struct dedirent *, df->df_mxents);
        for (ii = df->df_nents; ii < df->df_mxents; ii++) {
            df->df_dirent[ii] = New(struct dedirent, 1);
        }
    }
    ii = df->df_nents;
    memcpy(&(df->df_dirent[ii]->de_fattr), fa, sizeof(struct filattrrec));

    df->df_nents += 1;
}
/***************************************************************/
static struct scanrec* new_scanrec(struct cenv * ce, int cmdflags)
{
    struct scanrec * sr;

    sr = New(struct scanrec, 1);
    sr->sr_ce         = ce;
    sr->sr_cmdflags   = cmdflags;
    sr->sr_estat      = 0;
    sr->sr_namix      = 0;
    sr->sr_df         = NULL;

    return (sr);
}
/***************************************************************/
static void free_scanrec(struct scanrec * sr)
{
    if (sr->sr_df) free_dirfiles(sr->sr_df);
    Free(sr);
}
/***************************************************************/
#if IS_WINDOWS
int has_drive_name(const char * dirname) {

    int has_drive;

    has_drive = 0;

    if (isalpha(dirname[0]) && dirname[1] == ':' &&
        (!dirname[2] || dirname[2] == DIR_DELIM)) {
        has_drive = 2;
    }

    return (has_drive);
}
#else
int has_drive_name(const char * dirname) {

    return (0);
}
#endif
/***************************************************************/
/*@*/char * construct_fullname(char * cwd,
                        char * name,
                        char * fullname,
                        int fullmax) {

    char fname[PATH_MAXIMUM];
    int len;
    int ii;
    int jj;
    int isdir;
    char dirdelim;

    dirdelim = DIR_DELIM;
    if (has_drive_name(name) || name[0] == dirdelim) {
        len = IStrlen(name);
        if (len >= fullmax) return (0);
        strcpy(fullname, name);
        return (fullname);
    }

    len = IStrlen(cwd);
    if (!len || len + strlen(name) >= PATH_MAXIMUM) return (0);

    ii = 0;
    jj = 0;
    isdir = 0;

    strcpy(fname, cwd);
    if (fname[len - 1] != dirdelim) {
        fname[len++] = dirdelim;
        fname[len] = '\0';
    }
    strcpy(fname + len, name);

    while (fname[ii]) {
        if (isdir && fname[ii] == '.' && fname[ii+1] == '.' &&
            (fname[ii+2] == dirdelim || !fname[ii+2])) {
            jj--;
            while (jj > 0 && fullname[jj - 1] != dirdelim) jj--;
            if (!jj) return (0);
            ii += 2;
            if (fname[ii]) ii++;
        } else if (isdir && fname[ii] == '.' && (fname[ii+1] == dirdelim || !fname[ii+1])) {
            ii++;
            if (fname[ii]) ii++;
        } else {
            if (jj < fullmax) fullname[jj++] = fname[ii];
            if (fname[ii] == dirdelim) isdir = 1;
            else                       isdir = 0;
            ii++;
        }
    }

    if (jj >= fullmax) return (0);

    if (jj > 2 && fullname[jj - 1] == '.' && fullname[jj - 2] == dirdelim) {
        jj -= 1;
    }

    if (jj > 1 && fullname[jj - 1] == dirdelim) {
        if (has_drive_name(name)) {
            if (jj != 3) jj -= 1;
        } else {
            jj -= 1;
        }
    }

    fullname[jj] = '\0';

    return (fullname);
}
/***************************************************************/
/*@*/char * ux_fullyqualify(char * fullname, char * name, int fullmax) {
/*
** Can be called from Windows while testing.
*/

    char * out;
    char cwd[PATH_MAXIMUM];

    get_cwd(cwd, PATH_MAXIMUM);
    out = construct_fullname(cwd, name, fullname, fullmax);

    return (out);
}
/***************************************************************/
#if IS_WINDOWS
/*@*/char * win_fullyqualify(char * fullname, char * name, int fullmax) {

    char * out = 0;

#if IS_DEBUG
    out = ux_fullyqualify(fullname, name, fullmax);
#else
    out = _fullpath(fullname, name, fullmax);
#endif

    return (out);
}
#endif
/***************************************************************/
/*@*/char * fullyqualify(char * fullname, char * name, int fullmax) {

    char * out;

#if IS_WINDOWS
    out = win_fullyqualify(fullname, name, fullmax);
#else
    out = ux_fullyqualify(fullname, name, fullmax);
#endif

    return (out);
}
/***************************************************************/
int splitwild(const char * fullname,    /* in : full file name or pattern */
                char * dirnam,          /* out: top directory name */
                char *** pdirpats,      /* out: directory pattern list, if any */
                char * filpat,          /* out: file name or pattern */
                int swflags) {          /* in : SWFLAG_DOT_STAR = strip trailing *.* */
    int badfs;
    int fwild;
    int ii;
    int fnlen;
    int ddelix;
    int bgn_ii;
    int dirft;
    char ** dirpats;
    int ndirpats;
    int mxdirpats;

/*
** abcde/fgh
** 0123456789
**
** fnlen = 9
**      dirnam = "abcde"
**      dirpat = ""
**      filpat = "fgh"
**
** ab/efg?/hi*
** 000000000011
** 012345678901
**
** fnlen = 10
**      dirnam = "ab"
**      dirpat = "efg?"
**      filpat = "hi*"
**
*/
    badfs = 0;
    fwild = 0;

    fnlen = IStrlen(fullname);
    if (fnlen >= PATH_MAXIMUM) return (1);

    dirnam[0] = '\0';
    filpat[0] = '\0';
    dirpats   = NULL;
    (*pdirpats)   = NULL;
    ndirpats  = 0;
    mxdirpats = 0;

    ii = fnlen;
    while (ii > 0 && !IS_DIR_DELIM(fullname[ii - 1])) {
        ii--;
        if (IS_WILD(fullname[ii])) fwild = 1;
    }
    if (fnlen - ii >= FILE_MAXIMUM) return (1);

    if (!ii) {
        if (fwild) {
            memcpy(filpat, fullname, fnlen + 1);
        } else {
            dirft = file_stat(fullname);

            if (dirft == FTYP_DIR || dirft == FTYP_DOT_DIR) {
                memcpy(dirnam, fullname, fnlen + 1);
            } else {
                memcpy(filpat, fullname, fnlen + 1);
            }
        }
    } else {
        memcpy(filpat, fullname + ii, fnlen - ii + 1);
        fnlen = ii;

        ii = 0;
        ddelix = -1;
        while (ii < fnlen && !IS_WILD(fullname[ii])) {
            if (IS_DIR_DELIM(fullname[ii])) ddelix = ii;
            ii++;
        }

        if (ii == fnlen) {
            memcpy(dirnam, fullname, fnlen); /* no wildcards in dirnam */
            if (!fwild) {
                strcpy(dirnam + fnlen, filpat);
                filpat[0] = '\0';
            } else {
                dirnam[fnlen - 1] = '\0';
            }
        } else {
            /* IS_WILD(dirnam[ii]) */
            if (ddelix < 0) {
                ii = 0;
            } else {
                if (!ddelix) {
                    dirnam[1] = '\0';

#if IS_WINDOWS
                } else if (ddelix == 2 && dirnam[1] == ':' && isalpha(dirnam[0])) {
                    dirnam[0] = fullname[0];
                    dirnam[1] = fullname[1];
                    dirnam[2] = fullname[2];
                    dirnam[3] = '\0';
#endif
                } else {
                    memcpy(dirnam, fullname, ddelix);
                    dirnam[ddelix] = '\0';
                }
                ii = ddelix + 1;
            }
            while (ii < fnlen) {
                bgn_ii = ii;
                while (ii < fnlen && !IS_DIR_DELIM(fullname[ii])) ii++;
                if (bgn_ii == ii) badfs = 1; /* contains // */
                if (ndirpats == mxdirpats) {
                    if (!mxdirpats) mxdirpats = 4; /* chunk */
                    else mxdirpats *= 2;
                    dirpats = Realloc(dirpats, char*, mxdirpats + 1);
                }
                dirpats[ndirpats] = New(char, ii - bgn_ii + 1);
                memcpy(dirpats[ndirpats], fullname + bgn_ii, ii - bgn_ii);
                dirpats[ndirpats][ii - bgn_ii] = '\0';
                ndirpats++;
                ii++;
            }
        }
    }
    if (dirpats) {
        dirpats[ndirpats] = NULL;
    }

    if (badfs) {
        free_charlist(dirpats);
        dirpats = NULL;
    }

    (*pdirpats)   = dirpats;

    if (swflags & SWFLAG_DOT_STAR) {
        ii = IStrlen(filpat);
        if (ii >= 3 && !memcmp(filpat + ii - 3, "*.*", 3)) {
            filpat[ii - 2] = '\0'; /* remove trailing .* */
        }
    }

    return (badfs);
}
/***************************************************************/
struct getfilesrec {
    int                 gfr_estat;
    int                 gfr_namix;
    int                 gfr_mxfiles;
    int                 gfr_nfiles;
    char **             gfr_fnamelist;
};
/***************************************************************/
static struct getfilesrec* new_getfilesrec(void)
{
    struct getfilesrec * gfr;

    gfr = New(struct getfilesrec, 1);
    gfr->gfr_estat      = 0;
    gfr->gfr_namix      = 0;
    gfr->gfr_mxfiles    = 0;
    gfr->gfr_nfiles     = 0;
    gfr->gfr_fnamelist  = NULL;

    return (gfr);
}
/***************************************************************/
static void free_getfilesrec(struct getfilesrec * gfr)
{
    Free(gfr);
}
/***************************************************************/
void append_charlist(char *** p_charlist, char * buf, int * cllen, int * clmax) {

    if ((*cllen) + 1 >= (*clmax)) {
        if (!(*clmax)) (*clmax) = CHARLIST_CHUNK;
        else           (*clmax) *= 2;
        (*p_charlist) = Realloc((*p_charlist), char*, (*clmax));
    }

    (*p_charlist)[(*cllen)] = buf;
    (*cllen) += 1;
    (*p_charlist)[(*cllen)] = NULL;
}
/***************************************************************/
int getfilesrec_callback(void * vgfr, struct filattrrec * fa) {

    struct getfilesrec * gfr = (struct getfilesrec *)vgfr;
    char * fnam;

    if (!(fa->fa_fil_attr & FATTR_HIDDEN)) {
        fnam = Strdup(fa->fa_filepath + gfr->gfr_namix);
        append_charlist(&(gfr->gfr_fnamelist), fnam, &(gfr->gfr_nfiles), &(gfr->gfr_mxfiles));
    }

    return (gfr->gfr_estat);
}
/***************************************************************/
int get_filenames(const char * dfname, char *** fnamelist) {

    int estat;
    int swflags;
    struct getfilesrec * gfr;
    void * vdi;
    int dir_opts;
    char ** dirpats;
    int dir_flags;
    char dirnam[PATH_MAXIMUM];
    char filpat[FILE_MAXIMUM];

    estat           = 0;
    swflags         = 0;
    (*fnamelist)    = NULL;

    dir_flags = 0;
#if IS_WINDOWS
    dir_flags |= DIRFLAG_WIN;
#endif

    dir_opts        = 0;
    if (dir_flags & DIRFLAG_CASE)   dir_opts |= DIROPT_CASE;
    if (dir_flags & DIRFLAG_WIN)    swflags  |= SWFLAG_DOT_STAR;
    if (!(dir_flags & DIRFLAG_WIN)) dir_opts |= DIROPT_DOT_HIDES;

    if (splitwild(dfname, dirnam, &dirpats, filpat, swflags)) {
        estat = set_error(NULL, "Invalid wildcard: \"%s\"", dfname);
        return (estat);
    }

    gfr = new_getfilesrec();

    gfr->gfr_namix = 0;
    if (!dirnam[0]) gfr->gfr_namix = 2; /* Hide leading ./ */

    vdi = scan_directory(dirnam, dirpats, filpat, dir_opts, getfilesrec_callback, gfr);
    if (dx_stat(vdi)) {
        if (dx_stat(vdi) == DIRERR_CALLBACK_ERR) {
            estat = set_error(NULL, "Scan directory callback error.");
        } else {
            estat = set_error(NULL, "Scan directory error #%d: %s",
                dx_stat(vdi), dx_errmsg(vdi));
        }
    }
    dx_free(vdi);

    if (!estat) {
        (*fnamelist) = gfr->gfr_fnamelist;
    } else {
        free_charlist(gfr->gfr_fnamelist);
    }
    free_charlist(dirpats);

    free_getfilesrec(gfr);

    return (estat);
}
/***************************************************************/
