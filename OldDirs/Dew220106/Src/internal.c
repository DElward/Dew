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
        struct jvarvalue * superclass,
        JVAR_FREE_OBJECT destructor,
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
    if (!superclass) {
        INIT_JVARVALUE(&(jvv->jvv_val.jvv_jvvi->jvvi_superclass));
    } else {
        COPY_JVARVALUE(&(jvv->jvv_val.jvv_jvvi->jvvi_superclass), superclass);
    }
    jvv->jvv_val.jvv_jvvi->jvvi_destructor = destructor;
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
    FILE * outf,
    const char * outstr)

{
/*
**
*/
    int jstat = 0;

    fprintf(outf, "%s", outstr);

    return (jstat);
}
/***************************************************************/
static int jint_console_output(
        struct jrunexec  * jx,
        FILE             * outf,
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

    pbuf    = NULL;
    pbuflen = 0;
    pbufmax = 0;

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
        jstat = jrun_puts(jx, outf, pbuf);
    }

    Free(pbuf);

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
** 11/22/2021
*/
    int jstat = 0;
    struct jintobj_console * jioc = (struct jintobj_console *)this_obj;

    INIT_JVARVALUE(jvvrtn);

    if (!this_obj) {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_NULL_OBJECT,
            "Null object passed to %s()", func_name);
        return (jstat);
    }

    jstat = jint_console_output(jx, jioc->jioc_outf, jvvlargs, jvvrtn);

    return (jstat);
}
/***************************************************************/
static int jint_Console_error(
        struct jrunexec  * jx,
        const char       * func_name,
        void             * this_obj,
        struct jvarvalue * jvvlargs,
        struct jvarvalue * jvvrtn)
{
/*
** 11/22/2021
*/
    int jstat = 0;
    struct jintobj_console * jioc = (struct jintobj_console *)this_obj;

    INIT_JVARVALUE(jvvrtn);

    if (!this_obj) {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_NULL_OBJECT,
            "Null object passed to %s()", func_name);
        return (jstat);
    }

    jstat = jint_console_output(jx, jioc->jioc_errf, jvvlargs, jvvrtn);

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

    INCICLASSHREFS(jvv->jvv_val.jvv_jvvi);
    if (jvv->jvv_val.jvv_jvvi->jvvi_superclass.jvv_dtype == JVV_DTYPE_INTERNAL_CLASS) {
        INCICLASSHREFS(jvv->jvv_val.jvv_jvvi->jvvi_superclass.jvv_val.jvv_jvvi);
    }

    vnam = jvv->jvv_val.jvv_jvvi->jvvi_class_name;
    jstat = jvar_insert_new_global_variable(jx, vnam, jvv, 0);

    return (jstat);
}
/***************************************************************/
void jvar_init_jvarvalue_int_object(
    struct jrunexec * jx,
    struct jvarvalue * cjvv,
    void * this_ptr,
    struct jvarvalue_int_object * jvvo)
{
#if IS_DEBUG
    static int next_jvvo_sn = 0;
#endif

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
}
/***************************************************************/
struct jvarvalue_int_object * jvar_new_jvarvalue_int_object(
    struct jrunexec * jx,
    struct jvarvalue * cjvv,
    void * this_ptr)
{
    struct jvarvalue_int_object * jvvo;

    jvvo = New(struct jvarvalue_int_object, 1);

    jvar_init_jvarvalue_int_object(jx, cjvv, this_ptr, jvvo);
    if (cjvv->jvv_val.jvv_jvvi->jvvi_superclass.jvv_dtype == JVV_DTYPE_INTERNAL_CLASS) {
        jvar_init_jvarvalue(&(jvvo->jvvo_superclass));
        jvvo->jvvo_superclass.jvv_dtype = JVV_DTYPE_INTERNAL_OBJECT;
        jvvo->jvvo_superclass.jvv_val.jvv_jvvo = jvar_new_jvarvalue_int_object(jx,
            &(cjvv->jvv_val.jvv_jvvi->jvvi_superclass), this_ptr);
    } else {
        INIT_JVARVALUE(&(jvvo->jvvo_superclass));
    }

    return (jvvo);
}
/***************************************************************/
int jrun_add_internal_object(struct jrunexec * jx,
    const char * varname,
    struct jvarvalue * cjvv,
    void * this_ptr)
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
    obj_jvv.jvv_val.jvv_jvvo = jvar_new_jvarvalue_int_object(jx, cjvv, this_ptr);

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
    jioc->jioc_errf = stderr;

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

    jrun_init_internal_class(jx, JVV_INTERNAL_CLASS_Console, NULL,
            jint_free_jintobj_console,  &jvv);
    /* svare_new_int_class_method(jvv, ".new"              , svare_method_NN_new); */
    jstat = jrun_new_internal_class_method(jx, &jvv, "log"           , jint_Console_log  , JCX_FLAG_METHOD);
    jstat = jrun_new_internal_class_method(jx, &jvv, "error"         , jint_Console_error, JCX_FLAG_METHOD);

    if (!jstat) jstat = jrun_add_internal_class(jx, &jvv);

    if (!jstat) {
        jstat = jrun_add_internal_object(jx, JVV_INTERNAL_VAR_console, &jvv, jioc);
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
    obj_jvv->jvv_val.jvv_jvvo = jvar_new_jvarvalue_int_object(jx, cjvv, NULL);

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

    jrun_init_internal_class(jx, JVV_INTERNAL_TYPE_CLASS_String, NULL, NULL, &jvv);

    jstat = jrun_new_internal_class_method(jx, &jvv, "length"     , jint_String_length     , JCX_FLAG_PROPERTY);
    if (!jstat) {
        jstat = jrun_new_internal_class_method(jx, &jvv, "substring"  , jint_String_substring  , JCX_FLAG_METHOD);
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

    jrun_init_internal_class(jx, JVV_INTERNAL_TYPE_CLASS_Array, NULL, NULL, &jvv);

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
void jint_free_jintobj_Error(void * vjioe)
{
/*
** 11/11/2021
*/
    struct jintobj_Error * jioe = (struct jintobj_Error *)vjioe;

    Free(jioe->jioe_msg);
    Free(jioe);
}
/***************************************************************/
int jrun_load_Error(struct jrunexec * jx)
{
/*
** 11/10/2021
*/
    int jstat = 0;
    struct jvarvalue jvv;
    struct jvarvalue jvv_Error;

    jrun_init_internal_class(jx, JVV_INTERNAL_CLASS_Error, NULL, jint_free_jintobj_Error, &jvv_Error);
    jstat = jrun_new_internal_class_method(jx, &jvv_Error, JVV_INTERNAL_METHOD_toString, jint_Error_toString, JCX_FLAG_METHOD);
    if (!jstat) jstat = jrun_add_internal_class(jx, &jvv_Error);

    if (!jstat) jrun_init_internal_class(jx, JVV_INTERNAL_CLASS_AggregateError, &jvv_Error, NULL, &jvv);
    if (!jstat) jstat = jrun_add_internal_class(jx, &jvv);

    if (!jstat) jrun_init_internal_class(jx, JVV_INTERNAL_CLASS_EvalError, &jvv_Error, NULL, &jvv);
    if (!jstat) jstat = jrun_add_internal_class(jx, &jvv);

    if (!jstat) jrun_init_internal_class(jx, JVV_INTERNAL_CLASS_InternalError, &jvv_Error, NULL, &jvv);
    if (!jstat) jstat = jrun_add_internal_class(jx, &jvv);

    if (!jstat) jrun_init_internal_class(jx, JVV_INTERNAL_CLASS_RangeError, &jvv_Error, NULL, &jvv);
    if (!jstat) jstat = jrun_add_internal_class(jx, &jvv);

    if (!jstat) jrun_init_internal_class(jx, JVV_INTERNAL_CLASS_ReferenceError, &jvv_Error, NULL, &jvv);
    if (!jstat) jstat = jrun_add_internal_class(jx, &jvv);

    if (!jstat) jrun_init_internal_class(jx, JVV_INTERNAL_CLASS_SyntaxError, &jvv_Error, NULL, &jvv);
    if (!jstat) jstat = jrun_add_internal_class(jx, &jvv);

    if (!jstat) jrun_init_internal_class(jx, JVV_INTERNAL_CLASS_TypeError, &jvv_Error, NULL, &jvv);
    if (!jstat) jstat = jrun_add_internal_class(jx, &jvv);

    if (!jstat) jrun_init_internal_class(jx, JVV_INTERNAL_CLASS_URIError, &jvv_Error, NULL, &jvv);
    if (!jstat) jstat = jrun_add_internal_class(jx, &jvv);

    if (!jstat) jrun_init_internal_class(jx, JVV_INTERNAL_CLASS_UnimplementedError, &jvv_Error, NULL, &jvv);
    if (!jstat) jstat = jrun_add_internal_class(jx, &jvv);

    return (jstat);
}
/***************************************************************/
struct jvarvalue_int_object * jvar_new_jvarvalue_int_object_classname(
    struct jrunexec * jx,
    const char * classname,
    void * this_ptr)
{
/*
** 11/11/2021
*/
    struct jvarvalue_int_object * jvvo = NULL;
    struct jvarvalue * cjvv;
    struct jcontext * global_jcx;

    global_jcx = jvar_get_global_jcontext(jx);

    cjvv = jvar_find_local_variable(jx, global_jcx, classname);
    if (cjvv && cjvv->jvv_dtype == JVV_DTYPE_INTERNAL_CLASS) {
        jvvo = jvar_new_jvarvalue_int_object(jx, cjvv, this_ptr);
    }

    return (jvvo);
}
/***************************************************************/
static int jint_isNaN(
        struct jrunexec  * jx,
        struct jvarvalue * jvv)
{
/*
** 11/16/2021
*/
    int is_nan;
    int cstat;
    int intnum;
    double dblnum;

    is_nan = 1;

    switch (jvv->jvv_dtype) {
        case JVV_DTYPE_BOOL   :
            is_nan = 0;
            break;

        case JVV_DTYPE_JSINT   :
            is_nan = 0;
            break;

        case JVV_DTYPE_JSFLOAT   :
            is_nan = 0;
            break;

        case JVV_DTYPE_CHARS   :
            cstat = convert_string_to_number(jvv->jvv_val.jvv_val_chars.jvvc_val_chars, &intnum, &dblnum);
            if (cstat == 1 || cstat == 2) is_nan = 0;
            break;

        case JVV_DTYPE_LVAL:
            is_nan = jint_isNaN(jx, jvv->jvv_val.jvv_lval.jvvv_lval);
            break;

        default:
            break;
    }

    return (is_nan);
}
/***************************************************************/
static int jint_function_isNaN(
        struct jrunexec  * jx,
        const char       * func_name,
        void             * this_obj,
        struct jvarvalue * jvvlargs,
        struct jvarvalue * jvvrtn)
{
/*
** 11/16/2021
*/
    int jstat = 0;
    int is_nan;

    if (jvvlargs->jvv_val.jvv_jvvl->jvvl_nvals == 1) {
        is_nan = jint_isNaN(jx, &(jvvlargs->jvv_val.jvv_jvvl->jvvl_avals[0]));
    } else {
        is_nan = 1;
    }

    if (!jstat) {
        jvvrtn->jvv_dtype = JVV_DTYPE_BOOL;
        jvvrtn->jvv_val.jvv_val_bool = is_nan;
    }

    return (jstat);
}
/***************************************************************/
static int jrun_new_internal_function(
    struct jrunexec * jx,
    const char * method_name,
    JVAR_INTERNAL_FUNCTION ifuncptr,
    int jcxflags )

{
/*
** 11/16/2021
*/
    int jstat = 0;
    struct jvarvalue fjvv;
    struct jvarvalue_int_method * jvvim;

    jvar_init_jvarvalue(&fjvv);
    fjvv.jvv_dtype = JVV_DTYPE_INTERNAL_METHOD;
    jvvim = jvar_new_int_method(method_name, ifuncptr, NULL);
    fjvv.jvv_val.jvv_int_method = jvvim;
    INCIMETHREFS(jvvim);

    jstat = jvar_insert_new_global_variable(jx, method_name, &fjvv, 0);

    return (jstat);
}
/***************************************************************/
int jrun_load_internal_functions(struct jrunexec * jx)
{
/*
** 11/16/2021
*/
    int jstat = 0;

    jstat = jrun_new_internal_function(jx, "isNaN"         , jint_function_isNaN, JCX_FLAG_FUNCTION);

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
        if (!jstat) jstat = jrun_load_Error(jse->jse_jx);

        if (!jstat) {
            jstat = jrun_load_internal_functions(jse->jse_jx);
        }

        if (!jstat) {
            jstat = jrun_load_type_methods(jse->jse_jx);
        }
    }

    return (jstat);
}
/***************************************************************/
int jexp_internal_method_call(
                    struct jrunexec * jx,
                    struct jvarvalue_int_method * imeth,
                    void             * this_obj,
                    struct jvarvalue * jvvargs,
                    struct jvarvalue * rtnjvv)
{
/*
** 03/08/2021
*/
    int jstat = 0;

    // jstat = jrun_push_jfuncstate(jx, fjvv, NULL);

    jstat = (imeth->jvvim_method)(jx, imeth->jvvim_method_name, this_obj, jvvargs, rtnjvv);

    // if (!jstat) jstat = jrun_pop_jfuncstate(jx);

    return (jstat);
}
/***************************************************************/
