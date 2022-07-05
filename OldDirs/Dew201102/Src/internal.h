#ifndef INTERNALH_INCLUDED
#define INTERNALH_INCLUDED
/***************************************************************/
/*  internal.h --  Internal classes                            */
/***************************************************************/

#define JVV_INTERNAL_CLASS_CONSOLE      "Console"
#define JVV_INTERNAL_VAR_CONSOLE        "console"

/***************************************************************/


/***************************************************************/

int jenv_load_internal_classes(struct jsenv * jse);
int jrun_int_tostring(
    struct jrunexec * jx,
    struct jvarvalue * jvv,
    char ** outstr);

/***************************************************************/
#endif /* INTERNALH_INCLUDED */
