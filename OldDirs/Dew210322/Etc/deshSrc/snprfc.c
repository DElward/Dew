/***************************************************************/
/*
**  snprf.c  --  Routines for snprintf and smprintf
*/
/***************************************************************/
/*
History:
03/17/04 DTE  0.0 First Version
03/24/04 DTE  0.1 Added support for NANs, INFs, etc.
03/25/04 DTE  0.2 Added va_arg casts for Linux.
04/19/04 DTE  0.3 Added va_arg casts for Linux.
04/19/11 DTE  0.4 Added L for PRINTF_I64
07/13/11 DTE  0.5 Added Fmprintf() and Vfmprintf()
07/13/11 DTE  0.6 Added , flag
04/01/12 DTE  0.7 Fixed erroneous CON_MIN_64 and CON_MAX_64
*/
/***************************************************************/


#define PRINTF_EXTENSIONS 1


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>
#include <wchar.h>

#if PRINTF_EXTENSIONS
    #ifdef WIN32
        #define WIN32_LEAN_AND_MEAN
        #include <Windows.h>
    #endif
#endif

#include "config.h"

#include "cmemh.h"
#include "snprfh.h"

#ifndef New
    #define New(t,n)         ((t*)calloc(sizeof(t),(n)))
    #define Realloc(p,t,n)   ((t*)realloc((p),sizeof(t)*(n)))
    #define Free(p)           free(p)
#endif

#define WCSLEN(x) wcslen(x)

/***************************************************************/
#define FP_ISQNAN(n)    (fptype(n) == FPTYP_QNAN)
#define FP_ISSNAN(n)    (fptype(n) == FPTYP_SNAN)
#define FP_ISNINF(n)    (fptype(n) == FPTYP_NINF)
#define FP_ISPINF(n)    (fptype(n) == FPTYP_PINF)
#define FP_ISNEG0(n)    (fptype(n) == FPTYP_NZ)

#define SNPFLAG_MINUS   1
#define SNPFLAG_PLUS    2
#define SNPFLAG_SPACE   4
#define SNPFLAG_ZERO    8
#define SNPFLAG_MOD_H   16
#define SNPFLAG_MOD_L   32
#define SNPFLAG_WIDTH   64
#define SNPFLAG_PREC    128
#define SNPFLAG_SFLT    256
#define SNPFLAG_UPPER   512
#define SNPFLAG_G_FMT   1024
#define SNPFLAG_MOD_LL  2048
#define SNPFLAG_COMMA   4096

#define FPTYP_SNAN   0x0001  /* signaling NaN */
#define FPTYP_QNAN   0x0002  /* quiet NaN */
#define FPTYP_NINF   0x0004  /* negative infinity */
#define FPTYP_NN     0x0008  /* negative normal */
#define FPTYP_ND     0x0010  /* negative denormal */
#define FPTYP_NZ     0x0020  /* -0 */
#define FPTYP_PZ     0x0040  /* +0 */
#define FPTYP_PD     0x0080  /* positive denormal */
#define FPTYP_PN     0x0100  /* positive normal */
#define FPTYP_PINF   0x0200  /* positive infinity */

#define SNPERR(en)      (en)

/***************************************************************/
#if PRINTF_EXTENSIONS
#ifdef WIN32
void win_get_message(long errCode, char * errMsg, int errMsgSize) {

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
void ux_get_message(int errCode, char * errMsg, int errMsgSize) {

    strncpy(errMsg, strerror(errCode), errMsgSize);
}
#endif
/***************************************************************/
void os_get_message(long errCode, char * errMsg, int errMsgSize) {

    int mlen;

#ifdef WIN32
    win_get_message(errCode, errMsg, errMsgSize);
#else
    ux_get_message((int)errCode, errMsg, errMsgSize);
#endif

    mlen = (int)strlen(errMsg);
    if (mlen >= errMsgSize) mlen = errMsgSize - 1;
    while (mlen > 0 && isspace(errMsg[mlen-1])) mlen--;
    errMsg[mlen] = '\0';
}
/***************************************************************/
char * get_os_message(long errCode) {

    char * msgbuf;
    int    msglen;

    msglen = 256;
    msgbuf = New(char, msglen);
    os_get_message(errCode, msgbuf, msglen);

    return (msgbuf);
}
/***************************************************************/
char * get_strerror(int erno) {

    char * msgbuf;
    int    msglen;

    msglen = 256;
    msgbuf = New(char, msglen);
    strcpy(msgbuf, strerror(erno));

    return (msgbuf);
}
#endif /* PRINTF_EXTENSIONS */
/***************************************************************/

/***************************************************************/
int fptype(double fp) {

    short int one = 1;
    int typ = 0;
    int exp;
    int sgn;
    int res;
    int zer;
    unsigned char zeroes[8] =  { 0, 0, 0, 0, 0, 0, 0, 0 };

    if (sizeof(short int) != 2 || sizeof(double) != 8) return (0);

    if (((unsigned char*)(&one))[1]) {  /* forward endian */
        exp = ((((unsigned char*)(&fp))[0]) & 0x7F) << 4 |
              ((((unsigned char*)(&fp))[1]) >> 4);
        sgn = ((unsigned char*)(&fp))[0] >> 7;
        res = ((unsigned char*)(&fp))[1] & 0x0F;
        zer = !res && !memcmp((unsigned char*)(&fp) + 2, zeroes, 6);
    } else {
        exp = ((((unsigned char*)(&fp))[7]) & 0x7F) << 4 |
              ((((unsigned char*)(&fp))[6]) >> 4);
        sgn = ((unsigned char*)(&fp))[7] >> 7;
        res = ((unsigned char*)(&fp))[6] & 0x0F;
        zer = !res && !memcmp((unsigned char*)(&fp), zeroes, 6);
    }

    if (!exp) {
        if (zer) {
            if (sgn) typ = FPTYP_NZ;
            else     typ = FPTYP_PZ;
        } else {
            if (sgn) typ = FPTYP_ND;
            else     typ = FPTYP_PD;
        }
    } else if (exp == 0x7FF) {
        if (zer) {
            if (sgn) typ = FPTYP_NINF;
            else     typ = FPTYP_PINF;
        } else {
            if (res & 0x08) typ = FPTYP_QNAN;
            else            typ = FPTYP_SNAN;
        }
    } else {
        if (sgn) typ = FPTYP_NN;
        else     typ = FPTYP_PN;
    }

    return (typ);
}
/***************************************************************/
static void vsxpplaces(
        void (*addchars)(void * info, const char *buf, int buflen),
        void * info,
        char * tstr,
        int flags,
        int width,
        int prec) {

    int len;
    int pad;
    int ii;
    char tchar;

    len = Istrlen(tstr);
    pad = 0;
    if ((flags & SNPFLAG_PREC) && len > prec) len = prec;
    if ((flags & SNPFLAG_WIDTH) && len < width) {
        pad = width - len;
        if (!(flags & SNPFLAG_MINUS)) {
            tchar = ' ';
            for (ii = 0; ii < pad; ii++) {
                addchars(info, &tchar, 1);
            }
            pad = 0;
        }
    }

    addchars(info, tstr, len);

    if (pad) {
        tchar = ' ';
        for (ii = 0; ii < pad; ii++) {
            addchars(info, &tchar, 1);
        }
    }
}
/***************************************************************/
static void vsxpplaceq(
        void (*addchars)(void * info, const char *buf, int buflen),
        void * info,
        char * tstr,
        int flags,
        int width,
        int prec) {
/*
** csv string
**      specials: <comma> <double qoute>
*/

    int len;
    int pad;
    int quote;
    int ii;
    int jj;
    char tchar;

    quote = 0;
    len = 0;
    for (ii = 0; tstr[ii]; ii++) {
        len++;
        if (tstr[ii] == ',') quote = 1;
        else if (tstr[ii] == '\"') len++;
    }
    if (quote) len += 2;

    pad = 0;
    if ((flags & SNPFLAG_PREC) && len > prec) len = prec;
    if ((flags & SNPFLAG_WIDTH) && len < width) {
        pad = width - len;
        if (!(flags & SNPFLAG_MINUS)) {
            tchar = ' ';
            for (ii = 0; ii < pad; ii++) {
                addchars(info, &tchar, 1);
            }
            pad = 0;
        }
    }

    if (quote) addchars(info, "\"", 1);
    ii = 0;
    while (tstr[ii]) {
        for (jj = ii; tstr[jj] && tstr[jj] != '\"'; jj++) ;
        if (tstr[jj]) {
            addchars(info, tstr + ii, jj - ii + 1);
            addchars(info, "\"", 1);
            ii = jj + 1;
        } else if (jj > ii) {
            addchars(info, tstr + ii, jj - ii);
            ii = jj;
        }
    }
    if (quote) addchars(info, "\"", 1);

    if (pad) {
        tchar = ' ';
        for (ii = 0; ii < pad; ii++) {
            addchars(info, &tchar, 1);
        }
    }
}
/***************************************************************/
static void vsxpplacewides(
        void (*addchars)(void * info, const char *buf, int buflen),
        void * info,
        wchar_t* wstr,
        int flags,
        int width,
        int prec) {

    int len;
    int pad;
    int ii;
    char tchar;

    len = (int)WCSLEN(wstr);
    pad = 0;
    if ((flags & SNPFLAG_PREC) && len > prec) len = prec;
    if ((flags & SNPFLAG_WIDTH) && len < width) {
        pad = width - len;
        if (!(flags & SNPFLAG_MINUS)) {
            tchar = ' ';
            for (ii = 0; ii < pad; ii++) {
                addchars(info, &tchar, 1);
            }
            pad = 0;
        }
    }

    for (ii = 0; ii < len; ii++) {
        wctomb(&tchar, wstr[ii]);
        addchars(info, &tchar, 1);
    }

    if (pad) {
        tchar = ' ';
        for (ii = 0; ii < pad; ii++) {
            addchars(info, &tchar, 1);
        }
    }
}
/***************************************************************/
#if PRINTF_I64
static void vsxpplacell(
        void (*addchars)(void * info, const char *buf, int buflen),
        void * info,
        char * nbuf,
        int nbufmax,
        INT64_T ti64,
        int flags,
        int width,
        int prec) {

    int nlen;
    int nix;
    int pad;
    char tchar;
    int ii;
    char sign;

#if PRINTF_EXTENSIONS
    int comma;

    if (flags & SNPFLAG_COMMA)      comma = 3;
#endif

    if (ti64 < 0)                   sign = '-';
    else if (flags & SNPFLAG_PLUS)  sign = '+';
    else if (flags & SNPFLAG_SPACE) sign = ' ';
    else                            sign = 0;


    if (!ti64) {
        if (!(flags & SNPFLAG_PREC) || prec) {
            nlen = 1;
            nix = nbufmax - nlen - 1;
            strcpy(nbuf + nix, "0");
        } else { /* "%.0d" is blank on zero */
            nlen = 0;
            nix = nbufmax - 1;
        }
    } else if (ti64 == CON_MIN_64) {
        nlen = 19;
        nix = nbufmax - nlen - 1;
        strcpy(nbuf + nix, "9223372036854775808");
    } else {
        nlen = 0;
        nix  = nbufmax;
        if (ti64 < 0) {
            ti64 = -ti64;
        }
        while (ti64) {
            nix--;
            nbuf[nix] = (char)(ti64 % CON_10_64) + '0';
            ti64 /= CON_10_64;
            nlen++;
#if PRINTF_EXTENSIONS
            if (flags & SNPFLAG_COMMA) {
                comma--;
                if (!comma && ti64) {
                    nix--;
                    nbuf[nix] = ',';
                    nlen++;
                    comma = 3;
                }
            }
#endif
        }
    }

    if (flags & (SNPFLAG_PREC | SNPFLAG_ZERO)) {
        if (!(flags & SNPFLAG_PREC)) {
            prec = width;
            if (sign) prec--;
        }
        while (nix > 0 && nlen < prec) {
            nix--;
            nbuf[nix] = '0';
            nlen++;
        }
    }

    if (sign) {
        nix--;
        nbuf[nix] = sign;
        nlen++;
    }

    pad = 0;
    if ((flags & SNPFLAG_WIDTH) && nlen < width) {
        pad = width - nlen;
        if (!(flags & SNPFLAG_MINUS)) {
            while (nix > 0 && nlen < width) {
                nix--;
                nbuf[nix] = ' ';
                nlen++;
            }
            pad = 0;
        }
    }

    addchars(info, nbuf + nix, nlen);

    if (pad) {
        tchar = ' ';
        for (ii = 0; ii < pad; ii++) {
            addchars(info, &tchar, 1);
        }
    }

}
#endif
/***************************************************************/
static void vsxpplaced(
        void (*addchars)(void * info, const char *buf, int buflen),
        void * info,
        char * nbuf,
        int nbufmax,
        long tlong,
        int flags,
        int width,
        int prec) {

    int nlen;
    int nix;
    int pad;
    char tchar;
    int ii;
    char sign;

#if PRINTF_EXTENSIONS
    int comma;

    if (flags & SNPFLAG_COMMA)      comma = 3;
#endif

    if (tlong < 0)                  sign = '-';
    else if (flags & SNPFLAG_PLUS)  sign = '+';
    else if (flags & SNPFLAG_SPACE) sign = ' ';
    else                            sign = 0;

    if (!tlong) {
        if (!(flags & SNPFLAG_PREC) || prec) {
            nlen = 1;
            nix = nbufmax - nlen - 1;
            strcpy(nbuf + nix, "0");
        } else { /* "%.0d" is blank on zero */
            nlen = 0;
            nix = nbufmax - 1;
        }
    } else if (tlong == 0x80000000l) {
        nlen = 10;
        nix = nbufmax - nlen - 1;
        strcpy(nbuf + nix, "2147483648");
    } else {
        nlen = 0;
        nix  = nbufmax;
        if (tlong < 0) {
            tlong = -tlong;
        }
        while (tlong) {
            nix--;
            nbuf[nix] = (tlong % 10) + '0';
            tlong /= 10;
            nlen++;
#if PRINTF_EXTENSIONS
            if (flags & SNPFLAG_COMMA) {
                comma--;
                if (!comma && tlong) {
                    nix--;
                    nbuf[nix] = ',';
                    nlen++;
                    comma = 3;
                }
            }
#endif
        }
    }

    if (flags & (SNPFLAG_PREC | SNPFLAG_ZERO)) {
        if (!(flags & SNPFLAG_PREC)) {
            prec = width;
            if (sign) prec--;
        }
        while (nix > 0 && nlen < prec) {
            nix--;
            nbuf[nix] = '0';
            nlen++;
        }
    }

    if (sign) {
        nix--;
        nbuf[nix] = sign;
        nlen++;
    }

    pad = 0;
    if ((flags & SNPFLAG_WIDTH) && nlen < width) {
        pad = width - nlen;
        if (!(flags & SNPFLAG_MINUS)) {
            while (nix > 0 && nlen < width) {
                nix--;
                nbuf[nix] = ' ';
                nlen++;
            }
            pad = 0;
        }
    }

    addchars(info, nbuf + nix, nlen);

    if (pad) {
        tchar = ' ';
        for (ii = 0; ii < pad; ii++) {
            addchars(info, &tchar, 1);
        }
    }

}
/***************************************************************/
static void vsxpplacex(
        void (*addchars)(void * info, const char *buf, int buflen),
        void * info,
        char * nbuf,
        int nbufmax,
        long tlong,
        int flags,
        int width,
        int prec,
        char ten) {

    int nlen;
    int nix;
    int pad;
    char tchar;
    int ii;
    int dig;

    if (!tlong) {
        if (!(flags & SNPFLAG_PREC) || prec) {
            nlen = 1;
            nix = nbufmax - nlen - 1;
            strcpy(nbuf + nix, "0");
        } else { /* "%.0d" is blank on zero */
            nlen = 0;
            nix = nbufmax - 1;
        }
    } else {
        nlen = 0;
        nix  = nbufmax;
        while (tlong && nlen < 8) {
            nix--;
            dig = tlong & 0x0F;
            if (dig < 10) nbuf[nix] = dig + '0';
            else          nbuf[nix] = ten + dig - 10;
            tlong >>= 4;
            nlen++;
        }
    }

    if (flags & SNPFLAG_PREC) {
        while (nix > 0 && nlen < prec) {
            nix--;
            nbuf[nix] = '0';
            nlen++;
        }

    } else if (flags & SNPFLAG_ZERO) {
        while (nix > 0 && nlen < width) {
            nix--;
            nbuf[nix] = '0';
            nlen++;
        }
    }

    pad = 0;
    if ((flags & SNPFLAG_WIDTH) && nlen < width) {
        pad = width - nlen;
        if (!(flags & SNPFLAG_MINUS)) {
            while (nix > 0 && nlen < width) {
                nix--;
                nbuf[nix] = ' ';
                nlen++;
            }
            pad = 0;
        }
    }

    addchars(info, nbuf + nix, nlen);

    if (pad) {
        tchar = ' ';
        for (ii = 0; ii < pad; ii++) {
            addchars(info, &tchar, 1);
        }
    }

}
/***************************************************************/
static void vsxpplaceo(
        void (*addchars)(void * info, const char *buf, int buflen),
        void * info,
        char * nbuf,
        int nbufmax,
        long tlong,
        int flags,
        int width,
        int prec) {

    int nlen;
    int nix;
    int pad;
    char tchar;
    int ii;

    if (!tlong) {
        if (!(flags & SNPFLAG_PREC) || prec) {
            nlen = 1;
            nix = nbufmax - nlen - 1;
            strcpy(nbuf + nix, "0");
        } else { /* "%.0d" is blank on zero */
            nlen = 0;
            nix = nbufmax - 1;
        }
    } else {
        nlen = 0;
        nix  = nbufmax;
        while (tlong && nlen < 10) {
            nix--;
            nbuf[nix] = (tlong & 0x07) + '0';
            tlong >>= 3;
            nlen++;
        }
        if (tlong) {
            nix--;
            nbuf[nix] = (tlong & 0x03) + '0';
            nlen++;
        }
    }

    if (flags & SNPFLAG_PREC) {
        while (nix > 0 && nlen < prec) {
            nix--;
            nbuf[nix] = '0';
            nlen++;
        }

    } else if (flags & SNPFLAG_ZERO) {
        while (nix > 0 && nlen < width) {
            nix--;
            nbuf[nix] = '0';
            nlen++;
        }
    }

    pad = 0;
    if ((flags & SNPFLAG_WIDTH) && nlen < width) {
        pad = width - nlen;
        if (!(flags & SNPFLAG_MINUS)) {
            while (nix > 0 && nlen < width) {
                nix--;
                nbuf[nix] = ' ';
                nlen++;
            }
            pad = 0;
        }
    }

    addchars(info, nbuf + nix, nlen);

    if (pad) {
        tchar = ' ';
        for (ii = 0; ii < pad; ii++) {
            addchars(info, &tchar, 1);
        }
    }

}
/***************************************************************/
static void fpround(char * nbuf,
                    int nbufmax,
                    int rix,
                    int *exp) {

    int nlen;
    int carry;
    int ii;

    nlen = Istrlen(nbuf);
    if (rix <= 0) {
        nbuf[1] = '0';
        nbuf[2] = '\0';
        return;
    }

    if (rix < nlen) {
        if (nbuf[rix] >= '5' && nbuf[rix] <= '9') {
            carry = 1;
            ii = rix - 1;
            while (carry && ii > 0) {
                if (nbuf[ii] == '9') {
                    nbuf[ii] = '0';
                    ii--;
                } else {
                    nbuf[ii]++;
                    carry = 0;
                }
            }
            if (carry) {
                if (nlen + 1 == nbufmax) nlen--;
                memmove(nbuf + 2, nbuf + 1, nlen - 1);
                nbuf[1] = '1';
                (*exp)++;
                rix++;
            }
        }
        nlen = rix;
    }

    /* strip trailing zeros */
    ii = nlen;
    while (ii > 1 && ii > *exp + 2 && nbuf[ii-1] == '0') {
        ii--;
    }
    nbuf[ii] = '\0';
}
/***************************************************************/
static void fpnorm(char * nbuf,
                  int nbufmax,
                  double fp,
                  int prec,
                  int * exp) {

    int nix;
    int nd;
    double fl;

    *exp = 0;

    if (nbufmax < 32) return;  /* requires at least 32 bytes */

    if (FP_ISSNAN(fp)) {
        strcpy(nbuf, " 1#SNAN");
        return;
    } else if (FP_ISQNAN(fp)) {
        strcpy(nbuf, " 1#QNAN");
        return;
    } else if (FP_ISNINF(fp)) {
        strcpy(nbuf, "-1#INF");
        return;
    } else if (FP_ISPINF(fp)) {
        strcpy(nbuf, " 1#INF");
        return;
    } else if (FP_ISPINF(fp)) {
        strcpy(nbuf, "-0");
        return;
    } else if (fp == 0) {
        strcpy(nbuf, " 0");
        return;
    }

    nix = 1;
    nd  = 0;
    if (fp < 0) {
        nbuf[0] = '-';
        fp = -fp;
    } else {
        nbuf[0] = ' ';
    }
    if (fp < 1.0) {
        while (fp < 1.0) {
            fp *= 10.0;
            (*exp)--;
        }

        while (fp && nix + 1 < nbufmax && nd <= prec) {
            fl = floor(fp);
            nbuf[nix] = (int)fl + '0';
            nix++;
            nd++;
            fp = (fp - fl) * 10.0;
        }
        nbuf[nix] = '\0';
    } else {
        fl = floor(fp);
        nix = nbufmax - 1;
        while (nix && fl) {
            nbuf[nix] = (int)fmod(fl, 10.0) + '0';
            fl = floor(fl / 10.0);
            nix--;
        }
        if (!nix) {
            strcpy(nbuf + 1, "1#INF");
            return;
        }

        (*exp) = nbufmax - (nix + 2);
        memcpy(nbuf + 1, nbuf + nix + 1, *exp + 1);
        nix = *exp + 2;

        fp -= floor(fp);
        while (fp && nix + 1 < nbufmax && nd <= prec) {
            fp *= 10.0;
            fl = floor(fp);
            nbuf[nix] = (int)fl + '0';
            nix++;
            nd++;
            fp -= fl;
        }
        nbuf[nix] = '\0';
    }
}
/***************************************************************/
static void vsxpplacee(
        void (*addchars)(void * info, const char *buf, int buflen),
        void * info,
        char * nbuf,
        int nbufmax,
        int exp,
        int flags,
        int width,
        int prec,
        char expch) {

    int nlen;
    int nn;
    int nix;
    int nwid;
    int pad;
    char tbuf[8];
    char sign;
    char ech;

    nlen = Istrlen(nbuf);
    while (nlen > 2 && nbuf[nlen-1] == '0') nlen--;
    nix = 1;
    strcpy(tbuf, "0. +-");

    if (nbuf[0] == '-')            sign = '-';
    else if (flags & SNPFLAG_PLUS) sign = '+';
    else                           sign = ' ';

    nwid = prec + 6;
    if (prec) nwid++;

    if (nbuf[0] == '-' || (flags & (SNPFLAG_PLUS | SNPFLAG_SPACE))) {
        nwid++;
        if (flags & SNPFLAG_ZERO) {
            addchars(info, &sign, 1);
        } else {
            flags |= SNPFLAG_SFLT;
        }
    }

    pad = 0;
    if (flags & SNPFLAG_WIDTH) {
        pad = width - nwid;
        if (flags & SNPFLAG_G_FMT) {
            pad += prec - nlen + 2;
        }

        if (flags & SNPFLAG_ZERO) {
            while (pad > 0) {
                addchars(info, tbuf, 1); /* zero */
                pad--;
            }
        } else if (!(flags & SNPFLAG_MINUS)) {
            while (pad > 0) {
                addchars(info, tbuf + 2, 1); /* space */
                pad--;
            }
        }
    }

    if (flags & SNPFLAG_SFLT) {
        addchars(info, &sign, 1);
    }

    addchars(info, nbuf + nix, 1);
    nix++;
    if (prec) {
        if (!(flags & SNPFLAG_G_FMT) || nix < nlen) {
            addchars(info, tbuf + 1, 1); /* point */
        } else {
            pad++;
        }
    }

    if (nlen - 2 >= prec) {
        addchars(info, nbuf + nix, prec);
    } else {
        if (nlen > 2) addchars(info, nbuf + nix, nlen - 2);

        if (!(flags & SNPFLAG_G_FMT)) {
            nn = prec - nlen + 2;
            while (nn) {
                addchars(info, tbuf, 1); /* zero */
                nn--;
            }
        }
    }

    addchars(info, &expch, 1);
    if (exp < 0) {
        addchars(info, tbuf + 4, 1);                /* '-' */
        exp = -exp;
    } else {
        addchars(info, tbuf + 3, 1);                /* '+' */
    }

    nn = exp / 100;
    ech = nn + '0';
    addchars(info, &ech, 1);
    exp -= nn * 100;

    nn = exp / 10;
    ech = nn + '0';
    addchars(info, &ech, 1);
    exp -= nn * 10;

    nn = exp;
    ech = nn + '0';
    addchars(info, &ech, 1);

    while (pad > 0) {
        addchars(info, tbuf + 2, 1); /* space */
        pad--;
    }
}
/***************************************************************/
static void vsxpplacef(
        void (*addchars)(void * info, const char *buf, int buflen),
        void * info,
        char * nbuf,
        int nbufmax,
        int exp,
        int flags,
        int width,
        int prec,
        char expch) {

    int nlen;
    int nn;
    int nix;
    int nd;
    int nwid;
    int pad;
    char tbuf[4];
    char sign;

    nlen = Istrlen(nbuf);
    nix = 1;
    nd = 0;
    tbuf[0] = '0';
    tbuf[1] = '.';
    tbuf[2] = ' ';

    if (nbuf[0] == '-')            sign = '-';
    else if (flags & SNPFLAG_PLUS) sign = '+';
    else                           sign = ' ';

    nwid = prec + 1;
    if (prec) nwid++;
    if (exp > 0) nwid += exp;

    if (nbuf[0] == '-' || (flags & (SNPFLAG_PLUS | SNPFLAG_SPACE))) {
        nwid++;
        if (flags & SNPFLAG_ZERO) {
            addchars(info, &sign, 1);
        } else {
            flags |= SNPFLAG_SFLT;
        }
    }

    pad = 0;
    if (flags & SNPFLAG_WIDTH) {
        pad = width - nwid;

        if (flags & SNPFLAG_ZERO) {
            while (pad > 0) {
                addchars(info, tbuf, 1); /* zero */
                pad--;
            }
        } else if (!(flags & SNPFLAG_MINUS)) {
            while (pad > 0) {
                addchars(info, tbuf + 2, 1); /* space */
                pad--;
            }
        }
    }

    if (flags & SNPFLAG_SFLT) {
        addchars(info, &sign, 1);
    }

    if (exp < 0) {
        addchars(info, tbuf, 1); /* zero */
        if (prec) addchars(info, tbuf + 1, 1); /* zero point */
        nn = exp + 1;
        while (nn < 0 && nd < prec) {
            addchars(info, tbuf, 1); /* zero */
            nn++;
            nd++;
        }
    } else {
        nn = exp + 1;
        addchars(info, nbuf + 1, nn);
        nix += nn;
        if (prec) addchars(info, tbuf + 1, 1); /* point */
    }

    if (nbuf[1] != '0') {
        nn = nlen - nix;
        if (nn > 0) {
            addchars(info, nbuf + nix, nn);
            nd += nn;
        }
    }

    if (!(flags & SNPFLAG_G_FMT)) {
        while (nd < prec) {
            addchars(info, tbuf, 1); /* zero */
            nn++;
            nd++;
        }
    }

    while (pad > 0) {
        addchars(info, tbuf + 2, 1); /* space */
        pad--;
    }
}
/***************************************************************/
static void vsxpplaceg(
        void (*addchars)(void * info, const char *buf, int buflen),
        void * info,
        char * nbuf,
        int nbufmax,
        int exp,
        int flags,
        int width,
        int prec,
        char expch) {

    int nprec;
    int nlen;

    if (prec) fpround(nbuf, sizeof(nbuf), prec + 1, &exp);
    else      fpround(nbuf, sizeof(nbuf), 2, &exp);

    if (exp < -4 || (exp && exp >= prec)) {
        if (prec) prec--;
        vsxpplacee(addchars, info, nbuf, sizeof(nbuf),
                exp, flags, width, prec, expch);
    } else {
        nlen = Istrlen(nbuf);
        nprec = nlen - exp - 2;
        if (nprec < 0) nprec = 0;
        vsxpplacef(addchars, info, nbuf, nbufmax,
                    exp, flags, width, nprec, expch);
    }
}
/***************************************************************/
static void vsxprintf(
        void (*addchars)(void * info, const char *buf, int buflen),
        void * info,
        const char *fmt,
        va_list args)
{
/*
    snprintf();
    printf Format Specification Fields

    %[flags] [width] [.precision] [modifier] type

    flags:
        +       Result has leading +/- sign
        -       Result is left justified (default is right justified)
        space   Result has leading sign with using space for positive numbers
        0       Result is padded with leading zeros

    width:
        *       width is an int in
        number  indicates width in digits

    precision:
        *       precision is an int in
        number  indicates precision in digits

    modifier:
        hh      Argument is a char **
        h       Argument is a short int
        l       Argument is a long
        ll      Argument is a long long ** (Use L)

    type:
        c       char is result
        d       int is converted to decimal
        e       double is converted to [-]d.ddd e+/-dd
        f       double is converted to [-]ddd.ddd
        g       double is converted to [-]ddd.ddd or [-]d.ddd e+/-dd
        o       unsigned int is converted to octal
        s       zero-terminated string
        S       zero-terminated wide string
        x       unsigned int is converted to hexadecimal (abcdef)
        X       unsigned int is converted to hexadecimal (ABCDEF)

** - Not yet implemented

    state:
        0       Not in specification
        1       In specification

    If PRINTF_EXTENSIONS is #define'd, extensions are enabled as follows:
    flags:
        ,       Use commas as separator

    modifier:
        L       Argument is a long long

    type:
        m       Operating system error message is inserted
        q       Same as s, but quotes string for a .csv file

*/
    int fix;
    int fix0;
    int state;
    char ch;
    int len;
    int width;
    int prec;
    int flags;
    int err;
    char nbuf[128];
    int exp;
    char nullstr[] = "(null)";

    char   tchar[2];
    long   tlong;
    double tdouble;
    char * tstr;
#if PRINTF_I64
    INT64_T ti64;
#endif

    err   = 0;
    state = 0;
    flags = 0;
    prec  = 0;
    width = 0;
    fix   = 0;
    fix0  = 0;
    ch    = fmt[fix];

    while (ch) {
        if (!state) {
            if (ch == '%') {
                len = fix - fix0;
                if (len) addchars(info, fmt + fix0, len);
                if (fmt[fix+1] == '%') {
                    fix++;
                } else {
                    state = 1;
                    flags = 0;
                    width = 0;
                    prec  = 0;
                }
                fix0 = fix;
            }
        } else {
            /***** flags *****/
                /* '-' overrides '0', '+' overrides ' ' */
            if (ch == '-') {
                if (flags & (SNPFLAG_WIDTH | SNPFLAG_PREC))
                    err = SNPERR(2); /* out of order */
                flags |= SNPFLAG_MINUS;
                if (flags & SNPFLAG_ZERO) flags ^= SNPFLAG_ZERO;
            } else if (ch == '+') {
                if (flags & (SNPFLAG_WIDTH | SNPFLAG_PREC))
                    err = SNPERR(2); /* out of order */
                flags |= SNPFLAG_PLUS;
                if (flags & SNPFLAG_SPACE) flags ^= SNPFLAG_SPACE;
            } else if (ch == ' ') {
                if (flags & (SNPFLAG_WIDTH | SNPFLAG_PREC))
                    err = SNPERR(2); /* out of order */
                if (!(flags & SNPFLAG_PLUS)) flags |= SNPFLAG_SPACE;
            } else if (ch == '0') {
                if (flags & SNPFLAG_PREC) {
                    prec = (prec * 10) + (ch - '0');
                } else if (flags & SNPFLAG_WIDTH) {
                    width = (width * 10) + (ch - '0');
                } else {
                    if (!(flags & SNPFLAG_MINUS)) flags |= SNPFLAG_ZERO;
                }
#if PRINTF_EXTENSIONS
            } else if (ch == ',') {
                if (flags & (SNPFLAG_WIDTH | SNPFLAG_PREC))
                    err = SNPERR(2); /* out of order */
                flags |= SNPFLAG_COMMA;
#endif

            /***** width/precision *****/
            } else if (ch >= '1' && ch <= '9') {
                if (flags & SNPFLAG_PREC) {
                    prec = (prec * 10) + (ch - '0');
                } else {
                    width = (width * 10) + (ch - '0');
                    flags |= SNPFLAG_WIDTH;
                }
            } else if (ch == '.') {
                flags |= SNPFLAG_PREC;
            } else if (ch == '*') {
                if (flags & SNPFLAG_PREC) {
                    if (prec) err = SNPERR(3); /* duplicate*/
                    prec = va_arg(args, int);
                } else {
                    if (flags & SNPFLAG_WIDTH) err = SNPERR(3); /* duplicate */
                    width = va_arg(args, int);
                    flags |= SNPFLAG_WIDTH;
                }

            /***** modifier *****/
            } else if (ch == 'h') {
                flags |= SNPFLAG_MOD_H;
            } else if (ch == 'l') {
                flags |= SNPFLAG_MOD_L;
#if PRINTF_I64
            } else if (ch == 'L') {
                flags |= SNPFLAG_MOD_LL;
#endif

            /***** type s *****/
            } else if (ch == 's') {
                tstr = va_arg(args,char*);
                if (!tstr) tstr = nullstr;
                vsxpplaces(addchars, info,
                    tstr, flags, width, prec);
                state = 0;

            /***** type s *****/
            } else if (ch == 'S') {
                tstr = va_arg(args,char*);
                if (!tstr) tstr = nullstr;
                vsxpplacewides(addchars, info,
                    (wchar_t*)tstr, flags, width, prec);

                state = 0;

            /***** type c *****/
            } else if (ch == 'c') {
                tchar[0] = (char)va_arg(args, int);
                tchar[1] = '\0';
                vsxpplaces(addchars, info,
                    tchar, flags, width, prec);
                state = 0;

            /***** type d *****/
            } else if (ch == 'd') {
                if (flags & SNPFLAG_MOD_H) {
                    tlong = (short int)va_arg(args, int);
                } else if (flags & SNPFLAG_MOD_L) {
                    tlong = va_arg(args, long);
#if PRINTF_I64
                } else if (flags & SNPFLAG_MOD_LL) {
                    ti64 = va_arg(args, INT64_T);
#endif
                } else {
                    tlong = va_arg(args, int);
                }

                if (flags & SNPFLAG_MOD_LL) {
#if PRINTF_I64
                    vsxpplacell(addchars, info, nbuf, sizeof(nbuf),
                        ti64, flags, width, prec);
#endif
                } else {
                    vsxpplaced(addchars, info, nbuf, sizeof(nbuf),
                        tlong, flags, width, prec);
                }

                state = 0;

            /***** type o *****/
            } else if (ch == 'o') {
                if (flags & SNPFLAG_MOD_H) {
                    tlong = (short int)va_arg(args, int);
                } else if (flags & SNPFLAG_MOD_L) {
                    tlong = va_arg(args, long);
                } else {
                    tlong = va_arg(args, int);
                }

                vsxpplaceo(addchars, info, nbuf, sizeof(nbuf),
                    tlong, flags, width, prec);

                state = 0;

            /***** type x *****/
            } else if (ch == 'x' || ch == 'X') {
                if (flags & SNPFLAG_MOD_H) {
                    tlong = (short int)va_arg(args, int);
                } else if (flags & SNPFLAG_MOD_L) {
                    tlong = (long)va_arg(args, long);
                } else {
                    tlong = va_arg(args, int);
                }

                vsxpplacex(addchars, info, nbuf, sizeof(nbuf),
                    tlong, flags, width, prec, (char)(ch=='X'?'A':'a'));

                state = 0;

            /***** type e *****/
            } else if (ch == 'e' || ch == 'E') {
                tdouble = va_arg(args, double);

                if (!(flags & SNPFLAG_PREC)) prec = 6;  /* default */

                fpnorm(nbuf, sizeof(nbuf), tdouble, prec + 1, &exp);
                fpround(nbuf, sizeof(nbuf), prec + 2, &exp);
                vsxpplacee(addchars, info, nbuf, sizeof(nbuf),
                            exp, flags, width, prec, ch);
                state = 0;

            /***** type f *****/
            } else if (ch == 'f' || ch == 'F') {
                tdouble = va_arg(args, double);

                if (!(flags & SNPFLAG_PREC)) prec = 6;  /* default */

                fpnorm(nbuf, sizeof(nbuf), tdouble, prec + 1, &exp);
                fpround(nbuf, sizeof(nbuf), exp + 2 + prec, &exp);
                vsxpplacef(addchars, info, nbuf, sizeof(nbuf),
                        exp, flags, width, prec, ch);
                state = 0;

            /***** type g *****/
            } else if (ch == 'g' || ch == 'G') {
                tdouble = va_arg(args, double);

                if (!(flags & SNPFLAG_PREC)) prec = 6;  /* default */

                fpnorm(nbuf, sizeof(nbuf), tdouble, prec + 1, &exp);
                vsxpplaceg(addchars, info, nbuf, sizeof(nbuf),
                        exp, flags | SNPFLAG_G_FMT,
                        width, prec, (char)(ch=='G'?'E':'e'));
                state = 0;

#if PRINTF_EXTENSIONS
            } else if (ch == 'm') {
                tlong = va_arg(args, int);
                tstr = get_strerror(tlong);
                if (!tstr) tstr = nullstr;
                vsxpplaces(addchars, info,
                    tstr, flags, width, prec);
                Free(tstr);
                state = 0;
            } else if (ch == 'M') {
                tlong = va_arg(args, int);
                tstr = get_os_message(tlong);
                if (!tstr) tstr = nullstr;
                vsxpplaces(addchars, info,
                    tstr, flags, width, prec);
                Free(tstr);
                state = 0;

            /***** type q *****/
            } else if (ch == 'q') {
                tstr = va_arg(args,char*);
                if (!tstr) tstr = nullstr;
                vsxpplaceq(addchars, info,
                    tstr, flags, width, prec);
                state = 0;
#endif

            /***** unknown *****/
            } else {
                err = SNPERR(1); /* unrecognized format character */
                state = 0;
            }
            if (!state) fix0 = fix + 1;
        }
        fix++;
        ch = fmt[fix];
    }

    len = fix - fix0;
    if (len) addchars(info, fmt + fix0, len);
}
/***************************************************************/
struct snprintfinfo {
    char * buf;
    int    buflen;
    int    bufmax;
};
/***************************************************************/
static void snprintfbuf(void * info, const char *buf, int buflen) {

    struct snprintfinfo * snpi = (struct snprintfinfo *)info;

    int nchars;

    nchars = snpi->bufmax - snpi->buflen - 1;
    if (buflen < nchars) nchars = buflen;

    if (nchars) {
        memcpy(snpi->buf + snpi->buflen, buf, nchars);
        snpi->buflen += nchars;
    }
}
/***************************************************************/
int Snprintf(char *buf, size_t size, const char *fmt, ...)
{
    va_list args;
    struct snprintfinfo snpi;

    if (size <= 0) return (0);

    snpi.buf    = buf;
    snpi.buflen = 0;
    snpi.bufmax = (int)size;

    va_start(args, fmt);
    vsxprintf(snprintfbuf, &snpi, fmt, args);
    va_end(args);

    buf[snpi.buflen] = '\0';

    return snpi.buflen;
}
/***************************************************************/
int Vsnprintf(char *buf, size_t size, const char *fmt, va_list args)
{
    struct snprintfinfo snpi;

    if (size <= 0) return (0);

    snpi.buf    = buf;
    snpi.buflen = 0;
    snpi.bufmax = (int)size;

    vsxprintf(snprintfbuf, &snpi, fmt, args);

    buf[snpi.buflen] = '\0';

    return snpi.buflen;
}
/***************************************************************/
struct smprintfinfo {
    char * buf;
    int    buflen;
    int    bufmax;
};
/***************************************************************/
static void smprintfbuf(void * info, const char *buf, int buflen) {

    struct smprintfinfo * smpi = (struct smprintfinfo *)info;

    if (smpi->buflen + buflen >= smpi->bufmax) {
        smpi->bufmax += buflen + 64;
        smpi->buf = Realloc(smpi->buf, char, smpi->bufmax);
    }

    memcpy(smpi->buf + smpi->buflen, buf, buflen);
    smpi->buflen += buflen;
}
/***************************************************************/
char * Smprintf(const char *fmt, ...)
{
    va_list args;
    struct smprintfinfo smpi;

    smpi.buf    = NULL;
    smpi.buflen = 0;
    smpi.bufmax = 0;

    va_start(args, fmt);
    vsxprintf(smprintfbuf, &smpi, fmt, args);
    va_end(args);

    smprintfbuf(&smpi, "\0", 1);

    return smpi.buf;
}
/***************************************************************/
char * Vsmprintf(const char *fmt, va_list args)
{
    struct smprintfinfo smpi;

    smpi.buf    = NULL;
    smpi.buflen = 0;
    smpi.bufmax = 0;

    vsxprintf(smprintfbuf, &smpi, fmt, args);

    smprintfbuf(&smpi, "\0", 1);

    return smpi.buf;
}
/***************************************************************/
int Vfmprintf(FILE * fref, const char *fmt, va_list args)
{
    int out;
    struct smprintfinfo smpi;

    smpi.buf    = NULL;
    smpi.buflen = 0;
    smpi.bufmax = 0;

    vsxprintf(smprintfbuf, &smpi, fmt, args);

    if (!smpi.buflen) {
        out = 0;
    } else {
        smprintfbuf(&smpi, "\0", 1);

        if (fputs(smpi.buf, fref) < 0) out = -1;
        else                           out = smpi.buflen - 1;
    }
    Free(smpi.buf);

    return out;
}
/***************************************************************/
int Fmprintf(FILE * fref, const char *fmt, ...)
{
    va_list args;
    int out;
    struct smprintfinfo smpi;

    smpi.buf    = NULL;
    smpi.buflen = 0;
    smpi.bufmax = 0;

    va_start(args, fmt);
    vsxprintf(smprintfbuf, &smpi, fmt, args);
    va_end(args);

    if (!smpi.buflen) {
        out = 0;
    } else {
        smprintfbuf(&smpi, "\0", 1);

        if (fputs(smpi.buf, fref) < 0) out = -1;
        else                           out = smpi.buflen - 1;
    }
    Free(smpi.buf);

    return out;
}
/***************************************************************/
