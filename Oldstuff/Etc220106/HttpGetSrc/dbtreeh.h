/*
**  DBTREEH  --  dbtree functions
*/
/***************************************************************/
#ifndef DBTREEH_INCLUDED
#define DBTREEH_INCLUDED
/***************************************************************/
/* dbtree functions                                            */
/***************************************************************/

struct dbtree * dbtree_new(void);
int dbtree_insert(struct dbtree * dbt,
                  void * keyval,
                  int keyvallen,
                  void * pdata);
int dbtree_delete(struct dbtree * dbt, void * keyval, int keyvallen);
void dbtree_free(struct dbtree * dbt,
                     void (*free_proc)(void*));
void ** dbtree_find(struct dbtree * dbt, void * keyval, int keyvallen);
int dbtree_stats(struct dbtree * dbt, int stattyp);
void dbtree_display(struct dbtree * dbt);
void * dbtree_rewind(struct dbtree * dbt, int mode);
void ** dbtree_next(void * vdbtc);
void dbtree_close(void * vdbtc);

#define DBTREE_READ_FORWARD     0
#define DBTREE_READ_BACKWARD    1
#define DBTREE_READ_TREE        2
#define DBTREE_READ_QUICK       8

/***************************************************************/

#ifdef __mpexl
#define WCSLEN(x) wcslenex(x)
#else
#define WCSLEN(x) wcslen(x)
#endif

/***************************************************************/
#endif /* DBTREEH_INCLUDED */
/***************************************************************/
