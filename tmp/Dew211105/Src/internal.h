#ifndef INTERNALH_INCLUDED
#define INTERNALH_INCLUDED
/***************************************************************/
/*  internal.h --  Internal classes                            */
/***************************************************************/

#define JVV_INTERNAL_CLASS_CONSOLE      "Console"
#define JVV_INTERNAL_VAR_CONSOLE        "console"

/* Type classes */
#define JVV_INTERNAL_TYPE_CLASS_STRING  "String"
#define JVV_INTERNAL_TYPE_CLASS_ARRAY   "Array"

#define JVV_BRACKET_OPERATION           "[]"

struct jintobj_console { /* jioc_ */
#if IS_DEBUG
    char jioc_sn[4];    /* 'c' */
#endif
    FILE * jioc_outf;
};

/***************************************************************/


/***************************************************************/

int jenv_load_internal_classes(struct jsenv * jse);
int jrun_int_tostring(
    struct jrunexec * jx,
    struct jvarvalue * jvv,
    char ** outstr);

/***************************************************************/
#endif /* INTERNALH_INCLUDED */
