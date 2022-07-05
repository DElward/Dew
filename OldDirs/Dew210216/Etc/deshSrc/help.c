/***************************************************************/
/* help.c                                                      */
/***************************************************************/
/*

****************************************************************
*/

#include "config.h"

#if IS_WINDOWS
#include <io.h>
#include <direct.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <fcntl.h>
#include <errno.h>

#include "desh.h"
#include "cmd.h"
#include "msg.h"
#include "help.h"
#include "term.h"
#include "cmd2.h"
#include "when.h"
#include "assoc.h"
#include "snprfh.h"

#if INTERNAL_HELP
    char ** get_help_lines(const char * help_item);
#endif

char * get_help_src(void);


#define MIN_COLS_FOR_TABLE  4

#define FF_PAGELEN_DEFAULT  23
#define HTM_WIDTH_PER_CHAR  9

/***************************************************************/
/*
** Windows prompt info:
**
**  Ceasar
**      PROMPT=$P$G
*
**     Prompt substitutions:
** 
**     $q   = (equal sign) 
**     $$   $ (dollar sign) 
**     $t   Current time 
**     $d   Current date 
**     $p   Current drive and path 
**     $v   Windows XP version number 
**     $n   Current drive 
**     $g   > (greater-than sign) 
**     $l   < (less-than sign) 
**     $b   | (pipe) 
**     $_   ENTER-LINEFEED 
**     $e   ANSI escape code (code 27) 
**     $h   Backspace (to delete a character that has been written to the prompt command line) 
**     $a   & (ampersand)
**     $c   ( (left parenthesis)
**     $f   ) (right parenthesis)
**     $s   space
**
**  Extended
**  $+      Zero or more plus sign (+) characters depending upon the depth of the pushd
**          directory stack, one character for each level pushed.
**  $m      The remote name associated with the current drive letter or the empty string
**          if current drive is not a network drive.
**
** --------
**
** Linux prompt info:
**     PS1 - The value of this parameter is expanded (see PROMPTING below) and
**           used as the primary prompt string. The default value is \s-\v\$ .
**     PS2 - The value of this parameter is expanded as with PS1 and used as the
**           secondary prompt string. The default is >
**     PS3 - The value of this parameter is used as the prompt for the select command
**     PS4 - The value of this parameter is expanded as with PS1 and the value is
**           printed before each command bash displays during an execution trace.
**           The first character of PS4 is replicated multiple times, as necessary,
**           to indicate multiple levels of indirection. The default is +
** 
**     Newton:
**         PS1='$(ppwd \l)\u@\h:\w> '
**         PS2='> '
**         PS4='+ '
** 
**     Prompt substitutions:
** 
**     \a : an ASCII bell character (07)
**     \d : the date in "Weekday Month Date" format (e.g., "Tue May 26")
**     \D{format} : the format is passed to strftime(3) and the result is inserted into
**                  the prompt string; an empty format results in a locale-specific time
**                  representation. The braces are required
**     \e : an ASCII escape character (033)
**     \h : the hostname up to the first '.'
**     \H : the hostname
**     \j : the number of jobs currently managed by the shell
**     \l : the basename of the shell�s terminal device name
**     \n : newline
**     \r : carriage return
**     \s : the name of the shell, the basename of $0 (the portion following the final slash)
**     \t : the current time in 24-hour HH:MM:SS format
**     \T : the current time in 12-hour HH:MM:SS format
**     \@ : the current time in 12-hour am/pm format
**     \A : the current time in 24-hour HH:MM format
**     \u : the username of the current user
**     \v : the version of bash (e.g., 2.00)
**     \V : the release of bash, version + patch level (e.g., 2.00.0)
**     \w : the current working directory, with $HOME abbreviated with a tilde
**     \W : the basename of the current working directory, with $HOME abbreviated with a tilde
**     \! : the history number of this command
**     \# : the command number of this command
**     \$ : if the effective UID is 0, a #, otherwise a $
**     \nnn : the character corresponding to the octal number nnn
**     \\ : a backslash
**     \[ : begin a sequence of non-printing characters, which could be used to embed a terminal
**          control sequence into the prompt
**     \] : end a sequence of non-printing characters
** 
****************************************************************
**
********************************
**  Ideas:
**
**      Unix
**      Windows
**      filesets
**      installation
*/

/***************************************************************/

struct helpfil {
    char *  hf_help_name;
    char *  hf_help_topic;
    char ** hf_line;
    int     hf_nlines;
    int     hf_mxlines;
};

struct foutfil {
    int     ff_outfil;
    int     ff_rmargin;
    int     ff_lindent; /* left indent */
    int     ff_hindent; /* hanging indent */
    int     ff_pagelen; /* page length */
    int     ff_page_count;
    int     ff_line_count;
    int     ff_need_sp;
    int     ff_table_cols;
    int     ff_cursor_ix;
    int     ff_outbuf_len;
    int     ff_outbuf_max;
    char *  ff_outbuf;
    int     ff_tab_max;
    char *  ff_tabs;
    char *  ff_help_title;
    char *  ff_search_term;
    struct term *   ff_tty;
};

static struct helpfil ** help_files     = NULL;
static int               help_nfiles    = 0;
static int               help_mxfiles   = 0;

/***************************************************************/
/***************************************************************/
static char * html_helpfil_header[] = {
    "<html>",
    "<head>",
    "<title>$H</title>",
    "    <meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\"/>",
    "    <meta name=\"keywords\" content=\"shell, unix, Windows\" />",
    "    <style type=\"text/css\">",
    "        .styheadline1",
    "        {",
    "            border-bottom: #e0e0e0 1px solid; ",
    "            text-align: center;",
    "            border-left: #e0e0e0 1px solid;",
    "            padding-bottom: 2px;",
    "            background-color: #fcfcc8;",
    "            padding-left: 5px;",
    "            padding-right: 5px;",
    "            font-family: verdana, arial, helvetica, sans-serif;",
    "            color: #990000;",
    "            margin-left: 5px;",
    "            font-size: 20px;",
    "            border-top: #e0e0e0 1px solid;",
    "            font-weight: bold;",
    "            margin-right: 5px;",
    "            border-right: #e0e0e0 1px solid;",
    "            padding-top: 2px",
    "        }",
    "        .stytable",
    "        {",
    "            height: 28;",
    "            table-layout:auto;",
    "            padding: 1px;",
    "            font-size: 12px;",
    "        }",
    "        .styRow",
    "        {",
    "        }",
    "        .styCell",
    "        {",
    "            vertical-align:text-top;",
    "        }",
    "        .bordermain {",
    "            border-bottom: #990000 1px solid;",
    "            border-left: #990000 1px solid;",
    "            border-top: #990000 1px solid;",
    "            border-right: #990000 1px solid",
    "        }",
    "        p {",
    "            font-family: verdana, arial, helvetica, sans-serif;",
    "            font-size: 12px;",
    "        }",
    "        .styHead",
    "        {",
    "            font-size: 16;",
    "            font-family: verdana, arial, helvetica, sans-serif;",
    "        }",
    "    </style>",
    "",
    "</head>",
    "<!-- ================================================================ -->",
    "<!-- == begin body                                                 == -->",
    "<!-- ================================================================ -->",
    "<body bgcolor=\"#fcfce0\" lang=\"EN-US\" link=\"blue\" vlink=\"blue\">",
    "   <div align=\"center\">",
    "       <table width=\"600\" border=\"0\" cellspacing=\"0\" cellpadding=\"2\" class=\"BorderMain\">",
    "           <tr>",
    "               <td>",
    "                   <table width=\"100%\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">",
    "                       <tr>",
    "                           <td>",
    "                               <div align=\"center\">",
    "                                   <p class=\"styHeadline1\">$T</p>",
    "                               </div>",
    "                    </td></tr></table>",
    "                    <table width=\"100%\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">",
    NULL
};

static char * html_helpfil_footer[] = {
    "                           </td>",
    "                       </tr>",
    "                   </table>",
    "               </td>",
    "           </tr>",
    "       </table>",
    "   </div>",
    "<!-- ================================================================ -->",
    "<hr/>",
    "<p style=\"font-size:10px\"><em><span>$V $C Generated $D</span></em></p>",
    "</body>",
    "</html>",
    NULL
};

/***************************************************************/
void get_now(struct cenv * ce, char * datbuf, int datbufsize) {

/*
*/
    int derrn;

    derrn = fmt_d_date(get_time_now(ce->ce_gg->gg_is_dst),
                    "%d-%b-%Y %H:%M:%S", datbuf, datbufsize);
    if (derrn) {
        datbuf[0] = '\0';
    }
}
/***************************************************************/
int get_help_toke_quote(
            const char * hbuf,
            int * pix,
            struct str * tokestr) {

    int eol;
    char quot;
    int stix;
    char ch;

    eol = 0;
    quot = hbuf[*pix];
    stix = (*pix);
    (*pix) += 1;

    while (quot) {
        ch = hbuf[*pix];
        if (!ch) {
            eol = 1;
            quot = 0;
        } else if (ch == quot) {
            (*pix) += 1;
            append_str_len(tokestr, hbuf + stix, (*pix) - stix);
            if (hbuf[*pix] == quot) {
                (*pix) += 1;
                stix = (*pix);
            } else {
                quot = 0;
            }
        } else {
            (*pix) += 1;
        }
    }

    return (eol);
}
/***************************************************************/
int get_help_toke(
            const char * hbuf,
            int * pix,
            struct str * tokestr) {

    int eol;
    char ch;
    int stix;

    eol = 0;

    tokestr->str_len = 0;

    while (isspace(hbuf[*pix])) (*pix)++;

    if (!hbuf[*pix]) {
        eol = 1;
    } else {
        ch = hbuf[*pix];
        if (ch == '_') {
            eol = get_help_toke_quote(hbuf, pix, tokestr);
        } else {
            stix = (*pix);
            (*pix) += 1;
            while (hbuf[*pix] && !isspace(hbuf[*pix])) (*pix) += 1;

            append_str_len(tokestr, hbuf + stix, (*pix) - stix);
        }
    }
    APPCH(tokestr, '\0');

    return (eol);
}
/***************************************************************/
struct helpfil * new_helpfil(const char * help_name) {

    struct helpfil * hf;

    hf = New(struct helpfil, 1);

    hf->hf_help_name    = Strdup(help_name);
    hf->hf_help_topic   = NULL;
    hf->hf_line      = NULL;
    hf->hf_nlines    = 0;
    hf->hf_mxlines   = 0;

    if (help_nfiles == help_mxfiles) {
        help_mxfiles += 32;

        help_files = Realloc(help_files, struct helpfil*, help_mxfiles);
    }

    help_files[help_nfiles]= hf;
    help_nfiles += 1;

    return (hf);
}
/***************************************************************/
#if INTERNAL_HELP
int add_help(struct cenv * ce, const char * help_name, const char * helpfil_name) {

    int estat;
    int ii;
    int llen;
    char ** chelp_lines;
    const char *  hlin;
    struct helpfil * hf;

    estat = 0;
    chelp_lines = get_help_lines(help_name);
    if (!chelp_lines) {
        estat = set_ferror(ce, EINITHELP, "Unable to find help for %s",
            helpfil_name);
        return (estat);
    }

    hf = new_helpfil(help_name);

    ii = 0;
    while (!estat && chelp_lines[ii]) {
        hlin = chelp_lines[ii];

        if (hf->hf_nlines == hf->hf_mxlines) {
            hf->hf_mxlines += 32;

            hf->hf_line = Realloc(hf->hf_line, char*, hf->hf_mxlines);
        }

        llen = Istrlen(hlin);
        hf->hf_line[hf->hf_nlines] = New(char, llen + 1);
        memcpy(hf->hf_line[hf->hf_nlines], hlin, llen + 1);
        hf->hf_nlines += 1;
        ii++;
    }

    if (hf->hf_nlines == hf->hf_mxlines) {
        hf->hf_mxlines += 1;

        hf->hf_line = Realloc(hf->hf_line, char*, hf->hf_mxlines);
    }
    hf->hf_line[hf->hf_nlines] = NULL;

    return (estat);
}
#else
/********************************/
char * get_help_src(void) { return ("Files"); }
/********************************/
int add_help(struct cenv * ce, const char * help_name, const char * helpfil_name) {

    int estat;
    int ern;
    int done;
    char * dd;
    FILE * hfil;
    struct helpfil * hf;
    int llen;
    char hfil_nam[256];
    char hlin[HELP_LINE_SIZE];

    if (strlen(helpfil_name) >= sizeof(hfil_nam)) {
        estat = set_ferror(ce, EINITHELP, "Help file name too long: %s",
                           helpfil_name);
    }
    strcpy(hfil_nam, helpfil_name);
    if (DIR_DELIM != '\\') {
        do {
            dd = strchr(hfil_nam, '\\');
            if (dd) {
                (*dd) = DIR_DELIM;
                dd++;
            }
        } while (dd);
    }

    estat = 0;

    hfil = fopen(hfil_nam, "r");
    if (!hfil) {
        ern = ERRNO;
        estat = set_ferror(ce, EINITHELP, "Error opening help file %s: %m",
            hfil_nam, ern);
        return (estat);
    }

    hf = new_helpfil(help_name);
    hlin[sizeof(hlin) - 1] = '\0';

    done = 0;
    while (!estat && !done) {
        if (!fgets(hlin, sizeof(hlin) - 1, hfil)) {
            if (!ferror(hfil)) {
                done = 1;
            } else {
                ern = ERRNO;
                estat = set_ferror(ce, EINITHELP, "Error opening help file %s: %m",
                    helpfil_name, ern);
                return (estat);
            }
        } else {
            if (hf->hf_nlines == hf->hf_mxlines) {
                hf->hf_mxlines += 32;

                hf->hf_line = Realloc(hf->hf_line, char*, hf->hf_mxlines);
            }

            llen = Istrlen(hlin);
            while (llen > 0 && isspace(hlin[llen - 1])) llen--;
            hlin[llen] = '\0';
            hf->hf_line[hf->hf_nlines] = New(char, llen + 1);
            memcpy(hf->hf_line[hf->hf_nlines], hlin, llen + 1);
            hf->hf_nlines += 1;
        }
    }

    fclose(hfil);

    if (hf->hf_nlines == hf->hf_mxlines) {
        hf->hf_mxlines += 32;

        hf->hf_line = Realloc(hf->hf_line, char*, hf->hf_mxlines);
    }
    hf->hf_line[hf->hf_nlines] = NULL;

    return (estat);
}
#endif
/***************************************************************/
int init_help(struct cenv * ce) {

    int estat;

    estat = 0;
    if (help_nfiles) return (0);

#ifdef HELP_TESTING
    //if (!estat) estat = add_help(ce, "helptest"      , "help\\helptest.txt");
#endif

    /* index first */
    if (!estat) estat = add_help(ce, "index"         , "help\\index.txt");

    if (!estat) estat = add_help(ce, "commands"      , "help\\commands.txt");

    if (!estat) estat = add_help(ce, "errors"        , "help\\errors.txt");
    if (!estat) estat = add_help(ce, "datefmts"      , "help\\datefmts.txt");
    if (!estat) estat = add_help(ce, "desh"          , "help\\desh.txt");
    if (!estat) estat = add_help(ce, "expfunctions"  , "help\\expfunctions.txt");
    if (!estat) estat = add_help(ce, "expressions"   , "help\\expressions.txt");
    if (!estat) estat = add_help(ce, "intro"         , "help\\intro.txt");
    if (!estat) estat = add_help(ce, "keyboard"      , "help\\keyboard.txt");
    if (!estat) estat = add_help(ce, "parsing"       , "help\\parsing.txt");
    if (!estat) estat = add_help(ce, "redirection"   , "help\\redirection.txt");
    if (!estat) estat = add_help(ce, "scripting"     , "help\\scripting.txt");
    if (!estat) estat = add_help(ce, "shells"        , "help\\shells.txt");
    if (!estat) estat = add_help(ce, "substitution"  , "help\\substitution.txt");
    if (!estat) estat = add_help(ce, "userfunctions" , "help\\userfunctions.txt");
    if (!estat) estat = add_help(ce, "variables"     , "help\\variables.txt");

    if (!estat) estat = add_help(ce, "call"          , "help\\call.txt");
    if (!estat) estat = add_help(ce, "catch"         , "help\\catch.txt");
    if (!estat) estat = add_help(ce, "cd"            , "help\\cd.txt");
    if (!estat) estat = add_help(ce, "chmod"         , "help\\chmod.txt");
    if (!estat) estat = add_help(ce, "copy"          , "help\\copy.txt");
    if (!estat) estat = add_help(ce, "dir"           , "help\\dir.txt");
    if (!estat) estat = add_help(ce, "dot"           , "help\\dot.txt");
    if (!estat) estat = add_help(ce, "echo"          , "help\\echo.txt");
    if (!estat) estat = add_help(ce, "else"          , "help\\else.txt");
    if (!estat) estat = add_help(ce, "endfor"        , "help\\endfor.txt");
    if (!estat) estat = add_help(ce, "endfunction"   , "help\\endfunction.txt");
    if (!estat) estat = add_help(ce, "endif"         , "help\\endif.txt");
    if (!estat) estat = add_help(ce, "endtry"        , "help\\endtry.txt");
    if (!estat) estat = add_help(ce, "endwhile"      , "help\\endwhile.txt");
    if (!estat) estat = add_help(ce, "exit"          , "help\\exit.txt");
    if (!estat) estat = add_help(ce, "export"        , "help\\export.txt");
    if (!estat) estat = add_help(ce, "for"           , "help\\for.txt");
    if (!estat) estat = add_help(ce, "function"      , "help\\function.txt");
    if (!estat) estat = add_help(ce, "help"          , "help\\help.txt");
    if (!estat) estat = add_help(ce, "if"            , "help\\if.txt");
    if (!estat) estat = add_help(ce, "mkdir"         , "help\\mkdir.txt");
    if (!estat) estat = add_help(ce, "move"          , "help\\move.txt");
    if (!estat) estat = add_help(ce, "remove"        , "help\\remove.txt");
    if (!estat) estat = add_help(ce, "removedir"     , "help\\removedir.txt");
    if (!estat) estat = add_help(ce, "return"        , "help\\return.txt");
    if (!estat) estat = add_help(ce, "rmdir"         , "help\\rmdir.txt");
    if (!estat) estat = add_help(ce, "set"           , "help\\set.txt");
    if (!estat) estat = add_help(ce, "shift"         , "help\\shift.txt");
    if (!estat) estat = add_help(ce, "shopt"         , "help\\shopt.txt");
    if (!estat) estat = add_help(ce, "start"         , "help\\start.txt");
    if (!estat) estat = add_help(ce, "throw"         , "help\\throw.txt");
    if (!estat) estat = add_help(ce, "time"          , "help\\time.txt");
    if (!estat) estat = add_help(ce, "try"           , "help\\try.txt");
    if (!estat) estat = add_help(ce, "type"          , "help\\type.txt");
    if (!estat) estat = add_help(ce, "wait"          , "help\\wait.txt");
    if (!estat) estat = add_help(ce, "while"         , "help\\while.txt");

    /* built-in expfunctions */
    if (!estat) estat = add_help(ce, "addtodate"     , "help\\addtodate.txt");
    if (!estat) estat = add_help(ce, "argv"          , "help\\argv.txt");
    if (!estat) estat = add_help(ce, "basename"      , "help\\basename.txt");
    if (!estat) estat = add_help(ce, "char"          , "help\\char.txt");
    if (!estat) estat = add_help(ce, "chbase"        , "help\\chbase.txt");
    if (!estat) estat = add_help(ce, "code"          , "help\\code.txt");
    if (!estat) estat = add_help(ce, "dirname"       , "help\\dirname.txt");
    if (!estat) estat = add_help(ce, "extname"       , "help\\extname.txt");
    if (!estat) estat = add_help(ce, "find"          , "help\\find.txt");
    if (!estat) estat = add_help(ce, "finfo"         , "help\\finfo.txt");
    if (!estat) estat = add_help(ce, "left"          , "help\\left.txt");
    if (!estat) estat = add_help(ce, "len"           , "help\\len.txt");
    if (!estat) estat = add_help(ce, "lower"         , "help\\lower.txt");
    if (!estat) estat = add_help(ce, "matches"       , "help\\matches.txt");
    if (!estat) estat = add_help(ce, "mid"           , "help\\mid.txt");
    if (!estat) estat = add_help(ce, "now"           , "help\\now.txt");
    if (!estat) estat = add_help(ce, "prompt"        , "help\\prompt.txt");
    if (!estat) estat = add_help(ce, "rept"          , "help\\rept.txt");
    if (!estat) estat = add_help(ce, "right"         , "help\\right.txt");
    if (!estat) estat = add_help(ce, "subdates"      , "help\\subdates.txt");
    if (!estat) estat = add_help(ce, "subfromdate"   , "help\\subfromdate.txt");
    if (!estat) estat = add_help(ce, "trim"          , "help\\trim.txt");
    if (!estat) estat = add_help(ce, "upper"         , "help\\upper.txt");

    return (estat);
}
/***************************************************************/
void free_helpfil(struct helpfil * hf) {

    int ii;

    for (ii = 0; ii < hf->hf_nlines; ii++) {
        Free(hf->hf_line[ii]);
    }
    Free(hf->hf_line);

    Free(hf->hf_help_topic);
    Free(hf->hf_help_name);
    Free(hf);
}
/***************************************************************/
void free_help(void) {

    int ii;

    for (ii = 0; ii < help_nfiles; ii++) {
        free_helpfil(help_files[ii]);
    }
    Free(help_files);
}
/***************************************************************/
void foutfil_reset(struct foutfil * ff) {

    ff->ff_lindent      = 0;
    ff->ff_hindent      = 0;
    ff->ff_need_sp      = 0;
    ff->ff_table_cols   = 0;
    ff->ff_cursor_ix    = 0;

    memset(ff->ff_tabs, 0, ff->ff_tab_max);
}
/***************************************************************/
struct foutfil * new_foutfil(int outfil) {

    struct foutfil * ff;

    ff = New(struct foutfil, 1);

    ff->ff_outfil       = outfil;
    ff->ff_rmargin      = FOUTFIL_DEFAULT_RMARGIN;
    ff->ff_outbuf_len   = 0;
    ff->ff_outbuf_max   = 0;
    ff->ff_outbuf       = NULL;
    ff->ff_tab_max      = FOUTFIL_INIT_TAB_MAX;
    ff->ff_pagelen      = 0;
    ff->ff_page_count   = 0;
    ff->ff_line_count   = 0;
    ff->ff_tty          = NULL;
    ff->ff_help_title   = NULL;
    ff->ff_search_term  = NULL;

    ff->ff_tabs         = New(char, ff->ff_tab_max);

    foutfil_reset(ff);

    return (ff);
}
/***************************************************************/
void free_foutfil(struct foutfil * ff, int close_outfil) {

    if (close_outfil) {
        CLOSE(ff->ff_outfil);
    }
    if (ff->ff_tty) free_term(ff->ff_tty);
    Free(ff->ff_outbuf);
    Free(ff->ff_tabs);
    Free(ff->ff_help_title);
    Free(ff->ff_search_term);

    Free(ff);
}
/***************************************************************/
void foutfil_insert(struct foutfil * ff, int inix, const char * outbuf, int nout) {

    if (ff->ff_outbuf_len + nout >= ff->ff_outbuf_max) {
        ff->ff_outbuf_max = ff->ff_outbuf_len + nout + 64;
        ff->ff_outbuf = Realloc(ff->ff_outbuf, char, ff->ff_outbuf_max);
    }

    memmove(ff->ff_outbuf + inix + nout, ff->ff_outbuf + inix, ff->ff_outbuf_len - inix);

    memcpy(ff->ff_outbuf + inix, outbuf, nout);
    ff->ff_outbuf_len += nout;
}
/***************************************************************/
void foutfil_put_null(struct foutfil * ff) {

    if (ff->ff_outbuf_len + 1 >= ff->ff_outbuf_max) {
        ff->ff_outbuf_max = ff->ff_outbuf_len + 64;
        ff->ff_outbuf = Realloc(ff->ff_outbuf, char, ff->ff_outbuf_max);
    }

    ff->ff_outbuf[ff->ff_outbuf_len] = '\0';
}
/***************************************************************/
void foutfil_put(struct foutfil * ff, const char * outbuf, int nout) {

    if (ff->ff_outbuf_len + nout >= ff->ff_outbuf_max) {
        ff->ff_outbuf_max = ff->ff_outbuf_len + nout + 64;
        ff->ff_outbuf = Realloc(ff->ff_outbuf, char, ff->ff_outbuf_max);
    }

    memcpy(ff->ff_outbuf + ff->ff_outbuf_len, outbuf, nout);
    ff->ff_outbuf_len += nout;
}
/***************************************************************/
void foutfil_put_spaces(struct foutfil * ff, int nsp) {

    if (ff->ff_outbuf_len + nsp >= ff->ff_outbuf_max) {
        ff->ff_outbuf_max = ff->ff_outbuf_len + nsp + 64;
        ff->ff_outbuf = Realloc(ff->ff_outbuf, char, ff->ff_outbuf_max);
    }
    memset(ff->ff_outbuf + ff->ff_outbuf_len, ' ', nsp);
    ff->ff_outbuf_len += nsp;
    ff->ff_cursor_ix += nsp;
}
/***************************************************************/
int read_help_continue(struct cenv * ce,
                struct foutfil * ff,
                int help_flags) {

    int estat;
    int rtn;
    int inlen;
    char * inbuf;

    estat = 0;

    rtn = ff->ff_tty->termReadLine(ff->ff_tty, "Continue?", &inbuf, &inlen);
    if (!rtn) {
        if (inlen && (!Stricmp(inbuf, "N") || !Stricmp(inbuf, "NO"))) {
            estat = ESTAT_EOF;
        }
    } else if (rtn == TERMREAD_EOF) {
        estat = ESTAT_EOF;
    } else if (rtn == TERMREAD_INTR) {
        /* control_c(SIGINT); */
        estat = ESTAT_CTL_C;
    } else if (rtn < 0) {
        estat = set_error(ce, ETERMINAL, term_errmsg(ff->ff_tty, rtn));
    }

    return (estat);
}
/***************************************************************/
int write_help_line(struct cenv * ce,
                struct foutfil * ff,
                int help_flags) {

    int estat;
    int rtn;

    estat = 0;

    if (ff->ff_tty && (help_flags & HELPFLAG_PAGE) &&
        ff->ff_pagelen && ff->ff_page_count == ff->ff_pagelen) {
        estat = read_help_continue(ce, ff, help_flags);
        if (estat) return (estat);
        ff->ff_page_count = 0;
    }

    foutfil_put(ff, "\n", 1);
    rtn = (Int)WRITE(ff->ff_outfil, ff->ff_outbuf, ff->ff_outbuf_len);
    if (rtn < 0) {
        int ern = ERRNO;
        estat = set_ferror(ce, EWRITEHELP, "%m", ern);
        return (estat);
    }
    ff->ff_page_count   += 1;

    return (estat);
}
/***************************************************************/
int search_string(  const char * search_in,
                    const char * search_for,
                    int nocase) {

    int found;
    int forix;
    int inix;
    int eq;
    int len_for;
    int last_inix;

    found = 0;
    forix = 0;
    inix  = 0;

    if (nocase) {
        len_for = Istrlen(search_for);
        last_inix = Istrlen(search_in) - len_for;
        inix = 0;
        while (!found && inix <= last_inix) {
            forix = 0;
            eq   = 1;
            while (eq && forix < len_for) {
                if (toupper(search_for[forix]) == toupper(search_in[inix + forix])) forix++;
                else eq = 0;
            }
            if (eq) found = 1;
            else    inix++;
        }
    } else {
        char * where = strstr(search_in, search_for);
        if (where) {
            found = 1;
            inix = (Int)(where - search_in);
        }
    }

    if (!found) return (-1);

    return (inix);
}
/***************************************************************/
int write_search_help_line(struct cenv * ce,
                struct foutfil * ff,
                int help_flags) {

    int estat;
    int inix;
    int ovfllen;
    char nbuf[16];
    char ovfl[HELP_LINE_OUT_LEN + 2];

    estat = 0;

    foutfil_put_null(ff);
    if (search_string(ff->ff_outbuf, ff->ff_search_term, 1) >= 0) {

        inix = Istrlen(ff->ff_help_title);
        foutfil_insert(ff, 0, ff->ff_help_title, inix);

        sprintf(nbuf, " %d:", ff->ff_line_count);
        foutfil_insert(ff, inix, nbuf, Istrlen(nbuf));
        inix += Istrlen(nbuf);

        ovfllen = ff->ff_outbuf_len - (HELP_LINE_OUT_LEN - 1);
        if (ovfllen <= 0) {
            estat = write_help_line(ce, ff, help_flags);
        } else {
            if (ovfllen > HELP_LINE_OUT_LEN) {
                ovfllen = HELP_LINE_OUT_LEN;
            }
            memcpy(ovfl, ff->ff_outbuf + (HELP_LINE_OUT_LEN - 1), ovfllen);
            ovfl[ovfllen] = '\0';
            ff->ff_outbuf_len = (HELP_LINE_OUT_LEN - 1);

            estat = write_help_line(ce, ff, help_flags);
            if (!estat) {
                ff->ff_outbuf_len   = 0;

                foutfil_put(ff, ovfl, ovfllen);

                inix = Istrlen(ff->ff_help_title);
                foutfil_insert(ff, 0, ff->ff_help_title, inix);

                sprintf(nbuf, " %d:", ff->ff_line_count);
                foutfil_insert(ff, inix, nbuf, Istrlen(nbuf));
                inix += Istrlen(nbuf);

                memset(ff->ff_outbuf, ' ', inix);
                estat = write_help_line(ce, ff, help_flags);
            }
        }
    }

    return (estat);
}
/***************************************************************/
int render_new_line(struct cenv * ce,
                struct foutfil * ff,
                int help_flags) {

    int estat;

    ff->ff_line_count   += 1;

    if (help_flags & HELPFLAG_SEARCH) {
        estat = write_search_help_line(ce, ff, help_flags);
    } else {
        estat = write_help_line(ce, ff, help_flags);
    }

    ff->ff_cursor_ix    = 0;
    ff->ff_outbuf_len   = 0;

    return (estat);
}
/***************************************************************/
/*
int render_tab(struct cenv * ce,
                struct foutfil * ff,
                int help_flags) {

    int estat;

    estat = 0;

    return (estat);
}
*/
/***************************************************************/
/*
int render_lmargin(struct cenv * ce,
                struct foutfil * ff,
                int help_flags) {

    int estat;

    estat = 0;

    return (estat);
}
*/
/***************************************************************/
void ff_set_tab(struct foutfil * ff, int tab_stop) {

    int new_tab_max;

    if (tab_stop >= ff->ff_tab_max) {
        new_tab_max = tab_stop + 64;
        ff->ff_tabs = Realloc(ff->ff_tabs, char, new_tab_max);
        while (ff->ff_tab_max < new_tab_max) {
            ff->ff_tabs[ff->ff_tab_max] = 0;
            ff->ff_tab_max += 1;
        }
    }
    ff->ff_tabs[tab_stop] = 1;
}
/***************************************************************/
int ff_get_next_tab_stop(struct foutfil * ff, int from_col) {

    int tix;

    tix = from_col;
    while (tix < ff->ff_tab_max && !ff->ff_tabs[tix]) tix++;

    if (tix == ff->ff_tab_max) tix = -1;

    return (tix);
}
/***************************************************************/
int cook_token(struct cenv * ce,
                struct str * rawtoke,
                struct str * cookedtoke,
                int * ncookedchars,
                int help_flags) {

    int estat;

    estat = 0;

    if (rawtoke->str_len >= cookedtoke->str_max) {
        cookedtoke->str_max = rawtoke->str_max + 8;
        cookedtoke->str_buf = Realloc(cookedtoke->str_buf, char, cookedtoke->str_max);
    }

    if (rawtoke->str_len > 1 &&
        rawtoke->str_buf[0] == '_' &&
        rawtoke->str_buf[rawtoke->str_len - 1] == '_') {
        memcpy(cookedtoke->str_buf, rawtoke->str_buf + 1, rawtoke->str_len - 2);
        cookedtoke->str_len = rawtoke->str_len - 2;
    } else {
        memcpy(cookedtoke->str_buf, rawtoke->str_buf, rawtoke->str_len);
        cookedtoke->str_len = rawtoke->str_len;
    }
    cookedtoke->str_buf[cookedtoke->str_len] = '\0';
    (*ncookedchars) = cookedtoke->str_len;

    return (estat);
}
/***************************************************************/
int render_lindent(struct cenv * ce,
                struct foutfil * ff,
                int help_flags) {

    int estat;

    estat = 0;

    if (ff->ff_hindent) {
        foutfil_put_spaces(ff, ff->ff_hindent);
    } else if (ff->ff_lindent) {
        foutfil_put_spaces(ff, ff->ff_lindent);
    }

    return (estat);
}
/***************************************************************/
int render_return(struct cenv * ce,
                struct foutfil * ff,
                int help_flags) {

    int estat;

    estat = render_new_line(ce, ff, help_flags);
    if (!estat) estat = render_lindent(ce, ff, help_flags);

    return (estat);
}
/***************************************************************/
int render_token(struct cenv * ce,
                struct foutfil * ff,
                struct str * rawtoke,
                struct str * cookedtoke,
                int help_flags) {

    int estat;
    int tix;
    int nsp;
    int ncookedchars;

    estat = cook_token(ce, rawtoke, cookedtoke, &ncookedchars, help_flags);
    if (estat) return (estat);

    if (ff->ff_need_sp) {
        tix = ff_get_next_tab_stop(ff, ff->ff_cursor_ix + 1);

        if (tix < 0) nsp = 1;
        else         nsp = tix - ff->ff_cursor_ix;

        if (ff->ff_cursor_ix + nsp + cookedtoke->str_len >= ff->ff_rmargin) {
            estat = render_return(ce, ff, help_flags);
        } else {
            foutfil_put_spaces(ff, nsp);
        }
    }

    foutfil_put(ff, cookedtoke->str_buf, cookedtoke->str_len);
    ff->ff_cursor_ix += cookedtoke->str_len;

    ff->ff_need_sp = 1;

    return (estat);
}
/***************************************************************/
int render_help(struct cenv * ce,
                struct helpfil * hf,
                struct foutfil * ff,
                int help_flags) {

    int estat;
    int ix;
    int tix;
    int lnum;
    int eol;
    int nsp;
    struct str rawtoke;
    struct str cookedtoke;

    estat = 0;
    lnum = 0;
    ix = 0;

    init_str(&rawtoke, INIT_STR_CHUNK_SIZE);
    init_str(&cookedtoke, INIT_STR_CHUNK_SIZE);

    while (!estat && lnum < hf->hf_nlines) {
        nsp = 0;
        while (isspace(hf->hf_line[lnum][ix])) { ix++; nsp++; }

        if (!hf->hf_line[lnum][ix]) {
            lnum++;
            ix = 0;
            if (lnum < hf->hf_nlines) {
                if (!hf->hf_line[lnum][0]) {
                    if (ff->ff_need_sp) estat = render_new_line(ce, ff, help_flags);
                    if (!estat)         estat = render_new_line(ce, ff, help_flags);
                    foutfil_reset(ff);
                    lnum++;
                } else if (isspace(hf->hf_line[lnum][0])) {
                    if (ff->ff_need_sp) estat = render_new_line(ce, ff, help_flags);
                    nsp = 0;
                    while (isspace(hf->hf_line[lnum][ix])) { ix++; nsp++; }
                    foutfil_reset(ff);
                    ff->ff_lindent = nsp;
                    estat = render_lindent(ce, ff, help_flags);
                } else if (ff->ff_table_cols) {
                    if (ff->ff_need_sp) {
                        tix = ff_get_next_tab_stop(ff, ff->ff_cursor_ix + 1);
                        if (tix < 0) estat = render_return(ce, ff, help_flags);
                    }
                }
            }
        } else {
            if (nsp >= HELP_SPACES_FOR_TAB) {
                if (ff->ff_lindent || ff->ff_table_cols) {
                    if (!ff->ff_table_cols && !ff->ff_hindent) {
                        ff->ff_hindent = ff->ff_cursor_ix + nsp;
                        foutfil_put_spaces(ff,nsp);
                        ff->ff_need_sp = 0;
                    } else {
                        if (ff->ff_lindent) {
                            ff_set_tab(ff, ff->ff_lindent);
                            ff->ff_table_cols += 1;
                            if (ff->ff_hindent > ff->ff_lindent) {
                                ff_set_tab(ff, ff->ff_hindent);
                                ff->ff_table_cols += 1;
                            }
                            ff->ff_lindent = 0;
                            ff->ff_hindent = 0;
                        }
                        ff_set_tab(ff, ff->ff_cursor_ix + nsp);
                        ff->ff_need_sp    = 1;
                        ff->ff_table_cols += 1;
                    }
                } else {
                    ff->ff_lindent = ff->ff_cursor_ix + nsp;
                    foutfil_put_spaces(ff,nsp);
                    ff->ff_need_sp = 0;
                }
            }
            eol = get_help_toke(hf->hf_line[lnum], &ix, &rawtoke);
            if (!estat && !eol) {
                estat = render_token(ce, ff, &rawtoke, &cookedtoke, help_flags);
            }
        }
    }
    free_str_buf(&rawtoke);
    free_str_buf(&cookedtoke);

    if (!estat && ff->ff_need_sp) {
        estat = render_new_line(ce, ff, help_flags);
    }

    return (estat);
}
/***************************************************************/
#define HFF_HTM_STACK_SZ    16

#define HFF_HTM_STACK_TABLE 1
#define HFF_HTM_STACK_TR    2
#define HFF_HTM_STACK_TD    3
#define HFF_HTM_STACK_P     4

#define HFF_TABLE_STATE_TABLE 1
#define HFF_TABLE_STATE_TR    2
#define HFF_TABLE_STATE_TD    3

struct hfoutfil {
    FILE *  hff_houtfref;
    int     hff_table_cols;
    int     hff_current_col;
    int     hff_outbuf_len;
    int     hff_outbuf_max;
    int     hff_htm_stack_tos;
    int     hff_htm_stack[HFF_HTM_STACK_SZ];
    char *  hff_outbuf;
    int     hff_ntabs;
    int     hff_tab_max_cols;
    int     hff_table_state;
    char *  hff_col_tabs;
    struct term *   hff_tty;
};
/***************************************************************/
void push_htm_stack(struct hfoutfil * hff, int sitm) {

    if (hff->hff_htm_stack_tos == HFF_HTM_STACK_SZ) {
        printf("Internal error: Htm stack overflow.\n");
    } else {
        hff->hff_htm_stack[hff->hff_htm_stack_tos] = sitm;
        hff->hff_htm_stack_tos += 1;
    }
}
/***************************************************************/
int pop_htm_stack(struct hfoutfil * hff) {

    int sitm;

    if (!hff->hff_htm_stack_tos) {
        printf("Internal error: Htm stack underflow.\n");
        sitm = 0;
    } else {
        hff->hff_htm_stack_tos -= 1;
        sitm = hff->hff_htm_stack[hff->hff_htm_stack_tos];
    }

    return (sitm);
}
/***************************************************************/
int peek_htm_stack(struct hfoutfil * hff) {

    int sitm;

    if (!hff->hff_htm_stack_tos) {
        sitm = 0;
    } else {
        sitm = hff->hff_htm_stack[hff->hff_htm_stack_tos - 1];
    }

    return (sitm);
}
/***************************************************************/
void hfoutfil_reset(struct hfoutfil * hff) {

    hff->hff_table_cols   = 0;
    hff->hff_current_col  = 0;
    hff->hff_ntabs        = 0;
    hff->hff_table_state  = 0;

    memset(hff->hff_col_tabs, 0, hff->hff_tab_max_cols);
}
/***************************************************************/
struct hfoutfil * new_hfoutfil(FILE * nhfref) {

    struct hfoutfil * hff;

    hff = New(struct hfoutfil, 1);

    hff->hff_houtfref       = nhfref;
    hff->hff_outbuf_len     = 0;
    hff->hff_outbuf_max     = 0;
    hff->hff_htm_stack_tos  = 0;
    hff->hff_outbuf         = NULL;
    hff->hff_tab_max_cols   = FOUTFIL_INIT_TAB_MAX;
    hff->hff_ntabs          = 0;

    hff->hff_col_tabs       = New(char, hff->hff_tab_max_cols);

    hfoutfil_reset(hff);

    return (hff);
}
/***************************************************************/
void free_hfoutfil(struct hfoutfil * hff) {

    fclose(hff->hff_houtfref);

    if (hff->hff_tty) free_term(hff->hff_tty);
    Free(hff->hff_outbuf);
    Free(hff->hff_col_tabs);

    Free(hff);
}
/***************************************************************/
struct helpfil * find_help(struct cenv * ce,
                             const char * help_parm,
                             int exact_match,
                             int help_flags) {

    struct helpfil * hf;
    int ii;
    int hplen;

    hf = NULL;
    hplen = Istrlen(help_parm);
    if (!hplen) return (NULL);

    for (ii = 0; !hf && ii < help_nfiles; ii++) {
        if (!Stricmp(help_files[ii]->hf_help_name, help_parm)) {
            hf = help_files[ii];
        }
    }

    if (!hf && !exact_match) {
        for (ii = 0; !hf && ii < help_nfiles; ii++) {
            if (!Memicmp(help_files[ii]->hf_help_name, help_parm, hplen)) {
                hf = help_files[ii];
            }
        }
    }

    return (hf);
}
/***************************************************************/
void get_html_helpfname(struct cenv * ce,
                        const char * help_name,
                        char * help_fname,
                        int help_fname_sz,
                        int help_flags) {

    strmcpy(help_fname, help_name, help_fname_sz - 4);
    strcat (help_fname, ".htm");
}
/***************************************************************/
char * get_compile_date(void)
{
    return (__DATE__);
}
/***************************************************************/
char * get_compile_time(void)
{
    return (__TIME__);
}
/***************************************************************/
void qhsub(struct cenv * ce,
            char * tgt,
            const char * src,
            int tmax,
            struct helpfil * hf) {
/*
** Copies src to tgt, trucating src if too long.
**  tgt always termintated with '\0'
*/

    int bix;
    int six;
    int tix;
    int slen;
    int sublen;
    char * bch;
    char subbuf[128];

    six = 0;
    tix = 0;

    bch = strchr(src + six, '$');
    while (bch) {
        bix = (Int)(bch - src);
        slen = bix - six;
        subbuf[0] = '\0';
        if (slen + 2 < tmax - tix) {
            memcpy(tgt + tix, src + six, slen);
            tix += slen;
            switch (*(bch + 1)) {
                case 'C':
                    sprintf(subbuf, "%s %s", get_compile_date(), get_compile_time());
                    break;
                case 'D':
                    get_now(ce, subbuf, sizeof(subbuf));
                    break;
                case 'H':
                    strmcpy(subbuf, hf->hf_help_name, sizeof(subbuf));
                    break;
                case 'T':
                    if (hf->hf_help_topic) {
                        strmcpy(subbuf, hf->hf_help_topic, sizeof(subbuf));
                    }
                    break;
                case 'V':
                    get_desh_version(2, subbuf, sizeof(subbuf));
                    break;
                default:
                    subbuf[0] = '\0';
                    break;
            }
            sublen = Istrlen(subbuf);
            if (sublen + 2 < tmax - tix) {
                memcpy(tgt + tix, subbuf, sublen);
                tix += sublen;
            }
        }
        six = bix + 2;
        bch = strchr(src + six, '$');
    }

    slen = Istrlen(src + six);
    if (slen + 2 < tmax - tix) {
        memcpy(tgt + tix, src + six, slen);
        tix += slen;
    }

    tgt[tix]   = '\n';
    tgt[tix+1] = '\0';
}
/***************************************************************/
int write_html_helpfil_header(struct cenv * ce,
                struct helpfil * hf,
                struct hfoutfil * hff,
                int help_flags) {

    int estat;
    int ii;
    char hlin[HELP_LINE_SIZE];

    estat = 0;

    for (ii = 0; html_helpfil_header[ii]; ii++) {
        qhsub(ce, hlin, html_helpfil_header[ii], HELP_LINE_SIZE, hf);
        fputs(hlin, hff->hff_houtfref);
    }

    return (estat);
}
/***************************************************************/
int write_html_helpfil_footer(struct cenv * ce,
                struct helpfil * hf,
                struct hfoutfil * hff,
                int help_flags) {

    int estat;
    int ii;
    char hlin[HELP_LINE_SIZE];

    estat = 0;

    for (ii = 0; html_helpfil_footer[ii]; ii++) {
        qhsub(ce, hlin, html_helpfil_footer[ii], HELP_LINE_SIZE, hf);
        fputs(hlin, hff->hff_houtfref);
    }

    return (estat);
}
/***************************************************************/
int hfoutfil_put(struct hfoutfil * hff, const char * outbuf) {

    int nout;
    int estat;

    estat = 0;

    nout = Istrlen(outbuf) + 1;

    if (hff->hff_outbuf_len + nout >= hff->hff_outbuf_max) {
        hff->hff_outbuf_max = hff->hff_outbuf_len + nout + 64;

        hff->hff_outbuf = Realloc(hff->hff_outbuf, char, hff->hff_outbuf_max);
    }
    memcpy(hff->hff_outbuf + hff->hff_outbuf_len, outbuf, nout);
    hff->hff_outbuf_len += nout - 1;

    return (estat);
}
/***************************************************************/
int render_html(struct cenv * ce,
                struct hfoutfil * hff,
                int help_flags) {

    int estat;

    estat = 0;

    fputs(hff->hff_outbuf, hff->hff_houtfref);

    hff->hff_outbuf_len   = 0;

    return (estat);
}
/***************************************************************/
int render_html_br(struct cenv * ce,
                struct hfoutfil * hff,
                int help_flags) {

    int estat;

    hfoutfil_put(hff, "\n");

    estat = render_html(ce, hff, help_flags);

    return (estat);
}
/***************************************************************/
int render_html_new_line(struct cenv * ce,
                struct hfoutfil * hff,
                int help_flags) {

    int estat;

    hfoutfil_put(hff, "\n");

    estat = render_html(ce, hff, help_flags);

    return (estat);
}
/***************************************************************/
int render_html_end_of_block(struct cenv * ce,
                struct hfoutfil * hff,
                int stop_at,
                int help_flags) {
    int estat;
    int sitm;
    int done;

    estat = 0;
    done  = 0;

    while (!done) {
        if (!hff->hff_htm_stack_tos) {
            done = 1;
        } else {
            sitm = peek_htm_stack(hff);
            if (sitm == stop_at) {
                done = 1;
            } else {
                pop_htm_stack(hff);
                switch (sitm) {
                    case HFF_HTM_STACK_TABLE:
                        hfoutfil_put(hff, "\n</table>\n");
                        hff->hff_table_state = 0;
                        break;
                    case HFF_HTM_STACK_TR:
                        hfoutfil_put(hff, "</tr>");
                        hff->hff_table_state = HFF_TABLE_STATE_TABLE;
                        break;
                    case HFF_HTM_STACK_TD:
                        hfoutfil_put(hff, "</td>\n");
                        hff->hff_table_state = HFF_TABLE_STATE_TR;
                        break;
                    case HFF_HTM_STACK_P:
                        hfoutfil_put(hff, "</p>");
                        break;
                    default:
                        break;
                }
            }
        }
    }

    if (!stop_at) {
        hfoutfil_reset(hff);
    }

    return (estat);
}
/***************************************************************/
int render_html_end_of_line(struct cenv * ce,
                struct hfoutfil * hff,
                int help_flags) {

    int estat;

    estat = render_html_end_of_block(ce, hff, HFF_TABLE_STATE_TABLE, help_flags);
    if (!estat) {
        estat = render_html_new_line(ce, hff, help_flags);
        hff->hff_current_col = 0;
    }

    return (estat);
}
/***************************************************************/
void hff_set_tab(struct hfoutfil * hff, int curr_col, int tab_stop) {

    int new_tab_max_col;

    if (curr_col >= hff->hff_tab_max_cols) {
        new_tab_max_col = curr_col + 8;
        hff->hff_col_tabs = Realloc(hff->hff_col_tabs, char, new_tab_max_col);
        while (hff->hff_tab_max_cols < new_tab_max_col) {
            hff->hff_col_tabs[hff->hff_tab_max_cols] = 0;
            hff->hff_tab_max_cols += 1;
        }
    }

    if (!hff->hff_col_tabs[curr_col]) {
        hff->hff_col_tabs[curr_col]  = tab_stop;
        hff->hff_table_cols     += 1;
    }
}
/***************************************************************/
/****
int hff_get_next_tab_stop(struct hfoutfil * hff, int from_col) {

    int tix;

    tix = from_col + 1;
    while (tix < hff->hff_tab_max_cols && !hff->hff_col_tabs[tix]) tix++;

    if (tix == hff->hff_tab_max_cols) tix = -1;

    return (tix);
}
****/
/***************************************************************/
void append_str_len_html(struct str * hss, const char * buf, int blen) {

    int  ii;

    for (ii = 0; ii < blen; ii++) {
        switch (buf[ii]) {
            case '<' : append_str_len(hss, "&lt;", 4);      break;
            case '>' : append_str_len(hss, "&gt;", 4);      break;
            case '&' : append_str_len(hss, "&amp;", 5);     break;
            case '\'': append_str_len(hss, "&#39;", 5);     break;
            case '\"': append_str_len(hss, "&quot;", 6);    break;
            default: APPCH(hss,buf[ii]);                    break;
        }
    }
}
/***************************************************************/
int cook_html_token(struct cenv * ce,
                struct str * rawtoke,
                struct str * cookedtoke,
                int * ncookedchars,
                int help_flags) {
/*
** <a href="Projects1311081157.htm">Previous Schedule (11/08/2013 11:57)</a>
**
** <a href="#future_projects_list">Future Projects</a>
**      <a name="future_projects_list"></a>
*/
    int estat;
    int ii;
    int is_link;
    int rawlen;

    estat   = 0;
    is_link = 0;
    rawlen  = rawtoke->str_len;
    ii      = 0;
    cookedtoke->str_len = 0;

    if (rawlen > 1 &&
        rawtoke->str_buf[0] == '_' &&
        rawtoke->str_buf[rawlen - 1] == '_') {
        is_link = 1;
        ii      += 1;
        rawlen  -= 1;
        append_str_len(cookedtoke, "<a href=\"", 9);
        if (rawlen > 6 &&
            toupper(rawtoke->str_buf[ii    ]) == 'H' &&
            toupper(rawtoke->str_buf[ii + 1]) == 'T' &&
            toupper(rawtoke->str_buf[ii + 2]) == 'T' &&
            toupper(rawtoke->str_buf[ii + 3]) == 'P') {
            if (rawtoke->str_buf[ii + 4] == ':') {
                is_link = 2;
            } else if (toupper(rawtoke->str_buf[ii + 4]) == 'S' &&
                       rawtoke->str_buf[ii + 4] == ':') {
                is_link = 3;
                append_str_char(cookedtoke, "<a href=\"");
            }
        }
    }

    append_str_len_html(cookedtoke, rawtoke->str_buf + ii, rawlen - ii);

    if (is_link) {
        if (is_link == 1) {
            append_str_len(cookedtoke, ".htm\">", 6);
        } else {
            append_str_len(cookedtoke, "\" target=\"_blank\">", 18);
        }

        append_str_len_html(cookedtoke, rawtoke->str_buf + ii, rawlen - ii);
        append_str_len(cookedtoke, "</a>", 4);
    }
    APPCH(cookedtoke,'\0');

    return (estat);
}
/***************************************************************/
/****
int scan_html_help_src_line(struct cenv * ce,
                const char * help_line,
                struct hfoutfil * hff,
                struct str * rawtoke,
                struct str * cookedtoke,
                int help_flags) {

    int estat;
    int ix;
    int eol;
    int nsp;
    int curr_col;
    int ncookedchars;

    estat       = 0;
    curr_col    = 0;
    ix          = 0;
    eol         = 0;

    if (!help_line[ix]) {
        if (hff->hff_table_cols) {
            estat = render_html_end_of_table(ce, hff, help_flags);
        } else {
            estat = render_html_new_line(ce, hff, help_flags);
        }
        hfoutfil_reset(hff);
    }

    while (!estat && help_line[ix]) {
        nsp = 0;
        while (isspace(help_line[ix])) { ix++; nsp++; }

        if  (help_line[ix]) {
            curr_col += nsp;

            if (nsp >= HELP_SPACES_FOR_TAB) {
                hff_set_tab(hff, curr_col);
            }

            eol = get_help_toke(help_line, &ix, rawtoke);
            if (!estat) {
                estat = cook_html_token(ce, rawtoke, cookedtoke, &ncookedchars, help_flags);
                if (!estat) {
                    curr_col += ncookedchars;
                }
            }
        }
    }

    if (hff->hff_table_cols) {
        estat = render_html_start_table(ce, hff, help_flags);
        hff->hff_current_col = 0;
    }

    return (estat);
}
****/
/*
    Case 1 (done): Plain sentences:

        Desh is a new shell and command language interpreter that executes commands
        read from the standard input or from a file.  It has some features that are similar
        to other shells (such as bash POSIX shells), however most impementation 
        details differ from standard shells.

        HTML:

        Desh is a new shell and command language interpreter that executes commands
        read from the standard input or from a file.  It has some features that are similar
        to other shells (such as bash POSIX shells), however most impementation 
        details differ from standard shells.<br/><br/>

        * Need to translate _link_, &, < and >.

    Case 2 (done): Plain sentences (first line indented, second line not indented):

                This series of commands copies files aaa and bbb.  If
        an error occurs during copying, the catch commands are executed which
        display an error message.  If all copy commands are successful, the
        catch commands are skipped.

        HTML:

        <table class="stytable" border="0" >
         <tr class="styRow">
          <td width="180" class="styCell">&nbsp;</td>
          <td class="styCell">parse(&lt;string&gt;) Parse a string into tokens
        using spaces as demlimiters.  The parsing is the same as parameter command
        parsing.</td>
         </tr>
        </table>

    Case 3: Plain sentences (first line indented, second line indented):

                            COUNTER is 4
                            COUNTER is 3
                            COUNTER is 2
                            COUNTER is 1

        HTML:

        <table class="stytable" border="0" >
         <tr class="styRow">
          <td width="140" class="styCell">&nbsp;</td>
          <td class="styCell">COUNTER is 4</td>
         </tr>
         <tr class="styRow">
          <td width="140" class="styCell">&nbsp;</td>
          <td class="styCell">COUNTER is 3</td>
         </tr>
         <tr class="styRow">
          <td width="140" class="styCell">&nbsp;</td>
          <td class="styCell">COUNTER is 2</td>
         </tr>
         <tr class="styRow">
          <td width="140" class="styCell">&nbsp;</td>
          <td class="styCell">COUNTER is 1</td>
         </tr>
        </table>

    Case 4: Two columns (first line indented, second line not indented):

            <var>           The name of the variable set by for.  Each time through
        the loop, <var> is set to the next value.

        HTML:

        <table class="stytable" border="0" >
         <tr class="styRow">
          <td width="140" class="styCell">&nbsp;</td>
          <td width="100" class="styCell">&lt;var&gt;</td>
          <td class="styCell">The name of the variable set by for.  Each time through
        the loop, &lt;var&gt; is set to the next value.</td>
         </tr>
        </table>

    Case 5: Two columns (first line not indented, second line indented):

        Examples:   help commands
                    help dir

        HTML:

        <table class="stytable" border="0" >
         <tr class="styRow">
          <td width="100" class="styCell">Examples:</td>
          <td width="140" class="styCell">help commands</td>
         </tr>
         <tr class="styRow">
          <td width="100" class="styCell">&nbsp;</td>
          <td width="140" class="styCell">help dir</td>
         </tr>
        </table>

    Case 6: Two columns (first line not indented, second line indented more than the first):

        Examples:   help commands
                        help dir

        HTML:

        <table class="stytable" border="0" >
         <tr class="styRow">
          <td width="100" class="styCell">Examples:</td>
          <td width="140" class="styCell">help commands</td>
         </tr>
         <tr class="styRow">
          <td width="100" class="styCell">&nbsp;</td>
          <td width="140" class="styCell">&nbsp;&nbsp;&nbsp;&nbsp;help dir</td>
         </tr>
        </table>

    Case 7: Two columns (All lines indented, second line indented):

        Examples:       Case seven
                        help commands
                        help dir

        HTML:
            <table  class="stytable" border="0">
            <tr class="styRow">
            <td valign="top" width="80" class="styCell"></td>
            <td valign="top" width="160" class="styCell"><p>Examples:</p></td>
            <td class="styCell"><p>Case
            seven</p></td>
            </tr>
            <tr class="styRow">
            <td valign="top" width="80" class="styCell"></td>
            <td valign="top" width="160" class="styCell"></td><td class="styCell"><p>help
            commands</p></td>
            </tr>
            <tr class="styRow">
            <td valign="top" width="80" class="styCell"></td>
            <td valign="top" width="160" class="styCell"></td><td class="styCell"><p>help
            dir</p></td>
            </tr>
            </table>


    Case 8: Three or more columns:

            _commands_        _datefmts_        _desh_            _errors_
        _expfunctions_
        _expressions_
        _intro_
        _keyboard_

        HTML:

        <table class="stytable" border="0" >
         <tr class="styRow">
          <td width="50" class="styCell">&nbsp;</td>
          <td width="80" class="styCell"><a href="commands.htm">commands</a></td>
          <td width="80" class="styCell"><a href="datefmts.htm">datefmts</a></td>
          <td width="80" class="styCell"><a href="desh.htm">desh</a></td>
          <td width="80" class="styCell"><a href="errors.htm">errors</a></td>
         </tr>
         <tr class="styRow">
          <td width="50" class="styCell">&nbsp;</td>
          <td width="100" class="styCell"><a href="expfunctions.htm">expfunctions</a></td>
          <td width="100" class="styCell"><a href="expressions.htm">expressions</a></td>
          <td width="100" class="styCell"><a href="intro.htm">intro</a></td>
          <td width="100" class="styCell"><a href="keyboard.htm">keyboard</a></td>
         </tr>
        <br/><br/>

******** All cases

Case 1. Desh is a new shell and command language interpreter that executes commands
read from the standard input or from a file.  It has some features that are similar
to other shells (such as bash POSIX shells), however most impementation 
details differ from standard shells.

        Case 2. This series of commands copies files aaa and bbb.  If
an error occurs during copying, the catch commands are executed which
display an error message.  If all copy commands are successful, the
catch commands are skipped.

                    Case 3 is three
                    COUNTER is 4
                    COUNTER is 3
                    COUNTER is 2
                    COUNTER is 1

        Case 4     The argument number. argv(0) is the name of the
function or program.  If <n> is larger than ARGC (the highest argument
number), an empty string is returned.

Examples:       Case Five
                help commands
                help dir

Examples:       Case six
                    help commands
                help dir

        Examples:       Case seven
                        help commands
                        help dir


*/
            /*
            if (nsp >= HELP_SPACES_FOR_TAB) {
                hff_goto_tab(hff, curr_col);
            } else if (!ix && !hff->hff_table_cols) {
                tix = hff_get_next_tab_stop(hff, curr_col);
                if (tix > 0) {
                    curr_col = tix;
                    hff_goto_tab(hff, curr_col);
                }
            }
            */

/***************************************************************/
/****
struct hfoutfil {
    FILE *  hff_houtfref;
    int     hff_table_cols;
    int     hff_current_col;
    int     hff_outbuf_len;
    int     hff_outbuf_max;
    int     hff_htm_stack_tos;
    int     hff_htm_stack[HFF_HTM_STACK_SZ];
    char *  hff_outbuf;
    int     hff_ntabs;
    int     hff_tab_max_cols;
    char *  hff_col_tabs;
    struct term *   hff_tty;
};

****/
int scan_first_line_in_block(struct cenv * ce,
                struct helpfil * hf,
                int * lnum,
                struct hfoutfil * hff,
                int help_flags) {

    int emptyln;
    int ix;
    int nsp;
    int curr_col;

    emptyln     = 0;
    curr_col    = 0;
    ix          = 0;

    hff->hff_table_cols = 0;

    while (isspace(hf->hf_line[*lnum][ix])) ix++;

    while (hf->hf_line[*lnum][ix]) {
        nsp = 0;
        while (isspace(hf->hf_line[*lnum][ix])) { ix++; nsp++; }

        if (hf->hf_line[*lnum][ix]) {
            if (!curr_col || nsp >= HELP_SPACES_FOR_TAB) {
                hff_set_tab(hff, curr_col, ix);
                curr_col++;
            }
            while (hf->hf_line[*lnum][ix] && !isspace(hf->hf_line[*lnum][ix])) ix++;
        }
    }

    if (!curr_col) {
        emptyln = 1;
    } else if (curr_col == 1 && !hff->hff_col_tabs[0]) {
        push_htm_stack(hff, HFF_HTM_STACK_P);
        hfoutfil_put(hff, "<p>");
    } else {
        hff->hff_table_state = HFF_TABLE_STATE_TABLE;
        push_htm_stack(hff, HFF_HTM_STACK_TABLE);
        hfoutfil_put(hff, "<table  class=\"stytable\" border=\"0\">");
        render_html_new_line(ce, hff, help_flags);
    }

    return (emptyln);
}
/***************************************************************/
int hff_goto_next_html_tab(struct cenv * ce,
                struct hfoutfil * hff,
                int num_spaces,
                int help_flags) {

    int estat;
    int colwidth;
    int nsp;
    char outbuf[128];

    estat       = 0;
    nsp         = 0;

    if (hff->hff_current_col) {
        estat = render_html_end_of_block(ce, hff, HFF_HTM_STACK_TR, help_flags);
    } else {
        if (hff->hff_col_tabs[0]) {
            sprintf(outbuf, "<td width=\"%d\" class=\"styCell\"></td>",
                    hff->hff_col_tabs[0] * HTM_WIDTH_PER_CHAR);
            hfoutfil_put(hff, outbuf);
            nsp = num_spaces - hff->hff_col_tabs[0];
        }
        if (hff->hff_table_cols > 1 && num_spaces >= hff->hff_col_tabs[1]) {
            sprintf(outbuf, "<td valign=\"top\" width=\"%d\" class=\"styCell\"></td>",
                    (hff->hff_col_tabs[1] - hff->hff_col_tabs[0]) * HTM_WIDTH_PER_CHAR);
            hfoutfil_put(hff, outbuf);
            nsp = num_spaces - hff->hff_col_tabs[1];
            hff->hff_current_col += 1;
        }
   }

    if (hff->hff_current_col + 1 < hff->hff_table_cols) {
        colwidth = hff->hff_col_tabs[hff->hff_current_col + 1] - hff->hff_col_tabs[hff->hff_current_col];
        sprintf(outbuf, "<td valign=\"top\" width=\"%d\" class=\"styCell\">",
                colwidth * HTM_WIDTH_PER_CHAR);
        hfoutfil_put(hff, outbuf);
    } else {
        hfoutfil_put(hff, "<td class=\"styCell\">");
    }

    push_htm_stack(hff, HFF_HTM_STACK_TD);
    hff->hff_table_state = HFF_TABLE_STATE_TD;
    push_htm_stack(hff, HFF_HTM_STACK_P);
    hfoutfil_put(hff, "<p>");
    hff->hff_current_col += 1;

    while (nsp > 0) {
        strcpy(outbuf, "&nbsp;");
        hfoutfil_put(hff, outbuf);
        nsp--;
    }

    return (estat);
}
/***************************************************************/
int render_html_help_src_block(struct cenv * ce,
                struct helpfil * hf,
                int * lnum,
                struct hfoutfil * hff,
                struct str * rawtoke,
                struct str * cookedtoke,
                int help_flags) {

    int estat;
    int ix;
    int eol;
    int eob;
    int nsp;
    int curr_col;
    int ncookedchars;
    int emptyln;

    estat       = 0;
    curr_col    = 0;
    ix          = 0;
    eol         = 0;
    eob         = 0;

    emptyln = scan_first_line_in_block(ce, hf, lnum, hff, help_flags);
    if (emptyln) {
       (*lnum) += 1;
    } else {
        while (!estat && !eob) {
            nsp = 0;
            while (isspace(hf->hf_line[*lnum][ix])) { ix++; nsp++; }

            if (!hf->hf_line[*lnum][ix]) {
                (*lnum) += 1;
                if (!hf->hf_line[*lnum] || !hf->hf_line[*lnum][0]) {
                    eob = 1;
                } else {
                    if (isspace(hf->hf_line[*lnum][0])) {
                        estat = render_html_end_of_line(ce, hff, help_flags);
                        curr_col    = 0;
                    } else if (hff->hff_table_cols >= MIN_COLS_FOR_TABLE) {
                        if (hff->hff_current_col >= hff->hff_table_cols) {
                            estat = render_html_end_of_block(ce, hff, HFF_TABLE_STATE_TABLE, help_flags);
                            hff->hff_current_col = 0;
                            push_htm_stack(hff, HFF_HTM_STACK_TR);
                            hff->hff_table_state = HFF_TABLE_STATE_TR;
                            hfoutfil_put(hff, "<tr class=\"styRow\">");
                            estat = render_html_new_line(ce, hff, help_flags);
                        }
                        hff_goto_next_html_tab(ce, hff, nsp, help_flags);
                 } else {
                        if (hff->hff_outbuf_len < HELP_LINE_OUT_LEN) {
                            hfoutfil_put(hff, " ");
                        } else {
                            estat = render_html_new_line(ce, hff, help_flags);
                        }
                    }
                    ix = 0;
                }
            } else {
                if (hff->hff_table_state == HFF_TABLE_STATE_TABLE) {
                    push_htm_stack(hff, HFF_HTM_STACK_TR);
                    hff->hff_table_state = HFF_TABLE_STATE_TR;
                    hfoutfil_put(hff, "<tr class=\"styRow\">");
                    estat = render_html_new_line(ce, hff, help_flags);
                    if (nsp < HELP_SPACES_FOR_TAB) {
                        curr_col++;
                        hff_goto_next_html_tab(ce, hff, nsp, help_flags);
                    }
                }

                if (nsp >= HELP_SPACES_FOR_TAB) {
                    curr_col++;
                    hff_goto_next_html_tab(ce, hff, nsp, help_flags);
                } else if (nsp) {
                    if (hff->hff_outbuf_len >= HELP_LINE_OUT_LEN) {
                        estat = render_html_new_line(ce, hff, help_flags);
                    } else {
                        while (nsp) {
                            hfoutfil_put(hff, " ");
                            nsp--;
                        }
                    }
                }

                if (!estat) {
                    eol = get_help_toke(hf->hf_line[*lnum], &ix, rawtoke);
                }

                if (!estat && !eol) {
                    estat = cook_html_token(ce, rawtoke,
                                    cookedtoke, &ncookedchars, help_flags);
                    if (!estat) {
                        hfoutfil_put(hff, cookedtoke->str_buf);
                    }
                }
            }
        }
    }

    if (!estat) {
        estat = render_html_end_of_block(ce, hff, 0, help_flags);
    }

    if (!estat) {
        if (hf->hf_line[*lnum] && !hf->hf_line[*lnum][0]) {
            while (!estat && hf->hf_line[*lnum] && !hf->hf_line[*lnum][0]) {
                estat = render_html_br(ce, hff, help_flags);
                (*lnum) += 1;
            }
        } else {
            estat = render_html(ce, hff, help_flags);
        }
    }

    return (estat);
}
/***************************************************************/
int render_html_help(struct cenv * ce,
                struct helpfil * hf,
                struct hfoutfil * hff,
                int help_flags) {
    int estat;
    int lnum;
    struct str rawtoke;
    struct str cookedtoke;

    estat = 0;
    lnum  = 0;

    init_str(&rawtoke, INIT_STR_CHUNK_SIZE);
    init_str(&cookedtoke, INIT_STR_CHUNK_SIZE);

    if (hf->hf_help_topic) {
        Free(hf->hf_help_topic);
        hf->hf_help_topic = NULL;
    }

    if (hf->hf_line[lnum][0]) {
        hf->hf_help_topic = Strdup(hf->hf_line[lnum]);
        lnum++;
        if (hf->hf_line[lnum] && !hf->hf_line[lnum][0]) lnum++;
    }

    if (!estat) estat = write_html_helpfil_header(ce, hf, hff, help_flags);

    while (!estat && lnum < hf->hf_nlines) {
        if (!hf->hf_line[lnum][0]) {
            lnum++;
        } else {
            estat = render_html_help_src_block(ce, hf, &lnum, hff,
                    &rawtoke, &cookedtoke, help_flags);
        }
    }

    free_str_buf(&rawtoke);
    free_str_buf(&cookedtoke);

    return (estat);
}
/***************************************************************/
int write_html_helpfil_body(struct cenv * ce,
            struct helpfil * hf,
            struct hfoutfil * hff,
            int help_flags) {

    int estat;

    estat = render_html_help(ce, hf, hff, help_flags);

    return (estat);
}
/***************************************************************/
int write_html_helpfil(struct cenv * ce,
            const char * help_dir,
            struct helpfil * hf,
            int help_flags) {

    int estat;
    FILE * nhfref;
    struct hfoutfil * hff;
    int ern;
    char help_fname[FILE_MAXIMUM];
    char helpfil_name[PATH_MAXIMUM];

    estat = 0;

    get_html_helpfname(ce, hf->hf_help_name,
            help_fname, sizeof(help_fname), help_flags);

    os_makepath(help_dir, help_fname,
                    helpfil_name, PATH_MAXIMUM, DIR_DELIM);

    nhfref = fopen(helpfil_name, "w");
    if (!nhfref) {
        ern = ERRNO;
        estat = set_ferror(ce, ENEWHELP, "Error opening new html help file %s: %m",
            helpfil_name, ern);
        return (estat);
    }

    if (help_flags & HELPFLAG_VERBOSE) {
        estat = printll(ce, "writing html help file %s\n", helpfil_name);
    }

    hff = new_hfoutfil(nhfref);

    if (!estat) estat = write_html_helpfil_body(ce, hf, hff, help_flags);

    if (!estat) estat = write_html_helpfil_footer(ce, hf, hff, help_flags);

    free_hfoutfil(hff);

    return (estat);
}
/***************************************************************/
int make_directory(struct cenv * ce, const char * dir_name, int verbose) {

    int estat;
    int rtn;
    int ern;

    estat = 0;

    rtn = MKDIR(dir_name);
    if (rtn) {
        ern = ERRNO;
        estat = set_ferror(ce, EMKDIR,"%s: %m", dir_name, ern);
    } else if (verbose) {
        estat = printll(ce, "mkdir %s\n", dir_name);
    }

    return (estat);
}
/***************************************************************/
int write_html_help(struct cenv * ce, const char * help_dir, int help_flags) {

    int estat;
    int ftyp;
    int ii;

    estat = 0;

    ftyp = file_stat(help_dir);
    if (ftyp && !(help_flags & HELPFLAG_KEEP)) {
        if (!(help_flags & HELPFLAG_FORCE)) {
            estat = set_error(ce, ENOFORCE, help_dir);
        } else if (ftyp != FTYP_DIR) {
            estat = set_error(ce, EEXPDIR, help_dir);
        } else {
            estat = remove_directory(ce, CMDFLAG_FORCE, help_dir);
            if (!estat && (help_flags & HELPFLAG_VERBOSE)) {
                estat = printll(ce, "removed directory %s\n", help_dir);
            }
            if (estat && (help_flags & HELPFLAG_CONTINUE)) {
                if (help_flags & HELPFLAG_VERBOSE) {
                    estat = printll(ce, "Error removing directory %s\n", help_dir);
                }
                clear_error(ce, estat);
                estat = 0;
            }
        }
    }

    if (!estat && (!ftyp || !(help_flags & HELPFLAG_KEEP))) {
        estat = make_directory(ce, help_dir,
                        (help_flags & HELPFLAG_VERBOSE)?1:0);
    }

    if (!estat) {
        if (help_flags & HELPFLAG_TESTONLY) {
            estat = write_html_helpfil(ce, help_dir, help_files[0], help_flags);
            if (!estat && (help_flags & HELPFLAG_VERBOSE)) {
                estat = printll(ce, "%s.htm written to directory %s\n",
                            help_files[0]->hf_help_name, help_dir);
            }
        } else {
            for (ii = 0; !estat && ii < help_nfiles; ii++) {
                estat = write_html_helpfil(ce, help_dir, help_files[ii], help_flags);
            }

            if (!estat && (help_flags & HELPFLAG_VERBOSE)) {
                estat = printll(ce, "%d help files written to directory %s\n",
                            help_nfiles, help_dir);
            }
        }
    }

    return (estat);
}
/***************************************************************/
int write_c_helpfil_header(struct cenv * ce,
            FILE * nhfil,
            int help_flags) {

    int estat;
    char hlin[HELP_LINE_SIZE];
    char datbuf[32];

    estat = 0;
    get_now(ce, datbuf, sizeof(datbuf));

    strcpy(hlin, "/***************************************************************/\n");
    fputs(hlin, nhfil);

    strcpy(hlin, "/*  chelp.h                                                    */\n");
    fputs(hlin, nhfil);

    Snprintf(hlin, HELP_LINE_SIZE,
        "/*      Generated: %s                        */\n", datbuf);
    fputs(hlin, nhfil);

    strcpy(hlin, "/***************************************************************/\n");
    fputs(hlin, nhfil);

    strcpy(hlin, "\n");
    fputs(hlin, nhfil);

    strcpy(hlin, "#include <stdlib.h>\n");
    fputs(hlin, nhfil);

    strcpy(hlin, "#include <stdio.h>\n");
    fputs(hlin, nhfil);

    strcpy(hlin, "#include <string.h>\n");
    fputs(hlin, nhfil);

    strcpy(hlin, "\n");
    fputs(hlin, nhfil);

    Snprintf(hlin, HELP_LINE_SIZE,
        "char * get_help_src(void) { return (\"H %s\"); }\n", datbuf);
    fputs(hlin, nhfil);

    return (estat);
}
/***************************************************************/
int write_c_helpfil_footer(struct cenv * ce,
            FILE * nhfil,
            int help_flags) {

    int estat;
    int ii;
    int nlen;
    char cstmt[16];
    char spaces[32];
    char hlin[HELP_LINE_SIZE];

    estat = 0;

    strcpy(hlin, "\n");
    fputs(hlin, nhfil);

    strcpy(hlin, "/***************************************************************/\n");
    fputs(hlin, nhfil);

    strcpy(hlin, "char ** get_help_lines(const char * help_item) {\n");
    fputs(hlin, nhfil);

    strcpy(hlin, "\n");
    fputs(hlin, nhfil);

    strcpy(hlin, "    char ** chelp_lines = NULL;\n");
    fputs(hlin, nhfil);

    strcpy(hlin, "\n");
    fputs(hlin, nhfil);

    for (ii = 0; !estat && ii < help_nfiles; ii++) {
        if (ii) strcpy(cstmt, "else if"); else strcpy(cstmt, "     if");
        memset(spaces, ' ', sizeof(spaces));
        nlen = Istrlen(help_files[ii]->hf_help_name);
        if (nlen < sizeof(spaces)) spaces[14 - nlen] = '\0';
        else spaces[0] = '\0';

        Snprintf(hlin, HELP_LINE_SIZE,
            "    %s (!strcmp(help_item, \"%s\"%s)) chelp_lines = %s%s;\n",
            cstmt,
            help_files[ii]->hf_help_name,
            spaces,
            HELP_C_CHAR_PFX,
            help_files[ii]->hf_help_name);
        fputs(hlin, nhfil);
    }

    strcpy(hlin, "\n");
    fputs(hlin, nhfil);

    strcpy(hlin, "    return (chelp_lines);\n");
    fputs(hlin, nhfil);

    strcpy(hlin, "}\n");
    fputs(hlin, nhfil);

    strcpy(hlin, "/***************************************************************/\n");
    fputs(hlin, nhfil);

    return (estat);
}
/***************************************************************/
int cvt_line_to_c_str(const char * inlin,
                        char * hlin,
                        int hlin_max,
                        int help_flags) {

    int estat;
    int ii;
    int hlinix;

    estat = 0;
    hlinix = 0;

    hlin[hlinix++] = ' ';
    hlin[hlinix++] = ' ';
    hlin[hlinix++] = '\"';

    for (ii = 0; inlin[ii]; ii++) {
        if (hlinix + 6 < hlin_max) {
            if (inlin[ii] == '\"' || inlin[ii] == '\\') {
                hlin[hlinix++] = '\\';
            }
            hlin[hlinix++] = inlin[ii];
        }
    }

    hlin[hlinix++] = '\"';
    hlin[hlinix++] = ',';
    hlin[hlinix++] = '\n';
    hlin[hlinix]   = '\0';

    return (estat);
}
/***************************************************************/
int write_c_helpfil(struct cenv * ce,
            FILE * nhfil,
            struct helpfil * hf,
            int help_flags) {

    int estat;
    int ii;
    char hlin[HELP_LINE_SIZE];

    estat = 0;

    strcpy(hlin, "\n");
    fputs(hlin, nhfil);

    Snprintf(hlin, HELP_LINE_SIZE,
        "static char * %s%s[] = {\n",
        HELP_C_CHAR_PFX,
        hf->hf_help_name);
    fputs(hlin, nhfil);

    for (ii = 0; ii < hf->hf_nlines; ii++) {
        cvt_line_to_c_str(hf->hf_line[ii], hlin, HELP_LINE_SIZE, help_flags);
        fputs(hlin, nhfil);
    }

    strcpy(hlin, "NULL\n");
    fputs(hlin, nhfil);

    strcpy(hlin, "};\n");
    fputs(hlin, nhfil);


    return (estat);
}
/***************************************************************/
int write_c_help(struct cenv * ce, const char * help_fname, int help_flags) {

    int estat;
    int ftyp;
    int ii;
    int ern;
    FILE * nhfil;
    char help_c_file[FILE_MAXIMUM];

    estat = 0;
    nhfil = 0;

    if (strchr(help_fname, '.')) {
        strmcpy(help_c_file, help_fname, FILE_MAXIMUM);
    } else {
        strmcpy(help_c_file, help_fname, FILE_MAXIMUM - 2);
        strcat (help_c_file, ".h");
    }

    ftyp = file_stat(help_c_file);
    if (ftyp) {
        if (!(help_flags & HELPFLAG_FORCE)) {
            estat = set_error(ce, ENOFORCE, help_c_file);
        }
    }

    if (!estat) {
        nhfil = fopen(help_c_file, "w");
        if (!nhfil) {
            ern = ERRNO;
            estat = set_ferror(ce, ENEWHELP, "Error opening new help file %s: %m",
                help_c_file, ern);
        }
    }

    if (!estat) {
        estat = write_c_helpfil_header(ce, nhfil, help_flags);
    }

    if (!estat) {
        for (ii = 0; !estat && ii < help_nfiles; ii++) {
            estat = write_c_helpfil(ce, nhfil, help_files[ii], help_flags);
        }
    }

    if (!estat) {
        estat = write_c_helpfil_footer(ce, nhfil, help_flags);
    }

    if (nhfil) fclose(nhfil);

    return (estat);
}
/***************************************************************/
int setup_help_tty(struct cenv * ce, struct foutfil * ff, int help_flags) {

    int estat;
    int found;
    int vnum;

    estat = 0;

    if (ISATTY(ce->ce_stdin_f)) {
        ff->ff_tty = new_term(ce->ce_stdin_f, ce->ce_stdout_f, 0);
        if (!ff->ff_tty) {
            estat = set_error(ce, ETERMINAL, term_errmsg(NULL, TERRN_INIT));

            return (estat);
        }
    }

    if (ff->ff_tty) {
        found = get_var_int(ce, "DESHPAGELEN", &vnum);
        if (found == 1 && vnum >= 0) ff->ff_pagelen = vnum;
        else                         ff->ff_pagelen = FF_PAGELEN_DEFAULT;
    }

    return (estat);
}
/***************************************************************/
int search_help(struct cenv * ce, const char * help_parm, int help_flags) {

    int estat;
    int ii;
    struct foutfil * ff;

    estat = 0;

    ff = new_foutfil(ce->ce_stdout_f);
    ff->ff_search_term  = Strdup(help_parm);

    estat = setup_help_tty(ce, ff, help_flags);

    for (ii = 0; !estat && ii < help_nfiles; ii++) {
        Free(ff->ff_help_title);
        ff->ff_line_count = 0;

        ff->ff_help_title = Strdup(help_files[ii]->hf_help_name);

        estat = render_help(ce, help_files[ii], ff, help_flags);

        if (help_flags & HELPFLAG_TESTONLY) ii = help_nfiles;
    }

    free_foutfil(ff, 0);

    return (estat);
}
/***************************************************************/
int show_help(struct cenv * ce, const char * help_parm, int help_flags) {

    int estat;
    struct helpfil * hf;
    struct foutfil * ff;

    estat = 0;

    if (!help_nfiles) {
        estat = init_help(ce);
        if (estat) {
            help_nfiles = -1;
            return (estat);
        }
    }

    if (help_nfiles < 0) {
        estat = set_error(ce, ENOHELP, NULL);
        return (estat);
    }

    if (help_flags & HELPFLAG_HTML) {
        estat = write_html_help(ce, help_parm, help_flags);

    } else if (help_flags & HELPFLAG_WHERE) {
        estat = printll(ce, "%s\n", get_help_src());

    } else if (help_flags & HELPFLAG_C) {
        estat = write_c_help(ce, help_parm, help_flags);

    } else if (help_flags & HELPFLAG_SEARCH) {
        estat = search_help(ce, help_parm, help_flags);

    } else {
        hf = find_help(ce, help_parm, 0, help_flags);
        if (!hf) {
            estat = set_error(ce, EFINDHELP, help_parm);
            return (estat);
        }

        ff = new_foutfil(ce->ce_stdout_f);

        estat = setup_help_tty(ce, ff, help_flags);
        if (!estat) {
            estat = render_help(ce, hf, ff, help_flags);
        }

        free_foutfil(ff, 0);
    }

    return (estat);
}
/***************************************************************/
int chndlr_help(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix) {
/*
Syntax:

    help [...]                          help statement
*/

    int estat;
    int estat2;
    int nparms;
    int help_flags;
    int anyhelp;
    int ix;
    char * parm;
    struct parmlist pl;

    estat = 0;
    nparms = 0;
    help_flags = 0;
    anyhelp = 0;

    estat = parse_parmlist(ce, cmdstr, cmdix, &pl);
    if (estat) return (estat);

    estat = handle_redirs(ce, &pl);
    if (estat) return (estat);

    while (!estat && nparms < pl.pl_ce->ce_argc) {
        parm = pl.pl_ce->ce_argv[nparms];
        if (parm[0] == '-' && parm[1]) {
            ix = 0;
            while (!estat && parm[ix]) {
                switch (parm[ix]) {
                    case '-':
                        break;
                    case 'c': case 'C':
                        help_flags |= HELPFLAG_C;            /* undocumented */
                        break;
                    case 'e': case 'E':
                        help_flags |= HELPFLAG_CONTINUE;    /* undocumented */
                        break;
                    case 'f': case 'F':
                        help_flags |= HELPFLAG_FORCE;
                        break;
                    case 'h': case 'H':
                        help_flags |= HELPFLAG_HTML;
                        break;
                    case 'i': case 'I':
                        help_flags |= HELPFLAG_TESTONLY;
                        break;
                    case 'k': case 'K':
                        help_flags |= HELPFLAG_KEEP;
                        break;
                    case 'p': case 'P':
                        help_flags |= HELPFLAG_PAGE;
                        break;
                    case 's': case 'S':
                        help_flags |= HELPFLAG_SEARCH;
                        break;
                    case 'v': case 'V':
                        help_flags |= HELPFLAG_VERBOSE;
                        break;
                    case 'w': case 'W':
                        help_flags |= HELPFLAG_WHERE;
                        break;
                    default:
                        estat = set_ferror(ce, EBADOPT, "%c",
                                        parm[ix]);
                        break;
                }
                ix++;
            }
        } else if (parm[0]) {
            estat = show_help(ce, parm, help_flags);
            anyhelp = 1;
        }
        nparms++;            
    }

    if (!estat && !anyhelp) {
        if (help_flags & HELPFLAG_SEARCH) {
            estat = set_error(ce, ENOSEARCH, NULL);
        } else {
            estat = show_help(ce, "index", help_flags);
        }
    }

    estat2 = free_parmlist_buf(ce, &pl);
    if (!estat) estat = estat2;
    else if (estat2) estat = chain_error(ce, estat, estat2);

    return (estat);
}
/***************************************************************/
