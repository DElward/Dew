/***************************************************************/
/* intclas.c                                                   */
/***************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>
/***************************************************************/
#include "config.h"
#include "omar.h"
#include "dbtreeh.h"
#include "snprfh.h"
#include "json.h"
#include "onn.h"
#include "xeq.h"
#include "bid.h"
#include "handrec.h"
#include "crehands.h"
#include "intclas.h"
#include "util.h"
#include "thread.h"
#include "natural.h"
#include "pbn.h"

struct handreclist { /* hrl_ */
    int                 hrl_estat;
    int                 hrl_nhandrec;
    struct handrec **   hrl_ahandrec;
    struct playstats    hrl_pst;
};

/***************************************************************/
static int svare_func_defined(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)
{
    int svstat = 0;
    struct svarvalue * vsvv;

    if (nsvv != 1) {
        svstat = svare_set_error(svx, SVARERR_INVALID_NUM_PARMS,
            "Expecting 1 argument to %s. Found %d", func_name, nsvv);
    } else if (asvv[0]->svv_dtype != SVV_DTYPE_CHARS) {
        svstat = svare_set_error(svx, SVARERR_INVALID_PARM_TYPE,
            "Expecting string argument to %s.", func_name);
    } else {
        vsvv = svare_find_variable(svx, NULL,
            asvv[0]->svv_val.svv_chars.svvc_val_chars);
        if (vsvv) {
            svare_svarvalue_bool(rsvv, 1);
        } else {
            svare_svarvalue_bool(rsvv, 0);
        }
    }

    return (svstat);
}
/***************************************************************/
#define SVAR_CLASS_ID_XMLTREE           16385
#define SVAR_CLASS_ID_JSONTREE          16386


struct internal_class_def {   /* icd_ */
    char *          icd_clasnam;
    int             icd_class_id;
} intclas_list[] = {
    { SVAR_CLASS_NAME_ANALYSIS          , SVAR_CLASS_ID_ANALYSIS            }
  , { SVAR_CLASS_NAME_AUCTION           , SVAR_CLASS_ID_AUCTION             }
  , { SVAR_CLASS_NAME_BIDDING           , SVAR_CLASS_ID_BIDDING             }
  , { SVAR_CLASS_NAME_CONVCARD          , SVAR_CLASS_ID_CONVCARD            }
  , { SVAR_CLASS_NAME_HAND              , SVAR_CLASS_ID_HAND                }
  , { SVAR_CLASS_NAME_HAND_RECORD_LIST  , SVAR_CLASS_ID_HAND_RECORD_LIST    }
  , { SVAR_CLASS_NAME_HANDS             , SVAR_CLASS_ID_HANDS               }
  , { SVAR_CLASS_NAME_INTEGER           , SVAR_CLASS_ID_INTEGER             }
  , { SVAR_CLASS_NAME_NEURAL_NET        , SVAR_CLASS_ID_NEURAL_NET          }
  , { SVAR_CLASS_NAME_PBN               , SVAR_CLASS_ID_PBN                 }
  , { SVAR_CLASS_NAME_TESTING           , SVAR_CLASS_ID_TESTING             }
  , { SVAR_CLASS_NAME_THREAD            , SVAR_CLASS_ID_THREAD              }
  , { SVAR_CLASS_NAME_THREAD_HANDLE     , SVAR_CLASS_ID_THREAD_HANDLE       }
  , { SVAR_CLASS_NAME_WIRE              , SVAR_CLASS_ID_WIRE                }
  , { NULL      , 0     } };

static int svare_count_internal_classes(void)
{
/*
struct internal_class_def {
    char *          icd_clasnam;
    int             icd_num;
} intclas_list[] = {
*/
    int tcount = 0;

    while (intclas_list[tcount].icd_clasnam) {
#if IS_DEBUG
        if (tcount) {
            if (strcmp(intclas_list[tcount - 1].icd_clasnam,
                       intclas_list[tcount].icd_clasnam) > 0) {
                printf(
        "**** count_html_entities(): entities not in order at %d: %s\n",
                    tcount, intclas_list[tcount].icd_clasnam);
            }
        }
#endif
        tcount++;
    }

    return (tcount);
}
/***************************************************************/
static int svare_find_internal_class_ix(struct svarexec * svx,
    const char * classnam)
{
    static int num_classes = 0;
    int lo;
    int hi;
    int mid;

    if (!num_classes) {
        num_classes = svare_count_internal_classes();
    }

    lo = 0;
    hi = num_classes - 1;
    while (lo <= hi) {
        mid = (lo + hi) / 2;
        if (strcmp(classnam, intclas_list[mid].icd_clasnam) <= 0)
                                             hi = mid - 1;
        else                                 lo = mid + 1;
    }

    if (lo == num_classes ||
        strcmp(classnam, intclas_list[lo].icd_clasnam)) lo = -1;

    return (lo);
}
/***************************************************************/
static int svare_find_internal_class_name(struct svarexec * svx,
    const char * classnam)
{
    int iix;
    int cid;

    iix = svare_find_internal_class_ix(svx, classnam);

    cid = 0;
    if (iix >= 0) cid = intclas_list[iix].icd_class_id;

    return (cid);
}
/***************************************************************/
int svare_check_single_argument(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    const char * argmask,
    int * aix,
    int * matches)
{
    int svstat = 0;
    int tix;
    int cid;
    int argnum;
    char tok[100];

    argnum = 0;
    (*matches) = 0;

    while (!svstat && argmask[(*aix)] && argmask[(*aix)] != '|' && argnum < nsvv) {
        argnum++;
        switch (argmask[(*aix)]) {
            case '{':
                if (asvv[argnum - 1]->svv_dtype != SVV_DTYPE_INT_OBJECT) {
                    svstat = svare_set_error(svx, SVARERR_INVALID_PARM_TYPE,
                            "Expecting object as argument #%d to %s.", 1, func_name);
                } else {
                    tix = 0;
                    (*aix)++;
                    while (argmask[(*aix)] && argmask[(*aix)] != '}' && tix + 1 < sizeof(tok)) {
                        tok[tix++] = argmask[(*aix)];
                        (*aix)++;
                    }
                    tok[tix] = '\0';
                    cid = svare_find_internal_class_name(svx, tok);
                    if (!cid) {
                        svstat = svare_set_error(svx, SVARERR_BAD_ARGMASK,
                            "Bad internal class for function %s argument %d. Found \"%s\"",
                            func_name, argnum, tok);
                    } else {
                        if (asvv[argnum - 1]->svv_val.svv_svvi.svvi_int_class->svnc_class_id != cid) {
                                svstat = svare_set_error(svx, SVARERR_INVALID_PARM_TYPE,
                                        "Expecting %s as argument #%d to %s.", "object", argnum, func_name);
                        }
                    }
                }
                break;
            case '<':
                if (asvv[argnum - 1]->svv_dtype != SVV_DTYPE_INT_CLASS) {
                    svstat = svare_set_error(svx, SVARERR_INVALID_PARM_TYPE,
                            "Expecting %s as argument #%d to %s.", "class", argnum, func_name);
                } else {
                    tix = 0;
                    (*aix)++;
                    while (argmask[(*aix)] && argmask[(*aix)] != '>' && tix + 1 < sizeof(tok)) {
                        tok[tix++] = argmask[(*aix)];
                        (*aix)++;
                    }
                    tok[tix] = '\0';
                    cid = svare_find_internal_class_name(svx, tok);
                    if (!cid) {
                        svstat = svare_set_error(svx, SVARERR_BAD_ARGMASK,
                            "Bad internal class for function %s argument %d. Found \"%s\"",
                            func_name, argnum, tok);
                    } else {
                        if (asvv[argnum - 1]->svv_val.svv_svnc->svnc_class_id != cid) {
                            svstat = svare_set_error(svx, SVARERR_INVALID_PARM_TYPE,
                                    "Expecting %s as argument #%d to %s.", "class", argnum, func_name);
                        }
                    }
                }
                break;
            case 'b':
                if (asvv[argnum - 1]->svv_dtype != SVV_DTYPE_BOOL) {
                    svstat = svare_set_error(svx, SVARERR_INVALID_PARM_TYPE,
                            "Expecting boolean as argument #%d to %s.", argnum, func_name);
                }
                break;
            case 'd':
                if (asvv[argnum - 1]->svv_dtype != SVV_DTYPE_DOUBLE) {
                    svstat = svare_set_error(svx, SVARERR_INVALID_PARM_TYPE,
                            "Expecting double as argument #%d to %s.", argnum, func_name);
                }
                break;
            case 'i':
                if (asvv[argnum - 1]->svv_dtype != SVV_DTYPE_INT) {
                    svstat = svare_set_error(svx, SVARERR_INVALID_PARM_TYPE,
                            "Expecting integer as argument #%d to %s.", argnum, func_name);
                }
                break;
            case 'p':
                if (asvv[argnum - 1]->svv_dtype != SVV_DTYPE_USER_FUNCTION) {
                    svstat = svare_set_error(svx, SVARERR_INVALID_PARM_TYPE,
                            "Expecting function as argument #%d to %s.", argnum, func_name);
                }
                break;
            case 's':
                if (asvv[argnum - 1]->svv_dtype != SVV_DTYPE_CHARS) {
                    svstat = svare_set_error(svx, SVARERR_INVALID_PARM_TYPE,
                            "Expecting string as argument #%d to %s.", argnum, func_name);
                }
                break;
            case '.':
                /* Similar to C ... */
                argnum = nsvv;
                break;
            default:
                svstat = svare_set_error(svx, SVARERR_BAD_ARGMASK,
                    "Bad argument mask for function %s argument %d. Found %c",
                    func_name, argnum, argmask[(*aix)]);
                break;
        }
        (*aix)++;
    }

    if (!svstat && nsvv == argnum && (!argmask[(*aix)] || argmask[(*aix)] == '|')) {
        (*matches) = 1;
    }

    return (svstat);
}
/***************************************************************/
int svare_check_arguments(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    const char * argmask)
{
    int svstat = 0;
    int aix;
    int matches;

    aix = 0;
    matches = 0;

    while (!svstat && !matches && argmask[aix]) {
        svstat = svare_check_single_argument(svx, func_name, nsvv, asvv, argmask,
            &aix, &matches);
        if (!svstat && !matches && argmask[aix]) {
            while (argmask[aix] && argmask[aix] != '|') aix++;
            if (argmask[aix] == '|') aix++;
        }
    }

    if (!svstat && !matches) {
        svstat = svare_set_error(svx, SVARERR_BAD_ARGS,
            "Invalid argument list for function %s", func_name);
    }

    return (svstat);
}
/***************************************************************/
/**  NN -  struct neural_network                               */
/***************************************************************/
static int svare_method_NN_new(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)
{
    int svstat = 0;
    struct neural_network *  n_nn;

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "<" SVAR_CLASS_NAME_NEURAL_NET ">iii");

    if (!svstat) {
        n_nn = nn_new_default(
            asvv[1]->svv_val.svv_val_int,
            asvv[2]->svv_val.svv_val_int,
            asvv[3]->svv_val.svv_val_int);
        svare_store_int_object(svx, asvv[0]->svv_val.svv_svnc, rsvv, n_nn);
    }

    return (svstat);
}
/***************************************************************/
static int svare_method_NN_delete(struct svarexec * svx,   /* NULL for destructor */
    const char * func_name,     /* NULL for destructor */
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)    /* NULL for destructor */
{
    int svstat = 0;
    struct neural_network *  nn;

    nn = (struct neural_network *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
    nn_free(nn);

    return (svstat);
}
/***************************************************************/
static int svare_method_NN_Config(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)
{
    int svstat = 0;
    struct neural_network *  nn;
    char * cfg;
    double dblval;
    int rtn;

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "{" SVAR_CLASS_NAME_NEURAL_NET "}sd");

    if (!svstat) {
        nn = (struct neural_network *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        cfg = asvv[1]->svv_val.svv_chars.svvc_val_chars;
        rtn = 0;

        if (!Stricmp(cfg, "HiddenLayerBias")) {
            if (nsvv != 3) {
                svstat = svare_set_error(svx, SVARERR_INVALID_NUM_PARMS,
                    "Expecting 2 arguments to %s(%s). Found %d",
                    func_name, cfg, nsvv);
            } else {
                svstat = svar_get_double_variable(svx, asvv[2], &dblval);
                if (!svstat) {
                    rtn = nn_set_config(nn, NNCFG_HIDDEN_LAYER_BIAS, &dblval);
                }
            }
        } else if (!Stricmp(cfg, "OutputLayerBias")) {
            if (nsvv != 3) {
                svstat = svare_set_error(svx, SVARERR_INVALID_NUM_PARMS,
                    "Expecting 2 arguments to %s(%s). Found %d",
                    func_name, cfg, nsvv);
            } else {
                svstat = svar_get_double_variable(svx, asvv[2], &dblval);
                if (!svstat) {
                    rtn = nn_set_config(nn, NNCFG_OUTPUT_LAYER_BIAS, &dblval);
                }
            }
        } else if (!Stricmp(cfg, "LearningRate")) {
            if (nsvv != 3) {
                svstat = svare_set_error(svx, SVARERR_INVALID_NUM_PARMS,
                    "Expecting 2 arguments to %s(%s). Found %d",
                    func_name, cfg, nsvv);
            } else {
                svstat = svar_get_double_variable(svx, asvv[2], &dblval);
                if (!svstat) {
                    rtn = nn_set_config(nn, NNCFG_LEARNING_RATE, &dblval);
                }
            }
        } else if (!Stricmp(cfg, "Momentum")) {
            if (nsvv != 3) {
                svstat = svare_set_error(svx, SVARERR_INVALID_NUM_PARMS,
                    "Expecting 2 arguments to %s(%s). Found %d",
                    func_name, cfg, nsvv);
            } else {
                svstat = svar_get_double_variable(svx, asvv[2], &dblval);
                if (!svstat) {
                    rtn = nn_set_config(nn, NNCFG_MOMENTUM, &dblval);
                }
            }
        } else {
            svstat = svare_set_error(svx, SVARERR_UNRECOGNIZED_ARG,
                    "Unrecognized %s argument: \"%s\"", func_name, cfg);
        }

        if (!svstat && rtn) {
            svstat = svare_set_error(svx, SVARERR_FUNCTION_FAILED,
                    "Function %s(%s) returned an error %d",
                    func_name, cfg, rtn);
        }
    }

    return (svstat);
}
/***************************************************************/
static int svare_method_NN_GetTotalError(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)
{
    int svstat = 0;
    struct neural_network *  nn;

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "{" SVAR_CLASS_NAME_NEURAL_NET "}");

    if (!svstat) {
        nn = (struct neural_network *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        svare_svarvalue_double(rsvv, nn->nn_total_error);
    }

    return (svstat);
}
/***************************************************************/
static int svare_method_NN_GetTotalEpochs(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)
{
    int svstat = 0;
    struct neural_network *  nn;

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "{" SVAR_CLASS_NAME_NEURAL_NET "}");

    if (!svstat) {
        nn = (struct neural_network *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        svare_svarvalue_int(rsvv, nn->nn_num_epochs);
    }

    return (svstat);
}
/***************************************************************/
static int svare_method_NN_ElapsedTime(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)
{
    int svstat = 0;
    struct neural_network *  nn;

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "{" SVAR_CLASS_NAME_NEURAL_NET "}");

    if (!svstat) {
        nn = (struct neural_network *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        if (nn->nn_elapsed_time) {
            svare_store_string(rsvv, nn->nn_elapsed_time, IStrlen(nn->nn_elapsed_time));
        } else {
            svare_store_string(rsvv, NULL, 0);
        }
    }

    return (svstat);
}
/***************************************************************/
static int svare_method_NN_Load(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)
{
    int svstat = 0;
    int estat = 0;
    struct neural_network *  n_nn;
    char * loadfame;
    struct jsontree * jt;
    struct omarglobals * og = (struct omarglobals *)(svx->svx_gdata);


    /*     estat = nnt_load(og, nnd, loadfame);  */

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "<" SVAR_CLASS_NAME_NEURAL_NET ">s");

    if (!svstat) {
        loadfame = asvv[1]->svv_val.svv_chars.svvc_val_chars;
        estat = get_nn_jsontree(og, loadfame, &jt);
        if (!estat) {
            estat = nnt_jsonobject_to_neural_network(og, jt->jt_jv->jv_val.jv_jo, &n_nn);
            free_jsontree(jt);
        }
        if (estat) {
            svstat = svare_set_error(svx, SVARERR_FUNCTION_FAILED,
                    "Function %s(%s) returned an error %d",
                    func_name, loadfame, estat);
        } else {
            svare_store_int_object(svx, asvv[0]->svv_val.svv_svnc, rsvv, n_nn);
        }
    }

    return (svstat);
}
/***************************************************************/
static int svare_method_NN_Save(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)
{
    int svstat = 0;
    int estat;
    struct neural_network *  nn;
    char * savfame;
    struct omarglobals * og = (struct omarglobals *)(svx->svx_gdata);

    /*     estat = nnt_save(og, nnd->nnd_nn, savfame);  */

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "{" SVAR_CLASS_NAME_NEURAL_NET "}s");

    if (!svstat) {
        nn = (struct neural_network *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        savfame = asvv[1]->svv_val.svv_chars.svvc_val_chars;
        estat = nnt_save(og, nn, savfame);
        if (estat) {
            svstat = svare_set_error(svx, SVARERR_FUNCTION_FAILED,
                    "Function %s(%s) returned an error %d",
                    func_name, savfame, estat);
        }
    }

    return (svstat);
}
/***************************************************************/
/**  HRL -  struct handrec                                     */
/***************************************************************/
static int svare_method_HRL_new(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)
{
/*
struct handreclist {
    int                 hrl_estat;
    int                 hrl_nhandrec;
    struct handrec **   hrl_ahandrec;
};
*/
    int svstat = 0;
    struct handreclist * n_hrl;

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "<" SVAR_CLASS_NAME_HAND_RECORD_LIST ">");

    if (!svstat) {
        n_hrl = New(struct handreclist, 1);
        n_hrl->hrl_estat = 0;
        n_hrl->hrl_nhandrec = 0;
        n_hrl->hrl_ahandrec = NULL;
        svare_store_int_object(svx, asvv[0]->svv_val.svv_svnc, rsvv, n_hrl);
    }

    return (svstat);
}
/***************************************************************/
static int svare_method_HRL_delete(struct svarexec * svx,   /* NULL for destructor */
    const char * func_name,     /* NULL for destructor */
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)    /* NULL for destructor */
{
    int svstat = 0;
    struct handreclist * hrl;
    int ii;

    hrl = (struct handreclist *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
    if (hrl->hrl_ahandrec) {
        for (ii = 0; hrl->hrl_ahandrec[ii]; ii++) {
            free_handrec(hrl->hrl_ahandrec[ii]);
        }
        Free(hrl->hrl_ahandrec);
    }
    Free(hrl);

    return (svstat);
}
/***************************************************************/
static int svare_method_HRL_ReadFile(struct svarexec * svx,   /* NULL for destructor */
    const char * func_name,     /* NULL for destructor */
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)    /* NULL for destructor */
{
    int svstat = 0;
    struct handreclist * hrl;
    struct handrec * hr;
    char * fname;
    struct omarglobals * og = (struct omarglobals *)(svx->svx_gdata);

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "{" SVAR_CLASS_NAME_HAND_RECORD_LIST "}s");

    if (!svstat) {
        hrl = (struct handreclist *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        fname = asvv[1]->svv_val.svv_chars.svvc_val_chars;
        hr = parse_handrec(og, fname);
        if (hr) {
            if (hr->hr_estat) {
                svstat = svare_set_error(svx, SVARERR_FUNCTION_FAILED,
                        "Function %s(%s) returned an error %d",
                        func_name, fname, hr->hr_estat);
            }
            hrl->hrl_ahandrec = Realloc(hrl->hrl_ahandrec, struct handrec *, hrl->hrl_nhandrec + 2);
            hrl->hrl_ahandrec[hrl->hrl_nhandrec] = hr;
            hrl->hrl_nhandrec += 1;
            hrl->hrl_ahandrec[hrl->hrl_nhandrec] = NULL;
        }
    }

    return (svstat);
}
/***************************************************************/
static int svare_method_HRL_ReadDirectory(struct svarexec * svx,   /* NULL for destructor */
    const char * func_name,     /* NULL for destructor */
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)    /* NULL for destructor */
{
    int svstat = 0;
    struct handreclist * hrl;
    struct handrec **  ahandrec;
    char * dirname;
    char * hrreq;
    int nhandrec;
    int ii;
    struct omarglobals * og = (struct omarglobals *)(svx->svx_gdata);

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "{" SVAR_CLASS_NAME_HAND_RECORD_LIST "}s|"
        "{" SVAR_CLASS_NAME_HAND_RECORD_LIST "}ss");

    if (!svstat) {
        hrreq = NULL;
        hrl = (struct handreclist *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);

        dirname = asvv[1]->svv_val.svv_chars.svvc_val_chars;
        if (nsvv > 2) hrreq = asvv[2]->svv_val.svv_chars.svvc_val_chars;
        ahandrec = parse_handrecdir(og, dirname, hrreq);
        if (!ahandrec) {
            svstat = svare_set_error(svx, SVARERR_FUNCTION_FAILED,
                    "Function %s(%s) returned an error",
                    func_name, dirname);
        } else {
            nhandrec = 0;
            while (ahandrec[nhandrec]) nhandrec++;
            hrl->hrl_ahandrec = Realloc(hrl->hrl_ahandrec, struct handrec *, hrl->hrl_nhandrec + nhandrec + 1);
            for (ii = 0; ii < nhandrec; ii++) {
                hrl->hrl_ahandrec[hrl->hrl_nhandrec + ii] = ahandrec[ii];
            }
            hrl->hrl_nhandrec += nhandrec;
            hrl->hrl_ahandrec[hrl->hrl_nhandrec] = NULL;
            Free(ahandrec);
        }
    }

    return (svstat);
}
/***************************************************************/
static int svare_method_HRL_TrainNN(struct svarexec * svx,   /* NULL for destructor */
    const char * func_name,     /* NULL for destructor */
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)    /* NULL for destructor */
{
    int svstat = 0;
    struct handreclist * hrl;
    struct neural_network *  nn;
    char * cfg;
    int nepochs;
    int estat;
    struct omarglobals * og = (struct omarglobals *)(svx->svx_gdata);

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "{" SVAR_CLASS_NAME_HAND_RECORD_LIST "}{" SVAR_CLASS_NAME_NEURAL_NET "}si");

    if (!svstat) {
        hrl = (struct handreclist *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        nn = (struct neural_network *)(asvv[1]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        cfg = asvv[2]->svv_val.svv_chars.svvc_val_chars;
        nepochs = asvv[3]->svv_val.svv_val_int;

        if (!Stricmp(cfg, "Suited")) {
            estat = nnt_train_handrec(og, nn, hrl->hrl_ahandrec,
            SSORT_LONGEST, NNT_METHOD_HANDREC_SUITED, nepochs);
            if (estat) {
                svstat = svare_set_error(svx, SVARERR_FUNCTION_FAILED,
                        "Function %s(%s) returned an error",
                        func_name, cfg);
            }
        } else if (!Stricmp(cfg, "Notrump")) {
            estat = nnt_train_handrec(og, nn, hrl->hrl_ahandrec,
                SSORT_LONGEST, NNT_METHOD_HANDREC_NOTRUMP, nepochs);
            if (estat) {
                svstat = svare_set_error(svx, SVARERR_FUNCTION_FAILED,
                        "Function %s(%s) returned an error",
                        func_name, cfg);
            }
        } else {
            svstat = svare_set_error(svx, SVARERR_UNRECOGNIZED_ARG,
                    "Unrecognized %s argument: \"%s\"", func_name, cfg);
        }
    }

    return (svstat);
}
/***************************************************************/
static int svare_method_HRL_PlayNN(struct svarexec * svx,   /* NULL for destructor */
    const char * func_name,     /* NULL for destructor */
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)    /* NULL for destructor */
{
    int svstat = 0;
    struct handreclist * hrl;
    struct neural_network *  nn;
    char * cfg;
    int estat;
    struct omarglobals * og = (struct omarglobals *)(svx->svx_gdata);

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "{" SVAR_CLASS_NAME_HAND_RECORD_LIST "}{" SVAR_CLASS_NAME_NEURAL_NET "}s");

    if (!svstat) {
        hrl = (struct handreclist *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        nn = (struct neural_network *)(asvv[1]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        cfg = asvv[2]->svv_val.svv_chars.svvc_val_chars;
        init_playstats(&(hrl->hrl_pst));

        if (!Stricmp(cfg, "Suited")) {
            estat = nnt_play_handrec(og, nn, hrl->hrl_ahandrec,
                SSORT_LONGEST, NNT_METHOD_HANDREC_SUITED, &(hrl->hrl_pst));
            if (estat) {
                svstat = svare_set_error(svx, SVARERR_FUNCTION_FAILED,
                        "Function %s(%s) returned an error",
                        func_name, cfg);
            }
        } else if (!Stricmp(cfg, "Notrump")) {
            estat = nnt_play_handrec(og, nn, hrl->hrl_ahandrec,
                SSORT_LONGEST, NNT_METHOD_HANDREC_NOTRUMP, &(hrl->hrl_pst));
            if (estat) {
                svstat = svare_set_error(svx, SVARERR_FUNCTION_FAILED,
                        "Function %s(%s) returned an error",
                        func_name, cfg);
            }
        } else {
            svstat = svare_set_error(svx, SVARERR_UNRECOGNIZED_ARG,
                    "Unrecognized %s argument: \"%s\"", func_name, cfg);
        }
    }

    return (svstat);
}
/***************************************************************/
static int svare_method_HRL_GetSize(struct svarexec * svx,   /* NULL for destructor */
    const char * func_name,     /* NULL for destructor */
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)    /* NULL for destructor */
{
    int svstat = 0;
    struct handreclist * hrl;

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "{" SVAR_CLASS_NAME_HAND_RECORD_LIST "}");

    if (!svstat) {
        hrl = (struct handreclist *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        svare_svarvalue_int(rsvv, hrl->hrl_nhandrec);
    }

    return (svstat);
}
/***************************************************************/
static int svare_method_HRL_GetHandsPlayedNear(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)    /* NULL for destructor */
{
    int svstat = 0;
    struct handreclist * hrl;

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "{" SVAR_CLASS_NAME_HAND_RECORD_LIST "}");

    if (!svstat) {
        hrl = (struct handreclist *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        svare_svarvalue_int(rsvv,
            hrl->hrl_pst.pst_ncorrect + hrl->hrl_pst.pst_nplus1 + hrl->hrl_pst.pst_nminus1);
    }

    return (svstat);
}
/***************************************************************/
static int svare_method_HRL_GetHandsPlayedPlus1(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)    /* NULL for destructor */
{
    int svstat = 0;
    struct handreclist * hrl;

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "{" SVAR_CLASS_NAME_HAND_RECORD_LIST "}");

    if (!svstat) {
        hrl = (struct handreclist *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        svare_svarvalue_int(rsvv, hrl->hrl_pst.pst_nplus1);
    }

    return (svstat);
}
/***************************************************************/
static int svare_method_HRL_GetHandsPlayedMinus1(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)    /* NULL for destructor */
{
    int svstat = 0;
    struct handreclist * hrl;

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "{" SVAR_CLASS_NAME_HAND_RECORD_LIST "}");

    if (!svstat) {
        hrl = (struct handreclist *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        svare_svarvalue_int(rsvv, hrl->hrl_pst.pst_nminus1);
    }

    return (svstat);
}
/***************************************************************/
static int svare_method_HRL_GetHandsPlayedCorrect(struct svarexec * svx,   /* NULL for destructor */
    const char * func_name,     /* NULL for destructor */
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)    /* NULL for destructor */
{
    int svstat = 0;
    struct handreclist * hrl;

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "{" SVAR_CLASS_NAME_HAND_RECORD_LIST "}");

    if (!svstat) {
        hrl = (struct handreclist *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        svare_svarvalue_int(rsvv, hrl->hrl_pst.pst_ncorrect);
    }

    return (svstat);
}
/***************************************************************/
static int svare_method_HRL_GetHandsPlayed(struct svarexec * svx,   /* NULL for destructor */
    const char * func_name,     /* NULL for destructor */
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)    /* NULL for destructor */
{
    int svstat = 0;
    struct handreclist * hrl;

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "{" SVAR_CLASS_NAME_HAND_RECORD_LIST "}");

    if (!svstat) {
        hrl = (struct handreclist *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        svare_svarvalue_int(rsvv, hrl->hrl_pst.pst_nhands);
    }

    return (svstat);
}
/***************************************************************/
#ifdef OLD_WAY
void svar_add_function(struct svarexec * svx,
    const char * func_name,
    SVAR_INTERNAL_FUNCTION ifuncptr )
{
    int is_dup;

    is_dup = dbtree_insert(svx->svx_func_dbt,
        func_name, IStrlen(func_name) + 1, (void*)ifuncptr);
}
#endif
/***************************************************************/
int svar_add_function(struct svarexec * svx,
    const char * func_name,
    SVAR_INTERNAL_FUNCTION ifuncptr )
{
    int svstat = 0;
    int is_dup;
    struct svarvalue * svv;

    svv = svare_new_variable();
    svv->svv_dtype = SVV_DTYPE_INT_FUNCTION;
    svv->svv_val.svv_int_func = ifuncptr;

    if (svx->svx_nvars) {
        is_dup = dbtree_insert(svx->svx_avars[svx->svx_nvars-1]->svar_svarvalue_dbt,
            func_name, IStrlen(func_name) + 1, svv);
            if (is_dup) {
                svstat = svare_set_error(svx, SVARERR_DUPLICATE_INT_FUNC,
                    "Duplicate internal function: %s", func_name);
            }
    }

    return (svstat);
}
/***************************************************************/
/**  Integer -  Integer, like in Java                          */
/***************************************************************/
static int svare_method_Integer_new(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)
{
    int svstat = 0;
    struct jsinteger *  jsi;

    if (nsvv != 2) {
        svstat = svare_set_error(svx, SVARERR_INVALID_NUM_PARMS,
            "Expecting 1 arguments to %s. Found %d", func_name, nsvv - 1);
    } else if (asvv[1]->svv_dtype != SVV_DTYPE_INT) {
        svstat = svare_set_error(svx, SVARERR_INVALID_PARM_TYPE,
            "Expecting integer argument to %s.", func_name);
    } else {
        jsi = New(struct jsinteger, 1);
        jsi->jsi_value = asvv[1]->svv_val.svv_val_int;
        svare_store_int_object(svx, asvv[0]->svv_val.svv_svnc, rsvv, jsi);
    }

    return (svstat);
}
/***************************************************************/
static int svare_method_Integer_delete(struct svarexec * svx,   /* NULL for destructor */
    const char * func_name,     /* NULL for destructor */
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)    /* NULL for destructor */
{
    int svstat = 0;
    struct jsinteger *  jsi;

    jsi = (struct jsinteger *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
    Free(jsi);

    return (svstat);
}
/***************************************************************/
static int svare_method_Integer_toString(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)
{
    int svstat = 0;
    struct jsinteger *  jsi;
    int nlen;
    char numbuf[16];

    if (nsvv != 1) {
        svstat = svare_set_error(svx, SVARERR_INVALID_NUM_PARMS,
            "Expecting no arguments to %s. Found %d", func_name, nsvv);
    } else {
        jsi = (struct jsinteger *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        Snprintf(numbuf, sizeof(numbuf), "%d", jsi->jsi_value);
        nlen = IStrlen(numbuf);
        svare_store_string(rsvv, numbuf, nlen);
    }

    return (svstat);
}
/***************************************************************/
static int svare_method_Integer_Get(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)
{
    int svstat = 0;
    struct jsinteger *  jsi;

    if (nsvv != 1) {
        svstat = svare_set_error(svx, SVARERR_INVALID_NUM_PARMS,
            "Expecting no arguments to %s. Found %d", func_name, nsvv);
    } else {
        jsi = (struct jsinteger *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        svare_svarvalue_int(rsvv, jsi->jsi_value);
    }

    return (svstat);
}
/***************************************************************/
static int svare_method_Integer_Set(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)
{
    int svstat = 0;
    struct jsinteger *  jsi;

    if (nsvv != 2) {
        svstat = svare_set_error(svx, SVARERR_INVALID_NUM_PARMS,
            "Expecting no arguments to %s. Found %d", func_name, nsvv);
    } else if (asvv[1]->svv_dtype != SVV_DTYPE_INT) {
        svstat = svare_set_error(svx, SVARERR_INVALID_PARM_TYPE,
            "Expecting integer argument to %s.", func_name);
    } else {
        jsi = (struct jsinteger *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        jsi->jsi_value = asvv[1]->svv_val.svv_val_int;
    }

    return (svstat);
}
/***************************************************************/
/**  Testing                                                   */
/***************************************************************/
#if IS_TESTING_CLASS
static int svare_method_TestingNew(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)
{
    int svstat = 0;
    struct jsdouble *  jsd;

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "<" SVAR_CLASS_NAME_TESTING ">d");

    if (!svstat) {
        jsd = New(struct jsdouble, 1);
        jsd->jsd_value = asvv[1]->svv_val.svv_val_double;
        svare_store_int_object(svx, asvv[0]->svv_val.svv_svnc, rsvv, jsd);
    }

    return (svstat);
}
/***************************************************************/
static int svare_method_Testing_new(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)
{
    int svstat = 0;
    struct jsdouble *  jsd;

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "<" SVAR_CLASS_NAME_TESTING ">d");

    if (!svstat) {
        jsd = New(struct jsdouble, 1);
        jsd->jsd_value = asvv[1]->svv_val.svv_val_double;
        svare_store_int_object(svx, asvv[0]->svv_val.svv_svnc, rsvv, jsd);
    }

    return (svstat);
}
/***************************************************************/
static int svare_method_TestingDelete(struct svarexec * svx,   /* NULL for destructor */
    const char * func_name,     /* NULL for destructor */
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)    /* NULL for destructor */
{
    int svstat = 0;
    struct jsdouble *  jsd;

    jsd = (struct jsdouble *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
    Free(jsd);

    return (svstat);
}
/***************************************************************/
static int svare_method_TestingToString(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)
{
    int svstat = 0;
    struct jsdouble *  jsd;
    int nlen;
    char numbuf[32];

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "{" SVAR_CLASS_NAME_TESTING "}");

    if (!svstat) {
        jsd = (struct jsdouble *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        double_to_string(numbuf, jsd->jsd_value, sizeof(numbuf));
        nlen = IStrlen(numbuf);
        svare_store_string(rsvv, numbuf, nlen);
    }

    return (svstat);
}
/***************************************************************/
static int svare_method_TestingGet(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)
{
    int svstat = 0;
    struct jsdouble *  jsd;

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "{" SVAR_CLASS_NAME_TESTING "}");

    if (!svstat) {
        jsd = (struct jsdouble *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        svare_svarvalue_double(rsvv, jsd->jsd_value);
    }

    return (svstat);
}
/***************************************************************/
static int svare_method_TestingSet(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)
{
    int svstat = 0;
    struct jsdouble *  jsd;

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "{" SVAR_CLASS_NAME_TESTING "}d");

    if (!svstat) {
        jsd = (struct jsdouble *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        jsd->jsd_value = asvv[1]->svv_val.svv_val_double;
    }

    return (svstat);
}
#endif
/***************************************************************/
/**  Auction -  Auction, like in Java                          */
/***************************************************************/
static int svare_method_Auction_new(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)
{
    int svstat = 0;
    struct auction_class * auc;

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "<" SVAR_CLASS_NAME_AUCTION ">");

    if (!svstat) {
        auc = New(struct auction_class, 1);
        auc->auc_auction = new_auction(0, VUL_NONE);
        svare_store_int_object(svx, asvv[0]->svv_val.svv_svnc, rsvv, auc);
    }

    return (svstat);
}
/***************************************************************/
static int svare_method_Auction_delete(struct svarexec * svx,   /* NULL for destructor */
    const char * func_name,     /* NULL for destructor */
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)    /* NULL for destructor */
{
    int svstat = 0;
    struct auction_class * auc;

    auc = (struct auction_class *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
    Free(auc->auc_auction);
    Free(auc);

    return (svstat);
}
/***************************************************************/
static int svare_method_Auction_toString(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)
{
    int svstat = 0;
    struct auction_class * auc;
    char * austr;
    int aulen;

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "{" SVAR_CLASS_NAME_AUCTION "}");

    if (!svstat) {
        auc = (struct auction_class *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        austr = auction_to_chars(auc->auc_auction);
        aulen = IStrlen(austr);
        svare_store_string_pointer(rsvv, austr, aulen);
    }

    return (svstat);
}
/***************************************************************/
static int auction_makebid(
        struct svarexec * svx,
        struct auction_class * auc,
        const char * bidbuf)
{
    int svstat = 0;
    int ptr;
    int bt;
    int bl;
    int bs;
    int sdone;
    int nbids;
    int is_bid;
    struct omarglobals * og = (struct omarglobals *)(svx->svx_gdata);

    nbids = 0;
    ptr = 0;
    while (isspace(bidbuf[ptr])) ptr++;

    sdone = 0;
    while (!sdone && bidbuf[ptr]) {
        is_bid = next_bid(og, bidbuf, &ptr, &bt, &bl, &bs);
        if (!is_bid) {
            svstat = omarerr(og, "Invalid bid: %s", bidbuf);
            sdone = 1;
        } else {
            if (bt == NONE) {
                sdone = 1;
            } else {
                make_bid(auc->auc_auction, bt, bl, bs);
                nbids++;
            }
        }
    }
    
    if (!svstat && !nbids) {
        svstat = omarerr(og, "No bids: %s", bidbuf);
    }

    return (svstat);
}
/***************************************************************/
static int svare_method_Auction_makeBid(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)
{
/*
** See Also: handle_auction()
*/

    int svstat = 0;
    struct auction_class * auc;
    char * bidbuf;

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "{" SVAR_CLASS_NAME_AUCTION "}s");

    if (!svstat) {
        auc = (struct auction_class *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        bidbuf = asvv[1]->svv_val.svv_chars.svvc_val_chars;
        svstat = auction_makebid(svx, auc, bidbuf);
    }

    return (svstat);
}
/***************************************************************/
static int svare_method_Auction_setDealer(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)
{
/*
** See Also: handle_auction()
*/

    int svstat = 0;
    struct auction_class * auc;
    char * dlrstr;
    int dlr;
    struct omarglobals * og = (struct omarglobals *)(svx->svx_gdata);

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "{" SVAR_CLASS_NAME_AUCTION "}s");

    if (!svstat) {
        auc = (struct auction_class *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        dlrstr = asvv[1]->svv_val.svv_chars.svvc_val_chars;
        dlr = char_to_player(dlrstr[0]);
        if (dlr < 0) {
            svstat = omarerr(og, "Invalid dealer: %s", dlrstr);
        } else {
            auc->auc_auction->au_dealer = dlr;
        }
    }

    return (svstat);
}
/***************************************************************/
static int svare_method_Auction_setVulnerability(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)
{
/*
** See Also: handle_auction()
*/

    int svstat = 0;
    struct auction_class * auc;
    char * vulstr;
    int vul;
    struct omarglobals * og = (struct omarglobals *)(svx->svx_gdata);

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "{" SVAR_CLASS_NAME_AUCTION "}s");

    if (!svstat) {
        auc = (struct auction_class *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        vulstr = asvv[1]->svv_val.svv_chars.svvc_val_chars;
        vul = chars_to_vulnerability(vulstr);
        if (vul < 0) {
            svstat = omarerr(og, "Invalid vulnerability: %s", vulstr);
        } else {
            auc->auc_auction->au_vulnerability = vul;
        }
    }

    return (svstat);
}
/***************************************************************/
static int svare_method_Auction_getNextBidder(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)
{
/*
**
*/

    int svstat = 0;
    struct auction_class * auc;
    int bidder;
    int bdrlen;
    char bdrstr[8];

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "{" SVAR_CLASS_NAME_AUCTION "}");

    if (!svstat) {
        auc = (struct auction_class *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        bidder = (auc->auc_auction->au_dealer + auc->auc_auction->au_nbids) & 3;
        bdrstr[0] = display_player[bidder];
        bdrstr[1] = '\0';
        bdrlen = 1;
        svare_store_string(rsvv, bdrstr, bdrlen);
    }

    return (svstat);
}
/***************************************************************/
/**  Analysis                                                  */
/***************************************************************/
static int svare_Analysis_readBiddef(struct svarexec * svx,
    struct analysis_class * ans,
    const char * biddeffame)
{
    int svstat = 0;
    struct frec * biddef_fr;
    struct omarglobals * og = (struct omarglobals *)(svx->svx_gdata);

    biddef_fr = frec_open(biddeffame, "r", "Bid def file");
    if (!biddef_fr) {
        svstat = omarerr(og, "Error opening file: %s", biddeffame);
    } else {
        svstat = parse_bids(og, biddef_fr, ans->ans_seqinfo);
        frec_close(biddef_fr);
    }

    return (svstat);
}
/***************************************************************/
static int svare_method_Analysis_new(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)
{
    int svstat = 0;
    struct analysis_class * ans;
    char * biddeffame;

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "<" SVAR_CLASS_NAME_ANALYSIS ">s");

    if (!svstat) {
        biddeffame = asvv[1]->svv_val.svv_chars.svvc_val_chars;
        ans = New(struct analysis_class, 1);
        ans->ans_aninfo = new_analysis();
        ans->ans_seqinfo = new_seqinfo();
        svstat = svare_Analysis_readBiddef(svx, ans, biddeffame);

        svare_store_int_object(svx, asvv[0]->svv_val.svv_svnc, rsvv, ans);
    }

    return (svstat);
}
/***************************************************************/
static int svare_method_Analysis_delete(struct svarexec * svx,   /* NULL for destructor */
    const char * func_name,     /* NULL for destructor */
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)    /* NULL for destructor */
{
    int svstat = 0;
    struct analysis_class * ans;

    ans = (struct analysis_class *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
    free_cc(ans->ans_seqinfo);
    free_seqinfo(ans->ans_seqinfo);
    free_analysis(ans->ans_aninfo);
    Free(ans);

    return (svstat);
}
/***************************************************************/
static int svare_method_Analysis_toString(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)
{
    int svstat = 0;
    struct analysis_class * ans;
    char * ansstr;
    int anslen;

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "{" SVAR_CLASS_NAME_ANALYSIS "}");

    if (!svstat) {
        ans = (struct analysis_class *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        ansstr = NULL;
        anslen = IStrlen(ansstr);
        svare_store_string_pointer(rsvv, ansstr, anslen);
    }

    return (svstat);
}
/***************************************************************/
static int svare_method_Analysis_testFile(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)
{
/*
** Example:
**      set ans = new Analysis("biddef_test.txt");
**      call ans.testFile("bidtest.txt");
*/

    int svstat = 0;
    struct analysis_class * ans;
    char * bidtestfame;
    struct frec * bidtest_fr;
    struct omarglobals * og = (struct omarglobals *)(svx->svx_gdata);

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "{" SVAR_CLASS_NAME_ANALYSIS "}s");

    if (!svstat) {
        ans = (struct analysis_class *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        bidtestfame = asvv[1]->svv_val.svv_chars.svvc_val_chars;
        bidtest_fr = frec_open(bidtestfame, "r", "Bid TEST file");
        if (!bidtest_fr) {
            svstat = omarerr(og, "Error opening file: %s", bidtestfame);
        } else {
            analyze_bidfile(og, bidtest_fr, ans->ans_seqinfo);
            frec_close(bidtest_fr);
        }
    }

    return (svstat);
}
/***************************************************************/
static int svare_method_Analysis_printAnalysis(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)
{
    int svstat = 0;
    struct auction_class * auc;
    struct analysis_class * ans;
    struct omarglobals * og = (struct omarglobals *)(svx->svx_gdata);

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "{" SVAR_CLASS_NAME_ANALYSIS "}"
        "{" SVAR_CLASS_NAME_AUCTION "}" );

    if (!svstat) {
        ans = (struct analysis_class *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        auc = (struct auction_class *)(asvv[1]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        print_analysis(og, auc->auc_auction, ans->ans_aninfo);
    }

    return (svstat);
}
/***************************************************************/
#if 0
static int svare_method_Analysis_reset(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)
{
/*
**
*/

    int svstat = 0;
    struct analysis_class * ans;
    struct omarglobals * og = (struct omarglobals *)(svx->svx_gdata);

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "{" SVAR_CLASS_NAME_ANALYSIS "}");

    if (!svstat) {
        ans = (struct analysis_class *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        reset_analysis(ans->ans_aninfo);
    }

    return (svstat);
}
#endif
/***************************************************************/
/**  Bid                                                  */
/***************************************************************/
#if 0
static int svare_method_parse_bid(struct svarexec * svx,
    struct bid_class * bidc,
    const char * bidstr)
{
    int svstat = 0;
    int ok;
    int ptr;
    int bt;
    int bl;
    int bs;
    struct omarglobals * og = (struct omarglobals *)(svx->svx_gdata);

    ptr = 0;
    ok = next_bid(og, bidstr, &ptr, &bt, &bl, &bs);
    if (!ok) {
        svstat = omarerr(og, "Invalid bid: %s", bidstr);
    }
    bidc->bidc_encoded_bid = encode_bid(bt, bl, bs);

    return (svstat);
}
/***************************************************************/
static int svare_method_Bid_new(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)
{
    int svstat = 0;
    struct bid_class * bidc;
    char * bidstr;

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "<" SVAR_CLASS_NAME_BID ">s");

    if (!svstat) {
        bidc = New(struct bid_class, 1);
        bidstr = asvv[1]->svv_val.svv_chars.svvc_val_chars;
        svstat = svare_method_parse_bid(svx, bidc, bidstr);

        svare_store_int_object(svx, asvv[0]->svv_val.svv_svnc, rsvv, bidc);
    }

    return (svstat);
}
/***************************************************************/
static int svare_method_Bid_delete(struct svarexec * svx,   /* NULL for destructor */
    const char * func_name,     /* NULL for destructor */
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)    /* NULL for destructor */
{
    int svstat = 0;
    struct bid_class * bidc;

    bidc = (struct bid_class *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
    Free(bidc);

    return (svstat);
}
/***************************************************************/
static int svare_method_Bid_toString(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)
{
    int svstat = 0;
    struct bid_class * bidc;
    char * bidstr;
    int bidlen;
    struct omarglobals * og = (struct omarglobals *)(svx->svx_gdata);

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "{" SVAR_CLASS_NAME_BID "}");

    if (!svstat) {
        bidc = (struct bid_class *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        bidstr = ebid_to_chars(bidc->bidc_encoded_bid);
        bidlen = IStrlen(bidstr);
        svare_store_string_pointer(rsvv, bidstr, bidlen);
    }

    return (svstat);
}
#endif
/***************************************************************/
/**  Bidding                                                   */
/***************************************************************/
static int svare_Bidding_findBestBid(struct svarexec * svx,
    struct auction_class * auc,
    struct analysis_class *ans,
    struct hand_class * hnd)
{
    int ebid;
    struct omarglobals * og = (struct omarglobals *)(svx->svx_gdata);

    if (!ans->ans_seqinfo->cc) {
        init_cc(og, ans->ans_seqinfo);
    }
    analyze_auction(og, ans->ans_seqinfo, auc->auc_auction, ans->ans_aninfo, 0);

    ebid = find_best_bid(og,
        ans->ans_seqinfo, auc->auc_auction, ans->ans_aninfo, hnd->hnd_handinfo);

    return (ebid);
}
/***************************************************************/
static int svare_method_Bidding_findBestBid(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)
{
    int svstat = 0;
    struct auction_class * auc;
    struct hand_class * hnd;
    struct analysis_class * ans;
    int ebid;
    char * bidstr;
    int bidlen;

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "<" SVAR_CLASS_NAME_BIDDING ">"
        "{" SVAR_CLASS_NAME_AUCTION "}"
        "{" SVAR_CLASS_NAME_ANALYSIS "}"
        "{" SVAR_CLASS_NAME_HAND "}");

    if (!svstat) {
        auc = (struct auction_class *)(asvv[1]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        ans = (struct analysis_class *)(asvv[2]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        hnd = (struct hand_class *)(asvv[3]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        ebid = svare_Bidding_findBestBid(svx, auc, ans, hnd);
        bidstr = ebid_to_chars(ebid);
        bidlen = IStrlen(bidstr);
        svare_store_string_pointer(rsvv, bidstr, bidlen);
    }

    return (svstat);
}
/***************************************************************/
static void svare_Bidding_showNaturalBids(struct svarexec * svx,
    struct auction_class * auc,
    struct analysis_class *ans,
    struct hand_class * hnd,
    struct neural_network *  nn_suited,
    struct neural_network *  nn_notrump)
{
    struct omarglobals * og = (struct omarglobals *)(svx->svx_gdata);

    if (!ans->ans_seqinfo->cc) {
        init_cc(og, ans->ans_seqinfo);
    }
    analyze_auction(og, ans->ans_seqinfo, auc->auc_auction, ans->ans_aninfo, 0);

    show_natural_bids(og,
        ans->ans_seqinfo, auc->auc_auction, ans->ans_aninfo, hnd->hnd_handinfo,
        nn_suited, nn_notrump);
}
/***************************************************************/
static int svare_method_Bidding_showNaturalBids(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)
{
    int svstat = 0;
    struct auction_class * auc;
    struct hand_class * hnd;
    struct analysis_class * ans;
    struct neural_network *  nn_suited;
    struct neural_network *  nn_notrump;

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "<" SVAR_CLASS_NAME_BIDDING ">"
        "{" SVAR_CLASS_NAME_AUCTION "}"
        "{" SVAR_CLASS_NAME_ANALYSIS "}"
        "{" SVAR_CLASS_NAME_HAND "}"
        "{" SVAR_CLASS_NAME_NEURAL_NET "}"
        "{" SVAR_CLASS_NAME_NEURAL_NET "}" );

    if (!svstat) {
        auc = (struct auction_class *)(asvv[1]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        ans = (struct analysis_class *)(asvv[2]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        hnd = (struct hand_class *)(asvv[3]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        nn_suited = (struct neural_network *)(asvv[4]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        nn_notrump = (struct neural_network *)(asvv[5]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        svare_Bidding_showNaturalBids(svx, auc, ans, hnd, nn_suited, nn_notrump);
    }

    return (svstat);
}
/***************************************************************/
#ifdef OLD_STUFF
enum e_btypes {
    BTY_UNDEFINED,
    BTY_PASS,
    BTY_OPEN_1,
    BTY_OPEN_1N,
    BTY_OPEN_2C,
    BTY_OPEN_2,
    BTY_OPEN_2N,
    BTY_OPEN_3,
    BTY_OPEN_3N,
    BTY_OPEN_4,
    BTY_OPEN_5I,

    BTY_RESPOND_1_OVER_1,
    BTY_RESPOND_1N_OVER_1,
    BTY_RESPOND_2_OVER_1,
    BTY_RESPOND_2_OVER_1N,
    BTY_RESPOND_2_RAISE_1,
    BTY_RESPOND_2_RAISE_1N,

    BTY_REBID_1,
    BTY_REBID_1N,
    BTY_REBID_2_OVER_1,
    BTY_REBID_2_OVER_1N,
    BTY_REBID_2_OVER_2,
    BTY_REBID_2_SUIT_OVER_1,
    BTY_REBID_2_SUIT_OVER_1N,
    BTY_REBID_2_SUIT_OVER_2,
    BTY_REBID_2N_SUIT_OVER_1,
    BTY_REBID_2N_SUIT_OVER_1N,
    BTY_REBID_2N_SUIT_OVER_2,

    BTY_ZZZZ
};
enum e_bidder {
    BDR_OPENER,             /* BDR_OVERCALLER */
    BDR_RESPONDER,          /* BDR_OVERRESPONDER */
    BDR_ZZZZ
};
struct bidhistlist { /* bhl_ */
    int bhl_nbidhist;
    struct bidhist ** bhl_abidhist;
};
struct bidhist { /* bhis_ */
    int bhis_bt;        /* bt - BID, PASS, DOUBLE, ... */
    int bhis_bl;        /* bl - Bid level 1-7 */
    int bhis_bs;        /* bs - CLUBS, DIAMONDS, ... */
    int bhis_flags;
    enum e_bidder bhis_bidder;
    enum e_btypes bhis_bty;
    struct bidhistlist * bhis_bidhistlist;
};
/***************************************************************/
static enum e_btypes calculate_btype_2(
    int bt,
    int bl,
    int bs,
    struct bidhist * bhis)
{
    enum e_btypes bty;

    bty = BTY_UNDEFINED;

    return (bty);
}
/***************************************************************/
static enum e_btypes calculate_btype_1(
    int bt,
    int bl,
    int bs,
    struct bidhist * bhis)
{
    enum e_btypes bty;

    bty = BTY_UNDEFINED;
    if (bt == PASS) {
        bty = BTY_PASS;
    } else if (bt == BID) {
        switch (bl) {
            case 1:
                if (bs == NOTRUMP) {
                    bty = BTY_OPEN_1N;
                } else {
                    bty = BTY_OPEN_1;
                }
                break;
            case 2:
                if (bs == NOTRUMP) {
                    bty = BTY_OPEN_1N;
                } else {
                    bty = BTY_OPEN_1;
                }
                break;
            case 3:
                if (bs == NOTRUMP) {
                    bty = BTY_OPEN_1N;
                } else {
                    bty = BTY_OPEN_1;
                }
                break;
            case 4:
                if (bs != NOTRUMP) {
                    bty = BTY_OPEN_4;
                }
                break;
            case 5:
                if (bs == CLUBS || bs == DIAMONDS) {
                    bty = BTY_OPEN_5I;
                }
                break;
            default:
                break;
        }
    }

    return (bty);
}
/***************************************************************/
static enum e_btypes calculate_btype(
    int bt,
    int bl,
    int bs,
    struct bidhist * bhis)
{
    enum e_btypes bty;

    bty = BTY_UNDEFINED;
    if (bhis->bhis_bidhistlist) {
        bty = calculate_btype_2(bt, bl, bs, bhis);
    } else {
        bty = calculate_btype_1(bt, bl, bs, bhis);
    }

    return (bty);
}
/***************************************************************/
enum e_bdescs {
    BDE_EOL = 0,
    BDE_OPEN,
    BDE_PASS,
    BDE_1,
    BDE_1N,
    BDE_2C,
    BDE_2,
    BDE_2N,
    BDE_3,
    BDE_3N,
    BDE_4I,
    BDE_4J,
    BDE_4N,
    BDE_5I,
    BDE_5J,
    BDE_5N,
    BDE_6,
    BDE_6N,
    BDE_7,
    BDE_7N,
    BDE_RESPOND,
    BDE_OVER,
    BDE_RAISE,
    BDE_JUMP,
    BDE_SKIP,
    BDE_ZZ
};
#define BDESCS_SZ  8
struct bdeschistlist { /* bdhl_ */
    int bdhl_nbdh;
    struct bdeschist ** bdhl_abdh;
};
struct bdeschist { /* bdh_ */
    int bdh_bt;        /* bt - BID, PASS, DOUBLE, ... */
    int bdh_bl;        /* bl - Bid level 1-7 */
    int bdh_bs;        /* bs - CLUBS, DIAMONDS, ... */
    int bdh_flags;
    unsigned char bdh_bdescs[BDESCS_SZ];
};
/***************************************************************/
static void bdesc_describe_2(
    int bt,
    int bl,
    int bs,
    struct bdeschistlist * bdhl,
    unsigned char *  outbdescs,
    int enflags)
{
/*
**  Responses to opening bid
*/
    int outbdlen;
    int inbdix0;

    outbdlen = 0;
    inbdix0 = 0;

    if (bdhl->bdhl_abdh[0]->bdh_bdescs[inbdix0] == BDE_OPEN) {
        inbdix0++;
        outbdescs[outbdlen++] = BDE_RESPOND;
        if (bdhl->bdhl_abdh[0]->bdh_bdescs[inbdix0] == BDE_1) {    /* Open 1 of a suit */
            inbdix0++;
            if (bl == 1) {
                if (bs == NOTRUMP) {
                    outbdescs[outbdlen++] = BDE_1N;
                    outbdescs[outbdlen++] = BDE_OVER;
                    outbdescs[outbdlen++] = BDE_1;
                } else {
                    outbdescs[outbdlen++] = BDE_1;
                    outbdescs[outbdlen++] = BDE_OVER;
                    outbdescs[outbdlen++] = BDE_1;
                }
            } else if (bl == 2) {
                if (bs == NOTRUMP) {
                    outbdescs[outbdlen++] = BDE_2N;
                    outbdescs[outbdlen++] = BDE_OVER;
                    outbdescs[outbdlen++] = BDE_1;
                } else if (bs < bdhl->bdhl_abdh[0]->bdh_bs) {
                    outbdescs[outbdlen++] = BDE_2;
                    outbdescs[outbdlen++] = BDE_OVER;
                    outbdescs[outbdlen++] = BDE_1;
                } else if (bs == bdhl->bdhl_abdh[0]->bdh_bs) {
                    outbdescs[outbdlen++] = BDE_2;
                    outbdescs[outbdlen++] = BDE_RAISE;
                    outbdescs[outbdlen++] = BDE_1;
                } else {
                    outbdescs[outbdlen++] = BDE_2;
                    outbdescs[outbdlen++] = BDE_JUMP;
                    outbdescs[outbdlen++] = BDE_1;
                }
            } else if (bl == 3) {
                if (bs == NOTRUMP) {
                    outbdescs[outbdlen++] = BDE_3N;
                    outbdescs[outbdlen++] = BDE_OVER;
                    outbdescs[outbdlen++] = BDE_1;
                } else if (bs < bdhl->bdhl_abdh[0]->bdh_bs) {
                    outbdescs[outbdlen++] = BDE_3;
                    outbdescs[outbdlen++] = BDE_JUMP;
                    outbdescs[outbdlen++] = BDE_1;
                } else if (bs == bdhl->bdhl_abdh[0]->bdh_bs) {
                    outbdescs[outbdlen++] = BDE_3;
                    outbdescs[outbdlen++] = BDE_RAISE;
                    outbdescs[outbdlen++] = BDE_1;
                } else {
                    outbdescs[outbdlen++] = BDE_3;
                    outbdescs[outbdlen++] = BDE_SKIP;
                    outbdescs[outbdlen++] = BDE_1;
                }
            }
        } else if (bdhl->bdhl_abdh[0]->bdh_bdescs[inbdix0] == BDE_1N) {    /* Open 1 notrump */
            if (bl == 2) {
                if (bs == NOTRUMP) {
                    outbdescs[outbdlen++] = BDE_2N;
                    outbdescs[outbdlen++] = BDE_OVER;
                    outbdescs[outbdlen++] = BDE_1N;
                } else if (bs == CLUBS) {
                    outbdescs[outbdlen++] = BDE_2C;
                    outbdescs[outbdlen++] = BDE_OVER;
                    outbdescs[outbdlen++] = BDE_1N;
                } else {
                    outbdescs[outbdlen++] = BDE_2;
                    outbdescs[outbdlen++] = BDE_OVER;
                    outbdescs[outbdlen++] = BDE_1N;
                }
            } else if (bl == 3) {
                if (bs == NOTRUMP) {
                    outbdescs[outbdlen++] = BDE_3N;
                    outbdescs[outbdlen++] = BDE_OVER;
                    outbdescs[outbdlen++] = BDE_1N;
                } else {
                    outbdescs[outbdlen++] = BDE_3;
                    outbdescs[outbdlen++] = BDE_JUMP;
                    outbdescs[outbdlen++] = BDE_1N;
                }
            }
        }
    }
}
/***************************************************************/
static void bdesc_describe_1(
    int bt,
    int bl,
    int bs,
    unsigned char * outbdescs,
    int enflags)
{
    int outbdlen;

    outbdlen = 0;

    outbdescs[outbdlen++] = BDE_OPEN;
    if (bt == PASS) {
        outbdescs[outbdlen++] = BDE_PASS;
    } else if (bt == BID) {
        switch (bl) {
            case 1:
                if (bs == NOTRUMP) {
                    outbdescs[outbdlen++] = BDE_1N;
                } else {
                    outbdescs[outbdlen++] = BDE_1;
                }
                break;
            case 2:
                if (bs == NOTRUMP) {
                    outbdescs[outbdlen++] = BDE_2N;
                } else if (bs == CLUBS) {
                    outbdescs[outbdlen++] = BDE_2C;
                } else {
                    outbdescs[outbdlen++] = BDE_2;
                }
                break;
            case 3:
                if (bs == NOTRUMP) {
                    outbdescs[outbdlen++] = BDE_3N;
                } else {
                    outbdescs[outbdlen++] = BDE_3;
                }
                break;
            case 4:
                if (bs == NOTRUMP) {
                    outbdescs[outbdlen++] = BDE_4N;
                } else if (bs == CLUBS || bs == DIAMONDS) {
                    outbdescs[outbdlen++] = BDE_4I;
                } else {
                    outbdescs[outbdlen++] = BDE_4J;
                }
                break;
            case 5:
                if (bs == NOTRUMP) {
                    outbdescs[outbdlen++] = BDE_1N;
                } else if (bs == CLUBS || bs == DIAMONDS) {
                    outbdescs[outbdlen++] = BDE_5I;
                } else {
                    outbdescs[outbdlen++] = BDE_5J;
                }
                break;
            case 6:
                if (bs == NOTRUMP) {
                    outbdescs[outbdlen++] = BDE_6N;
                } else {
                    outbdescs[outbdlen++] = BDE_6;
                }
                break;
            case 7:
                if (bs == NOTRUMP) {
                    outbdescs[outbdlen++] = BDE_7N;
                } else {
                    outbdescs[outbdlen++] = BDE_7;
                }
                break;
            default:
                break;
        }
    }
}
#endif
/***************************************************************/
static char * bidder_names2[2] = { "OPENER", "RESPONDER" };
#define BDESCS_SZ  8
#define BDESCS_STR_SZ  48
struct bdeschistlist { /* bdhl_ */
    int bdhl_nbdh;
    int bdhl_xbdh;
    struct bdeschist ** bdhl_abdh;
};
struct bdeschist { /* bdh_ */
    int bdh_bt;        /* bt - BID, PASS, DOUBLE, ... */
    int bdh_bl;        /* bl - Bid level 1-7 */
    int bdh_bs;        /* bs - CLUBS, DIAMONDS, ... */
    int bdh_flags;
    unsigned char bdh_bdescs[BDESCS_SZ];
    struct bdeschistlist * bdh_bdhl;
};
enum e_bdescs {
    BDE_EOL = 0,
    BDE_OPEN,
    BDE_RESPOND,
    BDE_RESPOND_NT,

    BDE_PASS,
    BDE_BID_SUIT,
    BDE_BID_NOTRUMP,
    BDE_BID_MINOR,
    BDE_BID_MAJOR,
    BDE_BID_CLUBS,
    BDE_BID_DIAMONDS,
    BDE_BID_HEARTS,
    BDE_BID_SPADES,

    BDE_JUMP_SUIT,
    BDE_SKIP_SUIT,
    BDE_RAISE,

    BDE_ZZ
};
/***************************************************************/
static void bdesc_describe_2(
    int bt,
    int bl,
    int bs,
    struct bdeschistlist * bdhl,
    unsigned char *  outbdescs,
    int enflags)
{
/*
**  Responses to opening bid
*/
    int outbdlen;
    int inbdix0;

    outbdlen = 0;
    inbdix0 = 0;

    if (bdhl->bdhl_abdh[0]->bdh_bdescs[inbdix0] == BDE_OPEN) {
        inbdix0++;
        if (bdhl->bdhl_abdh[0]->bdh_bdescs[inbdix0] == BDE_BID_NOTRUMP) {
        outbdescs[outbdlen++] = BDE_RESPOND_NT;
            inbdix0++;
            if (bs == NOTRUMP) {
                outbdescs[outbdlen++] = BDE_BID_NOTRUMP;
                outbdescs[outbdlen++] = bl;
           } else if (bs == CLUBS && bl == bdhl->bdhl_abdh[0]->bdh_bl + 1) {
                outbdescs[outbdlen++] = BDE_BID_CLUBS;
                outbdescs[outbdlen++] = bl;
            } else {
                outbdescs[outbdlen++] = BDE_BID_SUIT;
                outbdescs[outbdlen++] = bl;
            }
        } else if (bdhl->bdhl_abdh[0]->bdh_bdescs[inbdix0] == BDE_BID_SUIT) {
            outbdescs[outbdlen++] = BDE_RESPOND;
            inbdix0++;
            if (bs == NOTRUMP) {
                outbdescs[outbdlen++] = BDE_BID_NOTRUMP;
                outbdescs[outbdlen++] = bl;
            } else if (bs == bdhl->bdhl_abdh[0]->bdh_bs) {
                outbdescs[outbdlen++] = BDE_RAISE;
                outbdescs[outbdlen++] = bl;
            } else if (bl == bdhl->bdhl_abdh[0]->bdh_bl) {
                outbdescs[outbdlen++] = BDE_BID_SUIT;
                outbdescs[outbdlen++] = bl;
            } else if (bl == bdhl->bdhl_abdh[0]->bdh_bl + 1) {
                if (bs < bdhl->bdhl_abdh[0]->bdh_bs) {
                    outbdescs[outbdlen++] = BDE_BID_SUIT;
                    outbdescs[outbdlen++] = bl;
                } else {
                    outbdescs[outbdlen++] = BDE_JUMP_SUIT;
                    outbdescs[outbdlen++] = bl;
                }
            } else if (bl == bdhl->bdhl_abdh[0]->bdh_bl + 2) {
                if (bs < bdhl->bdhl_abdh[0]->bdh_bs) {
                    outbdescs[outbdlen++] = BDE_JUMP_SUIT;
                    outbdescs[outbdlen++] = bl;
                } else {
                    outbdescs[outbdlen++] = BDE_SKIP_SUIT;
                    outbdescs[outbdlen++] = bl;
                }
            } else if (bl == bdhl->bdhl_abdh[0]->bdh_bl + 3) {
                if (bs < bdhl->bdhl_abdh[0]->bdh_bs) {
                    outbdescs[outbdlen++] = BDE_SKIP_SUIT;
                    outbdescs[outbdlen++] = bl;
                }
            }
        }
    }
}
/***************************************************************/
static void bdesc_describe_1(
    int bt,
    int bl,
    int bs,
    unsigned char * outbdescs,
    int enflags)
{
    int outbdlen;

    outbdlen = 0;

    outbdescs[outbdlen++] = BDE_OPEN;
    if (bt == PASS) {
        outbdescs[outbdlen++] = BDE_PASS;
    } else if (bt == BID) {
        if (bs == NOTRUMP) {
            outbdescs[outbdlen++] = BDE_BID_NOTRUMP;
            outbdescs[outbdlen++] = bl;
        } else if (bs == CLUBS && bl == 2) {
            outbdescs[outbdlen++] = BDE_BID_CLUBS;
            outbdescs[outbdlen++] = bl;
        } else {
            outbdescs[outbdlen++] = BDE_BID_SUIT;
            outbdescs[outbdlen++] = bl;
        }
    }
}
/***************************************************************/
/*@*/int increment_bid(int *bl, int *bs)
{
    if ((*bl) < 1 || (*bl) > 7 || ((*bl) == 7 && (*bs) >= NOTRUMP)) {
        (*bl) = 8;  /* Can use bl to determine end */
        (*bs) = CLUBS;
        return (1);
    }

    if ((*bs) < NOTRUMP) {
        (*bs)++;
    } else {
        (*bl)++;
        (*bs) = CLUBS;
    }

    return (0);
}
/***************************************************************/
struct bdeschist * new_bdeschist(
    int bt,
    int bl,
    int bs,
    unsigned char * bdescs,
    int enflags)
{
    struct bdeschist * bdh;

    bdh = New(struct bdeschist, 1);
    bdh->bdh_bt = bt;
    bdh->bdh_bl = bl;
    bdh->bdh_bs = bs;
    bdh->bdh_flags = enflags;
    bdh->bdh_bdhl  = NULL;

    strcpy(bdh->bdh_bdescs, bdescs);

    return (bdh);
}
/***************************************************************/
struct bdeschistlist * new_bdeschistlist(void)
{
    struct bdeschistlist * bdhl;

    bdhl = New(struct bdeschistlist, 1);
    bdhl->bdhl_nbdh = 0;
    bdhl->bdhl_xbdh = 0;
    bdhl->bdhl_abdh = NULL;

    return (bdhl);
}
/***************************************************************/
void add_bdeschistlist(
    struct bdeschistlist * bdhl,
    struct bdeschist     * bdh)
{
    if (bdhl->bdhl_nbdh == bdhl->bdhl_xbdh) {
        if (!bdhl->bdhl_xbdh) bdhl->bdhl_xbdh = 8;
        else bdhl->bdhl_xbdh *= 2;
        bdhl->bdhl_abdh = Realloc(bdhl->bdhl_abdh, struct bdeschist*, bdhl->bdhl_xbdh);
    }

    bdhl->bdhl_abdh[bdhl->bdhl_nbdh] = bdh;
    bdhl->bdhl_nbdh += 1;
}
/***************************************************************/
void free_bdeschist(struct bdeschist * bdh);
void free_bdeschistlist(struct bdeschistlist * bdhl)
{
    int ii;

    for (ii = 0; ii < bdhl->bdhl_nbdh; ii++) {
        free_bdeschist(bdhl->bdhl_abdh[ii]);
    }
    Free(bdhl->bdhl_abdh);
    Free(bdhl);
}
/***************************************************************/
void free_bdeschist(struct bdeschist * bdh)
{
    if (bdh->bdh_bdhl) free_bdeschistlist(bdh->bdh_bdhl);

    Free(bdh);
}
/***************************************************************/
void decode_e_bdescs(unsigned char bditm, char * bdstr)
{
    switch (bditm) {
        case BDE_EOL                : strcpy(bdstr, "EOL");             break; 
        case BDE_OPEN               : strcpy(bdstr, "OPEN");            break; 
        case BDE_RESPOND            : strcpy(bdstr, "RESPOND");         break; 
        case BDE_RESPOND_NT         : strcpy(bdstr, "RESPOND_NT");      break; 
        case BDE_PASS               : strcpy(bdstr, "PASS");            break; 
        case BDE_BID_SUIT           : strcpy(bdstr, "BID_SUIT");        break; 
        case BDE_BID_NOTRUMP        : strcpy(bdstr, "BID_NOTRUMP");     break; 
        case BDE_BID_MINOR          : strcpy(bdstr, "BID_MINOR");       break; 
        case BDE_BID_MAJOR          : strcpy(bdstr, "BID_MAJOR");       break; 
        case BDE_BID_CLUBS          : strcpy(bdstr, "BID_CLUBS");       break; 
        case BDE_BID_DIAMONDS       : strcpy(bdstr, "BID_DIAMONDS");    break; 
        case BDE_BID_HEARTS         : strcpy(bdstr, "BID_HEARTS");      break; 
        case BDE_BID_SPADES         : strcpy(bdstr, "BID_SPADES");      break; 
        case BDE_JUMP_SUIT          : strcpy(bdstr, "JUMP_SUIT");       break; 
        case BDE_SKIP_SUIT          : strcpy(bdstr, "SKIP_SUIT");       break; 
        case BDE_RAISE              : strcpy(bdstr, "RAISE");           break; 
        default                     : strcpy(bdstr, "?UNKNOWN?");       break; 
    }
}
/***************************************************************/
void decode_bdescs(const unsigned char * bdescs, char * bdescs_str)
{
    int bdlen;
    int bdix;

    bdlen = 0;
    bdix  = 0;

    if (bdescs[bdix]) {
        decode_e_bdescs(bdescs[bdix++], bdescs_str + bdlen);
        bdlen += IStrlen(bdescs_str + bdlen);
    }

    while (bdescs[bdix]) {
        bdescs_str[bdlen++] = '_';
        decode_e_bdescs(bdescs[bdix++], bdescs_str + bdlen);
        bdlen += IStrlen(bdescs_str + bdlen);
        if (bdescs[bdix]) {
            bdescs_str[bdlen++] = '_';
            bdescs_str[bdlen++] = '0' + bdescs[bdix++];
        }
    }
    bdescs_str[bdlen] = '\0';
}
/***************************************************************/
void show_bdeschistlist_tab(struct bdeschistlist * bdhl, struct dbtree * bdhl_dbt, int tab)
{
    int ii;
    struct bdeschist * bdh;
    char bidstr[4];
    void * vbdhl;
    int bdescslen;
    char bdescs_str[BDESCS_STR_SZ];
    char tabstr[100];

    for (ii = 0; ii < bdhl->bdhl_nbdh; ii++) {
        memset(tabstr, ' ', tab * 4);
        tabstr[tab * 4] = '\0';

        memset(bidstr, 0, 4);
        bdh = bdhl->bdhl_abdh[ii];
        if (bdh->bdh_bt == PASS) strcpy(bidstr, "P ");
        else if (bdh->bdh_bt == DOUBLE) strcpy(bidstr, "X ");
        else if (bdh->bdh_bt == REDOUBLE) strcpy(bidstr, "XX");
        else if (bdh->bdh_bt == BID) {
            bidstr[0] = '0' + bdh->bdh_bl;
            bidstr[1] = display_suit[bdh->bdh_bs];
        }
        bdescslen = IStrlen(bdh->bdh_bdescs);
        vbdhl = dbtree_find(bdhl_dbt, bdh->bdh_bdescs, bdescslen);
        if (!vbdhl) {
            dbtree_insert(bdhl_dbt, bdh->bdh_bdescs, bdescslen, bdhl);
            decode_bdescs(bdh->bdh_bdescs, bdescs_str);
            printf("%s%s %s\n", tabstr, bidstr, bdescs_str);
        }
        if (bdh->bdh_bdhl) {
            show_bdeschistlist_tab(bdh->bdh_bdhl, bdhl_dbt, tab + 1);
        }
    }
}
/***************************************************************/
void show_bdeschistlist(struct bdeschistlist * bdhl)
{
    struct dbtree * bdhl_dbt; /* dbtree of struct bdeschistlist * */
    bdhl_dbt = dbtree_new();
    show_bdeschistlist_tab(bdhl, bdhl_dbt, 0);
    dbtree_free(bdhl_dbt, NULL);
}
/***************************************************************/
static void svare_enumerate_round_2(struct svarexec * svx,
    int maxbids,
    struct bdeschistlist * bdhl,
    int enflags)
{
    int bl;
    int bs;
    int nbids;
    int ii;
    struct bdeschist * bdh;
    struct bdeschist * bdh2;
    struct bdeschistlist * bdhl2;
    unsigned char bdescs[BDESCS_SZ];

    for (ii = 0; ii < bdhl->bdhl_nbdh; ii++) {
        bdh = bdhl->bdhl_abdh[ii];
        bl = bdh->bdh_bl;
        bs = bdh->bdh_bs;
        nbids = 0;
        bdhl2 = new_bdeschistlist();
        add_bdeschistlist(bdhl2, bdh);
        while (nbids < maxbids) {
            increment_bid(&bl, &bs);
            nbids++;

            memset(bdescs, 0, BDESCS_SZ);
            bdesc_describe_2(BID, bl, bs, bdhl2, bdescs, enflags);
            if (bdescs[0]) {
                bdh2 = new_bdeschist(BID, bl, bs, bdescs, enflags);
                if (!bdh->bdh_bdhl) bdh->bdh_bdhl = new_bdeschistlist();
                add_bdeschistlist(bdh->bdh_bdhl, bdh2);
            }
        }
        Free(bdhl2->bdhl_abdh);
        Free(bdhl2);
    }
}
/***************************************************************/
static void svare_enumerate_round_1(struct svarexec * svx,
    int fbl,
    int fbs,
    int tbl,
    int tbs,
    struct bdeschistlist * bdhl,
    int enflags)
{
    int bl;
    int bs;
    int outbdlen;
    struct bdeschist * bdh;
    unsigned char bdescs[BDESCS_SZ];

    bl = fbl;
    bs = fbs;
    outbdlen = 0;

    while (bl <= tbl) {
        if (bl < tbl || (bl == tbl && bs <= tbs)) {
            memset(bdescs, 0, BDESCS_SZ);
            bdesc_describe_1(BID, bl, bs, bdescs, enflags);
            if (bdescs[0]) {
                bdh = new_bdeschist(BID, bl, bs, bdescs, enflags);
                add_bdeschistlist(bdhl, bdh);
            }
        }
        increment_bid(&bl, &bs);
    }
}
/***************************************************************/
static int svare_Bidding_enumerate(struct svarexec * svx,
    const char * bids,
    int nrounds)
{
    int svstat = 0;
    int ptr;
    int bt;
    int fbl;
    int fbs;
    int tbl;
    int tbs;
    int enflags;
    int is_bid;
    struct omarglobals * og = (struct omarglobals *)(svx->svx_gdata);
    struct bdeschistlist * bdhl;

    enflags = 0;
    ptr = 0;
    while (isspace(bids[ptr])) ptr++;

    is_bid = next_bid(og, bids, &ptr, &bt, &fbl, &fbs);
    if (!is_bid || bt != BID) {
        svstat = svare_set_error(svx, SVARERR_INVALID_ENUMERATE,
            "Invalid enumerate: %s", bids);
    } else {
        is_bid = next_bid(og, bids, &ptr, &bt, &tbl, &tbs);
        if (!is_bid || bt != BID) {
            svstat = svare_set_error(svx, SVARERR_INVALID_ENUMERATE,
                "Invalid enumerate: %s", bids);
        } else {
            if (fbl > tbl ||
                (fbl == tbl && fbs > tbs)) {
                svstat = svare_set_error(svx, SVARERR_INVALID_ENUMERATE,
                    "To bid less than from bid: %s", bids);
            } else {
                bdhl = new_bdeschistlist();
                svare_enumerate_round_1(svx, fbl, fbs, tbl, tbs,
                    bdhl, enflags);
                svare_enumerate_round_2(svx, 10,
                    bdhl, enflags);
                show_bdeschistlist(bdhl);
                free_bdeschistlist(bdhl);
            }
        }
    }

    return (svstat);
}
/***************************************************************/
static int svare_method_Bidding_enumerate(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)
{
    int svstat = 0;
    char * bids;
    int nrounds;

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "<" SVAR_CLASS_NAME_BIDDING ">si");

    if (!svstat) {
        bids = asvv[1]->svv_val.svv_chars.svvc_val_chars;
        nrounds = asvv[2]->svv_val.svv_val_int;
        svstat = svare_Bidding_enumerate(svx, bids, nrounds);
    }

    return (svstat);
}
/***************************************************************/
/**  ConvCard                                                  */
/***************************************************************/
static int svare_method_ConvCard_new(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)
{
    int svstat = 0;
    struct auction_class * auc;

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "<" SVAR_CLASS_NAME_CONVCARD ">");

    if (!svstat) {
        auc = New(struct auction_class, 1);
        auc->auc_auction = new_auction(0, VUL_NONE);
        svare_store_int_object(svx, asvv[0]->svv_val.svv_svnc, rsvv, auc);
    }

    return (svstat);
}
/***************************************************************/
static int svare_method_ConvCard_delete(struct svarexec * svx,   /* NULL for destructor */
    const char * func_name,     /* NULL for destructor */
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)    /* NULL for destructor */
{
    int svstat = 0;
    struct auction_class * auc;

    auc = (struct auction_class *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
    Free(auc->auc_auction);
    Free(auc);

    return (svstat);
}
/***************************************************************/
static int svare_method_ConvCard_toString(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)
{
    int svstat = 0;
    struct auction_class * auc;
    char * austr;
    int aulen;

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "{" SVAR_CLASS_NAME_CONVCARD "}");

    if (!svstat) {
        auc = (struct auction_class *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        austr = auction_to_chars(auc->auc_auction);
        aulen = IStrlen(austr);
        svare_store_string_pointer(rsvv, austr, aulen);
    }

    return (svstat);
}
/***************************************************************/
/**  Hand                                                      */
/***************************************************************/
static struct hand_class * new_hand_class(void)
{
    struct hand_class * hnd;

    hnd = New(struct hand_class, 1);
    hnd->hnd_handinfo = New(struct handinforec, 1);
    hnd->hnd_hand = New(struct hand, 1);

    return (hnd);
}
/***************************************************************/
static int svare_method_parse_hand(struct svarexec * svx,
    struct hand_class * hnd,
    const char * handstr)
{
    int svstat = 0;
    int ok;
    int ptr;
    struct omarglobals * og = (struct omarglobals *)(svx->svx_gdata);

    ptr = 0;
    ok = parse_hand(og, handstr, &ptr, hnd->hnd_hand);
    if (!ok) {
        svstat = omarerr(og, "Invalid hand: %s", handstr);
    }

    return (svstat);
}
/***************************************************************/
static int svare_method_Hand_new(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)
{
    int svstat = 0;
    struct hand_class * hnd;
    char * handstr;

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "<" SVAR_CLASS_NAME_HAND ">s");

    if (!svstat) {
        handstr = asvv[1]->svv_val.svv_chars.svvc_val_chars;
        hnd = new_hand_class();
        svstat = svare_method_parse_hand(svx, hnd, handstr);
        make_handinfo(hnd->hnd_hand, hnd->hnd_handinfo);

        svare_store_int_object(svx, asvv[0]->svv_val.svv_svnc, rsvv, hnd);
    }

    return (svstat);
}
/***************************************************************/
static int svare_method_Hand_delete(struct svarexec * svx,   /* NULL for destructor */
    const char * func_name,     /* NULL for destructor */
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)    /* NULL for destructor */
{
    int svstat = 0;
    struct hand_class * hnd;

    hnd = (struct hand_class *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
    Free(hnd->hnd_handinfo);
    Free(hnd->hnd_hand);
    Free(hnd);

    return (svstat);
}
/***************************************************************/
static int svare_method_Hand_toString(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)
{
    int svstat = 0;
    struct hand_class * hnd;
    char * hndstr;
    int hndlen;

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "{" SVAR_CLASS_NAME_HAND "}");

    if (!svstat) {
        hnd = (struct hand_class *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        hndstr = hand_to_chars(hnd->hnd_hand);
        hndlen = IStrlen(hndstr);
        svare_store_string_pointer(rsvv, hndstr, hndlen);
    }

    return (svstat);
}
/***************************************************************/
/**  Hands                                                     */
/***************************************************************/
static struct hands_class * new_hands_class(void)
{
    struct hands_class * hns;

    hns = New(struct hands_class, 1);
    hns->hns_hand = New(struct hand, 4);
    hns->hns_handinfo = New(struct handinforec, 4);
    hns->hns_hand_mask = 0;

    return (hns);
}
/***************************************************************/
static int svare_method_Hands_new(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)
{
    int svstat = 0;
    struct hands_class * hns;

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "<" SVAR_CLASS_NAME_HANDS ">");

    if (!svstat) {
        hns = new_hands_class();

        svare_store_int_object(svx, asvv[0]->svv_val.svv_svnc, rsvv, hns);
    }

    return (svstat);
}
/***************************************************************/
static int svare_method_Hands_delete(struct svarexec * svx,   /* NULL for destructor */
    const char * func_name,     /* NULL for destructor */
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)    /* NULL for destructor */
{
    int svstat = 0;
    struct hands_class * hns;

    hns = (struct hands_class *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
    Free(hns->hns_hand);
    Free(hns->hns_handinfo);
    Free(hns);

    return (svstat);
}
/***************************************************************/
static int svare_method_Hands_toString(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)
{
    int svstat = 0;
    struct hands_class * hns;
    char * hnsstr;
    int hnslen;

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "{" SVAR_CLASS_NAME_HANDS "}");

    if (!svstat) {
        hns = (struct hands_class *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        if (hns->hns_hand_mask != 15) {
            hnsstr = Strdup("Not all hands specified.");
        } else {
            hnsstr = hands_to_chars((HANDS*)hns->hns_hand);
        }
        hnslen = IStrlen(hnsstr);
        svare_store_string_pointer(rsvv, hnsstr, hnslen);
    }

    return (svstat);
}
/***************************************************************/
static int svare_method_Hands_getHand(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)
{
/*
**
*/
    int svstat = 0;
    struct hands_class * hns;
    struct hand_class * hnd;
    char * plyrstr;
    int plyr;
    struct svarvalue * csvv;
    struct omarglobals * og = (struct omarglobals *)(svx->svx_gdata);

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "{" SVAR_CLASS_NAME_HANDS "}s");

    if (!svstat) {
        hns = (struct hands_class *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        plyrstr = asvv[1]->svv_val.svv_chars.svvc_val_chars;
        plyr = char_to_player(plyrstr[0]);
        if (plyr < 0) {
            svstat = omarerr(og, "Invalid player: %s", plyrstr);
        } else if (!(hns->hns_hand_mask & (1 << plyr))) {
            svstat = omarerr(og, "No hand for player: %s", plyrstr);
        } else {
            csvv = svare_find_class(svx, SVAR_CLASS_NAME_HAND);
            if (!csvv) {
                svstat = svare_set_error(svx, SVARERR_UNDEFINED_CLASS,
                    "Undefined class: %s", SVAR_CLASS_NAME_HAND);
            } else {
                hnd = new_hand_class();
                copy_hand(hnd->hnd_hand, &((hns->hns_hand)[plyr]));
                make_handinfo(hnd->hnd_hand, hnd->hnd_handinfo);
                svare_store_int_object(svx, csvv->svv_val.svv_svnc, rsvv, hnd);
            }
        }
    }

    return (svstat);
}
/***************************************************************/
static void svare_HRL_makeHands(struct svarexec * svx,
    struct hands_class * hns,
        struct handrec * handrec)
{
    int ii;

    for (ii = 0; ii < 4; ii++) {
        copy_hand(&((hns->hns_hand)[ii]), &((handrec->hr_hands)[ii]));
        make_handinfo(&((hns->hns_hand)[ii]), &((hns->hns_handinfo)[ii]));
        hns->hns_hand_mask |= 1 << ii;
    }
}
/***************************************************************/
static int svare_method_HRL_HRLmakeHands(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)
{
    int svstat = 0;
    int estat = 0;
    struct handreclist * hrl;
    struct hands_class * hns;
    struct svarvalue * csvv;
    int hrlix;

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "{" SVAR_CLASS_NAME_HAND_RECORD_LIST "}i");

    if (!svstat) {
        hrl = (struct handreclist *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        hrlix = asvv[1]->svv_val.svv_val_int - 1;

        hns = new_hands_class();
        csvv = svare_find_class(svx, SVAR_CLASS_NAME_HANDS);
        if (!csvv) {
            svstat = svare_set_error(svx, SVARERR_UNDEFINED_CLASS,
                "Undefined class: %s", SVAR_CLASS_NAME_HANDS);
        } else {
            if (hrlix < 0 || hrlix >= hrl->hrl_nhandrec) {
                svstat = svare_set_error(svx, SVARERR_FUNCTION_FAILED,
                    "Function %s(%d) index is out of range.",
                    func_name, hrlix);
            } else {
                svare_HRL_makeHands(svx, hns, hrl->hrl_ahandrec[hrlix]);
                svare_store_int_object(svx, csvv->svv_val.svv_svnc, rsvv, hns);
            }
        }
    }

    return (svstat);
}
/***************************************************************/
static int svare_method_HRL_HRLgetDealer(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)
{
    int svstat = 0;
    int estat = 0;
    struct handreclist * hrl;
    int hrlix;
    int dlrlen;
    char dealer[8];

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "{" SVAR_CLASS_NAME_HAND_RECORD_LIST "}i");

    if (!svstat) {
        hrl = (struct handreclist *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        hrlix = asvv[1]->svv_val.svv_val_int - 1;

        if (hrlix < 0 || hrlix >= hrl->hrl_nhandrec) {
            svstat = svare_set_error(svx, SVARERR_FUNCTION_FAILED,
                "Function %s(%d) index is out of range.",
                func_name, hrlix);
        } else {
            dealer[0] = display_player[hrl->hrl_ahandrec[hrlix]->hr_dealer];
            dealer[1] = '\0';
            dlrlen = 1;
            svare_store_string(rsvv, dealer, dlrlen);
        }
    }

    return (svstat);
}
/***************************************************************/
static int svare_method_HRL_HRLgetVulnerability(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)
{
    int svstat = 0;
    int estat = 0;
    struct handreclist * hrl;
    int hrlix;
    int vullen;
    char vulstr[8];

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "{" SVAR_CLASS_NAME_HAND_RECORD_LIST "}i");

    if (!svstat) {
        hrl = (struct handreclist *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        hrlix = asvv[1]->svv_val.svv_val_int - 1;

        if (hrlix < 0 || hrlix >= hrl->hrl_nhandrec) {
            svstat = svare_set_error(svx, SVARERR_FUNCTION_FAILED,
                "Function %s(%d) index is out of range.",
                func_name, hrlix);
        } else {
            get_vulnerability(vulstr, hrl->hrl_ahandrec[hrlix]->hr_vulnerability);
            vullen = IStrlen(vulstr);
            svare_store_string(rsvv, vulstr, vullen);
        }
    }

    return (svstat);
}
/***************************************************************/
/* class setup                                                 */        
/***************************************************************/
struct svarvalue * svare_new_int_class(struct svarexec * svx, int class_id)
{
    struct svarvalue * svv;

    svv = svare_new_variable();
    svv->svv_dtype = SVV_DTYPE_INT_CLASS;
    svv->svv_val.svv_svnc = New(struct svarvalue_int_class, 1);
    svv->svv_val.svv_svnc->svnc_svrec = svar_new();
    svv->svv_val.svv_svnc->svnc_nrefs = 0;
    svv->svv_val.svv_svnc->svnc_class_id = class_id;

    return (svv);
}
/***************************************************************/
void svare_new_int_class_method(struct svarvalue * svv,
    const char * func_name,
    SVAR_INTERNAL_FUNCTION ifuncptr )

{
    int is_dup;
    int fnlen;
    struct svarvalue * fsvv;

    fsvv = svare_new_variable();
    fsvv->svv_dtype = SVV_DTYPE_METHOD;
    fsvv->svv_val.svv_method = ifuncptr;
    fnlen = IStrlen(func_name) + 1;

    is_dup = dbtree_insert(svv->svv_val.svv_svnc->svnc_svrec->svar_svarvalue_dbt,
        func_name, fnlen, (void*)fsvv);
}
/***************************************************************/
int svare_add_int_class(struct svarexec * svx,
    const char * vnam,
    struct svarvalue * svv)
{
    int svstat = 0;
    int is_dup;

    if (svx->svx_nvars) {
        svv->svv_val.svv_svnc->svnc_nrefs += 1;
        is_dup = dbtree_insert(svx->svx_avars[svx->svx_nvars-1]->svar_svarvalue_dbt,
            vnam, IStrlen(vnam) + 1, svv);
            if (is_dup) {
                svstat = svare_set_error(svx, SVARERR_DUPLICATE_INT_CLASS,
                    "Duplicate internal class: %s", vnam);
            }
    }

    return (svstat);
}
/***************************************************************/
void svare_init_internal_classes(struct svarexec * svx)
{
    struct svarvalue * svv;

    /* NeuralNet */
    svv = svare_new_int_class(svx, SVAR_CLASS_ID_NEURAL_NET);
    svare_new_int_class_method(svv, ".new"              , svare_method_NN_new);                 /* static */
    svare_new_int_class_method(svv, ".delete"           , svare_method_NN_delete);
    svare_new_int_class_method(svv, "NNConfig"          , svare_method_NN_Config);
    svare_new_int_class_method(svv, "NNGetTotalError"   , svare_method_NN_GetTotalError);
    svare_new_int_class_method(svv, "NNGetTotalEpochs"  , svare_method_NN_GetTotalEpochs);
    svare_new_int_class_method(svv, "NNElapsedTime"     , svare_method_NN_ElapsedTime);
    svare_new_int_class_method(svv, "NNLoad"            , svare_method_NN_Load);                /* static */
    svare_new_int_class_method(svv, "NNSave"            , svare_method_NN_Save);
    svare_add_int_class(svx, SVAR_CLASS_NAME_NEURAL_NET, svv);

    /* HandRecordList */
    svv = svare_new_int_class(svx, SVAR_CLASS_ID_HAND_RECORD_LIST);
    svare_new_int_class_method(svv, ".new"              , svare_method_HRL_new);                /* static */
    svare_new_int_class_method(svv, ".delete"           , svare_method_HRL_delete);
    svare_new_int_class_method(svv, "HRLReadFile"       , svare_method_HRL_ReadFile);
    svare_new_int_class_method(svv, "HRLReadDirectory"  , svare_method_HRL_ReadDirectory);
    svare_new_int_class_method(svv, "HRLTrainNN"        , svare_method_HRL_TrainNN);
    svare_new_int_class_method(svv, "HRLPlayNN"         , svare_method_HRL_PlayNN);
    svare_new_int_class_method(svv, "HRLGetSize"        , svare_method_HRL_GetSize);
    svare_new_int_class_method(svv, "HRLmakeHands"      , svare_method_HRL_HRLmakeHands);
    svare_new_int_class_method(svv, "HRLgetDealer"      , svare_method_HRL_HRLgetDealer);
    svare_new_int_class_method(svv, "HRLgetVulnerability",svare_method_HRL_HRLgetVulnerability);
    svare_new_int_class_method(svv, "HRLGetHandsPlayed" , svare_method_HRL_GetHandsPlayed);
    svare_new_int_class_method(svv, "HRLGetHandsPlayedCorrect", svare_method_HRL_GetHandsPlayedCorrect);
    svare_new_int_class_method(svv, "HRLGetHandsPlayedNear"   , svare_method_HRL_GetHandsPlayedNear);
    svare_new_int_class_method(svv, "HRLGetHandsPlayedPlus1"  , svare_method_HRL_GetHandsPlayedPlus1);
    svare_new_int_class_method(svv, "HRLGetHandsPlayedMinus1" , svare_method_HRL_GetHandsPlayedMinus1);
    svare_add_int_class(svx, SVAR_CLASS_NAME_HAND_RECORD_LIST, svv);

    /* Integer */
    svv = svare_new_int_class(svx, SVAR_CLASS_ID_INTEGER);
    svare_new_int_class_method(svv, ".new"              , svare_method_Integer_new);            /* static */
    svare_new_int_class_method(svv, ".delete"           , svare_method_Integer_delete);
    svare_new_int_class_method(svv, "toString"          , svare_method_Integer_toString);
    svare_new_int_class_method(svv, "get"               , svare_method_Integer_Get);
    svare_new_int_class_method(svv, "set"               , svare_method_Integer_Set);
    svare_add_int_class(svx, SVAR_CLASS_NAME_INTEGER, svv);

#if IS_TESTING_CLASS
    /* Testing */
    svv = svare_new_int_class(svx, SVAR_CLASS_ID_TESTING);
    svare_new_int_class_method(svv, ".new"              , svare_method_TestingNew);             /* static */
    svare_new_int_class_method(svv, "New"               , svare_method_Testing_new);
    svare_new_int_class_method(svv, ".delete"           , svare_method_TestingDelete);
    svare_new_int_class_method(svv, "toString"          , svare_method_TestingToString);
    svare_new_int_class_method(svv, "get"               , svare_method_TestingGet);
    svare_new_int_class_method(svv, "set"               , svare_method_TestingSet);
    svare_add_int_class(svx, SVAR_CLASS_NAME_TESTING, svv);
#endif

    /* Auction */
    svv = svare_new_int_class(svx, SVAR_CLASS_ID_AUCTION);
    svare_new_int_class_method(svv, ".new"              , svare_method_Auction_new);            /* static */
    svare_new_int_class_method(svv, ".delete"           , svare_method_Auction_delete);
    svare_new_int_class_method(svv, "toString"          , svare_method_Auction_toString);
    svare_new_int_class_method(svv, "makeBid"           , svare_method_Auction_makeBid);
    svare_new_int_class_method(svv, "setDealer"         , svare_method_Auction_setDealer);
    svare_new_int_class_method(svv, "setVulnerability"  , svare_method_Auction_setVulnerability);
    svare_new_int_class_method(svv, "getNextBidder"     , svare_method_Auction_getNextBidder);
    svare_add_int_class(svx, SVAR_CLASS_NAME_AUCTION, svv);

    /* Analysis */
    svv = svare_new_int_class(svx, SVAR_CLASS_ID_ANALYSIS);
    svare_new_int_class_method(svv, ".new"              , svare_method_Analysis_new);           /* static */
    svare_new_int_class_method(svv, ".delete"           , svare_method_Analysis_delete);
    svare_new_int_class_method(svv, "toString"          , svare_method_Analysis_toString);
    svare_new_int_class_method(svv, "testFile"          , svare_method_Analysis_testFile);
    svare_new_int_class_method(svv, "printAnalysis"     , svare_method_Analysis_printAnalysis);
    /* svare_new_int_class_method(svv, "reset"             , svare_method_Analysis_reset); */
    svare_add_int_class(svx, SVAR_CLASS_NAME_ANALYSIS, svv);

    /* Bidding */
    svv = svare_new_int_class(svx, SVAR_CLASS_ID_BIDDING);
    svare_new_int_class_method(svv, "findBestBid"        , svare_method_Bidding_findBestBid);       /* static */
    svare_new_int_class_method(svv, "showNaturalBids"    , svare_method_Bidding_showNaturalBids);   /* static */
    svare_new_int_class_method(svv, "enumerate"          , svare_method_Bidding_enumerate);   /* static */
    svare_add_int_class(svx, SVAR_CLASS_NAME_BIDDING, svv);

#if 0
    /* Bid */
    svv = svare_new_int_class(svx, SVAR_CLASS_ID_BID);
    svare_new_int_class_method(svv, ".new"              , svare_method_Bid_new);                /* static */
    svare_new_int_class_method(svv, ".delete"           , svare_method_Bid_delete);
    svare_new_int_class_method(svv, "toString"          , svare_method_Bid_toString);
    svare_add_int_class(svx, SVAR_CLASS_NAME_BID, svv);
#endif

    /* ConvCard */
    svv = svare_new_int_class(svx, SVAR_CLASS_ID_CONVCARD);
    svare_new_int_class_method(svv, ".new"              , svare_method_ConvCard_new);           /* static */
    svare_new_int_class_method(svv, ".delete"           , svare_method_ConvCard_delete);
    svare_new_int_class_method(svv, "toString"          , svare_method_ConvCard_toString);
    svare_add_int_class(svx, SVAR_CLASS_NAME_CONVCARD, svv);

    /* Hand */
    svv = svare_new_int_class(svx, SVAR_CLASS_ID_HAND);
    svare_new_int_class_method(svv, ".new"              , svare_method_Hand_new);               /* static */
    svare_new_int_class_method(svv, ".delete"           , svare_method_Hand_delete);
    svare_new_int_class_method(svv, "toString"          , svare_method_Hand_toString);
    svare_add_int_class(svx, SVAR_CLASS_NAME_HAND, svv);

    /* Hands */
    svv = svare_new_int_class(svx, SVAR_CLASS_ID_HANDS);
    svare_new_int_class_method(svv, ".new"              , svare_method_Hands_new);              /* static */
    svare_new_int_class_method(svv, ".delete"           , svare_method_Hands_delete);
    svare_new_int_class_method(svv, "toString"          , svare_method_Hands_toString);
    svare_new_int_class_method(svv, "getHand"           , svare_method_Hands_getHand);
    svare_add_int_class(svx, SVAR_CLASS_NAME_HANDS, svv);

    svare_init_internal_thread_class(svx);
    svare_init_internal_pbn_class(svx);
}
/***************************************************************/
void svare_init_internal_functions(struct svarexec * svx)
{
    int svstat = 0;

    svstat = svar_add_function(svx, "defined"    , svare_func_defined);
}
/***************************************************************/
