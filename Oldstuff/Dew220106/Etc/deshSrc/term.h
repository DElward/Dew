/***************************************************************/
/* term.h                                                     */
/***************************************************************/

#define TERM_ESC_CD_MAX  16

#if ! IS_WINDOWS
    #include <unistd.h>
    #include <termios.h>
    #include <sys/ioctl.h>

    #define TERM_CH_BUF_SIZE 16

    #define TERM_ESC_CDX_LEFT				0
    #define TERM_ESC_CDX_RIGHT				1
    #define TERM_ESC_CDX_UP					2
    #define TERM_ESC_CDX_DOWN				3
    #define TERM_ESC_CDX_HOME				4
    #define TERM_ESC_CDX_END 				5
    #define TERM_ESC_CDX_LEFT_DEL 			6
    #define TERM_ESC_CDX_RIGHT_DEL			7
    #define TERM_ESC_CDX_GOTO_BOL			8
    #define TERM_ESC_CDX_GOTO_1UP_RMARGIN	9

    #define TERM_NUM_ESC_CDS 		(TERM_ESC_CDX_GOTO_1UP_RMARGIN + 1)
#endif

#define TERMFLAG_NO_HISTORY 1
#define TERMFLAG_DEBUG      2
#define TERMFLAG_VERBOSE    4

#define TERMFLAG_MASK       0xFF

#define TERMTYP_FLAG_DEL_LEFT_DEL			1
#define TERMTYP_FLAG_ESC_RIGHT_DEL          2
#define TERMTYP_FLAG_XCODE_RIGHT_DEL        4

#define TERMTYP_FLAG_EOL_WRAP               8   /* Cursor wraps at EOL - default */
#define TERMTYP_FLAG_EOL_WRAP_NEXT         16   /* Cursor wraps at next - 'b' goes to eol - 1 */
#define TERMTYP_FLAG_EOL_VIRTUAL_NEXT      32   /* Cursor waits for next  - 'b' goes to eol */
#define TERMTYP_FLAG_EOL_REMAIN            64   /* Cursor remains at EOL */

#define TERMTYP_FLAG_ECHO                 128  /* Terminal echosL */

#define TERM_ERRMSG_MAX  127

struct term {
	int         term_in_fnum;
    int         term_out_fnum;
    int         term_flags;
    int         term_type_flags;
    int         term_new_in_lmode;	/* l local mode - Ux & Windows*/
    int         term_new_out_lmode;
    int         term_old_in_lmode;
    int         term_old_out_lmode;
    char *      term_inbuf;
    int         term_inbuf_len;
    int         term_inbuf_max;
    int         term_cursor_info1;
    int         term_cursor_info2;
    int         term_cursor_ix;
    int         term_screen_cursor_x;
    int         term_screen_width;
    char **     term_hist;
    int         term_hist_adv;
    int         term_hist_ix;
    int         term_hist_len;
    int         term_hist_max;
    void        (*term_sighndlr)(int);
    int         (*termInit)(struct term * tty);
    int         (*termShut)(struct term * tty);
    int         (*termBeginReadLine)(struct term * tty);
    int         (*termEndReadLine)(struct term * tty);
    int         (*termReadChar)(struct term * tty, int * rdch);
    int         (*termHandleKey)(struct term * tty, int rdch);
    int         (*termWriteChars)(struct term * tty, const char * outbuf, int outlen);
    int         (*termReadLine)(struct term * tty, char * prompt, char ** inbuf, int * inlen);
    int         (*termReadLineContinuation)(struct term * tty, char * prompt, char ** inbuf, int * inlen);
    int         (*termAddHistoryLine)(struct term * tty, const char * inbuf);
    int         (*termSetCursor)(struct term * tty, int insert_cursor);
    int         (*termSetFlags)(struct term * tty, int tFlags);
    int         (*termClearFlags)(struct term * tty, int tFlags);
    int         (*termUpdateLine)(struct term * tty, const char * outbuf);
    int         (*termSetErrMsg)(struct term * tty, int terrn, int syserr);
    char		term_errmsg[TERM_ERRMSG_MAX + 1];

#if ! IS_WINDOWS
    char		term_esc_code[TERM_NUM_ESC_CDS][TERM_ESC_CD_MAX];
    int         term_ch_buflen;
    int         term_ch_bufix;
    char		term_ch_buf[TERM_CH_BUF_SIZE];
    struct termios term_in_tios;
    struct termios term_out_tios;
#endif

    /* int         (*termSetCheckInterrupt)(struct term * tty, int (*intrChecker)(void)); */
    /* int         (*termIntrChecker)(void); */
};

#if IS_WINDOWS
    #define ISATTY      _isatty
    #define READ        _read
    #define WRITE       _write
    #define DUP         _dup
    #define CLOSE       _close
#else
    #define ISATTY      isatty
    #define READ        read
#ifndef DUP
    #define DUP         dup
#endif
#ifndef CLOSE
	#define CLOSE       close
#endif
#ifndef WRITE
    #define WRITE         write
#endif
#endif

#define TERMREAD_EOF        1
#define TERMREAD_INTR       2

#ifndef New
    #define New(t,n)         ((t*)calloc(sizeof(t),(n)))
    #define Realloc(p,t,n)   ((t*)realloc((p),sizeof(t)*(n)))
    #define Free(p)           free(p)
    #define Strdup(s)        (strcpy(New(char,strlen(s)+1),(s)))
#endif

/***************************************************************/
#define TERMKEY_ENTER       0x0100
#define TERMKEY_UP          0x0101
#define TERMKEY_DOWN        0x0102
#define TERMKEY_LEFT        0x0103
#define TERMKEY_RIGHT       0x0104
#define TERMKEY_RIGHT_DEL   0x0105
#define TERMKEY_LEFT_DEL    0x0106
#define TERMKEY_HOME        0x0107
#define TERMKEY_END         0x0108
#define TERMKEY_INTR        0x0109

#define ASCII_CTL_C			3
#define ASCII_CTL_D			4
#define ASCII_BACKSPACE		8
#define ASCII_TAB			9
#define ASCII_LINE_FEED		10
#define ASCII_RETURN		13
#define ASCII_ESCAPE		27
#define ASCII_SPACE			32
#define ASCII_DELETE		127

#define PRINTING_CHAR(c) ((c) >= ' ' && (c) <= '~')

#define TERRN_INIT          (-9001)
#define TERRN_WRITE         (-9002)
#define TERRN_SET_POS       (-9003)
#define TERRN_READ          (-9004)
#define TERRN_SET_INFO      (-9005)
#define TERRN_GET_POS       (-9006)
#define TERRN_GET_STATS     (-9007)

#if IS_WINDOWS
    #define TERMKEY_EOF         26      /* ctl-Z */
#else
    #define TERMKEY_EOF         4       /* ctl-D */
#endif

#define TERMKEY_MASK        0xFF00

int os_sleep(int millisecs);

struct term * new_term(int in_fnum, int out_fnum, int term_flags);
void free_term(struct term * tty);
char * term_errmsg(struct term * tty, int terrn);
/***************************************************************/
