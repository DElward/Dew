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
#include "jstok.h"
#include "jsexp.h"
#include "dbtreeh.h"
#include "snprfh.h"
#include "utl.h"
#include "internal.h"
#include "var.h"

/***************************************************************/
static struct jvarvalue * jrun_new_internal_class(
        struct jrunexec  * jx,
        const char       * class_name)
{
/*
**
*/
    struct jvarvalue * jvv;

    jvv = jvar_new_jvarvalue();
    jvv->jvv_dtype = JVV_DTYPE_INTERNAL_CLASS;
    jvv->jvv_val.jvv_jvvi = New(struct jvarvalue_internal_class, 1);
    jvv->jvv_val.jvv_jvvi->jvvi_jvar = jvar_new_jvarrec();
    jvv->jvv_val.jvv_jvvi->jvvi_nrefs = 0;
    jvv->jvv_val.jvv_jvvi->jvvi_class_name = Strdup(class_name);

    return (jvv);
}
/***************************************************************/
int jrun_int_tostring(
    struct jrunexec * jx,
    struct jvarvalue * jvv,
    char ** outstr)
{
/*
** See also: jvarvalue_to_chars()
*/
    int jstat = 0;
    char * out;
    char buf[32];

    out = NULL;

    switch (jvv->jvv_dtype) {
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
static int jrun_puts(
    struct jrunexec * jx,
    const char * outstr)

{
/*
**
*/
    int jstat = 0;

    printf("%s", outstr);

    return (jstat);
}
/***************************************************************/
static int jint_Console_log(
        struct jrunexec  * jx,
        const char       * func_name,
        struct jvarvalue_vallist * jvvlargs,
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

    for (ii = 0; !jstat && ii < jvvlargs->jvvl_nvals; ii++) {
        if (ii) append_chars(&pbuf, &pbuflen, &pbufmax, " ", 1);
        jstat = jrun_int_tostring(jx, &(jvvlargs->jvvl_avals[ii]), &logstr);
        if (!jstat && logstr) {
            append_chars(&pbuf, &pbuflen, &pbufmax, logstr, IStrlen(logstr));
        }
        Free(logstr);
    }
    if (!jstat) {
        append_chars(&pbuf, &pbuflen, &pbufmax, "\n", 1);
        jstat = jrun_puts(jx, pbuf);
    }

    Free(pbuf);

    return (jstat);
}
/***************************************************************/
static int jrun_new_internal_class_method(
    struct jrunexec * jx,
    struct jvarvalue * jvv,
    const char * method_name,
    JVAR_INTERNAL_FUNCTION ifuncptr )

{
/*
**
*/
    int jstat = 0;
    struct jvarvalue * fjvv;

    if (jvv->jvv_dtype != JVV_DTYPE_INTERNAL_CLASS) {
        jstat = jrun_set_error(jx, JSERR_EXP_INTERNAL_CLASS,
            "Expecting internal class for method");
        return (jstat);
    }

    fjvv = jvar_new_jvarvalue();
    fjvv->jvv_dtype = JVV_DTYPE_INTERNAL_METHOD;
    fjvv->jvv_val.jvv_int_method = ifuncptr;

    jstat = jvar_add_class_method(jx, jvv, method_name, fjvv);

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
        jstat = jrun_set_error(jx, JSERR_EXP_INTERNAL_CLASS,
            "Expecting internal class to add");
        return (jstat);
    }

    vnam = jvv->jvv_val.jvv_jvvi->jvvi_class_name;
    jstat = jvar_add_jvarvalue(jx, vnam, jvv);

    return (jstat);
}
/***************************************************************/
int jrun_add_internal_object(struct jrunexec * jx,
    const char * varname,
    struct jvarvalue * cjvv)
{
    int jstat = 0;
    struct jvarvalue * obj_jvv;

    if (cjvv->jvv_dtype != JVV_DTYPE_INTERNAL_CLASS) {
        jstat = jrun_set_error(jx, JSERR_EXP_INTERNAL_CLASS,
            "Expecting internal class to add");
        return (jstat);
    }
    cjvv->jvv_val.jvv_jvvi->jvvi_nrefs += 1;

    obj_jvv = jvar_new_jvarvalue();
    obj_jvv->jvv_dtype = JVV_DTYPE_OBJECT;
    obj_jvv->jvv_val.jvv_jvvo.jvvo_class = cjvv;
    obj_jvv->jvv_val.jvv_jvvo.jvvo_vars  = jvar_new_jvarrec();

    jstat = jvar_add_jvarvalue(jx, varname, obj_jvv);

    return (jstat);
}
/***************************************************************/
int jrun_load_Console(struct jrunexec * jx)
{
/*
**
*/
    int jstat = 0;
    struct jvarvalue * jvv;

    jvv = jrun_new_internal_class(jx, JVV_INTERNAL_CLASS_CONSOLE);
    if (jvv) {
        /* svare_new_int_class_method(jvv, ".new"              , svare_method_NN_new); */
        jstat = jrun_new_internal_class_method(jx, jvv, "log"           , jint_Console_log);

        if (!jstat) jstat = jrun_add_internal_class(jx, jvv);

        if (!jstat) jstat = jrun_add_internal_object(jx, JVV_INTERNAL_VAR_CONSOLE, jvv);
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
    }

    return (jstat);
}
/***************************************************************/
