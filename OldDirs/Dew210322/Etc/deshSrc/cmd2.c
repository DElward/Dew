/***************************************************************/
/* cmd2.c                                                      */
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


/***************************************************************/
/* Things needed for directory reading                         */

#if IS_WINDOWS
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#else
    #include <dirent.h>
    #include <unistd.h>
    #include <utime.h>
#endif


#include "desh.h"
#include "var.h"
#include "cmd.h"
#include "cmd2.h"
#include "when.h"
#include "assoc.h"
#include "msg.h"
#include "var.h"
#include "snprfh.h"
#include "when.h"

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

	ii = Istrlen(fullname);

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

	ii = Istrlen(fullname);
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

	ii = Istrlen(fullname);

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

	dnlen = Istrlen(dirname);
	fnlen = Istrlen(fname);

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

	dnlen = Istrlen(dirname);
	fnlen = Istrlen(fname);
	exlen = Istrlen(fext);
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
/***************************************************************/
int get_ux_fil_attr(int ux_fil_attr) {
    
    int fil_attr;
    
    fil_attr = 0;
    
    if (ux_fil_attr & _S_IFDIR)                 fil_attr |= FATTR_IS_DIR;
    else                                        fil_attr |= FATTR_FILE;

    if (!(ux_fil_attr & FSTAT_S_IWRITE_ALL))    fil_attr |= FATTR_READ_ONLY;

    return (fil_attr);
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
/*@*/int get_winfile_attr(const char * fname) {

	int fAttr;
    int result = 0;

	fAttr = GetFileAttributes(fname);
	if (fAttr <  0) {
        result = FTYP_NONE;
	} else {
        result = get_windows_fil_attr(fAttr);
    }

	return (result);
}
/***************************************************************/
int reverse_windows_fil_attr(int fil_attr) {

    int win_fil_attr;

    win_fil_attr = 0;

    if (fil_attr & FATTR_IS_DIR     ) win_fil_attr |= FILE_ATTRIBUTE_DIRECTORY;
    if (fil_attr & FATTR_READ_ONLY  ) win_fil_attr |= FILE_ATTRIBUTE_READONLY;
    if (fil_attr & FATTR_HIDDEN     ) win_fil_attr |= FILE_ATTRIBUTE_HIDDEN;

    return (win_fil_attr);
}
/***************************************************************/
/*@*/int set_winfile_attr(const char * fname, int attr_mask, int new_attr) {
/*
** Returns:
**      Success        - 0
**      Fail           - GetLastError()
*/
    int result = 0;
    int win_attr_mask;
    int win_new_attr;
	DWORD  fAttr;
	DWORD  newfAttr;

	fAttr = GetFileAttributes(fname);
	if (fAttr <  0) {
        result = GETERRNO;
	} else {
        win_attr_mask = reverse_windows_fil_attr(attr_mask);
        win_new_attr  = reverse_windows_fil_attr(new_attr);

        newfAttr = (fAttr & ~((DWORD)win_attr_mask)) | ((DWORD)win_attr_mask & (DWORD)win_new_attr);
        if (!SetFileAttributes(fname, newfAttr)) {
            result = GETERRNO;
        } else {
            result = 0;
        }
    }

	return (result);
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

    ii = Istrlen(cpMsgBuf);
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

    namix = Istrlen(fname);
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

    hFind = FindFirstFile(fa->fa_filepath, &FindFileData);
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
    fdnamlen = Istrlen(fdname);
    if (fdnamlen && fdname[fdnamlen-1] != DIR_DELIM) {
        fdname[fdnamlen++] = DIR_DELIM;
        fdname[fdnamlen]   = '\0';
    }

    fdname[fdnamlen++] = '*';
    fdname[fdnamlen]   = '\0';

    *pdbuf = 0;
    dd = New(struct dirdata, 1);
    dd->adv = 0;

    dd->hFind = FindFirstFile(fdname, &(dd->FindFileData));

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
        strcpy(buf, dd->FindFileData.cFileName);
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
int cvt_ux_fil_attr_rec(struct filattrrec * fa) {
    /*
     struct filattrrec {
     int     fa_fil_attr;
     uint32  fa_file_size_hi;
     uint32  fa_file_size_lo;
     time_t  fa_mod_date;
     time_t  fa_cr_date;
     int     fa_name_ix;
     char    fa_filepath[PATH_MAXIMUM];
     };
     
     struct stat {
     _dev_t st_dev;
     _ino_t st_ino;
     unsigned short st_mode;
     short st_nlink;
     short st_uid;
     short st_gid;
     _dev_t st_rdev;
     _off_t st_size;
     time_t st_atime;
     time_t st_mtime;
     time_t st_ctime;
     };
     
     */
    int result = 0;
    struct stat stbuf;
    /*
    struct tm * ltt;
    struct tm tt;
    time_t ct;
    
    ct = time(NULL);
    ltt = localtime(&ct);
    memcpy(&tt, ltt, sizeof(tt));
    tt.tm_year -= 2;
    tt.tm_mon  = 11;
    tt.tm_mday = 8;
    tt.tm_hour = 13;
    tt.tm_min  = 17;
    ct = mktime(&tt);
     */
    
    result = LSTAT(fa->fa_filepath, &stbuf);
    if (!result) {
        fa->fa_fil_attr = get_ux_fil_attr(stbuf.st_mode);
        
        fa->fa_cr_date  = STAT_CTIME(stbuf);
        fa->fa_mod_date = STAT_MTIME(stbuf);
        fa->fa_file_size_lo = STAT_FSIZE_LO(stbuf);
        fa->fa_file_size_hi = STAT_FSIZE_HI(stbuf);
    }
    
    return (result);
}
/***************************************************************/
int get_fil_attr_rec(struct dirInfo* di,
                     const char * fname,
                     struct filattrrec * fa) {
    int result = 0;
    
    if (!fname[0]) {
        strcpy(fa->fa_filepath, "."); /* cwd */
    } else {
        strcpy(fa->fa_filepath, fname);
    }
    
    result = cvt_ux_fil_attr_rec(fa);
    
    return (result);
}
/***************************************************************/
int cvt_fil_attr_rec(struct dirInfo* di,
                     void * dbuf,
                     struct filattrrec * fa) {
    
    int result = 0;
    
    result = cvt_ux_fil_attr_rec(fa);
    
    return (result);
}
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
    n_farec.fa_name_ix = Istrlen(n_farec.fa_filepath);

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

#ifdef REMOVE_DIR
/***************************************************************/
/*  removedir - Remove directory                               */
/***************************************************************/
#define VERSION "0.03"
/*
Version Date     Description
0.00    08-27-07 Began work
0.01    08-27-07 First version.
0.02    07-22-08 Added -cont option
0.03    06-27-12 Automatically delete read-only files on Windows

Date     Enhancement request
*/
/***************************************************************/
#define PROGRAM_NAME "REMOVEDIR"
/***************************************************************/
#define strdup(s)        (strcpy(New(char,strlen(s)+1),(s)))
#define MAX(n,m)         ( (n)>(m) ? (n) : (m) )
#define MIN(n,m)         ( (n)<(m) ? (n) : (m) )

#define WHMEM 0

#if WHMEM
    #include "CMEMH"
#else
    #define New(t,n)         ((t*)calloc(sizeof(t),(n)))
    #define Realloc(p,t,n)   ((t*)realloc((p),sizeof(t)*(n)))
    #define Free(p)           free(p)
#endif

/***************************************************************/
#define OPT_PAUSE       1
#define OPT_HELP        2
#define OPT_SHOW        4
#define OPT_NODEL       8
#define OPT_VERSION     16
#define OPT_CONT        32
#define OPT_TRY_HARDER  64
#define OPT_STATS       128
/***************************************************************/
/*
** Globals
*/
int gflags;                     /* Run flags */
long gnum_files_deleted;        /* Run flags */
long gnum_files_not_deleted;    /* Run flags */
long gnum_dirs_deleted;         /* Run flags */
long gnum_dirs_not_deleted;     /* Run flags */
/***************************************************************/
int set_error (char * fmt, ...);
/***************************************************************/

































/***************************************************************/
/* removedir.c                                                 */
/***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>
#include <direct.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>



int is_directory(char * fname);
int open_dir(const char * dname, void ** pdbuf);
int read_dir(void * dbuf, char * buf, int bufsize, int * eof);
int close_dir(void * dbuf);
/***************************************************************/

// #include "load.h"
// #include "makemap.h"
// #include "tree.h"
// #include "mapmerge.h"
// #include "eval.h"

#include "removedir.h"

/***************************************************************/
/*
    . SV_ is the ODS target table
    . IMAGE is the original Ecometry source
    . ECOM is the SQL Ecometry source

    . For each SV_ table mapping
      . Find IMAGE source table
        . Find corresponding ECOM table
      . For each SV_ column mapping
        . Parse expression and for each IMAGE column used
            . Find corresponding ECOM column
            . Find implicit conversion used to get from IMAGE to ECOM
            . Use conversion to replace IMAGE column with ECOM column
        . Output new mapping

*/
/***************************************************************/

/***************************************************************/
int is_directory(const char * fname) {

    struct stat stbuf;
    int result;
    int out;
    int fnlen;

    fnlen = strlen(fname);
    if (fnlen && fname[fnlen-1] == DIR_DELIM) {
        out = 1;
    } else {
        out = 0;
        result = lstat(fname, &stbuf);
        if (!result) {
            if (stbuf.st_mode & __S_IFDIR) out = 1;
        }
    }

    return (out);
}
/***************************************************************/

/***************************************************************/
void showusage(char * prognam) {

    printf("%s - v%s\n", PROGRAM_NAME, VERSION);
    printf("\n");
    printf("Usage: %s [options] <config file name>\n", prognam);
    printf("\n");
    printf("Options:\n");
    printf("  -cont              Continue on error.\n");
    printf("  -help              Show extended help.\n");
    printf("  -nodel             Don\'t delete files and directories.\n");
    printf("  -pause             Pause when done.\n");
    printf("  -show              Show files and directories.\n");
    printf("  -stats             Show stats when done.\n");
    printf("  -tryharder         Try harder on error.\n");
    printf("  -version           Show program version.\n");
    printf("\n");
}
/***************************************************************/
int set_error (char * fmt, ...) {

    va_list args;

    va_start (args, fmt);
    vfprintf(stderr, fmt, args);
    va_end (args);
    fprintf(stderr, "\n");

    return (-1);
}
/***************************************************************/
void init(void) {

    gnum_files_deleted          = 0;
    gnum_files_not_deleted      = 0;
    gnum_dirs_deleted           = 0;     
    gnum_dirs_not_deleted       = 0; 
}
/***************************************************************/
void shut(void) {

    if (gflags & OPT_STATS) {
        printf("Files deleted           : %ld\n", gnum_files_deleted);
        printf("Directories deleted     : %ld\n", gnum_dirs_deleted);
        if (gflags & OPT_CONT) {
            printf("Files not deleted       : %ld\n", gnum_files_not_deleted);
            printf("Directories not deleted : %ld\n", gnum_dirs_not_deleted);
        }
    }
}
/***************************************************************/
char * GetWinMessage(long errCode) {

    LPVOID lpMsgBuf;
    int ii;
    static char winmsg[256];

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

    strmcpy(winmsg, lpMsgBuf, sizeof(winmsg));

    LocalFree(lpMsgBuf);

    ii = strlen(winmsg);
    while (ii > 0 && isspace(winmsg[ii-1])) ii--;
    winmsg[ii] = '\0';

    return (winmsg);
}
/***************************************************************/
int make_deletable(char* filnam) {

    int err;

    err = 0;
    if (_chmod( filnam, _S_IWRITE | _S_IREAD)) {
        int en = errno;
        err = set_error("Error #%d on chmod(): %s\n%s",
                en, filnam, strerror(en));
    }

    return (err);
}
/***************************************************************/
int delete_file(char* filnam, int fil_attr) {

    int err;

    err = 0;
    if (!(gflags & OPT_NODEL)) {
        /* Added 6/27/2012 */
        if (fil_attr & FATTR_READ_ONLY) {
            err = make_deletable(filnam);
        }
        if (!err) {
            err = remove(filnam);
        }

        if (!err) {
            gnum_files_deleted++;
        } else {
            if (gflags & OPT_TRY_HARDER) {
                err = make_deletable(filnam);

                if (err) {
                    gnum_files_not_deleted++;
                } else {
                    if (DeleteFile(filnam)) {
                        gnum_files_deleted++;
                    } else {
                        int en = GetLastError();
                        err = set_error("Error #%d on DeleteFile(): %s\n%s",
                                    en, filnam, GetWinMessage(en));
                        gnum_files_not_deleted++;
                    }
                }
            } else {
                int en = errno;
                err = set_error("Error #%d removing file: %s\n%s",
                        en, filnam, strerror(en));
                gnum_files_not_deleted++;
            }             
        }
    }

    if (gflags & OPT_SHOW) {
        printf("File: %s\n", filnam);
    }

    return (err);
}
/***************************************************************/
int delete_dir(char* dirnam, int fil_attr) {

    int err;

    err = 0;
    if (!(gflags & OPT_NODEL)) {

        /* Added 6/27/2012 */
        if (fil_attr & FATTR_READ_ONLY) {
            err = make_deletable(dirnam);
        }
        if (!err) {
            err = rmdir(dirnam);
        }

        if (!err) {
            gnum_dirs_deleted++;
        } else {
            if (gflags & OPT_TRY_HARDER) {
                err = make_deletable(dirnam);

                if (err) {
                    gnum_dirs_not_deleted++;
                } else {
                    if (RemoveDirectory(dirnam)) {
                        gnum_dirs_deleted++;
                    } else {
                        int en = GetLastError();
                        err = set_error("Error #%d on RemoveDirectory(): %s\n%s",
                                    en, dirnam, GetWinMessage(en));
                        gnum_dirs_not_deleted++;
                    }
                }
            } else {
                int en = GetLastError();
                err = set_error("Error #%d removing directory: %s\n%s",
                    en, dirnam, GetWinMessage(en));
                gnum_dirs_not_deleted++;
            }
        }
    }

    if (gflags & OPT_SHOW) {
        printf("Dir : %s\n", dirnam);
    }

    return (err);
}
/***************************************************************/
int remove_dir(char* dirnam, int fil_attr, int depth);
int remove_dir_or_file(char* filepath, int depth, int fil_attr) {

    int err;

    if (fil_attr & FATTR_IS_DIR) {
        err = remove_dir(filepath, fil_attr, depth + 1);
    } else {
        err = delete_file(filepath, fil_attr);
    }

    return (err);
}
/***************************************************************/
int remove_dir(char* dirnam, int fil_attr, int depth) {

    int err = 0;
    void * vp;
    char filename[FILE_MAX];
    char filepath[PATH_MAX];
    int dnamlen;
    int done;
    int eof;
    int win_fil_attr;

    if (depth > MAX_DIR_RECUR_DEPTH) {
        return (DIRERR_TOO_DEEP);
    }

    if (!(fil_attr & FATTR_IS_DIR)) {
        /* Added 6/27/2012 */
        win_fil_attr = GetFileAttributes(dirnam);
        if (win_fil_attr < 0) {
            int en = GetLastError();
            err = set_error("Directory not accessible.  Error #%d: %s\n%s",
                en, dirnam, GetWinMessage(en));
            return (err);
        } else {
            fil_attr = get_windows_fil_attr(win_fil_attr);
            if (!(fil_attr & FATTR_IS_DIR)) {
                err = set_error("%s is not a directory.");
                return (err);
            }
        }
    }

    err = open_dir(dirnam, &vp);
    if (err) return (err);

    strcpy(filepath, dirnam);
    dnamlen = strlen(filepath);
    if (dnamlen && filepath[dnamlen-1] != DIR_DELIM) {
        filepath[dnamlen++] = DIR_DELIM;
        filepath[dnamlen]   = '\0';
    }

    done = 0;
    while (!done) {
        err = read_dir(vp, filename, FILE_MAX, &eof);
        if (eof || err) {
            done = 1;
        } else if (strcmp(filename, ".") &&
                   strcmp(filename, "..")) {
            strcpy(filepath + dnamlen, filename);

            err = remove_dir_or_file(filepath, depth + 1, get_fil_attr(vp));
            if (err) {
                if (!(gflags & OPT_CONT)) done = 1;
            }
        }
    }

    if (err) {
        close_dir(vp);
    } else {
        err = close_dir(vp);
        
        if (!err) {
            err = delete_dir(dirnam, fil_attr);
        }
    }

    return (err);
}
/***************************************************************/
/*@*/int istrcmp(char * b1, char * b2) {
/*
** Case insensitive strcmp
*/
    int ii;

    for (ii = 0; b1[ii]; ii++) {
             if (tolower(b1[ii]) < tolower(b2[ii])) return (-1);
        else if (tolower(b1[ii]) > tolower(b2[ii])) return (1);
    }
    if (b2[ii]) return (-1);
    else        return (0);
}
/***************************************************************/
int process(int argc, char *argv[]) {

    int ii;
    int stat = 0;

    for (ii = 1; !stat && ii < argc; ii++) {
        if (argv[ii][0] == '-') {
            if (!istrcmp(argv[ii], "-pause")) {
                gflags |= OPT_PAUSE;
            } else if (!istrcmp(argv[ii], "-help")) {
                gflags |= OPT_HELP;
            } else if (!istrcmp(argv[ii], "-version")) {
                gflags |= OPT_VERSION;
            } else if (!istrcmp(argv[ii], "-show")) {
                gflags |= OPT_SHOW;
            } else if (!istrcmp(argv[ii], "-nodel")) {
                gflags |= OPT_NODEL;
            } else if (!istrcmp(argv[ii], "-cont")) {
                gflags |= OPT_CONT;
            } else if (!istrcmp(argv[ii], "-tryharder")) {
                gflags |= OPT_TRY_HARDER;
            } else if (!istrcmp(argv[ii], "-stats")) {
                gflags |= OPT_STATS;
            } else if (!strcmp(argv[ii], "-?")) {
                gflags |= OPT_HELP;
            } else {
                stat = set_error("Unrecognized option %s", argv[ii]);
            }
        } else {
            stat = remove_dir(argv[ii], 0, 0);
        }
    }

    if (gflags & OPT_HELP) {
        showusage(argv[0]);
    }

    return (stat);
}
/***************************************************************/
int main(int argc, char *argv[]) {

    int stat;
    int exit_code;

    exit_code = 0;

    if (argc <= 1) {
        showusage(argv[0]);
        return (0);
    }

    init();

    stat = process(argc, argv);

    if (gflags & OPT_VERSION) {
        printf("%s - %s\n", PROGRAM_NAME, VERSION);
    }

    shut();

    if (gflags & OPT_PAUSE) {
        char inbuf[100];
        printf("Press ENTER to continue...");
        gets(inbuf);
    }

    if (stat) exit_code = 1;

    return (exit_code);
}
/***************************************************************/

#endif
/***************************************************************/
void get_cwd(char *cwd, int cwdmax) {

    if (!GETCWD(cwd, cwdmax)) {
        cwd[0] = '\0';
    }
}
/***************************************************************/
void delete_relative(struct glob * gg) {

/*
**  Since we changed directory, we need to delete ./ relative items
**  from the gg->gg_filref cache.
*/
    void * vdbtc;
    struct filref ** vhand;
    struct filref * fr;

    if (!gg->gg_filref) return;

    vdbtc = dbtree_rewind(gg->gg_filref, 0);
    vhand = (struct filref **)dbtree_next(vdbtc);

    while (vhand) {
        fr = (*vhand);
        if (fr->fr_cmdpath[0] == '.' ||
            (fr->fr_cmdpath[0] == '\"' && fr->fr_cmdpath[1] == '.')) {
            /* printf("%s\n", fr->fr_cmdpath); */
            dbtree_cursor_delete(vdbtc);
            free_filref(fr);
        }
        vhand = (struct filref **)dbtree_next(vdbtc);
    }
    dbtree_close(vdbtc);
}
/***************************************************************/
int change_directory(struct cenv * ce, const char * dirname) {

    int estat;
    int ern;
    char cwd[PATH_MAXIMUM];

    estat       = 0;

    if (CHDIR(dirname)) {
        ern = ERRNO;
        estat = set_ferror(ce, ECHDIR, "%s\n    %m", dirname, ern);
    } else {
        delete_relative(ce->ce_gg); /* delete ./ relative items */ 
        if (GETCWD(cwd, PATH_MAXIMUM)) {
            estat = export_var_value(ce, "PWD", cwd);
        }
    }

    return (estat);
}
/***************************************************************/
int chndlr_cd(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix) {
/*
Syntax: (debug only)

    cd [ <dir> ]                  Change directory

        -v      Verbose - show directory name to stdout
*/
    int estat;
    int estat2;
    int pix;
    int cmdflags;
    struct parmlist pl;
    char  dirname[PATH_MAXIMUM];

    estat       = 0;
    cmdflags    = 0;
    pix         = 0;
    dirname[0]  = '\0';

    estat = parse_parmlist(ce, cmdstr, cmdix, &pl);
    if (estat) return (estat);

    estat = handle_redirs(ce, &pl);
    if (estat) return (estat);

    while (!estat && pix < pl.pl_ce->ce_argc) {
        if (!CESTRCMP(ce, pl.pl_ce->ce_argv[pix], "-v")) {
            cmdflags |= CMDFLAG_VERBOSE;
        } else {
            strmcpy(dirname, pl.pl_ce->ce_argv[pix], PATH_MAXIMUM);
        }
        pix++;            
    }

    if (!dirname[0]) {
        estat = get_home_directory(ce, dirname, PATH_MAXIMUM);
    }

    if (!estat && dirname[0]) {
        estat = change_directory(pl.pl_ce, dirname);
        if (!estat & IS_VERBOSE(ce,cmdflags)) {
            get_cwd(dirname, PATH_MAXIMUM);
            estat = printll(pl.pl_ce, "Change directory %s\n", dirname);
        }
    }

    estat2 = free_parmlist_buf(ce, &pl);
    if (!estat) estat = estat2;
    else if (estat2) estat = chain_error(ce, estat, estat2);

    return (estat);
}
/***************************************************************/
int chmod_file(struct cenv * ce,
                const char * filname,
                short chmod_mask,
                short chmod_val) {
    int estat;
    struct file_stat_rec fsr;
    int ern;
    short new_mode;
    char errmsg[128];

    estat       = 0;
    if (get_file_stat(filname, &fsr, errmsg, sizeof(errmsg))) {
        estat = set_ferror(ce, ECHMODSTAT, "%s\n    %s", filname, errmsg);
    } else {
        new_mode = ((short)fsr.fsr_mode & ~(chmod_mask)) | (chmod_mask & chmod_val);
        if (CHMOD(filname, new_mode)) {
            ern = ERRNO;
            estat = set_ferror(ce, ECHMOD, "%s\n    %m", filname, ern);
        }
    }

    return (estat);
}
/***************************************************************/
int chndlr_chmod(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix) {
/*
Syntax: (debug only)

    chmod [ <opt> [ ...] ] <file> [ <file> ... ]                 chmod
*/
    int estat;
    int estat2;
    int pix;
    int ix;
    int num_files;
    int cmdflags;
    short chmod_mask;
    short chmod_val;
    char * parm;
    struct parmlist pl;

    estat       = 0;
    cmdflags    = 0;
    pix         = 0;
    chmod_mask  = 0;
    chmod_val   = 0;
    num_files   = 0;

    estat = parse_parmlist(ce, cmdstr, cmdix, &pl);
    if (estat) return (estat);

    estat = handle_redirs(ce, &pl);
    if (estat) return (estat);

    while (!estat && pix < pl.pl_ce->ce_argc) {
        parm = pl.pl_ce->ce_argv[pix];
        if (parm[0] == '-' && parm[1]) {
            ix = 1;
            while (!estat && parm[ix]) {
                switch (parm[ix]) {
                    case 'r': case 'R':
                        chmod_mask |= FSTAT_S_IREAD;
                        break;
                    case 'w': case 'W':
                        chmod_mask |= FSTAT_S_IWRITE;
                        break;
                    case 'x': case 'X':
                        chmod_mask = FSTAT_S_IEXEC;
                        break;
                    default:
                        estat = set_ferror(ce, ECHMODBADOPT, "%c",
                                        parm[ix]);
                        break;
                }
                ix++;
            }
        } else if (parm[0] == '+' && parm[1]) {
            ix = 1;
            while (!estat && parm[ix]) {
                switch (parm[ix]) {
                    case 'r': case 'R':
                        chmod_val  |= FSTAT_S_IREAD;
                        chmod_mask |= FSTAT_S_IREAD;
                        break;
                    case 'w': case 'W':
                        chmod_val  |= FSTAT_S_IWRITE;
                        chmod_mask |= FSTAT_S_IWRITE;
                        break;
                    case 'x': case 'X':
                        chmod_val  = FSTAT_S_IEXEC;
                        chmod_mask = FSTAT_S_IEXEC;
                        break;
                    default:
                        estat = set_ferror(ce, ECHMODBADOPT, "%c",
                                        parm[ix]);
                        break;
                }
                ix++;
            }
        } else if (isdigit(parm[0]) && !chmod_mask) {
            ix = 0;
            while (!estat && parm[ix]) {
                if (chmod_val > 07777 || parm[ix] < '0' || parm[ix] > '7') {
                    estat = set_ferror(ce, ECHMODBADOPT, "%s", parm);
                } else {
                    chmod_val = (chmod_val << 3) | (parm[ix] - '0');
                }
                ix++;
            }
            chmod_mask = (short)0xFFFF;
        } else {
            estat = chmod_file(pl.pl_ce, parm, chmod_mask, chmod_val);
            if (!estat) num_files++;
        }
        pix++;            
    }

    if (!estat && !num_files) {
        estat = set_error(ce, EUNEXPEOL, parm);
    }

    estat2 = free_parmlist_buf(ce, &pl);
    if (!estat) estat = estat2;
    else if (estat2) estat = chain_error(ce, estat, estat2);

    return (estat);
}
/***************************************************************/
int chndlr_mkdir(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix) {
/*
Syntax: (debug only)

    mkdir [ -v ] [ <dir> ] [ ...]                   Make directory

        -v      Verbose - show directory name to stdout
*/
    int estat;
    int estat2;
    int pix;
    int rtn;
    int ern;
    int cmdflags;
    struct parmlist pl;

    estat       = 0;
    cmdflags    = 0;
    pix         = 0;

    estat = parse_parmlist(ce, cmdstr, cmdix, &pl);
    if (estat) return (estat);

    estat = handle_redirs(ce, &pl);
    if (estat) return (estat);

    while (!estat && pix < pl.pl_ce->ce_argc) {
        if (!CESTRCMP(ce, pl.pl_ce->ce_argv[pix], "-v")) {
            cmdflags |= CMDFLAG_VERBOSE;
        } else {
            rtn = MKDIR(pl.pl_ce->ce_argv[pix]);
            if (rtn) {
                ern = ERRNO;
                estat = set_ferror(ce, EMKDIR,"%s: %m",
                                pl.pl_ce->ce_argv[pix], ern);
            } else if (cmdflags & CMDFLAG_VERBOSE) {
                estat = printll(pl.pl_ce, "mkdir %s\n", pl.pl_ce->ce_argv[pix]);
            }
        }
        pix++;            
    }

    estat2 = free_parmlist_buf(ce, &pl);
    if (!estat) estat = estat2;
    else if (estat2) estat = chain_error(ce, estat, estat2);

    return (estat);
}
/***************************************************************/
int chndlr_rmdir(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix) {
/*
Syntax: (debug only)

    rmdir [ -v ] [ <dir> ] [ ...]                   Remove empty directory

        -v      Verbose - show directory name to stdout
*/
    int estat;
    int estat2;
    int pix;
    int rtn;
    int ern;
    int cmdflags;
    struct parmlist pl;

    estat       = 0;
    cmdflags    = 0;
    pix      = 0;
    pix         = 0;

    estat = parse_parmlist(ce, cmdstr, cmdix, &pl);
    if (estat) return (estat);

    estat = handle_redirs(ce, &pl);
    if (estat) return (estat);

    while (!estat && pix < pl.pl_ce->ce_argc) {
        if (!CESTRCMP(ce, pl.pl_ce->ce_argv[pix], "-v")) {
            cmdflags |= CMDFLAG_VERBOSE;
        } else {
            rtn = RMDIR(pl.pl_ce->ce_argv[pix]);
            if (rtn) {
                ern = ERRNO;
                estat = set_ferror(ce, ERMDIR,"%s: %m",
                                pl.pl_ce->ce_argv[pix], ern);
            } else if (cmdflags & CMDFLAG_VERBOSE) {
                estat = printll(pl.pl_ce, "rmdir %s\n", pl.pl_ce->ce_argv[pix]);
            }
        }
        pix++;            
    }

    estat2 = free_parmlist_buf(ce, &pl);
    if (!estat) estat = estat2;
    else if (estat2) estat = chain_error(ce, estat, estat2);

    return (estat);
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
int dir_show_file_debug(void * vsr, struct filattrrec * fa) {

    struct scanrec * sr = (struct scanrec *)vsr;

    if ((sr->sr_cmdflags & CMDFLAG_SHOW_ALL) || !(fa->fa_fil_attr & FATTR_HIDDEN)) {
        sr->sr_estat = printll(sr->sr_ce, "%s\n", fa->fa_filepath + sr->sr_namix);
    }

    return (sr->sr_estat);
}
/***************************************************************/
int dir_show_file(void * vsr, struct filattrrec * fa) {

    struct scanrec * sr = (struct scanrec *)vsr;
    
    if (sr->sr_cmdflags & CMDFLAG_RECURSIVE) {
        sr->sr_estat = printll(sr->sr_ce, "%s\n", fa->fa_filepath);
    } else {
        sr->sr_estat = printll(sr->sr_ce, "%s\n", fa->fa_filepath + fa->fa_name_ix);
    }

    return (sr->sr_estat);
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
int dir_debug(struct cenv * ce,
                const char * dfname,
                char ** dirpats,
                const char * pfilpat,
                int cmdflags) {
    int estat;
    struct scanrec * sr;
    void * vdi;
    int dir_opts;

    estat       = 0;
    dir_opts    = 0;
    if (cmdflags & CMDFLAG_RECURSIVE) dir_opts |= DIROPT_RECURSIVE;
    if (ce->ce_flags & CEFLAG_CASE)   dir_opts |= DIROPT_CASE;
    if (!(ce->ce_flags & CEFLAG_WIN)) dir_opts |= DIROPT_DOT_HIDES;

    sr = new_scanrec(ce, cmdflags);

    /* set sr->sr_namix */
    if (cmdflags & CMDFLAG_RECURSIVE) {
        sr->sr_namix = 0;
    } else {
        sr->sr_namix = Istrlen(dfname);
#if IS_WINDOWS
        if (sr->sr_namix == 3 && dfname[1] == ':' && isalpha(dfname[0]) && dfname[2] == DIR_DELIM) {
            sr->sr_namix = 3;
        } else
#endif
        if (!sr->sr_namix) {
            sr->sr_namix = 2;
        } else if (dfname[sr->sr_namix - 1] != DIR_DELIM) {
            sr->sr_namix += 1;
        }
    }

    vdi = scan_directory(dfname, dirpats, pfilpat, dir_opts, dir_show_file_debug, sr);
    if (dx_stat(vdi)) {
        if (dx_stat(vdi) == DIRERR_CALLBACK_ERR) {
            estat = set_error(ce, EDIR, NULL);
        } else {
            estat = set_ferror(ce, EDIR, "%s:\n    %s", 
                    dfname, dx_errmsg(vdi));
        }
    }
    dx_free(vdi);
    free_scanrec(sr);

    return (estat);
}
/***************************************************************/
int add_to_dirfiles(void * vdf, struct filattrrec * fa) {

    struct dirfiles * df = (struct dirfiles *)vdf;

    if (!df->df_dirname) {
        if (fa->fa_name_ix && fa->fa_filepath[fa->fa_name_ix-1] == DIR_DELIM) {
            df->df_dnlen = fa->fa_name_ix - 1;
            if (df->df_dnlen == 2 && has_drive_name(fa->fa_filepath)) {
                df->df_dnlen = 3;
            }
            if (!df->df_dnlen) df->df_dnlen = 1;
        } else {
            df->df_dnlen = Istrlen(fa->fa_filepath);
        }

        df->df_dirname  = New(char, df->df_dnlen + 1);
        memcpy(df->df_dirname, fa->fa_filepath, df->df_dnlen);
        df->df_dirname[df->df_dnlen] = '\0';
    }
    
    if ((df->df_cmdflags & CMDFLAG_SHOW_ALL) || !(fa->fa_fil_attr & FATTR_HIDDEN)) {
        add_dirfile(df, fa->fa_filepath + fa->fa_name_ix, fa);
    }

    return (0);
}
/***************************************************************/
static int show_fileent(struct cenv * ce,
                    struct dedirent * de,
                    struct dir_work * dw)
{
/*
CMD.EXE

 Volume in drive C has no label.
 Volume Serial Number is F00C-758B

 Directory of C:\D\Projects\desh\etc

09/01/2012  08:13 AM    <DIR>          .
09/01/2012  08:13 AM    <DIR>          ..
07/15/2012  07:09 AM             4,176 deshNotes.txt
03/17/2006  11:24 AM            19,540 elbctl.txt
06/30/2012  06:43 AM             4,387 Examples.txt
07/14/2012  07:41 AM               257 InstallDecat.bat
07/13/2012  03:48 PM               504 RenameDirs.bat
08/31/2012  05:38 PM    <DIR>          tmpdir
06/30/2012  06:43 AM             5,144 UnixShell.txt
               6 File(s)         34,008 bytes
               3 Dir(s)  18,414,673,920 bytes free

*/

    int estat;
    char dtimebuf[32];
    char sizebuf[32];
    int64 fsize;
    d_date moddate;
    int dst_flag;

    /* Haven't figured this out, but this works for now */
    dst_flag = 0;
    if (!(ce->ce_flags & CEFLAG_WIN)) dst_flag = ce->ce_gg->gg_is_dst;

    moddate = time_t_to_d_time_dst(de->de_fattr.fa_mod_date, dst_flag);
    fmt_d_date(moddate, "%m/%d/%Y %I:%M %p", dtimebuf, sizeof(dtimebuf));

    if (de->de_fattr.fa_fil_attr & FATTR_IS_DIR) {
        strcpy(sizebuf, "<DIR>         ");
    } else {
        fsize = MKINT64(de->de_fattr.fa_file_size_hi, de->de_fattr.fa_file_size_lo);
        Snprintf(sizebuf, sizeof(sizebuf), "%,14Ld", fsize);
        if (dw) {
            dw->dw_dir_fst.fst_nfiles += 1;
            dw->dw_dir_fst.fst_total  += fsize;
        }
    }

    estat = printll(ce, "%s %s %s\n", dtimebuf, sizebuf,
                de->de_fattr.fa_filepath + de->de_fattr.fa_name_ix);

    return (estat);
}
/***************************************************************/
void update_modbuf_xeq(char * modbuf,
                        const char * fname,
                        char ** pathext,
                        int npathext) {
/*
** binary_find()
*/
    int  exlen;
    int  fnlen;
    int  ix;
    int  is_xeq;
    char ext[EXT_MAXIMUM + 2];

    fnlen = Istrlen(fname);
    ix = fnlen - 1;
    while (ix >= 0 && fname[ix] != '.' && fname[ix] != DIR_DELIM) ix--;

    if (ix > 0 && fname[ix] == '.' && ix + 1 < fnlen && (fnlen - ix - 1) < EXT_MAXIMUM) {
        exlen = 0;
        ix++; /* skip dot */
        while (fname[ix]) {
            ext[exlen++] = toupper(fname[ix++]);
        }
        ext[exlen] = '\0';
        is_xeq = binary_find(ext, pathext, npathext);
        if (is_xeq) {
            if (modbuf[1] == 'r' && modbuf[3] == '-') modbuf[3] = 'x';
            if (modbuf[4] == 'r' && modbuf[6] == '-') modbuf[6] = 'x';
            if (modbuf[7] == 'r' && modbuf[9] == '-') modbuf[9] = 'x';
        }
    }
}
/***************************************************************/
static int show_ux_fileent(struct cenv * ce,
                    struct dedirent * de,
                    struct dir_work * dw)
{
/*
$ ls -l
total 1246
-rw-rw-rw-   1 whii       taurus      231527 Dec 20  2005 Makefile
-rw-rw-rw-   1 whii       taurus      231571 May 24  2005 Makefile32
drwxrwxrwx   3 whii       taurus        1024 Apr  1  2011 allsys
drwxrwxrwx   4 whii       taurus        1024 Nov 20  2007 amisys
drwxrwxrwx   3 whii       taurus        2048 Dec  3  2009 bin
drwxrwxrwx   3 whii       taurus        1024 Jan 19  2009 bld
-rw-------   1 whii       taurus        4352 Oct 11  2010 capfil
-rw-rw-r--   1 root       sys            885 May 16 10:02 cl
drwxrwxrwx  10 whii       taurus        1024 Dec 13  2011 decop

-rw-rw-rw- Administrator  None       3326444 Mar 08 10:23 argtable2-13.tar.gz
*/

    int estat;
    char timebuf[32];
    char modbuf[32];
    char uidbuf[64];
    char gidbuf[64];
    char sizgidbuf[80];
    struct tm * loctm;
    int64 fsize;
    long days_diff;
    struct file_stat_rec fsr;
    char errmsg[128];

    days_diff = (long)(ce->ce_gg->gg_time_now - (de->de_fattr.fa_mod_date)) / SECS_IN_DAY;

    loctm = localtime(&(de->de_fattr.fa_mod_date));

    if (days_diff > 183) {
        strftime(timebuf, sizeof(timebuf), "%b %d  %Y", loctm);
    } else {
        strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", loctm);
    }

    if (get_file_stat(de->de_fattr.fa_filepath, &fsr, errmsg, sizeof(errmsg))) {
        strcpy(modbuf, "----------");
    } else {
        get_modbuf(fsr.fsr_mode, modbuf);

        if ((ce->ce_flags & CEFLAG_WIN) && dw->dw_npath_ext) {
            update_modbuf_xeq(modbuf,
                    de->de_fattr.fa_filepath + de->de_fattr.fa_name_ix,
                    dw->dw_path_ext_list,
                    dw->dw_npath_ext);
        }

        if (get_file_owner(de->de_fattr.fa_filepath, fsr.fsr_uid,
                    uidbuf, sizeof(uidbuf),
                    gidbuf, sizeof(gidbuf),
                    errmsg, sizeof(errmsg))) {
            uidbuf[0] = 0;
            gidbuf[0] = 0;
        }
    }

    if (de->de_fattr.fa_fil_attr & FATTR_IS_DIR) {
        Snprintf(sizgidbuf, sizeof(sizgidbuf), "%-18s", gidbuf);
    } else {
        fsize = MKINT64(de->de_fattr.fa_file_size_hi, de->de_fattr.fa_file_size_lo);
        if (fsize < 1000) {
            Snprintf(sizgidbuf, sizeof(sizgidbuf), "%-14s %3Ld", gidbuf, fsize);
        } else {
            Snprintf(sizgidbuf, sizeof(sizgidbuf), "%-8s %9Ld", gidbuf, fsize);
        }
    }

    estat = printll(ce, "%s %-14s %s %s %s\n", modbuf, uidbuf, sizgidbuf, timebuf,
                de->de_fattr.fa_filepath + de->de_fattr.fa_name_ix);

    return (estat);
}
struct sort_info {
    int sin_casein;
    int sin_grp_dirs;
};
/***************************************************************/
static int cmp_dirent_name(struct sort_info * sin, struct dedirent * de1, struct dedirent * de2) {

    int cmp;

    if (sin->sin_casein) {
        cmp   = Stricmp(de1->de_fattr.fa_filepath + de1->de_fattr.fa_name_ix,
                        de2->de_fattr.fa_filepath + de2->de_fattr.fa_name_ix);
    } else {
        cmp   = strcmp(de1->de_fattr.fa_filepath + de1->de_fattr.fa_name_ix,
                       de2->de_fattr.fa_filepath + de2->de_fattr.fa_name_ix);
    }

    if (sin->sin_grp_dirs) {
        if (!(de1->de_fattr.fa_fil_attr & FATTR_IS_DIR)) {
            if (de2->de_fattr.fa_fil_attr & FATTR_IS_DIR) {
                cmp = 1;
            }
        }
    }

    return (cmp);
}
/***************************************************************/
static int cmp_dirent_size(struct sort_info * sin, struct dedirent * de1, struct dedirent * de2) {

    int cmp;
    int64 fsize1;
    int64 fsize2;

    fsize1 = MKINT64(de1->de_fattr.fa_file_size_hi, de1->de_fattr.fa_file_size_lo);
    fsize2 = MKINT64(de2->de_fattr.fa_file_size_hi, de2->de_fattr.fa_file_size_lo);

    if (sin->sin_grp_dirs) {
        if ((de1->de_fattr.fa_fil_attr & FATTR_IS_DIR) ==
            (de2->de_fattr.fa_fil_attr & FATTR_IS_DIR)) {
            if (fsize1 == fsize2) {
                cmp = cmp_dirent_name(sin, de1, de2);
            } else {
                if (fsize1 > fsize2) {
                    cmp = 1;
                } else {
                    cmp = -1;
                }
            }
        } else {
            if (de1->de_fattr.fa_fil_attr & FATTR_IS_DIR) {
                cmp = -1;
            } else {
                cmp = 1;
            }
        }
    } else {
        if (fsize1 == fsize2) {
            cmp = cmp_dirent_name(sin, de1, de2);
        } else {
            if (fsize1 > fsize2) {
                cmp = 1;
            } else {
                cmp = -1;
            }
        }
    }

    return (cmp);
}
/***************************************************************/
static int cmp_dirent_time(struct sort_info * sin, struct dedirent * de1, struct dedirent * de2) {

    int cmp;

    if (sin->sin_grp_dirs) {
        if ((de1->de_fattr.fa_fil_attr & FATTR_IS_DIR) ==
            (de2->de_fattr.fa_fil_attr & FATTR_IS_DIR)) {
            if (de1->de_fattr.fa_mod_date == de2->de_fattr.fa_mod_date) {
                cmp = cmp_dirent_name(sin, de1, de2);
            } else {
                if (de1->de_fattr.fa_mod_date > de2->de_fattr.fa_mod_date) {
                    cmp = 1;
                } else {
                    cmp = -1;
                }
            }
        } else {
            if (de1->de_fattr.fa_fil_attr & FATTR_IS_DIR) {
                cmp = -1;
            } else {
                cmp = 1;
            }
        }
    } else {
        if (de1->de_fattr.fa_mod_date == de2->de_fattr.fa_mod_date) {
            cmp = cmp_dirent_name(sin, de1, de2);
        } else {
            if (de1->de_fattr.fa_mod_date > de2->de_fattr.fa_mod_date) {
                cmp = 1;
            } else {
                cmp = -1;
            }
        }
    }

    return (cmp);
}
/***************************************************************/
void qsdirents(struct sort_info * sin,
            struct dedirent ** y,
            int l,
            int r,
            int (*de_cmp)(struct sort_info * sin,
                            struct dedirent * de1,
                            struct dedirent * de2))
{
/*
** Quick sort of symbol struct:  y - data
**
** to call:  qssyms(y, 0, nyms - 1);
*/
    int i;
    int j;
    struct dedirent * v;
    struct dedirent * u;

    if (r > l) {
        v = y[r]; i = l - 1; j = r;
        for (;;) {
            while (i < j && de_cmp(sin, y[++i], v) < 0) ; /* use < 0 for asc, > 0 for desc */
            while (i < j && de_cmp(sin, y[--j], v) > 0) ; /* use > 0 for asc, < 0 for desc */
            if (i >= j) break;
            u = y[i]; y[i] = y[j]; y[j] = u;
        }
        u = y[i]; y[i] = y[r]; y[r] = u;
        qsdirents(sin, y, l, i-1, de_cmp);
        qsdirents(sin, y, i+1, r, de_cmp);
    }
}
/***************************************************************/
static void sort_dirfiles(struct cenv * ce,
                int dir_opts,
                int cmdflags,
                struct dirfiles * df)
{
    struct sort_info sin;
    struct dedirent * de0;
    int ii;
    int jj;

    sin.sin_casein   = 0;
    sin.sin_grp_dirs = 0;

    if (ce->ce_flags & CEFLAG_CASE)         sin.sin_casein   = 1;
    if (cmdflags & CMDFLAG_GROUP_DIRS)      sin.sin_grp_dirs = 1;

           if (cmdflags & CMDFLAG_SORT_TIME) {
        qsdirents(&sin, df->df_dirent, 0, df->df_nents - 1, cmp_dirent_time);
    } else if (cmdflags & CMDFLAG_SORT_SIZE) {
        qsdirents(&sin, df->df_dirent, 0, df->df_nents - 1, cmp_dirent_size);
    } else {
        qsdirents(&sin, df->df_dirent, 0, df->df_nents - 1, cmp_dirent_name);
    }

    if (cmdflags & CMDFLAG_DESC_SORT) {
        jj = df->df_nents - 1;
        for (ii = 0; ii < jj; ii++) {
            de0 = df->df_dirent[ii];
            df->df_dirent[ii] = df->df_dirent[jj];            
            df->df_dirent[jj] = de0;            
            jj--;
        }
    }
}
/***************************************************************/
static int show_dirfile_sum(struct cenv * ce,
                struct dir_work * dw)
{
    int estat;

    estat = printll(ce, "   %,9d File(s) %,14Ld bytes\n",
            dw->dw_dir_fst.fst_nfiles, dw->dw_dir_fst.fst_total);
    if (!estat) estat = printll(ce, "\n");

    dw->dw_tot_fst.fst_ndirs  += dw->dw_dir_fst.fst_ndirs;  /* should be 1 */
    dw->dw_tot_fst.fst_nfiles += dw->dw_dir_fst.fst_nfiles;
    dw->dw_tot_fst.fst_total  += dw->dw_dir_fst.fst_total;

    dw->dw_dir_fst.fst_ndirs  = 0;
    dw->dw_dir_fst.fst_nfiles = 0;
    dw->dw_dir_fst.fst_total  = 0;

    return (estat);
}
/***************************************************************/
int dir_full(struct cenv * ce,
                const char * dfname,
                char ** dirpats,
                const char * pfilpat,
                int cmdflags,
                struct dir_work * dw);

static int show_dirfiles(struct cenv * ce,
                const char * pfilpat,
                int dir_opts,
                int cmdflags,
                struct dirfiles * df,
                int dir_name_ix,
                struct dir_work * dw)
{
    int estat;
    int ii;
    int do_show;
    char * fname;
    char  filepath[PATH_MAXIMUM];

    estat = 0;
    do_show = 0;

    if (cmdflags & CMDFLAG_BARE) {
        memcpy(filepath, df->df_dirname, df->df_dnlen);
        filepath[df->df_dnlen] = DIR_DELIM;
        if (!df->df_nents) {
            do_show = 1;
        }
    }

    for (ii = 0; !do_show && ii < df->df_nents; ii++) {
        do_show = patmatches((dir_opts & DIROPT_CASE)?1:0, pfilpat,
                    df->df_dirent[ii]->de_fattr.fa_filepath + df->df_dirent[ii]->de_fattr.fa_name_ix);
    }

    if (do_show) {
        sort_dirfiles(ce, dir_opts, cmdflags, df);

        if (cmdflags & CMDFLAG_UX_STYLE) {
            estat = printll(ce, "%s\n", df->df_dirname);
        } else if (cmdflags & CMDFLAG_BARE) {
            if (dir_name_ix < df->df_dnlen) {
                estat = printll(ce, "%s\n",  df->df_dirname + dir_name_ix);
            }
        } else {
            if (strcmp(df->df_dirname, dw->dw_dir_fst.fst_dir_name)) {

                if (dw->dw_dir_fst.fst_ndirs) {
                    estat = show_dirfile_sum(ce, dw);
                }

                if (!estat) estat = printll(ce, " Directory of %s\n", df->df_dirname);
                if (!estat) estat = printll(ce, "\n");

                strcpy(dw->dw_dir_fst.fst_dir_name, df->df_dirname);
                dw->dw_dir_fst.fst_ndirs += 1;
            }
        }

        for (ii = 0; !estat && ii < df->df_nents; ii++) {
            do_show = patmatches((dir_opts & DIROPT_CASE)?1:0, pfilpat,
                    df->df_dirent[ii]->de_fattr.fa_filepath + df->df_dirent[ii]->de_fattr.fa_name_ix);
            if (do_show) {
                if (cmdflags & CMDFLAG_UX_STYLE) {
                    estat = show_ux_fileent(ce, df->df_dirent[ii], dw);
                } else if (cmdflags & CMDFLAG_BARE) {
                    if ((df->df_dirent[ii]->de_fattr.fa_fil_attr & FATTR_IS_DIR) &&
                        (cmdflags & CMDFLAG_RECURSIVE)) {
                        fname = df->df_dirent[ii]->de_fattr.fa_filepath + df->df_dirent[ii]->de_fattr.fa_name_ix;

                        if (strcmp(fname, ".") && strcmp(fname, "..")) {
                            strcpy(filepath + df->df_dnlen + 1, fname);
                            estat = dir_full(ce, filepath, NULL, pfilpat, cmdflags, dw);
                        }
                    } else {
                        estat = printll(ce, "%s\n",  df->df_dirent[ii]->de_fattr.fa_filepath + dir_name_ix);
                    }
                } else {
                    estat = show_fileent(ce, df->df_dirent[ii], dw);
                }
            }
        }
    }

    return (estat);
}
/***************************************************************/
int dir_full_dirs(struct cenv * ce,
                const char * dfname,
                char ** dirpats,
                const char * pfilpat,
                int cmdflags,
                struct dirfiles * df,
                struct dir_work * dw) {

    int estat;
    int ii;
    char * fname;
    char  filepath[PATH_MAXIMUM];

    estat       = 0;
    memcpy(filepath, df->df_dirname, df->df_dnlen);
    filepath[df->df_dnlen] = DIR_DELIM;

    for (ii = 0; !estat && ii < df->df_nents; ii++) {
        if (df->df_dirent[ii]->de_fattr.fa_fil_attr & FATTR_IS_DIR) {
            fname = df->df_dirent[ii]->de_fattr.fa_filepath +
                df->df_dirent[ii]->de_fattr.fa_name_ix;

            if (strcmp(fname, ".") && strcmp(fname, "..")) {
                strcpy(filepath + df->df_dnlen + 1, fname);
                estat = dir_full(ce, filepath, dirpats, pfilpat, cmdflags, dw);
            }
        }
    }

    return (estat);
}
/***************************************************************/
int dir_full(struct cenv * ce,
                const char * dfname,
                char ** dirpats,
                const char * pfilpat,
                int cmdflags,
                struct dir_work * dw) {
    int estat;
    int dir_opts;
    int dir_name_ix;
    struct dirfiles * df;
    void * vdi;

    estat       = 0;

    df = new_dirfiles(cmdflags);

    dir_opts = DIROPT_ONE_LEVEL;
    if (cmdflags & CMDFLAG_RECURSIVE)    dir_opts |= DIROPT_ALL_DIRS;
    else if (!(cmdflags & CMDFLAG_BARE)) dir_opts |= DIROPT_DOT_DOTDOT;
    if (ce->ce_flags & CEFLAG_CASE)      dir_opts |= DIROPT_CASE;
    if (!(ce->ce_flags & CEFLAG_WIN))    dir_opts |= DIROPT_DOT_HIDES;

    /* set dir_name_ix */
    if (cmdflags & CMDFLAG_RECURSIVE) {
        dir_name_ix = 0;
    } else {
        dir_name_ix = Istrlen(dfname);
        if (!dir_name_ix) {
            dir_name_ix = 2;
        } else if (dfname[dir_name_ix - 1] != DIR_DELIM) {
            dir_name_ix += 1;
        }
    }

    vdi = scan_directory(dfname, dirpats, pfilpat, dir_opts, add_to_dirfiles, df);
    if (dx_stat(vdi)) {
        estat = set_ferror(ce, EDIR, "%s:\n    %s", 
                    dfname, dx_errmsg(vdi));
    }
    dx_free(vdi);

    if (!estat) {
#ifdef OLD_WAY
        if (cmdflags & CMDFLAG_BARE) estat = show_all_dirfiles(ce, df);
        else                          estat = show_dirfiles(ce, pfilpat, dir_opts,
                                                            cmdflags, df, dw);
#endif

        if (!df->df_dirname) {      /* Old way: df->df_dirname = Strdup(dfname) */
            df->df_dnlen = Istrlen(dfname);
            df->df_dirname  = New(char, df->df_dnlen + 1);
            memcpy(df->df_dirname, dfname, df->df_dnlen + 1);
        }

        estat = show_dirfiles(ce, pfilpat, dir_opts, cmdflags, df, dir_name_ix, dw);
        if (!estat && !(cmdflags & CMDFLAG_BARE)) {
            if (dirpats && dirpats[0]) {
                estat = dir_full_dirs(ce, dfname, dirpats + 1, pfilpat, cmdflags, df, dw);
            } else if (cmdflags & CMDFLAG_RECURSIVE) {
                estat = dir_full_dirs(ce, dfname, NULL, pfilpat, cmdflags, df, dw);
            }
        }
    }

    free_dirfiles(df);

    return (estat);
}
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
        len = Istrlen(name);
        if (len >= fullmax) return (0);
        strcpy(fullname, name);
        return (fullname);
    }

    len = Istrlen(cwd);
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

    fnlen = Istrlen(fullname);
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
        ii = Istrlen(filpat);
        if (ii >= 3 && !memcmp(filpat + ii - 3, "*.*", 3)) {
            filpat[ii - 2] = '\0'; /* remove trailing .* */
        }
    }

    return (badfs);
}
/***************************************************************/
void set_time_globals(struct glob * gg) {

    gg->gg_time_now = time(NULL);
    get_timezone_info(&(gg->gg_timezone), &(gg->gg_is_dst));
}
/***************************************************************/
int binary_find(const char * srchstr, char ** srclist, int nlist) {

    int lo;
    int hi;
    int mid;

    if (!srclist || !srchstr[0]) return (0);

    lo = 0;
    hi = nlist - 1;
    while (lo <= hi) {
        mid = (lo + hi) / 2;
        if (strcmp(srchstr, srclist[mid]) <= 0) hi = mid - 1;
        else                                    lo = mid + 1;
    }

    if (lo < nlist && !strcmp(srchstr, srclist[lo])) mid = lo + 1;
    else mid = 0;

    return (mid);
}
/***************************************************************/
void qsstrlist(char ** y, int l, int r)
{
/*
** Quick sort of symbol struct:  y - data
**
** to call:  qssyms(y, 0, nyms - 1);
*/
    int i;
    int j;
    char * v;
    char * u;

    if (r > l) {
        v = y[r]; i = l - 1; j = r;
        for (;;) {
            while (i < j && strcmp(y[++i], v) < 0) ; /* use < 0 for asc, > 0 for desc */
            while (i < j && strcmp(y[--j], v) > 0) ; /* use > 0 for asc, < 0 for desc */
            if (i >= j) break;
            u = y[i]; y[i] = y[j]; y[j] = u;
        }
        u = y[i]; y[i] = y[r]; y[r] = u;
        qsstrlist(y, l, i-1);
        qsstrlist(y, i+1, r);
    }
}
/***************************************************************/
void set_dw_path_ext(struct cenv * ce, struct dir_work * dw) {

    char * pathext;
    int     peix;
    int     exlen;
    int     mxnpath;
    char ext[EXT_MAXIMUM + 2];

    pathext = find_globalvar(ce->ce_gg, "PATHEXT", 7);
    if (pathext) {
        peix = 0;
        mxnpath = 0;
        while (pathext[peix]) {
            if (pathext[peix] == ce->ce_gg->gg_path_delim) peix++;
            if (pathext[peix] != '.') {
                /* no leading '.', skip ext until ; */
                while (pathext[peix] && pathext[peix] != ce->ce_gg->gg_path_delim) {
                    peix++;
                }
            } else {
                exlen = 0;
                peix++; /* skip '.' */           
                while (pathext[peix] && pathext[peix] != ce->ce_gg->gg_path_delim) {
                    if (exlen < EXT_MAXIMUM) ext[exlen++] = toupper(pathext[peix]);
                    peix++;
                }

                if (exlen) {
                    ext[exlen] = '\0';
                    if (dw->dw_npath_ext == mxnpath) {
                        if (!mxnpath) mxnpath = 8;
                        else          mxnpath *= 2;
                        dw->dw_path_ext_list =
                            Realloc(dw->dw_path_ext_list, char*, mxnpath);
                    }
                    dw->dw_path_ext_list[dw->dw_npath_ext] = New(char, exlen + 1);
                    memcpy(dw->dw_path_ext_list[dw->dw_npath_ext], ext, exlen + 1);
                    dw->dw_npath_ext += 1;
                }
            }
        }
    }

    qsstrlist(dw->dw_path_ext_list, 0, dw->dw_npath_ext - 1);

}
/***************************************************************/
void free_dw_path_ext(struct dir_work * dw) {

    int ii;

    for (ii = 0; ii < dw->dw_npath_ext; ii++) {
        Free(dw->dw_path_ext_list[ii]);
    }
    Free(dw->dw_path_ext_list);
}
/***************************************************************/
int dir_cmd(struct cenv * ce,
                const char * dfname,
                int cmdflags,
                struct dir_work * dw) {

    int estat;
    int swflags;
    char ** dirpats;
    char dirnam[PATH_MAXIMUM];
    char fdirnam[PATH_MAXIMUM];
    char filpat[FILE_MAXIMUM];

    estat       = 0;
    swflags     = 0;
    set_time_globals(ce->ce_gg);
    if (ce->ce_flags & CEFLAG_WIN) {
        swflags  |= SWFLAG_DOT_STAR;
        dw->dw_npath_ext     = 0;
        dw->dw_path_ext_list = NULL;
        if (cmdflags & CMDFLAG_UX_STYLE) {
            set_dw_path_ext(ce, dw);
        }
    }

    if (splitwild(dfname, dirnam, &dirpats, filpat, swflags)) {
        estat = set_ferror(ce, EFILSET, "%s", dfname);
        return (estat);
    }

    if (!estat) {
        if (cmdflags & CMDFLAG_DEBUG) {
            estat = dir_debug(ce, dirnam, dirpats, filpat, cmdflags);
        } else if (cmdflags & CMDFLAG_BARE) {
            strmcpy(fdirnam, dirnam, PATH_MAXIMUM);
            if (!fdirnam[0]) strcpy(fdirnam, ".");
            estat = dir_full(ce, fdirnam, dirpats, filpat, cmdflags, dw);
        } else {
            fullyqualify(fdirnam, dirnam, PATH_MAXIMUM);
            estat = dir_full(ce, fdirnam, dirpats, filpat, cmdflags, dw);
        }
    }
    free_charlist(dirpats);

    return (estat);
}
/***************************************************************/
int chndlr_dir(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix) {
/*
Syntax:

    dir [ -<opt> [...] ] [ <dir> ] [ ...]          List files/directories

        <opt>   
        a or A  All files.  Show hidden files too.
        b or B  Bare.  Show only file names.
        d or D  Descending sort.  Show items in reverse order.
        G or G  Group directories.  Show directories first.
        r or R  Recursive.  Show nested directories too.
        s or S  Recursive, same as r(R)
        t or T  Time sort.  Show files in modification date order.
        x or X  UniX style.  Show files in format similar to Unix long style.
        z or Z  SiZe sort.  Show files in order of size.

    These should be exactly the same:

        1.  dir -b <fileset>

        2.  shopt -s f; for var in `echo <fileset>`; echo $var; endfor 

        3.  for var in $^{readdir("<fileset>")}; echo $var; endfor
*/
    int estat;
    int estat2;
    int pix;
    int ix;
    int did_dir;
    int cmdflags;
    char * parm;
    struct parmlist pl;
    int dir_opts;
    struct dir_work dwr;

    estat       = 0;
    cmdflags    = 0;
    dir_opts    = 0;
    pix         = 0;
    did_dir     = 0;
    memset(&dwr, 0, sizeof(struct dir_work));

    estat = parse_parmlist(ce, cmdstr, cmdix, &pl);
    if (estat) return (estat);

    estat = handle_redirs(ce, &pl);
    if (estat) return (estat);

    while (!estat && pix < pl.pl_ce->ce_argc) {
        parm = pl.pl_ce->ce_argv[pix];
        if (parm[0] == '-' && parm[1]) {
            ix = 0;
            while (!estat && parm[ix]) {
                switch (parm[ix]) {
                    case '-':
                        break;
                    case 'a': case 'A':
                        cmdflags |= CMDFLAG_SHOW_ALL;
                        break;
                    case 'b': case 'B':
                        cmdflags |= CMDFLAG_BARE;
                        break;
                    case 'd': case 'D':
                        cmdflags |= CMDFLAG_DESC_SORT;
                        break;
#if IS_DEBUG
                    case 'e': case 'E':
                        cmdflags |= CMDFLAG_DEBUG;
                        break;
#endif
                    case 'g': case 'G':
                        cmdflags |= CMDFLAG_GROUP_DIRS;
                        break;
                    case 'r': case 'R':
                    case 's': case 'S':
                        cmdflags |= CMDFLAG_RECURSIVE;
                        break;
                    case 't': case 'T':
                        cmdflags |= CMDFLAG_SORT_TIME;
                        break;
                    case 'x': case 'X':
                        cmdflags |= CMDFLAG_UX_STYLE;
                        break;
                    case 'z': case 'Z':
                        cmdflags |= CMDFLAG_SORT_SIZE;
                        break;
                    default:
                        estat = set_ferror(ce, EBADOPT, "%c",
                                        parm[ix]);
                        break;
                }
                ix++;
            }
        } else {
            did_dir++;
            estat = dir_cmd(pl.pl_ce, parm, cmdflags, &dwr);
        }
        pix++;            
    }

    if (!estat && !did_dir) {
        did_dir++;
        estat = dir_cmd(pl.pl_ce, ".", cmdflags, &dwr);
    }

    if (!estat && dwr.dw_dir_fst.fst_ndirs && !(cmdflags & (CMDFLAG_UX_STYLE | CMDFLAG_DEBUG))) {
        estat = show_dirfile_sum(pl.pl_ce, &dwr);
    }

    if (!estat && dwr.dw_tot_fst.fst_ndirs > 1 && !(cmdflags & (CMDFLAG_UX_STYLE | CMDFLAG_DEBUG))) {
        estat = printll(pl.pl_ce, "     Total Files Listed:\n");
        if (!estat) estat = printll(pl.pl_ce, "   %,9d File(s) %,14Ld bytes\n",
                dwr.dw_tot_fst.fst_nfiles, dwr.dw_tot_fst.fst_total);
    }

    if (ce->ce_flags & CEFLAG_WIN) {
        free_dw_path_ext(&dwr);
    }

    estat2 = free_parmlist_buf(ce, &pl);
    if (!estat) estat = estat2;
    else if (estat2) estat = chain_error(ce, estat, estat2);

    return (estat);
}
/***************************************************************/
#if IS_WINDOWS
int win_copy_file_to_file(struct cenv * ce,
                int cmdflags,
                const char * src,
                const char * tgt,
                struct stat * srcstbuf,
                struct stat * tgtstbuf) {
/*
*/
    int estat;
    int keepold;
    int rtn;

    estat       = 0;

    keepold     = 1;
    if (cmdflags & CMDFLAG_YES) keepold = 0;

    rtn = CopyFile(src, tgt, keepold);
    if (!rtn) {
        int ern = GETERRNO;
        estat = set_ferror(ce, ECOPYERR, "%s --> %s\n%M", src, tgt, ern);
    } else if ((ce->ce_flags & CEFLAG_VERBOSE) || (cmdflags & CMDFLAG_VERBOSE)) {
        estat = printlle(ce, "Copied %s --> %s\n", src, tgt);
    }

    return (estat);
}
#endif
/***************************************************************/
int win_fchown(int fd, int owner, int group) {

    return (-1);
}
/***************************************************************/
/*@*/int ux_set_attr(const char * fname,
                        int fmode,
                        char   * errmsg,
                        int      errmsgmax) {

    int err = 0;
    int ern;

    if (CHMOD(fname, fmode)) {
        ern = ERRNO;
        strmcpy(errmsg, strerror(ern), errmsgmax);
        err = -1;
    }

    return (err);
}
/***************************************************************/
/*@*/int ux_set_read_write(const char * fname,
                    char   * errmsg,
                    int      errmsgmax) {

    int err = 0;
    int ern;
    int fmode;

    fmode = FSTAT_S_IREAD | FSTAT_S_IWRITE;
    if (CHMOD(fname, fmode)) {
        ern = ERRNO;
        strmcpy(errmsg, strerror(ern), errmsgmax);
        err = -1;
    }

    return (err);
}
/***************************************************************/
int ux_rm_file(const char * filnam,
                    char   * errmsg,
                    int      errmsgmax) {

	int err = 0;
    int ern;
    int stat;

    stat = REMOVE(filnam);
    if (stat) {
        ern = ERRNO;
        if (ern == EACCES) {
            err = ux_set_read_write(filnam, errmsg, errmsgmax);
            if (!err) {
                /* Try again */
                stat = REMOVE(filnam);
                if (stat) {
                    ern = ERRNO;
                    strmcpy(errmsg, strerror(ern), errmsgmax);
                    err = -1;
                }
            }
        } else {
            strmcpy(errmsg, strerror(ern), errmsgmax);
            err = -1;
        }
    }

    return (err);
}
/***************************************************************/
int ux_copy_file(   const char * src,
                    const char * tgt,
                    int      cpflags,
                    struct stat * srcstbuf,
                    char   * errmsg,
                    int      errmsgmax) {
/*
** Returns:
**      0  - Success
**      201 - Error opening source
**      202 - Error opening target
**      203 - Error reading from source
**      204 - Error writing to target
**      205 - Error closing source
**      206 - Error closing target
**      207 - Error changing target times
**      208 - Error changing target ownership
**      209 - Error changing target mode
**      210 - Error allocating buffer
**      211 - User pressed Ctl-C
*/

    int err = 0;
    int srcfnum;
    int tgtfnum;
    int ern = 0;
    int done;
    int nread;
    int nwrot;
    int bufix;
    size_t bufsize;
    unsigned char * buf;
    struct utimbuf srctimes;
    int tgtmode;

    srcfnum = OPEN(src, _O_RDONLY | _O_BINARY);
    if (srcfnum == -1) {
        ern = ERRNO;
        strmcpy(errmsg, strerror(ern), errmsgmax);
        err = 201;
    } else {
        tgtmode = FSTAT_S_IFREG | FSTAT_S_IREAD | FSTAT_S_IWRITE;
        tgtfnum = OPEN3(tgt, _O_WRONLY | _O_CREAT | _O_BINARY, tgtmode);
        if (tgtfnum == -1) {
            ern = ERRNO;
            strmcpy(errmsg, strerror(ern), errmsgmax);
            err = 202;
        } else {
            bufsize = COPY_FBUF_SIZE;
            buf = malloc(bufsize);
            if (!buf) {
                ern = ERRNO;
                strmcpy(errmsg, strerror(ern), errmsgmax);
                err = 210;
            } else {
                done = 0;
                while (!done) {
                    if (global_user_break) {
                        err = 211;
                        nread = 0;
                    } else {
                        nread = (int)READ(srcfnum, buf, (unsigned int)bufsize);
                    }
                    if (nread == 0) {
                        done = 1;
                    } else if (nread < 0) {
                        ern = ERRNO;
                        strmcpy(errmsg, strerror(ern), errmsgmax);
                        err = 203;
                    } else {
                        bufix = 0;
                        while (bufix < nread) {
                            nwrot = (int)WRITE(tgtfnum, buf + bufix, nread - bufix);
                            if (nwrot <= 0) {
                                ern = ERRNO;
                                strmcpy(errmsg, strerror(ern), errmsgmax);
                                err = 204;
                                bufix = nread;  /* stop writing */
                                done = 1;       /* stop reading too */
                            } else {
                                bufix += nwrot;
                            }
                        }
                    }
                }
                free(buf);
            }

            if (!err && (cpflags & CPFLAG_CHOWN)) {
                if (FCHOWN(tgtfnum, srcstbuf->st_uid, srcstbuf->st_gid)) {
                    ern = ERRNO;
                    strmcpy(errmsg, strerror(ern), errmsgmax);
                    err = 208;
                }
            }

            if (CLOSE(tgtfnum)) {
                if (!err) {
                    ern = ERRNO;
                    strmcpy(errmsg, strerror(ern), errmsgmax);
                    err = 206;
                }
            }

            if (!err) {
                srctimes.actime  = srcstbuf->st_atime;
                srctimes.modtime = srcstbuf->st_mtime;
                if (utime(tgt, &srctimes)) {
                    ern = ERRNO;
                    strmcpy(errmsg, strerror(ern), errmsgmax);
                    err = 207;
                }
            }

            if (!err && (srcstbuf->st_mode != tgtmode)) {
                if (ux_set_attr(tgt, srcstbuf->st_mode, errmsg, errmsgmax)) {
                    err = 209;
                }
            }
        }
        if (CLOSE(srcfnum)) {
            if (!err) {
                ern = ERRNO;
                strmcpy(errmsg, strerror(ern), errmsgmax);
                err = 205;
            }
        }
    }

    return (err);
}
/***************************************************************/
int ux_copy_file_to_file(struct cenv * ce,
                int cmdflags,
                const char * src,
                const char * tgt,
                struct stat * srcstbuf,
                struct stat * tgtstbuf) {
    int estat;
    int rtn;
    int cpflags;
    char errmsg[128];

    estat       = 0;

    if (tgtstbuf) {
        if (!(cmdflags & CMDFLAG_YES)) {
            estat = set_error(ce, ETARGETEXISTS, tgt);
        } else {
            rtn = ux_rm_file(tgt, errmsg, sizeof(errmsg));
            if (rtn) {
                estat = set_error(ce, ETARGETEXISTS, errmsg);
            }
        }
    }

    if (!estat) {
        cpflags = 0;
        rtn = ux_copy_file(src, tgt, cpflags, srcstbuf, errmsg, sizeof(errmsg));
        if (rtn) {
            estat = set_ferror(ce, ECPFILE, "%s --> %s\n%s", src, tgt, errmsg);
        } else if ((ce->ce_flags & CEFLAG_VERBOSE) || (cmdflags & CMDFLAG_VERBOSE)) {
            estat = printlle(ce, "Copied %s --> %s\n", src, tgt);
        }
    }

    return (estat);
}
/***************************************************************/
int copy_file_to_file(struct cenv * ce,
                      int cmdflags,
                      const char * src,
                      const char * tgt,
                      struct stat * srcstbuf,
                      struct stat * tgtstbuf) {
    int estat;

#if IS_WINDOWS   
    estat = win_copy_file_to_file(ce, cmdflags, src, tgt, srcstbuf, tgtstbuf);
#else
    estat = ux_copy_file_to_file(ce, cmdflags, src, tgt, srcstbuf, tgtstbuf);
#endif
    
    return (estat);
}
/***************************************************************/
/*@*/int get_file_stbuf(const char * fname, struct stat *pstbuf) {

    int result = 0;

    if (STAT(fname, pstbuf)) {
        result = -1;
    }

    return (result);
}
/***************************************************************/
int copy_file_to_dir(struct cenv * ce,
                int cmdflags,
                const char * src,
                const char * tgt,
                struct stat * srcstbuf) {
/*
*/
    int estat;
    int tgtft;
    struct stat tgtstbuf;
    char dirnam[PATH_MAXIMUM];
    char tgtfil[PATH_MAXIMUM];
    char filnam[FILE_MAXIMUM];

    os_splitpath(src, dirnam, filnam, FILE_MAXIMUM);
    os_makepath(tgt, filnam, tgtfil, PATH_MAXIMUM, DIR_DELIM);

    tgtft = get_file_stbuf(tgtfil, &tgtstbuf);
    if (tgtft) {
        estat = copy_file_to_file(ce, cmdflags, src, tgtfil, srcstbuf, NULL);
    } else {
        estat = copy_file_to_file(ce, cmdflags, src, tgtfil, srcstbuf, &tgtstbuf);
    }

    return (estat);
}
/***************************************************************/
int copy_file(struct cenv * ce,
                int cmdflags,
                const char * src,
                const char * tgt) {
/*
*/
    int estat;
    int srcft;
    int tgtft;
    struct stat srcstbuf;
    struct stat tgtstbuf;

    estat       = 0;

    srcft = get_file_stbuf(src, &srcstbuf);
    if (srcft) {
        estat = set_error(ce, ENOCOPYSRC, src);
    } else if (F_S_ISDIR(srcstbuf.st_mode)) {
        estat = set_error(ce, ECOPYSRCDIR, src);
    } else {
        tgtft = get_file_stbuf(tgt, &tgtstbuf);
        if (tgtft) {
            estat = copy_file_to_file(ce, cmdflags, src, tgt, &srcstbuf, NULL);
        } else {
            if (F_S_ISDIR(tgtstbuf.st_mode)) {
                estat = copy_file_to_dir(ce, cmdflags, src, tgt, &srcstbuf);
            } else if (cmdflags & CMDFLAG_DIR_REQ) {
                estat = set_error(ce, EDIRREQ, tgt);
            } else if (!(cmdflags & CMDFLAG_YES)) {
                estat = set_error(ce, ECOPYTGTEXISTS, tgt);
            } else {
                estat = copy_file_to_file(ce, cmdflags, src, tgt, &srcstbuf, &tgtstbuf);
            }
        }
    }

    return (estat);
}
/***************************************************************/
int chndlr_copy(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix) {
/*
Syntax:

    copy [ -<opt> ] [ <dir> ] [ ...]          List files/directories

        <opt>   
        v or V  Verbose.  Show source and target.
        y or Y  Yes.  Deleting existing target, if it exists.
*/
    int estat;
    int estat2;
    int pix;
    int ix;
    int cmdflags;
    int ncopied;
    char * parm;
    struct parmlist pl;

    estat       = 0;
    cmdflags    = 0;
    pix         = 0;
    ncopied     = 0;
    parm        = "";

    estat = parse_parmlist(ce, cmdstr, cmdix, &pl);
    if (estat) return (estat);

    estat = handle_redirs(ce, &pl);
    if (estat) return (estat);

    while (!estat && pix < pl.pl_ce->ce_argc) {
        parm = pl.pl_ce->ce_argv[pix];
        if (parm[0] == '-' && parm[1]) {
            ix = 0;
            while (!estat && parm[ix]) {
                switch (parm[ix]) {
                    case '-':
                        break;
                    case 'v': case 'V':
                        cmdflags |= CMDFLAG_VERBOSE;
                        break;
                    case 'y': case 'Y':
                        cmdflags |= CMDFLAG_YES;
                        break;
                    default:
                        estat = set_ferror(ce, EBADOPT, "%c",
                                        parm[ix]);
                        break;
                }
                ix++;
            }
        } else if (pix + 1 < pl.pl_ce->ce_argc) {
            if (pl.pl_ce->ce_argc - pix > 2) cmdflags |= CMDFLAG_DIR_REQ;
            estat = copy_file(pl.pl_ce, cmdflags, parm, pl.pl_ce->ce_argv[pl.pl_ce->ce_argc - 1]);
            if (!estat) ncopied++;
        }
        pix++;            
    }

    if (!estat && !ncopied) {
        estat = set_error(ce, EUNEXPCOPYEOL, parm);
    }

    estat2 = free_parmlist_buf(ce, &pl);
    if (!estat) estat = estat2;
    else if (estat2) estat = chain_error(ce, estat, estat2);

    return (estat);
}
/***************************************************************/
int type_file(struct cenv * ce,
                int cmdflags,
                const char * fname,
                int * line_no) {

    int estat;
    int cmdfil;
    struct filinf * fi;
    struct str linstr;

    estat   = 0;
    fi      = NULL;

    if (!strcmp(fname, "-")) {
        cmdfil = ce->ce_stdin_f;
    } else {
        cmdfil = OPEN(fname, _O_RDONLY);

        if (cmdfil < 0) {
            estat = set_ferror(ce, EOPENFILE, "%s %m", fname, ERRNO);
        }
    }

    if (!estat) {
        fi = new_filinf(cmdfil, fname, CMD_LINEMAX);
        init_str(&linstr, INIT_STR_CHUNK_SIZE);

        estat = filinf_pair(ce, fi, ce->ce_stdout_f, "stdout");

        while (!estat) {
            estat = read_line(ce, fi, &linstr, PROMPT_GT, NULL);
            if (!estat) {
                (*line_no)++;
                if (cmdflags & CMDFLAG_NUMBERED) {
                    estat = printll(ce, "%6d  %s\n", (*line_no), linstr.str_buf);
                } else {
                    estat = printll(ce, "%s\n", linstr.str_buf);
                }
            }
        }
        if (estat == ESTAT_EOF) estat = 0;
        free_str_buf(&linstr);
    }

    if (fi) {
        if (!strcmp(fname, "-")) set_filinf_flags(fi, FIFLAG_NOCLOSE); /* do not close stdin */
        free_filinf(fi);
    }

    return (estat);
}
/***************************************************************/
int chndlr_type(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix) {
/*
Syntax: (debug only)

    type [ -n] [ <file> ] [ ...]                     Displays files like cat
*/
    int estat;
    int estat2;
    int pix;
    int ix;
    int num_files;
    int line_no;
    int cmdflags;
    char * parm;
    struct parmlist pl;

    estat       = 0;
    cmdflags    = 0;
    pix         = 0;
    num_files   = 0;
    line_no     = 0;

    estat = parse_parmlist(ce, cmdstr, cmdix, &pl);
    if (estat) return (estat);

    estat = handle_redirs(ce, &pl);
    if (estat) return (estat);

    while (!estat && pix < pl.pl_ce->ce_argc) {
        parm = pl.pl_ce->ce_argv[pix];
        if (parm[0] == '-' && parm[1]) {
            ix = 0;
            while (!estat && parm[ix]) {
                switch (parm[ix]) {
                    case '-':
                        break;
                    case 'n': case 'N':
                        cmdflags |= CMDFLAG_NUMBERED;
                        break;
                    default:
                        estat = set_ferror(ce, EBADOPT, "%c",
                                        parm[ix]);
                        break;
                }
                ix++;
            }
        } else {
            estat = type_file(pl.pl_ce, cmdflags, parm, &line_no);
            if (!estat) num_files++;
        }
        pix++;            
    }

    if (!estat && !num_files) {
        estat = type_file(pl.pl_ce, cmdflags, "-", &line_no);
        if (!estat) num_files++;
    }

    estat2 = free_parmlist_buf(ce, &pl);
    if (!estat) estat = estat2;
    else if (estat2) estat = chain_error(ce, estat, estat2);

    return (estat);
}
/***************************************************************/
/*@*/int get_uxfile_attr(const char * fname) {

    int result = 0;
    int fil_attr;
    struct stat stbuf;

    fil_attr = 0;
    result = LSTAT(fname, &stbuf);
    if (!result) {
        fil_attr = get_ux_fil_attr(stbuf.st_mode);
    }

    return (fil_attr);
}
/***************************************************************/
/*@*/int get_file_attr(const char * fname) {
/*
 #define FATTR_IS_DIR        1
 #define FATTR_READ_ONLY     2
 #define FATTR_HIDDEN        4
*/
#if IS_WINDOWS
    return get_winfile_attr(fname);
#else
    return get_uxfile_attr(fname);
#endif
}
/***************************************************************/
/*@*/int set_uxfile_attr(const char * fname, int attr_mask, int new_attr) {
/*
** Returns:
**      Success        - 0
**      Fail           - errno
*/

    int ern;
    int newmod;
    int result;
    struct stat stbuf;

    ern = 0;
    if (attr_mask & FATTR_READ_ONLY) {
        result = LSTAT(fname, &stbuf);
        if (result) {
            ern = errno;
        } else {
            newmod = stbuf.st_mode | (S_IRUSR | S_IWUSR); /* Add creator write */

            if (CHMOD(fname, newmod)) {
                ern = errno;
            }
        }
    }

    return (ern);
}
/***************************************************************/
/*@*/int set_file_attr(const char * fname, int attr_mask, int new_attr) {

#if IS_WINDOWS
    return set_winfile_attr(fname, attr_mask, new_attr);
#else
    return set_uxfile_attr(fname, attr_mask, new_attr);
#endif
}
/***************************************************************/
int remove_file(struct cenv * ce,
                int cmdflags,
                const char * fname) {
    int estat;
    int fil_attr;
    int ern;
    int rtn;

    estat       = 0;

    fil_attr = get_file_attr(fname);

    if (cmdflags & CMDFLAG_FORCE) {
        if (!fil_attr) return (0);

        if (fil_attr & FATTR_READ_ONLY) {
            ern = set_file_attr(fname, FATTR_READ_ONLY, 0);
            if (ern) {
                estat = set_ferror(ce, ESETATTR,  "%s: %M", fname, ern);
            }
        }
    } else {
        if (fil_attr & FATTR_READ_ONLY) {
            estat = set_error(ce, ERMREADONLY, fname);
        }
    }

    if (!estat) {
        rtn = REMOVE(fname);
        if (rtn) {
            ern = ERRNO;
            estat = set_ferror(ce, EREMOVE, "%s: %m",
                            fname, ern);
        } else {
            if (cmdflags & CMDFLAG_VERBOSE) {
                estat = printll(ce, "Removed %s\n", fname);
            }
        }
    }

    return (estat);
}
/***************************************************************/
int chndlr_remove(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix) {
/*
Syntax: (debug only)

    remove [ -v ] [ -f ] [ <dir> ] [ ...]                   Make directory

        -f      Force delete of read-only file.  Quiet if file non-existent.
        -v      Verbose - show directory name to stdout
*/
    int estat;
    int estat2;
    int pix;
    int ix;
    int num_files;
    int cmdflags;
    char * parm;
    struct parmlist pl;

    estat       = 0;
    cmdflags    = 0;
    pix         = 0;
    num_files      = 0;

    estat = parse_parmlist(ce, cmdstr, cmdix, &pl);
    if (estat) return (estat);

    estat = handle_redirs(ce, &pl);
    if (estat) return (estat);

    while (!estat && pix < pl.pl_ce->ce_argc) {
        parm = pl.pl_ce->ce_argv[pix];
        if (parm[0] == '-' && parm[1]) {
            ix = 0;
            while (!estat && parm[ix]) {
                switch (parm[ix]) {
                    case '-':
                        break;
                    case 'f': case 'F':
                        cmdflags |= CMDFLAG_FORCE;
                        break;
                    case 'v': case 'V':
                        cmdflags |= CMDFLAG_VERBOSE;
                        break;
                    default:
                        estat = set_ferror(ce, EBADOPT, "%c",
                                        parm[ix]);
                        break;
                }
                ix++;
            }
        } else {
            estat = remove_file(pl.pl_ce, cmdflags, parm);
            if (!estat) num_files++;
        }
        pix++;            
    }

    if (!estat && !num_files) {
        estat = set_error(ce, EUNEXPEOL, parm);
    }

    estat2 = free_parmlist_buf(ce, &pl);
    if (!estat) estat = estat2;
    else if (estat2) estat = chain_error(ce, estat, estat2);

    return (estat);
}
/***************************************************************/
int remove_dir_file(void * vsr, struct filattrrec * fa) {

    struct scanrec * sr = (struct scanrec *)vsr;
    int ern;
    int rtn;

    ern = 0;

    if (!(sr->sr_cmdflags & CMDFLAG_READ_ONLY)) {
        if (sr->sr_cmdflags & CMDFLAG_FORCE) {
            if (fa->fa_fil_attr & FATTR_READ_ONLY) {
                ern = set_file_attr(fa->fa_filepath, FATTR_READ_ONLY, 0);
                if (ern) {
                    sr->sr_estat = set_ferror(sr->sr_ce, ESETATTR,"%s: %M", fa->fa_filepath, ern);
                }
            }
        }

        if (!ern) {
            if (fa->fa_fil_attr & FATTR_IS_DIR) {
                rtn = RMDIR(fa->fa_filepath);
                if (rtn) {
                    ern = ERRNO;
                    sr->sr_estat = set_ferror(sr->sr_ce, ERMDIR,"%s: %m",
                                    fa->fa_filepath, ern);
                } else if (sr->sr_cmdflags & CMDFLAG_VERBOSE) {
                    sr->sr_estat = printll(sr->sr_ce, "rmdir %s\n", fa->fa_filepath);
                }
            } else {
                rtn = REMOVE(fa->fa_filepath);
                if (rtn) {
                    ern = ERRNO;
                    sr->sr_estat = set_ferror(sr->sr_ce, EREMOVE,"%s: %m",
                                    fa->fa_filepath, ern);
                } else if (sr->sr_cmdflags & CMDFLAG_VERBOSE) {
                    sr->sr_estat = printll(sr->sr_ce, "rmdir %s\n", fa->fa_filepath);
                }
            }
        }
    } else if (sr->sr_cmdflags & CMDFLAG_VERBOSE) {
        if (fa->fa_fil_attr & FATTR_IS_DIR) {
            sr->sr_estat = printll(sr->sr_ce, "dir  %s\n", fa->fa_filepath);
        } else {
            sr->sr_estat = printll(sr->sr_ce, "file %s\n", fa->fa_filepath);
        }
    }

    return (sr->sr_estat);
}
/***************************************************************/
int remove_directory(struct cenv * ce,
                int cmdflags,
                const char * dirname) {
    int estat;
    int dir_opts;
    struct scanrec * sr;
    struct dirInfo* di;
    struct filattrrec farec;

    estat       = 0;
    dir_opts    = DIROPT_POST_ORDER | DIROPT_RECURSIVE;

    sr = new_scanrec(ce, cmdflags);
    di = new_dirInfo();

    di->di_stat = get_fil_attr_rec(di, dirname, &farec);

    if (di->di_stat) {
        if (!(cmdflags & CMDFLAG_FORCE)) {
            estat = set_ferror(ce, EDIR, "%s:\n    %s", 
                    dirname, dx_errmsg(di));
        }
    } else {
        if (farec.fa_fil_attr & FATTR_IS_DIR) {
            di->di_stat = dir_scan(di, &farec, NULL, NULL, dir_opts,
                                     remove_dir_file, sr, 1);
            if (di->di_stat) {
                if (di->di_stat == DIRERR_CALLBACK_ERR) {
                    estat = chain_error(ce, set_error(ce, EREMOVEDIR, NULL), sr->sr_estat);
                } else {
                    estat = set_ferror(ce, EDIR, "%s:\n    %s", 
                            dirname, dx_errmsg(di));
                }
            }
        } else {
            estat = set_error(ce, EMUSTBEDIR, dirname);
        }
    }
    free_dirInfo(di);
    free_scanrec(sr);

    return (estat);
}
/***************************************************************/
int chndlr_removedir(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix) {
/*
Syntax: (debug only)

    removedir [ -v ] [ -f ] [ <dir> ] [ ...]                   Remove directory

        -f      Force delete of read-only file.  Quiet if file non-existent.
        -v      Verbose - show directory name to stdout
*/
    int estat;
    int estat2;
    int pix;
    int ix;
    int num_files;
    int cmdflags;
    char * parm;
    struct parmlist pl;

    estat       = 0;
    cmdflags    = 0;
    pix         = 0;
    num_files      = 0;

    estat = parse_parmlist(ce, cmdstr, cmdix, &pl);
    if (estat) return (estat);

    estat = handle_redirs(ce, &pl);
    if (estat) return (estat);

    while (!estat && pix < pl.pl_ce->ce_argc) {
        parm = pl.pl_ce->ce_argv[pix];
        if (parm[0] == '-' && parm[1]) {
            ix = 0;
            while (!estat && parm[ix]) {
                switch (parm[ix]) {
                    case '-':
                        break;
                    case 'f': case 'F':
                        cmdflags |= CMDFLAG_FORCE;
                        break;
                    case 'r': case 'R':
                        cmdflags |= CMDFLAG_READ_ONLY;
                        break;
                    case 'v': case 'V':
                        cmdflags |= CMDFLAG_VERBOSE;
                        break;
                    default:
                        estat = set_ferror(ce, EBADOPT, "%c",
                                        parm[ix]);
                        break;
                }
                ix++;
            }
        } else {
            estat = remove_directory(pl.pl_ce, cmdflags, parm);
            if (!estat) num_files++;
        }
        pix++;            
    }

    if (!estat && !num_files) {
        estat = set_error(ce, EUNEXPEOL, parm);
    }

    estat2 = free_parmlist_buf(ce, &pl);
    if (!estat) estat = estat2;
    else if (estat2) estat = chain_error(ce, estat, estat2);

    return (estat);
}
/***************************************************************/
int move_file(struct cenv * ce,
                int cmdflags,
                const char * src,
                const char * tgt) {
/*
*/
    int estat;
    int tgtft;
    int ern;

    estat       = 0;

    tgtft = file_stat(tgt);
    if (tgtft == FTYP_FILE) {
        if (!(cmdflags & CMDFLAG_YES)) {
            estat = set_error(ce, ECOPYTGTEXISTS, tgt);
        } else {
            estat = remove_file(ce, cmdflags, tgt);
        }
    }

    if (!estat) {
        if (RENAME(src, tgt)) {
            ern = ERRNO;
            estat = set_ferror(ce, ERENAME, "%s --> %s\n    %m", src, tgt, ern);
        }

        if (!estat & IS_VERBOSE(ce,cmdflags)) {
            estat = printll(ce, "Moved %s --> %s\n", src, tgt);
        }
    }

    return (estat);
}
/***************************************************************/
int chndlr_move(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix) {
/*
Syntax:

    move [ -<opt> ] [ <dir> ] [ ...]          Move files/directories

        <opt>   
        f or F  Force.  Force removal of target file.
        v or V  Verbose.  Show source and target.
        y or Y  Yes.  Deleting existing target, if it exists.
*/
    int estat;
    int estat2;
    int pix;
    int ix;
    int cmdflags;
    int num_files;
    char * parm;
    struct parmlist pl;

    estat       = 0;
    cmdflags    = 0;
    pix         = 0;
    num_files     = 0;
    parm        = "";

    estat = parse_parmlist(ce, cmdstr, cmdix, &pl);
    if (estat) return (estat);

    estat = handle_redirs(ce, &pl);
    if (estat) return (estat);

    while (!estat && pix < pl.pl_ce->ce_argc) {
        parm = pl.pl_ce->ce_argv[pix];
        if (parm[0] == '-' && parm[1]) {
            ix = 0;
            while (!estat && parm[ix]) {
                switch (parm[ix]) {
                    case '-':
                        break;
                    case 'f': case 'F':
                        cmdflags |= CMDFLAG_FORCE;
                        break;
                    case 'v': case 'V':
                        cmdflags |= CMDFLAG_VERBOSE;
                        break;
                    case 'y': case 'Y':
                        cmdflags |= CMDFLAG_YES;
                        break;
                    default:
                        estat = set_ferror(ce, EBADOPT, "%c",
                                        parm[ix]);
                        break;
                }
                ix++;
            }
        } else {
            if (pix + 2 == pl.pl_ce->ce_argc) {
                estat = move_file(pl.pl_ce, cmdflags, parm, pl.pl_ce->ce_argv[pl.pl_ce->ce_argc - 1]);
                if (!estat) num_files++;
                pix++;
            } else {
                estat = set_error(ce, ERENAMEPARMS, parm);
            }
        }
        pix++;            
    }

    if (!estat && !num_files) {
        estat = set_error(ce, ERENAMEPARMS, parm);
    }

    estat2 = free_parmlist_buf(ce, &pl);
    if (!estat) estat = estat2;
    else if (estat2) estat = chain_error(ce, estat, estat2);

    return (estat);
}
/***************************************************************/
struct getfilesrec {
    struct cenv *       gfr_ce;
    int                 gfr_estat;
    int                 gfr_namix;
    int                 gfr_mxfiles;
    int                 gfr_nfiles;
    char **             gfr_fnamelist;
};
/***************************************************************/
static struct getfilesrec* new_getfilesrec(struct cenv * ce)
{
    struct getfilesrec * gfr;

    gfr = New(struct getfilesrec, 1);
    gfr->gfr_ce         = ce;
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
int get_filenames(struct cenv * ce,
                const char * dfname,
                char *** fnamelist) {

    int estat;
    int swflags;
    struct getfilesrec * gfr;
    void * vdi;
    int dir_opts;
    char ** dirpats;
    char dirnam[PATH_MAXIMUM];
    char filpat[FILE_MAXIMUM];

    estat           = 0;
    swflags         = 0;
    (*fnamelist)    = NULL;
    dir_opts        = 0;
    if (ce->ce_flags & CEFLAG_CASE)   dir_opts |= DIROPT_CASE;
    if (ce->ce_flags & CEFLAG_WIN)    swflags  |= SWFLAG_DOT_STAR;
    if (!(ce->ce_flags & CEFLAG_WIN)) dir_opts |= DIROPT_DOT_HIDES;

    if (splitwild(dfname, dirnam, &dirpats, filpat, swflags)) {
        estat = set_ferror(ce, EFILSET, "%s", dfname);
        return (estat);
    }


    gfr = new_getfilesrec(ce);

    gfr->gfr_namix = 0;
    if (!dirnam[0]) gfr->gfr_namix = 2; /* Hide leading ./ */

    vdi = scan_directory(dirnam, dirpats, filpat, dir_opts, getfilesrec_callback, gfr);
    if (dx_stat(vdi)) {
        if (dx_stat(vdi) == DIRERR_CALLBACK_ERR) {
            estat = set_error(ce, EDIR, NULL);
        } else {
            estat = set_ferror(ce, EDIR, "%s:\n    %s", 
                    dfname, dx_errmsg(vdi));
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
