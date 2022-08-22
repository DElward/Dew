#ifndef JSENVH_INCLUDED
#define JSENVH_INCLUDED
/***************************************************************/
/* jsenv.h                                                     */
/***************************************************************/

#define USE_JVV_CHARS_POINTER       1       /* 1 passes tests */
#define USE_JVARVALUE_IMETHVAR      1       /* 1 passes tests */
#define PREP_INACTIVE_EXPRESSIONS   1       /* 1 passes tests */
#define FIX_220812                  0

typedef int32 JSINT;
typedef int32 JSBOOL;
typedef double JSFLOAT;
/***************************************************************/

#define DEBUG_FUNCTION_CALLS    0
#define DEBUG_OLD_ASSIGN        0       /* Delete this */

#define JENFLAG_NODEJS          1
#define JENFLAG_DEBUGFEATURES   4

#define MAIN_FUNCTION_NAME      "@main"

/***************************************************************/

struct jxcursor {   /* jxc */
    struct jtokenlist *     jxc_jtl;
    int                     jxc_toke_ix;
};

struct jerror { /* jerr_ */
    int                     jerr_stat;
    enum errTypes           jerr_errtyp;
    char *                  jerr_msg;
};

struct jcontext { /* jcx_ */
#if IS_DEBUG
    char jcx_sn[4]; /* 'X' */
#endif
    int                     jcx_njvv;
    int                     jcx_xjvv;
    struct jvarvalue      * jcx_ajvv;   /* Project: 10/14/2021 Made into struct jvarvalue * */
    /* int                   * jcx_flags; -- 03/01/2022 removed */
    struct jvarrec *        jcx_jvar;
    struct jvarvalue_function * jcx_jvvf;
    struct jcontext *       jcx_outer_jcx;
};

#define JCX_FLAG_METHOD     1
#define JCX_FLAG_PROPERTY   2
#define JCX_FLAG_CONST      4
#define JCX_FLAG_OPERATION  8
#define JCX_FLAG_FUNCTION   16

#define JERROR_OFFSET       4000
#define MAXIMUM_JERRORS     100

//#define JX_GLOBAL_CONTEXT_IX    0

struct jtokenlist {   /* jtl_ */
    int                     jtl_ntoks;
    int                     jtl_xtoks;
    struct jtoken **        jtl_atoks;
};

struct jsenv { /* jse_ */
	int                 jse_flags;
    struct jrunexec *   jse_jx;
};

#define JMAX_XSTAT_STACK     512     /* See also MAX_INCLUDE_STACK */
#define JMAX_FUNCTION_STACK  256

typedef int (*JVAR_INTERNAL_FUNCTION)
        (struct jrunexec  * jx,
         const char       * func_name,
         void             * this_ptr,
         struct jvarvalue * jvvlargs,
         struct jvarvalue * jvvrtn);

typedef void * (*JVAR_NEW_OBJECT)(void);
typedef void (*JVAR_FREE_OBJECT)(void * this_ptr);

struct jruncursor {   /* jxc */
    int     jxc_str_ix;
    int     jxc_char_ix;
    int     jxc_adv;
};

enum e_jvv_type {
    JVV_DTYPE_NONE = 0          ,
    JVV_DTYPE_BOOL = 65         , /* 'A' - JVV_FIRST_DTYPE */
    JVV_DTYPE_JSINT             , /* 'B' */
    JVV_DTYPE_JSFLOAT           , /* 'C' */
    JVV_DTYPE_CHARS             , /* 'D' */
    JVV_DTYPE_NUMBER            , /* 'E' */
    JVV_DTYPE_NULL              , /* 'F' */
    JVV_DTYPE_LVAL              , /* 'G' */
    JVV_DTYPE_INTERNAL_CLASS    , /* 'H' */
    JVV_DTYPE_INTERNAL_METHOD   , /* 'I' */
    JVV_DTYPE_INTERNAL_OBJECT   , /* 'J' */
    JVV_DTYPE_VALLIST           , /* 'K' */
    JVV_DTYPE_TOKEN             , /* 'L' */
    JVV_DTYPE_FUNCTION          , /* 'M' */   /* 02/08/2021 */
    JVV_DTYPE_FUNCVAR           , /* 'N' */   /* 09/03/2021 */
    JVV_DTYPE_ARRAY             , /* 'O' */   /* 09/24/2021 */
    JVV_DTYPE_IMETHVAR          , /* 'P' */   /* 10/05/2021 */
    JVV_DTYPE_OBJECT            , /* 'Q' */   /* 10/12/2021 */
    JVV_DTYPE_OBJPTR            , /* 'R' */   /* 10/19/2021 */
    JVV_DTYPE_DYNAMIC           , /* 'S' */   /* 10/25/2021 */
    JVV_ZZZZ
    /* Note: Add new types to jvar_get_dtype_name() */
    /* Also: jvar_store_jvarvalue() */
    /* Also: jvar_free_jvarvalue_data() */
};

#define JVV_FIRST_DTYPE         JVV_DTYPE_BOOL
#define JVV_LAST_DTYPE          JVV_ZZZZ
#define JVV_DTYPE_INDEX(t)      ((t) - JVV_FIRST_DTYPE)
#define JVV_NUM_DTYPES          (JVV_LAST_DTYPE - JVV_FIRST_DTYPE)

#define JVARVALUE_CHARS_MIN 16
struct jvarvalue_chars {   /* jvvc_ */
    char *  jvvc_val_chars;
    int     jvvc_length;
    int     jvvc_size;
    /* int     jvvc_flags; */
};
/* #define JVVC_FLAG_TEMPLATE      1 */

struct jvarvalue_token {   /* jvvt_ */
    char *  jvvt_token;
};

struct jvarvalue_number {   /* jvvn_ */
    char *  jvvn_val_chars;
    int     jvvn_length;
    int     jvvn_size;
};

struct jvarvalue_object {   /* jvvb_ */
#if IS_DEBUG
    char jvvb_sn[4]; /* 'B' */
#endif
    int                   jvvb_nRefs;
    struct jcontext     * jvvb_jcx;
    struct jvarrec      * jvvb_vars;
};
#if IS_DEBUG
void jvar_inc_object_refs(struct jvarvalue_object * jvvb, int val);
#define INCOBJREFS(b) (jvar_inc_object_refs((b),1))
#define DECOBJREFS(b) (jvar_inc_object_refs((b),-1))
#else
#define INCOBJREFS(b) (((b)->jvvb_nRefs)++)
#define DECOBJREFS(b) (((b)->jvvb_nRefs)--)
#endif

struct jvarvalue_objptr {   /* jvvp_ */
    struct jvarvalue              * jvvp_pval;  /* created with jvar_new_jvarvalue() */
};

struct jvarvalue_vallist {   /* jvvl_ */
    int                     jvvl_nvals;
    int                     jvvl_xvals;
    struct jvarvalue *      jvvl_avals;
};

struct jvarvalue_int_method {   /* jvvim_ */
#if IS_DEBUG
    char jvvim_sn[4];   /* 'I' */
#endif
    JVAR_INTERNAL_FUNCTION  jvvim_method;
    char *                  jvvim_method_name;
    struct jvarvalue *      jvvim_cjvv;
    int                     jvvim_nRefs;
};
#define INCIMETHREFS(m) (((m)->jvvim_nRefs)++)
#define DECIMETHREFS(m) (((m)->jvvim_nRefs)--)

#define JVVFA_FLAG_REST_PARAM   2   /* Last parameter is ... */

struct jvarvalue_funcarg {   /* jvvfa_ */
    char                          * jvvfa_arg_name;
    int                             jvvfa_flags;
    int                             jvvfa_vix;
};

#define JVVF_FLAG_GENERATOR     1   /* Function is function* */
#define JVVF_FLAG_RUN_TO_EOF    2   /* Run to EOF instead of closing } */

struct jvarvalue_function {   /* jvvf_ */
#if IS_DEBUG
    char jvvf_sn[4];    /* 'F' */
#endif
    char                          * jvvf_func_name;
    int                             jvvf_flags;
    int                             jvvf_nargs;
    int                             jvvf_xargs;
    struct jvarvalue_funcarg    **  jvvf_aargs;
    struct jxcursor                 jvvf_begin_jxc;
    struct jvarrec                * jvvf_vars;
    int                             jvvf_vix;
    int                             jvvf_nRefs;
};
#define NO_FUNCTION_NAME    "[no name]"

#define INCFUNCREFS(f) (((f)->jvvf_nRefs)++)
#define DECFUNCREFS(f) (((f)->jvvf_nRefs)--)

struct jvarvalue_lval {   /* jvvv_ */
    struct jvarvalue              * jvvv_lval;  /* Points to existing storage */
    struct jcontext               * jvvv_var_jcx;
    struct jvarvalue_object       * jvvv_jvvb;
};

struct jvarvalue_funcvar {   /* jvvfv_ */     /* Move into jvarvalue_objptr */
    struct jvarvalue_function     * jvvfv_jvvf;
    struct jcontext               * jvvfv_var_jcx;
    struct jvarvalue_int_object   * jvvfv_this_obj;
};
struct jvarvalue_imethvar {   /* jvvimv_ */     /* Move into jvarvalue_objptr */
    struct jvarvalue_int_method   * jvvimv_jvvim;
    void                          * jvvimv_this_ptr;
#if USE_JVARVALUE_IMETHVAR
    struct jvarvalue              * jvvimv_this_jvv;
#endif
};

typedef int (*JVAR_DYNAMIC_GET_FUNCTION)
        (struct jrunexec  * jx,
         struct jvarvalue_dynamic * jvvy,
         struct jvarvalue * arg,
         struct jvarvalue ** prtnjvv);

typedef int (*JVAR_DYNAMIC_SET_FUNCTION)
        (struct jrunexec  * jx,
         struct jvarvalue_dynamic * jvvy,
         struct jvarvalue * arg,
         struct jvarvalue * jvv);

typedef void (*JVAR_DYNAMIC_FREE_FUNCTION) (struct jvarvalue_dynamic * jvvy);

struct jvarvalue_dynamic {   /* jvvy_ */
#if IS_DEBUG
    char jvvy_sn[4];    /* 'Y' */
#endif
    JVAR_DYNAMIC_GET_FUNCTION   jvvy_get_proc;
    JVAR_DYNAMIC_SET_FUNCTION   jvvy_set_proc;
    JVAR_DYNAMIC_FREE_FUNCTION  jvvy_free_proc;
    void                  * jvvy_this_ptr;
    struct jvarvalue      * jvvy_arg;
    struct jvarvalue      * jvvy_rtn;
    struct jvarvalue      * jvvy_index;
};

/* #define JVV_FLAG_CONST      1 */

struct jvarvalue {   /* jvv_ */
#if IS_DEBUG
    char jvv_sn[4]; /* 'V' */
#endif
    enum e_jvv_type    jvv_dtype;
    union {
        int                                 jvv_val_bool;      /* JVV_DTYPE_BOOL            */
        JSINT                               jvv_val_jsint;     /* JVV_DTYPE_JSINT           */
        JSFLOAT                             jvv_val_jsfloat;   /* JVV_DTYPE_JSFLOAT         */
        void                              * jvv_void;          /* void*                     */
        struct jvarvalue_lval               jvv_lval;          /* JVV_DTYPE_LVAL            */
#if USE_JVV_CHARS_POINTER
        struct jvarvalue_chars            * jvv_val_chars;     /* JVV_DTYPE_CHARS           */
#else
        struct jvarvalue_chars              jvv_val_chars;     /* JVV_DTYPE_CHARS           */
#endif
        struct jvarvalue_number             jvv_val_number;    /* JVV_DTYPE_NUMBER          */
        struct jvarvalue_internal_class   * jvv_jvvi;          /* JVV_DTYPE_INTERNAL_CLASS  */
        struct jvarvalue_int_object       * jvv_jvvo;          /* JVV_DTYPE_INTERNAL_OBJECT */
        struct jvarvalue_int_method       * jvv_int_method;    /* JVV_DTYPE_INTERNAL_METHOD */
        struct jvarvalue_vallist          * jvv_jvvl;          /* JVV_DTYPE_VALLIST         */
        struct jvarvalue_token              jvv_val_token;     /* JVV_DTYPE_TOKEN           */
        struct jvarvalue_function         * jvv_val_function;  /* JVV_DTYPE_FUNCTION        */
        struct jvarvalue_funcvar            jvv_val_funcvar;   /* JVV_DTYPE_FUNCVAR         */
        struct jvarvalue_imethvar           jvv_val_imethvar;  /* JVV_DTYPE_IMETHVAR        */
        struct jvarvalue_array            * jvv_val_array;     /* JVV_DTYPE_ARRAY           */
        struct jvarvalue_object           * jvv_val_object;    /* JVV_DTYPE_OBJECT          */
        struct jvarvalue_objptr             jvv_objptr;        /* JVV_DTYPE_OBJPTR          */
        struct jvarvalue_dynamic          * jvv_val_dynamic;   /* JVV_DTYPE_DYNAMIC         */
    } jvv_val;
    int     jvv_flags; /* 02/28/2022 */
};

struct jvarvalue_array {   /* jvva_ */
    int                     jvva_nvals;
    int                     jvva_xvals;
    struct jvarvalue *      jvva_avals;
    int                     jvva_nRefs;
};
#if IS_DEBUG
void jvar_inc_array_refs(struct jvarvalue_array * jvva, int val);
#define INCARRAYREFS(a) (jvar_inc_array_refs((a),1))
#define DECARRAYREFS(a) (jvar_inc_array_refs((a),-1))
#else
#define INCARRAYREFS(a) (((a)->jvva_nRefs)++)
#define DECARRAYREFS(a) (((a)->jvva_nRefs)--)
#endif

#define JXS_FLAGS_BEGIN_FUNC    1

#define INCIOBJREFS(m) (((m)->jvvo_nRefs)++)
#define DECIOBJREFS(m) (((m)->jvvo_nRefs)--)
struct jvarvalue_int_object {   /* jvvo_ */
#if IS_DEBUG
    char jvvo_sn[4]; /* 'O' */
#endif
    int                 jvvo_nRefs;
    struct jvarvalue    jvvo_class;
    struct jvarvalue    jvvo_superclass;
    struct jcontext   * jvvo_jcx;
    void              * jvvo_this_ptr;
};

struct jvarvalue_internal_class {   /* jvvi_ */
    int                 jvvi_nRefs;
    struct jvarrec *    jvvi_jvar;
    char             *  jvvi_class_name;
    struct jvarvalue    jvvi_superclass;
    JVAR_FREE_OBJECT    jvvi_destructor;
};
#define INCICLASSHREFS(c) (((c)->jvvi_nRefs)++)
#define DECICLASSHREFS(c) (((c)->jvvi_nRefs)--)

/*****************/

struct jxstate {   /* jxs_ */
    enum e_xstatus     jxs_xstate;
    struct jxcursor    jxs_jxc;
    int                jxs_flags;
    struct jcontext  * jxs_jcx;
    int                jxs_finally_jstat;
    struct jtokeninfo * jxs_jti;    /* 12/13/2021 */
};

#define JFSFLAG_STRICT_MODE         1
#define JFSFLAG_STRICTER_MODE       2

struct jfuncstate {   /* jfs_ */
    int jfs_flags;
    //struct jxcursor    jxs_jxc;
    struct jvarvalue            jfs_rtnjvv;
    struct jvarvalue_function * jfs_jvvf;
    struct jcontext           * jfs_jcx;
    struct jcontext           * jfs_prev_jcx;
};

struct jrunexec {   /* jx_ */
    struct jxcursor         jx_jxc;
    int                     jx_flags;
    int                     jx_njs;
    int                     jx_xjs;
    struct jxstate   **     jx_js;
    int                     jx_njfs;
    int                     jx_xjfs;
    struct jfuncstate **    jx_jfs;
    int                     jx_nerrs;
    int                     jx_xerrs;
    struct jerror  **       jx_aerrs;
    struct jvarrec *        jx_globals;
    struct jcontext *       jx_global_jcx;
#if 0
    int                     jx_njtls;
    int                     jx_xjtls;
    struct jtokenlist **    jx_ajtls;
#endif
    int                     jx_nop_xref;
    short                 * jx_aop_xref;
    struct jcontext       * jx_head_jcx;                    /* 09/22/2021 */
    struct jvarvalue      * jx_type_objs[JVV_NUM_DTYPES];   /* 09/28/2021 */
    struct dbtree         * jx_module_dbt; /* tree of struct jint_module_rec * */
    //int                     jx_njcx;
    //int                     jx_xjcx;
    //struct jcontext     **  jx_ajcx;
};

#define JX_FLAG_PREPROCESSING 1     /* In preprocessing pass */
#define JX_FLAG_DEBUGFEATURES 2     /* Allow 'debug ...' */
#define JX_FLAG_NODEJS        4     /* Allow require() */

/***************************************************************/
enum e_xstatus {
      XSTAT_NONE                    = 0x0000
    , XSTAT_TRUE_IF                 = 0x0001
    , XSTAT_TRUE_ELSE               = 0x0002
    , XSTAT_TRUE_WHILE              = 0x0003
    , XSTAT_ACTIVE_BLOCK            = 0x0020
    , XSTAT_INCLUDE_FILE            = 0x0021
    , XSTAT_FUNCTION                = 0x0C22    // XSTATMASK_IS_RETURNABLE | XSTATMASK_IS_TRYABLE
    , XSTAT_BEGIN                   = 0x0023
    , XSTAT_BEGIN_THREAD            = 0x0024
    , XSTAT_TRY                     = 0x0825
    , XSTAT_CATCH                   = 0x0826
    , XSTAT_FINALLY                 = 0x0827

    , XSTAT_FALSE_IF                = 0x0111
    , XSTAT_FALSE_ELSE              = 0x0112
    , XSTAT_INACTIVE_IF             = 0x0113
    , XSTAT_INACTIVE_ELSE           = 0x0114
    , XSTAT_INACTIVE_WHILE          = 0x0115
    , XSTAT_INACTIVE_BLOCK          = 0x0121
    , XSTAT_DEFINE_FUNCTION         = 0x0122
    , XSTAT_INACTIVE_TRY            = 0x0123    /* 01/18/2022 - try was in false if, etc */
    , XSTAT_INACTIVE_CATCH          = 0x0124    /* 01/14/2022 - try completed without error */
    , XSTAT_INACTIVE_TRY_CATCH      = 0x0125    /* 01/18/2022 - in catch, try was in false if, etc */
    , XSTAT_INACTIVE_FINALLY        = 0x0126    /* 01/18/2022 - in finally, try was in false if, etc */
    , XSTAT_FAILED_CATCH            = 0x0127    /* 01/13/2022 - in catch, try failed, catch had error */

    , XSTAT_TRUE_IF_COMPLETE        = 0x0224
    , XSTAT_FALSE_IF_COMPLETE       = 0x0225
    , XSTAT_INACTIVE_IF_COMPLETE    = 0x0228
    , XSTAT_TRY_COMPLETE            = 0x0A25    /* 01/12/2022 */
    , XSTAT_CATCH_COMPLETE          = 0x0A26    /* 01/12/2022 */
    , XSTAT_FINALLY_COMPLETE        = 0x0A27    /* 01/13/2022 */

    , XSTATMASK_INACTIVE            = 0x0100
    , XSTATMASK_IS_COMPLETE         = 0x0200
    , XSTATMASK_IS_RETURNABLE       = 0x0400    /* return stops here, i.e. a function */
    , XSTATMASK_IS_TRYABLE          = 0x0800    /* return stops here, i.e. a function */
    //, XSTATMASK_IS_COMPOUND         = 0x0400    /* if, while, etc. */
    //, XSTATMASK_IS_IF               = 0x0800    /* if */
};
#define XSTAT_IS_ACTIVE(jx)   (((jx)->jx_js[(jx)->jx_njs - 1]->jxs_xstate & XSTATMASK_INACTIVE) == 0)
#define XSTAT_IS_COMPLETE(jx) (((jx)->jx_js[(jx)->jx_njs - 1]->jxs_xstate & XSTATMASK_IS_COMPLETE) != 0)
#define XSTAT_IS_RETURNABLE(jx) (((jx)->jx_js[(jx)->jx_njs - 1]->jxs_xstate & XSTATMASK_IS_RETURNABLE) != 0)
#define XSTAT_IS_TRYABLE(jx) (((jx)->jx_js[(jx)->jx_njs - 1]->jxs_xstate & XSTATMASK_IS_TRYABLE) != 0)

#define XSTAT_FLAGS(jx) ((jx)->jx_js[(jx)->jx_njs - 1]->jxs_flags)
#define XSTAT_XSTATE(jx) ((jx)->jx_js[(jx)->jx_njs - 1]->jxs_xstate)
#define XSTAT_JCX(jx) ((jx)->jx_js[(jx)->jx_njs - 1]->jxs_jcx)
#define XSTAT_FINALLY_JSTAT(jx) ((jx)->jx_js[(jx)->jx_njs - 1]->jxs_finally_jstat)
#define XSTAT_JTI(jx) ((jx)->jx_js[(jx)->jx_njs - 1]->jxs_jti)

#define JS_TOKIX(jx)  ((jx)->jx_jxc.jxc_toke_ix)
#define JS_TOKENLIST(jx) ((jx)->jx_jxc.jxc_jtl)
#define JS_TOKEN(jx) (JS_TOKENLIST(jx)->jtl_atoks[JS_TOKIX(jx)])

//#define GET_RVAL(j) (((j)->jvv_dtype == JVV_DTYPE_LVAL) ? (j)->jvv_val.jvv_lval.jvvv_lval : (j))
//#define GET_RVAL(j,k) ((((j)->jvv_dtype == JVV_DTYPE_LVAL) ? (*(k)=(j)->jvv_val.jvv_lval.jvvv_lval) : (*(k)=(j))),0)
//#define GET_RVAL(j,k) jexp_eval_rval(jx, (j), (k))

/***************************************************************/
struct jsenv * new_jsenv(int jenflags);
void free_jsenv(struct jsenv * jenv);
int jenv_initialize(struct jsenv * jse);
int jenv_xeq_string(struct jsenv * jse, const char * jsstr);
char * jenv_get_errmsg(struct jsenv * jse,
    int emflags,
    int jstat,
    int *jerrnum);
int jrun_next_token(struct jrunexec * jx, struct jtoken ** pjtok);
void jrun_peek_token(struct jrunexec * jx, struct jtoken * jtok);
int jrun_get_strict_mode(struct jrunexec * jx);
int jrun_get_stricter_mode(struct jrunexec * jx);
void jrun_init_jxcursor(struct jxcursor * tgtjxc);
void jrun_copy_current_jxc(struct jrunexec * jx, struct jxcursor * current_jxc);
void jrun_get_current_jxc(struct jrunexec * jx, struct jxcursor * current_jxc);
void jrun_set_current_jxc(struct jrunexec * jx, struct jxcursor * current_jxc);
int jrun_push_jfuncstate(struct jrunexec * jx,
    struct jvarvalue_function * fjvv,
    struct jcontext * outer_jcx);
int jrun_pop_jfuncstate(struct jrunexec * jx);
int jrun_push_xstat(struct jrunexec * jx, int xstat);
int jrun_update_xstat(struct jrunexec * jx);
void jrun_set_xstat_jxc(struct jrunexec * jx, struct jxcursor * src_jxc);
int jrun_pop_xstat(struct jrunexec * jx);
struct jvarvalue * jrun_get_return_jvv(struct jrunexec * jx);
int jrun_pop_push_xstat(struct jrunexec * jx, int xstat);
int jrun_set_strict_mode(struct jrunexec * jx);
int jrun_set_xstat(struct jrunexec * jx, int xstat);    /* 01/16/2022 */
struct jxcursor * jrun_new_jxcursor(void);              /* 01/28/2022 */
void jrun_free_jxcursor(struct jxcursor * jxc);         /* 01/28/2022 */
void jrun_setcursor(struct jrunexec * jx, struct jtokenlist * jtl, int tokix);

/***************************************************************/
#endif /* JSENVH_INCLUDED */
