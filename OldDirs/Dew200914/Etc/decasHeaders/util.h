/***************************************************************/
/*  util.h                                                     */
/***************************************************************/
#ifndef UTILH_INCLUDED
#define UTILH_INCLUDED

#define XSNPFLAG_MINUS      1
#define XSNPFLAG_ZERO       2
#define XSNPFLAG_WIDTH      4
#define XSNPFLAG_MOD_H      8
#define XSNPFLAG_MOD_L      16
#define XSNPFLAG_MOD_LL     32
#define XSNPFLAG_STR_XML    64

#define XSNP_S_FLAG_XML         1
#define XSNP_S_FLAG_IN_UTF8     2
#define XSNP_S_FLAG_IN_WIDE_BE  4
#define XSNP_S_FLAG_IN_WIDE_LE  8
#define XSNP_S_FLAG_OUT_UTF8    16
#define XSNP_S_FLAG_OUT_WIDE_BE 32
#define XSNP_S_FLAG_OUT_WIDE_LE 64
#define XSNP_S_FLAG_JSON        128

void appendbuf_spaces(char ** pbuf,
        int * pbuflen,
        int * pbufmax,
        int nspaces);
void appendbuf_n(char ** pbuf,
        int * pbuflen,
        int * pbufmax,
        const char * addbuf,
        int addlen);
void appendbuf_c(char ** pbuf,
        int * pbuflen,
        int * pbufmax,
        const char * addbuf);
void appendbuf_f(char ** pbuf,
        int * pbuflen,
        int * pbufmax,
        const char * fmt, ...);

#define HLOG_FLAG_USE_ADDR          1
#define HLOG_FLAG_MSG_EVERY_LINE    2

void hlog(FILE * outf,
             int logflags,
             const void * vldata,
             int ldatalen,
             const char * fmt, ...);


/***************************************************************/
#endif /* UTILH_INCLUDED */
/***************************************************************/
