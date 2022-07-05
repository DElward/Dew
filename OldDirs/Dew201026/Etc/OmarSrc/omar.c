/***************************************************************/
/* omar.c                                                      */
/***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include "config.h"
#include "omar.h"
#include "util.h"
#include "bid.h"
#include "handrec.h"
#include "onn.h"
#include "crehands.h"
#include "xeq.h"

struct arginfo {
    int     arg_flags;
    char ** arg_handrec;
    int     arg_nhandrec;
    char ** arg_handrecdir;
    int     arg_nhandrecdir;
    struct createhands ** arg_createhands;
    int     arg_ncreatehands;
    struct nnhands ** arg_nnhands;
    int     arg_nnnhands;
    struct nninit * arg_nninit;
    struct nntrain * arg_nntrain;
    struct nnbid * arg_nnbid;
    struct nnplay * arg_nnplay;
    int  arg_nnnsave;
    char **  arg_nnsave;
    char *  arg_nnload;
    int  arg_nxeq;
    char **  arg_xeq;
    int  arg_nddplay;
    char **  arg_ddplay;
    int  arg_ngib;
    char **  arg_gib;
};

#define ARG_FLAG_PAUSE       1
#define ARG_FLAG_HELP        2
#define ARG_FLAG_DONE        4
#define ARG_FLAG_VERSION     8
#define ARG_FLAG_SHOW        16
#define ARG_FLAG_VERSION_NUM 32
#define ARG_FLAG_VERBOSE     64
#define ARG_FLAG_CHECKCONTRACTS     128
#define ARG_FLAG_DEBUG              256
#define ARG_FLAG_QUIET              512
#define ARG_FLAG_SHOW_MEM           1024
#define ARG_FLAG_DDPLAY             2048
#define ARG_FLAG_GIB                4096

/***************************************************************/
/* Globals                                                     */
struct omarglobals * global_og = NULL;

/***************************************************************/
void omarout (struct omarglobals * og, const char * fmt, ...) {

    va_list args;
    FILE * outf;

    outf = get_outfile(og);

    va_start (args, fmt);
    vfprintf(outf, fmt, args);
    va_end (args);
}
/***************************************************************/
void showusage(char * prognam) {

    printf("%s - v%s\n", PROGRAM_NAME, VERSION);
    printf("\n");
    printf("Usage: %s [options] <config file name>\n", prognam);
    printf("\n");
    printf("Options:\n");
    printf("  -help             -?      Show extended help.\n");
    printf("  -biddef <fname>           Bidding definitions in <fname>\n");
    printf("  -bidtest <fname>          Bidding tests in <fname>\n");
    printf("  -checkcontracts           Show bidding definitions.\n");
    printf("  -createhands <t> <n> <f>  Writes <n> <t> type hands to file <f>.\n");
    printf("  -eof                      Ignore remaining arguments.\n");
    printf("  -handrec <fname>          Process json hand record\n");
    printf("  -handrecdir <dir>         Process directory of hand records\n");
    printf("  -nnhands <f>              Read neural net test hands from file <f>.\n");
    printf("  -out <fname>      -o      Write output to <fname>\n");
    printf("  -parms <fname>            Read program parms drom <fname>\n");
    printf("  -pause            -p      Pause when done.\n");
    printf("  -show                     Show bidding definitions.\n");
    printf("  -verbose          -v      Show more output.\n");
    printf("  -version                  Show program version.**\n");
    printf("\n");
    printf("  ** Unimplemented\n");
    printf("\n");
}
/***************************************************************/
int omarerr_v(struct omarglobals * og, const char * fmt, va_list args) {

    FILE * outf;

    outf = get_outfile(og);

    if (!(og->og_flags & OG_FLAG_QUIET)) vfprintf(outf, fmt, args);
    if (!(og->og_flags & OG_FLAG_QUIET)) fprintf(outf, "\n");

    if (og) og->og_nerrs++;

    return (ESTAT_ERROR);
}
/***************************************************************/
int omarwarn_v(struct omarglobals * og, const char * fmt, va_list args) {

    FILE * outf;

    outf = get_outfile(og);

    if (!(og->og_flags & OG_FLAG_QUIET)) vfprintf(outf, fmt, args);
    if (!(og->og_flags & OG_FLAG_QUIET)) fprintf(outf, "\n");

    if (og) og->og_nwarns++;

    return (ESTAT_WARN);
}
/***************************************************************/
int omarerr(struct omarglobals * og, const char * fmt, ...) {

    va_list args;
    int estat;

    va_start (args, fmt);
    estat = omarerr_v(og, fmt, args);
    va_end (args);

    return (estat);
}
/***************************************************************/
int set_error (struct omarglobals * og, const char * fmt, ...) {

    va_list args;
    int estat;

    va_start (args, fmt);
    estat = omarerr_v(og, fmt, args);
    va_end (args);

    return (estat);
}
/***************************************************************/
int set_warning (struct omarglobals * og, const char * fmt, ...) {

    va_list args;
    int estat;

    va_start (args, fmt);
    estat = omarwarn_v(og, fmt, args);
    va_end (args);

    return (estat);
}
/***************************************************************/
int omargeterr(struct omarglobals * og) {

    int estat = 0;

    if (og->og_nerrs) estat = ESTAT_ERROR;

    return (estat);
}
/***************************************************************/
struct omarglobals * new_omarglobals(void) {

    struct omarglobals * og;

    og = New(struct omarglobals, 1);

    og->og_outfile      = NULL;
    og->og_nerrs        = 0;
    og->og_nwarns       = 0;
    og->og_flags        = 0;
    og->og_nhandrec     = 0;
    og->og_xhandrec     = 0;
    og->og_ahandrec     = NULL;
    og->og_nnd          = NULL;
    og->og_nnthlist     = NULL;

    return (og);
}
/***************************************************************/
void free_omarglobals(struct omarglobals * og) {

    int ii;

    if (og->og_outfile) fclose(og->og_outfile);

    for (ii = 0; ii < og->og_nhandrec; ii++) {
        free_handrec(og->og_ahandrec[ii]);
    }
    Free(og->og_ahandrec);
    free_nndata(og->og_nnd);
    free_nntesthandlist(og->og_nnthlist);

    Free(og);
}
/***************************************************************/
struct arginfo * new_arginfo(void) {

    struct arginfo * args;

    args = New(struct arginfo, 1);
    args->arg_flags     = 0;

    args->arg_nhandrec  = 0;
    args->arg_handrec   = NULL;

    args->arg_nhandrecdir  = 0;
    args->arg_handrecdir   = NULL;

    args->arg_ncreatehands  = 0;
    args->arg_createhands   = NULL;

    args->arg_nnnhands  = 0;
    args->arg_nnhands   = NULL;

    args->arg_nnnsave  = 0;
    args->arg_nnsave   = NULL;

    args->arg_nninit   = NULL;
    args->arg_nntrain  = NULL;
    args->arg_nnload   = NULL;
    args->arg_nnbid    = NULL;
    args->arg_nnplay    = NULL;

    args->arg_nxeq      = 0;
    args->arg_xeq       = NULL;

    args->arg_nddplay  = 0;
    args->arg_ddplay   = NULL;

    args->arg_ngib     = 0;
    args->arg_gib      = NULL;

    return (args);
}
/***************************************************************/
void free_arginfo(struct arginfo * args) {

    int ii;

    for (ii = 0; ii < args->arg_nhandrec; ii++) {
        Free(args->arg_handrec[ii]);
    }
    Free(args->arg_handrec);

    for (ii = 0; ii < args->arg_nhandrecdir; ii++) {
        Free(args->arg_handrecdir[ii]);
    }
    Free(args->arg_handrecdir);

    for (ii = 0; ii < args->arg_ncreatehands; ii++) {
        free_createhands(args->arg_createhands[ii]);
    }
    Free(args->arg_createhands);

    for (ii = 0; ii < args->arg_nnnhands; ii++) {
        free_nnhands(args->arg_nnhands[ii]);
    }
    Free(args->arg_nnhands);

    for (ii = 0; ii < args->arg_nnnsave; ii++) {
        Free(args->arg_nnsave[ii]);
    }
    Free(args->arg_nnsave);

    for (ii = 0; ii < args->arg_nxeq; ii++) {
        Free(args->arg_xeq[ii]);
    }
    Free(args->arg_xeq);

    for (ii = 0; ii < args->arg_nddplay; ii++) {
        Free(args->arg_ddplay[ii]);
    }
    Free(args->arg_ddplay);

    for (ii = 0; ii < args->arg_ngib; ii++) {
        Free(args->arg_gib[ii]);
    }
    Free(args->arg_gib);

    free_nninit(args->arg_nninit);
    free_nntrain(args->arg_nntrain);
    free_nnbid(args->arg_nnbid);
    free_nnplay(args->arg_nnplay);
    Free(args->arg_nnload);

    Free(args);
}
/***************************************************************/
FILE * get_outfile(struct omarglobals * og) {

    FILE * outf;

    if (!og) {
        outf = stderr;
    } else {
        outf = og->og_outfile;
        if (!outf) outf = stdout;
    }

    return (outf);
}
/***************************************************************/
void set_outfile(struct omarglobals * og, FILE * outfile) {

    if (og->og_outfile) fclose(og->og_outfile);

    og->og_outfile = outfile;
}
/***************************************************************/
void set_omarglobals(struct omarglobals * og) {

    global_og = og;
}
/***************************************************************/
struct omarglobals * get_omarglobals(void) {

    return (global_og);
}
/***************************************************************/
int getargs(struct omarglobals * og, int argc, char *argv[], struct arginfo * args);

int parse_parmfile(struct omarglobals * og, const char * parmfname, struct arginfo * args) {

    int estat = 0;
    FILE * pfile;
    static int depth = 0;
    int done;
    char fbuf[256];
    long linenum;
    int ix;
    int parmc;
    char * parmbuf;
    char ** argv;

    depth++;
    if (depth > 8) {
        estat = set_error(og, "-parms depth exceeds maximum of 8.");
    } else {
        pfile = fopen(parmfname, "r");
        if (!pfile) {
            int ern = ERRNO;
            estat = set_error(og, "Error opening parms file %s: %m", parmfname, ern);
        }
    }

    if (!estat) {
        linenum = 0;
        done = 0;
        while (!estat && !done) {
            linenum++;
            if (!fgets(fbuf, sizeof(fbuf), pfile)) {
                if (feof(pfile)) {
                    done = 1;
                } else {
                    int ern = ERRNO;
                    estat = set_error(og, "Error reading parms file %s in line %ld: %m",
                        parmfname, linenum, ern);
                }
            } else {
                ix = IStrlen(fbuf);
                while (ix > 0 && isspace(fbuf[ix - 1])) ix--;
                fbuf[ix] = '\0';
                ix = 0;
                while (isspace(fbuf[ix])) ix++;
                if (fbuf[ix] && fbuf[ix] != '#') {
                    parmbuf = parse_command_line(fbuf, &parmc, "", 0);
                    if (parmbuf && parmc) {
                        argv = (char**)parmbuf;
                        estat = getargs(og, parmc, argv, args);
                        if (estat) {
                            if (estat == ESTAT_EOF) {
                                done = 1;
                                estat = 0;
                            } else {
                                estat = set_error(og, "Error in line %ld of parm file %s",
                                        linenum, parmfname);
                            }
                        }
                    }
                    Free(parmbuf);
                }
            }
        }
    }

    depth--;

    return (estat);
}
/***************************************************************/
int process_ddplay(struct omarglobals * og,
        int nddplayargs,
        char ** ddplayargs)
{
/*
** 03/17/2020
*/
    int estat = 0;

    extern int ddplay_main(int argc, char *argv[]);

    estat = ddplay_main(nddplayargs, ddplayargs);
    if (estat) {
        estat = set_error(og, "Play error.");
    }

    return (estat);
}
/***************************************************************/
int process_gib(struct omarglobals * og,
        int nddplayargs,
        char ** ddplayargs)
{
/*
** 03/20/2020
*/
    int estat = 0;
    int ii;
    char ** gibmyargv;

    extern void gib_main(int argc, char ** myargv);

    /* jjj() modifies arguments. That's why gibmyargv is necessary. */
    gibmyargv = New(char*, nddplayargs);
    for (ii = 0; ii < nddplayargs; ii++) gibmyargv[ii] = ddplayargs[ii];

    gib_main(nddplayargs, gibmyargv);

    Free(gibmyargv);

    return (estat);
}
/***************************************************************/
int getargs(struct omarglobals * og, int argc, char *argv[], struct arginfo * args) {

    int ii;
    int estat = 0;
    FILE * fil;

    for (ii = 1; !estat && ii < argc; ii++) {
        if (argv[ii][0] == '-') {
            if (!Stricmp(argv[ii], "-p") || !Stricmp(argv[ii], "-pause")) {
                args->arg_flags |= ARG_FLAG_PAUSE;
            } else if (!Stricmp(argv[ii], "-?") || !Stricmp(argv[ii], "-help")) {
                args->arg_flags |= ARG_FLAG_HELP | ARG_FLAG_DONE;
            } else if (!Stricmp(argv[ii], "-version")) {
                args->arg_flags |= ARG_FLAG_VERSION | ARG_FLAG_DONE;
            } else if (!Stricmp(argv[ii], "-versnum")) {
                args->arg_flags |= ARG_FLAG_VERSION_NUM | ARG_FLAG_DONE;
            } else if (!Stricmp(argv[ii], "-show")) {
                args->arg_flags |= ARG_FLAG_SHOW;
            } else if (!Stricmp(argv[ii], "-showmem")) {
                args->arg_flags |= ARG_FLAG_SHOW_MEM;
            } else if (!Stricmp(argv[ii], "-v") || !Stricmp(argv[ii], "-verbose")) {
                args->arg_flags |= ARG_FLAG_VERBOSE;
            } else if (!Stricmp(argv[ii], "-debug")) {
                args->arg_flags |= ARG_FLAG_DEBUG;
            } else if (!Stricmp(argv[ii], "-quiet")) {
                args->arg_flags |= ARG_FLAG_QUIET;
            } else if (!Stricmp(argv[ii], "-checkcontracts")) {
                args->arg_flags |= ARG_FLAG_CHECKCONTRACTS;
            } else if (!Stricmp(argv[ii], "-eof")) {
                estat = ESTAT_EOF;

            } else if (!Stricmp(argv[ii], "-handrec")) {
                ii++;
                if (ii < argc && argv[ii][0] != '-') {
                    args->arg_handrec =
                            Realloc(args->arg_handrec, char*, args->arg_nhandrec + 1);
                    args->arg_handrec[args->arg_nhandrec] = Strdup(argv[ii]);
                    args->arg_nhandrec += 1;
                } else {
                    estat = set_error(og, "Expected hand record file name after %s", argv[ii-1]);
                }

            } else if (!Stricmp(argv[ii], "-createhands")) {
                ii++;
                if (ii < argc && argv[ii][0] != '-') {
                    args->arg_createhands =
                            Realloc(args->arg_createhands, struct createhands *, args->arg_ncreatehands + 1);
                    args->arg_createhands[args->arg_ncreatehands] = new_createhands();
                    
                    estat = get_createhands(og,
                        args->arg_createhands[args->arg_ncreatehands],
                        &ii, argc, argv);
                    if (!estat) args->arg_ncreatehands += 1;
                    else free_createhands(args->arg_createhands[args->arg_ncreatehands]);
                } else {
                    estat = set_error(og, "Expected hand type after %s", argv[ii-1]);
                }

            } else if (!Stricmp(argv[ii], "-nnhands")) {
                ii++;
                if (ii < argc && argv[ii][0] != '-') {
                    args->arg_nnhands =
                        Realloc(args->arg_nnhands, struct nnhands *, args->arg_nnnhands + 1);
                    args->arg_nnhands[args->arg_nnnhands] = new_nnhands();
                    
                    estat = get_nnhands(og,
                        args->arg_nnhands[args->arg_nnnhands],
                        &ii, argc, argv);
                    if (!estat) args->arg_nnnhands += 1;
                    else free_nnhands(args->arg_nnhands[args->arg_nnnhands]);
                } else {
                    estat = set_error(og, "Expected hand type after %s", argv[ii-1]);
                }

            } else if (!Stricmp(argv[ii], "-nninit")) {
                ii++;
                if (ii < argc && argv[ii][0] != '-') {
                    if (args->arg_nninit) free_nninit(args->arg_nninit);
                    args->arg_nninit  = new_nninit();
                    
                    estat = get_nninit(og, args->arg_nninit,
                        &ii, argc, argv);
                    if (estat) { free_nninit(args->arg_nninit); args->arg_nninit = NULL; }
                } else {
                    estat = set_error(og, "Expected init info after %s", argv[ii-1]);
                }

            } else if (!Stricmp(argv[ii], "-nntrain")) {
                ii++;
                if (ii < argc && argv[ii][0] != '-') {
                    if (args->arg_nntrain) free_nntrain(args->arg_nntrain);
                    args->arg_nntrain  = new_nntrain();
                    
                    estat = get_nntrain(og, args->arg_nntrain,
                        &ii, argc, argv);
                    if (estat) { free_nntrain(args->arg_nntrain); args->arg_nntrain = NULL; }
                } else {
                    estat = set_error(og, "Expected train info after %s", argv[ii-1]);
                }

            } else if (!Stricmp(argv[ii], "-nnbid")) {
                ii++;

                if (args->arg_nnbid) free_nnbid(args->arg_nnbid);
                args->arg_nnbid  = new_nnbid();
                    
                estat = get_nnbid(og, args->arg_nnbid,
                    &ii, argc, argv);
                if (estat) { free_nnbid(args->arg_nnbid); args->arg_nnbid = NULL; }

            } else if (!Stricmp(argv[ii], "-nnplay")) {
                ii++;

                if (args->arg_nnplay) free_nnplay(args->arg_nnplay);
                args->arg_nnplay  = new_nnplay();
                    
                estat = get_nnplay(og, args->arg_nnplay,
                    &ii, argc, argv);
                if (estat) { free_nnplay(args->arg_nnplay); args->arg_nnplay = NULL; }

            } else if (!Stricmp(argv[ii], "-nnsave")) {
                ii++;
                if (ii < argc && argv[ii][0] != '-') {
                    args->arg_nnsave =
                        Realloc(args->arg_nnsave, char *, args->arg_nnnsave + 1);
                    args->arg_nnsave[args->arg_nnnsave] = Strdup(argv[ii]);
                    args->arg_nnnsave += 1;
                    ii++;
                } else {
                    estat = set_error(og, "Expected file name after %s", argv[ii-1]);
                }

            } else if (!Stricmp(argv[ii], "-xeq")) {
                ii++;
                if (ii < argc && argv[ii][0] != '-') {
                    args->arg_xeq =
                        Realloc(args->arg_xeq, char *, args->arg_nxeq + 1);
                    args->arg_xeq[args->arg_nxeq] = Strdup(argv[ii]);
                    args->arg_nxeq += 1;
                    ii++;
                } else {
                    estat = set_error(og, "Expected file name after %s", argv[ii-1]);
                }

            } else if (!Stricmp(argv[ii], "-ddplay")) {
                args->arg_flags |= ARG_FLAG_DDPLAY;
                ii++;
                while (ii < argc && argv[ii][0] == '+') {
                    args->arg_ddplay =
                        Realloc(args->arg_ddplay, char *, args->arg_nddplay + 1);
                    args->arg_ddplay[args->arg_nddplay] = Strdup(argv[ii]);
                    args->arg_nddplay += 1;
                    ii++;
                }

            } else if (!Stricmp(argv[ii], "-gib")) {
                args->arg_flags |= ARG_FLAG_GIB;
                args->arg_gib =
                    Realloc(args->arg_gib, char *, args->arg_ngib + 1);
                args->arg_gib[args->arg_ngib] = Strdup("gib");
                args->arg_ngib += 1;
                ii++;
                while (ii < argc) {
                    args->arg_gib =
                        Realloc(args->arg_gib, char *, args->arg_ngib + 1);
                    args->arg_gib[args->arg_ngib] = Strdup(argv[ii]);
                    args->arg_ngib += 1;
                    ii++;
                }

            } else if (!Stricmp(argv[ii], "-nnload")) {
                ii++;
                if (ii < argc && argv[ii][0] != '-') {
                    if (args->arg_nnload) Free(args->arg_nnload);
                    args->arg_nnload  = Strdup(argv[ii]);
                    ii++;
                } else {
                    estat = set_error(og, "Expected file name after %s", argv[ii-1]);
                }

            } else if (!Stricmp(argv[ii], "-handrecdir")) {
                ii++;
                if (ii < argc && argv[ii][0] != '-') {
                    args->arg_handrecdir =
                            Realloc(args->arg_handrecdir, char*, args->arg_nhandrecdir + 1);
                    args->arg_handrecdir[args->arg_nhandrecdir] = Strdup(argv[ii]);
                    args->arg_nhandrecdir += 1;
                } else {
                    estat = set_error(og, "Expected hand record file name after %s", argv[ii-1]);
                }

/*
-- Removed. Use: call ans.testFile("bidtest.txt");
            } else if (!Stricmp(argv[ii], "-biddef")) {
                ii++;
                if (ii < argc && argv[ii][0] != '-') {
                    args->arg_biddef =
                            Realloc(args->arg_biddef, char*, args->arg_nbiddef + 1);
                    args->arg_biddef[args->arg_nbiddef] = Strdup(argv[ii]);
                    args->arg_nbiddef += 1;
                } else {
                    estat = set_error(og, "Expected bidding definitions file name after %s", argv[ii-1]);
                }

            } else if (!Stricmp(argv[ii], "-bidtest")) {
                ii++;
                if (ii < argc && argv[ii][0] != '-') {
                    args->arg_bidtest =
                            Realloc(args->arg_bidtest, char*, args->arg_nbidtest + 1);
                    args->arg_bidtest[args->arg_nbidtest] = Strdup(argv[ii]);
                    args->arg_nbidtest += 1;
                } else {
                    estat = set_error(og, "Expected bidding tests file name after %s", argv[ii-1]);
                }
*/

            } else if (!Stricmp(argv[ii], "-o") || !Stricmp(argv[ii], "-out")) {
                ii++;
                if (ii < argc && argv[ii][0] != '-') {
                    fil = fopen(argv[ii], "w");
                    if (fil) {
                        set_outfile(og, fil);
                    } else {
                        int ern = ERRNO;
                        estat = set_error(og, "Error opening %s: %m", argv[ii], ern);
                    }
                } else {
                    estat = set_error(og, "Expected file name after %s", argv[ii-1]);
                }

            } else if (!Stricmp(argv[ii], "-parms")) {
                ii++;
                if (ii < argc || argv[ii][0] == '-') {
                    estat = parse_parmfile(og, argv[ii], args);
                } else {
                    estat = set_error(og, "Expected file name after %s", argv[ii-1]);
                }

            } else {
                estat = set_error(og, "Unrecognized option %s", argv[ii]);
            }
        }
    }

    return (estat);
}
/***************************************************************/
int arg_flags_to_og_flags(int arg_flags)
{
    int og_flags;

    og_flags = 0;
    if (arg_flags & ARG_FLAG_VERBOSE) og_flags |= OG_FLAG_VERBOSE;
    if (arg_flags & ARG_FLAG_DEBUG  ) og_flags |= OG_FLAG_DEBUG;
    if (arg_flags & ARG_FLAG_QUIET  ) og_flags |= OG_FLAG_QUIET;

    return (og_flags);
}
/***************************************************************/
/*@*/int main(int argc, char *argv[]) {

    SEQINFO * si;
    int estat;
    int ii;
    int jj;
    int argflags;
    struct arginfo * args;
    struct handrec * hr;
    struct handrec ** hrl;
    struct omarglobals * og;

    si = NULL;

    if (argc <= 1) {
        showusage(argv[0]);
        return (0);
    }

    og   = new_omarglobals();
    set_omarglobals(og);
    args = new_arginfo();

    estat = getargs(og, argc, argv, args);
    argflags = args->arg_flags;

    if (argflags & ARG_FLAG_HELP) {
        showusage(argv[0]);
    }

    if (!estat) {
        og->og_flags = arg_flags_to_og_flags(argflags);
    }

/*
**** Removed. Use:
        set ans = new Analysis("biddef_test.txt");
        call ans.testFile("bidtest.txt");

    for (ii = 0; ii < args->arg_nbiddef; ii++) {
        biddef_fr = frec_open(args->arg_biddef[ii], "r", "Bid def file");
        if (biddef_fr) {
            si = New(SEQINFO, 1);
            parse_bids(og, biddef_fr, si);
            frec_close(biddef_fr);
            if (args->arg_flags & ARG_FLAG_SHOW) {
                print_seqinfo(og, si);
            }
            if (ii < args->arg_nbidtest) {
                bidtest_fr = frec_open(args->arg_bidtest[ii], "r", "Bid TEST file");
                if (bidtest_fr) {
                    analyze_bidfile(og, bidtest_fr, si);
                    frec_close(bidtest_fr);
                }
            }
            free_seqinfo(si);
        }
    }
*/

    for (ii = 0; ii < args->arg_nhandrec; ii++) {
        hr = parse_handrec(og, args->arg_handrec[ii]);
        if (hr) {
            add_handrec(og, hr);
            if (argflags & ARG_FLAG_VERBOSE) {
                print_handrec(og, hr);
            }
            /*
            bid_handrec(og, hr, si);
            */
        }
    }

    for (ii = 0; ii < args->arg_nhandrecdir; ii++) {
        hrl = parse_handrecdir(og, args->arg_handrecdir[ii], NULL);
        for (jj = 0; hrl[jj]; jj++) {
            add_handrec(og, hrl[jj]);
        }
        Free(hrl);
    }

    if (argflags & ARG_FLAG_CHECKCONTRACTS) {
        check_contracts(og);
    }

    for (ii = 0; !estat && ii < args->arg_ncreatehands; ii++) {
        estat = process_createhands(og, args->arg_createhands[ii]);
    }

    for (ii = 0; !estat && ii < args->arg_nnnhands; ii++) {
        estat = process_nnhands(og, args->arg_nnhands[ii]);
    }

    if (!estat) {
        if (args->arg_nninit) {
            if (og->og_nnd) free_nndata(og->og_nnd);
            og->og_nnd = new_nndata();
            estat = process_nninit(og, args->arg_nninit, og->og_nnd);
        } else if (args->arg_nnload) {
            if (og->og_nnd) free_nndata(og->og_nnd);
            og->og_nnd = new_nndata();
            estat = process_nnload(og, args->arg_nnload, og->og_nnd);
        }
    }

    if (!estat) {
        if (args->arg_nntrain) {
            estat = process_nntrain(og, args->arg_nntrain, og->og_nnd);
        }
    }

    if (!estat) {
        if (args->arg_nnbid) {
            estat = process_nnbid(og, args->arg_nnbid, og->og_nnd);
        }
    }

    if (!estat) {
        if (args->arg_nnplay) {
            estat = process_nnplay(og, args->arg_nnplay, og->og_nnd);
        }
    }

    for (ii = 0; !estat && ii < args->arg_nnnsave; ii++) {
        estat = process_nnsave(og, args->arg_nnsave[ii], og->og_nnd);
    }

    for (ii = 0; !estat && ii < args->arg_nxeq; ii++) {
        estat = process_xeq(og, args->arg_xeq[ii]);
    }

    if (argflags & ARG_FLAG_DDPLAY) {
        estat = process_ddplay(og, args->arg_nddplay, args->arg_ddplay);
    }

    if (argflags & ARG_FLAG_GIB) {
        estat = process_gib(og, args->arg_ngib, args->arg_gib);
    }

    free_arginfo(args);
    free_omarglobals(og);

    if (get_unfreed_mem()) {
        if (argflags & ARG_FLAG_SHOW_MEM) {
            print_mem_stats("OmarMem.txt");
        }

    fprintf(stderr,
    "**************** %d bytes of memory not freed ****************\n",
            get_unfreed_mem());
    }

    if (argflags & ARG_FLAG_PAUSE) {
        char inbuf[100];
        printf("Press ENTER to exit...");
        fgets(inbuf, sizeof(inbuf), stdin);
    }

    return (0);
}
/***************************************************************/
