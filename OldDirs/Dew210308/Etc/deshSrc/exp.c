/***************************************************************/
/* exp. c                                                     */
/***************************************************************/
/*

****************************************************************
*/

#include "config.h"

#if IS_WINDOWS
#include <io.h>
#else
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <fcntl.h>
#include <errno.h>

#include "desh.h"
#include "cmd.h"
#include "var.h"
#include "exp.h"
#include "msg.h"
#include "snprfh.h"
#include "cmd2.h"
#include "when.h"
#include "assoc.h"
#include "term.h"

int parse_exp_entry(struct cenv * ce,
            const char * cmdbuf,
            int * pix,
            struct str * parmstr,
            struct var * result); /* forward */

void load_bi_funcs(struct glob * gg);  /* forward */

#define RELOP_NONE      0
#define RELOP_LT        1
#define RELOP_LE        2
#define RELOP_EQ        3
#define RELOP_GE        4
#define RELOP_GT        5
#define RELOP_NE        6

#define DATE_BUF_SZ     128

#if IS_WINDOWS
    #define CON_MAX_DIV_10_64   (922337203685477580i64)
    #define CON_MAX_DIV_16_64   (0x0FFFFFFFFFFFFFFFi64)
    /* #define CON_MAX_64          (9223372036854775807i64) - defined in snprf.h */
    #define CON_MIN_64          (0x8000000000000000i64)
    #define CON_ONE_64          (1i64)
#else
    #define CON_MAX_DIV_10_64   (922337203685477580ll)
    #define CON_MAX_DIV_16_64   (0x0FFFFFFFFFFFFFFFll)
    /* #define CON_MAX_64          (9223372036854775807ll) - defined in snprf.h */
    #define CON_MIN_64          (0x8000000000000000ll)
    #define CON_ONE_64          (1ll)
#endif

/***************************************************************/
/*@*/int hexstoint64 (const char * from, int64 *dest) {
/*
**  Convert from hexidecimal string to signed 64 bit binary
**  Range is: -9223372036854775808 to +9223372036854775807 with leading sign permitted
**  Returns:  0 if successful       ** Opposite of Warehouse stol()
**            1 if error
*/
    int ii;
    int jj;
    int len;
    int64 nn;
    const char *ptr;

    ptr = from;

    len = Istrlen(ptr);
    if (!len) return (1);  /* Don't allow "" */

    nn = 0;
    jj = 0;
    while (jj < len && nn <= CON_MAX_DIV_16_64) {
             if (ptr[jj] >= '0' && ptr[jj] <= '9') ii = ptr[jj] - '0';
        else if (ptr[jj] >= 'A' && ptr[jj] <= 'F') ii = ptr[jj] - '7';
        else if (ptr[jj] >= 'a' && ptr[jj] <= 'f') ii = ptr[jj] - 'W';
        else return (1);

        nn = (nn << 4) | ii;
        jj++;
    }

    if (jj<len) return (1);

    *dest = nn;

    return (0);
}
/***************************************************************/
/*@*/int stoint64 (const char * from, int64 *dest) {
/*
**  Convert from string to signed 64 bit binary
**  Range is: -9223372036854775808 to +9223372036854775807 with leading sign permitted
**  Returns:  0 if successful       ** Opposite of Warehouse stol()
**            1 if error
*/
    int ii;
    int jj;
    int len;
    int64 nn;
    const int64 max_div_10 = CON_MAX_DIV_10_64; /* 2^63 = 9223372036854775808 */
    const char *ptr;
    char sign;

    ptr = from;
    sign = ptr[0];

    if (sign == '0' && (ptr[1] == 'x' || ptr[1] == 'X')) {
        return hexstoint64(ptr + 2, dest);
    }

    if (sign == '-' || sign == '+') ptr++;
    else sign = '+';
    len = Istrlen(ptr);
    if (!len) return (1);  /* Don't allow "", "+", or "-" */

    nn = 0;
    jj = 0;
    while (jj<len && nn<max_div_10) {
        ii = ptr[jj] - '0';
        if (ii < 0 || ii > 9) return (1);
        nn = nn * 10 + ii;
        jj++;
    }

    if (jj<len && nn==max_div_10) {
        ii = 0;
        if (jj < len) ii = ptr[jj] - '0';
        if (ii < 0 || ii > 9) return (1);
        if (ii <= 7 && sign != '-') {
            nn = nn * 10 + ii;
            jj++;
        } else if (ii <= 8 && sign == '-') {
            nn = nn * (-10) - ii;
            jj++;
        }
    } else if (sign == '-') {
        nn = -nn;
    }

    if (jj<len) return (1);

    *dest = nn;

    return (0);
}
/***************************************************************/
/*@*/int hexstoi (const char * from, int *dest) {
/*
**  Convert from hexidecimal string to signed 64 bit binary
**  Range is: -9223372036854775808 to +9223372036854775807 with leading sign permitted
**  Returns:  0 if successful       ** Opposite of Warehouse stol()
**            1 if error
*/
    int ii;
    int jj;
    int len;
    int nn;
    const char *ptr;

    ptr = from;

    len = Istrlen(ptr);
    if (!len) return (1);  /* Don't allow "" */

    nn = 0;
    jj = 0;
    while (jj < len && nn <= 0x0FFFFFFFl) {
             if (ptr[jj] >= '0' && ptr[jj] <= '9') ii = ptr[jj] - '0';
        else if (ptr[jj] >= 'A' && ptr[jj] <= 'F') ii = ptr[jj] - '7';
        else if (ptr[jj] >= 'a' && ptr[jj] <= 'f') ii = ptr[jj] - 'W';
        else return (1);

        nn = (nn << 4) | ii;
        jj++;
    }

    if (jj<len) return (1);

    *dest = nn;

    return (0);
}
/***************************************************************/
/*@*/int stoi(const char * from, int *dest) {
/*
**  Convert from string to signed 32 bit binary
**  Range is: -2147483648 to +2147483647 with leading sign permitted
**  Returns:  0 if successful       ** Opposite of Warehouse stol()
**            1 if error
*/
    int ii;
    int jj;
    int len;
    int nn;
    const int32 max_div_10 = 214748364L;
    const char *ptr;
    char sign;

    *dest = 0;
    ptr = from;
    sign = ptr[0];

    if (sign == '0' && (ptr[1] == 'x' || ptr[1] == 'X')) {
        return hexstoi(ptr + 2, dest);
    }

    if (sign == '-' || sign == '+') ptr++;
    else sign = '+';
    len = Istrlen(ptr);
    if (!len) return (1);  /* Don't allow "", "+", or "-" */

    nn = 0;
    jj = 0;
    while (jj<len && nn<max_div_10) {
        ii = ptr[jj] - '0';
        if (ii < 0 || ii > 9) return (1);
        nn = nn * 10 + ii;
        jj++;
    }

    if (jj<len && nn==max_div_10) {
        ii = 0;
        if (jj < len) ii = ptr[jj] - '0';
        if (ii < 0 || ii > 9) return (1);
        if (ii <= 7 && sign != '-') {
            nn = nn * 10 + ii;
            jj++;
        } else if (ii <= 8 && sign == '-') {
            nn = nn * (-10) - ii;
            jj++;
        }
    } else if (sign == '-') {
        nn = -nn;
    }

    if (jj<len) return (1);

    *dest = nn;

    return (0);
}
/***************************************************************/
int get_relop(const char * rebuf) {

    int relop;

    relop = 0;

    if (rebuf[0] == '<') {
        if (!rebuf[1]) {
            relop = RELOP_LT;
        } else if (rebuf[1] == '=' && !rebuf[2]) {
            relop = RELOP_LE;
        } else if (rebuf[1] == '>' && !rebuf[2]) {
            relop = RELOP_NE;
        }
    } else if (rebuf[0] == '>') {
        if (!rebuf[1]) {
            relop = RELOP_GT;
        } else if (rebuf[1] == '=' && !rebuf[2]) {
            relop = RELOP_GE;
        }
    } else if (rebuf[0] == '=' && !rebuf[1]) {
        relop = RELOP_EQ;
    }

    return (relop);
}
/***************************************************************/
#ifdef OLD_WAY
int get_token_quote_sub(struct cenv * ce,
            const char * expbuf,
            int * pix,
            char quot,
            struct str * tokestr) {

    int estat;
    int subflags;
    struct str substr;

    subflags = SUBPARM_NONE;
    if (quot == '`') subflags = SUBPARM_QUOTE;

    init_str(&substr, 0);
    estat = sub_parm(ce, expbuf, pix, &substr, subflags);
    if (!estat) {
        append_str_len(tokestr, substr.str_buf, substr.str_len);
    }
    free_str_buf(&substr);

    return (estat);
}
/***************************************************************/
int get_token_quote(struct cenv * ce,
            const char * expbuf,
            int * pix,
            struct str * tokestr) {

    int eol;
    char quot;
    int stix;
    char ch;

    eol = 0;
    quot = expbuf[*pix];
    stix = (*pix);
    (*pix) += 1;

    while (quot) {
        ch = expbuf[*pix];
        if (!ch) {
            eol = set_error(ce, EUNMATCHEDQUOTE, NULL);
            quot = 0;
        } else if (ch == quot) {
            (*pix) += 1;
            append_str_len(tokestr, expbuf + stix, (*pix) - stix);
            if (expbuf[*pix] == quot) {
                (*pix) += 1;
                stix = (*pix);
            } else {
                quot = 0;
            }
        } else if (ch == '$' && quot != '\'') {
            append_str_len(tokestr, expbuf + stix, (*pix) - stix);
            (*pix) += 1;
            eol = get_token_quote_sub(ce, expbuf, pix, quot, tokestr);
            stix = (*pix);
        } else if (ch == '^' && quot != '\'') {
            (*pix) += 1;
            if (expbuf[*pix] == '^') {
                append_str_len(tokestr, expbuf + stix, (*pix) - stix);
                (*pix) += 1;
                stix = (*pix);
            }
        } else {
            (*pix) += 1;
        }
    }

    return (eol);
}
#endif
/***************************************************************/
void quote_str(struct str * srcstr) {
/*
** Fixup for token_to_var()
*/
    if (srcstr->str_len + 2 >= srcstr->str_max) {
        srcstr->str_max += 2;
        srcstr->str_buf = Realloc(srcstr->str_buf, char, srcstr->str_max);
    }

    memmove(srcstr->str_buf + 1, srcstr->str_buf, srcstr->str_len);
    srcstr->str_buf[0]                   = '\"';
    srcstr->str_buf[srcstr->str_len + 1] = '\"';
    srcstr->str_buf[srcstr->str_len + 2] = '\0';
    srcstr->str_len += 2;
}
/***************************************************************/
int get_token_quote(struct cenv * ce,
            const char * expbuf,
            int * pix,
            struct str * tokestr) {

    int estat;
    struct str expstr;

    init_str(&expstr, 0);
    set_str_char(&expstr, expbuf);

    estat = get_parmstr(ce, &expstr, pix, PFLAG_QUOTE, tokestr);
    if (!estat) {
        quote_str(tokestr);
    }

    free_str_buf(&expstr);

    return (estat);
}
/***************************************************************/
int is_good_token(struct cenv * ce, const char * toke) {

    int goodtoke;
    int ii;

    goodtoke = 1;
    for (ii = 0; goodtoke && toke[ii]; ii++) {
        if (!IS_VARCHAR(toke[ii])) goodtoke = 0;
    }

    return (goodtoke);
}
/***************************************************************/
int get_token_exp_sub(struct cenv * ce,
            const char * expbuf,
            int * pix,
            struct str * tokestr) {

/*
int get_parmstr(struct cenv * ce,
                struct str * cmdstr,
                int * cmdix,
                int parmflags,
                struct str * parmstr)

                PFLAG_EXP
*/

    int estat;
    int eol;
    int eix;
    struct str expstr;
    struct str substr;

    eol = 0;

    eix = 0;
    init_str(&substr, 0);
    init_str_char(&expstr, expbuf + (*pix));

    estat = get_parmstr(ce, &expstr, &eix, PFLAG_EXP, &substr);
    if (estat) {
        eol = estat;
    } else {
        (*pix) += eix;
        append_str_str(tokestr, &substr);
        if (!tokestr->str_len || !is_good_token(ce, tokestr->str_buf)) {
            eol = set_error(ce, EBADSUBTOKEN, tokestr->str_buf);
        }
    }

    free_str_buf(&expstr);
    free_str_buf(&substr);

    return (eol);
}
/***************************************************************/
int get_token_exp(struct cenv * ce,
            const char * expbuf,
            int * pix,
            struct str * tokestr) {

    int eol;
    char ch;
    int stix;

    eol = 0;

    tokestr->str_len = 0;

    while(isspace(expbuf[*pix])) (*pix)++;

    if (!expbuf[*pix]) {
        eol = ESTAT_EOL;
    } else {
        ch = expbuf[*pix];
        if (ch == '\'' || ch == '\"' || ch == '`') {
            eol = get_token_quote(ce, expbuf, pix, tokestr);
        } else if (isdigit(ch)) {
            stix = (*pix);
            (*pix) += 1;
            while (isalnum(expbuf[*pix])) (*pix) += 1;
            append_str_len(tokestr, expbuf + stix, (*pix) - stix);
            if (expbuf[*pix] == '$') {
                eol = get_token_exp_sub(ce, expbuf, pix, tokestr);
            }
        } else if (IS_FIRST_VARCHAR(ch)) {
            stix = (*pix);
            (*pix) += 1;
            while (IS_VARCHAR(expbuf[*pix])) (*pix) += 1;

            if (ce->ce_flags & CEFLAG_CASE) {
                append_str_len_upper(tokestr, expbuf + stix, (*pix) - stix);
            } else {
                append_str_len(tokestr, expbuf + stix, (*pix) - stix);
            }
            if (expbuf[*pix] == '$') {
                eol = get_token_exp_sub(ce, expbuf, pix, tokestr);
            }
        } else if (ch == '$') {
            eol = get_token_exp_sub(ce, expbuf, pix, tokestr);
        } else {
            APPCH(tokestr, ch);
            (*pix) += 1;
                /* check for <=, <>, >= */
            if ((ch == '<' && (expbuf[*pix] == '=' || expbuf[*pix] == '>'   )) ||
                (ch == '>' &&  expbuf[*pix] == '='                          )) {
                APPCH(tokestr, expbuf[*pix]);
                (*pix) += 1;
            }
        }
    }
    APPCH(tokestr, '\0');

    return (eol);
}
/***************************************************************/
void str_to_var(struct var * tgtvar, const char * srcbuf, int srclen) {

    tgtvar->var_typ = VTYP_STR;
    tgtvar->var_len = srclen;
    tgtvar->var_max = srclen + 1;
    tgtvar->var_buf = New(char, tgtvar->var_max);

    if (srclen > 0) memcpy(tgtvar->var_buf, srcbuf, srclen);
    tgtvar->var_buf[tgtvar->var_len] = '\0';
}
/***************************************************************/
void str_to_var_lower(struct var * tgtvar, const char * srcbuf, int srclen) {

    int ii;
    tgtvar->var_typ = VTYP_STR;
    tgtvar->var_len = srclen;
    tgtvar->var_max = srclen + 1;
    tgtvar->var_buf = New(char, tgtvar->var_max);
    for (ii = 0; ii < srclen; ii++) tgtvar->var_buf[ii] = tolower(srcbuf[ii]);
    tgtvar->var_buf[tgtvar->var_len] = '\0';
}
/***************************************************************/
void str_to_var_upper(struct var * tgtvar, const char * srcbuf, int srclen) {

    int ii;
    tgtvar->var_typ = VTYP_STR;
    tgtvar->var_len = srclen;
    tgtvar->var_max = srclen + 1;
    tgtvar->var_buf = New(char, tgtvar->var_max);
    for (ii = 0; ii < srclen; ii++) tgtvar->var_buf[ii] = toupper(srcbuf[ii]);
    tgtvar->var_buf[tgtvar->var_len] = '\0';
}
/***************************************************************/
struct var * dup_var(struct var * srcvar) {

    struct var * tgtvar;

    tgtvar = New(struct var, 1);

    copy_var(tgtvar, srcvar);

    return (tgtvar);
}
/***************************************************************/
void free_var(struct var * srcvar) {

    if (srcvar->var_typ == VTYP_STR) {
        Free(srcvar->var_buf);
    }
    Free(srcvar);
}
/***************************************************************/
void free_parms(int parmc, struct var ** parmv) {

    int ii;

    for (ii = parmc - 1; ii >= 0; ii--) free_var(parmv[ii]);
    Free(parmv);
}
/***************************************************************/
void copy_var(struct var * tgtvar, struct var * srcvar) {

    if (srcvar->var_typ == VTYP_STR) {
        str_to_var(tgtvar, srcvar->var_buf, srcvar->var_len);
    } else {
        tgtvar->var_typ = srcvar->var_typ;
        tgtvar->var_val = srcvar->var_val;
    }
}
/***************************************************************/
void replace_var(struct var * tgtvar, struct var * srcvar) {
/*
** Same as copy_var(), but tgtvar is expected to be a good var
*/

    if (srcvar->var_typ == VTYP_STR) {
        if (tgtvar->var_typ == VTYP_STR) {
            if (tgtvar->var_max <= srcvar->var_len) {
                tgtvar->var_max = srcvar->var_len + 1;
                tgtvar->var_buf = Realloc(tgtvar->var_buf, char, tgtvar->var_max);
            }
            tgtvar->var_len = srcvar->var_len;
            memcpy(tgtvar->var_buf, srcvar->var_buf, srcvar->var_len);
            tgtvar->var_buf[tgtvar->var_len] = '\0';
        } else {
            str_to_var(tgtvar, srcvar->var_buf, srcvar->var_len);
        }
    } else {
        if (tgtvar->var_typ == VTYP_STR) free_var_buf(tgtvar);
        tgtvar->var_typ = srcvar->var_typ;
        tgtvar->var_val = srcvar->var_val;
    }
}
/***************************************************************/
int cmd_to_str_var(struct cenv * ce,
                struct var * tgtvar,
                const char * srcbuf,
                int srclen) {

    int estat;
    char * srccmd;
    struct str cmdout;

    estat = 0;
    init_str(&cmdout, 0);

    srccmd = New(char, srclen + 1);
    memcpy(srccmd, srcbuf, srclen);
    srccmd[srclen] = '\0';

    estat = execute_command_string(ce, srccmd, &cmdout);

    str_to_var(tgtvar, cmdout.str_buf, cmdout.str_len);
    free_str_buf(&cmdout);
    Free(srccmd);

    return (estat);
}
/***************************************************************/
int token_to_var(struct cenv * ce, struct var * tgtvar, struct str * srcstr) {

    int estat;
    struct var * vval;
    char * gvval;
    struct str vnam;

    estat = 0;

    if (tgtvar->var_typ == VTYP_NULL) {
        return (estat);
    }

    if (!srcstr || !srcstr->str_buf || !srcstr->str_len) {
        tgtvar->var_max = VAR_CHUNK_SIZE;
        tgtvar->var_buf = New(char, tgtvar->var_max);
        tgtvar->var_buf[0] = '\0';
        tgtvar->var_len = 0;
        tgtvar->var_typ = VTYP_STR;

    } else if (srcstr->str_len >= 2 &&
               (srcstr->str_buf[0] == '\"' || srcstr->str_buf[0] == '\'') && 
               (srcstr->str_buf[0] == srcstr->str_buf[srcstr->str_len - 1])) {
        str_to_var(tgtvar, srcstr->str_buf + 1, srcstr->str_len - 2);

    } else if (isdigit(srcstr->str_buf[0])) {
        tgtvar->var_typ = VTYP_INT;
        if (stoint64(srcstr->str_buf, &(tgtvar->var_val))) {
            estat = set_error(ce, EBADNUM, srcstr->str_buf);
        }
    } else if (IS_FIRST_VARCHAR(srcstr->str_buf[0])) {
        copy_vnam(ce, &(vnam.str_buf),  &(vnam.str_len),  &(vnam.str_max), srcstr);

        vval = find_var(ce, vnam.str_buf, vnam.str_len);
        if (vval) {
            copy_var(tgtvar, vval);
        } else {
            gvval = find_globalvar_ex(ce, vnam.str_buf, vnam.str_len);
            if (gvval) {
                tgtvar->var_max = Istrlen(gvval) + 1;
                tgtvar->var_buf = New(char, tgtvar->var_max);
                strcpy(tgtvar->var_buf, gvval);
                tgtvar->var_len = tgtvar->var_max - 1;
                tgtvar->var_typ = VTYP_STR;
            } else if (!CESTRCMP(ce, vnam.str_buf, "false")) {
                tgtvar->var_typ = VTYP_BOOL;
                tgtvar->var_val = 0;
            } else if (!CESTRCMP(ce, vnam.str_buf, "true")) {
                tgtvar->var_typ = VTYP_BOOL;
                tgtvar->var_val = 1;
            } else {
                estat = set_error(ce, EBADID, vnam.str_buf);
            }
        }
        free_str_buf(&vnam);
    } else if (srcstr->str_buf[0] == '`') {
        estat = cmd_to_str_var(ce, tgtvar, srcstr->str_buf + 1, srcstr->str_len - 2);
    } else {
        estat = set_error(ce, EUNEXPTOK, srcstr->str_buf);
    }
    free_str_buf(srcstr);

    return (estat);
}
/***************************************************************/
void free_var_buf(struct var * vv) {

    if (vv->var_typ ==  VTYP_STR) {
        Free(vv->var_buf);
        vv->var_len = 0;
        vv->var_max = 0;
        vv->var_buf = NULL;
    }
}
/***************************************************************/
void vensure_str(struct cenv * ce, struct var * vv) {

/* must return some string */

    switch (vv->var_typ) {
        case VTYP_NULL:
            vv->var_max = 16;
            vv->var_buf = New(char, vv->var_max);
            vv->var_buf[0] = '\0';
            vv->var_len = 0;
            vv->var_typ = VTYP_STR;
            break;

        case VTYP_STR:
            /* do nothing */
            break;

        case VTYP_INT:
            /*
            **  Range is: -9223372036854775808 to 9223372036854775807
            */
            vv->var_max = 22;
            vv->var_buf = New(char, vv->var_max);
            Snprintf(vv->var_buf, vv->var_max, "%Ld", vv->var_val);
            vv->var_len = Istrlen(vv->var_buf);
            vv->var_typ = VTYP_STR;
            break;

        case VTYP_BOOL:
            if (ce->ce_flags & CEFLAG_CASE) {
                if (vv->var_val) {
                    vv->var_max = 5;
                    vv->var_buf = New(char, vv->var_max);
                    memcpy(vv->var_buf, "TRUE", vv->var_max);
                } else {
                    vv->var_max = 6;
                    vv->var_buf = New(char, vv->var_max);
                    memcpy(vv->var_buf, "FALSE", vv->var_max);
                }
            } else {
                if (vv->var_val) {
                    vv->var_max = 5;
                    vv->var_buf = New(char, vv->var_max);
                    memcpy(vv->var_buf, "true", vv->var_max);
                } else {
                    vv->var_max = 6;
                    vv->var_buf = New(char, vv->var_max);
                    memcpy(vv->var_buf, "false", vv->var_max);
                }
            }

            vv->var_len = vv->var_max - 1;
            vv->var_typ = VTYP_STR;
            break;

        default:
            vv->var_buf = Smprintf("** Invalid variable type %d **", vv->var_typ);
            vv->var_len = Istrlen(vv->var_buf);
            vv->var_max = vv->var_len + 1;
            vv->var_typ = VTYP_STR;
            break;
    }
}
/***************************************************************/
int chartoint64(const char * from, int nlen, int64 *dest) {

    int ii;
    int nl;
    int badnum = 0;
    char nbuf[64];

    if (!from) {
        (*dest) = 0;
    } else {
        ii = 0;
        nl = 0;
        while (ii < nlen && isspace(from[ii])) ii++;
        if (ii < nlen && (from[ii] == '+' || from[ii] == '-')) nbuf[nl++] = from[ii++];
        if (ii == nlen) {
            (*dest) = 0;
            badnum = 1; /* added 3/16/2014 */
        } else {
            while (ii < nlen && nl < sizeof(nbuf) && isdigit(from[ii])) nbuf[nl++] = from[ii++];
            while (ii < nlen && isspace(from[ii])) ii++;
            if (ii < nlen) {
                badnum = 1;
            } else {
                nbuf[nl] = '\0';
                badnum = stoint64(nbuf, dest);
            }
        }
    }

    return (badnum);
}
/***************************************************************/
int vensure_num(struct cenv * ce, struct var * vv) {

    int badnum;

    switch (vv->var_typ) {
         case VTYP_NULL:
            badnum = 0;
            break;

       case VTYP_STR:
            badnum = chartoint64(vv->var_buf, vv->var_len, &(vv->var_val));
            if (!badnum) {
                vv->var_typ = VTYP_INT;
                Free(vv->var_buf);
            }
            break;

        case VTYP_INT:
            badnum = 0;
            break;

        case VTYP_BOOL:
            badnum = 0;
            vv->var_typ = VTYP_INT;
            break;

        default:
            badnum = 1;
            break;
    }

    return (badnum);
}
/***************************************************************/
int chartobool(const char * from, int nlen, int64 *dest) {

    int ii;
    int nl;
    int badbool = 0;
    char nbuf[64];

    if (!from) {
        (*dest) = 0;
    } else {
        ii = 0;
        nl = 0;
        while (ii < nlen && isspace(from[ii])) ii++;
        if (ii == nlen) {
            (*dest) = 0;
            badbool = 1; /* added 3/16/2014 */
        } else {
            while (ii < nlen && nl < sizeof(nbuf) && isalpha(from[ii])) nbuf[nl++] = from[ii++];
            while (ii < nlen && isspace(from[ii])) ii++;
            if (ii < nlen) {
                badbool = 1;
            } else {
                nbuf[nl] = '\0';
                if (!Stricmp(nbuf, "TRUE" ) || !Stricmp(nbuf, "T") ||
                    !Stricmp(nbuf, "YES"  ) || !Stricmp(nbuf, "Y") ) {
                    (*dest) = 1;
                } else if (!Stricmp(nbuf, "FALSE") || !Stricmp(nbuf, "F") ||
                           !Stricmp(nbuf, "NO"   ) || !Stricmp(nbuf, "N") ) {
                    (*dest) = 0;
                } else {
                    badbool = stoint64(nbuf, dest);
                    if (!badbool && (*dest) != 0) (*dest) = 1;
                }
            }
        }
    }

    return (badbool);
}
/***************************************************************/
int vensure_bool(struct cenv * ce, struct var * vv) {

    int badbool;

    switch (vv->var_typ) {
         case VTYP_NULL:
            badbool = 0;
            break;

       case VTYP_STR:
            badbool = chartobool(vv->var_buf, vv->var_len, &(vv->var_val));
            if (!badbool) {
                vv->var_typ = VTYP_BOOL;
                Free(vv->var_buf);
            }
            break;

        case VTYP_INT:
            vv->var_typ = VTYP_BOOL;
            if (vv->var_val != 0) vv->var_val = 1;
            badbool = 0;
            break;

        case VTYP_BOOL:
            badbool = 0;
            break;

        default:
            /* do nothing ? */
            badbool = 0;
            break;
    }

    return (badbool);
}
/***************************************************************/
int bif_addtodate(struct cenv * ce,
            int parmc,
            struct var ** parmv,
            struct var * result) {

/*
    See also: bif_subfromdate()
    <date> = addtodate(<fromdate>, <fromfmt>, <interval>, <intervalfmt>, <destfmt>)
*/
    int estat;
    int derrn;
    char * fromdate;
    char * fromfmt;
    char * interval;
    char * intervalfmt;
    char * destfmt;
    char datbuf[DATE_BUF_SZ];

    estat = 0;

    fromdate    = parmv[0]->var_buf;
    fromfmt     = parmv[1]->var_buf;
    interval    = parmv[2]->var_buf;
    intervalfmt = parmv[3]->var_buf;
    destfmt     = parmv[4]->var_buf;

    derrn = dateaddsub(fromdate, fromfmt, interval, intervalfmt, 0,
                            destfmt, datbuf, sizeof(datbuf));
    if (derrn) {
        get_pd_errmsg(derrn, datbuf, sizeof(datbuf));
        estat = set_error(ce, EADDTODATE, datbuf);
    } else {
        str_to_var(result, datbuf, Istrlen(datbuf));
    }

    return (estat);
}
/***************************************************************/
int bif_argv(struct cenv * ce,
            int parmc,
            struct var ** parmv,
            struct var * result) {

/*
**  Syntax: ARGV(num)
*/
    int estat;
    int parm;

    estat = 0;
    parm = (int)(parmv[0]->var_val);

    if (parm < 0 || parm >= ce->ce_argc) {
        str_to_var(result, "", 0);
    } else {
        str_to_var(result, ce->ce_argv[parm], Istrlen(ce->ce_argv[parm]));
    }

    return (estat);
}
/***************************************************************/
int bif_basename(struct cenv * ce,
            int parmc,
            struct var ** parmv,
            struct var * result) {

/*
** See: http://pubs.opengroup.org/onlinepubs/9699919799/utilities/basename.html
*/
    int estat;
    int eix;
    int stix;
    int ix;

    estat = 0;
    stix = has_drive_name(parmv[0]->var_buf);

    if (parmv[0]->var_len == 0 ||
        (parmv[0]->var_len == 1 && parmv[0]->var_buf[0] == DIR_DELIM)) {
        str_to_var(result, parmv[0]->var_buf, parmv[0]->var_len);
    } else {
        eix = parmv[0]->var_len;
        if (parmv[0]->var_buf[eix-1] == DIR_DELIM) eix--;
        ix = eix;
        while (ix > stix && parmv[0]->var_buf[ix-1] != DIR_DELIM) ix--;
        str_to_var(result, parmv[0]->var_buf + ix, eix - ix);
    }

    return (estat);
}
/***************************************************************/
int bif_char(struct cenv * ce,
            int parmc,
            struct var ** parmv,
            struct var * result) {

/*
**  Syntax: CHAR(num)
**
** See also: CODE()
*/
    int estat;
    int parm;
    char ch;

    estat = 0;
    parm = (int)(parmv[0]->var_val);

    if (parm < 0 || parm > 255) {
        str_to_var(result, "", 0);
    } else {
        ch = (char)parm;
        str_to_var(result, &ch, 1);
    }

    return (estat);
}
/***************************************************************/
int bif_chbase(struct cenv * ce,
            int parmc,
            struct var ** parmv,
            struct var * result) {

/*
**  Syntax: CHBASE(numstr, frombase, tobase)
*/
    int estat;
    int64 num;
    uint64 unum;
    int64 maxnum;
    int frombase;
    int tobase;
    char out[80];
    int outix;
    int outlen;
    int frix;
    int frlen;
    int neg;
    int dig;
    const char * from;

    estat = 0;
    frombase  = (int)(parmv[1]->var_val);
    tobase    = (int)(parmv[2]->var_val);
    outix = 0;
    num = 0;
    neg = 0;
    from = parmv[0]->var_buf;
    frlen = parmv[0]->var_len;
    frix = 0;

    if (!from[frix] || (from[frix]  == '-' && !from[frix+1])) {
        estat = set_ferror(ce, EBADBASENUM, "%d: %s", frombase, from);
    } else {
        switch (frombase) {
            case 2:
                if (from[frix] == '-') { neg = 1; frix++; }
                else if (from[frix] == '+') { frix++; }
                maxnum = (((CON_ONE_64 << 60) - 1) << 3) | 7;
                while (!estat && frix < frlen && num <= maxnum) {
                         if (from[frix] >= '0' && from[frix] <= '1') dig = from[frix] - '0';
                    else {
                        estat = set_ferror(ce, EBADBASENUM, "%d: %s", frombase, from);
                    }

                    num = (num << 1) | dig;
                    frix++;
                }
                break;
 
            case 4:
                if (from[frix] == '-') { neg = 1; frix++; }
                else if (from[frix] == '+') { frix++; }
                maxnum = (((CON_ONE_64 << 60) - 1) << 2) | 3;
                while (!estat && frix < frlen && num <= maxnum) {
                         if (from[frix] >= '0' && from[frix] <= '3') dig = from[frix] - '0';
                    else {
                        estat = set_ferror(ce, EBADBASENUM, "%d: %s", frombase, from);
                    }

                    num = (num << 2) | dig;
                    frix++;
                }
                break;
 
            case 8:
                if (from[frix] == '-') { neg = 1; frix++; }
                else if (from[frix] == '+') { frix++; }
                maxnum = (((CON_ONE_64 << 60) - 1) << 1) | 1;
                while (!estat && frix < frlen && num <= maxnum) {
                         if (from[frix] >= '0' && from[frix] <= '7') dig = from[frix] - '0';
                    else {
                        estat = set_ferror(ce, EBADBASENUM, "%d: %s", frombase, from);
                    }

                    num = (num << 3) | dig;
                    frix++;
                }
                break;
 
            case 10:
                if (stoint64(from, &num)) {
                    estat = set_ferror(ce, EBADBASENUM, "%d: %s", frombase, from);
                }
                break;
 
            case 16:
                if (from[frix] == '-') { neg = 1; frix++; }
                else if (from[frix] == '+') { frix++; }
                maxnum = (CON_ONE_64 << 60) - 1;
                while (!estat && frix < frlen && num <= maxnum) {
                         if (from[frix] >= '0' && from[frix] <= '9') dig = from[frix] - '0';
                    else if (from[frix] >= 'A' && from[frix] <= 'F') dig = from[frix] - '7';
                    else if (from[frix] >= 'a' && from[frix] <= 'f') dig = from[frix] - 'W';
                    else {
                        estat = set_ferror(ce, EBADBASENUM, "%d: %s", frombase, from);
                    }

                    num = (num << 4) | dig;
                    frix++;
                }
                break;

            default:
                estat = set_ferror(ce, EBADBASE, "%d", frombase);
                break;
        }
    }
 
    /* check for neg and num == MIN 64 */
    if (!estat && neg) {
        if (num == CON_MIN_64) {
            estat = set_ferror(ce, EBADBASENUM, "%d: %s", frombase, from);
        } else {
            num = -num;
        }
    }

    outix = sizeof(out) - 1;
    out[outix--] = '\0';
    outlen = 0;

    if (!estat) {
        switch (tobase) {
            case 2:
                memcpy(&unum, &num, sizeof(unum));
                while (unum) {
                    dig = (int)(unum & 1);
                    out[outix--] = '0' + dig;
                    outlen++;
                    unum >>= 1;
                }
                break;
 
            case 4:
                memcpy(&unum, &num, sizeof(unum));
                while (unum) {
                    dig = (int)(unum & 3);
                    out[outix--] = '0' + dig;
                    outlen++;
                    unum >>= 2;
                }
                break;
 
            case 8:
                memcpy(&unum, &num, sizeof(unum));
                while (unum) {
                    dig = (int)(unum & 7);
                    out[outix--] = '0' + dig;
                    outlen++;
                    unum >>= 3;
                }
                break;
 
            case 10:
                neg = 0;
                if (num < 0) {
                    neg = 1;
                    if (num == CON_MIN_64) {
                        out[outix--] = '8';
                        outlen++;
                        num = CON_MAX_DIV_10_64;
                    } else {
                        num = -num;
                    }
                }
                while (num) {
                    dig = (int)(num % 10);
                    out[outix--] = '0' + dig;
                    outlen++;
                    num /= 10;
                }
                if (neg) {
                    out[outix--] = '-';
                    outlen++;
                }
                break;
 
            case 16:
                memcpy(&unum, &num, sizeof(unum));
                while (unum) {
                    dig = (int)(unum & 15);
                    if (dig <= 9) out[outix--] = '0' + dig;
                    else          out[outix--] = '7' + dig;
                    outlen++;
                    unum >>= 4;
                }
                break;

            default:
                estat = set_ferror(ce, EBADBASE, "%d", frombase);
                break;
        }
    }

    if (!outlen) {
        out[outix--] = '0';
        outlen++;
    }

    if (!estat) {
        str_to_var(result, out + outix + 1, outlen);
    }

    return (estat);
}
/***************************************************************/
int bif_code(struct cenv * ce,
            int parmc,
            struct var ** parmv,
            struct var * result) {

/*
**  Syntax: CODE(num)
**
** See also: CHAR()
*/
    int estat;

    estat = 0;

    if (parmv[0]->var_len) result->var_val = parmv[0]->var_buf[0];
    else                   result->var_val = -1;
    result->var_typ = VTYP_INT;

    return (estat);
}
/***************************************************************/
int bif_dirname(struct cenv * ce,
            int parmc,
            struct var ** parmv,
            struct var * result) {

/*
** See: http://pubs.opengroup.org/onlinepubs/9699919799/utilities/dirname.html
*/
    int estat;
    int done;
    int ix;
    int stix;
    int elen;
    int dlen;
    char dnam[4];

    estat = 0;
    done = 0;
    stix = has_drive_name(parmv[0]->var_buf);

    /* 1. check for all <slash><slash> */
    if (parmv[0]->var_len == 2 &&
        parmv[0]->var_buf[0] == DIR_DELIM && parmv[0]->var_buf[1] == DIR_DELIM) {
        elen = 2;
    } else {
        /* 2. check for all <slash> */
        if (parmv[0]->var_len) {
            ix = stix;
            while (ix < parmv[0]->var_len && parmv[0]->var_buf[ix] == DIR_DELIM) ix++;
            if (ix == parmv[0]->var_len) {
                if (stix) memcpy(dnam, parmv[0]->var_buf, stix);
                dlen = stix;
                dnam[dlen++] = DIR_DELIM;
                str_to_var(result, dnam, dlen);
                done = 1;
            }
        }

        if (!done) {
            elen = parmv[0]->var_len;
            /* 3. remove trailing <slash> */
            if (parmv[0]->var_buf[elen-1] == DIR_DELIM) elen--;
            ix = elen - 1;
            while (ix >= stix && parmv[0]->var_buf[ix] != DIR_DELIM) ix--;
            /* 4. check for no <slash> */
            if (ix < stix) {
                if (stix) memcpy(dnam, parmv[0]->var_buf, stix);
                dlen = stix;
                dnam[dlen++] = '.';
                str_to_var(result, dnam, dlen);
                done = 1;
            } else {
                /* 5. remove after trailing <slash> */
                elen = ix;
            }
        }
    }

    if (!done) {
        /* 7. remove trailing <slash> */
        ix = elen - 1;
        while (ix >= stix && parmv[0]->var_buf[ix] == DIR_DELIM) ix--;
        if (ix < stix) {
            if (stix) memcpy(dnam, parmv[0]->var_buf, stix);
            dlen = stix;
            dnam[dlen++] = DIR_DELIM;
            str_to_var(result, dnam, dlen);
        } else {
            str_to_var(result, parmv[0]->var_buf, ix + 1);
        }
    }

    return (estat);
}
/***************************************************************/
int bif_extname(struct cenv * ce,
            int parmc,
            struct var ** parmv,
            struct var * result) {

/*
*/
    int estat;
    int ix;

    estat = 0;

    ix = parmv[0]->var_len;
    while (ix > 0 &&
        parmv[0]->var_buf[ix] != DIR_DELIM && parmv[0]->var_buf[ix] != '.') ix--;
    if (ix >= 0  && parmv[0]->var_buf[ix] == '.') {
        str_to_var(result, parmv[0]->var_buf + ix, parmv[0]->var_len - ix);
    } else {
        str_to_var(result, "", 0);
    }

    return (estat);
}
/***************************************************************/
int bif_find(struct cenv * ce,
            int parmc,
            struct var ** parmv,
            struct var * result) {

/*
**  Syntax: FIND(str-to-find, within-str)
*/
    int estat;
    char * fstr;

    estat = 0;

    if (!parmv[0]->var_len) {
        result->var_val = 1;
    } else {
        fstr  = strstr(parmv[1]->var_buf, parmv[0]->var_buf);

        if (fstr) result->var_val = (fstr - parmv[1]->var_buf) + 1;
        else      result->var_val = 0;
    }
    result->var_typ = VTYP_INT;

    return (estat);
}
/***************************************************************/
int bif_finfo(struct cenv * ce,
            int parmc,
            struct var ** parmv,
            struct var * result) {

/*
**  Syntax: FINFO(file-name, info-type)
*/
    int estat;
    int infok;
    int ftyp;
    struct file_stat_rec fsr;
    char * fname;
    char uidbuf[64];
    char gidbuf[64];
    char mdatebuf[32];
    char errmsg[128];

    estat = 0;
    infok = 0;
    fname = parmv[0]->var_buf;

    switch (parmv[1]->var_buf[0]) {

        case 'a': case 'A':
            if (!Stricmp(parmv[1]->var_buf, "ACCDATE")) {
                infok = 1;
                if (get_file_stat(fname, &fsr, errmsg, sizeof(errmsg))) {
                    estat = set_ferror(ce, EFILESTAT,  "%s: %s", fname, errmsg);
                } else {
                    fmt_d_date(fsr.fsr_accdate, "%Y-%m-%d %H:%M:%S", mdatebuf, sizeof(mdatebuf));
                    str_to_var(result, mdatebuf, Istrlen(mdatebuf));
                }
            }
            break;


        case 'c': case 'C':
            if (!Stricmp(parmv[1]->var_buf, "CRDATE")) {
                infok = 1;
                if (get_file_stat(fname, &fsr, errmsg, sizeof(errmsg))) {
                    estat = set_ferror(ce, EFILESTAT,  "%s: %s", fname, errmsg);
                } else {
                    fmt_d_date(fsr.fsr_crdate, "%Y-%m-%d %H:%M:%S", mdatebuf, sizeof(mdatebuf));
                    str_to_var(result, mdatebuf, Istrlen(mdatebuf));
                }
            }
            break;


        case 'm': case 'M':
            if (!Stricmp(parmv[1]->var_buf, "MODDATE")) {
                infok = 1;
                if (get_file_stat(fname, &fsr, errmsg, sizeof(errmsg))) {
                    estat = set_ferror(ce, EFILESTAT,  "%s: %s", fname, errmsg);
                } else {
                    fmt_d_date(fsr.fsr_moddate, "%Y-%m-%d %H:%M:%S", mdatebuf, sizeof(mdatebuf));
                    str_to_var(result, mdatebuf, Istrlen(mdatebuf));
                }
            }
            break;

        case 'o': case 'O':
            if (!Stricmp(parmv[1]->var_buf, "OWNER")) {
                infok = 1;
                if (get_file_stat(fname, &fsr, errmsg, sizeof(errmsg))) {
                    estat = set_ferror(ce, EFILESTAT, "%s: %s", fname, errmsg);
                } else {
                    if (get_file_owner(fname, fsr.fsr_uid,
                                uidbuf, sizeof(uidbuf),
                                gidbuf, sizeof(gidbuf),
                                errmsg, sizeof(errmsg))) {
                        estat = set_ferror(ce, EBADFINFOOWNER, "%s: %s", fname, errmsg);
                    } else {
                        str_to_var(result, uidbuf, Istrlen(uidbuf));
                    }
                }
            }
            break;

        case 's': case 'S':
            if (!Stricmp(parmv[1]->var_buf, "SIZE")) {
                infok = 1;
                if (get_file_stat(fname, &fsr, errmsg, sizeof(errmsg))) {
                    estat = set_ferror(ce, EFILESTAT,  "%s: %s", fname, errmsg);
                } else {
                    num_to_var(result, fsr.fsr_size);
                }
            }
            break;

        case 't': case 'T':
            if (!Stricmp(parmv[1]->var_buf, "TYPE")) {
                infok = 1;
                ftyp = file_stat(fname);
                switch (ftyp) {
                    case FTYP_NONE:
                        str_to_var(result, "NONE", 4);
                        break;
                    case FTYP_FILE:
                        str_to_var(result, "FILE", 4);
                        break;
                    case FTYP_DIR:
                    case FTYP_DOT_DIR:
                        str_to_var(result, "DIR", 3);
                        break;
                    case FTYP_LINK:
                        str_to_var(result, "LINK", 4);
                        break;
                    case FTYP_PIPE:
                        str_to_var(result, "PIPE", 4);
                        break;
                    case FTYP_OTHR:
                        str_to_var(result, "OTHER", 5);
                        break;
                    default:
                        str_to_var(result, "UNKNOWN", 7); /* should not happen */
                        break;
               }
            }
            break;
        default:
            break;
    }

    if (!estat && !infok) {
        estat = set_error(ce, EBADFINFOTYPE, parmv[1]->var_buf);
    }

    return (estat);
}
/***************************************************************/
int bif_left(struct cenv * ce,
            int parmc,
            struct var ** parmv,
            struct var * result) {

/*
**  Syntax: LEFT(str, len)
*/
    int estat;
    int llen;

    estat = 0;
    llen = (int)(parmv[1]->var_val);

    if (llen >= parmv[0]->var_len) {
        copy_var(result, parmv[0]);
    } else if (llen <= 0) {
        str_to_var(result, "", 0);
    } else {
        str_to_var(result, parmv[0]->var_buf, llen);
    }

    return (estat);
}
/***************************************************************/
int bif_len(struct cenv * ce,
            int parmc,
            struct var ** parmv,
            struct var * result) {

/*
**  Syntax: LEN(str)
*/
    int estat;

    estat = 0;

    result->var_val = parmv[0]->var_len;
    result->var_typ = VTYP_INT;

    return (estat);
}
/***************************************************************/
int bif_lower(struct cenv * ce,
            int parmc,
            struct var ** parmv,
            struct var * result) {

/*
*/
    int estat;

    estat = 0;

    str_to_var_lower(result, parmv[0]->var_buf, parmv[0]->var_len);

    return (estat);
}
/***************************************************************/
int bif_matches(struct cenv * ce,
            int parmc,
            struct var ** parmv,
            struct var * result) {

/*
**  Syntax: MATCHES(str, pattern)
*/
    int estat;
    int matches;

    estat = 0;
    matches  = patmatches(0, parmv[1]->var_buf, parmv[0]->var_buf);

    if (matches) result->var_val = 1;
    else         result->var_val = 0;
    result->var_typ = VTYP_BOOL;

    return (estat);
}
/***************************************************************/
int bif_mid(struct cenv * ce,
            int parmc,
            struct var ** parmv,
            struct var * result) {

/*
**  Syntax: MID(str, index, len)        <-- First index is 1
*/
    int estat;
    int mix;
    int llen;

    estat = 0;
    mix  = (int)(parmv[1]->var_val) - 1;
    llen = (int)(parmv[2]->var_val);

    if (mix < 0 || mix >= parmv[0]->var_len) {
        str_to_var(result, "", 0);
    } else {
        if (llen > parmv[0]->var_len - mix) llen = parmv[0]->var_len - mix;

        if (llen <= 0) {
            str_to_var(result, "", 0);
        } else {
            str_to_var(result, parmv[0]->var_buf + mix, llen);
        }
    }

    return (estat);
}
/***************************************************************/
int bif_now(struct cenv * ce,
            int parmc,
            struct var ** parmv,
            struct var * result) {

/*
**  Syntax: NOW(date-fmt-str)
*/
    int estat;
    int derrn;
    char datbuf[DATE_BUF_SZ];

    estat = 0;

    derrn = fmt_d_date(get_time_now(ce->ce_gg->gg_is_dst),
                    parmv[0]->var_buf, datbuf, sizeof(datbuf));
    if (derrn) {
        get_pd_errmsg(derrn, datbuf, sizeof(datbuf));
        estat = set_error(ce, EDATEFMT, datbuf);
    } else {
        str_to_var(result, datbuf, Istrlen(datbuf));
    }

    return (estat);
}
/***************************************************************/
int bif_prompt(struct cenv * ce,
            int parmc,
            struct var ** parmv,
            struct var * result) {

/*
*/
    int estat;
    int infil;
    struct filinf * fi;
    struct str linstr;

    init_str(&linstr, INIT_STR_CHUNK_SIZE);
    infil = DUP(ce->ce_stdin_f);
    fi = new_filinf(infil, "stdin", CMD_LINEMAX);

    estat = filinf_pair(ce, fi, ce->ce_stdout_f, "stdout");

    if (!estat) estat = read_line(ce, fi, &linstr, PROMPT_STR, parmv[0]->var_buf);

    if (!estat) str_to_var(result, linstr.str_buf, linstr.str_len);

    free_filinf(fi);
    free_str_buf(&linstr);

    return (estat);
}
/***************************************************************/
int bif_rept(struct cenv * ce,
            int parmc,
            struct var ** parmv,
            struct var * result) {

/*
**  Syntax: REPT(str, num)
*/
    int estat;
    int nrpt;
    int newlen;
    int ii;
    char * tgt;
    char * src;
    int slen;

    estat = 0;
    nrpt = (int)(parmv[1]->var_val);
    slen = parmv[0]->var_len;
    src  = parmv[0]->var_buf;

    if (nrpt <= 0 || !slen) {
        str_to_var(result, "", 0);
    } else {
        newlen = nrpt * slen;
        if (newlen > ONE_MEGABYTE) {
            estat = set_ferror(ce, EREPTOOBIG, "%ld", newlen);
        } else {
            result->var_typ = VTYP_STR;
            result->var_len = newlen;
            result->var_max = newlen + 1;
            result->var_buf = New(char, result->var_max);
            tgt = result->var_buf;

            for (ii = 0; ii < nrpt; ii++) {
                memcpy(tgt, src, slen);
                tgt += slen;
            }
            (*tgt) = '\0';
        }
    }

    return (estat);
}
/***************************************************************/
int bif_right(struct cenv * ce,
            int parmc,
            struct var ** parmv,
            struct var * result) {

/*
**  Syntax: RIGHT(str, len)
*/
    int estat;
    int llen;

    estat = 0;
    llen = (int)(parmv[1]->var_val);

    if (llen >= parmv[0]->var_len) {
        copy_var(result, parmv[0]);
    } else if (llen <= 0) {
        str_to_var(result, "", 0);
    } else {
        str_to_var(result, parmv[0]->var_buf + parmv[0]->var_len - llen, llen);
    }

    return (estat);
}
/***************************************************************/
int bif_sleep(struct cenv * ce,
            int parmc,
            struct var ** parmv,
            struct var * result) {

/*
**  Syntax: SLEEP(millisecs)
*/
    int estat;
    int millisecs;
    int time_rem;

    estat = 0;
    millisecs = (int)(parmv[0]->var_val);
    time_rem  = os_sleep(millisecs);

    result->var_val = time_rem;
    result->var_typ = VTYP_INT;

    return (estat);
}
/***************************************************************/
int bif_subdates(struct cenv * ce,
            int parmc,
            struct var ** parmv,
            struct var * result) {

/*
    <interval> = subdates(<date1>, <date1fmt>, <date2>, <date2fmt>, <intervalfmt>)
*/
    int estat;
    int derrn;
    char * date1;
    char * date1fmt;
    char * date2;
    char * date2fmt;
    char  * outfmt;
    char datbuf[DATE_BUF_SZ];

    estat = 0;

    date1       = parmv[0]->var_buf;
    date1fmt    = parmv[1]->var_buf;
    date2       = parmv[2]->var_buf;
    date2fmt    = parmv[3]->var_buf;
    outfmt      = parmv[4]->var_buf;

    derrn = subdates(date1, date1fmt, date2, date2fmt, outfmt, datbuf, sizeof(datbuf));
    if (derrn) {
        get_pd_errmsg(derrn, datbuf, sizeof(datbuf));
        estat = set_error(ce, ESUBDATES, datbuf);
    } else {
        str_to_var(result, datbuf, Istrlen(datbuf));
    }

    return (estat);
}
/***************************************************************/
int bif_subfromdate(struct cenv * ce,
            int parmc,
            struct var ** parmv,
            struct var * result) {

/*
    See also: bif_addtodate()
    <date> = subfromdate(<fromdate>, <fromfmt>, <interval>, <intervalfmt>, <destfmt>)
*/
    int estat;
    int derrn;
    char * fromdate;
    char * fromfmt;
    char * interval;
    char * intervalfmt;
    char * destfmt;
    char datbuf[DATE_BUF_SZ];

    estat = 0;

    fromdate    = parmv[0]->var_buf;
    fromfmt     = parmv[1]->var_buf;
    interval    = parmv[2]->var_buf;
    intervalfmt = parmv[3]->var_buf;
    destfmt     = parmv[4]->var_buf;

    derrn = dateaddsub(fromdate, fromfmt, interval, intervalfmt, 1,
                            destfmt, datbuf, sizeof(datbuf));
    if (derrn) {
        get_pd_errmsg(derrn, datbuf, sizeof(datbuf));
        estat = set_error(ce, EADDTODATE, datbuf);
    } else {
        str_to_var(result, datbuf, Istrlen(datbuf));
    }

    return (estat);
}
/***************************************************************/
int bif_trim(struct cenv * ce,
            int parmc,
            struct var ** parmv,
            struct var * result) {

/*
**  Syntax: TRIM(str)
*/
    int estat;
    int bix;
    int eix;

    estat = 0;
    bix  = 0;
    eix = parmv[0]->var_len - 1;

    while (eix >= 0 && isspace(parmv[0]->var_buf[eix])) eix--;
    while (bix <= eix && isspace(parmv[0]->var_buf[bix])) bix++;

    if (eix < 0) {
        str_to_var(result, "", 0);
    } else {
        str_to_var(result, parmv[0]->var_buf + bix, eix - bix + 1);
    }

    return (estat);
}
/***************************************************************/
int bif_upper(struct cenv * ce,
            int parmc,
            struct var ** parmv,
            struct var * result) {

/*
*/
    int estat;

    estat = 0;

    str_to_var_upper(result, parmv[0]->var_buf, parmv[0]->var_len);

    return (estat);
}
/***************************************************************/
void add_bi_func(struct glob * gg,
                const char * funcnam,
                unsigned int        parm_mask,
                bifunc_handler fhndlr) {

    struct bifuncinfo * bfi;

    bfi = New(struct bifuncinfo, 1);
    strcpy(bfi->bfi_funcnam, funcnam);
    bfi->bfi_parm_mask      = parm_mask;
    bfi->bfi_bifunc_handler = fhndlr; 

    dbtree_insert(gg->gg_bi_funcs, funcnam, Istrlen(funcnam) + 1, bfi);
}
/***************************************************************/
void qcopy_var(struct var * tgtvar, struct var * srcvar) {

    memcpy(tgtvar, srcvar, sizeof(struct var));
    memset(srcvar, 0, sizeof(struct var));
}
/***************************************************************/
int builtin_function_call(struct cenv * ce,
            struct bifuncinfo * bfi,
            const char * cmdbuf,
            int * pix,
            struct str * parmstr,
            struct var * result) {
/*
        if (vhand) {
            bfi = *(struct bifuncinfo **)vhand;
            estat = (bfi->bfi_bifunc_handler)(ce, cmdbuf, pix, parmstr, result);
        }
            estat = parse_exp(ce, cmdbuf, pix, parmstr, result);

            if (!estat) {
                if (parmstr->str_buf[0] == ')' && !parmstr->str_buf[1]) {
                    eol = get_token_exp(ce, cmdbuf, pix, parmstr);
                } else {
                    estat = set_error(ce, EMISSINGCLOSE, NULL);
                }
            }
*/

    int estat;
    int eol;
    int             parmc;
    int             parmmax;
    unsigned int    parm_mask;
    struct var **   parmv;
    int             is_null;

    estat = 0;
    parmc = 0;
    parmmax = 0;
    parmv = NULL;
    is_null = 0;
    if (result->var_typ == VTYP_NULL) is_null = 1;

    parm_mask = bfi->bfi_parm_mask;
    eol = get_token_exp(ce, cmdbuf, pix, parmstr);
    if (eol) {
        if (eol > 0) estat = eol;
        else         estat = set_error(ce, EUNEXPEOL, parmstr->str_buf);
    } else if (parmstr->str_buf[0] == ')') {
        eol = 1;
    }

    while (!estat && !eol) {
        if (is_null) result->var_typ = VTYP_NULL;
        else         result->var_typ = VTYP_NONE;
        estat = parse_exp_entry(ce, cmdbuf, pix, parmstr, result);
        if (!estat) {
            switch (parm_mask & 0x0F) {
                case VTYP_STR :
                    if (result->var_typ != VTYP_STR) {
                        vensure_str(ce, result);
                    }
                    break;
                case VTYP_INT :
                    if (result->var_typ != VTYP_INT) {
                        eol = vensure_num(ce, result);
                        if (eol) {
                            vensure_str(ce, result);
                            estat = set_error(ce, EBADNUM, result->var_buf);
                        }
                    }
                    break;
                case VTYP_BOOL :
                    if (result->var_typ != VTYP_BOOL) {
                        eol = vensure_bool(ce, result);
                        if (eol) {
                            vensure_str(ce, result);
                            estat = set_error(ce, EEXPBOOL, result->var_buf);
                        }
                    }
                    break;
                default :
                    break;
            }
        }

        if (!estat) {
            if (!parm_mask) {
                estat = set_error(ce, EMANYPARMS, bfi->bfi_funcnam);
            } else {
                if (parmc == parmmax) {
                    parmmax += 4; /* chunk */
                    if (!parmc) parmv = New(struct var *, parmmax);
                    else parmv = Realloc(parmv, struct var *, parmmax);
                }
                parmv[parmc] = New(struct var, 1);
                qcopy_var(parmv[parmc], result);
                parm_mask >>= 4;
                parmc++;

                if (parmstr->str_buf[0] == ')') {
                    eol = get_token_exp(ce, cmdbuf, pix, parmstr);
                    if (eol > 0) estat = eol;
                    else eol = ESTAT_EOL;
                } else if (parmstr->str_buf[0] == ',') {
                    eol = get_token_exp(ce, cmdbuf, pix, parmstr);
                    if (eol) {
                        if (eol > 0) estat = eol;
                        else         estat = set_error(ce, EUNEXPEOL, bfi->bfi_funcnam);
                    }
                } else {
                    estat = set_error(ce, EEXPCOMMA, parmstr->str_buf);
                }
            }
        }
    }

    if (!estat) {
        if (parm_mask) {
            estat = set_error(ce, EFEWPARMS, bfi->bfi_funcnam);
        } else if (!is_null) {
            estat = (bfi->bfi_bifunc_handler)(ce, parmc, parmv, result);
        } else {
            result->var_typ = VTYP_NULL;
        }
    }
    free_parms(parmc, parmv);

    return (estat);
}
/***************************************************************/
int user_function_call(struct cenv * ce,
            struct usrfuncinfo * ufi,
            const char * cmdbuf,
            int * pix,
            struct str * parmstr,
            struct var * result) {
/*
*/

    int estat;
    int eol;
    int             parmc;
    int             parmmax;
    struct var * vparmlist;
    int             is_null;

    estat = 0;
    parmc = 0;
    parmmax = 0;
    vparmlist = NULL;
    is_null = 0;
    if (result->var_typ == VTYP_NULL) is_null = 1;

    eol = get_token_exp(ce, cmdbuf, pix, parmstr);
    if (eol) {
        if (eol > 0) estat = eol;
        else         estat = set_error(ce, EUNEXPEOL, parmstr->str_buf);
    } else if (parmstr->str_buf[0] == ')') {
        eol = get_token_exp(ce, cmdbuf, pix, parmstr);
        if (eol > 0) estat = eol;
        else eol = ESTAT_EOL;
    }

    while (!estat && !eol) {
        if (is_null) result->var_typ = VTYP_NULL;
        else         result->var_typ = VTYP_NONE;
        estat = parse_exp_entry(ce, cmdbuf, pix, parmstr, result);
        if (!estat) {
/*            if (result->var_typ != VTYP_STR) {
                 vensure_str(ce, result);
              } */
        }

        if (!estat) {
            if (parmc + 1 >= parmmax) {
                parmmax += PARMLIST_CHUNK;
                if (!parmc) vparmlist = New(struct var, parmmax);
                else vparmlist = Realloc(vparmlist, struct var, parmmax);
            }
            qcopy_var(&(vparmlist[parmc]), result);
            result->var_buf = NULL;
            result->var_len = 0;
            result->var_max = 0;
            parmc++;

            if (parmstr->str_buf[0] == ')') {
                eol = get_token_exp(ce, cmdbuf, pix, parmstr);
                if (eol > 0) estat = eol;
                else eol = ESTAT_EOL;
            } else if (parmstr->str_buf[0] == ',') {
                eol = get_token_exp(ce, cmdbuf, pix, parmstr);
                if (eol) {
                    if (eol > 0) estat = eol;
                    else         estat = set_error(ce, EUNEXPEOL, ufi->ufi_func_name);
                }
            } else {
                estat = set_error(ce, EEXPCOMMA, parmstr->str_buf);
            }
        }
    }

    if (!estat) {
        if (!is_null) {
            estat = call_usrfunc(ce, ufi, NULL, vparmlist, parmc, result);
         } else {
            result->var_typ = VTYP_NULL;
        }
    }
    Free(vparmlist);

    return (estat);
}
/***************************************************************/
int parse_function_call(struct cenv * ce,
            const char * cmdbuf,
            int * pix,
            struct str * parmstr,
            struct var * result) {

/*
*/
    int estat;
    void ** vhand;
    int ii;

    estat = 0;

    if (ce->ce_flags & CEFLAG_CASE) {
        for (ii = 0; ii < parmstr->str_len; ii++)
            parmstr->str_buf[ii] = tolower(parmstr->str_buf[ii]);
    }

    if (ce->ce_funclib && ce->ce_funclib->fl_funcs) {
        vhand = dbtree_find(ce->ce_funclib->fl_funcs, parmstr->str_buf, parmstr->str_len);
    } else {
        vhand = NULL;
    }

    if (vhand) {
        estat = user_function_call(ce, *(struct usrfuncinfo **)vhand,
                    cmdbuf, pix, parmstr, result);
    } else {
        if (!ce->ce_gg->gg_bi_funcs) {
            load_bi_funcs(ce->ce_gg);
        }
        vhand = dbtree_find(ce->ce_gg->gg_bi_funcs, parmstr->str_buf, parmstr->str_len + 1);
        if (vhand) {
            estat = builtin_function_call(ce, *(struct bifuncinfo **)vhand,
                        cmdbuf, pix, parmstr, result);
        } else {
            estat = set_error(ce, EUNDEFFUNC, parmstr->str_buf);
        }
    }

    return (estat);
}
/***************************************************************/
int parse_primary_sub_exp(struct cenv * ce,
            const char * cmdbuf,
            int * pix,
            struct str * parmstr,
            struct var * result) {

/*
*/
    int estat;
    int eol;
    struct str tokestr;

    init_str(&tokestr, 0);

    estat = sub_parm(ce, cmdbuf, pix, &tokestr, SUBPARM_NONE);
    if (!estat) {
        estat = token_to_var(ce, result, &tokestr);
        if (!estat) {
            eol = get_token_exp(ce, cmdbuf, pix, parmstr);
            if (eol > 0) estat = eol;            
        }
    }

    return (estat);
}
/***************************************************************/
int parse_primary_exp(struct cenv * ce,
            const char * cmdbuf,
            int * pix,
            struct str * parmstr,
            struct var * result) {

/*
    estat = parse_exp(ce, cmdbuf, pix, parmstr, &result);
    estat = token_to_var(ce, result, parmstr);
*/
    int estat;
    int eol;

    if (parmstr->str_buf[0] == '(' && !parmstr->str_buf[1]) {
        eol = get_token_exp(ce, cmdbuf, pix, parmstr);
        if (eol) {
            if (eol > 0) estat = eol;
            else         estat = set_error(ce, EUNEXPEOL, "(");
        } else {
            estat = parse_exp_entry(ce, cmdbuf, pix, parmstr, result);

            if (!estat) {
                if (parmstr->str_buf[0] == ')' && !parmstr->str_buf[1]) {
                    eol = get_token_exp(ce, cmdbuf, pix, parmstr);
                    if (eol > 0) estat = eol;
                } else {
                    estat = set_error(ce, EMISSINGCLOSE, NULL);
                }
            }
        }
   } else if (parmstr->str_buf[0] == '$' && !parmstr->str_buf[1]) {
         estat = parse_primary_sub_exp(ce, cmdbuf, pix, parmstr, result);
   } else {
        while(isspace(cmdbuf[*pix])) (*pix)++;
        if (cmdbuf[*pix] == '(' &&
            IS_FIRST_VARCHAR(parmstr->str_buf[0])) {
            (*pix) += 1;
             estat = parse_function_call(ce, cmdbuf, pix, parmstr, result);
       } else {
            estat = token_to_var(ce, result, parmstr);
            if (!estat) {
                eol = get_token_exp(ce, cmdbuf, pix, parmstr);
                if (eol > 0) estat = eol;
            }
        }
    }

    return (estat);
}
/***************************************************************/
int parse_unary_exp(struct cenv * ce,
            const char * cmdbuf,
            int * pix,
            struct str * parmstr,
            struct var * result) {

/*
    estat = parse_exp(ce, cmdbuf, pix, parmstr, &result);
    estat = token_to_var(ce, result, parmstr);
*/
    int estat;
    int eol;
    char op[8];

    if (((parmstr->str_buf[0] == '-'  ||
          parmstr->str_buf[0] == '+') &&
         !parmstr->str_buf[1]         ) ||
         !CESTRCMP(ce, parmstr->str_buf, "not")) {
 
        strcpy(op, parmstr->str_buf);
        eol = get_token_exp(ce, cmdbuf, pix, parmstr);
        if (eol) {
            if (eol > 0) estat = eol;
            else         estat = set_error(ce, EUNEXPEOL, op);
        } else {
            estat = parse_unary_exp(ce, cmdbuf, pix, parmstr, result);
            if (!estat) {
                if (op[0] == '-' || op[0] == '+') {
                    eol = vensure_num(ce, result);
                    if (eol) {
                        vensure_str(ce, result);
                        estat = set_error(ce, EBADNUM, result->var_buf);
                    } else if (op[0] == '-') {
                        if (result->var_typ == VTYP_INT) { /* not null */
                            result->var_val = - result->var_val;
                        }
                    }
                } else {
                    eol = vensure_bool(ce, result);
                    if (eol) {
                        vensure_str(ce, result);
                        estat = set_error(ce, EEXPBOOL, result->var_buf);
                    } else {
                        if (result->var_typ == VTYP_BOOL) { /* not null */
                            if (result->var_val == 0) result->var_val = 1;
                            else                      result->var_val = 0;
                        }
                    }
                }
            }
        }
    } else {
        estat = parse_primary_exp(ce, cmdbuf, pix, parmstr, result);
    }

    return (estat);
}
/***************************************************************/
int parse_term_exp(struct cenv * ce,
            const char * cmdbuf,
            int * pix,
            struct str * parmstr,
            struct var * result) {

/*
    estat = parse_exp(ce, cmdbuf, pix, parmstr, &result);
    estat = token_to_var(ce, result, parmstr);
*/
    int estat;
    int eol;
    struct var result2;
    char op;

    estat = parse_unary_exp(ce, cmdbuf, pix, parmstr, result);

    while (!estat && (parmstr->str_buf[0] == '*'  ||
                      parmstr->str_buf[0] == '/'  ||
                      parmstr->str_buf[0] == '%') &&
                     !parmstr->str_buf[1]         ) {
        op = parmstr->str_buf[0];

        eol = get_token_exp(ce, cmdbuf, pix, parmstr);
        if (eol) {
            if (eol > 0) estat = eol;
            else         estat = set_ferror(ce, EUNEXPEOL, "%c", op);
        } else {
            result2.var_typ = VTYP_NONE;
            estat = parse_unary_exp(ce, cmdbuf, pix, parmstr, &result2);
            if (!estat) {
                eol = vensure_num(ce, result);
                if (eol) {
                    vensure_str(ce, result);
                    estat = set_error(ce, EBADNUM, result->var_buf);
                } else {
                    eol = vensure_num(ce, &result2);
                    if (eol) {
                        vensure_str(ce, &result2);
                        estat = set_error(ce, EBADNUM, result2.var_buf);
                    } else {
                        if (result->var_typ == VTYP_INT) { /* not null */
                                 if (op == '*') result->var_val = result->var_val * result2.var_val;
                            else if (op == '/') {
                                if (result2.var_val) result->var_val = result->var_val / result2.var_val;
                                else estat = set_error(ce, EDIVIDEBYZERO, NULL);
                            } else {
                                if (result2.var_val) result->var_val = result->var_val % result2.var_val;
                                else estat = set_error(ce, EDIVIDEBYZERO, NULL);
                            }
                        }
                    }
                }
                free_var_buf(&result2);
            }
        }
    }

    return (estat);
}
/***************************************************************/
void append_var_len(struct var * vv, const char * buf, int blen) {

    if (vv->var_len + blen >= vv->var_max) {
        vv->var_max += blen + VAR_CHUNK_SIZE;
        vv->var_buf = Realloc(vv->var_buf, char, vv->var_max);
    }

    memcpy(vv->var_buf + vv->var_len, buf, blen);
    vv->var_len += blen;
}
/***************************************************************/
int parse_sum_exp(struct cenv * ce,
            const char * cmdbuf,
            int * pix,
            struct str * parmstr,
            struct var * result) {

/*
    estat = parse_exp(ce, cmdbuf, pix, parmstr, &result);
    estat = token_to_var(ce, result, parmstr);
*/
    int estat;
    int eol;
    struct var result2;
    char op;

    estat = parse_term_exp(ce, cmdbuf, pix, parmstr, result);

    while (!estat && (parmstr->str_buf[0] == '+'  ||
                      parmstr->str_buf[0] == '-'  ||
                      parmstr->str_buf[0] == '&') &&
                     !parmstr->str_buf[1]         ) {
        op = parmstr->str_buf[0];

        eol = get_token_exp(ce, cmdbuf, pix, parmstr);
        if (eol) {
            if (eol > 0) estat = eol;
            else         estat = set_ferror(ce, EUNEXPEOL, "%c", op);
        } else {
            result2.var_typ = VTYP_NONE;
            estat = parse_term_exp(ce, cmdbuf, pix, parmstr, &result2);
            if (!estat) {
                if (op == '&') {
                    vensure_str(ce, result);
                    vensure_str(ce, &result2);
                    if (result->var_typ == VTYP_STR && result2.var_typ == VTYP_STR) { /* not null */
                        append_var_len(result, result2.var_buf, result2.var_len + 1);
                        result->var_len -= 1; /* remove null at end */
                    } else {
                        result2.var_typ = VTYP_NULL;
                    }
                } else {
                    eol = vensure_num(ce, result);
                    if (eol) {
                        vensure_str(ce, result);
                        estat = set_error(ce, EBADNUM, result->var_buf);
                    } else {
                        eol = vensure_num(ce, &result2);
                        if (eol) {
                            vensure_str(ce, &result2);
                            estat = set_error(ce, EBADNUM, result2.var_buf);
                        } else {
                            if (result->var_typ == VTYP_INT && result2.var_typ == VTYP_INT) { /* not null */
                                if (op == '+') result->var_val = result->var_val + result2.var_val;
                                else           result->var_val = result->var_val - result2.var_val;
                            } else {
                                result2.var_typ = VTYP_NULL;
                            }
                        }
                    }
                }
                free_var_buf(&result2);
            }
        }
    }

    return (estat);
}
/***************************************************************/
void reduce_var(struct var * vv) {

    if (vv->var_typ == VTYP_STR) {
        if (!chartoint64(vv->var_buf, vv->var_len, &(vv->var_val))) {
            free_var_buf(vv);
            vv->var_typ = VTYP_INT;
        } else if (!chartobool(vv->var_buf, vv->var_len, &(vv->var_val))) {
            free_var_buf(vv);
            vv->var_typ = VTYP_BOOL;
        }
    }
#if IS_DEBUG
    if (vv->var_typ < 1 || vv->var_typ > 4) {
        printf("Bad var type: %d\n", vv->var_typ);
    }
#endif
}
/***************************************************************/
void do_relop(struct cenv * ce, int relop, struct var * result,  struct var * result2) {

    VITYP cmp;

    if (result->var_typ == VTYP_NULL || result2->var_typ == VTYP_NULL) { /*  null */
        result->var_typ = VTYP_NULL;
        return;
    }

    reduce_var(result);
    reduce_var(result2);

    if (result->var_typ != result2->var_typ) {
        if (result->var_typ == VTYP_STR || result2->var_typ == VTYP_STR) {
            if (result->var_typ  != VTYP_STR) vensure_str(ce, result);
            if (result2->var_typ != VTYP_STR) vensure_str(ce, result2);
        } else {
            if (result->var_typ  != VTYP_INT) vensure_num(ce, result);
            if (result2->var_typ != VTYP_INT) vensure_num(ce, result2);
        }
    }

    switch (result->var_typ) {
        case VTYP_STR : cmp = strcmp(result->var_buf, result2->var_buf);
                        free_var_buf(result);
                        break;
        case VTYP_INT : cmp = (result->var_val - result2->var_val)     ; break;
        case VTYP_BOOL: cmp = (result->var_val - result2->var_val)     ; break;
        default       : cmp = 0;                                       ; break;
    }

    result->var_val = 0;
    switch (relop) {
        case RELOP_LT : if (cmp <  0) result->var_val = 1; break;
        case RELOP_LE : if (cmp <= 0) result->var_val = 1; break;
        case RELOP_EQ : if (cmp == 0) result->var_val = 1; break;
        case RELOP_GE : if (cmp >= 0) result->var_val = 1; break;
        case RELOP_GT : if (cmp >  0) result->var_val = 1; break;
        case RELOP_NE : if (cmp != 0) result->var_val = 1; break;
        default       : ;                                ; break;
    }
    result->var_typ = VTYP_BOOL;
}
/***************************************************************/
int parse_relop_exp(struct cenv * ce,
            const char * cmdbuf,
            int * pix,
            struct str * parmstr,
            struct var * result) {

/*
    estat = parse_exp(ce, cmdbuf, pix, parmstr, &result);
    estat = token_to_var(ce, result, parmstr);
*/
    int estat;
    int eol;
    struct var result2;
    int relop;

    estat = parse_sum_exp(ce, cmdbuf, pix, parmstr, result);
    if (!estat) relop = get_relop(parmstr->str_buf);

    while (!estat && relop) {
        eol = get_token_exp(ce, cmdbuf, pix, parmstr);
        if (eol) {
            if (eol > 0) estat = eol;
            else         estat = set_error(ce, EUNEXPEOL, "and");
        } else {
            result2.var_typ = VTYP_NONE;
            estat = parse_sum_exp(ce, cmdbuf, pix, parmstr, &result2);
            if (!estat) {
                do_relop(ce, relop, result, &result2);
                free_var_buf(&result2);
                relop = get_relop(parmstr->str_buf);
            }
        }
    }

    return (estat);
}
/***************************************************************/
void check_null_result(struct var * result,
            int chkans,
            struct var * result2) {

    if (result->var_typ == VTYP_BOOL) {
        if (chkans) {
            if (result->var_val) result2->var_typ = VTYP_NULL;
            else                 result2->var_typ = VTYP_NONE;
        } else {
            if (result->var_val) result2->var_typ = VTYP_NONE;
            else                 result2->var_typ = VTYP_NULL;
        }
    }
}
/***************************************************************/
int parse_and_exp(struct cenv * ce,
            const char * cmdbuf,
            int * pix,
            struct str * parmstr,
            struct var * result) {

/*
    estat = parse_exp(ce, cmdbuf, pix, parmstr, &result);
    estat = token_to_var(ce, result, parmstr);

    and truth table:
                               result
                        false   true    null
                false : false   false   null
    result2     true  : false   true    null
                null  : false   false   null
*/
    int estat;
    int eol;
    struct var result2;

    estat = parse_relop_exp(ce, cmdbuf, pix, parmstr, result);
    while (!estat && !CESTRCMP(ce, parmstr->str_buf, "and")) {
        eol = vensure_bool(ce, result);
        if (eol) {
            vensure_str(ce, result);
            estat = set_error(ce, EEXPBOOL, result->var_buf);
        } else {
            eol = get_token_exp(ce, cmdbuf, pix, parmstr);
            if (eol) {
                if (eol > 0) estat = eol;
                else         estat = set_error(ce, EUNEXPEOL, "and");
            } else {
                check_null_result(result, 0, &result2);
                estat = parse_relop_exp(ce, cmdbuf, pix, parmstr, &result2);
                if (!estat) {
                    eol = vensure_bool(ce, &result2);
                    if (eol) {
                        vensure_str(ce, result);
                        estat = set_error(ce, EEXPBOOL, result->var_buf);
                    } else {
                        if (result->var_typ == VTYP_BOOL) {
                            if (result->var_val) {
                                if (result2.var_typ == VTYP_BOOL && result2.var_val) {
                                    result->var_val = 1;
                                } else {
                                    result->var_val = 0;
                                }
                            }
                        }
                    }
                    free_var_buf(&result2);
                }
            }
        }
    }

    return (estat);
}
/***************************************************************/
int parse_or_exp(struct cenv * ce,
            const char * cmdbuf,
            int * pix,
            struct str * parmstr,
            struct var * result) {

/*
    estat = parse_exp(ce, cmdbuf, pix, parmstr, &result);
    estat = token_to_var(ce, result, parmstr);

    or truth table:
                               result
                        false   true    null
                false : false   true    null
    result2     true  : true    true    null
                null  : false   true    null
*/
    int estat;
    int eol;
    struct var result2;

    estat = parse_and_exp(ce, cmdbuf, pix, parmstr, result);
    while (!estat && !CESTRCMP(ce, parmstr->str_buf, "or")) {
        eol = vensure_bool(ce, result);
        if (eol) {
            vensure_str(ce, result);
            estat = set_error(ce, EEXPBOOL, result->var_buf);
        } else {
            eol = get_token_exp(ce, cmdbuf, pix, parmstr);
            if (eol) {
                if (eol > 0) estat = eol;
                else         estat = set_error(ce, EUNEXPEOL, "or");
            } else {
                check_null_result(result, 1, &result2);
                estat = parse_and_exp(ce, cmdbuf, pix, parmstr, &result2);
                if (!estat) {
                    eol = vensure_bool(ce, &result2);
                    if (eol) {
                        vensure_str(ce, result);
                        estat = set_error(ce, EEXPBOOL, result->var_buf);
                    } else {
                        if (result->var_typ == VTYP_BOOL) {
                            if (!result->var_val) {
                                if (result2.var_typ == VTYP_BOOL && result2.var_val) {
                                    result->var_val = 1;
                                }
                            }
                        }
                    }
                    free_var_buf(&result2);
                }
            }
        }
    }

    return (estat);
}
/***************************************************************/
int parse_exp_entry(struct cenv * ce,
            const char * cmdbuf,
            int * pix,
            struct str * parmstr,
            struct var * result) {

/*
    estat = parse_exp(ce, cmdbuf, pix, parmstr, &result);
*/
    int estat;

    estat = parse_or_exp(ce, cmdbuf, pix, parmstr, result);

    return (estat);
}
/***************************************************************/
int parse_exp(struct cenv * ce,
            const char * cmdbuf,
            int * pix,
            struct str * parmstr,
            struct var * result) {

/*
    estat = parse_exp(ce, cmdbuf, pix, parmstr, &result);
*/
    int estat;

    result->var_typ = VTYP_NONE;
    estat = parse_exp_entry(ce, cmdbuf, pix, parmstr, result);
    if (!estat) {
        if (result->var_typ == VTYP_NONE) {
            /* happens if user function does return with no value */
            estat = set_error(ce, ENORETVAL, NULL);
        } else if (result->var_typ == VTYP_NULL) {
           /* should not happen */
            estat = set_error(ce, EINVALTYP, NULL);
        }
    }

    return (estat);
}
/***************************************************************/
void set_var_result(struct cenv * ce,
                    struct str * vnam,
                    struct var * result) {

    set_ce_var(ce, vnam->str_buf, vnam->str_len, result);

}
/***************************************************************/
int set_var_exp(struct cenv * ce,
            const char * cmdbuf,
            int * pix,
            struct str * parmstr) {

    int estat;
    int eol;
    struct str vnam;
    struct var result;

    estat = 0;

    eol = get_token_exp(ce, cmdbuf, pix, parmstr);
    if (eol) {
        if (eol > 0) estat = eol;
        else         estat = set_error(ce, EUNEXPEOL, "set");
    } else {
        while(isspace(cmdbuf[*pix])) (*pix)++;

        if (!cmdbuf[*pix]) {
            estat = set_error(ce, EUNEXPEOL, "set variable name");
            free_str_buf(parmstr);

        } else if (cmdbuf[*pix] == '=') {
            if (!is_good_vname(ce, parmstr->str_buf)) {
                estat = set_error(ce, EBADVARNAM, NULL);
            } else {
                copy_vnam(ce, &(vnam.str_buf),  &(vnam.str_len),  &(vnam.str_max), parmstr);

                (*pix) += 1;
                eol = get_token_exp(ce, cmdbuf, pix, parmstr);
                if (eol) {
                    if (eol > 0) estat = eol;
                    else         estat = set_error(ce, EUNEXPEXPEOL, NULL);
                    free_str_buf(parmstr);
                } else {
                    estat = parse_exp(ce, cmdbuf, pix, parmstr, &result);

                    if (!estat) {
                        if (parmstr->str_len) {
                            estat = set_error(ce, EEXPEOL, parmstr->str_buf);
                        } else {
                            set_var_result(ce, &vnam, &result);
                        }
                    }
                    free_var_buf(&result);
                }
                free_str_buf(&vnam);
            }
        } else {
            estat = set_ferror(ce, EEXPEQ, "%c", cmdbuf[*pix]);
        }
    }

    return (estat);
}
/***************************************************************/
int checkcsvparms(struct cenv * ce, const char * csvparms) {

    int estat;
    int ii;
    int ndelims;
    /* char csvdelims[FPR_CSVPARM_SIZE]; */

    estat = 0;
    ndelims = 0;

    /* memset(csvdelims, '\0', FPR_CSVPARM_SIZE); */

    for (ii = 0; !estat && csvparms[ii]; ii++) {
        if ((csvparms[ii] & 0x7F) != csvparms[ii]   ||
                isalnum(csvparms[ii])               ||
                csvparms[ii] == '\"'                ||
                csvparms[ii] == '\'') {
            estat = set_ferror(ce, EBADCSVDELIM, "%c", csvparms[ii]);
        } else {
            ndelims++;
        }
    }

    if (!estat && !ndelims) {
        estat = set_error(ce, ENOCSVDELIM, NULL);
    }

    return (estat);
}
/***************************************************************/
int parse_for_csvparse(struct cenv * ce,
            struct nest * ne,
            struct str * cmdstr,
                int * cmdix,
            struct str * parmstr) {

    int estat;
    int eol;
    struct var srcvar;
    struct var csvparms;
    struct forparserec * fpr;

    eol = get_token_exp(ce, cmdstr->str_buf, cmdix, parmstr);
    if (eol) {
        if (eol > 0) estat = eol;
        else         estat = set_error(ce, EUNEXPEOL, "for");
    } else if (parmstr->str_buf[0] == '(') {
        eol = get_token_exp(ce, cmdstr->str_buf, cmdix, parmstr);
        if (eol) {
            if (eol > 0) estat = eol;
            else         estat = set_error(ce, EUNEXPEOL, "for");
        } else {
            estat = parse_exp(ce, cmdstr->str_buf, cmdix, parmstr, &srcvar);
        }
    } else {
        estat = set_error(ce, EEXPPAREN, parmstr->str_buf);
    }

    if (!estat) {
        vensure_str(ce, &srcvar);
        if (parmstr->str_buf[0] == ')') {
            eol = get_token_exp(ce, cmdstr->str_buf, cmdix, parmstr);
            if (!eol) {
                estat = set_error(ce, EEXPEOL, parmstr->str_buf);
            } else if (eol > 0) {
                estat = eol;
            } else {
                fpr = new_forparserec(srcvar.var_buf, ",");
                ne->ne_data = fpr;
                ne->ne_for_typ = NEFORTYP_CSVPARSE;
            }
        } else if (parmstr->str_buf[0] == ',') {
            eol = get_token_exp(ce, cmdstr->str_buf, cmdix, parmstr);
            if (eol) {
                if (eol > 0) estat = eol;
                else         estat = set_error(ce, EUNEXPEOL, "for");
            } else {
                estat = parse_exp(ce, cmdstr->str_buf, cmdix, parmstr, &csvparms);
                if (!estat) {
                    vensure_str(ce, &csvparms);
                    if (parmstr->str_buf[0] == ')') {
                        eol = get_token_exp(ce, cmdstr->str_buf, cmdix, parmstr);
                        if (!eol) {
                            estat = set_error(ce, EEXPEOL, parmstr->str_buf);
                        } else if (eol > 0) {
                            estat = eol;
                        } else {
                            estat = checkcsvparms(ce, csvparms.var_buf);
                            if (!estat) {
                                fpr = new_forparserec(srcvar.var_buf, csvparms.var_buf);
                                ne->ne_data = fpr;
                                ne->ne_for_typ = NEFORTYP_CSVPARSE;
                            }
                        }
                        free_var_buf(&csvparms);
                    } else {
                        estat = set_error(ce, EEXPCLPAREN, parmstr->str_buf);
                    }
                }
            }
        } else {
            estat = set_error(ce, EEXPCLPAREN, parmstr->str_buf);
        }
        free_var_buf(&srcvar);
    }

    return (estat);
}
/***************************************************************/
int parse_for_in(struct cenv * ce,
                 struct nest * ne,
                 struct str * cmdstr,
                 int * cmdix,
                 struct parmlist * pl) {

    int estat;

    ne->ne_for_typ = NEFORTYP_IN;
    estat = parse_parmlist(ce, cmdstr, cmdix, pl);
    if (!estat) {
        if (pl->pl_piped_parmlist || pl->pl_redir_flags) {
            estat = set_error(ce, ENOREDIR, NULL);
            free_parmlist_buf(ce, pl);
        } else {
            ne->ne_data = pl->pl_cparmlist;
            Free(pl->pl_cparmlens);
            Free(pl->pl_cparmmaxs);
        }
    }

    return (estat);
}
/***************************************************************/
int parse_for_range(struct cenv * ce,
            struct nest * ne,
            struct str * cmdstr,
                int * cmdix,
            struct str * parmstr) {

    int estat;
    int eol;
    struct var result;

    eol = get_token_exp(ce, cmdstr->str_buf, cmdix, parmstr);
    if (eol) {
        if (eol > 0) estat = eol;
        else         estat = set_error(ce, EUNEXPEOL, "for");
    } else {
        estat = parse_exp(ce, cmdstr->str_buf, cmdix, parmstr, &result);
    }

    if (!estat) {
        eol = vensure_num(ce, &result);
        if (eol) {
            vensure_str(ce, &result);
            estat = set_error(ce, EBADNUM, result.var_buf);
        }
        if (!CESTRCMP(ce, parmstr->str_buf, "to")) {
            ne->ne_for_typ = NEFORTYP_TO;
            ne->ne_from_val = result.var_val;
        } else if (!CESTRCMP(ce, parmstr->str_buf, "downto")) {
            ne->ne_for_typ = NEFORTYP_DOWNTO;
           ne->ne_from_val = result.var_val;
        } else {
            estat = set_error(ce, EEXPTO, parmstr->str_buf);
        }
    }

    if (!estat) {
        eol = get_token_exp(ce, cmdstr->str_buf, cmdix, parmstr);
        if (eol) {
            if (eol > 0) estat = eol;
            else         estat = set_error(ce, EUNEXPEOL, "for");
        } else {
            estat = parse_exp(ce, cmdstr->str_buf, cmdix, parmstr, &result);
        }
    }

    if (!estat) {
        if (parmstr->str_len) {
            estat = set_error(ce, EEXPEOL, parmstr->str_buf);
        } else {
            eol = vensure_num(ce, &result);
            if (eol) {
                vensure_str(ce, &result);
                estat = set_error(ce, EBADNUM, result.var_buf);
            } else {
               ne->ne_to_val = result.var_val;
            }
        }
    }

    return (estat);
}
/***************************************************************/
int parse_for_parse(struct cenv * ce,
            struct nest * ne,
            struct str * cmdstr,
                int * cmdix,
            struct str * parmstr) {

    int estat;
    int eol;
    struct var result;
    struct forparserec * fpr;

    eol = get_token_exp(ce, cmdstr->str_buf, cmdix, parmstr);
    if (eol) {
        if (eol > 0) estat = eol;
        else         estat = set_error(ce, EUNEXPEOL, "for");
    } else if (parmstr->str_buf[0] == '(') {
        eol = get_token_exp(ce, cmdstr->str_buf, cmdix, parmstr);
        if (eol) {
            if (eol > 0) estat = eol;
            else         estat = set_error(ce, EUNEXPEOL, "for");
        } else {
            estat = parse_exp(ce, cmdstr->str_buf, cmdix, parmstr, &result);
        }
    } else {
        estat = set_error(ce, EEXPPAREN, parmstr->str_buf);
    }

    if (!estat) {
        vensure_str(ce, &result);
        if (parmstr->str_buf[0] == ')') {
            eol = get_token_exp(ce, cmdstr->str_buf, cmdix, parmstr);
            if (!eol) {
                estat = set_error(ce, EEXPEOL, parmstr->str_buf);
            } else if (eol > 0) {
                estat = eol;
            } else {
                fpr = new_forparserec(result.var_buf, NULL);
                ne->ne_data = fpr;
                ne->ne_for_typ = NEFORTYP_PARSE;
            }
        } else {
            estat = set_error(ce, EEXPCLPAREN, parmstr->str_buf);
        }
        free_var_buf(&result);
    }

    return (estat);
}
/***************************************************************/
int parse_for_read(struct cenv * ce,
            struct nest * ne,
            struct str * cmdstr,
                int * cmdix,
            struct str * parmstr) {

    int estat;
    int eol;
    struct var result;
    int forfil;
    struct forreadrec * frr;

    eol = get_token_exp(ce, cmdstr->str_buf, cmdix, parmstr);
    if (eol) {
        if (eol > 0) estat = eol;
        else         estat = set_error(ce, EUNEXPEOL, "for");
    } else if (parmstr->str_buf[0] == '(') {
        eol = get_token_exp(ce, cmdstr->str_buf, cmdix, parmstr);
        if (eol) {
            if (eol > 0) estat = eol;
            else         estat = set_error(ce, EUNEXPEOL, "for");
        } else {
            estat = parse_exp(ce, cmdstr->str_buf, cmdix, parmstr, &result);
        }
    } else {
        estat = set_error(ce, EEXPPAREN, parmstr->str_buf);
    }

    if (!estat) {
        vensure_str(ce, &result);
        if (parmstr->str_buf[0] == ')') {
            eol = get_token_exp(ce, cmdstr->str_buf, cmdix, parmstr);
            if (!eol) {
                estat = set_error(ce, EEXPEOL, parmstr->str_buf);
            } else if (eol > 0) {
                estat = eol;
            } else {
                forfil = OPEN(result.var_buf, _O_RDONLY | _O_BINARY);
                if (forfil < 0) {
                    int ern = ERRNO;
                    estat = set_ferror(ce, EOPENFORFILE,
                                "%s -- %m", result.var_buf, ern);
                } else {
                    frr = new_forreadrec(forfil, result.var_buf);
                    ne->ne_data = frr;
                    ne->ne_for_typ = NEFORTYP_READ;
                }
            }
        } else {
            estat = set_error(ce, EEXPCLPAREN, parmstr->str_buf);
        }
        free_var_buf(&result);
    }

    return (estat);
}
/***************************************************************/
int parse_for_readdir(struct cenv * ce,
            struct nest * ne,
            struct str * cmdstr,
                int * cmdix,
            struct str * parmstr) {

    int estat;
    int eol;
    struct var result;
    struct forreaddirrec * frdr;
    void * vrdr;
    int derr;

    eol = get_token_exp(ce, cmdstr->str_buf, cmdix, parmstr);
    if (eol) {
        if (eol > 0) estat = eol;
        else         estat = set_error(ce, EUNEXPEOL, "for");
    } else if (parmstr->str_buf[0] == '(') {
        eol = get_token_exp(ce, cmdstr->str_buf, cmdix, parmstr);
        if (eol) {
            if (eol > 0) estat = eol;
            else         estat = set_error(ce, EUNEXPEOL, "for");
        } else {
            estat = parse_exp(ce, cmdstr->str_buf, cmdix, parmstr, &result);
        }
    } else {
        estat = set_error(ce, EEXPPAREN, parmstr->str_buf);
    }

    if (!estat) {
        vensure_str(ce, &result);
        if (parmstr->str_buf[0] == ')') {
            eol = get_token_exp(ce, cmdstr->str_buf, cmdix, parmstr);
            if (!eol) {
                estat = set_error(ce, EEXPEOL, parmstr->str_buf);
            } else if (eol > 0) {
                estat = eol;
            } else {
                vrdr = new_read_directory();
                derr = open_directory(vrdr, result.var_buf);
                if (derr) {
                    estat = set_ferror(ce, EOPENFORDIR,
                                "%s -- %s",
                                result.var_buf, get_errmsg_directory(vrdr));
                } else {
                    frdr = new_forreaddirrec(vrdr, result.var_buf);
                    ne->ne_data = frdr;
                    ne->ne_for_typ = NEFORTYP_READDIR;
                }
            }
        } else {
            estat = set_error(ce, EEXPCLPAREN, parmstr->str_buf);
        }
        free_var_buf(&result);
    }

    return (estat);
}
/***************************************************************/
void append_str_var(struct cenv * ce, struct str * ss, struct var * vv) {

    char tbuf[24];

    switch (vv->var_typ) {
        case VTYP_STR:
            append_str_len(ss, vv->var_buf, vv->var_len + 1);
            ss->str_len -= 1; /* remove null at end */
            break;

        case VTYP_INT:
            /*
            **  Range is: -9223372036854775808 to 9223372036854775807
            */
            Snprintf(tbuf, sizeof(tbuf), "%Ld", vv->var_val);
            append_str(ss, tbuf);
            break;

        case VTYP_BOOL:
            if (ce->ce_flags & CEFLAG_CASE) {
                if (vv->var_val) {
                    strcpy(tbuf, "TRUE");
                } else {
                    strcpy(tbuf, "FALSE");
                }
            } else {
                if (vv->var_val) {
                    strcpy(tbuf, "true");
                } else {
                    strcpy(tbuf, "false");
                }
            }
            append_str(ss, tbuf);
            break;

        default:
            /* do nothing */
            break;
    }
}
/***************************************************************/
void load_bi_funcs(struct glob * gg) {
/*
**  builtin_function_call()
** parms:
**  Hex - Right to left
**    0 - none
**    1 - String  - VTYP_STR
**    2 - Number  - VTYP_INT
**    4 - Boolean - VTYP_BOOL
*/
    gg->gg_bi_funcs             = dbtree_new();
                           /* parms */
    add_bi_func(gg, "addtodate"     , 0x11111, bif_addtodate);
    add_bi_func(gg, "argv"          ,  0x0002, bif_argv);
    add_bi_func(gg, "basename"      ,  0x0001, bif_basename);
    add_bi_func(gg, "char"          ,  0x0002, bif_char);
    add_bi_func(gg, "chbase"        ,  0x0221, bif_chbase);
    add_bi_func(gg, "code"          ,  0x0001, bif_code);
    add_bi_func(gg, "dirname"       ,  0x0001, bif_dirname);
    add_bi_func(gg, "extname"       ,  0x0001, bif_extname);
    add_bi_func(gg, "find"          ,  0x0011, bif_find);
    add_bi_func(gg, "finfo"         ,  0x0011, bif_finfo);
    add_bi_func(gg, "left"          ,  0x0021, bif_left);
    add_bi_func(gg, "len"           ,  0x0001, bif_len);
    add_bi_func(gg, "lower"         ,  0x0001, bif_lower);
    add_bi_func(gg, "matches"       ,  0x0011, bif_matches);
    add_bi_func(gg, "mid"           ,  0x0221, bif_mid);
    add_bi_func(gg, "now"           ,  0x0001, bif_now);
    add_bi_func(gg, "prompt"        ,  0x0001, bif_prompt);
    add_bi_func(gg, "rept"          ,  0x0021, bif_rept);
    add_bi_func(gg, "right"         ,  0x0021, bif_right);
    add_bi_func(gg, "sleep"         ,  0x0002, bif_sleep);
    add_bi_func(gg, "subdates"      , 0x11111, bif_subdates);
    add_bi_func(gg, "subfromdate"   , 0x11111, bif_subfromdate);
    add_bi_func(gg, "trim"          ,  0x0001, bif_trim);
    add_bi_func(gg, "upper"         ,  0x0001, bif_upper);
}
/***************************************************************/
