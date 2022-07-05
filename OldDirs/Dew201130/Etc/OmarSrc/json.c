/***************************************************************/
/* json.c                                                      */
/***************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "config.h"
#include "omar.h"
#include "json.h"
#include "snprfh.h"
#include "dbtreeh.h"
#include "util.h"

/***************************************************************/
#if (SHRT_MAX == 32767)

#define JSONCOVFL            254
#define JSONC_SIZE(bytes)   (((bytes)+1) >> 1)
#define JSONC_TO_CHAR(j) ((j)>255?JSONCOVFL:(j))
#define CHAR_TO_JSONC(c) ((unsigned char)(c))
#else
#error No int16 value
#endif

#define JSON_IS_WHITE(jc) ((jc)<128 ? isspace(jc) : 0)
#define JSONC_BYTES(sl) (((sl) + 1) * sizeof(JSCHAR))

#define COPY_JSONC(t,s) if(s)(xmlcncpy((t),(s),sizeof(t) / sizeof(JSCHAR)))

/***************************************************************/

#define JSONERR_NOOPEN          (-2101)
#define JSONERR_EXPOPEN         (-2102)
#define JSONERR_EXPCLOSE        (-2103)
#define JSONERR_EXCOLON         (-2104)
#define JSONERR_EXQUOTE         (-2105)
#define JSONERR_BADESCAPE       (-2106)
#define JSONERR_BADHEXCHAR      (-2107)
#define JSONERR_BADVALUE        (-2108)
#define JSONERR_BADNUMBER       (-2109)
#define JSONERR_EXPCLOSEBRACKET (-2110)

#define JERRMSG_MAX          127

/***************************************************************/

#define JSON_ENC_UTF_8           0  /* default */
#define JSON_ENC_1_BYTE          1  /* Single byte encoding */
#define JSON_ENC_UNICODE_BE      2  /* Unicode big endian */
#define JSON_ENC_UNICODE_LE      3  /* Unicode little endian */

#define JSONCEOF             ((JSCHAR)EOF)

#define JSON_K_TBUF_INC          128
#define JSON_K_LBUF_INC          128
#define JSON_K_DEFAULT_FBUFSIZE  4096
#define JSON_K_DEFAULT_TBUFSIZE  256
#define JSON_K_DEFAULT_LBUFSIZE  256

#define JSON_ERRPARM_SIZE        128

typedef int (*JSON_FREAD)
    (void * vp, void * buffer, int buflen, char * errmsg, int errmsgsz);
typedef int (*JSON_FOPEN)
    (void * vp, const char * fname, const char * fmode,
    char * errmsg, int errmsgsz);
typedef int (*JSON_FCLOSE) (void * vp);

struct jsons_file { /* jf */
    int                       jf_err;
    char                      jf_errparm[JSON_ERRPARM_SIZE];
    char                      jf_errline[JSON_ERRPARM_SIZE];
    int                      jf_errlinenum;

    int                       jf_debug;

    /* element content info  */
    void                     * jf_vfp;
    JSON_FOPEN                 jf_f_fopen;
    JSON_FREAD                 jf_f_fread;
    JSON_FCLOSE                jf_f_fclose;
    int                        jf_errmsgsz;
    char                       jf_errmsg[JERRMSG_MAX + 1];
};

struct json_stream { /* js */
    struct jsons_file *       js_jf;
    int                       js_err;
    char                      js_errparm[JSON_ERRPARM_SIZE];

    /* file buffer */
    unsigned char           * js_fbuf;
    int                      js_fbuflen;
    int                      js_fbufsize;
    int                      js_fbufix;
    int                      js_prev_fbufix; /* for unget_char */
    int                       js_iseof;
    int                       js_tadv;
    int                       js_encoding;
    /* token buffer */
    JSCHAR                  * js_tbuf;
    int                      js_tbuflen;
    int                      js_tbufsize;
    /* line buffer */
    JSCHAR                  * js_lbuf;
    int                      js_lbuflen;
    int                      js_lbufsize;
    int                      js_linenum;
};
struct jsonpfile {  /* jp */
    FILE                *  jp_fref;
};
/***************************************************************/
#ifdef _DEBUG
    #define DEBUG_JSONC 1
#else
    #define DEBUG_JSONC 0
#endif

#define JSON_TYP_SCALAR         19
#define JSON_TYP_MIXED          20

/* struct vtlf {
    int                     vtlt_vtyp;
    union {
        struct vtlayoutobject  * vtlt_vtlo;
        struct vtlayoutarray   * vtlt_vtla;
        void  * vtlt_vlp;
    } vtlt_ltyp;
}; */
/***************************************************************/
#define RESET_BIT(n,b) ((n) & ~(b))

/***************************************************************/
int Json_err(struct json_stream * js) {

    return (js->js_err);
}
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
            Snprintf(ferrmsg, sizeof(ferrmsg),
                "%s\nFound \"%s\" in line #%d:\n%s",
                    errmsg, js->js_errparm,
                    js->js_jf->jf_errlinenum, js->js_jf->jf_errline);
        } else {
            Snprintf(ferrmsg, sizeof(ferrmsg),
                "%s\nFound \"%s\"", errmsg, js->js_errparm);
        }
    } else {
        if (js->js_jf && js->js_jf->jf_errline[0]) {
            Snprintf(ferrmsg, sizeof(ferrmsg),
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

    int len;

    if (!js) return (errnum);

    js->js_err = errnum;
    js->js_errparm[0] = '\0';

    if (js->js_jf) {
        if (js->js_lbuflen >= sizeof(js->js_jf->jf_errline)) {
            len = sizeof(js->js_jf->jf_errline) - 1;
        } else {
            len = js->js_lbuflen;
        }
        memcpy(js->js_jf->jf_errline, js->js_lbuf, (int)len + 1);
        js->js_jf->jf_errline[len] = '\0';
        js->js_jf->jf_errlinenum = js->js_linenum + 1;
    }

    return (errnum);
}
/***************************************************************/
static void Json_fread(struct json_stream * js, int bufix) {

    if (!js->js_jf) {
        js->js_iseof = 1;
    } else {
        if (js->js_jf->jf_f_fread) {
            js->js_fbuflen = js->js_jf->jf_f_fread(js->js_jf->jf_vfp,
                    js->js_fbuf + bufix, js->js_fbufsize - bufix,
                    js->js_jf->jf_errmsg, js->js_jf->jf_errmsgsz);
            if (js->js_fbuflen <= 0) {
                js->js_iseof = 1;
            }
            js->js_fbuflen += bufix;
            js->js_fbufix   = bufix;
        } else {
            js->js_iseof = 1;
        }
    }
}
/***************************************************************/
static void Json_detect_encoding(struct json_stream * js) {
/*
** See: http://en.wikipedia.org/wiki/Byte_Order_Mark
**
** Byte order mark:

    Unicode - Big endian
        js->js_fbuf[0]  0xfe   -2
        js->js_fbuf[1]  0xff   -1
        js->js_fbuf[2]  0x00    0
        js->js_fbuf[3]  0x3c   60
    Unicode - Little endian
        js->js_fbuf[0]  0xff   -1
        js->js_fbuf[1]  0xfe   -2
        js->js_fbuf[2]  0x3c   60
        js->js_fbuf[3]  0x00    0
    UTF-8
        js->js_fbuf[0]  0xef  -17
        js->js_fbuf[1]  0xbb  -69
        js->js_fbuf[2]  0xbf  -65
        js->js_fbuf[3]  0x3c   60
*/

    int rem;

    rem = js->js_fbuflen - js->js_fbufix;

    if (rem < JSONCMAXSZ) {
        if (rem > js->js_fbufix) {
            return;
        }
        if (rem) memcpy(js->js_fbuf, js->js_fbuf + js->js_fbufix, rem);

        Json_fread(js, rem);
        if (js->js_iseof) return;
        js->js_fbufix = 0;

        if (js->js_fbuflen - js->js_fbufix < JSONCMAXSZ) return;
    }

    if (js->js_fbuf[js->js_fbufix] == 0xFE) {
        if (js->js_fbuf[js->js_fbufix + 1] == 0xFF) {
            js->js_encoding = JSON_ENC_UNICODE_BE;
            js->js_fbufix += 2;
        }
    } else if (js->js_fbuf[js->js_fbufix] == 0xFF) {
        if (js->js_fbuf[js->js_fbufix + 1] == 0xFE) {
            js->js_encoding = JSON_ENC_UNICODE_LE;
            js->js_fbufix += 2;
        }
    } else if (js->js_fbuf[js->js_fbufix] == 0xEF) {
        if (js->js_fbuf[js->js_fbufix + 1] == 0xBB) {
            if (js->js_fbuf[js->js_fbufix + 2] == 0xBF) {
                js->js_encoding = JSON_ENC_UTF_8;
                js->js_fbufix += 3;
            }
        }
    }
}
/***************************************************************/
void Json_free_stream(struct json_stream * js) {

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
    js->js_tbuf         = New(JSCHAR, js->js_tbufsize);
    js->js_tbuflen      = 0;
    js->js_tadv         = 0;

    js->js_lbufsize     = JSON_K_DEFAULT_LBUFSIZE;
    js->js_lbuf         = New(JSCHAR, js->js_lbufsize);
    js->js_lbuflen      = 0;
    js->js_linenum      = 0;

    return (js);
}
/***************************************************************/
struct jsons_file * Json_new_jsons_file(JSON_FOPEN  p_xq_fopen,
                           JSON_FREAD  p_xq_fread,
                           JSON_FCLOSE p_xq_fclose,
                           void * vfp,
                           int vFlags) {

    struct jsons_file * jf;

    jf = New(struct jsons_file, 1);

    /* init xmls_file */
    jf->jf_err          = 0;
    jf->jf_errparm[0]   = '\0';
    jf->jf_debug        = 0;

    jf->jf_vfp            = vfp;
    jf->jf_f_fopen        = p_xq_fopen;
    jf->jf_f_fread        = p_xq_fread;
    jf->jf_f_fclose       = p_xq_fclose;

    return (jf);
}
/***************************************************************/
void Json_unget_char(struct json_stream * js) {

    js->js_fbufix = js->js_prev_fbufix;
    js->js_tadv = 0;
    if (js->js_lbuflen) js->js_lbuflen--;
}
/***************************************************************/
JSCHAR Json_get_char(struct json_stream * js) {
/*
    Code range
    hexadecimal      UTF-8
    000000 - 00007F  0xxxxxxx
    000080 - 0007FF  110xxxxx 10xxxxxx
    000800 - 00FFFF  1110xxxx 10xxxxxx 10xxxxxx

*/
    JSCHAR jc;
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
            jc = (JSCHAR)((cb[0] << 8) | cb[1]);
        } else if (js->js_encoding == JSON_ENC_UNICODE_LE) {
            jc = (JSCHAR)((cb[1] << 8) | cb[0]);
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
                jc = (JSCHAR)(((cb[0] & 0x0F) << 12) |
                           ((cb[1] & 0x3F) <<  6) | (cb[2] & 0x3F));
            } else {
                jc = (JSCHAR)(((cb[0] & 0x1F) << 6) | (cb[1] & 0x3F));
            }
        }
    } else {
        jc = (JSCHAR)(cb[0]);
    }

    if (jc == '\n' || jc == '\r') {
        js->js_lbuflen = 0;
        if (jc == '\n') js->js_linenum++;
    } else if (js->js_lbuflen || !JSON_IS_WHITE(jc)) {
        if (js->js_lbuflen == js->js_lbufsize) {
            js->js_lbufsize += JSON_K_LBUF_INC;
            js->js_lbuf = Realloc(js->js_lbuf, JSCHAR, js->js_lbufsize);
        }
        js->js_lbuf[js->js_lbuflen] = jc;
        js->js_lbuflen++;
    }

    return (jc);
}
/***************************************************************/
static JSCHAR Json_peek_char(struct json_stream * js) {

    JSCHAR jc;

    jc = Json_get_char(js);
    Json_unget_char(js);

    return (jc);
}
/***************************************************************/
static JSCHAR Json_get_nwchar(struct json_stream * js) {

    JSCHAR jc;

    do {
        jc = Json_get_char(js);
    } while (JSON_IS_WHITE(jc));
    if (jc == JSONCEOF) return (0);

    return (jc);
}
/***************************************************************/
static JSCHAR Json_peek_nwchar(struct json_stream * js) {

    JSCHAR jc;

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
    js->js_tbuf = Realloc(js->js_tbuf, JSCHAR, js->js_tbufsize);
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

    return (ja);
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
struct jsonvalue * jsonp_new_jsonvalue(int vtyp) {

    struct jsonvalue * jv;

    jv = New(struct jsonvalue, 1);
    jv->jv_vtyp = vtyp;
    jv->jv_val.jv_vp = NULL;

    return (jv);
}
/***************************************************************/
struct jsontree * jsonp_new_jsontree(void) {

    struct jsontree * jt;

    jt = New(struct jsontree, 1);
    jt->jt_jv = NULL;

    return (jt);
}
/***************************************************************/
struct jsonstring * jsonp_new_jsonstring(const JSCHAR * jbbuf, int jblen) {
/* See also dup_jsonstring() */

    struct jsonstring * jb;

    jb = New(struct jsonstring, 1);
    if (jbbuf) {
        jb->jb_bytes = (jblen + 1) * sizeof(JSCHAR);
        jb->jb_buf = New(JSCHAR, jblen + 1);
        memcpy(jb->jb_buf, jbbuf, jb->jb_bytes);
    }

    return (jb);
}
/***************************************************************/
struct jsonnumber * jsonp_new_jsonnumber(const JSCHAR * jnbuf, int jnlen) {

    struct jsonnumber * jn;

    jn = New(struct jsonnumber, 1);
    jn->jn_bytes = (jnlen + 1) * sizeof(JSCHAR);
    jn->jn_buf = New(JSCHAR, jn->jn_bytes);
    memcpy(jn->jn_buf, jnbuf, jn->jn_bytes);

    return (jn);
}
/***************************************************************/
static struct jsonnumber * Json_get_number(JSCHAR jc,
                        struct json_stream * js) {
/*
** See also: validate_string_json_number()
*/

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
static int jsonp_number(JSCHAR jc,
                       struct json_stream * js,
                           struct jsonnumber ** pjn) {


    int result;
    JSCHAR pc;

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
static JSCHAR * Json_next_token_alpha(JSCHAR jc, struct json_stream * js) {

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
static int jsonp_true(JSCHAR jc,
                       struct json_stream * js) {


    int result;
    JSCHAR * jtok;

    result = 0;
    jtok = Json_next_token_alpha(jc, js);
    if (JSONSTRCMP(jtok, JSON_VALUE_TRUE)) {
        result = Json_seterr(js, JSONERR_BADVALUE);
    }

    return (result);
}
/***************************************************************/
static int jsonp_false(JSCHAR jc,
                       struct json_stream * js) {


    int result;
    JSCHAR * jtok;

    result = 0;
    jtok = Json_next_token_alpha(jc, js);
    if (JSONSTRCMP(jtok, JSON_VALUE_FALSE)) {
        result = Json_seterr(js, JSONERR_BADVALUE);
    }

    return (result);
}
/***************************************************************/
static int jsonp_null(JSCHAR jc,
                       struct json_stream * js) {


    int result;
    JSCHAR * jtok;

    result = 0;
    jtok = Json_next_token_alpha(jc, js);
    if (JSONSTRCMP(jtok, JSON_VALUE_NULL)) {
        result = Json_seterr(js, JSONERR_BADVALUE);
    }

    return (result);
}
/***************************************************************/
static int jsonp_value( struct json_stream * js,
                        struct jsonvalue ** pjv);

static int jsonp_array( struct json_stream * js,
                        struct jsonarray ** pja) {

    /* assumes [ already from Json_get_char() */

    int result;
    JSCHAR pc;
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
#define VALID_HEX_CHAR(c) \
 ((((c)>='0'&&(c)<='9')||((c)>='A'&&(c)<='F')||((c)>='a'&&(c)<='f'))?1:0)
static int cvt_hex4(JSCHAR ujc[], JSCHAR * pjc) {


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
    JSCHAR jc;
    JSCHAR ujc[4];

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
int jsonp_insert_object_value(struct jsonobject * jo,
                        struct jsonstring * jb,
                        struct jsonvalue * jv) {
    int result;

    result = dbtree_insert(jo->jo_dbt, jb->jb_buf, jb->jb_bytes, jv);

    return (result);
}
/***************************************************************/
void jsonp_insert_object_value_with_dup_conversion(struct jsonobject * jo,
                        struct jsonstring * jb,
                        struct jsonvalue * jv) {
    int result;
    void * pjojv;
    struct jsonvalue * jojv;
    struct jsonvalue * n_jv;
    struct jsonarray * ja;

    pjojv = dbtree_find(jo->jo_dbt, jb->jb_buf, jb->jb_bytes);
    if (!pjojv) {
        result = jsonp_insert_object_value(jo, jb, jv);
    } else {
        jojv = *((struct jsonvalue **)pjojv);
        if (jojv->jv_vtyp == VTAB_VAL_ARRAY) {
            ja = jojv->jv_val.jv_ja;
        } else {
            ja = jsonp_new_jsonarray();
            n_jv = jsonp_new_jsonvalue(VTAB_VAL_ARRAY);
            jsonp_append_array_value(ja, jojv);

            n_jv->jv_val.jv_ja = ja;
            *((struct jsonvalue **)pjojv) = n_jv;
        }
        jsonp_append_array_value(ja, jv);
    }
}
/***************************************************************/
static int jsonp_object(struct json_stream * js,
                        struct jsonobject ** pjo) {

    /* assumes { already from Json_get_char() */

    int result;
    JSCHAR pc;
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
    JSCHAR pc;
    int done;
    JSCHAR *  jtok;

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
        if (!JSONSTRCMP(jtok, JSON_VALUE_TRUE)) {
            (*pjv) = jsonp_new_jsonvalue(VTAB_VAL_TRUE);
        } else {
            result = Json_seterr(js, JSONERR_BADVALUE);
        }
    } else if (pc == 'f') {
        jtok = Json_next_token_alpha(pc, js);
        if (!JSONSTRCMP(jtok, JSON_VALUE_FALSE)) {
            (*pjv) = jsonp_new_jsonvalue(VTAB_VAL_FALSE);
        } else {
            result = Json_seterr(js, JSONERR_BADVALUE);
        }
    } else if (pc == 'n') {
        jtok = Json_next_token_alpha(pc, js);
        if (!JSONSTRCMP(jtok, JSON_VALUE_NULL)) {
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
    JSCHAR pc;

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
int Json_process_json(int vtyp,
                        struct json_stream * js,
                        struct jsontree ** pjt) {

    int result;

    result = jsonp_first_value(vtyp, js, &((*pjt)->jt_jv));

    return (result);
}
/***************************************************************/
static int jsonpp_prolog(struct json_stream * js, int prolog_flags) {
/*
**  Find and start with {
*/
    JSCHAR pc;
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
        } else if (!(prolog_flags & 1)) {
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
int Json_begin_json(struct json_stream * js, int prolog_flags) {
/*
*/
    int vtyp;

    vtyp = jsonpp_prolog(js, prolog_flags);

    return (vtyp);
}
/***************************************************************/
static int jsonp_fopen (void *vfp,
                const char * fname,
                const char * fmode,
                     char * errmsg,
                     int    errmsgsz) {
/*
**  Read record from text file
**  Returns number of bytes in record including lf if successful
**  Returns 0 if end of file
**  Returns -1 if error
*/
    struct jsonpfile * jp = vfp;
    FILE * fref;
    int ern;

    errmsg[0] = '\0';

    fref = fopen(fname, fmode);
    if (!fref) {
        ern = ERRNO;
        strmcpy(errmsg, strerror(ern), errmsgsz);
        return (JSONE_FOPEN);
    }
    jp->jp_fref = fref;

    return (0);
}
/***************************************************************/
static int jsonp_fread (void *vfp,
                            void * rec,
                             int recsize,
                             char * errmsg,
                             int    errmsgsz) {
/*
**  Read record from text file
**  Returns number of bytes in record including lf if successful
**  Returns 0 if end of file
**  Returns -1 if error
*/
    struct jsonpfile * jp = vfp;
    int len;
    int ern;

    errmsg[0] = '\0';

    len = (int)fread(rec, 1, recsize, jp->jp_fref);
    if (!len) {
        if (feof(jp->jp_fref)) {
            len = 0;
        } else {
            ern = ERRNO;
            strmcpy(errmsg, strerror(ern), errmsgsz);
            len = JSONE_FREAD;
        }
    }

    return (len);
}
/***************************************************************/
static int jsonp_fclose(void *vfp) {
/*
**  Read record from text file
**  Returns number of bytes in record including lf if successful
**  Returns 0 if end of file
**  Returns -1 if error
*/
    struct jsonpfile * jp = vfp;

    fclose(jp->jp_fref);

    return (0);
}
/***************************************************************/
void free_jsonpfile(struct jsonpfile * jp) {

    Free(jp);
}
/***************************************************************/
void free_jsons_file(struct jsons_file * jf) {

    Free(jf);
}
/***************************************************************/
struct jsonpfile * jsonp_new_jsonpfile(void) {

    struct jsonpfile * jp;

    jp = New(struct jsonpfile, 1);

    return (jp);
}
/***************************************************************/
int jsonp_get_jsontree(const char * jsonfname,
                     struct jsontree ** pjt,
                     char * errmsg,
                     int    errmsgsz) {

    struct jsonpfile * jp;
    int estat;
    int vtyp;
    struct jsons_file    * jf;
    struct json_stream      * js;

    errmsg[0] = '\0';
    js = NULL;
    vtyp = 0;

    jp = jsonp_new_jsonpfile();

    estat = jsonp_fopen(jp, jsonfname, "r", errmsg, errmsgsz);
    if (estat) {
        Free(jp);
        return (estat);
    }

    jf = Json_new_jsons_file(jsonp_fopen,
                       jsonp_fread,
                       jsonp_fclose,
                       jp, 0);

    js = Json_new_json_stream(jf);
    Json_detect_encoding(js);
    if (!Json_err(js)) vtyp = Json_begin_json(js, 1);

    if (Json_err(js)) {
        strmcpy(errmsg, Json_err_msg(js), errmsgsz);
        jsonp_fclose(jf);
        Free(jp);
        Json_free_stream(js);
        free_jsons_file(jf);
        return (JSONE_FNEW);
    }

    (*pjt) = jsonp_new_jsontree();

    estat = Json_process_json(vtyp, js, pjt);

    if (Json_err(js)) {
        strmcpy(errmsg, Json_err_msg(js), errmsgsz);
        estat = JSONE_PROCESS;
    } else {
        estat = 0;
    }

    jsonp_fclose(jp);
    free_jsonpfile(jp);
    Json_free_stream(js);
    free_jsons_file(jf);

    return (estat);
}
/***************************************************************/
/* JSON functions                                              */
/***************************************************************/
void free_jsonvalue(void * vjv);

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
int print_jsonindent_value(void)
{
    return (4);
}
/***************************************************************/
int print_jsonindent_line_width(void)
{
    return (79);
}
/***************************************************************/
int print_json_spaces(FILE * outfile, int nspaces)
{
    int estat = 0;
    char ibuf[100];

    if (nspaces > 0) {
        if (nspaces >= sizeof(ibuf)) nspaces = sizeof(ibuf) - 1;

        memset(ibuf, ' ', nspaces);
        ibuf[nspaces] = '\0';
        estat = Fmprintf(outfile, "%s", ibuf);
    }

    return (estat);
}
/***************************************************************/
int print_jsonindent(FILE * outfile, int pjtflags, int jtdepth)
{
    int estat = 0;
    int nspaces;

    if (pjtflags & PJT_FLAG_INDENT) {
        nspaces = jtdepth * print_jsonindent_value();
        estat = print_json_spaces(outfile, nspaces);
    }

    return (estat);
}
/***************************************************************/
int print_jsonnumber(FILE * outfile, struct jsonnumber * jn, int pjtflags, int jtdepth) {

    int estat = 0;

    if (jn) {
        estat = Fmprintf(outfile, "%s", jn->jn_buf);
        if (estat < 0) return (estat);
    }

    return (0);
}
/***************************************************************/
int print_jsonstring(FILE * outfile, struct jsonstring * jb, int pjtflags, int jtdepth) {

    int estat = 0;

    if (jb) {
        estat = Fmprintf(outfile, "\"%s\"", jb->jb_buf);
        if (estat < 0) return (estat);
    }

    return (0);
}
/***************************************************************/
int print_jsonvalue_width(struct jsonvalue * jv) {

    int width;

    if (!jv) return (0);

    width = 0;
    switch (jv->jv_vtyp) {
        case VTAB_VAL_OBJECT:
            width = 1;
            break;
        case VTAB_VAL_ARRAY:
            if (jv->jv_val.jv_vp) {
                width = jv->jv_val.jv_ja->ja_nvals * 4 + 3;
            }
            break;
        case VTAB_VAL_STRING:
            if (jv->jv_val.jv_vp) {
                width = jv->jv_val.jv_jb->jb_bytes + 1;
            }
            break;

        case VTAB_VAL_NUMBER:
            if (jv->jv_val.jv_vp) {
                width = jv->jv_val.jv_jn->jn_bytes - 1;
            }
            break;
        case VTAB_VAL_TRUE:
            width = 4;
            break;
        case VTAB_VAL_FALSE:
            width = 5;
            break;
        case VTAB_VAL_NULL:
            width = 4;
            break;
        default:
            break;
    }

    return (width);
}
/***************************************************************/
int print_jsonarray_mxwidth(struct jsonarray * ja) {

    int mxwidth;
    int width;
    int ii;

    mxwidth = 0;
    for (ii = 0; ii < ja->ja_nvals; ii++) {
        width = print_jsonvalue_width(ja->ja_vals[ii]);
        if (width > mxwidth) mxwidth = width;
    }

    return (mxwidth);
}
/***************************************************************/
int print_jsonarray(FILE * outfile, struct jsonarray * ja, int pjtflags, int jtdepth) {

    int estat = 0;
    int ii;
    int pindent;
    int mxwidth;
    int perline;
    int currline;
    int width;

    if (ja) {
        estat = Fmprintf(outfile, "[ ");
        if (estat < 0) return (estat);

        if (pjtflags & PJT_FLAG_PRETTY) {
            estat = Fmprintf(outfile, "\n");
            estat = print_jsonindent(outfile, pjtflags, jtdepth + 1);
            pindent = (jtdepth + 1) * print_jsonindent_value();
            mxwidth = print_jsonarray_mxwidth(ja);
            perline = (print_jsonindent_line_width() - pindent) / (mxwidth + 2);
            if (perline < 1) perline = 1;
            currline = 0;
        };

        for (ii = 0; ii < ja->ja_nvals; ii++) {
            if (ii) {
                estat = Fmprintf(outfile, ", ");
                if ((pjtflags & PJT_FLAG_PRETTY) &&
                    currline == perline) {
                    estat = Fmprintf(outfile, "\n");
                    estat = print_jsonindent(outfile, pjtflags, jtdepth + 1);
                    currline = 0;
                }
            }
            if (estat < 0) return (estat);
            estat = print_jsonvalue(outfile, ja->ja_vals[ii], pjtflags, jtdepth);
            if (estat < 0) return (estat);
            currline++;

            if ((pjtflags & PJT_FLAG_PRETTY) &&
                (ja->ja_vals[ii]->jv_vtyp == VTAB_VAL_STRING ||
                 ja->ja_vals[ii]->jv_vtyp == VTAB_VAL_NUMBER)) {
                width = print_jsonvalue_width(ja->ja_vals[ii]);
                estat = print_json_spaces(outfile, mxwidth - width);
            };
        }
        estat = Fmprintf(outfile, " ]");
        if (estat < 0) return (estat);
    }

    return (0);
}
/***************************************************************/
int print_jsonobject(FILE * outfile, struct jsonobject * jo, int pjtflags, int jtdepth) {

    int estat = 0;
    void * vdbtc;
    void * pjv;
    struct jsonvalue * jv;
    JSCHAR *   jbbuf;
    int      jblen;
    int nfields;
    int indentsp = 4;

    if (jo) {
        estat = Fmprintf(outfile, "{\n");
        if (estat < 0) return (estat);
        nfields = 0;
        vdbtc = dbtree_rewind(jo->jo_dbt, DBTREE_READ_FORWARD);
        do {
            pjv = dbtree_next(vdbtc);
            if (pjv) {
                if (nfields) {
                    estat = Fmprintf(outfile, ",");
                    if (pjtflags & PJT_FLAG_LF) estat = Fmprintf(outfile, "\n");
                }
                estat = print_jsonindent(outfile, pjtflags, jtdepth + 1);
                if (estat < 0) return (estat);
                jv = *((struct jsonvalue **)pjv);
                jbbuf = dbtree_get_key(vdbtc, &jblen);
                estat = Fmprintf(outfile, "\"%s\" : ", jbbuf);
                if (estat < 0) return (estat);
                estat = print_jsonvalue(outfile, jv, pjtflags, jtdepth + 1);
                if (estat < 0) return (estat);
                nfields++;
            }
        } while (pjv);
        dbtree_close(vdbtc);

        if (nfields && (pjtflags & PJT_FLAG_LF)) estat = Fmprintf(outfile, "\n");
        if (estat < 0) return (estat);

        estat = print_jsonindent(outfile, pjtflags, jtdepth);
        if (estat < 0) return (estat);

        estat = Fmprintf(outfile, "}");
        if (estat < 0) return (estat);
    }

    return (0);
}
/***************************************************************/
int print_jsonvalue(FILE * outfile, struct jsonvalue * jv, int pjtflags, int jtdepth) {

    int estat = 0;

    if (jv) {
        switch (jv->jv_vtyp) {
            case VTAB_VAL_OBJECT:
                estat = print_jsonobject(outfile, jv->jv_val.jv_jo, pjtflags, jtdepth);
                if (estat < 0) return (estat);
                break;
            case VTAB_VAL_ARRAY:
                estat = print_jsonarray(outfile, jv->jv_val.jv_ja, pjtflags, jtdepth);
                if (estat < 0) return (estat);
                break;
            case VTAB_VAL_STRING:
                estat = print_jsonstring(outfile, jv->jv_val.jv_jb, pjtflags, jtdepth);
                if (estat < 0) return (estat);
                break;

            case VTAB_VAL_NUMBER:
                estat = print_jsonnumber(outfile, jv->jv_val.jv_jn, pjtflags, jtdepth);
                if (estat < 0) return (estat);
                break;
            case VTAB_VAL_TRUE:
                estat = Fmprintf(outfile, "%s", "TRUE");
                if (estat < 0) return (estat);
                break;
            case VTAB_VAL_FALSE:
                estat = Fmprintf(outfile, "%s", "FALSE");
                if (estat < 0) return (estat);
                break;
            case VTAB_VAL_NULL:
                estat = Fmprintf(outfile, "%s", "NULL");
                if (estat < 0) return (estat);
                break;
            default:
                estat = Fmprintf(outfile, "[jv_vtyp=%d]", jv->jv_vtyp);
                if (estat < 0) return (estat);
                break;
        }
    }

    return (0);
}
/***************************************************************/
int print_jsontree(FILE * outfile, struct jsontree * jt, int pjtflags) {

    int estat = 0;

    if (jt) {
        estat = print_jsonvalue(outfile, jt->jt_jv, pjtflags, 0);
        if (estat < 0) return (estat);
        estat = Fmprintf(outfile, "\n");
        if (estat < 0) return (estat);
    }

    return (0);
}
/***************************************************************/
/* json utilitiy functions                                     */
/***************************************************************/
struct jsonobject * jsonobject_get_jsonobject(struct jsonobject * jo, const char * fldname)
{
    struct jsonobject * rtn_jo;
    void * pjojv;
    struct jsonvalue * jojv;

    rtn_jo = NULL;

    pjojv = dbtree_find(jo->jo_dbt, fldname, IStrlen(fldname) + 1);
    if (pjojv) {
        jojv = *((struct jsonvalue **)pjojv);
        if (jojv->jv_vtyp == VTAB_VAL_OBJECT) {
            rtn_jo = jojv->jv_val.jv_jo;
        }
    }

    return (rtn_jo);
}
/***************************************************************/
struct jsonarray * jsonobject_get_jsonarray(struct jsonobject * jo, const char * fldname)
{
    struct jsonarray * rtn_ja;
    void * pjojv;
    struct jsonvalue * jojv;

    rtn_ja = NULL;

    pjojv = dbtree_find(jo->jo_dbt, fldname, IStrlen(fldname) + 1);
    if (pjojv) {
        jojv = *((struct jsonvalue **)pjojv);
        if (jojv->jv_vtyp == VTAB_VAL_ARRAY) {
            rtn_ja = jojv->jv_val.jv_ja;
        }
    }

    return (rtn_ja);
}
/***************************************************************/
struct jsonstring * jsonobject_get_jsonstring(struct jsonobject * jo, const char * fldname)
{
    struct jsonstring * rtn_jb;
    void * pjojv;
    struct jsonvalue * jojv;

    rtn_jb = NULL;

    pjojv = dbtree_find(jo->jo_dbt, fldname, IStrlen(fldname) + 1);
    if (pjojv) {
        jojv = *((struct jsonvalue **)pjojv);
        if (jojv->jv_vtyp == VTAB_VAL_STRING) {
            rtn_jb = jojv->jv_val.jv_jb;
        }
    }

    return (rtn_jb);
}
/***************************************************************/
int jsonnumber_to_integer(struct jsonnumber * jn, int * num)
{
    int nerr = 0;

    nerr = char_to_integer(jn->jn_buf, num);

    return (nerr);
}
/***************************************************************/
int jsonnumber_to_double(struct jsonnumber * jn, double * dval)
{
    int nerr = 0;

    nerr = string_to_double(jn->jn_buf, dval);

    return (nerr);
}
/***************************************************************/
int jsonobject_get_integer(struct jsonobject * jo, const char * fldname, int * num)
{
    struct jsonnumber * jn;
    void * pjn;
    struct jsonvalue * njv;
    int nerr = 0;

    jn = NULL;

    pjn = dbtree_find(jo->jo_dbt, fldname, IStrlen(fldname) + 1);
    if (pjn) {
        njv = *((struct jsonvalue **)pjn);
        if (njv->jv_vtyp == VTAB_VAL_NUMBER) {
            jn = njv->jv_val.jv_jn;
        }
    }

    if (!jn) {
        nerr = -9;
    } else {
        nerr = jsonnumber_to_integer(jn, num);
    }

    return (nerr);
}
/***************************************************************/
int jsonobject_get_double(struct jsonobject * jo, const char * fldname, double * dval)
{
    struct jsonnumber * jn;
    void * pjn;
    struct jsonvalue * njv;
    int nerr = 0;

    jn = NULL;

    pjn = dbtree_find(jo->jo_dbt, fldname, IStrlen(fldname) + 1);
    if (pjn) {
        njv = *((struct jsonvalue **)pjn);
        if (njv->jv_vtyp == VTAB_VAL_NUMBER) {
            jn = njv->jv_val.jv_jn;
        }
    }

    if (!jn) {
        nerr = -9;
    } else {
        nerr = jsonnumber_to_double(jn, dval);
    }

    return (nerr);
}
/***************************************************************/
char * jsonobject_get_string(struct jsonobject * jo, const char * fldname)
{
    struct jsonstring * jb;
    void * pjb;
    struct jsonvalue * njv;
    char * out;

    jb = NULL;

    pjb = dbtree_find(jo->jo_dbt, fldname, IStrlen(fldname) + 1);
    if (pjb) {
        njv = *((struct jsonvalue **)pjb);
        if (njv->jv_vtyp == VTAB_VAL_STRING) {
            jb = njv->jv_val.jv_jb;
        }
    }

    if (!jb) {
        out = NULL;
    } else {
        out = New(char, jb->jb_bytes);
        memcpy(out, jb->jb_buf, jb->jb_bytes);
    }

    return (out);
}
/***************************************************************/
char * jsonarray_get_string(struct jsonarray * ja, int jaindex)
{
    struct jsonstring * jb;
    struct jsonvalue * jajv;
    char * out;

    jb = NULL;

    if (jaindex < ja->ja_nvals) {
        jajv = ja->ja_vals[jaindex];
        if (jajv->jv_vtyp == VTAB_VAL_STRING) {
            jb = jajv->jv_val.jv_jb;
        }
    }

    if (!jb) {
        out = NULL;
    } else {
        out = New(char, jb->jb_bytes);
        memcpy(out, jb->jb_buf, jb->jb_bytes);
    }

    return (out);
}
/***************************************************************/
struct jsonstring * jsonarray_get_jsonstring(struct jsonarray * ja, int jaindex)
{
    struct jsonstring * rtn_jb;
    struct jsonvalue * jajv;

    rtn_jb = NULL;

    if (jaindex < ja->ja_nvals) {
        jajv = ja->ja_vals[jaindex];
        if (jajv->jv_vtyp == VTAB_VAL_STRING) {
            rtn_jb = jajv->jv_val.jv_jb;
        }
    }

    return (rtn_jb);
}
/***************************************************************/
struct jsonnumber * jsonarray_get_jsonnumber(struct jsonarray * ja, int jaindex)
{
    struct jsonnumber * rtn_jn;
    struct jsonvalue * jajv;

    rtn_jn = NULL;

    if (jaindex < ja->ja_nvals) {
        jajv = ja->ja_vals[jaindex];
        if (jajv->jv_vtyp == VTAB_VAL_NUMBER) {
            rtn_jn = jajv->jv_val.jv_jn;
        }
    }

    return (rtn_jn);
}
/***************************************************************/
int jsonarray_get_integer(struct jsonarray * ja, int jaindex, int * num)
{
    struct jsonnumber * jn;
    struct jsonvalue * jajv;
    int nerr = 0;

    jn = NULL;

    if (jaindex < ja->ja_nvals) {
        jajv = ja->ja_vals[jaindex];
        if (jajv->jv_vtyp == VTAB_VAL_NUMBER) {
            jn = jajv->jv_val.jv_jn;
        }
    }

    if (!jn) {
        nerr = -9;
    } else {
        nerr = jsonnumber_to_integer(jn, num);
    }

    return (nerr);
}
/***************************************************************/
int jsonarray_get_double(struct jsonarray * ja, int jaindex, double * dval)
{
    struct jsonnumber * jn;
    struct jsonvalue * jajv;
    int nerr = 0;

    jn = NULL;

    if (jaindex < ja->ja_nvals) {
        jajv = ja->ja_vals[jaindex];
        if (jajv->jv_vtyp == VTAB_VAL_NUMBER) {
            jn = jajv->jv_val.jv_jn;
        }
    }

    if (!jn) {
        nerr = -9;
    } else {
        nerr = jsonnumber_to_double(jn, dval);
    }

    return (nerr);
}
/***************************************************************/
struct jsonarray * jsonarray_get_jsonarray(struct jsonarray * ja, int jaindex)
{
    struct jsonarray * rtn_ja;
    struct jsonvalue * jajv;

    rtn_ja = NULL;

    if (jaindex < ja->ja_nvals) {
        jajv = ja->ja_vals[jaindex];
        if (jajv->jv_vtyp == VTAB_VAL_ARRAY) {
            rtn_ja = jajv->jv_val.jv_ja;
        }
    }

    return (rtn_ja);
}
/***************************************************************/
struct jsonobject * jsonarray_get_jsonobject(struct jsonarray * ja, int jaindex)
{
    struct jsonobject * rtn_jo;
    struct jsonvalue * jajv;

    rtn_jo = NULL;

    if (jaindex < ja->ja_nvals) {
        jajv = ja->ja_vals[jaindex];
        if (jajv->jv_vtyp == VTAB_VAL_OBJECT) {
            rtn_jo = jajv->jv_val.jv_jo;
        }
    }

    return (rtn_jo);
}
/***************************************************************/
int jsonobject_decode_base64(struct jsonobject * jo, const char * b64field)
{
    int cvtstat = 0;
    int maxlen;
    struct jsonstring * jb;
    char * bbuf;
    int bbuflen;

    jb = jsonobject_get_jsonstring(jo, b64field);
    if (!jb) {
        cvtstat = -1;
    } else {
        maxlen = FROM_B64_LEN(jb->jb_bytes);
        bbuf = New(char, maxlen);
        bbuflen = from_base64_c(bbuf, jb->jb_buf, jb->jb_bytes - 1, 0);
        memcpy(jb->jb_buf, bbuf, bbuflen);
        jb->jb_buf[bbuflen] = '\0';
        jb->jb_bytes = bbuflen + 1;
        Free(bbuf);
    }

    return (cvtstat);
}
/***************************************************************/
void jsonobject_put_int(struct jsonobject * jo, const char * fldnam, int ival)
{
    struct jsonvalue * jv;
    struct jsonnumber * jn;
    struct jsonstring * jb;
    int rtn;
    char nbuf[16];

    sprintf(nbuf, "%d", ival);
    jn = jsonp_new_jsonnumber(nbuf, IStrlen(nbuf) + 1);
    jv = jsonp_new_jsonvalue(VTAB_VAL_NUMBER);
    jv->jv_val.jv_jn = jn;

    jb = jsonp_new_jsonstring(fldnam, IStrlen(fldnam) + 1);

    rtn = jsonp_insert_object_value(jo, jb, jv);

    free_jsonstring(jb);
}
/***************************************************************/
void jsonobject_put_jv(struct jsonobject * jo, const char * fldnam, struct jsonvalue * jv)
{
    struct jsonstring * jb;
    int rtn;

    jb = jsonp_new_jsonstring(fldnam, IStrlen(fldnam) + 1);

    rtn = jsonp_insert_object_value(jo, jb, jv);

    free_jsonstring(jb);
}
/***************************************************************/
void jsonobject_put_double(struct jsonobject * jo, const char * fldnam, double dval)
{
    struct jsonvalue * jv;
    struct jsonnumber * jn;
    struct jsonstring * jb;
    int rtn;
    char nbuf[40];

    double_to_string(nbuf, dval, sizeof(nbuf));
    jn = jsonp_new_jsonnumber(nbuf, IStrlen(nbuf) + 1);
    jv = jsonp_new_jsonvalue(VTAB_VAL_NUMBER);
    jv->jv_val.jv_jn = jn;

    jb = jsonp_new_jsonstring(fldnam, IStrlen(fldnam) + 1);

    rtn = jsonp_insert_object_value(jo, jb, jv);

    free_jsonstring(jb);
}
/***************************************************************/
void jsonobject_put_string(struct jsonobject * jo, const char * fldnam, const char * strval)
{
    struct jsonvalue * jv;
    struct jsonstring * jb_val;
    struct jsonstring * jb_nam;
    int rtn;

    jb_val = jsonp_new_jsonstring(strval, IStrlen(strval) + 1);
    jv = jsonp_new_jsonvalue(VTAB_VAL_STRING);
    jv->jv_val.jv_jb = jb_val;

    jb_nam = jsonp_new_jsonstring(fldnam, IStrlen(fldnam) + 1);

    rtn = jsonp_insert_object_value(jo, jb_nam, jv);

    free_jsonstring(jb_nam);
}
/***************************************************************/

/***************************************************************/
void jsonarray_put_int(struct jsonarray * ja, int ival);
/***************************************************************/
void jsonarray_put_double(struct jsonarray * ja, double dval)
{
    struct jsonvalue * jv;
    struct jsonnumber * jn;
    int rtn;
    char nbuf[40];

    double_to_string(nbuf, dval, sizeof(nbuf));
    jn = jsonp_new_jsonnumber(nbuf, IStrlen(nbuf) + 1);
    jv = jsonp_new_jsonvalue(VTAB_VAL_NUMBER);
    jv->jv_val.jv_jn = jn;

    rtn = jsonp_append_array_value(ja, jv);
}
/***************************************************************/
void jsonarray_put_str(struct jsonarray * ja, const char * strval);
/***************************************************************/
void jsonarray_put_jv(struct jsonarray * ja, struct jsonvalue * jv)
{
    int rtn;

    rtn = jsonp_append_array_value(ja, jv);
}
/***************************************************************/

