#ifndef JSEXPH_INCLUDED
#define JSEXPH_INCLUDED
/***************************************************************/
/*  jsexp.h --  JavaScript expressions                         */
/***************************************************************/

#define USE_TABLE_EVAL          1
#define USE_TABLE_EVAL_V1       0
#define USE_TABLE_EVAL_V2       1

#define INIT_JVARVALUE(v) ((v)->jvv_dtype = JVV_DTYPE_NONE)
#define COPY_JVARVALUE(t,s) (memcpy((t), (s), sizeof(struct jvarvalue)))

/***************************************************************/
void jexp_init_op_xref(struct jrunexec * jx);
int jexp_evaluate(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    struct jvarvalue * jvv);
void jvar_store_chars_len(
    struct jvarvalue * jvvtgt,
    const char * jchars,
    int jcharslen);

/***************************************************************/

/***************************************************************/
#endif /* JSEXPH_INCLUDED */
