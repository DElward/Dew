#ifndef XEQH_INCLUDED
#define XEQH_INCLUDED
/*
**  XEQH  --  Header file for XEQC.C
*/
/***************************************************************/

#define XML_ERRMSG_SIZE     128

typedef int (*SVAR_INTERNAL_FUNCTION)
        (struct svarexec * svx,
        const char * func_name,
        int nsvv,
        struct svarvalue ** asvv,
        struct svarvalue *  rsvv);

struct svarcursor {   /* svc */
    int     svc_str_ix;
    int     svc_char_ix;
    int     svc_adv;
};

struct svarstate {   /* svs_ */
    enum e_xstatus     svs_xstate;
    struct svarcursor  svs_svc;
};

struct svarinput {   /* svi_ */
    struct strlist *    svi_strl;
    struct svarcursor   svi_pc;
    int                 svi_strl_needs_free;
};

struct svartokensave {   /* svts_ */
    struct strlist * svts_strl;
};
#define MAX_INCLUDE_STACK     256   /* See also MAX_XSTAT_STACK */
#define MAX_TOKEN_LINE_LENGTH 80

struct svarglob {   /* svg_ */
    struct svartokensave ** svg_asvts;      /* MT RO */
    int                     svg_nsvts;      /* MT RO */
    int                     svg_xsvts;      /* MT RO */
    int                     svg_svt_depth;  /* MT RO */
    int                     svg_nvars;
    int                     svg_mvars;
    struct svarrec **       svg_avars;      /* MT RO svx_avars[0] and svx_avars[1] */
#if IS_MULTITHREADED
    struct threadlist *     svg_thl;
#endif
};

struct svarexec {   /* svx_ */
    int                     svx_nsvs;
    struct svarstate **     svx_svs;
    int                     svx_xsvs;
    struct svarinput **     svx_asvi;
    int                     svx_nsvi;
    int                     svx_xsvi;
    struct svarinput *      svx_svi;
    struct svarcursor       svx_token_pc;
    void *                  svx_gdata;
    int                     svx_svstat;
    char *                  svx_errmsg;
    int                     svx_nvars;
    int                     svx_mvars;
    struct svarrec **       svx_avars;      /* MT RO svx_avars[0] and svx_avars[1] */

    /* svarglob handling */
    struct svarglob *       svx_locked_svg;
    void            *       svx_vsvg;
    int                     svx_lock_count;
}; /* MT RO = Read-only in multi-threaded environment -- svx_nrefs > 1 */

struct svarvalue_chars {   /* svvc_ */
    char *  svvc_val_chars;
    int svvc_length;
    int svvc_size;
};
/*
struct svarvalue_internalclass {
    int                 svic_class_id;
    struct svarrec *    svic_svrec;
};
*/
struct svarvalue_int_class {   /* svnc_ */
    int                 svnc_nrefs;
    int                 svnc_class_id;
    struct svarrec *    svnc_svrec;
};
struct svarvalue_ptr {   /* svvp_ */
    void *  svvp_val_ptr;
    int     svvp_class_id;
    void (*svvp_destructor)(void*);
    void * (*svvp_copy)(void*);
};
/*
struct svarvalue_object_value {
    void *             svvov_val_ptr;
    int                svvov_nrefs;
};
struct svarvalue_object {
    struct svarvalue              * svvo_class;
    struct svarvalue_object_value * svvo_svvov;
};
*/
struct svarvalue_int_object_value {   /* svviv_ */
    void *             svviv_val_ptr;
    int                svviv_nrefs;
};
struct svarvalue_int_object {   /* svvi_ */
    struct svarvalue_int_class    * svvi_int_class;
    struct svarvalue_int_object_value * svvi_svviv;
};
struct svarvalue_array {   /* sva_ */
    int    sva_nelement;
    int    sva_xelement;
    struct svarvalue ** sva_aelement;
    int    sva_nrefs;
};
struct svarformalparms {   /* svfp_ */
    int         svfp_nfp;
    char **     svfp_afp;
};
struct svarfunction {   /* svf_ */
    struct svarformalparms * svf_formals;
    struct svartokensave *   svf_body;
    char *                   svf_funcname;
    int                      svf_nrefs; /* do not free until 0 */
};
struct svarvalue {   /* svv */
    short    svv_flags;
    short    svv_dtype;
    union {
        int     svv_val_bool;
        int32   svv_val_int;
        int64   svv_val_long;
        void *  svv_void;
        struct svarvalue_chars svv_chars;
        double  svv_val_double;
        struct svarvalue_ptr svv_ptr;
        /* struct svarvalue_internalclass svv_svic; */
        SVAR_INTERNAL_FUNCTION svv_method;
        /* struct svarvalue_object svv_svvo; */
        struct svarfunction * svv_svf;
        struct svarvalue_array * svv_array;
        struct svarvalue_int_class * svv_svnc;
        struct svarvalue_int_object svv_svvi;
        SVAR_INTERNAL_FUNCTION svv_int_func;
    } svv_val;
};
struct svartoken {   /* svt */
    char *  svt_text;
    int     svt_tlen;
    int     svt_tmax;
};

#define SVV_FLAG_ACCESSED           1

#define SVV_DTYPE_NONE              0
#define SVV_DTYPE_BOOL              1
#define SVV_DTYPE_INT               2
#define SVV_DTYPE_LONG              3 /* unsupported */
#define SVV_DTYPE_DOUBLE            4 /* unsupported */
#define SVV_DTYPE_CHARS             5
#define SVV_DTYPE_PTR               6
/* #define SVV_DTYPE_CLASS             7   -- Old internal class - deprecated */
#define SVV_DTYPE_METHOD            8
/* #define SVV_DTYPE_OBJECT            9   -- Old internal object - deprecated */
#define SVV_DTYPE_USER_FUNCTION     10
#define SVV_DTYPE_NO_VALUE          11  /* Used by call statement */
#define SVV_DTYPE_ARRAY             12
#define SVV_DTYPE_INT_CLASS         13  /* New internal class */
#define SVV_DTYPE_INT_OBJECT        14  /* New internal object */
#define SVV_DTYPE_INT_FUNCTION      15  /* New internal function */

struct svarrec {   /* svar_ */
    struct dbtree       * svar_svarvalue_dbt; /* tree of struct svarvalue * */
};
#define RETURN_VARIABLE_NAME    "$RTN"
#define MAX_ARRAY_INDEX         16384

#define SVAR_FLAG_REMOVE_OPTIONAL_DOUBLE_QUOTES 1
#define SVAR_FLAG_REMOVE_OPTIONAL_SINGLE_QUOTES 2
#define SVAR_FLAG_NO_DUPLICATE_VAR              4

#define SVARERR_FAIL                            (-6801)
#define SVARERR_MISSING_REQUIRED_VAR            (-6802)
#define SVARERR_UNRECOGNIZED_VAR                (-6803)
#define SVARERR_UNRECOGNIZED_STMT               (-6804)
#define SVARERR_BAD_VARNAME                     (-6805)
#define SVARERR_EXPECT_EQUAL                    (-6806)
#define SVARERR_OPERATOR_TYPE_MISMATCH          (-6807)
#define SVARERR_UNDEFINED_VARIABLE              (-6808)
#define SVARERR_BAD_DELIMITER                   (-6809)
#define SVARERR_NO_COPY_AVAILABLE               (-6810)
#define SVARERR_EXP_STMT_TERMINATOR             (-6811)
#define SVARERR_BAD_NUMBER                      (-6812)
#define SVARERR_EXP_CLOSE_PAREN                 (-6813)
#define SVARERR_INVALID_NUM_PARMS               (-6814)
#define SVARERR_INVALID_PARM_TYPE               (-6815)
#define SVARERR_RUNTIME_STACK_OVERFLOW          (-6816)
#define SVARERR_EXPECT_BOOLEAN                  (-6817)
#define SVARERR_EXPECT_IF                       (-6818)
#define SVARERR_EXP_BLOCK_BEGIN                 (-6819)
#define SVARERR_BAD_FUNCTION                    (-6820)
#define SVARERR_DUPLICATE_VAR                   (-6821)
#define SVARERR_NOT_PERMITTED_HERE              (-6822)
#define SVARERR_RUNTIME_STACK_UNDERFLOW         (-6823)
#define SVARERR_RUNTIME_STACK_ITEMS_REMAIN      (-6824)
#define SVARERR_EXP_SEMICOLON_TERMINATOR        (-6825)
#define SVARERR_FUNCTION_FAILED                 (-6826)
#define SVARERR_UNRECOGNIZED_ARG                (-6827)
#define SVARERR_BAD_DOT                         (-6828)
#define SVARERR_UNMATCHED_QUOTE                 (-6829)
#define SVARERR_MEMBER_NOT_METHOD               (-6830)
#define SVARERR_CANNOT_FIND_MEMBER              (-6831)
#define SVARERR_SET_ENDPOINT                    (-6832)
#define SVARERR_NEW_MUST_BE_CLASS               (-6833)
#define SVARERR_CANNOT_BE_CLASS                 (-6834)
#define SVARERR_OPEN_INCLUDE                    (-6835)
#define SVARERR_READ_INCLUDE                    (-6836)
#define SVARERR_INCLUDE_STACK_OVERFLOW          (-6837)
#define SVARERR_INCLUDE_FILE_UNDERFLOW          (-6838)
#define SVARERR_NOT_UNIQUE                      (-6839)
#define SVARERR_DUPLICATE_FORMAL                (-6840)
#define SVARERR_EXPECT_COMMA                    (-6841)
#define SVARERR_EXPECT_BEGIN                    (-6842)
#define SVARERR_TOKEN_STACK_OVERFLOW            (-6843)
#define SVARERR_TOKEN_STACK_UNDERFLOW           (-6844)
#define SVARERR_TOO_MANY_PARMS                  (-6845)
#define SVARERR_UNDEFINED_CLASS                 (-6846)
#define SVARERR_THREAD_START_ERROR              (-6847)
#define SVARERR_BAD_ARGS                        (-6848)
#define SVARERR_NEGATIVE_ARRAY_INDEX            (-6849)
#define SVARERR_ARRAY_INDEX_TOO_BIG             (-6850)
#define SVARERR_EXP_CLOSE_BRACKET               (-6851)
#define SVARERR_BAD_BRACKET                     (-6852)
#define SVARERR_ARRAY_INDEX_BOUNDS              (-6853)
#define SVARERR_FUNCTION_CALL_NEEDS_PARENS      (-6854)
#define SVARERR_EXP_OPEN_PAREN                  (-6855)
#define SVARERR_BAD_ARGMASK                     (-6856)
#define SVARERR_DUPLICATE_INT_CLASS             (-6857)
#define SVARERR_DUPLICATE_INT_FUNC              (-6858)
#define SVARERR_INVALID_ENUMERATE               (-6859)
#define SVARERR_EXP_IF                          (-6860)

#define SVARERR_DUPLICATE_INT_FUNC              (-6858)

#define SVARERR_EOF                             (-1)
#define SVARERR_EOF_ERROR                       (-2)
#define SVARERR_STOPPED                         (-3)
#define SVARERR_EXIT                            (-4)
#define SVARERR_RETURN                          (-5)

/***************************************************************/
/*
-- Bit masks:
**      0x0100  Statement is inactive
**      0x0200  Statement is complete
**      0x0400  Statement is block (multi-statement)
*/
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
#define MAX_XSTAT_STACK     512     /* See also MAX_INCLUDE_STACK */
#define XSTAT_IS_ACTIVE(s) (((s)->svx_svs[(s)->svx_nsvs - 1]->svs_xstate & XSTATMASK_INACTIVE) == 0)
#define XSTAT_IS_COMPLETE(s) (((s)->svx_svs[(s)->svx_nsvs - 1]->svs_xstate & XSTATMASK_IS_COMPLETE) != 0)
#define XSTAT_IS_COMPOUND(s) (((s)->svx_svs[(s)->svx_nsvs - 1]->svs_xstate & XSTATMASK_IS_COMPOUND) != 0)

/* svx->svx_svs[svx->svx_nsvs - 1]->svs_xstate */

#define DISPFLAG_QUOTES     1   /* Enclose strings in quotes */
#define DISPFLAG_ESCAPE     2   /* Escape strings with backslahes */


/***************************************************************/
#define INIT_SVARVALUE(v) (memset((v), 0, sizeof(struct svarvalue)))
#define COPY_SVARVALUE(t,s) (memcpy((t), (s), sizeof(struct svarvalue)))

struct svarrec * svar_new(void);
void svar_free(struct svarrec * svrec);

int svar_set(
    struct svarrec * svrec,
    const char * svarnam,
    const char * svarval,
    int svarflags);

int svar_exec_strlist(struct svarrec * svrec, struct strlist * strl);
void svare_shut(struct svarexec * svx);
int svar_exec(struct svarexec * svx);
char * svar_get_error_message(struct svarexec * svx, int esvstat);

 /* 12/05/2017 */
int svar_get_int_variable(struct svarexec * svx,
        struct svarvalue * svv,
        int * p_num);
int process_xeq(struct omarglobals * og, const char * xeqfame);

int svare_set_error(struct svarexec * svx, int svstat, char *fmt, ...);
struct svarvalue * svare_new_variable(void);
struct svarvalue * svare_find_variable(struct svarexec * svx,
    struct svarvalue * csvv,
    const char * vnam);
void svare_svarvalue_bool(
    struct svarvalue * svv,
    int val_bool);
void svare_store_object(
    struct svarexec * svx,
    struct svarvalue * csvv,
    struct svarvalue * svv,
    void * val_ptr);
void svare_store_int_object(
    struct svarexec * svx,
    struct svarvalue_int_class * svnc,
    struct svarvalue * svv,
    void * val_ptr);
int svar_get_double_variable(struct svarexec * svx,
        struct svarvalue * svv,
        double * p_num);
void svare_svarvalue_double(
    struct svarvalue * svv,
    double val_double);
void svare_store_string(
    struct svarvalue * svv,
    const char * bufval,
    int          buflen);
void svare_store_string_pointer(
struct svarvalue * svv,
    char * bufval,
    int buflen);
void svare_svarvalue_int(
struct svarvalue * svv,
    int32 val_int);
struct svarvalue * svare_find_class(struct svarexec * svx,
    const char * cnam);
struct svarexec * svare_dup_svarexec(struct svarexec * svx);
void svare_release_svarexec(struct svarexec * svx);
int svare_call_user_function(struct svarexec * svx,
        struct svarfunction * svf,
        int nsvv,
        struct svarvalue ** asvv,
        struct svarvalue *  rsvv);
void svar_free_svarvalue(struct svarvalue * svv);
int svare_store_svarvalue(struct svarexec * svx,
        struct svarvalue * tsvv,
        struct svarvalue * ssvv);
int svare_push_xstat(struct svarexec * svx, int xstat);
int svare_pop_xstat(struct svarexec * svx, int old_xstat);
char * svare_svarvalue_to_string(struct svarexec * svx,
                            struct svarvalue * svv,
                                int dispflags);
void svare_get_svarglob(struct svarexec * svx);
void svare_release_svarglob(struct svarexec * svx);

/***************************************************************/
#endif /* XEQH_INCLUDED */
