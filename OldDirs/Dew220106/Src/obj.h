#ifndef OBJH_INCLUDED
#define OBJH_INCLUDED
/***************************************************************/
/* obj.h                                                       */
/***************************************************************/

int jexp_parse_object(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    struct jvarvalue * jvv);
int jexp_parse_object_inactive(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok);
int jvar_store_object(
    struct jrunexec * jx,
    struct jvarvalue * jvvtgt,
    struct jvarvalue * jvvsrc);

/***************************************************************/
#endif /* OBJH_INCLUDED */
