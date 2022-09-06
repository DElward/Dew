#ifndef PRINTH_INCLUDED
#define PRINTH_INCLUDED
/***************************************************************/
/*  print.h --  Debug/print functions                          */
/***************************************************************/

/* formats - sequential */
#define JFMT_FMT_none                   0x000000
#define JFMT_FMT_d                      0x000001
#define JFMT_FMT_f                      0x000002
#define JFMT_FMT_i                      0x000003
#define JFMT_FMT_j                      0x000004
#define JFMT_FMT_o                      0x000005
#define JFMT_FMT_O                      0x000006
#define JFMT_FMT_s                      0x000007

/* origins - sequential */
#define JFMT_ORIGIN_LOG                 0x000010
#define JFMT_ORIGIN_FORMAT              0x000020
#define JFMT_ORIGIN_TOSTRING            0x000030
#define JFMT_ORIGIN_TOCHARS             0x000040
#define JFMT_ORIGIN_INTERPOLATE         0x000080
#define JFMT_ORIGIN_EVAL                0x000100

/* filters - bits - for jpr_vartype_matches() */
#define JFMT_FILTER_VARS               0x1000000
#define JFMT_FILTER_CLASSES            0x2000000
#define JFMT_FILTER_METHODS            0x4000000

/* flags - bits */
#define JFMT_FLAG_SHOW_TYPE             0x001000
#define JFMT_FLAG_NESTED                0x002000
#define JFMT_FLAG_SHOW_NAME             0x004000    /* See append_function_args() */
#define JFMT_FLAG_SINGLE_QUOTES         0x008000
#define JFMT_FLAG_DOUBLE_QUOTES         0x010000
#define JFMT_FLAG_USE_ONLY_TYPE_NAME    0x020000
#define JFMT_FLAG_USE_NULL_FOR_NULL     0x040000
#define JFMT_FLAG_USE_BLANK_FOR_NULL    0x080000

/* masking */
#define JFMT_FLAGS_FMT_MASK             0x00000f
#define JFMT_FLAGS_NOT_FMT_MASK         0xfffff0
#define EXTRACT_JFMT_FMT(f) ((f) & JFMT_FLAGS_FMT_MASK)

#define JFMT_FLAGS_ORIGIN_MASK          0x000ff0
#define EXTRACT_JFMT_ORIGIN(f) ((f) & JFMT_FLAGS_ORIGIN_MASK)

#define NAN_DISPLAY_STRING              "NaN"
#define EMPTY_DISPLAY_STRING            "…"
#define UNDEFINED_DISPLAY_STRING        "undefined"
#define NULL_DISPLAY_STRING             "null"

#define TABWIDTH 4

int jpr_exec_debug_show(
                    struct jrunexec * jx,
                    char * dbgstr,
                    int * ix);
int jpr_jvarvalue_tostring(
    struct jrunexec * jx,
    char ** prbuf,
    int * prlen,
    int * prmax,
    struct jvarvalue * jvv,
    int vflags);
int jpr_exec_debug_show_help(struct jrunexec * jx);
void jpr_xstack_char(char * xbuf, int xstate);
int jpr_exec_debug_show_runstack(struct jrunexec * jx);
int jpr_jvarvalue_tocharstar(
    struct jrunexec * jx,
    char ** charstar,
    struct jvarvalue * jvv,
    int vflags);
int jpr_exec_debug_show_allvars(struct jrunexec * jx, int show_flags);
int jvar_interpolate_chars(
    struct jrunexec * jx,
    const char * jchars,
    int jcharslen,
    char ** p_strval,
    int * p_strvallen); /* 08/02/2022 */

/***************************************************************/
#endif /* PRINTH_INCLUDED */
