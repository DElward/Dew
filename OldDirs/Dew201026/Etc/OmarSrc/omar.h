/***************************************************************/
/* omar.h                                                      */
/***************************************************************/

#include "cmemh.h"

#define MAJOR_VERSION "0"
#define MINOR_VERSION "00"
#define VERSION MAJOR_VERSION "." MINOR_VERSION
#define PROGRAM_NAME "omar"

#define TOKEN_MAX       256

#define Strdup(s) (strcpy(New(char,strlen(s)+1),(s)))

#if IS_WINDOWS
    #define GETCWD      _getcwd
    #define LSTAT        stat
#else
    #define GETCWD      getcwd
    #define LSTAT       lstat
#endif

#define CHARLIST_CHUNK 64
#define SMTOKESIZE     64

#define ESTAT_ERROR         (-1)
#define ESTAT_EOF           (1)
#define ESTAT_WARN          (2)

#define OG_FLAG_VERBOSE     1
#define OG_FLAG_TRACE       2
#define OG_FLAG_DEBUG       4
#define OG_FLAG_QUIET       8

struct omarglobals {    /* og_ */
    FILE *              og_outfile;
    int                 og_nerrs;
    int                 og_nwarns;
    int                 og_flags;
    struct handrec **   og_ahandrec;
    int                 og_nhandrec;
    int                 og_xhandrec;
    struct nndata     * og_nnd;
    struct nntesthand ** og_nnthlist;
};

/***************************************************************/
int set_error(struct omarglobals * og, const char * fmt, ...);
int set_warning(struct omarglobals * og, const char * fmt, ...);
int omarerr(struct omarglobals * og, const char * fmt, ...);
int omarerr_v(struct omarglobals * og, const char * fmt, va_list args);
int omarwarn_v(struct omarglobals * og, const char * fmt, va_list args);
void omarout(struct omarglobals * og, const char * fmt, ...);
FILE * get_outfile(struct omarglobals * og);
struct omarglobals * get_omarglobals(void);
int omargeterr(struct omarglobals * og);
/***************************************************************/

