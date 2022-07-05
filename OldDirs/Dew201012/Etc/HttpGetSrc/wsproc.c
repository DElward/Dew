/*
**  wsproc.c -- wsdl processing functions
*/
/***************************************************************/
/*
*/
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>

#include "HttpGet.h"
#include "wsdl.h"
#include "xmlh.h"
#include "dbtreeh.h"
#include "snprfh.h"
#include "wsproc.h"
#include "rcall.h"

#define SNPRINTF Snprintf
#define XMLCTOSTR strmcpy
typedef int intr;

#define JSONERR_NOOPEN          101
#define JSONERR_EXPOPEN         102
#define JSONERR_EXPCLOSE        103
#define JSONERR_EXCOLON         104
#define JSONERR_EXQUOTE         105
#define JSONERR_BADESCAPE       106
#define JSONERR_BADHEXCHAR      107
#define JSONERR_BADVALUE        108
#define JSONERR_BADNUMBER       109
#define JSONERR_EXPCLOSEBRACKET 110

#define JSON_K_TBUF_INC          128
#define JSON_K_LBUF_INC          128
#define JSON_K_DEFAULT_FBUFSIZE  4096
#define JSON_K_DEFAULT_TBUFSIZE  256
#define JSON_K_DEFAULT_LBUFSIZE  256
#define JSON_ERRPARM_SIZE        128

#define JSON_ENC_UTF_8           0  /* default */
#define JSON_ENC_1_BYTE          1  /* Single byte encoding */
#define JSON_ENC_UNICODE_BE      2  /* Unicode big endian */
#define JSON_ENC_UNICODE_LE      3  /* Unicode little endian */

#define JSONCEOF             ((tWCHAR)EOF)
#define JSON_IS_WHITE(jc) ((jc)<128 ? isspace(jc) : 0)

struct jsons_file { /* jf */
    int                       jf_err;
    char                      jf_errparm[JSON_ERRPARM_SIZE];
    char                      jf_errline[JSON_ERRPARM_SIZE];
    intr                      jf_errlinenum;
};

struct json_stream { /* js */
    struct jsons_file *       js_jf;
    int                       js_err;
    char                      js_errparm[JSON_ERRPARM_SIZE];

    /* file buffer */
    unsigned char           * js_fbuf;
    intr                      js_fbuflen;
    intr                      js_fbufsize;
    intr                      js_fbufix;
    intr                      js_prev_fbufix; /* for unget_char */
    int                       js_iseof;
    int                       js_tadv;
    int                       js_encoding;
    /* token buffer */
    tWCHAR                  * js_tbuf;
    intr                      js_tbuflen;
    intr                      js_tbufsize;
    /* line buffer */
    tWCHAR                  * js_lbuf;
    intr                      js_lbuflen;
    intr                      js_lbufsize;
    intr                      js_linenum;
};

#define JSONCMAXSZ           4 /* Maximum number of bytes in raw char */
#define JSONCEOF             ((tWCHAR)EOF)
/***************************************************************/
/*     Utility functions                                       */
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
int xmlcstrcmp(const tWCHAR * xtok, const char * buf) {

    int cmp;
    int ii;

    if (!xtok) return (-1);

    cmp = 0;
    ii  = 0;
    while (!cmp && xtok[ii] && buf[ii]) {
        if (xtok[ii] == buf[ii])     ii++;
        else if (xtok[ii] < buf[ii]) cmp = -1;
        else                         cmp =  1;
    }
    if (!cmp) {
        if (xtok[ii]) cmp = 1;
        else if (buf[ii]) cmp = -1;
    }

    return (cmp);
}
/***************************************************************/
/* JSON stub functions                                         */
/***************************************************************/
static void Json_fread(struct json_stream * js, intr bufix) {

    js->js_iseof = 1;
}
/***************************************************************/
/* JSON stream functions                                       */
/***************************************************************/
/***************************************************************/
struct jsontree * jsonp_new_jsontree(void) {

    struct jsontree * jt;

    jt = New(struct jsontree, 1);
    jt->jt_jv = NULL;

    return (jt);
}
/***************************************************************/
static void Json_free_stream(struct json_stream * js) {

    Free(js->js_fbuf);
    Free(js->js_tbuf);
    Free(js->js_lbuf);

    Free(js);
}
/***************************************************************/
static struct json_stream * Json_new_json_stream(struct jsons_file * jf) {

    struct json_stream * js;

    js = New(struct json_stream, 1);

    js->js_jf           = jf;
    js->js_encoding     = 0;

    js->js_fbufsize     = JSON_K_DEFAULT_FBUFSIZE;
    js->js_fbuf         = New(unsigned char, js->js_fbufsize);
    js->js_fbuflen      = 0;
    js->js_fbufix       = 0;
    js->js_prev_fbufix  = 0;
    js->js_iseof        = 0;

    js->js_tbufsize     = JSON_K_DEFAULT_TBUFSIZE;
    js->js_tbuf         = New(tWCHAR, js->js_tbufsize);
    js->js_tbuflen      = 0;
    js->js_tadv         = 0;

    js->js_lbufsize     = JSON_K_DEFAULT_LBUFSIZE;
    js->js_lbuf         = New(tWCHAR, js->js_lbufsize);
    js->js_lbuflen      = 0;
    js->js_linenum      = 0;

    return (js);
}
/***************************************************************/
static void Json_unget_char(struct json_stream * js) {

    js->js_fbufix = js->js_prev_fbufix;
    js->js_tadv = 0;
    if (js->js_lbuflen) js->js_lbuflen--;
}
/***************************************************************/
static tWCHAR Json_get_char(struct json_stream * js) {
/*
    Code range
    hexadecimal      UTF-8
    000000 - 00007F  0xxxxxxx
    000080 - 0007FF  110xxxxx 10xxxxxx
    000800 - 00FFFF  1110xxxx 10xxxxxx 10xxxxxx

*/
    tWCHAR jc;
    unsigned char cb[JSONCMAXSZ];

    if (js->js_iseof) return (JSONCEOF);

    if (js->js_tadv) {
        js->js_fbufix++;
    }

    if (js->js_fbufix == js->js_fbuflen) {
        Json_fread(js, 0);
        if (js->js_iseof) return (JSONCEOF);
    }

    js->js_prev_fbufix = js->js_fbufix;
    cb[0] = js->js_fbuf[js->js_fbufix];
    js->js_tadv = 1;

    if (js->js_encoding & 2 || (!js->js_encoding && (cb[0] & 0x80))) {
        js->js_fbufix++;
        if (js->js_fbufix == js->js_fbuflen) {
            Json_fread(js, 1);
            if (js->js_iseof) return (JSONCEOF);
            js->js_fbuf[0]      = cb[0];
            js->js_prev_fbufix  = 0;
        }

        cb[1] = js->js_fbuf[js->js_fbufix];
        if (js->js_encoding == JSON_ENC_UNICODE_BE) {
            jc = (tWCHAR)((cb[0] << 8) | cb[1]);
        } else if (js->js_encoding == JSON_ENC_UNICODE_LE) {
            jc = (tWCHAR)((cb[1] << 8) | cb[0]);
        } else { /* UTF-8 */
            if (cb[0] & 0x20) {
                js->js_fbufix++;
                if (js->js_fbufix == js->js_fbuflen) {
                    Json_fread(js, 2);
                    if (js->js_iseof) return (JSONCEOF);
                    js->js_fbuf[0]      = cb[0];
                    js->js_fbuf[1]      = cb[1];
                    js->js_prev_fbufix  = 0;
                }

                cb[2] = js->js_fbuf[js->js_fbufix];
                jc = (tWCHAR)(((cb[0] & 0x0F) << 12) |
                           ((cb[1] & 0x3F) <<  6) | (cb[2] & 0x3F));
            } else {
                jc = (tWCHAR)(((cb[0] & 0x1F) << 6) | (cb[1] & 0x3F));
            }
        }
    } else {
        jc = (tWCHAR)(cb[0]);
    }

    if (jc == '\n' || jc == '\r') {
        js->js_lbuflen = 0;
        if (jc == '\n') js->js_linenum++;
    } else if (js->js_lbuflen || !JSON_IS_WHITE(jc)) {
        if (js->js_lbuflen == js->js_lbufsize) {
            js->js_lbufsize += JSON_K_LBUF_INC;
            js->js_lbuf = Realloc(js->js_lbuf, tWCHAR, js->js_lbufsize);
        }
        js->js_lbuf[js->js_lbuflen] = jc;
        js->js_lbuflen++;
    }

    return (jc);
}
/***************************************************************/
static tWCHAR Json_peek_char(struct json_stream * js) {

    tWCHAR jc;

    jc = Json_get_char(js);
    Json_unget_char(js);

    return (jc);
}
/***************************************************************/
static tWCHAR Json_get_nwchar(struct json_stream * js) {

    tWCHAR jc;

    do {
        jc = Json_get_char(js);
    } while (JSON_IS_WHITE(jc));
    if (jc == JSONCEOF) return (0);

    return (jc);
}
/***************************************************************/
static tWCHAR Json_peek_nwchar(struct json_stream * js) {

    tWCHAR jc;

    do {
        jc = Json_get_char(js);
    } while (JSON_IS_WHITE(jc));
    if (jc == JSONCEOF) return (0);

    Json_unget_char(js);

    return (jc);
}
/***************************************************************/
static void Json_expand_tbuf(struct json_stream * js) {

    js->js_tbufsize += JSON_K_TBUF_INC;
    js->js_tbuf = Realloc(js->js_tbuf, tWCHAR, js->js_tbufsize);
}
/***************************************************************/
int Json_err(struct json_stream * js) {

    return (js->js_err);
}
/***************************************************************/
/* JSON functions - Same as Wareehouse                         */
/***************************************************************/
void free_jsonvalue(void * vjv);

/***************************************************************/
/***************************************************************/
char * Json_err_msg(struct json_stream * js) {

    char errmsg[128];
    static char ferrmsg[256];

    switch (js->js_err) {
        case 0   : strcpy(errmsg,
            "No error.");
            break;

        case JSONERR_NOOPEN: strcpy(errmsg,
            "Could not find { to begin JSON.");
            break;

        case JSONERR_EXPOPEN: strcpy(errmsg,
            "Expecting { to begin JSON object.");
            break;

        case JSONERR_EXPCLOSE: strcpy(errmsg,
            "Expecting } to begin JSON object.");
            break;

        case JSONERR_EXCOLON: strcpy(errmsg,
            "Expecting : to separate JSON object.");
            break;

        case JSONERR_EXQUOTE: strcpy(errmsg,
            "Expecting closing \" to terminate JSON string.");
            break;

        case JSONERR_BADESCAPE: strcpy(errmsg,
            "Invalid characacter following backslah in JSON string.");
            break;

        case JSONERR_BADHEXCHAR: strcpy(errmsg,
            "Invalid hexadecimal characacter following \\u in JSON string.");
            break;

        case JSONERR_BADVALUE: strcpy(errmsg,
            "Invalid JSON value.");
            break;

        case JSONERR_BADNUMBER: strcpy(errmsg,
            "Invalid JSON number.");
            break;

        case JSONERR_EXPCLOSEBRACKET: strcpy(errmsg,
            "Expecting ] to terminate JSON array.");
            break;

        default         : sprintf(errmsg,
            "Unrecognized JSON error number %d", js->js_err);
            break;
    }

    if (js->js_errparm[0]) {
        if (js->js_jf && js->js_jf->jf_errline[0]) {
            SNPRINTF(ferrmsg, sizeof(ferrmsg),
                "%s\nFound \"%s\" in line #%d:\n%s",
                    errmsg, js->js_errparm,
                    js->js_jf->jf_errlinenum, js->js_jf->jf_errline);
        } else {
            SNPRINTF(ferrmsg, sizeof(ferrmsg),
                "%s\nFound \"%s\"", errmsg, js->js_errparm);
        }
    } else {
        if (js->js_jf && js->js_jf->jf_errline[0]) {
            SNPRINTF(ferrmsg, sizeof(ferrmsg),
                "%s\nin line #%d:\n%s",
                 errmsg, js->js_jf->jf_errlinenum, js->js_jf->jf_errline);
        } else {
            strcpy(ferrmsg, errmsg);
        }
    }

    return (ferrmsg);
}
/***************************************************************/
static int Json_seterr(struct json_stream * js, int errnum) {

    intr len;

    if (!js) return (errnum);

    js->js_err = errnum;
    js->js_errparm[0] = '\0';

    if (js->js_jf) {
        if (js->js_lbuflen >= sizeof(js->js_jf->jf_errline)) {
            len = sizeof(js->js_jf->jf_errline) - 1;
        } else {
            len = js->js_lbuflen;
        }
        XMLCTOSTR(js->js_jf->jf_errline, js->js_lbuf, (int)len);
        js->js_jf->jf_errline[len] = '\0';
        js->js_jf->jf_errlinenum = js->js_linenum + 1;
    }

    return (errnum);
}
/***************************************************************/
void free_jsonnumber(struct jsonnumber * jn) {

    if (jn) {
        Free(jn->jn_buf);
        Free(jn);
    }
}
/***************************************************************/
void free_jsonstring(struct jsonstring * jb) {

    if (jb) {
        Free(jb->jb_buf);
        Free(jb);
    }
}
/***************************************************************/
void free_jsonarray(struct jsonarray * ja) {

    int ii;

    if (ja) {
        for (ii = 0; ii < ja->ja_nvals; ii++) free_jsonvalue(ja->ja_vals[ii]);
        Free(ja->ja_vals);
        Free(ja);
    }
}
/***************************************************************/
void free_jsonobject(struct jsonobject * jo) {

    if (jo) {
        dbtree_free(jo->jo_dbt, free_jsonvalue);
        Free(jo);
    }
}
/***************************************************************/
void free_jsonvalue(void * vjv) {

    struct jsonvalue * jv = (struct jsonvalue *)vjv;

    if (jv) {
        switch (jv->jv_vtyp) {
            case VTAB_VAL_OBJECT:
                free_jsonobject(jv->jv_val.jv_jo);
                break;
            case VTAB_VAL_ARRAY:
                free_jsonarray(jv->jv_val.jv_ja);
                break;
            case VTAB_VAL_STRING:
                free_jsonstring(jv->jv_val.jv_jb);
                break;
            case VTAB_VAL_NUMBER:
                free_jsonnumber(jv->jv_val.jv_jn);
                break;
            case VTAB_VAL_TRUE:
            case VTAB_VAL_FALSE:
            case VTAB_VAL_NULL:
                break;
            default:
                break;
        }
        Free(jv);
    }
}
/***************************************************************/
void free_jsontree(struct jsontree * jt) {

    if (jt) {
        free_jsonvalue(jt->jt_jv);
        Free(jt);
    }
}
/***************************************************************/
const char * get_jsonvalue_name(struct jsonvalue * jv) {

    if (!jv) {
        return "(null)";
    } else {
        switch (jv->jv_vtyp) {
            case VTAB_VAL_OBJECT:
                return "OBJECT";
                break;
            case VTAB_VAL_ARRAY:
                return "ARRAY";
                break;
            case VTAB_VAL_STRING:
                return "STRING";
                break;
            case VTAB_VAL_NUMBER:
                return "NUMBER";
                break;
            case VTAB_VAL_TRUE:
                return "TRUE";
                break;
            case VTAB_VAL_FALSE:
                return "FALSE";
                break;
            case VTAB_VAL_NULL:
                return "NULL";
                break;
            default:
                return "unknown";
                break;
        }
    }
    return "?";
}
/***************************************************************/
struct jsonstring * jsonp_new_jsonstring(tWCHAR * jbbuf, int jblen) {

    struct jsonstring * jb;

    jb = New(struct jsonstring, 1);
    jb->jb_bytes = (jblen + 1) * sizeof(tWCHAR);
    jb->jb_buf = New(tWCHAR, jblen + 1);
    memcpy(jb->jb_buf, jbbuf, jb->jb_bytes);
#if DEBUG_JSONC
    COPY_JSONC(jb->jb_debug_name, jbbuf);
#endif

    return (jb);
}
/***************************************************************/
struct jsonnumber * jsonp_new_jsonnumber(tWCHAR * jnbuf, int jnlen) {

    struct jsonnumber * jn;

    jn = New(struct jsonnumber, 1);
    jn->jn_bytes = (jnlen + 1) * sizeof(tWCHAR);
    jn->jn_buf = New(tWCHAR, jn->jn_bytes);
    memcpy(jn->jn_buf, jnbuf, jn->jn_bytes);

    return (jn);
}
/***************************************************************/
struct jsonobject * jsonp_new_jsonobject(void) {

    struct jsonobject * jo;

    jo = New(struct jsonobject, 1);
    jo->jo_dbt = dbtree_new();

    return (jo);
}
/***************************************************************/
struct jsonarray * jsonp_new_jsonarray(void) {

    struct jsonarray * ja;

    ja = New(struct jsonarray, 1);
    ja->ja_nvals = 0;
    ja->ja_vals  = NULL;

    return (ja);
}
/***************************************************************/
struct jsonvalue * jsonp_new_jsonvalue(int vtyp) {

    struct jsonvalue * jv;

    jv = New(struct jsonvalue, 1);
    jv->jv_vtyp = vtyp;
    jv->jv_val.jv_vp = NULL;

    return (jv);
}
/***************************************************************/
static int jsonp_append_array_value(struct jsonarray * ja,
                        struct jsonvalue * jv) {
    int result;

    result = 0;

    ja->ja_vals = Realloc(ja->ja_vals, struct jsonvalue *, ja->ja_nvals + 1);
    ja->ja_vals[ja->ja_nvals] = jv;
    ja->ja_nvals += 1;

    return (result);
}
/***************************************************************/
static int jsonp_value( struct json_stream * js,
                        struct jsonvalue ** pjv);

static int jsonp_array( struct json_stream * js,
                        struct jsonarray ** pja) {

    /* assumes [ already from Json_get_char() */

    int result;
    tWCHAR pc;
    int done;
    struct jsonvalue * jv;

    done = 0;
    result = 0;
    (*pja) = NULL;

    pc = Json_peek_nwchar(js);
    if (!pc || pc == ']') {
        (*pja) = jsonp_new_jsonarray();
        Json_get_nwchar(js); /* advance, but do not change pc */
        done = 1;
    }

    while (!result && !done) {
        if (!pc || pc == ']') {
            if (!(*pja)) (*pja) = jsonp_new_jsonarray();
            done = 1;
        } else {
            jv = NULL;
            result = jsonp_value(js, &jv);
            if (!result) {
                if (!(*pja)) (*pja) = jsonp_new_jsonarray();
                result = jsonp_append_array_value((*pja), jv);
            }
            if (!result) {
                pc = Json_get_nwchar(js);
                if (pc == ',') {
                    pc = Json_peek_nwchar(js);
                    if (pc == ']') {
                        /* Technically, this is an error */
                        /* We will ignore it for now */
                    }
                }
            }
        }
    }

    if (!result && pc != ']') {
        result = Json_seterr(js, JSONERR_EXPCLOSEBRACKET);
    }

    return (result);
}
/***************************************************************/
static struct jsonnumber * Json_get_number(tWCHAR jc,
                        struct json_stream * js) {

    int done;
    int state;
    struct jsonnumber * jn;
/*
**  State:
**      0 - Initial, no digits
**      1 - Leading digits
**      2 - Just found '.', no digits yet
**      3 - Found '.', in digits
**      4 - Just found 'e' or 'E', no digits yet
**      5 - In digits after 'E', no '+' or '-'
**      6 - Just found '+' or '-' after 'E', no digits yet
**      7 - In digits after 'E+' or 'E-'
*/

    done   = 0;
    state   = 0;
    if (isdigit(jc))state = 1;
    js->js_tbuflen = 0;

    do {
        if (js->js_tbuflen == js->js_tbufsize) {
            Json_expand_tbuf(js);
        }
        js->js_tbuf[js->js_tbuflen] = jc;
        js->js_tbuflen++;

        jc = Json_get_char(js);
        if (isdigit(jc)) {
            if (!(state & 1)) state++;
        } else {
            if (jc == '.' && state == 1) {
                state = 2;
            } else if ((jc == 'e' || jc == 'E') && (state == 1 || state == 3)) {
                state = 4;
            } else if ((jc == '-' || jc == '+') && state == 4) {
                state = 6;
            } else {
                if (jc != JSONCEOF) Json_unget_char(js);
                done = 1;
            }
        }
    } while (!done);

    if (js->js_tbuflen == js->js_tbufsize) {
        Json_expand_tbuf(js);
    }
    js->js_tbuf[js->js_tbuflen] = 0;
    jn = jsonp_new_jsonnumber(js->js_tbuf, (int)js->js_tbuflen);

    return (jn);
}
/***************************************************************/
static int jsonp_number(tWCHAR jc,
                       struct json_stream * js,
                           struct jsonnumber ** pjn) {


    int result;
    tWCHAR pc;

    result = 0;
    (*pjn) = Json_get_number(jc, js);
    pc = Json_peek_nwchar(js);
    if (!(*pjn) || !((*pjn)->jn_bytes) || isalnum(pc) ||
        pc == '.' || pc == '-' || pc == '+') {
        result = Json_seterr(js, JSONERR_BADNUMBER);
    }

    return (result);
}
/***************************************************************/
static tWCHAR * Json_next_token_alpha(tWCHAR jc, struct json_stream * js) {

    int done;

    done   = 0;
    js->js_tbuflen = 0;

    do {
        if (js->js_tbuflen == js->js_tbufsize) {
            Json_expand_tbuf(js);
        }
        js->js_tbuf[js->js_tbuflen] = jc;
        js->js_tbuflen++;

        jc = Json_get_char(js);
        if (!isalpha(jc)) {
            if (jc != JSONCEOF) Json_unget_char(js);
            done = 1;
        }
    } while (!done);
    js->js_tbuf[js->js_tbuflen] = 0;

    return (js->js_tbuf);
}
/***************************************************************/
static int jsonp_true(tWCHAR jc,
                       struct json_stream * js) {


    int result;
    tWCHAR * jtok;

    result = 0;
    jtok = Json_next_token_alpha(jc, js);
    if (xmlcstrcmp(jtok, "true")) {
        result = Json_seterr(js, JSONERR_BADVALUE);
    }

    return (result);
}
/***************************************************************/
static int jsonp_false(tWCHAR jc,
                       struct json_stream * js) {


    int result;
    tWCHAR * jtok;

    result = 0;
    jtok = Json_next_token_alpha(jc, js);
    if (xmlcstrcmp(jtok, "false")) {
        result = Json_seterr(js, JSONERR_BADVALUE);
    }

    return (result);
}
/***************************************************************/
static int jsonp_null(tWCHAR jc,
                       struct json_stream * js) {


    int result;
    tWCHAR * jtok;

    result = 0;
    jtok = Json_next_token_alpha(jc, js);
    if (xmlcstrcmp(jtok, "null")) {
        result = Json_seterr(js, JSONERR_BADVALUE);
    }

    return (result);
}
/***************************************************************/
#define VALID_HEX_CHAR(c) \
 ((((c)>='0'&&(c)<='9')||((c)>='A'&&(c)<='F')||((c)>='a'&&(c)<='f'))?1:0)
static int cvt_hex4(tWCHAR ujc[], tWCHAR * pjc) {


    int result;
    int ii;
    int dig;

    result = 0;
    (*pjc) = 0;
    for (ii = 0; !result && ii < 4; ii++) {
        if (ujc[ii] >= '0' && ujc[ii] <= '9')      dig = ujc[ii] - '0';
        else if (ujc[ii] >= 'A' && ujc[ii] <= 'F') dig = ujc[ii] - '7';
        else if (ujc[ii] >= 'a' && ujc[ii] <= 'f') dig = ujc[ii] - 'W';
        else {
            dig = 0;
            result = (-1);
        }
        (*pjc) = ((*pjc) << 4) | dig;
    }

    return (result);
}
/***************************************************************/
struct jsonstring * Json_get_string(struct json_stream * js) {

    int done;
    struct jsonstring * jb;
    tWCHAR jc;
    tWCHAR ujc[4];

    done   = 0;
    js->js_tbuflen = 0;

    while (!done) {
        jc = Json_get_char(js);
        if (!jc) {
            Json_seterr(js, JSONERR_EXQUOTE);
            done = 1;
        } else if (jc == '\"') {
            done = 1;
        } else {
            if (js->js_tbuflen == js->js_tbufsize) {
                Json_expand_tbuf(js);
            }
            if (jc == '\\') {
                jc = Json_get_char(js);
                switch (jc) {
                    case '\\':
                        jc = '\\';
                        break;
                    case '\"':
                        jc = '\"';
                        break;
                    case 'b':
                        jc = '\b';
                        break;
                    case 'f':
                        jc = '\f';
                        break;
                    case 'n':
                        jc = '\n';
                        break;
                    case 'r':
                        jc = '\r';
                        break;
                    case 't':
                        jc = '\t';
                        break;
                    case 'u':
                        memset(ujc, 0, sizeof(ujc));
                        ujc[0] = Json_get_char(js);
                        if (VALID_HEX_CHAR(ujc[0])) ujc[1] = Json_get_char(js);
                        if (VALID_HEX_CHAR(ujc[1])) ujc[2] = Json_get_char(js);
                        if (VALID_HEX_CHAR(ujc[2])) ujc[3] = Json_get_char(js);
                        done = cvt_hex4(ujc, &jc);
                        if (done) {
                            Json_seterr(js, JSONERR_BADHEXCHAR);
                        }
                        break;
                    default:
                        Json_seterr(js, JSONERR_BADESCAPE);
                        done = 1;
                        break;
                }
            }
            if (!done) {
                js->js_tbuf[js->js_tbuflen] = jc;
                js->js_tbuflen++;
            }
        }
    }

    if (js->js_tbuflen == js->js_tbufsize) {
        Json_expand_tbuf(js);
    }
    js->js_tbuf[js->js_tbuflen] = 0;
    jb = jsonp_new_jsonstring(js->js_tbuf, (int)js->js_tbuflen);

    return (jb);
}
/***************************************************************/
static int jsonp_string(struct json_stream * js,
                           struct jsonstring ** pjb) {

    /* assumes " already from Json_get_char() */

    int result;

    result = 0;

    (*pjb) = Json_get_string(js);
    result = Json_err(js);

    return (result);
}
/***************************************************************/
static int jsonp_insert_object_value(struct jsonobject * jo,
                        struct jsonstring * jb,
                        struct jsonvalue * jv) {
    int result;

    result = 0;
#if DEBUG_JSONC
    COPY_JSONC(jdbgbuf, jb->jb_buf);
#endif

    dbtree_insert(jo->jo_dbt, jb->jb_buf, jb->jb_bytes, jv);

    return (result);
}
/***************************************************************/
static int jsonp_object(struct json_stream * js,
                        struct jsonobject ** pjo) {

    /* assumes { already from Json_get_char() */

    int result;
    tWCHAR pc;
    int done;
    struct jsonvalue * jv;
    struct jsonstring * jb;

    done = 0;
    result = 0;
    (*pjo) = NULL;

    pc = Json_get_nwchar(js);
    while (!result && !done) {
        if (!pc || pc == '}') {
            if (!(*pjo)) (*pjo) = jsonp_new_jsonobject();
            done = 1;
        } else if (pc != '\"') {
            result = Json_seterr(js, JSONERR_EXQUOTE);
        } else {
            jb = Json_get_string(js);
            result = Json_err(js);
            if (!result) {
                pc = Json_get_nwchar(js);
                if (pc != ':') {
                    result = Json_seterr(js, JSONERR_EXCOLON);
                } else {
                    jv = NULL;
                    result = jsonp_value(js, &jv);
                    if (!result) {
                        if (!(*pjo)) (*pjo) = jsonp_new_jsonobject();
                        result = jsonp_insert_object_value((*pjo), jb, jv);
                    }
                    if (!result) {
                        pc = Json_get_nwchar(js);
                        if (pc == ',') {
                            pc = Json_get_nwchar(js);
                            if (pc == '}') {
                                /* Technically, this is an error */
                                /* We will ignore it for now */
                            }
                        }
                    }
                }
                free_jsonstring(jb);
            }
        }
    }

    if (!result && pc != '}') {
        result = Json_seterr(js, JSONERR_EXPCLOSE);
    }

    return (result);
}
/***************************************************************/
static int jsonp_value( struct json_stream * js,
                        struct jsonvalue ** pjv) {

    int result;
    tWCHAR pc;
    int done;
    tWCHAR *  jtok;

    done = 0;
    result = 0;

    pc = Json_get_nwchar(js);
    if (pc == '\"') {
        (*pjv) = jsonp_new_jsonvalue(VTAB_VAL_STRING);
        result = jsonp_string(js, &((*pjv)->jv_val.jv_jb));
    } else if (pc == '[') {
        (*pjv) = jsonp_new_jsonvalue(VTAB_VAL_ARRAY);
        result = jsonp_array(js, &((*pjv)->jv_val.jv_ja));
    } else if (pc == '{') {
        (*pjv) = jsonp_new_jsonvalue(VTAB_VAL_OBJECT);
        result = jsonp_object(js, &((*pjv)->jv_val.jv_jo));
    } else if (pc == '-' || isdigit(pc)) {
        (*pjv) = jsonp_new_jsonvalue(VTAB_VAL_NUMBER);
        result = jsonp_number(pc, js, &((*pjv)->jv_val.jv_jn));
    } else if (pc == 't') {
        jtok = Json_next_token_alpha(pc, js);
        if (!xmlcstrcmp(jtok, "true")) {
            (*pjv) = jsonp_new_jsonvalue(VTAB_VAL_TRUE);
        } else {
            result = Json_seterr(js, JSONERR_BADVALUE);
        }
    } else if (pc == 'f') {
        jtok = Json_next_token_alpha(pc, js);
        if (!xmlcstrcmp(jtok, "false")) {
            (*pjv) = jsonp_new_jsonvalue(VTAB_VAL_FALSE);
        } else {
            result = Json_seterr(js, JSONERR_BADVALUE);
        }
    } else if (pc == 'n') {
        jtok = Json_next_token_alpha(pc, js);
        if (!xmlcstrcmp(jtok, "null")) {
            (*pjv) = jsonp_new_jsonvalue(VTAB_VAL_NULL);
        } else {
            result = Json_seterr(js, JSONERR_BADVALUE);
        }
    } else {
        result = Json_seterr(js, JSONERR_BADVALUE);
    }
    return (result);
}
/***************************************************************/
int jsonp_first_value(int vtyp,
                      struct json_stream * js,
                      struct jsonvalue ** pjv) {

    int result = 0;
    tWCHAR pc;

    (*pjv) = jsonp_new_jsonvalue(vtyp);

    switch (vtyp) {
        case VTAB_VAL_OBJECT:
            pc = Json_get_char(js);
            result = jsonp_object(js, &((*pjv)->jv_val.jv_jo));
            break;
        case VTAB_VAL_ARRAY:
            pc = Json_get_char(js);
            result = jsonp_array(js, &((*pjv)->jv_val.jv_ja));
            break;
        case VTAB_VAL_STRING:
            pc = Json_get_char(js);
            result = jsonp_string(js, &((*pjv)->jv_val.jv_jb));
            break;
        case VTAB_VAL_NUMBER:
            pc = Json_get_char(js);
            result = jsonp_number(pc, js, &((*pjv)->jv_val.jv_jn));
            break;
        case VTAB_VAL_TRUE:
            pc = Json_get_char(js);
            result = jsonp_true(pc, js);
            break;
        case VTAB_VAL_FALSE:
            pc = Json_get_char(js);
            result = jsonp_false(pc, js);
            break;
        case VTAB_VAL_NULL:
            pc = Json_get_char(js);
            result = jsonp_null(pc, js);
            break;
        default:
            result = Json_seterr(js, JSONERR_BADVALUE);
            break;
    }

    return (result);
}

/***************************************************************/
int jsonpp_prolog(struct json_stream * js, int prolog_flags) {
/*
**  Find and start with {
*/
    tWCHAR pc;
    int found;
    int done;
    int vtyp;

    vtyp = 0;
    done = 0;
    found = 0;
    while (!done && !found) {
        pc = Json_peek_nwchar(js);
        if (!pc) {
            done = 1;
        } else if (pc ==  '{') {
            found = 1;
            vtyp = VTAB_VAL_OBJECT;
        } else if (pc ==  '[') {
            found = 1;
            vtyp = VTAB_VAL_ARRAY;
        } else if (!(prolog_flags & JSON_PROLOG_SEARCH)) {
            done = 1;
            if (pc ==  '-' || isdigit(pc)) {
                found = 1;
                vtyp = VTAB_VAL_NUMBER;
            } else if (pc ==  '\"') {
                found = 1;
                vtyp = VTAB_VAL_STRING;
            }
        } else {
            if (pc ==  '=') {
                pc = Json_get_char(js);
                pc = Json_peek_nwchar(js);
                     if (pc ==  '{')  vtyp = VTAB_VAL_OBJECT;
                else if (pc ==  '[')  vtyp = VTAB_VAL_ARRAY;
                else if (pc ==  '\"') vtyp = VTAB_VAL_STRING;
                else if (pc ==  '-' || isdigit(pc)) vtyp = VTAB_VAL_NUMBER;
                if (vtyp) found = 1;
            } else {
                Json_get_char(js);
            }
        }
    }

    if (!found) {
        if (prolog_flags & JSON_PROLOG_NONE_OK) {
            vtyp = VTAB_VAL_NONE;
        } else {
            Json_seterr(js, JSONERR_NOOPEN);
            return (VTAB_VAL_ERROR);
        }
    }

    return (vtyp);
}
/***************************************************************/
static int Json_begin_json(struct json_stream * js, int prolog_flags) {
/*
*/
    int vtyp;

    vtyp = jsonpp_prolog(js, prolog_flags);

    return (vtyp);
}
/***************************************************************/
/* End of JSON functions from Warehouse                        */
/***************************************************************/
int Json_process_json(int vtyp,
                        struct json_stream * js,
                        struct jsontree ** pjt) {

    int result;
    tWCHAR pc;
    int nvtyp;
    struct jsonarray * ja;
    struct jsonvalue * jv;

    result = jsonp_first_value(vtyp, js, &((*pjt)->jt_jv));
    if (!result) {
        pc = Json_peek_nwchar(js);
        if (pc == ',') {
            ja  = jsonp_new_jsonarray();
            jsonp_append_array_value(ja, (*pjt)->jt_jv);

            (*pjt)->jt_jv = jsonp_new_jsonvalue(VTAB_VAL_ARRAY);
            (*pjt)->jt_jv->jv_val.jv_ja = ja;

            while (!result && pc == ',') {
                pc = Json_get_char(js); /* comma */
                nvtyp = jsonpp_prolog(js, 0);
                if (Json_err(js)) {
                    pc = -1;
                } else {
                    result = jsonp_first_value(nvtyp, js, &jv);
                    if (!result) {
                        jsonp_append_array_value(ja, jv);
                        pc = Json_peek_nwchar(js);
                    }
                }
            }
        }
    }

    return (result);
}
/***************************************************************/
struct json_stream * new_json_stream_from_chars(const char * charbuf)
{
    int charbuflen;
    struct json_stream * js;

    js = Json_new_json_stream(NULL);
    charbuflen = IStrlen(charbuf);
    js->js_fbufsize     = charbuflen + 1;
    js->js_fbuf         =
        Realloc(js->js_fbuf, unsigned char, js->js_fbufsize);
    js->js_fbuflen      = charbuflen;
    memcpy(js->js_fbuf, charbuf, charbuflen + 1);

    return (js);
}
/***************************************************************/
struct xmltree * xmlns_insert_subel(
    struct xmltree * xt,
    const char * soap_ns,
    const char * elname) {

    char * full_elname;
    int soap_ns_len;
    int elname_len;

    soap_ns_len = IStrlen(soap_ns);
    elname_len  = IStrlen(elname);

    full_elname = New(char, soap_ns_len + elname_len + 2);
    memcpy(full_elname, soap_ns, soap_ns_len);
    full_elname[soap_ns_len] = ':';
    memcpy(full_elname + soap_ns_len + 1, elname, elname_len + 1);

    xt = xmlp_new_element(full_elname, xt);

    return (xt);
}
/***************************************************************/
void xmlns_insert_attr(
    struct xmltree * xt,
    const char * soap_ns,
    const char * attrname,
    const char * attrval) {

    char * full_attrname;
    int soap_ns_len;
    int attrname_len;

    soap_ns_len = IStrlen(soap_ns);
    attrname_len  = IStrlen(attrname);

    full_attrname = New(char, soap_ns_len + attrname_len + 2);
    memcpy(full_attrname, soap_ns, soap_ns_len);
    full_attrname[soap_ns_len] = ':';
    memcpy(full_attrname + soap_ns_len + 1, attrname, attrname_len + 1);

    xmlp_insert_attr(xt, full_attrname, attrval, 0);

    Free(full_attrname);
}
/***************************************************************/
struct xmltree * wsdlc_new_soap_envelope(const char * soap_ns,
                            int env_flags) {

    struct xmltree * soap_env_xt;

    soap_env_xt = xmlns_insert_subel(NULL, soap_ns, SOAP_ENVELOPE);
    if (env_flags & SOAP_ENVFLAG_SOAP_NS) {
        xmlns_insert_attr(soap_env_xt,
            WSSH_TYP_XMLNS, soap_ns, NSURI_SOAP11_ENVELOPE);
    }

    if (env_flags & SOAP_ENVFLAG_SOAP12_NS) {
        xmlns_insert_attr(soap_env_xt,
            WSSH_TYP_XMLNS, soap_ns, NSURI_SOAP12_ENVELOPE);
    }

    return (soap_env_xt);
}
/***************************************************************/
/* forward */
int wsdlc_add_parm_wscontent(struct wsglob * wsg,
            struct wssopinfo * wssopi,
            struct wssoperation * wssop,
            struct wsspinfo * wsspi,
            struct jsonvalue * parm_jv, 
            const char * soap_call_ns,
            struct xmltree * soap_call_xt,
            struct wscontent * wstc);
/***************************************************************/
int wsdlc_add_parm_wscomplextype(struct wsglob * wsg,
            struct wssopinfo * wssopi,
            struct wssoperation * wssop,
            struct wsspinfo * wsspi,
            struct jsonvalue * parm_jv, 
            const char * soap_call_ns,
            struct xmltree * soap_call_xt,
            struct wscomplextype * wstx)
{
    int wstat = 0;
    int ii;

    for (ii = 0; !wstat && ii < wstx->wstx_ncontent; ii++) {
        wstat = wsdlc_add_parm_wscontent(
            wsg, wssopi, wssop, wsspi,
            parm_jv, soap_call_ns, soap_call_xt,
            wstx->wstx_acontent[ii]);
    }

    return (wstat);
}
/***************************************************************/
char * wsdlc_get_content_name(struct wscontent * wstc) {

    char * cname;

    cname = NULL;

    if (!wstc) return (NULL);

    switch (wstc->wstc_content_type) {
        case WSST_ELEMENT :
            cname = wstc->wstc_itm.wstcu_wselement->wste_name;
            break;
        case WSST_COMPLEX_TYPE :
            cname = wstc->wstc_itm.wstcu_wscomplextype->wstx_name;
            break;
        case WSST_ANY :
            /* cname = wstc->wstc_itm.wstcu_wsany */
            break;
        case WSST_ANY_ATTRIBUTE :
            /* cname = wstc->wstc_itm.wstcu_wsanyattribute */
            break;
        case WSST_SEQUENCE :
            /* cname = wstc->wstc_itm.wstcu_wssequence */;
            break;
        case WSST_SIMPLE_TYPE :
            cname = wstc->wstc_itm.wstcu_wssimpletype->wstt_name;
            break;
        case WSST_RESTRICTION :
            /* cname = wstc->wstc_itm.wstcu_wsrestriction */;
            break;
        case WSST_ENUMERATION :
            /* cname = wstc->wstc_itm.wstcu_wsenumeration */;
            break;
        case WSST_COMPLEX_CONTENT :
            /* cname = wstc->wstc_itm.wstcu_wscomplexcontent */;
            break;
        case WSST_EXTENSION :
            /* cname = wstc->wstc_itm.wstcu_wsextension */;
            break;
        case WSST_SIMPLE_CONTENT :
            /* cname = wstc->wstc_itm.wstcu_wssimplecontent */;
            break;
        case WSST_ATTRIBUTE :
            cname = wstc->wstc_itm.wstcu_wsattribute->wstb_name;
            break;
        case WSST_LIST :
            /* cname = wstc->wstc_itm.wstcu_wslist */;
            break;
        case WSST_UNION :
            /* cname = wstc->wstc_itm.wstcu_wsunion */;
            break;
        case WSST_ANNOTATION :
            /* cname = wstc->wstc_itm.wstcu_wsannotation */;
            break;
        case WSST_APPINFO :
            /* cname = wstc->wstc_itm.wstcu_wsappinfo */;
            break;
        case WSST_DOCUMENTATION :
            /* cname = wstc->wstc_itm.wstcu_wsdocumentation */;
            break;
        case WSST_ALL :
            /* cname = wstc->wstc_itm.wstcu_wsall */;
            break;
        case WSST_CHOICE :
            /* cname = wstc->wstc_itm.wstcu_wschoice */;
            break;
        case WSST_GROUP :
            cname = wstc->wstc_itm.wstcu_wsgroup->wstg_name;
            break;
        default:
            break;
    }

    return (cname);
}
/***************************************************************/
int wsdlc_add_parm_wssequence(struct wsglob * wsg,
            struct wssopinfo * wssopi,
            struct wssoperation * wssop,
            struct wsspinfo * wsspi,
            struct jsonvalue * parm_jv, 
            const char * soap_call_ns,
            struct xmltree * soap_call_xt,
            struct wssequence * wstq)
{
    int wstat = 0;
    int ii;
    char * cname;
    void ** vhand;
    struct jsonvalue * jv;

    switch (parm_jv->jv_vtyp) {
        case VTAB_VAL_OBJECT:
            for (ii = 0; !wstat && ii < wstq->wstq_ncontent; ii++) {
                cname = wsdlc_get_content_name(wstq->wstq_acontent[ii]);
                if (!cname) {
                    wstat = ws_set_error(wsg, WSE_NOSEQCONTENTNAME,
                                "");
                } else {
                    vhand = dbtree_find(parm_jv->jv_val.jv_jo->jo_dbt,
                        cname, IStrlen(cname) + 1); /* need wsdl_to_json(cname) */
                    if (!vhand) {
                        wstat = ws_set_error(wsg, WSE_FINDPARM,
                                    "%s", cname);
                    } else {
                        jv = *(struct jsonvalue **)vhand;
                        wstat = wsdlc_add_parm_wscontent(
                            wsg, wssopi, wssop, wsspi,
                            jv,
                            soap_call_ns, soap_call_xt,
                            wstq->wstq_acontent[ii]);
                    }
                }
            }
            break;
        case VTAB_VAL_ARRAY:
            if (wstq->wstq_ncontent > parm_jv->jv_val.jv_ja->ja_nvals) {
                wstat = ws_set_error(wsg, WSE_INSUFFICIENTPARMS, "");
            } else if (wstq->wstq_ncontent < parm_jv->jv_val.jv_ja->ja_nvals) {
                wstat = ws_set_error(wsg, WSE_TOOMANYPARMS, "");
            } else {
                for (ii = 0; !wstat && ii < wstq->wstq_ncontent; ii++) {
                    wstat = wsdlc_add_parm_wscontent(
                        wsg, wssopi, wssop, wsspi,
                        parm_jv->jv_val.jv_ja->ja_vals[ii],
                        soap_call_ns, soap_call_xt,
                        wstq->wstq_acontent[ii]);
                }
            }
            break;
        case VTAB_VAL_STRING:
        case VTAB_VAL_NUMBER:
        case VTAB_VAL_TRUE:
        case VTAB_VAL_FALSE:
        case VTAB_VAL_NULL:
            if (wstq->wstq_ncontent > 1) {
                wstat = ws_set_error(wsg, WSE_INSUFFICIENTPARMS, "");
            } else if (wstq->wstq_ncontent < 1) {
                wstat = ws_set_error(wsg, WSE_TOOMANYPARMS, "");
            } else {
                wstat = wsdlc_add_parm_wscontent(
                    wsg, wssopi, wssop, wsspi,
                    parm_jv, soap_call_ns, soap_call_xt,
                    wstq->wstq_acontent[0]);
            }
            break;

        default:
            wstat = ws_set_error(wsg, WSE_UNSUPPORTEDSEQYPE,
                        "%d", parm_jv->jv_vtyp);
            break;
    }

    return (wstat);
}
/***************************************************************/
char * jsonstring_to_xsd_string(struct jsonstring * parm_jb) {

    char * xsd_string;
    int string_len;

    string_len = parm_jb->jb_bytes;
    xsd_string = New(char, string_len);
    memcpy(xsd_string, parm_jb->jb_buf, string_len - 1);
    xsd_string[string_len] = '\0';

    return (xsd_string);
}
/***************************************************************/
int wsdlc_add_parm_jsonstring_to_xsd_element(struct wsglob * wsg,
            struct wssopinfo * wssopi,
            struct wssoperation * wssop,
            struct wsspinfo * wsspi,
            struct jsonstring * parm_jb,
            const char * soap_call_ns,
            struct xmltree * soap_call_xt,
            struct wselement * wste,
            const char * xsd_type,
            struct xmltree * soap_parm_xt)
{
    int wstat = 0;

    if (!strcmp(xsd_type, "string")) {
        soap_parm_xt->eldata = jsonstring_to_xsd_string(parm_jb);
    } else {
        wstat = ws_set_error(wsg, WSE_PARMCONVERSION,
                    "%s to %s", "string", wste->wste_type);
    }

    return (wstat);
}
/***************************************************************/
int wsdlc_add_parm_jsonarray_to_xsd_element(struct wsglob * wsg,
            struct wssopinfo * wssopi,
            struct wssoperation * wssop,
            struct wsspinfo * wsspi,
            struct jsonarray * parm_ja,
            const char * soap_call_ns,
            struct xmltree * soap_call_xt,
            struct wselement * wste,
            const char * xsd_type,
            struct xmltree * soap_parm_xt)
{
    int wstat = 0;


    return (wstat);
}
/***************************************************************/
int wsdlc_add_parm_jt_to_xsd_element(struct wsglob * wsg,
            struct wssopinfo * wssopi,
            struct wssoperation * wssop,
            struct wsspinfo * wsspi,
            struct jsonvalue * parm_jv, 
            const char * soap_call_ns,
            struct xmltree * soap_call_xt,
            struct wselement * wste,
            const char * xsd_type,
            struct xmltree * soap_parm_xt)
{
    int wstat = 0;

    switch (parm_jv->jv_vtyp) {
            case VTAB_VAL_OBJECT:
                wstat = ws_set_error(wsg, WSE_UNSUPPORTEDPARMTYPE,
                            "%s", get_jsonvalue_name(parm_jv));
                break;
            case VTAB_VAL_ARRAY:
                wstat = wsdlc_add_parm_jsonarray_to_xsd_element(
                    wsg, wssopi, wssop, wsspi,
                    parm_jv->jv_val.jv_ja,
                    soap_call_ns, soap_call_xt, wste, xsd_type, soap_parm_xt);
                break;
            case VTAB_VAL_STRING:
                wstat = wsdlc_add_parm_jsonstring_to_xsd_element(
                    wsg, wssopi, wssop, wsspi,
                    parm_jv->jv_val.jv_jb,
                    soap_call_ns, soap_call_xt, wste, xsd_type, soap_parm_xt);
                break;
            case VTAB_VAL_NUMBER:
                wstat = ws_set_error(wsg, WSE_UNSUPPORTEDPARMTYPE,
                            "%s", get_jsonvalue_name(parm_jv));
                break;
            case VTAB_VAL_TRUE:
                wstat = ws_set_error(wsg, WSE_UNSUPPORTEDPARMTYPE,
                            "%s", get_jsonvalue_name(parm_jv));
                break;
            case VTAB_VAL_FALSE:
                wstat = ws_set_error(wsg, WSE_UNSUPPORTEDPARMTYPE,
                            "%s", get_jsonvalue_name(parm_jv));
                break;
            case VTAB_VAL_NULL:
                wstat = ws_set_error(wsg, WSE_UNSUPPORTEDPARMTYPE,
                            "%s", get_jsonvalue_name(parm_jv));
                break;
            default:
                wstat = ws_set_error(wsg, WSE_UNSUPPORTEDPARMTYPE,
                            "%d", parm_jv->jv_vtyp);
                break;
    }

    return (wstat);
}
/***************************************************************/
int wsdlc_add_parm_wselement_xsd(struct wsglob * wsg,
            struct wssopinfo * wssopi,
            struct wssoperation * wssop,
            struct wsspinfo * wsspi,
            struct jsonvalue * parm_jv, 
            const char * soap_call_ns,
            struct xmltree * soap_call_xt,
            struct wselement * wste,
            const char * xsd_type)
{
    int wstat = 0;
    struct xmltree * soap_parm_xt;

    soap_parm_xt = xmlns_insert_subel(soap_call_xt, soap_call_ns, wste->wste_name);

    wstat = wsdlc_add_parm_jt_to_xsd_element(
        wsg, wssopi, wssop, wsspi,
        parm_jv, soap_call_ns, soap_call_xt,
        wste, xsd_type, soap_parm_xt);

    return (wstat);
}
/***************************************************************/
int wsdlc_add_parm_wselement(struct wsglob * wsg,
            struct wssopinfo * wssopi,
            struct wssoperation * wssop,
            struct wsspinfo * wsspi,
            struct jsonvalue * parm_jv, 
            const char * soap_call_ns,
            struct xmltree * soap_call_xt,
            struct wselement * wste)
{
    int wstat = 0;
    int ii;
    struct wscontent * wstc;
    char * nsuri;
    char * xsd_type;

    if (wste->wste_type) {
        nsuri = find_qname_uri(wsg->wsg_wsd->wsd_wstypes, wste->wste_type);
        xsd_type = xml_get_name(wste->wste_type);
        if (nsuri && !strcmp(nsuri, NSURI_XSD)) {
            wstat = wsdlc_add_parm_wselement_xsd(
                wsg, wssopi, wssop, wsspi,
                parm_jv, soap_call_ns, soap_call_xt,
                wste, xsd_type);
        } else {
            wstc = wsdl_find_wscontent(wsg->wsg_wsd->wsd_wstypes, wste->wste_type);
            if (wstc) {
                wstat = wsdlc_add_parm_wscontent(
                    wsg, wssopi, wssop, wsspi,
                    parm_jv, soap_call_ns, soap_call_xt, wstc);
            } else {
                wstat = ws_set_error(wsg, WSE_FINDELEMENTTYPE,
                            "%s", wste->wste_type);
            }
        }
    } else {
        for (ii = 0; !wstat && ii < wste->wste_ncontent; ii++) {
            wstat = wsdlc_add_parm_wscontent(
                wsg, wssopi, wssop, wsspi,
                parm_jv, soap_call_ns, soap_call_xt,
                wste->wste_acontent[ii]);
        }
    }

    return (wstat);
}
/***************************************************************/
int wsdlc_add_parm_wscontent(struct wsglob * wsg,
            struct wssopinfo * wssopi,
            struct wssoperation * wssop,
            struct wsspinfo * wsspi,
            struct jsonvalue * parm_jv, 
            const char * soap_call_ns,
            struct xmltree * soap_call_xt,
            struct wscontent * wstc)
{
    int wstat = 0;

    switch (wstc->wstc_content_type) {
        case WSST_ELEMENT :
            wstat = wsdlc_add_parm_wselement(
                wsg, wssopi, wssop, wsspi,
                parm_jv, soap_call_ns, soap_call_xt,
                wstc->wstc_itm.wstcu_wselement);
            /* free_wselement(wstc->wstc_itm.wstcu_wselement); */
            break;
        case WSST_COMPLEX_TYPE :
            wstat = wsdlc_add_parm_wscomplextype(
                wsg, wssopi, wssop, wsspi,
                parm_jv, soap_call_ns, soap_call_xt,
                wstc->wstc_itm.wstcu_wscomplextype);
            /* free_wscomplextype(wstc->wstc_itm.wstcu_wscomplextype); */
            break;
        case WSST_ANY :
            /* free_wsany(wstc->wstc_itm.wstcu_wsany); */
            break;
        case WSST_ANY_ATTRIBUTE :
            /* free_wsanyattribute(wstc->wstc_itm.wstcu_wsanyattribute); */
            break;
        case WSST_SEQUENCE :
            wstat = wsdlc_add_parm_wssequence(
                wsg, wssopi, wssop, wsspi,
                parm_jv, soap_call_ns, soap_call_xt,
                wstc->wstc_itm.wstcu_wssequence);
            /* free_wssequence(wstc->wstc_itm.wstcu_wssequence); */
            break;
        case WSST_SIMPLE_TYPE :
            /* free_wssimpletype(wstc->wstc_itm.wstcu_wssimpletype); */
            break;
        case WSST_RESTRICTION :
            /* free_wsrestriction(wstc->wstc_itm.wstcu_wsrestriction); */
            break;
        case WSST_ENUMERATION :
            /* free_wsenumeration(wstc->wstc_itm.wstcu_wsenumeration); */
            break;
        case WSST_COMPLEX_CONTENT :
            /* free_wscomplexcontent(wstc->wstc_itm.wstcu_wscomplexcontent); */
            break;
        case WSST_EXTENSION :
            /* free_wsextension(wstc->wstc_itm.wstcu_wsextension); */
            break;
        case WSST_SIMPLE_CONTENT :
            /* free_wssimplecontent(wstc->wstc_itm.wstcu_wssimplecontent); */
            break;
        case WSST_ATTRIBUTE :
            /* free_wsattribute(wstc->wstc_itm.wstcu_wsattribute); */
            break;
        case WSST_LIST :
            /* free_wslist(wstc->wstc_itm.wstcu_wslist); */
            break;
        case WSST_UNION :
            /* free_wsunion(wstc->wstc_itm.wstcu_wsunion); */
            break;
        case WSST_ANNOTATION :
            /* free_wsannotation(wstc->wstc_itm.wstcu_wsannotation); */
            break;
        case WSST_APPINFO :
            /* free_wsappinfo(wstc->wstc_itm.wstcu_wsappinfo); */
            break;
        case WSST_DOCUMENTATION :
            /* free_wsdocumentation(wstc->wstc_itm.wstcu_wsdocumentation); */
            break;
        case WSST_ALL :
            /* free_wsall(wstc->wstc_itm.wstcu_wsall); */
            break;
        case WSST_CHOICE :
            /* free_wschoice(wstc->wstc_itm.wstcu_wschoice); */
            break;
        case WSST_GROUP :
            /* free_wsgroup(wstc->wstc_itm.wstcu_wsgroup); */
            break;
        default:
            wstat = ws_set_error(wsg, WSE_UNIMPLEMENTEDPARM,
                        "%d", wstc->wstc_content_type);
            break;
    }

    return (wstat);
}
/***************************************************************/
int wsdlc_add_parameter(struct wsglob * wsg,
            struct wssopinfo * wssopi,
            struct wssoperation * wssop,
            struct wsspinfo * wsspi,
            struct jsonvalue * parm_jv, 
            const char * soap_call_ns,
            struct xmltree * soap_call_xt)
{
/*
**              <ns1:GetQuote xmlns:ns1='http://www.webserviceX.NET/'>
**                <ns1:symbol>PG</ns1:symbol>
**              </ns1:GetQuote>
********
**  POST /globalweather.asmx HTTP/1.1
**  Content-Type: text/xml
**  Content-Encoding: UTF-8
**  SOAPAction: http://www.webserviceX.NET/GetWeather
**  Host: www.webservicex.net
**  Content-Length: 293
**  
**  <s11:Envelope xmlns:s11='http://schemas.xmlsoap.org/soap/envelope/'>
**    <s11:Body>
**      <ns1:GetWeather xmlns:ns1='http://www.webserviceX.NET'>
**        <ns1:CityName>Shannon Airport</ns1:CityName>
**        <ns1:CountryName>Ireland</ns1:CountryName>
**      </ns1:GetWeather>
**    </s11:Body>
**  </s11:Envelope>
*/
    int wstat = 0;
    struct wscontent * wstc;

    wstc = NULL;
    if (wsspi->wsspi_element) {
        wstc = wsdl_find_wscontent(wsg->wsg_wsd->wsd_wstypes, wsspi->wsspi_element);
    }

    if (wstc) {
        wstat = wsdlc_add_parm_wscontent(
            wsg, wssopi, wssop, wsspi,
            parm_jv, soap_call_ns, soap_call_xt, wstc);
    }

    return (wstat);
}
/***************************************************************/
int wsdlc_add_parms(struct wsglob * wsg,
            struct wssopinfo * wssopi,
            struct wssoperation * wssop,
            struct jsonvalue * parm_jv,
            const char * soap_call_ns,
            struct xmltree * soap_call_xt)
{
/*
**              <ns1:GetQuote xmlns:ns1='http://www.webserviceX.NET/'>
**                <ns1:symbol>PG</ns1:symbol>
**              </ns1:GetQuote>
*/
    int wstat = 0;
    struct wsspinfo * wsspi;
    int ii;

    for (ii = 0; !wstat && ii < wssop->wssop_in_nwsspinfo; ii++) {
        wsspi = wssop->wssop_in_awsspinfo[ii];
        wstat = wsdlc_add_parameter(wsg, wssopi, wssop, wsspi,
            parm_jv, soap_call_ns, soap_call_xt);
    }

    if (!wstat) {
        if (ii < wssop->wssop_in_nwsspinfo) {
            wstat = ws_set_error(wsg, WSE_INSUFFICIENTPARMS, "");
        } else if (ii != wssop->wssop_in_nwsspinfo) {
            wstat = ws_set_error(wsg, WSE_TOOMANYPARMS, "");
        }
    }

    return (wstat);
}
/***************************************************************/
int wsdlc_add_soap_body(struct wsglob * wsg,
            struct wssopinfo * wssopi,
            struct wssoperation * wssop,
            struct jsonvalue * parm_jv,
            const char * soap_ns,
            struct xmltree * soap_env_xt)
{
/*
**          <s11:Envelope xmlns:s11='http://schemas.xmlsoap.org/soap/envelope/'>
**            <s11:Body>
**              <ns1:GetQuote xmlns:ns1='http://www.webserviceX.NET/'>
**                <ns1:symbol>PG</ns1:symbol>
**              </ns1:GetQuote>
**            </s11:Body>
**          </s11:Envelope>
*/
    int wstat = 0;
    struct xmltree * soap_body_xt;
    struct xmltree * soap_call_xt;
    char * ns_uri;

    soap_body_xt = xmlns_insert_subel(NULL, soap_ns, SOAP_BODY);
    soap_call_xt = xmlns_insert_subel(soap_body_xt, SOAP_CALL_NS, wssop->wssop_opname);

    /* not sure if this is the correct name space */
    ns_uri = wssop->wssop_wsd_ref->wsd_targetNamespace;

    if (!ns_uri) {
        /* not sure if this is the correct name space either */
        ns_uri = nsl_find_name_space(wssop->wssop_wsd_ref->wsd_nslist, "", 1);
    }

    if (ns_uri) {
        xmlns_insert_attr(soap_call_xt,
                WSSH_TYP_XMLNS, SOAP_CALL_NS, ns_uri);
    } else {
        wstat = ws_set_error(wsg, WSE_FINDNAMESPACE,
                        "%s", "");
    }

    if (!wstat) {
        wstat = wsdlc_add_parms(wsg, wssopi, wssop, parm_jv, SOAP_CALL_NS, soap_call_xt);
        if (wstat) free_xmltree(soap_body_xt);
    }

    if (!wstat) {
        xmlp_insert_subel_tree(soap_env_xt, soap_body_xt, 0);
    }

    return (wstat);
}
/***************************************************************/
int wsdl_call(struct wsglob * wsg,
            struct dynl_info * dynl,
            struct wssopinfo * wssopi,
            struct wssoperation * wssop,
            struct jsontree * parm_jv,
            FILE * outfile)
{
    int wstat = 0;
    struct rcall * rc;
    int env_flags;
    struct xmltree * soap_env_xt;
    struct xmltree * soap_result;
    char * xstr;

    rc = new_rcall(wssop->wssop_wsb_ref->wsb_binding_type);

    switch (rc->rc_binding_type) {
        case WSB_BINDING_TYPE_SOAP:
            env_flags = SOAP_ENVFLAG_SOAP_NS;
            break;
        case WSB_BINDING_TYPE_SOAP12:
            env_flags = SOAP_ENVFLAG_SOAP12_NS;
            break;
        default:
            wstat = ws_set_error(wsg, WSE_UNSUPPORTEDBINDING,
                        "%d", rc->rc_binding_type);
            break;
    }

    if (!wstat) {
        rcall_set_location(rc, wssop->wssop_wsp_ref->wsp_location);

        soap_env_xt = wsdlc_new_soap_envelope(SOAP_NS, env_flags);

        wstat = wsdlc_add_soap_body(wsg, wssopi, wssop, parm_jv->jt_jv, SOAP_NS, soap_env_xt);
        if (wstat) free_xmltree(soap_env_xt);
    }

    if (!wstat) {
        rc->rc_content_xt = soap_env_xt;

        if (rc->rc_location) {
            fprintf(outfile, "Location : %s\n", rc->rc_location);
            fprintf(outfile, "Operation: %s\n", wssopi->wssopi_full_opname);
            xstr = xmltree_to_string(rc->rc_content_xt, XTTS_NL | XTTS_INDENT);
            fprintf(outfile, "Content:\n%s", xstr);
            Free(xstr);

            soap_result = NULL;
            rcall_set_soap_action(rc, wssop->wssop_wsbo_ref->wsbo_soap_soapAction);
            /* rcall_set_host(rc, "www.webservicex.net"); -- Do we need "Host:" ? */
            wstat = rcall_call(wsg, dynl, rc, &soap_result);
        }
    }

    free_rcall(rc);

    return (wstat);
}
/***************************************************************/
int wsdl_call_wssopinfo(struct wsglob * wsg,
            struct dynl_info * dynl,
            struct wssopinfo * wssopi,
            struct wssoperation * wssop,
            const char * line,
            int * linix,
            FILE * outfile)
{
    int wstat = 0;
    int jstat = 0;
    struct json_stream * js;
    struct jsontree * jt;
    int vtyp;
    tWCHAR jc;

    js = new_json_stream_from_chars(line + (*linix));
    vtyp = Json_begin_json(js, JSON_PROLOG_NONE_OK);

    jt = jsonp_new_jsontree();

    if (!Json_err(js) && vtyp) {
        jstat = Json_process_json(vtyp, js, &jt);
    }

    if (Json_err(js)) {
        wstat = ws_set_error(wsg, WSE_BADSERVICEATTR,
                "JSON error %d: %s", Json_err(js), Json_err_msg(js));
    } else {
        jc = Json_get_char(js);
        if (jc != ')') {
            wstat = ws_set_error(wsg, WSE_EXPCLOSEPAREN,
                    "%c", (char)jc);
        } else {
            (*linix) += js->js_fbufix;
            wstat = wsdl_call(wsg, dynl, wssopi, wssop, jt, outfile);
        }
    }

    Json_free_stream(js);
    free_jsontree(jt);

    return (wstat);
}
/***************************************************************/
int wsdl_call_operation(struct wsglob * wsg,
            struct dynl_info * dynl,
            const char * full_opname,
            const char * line,
            int * linix,
            FILE * outfile)
{
    int wstat = 0;
    struct wssopinfo * wssopi;
    void ** vhand;

    wssopi = NULL;
    if (wsg->wsg_wssopl && wsg->wsg_wssopl->wssopl_wssopinfo_dbt) {
        vhand = dbtree_find(wsg->wsg_wssopl->wssopl_wssopinfo_dbt,
                (char*)full_opname,
                IStrlen(full_opname) + 1);
        if (vhand) wssopi = *((struct wssopinfo **)vhand);
    }

    if (!wssopi) { 
        wstat = ws_set_error(wsg, WSE_NOSUCHOPERATION,
                    "%s", full_opname);
    } else {
        if (wssopi->wssopi_nwssoperation != 1) { 
            wstat = ws_set_error(wsg, WSE_OPERATIONNOTUNIQUE,
                        "%s", full_opname);
        } else {
            wstat = wsdl_call_wssopinfo(wsg, dynl, wssopi,
                wssopi->wssopi_awssoperation[0],
                line, linix, outfile);
        }
    }

    return (wstat);
}
/***************************************************************/
/* End WSDL processing functions                               */
/***************************************************************/
