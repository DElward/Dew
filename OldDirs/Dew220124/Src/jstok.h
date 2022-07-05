#ifndef JSTOKH_INCLUDED
#define JSTOKH_INCLUDED
/***************************************************************/
/* jstok.h --  JavaScript tokens                               */
/***************************************************************/

#define JSW_UNKNOWN     0
#define JSW_KW          1
#define JSW_PU          2
#define JSW_NUMBER      3
#define JSW_ID          4
#define JSW_STRING      5
#define JSW_ERROR       6
#define JSW_EOF         7

#define JSKWI_RW        1
#define JSKWI_KEYWORD   2
#define JSKWI_VAR_OK    4   /* not reserved for a var name */
#define JSKWI_NO_UPDATE 8

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
#if 0
    JSKW_DEBUG               ,
#endif
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
    JSKW_GET                 ,
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
    JSKW_SET                 ,
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
    JSKW_YIELD               };

#define MAX_PUNCTUATOR_LENGTH  4

/* All 2+ char tokens must have the single char token  */
/* e.g. "<<=" must also have "<"                       */
/*       Listing just "@@" without "@" would not work. */
enum e_js_punctuator {
    JSPU_NONE = JSKW_YIELD + 1,
    JSPU_BANG                 ,    /* "!"    */
    JSPU_BANG_EQ              ,    /* "!="   */
    JSPU_BANG_EQ_EQ           ,    /* "!=="  */
    JSPU_PECENT               ,    /* "%"    */
    JSPU_PERCENT_EQ           ,    /* "%="   */
    JSPU_AMP                  ,    /* "&"    */
    JSPU_AMP_AMP              ,    /* "&&"   */
    JSPU_AMP_EQ               ,    /* "&="   */
    JSPU_OPEN_PAREN           ,    /* "("    */
    JSPU_CLOSE_PAREN          ,    /* ")"    */
    JSPU_STAR                 ,    /* "*"    */
    JSPU_STAR_STAR            ,    /* "**"   */
    JSPU_STAR_STAR_EQ         ,    /* "**="  */
    JSPU_STAR_EQ              ,    /* "*="   */
    JSPU_PLUS                 ,    /* "+"    */
    JSPU_PLUS_PLUS            ,    /* "++"   */
    JSPU_PLUS_EQ              ,    /* "+="   */
    JSPU_COMMA                ,    /* ","    */
    JSPU_MINUS                ,    /* "-"    */
    JSPU_MINUS_MINUS          ,    /* "--"   */
    JSPU_MINUS_EQ             ,    /* "-="   */
    JSPU_DOT                  ,    /* "."    */
    JSPU_DOT_DOT_DOT          ,    /* "..."  */
    JSPU_SLASH                ,    /* "/"    */
    JSPU_SLASH_EQ             ,    /* "/="   */
    JSPU_COLON                ,    /* ":"    */
    JSPU_SEMICOLON            ,    /* ";"    */
    JSPU_LT                   ,    /* "<"    */
    JSPU_LT_LT                ,    /* "<<"   */
    JSPU_LT_LT_EQ             ,    /* "<<="  */
    JSPU_LT_EQ                ,    /* "<="   */
    JSPU_EQ                   ,    /* "="    */
    JSPU_EQ_EQ                ,    /* "=="   */
    JSPU_EQ_EQ_EQ             ,    /* "==="  */
    JSPU_EQ_GT                ,    /* "=>"   */
    JSPU_GT                   ,    /* ">"    */
    JSPU_GT_EQ                ,    /* ">="   */
    JSPU_GT_GT                ,    /* ">>"   */
    JSPU_GT_GT_EQ             ,    /* ">>="  */
    JSPU_GT_GT_GT             ,    /* ">>>"  */
    JSPU_GT_GT_GT_EQ          ,    /* ">>>=" */
    JSPU_QUESTION             ,    /* "?"    */
    JSPU_QUESTION_DOT         ,    /* "?."   */
    JSPU_QUESTION_QUESTION    ,    /* "??"   */
#if IS_DEBUG
    JSPU_AT                   ,    /* "@"    */
    JSPU_AT_AT                ,    /* "@@"   */
#endif
    JSPU_OPEN_BRACKET         ,    /* "["    */
    JSPU_CLOSE_BRACKET        ,    /* "]"    */
    JSPU_HAT                  ,    /* "^"    */
    JSPU_HAT_EQ               ,    /* "^="   */
    JSPU_OPEN_BRACE           ,    /* "{"    */
    JSPU_BAR                  ,    /* "|"    */
    JSPU_BAR_EQ               ,    /* "|="   */
    JSPU_BAR_BAR              ,    /* "||"   */
    JSPU_CLOSE_BRACE          ,    /* "}"    */
    JSPU_TILDE                ,    /* "~"    */
    JSPU_ZZ_LAST              };

/***************************************************************/

#define JTOK_FLAG_PRE_LINETERM      1   /* 1 or more line terms before tokem */

struct jtokeninfo { /* jti_ */
    struct jvarrec * jti_jvar;
};

struct jtoken {   /* jtok */
    char *  jtok_text;  /* UTF-8 */
    int     jtok_tlen;
    int     jtok_tmax;
    struct jtokeninfo * jtok_jti;
    short   jtok_flags;
    short   jtok_typ;
    short   jtok_kw;
    short   jtok__unused__;
};

#define JTOK_FIRST_ID_CHAR(c) (isalpha(c) || (c) == '_' || (c) == '$')
#define JTOK_ID_CHAR(c) (isalnum(c) || (c) == '_' || (c) == '$')
#define JTOK_ISSPACE_CHAR(c) ((c) == ' ' || (c) == '\t' || (c) == '\v' || (c) == '\f' || (c) == '\xA0')
/* \xA0 = U+00A0 = NO-BREAK SPACE = <NBSP> */
#define JTOK_LINETERM_CHAR(c) ((c) == '\n' || (c) == '\r')
#define JTOK_QUOTE_CHAR(c) ((c) == '\"' || (c) == '\'')

#define JTOK_VAR_CHAR0(c) (isalpha(c) || (c) == '_')
#define JTOK_VAR_CHAR(c) (isalnum(c) || (c) == '_')
#define JTOK_CURRENT_CHAR(s,x) ((s)[*x])
#define JTOK_CURRENT_CHAR_IX(s,x) ((s)[x])
#define JTOK_NEXT_CHAR(s,x) ((s)[(*x)+1])

#define JTOK_STMT_TERM_CHAR(c) ((c) == ';' || (c) == '}')
#define JTOK_BLOCK_BEGIN_CHAR(c) ((c) == '{')
#define JTOK_BLOCK_END_CHAR(c) ((c) == '}')
#define JTOK_STMT_END_CHAR(c) ((c) == ';')

/***************************************************************/
struct jtoken * jtok_new_jtoken(void);
void jtok_free_jtoken(struct jtoken * jtok);
int jtok_create_tokenlist(struct jrunexec * jx,
    const char * jsstr,
    struct jtokenlist ** pjtl,
    int tflags);
void jtok_free_jtokenlist(struct jtokenlist * jtl);
void jrun_find_punctuator_name_by_pu(short pu, char * punam);
struct jtokeninfo * jtok_new_jtokeninfo(void);
const char * jrun_next_token_string(struct jrunexec * jx);

/***************************************************************/
#endif /* JSTOKH_INCLUDED */
