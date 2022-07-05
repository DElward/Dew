/***************************************************************/
/* desh.c                                                      */
/***************************************************************/

#include "config.h"

#if IS_WINDOWS
    #if IS_64_BIT
        #define DESH_PLATFORM  "X"
    #else
        #define DESH_PLATFORM  "W"
    #endif
#elif IS_LINUX
    #if IS_64_BIT
        #define DESH_PLATFORM  "K"
    #else
        #define DESH_PLATFORM  "L"
    #endif
#elif IS_MACINTOSH
    #define DESH_PLATFORM  "M"
#else
    #define DESH_PLATFORM  "Z"
#endif

#define DESH_VERSNUM    "01"
#define DESH_VERSION    DESH_PLATFORM DESH_VERSNUM
#define DESH_VFIX       "0019"
#define DESH_YEAR       "2014"
#define DESH_COMPANY    "Taurus Software, Inc."
#define DESH_PROGRAM    "DESH"

#if IS_DEBUG
    #define DESH_FIX        DESH_VFIX "D"
#else
    #define DESH_FIX        DESH_VFIX
#endif

/*
**
** Done
** 03/06/12 Started work
** 12/30/13 Release 1.0000 (Windows-32 only)
**
**              Exit code:  0 - success
**                          1 - Error 
**
****************************************************************
**
** http://en.wikipedia.org/wiki/Comparison_of_command_shells
**
**  To do:
**
** Fix
**      . How do you do this on UniX: echo [$V] ?   Output: [vval]
**      . What about: unset?
**      . What about: dir -s *.xml ?
**      . Remove unused #ifdef code
**      . Tests for fully qualified program names.
**
** History
**     Date New version
**     02/12/14 01.0001 - Fixed pattern matching where "file" did not match "file*"
**     02/12/14 01.0001 - Added DESHVERSION built in variable in format of PVVFFFF
**     02/17/14 01.0001 - CALL and dot(.) search PATH, then .
**     02/18/14 01.0001 - CD updates PWD
**     02/20/14 01.0001 - Added SLEEP function
**     02/20/14 01.0001 - call always seaches . dot only searches PATH
**     03/07/14 01.0001 - Fixed Windows shows p on: dir -x
**     03/07/14 01.0001 - Fixed bug with help -s
**     03/16/14 01.0004 - Zero length strings not number or boolean
**     04/01/14 01.0006 - XFIX: Qualified commands would not run
**     04/01/14 01.0006 - XFIX: "cd" with no parameters goes to home dir
**     04/01/14 01.0007 - Fixed command substitution in DESHPROMPT and <<
**     06/20/14 01.0009 - Added error ECMDARGFAILED
**     07/07/14 01.0017 - Fixed bug in free_nest_mess()
**     07/08/14 01.0018 - Fixed bug with dir -b
**     07/08/14 01.0019 - Fixed bug in init_str_char()
**
** Issues
**      . Check redirection on for ... in...  What should happen?
**      . What about DESHPATH and DESHPATHEXT for call, dot(.), desh.ini and after PATH?
**      . What about DESHCATCH, a command to catch errors?
**      . Erase "Continue?" in help
**      . Implement eval command
**      . What about cmd (parm) or (cmd parms) ?
**      . Make CEFLAG_VERBOSE and CEFLAG_DEBUG good
**      . What about command file with bad last line and no crlf?
**      . Fileset parm with no matches returns empty parm.  Is this correct?
**      . Remember where endwhile and endfor are for optimization
**      . Implement check for DESHOPTS variable at start.
**      . Should -w flag also set gg->gg_path_delim ?  What about $/ ?
**      . Bit operators BNOT, BAND, BOR, BXOR ?
**      . Test/document variables in init_required_variables(), move to find_globalvar_ex()
**      . Should '[' be considered wild in get_parmstr()
**      . Fix lengths on all calls to init_str()
**      . Investigate -x flag, CEFLAG_EXIT
**      . Maybe it would be quicker if get_parmstr() took a char* instead of a struct str
**      . Allow functions to be exported?
**      . Built-in functions: <ans> ::= PINFO(<str>, <typstr>)        <-- Process info
**      . for functions:      for <var> procs(<dirnam>)
**      . What about kill?
**      . What about touch of Windows?
**      . Should * sort on X?  Example: dir *.txt does not come out sorted.
**
** Enhancements:
**      . Command grouping with ( and )
**      . Make MOVE recursive to move into a directory
**      . Make COPY recursive to copy a directory
**      . Enhance PROMPT to <ans> ::= PROMPT(<str> [ , <num chars> [ , <timeout> ] ] )
**      . Built-in function:  <ans> ::= ISDATE(<datstr> , <fmtstr> )
**      . Built-in function:  <ans> ::= ISINT(<numstr>)
**      . Built-in function:  <ans> ::= ISBOOL(<boolstr>)
**      . Built-in function:  <ans> ::= FMTINT(<int>, <fmtstr>)
**      . Add shopt -b to disable ctl-c.  -b should be on during desh.ini processing
**
**  Done:
**      . Tests for wait.
**
********************************
**  
**  ASSOC .dsh=deshScript
**  FTYPE deshScript="C:\D\Bin\desh.exe" "%1" %*
*/

#if IS_WINDOWS
#include <io.h>
#if IS_DEBUG
    #include <CRTDBG.H>
#endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>

#include "desh.h"
#include "var.h"
#include "snprfh.h"
#include "exp.h"
#include "msg.h"
#include "cmd.h"
#include "cmd2.h"
#include "when.h"
#include "assoc.h"
#include "term.h"
#include "sysx.h"
#include "help.h"

#define DEBUG_SHOW_SAVE     0

/* Globals */
static int global_flags         = 0;
int global_user_break           = 0; /* ctl-c pressed */
/* static char * global_argv0      = NULL; */  /* original argv[0] */
int g_show_signals = 0;
/********/

/***************************************************************/
/*@*/int Stricmp(const char * b1, const char * b2) {
    /*
     ** Case insensitive strcmp
     */
    int ii;

    for (ii = 0; b1[ii]; ii++) {
        if (tolower(b1[ii]) < tolower(b2[ii])) return (-1);
        else if (tolower(b1[ii]) > tolower(b2[ii])) return (1);
    }
    if (b2[ii]) return (-1);
    else        return (0);
}
/***************************************************************/
/*@*/int Memicmp(const char * b1, const char * b2, size_t blen) {
    /*
     ** Case insensitive memcmp
     */
    size_t ii;

    for (ii = 0; ii < blen; ii++) {
        if (tolower(b1[ii]) < tolower(b2[ii])) return (-1);
        else if (tolower(b1[ii]) > tolower(b2[ii])) return (1);
    }
    return (0);
}
/***************************************************************/
void init_str(struct str * ss, int smax) {

    if (smax) {
        ss->str_max = smax;
        ss->str_buf = New(char, ss->str_max);
        ss->str_buf[0] = '\0';
        ss->str_len = 0;
    } else {
        ss->str_len = 0;
        ss->str_max = 0;
        ss->str_buf = NULL;
    }
}
/***************************************************************/
void init_str_char(struct str * ss, const char * buf) {

    int blen;

    if (buf) {
        blen = Istrlen(buf);
        ss->str_max = blen + 1;
        ss->str_buf = New(char, ss->str_max); /* Fixed 7/8/2014 */
        ss->str_len = blen;
        memcpy(ss->str_buf, buf, blen + 1);
    } else {
        ss->str_len = 0;
        ss->str_max = 0;
        ss->str_buf = NULL;
    }
}
/***************************************************************/
void free_str_buf(struct str * ss) {

    Free(ss->str_buf);
    ss->str_len = 0;
    ss->str_max = 0;
    ss->str_buf = NULL;
}
/***************************************************************/
void free_charlist(char ** charlist) {

    int ii;

    if (charlist) {
        for (ii = 0; charlist[ii]; ii++) Free(charlist[ii]);
        Free(charlist);
    }
}
/***************************************************************/
void append_charlist(char *** p_charlist, char * buf, int * cllen, int * clmax) {

    if ((*cllen) + 1 >= (*clmax)) {
        if (!(*clmax)) (*clmax) = CHARLIST_CHUNK;
        else           (*clmax) *= 2;
        (*p_charlist) = Realloc((*p_charlist), char*, (*clmax));
    }

    (*p_charlist)[(*cllen)] = buf;
    (*cllen) += 1;
    (*p_charlist)[(*cllen)] = NULL;
}
/***************************************************************/
/*
void set_str_ch(struct str * ss, char ch) {

    if (!ss->str_max) {
        ss->str_max = STR_CHUNK_SIZE;
        ss->str_buf = New(char, ss->str_max);
    }

    ss->str_len = 1;
    ss->str_buf[0] = ch;
    ss->str_buf[1] = '\0';
}
*/
/***************************************************************/
void append_str_ch(struct str * ss, char ch) {

    if (ss->str_len + 1 >= ss->str_max) {
        ss->str_max += STR_CHUNK_SIZE;
        ss->str_buf = Realloc(ss->str_buf, char, ss->str_max);

    }

    if (ch) {
        ss->str_buf[ss->str_len++] = ch;
    }
    ss->str_buf[ss->str_len]   = '\0';
}
/***************************************************************/
void append_str_len_at(struct str * ss, int ssix, const char * buf, int blen) {

    int newlen;

    if (ssix > ss->str_len) ssix = ss->str_len;

    newlen = ssix + blen;

    if (newlen >= ss->str_max) {
        ss->str_max = newlen + STR_CHUNK_SIZE;
        ss->str_buf = Realloc(ss->str_buf, char, ss->str_max);
    }

    memcpy(ss->str_buf + ssix, buf, blen);
    ss->str_len = newlen;
    ss->str_buf[newlen]   = '\0';
}
/***************************************************************/
void append_str_len(struct str * ss, const char * buf, int blen) {

    if (blen) {
        if (ss->str_len + blen >= ss->str_max) {
            ss->str_max += blen + STR_CHUNK_SIZE;
            ss->str_buf = Realloc(ss->str_buf, char, ss->str_max);
        }

        memcpy(ss->str_buf + ss->str_len, buf, blen);
        ss->str_len += blen;
    }
}
/***************************************************************/
void append_str_char(struct str * ss, const char * buf) {

    append_str_len(ss, buf, Istrlen(buf));
}
/***************************************************************/
void append_str_str(struct str * tgt, struct str * src) {

    append_str_len(tgt, src->str_buf, src->str_len + 1);
    tgt->str_len -= 1; /* remove null at end */
}
/***************************************************************/
void append_str_len_upper(struct str * ss, const char * buf, int blen) {

    int ii;
    char * sbuf;

    if (ss->str_len + blen >= ss->str_max) {
        ss->str_max += blen + STR_CHUNK_SIZE;
        ss->str_buf = Realloc(ss->str_buf, char, ss->str_max);
    }

    sbuf = ss->str_buf + ss->str_len;
    for (ii = 0; ii < blen; ii++) *sbuf++ = tolower(buf[ii]);
    ss->str_len += blen;
}
/***************************************************************/
void append_str(struct str * ss, const char * buf) {

    append_str_len(ss, buf, Istrlen(buf) + 1);
    ss->str_len -= 1; /* remove null at end */
}
/***************************************************************/
void set_str_len(struct str * ss, const char * buf, int blen) {

    if (blen + 1 >= ss->str_max) {
        ss->str_max += blen + 16;
        ss->str_buf = Realloc(ss->str_buf, char, ss->str_max);

    }

    memcpy(ss->str_buf, buf, blen);
    ss->str_len = blen;
    ss->str_buf[blen] = '\0';
}
/***************************************************************/
void set_str_char(struct str * ss, const char * buf) {

    set_str_len(ss, buf, Istrlen(buf));
}
/***************************************************************/
void set_str_char_f(struct str * ss, char *fmt, ...) {

    va_list args;
    char * emsg;

    va_start (args, fmt);
    emsg = Vsmprintf (fmt, args);
    va_end (args);

    set_str_len(ss, emsg, Istrlen(emsg));
    Free(emsg);
}
/***************************************************************/
struct funclib * new_funclib(void) {

    struct funclib * fl;

    fl = New(struct funclib, 1);

    fl->fl_funcs = dbtree_new();

    return (fl);
}
/***************************************************************/
void free_usrfuncinfo(void * vufi) {

    struct usrfuncinfo * ufi = (struct usrfuncinfo *)vufi;
    int ii;

    for (ii = 0; ii < ufi->ufi_nparms; ii++) {
        free_str_buf(ufi->ufi_sparmlist[ii]);
        Free(ufi->ufi_sparmlist[ii]);
    }
    Free(ufi->ufi_sparmlist);
    Free(ufi->ufi_func_name);

    if (ufi->ufi_stmt_list) {
		for (ii = 0; ufi->ufi_stmt_list[ii]; ii++) {
			Free(ufi->ufi_stmt_list[ii]);
		}
		Free(ufi->ufi_stmt_list);
	}

    Free(ufi);
}
/***************************************************************/
void free_usrfuncs(struct dbtree * dbt) {

    dbtree_free(dbt, free_usrfuncinfo);
}
/***************************************************************/
void free_funclib(struct funclib * fl) {

    if (fl->fl_funcs) dbtree_free(fl->fl_funcs, free_usrfuncinfo);

    Free(fl);
}
/***************************************************************/
struct nest * new_nest(struct cenv * ce, int netyp) {

    struct nest * ne;

    ne = New(struct nest, 1);

    ne->ne_typ      = netyp;
    ne->ne_vnam     = NULL;
    ne->ne_vnlen    = 0;
    ne->ne_vnmax    = 0;
    ne->ne_idle     = 0;
    ne->ne_flags    = 0;
    ne->ne_ix       = 0;
    ne->ne_data     = NULL;
    ne->ne_ufi      = NULL;
    ne->ne_loopback = 0;
    ne->ne_is_saved = 0;
    ne->ne_prev = ce->ce_nest;


    if (ne->ne_prev) {
        ne->ne_loopback = ne->ne_prev->ne_loopback;
        ne->ne_is_saved = ne->ne_prev->ne_is_saved;
        ne->ne_flags    = ne->ne_prev->ne_flags;
        ne->ne_prev->ne_saved_stmt_pc  = ce->ce_stmt_pc;
        ne->ne_prev->ne_saved_stmt_len = ce->ce_stmt_len;
    }

    ce->ce_nest = ne;

    return (ne);
}
/***************************************************************/
void free_for_data(struct nest * ne) {

    switch (ne->ne_for_typ) {
        case NEFORTYP_CSVPARSE :
            free_forparserec((struct forparserec *)ne->ne_data);
            break;
        case NEFORTYP_IN :
            free_charlist((char**)ne->ne_data);
            break;
        case NEFORTYP_PARSE :
            free_forparserec((struct forparserec *)ne->ne_data);
            break;
        case NEFORTYP_READ :
            free_forreadrec((struct forreadrec *)ne->ne_data);
            break;
        case NEFORTYP_READDIR :
            free_forreaddirrec((struct forreaddirrec *)ne->ne_data);
            break;
        default:
            break;
    }
}
/***************************************************************/
void free_nest(struct cenv * ce, struct nest * ne) {

    ce->ce_nest = ne->ne_prev;

    if (ne->ne_typ) {
        free_for_data(ne);
    }
    if (ce->ce_nest && ne->ne_idle == NEIDLE_ERROR) ce->ce_nest->ne_idle = NEIDLE_ERROR;

    Free(ne->ne_vnam);
    Free(ne);
}
/***************************************************************/
void free_nest_list(struct cenv * ce) {

    while (ce->ce_nest) free_nest(ce, ce->ce_nest);
}
/***************************************************************/
void new_errrec(struct glob * gg) {

    struct errrec * er;

    er = New(struct errrec, 1);

    er->er_in_use       = 0;
    er->er_next_avail   = gg->gg_next_avail_errrec;
    er->er_next_errrec_index  = 0;
    er->er_errnum       = 0;
    er->er_index        = gg->gg_max_errrec + 1;
    er->er_errmsg[0]    = '\0';

    gg->gg_errrec = Realloc(gg->gg_errrec, struct errrec *, gg->gg_max_errrec + 1);
    gg->gg_errrec[gg->gg_max_errrec] = er;

    gg->gg_next_avail_errrec = gg->gg_max_errrec;

    gg->gg_max_errrec += 1;
}
/***************************************************************/
struct errrec * get_avail_errrec(struct glob * gg) {

    struct errrec * er;
    int erix;

    if (gg->gg_next_avail_errrec < 0) {
        erix = gg->gg_max_errrec;
        new_errrec(gg);
    } else {
        erix = gg->gg_next_avail_errrec;
    }

    er = gg->gg_errrec[erix];
    gg->gg_next_avail_errrec = er->er_next_avail;
    er->er_next_avail = -1;

#if IS_DEBUG
    if (er->er_in_use) {
        fprintf(stdout, "*** Internal error: Error record already in use ***\n");
    }
#endif

    er->er_in_use = 1;

    return (er);
}
/***************************************************************/
void release_errrec(struct glob * gg, int errrec_index) {

    struct errrec * er;
    int erix;

    erix = errrec_index - 1;

#if IS_DEBUG
    if (erix < 0 || erix >= gg->gg_max_errrec) {
        fprintf(stdout, "*** Internal error: Error index (%d) out of range ***\n", errrec_index);
        return;
    }
#endif

    er = gg->gg_errrec[erix];

#if IS_DEBUG
    if (!er->er_in_use) {
        fprintf(stdout, "*** Internal error: Error record not in use ***\n");
        return;
    }
#endif

    er->er_next_avail = gg->gg_next_avail_errrec;
    gg->gg_next_avail_errrec = erix;

    er->er_in_use       = 0;
    er->er_errnum       = 0;
    er->er_errmsg[0]    = '\0';

    if (er->er_next_errrec_index) {
        release_errrec(gg, er->er_next_errrec_index);
        er->er_next_errrec_index = 0;
    }
}
/***************************************************************/
struct errrec * get_errrec(struct glob * gg, int errrec_index) {

    struct errrec * er;
    int erix;

    erix = errrec_index - 1;

#if IS_DEBUG
    if (erix < 0 || erix >= gg->gg_max_errrec) {
        fprintf(stdout, "*** Internal error: Error index (%d) out of range ***\n", errrec_index);
        return (NULL);
    }
#endif

    er = gg->gg_errrec[erix];

#if IS_DEBUG
    if (!er->er_in_use) {
        fprintf(stdout, "*** Internal error: Error record not in use ***\n");
        return (NULL);
    }
#endif

    return (er);
}
/***************************************************************/
void free_errrecs(struct glob * gg) {

    int ii;

    for (ii = 0; ii < gg->gg_max_errrec; ii++) Free(gg->gg_errrec[ii]);
    Free(gg->gg_errrec);
}
/***************************************************************/
struct glob * new_glob(char *envp[]) {

    struct glob * gg;
    char ** lenvp;

    glob_init_time();

    gg = New(struct glob, 1);

    gg->gg_cmds         = dbtree_new();
    gg->gg_idle_cmds    = dbtree_new();
    gg->gg_extinf       = NULL;
    gg->gg_filref       = NULL;
    gg->gg_bi_funcs     = NULL;
    gg->gg_path_delim   = DEFAULT_PATH_DELIM;

    gg->gg_genv = load_envp(envp, &lenvp, &(gg->gg_nenv), UPSHIFT_GLOBAL_VARNAMES);
    gg->gg_envp = lenvp;
#if IS_DEBUG
    gg->gg_mem_fname = NULL;
#endif

    gg->gg_next_avail_errrec  = -1;
    gg->gg_max_errrec   = 0;
    gg->gg_errrec       = NULL;

    new_errrec(gg);
    new_errrec(gg);

    set_time_globals(gg);

    return (gg);
}
/***************************************************************/
void free_cmdinfo(void * vci) {

    struct cmdinfo * ci = (struct cmdinfo *)vci;

    Free(ci->ci_cmdpath);
    Free(ci);
}
/***************************************************************/
void free_extinf(void * vxi) {

    struct extinf * xi = (struct extinf *)vxi;

    Free(xi->xi_cmdpath);
    Free(xi);
}
/***************************************************************/
void free_filref(void * vfr) {

    struct filref * fr = (struct filref *)vfr;

    Free(fr->fr_cmdpath);
    Free(fr);
}
/***************************************************************/
void free_bifuncinfo(void * vbfi) {

    struct bifuncinfo * bfi = (struct bifuncinfo *)vbfi;

    Free(bfi);
}
/***************************************************************/
void free_env(char ** argv) {

    int ii;

    for (ii = 0; argv[ii]; ii++) Free(argv[ii]);
    Free(argv);
}
/***************************************************************/
void free_glob(struct glob * gg) {

    dbtree_free(gg->gg_cmds, free_cmdinfo);
    dbtree_free(gg->gg_idle_cmds, free_cmdinfo);
    if (gg->gg_bi_funcs) dbtree_free(gg->gg_bi_funcs, free_bifuncinfo);
    dbtree_free(gg->gg_genv, NULL);

#if IS_DEBUG
    if (gg->gg_mem_fname) Free(gg->gg_mem_fname);
#endif

    free_env(gg->gg_envp);

    if (gg->gg_extinf) dbtree_free(gg->gg_extinf, free_extinf);
    if (gg->gg_filref) dbtree_free(gg->gg_filref, free_filref);

    free_errrecs(gg);
    Free(gg);
}
/***************************************************************/
void set_filinf_flags(struct filinf * fi, int fiflags) {

    fi->fi_flags           |= fiflags;
}
/***************************************************************/
void free_filinf(struct filinf * fi) {

/*
    if (fi->fi_infname) fprintf(stderr, "free_filinf(%s) fi->fi_tty=0x%08x\n",
                                fi->fi_infname, fi->fi_tty);
    else                fprintf(stderr, "free_filinf() fi->fi_tty=0x%08x\n", fi->fi_tty);
*/

    if (!(fi->fi_flags & FIFLAG_NOCLOSE)) {
if (!fi->fi_fileno) {
printf("stdin issue\n");
}
        CLOSE(fi->fi_fileno); /* do not close stdin */
    }
 
    Free(fi->fi_infname);
    Free(fi->fi_outfname);
    Free(fi->fi_inbuf);
    if (fi->fi_tty) free_term(fi->fi_tty);

    Free(fi);
}
/***************************************************************/
struct filinf * new_filinf(FILET fi_fileno, const char * infname, int inbufsz) {

    struct filinf * fi;

    fi = New(struct filinf, 1);

    fi->fi_fileno           = fi_fileno;
    if (infname) fi->fi_infname          = Strdup(infname);
    else         fi->fi_infname          = NULL;

    fi->fi_flags            = 0;
    fi->fi_eof              = 0;
    fi->fi_stdin_tty        = ISATTY(fi_fileno);
    fi->fi_inbufsz          = inbufsz;
    fi->fi_inbuf            = New(char, fi->fi_inbufsz);
    fi->fi_inbufix          = 0;
    fi->fi_inbuflen         = 0;

    fi->fi_outfname         = NULL;
    fi->fi_tty              = NULL;

    return (fi);
}
/***************************************************************/
int filinf_pair(struct cenv * ce,
                struct filinf * fi,
                FILET fi_outfileno,
                const char * outfname) {

    int estat;
    int term_flags;

    estat = 0;
    term_flags = 0;

    if (outfname)                       fi->fi_outfname  = Strdup(outfname);
    if (ce->ce_flags & CEFLAG_DEBUG)    term_flags      |= TERMFLAG_DEBUG;
    if (ce->ce_flags & CEFLAG_VERBOSE)  term_flags      |= TERMFLAG_VERBOSE;

    if (fi->fi_stdin_tty) {
        fi->fi_tty = new_term(fi->fi_fileno, fi_outfileno, term_flags);
        if (!fi->fi_tty) {
            estat = set_error(ce, ETERMINAL, term_errmsg(NULL, TERRN_INIT));
        }
    }

    return (estat);
}
/***************************************************************/
void free_forparserec(struct forparserec * fpr) {

    if (fpr) {
        Free(fpr->fpr_csvparms);
        free_str_buf(&(fpr->fpr_srcstr));
        free_str_buf(&(fpr->fpr_parmstr));

        Free(fpr);
    }
}
/***************************************************************/
void free_forreadrec(struct forreadrec * frr) {

    if (frr) {
        free_filinf(frr->frr_fi);
        free_str_buf(&(frr->frr_str));

        Free(frr);
    }
}
/***************************************************************/
void free_forreaddirrec(struct forreaddirrec * frdr) {

    if (frdr) {
        close_read_directory(frdr->frdr_rdr);
        free_str_buf(&(frdr->frdr_str));

        Free(frdr);
    }
}
/***************************************************************/
struct forparserec * new_forparserec(const char * srcstr, const char * csvparms) {

    int ii;
    struct forparserec * fpr;

    fpr = New(struct forparserec, 1);
    fpr->fpr_csvparms   = NULL;
    fpr->fpr_ix         = 0;

    init_str(&(fpr->fpr_parmstr), INIT_STR_CHUNK_SIZE);
    init_str_char(&(fpr->fpr_srcstr), srcstr);

    if (csvparms) {
        fpr->fpr_csvparms = New(char, FPR_CSVPARM_SIZE);
        memset(fpr->fpr_csvparms, '\0', FPR_CSVPARM_SIZE);
        for (ii = 0; csvparms[ii]; ii++) {
            fpr->fpr_csvparms[csvparms[ii] & 0x7F] = 1;
        }
    }

    return (fpr);
}
/***************************************************************/
struct forreadrec * new_forreadrec(FILET frr_fileno, const char * infname) {

    struct forreadrec * frr;

    frr = New(struct forreadrec, 1);

    frr->frr_fi = new_filinf(frr_fileno, infname, CMD_LINEMAX);
    init_str(&(frr->frr_str), INIT_STR_CHUNK_SIZE);

    return (frr);
}
/***************************************************************/
struct forreaddirrec * new_forreaddirrec(void * vrdr, const char * infname) {

    struct forreaddirrec * frdr;

    frdr = New(struct forreaddirrec, 1);

    frdr->frdr_rdr = vrdr;
    init_str(&(frdr->frdr_str), INIT_STR_CHUNK_SIZE);

    return (frdr);
}
/***************************************************************/
struct cenv * new_cenv( FILET       stdin_f,
                        FILET       stdout_t,
                        FILET       stderr_f,
                        struct filinf * fi,
                        int         close_flags,
                        int argc,
                        char *argv[],
                        int flags,
                        struct funclib * fl,
                        struct glob * gg,
                        struct cenv * file_ce,
                        struct cenv * prev_ce) {

    struct cenv * ce;

    ce = New(struct cenv, 1);

    ce->ce_stdin_f          = stdin_f;
    ce->ce_stdout_f         = stdout_t;
    ce->ce_stderr_f         = stderr_f;
    ce->ce_filinf           = fi;
    ce->ce_close_flags      = close_flags;

    ce->ce_argc             = argc;
    ce->ce_argv             = argv;
    ce->ce_vars             = NULL;
    ce->ce_flags            = flags;
    ce->ce_funclib          = fl;
    ce->ce_rtn              = 0;
    ce->ce_rtn_var          = NULL;
    ce->ce_nest             = NULL;
    ce->ce_stmt_max         = 0;
    ce->ce_stmt_len         = 0;
    ce->ce_stmt_pc          = 0;
    ce->ce_stmt_list        = NULL;
    ce->ce_errnum           = 0;
    ce->ce_errmsg           = NULL;
    ce->ce_errcmd           = NULL;
    ce->ce_gg               = gg;
    ce->ce_file_ce          = file_ce;
    ce->ce_prev_ce          = prev_ce;
    ce->ce_reading_saved    = 0;
    init_str(&ce->ce_cmd_input_str, 0);

    /* CEFLAG_PAUSE should not be inherited */
    if (ce->ce_flags & CEFLAG_PAUSE) {
        ce->ce_flags ^= CEFLAG_PAUSE;
    }

    return (ce);
}
/***************************************************************/
void free_cenv(struct cenv * curr_ce, struct cenv * ce) {

    int ii;

    if (ce->ce_close_flags & FREECE_STDIN) {
        CLOSE(ce->ce_stdin_f);
    }

    if (ce->ce_close_flags & FREECE_STDOUT) {
        CLOSE(ce->ce_stdout_f);
    }

    if (ce->ce_close_flags & FREECE_STDERR) {
        CLOSE(ce->ce_stderr_f);
    }

    if (ce->ce_close_flags & FREECE_ARGS) {
        for (ii = 0; ii < ce->ce_argc; ii++) Free(ce->ce_argv[ii]);
        Free(ce->ce_argv);
    }

    if (ce->ce_close_flags & FREECE_STMTS) {
        for (ii = 0; ii < ce->ce_stmt_len; ii++) {
            Free(ce->ce_stmt_list[ii]);
        }
        Free(ce->ce_stmt_list);
    }

    if (ce->ce_filinf) free_filinf(ce->ce_filinf);

    if ((ce->ce_close_flags & FREECE_VARS) && ce->ce_vars) {
        unload_vars(ce->ce_vars);
    }
    free_str_buf(&ce->ce_cmd_input_str);

    if (ce->ce_close_flags & FREECE_FUNCS) {
        free_funclib(ce->ce_funclib);
    }

    free_nest_list(ce);

    if (curr_ce) {
        if (ce->ce_errnum) {
            if (curr_ce->ce_errmsg) Free(curr_ce->ce_errmsg);
            if (curr_ce->ce_errcmd) Free(curr_ce->ce_errcmd);
            curr_ce->ce_errmsg = ce->ce_errmsg;
            curr_ce->ce_errcmd = ce->ce_errcmd;
            curr_ce->ce_errnum = ce->ce_errnum;
        }
    } else {
        if (ce->ce_errmsg) Free(ce->ce_errmsg);
        if (ce->ce_errcmd) Free(ce->ce_errcmd);
    }

    if (ce->ce_flags & CEFLAG_PAUSE) {
        char inbuf[100];
        fflush(stdin);
        fflush(stdout);
        fflush(stderr);

        printf("[%d] Press ENTER to continue...", rx_get_pid());
        READ(FILENO(stdin), inbuf, sizeof(inbuf));
    }

    Free(ce);
}
/***************************************************************/
int saving_cmds(struct cenv * ce) {
/*
** Returns 1 if next command should be saved to ce->ce_stmt_list
*/
    int saving;

    saving = 0;
    if (ce->ce_nest) saving = ce->ce_nest->ne_loopback;

    return (saving);
}
/***************************************************************/
int is_saved_cmd(struct cenv * ce) {
/*
** Returns 1 if next command should be read from ce->ce_stmt_list
*/
    int is_saved;

    is_saved = 0;
    if (ce->ce_nest) is_saved = ce->ce_nest->ne_is_saved;

    return (is_saved);
}
/***************************************************************/
void save_cmd(struct cenv * ce,
                    struct str * cmdstr,
                    int * cmdix) {
/*
** Saves command to ce->ce_stmt_list
*/
    int clen;

#if DEBUG_SHOW_SAVE
    printf("--> %d {%s}\n", ce->ce_stmt_len, cmdstr->str_buf);
#endif

    if (ce->ce_stmt_len >= ce->ce_stmt_max) {
        if (!ce->ce_stmt_max) ce->ce_stmt_max = COMMAND_CHUNK;
        else ce->ce_stmt_max *= 2;

        ce->ce_stmt_list = Realloc(ce->ce_stmt_list, char*, ce->ce_stmt_max);
    }

    clen = cmdstr->str_len - (*cmdix) + 1;
    ce->ce_stmt_list[ce->ce_stmt_len]= New(char, clen);
    memcpy(ce->ce_stmt_list[ce->ce_stmt_len], cmdstr->str_buf + (*cmdix), clen);
    ce->ce_stmt_len += 1;
}
/***************************************************************/
char * get_next_saved_cmd(struct cenv * ce) {
/*
** Saves command to ce->ce_stmt_list
*/
    char * cmd;

    if (ce->ce_stmt_pc < ce->ce_stmt_len) {
        cmd = ce->ce_stmt_list[ce->ce_stmt_pc];
#if DEBUG_SHOW_SAVE
    printf("<-- %d {%s}\n", ce->ce_stmt_pc, cmd);
#endif
        ce->ce_stmt_pc += 1;
    } else {
        cmd = NULL;
    }

    return (cmd);
}
/***************************************************************/
void delete_cmds(struct cenv * ce) {
/*
** Deletes entire ce->ce_stmt_list
*/
    int ii;

    for (ii = 0; ii < ce->ce_stmt_len; ii++) {
        Free(ce->ce_stmt_list[ii]);
        ce->ce_stmt_list[ii] = NULL;
    }

    ce->ce_stmt_len         = 0;
    ce->ce_stmt_pc          = 0;
}
/***************************************************************/
void set_cmd_pc(struct cenv * ce, int cpc) {
/*
** Set command to ce->ce_stmt_list
*/
    if (ce->ce_nest) {
        ce->ce_stmt_pc = cpc;
        ce->ce_nest->ne_is_saved = 1;
    }
}
/***************************************************************/
int get_var_str(struct cenv * ce,
                const char * vnam,
                struct str * varstr) {

    int found;
    int vnlen;
    struct var * vval;
    char * gvval;

    found = 0;
    varstr->str_len = 0;
    vnlen = Istrlen(vnam);

    vval = find_var(ce, vnam, vnlen);
    if (vval) {
        found = 1;
        append_str_var(ce, varstr, vval);
    } else {
        gvval = find_globalvar_ex(ce, vnam, vnlen);
        if (gvval) {
            found = 1;
            append_str(varstr, gvval);
        }
    }

    return (found);
}
/***************************************************************/
int get_var_int(struct cenv * ce,
                const char * vnam,
                int * vnum) {
/*
** Returns:
**          -1 : vnam found with bad number
**           0 : vnam not found
**           1 : vnam found with good number, vnum contains value
*/
    int found;
    struct str varstr;

    init_str(&varstr, 0);

    found = get_var_str(ce, vnam, &varstr);
    if (found) {
        if (stoi(varstr.str_buf, vnum)) {
            found = -1;
        }
    }

    free_str_buf(&varstr);

    return (found);
}
/***************************************************************/
void calc_prompt(struct cenv * ce,
                    int ptyp,
                    const char * prompt_string,
                    struct str * promptstr) {

    int estat;

    estat = 0;

    switch (ptyp) {
        case PROMPT_NONE:
            promptstr->str_len = 0;
            break;
        case PROMPT_CMD:
            if (get_var_str(ce, DESHPROMPT1_NAME, promptstr)) {
                estat = sub_str(ce, promptstr);
                if (estat) {
                    set_str_char_f(promptstr, "* Error #%d (%s) in " DESHPROMPT1_NAME " *",
                            get_estat_errnum(ce, estat), get_estat_errmsg(ce, estat));
                    estat = release_estat_error(ce, estat);
                }
            }
            break;
        case PROMPT_CMD_CONT:
            if (get_var_str(ce, DESHPROMPT2_NAME, promptstr)) {
                estat = sub_str(ce, promptstr);
                if (estat) {
                    set_str_char_f(promptstr, "* Error #%d  (%s) in " DESHPROMPT2_NAME " *",
                            get_estat_errnum(ce, estat), get_estat_errmsg(ce, estat));
                    estat = release_estat_error(ce, estat);
                }
            }
            break;
        case PROMPT_GT:
            set_str_char(promptstr, ">");
            break;
        case PROMPT_DASH:
            set_str_char(promptstr, "-");
            break;
        case PROMPT_STR:
            if (prompt_string) {
                set_str_char(promptstr, prompt_string);
            } else {
                set_str_char(promptstr, "?");   /* should never happen */
            }
            break;
        default:
            break;
    }
}
/***************************************************************/
int set_ctl_c_estat(struct cenv * ce) {

    int estat;

    if (ce->ce_nest && (ce->ce_nest->ne_flags & NEFLAG_BREAK)) {
        estat = set_error(ce, ERUNCTLC, NULL);
    } else {
        estat = ESTAT_CTL_C;
    }

    return (estat);
}
/***************************************************************/
int handle_ctl_c(struct cenv * ce) {

    int estat;

    estat = set_ctl_c_estat(ce);

    return (estat);
}
/***************************************************************/
static void control_c(int sig) {

#if IS_DEBUG
    fprintf(stderr, "\nDEBUG: Control-C\n"); fflush(stderr);
#endif

    global_user_break = 1;

    signal(sig, control_c);
}
/***************************************************************/
int reset_ctl_c(struct cenv * ce) {

    int estat;

    estat = 0;
    ce->ce_cmd_input_str.str_len = 0;
    global_user_break = 0;

    return (estat);
}
/***************************************************************/
int read_tty_prompt(struct cenv * ce, struct filinf * fi, struct str * promptstr) {

    int estat;
    int rtn;
    int inlen;
    char * inbuf;

    estat = 0;

    rtn = fi->fi_tty->termReadLine(fi->fi_tty, promptstr->str_buf, &inbuf, &inlen);
    if (!rtn) {
        if (inlen + 2 >= fi->fi_inbufsz) {
            fi->fi_inbufsz = inlen + 64; /* chunk */
            fi->fi_inbuf = Realloc(fi->fi_inbuf, char, fi->fi_inbufsz);
        }
        memcpy(fi->fi_inbuf, inbuf, inlen);
        fi->fi_inbuf[inlen++] = '\n';
        fi->fi_inbuf[inlen] = '\0';
        fi->fi_inbuflen = inlen;
    } else if (rtn == TERMREAD_EOF) {
        fi->fi_eof = 1;
        estat = ESTAT_EOF;
    } else if (rtn == TERMREAD_INTR) {
        control_c(SIGINT);
        estat = ESTAT_CTL_C;
    } else if (rtn < 0) {
        estat = set_error(ce, ETERMINAL, term_errmsg(fi->fi_tty, rtn));
    }

    return (estat);
}
/***************************************************************/
int read_tty(struct cenv * ce,
                struct filinf * fi,
                int ptyp,
                const char * prompt_string) {

    int estat;
    struct str promptstr;

    estat = 0;
    init_str(&promptstr, 32);

    calc_prompt(ce, ptyp, prompt_string, &promptstr);

    estat = read_tty_prompt(ce, fi, &promptstr);

    free_str_buf(&promptstr);

    return (estat);
}
/***************************************************************/
int read_filinf(struct cenv * ce,
                struct filinf * fi,
                int ptyp,
                const char * prompt_string) {

    int estat;
    int rtn;

    estat           = 0;
    fi->fi_inbufix  = 0;
    fi->fi_inbuflen = 0;

    if (fi->fi_eof) {
        estat = ESTAT_EOF;
    } else if (fi->fi_tty) {
        if (ce->ce_flags & CEFLAG_EXIT) {
            estat = ESTAT_EOF;
        } else {
            estat = read_tty(ce, fi, ptyp, prompt_string);
        }
    } else {
        rtn = (Int)READ(fi->fi_fileno, fi->fi_inbuf, fi->fi_inbufsz);
        if (rtn <= 0) {
            fi->fi_eof  = 1;
            if (rtn < 0) {
                int ern = ERRNO;
                if (fi->fi_infname)
                    estat = set_ferror(ce, EREADCMD,"%s %m", fi->fi_infname, ern);
                else
                    estat = set_ferror(ce, EREADCMD,"%m", ern);
            } else {
                estat = ESTAT_EOF;
            }
        } else {
            fi->fi_inbuflen = rtn;
        }
    }

    return (estat);
}
/***************************************************************/
int handle_error(struct cenv * ce, const char * errcmd, int pestat) {

    int estat;

    estat = pestat;
    if (pestat > 0) {
        set_errmsg(ce, errcmd, estat);
        if (!ce->ce_nest || !(ce->ce_nest->ne_flags & NEFLAG_NOERRMSG)) {
            show_errmsg(ce, errcmd, estat);
        } else {
            clear_error(ce, estat);
        }
        if (ce->ce_nest) {
            if (!ce->ce_filinf || !ce->ce_filinf->fi_tty) {
                ce->ce_nest->ne_idle = NEIDLE_ERROR;
            }
        }

        estat = 0;
    }

    return (estat);
}
/***************************************************************/
int execute_saved_command(struct cenv * ce, struct str * cmdstr) {

    int estat;
    int cmdix;

    cmdix = 0;

    estat = execute_command_handler(ce, cmdstr, &cmdix, 0);
    if (estat) {
        estat = handle_error(ce, cmdstr->str_buf, estat);
    }

    return (estat);
}
/***************************************************************/
int execute_command(struct cenv * ce,
                        struct str * cmdstr,
                        int * cmdix) {

    int estat;

    estat = 0;

    if (is_saved_cmd(ce)) {
        estat = execute_saved_commands(ce);
    }
    if (saving_cmds(ce)) {
        save_cmd(ce, cmdstr, cmdix);
    }

    if (!estat) {
        estat = execute_saved_command(ce, cmdstr);
    }

    return (estat);
}
/***************************************************************/
int read_line(struct cenv * ce,
                struct filinf * fi,
                struct str * linstr,
                int ptyp,
                const char * prompt_string) {

    int estat;
    int eol;
    char ch;

    estat           = 0;
    eol             = 0;
    linstr->str_len = 0;

    while (!estat && !eol) {
        if (fi->fi_inbufix  == fi->fi_inbuflen) {
            estat = read_filinf(ce, fi, ptyp, prompt_string);
            if (estat) {
                APPCH(linstr, '\0');
                eol = 1;
                if (estat == ESTAT_EOF && linstr->str_len) estat = 0;
            }
        } else {
            ch = fi->fi_inbuf[fi->fi_inbufix];
            if (ch == '\n') {
                if (linstr->str_len && linstr->str_buf[linstr->str_len - 1] == '\r')
                    linstr->str_len -= 1;
                APPCH(linstr, '\0');
                eol = 1;
            } else {
                APPCH(linstr, ch);
            }
            fi->fi_inbufix += 1;
        }
        if (!eol && global_user_break) {
        	estat = set_ctl_c_estat(ce);
        }
    }

#if IS_DEBUG
    if (!estat && linstr->str_len == 4 && !memcmp(linstr->str_buf, ":EOD", 4)) {
        return (ESTAT_EOF);
    }
#endif

    return (estat);
}
/***************************************************************/
int find_end(struct cenv * ce, struct str * input_str, int * cmdlinix) {

    int estat;
    int pdepth;
    int ix;
    char ch;
    char quot;
    char paren;
    char cparen;
    struct str contstr;

    estat       = 0;
    quot        = 0;
    pdepth      = 0;

    do {
        ch = input_str->str_buf[(*cmdlinix)];
        switch (ch) {
            case '\n':
                ch = 0; /* done */
                break;

            case '\"':
            case '\'':
            case '`' :
                if (quot == 0) {
                    quot = ch;
                } else if (quot == ch) {
                    if (input_str->str_buf[(*cmdlinix) + 1] == ch) {
                        (*cmdlinix) += 1;
                    } else {
                        quot = 0;
                    }
                }
                break;

            case '(':
            case '[':
            case '{':
                if (quot == 0) {
                    if (!pdepth) {
                        pdepth = 1;
                        paren = ch;
                        if (ch == '[')          cparen = ']';
                        else if (ch == '{')     cparen = '}';
                        else                    cparen = ')';
                    } else if (ch == paren) {
                        pdepth++;
                    }
                }
                break;

            case ')':
            case ']':
            case '}':
                if (quot == 0 && ch == cparen) {
                    pdepth--;
                    if (!pdepth) ch = 0;    /* done */
                }
                break;

            case CONTINUATION_CHAR:
                ix = (*cmdlinix) + 1;
                if (input_str->str_buf[ix] == CONTINUATION_CHAR) {
                    (*cmdlinix) += 1;
               } else {
                    while (isspace(input_str->str_buf[ix])) ix++;

                    if (!input_str->str_buf[ix] || (quot == 0 && input_str->str_buf[ix] == COMMENT_CHAR)) {
                        init_str(&contstr, INIT_STR_CHUNK_SIZE);
                        estat = read_line(ce, ce->ce_filinf, &contstr, PROMPT_CMD_CONT, NULL);
                        if (estat) {
                            ch = 0;    /* done */
                        } else {
                            input_str->str_len = (*cmdlinix);
                            append_str_str(input_str, &contstr);
                            free_str_buf(&contstr);
                            (*cmdlinix)--;
                        }
                    }
                }
                break;

            default:
                break;
        }
        if (ch) (*cmdlinix) += 1;
    } while (ch);

    if (!estat) {
        if (pdepth) {
            estat = set_ferror(ce, EUNMATCHEDPAREN, "%c", paren);
        } else if (quot) {
            estat = set_ferror(ce, EUNMATCHEDQUOTE, "%c", quot);
        }
    }

    return (estat);
}
/***************************************************************/
int read_command(struct cenv * ce, struct str * cmdstr, int * cmdstrix, struct str * input_str) {
/*
**  Command terminators
**      '\0'    - Always
**      '\n'    - Unless preceded by CONT_CHAR
**      '#'     - COMMENT_CHAR - Unless preceded by '&' or in quotation started by ", ', or `
**      ';'     - Unless in quotation started by ", ', or ` or in exp started by (, [, {
*/

    int estat;
    int pdepth;
    char ch;
    char quot;
    char paren;
    char cparen;
    int  ix;
    int  ignsp;

    estat       = 0;
    quot        = 0;
    pdepth      = 0;
    paren       = 0;
    cparen      = 0;
    ignsp       = 1;

    do {
        ch = input_str->str_buf[(*cmdstrix)];
        switch (ch) {
            case '\n':
                ch = 0; /* done */
                break;

            case '\"':
            case '\'':
            case '`' :
                if (!quot) {
                    quot = ch;
                } else if (quot == ch) {
                    if (input_str->str_buf[(*cmdstrix) + 1] == ch) {
                        APPCH(cmdstr, ch);
                        (*cmdstrix) += 1;
                    } else {
                        quot = 0;
                    }
                }
                break;

            case '(':
            case '[':
            case '{':
                if (!quot) {
                    if (!pdepth) {
                        pdepth = 1;
                        paren = ch;
                    } else if (paren == ch) {
                        pdepth++;
                    }
                }
                break;

            case ')':
                if (!quot && (paren == '(')) {
                    pdepth--;
                    if (!pdepth) paren = 0;
                }
                break;

            case ']':
                if (!quot && (paren == '[')) {
                    pdepth--;
                    if (!pdepth) paren = 0;
                }
                break;

            case '}':
                if (!quot && (paren == '{')) {
                    pdepth--;
                    if (!pdepth) paren = 0;
                }
                break;

            case ';':
                if (!quot && !pdepth) {
                    (*cmdstrix) += 1;
                    ch = 0; /* done */
                }
                break;

            case '$':
                if (quot) {
                    /*
                    ** checks "parms ${exp} parm2"
                    */
                    if (input_str->str_buf[(*cmdstrix) + 1] == '{') {
                        ix = (*cmdstrix);
                        (*cmdstrix) = ix + 1;
                        estat = find_end(ce, input_str, cmdstrix);
                        if (!estat) {
                            append_str_len(cmdstr, input_str->str_buf + ix, (*cmdstrix) - ix);
                            ch = '}'; 
                        }
                    }
                }
                break;

            case COMMENT_CHAR:
                if (!quot) {
                    ix = (*cmdstrix) + 1;
                    while (input_str->str_buf[ix]) ix++;
                    (*cmdstrix) = ix;
                    ch = 0; /* done */
                }
                break;

            case CONTINUATION_CHAR:
                ix = (*cmdstrix) + 1;
                if (input_str->str_buf[ix] == CONTINUATION_CHAR) {
                    APPCH(cmdstr, ch);
                    (*cmdstrix) += 1;
               } else {
                    while (isspace(input_str->str_buf[ix])) ix++;

                    if (!input_str->str_buf[ix] || (!quot && input_str->str_buf[ix] == COMMENT_CHAR)) {
                        estat = read_line(ce, ce->ce_filinf, &(ce->ce_cmd_input_str), PROMPT_CMD_CONT, NULL);
                        if (estat) {
                            ch = 0; /* done */
                        } else {
                            (*cmdstrix) = 0;
                            ch = '\n'; /* ignore */
                        }
                    }
                }
                break;

            default:
                break;
        }

        if (ch) {
            if (isspace(ch)) {
                if (ch == '\n') {
                    /* do nothing */
                } else {
                    if (!ignsp) {
                        APPCH(cmdstr, ch);
                        if (!quot) ignsp = 1;
                    }
                    (*cmdstrix) += 1;
                }
            } else {
                APPCH(cmdstr, ch);
                ignsp = 0;
                (*cmdstrix) += 1;
            }
        }
    } while (!estat && ch);

    APPCH(cmdstr, '\0');

    return (estat);
}
/***************************************************************/
int read_next_command(struct cenv * ce, struct str * cmdstr, int * cmd_ix) {

    int estat;
    int len;
    int done;
    char * cmd;

    estat = 0;

    if (ce->ce_reading_saved) {
        cmd = get_next_saved_cmd(ce);
        if (!cmd) {
            estat = ESTAT_EOF;
        } else {
            len = Istrlen(cmd);
            if (len >= cmdstr->str_max) {
                cmdstr->str_max = len + 1;
                cmdstr->str_buf = Realloc(cmdstr->str_buf, char, cmdstr->str_max);
            }
            memcpy(cmdstr->str_buf, cmd, len + 1);
            cmdstr->str_len = len;
        }
    } else {
        done            = 0;
        cmdstr->str_len = 0;

        while (!done) {
            if ((*cmd_ix) < ce->ce_cmd_input_str.str_len) {
                estat = read_command(ce, cmdstr, cmd_ix, &(ce->ce_cmd_input_str));
                if (estat) {
                    done = 1;
                } else {
                    if (cmdstr->str_len) done = 1;
                }
            } else {
                estat = read_line(ce, ce->ce_filinf, &(ce->ce_cmd_input_str), PROMPT_CMD, NULL);
                if (estat) done = 1;
                (*cmd_ix)   = 0;
            }
        }
    }

    return (estat);
}
/***************************************************************/
int execute_saved_commands(struct cenv * ce) {

    int estat;
    int reading_saved;
    int cmd_ix;
    struct str cmdstr;

    estat = 0;
    cmd_ix = 0;
    init_str(&cmdstr, 0);

    reading_saved = ce->ce_reading_saved;
    ce->ce_reading_saved = 1;

    do {
        estat = read_next_command(ce, &cmdstr, &cmd_ix);
        if (!estat) {
            estat = execute_saved_command(ce, &cmdstr);
        }
    } while (!estat);

    if (estat == ESTAT_EOF) {
        estat = 0;
    }

    free_str_buf(&cmdstr);
    ce->ce_reading_saved = reading_saved;

    return (estat);
}
/***************************************************************/
int read_and_execute_commands_from_file(struct cenv * ce,
                        const char * cmd_file_name) {

    int estat;
    int cmdix;
    int cmd_ix;
    int cmderr;
    struct nest * ne;
    struct str cmdstr;

    estat  = 0;
    cmderr = 0;
    cmd_ix = 0;

    init_str(&cmdstr, 0);
    ne = new_nest(ce, NETYP_FILE);

    do {
        if (is_saved_cmd(ce)) {
            /* Added 08/06/2013 to handle endfor and endwhile from terminal */
            estat = execute_saved_commands(ce);
        }
        if (!estat) {
            estat = read_next_command(ce, &cmdstr, &cmd_ix);
        }
        if (!estat) {
            if (ce->ce_filinf->fi_tty) {
                ce->ce_filinf->fi_tty->termAddHistoryLine(ce->ce_filinf->fi_tty, cmdstr.str_buf);
            }
            cmdix = 0;
            estat = execute_command(ce, &cmdstr, &cmdix);
        }
    } while (!estat);

    if (estat < 0) {
        if (estat == ESTAT_CTL_C) {
            estat = set_ctl_c_estat(ce);
        } else if (estat == ESTAT_EXIT) {
            while (ce->ce_nest && ce->ce_nest->ne_typ != NETYP_FILE) {
                free_nest(ce, ce->ce_nest);
            }
            estat = 0;
            if (ce->ce_rtn) {
                cmderr = 1;
            }
        } else {
            estat = 0;
            if (is_saved_cmd(ce)) {
                estat = execute_saved_commands(ce);
            }
        }
    }

    if (!estat) {
        if (ce->ce_nest && ce->ce_nest->ne_idle == NEIDLE_ERROR) {
            cmderr = 1;
        } else if (!ce->ce_nest || ce->ce_nest->ne_typ != NETYP_FILE) {
            clean_nest_mess(ce, NETYP_FILE, 0);
            estat = set_error(ce, EMISSINGEND, NULL);
            cmderr = 1;
        }
    }
    if (cmderr) free_nest(ce, ce->ce_nest);

    free_str_buf(&cmdstr);

	/* call command , '.' command, and <parm cmd> return here */

    if (estat > 0 && ce->ce_rtn == PROGRTN_SUCC) {
        /* EMISSINGEND can happen here */
        estat = handle_error(ce, NULL, estat);
    }

    if (!estat && cmderr) {
        estat = set_error(ce, ECMDFAILED, cmd_file_name);
    }

    return (estat);
}
/***************************************************************/
int execute_command_from_parms(struct cenv * ce, int parmix) {

    int estat;
    FILET cmdfil;
    struct cenv * nce;
    int ii;
    int argc;
    char ** argv;
    struct filinf * fi;

    /* should search path */
    cmdfil = OPEN(ce->ce_argv[parmix], _O_RDONLY);
    if (cmdfil < 0) {
        int ern = ERRNO;
        estat = set_ferror(ce, EOPENCMD, "%s %m", ce->ce_argv[parmix], ern);
    } else {
        estat = 0;
        fi = new_filinf(cmdfil, ce->ce_argv[parmix], CMD_LINEMAX);

        argc = ce->ce_argc - parmix;
        argv = New(char *, argc);
        for (ii = 0; ii < argc; ii++) {
            argv[ii] = Strdup(ce->ce_argv[ii + parmix]);
        }

        nce = new_cenv(ce->ce_stdin_f,
                        ce->ce_stdout_f,
                        ce->ce_stderr_f,
                        fi,
                        FREECE_ARGS | FREECE_STMTS | FREECE_VARS,
                        argc,
                        argv,
                        ce->ce_flags,
                        ce->ce_funclib,
                        ce->ce_gg,
                        NULL,
                        ce);

        estat = read_and_execute_commands_from_file(nce, ce->ce_argv[parmix]);

        ce->ce_rtn = nce->ce_rtn;

        free_cenv(ce, nce);
    }

    return (estat);
}
/***************************************************************/
int execute_command_list(struct cenv * ce, 
                        struct str * cmdliststr,
                        int * cmdstrix,
                        int neflags) {
/*
int read_command(struct cenv * ce, struct str * cmdstr, int * cmdstrix, struct str * input_str) {
*/
    int estat;
    int done;
    int cmdix;

    struct nest * ne;
    struct str cmdstr;

    estat = 0;
    init_str(&cmdstr, INIT_STR_CHUNK_SIZE);

    ne = new_nest(ce, NETYP_CMD);
    ne->ne_flags = neflags;

    done = 0;
    while (!done && !estat) {
        cmdstr.str_len = 0;
        estat = read_command(ce, &cmdstr, cmdstrix, cmdliststr);
        if (!estat) {
            if (!cmdstr.str_len) {
                done = 1;
            } else {
                cmdix = 0;
                estat = execute_command(ce, &cmdstr, &cmdix);
            }
        }
    }

    if (!estat && is_saved_cmd(ce)) {
        estat = execute_saved_commands(ce);
    }

    if (!estat) {
        if (!ce->ce_nest || ce->ce_nest->ne_typ != NETYP_CMD) {
            estat = set_error(ce, EMISSINGENDLIST, NULL);
        } else {
            if (ce->ce_nest->ne_idle == NEIDLE_ERROR) {
                estat = ESTAT_IDLEERR;
            }
        }
    }

    free_str_buf(&cmdstr);

    return (estat);
}
/***************************************************************/
int print_banner(struct cenv * ce) {

    int estat;
    char msg[128];

    estat = 0;

    if (ce->ce_flags & CEFLAG_VERSNO) {
        get_desh_version(8, msg, sizeof(msg));
        WRITE(ce->ce_stdout_f, msg, Istrlen(msg));
    } else {
        get_desh_version(9, msg, sizeof(msg));
        WRITE(ce->ce_stderr_f, msg, Istrlen(msg));
    }

    return (estat);
}
/***************************************************************/
int execute_command_cmdstr(struct cenv * ce, const char * cmdbuf, int parmix) {
/*
**  Use:    estat = read_command(ce, cmdstr, cmd_ix, &(ce->ce_cmd_input_str));
**          estat = execute_command_handler(ce, cmdstr, cmdix, 0);
**
** See also: int execute_command_string(struct cenv * ce,
                        const char * cmdbuf,
                        struct str * cmdout);
*/
    int estat;
    int cmdix;
    struct cenv * nce;
    struct str cmdstr;
    int argc;
    char ** argv;
    int ii;

    estat = 0;
    cmdix = 0;

    init_str(&cmdstr, 0);
    set_str_char(&cmdstr, cmdbuf);

    argc = ce->ce_argc - parmix;
    argv = New(char *, argc);
    for (ii = 0; ii < argc; ii++) {
        argv[ii] = Strdup(ce->ce_argv[ii + parmix]);
    }

    nce = new_cenv(ce->ce_stdin_f,
                    ce->ce_stdout_f,
                    ce->ce_stderr_f,
                    NULL,
                    FREECE_ARGS | FREECE_STMTS | FREECE_VARS,
                    argc,
                    argv,
                    ce->ce_flags,
                    ce->ce_funclib,
                    ce->ce_gg,
                    NULL,
                    ce);

    if (ce->ce_flags & CEFLAG_VERSNO) {
        estat = print_banner(ce);
    }

    estat = execute_command_list(nce, &cmdstr, &cmdix, 0);
    if (estat == ESTAT_IDLEERR) {   /* 6/20/2014 */
        estat = set_error(ce, ECMDARGFAILED, cmdstr.str_buf);
    }

    free_cenv(ce, nce);

    free_str_buf(&cmdstr);

    return (estat);
}
/***************************************************************/
void append_quoted_parm(struct str * sbuf,
                        const char * cbuf)
{
    int needsq;
    int ii;
    int jj;

    needsq = 0;
    if (!cbuf[0]) needsq = 1; /* zero length parm needs to be "" */

    for (ii = 0; !needsq && cbuf[ii]; ii++) {
        if (isspace(cbuf[ii])) needsq = 1;
    }
    if (needsq) APPCH(sbuf, '\"');

    for (ii = 0; cbuf[ii]; ii++) {
        if (cbuf[ii] == '\"') {
            jj = ii - 1;
            while (jj >= 0 && cbuf[jj] == '\\') {
                APPCH(sbuf, '\\');
                jj--;
            }
            APPCH(sbuf, '\\');
            APPCH(sbuf, cbuf[ii]);
        } else {
            APPCH(sbuf, cbuf[ii]);
        }
    }
    if (needsq) APPCH(sbuf, '\"');
}
/***************************************************************/
#ifdef OLD_WAY
int execute_command_from_parms(struct cenv * ce, int parmix) {

    int estat;
    int ii;
    int cmdix;
    int cmderr;
    struct nest * ne;
    struct str cmdstr;

    estat = 0;
    cmderr = 0;
    init_str(&cmdstr, 0);
    ne = new_nest(ce, NETYP_CMD_LIND_CMD);

    for (ii = parmix; ii < ce->ce_argc; ii++) {
        if (cmdstr.str_len) APPCH(&cmdstr, ' ');
        append_quoted_parm(&cmdstr, ce->ce_argv[ii]);
    }
    APPCH(&cmdstr, '\0');

    cmdix = 0;
    estat = execute_command_handler(ce, &cmdstr, &cmdix, 0);

    if (!estat) {
        if (ce->ce_nest && ce->ce_nest->ne_idle == NEIDLE_ERROR) {
            cmderr = 1;
        } else if (!ce->ce_nest || ce->ce_nest->ne_typ != NETYP_CMD_LIND_CMD) {
            estat = set_error(ce, EMISSINGEND, NULL);
            cmderr = 1;
        }
    }
    if (cmderr) free_nest(ce, ce->ce_nest);

    free_str_buf(&cmdstr);

    if (estat > 0) {
        /* EMISSINGEND can happen here */
        estat = handle_error(ce, NULL, estat);
    }

    if (!estat && cmderr) {
        estat = set_error(ce, ECMDFAILED, cmdstr.str_buf);
    }

    return (estat);
}
#endif
/***************************************************************/
void get_desh_version(int vfmt, char * vers, int versmax) {

    int vlen;

    if ((vfmt & 7) == 0) {
        Snprintf(vers, versmax, "%s%s",DESH_VERSION, DESH_FIX);
    } else if ((vfmt & 7) == 1) {
        Snprintf(vers, versmax, "%s - %s.%s (c) %s %s",
                DESH_PROGRAM, DESH_VERSION, DESH_FIX, DESH_YEAR, DESH_COMPANY);
    } else if ((vfmt & 7) == 2) {
        Snprintf(vers, versmax, "%s - %s.%s",
                DESH_PROGRAM, DESH_VERSION, DESH_FIX);
    } else if ((vfmt & 7) == 3) {
        Snprintf(vers, versmax, "%s%s",
        		DESH_VERSION, DESH_VFIX);
    } else {
        vers[0] = '\0';
    }

    if (vfmt & 8) {
        vlen = Istrlen(vers);
        if (vlen + 1 < versmax) {
            vers[vlen]     = '\n';
            vers[vlen + 1] = '\0';
        }
    }
}
/***************************************************************/
int find_home_file(struct cenv * ce,
                const char * cmdchar,
                struct str * homename) {

    int found;
    int ftyp;

    found = 0;

    if (get_var_str(ce, "HOME", homename)) {
        if (homename->str_len) {
            if (homename->str_buf[homename->str_len - 1] != DIR_DELIM) {
                APPCH(homename, DIR_DELIM);
            }
            append_str(homename, cmdchar);
            ftyp = file_stat(homename->str_buf);
            if (ftyp == FTYP_FILE) found = 1;
        }
    }

    return (found);
}
/***************************************************************/
int run_rc_file(struct cenv * ce) {
/*
** Use find_exe() to look for deshrc
*/
    int estat;
    int estat2;
    int     found;
    FILET rcfil;
    struct filinf * fi;
    struct str rcfile;

    estat = 0;
    init_str(&rcfile, INIT_STR_CHUNK_SIZE);

    found = find_home_file(ce, DESHRC_NAME, &rcfile);

    if (!found) {
        if (ce->ce_flags & CEFLAG_WIN) {
            found = find_home_file(ce, DESHRCINI_NAME, &rcfile);
        } else {
            found = find_home_file(ce, DESHRCDOT_NAME, &rcfile);
        }
    }

    if (found) {
        rcfil = OPEN(rcfile.str_buf, _O_RDONLY);
        if (rcfil < 0) {
            int ern = ERRNO;
            estat = set_ferror(ce, EOPENRCFILE, "%s %m", rcfile.str_buf, ern);
        } else {
            fi = new_filinf(rcfil, rcfile.str_buf, CMD_LINEMAX);
            ce->ce_filinf = fi;

            estat = read_and_execute_commands_from_file(ce, rcfile.str_buf);

            free_filinf(fi);

            if (estat) {
                estat2 = set_error(ce, ERUNRCFILE, rcfile.str_buf);
                estat = chain_error(ce, estat, estat2);
            }
        }
    }
    free_str_buf(&rcfile);

    return (estat);
}
/***************************************************************/
int read_cmds_from_stdin(struct cenv * ce,
                        const char * program_name) {
/*
** Use find_exe() to look for deshrc
*/
    int estat;
    int done;

    estat = print_banner(ce);

    done = 0;
    while (!done) {
        estat = read_and_execute_commands_from_file(ce, program_name);
        if (estat == ESTAT_CTL_C && (ce->ce_filinf->fi_stdin_tty)) {
            estat = reset_ctl_c(ce);
        } else {
            done = 1;
        }
    }

    return (estat);
}
/***************************************************************/
int global_ce_init(struct cenv * ce, struct filinf * fi_stdin, int gflags) {

    int estat;

    glob_init(ce->ce_gg, gflags);

    estat = init_required_variables(ce);
    if (!estat && !(gflags & CEFLAG_NORC)) {
        estat = run_rc_file(ce);
    }
    ce->ce_filinf = fi_stdin;

    global_flags = ce->ce_flags;

    signal(SIGINT, control_c);

    return (estat);
}
/***************************************************************/
int cmd_file_in(struct cenv * ce, struct filinf * fi_stdin) {

    int estat;
    int parmix;
    int oflag;
    int ginit;

    estat = 0;
    ginit = 0;
    parmix = 1;

#if FUNCTRACE
    printf("enter cmd_file_in()\n");
#endif

    while (!estat && parmix < ce->ce_argc) {
#if IS_DEBUG
            if (!Stricmp(ce->ce_argv[parmix], "-memstats")) {
                parmix++;
                if (parmix < ce->ce_argc && ce->ce_argv[parmix][0] != '-') {
                    if (ce->ce_gg->gg_mem_fname) {
                        Free(ce->ce_gg->gg_mem_fname);
                    }
                    ce->ce_gg->gg_mem_fname = Strdup(ce->ce_argv[parmix]);
                } else {
                    show_warn(ce, "Expected file name after %s", ce->ce_argv[parmix-1]);
                }
            } else
#endif

        if (ce->ce_argv[parmix][0] == '-') {
            if (CEDWNS(ce, ce->ce_argv[parmix][1]) == 'c') {
                ce->ce_flags |= CEFLAG_CMD;
                if (!ginit) {
                    estat = global_ce_init(ce, fi_stdin, ce->ce_flags);
                    ginit = 1;
                }
                parmix++;
                if (!estat && parmix < ce->ce_argc) {
                    estat = execute_command_cmdstr(ce, ce->ce_argv[parmix], parmix);
                    parmix = ce->ce_argc;
                }
            } else {
                oflag = get_opt_flag(ce->ce_argv[parmix][1]);

                if (oflag) {
                    ce->ce_flags |= oflag;
                } else {
                    show_warn(ce, "Invalid option: %s", ce->ce_argv[parmix]);
                }
            }
        } else {
            if (!ginit) {
                estat = global_ce_init(ce, fi_stdin, ce->ce_flags);
                ginit = 1;
            }
            if (!estat) {
                estat = execute_command_from_parms(ce, parmix);
                parmix = ce->ce_argc;
            }
        }
        parmix++;
    }

    if (!estat && !ginit) {
        estat = global_ce_init(ce, fi_stdin, ce->ce_flags);
        if (!estat) {
            ginit = 1;
            estat = read_cmds_from_stdin(ce, ce->ce_argv[0]);
        }
    }
#if FUNCTRACE
    printf("exit  cmd_file_in()\n");
#endif

    return (estat);
}
/***************************************************************/
/*
char * get_global_argv0(void) {

    return (global_argv0);
}
*/
/***************************************************************/
int main(int argc, char *argv[], char *envp[]) {

    int do_pause = 0;
    int err_num = 0;
    struct glob * gg;
    struct cenv * ce;
    struct filinf * fi_stdin;
    int estat;
    int estat2;
#if IS_DEBUG
    char mem_fname[128];
    mem_fname[0] = '\0';
#endif

    estat = 0;

    gg = new_glob(envp);

    ce = new_cenv(
                FILENO(stdin),
                FILENO(stdout),
                FILENO(stderr),
                NULL,
                FREECE_STMTS | FREECE_VARS | FREECE_FUNCS,
                argc,
                argv,
                DEFAULT_FLAGS,
                new_funclib(),
                gg,
                NULL,
                NULL);

    fi_stdin = new_filinf(FILENO(stdin), "stdin", CMD_LINEMAX);
    set_filinf_flags(fi_stdin, FIFLAG_NOCLOSE); /* do not close stdin */
    estat = filinf_pair(ce, fi_stdin, FILENO(stdout), "stdout");

    if (!estat) estat = cmd_file_in(ce, fi_stdin);
    if (ce->ce_flags & CEFLAG_PAUSE) {
        do_pause = 1;
        ce->ce_flags ^= CEFLAG_PAUSE;
    }

#if IS_DEBUG
    if (gg->gg_mem_fname) strncpy(mem_fname, gg->gg_mem_fname, sizeof(mem_fname));
#endif

    estat2 = rx_free_processes();
    if (!estat) estat = estat2;

    if (estat > 0) {
        show_errmsg(ce, NULL, estat);
        err_num = PROGRTN_FAIL;
    } else {
        err_num = ce->ce_rtn;
    }

    free_cenv(NULL, ce);
    free_glob(gg);
    free_help();

#if IS_DEBUG
   if (mem_fname[0]) print_mem_stats(mem_fname);
#endif

    if (do_pause) {
        char inbuf[100];
        fflush(stdin);
        fflush(stdout);
        fflush(stderr);
#if IS_DEBUG
        if (!err_num)   printf("[%d] Press ENTER to continue...", rx_get_pid());
        else            printf("[%d] FAILED(%d).  Press ENTER to continue...",
                                rx_get_pid(), err_num);
#else
        if (!err_num)   printf("Press ENTER to continue...");
        else            printf("FAILED(%d).  Press ENTER to continue...", err_num);
#endif
        READ(FILENO(stdin), inbuf, sizeof(inbuf));
    }

    return (err_num);
}
/***************************************************************/
