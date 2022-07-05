/***************************************************************/
/*
**  var.c  --  Routines for variables
*/
/***************************************************************/
/*
History:
03/18/12 DTE  0.0 First Version
*/

/***************************************************************/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "desh.h"
#include "var.h"
#include "cmd.h"
#include "exp.h"
#include "msg.h"
#include "when.h"
#include "assoc.h"
#include "snprfh.h"

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
void ** dbtree_find_hashed(struct dbtree * dbt,
                            int kh,
                            const void * keyval,
                            int keyvallen) {

    struct dbtree_node * dbn;

    if (!dbt->root) return NULL;

    dbn = dbtree_find_node(dbt->root, keyval, keyvallen, kh);

    if (dbn) return (&(dbn->pdata));

    return (NULL);
}
/***************************************************************/
int dbtree_gethash(const void * keyval, int keyvallen) {

    int kh;
    int ii;
    const unsigned char * kv = keyval;

    for( kh = 0, ii = 0; ii < keyvallen; ii++ ) kh = 131 * kh + kv[ii];

    return (kh);
}
/***************************************************************/
void ** dbtree_find_gethash(struct dbtree * dbt,
                            int * pkh,
                            const void * keyval,
                            int keyvallen) {

    int kh;
    int ii;
    struct dbtree_node * dbn;
    const unsigned char * kv = keyval;

    for( kh = 0, ii = 0; ii < keyvallen; ii++ ) kh = 131 * kh + kv[ii];
    (*pkh) = kh;

    if (!dbt->root) return NULL;

    dbn = dbtree_find_node(dbt->root, keyval, keyvallen, kh);

    if (dbn) return (&(dbn->pdata));

    return (NULL);
}
/***************************************************************/
void ** dbtree_find(struct dbtree * dbt, const void * keyval, int keyvallen) {

    int kh;
    int ii;
    struct dbtree_node * dbn;
    const unsigned char * kv = keyval;

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
        if (keyhash == dbn->keyhash && keyvallen == dbn->keyvallen &&
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
int dbtree_insert_hashed(struct dbtree * dbt,
                  int kh,
                  const void * keyval,
                  int keyvallen,
                  void * pdata) {

/* returns 0 - OK, 1 - duplicate key */

    int err = 0;
    struct dbtree_node * dbn;

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
int dbtree_insert(struct dbtree * dbt,
                  const void * keyval,
                  int keyvallen,
                  void * pdata) {

/* returns 0 - OK, 1 - duplicate key */

    int kh;
    int ii;
    int err;
    const unsigned char * kv = (const unsigned char *)keyval;

    for( kh = 0, ii = 0; ii < keyvallen; ii++ ) kh = 131 * kh + kv[ii];

    err = dbtree_insert_hashed(dbt, kh, keyval, keyvallen, pdata);

    return (err);
}
/***************************************************************/
static void dbtree_remove_node(struct dbtree * dbt,
                        struct dbtree_node * dbn,
                        struct dbtree_node ** parent) {

    Free(dbn->keyval);

    if (dbn->nnode) dbn->nnode->pnode = dbn->pnode;
    else            dbt->tail         = dbn->pnode;

    if (dbn->pnode) dbn->pnode->nnode = dbn->nnode;
    else            dbt->head         = dbn->nnode;

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
int dbtree_delete_hashed(struct dbtree * dbt, int kh, const void * keyval, int keyvallen) {

/* returns 0 - OK, 1 - no such key */

    int err = 0;

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
int dbtree_delete(struct dbtree * dbt, const void * keyval, int keyvallen) {

/* returns 0 - OK, 1 - no such key */

    int kh;
    int ii;
    int err = 0;
    const unsigned char * kv = keyval;

    for( kh = 0, ii = 0; ii < keyvallen; ii++ ) kh = 131 * kh + kv[ii];

    err = dbtree_delete_hashed(dbt, kh, keyval, keyvallen);

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
void ** dbtree_next_k(void * vdbtc, char ** keynam) {

    struct dbtree_cursor * dbtc = vdbtc;

    if (dbtc->stat && dbtc->wher) {
        dbtc->wher = dbtc->wher->nnode;
    }
    dbtc->stat = 1;
    if (!dbtc->wher) {
        (*keynam) = NULL;
        return (NULL);
    }
   (*keynam) = (char*)(dbtc->wher->keyval);

    return (&(dbtc->wher->pdata));
}
/***************************************************************/
int dbtree_cursor_delete(void * vdbtc) {

    struct dbtree_cursor * dbtc = vdbtc;
    int err;

    if (!dbtc->stat || !dbtc->wher) return (2); /* need dbtree_next first */

    err = dbtree_delete_hashed(dbtc->dbt, dbtc->wher->keyhash, 
                dbtc->wher->keyval, dbtc->wher->keyvallen);

    if (!err) {
        dbtc->stat = 0;
        dbtc->wher = dbtc->wher->nnode;
    }

    return (err);
}
/***************************************************************/
void dbtree_close(void * vdbtc) {

    struct dbtree_cursor * dbtc = vdbtc;

    if (!(dbtc->mode & DBTREE_READ_QUICK)) Free(dbtc);
}
/***************************************************************/
void ** dbtree_find_s(struct dbtree * dbt, const char * keyval) {

    return dbtree_find(dbt, keyval, (int)strlen(keyval) + 1);
}
/***************************************************************/
int dbtree_delete_s(struct dbtree * dbt, const char * keyval) {

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
            out = (int)dbt->nnodes;
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
void ** dbtree_direct(void * vdbtc, const void * keyval, int keyvallen) {

    struct dbtree_cursor * dbtc = (struct dbtree_cursor *)vdbtc;

    int kh;
    int ii;
    struct dbtree_node * dbn;
    const unsigned char * kv = (const unsigned char *)keyval;

    if (!dbtc->dbt->root) return NULL;

    for( kh = 0, ii = 0; ii < keyvallen; ii++ ) kh = 131 * kh + kv[ii];

    dbn = dbtree_find_node(dbtc->dbt->root, keyval, keyvallen, kh);

    dbtc->wher = dbn;
    dbtc->stat = 0;

    if (dbn) return (&(dbn->pdata));

    return (NULL);
}
/***************************************************************/
void set_dbtree_var(struct dbtree * dbt,
                  const char * vnam,
                  int vnlen,
                  intr vix) {

    char ** vhand;

    vhand = (char **)dbtree_find(dbt, vnam, vnlen);
    if (vhand) {
        memcpy((*vhand), &vix, sizeof(intr));
    } else {
        dbtree_insert(dbt, vnam, vnlen, (void*)vix);
    }
}
/***************************************************************/
struct dbtree * load_envp(char *envp[], char *** hlenvp, int * pnenv, int upsnams) {

    struct dbtree * dbt;
    int ii;
    int jj;
    char * eq;
    int vnamsz;
    int vnlen;
    int nenv;
    char vch;

    nenv = 0;
    while (envp[nenv]) nenv++;
    (*hlenvp) = New(char *, nenv + 1);
    (*pnenv) = nenv;

    dbt = dbtree_new();
    vnamsz = 0;

    for (ii = 0; ii < nenv; ii++) {
        (*hlenvp)[ii] = Strdup(envp[ii]);

        eq = strchr(envp[ii], '=');
        if (eq) {
            vnlen = (Int)(eq - envp[ii]);
            for(jj = 0; jj < vnlen; jj++) {
                vch = (*hlenvp)[ii][jj];
                if (!isalnum(vch) && vch != '_') vch = '_';
                if (upsnams) (*hlenvp)[ii][jj] = toupper(vch);
                else (*hlenvp)[ii][jj] = vch;
            } 
            set_dbtree_var(dbt, (*hlenvp)[ii], vnlen, ii);
        }
    }
   (*hlenvp)[nenv] = NULL;

    return (dbt);
}
/***************************************************************/
void free_vval(void * vval) {

    struct var * vv = (struct var *)vval;

    if (vv->var_typ == VTYP_STR) Free(vv->var_buf); 

    Free(vval);
}
/***************************************************************/
void unload_vars(struct dbtree * dbt) {

    dbtree_free(dbt, free_vval);
}
/***************************************************************/
struct var * new_str_var(const char * vval, int vvlen) {

    struct var * nvval;

    nvval = New(struct var, 1);
    str_to_var(nvval, vval, vvlen);

    return (nvval);
}
/***************************************************************/
void setvar_str(struct cenv * ce,
                char * vnam,
                int vnlen,
                char * vval) {
 
    struct var * nvval;

    nvval = new_str_var(vval, Istrlen(vval));

    if (!ce->ce_vars) ce->ce_vars = dbtree_new();
    dbtree_insert(ce->ce_vars, vnam, vnlen, nvval);
}
/***************************************************************/
void setvar_var(struct cenv * ce,
                char * vnam,
                int vnlen,
                struct var * vval) {
 
    struct var * nvval;

    nvval = New(struct var, 1);
    qcopy_var(nvval, vval);

    if (!ce->ce_vars) {
        ce->ce_vars = dbtree_new();
        ce->ce_close_flags |= FREECE_VARS;
    }

    dbtree_insert(ce->ce_vars, vnam, vnlen, nvval);
}
/***************************************************************/
#ifdef UNUSED
void copy_var_str(struct var * tgtvar, const char * vval, int vvlen) {

    if (tgtvar->var_typ ==  VTYP_STR) {
        if (tgtvar->var_max <= vvlen) {
            tgtvar->var_max = vvlen + 1;
            tgtvar->var_buf = Realloc(tgtvar->var_buf, char, tgtvar->var_max);
        }
        memcpy(tgtvar->var_buf, vval, vvlen);
        tgtvar->var_buf[vvlen] = '\0';
        tgtvar->var_len = vvlen;
    } else {
        str_to_var(tgtvar, vval, vvlen);
    }
}
#endif
/***************************************************************/
void set_ce_var(struct cenv * ce,
                  const char * vnam,
                  int vnlen,
                  struct var * vval) {

    struct var ** vhand;
    int hval;
    int * pix;

    if (ce->ce_vars) {
        vhand = (struct var **)dbtree_find_gethash(ce->ce_vars, &hval, vnam, vnlen);
    } else {
        hval= dbtree_gethash(vnam, vnlen);
        vhand = NULL;
    }

    if (vhand) {
        replace_var((*vhand), vval);
    } else {
         if (ce->ce_file_ce && ce->ce_file_ce->ce_vars) {
            vhand = (struct var **)dbtree_find_hashed(ce->ce_file_ce->ce_vars, hval, vnam, vnlen);
        }
        if (vhand) {
            replace_var((*vhand), vval);
        } else {
            pix = (int *)dbtree_find_hashed(ce->ce_gg->gg_genv, hval, vnam, vnlen);
            if (pix) {
                vensure_str(ce, vval);

                set_globalvar_ix(ce->ce_gg, *pix, vnam, vnlen, 
                        vval->var_buf, vval->var_len);
                free_var_buf(vval);
            } else {
                if (!ce->ce_vars) ce->ce_vars = dbtree_new();
                dbtree_insert_hashed(ce->ce_vars, hval, vnam, vnlen, (void*)dup_var(vval));
            }
        }
    }
}
/***************************************************************/
void num_to_var(struct var * tgtvar, VITYP num) {

    tgtvar->var_typ = VTYP_INT;
    tgtvar->var_val = num;
}
/***************************************************************/
void set_ce_var_num(struct cenv * ce,
                  const char * vnam,
                  int vnlen,
                  VITYP num) {

    struct var nvval;

    nvval.var_typ = VTYP_INT;
    nvval.var_val = num;
    set_ce_var(ce, vnam, vnlen, &nvval);
}
/***************************************************************/
void set_ce_var_str(struct cenv * ce,
                  const char * vnam,
                  int vnlen,
                  const char * vval,
                  int vvlen) {  /* vvlen should not include null terminator */

    struct var nvval;

    str_to_var(&nvval, vval, vvlen);
    set_ce_var(ce, vnam, vnlen, &nvval);
    free_var_buf(&nvval);
}
/***************************************************************/
int export_ce_var(struct cenv * ce,
                struct str * vnam,
                int hval,
                struct var * vval,
                struct dbtree * dbtvars) {

    int estat;
    intr gix;
    int * pix;

    estat = 0;

    vensure_str(ce, vval);
    pix = (int *)dbtree_find_hashed(ce->ce_gg->gg_genv, hval, vnam->str_buf, vnam->str_len);
    if (pix) {
        gix = (*pix);
    } else {
        gix = ce->ce_gg->gg_nenv;
        ce->ce_gg->gg_envp = Realloc(ce->ce_gg->gg_envp, char*, gix + 2);
        ce->ce_gg->gg_nenv += 1;
        ce->ce_gg->gg_envp[ce->ce_gg->gg_nenv] = NULL;
        dbtree_insert_hashed(ce->ce_gg->gg_genv, hval, vnam->str_buf, vnam->str_len, (void*)gix);
    }
    set_globalvar_ix(ce->ce_gg, (Int)gix, vnam->str_buf, vnam->str_len, vval->var_buf, vval->var_len);
    dbtree_delete_hashed(dbtvars, hval, vnam->str_buf, vnam->str_len);
    free_var(vval);

    return (estat);
}
/***************************************************************/
int export_var(struct cenv * ce, struct str * vnam) {

    int estat;
    struct var ** vhand;
    int hval;
    intr * pix;

    estat = 0;

    if (ce->ce_vars) {
        vhand = (struct var **)dbtree_find_gethash(ce->ce_vars, &hval, vnam->str_buf, vnam->str_len);
    } else {
        hval= dbtree_gethash(vnam->str_buf, vnam->str_len);
        vhand = NULL;
    }

    if (vhand) {
        estat = export_ce_var(ce, vnam, hval, (*vhand), ce->ce_vars);
    } else {
         if (ce->ce_file_ce && ce->ce_file_ce->ce_vars) {
            vhand = (struct var **)dbtree_find_hashed(ce->ce_file_ce->ce_vars, hval, vnam->str_buf, vnam->str_len);
        }
        if (vhand) {
            estat = export_ce_var(ce, vnam, hval, (*vhand), ce->ce_file_ce->ce_vars);
        } else {
            pix = (intr *)dbtree_find_hashed(ce->ce_gg->gg_genv, hval, vnam->str_buf, vnam->str_len);
            if (!pix) {
                estat = set_error(ce, ENOVAR, vnam->str_buf);
           }
        }
    }

    return (estat);
}
/***************************************************************/
int export_var_value(struct cenv * ce,
                     const char * varnam,
                     const char * varval) {

    int estat;
    int * pix;
    intr gix;
    int hval;
    int vnamlen;
    int vvallen;

    estat = 0;

    vnamlen = Istrlen(varnam);
    vvallen = Istrlen(varval);

    pix = (int *)dbtree_find_gethash(ce->ce_gg->gg_genv, &hval, varnam, vnamlen);
    if (pix) {
        gix = (*pix);
    } else {
        gix = ce->ce_gg->gg_nenv;
        ce->ce_gg->gg_envp = Realloc(ce->ce_gg->gg_envp, char*, gix + 2);
        ce->ce_gg->gg_nenv += 1;
        ce->ce_gg->gg_envp[ce->ce_gg->gg_nenv] = NULL;
        dbtree_insert_hashed(ce->ce_gg->gg_genv, hval, varnam, vnamlen, (void*)gix);
    }
    set_globalvar_ix(ce->ce_gg, (Int)gix, varnam, vnamlen, varval, vvallen);

    return (estat);
}
/***************************************************************/
int get_user_name(struct cenv * ce, char * vbuf, int vbufmax) {

    int estat;
#ifdef IS_WINDOWS
    char * user_domain;
    char * user_name;
#endif

    estat = 0;
    vbuf[0] = '\0';

#ifdef IS_WINDOWS
    user_name   = GETENV("USERNAME");
    if (user_name) {
        user_domain = GETENV("USERDOMAIN");
        if (user_domain) {
            Snprintf(vbuf, vbufmax, "%s\\%s", user_domain, user_name);
        } else {
            strmcpy(vbuf, user_name, vbufmax);
        }
    }

#endif

    return (estat);
}
/***************************************************************/
int get_deshprompt1(struct cenv * ce, char * vbuf, int vbufmax) {

    int estat;

    estat = 0;
    strmcpy(vbuf, "$ ", vbufmax);

    return (estat);
}
/***************************************************************/
int get_deshprompt2(struct cenv * ce, char * vbuf, int vbufmax) {

    int estat;

    estat = 0;
    strmcpy(vbuf, "> ", vbufmax);

    return (estat);
}
/***************************************************************/
int set_default_var_val(struct cenv * ce,
                    const char * vnam,
                    int (*vvproc)(struct cenv * ce, char * vbuf, int vbufmax)) {

    int estat;
    struct var ** vhand;
    int hval;
    int vnlen;
    struct var * nvval;
    char * gvval;
    char vvalbuf[PATH_MAXIMUM + 8];

    estat = 0;

    vnlen = Istrlen(vnam);

    gvval = find_globalvar(ce->ce_gg, vnam, vnlen);
    if (!gvval) {
        vhand = (struct var **)dbtree_find_gethash(ce->ce_vars, &hval, vnam, vnlen);
        if (!vhand) {
            estat = vvproc(ce, vvalbuf, sizeof(vvalbuf));
            if (!estat && vvalbuf[0]) {
                nvval = new_str_var(vvalbuf, Istrlen(vvalbuf));
                dbtree_insert_hashed(ce->ce_vars, hval, vnam, vnlen, (void*)nvval);
            }
        }
    }

    return (estat);
}
/***************************************************************/
int init_required_variables(struct cenv * ce) {

    int estat;

    if (!ce->ce_vars) ce->ce_vars = dbtree_new();

    estat = set_default_var_val(ce, "HOME", get_home_directory);
    if (!estat) estat = set_default_var_val(ce, "HOST", get_host_name);
    if (!estat) estat = set_default_var_val(ce, "USER", get_user_name);

    if (!estat) estat = set_default_var_val(ce, DESHPROMPT1_NAME, get_deshprompt1);
    if (!estat) estat = set_default_var_val(ce, DESHPROMPT2_NAME, get_deshprompt2);

    return (estat);
}
/***************************************************************/
