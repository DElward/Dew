/***************************************************************/
/* util.c                                                      */
/***************************************************************/
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <math.h>
#if IS_WINDOWS
    #include <time.h>
    #define WIN32_LEAN_AND_MEAN
    #include <Windows.h>
#endif

#include "omar.h"
#include "util.h"
#include "snprfh.h"

/***************************************************************/
/*     Utility Functions                                       */
/***************************************************************/
void strmcpy(char * tgt, const char * src, size_t tmax) {
/*
** Copies src to tgt, trucating src if too intr.
**  tgt always termintated with '\0'
*/

    size_t slen;

    slen = IStrlen(src) + 1;

    if (slen <= tmax) {
        memcpy(tgt, src, slen);
    } else if (tmax > 0) {  /* > 0 added 5/6/2013 */
        slen = tmax - 1;
        memcpy(tgt, src, slen);
        tgt[slen] = '\0';   /* changed from tgt[tmax] on 4/30/2013 */
    }
}
/***************************************************************/
/*@*/int istrcmp(const char * b1, const char * b2) {
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
char * parse_command_line(const char * cmd, int * pargc, const char * arg0, int flags) {

/*
**  flags   0 - treat backslashes like any character
**          1 - treat backslashes as an escape character
*/
    /* see http://msdn.microsoft.com/en-us/library/a1y7w461.aspx */
    int ix;
    int qix;
    int done;
    int nparms;
    int parmix;
    int tlen;
    int size;
    int tix;
    int arg0len;
    char * cargv;
    char ** argv;

    /* pass 1: count parms */
    arg0len = 0;
    nparms = 0;
    tlen = 0;
    ix = 0;
    while (cmd[ix]) {
        while (isspace(cmd[ix])) ix++;
        if (cmd[ix]) {
            qix = -1;
            done = 0;
            while (!done) {
                if (!cmd[ix]) done = 1;
                else if (cmd[ix] == '\"') {
                    if (qix < 0) qix = ix;
                    else qix = -1;
                    ix++;
                } else if (isspace(cmd[ix]) && (qix < 0)) {
                    done = 1;
                } else {
                    if (cmd[ix] == '\\' && cmd[ix+1] && (flags & 1)) ix++;
                    tlen++;
                    ix++;
                }
            }
            tlen++; /* for '\0' terminator */
            nparms++;
        }
    }

    if (arg0) {
        nparms++;
        arg0len = IStrlen(arg0) + 1;
        tlen += arg0len;
    }

    tix = sizeof(char*) * (nparms + 1);
    size = tix + tlen;
    cargv = New(char, size);
    argv = (char**)cargv;
    parmix = 0;

    if (arg0) {
        argv[parmix] = cargv + tix;
        memcpy(cargv + tix, arg0, arg0len);
        tix += arg0len;
        parmix++;
    }

    /* pass 2: move parms */
    ix = 0;
    while (cmd[ix]) {
        while (isspace(cmd[ix])) ix++;
        if (cmd[ix]) {
            qix = -1;
            argv[parmix] = NULL;
            done = 0;
            while (!done) {
                if (!cmd[ix]) done = 1;
                else if (cmd[ix] == '\"') {
                    if (qix < 0) qix = ix;
                    else qix = -1;
                    ix++;
                } else if (isspace(cmd[ix]) && (qix < 0)) {
                    done = 1;
                } else {
                    if (cmd[ix] == '\\' && cmd[ix+1] && (flags & 1)) ix++;
                    if (!argv[parmix]) argv[parmix] = cargv + tix;
                    cargv[tix++] = cmd[ix];
                    ix++;
                }
            }
            if (!argv[parmix]) argv[parmix] = cargv + tix;
            cargv[tix++] = '\0';
            parmix++;
        }
    }
    argv[nparms] = NULL;

    (*pargc) = nparms;

    return (cargv);
}
/***************************************************************/
void append_chars(char ** cstr,
                    int * slen,
                    int * smax,
                    const char * val,
                    int vallen)
{
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
void append_chars_rept(char ** cstr,
                    int * slen,
                    int * smax,
                    char val,
                    int vallen)
{
    if ((*slen) + vallen >= (*smax)) {
        if (!(*smax)) (*smax) = 16 + vallen;
        else         (*smax)  = ((*smax) * 2) + vallen;
        (*cstr) = Realloc(*cstr, char, (*smax));
    }
    memset((*cstr) + (*slen), val, vallen);
    (*slen) += vallen;
    (*cstr)[*slen] = '\0';
}
/***************************************************************/
#define NEEDS_ESCAPE(c) ((c) < ' ' || (c) > '~' || (c) == '\\' || (c) == '\"')
int any_escapes(const char * val)
{
    int anye;
    int vallen;
    int ii;

    anye = 0;
    if (val[0] == '\"') {
        vallen = IStrlen(val);
        if (vallen > 1 && val[vallen - 1] == '\"') vallen--;
        for (ii = 1; !anye && ii < vallen; ii++) {
            anye = NEEDS_ESCAPE(val[ii]);
        }
    } else {
        for (ii = 0; !anye && val[ii]; ii++) {
            anye = NEEDS_ESCAPE(val[ii]);
        }
    }

    return (anye);
}
/***************************************************************/
void get_char_escape(char * ebuf, char ch)
{
/*
** See also: xlate_backslash()
*/
    char hexbuf[] = "0123456789abcdef";

    if (ch < ' ' || ch > '~') {
        ebuf[0] = '\\';
        ebuf[2] = '\0';
        switch (ch) {
            case '\0': ebuf[1] = '0'; break;
            case '\a': ebuf[1] = 'a'; break;
            case '\b': ebuf[1] = 'b'; break;
            case '\f': ebuf[1] = 'f'; break;
            case '\n': ebuf[1] = 'n'; break;
            case '\r': ebuf[1] = 'r'; break;
            case '\t': ebuf[1] = 't'; break;
            default:
                ebuf[1] = 'x';
                ebuf[2] = hexbuf[(ch & 0xF0) >> 4];
                ebuf[3] = hexbuf[(ch & 0x0F)     ];
                ebuf[4] = '\0';
                break;
        }
    } else if (ch == '\"' || ch == '\\') {
        ebuf[0] = '\\';
        ebuf[1] = ch;
        ebuf[2] = '\0';
    } else {
        ebuf[0] = ch;
        ebuf[1] = '\0';
    }
}
/***************************************************************/
char * escape_text(const char * val)
{
    char * buf;
    int buflen;
    int bufmax;
    int vallen;
    int elen;
    int ii;
    char ebuf[8];

    buf    = NULL;
    buflen = 0;
    bufmax = 0;

    if (val[0] == '\"') {
        vallen = IStrlen(val);
        if (vallen > 1 && val[vallen - 1] == '\"') vallen--;
        append_chars(&buf, &buflen, &bufmax, val, 1);
        for (ii = 1; ii < vallen; ii++) {
            if (NEEDS_ESCAPE(val[ii])) {
                get_char_escape(ebuf, val[ii]);
                elen = IStrlen(ebuf);
                append_chars(&buf, &buflen, &bufmax, ebuf, elen);
            } else {
                append_chars(&buf, &buflen, &bufmax, val + ii, 1);
            }
        }
        if (val[vallen]) append_chars(&buf, &buflen, &bufmax, val + vallen, 1);
    } else {
        for (ii = 0; val[ii]; ii++) {
            if (NEEDS_ESCAPE(val[ii])) {
                get_char_escape(ebuf, val[ii]);
                elen = IStrlen(ebuf);
                append_chars(&buf, &buflen, &bufmax, ebuf, elen);
            } else {
                append_chars(&buf, &buflen, &bufmax, val + ii, 1);
            }
        }
    }

    return (buf);
}
/***************************************************************/
/*     File Utility Functions                                  */
/***************************************************************/
int set_error_f (struct frec * fr, const char * fmt, ...) {

    va_list args;

    va_start (args, fmt);
    vfprintf(stderr, fmt, args);
    va_end (args);
    fprintf(stderr, "\n");

    if (fr) {
        fprintf(stderr, "Error in file %s, line #%d\n", fr->fname, fr->flinenum);
    }

    return (-1);
}
/***************************************************************/
int frec_gets(char * line, int linemax, struct frec * fr) {

    int ii;
    int lix;
    int done;
    int estat;

    if (!fr) {
        return (-1);
    }

    lix = 0;
    done = 0;

    while (!done) {
        if (!fgets(fr->fbuf, fr->fbufsize, fr->fref)) {
            int en = ERRNO;
            done = 1;
            if (ferror(fr->fref) || !feof(fr->fref)) {
                estat = set_error_f(fr, "Error #%d reading file: %s\n%s",
                                    en, fr->fname, strerror(en));
                lix = (-1);
                fr->feof = -1;
            } else {
                fr->feof = 1;
                lix = (-1);
            }
        } else {
            fr->flinenum += 1;
            ii = IStrlen(fr->fbuf);
            while (ii > 0 && isspace(fr->fbuf[ii - 1])) ii--;
            if (linemax - lix <= ii) {
                estat = set_error_f(fr, "Buffer overflow reading file");
                lix = (-1);
                done = 1;
            } else {
                done = 1;
                memcpy(line + lix, fr->fbuf, ii);
                lix += ii;
                line[lix] = '\0';
            }
        }
    }

    return (lix);
}
/***************************************************************/
int frec_close(struct frec * fr) {

    if (fr) {
        fclose(fr->fref);
        Free(fr->fname);
        Free(fr->fbuf);
        Free(fr);
    }

    return (0);
}
/***************************************************************/
struct frec * frec_open(const char * fname, const char * fmode, const char * ftyp) {

    FILE * fref;
    struct frec * fr;

    fref = fopen(fname, fmode);
    if (!fref) {
        int ern = ERRNO;
        set_error_f(NULL, "Error #%d opening %s: %s\n%s",
                            ern, ftyp, fname, strerror(ern));
        fr = NULL;
    } else {
        fr = New(struct frec, 1);
        fr->fname = Strdup(fname);
        fr->fbufsize = LINE_MAX_SIZE + 2;
        fr->fbuf = New(char, fr->fbufsize);
        fr->flinenum = 0;
        fr->fbuflen = 0;
        fr->feof = 0;
        fr->fref = fref;
    }

    return (fr);
}
/***************************************************************/
int frec_puts(const char * line, struct frec * fr) {
 
    int estat = 0;
    int rtn;

    rtn = fputs(line, fr->fref);
    if (rtn < 0) {
        int en = ERRNO;
        estat = set_error_f(fr, "Error #%d writing to file: %s\n%s",
                            en, fr->fname, strerror(en));
    }

    return (estat);
}
/***************************************************************/
int frec_printf(struct frec * fr, char * fmt, ...) {

    va_list args;
    int estat = 0;
    int rtn;
    int en;

    va_start (args, fmt);
    rtn = vfprintf(fr->fref, fmt, args);
    if (rtn < 0) en = ERRNO;
    va_end (args);

    if (rtn < 0) {
        estat = set_error_f(fr, "Error #%d printing to file: %s\n%s",
                            en, fr->fname, strerror(en));
    }

    return (estat);
}
/***************************************************************/
/*     Base 64 Functions                                       */
/***************************************************************/
char base64_table[] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

#define B64_TERM_CHAR    '='

int from_base64_c(
    char * bbuf,
    const char * b64,
    int b64len,
    int b64flags)
{
/*
** Returns length of converted string
** 05/11/2017
*/
    unsigned char * ub64;
    int b64ix;
    int bbufix;
    unsigned char reverse_base64_table[256];

    ub64 = (unsigned char *)b64;

    /* make reverse table */
    memset(reverse_base64_table, 0, 256);
    for (b64ix = 0; base64_table[b64ix]; b64ix++) {
        reverse_base64_table[base64_table[b64ix]] = b64ix;
    }

    ub64 = (unsigned char *)b64;
    bbufix = 0;

    b64ix = 0;
    while (b64ix < b64len) {
        if (b64ix + 1 < b64len) {
            bbuf[bbufix++] =
                (reverse_base64_table[ub64[b64ix]] << 2) |
                ((reverse_base64_table[ub64[b64ix + 1]] >> 4) & 0x03);
        }
        if (b64ix + 2 < b64len && ub64[b64ix + 2] != B64_TERM_CHAR) {
            bbuf[bbufix++] =
                (reverse_base64_table[ub64[b64ix + 1]] << 4) |
                ((reverse_base64_table[ub64[b64ix + 2]] >> 2) & 0x0F);
            if (b64ix + 3 < b64len && ub64[b64ix + 3] != B64_TERM_CHAR) {
                bbuf[bbufix++] =
                    (reverse_base64_table[ub64[b64ix + 2]] << 6) |
                    (reverse_base64_table[ub64[b64ix + 3]] & 0x3F);
            }
        }
        b64ix += 4;
    }
    bbuf[bbufix] = '\0';

    return (bbufix);
}
/***************************************************************/
void free_charlist(char ** charlist) {

    int ii;

    if (charlist) {
        for (ii = 0; charlist[ii]; ii++) Free(charlist[ii]);
        Free(charlist);
    }
}
/***************************************************************/
static char xlate_backslash(const char *line, int * index) {
/*
** Translate a backslashed character - index points to backslash
** See also: get_char_escape()
*/

    char ch;
    char out;
    unsigned char h1;
    unsigned char h2;

    (*index)++;
    ch = line[(*index)];

    switch (ch) {
        case '0':               out = '\0'; break; /* null */
        case 'a': case 'A':     out = '\a'; break; /* bell */
        case 'b': case 'B':     out = '\b'; break; /* backspace */
        case 'f': case 'F':     out = '\f'; break; /* formfeed */
        case 'n': case 'N':     out = '\n'; break; /* newline */
        case 'r': case 'R':     out = '\r'; break; /* return */
        case 't': case 'T':     out = '\t'; break; /* tab */
        case 'x': case 'X':                        /* Hexadecimal */

            (*index)++;
            h1 = line[(*index)];
            if (h1) { (*index)++; h2 = line[(*index)]; }
            else return (0);

            if (h1 >= '0' && h1 <= '9')      out = (h1 - '0') << 4;
            else if (h1 >= 'A' && h1 <= 'F') out = ((h1 - 'A') + 10) << 4;
            else if (h1 >= 'a' && h1 <= 'f') out = ((h1 - 'a') + 10) << 4;
            else return (0);

            if (h2 >= '0' && h2 <= '9')      out |= (h2 - '0');
            else if (h2 >= 'A' && h2 <= 'F') out |= (h2 - 'A') + 10;
            else if (h2 >= 'a' && h2 <= 'f') out |= (h2 - 'a') + 10;
            else return (0);

            break;

        default:                out = ch;   break;
    }

    return (out);
}
/***************************************************************/
static void get_toke_quote(const char * bbuf,
                         int * bbufix,
                         char *toke,
                         int tokemax,
                         int gt_flags) {
/*
** Parse quotation
*/
    char quote;
    int tokelen;
    int done;
    char ch;

    tokelen = 0;
    toke[0] = '\0';

    ch = bbuf[*bbufix];
    (*bbufix)++;
    quote = ch;
    toke[tokelen++] = ch;
    done = 0;

    while (!done) {
        ch = bbuf[*bbufix];

        if (ch == '\\' && (gt_flags & GETTOKE_FLAG_C_BACKSLASH)) {
            ch = xlate_backslash(bbuf, bbufix);
            if (tokelen + 1 < tokemax) toke[tokelen++] = ch;
        } else if (ch == quote) {
            if (tokelen + 1 < tokemax) toke[tokelen++] = ch;
            (*bbufix)++;
            done = 1;
        } else {
            if (tokelen + 1 < tokemax) toke[tokelen++] = ch;
            (*bbufix)++;
        }
    }

    toke[tokelen] = '\0';
}
/***************************************************************/
void get_toke_char(const char * bbuf,
                         int * bbufix,
                         char *toke,
                         int tokemax,
                         int gt_flags)
{
/*
** Reads next token from bbuf, starting from bbufix.
**
** Token is placed into toke.
**
*/

    int tokelen;
    int done;
    char ch;

    tokelen = 0;
    toke[0] = '\0';

    while (bbuf[*bbufix] &&
           isspace(bbuf[*bbufix])) (*bbufix)++;
    if (!bbuf[*bbufix]) return;

    ch = bbuf[*bbufix];
    if (ch == '\"' ||
        (ch == '\'' && (gt_flags & GETTOKE_FLAG_APOSTROPHE_QUOTES))) {
        get_toke_quote(bbuf, bbufix, toke, tokemax, gt_flags);
        return;
    } else {
        done = 0;
        while (!done) {
            ch = bbuf[*bbufix];

            if (isalpha(ch) || isdigit(ch)) {
                if (tokelen + 1 < tokemax) {
                    if (gt_flags & GETTOKE_FLAG_UPSHIFT) toke[tokelen++] = toupper(ch);
                    else                                 toke[tokelen++] = ch;
                }
                (*bbufix)++;
            } else if (ch == '$'    ||
                       ch == '-'    || /* for numeric sign */
                       ch == '+'    || /* for numeric sign */
                       ch == CRAIGS_MINUS || /* bug? */
                       (ch & 0x80)  || /* for UTF8 */
                       ch == '_') {
                if (tokelen + 1 < tokemax) toke[tokelen++] = ch;
                (*bbufix)++;
            } else {
                if (!tokelen) {
                    if (tokelen + 1 < tokemax) toke[tokelen++] = ch;
                    (*bbufix)++;
                }
                done = 1;
            }
        }
    }

    toke[tokelen] = '\0';
}
/***************************************************************/
int char_to_integer(const char * numbuf, int * num)
{
/*
**  Max is +/- 99,999,999
**
**  Returns: 0=OK, -1=No number, -2=bad character,-3=overflow
*/
    int estat = 0;
    int neg;
    int ans;
    const char * bp;

    ans = 0;
    neg = 0;
    bp = numbuf;
    if (*bp == '-' || *bp == CRAIGS_MINUS) {
        neg = 1;
        bp++;
    } else if (*bp == '+') {
        bp++;
    }

    if (!(*bp)) {
        estat = -1;
    } else {
        while (!estat && (*bp) && ans <= 99999999L) {
            if (isdigit(*bp)) {
                ans = ans * 10 + (*bp) - '0';
                bp++;
            } else {
                estat = -2;
            }
        }
    }
    
    if (!estat && (*bp)) {
        estat = -3;
    }
    
    if (!estat) {
        if (neg) (*num) = -ans;
        else     (*num) = ans;
    }

    return (estat);
}
/***************************************************************/
int string_to_double(const char * dblbuf, double * answer)
{
    int estat;
    int neg;
    int ix;
    int ndigits;
    double nval;
    double dec;

    estat = 0;
    (*answer) = 0.0;
    nval = 0.0;
    neg = 0;
    ix = 0;
    ndigits = 0;

    if (dblbuf[ix] == '-') {
        neg = 1;
        ix++;
    } else if (dblbuf[ix] == '+') {
        ix++;
    }

    while (dblbuf[ix] >= '0' && dblbuf[ix] <= '9') {
        ndigits++;
        nval = (nval * 10.0) + (double)(dblbuf[ix] - '0');
        ix++;
    }

    if (dblbuf[ix] && dblbuf[ix] == '.') {
        ix++;
        dec = 1.0;
        while (dblbuf[ix] >= '0' && dblbuf[ix] <= '9') {
            ndigits++;
            dec /= 10.0;
            nval += dec * (double)(dblbuf[ix] - '0');
            ix++;
        }
    }

    if (dblbuf[ix]) {
        estat = 1;
    } else if (!ndigits) {
        estat = 2;
    } else {
        if (neg) nval = -nval;
        (*answer) = nval;
    }
    
    return (estat);
}
/***************************************************************/
void double_to_string(char * nbuf, double dval, int nbufsz)
{
    double anum;
    double inum;
    double dig;
    int neg;
    int fix;
    int eix;
    int sigdig;
    int sigdigmax;
    int tbufend;
    int ii;
    int carry;
    char ch;
    char tbuf[128];

    fix = sizeof(tbuf) / 2;
    eix = fix + 1;
    tbufend = sizeof(tbuf) - 1;
    sigdigmax = 15; /* maximum of 15 significant digits */
    sigdig = 0;

    if (dval == 0) {
        strmcpy(nbuf, "0", nbufsz);
        return;
    }

    if (dval < 0) {
        anum = -dval;
        neg = 1;
    } else  {
        anum = dval;
        neg = 0;
    }
    inum = floor(anum);

    if (inum == 0) {
        tbuf[fix--] = '0';
    } else {
        while (fix > 0 && inum > 0.0) {
            dig = fmod(inum, 10.0);
            ch = (int)(dig) + '0';
            tbuf[fix--] = ch;
            inum = (inum - dig) / 10.0;
            sigdig++;
        }
    }

    /* zero out junk digits */
    for (ii = fix + sigdigmax + 1; ii < eix; ii++) {
        tbuf[ii] = '0';
    }

    inum = anum - floor(anum);
    if (sigdig < sigdigmax && inum > 0.0) {
        tbuf[eix++] = '.';
        while (eix < tbufend && inum > 0.0 && sigdig < sigdigmax) {
            inum *= 10.0;
            dig = floor(inum);
            ch = (int)(dig) + '0';
            tbuf[eix++] = ch;
            inum = inum - dig;
            if (sigdig || ch != '0') sigdig++;
        }
        /* round */
        if (inum >= 0.5) {
            ii = eix;
            do {
                ii--;
                if (tbuf[ii] == '.') ii--;
                if (tbuf[ii] == '9') { tbuf[ii] = '0'; carry = 1; }
                else                 { tbuf[ii]++; ; carry = 0; }
            } while (carry && ii > 0);
            if (ii < fix) fix = ii;
        }
        /* remove trailing zeros after '.' */
        while (tbuf[eix - 1] == '0') {
            eix--;
        }
        if (tbuf[eix - 1] == '.') eix--;
    }

    if (neg) tbuf[fix--] = '-';
    tbuf[eix] = '\0';
    strmcpy(nbuf, tbuf + fix + 1, nbufsz);
}
/***************************************************************/

#define CAL_REVISION_YEAR           1752
#define CAL_REVISION_MONTH          9
#define CAL_REVISION_DAY            2
#define CAL_REVISION_NDAYS          11
#define CAL_YEAR_BIAS               6400

#define CAL_NDY1                    365L      /* Days in 1 year    */
#define CAL_NDY4                    1461L     /* Days in 4 years   */
#define CAL_NDY4_NLY                1460L     /* Days in 4 years with no leap year */
#define CAL_NDY100                  36524L    /* Days in 100 years */
#define CAL_NDY400                  146097L   /* Days in 400 years */
#define CAL_NDY100_LY4              36525L    /* Days in 100 years with leap year every 4 years */
#define CAL_NDY400_LY4              146100L   /* Days in 400 years with leap year every 4 years */

#define CAL_NDY1600_LY4  (4L * CAL_NDY400_LY4) /* Days in 1600 years with leap year every 4 years */
#define CAL_YEAR_BIAS_DAYS          ((CAL_YEAR_BIAS / 400L) * CAL_NDY400_LY4)
#define CAL_REVISION_DAY_NUMBER     (CAL_YEAR_BIAS_DAYS + 640164L)
#define CAL_JULIAN_DIFF             (CAL_YEAR_BIAS_DAYS - 1721056L)
/***************************************************************/
int ymd_to_daynum(int year, int month, int day) {
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
    int daynum;

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
void daynum_to_ymd(int daynum, int * year, int * month, int * day) {

    int yy;
    int mm;
    int dd;
    int leap;
    int quad;
    int dquad;
    int cent;
    int dcent;
    int adaynum;

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
}
#else
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
}
/***************************************************************/
#endif /* IS_WINDOWS */
/***************************************************************/
void start_cpu_timer(struct proctimes * pt_start) {

    get_cpu_timer(pt_start);
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
void stop_cpu_timer(struct proctimes * pt_start,
                    struct proctimes * pt_end,
                    struct proctimes * pt_result) {

    get_cpu_timer(pt_end);

    timestamp_diff(&(pt_end->pt_wall_time), &(pt_start->pt_wall_time), &(pt_result->pt_wall_time));
    timestamp_diff(&(pt_end->pt_cpu_time) , &(pt_start->pt_cpu_time) , &(pt_result->pt_cpu_time));
    timestamp_diff(&(pt_end->pt_sys_time) , &(pt_start->pt_sys_time) , &(pt_result->pt_sys_time));

}
/***************************************************************/
int format_timestamp(struct timestamp * ts, const char * fmt, char *buf, int bufsize) {
/*
** Similar to strftime(), fmt_d_date()
**
** See also: parse_timestamp()
**
** Supported formats:
**
**    %%  Percent sign
**    %i Tenths of second
**    %ii Hundredths of second
**    %iii Thousandths of second (milliseconds)
**    %M  Minute within hour as decimal number (00 - 59)
**    %nd Total number of days in timestamp, preceded by '-' if negative
**    %nD Total number of days in timestamp, blank if zero
**    %ns Number of seconds in with day
**    %nS Total number of seconds in timestamp, preceded by '-' if negative
**    %H  Hour in 24-hour format (00 - 23)
**    %S  Second within minute as decimal number (00 - 59)
*/
    int ii;
    int ix;
    int bx;
    int hh;
    int mi;
    int ss;
    int fmterr;
    char tb[32];

    hh = ts->ts_second / 60;
    mi = (ts->ts_second / 60) % 60;
    ss = ts->ts_second % 60;

    fmterr = 0;
    ix = 0;
    bx = 0;
    while (fmt[ix]) {
        if (fmt[ix] == '%' && fmt[ix+1]) {
            ix++;
            switch (fmt[ix]) {
                case '%':
                    if (bx + 1 >= bufsize) {
                        fmterr |= 2; /* overflow */
                    } else {
                        buf[bx++] = '%';
                    }
                    break;

                case 'H':
                    if (bx + 2 >= bufsize) {
                        fmterr |= 2; /* overflow */
                    } else {
                        buf[bx++] = (hh / 10) + '0';
                        buf[bx++] = (hh % 10) + '0';
                    }
                    break;

                case 'i':
                    if (bx + 1 >= bufsize) {
                        fmterr |= 2; /* overflow */
                    } else {
                        buf[bx++] = ((ts->ts_microsec / 100000L) % 10) + '0';
                        if (fmt[ix + 1] == 'i' && bx + 1 < bufsize) {
                            ix++;
                            buf[bx++] = ((ts->ts_microsec / 10000L) % 10) + '0';
                            if (fmt[ix + 1] == 'i' && bx + 1 < bufsize) {
                                ix++;
                                buf[bx++] = ((ts->ts_microsec / 1000L) % 10) + '0';
                            }
                        }
                    }
                    break;

                case 'M':
                    if (bx + 2 >= bufsize) {
                        fmterr |= 2; /* overflow */
                    } else {
                        buf[bx++] = (mi / 10) + '0';
                        buf[bx++] = (mi % 10) + '0';
                    }
                    break;

                case 'n':
                    ix++;
                    switch (fmt[ix]) {
                        case 'd':
                            Snprintf(tb, sizeof(tb), "%6d", ts->ts_daynum);
                            break;
                        case 'D':
                            if (ts->ts_daynum) {
                                Snprintf(tb, sizeof(tb), "%6d", ts->ts_daynum);
                            } else {
                                tb[0] = '\0';
                            }
                            break;
                        case 's':
                            Snprintf(tb, sizeof(tb), "%6Ld", ts->ts_second);
                            break;
                        case 'S':
                            Snprintf(tb, sizeof(tb), "%6d",
                                (ts->ts_daynum * 86400L) + ts->ts_second);
                            break;
                        default: ;
                            fmterr |= 1; /* bad % */
                            if (bx + 1 >= bufsize) {
                                fmterr |= 2; /* overflow */
                            } else {
                                buf[bx++] = fmt[ix];
                            }
                            break;
                        }
                        for (ii = 0; !fmterr && tb[ii]; ii++) {
                            if (bx + 1 >= bufsize) {
                                fmterr |= 2; /* overflow */
                            } else {
                                buf[bx++] = tb[ii];
                        }
                    }
                    break;

                case 'S':
                    if (bx + 2 >= bufsize) {
                        fmterr |= 2; /* overflow */
                    } else {
                        buf[bx++] = (ss / 10) + '0';
                        buf[bx++] = (ss % 10) + '0';
                    }
                    break;

                default: ;
                    fmterr |= 1; /* bad % */
                    if (bx + 1 >= bufsize) {
                        fmterr |= 2; /* overflow */
                    } else {
                        buf[bx++] = fmt[ix];
                    }
                    break;
            }
            ix++;
        } else {
            if (bx + 1 >= bufsize) {
                fmterr |= 2; /* overflow */
            } else {
                buf[bx++] = fmt[ix++];
            }
        }
    }
    if (bx >= bufsize) buf[bufsize - 1] = '\0';
    else               buf[bx] = '\0';

    return (fmterr);
}
/***************************************************************/
#define DIVMO(q,r,a,b) {q=a/b;r=a%b;if(r&&((a<0&&b>0)||(a>0&&b<0))){q--;r+=b;}}
void idivmo(int * q, int * r, int a, int b) {
/*
** Integer division and modulus (correct regardless of sign of a and b)
**
**      Does nothing at all if b is 0
**      C operators / and % are correct if sign(a) == sign(b)
**      If there is a remainder and sign(a) != sign(b), then b needs to be added to
**      the remainder and the quotient needs to be decremented.        
**
** Returns:
**  (*q) = floor(a / b)
**  (*r) = a - b * floor(a / b)     <-- Always same sign as b
*/

    if (b > 0) {
        (*q) = a / b;
        (*r) = a % b;
        if ((*r) && a < 0) {
            (*r) += b;
            (*q)--;
        }
    } else if (b < 0) {
        (*q) = a / b;
        (*r) = a % b;
        if ((*r) && a > 0) {
            (*r) += b;
            (*q)--;
        }
    }
}
/***************************************************************/
int parse_timestamp(const char* dbuf,
                int *ix,
                const char * fmt,
                struct timestamp * ts) {
/*
** Similar to parsedate()
**
** See also: format_timestamp()
**
** Supported formats:
**    %%  Percent sign
**    %i  Tenths of second
**    %ii Hundredths of second
**    %iii Thousandths of second (milliseconds)
**    %H  Hour in 24-hour format (00 - 23)
**    %M  Minute within hour as decimal number (00 - 59)
**    %nd Total number of days in interval, preceded by '-' if negative
**    %nD Total number of days in timestamp, blank if zero
**    %ns Number of seconds in with day
**    %nS Total number of seconds in timestamp, preceded by '-' if negative
**    %H  Hour in 24-hour format (00 - 23)
**    %S  Second within minute as decimal number (00 - 59)
*/
    int bx;
    int fx;
    int errn;
    int n_d;
    int n_S;
    int dd;
    int hh;
    int mi;
    int ss;
    int ni;
    int neg;
    int fracsecs;
    char fdone;
    char fdel;
    int tol;    /* tolerant - %H %d. ok with 1 digit */
    char buf[64];

    errn    = 0;

    ts->ts_daynum   = 0;
    ts->ts_second   = 0;
    ts->ts_microsec = 0;

    fx      = 0;
    n_d     = 0;
    n_S     = 0;
    dd      = 0;
    hh      = 0;
    mi      = 0;
    ss      = 0;
    ni      = 0;
    fracsecs = 0;
    tol     = 1;

    while (!errn && fmt[fx] && dbuf[*ix]) {
        if (fmt[fx] == '%' && fmt[fx+1]) {
            fx++;
            switch (fmt[fx]) {
                case '%':
                    buf[bx++] = '%';
                    break;

                case 'H':
                    if (dbuf[*ix] == ' ') {
                        (*ix)++;
                        hh = 0;
                    } else if (dbuf[*ix] >= '0' && dbuf[*ix] <= '9') {
                        hh = dbuf[*ix] - '0';
                        (*ix)++;
                    } else errn = (-3601); /* bad hour */
                    if (!errn && dbuf[*ix] != fmt[fx + 1]) {
                        if (dbuf[*ix] >= '0' && dbuf[*ix] <= '9') {
                            hh = (hh * 10) + dbuf[*ix] - '0';
                            (*ix)++;
                        } else if (!tol) errn = (-3601); /* bad hour */
                    }
                    break;

                case 'i':
                    while (!errn && fmt[fx] == 'i' && ni < 6) {
                        if (dbuf[*ix] >= '0' && dbuf[*ix] <= '9') {
                            fracsecs = (fracsecs * 10) + (dbuf[*ix] - '0');
                            (*ix)++;
                        } else errn = (-3612); /* bad fraction */
                        fx++;
                        ni++;
                    }
                    while (ni < 6) {
                        fracsecs *= 10;
                        ni++;
                    }
                    break;

                case 'M':
                    if (dbuf[*ix] == ' ') {
                        (*ix)++;
                        mi = 0;
                    } else if (dbuf[*ix] >= '0' && dbuf[*ix] <= '9') {
                        mi = dbuf[*ix] - '0';
                        (*ix)++;
                    } else errn = (-3602); /* bad minute */
                    if (!errn && dbuf[*ix] != fmt[fx + 1]) {
                        if (dbuf[*ix] >= '0' && dbuf[*ix] <= '9') {
                            mi = (mi * 10) + dbuf[*ix] - '0';
                            (*ix)++;
                        } else errn = (-3602); /* bad minute */
                    }
                    break;

                case 'n':
                    fx++;
                    fdel = fmt[fx + 1];
                    if (fdel == '%' || isdigit(fdel)) errn = (-3604);
                    fdone = 0;
                    switch (fmt[fx]) {
                        case 'd':
                            bx = 0;
                            neg = 0;
                            n_d = 0;
                            while (dbuf[*ix] == ' ') (*ix)++;
                            if (dbuf[*ix] == '-') { neg = 1; (*ix)++; }
                            while (!errn && dbuf[*ix] && !fdone) {
                                if (dbuf[*ix] >= '0' && dbuf[*ix] <= '9') {
                                    n_d = (n_d * 10) + dbuf[*ix] - '0';
                                    (*ix)++;
                                    bx++;
                                } else if (dbuf[*ix] == fdel) {
                                    fdone = 1;
                                } else {
                                    errn = (-3604); /* bad num days */
                                }
                            }
                            if (!errn && neg) n_d = -n_d;
                            break;

                        case 'D':
                            bx = 0;
                            neg = 0;
                            n_d = 0;
                            if (dbuf[*ix] == '-' ||
                                (dbuf[*ix] >= '0' && dbuf[*ix] <= '9')) {
                                if (dbuf[*ix] == '-') { neg = 1; (*ix)++; }
                                while (!errn && dbuf[*ix] && !fdone) {
                                    if (dbuf[*ix] >= '0' && dbuf[*ix] <= '9') {
                                        n_d = (n_d * 10) + dbuf[*ix] - '0';
                                        (*ix)++;
                                        bx++;
                                    } else if (dbuf[*ix] == fdel) {
                                        fdone = 1;
                                    } else {
                                        errn = (-3604); /* bad num days */
                                    }
                                }
                                if (!errn && neg) n_d = -n_d;
                            }
                            break;

                        case 's':
                        case 'S':
                            bx = 0;
                            neg = 0;
                            n_S = 0;
                            while (dbuf[*ix] == ' ') (*ix)++;
                            if (dbuf[*ix] == '-') { neg = 1; (*ix)++; }
                            while (!errn && dbuf[*ix] && !fdone) {
                                if (dbuf[*ix] >= '0' && dbuf[*ix] <= '9') {
                                    n_S = (n_S * 10) + dbuf[*ix] - '0';
                                    (*ix)++;
                                    bx++;
                                } else if (dbuf[*ix] == fdel) {
                                    fdone = 1;
                                } else {
                                    errn = (-3605); /* bad num seconds */
                                }
                            }
                            if (!errn && neg) n_S = -n_S;
                            break;

                        default:
                            errn = (-3609);  /* bad % character */
                            break;
                    }
                    break;

                case 'S':
                    if (dbuf[*ix] == ' ') {
                        ss = 0;
                        (*ix)++;
                    } else if (dbuf[*ix] >= '0' && dbuf[*ix] <= '9') {
                        ss = dbuf[*ix] - '0';
                        (*ix)++;
                    } else errn = (-3603); /* bad second */
                    if (!errn && dbuf[*ix] != fmt[fx + 1]) {
                        if (dbuf[*ix] >= '0' && dbuf[*ix] <= '9') {
                            ss = (ss * 10) + dbuf[*ix] - '0';
                            (*ix)++;
                        } else errn = (-3603); /* bad second */
                    }
                    break;

                default:
                    errn = (-3606);  /* bad % character */
                    break;
            }
            fx++;
        } else {
            if (fmt[fx] == dbuf[*ix]) {
                fx++;
                (*ix)++;
            } else {
                errn = (-3607); /* character doesn't match */
            }
        }
    }

    if (!errn && fmt[fx]) {
        errn = (-3608); /* end of dbuf */
    }

/****************************************************************
 ****************************************************************
 ********              CALCULATIONS HERE                 ********
 ****************************************************************
 ****************************************************************/


    /* Handle days, hours, minutes, seconds */
    if (n_d) {
        if (n_S) {
            errn = (-3610);
        } else {
            ts->ts_second = n_d * 86400;
        }
    } else if (n_S) {
        if (hh || mi || ss) {
            errn = (-3611);
        } else {
            idivmo(&(ts->ts_daynum), &(ts->ts_second), n_S, 86400);
        }
    }

    if (!errn) {
        if (hh) {
            if (hh > 23) {
                errn = (-3613);
            } else {
                ts->ts_second += hh * 3600L;
            }
        }
    }

    if (!errn) {
        if (mi) {
            if (mi > 59) {
                errn = (-3614);
            } else {
                ts->ts_second += mi * 60L;
            }
        }
    }

    if (!errn) {
        if (ss) {
            if (ss > 59) {
                errn = (-3615);
            } else {
                ts->ts_second += ss;
            }
        }
    }

    if (!errn) {
        ts->ts_microsec = fracsecs;
    }

    return (errn);
}
/***************************************************************/
void add_to_timestamp(struct timestamp * ts1, struct timestamp * ts2)
{
/*
** Note: Does not support negative values.
*/
    int carry;

    ts1->ts_microsec += ts2->ts_microsec;
    carry = 0;
    if (ts1->ts_microsec >= 1000000) {
        carry = 1;
        ts1->ts_microsec -= 1000000;
    }

    ts1->ts_second += ts2->ts_second + carry;
    carry = 0;
    if (ts1->ts_second >= 86400) {
        carry = 1;
        ts1->ts_microsec -= 86400;
    }

    ts1->ts_daynum += ts2->ts_daynum + carry;
}
/***************************************************************/
char * add_string_timestamps(const char* tsbuf1,
                const char * tsbuf2,
                const char * fmt)
{
    struct timestamp ts1;
    struct timestamp ts2;
    int tsix1;
    int tsix2;
    int tserr;
    char * tssum;
    char tsbuf[64];

    tssum = NULL;
    tsix1 = 0;
    tsix2 = 0;

    if (tsbuf1) {
        tserr = parse_timestamp(tsbuf1, &tsix1, fmt, &ts1);
        if (!tserr) {
            if (tsbuf2) {
                tserr = parse_timestamp(tsbuf2, &tsix2, fmt, &ts2);
                if (!tserr) {
                    add_to_timestamp(&ts1, &ts2);
                    format_timestamp(&ts1, fmt, tsbuf, sizeof(tsbuf));
                    tssum = Strdup(tsbuf);
                }
            } else {
                tssum = Strdup(tsbuf1);
            }
        }
    } else if (tsbuf2) {
        tserr = parse_timestamp(tsbuf2, &tsix2, fmt, &ts2);
        if (!tserr) tssum = Strdup(tsbuf2);
    }

    return (tssum);
}
/***************************************************************/
int Memicmp(const char * b1, const char * b2, size_t blen)
{
    /*
     ** Case insensitive memcmp
     */
    size_t ii;

    for (ii = 0; ii < blen; ii++) {
        if (tolower(b1[ii]) < tolower(b2[ii])) return (-1);
        else if (tolower(b1[ii]) > tolower(b2[ii])) return (1);
    }
    return (0);
}
/***************************************************************/
struct strlist * strlist_new(void) {

    struct strlist * strl;

    strl = New(struct strlist, 1);
    strl->strl_strs = NULL;
    strl->strl_len = 0;
    strl->strl_max = 0;
    strl->strl_ptr_count = 0;
    strl->strl_last_buflen = 0;

    return (strl);
}
/***************************************************************/
void strlist_free(struct strlist * strl)
{
    int ii;

    if (!strl) return;

    if (strl && strl->strl_strs && !(strl->strl_ptr_count)) {
        for (ii = 0; strl->strl_strs[ii]; ii++) {
            Free(strl->strl_strs[ii]);
        }
        Free(strl->strl_strs);
    }
    Free(strl);
}
/***************************************************************/
void strlist_print(struct strlist * strl)
{
    int ii;
    int slen;

    if (!strl) return;

    if (strl && strl->strl_strs && !(strl->strl_ptr_count)) {
        for (ii = 0; strl->strl_strs[ii]; ii++) {
            slen = IStrlen(strl->strl_strs[ii]);
            if (slen > 0 && strl->strl_strs[ii][slen - 1] == '\n') {
                printf("%d. %s", ii, strl->strl_strs[ii]);
            } else {
                printf("%d. %s\n", ii, strl->strl_strs[ii]);
            }
        }
    }
}
/***************************************************************/
void strlist_append_ptr(struct strlist * strl, char * buf, int buflen)
{

    if (strl->strl_len + 1 >= strl->strl_max) {
        if (!strl->strl_max) strl->strl_max = 16;
        else                 strl->strl_max *= 2;
        strl->strl_strs = Realloc(strl->strl_strs, char*, strl->strl_max);
    }

    strl->strl_strs[strl->strl_len] = buf;
    strl->strl_last_buflen = buflen;
    strl->strl_last_bufmax = buflen + 1;
    strl->strl_len += 1;
    strl->strl_strs[strl->strl_len] = NULL;
}
/***************************************************************/
void strlist_append_str(struct strlist * strl, const char * buf)
{
    int buflen;
    char * nbuf;

    buflen = IStrlen(buf);
    nbuf = New(char, buflen + 1);
    strcpy(nbuf, buf);

    strlist_append_ptr(strl, nbuf, buflen);
}
/***************************************************************/
void strlist_append_str_to_last(struct strlist * strl, const char * buf, int linemax)
{
    int buflen;

    buflen = IStrlen(buf);

    if (!strl->strl_len || buflen + strl->strl_last_buflen + 1 > linemax) {
        if (strl->strl_len + 1 >= strl->strl_max) {
            if (!strl->strl_max) strl->strl_max = 16;
            else                 strl->strl_max *= 2;
            strl->strl_strs = Realloc(strl->strl_strs, char*, strl->strl_max);
        }

        strl->strl_last_bufmax = linemax + 1;
        if (buflen >= linemax) strl->strl_last_bufmax = buflen + 1;
        strl->strl_strs[strl->strl_len] = New(char, strl->strl_last_bufmax);
        strcpy(strl->strl_strs[strl->strl_len], buf);
        strl->strl_last_buflen = buflen;
        strl->strl_len += 1;
        strl->strl_strs[strl->strl_len] = NULL;
    } else {
        if (buflen + strl->strl_last_buflen + 2 > strl->strl_last_bufmax) {
            strl->strl_last_bufmax = linemax + 1;
            if (buflen >= linemax) strl->strl_last_bufmax = buflen + 1;
            strl->strl_strs[strl->strl_len - 1] =
                Realloc(strl->strl_strs[strl->strl_len - 1], char, strl->strl_last_bufmax);
        }
        strl->strl_strs[strl->strl_len - 1][strl->strl_last_buflen++] = ' ';
        strcpy(&(strl->strl_strs[strl->strl_len - 1][strl->strl_last_buflen]), buf);
        strl->strl_last_buflen += buflen;
    }
    /* printf("tokes=\"%s\"\n", strl->strl_strs[strl->strl_len - 1]); */
}
/***************************************************************/
char * strdupx(const char * csrc, int cflags)
{
    int srclen;
    ;
    char * ctgt;

    ctgt = NULL;

    srclen = IStrlen(csrc);
    if (srclen > 2 && csrc[0] == '\"'  && csrc[srclen-1] == csrc[0]) {
        /* src is quoted */
        srclen -= 2;
        if (!(cflags & STRCPYX_REMOVE_QUOTES)) {
                ctgt = New(char, srclen + 3);
                ctgt[0] = '\"';
                memcpy(ctgt + 1, csrc + 1, srclen);
                ctgt[srclen + 1] = '\"';
                ctgt[srclen + 2] = '\0';
        } else {/* src is quoted & STRCPYX_REMOVE_QUOTES */
            ctgt = New(char, srclen + 1);
            memcpy(ctgt, csrc + 1, srclen);
            ctgt[srclen] = '\0';
        }
    } else {    /* src is NOT quoted */
        ctgt = New(char, srclen + 1);
        memcpy(ctgt, csrc, srclen);
        ctgt[srclen] = '\0';
    }

    return (ctgt);
}
/***************************************************************/
