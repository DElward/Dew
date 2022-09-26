#ifndef INTERNALH_INCLUDED
#define INTERNALH_INCLUDED
/***************************************************************/
/*  internal.h --  Internal classes                            */
/***************************************************************/

#define JVV_INTERNAL_CLASS_Console          "@Console"
#define JVV_INTERNAL_VAR_console            "console"

#define JVV_INTERNAL_METHOD_toString        "toString"
#define JVV_INTERNAL_METHOD__new            ".new"

/* Error classes */
#define JVV_INTERNAL_CLASS_Error            "Error"
#define JVV_INTERNAL_CLASS_AggregateError   "AggregateError"
#define JVV_INTERNAL_CLASS_EvalError        "EvalError"
#define JVV_INTERNAL_CLASS_InternalError    "InternalError"
#define JVV_INTERNAL_CLASS_RangeError       "RangeError"
#define JVV_INTERNAL_CLASS_ReferenceError   "ReferenceError"
#define JVV_INTERNAL_CLASS_SyntaxError      "SyntaxError"
#define JVV_INTERNAL_CLASS_TypeError        "TypeError"
#define JVV_INTERNAL_CLASS_URIError         "URIError"
#define JVV_INTERNAL_CLASS_UnimplementedError "UnimplementedError"

/* Type classes */
#define JVV_INTERNAL_TYPE_CLASS_JSInt       "@Integer"
#define JVV_INTERNAL_TYPE_CLASS_Boolean     "Boolean"
#define JVV_INTERNAL_TYPE_CLASS_Number      "Number"
#define JVV_INTERNAL_TYPE_CLASS_String      "String"
#define JVV_INTERNAL_TYPE_CLASS_Array       "Array"
#define JVV_INTERNAL_TYPE_CLASS_Object      "Object"
#define JVV_INTERNAL_TYPE_CLASS_Dynamic     "@Dynamic"

#define JVV_BRACKET_OPERATION               "[]"

struct jintobj_console { /* jioc_ */
#if IS_DEBUG
    char jioc_sn[4];    /* 'c' */
#endif
    FILE * jioc_outf;
    FILE * jioc_errf;
};

struct jintobj_util { /* jiou_ */
#if IS_DEBUG
    char jiou_sn[4];    /* 'u' */
#endif
    int jiou__filler;
};

struct jintobj_Error { /* jioe_ */
#if IS_DEBUG
    char jioe_sn[4];    /* 'E' */
#endif
    enum errTypes           jioe_errtyp;
    int                     jioe_msglen;
    char                  * jioe_msg;
};

struct jintobj_AggregateError { /* jioe_ae */
    struct jintobj_Error        jioe_ae_err;
};

struct jintobj_InternalError { /* jioe_ie */
    struct jintobj_Error        jioe_ie_err;
};

struct jintobj_NativeError { /* jioe_ne */
    struct jintobj_Error        jioe_ne_err;
};

struct jintobj_RangeError { /* jioe_ge */
    struct jintobj_Error        jioe_ge_err;
};

struct jintobj_ReferenceError { /* jioe_re */
    struct jintobj_Error        jioe_re_err;
};

struct jintobj_SyntaxError { /* jioe_se */
    struct jintobj_Error        jioe_se_err;
};

struct jintobj_TypeError { /* jioe_te */
    struct jintobj_Error        jioe_te_err;
};

struct jintobj_URIError { /* jioe_ue */
    struct jintobj_Error        jioe_ue_err;
};

struct jintobj_UnimplementedError { /* jioe_xe */
    struct jintobj_Error        jioe_xe_err;
};

/***************************************************************/


/***************************************************************/

int jenv_load_internal_classes(struct jsenv * jse);
struct jvarvalue_int_object * jvar_new_jvarvalue_int_object_classname(
struct jrunexec * jx,
    const char * classname,
    void * this_ptr);
int jexp_internal_method_call(
                    struct jrunexec * jx,
                    struct jvarvalue_int_method * imeth,
                    void             * this_ptr,
                    struct jvarvalue * jvvargs,
                    struct jvarvalue * rtnjvv);
void jint_free_modules(struct jrunexec * jx); /* 02/07/2022 */

/***************************************************************/
#endif /* INTERNALH_INCLUDED */
