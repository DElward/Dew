/***************************************************************/
/*
**  internal.c --  Internal classes
*/
/***************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>

#include "dew.h"
#include "jsenv.h"
#include "error.h"
#include "jstok.h"
#include "jsexp.h"
#include "dbtreeh.h"
#include "snprfh.h"
#include "utl.h"
#include "internal.h"
#include "var.h"
#include "print.h"
#include "array.h"

/***************************************************************/
static void jrun_init_internal_class(
        struct jrunexec  * jx,
        const char       * class_name,
        struct jvarvalue * jvv)
{
/*
**
*/
    jvar_init_jvarvalue(jvv);
    jvv->jvv_dtype = JVV_DTYPE_INTERNAL_CLASS;
    jvv->jvv_val.jvv_jvvi = New(struct jvarvalue_internal_class, 1);
    jvv->jvv_val.jvv_jvvi->jvvi_jvar = jvar_new_jvarrec();
    INCVARRECREFS(jvv->jvv_val.jvv_jvvi->jvvi_jvar);
    jvv->jvv_val.jvv_jvvi->jvvi_nRefs = 0;
    jvv->jvv_val.jvv_jvvi->jvvi_class_name = Strdup(class_name);
}
/***************************************************************/
#if OLD_PRINT & 4
int jrun_int_tostring(
    struct jrunexec * jx,
    struct jvarvalue * jvv,
    char ** outstr)
{
/*
**
*/
    int jstat = 0;
    char * out;
    char buf[32];

    out = NULL;
    jvv = GET_RVAL(jvv);

    switch (jvv->jvv_dtype) {
        case JVV_DTYPE_NONE  :
             out = Strdup("undefined");
            break;
        case JVV_DTYPE_BOOL  :
            if (jvv->jvv_val.jvv_val_bool) out = Strdup("true");
            else                           out = Strdup("false");
            break;
        case JVV_DTYPE_JSINT   :
            sprintf(buf, "%d", (int)jvv->jvv_val.jvv_val_jsint);
            out = Strdup(buf);
            break;
        case JVV_DTYPE_JSFLOAT:
            sprintf(buf, "%g", (double)jvv->jvv_val.jvv_val_jsfloat);
            out = Strdup(buf);
            break;
        case JVV_DTYPE_CHARS :
            if (jvv->jvv_val.jvv_val_chars.jvvc_length) {
                out = New(char, jvv->jvv_val.jvv_val_chars.jvvc_length + 1);
                strcpy(out, jvv->jvv_val.jvv_val_chars.jvvc_val_chars);
            }
            break;
        default:
            break;
    }

    (*outstr) = out;

    return (jstat);
}
/***************************************************************/
#else
int jrun_int_tostring(
    struct jrunexec * jx,
    struct jvarvalue * jvv,
    char ** outstr)
{
/*
** See also: jexp_quick_tostring(struct jrunexec * jx, struct jvarvalue * jvv, char * valstr, int valstrsz)
**                  jexp_print_stack()
** See also: jpr_jvarvalue_tostring(char ** prbuf, int * prlen, int * prmax, struct jvarvalue * jvv, int vflags)
**                  jpr_debug_show_jcontext()
** See also: jvarvalue_to_chars(struct jrunexec * jx, struct jvarvalue * jvvtgt, struct jvarvalue * jvvsrc)
**                  jexp_binop_add()
** See also: jrun_int_tostring(struct jrunexec * jx, struct jvarvalue * jvv, char ** outstr)
**                  jint_Console_log()
*/
    int jstat = 0;
    int prlen = 0;
    int prmax = 0;

    (*outstr) = NULL;
    jstat = jpr_jvarvalue_tostring(jx, outstr, &prlen, &prmax, jvv, JPR_FLAGS_LOG);

    return (jstat);
}
#endif
/***************************************************************/
static int jrun_puts(
    struct jrunexec * jx,
    struct jintobj_console * jioc,
    const char * outstr)

{
/*
**
*/
    int jstat = 0;

    fprintf(jioc->jioc_outf, "%s", outstr);

    return (jstat);
}
/***************************************************************/
static int jint_Console_log(
        struct jrunexec  * jx,
        const char       * func_name,
        void             * this_obj,
        struct jvarvalue * jvvlargs,
        struct jvarvalue * jvvrtn)
{
/*
**
*/
    int jstat = 0;
    int ii;
    char * logstr;
    char * pbuf;
    int    pbuflen;
    int    pbufmax;
    struct jintobj_console * jioc = (struct jintobj_console *)this_obj;

    pbuf    = NULL;
    pbuflen = 0;
    pbufmax = 0;

    if (!this_obj) {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_NULL_OBJECT,
            "Null object passed to %s()", func_name);
        return (jstat);
    }

    for (ii = 0; !jstat && ii < jvvlargs->jvv_val.jvv_jvvl->jvvl_nvals; ii++) {
        if (ii) append_chars(&pbuf, &pbuflen, &pbufmax, " ", 1);
        jstat = jrun_int_tostring(jx, &(jvvlargs->jvv_val.jvv_jvvl->jvvl_avals[ii]), &logstr);
        if (!jstat && logstr) {
            append_chars(&pbuf, &pbuflen, &pbufmax, logstr, IStrlen(logstr));
        }
        Free(logstr);
    }
    if (!jstat) {
        append_chars(&pbuf, &pbuflen, &pbufmax, "\n", 1);
        jstat = jrun_puts(jx, jioc, pbuf);
    }

    Free(pbuf);

    return (jstat);
}
/***************************************************************/
static int jrun_new_internal_class_method(
    struct jrunexec * jx,
    struct jvarvalue * jvv,
    const char * method_name,
    JVAR_INTERNAL_FUNCTION ifuncptr,
    int jcxflags )

{
/*
**
*/
    int jstat = 0;
    struct jvarvalue fjvv;
    struct jvarvalue_int_method * jvvim;

    if (jvv->jvv_dtype != JVV_DTYPE_INTERNAL_CLASS) {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_EXP_INTERNAL_CLASS,
            "Expecting internal class for method");
        return (jstat);
    }

    jvar_init_jvarvalue(&fjvv);
    fjvv.jvv_dtype = JVV_DTYPE_INTERNAL_METHOD;
    jvvim = jvar_new_int_method(method_name, ifuncptr, jvv);
    fjvv.jvv_val.jvv_int_method = jvvim;
    INCIMETHREFS(jvvim);

    jstat = jvar_add_class_method(jx, jvv, method_name, &fjvv, jcxflags);

    return (jstat);
}
/***************************************************************/
int jrun_add_internal_class(
    struct jrunexec * jx,
    struct jvarvalue * jvv)
{
/*
**
*/
    int jstat = 0;
    char * vnam;

    if (jvv->jvv_dtype != JVV_DTYPE_INTERNAL_CLASS) {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_EXP_INTERNAL_CLASS,
            "Expecting internal class to add");
        return (jstat);
    }

    vnam = jvv->jvv_val.jvv_jvvi->jvvi_class_name;
    jstat = jvar_insert_new_global_variable(jx, vnam, jvv, 0);

    return (jstat);
}
/***************************************************************/
struct jvarvalue_int_object * jvar_new_jvarvalue_int_object(
    struct jrunexec * jx,
    struct jvarvalue * cjvv,
    void * this_ptr,
    JVAR_FREE_OBJECT free_func)
{
    struct jvarvalue_int_object * jvvo;
#if IS_DEBUG
    static int next_jvvo_sn = 0;
#endif

    jvvo = New(struct jvarvalue_int_object, 1);
#if IS_DEBUG
    jvvo->jvvo_sn[0] = 'O';
    jvvo->jvvo_sn[1] = (next_jvvo_sn / 100) % 10 + '0';
    jvvo->jvvo_sn[2] = (next_jvvo_sn /  10) % 10 + '0';
    jvvo->jvvo_sn[3] = (next_jvvo_sn      ) % 10 + '0';
    next_jvvo_sn++;
#endif

    COPY_JVARVALUE(&(jvvo->jvvo_class), cjvv);
    INCICLASSHREFS(cjvv->jvv_val.jvv_jvvi);
    jvvo->jvvo_jcx   = jvar_new_jcontext(jx, cjvv->jvv_val.jvv_jvvi->jvvi_jvar, NULL);
    jvvo->jvvo_this_ptr = this_ptr;
    jvvo->jvvo_free_func = free_func;

    return (jvvo);
}
/***************************************************************/
int jrun_add_internal_object(struct jrunexec * jx,
    const char * varname,
    struct jvarvalue * cjvv,
    void * this_ptr,
    JVAR_FREE_OBJECT free_func)
{
    int jstat = 0;
    struct jvarvalue obj_jvv;

    if (cjvv->jvv_dtype != JVV_DTYPE_INTERNAL_CLASS) {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_EXP_INTERNAL_CLASS,
            "Expecting internal class to add");
        return (jstat);
    }

    jvar_init_jvarvalue(&obj_jvv);
    obj_jvv.jvv_dtype = JVV_DTYPE_INTERNAL_OBJECT;
    obj_jvv.jvv_val.jvv_jvvo = jvar_new_jvarvalue_int_object(jx, cjvv, this_ptr, free_func);

    jstat = jvar_insert_new_global_variable(jx, varname, &obj_jvv, 0);

    return (jstat);
}
/***************************************************************/
struct jintobj_console * jint_new_jintobj_console(void)
{
    struct jintobj_console * jioc;
#if IS_DEBUG
    static int next_jioc_sn = 0;
#endif

    jioc = New(struct jintobj_console, 1);
#if IS_DEBUG
    jioc->jioc_sn[0] = 'c';
    jioc->jioc_sn[1] = (next_jioc_sn / 100) % 10 + '0';
    jioc->jioc_sn[2] = (next_jioc_sn /  10) % 10 + '0';
    jioc->jioc_sn[3] = (next_jioc_sn      ) % 10 + '0';
    next_jioc_sn++;
#endif

    jioc->jioc_outf = stdout;

    return (jioc);
}
/***************************************************************/
void jint_free_jintobj_console(void * vjioc)
{
    struct jintobj_console * jioc = (struct jintobj_console *)vjioc;

    Free(jioc);
}
/***************************************************************/
int jrun_load_Console(struct jrunexec * jx)
{
/*
**
*/
    int jstat = 0;
    struct jvarvalue jvv;
    struct jintobj_console * jioc;

    jioc = jint_new_jintobj_console();

    jrun_init_internal_class(jx, JVV_INTERNAL_CLASS_CONSOLE, &jvv);
    /* svare_new_int_class_method(jvv, ".new"              , svare_method_NN_new); */
    jstat = jrun_new_internal_class_method(jx, &jvv, "log"           , jint_Console_log, JCX_FLAG_FUNCTION);

    if (!jstat) jstat = jrun_add_internal_class(jx, &jvv);

    if (!jstat) {
        jstat = jrun_add_internal_object(jx, JVV_INTERNAL_VAR_CONSOLE,
                &jvv, jioc, jint_free_jintobj_console);
    }

    return (jstat);
}
/***************************************************************/
static int jint_String_length(
        struct jrunexec  * jx,
        const char       * func_name,
        void             * this_obj,
        struct jvarvalue * jvvlargs,
        struct jvarvalue * jvvrtn)
{
/*
**
*/
    int jstat = 0;
    struct jvarvalue_chars * jvvc = (struct jvarvalue_chars *)this_obj;

    if (!this_obj) {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_NULL_OBJECT,
            "Null object passed to %s()", func_name);
        return (jstat);
    }

    jvvrtn->jvv_dtype = JVV_DTYPE_JSINT;
    jvvrtn->jvv_val.jvv_val_jsint = jvvc->jvvc_length;

    return (jstat);
}
/***************************************************************/
static int jint_String_substring(
        struct jrunexec  * jx,
        const char       * func_name,
        void             * this_obj,
        struct jvarvalue * jvvlargs,
        struct jvarvalue * jvvrtn)
{
/*
** 10/07/2021
*/
    int jstat = 0;
    int ii;
    int firstix;
    int endix;
    int intval;
    struct jvarvalue_chars * jvvc = (struct jvarvalue_chars *)this_obj;

    if (!this_obj) {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_NULL_OBJECT,
            "Null object passed to %s()", func_name);
        return (jstat);
    }

    firstix = 0;
    endix = jvvc->jvvc_length;

    for (ii = 0; !jstat && ii < jvvlargs->jvv_val.jvv_jvvl->jvvl_nvals; ii++) {
        if (ii < 2) {
            jstat = jrun_ensure_int(jx, &(jvvlargs->jvv_val.jvv_jvvl->jvvl_avals[ii]), &intval, 0);
            if (!jstat) {
                     if (ii == 0) firstix = intval;
                else if (ii == 1) endix   = intval;
            }
        }
    }

    if (!jstat) {
        if (firstix < 0) firstix = 0;
        if (endix > jvvc->jvvc_length) endix = jvvc->jvvc_length;
        if (endix > firstix) {
            jvar_store_chars_len(jvvrtn, jvvc->jvvc_val_chars + firstix, endix - firstix);
        } else {
            jvar_store_chars_len(jvvrtn, "", 0);
        }
    }

    return (jstat);
}
/***************************************************************/
int jvar_insert_new_type_object(struct jrunexec * jx,
    enum e_jvv_type dtype,
    struct jvarvalue * jvv)
{
    int jstat = 0;
    int tix;

    tix = JVV_DTYPE_INDEX(dtype);

    if (jx->jx_type_objs[tix] == NULL) {
        jx->jx_type_objs[tix] = jvv;
    } else {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_EXP_INTERNAL_CLASS,
            "Duplicate internal type class to add");
        return (jstat);
    }

    return (jstat);
}
/***************************************************************/
int jrun_add_internal_type_object(struct jrunexec * jx,
    enum e_jvv_type dtype,
    struct jvarvalue * cjvv)
{
/*
** 09/28/2021
*/
    int jstat = 0;
    struct jvarvalue * obj_jvv;

    if (cjvv->jvv_dtype != JVV_DTYPE_INTERNAL_CLASS) {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_EXP_INTERNAL_CLASS,
            "Expecting internal class to add");
        return (jstat);
    }

    obj_jvv = jvar_new_jvarvalue();
    obj_jvv->jvv_dtype = JVV_DTYPE_INTERNAL_OBJECT;
    obj_jvv->jvv_val.jvv_jvvo = jvar_new_jvarvalue_int_object(jx, cjvv, NULL, NULL);

    jstat = jvar_insert_new_type_object(jx, dtype, obj_jvv);

    return (jstat);
}
/***************************************************************/
int jrun_load_type_String(struct jrunexec * jx)
{
/*
** 09/28/2021
*/
    int jstat = 0;
    struct jvarvalue jvv;

    jrun_init_internal_class(jx, JVV_INTERNAL_TYPE_CLASS_STRING, &jvv);

    jstat = jrun_new_internal_class_method(jx, &jvv, "length"     , jint_String_length     , JCX_FLAG_PROPERTY);
    if (!jstat) {
        jstat = jrun_new_internal_class_method(jx, &jvv, "substring"  , jint_String_substring  , JCX_FLAG_FUNCTION);
    }

    if (!jstat) jstat = jrun_add_internal_class(jx, &jvv);

    if (!jstat) jstat = jrun_add_internal_type_object(jx, JVV_DTYPE_CHARS, &jvv);

    return (jstat);
}
/***************************************************************/
int jrun_load_type_Array(struct jrunexec * jx)
{
/*
** 10/21/2021
*/
    int jstat = 0;
    struct jvarvalue jvv;

    jrun_init_internal_class(jx, JVV_INTERNAL_TYPE_CLASS_ARRAY, &jvv);

    jstat = jrun_new_internal_class_method(jx, &jvv, "length"     , jint_Array_length     , JCX_FLAG_PROPERTY);
    if (!jstat) {
        jstat = jrun_new_internal_class_method(jx, &jvv, JVV_BRACKET_OPERATION  , jint_Array_subscript  , JCX_FLAG_OPERATION);
    }

    if (!jstat) jstat = jrun_add_internal_class(jx, &jvv);

    if (!jstat) jstat = jrun_add_internal_type_object(jx, JVV_DTYPE_ARRAY, &jvv);

    return (jstat);
}
/***************************************************************/
int jrun_load_type_methods(struct jrunexec * jx)
{
/*
** 09/28/2021
*/
    int jstat = 0;

    jstat = jrun_load_type_String(jx);
    if (!jstat) {
        jstat = jrun_load_type_Array(jx);
    }

    return (jstat);
}
/***************************************************************/
int jenv_load_internal_classes(struct jsenv * jse)
{
/*
**
*/
    int jstat = 0;

    if (jse->jse_flags & JENFLAG_NODEJS) {
        jstat = jrun_load_Console(jse->jse_jx);

        if (!jstat) {
            jstat = jrun_load_type_methods(jse->jse_jx);
        }
    }

    return (jstat);
}
/***************************************************************/
