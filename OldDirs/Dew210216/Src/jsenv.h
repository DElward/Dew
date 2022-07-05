#ifndef JSENVH_INCLUDED
#define JSENVH_INCLUDED
/***************************************************************/
/* jsenv.h                                                     */
/***************************************************************/

typedef int32 JSINT;
typedef int32 JSBOOL;
typedef double JSFLOAT;
/***************************************************************/

#define JENFLAG_NODEJS          1
#define DEFAULT_ARGLIST_SIZE    4
#define FUNC_NAME_MAX           80
/***************************************************************/

struct jscontext { /* jcx_ */
    struct dbtree     * jcx_dbt;    /* tree of struct jsvar */
};

#if 0
struct jxcursor {   /* jxc */
    int     jxc_str_ix;
    int     jxc_char_ix;
    int     jxc_adv;
};
#endif
struct jxcursor {   /* jxc */
    struct jtokenlist *     jxc_jtl;
    int                     jxc_toke_ix;
};

struct jerror { /* jerr_ */
    int                     jerr_stat;
    char *                  jerr_msg;
};
#define JERROR_OFFSET       4000
#define MAXIMUM_JERRORS     100

struct jrunexec {   /* jx_ */
    struct jxcursor         jx_jxc;
    int                     jx_njs;
    int                     jx_xjs;
    struct jxstate   **     jx_js;
    int                     jx_njfs;
    int                     jx_xjfs;
    struct jfuncstate **    jx_jfs;
    int                     jx_nerrs;
    int                     jx_xerrs;
    struct jerror  **       jx_aerrs;
    int                     jx_nvars;
    int                     jx_xvars;
    struct jvarrec **       jx_avars;
    int                     jx_njtls;
    int                     jx_xjtls;
    struct jtokenlist **    jx_ajtls;
    int                     jx_nop_xref;
    short                 * jx_aop_xref;
};

struct jtokenlist {   /* jtl_ */
    int                     jtl_ntoks;
    int                     jtl_xtoks;
    struct jtoken **        jtl_atoks;
};

struct jsenv { /* jse_ */
	int                 jse_flags;
    struct jrunexec *   jse_jx;
};

#define JERR_EOF                                (-1)
#define JERR_EOF_ERROR                          (-2)
#define JERR_STOPPED                            (-3)
#define JERR_EXIT                               (-4)
#define JERR_RETURN                             (-5)

#define JSERR_UNMATCHED_QUOTE                   (-9801)
#define JSERR_INVALID_ESCAPE                    (-9802)
#define JSERR_INVALID_ID                        (-9803)
#define JSERR_ID_RESERVED                       (-9804)
#define JSERR_ID_INVALID_ESCAPE                 (-9805)
#define JSERR_UNSUPPORTED_STMT                  (-9806)
#define JSERR_OPERATOR_TYPE_MISMATCH            (-9807)
#define JSERR_INVALID_TOKEN_TYPE                (-9808)
#define JSERR_INVALID_NUMBER                    (-9809)
#define JSERR_UNEXPECTED_SPECIAL                (-9810)
#define JSERR_EXP_CLOSE_PAREN                   (-9811)
#define JSERR_UNEXPECTED_KEYWORD                (-9812)
#define JSERR_UNDEFINED_VARIABLE                (-9813)
#define JSERR_EXP_INTERNAL_CLASS                (-9814)
#define JSERR_DUP_INTERNAL_METHOD               (-9815)
#define JSERR_DUP_INTERNAL_CLASS                (-9816)
#define JSERR_DUPLICATE_VARIABLE                (-9817)
#define JSERR_EXP_ID_AFTER_DOT                  (-9818)
#define JSERR_EXP_OBJECT_AFTER_DOT              (-9819)
#define JSERR_DUP_CLASS_MEMBER                  (-9820)
#define JSERR_UNDEFINED_CLASS_MEMBER            (-9821)
#define JSERR_EXP_METHOD                        (-9822)
#define JSERR_INTERNAL_ERROR                    (-9823)
#define JSERR_ILLEGAL_EXPONENTIATION            (-9824)
#define JSERR_UNSUPPORTED_OPERATION             (-9825)
#define JSERR_INVALID_VARNAME                   (-9826)

#define JSERR_RUNTIME_STACK_OVERFLOW            (-7801)
#define JSERR_RUNTIME_STACK_UNDERFLOW           (-7802)

/* JavaScript errors from: */
/* https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Error#Error_types */
#define JSERROBJ_SCRIPT_SYNTAX_ERROR            (-9901)    /* SyntaxError */
#define JSERROBJ_SCRIPT_REFERENCE_ERROR         (-9902)    /* ReferenceError */

#define JMAX_XSTAT_STACK     512     /* See also MAX_INCLUDE_STACK */
#define JMAX_FUNCTION_STACK  256

typedef int (*JVAR_INTERNAL_FUNCTION)
        (struct jrunexec  * jx,
         const char       * func_name,
         struct jvarvalue * jvvlargs,
         struct jvarvalue * jvvrtn);

struct jruncursor {   /* jxc */
    int     jxc_str_ix;
    int     jxc_char_ix;
    int     jxc_adv;
};

enum e_jvv_type {
    JVV_DTYPE_NONE = 0          ,
    JVV_DTYPE_BOOL              ,
    JVV_DTYPE_JSINT             ,
    JVV_DTYPE_JSFLOAT           ,
    JVV_DTYPE_CHARS             ,
    JVV_DTYPE_NUMBER            ,
    JVV_DTYPE_NULL              ,
    JVV_DTYPE_LVAL              ,
    JVV_DTYPE_INTERNAL_CLASS    ,
    JVV_DTYPE_INTERNAL_METHOD   ,
    JVV_DTYPE_OBJECT            ,
    JVV_DTYPE_VALLIST           ,
    JVV_DTYPE_TOKEN             ,
    JVV_DTYPE_FUNCTION          ,   /* 02/08/2021 */
    JVV_ZZZZ     
};

#define JVARVALUE_CHARS_MIN 16
struct jvarvalue_chars {   /* jvvc_ */
    char *  jvvc_val_chars;
    int     jvvc_length;
    int     jvvc_size;
};

struct jvarvalue_token {   /* jvvt_ */
    char *  jvvt_token;
};

struct jvarvalue_number {   /* jvvn_ */
    char *  jvvn_val_chars;
    int     jvvn_length;
    int     jvvn_size;
};
struct jvarvalue_internal_class {   /* jvvi_ */
    int                 jvvi_nrefs;
    struct jvarrec *    jvvi_jvar;
    char             *  jvvi_class_name;
};
struct jvarvalue_object {   /* jvvo_ */
    struct jvarvalue *  jvvo_class;
    struct jvarrec  *   jvvo_vars;
};

struct jvarvalue_vallist {   /* jvvl_ */
    int                     jvvl_nvals;
    int                     jvvl_xvals;
    struct jvarvalue *      jvvl_avals;
};

struct jvarvalue_int_method {   /* jvvim_ */
    JVAR_INTERNAL_FUNCTION  jvvim_method;
    char *                  jvvim_method_name;
};

struct jvarvalue_funcarg {   /* jvvfa_ */
    char                          * jvvfa_arg_name;
};

#define JVVF_FLAG_GENERATOR     1   /* Function is function* */
#define JVVF_FLAG_REST_PARAM    2   /* Last parameter is ... */

struct jvarvalue_function {   /* jvvf_ */
    char                          * jvvf_func_name;
    int                             jvvf_flags;
    int                             jvvf_nargs;
    int                             jvvf_xargs;
    struct jvarvalue_funcarg    **  jvvf_aargs;
    struct jxcursor                 jvvf_begin_jxc;
};

struct jvarvalue {   /* jvv_ */
    enum e_jvv_type    jvv_dtype;
    union {
        int                                 jvv_val_bool;      /* JVV_DTYPE_BOOL            */
        JSINT                               jvv_val_jsint;     /* JVV_DTYPE_JSINT           */
        JSFLOAT                             jvv_val_jsfloat;   /* JVV_DTYPE_JSFLOAT         */
        void                              * jvv_void;          /* void*                     */
        struct jvarvalue                  * jvv_lval;          /* JVV_DTYPE_LVAL            */
        struct jvarvalue_chars              jvv_val_chars;     /* JVV_DTYPE_CHARS           */
        struct jvarvalue_number             jvv_val_number;    /* JVV_DTYPE_NUMBER          */
        struct jvarvalue_internal_class   * jvv_jvvi;          /* JVV_DTYPE_INTERNAL_CLASS  */
        struct jvarvalue_object             jvv_jvvo;          /* JVV_DTYPE_OBJECT          */
        struct jvarvalue_int_method         jvv_int_method;    /* JVV_DTYPE_INTERNAL_METHOD */
        struct jvarvalue_vallist          * jvv_jvvl;          /* JVV_DTYPE_VALLIST         */
        struct jvarvalue_token              jvv_val_token;     /* JVV_DTYPE_TOKEN           */
        struct jvarvalue_function         * jvv_val_function;  /* JVV_DTYPE_FUNCTION        */
    } jvv_val;
};

struct jxstate {   /* jxs_ */
    enum e_xstatus     jxs_xstate;
    struct jxcursor    jxs_jxc;
};

#define JFSFLAG_STRICT_MODE         1
#define JFSFLAG_STRICTER_MODE       2

struct jfuncstate {   /* jfs_ */
    int jfs_flags;
    //struct jxcursor    jxs_jxc;
};

/***************************************************************/
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
#define XSTAT_IS_ACTIVE(jx)   (((jx)->jx_js[(jx)->jx_njs - 1]->jxs_xstate & XSTATMASK_INACTIVE) == 0)
#define XSTAT_IS_COMPLETE(jx) (((jx)->jx_js[(jx)->jx_njs - 1]->jxs_xstate & XSTATMASK_IS_COMPLETE) != 0)

#define JS_TOKIX(jx)  ((jx)->jx_jxc.jxc_toke_ix)
#define JS_TOKENLIST(jx) ((jx)->jx_jxc.jxc_jtl)
#define JS_TOKEN(jx) (JS_TOKENLIST(jx)->jtl_atoks[JS_TOKIX(jx)])

#define GET_RVAL(j) (((j)->jvv_dtype == JVV_DTYPE_LVAL) ? (j)->jvv_val.jvv_lval : (j))

/***************************************************************/
struct jsenv * new_jsenv(int jenflags);
void free_jsenv(struct jsenv * jenv);
int jenv_initialize(struct jsenv * jse);
int jenv_xeq_string(struct jsenv * jse, const char * jsstr);
int jrun_set_error(struct jrunexec * jx, int jstat, char *fmt, ...);
char * jenv_get_errmsg(struct jsenv * jse,
    int emflags,
    int jstat,
    int *jerrnum);
int jrun_next_token(struct jrunexec * jx, struct jtoken ** pjtok);
void jrun_peek_token(struct jrunexec * jx, struct jtoken * jtok);
int jrun_push_jfuncstate(struct jrunexec * jx);
int jrun_pop_jfuncstate(struct jrunexec * jx);
int jrun_get_strict_mode(struct jrunexec * jx);
int jrun_get_stricter_mode(struct jrunexec * jx);
void jrun_init_jxcursor(struct jxcursor * tgtjxc);

/***************************************************************/
#endif /* JSENVH_INCLUDED */
