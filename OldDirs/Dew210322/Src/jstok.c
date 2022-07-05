/***************************************************************/
/*
**  jstok.c --  JavaScript tokens
*/
/***************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>

#include "dew.h"
#include "jsenv.h"
#include "jstok.h"
#include "dbtreeh.h"
#include "snprfh.h"
#include "utl.h"

/***************************************************************/
struct js_reserved_words {
    char * jrw_name;
    short  jrw_rw;
    short  jrw_type;
    short  jrw_kwinfo;
    short  jrw__filler1;
} js_reserved_word_list[] = {
    { "abstract"      , JSRW_ABSTRACT        , JSW_KW  , JSKWI_KEYWORD   },
    { "arguments"     , JSKW_ARGUMENTS       , JSW_KW  , JSKWI_KEYWORD | JSKWI_VAR_OK },
    { "await"         , JSKW_AWAIT           , JSW_KW  , JSKWI_RW        },
    { "boolean"       , JSKW_BOOLEAN         , JSW_KW  , JSKWI_KEYWORD   },
    { "break"         , JSKW_BREAK           , JSW_KW  , JSKWI_RW        },
    { "byte"          , JSKW_BYTE            , JSW_KW  , JSKWI_KEYWORD   },
    { "case"          , JSKW_CASE            , JSW_KW  , JSKWI_RW        },
    { "catch"         , JSKW_CATCH           , JSW_KW  , JSKWI_RW        },
    { "char"          , JSKW_CHAR            , JSW_KW  , JSKWI_KEYWORD   },
    { "class"         , JSKW_CLASS           , JSW_KW  , JSKWI_RW        },
    { "const"         , JSKW_CONST           , JSW_KW  , JSKWI_RW        },
    { "continue"      , JSKW_CONTINUE        , JSW_KW  , JSKWI_RW        },
#if IS_DEBUG
    { "debug"         , JSKW_DEBUG           , JSW_KW  , JSKWI_RW        },
#endif
    { "debugger"      , JSKW_DEBUGGER        , JSW_KW  , JSKWI_RW        },
    { "default"       , JSKW_DEFAULT         , JSW_KW  , JSKWI_RW        },
    { "delete"        , JSKW_DELETE          , JSW_KW  , JSKWI_KEYWORD   },
    { "do"            , JSKW_DO              , JSW_KW  , JSKWI_RW        },
    { "double"        , JSKW_DOUBLE          , JSW_KW  , JSKWI_KEYWORD   },
    { "else"          , JSKW_ELSE            , JSW_KW  , JSKWI_RW | JSKWI_NO_UPDATE },
    { "enum"          , JSKW_ENUM            , JSW_KW  , JSKWI_RW        },
    { "eval"          , JSKW_EVAL            , JSW_KW  , JSKWI_KEYWORD   },
    { "export"        , JSKW_EXPORT          , JSW_KW  , JSKWI_RW        },
    { "extends"       , JSKW_EXTENDS         , JSW_KW  , JSKWI_RW        },
    { "false"         , JSKW_FALSE           , JSW_KW  , JSKWI_RW        },
    { "final"         , JSKW_FINAL           , JSW_KW  , JSKWI_KEYWORD   },
    { "finally"       , JSKW_FINALLY         , JSW_KW  , JSKWI_RW        },
    { "float"         , JSKW_FLOAT           , JSW_KW  , JSKWI_KEYWORD   },
    { "for"           , JSKW_FOR             , JSW_KW  , JSKWI_RW        },
    { "function"      , JSKW_FUNCTION        , JSW_KW  , JSKWI_RW        },
    { "get"           , JSKW_GET             , JSW_KW  , JSKWI_KEYWORD | JSKWI_VAR_OK },
    { "goto"          , JSKW_GOTO            , JSW_KW  , JSKWI_KEYWORD   },
    { "if"            , JSKW_IF              , JSW_KW  , JSKWI_RW | JSKWI_NO_UPDATE },
    { "implements"    , JSKW_IMPLEMENTS      , JSW_KW  , JSKWI_KEYWORD   },
    { "import"        , JSKW_IMPORT          , JSW_KW  , JSKWI_RW        },
    { "in"            , JSKW_IN              , JSW_KW  , JSKWI_RW        },
    { "instanceof"    , JSKW_INSTANCEOF      , JSW_KW  , JSKWI_RW        },
    { "int"           , JSKW_INT             , JSW_KW  , JSKWI_KEYWORD   },
    { "interface"     , JSKW_INTERFACE       , JSW_KW  , JSKWI_KEYWORD   },
    { "let"           , JSKW_LET             , JSW_KW  , JSKWI_KEYWORD   },
    { "long"          , JSKW_LONG            , JSW_KW  , JSKWI_KEYWORD   },
    { "native"        , JSKW_NATIVE          , JSW_KW  , JSKWI_KEYWORD   },
    { "new"           , JSKW_NEW             , JSW_KW  , JSKWI_RW        },
    { "null"          , JSKW_NULL            , JSW_KW  , JSKWI_RW        },
    { "package"       , JSKW_PACKAGE         , JSW_KW  , JSKWI_KEYWORD   },
    { "private"       , JSKW_PRIVATE         , JSW_KW  , JSKWI_KEYWORD   },
    { "protected"     , JSKW_PROTECTED       , JSW_KW  , JSKWI_KEYWORD   },
    { "public"        , JSKW_PUBLIC          , JSW_KW  , JSKWI_KEYWORD   },
    { "return"        , JSKW_RETURN          , JSW_KW  , JSKWI_RW        },
    { "set"           , JSKW_SET             , JSW_KW  , JSKWI_KEYWORD | JSKWI_VAR_OK },
    { "short"         , JSKW_SHORT           , JSW_KW  , JSKWI_KEYWORD   },
    { "static"        , JSKW_STATIC          , JSW_KW  , JSKWI_KEYWORD   },
    { "super"         , JSKW_SUPER           , JSW_KW  , JSKWI_RW        },
    { "switch"        , JSKW_SWITCH          , JSW_KW  , JSKWI_RW        },
    { "synchronized"  , JSKW_SYNCHRONIZED    , JSW_KW  , JSKWI_KEYWORD   },
    { "this"          , JSKW_THIS            , JSW_KW  , JSKWI_RW        },
    { "throw"         , JSKW_THROW           , JSW_KW  , JSKWI_RW        },
    { "throws"        , JSKW_THROWS          , JSW_KW  , JSKWI_KEYWORD   },
    { "transient"     , JSKW_TRANSIENT       , JSW_KW  , JSKWI_KEYWORD   },
    { "true"          , JSKW_TRUE            , JSW_KW  , JSKWI_RW        },
    { "try"           , JSKW_TRY             , JSW_KW  , JSKWI_RW        },
    { "typeof"        , JSKW_TYPEOF          , JSW_KW  , JSKWI_RW        },
    { "var"           , JSKW_VAR             , JSW_KW  , JSKWI_RW        },
    { "void"          , JSKW_VOID            , JSW_KW  , JSKWI_RW        },
    { "volatile"      , JSKW_VOLATILE        , JSW_KW  , JSKWI_KEYWORD   },
    { "while"         , JSKW_WHILE           , JSW_KW  , JSKWI_RW | JSKWI_NO_UPDATE },
    { "with"          , JSKW_WITH            , JSW_KW  , JSKWI_RW        },
    { "yield"         , JSKW_YIELD           , JSW_KW  , JSKWI_RW        },
    { "#"               , 0                        , 0                   } };
static int nb_reserved_words = 0;

struct js_punctuators {
    char * jpu_name;
    int    jpu_pu;
    int    jpu_type;
} js_punctuator_list[] = {
    { "!"     , JSPU_BANG               , JSW_PU      },
    { "!="    , JSPU_BANG_EQ            , JSW_PU      },
    { "!=="   , JSPU_BANG_EQ_EQ         , JSW_PU      },
    { "%"     , JSPU_PECENT             , JSW_PU      },
    { "%="    , JSPU_PERCENT_EQ         , JSW_PU      },
    { "&"     , JSPU_AMP                , JSW_PU      },
    { "&&"    , JSPU_AMP_AMP            , JSW_PU      },
    { "&="    , JSPU_AMP_EQ             , JSW_PU      },
    { "("     , JSPU_OPEN_PAREN         , JSW_PU      },
    { ")"     , JSPU_CLOSE_PAREN        , JSW_PU      },
    { "*"     , JSPU_STAR               , JSW_PU      },
    { "**"    , JSPU_STAR_STAR          , JSW_PU      },
    { "**="   , JSPU_STAR_STAR_EQ       , JSW_PU      },
    { "*="    , JSPU_STAR_EQ            , JSW_PU      },
    { "+"     , JSPU_PLUS               , JSW_PU      },
    { "++"    , JSPU_PLUS_PLUS          , JSW_PU      },
    { "+="    , JSPU_PLUS_EQ            , JSW_PU      },
    { ","     , JSPU_COMMA              , JSW_PU      },
    { "-"     , JSPU_MINUS              , JSW_PU      },
    { "--"    , JSPU_MINUS_MINUS        , JSW_PU      },
    { "-="    , JSPU_MINUS_EQ           , JSW_PU      },
    { "."     , JSPU_DOT                , JSW_PU      },
    { "..."   , JSPU_DOT_DOT_DOT        , JSW_PU      },
    { "/"     , JSPU_SLASH              , JSW_PU      },
    { "/="    , JSPU_SLASH_EQ           , JSW_PU      },
    { ":"     , JSPU_COLON              , JSW_PU      },
    { ";"     , JSPU_SEMICOLON          , JSW_PU      },
    { "<"     , JSPU_LT                 , JSW_PU      },
    { "<<"    , JSPU_LT_LT              , JSW_PU      },
    { "<<="   , JSPU_LT_LT_EQ           , JSW_PU      },
    { "<="    , JSPU_LT_EQ              , JSW_PU      },
    { "="     , JSPU_EQ                 , JSW_PU      },
    { "=="    , JSPU_EQ_EQ              , JSW_PU      },
    { "==="   , JSPU_EQ_EQ_EQ           , JSW_PU      },
    { "=>"    , JSPU_EQ_GT              , JSW_PU      },
    { ">"     , JSPU_GT                 , JSW_PU      },
    { ">="    , JSPU_GT_EQ              , JSW_PU      },
    { ">>"    , JSPU_GT_GT              , JSW_PU      },
    { ">>="   , JSPU_GT_GT_EQ           , JSW_PU      },
    { ">>>"   , JSPU_GT_GT_GT           , JSW_PU      },
    { ">>>="  , JSPU_GT_GT_GT_EQ        , JSW_PU      },
    { "?"     , JSPU_QUESTION           , JSW_PU      },
    { "?."    , JSPU_QUESTION_DOT       , JSW_PU      },
    { "??"    , JSPU_QUESTION_QUESTION  , JSW_PU      },
#if IS_DEBUG
    { "@"     , JSPU_AT                 , JSW_PU      },
    { "@@"    , JSPU_AT_AT              , JSW_PU      },
#endif
    { "["     , JSPU_OPEN_BRACKET       , JSW_PU      },
    { "]"     , JSPU_CLOSE_BRACKET      , JSW_PU      },
    { "^"     , JSPU_HAT                , JSW_PU      },
    { "^="    , JSPU_HAT_EQ             , JSW_PU      },
    { "{"     , JSPU_OPEN_BRACE         , JSW_PU      },
    { "|"     , JSPU_BAR                , JSW_PU      },
    { "|="    , JSPU_BAR_EQ             , JSW_PU      },
    { "||"    , JSPU_BAR_BAR            , JSW_PU      },
    { "}"     , JSPU_CLOSE_BRACE        , JSW_PU      },
    { "~"     , JSPU_TILDE              , JSW_PU      },
    { "#"     , 0                       , 0           } };
static int nb_punctuators = 0;

/***************************************************************/
static short jrun_calc_nb_reserved_words(const struct js_reserved_words * rwl)
{
/*
**
*/
    int ix;

    for (ix = 1; rwl[ix].jrw_name[0] != '#'; ix++) {
        if (strcmp(rwl[ix-1].jrw_name,
                    rwl[ix].jrw_name) >= 0) {
            printf("Internal problem with reserved word names: %s < %s\n",
                        rwl[ix].jrw_name,
                        rwl[ix-1].jrw_name);
        }
        if (rwl[ix-1].jrw_rw >=
            rwl[ix].jrw_rw) {
            printf("Internal problem with reserved word values: %d < %d\n",
                        rwl[ix].jrw_rw,
                        rwl[ix-1].jrw_rw);
        }
    }
    return (ix);
}
/***************************************************************/
static int jrun_find_keyword_index(
        const struct js_reserved_words * rwl,
        int nrw,
        const char * kwnam)
{
/*
**
*/
    int lo;
    int hi;
    int mid;

    lo = 0;
    hi = nrw - 1;
    while (lo <= hi) {
        mid = (lo + hi) / 2;
        if (strcmp(kwnam, rwl[mid].jrw_name) <= 0) hi = mid - 1;
        else                                       lo = mid + 1;
    }

    return (lo);
}
/***************************************************************/
static struct js_reserved_words * jrun_find_keyword(const char * kwnam)
{
/*
**
*/
    int ix;
    struct js_reserved_words * jsrw;

    if (!nb_reserved_words) {
        nb_reserved_words = jrun_calc_nb_reserved_words(js_reserved_word_list);
    }

    ix = jrun_find_keyword_index(js_reserved_word_list, nb_reserved_words, kwnam);
    if (ix < nb_reserved_words && !strcmp(kwnam, js_reserved_word_list[ix].jrw_name)) {
        jsrw = &(js_reserved_word_list[ix]);
    } else {
        jsrw = NULL;
    }

    return (jsrw);
}
/***************************************************************/
static int jrun_find_keyword_name_index(
        const struct js_reserved_words * rwl,
        int nrw,
        short kw)
{
/*
**
*/
    int lo;
    int hi;
    int mid;

    lo = 0;
    hi = nrw - 1;
    while (lo <= hi) {
        mid = (lo + hi) / 2;
        if (kw <= rwl[mid].jrw_rw) hi = mid - 1;
        else                       lo = mid + 1;
    }

    return (lo);
}
/***************************************************************/
static char * jrun_find_keyword_name(short kw)
{
/*
**
*/
    char * kwnam;
    int ix;

    if (!nb_reserved_words) {
        nb_reserved_words = jrun_calc_nb_reserved_words(js_reserved_word_list);
    }

    ix = jrun_find_keyword_name_index(js_reserved_word_list, nb_reserved_words, kw);
    if (ix < nb_reserved_words && js_reserved_word_list[ix].jrw_rw == kw) {
        kwnam = js_reserved_word_list[ix].jrw_name;
    } else {
        kwnam = NULL;
    }

    return (kwnam);
}
/***************************************************************/
static short jrun_calc_nb_punctuators(const struct js_punctuators * pul)
{
/*
**
*/
    int ix;

    for (ix = 1; pul[ix].jpu_name[0] != '#'; ix++) {
        if (strcmp(pul[ix-1].jpu_name,
                    pul[ix].jpu_name) >= 0) {
            printf("Internal problem with punctuator names: %s < %s\n",
                        pul[ix].jpu_name,
                        pul[ix-1].jpu_name);
        }
        if (pul[ix-1].jpu_pu >=
            pul[ix].jpu_pu) {
            printf("Internal problem with punctuator values: %d < %d\n",
                        pul[ix].jpu_pu,
                        pul[ix-1].jpu_pu);
        }
    }
    return (ix);
}
/***************************************************************/
void jrun_find_punctuator_name_by_pu(short pu, char * punam)
{
/*
**
*/
    int ix;
    int found;

    found = 0;
    ix = 0;
    while (!found && ix < nb_punctuators) {
        if (js_punctuator_list[ix].jpu_pu == pu) found = 1;
        else ix++;
    }

    if (!found) {
        punam[0] = '\0';
    } else {
        strcpy(punam, js_punctuator_list[ix].jpu_name);
    }
}
/***************************************************************/
static int jrun_find_punctuator_index(
        const struct js_punctuators * pul,
        int npu,
        const char * punam)
{
/*
**
*/
    int lo;
    int hi;
    int mid;

    lo = 0;
    hi = npu - 1;
    while (lo <= hi) {
        mid = (lo + hi) / 2;
        if (strcmp(punam, pul[mid].jpu_name) <= 0) hi = mid - 1;
        else                                       lo = mid + 1;
    }

    return (lo);
}
/***************************************************************/
static short jrun_find_punctuator(const char * punam)
{
/*
**
*/
    int ix;
    short pu;

    if (!nb_punctuators) {
        nb_punctuators = jrun_calc_nb_punctuators(js_punctuator_list);
    }

    ix = jrun_find_punctuator_index(js_punctuator_list, nb_punctuators, punam);
    if (ix < nb_punctuators && !strcmp(punam, js_punctuator_list[ix].jpu_name)) {
        pu = js_punctuator_list[ix].jpu_pu;
    } else {
        pu = 0;
    }

    return (pu);
}
/***************************************************************/
static int jrun_find_punctuator_index_exact(const char * punam)
{
/*
**
*/
    int puix;

    if (!nb_punctuators) {
        nb_punctuators = jrun_calc_nb_punctuators(js_punctuator_list);
    }

    puix = jrun_find_punctuator_index(js_punctuator_list, nb_punctuators, punam);
    if (puix >= nb_punctuators  || strcmp(punam, js_punctuator_list[puix].jpu_name)) {
        puix = -1;
    }

    return (puix);
}
/***************************************************************/
static int jrun_find_punctuator_name_index(
        const struct js_punctuators * pul,
        int npu,
        short pu)
{
/*
**
*/
    int lo;
    int hi;
    int mid;

    lo = 0;
    hi = npu - 1;
    while (lo <= hi) {
        mid = (lo + hi) / 2;
        if (pu <= pul[mid].jpu_pu) hi = mid - 1;
        else                       lo = mid + 1;
    }

    return (lo);
}
/***************************************************************/
static char * jrun_find_punctuator_name(short pu)
{
/*
**
*/
    char * punam;
    int ix;

    if (!nb_punctuators) {
        nb_punctuators = jrun_calc_nb_punctuators(js_punctuator_list);
    }

    ix = jrun_find_punctuator_name_index(js_punctuator_list, nb_punctuators, pu);
    if (ix < nb_punctuators && js_punctuator_list[ix].jpu_pu == pu) {
        punam = js_punctuator_list[ix].jpu_name;
    } else {
        punam = NULL;
    }

    return (punam);
}
/***************************************************************/
struct jtoken * jtok_new_jtoken(void)
{
    struct jtoken * jtok;

    jtok = New(struct jtoken, 1);
    jtok->jtok_tlen  = 0;
    jtok->jtok_tmax  = 0;
    jtok->jtok_text  = NULL;
    jtok->jtok_flags = 0;
    jtok->jtok_typ   = 0;
    jtok->jtok_kw    = 0;

    return (jtok);
}
/***************************************************************/
void jtok_free_jtoken(struct jtoken * jtok)
{
    Free(jtok->jtok_text);
    Free(jtok);
}
/***************************************************************/
static HTMLC jtok_get_char(struct jrunexec * jx,
        const char * jstr,
        int * jstrix)
{
/*
**
*/
    HTMLC jchar;
    unsigned char cb[HTTPCMAXSZ];

    cb[0] = jstr[*jstrix];
    if (cb[0] < 0x80) {
        if (cb[0]) (*jstrix)++;
        jchar = cb[0];
    } else if (cb[0] < 0xC2) {
        /* continuation or overlong 2-byte sequence */
        /* goto ERROR1; */
        (*jstrix)++;
        jchar = 0;
    } else if (cb[0] < 0xE0) {
        /* 2-byte sequence */
        (*jstrix)++;
        cb[1] = jstr[*jstrix];
        if ((cb[1] & 0xC0) != 0x80) {
            /* goto ERROR2; */
            if (cb[1]) (*jstrix)++;
            jchar = 0;
        } else {
            jchar = (cb[0] << 6) + cb[1] - 0x3080;
            (*jstrix)++;
        }
    } else if (cb[0] < 0xF0) {
        /* 3-byte sequence */
        (*jstrix)++;
        cb[1] = jstr[*jstrix];
        if ((cb[1] & 0xC0) != 0x80) {
            /* goto ERROR2; */
            if (cb[0]) (*jstrix)++;
            jchar = 0;
        } else {
            if (cb[0] == 0xE0 && cb[1] < 0xA0) {
                /* goto ERROR2; */ /* overlong */
                if (cb[1]) (*jstrix)++;
                jchar = 0;
            } else {
                cb[2] = jstr[*jstrix];
                if ((cb[2] & 0xC0) != 0x80) {
                    /* goto ERROR3; */
                    if (cb[2]) (*jstrix)++;
                    jchar = 0;
                } else {
                    jchar = (cb[0] << 12) + (cb[1] << 6) + cb[2] - 0xE2080;
                }
            }
        }
    } else if (cb[0] < 0xF5) {
        /* 4-byte sequence */
        (*jstrix)++;
        jchar = 0;
    } else {
        /* > U+10FFFF */
        /* goto ERROR1; */
        (*jstrix)++;
        jchar = 0;
    }

    return (jchar);
}
/***************************************************************/
static HTMLC jtok_next_char(struct jrunexec * jx,
        const char * jstr,
        int * jstrix)
{
    int save_jstrix;
    HTMLC jchar;

    save_jstrix = (*jstrix);
    jchar = jtok_get_char(jx, jstr, jstrix);
    (*jstrix) = save_jstrix;

    return (jchar);
}
/***************************************************************/
static HTMLC jtok_xlate_backslash(struct jrunexec * jx,
        const char * jstr,
        int * jstrix,
        int * is_err)
{
/*
** Translate a backslashed character
*/

    HTMLC jchar;
    HTMLC out;
    char hx;
    int ii;

    (*is_err) = 0;
    jchar = JTOK_CURRENT_CHAR(jstr, jstrix);

    switch (jchar) {
        case '0':       out = '\0'; break; /* null */
        case 'a':       out = '\a'; break; /* bell */
        case 'b':       out = '\b'; break; /* backspace */
        case 'n':       out = '\n'; break; /* newline */
        case 'r':       out = '\r'; break; /* return */
        case 't':       out = '\t'; break; /* tab */
        case 'v':       out = '\v'; break; /* verticle tab */
        case 'x':                          /* Hexadecimal 2 digits */
            out = 0;
            for (ii = 0; ii < 2; ii++) {
                (*jstrix)++;
                hx = JTOK_CURRENT_CHAR(jstr, jstrix);
                if (hx >= '0' && hx <= '9')      out = (out << 4) | (hx - '0');
                else if (hx >= 'A' && hx <= 'F') out = (out << 4) | ((hx - 'A') + 10);
                else if (hx >= 'a' && hx <= 'f') out = (out << 4) | ((hx - 'a') + 10);
                else { (*is_err) = 1; return (0); }
            }
            break;
        case 'u':                          /* Hexadecimal digits */
            out = 0;
            for (ii = 0; ii < 4; ii++) {
                (*jstrix)++;
                hx = JTOK_CURRENT_CHAR(jstr, jstrix);
                if (hx >= '0' && hx <= '9')      out = (out << 4) | (hx - '0');
                else if (hx >= 'A' && hx <= 'F') out = (out << 4) | ((hx - 'A') + 10);
                else if (hx >= 'a' && hx <= 'f') out = (out << 4) | ((hx - 'a') + 10);
                else { (*is_err) = 1; return (0); }
            }
            break;
        case '\\':      out = '\\'; break; /* backslash */
        case '\'':      out = '\''; break; /* apostrophe */
        case '\"':      out = '\"'; break; /* double quote */

        default:
            (*is_err) = 1;
            out = jchar;
            break;
    }

    if (*is_err) jchar = 0;
    else (*jstrix)++;

    return (out);
}
/***************************************************************/
static void jtok_ensure_token_space(struct jtoken * jtok, int nchars)
{
/*
**
*/
    if (jtok->jtok_tlen + nchars >= jtok->jtok_tmax) {
        if (!jtok->jtok_tmax) jtok->jtok_tmax = (nchars>8)?nchars:8;
        else                  jtok->jtok_tmax += nchars + 8;
        jtok->jtok_text = Realloc(jtok->jtok_text, char, jtok->jtok_tmax);
    }
}
/***************************************************************/
static void jtok_append_token(struct jtoken * jtok, HTMLC jchar)
{
/*
**
*/
    jtok_ensure_token_space(jtok, 4);

    /* put as utf-8 */
    if (jchar < 0x80) {
        jtok->jtok_text[jtok->jtok_tlen++] = (unsigned char)(jchar & 0xFF);
    } else if (jchar <= 0x7FF) {
        jtok->jtok_text[jtok->jtok_tlen++] = (unsigned char)((jchar >> 6) + 0xC0);
        jtok->jtok_text[jtok->jtok_tlen++] = (unsigned char)((jchar & 0x3F) + 0x80);
    } else {
        jtok->jtok_text[jtok->jtok_tlen++] = (unsigned char)((jchar >> 12) + 0xE0);
        jtok->jtok_text[jtok->jtok_tlen++] = (unsigned char)(((jchar >> 6) & 0x3F) + 0x80);
        jtok->jtok_text[jtok->jtok_tlen++] = (unsigned char)((jchar & 0x3F) + 0x80);
    }
}
/***************************************************************/
static void jtok_append_token_null(struct jtoken * jtok)
{
/*
**
*/
    jtok_ensure_token_space(jtok, 1);

    /* put as utf-8 */
    jtok->jtok_text[jtok->jtok_tlen] = '\0';
}
/***************************************************************/
static int jtok_get_token_quote(struct jrunexec * jx,
        HTMLC qchar,
        struct jtoken * jtok,
        const char * jstr,
        int * jstrix)
{
/*
**
*/
    int jstat = 0;
    int done;
    int is_err;
    HTMLC jchar;

    done = 0;
    jtok_append_token(jtok, qchar);

    while (!done) {
        if (JTOK_CURRENT_CHAR(jstr,jstrix) == '\\') {
            jchar = JTOK_NEXT_CHAR(jstr, jstrix);
            if (JTOK_LINETERM_CHAR(jchar)) {
                (*jstrix) += 2;    /* skip '\n' or '\r' */
            } else {
                (*jstrix)++;
                jchar = jtok_xlate_backslash(jx, jstr, jstrix, &is_err);
                jtok_append_token(jtok, jchar);
                if (is_err) {
                    done = 1;
                    jstat = jrun_set_error(jx, JSERR_INVALID_ESCAPE,
                            "Invalid escape sequence");
                    jtok->jtok_typ = JSW_ERROR;
                }
            }
        } else {
            jchar = jtok_get_char(jx, jstr, jstrix);
            if (!jchar) {
                done = 1;
                jstat = jrun_set_error(jx, JSERR_UNMATCHED_QUOTE,
                        "Unmatched quotation");
                jtok->jtok_typ = JSW_ERROR;
            } else {
                if (jchar == qchar) done = 1;
                jtok_append_token(jtok, jchar);
            }
        }
    }
    jtok_append_token_null(jtok);

    return (jstat);
}
/***************************************************************/
static void jtok_advance_to_end_of_line(struct jrunexec * jx,
        const char * jstr,
        int * jstrix)
{
/*
**
*/
    HTMLC jchar;

    do {
        jchar = jtok_get_char(jx, jstr, jstrix);
    } while (jchar && !JTOK_LINETERM_CHAR(jchar));
}
/***************************************************************/
static void jtok_advance_to_end_of_comment(struct jrunexec * jx,
        const char * jstr,
        int * jstrix)
{
/*
**
*/
    HTMLC jchar;
    HTMLC njchar;
    int done;

    done = 0;
    while (!done) {
        jchar = jtok_get_char(jx, jstr, jstrix);
        if (!jchar) {
            done = 1;
        } else if (jchar == '*') {
            njchar = jtok_next_char(jx, jstr, jstrix);
            if (!njchar || njchar == '/') {
                njchar = jtok_get_char(jx, jstr, jstrix); /* '/' */
                done = 1;
            }
        }
    }
}
/***************************************************************/
static HTMLC jtok_skip_spaces(struct jrunexec * jx,
        const char * jstr,
        int * jstrix,
        short * tokflags)
{
/*
**
*/
    HTMLC jchar;
    HTMLC njchar;
    int done;

    done = 0;
    while (!done) {
        if (JTOK_CURRENT_CHAR(jstr,jstrix) == '\\') {
            done = 1;
            jchar = jtok_get_char(jx, jstr, jstrix);    /* '\\' */
        } else {
            jchar = jtok_get_char(jx, jstr, jstrix);
            if (!JTOK_ISSPACE_CHAR(jchar)) {
                if (JTOK_LINETERM_CHAR(jchar)) {
                    (*tokflags) |= JTOK_FLAG_PRE_LINETERM;
                } else if (jchar == '/') {
                    njchar = jtok_next_char(jx, jstr, jstrix);
                    if (njchar == '/') {
                        jtok_advance_to_end_of_line(jx, jstr, jstrix);
                    } else if (njchar == '*') {
                        njchar = jtok_get_char(jx, jstr, jstrix); /* '*' */
                        jtok_advance_to_end_of_comment(jx, jstr, jstrix);
                    } else {
                        done = 1;
                    }
                } else {
                    done = 1;
                }
            }
        }
    }

    return (jchar);
}
/***************************************************************/
static int jtok_get_punctuator(struct jrunexec * jx,
        const struct js_punctuators * pul,
        struct jtoken * jtok,
        const char * jstr,
        int * jstrix)
{
/*
**  jtok_append_token() already done for first char
*/
/* ">"    */
/* ">="   */
/* ">>"   */
/* ">>="  */
/* ">>>"  */
/* "."    */
/* "..."  */

    int jstat = 0;
    char jch;
    int puix;
    int plen;
    int ix;
    int jsix;
    int done;
    char pun[MAX_PUNCTUATOR_LENGTH + 2];

    //              jchar = JTOK_NEXT_CHAR(jstr, jstrix);
    jtok->jtok_typ = JSW_PU;
    jtok_ensure_token_space(jtok, MAX_PUNCTUATOR_LENGTH + 1);

    puix = jrun_find_punctuator_index_exact(jtok->jtok_text);
    if (puix < 0) {
        jtok->jtok_kw = JSPU_NONE;  /* e.g. '@' */
    } else {
        jtok->jtok_kw = pul[puix].jpu_pu;
        pun[0] = jtok->jtok_text[0];
        jch = JTOK_CURRENT_CHAR(jstr, jstrix);
        ix = puix + 1;
        while (pul[ix].jpu_name[0] == pun[0] && pul[ix].jpu_name[1] != jch) ix++;
        if (pul[ix].jpu_name[0] != pun[0]) {
            /* Only first characters match */
            /* jtok->jtok_kw = pul[puix].jpu_pu; */
        } else {
            /* First 2 characters match */
            done = 0;
            jsix = (*jstrix) + 1;
            plen = 2;
            pun[1] = jch;
            do {
                if (pul[ix].jpu_name[plen] == '\0') {
                    while (jtok->jtok_tlen < plen) {
                        jtok->jtok_text[jtok->jtok_tlen] = pun[jtok->jtok_tlen];
                        jtok->jtok_tlen++;
                    }
                    jtok->jtok_kw = pul[ix].jpu_pu;
                }
                jch = JTOK_CURRENT_CHAR_IX(jstr, jsix);
                while (!memcmp(pul[ix].jpu_name, pun, plen) && 
                       pul[ix].jpu_name[plen] != jch) ix++;
                if (memcmp(pul[ix].jpu_name, pun, plen)) {
                    /* First plen characters match */
                    done = 1;
                } else {
                    /* First 2 or more (plen) characters match */
                    pun[plen++] = jch;
                    jsix++;
                }
            } while (!done);
            (*jstrix) = jsix; /* 10/13/2020 */
        }
    }

    return (jstat);
}
/***************************************************************/
int jtok_get_backslashed_token(struct jrunexec * jx,
        struct jtoken * jtok,
        const char * jstr,
        int * jstrix)
{
/*
**
*/
    int jstat = 0;
    HTMLC jchar;
    char jch;
    int done;
    int ii;
    char hx;

    jch = '\\';
    done = 0;
    while (!done) {
        if (JTOK_CURRENT_CHAR(jstr, jstrix) != 'u') {
            jstat = jrun_set_error(jx, JSERR_ID_INVALID_ESCAPE,
                    "Expecting \\u in identifer escape sequence");
        } else {
            jchar = 0;
            (*jstrix)++;
            for (ii = 0; !jstat && ii < 4; ii++) {
                hx = JTOK_CURRENT_CHAR(jstr, jstrix);
                if (hx >= '0' && hx <= '9')      jchar = (jchar << 4) | (hx - '0');
                else if (hx >= 'A' && hx <= 'F') jchar = (jchar << 4) | ((hx - 'A') + 10);
                else if (hx >= 'a' && hx <= 'f') jchar = (jchar << 4) | ((hx - 'a') + 10);
                else {
                    jstat = jrun_set_error(jx, JSERR_ID_INVALID_ESCAPE,
                            "Invalid identifer escape sequence");
                }
                (*jstrix)++;
            }
            jtok_append_token(jtok, jchar);
        }

        if (!jstat) {
            do {
                jchar = jtok_get_char(jx, jstr, jstrix);
                if (jchar != '\\') {
                    if (jchar >= 128 || JTOK_ID_CHAR(jchar)) {
                        jtok_append_token(jtok, jchar);
                    } else {
                        done = 1;
                    }
                }
            } while (!done && jchar != '\\');
        }
    }
    jtok_append_token(jtok, 0);

    if (jstat) {
        jtok->jtok_typ = JSW_ERROR;
    } else {
        if (jtok->jtok_tlen == 0 || !JTOK_FIRST_ID_CHAR(jtok->jtok_text[0])) {
            jstat = jrun_set_error(jx, JSERR_INVALID_ID,
                    "Invalid identifer");
        } else if (jrun_find_keyword(jtok->jtok_text)) {
            jstat = jrun_set_error(jx, JSERR_ID_RESERVED,
                    "Identifer reserved");
        } else {
            jtok->jtok_typ = JSW_ID;
        }
    }

    return (jstat);
}
/***************************************************************/
int jtok_get_token(struct jrunexec * jx,
        struct jtoken * jtok,
        const char * jstr,
        int * jstrix)
{
/*
**
*/
    int jstat = 0;
    HTMLC jchar;
    int all_digits;
    int done;
    int prev_jstrix;

    all_digits = 1;
    jtok->jtok_tlen  = 0;
    jtok->jtok_flags = 0;
    jtok->jtok_typ   = 0;
    jtok->jtok_kw    = 0;

    jchar = jtok_skip_spaces(jx, jstr, jstrix, &(jtok->jtok_flags));

    if (!jchar) {
        jstat = JERR_EOF;
    } else if (JTOK_QUOTE_CHAR(jchar)) {
        jstat = jtok_get_token_quote(jx, jchar, jtok, jstr, jstrix);
        jtok->jtok_typ = JSW_STRING;
    } else if (jchar == '\\') {
        jstat = jtok_get_backslashed_token(jx, jtok, jstr, jstrix);
    } else if (jchar == '.' && isdigit(JTOK_CURRENT_CHAR(jstr, jstrix))) {
        jtok->jtok_typ = JSW_NUMBER;
        do {
            jtok_append_token(jtok, jchar);
            jchar = jtok_get_char(jx, jstr, jstrix);
        } while (JTOK_ID_CHAR(jchar));
    } else {
        done = 0;
        while (!done) {
            jtok_append_token(jtok, jchar);
            if (!JTOK_ID_CHAR(jchar)) {
                jstat = jtok_get_punctuator(jx, js_punctuator_list,
                    jtok, jstr, jstrix);
                done = 1;
            } else {
                if (all_digits) all_digits = isdigit(jchar);
                if (all_digits) jtok->jtok_typ = JSW_NUMBER;
                else if (!jtok->jtok_typ) jtok->jtok_typ = JSW_ID;
                prev_jstrix = (*jstrix);
                jchar = jtok_get_char(jx, jstr, jstrix);
                if (!JTOK_ID_CHAR(jchar)) {
                    if (jchar == '.' && all_digits) {
                        all_digits = 0;
                        jtok_append_token(jtok, jchar); /* "." */
                        prev_jstrix = (*jstrix);
                        jchar = jtok_get_char(jx, jstr, jstrix);
                        if (!JTOK_ID_CHAR(jchar)) {
                            (*jstrix) = prev_jstrix;
                            done = 1;
                        }
                    } else {
                        if (jchar == '\\') {
                            jstat = jtok_get_backslashed_token(jx, jtok, jstr, jstrix);
                        } else {
                            (*jstrix) = prev_jstrix;
                        }
                        done = 1;
                    }
                }
            }
        }
        jtok_append_token(jtok, 0);

        if (jtok->jtok_typ == JSW_ID) {
            struct js_reserved_words * jsrw;
            jsrw = jrun_find_keyword(jtok->jtok_text);
            if (jsrw) {
                jtok->jtok_kw       = jsrw->jrw_rw;
                jtok->jtok_typ      = jsrw->jrw_type;
                jtok->jtok_flags    = jsrw->jrw_kwinfo;
            }
        }
    }

    return (jstat);
}
/***************************************************************/
struct jtokenlist * jtok_new_jtokenlist(void)
{
/*
**
*/
    struct jtokenlist * jtl;

    jtl = New(struct jtokenlist, 1);
    jtl->jtl_ntoks        = 0;
    jtl->jtl_xtoks        = 0;
    jtl->jtl_atoks        = NULL;

    return (jtl);
}
/***************************************************************/
void jtok_free_jtokenlist(struct jtokenlist * jtl)
{
/*
**
*/
    int ii;

    if (!jtl) return;

    for (ii = jtl->jtl_ntoks - 1; ii >= 0; ii--)
        jtok_free_jtoken(jtl->jtl_atoks[ii]);
    Free(jtl->jtl_atoks);

    Free(jtl);
}
/***************************************************************/
void jtok_add_to_jtokenlist(struct jtokenlist * jtl, struct jtoken * jtok)
{
/*
**
*/
    if (jtl->jtl_ntoks == jtl->jtl_xtoks) {
        if (!jtl->jtl_xtoks) jtl->jtl_xtoks = 4;
        else jtl->jtl_xtoks *= 2;
        jtl->jtl_atoks = Realloc(jtl->jtl_atoks, struct jtoken * , jtl->jtl_xtoks);
    }
    jtl->jtl_atoks[jtl->jtl_ntoks] = jtok;
    jtl->jtl_ntoks += 1;
}
/***************************************************************/
void jtok_print_jtoken(struct jtoken * jtok)
{
/*
**
*/
    char ttyp[16];
    char * kwnam;

    switch (jtok->jtok_typ) {
        case JSW_UNKNOWN: strcpy(ttyp, "Unk");          break;
        case JSW_KW     : strcpy(ttyp, "KW");           break;
        case JSW_PU     : strcpy(ttyp, "Pun");          break;
        case JSW_NUMBER : strcpy(ttyp, "Num");          break;
        case JSW_ID     : strcpy(ttyp, "Id");           break;
        case JSW_STRING : strcpy(ttyp, "Str");          break;
        default         : sprintf(ttyp, "?%d?", jtok->jtok_typ);
            break;
    }

    kwnam = NULL;
    if (jtok->jtok_typ == JSW_KW) {
        kwnam = jrun_find_keyword_name(jtok->jtok_kw);
        if (kwnam) {
            if (!strcmp(jtok->jtok_text, kwnam)) {
                printf("{%s,%s,ok} ", jtok->jtok_text, ttyp);
            } else {
                printf("{%s,%s,ERR`%s`} ", jtok->jtok_text, ttyp, kwnam);
            }
        } else {
            printf("{%s,%s,ERROR} ", jtok->jtok_text, ttyp);
        }
    } else if (jtok->jtok_typ == JSW_PU) {
        kwnam = jrun_find_punctuator_name(jtok->jtok_kw);
        if (kwnam) {
            if (!strcmp(jtok->jtok_text, kwnam)) {
                printf("{%s,%s,ok} ", jtok->jtok_text, ttyp);
            } else {
                printf("{%s,%s,ERR`%s`} ", jtok->jtok_text, ttyp, kwnam);
            }
        } else {
            if (jtok->jtok_kw == JSPU_NONE) {
                printf("{%s,%s} ", jtok->jtok_text, ttyp);
            } else {
                printf("{%s,%s,ERROR} ", jtok->jtok_text, ttyp);
            }
        }
    } else {
        printf("{%s,%s} ", jtok->jtok_text, ttyp);
    }
}
/***************************************************************/
void jtok_print_tokenlist(struct jtokenlist * jtl)
{
/*
**
*/
    int ii;

    printf("jtokenlist n=%d ", jtl->jtl_ntoks);

    for (ii = 0; ii < jtl->jtl_ntoks; ii++) {
        jtok_print_jtoken(jtl->jtl_atoks[ii]);
    }
    printf("\n");
}
/***************************************************************/
int jtok_create_tokenlist(  struct jrunexec * jx,
                            const char * jstr,
                            struct jtokenlist ** pjtl)
{
/*
**
*/
    int jstat;
    struct jtoken * jtok;
    int jstrix;

    jstat = 0;
    jstrix = 0;
    (*pjtl) = jtok_new_jtokenlist();

    while (!jstat) {
        jtok = jtok_new_jtoken();
        jstat = jtok_get_token(jx, jtok, jstr, &jstrix);
        if (!jstat) {
            jtok_add_to_jtokenlist(*pjtl, jtok);
        }
    }

    if (jstat == JERR_EOF) {
        jstat = 0;
    }
    jtok_free_jtoken(jtok);

    /* jtok_print_tokenlist(*pjtl); */

    return (jstat);
}
/***************************************************************/
