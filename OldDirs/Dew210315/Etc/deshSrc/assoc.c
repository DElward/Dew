/***************************************************************/
/* assoc.c                                                     */
/***************************************************************/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>


#if IS_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <tchar.h>
#include <shlobj.h>
#include <shlguid.h>
#include <shellapi.h>
#include <Shlwapi.h>
#include <Aclapi.h>
#include <io.h>
#else
#include <time.h>
#include <sys/time.h>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#endif

/***************************************************************/

#include "desh.h"
#include "when.h"
#include "assoc.h"
#include "snprfh.h"
#include "msg.h"
#include "cmd.h"
#include "cmd2.h"
#include "sysx.h"

/***************************************************************/
#if IS_WINDOWS
void get_timezone_info(int * tzone, int * is_dst) {

    int rtn;
    TIME_ZONE_INFORMATION tzi;

    (*tzone) = 0;
    (*is_dst) = 0;

    rtn = GetTimeZoneInformation(&tzi);
    if (rtn == TIME_ZONE_ID_DAYLIGHT) (*is_dst) = 1;
    (*tzone) = tzi.Bias;
}
/***************************************************************/
#else
void get_timezone_info(int * tzone, int * is_dst) {

    struct timeval tvi;
    struct timezone tzi;

    (*tzone) = 0;
    (*is_dst) = 0;

    if (!gettimeofday(&tvi, &tzi)) {
        (*tzone)  = tzi.tz_minuteswest;
        (*is_dst) = tzi.tz_dsttime;
    }
}
#endif
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
	    fAttr = GetFileAttributes(fname);
	    if (fAttr <  0) {
            result = FTYP_NONE;
	    } else {
            if (fAttr & FILE_ATTRIBUTE_DIRECTORY)   result = FTYP_DIR;
            else {
                fnlen = Istrlen(fname);
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
#if IS_WINDOWS
/*@*/int win_check_file_access(const char * fname, int facc) {
    
	int fAttr;
    int result = 0;
    int fnlen;
    
    if (fname[0] == '.' && (fname[1] == '\0' ||
                            (fname[1] == '.' && fname[2] == '\0'))) {
        result = 0;
    } else {
	    fAttr = GetFileAttributes(fname);
	    if (fAttr <  0) {
            result = 0;
	    } else {
            if (fAttr & FILE_ATTRIBUTE_DIRECTORY) {
                result = 0;
             } else {
                fnlen = Istrlen(fname);
                if (fnlen > 4 && fname[fnlen-4] == '.'  &&
                    (fname[fnlen-3] == 'l' || fname[fnlen-3] == 'L') &&
                    (fname[fnlen-2] == 'n' || fname[fnlen-2] == 'N') &&
                    (fname[fnlen-1] == 'k' || fname[fnlen-1] == 'K')) {
                    result = 0;
                } else  {
                    result = 1;
                }
            }
        }
    }
    
	return (result);
}
#endif
/***************************************************************/
/*@*/int ux_check_file_access(const char * fname, int facc) {
    
    int result = 0;
    struct stat stbuf;

    result = 0;

    if (!STAT(fname, &stbuf)) {
        if (F_S_ISREG(stbuf.st_mode) && (stbuf.st_mode & facc)) result = 1;
    }
    
    return (result);
}
/***************************************************************/
/*@*/int check_file_access(const char * fname, int facc) {
    
#if IS_WINDOWS
    return win_check_file_access(fname, facc);
#else
    return ux_check_file_access(fname, facc);
#endif
}
/***************************************************************/
void get_modbuf(int fs_mode, char * modbuf) {

    int ifmt;

    strcpy(modbuf, "----------");

    ifmt = fs_mode & FSTAT_S_IFMT;

         if (ifmt == FSTAT_S_IFDIR) modbuf[0] = 'd';
    else if (ifmt == FSTAT_S_IFIFO) modbuf[0] = 'p';
    else if (ifmt == FSTAT_S_IFCHR) modbuf[0] = 'c';

    if (fs_mode & S_IRUSR) modbuf[1] = 'r';
    if (fs_mode & S_IWUSR) modbuf[2] = 'w';
    if (fs_mode & S_IXUSR) modbuf[3] = 'x';

    if (fs_mode & S_IRGRP) modbuf[4] = 'r';
    if (fs_mode & S_IWGRP) modbuf[5] = 'w';
    if (fs_mode & S_IXGRP) modbuf[6] = 'x';

    if (fs_mode & S_IROTH) modbuf[7] = 'r';
    if (fs_mode & S_IWOTH) modbuf[8] = 'w';
    if (fs_mode & S_IXOTH) modbuf[9] = 'x';

}
/***************************************************************/
#if IS_WINDOWS
int winfile_get_file_stat(const char * fname,
                    struct file_stat_rec * fsr,
                    char   * errmsg,
                    int      errmsgmax) {

	int fResult;
    int result = 0;
    time_t fdat;
    char * emsg;
    WIN32_FILE_ATTRIBUTE_DATA fAttrData;

    memset(fsr, 0, sizeof(struct file_stat_rec));

    errmsg[0]  = '\0';

	fResult = GetFileAttributesEx(fname, GetFileExInfoStandard, &fAttrData);
	if (!fResult) {
        result = -1;
        emsg = GetWinMessage(GetLastError());
        strmcpy(errmsg, emsg, errmsgmax);
        Free(emsg);
	} else {
        if (fAttrData.dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
            fsr->fsr_mode = _S_IREAD;
        } else {
            fsr->fsr_mode = (_S_IREAD|_S_IWRITE);
        }
        /* propagate user read/write/execute bits to group/other fields */

        fsr->fsr_mode |= (fsr->fsr_mode & 0700) >> 3;
        fsr->fsr_mode |= (fsr->fsr_mode & 0700) >> 6;

        fdat = filetime_to_time(&(fAttrData.ftLastAccessTime)),
        fsr->fsr_accdate = time_t_to_d_time(fdat, 0);

        fdat = filetime_to_time(&(fAttrData.ftCreationTime)),
        fsr->fsr_crdate = time_t_to_d_time(fdat, 0);

        fdat = filetime_to_time(&(fAttrData.ftLastWriteTime)),
        fsr->fsr_moddate = time_t_to_d_time(fdat, 0);

        fsr->fsr_size = ((int64)fAttrData.nFileSizeHigh << 32) |
                         (int64)fAttrData.nFileSizeLow;
    }

	return (result);
}
#endif
/***************************************************************/
int uxfile_get_file_stat(const char * fname,
                    struct file_stat_rec * fsr,
                    char   * errmsg,
                    int      errmsgmax) {
/*
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

 struct filattrrec {
 int     fa_fil_attr;
 uint32  fa_file_size_hi;
 uint32  fa_file_size_lo;
 time_t  fa_mod_date;
 time_t  fa_cr_date;
 int     fa_name_ix;
 char    fa_filepath[PATH_MAXIMUM];
 };
 
 struct file_stat_rec {
 int     fsr_mode;
 int     fsr_uid;
 int     fsr_gid;
 d_date  fsr_accdate;
 d_date  fsr_crdate;
 d_date  fsr_moddate;
 int64   fsr_size;
 };


*/
    int result = 0;
    int ern;
    int is_dst;
    int tzone;
    struct stat stbuf;

    memset(fsr, 0, sizeof(struct file_stat_rec));
    errmsg[0]  = '\0';

    result = LSTAT(fname, &stbuf);
    if (result) {
        ern = ERRNO;
        strmcpy(errmsg, strerror(ern), errmsgmax);
    } else {
        get_timezone_info(&(tzone), &(is_dst));
        is_dst = 0;

        fsr->fsr_mode  = stbuf.st_mode;
        fsr->fsr_uid   = stbuf.st_uid;
        fsr->fsr_gid   = stbuf.st_gid;

        fsr->fsr_accdate = time_t_to_d_time(stbuf.st_atime, is_dst);
        fsr->fsr_crdate  = time_t_to_d_time(stbuf.st_mtime, is_dst);
        fsr->fsr_moddate = time_t_to_d_time(stbuf.st_ctime, is_dst);
        fsr->fsr_size    = (int64)(stbuf.st_size);
    }

    return (result);
}
/***************************************************************/
int get_file_stat(const char * fname,
                    struct file_stat_rec * fsr,
                    char   * errmsg,
                    int      errmsgmax) {

#if IS_WINDOWS
    return winfile_get_file_stat(fname, fsr, errmsg, errmsgmax);
#else
    return uxfile_get_file_stat(fname, fsr, errmsg, errmsgmax);
#endif
}
/***************************************************************/
#if IS_WINDOWS
int win_get_file_owner(char * fname,
                    int fs_uid,
                    char * ownnam,
                    int ownsiz,
                    char * grpnam,
                    int grpsiz,
                    char * errmsg,
                    int errmsgmax) {

/*
**  DWORD GetNamedSecurityInfo(
**    LPTSTR pObjectName,                        // object name
**    SE_OBJECT_TYPE ObjectType,                 // object type
**    SECURITY_INFORMATION SecurityInfo,         // information type
**    PSID *ppsidOwner,                          // owner SID
**    PSID *ppsidGroup,                          // primary group SID
**    PACL *ppDacl,                              // DACL
**    PACL *ppSacl,                              // SACL
**    PSECURITY_DESCRIPTOR *ppSecurityDescriptor // SD
**  );
**  
**  BOOL LookupAccountSid(
**    LPCTSTR lpSystemName,  // name of local or remote computer
**    PSID Sid,              // security identifier
**    LPTSTR Name,           // account name buffer
**    LPDWORD cbName,        // size of account name buffer
**    LPTSTR DomainName,     // domain name
**    LPDWORD cbDomainName,  // size of domain name buffer
**    PSID_NAME_USE peUse    // SID type
**  );
**  
*/
    int rtn;
    //LPTSTR                  pObjectName;             // object name
    //SE_OBJECT_TYPE          ObjectType;              // object type
    //SECURITY_INFORMATION    SecurityInfo;            // information type
    PSID                    psidOwner;               // owner SID
    PSID                    psidGroup;               // primary group SID
    //PACL                    pDacl;                   // DACL
    //PACL                    pSacl;                   // SACL
    PSECURITY_DESCRIPTOR    pSecurityDescriptor;     // SD
    SID_NAME_USE            peUse;
    DWORD cbName;
    DWORD cbDomainName;
    int ern;
    int erv;
    int domsiz;
    char domnam[64];

    erv = 0;
    errmsg[0] = 0;

    rtn = GetNamedSecurityInfo(fname,
                SE_FILE_OBJECT,
                OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION,
                &psidOwner,
                &psidGroup,
                NULL,
                NULL,
                &pSecurityDescriptor);

    if (rtn != ERROR_SUCCESS) {
        ern = GETERRNO;
        Snprintf(errmsg, errmsgmax, "Error on GetNamedSecurityInfo(): %M",
            ern);
        erv = -1;
        pSecurityDescriptor = NULL;
    }

    if (!erv) {
        domsiz = sizeof(domnam);

        cbName       = ownsiz;
        cbDomainName = domsiz;
        rtn = LookupAccountSid(NULL,
                psidOwner, ownnam, &cbName, domnam, &cbDomainName, &peUse);
        if (!rtn) {
            ern = GETERRNO;
            Snprintf(errmsg, errmsgmax, "Error on LookupAccountSid(%d) owner: %M",
                (int)psidOwner, ern);
            erv = -1;
        }
    }

    if (!erv) {
       domsiz = sizeof(domnam);

        cbName       = grpsiz;
        cbDomainName = domsiz;
        rtn = LookupAccountSid(NULL,
                psidGroup, grpnam, &cbName, domnam, &cbDomainName, &peUse);
        if (!rtn) {
            ern = GETERRNO;
            Snprintf(errmsg, errmsgmax, "Error on LookupAccountSid(%d) group: %M",
                (int)psidGroup, ern);
            erv = -1;
        }
    }
    if (pSecurityDescriptor) LocalFree(pSecurityDescriptor);
        
    return (erv);
}
#endif
/***************************************************************/
#if ! IS_WINDOWS /* Unix */
int ux_get_file_owner(char * fname,
		int fs_uid,
		char * ownnam,
		int ownsiz,
		char * grpnam,
		int grpsiz,
		char * errmsg,
		int errmsgmax) {
	int erv = 0;
	int ern;
	struct passwd * ppwd;
	struct group * pgrp;
	struct stat stbuf;

	if (LSTAT(fname, &stbuf)) {
		ern = ERRNO;
		strmcpy(errmsg, strerror(ern), errmsgmax);
		erv = -1;
	} else {
		ppwd = getpwuid(stbuf.st_uid);
		if (!ppwd) {
			ern = ERRNO;
			strmcpy(errmsg, strerror(ern), errmsgmax);
			erv = -1;
		}
	}
	if (!erv) {
		pgrp = getgrgid(stbuf.st_gid);
		if (!ppwd) {
			ern = ERRNO;
			strmcpy(errmsg, strerror(ern), errmsgmax);
			erv = -1;
		}
	}
	if (!erv) {
		strmcpy(ownnam, ppwd->pw_name, ownsiz);
		strmcpy(grpnam, pgrp->gr_name, grpsiz);
	}
    return (erv);
}
#endif
/***************************************************************/
int get_file_owner(char * fname,
                   int fs_uid,
                   char * ownnam,
                   int ownsiz,
                   char * grpnam,
                   int grpsiz,
                   char * errmsg,
                   int errmsgmax) {
#if IS_WINDOWS
    return win_get_file_owner(fname, fs_uid,
                              ownnam, ownsiz,
                              grpnam, grpsiz,
                              errmsg, errmsgmax);
#else
    return ux_get_file_owner(fname, fs_uid,
                              ownnam, ownsiz,
                              grpnam, grpsiz,
                              errmsg, errmsgmax);
#endif
}
/***************************************************************/
#if IS_WINDOWS

static int com_inited = 0;

static int                  stat_AssocQueryString = 0;
typedef DWORD               ASSOCF;
typedef int                 ASSOCSTR;
typedef HRESULT (WINAPI *p_AssocQueryString) (
    ASSOCF flags, 
    ASSOCSTR str, 
    LPCTSTR pszAssoc, 
    LPCTSTR pszExtra, 
    LPTSTR pszOut, 
    DWORD *pcchOut
);
static p_AssocQueryString   dl_AssocQueryString;

#define ASSOCF_NOTRUNCATE   0x00000020
#define ASSOCSTR_COMMAND    1

/***************************************************************/
static void load_AssocQueryString(char * errmsg, int errmsgmax) {

    HMODULE hLibrary;
    int ern;

    hLibrary = LoadLibrary("Shlwapi.dll");
    if ((intr)hLibrary < 32) {
        ern = GETERRNO;
        Snprintf(errmsg, errmsgmax, "Error loading library Shlwapi.dll: %M",
                ern);
        stat_AssocQueryString = -2;
    } else {
        dl_AssocQueryString = (p_AssocQueryString)
                        GetProcAddress(hLibrary, "AssocQueryStringA");
        if (!dl_AssocQueryString) {
            ern = GETERRNO;
            Snprintf(errmsg, errmsgmax, "Error loading procedure AssocQueryStringA(): %M",
                ern);
            stat_AssocQueryString = -1;
        } else {
            stat_AssocQueryString = 1;
        }
    }
}
/***************************************************************/
#if CHECK_OPENWITH
int is_open_with_exe(const char * pname) {

    int is_open_with;
    int pnlen;

    is_open_with = 0;
    pnlen = Istrlen(pname);
    if (pnlen > 13) {
        is_open_with = ! Stricmp(pname + pnlen - 13, "\\OpenWith.Exe");
    }

	return (is_open_with);
}
/***************************************************************/
int assoc_open_with(const char * pszDst) {

    int a_open_with;
    char *  cargv;
    char ** scargv;
    int     argc;

    a_open_with = 0;

    cargv = parse_command_line(pszDst, &argc, NULL, 0);
    scargv = (char**)cargv;

    if (argc) {
        a_open_with = is_open_with_exe(scargv[0]);
    }

    Free(cargv);

	return (a_open_with);
}
#endif
/***************************************************************/
int resolve_assoc( const char * pszSrc,
		char * pszDst,
        int pszDstSize,
		char * errmsg,
        int errmsgmax) {

	// See help for: AssocQueryKey()

/*
**  Could not get this to work on XP with static DLL
**
**  Returns:
**      -3 - Associations not available, i.e. not a Windows system
**      -2 - Could not find Shlwapi.dll
**      -1 - Could not load function AssocQueryString() from dll
**       0 - Success, however pszDst[0] may be zero (I guess)
**       1 - AssocQueryString() failed
**
** Set command prompt commands: ASSOC and FTYPE
*/
	int result = 0;
    DWORD pcchout;
    char pszExtra[] = "open";

    result = 1;

	errmsg[0] = '\0';
	pszDst[0] = '\0';

    if (!stat_AssocQueryString) {
        load_AssocQueryString(errmsg, errmsgmax);
    }
    if (stat_AssocQueryString < 0) {
        return (stat_AssocQueryString);
    }

    pcchout = pszDstSize;
    result = dl_AssocQueryString(ASSOCF_NOTRUNCATE, ASSOCSTR_COMMAND, pszSrc, pszExtra,
                    pszDst, &pcchout);
    if (result) {
        result = 1;
        Snprintf(errmsg, errmsgmax, "No association for %s", pszSrc);
    }
 #if CHECK_OPENWITH
   else if (assoc_open_with(pszDst)) {

        result = 1;
        Snprintf(errmsg, errmsgmax, "Association for %s is OpenWith.exe", pszSrc);
    }
#endif

	return result;
}
#else
/***************************************************************/
int resolve_assoc( const char * pszSrc,
                  char * pszDst,
                  int pszDstSize,
                  char * errmsg,
                  int errmsgmax) {

/*
**  Returns:
**      -3 - Associations not available, i.e. not a Windows system
*/
	int result = 0;

    result = -3;
    Snprintf(errmsg, errmsgmax, "File associations not available for %s", pszSrc);

	return result;
}
#endif
/***************************************************************/
#if ! IS_WINDOWS /* Unix */
int make_link( const char * link_name,
		const char * link_tgt,
		const char * link_desc,
		char * errmsg,
          int errmsgmax) {

	if (symlink(link_tgt, link_name)) {
		int ern = ERRNO;
		Snprintf(errmsg, errmsgmax, "symlink() error on %s\n%m", link_name, ern);
		return -1;
	}
	/*
    There are nine system calls that do not follow links, and which operate on the symbolic link itself.
    They are: lchflags(2), lchmod(2), lchown(2), lstat(2), lutimes(3), readlink(2), rename(2), rmdir(2),
    and unlink(2).  Because remove(3) is an alias for unlink(2), it also does not follow symbolic links.
    When rmdir(2) is applied to a symbolic link, it fails with the error ENOTDIR.
	 */
	return (0);
}
#endif
/***************************************************************/
/*
** tempfile functions
*/
/***************************************************************/
/*@*/struct tempfile * tmpfile_open(void) {
/*
** Opens a temporary file. 
*/

    FILE * fref;
    struct tempfile * tf;
#if ! USE_TMPFILE
    static char * tmpdir = NULL;
    static int    tmpdirstat = 0;
    char *tfnam;

	#if IS_WINDOWS
		const char tmpvarnam[]  = "TMP";
		const char fmode[]      = "w+D"; /* "D" means temp - see _openfile() */
	#else
		const char tmpvarnam[]  = "TMPDIR";
		const char fmode[]      = "w+";
	#endif
#endif

    tf = NULL;

#if USE_TMPFILE
    fref = tmpfile();
    if (fref) {
        REG_FOPEN(fref, "(temp)");
        tf = New(struct tempfile, 1);
        tf->tf_fref = fref;
        tf->tf_fname = Strdup("(temp)");
    }
#else
    if (!tmpdirstat) {
        tmpdir = GETENV(tmpvarnam);
        tmpdirstat = 1;
    }

    tfnam = TEMPNAM(tmpdir, "desh"); /* Using tmpfile or mkstemp is a safe way to avoid this problem.  */
    if (tfnam) {
        fref = fopen(tfnam, fmode);
        if (fref) {
            REG_FOPEN(fref, tfnam);
            tf = New(struct tempfile, 1);
            tf->tf_fref = fref;
            tf->tf_fname = Strdup(tfnam);
        }
    }
#endif

    return (tf);
};
/***************************************************************/
void tmpfile_close(struct tempfile * tf) {
/*
** Closes a temporary file. 
*/

    REG_FCLOSE(tf->tf_fref);
    fclose(tf->tf_fref);
    REMOVE(tf->tf_fname);
    Free(tf->tf_fname);
    Free(tf);
}
/***************************************************************/
int tmpfile_write(struct tempfile * tf, const char * buf, int blen) {
/*
** Writes to a temporary file. 
*/
    int rtn;

    rtn = (Int)fwrite(buf, blen, 1, tf->tf_fref);

    return (rtn);
}
/***************************************************************/
int tmpfile_filno(struct tempfile * tf) {
/*
** Gets temporary file fref
*/

    return (FILENO(tf->tf_fref));
}
/***************************************************************/
const char * tmpfile_filnam(struct tempfile * tf) {
/*
** Gets temporary file fref
*/

    return (tf->tf_fname);
}
/***************************************************************/
int tmpfile_reset(struct tempfile * tf) {
/*
** Writes to a temporary file. 
*/
    rewind(tf->tf_fref);

    return (FILENO(tf->tf_fref));
}

#if WAREHOUSE_CODE

/***************************************************************/
/*@*/static void nt_set_working_dir(char *username,
                                    char * domainname,
                                    EBLK **err)
{
   NET_API_STATUS net_api_status;
   USER_INFO_1 *user_info_1;

   WCHAR unicode_username[200];
   WCHAR unicode_domainname[200];
   WCHAR * dp;
   char home_directory[200];
   LPBYTE domain_controller = 0;
   char dc[100];

   if (!str_to_wstr(username,
                    unicode_username,
                    sizeof unicode_username / sizeof(WCHAR))) {
      *err = set_error(5270);
      set_error_text(*err, username);
      return;
   }
   if (domainname && (domainname[0] != '.' || domainname[1])) {
       if (!str_to_wstr(domainname,
                    unicode_domainname,
                    sizeof unicode_username / sizeof(WCHAR))) {
          *err = set_error(5271);
          set_error_text(*err, domainname);
          return;
       }
       net_api_status = NetGetDCName(NULL, unicode_domainname,
                        &domain_controller);
       if (net_api_status != NERR_Success) {
        if (net_api_status == NERR_DCNotFound) {
          *err = set_error(5273);
          set_error_text(*err, domainname);
           return;
        } else {
          *err = set_error(5272);
          set_error_text(*err, domainname);
          set_error_long(*err, "%ld", net_api_status);
           return;
        }
       }
       dp = (WCHAR*)domain_controller;
       wstr_to_str(dp, dc, sizeof(dc)); /* for debugging only */
   } else {
       dp = NULL;
       dc[0] = '\0';
   }

   net_api_status = NetUserGetInfo(
                                   dp,   // Local server
                                   unicode_username,
                                   1,      // indicates type of next arg,
                                           // the buffer
                                   (LPBYTE *)(&user_info_1)   // buffer
                                  );
   if (net_api_status != NERR_Success) {
       char s[256];
       if (net_api_status == ERROR_ACCESS_DENIED) {
          strcpy(s, "Access Denied.  The user does not have access"
                    "to the requested information.");

       } else if (net_api_status == NERR_InvalidComputer) {
          strcpy(s, "The computer name is invalid.");

       } else if (net_api_status == NERR_UserNotFound) {
          strcpy(s, "The user name could not be found");

       } else if (net_api_status == ERROR_BAD_NETPATH) {
          sprintf(s,
            "The network path specified in the %s (\"%s\") was not found.",
            "servername parameter", dc);

       } else if (net_api_status == ERROR_INVALID_LEVEL) {
          strcpy(s,
              "The value specified for the level parameter (1) is invalid.");

       } else {
          sprintf(s,
            "Unrecognized error code returned: %ld",
                   (long)net_api_status);
       }
       *err = set_error(5257);
       set_error_text(*err, s);
       return;
   }

   if (!wstr_to_str(
                    user_info_1->usri1_home_dir,
                    home_directory,
                    sizeof home_directory
                   )) {
      *err = set_error(5061);
      return;
   }

   if (strlen(home_directory) > 0) {
      if (!SetCurrentDirectory(home_directory)) {
         *err = set_error(5062);
         set_error_text(*err, home_directory);
         return;
      }
   }
   (void)NetApiBufferFree(user_info_1);
   if (domain_controller) (void)NetApiBufferFree(domain_controller);
}
/***************************************************************/


#endif
/***************************************************************/
#if IS_WINDOWS
static int get_win_home_directory(struct cenv * ce, char * dirname, int dnmax) {

    int estat;
    char *  home_dir;

    estat       = 0;

    home_dir = find_globalvar(ce->ce_gg, "USERPROFILE", 11);
    if (home_dir) {
        strmcpy(dirname, home_dir, dnmax);
    } else {
        dirname[0] = '\0';
    }

    return (estat);
}
/***************************************************************/
#else
static int get_ux_home_directory(struct cenv * ce, char * dirname, int dnmax) {

    int estat;
    struct passwd *pwrec;
    char *  home_dir;

    estat      = 0;
    dirname[0] = '\0';

    home_dir = find_globalvar(ce->ce_gg, "HOME", 4);
    if (home_dir) {
        strmcpy(dirname, home_dir, dnmax);
    } else {
        pwrec = getpwuid(getuid());
        if (pwrec) {
            strmcpy(dirname, pwrec->pw_dir, dnmax);
        }
    }

    return (estat);
}
#endif
/***************************************************************/
int get_home_directory(struct cenv * ce, char * dirname, int dnmax) {

    int estat;

    estat       = 0;

#if IS_WINDOWS
        estat = get_win_home_directory(ce, dirname, dnmax);
#else
        estat = get_ux_home_directory(ce, dirname, dnmax);
#endif

    return (estat);
}
/***************************************************************/
#if IS_WINDOWS
static int get_win_host_name(struct cenv * ce, char * hostname, int hnmax) {
/*
    WSADATA wsaData;

    WSAStartup(MAKEWORD(2, 2), &wsaData);

    if (gethostname(hostname, hnmax)) {
        hostname[0] = '\0';
    }

    WSACleanup();

*/
    int estat;
    char *  host_nam;

    estat       = 0;

    host_nam = find_globalvar(ce->ce_gg, "COMPUTERNAME", 12);
    if (host_nam) {
        strmcpy(hostname, host_nam, hnmax);
    } else {
        hostname[0] = '\0';
    }

    return (estat);
}
/***************************************************************/
#else
static int get_ux_host_name(struct cenv * ce, char * hostname, int hnmax) {

    int estat;

    estat      = 0;

    if (gethostname(hostname, hnmax)) {
        hostname[0] = '\0';
    }

    return (estat);
}
#endif
/***************************************************************/
int get_host_name(struct cenv * ce, char * hostname, int hnmax) {

    int estat;

    estat       = 0;

#if IS_WINDOWS
        estat = get_win_host_name(ce, hostname, hnmax);
#else
        estat = get_ux_host_name(ce, hostname, hnmax);
#endif

    return (estat);
}
/***************************************************************/
