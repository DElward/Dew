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
struct jprototype * jint_new_jprototype(struct jrunexec * jx)
{
    struct jprototype * jpt;
#if IS_DEBUG
    static int next_jpt_sn = 0;
#endif

    jpt = New(struct jprototype, 1);
#if IS_DEBUG
    jpt->jpt_sn[0] = 'P';
    jpt->jpt_sn[1] = (next_jpt_sn / 100) % 10 + '0';
    jpt->jpt_sn[2] = (next_jpt_sn /  10) % 10 + '0';
    jpt->jpt_sn[3] = (next_jpt_sn      ) % 10 + '0';
    next_jpt_sn++;
#endif
    jpt->jpt_nRefs = 0;

    jpt->jpt_jvar = jvar_new_jvarrec();
    //INCVARRECREFS(jpt->jpt_jvar);
    jpt->jpt_jcx = jvar_new_jcontext(jx, jpt->jpt_jvar, NULL);
    INCPROTOTYPEREFS(jpt);

    return (jpt);
}
/***************************************************************/
void jint_free_jprototype(struct jprototype * jpt)
{
    DECPROTOTYPEREFS(jpt);

    if (!jpt->jpt_nRefs) {
        //jvar_free_jvarrec(jpt->jpt_jvar);
        jvar_free_jcontext(jpt->jpt_jcx);
        Free(jpt);
    }
}
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
    jvv->jvv_flags = 0;

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
        case JVV_DTYPE_POINTER         : out = "Pointer";           break;
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
        jint_free_jprototype(jvvi->jvvi_prototype);
#if ALLOW_INT_CLASS_VARS
        jvar_free_jcontext(jvvi->jvvi_jcx);
#else
        jvar_free_jvarrec(jvvi->jvvi_jvar);
#endif
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
        /* jvar_free_jvarvalue_data(&(jvvim->jvvim_class)); */
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
static void jvvl_free_jvarvalue_imethvar_data(struct jvarvalue_imethvar * jvvimv)
{
/*
** 03/18/2022
*/
    jvar_free_jvarvalue_internal_method(jvvimv->jvvimv_jvvim);
    if (jvvimv->jvvimv_this_jvv) jvar_free_jvarvalue(jvvimv->jvvimv_this_jvv);
}
/***************************************************************/
struct jvarvalue_chars * jvar_new_jvarvalue_chars()
{
/*
** 03/15/2022
*/
    struct jvarvalue_chars * jvv_chars;

    jvv_chars = New(struct jvarvalue_chars, 1);
    jvv_chars->jvvc_val_chars = NULL;
    jvv_chars->jvvc_length = 0;
    jvv_chars->jvvc_size   = 0;

    return (jvv_chars);
}
/***************************************************************/
void jvar_free_jvarvalue_chars(struct jvarvalue_chars * jvv_chars)
{
/*
** 03/15/2022
*/
    Free(jvv_chars->jvvc_val_chars);
    Free(jvv_chars);
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
        case JVV_DTYPE_PROTOTYPE:   /* 10/04/2022 */
            jvv->jvv_dtype = JVV_DTYPE_NONE;
            break;

        case JVV_DTYPE_LVAL :
            jvv->jvv_dtype = JVV_DTYPE_NONE;
            if (jvv->jvv_val.jvv_lval.jvvv_parent) {
                jvar_free_jvarvalue(jvv->jvv_val.jvv_lval.jvvv_parent);
            }
            break;

        case JVV_DTYPE_CHARS :
            jvar_free_jvarvalue_chars(jvv->jvv_val.jvv_val_chars);
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
            jvvl_free_jvarvalue_imethvar_data(&(jvv->jvv_val.jvv_val_imethvar));
            jvv->jvv_dtype = JVV_DTYPE_NONE;
            break;

        case JVV_DTYPE_ARRAY :
            jvar_free_jvarvalue_array(jvv->jvv_val.jvv_val_array);
            jvv->jvv_val.jvv_val_array = NULL;
            jvv->jvv_dtype = JVV_DTYPE_NONE;
            break;
        
        case JVV_DTYPE_OBJPTR:
            jvar_free_jvarvalue(jvv->jvv_val.jvv_objptr.jvvp_pval);
            jvv->jvv_dtype = JVV_DTYPE_NONE;
            break;

        case JVV_DTYPE_DYNAMIC :
            free_jvarvalue_dynamic(jvv->jvv_val.jvv_val_dynamic);
            jvv->jvv_dtype = JVV_DTYPE_NONE;
            break;

        case JVV_DTYPE_OBJECT :
            job_free_jvarvalue_object(jvv->jvv_val.jvv_val_object);
            jvv->jvv_val.jvv_val_object = NULL;
            jvv->jvv_dtype = JVV_DTYPE_NONE;
            break;

        case JVV_DTYPE_POINTER   :
            jvar_free_jvarvalue_pointer(jvv->jvv_val.jvv_pointer);
            jvv->jvv_dtype = JVV_DTYPE_NONE;
            jvv->jvv_val.jvv_pointer = NULL;
            break;

        default:
            printf("jvar_free_jvarvalue_data(%s)\n",
                jvar_get_dtype_name(jvv->jvv_dtype));
            break;
    }
#if IS_DEBUG
    jvv->jvv_sn[0] = '~';
    jvv->jvv_sn[1] = '~';
    jvv->jvv_sn[2] = '~';
    jvv->jvv_sn[3] = '~';
#endif
}
/***************************************************************/
void jvar_free_jvarvalue(struct jvarvalue * jvv)
{
/*
** 02/19/2021
*/
    if (jvv) {
        jvar_free_jvarvalue_data(jvv);
        Free(jvv);
    }
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
void jvar_store_this(struct jrunexec * jx, struct jvarvalue * jvv)
{
/*
** 09/15/2022
*/
    struct jvarvalue * this_jvv;

    jvar_free_jvarvalue_data(jvv);

    this_jvv = jvar_get_current_object(jx);
    if (this_jvv->jvv_dtype == JVV_DTYPE_OBJECT) {
        if (this_jvv) {
            jvv->jvv_dtype = JVV_DTYPE_OBJECT;
            jvv->jvv_val.jvv_val_object = this_jvv->jvv_val.jvv_val_object;
            INCOBJREFS(this_jvv->jvv_val.jvv_val_object);
        } else {
            jvv->jvv_dtype = JVV_DTYPE_NULL;
            jvv->jvv_val.jvv_void = NULL;
        }
    } else {
        jvv->jvv_dtype = JVV_DTYPE_NULL;
        jvv->jvv_val.jvv_void = NULL;
    }
}
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

    //jstat = jexp_eval_rval(jx,jvvsrc, &jvv);
    //if (jstat) return (jstat);

    switch (jvvsrc->jvv_dtype) {
        case JVV_DTYPE_NONE:
            jvar_free_jvarvalue_data(jvvtgt);
            jvvsrc->jvv_dtype = JVV_DTYPE_NONE;
            break;
        case JVV_DTYPE_LVAL:
            jstat = jvar_store_jvarvalue(jx, jvvtgt, jvvsrc->jvv_val.jvv_lval.jvvv_lval);
            break;
        case JVV_DTYPE_DYNAMIC:
            jstat = (jvvsrc->jvv_val.jvv_val_dynamic->jvvy_get_proc)(jx,
                jvvsrc->jvv_val.jvv_val_dynamic, NULL, &jvv);
            if (!jstat) jstat = jvar_store_jvarvalue(jx, jvvtgt, jvv);
            break;
        case JVV_DTYPE_NULL   :
            jvar_store_null(jvvtgt);
            break;
        case JVV_DTYPE_BOOL   :
            jvar_store_bool(jvvtgt, jvvsrc->jvv_val.jvv_val_bool);
            break;
        case JVV_DTYPE_JSINT   :
            jvar_store_jsint(jvvtgt, jvvsrc->jvv_val.jvv_val_jsint);
            break;
        case JVV_DTYPE_JSFLOAT   :
            jvar_store_jsfloat(jvvtgt, jvvsrc->jvv_val.jvv_val_jsfloat);
            break;
        case JVV_DTYPE_CHARS   :
            jvar_store_chars_len(jvvtgt,
                jvvsrc->jvv_val.jvv_val_chars->jvvc_val_chars,
                jvvsrc->jvv_val.jvv_val_chars->jvvc_length);
            break;
        case JVV_DTYPE_INTERNAL_METHOD   :
            jvvtgt->jvv_dtype = jvvsrc->jvv_dtype;
            jvvtgt->jvv_val.jvv_int_method = jvvsrc->jvv_val.jvv_int_method;
            INCIMETHREFS(jvvsrc->jvv_val.jvv_int_method);
            //jstat = jrun_set_error(jx, JSERR_INVALID_STORE,
            //    "Cannot store to internal method");
            break;
        case JVV_DTYPE_INTERNAL_OBJECT   :
            jstat = jvar_store_int_object(jx, jvvtgt, jvvsrc);
            break;

        case JVV_DTYPE_FUNCTION   :
            jvar_store_function(jvvtgt, jvvsrc);
            break;

        case JVV_DTYPE_INTERNAL_CLASS   :
            jvar_free_jvarvalue_data(jvvtgt);

            jvvtgt->jvv_dtype = JVV_DTYPE_INTERNAL_CLASS;
            jvvtgt->jvv_val.jvv_jvvi = jvvsrc->jvv_val.jvv_jvvi;
            INCICLASSHREFS(jvvsrc->jvv_val.jvv_jvvi);
            break;

        case JVV_DTYPE_FUNCVAR   :
            jvar_free_jvarvalue_data(jvvtgt);

            jvvtgt->jvv_dtype = JVV_DTYPE_FUNCVAR;
            jvvtgt->jvv_val.jvv_val_funcvar.jvvfv_jvvf = jvvsrc->jvv_val.jvv_val_funcvar.jvvfv_jvvf;
            jvvtgt->jvv_val.jvv_val_funcvar.jvvfv_this_obj = jvvsrc->jvv_val.jvv_val_funcvar.jvvfv_this_obj;
            jvvtgt->jvv_val.jvv_val_funcvar.jvvfv_var_jcx = jvvsrc->jvv_val.jvv_val_funcvar.jvvfv_var_jcx;
            if (jvvsrc->jvv_val.jvv_val_funcvar.jvvfv_jvvf) {
                INCFUNCREFS(jvvsrc->jvv_val.jvv_val_funcvar.jvvfv_jvvf);
            }
            break;

        case JVV_DTYPE_IMETHVAR   :
            jvar_free_jvarvalue_data(jvvtgt);

            jvvtgt->jvv_dtype = JVV_DTYPE_IMETHVAR;
            jvvtgt->jvv_val.jvv_val_imethvar.jvvimv_jvvim = jvvsrc->jvv_val.jvv_val_imethvar.jvvimv_jvvim;
            jvvtgt->jvv_val.jvv_val_imethvar.jvvimv_this_ptr = jvvsrc->jvv_val.jvv_val_imethvar.jvvimv_this_ptr;
            if (jvvsrc->jvv_val.jvv_val_imethvar.jvvimv_jvvim) {
                INCIMETHREFS(jvvsrc->jvv_val.jvv_val_imethvar.jvvimv_jvvim);
            }
            break;

        case JVV_DTYPE_ARRAY   :
            //jstat = jvar_store_array(jx, jvvtgt, jvv);
            jvar_free_jvarvalue_data(jvvtgt);

            jvvtgt->jvv_dtype = JVV_DTYPE_OBJPTR;
            jvvtgt->jvv_val.jvv_objptr.jvvp_pval = jvar_new_jvarvalue();
            jvvtgt->jvv_val.jvv_objptr.jvvp_pval->jvv_dtype = JVV_DTYPE_ARRAY;
            jvvtgt->jvv_val.jvv_objptr.jvvp_pval->jvv_val.jvv_val_array = jvvsrc->jvv_val.jvv_val_array;
            INCARRAYREFS(jvvsrc->jvv_val.jvv_val_array);
            break;

        case JVV_DTYPE_OBJECT   :
            //jstat = jvar_store_object(jx, jvvtgt, jvv);
            jvar_free_jvarvalue_data(jvvtgt);

            jvvtgt->jvv_dtype = JVV_DTYPE_OBJPTR;
            jvvtgt->jvv_val.jvv_objptr.jvvp_pval = jvar_new_jvarvalue();
            jvvtgt->jvv_val.jvv_objptr.jvvp_pval->jvv_dtype = JVV_DTYPE_OBJECT;
            jvvtgt->jvv_val.jvv_objptr.jvvp_pval->jvv_val.jvv_val_object = jvvsrc->jvv_val.jvv_val_object;
            INCOBJREFS(jvvsrc->jvv_val.jvv_val_object);
            break;
        
        case JVV_DTYPE_OBJPTR:
            jvar_free_jvarvalue_data(jvvtgt);

            jvvtgt->jvv_dtype = JVV_DTYPE_OBJPTR;
            switch (jvvsrc->jvv_val.jvv_objptr.jvvp_pval->jvv_dtype) {
                case JVV_DTYPE_ARRAY   :
                    jvvtgt->jvv_val.jvv_objptr.jvvp_pval = jvar_new_jvarvalue();
                    jvvtgt->jvv_val.jvv_objptr.jvvp_pval->jvv_dtype = JVV_DTYPE_ARRAY;
                    jvvtgt->jvv_val.jvv_objptr.jvvp_pval->jvv_val.jvv_val_array =
                        jvvsrc->jvv_val.jvv_objptr.jvvp_pval->jvv_val.jvv_val_array;
                    INCARRAYREFS(jvvsrc->jvv_val.jvv_objptr.jvvp_pval->jvv_val.jvv_val_array);
                    break;
                case JVV_DTYPE_OBJECT   :
                    jvvtgt->jvv_val.jvv_objptr.jvvp_pval = jvar_new_jvarvalue();
                    jvvtgt->jvv_val.jvv_objptr.jvvp_pval->jvv_dtype = JVV_DTYPE_OBJECT;
                    jvvtgt->jvv_val.jvv_objptr.jvvp_pval->jvv_val.jvv_val_object =
                        jvvsrc->jvv_val.jvv_objptr.jvvp_pval->jvv_val.jvv_val_object;
                    INCOBJREFS(jvvsrc->jvv_val.jvv_objptr.jvvp_pval->jvv_val.jvv_val_object);
                    break;
                default:
                    jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_INVALID_STORE,
                        "Cannot store to object pointer type %s", jvar_get_dtype_name(jvvsrc->jvv_dtype));
                    break;
            }
            break;

        case JVV_DTYPE_POINTER   :
            jvar_free_jvarvalue_data(jvvtgt);

            jvvtgt->jvv_dtype = JVV_DTYPE_POINTER;
            jvvtgt->jvv_val.jvv_pointer = jvvsrc->jvv_val.jvv_pointer;
            INCPOINTERREFS(jvvsrc->jvv_val.jvv_pointer);
            break;

        default:
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_INVALID_STORE,
                "Cannot store to type %s", jvar_get_dtype_name(jvvsrc->jvv_dtype));
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
struct jvarvalue * jvar_get_jvv_from_vix(struct jrunexec * jx,
    struct jcontext * jcx,
    int vix)
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
    } else {
        jvv = NULL;
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
struct jvarvalue * jvar_int_prototype_member(struct jrunexec * jx,
    struct jvarvalue_internal_class * jvvi,
    const char * mbrname)
{
/*
** 01/24/2023
*/
    struct jvarvalue *  jvv;
    int * pvix;
    struct jprototype * jpt;

    jvv = NULL;
    if (jvvi) {
        jpt = jvvi->jvvi_prototype;
        if (jpt && jpt->jpt_jvar && jpt->jpt_jcx) {
            pvix = jvar_find_in_jvarrec(jpt->jpt_jvar, mbrname);
            if (pvix) {
                jvv = jvar_get_jvv_from_vix(jx, jpt->jpt_jcx, *pvix);
            }
        }
    }

    return (jvv);
}
/***************************************************************/
struct jvarvalue * jvar_int_object_member(struct jrunexec * jx,
    struct jvarvalue_int_object * jvvo,
    const char * mbrname,
    int mbrflags)
{
/*
** 01/24/2023 - Added mbrflags
*/
    struct jvarvalue *  jvv;
    struct jcontext * jcx;
    int * pvix;
    struct jvarvalue_int_object * njvvo;

    jvv = NULL;

    pvix = NULL;
    njvvo = jvvo;
    while (njvvo && !jvv) {
        if (mbrflags & MBRFLAG_USE_OBJECT) {
            jcx = jvar_get_int_object_jcontext(jx, njvvo);
            pvix = jvar_find_in_jvarrec(jcx->jcx_jvar, mbrname);
            if (pvix) {
                jvv = jvar_get_jvv_from_vix(jx, jcx, *pvix);
            }
        }
        
        if (!jvv && (mbrflags & MBRFLAG_USE_PROTOTYPE)) {
            if (njvvo->jvvo_class.jvv_dtype == JVV_DTYPE_INTERNAL_CLASS) {
                jvv = jvar_int_prototype_member(jx, njvvo->jvvo_class.jvv_val.jvv_jvvi, mbrname);
            }
        }

        if (!jvv && (njvvo->jvvo_superclass.jvv_dtype == JVV_DTYPE_INTERNAL_OBJECT)) {
            njvvo = njvvo->jvvo_superclass.jvv_val.jvv_jvvo;
        } else {
            njvvo = NULL;
        }
    }

    return (jvv);
}
/***************************************************************/
struct jvarvalue * jvar_int_class_member(struct jrunexec * jx,
    struct jvarvalue * cjvv,
    const char * mbrname)
{
/*
** 08/25/2022
*/
    struct jvarvalue *  jvv;
    int * pvix;
    struct jvarvalue_internal_class * jvvi;

    jvv = NULL;

    if (cjvv) {
        if (cjvv->jvv_dtype == JVV_DTYPE_INTERNAL_CLASS) {
            jvvi = cjvv->jvv_val.jvv_jvvi;
            if (!strcmp(mbrname, JVV_PROTOTYPE_NAME)) {
                jvv = jvar_new_jvarvalue();
                jvv->jvv_dtype = JVV_DTYPE_PROTOTYPE;
                jvv->jvv_val.jvv_prototype.jvpt_jpt = jvvi->jvvi_prototype;
                jvv->jvv_flags = JCX_FLAG_PROTOTYPE;
                INCICLASSHREFS(jvvi);
            } else {
                pvix = jvar_find_in_jvarrec(jvvi->jvvi_jvar, mbrname);
#if ALLOW_INT_CLASS_VARS
                if (pvix && (*pvix) >= 0 && (*pvix) < jvvi->jvvi_jcx->jcx_njvv) {
                    jvv = jvvi->jvvi_jcx->jcx_ajvv + (*pvix);
                }
#else
                if (pvix && (*pvix) >= 0 && (*pvix) < jvvi->jvvi_jvar->jvar_nconsts) {
                    jvv = &(jvvi->jvvi_jvar->jvar_aconsts[*pvix]);
                }
#endif
            }
        }
    }

    return (jvv);
}
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
        //jcx->jcx_flags[vix] = 0;
    }

    for (ii = 0; ii < jcx->jcx_jvar->jvar_nvars; ii++) {
#if DEBUG_FREE_VARS
    printf("free data for: %s\n", jvar_get_varname_from_vix(jcx->jcx_jvar, ii));
#endif
        jvar_free_jvarvalue_data(jcx->jcx_ajvv + ii);
        //jcx->jcx_flags[ii] = 0;
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
        //Free(jcx->jcx_flags);
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
    //jcx->jcx_flags = NULL;
    jcx->jcx_jvar = jvar;
    INCVARRECREFS(jvar);
    if (jcx->jcx_xjvv) {
        jcx->jcx_ajvv = New(struct jvarvalue, jcx->jcx_xjvv);
        //jcx->jcx_flags = New(int, jcx->jcx_xjvv);
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
    struct jvarvalue * jvv)
{
    int jstat = 0;
    struct jcontext * jcx;

    jcx = jvar_get_global_jcontext(jx);

    jstat = jvar_new_ad_hoc_variable(jx, jcx, varname, jvv);

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
#if ALLOW_INT_CLASS_VARS
    struct jvarvalue * njvv;
#endif

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
#if ALLOW_INT_CLASS_VARS
        if (cjvv->jvv_dtype != JVV_DTYPE_INTERNAL_CLASS) {
            jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_EXP_INTERNAL_CLASS,
                "Expecting internal class for method: %s", member_name);
        } else {
            njvv = jvar_get_jvv_with_expand(jx, cjvv->jvv_val.jvv_jvvi->jvvi_jcx, vix);
            COPY_JVARVALUE(njvv, mjvv);
        }
#endif
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
int jvar_new_local_function(struct jrunexec * jx,
    struct jtiopenbrace * jtiob,
    struct jvarvalue    * fjvv_child,
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
    int vix)
{
/*
** 03/04/2021
*/
    struct jvarvalue * jvv;

    if (vix >= jcx->jcx_xjvv) {
        if (!jcx->jcx_xjvv) jcx->jcx_xjvv = 4;
        while (vix >= jcx->jcx_xjvv) jcx->jcx_xjvv *= 2;
        jcx->jcx_ajvv = Realloc(jcx->jcx_ajvv, struct jvarvalue, jcx->jcx_xjvv);
        memset(jcx->jcx_ajvv + jcx->jcx_njvv, 0, (jcx->jcx_xjvv - jcx->jcx_njvv) * sizeof(struct jvarvalue));
    }

    if (vix >= jcx->jcx_njvv) {
        jcx->jcx_njvv = vix + 1;
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
    struct jvarvalue * jvv)
{
/*
** 03/01/2021
*/
    int jstat = 0;
    int * pvix;
    int vix;
    struct jvarvalue * tgt_jvv;

    pvix = jvar_find_in_jvarrec(jcx->jcx_jvar, varname);
    if (pvix) {
        vix = *pvix;
    } else {
        vix = jvar_insert_into_jvarrec(jcx->jcx_jvar, varname);
    }
    tgt_jvv = jvar_get_jvv_with_expand(jx, jcx, vix);
#if IS_DEBUG
    if (!tgt_jvv) {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_INTERNAL_ERROR,
            "Internal error: Cannot find new variable: %s", varname);
        return (jstat);
    }
#endif
    if (tgt_jvv) {
        COPY_JVARVALUE(tgt_jvv, jvv);
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

    jvv = NULL;

    pvix = jvar_find_in_jvarrec(local_jcx->jcx_jvar, varname);
    if (pvix) {
        jvv = jvar_get_jvv_from_vix(jx, local_jcx, *pvix);
    }

    return (jvv);
}
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
struct jvarvalue * jvar_get_current_object(struct jrunexec * jx)
{
/*
** 09/13/2022
*/
    struct jvarvalue * this_jvv = NULL;

    if (jx->jx_njfs) {
        this_jvv = jx->jx_jfs[jx->jx_njfs - 1]->jfs_this_jvv;
    }

    return (this_jvv);
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
static struct jvarvalue * jvar_new_jvv_at_vix(struct jrunexec * jx,
    struct jcontext * jcx,
    int vix)
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
    } else {
        jvv = NULL;
    }

    return (jvv);
}
/***************************************************************/
struct jvarvalue * jvar_find_variable(
    struct jrunexec * jx,
    struct jtoken * jtok,
    struct jcontext ** pvar_jcx)
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
            jvv = jvar_get_jvv_from_vix(jx, jcx, *pvix);
            if (!jvv) {
                jvv = jvar_new_jvv_at_vix(jx, jcx, *pvix);
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
    if (cjvv) {
        COPY_JVARVALUE(&(jvvim->jvvim_class), cjvv);
        /* if (cjvv->jvv_dtype == JVV_DTYPE_INTERNAL_CLASS) { */
        /*     INCICLASSHREFS(cjvv->jvv_val.jvv_jvvi);        */
        /* }                                                  */
    } else {
        jvvim->jvvim_class.jvv_dtype = JVV_DTYPE_NONE;
    }
    jvvim->jvvim_nRefs = 0;

    return (jvvim);
}
/***************************************************************/
int jrun_ensure_number(
    struct jrunexec * jx,
    struct jvarvalue * jvv,
    JSFLOAT *fltval,
    int ensflags)
{
/*
** 08/25/2022
*/
    int jstat = 0;
    int cstat;
    int intnum;
    double dblnum;
    struct jvarvalue * rjvv;

    (*fltval) = 0.0;

    switch (jvv->jvv_dtype) {
        
        case JVV_DTYPE_BOOL   :
            (*fltval) = jvv->jvv_val.jvv_val_bool?1.0:0.0;
            break;

        case JVV_DTYPE_JSINT   :
            (*fltval) = (JSFLOAT)(jvv->jvv_val.jvv_val_jsint);
            break;

        case JVV_DTYPE_JSFLOAT   :
            (*fltval) = jvv->jvv_val.jvv_val_jsfloat;
            break;

        case JVV_DTYPE_CHARS   :
            cstat = convert_string_to_number(jvv->jvv_val.jvv_val_chars->jvvc_val_chars, &intnum, &dblnum);
            if (cstat == 1) (*fltval) = (JSFLOAT)intnum;
            else if (cstat == 2) (*fltval) = dblnum;
            else if (ensflags & ENSURE_FLAG_ERROR) {
                jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_CONVERT_TO_NUM,
                    "Expecting number. Found string: %s",        
                    jvv->jvv_val.jvv_val_chars->jvvc_val_chars);
            }
            break;

        case JVV_DTYPE_LVAL:
            jstat = jrun_ensure_number(jx, jvv->jvv_val.jvv_lval.jvvv_lval, fltval, ensflags);
            break;

        case JVV_DTYPE_DYNAMIC:
            jstat = (jvv->jvv_val.jvv_val_dynamic->jvvy_get_proc)(jx,
                jvv->jvv_val.jvv_val_dynamic,
                NULL,
                &rjvv);
            if (!jstat) {
                jstat = jrun_ensure_number(jx, rjvv, fltval, ensflags);
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
            if (ensflags & ENSURE_FLAG_ERROR) {
                jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_CONVERT_TO_NUM,
                    "Expecting number. Found type: %s",        
                    jvar_get_dtype_name(jvv->jvv_dtype));
            }
            break;
    }

    if (!jstat) {
        if (ensflags & ENSURE_FLAG_REQUIRE_INTEGER) {
            JSFLOAT jsflt = floor(*fltval);
            JSINT   jsint = (JSINT)jsflt;
            if (jsflt == (*fltval) && (JSFLOAT)jsint == jsflt) {    /* this range checks */
                (*fltval) = jsflt;
            } else if (ensflags & ENSURE_FLAG_ERROR) {
                jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_CONVERT_TO_NUM,
                    "Expecting integer number. Found: %g", (*fltval));
            } else {
                (*fltval) = 0.0;
            }
        }
    }

    return (jstat);
}
/***************************************************************/
int jrun_ensure_int(
    struct jrunexec * jx,
    struct jvarvalue * jvv,
    JSINT *intval,
    int ensflags)
{
/*
** 05/25/2022
*/
    int jstat = 0;
    JSFLOAT fltval;

    jstat = jrun_ensure_number(jx, jvv, &fltval, ensflags | ENSURE_FLAG_REQUIRE_INTEGER);
    if (!jstat) {
        (*intval) = (JSINT)floor(fltval);
    }

    return (jstat);
}
/***************************************************************/
int jrun_ensure_boolean(
    struct jrunexec * jx,
    struct jvarvalue * jvv,
    int * boolval,
    int ensflags)
{
/*
** 09/07/2022
*/
    int jstat = 0;
    int cstat;
    int boolans;
    struct jvarvalue * rjvv;

    (*boolval) = 0;

    switch (jvv->jvv_dtype) {
        
        case JVV_DTYPE_BOOL   :
            (*boolval) = jvv->jvv_val.jvv_val_bool?1:0;
            break;

        case JVV_DTYPE_JSINT   :
            (*boolval) = jvv->jvv_val.jvv_val_jsint?1:0;
            break;

        case JVV_DTYPE_JSFLOAT   :
            (*boolval) = jvv->jvv_val.jvv_val_jsfloat?1:0;
            break;

        case JVV_DTYPE_CHARS   :
            cstat = convert_string_to_boolean(jvv->jvv_val.jvv_val_chars->jvvc_val_chars, &boolans, ensflags);
            if (cstat == 0) (*boolval) = boolans;
            else if (ensflags & ENSURE_FLAG_ERROR) {
                jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_CONVERT_TO_BOOLEAN,
                    "Expecting boolean. Found string: %s",        
                    jvv->jvv_val.jvv_val_chars->jvvc_val_chars);
            }
            break;

        case JVV_DTYPE_LVAL:
            jstat = jrun_ensure_boolean(jx, jvv->jvv_val.jvv_lval.jvvv_lval, boolval, ensflags);
            break;

        case JVV_DTYPE_DYNAMIC:
            jstat = (jvv->jvv_val.jvv_val_dynamic->jvvy_get_proc)(jx,
                jvv->jvv_val.jvv_val_dynamic,
                NULL,
                &rjvv);
            if (!jstat) {
                jstat = jrun_ensure_boolean(jx, rjvv, boolval, ensflags);
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
            if (ensflags & ENSURE_FLAG_ERROR) {
                jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_CONVERT_TO_BOOLEAN,
                    "Expecting boolean. Found type: %s",        
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
                jvv->jvv_val.jvv_val_chars->jvvc_val_chars,
                jvv->jvv_val.jvv_val_chars->jvvc_length);
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
    Free(jvvy);
}
/***************************************************************/
struct jvarvalue * jvar_next_iterate_jvv(struct jvv_iterator * jvvit)
{
/*
** 03/22/2022
*/
    struct jvarvalue * jvv = NULL;

    if (!(jvvit->jvvit_flags & JVVAIT_FLAG_DONE)) {
        if (jvvit->jvvit_jvv->jvv_dtype == JVV_DTYPE_VALLIST) {
            if (jvvit->jvvit_flags & JVVAIT_FLAG_ADVANCE) {
                jvvit->jvvit_ix += 1;
            } else {
                jvvit->jvvit_flags |= JVVAIT_FLAG_ADVANCE;
            }
            if (jvvit->jvvit_ix < jvvit->jvvit_jvv->jvv_val.jvv_jvvl->jvvl_nvals) {
                jvv = &(jvvit->jvvit_jvv->jvv_val.jvv_jvvl->jvvl_avals[jvvit->jvvit_ix]);
            } else {
                jvvit->jvvit_flags |=  JVVAIT_FLAG_DONE;
            }
        } else {
            if (jvvit->jvvit_flags & JVVAIT_FLAG_ADVANCE) {
                jvvit->jvvit_flags |=  JVVAIT_FLAG_DONE;
            } else {
                jvv = jvvit->jvvit_jvv;
                jvvit->jvvit_flags |= JVVAIT_FLAG_ADVANCE;
            }
        }
    }

    return (jvv);
}
/***************************************************************/
struct jvarvalue * jvar_begin_iterate_jvv(
        int jvvitflags,
        struct jvv_iterator * jvvit,
        struct jvarvalue * jvvlargs)
{
/*
** 03/22/2022
*/
    struct jvarvalue * jvv;

    jvvit->jvvit_ix    = 0;
    jvvit->jvvit_flags = jvvitflags;
    jvvit->jvvit_jvv   = jvvlargs;

    jvv = jvar_next_iterate_jvv(jvvit);

    return (jvv);
}
/***************************************************************/
void jvar_close_iterate_jvv_data(struct jvv_iterator * jvvit)
{
/*
** 03/22/2022
*/
}
/***************************************************************/
struct jvarvalue_pointer * jvar_new_jvarvalue_pointer(void)
{
/*
** 08/26/2022
*/
    struct jvarvalue_pointer * jvvr;

    jvvr = New(struct jvarvalue_pointer, 1);
    jvvr->jvvr_jvv.jvv_dtype = JVV_DTYPE_NONE;
    jvvr->jvvr_jvv.jvv_val.jvv_void = NULL;
    jvvr->jvvr_nRefs = 1;

    return (jvvr);
}
/***************************************************************/
void jvar_free_jvarvalue_pointer(struct jvarvalue_pointer * jvvr)
{
/*
** 08/26/2022
*/
    DECPOINTERREFS(jvvr);
    if (!jvvr->jvvr_nRefs) {
        jvar_free_jvarvalue_data(&(jvvr->jvvr_jvv));
        Free(jvvr);
    }
}
/***************************************************************/
int jvar_calc_typeof(struct jrunexec * jx,
        struct jvarvalue * jvv,
        char ** typeofstr,
        int   * typeoflen)
{
/*
** 08/29/2022
**
** typeof
*/
    int jstat = 0;
    int typeofmax;
    struct jvarvalue * tgtjvv;

    (*typeofstr) = NULL;
    (*typeoflen) = 0;
    typeofmax    = 0;

    switch (jvv->jvv_dtype) {
        case JVV_DTYPE_NONE  :
            append_charval(typeofstr, typeoflen, &typeofmax, "undefined");
            break;

        case JVV_DTYPE_BOOL  :
            append_charval(typeofstr, typeoflen, &typeofmax, "boolean");
            break;

        case JVV_DTYPE_JSINT   :
            append_charval(typeofstr, typeoflen, &typeofmax, "number");
            break;

        case JVV_DTYPE_JSFLOAT:
            append_charval(typeofstr, typeoflen, &typeofmax, "number");
            break;

        case JVV_DTYPE_LVAL :
            jstat = jvar_calc_typeof(jx, jvv->jvv_val.jvv_lval.jvvv_lval, typeofstr, typeoflen);
            break;

        case JVV_DTYPE_CHARS :
            append_charval(typeofstr, typeoflen, &typeofmax, "string");
            break;

        case JVV_DTYPE_INTERNAL_CLASS :
            append_charval(typeofstr, typeoflen, &typeofmax, "function");
            break;

        case JVV_DTYPE_INTERNAL_OBJECT :
            append_charval(typeofstr, typeoflen, &typeofmax, "object");
            break;

        case JVV_DTYPE_INTERNAL_METHOD:
            append_charval(typeofstr, typeoflen, &typeofmax, "function");
            break;

        case JVV_DTYPE_TOKEN :
            append_charval(typeofstr, typeoflen, &typeofmax, "JVV_DTYPE_TOKEN");
            break;

        case JVV_DTYPE_VALLIST:
            append_charval(typeofstr, typeoflen, &typeofmax, "JVV_DTYPE_VALLIST");
            break;

        case JVV_DTYPE_FUNCTION:
            append_charval(typeofstr, typeoflen, &typeofmax, "JVV_DTYPE_FUNCTION");
            break;

        case JVV_DTYPE_FUNCVAR:
            append_charval(typeofstr, typeoflen, &typeofmax, "function");
            break;
        
        case JVV_DTYPE_IMETHVAR:
            append_charval(typeofstr, typeoflen, &typeofmax, "function");
            break;

        case JVV_DTYPE_ARRAY :
            append_charval(typeofstr, typeoflen, &typeofmax, "JVV_DTYPE_ARRAY");
            break;
        
        case JVV_DTYPE_OBJPTR:
            switch (jvv->jvv_val.jvv_objptr.jvvp_pval->jvv_dtype) {
                case JVV_DTYPE_ARRAY   :
                    append_charval(typeofstr, typeoflen, &typeofmax, "object");
                    break;
                case JVV_DTYPE_OBJECT   :
                    append_charval(typeofstr, typeoflen, &typeofmax, "object");
                    break;
                default:
                    append_charval(typeofstr, typeoflen, &typeofmax, "*unsupported object pointer type*");
                    break;
            }
            break;

        case JVV_DTYPE_DYNAMIC :
            jstat = (jvv->jvv_val.jvv_val_dynamic->jvvy_get_proc)(jx,
                     jvv->jvv_val.jvv_val_dynamic, NULL, &tgtjvv);
            if (!jstat) {
                jstat = jvar_calc_typeof(jx, tgtjvv, typeofstr, typeoflen);
            }
            break;

        case JVV_DTYPE_OBJECT :
            append_charval(typeofstr, typeoflen, &typeofmax, "JVV_DTYPE_OBJECT");
            break;

        case JVV_DTYPE_POINTER   :
            append_charval(typeofstr, typeoflen, &typeofmax, "object");
            break;

        default:
            append_charval(typeofstr, typeoflen, &typeofmax, "*unsupported type*");
            break;
    }

    return (jstat);
}
/***************************************************************/
