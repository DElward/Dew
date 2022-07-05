/*
**  JNICTest.c  -- JNI test program in C
**
**  References:
**      http://docs.oracle.com/javase/1.5.0/docs/guide/jni/spec/functions.html
**
**  Makefile for dynamic load on Plaid (Linux 32)
*/

/* ******** ******** ******** ******** */
/*
    #define's to help debug memory issue
*/
#define DEBUG_MEM_MARKERS   0

/* ******** ******** ******** ******** */

#define DYN_LOAD    1

#ifdef WIN32
    #define IS_WINDOWS  1
#else
    #define IS_WINDOWS  0
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#include <stdarg.h>
#include <time.h>

#include "jni.h"
#include "cmemh.h"
#include "dbtreeh.h"
#include "snprfh.h"

#define VSMPRINTF  Vsmprintf
#define SNPRINTF   Snprintf
#define SMPRINTF   Smprintf

#define IStrlen(s)   ((int)strlen(s))

int dejaTrace(struct djenv * dje, const char * fmt, ...);

#if DEBUG_MEM_MARKERS
    #define DEBUG_MEM_MAX_FRAME_DEPTH   32
#endif


#if IS_WINDOWS
    #define WIN32_LEAN_AND_MEAN
    #include <Windows.h>
    #include <sys/timeb.h>
    #include <process.h>
#if (WINVER >= 0x0500)
    #include <Psapi.h>
#else

typedef struct _PROCESS_MEMORY_COUNTERS {
    DWORD cb;
    DWORD PageFaultCount;
    SIZE_T PeakWorkingSetSize;
    SIZE_T WorkingSetSize;
    SIZE_T QuotaPeakPagedPoolUsage;
    SIZE_T QuotaPagedPoolUsage;
    SIZE_T QuotaPeakNonPagedPoolUsage;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
    SIZE_T PeakPagefileUsage;
} PROCESS_MEMORY_COUNTERS;

#endif
#else
    #define USE_DLOPEN 1
    #define JNICALL
#endif

#if (LONG_MAX == 2147483647L)
    #define LONGS_64_BIT    0
#else
    #define LONGS_64_BIT    1  /* Hopefully */
#endif

#if IS_WINDOWS
    #define INT64_T __int64
    typedef __int64 int64;
    typedef unsigned __int64 uint64;
#else
    #define INT64_T long long
    typedef long long int64;
    typedef unsigned long long uint64;
#endif

#if DYN_LOAD
    #if IS_WINDOWS

    #elif defined(__hpux)

    #elif defined(USE_DLOPEN)
        #include <dlfcn.h>
    #else
        #error "No dynamic link library"
    #endif

    typedef
         jint  (JNICALL *p_JNI_CreateJavaVM) (
                                       JavaVM **pvm,
                                       void **penv,
                                       void *args);

    p_JNI_CreateJavaVM     JNI_CREATEJAVAVM     = NULL;
    #define LOAD_MSG            "Dynamic load of JNI_CreateJavaVM from %s\n"
#else
    #define JNI_CREATEJAVAVM    JNI_CreateJavaVM
    #define LOAD_MSG            "Static load of JNI_CreateJavaVM\n"
#endif

#ifndef New
    #define New(t,n)         ((t*)calloc(sizeof(t),(n)))
    #define Realloc(p,t,n)   ((t*)realloc((p),sizeof(t)*(n)))
    #define Free(p)           free(p)
    #define Strdup(s)        (strcpy(New(char,strlen(s)+1),(s)))
#endif
#define Strapp(v,s) \
{int vl=(int)strlen(v);(v)=Realloc((v),char,vl+strlen(s)+1);strcpy((v)+vl,(s));}

static int global_opt_pause     = 0;
static int global_opt_verbose   = 0;

#define MAX_VM_OPTS 16







/***************************************************************/
/*     Utility functions                                       */
/***************************************************************/
void strmcpy(char * tgt, const char * src, size_t tmax) {
/*
** Copies src to tgt, trucating src if too intr.
**  tgt always termintated with '\0'
*/

    size_t slen;

    slen = strlen(src) + 1;

    if (slen <= tmax) {
        memcpy(tgt, src, slen);
    } else if (tmax > 0) {  /* > 0 added 5/6/2013 */
        slen = tmax - 1;
        memcpy(tgt, src, slen);
        tgt[slen] = '\0';   /* changed from tgt[tmax] on 4/30/2013 */
    }
}
/***************************************************************/
/*@*/int os_fexists(const char * fname) {

    int fexists;

#if IS_WINDOWS
    int fAttr;

    fexists = 0;

    fAttr = GetFileAttributes(fname);
    if (fAttr > 0) fexists = 1;
#else
    fexists = 0;
    if (os_ftype(fname)) fexists = 1;
#endif

    return (fexists);
}
/***************************************************************/
int os_getpid() {

    int pid;

#if IS_WINDOWS
    pid = _getpid();
#else
    pid = getpid();
#endif

    return (pid);
}
/***************************************************************/
#if IS_WINDOWS
static int win_get_registry_jvm(char * reg_val, int reg_val_sz);
#endif  /* IS_WINDOWS */
/***************************************************************/
int os_get_registry_jvm(char * reg_val, int reg_val_sz) {
/*
 *	 Windows:
 *		 Uses registry value of:
 *			HKEY_LOCAL_MACHINE\SOFTWARE\JavaSoft\Java Runtime Environment\
 *				CurrentVersion\RuntimeLib
 *
 *	 Linux 32:
 *		 Uses: /usr/lib/jvm/jre/lib/i386/client/libjvm.so
 */
    int regerr;

#if IS_WINDOWS
    regerr = win_get_registry_jvm(reg_val, reg_val_sz);
#elif defined(LINUX_X86)
    strmcpy(reg_val, "/usr/lib/jvm/jre/lib/i386/client/libjvm.so", reg_val_sz);
    regerr = 0;
#else
    reg_val[0] = '\0';
    regerr = -1;
#endif

    return (regerr);
}
/***************************************************************/
#if IS_WINDOWS
static void win_get_message(long errCode, char * errMsg, long errMsgSize) {

    LPVOID lpMsgBuf;

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        errCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (LPTSTR) &lpMsgBuf,
        0,
        NULL
    );
    strncpy(errMsg, lpMsgBuf, errMsgSize);

    LocalFree( lpMsgBuf );
}
#endif
/***************************************************************/
void ux_get_message(long errCode, char * errMsg, long errMsgSize) {

    strncpy(errMsg, strerror(errCode), errMsgSize);
}
/***************************************************************/
void get_message(long errCode, char * errMsg, long errMsgSize) {

#if IS_WINDOWS
    win_get_message(errCode, errMsg, errMsgSize);
#else
    ux_get_message(errCode, errMsg, errMsgSize);
#endif
}
/***************************************************************/
char * get_message_str(long errCode) {

    static char osmsg[256];

    get_message(errCode, osmsg, sizeof(osmsg));

    return (osmsg);
}
/***************************************************************/
static void * dynldlibfunction(const char * libname,
                    const char * procname,
                    char* errmsg,
                    int errmsgmax) {
    void * library;
    void * plabel;

#if IS_WINDOWS
    library = LoadLibrary(libname);
    if ((int)library < 32) {
        get_message(GetLastError(), errmsg, errmsgmax);
        plabel = NULL;
    } else {
        plabel = GetProcAddress(library, procname);
        if (!plabel) {
            get_message(GetLastError(), errmsg, errmsgmax);
        }
    }
#elif defined(__hpux)
    int lflags;
    int result;
    lflags = BIND_DEFERRED /* | BIND_VERBOSE */;
    library = shl_load(libname, lflags, 0L);
    if (!library) {
        int ern;
        ern = errno;
        get_message(ern, errmsg, errmsgmax);
        plabel = NULL:
    } else {
        result = shl_findsym((void*)(&library), procname, TYPE_PROCEDURE, &plabel);
        if (result) {
            int ern;
            ern = errno;
            get_message(ern, errmsg, errmsgmax);
            plabel = NULL;
        }
    }
#elif defined(USE_DLOPEN)
    int lflags;
    lflags = RTLD_NOW;
    library = dlopen(libname, lflags);

    if (!library) {
        sprintf(errmsg, "Cannot load existing library: %s\nDL Error: %s",
                libname, dlerror());
        plabel = NULL;
    } else {
        plabel = dlsym(library, procname);
        if (!plabel) {
            sprintf(errmsg, "Cannot find function %s in library: %s\nDL Error: %s",
                    procname, libname, dlerror());
        }
    }
#else
    sprintf(errmsg, "No dynamic load functions available.");
    plabel = NULL:
#endif

    return (plabel);
}
/***************************************************************/
void * dynloadjvm( const char * libname,
                        const char * procname,
                        int * err,
                        char* errmsg,
                        int errmsgmax) {
    void * plabel;

    errmsg[0] = '\0';
    (*err) = 0;

    plabel = dynldlibfunction(libname, procname, errmsg, errmsgmax);
    if (!plabel) (*err) = 1;

    return (plabel);
}
/***************************************************************/
int is_wow_64(void) {
/*
** Returns:
**      -1 - Could not determine Wow64
**       0 - Is not Wow64
**       1 - Is Wow64
*/
    int is64;
    BOOL bIs64;
    HMODULE hLibrary;
    typedef BOOL (WINAPI *p_IsWow64Process) (
                    HANDLE hProcess,
                    PBOOL Wow64Process);

    static int loaded_IsWow64Process = 0;
    static p_IsWow64Process dl_IsWow64Process = NULL;

    if (!loaded_IsWow64Process) {
        loaded_IsWow64Process = -1;
        hLibrary = LoadLibrary("KERNEL32.DLL");
        if ((int)hLibrary >= 32) {
            dl_IsWow64Process = (p_IsWow64Process)
                            GetProcAddress(hLibrary, "IsWow64Process");
            if (dl_IsWow64Process) loaded_IsWow64Process = 1;
        }
    }

    if (loaded_IsWow64Process < 0) {
        is64 = -1;
    } else {
        bIs64 = 0;
        if (dl_IsWow64Process(GetCurrentProcess(), &bIs64)) {
            is64 = bIs64;
        } else {
            is64 = -1;
        }
    }

    return (is64);
}
typedef DWORD (WINAPI *p_GetProcessMemoryInfo) (
    HANDLE Process,
    PROCESS_MEMORY_COUNTERS * ppsmemCounters,
    DWORD cb);
static p_GetProcessMemoryInfo  dl_GetProcessMemoryInfo;
static int stat_GetProcessMemoryInfo = 0;
/***************************************************************/
static void load_GetProcessMemoryInfo(int verbose) {

    HMODULE hLibrary;

    hLibrary = LoadLibrary("PSAPI.DLL");
    if ((int64)hLibrary < 32) { 
        if (verbose) {
            printf("Error loading library PSAPI.DLL: %d\n",
                    (int)hLibrary);
        }
        stat_GetProcessMemoryInfo = -2;
    } else {
        dl_GetProcessMemoryInfo = (p_GetProcessMemoryInfo)
                        GetProcAddress(hLibrary, "GetProcessMemoryInfo");
        if (!dl_GetProcessMemoryInfo) {
            stat_GetProcessMemoryInfo = -1;
            if (verbose) {
                printf("Error loading procedure GetProcessMemoryInfo()\n");
            }
        } else {
            stat_GetProcessMemoryInfo = 1;
        }
    }
}
/***************************************************************/
int64 win_get_process_memory_usage(void) {

    int64 mem_usage;
    int rtn;
    PROCESS_MEMORY_COUNTERS mem_counters;

    if (!stat_GetProcessMemoryInfo) load_GetProcessMemoryInfo(0);
    if (stat_GetProcessMemoryInfo < 0) return (-1);

    mem_usage = 0;

    rtn = dl_GetProcessMemoryInfo(GetCurrentProcess(),
                &mem_counters, sizeof(mem_counters));
    if (rtn) {
        mem_usage = mem_counters.WorkingSetSize;
    }

    return (mem_usage);
}
/***************************************************************/
int os_get_process_memory_usage(int * megabytes, int * nbytes) {

    int rtn;
    int64 mem_usage;

    rtn = 0;
    (*megabytes) = 0;
    (*nbytes)    = 0;

#if IS_WINDOWS
    mem_usage = win_get_process_memory_usage();
#elif defined(LINUX_COMPILE)
    mem_usage = linux_get_process_memory_usage();
#else
    mem_usage = -1;
#endif

    if (mem_usage < 0) {
        rtn = -1;
    } else {
        (*megabytes) = (int)(mem_usage >> 20);
        (*nbytes)    = (int)(mem_usage & 0xFFFFFL);
    }

    return (rtn);

}
/***************************************************************/
int os_get_process_memory_megabytes(void) {

    int rtn;
    int megabytes;
    int nbytes;

    rtn = os_get_process_memory_usage(&megabytes, &nbytes);
    if (rtn >= 0) rtn = megabytes;

    return (rtn);
}
/***************************************************************/
int set_error (const char * fmt, ...) {

    va_list args;

    va_start (args, fmt);
    vfprintf(stderr, fmt, args);
    va_end (args);
    fprintf(stderr, "\n");

    return (-1);
}
/***************************************************************/





/***************************************************************/
/***************************************************************/
/*
**  deja.c  --  Functions for deja
*/
/***************************************************************/
/*
History:
06/10/13 DTE  0.0 Began work
*/
/***************************************************************/

#if defined(WIN32)
    #define CLASS_PATH_DELIM    ';'
#else
    #define CLASS_PATH_DELIM    ':'
#endif



/*
    Todo:
        Allow variable paramters to be casted.
                See djSignature_to_djmethrec() case 'v'
        Allow field access (call, GetFieldID(), Get<type>Field()
        Support arrays

----------------------------------------------------------------

    References:
        JNI:
    http://docs.oracle.com/javase/1.5.0/docs/guide/jni/spec/functions.html

        Java grammar:
        http://docs.oracle.com/javase/specs/
            http://slps.github.io/zoo/#java
            http://slps.github.io/zoo/java/java-5-jls-impl.html

*/

typedef jint (JNICALL *p_JNI_CreateJavaVM)(JavaVM **pvm,void **penv,void *args);

/***************************************************************/

#define DJKERR_EOF              (-101)

#define DJERRFINDJVM            (8401)
#define DJERRLOADJVM            (8402)
#define DJERRCREATEJVM          (8403)
#define DJERRUNIMPLEMENTED      (8404)
#define DJERROPENPTFILE         (8405)
#define DJERRREADPTFILE         (8406)
#define DJERREXPCLASS           (8407)
#define DJERRDUPCLASS           (8408)
#define DJERRBADCLASSNAME       (8409)
#define DJERREXPCLOSETYPE       (8410)
#define DJERREXPCLOSEBODY       (8411)
#define DJERREXPCLOSEBRACE      (8412)
#define DJERREXPOPENBODY        (8413)
#define DJERRNOPACKAGE          (8414)
#define DJERREXPCLOSEBRACKET    (8415)
#define DJERREXPSEMICOLON       (8416)
#define DJERRNOINIT             (8417)
#define DJERREXPOPENPAREN       (8418)
#define DJERREXPELIPSES         (8419)
#define DJERRBADIDENTIFIER      (8420)
#define DJERREXPCLOSEPAREN      (8421)
#define DJERRFIELDMETHOD        (8422)
#define DJERRDUPOVERLOAD        (8423)
#define DJERREXPQUESTION        (8424)
#define DJERRNOVARCONTEXT       (8425)
#define DJERRVARCLASSMISMATCH   (8426)
#define DJERRJNIPUSHFRAME       (8427)
#define DJERRJNIVERSION         (8428)
#define DJERRDUPVAR             (8429)
#define DJERRJNINEWSTRING       (8428)
#define DJERRFINDCLASS          (8431)
#define DJERRNOJVM              (8432)
#define DJERREXPEOF             (8433)
#define DJERRBADSIGCHAR         (8434)
#define DJERREXCEEDSMAXPARMS    (8435)
#define DJERRFINDMETHOD         (8436)
#define DJERRJNIFINDMETHOD      (8437)
#define DJERRCALLVAR            (8438)
#define DJERRNOINSTANCE         (8439)
#define DJERRVARNOTOBJECT       (8440)
#define DJERRJNIPROCID          (8441)
#define DJERREXCEPTION          (8442)
#define DJERRNOSUCHVAR          (8443)
#define DJERROTINSTANTIATED     (8444)
#define DJERREXPCLOSECAST       (8445)
#define DJERRNOCLASS            (8446)
#define DJERRNOCAST             (8447)
#define DJVARNOTFOUND           (8448)
#define DJNEWVARREF             (8449)
#define DJERRNOJVMFOUND         (8450)

/***************************************************************/
#define DJC_CLS_MOD_PUBLIC      1
#define DJC_CLS_MOD_ABSTRACT    2
#define DJC_CLS_MOD_FINAL       4
#define DJC_CLS_MOD_STRICTFP    8

#define DJC_PARM_MOD_FINAL      16

#define DJC_MEM_MOD_PUBLIC          256
#define DJC_MEM_MOD_PRIVATE         512
#define DJC_MEM_MOD_PROTECTED       1024
#define DJC_MEM_MOD_STATIC          2028
#define DJC_MEM_MOD_ABSTRACT        4096
#define DJC_MEM_MOD_FINAL           8192
#define DJC_MEM_MOD_TRANSIENT       16384
#define DJC_MEM_MOD_VOLATILE        32768
#define DJC_MEM_MOD_SYNCHRONIZED    65536
#define DJC_MEM_MOD_NATIVE          131072

/***************************************************************/
#define DJT_TYPE_VOID           1
#define DJT_TYPE_BOOLEAN        2
#define DJT_TYPE_BYTE           3
#define DJT_TYPE_SHORT          4
#define DJT_TYPE_INT            5
#define DJT_TYPE_LONG           6
#define DJT_TYPE_CHAR           7
#define DJT_TYPE_FLOAT          8
#define DJT_TYPE_DOUBLE         9
#define DJT_TYPE_CLASS          16
#define DJT_TYPE_TYPED_CLASS    17

/***************************************************************/
#define JNIPROC_VOID            1
#define JNIPROC_BOOLEAN         2
#define JNIPROC_BYTE            3
#define JNIPROC_SHORT           4
#define JNIPROC_INT             5
#define JNIPROC_LONG            6
#define JNIPROC_CHAR            7
#define JNIPROC_FLOAT           8
#define JNIPROC_DOUBLE          9
#define JNIPROC_OBJECT          10
#define JNIPROC_NEW             11

/***************************************************************/

#define JS_ERR_IX_DELTA 1000

#define DEFAULT_FRAME_CAPACITY  64

#define DEJA_LINE_MAX       1024
#define DEJA_TOKE_MAX       256

#define DEJA_FULL_NAME_MAX   DEJA_TOKE_MAX
#define DEJA_CLASS_NAME_MAX  DEJA_TOKE_MAX
#define DEJA_MEMBER_NAME_MAX DEJA_TOKE_MAX

#define JAVA_VAR_CHAR(ch) (isalnum(ch) || (ch) == '_' || (ch) == '$')
#define JAVA_FIRST_VAR_CHAR(ch) (isalpha(ch) || (ch) == '_' || (ch) == '$')

#define JNI_INIT_METHOD     "<init>"
#define JNI_MAX_PARMS       32

/*
typedef union jvalue {
    jboolean z;
    jbyte    b;
    jchar    c;
    jshort   s;
    jint     i;
    jlong    j;
    jfloat   f;
    jdouble  d;
    jobject  l;
} jvalue;

union jni_parm_union {
    char        pu_cchar;
    double      pu_cdouble;
    float       pu_cfloat;
    int         pu_cint;
    long        pu_clong;
    void*       pu_ptr;
    int16       pu_int16;
    int32       pu_int32;
    int64       pu_int64;
    REG_T       pu_reg;
};
*/
/***************************************************************/
/* Globals                                                     */
/*                                                             */
/* Necessary because only 1 JVM is permitted to be running     */
/* per process.                                                */
/*                                                             */
/***************************************************************/
static JNIEnv *     global_je_env       = NULL;
static JavaVM *     global_jvm          = NULL;
static int          global_je_env_count = 0;

#define DJEFLAG_DEBUG               1
#define DJEFLAG_DESTROY_UNUSED_JVM  2
#define DJEFLAG_PUSH_LOCAL_FRAMES   4
#define DJEFLAG_ENABLE_HEAP_SIZE    8

#define DJE_FLAGS_DEFAULT           DJEFLAG_PUSH_LOCAL_FRAMES

#define DJEFLAG_DEBUG_OPTION_VERBOSE_CLASS      2
#define DJEFLAG_DEBUG_OPTION_VERBOSE_JNI        4
#define DJEFLAG_DEBUG_OPTION_VERBOSE_GC         8
#define DJEFLAG_DEBUG_OPTION_HEAP               16
#define DJEFLAG_DEBUG_OPTION_JMXREMOTE          32
#define DJEFLAG_DEBUG_OPTION_FLAGS              64

/***************************************************************/

typedef jvalue JNI_PARM;

struct djerr {  /* djx_ */
    int     djx_errnum;
    char *  djx_errmsg;
};
/***************************************************************/
/*
    Structures:
        struct djclass      djc_
        struct djclassdef   djd_
        struct djformals    djf_
        struct djpackage    djk_
        struct djmember     djm_
        struct djvarnest    djn_
        struct djforparm    djp_
        struct djmethrec    djr_
        struct djtype       djt_
        struct djvar        djv_
        struct djtypeparm   dju_

*/
#define DJI_TYPE_FIELD      1
#define DJI_TYPE_METHOD     2

/***************************************************************/
struct djpackage {  /* djk_ */
    char *                  djk_pkg_name;
    struct djenv *          djk_dje;
    struct dbtree *         djk_classes;
};
/***************************************************************/
struct djenv {  /* djx_ */
    JNIEnv *                dje_je_env;
    JavaVM *                dje_jvm;
    JavaVMOption *          dje_vm_options;
    char *                  dje_jvmlib_name;
    int                     dje_nvm_options;
    int                     dje_nclasspath;
    int                     dje_frame_capacity;
    FILE *                  dje_trace_file;
    int                     dje_jvm_version;
    char **                 dje_classpath;
    p_JNI_CreateJavaVM      dje_dJNI_CreateJavaVM;
    int                     dje_nerrs;
    int                     dje_flags;
    int                     dje_mem_debug;  /* DEBUG_MEM_MARKERS */
    struct djerr **         dje_errs;
    struct dbtree *         dje_packages;
    struct djvarnest *      dje_varnest;
    jthrowable              dje_ex;
    jstring                 dje_JNI_err_jstring;
    const char *            dje_JNI_err_chars;
    char *                  dje_JNI_error_msg;
    char *                  dje_debug_JNI_err_jstring;
#if DEBUG_MEM_MARKERS
    /* Only used #if DEBUG_MEM_MARKERS */
    int                     dje_debug_frame_depth;
    char *                  dje_debug_frame_marker[DEBUG_MEM_MAX_FRAME_DEPTH];
#endif
    /* Only to get Jave heap in information */
    jclass                  dje_heap_RuntimeClass;
    jobject                 dje_heap_RuntimeObject;
    jmethodID               dje_heap_gcMethod;
    jmethodID               dje_heap_freeMemoryMethod;
    jmethodID               dje_heap_maxMemoryMethod;
    jmethodID               dje_heap_totalMemoryMethod;
};
/***************************************************************/
struct djvar {  /* djv_ */
    char *                  djv_var_name;
    struct djtype *         djv_var_type;
    int                     djv_copied;
    jobject                 djv_jobject;
    /* Only used #if DEBUG_MEM_MARKERS */
    char *                  djv_debug_marker;
};
/***************************************************************/
struct djmethrec {  /* djr_ */
    int                     djr_nparms;
    char *                  djr_jni_signature;
    struct djformals *      djr_formals;
    JNI_PARM                djr_rtn_val;
    JNI_PARM                djr_parms[JNI_MAX_PARMS];
    int                     djr_rel_flag[JNI_MAX_PARMS];
    char *                  djr_debug_parm_marker[JNI_MAX_PARMS];
};
/***************************************************************/
struct djvarnest {  /* djn_ */
    struct dbtree *         djn_vars;
    struct djvarnest *      djn_prev_varnest;
};
/***************************************************************/
struct djtypeparm {  /* dju_ */
    char *                  dju_tparm_name;
    int                     dju_nbound;
    struct djtype **        dju_bound;
};
/***************************************************************/
struct djtype {  /* djt_ */
    int                     djt_typetype;
    struct djclass *        djt_class;
    int                     djt_ntype_args;
    struct djtype **        djt_type_args;
};
/***************************************************************/
struct djforparm {  /* djp_ */
    struct djformals *      djp_parent;
    char *                  djp_parm_name;
    struct djtype *         djp_parm_type;
    int                     djp_narray_dims;
    int                     djp_is_vararg;
};
/***************************************************************/
struct djformals {  /* djf_ */
    struct djmember *       djf_parent;
    struct djtype *         djf_rtn_type;
    jmethodID               djf_jmethodID;
    char *                  djj_method_name;
    int                     djf_jni_procID;
    int                     djf_nforparms;
    struct djforparm **     djf_forparms;
    int                     djf_nthrows;
    struct djclass **       djf_throws;
    int                     djf_narray_dims;
    int                     djf_ntype_parms;
    struct djtypeparm **    djf_type_parms;
};
/***************************************************************/
struct djmember {  /* djm_ */
    char *                  djm_member_name;
    struct djclassdef *     djm_parent;
    int                     djm_field_method;
    struct djtype *         djm_field_type;
    int                     djm_modifiers;
    int                     djm_nformals;
    struct djformals **     djm_formals;
};
/***************************************************************/
struct djclassdef {  /* djd_ */

    struct djclass *        djd_parent;
    int                     djd_modifiers;
    struct djtype *         djd_superclass;
    struct dbtree *         djd_members;
    int                     djd_ntype_parms;
    struct djtypeparm **    djd_type_parms;
    int                     djd_nimplements;
    struct djtype **        djd_implements;
};
/***************************************************************/
struct djclass {  /* djc_ */
    struct djpackage *      djc_parent;
    char *                  djc_class_name;
    char *                  djc_compiled_from;
    struct djclassdef *     djc_classdef;
    struct djclassdef *     djc_implied_classdef;
    jclass                  djc_jclass;
    int                     djc_ncasts;
    char **                 djc_casts;
};

/* Forward functions */
static int dj_release_variable(struct djenv * dje, struct djvar * djv);

/***************************************************************/
static int dj_set_error(struct djenv * dje, int injstat, char *fmt, ...) {

    va_list args;
    int eix;
    char * emsg;
    struct djerr * djx;

    va_start (args, fmt);
    emsg = VSMPRINTF (fmt, args);
    va_end (args);

    eix = 0;
    while (eix < dje->dje_nerrs && dje->dje_errs[eix]) eix++;

    if (eix == dje->dje_nerrs) {
        dje->dje_errs =
            Realloc(dje->dje_errs, struct djerr*, dje->dje_nerrs + 1);
        eix = dje->dje_nerrs;
        dje->dje_nerrs += 1;
    }

    djx = New(struct djerr, 1);
    dje->dje_errs[eix] = djx;

    djx->djx_errmsg = emsg;
    djx->djx_errnum = injstat;

    return (eix + JS_ERR_IX_DELTA);
}
/***************************************************************/
static int dj_clear_error(struct djenv * dje, int eref) {

    int eix;

    if (eref < 0) return (0);

    eix = eref - JS_ERR_IX_DELTA;
    if (eix < 0 || eix >= dje->dje_nerrs) return (-1);

    Free(dje->dje_errs[eix]->djx_errmsg);
    Free(dje->dje_errs[eix]);
    dje->dje_errs[eix] = NULL;

    while (dje->dje_nerrs > 0 && dje->dje_errs[dje->dje_nerrs - 1] == NULL)
        dje->dje_nerrs -= 1;

    return (0);
}
/***************************************************************/
static void dj_clear_all_errors(struct djenv * dje) {

    int eix;

    while (dje->dje_nerrs > 0) {
        eix = dje->dje_nerrs;
        while (eix > 0 && dje->dje_errs[eix - 1] == NULL) eix--;
        if (eix) {
            dj_clear_error(dje, eix + JS_ERR_IX_DELTA - 1); /* 02/19/2015 */
        } else {
            dje->dje_nerrs = 0; /* stop loop */
        }
    }

    Free(dje->dje_errs);
    dje->dje_errs = NULL;
}
/***************************************************************/
static char * dj_get_error_msg(struct djenv * dje, int eref) {

    int eix;
    char * emsg;

    if (eref < 0) {
        switch (eref) {
            case DJKERR_EOF:
                emsg = SMPRINTF("%s DJKERR %d",
                    "End of file", eref);
                break;
            default:
                emsg = SMPRINTF("%s DJKERR %d",
                    "Unknown error number", eref);
                break;
        }
    } else {
        eix = eref - JS_ERR_IX_DELTA;
        if (eix < 0 || eix >= dje->dje_nerrs) return NULL;
        if (!dje->dje_errs[eix]) return NULL;

        emsg = SMPRINTF("%s DJERR %d",
            dje->dje_errs[eix]->djx_errmsg, dje->dje_errs[eix]->djx_errnum);
    }

    return (emsg);
}
/***************************************************************/
static int dj_show_error(struct djenv * dje, FILE * outf, int eref) {

    char * emsg;

    emsg = dj_get_error_msg(dje, eref);
    if (!emsg) return (-1);

    fprintf(outf, "%s\n", emsg);

    dj_clear_error(dje, eref);
    Free(emsg);

    return (0);
}
/***************************************************************/
static int dj_find_jvmlib_name(char * jvmlib, int jvmlib_sz) {

    int jstat;
    char * jvmlibvar;

    jstat = 0;
    jvmlibvar    = getenv("JVMLIB");
    if (jvmlibvar && jvmlibvar[0]) {
        strmcpy(jvmlib, jvmlibvar, jvmlib_sz);
    } else {
        if (os_get_registry_jvm(jvmlib, jvmlib_sz)) {
            jstat = 2;
        }
    }

    return (jstat);
}
/***************************************************************/
static int dj_load_jvmlib(struct djenv * dje, const char * jvmlib) {
/*
Working dir: C:\Program Files\Java\jdk1.7.0_21\jre\bin\client
*/
    int jstat;
    int djerr;
    char errmsg[256];

    djerr = 0;

    /* Do JNI_CreateJavaVM first */
    dje->dje_dJNI_CreateJavaVM = (p_JNI_CreateJavaVM)dynloadjvm(jvmlib,
                        "JNI_CreateJavaVM", &jstat, errmsg, sizeof(errmsg));

    if (!dje->dje_dJNI_CreateJavaVM) {
        if (os_fexists(jvmlib)) {
            if (dje->dje_trace_file) {
                dejaTrace(dje,
                    "FAIL Load JNI_CreateJavaVM(%s) no such file", jvmlib);
            }

            djerr = dj_set_error(dje, DJERRFINDJVM,
                "Unable to find Java runtime library: %s", jvmlib);
        } else {
            if (dje->dje_trace_file) {
                dejaTrace(dje,
                    "FAIL Load JNI_CreateJavaVM(%s) cannot load", jvmlib);
            }

            djerr = dj_set_error(dje, DJERRLOADJVM,
                            "Error loading Java runtime library: %s", errmsg);
        }
    } else {
        if (dje->dje_trace_file) {
            dejaTrace(dje, "succ Load JNI_CreateJavaVM(%s)", jvmlib);
        }
    }

    return (djerr);
}
/***************************************************************/
static void dj_set_classpath_option(struct djenv * dje,
                JavaVMOption * optbuf)
{

    int ii;
    int slen;
    int tlen;
    char * cpopt;

    cpopt = Strdup("-Djava.class.path=");
    tlen = IStrlen(cpopt);

    for ( ii = 0; ii < dje->dje_nclasspath; ii++) {
        tlen += IStrlen(dje->dje_classpath[ii]) + 1; /* 1 for CLASS_PATH_DELIM */
    }
    cpopt = Realloc(cpopt, char, tlen);

    tlen = IStrlen(cpopt);

    for ( ii = 0; ii < dje->dje_nclasspath; ii++) {
        slen = IStrlen(dje->dje_classpath[ii]);
        memcpy(cpopt + tlen, dje->dje_classpath[ii], slen);
        tlen += slen;
        if (cpopt[tlen-1] != CLASS_PATH_DELIM) {
            cpopt[tlen++] = CLASS_PATH_DELIM;
        }
    }
    cpopt[tlen - 1] = '\0'; /* overwrite last CLASS_PATH_DELIM */

    optbuf->optionString = cpopt;
}
/***************************************************************/
/* prototype parsing                                           */
/***************************************************************/
struct djpi_file {
    FILE * djpif_ptf;
    int    djpif_line_num;
    char * djpif_fname;
};
struct djps_stream {
    char * djps_toke;
    int    djps_toke_len;
    int    djps_toke_sz;
    int    djps_buf_ix;
    char * djps_buf;
    int    djps_buf_sz;
    void * djps_data;
    int (*djps_read_next_func)(struct djenv * dje, void *, int * , char *, int);
};
/***************************************************************/
static int djpf_read_next_line(struct djenv * dje,
                        void * vdjpif,
                        int * is_eof,
                        char * fbuf,
                        int fbufsz) {

    int djerr;
    int ern;
    struct djpi_file * djpif = (struct djpi_file *)vdjpif;

    djerr = 0;
    (*is_eof) = 0;

    if (!fgets(fbuf, fbufsz, djpif->djpif_ptf)) {
        ern = errno;
        (*is_eof) = 1;
        if (ferror(djpif->djpif_ptf)) {
            djerr = dj_set_error(dje, DJERRREADPTFILE,
                        "Error reading prototype file %s: %m",
                        djpif->djpif_fname, ern);
        }
    }

    return (djerr);
}
/***************************************************************/
static struct djpi_file * djp_new_djpi_file(FILE * ptf,
                                            const char * ptfname) {

    struct djpi_file * djpif;

    djpif = New(struct djpi_file, 1);
    djpif->djpif_ptf        = ptf;
    djpif->djpif_line_num   = 0;
    djpif->djpif_fname      = Strdup(ptfname);

    return (djpif);
}
/***************************************************************/
static void djp_free_djpi_file(struct djpi_file * djpif) {

    Free(djpif->djpif_fname);
    Free(djpif);
}
/***************************************************************/
static struct djps_stream * djp_new_djps(
                        int (*read_next_func)(struct djenv * dje,
                                              void *,
                                              int * ,
                                              char *,
                                              int),
                        void * vdjpi) {

    struct djps_stream * djps;

    djps = New(struct djps_stream, 1);

    djps->djps_toke_sz          = DEJA_TOKE_MAX;
    djps->djps_toke             = New(char, djps->djps_toke_sz);
    djps->djps_toke_len         = 0;
    djps->djps_buf_sz           = DEJA_LINE_MAX;
    djps->djps_buf              = New(char, djps->djps_buf_sz);
    djps->djps_buf_ix           = 0;
    djps->djps_read_next_func   = read_next_func;
    djps->djps_data             = vdjpi;

    djps->djps_buf[0]           = '\0'; /* to force read first time */

    return (djps);
}
/***************************************************************/
static void djp_free_djps(struct djps_stream * djps) {

    Free(djps->djps_toke);
    Free(djps->djps_buf);
    Free(djps);
}
/***************************************************************/
static int djp_peek_char(struct djenv * dje,
                         struct djps_stream * djps,
                         char * pch) {

    int djerr;
    int is_eof;

    djerr = 0;
    (*pch) = '\0';

    while (isspace(djps->djps_buf[djps->djps_buf_ix])) djps->djps_buf_ix += 1;

    while (!djerr && !djps->djps_buf[djps->djps_buf_ix]) {
        djerr = djps->djps_read_next_func(dje, djps->djps_data, &is_eof,
                djps->djps_buf, djps->djps_buf_sz);
        if (!djerr) {
            if (is_eof) {
                djerr = DJKERR_EOF;
            }
        }
        if (!djerr) {
            djps->djps_buf_ix = 0;
            while (isspace(djps->djps_buf[djps->djps_buf_ix]))
                djps->djps_buf_ix += 1;
        }
    }

    if (!djerr) {
        (*pch) = djps->djps_buf[djps->djps_buf_ix];
    }

    return (djerr);
}
/***************************************************************/
static char djp_char_index(struct djps_stream * djps, int ch_index) {

    int ix;
    char ch;

    if (ch_index < 0) {
        ch = 0;
    } else {
        ch = djps->djps_buf[djps->djps_buf_ix];
        ix = 0;
        while (ch && ix < ch_index) {
            ch = djps->djps_buf[djps->djps_buf_ix + ix];
            ix++;
        }
    }

    return (ch);
}
/***************************************************************/
static void djp_add_index(struct djps_stream * djps, int ch_index) {

    int ix;
    char ch;

    if (ch_index > 0) {
        ch = djps->djps_buf[djps->djps_buf_ix];
        ix = ch_index;
        while (ch && ix) {
            ix--;
            djps->djps_buf_ix += 1;
            ch = djps->djps_buf[djps->djps_buf_ix];
        }
    }
}
/***************************************************************/
static int djp_get_toke(struct djenv * dje, struct djps_stream * djps) {
/*
** Gets next java token
**
** Supports
**      . Blank lines
**      . Double quoted strings
**
** Does not support:
**      . Comments
**      . Multi-character spacial tokens, e.g. <=
**      . Escapes in quotes, e.g. \n
*/

    int djerr;
    char ch;

    djerr = djp_peek_char(dje, djps, &ch);

    if (!djerr) {
        djps->djps_toke_len = 0;

        if (JAVA_VAR_CHAR(ch)) {
            while (JAVA_VAR_CHAR(ch)) {
                if (djps->djps_toke_len < djps->djps_toke_sz)
                    djps->djps_toke[djps->djps_toke_len++] = ch;
                djps->djps_buf_ix += 1;
                ch = djps->djps_buf[djps->djps_buf_ix];
            }

            if (djps->djps_toke_len < djps->djps_toke_sz)
                djps->djps_toke[djps->djps_toke_len] = '\0';
            else djps->djps_toke[djps->djps_toke_sz - 1] = '\0';
        } else if (ch == '\"') {
            do {
                if (djps->djps_toke_len < djps->djps_toke_sz)
                    djps->djps_toke[djps->djps_toke_len++] = ch;
                djps->djps_buf_ix += 1;
                ch = djps->djps_buf[djps->djps_buf_ix];
            } while (ch != '\"');
            if (djps->djps_toke_len < djps->djps_toke_sz)
                djps->djps_toke[djps->djps_toke_len++] = ch;
            djps->djps_buf_ix += 1;

            if (djps->djps_toke_len < djps->djps_toke_sz)
                djps->djps_toke[djps->djps_toke_len] = '\0';
            else djps->djps_toke[djps->djps_toke_sz - 1] = '\0';
        } else {
            djps->djps_toke[djps->djps_toke_len++] = ch;
            djps->djps_toke[djps->djps_toke_len]   = '\0';
            djps->djps_buf_ix += 1;
        }
    }

    return (djerr);
}
/***************************************************************/
static struct djtype * djp_new_djtype(void)
{
/*
*/

    struct djtype * djt;

    djt = New(struct djtype, 1);
/*
*/
    if (!djt) return (NULL);

    djt->djt_typetype    = 0;
    djt->djt_class       = NULL;
    djt->djt_ntype_args  = 0;
    djt->djt_type_args   = NULL;

    return (djt);
}
/***************************************************************/
static void djp_free_djtype(struct djtype * djt)
{
    int ii;

    for (ii = 0; ii < djt->djt_ntype_args; ii++) {
        djp_free_djtype(djt->djt_type_args[ii]);
    }

    Free(djt->djt_type_args);
    Free(djt);
}
/***************************************************************/
static struct djtypeparm * djp_new_djtypeparm(void)
{
    struct djtypeparm * dju;

    dju = New(struct djtypeparm, 1);
    if (!dju) return (NULL);

    dju->dju_tparm_name  = NULL;
    dju->dju_nbound      = 0;
    dju->dju_bound       = NULL;

    return (dju);
}
/***************************************************************/
static void djp_free_djtypeparm(struct djtypeparm * dju)
{
    int ii;

    for (ii = 0; ii < dju->dju_nbound; ii++) {
        djp_free_djtype(dju->dju_bound[ii]);
    }
    Free(dju->dju_bound);
    Free(dju->dju_tparm_name);

    Free(dju);
}
/***************************************************************/
static struct djforparm * djp_new_djforparm(struct djformals * parent,
                                        const char * parm_name,
                                        struct djtype * parm_type,
                                        int narray_dims)

{
    struct djforparm * djp;

    djp = New(struct djforparm, 1);
    if (!djp) return (NULL);

    if (parm_name)  djp->djp_parm_name = Strdup(parm_name);
    else            djp->djp_parm_name = NULL;

    djp->djp_parent         = parent;
    djp->djp_parm_type      = parm_type;
    djp->djp_narray_dims    = 0;
    djp->djp_is_vararg      = 0;

    return (djp);
}
/***************************************************************/
static struct djformals * djp_new_djformals(struct djtype * rtn_type)
{
    struct djformals * djf;

    djf = New(struct djformals, 1);
    if (!djf) return (NULL);

    djf->djf_parent         = NULL;
    djf->djf_rtn_type       = rtn_type;
    djf->djf_jmethodID      = NULL;
    djf->djj_method_name    = NULL;
    djf->djf_jni_procID     = 0;
    djf->djf_nforparms      = 0;
    djf->djf_forparms       = NULL;
    djf->djf_nthrows        = 0;
    djf->djf_throws         = NULL;
    djf->djf_narray_dims    = 0;
    djf->djf_ntype_parms    = 0;
    djf->djf_type_parms     = NULL;

    return (djf);
}
/***************************************************************/
static struct djmember * djp_new_djmember(struct djclassdef * parent,
                                    const char * member_name,
                                    int field_method)
{
    struct djmember * djm;

    djm = New(struct djmember, 1);
    if (!djm) return (NULL);

    djm->djm_member_name    = Strdup(member_name);
    djm->djm_parent         = parent;
    djm->djm_field_method   = field_method;
    djm->djm_field_type     = NULL;
    djm->djm_modifiers      = 0;
    djm->djm_nformals       = 0;
    djm->djm_formals        = NULL;

    return (djm);
}
/***************************************************************/
static struct djclass * djp_new_djclass(void)
{
    struct djclass * djc;

    djc = New(struct djclass, 1);
    if (!djc) return (NULL);

    djc->djc_parent             = NULL;
    djc->djc_class_name         = NULL;
    djc->djc_compiled_from      = NULL;
    djc->djc_classdef           = NULL;
    djc->djc_implied_classdef   = NULL;
    djc->djc_jclass             = NULL;
    djc->djc_ncasts             = 0;
    djc->djc_casts              = NULL;

    return (djc);
}
/***************************************************************/
static struct djclassdef * djp_new_djclassdef(struct djclass * parent)
{
    struct djclassdef * djd;

    djd = New(struct djclassdef, 1);
    if (!djd) return (NULL);

    djd->djd_parent        = parent;
    djd->djd_modifiers     = 0;
    djd->djd_superclass    = NULL;
    djd->djd_members       = NULL;
    djd->djd_ntype_parms   = 0;
    djd->djd_type_parms    = NULL;
    djd->djd_nimplements   = 0;
    djd->djd_implements    = NULL;

    return (djd);
}
/***************************************************************/
void djp_free_forparm(struct djforparm * djp) {

    if (djp->djp_parm_type) djp_free_djtype(djp->djp_parm_type);

    Free(djp->djp_parm_name);
    Free(djp);
}
/***************************************************************/
void djp_free_formals(struct djformals * djf) {

    int ii;

    for (ii = 0; ii < djf->djf_nforparms; ii++) {
        djp_free_forparm(djf->djf_forparms[ii]);
    }
    Free(djf->djf_forparms);
    if (djf->djf_rtn_type) djp_free_djtype(djf->djf_rtn_type);

    for (ii = 0; ii < djf->djf_ntype_parms; ii++) {
        djp_free_djtypeparm(djf->djf_type_parms[ii]);
    }
    Free(djf->djf_type_parms);

    Free(djf->djf_throws);
    Free(djf->djj_method_name);

    Free(djf);
}
/***************************************************************/
void djp_free_member(struct djmember * djm) {

    int ii;

    for (ii = 0; ii < djm->djm_nformals; ii++) {
        djp_free_formals(djm->djm_formals[ii]);
    }
    Free(djm->djm_formals);

    if (djm->djm_field_type) djp_free_djtype(djm->djm_field_type);

    Free(djm->djm_member_name);
    Free(djm);
}
/***************************************************************/
void djp_free_vmember(void * vdjm) {

    struct djmember * djm = (struct djmember *)vdjm;

    djp_free_member(djm);
}
/***************************************************************/
static void djp_free_djclassdef(struct djclassdef * djd)
{
    int ii;

    for (ii = 0; ii < djd->djd_ntype_parms; ii++) {
        djp_free_djtypeparm(djd->djd_type_parms[ii]);
    }
    Free(djd->djd_type_parms);

    for (ii = 0; ii < djd->djd_nimplements; ii++) {
        djp_free_djtype(djd->djd_implements[ii]);
    }
    Free(djd->djd_implements);

    if (djd->djd_superclass) djp_free_djtype(djd->djd_superclass);

    if (djd->djd_members) {
        dbtree_free(djd->djd_members, djp_free_vmember);
    }

    Free(djd);
}
/***************************************************************/
static void djp_free_djclass(struct djclass * djc)
{
    int ii;

    for (ii = 0; ii < djc->djc_ncasts; ii++) {
        Free(djc->djc_casts[ii]);
    }
    Free(djc->djc_casts);

    Free(djc->djc_class_name);
    Free(djc->djc_compiled_from);

    if (djc->djc_classdef) djp_free_djclassdef(djc->djc_classdef);
    if (djc->djc_implied_classdef)
        djp_free_djclassdef(djc->djc_implied_classdef);

    Free(djc);
}
/***************************************************************/
void dji_free_vdjclass(void * vdjc) {

    struct djclass * djc = (struct djclass *)vdjc;

    djp_free_djclass(djc);
}
/***************************************************************/
static void djp_free_djvar(struct djvar * djv)
{
    dj_release_variable(djv->djv_var_type->djt_class->djc_parent->djk_dje, djv);
    Free(djv->djv_var_name);
    djp_free_djtype(djv->djv_var_type);

    Free(djv);
}
/***************************************************************/
void dji_free_vdjvar(void * vdjv) {

    struct djvar * djv = (struct djvar *)vdjv;

    djp_free_djvar(djv);
}
/***************************************************************/
static void djp_free_djvarnest(struct djvarnest * djn)
{

    if (djn->djn_vars) {
        dbtree_free(djn->djn_vars, dji_free_vdjvar);
    }

    Free(djn);
}
/***************************************************************/
static struct djvarnest * djp_new_djvarnest(struct djvarnest * prev_varnest)
{
    struct djvarnest * djn;

    djn = New(struct djvarnest, 1);
    if (!djn) return (NULL);

    djn->djn_vars           = dbtree_new();
    djn->djn_prev_varnest   = prev_varnest;

    return (djn);
}
/***************************************************************/
static struct djmethrec * djp_new_djmethrec(void)
{
    struct djmethrec * djr;

    djr = New(struct djmethrec, 1);

    if (!djr) return (NULL);

    djr->djr_nparms         = 0;
    djr->djr_jni_signature  = NULL;
    djr->djr_formals        = NULL;
    memset(&(djr->djr_rtn_val), 0, sizeof(JNI_PARM));
    memset(djr->djr_parms   , 0, sizeof(JNI_PARM) * JNI_MAX_PARMS);
    memset(djr->djr_rel_flag, 0, sizeof(int) * JNI_MAX_PARMS);
    memset(djr->djr_debug_parm_marker, 0, sizeof(char*) * JNI_MAX_PARMS);

    return (djr);
}
/***************************************************************/
static void djp_free_djmethrec(struct djmethrec * djr)
{
    Free(djr->djr_jni_signature);

    if (djr->djr_formals) djp_free_formals(djr->djr_formals);

    Free(djr);
}
/***************************************************************/
static struct djvar * djp_new_djvar(
    const char * var_name,
    struct djtype * djt)
{
    struct djvar * djv;

    djv = New(struct djvar, 1);
    if (!djv) return (NULL);

    djv->djv_var_name   = Strdup(var_name);
    djv->djv_var_type   = djt;
    djv->djv_copied     = 0;
    djv->djv_jobject    = NULL;
    djv->djv_debug_marker = NULL;

    return (djv);
}
/***************************************************************/
static struct djpackage * djp_new_djpackage(struct djenv * dje,
                        const char * pkg_name)
{
    struct djpackage * djk;

    djk = New(struct djpackage, 1);
    if (!djk) return (NULL);

    djk->djk_pkg_name   = Strdup(pkg_name);
    djk->djk_dje        = dje;
    djk->djk_classes    = NULL;

    return (djk);
}
/***************************************************************/
static void djp_free_djpackage(struct djpackage * djk)
{

    if (djk->djk_classes) {
        dbtree_free(djk->djk_classes, dji_free_vdjclass);
    }

    Free(djk->djk_pkg_name);
    Free(djk);
}
/***************************************************************/
static void djp_free_vdjpackage(void * vdjk)
{

    struct djpackage * djk = (struct djpackage *)vdjk;

    djp_free_djpackage(djk);
}
/***************************************************************/
static int djp_check_variable_name(struct djenv * dje, const char * varnam) {

    int djerr;
    int ii;

    djerr = 0;

    if (!JAVA_FIRST_VAR_CHAR(varnam[0])) {
        djerr = dj_set_error(dje, DJERRBADIDENTIFIER,
            "Identifier contains an invalid first character. Found \"%s\"",
            varnam);
    } else {
        ii = 1;
        while (!djerr && varnam[ii]) {
            if (! JAVA_VAR_CHAR(varnam[ii])) {
                djerr = dj_set_error(dje, DJERRBADIDENTIFIER,
                    "Identifier contains invalid characters. Found \"%s\"",
                    varnam);
            } else {
                ii++;
            }
        }
    }

    return (djerr);
}
/***************************************************************/
static int get_class_modifier(const char * modbuf) {
/*
      12.      <class modifier> ::= public | abstract | final
*/
    int class_mod;

    class_mod = 0;
         if (!strcmp(modbuf, "public"  )) class_mod = DJC_CLS_MOD_PUBLIC;
    else if (!strcmp(modbuf, "abstract")) class_mod = DJC_CLS_MOD_ABSTRACT;
    else if (!strcmp(modbuf, "final"   )) class_mod = DJC_CLS_MOD_FINAL;
    else if (!strcmp(modbuf, "strictfp")) class_mod = DJC_CLS_MOD_STRICTFP;

    return (class_mod);
}
/***************************************************************/
static int djp_parse_class_modifiers(struct djenv * dje,
                                        struct djps_stream * djps,
                                        int * class_modifiers) {
/*
      11.      <class modifiers> ::= <class modifier> |
                <class modifiers> <class modifier>
*/
    int djerr;
    int class_mod;

    djerr = 0;
    (*class_modifiers) = 0;

    class_mod = get_class_modifier(djps->djps_toke);
    while (!djerr && class_mod) {
        (*class_modifiers) |= class_mod;
        djerr = djp_get_toke(dje, djps);
        class_mod = get_class_modifier(djps->djps_toke);
    }

    return (djerr);
}
/***************************************************************/
static int djp_parse_Identifier(struct djenv * dje,
                               struct djps_stream * djps,
                               char * pkg_name,
                               int    pkg_name_sz,
                               char * class_name,
                               int    class_name_sz) {
/*
*/
    int djerr;
    int pnlen;
    int rm;
    char ch;

    djerr = djp_peek_char(dje, djps, &ch);
    pnlen = 0;

    while (!djerr && (ch == '.' || ch == '/')) {
        rm = pkg_name_sz - pnlen - 1;
        if (rm > djps->djps_toke_len) {
            if (pnlen) pkg_name[pnlen++] = ch;
            memcpy(pkg_name + pnlen, djps->djps_toke, djps->djps_toke_len);
            pnlen += djps->djps_toke_len;
        }
        djerr = djp_get_toke(dje, djps);        /* "." */
        if (!djerr) djp_get_toke(dje, djps);
        if (!djerr) djerr = djp_peek_char(dje, djps, &ch);
    }

    if (pnlen + 1 < pkg_name_sz) pkg_name[pnlen]           = '\0';
    else                         pkg_name[pkg_name_sz - 1] = '\0';

    if (!djerr && class_name_sz > djps->djps_toke_len) {
        memcpy(class_name, djps->djps_toke, djps->djps_toke_len + 1);

        if (!JAVA_FIRST_VAR_CHAR(class_name[0])) {
            djerr = dj_set_error(dje, DJERRBADCLASSNAME,
                "Invalid class name \"%s\"", class_name);
        }
    }
    if (!djerr) djp_get_toke(dje, djps);

    return (djerr);
}
/***************************************************************/
static struct djpackage * djp_find_package(struct djenv * dje,
                                            const char * pkg_name) {

    struct djpackage * djk;
    struct djpackage ** vhand;
    int pnlen;

    djk = NULL;
    pnlen = IStrlen(pkg_name);

    if (dje->dje_packages) {
        vhand = (struct djpackage **)dbtree_find(dje->dje_packages,
                                pkg_name, pnlen);
        if (vhand) djk = *vhand;
    }

    return (djk);
}
/***************************************************************/
static struct djpackage * djp_find_or_create_package(struct djenv * dje,
                                const char * pkg_name) {

    struct djpackage * djk;
    struct djpackage ** vhand;
    int pnlen;

    djk = NULL;
    pnlen = IStrlen(pkg_name);

    if (dje->dje_packages) {
        vhand = (struct djpackage **)dbtree_find(dje->dje_packages,
                                pkg_name, pnlen);
        if (vhand) djk = *vhand;
    } else {
        dje->dje_packages = dbtree_new();
    }

    if (!djk) {
        djk = djp_new_djpackage(dje, pkg_name);
        dbtree_insert(dje->dje_packages, pkg_name, pnlen, djk);
    }

    return (djk);
}
/***************************************************************/
/*
static void djp_remove_class_from_package(struct djpackage * djk,
                                       const char * class_name) {
    struct djclass * djc;
    struct djclass ** vhand;
    int cnlen;

    djc = NULL;
    cnlen = strlen(class_name);

    if (djk->djk_classes) {
        vhand = (struct djclass **)dbtree_find(djk->djk_classes,
                                class_name, cnlen);
        if (vhand) djc = *vhand;
    }

    if (djc) {
        djp_free_djclass(djc);
        dbtree_delete(djk->djk_classes, class_name, cnlen);
    }
}
*/
/***************************************************************/
static struct djclass * djp_find_class_in_package(struct djpackage * djk,
                                       const char * class_name) {

    struct djclass * djc;
    struct djclass ** vhand;
    int cnlen;

    djc = NULL;
    cnlen = IStrlen(class_name);

    if (djk->djk_classes) {
        vhand = (struct djclass **)dbtree_find(djk->djk_classes,
                                            class_name, cnlen);
        if (vhand) djc = *vhand;
    }

    return (djc);
}
/***************************************************************/
static void djp_insert_class_into_package(struct djpackage * djk,
                                       struct djclass * djc) {

    int cnlen;

    cnlen = IStrlen(djc->djc_class_name);

    if (!djk->djk_classes) djk->djk_classes = dbtree_new();
    dbtree_insert(djk->djk_classes, djc->djc_class_name, cnlen, djc);
}
/***************************************************************/
static struct djclass * djp_find_or_create_class(struct djenv * dje,
                        const char * pkg_name,
                        const char * class_name) {

    struct djpackage * djk;
    struct djpackage ** vhdjk;
    struct djclass * djc;
    struct djclass ** vhdjc;
    int cnlen;
    int pnlen;

    djk = NULL;
    djc = NULL;
    pnlen = IStrlen(pkg_name);
    cnlen = IStrlen(class_name);

    if (dje->dje_packages) {
        vhdjk = (struct djpackage **)dbtree_find(dje->dje_packages,
                                                pkg_name, pnlen);
        if (vhdjk) djk = *vhdjk;
    } else {
        dje->dje_packages = dbtree_new();
    }

    if (!djk) {
        djk = djp_new_djpackage(dje, pkg_name);
        dbtree_insert(dje->dje_packages, pkg_name, pnlen, djk);
    } else if (djk->djk_classes) {
        vhdjc = (struct djclass **)dbtree_find(djk->djk_classes,
                                        class_name, cnlen);
        if (vhdjc) djc = *vhdjc;
    }

    if (!djc) {
        djc = djp_new_djclass();
        djc->djc_parent = djk;

        djc->djc_class_name = New(char, cnlen + 1);
        memcpy(djc->djc_class_name, class_name, cnlen + 1);

        if (!djk->djk_classes) djk->djk_classes = dbtree_new();
        dbtree_insert(djk->djk_classes, class_name, cnlen, djc);
    }

    return (djc);
}
/***************************************************************/
static int djp_parse_type(struct djenv * dje,
                                        struct djps_stream * djps,
                                        struct djtype * djt);

static int djp_parse_type_arguments(struct djenv * dje,
                                        struct djps_stream * djps,
                                        struct djtype * djt) {
    int djerr;
    int done;
    struct djtype * ndjt;

    djerr = 0;
    ndjt = djp_new_djtype();

    done = 0;
    while (!djerr && !done) {
        djerr = djp_parse_type(dje, djps, ndjt);
        if (!djerr) {
            djt->djt_type_args =
        Realloc(djt->djt_type_args, struct djtype *, djt->djt_ntype_args + 1);
            djt->djt_type_args[djt->djt_ntype_args] = ndjt;
            djt->djt_ntype_args += 1;

            if (!strcmp(djps->djps_toke, ",")) {
                djerr = djp_get_toke(dje, djps);
            } else {
                done = 1;
            }
        }
    }

    return (djerr);
}
/***************************************************************/
static int djp_get_simple_type(const char * simptyp) {
/*
      33.      <field modifier> ::= public | protected | private | static |
final | transient | volatile

      42.      <method modifier> ::= public | protected | private | static |
abstract | final
                                                                         |
synchronized | native

*/
    int styp;

    styp = 0;
            if (!strcmp(simptyp, "void")) {
        styp    = DJT_TYPE_VOID;
    } else if (!strcmp(simptyp, "boolean")) {
        styp    = DJT_TYPE_BOOLEAN;
    } else if (!strcmp(simptyp, "byte")) {
        styp    = DJT_TYPE_BYTE;
    } else if (!strcmp(simptyp, "short")) {
        styp    = DJT_TYPE_SHORT;
    } else if (!strcmp(simptyp, "int")) {
        styp    = DJT_TYPE_INT;
    } else if (!strcmp(simptyp, "long")) {
        styp    = DJT_TYPE_LONG;
    } else if (!strcmp(simptyp, "char")) {
        styp    = DJT_TYPE_CHAR;
    } else if (!strcmp(simptyp, "float")) {
        styp    = DJT_TYPE_FLOAT;
    } else if (!strcmp(simptyp, "double")) {
        styp    = DJT_TYPE_DOUBLE;
    }

    return (styp);
}
/***************************************************************/
static int djp_parse_type_info(struct djenv * dje,
                                        struct djps_stream * djps,
                                        struct djtype * djt,
                                        const char * pkg_name,
                                        const char * class_name) {
/*
*/
    int djerr;
    int styp;
    struct djclass * djc;

    djerr = 0;

    styp = djp_get_simple_type(class_name);
    if (styp) {
        if (pkg_name[0]) {
            djerr = dj_set_error(dje, DJERRNOPACKAGE,
                "Package name not permitted for simple type. Found \"%s.%s\"",
                pkg_name, class_name);
        } else {
            djt->djt_typetype    = styp;
        }
    } else {
        djc = djp_find_or_create_class(dje, pkg_name, class_name);
        djt->djt_class = djc;

        if (!strcmp(djps->djps_toke, "<")) {
            djt->djt_typetype    = DJT_TYPE_TYPED_CLASS;
            djerr = djp_get_toke(dje, djps);
            if (!djerr) {
                if (!strcmp(djps->djps_toke, "?")) {
                    djerr = djp_get_toke(dje, djps);
                } else {
                    djerr = djp_parse_type_arguments(dje, djps, djt);
                }

                if (!djerr) {
                    if (!strcmp(djps->djps_toke, ">")) {
                        djerr = djp_get_toke(dje, djps);
                    } else {
                        djerr = dj_set_error(dje, DJERREXPCLOSETYPE,
            "Expecting \">\" to terminate parameterized type. Found \"%s\"",
            djps->djps_toke);
                    }
                }
            }
        } else {
            djt->djt_typetype    = DJT_TYPE_CLASS;
        }
    }

    return (djerr);
}
/***************************************************************/
static int djp_parse_type(struct djenv * dje,
                                        struct djps_stream * djps,
                                        struct djtype * djt) {
/*
*/
    int djerr;
    char pkg_name[DEJA_FULL_NAME_MAX];
    char class_name[DEJA_CLASS_NAME_MAX];

    djerr = djp_parse_Identifier(dje, djps,
            pkg_name, sizeof(pkg_name), class_name, sizeof(class_name));

    if (!djerr) {
        djerr = djp_parse_type_info(dje, djps, djt, pkg_name, class_name);
    }

    return (djerr);
}
/***************************************************************/
static int djp_parse_type_bound(struct djenv * dje,
                                        struct djps_stream * djps,
                                        struct djtypeparm * dju) {

/*
    TypeParameters:       "<" TypeParameter ("," TypeParameter)* ">"
    TypeParameter:        Identifier ("extends" Bound)?
    Bound:                Type ("&" Type)*


    struct djtype * djt;

    djt = djp_new_djtype();

    djerr = djp_parse_type(dje, djps, djt);
    if (!djerr) {
        djd->djd_superclass = djt;
    } else {
        djp_free_djtype(djt);
    }
*/
    int djerr;
    int done;
    struct djtype * djt;

    djerr = 0;
    dju->dju_nbound = 0;
    dju->dju_bound  = NULL;

    done = 0;
    while (!djerr && !done) {
        djt = djp_new_djtype();

        djerr = djp_parse_type(dje, djps, djt);
        if (djerr) {
            djp_free_djtype(djt);
        } else {
            dju->dju_bound =
                Realloc(dju->dju_bound, struct djtype *, dju->dju_nbound + 1);
            dju->dju_bound[dju->dju_nbound] = djt;
            dju->dju_nbound += 1;

            if (!strcmp(djps->djps_toke, "&")) {
                djerr = djp_get_toke(dje, djps);
            } else {
                done = 1;
            }
        }
    }

    return (djerr);
}
/***************************************************************/
static int djp_parse_type_list(struct djenv * dje,
                                        struct djps_stream * djps,
                                        int * p_ntype,
                                        struct djtype *** p_type) {

/*
TypeList:        Type ("," Type)*
*/
    int djerr;
    int done;
    struct djtype * djt;

    djerr = 0;
    (*p_ntype) = 0;
    (*p_type)  = NULL;

    done = 0;
    while (!djerr && !done) {
        djt = djp_new_djtype();

        djerr = djp_parse_type(dje, djps, djt);
        if (djerr) {
            djp_free_djtype(djt);
        } else {
            (*p_type) =
                Realloc((*p_type), struct djtype *, (*p_ntype) + 1);
            (*p_type)[(*p_ntype)] = djt;
            (*p_ntype) += 1;

            if (!strcmp(djps->djps_toke, ",")) {
                djerr = djp_get_toke(dje, djps);
            } else {
                done = 1;
            }
        }
    }

    return (djerr);
}
/***************************************************************/
static char * djp_make_single_Identifier(struct djenv * dje,
                               const char * pkg_name,
                               const char * class_name) {
/*
** See also djp_make_full_class()
*/

    char * id;

    if (pkg_name[0]) {
        id = SMPRINTF("%s.%s", pkg_name, class_name);
    } else {
        id = Strdup(class_name);
    }

    return (id);
}
/***************************************************************/
static int djp_parse_type_parameters(struct djenv * dje,
                                        struct djps_stream * djps,
                                        int * p_ntype_parms,
                                        struct djtypeparm *** p_type_parms) {

/*
    TypeParameters:       "<" TypeParameter ("," TypeParameter)* ">"
    TypeParameter:        Identifier ("extends" Bound)?
    Bound:                Type ("&" Type)*
*/
    int djerr;
    int done;
    struct djtypeparm * dju;
    char pkg_name[DEJA_FULL_NAME_MAX];
    char class_name[DEJA_CLASS_NAME_MAX];

    djerr = 0;
    (*p_ntype_parms) = 0;
    (*p_type_parms)  = NULL;

    done = 0;
    while (!djerr && !done) {
        djerr = djp_parse_Identifier(dje, djps,
                pkg_name, sizeof(pkg_name), class_name, sizeof(class_name));
        if (!djerr) {
            dju = djp_new_djtypeparm();
            dju->dju_tparm_name = djp_make_single_Identifier(dje,
                                        pkg_name, class_name);
            if (!strcmp(djps->djps_toke, "extends")) {
                djerr = djp_get_toke(dje, djps);
                if (!djerr) djerr = djp_parse_type_bound(dje, djps, dju);
            }
        }

        if (!djerr) {
            (*p_type_parms) =
        Realloc((*p_type_parms), struct djtypeparm *, (*p_ntype_parms) + 1);
            (*p_type_parms)[(*p_ntype_parms)] = dju;
            (*p_ntype_parms) += 1;

            if (!strcmp(djps->djps_toke, ",")) {
                djerr = djp_get_toke(dje, djps);
            } else {
                done = 1;
            }
        }
    }

    if (!djerr) {
        if (!strcmp(djps->djps_toke, ">")) {
            djerr = djp_get_toke(dje, djps);
        } else {
            djerr = dj_set_error(dje, DJERREXPCLOSETYPE,
                "Expecting \">\" to terminate type parameters. Found \"%s\"",
                djps->djps_toke);
        }
    }

    return (djerr);
}
/***************************************************************/
static int djp_get_super(struct djenv * dje,
                                        struct djps_stream * djps,
                                        struct djclassdef * djd) {
/*
*/
    int djerr;
    struct djtype * djt;

    djt = djp_new_djtype();

    djerr = djp_parse_type(dje, djps, djt);
    if (!djerr) {
        djd->djd_superclass = djt;
    } else {
        djp_free_djtype(djt);
    }

    return (djerr);
}
/***************************************************************/
static int get_member_modifier(const char * modbuf) {
/*
      33.      <field modifier> ::= public | protected | private | static |
                                    final | transient | volatile

      42.      <method modifier> ::= public | protected | private | static |
                                    abstract | final
                                                                         |
synchronized | native

*/
    int mmod;

    mmod = 0;
         if (!strcmp(modbuf, "public"       )) mmod = DJC_MEM_MOD_PUBLIC;
    else if (!strcmp(modbuf, "private"      )) mmod = DJC_MEM_MOD_PRIVATE;
    else if (!strcmp(modbuf, "protected"    )) mmod = DJC_MEM_MOD_PROTECTED;
    else if (!strcmp(modbuf, "static"       )) mmod = DJC_MEM_MOD_STATIC;
    else if (!strcmp(modbuf, "abstract"     )) mmod = DJC_MEM_MOD_ABSTRACT;
    else if (!strcmp(modbuf, "final"        )) mmod = DJC_MEM_MOD_FINAL;
    else if (!strcmp(modbuf, "transient"    )) mmod = DJC_MEM_MOD_TRANSIENT;
    else if (!strcmp(modbuf, "volatile"     )) mmod = DJC_MEM_MOD_VOLATILE;
    else if (!strcmp(modbuf, "synchronized" )) mmod = DJC_MEM_MOD_SYNCHRONIZED;
    else if (!strcmp(modbuf, "native"       )) mmod = DJC_MEM_MOD_NATIVE;

    return (mmod);
}
/***************************************************************/
static int djp_parse_member_modifiers(struct djenv * dje,
                                        struct djps_stream * djps,
                                        int * member_modifiers) {
/*
      11.      <class modifiers> ::= <class modifier> |
                            <class modifiers> <class modifier>
*/
    int djerr;
    int member_mod;

    djerr = 0;
    (*member_modifiers) = 0;

    member_mod = get_member_modifier(djps->djps_toke);
    while (!djerr && member_mod) {
        (*member_modifiers) |= member_mod;
        djerr = djp_get_toke(dje, djps);
        member_mod = get_member_modifier(djps->djps_toke);
    }

    return (djerr);
}

/***************************************************************/
static int djp_parse_NormalInterfaceDeclaration(struct djenv * dje,
                                        struct djps_stream * djps,
                                        struct djclassdef * djd) {
/*
*/
    int djerr;

    djerr = dj_set_error(dje, DJERRUNIMPLEMENTED,
        "djp_parse_NormalInterfaceDeclaration() not implemented.");

    return (djerr);
}
/***************************************************************/
static int djp_parse_ClassDeclaration(struct djenv * dje,
                                        struct djps_stream * djps,
                                        struct djclassdef * djd) {
/*
*/
    int djerr;

    djerr = dj_set_error(dje, DJERRUNIMPLEMENTED,
        "djp_parse_ClassDeclaration() not implemented.");

    return (djerr);
}
/***************************************************************/
static int djp_parse_Annotations(struct djenv * dje,
                                        struct djps_stream * djps,
                                        struct djclassdef * djd) {
/*
*/
    int djerr;

    djerr = dj_set_error(dje, DJERRUNIMPLEMENTED,
        "Annotations not implemented.");

    return (djerr);
}
/***************************************************************/
void djp_add_variable_forparm(struct djformals * djf,
                                const char * parm_name,
                                struct djtype * djt,
                                int narray_dims)
{
    struct djforparm * djp;

    djp = djp_new_djforparm(djf, parm_name, djt, narray_dims);
    djp->djp_is_vararg  = 1;

    djf->djf_forparms =
        Realloc(djf->djf_forparms, struct djforparm*, djf->djf_nforparms + 1);
    djf->djf_forparms[djf->djf_nforparms] = djp;
    djf->djf_nforparms += 1;
}
/***************************************************************/
void djp_add_forparm(struct djformals * djf,
                        const char * parm_name,
                        struct djtype * djt,
                        int narray_dims)
{
    struct djforparm * djp;

    djp = djp_new_djforparm(djf, parm_name, djt, narray_dims);

    djf->djf_forparms =
        Realloc(djf->djf_forparms, struct djforparm*, djf->djf_nforparms + 1);
    djf->djf_forparms[djf->djf_nforparms] = djp;
    djf->djf_nforparms += 1;
}
/***************************************************************/
static int djp_parse_narray_dims(struct djenv * dje,
                                        struct djps_stream * djps,
                                        int * narray_dims) {
/*
      11.      <class modifiers> ::= <class modifier> |
                <class modifiers> <class modifier>
*/
    int djerr;

    djerr = 0;
    (*narray_dims) = 0;

    while (!djerr && !strcmp(djps->djps_toke, "[")) {
        djerr = djp_get_toke(dje, djps);
        if (!djerr) {
            if (!strcmp(djps->djps_toke, "]")) {
                (*narray_dims) += 1;
                djerr = djp_get_toke(dje, djps);
            } else {
                djerr = dj_set_error(dje, DJERREXPCLOSEBRACKET,
                    "Expecting \"]\" to end array dimension. Found \"%s\"",
                    djps->djps_toke);
            }
        }
    }

    return (djerr);
}
/***************************************************************/
static int djp_parse_FormalParameters(struct djenv * dje,
                                        struct djps_stream * djps,
                                        struct djclassdef * djd,
                                        struct djformals * djf) {
/*
FormalParameters:           "(" FormalParameterDecls? ")"

FormalParameterDecls:       ("final" Annotations?
                                    Type FormalParameterDeclsRest)?

Annotations:                Annotation Annotations?
Annotation:                 "@" QualifiedIdentifier ("(" (Identifier "=")?
                                ElementValue ")")?

FormalParameterDeclsRest:   VariableDeclaratorId ("," FormalParameterDecls)?
FormalParameterDeclsRest:   "..." VariableDeclaratorId

VariableDeclaratorId:       Identifier ("[" "]")*

*/
    int djerr;
    int parm_mod;
    int done;
    int field_narray_dims;
    char * parm_name;
    struct djtype * djt;

    djerr       = 0;
    done        = 0;
    parm_mod    = 0;

    if (!strcmp(djps->djps_toke, ")")) {
        done = 1;
    } else {
        while (!djerr && !done) {
            field_narray_dims = 0;

            if (!strcmp(djps->djps_toke, "final")) {
                parm_mod |= DJC_PARM_MOD_FINAL;
                djerr = djp_get_toke(dje, djps);
            }

            if (!djerr) {
                if (!strcmp(djps->djps_toke, "@")) {
                    parm_mod |= DJC_PARM_MOD_FINAL;
                    djerr = djp_parse_Annotations(dje, djps, djd);
                }
            }

            if (!djerr) {
                djt = djp_new_djtype();
                djerr = djp_parse_type(dje, djps, djt);
            }

            /* check for ... */
            if (!strcmp(djps->djps_toke, ".")) {
                if (djp_char_index(djps, 1) == '.' &&
                    djp_char_index(djps, 2) == '.') {
                    djp_add_index(djps, 2);
                    djerr = djp_get_toke(dje, djps);
                    if (!djerr) {
                        if (! JAVA_FIRST_VAR_CHAR(djps->djps_toke[0])) {
                            parm_name = NULL;
                        } else {
                            parm_name = djps->djps_toke;
                            djerr = djp_check_variable_name(dje, parm_name);
                        }

                        if (!djerr) {
                            djerr = djp_parse_narray_dims(dje,
                                                    djps, &field_narray_dims);
                        }
                        if (!djerr) {
                            djp_add_variable_forparm(djf,
                                            parm_name, djt, field_narray_dims);
                            djerr = djp_get_toke(dje, djps);
                            done = 1;
                        }
                    }
                } else {
                    djerr = dj_set_error(dje, DJERREXPELIPSES,
        "Expecting \"...\" to indicate variable parameters. Found \"%s%c%c\"",
        djps->djps_toke, djp_char_index(djps, 1), djp_char_index(djps, 2));
                }
            } else {
                if (! JAVA_FIRST_VAR_CHAR(djps->djps_toke[0])) {
                    parm_name = NULL;
                } else {
                    parm_name = djps->djps_toke;
                    djerr = djp_check_variable_name(dje, parm_name);
                    if (!djerr) {
                        djerr = djp_get_toke(dje, djps);
                    }
                }

                if (!djerr) {
                    djerr = djp_parse_narray_dims(dje,
                                                    djps, &field_narray_dims);
                }
                if (!djerr) {
                    djp_add_forparm(djf, parm_name, djt, field_narray_dims);
                }
                if (!djerr) {
                    if (!strcmp(djps->djps_toke, ",")) {
                        djerr = djp_get_toke(dje, djps);
                    } else {
                        done = 1;
                    }
                }
            }
        }
    }

    if (!djerr) {
        if (!strcmp(djps->djps_toke, ")")) {
            djerr = djp_get_toke(dje, djps);
        } else {
            djerr = dj_set_error(dje, DJERREXPCLOSEPAREN,
                "Expecting \")\" to end formal parameters. Found \"%s\"",
                djps->djps_toke);
        }
    }

    return (djerr);

}
/***************************************************************/
static int djp_class_names_equal(   const char * pkg_name1,
                                    const char * class_name1,
                                    const char * pkg_name2,
                                    const char * class_name2) {
/*
*/
    int eq;

    eq = 0;
    if (!strcmp(pkg_name1  , pkg_name2  ) &&
        !strcmp(class_name1, class_name2) ) eq = 1;

    return (eq);
}
/***************************************************************/
static int djp_classes_equal(struct djclass * djc1, struct djclass * djc2) {

    int eq;

    eq = djp_class_names_equal(djc1->djc_parent->djk_pkg_name,
                               djc1->djc_class_name,
                               djc2->djc_parent->djk_pkg_name,
                               djc2->djc_class_name);

    return (eq);
}
/***************************************************************/
static int djp_types_equal(struct djtype * djt1, struct djtype * djt2) {

    int eq;
    int ii;

    if (!djt1) {
        if (!djt2) eq = 1;
        else       eq = 0;
    } else if (!djt2) {
        eq = 0;
    } else if (djt1->djt_typetype != djt2->djt_typetype) {
        eq = 0;
    } else {
        if (djt1->djt_typetype == DJT_TYPE_TYPED_CLASS) {
            eq = djp_classes_equal(djt1->djt_class, djt2->djt_class);
            if (eq) {
                if (djt1->djt_ntype_args != djt2->djt_ntype_args) eq = 0;
            }

            ii = 0;
            while (eq && ii < djt1->djt_ntype_args) {
                eq = djp_types_equal(djt1->djt_type_args[ii],
                                     djt2->djt_type_args[ii]);
                ii++;
            }
        } else if (djt1->djt_typetype == DJT_TYPE_CLASS) {
            eq = djp_classes_equal(djt1->djt_class, djt2->djt_class);
        } else {
            eq = 1;
        }
    }

    return (eq);
}
/***************************************************************/
static int djp_parms_equal(struct djforparm * djp1, struct djforparm * djp2) {

    int eq;

    if (djp1->djp_narray_dims != djp1->djp_narray_dims) {
        eq = 0;
    } else {
        eq = djp_types_equal(djp1->djp_parm_type, djp2->djp_parm_type);
    }

    return (eq);
}
/***************************************************************/
static int djp_formals_equal(struct djformals * djf1,
                             struct djformals * djf2) {

    int ix;
    int eq;

    if (djf1->djf_nforparms != djf2->djf_nforparms) {
        eq = 0;
    } else if (!djp_types_equal(djf1->djf_rtn_type, djf2->djf_rtn_type)) {
        eq = 0;
    } else {
        eq = 1;
        ix  = 0;
        while (eq && ix < djf1->djf_nforparms) {
            if (djp_parms_equal(djf1->djf_forparms[ix],
                                djf2->djf_forparms[ix])) {
                ix++;
            } else {
                eq = 0;
            }
        }
    }

    return (eq);
}
/***************************************************************/
static int djp_find_formals_index(struct djmember * djm,
                                       struct djformals * djf) {

    int ix;
    int found;

    found = 0;
    ix  = 0;
    while (!found && ix < djm->djm_nformals) {
        if (djp_formals_equal(djm->djm_formals[ix], djf)) {
            found = 1;
        } else {
            ix++;
        }
    }

    if (!found) ix = -1;

    return (ix);
}
/***************************************************************/
static struct djmember * djp_find_member_in_class(struct djclassdef * djd,
                                       const char * member_name,
                                       int mnlen) {

    struct djmember * djm;
    struct djmember ** vhand;

    djm = NULL;
    mnlen = IStrlen(member_name);

    if (djd->djd_members) {
        vhand = (struct djmember **)dbtree_find(djd->djd_members,
                            member_name, mnlen);
        if (vhand) djm = *vhand;
    }

    return (djm);
}
/***************************************************************/
static int djp_add_formals(struct djenv * dje,
                            struct djclassdef * djd,
                            const char * member_name,
                            struct djformals * djf) {
    int djerr;
    int mnlen;
    struct djmember * djm;

    djerr = 0;
    mnlen = IStrlen(member_name);
    djm = djp_find_member_in_class(djd, member_name, mnlen);
    if (djm) {
        if (djm->djm_field_method == DJI_TYPE_METHOD) {
            if (djp_find_formals_index(djm, djf) >= 0) {
                djerr = dj_set_error(dje, DJERRDUPOVERLOAD,
        "A method was found with a duplicate parameter list for method %s.%s",
        djd->djd_parent->djc_class_name, member_name);
            }
        } else {
            djerr = dj_set_error(dje, DJERRFIELDMETHOD,
                "A method may not have the same name as a field. Found \"%s\"",
                member_name);
        }
    } else {
        djm = djp_new_djmember(djd, member_name, DJI_TYPE_METHOD);
        if (!djd->djd_members) djd->djd_members = dbtree_new();
        dbtree_insert(djd->djd_members, member_name, mnlen, djm);
    }

    if (!djerr) {
        djf->djf_parent = djm;
        djm->djm_formals =
            Realloc(djm->djm_formals, struct djformals*, djm->djm_nformals + 1);
        djm->djm_formals[djm->djm_nformals] = djf;
        djm->djm_nformals += 1;
    }

    return (djerr);
}
/***************************************************************/
static int djp_parse_throws(struct djenv * dje,
                                        struct djps_stream * djps,
                                        struct djclassdef * djd,
                                        struct djformals * djf) {
    int djerr;
    int done;
    struct djclass * djc;
    char pkg_name[DEJA_FULL_NAME_MAX];
    char class_name[DEJA_CLASS_NAME_MAX];

    djerr = 0;
    done = 0;
    while (!djerr && !done) {
        djerr = djp_parse_Identifier(dje, djps,
                pkg_name, sizeof(pkg_name), class_name, sizeof(class_name));
        if (!djerr) {
            djc = djp_find_or_create_class(dje, pkg_name, class_name);

            djf->djf_throws =
                Realloc(djf->djf_throws, struct djclass*, djf->djf_nthrows + 1);
            djf->djf_throws[djf->djf_nthrows] = djc;
            djf->djf_nthrows += 1;

            if (!strcmp(djps->djps_toke, ",")) {
                djerr = djp_get_toke(dje, djps);
            } else {
                done = 1;
            }
        }
    }

    return (djerr);
}
/***************************************************************/
static int djp_parse_GenericMethodOrConstructorDecl(struct djenv * dje,
                                        struct djps_stream * djps,
                                        struct djclassdef * djd) {
/*
GenericMethodOrConstructorDecl:        TypeParameters
        GenericMethodOrConstructorRest

TypeParameters:                        "<" TypeParameter
        ("," TypeParameter)* ">"

GenericMethodOrConstructorRest:        (Type | "void") Identifier
         MethodDeclaratorRest

MethodDeclaratorRest:                  FormalParameters ("[" "]")*
        ("throws" QualifiedIdentifierList)? (MethodBody | ";")


*/
    int djerr;
    struct djformals * djf;
    struct djtype * djt;
    int member_narray_dims;
    char member_name[DEJA_MEMBER_NAME_MAX];

    member_narray_dims = 0;

    djt = djp_new_djtype();
    djf = djp_new_djformals(djt);

    djerr = djp_parse_type_parameters(dje, djps,
                &(djf->djf_ntype_parms), &(djf->djf_type_parms));

    if (!djerr) {
        djerr = djp_parse_type(dje, djps, djt);
    }
    if (!djerr) {
        strmcpy(member_name, djps->djps_toke, sizeof(member_name));
        djerr = djp_get_toke(dje, djps);
    }

    if (!djerr) {
        if (!strcmp(djps->djps_toke, "(")) {
            djerr = djp_get_toke(dje, djps);

            if (!djerr) {
                djerr = djp_parse_FormalParameters(dje, djps, djd, djf);
            }

            if (djerr) {
                djp_free_formals(djf);
            } else {
                djerr = djp_parse_narray_dims(dje, djps, &member_narray_dims);
                if (!djerr) {
                    djerr = djp_add_formals(dje, djd, member_name, djf);
                    if (!djerr) {
                        djf->djf_narray_dims = member_narray_dims;
                        if (!strcmp(djps->djps_toke, "throws")) {
                            djerr = djp_get_toke(dje, djps);
                            if (!djerr) djerr =
                                djp_parse_throws(dje, djps, djd, djf);
                        }
                    }
                }
            }
        } else {
            djerr = dj_set_error(dje, DJERREXPOPENPAREN,
                "Expecting \"(\" to indicate generic method. Found \"%s\"",
                djps->djps_toke);
        }
    }

    return (djerr);
}
/***************************************************************/
static int djp_parse_VoidMethodDeclaratorRest(struct djenv * dje,
                                        struct djps_stream * djps,
                                        struct djclassdef * djd) {
/*
*/
    int djerr;
    struct djformals * djf;
    struct djtype * djt;
    char method_name[DEJA_MEMBER_NAME_MAX];

    strmcpy(method_name, djps->djps_toke, sizeof(method_name));
    djerr = djp_check_variable_name(dje, method_name);

    if (!djerr) {
        djerr = djp_get_toke(dje, djps);
    }

    if (!djerr) {
        if (!strcmp(djps->djps_toke, "(")) {
            djerr = djp_get_toke(dje, djps);

            if (!djerr) {
                djt = djp_new_djtype();
                djt->djt_typetype    = DJT_TYPE_VOID;
                djf = djp_new_djformals(djt);

                djerr = djp_parse_FormalParameters(dje, djps, djd, djf);
                if (djerr) {
                    djp_free_formals(djf);
                } else {
                    djerr = djp_add_formals(dje, djd, method_name, djf);
                }
            }
        } else {
            djerr = dj_set_error(dje, DJERREXPOPENPAREN,
                "Expecting \"(\" to indicate void method. Found \"%s\"",
                djps->djps_toke);
        }
    }

    if (!djerr) {
        if (!strcmp(djps->djps_toke, "throws")) {
            djerr = djp_get_toke(dje, djps);
            if (!djerr) djerr = djp_parse_throws(dje, djps, djd, djf);
        }
    }

    return (djerr);
}
/***************************************************************/
static int djp_parse_ConstructorDeclaratorRest(struct djenv * dje,
                                        struct djps_stream * djps,
                                        struct djclassdef * djd) {
/*
ConstructorDeclaratorRest:  FormalParameters ("throws" QualifiedIdentifierList)?
                             MethodBody

FormalParameters:           "(" FormalParameterDecls? ")"

FormalParameterDecls:       ("final" Annotations?
                                Type FormalParameterDeclsRest)?

Annotations:                Annotation Annotations?
Annotation:                 "@" QualifiedIdentifier ("(" (Identifier "=")?
                            ElementValue ")")?

FormalParameterDeclsRest:   VariableDeclaratorId ("," FormalParameterDecls)?
FormalParameterDeclsRest:   "..." VariableDeclaratorId

VariableDeclaratorId:       Identifier ("[" "]")*


*/
    int djerr;
    struct djformals * djf;

    djerr = 0;
    djf = djp_new_djformals(NULL);
/*
    if (!strcmp(djps->djps_toke, "<")) {
        **
        ** I cannot find this case in the syntax, but
        ** com.google.gdata.data.BaseEntry uses it.
        **
        djerr = djp_get_toke(dje, djps);
        if (!djerr) {
            if (!strcmp(djps->djps_toke, "?")) {
                djerr = djp_get_toke(dje, djps);
            } else {
                djerr = dj_set_error(dje, DJERREXPQUESTION,
                                "Expecting \"?\" here. Found \"%s\"",
                                djps->djps_toke);
            }

            if (!strcmp(djps->djps_toke, ">")) {
                djerr = djp_get_toke(dje, djps);
            } else {
                djerr = dj_set_error(dje, DJERREXPCLOSETYPE,
        "Expecting \">\" to terminate parameterized type. Found \"%s\"",
        djps->djps_toke);
            }
        }
    }
*/
    if (!djerr) {
        if (!strcmp(djps->djps_toke, "(")) {
            djerr = djp_get_toke(dje, djps);
            if (!djerr) {
                djerr = djp_parse_FormalParameters(dje, djps, djd, djf);
                if (djerr) {
                    djp_free_formals(djf);
                } else {
                    djerr = djp_add_formals(dje, djd,
                                djd->djd_parent->djc_class_name, djf);
                }
            }
        } else {
            djerr = dj_set_error(dje, DJERREXPOPENPAREN,
                "Expecting \"(\" to indicate constuctor method. Found \"%s\"",
                djps->djps_toke);
        }
    }

    return (djerr);
}
/***************************************************************/
static int djp_define_field(struct djenv * dje,
                                        struct djclassdef * djd,
                                        const char * member_name,
                                        struct djtype * djt,
                                        int member_mod,
                                        int member_narray_dims) {
/*
                    djerr = djp_define_field(djp, djd,
                            member_name, djt, member_mod, member_narray_dims);
*/
    int djerr;
    int mnlen;
    struct djmember * djm;

    djerr = 0;
    mnlen = IStrlen(member_name);
    djm = djp_find_member_in_class(djd, member_name, mnlen);
    if (djm) {
        if (djm->djm_field_method == DJI_TYPE_FIELD) {
            djerr = dj_set_error(dje, DJERRDUPOVERLOAD,
                            "Field \"%s\" defined more than once.",
                            djd->djd_parent->djc_class_name, member_name);
        } else {
            djerr = dj_set_error(dje, DJERRFIELDMETHOD,
                "A method may not have the same name as a field. Found \"%s\"",
                member_name);
        }
    } else {
        djm = djp_new_djmember(djd, member_name, DJI_TYPE_FIELD);
        djm->djm_field_type = djt;

        if (!djd->djd_members) djd->djd_members = dbtree_new();
        dbtree_insert(djd->djd_members, member_name, mnlen, djm);
    }

    return (djerr);
}
/***************************************************************/
static int djp_parse_MethodOrFieldDecl(struct djenv * dje,
                                        struct djps_stream * djps,
                                        struct djclassdef * djd,
                                        int member_mod,
                                        const char * pkg_name,
                                        const char * class_name) {
/*
2. MemberDecl:        Type Identifier ("[" "]")* ("=" VariableInitializer)?
3. MemberDecl:        Type Identifier FormalParameters ("[" "]")*
    ("throws" QualifiedIdentifierList)? (MethodBody | ";")

MethodOrFieldDecl:    Type Identifier MethodOrFieldRest

FormalParameters:      "(" FormalParameterDecls? ")"

********
  public static com.google.gdata.data.BaseEntry<?>
        readEntry(com.google.gdata.data.ParseSource)
            throws  java.io.IOException,
                    com.google.gdata.util.ParseException,
                    com.google.gdata.util.ServiceException;


*/
    int djerr;
    struct djtype * djt;
    struct djformals * djf;
    int member_narray_dims;
    char member_name[DEJA_MEMBER_NAME_MAX];

    member_narray_dims = 0;

    djt = djp_new_djtype();

    djerr = djp_parse_type_info(dje, djps, djt, pkg_name, class_name);
    if (!djerr) {
        strmcpy(member_name, djps->djps_toke, sizeof(member_name));
        djerr = djp_get_toke(dje, djps);
    }

    if (!djerr) {
        if (!strcmp(djps->djps_toke, "[")) {
            djerr = djp_parse_narray_dims(dje, djps, &member_narray_dims);
            if (!djerr) {
                if (!strcmp(djps->djps_toke, ";")) {
                    djerr = djp_define_field(dje, djd,
                            member_name, djt, member_mod, member_narray_dims);
                } else if (!strcmp(djps->djps_toke, "=")) {
                    djerr = dj_set_error(dje, DJERRNOINIT,
                        "Member initialization not supported.  Member \"%s\"",
                        member_name);
                } else {
                    djerr = dj_set_error(dje, DJERREXPSEMICOLON,
                "Expecting \";\" to end member definition. Found \"%s\"",
                            djps->djps_toke);
                }
            }
        } else if (!strcmp(djps->djps_toke, "(")) {
            djf = djp_new_djformals(djt);
            djerr = djp_get_toke(dje, djps);
            if (!djerr) djerr = djp_parse_FormalParameters(dje,djps,djd,djf);
            if (djerr) {
                djp_free_formals(djf);
            } else {
                djerr = djp_add_formals(dje, djd, member_name, djf);
                if (!djerr) {
                    if (!strcmp(djps->djps_toke, "throws")) {
                        djerr = djp_get_toke(dje, djps);
                        if (!djerr) djerr = djp_parse_throws(dje,djps,djd,djf);
                    }
                }
            }
        } else {
            if (!strcmp(djps->djps_toke, ";")) {
                djerr = djp_define_field(dje, djd,
                        member_name, djt, member_mod, 0);
            } else if (!strcmp(djps->djps_toke, "=")) {
                djerr = dj_set_error(dje, DJERRNOINIT,
                    "Member initialization not supported.  Member \"%s\"",
                    member_name);
            } else {
                djerr = dj_set_error(dje, DJERREXPSEMICOLON,
                    "Expecting \";\" to end member definition. Found \"%s\"",
                    djps->djps_toke);
            }
        }
    }

    return (djerr);
}
/***************************************************************/
static int djp_parse_MemberDecl(struct djenv * dje,
                                        struct djps_stream * djps,
                                        struct djclassdef * djd) {
/*

      18.      <class body declaration>     ::=
                    <class member declaration> | <static initializer> |
                    <constructor declaration>
      19.      <class member declaration>   ::= <field declaration> |
                    <method declaration>
      20.      <static initializer>         ::= static <block>
      21.      <constructor declaration>    ::= <constructor modifiers>?
                    <constructor declarator>

      24.      <constructor declarator>     ::= <simple type name>
                    ( <formal parameter list>? )

      31.      <field declaration>          ::= <field modifiers>?
                    <type> <variable declarators> ;
      38.      <method declaration>         ::= <method header> <method body>
      39.      <method header>              ::= <method modifiers>?
                    <result type>
                    <method declarator> <throws>?


MemberDecl:        GenericMethodOrConstructorDecl
MemberDecl:        MethodOrFieldDecl
MemberDecl:        "void" Identifier VoidMethodDeclaratorRest
MemberDecl:        Identifier ConstructorDeclaratorRest
MemberDecl:        InterfaceDeclaration
MemberDecl:        ClassDeclaration


GenericMethodOrConstructorDecl:     TypeParameters
                                    GenericMethodOrConstructorRest
MethodOrFieldDecl:                  Type Identifier MethodOrFieldRest
VoidMethodDeclaratorRest:           FormalParameters
                        ("throws" QualifiedIdentifierList)? (MethodBody | ";")
ConstructorDeclaratorRest:          FormalParameters
                        ("throws" QualifiedIdentifierList)? MethodBody
InterfaceDeclaration:               NormalInterfaceDeclaration

MethodOrFieldRest:                  VariableDeclaratorRest
MethodOrFieldRest:                  MethodDeclaratorRest

VariableDeclaratorRest:             ("[" "]")* ("=" VariableInitializer)?


MethodDeclaratorRest:               FormalParameters ("[" "]")*
                ("throws" QualifiedIdentifierList)? (MethodBody | ";")


GenericMethodOrConstructorRest:     (Type | "void") Identifier
                        MethodDeclaratorRest

NormalInterfaceDeclaration:        "interface" Identifier TypeParameters?
                ("extends" TypeList)? InterfaceBody

-------------------------\

1. MemberDecl: "<" TypeParameter ("," TypeParameter)* ">" (Type | "void")
                    Identifier MethodDeclaratorRest
2. MemberDecl: Type Identifier ("[" "]")* ("=" VariableInitializer)?
3. MemberDecl: Type Identifier FormalParameters ("[" "]")*
            ("throws" QualifiedIdentifierList)? (MethodBody | ";")
4. MemberDecl: "void" Identifier FormalParameters
                    ("throws" QualifiedIdentifierList)? (MethodBody | ";")
5. MemberDecl: Identifier FormalParameters
            ("throws" QualifiedIdentifierList)? MethodBody
6. MemberDecl: "interface" Identifier TypeParameters?
                ("extends" TypeList)? InterfaceBody
7. MemberDecl: "class" Identifier TypeParameters?
        ("extends" Type)? ("implements" TypeList)? ClassBody

*/
    int djerr;
    int member_mod;
    char pkg_name[DEJA_FULL_NAME_MAX];
    char class_name[DEJA_CLASS_NAME_MAX];

    djerr = djp_parse_member_modifiers(dje, djps, &member_mod);
    if (!djerr) {
        if (!strcmp(djps->djps_toke, "<")) {            /* 1. MemberDecl */
            djerr = djp_get_toke(dje, djps);
            if (!djerr) {
                djerr = djp_parse_GenericMethodOrConstructorDecl(dje,djps,djd);
            }
        } else if (!strcmp(djps->djps_toke, "void")) {  /* 4. MemberDecl */
            djerr = djp_get_toke(dje, djps);
            if (!djerr) {
                djerr = djp_parse_VoidMethodDeclaratorRest(dje, djps, djd);
            }
        } else if (!strcmp(djps->djps_toke, "class")) { /* 7. MemberDecl */
            djerr = djp_get_toke(dje, djps);
            if (!djerr) {
                djerr = djp_parse_ClassDeclaration(dje, djps, djd);
            }
        } else if (!strcmp(djps->djps_toke, "interface")) { /* 6. MemberDecl */
            djerr = djp_get_toke(dje, djps);
            if (!djerr) {
                djerr = djp_parse_NormalInterfaceDeclaration(dje, djps, djd);
            }
        } else {
            djerr = djp_parse_Identifier(dje, djps,
                    pkg_name, sizeof(pkg_name), class_name, sizeof(class_name));
            if (!djerr) {
                if (djp_class_names_equal(
                      djd->djd_parent->djc_parent->djk_pkg_name,
                      djd->djd_parent->djc_class_name,
                      pkg_name,
                      class_name) && !strcmp(djps->djps_toke, "(")) {
                          /* 5. MemberDecl */
                    djerr = djp_parse_ConstructorDeclaratorRest(dje, djps,djd);
                } else {
                    /* 2. or 3.. MemberDecl */
                    djerr = djp_parse_MethodOrFieldDecl(dje,
                            djps, djd, member_mod, pkg_name, class_name);
                }
            }
        }
    }

    if (!djerr) {
        if (!strcmp(djps->djps_toke, ";")) {
            djerr = djp_get_toke(dje, djps);
        } else {
            djerr = dj_set_error(dje, DJERREXPSEMICOLON,
                "Expecting \";\" to end member definition. Found \"%s\"",
                djps->djps_toke);
        }
    }

    return (djerr);
}
/***************************************************************/
static int djp_parse_ClassBody(struct djenv * dje,
                                        struct djps_stream * djps,
                                        struct djclassdef * djd) {
/*
ClassBody:                  "{" ClassBodyDeclaration* "}"
ClassBodyDeclaration:       ";"
ClassBodyDeclaration:       "static"? Block
ClassBodyDeclaration:       Modifier* MemberDecl

MemberDecl:                 GenericMethodOrConstructorDecl
MemberDecl:                 MethodOrFieldDecl
MemberDecl:                 "void" Identifier VoidMethodDeclaratorRest
MemberDecl:                 Identifier ConstructorDeclaratorRest
MemberDecl:                 InterfaceDeclaration

*/
    int djerr;
    int done;
    char ch;

    djerr = 0;
    done = 0;
    while (!djerr && !done) {
        if (!strcmp(djps->djps_toke, ";")) {
            djerr = djp_get_toke(dje, djps);
        } else if (!strcmp(djps->djps_toke, "}")) {
            done = 1;
        } else if (!strcmp(djps->djps_toke, "static")) {
            djerr = djp_peek_char(dje, djps, &ch);
            if (!djerr && ch == '{') {
                djerr = djp_get_toke(dje, djps);    /* "{" */
                if (!djerr) djerr = djp_get_toke(dje, djps);/* should be "}" */
                if (!djerr) {
                    if (!strcmp(djps->djps_toke, "}")) {
                        djerr = djp_get_toke(dje, djps);
                    } else {
                        djerr = dj_set_error(dje, DJERREXPCLOSEBRACE,
                    "Expecting \"}\" to end static initializer. Found \"%s\"",
                    djps->djps_toke);
                    }
                }
            } else {
                djerr = djp_parse_MemberDecl(dje, djps, djd);
            }
        } else {
            djerr = djp_parse_MemberDecl(dje, djps, djd);
        }
    }

    return (djerr);
}
/***************************************************************/
static int djp_parse_class_declaration(struct djenv * dje,
                                        struct djps_stream * djps,
                                        char * compiled_from) {
/*
      10.      <class declaration> ::= <class modifiers> ?
                    class <identifier> <super>? <interfaces>? <class body>
*/
    int djerr;
    int class_mod;
    struct djpackage * djk;
    struct djclass * djc;
    struct djclassdef * djd;
    char pkg_name[DEJA_FULL_NAME_MAX];
    char class_name[DEJA_CLASS_NAME_MAX];

    djerr = djp_parse_class_modifiers(dje, djps, &class_mod);
    if (!djerr) {
        if (!strcmp(djps->djps_toke, "class")) {
            djerr = djp_get_toke(dje, djps);
            if (!djerr) {
                djerr = djp_parse_Identifier(dje, djps,
                        pkg_name, sizeof(pkg_name),
                        class_name, sizeof(class_name));
            }

            if (!djerr) {
                djk = djp_find_or_create_package(dje, pkg_name);

                djc = djp_find_class_in_package(djk, class_name);
                if (djc) {
                    if (djc->djc_classdef) {
                        djerr = dj_set_error(dje, DJERRDUPCLASS,
                            "Duplicate class %s.%s", pkg_name, class_name);
                    }
                } else {
                    djc = djp_new_djclass();
                    djc->djc_parent         = djk;
                    djc->djc_class_name     = Strdup(class_name);
                    djp_insert_class_into_package(djk, djc);
                }
            }
        } else {
            djerr = dj_set_error(dje, DJERREXPCLASS,
                "Expecting \"class\" to begin class declaration. Found \"%s\"",
                djps->djps_toke);
        }
    }

    if (!djerr) {
        djd = djp_new_djclassdef(djc);
        djc->djc_classdef = djd;
        djc->djc_compiled_from  = compiled_from;

        if (!strcmp(djps->djps_toke, "<")) {
            djerr = djp_get_toke(dje, djps);
            if (!djerr) {
                djerr = djp_parse_type_parameters(dje, djps,
                            &(djd->djd_ntype_parms), &(djd->djd_type_parms));

            }
        }
    }

    if (!djerr) {
        djd->djd_modifiers = class_mod;
        if (!strcmp(djps->djps_toke, "extends")) {
            djerr = djp_get_toke(dje, djps);
            if (!djerr) {
                djerr = djp_get_super(dje, djps, djd);
            }
        }
    }

    if (!djerr) {
        if (!strcmp(djps->djps_toke, "implements")) {
            djerr = djp_get_toke(dje, djps);
            if (!djerr) {
                djerr = djp_parse_type_list(dje, djps,
                            &(djd->djd_nimplements), &(djd->djd_implements));
            }
        }
    }

    if (!djerr) {
        if (!strcmp(djps->djps_toke, "{")) {
            djerr = djp_get_toke(dje, djps);
            if (!djerr) {
                djerr = djp_parse_ClassBody(dje, djps, djd);
            }
            if (!djerr) {
                if (!strcmp(djps->djps_toke, "}")) {
                    djerr = djp_get_toke(dje, djps);
                    if (djerr == DJKERR_EOF) {
                        djerr = 0;
                    } else {
                        djerr = dj_set_error(dje, DJERREXPEOF,
                "Expecting end-of-file to terminate class. Found \"%s\"",
                djps->djps_toke);
                    }
                } else {
                    djerr = dj_set_error(dje, DJERREXPCLOSEBODY,
                        "Expecting \"}\" to terminate class body. Found \"%s\"",
                        djps->djps_toke);
                }
            }
        } else if (!strcmp(djps->djps_toke, ";")) {
            /* ad hoc termination of class */
            djerr = djp_get_toke(dje, djps);
        } else {
            djerr = dj_set_error(dje, DJERREXPOPENBODY,
                            "Expecting \"{\" to begin class body. Found \"%s\"",
                            djps->djps_toke);
        }
    }

    return (djerr);
}
/***************************************************************/
static int djp_parse_class(struct djenv * dje,
                            struct djps_stream * djps) {

    int djerr;
    char * compiled_from;

    djerr = 0;
    compiled_from = NULL;

    if (!djerr && !strcmp(djps->djps_toke, "Compiled")) {
        djerr = djp_get_toke(dje, djps);
        if (!djerr && !strcmp(djps->djps_toke, "from")) {
            djerr = djp_get_toke(dje, djps);
            if (!djerr) {
                compiled_from = Strdup(djps->djps_toke);
                djerr = djp_get_toke(dje, djps);
            }
        }
    }

    djerr = djp_parse_class_declaration(dje, djps, compiled_from);

    return (djerr);
}
/***************************************************************/
static int djp_parse_prototype_file(struct djenv * dje,
                                    FILE * ptf,
                                    const char * ptfname) {

    int djerr;
    struct djpi_file   * djpif;
    struct djps_stream * djps;

    djpif = djp_new_djpi_file(ptf, ptfname);
    djps = djp_new_djps(djpf_read_next_line, djpif);

    djerr = djp_get_toke(dje, djps);

    if (!djerr) {
        djerr = djp_parse_class(dje, djps);
    }

    djp_free_djpi_file(djpif);
    djp_free_djps(djps);

    return (djerr);
}
/***************************************************************/
/* deja public calls                                           */
/***************************************************************/
/*@*/void get_precise_timestamp(char * datebuf, int datebufsize, int tsflags) {
/*
** Returns current date and time
**      tsflags
**          32 - include CC part of year
**          16 - include YY part of year
**           8 - include MM-DD date
**           4 - include HH:MM time
**           2 - include SS time
**           1 - include mmm time
**
*/
    time_t secs;
    int msecs;
    int dblen;
    int nn;
    struct tm * tp;

#if IS_WINDOWS
     struct _timeb tm;
     _ftime(&tm);
     secs = tm.time;
     msecs = tm.millitm;
#else
    time(&secs);
    msecs = 0;
#endif

    tp = localtime(&secs);

    dblen = 0;
    /* century */
    if (tsflags & 32) {
        nn = (tp->tm_year / 100) + 20;;
        if (dblen < datebufsize) datebuf[dblen++] = (nn / 10) +'0';
        if (dblen < datebufsize) datebuf[dblen++] = (nn % 10) +'0';
    }

    /* year */
    if (tsflags & 16) {
        nn = tp->tm_year % 100;
        if (dblen < datebufsize) datebuf[dblen++] = (nn / 10) +'0';
        if (dblen < datebufsize) datebuf[dblen++] = (nn % 10) +'0';
    }

    /* month-day */
    if (tsflags & 8) {
        if (tsflags & 16 && (dblen < datebufsize)) datebuf[dblen++] = '-';

        nn = tp->tm_mon + 1;
        if (dblen < datebufsize) datebuf[dblen++] = (nn / 10) +'0';
        if (dblen < datebufsize) datebuf[dblen++] = (nn % 10) +'0';

        if (dblen < datebufsize) datebuf[dblen++] = '-';
        nn = tp->tm_mday;
        if (dblen < datebufsize) datebuf[dblen++] = (nn / 10) +'0';
        if (dblen < datebufsize) datebuf[dblen++] = (nn % 10) +'0';
    }

    /* hour:minute */
    if (tsflags & 4) {
        if (tsflags & 8 && (dblen < datebufsize)) datebuf[dblen++] = ' ';

        nn = tp->tm_hour;
        if (dblen < datebufsize) datebuf[dblen++] = (nn / 10) +'0';
        if (dblen < datebufsize) datebuf[dblen++] = (nn % 10) +'0';

        if (dblen < datebufsize) datebuf[dblen++] = ':';
        nn = tp->tm_min;
        if (dblen < datebufsize) datebuf[dblen++] = (nn / 10) +'0';
        if (dblen < datebufsize) datebuf[dblen++] = (nn % 10) +'0';
    }

    /* second */
    if (tsflags & 2) {
        if (tsflags & 4 && (dblen < datebufsize)) datebuf[dblen++] = ':';

        nn = tp->tm_sec;
        if (dblen < datebufsize) datebuf[dblen++] = (nn / 10) +'0';
        if (dblen < datebufsize) datebuf[dblen++] = (nn % 10) +'0';
    }

    /* millisecond */
    if (tsflags & 1) {
        if (tsflags & 2 && (dblen < datebufsize)) datebuf[dblen++] = '.';

        nn = (msecs % 1000);
        if (dblen < datebufsize) datebuf[dblen++] = (nn / 100)       +'0';
        if (dblen < datebufsize) datebuf[dblen++] = ((nn / 10) % 10) +'0';
        if (dblen < datebufsize) datebuf[dblen++] = (nn % 10)        +'0';
    }

    if (dblen < datebufsize) datebuf[dblen] = '\0';
}
/***************************************************************/
int dejaTrace_v(FILE * trace_file, const char * fmt, va_list args)
{
    int trace_err;
    char * bbuf;
    char * logbuf;
    char datebuf[32];

    int os_get_process_memory_megabytes(void);

    trace_err = 0;

    if (trace_file) {
        get_precise_timestamp(datebuf, sizeof(datebuf) - 1, 7);

        bbuf = VSMPRINTF (fmt, args);

        logbuf = SMPRINTF("%s %3d %ld %s\n",
            datebuf, os_get_process_memory_megabytes(), os_getpid(), bbuf);
        if (fputs(logbuf, trace_file) < 0) {
            trace_err = errno;
            if (!trace_err) trace_err = -1;
        }
        Free(logbuf);
        Free(bbuf);
    }

    return (trace_err);
}
/***************************************************************/
int dejaTrace_f(FILE * trace_file, const char * fmt, ...)
{
    va_list args;
    int trace_err;

    va_start(args, fmt);
    trace_err = dejaTrace_v(trace_file, fmt, args);
    va_end(args);

    return (trace_err);
}
/***************************************************************/
int dejaTrace(struct djenv * dje, const char * fmt, ...)
{
    va_list args;
    int trace_err;

    va_start(args, fmt);
    trace_err = dejaTrace_v(dje->dje_trace_file, fmt, args);
    va_end(args);

    return (trace_err);
}
/***************************************************************/
int dejaShowError(struct djenv * dje, FILE * outf, int eref) {

    return dj_show_error(dje, outf, eref);
}
/***************************************************************/
char * dejaGetErrorMsg(struct djenv * dje, int eref) {
/*
** Message needs to be free'd
*/
    char * emsg;

    emsg = dj_get_error_msg(dje, eref);
    if (!emsg) return ("");

    dj_clear_error(dje, eref);

    return(emsg);
}
/***************************************************************/
struct djenv * dejaNewEnv(void)
{
    struct djenv * dje;

    dje = New(struct djenv, 1);
    if (!dje) return (NULL);

    dje->dje_je_env             = NULL;
    dje->dje_jvm                = NULL;
    dje->dje_jvmlib_name        = NULL;
    dje->dje_vm_options         = NULL;
    dje->dje_nvm_options        = 0;
    dje->dje_frame_capacity     = DEFAULT_FRAME_CAPACITY;
    dje->dje_trace_file         = NULL;
    dje->dje_jvm_version        = 0;
    dje->dje_nclasspath         = 0;
    dje->dje_classpath          = NULL;
    dje->dje_dJNI_CreateJavaVM  = NULL;
    dje->dje_nerrs              = 0;
    dje->dje_flags              = DJE_FLAGS_DEFAULT;
    dje->dje_mem_debug          = DEBUG_MEM_MARKERS;
    dje->dje_errs               = NULL;
    dje->dje_packages           = NULL;
    dje->dje_varnest            = NULL;
    dje->dje_ex                 = NULL;
    dje->dje_JNI_err_jstring    = NULL;
    dje->dje_JNI_err_chars      = NULL;
    dje->dje_JNI_error_msg      = NULL;

    if (dje->dje_mem_debug) {
#if DEBUG_MEM_MARKERS
        dje->dje_debug_frame_depth      = 0;
        memset(dje->dje_debug_frame_marker, 0, sizeof(dje->dje_debug_frame_marker));
#endif
    }

    dje->dje_heap_RuntimeClass      = NULL;
    dje->dje_heap_RuntimeObject     = NULL;
    dje->dje_heap_gcMethod          = NULL;
    dje->dje_heap_freeMemoryMethod  = NULL;
    dje->dje_heap_maxMemoryMethod   = NULL;
    dje->dje_heap_totalMemoryMethod = NULL;

    return (dje);
}
/***************************************************************/
static int dj_pop_varnest(struct djenv * dje)
{
    int djerr;
    struct djvarnest *      prev_varnest;

    djerr = 0;

    if (dje->dje_varnest) {
        prev_varnest = dje->dje_varnest->djn_prev_varnest;
        djp_free_djvarnest(dje->dje_varnest);
        dje->dje_varnest = prev_varnest;
    }

    if (dje->dje_flags & DJEFLAG_PUSH_LOCAL_FRAMES) {
        if (dje->dje_trace_file) {
            dejaTrace(dje, "call PopLocalFrame(NULL)");
        }

        (*(dje->dje_je_env))->PopLocalFrame(dje->dje_je_env, NULL);
#if DEBUG_MEM_MARKERS
        Free(dje->dje_debug_frame_marker[--dje->dje_debug_frame_depth]);
#endif
    }

    return (djerr);
}
/***************************************************************/
static int dj_push_varnest(struct djenv * dje)
{
    int djerr;
    int jnierr;
    struct djvarnest * new_varnest;

    djerr = 0;

    new_varnest = djp_new_djvarnest(dje->dje_varnest);

    dje->dje_varnest = new_varnest;

    if (dje->dje_flags & DJEFLAG_PUSH_LOCAL_FRAMES) {
        jnierr = (*(dje->dje_je_env))->PushLocalFrame(dje->dje_je_env,
                                                dje->dje_frame_capacity);
        if (jnierr) {
            if (dje->dje_trace_file) {
                dejaTrace(dje,
                    "FAIL PushLocalFrame(%d)", dje->dje_frame_capacity);
            }

            djerr = dj_set_error(dje, DJERRJNIPUSHFRAME,
                        "JNI call to PushLocalFrame(%d) failed with error %d",
                        dje->dje_frame_capacity, jnierr);
        } else {
            if (dje->dje_trace_file) {
                dejaTrace(dje,
                    "succ PushLocalFrame(%d)", dje->dje_frame_capacity);
            }
#if DEBUG_MEM_MARKERS
            dje->dje_debug_frame_marker[dje->dje_debug_frame_depth++] = Strdup("PushLocalFrame()");
#endif
        }
    }

    return (djerr);
}
/***************************************************************/
char * dj_get_JNI_message(struct djenv * dje);
void dj_release_JNI_message(struct djenv * dje);

static int dj_enable_heap_size(struct djenv * dje)
{
    int djerr;
    jmethodID getRuntimeMethod;

    djerr = 0;
    dje->dje_heap_RuntimeClass = (*(dje->dje_je_env))->FindClass(
                        dje->dje_je_env, "java/lang/Runtime");
    if (!dje->dje_heap_RuntimeClass) {
        if (dje->dje_trace_file) {
            dejaTrace(dje, "FAIL FindClass(%s) returned NULL",
                "java/lang/Runtime");
                dejaTrace(dje, "call ExceptionOccurred()");
        }

        dje->dje_ex = (*(dje->dje_je_env))->ExceptionOccurred(
                                                        dje->dje_je_env);
        if (!dje->dje_ex) {
            djerr = dj_set_error(dje, DJERRFINDCLASS,
                            "Unable to find class %s", "java/lang/Runtime");
        } else {
            djerr = dj_set_error(dje, DJERRFINDCLASS,
                                "Error finding class %s: %s",
                                "java/lang/Runtime", dj_get_JNI_message(dje));
            dj_release_JNI_message(dje);
        }
    } else {
        if (dje->dje_trace_file) {
            dejaTrace(dje, "rtn  FindClass(%s)=0x%08x",
                    "java/lang/Runtime", dje->dje_heap_RuntimeClass);
        }

        getRuntimeMethod = (*(dje->dje_je_env))->GetStaticMethodID(dje->dje_je_env,
                            dje->dje_heap_RuntimeClass, "getRuntime", "()Ljava/lang/Runtime;");
        if (!getRuntimeMethod) {
            if (dje->dje_trace_file) {
                dejaTrace(dje,
                    "FAIL GetStaticMethodID(0x%08x, %s, %s) returned NULL",
                    dje->dje_heap_RuntimeClass, "getRuntime", "()Ljava/lang/Runtime;");
            }

            dje->dje_JNI_error_msg =
                Strdup("JNI ERROR: Cannot find getRuntime()");
        } else {
            if (dje->dje_trace_file) {
                dejaTrace(dje, "rtn  GetStaticMethodID(0x%08x, %s, %s)=0x%08x",
                    dje->dje_heap_RuntimeClass, "getRuntime", "()Ljava/lang/String;",getRuntimeMethod);
            }

            dje->dje_heap_RuntimeObject = (*(dje->dje_je_env))->CallStaticObjectMethod(
                dje->dje_je_env, dje->dje_heap_RuntimeClass, getRuntimeMethod);
            if (!dje->dje_heap_RuntimeObject) {
                if (dje->dje_trace_file) {
                    dejaTrace(dje,
                        "FAIL CallStaticObjectMethod() returned NULL");
                }
                dje->dje_JNI_error_msg =
                    Strdup("JNI ERROR: Cannot call getRuntime()");
            } else {
                if (dje->dje_trace_file) {
                    dejaTrace(dje,
                        "rtn  CallStaticObjectMethod(0x%08x)=0x%08x",
                        dje->dje_heap_RuntimeClass, dje->dje_heap_RuntimeObject);
                }

                /* gc */
                dje->dje_heap_gcMethod = (*(dje->dje_je_env))->GetMethodID(dje->dje_je_env,
                                    dje->dje_heap_RuntimeClass, "gc", "()V");
                if (!dje->dje_heap_gcMethod) {
                    if (dje->dje_trace_file) {
                        dejaTrace(dje,
                            "FAIL GetMethodID(0x%08x, %s, %s) returned NULL",
                            dje->dje_heap_RuntimeClass, "gc", "()J");
                    }

                    dje->dje_JNI_error_msg =
                        Strdup("JNI ERROR: Cannot find Runtime.gc()");
                } else {
                    if (dje->dje_trace_file) {
                        dejaTrace(dje, "rtn  GetMethodID(0x%08x, %s, %s)=0x%08x",
                            dje->dje_heap_RuntimeClass, "gc", "()J",dje->dje_heap_gcMethod);
                    }
                }

                /* freeMemory */
                dje->dje_heap_freeMemoryMethod = (*(dje->dje_je_env))->GetMethodID(dje->dje_je_env,
                                    dje->dje_heap_RuntimeClass, "freeMemory", "()J");
                if (!dje->dje_heap_freeMemoryMethod) {
                    if (dje->dje_trace_file) {
                        dejaTrace(dje,
                            "FAIL GetMethodID(0x%08x, %s, %s) returned NULL",
                            dje->dje_heap_RuntimeClass, "freeMemory", "()J");
                    }

                    dje->dje_JNI_error_msg =
                        Strdup("JNI ERROR: Cannot find Runtime.freeMemory()");
                } else {
                    if (dje->dje_trace_file) {
                        dejaTrace(dje, "rtn  GetMethodID(0x%08x, %s, %s)=0x%08x",
                            dje->dje_heap_RuntimeClass, "freeMemory", "()J",dje->dje_heap_freeMemoryMethod);
                    }
                }

                /* maxMemory */
                dje->dje_heap_maxMemoryMethod = (*(dje->dje_je_env))->GetMethodID(dje->dje_je_env,
                                    dje->dje_heap_RuntimeClass, "maxMemory", "()J");
                if (!dje->dje_heap_maxMemoryMethod) {
                    if (dje->dje_trace_file) {
                        dejaTrace(dje,
                            "FAIL GetMethodID(0x%08x, %s, %s) returned NULL",
                            dje->dje_heap_RuntimeClass, "maxMemory", "()J");
                    }

                    dje->dje_JNI_error_msg =
                        Strdup("JNI ERROR: Cannot find Runtime.maxMemory()");
                } else {
                    if (dje->dje_trace_file) {
                        dejaTrace(dje, "rtn  GetMethodID(0x%08x, %s, %s)=0x%08x",
                            dje->dje_heap_RuntimeClass, "maxMemory", "()J",dje->dje_heap_maxMemoryMethod);
                    }
                }

                /* totalMemory */
                dje->dje_heap_totalMemoryMethod = (*(dje->dje_je_env))->GetMethodID(dje->dje_je_env,
                                    dje->dje_heap_RuntimeClass, "totalMemory", "()J");
                if (!dje->dje_heap_totalMemoryMethod) {
                    if (dje->dje_trace_file) {
                        dejaTrace(dje,
                            "FAIL GetMethodID(0x%08x, %s, %s) returned NULL",
                            dje->dje_heap_RuntimeClass, "totalMemory", "()J");
                    }

                    dje->dje_JNI_error_msg =
                        Strdup("JNI ERROR: Cannot find Runtime.totalMemory()");
                } else {
                    if (dje->dje_trace_file) {
                        dejaTrace(dje, "rtn  GetMethodID(0x%08x, %s, %s)=0x%08x",
                            dje->dje_heap_RuntimeClass, "totalMemory", "()J",dje->dje_heap_totalMemoryMethod);
                    }
                }
            }
        }
    }

    return (djerr);
}
/***************************************************************/
static int dj_garbage_collect(struct djenv * dje)
{
    int djerr;

    djerr = 0;
    if (dje->dje_heap_RuntimeObject && dje->dje_heap_gcMethod) {
        (*(dje->dje_je_env))->CallVoidMethod(
            dje->dje_je_env, dje->dje_heap_RuntimeObject, dje->dje_heap_gcMethod);

        dje->dje_ex = (*(dje->dje_je_env))->ExceptionOccurred(
                                                            dje->dje_je_env);
        if (dje->dje_trace_file) {
            dejaTrace(dje, "rtn  ExceptionOccurred()=0x%08x after %s()",
                dje->dje_ex, "Runtime.gc()");
        }
        if (dje->dje_ex) {
            djerr = dj_set_error(dje, DJERREXCEPTION,
                        "Exception occurred in method %s: %s",
                        "Runtime.gc()", dj_get_JNI_message(dje));
            dj_release_JNI_message(dje);
        } else {
            if (dje->dje_trace_file) {
                dejaTrace(dje, "rtn  %s()",
                    "Runtime.gc()");
            }
        }
    }

    return (djerr);
}
/***************************************************************/
#define DEJAHEAP_METHOD_MASK        15
#define DEJAHEAP_FLAG_RUN_GC        16
#define DEJAHEAP_FLAG_FREE_MEMORY   1
#define DEJAHEAP_FLAG_MAX_MEMORY    2
#define DEJAHEAP_FLAG_TOTAL_MEMORY  4

int dejaGetHeapSize(struct djenv * dje, int heap_flags, int64 * heap_size)
{
    int djerr;
    jlong hsz;
    jmethodID heapMethod;
    char method_name[32];

    (*heap_size) = 0;
    djerr = 0;
    method_name[0] = '\0';
    heapMethod = NULL;

    if (!dje->dje_heap_RuntimeObject && (dje->dje_flags & DJEFLAG_ENABLE_HEAP_SIZE)) {
        djerr = dj_enable_heap_size(dje);
    }

    if (!djerr) {
        switch (heap_flags & DEJAHEAP_METHOD_MASK) {
            case DEJAHEAP_FLAG_FREE_MEMORY  :
                heapMethod = dje->dje_heap_freeMemoryMethod;
                strcpy(method_name, "Runtime.freeMemory()");
                break;
            case DEJAHEAP_FLAG_MAX_MEMORY  :
                heapMethod = dje->dje_heap_maxMemoryMethod;
                strcpy(method_name, "Runtime.maxMemory()");
                break;
            case DEJAHEAP_FLAG_TOTAL_MEMORY  :
                heapMethod = dje->dje_heap_totalMemoryMethod;
                strcpy(method_name, "Runtime.totalMemory()");
                break;
            default  :
                break;
        }
    }

    if (!djerr && dje->dje_heap_RuntimeObject && dje->dje_heap_gcMethod && (heap_flags & DEJAHEAP_FLAG_RUN_GC)) {
        djerr = dj_garbage_collect(dje);
    }

    if (djerr || !dje->dje_heap_RuntimeObject || !heapMethod) {
        (*heap_size) = -1;
    } else {
        hsz = (*(dje->dje_je_env))->CallLongMethod(
            dje->dje_je_env, dje->dje_heap_RuntimeObject, heapMethod);

        dje->dje_ex = (*(dje->dje_je_env))->ExceptionOccurred(
                                                            dje->dje_je_env);
        if (dje->dje_trace_file) {
            dejaTrace(dje, "rtn  ExceptionOccurred()=0x%08x after %s()",
                dje->dje_ex, method_name);
        }
        if (dje->dje_ex) {
            djerr = dj_set_error(dje, DJERREXCEPTION,
                        "Exception occurred in method %s: %s",
                        method_name, dj_get_JNI_message(dje));
            dj_release_JNI_message(dje);
            (*heap_size) = -1;
        } else {
            if (dje->dje_trace_file) {
                dejaTrace(dje, "rtn  %s()=%Ld",
                    method_name, hsz);
            }
            (*heap_size) = hsz;
        }
    }

    return (djerr);
}
/***************************************************************/
void dejaDestroyGlobalJavaVM(void)
{
    if (global_jvm) (*global_jvm)->DestroyJavaVM(global_jvm);

    global_je_env       = NULL;
    global_jvm          = NULL;
    global_je_env_count = 0;
}
/***************************************************************/
void dejaFreeEnv(struct djenv * dje)
{
    int ii;

    while (dje->dje_varnest) {
        dj_pop_varnest(dje);
    }

    if (dje->dje_nclasspath) {
        for ( ii = 0; ii < dje->dje_nclasspath; ii++) {
            Free(dje->dje_classpath[ii]);
        }
        Free(dje->dje_classpath);
    }

    if (dje->dje_nvm_options) {
        for ( ii = 0; ii < dje->dje_nvm_options; ii++) {
            Free(dje->dje_vm_options[ii].optionString);
        }
        Free(dje->dje_vm_options);
    }

    if (dje->dje_jvmlib_name) Free(dje->dje_jvmlib_name);

    if (dje->dje_packages) {
        dbtree_free(dje->dje_packages, djp_free_vdjpackage);
    }

    if (dje->dje_jvm) {
        global_je_env_count -= 1;
        if (!global_je_env_count &&
                (dje->dje_flags & DJEFLAG_DESTROY_UNUSED_JVM)) {
            if (dje->dje_trace_file) {
                dejaTrace(dje, "call DestroyJavaVM()");
            }

            dejaDestroyGlobalJavaVM();
        }
    }
    dj_clear_all_errors(dje);

    if (dje->dje_trace_file) {
        dejaTrace(dje, "-------- End trace   --------");
        fclose(dje->dje_trace_file);
        dje->dje_trace_file = NULL;
    }

    Free(dje);
}
/***************************************************************/
#if 0
void dejaEnvAddClassPath(struct djenv * dje, const char * cp)
{
    int cplen;

    cplen = strlen(cp);

    dje->dje_classpath =
        Realloc(dje->dje_classpath, char*, dje->dje_nclasspath + 1);

    if (cplen && cp[cplen-1] == CLASS_PATH_DELIM) {
        dje->dje_classpath[dje->dje_nclasspath] = New(char, cplen + 1);
        memcpy(dje->dje_classpath[dje->dje_nclasspath], cp, cplen + 1);
    } else {
        dje->dje_classpath[dje->dje_nclasspath] = New(char, cplen + 2);
        memcpy(dje->dje_classpath[dje->dje_nclasspath], cp, cplen);
        dje->dje_classpath[dje->dje_nclasspath][cplen++] = CLASS_PATH_DELIM;
        dje->dje_classpath[dje->dje_nclasspath][cplen]   = '\0';
    }
    dje->dje_nclasspath += 1;
}
#endif
/***************************************************************/
void dejaEnvAddClassPath(struct djenv * dje, const char * cp)
{
    dje->dje_classpath =
        Realloc(dje->dje_classpath, char*, dje->dje_nclasspath + 1);
    dje->dje_classpath[dje->dje_nclasspath] = Strdup(cp);
    dje->dje_nclasspath += 1;
}
/***************************************************************/
void dejaEnvSetJVMRuntimeLib(struct djenv * dje, const char * rtlib)
{
    if (dje->dje_jvmlib_name) Free(dje->dje_jvmlib_name);

    if (rtlib) dje->dje_jvmlib_name = Strdup(rtlib);
    else       dje->dje_jvmlib_name = NULL;
}
/***************************************************************/
void dejaSetFlags(struct djenv * dje, int flag_val)
{
    dje->dje_flags |= flag_val;
}
/***************************************************************/
void dejaResetFlags(struct djenv * dje, int flag_val)
{
    dje->dje_flags &= ~flag_val;
}
/***************************************************************/
void dejaSetDebug(struct djenv * dje, int debug_val)
{
    if (debug_val) {
        dje->dje_flags |= DJEFLAG_DEBUG;

        dje->dje_vm_options =
    Realloc(dje->dje_vm_options, JavaVMOption, dje->dje_nvm_options + 8);
        memset(dje->dje_vm_options + dje->dje_nvm_options, 0,
                    sizeof(JavaVMOption) * 8);

        if (debug_val & DJEFLAG_DEBUG_OPTION_FLAGS) {   /* 02/24/2015 */
            dje->dje_vm_options[dje->dje_nvm_options].optionString =
                Strdup("-XX:+PrintFlagsFinal");
            dje->dje_nvm_options += 1;
        }

        if (debug_val & DJEFLAG_DEBUG_OPTION_JMXREMOTE) {
            dje->dje_vm_options[dje->dje_nvm_options].optionString =
                Strdup("-Dcom.sun.management.jmxremote");
            dje->dje_nvm_options += 1;
        }

        if (debug_val & DJEFLAG_DEBUG_OPTION_HEAP) {
            dje->dje_vm_options[dje->dje_nvm_options].optionString =
                Strdup("-agentlib:hprof=file=snapshot.hprof,format=b");
            dje->dje_nvm_options += 1;
            dje->dje_vm_options[dje->dje_nvm_options].optionString =
                Strdup("-XX:+HeapDumpOnOutOfMemoryError");
            dje->dje_nvm_options += 1;
        }

        if (debug_val & DJEFLAG_DEBUG_OPTION_VERBOSE_GC) {
            dje->dje_vm_options[dje->dje_nvm_options].optionString =
                Strdup("-verbose:gc");
            dje->dje_nvm_options += 1;
        }

        if (debug_val & DJEFLAG_DEBUG_OPTION_VERBOSE_JNI) {
            dje->dje_vm_options[dje->dje_nvm_options].optionString =
                Strdup("-verbose:jni");
            dje->dje_nvm_options += 1;
        }

        if (debug_val & DJEFLAG_DEBUG_OPTION_VERBOSE_CLASS) {
            dje->dje_vm_options[dje->dje_nvm_options].optionString =
                Strdup("-verbose:class");
            dje->dje_nvm_options += 1;
        }
    }
}
/***************************************************************/
void dejaSetMemory(struct djenv * dje,
                const char * minmem,
                const char * maxmem)
{
    dje->dje_vm_options =
        Realloc(dje->dje_vm_options, JavaVMOption, dje->dje_nvm_options + 2);

    if (maxmem && maxmem[0]) {
        if (isdigit(maxmem[strlen(maxmem)-1])) {
            dje->dje_vm_options[dje->dje_nvm_options].optionString =
                SMPRINTF("-Xmx%sm", maxmem);
        } else {
            dje->dje_vm_options[dje->dje_nvm_options].optionString =
                SMPRINTF("-Xmx%s", maxmem);
        }

        dje->dje_nvm_options += 1;
    }

    if (minmem && minmem[0]) {
        if (isdigit(minmem[strlen(minmem)-1])) {
            dje->dje_vm_options[dje->dje_nvm_options].optionString =
                SMPRINTF("-Xms%sm", minmem);
        } else {
            dje->dje_vm_options[dje->dje_nvm_options].optionString =
                SMPRINTF("-Xms%s", minmem);
        }

        dje->dje_nvm_options += 1;
    }
}
/***************************************************************/
int dejaSetTraceFile(struct djenv * dje, const char * trace_fname)
{
    int open_err;

    if (dje->dje_trace_file) fclose(dje->dje_trace_file);

    dje->dje_trace_file = fopen(trace_fname, "w");
    if (dje->dje_trace_file) {
        open_err = 0;
        dejaTrace(dje, "-------- Begin trace --------");
    } else {
        open_err = errno;
        if (!open_err) open_err = -1;
    }

    return (open_err);
}
/***************************************************************/
int dejaEnvStartJVM(struct djenv * dje)
{
/*
** JNI_CreateJavaVM() doc:
http://docs.oracle.com/javase/6/docs/technotes/guides/jni/spec/invocation.html
**
** To handle two or more calls to JNI_CreateJavaVM() should call
** JNI_GetCreatedJavaVMs(), then GetEnv() to get environment if
** JVM already exists.
*/
    int jstat;
    int djerr;
    char * jvmlib_name;
    JavaVMInitArgs vm_args;
    char jvmlib[256];

    djerr = 0;

    if (global_je_env) {
        dje->dje_je_env      = global_je_env;
        dje->dje_jvm         = global_jvm;
        global_je_env_count += 1;
    } else {
        jvmlib_name = dje->dje_jvmlib_name;
        if (!jvmlib_name) {
            if (!dj_find_jvmlib_name(jvmlib, sizeof(jvmlib))) {
                jvmlib_name = jvmlib;
            }
        }

        if (!djerr && (!jvmlib_name || !jvmlib_name[0])) {
            djerr = dj_set_error(dje, DJERRNOJVMFOUND,
                            "No Java runtime library available.");
        }

        if (!djerr) {
            djerr = dj_load_jvmlib(dje, jvmlib_name);
            if (!djerr) {
                memset(&vm_args, 0, sizeof(vm_args));
                vm_args.version = JNI_VERSION_1_2;

                if (dje->dje_nclasspath) {
                    dje->dje_vm_options =
        Realloc(dje->dje_vm_options, JavaVMOption, dje->dje_nvm_options + 1);
                    memset(dje->dje_vm_options + dje->dje_nvm_options, 0,
                                sizeof(JavaVMOption));

                    dj_set_classpath_option(dje,
                                 dje->dje_vm_options + dje->dje_nvm_options);
                    dje->dje_nvm_options += 1;
                }

                vm_args.nOptions = dje->dje_nvm_options;
                vm_args.options  = dje->dje_vm_options;

                jstat = dje->dje_dJNI_CreateJavaVM(&(dje->dje_jvm),
                                                (void**)&(dje->dje_je_env),
                                                &vm_args);
                if (jstat != JNI_OK) {
                    if (dje->dje_trace_file) {
                        dejaTrace(dje, "FAIL JNI_CreateJavaVM()=%d", jstat);
                    }
                    djerr = dj_set_error(dje, DJERRCREATEJVM,
                        "JNI_CreateJavaVM() failed with error %d", jstat);
                } else {
                    if (dje->dje_trace_file) {
                        int dii;
                        int jolen;
                        char * jopts;

                        jolen = 0;
                        for (dii = 0; dii < vm_args.nOptions; dii++) {
                            jolen +=
                                IStrlen(vm_args.options[dii].optionString) + 1;
                        }
                        if (!jolen) {
                            jopts = Strdup("");
                        } else {
                            jopts = New(char, jolen + 2);
                            jopts[0] = '\0';
                            for (dii = 0; dii < vm_args.nOptions; dii++) {
                                if (dii) strcat(jopts, "|");
                                strcat(jopts,
                                    vm_args.options[dii].optionString);
                            }
                        }

                        dejaTrace(dje, "rtn  JNI_CreateJavaVM(%s)", jopts);
                        Free(jopts);
                    }
                }
            }
            if (!djerr) {
                global_je_env       = dje->dje_je_env;
                global_jvm          = dje->dje_jvm;
                global_je_env_count = 1;
            }
        }
    }

    /* This is here to make certain the JVM is working */
    if (!djerr) {

        dje->dje_jvm_version =
            (*(dje->dje_je_env))->GetVersion(dje->dje_je_env);
        if (dje->dje_jvm_version < 0) {
            if (dje->dje_trace_file) {
                dejaTrace(dje, "FAIL GetVersion()=%d", dje->dje_jvm_version);
            }

            djerr = dj_set_error(dje, DJERRJNIVERSION,
                        "JNI call to GetVersion() failed with error %d",
                        dje->dje_jvm_version);
        } else {
            if (dje->dje_trace_file) {
                dejaTrace(dje, "rtn  GetVersion()=%d.%d",
                    (dje->dje_jvm_version) >> 16,
                    (dje->dje_jvm_version) & 0xFFFF);
            }
        }
    }

    if (!djerr) {
        djerr = dj_push_varnest(dje);
    }

    return (djerr);
}
/***************************************************************/
#if ALLOW_DEJA_IMPORT
int dejaImportClassPrototypeFile(struct djenv * dje, const char * ptfname)
{
    int djerr;
    int ern;
    FILE * ptf;

    djerr = 0;

    ptf = fopen(ptfname, "r");
    if (!ptf) {
        ern = errno;
        djerr = dj_set_error(dje, DJERROPENPTFILE,
                        "Error opening prototype file %s: %m", ptfname, ern);
        return (djerr);
    }

    djerr = djp_parse_prototype_file(dje, ptf, ptfname);

    fclose(ptf);

    return (djerr);
}
#endif
/***************************************************************/
static int dj_release_variable(struct djenv * dje, struct djvar * djv)
{
    int djerr;

    djerr = 0;

    if (djv->djv_jobject) {
        if (dje->dje_trace_file) {
            dejaTrace(dje, "call DeleteLocalRef(%s.%s)=0x%08x",
                djv->djv_var_type->djt_class->djc_class_name,
                djv->djv_var_name, djv->djv_jobject);
        }

        (*(dje->dje_je_env))->DeleteLocalRef(dje->dje_je_env,djv->djv_jobject);
        djv->djv_jobject = NULL;

        if (dje->dje_mem_debug) {
            Free(djv->djv_debug_marker);
            djv->djv_debug_marker = NULL;
        }
    }

    return (djerr);
}
/***************************************************************/
static void djp_parse_full_class(struct djenv * dje,
                               const char * full_class_name,
                               char * pkg_name,
                               int    pkg_name_sz,
                               char * class_name,
                               int    class_name_sz) {
/*
*/
    int fcnlen;
    int ix;
    int ilen;

    fcnlen = IStrlen(full_class_name);
    ix = fcnlen - 1;
    while (ix >= 0 && full_class_name[ix] != '.') ix--;

    if (ix < 0) {
        pkg_name[0] = '\0';
        ilen = fcnlen;
        if (ilen >= class_name_sz) ilen = class_name_sz - 1;
        memcpy(class_name, full_class_name, ilen + 1);
    } else {
        ilen = ix;
        if (ilen >= pkg_name_sz) ilen = class_name_sz - 1;
        memcpy(pkg_name, full_class_name, ilen);
        pkg_name[ilen] = '\0';

        ilen = fcnlen - ix - 1;
        if (ilen >= class_name_sz) ilen = class_name_sz - 1;
        memcpy(class_name, full_class_name + ix + 1, ilen + 1);
    }
}
/***************************************************************/
static void djp_make_full_class(struct djenv * dje,
                               char * full_class_name,
                               int    full_class_name_sz,
                               const char * pkg_name,
                               const char * class_name,
                               int slash_delimiters) {
/*
** See also djp_make_single_Identifier()
*/
    int pnlen;
    int cnlen;
    char * dot;

    pnlen = IStrlen(pkg_name);
    cnlen = IStrlen(class_name);

    if (pnlen) {
        if (pnlen + 2 >= full_class_name_sz)
            pnlen = full_class_name_sz - 3;

        if (pnlen + cnlen + 2 >= full_class_name_sz)
            cnlen = full_class_name_sz - (pnlen + 2);

        memcpy(full_class_name, pkg_name, pnlen);

        if (slash_delimiters) {
            full_class_name[pnlen]     = '/';
            full_class_name[pnlen + 1] = '\0';

            dot = strchr(full_class_name, '.');
            while (dot) {
                (*dot) = '/';
                dot = strchr(dot + 1, '.');
            }
        } else {
            full_class_name[pnlen] = '.';
        }

        memcpy(full_class_name + pnlen + 1, class_name, cnlen + 1);
    } else {
        if (cnlen + 1 >= full_class_name_sz)
            cnlen = full_class_name_sz - 1;
        memcpy(full_class_name, class_name, cnlen + 1);
    }

}
/***************************************************************/
#ifdef UNUSED
static struct djclass * djp_find_full_class(struct djenv * dje,
                                const char * full_class_name) {

    struct djpackage * djk;
    struct djclass * djc;
    struct djclass ** vhdjc;
    char pkg_name[DEJA_FULL_NAME_MAX];
    char class_name[DEJA_CLASS_NAME_MAX];

    djerr = 0;
    djc = NULL;

    djp_parse_full_class(dje, full_class_name,
            pkg_name, sizeof(pkg_name), class_name, sizeof(class_name));

    djk = djp_find_package(dje, pkg_name);
    if (djk && djk->djk_classes) {
        vhdjc = (struct djclass **)dbtree_find(djk->djk_classes,
                                    class_name, strlen(class_name));
        if (vhdjc) djc = *vhdjc;
    }

    return (djc);
}
#endif
/***************************************************************/
static int djp_classs_equal(struct djclass * djc1,

                               const char * pkg_name1,
                               const char * class_name1,
                               const char * pkg_name2,
                               const char * class_name2) {
    int matches;

    matches = 0;
    if (!strcmp(pkg_name1  , pkg_name2) &&
        !strcmp(class_name1, class_name2)) {
        matches = 1;
    }

    return (matches);
}
/***************************************************************/
static int dj_match_class_or_subclass(struct djenv * dje,
                            struct djclass * parent_djc,
                            struct djclass * sub_djc)

{
    int djerr;
    int matches;
    struct djclass * djc;
    struct djtype *  djt;
    int is_assignable;

    djerr = 0;
    matches = 0;

    djc = sub_djc;

    if (!parent_djc || !djc) {
        matches = 0;
    } else if (djp_classes_equal(parent_djc, djc)) {
        matches = 1;
    } else {
        while (!matches && djc) {
            if (!djc->djc_classdef) {
                djc = NULL;
            } else {
                djt = djc->djc_classdef->djd_superclass;
                if (!djt) {
                    djc = NULL;
                } else {
                    djc = djt->djt_class;
                    if (djc) {
                        if (djp_classes_equal(parent_djc, djc)) {
                            matches = 1;
                        }
                    }
                }
            }
        }
    }

    if (!matches && parent_djc->djc_jclass && sub_djc->djc_jclass) {
       is_assignable = (*(dje->dje_je_env))->IsAssignableFrom(
                        dje->dje_je_env,
                        sub_djc->djc_jclass,
                        parent_djc->djc_jclass);
        if (is_assignable) matches = 1;

        if (dje->dje_trace_file) {
            dejaTrace(dje, "rtn  IsAssignableFrom(%s.%s, %s.%s)=%d\n",
                sub_djc->djc_parent->djk_pkg_name,
                sub_djc->djc_class_name,
                parent_djc->djc_parent->djk_pkg_name,
                parent_djc->djc_class_name,
                (int)is_assignable);
        }
    }

    if (!matches) {
        djerr = dj_set_error(dje, DJERRVARCLASSMISMATCH,
            "Object of class %s.%s cannot be redefined as class %s.%s",
            parent_djc->djc_parent->djk_pkg_name, parent_djc->djc_class_name,
            sub_djc->djc_parent->djk_pkg_name   , sub_djc->djc_class_name);
    }

    return (djerr);
}
/***************************************************************/
static struct djvar * dj_find_variable(struct djenv * dje,
                                        const char * var_name)
{
    struct djvar * djv;
    struct djvar ** vdjv;

    djv = NULL;
    if (dje->dje_varnest) {
        vdjv = (struct djvar **)dbtree_find(dje->dje_varnest->djn_vars,
                    var_name, IStrlen(var_name));
        if (vdjv) djv = *vdjv;
    }

    return (djv);
}
/***************************************************************/
char * dj_get_JNI_message(struct djenv * dje) {
/*
** Assumes         dje->dje_ex = (*(dje->dje_je_env))->ExceptionOccurred(
**                                          dje->dje_je_env);
**
** Sets dje->dje_JNI_error_msg
*/
    jmethodID toString;
    jclass objClass;
    jboolean isCopy;
    const char * err_chars;

    if (dje->dje_JNI_error_msg) {
        Free(dje->dje_JNI_error_msg);
        dje->dje_JNI_err_chars = NULL;
        dje->dje_JNI_error_msg = NULL;
    }

    if (!dje->dje_ex) {
        dje->dje_JNI_error_msg =
            Strdup("JNI ERROR: No exception.");
    } else {
        objClass = (*(dje->dje_je_env))->FindClass(
                        dje->dje_je_env, "java/lang/Object");
        if (!objClass) {
            if (dje->dje_trace_file) {
                dejaTrace(dje, "FAIL FindClass(%s) returned NULL",
                    "java/lang/Object");
            }

            dje->dje_JNI_error_msg =
                Strdup("JNI ERROR: Cannot find class java/lang/Object");
        } else {
            if (dje->dje_trace_file) {
                dejaTrace(dje, "rtn  FindClass(%s)=0x%08x",
                        "java/lang/Object", objClass);
            }

            toString = (*(dje->dje_je_env))->GetMethodID(dje->dje_je_env,
                            objClass, "toString", "()Ljava/lang/String;");

            if (!toString) {
                if (dje->dje_trace_file) {
                    dejaTrace(dje,
                        "FAIL GetMethodID(0x%08x, %s, %s) returned NULL",
                        objClass, "toString", "()Ljava/lang/String;");
                }

                dje->dje_JNI_error_msg =
                    Strdup("JNI ERROR: Cannot find toString()");
            } else {
                if (dje->dje_trace_file) {
                    dejaTrace(dje, "rtn  GetMethodID(0x%08x, %s, %s)=0x%08x",
                        objClass, "toString", "()Ljava/lang/String;",toString);
                }

                dje->dje_JNI_err_jstring =
                    (jstring) (*(dje->dje_je_env))->CallObjectMethod(
                    dje->dje_je_env, dje->dje_ex, toString);
                if (!dje->dje_JNI_err_jstring) {
                    if (dje->dje_trace_file) {
                        dejaTrace(dje,
                            "FAIL CallObjectMethod() returned NULL");
                    }
                    dje->dje_JNI_error_msg =
                        Strdup("JNI ERROR: Null error message");
                } else {
                    if (dje->dje_trace_file) {
                        dejaTrace(dje,
                            "rtn  CallObjectMethod(0x%08x, 0x%08x)=0x%08x",
                            dje->dje_ex, toString, dje->dje_JNI_err_jstring);
                    }
                    if (dje->dje_mem_debug) {
                        dje->dje_debug_JNI_err_jstring = Strdup("New dje_JNI_err_jstring");
                    }
                    err_chars = (*(dje->dje_je_env))->GetStringUTFChars(
                        dje->dje_je_env, dje->dje_JNI_err_jstring, &isCopy);
                    if (!err_chars) {
                        if (dje->dje_trace_file) {
                            dejaTrace(dje,
                                "FAIL GetStringUTFChars(0x%08x) returned NULL",
                                dje->dje_JNI_err_jstring);
                        }
                        dje->dje_JNI_error_msg = Strdup(
                            "JNI ERROR: Null GetStringUTFChars() message");
                    } else {
                        if (dje->dje_trace_file) {
                            dejaTrace(dje,
                                "rtn  GetStringUTFChars(0x%08x)=\"%s\"",
                                dje->dje_JNI_err_jstring, err_chars);

                        }
                        dje->dje_JNI_error_msg = Strdup(err_chars);
                    }
                }
            }
        }
    }

    return (dje->dje_JNI_error_msg);
}
/***************************************************************/
void dj_release_JNI_message(struct djenv * dje) {

    if (dje->dje_ex) {
        if (dje->dje_trace_file) {
            dejaTrace(dje, "call ExceptionClear()");
        }

        (*(dje->dje_je_env))->ExceptionClear(dje->dje_je_env);
        dje->dje_ex = NULL;
    }

    if (dje->dje_JNI_error_msg) {
        if (dje->dje_trace_file) {
            dejaTrace(dje, "call ReleaseStringUTFChars(0x%08x)",
                dje->dje_JNI_err_jstring);
        }

        (*(dje->dje_je_env))->ReleaseStringUTFChars(dje->dje_je_env,
                        dje->dje_JNI_err_jstring, dje->dje_JNI_err_chars);
        Free(dje->dje_JNI_error_msg);
        dje->dje_JNI_err_chars = NULL;
        dje->dje_JNI_error_msg = NULL;
        if (dje->dje_mem_debug) {
            Free(dje->dje_debug_JNI_err_jstring);
            dje->dje_debug_JNI_err_jstring = NULL;
        }
    }
}
/***************************************************************/
static int dj_find_and_set_java_class(struct djenv * dje,
                           struct djclass * djc)
{
/*
jthrowable g_jex;
#define JAVA_ERROR(jenv) (g_jex = (*jenv)->ExceptionOccurred(jenv))
*/
    int djerr;
    char full_class_name[DEJA_FULL_NAME_MAX];

    djerr = 0;

    if (!djc->djc_jclass) {
        djp_make_full_class(dje, full_class_name, sizeof(full_class_name),
            djc->djc_parent->djk_pkg_name, djc->djc_class_name, 1);

        djc->djc_jclass = (*(dje->dje_je_env))->FindClass(dje->dje_je_env,
                                full_class_name);
        if (!djc->djc_jclass) {
            if (dje->dje_trace_file) {
                dejaTrace(dje, "FAIL FindClass(%s) returned NULL",
                    full_class_name);
                dejaTrace(dje, "call ExceptionOccurred()");
            }

            dje->dje_ex = (*(dje->dje_je_env))->ExceptionOccurred(
                                                            dje->dje_je_env);
            if (!dje->dje_ex) {
                djerr = dj_set_error(dje, DJERRFINDCLASS,
                                "Unable to find class %s", full_class_name);
            } else {
                djerr = dj_set_error(dje, DJERRFINDCLASS,
                                    "Error finding class %s: %s",
                                    full_class_name, dj_get_JNI_message(dje));
                dj_release_JNI_message(dje);
            }
        } else {
            if (dje->dje_trace_file) {
                dejaTrace(dje, "rtn  FindClass(%s)=0x%08x",
                        full_class_name, djc->djc_jclass);
            }
        }
    }

    return (djerr);
}
/***************************************************************/
static struct djtype * djv_new_class_djtype(struct djclass * djc)
{
    struct djtype * djt;

    djt = djp_new_djtype();
    djt->djt_typetype    = DJT_TYPE_CLASS;
    djt->djt_class = djc;

    return (djt);
}
/***************************************************************/
static int dj_new_variable(struct djenv * dje,
                                        const char * var_name,
                                        struct djclass * djc,
                                        struct djvar ** pdjv)
{
    int djerr;
    struct djvar * djv;
    struct djtype * djt;

    djerr = 0;
    (*pdjv) = NULL;

    if (!dje->dje_varnest) {
        djerr = dj_set_error(dje, DJERRNOVARCONTEXT,
                            "No context for variable %s", var_name);
    } else {
        djt = djv_new_class_djtype(djc);

        djv = djp_new_djvar(var_name, djt);

        dbtree_insert(dje->dje_varnest->djn_vars,
                        var_name, IStrlen(var_name), djv);

        (*pdjv) = djv;
    }

    return (djerr);
}
/***************************************************************/
void append_cstr(char ** cstr, int * slen, int * smax, const char * val) {

    int vallen;

    vallen = IStrlen(val);
    if ((*slen) + vallen >= (*smax)) {
        if (!(*smax)) (*smax) = 16 + vallen;
        else         (*smax)  = ((*smax) * 2) + vallen;
        (*cstr) = Realloc(*cstr, char, (*smax));
    }
    memcpy((*cstr) + (*slen), val, vallen + 1);
    (*slen) += vallen;
}
/***************************************************************/
static int djv_get_type_from_str(struct djenv * dje,
                        const char * type_str,
                        struct djtype ** pdjt) {
/*
*/
    int djerr;
    int styp;
    struct djclass * djc;
    struct djtype * djt;
    char pkg_name[DEJA_FULL_NAME_MAX];
    char class_name[DEJA_CLASS_NAME_MAX];


    djerr = 0;
    (*pdjt) = NULL;
    djt = djp_new_djtype();

    djp_parse_full_class(dje, type_str,
            pkg_name, sizeof(pkg_name), class_name, sizeof(class_name));

    styp = djp_get_simple_type(type_str);
    if (styp) {
        if (pkg_name[0]) {
            djerr = dj_set_error(dje, DJERRNOPACKAGE,
                "Package name not permitted for simple type. Found \"%s.%s\"",
                pkg_name, class_name);
        } else {
            djt->djt_typetype    = styp;
            (*pdjt) = djt;
        }
    } else {
        djc = djp_find_or_create_class(dje, pkg_name, class_name);
        djt->djt_typetype    = DJT_TYPE_CLASS;
        djt->djt_class = djc;
        (*pdjt) = djt;
    }

    return (djerr);
}
/***************************************************************/
static int append_jni_sig(struct djenv * dje,
                    struct djtype * djt,
                    char ** jni_sig,
                    int * jnislen,
                    int * jnismax) {

    int djerr;
    char full_class_name[DEJA_FULL_NAME_MAX];

    djerr           = 0;

    if (djt) {
        switch (djt->djt_typetype) {
            case DJT_TYPE_VOID:
                append_cstr(jni_sig, jnislen, jnismax, "V");
                break;
            case DJT_TYPE_BOOLEAN:
                append_cstr(jni_sig, jnislen, jnismax, "Z");
                break;
            case DJT_TYPE_BYTE:
                append_cstr(jni_sig, jnislen, jnismax, "B");
                break;
            case DJT_TYPE_SHORT:
                append_cstr(jni_sig, jnislen, jnismax, "S");
                break;
            case DJT_TYPE_INT:
                append_cstr(jni_sig, jnislen, jnismax, "I");
                break;
            case DJT_TYPE_LONG:
                append_cstr(jni_sig, jnislen, jnismax, "L");
                break;
            case DJT_TYPE_CHAR:
                append_cstr(jni_sig, jnislen, jnismax, "C");
                break;
            case DJT_TYPE_FLOAT:
                append_cstr(jni_sig, jnislen, jnismax, "F");
                break;
            case DJT_TYPE_DOUBLE:
                append_cstr(jni_sig, jnislen, jnismax, "D");
                break;
            case DJT_TYPE_CLASS:
            case DJT_TYPE_TYPED_CLASS:
                append_cstr(jni_sig, jnislen, jnismax, "L");
                djp_make_full_class(dje, full_class_name,
                    sizeof(full_class_name),
                    djt->djt_class->djc_parent->djk_pkg_name,
                    djt->djt_class->djc_class_name, 1);
                append_cstr(jni_sig, jnislen, jnismax, full_class_name);
                append_cstr(jni_sig, jnislen, jnismax, ";");
                break;
            default:
                append_cstr(jni_sig, jnislen, jnismax, "?");
                break;
        }
    }

    return (djerr);
}
/***************************************************************/
static struct djtype * djv_dup_type(struct djenv * dje, struct djtype * djt)
{
/*
*/

    struct djtype * new_djt;

    new_djt = djp_new_djtype();
/*
*/
    if (!new_djt) return (NULL);

    new_djt->djt_typetype    = djt->djt_typetype;
    new_djt->djt_class       = djt->djt_class;
    new_djt->djt_ntype_args  = 0;       /* type_args not supported here */
    new_djt->djt_type_args   = NULL;

    return (new_djt);
}
/***************************************************************/
static int dj_get_class(struct djenv * dje,
                        const char * full_class_name,
                        struct djclass ** pdjc)
{
    int djerr;
    struct djclass * djc;
    char pkg_name[DEJA_FULL_NAME_MAX];
    char class_name[DEJA_CLASS_NAME_MAX];

    djerr   = 0;
    (*pdjc) = NULL;

    djp_parse_full_class(dje, full_class_name,
            pkg_name, sizeof(pkg_name), class_name, sizeof(class_name));

    djc = djp_find_or_create_class(dje, pkg_name, class_name);

    djerr = dj_find_and_set_java_class(dje, djc);

    if (!djerr) (*pdjc) = djc;

    return (djerr);
}
/***************************************************************/
static int djv_find_cast(struct djenv * dje,
                                    struct djclass * djc,
                                    const char * full_class_name)
{
    int found;
    int ix;

    found   = 0;
    ix      = 0;

    while (!found && ix < djc->djc_ncasts) {
        if (!strcmp(djc->djc_casts[ix], full_class_name)) found = 1;
        else ix++;
    }

    return (found);
}
/***************************************************************/
static void djv_add_cast(struct djenv * dje,
                                    struct djclass * djc,
                                    const char * full_class_name)
{
    djc->djc_casts = Realloc(djc->djc_casts, char*, djc->djc_ncasts + 1);
    djc->djc_casts[djc->djc_ncasts] = Strdup(full_class_name);
    djc->djc_ncasts += 1;
}
/***************************************************************/
static int djv_get_casted_variable(struct djenv * dje,
                                    const char * objName,
                                    const char * methodName,
                                    struct djvar ** cast_djv,
                                    struct djtype ** cast_djt)
{
    int djerr;
    int ix;
    int cnl;
    int is_assignable;
    struct djclass * djc;
    char full_class_name[DEJA_FULL_NAME_MAX + 2];

    djerr       = 0;
    (*cast_djv) = NULL;
    (*cast_djt) = NULL;
    cnl         = 0;

    ix = 1;
    while (objName[ix] && objName[ix] != ')' && cnl < DEJA_FULL_NAME_MAX) {
        full_class_name[cnl++] = objName[ix++];
    }
    full_class_name[cnl] = '\0';

    if (objName[ix] != ')') {
        djerr = dj_set_error(dje, DJERREXPCLOSECAST,
                        "Expecting \")\" to end cast.");
    } else {
        (*cast_djv) = dj_find_variable(dje, objName + ix + 1);
        if (!(*cast_djv)) {
            djerr = dj_set_error(dje, DJERRNOSUCHVAR,
                "Destination object \"%s\" for method %s has not been defined",
                                objName + ix + 1, methodName);
        } else if (!((*cast_djv)->djv_var_type->djt_class->djc_jclass)) {
            djerr = dj_set_error(dje, DJERRNOCLASS,
                        "Destination object \"%s\" for method %s has no class",
                                        objName + ix + 1, methodName);
        } else {
            djerr = dj_get_class(dje, full_class_name, &djc);

            (*cast_djt) = djv_new_class_djtype(djc);
        }
    }

    if (!djerr) {
        if (!djv_find_cast(dje,
                          (*cast_djv)->djv_var_type->djt_class,
                          full_class_name)) {
            is_assignable = (*(dje->dje_je_env))->IsAssignableFrom(
                            dje->dje_je_env,
                            (*cast_djv)->djv_var_type->djt_class->djc_jclass,
                            djc->djc_jclass);

            if (dje->dje_trace_file) {
                dejaTrace(dje, "rtn  IsAssignableFrom(%s, %s.%s)=%d",
                    full_class_name,
            (*cast_djv)->djv_var_type->djt_class->djc_parent->djk_pkg_name,
                    (*cast_djv)->djv_var_type->djt_class->djc_class_name,
                    (int)is_assignable);
            }

            if (!is_assignable) {
                djerr = dj_set_error(dje, DJERRNOCAST,
    "Cannot cast to class %s from class %s.%s",
        full_class_name,
        (*cast_djv)->djv_var_type->djt_class->djc_parent->djk_pkg_name,
        (*cast_djv)->djv_var_type->djt_class->djc_class_name);
            } else {
                djv_add_cast(dje,
                             (*cast_djv)->djv_var_type->djt_class,
                             full_class_name);
            }
        }
    }

    if (djerr && (*cast_djt)) djp_free_djtype(*cast_djt);

    return (djerr);
}
/***************************************************************/
static int djv_get_variable_and_type(struct djenv * dje,
                        const char * full_var_name,
                        const char * methodName,
                        struct djvar *  src_djv,
                        struct djvar  ** pdjv,
                        struct djtype ** pdjt)
{
    int djerr;
    struct djvar *  djv;
    struct djtype * djt;

    djerr = 0;

    (*pdjv) = NULL;
    (*pdjt) = NULL;

    if (full_var_name[0] == '(') {
        djerr = djv_get_casted_variable(dje, full_var_name, methodName,
                            &djv, &djt);
    } else {
        djv = dj_find_variable(dje, full_var_name);
        if (!djv) {
            djerr = dj_set_error(dje, DJERRNOSUCHVAR,
                        "Object \"%s\" for method %s has not been defined",
                                            full_var_name, methodName);
        } else {
            if (src_djv) {
                djerr = dj_match_class_or_subclass(dje,
                    djv->djv_var_type->djt_class,
                    src_djv->djv_var_type->djt_class);
                if (!djerr) {
                    djerr = dj_release_variable(dje, djv);
                }
            }
            if (!djerr) {
                djt = djv_dup_type(dje, djv->djv_var_type);
            }
        }
    }

    if (!djerr) {
        (*pdjv) = djv;
        (*pdjt) = djt;
    }

    return (djerr);
}
/***************************************************************/
static int djSignature_to_djmethrec(struct djenv * dje,
                    struct djmethrec * djr,
                    struct djclass * djc,
                    const char * methodName,
                    struct djtype *  rtn_djt,
                    const char * djSignature,
                    va_list args)
{
/*
    type:
        b       Boolean (0 if int = 0, 1 otherwise)
        c       c char
        d       c double
        f       c float
        i       c int
        l       c long
        s       c zero terminated string
        v       zero terminated variable name, may be type casted
        w       64-bit integer (wide)
        z       zero terminated class name

    Type Signature Java Type
        B byte
        C char
        D double
        F float
        I int
        J long
        Lfully-qualified-class;
        S short
        V void
        Z boolean
        [array-type

*/
    int djerr;
    int six;
    char * cstr;
    jstring jstringparm;
    char * jni_sig;
    int jnismax;
    int jnislen;
    int itmp;
    struct djtype * djt;
    struct djvar * djv;
    struct djclass * z_djc;

    djerr           = 0;
    djr->djr_nparms = 0;
    six             = 0;
    jni_sig         = NULL;
    jnismax         = 0;
    jnislen         = 0;

    append_cstr(&jni_sig, &jnislen, &jnismax, "(");

    while (!djerr && djSignature[six]) {
        if (djr->djr_nparms == JNI_MAX_PARMS) {
            djerr = dj_set_error(dje, DJERREXCEEDSMAXPARMS,
                        "Call exceeds the limit of %d parameters",
                        JNI_MAX_PARMS);
        } else {
            switch (djSignature[six]) {
                case 'b':
                    itmp = va_arg(args, int);
                    djr->djr_parms[djr->djr_nparms].z = (itmp?1:0);
                    append_cstr(&jni_sig, &jnislen, &jnismax, "Z");
                    djv_get_type_from_str(dje, "boolean", &djt);
                    break;

                case 'c':
                    djr->djr_parms[djr->djr_nparms].b = (char)va_arg(args, int);
                    append_cstr(&jni_sig, &jnislen, &jnismax, "B");
                    djv_get_type_from_str(dje, "byte", &djt);
                    break;

                case 'd':
                    djr->djr_parms[djr->djr_nparms].d = va_arg(args, double);
                    append_cstr(&jni_sig, &jnislen, &jnismax, "D");
                    djv_get_type_from_str(dje, "double", &djt);
                    break;

                case 'f':
                    djr->djr_parms[djr->djr_nparms].f =
                                                (float)va_arg(args, double);
                    append_cstr(&jni_sig, &jnislen, &jnismax, "F");
                    djv_get_type_from_str(dje, "float", &djt);
                    break;

                case 'i':
                    djr->djr_parms[djr->djr_nparms].i = va_arg(args, int);
                    append_cstr(&jni_sig, &jnislen, &jnismax, "I");
                    djv_get_type_from_str(dje, "int", &djt);
                    break;

                case 'l':
                    if (LONGS_64_BIT) {
                        djr->djr_parms[djr->djr_nparms].j = va_arg(args, long);
                        append_cstr(&jni_sig, &jnislen, &jnismax, "L");
                        djv_get_type_from_str(dje, "long", &djt);
                    } else {
                        djr->djr_parms[djr->djr_nparms].i = va_arg(args, long);
                        append_cstr(&jni_sig, &jnislen, &jnismax, "I");
                        djv_get_type_from_str(dje, "int", &djt);
                    }
                    break;

                case 's':
                    cstr = va_arg(args,char*);
                    jstringparm = (*(dje->dje_je_env))->NewStringUTF(
                                                        dje->dje_je_env, cstr);
                    if (!jstringparm) {
                        if (dje->dje_trace_file) {
                            dejaTrace(dje,
                                "FAIL NewStringUTF(\"%s\") returned NULL",
                                cstr);
                        }

                        djerr = dj_set_error(dje, DJERRJNINEWSTRING,
                                "JNI NewStringUTF() failed for string \"%s\"",
                                cstr, jstringparm);
                    } else {
                        if (dje->dje_trace_file) {
                            dejaTrace(dje, "rtn  NewStringUTF(\"%s\")=0x%08x",
                                cstr, jstringparm); /* 02/24/2015 */
                        }
                        djr->djr_rel_flag[djr->djr_nparms] = 1;
                        djr->djr_parms[djr->djr_nparms].l = jstringparm;
                        append_cstr(&jni_sig, &jnislen, &jnismax,
                                                    "Ljava/lang/String;");
                        djv_get_type_from_str(dje, "java.lang.String", &djt);
                        if (dje->dje_mem_debug) {
                            djr->djr_debug_parm_marker[djr->djr_nparms] = Strdup("String parm");
                        }
                    }
                    break;

                case 'v':
                    cstr = va_arg(args,char*);
                    djerr = djv_get_variable_and_type(dje, cstr, methodName,
                                        NULL, &djv, &djt);

                    if (!djv || !djv->djv_jobject) {    /* 02/19/2015 */
                        djerr = dj_set_error(dje, DJERRNOINSTANCE,
            "Object \"%s\" in method call %s has not been instantiated.",
                                            cstr, methodName);
                    } else if (!djv->djv_var_type->djt_class) {
                        djerr = dj_set_error(dje, DJERRVARNOTOBJECT,
                    "Object \"%s\" in method call %s does not have a class.",
                                    cstr, methodName);
                    } else {
                        djr->djr_parms[djr->djr_nparms].l = djv->djv_jobject;
                        append_jni_sig(dje, djt, &jni_sig, &jnislen, &jnismax);
                    }
                    break;

                case 'w':
                    djr->djr_parms[djr->djr_nparms].j = va_arg(args, int64);
                    append_cstr(&jni_sig, &jnislen, &jnismax, "L");
                    djv_get_type_from_str(dje, "long", &djt);
                    break;

                case 'z':
                    cstr = va_arg(args,char*);
                    djerr = dj_get_class(dje, cstr, &z_djc);
                    if (!djerr) {
                         djr->djr_parms[djr->djr_nparms].l = z_djc->djc_jclass;
                        append_cstr(&jni_sig, &jnislen, &jnismax,
                                                "Ljava/lang/Class;");
                        djv_get_type_from_str(dje, "java.lang.Class", &djt);
                    }
                    break;

                default:
                    djerr = dj_set_error(dje, DJERRBADSIGCHAR,
                                "Invalid signature character \"%c\"",
                                djSignature[six]);
                    break;
            }
        }

        if (!djerr) {
            djp_add_forparm(djr->djr_formals, NULL, djt, 0);
            djr->djr_nparms += 1;
            six++;
        }
    }

    if (!djerr) {
        append_cstr(&jni_sig, &jnislen, &jnismax, ")");
        if (rtn_djt) append_jni_sig(dje, rtn_djt, &jni_sig, &jnislen, &jnismax);
        else append_cstr(&jni_sig, &jnislen, &jnismax, "V");
        djr->djr_jni_signature = jni_sig;
    } else {
        Free(jni_sig);
    }

    return (djerr);
}
/***************************************************************/
static void djv_release_djmethrec(struct djenv * dje, struct djmethrec * djr)
{
    int ii;
    jstring jstringparm;

    for (ii = 0; ii < djr->djr_nparms; ii++) {
        if (djr->djr_rel_flag[ii] && djr->djr_parms[ii].l) {
            jstringparm = djr->djr_parms[ii].l;
            if (dje->dje_trace_file) {
                dejaTrace(dje, "call DeleteLocalRef(0x%08x)", jstringparm);
            }
            (*(dje->dje_je_env))->DeleteLocalRef(dje->dje_je_env, jstringparm);
            if (dje->dje_mem_debug) {
                Free(djr->djr_debug_parm_marker[ii]);
            }
        }
    }

    djp_free_djmethrec(djr);
}
/***************************************************************/
struct djformals * djv_find_matching_formals_for_method(struct djenv * dje,
                    struct djclassdef * djd,
                    const char * methodName,
                    struct djformals * match_djf)
{
    int fix;
    struct djformals * djf;
    struct djmember * djm;

    djf = NULL;
    djm = djp_find_member_in_class(djd, methodName, IStrlen(methodName));
    if (djm) {
        fix = djp_find_formals_index(djm, match_djf);
        if (fix >= 0) {
            djf = djm->djm_formals[fix];
        }
    }

    return (djf);
}
/***************************************************************/
static int djv_get_jni_procID(struct djtype * djt) {

    int jni_procID;

    if (!djt) {
        jni_procID = JNIPROC_NEW;
    } else {
        switch (djt->djt_typetype) {
            case DJT_TYPE_VOID:     jni_procID = JNIPROC_VOID;      break;
            case DJT_TYPE_BOOLEAN:  jni_procID = JNIPROC_BOOLEAN;   break;
            case DJT_TYPE_BYTE:     jni_procID = JNIPROC_BYTE;      break;
            case DJT_TYPE_SHORT:    jni_procID = JNIPROC_SHORT;     break;
            case DJT_TYPE_INT:      jni_procID = JNIPROC_INT;       break;
            case DJT_TYPE_LONG:     jni_procID = JNIPROC_LONG;      break;
            case DJT_TYPE_FLOAT:    jni_procID = JNIPROC_FLOAT;     break;
            case DJT_TYPE_DOUBLE:   jni_procID = JNIPROC_DOUBLE;    break;
            case DJT_TYPE_CLASS:    jni_procID = JNIPROC_OBJECT;    break;
            case DJT_TYPE_TYPED_CLASS: jni_procID = JNIPROC_OBJECT; break;
            default:                jni_procID = JNIPROC_OBJECT;    break;
        }
    }

    return (jni_procID);
}
/***************************************************************/
static int djv_find_method(struct djenv * dje,
                    struct djformals ** pdjf,
                    struct djmethrec * djr,
                    struct djclass * djc,
                    const char * methodName,
                    struct djtype *  rtn_djt,
                    const char * djSignature,
                    va_list args)
{
    int djerr;
    struct djformals * djf;
    struct djclassdef * djd;
    struct djclass * djc_implied;
    struct djclass * super_djc;
    jmethodID meth;

    djc_implied = NULL;
    (*pdjf)     = NULL;
    djf         = NULL;

    djr->djr_formals = djp_new_djformals(rtn_djt);

    djerr = djSignature_to_djmethrec(dje, djr, djc,
                    methodName, rtn_djt, djSignature, args);
    if (!djerr) {
        if (!djc->djc_classdef) {
            djc_implied = djc;
            if (djc_implied->djc_implied_classdef) {
                djf = djv_find_matching_formals_for_method(dje,
                    djc_implied->djc_implied_classdef,
                    methodName, djr->djr_formals);
            }
        } else {
            djd = djc->djc_classdef;
            djf = NULL;

            while (!djf && djd) {
                djf = djv_find_matching_formals_for_method(dje,
                    djd, methodName, djr->djr_formals);
                if (!djf) {
                    if (!djd->djd_superclass) {
                        djd = NULL;
                    } else {
                        super_djc = djd->djd_superclass->djt_class;
                        if (!super_djc) {
                            djd = NULL;
                        } else {
                            if (super_djc->djc_classdef) {
                                djd = super_djc->djc_classdef;
                            } else {
                                djd = NULL;
                                djc_implied = super_djc;
                            }
                        }
                    }
                }
            }
        }
    }

    if (!djerr) {
        if (!djf && !djc_implied) {
            djerr = dj_set_error(dje, DJERRFINDMETHOD,
                "Method %s cannot be found in class %s.%s or superclass.",
                methodName,
                djc->djc_parent->djk_pkg_name, djc->djc_class_name);
        }
    }

    if (!djerr) {
        if (!djf || !djf->djf_jmethodID) {
            meth = (*(dje->dje_je_env))->GetMethodID(dje->dje_je_env,
                djc->djc_jclass, methodName, djr->djr_jni_signature);

            dje->dje_ex = (*(dje->dje_je_env))->ExceptionOccurred(
                                                    dje->dje_je_env);

            if (dje->dje_trace_file) {
                dejaTrace(dje,
                    "rtn  ExceptionOccurred()=0x%08x after GetMethodID()",
                    dje->dje_ex);
            }

            if (dje->dje_ex) {
                if (dje->dje_trace_file) {
                    dejaTrace(dje,
                        "FAIL GetMethodID(%s, %s, %s) returned exception",
                        djc->djc_class_name,
                        methodName,
                        djr->djr_jni_signature);
                }

                djerr = dj_set_error(dje, DJERRJNIFINDMETHOD,
                            "Error finding method %s in class %s.%s: %s",
                            methodName,
                            djc->djc_parent->djk_pkg_name,
                            djc->djc_class_name,
                            dj_get_JNI_message(dje));
                dj_release_JNI_message(dje);
            } else {
                if (dje->dje_trace_file) {
                    dejaTrace(dje, "rtn  GetMethodID(%s, %s, %s)=0x%08x",
                        djc->djc_class_name, methodName,
                        djr->djr_jni_signature, meth);
                }

                if (djf) {
                    djf->djf_jmethodID = meth;
                } else {
                    if (!djc_implied->djc_implied_classdef) {
                        djc_implied->djc_implied_classdef =
                            djp_new_djclassdef(djc_implied);
                    }
                    djd = djc_implied->djc_implied_classdef;
                    djf = djr->djr_formals;
                    djr->djr_formals = NULL;

                    djerr = djp_add_formals(dje, djd, methodName, djf);
                    if (!djerr) {
                        djf->djf_jmethodID = meth;
                    }
                }
                if (!djf->djj_method_name)
                    djf->djj_method_name = Strdup(methodName);
                djf->djf_jni_procID = djv_get_jni_procID(rtn_djt);
            }
        }
    }

    if (!djerr) {
        (*pdjf)     = djf;
    }

    return (djerr);
}
/***************************************************************/
int djv_call_method(struct djenv * dje,
                    struct djvar * djv,
                    struct djformals * djf,
                    struct djmethrec * djr)
{
/*
typedef union jvalue {
    jboolean z;
    jbyte    b;
    jchar    c;
    jshort   s;
    jint     i;
    jlong    j;
    jfloat   f;
    jdouble  d;
    jobject  l;
} jvalue;
*/
    int djerr;
    char jnifunc[32];

    djerr = 0;
    jnifunc[0] = '\0';

    switch (djf->djf_jni_procID) {
        case JNIPROC_VOID:
            strcpy(jnifunc, "CallVoidMethod");
            (*(dje->dje_je_env))->CallVoidMethodA(
                dje->dje_je_env, djv->djv_jobject, djf->djf_jmethodID,
                djr->djr_parms);
            break;

        case JNIPROC_BOOLEAN:
            strcpy(jnifunc, "CallBooleanMethod");
            djr->djr_rtn_val.z = (*(dje->dje_je_env))->CallBooleanMethodA(
                dje->dje_je_env, djv->djv_jobject, djf->djf_jmethodID,
                djr->djr_parms);
            break;

        case JNIPROC_BYTE:
            strcpy(jnifunc, "CallByteMethod");
            djr->djr_rtn_val.b = (*(dje->dje_je_env))->CallByteMethodA(
                dje->dje_je_env, djv->djv_jobject, djf->djf_jmethodID,
                djr->djr_parms);
            break;

        case JNIPROC_SHORT:
            strcpy(jnifunc, "CallShortMethod");
            djr->djr_rtn_val.s = (*(dje->dje_je_env))->CallShortMethodA(
                dje->dje_je_env, djv->djv_jobject, djf->djf_jmethodID,
                djr->djr_parms);
            break;

        case JNIPROC_INT:
            strcpy(jnifunc, "CallIntMethod");
            djr->djr_rtn_val.i = (*(dje->dje_je_env))->CallIntMethodA(
                dje->dje_je_env, djv->djv_jobject, djf->djf_jmethodID,
                &(djr->djr_parms[0]));
            break;

        case JNIPROC_CHAR:
            strcpy(jnifunc, "CallCharMethod");
            djr->djr_rtn_val.c = (*(dje->dje_je_env))->CallCharMethodA(
                dje->dje_je_env, djv->djv_jobject, djf->djf_jmethodID,
                djr->djr_parms);
            break;

        case JNIPROC_FLOAT:
            strcpy(jnifunc, "CallFloatMethod");
            djr->djr_rtn_val.f = (*(dje->dje_je_env))->CallFloatMethodA(
                dje->dje_je_env, djv->djv_jobject, djf->djf_jmethodID,
                djr->djr_parms);
            break;

        case JNIPROC_DOUBLE:
            strcpy(jnifunc, "CallDoubleMethod");
            djr->djr_rtn_val.d = (*(dje->dje_je_env))->CallDoubleMethodA(
                dje->dje_je_env, djv->djv_jobject, djf->djf_jmethodID,
                djr->djr_parms);
            break;

        case JNIPROC_OBJECT:
            strcpy(jnifunc, "CallObjectMethod");
            djr->djr_rtn_val.l = (*(dje->dje_je_env))->CallObjectMethodA(
                dje->dje_je_env, djv->djv_jobject, djf->djf_jmethodID,
                djr->djr_parms);
            break;

        case JNIPROC_NEW:
            strcpy(jnifunc, "NewObject");
            djv->djv_jobject = (*(dje->dje_je_env))->NewObjectA(
                                    dje->dje_je_env,
                                    djv->djv_var_type->djt_class->djc_jclass,
                                    djf->djf_jmethodID,
                                    djr->djr_parms);
            break;

        default:
            djerr = dj_set_error(dje, DJERRJNIPROCID,
                        "Invalid JNI procedure ID %d", djf->djf_jni_procID);
            break;
    }

    if (!djerr) {
        dje->dje_ex = (*(dje->dje_je_env))->ExceptionOccurred(dje->dje_je_env);
        if (dje->dje_trace_file) {
            dejaTrace(dje, "rtn  ExceptionOccurred()=0x%08x after %s()",
                dje->dje_ex, jnifunc);
        }
        if (dje->dje_ex) {
            djerr = dj_set_error(dje, DJERREXCEPTION,
                        "Exception occurred in method %s: %s",
                        djf->djf_parent->djm_member_name,
                        dj_get_JNI_message(dje));
            dj_release_JNI_message(dje);
        } else {
            if (dje->dje_trace_file) {
                char * val;

                switch (djf->djf_jni_procID) {
                    case JNIPROC_VOID:
                        val = Strdup("");
                        break;

                    case JNIPROC_BOOLEAN:
                        if (djr->djr_rtn_val.z) val = Strdup("=1");
                        else                    val = Strdup("=0");
                        break;

                    case JNIPROC_BYTE:
                        val = SMPRINTF("=%d", (int)djr->djr_rtn_val.b);
                        break;

                    case JNIPROC_SHORT:
                        val = SMPRINTF("=%d", (int)djr->djr_rtn_val.s);
                        break;

                    case JNIPROC_INT:
                        val = SMPRINTF("=%d", (int)djr->djr_rtn_val.i);
                        break;

                    case JNIPROC_CHAR:
                        if (djr->djr_rtn_val.c < ' ' ||
                            djr->djr_rtn_val.c > '~') {
                            val = SMPRINTF("=\\x%02x",
                                    (int)djr->djr_rtn_val.c);
                        } else {
                            val = SMPRINTF("=\'%c\'",
                                    (int)djr->djr_rtn_val.c);
                        }
                        break;

                    case JNIPROC_FLOAT:
                        val = SMPRINTF("=%g", (double)djr->djr_rtn_val.f);
                        break;

                    case JNIPROC_DOUBLE:
                        val = SMPRINTF("=%g", (double)djr->djr_rtn_val.d);
                        break;

                    case JNIPROC_OBJECT:
                        val = SMPRINTF("=(%s)0x%08x",
                            djv->djv_var_type->djt_class->djc_class_name,
                            (int)djr->djr_rtn_val.l);
                       break;

                    case JNIPROC_NEW:
                        val = SMPRINTF("=0x%08x",
                            (int)djv->djv_jobject);
                        break;

                    default:
                        val = Strdup("?");
                        break;
                }
                if (djf->djf_jni_procID == JNIPROC_NEW) {
                    dejaTrace(dje, "rtn  Call %s.%s %s(%s)%s",
                        djv->djv_var_name,
                        djf->djf_parent->djm_member_name,
                        jnifunc,
                        djv->djv_var_type->djt_class->djc_class_name,
                        val);
                } else {
                    dejaTrace(dje, "rtn  Call %s.%s %s(0x%08x)%s",
                        djv->djv_var_name,
                        djf->djf_parent->djm_member_name,
                        jnifunc, djv->djv_jobject, val);
                }
                Free(val);
            }
        }
    }

    return (djerr);
}
/***************************************************************/
int dejaNewObject(struct djenv * dje,
                            const char * objName,
                            const char * fullClassName,
                            const char * djSignature, ...)
{
    va_list args;
    int djerr;
    struct djvar * djv;
    struct djclass * djc;
    struct djmethrec * djr;
    struct djformals * djf;

    djerr = 0;
    djr = NULL;

    if (!dje->dje_je_env) {
        djerr = dj_set_error(dje, DJERRNOJVM, "JVM not started.");
        return (djerr);
    }

    djerr = dj_get_class(dje, fullClassName, &djc);

    if (!djerr) {
        djv = dj_find_variable(dje, objName);
        if (djv) {
            djerr = dj_match_class_or_subclass(dje,
                                        djv->djv_var_type->djt_class, djc);
            if (!djerr) {
                djerr = dj_release_variable(dje, djv);
            }
        } else {
            djerr = dj_new_variable(dje, objName, djc, &djv);
        }
    }

    if (!djerr) {
        djr = djp_new_djmethrec();

        va_start(args, djSignature);
        djerr = djv_find_method(dje, &djf, djr, djc,
                    JNI_INIT_METHOD, NULL, djSignature, args);
        va_end(args);
    }

    if (!djerr) {
        djerr = djv_call_method(dje, djv, djf, djr);
    }

    if (djr) {
        if (dje->dje_mem_debug) djv->djv_debug_marker = Strdup(objName);
        djv_release_djmethrec(dje, djr);
    }

    return (djerr);
}
/***************************************************************/
int dejaNewNullObject(struct djenv * dje,
                            const char * objName,
                            const char * fullClassName)
{
    int djerr;
    struct djvar * djv;
    struct djclass * djc;
    djerr = 0;

    if (!dje->dje_je_env) {
        djerr = dj_set_error(dje, DJERRNOJVM, "JVM not started.");
        return (djerr);
    }

    djerr = dj_get_class(dje, fullClassName, &djc);
    if (!djerr) {
        djv = dj_find_variable(dje, objName);
        if (djv) {
            djerr = dj_set_error(dje, DJERRDUPVAR,
                            "Duplicate variable %s", objName);
        } else {
            djerr = dj_new_variable(dje, objName, djc, &djv);
        }
    }

    return (djerr);
}
/***************************************************************/
static int djv_check_var(struct djenv * dje,
                            const char * objName,
                            const char * methodName,
                            struct djvar ** pdjv)
{
    int djerr;
    struct djvar * djv;

    djerr = 0;
    (*pdjv) = NULL;

    djv = dj_find_variable(dje, objName);
    if (!djv) {
        djerr = dj_set_error(dje, DJERRNOSUCHVAR,
            "Object \"%s\" used to call method %s has not been defined",
                            objName, methodName);
    } else if (!djv->djv_jobject) {
        djerr = dj_set_error(dje, DJERROTINSTANTIATED,
            "Object \"%s\" used to call method %s has not been instantiated",
                            objName, methodName);
    } else {
        (*pdjv) = djv;
    }

    return (djerr);
}
/***************************************************************/
int dejaCallVoidMethod(struct djenv * dje,
                            const char * objName,
                            const char * methodName,
                            const char * djSignature, ...)
{
    va_list args;
    int djerr;
    struct djvar * djv;
    struct djmethrec * djr;
    struct djformals * djf;
    struct djtype * rtn_djt;

    djr = NULL;
    djerr = djv_check_var(dje, objName, methodName, &djv);

    if (!djerr) {
        djr = djp_new_djmethrec();
        djv_get_type_from_str(dje, "void", &rtn_djt);

        va_start(args, djSignature);
        djerr = djv_find_method(dje, &djf, djr, djv->djv_var_type->djt_class,
                    methodName, rtn_djt, djSignature, args);
        va_end(args);
    }

    if (!djerr) {
        djerr = djv_call_method(dje, djv, djf, djr);
    }

    if (djr) {
        djv_release_djmethrec(dje, djr);
    }

    return (djerr);
}
/***************************************************************/
int dejaCallBooleanMethod(struct djenv * dje,
                            int * rtnInt,
                            const char * objName,
                            const char * methodName,
                            const char * djSignature, ...)
{
    va_list args;
    int djerr;
    struct djvar * djv;
    struct djmethrec * djr;
    struct djformals * djf;
    struct djtype * rtn_djt;

    djr = NULL;
    djerr = djv_check_var(dje, objName, methodName, &djv);

    if (!djerr) {
        djr = djp_new_djmethrec();
        djv_get_type_from_str(dje, "boolean", &rtn_djt);

        va_start(args, djSignature);
        djerr = djv_find_method(dje, &djf, djr, djv->djv_var_type->djt_class,
                    methodName, rtn_djt, djSignature, args);
        va_end(args);
    }

    if (!djerr) {
        djerr = djv_call_method(dje, djv, djf, djr);
        (*rtnInt) = djr->djr_rtn_val.z;
    }

    if (djr) djv_release_djmethrec(dje, djr);

    return (djerr);
}
/***************************************************************/
int dejaCallByteMethod(struct djenv * dje,
                            int * rtnInt,
                            const char * objName,
                            const char * methodName,
                            const char * djSignature, ...)
{
    va_list args;
    int djerr;
    struct djvar * djv;
    struct djmethrec * djr;
    struct djformals * djf;
    struct djtype * rtn_djt;

    djr = NULL;
    djerr = djv_check_var(dje, objName, methodName, &djv);

    if (!djerr) {
        djr = djp_new_djmethrec();
        djv_get_type_from_str(dje, "byte", &rtn_djt);

        va_start(args, djSignature);
        djerr = djv_find_method(dje, &djf, djr, djv->djv_var_type->djt_class,
                    methodName, rtn_djt, djSignature, args);
        va_end(args);
    }

    if (!djerr) {
        djerr = djv_call_method(dje, djv, djf, djr);
        (*rtnInt) = djr->djr_rtn_val.b;
    }

    if (djr) djv_release_djmethrec(dje, djr);

    return (djerr);
}
/***************************************************************/
int dejaCallCharMethod(struct djenv * dje,
                            int * rtnInt,
                            const char * objName,
                            const char * methodName,
                            const char * djSignature, ...)
{
    va_list args;
    int djerr;
    struct djvar * djv;
    struct djmethrec * djr;
    struct djformals * djf;
    struct djtype * rtn_djt;

    djr = NULL;
    djerr = djv_check_var(dje, objName, methodName, &djv);

    if (!djerr) {
        djr = djp_new_djmethrec();
        djv_get_type_from_str(dje, "char", &rtn_djt);

        va_start(args, djSignature);
        djerr = djv_find_method(dje, &djf, djr, djv->djv_var_type->djt_class,
                    methodName, rtn_djt, djSignature, args);
        va_end(args);
    }

    if (!djerr) {
        djerr = djv_call_method(dje, djv, djf, djr);
        (*rtnInt) = djr->djr_rtn_val.c;
    }

    if (djr) djv_release_djmethrec(dje, djr);

    return (djerr);
}
/***************************************************************/
int dejaCallShortMethod(struct djenv * dje,
                            const char * objName,
                            int * rtnInt,
                            const char * methodName,
                            const char * djSignature, ...)
{
    va_list args;
    int djerr;
    struct djvar * djv;
    struct djmethrec * djr;
    struct djformals * djf;
    struct djtype * rtn_djt;

    djr = NULL;
    djerr = djv_check_var(dje, objName, methodName, &djv);

    if (!djerr) {
        djr = djp_new_djmethrec();
        djv_get_type_from_str(dje, "short", &rtn_djt);

        va_start(args, djSignature);
        djerr = djv_find_method(dje, &djf, djr, djv->djv_var_type->djt_class,
                    methodName, rtn_djt, djSignature, args);
        va_end(args);
    }

    if (!djerr) {
        djerr = djv_call_method(dje, djv, djf, djr);
        (*rtnInt) = djr->djr_rtn_val.s;
    }

    if (djr) djv_release_djmethrec(dje, djr);

    return (djerr);
}
/***************************************************************/
int dejaCallIntMethod(struct djenv * dje,
                            int * rtnInt,
                            const char * objName,
                            const char * methodName,
                            const char * djSignature, ...)
{
    va_list args;
    int djerr;
    struct djvar * djv;
    struct djmethrec * djr;
    struct djformals * djf;
    struct djtype * rtn_djt;

    djr = NULL;
    djerr = djv_check_var(dje, objName, methodName, &djv);

    if (!djerr) {
        djr = djp_new_djmethrec();
        djv_get_type_from_str(dje, "int", &rtn_djt);

        va_start(args, djSignature);
        djerr = djv_find_method(dje, &djf, djr, djv->djv_var_type->djt_class,
                    methodName, rtn_djt, djSignature, args);
        va_end(args);
    }

    if (!djerr) {
        djerr = djv_call_method(dje, djv, djf, djr);
        (*rtnInt) = djr->djr_rtn_val.i;
    }

    if (djr) djv_release_djmethrec(dje, djr);

    return (djerr);
}
/***************************************************************/
int dejaCallLongMethod(struct djenv * dje,
                            int64 * rtnVal,
                            const char * objName,
                            const char * methodName,
                            const char * djSignature, ...)
{
    va_list args;
    int djerr;
    struct djvar * djv;
    struct djmethrec * djr;
    struct djformals * djf;
    struct djtype * rtn_djt;

    djr = NULL;
    djerr = djv_check_var(dje, objName, methodName, &djv);

    if (!djerr) {
        djr = djp_new_djmethrec();
        djv_get_type_from_str(dje, "long", &rtn_djt);

        va_start(args, djSignature);
        djerr = djv_find_method(dje, &djf, djr, djv->djv_var_type->djt_class,
                    methodName, rtn_djt, djSignature, args);
        va_end(args);
    }

    if (!djerr) {
        djerr = djv_call_method(dje, djv, djf, djr);
        (*rtnVal) = djr->djr_rtn_val.j;
    }

    if (djr) djv_release_djmethrec(dje, djr);

    return (djerr);
}
/***************************************************************/
int dejaCallFloatMethod(struct djenv * dje,
                            float * rtnVal,
                            const char * objName,
                            const char * methodName,
                            const char * djSignature, ...)
{
    va_list args;
    int djerr;
    struct djvar * djv;
    struct djmethrec * djr;
    struct djformals * djf;
    struct djtype * rtn_djt;

    djr = NULL;
    djerr = djv_check_var(dje, objName, methodName, &djv);

    if (!djerr) {
        djr = djp_new_djmethrec();
        djv_get_type_from_str(dje, "float", &rtn_djt);

        va_start(args, djSignature);
        djerr = djv_find_method(dje, &djf, djr, djv->djv_var_type->djt_class,
                    methodName, rtn_djt, djSignature, args);
        va_end(args);
    }

    if (!djerr) {
        djerr = djv_call_method(dje, djv, djf, djr);
        (*rtnVal) = djr->djr_rtn_val.f;
    }

    if (djr) djv_release_djmethrec(dje, djr);

    return (djerr);
}
/***************************************************************/
int dejaCallDoubleMethod(struct djenv * dje,
                            double * rtnVal,
                            const char * objName,
                            const char * methodName,
                            const char * djSignature, ...)
{
    va_list args;
    int djerr;
    struct djvar * djv;
    struct djmethrec * djr;
    struct djformals * djf;
    struct djtype * rtn_djt;

    djr = NULL;
    djerr = djv_check_var(dje, objName, methodName, &djv);

    if (!djerr) {
        djr = djp_new_djmethrec();
        djv_get_type_from_str(dje, "double", &rtn_djt);

        va_start(args, djSignature);
        djerr = djv_find_method(dje, &djf, djr, djv->djv_var_type->djt_class,
                    methodName, rtn_djt, djSignature, args);
        va_end(args);
    }

    if (!djerr) {
        djerr = djv_call_method(dje, djv, djf, djr);
        (*rtnVal) = djr->djr_rtn_val.d;
    }

    if (djr) djv_release_djmethrec(dje, djr);

    return (djerr);
}
/***************************************************************/
int dejaCallObjectMethod(struct djenv * dje,
                            const char * destObjName,
                            const char * objName,
                            const char * methodName,
                            const char * djSignature, ...)
{
    va_list args;
    int djerr;
    struct djvar * djv;
    struct djvar * dest_djv;
    struct djmethrec * djr;
    struct djformals * djf;
    struct djtype * dest_djt;

    djr = NULL;
    djf = NULL;

    djerr = djv_check_var(dje, objName, methodName, &djv);
    if (!djerr) {
        djerr = djv_get_variable_and_type(dje, destObjName, methodName,
                            NULL, &dest_djv, &dest_djt);
    }

    if (!djerr) {
        djr = djp_new_djmethrec();

        va_start(args, djSignature);
        djerr = djv_find_method(dje, &djf, djr, djv->djv_var_type->djt_class,
                    methodName, dest_djt, djSignature, args);
        va_end(args);
    }

    if (!djerr) {
        djerr = djv_call_method(dje, djv, djf, djr);
        /* DELETE_REUSED_OBJECTS - Delete object before overwriting it */
        /* Fixed 2/26/2015 */
        if (dest_djv->djv_jobject) {
            if (dje->dje_trace_file) {
                dejaTrace(dje, "call DeleteLocalRef(%s.%s)=0x%08x",
                    dest_djv->djv_var_type->djt_class->djc_class_name,
                    dest_djv->djv_var_name, dest_djv->djv_jobject);
            }
            (*(dje->dje_je_env))->DeleteLocalRef(
                    dje->dje_je_env, dest_djv->djv_jobject);
            dest_djv->djv_jobject = NULL;
            if (dje->dje_mem_debug) {
                Free(dest_djv->djv_debug_marker);
                dest_djv->djv_debug_marker = NULL;
            }
        }
        dest_djv->djv_jobject = djr->djr_rtn_val.l;
    }

    if (djr) {
        if (dje->dje_mem_debug)
                dest_djv->djv_debug_marker = Strdup(dest_djv->djv_var_name);
        djv_release_djmethrec(dje, djr);
    }

    return (djerr);
}
/***************************************************************/
int dejaCallStringMethod(struct djenv * dje,
                            char ** rtnStr,
                            const char * objName,
                            const char * methodName,
                            const char * djSignature, ...)
{
    va_list args;
    int djerr;
    struct djvar * djv;
    struct djmethrec * djr;
    struct djformals * djf;
    struct djtype * rtn_djt;
    jstring jstr;
    jboolean is_copy;
    char * rtnval;
    int rlen;

    djr = NULL;
    jstr = NULL;
    djerr = djv_check_var(dje, objName, methodName, &djv);

    if (!djerr) {
        djr = djp_new_djmethrec();
        djv_get_type_from_str(dje, "java.lang.String", &rtn_djt);

        va_start(args, djSignature);
        djerr = djv_find_method(dje, &djf, djr, djv->djv_var_type->djt_class,
                    methodName, rtn_djt, djSignature, args);
        va_end(args);
    }

    if (!djerr) {
        djerr = djv_call_method(dje, djv, djf, djr);
        if (!djerr) {
            jstr = djr->djr_rtn_val.l;
            if (!jstr) {
                (*rtnStr) = NULL;
            }
        }
    }

    if (!djerr && jstr) {
        rtnval = (char*)((*(dje->dje_je_env))->GetStringUTFChars(
                        dje->dje_je_env, jstr, &is_copy));
        dje->dje_ex = (*(dje->dje_je_env))->ExceptionOccurred(dje->dje_je_env);

        if (dje->dje_trace_file) {
            dejaTrace(dje,
                "rtn  ExceptionOccurred()=0x%08x after GetStringUTFChars()",
                dje->dje_ex);
        }


        if (dje->dje_ex) {
            if (dje->dje_trace_file) {
                dejaTrace(dje,
                    "FAIL GetStringUTFChars(0x%08x) returned exception",
                    jstr);
            }

            djerr = dj_set_error(dje, DJERREXCEPTION,
                        "Exception occurred in JNI GetStringUTFChars(): %s",
                        dj_get_JNI_message(dje));
            dj_release_JNI_message(dje);
        } else {
            rlen = IStrlen(rtnval) + 1;
            (*rtnStr) = New(char, rlen);
            memcpy(*rtnStr, rtnval, rlen);

            if (dje->dje_trace_file) {
                dejaTrace(dje, "rtn  GetStringUTFChars(0x%08x)=\"%s\"",
                    jstr, (*rtnStr));
                dejaTrace(dje, "call ReleaseStringUTFChars(0x%08x)", jstr);
            }

            (*(dje->dje_je_env))->ReleaseStringUTFChars(
                                dje->dje_je_env, jstr, rtnval);

            (*(dje->dje_je_env))->DeleteLocalRef(dje->dje_je_env, jstr);
        }
    }

    if (djr) {
        djv_release_djmethrec(dje, djr);
    }

    return (djerr);
}
/***************************************************************/
void dejaReleaseString(struct djenv * dje, char * rtnStr)
{
    Free(rtnStr);
}
/***************************************************************/
static int dj_push_frame_v(struct djenv * dje,
                            const char * var_name_1,
                            va_list args)
{
    int djerr;
    struct djvarnest * prev_varnest;
    struct djvar * djv;
    struct djvar * new_djv;
    struct djvar ** vdjv;
    const char * var_name;

    djerr = dj_push_varnest(dje);

    if (!djerr && dje->dje_varnest->djn_prev_varnest) {
        prev_varnest = dje->dje_varnest->djn_prev_varnest;
        var_name = var_name_1;

        while (!djerr && var_name) {
            vdjv = (struct djvar **)dbtree_find(prev_varnest->djn_vars,
                        var_name, IStrlen(var_name));
            if (!vdjv) {
                djerr = dj_set_error(dje, DJVARNOTFOUND,
                        "Object \"%s\" has not been defined.", var_name);
            } else {
                djv = *vdjv;
                djerr = dj_new_variable(dje,
                    var_name, djv->djv_var_type->djt_class, &new_djv);

                if (!djerr && djv->djv_jobject) {
                    new_djv->djv_jobject = (*(dje->dje_je_env))->NewLocalRef(
                            dje->dje_je_env, djv->djv_jobject);

                    if (!new_djv->djv_jobject) {
                        if (dje->dje_trace_file) {
                            dejaTrace(dje,
                                "FAIL NewLocalRef(0x%08x) returned null",
                                djv->djv_jobject);
                        }

                        djerr = dj_set_error(dje, DJNEWVARREF,
                                "Error creating new reference for \"%s\"",
                                var_name);
                    } else {
                        if (dje->dje_trace_file) {
                            dejaTrace(dje, "rtn  NewLocalRef(%s.%s)=0x%08x",
                                djv->djv_var_type->djt_class->djc_class_name,
                                var_name, new_djv->djv_jobject);
                        }
                        if (dje->dje_mem_debug) {
                            new_djv->djv_debug_marker = Strdup(var_name);
                        }
                    }
                }
                var_name = va_arg(args, const char*);
            }
        }
    }

    return (djerr);
}
/***************************************************************/
int dejaPushFrame(struct djenv * dje, const char * var_name_1, ...)
{
    va_list args;
    int djerr;

    va_start(args, var_name_1);
    djerr = dj_push_frame_v(dje, var_name_1, args);
    va_end(args);

    return (djerr);
}
/***************************************************************/
int dejaDeleteVarObject(struct djenv * dje, const char * var_name)

{
    int djerr;
    struct djvar *  djv;

    djerr = 0;
    djv = dj_find_variable(dje, var_name);
    if (!djv) {
        djerr = dj_set_error(dje, DJERRNOSUCHVAR,
                    "Object \"%s\" not been defined",
                                        var_name);
    } else {
        if (djv->djv_jobject) {
            if (dje->dje_trace_file) {
                dejaTrace(dje, "call DeleteLocalRef(%s.%s)=0x%08x",
                    djv->djv_var_type->djt_class->djc_class_name,
                    djv->djv_var_name, djv->djv_jobject);
            }
            (*(dje->dje_je_env))->DeleteLocalRef(
                    dje->dje_je_env, djv->djv_jobject);
            djv->djv_jobject = NULL;
            if (dje->dje_mem_debug) {
                Free(djv->djv_debug_marker);
                djv->djv_debug_marker = NULL;
            }
        }
    }

    return (djerr);
}
/***************************************************************/
int dejaPopFrame(struct djenv * dje)
{
    int djerr;

    djerr = dj_pop_varnest(dje);

    return (djerr);
}
/***************************************************************/
#ifdef NOT_YET
int dejaToString(struct djenv * dje,
                            const char * objName,
                            char ** rtnStr)
{

    Need to call djv_find_method()

    int djerr;
    struct djvar * djv;
    struct djmethrec * djr;
    struct djformals * djf;
    struct djtype * rtn_djt;
    jstring jstr;
    jboolean is_copy;
    char * rtnval;
    int rlen;

    djr = NULL;
    djerr = djv_check_var(dje, objName, "toString", &djv);

    if (!djerr) {
        djr = djp_new_djmethrec();
        djv_get_type_from_str(dje, "java.lang.String", &rtn_djt);
        djr->djr_formals = djp_new_djformals(rtn_djt);

        djf = djv_find_matching_formals_for_method(dje,
            djd, "toString", djr->djr_formals);

        va_start(args, djSignature);
        djerr = djv_find_method(dje, &djf, djr, djv->djv_var_type->djt_class,
                    methodName, rtn_djt, djSignature, args);
        va_end(args);
    }

    if (!djerr) {
        djerr = djv_call_method(dje, djv, djf, djr);
        jstr = djr->djr_rtn_val.l;

        rtnval = (char*)((*(dje->dje_je_env))->GetStringUTFChars(
                                dje->dje_je_env, jstr, &is_copy));
        dje->dje_ex = (*(dje->dje_je_env))->ExceptionOccurred(dje->dje_je_env);
        if (dje->dje_ex) {
            if (dje->dje_trace_file) {
                dejaTrace(dje,
                    "FAIL GetStringUTFChars(0x%08x) returned NULL", jstr);
            }

            djerr = dj_set_error(dje, DJERREXCEPTION,
                        "Exception occurred in JNI GetStringUTFChars(): %s",
                        dj_get_JNI_message(dje));
            dj_release_JNI_message(dje);
        } else {
            rlen = strlen(rtnval) + 1;
            (*rtnStr) = New(char, rlen);
            memcpy(*rtnStr, rtnval, rlen);

            if (dje->dje_trace_file) {
                dejaTrace(dje,
                    "rtn  GetStringUTFChars(0x%08x)=\"%s\"", jstr, rtnval);
                dejaTrace(dje, "call ReleaseStringUTFChars(0x%08x)",
                    jstr);
            }

            (*(dje->dje_je_env))->ReleaseStringUTFChars(
                            dje->dje_je_env, jstr, rtnval);
        }
    }

    return (djerr);
}
#endif

/***************************************************************/
void get_nojava_msg(char * nojavamsg, int nojavamsg_sz) {

#if defined(WIN32)
    int need32bit_java;
    char * pfx86;
    char jnamex86[256];

    int is_wow_64(void);

    need32bit_java = 0;

    if (is_wow_64() > 0) {
        pfx86 = getenv("ProgramFiles(x86)");
        if (pfx86) {
            SNPRINTF(jnamex86, sizeof(jnamex86), "%s\\%s", pfx86, "Java");

            if (!os_fexists(jnamex86)) {
                need32bit_java = 1;
            }
        }
    }

    if (need32bit_java) {
        SNPRINTF(nojavamsg, nojavamsg_sz, "%s",
                            "Unavailable (Need 32-bit Java)");
    } else {
        SNPRINTF(nojavamsg, nojavamsg_sz, "%s", "Unavailable");
    }
#else
    SNPRINTF(nojavamsg, nojavamsg_sz, "%s", "Unavailable");
#endif
}
/***************************************************************/





/***************************************************************/
#if IS_WINDOWS
int win_get_registry_jvm(char * reg_val, int reg_val_sz) {

/*
** returns:
**  0 - server found and name is in full_prog_name
**  1 - server not found
**  2 - server wrong type
*/
    int srcherr;
    HKEY hKeyJRE;
    HKEY hKeyCurrVers;
    int rtn;
    DWORD ent_typ;
    LONG reg_val_len;
    char java_vers[128];

    srcherr = 0;

    reg_val[0] = '\0';
    rtn = RegOpenKey(HKEY_LOCAL_MACHINE,
        "SOFTWARE\\JavaSoft\\Java Runtime Environment", &hKeyJRE);
    if (rtn !=  ERROR_SUCCESS) {
        srcherr = 1;
    } else {
        reg_val_len = sizeof(java_vers);
        rtn = RegQueryValueEx(hKeyJRE,
            "CurrentVersion", NULL, &ent_typ, java_vers, &reg_val_len);

        if (rtn != ERROR_SUCCESS) {
            srcherr = 1;
            /* ReportRegistryCallError(rtn); */
        } else if (ent_typ != REG_SZ) {
            srcherr = 2;
        } else {
            rtn = RegOpenKey(hKeyJRE, java_vers, &hKeyCurrVers);
            if (rtn !=  ERROR_SUCCESS) {
                srcherr = 1;
            } else {
                reg_val_len = reg_val_sz;
                rtn = RegQueryValueEx(hKeyCurrVers,
                    "RuntimeLib", NULL, &ent_typ, reg_val, &reg_val_len);

                if (rtn != ERROR_SUCCESS) {
                    srcherr = 1;
                    /* ReportRegistryCallError(rtn); */
                } else if (ent_typ != REG_SZ) {
                    srcherr = 2;
                }
                RegCloseKey(hKeyCurrVers);
            }
        }

        RegCloseKey(hKeyJRE);
    }

    return (srcherr);
}
#endif
/***************************************************************/
int ux_get_registry_jvm(char * reg_val, int reg_val_sz) {

    return (-1);
}
/***************************************************************/
int get_registry_jvm(char * reg_val, int reg_val_sz) {

#if IS_WINDOWS
    return win_get_registry_jvm(reg_val, reg_val_sz);
#else
    return ux_get_registry_jvm(reg_val, reg_val_sz);
#endif
}
/***************************************************************/
int init_java(char* errmsg, int errmsgmax) {
/*
Working dir: C:\Program Files\Java\jdk1.7.0_21\jre\bin\client
*/
    char * jvmlibvar;
    char jvmlib[256];

    jvmlibvar  = NULL;
    jvmlib[0] = '\0';

#if DYN_LOAD

    jvmlibvar    = getenv("JVMLIB");
    if (jvmlibvar && jvmlibvar[0]) {
        strncpy(jvmlib, jvmlibvar, sizeof(jvmlib));
    } else {
        if (get_registry_jvm(jvmlib, sizeof(jvmlib))) {
            sprintf(errmsg,
                "JVMLIB not defined and no library name not available.");
            return (-1);
        }
    }

    /* Do JNI_CreateJavaVM first */
    JNI_CREATEJAVAVM = (p_JNI_CreateJavaVM)
        dynldlibfunction(jvmlib, "JNI_CreateJavaVM", errmsg, errmsgmax);
    if (!JNI_CREATEJAVAVM) {
        return (1);
    }
#endif

    printf(LOAD_MSG, jvmlib);
    
    return 0;
}
/***************************************************************/
void get_JNI_message(JNIEnv *env,
                            jthrowable jexcept,
                            char* errmsg,
                            int errmsgmax)
{
/*
** Assumes         jexcept = (*env)->ExceptionOccurred(env)
*/
    jmethodID toString;
    jclass objClass;
    jboolean isCopy;
    jstring jerrmsg;
    const char * err_chars;

    if (!jexcept) {
        strcpy(errmsg, "JNI ERROR: No exception.");
    } else {
        objClass = (*env)->FindClass(
                        env, "java/lang/Object");
        if (!objClass) {
            strcpy(errmsg, "JNI ERROR: Cannot find class java/lang/Object");
        } else {
            toString = (*env)->GetMethodID(env,
                            objClass, "toString", "()Ljava/lang/String;");
            if (!toString) {
                strcpy(errmsg, "JNI ERROR: Cannot find toString()");
            } else {
                jerrmsg =
                    (jstring) (*env)->CallObjectMethod(
                    env, jexcept, toString);
                if (!jerrmsg) {
                    strcpy(errmsg, "JNI ERROR: Null error message");
                } else {
                    err_chars = (*env)->GetStringUTFChars(
                        env, jerrmsg, &isCopy);
                    if (!err_chars) {
                        strcpy(errmsg, "JNI ERROR: Null GetStringUTFChars() message");
                    } else {
                        strcpy(errmsg, err_chars);
                        (*env)->ReleaseStringUTFChars(env, jerrmsg, err_chars);
                    }
                }
            }
        }
    }
}
/*************************************************************/
int test_class(JNIEnv *env, const char * class_name)
{
    int rtn;
    jclass foundClass;
    jthrowable jexcept;
    char jmsg[256];

    rtn = 0;
    printf("Test class: %s\n", class_name);

    foundClass = (*env)->FindClass(env, class_name);
    jexcept = (*env)->ExceptionOccurred(env);
    if (jexcept) {
        get_JNI_message(env, jexcept, jmsg, sizeof(jmsg));
        printf("Exception on FindClass(%s): %s\n",
                class_name, jmsg);
        return (-1);
    }
    if (!foundClass) {
        printf("FindClass(%s) returned null.\n", class_name);
        return (-1);
    }
    printf("FindClass(%s) succsessful.\n", class_name);

    return (rtn);
}
/*************************************************************/
int test_jstring(JNIEnv *env, char* errmsg, int errmsgmax)
{
    int rtn;
    jmethodID lenMethod;
    jclass stringClass;
    jstring tstring;
    jint tstringLen;
    jboolean isCopy;
    jthrowable jexcept;
    const jbyte* jStringChars;
    char tchars[128];
    char jmsg[256];

    rtn = 0;

    strcpy(tchars, "JNI test string.");

    tstring = (*env)->NewStringUTF(env, tchars);
    jexcept = (*env)->ExceptionOccurred(env);
    if (jexcept) {
        sprintf(errmsg, "Exception occurred on call to NewStringUTF()\n");
        return (-1);
    }
    if (!tstring) {
        sprintf(errmsg, "NewStringUTF() returned null.\n");
        return (-1);
    }

    stringClass = (*env)->FindClass(env, "java/lang/String");
    jexcept = (*env)->ExceptionOccurred(env);
    if (jexcept) {
        sprintf(errmsg, "Exception occurred on call to FindClass(java/lang/String)\n");
        return (-1);
    }
    if (!stringClass) {
        sprintf(errmsg, "FindClass(java/lang/String) returned null.\n");
        return (-1);
    }

    lenMethod = (*env)->GetMethodID(env, stringClass, "length", "()I");
    jexcept = (*env)->ExceptionOccurred(env);
    if (jexcept) {
        get_JNI_message(env, jexcept, jmsg, sizeof(jmsg));
        sprintf(errmsg, "GetMethodID(String.length) exception: %s\n", jmsg);
        return (-1);
    }
    if (!lenMethod) {
        sprintf(errmsg, "GetMethodID(String.length) returned null.\n");
        return (-1);
    }

    tstringLen = (*env)->CallIntMethod(env, tstring, lenMethod);
    jexcept = (*env)->ExceptionOccurred(env);
    if (jexcept) {
        sprintf(errmsg, "Exception occurred on call to CallIntMethod()\n");
        return (-1);
    }
    if ((int)tstringLen != (int)strlen(tchars)) {
        sprintf(errmsg, "Length mismatch String.length=%d, strlen=%d\n",
                (int)tstringLen, (int)strlen(tchars));
        return (-1);
    }

    jStringChars = (*env)->GetStringUTFChars(env, tstring, &isCopy);
    jexcept = (*env)->ExceptionOccurred(env);
    if (jexcept) {
        sprintf(errmsg, "Exception occurred on call to GetStringUTFChars()\n");
        return (-1);
    }
    if (!jStringChars) {
        sprintf(errmsg, "GetStringUTFChars() returned null.\n");
        return (-1);
    }
    if (strcmp(tchars, jStringChars)) {
        sprintf(errmsg, "String mismatch: chars=\"%s\" String=\"%s\"\n",
                tchars, jStringChars);
        return (-1);
    }

    (*env)->ReleaseStringUTFChars(env, tstring, jStringChars);
    jexcept = (*env)->ExceptionOccurred(env);
    if (jexcept) {
        sprintf(errmsg, "Exception occurred on call to ReleaseStringUTFChars()\n");
        return (-1);
    }

    return (rtn);
}
/*************************************************************/
int test_jvm(int argc, char *argv[])
{
    int rtn;
    int fcrtn;
    int ii;
    JavaVMOption options[MAX_VM_OPTS];
    JNIEnv *env;
    JavaVM *jvm;
    JavaVMInitArgs vm_args;
    long status;
    int jvm_version;
    char errmsg[256];

    memset(&vm_args, 0, sizeof(vm_args));
    vm_args.nOptions = 0;
    vm_args.version = JNI_VERSION_1_2;
    vm_args.options = options;

    ii = 1;
    while (ii < argc) {
        if (argv[ii][0] == '-') {
            if (!strcmp(argv[ii], "-c")) {
                ii++;
                if (ii < argc && vm_args.nOptions < MAX_VM_OPTS) {
                    options[vm_args.nOptions].optionString =
                        Strdup("-Djava.class.path=");
                    Strapp(options[vm_args.nOptions].optionString, argv[ii]);
                    vm_args.nOptions += 1;
                }
            } else if (!strcmp(argv[ii], "-p")) {
                global_opt_pause = 1;
            } else if (!strcmp(argv[ii], "-v")) {
                global_opt_verbose = 1;

                if (vm_args.nOptions < MAX_VM_OPTS) {
                    options[vm_args.nOptions].optionString =
                        Strdup("-verbose:class");
                    vm_args.nOptions += 1;
                }

                if (vm_args.nOptions < MAX_VM_OPTS) {
                    options[vm_args.nOptions].optionString =
                        Strdup("-verbose:jni");
                    vm_args.nOptions += 1;
                }

                if (vm_args.nOptions < MAX_VM_OPTS) {
                    options[vm_args.nOptions].optionString =
                        Strdup("-verbose:gc");
                    vm_args.nOptions += 1;
                }
            } else {
                printf("Usage: %s [-c path] [-p] [-v] [class] [...]\n",
                        argv[0]);
            }
        }
        ii++;
    }

    rtn = init_java(errmsg, sizeof(errmsg));
    if (rtn) {
        printf("Error initializing JVM: %s\n", errmsg);
        return (rtn);
    }

    for (ii = 0; ii < vm_args.nOptions; ii++) {
        printf("vm_args.options[%d]=%s\n",
            ii, options[ii].optionString);
    }

    status = JNI_CREATEJAVAVM(&jvm, (void**)&env, &vm_args);

    if (status == JNI_ERR) {
        printf("JNI_CreateJavaVM() failed with %d\n", (int)status);
    } else {
        printf("Successfully started JVM with JNI_CreateJavaVM()\n");
        jvm_version = (*env)->GetVersion(env);
        if (jvm_version < 0) {
            printf("JNI call to GetVersion() failed with %d\n", jvm_version);
        } else {
            printf("JNI version %d.%d\n",
                jvm_version >> 16, jvm_version & 0xFFFF);
            rtn = test_jstring(env, errmsg, sizeof(errmsg));
            if (rtn) {
                printf("String test failed: %s\n", errmsg);
            } else {
                printf("String test passed.\n");
            }

            if (!rtn) {
                ii = 1;
                while (ii < argc) {
                    if (argv[ii][0] == '-') {
                        if (!strcmp(argv[ii], "-c")) ii++;
                    } else {
                        fcrtn = test_class(env, argv[ii]);
                        if (fcrtn) rtn += 1;
                    }
                    ii++;
                }
            }

        }
        (*jvm)->DestroyJavaVM(jvm);
    }

    return (rtn);
}
/*************************************************************/
int parse_global_args(int argc, char *argv[])
{
    int rtn;
    int ii;

    rtn = 0;
    ii = 1;
    while (ii < argc) {
        if (argv[ii][0] == '-') {
            if (!strcmp(argv[ii], "-p")) {
                global_opt_pause = 1;
            } else if (!strcmp(argv[ii], "-v")) {
                global_opt_verbose = 1;
            }
        }
        ii++;
    }

    return (rtn);
}
/*************************************************************/
int parse_jvm_args(int argc, char *argv[], struct djenv * dje)
{
    int rtn;
    int ii;

    rtn = 0;
    ii = 1;
    while (ii < argc) {
        if (argv[ii][0] == '-') {
            if (!strcmp(argv[ii], "-c")) {
                ii++;
                if (ii < argc) {
                    dejaEnvAddClassPath(dje, argv[ii]);
                }
            } else if (!strcmp(argv[ii], "-t")) {
                ii++;
                if (ii < argc) {
                    dejaSetTraceFile(dje, argv[ii]);
                }
            } else if (!strcmp(argv[ii], "-x")) {
                dejaSetDebug(dje, 64);
            } else if (!strcmp(argv[ii], "-h")) {
                dejaSetFlags(dje, DJEFLAG_ENABLE_HEAP_SIZE);
            }
        }
        ii++;
    }

    return (rtn);
}
/***************************************************************/
static int set_dj_err (struct djenv * dje, int djerr) {

    int rtn;
    char * djmsg;

    djmsg = dejaGetErrorMsg(dje, djerr);
    rtn = set_error("Deja error: %s\n", djmsg);
    dejaReleaseString(dje, djmsg);

    return (rtn);
}
/*************************************************************/
int test_setup_first_read(struct djenv * dje,
    int start_index,
    int * has_next)
{
    int djerr;

    djerr = 0;
    if (start_index) {
        djerr = dejaCallVoidMethod(dje,
            "gaDataQuery", "setStartIndex", "i", start_index);
    }
    if (!djerr) {
        djerr = dejaNewNullObject(dje,
            "gaDataFeed", "com.google.gdata.data.analytics.DataFeed");
    }

    if (!djerr) {
        djerr = dejaCallObjectMethod(dje,
                "(com.google.gdata.data.IFeed)gaDataFeed",
                "gaService", "getFeed",
                "vz", "(com.google.gdata.client.Query)gaDataQuery",
                      "com.google.gdata.data.analytics.DataFeed");
    }

    if (!djerr) {
        djerr = dejaNewNullObject(dje, "gaDataFeedList",
                            "java.util.List");
    }

    if (!djerr) {
        djerr = dejaNewNullObject(dje, "gaDataFeedIterator",
                            "java.util.Iterator");
    }

    if (!djerr) {
        djerr = dejaNewNullObject(dje, "gaEntry",
                            "com.google.gdata.data.analytics.DataEntry");
    }

    if (!djerr) {
        djerr = dejaCallObjectMethod(dje, "gaDataFeedList",
                        "gaDataFeed", "getEntries", "");
    }

    if (!djerr) {
        djerr = dejaCallObjectMethod(dje, "gaDataFeedIterator",
                        "gaDataFeedList", "iterator", "");
    }

    (*has_next) = 0;
    if (!djerr) {
        djerr = dejaCallBooleanMethod(dje, has_next,
            "gaDataFeedIterator", "hasNext", "");
    }

    return (djerr);
}
/*************************************************************/
int test_setup_reread(struct djenv * dje,
    int start_index,
    int * has_next)
{
    int djerr;

    djerr = 0;

#if 0
    if (!djerr) {
        djerr = dejaDeleteVarObject(dje, "gaDataFeed");
    }

    if (!djerr) {
        djerr = dejaDeleteVarObject(dje, "gaDataFeedList");
    }

    if (!djerr) {
        djerr = dejaDeleteVarObject(dje, "gaDataFeedIterator");
    }
#endif

    if (start_index) {
        djerr = dejaCallVoidMethod(dje,
                "gaDataQuery", "setStartIndex", "i", start_index);
    }

    if (!djerr) {
        djerr = dejaCallObjectMethod(dje,
                "(com.google.gdata.data.IFeed)gaDataFeed",
                "gaService", "getFeed",
                "vz", "(com.google.gdata.client.Query)gaDataQuery",
                      "com.google.gdata.data.analytics.DataFeed");
    }

    if (!djerr) {
        djerr = dejaCallObjectMethod(dje, "gaDataFeedList",
                        "gaDataFeed", "getEntries", "");
    }

    if (!djerr) {
        djerr = dejaCallObjectMethod(dje, "gaDataFeedIterator",
                        "gaDataFeedList", "iterator", "");
    }

    (*has_next) = 0;
    if (!djerr) {
        djerr = dejaCallBooleanMethod(dje, has_next,
            "gaDataFeedIterator", "hasNext", "");
    }

    return (djerr);
}
/*************************************************************/
int prompt_continue(const char * msg)
{
    int rtn;
    char ans[64];

    rtn = 0;
    fprintf(stdout, "%s", msg);
    fgets(ans, sizeof(ans), stdin);
    if (ans[0] == 'n' || ans[0] == 'N') rtn = 1;

    return (rtn);
}

/*************************************************************/
int show_line(int record_number, int memmb, int jtotmemmb, int jfreememmb, const char * outln)
{
    int rtn;
    static int prev_memmb = 0;

    rtn = 0;

    fprintf(stdout, "%6d %4d %4d %4d %3d %-52.52s\n", record_number, memmb,
        jtotmemmb, jfreememmb, jtotmemmb - jfreememmb, outln);
    if (memmb > prev_memmb && memmb > 1024) {
        rtn = prompt_continue("Keep processing...");
    }
    prev_memmb = memmb;

    return (rtn);
}
/*************************************************************/
int test_show_line(struct djenv * dje, int record_number, int show_rate, const char * outln)
{
    int rtn;
    int memmb;
    int jtotmemmb;
    int64 jtotmem;
    int jfreememmb;
    int64 jfreemem;

    rtn = 0;

    if (record_number % show_rate == 0) {
        memmb = os_get_process_memory_megabytes();
        rtn = dejaGetHeapSize(dje, DEJAHEAP_FLAG_RUN_GC | DEJAHEAP_FLAG_TOTAL_MEMORY, &jtotmem);
        if (!rtn) {
            jtotmemmb = (int)(jtotmem / (1024 * 1024));
            rtn = dejaGetHeapSize(dje, DEJAHEAP_FLAG_FREE_MEMORY, &jfreemem);
        }
        if (!rtn) {
            jfreememmb = (int)(jfreemem / (1024 * 1024));
            rtn = show_line(record_number, memmb, jtotmemmb, jfreememmb, outln);
        }
    }

    return (rtn);
}
/*************************************************************/
int lw_init_service(struct djenv * dje)
{
    int djerr;
    char user[64];
    char pass[64];

    djerr = 0;
    strcpy(user, "vasalt@gmail.com");
    strcpy(pass, "h1ssfish");

    //// Open database
    if (!djerr) {
        djerr = dejaNewObject(dje, "gaService",
                    "com.google.gdata.client.analytics.AnalyticsService",
                    "s", "taurus-wh-1");
    }

    if (!djerr) {
        djerr = dejaCallVoidMethod(dje, "gaService", "setUserCredentials",
                    "ss", user, pass);
    }

    if (!djerr) {
        djerr = dejaNewObject(dje, "gaURL", "java.net.URL",
                     "s", "https://www.google.com/analytics/feeds/data");
    }

    return (djerr);
}
/*************************************************************/
int lw_begin_read(struct djenv * dje, const char * dims, const char * mets, int max_results)
{
    int djerr;
    int new_max_results;
    char gaid[64];
    char stdt[64];
    char endt[64];

    djerr = 0;

    strcpy(gaid, "ga:2024916");

    strcpy(stdt, "2014-07-01");
    strcpy(endt, "2015-01-31");

    //// Begin read
    if (!djerr) {
        djerr = dejaPushFrame(dje, "gaService", "gaURL", NULL);
    }

    if (!djerr) {
        djerr = dejaNewObject(dje, "gaDataQuery",
                "com.google.gdata.client.analytics.DataQuery",
                "v", "gaURL");
    }

    if (!djerr) {
        djerr = dejaCallVoidMethod(dje, "gaDataQuery", "setIds",
            "s", gaid);
    }

    if (!djerr) {
        djerr = dejaCallVoidMethod(dje,
                                "gaDataQuery", "setStartDate",
                                "s", stdt);
    }

    if (!djerr) {
        djerr = dejaCallVoidMethod(dje,
                                "gaDataQuery", "setEndDate",
                                "s", endt);
    }

    if (!djerr) {
        djerr = dejaCallVoidMethod(dje,
            "gaDataQuery", "setDimensions", "s", dims);
    }

    if (!djerr) {
        djerr = dejaCallVoidMethod(dje,
            "gaDataQuery", "setMetrics", "s", mets);
    }

    if (!djerr) {
        if (max_results) {
            djerr = dejaCallVoidMethod(dje,
                "gaDataQuery", "setMaxResults", "i", max_results);

            if (!djerr) {
                djerr = dejaCallIntMethod(dje, &new_max_results,
                    "gaDataQuery", "getMaxResults", "");
            }
            if (!djerr) {
                if (new_max_results != max_results) {
                    fprintf(stdout, "Max results does not equal requested. Requested=%d, Actual=%d\n",
                        max_results, new_max_results);
                    max_results = new_max_results;
                }
            }
        }
    }

    return (djerr);
}
/*************************************************************/
int test_Legendary_Whitetails(int argc, char *argv[])
{
/*
**  C:\D\Wh\Current\WhIde\Google 1410\lib\gdata-analytics-2.1.jar
**  C:\D\Wh\Current\WhIde\Google 1410\guava-18.0.jar
*/
    int rtn;
    int djerr;
    int max_results;
    int start_index;
    int has_next;
    int show_data;
    int fix;
    int ix;
    int vix;
    int outix;
    int done;
    int record_count;
    int record_limit;
    int batch_count;
    int show_rate;
    struct djenv * dje;
    char * field_value;
    char field_name[64];
    char mets[256];
    char dims[256];
    char outln[256];

    printf("Enter test_Legendary_Whitetails()\n");
    dje = dejaNewEnv();

    max_results  = 10;
    record_limit = 20;
    show_rate    = 1;

    show_data = 1;

    rtn = parse_jvm_args(argc, argv, dje);
    if (rtn) return (rtn);

    djerr = dejaEnvStartJVM(dje);

    if (!djerr) {
        djerr = lw_init_service(dje);
    }

    outix = 0;
    record_count = 0;
    start_index = 0;
    done = 0;

    strcpy(mets, "ga:sessions,ga:users,ga:bounces,ga:sessionDuration");
    strcpy(dims, "ga:date,ga:hostname,ga:country,ga:region,ga:city,ga:userType");

    if (!djerr) {
        djerr = lw_begin_read(dje, dims, mets, max_results);
    }

    djerr = test_setup_first_read(dje, start_index, &has_next);
    if (!djerr && !has_next) done = 1;

    while (!djerr && !done) {
        batch_count = max_results;
        while (!djerr && !done && has_next) {
            if (!djerr) {
                djerr = dejaCallObjectMethod(dje, "(java.lang.Object)gaEntry",
                                    "gaDataFeedIterator", "next", "");
            }
            if (show_data) {
                if (!djerr) {
                    // dims
                    ix = 0;
                    while (!djerr && dims[ix]) {
                        fix = 0;
                        while (dims[ix] && dims[ix] != ',') field_name[fix++] = dims[ix++];
                        field_name[fix] = '\0';
                        djerr = dejaCallStringMethod(dje, &(field_value),
                            "gaEntry", "stringValueOf",
                                    "s", field_name);
                        if (dims[ix]) ix++;

                        if (outix) outln[outix++] = ' ';
                        vix = 0;
                        while (field_value[vix]) outln[outix++] = field_value[vix++];
                        dejaReleaseString(dje, field_value);
                    }

                    // mets
                    ix = 0;
                    while (!djerr && mets[ix]) {
                        fix = 0;
                        while (mets[ix] && mets[ix] != ',') field_name[fix++] = mets[ix++];
                        field_name[fix] = '\0';
                        djerr = dejaCallStringMethod(dje, &(field_value),
                            "gaEntry", "stringValueOf",
                                    "s", field_name);
                        if (mets[ix]) ix++;

                        if (outix) outln[outix++] = ' ';
                        vix = 0;
                        while (field_value[vix]) outln[outix++] = field_value[vix++];
                        dejaReleaseString(dje, field_value);
                    }
                }
            } else {    /* !show_data */
                strcpy(field_name, "<no data shown>");
                vix = 0;
                while (field_name[vix]) outln[outix++] = field_name[vix++];
            }

            if (!djerr) {
                record_count++;
                batch_count--;

                outln[outix] = '\0';
                done = test_show_line(dje, record_count, show_rate, outln);
                outix = 0;
            }

            if (!djerr) {
                djerr = dejaDeleteVarObject(dje, "gaEntry");
            }

            if (!djerr && !done) {
                if (!record_limit || record_count < record_limit) {
                    djerr = dejaCallBooleanMethod(dje, &has_next,
                        "gaDataFeedIterator", "hasNext", "");
                } else {
                    has_next = 0;
                }
            }
        }

        if (!djerr && !done) {
            if (!max_results || batch_count > 0 || (record_limit && record_count >= record_limit)) {
                done = 1;
            } else {
                if (!djerr) {
                    start_index = record_count;
                    djerr = test_setup_reread(dje, start_index, &has_next);
                    if (!djerr && !has_next) done = 1;
                }
            }
        }
    }
    //// End read
    if (!djerr) {
        fprintf(stdout, "records read: %d \n", record_count);

        djerr = dejaPopFrame(dje);
    }

    //// Done
    if (djerr) {
        rtn = set_dj_err(dje, djerr);
    }

    //// Close
    dejaFreeEnv(dje);

    return (rtn);
}
/*************************************************************/
int main(int argc, char *argv[])
{
/*
    -p -t tracFil.txt -c "C:\D\Wh\Current\WhIde\Google 1410\lib\gdata-analytics-2.1.jar" -c "C:\D\Wh\Current\WhIde\Google 1410\guava-18.0.jar"
*/
    int rtn;

    printf("JNICTest sizeof(void*)=%d\n", sizeof(void*));

    rtn = parse_global_args(argc, argv);
    if (!rtn) {
        rtn = test_Legendary_Whitetails(argc, argv);
    }

    if (!rtn) {
        printf("All tests passed.\n");
    }
    print_mem_stats("con");

    if (global_opt_pause) {
        prompt_continue("Continue...");
    }

    exit (rtn);
}
/*************************************************************/
