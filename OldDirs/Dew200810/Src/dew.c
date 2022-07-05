/***************************************************************/
/* dew.c                                                       */
/***************************************************************/

/***************************************************************/
/*  NOTE: In MS Visual Studio 2013                             */
/*        Properties -> Linker -> General -> Output file       */
/*        must be: $(OutDir)$(TargetName)$(TargetExt)          */
/***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>

#include "dew.h"
#include "utl.h"
#include "snprfh.h"
#include "httpio.h"
#include "parsehttp.h"
#include "jsenv.h"

/*
** JavaScript Documentation
**      https://www.w3schools.com/js/default.asp
**      https://en.wikipedia.org/wiki/JavaScript
**      DOM: https://developer.mozilla.org/en-US/docs/Web/API/Document_Object_Model
**
** JavaScript Specification
**      https://www.ecma-international.org/publications/standards/Ecma-262.htm
**      DOM: https://dom.spec.whatwg.org/
**
** JavaScript Reserved Words
**      https://www.w3schools.com/js/js_reserved.asp
**
** HTML Specifications
**  HTML 4.01   http://www.w3.org/TR/html4/
**  HTML 5.2    http://www.w3.org/TR/html5/
**  HTML 5.3    https://w3c.github.io/html/index.html
**
** HTML Documentation
** W3 Schools   https://www.w3schools.com/html/default.asp
** Wikipedia    https://en.wikipedia.org/wiki/HTML_element
**              https://html.com/
*/

/***************************************************************/
enum e_int_file_type {
    IFILTYP_NONE            = 0,
    IFILTYP_HTTP,
    IFILTYP_JAVASCRIPT
};
/***************************************************************/
/* Globals                                                     */

struct globals * g_globals = NULL;

/***************************************************************/
void showusage(char * prognam) {

    printf("%s - v%s\n", PROGRAM_NAME, VERSION);
    printf("\n");
    printf("Usage: %s [options] <url>\n", prognam);
    printf("\n");
    printf("Options:\n");
    printf("  -pause            -p      Pause when done.\n");
    printf("  -version                  Show program version.\n");
    printf("\n");
}
/***************************************************************/
void printout (char * fmt, ...) {

    va_list args;
    struct globals * glb;
    FILE * outf;

    glb = get_globals();
    if (glb->glb_outfile) outf = glb->glb_outfile;
    else outf = stdout;

    va_start (args, fmt);
    vfprintf(outf, fmt, args);
    va_end (args);

    release_globals(glb);
}
/***************************************************************/
void printerr (char * fmt, ...) {

    va_list args;
    struct globals * glb;
    FILE * outf;

    glb = get_globals();
    if (glb->glb_errfile) outf = glb->glb_errfile;
    else outf = stderr;

    va_start (args, fmt);
    vfprintf(outf, fmt, args);
    va_end (args);

    release_globals(glb);
}
/***************************************************************/
static char * find_dewvar(const char * vname) {

    char * vval;

    vval = NULL;

    return (vval);
}
/***************************************************************/
char * get_dewvar(const char * vname) {

    char * out;

    out = find_dewvar(vname);

    if (!out) {
        if (!out) out = getenv(vname);
    }

    return (out);
}
/***************************************************************/
int set_error(int estat, const char * fmt, ...)
{

	va_list args;
    char msgbuf[MSGOUT_SIZE];

	va_start(args, fmt);
	vsprintf(msgbuf, fmt, args);
	va_end(args);

    printerr("%s (DEERR %d)\n", msgbuf, estat);

	return (estat);
}
/***************************************************************/
int set_error_f(int estat, int ern, const char * fmt, ...)
{

	va_list args;
    char msgbuf[MSGOUT_SIZE];
    char ferrmsg[FERRMSG_MAX + 2];

	va_start(args, fmt);
	vsprintf(msgbuf, fmt, args);
	va_end(args);

    strmcpy(ferrmsg, strerror(ern), FERRMSG_MAX);

    printerr("%s: %s (DEERR %d)\n", msgbuf, ferrmsg, estat);

	return (estat);
}
/***************************************************************/
struct globals * get_globals(void) {

    struct globals * glb;

    glb = g_globals;
    glb->glb_nRefs += 1;

    return (glb);
}
/***************************************************************/
struct globals * init_globals(void) {

    struct globals * glb;

    glb = New(struct globals, 1);
    glb->glb_nRefs      = 0;
    glb->glb_flags      = 0;
    glb->glb_jsflags    = 0;
    glb->glb_nparms     = 0;
    glb->glb_parms      = NULL;
    glb->glb_outfile    = NULL;
    glb->glb_errfile    = NULL;
    glb->glb_hte        = NULL;

    g_globals = glb;

    return (get_globals());
}
/***************************************************************/
void free_parms(struct globals * glb) {

	int ii;

	if (!glb->glb_parms) return;

	for (ii = 0; ii < glb->glb_nparms; ii++) {
		Free(glb->glb_parms[ii]);
	}
	Free(glb->glb_parms);
}
/***************************************************************/
void shut_globals(struct globals * glb) {

	free_parms(glb);
    htFreeEnv(glb->glb_hte);
	Free(glb);
}
/***************************************************************/
void release_globals(struct globals * glb) {

    glb->glb_nRefs -= 1;

    if (!glb->glb_nRefs) {
    	shut_globals(glb);
    }
}
/***************************************************************/
int dew_process_httpstr(
    struct globals * glb,
    enum e_int_file_type in_filtyp,
    const char * httpstr,
    int httplen)
{
/*
**
*/

    int estat = 0;
    struct htmltree * htree;
    char errmsg[ERRMSG_MAX];

    estat = html_get_htmltree_from_string(httpstr,
        &htree, HTMLSTR_FLAG_ENC_UTF_8, errmsg, sizeof(errmsg));
    if (!estat) {
        show_htmltree(htree,
            SHOWFLAG_SCRIPT, EXFLAG_NO_DOCTYPE | EXFLAG_INDENT);
        free_htmltree(htree);
    } else {
        estat = set_error(ESTAT_PROCESS_URL, "Error processing html: %s", errmsg);
    }

    return (estat);
}
/***************************************************************/
int glb_jsflags_to_jenflags(int gjsflags)
{
    int jenflags;

    jenflags = 0;
    if (gjsflags & GLB_JSFLAG_NODEJS) {
        jenflags |= JENFLAG_NODEJS;
    }

    return (jenflags);
}
/***************************************************************/
int dew_process_jsstr(
    struct globals * glb,
    enum e_int_file_type in_filtyp,
    const char * jsstr,
    int jslen)
{
/*
**
*/

    int estat = 0;

    if (!glb->glb_jse) {
        glb->glb_jse = new_jsenv(glb_jsflags_to_jenflags(glb->glb_jsflags));
    }

    return (estat);
}
/***************************************************************/
int dew_process_filestr(
    struct globals * glb,
    enum e_int_file_type in_filtyp,
    const char * filstr,
    int fillen)
{
/*
**
*/
    int estat = 0;

    switch (in_filtyp) {
        case IFILTYP_HTTP:
		    estat = dew_process_httpstr(glb, in_filtyp, filstr, fillen);
            break;
        case IFILTYP_JAVASCRIPT:
		    estat = dew_process_jsstr(glb, in_filtyp, filstr, fillen);
            break;
        default:
            estat = set_error(ESTAT_UNSUPPORTED_FILTYP, "Unsupported file type");
            break;
    }

    return (estat);
}
/***************************************************************/
int read_file( const char * file_name,
                char ** filestring,
                int   * filestringlength)
{
/*
** 08/04/2020
*/
    int estat = 0;
    int ern;
    int flen;
    int done;
    int fstrmax;
    FILE * fref;
    char fbuf[FBUFFER_SIZE];

    (*filestring) = NULL;
    (*filestringlength) = 0;
    done = 0;
    fstrmax = 0;

    fref = fopen(file_name, "r");
    if (!fref) {
        ern = ERRNO;
        estat = set_error_f(ESTAT_OPEN_FILE, ern, "Error opening file %s", file_name);
    }

    while (!done && !estat) {
        flen = (int)fread(fbuf, 1, FBUFFER_SIZE, fref);
        if (flen <= 0) {
            if (feof(fref)) {
                done = 1;
            } else {
                ern = ERRNO;
                estat = set_error_f(ESTAT_READ_FILE, ern, "Error reading file %s", file_name);
            }
        } else {
            if ((*filestringlength) + flen > fstrmax) {
                if (!fstrmax) fstrmax = flen;
                else fstrmax *= 2;
                (*filestring) = Realloc((*filestring), char, fstrmax);
            }
            memcpy((*filestring) + (*filestringlength), fbuf, flen);
            (*filestringlength) += flen;
        }
    }

    return (estat);
}
/***************************************************************/
enum e_int_file_type calc_int_file_type(
    struct globals * glb,
    const char * nam)
{
/*
**
*/
    enum e_int_file_type in_filtyp;
    char ext[8];
    int ii;
    int extix;

    in_filtyp = IFILTYP_NONE;
    ii = IStrlen(nam) - 1;
    extix = sizeof(ext) - 1;
    ext[extix] = '\0';
    while (extix > 0 && ii > 0 && isalnum(nam[ii])) {
        extix--;
        ext[extix] = toupper(nam[ii]);
        ii--;
    }

    if (extix > 0 && ii >= 0 && nam[ii] == '.') {
             if (!strcmp(ext + extix,  "HTTP")) in_filtyp = IFILTYP_HTTP;
        else if (!strcmp(ext + extix,  "HTM" )) in_filtyp = IFILTYP_HTTP;
        else if (!strcmp(ext + extix,  "JS"  )) in_filtyp = IFILTYP_JAVASCRIPT;
    }

    return (in_filtyp);
}
/***************************************************************/
int dew_process_url(struct globals * glb, const char * url) {

    int estat = 0;
    char * httpstr;
    char * emsg;
    int httplen;
    int hterr;
    int ix;
    int scheme;
    enum e_int_file_type in_filtyp;
    
    ix = 0;
    scheme = uri_get_scheme(url, &ix);

    if (!scheme) {
        estat = read_file(url, &httpstr, &httplen);
        if (!estat) in_filtyp = calc_int_file_type(glb, url);
    } else if (scheme == URI_SCHEME_FILE) {
        estat = read_file(url + ix, &httpstr, &httplen);
        if (!estat) in_filtyp = calc_int_file_type(glb, url);
    } else {    
        if (!glb->glb_hte) {
            glb->glb_hte = htNewEnv();
        }

        hterr = ht_get_url(glb->glb_hte, url, NULL, &httpstr, &httplen);
        if (hterr) {
            emsg = ht_get_error_msg(glb->glb_hte, hterr);
		    estat = set_error(ESTAT_GET_URL, "Error getting URL %s: %s",
                url, emsg);
            Free(emsg);
        } else {
            in_filtyp = calc_int_file_type(glb, url);
            if (!in_filtyp) in_filtyp = IFILTYP_HTTP;
        }
    }

    if (!estat) {
		estat = dew_process_filestr(glb, in_filtyp, httpstr, httplen);
        Free(httpstr);
    }

    return (estat);
}
/***************************************************************/
int dew_process(struct globals * glb) {

    int estat = 0;
	int ii;

	for (ii = 0; !estat && ii < glb->glb_nparms; ii++) {
		estat = dew_process_url(glb, glb->glb_parms[ii]);
	}

    return (estat);
}
/***************************************************************/
char * parse_command_line(const char * cmd, int * pargc, const char * arg0, int flags)
{
/*
**  flags   0 - treat backslashes like any character
**          1 - treat backslashes as an escape character
*/
/* see http://msdn.microsoft.com/en-us/library/a1y7w461.aspx */
    int ix;
    int qix;
    int done;
    int nparms;
    int parmix;
    int tlen;
    int size;
    int tix;
    int arg0len;
    char * cargv;
    char ** argv;

    /* pass 1: count parms */
    arg0len = 0;
    nparms = 0;
    tlen = 0;
    ix = 0;
    while (cmd[ix]) {
        while (isspace(cmd[ix])) ix++;
        if (cmd[ix]) {
            qix = -1;
            done = 0;
            while (!done) {
                if (!cmd[ix]) done = 1;
                else if (cmd[ix] == '\"') {
                    if (qix < 0) qix = ix;
                    else qix = -1;
                    ix++;
                }
                else if (isspace(cmd[ix]) && (qix < 0)) {
                    done = 1;
                }
                else {
                    if (cmd[ix] == '\\' && cmd[ix + 1] && (flags & 1)) ix++;
                    tlen++;
                    ix++;
                }
            }
            tlen++; /* for '\0' terminator */
            nparms++;
        }
    }

    if (arg0) {
        nparms++;
        arg0len = IStrlen(arg0) + 1;
        tlen += arg0len;
    }

    tix = sizeof(char*) * (nparms + 1);
    size = tix + tlen;
    cargv = New(char, size);
    argv = (char**)cargv;
    parmix = 0;

    if (arg0) {
        argv[parmix] = cargv + tix;
        memcpy(cargv + tix, arg0, arg0len);
        tix += arg0len;
        parmix++;
    }

    /* pass 2: move parms */
    ix = 0;
    while (cmd[ix]) {
        while (isspace(cmd[ix])) ix++;
        if (cmd[ix]) {
            qix = -1;
            argv[parmix] = NULL;
            done = 0;
            while (!done) {
                if (!cmd[ix]) done = 1;
                else if (cmd[ix] == '\"') {
                    if (qix < 0) qix = ix;
                    else qix = -1;
                    ix++;
                }
                else if (isspace(cmd[ix]) && (qix < 0)) {
                    done = 1;
                }
                else {
                    if (cmd[ix] == '\\' && cmd[ix + 1] && (flags & 1)) ix++;
                    if (!argv[parmix]) argv[parmix] = cargv + tix;
                    cargv[tix++] = cmd[ix];
                    ix++;
                }
            }
            if (!argv[parmix]) argv[parmix] = cargv + tix;
            cargv[tix++] = '\0';
            parmix++;
        }
    }
    argv[nparms] = NULL;

    (*pargc) = nparms;

    return (cargv);
}
/***************************************************************/
int getargs(int argc, char *argv[], struct globals * glb);

int parse_parmfile(struct globals * glb, const char * parmfname)
{

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

    pfile = NULL;
	depth++;
	if (depth > 8) {
		estat = set_error(ESTAT_PARMDEPTH, "-parms depth exceeds maximum of 8.");
	} else {
		pfile = fopen(parmfname, "r");
		if (!pfile) {
			int ern = ERRNO;
			estat = set_error(ESTAT_PARMFILE,
                "Error #%d opening parms file %s: %s",
                ern, parmfname, strerror(ern));
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
				}
				else {
					int ern = ERRNO;
					estat = set_error(ESTAT_PARMFGETS,
                        "Error reading parms file %s in line %ld: %m",
						parmfname, linenum, ern);
				}
			}
			else {
				ix = IStrlen(fbuf);
				while (ix > 0 && isspace(fbuf[ix - 1])) ix--;
				fbuf[ix] = '\0';
				ix = 0;
				while (isspace(fbuf[ix])) ix++;
				if (fbuf[ix] && fbuf[ix] != '#') {
					parmbuf = parse_command_line(fbuf, &parmc, "", 0);
					if (parmbuf && parmc) {
						argv = (char**)parmbuf;
						estat = getargs(parmc, argv, glb);
						if (estat) {
							estat = set_error(ESTAT_PARM,
                                "Error in line %ld of parm file %s",
								linenum, parmfname);
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
int getargs(int argc, char *argv[], struct globals * glb)
{

    int ii;
    int estat = 0;

    for (ii = 1; !estat && ii < argc; ii++) {
        if (argv[ii][0] == '-') {
            if (!Stricmp(argv[ii], "-p") || !Stricmp(argv[ii], "-pause")) {
                glb->glb_flags |= GLB_FLAG_PAUSE;
            } else if (!Stricmp(argv[ii], "-?") || !Stricmp(argv[ii], "-help")) {
                glb->glb_flags |= GLB_FLAG_HELP | GLB_FLAG_DONE;
            } else if (!Stricmp(argv[ii], "-version")) {
                glb->glb_flags |= GLB_FLAG_VERSION | GLB_FLAG_DONE;
            } else if (!Stricmp(argv[ii], "-showmem")) {
                glb->glb_flags |= GLB_FLAG_SHOWMEM;
            } else if (!Stricmp(argv[ii], "-jsuse")) {
			    ii++;
			    if (ii < argc || argv[ii][0] == '-') {
				    if (!Stricmp(argv[ii], "nodejs")) {
                        glb->glb_jsflags |= GLB_JSFLAG_NODEJS;
                    } else {
				        estat = set_error(ESTAT_BAD_JSUSE,
                            "Invalid js type after %s", argv[ii - 1]);
                    }
			    } else {
				    estat = set_error(ESTAT_BAD_JSUSE,
                        "Expected js type after %s", argv[ii - 1]);
                }

		    } else if (!Stricmp(argv[ii], "-parms")) {
			    ii++;
			    if (ii < argc || argv[ii][0] == '-') {
				    estat = parse_parmfile(glb, argv[ii]);
			    } else {
				    estat = set_error(ESTAT_PARMEXPFNAME,
                        "Expected file name after %s", argv[ii - 1]);
                }
                   
            } else {
                estat = set_error(ESTAT_PARMBAD, "Unrecognized option %s", argv[ii]);
            }
		} else {
			glb->glb_parms = Realloc(glb->glb_parms, char*, glb->glb_nparms + 1);
			glb->glb_parms[glb->glb_nparms] = Strdup(argv[ii]);
			glb->glb_nparms += 1;
		}
    }

    return (estat);
}
/***************************************************************/
int main(int argc, char *argv[])
{

    int estat;
    int gflags;
    struct globals * glb;

    if (argc <= 1) {
        showusage(argv[0]);
        return (0);
    }

    glb = init_globals();

    estat = getargs(argc, argv, glb);

    gflags = glb->glb_flags;

    if (gflags & GLB_FLAG_HELP) {
        showusage(argv[0]);
    }

    if (gflags & GLB_FLAG_VERSION) {
        printf("%s - %s\n", PROGRAM_NAME, VERSION);
    }

    if (!estat && !(gflags & GLB_FLAG_DONE)) {
        estat = dew_process(glb);
    }

    release_globals(glb);

    if ((gflags & GLB_FLAG_SHOWMEM) && CMEM_TYPE) {
        if (get_unfreed_mem() == 0) {
            printf("---- All memory freed ----\n");
        } else {
            printf("**** %d bytes of memory not freed ----\n", get_unfreed_mem());
            print_mem_stats(DEWMEM_FNAME);
            printf("**** Memory stats in %s ****\n", DEWMEM_FNAME);
        }
    }

    if (gflags & GLB_FLAG_PAUSE) {
        char inbuf[100];
        printf("Press ENTER to continue...");
        fgets(inbuf, sizeof(inbuf), stdin);
    }

    return (estat);
}
/***************************************************************/