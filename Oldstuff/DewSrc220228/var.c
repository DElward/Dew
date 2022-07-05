/***************************************************************/
/*
**  var.c --  Variable classes
*/
/***************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>
#include <math.h>

#include "dew.h"
#include "jsenv.h"
#include "error.h"
#include "jstok.h"
#include "jsexp.h"
#include "dbtreeh.h"
#include "snprfh.h"
#include "utl.h"
#include "var.h"
#include "array.h"
#include "obj.h"
#include "prep.h"

#define DEBUG_FREE_VARS 0

/***************************************************************/
void jvar_init_jvarvalue(struct jvarvalue * jvv)
{
#if IS_DEBUG
    static int next_jvv_sn = 0;
#endif

#if IS_DEBUG
    jvv->jvv_sn[0] = 'V';
    jvv->jvv_sn[1] = (next_jvv_sn / 100) % 10 + '0';
    jvv->jvv_sn[2] = (next_jvv_sn /  10) % 10 + '0';
    jvv->jvv_sn[3] = (next_jvv_sn      ) % 10 + '0';
    next_jvv_sn++;
#endif
    /* jvv->jvv_flags = 0; */

    INIT_JVARVALUE(jvv);
}
/***************************************************************/
struct jvarvalue * jvar_new_jvarvalue(void)
{
    struct jvarvalue * jvv;

    jvv = New(struct jvarvalue, 1);

    jvar_init_jvarvalue(jvv);

    return (jvv);
}
/***************************************************************/
struct jvarrec * jvar_new_jvarrec(void)
{
    struct jvarrec * jvar;
#if IS_DEBUG
    static int next_jvar_sn = 0;
#endif

    jvar = New(struct jvarrec, 1);
#if IS_DEBUG
    jvar->jvar_sn[0] = 'A';
    jvar->jvar_sn[1] = (next_jvar_sn / 100) % 10 + '0';
    jvar->jvar_sn[2] = (next_jvar_sn /  10) % 10 + '0';
    jvar->jvar_sn[3] = (next_jvar_sn      ) % 10 + '0';
    next_jvar_sn++;
#endif

    jvar->jvar_jvarvalue_dbt    = dbtsi_new();
    /*jvar->jvar_prev             = NULL;*/
    jvar->jvar_nRefs            = 0;
    jvar->jvar_nvars            = 0;
    jvar->jvar_nconsts          = 0;
    jvar->jvar_xconsts          = 0;
    jvar->jvar_avix             = NULL;
    jvar->jvar_aconsts          = NULL;

    return (jvar);
}
/***************************************************************/
#if 0
void jvar_set_jvvflags(struct jvarvalue * jvv, int jvvflags)
{
/*
** 09/23/2021
*/
    jvv->jvv_flags = jvvflags;
}
#endif
/***************************************************************/
int * jvar_find_in_jvarrec(struct jvarrec * jvar, const char * varname)
{
/*
** 03/03/2021
*/

    int * pvix;

    pvix = dbtsi_find(jvar->jvar_jvarvalue_dbt, varname);

    return (pvix);
}
/***************************************************************/
static void jvar_new_local_const(struct jvarrec * jvar, struct jvarvalue * jvv, int vix)
{
/*
** 03/03/2021
*/
    if (jvar->jvar_nconsts == jvar->jvar_xconsts) {
        if (!jvar->jvar_nconsts) jvar->jvar_xconsts = 2;
        else                     jvar->jvar_xconsts *= 2;
        jvar->jvar_avix =
            Realloc(jvar->jvar_avix, int, jvar->jvar_xconsts);
        jvar->jvar_aconsts =
            Realloc(jvar->jvar_aconsts, struct jvarvalue, jvar->jvar_xconsts);
    }
    jvar->jvar_avix[jvar->jvar_nconsts] = vix;
    COPY_JVARVALUE(jvar->jvar_aconsts + jvar->jvar_nconsts, jvv);
    jvar->jvar_nconsts += 1;
}
/***************************************************************/
#if 0
int jvar_is_const_jvv(struct jvarvalue * jvv)
{
/*
** 03/05/2021
*/

    int is_const;

    is_const = 0;
    if (jvv) {
        if (jvv->jvv_dtype == JVV_DTYPE_INTERNAL_METHOD) {
            is_const = 1;
        }
    }

    return (is_const);
}
#endif
/***************************************************************/
int jvar_insert_into_jvarrec(struct jvarrec * jvar, const char * varname)
{
/*
** 03/05/2021
*/

    int is_dup;
    int vix;

    vix = jvar->jvar_nvars;

    is_dup = dbtsi_insert(jvar->jvar_jvarvalue_dbt, varname, vix);
    if (is_dup) {
        vix = VINVALID;
    } else {
        jvar->jvar_nvars += 1;
    }
#if 0 && IS_DEBUG
    if (vix < 0) {
        printf("**DEBUG** Duplicate variable name: %s\n", varname);
    }
#endif

    return (vix);
}
/***************************************************************/
char * jvar_get_dtype_name(int dtype)
{
/*
**
*/
    char * out;

    switch (dtype) {
        case JVV_DTYPE_NONE            : out = "None";              break;
        case JVV_DTYPE_BOOL            : out = "Bool";              break;
        case JVV_DTYPE_JSINT           : out = "JSInt";             break;
        case JVV_DTYPE_JSFLOAT         : out = "JSFloat";           break;
        case JVV_DTYPE_CHARS           : out = "Chars";             break;
        case JVV_DTYPE_NUMBER          : out = "Number";            break;
        case JVV_DTYPE_NULL            : out = "Null";              break;
        case JVV_DTYPE_LVAL            : out = "LVal";              break;
        case JVV_DTYPE_INTERNAL_CLASS  : out = "InternalClass";     break;
        case JVV_DTYPE_INTERNAL_METHOD : out = "InternalMethod";    break;
        case JVV_DTYPE_INTERNAL_OBJECT : out = "InternalObject";    break;
        case JVV_DTYPE_VALLIST         : out = "ValueList";         break;
        case JVV_DTYPE_TOKEN           : out = "Token";             break;
        case JVV_DTYPE_FUNCTION        : out = "Function";          break;
        case JVV_DTYPE_FUNCVAR         : out = "FuncVar";           break;
        case JVV_DTYPE_ARRAY           : out = "Array";             break;
        case JVV_DTYPE_IMETHVAR        : out = "IMethVar";          break;
        case JVV_DTYPE_OBJECT          : out = "Object";            break;
        case JVV_DTYPE_OBJPTR          : out = "ObjPtr";            break;
        case JVV_DTYPE_DYNAMIC         : out = "Dynamic";           break;
        default                        : out = "*Undefined*";       break;
    }

    return (out);
}
/***************************************************************/
void jvar_free_jvarvalue_internal_class(struct jvarvalue_internal_class * jvvi)
{
/*
**
*/
    DECICLASSHREFS(jvvi);
    if (jvvi->jvvi_nRefs == 0) {
        if (jvvi->jvvi_superclass.jvv_dtype == JVV_DTYPE_INTERNAL_CLASS) {
            jvar_free_jvarvalue_data(&(jvvi->jvvi_superclass));
        }
        Free(jvvi->jvvi_class_name);
        /* jvar_free_jvarrec_consts(jvvi->jvvi_jvar); */
        jvar_free_jvarrec(jvvi->jvvi_jvar);
        Free(jvvi);
    }
}
/***************************************************************/
void jvar_free_jvarvalue_internal_method(struct jvarvalue_int_method * jvvim)
{
/*
** 09/30/2021
*/
    DECIMETHREFS(jvvim);
    if (jvvim->jvvim_nRefs == 0) {
        Free(jvvim->jvvim_method_name);
        Free(jvvim);
    }
}
/***************************************************************/
void jvar_free_jvarvalue_int_object(struct jvarvalue_int_object * jvvo)
{
/*
**
*/
    DECIOBJREFS(jvvo);
    if (jvvo->jvvo_nRefs == 0) {
        if (jvvo->jvvo_superclass.jvv_dtype != JVV_DTYPE_NONE) {
            jvar_free_jvarvalue_data(&(jvvo->jvvo_superclass));
        }

        if (jvvo->jvvo_this_ptr && jvvo->jvvo_class.jvv_dtype == JVV_DTYPE_INTERNAL_CLASS) {
            if (jvvo->jvvo_class.jvv_val.jvv_jvvi->jvvi_destructor) {
                (jvvo->jvvo_class.jvv_val.jvv_jvvi->jvvi_destructor)(jvvo->jvvo_this_ptr);
            }
        }

        jvar_free_jcontext(jvvo->jvvo_jcx);
        jvar_free_jvarvalue_data(&(jvvo->jvvo_class));
        Free(jvvo);
    }
}
/***************************************************************/
static void jvvl_free_jvarvalue_vallist(struct jvarvalue_vallist * jvvlargs)
{
/*
**
*/
    int ii;

    for (ii = jvvlargs->jvvl_nvals - 1; ii >= 0; ii--) {
        jvar_free_jvarvalue_data(&(jvvlargs->jvvl_avals[ii]));
    }
    Free(jvvlargs->jvvl_avals);
    Free(jvvlargs);
}
/***************************************************************/
void jvar_free_jvarvalue_data(struct jvarvalue * jvv)
{
/*
**
*/
    if (!jvv) return;

    switch (jvv->jvv_dtype) {
        case JVV_DTYPE_NONE  :
        case JVV_DTYPE_BOOL  :
        case JVV_DTYPE_JSINT   :
        case JVV_DTYPE_JSFLOAT:
        case JVV_DTYPE_LVAL :
            jvv->jvv_dtype = JVV_DTYPE_NONE;
            break;

        case JVV_DTYPE_CHARS :
            Free(jvv->jvv_val.jvv_val_chars.jvvc_val_chars);
            jvv->jvv_dtype = JVV_DTYPE_NONE;
            break;

        case JVV_DTYPE_INTERNAL_CLASS :
            jvar_free_jvarvalue_internal_class(jvv->jvv_val.jvv_jvvi);
            jvv->jvv_dtype = JVV_DTYPE_NONE;
            break;

        case JVV_DTYPE_INTERNAL_OBJECT :
            jvar_free_jvarvalue_int_object(jvv->jvv_val.jvv_jvvo);
            break;

        case JVV_DTYPE_INTERNAL_METHOD:
            jvar_free_jvarvalue_internal_method(jvv->jvv_val.jvv_int_method);
            jvv->jvv_dtype = JVV_DTYPE_NONE;
            break;

        case JVV_DTYPE_TOKEN :
            Free(jvv->jvv_val.jvv_val_token.jvvt_token);
            jvv->jvv_dtype = JVV_DTYPE_NONE;
            break;

        case JVV_DTYPE_VALLIST:
            jvvl_free_jvarvalue_vallist(jvv->jvv_val.jvv_jvvl);
            jvv->jvv_dtype = JVV_DTYPE_NONE;
            break;

        case JVV_DTYPE_FUNCTION:
            jvar_free_jvarvalue_function(jvv->jvv_val.jvv_val_function);
            jvv->jvv_dtype = JVV_DTYPE_NONE;
            break;

        case JVV_DTYPE_FUNCVAR:
            jvar_free_jvarvalue_function(jvv->jvv_val.jvv_val_funcvar.jvvfv_jvvf);
            jvv->jvv_val.jvv_val_funcvar.jvvfv_jvvf = NULL;
            jvv->jvv_dtype = JVV_DTYPE_NONE;
            break;
        
        case JVV_DTYPE_IMETHVAR:
            jvar_free_jvarvalue_internal_method(jvv->jvv_val.jvv_val_imethvar.jvvimv_jvvim);
            jvv->jvv_val.jvv_val_funcvar.jvvfv_jvvf = NULL;
            jvv->jvv_dtype = JVV_DTYPE_NONE;
            break;

        case JVV_DTYPE_ARRAY :
            jvar_free_jvarvalue_array(jvv->jvv_val.jvv_val_array);
            jvv->jvv_val.jvv_val_array = NULL;
            jvv->jvv_dtype = JVV_DTYPE_NONE;
            break;
        
        case JVV_DTYPE_OBJPTR:
#if USE_OLD_JVARVALUE_OBJPTR
            switch (jvv->jvv_val.jvv_objptr.jvvp_dtype) {
                case JVV_DTYPE_ARRAY   :
                    jvar_free_jvarvalue_array(jvv->jvv_val.jvv_objptr.jvvp_val.jvvp_val_array);
                    break;
                default:
                    break;
            }
#else
            switch (jvv->jvv_val.jvv_objptr.jvvp_pval->jvv_dtype) {
                case JVV_DTYPE_ARRAY   :
                    jvar_free_jvarvalue(jvv->jvv_val.jvv_objptr.jvvp_pval);
                    break;
                default:
                    break;
            }
#endif
            jvv->jvv_dtype = JVV_DTYPE_NONE;
            break;

        case JVV_DTYPE_DYNAMIC :
            free_jvarvalue_dynamic(jvv->jvv_val.jvv_val_dynamic);
            jvv->jvv_dtype = JVV_DTYPE_NONE;
            break;

        default:
            printf("jvar_free_jvarvalue_data(%s)\n",
                jvar_get_dtype_name(jvv->jvv_dtype));
            break;
    }
}
/***************************************************************/
void jvar_free_jvarvalue(struct jvarvalue * jvv)
{
/*
** 02/19/2021
*/
    jvar_free_jvarvalue_data(jvv);
    Free(jvv);
}
/***************************************************************/
void jvar_store_null(struct jvarvalue * jvv)
{
/*
**
*/
    jvar_free_jvarvalue_data(jvv);
    jvv->jvv_dtype = JVV_DTYPE_NULL;
}
/***************************************************************/
void jvar_store_bool(
    struct jvarvalue * jvv,
    int val_jsbool)
{
/*
**
*/
    jvar_free_jvarvalue_data(jvv);
    jvv->jvv_dtype = JVV_DTYPE_BOOL;
    jvv->jvv_val.jvv_val_bool = val_jsbool?1:0;
}
/***************************************************************/
void jvar_store_jsint(
    struct jvarvalue * jvv,
    JSINT val_jsint)
{
/*
**
*/
    jvar_free_jvarvalue_data(jvv);
    jvv->jvv_dtype = JVV_DTYPE_JSINT;

    jvv->jvv_val.jvv_val_jsint = val_jsint;
}
/***************************************************************/
void jvar_store_jsfloat(
    struct jvarvalue * jvv,
    JSFLOAT val_jsfloat)
{
/*
**
*/
    jvar_free_jvarvalue_data(jvv);
    jvv->jvv_dtype = JVV_DTYPE_JSFLOAT;
    jvv->jvv_val.jvv_val_jsfloat = val_jsfloat;
}
/***************************************************************/
int jvar_store_int_object(
    struct jrunexec * jx,
    struct jvarvalue * jvvtgt,
    struct jvarvalue * jvvsrc)
{
/*
** 09/15/2021
*/
    int jstat = 0;

    jvar_free_jvarvalue_data(jvvtgt);

    jvvtgt->jvv_dtype = JVV_DTYPE_INTERNAL_OBJECT;
    jvvtgt->jvv_val.jvv_jvvo = jvvsrc->jvv_val.jvv_jvvo;
    INCIOBJREFS(jvvsrc->jvv_val.jvv_jvvo);

    return (jstat);
}
/***************************************************************/
void jvar_store_function(
    struct jvarvalue * jvvtgt,
    struct jvarvalue * jvvsrc)
{
/*
** 08/13/2021
*/
    jvar_free_jvarvalue_data(jvvtgt);

    jvvtgt->jvv_dtype = JVV_DTYPE_FUNCTION;
    jvvtgt->jvv_val.jvv_val_function = jvvsrc->jvv_val.jvv_val_function;

    jvvsrc->jvv_dtype = JVV_DTYPE_NONE;
    jvvsrc->jvv_val.jvv_val_function = NULL;
}
/***************************************************************/
#if ! USE_OLD_JVARVALUE_OBJPTR
static struct jvarvalue_objptr * job_new_jvarvalue_objptr(struct jvarvalue * jvv)
{
/*
** 02/03/2022
*/
    struct jvarvalue_objptr * jvvp;

    jvvp = New(struct jvarvalue_objptr, 1);

    return (jvvp);
}
/***************************************************************/
void job_free_jvarvalue_objptr(struct jvarvalue_objptr * jvvp)
{
    Free(jvvp);
}
#endif
/***************************************************************/
int jvar_store_jvarvalue(
    struct jrunexec * jx,
    struct jvarvalue * jvvtgt,
    struct jvarvalue * jvvsrc)
{
/*
**
    JVV_DTYPE_BOOL              ,
    JVV_DTYPE_JSINT             ,
    JVV_DTYPE_JSFLOAT           ,
    JVV_DTYPE_CHARS             ,
    JVV_DTYPE_NUMBER            ,
    JVV_DTYPE_NULL              ,
    JVV_DTYPE_LVAL              ,
    JVV_DTYPE_INTERNAL_CLASS    ,
    JVV_DTYPE_INTERNAL_METHOD   ,
    JVV_DTYPE_OBJECT            ,
*/
    int jstat = 0;
    struct jvarvalue * jvv;

    jstat = jexp_eval_rval(jx,jvvsrc, &jvv);
    if (jstat) return (jstat);

    switch (jvv->jvv_dtype) {
        case JVV_DTYPE_NONE:
            jvar_free_jvarvalue_data(jvvtgt);
            jvv->jvv_dtype = JVV_DTYPE_NONE;
            break;
        case JVV_DTYPE_NULL   :
            jvar_store_null(jvvtgt);
            break;
        case JVV_DTYPE_BOOL   :
            jvar_store_bool(jvvtgt, jvv->jvv_val.jvv_val_bool);
            break;
        case JVV_DTYPE_JSINT   :
            jvar_store_jsint(jvvtgt, jvv->jvv_val.jvv_val_jsint);
            break;
        case JVV_DTYPE_JSFLOAT   :
            jvar_store_jsfloat(jvvtgt, jvv->jvv_val.jvv_val_jsfloat);
            break;
        case JVV_DTYPE_CHARS   :
            jvar_store_chars_len(jvvtgt,
                jvv->jvv_val.jvv_val_chars.jvvc_val_chars,
                jvv->jvv_val.jvv_val_chars.jvvc_length);
            break;
        case JVV_DTYPE_INTERNAL_METHOD   :
            jvvtgt->jvv_dtype = jvv->jvv_dtype;
            jvvtgt->jvv_val.jvv_int_method = jvv->jvv_val.jvv_int_method;
            INCIMETHREFS(jvv->jvv_val.jvv_int_method);
            //jstat = jrun_set_error(jx, JSERR_INVALID_STORE,
            //    "Cannot store to internal method");
            break;
        case JVV_DTYPE_INTERNAL_OBJECT   :
            jstat = jvar_store_int_object(jx, jvvtgt, jvv);
            break;

        case JVV_DTYPE_FUNCTION   :
            jvar_store_function(jvvtgt, jvv);
            break;

        case JVV_DTYPE_INTERNAL_CLASS   :
            jvvtgt->jvv_val.jvv_jvvi = jvv->jvv_val.jvv_jvvi;
            break;

        case JVV_DTYPE_FUNCVAR   :
            jvar_free_jvarvalue_data(jvvtgt);

            jvvtgt->jvv_dtype = JVV_DTYPE_FUNCVAR;
            jvvtgt->jvv_val.jvv_val_funcvar.jvvfv_jvvf = jvv->jvv_val.jvv_val_funcvar.jvvfv_jvvf;
            jvvtgt->jvv_val.jvv_val_funcvar.jvvfv_this_obj = jvv->jvv_val.jvv_val_funcvar.jvvfv_this_obj;
            jvvtgt->jvv_val.jvv_val_funcvar.jvvfv_var_jcx = jvv->jvv_val.jvv_val_funcvar.jvvfv_var_jcx;
            if (jvv->jvv_val.jvv_val_funcvar.jvvfv_jvvf) {
                INCFUNCREFS(jvv->jvv_val.jvv_val_funcvar.jvvfv_jvvf);
            }
            break;

        case JVV_DTYPE_IMETHVAR   :
            jvar_free_jvarvalue_data(jvvtgt);

            jvvtgt->jvv_dtype = JVV_DTYPE_IMETHVAR;
            jvvtgt->jvv_val.jvv_val_imethvar.jvvimv_jvvim = jvv->jvv_val.jvv_val_imethvar.jvvimv_jvvim;
            jvvtgt->jvv_val.jvv_val_imethvar.jvvimv_this_ptr = jvv->jvv_val.jvv_val_imethvar.jvvimv_this_ptr;
            if (jvv->jvv_val.jvv_val_imethvar.jvvimv_jvvim) {
                INCIMETHREFS(jvv->jvv_val.jvv_val_imethvar.jvvimv_jvvim);
            }
            break;

        case JVV_DTYPE_ARRAY   :
            //jstat = jvar_store_array(jx, jvvtgt, jvv);
            jvar_free_jvarvalue_data(jvvtgt);

            jvvtgt->jvv_dtype = JVV_DTYPE_OBJPTR;
#if USE_OLD_JVARVALUE_OBJPTR
            jvvtgt->jvv_val.jvv_objptr.jvvp_dtype = JVV_DTYPE_ARRAY;
            jvvtgt->jvv_val.jvv_objptr.jvvp_val.jvvp_val_array = jvv->jvv_val.jvv_val_array;
            jvv->jvv_val.jvv_val_array->jvva_nRefs += 1;
#else
            jvvtgt->jvv_val.jvv_objptr.jvvp_pval = jvar_new_jvarvalue();
            jvvtgt->jvv_val.jvv_objptr.jvvp_pval->jvv_dtype = JVV_DTYPE_ARRAY;
            jvvtgt->jvv_val.jvv_objptr.jvvp_pval->jvv_val.jvv_val_array = jvv->jvv_val.jvv_val_array;
            jvv->jvv_val.jvv_val_array->jvva_nRefs += 1;
#endif
            break;

        case JVV_DTYPE_OBJECT   :
            jstat = jvar_store_object(jx, jvvtgt, jvv);
            break;
        
        case JVV_DTYPE_OBJPTR:
            jvar_free_jvarvalue_data(jvvtgt);

            jvvtgt->jvv_dtype = JVV_DTYPE_OBJPTR;
#if USE_OLD_JVARVALUE_OBJPTR
            switch (jvv->jvv_val.jvv_objptr.jvvp_dtype) {
                case JVV_DTYPE_ARRAY   :
                    jvvtgt->jvv_val.jvv_objptr.jvvp_dtype = JVV_DTYPE_ARRAY;
                    jvvtgt->jvv_val.jvv_objptr.jvvp_val.jvvp_val_array =
                        jvv->jvv_val.jvv_objptr.jvvp_val.jvvp_val_array;
                    jvv->jvv_val.jvv_objptr.jvvp_val.jvvp_val_array->jvva_nRefs += 1;
                    break;
                default:
                    jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_INVALID_STORE,
                        "Cannot store to object pointer type %s", jvar_get_dtype_name(jvv->jvv_dtype));
                    break;
            }
#else
            switch (jvv->jvv_val.jvv_objptr.jvvp_pval->jvv_dtype) {
                case JVV_DTYPE_ARRAY   :
                    jvvtgt->jvv_val.jvv_objptr.jvvp_pval = jvar_new_jvarvalue();
                    jvvtgt->jvv_val.jvv_objptr.jvvp_pval->jvv_dtype = JVV_DTYPE_ARRAY;
                    jvvtgt->jvv_val.jvv_objptr.jvvp_pval->jvv_val.jvv_val_array =
                        jvv->jvv_val.jvv_objptr.jvvp_pval->jvv_val.jvv_val_array;
                    jvv->jvv_val.jvv_objptr.jvvp_pval->jvv_val.jvv_val_array->jvva_nRefs += 1;
                    break;
                default:
                    jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_INVALID_STORE,
                        "Cannot store to object pointer type %s", jvar_get_dtype_name(jvv->jvv_dtype));
                    break;
            }
#endif
            break;

        default:
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_INVALID_STORE,
                "Cannot store to type %s", jvar_get_dtype_name(jvv->jvv_dtype));
            break;
    }

    return (jstat);
}
/***************************************************************/
int jvar_store_jvarvalue_n(
    struct jrunexec * jx,
    struct jvarvalue ** p_jvvtgt,
    struct jvarvalue * jvvsrc)
{
/*
** 07/15/2021
*/

    if (!(*p_jvvtgt)) {
        (*p_jvvtgt) = jvar_new_jvarvalue();
    }

    return (jvar_store_jvarvalue(jx, (*p_jvvtgt), jvvsrc));
}
/***************************************************************/
int jvar_dup_jvarvalue(
    struct jrunexec * jx,
    struct jvarvalue ** p_jvvtgt,
    struct jvarvalue * jvvsrc)
{
/*
**
*/
    int jstat;

    (*p_jvvtgt) = jvar_new_jvarvalue();
    jstat = jvar_store_jvarvalue(jx, (*p_jvvtgt), jvvsrc);

    return (jstat);
}
/***************************************************************/
#if 0
static void jvv_free_vjvarvalue(void * vjvv)
{
    jvar_free_jvarvalue((struct jvarvalue*)vjvv);
}
#endif
/***************************************************************/
static void jvar_free_jvarrec_consts(struct jvarrec * jvar)
{
/*
** 03/15/2021
*/
    int ii;

    for (ii = 0; ii < jvar->jvar_nconsts; ii++) {
        jvar_free_jvarvalue_data(jvar->jvar_aconsts + ii);
    }
    Free(jvar->jvar_avix);
    Free(jvar->jvar_aconsts);
}
/***************************************************************/
void jvar_free_jvarrec(struct jvarrec * jvar)
{
    /* printf("jvar_free_jvarrec(%4.4s) nRefs=%d\n", jvar->jvar_sn, jvar->jvar_nRefs); */
    DECVARRECREFS(jvar);
    if (!jvar->jvar_nRefs) {
        jvar_free_jvarrec_consts(jvar);
        dbtsi_free(jvar->jvar_jvarvalue_dbt);
        Free(jvar);
    }
}
/***************************************************************/
#if 0
void jvar_push_vars(struct jrunexec * jx, struct jvarrec * jvar)
{
/*
**
*/
    if (jx->jx_nvars == jx->jx_xvars) {
        jx->jx_xvars += 4;
        jx->jx_avars = Realloc(jx->jx_avars,struct jvarrec *,jx->jx_xvars);
    }
    jx->jx_avars[jx->jx_nvars] = jvar;
    jx->jx_nvars += 1;
}
/***************************************************************/
struct jvarrec * jvar_pop_vars(struct jrunexec * jx)
{
/*
**
*/
    struct jvarrec * jvar;

    if (jx->jx_nvars > 0) {
        jx->jx_nvars -= 1;
        jvar_free_jvarrec(jx->jx_avars[jx->jx_nvars]);
    }

    jvar = NULL;
    if (jx->jx_nvars > 0) {
        jvar = jx->jx_avars[jx->jx_nvars - 1];
    }

    return (jvar);
}
#endif
/***************************************************************/
struct jvarvalue * jvar_find_class_member(struct jrunexec * jx,
    struct jvarvalue * cjvv,
    const char * mbrname)
{
/*
**
*/
    struct jvarvalue *  jvv = NULL;
//    struct jvarvalue ** pjvv;
//    int mnlen;
//
//    jvv = NULL;
//    mnlen = IStrlen(mbrname) + 1;
//
//    /* search class variables */
//    if (cjvv) {
//        if (cjvv->jvv_dtype == JVV_DTYPE_OBJECT) {
//            pjvv = (struct jvarvalue **)dbtree_find(
//                cjvv->jvv_val.jvv_jvvo.jvvo_vars->jvar_jvarvalue_dbt,
//                mbrname, mnlen);
//        } else if (cjvv->jvv_dtype == JVV_DTYPE_INTERNAL_CLASS) {
//            pjvv = (struct jvarvalue **)dbtree_find(
//                cjvv->jvv_val.jvv_jvvi->jvvi_jvar->jvar_jvarvalue_dbt,
//                mbrname, mnlen);
//        }
//        if (pjvv) {
//            jvv = (*pjvv);
//        }
//    }

    return (jvv);
}
/***************************************************************/
#ifdef OLD_WAY
struct jvarvalue * xxxjvar_object_member(struct jrunexec * jx,
    struct jvarvalue * cjvv,
    const char * mbrname)
{
/*
**
*/
    struct jvarvalue *  jvv = NULL;
    struct jvarvalue ** pjvv;
    int mnlen;
    int is_dup;

    jvv = NULL;
    mnlen = IStrlen(mbrname) + 1;

    if (cjvv) {
        if (cjvv->jvv_dtype == JVV_DTYPE_OBJECT) {
            /* look in object */
            pjvv = (struct jvarvalue **)dbtree_find(
                cjvv->jvv_val.jvv_jvvo.jvvo_vars->jvar_jvarvalue_dbt,
                mbrname, mnlen);
            if (pjvv) {
                jvv = (*pjvv);
            } else {
                /* if cannot find in object, look in class */
                jvv = jvar_find_class_member(jx, cjvv->jvv_val.jvv_jvvo.jvvo_class, mbrname);
                if (jvv) {
                    /* if found in class, duplicate and put in object */
                    jvv = jvv_dup_jvarvalue(jx, jvv);
                    is_dup = dbtree_insert(
                        cjvv->jvv_val.jvv_jvvo.jvvo_vars->jvar_jvarvalue_dbt,
                        mbrname, mnlen, jvv);
                }
            }
        }
    }

    return (jvv);
}
#endif
/***************************************************************/
#ifdef OLD_WAY
struct jvarvalue * xxxjvar_get_jvv_from_vix(struct jrunexec * jx,
    struct jcontext * jcx,
    int vix,
    int * jcxflags)
{
/*
** 03/04/2021
*/
    struct jvarvalue * jvv;

#if IS_DEBUG
    if (vix < 0 || vix >= jcx->jcx_njvv) {
        printf("**DEBUG** Cannot find variable index: %d", vix);
    }
#endif

    if (vix < jcx->jcx_njvv) {
        jvv = jcx->jcx_ajvv[vix];
        (*jcxflags) = jcx->jcx_flags[vix];
    } else {
        jvv = NULL;
        (*jcxflags) = 0;
    }

    return (jvv);
}
/***************************************************************/
struct jvarvalue ** jvar_get_pjvv_from_vix(struct jrunexec * jx,
    struct jcontext * jcx,
    int vix,
    int * jcxflags)
{
/*
** 10/13/2021
*/
    struct jvarvalue ** pjvv;

#if IS_DEBUG
    if (vix < 0 || vix >= jcx->jcx_njvv) {
        printf("**DEBUG** Cannot find variable index: %d", vix);
    }
#endif

    if (vix < jcx->jcx_njvv) {
        pjvv = &(jcx->jcx_ajvv[vix]);
        (*jcxflags) = jcx->jcx_flags[vix];
    } else {
        pjvv = NULL;
        (*jcxflags) = 0;
    }

    return (pjvv);
}
/***************************************************************/
struct jvarvalue * xxxjvar_get_jvv_from_vix(struct jrunexec * jx,
    struct jcontext * jcx,
    int vix,
    int * jcxflags)
{
/*
** 03/04/2021
*/
    struct jvarvalue * jvv;
    struct jvarvalue ** pjvv;

    pjvv = jvar_get_pjvv_from_vix(jx, jcx, vix, jcxflags);
    if (pjvv) jvv = (*pjvv);
    else jvv = NULL;

    return (jvv);
}
#endif
/***************************************************************/
struct jvarvalue ** jvar_get_pjvv_from_vix(struct jrunexec * jx,
    struct jcontext * jcx,
    int vix,
    int * jcxflags)
{
/*
** 10/13/2021
*/

    return (NULL);
}
/***************************************************************/
struct jvarvalue * jvar_get_jvv_from_vix(struct jrunexec * jx,
    struct jcontext * jcx,
    int vix,
    int * jcxflags)
{
/*
** 03/04/2021
*/
    struct jvarvalue * jvv;

#if IS_DEBUG
    if (vix < 0 || vix >= jcx->jcx_njvv) {
        printf("**DEBUG** Cannot find variable index: %d", vix);
    }
#endif

    if (vix < jcx->jcx_njvv) {
        jvv = &(jcx->jcx_ajvv[vix]);
        (*jcxflags) = jcx->jcx_flags[vix];
    } else {
        jvv = NULL;
        (*jcxflags) = 0;
    }

    return (jvv);
}
/***************************************************************/
struct jcontext * jvar_get_int_object_jcontext(struct jrunexec * jx,
    struct jvarvalue_int_object * jvvo)
{
/*
** 03/04/2021
*/
    struct jcontext * jcx = NULL;

    if (jvvo) {
        if (jvvo->jvvo_class.jvv_dtype == JVV_DTYPE_INTERNAL_CLASS) {
            /* (*pjvar) = jvvo->jvvo_class->jvv_val.jvv_jvvi->jvvi_jvar; */
            jcx = jvvo->jvvo_jcx;
        }
    }

    return (jcx);
}
/***************************************************************/
struct jvarvalue * jvar_int_object_member(struct jrunexec * jx,
    struct jvarvalue * cjvv,
    const char * mbrname,
    int * jcxflags)
{
/*
**
*/
    struct jvarvalue *  jvv = NULL;
    struct jcontext * jcx;
    int * pvix;
    struct jvarvalue_int_object * jvvo;

    jvv = NULL;
    (*jcxflags) = 0;

    if (cjvv) {
        if (cjvv->jvv_dtype == JVV_DTYPE_INTERNAL_OBJECT) {
            jvvo = cjvv->jvv_val.jvv_jvvo;
            pvix = NULL;
            while (jvvo && !pvix) {
                jcx = jvar_get_int_object_jcontext(jx, jvvo);
                pvix = jvar_find_in_jvarrec(jcx->jcx_jvar, mbrname);
                if (pvix) {
                    jvv = jvar_get_jvv_from_vix(jx, jcx, *pvix, jcxflags);
                } else if (jvvo->jvvo_superclass.jvv_dtype == JVV_DTYPE_INTERNAL_OBJECT) {
                    jvvo = jvvo->jvvo_superclass.jvv_val.jvv_jvvo;
                } else {
                    jvvo = NULL;
                }
            }
        }
    }

    return (jvv);
}
/***************************************************************/
#ifdef OLD_WAY
struct jvarvalue * xxxjvar_find_global_variable(struct jrunexec * jx, const char * varname)
{
/*
**
*/
    struct jvarvalue ** pjvv;
    struct jvarvalue *  jvv;
    struct jvarrec   *  jvar;
    int vnlen;

    jvv = NULL;
    vnlen = IStrlen(varname) + 1;

    /* search global variables */
    jvar = jx->jx_globals;
    while (!jvv && jvar) {
        pjvv = (struct jvarvalue **)dbtree_find(
            jvar->jvar_jvarvalue_dbt, varname, vnlen);
        if (pjvv) {
            jvv = (*pjvv);
        } else {
            jvar = jvar->jvar_prev;
        }
    }

    return (jvv);
}
/***************************************************************/
int jvar_find_global_varid(struct jrunexec * jx, const char * varname)
{
/*
** 03/02/2021
*/
    int vix;

    vix = VINVALID;

    return (vix);
}
/***************************************************************/
int jvar_find_varid(struct jrunexec * jx, const char * varname)
{
/*
** 03/02/2021
*/
    int varid;

    varid = VINVALID;

    return (varid);
}
#endif
// /***************************************************************/
// struct jvarvalue * jvar_get_variable_from_varid(struct jrunexec * jx, int varid)
// {
// /*
// ** 03/02/2021
// */
//     struct jvarvalue *  jvv;
// 
//     jvv = NULL;
// 
//     return (jvv);
// }
/***************************************************************/
#ifdef OLD_WAY
int xxxjvar_insert_new_global_variable(struct jrunexec * jx,
    const char * varname,
    struct jvarvalue * jvv)
{
    int jstat = 0;
    int is_dup;
    struct jvarrec * jvar;

    jvar = jx->jx_globals;
    is_dup = dbtree_insert(jvar->jvar_jvarvalue_dbt,
        varname, IStrlen(varname) + 1, jvv);
    if (is_dup) {
        if (jvv->jvv_dtype == JVV_DTYPE_INTERNAL_CLASS) {
            jstat = jrun_set_error(jx, JSERR_DUP_INTERNAL_CLASS,
                "Duplicate internal class: %s", varname);
        } else {
            jstat = jrun_set_error(jx, JSERR_DUPLICATE_VARIABLE,
                "Duplicate variable: %s", varname);
        }
    }

    return (jstat);
}
#endif
/***************************************************************/
#if DEBUG_FREE_VARS
char * jvar_get_varname_from_vix(struct jvarrec * jvar, int vix)
{
/*
** 01/16/2022
*/
    int found = 0;
    void * vcs;
    int * pvix;
    int klen;
    char * field_name;

    vcs = dbtsi_rewind(jvar->jvar_jvarvalue_dbt, DBTREE_READ_FORWARD);
    do {
        pvix = dbtsi_next(vcs);
        if (pvix) {
            field_name = dbtsi_get_key(vcs, &klen);
            if ((*pvix) == vix) found = 1;
        }
    } while (!found && pvix);
    dbtsi_close(vcs);

    if (!found) field_name = "*bad vix*";

    return (field_name);
}
#endif
/***************************************************************/
void jvar_free_function_context(struct jcontext * jcx)
{
/*
** 03/07/2021
*/
    int ii;
    int vix;

    if (!jcx || !jcx->jcx_jvar) {
#if IS_DEBUG
        if (!jcx) printf("jvar_free_function_context(): jcx is null.\n");
        else      printf("jvar_free_function_context(): jcx->jcx_jvar is null.\n");
#endif
        return;
    }

    /* Initialize constants so they don't get freed */
    for (ii = 0; ii < jcx->jcx_jvar->jvar_nconsts; ii++) {
        vix = jcx->jcx_jvar->jvar_avix[ii];
        INIT_JVARVALUE(jcx->jcx_ajvv + vix);
        jcx->jcx_flags[vix] = 0;
    }

    for (ii = 0; ii < jcx->jcx_jvar->jvar_nvars; ii++) {
#if DEBUG_FREE_VARS
    printf("free data for: %s\n", jvar_get_varname_from_vix(jcx->jcx_jvar, ii));
#endif
        jvar_free_jvarvalue_data(jcx->jcx_ajvv + ii);
        jcx->jcx_flags[ii] = 0;
    }
    jvar_free_jvarrec(jcx->jcx_jvar);
}
/***************************************************************/
void jvar_free_jcontext(struct jcontext * jcx)
{
/*
** 03/03/2021
*/
    jvar_free_function_context(jcx);

    if (jcx) {
        Free(jcx->jcx_ajvv);
        Free(jcx->jcx_flags);
        Free(jcx);
    }
}
/***************************************************************/
struct jcontext * jvar_new_jcontext(struct jrunexec * jx,
    struct jvarrec * jvar,
    struct jcontext * outer_jcx)
{
/*
** 03/03/2021
*/
    struct jcontext * jcx;
    int ii;
    int vix;
#if IS_DEBUG
    static int next_jcx_sn = 0;
#endif

    jcx = New(struct jcontext, 1);
#if IS_DEBUG
    jcx->jcx_sn[0] = 'X';
    jcx->jcx_sn[1] = (next_jcx_sn / 100) % 10 + '0';
    jcx->jcx_sn[2] = (next_jcx_sn /  10) % 10 + '0';
    jcx->jcx_sn[3] = (next_jcx_sn      ) % 10 + '0';
    next_jcx_sn++;
#endif
    jcx->jcx_outer_jcx = outer_jcx;
    jcx->jcx_jvvf = NULL;
    jcx->jcx_njvv = jvar->jvar_nvars;
    jcx->jcx_xjvv = jvar->jvar_nvars;
    jcx->jcx_ajvv = NULL;
    jcx->jcx_flags = NULL;
    jcx->jcx_jvar = jvar;
    INCVARRECREFS(jvar);
    if (jcx->jcx_xjvv) {
        jcx->jcx_ajvv = New(struct jvarvalue, jcx->jcx_xjvv);
        jcx->jcx_flags = New(int, jcx->jcx_xjvv);
    }

    /* Copy constants into jcx->jcx_ajvv */
    for (ii = 0; ii < jvar->jvar_nconsts; ii++) {
        vix = jvar->jvar_avix[ii];
        COPY_JVARVALUE(jcx->jcx_ajvv + vix, jvar->jvar_aconsts + ii);
    }

    return (jcx);
}
/***************************************************************/
struct jvarrec * jvar_get_global_jvarrec(struct jrunexec * jx)
{
/*
** 03/04/2021
*/

    return (jx->jx_globals);
}
/***************************************************************/
struct jcontext * jvar_get_global_jcontext(struct jrunexec * jx)
{
/*
** 03/03/2021
*/
    struct jcontext * jcx;

    if (!jx->jx_global_jcx) {
        jx->jx_global_jcx = jvar_new_jcontext(jx, jvar_get_global_jvarrec(jx), NULL);
        jvar_set_head_jcontext(jx, jx->jx_global_jcx);
    }
    jcx = jx->jx_global_jcx;

    return (jcx);
}
/***************************************************************/
int jvar_insert_new_global_variable(struct jrunexec * jx,
    const char * varname,
    struct jvarvalue * jvv,
    int jcxflags)
{
    int jstat = 0;
    struct jcontext * jcx;

    jcx = jvar_get_global_jcontext(jx);

    jstat = jvar_new_ad_hoc_variable(jx, jcx, varname, jvv, jcxflags);

    return (jstat);
}
/***************************************************************/
int jvar_add_class_method(
    struct jrunexec * jx,
    struct jvarvalue * cjvv,
    const char * member_name,
    struct jvarvalue * mjvv)

{
/*
**
*/
    int jstat = 0;
    int vix;

    vix = jvar_insert_into_jvarrec(cjvv->jvv_val.jvv_jvvi->jvvi_jvar, member_name);
    if (vix < 0) {
        if (mjvv->jvv_dtype == JVV_DTYPE_INTERNAL_METHOD) {
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_DUP_INTERNAL_METHOD,
                "Duplicate internal class method: %s", member_name);
        } else {
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_DUP_CLASS_MEMBER,
                "Duplicate class member: %s", member_name);
        }
        return (jstat);
    } else {
        jvar_new_local_const(cjvv->jvv_val.jvv_jvvi->jvvi_jvar, mjvv, vix);
    }

    return (jstat);
}
/***************************************************************/
#if 0
static int jvar_check_varname(
    struct jrunexec * jx,
    const char * varname)
{
/*
** 01/12/2021
*/
    int jstat = 0;
    int ii;

    if (JTOK_FIRST_ID_CHAR(varname[0])) {
        for (ii = 1; !jstat && varname[ii]; ii++) {
            if (JTOK_ID_CHAR(varname[ii])) {
                /* ok */
            } else {
                jstat = jrun_set_error(jx, JSERR_INVALID_VARNAME,
                    "Invalid character in variable name \'%s\'", varname);
            }
        }
    } else {
        jstat = jrun_set_error(jx, JSERR_INVALID_VARNAME,
            "Invalid first character in variable name \'%s\'", varname);
    }

    return (jstat);
}
#endif
/***************************************************************/
int jvar_is_valid_varname(
    struct jrunexec * jx,
    struct jtoken * jtok)
{
/*
** 02/26/2021
**
** Returns:
**  0 - jtok->jtok_text is valid variable name
**  1 - jtok->jtok_text is an invalid variable name
**  2 - jtok->jtok_text is a reserved word
*/
    int is_valid = 0;

    if (jtok->jtok_typ != JSW_ID) {
        if (jtok->jtok_typ != JSW_KW) {
            is_valid = 1;
        } else {
            if ((jtok->jtok_flags & JSKWI_VAR_OK) == 0) {
                is_valid = 2;
            }
        }
    }

    return (is_valid);
}
/***************************************************************/
int jvar_validate_varname(
    struct jrunexec * jx,
    struct jtoken * jtok)
{
/*
** 01/12/2021
*/
    int jstat = 0;
    int is_valid ;

    is_valid = jvar_is_valid_varname(jx, jtok);

    if (is_valid) {
        if (is_valid == 2) {
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_INVALID_VARNAME,
                "Reserved word used as identifier. Found \'%s\'", jtok->jtok_text);
        } else {
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_INVALID_VARNAME,
                "Expecting identifier. Found \'%s\'", jtok->jtok_text);
        }
    }

    return (jstat);
}
/***************************************************************/
int jvar_new_local_variable(struct jrunexec * jx,
    struct jvarvalue_function * jvvf,
    const char * varname)
{
/*
** 02/26/2021
*/
    int vix;

    vix = jvar_insert_into_jvarrec(jvvf->jvvf_vars, varname);

    return (vix);
}
/***************************************************************/
#if 0
void xxxjvar_new_local_function(struct jrunexec * jx,
    struct jvarvalue_function * jvvf_parent,
    struct jvarvalue_function * jvvf_child,
    int vix_parent)
{
/*
** 02/26/2021
*/
    char * funcname = jvvf_child->jvvf_func_name;

    if (jvvf_parent->jvvf_nfuncs == jvvf_parent->jvvf_xfuncs) {
        if (!jvvf_parent->jvvf_nfuncs) jvvf_parent->jvvf_xfuncs = 2;
        else                   jvvf_parent->jvvf_xfuncs *= 2;
        jvvf_parent->jvvf_afuncs =
            Realloc(jvvf_parent->jvvf_afuncs, struct jvarvalue_function *, jvvf_parent->jvvf_xfuncs);
    }
    jvvf_parent->jvvf_afuncs[jvvf_parent->jvvf_nfuncs] = jvvf_child;
    jvvf_parent->jvvf_nfuncs += 1;

    jvvf_child->jvvf_vix = vix_parent;
}
#endif
/***************************************************************/
#if 0
int xxxjvar_new_local_function(struct jrunexec * jx,
    struct jvarvalue_function * jvvf_parent,
    struct jvarvalue          * fjvv_child,
    int jcxflags)
{
/*
** 02/26/2021
*/
    int jstat = 0;
    int vix_parent;

    if (fjvv_child->jvv_dtype != JVV_DTYPE_FUNCTION ||
        !fjvv_child->jvv_val.jvv_val_function->jvvf_func_name) {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERROBJ_SCRIPT_SYNTAX_ERROR,
            "Expecting function name.");
    } else {
        vix_parent = jvar_insert_into_jvarrec(jvvf_parent->jvvf_vars,
                fjvv_child->jvv_val.jvv_val_function->jvvf_func_name);
        if (vix_parent < 0) {
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERROBJ_SCRIPT_SYNTAX_ERROR,
                "Duplicate function name. Found: '%s'",
                    fjvv_child->jvv_val.jvv_val_function->jvvf_func_name);
        } else {
            fjvv_child->jvv_val.jvv_val_function->jvvf_vix = vix_parent;

            jvar_new_local_const(jvvf_parent->jvvf_vars, fjvv_child, vix_parent, jcxflags);
        }
    }

    return (jstat);
}
#endif
/***************************************************************/
int jvar_new_local_function(struct jrunexec * jx,
    struct jtiopenbrace * jtiob,
    struct jvarvalue    * fjvv_child,
    int jcxflags,
    int skip_flags)
{
/*
** 12/28/2021
*/
    int jstat = 0;
    int vix;
    int * pvix;

    if (fjvv_child->jvv_dtype != JVV_DTYPE_FUNCTION ||
        !fjvv_child->jvv_val.jvv_val_function->jvvf_func_name) {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERROBJ_SCRIPT_SYNTAX_ERROR,
            "Expecting function name.");
    } else {
        fjvv_child->jvv_flags = jcxflags;
        pvix = jvar_find_in_jvarrec(jtiob->jtiob_jvar, 
            fjvv_child->jvv_val.jvv_val_function->jvvf_func_name);
        if (!pvix) {
            if (skip_flags & SKIP_FLAG_HOIST) {
                vix = jvar_insert_into_jvarrec(jtiob->jtiob_jvar, 
                    fjvv_child->jvv_val.jvv_val_function->jvvf_func_name);
                fjvv_child->jvv_val.jvv_val_function->jvvf_vix = vix;

                jvar_new_local_const(jtiob->jtiob_jvar, fjvv_child, vix);
            }
        } else {
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERROBJ_SCRIPT_SYNTAX_ERROR,
                "Duplicate function name. Found: '%s'",
                    fjvv_child->jvv_val.jvv_val_function->jvvf_func_name);
        }
    }

    return (jstat);
}
/***************************************************************/
int jvar_find_local_variable_index(struct jrunexec * jx,
    struct jvarvalue_function * jvvf,
    const char * varname)
{
/*
** 02/26/2021
*/
    int vix;

    vix = VINVALID;

    return (vix);
}
/***************************************************************/
struct jvarvalue * jvar_get_jvv_with_expand(struct jrunexec * jx,
    struct jcontext * jcx,
    int vix,
    int * jcxflags) /* in/out */
{
/*
** 03/04/2021
*/
    struct jvarvalue * jvv;

    if (vix >= jcx->jcx_xjvv) {
        if (!jcx->jcx_xjvv) jcx->jcx_xjvv = 4;
        while (vix >= jcx->jcx_xjvv) jcx->jcx_xjvv *= 2;
        jcx->jcx_ajvv = Realloc(jcx->jcx_ajvv, struct jvarvalue, jcx->jcx_xjvv);
        jcx->jcx_flags = Realloc(jcx->jcx_flags, int, jcx->jcx_xjvv);
        memset(jcx->jcx_ajvv + jcx->jcx_njvv, 0, (jcx->jcx_xjvv - jcx->jcx_njvv) * sizeof(struct jvarvalue));
        memset(jcx->jcx_flags + jcx->jcx_njvv, 0, (jcx->jcx_xjvv - jcx->jcx_njvv) * sizeof(int));
    }

    if (vix >= jcx->jcx_njvv) {
        jcx->jcx_flags[vix] = (*jcxflags);
        jcx->jcx_njvv = vix + 1;
    } else {
        (*jcxflags) = jcx->jcx_flags[vix];
    }

    jvv = jcx->jcx_ajvv + vix;

    return (jvv);
}
/***************************************************************/
int jvar_is_jvv_const(struct jvarvalue * jvv)
{
/*
** 03/16/2021
*/
    int is_const = 0;

    if ((jvv->jvv_dtype == JVV_DTYPE_FUNCTION)          ||
        (jvv->jvv_dtype == JVV_DTYPE_INTERNAL_METHOD)   ||
        (jvv->jvv_dtype == JVV_DTYPE_INTERNAL_CLASS)    ) {
        is_const = 1;
    }

    return (is_const);
}
/***************************************************************/
int jvar_new_ad_hoc_variable(struct jrunexec * jx,
    struct jcontext * jcx,
    const char * varname,
    struct jvarvalue * jvv,
    int jcxflags)
{
/*
** 03/01/2021
*/
    int jstat = 0;
    int * pvix;
    int vix;
    int injcxflags = 0;
    struct jvarvalue * tgt_jvv;

    pvix = jvar_find_in_jvarrec(jcx->jcx_jvar, varname);
    if (pvix) {
        vix = *pvix;
    } else {
        vix = jvar_insert_into_jvarrec(jcx->jcx_jvar, varname);
    }
    tgt_jvv = jvar_get_jvv_with_expand(jx, jcx, vix, &injcxflags);
    if (tgt_jvv) {
        COPY_JVARVALUE(tgt_jvv, jvv);
        tgt_jvv->jvv_flags = jcxflags;
        if (jvar_is_jvv_const(jvv)) {
            jvar_new_local_const(jcx->jcx_jvar, jvv, vix);
        }
    }

    return (jstat);
}
/***************************************************************/
struct jvarvalue * jvar_find_local_variable(
    struct jrunexec * jx,
    struct jcontext * local_jcx,
    const char * varname)
{
/*
** 03/02/2021
*/
    struct jvarvalue *  jvv;
    int * pvix;
    int jcxflags;

    jvv = NULL;

    pvix = jvar_find_in_jvarrec(local_jcx->jcx_jvar, varname);
    if (pvix) {
        jvv = jvar_get_jvv_from_vix(jx, local_jcx, *pvix, &jcxflags);
    }

    return (jvv);
}
/***************************************************************/
#ifdef OLD_WAY
struct jcontext * jvar_get_current_jcontext(struct jrunexec * jx)
{
/*
** 03/04/2021
*/
    struct jfuncstate * jxfs;
    struct jcontext * jcx;

    if (jx->jx_njfs) {
        jxfs = jx->jx_jfs[jx->jx_njfs - 1];
        jcx = jxfs->jfs_jcx;
    } else {
        jcx = jvar_get_global_jcontext(jx);
    }

    return (jcx);
}
#endif
/***************************************************************/
struct jfuncstate * jvar_get_current_jfuncstate(struct jrunexec * jx)
{
/*
** 03/18/2021
*/
    struct jfuncstate * jxfs = NULL;

    if (jx->jx_njfs) {
        jxfs = jx->jx_jfs[jx->jx_njfs - 1];
    }

    return (jxfs);
}
/***************************************************************/
struct jcontext * jvar_get_head_jcontext(struct jrunexec * jx)
{
/*
** 09/22/2021
*/
    struct jcontext * jcx;

    jcx = jx->jx_head_jcx;

    return (jcx);
}
/***************************************************************/
void jvar_set_head_jcontext(struct jrunexec * jx, struct jcontext * jcx)
{
/*
** 09/22/2021
*/
    jx->jx_head_jcx = jcx;
}
/***************************************************************/
struct jvarvalue * jvar_new_jvv_at_vix(struct jrunexec * jx,
    struct jcontext * jcx,
    int vix,
    int jcxflags)
{
/*
** 03/18/2021
*/
    struct jvarvalue * jvv;

#if IS_DEBUG
    if (vix < 0 || vix >= jcx->jcx_njvv) {
        printf("**DEBUG** Cannot find new variable index: %d", vix);
    }
#endif

    if (vix < jcx->jcx_njvv) {
        jvv = jvar_new_jvarvalue();
        COPY_JVARVALUE(jcx->jcx_ajvv + vix, jvv);
        jcx->jcx_flags[vix] = jcxflags;
    } else {
        jvv = NULL;
    }

    return (jvv);
}
/***************************************************************/
struct jvarvalue * jvar_find_variable(
    struct jrunexec * jx,
    struct jtoken * jtok,
    struct jcontext ** pvar_jcx,
    int * jcxflags) /* in/out */
{
/*
** 03/05/2021
*/
    struct jvarvalue *  jvv = NULL;
    struct jcontext * jcx;
    int * pvix;

    (*pvar_jcx) = NULL;

    jcx = jvar_get_head_jcontext(jx);
    while (jcx && !jvv) {
        pvix = jvar_find_in_jvarrec(jcx->jcx_jvar, jtok->jtok_text);
        if (!pvix) {
            jcx = jcx->jcx_outer_jcx;
        } else {
            (*pvar_jcx) = jcx;
            jvv = jvar_get_jvv_from_vix(jx, jcx, *pvix, jcxflags);
            if (!jvv) {
                jvv = jvar_new_jvv_at_vix(jx, jcx, *pvix, *jcxflags);
            }
        }
    }

    return (jvv);
}
/***************************************************************/
struct jvarvalue_int_method * jvar_new_int_method(
    const char * method_name,
    JVAR_INTERNAL_FUNCTION ifuncptr,
    struct jvarvalue * cjvv)
{
/*
**
*/
    struct jvarvalue_int_method * jvvim;
#if IS_DEBUG
    static int next_jvvim_sn = 0;
#endif

    jvvim = New(struct jvarvalue_int_method, 1);
#if IS_DEBUG
    jvvim->jvvim_sn[0] = 'I';
    jvvim->jvvim_sn[1] = (next_jvvim_sn / 100) % 10 + '0';
    jvvim->jvvim_sn[2] = (next_jvvim_sn /  10) % 10 + '0';
    jvvim->jvvim_sn[3] = (next_jvvim_sn      ) % 10 + '0';
    next_jvvim_sn++;
#endif

    jvvim->jvvim_method = ifuncptr;
    jvvim->jvvim_method_name = Strdup(method_name);
    jvvim->jvvim_cjvv = cjvv;

    return (jvvim);
}
/***************************************************************/
int jrun_ensure_int(
    struct jrunexec * jx,
    struct jvarvalue * jvv,
    JSINT *intval,
    int ensflags)
{
/*
** 10/07/2021
**
** See also: jexp_eval_rval()
*/
    int jstat = 0;
    int cstat;
    int intnum;
    double dblnum;
    struct jvarvalue * rjvv;

    (*intval) = 0;

    switch (jvv->jvv_dtype) {
        
        case JVV_DTYPE_BOOL   :
            (*intval) = jvv->jvv_val.jvv_val_bool?1:0;
            break;

        case JVV_DTYPE_JSINT   :
            (*intval) = jvv->jvv_val.jvv_val_jsint;
            break;

        case JVV_DTYPE_JSFLOAT   :
            (*intval) = (int)floor(jvv->jvv_val.jvv_val_jsfloat);
            break;

        case JVV_DTYPE_CHARS   :
            cstat = convert_string_to_number(jvv->jvv_val.jvv_val_chars.jvvc_val_chars, &intnum, &dblnum);
            if (cstat == 1) (*intval) = intnum;
            else if (cstat == 2) (*intval) = (int)floor(dblnum);
            else if (ensflags & ENSINT_FLAG_INTERR) {
                jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_CONVERT_TO_INT,
                    "Expecting integer. Found string: %s",        
                    jvv->jvv_val.jvv_val_chars.jvvc_val_chars);
            }
            break;

        case JVV_DTYPE_LVAL:
            jstat = jrun_ensure_int(jx, jvv->jvv_val.jvv_lval.jvvv_lval, intval, ensflags);
            break;

        case JVV_DTYPE_DYNAMIC: /* 02/21/2022 */
            jstat = (jvv->jvv_val.jvv_val_dynamic->jvvy_get_proc)(jx,
                jvv->jvv_val.jvv_val_dynamic,
                NULL,
                &rjvv);
            if (!jstat) {
                jstat = jrun_ensure_int(jx, rjvv, intval, ensflags);
            }
            break;

        case JVV_DTYPE_NONE   :
        case JVV_DTYPE_INTERNAL_CLASS:
        case JVV_DTYPE_INTERNAL_METHOD:
        case JVV_DTYPE_INTERNAL_OBJECT:
        case JVV_DTYPE_VALLIST:
        case JVV_DTYPE_TOKEN:
        case JVV_DTYPE_FUNCTION:
        case JVV_DTYPE_FUNCVAR   :
        case JVV_DTYPE_ARRAY:
        default:
            if (ensflags & ENSINT_FLAG_INTERR) {
                jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_CONVERT_TO_INT,
                    "Expecting integer. Found type: %s",        
                    jvar_get_dtype_name(jvv->jvv_dtype));
            }
            break;
    }

    return (jstat);
}
/***************************************************************/
int jrun_ensure_chars(
    struct jrunexec * jx,
    struct jvarvalue * jvv,
    struct jvarvalue * jvvchars,
    int ensflags)
{
/*
** 02/06/2022
*/
    int jstat = 0;
    char numbuf[64];

    INIT_JVARVALUE(jvvchars);

    switch (jvv->jvv_dtype) {
        
        case JVV_DTYPE_BOOL   :
            strcpy(numbuf, jvv->jvv_val.jvv_val_bool?"true":"false");
            jvar_store_chars_len(jvvchars, numbuf, IStrlen(numbuf));
            break;

        case JVV_DTYPE_JSINT   :
            Snprintf(numbuf, sizeof(numbuf), "%d", jvv->jvv_val.jvv_val_jsint);
            jvar_store_chars_len(jvvchars, numbuf, IStrlen(numbuf));
            break;

        case JVV_DTYPE_JSFLOAT   :
            Snprintf(numbuf, sizeof(numbuf), "%g", jvv->jvv_val.jvv_val_jsfloat);
            jvar_store_chars_len(jvvchars, numbuf, IStrlen(numbuf));
            break;

        case JVV_DTYPE_CHARS   :
            jvar_store_chars_len(jvvchars,
                jvv->jvv_val.jvv_val_chars.jvvc_val_chars,
                jvv->jvv_val.jvv_val_chars.jvvc_length);
            break;

        case JVV_DTYPE_LVAL:
            jstat = jrun_ensure_chars(jx, jvv->jvv_val.jvv_lval.jvvv_lval, jvvchars, ensflags);
            break;

        default:
            if (ensflags & ENSCHARS_FLAG_ERR) {
                jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_CONVERT_TO_CHARS,
                    "Expecting character string. Found type: %s",        
                    jvar_get_dtype_name(jvv->jvv_dtype));
            }
            break;
    }

    return (jstat);
}
/***************************************************************/
int jvar_get_block_jcontext(
                    struct jrunexec * jx,
                    const char * varname,
                    struct jcontext ** pjcx)
{
/*
** 12/28/2021
*/

    int jstat = 0;
    struct jcontext * jcx;
    struct jvarrec * jvar;
    struct jtokeninfo * jti;

    jcx = XSTAT_JCX(jx);
    if (!jcx) {
        if (XSTAT_XSTATE(jx) != XSTAT_ACTIVE_BLOCK) {
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_MUST_BE_BLOCK,
                "Variable \'%s\' must be declared in block.", varname);
        } else {
            jti = XSTAT_JTI(jx);
            if (!jti) {
                jstat = jrun_set_error(jx, errtyp_InternalError, JSERR_INTERNAL_ERROR,
                    "Internal error: Variable \'%s\' not hoisted.", varname);
#if IS_DEBUG
            } else if (jti->jti_type != JTI_TYPE_OPEN_BRACE) {
                jstat = jrun_set_error(jx, errtyp_InternalError, JSERR_INTERNAL_ERROR,
                    "Internal error: Tokenifo for \'%s\' is wrong type: %d", varname, jti->jti_type);
#endif
            } else {
                jvar = jti->jti_val.jti_val_jtiob->jtiob_jvar;
                jcx = jvar_new_jcontext(jx, jvar, jvar_get_head_jcontext(jx));
                jvar_set_head_jcontext(jx, jcx);
                XSTAT_JCX(jx) = jcx;
            }
        }
    }
    (*pjcx) = jcx;

    return (jstat);
}
/***************************************************************/
struct jvarvalue_dynamic * new_jvarvalue_dynamic(void)
{
/*
** 02/22/2022
*/
    struct jvarvalue_dynamic * jvvy;
#if IS_DEBUG
    static int next_jvvy_sn = 0;
#endif

    jvvy = New(struct jvarvalue_dynamic, 1);
#if IS_DEBUG
    jvvy->jvvy_sn[0] = 'Y';
    jvvy->jvvy_sn[1] = (next_jvvy_sn / 100) % 10 + '0';
    jvvy->jvvy_sn[2] = (next_jvvy_sn /  10) % 10 + '0';
    jvvy->jvvy_sn[3] = (next_jvvy_sn      ) % 10 + '0';
    next_jvvy_sn++;
#endif
    jvvy->jvvy_get_proc  = NULL;
    jvvy->jvvy_set_proc  = NULL;
    jvvy->jvvy_free_proc = NULL;
    jvvy->jvvy_this_ptr  = NULL;
    jvvy->jvvy_arg       = NULL;
    jvvy->jvvy_rtn       = NULL;
    jvvy->jvvy_index     = NULL;

    return (jvvy);
}
/***************************************************************/
void free_jvarvalue_dynamic(struct jvarvalue_dynamic * jvvy)
{
/*
** 02/22/2022
*/
    if (jvvy->jvvy_free_proc) {
        (jvvy->jvvy_free_proc)(jvvy);
    }
    jvar_free_jvarvalue(jvvy->jvvy_rtn);
    jvar_free_jvarvalue(jvvy->jvvy_index);
    Free(jvvy);
}
/***************************************************************/
