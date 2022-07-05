#ifndef UTILH_INCLUDED
#define UTILH_INCLUDED
/***************************************************************/
/* util.h                                                      */
/***************************************************************/
struct frec {
    FILE * fref;
    unsigned char * fbuf;
    char * fname;
    int    fbufsize;
    int    fbuflen;
    int    flinenum;
    int    feof;
};
#define LINE_MAX_SIZE   1024
#define LINE_MAX_LEN    (LINE_MAX_SIZE - 1)

#define FROM_B64_LEN(b64len) (((b64len) + 3) / 4 * 3)

#define GETTOKE_FLAG_UPSHIFT            1
#define GETTOKE_FLAG_C_BACKSLASH        2
#define GETTOKE_FLAG_APOSTROPHE_QUOTES  4

struct timestamp {
    int ts_daynum;
    int ts_second;
    int ts_microsec;
};
struct proctimes {
    struct timestamp pt_wall_time;
    struct timestamp pt_cpu_time;
    struct timestamp pt_sys_time;
};

struct strlist {
    char ** strl_strs;
    int strl_len;
    int strl_max;
    int strl_ptr_count;
    int strl_last_buflen;
    int strl_last_bufmax;
};

#define STRCPYX_REMOVE_QUOTES   1
/* typedef wchar_t tWCHAR; */

/***************************************************************/
void strmcpy(char * tgt, const char * src, size_t tmax);
char * parse_command_line(const char * cmd, int * pargc, const char * arg0, int flags);
int istrcmp(const char * b1, const char * b2);

struct frec * frec_open(const char * fname, const char * fmode, const char * ftyp);
int frec_close(struct frec * fr);
int frec_gets(char * line, int linemax, struct frec * fr);
int frec_puts(const char * line, struct frec * fr);
int frec_printf(struct frec * fr, char * fmt, ...);

int from_base64_c(
    char * bbuf,
    const char * b64,
    int b64len,
    int b64flags);
void free_charlist(char ** charlist);
void append_chars(char ** cstr,
                    int * slen,
                    int * smax,
                    const char * val,
                    int vallen);
void append_chars_rept(char ** cstr,
                    int * slen,
                    int * smax,
                    char val,
                    int vallen);
int any_escapes(const char * val);
char * escape_text(const char * val);

int char_to_integer(const char * numbuf, int * num);
void get_toke_char (const char * bbuf,
                         int * bbufix,
                         char *toke,
                         int tokemax,
                         int gt_flags);
#define CRAIGS_MINUS    '\x12'
int string_to_double(const char * dblbuf, double * answer);
void double_to_string(char * nbuf, double dval, int nbufsz);
void start_cpu_timer(struct proctimes * pt_start);
void stop_cpu_timer(struct proctimes * pt_start,
                    struct proctimes * pt_end,
                    struct proctimes * pt_result);
int format_timestamp(struct timestamp * ts, const char * fmt, char *buf, int bufsize);
int parse_timestamp(const char* dbuf,
                int *ix,
                const char * fmt,
                struct timestamp * ts);
char * add_string_timestamps(const char* tsbuf1,
                const char * tsbuf2,
                const char * fmt);
int Memicmp(const char * b1, const char * b2, size_t blen);
void strlist_append_str(struct strlist * strl, const char * buf);
void strlist_append_ptr(struct strlist * strl, char * buf, int buflen);
void strlist_free(struct strlist * strl);
struct strlist * strlist_new(void);
void strlist_append_str_to_last(struct strlist * strl, const char * buf, int linemax);
void strlist_print(struct strlist * strl);
char * strdupx(const char * csrc, int cflags);

/***************************************************************/
#endif /* UTILH_INCLUDED */
