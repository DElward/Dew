/*
**  XMLC -- XML parser functions
*/
/***************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>

#include "HttpGet.h"
#include "dbtreeh.h"
#include "snprfh.h"
#include "xmlh.h"

#define ERRNO       errno

/***************************************************************/
/* BEGIN Structures for Xml_ functions                         */
/* #include <limits.h> */

#define XML_K_DEFAULT_FBUFSIZE  4096    /* Must be at least XMLCMAXSZ * 2 */
#define XML_K_DEFAULT_TBUFSIZE  256
#define XML_K_FBUF_INC          128
#define XML_K_TBUF_INC          128
#define XML_MAX_ENTITY_RECUR    100
#define XML_K_DEFAULT_EBUFSIZE  32
#define XML_K_EBUF_INC          32
#define XML_K_CBUF_INC          128
#define XML_K_DEFAULT_LBUFSIZE  256
#define XML_K_LBUF_INC          128

#define XML_ENC_UTF_8           0  /* default */
#define XML_ENC_1_BYTE          1  /* Single byte encoding */
#define XML_ENC_UNICODE_BE      2  /* Unicode big endian */
#define XML_ENC_UNICODE_LE      3  /* Unicode little endian */

#define XML_IMPLY_ITEMS         1000 /* items to use for implied DTD */
/***************************************************************/
#define ENTYP_SYS       0
#define ENTYP_GENERAL   1
#define ENTYP_PARM      2

struct xt_entity {
    XMLC *  e_name;
    XMLC *  e_val;
    struct xt_extID * e_extID;
    XMLC *  e_ndata;
    int     e_typ;  /* 0 = system, 1 = regular, 2 = parameter */
    int     e_len;
    int     e_ix;   /* 1 if e_val is quoted */
};
/***************************************************************/
struct xt_entities {
    struct dbtree * xe_entity;
};
/***************************************************************/
struct xt_eltree {

    XMLC * lt_op;
    XMLC lt_repeat;
    int lt_pcdata;
    int lt_nnodes;
    struct xt_eltree ** lt_node;
};
/***************************************************************/
#define REPEAT_ONE          0
#define REPEAT_ZERO_OR_ONE  '?'
#define REPEAT_ONE_OR_MORE  '+'
#define REPEAT_ZERO_OR_MORE '*'

struct xt_imp_element {
    XMLC *              m_elName;
    XMLC                m_repeat;
    int                 m_accessed;
};
/***************************************************************/
struct xt_i_elements {
    int                 i_pcdata;
    struct dbtree *     i_ielements; /* struct xt_imp_element */
};
/***************************************************************/
#define ELTYP_ANY       1
#define ELTYP_EMPTY     2
#define ELTYP_TREE      3
#define ELTYP_IMPLICIT  4

struct xt_element {
    XMLC *  l_name;
    int     l_typ;  /* 1 = ANY, 2 = EMPTY, 3 = tree */
    int     l_preserve_space;
    struct xt_attlist * l_attlist;
    struct xt_eltree  * l_tree;
    struct xt_i_elements * l_i_elements;
};
/***************************************************************/
struct xt_elements {
    struct dbtree * xl_elements; /* struct xt_element */
};
/***************************************************************/
#define EXTYP_SYSTEM    1
#define EXTYP_PUBLIC    2
struct xt_extID {
    int    x_typ;
    XMLC * x_extID;
    XMLC * x_pubID;
};
/***************************************************************/
struct xt_extFile {
    void * fref;
};
/***************************************************************/
#define ATTTYP_CDATA        1
#define ATTTYP_ENUM         2
#define ATTTYP_NOTATION     3
#define ATTTYP_ID           4
#define ATTTYP_IDREF        5
#define ATTTYP_IDREFS       6
#define ATTTYP_ENTITY       7
#define ATTTYP_ENTITIES     8
#define ATTTYP_NMTOKEN      9
#define ATTTYP_NMTOKENS     10

#define ATTDEFTYP_REQUIRED  1
#define ATTDEFTYP_IMPLIED   2
#define ATTDEFTYP_FIXED     3
#define ATTDEFTYP_VAL       4
/***************************************************************/
struct xt_eitem {
    XMLC *  e_name;
};

struct xt_attdef {
    XMLC *  a_attname;
    int     a_atttyp;
    int     a_defaulttyp;
    int     a_ref_flag;
    XMLC *  a_defaultval;
    struct dbtree * a_elist;  /* tree struct xt_eitem *'s */
};
/***************************************************************/
struct xt_attlist {

    XMLC *  a_elname;
    struct dbtree * a_attdefs;
};
/***************************************************************/
struct xt_attlists {
    struct dbtree * xa_attlist;
};
/***************************************************************/
#define NOTTYP_SYSTEM   1
#define NOTTYP_PUBLIC   2

struct xt_notation {
    XMLC *  n_notname;
    struct xt_extID * n_extID;
};
/***************************************************************/
struct xt_notations {
    struct dbtree * xn_notations;
};
/***************************************************************/
struct xt_elnode {
    XMLC                    * xw_elNam;
    int                       xw_nnodes;
    struct xt_elnode       ** xw_nodes;
};
/***************************************************************/
struct xml_stream {
#ifdef _DEBUG
    char    aatok[128];
#endif
    struct xmls_file *        xf_xf;
    struct xt_extFile *       xf_extFile;

    /* file buffer */
    unsigned char           * xf_fbuf;
    long                      xf_fbuflen;
    long                      xf_fbufsize;
    long                      xf_fbufix;
    long                      xf_prev_fbufix; /* for unget_char */
    int                       xf_iseof;
    int                       xf_tadv;
    int                       xf_encoding;
    /* token buffer */
    XMLC                    * xf_tbuf;
    long                      xf_tbuflen;
    long                      xf_tbufsize;
    /* line buffer */
    XMLC                    * xf_lbuf;
    long                      xf_lbuflen;
    long                      xf_lbufsize;
    long                      xf_linenum;
};
/***************************************************************/
struct xt_ielement {
    int                       xc_mxElements;
    int                       xc_ncElements;
    struct xt_element      ** xc_cElements;
    struct xt_element       * xc_element;
    int                       xc_pcdata;
};
/***************************************************************/
#define XMLSPCTYP_ELEMENT   1
#define XMLSPCTYP_CONTENT   2
#define XMLSPCTYP_ENDTAG    3
struct xt_precontent {
    struct xt_precontent    * xp_next;
    unsigned char             xp_typ;
    unsigned char             xp_flag;
    unsigned char             xp_fill0;
    unsigned char             xp_fill1;
    int                       xp_buffer;
};
/***************************************************************/
#define XMLSFLAG_HASDTD     1
#define XMLSFLAG_IMPLYDTD   2
#define XMLSFLAG_DIDIMPLY   4

struct xmls_file {
    int                       xv_err;
    char                      xv_errparm[128];
    char                      xv_errline[128];
    long                      xv_errlinenum;
    struct xml_stream       * xv_stream;
    int                       xv_vFlags;
    int                       xv_sFlags;
    int                       xv_debug;
    int                       xv_standalone;
    char                      xv_vers[8]; /* either 1.0 or 1.1 */

    /* element content info  */
    XML_FOPEN                 xq_fopen;
    XML_FREAD                 xq_fread;
    XML_FCLOSE                xq_fclose;
    struct xt_extFile *       xv_extFile;

    /* element content info  */
    int                       xc_maxDepth;
    struct xt_ielement     ** xc_ielements;

    struct xt_precontent    * xp_pchead;
    struct xt_precontent    * xp_pctail;

    /* entity name */
    XMLC                    * xf_ebuf;
    long                      xf_ebuflen;
    long                      xf_ebufsize;

    /****** !DOCTYPE ************************/
    XMLC                    * xs_docname;
    struct xt_entities      * xs_p_entities;
    struct xt_entities      * xs_g_entities;
    struct xt_attlists      * xs_attlists;
    struct xt_elements      * xs_elements;
    struct xt_notations     * xs_notations;

    /****** parsing info *******************/
    XML_ELEMENT               xi_p_element;
    XML_CONTENT               xi_p_content;
    XML_ENDTAG                xi_p_endtag;
    void                    * xi_vp;

    int                       xi_depth;
    int                       xi_elNamSize;
    int                       xi_nAttrsSize;
    int                     * xi_attrNamesSize;
    int                     * xi_attrValsSize;
    int                       xi_contentSize;
    int                       xi_contentLen;
    int                       xi_rootElement;

    XMLC                    * xi_elNam;
    int                       xi_nAttrs;
    XMLC                   ** xi_attrNames;
    int                     * xi_attrLens;
    XMLC                   ** xi_attrVals;
    XMLC                    * xi_content;
};
/* END Structures for Xml_ functions                           */
/***************************************************************/
/* function prototypes */
static void xpp_extSubsetDecl(struct xmls_file * xf,
                          struct xml_stream * xs);

#define XCX_RCHR    1
#define XCX_WHIT    2
#define XCX_DIG     4
#define XCX_UPPR    8
#define XCX_LOWR    16
#define XCX_NAMS    32      /* Name start */
#define XCX_NAME    64      /* Name character */
#define XCX_QUOT    128     /* Name start */
#define XCX_PBID    256     /* PubidChar */

#define XCXS_ALPH (XCX_LOWR | XCX_UPPR)
#define XCXS_NAME (XCX_NAME | XCX_NAMS | XCXS_ALPH)
#define XCXS_NAMS (XCX_NAMS | XCXS_ALPH)
#define XCXS_PBID (XCX_DIG  | XCX_PBID | XCXS_ALPH)
/* PubidChar ::=  #x20 | #xD | #xA | [a-zA-Z0-9] | [-'()+,./:=?;!*#@$_%] */

static short int xml_ctf[128] = {
/* 00  ^@ */ 0                  , /* 01  ^A */ XCX_RCHR           ,
/* 02  ^B */ XCX_RCHR           , /* 03  ^C */ XCX_RCHR           ,
/* 04  ^D */ XCX_RCHR           , /* 05  ^E */ XCX_RCHR           ,
/* 06  ^F */ XCX_RCHR           , /* 07  ^G */ XCX_RCHR           ,
/* 08  ^H */ XCX_RCHR           , /* 09  ^I */ XCX_WHIT           ,
/* 0A  ^J */ XCX_WHIT | XCX_PBID, /* 0B  ^K */ XCX_RCHR           ,
/* 0C  ^L */ XCX_RCHR           , /* 0D  ^M */ XCX_WHIT | XCX_PBID,
/* 0E  ^N */ XCX_RCHR           , /* 0F  ^O */ XCX_RCHR           ,
/* 10  ^P */ XCX_RCHR           , /* 11  ^Q */ XCX_RCHR           ,
/* 12  ^R */ XCX_RCHR           , /* 13  ^S */ XCX_RCHR           ,
/* 14  ^T */ XCX_RCHR           , /* 15  ^U */ XCX_RCHR           ,
/* 16  ^V */ XCX_RCHR           , /* 17  ^W */ XCX_RCHR           ,
/* 18  ^X */ XCX_RCHR           , /* 19  ^Y */ XCX_RCHR           ,
/* 1A  ^Z */ XCX_RCHR           , /* 1B     */ XCX_RCHR           ,
/* 1C     */ XCX_RCHR           , /* 1D     */ XCX_RCHR           ,
/* 1E     */ XCX_RCHR           , /* 1F     */ XCX_RCHR           ,
/* 20 ' ' */ XCX_WHIT | XCX_PBID, /* 21 '!' */ XCX_PBID,
/* 22 '"' */ XCX_QUOT,            /* 23 '#' */ XCX_PBID,
/* 24 '$' */ XCX_PBID,            /* 25 '%' */ XCX_PBID,
/* 26 '&' */ 0       ,            /* 27 ''' */ XCX_QUOT | XCX_PBID,
/* 28 '(' */ XCX_PBID,            /* 29 ')' */ XCX_PBID,
/* 2A '*' */ XCX_PBID,            /* 2B '+' */ XCX_PBID,
/* 2C ',' */ XCX_PBID,            /* 2D '-' */ XCX_NAME | XCX_PBID,
/* 2E '.' */ XCX_NAME | XCX_PBID, /* 2F '/' */ XCX_PBID,
/* 30 '0' */ XCX_DIG  | XCX_NAME, /* 31 '1' */ XCX_DIG  | XCX_NAME,
/* 32 '2' */ XCX_DIG  | XCX_NAME, /* 33 '3' */ XCX_DIG  | XCX_NAME,
/* 34 '4' */ XCX_DIG  | XCX_NAME, /* 35 '5' */ XCX_DIG  | XCX_NAME,
/* 36 '6' */ XCX_DIG  | XCX_NAME, /* 37 '7' */ XCX_DIG  | XCX_NAME,
/* 38 '8' */ XCX_DIG  | XCX_NAME, /* 39 '9' */ XCX_DIG  | XCX_NAME,
/* 3A ':' */ XCX_NAMS | XCX_PBID, /* 3B ';' */ XCX_PBID,
/* 3C '<' */ 0       ,            /* 3D '=' */ XCX_PBID,
/* 3E '>' */ 0       ,            /* 3F '?' */ XCX_PBID,
/* 40 '@' */ XCX_PBID,            /* 41 'A' */ XCX_NAMS | XCX_UPPR,
/* 42 'B' */ XCX_NAMS | XCX_UPPR, /* 43 'C' */ XCX_NAMS | XCX_UPPR,
/* 44 'D' */ XCX_NAMS | XCX_UPPR, /* 45 'E' */ XCX_NAMS | XCX_UPPR,
/* 46 'F' */ XCX_NAMS | XCX_UPPR, /* 47 'G' */ XCX_NAMS | XCX_UPPR,
/* 48 'H' */ XCX_NAMS | XCX_UPPR, /* 49 'I' */ XCX_NAMS | XCX_UPPR,
/* 4A 'J' */ XCX_NAMS | XCX_UPPR, /* 4B 'K' */ XCX_NAMS | XCX_UPPR,
/* 4C 'L' */ XCX_NAMS | XCX_UPPR, /* 4D 'M' */ XCX_NAMS | XCX_UPPR,
/* 4E 'N' */ XCX_NAMS | XCX_UPPR, /* 4F 'O' */ XCX_NAMS | XCX_UPPR,
/* 50 'P' */ XCX_NAMS | XCX_UPPR, /* 51 'Q' */ XCX_NAMS | XCX_UPPR,
/* 52 'R' */ XCX_NAMS | XCX_UPPR, /* 53 'S' */ XCX_NAMS | XCX_UPPR,
/* 54 'T' */ XCX_NAMS | XCX_UPPR, /* 55 'U' */ XCX_NAMS | XCX_UPPR,
/* 56 'V' */ XCX_NAMS | XCX_UPPR, /* 57 'W' */ XCX_NAMS | XCX_UPPR,
/* 58 'X' */ XCX_NAMS | XCX_UPPR, /* 59 'Y' */ XCX_NAMS | XCX_UPPR,
/* 5A 'Z' */ XCX_NAMS | XCX_UPPR, /* 5B '[' */ 0       ,
/* 5C '\' */ 0       ,            /* 5D ']' */ 0       ,
/* 5E '^' */ 0       ,            /* 5F '_' */ XCX_NAMS | XCX_PBID,
/* 60 '`' */ 0       ,            /* 61 'a' */ XCX_NAMS | XCX_LOWR,
/* 62 'b' */ XCX_NAMS | XCX_LOWR, /* 63 'c' */ XCX_NAMS | XCX_LOWR,
/* 64 'd' */ XCX_NAMS | XCX_LOWR, /* 65 'e' */ XCX_NAMS | XCX_LOWR,
/* 66 'f' */ XCX_NAMS | XCX_LOWR, /* 67 'g' */ XCX_NAMS | XCX_LOWR,
/* 68 'h' */ XCX_NAMS | XCX_LOWR, /* 69 'i' */ XCX_NAMS | XCX_LOWR,
/* 6A 'j' */ XCX_NAMS | XCX_LOWR, /* 6B 'k' */ XCX_NAMS | XCX_LOWR,
/* 6C 'l' */ XCX_NAMS | XCX_LOWR, /* 6D 'm' */ XCX_NAMS | XCX_LOWR,
/* 6E 'n' */ XCX_NAMS | XCX_LOWR, /* 6F 'o' */ XCX_NAMS | XCX_LOWR,
/* 70 'p' */ XCX_NAMS | XCX_LOWR, /* 71 'q' */ XCX_NAMS | XCX_LOWR,
/* 72 'r' */ XCX_NAMS | XCX_LOWR, /* 73 's' */ XCX_NAMS | XCX_LOWR,
/* 74 't' */ XCX_NAMS | XCX_LOWR, /* 75 'u' */ XCX_NAMS | XCX_LOWR,
/* 76 'v' */ XCX_NAMS | XCX_LOWR, /* 77 'w' */ XCX_NAMS | XCX_LOWR,
/* 78 'x' */ XCX_NAMS | XCX_LOWR, /* 79 'y' */ XCX_NAMS | XCX_LOWR,
/* 7A 'z' */ XCX_NAMS | XCX_LOWR, /* 7B '{' */ 0       ,
/* 7C '|' */ 0       ,            /* 7D '}' */ 0       ,
/* 7E '~' */ 0       ,            /* 7F     */ 0
};

/***************************************************************/
/* Error numbers                                               */
#define XMLERR_NOERR            0
#define XMLERR_NOTXML           1
#define XMLERR_BADVERS          2
#define XMLERR_UNSUPENCODING    3
#define XMLERR_BADENCMARK       4
#define XMLERR_BADSTANDALONE    5
#define XMLERR_EXPLITERAL       6
#define XMLERR_BADPUBID         7
#define XMLERR_BADNAME          8
#define XMLERR_BADPEREF         9
#define XMLERR_BADROOT          10
#define XMLERR_EXPCLOSE         11
#define XMLERR_EXPDOCTYPE       12
#define XMLERR_DUPDOCTYPE       13
#define XMLERR_EXPMARKUP        14
#define XMLERR_EXPNDATA         15
#define XMLERR_EXPSYSPUB        16
#define XMLERR_EXPANYEMPTY      17
#define XMLERR_EXPPAREN         18
#define XMLERR_EXPSEP           19
#define XMLERR_REPEAT           20
#define XMLERR_EXPCLOSEPAREN    21
#define XMLERR_EXPBAR           22
#define XMLERR_EXPSTAR          23
#define XMLERR_EXPCOMMA         24
#define XMLERR_EXPREPEAT        25
#define XMLERR_EXPATTTYP        26
#define XMLERR_EXPATTVAL        27
#define XMLERR_EXPATTDEF        28
#define XMLERR_EXPPCDATA        29
#define XMLERR_NOSUCHPE         30
#define XMLERR_RECURENTITY      31
#define XMLERR_BADEXTDTD        32
#define XMLERR_OPENSYSFILE      33
#define XMLERR_EXPSYSEOF        34
#define XMLERR_EXPIGNINC        35
#define XMLERR_EXPOPBRACKET     36
#define XMLERR_EXPCLBRACKET     37
#define XMLERR_EXPPEREF         38  /* should never happen */
#define XMLERR_BADIGNINC        39
#define XMLERR_EXPPUBLIT        40
#define XMLERR_BADFORM          41
#define XMLERR_DUPATTR          42
#define XMLERR_UNEXPEOF         43
#define XMLERR_INTERNAL1        44  /* should never happen */
#define XMLERR_EXPEQ            45
#define XMLERR_BADGEREF         46
#define XMLERR_NOSUCHGE         47
#define XMLERR_BADCHREF         48
#define XMLERR_EXPCTL           49
#define XMLERR_NOSUCHELEMENT    50
#define XMLERR_NOSUCHATTLIST    51
#define XMLERR_DUPATTLIST       52
#define XMLERR_ATTREQ           53
#define XMLERR_UNEXPCHARS       54
#define XMLERR_EXPMISC          55
#define XMLERR_UNEXPCLOSE       56

/***************************************************************/
/* Inline Functions                                            */
#define XML_IS_WHITE(xc) ((xc)<128 ? (xml_ctf[xc]&XCX_WHIT ) : 0)
#define XML_IS_NSTART(xc)((xc)<128 ? (xml_ctf[xc]&XCXS_NAMS) : XMLC_NStart(xc))
#define XML_IS_NAME(xc)  ((xc)<128 ? (xml_ctf[xc]&XCXS_NAME) : XMLC_Name(xc))
#define XML_IS_QUOTE(xc) ((xc)<128 ? (xml_ctf[xc]&XCX_QUOT ) : 0)
#define XML_IS_PUBID(xc) ((xc)<128 ? (xml_ctf[xc]&XCXS_PBID) : 0)
#define XML_IS_ALPHA(xc) ((xc)<128 ? (xml_ctf[xc]&XCXS_ALPH) : 0)
#define XML_IS_RCHAR(xc) ((xc)<128 ? (xml_ctf[xc]&XCX_RCHR)  : XMLC_RChar(xc))
#define xv_err(xf) ((xf)->xv_err)
/***************************************************************/
/*
** set flags
*/
#define FILE_ACCESSED     1 /* File has ever been accessed since rewind */
#define FILE_LOCKED       2 /* File is currently locked */
#define FILE_INUSE        4 /* File is being used */

#define XML_OPTION_FIELDNAMES 1 /* First record contains field names */
/***************************************************************/
#if 0
void T1memcpy(void * tgt, void * src, int nb) {

    int ii;
    char * c_tgt = (char*)tgt;
    char * c_src = (char*)src;

    for (ii = 0; ii < nb; ii++) {
        c_tgt[ii] = c_src[ii];
    }
}
#endif
/***************************************************************/
void mvchartoxmlc(XMLC * tgt, char * src, int srclen) {

    int ii;
    for (ii = 0; ii < srclen; ii++)
        tgt[ii] = (unsigned char)src[ii];
}
/***************************************************************/
void mvxmlctochar(char * tgt, XMLC * src, int srclen) {

    int ii;
    for (ii = 0; ii < srclen; ii++) {
        if (src[ii] > 254) tgt[ii] = (unsigned char)XMLCOVFL;
        else               tgt[ii] = (unsigned char)src[ii];
    }
}
/***************************************************************/
void strtoxmlc(XMLC * tgt, char * src, int tgtlen) {

    int ii;
    for (ii = 0; ii < tgtlen && src[ii]; ii++)
        tgt[ii] = (unsigned char)src[ii];
    if (ii < tgtlen) tgt[ii] = 0; else tgt[tgtlen-1] = 0;
}
/***************************************************************/
void xmlctostr(char * tgt, XMLC * src, int tgtlen) {

    int ii;
    for (ii = 0; ii < tgtlen && src[ii]; ii++) {
        if (src[ii] > 254) tgt[ii] = (unsigned char)XMLCOVFL;
        else               tgt[ii] = (unsigned char)src[ii];
    }
    if (ii < tgtlen) tgt[ii] = 0; else tgt[tgtlen-1] = 0;
}
/***************************************************************/
static int xmlcstrcmp(XMLC * xtok, char * buf) {

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
static int xmlcistrcmp(XMLC * xtok, char * buf) {

    int cmp;
    int ii;

    if (!xtok) return (-1);

    cmp = 0;
    ii  = 0;
    while (!cmp && xtok[ii] && buf[ii]) {
        if (xtok[ii] == buf[ii])     ii++;
        else if (XML_IS_ALPHA(xtok[ii]) &&
                 XML_IS_ALPHA(buf[ii]) &&
                 tolower(xtok[ii]) == tolower(buf[ii])) ii++;
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
int xmlccmp(XMLC * xtok1, XMLC * xtok2) {

    int cmp;
    int ii;

    cmp = 0;
    ii  = 0;
    while (!cmp && xtok1[ii] && xtok2[ii]) {
        if      (xtok1[ii] == xtok2[ii])    ii++;
        else if (xtok1[ii] < xtok2[ii])     cmp = -1;
        else                                cmp =  1;
    }
    if (!cmp) {
        if      (xtok1[ii]) cmp = 1;
        else if (xtok2[ii]) cmp = -1;
    }

    return (cmp);
}
/***************************************************************/
int xmlclen(XMLC * xtok) {

    int len;

    if (!xtok) return (0);

    len = 0;
    while (xtok[len]) len++;

    return (len);
}
/***************************************************************/
static int xmlcstrbytes(XMLC * xtok) {

    int len;

    if (!xtok) return (0);

    len = 0;
    while (xtok[len]) len++;

    return (XMLC_BYTES(len + 1));
}
/***************************************************************/
XMLC * xmlcstrdup(XMLC * xtok)
{
    int len;
    XMLC * out;

    if (!xtok) return (0);

    len = xmlcstrbytes(xtok);

    out = New(XMLC, len);
    memcpy(out, xtok, len);

    return (out);
}
/***************************************************************/
XMLC *  xmlcstrdups(char * str) {

    int len;
    XMLC * out;

    if (!str) return (0);

    len = strlen(str) + 1;

    out = New(XMLC, len);
    strtoxmlc(out, str, len);

    return (out);
}
/***************************************************************/
void xmlccpy(XMLC * xtgt, XMLC * xsrc) {

    int ii;

    for (ii = 0; xsrc[ii]; ii++) xtgt[ii] = xsrc[ii];
    xtgt[ii] = xsrc[ii];
}
/***************************************************************/
void xmlccat(XMLC * xtgt, XMLC * xsrc) {

    xmlccpy(xtgt + xmlclen(xtgt), xsrc);
}
/***************************************************************/
static int Xml_insert_entity(struct xt_entities * entities,
                  int etyp,
                  XMLC * wkeyval,
                  XMLC * wsdata,
                  struct xt_extID * extID,
                  XMLC * ndata) {

/*
** Assumes wkeyval and wsdata have already been "New'd"
*/
    struct xt_entity * en;
    struct xt_entity ** pen;
    int keylen;

    keylen = xmlcstrbytes(wkeyval);
    pen = (struct xt_entity **)dbtree_find(entities->xe_entity,
                            wkeyval, keylen);
    if (pen) return (0);

    en = New(struct xt_entity, 1);

    en->e_typ   = etyp;
    en->e_name  = wkeyval;
    en->e_val   = wsdata;
    en->e_extID = extID;
    en->e_ndata = ndata;
    en->e_len   = xmlclen(wsdata);
    en->e_ix    = 1;

    return dbtree_insert(entities->xe_entity,
                            wkeyval, keylen, en);
}
/***************************************************************/
static int Xml_insert_entity_ss(struct xt_entities * entities,
                  char * keyval,
                  char * sdata) {

    XMLC wkeyval[128];
    XMLC wsdata[128];
    int lkeyval;
    int lsdata;

    lkeyval = strlen(keyval) + 1;
    if (lkeyval > 128) return (-1);
    strtoxmlc(wkeyval, keyval, 128);

    lsdata = strlen(sdata) + 1;
    if (lsdata > 128) return (-1);
    strtoxmlc(wsdata, sdata, 128);

    return Xml_insert_entity(entities, ENTYP_SYS,
                    xmlcstrdup(wkeyval), xmlcstrdup(wsdata), 0, 0);
}
/***************************************************************/
static struct xt_element * Xml_new_element(XMLC * elName, int eltyp) {
/*
** eltyp:
**   1 - ANY type element
**   2 - EMPTY type element
**   3 - Mixed/children element
*/
    struct xt_element * el;

    el = New(struct xt_element, 1);
    el->l_name       = elName;
    el->l_typ        = eltyp;
    el->l_attlist    = NULL;
    el->l_tree       = NULL;
    el->l_i_elements = NULL;

    return (el);
}
/***************************************************************/
static void Xml_free_extID(struct xt_extID * extID) {

    Free(extID->x_extID);
    Free(extID->x_pubID);
    Free(extID);
}
/***************************************************************/
static void Xml_free_eltree(struct xt_eltree * elt) {

    int ii;

    if (!elt) return;

    Free(elt->lt_op);

    for (ii = 0; ii < elt->lt_nnodes; ii++) {
        Xml_free_eltree(elt->lt_node[ii]);
    }
    Free(elt->lt_node);
    Free(elt);
}
/***************************************************************/
static void Xml_free_imp_element(void * pdata) {

    struct xt_imp_element * mel = pdata;

    Free(mel->m_elName);

    Free(mel);
}
/***************************************************************/
static void Xml_free_i_elements(struct xt_i_elements * ie) {

    dbtree_free(ie->i_ielements, Xml_free_imp_element);

    Free(ie);
}
/***************************************************************/
static void Xml_free_element(void * pdata) {

    struct xt_element * el = pdata;

    Free(el->l_name);
    Xml_free_eltree(el->l_tree);
    if (el->l_i_elements) Xml_free_i_elements(el->l_i_elements);

    Free(el);
}
/***************************************************************/
static void Xml_free_elements(struct xt_elements * elements) {

    dbtree_free(elements->xl_elements, Xml_free_element);

    Free(elements);
}
/***************************************************************/
static void Xml_free_entity(void * pdata) {

    struct xt_entity * en = pdata;

    if (en->e_extID) Xml_free_extID(en->e_extID);
    Free(en->e_ndata);
    Free(en->e_name);
    Free(en->e_val);

    Free(en);
}
/***************************************************************/
static void Xml_free_entities(struct xt_entities * entities) {

    dbtree_free(entities->xe_entity, Xml_free_entity);

    Free(entities);
}
/***************************************************************/
static void Xml_free_elist(void * pdata) {

    struct xt_eitem * ei = pdata;

    Free(ei->e_name);

    Free(ei);
}
/***************************************************************/
static void Xml_free_attdef(void * pdata) {

    struct xt_attdef * ad = pdata;

    Free(ad->a_attname);
    Free(ad->a_defaultval);

    if (ad->a_elist) dbtree_free(ad->a_elist, Xml_free_elist);

    Free(ad);
}
/***************************************************************/
static void Xml_free_attlist(void * pdata) {

    struct xt_attlist * al = pdata;

    Free(al->a_elname);

    dbtree_free(al->a_attdefs, Xml_free_attdef);

    Free(al);
}
/***************************************************************/
static void Xml_free_attlists(struct xt_attlists * attlists) {

    dbtree_free(attlists->xa_attlist, Xml_free_attlist);

    Free(attlists);
}
/***************************************************************/
static void Xml_free_notation(void * pdata) {

    struct xt_notation * not = pdata;

    Free(not->n_notname);
    Xml_free_extID(not->n_extID);

    Free(not);
}
/***************************************************************/
static void Xml_free_notations(struct xt_notations * notations) {

    dbtree_free(notations->xn_notations, Xml_free_notation);

    Free(notations);
}
/***************************************************************/
static void Xml_free_stream(struct xml_stream * xs) {

    Free(xs->xf_fbuf);
    Free(xs->xf_tbuf);
    Free(xs->xf_lbuf);

    Free(xs);
}
/***************************************************************/
static void Xml_free_parsinfo(struct xmls_file * xf) {

    int ii;

    for (ii = 0; ii < xf->xi_nAttrsSize; ii++) {
        Free(xf->xi_attrNames[ii]);
        Free(xf->xi_attrVals[ii]);
    }
    Free(xf->xi_attrNamesSize);
    Free(xf->xi_attrValsSize);
    Free(xf->xi_elNam);
    Free(xf->xi_attrNames);
    Free(xf->xi_attrLens);
    Free(xf->xi_attrVals);
    Free(xf->xi_content);

    Free(xf->xf_ebuf);
}
/***************************************************************/
static void Xml_free_ielement(struct xt_ielement * xie) {

    Free(xie->xc_cElements);

    Free(xie);
}
/***************************************************************/
static void Xml_free_content(struct xmls_file * xf) {

    int ii;

    for (ii = 0; ii < xf->xc_maxDepth; ii++) {
        Xml_free_ielement(xf->xc_ielements[ii]);
    }
    Free(xf->xc_ielements);
}
/***************************************************************/
static void Xml_free_pchead(struct xmls_file * xf) {

    struct xt_precontent * xpc;

    xpc = xf->xp_pchead;
    if (xpc) {
        xf->xp_pchead = xpc->xp_next;
        if (!(xf->xp_pchead)) xf->xp_pctail = 0;
        Free(xpc);
    }
}
/***************************************************************/
static void Xml_free(struct xmls_file * xf) {

    Free(xf->xs_docname);

    Xml_free_stream(xf->xv_stream);

    Xml_free_entities(xf->xs_g_entities);
    Xml_free_entities(xf->xs_p_entities);
    Xml_free_attlists(xf->xs_attlists);
    Xml_free_elements(xf->xs_elements);
    Xml_free_notations(xf->xs_notations);

    Xml_free_content(xf);

    Xml_free_parsinfo(xf);

    while (xf->xp_pchead) Xml_free_pchead(xf);

    Free(xf);
}
/***************************************************************/
static struct xml_stream * Xml_new_stream(struct xmls_file * xf,
                                          struct xt_extFile * extFil,
                                          const char * xml_string) {

    struct xml_stream * xs;

    xs = New(struct xml_stream, 1);

    xs->xf_xf           = xf;
    xs->xf_encoding     = 0;

    if (xml_string) {
        xs->xf_fbuflen      = (int)strlen(xml_string);
        xs->xf_fbufsize     = xs->xf_fbuflen + 1;
        xs->xf_fbuf         = New(unsigned char, xs->xf_fbufsize);
        memcpy(xs->xf_fbuf, xml_string, xs->xf_fbuflen + 1);
    } else {
        xs->xf_fbufsize     = XML_K_DEFAULT_FBUFSIZE;
        xs->xf_fbuf         = New(unsigned char, xs->xf_fbufsize);
        xs->xf_fbuflen      = 0;
    }
    xs->xf_fbufix       = 0;
    xs->xf_prev_fbufix  = 0;
    xs->xf_iseof        = 0;

    xs->xf_tbufsize     = XML_K_DEFAULT_TBUFSIZE;
    xs->xf_tbuf         = New(XMLC, xs->xf_tbufsize);
    xs->xf_tbuflen      = 0;
    xs->xf_tadv         = 0;

    xs->xf_lbufsize     = XML_K_DEFAULT_LBUFSIZE;
    xs->xf_lbuf         = New(XMLC, xs->xf_lbufsize);
    xs->xf_lbuflen      = 0;
    xs->xf_linenum      = 0;
    xs->xf_extFile      = extFil;

    return (xs);
}
/***************************************************************/
static void Xml_fread(struct xml_stream * xs, int bufix) {

    if (!xs->xf_extFile) {
        xs->xf_iseof = 1;
    } else {
        if (xs->xf_xf->xq_fread) {
            xs->xf_fbuflen = xs->xf_xf->xq_fread(xs->xf_extFile->fref,
                    xs->xf_fbuf + bufix, xs->xf_fbufsize - bufix);
            if (xs->xf_fbuflen <= 0) {
                xs->xf_iseof = 1;
            }
            xs->xf_fbuflen += bufix;
            xs->xf_fbufix   = bufix;
        } else {
            xs->xf_iseof = 1;
        }
    }
}
/***************************************************************/
static int XMLC_Char(XMLC xc) {
/*
2]    Char    ::=    [#x1-#xD7FF] | [#xE000-#xFFFD] | [#x10000-#x10FFFF]
[2a]    RestrictedChar    ::=    [#x1-#x8] | [#xB-#xC] | [#xE-#x1F] |
            [#x7F-#x84] | [#x86-#x9F]
*/
    return (0);
}
/***************************************************************/
static int XMLC_RChar(XMLC xc) {
/*
[2a]    RestrictedChar    ::=    [#x1-#x8] | [#xB-#xC] | [#xE-#x1F] |
            [#x7F-#x84] | [#x86-#x9F]
*/
    int ok;

    ok = (xc >= 0x0001 && xc <= 0x0008) ||
         (xc >= 0x000B && xc <= 0x000C) ||
         (xc >= 0x000E && xc <= 0x001F) ||
         (xc >= 0x007F && xc <= 0x0084) ||
         (xc >= 0x0086 && xc <= 0x009F);

    return (ok);
}
/***************************************************************/
int XMLC_NStart(XMLC xc) {
/*
[4]    NameStartChar    ::=    ":" | [A-Z] | "_" | [a-z] | [#xC0-#xD6] |
            [#xD8-#xF6] | [#xF8-#x2FF] | [#x370-#x37D] | [#x37F-#x1FFF] |
            [#x200C-#x200D] | [#x2070-#x218F] | [#x2C00-#x2FEF] |
            [#x3001-#xD7FF] | [#xF900-#xFDCF] | [#xFDF0-#xFFFD] |
            [#x10000-#xEFFFF]
[4a]    NameChar    ::=    NameStartChar | "-" | "." | [0-9] | #xB7 |
            [#x0300-#x036F] | [#x203F-#x2040]
[5]    Name    ::=    NameStartChar (NameChar)*
[6]    Names    ::=    Name (#x20 Name)*
[7]    Nmtoken    ::=    (NameChar)+
[8]    Nmtokens    ::=    Nmtoken (#x20 Nmtoken)*
*/
    int ok;

    ok = (xc == ':' || xc == '_') ||
         (xc >= 'A' && xc <= 'Z') ||
         (xc >= 'a' && xc <= 'a') ||
         (xc >= 0x00D8 && xc <= 0x00F6) ||
         (xc >= 0x00F8 && xc <= 0x02FF) ||
         (xc >= 0x0370 && xc <= 0x00F6) ||
         (xc >= 0x037F && xc <= 0x1FFF) ||
         (xc >= 0x200C && xc <= 0x200D) ||
         (xc >= 0x2070 && xc <= 0x218F) ||
         (xc >= 0x2C00 && xc <= 0x2FEF) ||
         (xc >= 0x3001 && xc <= 0xD7FF) ||
         (xc >= 0xF900 && xc <= 0xFDCF) ||
         (xc >= 0xFDF0 && xc <= 0xFFFD);

    return (ok);
}
/***************************************************************/
static int XMLC_Name(XMLC xc) {
/*
[4a]    NameChar    ::=    NameStartChar | "-" | "." | [0-9] | #xB7 |
            [#x0300-#x036F] | [#x203F-#x2040]
*/
    int ok;

    ok = XMLC_NStart(xc) ||
         (xc == '-' || xc == '.' || xc == 0xB7) ||
         (xc >= '0' && xc <= '9') ||
         (xc >= 0x0300 && xc <= 0x036F) ||
         (xc >= 0x203F && xc <= 0x2040);

    return (ok);
}
/***************************************************************/
static void Xml_unget_char(struct xml_stream * xs) {

    xs->xf_fbufix = xs->xf_prev_fbufix;
    xs->xf_tadv = 0;
    if (xs->xf_lbuflen) xs->xf_lbuflen--;
}
/***************************************************************/
static XMLC Xml_get_char(struct xml_stream * xs) {
/*
    Code range
    hexadecimal      UTF-8
    000000 - 00007F  0xxxxxxx
    000080 - 0007FF  110xxxxx 10xxxxxx
    000800 - 00FFFF  1110xxxx 10xxxxxx 10xxxxxx

*/
    XMLC xc;
    unsigned char cb[XMLCMAXSZ];

    if (xs->xf_iseof) return (XMLCEOF);

    if (xs->xf_tadv) {
        xs->xf_fbufix++;
    }

    if (xs->xf_fbufix == xs->xf_fbuflen) {
        Xml_fread(xs, 0);
        if (xs->xf_iseof) return (XMLCEOF);
    }

    xs->xf_prev_fbufix = xs->xf_fbufix;
    cb[0] = xs->xf_fbuf[xs->xf_fbufix];
    xs->xf_tadv = 1;

    if (xs->xf_encoding & 2 || (!xs->xf_encoding && (cb[0] & 0x80))) {
        xs->xf_fbufix++;
        if (xs->xf_fbufix == xs->xf_fbuflen) {
            Xml_fread(xs, 1);
            if (xs->xf_iseof) return (XMLCEOF);
            xs->xf_fbuf[0]      = cb[0];
            xs->xf_prev_fbufix  = 0;
        }

        cb[1] = xs->xf_fbuf[xs->xf_fbufix];
        if (xs->xf_encoding == XML_ENC_UNICODE_BE) {
            xc = (XMLC)((cb[0] << 8) | cb[1]);
        } else if (xs->xf_encoding == XML_ENC_UNICODE_LE) {
            xc = (XMLC)((cb[1] << 8) | cb[0]);
        } else { /* UTF-8 */
            if (cb[0] & 0x20) {
                xs->xf_fbufix++;
                if (xs->xf_fbufix == xs->xf_fbuflen) {
                    Xml_fread(xs, 2);
                    if (xs->xf_iseof) return (XMLCEOF);
                    xs->xf_fbuf[0]      = cb[0];
                    xs->xf_fbuf[1]      = cb[1];
                    xs->xf_prev_fbufix  = 0;
                }

                cb[2] = xs->xf_fbuf[xs->xf_fbufix];
                xc = (XMLC)(((cb[0] & 0x0F) << 12) |
                           ((cb[1] & 0x3F) <<  6) | (cb[2] & 0x3F));
            } else {
                xc = (XMLC)(((cb[0] & 0x1F) << 6) | (cb[1] & 0x3F));
            }
        }
    } else {
        xc = (XMLC)(cb[0]);
    }

    if (xc == '\n' || xc == '\r') {
        xs->xf_lbuflen = 0;
        if (xc == '\n') xs->xf_linenum++;
    } else if (xs->xf_lbuflen || !XML_IS_WHITE(xc)) {
        if (xs->xf_lbuflen == xs->xf_lbufsize) {
            xs->xf_lbufsize += XML_K_LBUF_INC;
            xs->xf_lbuf = Realloc(xs->xf_lbuf, XMLC, xs->xf_lbufsize);
        }
        xs->xf_lbuf[xs->xf_lbuflen] = xc;
        xs->xf_lbuflen++;
    }

    return (xc);
}
/***************************************************************/
static XMLC Xml_peek_char(struct xml_stream * xs) {

    XMLC xc;

    xc = Xml_get_char(xs);
    Xml_unget_char(xs);

    return (xc);
}
/***************************************************************/
static XMLC Xml_get_nwchar(struct xml_stream * xs) {

    XMLC xc;

    do {
        xc = Xml_get_char(xs);
    } while (XML_IS_WHITE(xc));
    if (xc == XMLCEOF) return (0);

    return (xc);
}
/***************************************************************/
static XMLC Xml_peek_nwchar(struct xml_stream * xs) {

    XMLC xc;

    do {
        xc = Xml_get_char(xs);
    } while (XML_IS_WHITE(xc));
    if (xc == XMLCEOF) return (0);

    Xml_unget_char(xs);

    return (xc);
}
/***************************************************************/
static void Xml_seterr_S(struct xmls_file * xf,
                         struct xml_stream * xs,
                         int errnum,
                         XMLC * eparm) {

    int len;

    xf->xv_err = errnum;
    if (eparm) {
        xmlctostr(xf->xv_errparm, eparm, sizeof(xf->xv_errparm));
    } else {
        xf->xv_errparm[0] = '\0';
    }

    if (xs) {
        if (xs->xf_lbuflen >= sizeof(xf->xv_errline)) {
            len = sizeof(xf->xv_errline) - 1;
        } else {
            len = xs->xf_lbuflen;
        }
        xmlctostr(xf->xv_errline, xs->xf_lbuf, len);
        xf->xv_errline[len] = '\0';
        xf->xv_errlinenum = xs->xf_linenum + 1;
    } else {
        xf->xv_errparm[0] = '\0';
        xf->xv_errlinenum = 0;
    }
}
/***************************************************************/
static void Xml_seterr_C(struct xmls_file * xf,
                         struct xml_stream * xs,
                         int errnum,
                         XMLC eparm) {

    XMLC ep[2];

    ep[0] = eparm;
    ep[1] = 0;

    Xml_seterr_S(xf, xs, errnum, ep);
}
/***************************************************************/
static void Xml_seterr(struct xmls_file * xf,
                       struct xml_stream * xs,
                       int errnum) {

    Xml_seterr_S(xf, xs, errnum, 0);
}
/***************************************************************/
static void Xml_seterr_i(struct xmls_file * xf,
                         struct xml_stream * xs,
                         int errnum,
                         int parm) {

    char nbuf[16];
    XMLC pbuf[16];

    sprintf(nbuf, "%d", parm);
    strtoxmlc(pbuf, nbuf, 16);

    Xml_seterr_S(xf, xs, errnum, pbuf);
}
/***************************************************************/
static void Xml_autodetect_encoding(struct xml_stream * xs) {
/*
** See: http://en.wikipedia.org/wiki/Byte_Order_Mark
**
** Byte order mark:

    Unicode - Big endian
        xs->xf_fbuf[0]  0xfe   -2
        xs->xf_fbuf[1]  0xff   -1
        xs->xf_fbuf[2]  0x00    0
        xs->xf_fbuf[3]  0x3c   60
    Unicode - Little endian
        xs->xf_fbuf[0]  0xff   -1
        xs->xf_fbuf[1]  0xfe   -2
        xs->xf_fbuf[2]  0x3c   60
        xs->xf_fbuf[3]  0x00    0
    UTF-8
        xs->xf_fbuf[0]  0xef  -17
        xs->xf_fbuf[1]  0xbb  -69
        xs->xf_fbuf[2]  0xbf  -65
        xs->xf_fbuf[3]  0x3c   60
*/

    int rem;

    rem = xs->xf_fbuflen - xs->xf_fbufix;

    if (rem < XMLCMAXSZ) {
        if (rem > xs->xf_fbufix) {
            return;
        }
        if (rem) memcpy(xs->xf_fbuf, xs->xf_fbuf + xs->xf_fbufix, rem);

        Xml_fread(xs, rem);
        if (xs->xf_iseof) return;
        xs->xf_fbufix = 0;

        if (xs->xf_fbuflen - xs->xf_fbufix < XMLCMAXSZ) return;
    }

    if (xs->xf_fbuf[xs->xf_fbufix] == 0xFE) {
        if (xs->xf_fbuf[xs->xf_fbufix + 1] == 0xFF) {
            xs->xf_encoding = XML_ENC_UNICODE_BE;
            xs->xf_fbufix += 2;
        }
    } else if (xs->xf_fbuf[xs->xf_fbufix] == 0xFF) {
        if (xs->xf_fbuf[xs->xf_fbufix + 1] == 0xFE) {
            xs->xf_encoding = XML_ENC_UNICODE_LE;
            xs->xf_fbufix += 2;
        }
    } else if (xs->xf_fbuf[xs->xf_fbufix] == 0xEF) {
        if (xs->xf_fbuf[xs->xf_fbufix + 1] == 0xBB) {
            if (xs->xf_fbuf[xs->xf_fbufix + 2] == 0xBF) {
                xs->xf_encoding = XML_ENC_UTF_8;
                xs->xf_fbufix += 3;
            }
        }
    }
}
/***************************************************************/
static void Xml_expand_tbuf(struct xml_stream * xs) {

    xs->xf_tbufsize += XML_K_TBUF_INC;
    xs->xf_tbuf = Realloc(xs->xf_tbuf, XMLC, xs->xf_tbufsize);
}
/***************************************************************/
static XMLC * Xml_next_token_name(XMLC xc, struct xml_stream * xs) {

    int done;

    done   = 0;
    do {
        if (xs->xf_tbuflen == xs->xf_tbufsize) {
            Xml_expand_tbuf(xs);
        }
        xs->xf_tbuf[xs->xf_tbuflen] = xc;
        xs->xf_tbuflen++;

        xc = Xml_get_char(xs);
        if (!XML_IS_NAME(xc)) {
            if (xc != XMLCEOF) Xml_unget_char(xs);
            done = 1;
        }
    } while (!done);

    return (xs->xf_tbuf);
}
/***************************************************************/
static XMLC * Xml_next_token_quote_raw(XMLC xc, struct xml_stream * xs) {

    int done;
    XMLC quot;

    quot = xc;
    xc   = '\"';
    done   = 0;
    do {
        if (xs->xf_tbuflen == xs->xf_tbufsize) {
            Xml_expand_tbuf(xs);
        }
        xs->xf_tbuf[xs->xf_tbuflen] = xc;
        xs->xf_tbuflen++;

        xc = Xml_get_char(xs);
        if (xc == XMLCEOF) {
            done = 1;
        } else if (xc == quot) {
            if (xs->xf_tbuflen == xs->xf_tbufsize) {
                Xml_expand_tbuf(xs);
            }
            xs->xf_tbuf[xs->xf_tbuflen] = '\"';
            xs->xf_tbuflen++;
            done = 1;
        }
    } while (!done);

    return (xs->xf_tbuf);
}
/***************************************************************/
static XMLC * Xml_next_token_raw(struct xml_stream * xs) {

    int done;
    XMLC xc;

    done   = 0;
    xs->xf_tbuflen = 0;

    do {
        xc = Xml_get_char(xs);
    } while (XML_IS_WHITE(xc));
    if (xc == XMLCEOF) return (0);

    if (XML_IS_NSTART(xc)) {
        Xml_next_token_name(xc, xs);
    } else if (XML_IS_QUOTE(xc)) {
        Xml_next_token_quote_raw(xc, xs);
    } else {
        if (xs->xf_tbuflen == xs->xf_tbufsize) {
            Xml_expand_tbuf(xs);
        }
        xs->xf_tbuf[xs->xf_tbuflen] = xc;
        xs->xf_tbuflen++;
    }

    if (xs->xf_tbuflen == xs->xf_tbufsize) {
        Xml_expand_tbuf(xs);
    }
    xs->xf_tbuf[xs->xf_tbuflen] = 0;

#ifdef _DEBUG
    wcstombs(xs->aatok, xs->xf_tbuf, sizeof(xs->aatok));
#endif

    return (xs->xf_tbuf);
}
/***************************************************************/
static struct xt_entities *  Xml_new_entities(void) {

    struct xt_entities * entities;

    entities = New(struct xt_entities, 1);
    entities->xe_entity = dbtree_new();

    /* predefined entities */
    Xml_insert_entity_ss(entities, "lt"  , "\"<\"");
    Xml_insert_entity_ss(entities, "gt"  , "\">\"");
    Xml_insert_entity_ss(entities, "amp" , "\"&\"");
    Xml_insert_entity_ss(entities, "quot", "\"\"\"");
    Xml_insert_entity_ss(entities, "apos", "\"\'\"");

    return (entities);
}
/***************************************************************/
static struct xt_elements *  Xml_new_elements(void) {

    struct xt_elements * elements;

    elements = New(struct xt_elements, 1);
    elements->xl_elements = dbtree_new();

    return (elements);
}
/***************************************************************/
static struct xt_attlists *  Xml_new_attlists(void) {

    struct xt_attlists * attlists;

    attlists = New(struct xt_attlists, 1);
    attlists->xa_attlist = dbtree_new();

    return (attlists);
}
/***************************************************************/
static struct xt_notations *  Xml_new_notations(void) {

    struct xt_notations * notations;

    notations = New(struct xt_notations, 1);
    notations->xn_notations = dbtree_new();

    return (notations);
}
/***************************************************************/
static void Xml_doc_init(struct xmls_file * xf) {

    xf->xs_g_entities  = Xml_new_entities();
    xf->xs_p_entities  = Xml_new_entities();
    xf->xs_elements    = Xml_new_elements();
    xf->xs_attlists    = Xml_new_attlists();
    xf->xs_notations   = Xml_new_notations();
}
/***************************************************************/
static void * Xml_std_fopen(
                void * pfref,
                char * fname,
                char * fmode) {

    FILE * fref;

    fref = fopen(fname, fmode);

    return (fref);
}
/***************************************************************/
static long Xml_std_fread(void * pfref,
                void * buffer,
                long buflen) {

    struct xt_extFile * extFil = pfref;
    long nbytes;

    nbytes = fread(buffer, 1, buflen, extFil->fref);

    return (nbytes);
}
/***************************************************************/
static int Xml_std_fclose(void * pfref) {

    FILE * fref = pfref;

    fclose(fref);

    return (0);
}
/***************************************************************/
static void * xpp_open_quoted_fname(struct xmls_file * xf,
                                    XMLC * qxmlcfname) {

    void * fref;
    char fname[1024];
    char * pfname;
    int len;

    xmlctostr(fname, qxmlcfname, sizeof(fname));
    len = strlen(fname);
    if (len > 2 &&
        (fname[0] == '\'' || fname[0] == '\"') &&
        fname[0] == fname[len-1]) {
        fname[len-1] = '\0';
        pfname = fname + 1;
    } else {
        pfname = fname;
    }

    fref = 0;
    if (xf->xq_fopen) {
        fref = xf->xq_fopen(xf->xv_extFile->fref, pfname, "r");
    }

    return (fref);
}
/***************************************************************/
static struct xml_stream * xpp_open_ext_ID(struct xmls_file * xf,
                                           struct xml_stream * xs,
                                           struct xt_extID * extID) {

    struct xml_stream * nxs;
    struct xt_extFile * extFil;
    void * fref;

    fref = 0;
    if (!fref && extID->x_pubID) {
        fref = xpp_open_quoted_fname(xf, extID->x_pubID);
    }

    if (!fref && extID->x_extID) {
        fref = xpp_open_quoted_fname(xf, extID->x_extID);
    }

    if (!fref) {
        Xml_seterr(xf, xs, XMLERR_OPENSYSFILE);
        return (0);
    } else {
        extFil = New(struct xt_extFile, 1);
        extFil->fref = fref;
        nxs = Xml_new_stream(xf, extFil, NULL);
        Xml_autodetect_encoding(nxs);
    }

    return (nxs);
}
/***************************************************************/
static void xpp_close_extFile(struct xmls_file * xf,
                              struct xt_extFile * extFil) {


    if (xf->xq_fclose) {
        xf->xq_fclose(extFil->fref);
    }

    Free(extFil);
}
/***************************************************************/
static void xpp_free_stream(struct xmls_file * xf,
                            struct xml_stream * xs) {

    if (xs->xf_extFile) {
        xpp_close_extFile(xf, xs->xf_extFile);
    }

    Xml_free_stream(xs);
}
/***************************************************************/
static void xpp_insert_pe(XMLC * peNam,
                          struct xmls_file * xf,
                          struct xml_stream * xs,
                          struct xml_stream * nxs,
                          int depth) {

    struct xt_entity * en;
    struct xt_entity ** pen;
    int eix;
    XMLC xc;
    int state;
    int last;
    struct xml_stream * exs;

    if (depth > XML_MAX_ENTITY_RECUR) {
        Xml_seterr(xf, xs, XMLERR_RECURENTITY);
        return;
    }

    pen = (struct xt_entity **)dbtree_find(xf->xs_p_entities->xe_entity,
                            peNam, xmlcstrbytes(peNam));
    if (!pen) {
        Xml_seterr_S(xf, xs, XMLERR_NOSUCHPE, peNam);
    } else {
        en = *pen;
        exs = 0;
        state = 0;
        last = en->e_len - en->e_ix;
        if (en->e_extID) {
            exs = xpp_open_ext_ID(xf, xs, en->e_extID);
            if (Xml_err(xf)) return;
        } else {
            eix = en->e_ix;
            last = en->e_len - en->e_ix;
        }

        while (state >= 0) {
            if (exs) {
                xc = Xml_get_char(exs);
                if (xc == XMLCEOF) state = -1;
            } else {
                if (eix < last) {
                    xc = en->e_val[eix];
                    eix++;
                } else {
                    state = -1;
                }
            }

            if (!state) {
                if (xc == '%') {
                    if (exs ? XML_IS_NSTART(Xml_peek_char(exs)) :
                              XML_IS_NSTART(en->e_val[eix])) {
                        nxs->xf_tbuflen = 0;
                        state = 1;
                    }
                }
            } else if (state == 1) {
                if (xc == ';') {
                    if (nxs->xf_tbuflen == nxs->xf_tbufsize) {
                        Xml_expand_tbuf(nxs);
                    }
                    nxs->xf_tbuf[nxs->xf_tbuflen] = 0;
#ifdef _DEBUG
    wcstombs(nxs->aatok, nxs->xf_tbuf, sizeof(nxs->aatok));
#endif
                    xpp_insert_pe(nxs->xf_tbuf, xf, xs, nxs, depth + 1);
                    state = 0;
                    xc = 0;
                } else if (XML_IS_NAME(xc)) {
                    if (nxs->xf_tbuflen == nxs->xf_tbufsize) {
                        Xml_expand_tbuf(nxs);
                    }
                    nxs->xf_tbuf[nxs->xf_tbuflen] = xc;
                    nxs->xf_tbuflen++;
                } else {
                    Xml_seterr(xf, xs, XMLERR_BADPEREF);
                }
            }

            if (!state && xc) {
                if (nxs->xf_fbufix + 1 >= nxs->xf_fbufsize) {
                    nxs->xf_fbufsize += XML_K_FBUF_INC;
                    nxs->xf_fbuf = 
                        Realloc(nxs->xf_fbuf,unsigned char,nxs->xf_fbufsize);
                }

                nxs->xf_fbuf[nxs->xf_fbufix] = (unsigned char)(xc >> 8);
                nxs->xf_fbufix++;
                nxs->xf_fbuf[nxs->xf_fbufix] = (unsigned char)(xc & 0xFF);
                nxs->xf_fbufix++;
            }
        }
        if (exs) xpp_free_stream(xf, exs);
    }
}
/***************************************************************/
static struct xml_stream * xpp_new_pe_stream(struct xmls_file * xf,
                                             struct xml_stream * xs) {

    struct xml_stream * nxs;
    XMLC xc;
    XMLC pc;
    XMLC quote;
    XMLC * peNam;
    int done;
    int appch;

    nxs = Xml_new_stream(xf, 0, NULL);
    nxs->xf_encoding = XML_ENC_UNICODE_BE;
    done = xs->xf_iseof;
    quote = 0;
    nxs->xf_fbufix = 0;

    while (!done) {
        xc = Xml_get_char(xs);
        if (xc == XMLCEOF) {
            done = 1;
        } else {
            appch = 1;
            if (nxs->xf_fbufix + 1 >= xs->xf_fbufsize) {
                nxs->xf_fbufsize += XML_K_FBUF_INC;
                nxs->xf_fbuf =
                    Realloc(nxs->xf_fbuf, unsigned char, nxs->xf_fbufsize);
            }

            if (quote) {
                if (xc == quote) quote = 0;
            } else if (xc == '%') {
                pc = Xml_peek_char(xs);
                if (XML_IS_NSTART(pc)) {
                    appch = 0;
                    peNam = Xml_next_token_raw(xs);
                    if (peNam) {
                        xc = Xml_get_char(xs);
                        if (xc != ';') {
                            Xml_seterr(xf, xs, XMLERR_BADPEREF);
                        } else {
                            xpp_insert_pe(peNam, xf, xs, nxs, 0);
                        }
                    }
                }
            } else {
                if (xc == '>' || xc == '<') {
                    appch = 0;
                    done = 1;
                    Xml_unget_char(xs);
                } else {
                    if (xc == '\'' || xc == '\"') quote = xc;
                }
            }

            if (appch) {
                nxs->xf_fbuf[nxs->xf_fbufix] = (unsigned char)(xc >> 8);
                nxs->xf_fbufix++;
                nxs->xf_fbuf[nxs->xf_fbufix] = (unsigned char)(xc & 0xFF);
                nxs->xf_fbufix++;
            }
        }
    }
    nxs->xf_fbuflen = nxs->xf_fbufix;
    nxs->xf_fbufix = 0;

    return (nxs);
}
/***************************************************************/
static struct xml_stream * xpp_pe_stream(XMLC * peNam,
                                            struct xmls_file * xf,
                                            struct xml_stream * xs) {
    struct xml_stream * nxs;
    struct xt_entity * en;
    struct xt_entity ** pen;

    nxs = 0;
    pen = (struct xt_entity **)dbtree_find(xf->xs_p_entities->xe_entity,
                            peNam, xmlcstrbytes(peNam));
    if (!pen) {
        Xml_seterr_S(xf, xs, XMLERR_NOSUCHPE, peNam);
    } else {
        en = *pen;
        if (en->e_extID) {
            nxs = xpp_open_ext_ID(xf, xs, en->e_extID);
            if (Xml_err(xf)) return (0);
        } else {
            nxs = Xml_new_stream(xf, 0, NULL);
            nxs->xf_encoding = XML_ENC_UNICODE_BE;
            xpp_insert_pe(peNam, xf, xs, nxs, 0);
            nxs->xf_fbufix = 0;
            nxs->xf_fbuflen = nxs->xf_fbufix;
            nxs->xf_fbufix = 0;
        }
    }

    return (nxs);
}
/***************************************************************/
static struct xml_stream * xpp_new_pe_value_stream(struct xmls_file * xf,
                                             struct xml_stream * xs) {

    struct xml_stream * nxs;
    XMLC xc;
    XMLC pc;
    XMLC * peNam;

    nxs = 0;

    xc = Xml_get_char(xs);
    if (xc == '%') {
        pc = Xml_peek_char(xs);
        if (XML_IS_NSTART(pc)) {
            peNam = Xml_next_token_raw(xs);
            if (peNam) {
                xc = Xml_get_char(xs);
                if (xc != ';') {
                    Xml_seterr(xf, xs, XMLERR_BADPEREF);
                } else {
                    nxs = xpp_pe_stream(peNam, xf, xs);
                }
            }
        }
    } else {
        Xml_seterr(xf, xs, XMLERR_EXPPEREF);
    }

    return (nxs);
}
/***************************************************************/
static void xpp_VersionInfo(struct xmls_file * xf,
                          struct xml_stream * xs) {
/*
[22]    prolog    ::=    XMLDecl Misc* (doctypedecl Misc*)?
[23]    XMLDecl    ::=    '<?xml' VersionInfo EncodingDecl? SDDecl? S?'?>'
[24]    VersionInfo    ::=
            S 'version' Eq ("'" VersionNum "'" | '"' VersionNum '"')
[25]    Eq    ::=    S? '=' S?
[26]    VersionNum    ::=    '1.1'
*/
    XMLC * tok;

    tok = Xml_next_token_raw(xs);
    if (xmlcstrcmp(tok, "=")) {
        Xml_seterr(xf, xs, XMLERR_NOTXML);
        return;
    }

    tok = Xml_next_token_raw(xs);
    if (!xmlcstrcmp(tok, "\"1.0\"")) {
        strcpy(xf->xv_vers, "1.0");
    } else if (!xmlcstrcmp(tok, "\"1.1\"")) {
        strcpy(xf->xv_vers, "1.1");
    } else {
        Xml_seterr(xf, xs, XMLERR_BADVERS);
        return;
    }
}
/***************************************************************/
static void xpp_EncodingDecl(struct xmls_file * xf,
                          struct xml_stream * xs) {
/*
[80]    EncodingDecl    ::=
            S 'encoding' Eq ('"' EncName '"' | "'" EncName "'" )
[81]    EncName    ::=    [A-Za-z] ([A-Za-z0-9._] | '-')*
*/
    XMLC * tok;

    tok = Xml_next_token_raw(xs);
    if (xmlcstrcmp(tok, "=")) {
        Xml_seterr(xf, xs, XMLERR_NOTXML);
        return;
    }

    tok = Xml_next_token_raw(xs);
    if (!xmlcstrcmp(tok, "\"UTF-8\"")||
        !xmlcstrcmp(tok, "\"utf-8\"") ) {
        if (xs->xf_encoding & 2) {
            Xml_seterr(xf, xs, XMLERR_BADENCMARK);
        } else {
            xs->xf_encoding = XML_ENC_UTF_8;
        }
    } else if (!xmlcistrcmp(tok, "\"ISO-8859-1\"") ||
               !xmlcistrcmp(tok, "\"US-ASCII\"") ) {
        if (xs->xf_encoding & 2) {
            Xml_seterr(xf, xs, XMLERR_BADENCMARK);
        } else {
            xs->xf_encoding = XML_ENC_1_BYTE;
        }
    } else if (!xmlcstrcmp(tok, "\"UTF-16\"") ||
               !xmlcstrcmp(tok, "\"utf-16\"") ) {
        if (!(xs->xf_encoding & 2)) {
            Xml_seterr(xf, xs, XMLERR_BADENCMARK);
        } else {
            /* use autodetect value */
        }
    } else {
        Xml_seterr(xf, xs, XMLERR_UNSUPENCODING);
        return;
    }
}
/***************************************************************/
static void xpp_SDDecl(struct xmls_file * xf,
                          struct xml_stream * xs) {
/*
[32]    SDDecl    ::=
            #x20+ 'standalone' Eq
                (("'" ('yes' | 'no') "'") | ('"' ('yes' | 'no') '"'))
*/
    XMLC * tok;

    tok = Xml_next_token_raw(xs);
    if (xmlcstrcmp(tok, "=")) {
        Xml_seterr(xf, xs, XMLERR_NOTXML);
        return;
    }

    tok = Xml_next_token_raw(xs);
    if (!xmlcstrcmp(tok, "\"yes\"")) {
        xf->xv_standalone = 1;
    } else if (!xmlcstrcmp(tok, "\"no\"")) {
        xf->xv_standalone = 0;
    } else {
        Xml_seterr(xf, xs, XMLERR_BADSTANDALONE);
        return;
    }
}
/***************************************************************/
static void xpp_XMLDecl(struct xmls_file * xf,
                          struct xml_stream * xs) {
/*
[22]    prolog    ::=    XMLDecl Misc* (doctypedecl Misc*)?
[23]    XMLDecl    ::=    '<?xml' VersionInfo EncodingDecl? SDDecl? S?'?>'
[24]    VersionInfo    ::=
                S 'version' Eq ("'" VersionNum "'" | '"' VersionNum '"')
[25]    Eq    ::=    S? '=' S?
[26]    VersionNum    ::=    '1.1'
[27]    Misc    ::=    Comment | PI | S
[32]    SDDecl    ::=
            #x20+ 'standalone' Eq
                (("'" ('yes' | 'no') "'") | ('"' ('yes' | 'no') '"'))
*/
    XMLC * tok;

    tok = Xml_next_token_raw(xs);
    if (xmlcstrcmp(tok, "<")) {
        Xml_seterr(xf, xs, XMLERR_NOTXML);
        return;
    }

    tok = Xml_next_token_raw(xs);
    if (xmlcstrcmp(tok, "?")) {
        Xml_seterr(xf, xs, XMLERR_NOTXML);
        return;
    }

    tok = Xml_next_token_raw(xs);
    if (xmlcstrcmp(tok, "xml")) {
        Xml_seterr(xf, xs, XMLERR_NOTXML);
        return;
    }

    tok = Xml_next_token_raw(xs);
    if (!xmlcstrcmp(tok, "version")) {
        xpp_VersionInfo(xf, xs);
        if (xv_err(xf)) return;
        tok = Xml_next_token_raw(xs);
    }

    if (!xmlcstrcmp(tok, "encoding")) {
        xpp_EncodingDecl(xf, xs);
        if (xv_err(xf)) return;
        tok = Xml_next_token_raw(xs);
    }

    if (!xmlcstrcmp(tok, "standalone")) {
        xpp_SDDecl(xf, xs);
        if (xv_err(xf)) return;
        tok = Xml_next_token_raw(xs);
    }

    if (xmlcstrcmp(tok, "?")) {
        Xml_seterr(xf, xs, XMLERR_NOTXML);
        return;
    }

    tok = Xml_next_token_raw(xs);
    if (xmlcstrcmp(tok, ">")) {
        Xml_seterr(xf, xs, XMLERR_NOTXML);
        return;
    }
}
/***************************************************************/
static XMLC * xpp_get_SystemLiteral(struct xmls_file * xf,
                          struct xml_stream * xs) {

    XMLC * tok;

    tok = Xml_next_token_raw(xs);
    if (!tok){
        Xml_seterr(xf, xs, XMLERR_EXPLITERAL);
        return (0);
    }

    if (!XML_IS_QUOTE(tok[0])) {
        Xml_seterr(xf, xs, XMLERR_EXPLITERAL);
        return (0);
    }

    return xmlcstrdup(tok);
}
/***************************************************************/
static XMLC * xpp_get_PubidLiteral(struct xmls_file * xf,
                          struct xml_stream * xs) {

    int ii;
    int len;
    XMLC * tok;

    tok = Xml_next_token_raw(xs);
    if (!tok){
        Xml_seterr(xf, xs, XMLERR_EXPLITERAL);
        return (0);
    }

    if (!XML_IS_QUOTE(tok[0])) {
        Xml_seterr(xf, xs, XMLERR_EXPLITERAL);
        return (0);
    }

    /* validate string for pubid chars */
    len = xmlclen(tok) - 1;
    for (ii = 1; ii < len; ii++) {
        if (!XML_IS_PUBID(tok[ii])) {
            Xml_seterr(xf, xs, XMLERR_BADPUBID);
            return (0);
        }
    }

    return xmlcstrdup(tok);
}
/***************************************************************/
static XMLC * xpp_get_token_raw(struct xml_stream * xs) {

    XMLC *tok;

    tok = Xml_next_token_raw(xs);
    if (!tok) return (0);

    return xmlcstrdup(tok);
}
/***************************************************************/
static XMLC * xpp_next_Name(struct xmls_file * xf,
                           struct xml_stream * xs) {

    int ii;
    int len;
    XMLC *tok;

    tok = Xml_next_token_raw(xs);
    if (!tok) return (0);

    if (!XML_IS_NSTART(tok[0])) {
        Xml_seterr_S(xf, xs, XMLERR_BADNAME, tok);
        return (0);
    }

    /* validate string for pubid chars */
    len = xmlclen(tok);
    for (ii = 1; ii < len; ii++) {
        if (!XML_IS_NAME(tok[ii])) {
            Xml_seterr_S(xf, xs, XMLERR_BADNAME, tok);
            return (0);
        }
    }

    return (tok);
}
/***************************************************************/
static XMLC * xpp_get_Name(struct xmls_file * xf,
                      struct xml_stream * xs) {

    XMLC *nam;

    nam = xpp_next_Name(xf, xs);
    if (!nam) return (0);

    return xmlcstrdup(nam);
}
/***************************************************************/
static XMLC * xpp_get_NMToken(struct xmls_file * xf,
                      struct xml_stream * xs) {

    int ii;
    int len;
    XMLC *tok;

    tok = Xml_next_token_raw(xs);
    if (!tok) return (0);

    /* validate string for pubid chars */
    len = xmlclen(tok);
    for (ii = 0; ii < len; ii++) {
        if (!XML_IS_NAME(tok[ii])) {
            Xml_seterr_S(xf, xs, XMLERR_BADNAME, tok);
            return (0);
        }
    }

    return xmlcstrdup(tok);
}
/***************************************************************/
static struct xt_extID * xpp_ExternalID(struct xmls_file * xf,
                          struct xml_stream * xs,
                          int pubSysReq) {
/*
[75]    ExternalID    ::=    'SYSTEM' S SystemLiteral
                           | 'PUBLIC' S PubidLiteral S SystemLiteral
*/

    XMLC * tok;
    XMLC * slit;
    XMLC * plit;
    struct xt_extID * extID;
    int xtyp;
    XMLC pc;

    slit = 0;
    plit = 0;
    xtyp = 0;

    tok = Xml_next_token_raw(xs);
    if (!tok) return (0);

    if (!xmlcstrcmp(tok, "SYSTEM")) {
        slit = xpp_get_SystemLiteral(xf, xs);
        if (xv_err(xf)) return (0);
        xtyp = EXTYP_SYSTEM;
    } else if (!xmlcstrcmp(tok, "PUBLIC")) {
        plit = xpp_get_PubidLiteral(xf, xs);
        if (xv_err(xf)) return (0);
        xtyp = EXTYP_PUBLIC;

        pc = Xml_peek_nwchar(xs);
        if (XML_IS_QUOTE(pc)) {
            slit = xpp_get_SystemLiteral(xf, xs);
            if (xv_err(xf)) return (0);
        } else if (pubSysReq) {
            Free(plit);
            Xml_seterr(xf, xs, XMLERR_EXPPUBLIT);
            return (0);
        }
    } else {
        Xml_seterr(xf, xs, XMLERR_EXPSYSPUB);
        return (0);
    }

    extID = New(struct xt_extID, 1);
    extID->x_typ   = xtyp;
    extID->x_extID = slit;
    extID->x_pubID = plit;

    return (extID);
}
/***************************************************************/
static void xpp_PEReference(struct xmls_file * xf,
                          struct xml_stream * xs) {
/*
[69]    PEReference    ::=    '%' Name ';'
*/
    XMLC * PEName;

    Xml_get_char(xs); /* % */

    PEName = xpp_get_Name(xf, xs);
    if (!PEName) return;

    if (Xml_peek_nwchar(xs) == ';') {
        /* Do something with parameter entity here */
    } else {
        Xml_seterr(xf, xs, XMLERR_BADPEREF);
        return;
    }

    Free(PEName);
}
/***************************************************************/
static struct xt_attlist * xpp_new_attlist(XMLC * iName) {

    struct xt_attlist * att;

    att = New(struct xt_attlist, 1);
    att->a_elname = iName;
    att->a_attdefs = dbtree_new();

    return (att);
}
/***************************************************************/
static struct xt_attdef * xpp_new_attdef(XMLC * iName) {

    struct xt_attdef * ad;

    ad = New(struct xt_attdef, 1);
    ad->a_attname       = iName;
    ad->a_ref_flag      = 0;
    ad->a_atttyp        = 0;
    ad->a_defaultval    = NULL;
    ad->a_defaulttyp    = 0;
    ad->a_elist         = NULL;

    return (ad);
}
/***************************************************************/
static int xpp_AttType(XMLC * attName) {

    int typ;

         if (!xmlcstrcmp(attName, "CDATA"))     typ = ATTTYP_CDATA;
    else if (!xmlcstrcmp(attName, "("))         typ = ATTTYP_ENUM;
    else if (!xmlcstrcmp(attName, "NOTATION"))  typ = ATTTYP_NOTATION;
    else if (!xmlcstrcmp(attName, "ID"))        typ = ATTTYP_ID;
    else if (!xmlcstrcmp(attName, "IDREF"))     typ = ATTTYP_IDREF;
    else if (!xmlcstrcmp(attName, "IDREFS"))    typ = ATTTYP_IDREFS;
    else if (!xmlcstrcmp(attName, "ENTITY"))    typ = ATTTYP_ENTITY;
    else if (!xmlcstrcmp(attName, "ENTITIES"))  typ = ATTTYP_ENTITIES;
    else if (!xmlcstrcmp(attName, "NMTOKEN"))   typ = ATTTYP_NMTOKEN;
    else if (!xmlcstrcmp(attName, "NMTOKENS"))  typ = ATTTYP_NMTOKENS;
    else                                        typ = 0;

    return (typ);
}
/***************************************************************/
static void xpp_add_attlist(struct xt_attlist * att,
                            struct xt_attdef * ad) {

    int dup;

    dup = dbtree_insert(att->a_attdefs,
        ad->a_attname, xmlcstrbytes(ad->a_attname), ad);

    if (dup) {
        Xml_free_attdef(ad);
    }
}
/***************************************************************/
static void xpp_add_attdef_list(struct xt_attdef * ad,
                                XMLC * nm) {

    struct xt_eitem * ei;

    if (!ad->a_elist) {
        ad->a_elist = dbtree_new();
    }

    ei = New(struct xt_eitem, 1);
    ei->e_name = nm;
    dbtree_insert(ad->a_elist, nm, xmlcstrbytes(nm), ei);
}
/***************************************************************/
static void xpp_Enumeration(struct xmls_file * xf,
                          struct xml_stream * xs,
                          struct xt_attdef * ad) {

    XMLC * nm;
    int done;
    XMLC pc;

    done = 0;
    while (!done) {
        nm = xpp_get_NMToken(xf, xs);
        if (!nm) return;

        xpp_add_attdef_list(ad, nm);
        pc = Xml_peek_nwchar(xs);
        if (pc == ')') {
            Xml_get_char(xs); /* ) */
            done = 1;
        } else if (pc == '|') {
            Xml_get_char(xs); /* | */
        } else {
            Xml_seterr_C(xf, xs, XMLERR_EXPBAR, pc);
            done = 1;
        }
    }
}
/***************************************************************/
static void xpp_NotationType(struct xmls_file * xf,
                          struct xml_stream * xs,
                          struct xt_attdef * ad) {

    XMLC * nm;
    int done;
    XMLC pc;

    done = 0;

    pc = Xml_get_nwchar(xs);
    if (pc != '(') {
        Xml_seterr_C(xf, xs, XMLERR_EXPPAREN, pc);
        done = 1;
    }

    while (!done) {
        nm = xpp_get_Name(xf, xs);
        if (!nm) return;

        xpp_add_attdef_list(ad, nm);
        pc = Xml_peek_nwchar(xs);
        if (pc == ')') {
            Xml_get_char(xs); /* ) */
            done = 1;
        } else if (pc == '|') {
            Xml_get_char(xs); /* | */
        } else {
            Xml_seterr_C(xf, xs, XMLERR_EXPBAR, pc);
            done = 1;
        }
    }
}
/***************************************************************/
static void xpp_AttValue(struct xmls_file * xf,
                          struct xml_stream * xs,
                          struct xt_attdef * ad) {
/*
[10]    AttValue    ::=    '"' ([^<&"] | Reference)* '"'
*/
    int len;
    XMLC *tok;

    tok = Xml_next_token_raw(xs);
    if (!tok) return;
    len = xmlclen(tok);

    if (len < 2 || !XML_IS_QUOTE(tok[0]) || tok[0] != tok[len-1]) {
        Xml_seterr(xf, xs, XMLERR_EXPLITERAL);
        return;
    }

    ad->a_defaultval = xmlcstrdup(tok);
}
/***************************************************************/
static void xpp_DefaultDecl(struct xmls_file * xf,
                          struct xml_stream * xs,
                          struct xt_attdef * ad) {
/*
[60]    DefaultDecl    ::=    '#REQUIRED' | '#IMPLIED'
                                   | (('#FIXED' S)? AttValue)

[10]    AttValue    ::=    '"' ([^<&"] | Reference)* '"'
*/
    XMLC pc;
    XMLC * tok;

    pc = Xml_peek_nwchar(xs);
    if (pc == '#') {
        Xml_get_char(xs); /* # */
        tok = Xml_next_token_raw(xs);
        if (!xmlcstrcmp(tok, "REQUIRED")) {
            ad->a_defaulttyp = ATTDEFTYP_REQUIRED;
        } else if (!xmlcstrcmp(tok, "IMPLIED")) {
            ad->a_defaulttyp = ATTDEFTYP_IMPLIED;
        } else if (!xmlcstrcmp(tok, "FIXED")) {
            ad->a_defaulttyp = ATTDEFTYP_FIXED;
            xpp_AttValue(xf, xs, ad);
        } else {
            Xml_seterr_S(xf, xs, XMLERR_EXPATTDEF, tok);
        }
    } else if (pc == '\'' || pc == '"') {
        ad->a_defaulttyp = ATTDEFTYP_VAL;
        xpp_AttValue(xf, xs, ad);
    } else {
        Xml_seterr_C(xf, xs, XMLERR_EXPATTVAL, pc);
    }
}
/***************************************************************/
static void xpp_AttlistDecl(struct xmls_file * xf,
                          struct xml_stream * xs) {
/*
[52]    AttlistDecl    ::=    '<!ATTLIST' S Name AttDef* S? '>'
[53]    AttDef    ::=    S Name S AttType S DefaultDecl
[54]    AttType    ::=    StringType | TokenizedType | EnumeratedType
[55]    StringType    ::=    'CDATA'
[56]    TokenizedType    ::=    'ID'
                               | 'IDREF'
                               | 'IDREFS'
                               | 'ENTITY'
                               | 'ENTITIES'
                               | 'NMTOKEN'
                               | 'NMTOKENS'

[57]    EnumeratedType    ::=    NotationType | Enumeration
[58]    NotationType    ::=    'NOTATION' S '(' S? Name (S? '|' S? Name)* S? ')'
[59]    Enumeration    ::=    '(' S? Nmtoken (S? '|' S? Nmtoken)* S? ')'
[60]    DefaultDecl    ::=    '#REQUIRED' | '#IMPLIED'
                                   | (('#FIXED' S)? AttValue)
*/
    XMLC * elName;
    XMLC * attName;
    XMLC * tok;
    struct xt_attlist * att;
    struct xt_attlist ** patt;
    struct xt_attdef * ad;
    int done;
    XMLC   pc;

    elName = xpp_get_Name(xf, xs);
    if (!elName) return;

    patt = (struct xt_attlist **)dbtree_find(xf->xs_attlists->xa_attlist,
                elName, xmlcstrbytes(elName));
    if (!patt) {
        att = xpp_new_attlist(elName);

        dbtree_insert(xf->xs_attlists->xa_attlist,
            elName, xmlcstrbytes(elName), att);
    } else {
        Free(elName);
        att = *patt;
    }

    done = 0;
    while (!done) {
        pc = Xml_peek_nwchar(xs);
        if (!pc || pc == '>') {
            done = 1;
        } else {
            attName = xpp_get_Name(xf, xs);
            if (!attName) {
                done = 1;
                Xml_free_attlist(att);
            } else {
                ad = xpp_new_attdef(attName);

                tok = Xml_next_token_raw(xs);
                ad->a_atttyp = xpp_AttType(tok);
                if (!ad->a_atttyp) {
                    Xml_seterr(xf, xs, XMLERR_EXPATTTYP);
                } else if (ad->a_atttyp == ATTTYP_ENUM) {
                    xpp_Enumeration(xf, xs, ad);
                } else if (ad->a_atttyp == ATTTYP_NOTATION) {
                    xpp_NotationType(xf, xs, ad);
                }

                if (!xv_err(xf)) {
                    xpp_DefaultDecl(xf, xs, ad);
                }
                if (xv_err(xf)) {
                    Xml_free_attdef(ad);
                    done = 1;
                } else {
                    xpp_add_attlist(att, ad);
                }
            }
        }
    }
}
/***************************************************************/
#if 0
static char * xpp_get_stream(struct xml_stream * xs) {

   char * buf;
   int ix;
   int bufsize;
   int bufix;
   char ch;

    ix = xs->xf_fbufix;
    if (xs->xf_tadv) ix++;

    buf = 0;
    bufsize = 0;
    bufix = 0;

    while (ix < xs->xf_fbuflen) {
        if (xs->xf_encoding == XML_ENC_UNICODE_BE) {
            ix++;
            if (ix < xs->xf_fbuflen) ch = xs->xf_fbuf[ix];
            else ch = 0;
            ix++;
        } else if (xs->xf_encoding == XML_ENC_UNICODE_LE) {
            ch = xs->xf_fbuf[ix];
            ix += 2;
        } else {
            ch = xs->xf_fbuf[ix];
            ix++;
        }

        if (bufix == bufsize) {
            bufsize += 128;
            buf = Realloc(buf, char, bufsize);
        }
        buf[bufix] = ch;
        bufix++;
    }
    if (bufix == bufsize) {
        bufsize += 16;
        buf = Realloc(buf, char, bufsize);
    }
    buf[bufix] = 0;

    return (buf);
}
#endif
/***************************************************************/
static void xpp_AttlistDecl_PE(struct xmls_file * xf,
                              struct xml_stream * xs) {

    struct xml_stream * nxs;

    nxs = xpp_new_pe_stream(xf, xs);
    if (xv_err(xf)) return;

    xpp_AttlistDecl(xf, nxs);

    xpp_free_stream(xf, nxs);
}
/***************************************************************/
static struct xt_eltree * xpp_new_element_node(XMLC * iName) {

    struct xt_eltree * elt;

    elt = New(struct xt_eltree, 1);
    elt->lt_op     = iName;
    elt->lt_pcdata = 0;
    elt->lt_repeat = 0;
    elt->lt_nnodes = 0;
    elt->lt_node   = NULL;

    return (elt);
}
/***************************************************************/
static struct xt_eltree * xpp_new_element_op(struct xt_eltree * et,
                            XMLC * opName) {

    struct xt_eltree * elt;

    elt = New(struct xt_eltree, 1);
    elt->lt_op      = opName;
    elt->lt_pcdata  = 0;
    elt->lt_repeat  = 0;
    elt->lt_nnodes  = 1;
    elt->lt_node    = New(struct xt_eltree*, 1);
    elt->lt_node[0] = et;

    return (elt);
}
/***************************************************************/
static void xpp_add_element_node(struct xt_eltree * elt,
                                 struct xt_eltree * nelt) {

    elt->lt_node =
            Realloc(elt->lt_node, struct xt_eltree*, elt->lt_nnodes + 1);
    elt->lt_node[elt->lt_nnodes] = nelt;
    elt->lt_nnodes++;
}
/***************************************************************/
static void xpp_add_first_element_node(struct xt_eltree * elt,
                                 struct xt_eltree * nelt) {

    int ii;

    elt->lt_node =
            Realloc(elt->lt_node, struct xt_eltree*, elt->lt_nnodes + 1);

    for (ii = elt->lt_nnodes; ii > 0; ii--) {
        elt->lt_node[ii] = elt->lt_node[ii - 1];
    }

    elt->lt_node[0] = nelt;

    elt->lt_nnodes++;
}
/***************************************************************/
static struct xt_eltree * xpp_Mixed(struct xmls_file * xf,
                          struct xml_stream * xs,
                          XMLC * iName) {
/*
Improved:
    Mixed       ::= '(' S? '#PCDATA' S? ')' |
                    '(' S? '#PCDATA' S? '|' MixedList S? ')*'

    MixedList   ::= S? Name | S? Name S? '|' MixedList
*/
    int done;
    XMLC pc;
    XMLC * opName;
    XMLC * mName;
    struct xt_eltree * et = 0;
    struct xt_eltree * nelt;

    pc = Xml_peek_nwchar(xs);

    if (pc == ')') {
        et = xpp_new_element_node(iName); /* #PCDATA */
        et->lt_pcdata = 1;
        Xml_get_char(xs); /* ) */
    } else if (pc != '|') {
        Xml_seterr(xf, xs, XMLERR_EXPBAR);
    } else {
        opName = xpp_get_token_raw(xs); /* | */
        et = xpp_new_element_node(opName);
        et->lt_pcdata = 1;
        nelt = xpp_new_element_node(iName);
        xpp_add_element_node(et, nelt); /* #PCDATA */
        done = 0;
        while (!done) {
            mName = xpp_get_Name(xf, xs);
            if (!mName) return (0);
            nelt = xpp_new_element_node(mName);
            xpp_add_element_node(et, nelt);
            pc = Xml_peek_nwchar(xs);
            if (pc == ')') {
                Xml_get_char(xs); /* ) */
                pc = Xml_peek_char(xs); /* * */
                if (pc != '*') {
                    Xml_seterr(xf, xs, XMLERR_EXPSTAR);
                } else {
                    et->lt_repeat = pc;
                    Xml_get_char(xs); /* * */
                }
                done = 1;
            } else if (pc == '|') {
                Xml_get_char(xs);
            } else {
                Xml_seterr(xf, xs, XMLERR_EXPBAR);
                done = 1;
            }
        }
    }

    return (et);
}
/***************************************************************/
static struct xt_eltree * xpp_children(struct xmls_file * xf,
                          struct xml_stream * xs,
                          XMLC * iName) {
/*
Improved:
    children    ::= choiceSeq | choiceSeq repeater

    choiceSeq   ::= '(' S? cp S? ')' | '(' S? cp S? '|' choiceList S? ')' |
                    '(' S? cp S? ',' seqList S? ')'

    repeater    ::= '?' | '*' | '+'

    choiceList  ::= S? cp | S? cp '|' choiceList

    cp          ::= NmChoiceSeq | NmChoiceSeq repeater

    NmChoiceSeq ::= Name | choiceSeq

    seqList     ::= S? cp | S? cp ',' seqList
*/
    int done;
    XMLC pc;
    XMLC op;
    XMLC * opName;
    XMLC * mName;
    struct xt_eltree * et = 0;
    struct xt_eltree * nelt;

    if (!xmlcstrcmp(iName, "(")) {
        Free(iName);
        mName = xpp_get_token_raw(xs); /* ( */
        et = xpp_children(xf, xs, mName);
    } else {
        et = xpp_new_element_node(iName); /* Name */
    }

    pc = Xml_peek_nwchar(xs);
    if (pc == '*' || pc == '+' || pc == '?') {
        et->lt_repeat = pc;
        Xml_get_char(xs); /* rpt */
        pc = Xml_peek_nwchar(xs);
    }

    if (pc == ')') {
        Xml_get_char(xs); /* ) */
        pc = Xml_peek_char(xs);
        if (pc == '*' || pc == '+' || pc == '?') {
            et->lt_repeat = pc;
            Xml_get_char(xs); /* rpt */
        }
    } else if (pc == '|' || pc == ',') {
        op = pc;
        opName = xpp_get_token_raw(xs);
        et = xpp_new_element_op(et, opName);
        done = 0;
        while (!done) {
            pc = Xml_peek_nwchar(xs);
            if (pc == '(') {
                Xml_get_char(xs); /* ( */
                mName = xpp_get_token_raw(xs); /* , */
                nelt = xpp_children(xf, xs, mName);
            } else {
                mName = xpp_get_Name(xf, xs); /* , */
                nelt = xpp_new_element_node(mName);
                pc = Xml_peek_nwchar(xs);
                if (pc == '*' || pc == '+' || pc == '?') {
                    nelt->lt_repeat = pc;
                    Xml_get_char(xs); /* rpt */
                }
            }
            xpp_add_element_node(et, nelt);
            pc = Xml_peek_nwchar(xs);
            if (pc == ')') {
                Xml_get_char(xs); /* ) */
                pc = Xml_peek_char(xs);
                if (pc == '*' || pc == '+' || pc == '?') {
                    et->lt_repeat = pc;
                    Xml_get_char(xs); /* rpt */
                }
                done = 1;
            } else if (pc == op) {
                Xml_get_char(xs);
            } else {
                Xml_seterr(xf, xs, XMLERR_EXPBAR);
                done = 1;
            }
        }
    } else {
        Xml_seterr(xf, xs, XMLERR_EXPSEP);
    }

    return (et);
}
/***************************************************************/
static struct xt_eltree * xpp_Mixed_children(struct xmls_file * xf,
                          struct xml_stream * xs) {
/*
Original:
[46]    contentspec ::=    'EMPTY' | 'ANY' | Mixed | children
[47]    children    ::=    (choice | seq) ('?' | '*' | '+')?
[48]    cp          ::=    (Name | choice | seq) ('?' | '*' | '+')?
[49]    choice      ::=    '(' S? cp ( S? '|' S? cp )+ S? ')'
[50]    seq         ::=    '(' S? cp ( S? ',' S? cp )* S? ')'
[51]    Mixed       ::=    '(' S? '#PCDATA' (S? '|' S? Name)* S? ')*'
                           | '(' S? '#PCDATA' S? ')'

Improved:
    Mixed       ::= '(' S? '#PCDATA' S? ')' |
                    '(' S? '#PCDATA' S? '|' MixedList S? ')*'

    MixedList   ::= S? Name | S? Name S? '|' MixedList

    children    ::= choiceSeq | choiceSeq repeater

    choiceSeq   ::= '(' S? cp S? ')' | '(' S? cp S? '|' choiceList S? ')' |
                    '(' S? cp S? ',' seqList S? ')'

    repeater    ::= '?' | '*' | '+'

    choiceList  ::= S? cp | S? cp '|' choiceList

    cp          ::= NmChoiceSeq | NmChoiceSeq repeater

    NmChoiceSeq ::= Name | choiceSeq

    seqList     ::= S? cp | S? cp ',' seqList


*/
    XMLC * iName;
    struct xt_eltree * et = 0;
    XMLC pc;

    pc = Xml_peek_nwchar(xs);
    if (pc == '#') {
        Xml_get_char(xs); /* # */
        iName = Xml_next_token_raw(xs);
        if (!xmlcstrcmp(iName, "PCDATA")) {
            iName = xmlcstrdups("#PCDATA");
            et = xpp_Mixed(xf, xs, iName);
        } else {
            Xml_seterr(xf, xs, XMLERR_EXPPCDATA);
        }
    } else if (pc == '(') {
        iName = xpp_get_token_raw(xs); /* ( */
        if (iName) {
            et = xpp_children(xf, xs, iName);
        }
    } else {
        iName = xpp_get_Name(xf, xs);
        if (iName) {
            et = xpp_children(xf, xs, iName);
        }
    }

    return (et);
}
/***************************************************************/
static void xpp_add_element(struct xt_elements * elements,
                            struct xt_element * el,
                            XMLC * elName) {

    int dup;

    dup = dbtree_insert(elements->xl_elements,
            elName, xmlcstrbytes(elName), el);

    if (dup) {
        Xml_free_element(el);
    }
}
/***************************************************************/
static void xpp_elementdecl(struct xmls_file * xf,
                          struct xml_stream * xs) {
/*
[45]    elementdecl    ::=    '<!ELEMENT' S Name S contentspec S? '>'
[46]    contentspec    ::=    'EMPTY' | 'ANY' | Mixed | children
                           | '(' S? '#PCDATA' S? ')'
*/
    XMLC * elName;
    XMLC * tok;
    XMLC pc;
    struct xt_element * el = 0;

    elName = xpp_get_Name(xf, xs);
    if (!elName) return;

    pc = Xml_peek_nwchar(xs);

    if (XML_IS_ALPHA(pc)) {
        tok = Xml_next_token_raw(xs);
        if (!xmlcstrcmp(tok, "ANY")) {
            el = Xml_new_element(elName, ELTYP_ANY);
        } else if (!xmlcstrcmp(tok, "EMPTY")) {
            el = Xml_new_element(elName, ELTYP_EMPTY);
        } else {
            Xml_seterr(xf, xs, XMLERR_EXPANYEMPTY);
            return;
        }

        xpp_add_element(xf->xs_elements, el, elName);
    } else if (pc == '(') {
        Xml_get_char(xs); /* ( */
        el = Xml_new_element(elName, ELTYP_TREE);
        el->l_tree = xpp_Mixed_children(xf, xs);
        if (!xv_err(xf)) {
            xpp_add_element(xf->xs_elements, el, elName);
        }

        if (xv_err(xf)) {
            Xml_free_element(el);
            el = 0;
        }
    } else {
        Xml_seterr(xf, xs, XMLERR_EXPPAREN);
    }
}
/***************************************************************/
static void xpp_elementdecl_PE(struct xmls_file * xf,
                               struct xml_stream * xs) {

    struct xml_stream * nxs;

    nxs = xpp_new_pe_stream(xf, xs);
    if (xv_err(xf)) return;

    xpp_elementdecl(xf, nxs);

    xpp_free_stream(xf, nxs);
}
/***************************************************************/
static XMLC * xpp_get_EntityValue(struct xmls_file * xf,
                                  struct xml_stream * xs) {
/*
[9]    EntityValue    ::=    '"' ([^%&"] | PEReference | Reference)* '"'
   |  "'" ([^%&'] | PEReference | Reference)* "'"
*/
    int len;
    XMLC *tok;

    tok = Xml_next_token_raw(xs);
    if (!tok) return (0);
    len = xmlclen(tok);

    if (len < 2 || !XML_IS_QUOTE(tok[0]) || tok[0] != tok[len-1]) {
        Xml_seterr(xf, xs, XMLERR_EXPLITERAL);
        return (0);
    }

    return xmlcstrdup(tok);
}
/***************************************************************/
static XMLC * xpp_NDataDecl(struct xmls_file * xf,
                          struct xml_stream * xs) {
/*
[76]    NDataDecl    ::=    S 'NDATA' S Name
*/

    XMLC * tok;
    XMLC * ndName;

    tok = Xml_next_token_raw(xs);
    if (!tok) return (0);

    if (!xmlcstrcmp(tok, "NDATA")) {
        ndName = xpp_get_Name(xf, xs);
        if (!ndName) return (0);
    } else {
        Xml_seterr(xf, xs, XMLERR_EXPNDATA);
        return (0);
    }

    return (ndName);
}
/***************************************************************/
static void xpp_EntityDef(struct xmls_file * xf,
                          struct xml_stream * xs,
                          XMLC * eName) {
/*
[73]    EntityDef    ::=    EntityValue| (ExternalID NDataDecl?)
[9]    EntityValue    ::=    '"' ([^%&"] | PEReference | Reference)* '"'
   |  "'" ([^%&'] | PEReference | Reference)* "'"
[67]    Reference    ::=    EntityRef | CharRef
[68]    EntityRef    ::=    '&' Name ';'
[66]    CharRef      ::=    '&#' [0-9]+ ';'  | '&#x' [0-9a-fA-F]+ ';'
[76]    NDataDecl    ::=    S 'NDATA' S Name
*/
    XMLC * eValue;
    XMLC * eNdata;
    XMLC pc;
    struct xt_extID * extID;

    extID  = 0;
    eNdata = 0;
    eValue = 0;

    pc = Xml_peek_nwchar(xs);

    if (XML_IS_ALPHA(pc)) {
        extID = xpp_ExternalID(xf, xs, 1);
        if (xv_err(xf)) return;

        pc = Xml_peek_nwchar(xs);
        if (XML_IS_ALPHA(pc)) {
            eNdata = xpp_NDataDecl(xf, xs);
            if (xv_err(xf)) return;
        }
    } else {
        eValue = xpp_get_EntityValue(xf, xs);
        if (xv_err(xf)) return;
    }

    Xml_insert_entity(xf->xs_g_entities, ENTYP_GENERAL,
            eName, eValue, extID, eNdata);
}
/***************************************************************/
static void xpp_PEDef(struct xmls_file * xf,
                          struct xml_stream * xs,
                          XMLC * eName) {
/*
[74]    PEDef    ::=    EntityValue | ExternalID
[9]    EntityValue    ::=    '"' ([^%&"] | PEReference | Reference)* '"'
   |  "'" ([^%&'] | PEReference | Reference)* "'"
[75]    ExternalID    ::=    'SYSTEM' S SystemLiteral
               | 'PUBLIC' S PubidLiteral S SystemLiteral
*/
    XMLC * eValue;
    XMLC * eNdata;
    XMLC pc;
    struct xt_extID * extID;

    extID  = 0;
    eNdata = 0;
    eValue = 0;

    pc = Xml_peek_nwchar(xs);

    if (XML_IS_ALPHA(pc)) {
        extID = xpp_ExternalID(xf, xs, 1);
        if (xv_err(xf)) return;
    } else {
        eValue = xpp_get_EntityValue(xf, xs);
        if (xv_err(xf)) return;
    }

    Xml_insert_entity(xf->xs_p_entities, ENTYP_PARM,
            eName, eValue, extID, 0);
}
/***************************************************************/
static void xpp_EntityDecl(struct xmls_file * xf,
                          struct xml_stream * xs) {
/*
[70]    EntityDecl    ::=    GEDecl | PEDecl
[71]    GEDecl    ::=    '<!ENTITY' S Name S EntityDef S? '>'
[72]    PEDecl    ::=    '<!ENTITY' S '%' S Name S PEDef S? '>'
[73]    EntityDef    ::=    EntityValue| (ExternalID NDataDecl?)
[74]    PEDef    ::=    EntityValue | ExternalID
[9]    EntityValue    ::=    '"' ([^%&"] | PEReference | Reference)* '"'
   |  "'" ([^%&'] | PEReference | Reference)* "'"
[67]    Reference    ::=    EntityRef | CharRef
[68]    EntityRef    ::=    '&' Name ';'
[66]    CharRef      ::=    '&#' [0-9]+ ';'  | '&#x' [0-9a-fA-F]+ ';'

[76]    NDataDecl    ::=    S 'NDATA' S Name   [Name is a notation name]

*/
    XMLC * eName;

    eName = 0;

    if (Xml_peek_nwchar(xs) == '%') {
        Xml_get_char(xs); /* % */

        eName = xpp_get_Name(xf, xs);
        if (!eName) return;

        xpp_PEDef(xf, xs, eName);
        if (xv_err(xf)) return;
    } else {
        eName = xpp_get_Name(xf, xs);
        if (!eName) return;

        xpp_EntityDef(xf, xs, eName);
        if (xv_err(xf)) return;
    }
}
/***************************************************************/
static void xpp_EntityDecl_PE(struct xmls_file * xf,
                               struct xml_stream * xs) {

    struct xml_stream * nxs;

    nxs = xpp_new_pe_stream(xf, xs);
    if (xv_err(xf)) return;

    xpp_EntityDecl(xf, nxs);

    xpp_free_stream(xf, nxs);
}
/***************************************************************/
static struct xt_notation * xpp_get_notation(struct xmls_file * xf,
                          struct xml_stream * xs,
                          XMLC * nName) {
/*
[75]    ExternalID    ::=    'SYSTEM' S SystemLiteral
                           | 'PUBLIC' S PubidLiteral S SystemLiteral
[82]    NotationDecl ::= '<!NOTATION' S Name S (ExternalID | PublicID) S? '>'
[83]    PublicID     ::= 'PUBLIC' S PubidLiteral
*/

    struct xt_notation  * not;
    struct xt_extID  * extID;

    extID = xpp_ExternalID(xf, xs, 0);
    if (xv_err(xf)) return (0);

    not = New(struct xt_notation, 1);
    not->n_notname = nName;
    not->n_extID   = extID;

    return (not);
}
/***************************************************************/
static void xpp_NotationDecl(struct xmls_file * xf,
                          struct xml_stream * xs) {
/*
[82]    NotationDecl ::= '<!NOTATION' S Name S (ExternalID | PublicID) S? '>'
[83]    PublicID     ::= 'PUBLIC' S PubidLiteral
*/
    XMLC * nName;
    struct xt_notation  * not;
    struct xt_notation  ** pnot;

    nName = xpp_get_Name(xf, xs);
    if (!nName) return;

    not = xpp_get_notation(xf, xs, nName);
    if (!not) return;

    pnot = (struct xt_notation **)dbtree_find(xf->xs_notations->xn_notations,
                            nName,  xmlcstrbytes(nName));
    if (!pnot) {
        dbtree_insert(xf->xs_notations->xn_notations,
                                nName,  xmlcstrbytes(nName), not);
    } else {
        Xml_free_notation(not);
    }
}
/***************************************************************/
static void xpp_NotationDecl_PE(struct xmls_file * xf,
                               struct xml_stream * xs) {

    struct xml_stream * nxs;

    nxs = xpp_new_pe_stream(xf, xs);
    if (xv_err(xf)) return;

    xpp_NotationDecl(xf, nxs);

    xpp_free_stream(xf, nxs);
}
/***************************************************************/
static void xpp_Comment(struct xmls_file * xf,
                          struct xml_stream * xs) {
/*
[15]    Comment    ::=    '<!--' ((Char - '-') | ('-' (Char - '-')))* '-->'
*/
    int done;
    XMLC xc;
    int nm;

    done = 0;
    nm   = 0;
    while (!done) {
        xc = Xml_get_char(xs);
        if (xc == XMLCEOF) {
            done = 1;
        } else if (xc == '-') {
            nm++;
        } else if (xc == '>') {
            if (nm >= 2) done = 1;
            else         nm   = 0;
        } else {
            nm = 0;
        }
    }
    Xml_unget_char(xs);
}
/***************************************************************/
static void xpp_PI(struct xmls_file * xf,
                          struct xml_stream * xs) {
/*
[16]    PI    ::=    '<?' PITarget (S (Char* - (Char* '?>' Char*)))? '?>'
[17]    PITarget    ::=    Name - (('X' | 'x') ('M' | 'm') ('L' | 'l'))
*/
    int done;
    XMLC xc;
    int nm;

    done = 0;
    nm   = 0;
    while (!done) {
        xc = Xml_get_char(xs);
        if (xc == XMLCEOF) {
            done = 1;
        } else if (xc == '?') {
            nm++;
        } else if (xc == '>') {
            if (nm) done = 1;
            else    nm   = 0;
        } else {
            nm = 0;
        }
    }
}
/***************************************************************/
static void xpp_markupdecl(struct xmls_file * xf,
                          struct xml_stream * xs) {
/*
[29]    markupdecl    ::=    elementdecl | AttlistDecl | EntityDecl |
                                NotationDecl | PI | Comment
[45]    elementdecl    ::=    '<!ELEMENT' S Name S contentspec S? '>'
[52]    AttlistDecl    ::=    '<!ATTLIST' S Name AttDef* S? '>'
[70]    EntityDecl    ::=    GEDecl | PEDecl
[82]    NotationDecl    ::=
            '<!NOTATION' S Name S (ExternalID | PublicID) S? '>'
[16]    PI    ::=    '<?' PITarget (S (Char* - (Char* '?>' Char*)))? '?>'
[15]    Comment    ::=    '<!--' ((Char - '-') | ('-' (Char - '-')))* '-->'
*/
    XMLC * tok;

    tok = Xml_next_token_raw(xs);
    if (!tok) return;

    if (!xmlcstrcmp(tok, "ATTLIST")) {
        xpp_AttlistDecl_PE(xf, xs);
        if (xv_err(xf)) return;
    } else if (!xmlcstrcmp(tok, "ELEMENT")) {
        xpp_elementdecl_PE(xf, xs);
        if (xv_err(xf)) return;
    } else if (!xmlcstrcmp(tok, "ENTITY")) {
        xpp_EntityDecl_PE(xf, xs);
        if (xv_err(xf)) return;
    } else if (!xmlcstrcmp(tok, "NOTATION")) {
        xpp_NotationDecl_PE(xf, xs);
        if (xv_err(xf)) return;
    } else if (tok[0] == '-') {
        xpp_Comment(xf, xs);
        if (xv_err(xf)) return;
    } else {
        Xml_seterr_S(xf, xs, XMLERR_EXPMARKUP, tok);
        return;
    }
}
/***************************************************************/
static void xpp_intSubset(struct xmls_file * xf,
                          struct xml_stream * xs) {
/*
[28a]    DeclSep    ::=    PEReference | S
[28b]    intSubset    ::=    (markupdecl | DeclSep)*
[29]    markupdecl    ::=
          elementdecl | AttlistDecl | EntityDecl | NotationDecl | PI | Comment
*/
    int done;
    XMLC pc;
    struct xml_stream * nxs;

    done = 0;

    while (!done) {
        pc = Xml_peek_nwchar(xs); /* should be < */
        if (pc == '%') {
            /* xpp_PEReference(xf, xs); */
            nxs = xpp_new_pe_stream(xf, xs);
            if (xv_err(xf)) return;

            xpp_intSubset(xf, nxs);

            xpp_free_stream(xf, nxs);
            if (xv_err(xf)) return;
        } else if (pc == '<') {
            Xml_get_char(xs); /* < */
            pc = Xml_get_char(xs);
            if (pc == '!') {
                xpp_markupdecl(xf, xs);
                if (xv_err(xf)) return;
            } else if (pc == '?') {
                xpp_PI(xf, xs);
                if (xv_err(xf)) return;
            } else {
                Xml_seterr_C(xf, xs, XMLERR_EXPMARKUP, pc);
                return;
            }

            pc = Xml_get_nwchar(xs);
            if (pc != '>') {
                Xml_seterr(xf, xs, XMLERR_EXPCLOSE);
                return;
            }
        } else {
            done = 1;
        }
    }
}
/***************************************************************/
static void xpp_TextDecl(struct xmls_file * xf,
                          struct xml_stream * xs) {
/*
[77]    TextDecl    ::=    '<?xml' VersionInfo? EncodingDecl S? '?>'
*/
    XMLC * tok;

    tok = Xml_next_token_raw(xs);
    if (!xmlcstrcmp(tok, "xml")) {
        tok = Xml_next_token_raw(xs);
        if (!xmlcstrcmp(tok, "version")) {
            xpp_VersionInfo(xf, xs);
            if (xv_err(xf)) return;
            tok = Xml_next_token_raw(xs);
        }

        if (!xmlcstrcmp(tok, "encoding")) {
            xpp_EncodingDecl(xf, xs);
            if (xv_err(xf)) return;
            tok = Xml_next_token_raw(xs);
        }

        if (xmlcstrcmp(tok, "?")) {
            Xml_seterr(xf, xs, XMLERR_NOTXML);
            return;
        }

        tok = Xml_next_token_raw(xs);
        if (xmlcstrcmp(tok, ">")) {
            Xml_seterr(xf, xs, XMLERR_NOTXML);
            return;
        }
    }
}
/***************************************************************/
static void xpp_includeSect(struct xmls_file * xf,
                          struct xml_stream * xs) {
/*
[62]    includeSect    ::=    '<![' S? 'INCLUDE' S? '[' extSubsetDecl ']]>'
*/
    XMLC pc;

    pc = Xml_get_nwchar(xs);
    if (pc != '[') {
        Xml_seterr_C(xf, xs, XMLERR_EXPOPBRACKET, pc);
        return;
    }
    xpp_extSubsetDecl(xf, xs);
    if (xv_err(xf)) return;

    pc = Xml_get_nwchar(xs); /* should be > */
    if (pc != '>') {
        Xml_seterr_C(xf, xs, XMLERR_EXPCLOSE, pc);
    } else {
        pc = Xml_get_nwchar(xs);
        if (pc != ']') {
            Xml_seterr_C(xf, xs, XMLERR_EXPCLBRACKET, pc);
            return;
        }
    }
}
/***************************************************************/
static void xpp_ignoreSect(struct xmls_file * xf,
                          struct xml_stream * xs) {
/*
[62]    includeSect    ::=    '<![' S? 'INCLUDE' S? '[' extSubsetDecl ']]>'
[63]    ignoreSect    ::=    '<![' S? 'IGNORE' S? '[' ignoreSectContents* ']]>'
[64]    ignoreSectContents    ::=
            Ignore ('<![' ignoreSectContents ']]>' Ignore)*
[65]    Ignore    ::=    Char* - (Char* ('<![' | ']]>') Char*)
*/
    XMLC xc;
    int depth;
    int state;
    XMLC quote;

    xc = Xml_get_nwchar(xs);
    if (xc != '[') {
        Xml_seterr_C(xf, xs, XMLERR_EXPOPBRACKET, xc);
        return;
    }

    depth = 1;
    state = 1;
    quote = 0;

    while (state) {
        xc = Xml_get_char(xs);
        switch (xc) {
            case XMLCEOF:
                Xml_seterr(xf, xs, XMLERR_EXPOPBRACKET);
                state = 0;
                break;
            case '\'': case '\"':
                state = 1;
                if (quote) quote = 0;
                else       quote = xc;
                break;
            case '<':
                if (!quote) state = 2;
                break;
            case '!':
                if (!quote) {
                    if (state == 2) state = 3;
                    else            state = 1;
                }
                break;
            case '[':
                if (!quote) {
                    if (state == 3) depth++;
                    state = 1;
                }
                break;
            case ']':
                if (!quote) {
                    if (state == 4) state = 5;
                    else            state = 4;
                }
                break;
            case '>':
                if (!quote) {
                    if (state == 5) {
                        depth--;
                        if (!depth) {
                            Xml_unget_char(xs);
                            state = 0;
                        } else {
                            state = 1;
                        }
                    } else {
                        state = 1;
                    }
                }
                break;
            default:
                state = 1;
                break;
        }
    }
}
/***************************************************************/
static void xpp_conditionalSect(struct xmls_file * xf,
                          struct xml_stream * xs) {
/*
[61]    conditionalSect    ::=    includeSect | ignoreSect
[62]    includeSect    ::=    '<![' S? 'INCLUDE' S? '[' extSubsetDecl ']]>'
[63]    ignoreSect    ::=    '<![' S? 'IGNORE' S? '[' ignoreSectContents* ']]>'
[64]    ignoreSectContents    ::=
            Ignore ('<![' ignoreSectContents ']]>' Ignore)*
[65]    Ignore    ::=    Char* - (Char* ('<![' | ']]>') Char*)
*/
    XMLC * tok;
    XMLC pc;

    tok = Xml_next_token_raw(xs);
    if (!xmlcstrcmp(tok, "INCLUDE")) {
        xpp_includeSect(xf, xs);
        if (xv_err(xf)) return;

        pc = Xml_get_nwchar(xs);
        if (pc != ']') {
            Xml_seterr_C(xf, xs, XMLERR_EXPCLBRACKET, pc);
            return;
        }
    } else if (!xmlcstrcmp(tok, "IGNORE")) {
        xpp_ignoreSect(xf, xs);
        if (xv_err(xf)) return;
    } else {
        Xml_seterr_S(xf, xs, XMLERR_EXPIGNINC, tok);
        return;
    }
}
/***************************************************************/
static void xpp_conditionalSectPE(struct xmls_file * xf,
                          struct xml_stream * xs) {
    XMLC * tok;
    XMLC pc;
    struct xml_stream * nxs;

    nxs = xpp_new_pe_value_stream(xf, xs);
    if (xv_err(xf)) return;

    tok = Xml_next_token_raw(nxs);
    pc = Xml_peek_nwchar(nxs);
    if (pc && pc != XMLCEOF) {
        Xml_seterr(xf, xs, XMLERR_BADIGNINC);
    } else {
        if (!xmlcstrcmp(tok, "INCLUDE")) {
            xpp_includeSect(xf, xs);

            pc = Xml_get_nwchar(xs);
            if (pc != ']') {
                Xml_seterr_C(xf, xs, XMLERR_EXPCLBRACKET, pc);
                return;
            }
        } else if (!xmlcstrcmp(tok, "IGNORE")) {
            xpp_ignoreSect(xf, xs);
        } else {
            Xml_seterr_S(xf, xs, XMLERR_EXPIGNINC, tok);
        }
    }

    xpp_free_stream(xf, nxs);
}
/***************************************************************/
static void xpp_conditional_markupdecl(struct xmls_file * xf,
                          struct xml_stream * xs) {
/*
[31]    extSubsetDecl    ::=    ( markupdecl | conditionalSect | DeclSep)*
[29]    markupdecl    ::=
          elementdecl | AttlistDecl | EntityDecl | NotationDecl | PI | Comment
[28a]    DeclSep    ::=    PEReference | S

[61]    conditionalSect    ::=    includeSect | ignoreSect
*/
    XMLC pc;
    XMLC xc;

    xc = Xml_get_char(xs);
    if (xc == '!') {
        pc = Xml_peek_nwchar(xs);
        if (pc == '[') {
            Xml_get_char(xs);  /* [ */
            pc = Xml_peek_nwchar(xs);
            if (pc == '%') {
                xpp_conditionalSectPE(xf, xs);
            } else {
                xpp_conditionalSect(xf, xs);
            }
        } else {
            xpp_markupdecl(xf, xs);
            if (xv_err(xf)) return;
        }
    } else if (xc == '?') {
        xpp_PI(xf, xs);
        if (xv_err(xf)) return;
    } else {
        Xml_seterr_C(xf, xs, XMLERR_EXPMARKUP, xc);
        return;
    }
}
/***************************************************************/
static void xpp_extSubsetDecl(struct xmls_file * xf,
                          struct xml_stream * xs) {
/*
[31]    extSubsetDecl    ::=    ( markupdecl | conditionalSect | DeclSep)*
[29]    markupdecl    ::=
          elementdecl | AttlistDecl | EntityDecl | NotationDecl | PI | Comment
[28a]    DeclSep    ::=    PEReference | S

[61]    conditionalSect    ::=    includeSect | ignoreSect
*/
    XMLC pc;
    int done;
    struct xml_stream * nxs;

    done = 0;
    while (!done) {
        pc = Xml_peek_nwchar(xs);
        if (pc == '%') {
            nxs = xpp_new_pe_value_stream(xf, xs);
            if (xv_err(xf)) return;

            xpp_extSubsetDecl(xf, nxs);
            if (!xv_err(xf)) {
                pc = Xml_get_char(nxs); /* EOF */
                if (pc != XMLCEOF) {
                    Xml_seterr_C(xf, nxs, XMLERR_EXPCLOSE, pc);
                }
            }
            xpp_free_stream(xf, nxs);
            if (xv_err(xf)) return;
        } else if (pc == '<') {
            Xml_get_char(xs); /* < */
            xpp_conditional_markupdecl(xf, xs);
            if (xv_err(xf)) return;

            pc = Xml_peek_nwchar(xs);
            if (pc == '>') {
                Xml_get_char(xs); /* > */
            } else {
                done = 1;
            }
        } else {
            done = 1;
        }
    }
}
/***************************************************************/
static void xpp_extSubset(struct xmls_file * xf,
                          struct xml_stream * xs,
                          struct xt_extID * extID) {
/*
[30]    extSubset    ::=    TextDecl? extSubsetDecl
[31]    extSubsetDecl    ::=    ( markupdecl | conditionalSect | DeclSep)*
[77]    TextDecl    ::=    '<?xml' VersionInfo? EncodingDecl S? '?>'
*/
    struct xml_stream * nxs;
    XMLC pc;
    XMLC * tok;

    nxs = xpp_open_ext_ID(xf, xs, extID);
    if (!nxs) return;

    pc = Xml_peek_nwchar(nxs); /* should be < */

    if (pc == '<') {
        Xml_get_char(nxs); /* < */
        pc = Xml_peek_nwchar(nxs);
        if (pc == '?') {
            Xml_get_char(nxs); /* ? */
            xpp_TextDecl(xf, nxs);
            pc = Xml_peek_nwchar(nxs); /* should be < */
        } else if (pc == '!') {
            Xml_get_char(nxs); /* ! */
            tok = Xml_next_token_raw(nxs);
            if (tok[0] == '-') {
                xpp_Comment(xf, nxs);
                Xml_get_char(nxs); /* > */
                pc = Xml_peek_nwchar(nxs);
            }
        } else {
            Xml_unget_char(nxs);
        }
    }

    if (pc == '<') {
        xpp_extSubsetDecl(xf, nxs);
        if (xv_err(xf)) return;

        pc = Xml_get_nwchar(nxs); /* should be > */
        if (pc && pc != '>') {
            Xml_seterr_C(xf, nxs, XMLERR_EXPCLOSE, pc);
        } else {
            pc = Xml_peek_nwchar(nxs); /* should be < */
            if (pc && pc != XMLCEOF) {
                Xml_seterr(xf, nxs, XMLERR_EXPSYSEOF);
            }
        }
    } else {
        Xml_seterr(xf, nxs, XMLERR_BADEXTDTD);
    }

    xpp_free_stream(xf, nxs);
}
/***************************************************************/
static void xpp_doctypedecl(struct xmls_file * xf,
                          struct xml_stream * xs) {
/*
[28]    doctypedecl ::=
            '<!DOCTYPE' S Name (S ExternalID)? S? ('[' intSubset ']' S?)? '>'
[28b]    intSubset    ::=    (markupdecl | DeclSep)*
[75]    ExternalID    ::=    'SYSTEM' S SystemLiteral
                           | 'PUBLIC' S PubidLiteral S SystemLiteral
*/
    XMLC * tok;
    struct xt_extID * extID;

    xf->xv_sFlags |= XMLSFLAG_HASDTD;

    xf->xs_docname = xpp_get_Name(xf, xs);
    if (!xf->xs_docname) return;

    if (Xml_peek_nwchar(xs) != '[') {
        extID = xpp_ExternalID(xf, xs, 1);
        if (xv_err(xf)) return;

        xpp_extSubset(xf, xs, extID);

        Xml_free_extID(extID);
        if (xv_err(xf)) return;
    }

    if (Xml_peek_nwchar(xs) == '[') {
        Xml_get_char(xs); /* [ */

        xpp_intSubset(xf, xs);
        if (xv_err(xf)) return;

        if (Xml_peek_nwchar(xs) == ']') {
            Xml_get_char(xs); /* ] */
        }
    }

    if (Xml_peek_nwchar(xs) == '>') {
        tok = Xml_next_token_raw(xs);
    } else {
        Xml_seterr(xf, xs, XMLERR_EXPCLOSE);
        return;
    }
}
/***************************************************************/
static void xpp_Misc_doctypedecl(struct xmls_file * xf,
                          struct xml_stream * xs) {
/*
[22]    prolog    ::=    XMLDecl Misc* (doctypedecl Misc*)?
[27]    Misc    ::=    Comment | PI | S
[28]    doctypedecl ::=
            '<!DOCTYPE' S Name (S ExternalID)? S? ('[' intSubset ']' S?)? '>'
[15]    Comment    ::=    '<!--' ((Char - '-') | ('-' (Char - '-')))* '-->'
[16]    PI    ::=    '<?' PITarget (S (Char* - (Char* '?>' Char*)))? '?>'
*/
    XMLC * tok;
    int done;
    int doctype;
    XMLC pc;

    done = 0;
    doctype = 0;
    pc = Xml_peek_nwchar(xs);

    while (!done) {
        if (pc == '!') {
            Xml_get_char(xs); /* ! */
            tok = Xml_next_token_raw(xs);
            if (!xmlcstrcmp(tok, "DOCTYPE")) {
                if (doctype) {
                    Xml_seterr(xf, xs, XMLERR_DUPDOCTYPE);
                    return;
                } else {
                    xpp_doctypedecl(xf, xs);
                    if (xv_err(xf)) return;
                    doctype = 1;
                }
            } else if (tok[0] == '-') {
                xpp_Comment(xf, xs);
                Xml_get_char(xs); /* > */
            } else {
                Xml_seterr(xf, xs, XMLERR_EXPDOCTYPE);
                return;
            }
        } else if (pc == '?') {
            xpp_PI(xf, xs);
            if (xv_err(xf)) return;
        } else {
            done = 1;
        }

        if (!done) {
            pc = Xml_peek_nwchar(xs);
            if (pc ==  '<') {
                Xml_get_char(xs); /* < */
                pc = Xml_peek_nwchar(xs);
            } else {
                done = 1;
            }
        }
    }
}
/***************************************************************/
static void xpp_prolog(struct xmls_file * xf,
                          struct xml_stream * xs) {
/*
[22]    prolog    ::=    XMLDecl Misc* (doctypedecl Misc*)?
[23]    XMLDecl    ::=    '<?xml' VersionInfo EncodingDecl? SDDecl? S?'?>'
*/
    XMLC pc;

    xpp_XMLDecl(xf, xs);
    if (xv_err(xf)) return;

    pc = Xml_peek_nwchar(xs);
    if (!pc) return;

    if (pc ==  '<') {
        Xml_get_char(xs); /* < */
        xpp_Misc_doctypedecl(xf, xs);
        if (xv_err(xf)) return;
    } else {
        Xml_seterr(xf, xs, XMLERR_BADROOT);
        return;
    }
}
/***************************************************************/
static void xut_set_elNam(struct xmls_file * xf, XMLC * elNam) {

    int xLen;

    xLen = xmlclen(elNam) + 1;
    if (xLen > xf->xi_elNamSize) {
        xf->xi_elNamSize = xLen + 32;
        xf->xi_elNam =
            Realloc(xf->xi_elNam, XMLC, XMLC_BYTES(xf->xi_elNamSize));
    }

    memcpy(xf->xi_elNam, elNam, XMLC_BYTES(xLen));
}
/***************************************************************/
static void xut_set_attrNam(struct xmls_file * xf,
                            struct xml_stream * xs,
                            XMLC * attrNam) {

    int xLen;
    int nA;
    int ii;

    nA = xf->xi_nAttrs;

    if (nA == xf->xi_nAttrsSize) {
        xf->xi_nAttrsSize += 4;
        xf->xi_attrNames =
            Realloc(xf->xi_attrNames, XMLC *, xf->xi_nAttrsSize);
        xf->xi_attrVals =
            Realloc(xf->xi_attrVals , XMLC *, xf->xi_nAttrsSize);
        xf->xi_attrLens =
            Realloc(xf->xi_attrLens , int, xf->xi_nAttrsSize);
        xf->xi_attrNamesSize =
            Realloc(xf->xi_attrNamesSize , int, xf->xi_nAttrsSize);
        xf->xi_attrValsSize =
            Realloc(xf->xi_attrValsSize , int, xf->xi_nAttrsSize);

        for (ii = nA; ii < xf->xi_nAttrsSize; ii++) {
            xf->xi_attrNames[ii]        = 0;
            xf->xi_attrVals[ii]         = 0;
            xf->xi_attrLens[ii]         = 0;
            xf->xi_attrNamesSize[ii]    = 0;
            xf->xi_attrValsSize[ii]     = 0;
        }
    }

    xLen = xmlclen(attrNam) + 1;
    if (xLen >= xf->xi_attrNamesSize[nA]) {
        xf->xi_attrNamesSize[nA] = xLen + 32;
        xf->xi_attrNames[nA] =
            Realloc(xf->xi_attrNames[nA], XMLC, xf->xi_attrNamesSize[nA]);
    }
    memcpy(xf->xi_attrNames[nA], attrNam, XMLC_BYTES(xLen));

    xf->xi_nAttrs++;
}
/***************************************************************/
static void xup_expand_cbuf(XMLC ** cbuf,
                               int   * cbufSize) {

    *cbufSize += XML_K_CBUF_INC;
    *cbuf = Realloc(*cbuf, XMLC, *cbufSize);
}
/***************************************************************/
static void xup_copy_tbuf(struct xmls_file * xf,
                           struct xml_stream * xs,
                               XMLC ** cbuf,
                               int   * cbufSize,
                               int   * cbufLen) {

    XMLC xc;
    int ii;

    *cbufLen = 0;
    for (ii = 0; ii < xs->xf_tbuflen; ii++) {
        xc = xs->xf_tbuf[ii];
        if (*cbufLen >= *cbufSize) {
            xup_expand_cbuf(cbuf, cbufSize);
        }
        (*cbuf)[*cbufLen] = xc;
        (*cbufLen)++;
    }

    /* Add zero terminator */
    if (*cbufLen >= *cbufSize) {
        xup_expand_cbuf(cbuf, cbufSize);
    }
    (*cbuf)[*cbufLen] = 0;
}
/***************************************************************/
static void Xml_next_char_ref(struct xmls_file * xf,
                       struct xml_stream * xs) {
/*
[66]    CharRef    ::=    '&#' [0-9]+ ';'  | '&#x' [0-9a-fA-F]+ ';'



*/
    XMLC xc;
    unsigned int num;
    int ix;

    num = 0;
    xc = 0;
    ix = 1; /* skip over # */

    if (ix < xf->xf_ebuflen && xf->xf_ebuf[ix] == 'x') {
        ix++;
        while (ix < xf->xf_ebuflen) {
            xc = xf->xf_ebuf[ix++];
            if (xc >= '0' && xc <= '9') {
                num = (num << 4) | (xc - '0');
            } else if (xc >= 'A' && xc <= 'F') {
                num = (num << 4) | (xc - '7');
            } else if (xc >= 'a' && xc <= 'f') {
                num = (num << 4) | (xc - 'W');
            } else {
                Xml_seterr(xf, xs, XMLERR_BADCHREF);
                return;
            }
        }
    } else {
        while (ix < xf->xf_ebuflen) {
            xc = xf->xf_ebuf[ix++];
            if (xc >= '0' && xc <= '9') {
                num = (num * 10) + (xc - '0');
            } else {
                Xml_seterr(xf, xs, XMLERR_BADCHREF);
                return;
            }
        }
    }

    /* check to make certain at least one character processed */
    if (!xc) {
        Xml_seterr(xf, xs, XMLERR_BADCHREF);
        return;
    }

    xc = (XMLC)num;
    if (xs->xf_tbuflen == xs->xf_tbufsize) {
        Xml_expand_tbuf(xs);
    }
    xs->xf_tbuf[xs->xf_tbuflen] = xc;
    xs->xf_tbuflen++;
}
/***************************************************************/
static void Xml_next_token_ref(struct xmls_file * xf,
                               struct xml_stream * xs,
                               int depth);
/********/
static void Xml_put_entity_char(XMLC xc,
                               struct xmls_file * xf,
                               struct xml_stream * xs,
                               int depth) {

    if (xc == ';') {
        if (!xf->xf_ebuflen) {
            Xml_seterr(xf, xs, XMLERR_BADGEREF);
            return;
        }
        xf->xf_ebuf[xf->xf_ebuflen] = 0;
        Xml_next_token_ref(xf, xs, depth + 1);
    } else {
        if (!xf->xf_ebuflen) {
            if (!XML_IS_NSTART(xc) &&  xc != '#') {
                Xml_seterr(xf, xs, XMLERR_BADGEREF);
                return;
            }
        } else if (!XML_IS_NAME(xc)) {
            Xml_seterr(xf, xs, XMLERR_BADGEREF);
            return;
        }

        if (xf->xf_ebuflen + 1 >= xf->xf_ebufsize) {
            xf->xf_ebufsize += XML_K_EBUF_INC;
            xf->xf_ebuf = Realloc(xf->xf_ebuf, XMLC, xf->xf_ebufsize);
        }
        xf->xf_ebuf[xf->xf_ebuflen] = xc;
        xf->xf_ebuflen++;
    }
}
/***************************************************************/
static void Xml_next_token_ref(struct xmls_file * xf,
                               struct xml_stream * xs,
                               int depth) {

    struct xt_entity * en;
    struct xt_entity ** pen;
    int ii;
    XMLC xc;
    int state;
    int last;

    if (depth > XML_MAX_ENTITY_RECUR) {
        Xml_seterr(xf, xs, XMLERR_RECURENTITY);
        return;
    }

    if (xf->xf_ebuflen && xf->xf_ebuf[0] == '#') {
        Xml_next_char_ref(xf, xs);
        return;
    }

    pen = (struct xt_entity **)dbtree_find(xf->xs_g_entities->xe_entity,
                            xf->xf_ebuf, XMLC_BYTES(xf->xf_ebuflen + 1));
    if (!pen) {
        Xml_seterr_S(xf, xs, XMLERR_NOSUCHGE, xf->xf_ebuf);
    } else {
        en = *pen;
        state = 0;
        last = en->e_len - en->e_ix;
        for (ii = en->e_ix; ii < last; ii++) {
            xc = en->e_val[ii];
            if (state) {
                Xml_put_entity_char(xc, xf, xs, depth);
                if (xv_err(xf)) return;
                if (xc == ';') state = 2;
            } else if (xc == '&') {
                xf->xf_ebuflen = 0;
                state = 1;
            }

            if (!state) {
                if (xs->xf_tbuflen == xs->xf_tbufsize) {
                    Xml_expand_tbuf(xs);
                }
                xs->xf_tbuf[xs->xf_tbuflen] = xc;
                xs->xf_tbuflen++;
            } else if (state == 2) {
                state = 0;
            }
        }
    }
}
/***************************************************************/
static XMLC * Xml_next_token_parsed(struct xmls_file * xf,
                                    struct xml_stream * xs,
                                    int ttyp) {
/*
** ttyp:
**   0 - literal
**   1 - content
**   2 - content, xml:space="preserve"
*/

    XMLC xc;
    XMLC termc;
    int done;
    int state;
    int whiteix;

    done   = 0;
    state  = 0;
    whiteix = -1;
    xs->xf_tbuflen = 0;

    if (!ttyp) {
        xc = Xml_get_nwchar(xs);
        if (xc == '\'' || xc == '"') {
            termc = xc;
        } else {
            Xml_seterr(xf, xs, XMLERR_EXPLITERAL);
            return (0);
        }
    } else {
        termc = '<';
    }

    while (!done) {
        xc = Xml_get_char(xs);
        if (xc == XMLCEOF) {
            Xml_seterr(xf, xs, XMLERR_UNEXPEOF);
            return (0);
        } else if (xc == termc) {
            if (ttyp) Xml_unget_char(xs);
            done = 1;
        } else {
            if (state) {
                Xml_put_entity_char(xc, xf, xs, 0);
                if (xv_err(xf)) return (0);
                if (xc == ';') state = 2;
            } else if (xc == '&') {
                xf->xf_ebuflen = 0;
                state = 1;
            }

            if (!state) {
                if (xs->xf_tbuflen == xs->xf_tbufsize) {
                    Xml_expand_tbuf(xs);
                }
                if (XML_IS_WHITE(xc)) {
                    if (whiteix < 0) whiteix = xs->xf_tbuflen;
                } else {
                    whiteix = -1;
                }
                xs->xf_tbuf[xs->xf_tbuflen] = xc;
                xs->xf_tbuflen++;
            } else if (state == 2) {
                state = 0;
            }
        }
    }

    /* strip trailing whitespace for content */
    if (ttyp == 1 && whiteix >= 0) xs->xf_tbuflen = whiteix;

    if (xs->xf_tbuflen == xs->xf_tbufsize) {
        Xml_expand_tbuf(xs);
    }
    xs->xf_tbuf[xs->xf_tbuflen] = 0;

    return (xs->xf_tbuf);
}
/***************************************************************/
static void xup_AttValue(struct xmls_file * xf,
                       struct xml_stream * xs) {

    int nA;

    nA = xf->xi_nAttrs - 1;

    xup_copy_tbuf(xf, xs,
        &(xf->xi_attrVals[nA]),
        &(xf->xi_attrValsSize[nA]),
        &(xf->xi_attrLens[nA]));
}
/***************************************************************/
static void xup_Attribute(struct xmls_file * xf,
                       struct xml_stream * xs) {

/*
[41]    Attribute    ::=    Name Eq AttValue
[10]    AttValue    ::=    '"' ([^<&"] | Reference)* '"'
                        |  "'" ([^<&'] | Reference)* "'"
[66]    CharRef    ::=    '&#' [0-9]+ ';'  | '&#x' [0-9a-fA-F]+ ';'
[67]    Reference    ::=    EntityRef | CharRef
[68]    EntityRef    ::=    '&' Name ';'
*/
    XMLC * atNam;
    XMLC xc;

    atNam = xpp_next_Name(xf, xs);
    if (!atNam) return;

    xut_set_attrNam(xf, xs, atNam);
    if (xv_err(xf)) return;

    xc = Xml_get_nwchar(xs);
    if (xc != '=') {
        Xml_seterr(xf, xs, XMLERR_EXPEQ);
        return;
    }

    Xml_next_token_parsed(xf, xs, 0);
    if (xv_err(xf)) return;

    xup_AttValue(xf, xs);
    if (xv_err(xf)) return;
}
/***************************************************************/
static struct xt_element *  xpp_add_implicit_element(struct xmls_file * xf) {

    struct xt_element * el;
    XMLC * elNam;

    elNam = xmlcstrdup(xf->xi_elNam);

    el = Xml_new_element(elNam, ELTYP_IMPLICIT);
    xpp_add_element(xf->xs_elements, el, elNam);

    return (el);
}
/***************************************************************/
static struct xt_attlist * xpp_element_attlist(struct xmls_file * xf,
                                      struct xt_element  * el) {

    struct xt_attlist * att;
    struct xt_attlist ** patt;

    patt = (struct xt_attlist **)dbtree_find(xf->xs_attlists->xa_attlist,
                            xf->xi_elNam,  xmlcstrbytes(xf->xi_elNam));
    if (!patt) {
        att = xpp_new_attlist(xmlcstrdup(xf->xi_elNam));

        dbtree_insert(xf->xs_attlists->xa_attlist,
                      xf->xi_elNam,  xmlcstrbytes(xf->xi_elNam), att);
    } else {
        att = *patt;
    }
    el->l_attlist = att;

    return (att);
}
/***************************************************************/
static void xpp_add_implicit_attlists(struct xmls_file * xf,
                                      struct xt_element  * el) {
/*
*/

    int ii;
    struct xt_attlist * att;
    struct xt_attdef ** pad;
    struct xt_attdef * ad;

    att = el->l_attlist;

    for (ii = 0; ii < xf->xi_nAttrs; ii++) {
        pad = (struct xt_attdef **)dbtree_find(att->a_attdefs,
                xf->xi_attrNames[ii],  xmlcstrbytes(xf->xi_attrNames[ii]));
        if (!pad) {
            ad = xpp_new_attdef(xmlcstrdup(xf->xi_attrNames[ii]));
            ad->a_atttyp = ATTTYP_CDATA;
            ad->a_defaulttyp = ATTDEFTYP_IMPLIED;
            xpp_add_attlist(att, ad);
        }
    }
}
/***************************************************************/
static void xpp_validate_att_value(struct xmls_file * xf,
                          struct xml_stream * xs,
                          struct xt_element  * el,
                          struct xt_attdef * ad,
                          int attValLen,
                          XMLC * attVal) {

    if (!xmlcstrcmp(ad->a_attname, "xml:space")) {
        if (!xmlcstrcmp(attVal, "preserve")) {
            el->l_preserve_space = 1;
        }
    }
}
/***************************************************************/
static void xpp_default_att_value(struct xmls_file * xf,
                          struct xml_stream * xs,
                          struct xt_attdef * ad) {

    int vLen;
    XMLC xc;
    int ii;

    vLen = xmlclen(ad->a_defaultval) - 1; /* remove quotes */

    xs->xf_tbuflen = 0;
    for (ii = 1; ii < vLen; ii++) {
        xc = ad->a_defaultval[ii];

        if (xs->xf_tbuflen == xs->xf_tbufsize) {
            Xml_expand_tbuf(xs);
        }
        xs->xf_tbuf[xs->xf_tbuflen] = xc;
        xs->xf_tbuflen++;
    }

    if (xs->xf_tbuflen == xs->xf_tbufsize) {
        Xml_expand_tbuf(xs);
    }
    xs->xf_tbuf[xs->xf_tbuflen] = 0;
    xs->xf_tbuflen++;

    xut_set_attrNam(xf, xs, ad->a_attname);
    if (xv_err(xf)) return;

    xup_AttValue(xf, xs);
    if (xv_err(xf)) return;
}
/***************************************************************/
static void xpp_p_attlist(struct xmls_file * xf,
                          struct xml_stream * xs,
                          struct xt_element  * el) {
/*
*/

    int ii;
    struct xt_attlist * att;
    struct xt_attdef ** pad;
    struct xt_attdef * ad;
    void * vdbtc;

    att = el->l_attlist;

    for (ii = 0; ii < xf->xi_nAttrs; ii++) {
        pad = (struct xt_attdef **)dbtree_find(att->a_attdefs,
                xf->xi_attrNames[ii],  xmlcstrbytes(xf->xi_attrNames[ii]));
        if (!pad) {
            if (!(xf->xv_sFlags |= XMLSFLAG_DIDIMPLY)) {
                if (xf->xv_vFlags & XMLVFLAG_VALIDATE) {
                    Xml_seterr_S(xf, xs,
                            XMLERR_NOSUCHATTLIST, xf->xi_attrNames[ii]);
                    return;
                }
            }
        } else {
            ad = *pad;
            if (ad->a_ref_flag) {
                    Xml_seterr_S(xf, xs,
                            XMLERR_DUPATTLIST, xf->xi_attrNames[ii]);
                    return;
            } else {
                ad->a_ref_flag = 1;
                xpp_validate_att_value(xf, xs, el, ad,
                        xf->xi_attrLens[ii], xf->xi_attrVals[ii]);
                if (xv_err(xf)) return;
            }
        }
    }

    /* process default value */
    vdbtc = dbtree_rewind(att->a_attdefs, DBTREE_READ_QUICK);
    do {
        pad = (struct xt_attdef **)dbtree_next(vdbtc);
        if (pad) {
            ad = *pad;
            if (!(ad->a_ref_flag)) {
                if (ad->a_defaulttyp == ATTDEFTYP_REQUIRED) {
                    if (xf->xv_vFlags & XMLVFLAG_VALIDATE) {
                        Xml_seterr_S(xf, xs,
                                XMLERR_ATTREQ, ad->a_attname);
                        pad = 0; /* stop loop */
                    }
                } else if (ad->a_defaulttyp == ATTDEFTYP_FIXED ||
                           ad->a_defaulttyp == ATTDEFTYP_VAL) {
                    xpp_default_att_value(xf, xs, ad);

                    xpp_validate_att_value(xf, xs, el, ad,
                            xf->xi_attrLens[xf->xi_nAttrs-1],
                            xf->xi_attrVals[xf->xi_nAttrs-1]);
                    if (xv_err(xf)) pad = 0; /* stop loop */
                }
            }
        }
    } while (pad);
    dbtree_close(vdbtc);
}
/***************************************************************/
static void xup_p_element(struct xmls_file * xf,
                       struct xml_stream * xs,
                       struct xt_element  * el) {

    int ii;
    struct xt_ielement * xie;

    xf->xc_ielements[xf->xi_depth - 1]->xc_element = el;

    if (xf->xi_depth > 1) {
        xie = xf->xc_ielements[xf->xi_depth - 2];

        ii = xie->xc_ncElements;
        if (ii == xie->xc_mxElements) {
            xie->xc_cElements =
                Realloc(xie->xc_cElements, struct xt_element *, ii + 1);
            xie->xc_mxElements++;
        }
        xie->xc_cElements[ii] = el;
        xie->xc_ncElements++;
    }

    if (xf->xv_sFlags & XMLSFLAG_IMPLYDTD) {
        xpp_add_implicit_attlists(xf, el);
    } else {
        xpp_p_attlist(xf, xs, el);
    }
}
/***************************************************************/
static struct xt_ielement * xup_new_ielement(void) {

    struct xt_ielement * xie;

    xie = New(struct xt_ielement, 1);

    xie->xc_mxElements      = 0;
    xie->xc_ncElements      = 0;
    xie->xc_element         = 0;
    xie->xc_pcdata          = 0;

    xie->xc_cElements       = 0;

    return (xie);
}
/***************************************************************/
static void xup_init_element(struct xmls_file * xf) {

    if (xf->xi_depth == xf->xc_maxDepth) {
        xf->xc_ielements  =
            Realloc(xf->xc_ielements , struct xt_ielement *, xf->xi_depth + 1);
        xf->xc_ielements[xf->xc_maxDepth] = xup_new_ielement();
        xf->xc_maxDepth = xf->xi_depth + 1;
    }

    xf->xc_ielements[xf->xi_depth]->xc_ncElements  = 0;
    xf->xc_ielements[xf->xi_depth]->xc_element     = 0;
    xf->xc_ielements[xf->xi_depth]->xc_pcdata      = 0;

    xf->xi_nAttrs = 0;
    xf->xi_contentLen = 0;
    xf->xi_depth++;
}
/***************************************************************/
static struct xt_i_elements * xup_new_i_elements(void) {

    struct xt_i_elements * ie;

    ie = New(struct xt_i_elements, 1);
    ie->i_pcdata = 0;
    ie->i_ielements = dbtree_new();

    return (ie);
}
/***************************************************************/
static void xup_imply_contents(struct xmls_file * xf,
                               struct xt_ielement * xie) {

    int ii;
    struct xt_element  * el;
    struct xt_imp_element ** pmel;
    struct xt_imp_element * mel;
    XMLC * elNam;
    void * vdbtc;

    el = xie->xc_element;
    if (!el->l_i_elements) el->l_i_elements = xup_new_i_elements();

    vdbtc = dbtree_rewind(el->l_i_elements->i_ielements, DBTREE_READ_QUICK);
    do {
        pmel = (struct xt_imp_element **)dbtree_next(vdbtc);
        if (pmel) (*pmel)->m_accessed = 0;
    } while (pmel);
    dbtree_close(vdbtc);

    el->l_i_elements->i_pcdata |= xie->xc_pcdata;

    for (ii = 0; ii < xie->xc_ncElements; ii++) {
        elNam = xie->xc_cElements[ii]->l_name;
        pmel = (struct xt_imp_element **)dbtree_find(
                                el->l_i_elements->i_ielements,
                                elNam,  xmlcstrbytes(elNam));
        if (!pmel) {
            mel = New(struct xt_imp_element, 1);
            mel->m_elName = xmlcstrdup(elNam);
            mel->m_repeat = 0;
            mel->m_accessed = 1;
            dbtree_insert(el->l_i_elements->i_ielements,
                elNam,  xmlcstrbytes(elNam), mel);
        } else {
            (*pmel)->m_accessed++;
        }
    }

    vdbtc = dbtree_rewind(el->l_i_elements->i_ielements, DBTREE_READ_QUICK);
    do {
        pmel = (struct xt_imp_element **)dbtree_next(vdbtc);
        if (pmel) {
            mel = *pmel;
            switch (mel->m_repeat) {
                case REPEAT_ONE :
                    if (!mel->m_accessed)
                        mel->m_repeat = REPEAT_ZERO_OR_MORE;
                    else if (mel->m_accessed > 1)
                        mel->m_repeat = REPEAT_ONE_OR_MORE;
                    break;
                case REPEAT_ZERO_OR_MORE :
                    /* do nothing */
                    break;
                case REPEAT_ZERO_OR_ONE :
                    if (mel->m_accessed > 1)
                        mel->m_repeat = REPEAT_ZERO_OR_MORE;
                    break;
                case REPEAT_ONE_OR_MORE :
                    if (!mel->m_accessed)
                        mel->m_repeat = REPEAT_ZERO_OR_ONE;
                    break;
                default:
                    break;
            }
        }

    } while (pmel);
    dbtree_close(vdbtc);
}
/***************************************************************/
void xup_imply_element(struct xmls_file * xf, struct xt_element * el) {
/*
*/
    struct xt_imp_element ** pmel;
    struct xt_imp_element * mel;
    void * vdbtc;
    struct xt_eltree * et;
    struct xt_eltree * nelt;

    et = 0;
    if (el->l_i_elements->i_pcdata) {
        et = xpp_new_element_node(xmlcstrdups("#PCDATA"));
        et->lt_repeat = REPEAT_ZERO_OR_MORE;
        vdbtc = dbtree_rewind(el->l_i_elements->i_ielements,
                                DBTREE_READ_FORWARD);
        do {
            pmel = (struct xt_imp_element **)dbtree_next(vdbtc);
            if (pmel) {
                mel = *pmel;
                nelt = xpp_new_element_node(xmlcstrdup(mel->m_elName));
                nelt->lt_repeat = mel->m_repeat;
                if (!et->lt_nnodes) {
                    et = xpp_new_element_op(et, xmlcstrdups("|"));
                }
                xpp_add_element_node(et, nelt);
            }
        } while (pmel);
        dbtree_close(vdbtc);
    } else {
        vdbtc = dbtree_rewind(el->l_i_elements->i_ielements,
                                DBTREE_READ_FORWARD);
        do {
            pmel = (struct xt_imp_element **)dbtree_next(vdbtc);
            if (pmel) {
                mel = *pmel;
                if (!et) {
                    et = xpp_new_element_node(xmlcstrdup(mel->m_elName));
                    et->lt_repeat = mel->m_repeat;
                } else {
                    nelt = xpp_new_element_node(xmlcstrdup(mel->m_elName));
                    nelt->lt_repeat = mel->m_repeat;
                    if (!et->lt_nnodes) {
                        et = xpp_new_element_op(et, xmlcstrdups(","));
                    }
                    xpp_add_element_node(et, nelt);
                }
            }
        } while (pmel);
        dbtree_close(vdbtc);
    }

    el->l_tree = et;
}
/***************************************************************/
void xup_imply_elements(struct xmls_file * xf) {
/*
*/
    struct xt_element ** pel;
    void * vdbtc;
    int depth;

    depth = xf->xi_depth;
    while (depth)  {
        depth--;
        xup_imply_contents(xf, xf->xc_ielements[depth]);
    }

    vdbtc = dbtree_rewind(xf->xs_elements->xl_elements, DBTREE_READ_FORWARD);
    do {
        pel = (struct xt_element **)dbtree_next(vdbtc);
        if (pel && (*pel)->l_i_elements) xup_imply_element(xf, *pel);
    } while (pel);
    dbtree_close(vdbtc);
}
/***************************************************************/
static void xup_end_element(struct xmls_file * xf) {

    int ii;
    struct xt_ielement * xie;

    if (xf->xv_debug & 1) {
        xie = xf->xc_ielements[xf->xi_depth - 1];
        printf("End element: %S\n", xie->xc_element->l_name);

        for (ii = 0; ii < xie->xc_ncElements; ii++) {
            printf("    :%S\n",
                xie->xc_cElements[ii]->l_name);
        }
    }

    xf->xi_depth--;

    if (xf->xv_sFlags & XMLSFLAG_IMPLYDTD) {
        xup_imply_contents(xf, xf->xc_ielements[xf->xi_depth]);
    }
}
/***************************************************************/
static int xup_ETag(struct xmls_file * xf,
                       struct xml_stream * xs) {
/*
[42]    ETag    ::=    '</' Name S? '>'
*/

    int result;
    XMLC * elNam;
    XMLC xc;

    result = 0;
    if (!xf->xi_depth) {
        Xml_seterr(xf, xs, XMLERR_UNEXPCLOSE);
        return (XMLRESULT_ERR);
    }

    elNam = xpp_next_Name(xf, xs);
    if (!elNam) return (XMLRESULT_ERR);

    /* Make certain close matches */
    if (xmlccmp(elNam,
                xf->xc_ielements[xf->xi_depth-1]->xc_element->l_name)) {
        Xml_seterr(xf, xs, XMLERR_UNEXPCLOSE);
        return (XMLRESULT_ERR);
    }

    xut_set_elNam(xf, elNam);

    xc = Xml_get_nwchar(xs);
    if (xc != '>') {
        Xml_seterr(xf, xs, XMLERR_EXPCLOSE);
        result = XMLRESULT_ERR;
    } else {
        if (xf->xi_p_endtag) {
            result = xf->xi_p_endtag(xf->xi_vp, xf->xi_elNam);
        }
    }

    xup_end_element(xf);

    return (result);
}
/***************************************************************/
static int xup_p_content(struct xmls_file * xf) {

    int result;

    xf->xc_ielements[xf->xi_depth - 1]->xc_pcdata = 1;

    if (xf->xi_p_content) {
        result = xf->xi_p_content(xf->xi_vp,
                xf->xi_contentLen,
                xf->xi_content);
    }

    return (result);
}
/***************************************************************/
static int xup_cdata(struct xmls_file * xf,
                       struct xml_stream * xs) {

    int done;
    XMLC xc;
    int nc;
    int result;

    done = 0;
    nc   = 0;
    xf->xi_contentLen = 0;

    while (!done) {
        xc = Xml_get_char(xs);
        if (xc == XMLCEOF) {
            done = 1;
        } else if (xc == ']') {
            nc++;
        } else if (xc == '>' && nc >= 2) {
            done = 1;
        } else {
            while (nc) {
                if (xf->xi_contentLen == xf->xi_contentSize) {
                    xup_expand_cbuf(&(xf->xi_content), &(xf->xi_contentSize));
                }
                xf->xi_content[xf->xi_contentLen] = ']';
                xf->xi_contentLen++;
                nc--;
            }
            if (xf->xi_contentLen == xf->xi_contentSize) {
                xup_expand_cbuf(&(xf->xi_content), &(xf->xi_contentSize));
            }
            xf->xi_content[xf->xi_contentLen] = xc;
            xf->xi_contentLen++;
        }
    }

    /* Add zero terminator */
    if (xf->xi_contentLen == xf->xi_contentSize) {
        xup_expand_cbuf(&(xf->xi_content), &(xf->xi_contentSize));
    }
    xf->xi_content[xf->xi_contentLen] = 0;

    result = xup_p_content(xf);

    return (result);
}
/***************************************************************/
static int xup_bang(struct xmls_file * xf,
                       struct xml_stream * xs) {

    int result;
    XMLC xc;
    XMLC * tok;

    result = 0;
    xc = Xml_get_char(xs); /* ! */
    if (xc == '-') {
        xpp_Comment(xf, xs);
        Xml_get_char(xs); /* > */
    } else if (xc == '[') {
        tok = Xml_next_token_raw(xs);
        if (!xmlcstrcmp(tok, "CDATA")) {
            xc = Xml_get_char(xs); /* ! */
            if (xc == '[') {
                result = xup_cdata(xf, xs);
            } else {
                Xml_seterr_C(xf, xs, XMLERR_EXPCTL, xc);
                result = XMLRESULT_ERR;
            }
        } else {
            Xml_seterr_C(xf, xs, XMLERR_EXPCTL, xc);
            result = XMLRESULT_ERR;
        }
    } else {
        Xml_seterr_C(xf, xs, XMLERR_EXPCTL, xc);
        result = XMLRESULT_ERR;
    }

    return (result);
}
/***************************************************************/
static int xup_PI(struct xmls_file * xf,
                       struct xml_stream * xs) {

    int result;

    result = 0;
    xpp_PI(xf, xs);
    if (xv_err(xf)) result = XMLRESULT_ERR;

    return (result);
}
/***************************************************************/
static struct xt_element * xup_start_element(struct xmls_file * xf,
                       struct xml_stream * xs) {
/*
*/

    struct xt_attlist * att;
    struct xt_element ** pel;
    struct xt_element  * el;
    struct xt_attdef ** ad;
    void * vdbtc;

    pel = (struct xt_element **)dbtree_find(xf->xs_elements->xl_elements,
                            xf->xi_elNam,  xmlcstrbytes(xf->xi_elNam));
    if (!pel) {
        if (xf->xv_vFlags & XMLVFLAG_VALIDATE) {
            Xml_seterr(xf, xs, XMLERR_NOSUCHELEMENT);
            return (0);
        } else {
            el = xpp_add_implicit_element(xf);
        }
    } else {
        el = *pel;
    }
    el->l_preserve_space = 0;

    att = el->l_attlist;
    if (!att) att = xpp_element_attlist(xf, el);

    vdbtc = dbtree_rewind(att->a_attdefs, DBTREE_READ_QUICK);
    do {
        ad = (struct xt_attdef **)dbtree_next(vdbtc);
        if (ad) (*ad)->a_ref_flag = 0;
    } while (ad);
    dbtree_close(vdbtc);

    return (el);
}
/***************************************************************/
static int xup_element(struct xmls_file * xf,
                       struct xml_stream * xs) {

/*
[39]    element    ::=    EmptyElemTag | STag content ETag
[40]    STag    ::=    '<' Name (S Attribute)* S? '>'
[41]    Attribute    ::=    Name Eq AttValue
[42]    ETag    ::=    '</' Name S? '>'
[44]    EmptyElemTag    ::=    '<' Name (S Attribute)* S? '/>'
*/
    int result;
    XMLC * elNam;
    int done;
    XMLC pc;
    int empty;
    struct xt_element  * el;

    pc = Xml_peek_char(xs);
    if (pc == '!') {
        Xml_get_char(xs); /* ! */
        result = xup_bang(xf, xs);
        return (result);
    } else if (pc == '?') {
        Xml_get_char(xs); /* ? */
        result = xup_PI(xf, xs);
        return (result);
    } else if (pc == '/') {
        Xml_get_char(xs); /* / */
        result = xup_ETag(xf, xs);
        return (result);
    }

    result = 0;
    empty = 0;
    xup_init_element(xf);

    elNam = xpp_next_Name(xf, xs);
    if (!elNam) return (XMLRESULT_ERR);

    xut_set_elNam(xf, elNam);

    if (!xf->xs_docname && (xf->xv_sFlags & XMLSFLAG_IMPLYDTD)) {
        xf->xs_docname = xmlcstrdup(elNam);
    }

    el = xup_start_element(xf, xs);
    if (xv_err(xf)) return (XMLRESULT_ERR);

    done = 0;
    while (!done) {
        pc = Xml_peek_nwchar(xs);
        if (!pc) {
            Xml_seterr(xf, xs, XMLERR_UNEXPEOF);
            result = XMLRESULT_ERR;
            done = 1;
        } else if (pc == '/') {
            done = 1;
            empty = 1;
            Xml_get_char(xs); /* / */
            pc = Xml_get_char(xs); /* > */
            if (pc != '>') {
                Xml_seterr(xf, xs, XMLERR_EXPCLOSE);
                result = XMLRESULT_ERR;
            }
        } else if (pc == '>') {
            Xml_get_char(xs); /* > */
            done = 1;
        } else {
            xup_Attribute(xf, xs);
            if (xv_err(xf)) {
                done = 1;
                result = XMLRESULT_ERR;
            }
        }
    }

    if (!xv_err(xf)) {
        xup_p_element(xf, xs, el);
    }

    if (!xv_err(xf)) {
        if (xf->xi_p_element) {
            result = xf->xi_p_element(xf->xi_vp,
                            xf->xi_elNam,
                            empty,
                            xf->xi_nAttrs,
                            xf->xi_attrNames,
                            xf->xi_attrLens,
                            xf->xi_attrVals);
        }
    }

    if (empty) {
        xup_end_element(xf);
    }

    return (result);
}
/***************************************************************/
static int xup_content(struct xmls_file * xf,
                       struct xml_stream * xs,
                       struct xt_element * el) {

/*
[43]    content    ::=
           CharData? ((element | Reference | CDSect | PI | Comment) CharData?)*

[14]    CharData    ::=    [^<&]* - ([^<&]* ']]>' [^<&]*)

[18]    CDSect    ::=    CDStart CData CDEnd
[19]    CDStart    ::=    '<![CDATA['
[20]    CData    ::=    (Char* - (Char* ']]>' Char*))
[21]    CDEnd    ::=    ']]>'

[67]    Reference    ::=    EntityRef | CharRef
*/
    int result;

    result = 0;

    Xml_next_token_parsed(xf, xs, el->l_preserve_space?2:1);
    if (xv_err(xf)) return XMLRESULT_ERR;

    xup_copy_tbuf(xf, xs,
        &(xf->xi_content), &(xf->xi_contentSize), &(xf->xi_contentLen));

    if (xf->xi_contentLen && xf->xi_depth) {
        result = xup_p_content(xf);
    }

    return (result);
}
/***************************************************************/
static int xup_misc(struct xmls_file * xf,
                    struct xml_stream * xs) {
/*
[27]    Misc    ::=    Comment | PI | S
[15]    Comment    ::=    '<!--' ((Char - '-') | ('-' (Char - '-')))* '-->'
[16]    PI    ::=    '<?' PITarget (S (Char* - (Char* '?>' Char*)))? '?>'
*/
    XMLC xc;
    int result;

    result = 0;
    xc = Xml_get_char(xs);
    if (xc == '!') {
        xc = Xml_get_char(xs); /* ! */
        if (xc == '-') {
            xpp_Comment(xf, xs);
            Xml_get_char(xs); /* > */
        } else {
            Xml_seterr(xf, xs, XMLERR_EXPMISC);
            result = XMLRESULT_ERR;
        }
    } else if (xc == '?') {
        xpp_PI(xf, xs);
        if (xv_err(xf)) result = XMLRESULT_ERR;

    } else {
        Xml_seterr(xf, xs, XMLERR_EXPMISC);
        result = XMLRESULT_ERR;
    }

    return (result);
}
/***************************************************************/
static int xpp_process(struct xmls_file * xf,
                       struct xml_stream * xs) {

    int result;
    XMLC pc;
    struct xt_element * el;

    result = 0;
    while (!result) {
        if (xf->xi_depth) {
            el = xf->xc_ielements[xf->xi_depth - 1]->xc_element;
        } else {
            el = 0;
        }

        if (el && el->l_preserve_space) pc = Xml_peek_char(xs);
        else                            pc = Xml_peek_nwchar(xs);

        if (!pc) {
            if (el) {
                Xml_seterr(xf, xs, XMLERR_UNEXPEOF);
                result = XMLRESULT_ERR;
            } else {
                result = XMLRESULT_EOF;
            }
        } else if (el) {
            if (pc == '<') {
                Xml_get_char(xs); /* < */
                result = xup_element(xf, xs);
                if (!result && xv_err(xf)) result = XMLRESULT_ERR;
            } else {
                result = xup_content(xf, xs,
                            xf->xc_ielements[xf->xi_depth - 1]->xc_element);
                if (!result && xv_err(xf)) result = XMLRESULT_ERR;
            }
        } else {
            if (!xf->xi_rootElement) {
                result = xup_element(xf, xs);
                if (!result && xv_err(xf)) result = XMLRESULT_ERR;
                if (xf->xi_depth) xf->xi_rootElement = 1;
            } else if (pc == '<') {
                Xml_get_char(xs); /* < */
                result = xup_misc(xf, xs);
            } else {
                Xml_seterr(xf, xs, XMLERR_UNEXPCHARS);
                result = XMLRESULT_ERR;
            }
        }
    }

    return (result);
}
/***************************************************************/
static void Xml_begin_xml(struct xmls_file * xf) {
/*
[1]    document    ::=    prolog element Misc* - Char* RestrictedChar Char*
[2]    Char    ::=    [#x1-#xD7FF] | [#xE000-#xFFFD] | [#x10000-#x10FFFF]
[2a]    RestrictedChar    ::=
                [#x1-#x8] | [#xB-#xC] | [#xE-#x1F] | [#x7F-#x84] | [#x86-#x9F]
[27]    Misc    ::=    Comment | PI | S
[39]    element    ::=    EmptyElemTag | STag content ETag
[40]    STag    ::=    '<' Name (S Attribute)* S? '>'
[41]    Attribute    ::=    Name Eq AttValue
[42]    ETag    ::=    '</' Name S? '>'
[43]    content    ::=
           CharData? ((element | Reference | CDSect | PI | Comment) CharData?)*
[44]    EmptyElemTag    ::=    '<' Name (S Attribute)* S? '/>'
*/

    xpp_prolog(xf, xf->xv_stream);
    if (xv_err(xf)) return;
}
/***************************************************************/
int Xml_has_dtd(struct xmls_file * xf) {
/*
*/
    return (xf->xv_sFlags & XMLSFLAG_HASDTD);
}
/***************************************************************/
/* BEGIN Xml_ functions                                        */
/***************************************************************/
int Xml_process_xml(struct xmls_file * xf,
                    void * vp,
                    XML_ELEMENT p_xf_element,
                    XML_CONTENT p_xf_content,
                    XML_ENDTAG  p_xf_endtag) {

    int result;

    xf->xi_p_element = p_xf_element;
    xf->xi_p_content = p_xf_content;
    xf->xi_p_endtag  = p_xf_endtag;
    xf->xi_vp        = vp;

    if (xv_err(xf)) result = XMLRESULT_ERR;
    else            result = 0;

    if (!result) {
        result = xpp_process(xf, xf->xv_stream);
    }

    return (result);
}
/***************************************************************/
static void Xml_init_parsinfo(struct xmls_file * xf) {

    xf->xi_p_element        =    0;
    xf->xi_p_content        = 0;
    xf->xi_p_endtag         = 0;

    xf->xi_depth            = 0;
    xf->xi_elNamSize        = 0;
    xf->xi_nAttrsSize       = 0;
    xf->xi_attrNamesSize    = 0;
    xf->xi_attrValsSize     = 0;
    xf->xi_contentSize      = 0;
    xf->xi_contentLen       = 0;
    xf->xi_rootElement      = 0;

    xf->xi_elNam            = 0;
    xf->xi_nAttrs           = 0;
    xf->xi_attrNames        = 0;
    xf->xi_attrLens         = 0;
    xf->xi_attrVals         = 0;

    xf->xf_ebufsize         = XML_K_DEFAULT_EBUFSIZE;
    xf->xf_ebuf             = New(XMLC, xf->xf_ebufsize);
    xf->xf_ebuflen          = 0;
}
/***************************************************************/
struct xmls_file * Xml_new(XML_FOPEN  p_xq_fopen,
                           XML_FREAD  p_xq_fread,
                           XML_FCLOSE p_xq_fclose,
                           void * vfp,
                           int vFlags) {

    struct xmls_file * xf;

    xf = New(struct xmls_file, 1);

    /* init xmls_file */
    xf->xv_err          = 0;
    xf->xv_errparm[0]   = '\0';
    xf->xv_standalone   = 0;
    xf->xv_vers[0]      = '\0';
    xf->xv_sFlags       = 0;
    xf->xv_debug        = 0;

    /* init element content info */
    xf->xc_maxDepth     = 0;
    xf->xc_ielements    = 0;

    xf->xp_pchead       = 0;
    xf->xp_pctail       = 0;

    xf->xv_vFlags       = vFlags & XMLVFLAG_MASK;

    Xml_init_parsinfo(xf);

    xf->xq_fopen        = p_xq_fopen;
    xf->xq_fread        = p_xq_fread;
    xf->xq_fclose       = p_xq_fclose;

    xf->xv_extFile = New(struct xt_extFile, 1);
    xf->xv_extFile->fref = vfp;
    xf->xv_stream = Xml_new_stream(xf, xf->xv_extFile, NULL);

    Xml_doc_init(xf);

    Xml_autodetect_encoding(xf->xv_stream);
    if (!xv_err(xf)) Xml_begin_xml(xf);

    return (xf);
}
/***************************************************************/
void Xml_close(struct xmls_file * xf) {

    Free(xf->xv_extFile);

    Xml_free(xf);
}
/***************************************************************/
int Xml_err(struct xmls_file * xf) {

    return (xf->xv_err);
}
/***************************************************************/
char * Xml_err_msg(struct xmls_file * xf) {

    char errmsg[128];
    static char ferrmsg[256];

    switch (xf->xv_err) {
        case 0   : strcpy(errmsg,
            "No error.");
            break;
        case XMLERR_NOTXML   : strcpy(errmsg,
            "File is not a valid XML file.");
            break;
        case XMLERR_BADVERS   : strcpy(errmsg,
            "Expecting xml version 1.0 or 1.1.");
            break;
        case XMLERR_UNSUPENCODING   : strcpy(errmsg,
            "Unsupported encoding.");
            break;
        case XMLERR_BADENCMARK   : strcpy(errmsg,
            "File has conflicting/bad byte order mark.");
            break;
        case XMLERR_BADSTANDALONE   : strcpy(errmsg,
            "Bad value for standalone");
            break;
        case XMLERR_EXPLITERAL   : strcpy(errmsg,
            "Expecting quoted literal string");
            break;
        case XMLERR_BADPUBID   : strcpy(errmsg,
            "Illegal characters in literal string");
            break;
        case XMLERR_BADNAME   : strcpy(errmsg,
            "Illegal characters in name");
            break;
        case XMLERR_BADPEREF  : strcpy(errmsg,
            "Expecting semicolon at end of parameter entity reference");
            break;
        case XMLERR_BADROOT  : strcpy(errmsg,
            "Bad root element.");
            break;
        case XMLERR_EXPCLOSE: strcpy(errmsg,
            "Expecting > to close construct.");
            break;
        case XMLERR_EXPDOCTYPE: strcpy(errmsg,
            "Expecting DOCTYPE.");
            break;
        case XMLERR_DUPDOCTYPE: strcpy(errmsg,
            "Only one DOCTYPE permitted");
            break;
        case XMLERR_EXPMARKUP: strcpy(errmsg,
            "Expecting markup ENTITY, ELEMENT, ATTLIST, or NOTATION.");
            break;
        case XMLERR_EXPNDATA: strcpy(errmsg,
            "Expecting NDATA.");
            break;
        case XMLERR_EXPSYSPUB: strcpy(errmsg,
            "Expecting SYSTEM or PUBLIC.");
            break;
        case XMLERR_EXPANYEMPTY: strcpy(errmsg,
            "Expecting ANY or EMPTY.");
            break;
        case XMLERR_EXPPAREN: strcpy(errmsg,
            "Expecting opening parenthesis.");
            break;
        case XMLERR_EXPSEP: strcpy(errmsg,
            "Expecting separator , or |.");
            break;
        case XMLERR_REPEAT: strcpy(errmsg,
            "Expecting +, *, or ?.");
            break;
        case XMLERR_EXPCLOSEPAREN: strcpy(errmsg,
            "Expecting closing parenthesis.");
            break;
        case XMLERR_EXPBAR: strcpy(errmsg,
            "Expecting vertical bar.");
            break;
        case XMLERR_EXPSTAR: strcpy(errmsg,
            "Expecting asterisk after #PCDATA list.");
            break;
        case XMLERR_EXPCOMMA: strcpy(errmsg,
            "Expecting comma.");
            break;
        case XMLERR_EXPREPEAT: strcpy(errmsg,
            "Expecting *, +, or ?.");
            break;
        case XMLERR_EXPATTTYP: strcpy(errmsg,
            "Expecting valid ATTLIST type.");
            break;
        case XMLERR_EXPATTVAL: strcpy(errmsg,
            "Expecting valid ATTLIST value.");
            break;
        case XMLERR_EXPATTDEF: strcpy(errmsg,
            "Expecting valid ATTLIST default value.");
            break;
        case XMLERR_EXPPCDATA: strcpy(errmsg,
            "Expecting #PCDATA.");
            break;
        case XMLERR_NOSUCHPE: strcpy(errmsg,
            "Parameter entity not declared.");
            break;
        case XMLERR_RECURENTITY: strcpy(errmsg,
            "Infinitely recursive entities.");
            break;
        case XMLERR_BADEXTDTD: strcpy(errmsg,
            "Bad external DTD file.");
            break;
        case XMLERR_OPENSYSFILE: strcpy(errmsg,
            "Unable to access SYSTEM file.");
            break;
        case XMLERR_EXPSYSEOF: strcpy(errmsg,
            "Expecting end of SYSTEM file.");
            break;
        case XMLERR_EXPIGNINC: strcpy(errmsg,
            "Expecting IGNORE or INCLUDE.");
            break;
        case XMLERR_EXPOPBRACKET: strcpy(errmsg,
            "Expecting [ to begin IGNORE/INCLUDE.");
            break;
        case XMLERR_EXPCLBRACKET: strcpy(errmsg,
            "Expecting ] to terminate IGNORE/INCLUDE.");
            break;
        case XMLERR_EXPPEREF: strcpy(errmsg,
            "Expecting % to begin PE reference.");
            break;
        case XMLERR_BADIGNINC: strcpy(errmsg,
            "Bad IGNORE or INCLUDE PE reference.");
            break;
        case XMLERR_EXPPUBLIT: strcpy(errmsg,
            "Expecting system literal for PUBLIC.");
            break;
        case XMLERR_BADFORM  : strcpy(errmsg,
            "Expecting xml to begin with '<'");
            break;
        case XMLERR_DUPATTR  : strcpy(errmsg,
            "Duplicate attribute specified");
            break;
        case XMLERR_UNEXPEOF  : strcpy(errmsg,
            "Unexpected end of file encountered");
            break;
        case XMLERR_INTERNAL1  : strcpy(errmsg,
            "Internal problem #1");
            break;
        case XMLERR_EXPEQ  : strcpy(errmsg,
            "Expecting = after attribute name.");
            break;
        case XMLERR_BADGEREF  : strcpy(errmsg,
            "Expecting semicolon at end of entity reference");
            break;
        case XMLERR_NOSUCHGE: strcpy(errmsg,
            "Entity not declared.");
            break;
        case XMLERR_BADCHREF  : strcpy(errmsg,
            "Bad character reference");
            break;
        case XMLERR_EXPCTL  : strcpy(errmsg,
            "Expecting <!-- comment, or <!CDATA");
            break;
        case XMLERR_NOSUCHELEMENT: strcpy(errmsg,
            "Element not declared.");
            break;
        case XMLERR_NOSUCHATTLIST: strcpy(errmsg,
            "Attribute list not declared.");
            break;
        case XMLERR_DUPATTLIST: strcpy(errmsg,
            "Attribute list specified more than once.");
            break;
        case XMLERR_ATTREQ: strcpy(errmsg,
            "Required attribute not specified.");
            break;
        case XMLERR_UNEXPCHARS: strcpy(errmsg,
            "Unexpected characters in file.");
            break;
        case XMLERR_EXPMISC: strcpy(errmsg,
            "Expecting only comment or end of file.");
            break;
        case XMLERR_UNEXPCLOSE: strcpy(errmsg,
            "Mismatched close element.");
            break;

        default         : sprintf(errmsg,
            "Unrecognized XML error number %d", xf->xv_err);
            break;
    }

    if (xf->xv_errparm[0]) {
        if (xf->xv_errline[0]) {
            Snprintf(ferrmsg, sizeof(ferrmsg),
                "%s\nFound \"%s\" in line #%d:\n%s",
                    errmsg, xf->xv_errparm, xf->xv_errlinenum, xf->xv_errline);
        } else {
            Snprintf(ferrmsg, sizeof(ferrmsg),
                "%s\nFound \"%s\"", errmsg, xf->xv_errparm);
        }
    } else {
        if (xf->xv_errline[0]) {
            Snprintf(ferrmsg, sizeof(ferrmsg),
                "%s\nin line #%d:\n%s",
                 errmsg, xf->xv_errlinenum, xf->xv_errline);
        } else {
            strcpy(ferrmsg, errmsg);
        }
    }

    return (ferrmsg);
}
/***************************************************************/
/* END Xml_ functions                                          */
/***************************************************************/
struct xmlpfile {
    struct xmls_file    * xptr;         /* XML file */
    FILE                * xfref;
    struct xmltree     ** xtree;
    struct xmltree      * cxtree;
};
/***************************************************************/
struct xmltree * xmlp_new_element(char * elnam, struct xmltree * parent) {

    struct xmltree * xtree;

    xtree = New(struct xmltree, 1);
    xtree->elname  = elnam;
    xtree->nattrs  = 0;
    xtree->attrnam = 0;
    xtree->eldata  = 0;
    xtree->nsubel  = 0;
    xtree->subel   = 0;
    xtree->parent  = parent;

    if (parent) {
        parent->subel   = Realloc(parent->subel, struct xmltree *, parent->nsubel + 1);
        parent->subel[parent->nsubel] = xtree;
        parent->nsubel += 1;
    }

    return (xtree);
}
/***************************************************************/
int  xmlp_element (void * vp,
                     XMLC * elNam,
                     int     emptyFlag,
                     int     nAttrs,
                     XMLC ** attrNames,
                     int   * attrLens,
                     XMLC ** attrVals) {

    struct xmlpfile * xmlp = (struct xmlpfile *)vp;
    int elNlen;
    int aNlen;
    int ii;
    char * celNam;
    struct xmltree * nxtree;

    elNlen = xmlclen(elNam) + 1;
    celNam = New(char, elNlen);
    xmlctostr(celNam, elNam, elNlen);

    nxtree = xmlp_new_element(celNam, xmlp->cxtree);
    if (!emptyFlag) xmlp->cxtree = nxtree;
    if (!(*(xmlp->xtree))) *(xmlp->xtree) = nxtree;

    nxtree->nattrs  = nAttrs;
    if (nAttrs) {
        nxtree->attrnam = New(char *, nAttrs);
        nxtree->attrval = New(char *, nAttrs);

        for (ii = 0; ii < nAttrs; ii++) {
            aNlen = xmlclen(attrNames[ii]) + 1;
            nxtree->attrnam[ii] = New(char, aNlen);
            xmlctostr(nxtree->attrnam[ii], attrNames[ii], aNlen);

            nxtree->attrval[ii] = New(char, attrLens[ii] + 1);
            mvxmlctochar(nxtree->attrval[ii], attrVals[ii], attrLens[ii]);
            nxtree->attrval[ii][attrLens[ii]] = '\0';
        }
    }

    return (1);
}
/***************************************************************/
int  xmlp_content (void * vp, int contLen, XMLC * contVal) {

    struct xmlpfile * xmlp = (struct xmlpfile *)vp;
    char * xdat;
    int dlen;

    if (xmlp->cxtree) {
        xdat = New(char, contLen + 1);
        mvxmlctochar(xdat, contVal, contLen);
        xdat[contLen] = '\0';
        if (xmlp->cxtree->eldata) {
            dlen = strlen(xmlp->cxtree->eldata);
            xmlp->cxtree->eldata =
                Realloc(xmlp->cxtree->eldata, char, dlen + contLen + 1);
            memcpy(xmlp->cxtree->eldata + dlen, xdat, contLen);
            xmlp->cxtree->eldata[dlen + contLen] = '\0';
            Free(xdat);
        } else {
            xmlp->cxtree->eldata = xdat;
        }
    }

    return (1);
}
/***************************************************************/
int xmlp_endtag  (void * vp, XMLC * elNam) {

    struct xmlpfile * xmlp = (struct xmlpfile *)vp;

    if (xmlp->cxtree) {
        xmlp->cxtree = xmlp->cxtree->parent;
    }

    return (1);
}
/***************************************************************/
void xmlp_parse_xml_file(struct xmls_file * xf) {
/*
*/
    int done;
    int result;

    if (xv_err(xf)) return;

    xf->xi_p_element = xmlp_element;
    xf->xi_p_content = xmlp_content;
    xf->xi_p_endtag  = xmlp_endtag;
    xf->xi_vp        = xf;

    done        = 0;
    while (!done) {
        result = xpp_process(xf, xf->xv_stream);
        if (result < 0 || Xml_err(xf)) {
            done = 1;
        }
    }
}
/***************************************************************/
static void * xmlp_fopen (void *vfp,
                char * fname,
                char * fmode) {
/*
**  Read record from text file
**  Returns number of bytes in record including lf if successful
**  Returns 0 if end of file
**  Returns -1 if error
*/
    struct xmlpfile * xmlp = vfp;
    FILE * fref;

    fref = fopen(fname, fmode);

    return (fref);
}
/***************************************************************/
static long xmlp_fread (void *vfp,
                            void * rec,
                            long recsize) {
/*
**  Read record from text file
**  Returns number of bytes in record including lf if successful
**  Returns 0 if end of file
**  Returns -1 if error
*/
    long len;
    struct xmlpfile * xmlp = vfp;

    len = fread(rec, 1, recsize, xmlp->xfref);
    if (!len) {
        if (feof(xmlp->xfref)) len = 0;
        else                   len = -1;
    }

    return (len);
}
/***************************************************************/
static int xmlp_fclose(void *vfp) {
/*
**  Read record from text file
**  Returns number of bytes in record including lf if successful
**  Returns 0 if end of file
**  Returns -1 if error
*/
    struct xmlpfile * xmlp = vfp;

    fclose(xmlp->xfref);

    return (0);
}
/***************************************************************/
int xmlp_get_xmltree_from_file(const char * xmlfname,
                     struct xmltree ** pxtree,
                     char * errmsg,
                     int    errmsgsz) {

    struct xmlpfile * xmlp;
    int eno;
    int stat;

    errmsg[0] = '\0';

    xmlp = New(struct xmlpfile, 1);
    xmlp->xfref = fopen(xmlfname, "r");
    if (!xmlp->xfref) {
        eno = ERRNO;
        strncpy(errmsg, strerror(eno), errmsgsz);
        Free(xmlp);
        return (XMLE_FOPEN);
    }

    xmlp->xptr = Xml_new(xmlp_fopen,
                       xmlp_fread,
                       xmlp_fclose,
                       xmlp, 0);
    if (Xml_err(xmlp->xptr)) {
        strncpy(errmsg, Xml_err_msg(xmlp->xptr), errmsgsz);
        fclose(xmlp->xfref);
        Free(xmlp);
        return (XMLE_FNEW);
    }

    (*pxtree)    = 0;
    xmlp->xtree = pxtree;

    stat = 0;
    while (stat >= 0) {
        stat = Xml_process_xml(xmlp->xptr, xmlp,
                    xmlp_element,
                    xmlp_content,
                    xmlp_endtag);
    }

    if (Xml_err(xmlp->xptr)) {
        strncpy(errmsg, Xml_err_msg(xmlp->xptr), errmsgsz);
        stat = XMLE_PROCESS;
    } else {
        stat = 0;
    }

    Xml_close(xmlp->xptr);
    fclose(xmlp->xfref);
    Free(xmlp);

    return (stat);
}
/***************************************************************/
struct xmls_file * Xml_new_from_string(const char * xml_string,
                           int vFlags) {

    struct xmls_file * xf;

    xf = New(struct xmls_file, 1);

    /* init xmls_file */
    xf->xv_err          = 0;
    xf->xv_errparm[0]   = '\0';
    xf->xv_standalone   = 0;
    xf->xv_vers[0]      = '\0';
    xf->xv_sFlags       = 0;
    xf->xv_debug        = 0;

    /* init element content info */
    xf->xc_maxDepth     = 0;
    xf->xc_ielements    = 0;

    xf->xp_pchead       = 0;
    xf->xp_pctail       = 0;

    xf->xv_vFlags       = vFlags & XMLVFLAG_MASK;

    Xml_init_parsinfo(xf);

    xf->xq_fopen        = NULL;
    xf->xq_fread        = NULL;
    xf->xq_fclose       = NULL;

    xf->xv_extFile      = NULL;
    xf->xv_stream = Xml_new_stream(xf, xf->xv_extFile, xml_string);

    Xml_begin_xml(xf);

    Xml_doc_init(xf);

    return (xf);
}
/***************************************************************/
int xmlp_get_xmltree_from_string(const char * xml_string,
                     struct xmltree ** pxtree,
                     char * errmsg,
                     int    errmsgsz) {

    struct xmlpfile * xmlp;
    int stat;

    errmsg[0] = '\0';
    xmlp = New(struct xmlpfile, 1);
    xmlp->xfref = NULL;

    xmlp->xptr = Xml_new_from_string(xml_string, 0);

    (*pxtree)    = 0;
    xmlp->xtree = pxtree;

    stat = 0;
    while (stat >= 0) {
        stat = Xml_process_xml(xmlp->xptr, xmlp,
                    xmlp_element,
                    xmlp_content,
                    xmlp_endtag);
    }

    if (Xml_err(xmlp->xptr)) {
        strncpy(errmsg, Xml_err_msg(xmlp->xptr), errmsgsz);
        stat = XMLE_PROCESS;
    } else {
        stat = 0;
    }

    Xml_close(xmlp->xptr);
    Free(xmlp);

    return (stat);
}
/***************************************************************/
void print_xmltree(FILE * outf, struct xmltree * xtree, int depth) {

    int ii;

    for (ii = 0; ii < depth; ii++) {
        fprintf(outf, "    ");
    }

    fprintf(outf, "<%s", xtree->elname);
    for (ii = 0; ii < xtree->nattrs; ii++) {
        fprintf(outf, " %s=\"%s\"", xtree->attrnam[ii], xtree->attrval[ii]);
    }

    if (xtree->eldata || xtree->nsubel) {
        fprintf(outf, ">", xtree->elname);
        if (xtree->eldata) {
            fprintf(outf, "%s", xtree->eldata);
        }
        if (xtree->nsubel) {
            fprintf(outf, "\n");
            for (ii = 0; ii < xtree->nsubel; ii++) {
                print_xmltree(outf, xtree->subel[ii], depth + 1);
            }
            for (ii = 0; ii < depth; ii++) {
                fprintf(outf, "    ");
            }
            fprintf(outf, "</%s>\n", xtree->elname);
        } else {
            fprintf(outf, "</%s>\n", xtree->elname);
        }
    } else {
        fprintf(outf, "/>\n");
    }
}
/***************************************************************/
int write_xmltree(char * fname, struct xmltree * xtree) {
/*
** Returns 0 - Success
**         errno if not success
*/

    FILE * outf;
    int en;

    outf = fopen(fname, "w");
    if (!outf) {
        en = ERRNO;
        if (!en) return (-1);
        return (en);
    }

    fprintf(outf, "<?xml version=\"1.0\"?>\n");
    print_xmltree(outf, xtree, 0);
    fclose(outf);

    return (0);
}
/***************************************************************/
void free_xmltree(struct xmltree * xtree) {

    int ii;

    if (xtree->nsubel) {
        for (ii = 0; ii < xtree->nsubel; ii++) {
            free_xmltree(xtree->subel[ii]);
        }
        Free(xtree->subel);
    }

    if (xtree->nattrs) {
        for (ii = 0; ii < xtree->nattrs; ii++) {
            Free(xtree->attrnam[ii]);
            Free(xtree->attrval[ii]);
        }
        Free(xtree->attrnam);
        Free(xtree->attrval);
    }
    Free(xtree->eldata);
    Free(xtree->elname);
    Free(xtree);
}
/***************************************************************/
void xmlp_replace_subel_value(struct xmltree * xtree,
            int subelIx,
            char * subelVal) {

    Free(xtree->subel[subelIx]->eldata);
    xtree->subel[subelIx]->eldata = Strdup(subelVal);
}
/***************************************************************/
struct xmltree * xmlp_insert_subel(struct xmltree * xtree,
            char * subelName,
            char * subelVal,
            int    atFront) {

    struct xmltree * subel;
    int ii;

    subel = xmlp_new_element(Strdup(subelName), xtree);
    if (subelVal) subel->eldata = Strdup(subelVal);

    if (atFront) {
        for (ii = xtree->nsubel - 1; ii; ii--) {
            xtree->subel[ii] = xtree->subel[ii-1];
        }
        xtree->subel[0] = subel;
    }

    return (subel);
}
/***************************************************************/
void xmlp_delete_subel(struct xmltree * xtree, int subelIx) {

    int ii;

    if (subelIx < 0 || subelIx >= xtree->nsubel) return;

    free_xmltree(xtree->subel[subelIx]);

    for (ii = subelIx + 1; ii < xtree->nsubel; ii++) {
        xtree->subel[ii-1] = xtree->subel[ii];
    }
    xtree->nsubel -= 1;
}
/***************************************************************/
void xmlp_remove_subel(struct xmltree * xtree, int subelIx) {

    int ii;

    Free(xtree->subel[subelIx]);

    xtree->nsubel -= 1;
    for (ii = subelIx; ii < xtree->nsubel; ii++) {
        xtree->subel[ii] = xtree->subel[ii+1];
    }
}
/***************************************************************/
void xmlp_insert_attr(struct xmltree * xtree,
            const char * attrName,
            const char * attrVal,
            int    atFront) {

    int ii;

    xtree->attrnam = Realloc(xtree->attrnam, char *, xtree->nattrs + 1);
    xtree->attrval = Realloc(xtree->attrval, char *, xtree->nattrs + 1);

    if (atFront) {
        for (ii = xtree->nattrs - 1; ii; ii--) {
            xtree->attrnam[ii] = xtree->attrnam[ii-1];
            xtree->attrval[ii] = xtree->attrval[ii-1];
        }
        xtree->attrnam[0] = Strdup(attrName);
        if (attrVal) xtree->attrval[0] = Strdup(attrVal);
    } else {
        xtree->attrnam[xtree->nattrs] = Strdup(attrName);
        if (attrVal) xtree->attrval[xtree->nattrs] = Strdup(attrVal);
    }
    xtree->nattrs += 1;
}
/***************************************************************/
void xmlp_insert_subel_tree(struct xmltree * xtree,
            struct xmltree * subel,
            int    atFront) {

    int ii;

    xtree->subel   = Realloc(xtree->subel, struct xmltree *, xtree->nsubel + 1);
    xtree->subel[xtree->nsubel] = subel;
    xtree->nsubel += 1;

    if (atFront) {
        for (ii = xtree->nsubel - 1; ii; ii--) {
            xtree->subel[ii] = xtree->subel[ii-1];
        }
        xtree->subel[0] = subel;
    }
}
/***************************************************************/
static void xtts_raw_string(char ** xtts_buf,
                int  *  xtts_len,
                int  *  xtts_max,
                const char * tstr)
{
    int tlen;

    tlen = IStrlen(tstr);
    if ((*xtts_len) + tlen >= (*xtts_max)) {
        if (!(*xtts_max)) (*xtts_max) = 64;
        else              (*xtts_max) *= 2;
        /* check xtts_max again, in case tlen is giant */
        if ((*xtts_len) + tlen >= (*xtts_max))
            (*xtts_max) = (*xtts_len) + tlen + 64;
        (*xtts_buf) = Realloc((*xtts_buf), char, (*xtts_max));
    }
    memcpy((*xtts_buf) + (*xtts_len), tstr, tlen + 1);
    (*xtts_len) += tlen;
}
/***************************************************************/
static void xtts_encode(char ** xtts_buf,
                int  *  xtts_len,
                int  *  xtts_max,
                const char * tstr)
{
                xtts_raw_string(xtts_buf, xtts_len, xtts_max, tstr);
}
/***************************************************************/
static void xtts_add_buf(char ** xtts_buf,
                int  *  xtts_len,
                int  *  xtts_max,
                unsigned int add_buf,
                ...) {
/*
** add_buf: in hex using 4 bits per string
**      0 - no string
**      1 - raw string, do not encode
**      2 - string with XML encoding
**
*/
    va_list args;
    unsigned int rev_add_buf;
    unsigned int ab;
    int pt;
    char * tstr;

    ab = add_buf;
    rev_add_buf = 0;

    while (ab) {
        rev_add_buf = (rev_add_buf << 4) | (ab & 0x0F);
        ab >>= 4;
    }

    va_start(args, add_buf);
    while (rev_add_buf) {
        tstr = va_arg(args,char*);
        pt = rev_add_buf & 0x0F;
        switch (pt) {
            case 1:
                xtts_raw_string(xtts_buf, xtts_len, xtts_max, tstr);
                break;
            case 2:
                xtts_encode(xtts_buf, xtts_len, xtts_max, tstr);
                break;
            default:
                break;
        }
        rev_add_buf >>= 4;
    }
    va_end(args);
}
/***************************************************************/
static void xtts_main(struct xmltree * xtree,
                int xtts_flags,
                char ** xtts_buf,
                int  *  xtts_len,
                int  *  xtts_max,
                int     depth) {

    int ii;

    if (xtts_flags & XTTS_INDENT) {
        for (ii = 0; ii < depth; ii++) {
            xtts_add_buf(xtts_buf, xtts_len, xtts_max, 0x1, "    ");
        }
    }

    xtts_add_buf(xtts_buf, xtts_len, xtts_max, 0x11,
            "<", xtree->elname);
    for (ii = 0; ii < xtree->nattrs; ii++) {
        xtts_add_buf(xtts_buf, xtts_len, xtts_max, 0x11121,
            " ", xtree->attrnam[ii], "=\"", xtree->attrval[ii], "\"");
    }

    if (xtree->eldata || xtree->nsubel) {
        if (xtree->eldata) {
            xtts_add_buf(xtts_buf, xtts_len, xtts_max, 0x12,
                    ">", xtree->eldata);
        } else {
            xtts_add_buf(xtts_buf, xtts_len, xtts_max, 0x1, ">");
        }
        if (xtree->nsubel) {
            if (xtts_flags & XTTS_NL) {
                xtts_add_buf(xtts_buf, xtts_len, xtts_max, 0x1, "\n");
            }
            for (ii = 0; ii < xtree->nsubel; ii++) {
                xtts_main(xtree->subel[ii], xtts_flags,
                    xtts_buf, xtts_len, xtts_max, depth + 1);
            }
            if (xtts_flags & XTTS_INDENT) {
                for (ii = 0; ii < depth; ii++) {
                    xtts_add_buf(xtts_buf, xtts_len, xtts_max, 0x1, "    ");
                }
            }
        }
        if (xtts_flags & XTTS_NL) {
            xtts_add_buf(xtts_buf, xtts_len, xtts_max, 0x111,
                "</", xtree->elname, ">\n");
        } else {
            xtts_add_buf(xtts_buf, xtts_len, xtts_max, 0x111,
                "</", xtree->elname, ">");
        }
    } else {
        if (xtts_flags & XTTS_NL) {
            xtts_add_buf(xtts_buf, xtts_len, xtts_max, 0x1, "/>\n");
        } else {
            xtts_add_buf(xtts_buf, xtts_len, xtts_max, 0x1, "/>");
        }
    }
}
/***************************************************************/
char * xmltree_to_string(struct xmltree * xtree, int xtts_flags) {

    char * xtts_buf;
    int    xtts_len;
    int    xtts_max;

    if (!xtree) return Strdup("");

    xtts_buf = NULL;
    xtts_len = 0;
    xtts_max = 0;

    if (xtts_flags & XTTS_XML_HEADER) {
        xtts_add_buf(&xtts_buf, &xtts_len, &xtts_max, 0x1,
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
        if (xtts_flags & XTTS_NL) {
            xtts_add_buf(&xtts_buf, &xtts_len, &xtts_max, 0x1,
                "\n");
        }
    }

    xtts_main(xtree, xtts_flags, &xtts_buf, &xtts_len, &xtts_max, 0);

    return (xtts_buf);
}
/***************************************************************/
