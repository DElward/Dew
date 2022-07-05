/***************************************************************/
/* help.h                                                      */
/***************************************************************/

#define FOUTFIL_INIT_TAB_MAX    8
#define FOUTFIL_DEFAULT_RMARGIN 80


#define HELPFLAG_VERBOSE    1
#define HELPFLAG_FORCE      2
#define HELPFLAG_C          4
#define HELPFLAG_SEARCH     8
#define HELPFLAG_HTML       16
#define HELPFLAG_CONTINUE   32
#define HELPFLAG_KEEP       64
#define HELPFLAG_TESTONLY   128
#define HELPFLAG_PAGE       256
#define HELPFLAG_WHERE      512

#define HELP_LINE_OUT_LEN   80 /* Approximate maximum */

#define HELP_LINE_SIZE      256
#define HELP_SPACES_FOR_TAB 3

#define HELP_C_CHAR_PFX     "chelp_text_"

/***************************************************************/
int chndlr_help(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix);
void free_help(void);
/***************************************************************/
