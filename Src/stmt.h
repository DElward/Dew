#ifndef STMTH_INCLUDED
#define STMTH_INCLUDED
/***************************************************************/
/* stmt.h                                                      */
/***************************************************************/

typedef int (*JCR_COMMAND_FUNCTION)
        (struct jrunexec * jx,
         struct jtoken ** pjtok);

int jrun_exec_block(struct jrunexec * jx,
                    struct jtoken ** pjtok);
int jrun_get_cmd_rec_list_size(struct jrunexec * jx);

/***************************************************************/
#endif /* STMTH_INCLUDED */
