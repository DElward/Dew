#ifndef PRINTH_INCLUDED
#define PRINTH_INCLUDED
/***************************************************************/
/*  print.h --  Debug/print functions                          */
/***************************************************************/

#define JPR_FLAG_SHOW_VARS          1
#define JPR_FLAG_SHOW_CLASSES       2
#define JPR_FLAG_SHOW_FUNCTIONS     4
#define JPR_FLAG_SHOW_LVALUE        8
#define JPR_FLAG_SHOW_OBJECTS       16
#define JPR_FLAG_SHOW_TYPE          0x100
#define JPR_FLAGS_SHOW_QUOTES       0x200
#define JPR_FLAG_TYPE_TO_STRING     0x400
#define JPR_FLAG_LOG                0x800

#define JPR_FLAGS_ALL_TYPES         0xff
#define JPR_FLAGS_TO_STRING_TYPES   (JPR_FLAG_TYPE_TO_STRING | JPR_FLAGS_ALL_TYPES)
#define JPR_FLAGS_LOG               (JPR_FLAG_LOG | JPR_FLAGS_ALL_TYPES)

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

/***************************************************************/
#endif /* PRINTH_INCLUDED */
