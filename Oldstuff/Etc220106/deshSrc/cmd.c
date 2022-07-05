/***************************************************************/
/* cmd.c                                                       */
/***************************************************************/
/*
**
** Done
** 03/06/12 Started work
**
**              Exit code:  0 - success
**                          1 - Error 
**
****************************************************************

****************************************************************
*/
/*
    From: http://www.computerhope.com/jargon/i/intecomm.htm

        command.com internal commands

            ASSOC           ERASE           REN
            ATMADM          EXIT            RENAME
            BREAK           FOR             RMDIR
            CALL            GOTO            SET
            CD              IF              SETLOCAL
            CHDIR           MD              SHIFT
            CLS             MKDIR           START
            COLOR           MOVE            TIME
            COPY            PATH            TITLE
            DATE            PAUSE           TYPE
            DEL             POPD            UNLOCK
            DIR             PROMPT          VER
            ECHO            PUSHD           VERIFY
            ENDLOCAL        RD              VOL


    For bash, see: http://linux.about.com/library/cmd/blcmdl1_builtin.htm

            :                   jobs
            .                   kill
            alias               kill
            bg                  let
            bind                local
            break               logout
            cd                  popd
            command             printf
            compgen             pushd
            complete            pushd
            continue            pwd
            declare             read
            typeset             readonly
            dirs                return
            disown              set
            echo                shift
            enable              shopt
            eval                suspend
            exec                test
            exit                times
            export              trap
            export              type
            fc                  ulimit
            fg                  umask
            getopts             unalias
            hash                unset
            help                wait
            history

*/

#include "config.h"

#if IS_WINDOWS
    #include <io.h>
    #include <direct.h>
#else
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <fcntl.h>
#include <math.h>
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
#include "sysx.h"
#include "help.h"

#define DEBUG_SHOW_FUNCS    0
#define DEBUG_SHOW_PARMLIST 0

/***************************************************************/
#if DEBUG_SHOW_FUNCS
void show_usrfunc(struct cenv * ce,
                    struct usrfuncinfo * ufi,
                    struct var * vparmlist,
                    int nparms) {

    int parmix;
    int ii;
    struct var tvar;

    printf("Function: %s, num statements=%d, formal parms=%d, actual parms=%d\n",
                ufi->ufi_func_name, ufi->ufi_stmt_len, ufi->ufi_nparms, nparms);

    if (ufi->ufi_flags & UFIFLAG_OPT_PARMS) {
        printf("Function parameters optional for: %s\n", ufi->ufi_func_name);
    }

    for (parmix = 0; parmix < ufi->ufi_nparms; parmix++) {
        if (parmix < nparms) {
            copy_var(&tvar, &(vparmlist[parmix]));
            vensure_str(ce, &tvar);
            printf("    Parm %-16s : %s\n", ufi->ufi_sparmlist[parmix]->str_buf, tvar.var_buf);
            free_var_buf(&tvar);
        } else {
            printf("    Parm %-16s : %s\n", ufi->ufi_sparmlist[parmix]->str_buf, "<null>");
        }
    }

    while (parmix < nparms) {
        copy_var(&tvar, &(vparmlist[parmix]));
        vensure_str(ce, &tvar);
        printf("    Parm %-16s : %s\n", "<none>", tvar.var_buf);
        free_var_buf(&tvar);
        parmix++;
    }

    printf("  Statements:\n");
    for (ii = 0; ii < ufi->ufi_stmt_len; ii++) {
        printf("    %2d %s\n", ii, ufi->ufi_stmt_list[ii]);
    }
}
#endif
/***************************************************************/
void copy_str(struct str * tgtstr, struct str * srcstr) {

    tgtstr->str_len = srcstr->str_len;
    tgtstr->str_max = srcstr->str_len + 1;
    tgtstr->str_buf = New(char, tgtstr->str_max);
    memcpy(tgtstr->str_buf, srcstr->str_buf, srcstr->str_len);
    tgtstr->str_buf[tgtstr->str_len] = '\0';
}
/***************************************************************/
void qcopy_str(struct str * tgtstr, struct str * srcstr) {

    if (tgtstr->str_buf) Free(tgtstr->str_buf);
    tgtstr->str_buf = srcstr->str_buf;
    tgtstr->str_len = srcstr->str_len;
    tgtstr->str_max = srcstr->str_max;
    srcstr->str_len = 0;
    srcstr->str_max = 0;
    srcstr->str_buf = NULL;
}
/***************************************************************/
void qcopy_str_char(char ** tgtchar, struct str * srcstr) {

    (*tgtchar) = srcstr->str_buf;
    srcstr->str_len = 0;
    srcstr->str_max = 0;
    srcstr->str_buf = NULL;
}
/***************************************************************/
void qcopy_var_char(char ** tgtchar, struct var * srcvar) {

    (*tgtchar) = srcvar->var_buf;
    srcvar->var_len = 0;
    srcvar->var_max = 0;
    srcvar->var_buf = NULL;
}
/***************************************************************/
void free_strlist(struct str ** strlist) {

    int ii;

    if (strlist) {
        for (ii = 0; strlist[ii]; ii++) {
            Free(strlist[ii]->str_buf);
            Free(strlist[ii]);
        }
        Free(strlist);
    }
}
/***************************************************************/
void display_usrfunc(struct cenv * ce,
                    struct usrfuncinfo * ufi,
                    struct var * vparmlist,
                    int nparms) {

    int parmix;
    struct var tvar;

    printlle(ce, "function %s", ufi->ufi_func_name);

    for (parmix = 0; parmix < ufi->ufi_nparms; parmix++) {
        if (!parmix) printlle(ce, "(");
        else printlle(ce, ",");
        if (parmix < nparms) {
            copy_var(&tvar, &(vparmlist[parmix]));
            vensure_str(ce, &tvar);
            printlle(ce, "%s=\"%s\"", ufi->ufi_sparmlist[parmix]->str_buf, tvar.var_buf);
            free_var_buf(&tvar);
        } else {
            printlle(ce, "%s=\"\"", ufi->ufi_sparmlist[parmix]->str_buf);
        }
    }

    while (parmix < nparms) {
        if (!parmix) printlle(ce, "(");
        else printlle(ce, ",");
        copy_var(&tvar, &(vparmlist[parmix]));
        vensure_str(ce, &tvar);
        printlle(ce, "\"%s\"", tvar.var_buf);
        free_var_buf(&tvar);
        parmix++;
    }
    if (parmix) printlle(ce, ")");
    printlle(ce, "\n");
}
/***************************************************************/
int call_usrfunc(struct cenv * ce,
                 struct usrfuncinfo * ufi,
                 struct parmlist * pl,
                 struct var * vparmlist,
                 int nparms,
                 struct var * result) {

    int estat;
    int parmix;
    struct cenv * fce;
    struct cenv * file_ce;
    struct nest * ne;
    int argix;
    int func_failed;
    struct var tvar;

    estat = 0;
    func_failed = 0;

#if DEBUG_SHOW_FUNCS
    show_usrfunc(ce, ufi, vparmlist, nparms);
#endif
    if (ce->ce_flags & CEFLAG_VERBOSE) {
        display_usrfunc(ce, ufi, vparmlist, nparms);
    }

    if (!(ufi->ufi_flags & UFIFLAG_OPT_PARMS)) {
        if (nparms < ufi->ufi_nparms) {
            estat = set_ferror(ce, EMISSINGPARM, "%d", ufi->ufi_nparms - nparms);
            for (parmix = 0; parmix < nparms; parmix++) {
                free_var_buf(&(vparmlist[parmix]));
            }
            return (estat);
        }
    }

    if (ce->ce_file_ce) file_ce = ce->ce_file_ce;
    else file_ce = ce;

    if (pl) {
        estat = handle_redirs(ce, pl);
        if (estat) return (estat);
        fce = pl->pl_ce;
        fce->ce_file_ce = file_ce;
        fce->ce_close_flags |= FREECE_VARS;
    } else {
        fce = new_cenv(ce->ce_stdin_f,
                        ce->ce_stdout_f,
                        ce->ce_stderr_f,
                        NULL,
                        FREECE_VARS,
                        0,
                        NULL,
                        ce->ce_flags,
                        ce->ce_funclib,
                        ce->ce_gg,
                        file_ce,
                        ce);
    }

   for (parmix = 0; parmix < ufi->ufi_nparms; parmix++) {
        if (parmix < nparms) {
            setvar_var(fce,
                    ufi->ufi_sparmlist[parmix]->str_buf, 
                    ufi->ufi_sparmlist[parmix]->str_len,
                    &(vparmlist[parmix]));
        } else {
            setvar_str(fce,
                    ufi->ufi_sparmlist[parmix]->str_buf, 
                    ufi->ufi_sparmlist[parmix]->str_len,
                    "");
        }
    }

    if (nparms <= ufi->ufi_nparms) {
        fce->ce_argc = 1;
        fce->ce_argv = New(char*, 1);
    } else {
        fce->ce_argc = nparms - ufi->ufi_nparms + 1;
        fce->ce_argv = New(char*, fce->ce_argc);

        parmix = ufi->ufi_nparms;
        argix = 1;
        while (parmix < nparms) {
            qcopy_var(&tvar, &(vparmlist[parmix]));
            vensure_str(ce, &tvar);
            qcopy_var_char(&(fce->ce_argv[argix]), &tvar);
            parmix++;
            argix++;
        }
    }
    fce->ce_argv[0] = Strdup(ufi->ufi_func_name);
    fce->ce_close_flags |= FREECE_ARGS;

    ne = new_nest(fce, NETYP_EXEC_FUNC);
    ne->ne_flags = ce->ce_nest->ne_flags | NEFLAG_IN_FUNC;
    ne->ne_loopback = 1;

    fce->ce_stmt_list = ufi->ufi_stmt_list;
    fce->ce_stmt_len  = ufi->ufi_stmt_len;

    set_cmd_pc(fce, 0);

    estat = execute_saved_commands(fce);

	/* functions return here */

    if (!estat) {
        if (!fce->ce_nest || fce->ce_nest->ne_typ != NETYP_EXEC_FUNC) {
            estat = set_error(ce, EMISSINGEND, NULL);
        } else {
            if (fce->ce_nest->ne_idle == NEIDLE_ERROR) func_failed = 1;
            free_nest(fce, ne);
        }
    }

    if (func_failed) {
        ce->ce_nest->ne_idle = NEIDLE_ERROR;
    }

    /* set ce->ce_rtn */
    if (estat && !fce->ce_rtn) ce->ce_rtn = PROGRTN_FAIL;
    else                       ce->ce_rtn = fce->ce_rtn;

    if (fce->ce_rtn_var) {
        qcopy_var(result, fce->ce_rtn_var);
        Free(fce->ce_rtn_var);
    } else {
        result->var_typ = VTYP_INT;
        result->var_val = fce->ce_rtn;
    }

    if (!pl) free_cenv(ce, fce);

    return (estat);
}
/***************************************************************/
int call_usrfunc_strparms(struct cenv * ce,
                 struct usrfuncinfo * ufi,
                 struct parmlist * pl,
                 struct var * result) {

    int estat;
    int             parmc;
    int             parmmax;
    struct var * vparmlist;

    estat = 0;
    parmc = 0;
    parmmax = 0;
    vparmlist = NULL;

    while (!estat && parmc < pl->pl_nparms) {
        if (parmc >= parmmax) {
            parmmax += 4; /* CHUNK */
            if (!parmc) vparmlist = New(struct var, parmmax);
            else vparmlist = Realloc(vparmlist, struct var, parmmax);
        }
        vparmlist[parmc].var_typ = VTYP_STR;
        vparmlist[parmc].var_buf = pl->pl_cparmlist[parmc];
        vparmlist[parmc].var_len = pl->pl_cparmlens[parmc];
        vparmlist[parmc].var_max = pl->pl_cparmmaxs[parmc];
        pl->pl_cparmlist[parmc] = NULL;
        parmc++;
    }

    Free(pl->pl_cparmlist);
    pl->pl_cparmlist = NULL;
    pl->pl_nparms = 0;

    if (!estat) {
        estat = call_usrfunc(ce, ufi, pl, vparmlist, parmc, result);
    }

    Free(vparmlist);

    return (estat);
}
/***************************************************************/
int get_wtype(struct cenv * ce,
                const char * parms,
                char * wtypbuf,
                int wtypsiz) {
/*
IQueryAssociations interface 
    AssocQueryKey function

int resolve_assoc( char * pszDst,
		char * pszDesc,
		const char * pszSrc,
        uint32 link_to,    // link timeout
		char * errmsg,
        int errmsgmax);
*/
    int estat = 0;
    char errmsg[ERRMSG_MAX + 1];

    if (resolve_assoc(parms, wtypbuf, wtypsiz, errmsg, ERRMSG_MAX)) {
        estat = set_error(ce, EASSOC, errmsg);
    }

    return (estat);
}
/***************************************************************/
#ifdef UNUSED
void copy_char_str_upper(struct str * tgtstr, const char * srcchar) {

    int slen;
    int ii;

    slen = strlen(srcchar);
    tgtstr->str_max = slen + 1;
    tgtstr->str_buf = New(char, tgtstr->str_max);
    for (ii = 0; ii < slen; ii++) tgtstr->str_buf[ii] = toupper(srcchar[ii]);
    tgtstr->str_buf[slen] = '\0';
    tgtstr->str_len = slen;
}
#endif
/***************************************************************/
void copy_char_str_len(struct str * tgtstr, const char * srcchar, int slen) {

    if (tgtstr->str_max <= slen) {
        tgtstr->str_max = slen + 1;
        tgtstr->str_buf = Realloc(tgtstr->str_buf, char, tgtstr->str_max);
    }
    memcpy(tgtstr->str_buf, srcchar, slen + 1);
    tgtstr->str_len = slen;
}
/***************************************************************/
void copy_char_str(struct str * tgtstr, const char * srcchar) {

    copy_char_str_len(tgtstr, srcchar, Istrlen(srcchar));
}
/***************************************************************/
void copy_str_char(char ** tgtchar, struct str * srcstr) {

    (*tgtchar) =  New(char, srcstr->str_len + 1);
    memcpy(*tgtchar, srcstr->str_buf, srcstr->str_len + 1);
}
/***************************************************************/
int search_path(struct cenv * ce,
                    const char * path,
                    const char * parm0,
                    int search_flags,
                    struct str * cmdname) {

    int     found;
    int     pathix;
    int     pix;
    int     dot_searched;

    pathix       = 0;
    found        = 0;
    dot_searched = 0;

    while (!found && path[pathix]) {
        cmdname->str_len = 0;
        while (path[pathix] && path[pathix] != ce->ce_gg->gg_path_delim) {
            APPCH(cmdname, path[pathix]);
            pathix++;
        }
        if (cmdname->str_len) {
            if (cmdname->str_len == 1 && cmdname->str_buf[0] == '.') dot_searched = 1;
            if (cmdname->str_buf[cmdname->str_len - 1] != DIR_DELIM) {
                APPCH(cmdname, DIR_DELIM);
            }
            pix = 0;
            while (parm0[pix]) {
                APPCH(cmdname, parm0[pix]);
                pix++;
            }
            APPCH(cmdname, '\0');
            if (check_file_access(cmdname->str_buf, SRCHFLAG_TO_FACC(search_flags))) found = 1;
        }
        if (path[pathix]) pathix++; /* skip delim */
    }

    if (!found && !dot_searched && (search_flags & SRCHFLAG_SEACH_DOT)) {
        copy_char_str_len(cmdname, ".", 1);
        append_str_ch(cmdname, DIR_DELIM);
        append_str(cmdname, parm0);
        if (check_file_access(cmdname->str_buf, SRCHFLAG_TO_FACC(search_flags))) found = 1;
    }

    return (found);
}
/***************************************************************/
int find_exe(struct cenv * ce,
                const char * cmdchar,
                int search_flags,
                struct str * cmdstr) {

    char *  path;
    int     found;

    found = 0;
    path = find_globalvar(ce->ce_gg, "PATH", 4);
    if (path) {
        found = search_path(ce, path, cmdchar, search_flags, cmdstr);
    }

    return (found);
}
/***************************************************************/
void normalize_cmd(struct cenv * ce, const char * cmdchar, struct str * cmdstr) {

    char *  cargv;
    char ** scargv;
    int     argc;
    char *  argv0;
    int     dotix;
    int     delimix;
    int     ii;
    struct str cmdexe;

    cargv = parse_command_line(cmdchar, &argc, NULL, 0);
    scargv = (char**)cargv;

    if (!argc) {
        copy_char_str(cmdstr, cmdchar);
    } else {
        argv0 = scargv[0];
        dotix = -1;
        init_str(&cmdexe, INIT_STR_CHUNK_SIZE);

        copy_char_str(&cmdexe, argv0);
        delimix = cmdexe.str_len - 1;
        while (delimix >= 0 && !IS_DIR_DELIM(argv0[delimix])) {
            if (dotix < 0 && argv0[delimix] == '.') dotix = delimix;
            delimix--;
        }

        if (dotix < 0) {
            append_str(&cmdexe, ".EXE");
        }
        init_str(cmdstr, INIT_STR_CHUNK_SIZE);

        if (!find_exe(ce, cmdexe.str_buf, SRCHFLAG_SEACH_READ, cmdstr)) {
            copy_char_str(cmdstr, cmdchar);
        } else {
            for (ii = 1; ii < argc; ii++) {
                if (cmdstr->str_len) APPCH(cmdstr, ' ');
                append_quoted(cmdstr, scargv[ii]);
            }
            APPCH(cmdstr, '\0');
        }
        free_str_buf(&cmdexe);
    }
    Free(cargv);
}
/***************************************************************/
int lookup_ext(struct cenv * ce,
                const char * ext,
                        struct extinf ** pxi) {
/*
*/
    int estat;
    struct extinf ** vhand;
    int elen;
    struct str cmdstr;
    char wtypbuf[ASSOC_MAX];
    char uext[EXT_MAXIMUM];

    estat = 0;

    for (elen = 0; ext[elen]; elen++) uext[elen] = toupper(ext[elen]);
    uext[elen] = '\0';

    if (!ce->ce_gg->gg_extinf) {
        ce->ce_gg->gg_extinf = dbtree_new();
        vhand = NULL;
    } else {
        vhand = (struct extinf **)dbtree_find(ce->ce_gg->gg_extinf,
                                uext, elen);
    }

    if (vhand) {
        (*pxi) = (*vhand);
    } else {
        estat = get_wtype(ce, uext, wtypbuf, sizeof(wtypbuf));
        if (!estat) {
             normalize_cmd(ce, wtypbuf, &cmdstr);
            (*pxi) = New(struct extinf, 1);
            (*pxi)->xi_cmdpath = New(char, cmdstr.str_len + 1);
            memcpy((*pxi)->xi_cmdpath, cmdstr.str_buf, cmdstr.str_len + 1);
            dbtree_insert(ce->ce_gg->gg_extinf,
                                uext, elen, (*pxi));
            free_str_buf(&cmdstr);
        }
    }

    return (estat);
}
/***************************************************************/
int winsub_parm1(struct cenv * ce,
                    const char * incmd,
                    const char * parm1,
                    struct str * outcmd) {

    int subbed;
    int ix;
    int ii;
/*
** "xxxx %%1 yy"  --> "xxxx %1 yy"
**  01234567890        0123456789
*/

    if (ce->ce_flags & CEFLAG_DEBUG) {
        fprintf(stderr, "in  cmd  : %s\n", incmd);
    }

    subbed = 0;
    outcmd->str_len = 0;
    ix = 0;
    while (incmd[ix]) {
        if (incmd[ix] == '%') {
            if (incmd[ix + 1] == '%') {
                APPCH(outcmd, '%');
                ix += 2;
            } else if (isdigit(incmd[ix + 1])) {
                if (incmd[ix + 1] == '1') {
                    for (ii = 0; parm1[ii]; ii++) APPCH(outcmd,parm1[ii]);
                }
                ix += 2;
                subbed = 1;
            } else if (incmd[ix + 1] == 'l' || incmd[ix + 1] == 'L') {
                /* This appears to be necessary because someone at  */
                /* Microsoft could not tell a %1 from a %l          */
                for (ii = 0; parm1[ii]; ii++) APPCH(outcmd,parm1[ii]);
                ix += 2;
                subbed = 1;
            } else if (incmd[ix + 1] == '*') {
                /* Ignore %* because it will be done anyway */
                /* Hopefully, this won't ever be a problem. */
                ix += 2;
            } else {
                APPCH(outcmd, incmd[ix]);
                ix++;
            }
        } else {
            APPCH(outcmd, incmd[ix]);
            ix++;
        }
    }
    APPCH(outcmd, '\0');

    if (ce->ce_flags & CEFLAG_DEBUG) {
        fprintf(stderr, "out cmd %d: %s\n", subbed, outcmd->str_buf);
    }

    return (subbed);
}
/***************************************************************/
#if IS_WINDOWS
int is_command_file(struct cenv * ce,
                    const char * parm0,
                    int dotix,
                    struct str * cmdname,
                    struct parmlist * pl) {

    int estat;
    int ftyp;
    struct extinf * xi;

    estat = 0;

    ftyp = file_stat(parm0);
    if (ftyp != FTYP_FILE) {
        estat = set_error(ce, ENOCMD, parm0);
    } else {
        if (dotix >= 0 && (ce->ce_flags & CEFLAG_WIN)) {
            estat = lookup_ext(ce, parm0 + dotix, &xi);
            if (!estat) {
                pl->pl_parm1_subbed = winsub_parm1(ce, xi->xi_cmdpath, parm0, cmdname);
            }
        }
    }

    return (estat);
}
#else
int is_command_file(struct cenv * ce,
                    const char * parm0,
                    int dotix,
                    struct str * cmdname,
                    struct parmlist * pl) {
/*
** Not used in practice.
**      Can be called if (ce->ce_flags & CEFLAG_WIN) and PATHEXT is used on Unix.
*/
    int estat;
    
    estat = 0;
    
    if (!check_file_access(parm0, SRCHFLAG_TO_FACC(SRCHFLAG_SEACH_XEQ))) {
        estat = set_error(ce, ENOCMD, parm0);
    } else {
    	copy_char_str(cmdname, parm0);
    }

    return (estat);
}
#endif
/***************************************************************/
int ext_in_pathext(struct cenv * ce,
                    const char * cmdname,
                    int cnlen,
                    int dotix) {

    int     found;
    int     peix;
    int     eq;
    int     ix;
    char * pathext;

    found = 0;
    pathext = find_globalvar(ce->ce_gg, "PATHEXT", 7);
    if (pathext) {
        peix = 0;
        while (!found && pathext[peix]) {
            eq = 1;
            ix = dotix;
            while (eq && pathext[peix] && pathext[peix] != ce->ce_gg->gg_path_delim) {
                if (toupper(pathext[peix]) == toupper(cmdname[ix++])) peix++;
                else eq = 0;
            }

            if (eq) {
                found = 1;
            } else {
                while (pathext[peix] && pathext[peix] != ce->ce_gg->gg_path_delim) {
                    peix++;
                }
                if (pathext[peix]) peix++; /* skip delim */
            }
        }
    }

    return (found);
}
/***************************************************************/
int search_pathext(struct cenv * ce,
                    const char * pathext,
                    struct str * cmdname) {

    int     found;
    int     peix;
    int     cnlen;
    int     ftyp;

    cnlen = cmdname->str_len;

    peix = 0;
    found = 0;
    while (!found && pathext[peix]) {
        cmdname->str_len = cnlen;
        while (pathext[peix] && pathext[peix] != ce->ce_gg->gg_path_delim) {
            APPCH(cmdname, pathext[peix]);
            peix++;
        }
        APPCH(cmdname, '\0');
        ftyp = file_stat(cmdname->str_buf);
        if (ftyp == FTYP_FILE)  found = 1;
        else if (pathext[peix]) peix++; /* skip delim */
    }

    return (found);
}
/***************************************************************/
int find_command_ext(struct cenv * ce,
                    const char * parm0,
                    int parm0len,
                    struct str * cmdname,
                    struct parmlist * pl) {

    int     estat;
    int     found;
    char * pathext;
    struct extinf * xi;
    struct str cmdext;

    estat = 0;
    pathext = find_globalvar(ce->ce_gg, "PATHEXT", 7);
    if (!pathext) {
        estat = set_error(ce, ENOPATHEXT, NULL);
    } else {
        cmdext.str_max = parm0len + 8;
        cmdext.str_buf = New(char, cmdext.str_max);
        memcpy(cmdext.str_buf, parm0, parm0len);
        cmdext.str_len = parm0len;
        APPCH(&cmdext, '\0');
        found = search_pathext(ce, pathext, &cmdext);
        if (!found) {
            estat = set_ferror(ce, EFINDCMD, "%s", parm0);
        } else {
            estat = lookup_ext(ce, cmdext.str_buf + parm0len, &xi);
            if (!estat) {
                pl->pl_parm1_subbed = winsub_parm1(ce, xi->xi_cmdpath, cmdext.str_buf, cmdname);
            }
        }
        free_str_buf(&cmdext);
    }

    return (estat);
}
/***************************************************************/
int find_command_file_path(struct cenv * ce,
                    const char * parm0,
                    int parm0len,
                    int dotix,
                    int search_flags,
                    struct str * cmdname,
                    struct parmlist * pl) {

    int     estat;
    int     found;
    char * path;
    struct extinf * xi;
    struct str cmdpath;
    struct filref ** vhand;
    struct filref * fr;

    estat = 0;

    if (!ce->ce_gg->gg_filref) {
        ce->ce_gg->gg_filref = dbtree_new();
        vhand = NULL;
    } else {
        vhand = (struct filref **)dbtree_find(ce->ce_gg->gg_filref,
                                parm0, parm0len);
    }

    if (vhand) {
        fr = (*vhand);
        copy_char_str(cmdname, fr->fr_cmdpath);
        pl->pl_parm1_subbed = fr->fr_parm_subbed;
    } else {
        path = find_globalvar(ce->ce_gg, "PATH", 4);
        if (!path) {
            estat = set_error(ce, ENOPATH, NULL);
        } else {
            init_str(&cmdpath, INIT_STR_CHUNK_SIZE);

            found = search_path(ce, path, parm0, search_flags, &cmdpath);
            if (!found) {
                estat = set_ferror(ce, EFINDCMD, "%s", parm0);
            } else {
                if (dotix >= 0 && (ce->ce_flags & CEFLAG_WIN)) {
                    estat = lookup_ext(ce, parm0 + dotix, &xi);
                    if (!estat) {
                        pl->pl_parm1_subbed = winsub_parm1(ce, xi->xi_cmdpath,
													cmdpath.str_buf, cmdname);

                        fr = New(struct filref, 1);
                        fr->fr_cmdpath = Strdup(cmdname->str_buf);
                        fr->fr_parm_subbed = pl->pl_parm1_subbed;
                        dbtree_insert(ce->ce_gg->gg_filref,
                                            parm0, parm0len, fr);
                    }
                } else {
                    qcopy_str(cmdname, &cmdpath);
                }
            }
            free_str_buf(&cmdpath);
        }
    }

    return (estat);
}
/***************************************************************/
int search_path_pathext(struct cenv * ce,
                    const char * path,
                    const char * pathext,
                    const char * parm0,
                    int search_flags,
                    struct str * cmdname,
                    int * pdotix) {

    int     found;
    int     pathix;
    int     pix;
    int     dot_searched;

    (*pdotix)    = -1;
    pathix       = 0;
    found        = 0;
    dot_searched = 0;

    while (!found && path[pathix]) {
        cmdname->str_len = 0;
        while (path[pathix] && path[pathix] != ce->ce_gg->gg_path_delim) {
            APPCH(cmdname, path[pathix]);
            pathix++;
        }
        if (cmdname->str_len) {
            if (cmdname->str_len == 1 && cmdname->str_buf[0] == '.') dot_searched = 1;
            if (cmdname->str_buf[cmdname->str_len - 1] != DIR_DELIM) {
                APPCH(cmdname, DIR_DELIM);
            }
            pix = 0;
            while (parm0[pix]) {
                APPCH(cmdname, parm0[pix]);
                pix++;
            }
            APPCH(cmdname, '\0');
            (*pdotix) = cmdname->str_len;
            found = search_pathext(ce, pathext, cmdname);
        }
        if (path[pathix]) pathix++; /* skip delim */
    }

    if (!found && !dot_searched && (search_flags & SRCHFLAG_SEACH_DOT)) {
        copy_char_str_len(cmdname, ".", 1);
        append_str_ch(cmdname, DIR_DELIM);
        append_str(cmdname, parm0);
        (*pdotix) = cmdname->str_len;
        found = search_pathext(ce, pathext, cmdname);
    }

    return (found);
}
/***************************************************************/
int find_command_file_pathext(struct cenv * ce,
                    const char * parm0,
                    int parm0len,
                    int search_flags,
                    struct str * cmdname,
                    struct parmlist * pl) {


    int     estat;
    int     found;
    int     dotix;
    char * path;
    char * pathext;
    struct extinf * xi;
    struct str cmdpath;
    struct filref ** vhand;
    struct filref * fr;

    estat = 0;

    if (!ce->ce_gg->gg_filref) {
        ce->ce_gg->gg_filref = dbtree_new();
        vhand = NULL;
    } else {
        vhand = (struct filref **)dbtree_find(ce->ce_gg->gg_filref,
                                parm0, parm0len);
    }

    if (vhand) {
        fr = (*vhand);
        copy_char_str(cmdname, fr->fr_cmdpath);
        pl->pl_parm1_subbed = fr->fr_parm_subbed;
    } else {
        pathext = find_globalvar(ce->ce_gg, "PATHEXT", 7);
        if (!pathext) {
            estat = set_error(ce, ENOPATHEXT, NULL);
        } else {
            path = find_globalvar(ce->ce_gg, "PATH", 4);
            if (!path) {
                estat = set_error(ce, ENOPATH, NULL);
            }
        }

        if (!estat) {
            init_str(&cmdpath, INIT_STR_CHUNK_SIZE);
            found = search_path_pathext(ce, path, pathext, parm0, search_flags,
                                            &cmdpath, &dotix);
            if (!found) {
                estat = set_ferror(ce, EFINDCMD, "%s", parm0);
            } else {
                if (dotix >= 0 && (ce->ce_flags & CEFLAG_WIN)) {
                    estat = lookup_ext(ce, cmdpath.str_buf + dotix, &xi);
                    if (!estat) {
                        pl->pl_parm1_subbed =
                            winsub_parm1(ce, xi->xi_cmdpath, cmdpath.str_buf, cmdname);

                        fr = New(struct filref, 1);
                        fr->fr_cmdpath = Strdup(cmdname->str_buf);
                        fr->fr_parm_subbed = pl->pl_parm1_subbed;
                        dbtree_insert(ce->ce_gg->gg_filref,
                                            parm0, parm0len, fr);

                    }
                }
            }
            free_str_buf(&cmdpath);
        }
    }

    return (estat);
}
/***************************************************************/
int find_command_file(struct cenv * ce,
                    const char * parm0,
                    int parm0len,
                    int dotix,
                    int search_flags,
                    struct str * cmdname,
                    struct parmlist * pl) {

    int     estat;

    if (dotix >= 0 || !(ce->ce_flags & CEFLAG_WIN)) {
        estat = find_command_file_path(ce, parm0, parm0len, dotix,
                                    search_flags, cmdname, pl);
    } else {
        estat = find_command_file_pathext(ce, parm0, parm0len, search_flags, cmdname, pl);
    }

    return (estat);
}
/***************************************************************/
/*
void do_pl_shift(struct parmlist * pl) {

    int ii;

    if (pl->pl_nparms > 0) {
        Free(pl->pl_cparmlist[0]);
        pl->pl_nparms -= 1;
    }

    for (ii = 0; ii < pl->pl_nparms; ii++) {
        pl->pl_cparmlist[ii] = pl->pl_cparmlist[ii + 1];
    }
    pl->pl_cparmlist[pl->pl_nparms] = NULL;
}
*/
/***************************************************************/
int prep_command(struct cenv * ce,
                    struct str * parm0,
                    int insert_parm0,
                    struct parmlist * pl) {
/*
** parm0 on in  Command name
** parm0 on out Fully qualified command
**       on out pl->pl_cparmlist[0] contains original parm0 on in
*/
    int     estat;
    int     cnlen;
    int     dotix;
    int     delimix;

/*
**      if (ce->ce_flags & CEFLAG_CASE) upshifts
*/

    estat = 0;
    cnlen = parm0->str_len;
    dotix = -1;

    delimix = cnlen-1;
    while (delimix >= 0 && !IS_DIR_DELIM(parm0->str_buf[delimix])) {
        if (dotix < 0 && parm0->str_buf[delimix] == '.') dotix = delimix;
        delimix--;
    }

    if (insert_parm0) {
    /* add pl_cparmlist[0] */
        pl->pl_nparms +=  1;
        if (pl->pl_nparms == pl->pl_mxparms) {
            pl->pl_mxparms += 1;
            pl->pl_cparmlist = Realloc(pl->pl_cparmlist, char*, pl->pl_mxparms);
        }
        memmove(&(pl->pl_cparmlist[1]), &(pl->pl_cparmlist[0]),
						pl->pl_nparms * sizeof(char*));
        qcopy_str_char(&(pl->pl_cparmlist[0]), parm0);
    }

    if (delimix >= 0) {
        if (dotix >= 0 || !(ce->ce_flags & CEFLAG_WIN)) {
            estat = is_command_file(ce, pl->pl_cparmlist[0], dotix, parm0, pl);
        } else {
            estat = find_command_ext(ce, pl->pl_cparmlist[0], cnlen, parm0, pl);
        }
    } else {
        /* Do not search PATH if ext not in PATHEXT, e.g. .jpg, .txt */ 
        if (dotix >= 0 && (ce->ce_flags & CEFLAG_WIN) &&
            !ext_in_pathext(ce, pl->pl_cparmlist[0], cnlen, dotix)) {
            estat = is_command_file(ce, pl->pl_cparmlist[0], dotix, parm0, pl);
        } else {
            estat = find_command_file(ce, pl->pl_cparmlist[0], cnlen, dotix,
                        IS_SEARCH_DOT(ce) | SRCHFLAG_SEACH_EXEC, parm0, pl);
        }
    }

/*
    if (!estat && pl->pl_parm1_subbed) {
        do_pl_shift(pl);
    }
*/
    return (estat);
}
/***************************************************************/
int wait_proclist(struct cenv * ce, void ** proclist, int nprocs, int rxwaitflags) {

    int     estat;
    int     excd;
    void * vrs;

    estat = 0;
    excd  = 0;
    vrs   = NULL;

    if (nprocs) {
        vrs = proclist[0];
    }

    if (vrs) {
        if (rx_wait(proclist, nprocs, rxwaitflags)) {
            estat = set_error(ce, EWAIT, rx_errmsg_list(proclist, nprocs));
        } else {
            excd |= rx_exitcode(proclist, nprocs);
        }
        rx_free(proclist, nprocs);
    }
    Free(proclist);

    ce->ce_rtn = excd;

    return (estat);
}
/***************************************************************/
int check_proclist(struct cenv * ce, void ** proclist, int nprocs, int rxwaitflags) {

    int     estat;
    int     excd;

    estat = 0;
    excd  = 0;

    if (proclist[0]) {
        if (rx_check_proclist(proclist, nprocs, rxwaitflags)) {
            estat = set_error(ce, EWAIT, rx_errmsg_list(proclist, nprocs));
        }
    }

    return (estat);
}
/***************************************************************/
void close_all_ce_filinf(struct cenv * ce) {

    if (ce->ce_prev_ce) {
        close_all_ce_filinf(ce->ce_prev_ce);
    }

    if (ce->ce_filinf) {
        if (!(ce->ce_filinf->fi_flags & FIFLAG_NOCLOSE)) {
	        CLOSE(ce->ce_filinf->fi_fileno);
        }
    }
}
/***************************************************************/
void ux_post_fork_cleanup(struct parmlist * pl) {

    close_all_ce_filinf(pl->pl_ce);

    if (pl->pl_piped_flags & PIPEFLAG_UNUSED_OUT) {
        CLOSE(pl->pl_unused_out);
    }
}
/***************************************************************/
void win_post_fork_show_unused(FILE * logf, struct parmlist * pl) {

    if (pl->pl_piped_flags & PIPEFLAG_UNUSED_IN) {
    	fprintf(logf,"%d closing pl->pl_unused_in=%d\n", rx_get_pid(), pl->pl_unused_in);
    }

    if (pl->pl_piped_flags & PIPEFLAG_UNUSED_OUT) {
    	fprintf(logf,"%d has pl->pl_unused_out=%d\n", rx_get_pid(), pl->pl_unused_out);
    }
}
/***************************************************************/
void show_ce_filinf(FILE * logf, struct cenv * ce) {

    if (ce->ce_prev_ce) {
        show_ce_filinf(logf, ce->ce_prev_ce);
    }

    if (ce->ce_filinf) {
        if (!(ce->ce_filinf->fi_flags & FIFLAG_NOCLOSE)) {
	        fprintf(logf,"%d ce->ce_filinf->fi_fileno=%d\n", rx_get_pid(), ce->ce_filinf->fi_fileno);
        }
    }
}
/***************************************************************/
void win_post_fork_cleanup(struct parmlist * pl) {
}
/***************************************************************/
void post_fork_cleanup(void * pevp) {

    ux_post_fork_cleanup((struct parmlist *)pevp);
}
/***************************************************************/
int run_command(struct cenv * ce,
                    struct str * parm0,
                    struct parmlist * pl) {
/*
** parm0 - Fully qualified command
**         on out pl->pl_cparmlist[0] contains original parm0 on in
*/
    int     estat;
    int run_flags;
    void * vrs;

    estat     = 0;
    run_flags = 0;
    if (pl->pl_redir_flags & REDIR_STDIN_MASK)  run_flags |= RUNFLAG_STDIN;
    if (pl->pl_redir_flags & REDIR_STDOUT_MASK) run_flags |= RUNFLAG_STDOUT;
    if (pl->pl_redir_flags & REDIR_STDERR_MASK) run_flags |= RUNFLAG_STDERR;
    if (ce->ce_flags & CEFLAG_DEBUG)            run_flags |= RUNFLAG_DEBUG;
    if (pl->pl_parm1_subbed)                    run_flags |= RUNFLAG_SKIP_PARM1;

#if IS_DEBUG_PARM0
    pl->pl_dbg_parm0 = Strdup(parm0->str_buf);
#endif

    vrs = rx_runx(parm0->str_buf,
    				pl->pl_nparms,
                    pl->pl_cparmlist,
                    run_flags,
                    pl->pl_ce->ce_gg->gg_envp,
                    pl->pl_ce->ce_stdin_f,
                    pl->pl_ce->ce_stdout_f,
                    pl->pl_ce->ce_stderr_f,
                    post_fork_cleanup,
                    pl);
    if (rx_stat(vrs)) {
        estat = set_error(ce, ERUN, rx_errmsg(vrs));
    } else if (pl->pl_vrs) {
        estat = set_error(ce, ERUN, "Internal error.  Program already started.");
    } else {
        pl->pl_vrs = vrs;
    }

    return (estat);
}
/***************************************************************/
int chndlr_generic(struct cenv * ce,
                struct str * rawparm0,
                struct str * parm0,
                struct str * cmdstr,
                int * cmdix,
                int xeqflags) {

    int estat;
    int estat2;
    struct parmlist pl;
    void ** vhand;
    struct usrfuncinfo * ufi;
    struct var result;

    estat = 0;

    estat = parse_parmlist(ce, cmdstr, cmdix, &pl);
    if (estat) return (estat);

    pl.pl_xeqflags = xeqflags;

    if (ce->ce_funclib && ce->ce_funclib->fl_funcs) {
        vhand = dbtree_find(ce->ce_funclib->fl_funcs,
							parm0->str_buf, parm0->str_len);
    } else {
        vhand = NULL;
    }
    if (vhand) {
        ufi = *(struct usrfuncinfo **)vhand;
        estat = call_usrfunc_strparms(ce, ufi, &pl, &result);
        free_var_buf(&result);
   } else {
        estat = prep_command(ce, rawparm0, 1, &pl);
        if (!estat) {
            estat = handle_redirs(ce, &pl);
        }

        if (!estat) {
            estat = run_command(ce, rawparm0, &pl);
        }
    }

    estat2 = free_parmlist_buf(ce, &pl);
    if (!estat) estat = estat2;
    else if (estat2) estat = chain_error(ce, estat, estat2);

    return (estat);
}
/***************************************************************/
int show_vars(struct cenv * ce, const char * tmsg, struct cenv * show_ce) {

    int estat;
    void * vdbtc;
    char * vnam;
    struct var ** vhand;
    struct var result;
    int nshown;

    estat = 0;
    nshown = 0;
    if (show_ce->ce_vars) {
        vdbtc = dbtree_rewind(show_ce->ce_vars, 0);
        vhand = (struct var **)dbtree_next_k(vdbtc, &vnam);

        while (vhand) {
            if ((*vhand)->var_typ == VTYP_STR) {
                printll(ce, "%s%s=%s\n", tmsg, vnam, (*vhand)->var_buf);
            } else {
                copy_var(&result, (*vhand));
                vensure_str(ce, &result);
                printll(ce, "%s%s=%s\n", tmsg, vnam, result.var_buf);
                free_var_buf(&result);
            }
            nshown++;
            vhand = (struct var **)dbtree_next_k(vdbtc, &vnam);
        }
        dbtree_close(vdbtc);
        if (!nshown) printll(ce, "no %s found\n", tmsg);
    }

    return (estat);
}
/***************************************************************/
int show_functions(struct cenv * ce, const char * tmsg, struct cenv * show_ce) {

    int estat;
    void * vdbtc;
    char * vnam;
    struct usrfuncinfo ** vhand;
    struct usrfuncinfo * ufi;
    struct str funcparms;
    int pix;
    int nshown;

    estat = 0;
    nshown = 0;
    init_str(&funcparms, INIT_STR_CHUNK_SIZE);

    if (show_ce->ce_vars) {
        vdbtc = dbtree_rewind(show_ce->ce_funclib->fl_funcs, 0);
        vhand = (struct usrfuncinfo **)dbtree_next_k(vdbtc, &vnam);

        while (vhand) {
            ufi = (*vhand);
            set_str_char(&funcparms, "(");
            for (pix = 0; pix < ufi->ufi_nparms; pix++) {
                if (pix) append_str_ch(&funcparms, ',');
                append_str_str(&funcparms, ufi->ufi_sparmlist[pix]);
            }
            append_str_ch(&funcparms, ')');

            printll(ce, "%s %s%s\n", tmsg, ufi->ufi_func_name, funcparms.str_buf);
            nshown++;
            vhand = (struct usrfuncinfo **)dbtree_next_k(vdbtc, &vnam);
        }
        dbtree_close(vdbtc);
        if (!nshown) printll(ce, "no %s found\n", tmsg);
    }

    free_str_buf(&funcparms);

    return (estat);
}
/***************************************************************/
int show_global_vars(struct cenv * ce) {

    int estat;
    char ** envp;
    int ii;

    estat = 0;
    envp = ce->ce_gg->gg_envp;
    if (ce->ce_flags & CEFLAG_DEBUG) {
        for (ii = 0; envp[ii]; ii++) {
            printll(ce, "global    %s\n", envp[ii]);
        }
    } else {
        for (ii = 0; envp[ii]; ii++) {
            printll(ce, "%s\n", envp[ii]);
        }
    }

    return (estat);
}
/***************************************************************/
int cmd_set_show(struct cenv * ce,
                struct str * cmdstr,
                int * cmdix) {

    int estat;
    int estat2;
    int nparms;
    struct parmlist pl;

    estat = 0;
    nparms = 0;

    estat = parse_parmlist(ce, cmdstr, cmdix, &pl);
    if (estat) return (estat);

    estat = handle_redirs(ce, &pl);
    if (estat) return (estat);

    if (ce->ce_flags & CEFLAG_DEBUG) {
        estat = show_functions(pl.pl_ce, "function ", ce);
        if (ce->ce_file_ce) estat = show_vars(pl.pl_ce, "local     ", ce);
        else                estat = show_vars(pl.pl_ce, "script    ", ce);
    } else {
        estat = show_vars(pl.pl_ce, "", ce);
    }

    if (ce->ce_file_ce) {
        if (ce->ce_flags & CEFLAG_DEBUG) {
            estat = show_vars(pl.pl_ce, "script    ", ce->ce_file_ce);
        } else {
            estat = show_vars(pl.pl_ce, "", ce->ce_file_ce);
        }
    }
    estat = show_global_vars(pl.pl_ce);

    estat2 = free_parmlist_buf(ce, &pl);
    if (!estat) estat = estat2;
    else if (estat2) estat = chain_error(ce, estat, estat2);

    return (estat);
}
/***************************************************************/
int is_good_vname(struct cenv * ce, const char * vnam) {

    int goodvnam;
    int ii;

    if (!vnam || !IS_FIRST_VARCHAR(vnam[0])) {
        goodvnam = 0;
    } else {
        goodvnam = 1;
        ii = 1;
        while (goodvnam && vnam[ii]) {
            if (IS_VARCHAR(vnam[ii])) ii++;
            else goodvnam = 0;
        }
    }

    return (goodvnam);
}
/***************************************************************/
void copy_fnam(struct cenv * ce,
        char ** pvnamtgt,
        int * pvnamlen,
        int * pvnammax,
        struct str * srcstr)
{
/*
**  Same as copy_vnam(), but
**      if (ce->ce_flags & CEFLAG_CASE) downshifts
*/
    int ii;

    if (ce->ce_flags & CEFLAG_CASE) {
        (*pvnammax) = srcstr->str_len + 1;
        (*pvnamtgt) = New(char, (*pvnammax));
        (*pvnamlen) = srcstr->str_len;
        for (ii = 0; ii < srcstr->str_len; ii++)
            (*pvnamtgt)[ii] = tolower(srcstr->str_buf[ii]);
        srcstr->str_buf[srcstr->str_len] = '\0';
        Free(srcstr->str_buf );
    } else {
        (*pvnamtgt) = srcstr->str_buf;
        (*pvnamlen) = srcstr->str_len;
        (*pvnammax) = srcstr->str_max;
    }
    srcstr->str_len = 0;
    srcstr->str_max = 0;
    srcstr->str_buf = NULL;
}
/***************************************************************/
void copy_vnam(struct cenv * ce,
        char ** pvnamtgt,
        int * pvnamlen,
        int * pvnammax,
        struct str * srcstr)
{
/*
**  Same as copy_fnam(), but
**      if (ce->ce_flags & CEFLAG_CASE) upshifts
*/
    int ii;

    if (ce->ce_flags & CEFLAG_CASE) {
        (*pvnammax) = srcstr->str_len + 1;
        (*pvnamtgt) = New(char, (*pvnammax));
        (*pvnamlen) = srcstr->str_len;
        for (ii = 0; ii < srcstr->str_len; ii++)
            (*pvnamtgt)[ii] = toupper(srcstr->str_buf[ii]);
        srcstr->str_buf[srcstr->str_len] = '\0';
        Free(srcstr->str_buf );
    } else {
        (*pvnamtgt) = srcstr->str_buf;
        (*pvnamlen) = srcstr->str_len;
        (*pvnammax) = srcstr->str_max;
    }
    srcstr->str_len = 0;
    srcstr->str_max = 0;
    srcstr->str_buf = NULL;
}
/***************************************************************/
void set_globalvar_ix(struct glob * gg,
                  int ix,
                  const char * vnam,
                  int vnlen,
                  const char * vval,
                  int vvlen) {

    int tlen;

    tlen = vnlen + vvlen + 2;

    gg->gg_envp[ix] = Realloc(gg->gg_envp[ix], char, tlen);

    memcpy(gg->gg_envp[ix], vnam, vnlen);
    gg->gg_envp[ix][vnlen] = '=';
    memcpy(gg->gg_envp[ix] + vnlen + 1, vval, vvlen);
    gg->gg_envp[ix][tlen - 1] = '\0';
}
/***************************************************************/
int read_to_new_line(struct cenv * ce, struct filinf * fi, struct str * fbuf) {

    int estat;
    char ch;
    int stix;
    int nc;

    estat           = 0;
    ch              = 0;
    fbuf->str_len   = 0;
    stix            = fi->fi_inbufix;

    do {
        if (fi->fi_inbufix < fi->fi_inbuflen) {
            ch = fi->fi_inbuf[fi->fi_inbufix];
            fi->fi_inbufix += 1;
        } else {
            nc = fi->fi_inbufix - stix;
            if (nc) {
                append_str_len(fbuf, fi->fi_inbuf + stix, nc);
            }
            estat = read_filinf(ce, fi, PROMPT_NONE, NULL);
            if (estat == ESTAT_EOF && fbuf->str_len) {
                estat = 0;
                ch = '\n';
            }
            stix = fi->fi_inbufix;
        }
    } while (!estat && ch != '\n');

    nc = fi->fi_inbufix - stix;
    if (nc) {
        append_str_len(fbuf, fi->fi_inbuf + stix, nc);
    }

    if (fbuf->str_len && fbuf->str_buf[fbuf->str_len - 1] == '\n') {
        fbuf->str_len -= 1;
    }
    if (fbuf->str_len && fbuf->str_buf[fbuf->str_len - 1] == '\r') {
        fbuf->str_len -= 1;
    }

    APPCH(fbuf, '\0');

    return (estat);
}
/***************************************************************/
static int get_toke_csv (const char * line,
                         int        * index,
                         char      ** token_buf,
                         int        * token_max,
                         int        * token_len,
                         const char   delims[]) {
/*
**  Parse a comma delimited token
**      Should work like Microsoft Excel
*/
    int done;
    int eol;
    char quot;

    (*token_len) = 0;
    eol = 0;
    done = 0;

    quot = 0;
    if (line[(*index)] == '\"') {
        quot = line[(*index)];
        (*index)++;
    }

    while (!done) {
        if (!line[(*index)]) {
            done = 1;
            if (!(*token_len)) eol = 1;
        } else {
            if ((*token_len) == (*token_len)) {
                (*token_max) += STR_CHUNK_SIZE;
                (*token_buf)  = Realloc((*token_buf), char, (*token_max));
            }

            if (quot) {
                if (line[(*index)] == quot) {
                    if (line[(*index) + 1] == quot) {
                        (*index)++;
                        (*token_buf)[(*token_len)++] = line[(*index)];
                    } else {
                        quot = 0;
                    }
                } else {
                    (*token_buf)[(*token_len)++] = line[(*index)];
                }
            } else {
                if (!isalnum(line[(*index)]) &&
                           delims[line[(*index)] & 0x7F]) {
                    done = 1;
                } else {
                    (*token_buf)[(*token_len)++] = line[(*index)];
                }
            }
            (*index)++;
        }
    }

    if ((*token_len) == (*token_len)) {
        (*token_max) += STR_CHUNK_SIZE;
        (*token_buf)  = Realloc((*token_buf), char, (*token_max));
    }
    (*token_buf)[(*token_len)] = 0;

    return (eol);
}
/***************************************************************/
int nendlr_for_csvparse(struct cenv * ce) {
/*
**  Needed for for ... csvparse()
**
**      #define NEFORTYP_CSVPARSE   desh.h
**      struct forparserec ...      desh.h
**      new_forparserec()           desh.c
**      free_forparserec()          desh.c
**      parse_for_csvparse()        exp.c
**      nendlr_for_csvparse()       cmd.c
*/

    int estat;
    struct nest * ne;
    int eol;
    struct forparserec * fpr;

    ne = ce->ce_nest;
    fpr = (struct forparserec *)ne->ne_data;

    estat = 0;
    eol = get_toke_csv(fpr->fpr_srcstr.str_buf,
                        &(fpr->fpr_ix),
                        &(fpr->fpr_parmstr.str_buf),
                        &(fpr->fpr_parmstr.str_max),
                        &(fpr->fpr_parmstr.str_len),
                        fpr->fpr_csvparms);

    if (!eol) {
        set_ce_var_str(ce, ne->ne_vnam, ne->ne_vnlen,
                        fpr->fpr_parmstr.str_buf, fpr->fpr_parmstr.str_len);
    } else {
        ne->ne_idle = NEIDLE_SKIP;
    }

    return (estat);
}
/***************************************************************/
int nendlr_for_in(struct cenv * ce) {

    int estat;
    struct nest * ne;
    char ** cparmlist;

    estat = 0;
    ne = ce->ce_nest;
    cparmlist = (char**)ne->ne_data;

    if (!cparmlist || !cparmlist[ne->ne_ix]) {
        ne->ne_idle = NEIDLE_SKIP;
    } else {
        set_ce_var_str(ce, ne->ne_vnam, ne->ne_vnlen,
            cparmlist[ne->ne_ix], Istrlen(cparmlist[ne->ne_ix]));
        ne->ne_ix += 1;
    }

    return (estat);
}
/***************************************************************/
int nendlr_for_parse(struct cenv * ce) {
/*
**  Needed for for ... parse()
**
**      #define NEFORTYP_PARSE      desh.h
**      struct forparserec ...      desh.h
**      new_forparserec()           desh.c
**      free_forparserec()          desh.c
**      parse_for_parse()           exp.c
**      nendlr_for_parse()          cmd.c
*/

    int estat;
    struct nest * ne;
    struct forparserec * fpr;

    ne = ce->ce_nest;
    fpr = (struct forparserec *)ne->ne_data;

    estat = 0;

    while (isspace(fpr->fpr_srcstr.str_buf[fpr->fpr_ix])) (fpr->fpr_ix)++;

    if (fpr->fpr_srcstr.str_buf[fpr->fpr_ix]) {
        fpr->fpr_parmstr.str_len = 0;
        estat = get_parmstr(ce, &(fpr->fpr_srcstr), &(fpr->fpr_ix),
                                PFLAG_FOR_PARSE, &(fpr->fpr_parmstr));
        if (!estat) {
            set_ce_var_str(ce, ne->ne_vnam, ne->ne_vnlen,
                            fpr->fpr_parmstr.str_buf, fpr->fpr_parmstr.str_len);
        }
    } else {
        ne->ne_idle = NEIDLE_SKIP;
    }

    return (estat);
}
/***************************************************************/
int nendlr_for_read(struct cenv * ce) {

    int estat;
    struct nest * ne;
    struct forreadrec * frr;

    ne = ce->ce_nest;
    frr = (struct forreadrec *)ne->ne_data;

    estat = read_to_new_line(ce, frr->frr_fi, &(frr->frr_str));

    if (estat) {
        ne->ne_idle = NEIDLE_SKIP;
        if (estat == ESTAT_EOF) estat = 0;
    } else {
        set_ce_var_str(ce, ne->ne_vnam, ne->ne_vnlen,
                        frr->frr_str.str_buf, frr->frr_str.str_len);
    }

    return (estat);
}
/***************************************************************/
int nendlr_for_readdir(struct cenv * ce) {

    int estat;
    int derr;
    int is_eof;
    struct nest * ne;
    struct forreaddirrec * frdr;
    char dirent[PATH_MAXIMUM + 2];

    ne = ce->ce_nest;
    frdr = (struct forreaddirrec *)ne->ne_data;

    estat = 0;
    derr = read_directory(frdr->frdr_rdr, dirent, sizeof(dirent), &is_eof);
    if (derr) {
        estat = set_error(ce, EREADFORDIR, get_errmsg_directory(frdr->frdr_rdr));
    } else if (is_eof) {
        ne->ne_idle = NEIDLE_SKIP;
    } else {
        set_ce_var_str(ce, ne->ne_vnam, ne->ne_vnlen,
                        dirent, Istrlen(dirent));
    }

    return (estat);
}
/***************************************************************/
int nendlr_for_downto(struct cenv * ce) {

    int estat;
    struct nest * ne;

    estat = 0;
    ne = ce->ce_nest;

    set_ce_var_num(ce, ne->ne_vnam, ne->ne_vnlen,  ne->ne_from_val);

    if (ne->ne_from_val < ne->ne_to_val || ne->ne_for_done) {
        ne->ne_idle = NEIDLE_SKIP;
    } else {
        if (ne->ne_from_val == ne->ne_to_val) {
            ne->ne_for_done = 1;  /* needed for min int */
        }
        ne->ne_from_val -= 1;
    }

    return (estat);
}
/***************************************************************/
int nendlr_for_to(struct cenv * ce) {

    int estat;
    struct nest * ne;

    estat = 0;
    ne = ce->ce_nest;

    set_ce_var_num(ce, ne->ne_vnam, ne->ne_vnlen,  ne->ne_from_val);

    if (ne->ne_from_val > ne->ne_to_val || ne->ne_for_done) {
        ne->ne_idle = NEIDLE_SKIP;
    } else {
        if (ne->ne_from_val == ne->ne_to_val) {
            ne->ne_for_done = 1;  /* needed for max int */
        }
        ne->ne_from_val += 1;
    }

    return (estat);
}
/***************************************************************/
int read_command_string(struct cenv * ce, struct str * linstr, int ptyp) {

    int estat;
    int len;
    char * cmd;

    estat = 0;

    if (ce->ce_reading_saved) {
        cmd = get_next_saved_cmd(ce);
        if (!cmd) {
            estat = ESTAT_EOF;
        } else {
            len = Istrlen(cmd);
            if (len >= linstr->str_max) {
                linstr->str_max = len + 1;
                linstr->str_buf = Realloc(linstr->str_buf, char, linstr->str_max);
            }
            memcpy(linstr->str_buf, cmd, len + 1);
            linstr->str_len = len;
        }
    } else {
        estat = read_line(ce, ce->ce_filinf, linstr, ptyp, NULL);
    }

    return (estat);
}
/***************************************************************/
int sub_str_back_quote(struct cenv * ce,
                        const char * cmdbuf,
                        int * ix,
                        struct str * subbedstr) {
/*
*/
    int estat;
    int stix;
    struct str cmdstr;
    struct str cmdout;

    estat = 0;
    init_str(&cmdstr, INIT_STR_CHUNK_SIZE);
    init_str(&cmdout, 0);

    stix = (*ix);

    (*ix)++;
    while (cmdbuf[*ix] && cmdbuf[*ix] != '`') {
        APPCH(&cmdstr, cmdbuf[*ix]);
        (*ix)++;
    }
    APPCH(&cmdstr, '\0');
    if (cmdbuf[*ix] == '`') (*ix)++;

    estat = execute_command_string(ce, cmdstr.str_buf, &cmdout);

    if (!estat) append_str_str(subbedstr, &cmdout);

    free_str_buf(&cmdout);
    free_str_buf(&cmdstr);

    return (estat);
}
/***************************************************************/
int sub_str(struct cenv * ce, struct str * linstr) {

    int estat;
    int testat;
    int ix;
    int stix;
    char ch;
    struct str subbedstr;

    estat       = 0;
    stix = 0;
    ix = 0;
    ch = linstr->str_buf[ix];
    init_str(&subbedstr, 0);

    while (ch) {
        if (ch == '$') {
            append_str_len(&subbedstr, linstr->str_buf + stix, ix - stix);
            ix++;
            testat  = sub_parm(ce, linstr->str_buf, &ix, &subbedstr, SUBPARM_NONE);
            if (!estat) estat = testat;
            stix = ix;
        } else if (ch == '`') {
            append_str_len(&subbedstr, linstr->str_buf + stix, ix - stix);
            testat  = sub_str_back_quote(ce, linstr->str_buf, &ix, &subbedstr);
            if (!estat) estat = testat;
            stix = ix;
        } else {
            ix++;
        }
        ch = linstr->str_buf[ix];
    }
    append_str_len(&subbedstr, linstr->str_buf + stix, ix - stix);
    APPCH(&subbedstr, '\0');

    qcopy_str(linstr, &subbedstr);

    return (estat);
}
/***************************************************************/
int open_stdin_here(struct cenv * ce, struct parmlist * pl, FILET *fnum) {

    int estat;
    int elen;
    int ix;
    int rtn;
    int done;
    int linix;
    int subbing;
    struct str linstr;

    estat       = 0;
    done        = 0;
    (*fnum)     = -1;
    elen        = Istrlen(pl->pl_stdin_fnam);
    init_str(&linstr, INIT_STR_CHUNK_SIZE);

    subbing = 1;
    if ((pl->pl_redir_flags & REDIR_STDIN_MASK) == REDIR_STDIN_HERE_NOSUB)
			subbing = 0;

    pl->pl_stdin_tf = tmpfile_open();
    if (!pl->pl_stdin_tf) {
        estat = set_ferror(ce, EOPENTMPFIL, "%m", ERRNO);
        return (estat);
    }

    while (!estat && !done) {
        estat = read_command_string(ce, &linstr, PROMPT_GT);
        if (!estat) {

            if (saving_cmds(ce) && !ce->ce_reading_saved) {
                linix = 0;
                save_cmd(ce, &linstr, &linix);
            }

            if (linstr.str_len >= elen &&
					!CEMEMCMP(ce, linstr.str_buf, pl->pl_stdin_fnam, elen)) {
                ix = elen;
                while (isspace(linstr.str_buf[ix])) ix++;
                if (!linstr.str_buf[ix]) done = 1;
            }
            
            if (!done) {
                APPCH(&linstr, '\n');
                APPCH(&linstr, '\0');
                if (subbing) {
                    /* estat = */ sub_str(ce, &linstr);
                }
                rtn = tmpfile_write(pl->pl_stdin_tf, linstr.str_buf, linstr.str_len);
                if (rtn <= 0) {
                    estat = set_ferror(ce, EWRITETMPFIL, "%m", ERRNO);
                }
            }
        }
    }
    free_str_buf(&linstr);

    if (estat) {
        tmpfile_close(pl->pl_stdin_tf);
        pl->pl_stdin_tf = NULL;
    } else {
        (*fnum) = tmpfile_reset(pl->pl_stdin_tf);
        if ((*fnum) < 0) {
            estat = set_ferror(ce, ERESETTMPFIL, "%m", ERRNO);
        }
    }

    return (estat);
}
/***************************************************************/
int open_for_redir(struct cenv * ce,
                    const char * fname,
                    int opmode,
                    FILET *fnum,
                    int *opened) {

/*
**  opmode:
**      0 - read only
**      1 - write
**      2 - append
**
**  To get Windows handle, call
**
**      long _get_osfhandle( int filehandle );
**
*/
    int estat;
    int omod;
    int pmode;

    estat = 0;
    pmode = FSTAT_S_IREAD | FSTAT_S_IWRITE;
    (*opened) = 0;

         if (opmode == 0) omod = _O_RDONLY;
    else if (opmode == 1) omod = _O_WRONLY | _O_CREAT | _O_TRUNC;
    else                  omod = _O_WRONLY | _O_CREAT | _O_APPEND;

    (*fnum) = OPEN3(fname, omod, pmode);
    if ((*fnum) < 0) {
        estat = set_ferror(ce, EOPENREDIR, "%s %m", fname, ERRNO);
    } else {
        (*opened) = 1;
        if (opmode == 2)  {
             LSEEK((*fnum), 0, SEEK_END ); /* necessary on Windows */
        }
    }

    return (estat);
}
/***************************************************************/
int redir_fnum(struct cenv * ce,
                    const char * f_name,
                    FILET *fnum,
                    FILET   stdin_f,
                    FILET   stdout_f,
                    FILET   stderr_f) {

    int estat;

    estat = 0;

    if (f_name[0] == '&' && isdigit(f_name[1])) {
               if (!strcmp(f_name, "&0")) {
            (*fnum) = stdin_f;
        } else if (!strcmp(f_name, "&1")) {
            (*fnum) = stdout_f;
        } else if (!strcmp(f_name, "&2")) {
            (*fnum) = stderr_f;
        } else {
            estat = set_error(ce, ENOREDIR, NULL);
        }
    }

    return (estat);
}
/***************************************************************/
int handle_std_redirs(struct cenv * ce,
                    struct parmlist * pl) {

    int     estat;
    FILET   stdin_f;
    FILET   stdout_f;
    FILET   stderr_f;
    int     close_flags;
    int     fopened;

    estat = 0;
    fopened = 0;
    close_flags = 0;

    if (pl->pl_redir_flags & REDIR_STDIN_MASK) {
        if (pl->pl_stdin_fnam[0] == '&' && isdigit(pl->pl_stdin_fnam[1])) {
            stdin_f = -1;
        } else {
            if (pl->pl_redir_flags & REDIR_STDIN_HERE) {
                estat = open_stdin_here(ce, pl, &stdin_f);
            } else {
                estat = open_for_redir(ce, pl->pl_stdin_fnam, 0, &stdin_f, &fopened);
            }
            if (fopened) close_flags |= FREECE_STDIN;
        }
    } else {
        stdin_f = ce->ce_stdin_f;
    }

    if (!estat) {
        if (pl->pl_redir_flags & REDIR_STDOUT_MASK) {
            if (pl->pl_stdout_fnam[0] == '&' && isdigit(pl->pl_stdout_fnam[1])) {
                stdout_f = -1;
            } else {
                if (pl->pl_redir_flags & REDIR_STDOUT_APPEND) {
                    estat = open_for_redir(ce, pl->pl_stdout_fnam, 2, &stdout_f, &fopened);
                } else {
                    estat = open_for_redir(ce, pl->pl_stdout_fnam, 1, &stdout_f, &fopened);
                }
                if (fopened) close_flags |= FREECE_STDOUT;
            }
        } else {
            stdout_f = ce->ce_stdout_f;
        }
    }

    if (!estat) {
        if (pl->pl_redir_flags & REDIR_STDERR_MASK) {
            if (pl->pl_stderr_fnam[0] == '&' && isdigit(pl->pl_stderr_fnam[1])) {
                stderr_f = -1;
            } else {
                if (pl->pl_redir_flags & REDIR_STDERR_APPEND) {
                    estat = open_for_redir(ce, pl->pl_stderr_fnam, 2, &stderr_f, &fopened);
                } else {
                    estat = open_for_redir(ce, pl->pl_stderr_fnam, 1, &stderr_f, &fopened);
                }
                if (fopened) close_flags |= FREECE_STDERR;
            }
        } else {
            stderr_f = ce->ce_stderr_f;
        }
    }

    if (!estat && stdin_f < 0) {
        estat = redir_fnum(ce, pl->pl_stdin_fnam , &stdin_f , stdin_f, stdout_f, stderr_f);
    }

    if (!estat && stdout_f < 0) {
        estat = redir_fnum(ce, pl->pl_stdout_fnam, &stdout_f, stdin_f, stdout_f, stderr_f);
    }

    if (!estat && stderr_f < 0) {
        estat = redir_fnum(ce, pl->pl_stderr_fnam, &stderr_f, stdin_f, stdout_f, stderr_f);
    }

    if (estat) {
        pl->pl_ce = NULL;
    } else {
        pl->pl_ce = new_cenv(stdin_f,
                        stdout_f,
                        stderr_f,
                        NULL,
                        close_flags,
                        pl->pl_nparms,
                        pl->pl_cparmlist,
                        ce->ce_flags,
                        ce->ce_funclib,
                        ce->ce_gg,
                        NULL,
                        ce);
    }

    return (estat);
}
/***************************************************************/
int run_piped(struct cenv * ce, struct parmlist * pl) {

    int estat;
    int pix;
    void ** vhand;
    struct cmdinfo ** hci;
    struct str parm0;

    estat = 0;
    pix = 0;

    hci = (struct cmdinfo **)dbtree_find(ce->ce_gg->gg_cmds,
                                pl->pl_cparmlist[0], pl->pl_cparmlens[0] + 1);

    if (hci) {
        estat = set_error(ce, EPIPETOINT, pl->pl_cparmlist[0]);
    } else {
        if (ce->ce_funclib && ce->ce_funclib->fl_funcs) {
            vhand = dbtree_find(ce->ce_funclib->fl_funcs, 
                    pl->pl_cparmlist[0], pl->pl_cparmlens[0]);
        } else {
            vhand = NULL;
        }
        if (vhand) {
            estat = set_error(ce, EPIPETOFUNC, pl->pl_cparmlist[0]);
        }
    }

    if (!estat) {
        init_str(&parm0, pl->pl_cparmlens[0] + 1);
        append_str_len(&parm0, pl->pl_cparmlist[0], pl->pl_cparmlens[0]);

        estat = prep_command(ce, &parm0, 0, pl);
        if (!estat) {
            estat = run_command(ce, &parm0, pl);
        }
        free_str_buf(&parm0);
    }

    return (estat);
}
/***************************************************************/
int open_piped_redir(struct cenv * ce,
                            FILET * pipein_f,
                            FILET * pipeout_f)
{
    int estat;
    int rtn;
    int ern;
    int pipefd[2];

    estat = 0;

    rtn = PIPE(pipefd);  /* create non-inheritable pipe */
    if (rtn < 0) {
        ern = ERRNO;
        estat = set_ferror(ce, EOPENPIPE,"%m", ern);
    } else {
        (*pipein_f) = pipefd[0];
        (*pipeout_f) = pipefd[1];
    }

    return (estat);
}
/***************************************************************/
int handle_piped_to_redirs(struct cenv * ce,
                            FILET * pipeout_f,
                            struct parmlist * pl);

int handle_piped_from_redirs(struct cenv * ce,
                            FILET * pipeout_f,
                            struct parmlist * pl) {
    int     estat;
    FILET   new_pipein_f;
    FILET   new_pipeout_f;
    FILET   stderr_f;
    int     fopened;
    int     close_flags;

    estat = 0;
    fopened = 0;
    close_flags = 0;


    if (pl->pl_redir_flags & REDIR_STDERR_MASK) {
        if (pl->pl_stderr_fnam[0] == '&' && isdigit(pl->pl_stderr_fnam[1])) {
            stderr_f = -1;
        } else {
            if (pl->pl_redir_flags & REDIR_STDERR_APPEND) {
                estat = open_for_redir(ce, pl->pl_stderr_fnam, 2, &stderr_f, &fopened);
            } else {
                estat = open_for_redir(ce, pl->pl_stderr_fnam, 1, &stderr_f, &fopened);
            }
            if (estat) return (estat);
            if (fopened) close_flags |= FREECE_STDERR;
        }
    } else {
        stderr_f = ce->ce_stderr_f;
    }

    if (pl->pl_redir_flags & REDIR_STDOUT_MASK) {
        estat = set_error(ce, ESTDOUTPIPED, NULL);
    } else {
        estat = handle_piped_to_redirs(ce, &new_pipeout_f, pl->pl_piped_parmlist);

        if (!estat) {
            estat = open_piped_redir(ce, &new_pipein_f, pipeout_f);
        }

        if (stderr_f < 0) {
            estat = redir_fnum(ce, pl->pl_stderr_fnam,
							&stderr_f, new_pipein_f, new_pipeout_f, stderr_f);
        }

        if (!estat) {
            pl->pl_ce = new_cenv(new_pipein_f,
                            new_pipeout_f,
                            stderr_f,
                            NULL,
                            close_flags,
                            pl->pl_nparms,
                            pl->pl_cparmlist,
                            ce->ce_flags,
                            ce->ce_funclib,
                            ce->ce_gg,
                            NULL,
                            ce);

            pl->pl_redir_flags |=  REDIR_STDIN_FILE;
            pl->pl_piped_flags |= PIPEFLAG_UNUSED_OUT;
            pl->pl_unused_out  = (*pipeout_f);

            estat = run_piped(ce, pl);
        }
        if (estat) {
            CLOSE(new_pipeout_f);
        } else {
            CLOSE(new_pipein_f);     /* close parent's read handle */ /* 8/29/12 */
            CLOSE(new_pipeout_f);   /* close parent's write handle */
            pl->pl_redir_flags |=  REDIR_STDOUT_FILE;
            pl->pl_redir_flags |=  REDIR_STDIN_FILE;
        }
    }

    return (estat);
}
/***************************************************************/
int handle_piped_to_redirs(struct cenv * ce,
                            FILET * pipeout_f,
                            struct parmlist * pl) {

    int     estat;
    FILET   pipein_f;

    estat = 0;

    if (pl->pl_redir_flags & REDIR_STDIN_MASK) {
        estat = set_error(ce, ESTDINPIPED, NULL);
    } else {
        if (pl->pl_piped_parmlist) {
            estat = handle_piped_from_redirs(ce, pipeout_f, pl);
        } else {
            estat = open_piped_redir(ce, &pipein_f, pipeout_f);
            if (!estat) {
                estat = handle_std_redirs(ce, pl);
            }
            if (!estat) {
                pl->pl_piped_flags |= PIPEFLAG_UNUSED_OUT;
                pl->pl_unused_out  = (*pipeout_f);
                pl->pl_ce->ce_stdin_f = pipein_f;
                pl->pl_redir_flags |=  REDIR_STDIN_FILE;

                estat = run_piped(ce, pl);
                CLOSE(pipein_f);
            }
        }
   }

    return (estat);
}
/***************************************************************/
int handle_piped_redirs(struct cenv * ce, struct parmlist * pl) {

    int     estat;
    FILET   stdin_f;
    FILET   stdout_f;
    FILET   stderr_f;
    FILET   pipeout_f;
    int     close_flags;
    int     fopened;

    estat = 0;
    fopened = 0;
    close_flags = 0;

    if (pl->pl_redir_flags & REDIR_STDIN_MASK) {
        if (pl->pl_stdin_fnam[0] == '&' && isdigit(pl->pl_stdin_fnam[1])) {
            stdin_f = -1;
        } else {
            if (pl->pl_redir_flags & REDIR_STDIN_HERE) {
                estat = open_stdin_here(ce, pl, &stdin_f);
            } else {
                estat = open_for_redir(ce, pl->pl_stdin_fnam, 0, &stdin_f, &fopened);
            }
            if (fopened) close_flags |= FREECE_STDIN;
        }
    } else {
        stdin_f = ce->ce_stdin_f;
    }

    if (pl->pl_redir_flags & REDIR_STDOUT_MASK) {
        estat = set_error(ce, ESTDOUTPIPED, NULL);
    } else {
        estat = handle_piped_to_redirs(ce, &pipeout_f, pl->pl_piped_parmlist);
        if (!estat) {
            stdout_f = pipeout_f;

            pl->pl_redir_flags |=  REDIR_STDOUT_FILE;
            close_flags |= FREECE_STDOUT;
        }
    }

    if (!estat && (pl->pl_redir_flags & REDIR_STDERR_MASK)) {
        if (pl->pl_stderr_fnam[0] == '&' && isdigit(pl->pl_stderr_fnam[1])) {
            stderr_f = -1;
        } else {
            if (pl->pl_redir_flags & REDIR_STDERR_APPEND) {
                estat = open_for_redir(ce, pl->pl_stderr_fnam, 2, &stderr_f, &fopened);
            } else {
                estat = open_for_redir(ce, pl->pl_stderr_fnam, 1, &stderr_f, &fopened);
            }
            if (fopened) close_flags |= FREECE_STDERR;
        }
    } else {
        stderr_f = ce->ce_stderr_f;
    }

    if (!estat && stdin_f < 0) {
        estat = redir_fnum(ce, pl->pl_stdin_fnam , &stdin_f , stdin_f, stdout_f, stderr_f);
    }

    if (!estat && stdout_f < 0) {
        estat = redir_fnum(ce, pl->pl_stdout_fnam, &stdout_f, stdin_f, stdout_f, stderr_f);
    }

    if (!estat && stderr_f < 0) {
        estat = redir_fnum(ce, pl->pl_stderr_fnam, &stderr_f, stdin_f, stdout_f, stderr_f);
    }

    if (estat) {
        pl->pl_ce = NULL;
    } else {
        pl->pl_ce = new_cenv(stdin_f,
                        stdout_f,
                        stderr_f,
                        NULL,
                        close_flags,
                        pl->pl_nparms,
                        pl->pl_cparmlist,
                        ce->ce_flags,
                        ce->ce_funclib,
                        ce->ce_gg,
                        NULL,
                        ce);
    }

    return (estat);
}
/***************************************************************/
int handle_redirs(struct cenv * ce, struct parmlist * pl) {

    int     estat;

    if (pl->pl_piped_parmlist) {
        estat = handle_piped_redirs(ce, pl);
    } else {
        estat = handle_std_redirs(ce, pl);
    }

    return (estat);
}
/***************************************************************/
int chndlr_echo(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix) {
/*
Syntax: (debug only)

    echo [ <parm> ] [ ...]                     Displays parms
*/
    int estat;
    int estat2;
    int nparms;
    struct parmlist pl;

    estat = 0;
    nparms = 0;

    estat = parse_parmlist(ce, cmdstr, cmdix, &pl);
    if (estat) return (estat);

    estat = handle_redirs(ce, &pl);
    if (estat) return (estat);

    while (!estat && nparms < pl.pl_ce->ce_argc) {
        if (nparms) {
            estat = printll(pl.pl_ce, " %s", pl.pl_ce->ce_argv[nparms]);
        } else {
            estat = printll(pl.pl_ce, "%s", pl.pl_ce->ce_argv[nparms]);
        }
        nparms++;            
    }
    if (!estat) estat = printll(pl.pl_ce, "\n");

    estat2 = free_parmlist_buf(ce, &pl);
    if (!estat) estat = estat2;
    else if (estat2) estat = chain_error(ce, estat, estat2);

    return (estat);
}
/***************************************************************/
int chndlr_dot(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix) {
/*
    . file [ <parm> ] [ ...]                   Reads and executes from file
*/
    int estat;
    int estat2;
    int nparms;
    int delimix;
    struct parmlist pl;
    FILET cmdfil;
    struct filinf * fi;
    struct str cmdname;

    estat = 0;
    nparms = 0;

    estat = parse_parmlist(ce, cmdstr, cmdix, &pl);
    if (estat) return (estat);

    if (!pl.pl_nparms) {
        estat = set_error(ce, EEXPFILE, NULL);
        return (estat);
    }

    delimix = pl.pl_cparmlens[0] - 1;
    while (delimix >= 0 && !IS_DIR_DELIM(pl.pl_cparmlist[0][delimix])) {
        delimix--;
    }
    init_str(&cmdname, INIT_STR_CHUNK_SIZE);
    
    if (delimix < 0) {
        if (!find_exe(ce, pl.pl_cparmlist[0], IS_SEARCH_DOT(ce) | SRCHFLAG_SEACH_READ, &cmdname)) {
            cmdname.str_buf[0] = '\0';
        }
    } else {
        if (file_stat(pl.pl_cparmlist[0]) != FTYP_FILE) {
            estat = set_error(ce, EFINDFILE, pl.pl_cparmlist[0]);
        }
        copy_char_str_len(&cmdname, pl.pl_cparmlist[0], pl.pl_cparmlens[0]);
    }

    if (!estat) {
        estat = handle_redirs(ce, &pl);
    }

    if (!estat) {
        cmdfil = OPEN(cmdname.str_buf, _O_RDONLY);
        if (cmdfil < 0) {
            estat = set_ferror(ce, EOPENFILE, "%s %m", cmdname.str_buf, ERRNO);
        }
    }

    if (!estat) {
        fi = new_filinf(cmdfil, cmdname.str_buf, CMD_LINEMAX);

        /* Fixup pl.pl_ce for proper behavior */
        pl.pl_ce->ce_filinf      = fi;
        if (!ce->ce_vars) ce->ce_vars = dbtree_new();
        pl.pl_ce->ce_vars        = ce->ce_vars;
        pl.pl_ce->ce_close_flags |= FREECE_STMTS;

        estat = read_and_execute_commands_from_file(pl.pl_ce, cmdname.str_buf);
    }

    free_str_buf(&cmdname);

    estat2 = free_parmlist_buf(ce, &pl);
    if (!estat) estat = estat2;
    else if (estat2) estat = chain_error(ce, estat, estat2);

    return (estat);
}
/***************************************************************/
int chndlr_call(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix) {
/*
    call file [ <parm> ] [ ...]                   Reads and executes from file
*/
    int estat;
    int estat2;
    int nparms;
    int delimix;
    struct parmlist pl;
    FILET cmdfil;
    struct filinf * fi;
    struct str cmdname;

    estat = 0;
    nparms = 0;

    estat = parse_parmlist(ce, cmdstr, cmdix, &pl);
    if (estat) return (estat);

    if (!pl.pl_nparms) {
        estat = set_error(ce, EEXPFILE, NULL);
        return (estat);
    }

    delimix = pl.pl_cparmlens[0] - 1;
    while (delimix >= 0 && !IS_DIR_DELIM(pl.pl_cparmlist[0][delimix])) {
        delimix--;
    }
    init_str(&cmdname, INIT_STR_CHUNK_SIZE);
    
    /* call always searches dot */
    if (delimix < 0) {
        if (!find_exe(ce, pl.pl_cparmlist[0], SRCHFLAG_SEACH_DOT | SRCHFLAG_SEACH_READ, &cmdname)) {
            cmdname.str_buf[0] = '\0';
        }
    } else {
        if (file_stat(pl.pl_cparmlist[0]) != FTYP_FILE) {
            estat = set_error(ce, EFINDFILE, pl.pl_cparmlist[0]);
        }
        copy_char_str_len(&cmdname, pl.pl_cparmlist[0], pl.pl_cparmlens[0]);
    }

    if (!estat) {
        estat = handle_redirs(ce, &pl);
    }

    if (!estat) {
        cmdfil = OPEN(cmdname.str_buf, _O_RDONLY);
        if (cmdfil < 0) {
            estat = set_ferror(ce, EOPENFILE, "%s %m", cmdname.str_buf, ERRNO);
        }
    }

    if (!estat) {
        fi = new_filinf(cmdfil, cmdname.str_buf, CMD_LINEMAX);

        /* Fixup pl.pl_ce for proper behavior */
        pl.pl_ce->ce_filinf      = fi;
        if (!ce->ce_vars) ce->ce_vars = dbtree_new();
        pl.pl_ce->ce_file_ce      = pl.pl_ce;
        pl.pl_ce->ce_funclib      = new_funclib();
        pl.pl_ce->ce_close_flags |= FREECE_STMTS | FREECE_VARS | FREECE_FUNCS;

        estat = read_and_execute_commands_from_file(pl.pl_ce,  cmdname.str_buf);
        if (estat) ce->ce_rtn = pl.pl_ce->ce_rtn;
    }

    free_str_buf(&cmdname);

    estat2 = free_parmlist_buf(ce, &pl);
    if (!estat) estat = estat2;
    else if (estat2) estat = chain_error(ce, estat, estat2);

    return (estat);
}
/***************************************************************/
void free_nest_mess(struct cenv * ce, int free_depth) {

    struct nest * ne;
    struct nest * ne_prev;  /* Added 7/7/2014 */
    int depth;

    ne = ce->ce_nest;
    depth = 0;

    while (ne && depth < free_depth) {
        ne_prev = ne->ne_prev;
        Free(ne->ne_data);
        free_nest(ce, ne);
        if (!saving_cmds(ce)) {
            delete_cmds(ce);
        }
        ne = ne_prev;
        depth++;
    }
}
/***************************************************************/
void clean_nest_mess(struct cenv * ce, int end_typ, int alt_end_typ) {

    struct nest * ne;
    int depth;
    int done;
    int free_mess;

    ne = ce->ce_nest;
    depth = 0;
    done  = 0;
    free_mess  = 0;

    while (ne && !done) {
        if (end_typ == NETYP_FILE && ne->ne_typ == NETYP_FILE) {
            done = 1;
        } else if (ne->ne_typ == end_typ ||
                   ne->ne_typ == alt_end_typ) {
            done = 1;
            free_mess  = 1;
            depth++;
        } else if (end_typ != NETYP_FILE &&
                    (ne->ne_typ == NETYP_EXEC_FUNC || 
                     ne->ne_typ == NETYP_IDLE_FUNC)) {
            done = 1;
        } else {
            ne = ne->ne_prev;
            depth++;
        }

    }
    if (free_mess) {
        free_nest_mess(ce, depth);
    }
}
/***************************************************************/
int chndlr_catch(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix) {
/*
Syntax:

    catch                          catch statement
*/
    int estat;
    struct nest * ne;

    estat = 0;
    ne = ce->ce_nest;
    
    if (!ne || (ne->ne_typ != NETYP_TRY)) {
        clean_nest_mess(ce, NETYP_TRY, NETYP_CATCH);
        estat = set_error(ce, EUNEXPCMD, "catch");
    } else {
        ne->ne_typ = NETYP_CATCH;
        if (ne->ne_prev && ne->ne_prev->ne_idle) ne->ne_idle = NEIDLE_SKIP;
        else if (ne->ne_idle != NEIDLE_ERROR) {
            ne->ne_idle = NEIDLE_SKIP;
        } else {
            while (isspace(cmdstr->str_buf[*cmdix])) (*cmdix)++;
            if (cmdstr->str_buf[*cmdix]) {
                estat = set_error(ce, EEXPEOL, cmdstr->str_buf);
            }
            ne->ne_idle = 0;
            if (ne->ne_prev) ne->ne_flags = ne->ne_prev->ne_flags;
            else             ne->ne_flags = 0; /* should not happen */
        }
    }

    return (estat);
}
/***************************************************************/
int inline_sub(struct cenv * ce,
                struct str * cmdstr,
                int * pix) {

    int estat;

    estat = sub_parm_inline(ce, cmdstr, pix, SUBPARM_NONE);

    return (estat);
}
/***************************************************************/
int chndlr_exit(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix) {
/*
    exit [ <exit status> ]
*/
    int estat;
    int stix;
    int hard_exit;
    int exval;
    struct str parmstr;

    estat = 0;
    hard_exit = 0;
    exval = 0;

    stix = (*cmdix);
    init_str(&parmstr, 0);

    while (isspace(cmdstr->str_buf[*cmdix])) (*cmdix)++;

    while (!estat && (cmdstr->str_buf[*cmdix] == '-' || cmdstr->str_buf[*cmdix] == '$')) {
        if (cmdstr->str_buf[*cmdix] == '$') {
            estat = inline_sub(ce, cmdstr, cmdix);
        } else {
            (*cmdix)++;
            if (CEDWNS(ce, cmdstr->str_buf[*cmdix]) == 'e') {
                if (ce->ce_nest && ce->ce_nest->ne_idle == NEIDLE_ERROR) {
                    hard_exit   = 1;
                    exval       = 1;
                }
                (*cmdix)++;
                while (isspace(cmdstr->str_buf[*cmdix])) (*cmdix)++;
            } else {
                estat = set_ferror(ce, EBADOPT, "%c", cmdstr->str_buf[*cmdix]);
            }
        }
    }

    if (cmdstr->str_buf[*cmdix]) {
        estat = get_parm(ce, cmdstr, cmdix, 0, &parmstr);
        if (estat) return (estat);

        if (stoi(parmstr.str_buf, &exval)) {
            estat = set_error(ce, EEXITPARM, parmstr.str_buf);
        }
    }

    if (!estat) {
        while (isspace(cmdstr->str_buf[*cmdix])) (*cmdix)++;
        if (cmdstr->str_buf[*cmdix]) {
            estat = set_error(ce, EEXPEOL, cmdstr->str_buf);
        }
    }

    if (estat) exval = 1;

    free_str_buf(&parmstr);

    if (hard_exit || !ce->ce_nest || !ce->ce_nest->ne_idle) {    
        ce->ce_close_flags |= FREECE_NEST;
        ce->ce_rtn = exval;

        if (!estat) estat = ESTAT_EXIT;
    }

    return (estat);
}
/***************************************************************/
int chndlr_idle_endfor(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix) {
/*
Syntax:

    done                                        Ends for loop
*/
    int estat;
    struct nest * ne;

    estat = 0;
    ne = ce->ce_nest;
    if (ne && (ne->ne_typ == NETYP_FOR || ne->ne_typ == NETYP_FOR_IDLE)) {    
        free_nest(ce, ne);
    }
    if (!saving_cmds(ce)) {
        delete_cmds(ce);
    }

    return (estat);
}
/***************************************************************/
int chndlr_idle_endwhile(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix) {
/*
Syntax:

    endwhile                                  Ends while loop
*/
    int estat;
    struct nest * ne;

    estat = 0;
    ne = ce->ce_nest;
    
    if (ne && ne->ne_typ == NETYP_WHILE) {    
        free_nest(ce, ne);
    }
    if (!saving_cmds(ce)) {
        delete_cmds(ce);
    }

    return (estat);
}
/***************************************************************/
int chndlr_idle_for(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix) {
/*
*/
    int estat;
    struct nest * ne;

    estat = 0;

    ne = new_nest(ce, NETYP_FOR_IDLE);
    ne->ne_idle = NEIDLE_SKIP;

    return (estat);
}
/***************************************************************/
int chndlr_idle_function(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix) {
/*
*/
    int estat;
    struct nest * ne;

    estat = 0;
    ne = ce->ce_nest;

    if (ne && (ne->ne_flags & NEFLAG_IN_FUNC)) {
        estat = set_error(ce, ENOFUNCHERE, NULL);
    } else {
        ne = new_nest(ce, NETYP_IDLE_FUNC);
        ne->ne_idle = NEIDLE_SKIP;
}

    return (estat);
}
/***************************************************************/
int handle_endfor(struct cenv * ce) {
/*
Syntax:

    done                                        Ends for loop
*/
    int estat;
    struct nest * ne;

    estat = 0;
    ne = ce->ce_nest;

    switch (ne->ne_for_typ) {
        case NEFORTYP_CSVPARSE :
            estat = nendlr_for_csvparse(ce);
            break;
        case NEFORTYP_IN :
            estat = nendlr_for_in(ce);
            break;
        case NEFORTYP_TO :
            estat = nendlr_for_to(ce);
            break;
        case NEFORTYP_DOWNTO :
            estat = nendlr_for_downto(ce);
            break;
        case NEFORTYP_PARSE :
            estat = nendlr_for_parse(ce);
            break;
        case NEFORTYP_READ :
            estat = nendlr_for_read(ce);
            break;
        case NEFORTYP_READDIR :
            estat = nendlr_for_readdir(ce);
            break;
        default:
            break;
    }

    return (estat);
}
/***************************************************************/
int chndlr_endfor(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix) {
/*
Syntax:

    endfor                                        Ends for loop
*/
    int estat;
    struct nest * ne;

    estat = 0;
    ne = ce->ce_nest;
    
    if (!ne || (ne->ne_typ != NETYP_FOR)) {
        estat = set_error(ce, EUNEXPCMD, "endfor");
    } else {
        while (isspace(cmdstr->str_buf[*cmdix])) (*cmdix)++;
        if (cmdstr->str_buf[*cmdix]) {
            estat = set_error(ce, EEXPEOL, cmdstr->str_buf);
        } else {
            estat = handle_endfor(ce);
            /* if (!estat && !ne->ne_idle) set_cmd_pc(ce, ne->ne_label); */
            if (!estat) set_cmd_pc(ce, ne->ne_label);
        }
    }

    return (estat);
}
/***************************************************************/
#ifdef UNUSED
int in_function(struct cenv * ce) {

	int in_func;
	struct nest * ne;

	in_func = 0;
    ne = ce->ce_nest;
    while (ne && ne->ne_typ != NETYP_IDLE_FUNC && ne->ne_prev) {
		ne = ne->ne_prev;
	}
    if (ne && ne->ne_typ == NETYP_IDLE_FUNC) in_func = 1;

    return (in_func);
}
#endif
/***************************************************************/
int chndlr_idle_endfunction(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix) {
/*
Syntax:

    end                                        Ends function declaration
*/
    int estat;
    int ii;
    struct nest * ne;
    struct nest * ne_prev;
    struct usrfuncinfo * ufi;

    estat = 0;
    ne = ce->ce_nest;
    
    if (!ne) {
        estat = set_error(ce, EUNEXPCMD, "endfunction");
    } else {
        if (ne->ne_typ != NETYP_IDLE_FUNC) {
			/*
			** endfunction cleans up missing end's for if's, while's, etc.
			*/
			/* if (in_function(ce)) */
            if (ne->ne_flags & NEFLAG_IN_FUNC) {
				while (ne && ne->ne_typ != NETYP_IDLE_FUNC && ne->ne_prev) {
					ne_prev = ne->ne_prev;
					free_nest(ce, ne);
					ne = ne_prev;
				}
				estat = set_error(ce, EMISSINGEND, NULL);
			} else {
				if (ne->ne_idle != NEIDLE_ERROR) {
                    estat = set_error(ce, ENOTINFUNC, NULL);
                }
			}
		} else {
    	    ufi = ne->ne_ufi;
		    if (ufi) {
                ufi->ufi_stmt_list = New(char *, ce->ce_stmt_len);

                for (ii = 0; ii < ce->ce_stmt_len - 1; ii++) {
                    ufi->ufi_stmt_list[ii] = ce->ce_stmt_list[ii];
                    ce->ce_stmt_list[ii] = NULL;
                }

                /* handle "end" */
                ufi->ufi_stmt_list[ce->ce_stmt_len - 1] = NULL;
                Free(ce->ce_stmt_list[ce->ce_stmt_len - 1]);
                ce->ce_stmt_list[ce->ce_stmt_len - 1] = NULL;
                ufi->ufi_stmt_len = ce->ce_stmt_len - 1;
            }

            free_nest(ce, ne);
            if (ce->ce_nest) {
                ce->ce_stmt_len = ce->ce_nest->ne_saved_stmt_len;
                ce->ce_stmt_pc  = ce->ce_nest->ne_saved_stmt_pc;
            } else {
                ce->ce_stmt_len = 0;
                ce->ce_stmt_pc  = 0;
            }
        }
    }

    return (estat);
}
/***************************************************************/
int chndlr_endfunction(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix) {
/*
Syntax:

    end                                        Ends function declaration
*/
    int estat;

    estat = set_error(ce, EUNEXPCMD, "endfunction");

    return (estat);
}
/***************************************************************/
int chndlr_export(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix) {
/*
Syntax:

    export <var> = <exp>                           Sets value of variable and makes global

    export <var>                                   Makes variable <var> global

*/

    int estat;
    int eol;
    int stix;
    struct str parmstr;
    struct str vnam;

    estat = 0;
    stix = (*cmdix);
    init_str(&parmstr, 0);

    eol = get_token_exp(ce, cmdstr->str_buf, cmdix, &parmstr);
    if (eol) {
        if (eol > 0) estat = eol;
        else estat = set_error(ce, EUNEXPEOL, "export");
    } else if (!is_good_vname(ce, parmstr.str_buf)) {
        estat = set_error(ce, EBADVARNAM, NULL);
    } else {
        copy_vnam(ce, &(vnam.str_buf),  &(vnam.str_len),  &(vnam.str_max), &parmstr);
        while (isspace(cmdstr->str_buf[*cmdix])) (*cmdix)++;
        if (cmdstr->str_buf[*cmdix]) {
            if (cmdstr->str_buf[*cmdix] == '=') {
                (*cmdix) = stix;
                estat = set_var_exp(ce, cmdstr->str_buf, cmdix, &parmstr);
            } else {
                estat = set_ferror(ce, EEXPEQ, "%c", cmdstr->str_buf[*cmdix]);
            }
        }
        if (!estat) {
            estat = export_var(ce, &vnam);
        }
        free_str_buf(&vnam);
    }
    free_str_buf(&parmstr);

    return (estat);
}
/***************************************************************/
int chndlr_for(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix) {
/*
Syntax:

    for <var> in <parmlist>                     Sets <var> and does <cmdlist>
        <cmdlist>                               for each parm
    endfor

    for <var> = <sexp> { to | downto } <eexp>  Sets <var> and does <cmdlist>
        <cmdlist>                               for each value in range of
    endfor                                      <sexp> to <eexp>
 
    for <var> read(<file name>)                 Sets <var> for each line
        <cmdlist>                               in <file name>
    endfor

    ** Unimplimented **
 
    for <var> <for function> ( <parms> )        Sets <var> for each result
        <cmdlist>                               of <for function>
    endfor

        <for function>:
            csvparse(<string>,<flags>)          Each CSV item in <string exp>
            parse(<string exp>)                 Each item in <string exp>
            readdir(<directory name>)           Each file/dir in <directory name>
            tokens(<string exp>, <delims>)      Each token in <string exp>
*/
    int estat;
    int eol;
    struct str parmstr;
    struct nest * ne;
    struct parmlist pl;
    int npc;

    estat = 0;
    init_str(&parmstr, 0);

    eol = get_token_exp(ce, cmdstr->str_buf, cmdix, &parmstr);
    if (eol) {
        if (eol > 0) estat = eol;
        else estat = set_error(ce, EUNEXPEOL, "for");
    } else if (!is_good_vname(ce, parmstr.str_buf)) {
        estat = set_error(ce, EBADVARNAM, NULL);
    } else {
        if (is_saved_cmd(ce)) npc = ce->ce_stmt_pc;
        else                  npc = ce->ce_stmt_len;

        ne = new_nest(ce, 0);
        ne->ne_typ      = NETYP_FOR;
        ne->ne_for_typ  = 0;
        ne->ne_for_done = 0;

        copy_vnam(ce, &(ne->ne_vnam),  &(ne->ne_vnlen),  &(ne->ne_vnmax), &parmstr);

        eol = get_token_exp(ce, cmdstr->str_buf, cmdix, &parmstr);
        if (eol) {
            if (eol > 0) estat = eol;
            else estat = set_error(ce, EUNEXPEOL, "for");
        } else {
            if (!CESTRCMP(ce, parmstr.str_buf, "csvparse")) {
                estat = parse_for_csvparse(ce, ne, cmdstr, cmdix, &parmstr);

            } else if (!CESTRCMP(ce, parmstr.str_buf, "in")) {
                estat = parse_for_in(ce, ne, cmdstr, cmdix, &pl);

            } else if (!CESTRCMP(ce, parmstr.str_buf, "parse")) {
                estat = parse_for_parse(ce, ne, cmdstr, cmdix, &parmstr);

            } else if (!CESTRCMP(ce, parmstr.str_buf, "read")) {
                estat = parse_for_read(ce, ne, cmdstr, cmdix, &parmstr);

            } else if (!CESTRCMP(ce, parmstr.str_buf, "readdir")) {
                estat = parse_for_readdir(ce, ne, cmdstr, cmdix, &parmstr);

            } else if (!strcmp(parmstr.str_buf, "=")) {
                estat = parse_for_range(ce, ne, cmdstr, cmdix, &parmstr);

            } else {
                estat = set_error(ce, EBADFOR, parmstr.str_buf);
            }
        }
        if (estat) {
            free_nest(ce, ne);
        } else if (!ne->ne_idle) {
            ne->ne_loopback = 1;
            ne->ne_label = npc;
        }
    }
    Free(parmstr.str_buf);

    if (!estat) {
        estat = handle_endfor(ce);
    }

    return (estat);
}
/***************************************************************/
void define_dup_function(struct cenv * ce,
						char * fnam,
						int fnamlen,
						struct usrfuncinfo * ufi) {
    int is_dup;
	int dup_num;
	char * tfnam;
	char * nfnam;

	nfnam = New(char, fnamlen + 1);
	memcpy(nfnam, fnam, fnamlen);
	nfnam[fnamlen] = '\0';

	dup_num = 1;
	is_dup = 1;
	while (is_dup) {
		dup_num++;
		tfnam = Smprintf("%s_$DUP%d", nfnam, dup_num);

        is_dup = dbtree_insert(ce->ce_funclib->fl_funcs,
                    tfnam, Istrlen(tfnam), ufi);
		Free(tfnam);
	}
	Free(nfnam);
}
/***************************************************************/
int check_dup_parm_names(struct cenv * ce, struct str ** sparmlist, int nparms) {

    int estat;
    int ii;
    int jj;

    estat = 0;
    for (ii = 1; !estat && ii < nparms; ii++) {
        for (jj = 0; jj < ii; jj++) {
            if (!CESTRCMP(ce, sparmlist[ii]->str_buf, sparmlist[jj]->str_buf)) {
                estat = set_error(ce, EDUPPARMNAME, sparmlist[ii]->str_buf);
            }
        }
    }

    return (estat);
}
/***************************************************************/
int chndlr_function(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix) {
/*
Syntax:

    function <function name> [ ( <parmlist> ) ] Defines user function <funcname>
        <cmdlist>                               
    end

    parmlist ::= <parm> | <parm> , <parmlist>

*/
    int estat;
    int eol;
    struct str parmstr;
    struct nest * ne;
    int nparms;
    int mxparms;
    struct str ** sparmlist;
    int npc;
    int is_dup;
    int ii;

    estat = 0;
    nparms = 0;
    mxparms = 0;
    sparmlist = 0;
    init_str(&parmstr, 0);

/*
    if (is_saved_cmd(ce)) npc = ce->ce_stmt_pc;
    else                  npc = ce->ce_stmt_len;
*/
    if (saving_cmds(ce)) {
        estat = set_error(ce, ENOFUNCSTRUCT, NULL);
        return (estat);
    }

    npc = ce->ce_stmt_len;

    eol = get_token_exp(ce, cmdstr->str_buf, cmdix, &parmstr);
    if (eol) {
        if (eol > 0) estat = eol;
        else estat = set_error(ce, EUNEXPEOL, "function");
    } else if (!is_good_vname(ce, parmstr.str_buf)) {
        estat = set_error(ce, EBADFUNCNAM, NULL);

    } else if (!ce->ce_funclib) {
        estat = set_error(ce, ENOFUNCHERE, NULL);

    } else if (ce->ce_nest && (ce->ce_nest->ne_flags & NEFLAG_IN_FUNC)) {
        estat = set_error(ce, ENOFUNCHERE, NULL);

    } else {
        ne = new_nest(ce, NETYP_IDLE_FUNC);
        copy_fnam(ce, &(ne->ne_vnam),  &(ne->ne_vnlen),  &(ne->ne_vnmax), &parmstr);

        eol = get_token_exp(ce, cmdstr->str_buf, cmdix, &parmstr);
        if (eol > 0) estat = eol;

        if (!eol && parmstr.str_buf[0] == '(' && !parmstr.str_buf[1]) {
            do {
                eol = get_token_exp(ce, cmdstr->str_buf, cmdix, &parmstr);
                if (eol) {
                    if (eol > 0) estat = eol;
                    else estat = set_error(ce, EUNEXPEOL, "function parameters");
                } else if (parmstr.str_buf[0] == ')') {
                    while (isspace(cmdstr->str_buf[(*cmdix)])) (*cmdix)++;
                    if (!cmdstr->str_buf[(*cmdix)]) eol = 1;
                } else if (!is_good_vname(ce, parmstr.str_buf)) {
                    estat = set_error(ce, EBADVARNAM, NULL);
                } else {
                    if (nparms == mxparms) {
                        if (!mxparms) mxparms = PARMLIST_CHUNK;
                        else          mxparms *= 2;
                        sparmlist = Realloc(sparmlist, struct str*, mxparms);
                    }
                    sparmlist[nparms] = New(struct str, 1);
                    copy_vnam(ce,
                        &(sparmlist[nparms]->str_buf),  &(sparmlist[nparms]->str_len),  &(sparmlist[nparms]->str_max),
                        &parmstr);
                    nparms++;
                    while (isspace(cmdstr->str_buf[(*cmdix)])) (*cmdix)++;
                    if (cmdstr->str_buf[(*cmdix)] == ',') {
                        (*cmdix)++;
                        while (isspace(cmdstr->str_buf[(*cmdix)])) (*cmdix)++;
                    } else if (cmdstr->str_buf[(*cmdix)] != ')') {
                        estat = set_ferror(ce, EEXPFORMCOMMA, "%c", cmdstr->str_buf[(*cmdix)]);
                    }
                }
            } while (!estat && !eol && cmdstr->str_buf[(*cmdix)] != ')');
            if (!estat && !eol) {
                (*cmdix)++; /* skip ) */
                eol = get_token_exp(ce, cmdstr->str_buf, cmdix, &parmstr);
                if (eol > 0) estat = eol;
            }
        }

        if (!estat && !eol) {
            estat = set_error(ce, EEXPEOL, parmstr.str_buf);
        }

        if (!estat) {
            estat = check_dup_parm_names(ce, sparmlist, nparms);
        }

        if (!estat) {
            ne->ne_ufi = New(struct usrfuncinfo, 1);
            ne->ne_ufi->ufi_nparms    = nparms;
            ne->ne_ufi->ufi_sparmlist = sparmlist;
            ne->ne_ufi->ufi_flags     = 0;
            ne->ne_ufi->ufi_stmt_list = NULL;
            ne->ne_ufi->ufi_stmt_len  = 0;
            ne->ne_ufi->ufi_func_name = New(char, ne->ne_vnlen + 1);
            memcpy(ne->ne_ufi->ufi_func_name, ne->ne_vnam, ne->ne_vnlen + 1);

            if (ce->ce_flags & CEFLAG_OPT_PARMS) ne->ne_ufi->ufi_flags |= UFIFLAG_OPT_PARMS;

            is_dup = dbtree_insert(ce->ce_funclib->fl_funcs,
                        ne->ne_vnam, ne->ne_vnlen, ne->ne_ufi);
            if (is_dup) {
                define_dup_function(ce, ne->ne_vnam, ne->ne_vnlen, ne->ne_ufi);
                estat = set_error(ce, ENODUPFUNC, ne->ne_vnam);
            }
			ne->ne_loopback = 1;
			ne->ne_label	= npc;
			ne->ne_idle		= NEIDLE_SKIP;
			ne->ne_flags	= NEFLAG_IN_FUNC;
		} else {
			ne->ne_idle		= NEIDLE_ERROR;
			ne->ne_flags	= NEFLAG_IN_FUNC;
            if (nparms) {
                for (ii = 0; ii < nparms; ii++) {
                    free_str_buf(sparmlist[ii]);
                    Free(sparmlist[ii]);
                }
            }   Free(sparmlist);
        }
    }
    Free(parmstr.str_buf);

    return (estat);
}
/***************************************************************/
int chk_condition(struct cenv * ce,
                const char * parms,
                int * pix,
                struct var * result) {

                            
/*
*/

    int estat;
    int eol;
    struct str parmstr;

    estat = 0;
    init_str(&parmstr, 0);

    eol = get_token_exp(ce, parms, pix, &parmstr);
    if (eol) {
        if (eol > 0) estat = eol;
        else estat = set_error(ce, EUNEXPEXPEOL, NULL);
        free_str_buf(&parmstr);
    } else {
        estat = parse_exp(ce, parms, pix, &parmstr, result);

        if (!estat) {
            eol = vensure_bool(ce, result);
            if (eol) {
                vensure_str(ce, result);
                estat = set_error(ce, EEXPBOOL, result->var_buf);
            } else {
                while (isspace(parms[*pix])) (*pix)++;
                if (parms[*pix]) {
                    estat = set_error(ce, EEXPEOL, parms);
                }
            }
        }
    }

    free_str_buf(&parmstr);

    return (estat);
}
/***************************************************************/
int chndlr_if(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix) {
/*
Syntax:

    if <exp>                           if statement
        [ <stmtlist> ]
    [ else if <exp> ]
        [ <stmtlist> ]
    [ else ]
        [ <stmtlist> ]
    endif
*/

    int estat;
    struct nest * ne;
    struct var result;
    int pix;

    pix = 0;
    estat = chk_condition(ce, cmdstr->str_buf, cmdix, &result);
    if (!estat) {
        ne = new_nest(ce, NETYP_IF);
        if (result.var_val) ne->ne_idle = 0;
        else                ne->ne_idle = NEIDLE_SKIP;
    } else {
        ne = new_nest(ce, NETYP_IF);
        ne->ne_idle = 0; /* assume bad if is true */
        free_var_buf(&result);
    }

    return (estat);
}
/***************************************************************/
int chndlr_idle_if(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix) {
/*
Syntax:

    end                                        Ends function declaration
*/
    int estat;
    struct nest * ne;

    estat = 0;

    ne = new_nest(ce, NETYP_IF);
    ne->ne_idle = NEIDLE_SKIP;

    return (estat);
}
/***************************************************************/
int chndlr_else(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix) {
/*
Syntax:

    else [ if <exp> ]                       else statement
*/
    int estat;
    struct nest * ne;
    struct var result;

    estat = 0;
    ne = ce->ce_nest;
    
    if (!ne || (ne->ne_typ != NETYP_IF && ne->ne_typ != NETYP_ELSE)) {
		if (ne->ne_idle != NEIDLE_ERROR) {
            clean_nest_mess(ce, NETYP_IF, NETYP_ELSE);
            estat = set_error(ce, EUNEXPCMD, "else");
        }
    } else {
        if (ne->ne_prev && ne->ne_prev->ne_idle)    ne->ne_idle = NEIDLE_SKIP;
        else if (!ne->ne_idle) {
            ne->ne_typ = NETYP_ELSE;
            ne->ne_idle = NEIDLE_SKIP;
        } else if (ne->ne_typ == NETYP_IF && ne->ne_idle == NEIDLE_SKIP) {
            while (isspace(cmdstr->str_buf[*cmdix])) (*cmdix)++;
            if (CEDWNS(ce, cmdstr->str_buf[*cmdix])    == 'i' &&
                CEDWNS(ce, cmdstr->str_buf[(*cmdix)+ 1]) == 'f' &&
                isspace(cmdstr->str_buf[(*cmdix) + 2]) ) {

                (*cmdix) += 3;
                estat = chk_condition(ce, cmdstr->str_buf, &(*cmdix), &result);
                if (!estat) {
                    if (result.var_val) ne->ne_idle = 0;
                    else                ne->ne_idle = NEIDLE_SKIP;
                } else {
                    ne->ne_idle = 0; /* assume bad else if is true */
                    free_var_buf(&result);
                }
            } else {
                if (cmdstr->str_buf[*cmdix]) {
                    estat = set_error(ce, EEXPEOL, cmdstr->str_buf);
                }
                ne->ne_idle = 0;
            }
        }
    }

    return (estat);
}
/***************************************************************/
int chndlr_endif(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix) {
/*
Syntax:

    endif                                        Ends if declaration
*/
    int estat;
    struct nest * ne;

    estat = 0;
    ne = ce->ce_nest;
    
    if (!ne || (ne->ne_typ != NETYP_IF && ne->ne_typ != NETYP_ELSE)) {
		if (ne->ne_idle != NEIDLE_ERROR) {
            clean_nest_mess(ce, NETYP_IF, NETYP_ELSE);
            estat = set_error(ce, EUNEXPCMD, "endif");
        }
    } else {
        while (isspace(cmdstr->str_buf[*cmdix])) (*cmdix)++;
        if (cmdstr->str_buf[*cmdix]) {
            estat = set_error(ce, EEXPEOL, cmdstr->str_buf);
        }
        free_nest(ce, ne);
    }

    return (estat);
}

/***************************************************************/
int chndlr_endtry(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix) {
/*
Syntax:

    endtry                                        Ends try declaration
*/
    int estat;
    struct nest * ne;

    estat = 0;
    ne = ce->ce_nest;
    
    if (!ne || (ne->ne_typ != NETYP_TRY && ne->ne_typ != NETYP_CATCH)) {
        clean_nest_mess(ce, NETYP_TRY, NETYP_CATCH);
        estat = set_error(ce, EUNEXPCMD, "endtry");
    } else {
        while (isspace(cmdstr->str_buf[*cmdix])) (*cmdix)++;
        if (cmdstr->str_buf[*cmdix]) {
            estat = set_error(ce, EEXPEOL, cmdstr->str_buf);
        }

        if (ne->ne_typ == NETYP_TRY) {
            /*
            ** free_nest() sets ce->ce_nest->ne_idle = NEIDLE_ERROR if there was an
            ** error. If there was no catch, the error needs to cleared.
            */
            ne->ne_idle = 0;
        }
        free_nest(ce, ne);
    }

    return (estat);
}
/***************************************************************/
char * get_verbose_depth_str(struct cenv * ce, int subdepth) {

    static char spaces[128];
    int nsp;
    struct nest * ne;

    nsp = -(subdepth);
    ne = ce->ce_nest;
    while (ne) {
        nsp += 2;
        ne = ne->ne_prev;
    }
    nsp -= 2 * subdepth;
    if (nsp >= sizeof(spaces)) nsp = sizeof(spaces) - 1;

    memset(spaces, ' ', nsp);
    spaces[nsp] = '\0';

    return (spaces);
}    
/***************************************************************/
int chndlr_endwhile(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix) {
/*
Syntax:

    endwhile                                  Ends while loop
*/
    int estat;
    struct nest * ne;
    char * condparms;
    struct var result;
    int pix;

    estat = 0;
    ne = ce->ce_nest;
    
    if (!ne || ne->ne_typ != NETYP_WHILE) {
        clean_nest_mess(ce, NETYP_WHILE, 0);
        estat = set_error(ce, EUNEXPCMD, "endwhile");
        return (estat);
    } else {
        pix = 0;
        while (isspace(cmdstr->str_buf[*cmdix])) (pix)++;
        if (cmdstr->str_buf[*cmdix]) {
            estat = set_error(ce, EEXPEOL, cmdstr->str_buf);
       } else {
            condparms = (char*)(ne->ne_data);
            pix = 0;
            if (ce->ce_flags & CEFLAG_VERBOSE) {
                printlle(ce, "%swhile %s\n", get_verbose_depth_str(ce,1), condparms);
            }
            estat = chk_condition(ce, condparms, &pix, &result);
        }
        if (estat) {
            ne->ne_idle = NEIDLE_SKIP;
        } else {
            if (result.var_val) ne->ne_idle = 0;
            else                ne->ne_idle = NEIDLE_SKIP;
            if (!ne->ne_idle) {
                set_cmd_pc(ce, ne->ne_label);
            }
        }
    }
    if (ne->ne_idle) {
        Free(ne->ne_data);
        free_nest(ce, ne);
        if (!saving_cmds(ce)) {
            delete_cmds(ce);
        }
    }

    return (estat);
}
/***************************************************************/
int chndlr_return(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix) {
/*
Syntax:

    return <exp>                           Sets return value of function

*/

    int estat;
    int pix;
    int eol;
    struct str parmstr;
    struct var result;

    if (!ce->ce_nest || !(ce->ce_nest->ne_flags & NEFLAG_IN_FUNC)) {
        estat = set_error(ce, ENORETURNHERE, NULL);
        return (estat);
    }

    estat = 0;
    pix = 0;
    init_str(&parmstr, 0);

    eol = get_token_exp(ce, cmdstr->str_buf, cmdix, &parmstr);
    if (eol > 0) {
        estat = eol;
    } else if (eol) {
        ce->ce_rtn_var = New(struct var, 1);
        ce->ce_rtn_var->var_typ = VTYP_NONE;
        ce->ce_stmt_pc = ce->ce_stmt_len;
    } else {
        estat = parse_exp(ce, cmdstr->str_buf, cmdix, &parmstr, &result);

        if (!estat) {
            if (parmstr.str_len) {
                estat = set_error(ce, EEXPEOL, parmstr.str_buf);
            } else {
                while (ce->ce_nest && ce->ce_nest->ne_typ != NETYP_EXEC_FUNC) {
                    free_nest(ce, ce->ce_nest);
                }
                ce->ce_rtn_var = New(struct var, 1);
                qcopy_var(ce->ce_rtn_var, &result);
                ce->ce_stmt_pc = ce->ce_stmt_len;
            }
        }
    }

    free_str_buf(&parmstr);

    return (estat);
}
/***************************************************************/
int chndlr_set(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix) {
/*
Syntax:

    set                                         Shows all variables

    set <var> = <exp>                           Sets value of variable

*/

    int estat;
    int stix;
    struct str parmstr;


    estat = 0;
    stix = (*cmdix);
    init_str(&parmstr, 0);

    while (isspace(cmdstr->str_buf[*cmdix])) (*cmdix)++;

    if (!isalnum(cmdstr->str_buf[*cmdix]) && cmdstr->str_buf[*cmdix] != '_') {
        estat = cmd_set_show(ce, cmdstr, cmdix);
    } else {
        (*cmdix) = stix;
        estat = set_var_exp(ce, cmdstr->str_buf, cmdix, &parmstr);
    }
    free_str_buf(&parmstr);

    return (estat);
}
/***************************************************************/
int get_opt_flag(char oparm) {

    int oflag;

    oflag = 0;

           if (oparm == 'a') {
        oflag = CEFLAG_VERSNO;
    } else if (oparm == 'd') {
        oflag = CEFLAG_DEBUG;
    } else if (oparm == 'f') {
        oflag = CEFLAG_SUBFSET;
    } else if (oparm == 'i') {
        oflag = CEFLAG_CASE;
    } else if (oparm == 'n') {
        oflag = CEFLAG_NORC;
    } else if (oparm == 'p') {
        oflag = CEFLAG_PAUSE;
    } else if (oparm == 'r') {
        oflag = CEFLAG_OPT_PARMS;
    } else if (oparm == 'v') {
        oflag = CEFLAG_VERBOSE;
    } else if (oparm == 'w') {
        oflag = CEFLAG_WIN;
    } else if (oparm == 'x') {
        oflag = CEFLAG_EXIT;
    }

    return (oflag);
}
/***************************************************************/
int show_flags(struct cenv * ce, int flag_mask) {

    int estat;
    int oflag;
    char oparm;

    estat = 0;

    oflag = 1;
    while (oflag < CEFLAG_ALL_FLAGS) {
        if (oflag & flag_mask) {
                   if (oflag == CEFLAG_VERSNO) {
                oparm = 'a';
            } else if (oflag == CEFLAG_CMD) {
                oparm = 'c';
            } else if (oflag == CEFLAG_DEBUG) {
                oparm = 'd';
            } else if (oflag == CEFLAG_SUBFSET) {
                oparm = 'f';
            } else if (oflag == CEFLAG_CASE) {
                oparm = 'i';
            } else if (oflag == CEFLAG_NORC) {
                oparm = 'n';
            } else if (oflag == CEFLAG_PAUSE) {
                oparm = 'p';
            } else if (oflag == CEFLAG_VERBOSE) {
                oparm = 'v';
            } else if (oflag == CEFLAG_WIN) {
                oparm = 'w';
            } else if (oflag == CEFLAG_EXIT) {
                oparm = 'x';
            } else {
                oparm = '\0';
            }

            if (oparm) {
                if (ce->ce_flags & oflag) {
                    printll(ce, "shopt -s %c\n", oparm);
                } else {
                    printll(ce, "shopt -u %c\n", oparm);
                }
            }
        }
        oflag <<= 1;
    }

    return (estat);
}
/***************************************************************/
int chndlr_shift(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix) {
/*
    shift [ <n parms> ]
*/
    int estat;
    int nparms;
    int ii;
    struct str parmstr;


    estat  = 0;
    nparms = 1;
    init_str(&parmstr, 0);

    while (isspace(cmdstr->str_buf[*cmdix])) (*cmdix)++;

    if (cmdstr->str_buf[*cmdix]) {
        estat = get_parm(ce, cmdstr, cmdix, 0, &parmstr);
        if (estat) return (estat);

        if (stoi(parmstr.str_buf, &nparms)) {
            estat = set_error(ce, ESHIFTPARM, parmstr.str_buf);
        } else if (nparms >= ce->ce_argc) {
            nparms = ce->ce_argc - 1;
        }
    }

    if (!estat) {
        while (isspace(cmdstr->str_buf[*cmdix])) (*cmdix)++;
        if (cmdstr->str_buf[*cmdix]) {
            estat = set_error(ce, EEXPEOL, cmdstr->str_buf);
        }
    }

    free_str_buf(&parmstr);

    if (!estat) {
        if (nparms > 0 && nparms < ce->ce_argc) {
            /* remember - cannot touch ce->ce_argv[0] */
            for (ii = 1; ii <= nparms; ii++) {
                Free(ce->ce_argv[ii]);
            }
            for (ii = nparms + 1; ii < ce->ce_argc; ii++) {
                ce->ce_argv[ii - nparms] = ce->ce_argv[ii];
            }
            for (ii = ce->ce_argc - nparms; ii < ce->ce_argc; ii++) {
                ce->ce_argv[ii] = NULL;
            }
            if (ce->ce_argc >= 8 && ce->ce_argc - nparms < 8) { /* 8 is arbitrary */
                ce->ce_argv = Realloc(ce->ce_argv, char*, ce->ce_argc - nparms);
            }
            ce->ce_argc -= nparms;
        }
    }

    return (estat);
}
/***************************************************************/
int chndlr_shopt(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix) {
/*
Syntax:

    shopt [ -p | -s | -u <flag> [ ... ]                 Sets/resets run flags

*/

    int estat;
    int do_set;
    int do_unset;
    int do_print;
    int flag_mask;
    int oflag;
    char oparm;
    int estat2;
    int parmix;
    struct parmlist pl;

    estat       = 0;
    parmix      = 0;
    do_set      = 0;
    do_unset    = 0;
    do_print    = 0;
    flag_mask   = 0;

    estat = parse_parmlist(ce, cmdstr, cmdix, &pl);
    if (estat) return (estat);

    estat = handle_redirs(ce, &pl);
    if (estat) return (estat);

    while (!estat && parmix < pl.pl_ce->ce_argc &&
            pl.pl_ce->ce_argv[parmix][0] == '-') {
        if (!CESTRCMP(ce, pl.pl_ce->ce_argv[parmix], "-p")) {
            do_print = 1;
        } else if (!CESTRCMP(ce, pl.pl_ce->ce_argv[parmix], "-s")) {
            if (do_unset) {
                estat = set_error(ce, EOPTCONFLICT, "-u");
            } else {
                do_set = 1;
            }
        } else if (!CESTRCMP(ce, pl.pl_ce->ce_argv[parmix], "-u")) {
            if (do_set) {
                estat = set_error(ce, EOPTCONFLICT, "-s");
            } else {
                do_unset = 1;
            }
        } else {
            estat = set_ferror(ce, EBADOPT, "%s", pl.pl_ce->ce_argv[parmix]);
        }
        parmix++;
    }

    while (!estat && parmix < pl.pl_ce->ce_argc) {
        if (pl.pl_ce->ce_argv[parmix][0] && !pl.pl_ce->ce_argv[parmix][1]) {
            oparm = CEDWNS(ce, pl.pl_ce->ce_argv[parmix][0]);
            oflag = get_opt_flag(oparm);
            if (!oflag) {
                estat = set_ferror(ce, EINVALFLAG, "%c", oparm);
            } else {
                flag_mask |= oflag;
                (*cmdix)++;
            }
            parmix++;
        } else {
            estat = set_ferror(ce, EINVALFLAG, "%s", pl.pl_ce->ce_argv[parmix]);
        }
    }

    if (!estat) {
        if (!flag_mask) {
            if (do_set || do_unset) {
                estat = set_error(ce, EINVALFLAG, "flag required");
            } else {
                estat = show_flags(pl.pl_ce, CEFLAG_ALL_FLAGS);
            }
        } else if (do_set) {
            ce->ce_flags |= flag_mask;
            if (do_print) {
                estat = show_flags(pl.pl_ce, flag_mask);
            }
        } else if (do_unset) {
            ce->ce_flags &= ~flag_mask;
            if (do_print) {
                estat = show_flags(pl.pl_ce, flag_mask);
            }
        } else if (do_print) {
            estat = show_flags(pl.pl_ce, flag_mask);
        } else {
            estat = set_error(ce, EBADOPT, "-s for set or -u for unset required");
        }
    }

    estat2 = free_parmlist_buf(ce, &pl);
    if (!estat) estat = estat2;
    else if (estat2) estat = chain_error(ce, estat, estat2);

    return (estat);
}
/***************************************************************/
int chndlr_show(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix) {
/*
Syntax: (debug only)

    show [ <show parm> ] [ ...]                     Displays variables and others


    <show parm> ::= filevars | globalvars | localvars
*/
    int estat;
    int estat2;
    int nparms;
    struct parmlist pl;

    estat = 0;
    nparms = 0;

    estat = parse_parmlist(ce, cmdstr, cmdix, &pl);
    if (estat) return (estat);

    estat = handle_redirs(ce, &pl);
    if (estat) return (estat);

    while (!estat && nparms < pl.pl_ce->ce_argc) {
        if (!CESTRCMP(ce, pl.pl_ce->ce_argv[nparms], "localvars")) {
            if (ce->ce_vars) estat = show_vars(pl.pl_ce, "localvar ", ce);
            else printll(pl.pl_ce, "%s\n", "no local vars");

        } else if (!CESTRCMP(ce, pl.pl_ce->ce_argv[nparms], "filevars")) {
            if (ce->ce_file_ce && ce->ce_file_ce->ce_vars) 
                estat = show_vars(pl.pl_ce, "filevar  ", ce->ce_file_ce);
            else printll(pl.pl_ce, "%s\n", "no file vars");

        } else if (!CESTRCMP(ce, pl.pl_ce->ce_argv[nparms], "globalvars")) {
            estat = show_global_vars(pl.pl_ce);
        } else {
            estat = set_error(ce, EBADPARM, pl.pl_ce->ce_argv[nparms]); 
        }
        nparms++;
    }

    estat2 = free_parmlist_buf(ce, &pl);
    if (!estat) estat = estat2;
    else if (estat2) estat = chain_error(ce, estat, estat2);

    return (estat);
}
/***************************************************************/
int chndlr_start(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix) {
/*
    start <cmd> [ <parm> ] [ ...]             Runs <cmd> with no waiting
*/
    int estat;
    int xeqflags;

    estat = 0;
    xeqflags = XEQFLAG_NOWAIT;

    while (isspace(cmdstr->str_buf[*cmdix])) (*cmdix)++;

    while (!estat && (cmdstr->str_buf[*cmdix] == '-' || cmdstr->str_buf[*cmdix] == '$')) {
        if (cmdstr->str_buf[*cmdix] == '$') {
            estat = inline_sub(ce, cmdstr, cmdix);
        } else {
            (*cmdix)++;
            if (tolower(cmdstr->str_buf[*cmdix]) == 'v') {
                xeqflags |= XEQFLAG_VERBOSE;
                (*cmdix)++;
                while (isspace(cmdstr->str_buf[*cmdix])) (*cmdix)++;
            } else {
                estat = set_ferror(ce, EBADOPT, "%c", cmdstr->str_buf[*cmdix]);
            }
        }
    }

    if (!estat) {
        estat = execute_command_handler(ce, cmdstr, cmdix, xeqflags);
    }

    return (estat);
}
/***************************************************************/
int chndlr_throw(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix) {
/*
Syntax:

    throw <exp>
*/

    int estat;
    struct var result;
    int pix;
    int eol;
    struct str parmstr;

    estat = 0;
    pix = (*cmdix);
    init_str(&parmstr, 0);

    eol = get_token_exp(ce, cmdstr->str_buf, cmdix, &parmstr);
    if (eol) {
        if (eol > 0) estat = eol;
        else         estat = set_error(ce, ETHROW, NULL);
    } else {
        estat = parse_exp(ce, cmdstr->str_buf, cmdix, &parmstr, &result);

        if (!estat) {
            vensure_str(ce, &result);
            while (isspace(cmdstr->str_buf[*cmdix])) (*cmdix)++;
            if (cmdstr->str_buf[*cmdix]) {
                estat = set_error(ce, EEXPEOL, cmdstr->str_buf);
            } else {
                estat = set_error(ce, ETHROW, result.var_buf);
            }
            free_var_buf(&result);
        }
    }

    free_str_buf(&parmstr);

    return (estat);
}
/***************************************************************/
int chndlr_time(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix) {
/*
    time [-p] <cmd> [ <parm> ] [ ...]             Shows time to run  <cmd>
*/
    int estat;
    int posix_fmt;
    int hi_p;
    struct proctimes pt_start;
    struct proctimes pt_end;
    struct proctimes pt;

    estat = 0;
    posix_fmt = 0;
    hi_p = 0;

    while (isspace(cmdstr->str_buf[*cmdix])) (*cmdix)++;

    while (!estat && (cmdstr->str_buf[*cmdix] == '-' || cmdstr->str_buf[*cmdix] == '$')) {
        if (cmdstr->str_buf[*cmdix] == '$') {
            estat = inline_sub(ce, cmdstr, cmdix);
        } else {
            (*cmdix)++;
            if (tolower(cmdstr->str_buf[*cmdix]) == 'h') {
                hi_p = 1;
                (*cmdix)++;
                while (isspace(cmdstr->str_buf[*cmdix])) (*cmdix)++;
            } else if (tolower(cmdstr->str_buf[*cmdix]) == 'p') {
                posix_fmt = 1;
                (*cmdix)++;
                while (isspace(cmdstr->str_buf[*cmdix])) (*cmdix)++;
            } else {
                estat = set_ferror(ce, EBADOPT, "%c", cmdstr->str_buf[*cmdix]);
            }
        }
    }
/*
Ruby:~ Dave$ time
real	0m0.000s
user	0m0.000s
sys	0m0.000s
Ruby:~ Dave$ time -p
real 0.00
user 0.00
sys 0.00
*/
    if (!estat) {
        start_cpu_timer(&pt_start, hi_p);
        estat = execute_command_handler(ce, cmdstr, cmdix, 0);
        stop_cpu_timer(&pt_start, &pt_end, &pt);
 
        if (!estat) {
            if (!posix_fmt) {
                printlle(ce, "real %dm%d.%03ds\n",
                    pt.pt_wall_time.ts_daynum * 1440 + pt.pt_wall_time.ts_second / 60,
                    pt.pt_wall_time.ts_second % 60,
                    pt.pt_wall_time.ts_microsec / 1000);

                printlle(ce, "user %dm%d.%03ds\n",
                    pt.pt_cpu_time.ts_daynum * 1440 + pt.pt_cpu_time.ts_second / 60,
                    pt.pt_cpu_time.ts_second % 60,
                    pt.pt_cpu_time.ts_microsec / 1000);

                printlle(ce, "sys  %dm%d.%03ds\n",
                    pt.pt_sys_time.ts_daynum * 1440 + pt.pt_sys_time.ts_second / 60,
                    pt.pt_sys_time.ts_second % 60,
                    pt.pt_sys_time.ts_microsec / 1000);

                if (pt.pt_use_hi_p) {
                    double elap_int;
                    double elap_frac;

                    elap_frac = modf(pt.pt_hi_elap_secs, &elap_int);
                    printlle(ce, "hi   %dm%d.%06ds\n",
                        ((int)elap_int / 60),
                        ((int)elap_int % 60),
                        (int)floor(elap_frac * (double)1000000));
                }
            } else {
                printlle(ce, "real %d.%02d\n",
                    pt.pt_wall_time.ts_daynum * 1440 + pt.pt_wall_time.ts_second,
                    pt.pt_wall_time.ts_microsec / 10000);

                printlle(ce, "user %d.%02d\n",
                    pt.pt_cpu_time.ts_daynum * 1440 + pt.pt_cpu_time.ts_second,
                    pt.pt_cpu_time.ts_microsec / 10000);

                printlle(ce, "sys %d.%02d\n",
                    pt.pt_sys_time.ts_daynum * 1440 + pt.pt_sys_time.ts_second,
                    pt.pt_sys_time.ts_microsec / 10000);

                if (pt.pt_use_hi_p) {
                    double elap_int;
                    double elap_frac;

                    elap_frac = modf(pt.pt_hi_elap_secs, &elap_int);
                    printlle(ce, "hi   %dm%d.%06ds\n",
                        ((int)elap_int / 60),
                        ((int)elap_int % 60),
                        (int)floor(elap_frac * (double)1000000));
                }
            }
        }
    }

    return (estat);
}
/***************************************************************/
int chndlr_try(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix) {
/*
    try [-b] [-q] <cmd> [ <parm> ] [ ...]

        -or-

    try [-b] [-q]
        <cmds>
    [ recover ]
        [ <cmds> ]
    endtry
*/
    int estat;
    int nomsg;
    int recover_break;
    struct nest * ne;

    estat = 0;
    nomsg = 0;
    recover_break = 0;

    while (isspace(cmdstr->str_buf[*cmdix])) (*cmdix)++;

    while (!estat && (cmdstr->str_buf[*cmdix] == '-' || cmdstr->str_buf[*cmdix] == '$')) {
        if (cmdstr->str_buf[*cmdix] == '$') {
            estat = inline_sub(ce, cmdstr, cmdix);
        } else {
            (*cmdix)++;
            if (CEDWNS(ce, cmdstr->str_buf[*cmdix]) == 'b') {
                recover_break = 1;
                (*cmdix)++;
                while (isspace(cmdstr->str_buf[*cmdix])) (*cmdix)++;
            } else if (CEDWNS(ce, cmdstr->str_buf[*cmdix]) == 'q') {
                nomsg = 1;
                (*cmdix)++;
                while (isspace(cmdstr->str_buf[*cmdix])) (*cmdix)++;
            } else {
                estat = set_ferror(ce, EBADOPT, "%c", cmdstr->str_buf[*cmdix]);
            }
        }
    }

    if (!estat) {
        if (cmdstr->str_buf[*cmdix]) {
            estat = execute_command_handler(ce, cmdstr, cmdix, 0);
            if (estat) {
                if (nomsg) {
                    clear_error(ce, estat);
                } else {
                    show_errmsg(ce, cmdstr->str_buf, estat);
                }
                estat = 0;
            }
        } else {
            ne = new_nest(ce, NETYP_TRY);
            ne->ne_idle = 0;
            if (nomsg) ne->ne_flags |= NEFLAG_NOERRMSG;
            if (recover_break) ne->ne_flags |= NEFLAG_BREAK;
        }
    }

    return (estat);
}
/***************************************************************/
int chndlr_idle_try(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix) {
/*
    See: chndlr_try()
*/
    int estat;
    int nomsg;
    struct nest * ne;

    estat = 0;
    nomsg = 0;

    while (isspace(cmdstr->str_buf[*cmdix])) (*cmdix)++;

    while (!estat && (cmdstr->str_buf[*cmdix] == '-' || cmdstr->str_buf[*cmdix] == '$')) {
        if (cmdstr->str_buf[*cmdix] == '$') {
            estat = inline_sub(ce, cmdstr, cmdix);
        } else {
            (*cmdix)++;
            if (CEDWNS(ce, cmdstr->str_buf[*cmdix]) == 'q') {
                nomsg = 1;
            }
            (*cmdix)++;
        }
        while (isspace(cmdstr->str_buf[*cmdix])) (*cmdix)++;
    }

    if (!estat) {
        if (!cmdstr->str_buf[*cmdix]) {
            ne = new_nest(ce, NETYP_TRY);
            ne->ne_idle = NEIDLE_SKIP;
        }
    }

    return (estat);
}
/***************************************************************/
int chndlr_wait(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix) {
/*
Syntax: (debug only)

    wait [ -v ] pid [ pid [ ... ] ]

        -v      Verbose
*/
    int estat;
    int estat2;
    int pix;
    int ix;
    int cmdflags;
    char * parm;
    int l_npids;
    int l_mxpid;
    int pidparm;
    int rxstat;
    int exit_code;
    int rxwaitflags;
    PID_T * l_pidlist;
    struct parmlist pl;

    estat       = 0;
    cmdflags    = 0;
    pix         = 0;
    rxwaitflags = 0;
    exit_code   = 0;
    l_npids     = 0;
    l_mxpid     = 0;
    l_pidlist   = NULL;

    estat = parse_parmlist(ce, cmdstr, cmdix, &pl);
    if (estat) return (estat);

    estat = handle_redirs(ce, &pl);
    if (estat) return (estat);

    while (!estat && pix < pl.pl_ce->ce_argc) {
        parm = pl.pl_ce->ce_argv[pix];
        if (parm[0] == '-' && parm[1]) {
            ix = 0;
            while (!estat && parm[ix]) {
                switch (parm[ix]) {
                    case '-':
                        break;
                    case 'a': case 'A':
                        cmdflags |= CMDFLAG_SHOW_ALL;
                        break;
                    case 'i': case 'I':
                        rxwaitflags |= RXWAIT_IGNERR;
                        break;
                    case 'v': case 'V':
                        rxwaitflags |= RXWAIT_VERBOSE;
                        break;
                    default:
                        estat = set_ferror(ce, EBADOPT, "%c",
                                        parm[ix]);
                        break;
                }
                ix++;
            }
        } else {
            if (stoi(parm, &pidparm)) {
                estat = set_error(ce, EBADPID, parm);
            } else {
	            if (l_npids == l_mxpid) {
		            if (!l_mxpid) l_mxpid = 4;
		            else l_mxpid *= 2;

		            l_pidlist  = Realloc(l_pidlist, PID_T, l_mxpid);
	            }
	            l_pidlist[l_npids] = pidparm;
	            l_npids++;
            }
        }
        pix++;            
    }

    if (!estat) {
        if (cmdflags & CMDFLAG_SHOW_ALL) {
            if (l_npids) {
                estat = set_error(ce, EEXPEOL, "pid");
            } else {
                rx_get_pidlist(&l_pidlist, &l_npids);
            }
        } else {
            if (!l_npids) {
                estat = set_error(ce, EUNEXPEOL, parm);
            }
        }
    }

    if (!estat && l_npids) {
        rxstat = rx_wait_pidlist(l_pidlist, l_npids, &exit_code, rxwaitflags);
        if (rxstat) {
            estat = set_ferror(ce, EWAITERR, "%d", rxstat);
        } else {
            ce->ce_rtn = exit_code;
        }
    }

    Free(l_pidlist);

    estat2 = free_parmlist_buf(ce, &pl);
    if (!estat) estat = estat2;
    else if (estat2) estat = chain_error(ce, estat, estat2);

    if (!estat && exit_code && !(rxwaitflags & RXWAIT_IGNERR)) {
        estat = set_error(ce, EWAITCMDFAIL, NULL);
    }

    return (estat);
}
/***************************************************************/
int chndlr_while(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix) {
/*
Syntax:

    while <exp>                           while statement
        [ <stmtlist> ]
    endwhile
*/

    int estat;
    struct nest * ne;
    struct var result;
    int stix;
    int npc;

    stix = (*cmdix);
    estat = chk_condition(ce, cmdstr->str_buf, cmdix, &result);
    if (!estat) {
        if (is_saved_cmd(ce)) npc = ce->ce_stmt_pc;
        else                  npc = ce->ce_stmt_len;

        ne = new_nest(ce, NETYP_WHILE);
        if (result.var_val) ne->ne_idle = 0;
        else                ne->ne_idle = NEIDLE_SKIP;
        if (!ne->ne_idle) {
            ne->ne_loopback = 1;
            ne->ne_label = npc;
            ne->ne_data = (char*)Strdup(cmdstr->str_buf + stix);
       }
    } else {
        ne = new_nest(ce, NETYP_WHILE);
        ne->ne_idle = NEIDLE_SKIP; /* assume bad while is false */
        free_var_buf(&result);
    }

    return (estat);
}
/***************************************************************/
int chndlr_idle_while(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix) {
/*
Syntax:

    end                                        Ends function declaration
*/
    int estat;
    struct nest * ne;

    estat = 0;

    ne = new_nest(ce, NETYP_WHILE);
    ne->ne_idle = NEIDLE_SKIP;

    return (estat);
}
/***************************************************************/
#if IS_WINDOWS
int chndlr_wtype(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix) {
/*
Syntax: (debug only)

    wtype [ <parm> ] [ ...]                     Displays wtypes
*/
    int estat;
    int estat2;
    int nparms;
    struct parmlist pl;
    char wtypbuf[ASSOC_MAX];

    estat = 0;
    nparms = 0;

    estat = parse_parmlist(ce, cmdstr, cmdix, &pl);
    if (estat) return (estat);

    estat = handle_redirs(ce, &pl);
    if (estat) return (estat);

    while (!estat && nparms < pl.pl_ce->ce_argc) {
        estat = get_wtype(ce, pl.pl_ce->ce_argv[nparms], wtypbuf, sizeof(wtypbuf));

        if (!estat) {
            printll(pl.pl_ce, "%s\n", wtypbuf);
        }
        nparms++;            
    }

    estat2 = free_parmlist_buf(ce, &pl);
    if (!estat) estat = estat2;
    else if (estat2) estat = chain_error(ce, estat, estat2);

    return (estat);
}
#endif
/***************************************************************/
char * find_globalvar(struct glob * gg,
                  const char * vnam,
                  int vnlen) {

    char * vval;
    intr * pix;

    pix = (intr *)dbtree_find(gg->gg_genv, vnam, vnlen);
    if (pix) {
        vval = gg->gg_envp[*pix] + vnlen + 1;
    } else {
        vval = NULL;
    }

    return (vval);
}
/***************************************************************/
char * find_globalvar_ex(struct cenv * ce,
                  const char * vnam,
                  int vnlen) {

/*
**  Internally defined variables:
**      ARGC            Number of arguments used to call script/function
**      DESHVERSION
**      ERRCMD          In catch, command that caused error
**      ERRMSG          In catch, error message from command that caused error
**      ERRNUM          In catch, desh error number of command that caused error
**      HOME            Home directory of user
**      PID             Process ID of desh process
**      PWD             Current working directory
*/
    int estat;
    char * vval;
    static char pwd_var[PATH_MAXIMUM];

    vval = find_globalvar(ce->ce_gg, vnam, vnlen);
    if (!vval) {
        switch (vnlen) {
            case 3:
                if (!memcmp(vnam, "PWD", vnlen)) {
                    if (!GETCWD(pwd_var, PATH_MAXIMUM)) {
                        pwd_var[0] = '\0';
                    }
                    vval = pwd_var;
                } else if (!memcmp(vnam, "PID", vnlen)) {
                    sprintf(pwd_var, "%d", rx_get_pid());
                    vval = pwd_var;
                }
                break;

            case 4:
                if (!memcmp(vnam, "ARGC", vnlen)) {
                    sprintf(pwd_var, "%d", ce->ce_argc - 1);
                    vval = pwd_var;
/*
                } else if (!memcmp(vnam, "DESH", vnlen)) {
                    vval = get_global_argv0();
*/
                } else if (!memcmp(vnam, "HOME", vnlen)) {
                    estat = get_home_directory(ce, pwd_var, sizeof(pwd_var));
                    if (estat) {
                        set_errmsg(ce, "$HOME", estat);
                        strmcpy(pwd_var, ce->ce_errmsg, PATH_MAXIMUM);
                        clear_error(ce, estat);
                    }
                    vval = pwd_var;
                }
                break;

            case 6:
                if (!memcmp(vnam, "ERRMSG", vnlen)) {
                    if (ce->ce_errmsg) {
                        strmcpy(pwd_var, ce->ce_errmsg, PATH_MAXIMUM);
                    } else {
                        pwd_var[0] = '\0';
                    }
                    vval = pwd_var;
                } else if (!memcmp(vnam, "ERRCMD", vnlen)) {
                    if (ce->ce_errcmd) {
                        strmcpy(pwd_var, ce->ce_errcmd, PATH_MAXIMUM);
                    } else {
                        pwd_var[0] = '\0';
                    }
                    vval = pwd_var;
                } else if (!memcmp(vnam, "ERRNUM", vnlen)) {
                    sprintf(pwd_var, "%d", ce->ce_errnum);
                    vval = pwd_var;
                }
                break;

            case 11:
                if (!memcmp(vnam, "DESHVERSION", vnlen)) {
                    get_desh_version(3, pwd_var, PATH_MAXIMUM);
                    vval = pwd_var;
                }
                break;

            default:
                break;
        }
    }

    return (vval);
}
/***************************************************************/
struct var * find_var(struct cenv * ce,
                  const char * vnam,
                  int vnlen) {

    struct var * vval;
    struct var ** vhand;

    if (ce->ce_vars) {
        vhand = (struct var **)dbtree_find(ce->ce_vars, vnam, vnlen);
    } else {
        vhand = NULL;
    }

    if (vhand) {
        vval = (*vhand);
    } else {
        if (ce->ce_file_ce && ce->ce_file_ce->ce_vars) {
            vhand = (struct var **)dbtree_find(ce->ce_file_ce->ce_vars, vnam, vnlen);
        }

        if (vhand) {
            vval = (*vhand);
        } else {
            vval = NULL;
        }
    }

    return (vval);
}
/***************************************************************/
void get_varstr(struct cenv * ce,
            const char * cmdbuf,
            int * pix,
            struct str * varstr) {
/*
** assumes IS_FIRST_VARCHAR(cmdbuf[*pix]
*/
    int vlen;

    vlen = 0;
    init_str(varstr, 0);

    if (ce->ce_flags & CEFLAG_CASE) {
        do {
            if (vlen + 1 >= varstr->str_max) {
                varstr->str_max += STR_CHUNK_SIZE;
                varstr->str_buf = Realloc(varstr->str_buf, char, varstr->str_max);
            }
            varstr->str_buf[vlen++] = toupper(cmdbuf[*pix]);
            (*pix) += 1;
        } while (IS_VARCHAR(cmdbuf[*pix]));
    } else {
        do {
            if (vlen + 1 >= varstr->str_max) {
                varstr->str_max += STR_CHUNK_SIZE;
                varstr->str_buf = Realloc(varstr->str_buf, char, varstr->str_max);
            }
            varstr->str_buf[vlen++] = cmdbuf[*pix];
            (*pix) += 1;
        } while (IS_VARCHAR(cmdbuf[*pix]));
    }
    varstr->str_len         = vlen;
    varstr->str_buf[vlen] = '\0';
}
/***************************************************************/
int sub_parm_exp(struct cenv * ce,
            const char * cmdbuf,
            int * pix,
            struct str * parmstr,
            int subflags) {

    int estat;
    int eol;
    struct var result;
    struct str tokestr;

    init_str(&tokestr, 0);
    eol = get_token_exp(ce, cmdbuf, pix, &tokestr);
    if (eol) {
        if (eol > 0) estat = eol;
        else         estat = set_error(ce, EUNEXPEOL, "${");
    } else {
        estat = parse_exp(ce, cmdbuf, pix, &tokestr, &result);

        if (!estat) {
            if (tokestr.str_buf[0] == '}' && !tokestr.str_buf[1]) {
                vensure_str(ce, &result);
                append_str_len(parmstr, result.var_buf, result.var_len + 1);
                parmstr->str_len -= 1; /* remove null at end */
            } else {
                estat = set_error(ce, EMISSINGBRACE, NULL);
            }
            free_var_buf(&result);
        }
    }
    free_str_buf(&tokestr);

    return (estat);
}
/***************************************************************/
int read_file_into_string(struct cenv * ce,
                        int fnum,
                        const char * filnam,
                        struct str * cmdout) {

    int estat;
    struct filinf * fi;

    estat = 0;
    fi = new_filinf(fnum, filnam, TMPFIL_LINEMAX);

    while (!estat) {
        estat = read_filinf(ce, fi, PROMPT_NONE, NULL);
        if (!estat) {
            append_str_len(cmdout, fi->fi_inbuf, fi->fi_inbuflen);
        }
    }
    if (estat == ESTAT_EOF) estat = 0;

    set_filinf_flags(fi, FIFLAG_NOCLOSE); /* keep file from being closed - closed in tmpfile_close() */
    free_filinf(fi);

    if (cmdout->str_len && cmdout->str_buf[cmdout->str_len - 1] == '\n') {
        cmdout->str_len -= 1;
    }

    if (cmdout->str_len && cmdout->str_buf[cmdout->str_len - 1] == '\r') {
        cmdout->str_len -= 1;
    }
    APPCH(cmdout, '\0');

    return (estat);
}
/***************************************************************/
#define CMD_LIST 1
int execute_command_string(struct cenv * ce,
                        const char * cmdbuf,
                        struct str * cmdout) {
/*
**          estat = execute_command_handler(ce, cmdstr, cmdix, 0);
*/
    int estat;
    int cmdix;
    int fnum;
    int close_flags;
    struct cenv * nce;
    struct tempfile * stdout_tf;
    struct str cmdstr;
#if CMD_LIST == 0
    struct nest * ne;
#endif

    estat = 0;
    cmdix = 0;
    close_flags = FREECE_STMTS | FREECE_VARS;

    stdout_tf = tmpfile_open();
    if (!stdout_tf) {
        estat = set_ferror(ce, EOPENTMPFIL, "%m", ERRNO);
        return (estat);
    }

    init_str(&cmdstr, 0);
    set_str_char(&cmdstr, cmdbuf);

    nce = new_cenv(ce->ce_stdin_f,
                    tmpfile_filno(stdout_tf),
                    ce->ce_stderr_f,
                    NULL,
                    close_flags,
                    0,
                    NULL,
                    ce->ce_flags,
                    ce->ce_funclib,
                    ce->ce_gg,
                    NULL,
                    ce);

#if CMD_LIST == 1
    estat = execute_command_list(nce, &cmdstr, &cmdix, ce->ce_nest->ne_flags);
    if (estat == ESTAT_IDLEERR) {
        ce->ce_nest->ne_idle = NEIDLE_ERROR;
        estat = 0;
    }
#else
    ne = new_nest(nce, NETYP_CMD);
    ne->ne_flags = ce->ce_nest->ne_flags;

    estat = execute_command_handler(nce, &cmdstr, &cmdix, 0);

    if (!estat) {
        if (!nce->ce_nest || nce->ce_nest->ne_typ != NETYP_CMD) {
            estat = set_error(ce, EMISSINGEND, NULL);
        } else {
            if (nce->ce_nest->ne_idle == NEIDLE_ERROR) ce->ce_nest->ne_idle = NEIDLE_ERROR;
        }
    }
#endif

    if (!estat && !ce->ce_nest->ne_idle) {
        fnum = tmpfile_reset(stdout_tf);
        if (fnum < 0) {
            estat = set_ferror(ce, ERESETTMPFIL, "%m", ERRNO);
        } else {
            estat = read_file_into_string(ce, fnum, tmpfile_filnam(stdout_tf), cmdout);
        }
    }
    tmpfile_close(stdout_tf);

    free_cenv(ce, nce);

    free_str_buf(&cmdstr);

    return (estat);
}
/***************************************************************/
int nondelimiting_special(char ch) {
/*
** Returns 1 if ch is a special character that is *allowed*
** in a Warehouse symbol.  Otherwise 0 is returned.
*/

    if (ch == '+' || ch == '-' || ch == '*' || ch == '/' ||
        ch == '?' || ch == '.' || ch == '%' || ch == '&' ||
        ch == '@' || ch == '_' || ch == '$' || ch == ':' ||
        ch == '\\')
                                 return (1);
    else                         return (0);
}
/***************************************************************/
void append_str_quoted (struct str * sbuf,
                        const char * cbuf,
                        int          clen)
{
    int needsq;
    int ii;

    needsq = 0;
    if (!clen) needsq = 1; /* zero length parm needs to be '' */

    for (ii = 0; !needsq && ii < clen; ii++) {
        if (!isalnum(cbuf[ii]) && !nondelimiting_special(cbuf[ii])) needsq = 1;
    }
    if (needsq) APPCH(sbuf, '\'');

    for (ii = 0; ii < clen; ii++) {
        APPCH(sbuf, cbuf[ii]);
        if (cbuf[ii] == '\'') APPCH(sbuf, cbuf[ii]);
    }
    if (needsq) APPCH(sbuf, '\'');
}
/***************************************************************/
int sub_parm_parms(struct cenv * ce,
            const char * cmdbuf,
            int * pix,
            struct str * parmstr,
            int subflags) {

    int estat;
    int ii;

    estat = 0;
    for (ii = 1; ii < ce->ce_argc; ii++) {
        if (ii > 1) APPCH(parmstr, ' ');
        if (subflags & SUBPARM_QUOTE) {
            append_str_quoted(parmstr, ce->ce_argv[ii], Istrlen(ce->ce_argv[ii]));
        } else {
            append_str_char(parmstr, ce->ce_argv[ii]);
        }
    }

    return (estat);
}
/***************************************************************/
int sub_parm(struct cenv * ce,
            const char * cmdbuf,
            int * pix,
            struct str * parmstr,
            int subflags) {
/*
**    $var            Substitutes with var value
**
**    $%xx            Substitutes characted represented by hexadecimal xx, e,g.
**                    line feed is $%0A, return is $%0D
**
**    $/              Substitutes directory separator.  That is "\" on Windows
**                    and "/" on others.
**
**    $^var           Substitutes with var value in line - not done in sub_parm()
**                      - done in sub_parm_inline()
**
**    $$              Substitutes $.
**
**    $n              Substitutes parameter n
**
**    $*              Substitutes all parameters
**
**    $?              Substitutes last return code
**
**    ${exp}          Substitutes with expression result
**
**    $(command)      Substitutes with command output * unimplemented
**
**    `command`       Substitutes with command output - done in sub_back_quote()
**
**    ^fileset        Substitutes with files matching fileset
**                      - done in sub_parm_fileset() and get_token_quote()
**
*/
    int estat;
    struct var * vval;
    char * gvval;
    int pn;
    struct str varstr;
    int ii;
    char nbuf[16];

    estat = 0;

    if (isdigit(cmdbuf[*pix])) {
        pn = cmdbuf[*pix] - '0';
        (*pix) += 1;
        if (pn < ce->ce_argc) {
            append_str(parmstr, ce->ce_argv[pn]);
        }

    } else if (cmdbuf[*pix] == '%') {
        (*pix) += 1;
        if (isalnum(cmdbuf[*pix]) && isalnum(cmdbuf[*pix + 1])) {
                 if (cmdbuf[*pix] >= '0' && cmdbuf[*pix] <= '9') pn = cmdbuf[*pix] - '0';
            else if (cmdbuf[*pix] >= 'A' && cmdbuf[*pix] <= 'F') pn = cmdbuf[*pix] - '7';
            else if (cmdbuf[*pix] >= 'a' && cmdbuf[*pix] <= 'f') pn = cmdbuf[*pix] - 'W';
            else pn = 0;
            (*pix) += 1;
            pn <<= 4;
                 if (cmdbuf[*pix] >= '0' && cmdbuf[*pix] <= '9') pn |= cmdbuf[*pix] - '0';
            else if (cmdbuf[*pix] >= 'A' && cmdbuf[*pix] <= 'F') pn |= cmdbuf[*pix] - '7';
            else if (cmdbuf[*pix] >= 'a' && cmdbuf[*pix] <= 'f') pn |= cmdbuf[*pix] - 'W';
            (*pix) += 1;
            APPCH(parmstr, pn);
        }

    } else if (cmdbuf[*pix] == '/') {
        (*pix) += 1;
        APPCH(parmstr, DIR_DELIM);

    } else if (cmdbuf[*pix] == '$') {
        (*pix) += 1;
        APPCH(parmstr, '$');

#ifdef ALLOW_BUCKS_POUND    /* does not work because # is comment char */
    } else if (cmdbuf[*pix] == '#') {
        (*pix) += 1;
        Snprintf(nbuf, sizeof(nbuf), "%d", ce->ce_argc - 1);
        for (ii = 0; nbuf[ii]; ii++) {
            APPCH(parmstr, nbuf[ii]);
        }
#endif

    } else if (isdigit(cmdbuf[*pix])) {
        ii = 0;
        do {
            ii = (ii * 10) + cmdbuf[*pix] - '0';
            (*pix) += 1;
        } while (isdigit(cmdbuf[*pix]));

        if (ii >= 0 && ii < ce->ce_argc) {
            if (subflags & SUBPARM_QUOTE) {
                append_str_quoted(parmstr, ce->ce_argv[ii], Istrlen(ce->ce_argv[ii]));
            } else {
                append_str_char(parmstr, ce->ce_argv[ii]);
            }
        }

    } else if (cmdbuf[*pix] == '?') {
        (*pix) += 1;
        Snprintf(nbuf, sizeof(nbuf), "%d", ce->ce_rtn);
        for (ii = 0; nbuf[ii]; ii++) {
            APPCH(parmstr, nbuf[ii]);
        }

/*
    } else if (cmdbuf[*pix] == '~') {
        (*pix) += 1;
        gvval = find_globalvar_ex(ce, "HOME", 4);
        if (gvval) {
            if (subflags & SUBPARM_QUOTE) {
                append_str_quoted(parmstr, gvval, strlen(gvval));
            } else {
                append_str(parmstr, gvval);
            }
        }
*/

    } else if (IS_FIRST_VARCHAR(cmdbuf[*pix])) {
        get_varstr(ce, cmdbuf, pix, &varstr);
        vval = find_var(ce, varstr.str_buf, varstr.str_len);
        if (vval) {
            if ((subflags & SUBPARM_QUOTE) && (vval->var_typ == VTYP_STR)) {
                append_str_quoted(parmstr, vval->var_buf, vval->var_len);
            } else {
                append_str_var(ce, parmstr, vval);
            }
        } else {
            gvval = find_globalvar_ex(ce, varstr.str_buf, varstr.str_len);
            if (gvval) {
                if (subflags & SUBPARM_QUOTE) {
                    append_str_quoted(parmstr, gvval, Istrlen(gvval));
                } else {
                    append_str(parmstr, gvval);
                }
            }
        }
        free_str_buf(&varstr);
    } else if (cmdbuf[*pix] == '{') {
        (*pix) += 1;
        estat = sub_parm_exp(ce, cmdbuf, pix, parmstr, subflags);
    } else if (cmdbuf[*pix] == '*') {
        (*pix) += 1;
        estat = sub_parm_parms(ce, cmdbuf, pix, parmstr, subflags);
    } else {
        APPCH(parmstr, '$'); /* bad char after '$' -- error ?? e.g. $@ --> $@ */
    }

    return (estat);
}
/***************************************************************/
int sub_parm_inline(struct cenv * ce,
                struct str * cmdstr,
                int * pix,
                int subflags) {

    int estat;
    int stix;
    int pnlen;
    int vndiff;
    int taillen;
    struct str parmstr;

    init_str(&parmstr, 0);

    stix = (*pix);
    (*pix) += 1; /* skip $ */
    if (cmdstr->str_buf[*pix] == '^') (*pix) += 1; /* skip ^ */
    else subflags |= SUBPARM_QUOTE ;

    estat = sub_parm(ce, cmdstr->str_buf, pix, &parmstr, subflags /* | SUBPARM_QUOTE */);
    if (!estat) {
        pnlen = (*pix) - stix;
        vndiff = parmstr.str_len - pnlen;
        taillen = cmdstr->str_len - (*pix) + 1;
        if (vndiff) {
            if (vndiff > 0) {
                if (cmdstr->str_len + vndiff >= cmdstr->str_max) {
                    cmdstr->str_max = cmdstr->str_len + vndiff + 16;
                    cmdstr->str_buf = Realloc(cmdstr->str_buf, char, cmdstr->str_max);
                }
            }
            memmove(cmdstr->str_buf + stix + parmstr.str_len,
                    cmdstr->str_buf + (*pix), taillen);
            cmdstr->str_len += vndiff;
        }
        memcpy(cmdstr->str_buf + stix, parmstr.str_buf, parmstr.str_len);
        (*pix) = stix;
    }
    free_str_buf(&parmstr);

    return (estat);
}
/***************************************************************/
void append_str_char_quotes(struct str * ss, const char * buf,  int quotes) {
/*
**  quotes
**      0 - no quotes
**      1 - always quote
**      2 - quote if needed
*/
    if (quotes & SUBPARM_QUOTE) {
        APPCH(ss, '\"');
        append_str_len(ss, buf, Istrlen(buf));
        APPCH(ss, '\"');
    } else if (quotes & SUBPARM_QUOTE_IF_NEEDED) {
        append_str_quoted(ss, buf, Istrlen(buf));
    } else {
        append_str_len(ss, buf, Istrlen(buf));
    }
}
/***************************************************************/
struct str * charlist_to_str(char ** charlist, int quotes) {

    struct str * ss;
    int ii;

    ss = New(struct str, 1);
    init_str(ss, INIT_STR_CHUNK_SIZE);

    for (ii = 0; charlist[ii]; ii++) {
        if (ii) APPCH(ss, ' ');
        append_str_char_quotes(ss, charlist[ii], quotes);
    }
    APPCH(ss, '\0');

    return (ss);
}
/***************************************************************/
int sub_parm_fileset(struct cenv * ce,
                struct str * cmdstr,
                int   init_ix,
                int   ix,
                struct str * parmstr) {

    int estat;
    int pnlen;
    int vndiff;
    int taillen;
    char ** flist;
    struct str * fliststr;

    estat = get_filenames(ce, parmstr->str_buf, &flist);
    if (estat) return (estat);

    taillen = cmdstr->str_len - ix + 1;

    if (!flist || !flist[0]) {
        set_str_char(parmstr, "");
        memmove(cmdstr->str_buf + init_ix + parmstr->str_len,
                cmdstr->str_buf + ix, taillen);
    } else {
        fliststr = charlist_to_str(flist + 1, 1);

        set_str_char(parmstr, flist[0]);

        pnlen = ix - init_ix;
        vndiff = fliststr->str_len - pnlen;
        if (vndiff) {
            if (vndiff > 0) {
                if (cmdstr->str_len + vndiff >= cmdstr->str_max) {
                    cmdstr->str_max = cmdstr->str_len + vndiff + 16;
                    cmdstr->str_buf = Realloc(cmdstr->str_buf, char, cmdstr->str_max);
                }
            }
            memmove(cmdstr->str_buf + init_ix + fliststr->str_len,
                    cmdstr->str_buf + ix, taillen);
            cmdstr->str_len += vndiff;
        }
        memcpy(cmdstr->str_buf + init_ix, fliststr->str_buf, fliststr->str_len);

        free_str_buf(fliststr);
        Free(fliststr);
    }
    free_charlist(flist);

    return (estat);
}
/***************************************************************/
int sub_parm_fileset_parm(struct cenv * ce,
                struct str * parmstr) {

    int estat;
    char ** flist;
    struct str * fliststr;

    estat = get_filenames(ce, parmstr->str_buf, &flist);
    if (estat) return (estat);

    if (!flist || !flist[0]) {
        set_str_char(parmstr, "");
    } else {
        fliststr = charlist_to_str(flist, SUBPARM_QUOTE_IF_NEEDED);
        free_str_buf(parmstr);
        qcopy_str(parmstr, fliststr);
        Free(fliststr);
    }
    free_charlist(flist);

    return (estat);
}
/***************************************************************/
int sub_back_quote(struct cenv * ce, struct str * parmstr, int psix) {
/*
**          estat = execute_command_handler(ce, cmdstr, cmdix, 0);
*/
    int estat;
    struct str cmdout;

    estat = 0;
    init_str(&cmdout, 0);

    estat = execute_command_string(ce, parmstr->str_buf + psix, &cmdout);

    append_str_len_at(parmstr, psix, cmdout.str_buf, cmdout.str_len);
    free_str_buf(&cmdout);

    return (estat);
}
/***************************************************************/
int get_parmstr(struct cenv * ce,
                struct str * cmdstr,
                int * cmdix,
                int parmflags,
                struct str * parmstr) {
/*
**  parmflags:
**      0 = normal
**      PFLAG_SLASH_DELIMITS 1 = slash (/) delimits
*/

    int estat;
    int pdepth;
    char ch;
    char quot;
    char quot0;
    char paren;
    char cparen;
    int ix;
    int init_ix;
    int eop;
    int sub_wild;
    int any_wild;
    int back_quote;
    int num_inline_subs;
    int subflags;

    estat           = 0;

    num_inline_subs = 0;

    pdepth      = 0;
    quot        = 0;
    quot0       = 0;
    paren       = 0;
    cparen      = 0;
    back_quote  = -1;
    ix          = (*cmdix);
    init_ix     = ix;
    ch          = cmdstr->str_buf[ix];

    any_wild    = 0;
    sub_wild    = 0;
    if (ce->ce_flags & CEFLAG_SUBFSET) sub_wild = 1;

    eop = 0;

    while (!eop) {
        switch (ch) {
            case '\0':
                eop = 1;
                if (quot && (parmflags & PFLAG_QUOTE)) {
                    estat = set_ferror(ce, EUNMATCHEDQUOTE, "%c", quot);
                }
                break;

            case '\"':
            case '\'':
                if (quot == 0) {
                    quot = ch;
                    ch = 0;
                    ix += 1;
                } else if (ch == quot) {
                    ix += 1;
                    if (cmdstr->str_buf[ix] != quot) {
                        ch = 0;
                        quot = 0;
                        if (parmflags & PFLAG_QUOTE) {
                            eop = 1;
                        }
                    }
                }
                break;

            case '`':
                if (quot == 0 || quot == '\"') {
                    ix++;
                    if (cmdstr->str_buf[ix] != ch) {
                        quot0       = quot;
                        quot        = ch;
                        back_quote  = parmstr->str_len;
                        ch          = 0;
                    }
                } else if (ch == quot) {
                    ix++;
                    if (cmdstr->str_buf[ix] != quot) {
                        APPCH(parmstr, '\0');
                        estat = sub_back_quote(ce, parmstr, back_quote);
                        ch          = 0;
                        quot        = quot0;
                        quot0       = 0;
                        back_quote  = -1;
                    }
                }
                break;

            case '(':
            case '[':
            case '{':
                if (quot == 0) {
                    if (parmflags & PFLAG_EXP) {
                        eop = 1;
                    } else if (!pdepth) {
                        paren = ch;
                        pdepth = 1;
                        if (paren == '[')       cparen = ']';
                        else if (paren == '{')  cparen = '}';
                        else                    cparen = ')';
                    } else if (ch == paren) {
                        pdepth++;
                    }
                }
                break;

            case ')':
            case ']':
            case '}':
                if (quot == 0) {
                    if (pdepth) {
                        if (ch == cparen) {
                            pdepth--;
                            if (!pdepth) cparen = 0;
                            if (ch == ']') {
                                any_wild |= 1;
                                if (quot == '\"') any_wild |= 2;
                            }
                        }
                    } else if (parmflags & PFLAG_EXP) {
                        eop = 1;
                    }
                }
                break;

            case ' ':
            case '\r':
            case '\t':
            case '\n':
                if (quot == 0 && pdepth == 0) {
                    eop = 1;
                }
                break;

            case '/':
                if ((parmflags & PFLAG_SLASH_DELIMITS) && quot == 0 && pdepth == 0) {
                    if (parmstr->str_len) eop = 1;
                }
                break;

            case '^':
                if ((quot == 0 || quot != '\'') && pdepth == 0) {
                    if (cmdstr->str_buf[ix + 1] != '^' && parmstr->str_len == 0) {
                        sub_wild = 1;
                        ch = 0;
                    }
                    ix += 1;
                }
                break;

            case '*':
            case '?':
                if (quot == 0 || quot == '\"') {
                    any_wild |= 1;
                    if (quot == '\"') any_wild |= 2;
                }
                break;

            case '$':
                if (quot == 0 || quot != '\'') {
                    if ((cmdstr->str_buf[ix + 1] == '^' && cmdstr->str_buf[ix + 2] != '^') ||
                        cmdstr->str_buf[ix + 1] == '*' ) {
                        if (num_inline_subs < MAX_INLINE_SUBS) {
                            estat = sub_parm_inline(ce, cmdstr, &ix, SUBPARM_NONE);
                            num_inline_subs++;
                        } else {
                            estat = set_ferror(ce, EMAXSUBS, "%d", MAX_INLINE_SUBS);
                        }
                    } else {
                        ix += 1;
                        subflags = SUBPARM_NONE;
                        if (quot == '`') subflags = SUBPARM_QUOTE;
                        estat = sub_parm(ce, cmdstr->str_buf, &ix, parmstr, subflags);
                    }
                    ch = 0; /* do not put char in (*parmstrlist)[0] */
                    if (estat) eop = 1;
                }
                break;

            default:
                if ((parmflags & PFLAG_EXP) && quot == 0 && pdepth == 0) {
                    if (!IS_VARCHAR(ch)) {
                        eop = 1;
                    }
                }
                break;
        }

        if (!eop)  {
            if (ch) {
                APPCH(parmstr, ch);
                ix += 1;
            }

            ch = cmdstr->str_buf[ix];
        }
    }
    (*cmdix) = ix;
    APPCH(parmstr, '\0');

    if (!estat && sub_wild && any_wild && !ISIDLE(ce)) { /* added !ISIDLE(ce) 6/30/2014 */
        if (any_wild & 2) {
            estat = sub_parm_fileset_parm(ce, parmstr);
        } else {
            estat = sub_parm_fileset(ce, cmdstr, init_ix, ix, parmstr);
            (*cmdix) = init_ix;
        }
    }

    if (estat) free_str_buf(parmstr);

    return (estat);
}
/***************************************************************/
int check_no_redir(struct cenv * ce, const char * parm) {

    int estat;

    estat = 0;
 
    if (parm[0] == '<' || parm[0] == '>' ||
        parm[0] == '|' || 
            (isdigit(parm[0]) && parm[1] == '>')) {
        estat = set_error(ce, EBADREDIR, NULL);
    }

    return (estat);
}
/***************************************************************/
int get_parm(struct cenv * ce,
            struct str * cmdstr,
            int * cmdix,
            int parmflags,
            struct str * parmstr) {
/*
** This is here so that get_parm() cannot return a zero length parm.
** A a zero length parm is used to indicate end of command.
*/
    int estat;

    estat = 0;

    while (isspace(cmdstr->str_buf[*cmdix])) (*cmdix)++;
    estat = check_no_redir(ce, cmdstr->str_buf + (*cmdix));
    if (estat) return (estat);

    while (!estat && !parmstr->str_len && cmdstr->str_buf[*cmdix]) {
        estat = get_parmstr(ce, cmdstr, cmdix, parmflags, parmstr);
        if (!estat) {
            if (!parmstr->str_len) {
                while (isspace(cmdstr->str_buf[*cmdix])) (*cmdix)++;
            }
        }
    }
    if (estat) free_str_buf(parmstr);

    return (estat);
}
/***************************************************************/
void init_parmlist(struct parmlist * pl) {

    pl->pl_nparms       = 0;
    pl->pl_mxparms      = 0;
    pl->pl_redir_flags  = 0;
    pl->pl_xeqflags     = 0;
    pl->pl_parm1_subbed = 0;
    pl->pl_cparmlist    = NULL;
    pl->pl_cparmlens    = NULL;
    pl->pl_cparmmaxs    = NULL;
    pl->pl_stdin_fnam   = NULL;
    pl->pl_stdout_fnam  = NULL;
    pl->pl_stderr_fnam  = NULL;
    pl->pl_stdin_tf     = NULL;
    pl->pl_ce           = NULL;
    pl->pl_piped_flags  = 0;

    pl->pl_vrs            = NULL;
    pl->pl_piped_parmlist = NULL;

#if IS_DEBUG_PARM0
    pl->pl_dbg_parm0 = NULL;
#endif
}
/***************************************************************/
void add_proclist(void *** proclist, int * mxprocs, int * nprocs, void * vrs) {

    if ((*nprocs) == (*mxprocs)) {
        (*mxprocs) += 4; /* chunk */
        (*proclist) = Realloc((*proclist), void*, (*mxprocs));
    }
    (*proclist)[(*nprocs)] = vrs;
    (*nprocs) += 1;
}
/***************************************************************/
void free_parmlist_data(struct cenv * ce,
                        struct parmlist * pl,
                        void *** proclist,
                        int * mxprocs,
                        int * nprocs) {

    int ii;

    if (pl->pl_vrs) {
        add_proclist(proclist, mxprocs, nprocs, pl->pl_vrs);
    }

    if (pl->pl_piped_parmlist) {
        free_parmlist_data(ce, pl->pl_piped_parmlist, proclist, mxprocs, nprocs);
        Free(pl->pl_piped_parmlist);
    }

    if (pl->pl_ce) {
        free_cenv(ce, pl->pl_ce);
    }

    for (ii = 0; ii < pl->pl_nparms; ii++) Free(pl->pl_cparmlist[ii]);
    Free(pl->pl_cparmlist);
    Free(pl->pl_cparmlens);
    Free(pl->pl_cparmmaxs);

    Free(pl->pl_stdin_fnam);
    Free(pl->pl_stdout_fnam);
    Free(pl->pl_stderr_fnam);

#if IS_DEBUG_PARM0
    if (pl->pl_dbg_parm0) Free(pl->pl_dbg_parm0);
#endif

    if (pl->pl_stdin_tf) tmpfile_close(pl->pl_stdin_tf);
}
/***************************************************************/
void setvar_start_pidlist(struct cenv * ce, void ** proclist, int nprocs, int rxwaitflags) {

    int  ii;
    struct str      pid_str;
    char pidbuf[16];
    char vnam[16];

    init_str(&pid_str, 16);

    for (ii = 0; ii < nprocs; ii++) {
        rx_get_pid_str(proclist[ii], pidbuf, sizeof(pidbuf));
        if (ii) APPCH(&pid_str, ' ');
        append_str(&pid_str, pidbuf);
    }

    strcpy(vnam, "DESHSTARTPID");
    set_ce_var_str(ce, vnam, Istrlen(vnam), pid_str.str_buf, pid_str.str_len);

    if (rxwaitflags & RXWAIT_VERBOSE) {
        fprintf(stderr, "%s\n", pid_str.str_buf);
        fflush(stderr);
    }

    free_str_buf(&pid_str);
}
/***************************************************************/
int free_parmlist_buf(struct cenv * ce, struct parmlist * pl) {

    int estat;
    void ** proclist;
    int mxprocs;
    int nprocs;
    int rxwaitflags;

    estat   = 0;
    proclist = NULL;
    mxprocs  = 0;
    nprocs   = 0;
    rxwaitflags = 0;

    free_parmlist_data(ce, pl, &proclist, &mxprocs, &nprocs);

    if (nprocs) {
        if (pl->pl_xeqflags & XEQFLAG_NOWAIT) {
            if (pl->pl_xeqflags & XEQFLAG_VERBOSE) {
                rxwaitflags |= RXWAIT_VERBOSE;
            }

            setvar_start_pidlist(ce, proclist, nprocs, rxwaitflags);
            estat = check_proclist(ce, proclist, nprocs, rxwaitflags);
            Free(proclist);
        } else {
            estat = wait_proclist(ce, proclist, nprocs, rxwaitflags);
            if (!estat && ce->ce_rtn) {
                estat = set_ferror(ce, EPROGERR, "%d", ce->ce_rtn);
            }
        }
    }

    return (estat);
}
/***************************************************************/
int check_redir(struct cenv * ce, const char * parm, int * pix) {

    int redir_flags;
    int fno;

    redir_flags = 0;
 
    if (isdigit(parm[*pix])) {
        fno = parm[*pix] - '0';
        if (parm[(*pix) + 1] == '>') {
            if (fno < 1 || fno > 2) {
                redir_flags = REDIR_ERROR; /* use n> where 2 > 2 */
            } else if (parm[(*pix) + 2] == '>') {
                if (fno == 1) redir_flags = REDIR_STDOUT_APPEND;
                else          redir_flags = REDIR_STDERR_APPEND;
                (*pix) += 3;
            } else {
                (*pix) += 2;

                if (fno == 1) {
                    redir_flags = REDIR_STDOUT_FILE;
                } else {
                    redir_flags = REDIR_STDERR_FILE;
                }
            }           

        } else if (parm[(*pix) + 1] == '<') {
            if (fno) {
                redir_flags = REDIR_ERROR;  /* use < on stdout or stderr */
            } else {
                if (parm[(*pix) + 2] == '<') {
                    if (parm[(*pix) + 3] == '<') {
                        redir_flags = REDIR_STDIN_HERE_NOSUB;  /* n<<<name */
                        (*pix) += 4;
                    } else {
                        redir_flags = REDIR_STDIN_HERE;  /* n<<name */
                        (*pix) += 3;
                    }
                } else {
                    redir_flags = REDIR_STDIN_FILE; /* n<file */
                    (*pix) += 2;
               }
            }
        }

    } else if (parm[(*pix)] == '>') {
        if (parm[(*pix) + 1] == '>') {
            redir_flags = REDIR_STDOUT_APPEND; /* >>file */
            (*pix) += 2;
        } else {
            redir_flags = REDIR_STDOUT_FILE; /* >>file */
            (*pix) += 1;
        }           

    } else if (parm[(*pix)] == '<') {
        if (parm[(*pix) + 1] == '<') {
             if (parm[(*pix) + 2] == '<') {
                redir_flags = REDIR_STDIN_HERE_NOSUB;  /* <<<name */
                (*pix) += 3;
            } else {
                redir_flags = REDIR_STDIN_HERE;  /* <<name */
                (*pix) += 2;
            }
        } else {
            redir_flags = REDIR_STDIN_FILE; /* <file */
            (*pix) += 1;
        }

    } else if (parm[(*pix)] == '|') {
        redir_flags = REDIR_PIPE;
        (*pix) += 1;
    }

    if (redir_flags) {
        while (isspace(parm[*pix])) (*pix)++;
        if (parm[(*pix)] == '>' || parm[(*pix)] == '<' ||
            parm[(*pix)] == '|' || parm[(*pix)] == '\0') {
            redir_flags = REDIR_ERROR;  /* bad char after redir */
        }
    }

    return (redir_flags);
}
/***************************************************************/
#if DEBUG_SHOW_PARMLIST
void show_parmlist(struct parmlist * pl) {

    int ii;

    printf("parmlist:\n");
    for (ii = 0; ii < pl->pl_nparms; ii++) {
        printf("%4d %s\n",  ii, pl->pl_cparmlist[ii]);
    }
}
#endif
/***************************************************************/
int parse_parmlist(struct cenv * ce,
                    struct str * cmdstr,
                    int * cmdix,
                    struct parmlist * pl) {
    int estat;
    struct str parmstr;
    int nparms;
    int mxparms;
    int redir_flags;

    estat = 0;
    nparms = 0;
    mxparms = 0;
    init_parmlist(pl);

    while (!estat && cmdstr->str_buf[*cmdix]) {
        while (isspace(cmdstr->str_buf[*cmdix])) (*cmdix)++;
        init_str(&parmstr, 0);

        redir_flags = check_redir(ce, cmdstr->str_buf, cmdix);
        if (redir_flags) {
            while (isspace(cmdstr->str_buf[*cmdix])) (*cmdix)++;
        }

        if (redir_flags == REDIR_PIPE) {
            pl->pl_piped_parmlist = New(struct parmlist, 1);
            estat = parse_parmlist(ce, cmdstr, cmdix, pl->pl_piped_parmlist);
        } else if (cmdstr->str_buf[*cmdix]) {
            estat = get_parmstr(ce, cmdstr, cmdix, 0, &parmstr);
            if (!estat) {
                if (redir_flags & REDIR_STDIN_MASK) {
                    if (pl->pl_redir_flags & REDIR_STDIN_MASK) {
                        estat = set_error(ce, EDUPREDIR, NULL);
                    } else {
                        pl->pl_stdin_fnam = parmstr.str_buf;
                        pl->pl_redir_flags |= redir_flags;
                    }
                } else if (redir_flags & REDIR_STDOUT_MASK) {
                    if (pl->pl_redir_flags & REDIR_STDOUT_MASK) {
                        estat = set_error(ce, EDUPREDIR, NULL);
                    } else {
                        pl->pl_stdout_fnam = parmstr.str_buf;
                        pl->pl_redir_flags |= redir_flags;
                    }
                } else if (redir_flags & REDIR_STDERR_MASK) {
                    if (pl->pl_redir_flags & REDIR_STDERR_MASK) {
                        estat = set_error(ce, EDUPREDIR, NULL);
                    } else {
                        pl->pl_stderr_fnam = parmstr.str_buf;
                        pl->pl_redir_flags |= redir_flags;
                    }
                } else {
                    if (pl->pl_nparms == pl->pl_mxparms) {
                        if (!pl->pl_mxparms) pl->pl_mxparms = PARMLIST_CHUNK;
                        else          pl->pl_mxparms *= 2;
                        pl->pl_cparmlist = Realloc(pl->pl_cparmlist, char*, pl->pl_mxparms);
                        pl->pl_cparmlens = Realloc(pl->pl_cparmlens, int, pl->pl_mxparms);
                        pl->pl_cparmmaxs = Realloc(pl->pl_cparmmaxs, int, pl->pl_mxparms);
                    }
                    pl->pl_cparmlist[pl->pl_nparms] = parmstr.str_buf;
                    pl->pl_cparmlens[pl->pl_nparms] = parmstr.str_len;
                    pl->pl_cparmmaxs[pl->pl_nparms] = parmstr.str_max;
                    pl->pl_nparms += 1;
                }
            }
        }
    }

    if (estat) {
        free_parmlist_buf(ce, pl);
    } else {
        if (pl->pl_nparms == pl->pl_mxparms) {
            pl->pl_mxparms += 1;
            pl->pl_cparmlist = Realloc(pl->pl_cparmlist, char*, pl->pl_mxparms);
        }
        pl->pl_cparmlist[pl->pl_nparms] = NULL;

#if DEBUG_SHOW_PARMLIST
        show_parmlist(pl);
#endif
    }

    return (estat);
}
/***************************************************************/
int idle_stdin_here(struct cenv * ce, const char * eof_name) {

    int estat;
    int elen;
    int ix;
    int done;
    int linix;
    struct str linstr;

    estat       = 0;
    done        = 0;
    elen        = Istrlen(eof_name);
    init_str(&linstr, INIT_STR_CHUNK_SIZE);

    while (!estat && !done) {
        estat = read_command_string(ce, &linstr, PROMPT_DASH);
        if (!estat) {

            if (saving_cmds(ce) && !ce->ce_reading_saved) {
                linix = 0;
                save_cmd(ce, &linstr, &linix);
            }

            if (linstr.str_len >= elen && !CEMEMCMP(ce, linstr.str_buf, eof_name, elen)) {
                ix = elen;
                while (isspace(linstr.str_buf[ix])) ix++;
                if (!linstr.str_buf[ix]) done = 1;
            }
        }
    }
    free_str_buf(&linstr);

    return (estat);
}
/***************************************************************/
int handle_idle_redirs(struct cenv * ce,
                    struct parmlist * pl) {

    int     estat;

    estat = 0;

    if (pl->pl_redir_flags & REDIR_STDIN_MASK) {
        if (pl->pl_stdin_fnam[0] != '&' || !isdigit(pl->pl_stdin_fnam[1])) {
            estat = idle_stdin_here(ce, pl->pl_stdin_fnam);
        }
    }

    return (estat);
}
/***************************************************************/
int chndlr_idle_generic(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix) {

    int estat;
    char * lt;
    struct parmlist pl;

    estat = 0;

    /* must check for stdin here and save the lines if there is one */
    lt = strchr(cmdstr->str_buf + (*cmdix), '<');
    if (lt && lt[1] == '<') {
        estat = parse_parmlist(ce, cmdstr, cmdix, &pl);
        if (estat) return (0); /* ignore error, since we are idle */

        estat = handle_idle_redirs(ce, &pl);

        free_parmlist_buf(ce, &pl);
    }

    return (estat);
}
/***************************************************************/
int idle_command_handler(struct cenv * ce,
                struct str * cmdstr,
                int * cmdix) {

    int estat;
    int parmflags;
    struct str parmstr;
    struct cmdinfo ** hci;

    estat = 0;

    parmflags = 0;
    if (ce->ce_flags & CEFLAG_WIN) parmflags = PFLAG_SLASH_DELIMITS;

    init_str(&parmstr, 0);

    if (ce->ce_flags & CEFLAG_VERBOSE) {
        printlle(ce, "**%s\n", cmdstr->str_buf + (*cmdix));
    }

    estat = get_parm(ce, cmdstr, cmdix, parmflags, &parmstr);

    if (!estat) {
        if (parmstr.str_len) {
            hci = (struct cmdinfo **)dbtree_find(ce->ce_gg->gg_idle_cmds,
							parmstr.str_buf, parmstr.str_len + 1);

            if (hci) {
                estat = ((*hci)->ci_cmd_handler)(ce, (*hci), cmdstr, cmdix);
            } else {
                estat = chndlr_idle_generic(ce, NULL, cmdstr, cmdix);
            }
        }
        free_str_buf(&parmstr);
    }

    return (estat);
}
/***************************************************************/
int execute_command_handler(struct cenv * ce, 
                        struct str * cmdstr,
                        int * cmdix,
                        int xeqflags) {
/*
void glob_init(struct glob * gg, int gflags) {

    glob_add_cmd(gg, "done", chndlr_endfor);
}
*/
    int estat;
    int ii;
    int parmflags;
    struct str rawparmstr;
    struct str parmstr;
    struct cmdinfo ** hci;

    estat = 0;

    if (global_user_break) {
        estat = handle_ctl_c(ce);
        if (estat) return (estat);
    }

    parmflags = 0;
    if (ce->ce_flags & CEFLAG_WIN) parmflags = PFLAG_SLASH_DELIMITS;

    if (ISIDLE(ce)) {
        estat = idle_command_handler(ce, cmdstr, cmdix);
    } else {
        init_str(&parmstr, 0);
        /* ce->ce_rtn = PROGRTN_SUCC; -- overwrites $? */

        if ((ce->ce_flags & CEFLAG_VERBOSE) && 
            (!ce->ce_filinf || !ce->ce_filinf->fi_stdin_tty)) {
            printlle(ce, "%s%s\n", get_verbose_depth_str(ce,0), cmdstr->str_buf + (*cmdix));
        }

        estat = get_parm(ce, cmdstr, cmdix, parmflags, &parmstr);

        if (!estat) {
            if (parmstr.str_len) {
                copy_str(&rawparmstr, &parmstr);
                if (ce->ce_flags & CEFLAG_CASE) {
                    for (ii = 0; ii < parmstr.str_len; ii++)
                        parmstr.str_buf[ii] = tolower(parmstr.str_buf[ii]);
                }

                hci = (struct cmdinfo **)dbtree_find(ce->ce_gg->gg_cmds,
                                            parmstr.str_buf, parmstr.str_len + 1);

                if (hci) {
                    estat = ((*hci)->ci_cmd_handler)(ce, (*hci), cmdstr, cmdix);
                } else {
                    estat = chndlr_generic(ce, &rawparmstr, &parmstr, cmdstr, cmdix, xeqflags);
                }
                free_str_buf(&rawparmstr);
            }
            free_str_buf(&parmstr);
        }
    }

    return (estat);
}
/***************************************************************/

/***************************************************************/
void glob_add_cmd(struct glob * gg,
                const char * cmdnam, 
                cmd_handler chndlr) {

    struct cmdinfo * ci;

    ci = New(struct cmdinfo, 1);
    ci->ci_cmd_handler = chndlr; 
    ci->ci_cmdpath     = NULL; 

    dbtree_insert(gg->gg_cmds, cmdnam, Istrlen(cmdnam) + 1, ci);
}
/***************************************************************/
void glob_add_idle_cmd(struct glob * gg,
                const char * cmdnam, 
                cmd_handler chndlr) {

    struct cmdinfo * ci;

    ci = New(struct cmdinfo, 1);
    ci->ci_cmd_handler = chndlr; 
    ci->ci_cmdpath     = NULL; 

    dbtree_insert(gg->gg_idle_cmds, cmdnam, Istrlen(cmdnam) + 1, ci);
}
/***************************************************************/
void glob_init(struct glob * gg, int gflags) {

    glob_add_cmd(gg, "call"         , chndlr_call);
    glob_add_cmd(gg, "catch"        , chndlr_catch);
    glob_add_cmd(gg, "cd"           , chndlr_cd);
    glob_add_cmd(gg, "copy"         , chndlr_copy);         /* for Windows */
    glob_add_cmd(gg, "."            , chndlr_dot);
    glob_add_cmd(gg, "dir"          , chndlr_dir);          /* for Windows */
    glob_add_cmd(gg, "echo"         , chndlr_echo);
    glob_add_cmd(gg, "else"         , chndlr_else);
    glob_add_cmd(gg, "endfor"       , chndlr_endfor);
    glob_add_cmd(gg, "endfunction"  , chndlr_endfunction);
    glob_add_cmd(gg, "endif"        , chndlr_endif);
    glob_add_cmd(gg, "endtry"       , chndlr_endtry);
    glob_add_cmd(gg, "endwhile"     , chndlr_endwhile);
    glob_add_cmd(gg, "exit"         , chndlr_exit);
    glob_add_cmd(gg, "export"       , chndlr_export);
    glob_add_cmd(gg, "for"          , chndlr_for);
    glob_add_cmd(gg, "function"     , chndlr_function);
    glob_add_cmd(gg, "help"         , chndlr_help);
    glob_add_cmd(gg, "if"           , chndlr_if);
    glob_add_cmd(gg, "move"         , chndlr_move);
    glob_add_cmd(gg, "remove"       , chndlr_remove);
    glob_add_cmd(gg, "removedir"    , chndlr_removedir);
    glob_add_cmd(gg, "return"       , chndlr_return);
    glob_add_cmd(gg, "set"          , chndlr_set);
    glob_add_cmd(gg, "shift"        , chndlr_shift);
    glob_add_cmd(gg, "shopt"        , chndlr_shopt);
    glob_add_cmd(gg, "start"        , chndlr_start);
    glob_add_cmd(gg, "throw"        , chndlr_throw);
    glob_add_cmd(gg, "time"         , chndlr_time);
    glob_add_cmd(gg, "try"          , chndlr_try);
    glob_add_cmd(gg, "type"         , chndlr_type);         /* for Windows */
    glob_add_cmd(gg, "wait"         , chndlr_wait);
    glob_add_cmd(gg, "while"        , chndlr_while);

    glob_add_idle_cmd(gg, "catch"       , chndlr_catch);
    glob_add_idle_cmd(gg, "else"        , chndlr_else);
    glob_add_idle_cmd(gg, "exit"        , chndlr_exit);
    glob_add_idle_cmd(gg, "endfor"      , chndlr_idle_endfor);
    glob_add_idle_cmd(gg, "endfunction" , chndlr_idle_endfunction);
    glob_add_idle_cmd(gg, "endif"       , chndlr_endif);
    glob_add_idle_cmd(gg, "endtry"      , chndlr_endtry);
    glob_add_idle_cmd(gg, "endwhile"    , chndlr_idle_endwhile);
    glob_add_idle_cmd(gg, "function"    , chndlr_idle_function);
    glob_add_idle_cmd(gg, "for"         , chndlr_idle_for);
    glob_add_idle_cmd(gg, "if"          , chndlr_idle_if);
    glob_add_idle_cmd(gg, "try"         , chndlr_idle_try);
    glob_add_idle_cmd(gg, "while"       , chndlr_idle_while);

#if IS_WINDOWS
    glob_add_cmd(gg, "chmod"            , chndlr_chmod);
    glob_add_cmd(gg, "mkdir"            , chndlr_mkdir);
    glob_add_cmd(gg, "rmdir"            , chndlr_rmdir);

#if IS_DEBUG
    glob_add_cmd(gg, "show"             , chndlr_show);
    glob_add_cmd(gg, "wtype"            , chndlr_wtype);
#endif
#endif

}
/***************************************************************/
