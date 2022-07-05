/***************************************************************/
/* exp. h                                                      */
/***************************************************************/

#define VTYP_NONE       0
#define VTYP_STR        1
#define VTYP_INT        2
#define VTYP_BOOL       4
#define VTYP_NULL       5 /* used for partial expression evaluation */

struct var {
    int         var_typ;
    VITYP       var_val;
    char *      var_buf;
    int         var_len;
    int         var_max;
};
/***************************************************************/
int stoi(const char * from, int *dest);

int get_token_exp(struct cenv * ce,
            const char * expbuf,
            int * pix,
            struct str * tokestr);
void vensure_str(struct cenv * ce, struct var * vv);
int vensure_bool(struct cenv * ce, struct var * vv);
void qcopy_var(struct var * tgtvar, struct var * srcvar);
void free_var(struct var * srcvar);
void free_var_buf(struct var * vv);
void append_str_var(struct cenv * ce, struct str * ss, struct var * vv);
void copy_var(struct var * tgtvar, struct var * srcvar);
void replace_var(struct var * tgtvar, struct var * srcvar);
void str_to_var(struct var * tgtvar, const char * srcbuf, int srclen);
struct var * dup_var(struct var * srcvar);

int set_var_exp(struct cenv * ce,
            const char * cmdbuf,
            int * pix,
            struct str * parmstr);
int parse_for_csvparse(struct cenv * ce,
            struct nest * ne,
            struct str * cmdstr,
                int * cmdix,
            struct str * parmstr);
int parse_for_in(struct cenv * ce,
                    struct nest * ne,
                    struct str * cmdstr,
                int * cmdix,
                    struct parmlist * pl);
int parse_for_range(struct cenv * ce,
            struct nest * ne,
            struct str * cmdstr,
                int * cmdix,
            struct str * parmstr);
int parse_for_parse(struct cenv * ce,
            struct nest * ne,
            struct str * cmdstr,
                int * cmdix,
            struct str * parmstr);
int parse_for_read(struct cenv * ce,
            struct nest * ne,
            struct str * cmdstr,
                int * cmdix,
            struct str * parmstr);
int parse_for_readdir(struct cenv * ce,
            struct nest * ne,
            struct str * cmdstr,
                int * cmdix,
            struct str * parmstr);
int parse_exp(struct cenv * ce,
            const char * cmdbuf,
            int * pix,
            struct str * parmstr,
            struct var * result);
/***************************************************************/
