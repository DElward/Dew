/***************************************************************/
/*
**  dbtree.c  --  Routines for quick binary trees
*/
/***************************************************************/
/*
History:
04/28/04 DTE  0.0 First Version
*/
/***************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "cmemh.h"
#include "dbtreeh.h"

#ifndef New
    #define New(t,n)         ((t*)calloc(sizeof(t),(n)))
    #define Realloc(p,t,n)   ((t*)realloc((p),sizeof(t)*(n)))
    #define Free(p)           free(p)
    #define Strdup(s)        (strcpy(New(char,strlen(s)+1),(s)))
#endif

struct dbtree_node {
    void * keyval;
    int    keyvallen;
    int    keyhash;
    void * pdata;
    struct dbtree_node * lnode; /* left node */
    struct dbtree_node * rnode; /* right node */
    struct dbtree_node * pnode; /* prev node */
    struct dbtree_node * nnode; /* next node */
};
struct dbtree {
    struct dbtree_node * root;
    struct dbtree_node * head;
    struct dbtree_node * tail;
    long                 nnodes;
};
struct dbtree_cursor {
    struct dbtree_node * wher;
    int                  mode;
    int                  stat;
    struct dbtree      * dbt;
};
/***************************************************************/
struct dbtree * dbtree_new(void) {

    struct dbtree * dbt;

    dbt = New(struct dbtree, 1);
    dbt->root   = NULL;
    dbt->head   = NULL;
    dbt->tail   = NULL;
    dbt->nnodes = 0;

    return (dbt);
}
/***************************************************************/
static void dbtree_free_node(struct dbtree_node * dbn,
                             void (*free_proc)(void*)) {

#if 0
if (!memcmp(dbn->keyval, "t101_18", 7)) {
    int break_here = 1;
}
#endif

    Free(dbn->keyval);
    if (free_proc) free_proc(dbn->pdata);

    if (dbn->lnode) dbtree_free_node(dbn->lnode, free_proc);
    if (dbn->rnode) dbtree_free_node(dbn->rnode, free_proc);

    Free(dbn);
}
/***************************************************************/
void dbtree_free(struct dbtree * dbt,
                     void (*free_proc)(void*)) {

    if (dbt->root) dbtree_free_node(dbt->root, free_proc);

    Free(dbt);
}
/***************************************************************/
static struct dbtree_node * dbtree_find_node(struct dbtree_node * dbn,
                        const void * keyval,
                        int    keyvallen,
                        int keyhash) {

    if (keyhash < dbn->keyhash) {
        if (dbn->lnode) {
            return dbtree_find_node(dbn->lnode, keyval, keyvallen, keyhash);
        } else {
            return NULL;
        }
    } else {
        if (keyhash == dbn->keyhash && keyvallen == dbn->keyvallen &&
            !memcmp(dbn->keyval, keyval, keyvallen)) {
            return (dbn);
        } else {
            if (dbn->rnode) {
                return dbtree_find_node(dbn->rnode, keyval, keyvallen, keyhash);
            } else {
                return NULL;
            }
        }
    }
}
/***************************************************************/
void ** dbtree_find(struct dbtree * dbt, const void * keyval, int keyvallen) {

    int kh;
    int ii;
    struct dbtree_node * dbn;
    unsigned const char * kv = keyval;

    if (!dbt->root) return NULL;

    for( kh = 0, ii = 0; ii < keyvallen; ii++ ) kh = 131 * kh + kv[ii];

    dbn = dbtree_find_node(dbt->root, keyval, keyvallen, kh);

    if (dbn) return (&(dbn->pdata));

    return (NULL);
}
/***************************************************************/
struct dbtree_node * dbtree_new_node(const void * keyval,
                                     int    keyvallen,
                                     int keyhash,
                                     void * pdata) {

    struct dbtree_node * dbn;

    dbn = New(struct dbtree_node, 1);
    dbn->keyval     = New(char, keyvallen);
    memcpy(dbn->keyval, keyval, keyvallen);
    dbn->keyvallen  = keyvallen;
    dbn->keyhash    = keyhash;
    dbn->pdata      = pdata;
    dbn->lnode      = NULL;
    dbn->rnode      = NULL;
    dbn->pnode      = NULL;
    dbn->nnode      = NULL;

    return (dbn);
}
/***************************************************************/
static struct dbtree_node * dbtree_insert_node(struct dbtree_node * dbn,
                        const void * keyval,
                        int keyvallen,
                        int keyhash,
                        void * pdata) {

    struct dbtree_node * ndbn;

    if (keyhash < dbn->keyhash) {
        if (dbn->lnode) {
            ndbn = dbtree_insert_node(dbn->lnode, keyval, keyvallen,
                                        keyhash, pdata);
        } else {
            ndbn = dbtree_new_node(keyval, keyvallen,
                                        keyhash, pdata);
            dbn->lnode = ndbn;
        }
    } else {
        if (keyhash == dbn->keyhash &&
            !memcmp(dbn->keyval, keyval, dbn->keyvallen)) {
            ndbn = 0;  /* duplicate */
        } else {
            if (dbn->rnode) {
                ndbn = dbtree_insert_node(dbn->rnode, keyval, keyvallen,
                                        keyhash, pdata);
            } else {
                ndbn = dbtree_new_node(keyval, keyvallen,
                                        keyhash, pdata);
                dbn->rnode = ndbn;
            }
        }
    }

    return (ndbn);
}
/***************************************************************/
int dbtree_insert(struct dbtree * dbt,
                  const void * keyval,
                  int keyvallen,
                  void * pdata) {

/* returns 0 - OK, 1 - duplicate key */

    int kh;
    int ii;
    int err = 0;
    unsigned const char * kv = keyval;
    struct dbtree_node * dbn;

    for( kh = 0, ii = 0; ii < keyvallen; ii++ ) kh = 131 * kh + kv[ii];

    if (!dbt->root) {
        dbn = dbtree_new_node(keyval, keyvallen, kh, pdata);
        dbt->root = dbn;
        dbt->head = dbn;
        dbt->tail = dbn;
        dbt->nnodes = 1;
    } else {
        dbn = dbtree_insert_node(dbt->root, keyval, keyvallen, kh, pdata);
        if (!dbn) {
            err = 1;
        } else {
            dbt->tail->nnode = dbn;
            dbn->pnode       = dbt->tail;
            dbt->tail        = dbn;
            dbt->nnodes++;
        }
    }

    return (err);
}
/***************************************************************/
static void dbtree_remove_node(struct dbtree * dbt,
                        struct dbtree_node * dbn,
                        struct dbtree_node ** parent) {

/*
** 11/17/2017: This needed to be fixed for the case when:
**      dbt->root == dbn
**      dbn->lnode is not NULL
**      dbn->rnode is not NULL
**      dbn->lnode->lnode is NULL
**      dbn->lnode->rnode is NULL
**
    From: DBTree.h
        template<class P_DATATYPE>
        void CDBTree<P_DATATYPE>::DbTreeRemoveNode(
                CDBTreeNode<P_DATATYPE> * dbn,
                CDBTreeNode<P_DATATYPE> ** parent) {

        CDBTreeNode<P_DATATYPE> * tdbn;

        if (dbn->m_nnode) dbn->m_nnode->m_pnode = dbn->m_pnode;
        else              m_tail                = dbn->m_pnode;

        if (dbn->m_pnode) dbn->m_pnode->m_nnode = dbn->m_nnode;
        else              m_head                = dbn->m_nnode;

        if (dbn->m_lnode) {
            if (dbn->m_rnode &&
                dbn->m_rnode->m_rnode && dbn->m_rnode->m_lnode) {
                //
                // These lines are not necessary, but they should help
                // keep the tree more balanced.
                //
                (*parent) = dbn->m_rnode;
                tdbn = dbn->m_rnode;
                while (tdbn->m_lnode) tdbn = tdbn->m_lnode;
                tdbn->m_lnode = dbn->m_lnode;
            } else {
                (*parent) = dbn->m_lnode;
                tdbn = dbn->m_lnode;
                while (tdbn->m_rnode) tdbn = tdbn->m_rnode;
                tdbn->m_rnode = dbn->m_rnode;
            }
        } else if (dbn->m_rnode) {
            (*parent) = dbn->m_rnode;
        } else {
            (*parent) = NULL;
        }

        dbn->m_lnode = NULL;
        dbn->m_rnode = NULL;
        dbn->m_nnode = NULL;
        dbn->m_pnode = NULL;
*/
    struct dbtree_node * tdbn;

    Free(dbn->keyval);

    if (dbn->nnode) dbn->nnode->pnode = dbn->pnode;
    else            dbt->tail         = dbn->pnode;

    if (dbn->pnode) dbn->pnode->nnode = dbn->nnode;
    else            dbt->head         = dbn->nnode;

#ifdef OLD_WAY
    if (dbn->lnode) {
        if (dbn->rnode) {
            /* case 1: lnode: YES  rnode: YES */
            dbn->keyval     = dbn->lnode->keyval;
            dbn->keyvallen  = dbn->lnode->keyvallen;
            dbn->keyhash    = dbn->lnode->keyhash;
            dbn->lnode      = dbn->lnode->lnode;
            /* dbn->rnode stays */
        } else {
            /* case 2: lnode: YES  rnode: NO  */
            (*parent) = dbn->lnode;
            Free(dbn);
            /* read dbn->rnode */
        }
    } else if (dbn->rnode) {
            /* case 3: lnode: NO   rnode: YES */
            (*parent) = dbn->rnode;
            Free(dbn);
    } else {
            /* case 4: lnode: NO   rnode: NO  */
        (*parent) = NULL;
        Free(dbn);
    }
#else
    if (dbn->lnode) {
        if (dbn->rnode && dbn->rnode->rnode && dbn->rnode->lnode) {
            /*
            ** These lines are not necessary, but they should help
            ** keep the tree more balanced.
            */
            (*parent) = dbn->rnode;
            tdbn = dbn->rnode;
            while (tdbn->lnode) tdbn = tdbn->lnode;
            tdbn->lnode = dbn->lnode;
        } else {
            (*parent) = dbn->lnode;
            tdbn = dbn->lnode;
            while (tdbn->rnode) tdbn = tdbn->rnode;
            tdbn->rnode = dbn->rnode;
        }
    } else if (dbn->rnode) {
        (*parent) = dbn->rnode;
    } else {
        (*parent) = NULL;
    }
    Free(dbn);
#endif
}
/***************************************************************/
static int dbtree_delete_node(struct dbtree * dbt,
                        struct dbtree_node * dbn,
                        const void * keyval,
                        int    keyvallen,
                        int keyhash,
                        struct dbtree_node ** parent) {

/* returns 0 - OK, 1 - no such key */

    int err = 0;

    if (keyhash < dbn->keyhash) {
        if (dbn->lnode) {
            err = dbtree_delete_node(dbt, dbn->lnode,
                    keyval, keyvallen, keyhash, &(dbn->lnode));
        } else {
            err = 1;  /* no such key */
        }
    } else {
        if (keyhash == dbn->keyhash && keyvallen == dbn->keyvallen &&
            !memcmp(dbn->keyval, keyval, keyvallen)) {
            dbtree_remove_node(dbt, dbn, parent);
        } else {
            if (dbn->rnode) {
                err = dbtree_delete_node(dbt, dbn->rnode,
                        keyval, keyvallen, keyhash, &(dbn->rnode));
            } else {
                err = 1;  /* no such key */
            }
        }
    }

    return (err);
}
/***************************************************************/
int dbtree_delete(struct dbtree * dbt, const void * keyval, int keyvallen) {

/* returns 0 - OK, 1 - no such key */

    int kh;
    int ii;
    int err = 0;
    unsigned const char * kv = keyval;

    for( kh = 0, ii = 0; ii < keyvallen; ii++ ) kh = 131 * kh + kv[ii];

    if (!dbt->root) {
        err = 1;  /* no such key */
    } else {
        err = dbtree_delete_node(dbt, dbt->root,
                    keyval, keyvallen, kh, &(dbt->root));
        if (!err) dbt->nnodes--;
    }

    return (err);
}
/***************************************************************/
void * dbtree_rewind(struct dbtree * dbt, int mode) {
/*
** Mode:
**   0 - Default mode (reentrant)
**   1 - Backwards (unimplemented)
**   2 - Tree order (unimplemented)
**   8 - Quick mode (non-reentrant)
*/
    struct dbtree_cursor * dbtc;
    static struct dbtree_cursor s_dbtc;

    if (!(mode & DBTREE_READ_QUICK)) {
        dbtc = New(struct dbtree_cursor, 1);
    } else {
        dbtc = &s_dbtc;
    }

    dbtc->stat = 0;
    dbtc->mode = mode;
    dbtc->dbt  = dbt;
    dbtc->wher = dbt->head;

    return (dbtc);
}
/***************************************************************/
void ** dbtree_next(void * vdbtc) {

    struct dbtree_cursor * dbtc = vdbtc;

    if (dbtc->stat && dbtc->wher) {
        dbtc->wher = dbtc->wher->nnode;
    }
    dbtc->stat = 1;
    if (!dbtc->wher) return (NULL);

    return (&(dbtc->wher->pdata));
}
/***************************************************************/
void * dbtree_get_key(void * vdbtc, int * key_len) {

    struct dbtree_cursor * dbtc = vdbtc;

    if (!dbtc->wher) {
        (*key_len) = 0;
        return (NULL);
    }
    (*key_len) = dbtc->wher->keyvallen;

    return (dbtc->wher->keyval);
}
/***************************************************************/
void dbtree_close(void * vdbtc) {

    struct dbtree_cursor * dbtc = vdbtc;

    if (!(dbtc->mode & DBTREE_READ_QUICK)) Free(dbtc);
}
/***************************************************************/
void ** dbtree_find_s(struct dbtree * dbt, char * keyval) {

    return dbtree_find(dbt, keyval, (int)strlen(keyval) + 1);
}
/***************************************************************/
int dbtree_delete_s(struct dbtree * dbt, char * keyval) {

    return dbtree_delete(dbt, keyval, (int)strlen(keyval) + 1);
}
/***************************************************************/
int dbtree_insert_s(struct dbtree * dbt,
                  char * keyval,
                  void * pdata) {

    return dbtree_insert(dbt, keyval, (int)strlen(keyval) + 1, pdata);
}
/***************************************************************/
void dbtree_display_node(struct dbtree_node * dbn, int depth) {

    if (dbn->lnode) {
        dbtree_display_node(dbn->lnode, depth + 1);
    }

    printf("%*.*s%s\n", depth * 2, depth * 2, "", (char*)dbn->keyval);

    if (dbn->rnode) {
        dbtree_display_node(dbn->rnode, depth + 1);
    }
}
/***************************************************************/
void dbtree_display(struct dbtree * dbt) {

    dbtree_display_node(dbt->root, 0);
}
/***************************************************************/
static void dbtree_stats_node(struct dbtree_node * dbn,
                  int * maxdepth,
                  int   depth) {

    depth++;

    if (depth > *maxdepth) *maxdepth = depth;

    if (dbn->lnode) dbtree_stats_node(dbn->lnode, maxdepth, depth);
    if (dbn->rnode) dbtree_stats_node(dbn->rnode, maxdepth, depth);
}
/***************************************************************/
int dbtree_stats(struct dbtree * dbt, int stattyp) {
/*
** stattyp:
**   0 - Number of nodes
**   1 - Maximum tree depth
*/
    int out;

    out = 0;

    switch (stattyp) {
        case 0:
            out = dbt->nnodes;
            break;
        case 1:
            if (dbt->root) dbtree_stats_node(dbt->root, &out, 0);
            break;
        default:
            break;
    }

    return (out);
}
/***************************************************************/
static int dbtree_scan_node(struct dbtree_node * dbn,
                             void* px,
                             int (*scan_proc)(void*, void*, void*))
{
/*
** 02/22/2021
*/
    int done = 0;

    if (scan_proc) done = scan_proc(px, dbn->keyval, dbn->pdata);

    if (!done) if (dbn->lnode) done = dbtree_scan_node(dbn->lnode, px, scan_proc);
    if (!done) if (dbn->rnode) done = dbtree_scan_node(dbn->rnode, px, scan_proc);

    return (done);
}
/***************************************************************/
int dbtree_scan(struct dbtree * dbt,
                    void* px,
                    int (*scan_proc)(void*, void*, void*))
{
/*
** 02/22/2021
*/
    int done = 0;

    if (dbt->root) done = dbtree_scan_node(dbt->root, px, scan_proc);

    return (done);
}
/***************************************************************/
