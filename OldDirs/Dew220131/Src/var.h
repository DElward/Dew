#ifndef VARH_INCLUDED
#define VARH_INCLUDED
/***************************************************************/
/*  var.h --  Variable classes                                 */
/***************************************************************/

#define JX_GLOBALS_IX   0

#define VINVALID (-1)

struct jvarrec {   /* jvar_ */
#if IS_DEBUG
    char jvar_sn[4]; /* 'A' */
#endif
    struct dbtsi       * jvar_jvarvalue_dbt; /* tree of struct jvarvalue * */
    /*struct jvarrec      * jvar_prev;*/
    int                             jvar_nRefs;
    int                             jvar_nvars;
    int                             jvar_nconsts;
    int                             jvar_xconsts;
    int                          *  jvar_avix;
    struct jvarvalue             *  jvar_aconsts; /* 10/14/2021 Made struct jvarvalue* */
    int                          *  jvar_const_jcxflags;
};
#define INCVARRECREFS(v) (((v)->jvar_nRefs)++)
#define DECVARRECREFS(v) (((v)->jvar_nRefs)--)

/***************************************************************/


/***************************************************************/

void jvar_init_jvarvalue(struct jvarvalue * jvv);
struct jvarrec * jvar_new_jvarrec(void);
struct jvarvalue * jvar_new_jvarvalue(void);
void jvar_set_jvvflags(struct jvarvalue * jvv, int jvvflags);
int * jvar_find_in_jvarrec(struct jvarrec * jvar, const char * varname);
int jvar_insert_into_jvarrec(struct jvarrec * jvar, const char * varname);
struct jvarvalue * jvar_find_local_variable(
    struct jrunexec * jx,
    struct jcontext * local_jcx,
    const char * varname);
int jvar_insert_new_global_variable(struct jrunexec * jx,
    const char * varname,
    struct jvarvalue * jvv,
    int jcxflags);
struct jvarvalue * jvar_find_variable(
    struct jrunexec * jx,
    struct jtoken * jtok,
    struct jcontext ** pvar_jcx,
    int * jcxflags); /* in/out */
void jvar_free_jvarrec(struct jvarrec * jvar);
void jvar_push_vars(struct jrunexec * jx, struct jvarrec * jvar);
struct jvarvalue * jvar_int_object_member(struct jrunexec * jx,
    struct jvarvalue * cjvv,
    const char * mbrname,
    int * jcxflags);
int jvar_add_class_method(
    struct jrunexec * jx,
    struct jvarvalue * cjvv,
    const char * member_name,
    struct jvarvalue * mjvv,
    int jcxflags);
void jvar_free_jvarvalue_data(struct jvarvalue * jvv);
void jvar_store_null(struct jvarvalue * jvv);
void jvar_store_bool(
    struct jvarvalue * jvv,
    int val_jsbool);
void jvar_store_jsint(
    struct jvarvalue * jvv,
    JSINT val_jsint);
void jvar_store_jsfloat(
    struct jvarvalue * jvv,
    JSFLOAT val_jsfloat);
int jvar_store_jvarvalue(
    struct jrunexec * jx,
    struct jvarvalue * jvvtgt,
    struct jvarvalue * jvvsrc);
int jvar_store_jvarvalue_n(
    struct jrunexec * jx,
    struct jvarvalue ** p_jvvtgt,
    struct jvarvalue * jvvsrc);
int jvar_validate_varname(
    struct jrunexec * jx,
    struct jtoken * jtok);
void jvar_free_jvarvalue(struct jvarvalue * jvv);
struct jvarrec * jvar_pop_vars(struct jrunexec * jx);
int jvar_is_valid_varname(
    struct jrunexec * jx,
    struct jtoken * jtok);
int jvar_find_local_variable_index(struct jrunexec * jx,
    struct jvarvalue_function * jvvf,
    const char * varname);
int jvar_new_local_variable(struct jrunexec * jx,
    struct jvarvalue_function * jvvf,
    const char * varname);
int jvar_new_ad_hoc_variable(struct jrunexec * jx,
    struct jcontext * jcx,
    const char * varname,
    struct jvarvalue * jvv,
    int jcxflags);
int jvar_new_local_function(struct jrunexec * jx,
    struct jtokeninfo * jti_brace,
    struct jvarvalue          * fjvv_child,
    int jcxflags,
    int skip_flags);
void jvar_free_jcontext(struct jcontext * jcx);
struct jcontext * jvar_get_global_jcontext(struct jrunexec * jx);
struct jvarrec * jvar_get_global_jvarrec(struct jrunexec * jx);
struct jcontext * jvar_get_global_jcontext(struct jrunexec * jx);
struct jcontext * jvar_new_jcontext(struct jrunexec * jx,
    struct jvarrec * jvar,
    struct jcontext * outer_jcx);
void jvar_free_function_context(struct jcontext * jcx);
/* void jvar_free_jvarrec_consts(struct jvarrec * jvar); */
/* struct jcontext * jvar_get_current_jcontext(struct jrunexec * jx); */
struct jfuncstate * jvar_get_current_jfuncstate(struct jrunexec * jx);
struct jvarvalue * jvar_get_jvv_from_vix(struct jrunexec * jx,
    struct jcontext * jcx,
    int vix,
    int * jcxflags);
struct jvarvalue ** jvar_get_pjvv_from_vix(struct jrunexec * jx,
    struct jcontext * jcx,
    int vix,
    int * jcxflags);
struct jvarvalue * jvar_get_jvv_with_expand(struct jrunexec * jx,
    struct jcontext * jcx,
    int vix,
    int * jcxflags);
struct jcontext * jvar_get_head_jcontext(struct jrunexec * jx);
void jvar_set_head_jcontext(struct jrunexec * jx, struct jcontext * jcx);
struct jvarvalue_int_method * jvar_new_int_method(
    const char * method_name,
    JVAR_INTERNAL_FUNCTION ifuncptr,
    struct jvarvalue * cjvv);
char * jvar_get_dtype_name(int dtype);
int jvar_dup_jvarvalue(
    struct jrunexec * jx,
    struct jvarvalue ** p_jvvtgt,
    struct jvarvalue * jvvsrc);

#define ENSINT_FLAG_INTERR  1
int jrun_ensure_int(
struct jrunexec * jx,
struct jvarvalue * jvv,
    int *intval,
    int ensflags);
int jvar_get_block_jcontext(
                    struct jrunexec * jx,
                    const char * varname,
                    struct jcontext ** pjcx);
/***************************************************************/
#endif /* VARH_INCLUDED */
