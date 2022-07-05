/***************************************************************/
/*
**  osc.c --  OS Specific and utilities
*/
/***************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "config.h"

#include "HttpGet.h"
#include "osc.h"

#if IS_WINDOWS
    #include <time.h>
    #define WIN32_LEAN_AND_MEAN
    #include <Windows.h>
#include <sys/types.h>
#include <sys/stat.h>

    #define PATH_SEPARATOR ';'
    #define DIRDELIM '\\'
#else
    #define PATH_SEPARATOR ':'
    #define DIRDELIM '/'
#endif

typedef int     d_daynum;    /* Days since Day 0 - Friday December 31, 1999 */

/***************************************************************/
void Strmcpy(char * tgt, const char * src, size_t tmax) {
/*
** Copies src to tgt, trucating src if too long.
**  tgt always termintated with '\0'
*/

    size_t slen;

    slen = strlen(src) + 1;

    if (slen <= tmax) {
        memcpy(tgt, src, slen);
    } else if (tmax) {
        slen = tmax - 1;
        memcpy(tgt, src, slen);
        tgt[tmax] = '\0';
    }
}
/***************************************************************/
struct dynprocinfo {
    char * libpath;
    char * libname;
    char * procname;
    void * libinfo;
    void * plabel;
};

/***************************************************************/
#ifdef S_IFDIR
    #define t_S_IFDIR               S_IFDIR
    #define t_S_IFREG               S_IFREG
#else
    #ifdef _S_IFDIR
        #define t_S_IFDIR           _S_IFDIR
        #define t_S_IFREG           _S_IFREG
    #else
        #ifdef __S_IFDIR
            #define t_S_IFDIR       __S_IFDIR
            #define t_S_IFREG       __S_IFREG
        #else
            #error No S_IFDIR defined.
        #endif
    #endif
#endif
/***************************************************************/
/*@*/int os_stat(const char * fname, struct stat * stbuf) {

    int result;

    result = stat(fname, stbuf);

    return (result);
}
/***************************************************************/
/*@*/int os_ftype(const char * fname) {
/*
** Figures out file type.
**  Returns:
**      OSFTYP_NONE (0) - No such file or cannot access
**      OSFTYP_FILE (1) - Regular file
**      OSFTYP_DIR  (2) - Directory
**      OSFTYP_OTHR (3) - Special file
*/
    int result;
    struct stat stbuf;

    result = OSFTYP_NONE;
    if (os_stat(fname, &stbuf)) return (OSFTYP_NONE);

    if (stbuf.st_mode & t_S_IFDIR)      result = OSFTYP_DIR;
    else if (stbuf.st_mode & t_S_IFREG) result = OSFTYP_FILE;
    else                                result = OSFTYP_OTHR;

    return (result);
}
/***************************************************************/
#ifdef WIN32
void win_get_message(long errCode, char * errMsg, long errMsgSize) {

    LPVOID lpMsgBuf;

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        errCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (LPTSTR) &lpMsgBuf,
        0,
        NULL
    );
    strncpy(errMsg, lpMsgBuf, errMsgSize);

    LocalFree( lpMsgBuf );
}
/***************************************************************/
#else
void ux_get_message(long errCode, char * errMsg, long errMsgSize) {

    strncpy(errMsg, strerror(errCode), errMsgSize);
}
#endif
/***************************************************************/
void os_get_message(long errCode, char * errMsg, long errMsgSize) {

    int mlen;

#ifdef WIN32
    win_get_message(errCode, errMsg, errMsgSize);
#else
    ux_get_message(errCode, errMsg, errMsgSize);
#endif

    mlen = strlen(errMsg);
    if (mlen >= errMsgSize) mlen = errMsgSize - 1;
    while (mlen > 0 && isspace(errMsg[mlen-1])) mlen--;
    errMsg[mlen] = '\0';
}
/***************************************************************/
char * os_get_message_str(long errCode) {

    static char osmsg[256];

    os_get_message(errCode, osmsg, sizeof(osmsg));

    return (osmsg);
}
/***************************************************************/
static int dynl_dynchecktrace(void) {
/*
** Set DYNTRACE: 0 - No tracing
**                 1 - Trace dynldlib() and dynsearchlibary()
**                 2 - Trace dynldlib(), dynsearchlibary() and dynloadproc()
**                 4 - Trace entry to dynload functions
*/

    static int dynl_dyntraceflag = 0;  /* -1 = off, 0 = unknown, 1 = on */
    char * tf;

    if (!dynl_dyntraceflag) {
        tf = getenv("DYNTRACE");
        if (tf && atol(tf) > 0) dynl_dyntraceflag = atol(tf);
        else                    dynl_dyntraceflag = -256;
    }

    return (dynl_dyntraceflag);
}
/***************************************************************/
static void dynl_dyntrace(   char * msg,
                        char * libproc,
                        void * result) {

    if (dynl_dynchecktrace() < 0) return;

    printf("DYNTRACE: %s: %-15s = 0x%lx\n", msg, libproc, (int)result);
}
/***************************************************************/
static void * dynl_dynldlib(struct dynl_info * dynl,
                    char * libname,
                    char* errmsg,
                    int errmsgmax) {
    void * lib;
    int ftyp;
    int ern;
    int llflags;

    llflags = 0;

#if defined(WIN32)
    llflags = LOAD_WITH_ALTERED_SEARCH_PATH;
    lib = LoadLibraryEx(libname, NULL, llflags);
    if (!lib) {
        ern = GetLastError();
        ftyp = os_ftype(libname);

        if (ftyp == OSFTYP_NONE) {
            sprintf(errmsg, "Cannot load missing library: %s\nError %d: %s",
                    libname, ern, os_get_message_str(ern));
        } else {
            sprintf(errmsg, "Cannot load existing library: %s\nError %d: %s",
                    libname, ern, os_get_message_str(ern));
        }

        if (dynl_dynchecktrace() > 0) {
            printf("LoadLibrary(%s) failed with error #%ld\n",
                    libname, GetLastError());
        }
        lib = 0;
    }

#elif defined(__hpux)
    int lflags;
    lflags = BIND_DEFERRED;
    lflags |= ((dynchecktrace() > 0) ? BIND_VERBOSE : 0);
    lib = shl_load(libname, lflags, 0L);
    if (!lib && dynchecktrace() > 0) {
      int ern;
      int ftyp;
      ern = errno;
      printf("DYNTRACE: shl_load(%s,%d) failed.\n",
                         libname, lflags);
      printf("DYNTRACE:    errno=%d: %s\n",
                         ern, strerror(ern));
      ftyp = os_ftype(libname);
      if (!ftyp) {
          printf("DYNTRACE:    stat() failed.\n");
      } else if (ftyp == OSFTYP_FILE) {
          printf("DYNTRACE:    stat() succeeded.\n");
      } else {
          printf("DYNTRACE:    ftype(%s)=%d\n",
                     libname, os_ftype(libname));
      }
    }
#elif USE_DLOPEN
    int lflags;
    lflags = RTLD_NOW;
    lib = dlopen(libname, lflags);

    if (!lib) {
        ern = errno;
        ftyp = os_ftype(libname);

#if HAS_DLERROR
        if (ftyp == OSFTYP_NONE) {
            sprintf(errmsg, "Cannot load missing library: %s\nDL Error: %s",
                    libname, dlerror());
        } else {
            sprintf(errmsg, "Cannot load existing library: %s\nDL Error: %s",
                    libname, dlerror());
        }
#else
        if (ftyp == OSFTYP_NONE) {
            sprintf(errmsg, "Cannot load missing library: %s\nError %d: %s",
                    libname, ern, strerror(ern));
        } else {
            sprintf(errmsg, "Cannot load existing library: %s\nError %d: %s",
                    libname, ern, strerror(ern));
        }
#endif

    if (!lib && dynchecktrace() > 0) {
        ern = errno;
        printf("DYNTRACE: dlopen(%s,%d) failed.\n",
                         libname, lflags);
        printf("DYNTRACE:    errno=%d: %s\n",
                         ern, strerror(ern));
        if (!ftyp) {
            printf("DYNTRACE:    stat() failed.\n");
        } else if (ftyp == 1) {
            printf("DYNTRACE:    stat() succeeded.\n");
#if HAS_DLERROR
            printf("DYNTRACE:    dlopen()->dlerror()=%s\n", dlerror());
#endif
      } else {
            printf("DYNTRACE:    ftype(%s)=%d\n",
                     libname, os_ftype(libname));
      }
    }
#else
    lib = 0;
#endif

    dynl_dyntrace("dynamic load library  ", libname, lib);

    return (lib);
}
/***************************************************************/
static void * dynl_dynsearchlibary(struct dynl_info * dynl,
                                void * library,
                                char * procname) {

    void * plabel;

#if defined(WIN32)
    plabel = GetProcAddress(library, procname);
#elif defined(__hpux)
    int result;

    result = shl_findsym((void*)(&library), procname, TYPE_PROCEDURE, &plabel);
    if (result) {
       plabel = 0;
    }

    if (!plabel && dynchecktrace() > 0) {
        int nsym;
        int jj;
        int kk;
        int mm;
        int nn;
        struct shl_symbol * syms;

        printf("shl_findsym(%s):  result=%d, errno=%d\n",
                procname, result, errno);

        nsym = shl_getsymbols(library, TYPE_PROCEDURE,
                              EXPORT_SYMBOLS, malloc, &syms);
        if (nsym < 0) {
            printf("shl_getsymbols error %d\n", errno);
        } else {
            nn = 0;
            printf("shl_getsymbols returned %d symbols.\n", nsym);
            fflush(stdout);
            for (jj = 0; jj < nsym; jj++) {
                kk = strlen(procname) - 1;
                mm = strlen(syms[jj].name) - 1;
                while (mm >= 0 && kk >= 0 &&
                       toupper(procname[kk]) ==
                       toupper(syms[jj].name[mm])) {
                    kk--;
                    mm--;
                }
                if (kk < 0) {
                    printf("Procedure %s(%hd) in library is similar to %s\n",
                           syms[jj].name, syms[jj].type, procname);
                    nn++;
                }
            }
            if (!nn) {
               printf("No procedures similar to %s found.\n",
                      procname);
            }
            free(syms);
        }
    }
#elif USE_DLOPEN
    plabel = dlsym(library, procname);
    #if HAS_DLERROR
        if (!plabel && dynchecktrace() > 0) {
             printf("DYNTRACE:    dlsym()->dlerror()=%s\n", dlerror());
        }
    #endif
#else
    return (0);
#endif

    dynl_dyntrace("dymamic load procedure", procname, plabel);

    return (plabel);
}
/***************************************************************/
static void * dynl_dynloadlibary(struct dynl_info * dynl,
                            char * libpath,
                            char * libname,
                            char* errmsg,
                            int errmsgmax) {
/*
**
*/
    char flibname[256];
    void * lib;
    char * vn;
    int ii;
    int jj;

    lib = 0;
    vn = getenv("WHLIBPATH");
    if (vn) {
        ii = 0;
        jj = 0;
        lib = 0;
        while (!lib && vn[ii]) {
            jj = 0;
            while (vn[ii] && vn[ii] != ';') {
                flibname[jj] = vn[ii];
                jj++;
                ii++;
            }
            flibname[jj] = '\0';
            if (vn[ii]) ii++; /* skip comma */
            if (jj) {
                if (flibname[jj-1] != DIRDELIM) {
                    flibname[jj] = DIRDELIM;
                    jj++;
                }
                strcpy(flibname + jj, libname);
                lib = dynl_dynldlib(dynl, flibname, errmsg, errmsgmax);
            }
        }
    }

    if (!lib) {
        if (libpath && libpath[0]) {
            jj = strlen(libpath);
            memcpy(flibname, libpath, jj);
            if (flibname[jj-1] != DIRDELIM) {
                flibname[jj] = DIRDELIM;
                jj++;
            }
            strcpy(flibname + jj, libname);
        } else {
            strcpy(flibname, libname);
        }
        lib = dynl_dynldlib(dynl, flibname, errmsg, errmsgmax);
    }

    return (lib);
}
/***************************************************************/
void * dynl_dynloadproc(struct dynl_info * dynl,
                    char * libpath,
                    char * libname,
                    char * procname,
                    int * err,
                    char* errmsg,
                    int errmsgmax) {
/*
** Returns procedure label of procedure.
**
**   Err:  0 - Success (plabel is 0 if failed previously for same procname)
**         1 - Error loading library
**         2 - Procedure not found in library
**         3 - Null libname argument
**         4 - Null procname argument
*/
    int found;
    int ix;
    void * libinfo;
    struct dynprocinfo * dpi;

    errmsg[0] = '\0';

    if (dynl_dynchecktrace() & 2) {
        printf("DYNTRACE2 dynloadproc: libpath=%s, libname=%s, procname=%s\n",
               libpath, libname, procname);
    }

    if (!libname && !libname[0]) {
        *err = 3;
        return (0);
    }

    if (!procname || !procname[0]) {
        *err = 4;
        return (0);
    }

    *err  = 0;
    libinfo = 0;
    found   = 0;
    ix = 0;
    while (!found && ix < dynl->dynl_ndynprocs) {
        if (dynl->dynl_dynprocs[ix] &&
            !strcmp(dynl->dynl_dynprocs[ix]->libpath, libpath) &&
            !strcmp(dynl->dynl_dynprocs[ix]->libname, libname) ) {
            if (!libinfo) libinfo = dynl->dynl_dynprocs[ix]->libinfo;
            if (!strcmp(dynl->dynl_dynprocs[ix]->procname, procname)) found = 1;
        }
        if (!found) ix++;
    }

    if (found) return (dynl->dynl_dynprocs[ix]->plabel);

    if (!libinfo) {
        libinfo = dynl_dynloadlibary(dynl, libpath, libname, errmsg, errmsgmax);
        if (dynl_dynchecktrace() & 2) {
            if (libinfo) printf("dynl_dynloadproc(%s) succeeded\n", libname);
            else         printf("dynl_dynloadproc(%s) failed\n", libname);
        }
        if (!libinfo) {
            *err = 1;
            return (0);
        }
    }

    dpi = New(struct dynprocinfo, 1);
    dynl->dynl_dynprocs = Realloc(dynl->dynl_dynprocs, struct dynprocinfo *, dynl->dynl_ndynprocs + 1);
    dynl->dynl_dynprocs[dynl->dynl_ndynprocs] = dpi;
    dynl->dynl_ndynprocs += 1;

    if (libpath) dpi->libpath  = Strdup(libpath);
    if (libname) dpi->libname  = Strdup(libname);
    dpi->procname = Strdup(procname);
    dpi->libinfo  = libinfo;

    dpi->plabel = dynl_dynsearchlibary(dynl, libinfo, procname);
    if (!dpi->plabel) {
        *err = 2;
    }

    return (dpi->plabel);
}
/***************************************************************/
void dynl_dyntraceentry(struct dynl_info * dynl,
                   char * dynproc,
                   int    flags,
                   char * libhome,      /* flags & 8 */
                   char * libpath,      /* flags & 4 */
                   char * libname,      /* flags & 2 */
                   char * procname) {   /* flags & 1 */

    char lh[256];
    char ln[256];
    char lp[256];
    char pn[256];

    if (!(dynl_dynchecktrace() & 4)) return;

    if (libhome) {
      if (libhome[0]) strcpy(lh, libhome); else strcpy(lh, "(blank)");
    } else strcpy(lh, "(null)");

    if (libpath) {
      if (libpath[0]) strcpy(lp, libpath); else strcpy(lp, "(blank)");
    } else strcpy(lp, "(null)");

    if (libname) {
      if (libname[0]) strcpy(ln, libname); else strcpy(ln, "(blank)");
    } else strcpy(ln, "(null)");

    if (procname) {
      if (procname[0]) strcpy(pn, procname); else strcpy(pn, "(blank)");
    } else strcpy(pn, "(null)");

    if (flags & 4) {
        printf("DYNTRACE4 Entry %s:\n", dynproc);
        printf("    libhome =%s\n", lh);
        printf("    libpath =%s\n", lp);
        printf("    libname =%s\n", ln);
        printf("    procname=%s\n", pn);
    } else {
        printf("DYNTRACE4 Entry %s:\n", dynproc);
        printf("    libhome =%s\n", lh);
        printf("    libname =%s\n", ln);
        printf("    procname=%s\n", pn);
    }
}
/***************************************************************/
void * dynl_dynloadjvm( struct dynl_info * dynl,
                        char * libname,
                        char * procname,
                        int * err,
                        char* errmsg,
                        int errmsgmax) {
    void * plabel;

    errmsg[0] = '\0';

    dynl_dyntraceentry(dynl, "dynloadjvm", 0x0B, 0, 0, libname, procname);

    plabel = dynl_dynloadproc(dynl, NULL, libname, procname, err, errmsg, errmsgmax);

    return (plabel);
}
/***************************************************************/
struct dynl_info * dynl_dynloadnew(void) {

    struct dynl_info * dynl;

    dynl = New(struct dynl_info, 1);
    dynl->dynl_ndynprocs = 0;
    dynl->dynl_dynprocs  = NULL;
    dynl->dcf            = NULL;

    return (dynl);
}
/***************************************************************/
void dynl_dynloadfree(struct dynl_info * dynl) {

    int ix;
    struct dynprocinfo * dpi;

    if (!dynl) return;

    for (ix = 0; ix < dynl->dynl_ndynprocs; ix++) {
        dpi = dynl->dynl_dynprocs[ix];
        if (dpi) {
            Free(dpi->libpath);
            Free(dpi->libname);
            Free(dpi->procname);
            Free(dpi);
        }
    }
    Free(dynl->dynl_dynprocs);
    Free(dynl->dcf);
    Free(dynl);
}
/***************************************************************/
static int win_get_registry_jvm(char * reg_val, int reg_val_sz) {

    int srcherr;
    HKEY hKeyJRE;
    HKEY hKeyCurrVers;
    int rtn;
    DWORD ent_typ;
    LONG reg_val_len;
    char java_vers[128];

    srcherr = 0;

    reg_val[0] = '\0';
    rtn = RegOpenKey(HKEY_LOCAL_MACHINE,
        "SOFTWARE\\JavaSoft\\Java Runtime Environment", &hKeyJRE);
    if (rtn !=  ERROR_SUCCESS) {
        srcherr = 1;
    } else {
        reg_val_len = sizeof(java_vers);
        rtn = RegQueryValueEx(hKeyJRE,
            "CurrentVersion", NULL, &ent_typ, java_vers, &reg_val_len);

        if (rtn != ERROR_SUCCESS) {
            srcherr = 1;
            /* ReportRegistryCallError(rtn); */
        } else if (ent_typ != REG_SZ) {
            srcherr = 2;
        } else {
            rtn = RegOpenKey(hKeyJRE, java_vers, &hKeyCurrVers);
            if (rtn !=  ERROR_SUCCESS) {
                srcherr = 1;
            } else {
                reg_val_len = reg_val_sz;
                rtn = RegQueryValueEx(hKeyCurrVers,
                    "RuntimeLib", NULL, &ent_typ, reg_val, &reg_val_len);

                if (rtn != ERROR_SUCCESS) {
                    srcherr = 1;
                    /* ReportRegistryCallError(rtn); */
                } else if (ent_typ != REG_SZ) {
                    srcherr = 2;
                }
                RegCloseKey(hKeyCurrVers);
            }
        }

        RegCloseKey(hKeyJRE);
    }

    return (srcherr);
}
/***************************************************************/
int os_get_registry_jvm(char * reg_val, int reg_val_sz) {

    int regerr;

#ifdef _WIN32
    regerr = win_get_registry_jvm(reg_val, reg_val_sz);
#else
    reg_val[0] = '\0';
    regerr = -1;
#endif

    return (regerr);
}
/***************************************************************/
void calcpathlib(char * libhome,    /* in  */
                 char * libpath,    /* in  */
                 char * libname,    /* in  */
                 char * dflthome,   /* in  */
                 char * dfltpath,   /* in  */
                 char * dfltname,   /* in  */
                 char * lpath,      /* out */
                 char * lname) {    /* out */
/*
** libhome should be getenv("??")
** libpath should be getenv("WH??PATH")
** libname should be getenv("WH??LIB")
*/
    int lplen;
    char ldir[256];

    if (libhome && libhome[0]) {
        strcpy(lpath, libhome);
    } else {
        strcpy(lpath, dflthome);
    }

    lplen = strlen(lpath);
    if (lplen && lpath[lplen-1] == DIRDELIM) {
        lplen--;
        lpath[lplen] = '\0';
    }

    if (lpath[0]) {
        if (libpath  && libpath[0]) {
            strcpy(ldir, libpath);
        } else {
            strcpy(ldir, dfltpath);
        }

        if (ldir[0]) {
            if (ldir[0] != DIRDELIM) {
                lpath[lplen] = DIRDELIM;
                lplen++;
            }
            strcpy(lpath + lplen, ldir);
        }
    }

    if (!libname || !libname[0]) strcpy(lname, dfltname);
    else                         strcpy(lname, libname);
}
/***************************************************************/
/***************************************************************/
d_daynum ymd_to_daynum(int year, int month, int day) {
/*
** Day 0 - Friday December 31, 1999
**
**      #define CAL_YEAR_BIAS               6400
**      09/02/1752 --> 2977764
**      09/14/1752 --> 2977765
**      12/31/1752 --> 2977873
**      12/31/1755 --> 2978968
**      12/31/1799 --> 2995039
**      12/31/1999 --> 3068087
*/
    int dn;
    int tt;
    int adj;
    int byear;
    int quad;
    int yquad;
    int cent;
    int ycent;
    d_daynum daynum;

    adj = 0;

    if (year <= CAL_REVISION_YEAR) {
        byear = year + CAL_YEAR_BIAS;
        if (year < 0) byear++;

        dn = (byear / 4) * CAL_NDY4;
        tt = byear & 3;
        dn += (tt * 365);
        if (tt || month > 2) dn++;
        if (year == CAL_REVISION_YEAR) {
            if (month > CAL_REVISION_MONTH ||
                (month == CAL_REVISION_MONTH && day > CAL_REVISION_DAY)) {
                adj += CAL_REVISION_NDAYS;
            }
        }
    } else {
        byear = year - 1600;
        adj += CAL_REVISION_NDAYS;

        quad  = byear / 400;
        yquad = byear - (quad * 400);
        cent  = yquad / 100;
        ycent = yquad - (cent * 100);
        tt    = ycent & 3;

        if (!cent) {
            dn  =  (ycent / 4) * CAL_NDY4;
            dn += (tt * 365);
            if (tt || month > 2) dn++;
        } else if (ycent < 4) {
            dn  = 365 * ycent + 1;
        } else {
            dn  =  (ycent / 4) * CAL_NDY4;
            dn += (tt * 365);
            if (tt || month > 2) dn++;
        }

        dn += (quad * CAL_NDY400);
        dn += (cent * CAL_NDY100);
        dn += CAL_NDY1600_LY4;
        dn += CAL_YEAR_BIAS_DAYS;
        dn += 1;    /* for Feb 29, 1700 */
   }

    switch (month) {
        case  1:            break;  case  2: dn +=  31; break;
        case  3: dn +=  59; break;  case  4: dn +=  90; break;
        case  5: dn += 120; break;  case  6: dn += 151; break;
        case  7: dn += 181; break;  case  8: dn += 212; break;
        case  9: dn += 243; break;  case 10: dn += 273; break;
        case 11: dn += 304; break;  case 12: dn += 334; break;
        default:;
    }

    daynum = (dn + day - adj);

    return (daynum - CAL_JULIAN_DIFF);
}
/***************************************************************/
void daynum_to_ymd(d_daynum daynum, int * year, int * month, int * day) {

    int yy;
    int mm;
    int dd;
    int leap;
    int quad;
    int dquad;
    int cent;
    int dcent;
    d_daynum adaynum;

    adaynum  = daynum + CAL_JULIAN_DIFF;

    if (adaynum <= CAL_REVISION_DAY_NUMBER) {
        leap = 0;
        yy = ((adaynum-1) / CAL_NDY4);
        dd = (adaynum) - (yy * CAL_NDY4);
        yy = (yy * 4) - CAL_YEAR_BIAS;
             if (dd > 1096) { yy += 3; dd -= 1096; }
        else if (dd > 731)  { yy += 2; dd -= 731;  }
        else if (dd > 366)  { yy += 1; dd -= 366;  }
        else leap = 1;

             if (dd > 334 + leap) { mm = 12; dd -= (334 + leap); }
        else if (dd > 304 + leap) { mm = 11; dd -= (304 + leap); }
        else if (dd > 273 + leap) { mm = 10; dd -= (273 + leap); }
        else if (dd > 243 + leap) { mm =  9; dd -= (243 + leap); }
        else if (dd > 212 + leap) { mm =  8; dd -= (212 + leap); }
        else if (dd > 181 + leap) { mm =  7; dd -= (181 + leap); }
        else if (dd > 151 + leap) { mm =  6; dd -= (151 + leap); }
        else if (dd > 120 + leap) { mm =  5; dd -= (120 + leap); }
        else if (dd > 90  + leap) { mm =  4; dd -= (90  + leap); }
        else if (dd > 59  + leap) { mm =  3; dd -= (59  + leap); }
        else if (dd > 31        ) { mm =  2; dd -= (31        ); }
        else                      { mm =  1;                     }
    } else {
        adaynum -= CAL_YEAR_BIAS_DAYS;
        adaynum += CAL_REVISION_NDAYS;
        adaynum -= CAL_NDY1600_LY4;
        adaynum--;    /* for Feb 29, 1700 */
        quad   = (adaynum-1) / CAL_NDY400;
        dquad  = adaynum - (quad * CAL_NDY400);
        if (dquad <= CAL_NDY100_LY4) {
            leap = 0;
            yy = ((dquad-1) / CAL_NDY4);
            dd = (dquad) - (yy * CAL_NDY4);
            yy *= 4;
                 if (dd > 1096) { yy += 3; dd -= 1096; }
            else if (dd > 731)  { yy += 2; dd -= 731;  }
            else if (dd > 366)  { yy += 1; dd -= 366;  }
            else leap = 1;
            yy += 1600 + (400 * quad);
        } else {
            dquad  -= CAL_NDY100_LY4;
            cent    = (dquad - 1) / CAL_NDY100;
            dcent   = dquad - (cent * CAL_NDY100);
            leap    = 0;

            if (dcent <= CAL_NDY4_NLY) {
                yy = (dcent-1) / CAL_NDY1;
                dd = dcent - (yy * CAL_NDY1);
                yy += 1700 + (100 * cent) + (400 * quad);
            } else {
                dcent  -= CAL_NDY4_NLY;
                yy = (dcent-1) / CAL_NDY4;
                dd = dcent - (yy * CAL_NDY4);
                yy = 1704 + (yy * 4) + (100 * cent) + (400 * quad);
                     if (dd > 1096) { yy += 3; dd -= 1096; }
                else if (dd > 731 ) { yy += 2; dd -= 731 ; }
                else if (dd > 366 ) { yy += 1; dd -= 366 ; }
                else leap = 1;
            }
        }

             if (dd > 334 + leap) { mm = 12; dd -= (334 + leap); }
        else if (dd > 304 + leap) { mm = 11; dd -= (304 + leap); }
        else if (dd > 273 + leap) { mm = 10; dd -= (273 + leap); }
        else if (dd > 243 + leap) { mm =  9; dd -= (243 + leap); }
        else if (dd > 212 + leap) { mm =  8; dd -= (212 + leap); }
        else if (dd > 181 + leap) { mm =  7; dd -= (181 + leap); }
        else if (dd > 151 + leap) { mm =  6; dd -= (151 + leap); }
        else if (dd > 120 + leap) { mm =  5; dd -= (120 + leap); }
        else if (dd > 90  + leap) { mm =  4; dd -= (90  + leap); }
        else if (dd > 59  + leap) { mm =  3; dd -= (59  + leap); }
        else if (dd > 31        ) { mm =  2; dd -= (31        ); }
        else                      { mm =  1;                     }
    }

    *year  = yy;
    if (*year <= 0) (*year)--;
    *month = mm;
    *day   = dd;
}
/***************************************************************/
void timestamp_diff(struct timestamp * ts1,
                    struct timestamp * ts2,
                    struct timestamp * tsr) {
    
    tsr->ts_microsec = ts1->ts_microsec - ts2->ts_microsec;
    tsr->ts_second   = ts1->ts_second   - ts2->ts_second;
    tsr->ts_daynum   = ts1->ts_daynum   - ts2->ts_daynum;
    
    while (tsr->ts_microsec < 0) {
        tsr->ts_microsec += 1000000L;
        tsr->ts_second   -= 1;
    }
    
    while (tsr->ts_second < 0) {
        tsr->ts_second += 86400L;
        tsr->ts_daynum -= 1;
    }
}
/***************************************************************/
#if IS_WINDOWS
/***************************************************************/
void systemtime_to_timestamp(const SYSTEMTIME * systim, struct timestamp * ts) {

	ts->ts_daynum   = (int)ymd_to_daynum(systim->wYear, systim->wMonth, systim->wDay);
	ts->ts_second   = systim->wHour * 3600  + systim->wMinute * 60 + systim->wSecond;
    ts->ts_microsec = systim->wMilliseconds * 1000;
}
/***************************************************************/
void filetime_to_timestamp(const FILETIME * filtim, struct timestamp * ts) {

	SYSTEMTIME systim;
    FILETIME locfiltim;

    if (!FileTimeToLocalFileTime(filtim, &locfiltim)) {
        systim.wYear         = 1988;
        systim.wMonth        = 8;
        systim.wDay          = 8;
        systim.wHour         = 0;
        systim.wMinute       = 0;
        systim.wSecond       = 0;
        systim.wMilliseconds = 0;
    } else {
	    if (!FileTimeToSystemTime(&locfiltim, &systim)) {
            systim.wYear         = 1977;
            systim.wMonth        = 7;
            systim.wDay          = 7;
            systim.wHour         = 0;
            systim.wMinute       = 0;
            systim.wSecond       = 0;
            systim.wMilliseconds = 0;
	    }
    }
    systemtime_to_timestamp(&systim, ts);
}
/***************************************************************/
void filetimeticks_to_timestamp(const FILETIME * filtim, struct timestamp * ts) {

    int64 ft;

    ft = (int64)(filtim->dwHighDateTime) << 32 | (int64)(filtim->dwLowDateTime);
    ft /= 10; /* convert 100ns to microsecs */

    ts->ts_microsec = (int)(ft % 1000000L);
    ft /= 1000000L; /* convert microsecs to secs */

    ts->ts_second   = (int)(ft % 86400L);
    ts->ts_daynum   = (int)(ft / 86400L);
}
/***************************************************************/
void get_cpu_timer(struct proctimes * pt) {

	SYSTEMTIME locsystim;
    FILETIME creationTime;
    FILETIME exitTime;
    FILETIME kernelTime;
    FILETIME userTime;
/*
BOOL WINAPI GetProcessTimes(
  __in   HANDLE hProcess,
  __out  LPFILETIME lpCreationTime,
  __out  LPFILETIME lpExitTime,
  __out  LPFILETIME lpKernelTime,
  __out  LPFILETIME lpUserTime
);
HANDLE WINAPI GetCurrentProcess(void);
*/
    GetLocalTime(&locsystim);
    systemtime_to_timestamp(&locsystim, &(pt->pt_wall_time));

    if (!GetProcessTimes(GetCurrentProcess(),
            &creationTime, &exitTime, &kernelTime, &userTime)) {
        memset(&(pt->pt_cpu_time), 0, sizeof(struct timestamp));
        memset(&(pt->pt_sys_time), 0, sizeof(struct timestamp));
    } else {
        filetimeticks_to_timestamp(&userTime, &(pt->pt_cpu_time));
        filetimeticks_to_timestamp(&kernelTime, &(pt->pt_sys_time));
    }

    if (pt->pt_use_hi_p) {
        LARGE_INTEGER hiprec_counter;
        if (!QueryPerformanceCounter(&hiprec_counter)) {
            pt->pt_use_hi_p = 0;
        } else {
            memcpy(&(pt->pt_hi_p), &(hiprec_counter.QuadPart), sizeof(hiprec_counter.QuadPart));
        }
#if IS_DEBUG
    if (sizeof(hiprec_counter.QuadPart) != sizeof(int64)) {
        fprintf(stderr, "** Timer size mismatch **\n"); fflush(stderr);
    }
#endif
    }

}
/***************************************************************/
int get_frequency_hi_timer(int64 * hi_freq) {

	int rtn;
    LARGE_INTEGER hiprec_freq;

	rtn = 0;

    if (!QueryPerformanceFrequency(&hiprec_freq)) {
        rtn = -1;
    } else {
    	(*hi_freq) = hiprec_freq.QuadPart;
   }

	return (rtn);
}
#else
/***************************************************************/
#if IS_MACINTOSH
/***************************************************************/
int get_hi_timespec(struct timespec * ts) {

    clock_serv_t cclock;
    mach_timespec_t mts;

    host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);
    ts->tv_sec = mts.tv_sec;
    ts->tv_nsec = mts.tv_nsec;
    
    return (0);
}
#else
int get_hi_timespec(struct timespec * ts) {

    return (clock_gettime(CLOCK_MONOTONIC, ts));
}
#endif
/***************************************************************/
void timeval_to_timestamp(const struct timeval * tv, struct timestamp * ts) {
    
	ts->ts_daynum   = (Int)((tv->tv_sec / SECS_IN_DAY) + TIME_D_DATE_DAYS_DIFF);
	ts->ts_second   = tv->tv_sec % SECS_IN_DAY;
    ts->ts_microsec = tv->tv_usec;
}
/***************************************************************/
void get_cpu_timer(struct proctimes * pt) {

	struct timeval tv;
	struct rusage pusage;

	if (!gettimeofday(&tv, NULL)) {
		timeval_to_timestamp(&tv, &(pt->pt_wall_time));
	} else {
		memset(&(pt->pt_wall_time), 0, sizeof(struct timestamp));
	}

	if (!getrusage(RUSAGE_SELF, &pusage)) {
		timeval_to_timestamp(&(pusage.ru_utime), &(pt->pt_cpu_time));
		timeval_to_timestamp(&(pusage.ru_stime), &(pt->pt_sys_time));
	} else {
		memset(&(pt->pt_cpu_time), 0, sizeof(struct timestamp));
		memset(&(pt->pt_sys_time), 0, sizeof(struct timestamp));
	}

    if (pt->pt_use_hi_p) {
    	 struct timespec ts;
    	 /* clock_gettime(CLOCK_REALTIME, &ts); */
    	 if (!get_hi_timespec(&ts)) {
    	     pt->pt_hi_p = (int64)(1000000000L) * (int64)(ts.tv_sec) + (int64)(ts.tv_nsec);
    	 } else {
    		 pt->pt_hi_p = 0;
    	 }
    }
}
/***************************************************************/
int get_frequency_hi_timer(int64 * hi_freq) {
/*
 * To check precision of timer
 * :
 *     struct timespec getres_freq;
 *     clock_getres(CLOCK_MONOTONIC, &getres_freq);
 */

    (*hi_freq) = 1000000000LL; /* clock_gettime(CLOCK_MONOTONIC) is always nanosecs */

	return (0);
}
#endif /* IS_WINDOWS */
/***************************************************************/
void start_cpu_timer(struct proctimes * pt_start, int use_hi_p) {

    pt_start->pt_use_hi_p = use_hi_p;

    get_cpu_timer(pt_start);
}
/***************************************************************/
void stop_cpu_timer(struct proctimes * pt_start,
                    struct proctimes * pt_end,
                    struct proctimes * pt_result) {

	int64 hi_freq;

	pt_end->pt_use_hi_p = pt_start->pt_use_hi_p;
    get_cpu_timer(pt_end);

    timestamp_diff(&(pt_end->pt_wall_time), &(pt_start->pt_wall_time), &(pt_result->pt_wall_time));
    timestamp_diff(&(pt_end->pt_cpu_time) , &(pt_start->pt_cpu_time) , &(pt_result->pt_cpu_time));
    timestamp_diff(&(pt_end->pt_sys_time) , &(pt_start->pt_sys_time) , &(pt_result->pt_sys_time));

    pt_result->pt_use_hi_p = 0;
    if (pt_start->pt_use_hi_p && pt_end->pt_use_hi_p) {
        pt_result->pt_hi_p = pt_end->pt_hi_p - pt_start->pt_hi_p;
        if (get_frequency_hi_timer(&hi_freq)) {
            pt_result->pt_use_hi_p = 0;
        } else {
            pt_result->pt_use_hi_p = 1;

            pt_result->pt_hi_elap_secs =
                (double)(pt_result->pt_hi_p) / (double)(hi_freq);
        }
    }
}
/***************************************************************/
