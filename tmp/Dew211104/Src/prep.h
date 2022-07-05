#ifndef PREPH_INCLUDED
#define PREPH_INCLUDED
/***************************************************************/
/*  prep.h -- Preprocessing functions                          */
/***************************************************************/

#define SKIP_FLAG_HOIST         1
#define SKIP_FLAG_UNTIL_EOF     2

/***************************************************************/
int jprep_preprocess_block(
                    struct jrunexec * jx,
                    struct jvarvalue_function * jvvf,
                    struct jtoken ** pjtok,
                    int skip_flags);
int jprep_preprocess_main(
    struct jrunexec * jx,
    struct jvarvalue_function ** pjvvf);
int jprep_skip_generic_statement(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    int skip_flags);

/***************************************************************/
#endif /* PREPH_INCLUDED */
