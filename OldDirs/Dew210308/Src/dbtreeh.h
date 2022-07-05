/*
**  dbtreeh.h  --  dbtree functions
*/
/***************************************************************/
#ifndef DBTREEH_INCLUDED
#define DBTREEH_INCLUDED
/***************************************************************/
/* dbtree functions                                            */
/***************************************************************/

struct dbtree * dbtree_new(void);
int dbtree_insert(struct dbtree * dbt,
                  const void * keyval,
                  int keyvallen,
                  void * pdata);
int dbtree_delete(struct dbtree * dbt, const void * keyval, int keyvallen);
void dbtree_free(struct dbtree * dbt,
                     void (*free_proc)(void*));
void ** dbtree_find(struct dbtree * dbt, const void * keyval, int keyvallen);
int dbtree_stats(struct dbtree * dbt, int stattyp);
void dbtree_display(struct dbtree * dbt);
void * dbtree_rewind(struct dbtree * dbt, int mode);
void ** dbtree_next(void * vdbtc);
void dbtree_close(void * vdbtc);
void * dbtree_get_key(void * vdbtc, int * key_len); /* Added 11/6/2014 */
int dbtree_scan(struct dbtree * dbt,
                    void* px,
                    int (*scan_proc)(void*, void*, void*)); /* Added 02/22/2021 */

#define DBTREE_READ_FORWARD     0
#define DBTREE_READ_BACKWARD    1
#define DBTREE_READ_TREE        2
#define DBTREE_READ_QUICK       8

/***************************************************************/
/* dbtsi functions                                             */
/***************************************************************/

#define dbtsi_new   (struct dbtsi*)dbtree_new
#define dbtsi_free(dbtsi) dbtree_free((struct dbtree*)(dbtsi), NULL);
#define dbtsi_insert(dbtsi,keyval,intval) dbtree_insert((struct dbtree*)(dbtsi),(keyval),(int)strlen(keyval)+1,(void*)((intr)intval))
#define dbtsi_find(dbtsi,keyval) (int*)dbtree_find((struct dbtree*)(dbtsi),(keyval),(int)strlen(keyval)+1)
#define dbtsi_scan(dbtsi,px,sp) dbtree_scan((struct dbtree*)(dbtsi),(px),(sp))

#ifdef __mpexl
#define WCSLEN(x) wcslenex(x)
#else
#define WCSLEN(x) wcslen(x)
#endif

/***************************************************************/
#endif /* DBTREEH_INCLUDED */
/***************************************************************/
