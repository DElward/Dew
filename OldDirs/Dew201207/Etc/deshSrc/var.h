/***************************************************************/
/* var.h                                                       */
/***************************************************************/

#define DBTREE_READ_DEFAULT     0
#define DBTREE_READBACKWARDS    1   /* unimplemented */
#define DBTREE_READ_TREE        2   /* unimplemented */
#define DBTREE_READ_QUICK       8

struct dbtree * dbtree_new(void);
void dbtree_free(struct dbtree * dbt, void (*free_proc)(void*));
int dbtree_insert(struct dbtree * dbt,
                  const void * keyval,
                  int keyvallen,
                  void * pdata);
void ** dbtree_find(struct dbtree * dbt, const void * keyval, int keyvallen);

void * dbtree_rewind(struct dbtree * dbt, int mode);
void ** dbtree_next(void * vdbtc);
void ** dbtree_next_k(void * vdbtc, char ** keynam);
void dbtree_close(void * vdbtc);
int dbtree_cursor_delete(void * vdbtc);

/* specials */
int dbtree_gethash(const void * keyval, int keyvallen);
void ** dbtree_find_gethash(struct dbtree * dbt,
                            int * pkh,
                            const void * keyval,
                            int keyvallen);
void ** dbtree_find_hashed(struct dbtree * dbt,
                            int kh,
                            const void * keyval,
                            int keyvallen);
int dbtree_insert_hashed(struct dbtree * dbt,
                  int kh,
                  const void * keyval,
                  int keyvallen,
                  void * pdata);
int dbtree_delete_hashed(struct dbtree * dbt, int kh, const void * keyval, int keyvallen);

struct dbtree * load_envp(char *envp[], char *** hlenvp, int * pnenv, int upsnams);
void unload_vars(struct dbtree * dbt);
void setvar_str(struct cenv * ce,
                char * vnam,
                int vnlen,
                char * vval);
void setvar_var(struct cenv * ce,
                char * vnam,
                int vnlen,
                struct var * vval);
void set_ce_var(struct cenv * ce,
                  const char * vnam,
                  int vnlen,
                  struct var * vval);
void set_ce_var_str(struct cenv * ce,
                  const char * vnam,
                  int vnlen,
                  const char * vval,
                  int vvlen);
void set_ce_var_num(struct cenv * ce,
                  const char * vnam,
                  int vnlen,
                  VITYP num);
int export_var(struct cenv * ce, struct str * vnam);
int export_var_value(struct cenv * ce,
                     const char * varnam,
                     const char * varval);
int init_required_variables(struct cenv * ce);
void num_to_var(struct var * tgtvar, VITYP num);
/***************************************************************/
