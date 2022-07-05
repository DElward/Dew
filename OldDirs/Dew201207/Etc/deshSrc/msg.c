/***************************************************************/
/* msg.c                                                       */
/***************************************************************/
/*

****************************************************************
*/

#include "config.h"

#if IS_WINDOWS
#include <io.h>
#else
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <fcntl.h>
#include <errno.h>

#include "desh.h"
#include "snprfh.h"
#include "msg.h"

/***************************************************************/
void strmcpy(char * tgt, const char * src, size_t tmax) {
/*
** Copies src to tgt, trucating src if too long.
**  tgt always termintated with '\0'
*/

    size_t slen;

    slen = strlen(src) + 1;

    if (slen <= tmax) {
        memcpy(tgt, src, slen);
    } else if (tmax) {
        slen = tmax - 1;
        memcpy(tgt, src, slen);
        tgt[slen] = '\0';
    }
}
/***************************************************************/
void strmcat(char * tgt, char * src, size_t tmax) {

    size_t tlen;
    size_t slen;

    tlen = strlen(tgt);
    slen = strlen(src);

    if (tlen + slen < tmax) {
        memcpy(tgt + tlen, src, slen + 1);
    } else if (tlen < tmax) {
        slen = tmax - tlen;
        memcpy(tgt + tlen, src, slen - 1);
        tgt[tlen + slen] = '\0';
    }
}
/***************************************************************/
void get_errmsg(int inestat, const char * eparm, char ** msg) {

    char * errmsg;
    const char * errparm;

    (*msg) = NULL;
    errmsg = NULL;

    if (eparm) errparm = eparm;
    else       errparm = "";

    switch (inestat) {
        case EOPENCMD   :
            errmsg = Smprintf("Error opening command file %s", errparm);
            break;

        case EREADCMD   :
            errmsg = Smprintf("Error reading command file %s", errparm);
            break;

        case ETHROW   :
            errmsg = Smprintf("Throw \"%s\"", errparm);
            break;

        case EINVALFLAG :
            errmsg = Smprintf("Invalid flag: %s", errparm);
            break;

        case EBADOPT :
            errmsg = Smprintf("Invalid option: %s", errparm);
            break;

        case EBADVARNAM :
            errmsg = Smprintf("Variable name nust be alphanumeric or _.");
            break;

        case EUNEXPEOL :
            errmsg = Smprintf("Unexpected end of line after \'%s\'", errparm);
            break;

        case EBADREDIR :
            errmsg = Smprintf("Input/output redirect not permitted here");
            break;

        case EBADFOR :
            errmsg = Smprintf("Expected valid keyword in for.  Found \'%s\'", errparm);
            break;

        case EUNEXPCMD :
            errmsg = Smprintf("Command \'%s\' unexpected here.", errparm);
            break;

        case EMISSINGEND :
            errmsg = Smprintf("Missing end of construct");
            break;

        case EUNEXPEXPEOL :
            errmsg = Smprintf("Unexpected end of expression.");
            break;

        case EEXPEQ :
            errmsg = Smprintf("Expecting equal sign (=).  Found \'%s\'", errparm);
            break;

        case EBADNUM :
            errmsg = Smprintf("Invalid number.  Found \'%s\'", errparm);
            break;

        case EUNEXPTOK :
            errmsg = Smprintf("Expecting identifier or literal.  Found \'%s\'", errparm);
            break;

        case EBADID :
            errmsg = Smprintf("Unrecognized identifier.  Found \'%s\'", errparm);
            break;

        case EEXPNUM :
            errmsg = Smprintf("Expected valid number.  Found \'%s\'", errparm);
            break;

        case EEXPBOOL :
            errmsg = Smprintf("Expected valid boolean value.  Found \'%s\'", errparm);
            break;

        case EEXPEOL :
            errmsg = Smprintf("Expected end of line.  Found \'%s\'", errparm);
            break;

        case EEXPTO :
            errmsg = Smprintf("Expected \'to\' or \'downto\'.  Found \'%s\'", errparm);
            break;

        case EMISSINGCLOSE :
            errmsg = Smprintf("Missing closing parenthesis.");
            break;

        case EMANYPARMS :
            errmsg = Smprintf("Too many parameters supplied to function \'%s\'", errparm);
            break;

        case EFEWPARMS :
            errmsg = Smprintf("Too few parameters supplied to function \'%s\'", errparm);
            break;

        case EEXPCOMMA :
            errmsg = Smprintf("Expected comma or closing parenthesis.  Found \'%s\'", errparm);
            break;

        case EMISSINGBRACE :
            errmsg = Smprintf("Missing closing brace.");
            break;

        case EBADFUNCNAM :
            errmsg = Smprintf("Function name nust be alphanumeric or _.");
            break;

        case EBADFUNCHDR :
            errmsg = Smprintf("Bad function header.");
            break;

        case ENOFUNCHERE :
            errmsg = Smprintf("Function not permitted here.");
            break;

        case ENODUPFUNC :
            errmsg = Smprintf("Function \'%s\' already defined.", errparm);
            break;

        case EINVALTYP :
            errmsg = Smprintf("Invalid expression result.");
            break;

        case ENORETVAL :
            errmsg = Smprintf("Function must return a value to use in expression.");
            break;

        case ENOVAR :
            errmsg = Smprintf("Variable \'%s\' not set.", errparm);
            break;

        case EDUPREDIR :
            errmsg = Smprintf("Duplicate input/output redirection specified.");
            break;

        case ENOREDIR :
            errmsg = Smprintf("Input/output redirection not permitted here.");
            break;

        case EOPENREDIR   :
            errmsg = Smprintf("Error opening input/output redirection file %s", errparm);
            break;

        case EASSOC   :
            errmsg = Smprintf("File association not available: %s", errparm);
            break;

        case ENOCMD   :
            errmsg = Smprintf("Command does not exist: %s", errparm);
            break;

        case EFINDCMD   :
            errmsg = Smprintf("Cannot find command: %s", errparm);
            break;

        case EINVALCMD   :
            errmsg = Smprintf("Command is not valid: %s", errparm);
            break;

        case ENOPATHEXT   :
            errmsg = Smprintf("Cannot find PATHEXT variable.");
            break;

        case ENOPATH      :
            errmsg = Smprintf("Cannot find PATH variable.");
            break;

        case ERUN   :
            errmsg = Smprintf("Error running command: %s", errparm);
            break;

        case EWAIT   :
            errmsg = Smprintf("Error completing command: %s", errparm);
            break;

        case EFINDFILE   :
            errmsg = Smprintf("Unable to find file %s", errparm);
            break;

        case EEXPFILE :
            errmsg = Smprintf("Expected name file to read");
            break;

        case EOPENFILE   :
            errmsg = Smprintf("Error opening file %s", errparm);
            break;

        case EUNMATCHEDPAREN   :
            errmsg = Smprintf("Error unmatched parenthesis : \"%s\"", errparm);
            break;

        case EUNMATCHEDQUOTE   :
            errmsg = Smprintf("Error unmatched quote: (%s)", errparm);
            break;

        case EOPENTMPFIL   :
            errmsg = Smprintf("Error opening temp file %s", errparm);
            break;

        case EWRITETMPFIL   :
            errmsg = Smprintf("Error writing to temp file %s", errparm);
            break;

        case ERESETTMPFIL   :
            errmsg = Smprintf("Error on temp file reset %s", errparm);
            break;

        case EEXPPAREN :
            errmsg = Smprintf("Expected open parenthesis.  Found \'%s\'", errparm);
            break;

        case ETERMINAL :
            errmsg = Smprintf("Terminal error: %s", errparm);
            break;

        case EEXPCLPAREN :
            errmsg = Smprintf("Expected close parenthesis.  Found \'%s\'", errparm);
            break;

        case EOPENFORFILE :
            errmsg = Smprintf("Error opening for read file: %s", errparm);
            break;

        case EWRITESTDOUT :
            errmsg = Smprintf("Error writing to stdout: %s", errparm);
            break;

        case EWRITESTDERR :
            errmsg = Smprintf("Error writing to stderr: %s", errparm);
            break;

        case ESTDOUTPIPED :
            errmsg = Smprintf("Cannot redirect stdout when output is piped");
            break;

        case EOPENPIPE :
            errmsg = Smprintf("Error opening pipe: %s", errparm);
            break;

        case ESTDINPIPED :
            errmsg = Smprintf("Cannot redirect stdin when input is piped");
            break;

        case EPIPETOINT :
            errmsg = Smprintf("Cannot pipe to internal command: %s", errparm);
            break;

        case EPIPETOFUNC :
            errmsg = Smprintf("Cannot pipe to user function: %s", errparm);
            break;

        case EBADPARM   :
            errmsg = Smprintf("Parameter is not valid: %s", errparm);
            break;

        case ERMDIR :
            errmsg = Smprintf("Error removing directory %s", errparm);
            break;

        case EREMOVE :
            errmsg = Smprintf("Error removing file %s", errparm);
            break;

        case EEXPFORMCOMMA :
            errmsg = Smprintf("Expected comma or closing parenthesis after formal parameter.  Found \'%s\'", errparm);
            break;

        case EMAXSUBS :
            errmsg = Smprintf("Maximum inline substitution limit of %s reached.", errparm);
            break;

        case EMKDIR :
            errmsg = Smprintf("Error creating directory %s", errparm);
            break;

        case EDIR :
            errmsg = Smprintf("Error on directory %s", errparm);
            break;

        case EFILSET :
            errmsg = Smprintf("Bad fileset: %s", errparm);
            break;

        case EUNEXPCOPYEOL :
            errmsg = Smprintf("Unexpected end of line in COPY after \'%s\'", errparm);
            break;

        case ENOCOPYSRC :
            errmsg = Smprintf("Source of copy does not exist: %s", errparm);
            break;

        case ECOPYSRCDIR :
            errmsg = Smprintf("Source of copy is a directory: %s", errparm);
            break;

        case ECOPYTGTEXISTS :
            errmsg = Smprintf("Target of copy exists and y(Y) not specified: %s", errparm);
            break;

        case ECOPYERR :
            errmsg = Smprintf("Error copying file: %s", errparm);
            break;

        case EDIRREQ :
            errmsg = Smprintf("Target of copy must be a directory with more than one source: %s", errparm);
            break;

        case ESETATTR :
            errmsg = Smprintf("Error setting file attributes: %s", errparm);
            break;

        case EMUSTBEDIR :
            errmsg = Smprintf("removedir must be a directory: %s", errparm);
            break;

        case EREMOVEDIR :
            errmsg = Smprintf("Error during removedir: %s", errparm);
            break;

        case ECHMODBADOPT :
            errmsg = Smprintf("Invalid chmod parameter: %s", errparm);
            break;

        case ECHMODSTAT :
            errmsg = Smprintf("Error on stat for chmod: %s", errparm);
            break;

        case ECHMOD :
            errmsg = Smprintf("Error on chmod: %s", errparm);
            break;

        case ECHDIR :
            errmsg = Smprintf("Error on change directory: %s", errparm);
            break;

        case ERENAMEPARMS :
            errmsg = Smprintf("RENAME expects source and target only.  Found: \'%s\'", errparm);
            break;

        case ERENAME :
            errmsg = Smprintf("Error on rename: %s", errparm);
            break;

        case EDATEFMT :
            errmsg = Smprintf("Error on date format: %s", errparm);
            break;

        case EEXITPARM :
            errmsg = Smprintf("Expected number for exit value.  Found \'%s\'", errparm);
            break;

        case EADDTODATE :
            errmsg = Smprintf("Error adding to date: %s", errparm);
            break;

        case ESUBDATES :
            errmsg = Smprintf("Error subtracting dates: %s", errparm);
            break;

        case EPROGERR :
            errmsg = Smprintf("Program returned error: %s", errparm);
            break;

        case ECMDFAILED :
            errmsg = Smprintf("Command file %s had error.", errparm);
            break;

        case ENOTINFUNC :
            errmsg = Smprintf("Not in function");
            break;

        case EDUPPARMNAME :
            errmsg = Smprintf("Duplicate function parameter name: %s", errparm);
            break;

        case EOPENRCFILE :
            errmsg = Smprintf("Error opening rc file %s", errparm);
            break;

        case ERUNRCFILE :
            errmsg = Smprintf("Error running commands in rc file %s", errparm);
            break;

        case ERUNCTLC :
            errmsg = Smprintf("User pressed Control-C");
            break;

        case EOPTCONFLICT :
            errmsg = Smprintf("Option conflicts with prior option %s", errparm);
            break;

        case ESHIFTPARM :
            errmsg = Smprintf("Expected number parameters to shift.  Found \'%s\'", errparm);
            break;

        case EMISSINGPARM :
            errmsg = Smprintf("Function call missing %s required parameter(s)", errparm);
            break;

        case EUNDEFFUNC :
            errmsg = Smprintf("Undefined function name: %s", errparm);
            break;

        case EINITHELP :
            errmsg = Smprintf("Error initializing help: %s", errparm);
            break;

        case ENOHELP :
            errmsg = Smprintf("Help not available");
            break;

        case EFINDHELP :
            errmsg = Smprintf("Unable to find help topic: %s", errparm);
            break;

        case EWRITEHELP :
            errmsg = Smprintf("Error writing help: %s", errparm);
            break;

        case EDIVIDEBYZERO :
            errmsg = Smprintf("Attempt to divide by zero");
            break;

        case EBADSUBTOKEN :
            errmsg = Smprintf("Invalid token resulting from substitution.  Found \'%s\'\n"
                              "Try using quotes or removing the substitution.", errparm);
            break;

        case ENOFUNCSTRUCT :
            errmsg = Smprintf("Function definition not permitted in structure.");
            break;

        case ENORETURNHERE :
            errmsg = Smprintf("Return only permitted within function definition.");
            break;

        case EBADFINFOTYPE :
            errmsg = Smprintf("Invalid info type on FINFO.  Found \'%s\'", errparm);
            break;

        case EBADFINFOOWNER :
            errmsg = Smprintf("Error on FINFO OWNER %s", errparm);
            break;

        case EMISSINGENDLIST :
            errmsg = Smprintf("Missing end of construct in statement string");
            break;

        case EBADBASE :
            errmsg = Smprintf("CHBASE base must be 2,4,8,10 or 16.  Found \'%s\'", errparm);
            break;

        case EBADBASENUM :
            errmsg = Smprintf("Number is not valid in base %s", errparm);
            break;

        case EFILESTAT :
            errmsg = Smprintf("Error getting file information: %s", errparm);
            break;

        case EOPENFORDIR :
            errmsg = Smprintf("Error opening directory: %s", errparm);
            break;

        case EREADFORDIR :
            errmsg = Smprintf("Error reading directory: %s", errparm);
            break;

        case EBADCSVDELIM :
            errmsg = Smprintf("Invalid delimiter for csvparse(): %s", errparm);
            break;

        case ENOCSVDELIM :
            errmsg = Smprintf("At least one delimiter is required for csvparse()");
            break;

        case ENOFORCE :
            errmsg = Smprintf("%s already exists and -f specified.", errparm);
            break;

        case EEXPDIR :
            errmsg = Smprintf("%s already exists and is not a directory.", errparm);
            break;

        case ENEWHELP :
            errmsg = Smprintf("Error creating new help file: %s", errparm);
            break;

        case ENOSEARCH :
            errmsg = Smprintf("Search term is required for help -s");
            break;

        case EREPTOOBIG :
            errmsg = Smprintf("REPT result exceeds %s bytes.", errparm);
            break;

        case ETARGETEXISTS :
            errmsg = Smprintf("Target exists and y(Y) not specified: %s", errparm);
            break;

        case ERMTARGET :
            errmsg = Smprintf("Error removing target with y(Y) specified: %s", errparm);
            break;

        case ECPFILE :
            errmsg = Smprintf("Error copying file: %s", errparm);
            break;

        case EBADPID :
            errmsg = Smprintf("Expected valid pid.  Found \'%s\'", errparm);
            break;

        case EWAITERR   :
            errmsg = Smprintf("Error #%s on wait", errparm);
            break;

        case EWAITCMDFAIL :
            errmsg = Smprintf("One of the STARTed commands failed.");
            break;

        case ERMREADONLY :
            errmsg = Smprintf("Attempt to remove read-only file without specifying -f: %s", errparm);
            break;

        case ECMDARGFAILED :
            errmsg = Smprintf("Error on command argument: %s", errparm);
            break;

        default:
            if (inestat == ESTAT_EOF) {
                errmsg = Smprintf("End of file");
            } else if (inestat == ESTAT_EXIT) {
                errmsg = Smprintf("Program exit");
            } else if (inestat == ESTAT_CTL_C) {
                errmsg = Smprintf("Ctl-C");
            } else {
                errmsg = Smprintf("Unrecognized error number %d", inestat);
            }
            break;
    }

    (*msg) = errmsg;
}
/***************************************************************/
int set_error(struct cenv * ce, int inestat, const char * eparm) {

    struct errrec * er;
    char * ebuf;
    char mbuf[32];

    er = get_avail_errrec(ce->ce_gg);

    er->er_errnum       = inestat;

    get_errmsg(inestat, eparm, &ebuf);
	strmcpy(er->er_errmsg, ebuf, ER_ERRMSG_MAX);
    Free(ebuf);

    Snprintf(mbuf, sizeof(mbuf), " DESHERR %d", inestat);
    strmcat(er->er_errmsg, mbuf, ER_ERRMSG_MAX);

    return (er->er_index);
}
/***************************************************************/
int chain_error(struct cenv * ce, int inestat, int inestat2) {

    struct errrec * er;
    struct errrec * er2;

    if (inestat) {
        er  = get_errrec(ce->ce_gg, inestat);
        if (inestat2) {
            er2 = get_errrec(ce->ce_gg, inestat2);
        } else {
            er2 = NULL;
        }
    } else if (inestat2) {
        er2 = NULL;
        er  = get_errrec(ce->ce_gg, inestat2);
    } else {
        return (0);
    }

    if (er2) {
        er->er_next_errrec_index = er2->er_index;
    }

    return (er->er_index);
}
/***************************************************************/
int set_ferror(struct cenv * ce, int inestat, char *fmt, ...) {

    va_list args;
    int eix;
    char * emsg;

    va_start (args, fmt);
    emsg = Vsmprintf (fmt, args);
    va_end (args);

    eix = set_error(ce, inestat, emsg);
    Free(emsg);

    return (eix);
}
/***************************************************************/
/*
int is_eof_error (struct cenv * ce, int inestat) {

    int is_eof;

    is_eof = 0;
    if (inestat == ESTAT_EOF) is_eof = 1;

    return (is_eof);
}
*/
/***************************************************************/
void clear_error(struct cenv * ce, int inestat) {

    release_errrec(ce->ce_gg, inestat);
}
/***************************************************************/
void show_errmsg_stack(struct cenv * ce, const char * errcmd, int inestat) {

    struct errrec * er;

    er = get_errrec(ce->ce_gg, inestat);

    if (errcmd) {
        WRITE(ce->ce_stderr_f, errcmd, Istrlen(errcmd));
        WRITE(ce->ce_stderr_f, "\n", 1);
    }

    WRITE(ce->ce_stderr_f, er->er_errmsg, Istrlen(er->er_errmsg));
    WRITE(ce->ce_stderr_f, "\n", 1);

    if (er->er_next_errrec_index) {
        show_errmsg_stack(ce, NULL, er->er_next_errrec_index);
    }
}
/***************************************************************/
char * get_estat_errmsg(struct cenv * ce, int inestat) {

    struct errrec * er;
    char * errmsg;
    static char static_errmsg[ER_STATIC_ERRMSG_MAX];

    if (inestat == 0) {
        static_errmsg[0] = '\0';
        errmsg = static_errmsg;
    } else if (inestat < 0) {
        switch (inestat) {
            case ESTAT_EOF      : strcpy(static_errmsg, "End of file.");            break;
            case ESTAT_EXIT     : strcpy(static_errmsg, "Exit encountered.");       break;
            case ESTAT_CTL_C    : strcpy(static_errmsg, "Control-C interrupt.");    break;
            case ESTAT_EOL      : strcpy(static_errmsg, "End of line.");            break;
            case ESTAT_IDLEERR  : strcpy(static_errmsg, "Error while idle.");       break;
            default:
                sprintf(static_errmsg, "Unrecognized internal error: %d", inestat);
                break;
        }
        errmsg = static_errmsg;
    } else {
        er = get_errrec(ce->ce_gg, inestat);
        errmsg = er->er_errmsg;
    }

    return (errmsg);
}
/***************************************************************/
int get_estat_errnum(struct cenv * ce, int inestat) {

    struct errrec * er;
    int errnum;

    errnum = 0;

    if (inestat < 0) {
        errnum = inestat;
    } else if (inestat > 0) {
        er = get_errrec(ce->ce_gg, inestat);
        errnum = er->er_errnum;
    }

    return (errnum);
}
/***************************************************************/
int release_estat_error(struct cenv * ce, int inestat) {

    struct errrec * er;

    if (inestat > 0) {
        er = get_errrec(ce->ce_gg, inestat);

        clear_error(ce, inestat);
    }

    return (0);
}
/***************************************************************/
void show_errmsg(struct cenv * ce, const char * errcmd, int inestat) {

    show_errmsg_stack(ce, errcmd, inestat);

    clear_error(ce, inestat);
}
/***************************************************************/
void set_errmsg(struct cenv * ce, const char * errcmd, int inestat) {

    struct errrec * er;

    er = get_errrec(ce->ce_gg, inestat);
    if (ce->ce_errmsg) Free(ce->ce_errmsg);
    if (ce->ce_errcmd) Free(ce->ce_errcmd);

    if (errcmd) ce->ce_errcmd = Strdup(errcmd);
    else        ce->ce_errcmd = NULL;

    ce->ce_errmsg = Strdup(er->er_errmsg);
    ce->ce_errnum = er->er_errnum;
}
/***************************************************************/
void show_warn(struct cenv * ce, const char * fmt, ...) {

    va_list args;
    char ebuf[256];

    va_start(args, fmt);
    Vsnprintf(ebuf, sizeof(ebuf), fmt, args);
    va_end(args);
    WRITE(ce->ce_stderr_f, ebuf, Istrlen(ebuf));
    WRITE(ce->ce_stderr_f, "\n", 1);
}
/***************************************************************/
int printll(struct cenv * ce, const char * fmt, ...) {

    va_list args;
    int estat;
    int rtn;
    int ern;
    char * mbuf;

    estat = 0;

    va_start(args, fmt);
    mbuf = Vsmprintf(fmt, args);
    va_end(args);
    rtn = (Int)WRITE(ce->ce_stdout_f, mbuf, Istrlen(mbuf));
    if (rtn < 0) {
        ern = ERRNO;
        estat = set_ferror(ce, EWRITESTDOUT,"%m", ern);
    }
    Free(mbuf);

    return (estat);
}
/***************************************************************/
int printlle(struct cenv * ce, const char * fmt, ...) {

    va_list args;
    int estat;
    int rtn;
    int ern;
    char * mbuf;

    estat = 0;

    va_start(args, fmt);
    mbuf = Vsmprintf(fmt, args);
    va_end(args);
    rtn = (Int)WRITE(ce->ce_stderr_f, mbuf, Istrlen(mbuf));
    if (rtn < 0) {
        ern = ERRNO;
        estat = set_ferror(ce, EWRITESTDERR,"%m", ern);
    }
    Free(mbuf);

    return (estat);
}
/***************************************************************/
