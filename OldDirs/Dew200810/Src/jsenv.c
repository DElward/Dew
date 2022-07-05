/***************************************************************/
/*
**  jsenv.c --  JavaScript Environment/main
*/
/***************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "dew.h"
#include "jsenv.h"
#include "dbtreeh.h"

/***************************************************************/
#define JSW_KW          1

enum e_js_reserved_words {
    JSKW_NONE = 0            ,
    JSRW_ABSTRACT            ,
    JSKW_ARGUMENTS           ,
    JSKW_AWAIT               ,
    JSKW_BOOLEAN             ,
    JSKW_BREAK               ,
    JSKW_BYTE                ,
    JSKW_CASE                ,
    JSKW_CATCH               ,
    JSKW_CHAR                ,
    JSKW_CLASS               ,
    JSKW_CONST               ,
    JSKW_CONTINUE            ,
    JSKW_DEBUGGER            ,
    JSKW_DEFAULT             ,
    JSKW_DELETE              ,
    JSKW_DO                  ,
    JSKW_DOUBLE              ,
    JSKW_ELSE                ,
    JSKW_ENUM                ,
    JSKW_EVAL                ,
    JSKW_EXPORT              ,
    JSKW_EXTENDS             ,
    JSKW_FALSE               ,
    JSKW_FINAL               ,
    JSKW_FINALLY             ,
    JSKW_FLOAT               ,
    JSKW_FOR                 ,
    JSKW_FUNCTION            ,
    JSKW_GOTO                ,
    JSKW_IF                  ,
    JSKW_IMPLEMENTS          ,
    JSKW_IMPORT              ,
    JSKW_IN                  ,
    JSKW_INSTANCEOF          ,
    JSKW_INT                 ,
    JSKW_INTERFACE           ,
    JSKW_LET                 ,
    JSKW_LONG                ,
    JSKW_NATIVE              ,
    JSKW_NEW                 ,
    JSKW_NULL                ,
    JSKW_PACKAGE             ,
    JSKW_PRIVATE             ,
    JSKW_PROTECTED           ,
    JSKW_PUBLIC              ,
    JSKW_RETURN              ,
    JSKW_SHORT               ,
    JSKW_STATIC              ,
    JSKW_SUPER               ,
    JSKW_SWITCH              ,
    JSKW_SYNCHRONIZED        ,
    JSKW_THIS                ,
    JSKW_THROW               ,
    JSKW_THROWS              ,
    JSKW_TRANSIENT           ,
    JSKW_TRUE                ,
    JSKW_TRY                 ,
    JSKW_TYPEOF              ,
    JSKW_VAR                 ,
    JSKW_VOID                ,
    JSKW_VOLATILE            ,
    JSKW_WHILE               ,
    JSKW_WITH                ,
    JSKW_YIELD                };

struct js_reserved_words_list {
    char * jrw_name;
    int    jrw_rw;
    int    jrw_type;
} js_reserved_words[] = {
    { "abstract"        , JSRW_ABSTRACT            , JSW_KW      },
    { "arguments"       , JSKW_ARGUMENTS           , JSW_KW      },
    { "await"           , JSKW_AWAIT               , JSW_KW      },
    { "boolean"         , JSKW_BOOLEAN             , JSW_KW      },
    { "break"           , JSKW_BREAK               , JSW_KW      },
    { "byte"            , JSKW_BYTE                , JSW_KW      },
    { "case"            , JSKW_CASE                , JSW_KW      },
    { "catch"           , JSKW_CATCH               , JSW_KW      },
    { "char"            , JSKW_CHAR                , JSW_KW      },
    { "class"           , JSKW_CLASS               , JSW_KW      },
    { "const"           , JSKW_CONST               , JSW_KW      },
    { "continue"        , JSKW_CONTINUE            , JSW_KW      },
    { "debugger"        , JSKW_DEBUGGER            , JSW_KW      },
    { "default"         , JSKW_DEFAULT             , JSW_KW      },
    { "delete"          , JSKW_DELETE              , JSW_KW      },
    { "do"              , JSKW_DO                  , JSW_KW      },
    { "double"          , JSKW_DOUBLE              , JSW_KW      },
    { "else"            , JSKW_ELSE                , JSW_KW      },
    { "enum"            , JSKW_ENUM                , JSW_KW      },
    { "eval"            , JSKW_EVAL                , JSW_KW      },
    { "export"          , JSKW_EXPORT              , JSW_KW      },
    { "extends"         , JSKW_EXTENDS             , JSW_KW      },
    { "false"           , JSKW_FALSE               , JSW_KW      },
    { "final"           , JSKW_FINAL               , JSW_KW      },
    { "finally"         , JSKW_FINALLY             , JSW_KW      },
    { "float"           , JSKW_FLOAT               , JSW_KW      },
    { "for"             , JSKW_FOR                 , JSW_KW      },
    { "function"        , JSKW_FUNCTION            , JSW_KW      },
    { "goto"            , JSKW_GOTO                , JSW_KW      },
    { "if"              , JSKW_IF                  , JSW_KW      },
    { "implements"      , JSKW_IMPLEMENTS          , JSW_KW      },
    { "import"          , JSKW_IMPORT              , JSW_KW      },
    { "in"              , JSKW_IN                  , JSW_KW      },
    { "instanceof"      , JSKW_INSTANCEOF          , JSW_KW      },
    { "int"             , JSKW_INT                 , JSW_KW      },
    { "interface"       , JSKW_INTERFACE           , JSW_KW      },
    { "let"             , JSKW_LET                 , JSW_KW      },
    { "long"            , JSKW_LONG                , JSW_KW      },
    { "native"          , JSKW_NATIVE              , JSW_KW      },
    { "new"             , JSKW_NEW                 , JSW_KW      },
    { "null"            , JSKW_NULL                , JSW_KW      },
    { "package"         , JSKW_PACKAGE             , JSW_KW      },
    { "private"         , JSKW_PRIVATE             , JSW_KW      },
    { "protected"       , JSKW_PROTECTED           , JSW_KW      },
    { "public"          , JSKW_PUBLIC              , JSW_KW      },
    { "return"          , JSKW_RETURN              , JSW_KW      },
    { "short"           , JSKW_SHORT               , JSW_KW      },
    { "static"          , JSKW_STATIC              , JSW_KW      },
    { "super"           , JSKW_SUPER               , JSW_KW      },
    { "switch"          , JSKW_SWITCH              , JSW_KW      },
    { "synchronized"    , JSKW_SYNCHRONIZED        , JSW_KW      },
    { "this"            , JSKW_THIS                , JSW_KW      },
    { "throw"           , JSKW_THROW               , JSW_KW      },
    { "throws"          , JSKW_THROWS              , JSW_KW      },
    { "transient"       , JSKW_TRANSIENT           , JSW_KW      },
    { "true"            , JSKW_TRUE                , JSW_KW      },
    { "try"             , JSKW_TRY                 , JSW_KW      },
    { "typeof"          , JSKW_TYPEOF              , JSW_KW      },
    { "var"             , JSKW_VAR                 , JSW_KW      },
    { "void"            , JSKW_VOID                , JSW_KW      },
    { "volatile"        , JSKW_VOLATILE            , JSW_KW      },
    { "while"           , JSKW_WHILE               , JSW_KW      },
    { "with"            , JSKW_WITH                , JSW_KW      },
    { "yield"           , JSKW_YIELD               , JSW_KW      } };

/***************************************************************/
struct jsenv * new_jsenv(int jenflags)
{
    struct jsenv * jenv;

    jenv = New(struct jsenv, 1);
    jenv->jen_flags = jenflags;

    return (jenv);
}
/***************************************************************/
void free_jsenv(struct jsenv * jenv)
{
    Free(jenv);
}
/***************************************************************/
void js_add_class(struct jsenv * jenv,
        struct jscontext * jcx,
        const char * class_name,
        int (*class_initializer)(struct jsenv * jenv, struct jscontext * jcx) )
{
}
/***************************************************************/
int jsclass_init_console(struct jsenv * jenv, struct jscontext * jcx)
{
    int jstat;

    jstat = 0;

    return (jstat);
}
/***************************************************************/
void jen_init_internal_classes(struct jsenv * jenv, struct jscontext * jcx)
{
    js_add_class(jenv, jcx, "console", jsclass_init_console);
}
enum e_xstatus {
      XSTAT_NONE                    = 0x0000
    , XSTAT_TRUE_IF                 = 0x0401
    , XSTAT_TRUE_ELSE               = 0x0402
    , XSTAT_TRUE_WHILE              = 0x0403
    , XSTAT_ACTIVE_BLOCK            = 0x0020
    , XSTAT_INCLUDE_FILE            = 0x0021
    , XSTAT_FUNCTION                = 0x0022
    , XSTAT_BEGIN                   = 0x0023
    , XSTAT_BEGIN_THREAD            = 0x0024

    , XSTAT_FALSE_IF                = 0x0511
    , XSTAT_FALSE_ELSE              = 0x0512
    , XSTAT_INACTIVE_IF             = 0x0113
    , XSTAT_INACTIVE_ELSE           = 0x0114
    , XSTAT_INACTIVE_WHILE          = 0x0115
    , XSTAT_INACTIVE_BLOCK          = 0x0121
    , XSTAT_DEFINE_FUNCTION         = 0x0122
    , XSTAT_RETURN                  = 0x0123

    , XSTAT_TRUE_IF_COMPLETE        = 0x0224
    , XSTAT_FALSE_IF_COMPLETE       = 0x0225
    , XSTAT_INACTIVE_IF_COMPLETE    = 0x0228

    , XSTATMASK_INACTIVE            = 0x0100
    , XSTATMASK_IS_COMPLETE         = 0x0200
    , XSTATMASK_IS_COMPOUND         = 0x0400    /* if, while, etc. */
    , XSTATMASK_IS_IF               = 0x0800    /* if */
};

#define JRUNERR_EOF                             (-1)
#define JRUNERR_EOF_ERROR                       (-2)
#define JRUNERR_STOPPED                         (-3)
#define JRUNERR_EXIT                            (-4)
#define JRUNERR_RETURN                          (-5)

struct jtoken {   /* jtok */
    char *  jtok_text;
    int     jtok_tlen;
    int     jtok_tmax;
};

struct jruncursor {   /* jxc */
    int     jxc_str_ix;
    int     jxc_char_ix;
    int     jxc_adv;
};

struct jrunexec {   /* jx_ */
    int                     jx_nsvs;
    struct jrunstate **     jx_svs;
    int                     jx_xsvs;
    struct jruninput **     jx_asvi;
    int                     jx_nsvi;
    int                     jx_xsvi;
    struct jruninput *      jx_svi;
    struct jruncursor       jx_token_pc;
    void *                  jx_gdata;
    int                     jx_svstat;
    char *                  jx_errmsg;
    int                     jx_nvars;
    int                     jx_mvars;
    struct jrunrec **       jx_avars;      /* MT RO jx_avars[0] and jx_avars[1] */

    /* jrunglob handling */
    struct jrunglob *       jx_locked_svg;
    void            *       jx_vsvg;
    int                     jx_lock_count;
}; /* MT RO = Read-only in multi-threaded environment -- jx_nrefs > 1 */
/***************************************************************/
static struct jtoken * jrun_new_jtoken(void)
{
    struct jtoken * jtok;

    jtok = New(struct jtoken, 1);
    jtok->jtok_tlen = 0;
    jtok->jtok_tmax = 0;
    jtok->jtok_text = NULL;

    return (jtok);
}
/***************************************************************/
static void jrun_free_jtoken(struct jtoken * jtok)
{
    Free(jtok->jtok_text);
    Free(jtok);
}
/***************************************************************/
static int jrun_exec_jrunxec(struct jrunxec * jx)
{
    int jstat = 0;
    struct jtoken * jtok;

    jtok = jrun_new_jtoken();

    //jstat = jrun_push_xstat(svx, XSTAT_BEGIN);
    //if (!jstat) jstat = jrun_get_token(svx, jtok);
    //if (!jstat) jstat = jrun_exec_block(svx, jtok);

    if (jstat == JRUNERR_EXIT || jstat == JRUNERR_EOF) jstat = 0;

    if (!jstat) {
        //jstat = jrun_pop_final_xstat(svx);
    }

    jrun_free_jtoken(jtok);

    return (jstat);
}
/***************************************************************/
