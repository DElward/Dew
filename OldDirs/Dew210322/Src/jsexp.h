#ifndef JSEXPH_INCLUDED
#define JSEXPH_INCLUDED
/***************************************************************/
/*  jsexp.h --  JavaScript expressions                         */
/***************************************************************/

#define INIT_JVARVALUE(v) ((v)->jvv_dtype = JVV_DTYPE_NONE)
#define COPY_JVARVALUE(t,s) (memcpy((t), (s), sizeof(struct jvarvalue)))

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
#if IS_DEBUG
int jrun_debug_chars(
                    struct jrunexec * jx,
                    char * dbgstr);
#endif
int jexp_eval_assignment_inactive(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok);
int jexp_eval_list(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    struct jvarvalue * jvv);
int jexp_evaluate_inactive(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok);
struct jvarvalue_function * new_jvarvalue_function(void);
int jvar_parse_function(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    struct jvarvalue * jvv);
int jexp_call_function(
                    struct jrunexec * jx,
                    struct jvarvalue * fjvv,
                    struct jvarvalue * jvvparms,
                    struct jcontext * outer_jcx,
                    struct jvarvalue * rtnjvv);

/***************************************************************/

/***************************************************************/
#endif /* JSEXPH_INCLUDED */
