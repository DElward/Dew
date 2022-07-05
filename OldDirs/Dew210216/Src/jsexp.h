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

/***************************************************************/

/***************************************************************/
#endif /* JSEXPH_INCLUDED */
