#ifndef STMTH_INCLUDED
#define STMTH_INCLUDED
/***************************************************************/
/* stmt.h                                                      */
/***************************************************************/

#define USE_COMMAND_TABLE   0

#if (USE_COMMAND_TABLE & 1) != 0
typedef int (*JCR_COMMAND_FUNCTION)
        (struct jrunexec * jx,
         struct jtoken ** pjtok);
#endif

int jrun_exec_block(struct jrunexec * jx, struct jtoken ** pjtok);

/***************************************************************/
#endif /* STMTH_INCLUDED */
