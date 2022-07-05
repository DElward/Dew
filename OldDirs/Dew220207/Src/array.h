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
//int jvar_store_array(
//    struct jrunexec * jx,
//    struct jvarvalue * jvvtgt,
//    struct jvarvalue * jvvsrc);
int jexp_compare_arrays(
    struct jvarvalue_array * jvva1,
    struct jvarvalue_array * jvva2);
int jint_Array_length(
        struct jrunexec  * jx,
        const char       * func_name,
        void             * this_ptr,
        struct jvarvalue * jvvlargs,
        struct jvarvalue * jvvrtn);
int jint_Array_subscript(
        struct jrunexec  * jx,
        const char       * func_name,
        void             * this_ptr,
        struct jvarvalue * jvvlargs,
        struct jvarvalue * jvvrtn);
int jprep_parse_array_inactive(
                    struct jrunexec * jx,
                    struct jtoken ** pjtok,
                    int skip_flags);

/***************************************************************/
#endif /* ARRAYH_INCLUDED */
