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
int jprep_parse_object_inactive(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    int skip_flags);
void job_free_jvarvalue_object(struct jvarvalue_object * jvvb);
int jpr_object_tostring(
    struct jrunexec * jx,
    char ** prbuf,
    int * prlen,
    int * prmax,
    struct jvarvalue_object * jvvb,
    int jfmtflags);
#if FIX_220830
int jexp_binop_dot_object(struct jrunexec * jx,
    struct jvarvalue * jvvobject,
    struct jvarvalue * jvvparent,
    struct jvarvalue * jvvfield);
#else
int jexp_binop_dot_object(struct jrunexec * jx,
    struct jvarvalue * jvvobject,
    struct jvarvalue_object * jvvb,
    struct jvarvalue * jvvfield);
#endif
struct jvarvalue_object * job_new_jvarvalue_object(struct jrunexec * jx);

/***************************************************************/
#endif /* OBJH_INCLUDED */
