/***************************************************************/
/*
**  utl.c --  Utilities
*/
/***************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if defined(_Windows) || defined(WIN32)
    #define WIN32_LEAN_AND_MEAN
    #include <Windows.h>
#endif

#include "dew.h"
#include "utl.h"
#include "snprfh.h"

/***************************************************************/
void strmcpy(char * tgt, const char * src, size_t tmax) {
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
char * os_get_message_str(long errCode) {

    static char osmsg[MSGOUT_SIZE];

    os_get_message(errCode, osmsg, sizeof(osmsg));

    return (osmsg);
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
        tf = get_dewvar("DYNTRACE");
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
    int ern;
    int llflags;

    llflags = 0;

#if defined(WIN32)
    llflags = LOAD_WITH_ALTERED_SEARCH_PATH;
    lib = LoadLibraryEx(libname, NULL, llflags);
    if (!lib) {
        ern = GetLastError();

        sprintf(errmsg, "Cannot load existing library: %s\nError %d: %s",
                libname, ern, os_get_message_str(ern));

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
    vn = get_dewvar("DEWLIBPATH");
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
            jj = IStrlen(libpath);
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

    lplen = IStrlen(lpath);
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
void append_chars(char ** cstr,
                    int * slen,
                    int * smax,
                    const char * val,
                    int vallen)
{
/*
**
*/

    if ((*slen) + vallen >= (*smax)) {
        if (!(*smax)) (*smax) = 16 + vallen;
        else         (*smax)  = ((*smax) * 2) + vallen;
        (*cstr) = Realloc(*cstr, char, (*smax));
    }
    memcpy((*cstr) + (*slen), val, vallen);
    (*slen) += vallen;
    (*cstr)[*slen] = '\0';
}
/***************************************************************/
int32 get_utf8char (const unsigned char * text, int textlen, int * ix) {

/*
**  See: https://en.wikipedia.org/wiki/UTF-8
**  See: http://www.fileformat.info/info/unicode/utf8.htm
*/
    int32 utfchar;
    int code_unit1;
    int code_unit2;
    int code_unit3;
    int code_unit4;

    utfchar = 0;
    if ((*ix) < textlen) {
        code_unit1 = text[(*ix)++];
        if (code_unit1 < 0x80) {
            utfchar = code_unit1;
        } else if (code_unit1 < 0xC2) {
            /* continuation or overlong 2-byte sequence */
            /* goto ERROR1; */
        } else if (code_unit1 < 0xE0) {
            if ((*ix) < textlen) {
                /* 2-byte sequence */
                code_unit2 = text[(*ix)++];
                if ((code_unit2 & 0xC0) != 0x80) {
                    /* goto ERROR2; */
                } else {
                    utfchar = (code_unit1 << 6) + code_unit2 - 0x3080;
                }
            }
        } else if (code_unit1 < 0xF0) {
            if ((*ix) + 1 < textlen) {
                /* 3-byte sequence */
                code_unit2 = text[(*ix)++];
                if ((code_unit2 & 0xC0) != 0x80) {
                    /* goto ERROR2; */
                } else {
                    if (code_unit1 == 0xE0 && code_unit2 < 0xA0) {
                        /* goto ERROR2; */ /* overlong */
                    } else {
                        code_unit3 = text[(*ix)++];
                        if ((code_unit3 & 0xC0) != 0x80) {
                            /* goto ERROR3; */
                        } else {
                            utfchar =
                                (code_unit1 << 12) + (code_unit2 << 6) +
                                code_unit3 - 0xE2080;
                        }
                    }
                }
            }
        } else if (code_unit1 < 0xF5) {
            if ((*ix) + 2 < textlen) {
                /* 4-byte sequence */
                code_unit2 = text[(*ix)++];
                if ((code_unit2 & 0xC0) != 0x80) {
                    /* goto ERROR2; */
                } else {
                    if (code_unit1 == 0xF0 && code_unit2 < 0x90) {
                        /* goto ERROR2; */ /* overlong */
                    } else {
                        if (code_unit1 == 0xF4 && code_unit2 >= 0x90) {
                            /* goto ERROR2; */ /* > U+10FFFF */
                        } else {
                            code_unit3 = text[(*ix)++];
                            if ((code_unit3 & 0xC0) != 0x80) {
                                /* goto ERROR3; */
                            } else {
                                code_unit4 = text[(*ix)++];
                                if ((code_unit4 & 0xC0) != 0x80) {
                                    /* goto ERROR4; */
                                } else {
                                    utfchar = (code_unit1 << 18) +
                                              (code_unit2 << 12) +
                                              (code_unit3 << 6)  +
                                              code_unit4 - 0x3C82080;
                                }
                            }
                        }
                    }
                }
            }
        } else {
            /* > U+10FFFF */
            /* goto ERROR1; */
        }
    }

    return (utfchar);
}
/***************************************************************/
int strlen_of_utf8_chars(const char *utf8buf)
{
    int ix;
    int len;

    ix = 0;
    len = 0;
    while (utf8buf[ix]) {
        len += 1;
        if ((unsigned char)utf8buf[ix] < 0x80) {
            ix += 1;
        } else if ((unsigned char)utf8buf[ix] < 0xE0) {
            ix += 1;
            if ((unsigned char)utf8buf[ix] & 0x80) ix += 1;
        } else if ((unsigned char)utf8buf[ix] < 0xF0) {
            ix += 1;
            if ((unsigned char)utf8buf[ix] & 0x80) ix += 1;
            if ((unsigned char)utf8buf[ix] & 0x80) ix += 1;
        } else {
            ix += 1;
            if ((unsigned char)utf8buf[ix] & 0x80) ix += 1;
            if ((unsigned char)utf8buf[ix] & 0x80) ix += 1;
            if ((unsigned char)utf8buf[ix] & 0x80) ix += 1;
        }
    }

    return (len);
}
/***************************************************************/
int put_utf8char (int32 utfchar, unsigned char * dest) {
/*
** Returns number of bytes put into dest
** See also: Html_token_to_utf8()
*/
    int dlen;

    dlen = 0;
    if (utfchar < 0x80) {
        dest[dlen++] = (unsigned char)(utfchar & 0xFF);
    } else if (utfchar <= 0x7FF) {
        dest[dlen++] = (unsigned char)((utfchar >> 6) + 0xC0);
        dest[dlen++] = (unsigned char)((utfchar & 0x3F) + 0x80);
    } else if (utfchar <= 0xFFFF) {
        dest[dlen++] = (unsigned char)((utfchar >> 12) + 0xE0);
        dest[dlen++] = (unsigned char)(((utfchar >> 6) & 0x3F) + 0x80);
        dest[dlen++] = (unsigned char)((utfchar & 0x3F) + 0x80);
    } else if (utfchar <= 0x10FFFF) {
        dest[dlen++] = (unsigned char)((utfchar >> 18) + 0xF0);
        dest[dlen++] = (unsigned char)(((utfchar >> 12) & 0x3F) + 0x80);
        dest[dlen++] = (unsigned char)(((utfchar >> 6) & 0x3F) + 0x80);
        dest[dlen++] = (unsigned char)((utfchar & 0x3F) + 0x80);
    } else {
        /* error("invalid code_point"); */
    }

    return (dlen);
}
/***************************************************************/
int htmlctochar(char * tgt,
            const HTMLC * src,
            int srclen,
             int tgtmax)
{
/*
**  Use this function which puts to UTF-8 characters.
*/

    int ii;
    int tgtlen;

    tgtlen = 0;

    for (ii = 0; ii < srclen; ii++) {
        if (tgtlen + 4 < tgtmax) {
            tgtlen += put_utf8char((int32)(src[ii]),
                (unsigned char *)tgt + tgtlen);
        }
    }

    /* Append null terminator */
    if (tgtlen == tgtmax) tgtlen--;
    tgt[tgtlen] = '\0';

    return (tgtlen);
}
/***************************************************************/
int htmlclen(const HTMLC * hbuf)
{

    int len;

    if (!hbuf) return (0);

    len = 0;
    while (hbuf[len]) len++;

    return (len);
}
/***************************************************************/
int htmlcstrbytes(const HTMLC * hbuf)
{

    int len;

    if (!hbuf) return (0);

    len = 0;
    while (hbuf[len]) len++;

    return (HTMLC_BYTES(len + 1));
}
/***************************************************************/
void qhtmlctostr(char * tgt, const HTMLC * src, int tgtlen)
{

    int ii;
    for (ii = 0; ii < tgtlen && src[ii]; ii++) {
        if (src[ii] > 254) tgt[ii] = (unsigned char)HTMLCOVFL;
        else               tgt[ii] = (unsigned char)src[ii];
    }
    if (ii < tgtlen) tgt[ii] = 0; else tgt[tgtlen-1] = 0;
}
/***************************************************************/
void strtohtmlc(HTMLC * tgt, const char * src, int tgtlen)
{

    int ii;
    for (ii = 0; ii < tgtlen && src[ii]; ii++)
        tgt[ii] = (unsigned char)src[ii];
    if (ii < tgtlen) tgt[ii] = 0; else tgt[tgtlen-1] = 0;
}
/***************************************************************/
HTMLC * htmlcstrdup(const HTMLC * hbuf)
{
    int len;
    HTMLC * out;

    if (!hbuf) return (0);

    len = htmlclen(hbuf) + 1;

    out = New(HTMLC, len);
    memcpy(out, hbuf, HTMLC_BYTES(len));

    return (out);
}
/***************************************************************/
/* URIs                                                        */
/***************************************************************/
#define URC_CTL     1   /* control */
#define URC_ALNU    2   /* alphanumeric */
#define URC_SCHE    4   /* scheme */
#define URC_RESE    8   /* reserved */
#define URC_UNRE    16  /* unreserved */

/* Reserved URI Characters:  ! * ' ( ) ; : @ & = + $ , / ? # [ ] */
static unsigned char uri_charlist[128] = {
/* 00  ^@ */ 0                  , /* 01  ^A */ URC_CTL            ,
/* 02  ^B */ URC_CTL            , /* 03  ^C */ URC_CTL            ,
/* 04  ^D */ URC_CTL            , /* 05  ^E */ URC_CTL            ,
/* 06  ^F */ URC_CTL            , /* 07  ^G */ URC_CTL            ,
/* 08  ^H */ URC_CTL            , /* 09  ^I */ URC_CTL            ,
/* 0A  ^J */ URC_CTL            , /* 0B  ^K */ URC_CTL            ,
/* 0C  ^L */ URC_CTL            , /* 0D  ^M */ URC_CTL            ,
/* 0E  ^N */ URC_CTL            , /* 0F  ^O */ URC_CTL            ,
/* 10  ^P */ URC_CTL            , /* 11  ^Q */ URC_CTL            ,
/* 12  ^R */ URC_CTL            , /* 13  ^S */ URC_CTL            ,
/* 14  ^T */ URC_CTL            , /* 15  ^U */ URC_CTL            ,
/* 16  ^V */ URC_CTL            , /* 17  ^W */ URC_CTL            ,
/* 18  ^X */ URC_CTL            , /* 19  ^Y */ URC_CTL            ,
/* 1A  ^Z */ URC_CTL            , /* 1B     */ URC_CTL            ,
/* 1C     */ URC_CTL            , /* 1D     */ URC_CTL            ,
/* 1E     */ URC_CTL            , /* 1F     */ URC_CTL            ,
/* 20 ' ' */ URC_CTL            , /* 21 '!' */ URC_RESE           ,
/* 22 '"' */ URC_UNRE           , /* 23 '#' */ URC_RESE           ,
/* 24 '$' */ URC_RESE           , /* 25 '%' */ URC_UNRE           ,
/* 26 '&' */ URC_RESE           , /* 27 ''' */ URC_RESE           ,
/* 28 '(' */ URC_RESE           , /* 29 ')' */ URC_RESE           ,
/* 2A '*' */ URC_RESE           , /* 2B '+' */ URC_SCHE | URC_RESE,
/* 2C ',' */ URC_RESE           , /* 2D '-' */ URC_SCHE | URC_UNRE,
/* 2E '.' */ URC_SCHE | URC_UNRE, /* 2F '/' */ URC_RESE           ,
/* 30 '0' */ URC_ALNU           , /* 31 '1' */ URC_ALNU           ,
/* 32 '2' */ URC_ALNU           , /* 33 '3' */ URC_ALNU           ,
/* 34 '4' */ URC_ALNU           , /* 35 '5' */ URC_ALNU           ,
/* 36 '6' */ URC_ALNU           , /* 37 '7' */ URC_ALNU           ,
/* 38 '8' */ URC_ALNU           , /* 39 '9' */ URC_ALNU           ,
/* 3A ':' */ URC_RESE           , /* 3B ';' */ URC_RESE           ,
/* 3C '<' */ URC_UNRE           , /* 3D '=' */ URC_RESE           ,
/* 3E '>' */ URC_UNRE           , /* 3F '?' */ URC_RESE           ,
/* 40 '@' */ URC_RESE           , /* 41 'A' */ URC_ALNU           ,
/* 42 'B' */ URC_ALNU           , /* 43 'C' */ URC_ALNU           ,
/* 44 'D' */ URC_ALNU           , /* 45 'E' */ URC_ALNU           ,
/* 46 'F' */ URC_ALNU           , /* 47 'G' */ URC_ALNU           ,
/* 48 'H' */ URC_ALNU           , /* 49 'I' */ URC_ALNU           ,
/* 4A 'J' */ URC_ALNU           , /* 4B 'K' */ URC_ALNU           ,
/* 4C 'L' */ URC_ALNU           , /* 4D 'M' */ URC_ALNU           ,
/* 4E 'N' */ URC_ALNU           , /* 4F 'O' */ URC_ALNU           ,
/* 50 'P' */ URC_ALNU           , /* 51 'Q' */ URC_ALNU           ,
/* 52 'R' */ URC_ALNU           , /* 53 'S' */ URC_ALNU           ,
/* 54 'T' */ URC_ALNU           , /* 55 'U' */ URC_ALNU           ,
/* 56 'V' */ URC_ALNU           , /* 57 'W' */ URC_ALNU           ,
/* 58 'X' */ URC_ALNU           , /* 59 'Y' */ URC_ALNU           ,
/* 5A 'Z' */ URC_ALNU           , /* 5B '[' */ URC_RESE           ,
/* 5C '\' */ URC_UNRE           , /* 5D ']' */ URC_RESE           ,
/* 5E '^' */ URC_UNRE           , /* 5F '_' */ URC_UNRE           ,
/* 60 '`' */ URC_UNRE           , /* 61 'a' */ URC_ALNU           ,
/* 62 'b' */ URC_ALNU           , /* 63 'c' */ URC_ALNU           ,
/* 64 'd' */ URC_ALNU           , /* 65 'e' */ URC_ALNU           ,
/* 66 'f' */ URC_ALNU           , /* 67 'g' */ URC_ALNU           ,
/* 68 'h' */ URC_ALNU           , /* 69 'i' */ URC_ALNU           ,
/* 6A 'j' */ URC_ALNU           , /* 6B 'k' */ URC_ALNU           ,
/* 6C 'l' */ URC_ALNU           , /* 6D 'm' */ URC_ALNU           ,
/* 6E 'n' */ URC_ALNU           , /* 6F 'o' */ URC_ALNU           ,
/* 70 'p' */ URC_ALNU           , /* 71 'q' */ URC_ALNU           ,
/* 72 'r' */ URC_ALNU           , /* 73 's' */ URC_ALNU           ,
/* 74 't' */ URC_ALNU           , /* 75 'u' */ URC_ALNU           ,
/* 76 'v' */ URC_ALNU           , /* 77 'w' */ URC_ALNU           ,
/* 78 'x' */ URC_ALNU           , /* 79 'y' */ URC_ALNU           ,
/* 7A 'z' */ URC_ALNU           , /* 7B '{' */ URC_UNRE           ,
/* 7C '|' */ URC_UNRE           , /* 7D '}' */ URC_UNRE          ,
/* 7E '~' */ URC_UNRE           , /* 7F     */ URC_CTL
};
/***************************************************************/
struct uri * uri_new(int uri_scheme) {

    struct uri * urli;

    urli = New(struct uri, 1);
    urli->uri_scheme    = uri_scheme;
    urli->uri_errnum    = 0;
    urli->uri_user      = NULL;
    urli->uri_pass      = NULL;
    urli->uri_host      = NULL;
    urli->uri_port      = 0;
    urli->uri_raw_path  = NULL;

    return (urli);
}
/***************************************************************/
void uri_free(struct uri * urli) {

    if (!urli) return;

    Free(urli->uri_user);
    Free(urli->uri_pass);
    Free(urli->uri_host);
    Free(urli->uri_raw_path);
    Free(urli);
}
/***************************************************************/
struct uri * uri_dup(struct uri * urli) {

    struct uri * n_urli;

    n_urli = uri_new(urli->uri_scheme);
    n_urli->uri_errnum    = urli->uri_errnum;
    n_urli->uri_port      = urli->uri_port;

    if (urli->uri_user)     n_urli->uri_user     = Strdup(urli->uri_user);
    if (urli->uri_pass)     n_urli->uri_pass     = Strdup(urli->uri_pass);
    if (urli->uri_host)     n_urli->uri_host     = Strdup(urli->uri_host);
    if (urli->uri_raw_path) n_urli->uri_raw_path = Strdup(urli->uri_raw_path);

    return (n_urli);
}
/***************************************************************/
int uri_get_errnum(struct uri * urli) {

    if (!urli) return (URI_ERR_NO_URI);

    return (urli->uri_errnum);
}
/***************************************************************/
void uri_set_path(struct uri * urli, const char * uri_path) {

    Free(urli->uri_raw_path);
    urli->uri_raw_path = NULL;

    if (uri_path) urli->uri_raw_path = Strdup(uri_path);
}
/***************************************************************/
static void uri_fixitm(char * urlitm)
{
/*
** See: https://en.wikipedia.org/wiki/Percent-encoding
*/
    int ii;
    int jj;
    unsigned char ch1;
    unsigned char ch2;

    ii = 0;
    jj = 0;
    while (urlitm[ii]) {
        if (urlitm[ii] == '%' && urlitm[ii+1] && urlitm[ii+2]) {
            ch1 = urlitm[ii+1];
            if (ch1 >= '0' && ch1 <= '9')      ch1 -= '0';
            else if (ch1 >= 'A' && ch1 <= 'F') ch1 -= '7';
            else if (ch1 >= 'a' && ch1 <= 'f') ch1 -= 'W';
            ch2 = urlitm[ii+2];
            if (ch2 >= '0' && ch2 <= '9')      ch2 -= '0';
            else if (ch2 >= 'A' && ch2 <= 'F') ch2 -= '7';
            else if (ch2 >= 'a' && ch2 <= 'f') ch2 -= 'W';
            urlitm[jj++] = (ch1 << 4) | ch2;
            ii += 3;
        } else {
            urlitm[jj++] = urlitm[ii++];
        }
    }
    urlitm[jj] = '\0';
}
/***************************************************************/
int uri_get_scheme(const char * uri_str, int * uix)
{
    int scheme;
    int ix;
    char scheme_str[URI_SCHEME_MAX + 2];

    scheme = 0;
    ix = 0;

    /* scheme */
    /*  "any combination of letters, digits, plus, period, or hyphen" */
    while (ix <  URI_SCHEME_MAX && (uri_str[*uix] < 128) &&
            (uri_charlist[uri_str[*uix]] & (URC_ALNU | URC_SCHE))) {
        scheme_str[ix++] = tolower(uri_str[(*uix)++]);
    }
    if (uri_str[*uix] == ':') scheme_str[ix++] = uri_str[(*uix)++];
    scheme_str[ix] = '\0';

         if (!strcmp(scheme_str, "file:"  )) scheme = URI_SCHEME_FILE;
    else if (!strcmp(scheme_str, "http:"  )) scheme = URI_SCHEME_HTTP;
    else if (!strcmp(scheme_str, "https:" )) scheme = URI_SCHEME_HTTPS;
    else if (!strcmp(scheme_str, "ftp:"   )) scheme = URI_SCHEME_FTP;
    else if (!strcmp(scheme_str, "mailto:")) scheme = URI_SCHEME_MAILTO;
    else if (!strcmp(scheme_str, "urn:"   )) scheme = URI_SCHEME_URN;

    return (scheme);
}
/***************************************************************/
struct uri * uri_parse(const char * full_uri) {

/*
** URI: https://en.wikipedia.org/wiki/Uniform_Resource_Identifier
**
** Syntax:
**      scheme:[//[user[:password]@]host[:port]][/path][?query][#fragment]
**
** Examples:
**  ftp://user:password@host:25/path
**
**      RFC 3986 section 2.2 Reserved Characters (January 2005)
**          ! * ' ( ) ; : @ & = + $ , / ? # [ ]
**
*/
    struct uri * urli;
    int ix;
    int tix;
    int scheme;
    int port;
    int pathlen;
    char toke[URI_TOKEN_MAX + 2];

    port = 0;
    ix = 0;
    tix = 0;
    urli = NULL;

    scheme = uri_get_scheme(full_uri, &ix);
    if (!scheme) return (NULL);

    urli = uri_new(scheme);

    if (scheme == URI_SCHEME_FILE ||
        scheme == URI_SCHEME_MAILTO ||
        scheme == URI_SCHEME_URN) {
        urli->uri_raw_path = Strdup(full_uri + ix);

        return (urli);
    }

    if (full_uri[ix] == '/') ix++;
    else urli->uri_errnum = URI_ERR_EXP_SLASH_SLASH;

    if (!urli->uri_errnum) {
        if (full_uri[ix] == '/') ix++;
        else urli->uri_errnum = URI_ERR_EXP_SLASH_SLASH;
    }

    if (urli->uri_errnum) {
        return (urli);
    }

    tix = 0;
    while (tix <  URI_TOKEN_MAX && (full_uri[ix] < 128) &&
            (uri_charlist[full_uri[ix]] & (URC_ALNU | URC_UNRE))) {
        toke[tix++] = full_uri[ix++];
    }
    toke[tix] = '\0';

    if (full_uri[ix] == ':' || full_uri[ix] == '@') {
        urli->uri_user = New(char, tix + 1);
        memcpy(urli->uri_user, toke, tix + 1);
        uri_fixitm(urli->uri_user);
        ix++;
        if (full_uri[ix-1] == ':') {
            tix = 0;
            while (tix <  URI_TOKEN_MAX && (full_uri[ix] < 128) &&
                    (uri_charlist[full_uri[ix]] & (URC_ALNU | URC_UNRE))) {
                toke[tix++] = full_uri[ix++];
            }
            toke[tix] = '\0';
            urli->uri_pass = New(char, tix + 1);
            memcpy(urli->uri_pass, toke, tix + 1);
            uri_fixitm(urli->uri_pass);
            ix++;
        }

        tix = 0;
        while (tix <  URI_TOKEN_MAX && (full_uri[ix] < 128) &&
                (uri_charlist[full_uri[ix]] & (URC_ALNU | URC_UNRE))) {
            toke[tix++] = full_uri[ix++];
        }
        toke[tix] = '\0';
    }

    urli->uri_host = New(char, tix + 1);
    memcpy(urli->uri_host, toke, tix + 1);
    uri_fixitm(urli->uri_host);

    if (full_uri[ix] == ':') {
        tix = 0;
        while (isdigit(full_uri[ix])) {
            urli->uri_port = (urli->uri_port * 10) + (full_uri[ix] - '0');
            ix++;
        }
        /* validate port */
        if (urli->uri_port < 1 || urli->uri_port > 65535 ||
            full_uri[ix] >=  128 ||
            (full_uri[ix] && !(uri_charlist[full_uri[ix]] & URC_RESE))) {
            urli->uri_errnum = URI_ERR_BAD_PORT;
        }
    }

    /* The rest of it is the raw path */
    pathlen = IStrlen(full_uri + ix);
    if (pathlen) {
        urli->uri_raw_path = New(char, pathlen + 1);
        memcpy(urli->uri_raw_path, full_uri + ix, pathlen);
    }

    return (urli);
}
/***************************************************************/
int uri_pctenclen(const char * uritok)
{
    int enclen;
    int ix;

    enclen = 0;
    if (uritok) {
        ix = 0;
        while (uritok[ix]) {
            enclen += 1;
            if (uritok[ix] >= 128 ||
                    (uri_charlist[uritok[ix]] & (URC_CTL | URC_RESE))) {
                enclen += 2;
            }
            ix++;
        }
    }

    return (enclen);
}
/***************************************************************/
