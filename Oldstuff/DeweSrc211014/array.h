#ifndef ARRAYH_INCLUDED
#define ARRAYH_INCLUDED
/***************************************************************/
/* array.h                                                     */
/***************************************************************/

int jexp_parse_array(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    struct jvarvalue * jvv);
void jvar_free_jvarvalue_array(struct jvarvalue_array * jvva);
int jexp_parse_array_inactive(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok);
int jvar_store_array(
    struct jrunexec * jx,
    struct jvarvalue * jvvtgt,
    struct jvarvalue * jvvsrc);

/***************************************************************/
#endif /* ARRAYH_INCLUDED */
