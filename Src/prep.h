#ifndef PREPH_INCLUDED
#define PREPH_INCLUDED
/***************************************************************/
/*  prep.h -- Preprocessing functions                          */
/***************************************************************/

#define SKIP_FLAG_HOIST         1

//#define SKIP_FLAG_FUNCTION_EXP_MASK     (SKIP_FLAG_HOIST)

/***************************************************************/
int jprep_preprocess_block(
                    struct jrunexec * jx,
                    struct jvarvalue_function * jvvf,
                    struct jtoken ** pjtok,
                    int skip_flags);
int jprep_preprocess_main(
                    struct jrunexec * jx,
                    struct jvarvalue_function ** pjvvf);
struct jtokeninfo * jprep_ensure_jtokeninfo(
                    struct jrunexec * jx,
                    enum e_jti_type titype,
                    struct jtoken * jtok);
int jprep_expression(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    int skip_flags);
int jprep_eval_assignment(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    int skip_flags);
int jprep_eval_function_call(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    int skip_flags);
int jprep_eval_primary(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    int skip_flags);
/***************************************************************/
#endif /* PREPH_INCLUDED */
