#ifndef VARH_INCLUDED
#define VARH_INCLUDED
/***************************************************************/
/*  var.h --  Variable classes                                 */
/***************************************************************/

#define JX_GLOBALS_IX   0

#define VINVALID (-1)

struct jvarrec {   /* jvar_ */
    struct dbtsi       * jvar_jvarvalue_dbt; /* tree of struct jvarvalue * */
    /*struct jvarrec      * jvar_prev;*/
    int                             jvar_nvars;
    int                             jvar_nconsts;
    int                             jvar_xconsts;
    struct jvarvalue            **  jvar_aconsts;
};

/***************************************************************/


/***************************************************************/

struct jvarrec * jvar_new_jvarrec(void);
struct jvarvalue * jvar_new_jvarvalue(void);
int * jvar_find_in_jvarrec(struct jvarrec * jvar, const char * varname);
int jvar_insert_into_jvarrec(struct jvarrec * jvar, const char * varname);
void jvv_free_jvarvalue(struct jvarvalue * jvv);
struct jvarvalue * jvar_find_global_variable(struct jrunexec * jx, const char * varname);
int jvar_insert_new_global_variable(struct jrunexec * jx,
    const char * varname,
    struct jvarvalue * jvv);
struct jvarvalue * jvar_find_variable(struct jrunexec * jx, const char * varname);
void jvar_free_jvarrec(struct jvarrec * jvar);
void jvar_push_vars(struct jrunexec * jx, struct jvarrec * jvar);
struct jvarvalue * jvar_object_member(struct jrunexec * jx,
    struct jvarvalue * cjvv,
    const char * mbrname);
int jvar_add_class_method(
    struct jrunexec * jx,
    struct jvarvalue * cjvv,
    const char * member_name,
    struct jvarvalue * mjvv);
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
void jvar_store_jvarvalue(
    struct jrunexec * jx,
    struct jvarvalue * jvvtgt,
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
    struct jvarrec * jvar,
    struct jcontext * jcx,
    const char * varname,
    struct jvarvalue * jvv);
int jvar_new_local_function(struct jrunexec * jx,
    struct jvarvalue_function * jvvf_parent,
    struct jvarvalue_function * jvvf_child);
void jvar_free_jcontext(struct jcontext * jcx);
struct jcontext * jvar_get_global_jcontext(struct jrunexec * jx);
struct jvarrec * jvar_get_global_jvarrec(struct jrunexec * jx);
struct jcontext * jvar_get_new_jcontext(struct jrunexec * jx);
void jvar_get_current_jcontext(struct jrunexec * jx,
    struct jvarrec ** pjvar,
    struct jcontext ** pjcx);
struct jcontext * jvar_new_jcontext(struct jvarrec * jvar);

/***************************************************************/
#endif /* VARH_INCLUDED */
