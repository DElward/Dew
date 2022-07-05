/**************************************************************/
/* util.c                                                     */
/**************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>
#include <errno.h>

#include "types.h"
#include "util.h"

#define WCHAR_LEN_TO_CHAR_LEN(n) ((n) * sizeof(tWCHAR))
#define WStrlen(s)      (xmlclen(s))

/***************************************************************/
int xmlclen(const tWCHAR * xtok) {

    int len;

    if (!xtok) return (0);

    len = 0;
    while (xtok[len]) len++;

    return (len);
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
int32 get_utf16lechar (const unsigned char * text, int textlen, int * ix) {

    int32 utfchar;
    tWCHAR code_unit;
    tWCHAR code_unit_2;

    utfchar = 0;

    if ((*ix) + 1 < textlen) {
        code_unit = text[(*ix) + 1] << 8 | text[(*ix)]; /* little endian */
        (*ix) += 2;
        utfchar = code_unit;

        if (code_unit >= 0xD800 && code_unit <= 0xDBFF && (*ix) < textlen) {
            code_unit_2 = text[(*ix) + 1] << 8 | text[(*ix)]; /*little endian*/
            if (code_unit_2 >= 0xDC00 && code_unit_2 <= 0xDFFF) {
                (*ix) += 2;
                utfchar = (code_unit << 10) + code_unit_2 - 0x35FDC00;
            }
        }
    }

    return (utfchar);
}
/***************************************************************/
int32 get_utf16bechar (const unsigned char * text, int textlen, int * ix) {

    int32 utfchar;
    tWCHAR code_unit;
    tWCHAR code_unit_2;

    utfchar = 0;

    if ((*ix) + 1 < textlen) {
        code_unit = text[(*ix)] << 8 | text[(*ix) + 1]; /* big endian */
        (*ix) += 2;
        utfchar = code_unit;

        if (code_unit >= 0xD800 && code_unit <= 0xDBFF && (*ix) < textlen) {
            code_unit_2 = text[(*ix)] << 8 | text[(*ix) + 1]; /* big endian*/
            if (code_unit_2 >= 0xDC00 && code_unit_2 <= 0xDFFF) {
                (*ix) += 2;
                utfchar = (code_unit << 10) + code_unit_2 - 0x35FDC00;
            }
        }
    }

    return (utfchar);
}
/***************************************************************/
int put_utf16lechar (int32 utfchar, unsigned char * dest) {
/*
** utfchar is in native endian
*/

    int dlen;
    tWCHAR code_unit;

    dlen = 0;
    if (utfchar < 0x10000) {
        code_unit = (tWCHAR)utfchar;
        dest[dlen++] = (unsigned char)(code_unit & 0xFF);
        dest[dlen++] = (unsigned char)((code_unit >> 8) & 0xFF);
    } else if (utfchar <= 0x10FFFF) {
        code_unit = (tWCHAR)((utfchar >> 10) + 0xD7C0);
        dest[dlen++] = (unsigned char)(code_unit & 0xFF);
        dest[dlen++] = (unsigned char)((code_unit >> 8) & 0xFF);
        code_unit = (tWCHAR)((utfchar & 0x3FF) + 0xDC00);
        dest[dlen++] = (unsigned char)(code_unit & 0xFF);
        dest[dlen++] = (unsigned char)((code_unit >> 8) & 0xFF);
    } else {
        /* error("invalid utfchar"); */
    }

    return (dlen);
}
/***************************************************************/
int put_utf16bechar (int32 utfchar, unsigned char * dest) {
/*
** utfchar is in native endian
*/

    int dlen;
    tWCHAR code_unit;

    dlen = 0;
    if (utfchar < 0x10000) {
        code_unit = (tWCHAR)utfchar;
        dest[dlen++] = (unsigned char)((code_unit >> 8) & 0xFF);
        dest[dlen++] = (unsigned char)(code_unit & 0xFF);
    } else if (utfchar <= 0x10FFFF) {
        code_unit = (tWCHAR)((utfchar >> 10) + 0xD7C0);
        dest[dlen++] = (unsigned char)((code_unit >> 8) & 0xFF);
        dest[dlen++] = (unsigned char)(code_unit & 0xFF);
        code_unit = (tWCHAR)((utfchar & 0x3FF) + 0xDC00);
        dest[dlen++] = (unsigned char)((code_unit >> 8) & 0xFF);
        dest[dlen++] = (unsigned char)(code_unit & 0xFF);
    } else {
        /* error("invalid utfchar"); */
    }

    return (dlen);
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
int put_rawchar (int32 utfchar, unsigned char * dest) {

    int dlen;

    dlen = 0;
    dest[dlen++] = (unsigned char)(utfchar & 0xFF);

    return (dlen);
}
/***************************************************************/
void appendbuf_spaces(char ** pbuf,
        int * pbuflen,
        int * pbufmax,
        int nspaces)
{
/*
**  01/08/2020
*/
    if (nspaces > 0) {
        if ((*pbuflen) + nspaces >= (*pbufmax)) {
            (*pbufmax) = (*pbufmax) + nspaces + 63;
            (*pbuf) = Realloc((*pbuf), char, (*pbufmax));
        }
        memset((*pbuf) + (*pbuflen), ' ', nspaces);
        (*pbuflen) += nspaces;
        (*pbuf)[*pbuflen] = '\0';
    }
}
/***************************************************************/
void appendbuf_n(char ** pbuf,
        int * pbuflen,
        int * pbufmax,
        const char * addbuf,
        int addlen)
{
/*
**  04/27/2018: See also char_append() and append_chars() and text_append()
*/
    if ((*pbuflen) + addlen >= (*pbufmax)) {
        (*pbufmax) = (*pbufmax) + addlen + 63;
        (*pbuf) = Realloc((*pbuf), char, (*pbufmax));
    }
    memcpy((*pbuf) + (*pbuflen), addbuf, addlen);
    (*pbuflen) += addlen;
    (*pbuf)[*pbuflen] = '\0';
}
/***************************************************************/
void appendbuf_c(char ** pbuf,
        int * pbuflen,
        int * pbufmax,
        const char * addbuf)
{
/*
**  01/08/2020
*/
    appendbuf_n(pbuf, pbuflen, pbufmax, addbuf, (int)strlen(addbuf));
}
/***************************************************************/
void appendbuf_f(char ** pbuf,
        int * pbuflen,
        int * pbufmax,
        const char * fmt, ...)
{
/*
**  01/16/2020
*/

	va_list args;
    char msgbuf[256];

	va_start(args, fmt);
	vsprintf(msgbuf, fmt, args);
	va_end(args);

    appendbuf_n(pbuf, pbuflen, pbufmax, msgbuf, (int)strlen(msgbuf));
}
/***************************************************************/
/* added 2/4/2016 */
void appendbuf_s(char ** pbuf,
        int * pbuflen,
        int * pbufmax,
        const char * addbuf,
        int flags,      /* flags is unused */
        int s_flags,
        int width)      /* width is unused */
{
    int32 outch;
    int aix;
    int addlen;
    int ii;
    int outbuflen;
    char xbuf[16];
    unsigned char outbuf[16];

    addlen = IStrlen(addbuf);
    aix = 0;

    while (addbuf[aix]) {
        if (s_flags & XSNP_S_FLAG_IN_UTF8) {
            outch = get_utf8char((unsigned char *)addbuf, addlen, &aix);
        } else {
            outch = (unsigned char)addbuf[aix];
            aix++;
        }

        xbuf[0] = '\0';
        outbuflen = 0;
        if (s_flags & XSNP_S_FLAG_XML) {
            /* See also xml_string() */
            switch (outch) {
                case '\"': strcpy(xbuf, "&quot;"); break;
                case '&' : strcpy(xbuf, "&amp;" ); break;
                case '\'': strcpy(xbuf, "&apos;"); break;
                case '<' : strcpy(xbuf, "&lt;"  ); break;
                case '>' : strcpy(xbuf, "&gt;"  ); break;
                default:
                    if (outch < 32 || outch > 126) {
                        sprintf(xbuf, "&#%d;", (int)outch);
                    }
                    break;
            }
        } else if (s_flags & XSNP_S_FLAG_JSON) {
            switch (outch) {
                case '\"': strcpy(xbuf, "\\\"") ; break;
                case '\\': strcpy(xbuf, "\\\\" ); break;
                case '\b': strcpy(xbuf, "\\b")  ; break;
                case '\f': strcpy(xbuf, "\\f")  ; break;
                case '\n': strcpy(xbuf, "\\n")  ; break;
                case '\r': strcpy(xbuf, "\\r")  ; break;
                case '\t': strcpy(xbuf, "\\t")  ; break;
                default:
                    if (outch < 32 || outch > 126) {
                        sprintf(xbuf, "\\u%04X", (int)outch);
                    }
                    break;
            }
        }

        if (xbuf[0]) {
            for (ii = 0; xbuf[ii]; ii++) {
                if (s_flags & XSNP_S_FLAG_OUT_UTF8) {
                    outbuflen +=
                        put_utf8char((int32)xbuf[ii], outbuf + outbuflen);
                } else if (s_flags & XSNP_S_FLAG_OUT_WIDE_BE) {
                    outbuflen +=
                        put_utf16bechar((int32)xbuf[ii], outbuf + outbuflen);
                } else if (s_flags & XSNP_S_FLAG_OUT_WIDE_LE) {
                    outbuflen +=
                        put_utf16lechar((int32)xbuf[ii], outbuf + outbuflen);
                } else {
                    outbuflen +=
                        put_rawchar((int32)xbuf[ii], outbuf + outbuflen);
                }
            }
        } else {
            if (s_flags & XSNP_S_FLAG_OUT_UTF8) {
                outbuflen = put_utf8char(outch, outbuf);
            } else if (s_flags & XSNP_S_FLAG_OUT_WIDE_BE) {
                outbuflen = put_utf16bechar(outch, outbuf);
            } else if (s_flags & XSNP_S_FLAG_OUT_WIDE_LE) {
                outbuflen = put_utf16lechar(outch, outbuf);
            } else {
                outbuflen = put_rawchar(outch, outbuf);
            }
        }
        appendbuf_n(pbuf, pbuflen, pbufmax, (char*)outbuf, outbuflen);
    }
}
/***************************************************************/
/* added 2/4/2016 */
static void appendbuf_wide_s(char ** pbuf,
        int * pbuflen,
        int * pbufmax,
        const tWCHAR * waddbuf,
        int flags,      /* flags is unused */
        int s_flags,
        int width)      /* width is unused */
{
    /**** Needs testing/verification ****/

    int32 outch;
    int aix;
    int addlen;
    int ii;
    int outbuflen;
    char xbuf[16];
    unsigned char outbuf[16];

    addlen = WCHAR_LEN_TO_CHAR_LEN(WStrlen(waddbuf));
    aix = 0;

    while (waddbuf[aix]) {
        if (s_flags & XSNP_S_FLAG_IN_UTF8) {
            outch =  get_utf16lechar((unsigned char *)waddbuf, addlen, &aix);
        } else {
            outch = waddbuf[aix];
            aix++;
        }

        xbuf[0] = '\0';
        outbuflen = 0;
        if (s_flags & XSNP_S_FLAG_XML) {
            /* See also xml_string() */
            switch (outch) {
                case '\"': strcpy(xbuf, "&quot;"); break;
                case '&' : strcpy(xbuf, "&amp;" ); break;
                case '\'': strcpy(xbuf, "&apos;"); break;
                case '<' : strcpy(xbuf, "&lt;"  ); break;
                case '>' : strcpy(xbuf, "&gt;"  ); break;
                default:
                    if (outch < 32 || outch > 126) {
                        sprintf(xbuf, "&#%d;", (int)outch);
                    }
                    break;
            }
        } else if (s_flags & XSNP_S_FLAG_JSON) {
            switch (outch) {
                case '\"': strcpy(xbuf, "\\\"") ; break;
                case '\\': strcpy(xbuf, "\\\\" ); break;
                case '\b': strcpy(xbuf, "\\b")  ; break;
                case '\f': strcpy(xbuf, "\\f")  ; break;
                case '\n': strcpy(xbuf, "\\n")  ; break;
                case '\r': strcpy(xbuf, "\\r")  ; break;
                case '\t': strcpy(xbuf, "\\t")  ; break;
                default:
                    if (outch < 32 || outch > 126) {
                        sprintf(xbuf, "\\u%04X", (int)outch);
                    }
                    break;
            }
        }

        if (xbuf[0]) {
            for (ii = 0; xbuf[ii]; ii++) {
                if (s_flags & XSNP_S_FLAG_OUT_UTF8) {
                    outbuflen +=
                        put_utf8char((int32)xbuf[ii], outbuf + outbuflen);
                } else if (s_flags & XSNP_S_FLAG_OUT_WIDE_BE) {
                    outbuflen +=
                        put_utf16bechar((int32)xbuf[ii], outbuf + outbuflen);
                } else if (s_flags & XSNP_S_FLAG_OUT_WIDE_LE) {
                    outbuflen +=
                        put_utf16lechar((int32)xbuf[ii], outbuf + outbuflen);
                } else {
                    outbuflen +=
                        put_rawchar((int32)xbuf[ii], outbuf + outbuflen);
                }
            }
        } else {
            if (s_flags & XSNP_S_FLAG_OUT_UTF8) {
                outbuflen = put_utf8char(outch, outbuf);
            } else if (s_flags & XSNP_S_FLAG_OUT_WIDE_BE) {
                outbuflen = put_utf16bechar(outch, outbuf);
            } else if (s_flags & XSNP_S_FLAG_OUT_WIDE_LE) {
                outbuflen = put_utf16lechar(outch, outbuf);
            } else {
                outbuflen = put_rawchar(outch, outbuf);
            }
        }
        appendbuf_n(pbuf, pbuflen, pbufmax, (char*)outbuf, outbuflen);
    }
}
/***************************************************************/
/* added 2/4/2016 */
char * VXSmprintf(const char *fmt, va_list args)
{
/*
** Simplified "printf" returns char* of resulting string
** Return must be free'd
*/

/*
    snprintf();
    printf Format Specification Fields

    %[flags] [width] [modifier] [ ( string-flags ) ] type

    flags:
        -       Result is left justified (default is right justified)
        0       Result is padded with leading zeros

    modifier:
        h       Argument is a short int
        l       Argument is a intr
        ll      Argument is a intr intr **

    type:
        c       char is result
        d       int is converted to decimal
        s       zero-terminated string
        S       zero-terminated wide string
        x       unsigned int is converted to hexadecimal (abcdef)
        X       unsigned int is converted to hexadecimal (ABCDEF)

    string-flag:
        u       input argument is UTF-8
        b       input is UTF-16 big endian
        w       input is UTF-16 little endian
        U       output is UTF-8
        B       output is UTF-16 big endian
        W       output is UTF-16 little endian
        a       output to XML with escapes like &amp; --* was 'x' 02/09/2017
        j       output to JSON with escapes like \u00E9

    Example: %(a)s      <-- Output with XML escapes

** - Not yet implemented

    state:
        0       Not in specification
        1       In specification

*/
    int fix;
    int state;
    char ch;
    int width;
    int flags;
    int s_flags;
    int err;
    char nbuf[128];
    int nix;
    int neg;
    int nlen;
    long   tlong;
    double tdouble;
    char * tstr;
    int64 ti64;
    char * outbuf;
    int outmax;
    int outlen;
    char nullstr[] = "(null)";
    char hex_u[] = "0123456789ABCDEF";
    char hex_l[] = "0123456789abcdef";

    err   = 0;
    state = 0;
    flags = 0;
    s_flags = 0;
    width = 0;
    fix   = 0;
    ch    = fmt[fix];

    outbuf = NULL;
    outmax = 0;
    outlen = 0;

    while (ch) {
        if (!state) {
            if (ch == '%') {
                if (fmt[fix+1] == '%') {
                    fix++;
                    appendbuf_n(&outbuf, &outlen, &outmax, &ch, 1);
                } else {
                    state = 1;
                    flags = 0;
                    s_flags = 0;
                    width = 0;
                }
            } else {
                appendbuf_n(&outbuf, &outlen, &outmax, &ch, 1);
            }
        } else if (state == 1) {
            /***** flags *****/
                /* '-' overrides '0', '+' overrides ' ' */
            if (ch == '-') {
                flags |= XSNPFLAG_MINUS;
            } else if (ch == '0') {
                flags |= XSNPFLAG_ZERO;
            } else if (ch == '(') {
                state = 2;  /* specify s_flags */
            } else if (ch >= '1' && ch <= '9') {
                width = (width * 10) + (ch - '0');
                flags |= XSNPFLAG_WIDTH;

            /***** modifier *****/
            } else if (ch == 'h') {
                flags |= XSNPFLAG_MOD_H;
            } else if (ch == 'l') {
                flags |= XSNPFLAG_MOD_L;
            } else if (ch == 'L') {
                flags |= XSNPFLAG_MOD_LL;

            /***** type s *****/
            } else if (ch == 's') {
                tstr = va_arg(args,char*);
                if (!tstr) tstr = nullstr;
                appendbuf_s(&outbuf, &outlen, &outmax,
                    tstr, flags, s_flags, width);
                state = 0;

            /***** type s *****/
            } else if (ch == 'S') {
                tstr = va_arg(args,char*);
                if (!tstr) tstr = nullstr;;
                appendbuf_wide_s(&outbuf, &outlen, &outmax,
                    (tWCHAR*)tstr, flags, s_flags, width);
                state = 0;

            /***** type c *****/
            } else if (ch == 'c') {
                nbuf[0] = (char)va_arg(args, int);
                appendbuf_n(&outbuf, &outlen, &outmax, nbuf, 1);
                state = 0;

            /***** type d and x *****/
            } else if (ch == 'd' || ch == 'x' || ch == 'X') {
                neg = 0;
                nix = sizeof(nbuf) - 1;
                nbuf[nix--] = '\0';
                if (flags & XSNPFLAG_MOD_LL) {
                    ti64 = va_arg(args, int64);
                    if (!ti64) {
                        nbuf[nix--] = '0';
                    } else if (ch == 'd') {
                        if (ti64 < 0) { ti64 = -ti64; neg = 1; }
                        while (ti64 && nix >= 0) {
                            nbuf[nix--] = (char)((ti64 % 10) + '0');
                            ti64 /= 10;
                        }
                    } else if (ch == 'x') {    /* lower hex */
                        while (ti64 && nix >= 0) {
                            nbuf[nix--] = hex_l[(int)(ti64 & 0x0F)];
                            ti64 >>= 4;
                        }
                    } else {    /* upper hex */
                        while (ti64 && nix >= 0) {
                            nbuf[nix--] = hex_u[(int)(ti64 & 0x0F)];
                            ti64 >>= 4;
                        }
                    }
                } else {
                    if (flags & XSNPFLAG_MOD_H) {
                        tlong = (short int)va_arg(args, int);
                    } else if (flags & XSNPFLAG_MOD_L) {
                        tlong = va_arg(args, long);
                    } else {
                        tlong = va_arg(args, int);
                    }
                    if (!tlong) {
                        nbuf[nix--] = '0';
                    } else if (ch == 'd') {
                        if (tlong < 0) { tlong = -tlong; neg = 1; }
                        while (tlong && nix >= 0) {
                            nbuf[nix--] = (tlong % 10) + '0';
                            tlong /= 10;
                        }
                    } else if (ch == 'x') {    /* lower hex */
                        while (tlong && nix >= 0) {
                            nbuf[nix--] = hex_l[(int)(tlong & 0x0F)];
                            tlong >>= 4;
                        }
                    } else {    /* upper hex */
                        while (tlong && nix >= 0) {
                            nbuf[nix--] = hex_u[(int)(tlong & 0x0F)];
                            tlong >>= 4;
                        }
                    }
                }
                if (neg) nbuf[nix--] = '-';
                nix++; /* point to first character */
                nlen = sizeof(nbuf) - nix - 1;
                if ((flags & XSNPFLAG_WIDTH) && (width > nlen)) {
                    width -= nlen;
                    if (flags & XSNPFLAG_MINUS) {
                        appendbuf_n(&outbuf, &outlen, &outmax, nbuf + nix, nlen);
                        while (width) {
                            appendbuf_n(&outbuf, &outlen, &outmax, " ", 1);
                            width--;
                        }
                    } else {    /* XSNPFLAG_WIDTH && !XSNPFLAG_MINUS */
                        if (flags & XSNPFLAG_ZERO) {
                            if (nbuf[nix] == '-') {
                                appendbuf_n(&outbuf, &outlen, &outmax,
                                    nbuf + nix, 1);
                                nix++;
                            }
                            while (width) {
                                appendbuf_n(&outbuf, &outlen, &outmax, "0", 1);
                                width--;
                            }
                        } else {    /* XSNPFLAG_WIDTH && */
                                    /* !XSNPFLAG_MINUS && !XSNPFLAG_MINUS */
                            while (width) {
                                appendbuf_n(&outbuf, &outlen, &outmax, " ", 1);
                                width--;
                            }
                        }
                        appendbuf_n(&outbuf, &outlen, &outmax, nbuf + nix, nlen);
                    }
                } else {    /* !XSNPFLAG_WIDTH */
                    appendbuf_n(&outbuf, &outlen, &outmax, nbuf + nix, nlen);
                }
                state = 0;

            /***** type o *****/
                /* o not supported */
                state = 0;

            /***** type e *****/
            } else if (ch == 'e' || ch == 'E') {
                /* e not supported */
                tdouble = va_arg(args, double);
                state = 0;

            /***** type f *****/
            } else if (ch == 'f' || ch == 'F') {
                /* f not supported */
                tdouble = va_arg(args, double);
                state = 0;

            /***** type g *****/
            } else if (ch == 'g' || ch == 'G') {
                /* g not supported */
                tdouble = va_arg(args, double);
                state = 0;

            /***** unknown *****/
            } else {
                if (!err) err = 1; /* unrecognized format character */
                state = 0;
            }
        } else if (state == 2) {
            if (ch == ')') {
                state = 1;  /* done with s_flags */

            /***** s_flag X *****/
            } else if (ch == 'a') { /* was 'x' 02/09/2017 */
                /* output to XML with escapes like &amp; */
                s_flags |= XSNP_S_FLAG_XML;

            } else if (ch == 'j') {
                /* output to JSON with escapes like \u00E9 */
                s_flags |= XSNP_S_FLAG_JSON;

            } else if (ch == 'u') {
                /* input is UTF-8 */
                s_flags |= XSNP_S_FLAG_IN_UTF8;

            } else if (ch == 'b') {
                /* input is UTF-16 big endian */
                s_flags |= XSNP_S_FLAG_IN_WIDE_BE;

            } else if (ch == 'w') {
                /* input is UTF-16 little endian */
                s_flags |= XSNP_S_FLAG_IN_WIDE_LE;

            } else if (ch == 'U') {
                /* output is UTF-8 */
                s_flags |= XSNP_S_FLAG_OUT_UTF8;

            } else if (ch == 'B') {
                /* output is UTF-16 big endian */
                s_flags |= XSNP_S_FLAG_OUT_WIDE_BE;

            } else if (ch == 'W') {
                /* output is UTF-16 little endian */
                s_flags |= XSNP_S_FLAG_OUT_WIDE_LE;

            } else {
                if (!err) err = 2; /* bad s_flag */
            }
        }
        fix++;
        ch = fmt[fix];
    }

    /* Added 08/18/2016 */
    if (!outbuf) {
        outbuf = New(char, 1);
        outbuf[0] = '\0';
    }

    return outbuf;
}
/***************************************************************/
/* added 2/4/2016 */
int XFprintf(FILE * outf, const char *fmt, ...)
{
    va_list args;
    char * outbuf;
    int err = 0;
    int rtn;

    va_start(args, fmt);
    outbuf = VXSmprintf(fmt, args);
    va_end(args);

    if (!outf) {
        rtn = fputs(outbuf, stdout);
    } else {
        rtn = fputs(outbuf, outf);
    }
    if (rtn < 0) err = -1;
    Free(outbuf);

    return err;
}
/***************************************************************/
/* added 04/25/2018 */
char * XSmprintf(const char *fmt, ...)
{
    va_list args;
    char * outbuf;

    va_start(args, fmt);
    outbuf = VXSmprintf(fmt, args);
    va_end(args);

    return (outbuf);
}
/***************************************************************/
void hlog(FILE * outf,
             int logflags,
             const void * vldata,
             int ldatalen,
             const char * fmt, ...)
{
    va_list args;
    char * msg;
    int remain;
    int linemax;
    int linewidth;
    int outlen;
    int asctab;
    int towrite;
    int ldataix;
    int ii;
    int num;
    unsigned char * ldata;
    char outbuf[128];

    linewidth = 80;
    linemax = 16;
    msg = NULL;
    asctab = linewidth - (linemax + (linemax / 4) +  1);
    ldataix = 0;
    ldata = (unsigned char *)vldata;
    if (!outf) outf = stdout;

    if (fmt) {
        va_start(args, fmt);
        msg = VXSmprintf(fmt, args);
        va_end(args);
        if (!(logflags & HLOG_FLAG_MSG_EVERY_LINE)) {
            fprintf(outf, "%s\n", msg);
        }
    }

    remain = ldatalen;
    while (remain) {
        if (remain > linemax) towrite = linemax;
        else                  towrite = remain;

        if (msg && (logflags & HLOG_FLAG_MSG_EVERY_LINE)) {
            if (logflags & HLOG_FLAG_USE_ADDR) {
                sprintf(outbuf, "%s %08x:", msg, ((int)(ldata + ldataix) & 0xFFFFFFFFL));
            } else {
                sprintf(outbuf, "%s %04x:", msg, ldataix);
            }
        } else {
            if (logflags & HLOG_FLAG_USE_ADDR) {
                sprintf(outbuf, "%08x:", ((int)(ldata + ldataix) & 0xFFFFFFFFL));
            } else {
                sprintf(outbuf, "%04x:", ldataix);
            }
        }
        outlen = IStrlen(outbuf);

        for (ii = 0; ii < towrite; ii++) {
            outbuf[outlen++] = ' ';
            //if (ii && !(ii & 3)) outbuf[outlen++] = ' ';
            if (ii && !(ii & 7)) outbuf[outlen++] = ' ';

            num = (int)((unsigned char)ldata[ldataix + ii] & 0xF0) >> 4;
            if (num > 9) outbuf[outlen++] = num + 'W';
            else         outbuf[outlen++] = num + '0';

            num = (int)((unsigned char)ldata[ldataix + ii] & 0x0F);
            if (num > 9) outbuf[outlen++] = num + 'W';
            else         outbuf[outlen++] = num + '0';

        }
        while (outlen < asctab) outbuf[outlen++] = ' ';

        for (ii = 0; ii < towrite; ii++) {
            if (!(ii & 3)) outbuf[outlen++] = ' ';

            num = (int)((unsigned char)ldata[ldataix + ii]);
            if (num < ' ' || num > '~') outbuf[outlen++] = '.';
            else                        outbuf[outlen++] = ldata[ldataix + ii];
        }

        outbuf[outlen] = '\0';
        fprintf(outf, "%s\n", outbuf);
        remain -= towrite;
        ldataix += towrite;
    }
    Free(msg);
}
/***************************************************************/
