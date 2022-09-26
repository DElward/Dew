#ifndef JSEXPH_INCLUDED
#define JSEXPH_INCLUDED
/***************************************************************/
/*  jsexp.h --  JavaScript expressions                         */
/***************************************************************/

#define COPY_JVARVALUE(t,s) (memcpy((t), (s), sizeof(struct jvarvalue)))
#if IS_DEBUG
    //#define INIT_JVARVALUE(v) ((v)->jvv_dtype = JVV_DTYPE_NONE);((v)->jvv_sn[0]='#');((v)->jvv_sn[1]='\0')
    #define INIT_JVARVALUE(v) ((v)->jvv_dtype = JVV_DTYPE_NONE)
#else
    #define INIT_JVARVALUE(v) ((v)->jvv_dtype = JVV_DTYPE_NONE)
#endif

typedef int (*JXE_BINARY_OPERATOR_FUNCTION)
        (struct jrunexec          * jx,
         struct jvarvalue         * jvv1,
         struct jvarvalue         * jvv2);
typedef int (*JXE_UNARY_OPERATOR_FUNCTION)
        (struct jrunexec          * jx,
         struct jvarvalue         * jvvans,
         struct jvarvalue         * jvv1);

#define JX_DOT_PREC 15
struct jexp_exp_rec { /* jxe_ */
    short jxe_bin_precedence;
    short jxe_unop_precedence;
    int jxe_pu;
    int jxe_opflags;
    JXE_BINARY_OPERATOR_FUNCTION jxe_binop;
    JXE_UNARY_OPERATOR_FUNCTION  jxe_unop;
};
#define JXE_OPFLAG_RTOL_ASSOCIATIVITY    1
#define JXE_OPFLAG_UNARY_OPERATION       2
#define JXE_OPFLAG_BINARY_OPERATION      4
#define JXE_OPFLAG_LOGICAL_OR            8  /* if 1st op true, do not eval 2nd op */
#define JXE_OPFLAG_LOGICAL_AND          16  /* if 1st op false, do not eval 2nd op */
#define JXE_OPFLAGS_LOGICAL             (JXE_OPFLAG_LOGICAL_AND | JXE_OPFLAG_LOGICAL_OR)
#define JXE_OPFLAG_EQ(f,v) (((f) & (v)) == (v))
#define JXE_OPFLAG_CLOSE_PAREN          32
#define JXE_OPFLAG_TOKEN                64
#define JXE_OPFLAG_FUNCTION_CALL        128
//#define JXE_OPFLAG_OPEN_BRACKET         256
#define JXE_OPFLAG_POST_UNARY_OPERATION 512     /* 02/18/2022 */
#define JXE_OPFLAG_HIPRI               1024     /* 08/24/2022 */
/* See: jexp_print_stack() */

//#define HIGHEST_PRECEDENCE      2
#define GET_XLX(tok) \
(((tok)->jtok_typ == JSW_PU || (tok)->jtok_typ == JSW_KW)?jx->jx_aop_xref[(tok)->jtok_kw]:(-1))

/***************************************************************/
void jexp_init_op_xref(struct jrunexec * jx);
int jexp_eval_value(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    struct jvarvalue * jvv);
int jexp_evaluate(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    struct jvarvalue * jvv);
void jvar_store_chars_len(
    struct jvarvalue * jvvtgt,
    const char * jchars,
    int jcharslen);
int jexp_istrue(struct jrunexec * jx,
         struct jvarvalue         * jvv1,
         int * istrue);
int jexp_evaluate_inactive(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok);
int jrun_exec_function_stmt(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok);
void jvar_free_jvarvalue_function(struct jvarvalue_function * jvvf);
int jrun_exec_return_stmt(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok);
void jexp_quick_tostring(
                    struct jrunexec * jx,
                    struct jvarvalue * jvv,
                    char * valstr,
                    int valstrsz);
void jexp_quick_print(
                    struct jrunexec * jx,
                    struct jvarvalue * jvv,
                    char * msg);
#if IS_DEBUG
int jrun_debug_chars(
                    struct jrunexec * jx,
                    char * dbgstr);
#endif
int jexp_eval_assignment_inactive(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok);
int jexp_eval_assignment(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    struct jvarvalue * jvv);
int jexp_eval_list(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    struct jvarvalue * jvv);
struct jvarvalue_function * new_jvarvalue_function(void);
int jvar_parse_function(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    struct jvarvalue * jvv,
                    int skip_flags);
int jexp_call_function(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    struct jvarvalue_function * fjvv,
                    struct jvarvalue * this_jvv,
                    struct jvarvalue * jvvparms,
                    struct jcontext * outer_jcx,
                    struct jvarvalue * rtnjvv);
int jexp_eval_assignment_inactive(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok);
int jvar_assign_jvarvalue(
    struct jrunexec * jx,
    struct jvarvalue * jvvtgt,
    struct jvarvalue * jvvsrc);

//  int jexp_eval_rval(struct jrunexec * jx,
//      struct jvarvalue * jvv,
//      struct jvarvalue ** prtnjvv);
int jexp_get_rval(struct jrunexec * jx,
    struct jvarvalue ** ptgtjvv,
    struct jvarvalue * srcjvv,
    int rvalflags);
#define RVAL_FLAGS_KEEP_POINTER     1
struct jexp_exp_rec * jeval_get_exp_list(void);
int jexp_eval_function_stmt_inactive(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok);
int jvar_parse_function_arguments_inactive(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok);
void jvar_store_lval(
    struct jrunexec * jx,
    struct jvarvalue * jvvtgt,
    struct jvarvalue * jvvsrc,
    struct jcontext * var_jcx);
/***************************************************************/

/***************************************************************/
#endif /* JSEXPH_INCLUDED */
