#ifndef VARH_INCLUDED
#define VARH_INCLUDED
/***************************************************************/
/*  var.h --  Variable classes                                 */
/***************************************************************/

#define JX_GLOBALS_IX   0

struct jvarrec {   /* jvar_ */
    struct dbtree       * jvar_jvarvalue_dbt; /* tree of struct jvarvalue * */
    struct jvarrec      * jvar_prev;
};

/***************************************************************/


/***************************************************************/

struct jvarvalue * jvar_new_variable(void);
struct jvarrec * jvar_new_jvarrec(void);
struct jvarvalue * jvar_new_jvarvalue(void);
struct jvarvalue * jvar_find_variable(struct jrunexec * jx, const char * varnamne);
void jvar_free_jvarrec(struct jvarrec * jvar);
void jvar_push_vars(struct jrunexec * jx, struct jvarrec * jvar);
int jvar_insert_new_jvarvalue(struct jrunexec * jx,
    const char * varname,
    struct jvarvalue * jvv);
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
/***************************************************************/
#endif /* VARH_INCLUDED */
