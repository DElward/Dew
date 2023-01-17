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
#include "obj.h"

struct jint_module_rec {     /* jmr_ */
    char  * jmr_class_name;
    int   (*jmr_load_module)(struct jrunexec * jx, struct jvarvalue * jvvrtn);
    int     jmr_flags;
};

/***************************************************************/
#if IS_DEBUG
void jvar_inc_internal_class_refs(struct jvarvalue_internal_class * jvvi, int val)
{
/*
** 09/01/2022
*/
    jvvi->jvvi_nRefs += val;
    if (jvvi->jvvi_nRefs < 0) {
        printf("******** jvvi->jvvi_nRefs < 0 ********\n");
    } else if (jvvi->jvvi_nRefs > 10000) {
        printf("******** jvvi->jvvi_nRefs > 10000 = %d ********\n", jvvi->jvvi_nRefs);
    }
}
#endif
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
#if IS_DEBUG
    static int next_jvvi_sn = 0;
#endif

    jvar_init_jvarvalue(jvv);
    jvv->jvv_dtype = JVV_DTYPE_INTERNAL_CLASS;
    jvv->jvv_val.jvv_jvvi = New(struct jvarvalue_internal_class, 1);
#if IS_DEBUG
    jvv->jvv_val.jvv_jvvi->jvvi_sn[0] = 'C';
    jvv->jvv_val.jvv_jvvi->jvvi_sn[1] = (next_jvvi_sn / 100) % 10 + '0';
    jvv->jvv_val.jvv_jvvi->jvvi_sn[2] = (next_jvvi_sn /  10) % 10 + '0';
    jvv->jvv_val.jvv_jvvi->jvvi_sn[3] = (next_jvvi_sn      ) % 10 + '0';
    next_jvvi_sn++;
#endif
    jvv->jvv_val.jvv_jvvi->jvvi_prototype = jint_new_jprototype(jx);
#if ALLOW_INT_CLASS_VARS
    jvv->jvv_val.jvv_jvvi->jvvi_jcx = NULL; /* 01/17/2023 */
#endif
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
int jrun_int_format(
    struct jrunexec * jx,
    char ** prbuf,
    int * prlen,
    int * prmax,
    struct jvarvalue_vallist * jvvl,
    int * index,
    int jfmtflags)
{
/*
**  02/09/2022
**
**  From: https://nodejs.org/docs/latest-v13.x/api/util.html
**  Specifiers are:
**      %s: String will be used to convert all values except BigInt, Object and -0.
**          BigInt values will be represented with an n and Objects that have no user
**          defined toString function are inspected using util.inspect() with
**          options { depth: 0, colors: false, compact: 3 }.
**      %d: Number will be used to convert all values except BigInt and Symbol.
**      %i: parseInt(value, 10) is used for all values except BigInt and Symbol.
**      %f: parseFloat(value) is used for all values expect Symbol.
**      %j: JSON. Replaced with the string '[Circular]' if the argument contains
**          circular references.
**      %o: Object. A string representation of an object with generic JavaScript
**          object formatting. Similar to util.inspect() with
**          options { showHidden: true, showProxy: true }. This will show the full
**          object including non-enumerable properties and proxies.
**      %O: Object. A string representation of an object with generic JavaScript
**          object formatting. Similar to util.inspect() without options. This will
**          show the full object not including non-enumerable properties and proxies.
**      %%: single percent sign ('%'). This does not consume an argument.
*/
    int jstat = 0;
    int done;
    int ix;
    char * pctchr;
    int fmtlen;
    int fmtflag;
    int fmtix;
    int njfmtflags;
    int remlen;
    char * rembuf;
    int tlen;
    int tmax;
    int skip;
    char * tbuf;
    char tmpbuf[128];

    tmax = 0;
    tbuf = NULL;

    done = 0;
    ix = 0;
    while (!jstat && !done) {
        pctchr = strchr((*prbuf) + ix, '%');
        if (!pctchr) {
            done = 1;
        } else {
            skip = 0;
            ix = (int)(pctchr - (*prbuf));   /* pointer subtraction */
            fmtix = ix + 1;
            fmtlen = fmtix - ix + 1;
            switch ((*prbuf)[fmtix]) {
                case 'd': fmtflag = JFMT_FMT_d;    break;
                case 'f': fmtflag = JFMT_FMT_f;    break;
                case 'i': fmtflag = JFMT_FMT_i;    break;
                case 'j': fmtflag = JFMT_FMT_j;    break;
                case 'o': fmtflag = JFMT_FMT_o;    break;
                case 'O': fmtflag = JFMT_FMT_O;    break;
                case 's': fmtflag = JFMT_FMT_s;    break;
                case '%': skip = 1;                   break;
                default:
                    skip = 1;
                    fmtlen--;
                    break;
            }
            remlen = (*prlen) - ix - fmtlen;
            if (remlen < sizeof(tmpbuf)) {
                memcpy(tmpbuf, (*prbuf) + ix + fmtlen, remlen + 1);
                rembuf = tmpbuf;
            } else {
                tlen = 0;
                append_chars(&tbuf, &tlen, &tmax, (*prbuf) + ix + fmtlen, remlen);
                rembuf = tbuf;
            }
            if (skip) {
                (*prlen) = ix + 1;
            } else {
                (*prlen) = ix;
                njfmtflags = (jfmtflags & JFMT_FLAGS_NOT_FMT_MASK) | fmtflag;
                jstat = jpr_jvarvalue_tostring(jx, prbuf, prlen, prmax, &(jvvl->jvvl_avals[*index]), njfmtflags);
                (*index)++;
            }
            if (!jstat) {
                ix = (*prlen);
                append_chars(prbuf, prlen, prmax, rembuf, remlen);
            }
        }
    }
    if (tbuf) Free(tbuf);

    return (jstat);
}
/***************************************************************/
static int jrun_int_tostring(
    struct jrunexec * jx,
    struct jvarvalue_vallist * jvvl,
    int * index,
    char ** outstr,
    int jfmtflags)
{
/*
**  Reworked: 02/08/2022
*/
    int jstat = 0;
    int prlen = 0;
    int prmax = 0;
    int njfmtflags;

    (*outstr) = NULL;
    if (*index < jvvl->jvvl_nvals) {
        jstat = jpr_jvarvalue_tostring(jx, outstr, &prlen, &prmax, &(jvvl->jvvl_avals[*index]), jfmtflags);
        if (!jstat && !(*index)) {
            (*index)++;
            jstat = jrun_int_format(jx, outstr, &prlen, &prmax, jvvl, index, jfmtflags);
        }
    }

    while (!jstat && (*index < jvvl->jvvl_nvals)) {
        append_charval(outstr, &prlen, &prmax, " "); /* add a space for a comma */
        njfmtflags = (jfmtflags & JFMT_FLAGS_NOT_FMT_MASK);
        jstat = jpr_jvarvalue_tostring(jx, outstr, &prlen, &prmax, &(jvvl->jvvl_avals[*index]), njfmtflags);
        (*index)++;
    }

    return (jstat);
}
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
    int jfmtflags;

    pbuf    = NULL;
    pbuflen = 0;
    pbufmax = 0;
    jfmtflags = JFMT_ORIGIN_LOG;

    for (ii = 0; !jstat && ii < jvvlargs->jvv_val.jvv_jvvl->jvvl_nvals; ii++) {
        if (ii) append_chars(&pbuf, &pbuflen, &pbufmax, " ", 1);
        jstat = jrun_int_tostring(jx, jvvlargs->jvv_val.jvv_jvvl, &ii, &logstr, jfmtflags);
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
        void             * this_ptr,
        struct jvarvalue * jvvlargs,
        struct jvarvalue * jvvrtn)
{
/*
** 11/22/2021
*/
    int jstat = 0;
    struct jintobj_console * jioc = (struct jintobj_console *)this_ptr;

    INIT_JVARVALUE(jvvrtn);

    if (!this_ptr) {
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
        void             * this_ptr,
        struct jvarvalue * jvvlargs,
        struct jvarvalue * jvvrtn)
{
/*
** 11/22/2021
*/
    int jstat = 0;
    struct jintobj_console * jioc = (struct jintobj_console *)this_ptr;

    INIT_JVARVALUE(jvvrtn);

    if (!this_ptr) {
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
    int jvvflags )

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
    fjvv.jvv_flags = jvvflags;
    jvvim = jvar_new_int_method(method_name, ifuncptr, jvv);
    fjvv.jvv_val.jvv_int_method = jvvim;
    INCIMETHREFS(jvvim);

    jstat = jvar_add_class_method(jx, jvv, method_name, &fjvv);

    return (jstat);
}
/***************************************************************/
static int jrun_new_internal_prototype_method(
    struct jrunexec * jx,
    struct jvarvalue * jvv,
    const char * method_name,
    JVAR_INTERNAL_FUNCTION ifuncptr,
    int jvvflags )

{
/*
** 10/03/2022
*/
    int jstat = 0;

    jstat = jrun_new_internal_class_method(jx, jvv, method_name, ifuncptr, jvvflags);

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
    jstat = jvar_insert_new_global_variable(jx, vnam, jvv);

    return (jstat);
}
/***************************************************************/
void jvar_init_jvarvalue_int_object(
    struct jrunexec * jx,
    struct jvarvalue * cjvv,
    void             * this_ptr,
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
    void             * this_ptr)
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
    INCIOBJREFS(jvvo);

    return (jvvo);
}
/***************************************************************/
int jrun_add_internal_object(struct jrunexec * jx,
    const char * varname,
    struct jvarvalue * cjvv,
    void             * this_ptr)
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

    jstat = jvar_insert_new_global_variable(jx, varname, &obj_jvv);

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
struct jintobj_util * jint_new_jintobj_util(void)
{
    struct jintobj_util * jiou;
#if IS_DEBUG
    static int next_jiou_sn = 0;
#endif

    jiou = New(struct jintobj_util, 1);
#if IS_DEBUG
    jiou->jiou_sn[0] = 'u';
    jiou->jiou_sn[1] = (next_jiou_sn / 100) % 10 + '0';
    jiou->jiou_sn[2] = (next_jiou_sn /  10) % 10 + '0';
    jiou->jiou_sn[3] = (next_jiou_sn      ) % 10 + '0';
    next_jiou_sn++;
#endif

    jiou->jiou__filler = 6789;

    return (jiou);
}
/***************************************************************/
void jint_free_jintobj_util(void * vjiou)
{
    struct jintobj_util * jiou = (struct jintobj_util *)vjiou;

    Free(jiou);
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
        void             * this_ptr,
        struct jvarvalue * jvvlargs,
        struct jvarvalue * jvvrtn)
{
/*
**
*/
    int jstat = 0;
    struct jvarvalue_chars * jvvc = (struct jvarvalue_chars *)this_ptr;

    if (!this_ptr) {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_NULL_OBJECT,
            "Null object passed to %s()", func_name);
        return (jstat);
    }

    jvvrtn->jvv_dtype = JVV_DTYPE_JSINT;
    jvvrtn->jvv_val.jvv_val_jsint = jvvc->jvvc_length;

    return (jstat);
}
/***************************************************************/
static void jint_concat_chars(
    struct jvarvalue * jvvtgt,
    struct jvarvalue_chars * jvv_chars1)
{
/*
** 10/03/2022
**
** See also: jvar_cat_chars()
*/
    struct jvarvalue_chars * jvv_chars_tgt;
    int tlen;

    if (jvvtgt->jvv_dtype == JVV_DTYPE_CHARS) {
        jvv_chars_tgt = jvvtgt->jvv_val.jvv_val_chars;
        tlen = jvv_chars_tgt->jvvc_length + jvv_chars1->jvvc_length;
        if (tlen + 1 >= jvv_chars_tgt->jvvc_size) {
            jvv_chars_tgt->jvvc_size = tlen + JVARVALUE_CHARS_MIN;
            jvv_chars_tgt->jvvc_val_chars =
                Realloc(jvv_chars_tgt->jvvc_val_chars, char, jvv_chars_tgt->jvvc_size);
        }

        memcpy(jvv_chars_tgt->jvvc_val_chars + jvv_chars_tgt->jvvc_length,
               jvv_chars1->jvvc_val_chars, jvv_chars1->jvvc_length);
        jvv_chars_tgt->jvvc_length = tlen;
        jvv_chars_tgt->jvvc_val_chars[tlen] = '\0';
    }
}
/***************************************************************/
static int jint_String_concat(
        struct jrunexec  * jx,
        const char       * func_name,
        void             * this_ptr,
        struct jvarvalue * jvvlargs,
        struct jvarvalue * jvvrtn)
{
/*
** 10/03/2022
*/
    int jstat = 0;
    int ii;
    struct jvarvalue_chars * jvvc = (struct jvarvalue_chars *)this_ptr;
    struct jvarvalue jvvchars;

    if (!this_ptr) {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_NULL_OBJECT,
            "Null object passed to %s()", func_name);
        return (jstat);
    }
    INIT_JVARVALUE(&(jvvchars));

    jvar_store_chars_len(jvvrtn, jvvc->jvvc_val_chars, jvvc->jvvc_length);

    for (ii = 0; !jstat && (ii < jvvlargs->jvv_val.jvv_jvvl->jvvl_nvals); ii++) {
        jstat = jrun_ensure_chars(jx, &(jvvlargs->jvv_val.jvv_jvvl->jvvl_avals[ii]),
                                    &jvvchars, ENSCHARS_FLAG_ERR);
        if (!jstat && (jvvchars.jvv_dtype == JVV_DTYPE_CHARS)) {
            jint_concat_chars(jvvrtn, jvvchars.jvv_val.jvv_val_chars);
        }
        jvar_free_jvarvalue_data(&jvvchars);
    }

    return (jstat);
}
/***************************************************************/
static int jint_String_substring(
        struct jrunexec  * jx,
        const char       * func_name,
        void             * this_ptr,
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
    struct jvarvalue_chars * jvvc = (struct jvarvalue_chars *)this_ptr;

    if (!this_ptr) {
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
int jint_String_toString(
        struct jrunexec  * jx,
        const char       * func_name,
        void             * this_ptr,
        struct jvarvalue * jvvlargs,
        struct jvarvalue * jvvrtn)
{
/*
** 02/15/2022
*/
    int jstat = 0;
    struct jvarvalue_chars * jvvc = (struct jvarvalue_chars *)this_ptr;

    jvar_store_chars_len(jvvrtn, jvvc->jvvc_val_chars, jvvc->jvvc_length);

    return (jstat);
}
/***************************************************************/
static int jint_String__new(
        struct jrunexec  * jx,
        const char       * func_name,
        void             * this_ptr,
        struct jvarvalue * jvvlargs,
        struct jvarvalue * jvvrtn)
{
/*
** 09/06/2022
*/
    int jstat = 0;
    int jfmtflags;
    char * prbuf = NULL;
    int prlen = 0;
    int prmax = 0;

    prbuf = NULL;
    prlen = 0;
    prmax = 0;
    jfmtflags = JFMT_ORIGIN_TOCHARS;

    if (jvvlargs->jvv_val.jvv_jvvl->jvvl_nvals > 0) {
        jstat = jpr_jvarvalue_tostring(jx, &prbuf, &prlen, &prmax,
            &(jvvlargs->jvv_val.jvv_jvvl->jvvl_avals[0]), jfmtflags);
        if (!jstat) {
            jvar_store_chars_len(jvvrtn, prbuf, prlen);
            Free(prbuf);
        }
    } else {
        jvar_store_chars_len(jvvrtn, "", 0);
    }

    return (jstat);
}
/***************************************************************/
int jrun_load_type_String(struct jrunexec * jx)
{
/*
** 09/28/2021
**  
**  From:
**  https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/String
**
**  Static methods
**  String.fromCharCode()               Returns a string created by using the specified sequence of Unicode values.
**  String.fromCodePoint()              Returns a string created by using the specified sequence of code points.
**  String.raw()                        Returns a string created from a raw template string.
**
**  Instance properties
**  String.prototype.length             Reflects the length of the string. Read-only.
**
**  Instance methods
**  String.prototype.at()               Returns the character at the specified index. Negative integers count back from end.
**  String.prototype.charAt()           Returns the character at the specified index.
**  String.prototype.charCodeAt()       Returns a number that is the UTF-16 code value at the given index.
**  String.prototype.codePointAt()      Returns a nonnegative integer of the UTF-16 encoded code point starting at the specified pos.
**  String.prototype.concat()           Combines the text of two (or more) strings and returns a new string.
**  String.prototype.includes()         Determines whether the calling string contains searchString.
**  String.prototype.endsWith()         Determines whether a string ends with the characters of the string searchString.
**  String.prototype.indexOf()          Index within the calling String object of first occurrence of searchValue, or -1 if not found.
**  String.prototype.lastIndexOf()      Index within the calling String object of last occurrence of searchValue, or -1 if not found.
**  String.prototype.localeCompare()    Number indicating if the compareString comes before, after, or is equivalent in sort order.
**  String.prototype.match()            Used to match regular expression against a string.
**  String.prototype.matchAll()         Returns an iterator of all regexp's matches.
**  String.prototype.normalize()        Returns the Unicode Normalization Form of the calling string value.
**  String.prototype.padEnd()           Pads the current string from end with a given string and returns a new string of targetLength.
**  String.prototype.padStart()         Pads the current string from start with a given string and returns a new string of targetLength.
**  String.prototype.repeat()           Returns a string consisting of the elements of the object repeated count times.
**  String.prototype.replace()          Replaces occurrences of searchFor with replaceWith. searchFor may be string, regexp, and replaceWith may be a string or function.
**  String.prototype.replaceAll()       Replaces all occurrences of searchFor with replaceWith. searchFor may be a string, regexp, and replaceWith may be a string or function.
**  String.prototype.search()           Search for a match between a regular expression regexp and the calling string.
**  String.prototype.slice()            Extracts a section of a string and returns a new string.
**  String.prototype.split()            Returns an array of strings populated by splitting the calling string at occurrences of the substring sep.
**  String.prototype.startsWith()       Determines whether the calling string begins with the characters of string searchString.
**  String.prototype.substring()        Returns a new string containing characters of the calling string from (or between) the specified index (or indices).
**  String.prototype.toLocaleLowerCase()                        The characters within a string are converted to lowercase in the current locale.
**  String.prototype.toLocaleUpperCase( [locale, ...locales])   The characters within a string are converted to uppercase in the current locale.
**  String.prototype.toLowerCase()      Returns the calling string value converted to lowercase.
**  String.prototype.toString()         Returns a string representing the specified object. Overrides the Object.prototype.toString() method.
**  String.prototype.toUpperCase()      Returns the calling string value converted to uppercase.
**  String.prototype.trim()             Trims whitespace from the beginning and end of the string.
**  String.prototype.trimStart()        Trims whitespace from the beginning of the string.
**  String.prototype.trimEnd()          Trims whitespace from the end of the string.
**  String.prototype.valueOf()          Returns the primitive value of string. Overrides the Object.prototype.valueOf() method.
**  String.prototype[@@iterator]()      Returns a new iterator that iterates over the code points of a String value.
**  
*/
    int jstat = 0;
    struct jvarvalue jvv;

    jrun_init_internal_class(jx, JVV_INTERNAL_TYPE_CLASS_String, NULL, NULL, &jvv);

    jstat = jrun_new_internal_class_method(jx, &jvv, "length"     , jint_String_length     , JCX_FLAG_PROPERTY);
    if (!jstat) jstat = jrun_new_internal_class_method(jx, &jvv, "concat"     , jint_String_concat     , JCX_FLAG_METHOD);
    if (!jstat) jstat = jrun_new_internal_class_method(jx, &jvv, "substring"  , jint_String_substring  , JCX_FLAG_METHOD);
    if (!jstat) jstat = jrun_new_internal_class_method(jx, &jvv, JVV_INTERNAL_METHOD_toString, jint_String_toString, JCX_FLAG_METHOD);
    if (!jstat) jstat = jrun_new_internal_class_method(jx, &jvv, JVV_INTERNAL_METHOD__new, jint_String__new, JCX_FLAG_METHOD);

    if (!jstat) jstat = jrun_add_internal_class(jx, &jvv);

    if (!jstat) jstat = jrun_add_internal_type_object(jx, JVV_DTYPE_CHARS, &jvv);

    return (jstat);
}
/***************************************************************/
int jint_JSint_toString(
        struct jrunexec  * jx,
        const char       * func_name,
        void             * this_ptr,
        struct jvarvalue * jvvlargs,
        struct jvarvalue * jvvrtn)
{
/*
** 02/15/2022
*/
    int jstat = 0;
    JSINT * jsi = (JSINT *)this_ptr;    /* See jexp_binop_dot() */
    char numbuf[16];

    sprintf(numbuf, "%d", (*jsi));
    jvar_store_chars_len(jvvrtn, numbuf, IStrlen(numbuf));

    return (jstat);
}
/***************************************************************/
int jrun_load_type_JSInt(struct jrunexec * jx)
{
/*
** 09/28/2021
*/
    int jstat = 0;
    struct jvarvalue jvv;

    jrun_init_internal_class(jx, JVV_INTERNAL_TYPE_CLASS_JSInt, NULL, NULL, &jvv);

    jstat = jrun_new_internal_class_method(jx, &jvv, JVV_INTERNAL_METHOD_toString, jint_JSint_toString, JCX_FLAG_METHOD);

    if (!jstat) jstat = jrun_add_internal_class(jx, &jvv);

    if (!jstat) jstat = jrun_add_internal_type_object(jx, JVV_DTYPE_JSINT, &jvv);

    return (jstat);
}
/***************************************************************/
int jint_Boolean__new(
        struct jrunexec  * jx,
        const char       * func_name,
        void             * this_ptr,
        struct jvarvalue * jvvlargs,
        struct jvarvalue * jvvrtn)
{
/*
** 09/07/2022
*/
    int jstat = 0;
    int boolval;

    boolval = 0;

    if (jvvlargs->jvv_val.jvv_jvvl->jvvl_nvals > 0) {
        jstat = jrun_ensure_boolean(jx, &(jvvlargs->jvv_val.jvv_jvvl->jvvl_avals[0]), &boolval, 0);
    }

    if (!jstat) {
        jvvrtn->jvv_dtype = JVV_DTYPE_BOOL;
        jvvrtn->jvv_val.jvv_val_bool = boolval;
    }

    return (jstat);
}
/***************************************************************/
int jrun_load_type_Boolean(struct jrunexec * jx)
{
/*
** 09/07/2022
*/
    int jstat = 0;
    struct jvarvalue jvv;

    jrun_init_internal_class(jx, JVV_INTERNAL_TYPE_CLASS_Boolean, NULL, NULL, &jvv);

    if (!jstat) jstat = jrun_new_internal_class_method(jx, &jvv, JVV_INTERNAL_METHOD__new, jint_Boolean__new, JCX_FLAG_METHOD);

    if (!jstat) jstat = jrun_add_internal_class(jx, &jvv);

    if (!jstat) jstat = jrun_add_internal_type_object(jx, JVV_DTYPE_BOOL, &jvv);

    return (jstat);
}
/***************************************************************/
int jint_Number_toString(
        struct jrunexec  * jx,
        const char       * func_name,
        void             * this_ptr,
        struct jvarvalue * jvvlargs,
        struct jvarvalue * jvvrtn)
{
/*
** 08/25/2022
*/
    int jstat = 0;
    JSFLOAT * jsf = (JSFLOAT *)this_ptr;    /* See jexp_binop_dot() */
    char numbuf[32];

    sprintf(numbuf, "%g", (*jsf));  /* See jpr_jvarvalue_tostring() */
    jvar_store_chars_len(jvvrtn, numbuf, IStrlen(numbuf));

    return (jstat);
}
/***************************************************************/
int jint_Number__new(
        struct jrunexec  * jx,
        const char       * func_name,
        void             * this_ptr,
        struct jvarvalue * jvvlargs,
        struct jvarvalue * jvvrtn)
{
/*
** 08/25/2022
struct jvarvalue_pointer {
    struct jvarvalue        jvvr_jvv;
    int                     jvvr_nRefs;
};

*/
    int jstat = 0;
    JSFLOAT fltval;

    fltval = 0.0;

    if (jvvlargs->jvv_val.jvv_jvvl->jvvl_nvals > 0) {
        jstat = jrun_ensure_number(jx, &(jvvlargs->jvv_val.jvv_jvvl->jvvl_avals[0]), &fltval, 0);
    }

    if (!jstat) {
        jvvrtn->jvv_dtype = JVV_DTYPE_JSFLOAT;
        jvvrtn->jvv_val.jvv_val_jsfloat = fltval;
    }

    return (jstat);
}
/***************************************************************/
int jrun_load_type_Number(struct jrunexec * jx)
{
/*
** 08/25/2022
*/
    int jstat = 0;
    struct jvarvalue jvv;

    jrun_init_internal_class(jx, JVV_INTERNAL_TYPE_CLASS_Number, NULL, NULL, &jvv);

    jstat = jrun_new_internal_class_method(jx, &jvv, JVV_INTERNAL_METHOD_toString, jint_Number_toString, JCX_FLAG_METHOD);
    if (!jstat) jstat = jrun_new_internal_class_method(jx, &jvv, JVV_INTERNAL_METHOD__new, jint_Number__new, JCX_FLAG_METHOD);

    if (!jstat) jstat = jrun_add_internal_class(jx, &jvv);

    if (!jstat) jstat = jrun_add_internal_type_object(jx, JVV_DTYPE_JSFLOAT, &jvv);

    return (jstat);
}
/***************************************************************/
int jrun_load_type_Array(struct jrunexec * jx)
{
/*
** 10/21/2021
**  
**  From:
**  https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Array
**
**  Static methods
**  String.fromCharCode()               Returns a string created by using the specified sequence of Unicode values.
**  String.fromCodePoint()              Returns a string created by using the specified sequence of code points.
**  String.raw()                        Returns a string created from a raw template string.
**
**  Instance properties
**  String.prototype.length             Reflects the length of the string. Read-only.
**
**  Constructor
**  Array()                             Creates a new Array object.
**
**  Static properties
**  get Array[@@species]                Returns the Array constructor.
**  
**  Static methods
**  Array.from()                        Creates a new Array instance from an array-like object or iterable object.
**  Array.isArray()                     Returns true if the argument is an array, or false otherwise.
**  Array.of()                          Creates a new Array instance with a variable number of arguments, regardless of number or type of the arguments.
**  
**  Instance properties
**  Array.prototype.length              Reflects the number of elements in an array.
**  Array.prototype[@@unscopables]      Contains property names that were not included in the ECMAScript standard prior to the ES2015 version and that are ignored for with statement-binding purposes.
**  
**  Instance methods
**  Array.prototype.at()                Returns the array item at the given index. Accepts negative integers, which count back from the last item.
**  Array.prototype.concat()            Returns a new array that is the calling array joined with other array(s) and/or value(s).
**  Array.prototype.copyWithin()        Copies a sequence of array elements within an array.
**  Array.prototype.entries()           Returns a new array iterator object that contains the key/value pairs for each index in an array.
**  Array.prototype.every()             Returns true if every element in the calling array satisfies the testing function.
**  Array.prototype.fill()              Fills all the elements of an array from a start index to an end index with a static value.
**  Array.prototype.filter()            Returns a new array containing all elements of the calling array for which the provided filtering function returns true.
**  Array.prototype.find()              Returns the value of the first element in the array that satisfies the provided testing function, or undefined if no appropriate element is found.
**  Array.prototype.findIndex()         Returns the index of the first element in the array that satisfies the provided testing function, or -1 if no appropriate element was found.
**  Array.prototype.findLast()          Returns the value of the last element in the array that satisfies the provided testing function, or undefined if no appropriate element is found.
**  Array.prototype.findLastIndex()     Returns the index of the last element in the array that satisfies the provided testing function, or -1 if no appropriate element was found.
**  Array.prototype.flat()              Returns a new array with all sub-array elements concatenated into it recursively up to the specified depth.
**  Array.prototype.flatMap()           Returns a new array formed by applying a given callback function to each element of the calling array, and then flattening the result by one level.
**  Array.prototype.forEach()           Calls a function for each element in the calling array.
**  Array.prototype.group() **          Groups the elements of an array into an object according to the strings returned by a test function.
**  Array.prototype.groupToMap() **     Groups the elements of an array into a Map according to values returned by a test function.
**  Array.prototype.includes()          Determines whether the calling array contains a value, returning true or false as appropriate.
**  Array.prototype.indexOf()           Returns the first (least) index at which a given element can be found in the calling array.
**  Array.prototype.join()              Joins all elements of an array into a string.
**  Array.prototype.keys()              Returns a new array iterator that contains the keys for each index in the calling array.
**  Array.prototype.lastIndexOf()       Returns the last (greatest) index at which a given element can be found in the calling array, or -1 if none is found.
**  Array.prototype.map()               Returns a new array containing the results of invoking a function on every element in the calling array.
**  Array.prototype.pop()               Removes the last element from an array and returns that element.
**  Array.prototype.push()              Adds one or more elements to the end of an array, and returns the new length of the array.
**  Array.prototype.reduce()            Executes a user-supplied "reducer" callback function on each element of the array (from left to right), to reduce it to a single value.
**  Array.prototype.reduceRight()       Executes a user-supplied "reducer" callback function on each element of the array (from right to left), to reduce it to a single value.
**  Array.prototype.reverse()           Reverses the order of the elements of an array in place. (First becomes the last, last becomes first.)
**  Array.prototype.shift()             Removes the first element from an array and returns that element.
**  Array.prototype.slice()             Extracts a section of the calling array and returns a new array.
**  Array.prototype.some()              Returns true if at least one element in the calling array satisfies the provided testing function.
**  Array.prototype.sort()              Sorts the elements of an array in place and returns the array.
**  Array.prototype.splice()            Adds and/or removes elements from an array.
**  Array.prototype.toLocaleString()    Returns a localized string representing the calling array and its elements. Overrides the Object.prototype.toLocaleString() method.
**  Array.prototype.toString()          Returns a string representing the calling array and its elements. Overrides the Object.prototype.toString() method.
**  Array.prototype.unshift()           Adds one or more elements to the front of an array, and returns the new length of the array.
**  Array.prototype.values()            Returns a new array iterator object that contains the values for each index in the array.
**  Array.prototype[@@iterator]()       An alias for the values() method by default.
**  ** = Experimental
**
*/
    int jstat = 0;
    struct jvarvalue jvv;

    jrun_init_internal_class(jx, JVV_INTERNAL_TYPE_CLASS_Array, NULL, NULL, &jvv);

    jstat = jrun_new_internal_class_method(jx, &jvv, "length"     , jint_Array_length     , JCX_FLAG_PROPERTY);
    if (!jstat) {
        jstat = jrun_new_internal_class_method(jx, &jvv, "push"     , jint_Array_push     , JCX_FLAG_METHOD);
    }
    if (!jstat) {
        jstat = jrun_new_internal_class_method(jx, &jvv, JVV_BRACKET_OPERATION  , jint_Array_subscript  , JCX_FLAG_OPERATION);
    }
    if (!jstat) {
        jstat = jrun_new_internal_class_method(jx, &jvv, JVV_INTERNAL_METHOD_toString, jint_Array_toString, JCX_FLAG_METHOD);
    }

    if (!jstat) jstat = jrun_add_internal_class(jx, &jvv);

    if (!jstat) jstat = jrun_add_internal_type_object(jx, JVV_DTYPE_ARRAY, &jvv);

    return (jstat);
}
/***************************************************************/
int jint_Object__new(
        struct jrunexec  * jx,
        const char       * func_name,
        void             * this_ptr,
        struct jvarvalue * jvvlargs,
        struct jvarvalue * jvvrtn)
{
/*
** 09/22/2022
*/
    int jstat = 0;
    int boolval;

    boolval = 0;

    if (jvvlargs->jvv_val.jvv_jvvl->jvvl_nvals > 0) {
        jstat = jrun_ensure_boolean(jx, &(jvvlargs->jvv_val.jvv_jvvl->jvvl_avals[0]), &boolval, 0);
    }

    if (!jstat) {
        jvvrtn->jvv_dtype = JVV_DTYPE_OBJECT;
        jvvrtn->jvv_val.jvv_val_object = job_new_jvarvalue_object(jx);
    }

    return (jstat);
}
/***************************************************************/
int jrun_load_type_Object(struct jrunexec * jx)
{
/*
** 09/22/2022
*/
    int jstat = 0;
    struct jvarvalue jvv;

    jrun_init_internal_class(jx, JVV_INTERNAL_TYPE_CLASS_Object, NULL, NULL, &jvv);

    if (!jstat) jstat = jrun_new_internal_class_method(jx, &jvv, JVV_INTERNAL_METHOD__new, jint_Object__new, JCX_FLAG_METHOD);

    if (!jstat) jstat = jrun_add_internal_class(jx, &jvv);

    if (!jstat) jstat = jrun_add_internal_type_object(jx, JVV_DTYPE_OBJECT, &jvv);

    return (jstat);
}
/***************************************************************/
int jrun_load_type_methods(struct jrunexec * jx)
{
/*
** 09/28/2021
*/
    int jstat = 0;

                jstat = jrun_load_type_Array(jx);
    if (!jstat) jstat = jrun_load_type_Boolean(jx);
    if (!jstat) jstat = jrun_load_type_JSInt(jx);
    if (!jstat) jstat = jrun_load_type_Number(jx);
    if (!jstat) jstat = jrun_load_type_Object(jx);
    if (!jstat) jstat = jrun_load_type_String(jx);

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
    const char      * classname,
    void            * this_ptr)
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
            cstat = convert_string_to_number(jvv->jvv_val.jvv_val_chars->jvvc_val_chars, &intnum, &dblnum);
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
        void             * this_ptr,
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
static int jint_require(
        struct jrunexec  * jx,
        const char       * req_str,
        struct jvarvalue * jvvrtn)
{
/*
** 02/06/2022
*/
    int jstat = 0;
    void ** vjmr;
    struct jint_module_rec * jmr;

    vjmr = dbtree_find(jx->jx_module_dbt, req_str, IStrlen(req_str) + 1);
    if (vjmr) {
        jmr = *(struct jint_module_rec **)vjmr;
        jstat = (jmr->jmr_load_module)(jx, jvvrtn);
    }

    return (jstat);
}
/***************************************************************/
static int jint_function_require(
        struct jrunexec  * jx,
        const char       * func_name,
        void             * this_ptr,
        struct jvarvalue * jvvlargs,
        struct jvarvalue * jvvrtn)
{
/*
** 02/06/2022
*/
    int jstat = 0;
    struct jvarvalue jvvchars;

    if (jvvlargs->jvv_val.jvv_jvvl->jvvl_nvals == 1) {
        jstat = jrun_ensure_chars(jx, &(jvvlargs->jvv_val.jvv_jvvl->jvvl_avals[0]),
                                    &jvvchars, ENSCHARS_FLAG_ERR);
        if (!jstat) {
            jstat = jint_require(jx, jvvchars.jvv_val.jvv_val_chars->jvvc_val_chars, jvvrtn);
            jvar_free_jvarvalue_data(&jvvchars);
        }
    } else {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_EXP_N_ARGS,
            "Function %s() expects one argument", func_name);
    }

    return (jstat);
}
/***************************************************************/
static int jrun_new_internal_function(
    struct jrunexec * jx,
    const char * method_name,
    JVAR_INTERNAL_FUNCTION ifuncptr,
    int jvvflags )

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

    jstat = jvar_insert_new_global_variable(jx, method_name, &fjvv);

    return (jstat);
}
/***************************************************************/
int jrun_load_internal_functions(struct jrunexec * jx)
{
/*
** 11/16/2021
*/
    int jstat = 0;

    jstat = jrun_new_internal_function(jx, "isNaN"         , jint_function_isNaN    , JCX_FLAG_FUNCTION);

    if (jx->jx_flags & JX_FLAG_NODEJS) {
        jstat = jrun_new_internal_function(jx, "require"       , jint_function_require  , JCX_FLAG_FUNCTION);
    }

    return (jstat);
}
/***************************************************************/
void jint_vfree_module_rec(void * vjmr)
{
/*
** 02/07/2022
*/
    struct jint_module_rec * jmr = (struct jint_module_rec *)vjmr;

    Free(jmr->jmr_class_name);
    Free(jmr);
}
/***************************************************************/
void jint_free_modules(struct jrunexec * jx)
{
/*
** 02/07/2022
*/
    dbtree_free(jx->jx_module_dbt, jint_vfree_module_rec);
}
/***************************************************************/
static int jint_Util_format(
        struct jrunexec  * jx,
        const char       * func_name,
        void             * this_ptr,
        struct jvarvalue * jvvlargs,
        struct jvarvalue * jvvrtn)
{
/*
** 02/07/2022
*/
    int jstat = 0;
    struct jintobj_util * jiou = (struct jintobj_util *)this_ptr;
    int ii;
    char * logstr;
    char * pbuf;
    int    pbuflen;
    int    pbufmax;
    int jfmtflags;

    pbuf    = NULL;
    pbuflen = 0;
    pbufmax = 0;
    jfmtflags = JFMT_ORIGIN_FORMAT;
    INIT_JVARVALUE(jvvrtn);

    for (ii = 0; !jstat && ii < jvvlargs->jvv_val.jvv_jvvl->jvvl_nvals; ii++) {
        if (ii) append_chars(&pbuf, &pbuflen, &pbufmax, " ", 1);
        jstat = jrun_int_tostring(jx, jvvlargs->jvv_val.jvv_jvvl, &ii, &logstr, jfmtflags);
        if (!jstat && logstr) {
            append_chars(&pbuf, &pbuflen, &pbufmax, logstr, IStrlen(logstr));
        }
        Free(logstr);
    }
    if (!jstat) {
        jvar_store_chars_len(jvvrtn, pbuf, pbuflen);
    }

    Free(pbuf);

    return (jstat);
}
/***************************************************************/
int jrun_create_internal_object(struct jrunexec * jx,
    struct jvarvalue * cjvv,
    void             * this_ptr,
    struct jvarvalue * jvvrtn)
{
/*
** 02/07/2022
*/
    int jstat = 0;

    if (cjvv->jvv_dtype != JVV_DTYPE_INTERNAL_CLASS) {
        jstat = jrun_set_error(jx, errtyp_UnimplementedError, JSERR_EXP_INTERNAL_CLASS,
            "Expecting internal class to add");
        return (jstat);
    }

    jvar_init_jvarvalue(jvvrtn);
    jvvrtn->jvv_dtype = JVV_DTYPE_INTERNAL_OBJECT;
    jvvrtn->jvv_val.jvv_jvvo = jvar_new_jvarvalue_int_object(jx, cjvv, this_ptr);

    return (jstat);
}
/***************************************************************/
int jint_load_module_util(struct jrunexec * jx, struct jvarvalue * jvvrtn)
{
/*
** 02/07/2022
**
** See: https://www.w3schools.com/nodejs/ref_util.asp
*/
    int jstat = 0;
    struct jvarvalue jvv;
    struct jintobj_util * jiou;

    jiou = jint_new_jintobj_util();

    jrun_init_internal_class(jx, "util", NULL, jint_free_jintobj_util, &jvv);

    jstat = jrun_new_internal_class_method(jx, &jvv, "format"           , jint_Util_format  , JCX_FLAG_METHOD);

    if (!jstat) {
        jstat = jrun_create_internal_object(jx, &jvv, jiou, jvvrtn);
    }

    return (jstat);
}
/***************************************************************/
/* module list                                                 */
/* See: https://www.w3schools.com/nodejs/ref_modules.asp       */
/***************************************************************/
#define JMR_FLAG_NODEJS_MODULE      1
struct jint_module_rec jint_module_list[] = {
    { "util"        , jint_load_module_util     , JMR_FLAG_NODEJS_MODULE}, 
    {  NULL,  NULL, 0}
};
/***************************************************************/
int jrun_load_loadjs_names(struct jrunexec * jx)
{
/*
** 02/07/2022
*/
    int jstat = 0;
    int ii;
    int done;
    struct jint_module_rec * jmr;
    struct jint_module_rec * njmr;
    int is_dup;

    done = 0;
    ii   = 0;

    while (!jstat && !done) {
        jmr = jint_module_list + ii;
        if (!jmr->jmr_class_name) {
            done = 1;
        } else if (jmr->jmr_flags & JMR_FLAG_NODEJS_MODULE) {
            njmr = New(struct jint_module_rec, 1);
            njmr->jmr_class_name  = Strdup(jmr->jmr_class_name);
            njmr->jmr_load_module = jmr->jmr_load_module;
            njmr->jmr_flags       = jmr->jmr_flags;
            is_dup = dbtree_insert(jx->jx_module_dbt, jmr->jmr_class_name, IStrlen(jmr->jmr_class_name) + 1, njmr);
        }
        ii++;
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
        if (!jstat) jstat = jrun_load_Error(jse->jse_jx);

        if (!jstat) {
            jstat = jrun_load_internal_functions(jse->jse_jx);
        }

        if (!jstat) {
            jstat = jrun_load_type_methods(jse->jse_jx);
        }

        if (!jstat) {
            jstat = jrun_load_loadjs_names(jse->jse_jx);
        }
    }

    return (jstat);
}
/***************************************************************/
int jexp_internal_method_call(
                    struct jrunexec * jx,
                    struct jvarvalue_int_method * imeth,
                    void             * this_ptr,
                    struct jvarvalue * jvvargs,
                    struct jvarvalue * rtnjvv)
{
/*
** 03/08/2021
*/
    int jstat = 0;

    // jstat = jrun_push_jfuncstate(jx, fjvv, NULL);

    jstat = (imeth->jvvim_method)(jx, imeth->jvvim_method_name, this_ptr, jvvargs, rtnjvv);

    // if (!jstat) jstat = jrun_pop_jfuncstate(jx);

    return (jstat);
}
/***************************************************************/
