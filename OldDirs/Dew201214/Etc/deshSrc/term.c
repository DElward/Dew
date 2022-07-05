/***************************************************************/
/* term.c                                                      */
/***************************************************************/
/*
 * To do:
 *
 * 		TERMKEY_LEFT
 * 		TERMKEY_RIGHT
 * 		TERMKEY_RIGHT_DEL
 * 		TERMKEY_INTR
 *
 * Done:
 * 		TERMKEY_ENTER		- Uses ASCII_LINE_FEED
 * 		TERMKEY_UP
 * 		TERMKEY_DOWN
 * 		TERMKEY_LEFT_DEL	- Uses ASCII_BACKSPACE, special wrap at BOL
 * 		TERMKEY_HOME		- Could not find escape documentation
 * 		TERMKEY_END			- Could not find escape documentation

****************************************************************
*/
#include "config.h"

#if IS_WINDOWS
    #include <windows.h>
    #include <io.h>
#else
    #include <time.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>

#include "term.h"

#define DEBUG_TERMINAL	0

/* Clear screen in Windows. See: http://support.microsoft.com/kb/99261 */

/***************************************************************/
/***************************************************************/
int os_sleep(int millisecs) {
    
	int rtn;
    
#if IS_WINDOWS
    /* Windows sleep */
    Sleep(millisecs);
    rtn = 0;
#else
    /* Unix sleep */
    
    struct timespec requested_time;
    struct timespec remaining_time;
    
    requested_time.tv_sec  = millisecs / 1000;
    requested_time.tv_nsec = (millisecs % 1000) * 1000000L;
    
    remaining_time.tv_sec  = 0;
    remaining_time.tv_nsec = 0;
    
    nanosleep(&requested_time, &remaining_time);
    
    rtn = (int)((remaining_time.tv_sec * 1000) +
                    (remaining_time.tv_nsec / 1000000L));
#endif
    
    return (rtn);
}
/***************************************************************/
#if IS_WINDOWS
/***************************************************************/
int termWinSetErrMsg(struct term * tty, int terrn, int syserr)
{
    LPVOID lpMsgBuf;

    if (tty && syserr > 0) {
	    FormatMessage(
	        FORMAT_MESSAGE_ALLOCATE_BUFFER |
	        FORMAT_MESSAGE_FROM_SYSTEM |
	        FORMAT_MESSAGE_IGNORE_INSERTS,
	        NULL,
	        syserr,
	        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
	        (LPTSTR) &lpMsgBuf,
	        0,
	        NULL
	    );
		strncpy(tty->term_errmsg, lpMsgBuf, TERM_ERRMSG_MAX);
		tty->term_errmsg[TERM_ERRMSG_MAX] = '\0';

	    LocalFree( lpMsgBuf );
	}

    return (terrn);
}
/***************************************************************/
int termSetConsoleMode(struct term * tty, int isout, HANDLE hConsoleHandle, DWORD dwMode)
{
    int terrn;
    BOOL bSuccess;

    terrn = 0;

    bSuccess = SetConsoleMode(hConsoleHandle, dwMode);

    if (tty->term_flags & TERMFLAG_DEBUG) {
        if (!bSuccess) {
            fprintf(stderr, "SetConsoleMode failure %d\n", GetLastError());
        } else {
            fprintf(stderr, "SetConsoleMode(%s 0x%x)\n", isout?"Out":"In", dwMode);
        }
    }

    return (terrn);
}
/***************************************************************/
int termGetConsoleMode(struct term * tty, int isout, HANDLE hConsoleHandle, DWORD * dwMode)
{
    int terrn;
    BOOL bSuccess;

    terrn = 0;

    bSuccess = GetConsoleMode(hConsoleHandle, dwMode);

    if (tty->term_flags & TERMFLAG_DEBUG) {
        if (!bSuccess) {
            fprintf(stderr, "GetConsoleMode failure %d\n", GetLastError());
        } else {
            fprintf(stderr, "GetConsoleMode(%s)=0x%x\n", isout?"Out":"In", *dwMode);
        }
    }

    return (terrn);
}
/***************************************************************/
int termWinGetScreenStats(struct term * tty)
{
    HANDLE hStdOut;
    CONSOLE_SCREEN_BUFFER_INFO curInf;
    COORD curSiz;
    BOOL bSuccess;
    int terrn;

    terrn = 0;

    hStdOut = (HANDLE)_get_osfhandle(tty->term_out_fnum);
    bSuccess = GetConsoleScreenBufferInfo(hStdOut, &curInf);
    if (bSuccess) {
        curSiz                      = curInf.dwSize;
        tty->term_screen_width      = curSiz.X;
        curSiz                      = curInf.dwCursorPosition;
        tty->term_screen_cursor_x   = curSiz.X;
    }

    return (terrn);
}
/***************************************************************/
int termWinBeginReadLine(struct term * tty)
{
/*
** This is necessary to handle Control-C.  The problem is with
** ReadConsoleInput().  ReadConsoleInput() won't return when ctl-C is pressed
** unless ENABLE_PROCESSED_INPUT is off.  But when ENABLE_PROCESSED_INPUT
** is off, ctl-C won't do anything (no signal) when the rest of the program
** is busy.
**
** Note: When SetConsoleCtrlHandler() is used, a ctl-C calls the handler
** during ReadConsoleInput(), but ReadConsoleInput() still won't return to
** the caller.
*/
    int terrn;
    HANDLE hStdIn;
    DWORD dwInMode;

    tty->term_sighndlr = signal(SIGINT, NULL);

    hStdIn = (HANDLE)_get_osfhandle(tty->term_in_fnum);
    if (hStdIn) {
        dwInMode = tty->term_new_in_lmode;
        terrn = termSetConsoleMode(tty, 0, hStdIn, dwInMode);
    }

    terrn = termWinGetScreenStats(tty);

    return (terrn);
}
/***************************************************************/
int termWinEndReadLine(struct term * tty)
{
    int terrn;
    HANDLE hStdIn;
    DWORD dwInMode;

    terrn = 0;

    hStdIn = (HANDLE)_get_osfhandle(tty->term_in_fnum);
    if (hStdIn) {
        dwInMode = tty->term_old_in_lmode;
        terrn = termSetConsoleMode(tty, 0, hStdIn, dwInMode);
    }

    tty->term_sighndlr = signal(SIGINT, tty->term_sighndlr);

    terrn = tty->termSetCursor(tty, 0);

    return (terrn);
}
/***************************************************************/
/*
BOOL WINAPI CtrlHandler(DWORD fdwCtrlType) {

    fprintf(stderr, "\nControl-C CtrlHandler()\n"); fflush(stderr);

    return (TRUE);
}
*/
/***************************************************************/
int termWinInit(struct term * tty) {

    BOOL bSuccess;
    HANDLE hStdIn;
    HANDLE hStdOut;
    DWORD dwMode;
    CONSOLE_CURSOR_INFO ConsoleCursorInfo;
    int terrn;

    terrn = 0;

    hStdIn = (HANDLE)_get_osfhandle(tty->term_in_fnum);
    terrn = termGetConsoleMode(tty, 0, hStdIn, &dwMode);
    if (!terrn) {
        tty->term_old_in_lmode = dwMode;
        tty->term_new_in_lmode =
            (dwMode & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT)) |
                ENABLE_WINDOW_INPUT;
    }

    hStdOut = (HANDLE)_get_osfhandle(tty->term_out_fnum);
    terrn = termGetConsoleMode(tty, 1, hStdOut, &dwMode);
    if (!terrn) {
        tty->term_old_out_lmode = dwMode;
        tty->term_new_out_lmode = dwMode | ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT;
        terrn = termSetConsoleMode(tty, 1, hStdOut, (DWORD)tty->term_new_out_lmode);
    }

    bSuccess = GetConsoleCursorInfo(hStdOut, &ConsoleCursorInfo);
    if (bSuccess) {
        tty->term_cursor_info1          = ConsoleCursorInfo.dwSize;
        tty->term_cursor_info2          = ConsoleCursorInfo.bVisible;
    }

    terrn = termWinGetScreenStats(tty);

    /* bSuccess = SetConsoleCtrlHandler(CtrlHandler, TRUE);    */

    return (terrn);
}
/***************************************************************/
int termWinShut(struct term * tty) {

    HANDLE hStdIn;
    HANDLE hStdOut;
    DWORD dwInMode;
    DWORD dwOutMode;
    int terrn;

    terrn = 0;

/*
    fprintf(stderr, "termWinShut(%s)=0x%x\n", 0?"Out":"In", tty->term_old_in_lmode);
    fprintf(stderr, "termWinShut(%s)=0x%x\n", 1?"Out":"In", tty->term_old_out_lmode);
*/
    hStdIn = (HANDLE)_get_osfhandle(tty->term_in_fnum);
    if (hStdIn) {
        dwInMode = tty->term_old_in_lmode;
        terrn = termSetConsoleMode(tty, 0, hStdIn, dwInMode);
    }

    hStdOut = (HANDLE)_get_osfhandle(tty->term_out_fnum);
    if (hStdOut) {
        dwOutMode = tty->term_old_out_lmode;
        terrn = termSetConsoleMode(tty, 1, hStdOut, dwOutMode);
    }

    return (terrn);
}
/***************************************************************/
int termWinWriteChars(struct term * tty, const char * outbuf, int outlen)
{
    int terrn;
    HANDLE hStdOut;
    DWORD dwWritten;
    BOOL bSuccess;
    int ii;
    int outix;
    int wlen;
    CONSOLE_SCREEN_BUFFER_INFO curInf;
    COORD curPos;

    terrn = 0;
    hStdOut = (HANDLE)_get_osfhandle(tty->term_out_fnum);

    outix = 0;
    for (ii = 0; ii < outlen; ii++) {
        if (outbuf[ii] >= ' ' && !(outbuf[ii] & TERMKEY_MASK)) {
            tty->term_cursor_ix += 1;
            tty->term_screen_cursor_x += 1;
            if (tty->term_screen_cursor_x >= tty->term_screen_width) {
                tty->term_screen_cursor_x -= tty->term_screen_width;
            }
        } else if (outbuf[ii] == '\r' || outbuf[ii] == '\n') {
            tty->term_cursor_ix = 0;
        } else if (outbuf[ii] == '\b' ) {
            tty->term_cursor_ix -= 1;
            tty->term_screen_cursor_x -= 1;
            if (tty->term_screen_cursor_x < 0) {
                /*
                ** This is all needed because backspace does not wrap backward */
                tty->term_screen_cursor_x = tty->term_screen_width - 1;

                wlen = ii - outix;
                if (wlen) {
                    bSuccess = WriteConsole(hStdOut, outbuf + outix, wlen, &dwWritten, NULL);
                    if (!bSuccess) {
                        terrn = TERRN_WRITE;
                    }
                }
                bSuccess = GetConsoleScreenBufferInfo(hStdOut, &curInf);
                if (!bSuccess) {
                    terrn = TERRN_GET_POS;
                } else {
                    curPos = curInf.dwCursorPosition;
                    curPos.Y -= 1;
                    curPos.X  = tty->term_screen_cursor_x;
                    bSuccess = SetConsoleCursorPosition(hStdOut, curPos);
                    if (!bSuccess) {
                        terrn = TERRN_SET_POS;
                    }
                }
                outix = ii + 1;
            }
        }
    }

    wlen = outlen - outix;
    if (wlen) {
        bSuccess = WriteConsole(hStdOut, outbuf + outix, wlen, &dwWritten, NULL);
        if (!bSuccess) {
            terrn = TERRN_WRITE;
        }
    }

    return (terrn);
}
/***************************************************************/
int termWinReadChar(struct term * tty, int * rdch)
{
/*
    For timeout:

        if (WaitForSingleObject(hStdin, 5000) == WAIT_OBJECT_0) {
            // Input (keyboard and/or mouse, depending on console mode) is available...
            ReadConsoleInput(hStdin,irInBuf,128, &cNumRead);
        }  
*/
    int terrn;
    int done;
    HANDLE hStdIn;
    BOOL bSuccess;
    INPUT_RECORD inputBuffer;
    DWORD dwInputEvents; /* number of events actually read */

    terrn = 0;

    hStdIn = (HANDLE)_get_osfhandle(tty->term_in_fnum);

    done = 0;
    while (!done) {
        bSuccess = ReadConsoleInput(hStdIn, &inputBuffer, 1, &dwInputEvents);
        if (!bSuccess) {
            done = 1;
            terrn = TERRN_READ;
#if IS_DEBUG
        } else if (inputBuffer.EventType == FOCUS_EVENT) {
            /* do nothing */
#endif
        } else {
            if (inputBuffer.EventType == WINDOW_BUFFER_SIZE_EVENT) {
                terrn = termWinGetScreenStats(tty);
                if (terrn) done = 1;
            } else {
                if (inputBuffer.EventType == KEY_EVENT && inputBuffer.Event.KeyEvent.bKeyDown) {
                    done = 1;
                }
            }
        }
    }

    if (!terrn) {
        if (inputBuffer.Event.KeyEvent.wVirtualKeyCode == VK_RETURN) {
            (*rdch) = TERMKEY_ENTER;
        } else if (inputBuffer.Event.KeyEvent.wVirtualKeyCode == VK_UP) {
            (*rdch) = TERMKEY_UP;
        } else if (inputBuffer.Event.KeyEvent.wVirtualKeyCode == VK_DOWN) {
            (*rdch) = TERMKEY_DOWN;
        } else if (inputBuffer.Event.KeyEvent.wVirtualKeyCode == VK_LEFT) {
            (*rdch) = TERMKEY_LEFT;
        } else if (inputBuffer.Event.KeyEvent.wVirtualKeyCode == VK_RIGHT) {
            (*rdch) = TERMKEY_RIGHT;
        } else if (inputBuffer.Event.KeyEvent.wVirtualKeyCode == VK_DELETE) {
            (*rdch) = TERMKEY_RIGHT_DEL;
        } else if (inputBuffer.Event.KeyEvent.wVirtualKeyCode == VK_BACK) {
            (*rdch) = TERMKEY_LEFT_DEL;
        } else if (inputBuffer.Event.KeyEvent.wVirtualKeyCode == VK_HOME) {
            (*rdch) = TERMKEY_HOME;
        } else if (inputBuffer.Event.KeyEvent.wVirtualKeyCode == VK_END) {
            (*rdch) = TERMKEY_END;
        } else if (inputBuffer.Event.KeyEvent.uChar.AsciiChar == 3) {
            (*rdch) = TERMKEY_INTR;
        } else {
            (*rdch) = inputBuffer.Event.KeyEvent.uChar.AsciiChar;
        }
    }

    return (terrn);
}
/***************************************************************/
int termWinSetCursor(struct term * tty, int insert_cursor)
{
    BOOL bSuccess;
    HANDLE hStdOut;
    CONSOLE_CURSOR_INFO ConsoleCursorInfo;
    int terrn;

    terrn = 0;

    hStdOut = (HANDLE)_get_osfhandle(tty->term_out_fnum);

    if (insert_cursor) {
        ConsoleCursorInfo.dwSize = 100; /* 100 percent of character cell */
    } else {
        ConsoleCursorInfo.dwSize = tty->term_cursor_info1;
    }
    ConsoleCursorInfo.bVisible = tty->term_cursor_info2;

    bSuccess = SetConsoleCursorInfo(hStdOut, &ConsoleCursorInfo);
    if (!bSuccess) {
        terrn = TERRN_SET_INFO;
    }

    return (terrn);
}
/***************************************************************/
#else
/***************************************************************/
/* Ux term functions                                           */
/***************************************************************/
void termShowbuf(struct term * tty,
		const char * msg,
		const char * buf,
		int buflen)
{
	int ii;
    char ch;

	if (msg && msg[0]) {
		fprintf(stdout, "%s", msg);
	}
	for (ii = 0; ii < buflen; ii++) {
        ch = buf[ii];
		if (ch >= ' ' && ch <= '~') {
			fprintf(stdout, "%c", ch);
		} else if (ch <= 26) {
			fprintf(stdout, "^%c", 64 + ch);
		} else if (ch == 27) {
			fprintf(stdout, "Esc");
		} else if (ch == 127) {
			fprintf(stdout, "Del");
		} else {
			fprintf(stdout, "\\x%02x", ch);
		}
	}
	fprintf(stdout, "|");
	fflush(stdout);
}
/***************************************************************/
int termUxSetErrMsg(struct term * tty, int terrn, int syserr)
{
	if (tty && syserr > 0) {
		strncpy(tty->term_errmsg, strerror(syserr), TERM_ERRMSG_MAX);
		tty->term_errmsg[TERM_ERRMSG_MAX] = '\0';
	}

    return (terrn);
}
/***************************************************************/
int termUxGetScreenStats(struct term * tty)
{
    int terrn;
    struct winsize ts;

    terrn = 0;

    if (ioctl(0, TIOCGWINSZ, &ts)) {
    	terrn = termUxSetErrMsg(tty, TERRN_GET_STATS, errno);
    	tty->term_screen_width = 0;
    } else {
        tty->term_screen_width      = ts.ws_col;
    }

    if (!tty->term_screen_width) tty->term_screen_width = 80;
    
    return (terrn);
}
/***************************************************************/
void termUxSetHPEscCds(struct term * tty)
{
	strcpy(tty->term_esc_code[TERM_ESC_CDX_LEFT]				, "\x1B\x44");
	strcpy(tty->term_esc_code[TERM_ESC_CDX_RIGHT]				, "\x1B\x43");
	strcpy(tty->term_esc_code[TERM_ESC_CDX_UP]					, "\x1B\x41");
	strcpy(tty->term_esc_code[TERM_ESC_CDX_DOWN]				, "\x1B\x42");
	strcpy(tty->term_esc_code[TERM_ESC_CDX_HOME]				, "\0"); /* none */
	strcpy(tty->term_esc_code[TERM_ESC_CDX_END]					, "\0"); /* none */
	strcpy(tty->term_esc_code[TERM_ESC_CDX_GOTO_BOL]			, "\x1B\x47");
	strcpy(tty->term_esc_code[TERM_ESC_CDX_GOTO_1UP_RMARGIN]	, "\x1B&a%dC");
}
/***************************************************************/
void termUxSetANSIEscCds(struct term * tty, int term_type_flags)
{
	strcpy(tty->term_esc_code[TERM_ESC_CDX_LEFT]				, "\x1B[D");
	strcpy(tty->term_esc_code[TERM_ESC_CDX_RIGHT]				, "\x1B[C");
	strcpy(tty->term_esc_code[TERM_ESC_CDX_UP]					, "\x1B[A");
	strcpy(tty->term_esc_code[TERM_ESC_CDX_DOWN]				, "\x1B[B");
	strcpy(tty->term_esc_code[TERM_ESC_CDX_HOME]				, "\x1B\x4F\x48");
	strcpy(tty->term_esc_code[TERM_ESC_CDX_END]					, "\x1B\x4F\x46");
	strcpy(tty->term_esc_code[TERM_ESC_CDX_GOTO_BOL]			, "\x1B[;0H");
	strcpy(tty->term_esc_code[TERM_ESC_CDX_GOTO_1UP_RMARGIN]	, "\x1B[A\x1B[%dC");
    
    if (term_type_flags & TERMTYP_FLAG_ESC_RIGHT_DEL) {
        strcpy(tty->term_esc_code[TERM_ESC_CDX_RIGHT_DEL]			, "\x1B[3~");
    }
    tty->term_esc_code[TERM_ESC_CDX_LEFT_DEL][0] = '\b';	/* default */
    tty->term_esc_code[TERM_ESC_CDX_LEFT_DEL][1] = '\0';
/*
 From: https://github.com/danlucraft/redcar-bundles/blob/master/Bundles/TextMate.tmbundle/Support/bin/list_shortcuts.rb
 
 KEY_BYTES = {
 "\xEF\x9C\x80"        => "up",
 "\xEF\x9C\x81"        => "down",
 "\xEF\x9C\x82"        => "left",
 "\xEF\x9C\x83"        => "right",
 "\xEF\x9C\xA8"        => "delete",
 "\xEF\x9C\xA9"        => "home",
 "\xEF\x9C\xAB"        => "end",
 "\xEF\x9C\xAC"        => "pageup",
 "\xEF\x9C\xAD"        => "pagedown",
 "\xEF\x9C\xB9"        => "clear",
 "\x7F\x0A"            => "backspace",
 "\n"                  => "return",
 "\r"                  => "return",
 "\x03"                => "enter",
 "\x1B"                => "escape",
 "\t"                  => "tab",
 "\x19"                => "backtab",
 }
*/
    if (term_type_flags & TERMTYP_FLAG_XCODE_RIGHT_DEL) {
        strcpy(tty->term_esc_code[TERM_ESC_CDX_RIGHT_DEL]			, "\xef\x9c\xa8");
    }
}
/***************************************************************/
int termUxInit(struct term * tty) {

	int terrn;
    int ern;

    terrn = 0;
    tty->term_ch_buflen = 0;
    tty->term_ch_bufix  = 0;

    termUxGetScreenStats(tty);
    termUxSetANSIEscCds(tty, tty->term_type_flags);

    if (!tcgetattr(tty->term_in_fnum, &(tty->term_in_tios))) {
        tty->term_old_in_lmode           = (int)tty->term_in_tios.c_lflag;

        /* Set left delete to current ERASE chacter */
        tty->term_esc_code[TERM_ESC_CDX_LEFT_DEL][0] = tty->term_in_tios.c_cc[VERASE];
        tty->term_esc_code[TERM_ESC_CDX_LEFT_DEL][1] = '\0';
    } else {
    	if (tty->term_flags & TERMFLAG_DEBUG) {
    		ern = ERRNO;
  			fprintf(stderr, "tcgetattr(stdin) failure %d: %s\n", ern, strerror(ern));
    	}
    }

    if (!tcgetattr(tty->term_out_fnum, &(tty->term_out_tios))) {
        tty->term_old_out_lmode          = (int)tty->term_out_tios.c_lflag;
    } else {
    	if (tty->term_flags & TERMFLAG_DEBUG) {
    		ern = ERRNO;
  			fprintf(stderr, "tcgetattr(stdout) failure %d: %s\n", ern, strerror(ern));
    	}

    }

    return (terrn);
}
/***************************************************************/
int termUxShut(struct term * tty) {
    
    int terrn;
    
    terrn = 0;
    
    return (terrn);
}
/***************************************************************/
int termUxBeginReadLine(struct term * tty) {
    
    int terrn;
    int ern;
    struct termios ntios;
    int vtmin;
    int vttime;

    terrn = 0;

    memcpy(&ntios, &(tty->term_in_tios), sizeof(struct termios));
    
    vtmin = ntios.c_cc[VMIN];
    vttime = ntios.c_cc[VTIME];

    ntios.c_lflag &= ~(ICANON); /* Clear ICANON. */
    if (!(tty->term_type_flags & TERMTYP_FLAG_ECHO)) ntios.c_lflag &= ~(ECHO); /* Clear ECHO. */
    ntios.c_cc[VMIN] = 1;
    ntios.c_cc[VTIME] = 0;

    if (tcsetattr(tty->term_in_fnum, TCSAFLUSH, &ntios)) {
    	if (tty->term_flags & TERMFLAG_DEBUG) {
    		ern = ERRNO;
  			fprintf(stderr, "termUxBeginReadLine: tcsetattr(stdin) failure %d: %s\n", ern, strerror(ern));
    	}

    }
    
    return (terrn);
}
/***************************************************************/
int termUxEndReadLine(struct term * tty) {
    
    int terrn;
    int ern;
    struct termios ntios;

    terrn = 0;

    memcpy(&ntios, &(tty->term_in_tios), sizeof(struct termios));

    if (tcsetattr(tty->term_in_fnum, TCSANOW, &ntios)) {
    	if (tty->term_flags & TERMFLAG_DEBUG) {
    		ern = ERRNO;
  			fprintf(stderr, "termUxEndReadLine: tcsetattr(stdin) failure %d: %s\n", ern, strerror(ern));
    	}

    }
    
    return (terrn);
}
/***************************************************************/
int termUxEscapeMatch(struct term * tty, const char * seq, int esc_ix)
{
	int escMatch;
	int escLen;

	escMatch = 0;
	escLen = Istrlen(tty->term_esc_code[esc_ix]);

	if (escLen && !memcmp(seq, tty->term_esc_code[esc_ix], escLen)) {
		escMatch = 1;
		tty->term_ch_bufix += escLen;
	}

    return (escMatch);
}
/***************************************************************/
int termUxXlateEscape(struct term * tty)
{
	int vkey;
    int max_len;
	char seq[TERM_CH_BUF_SIZE + 2];

	vkey = tty->term_ch_buf[tty->term_ch_bufix];
    max_len = tty->term_ch_buflen - tty->term_ch_bufix;

	memcpy(seq, tty->term_ch_buf + tty->term_ch_bufix, max_len);
	tty->term_ch_buf[max_len] = '\0';

	if (termUxEscapeMatch(tty, seq, TERM_ESC_CDX_LEFT)) {
		vkey = TERMKEY_LEFT;
	} else if (termUxEscapeMatch(tty, seq, TERM_ESC_CDX_RIGHT)) {
		vkey = TERMKEY_RIGHT;
	} else if (termUxEscapeMatch(tty, seq, TERM_ESC_CDX_UP)) {
		vkey = TERMKEY_UP;
	} else if (termUxEscapeMatch(tty, seq, TERM_ESC_CDX_DOWN)) {
		vkey = TERMKEY_DOWN;
	} else if (termUxEscapeMatch(tty, seq, TERM_ESC_CDX_HOME)) {
		vkey = TERMKEY_HOME;
	} else if (termUxEscapeMatch(tty, seq, TERM_ESC_CDX_END)) {
		vkey = TERMKEY_END;
	} else if (termUxEscapeMatch(tty, seq, TERM_ESC_CDX_LEFT_DEL)) {
		vkey = TERMKEY_LEFT_DEL;
	} else if (termUxEscapeMatch(tty, seq, TERM_ESC_CDX_RIGHT_DEL)) {
		vkey = TERMKEY_RIGHT_DEL;
	} else {
        tty->term_ch_bufix += 1;
    }
/*fprintf(stdout, "termUxXlateEscape vkey=%x\n", vkey); fflush(stdout);*/

    return (vkey);
}
/***************************************************************/
#if DEBUG_TERMINAL
int dbgt_ix = 0;
char * dbgt_list[] = {
		"abc\x7f*\n",
		"//\n",
		NULL
};

int termDbgReadChars(char * buf, int buflen) {

	int rtn;

	if (!dbgt_list[dbgt_ix]) {
		rtn = -1;
	} else {
		rtn = Istrlen(dbgt_list[dbgt_ix]);
		if (rtn > buflen) rtn = buflen;
		memcpy(buf, dbgt_list[dbgt_ix], rtn);
		dbgt_ix++;
	}

	return (rtn);
}
#endif
/***************************************************************/
int termUxReadChar(struct term * tty, int * rdch)
{
/*
**  See 'man tcsetattr' and 'man termios' for Unix terminal I/O info.
*/
    int terrn;
    int rtn;
    int ern;

    terrn = 0;
    (*rdch) = 0;

#if DEBUG_TERMINAL
    if (tty->term_ch_bufix == tty->term_ch_buflen) {
    	rtn = termDbgReadChars(tty->term_ch_buf, TERM_CH_BUF_SIZE);
    	if (rtn > 0) {
    	    tty->term_ch_buflen = rtn;
    	    tty->term_ch_bufix  = 0;
    	}
    }
#endif

    while (!terrn && tty->term_ch_bufix == tty->term_ch_buflen) {
    	rtn = (Int)READ(tty->term_in_fnum, tty->term_ch_buf, TERM_CH_BUF_SIZE);
    	if (rtn > 0) {
/*    		termShowbuf(tty, "termUxReadChar: ", tty->term_ch_buf, rtn);*/
    		tty->term_ch_buflen = rtn;
    	    tty->term_ch_bufix  = 0;
    	} else {
            ern = ERRNO;
            if (ern != EINTR) {
                sprintf(tty->term_errmsg, "Terminal read error %d: %s", ern, strerror(ern));
                terrn = -1;
            }
    	}
    }

    if (!terrn) {
   		(*rdch) = tty->term_ch_buf[tty->term_ch_bufix];

   		if ((*rdch) == ASCII_LINE_FEED) {
   			(*rdch) = TERMKEY_ENTER;
            tty->term_ch_bufix += 1;
   		} else if ( ! PRINTING_CHAR(*rdch)) {
   			(*rdch) = termUxXlateEscape(tty);
   		} else {
            tty->term_ch_bufix += 1;
        }
    }

    return (terrn);
}
/***************************************************************/
int termUxGoto1UpRMargin(struct term * tty, int rmargin)
{
    int terrn;
    int rtn;
    int outlen;
    char outbuf[TERM_ESC_CD_MAX + 16];
    
    terrn = 0;
    
	sprintf(outbuf, tty->term_esc_code[TERM_ESC_CDX_GOTO_1UP_RMARGIN], rmargin);
    /*    strcpy(outbuf, tty->term_esc_code[TERM_ESC_CDX_LEFT]);*/
    outlen = Istrlen(outbuf);
    rtn = (int)WRITE(tty->term_out_fnum, outbuf, outlen);
    if (rtn != outlen) {
		terrn = termUxSetErrMsg(tty, TERRN_WRITE, errno);
    } else {
    	tty->term_screen_cursor_x = rmargin - 1;
    }
    
    return (terrn);
}
/***************************************************************/
int termUxGoto1Left(struct term * tty)
{
    int terrn;
    int rtn;
    int outlen;
    char outbuf[TERM_ESC_CD_MAX + 16];
    
    terrn = 0;
    
	strcpy(outbuf, tty->term_esc_code[TERM_ESC_CDX_LEFT]);
    outlen = Istrlen(outbuf);
    rtn = (int)WRITE(tty->term_out_fnum, outbuf, outlen);
    if (rtn != outlen) {
		terrn = termUxSetErrMsg(tty, TERRN_WRITE, errno);
    }
    
    return (terrn);
}
/***************************************************************/
int termUxWriteChars(struct term * tty, const char * outbuf, int outlen)
{
	int terrn;
	int rtn;
	int ii;
	int outix;
	int wlen;
    int use_move_left;

	terrn = 0;
    use_move_left = 0;

	outix = 0;
	for (ii = 0; ii < outlen; ii++) {
		if (outbuf[ii] >= ' ' && !(outbuf[ii] & TERMKEY_MASK)) {
			tty->term_cursor_ix += 1;
			tty->term_screen_cursor_x += 1;
			/*
			 * Use term_screen_cursor_x > term_screen_width instead
			 * of >= because Linux does not wrap unless an attempt is
			 * made to write in column after last.  If == and next
			 * character is ASCII_BACKSPACE, then it works!
			 */
			if ((tty->term_type_flags & (TERMTYP_FLAG_EOL_VIRTUAL_NEXT | TERMTYP_FLAG_EOL_WRAP_NEXT)) &&
                tty->term_screen_cursor_x > tty->term_screen_width) {
				tty->term_screen_cursor_x -= tty->term_screen_width;
			} else if ((tty->term_type_flags & TERMTYP_FLAG_EOL_WRAP) &&
                 tty->term_screen_cursor_x >= tty->term_screen_width) {
				tty->term_screen_cursor_x -= tty->term_screen_width;

			}
		} else if (outbuf[ii] == '\r' || outbuf[ii] == '\n') {
			tty->term_cursor_ix = 0;
		} else if (outbuf[ii] == '\b' ) {
			tty->term_cursor_ix -= 1;
			tty->term_screen_cursor_x -= 1;
			if (tty->term_screen_cursor_x < 0) {
				/*
                ** This is all needed because backspace does not wrap backward
                */
				termUxGetScreenStats(tty);

				wlen = ii - outix;
				if (wlen) {
					rtn = (int)WRITE(tty->term_out_fnum, outbuf + outix, wlen);
					if (rtn < 0) {
						terrn = termUxSetErrMsg(tty, TERRN_WRITE, errno);
					}
				}
				termUxGoto1UpRMargin(tty, tty->term_screen_width);
				outix = ii + 1;
			} else if ((tty->term_type_flags & TERMTYP_FLAG_EOL_WRAP_NEXT) &&
                        tty->term_screen_cursor_x + 1 == tty->term_screen_width) {
				/*
                 ** This is all needed because the on Macintosh OS X Terminal program,
                 ** the cursor stays on the last column when a character is written to the
                 ** last column.  When this happens, '\b' moves the cursor left, but <esc>[D
                 ** keeps the cursor on the last column, with next character overwrite.
                 */
				wlen = ii - outix;
				if (wlen) {
					rtn = (int)WRITE(tty->term_out_fnum, outbuf + outix, wlen);
					if (rtn < 0) {
						terrn = termUxSetErrMsg(tty, TERRN_WRITE, errno);
					}
				}
				termUxGoto1Left(tty);
				outix = ii + 1;
            }
		}
	}

	wlen = outlen - outix;
	if (wlen) {
		rtn = (int)WRITE(tty->term_out_fnum, outbuf + outix, wlen);
		if (rtn < 0) {
			terrn = termUxSetErrMsg(tty, TERRN_WRITE, errno);
		}
	}

	return (terrn);
}
/***************************************************************/
int termUxSetCursor(struct term * tty, int insert_cursor)
{
    int terrn;
    
    terrn = 0;
    
    return (terrn);
}
/***************************************************************/
#endif
/***************************************************************/
int termUpdateLine(struct term * tty, const char * outbuf)
{
    int terrn;
    char * obuf;
    int olen;
    int obix;
    int ilen;
    int stix;
    int outlen;

    terrn = 0;

    stix = 0;
    while (stix < tty->term_inbuf_len && outbuf[stix] &&
            outbuf[stix] == tty->term_inbuf[stix]) stix++;

    outlen = stix;
    while (outbuf[outlen]) outlen++;

    if (tty->term_inbuf_len > outlen) olen = 3 * (tty->term_inbuf_len) + 4;
    else                              olen = 2 * (outlen) + 4;
    obix = 0;
    obuf = New(char, olen);

    /* put cursor on tty->term_inbuf[stix] */
    ilen = tty->term_cursor_ix - stix;
    if (ilen > 0) {
        memset(obuf + obix, '\b', ilen);
        obix += ilen;
    } else if (ilen < 0) {
        memcpy(obuf + obix, tty->term_inbuf + tty->term_cursor_ix, -ilen);
        obix -= ilen;
    }

    /* write outbuf[stix] */
    ilen = outlen - stix;
    if (ilen > 0) {
        memcpy(obuf + obix, outbuf + stix, ilen);
        obix += ilen;
    }

    /* space over remaining tty->term_inbuf[outlen] */
    ilen = tty->term_inbuf_len - outlen;
    if (ilen > 0) {
        memset(obuf + obix, ' ', ilen);
        obix += ilen;
        memset(obuf + obix, '\b', ilen);
        obix += ilen;
    }

    terrn = tty->termWriteChars(tty, obuf, obix);
    Free(obuf);

    /* set tty->term_inbuf to outbuf */
    if (outlen >= tty->term_inbuf_max) {
        tty->term_inbuf_max = outlen + 128;
        tty->term_inbuf =
            Realloc(tty->term_inbuf, char, tty->term_inbuf_max);
    }
    memcpy(tty->term_inbuf, outbuf, outlen);
    tty->term_inbuf_len = outlen;

    return (terrn);
}
/***************************************************************/
int termHandleKey(struct term * tty, int rdch)
{
    int terrn;
    char * obuf;
    int olen;
    int obix;
    int ilen;
    char ch;
    char outbuf[TERM_ESC_CD_MAX + 4];

/*
    CONSOLE_SCREEN_BUFFER_INFO curInf;
    COORD curPos;
    BOOL bSuccess;
    HANDLE hStdOut;

        case TERMKEY_RIGHT:
            if (tty->term_cursor_ix < tty->term_inbuf_len) {
                hStdOut = (HANDLE)_get_osfhandle(tty->term_out_fnum);
                bSuccess = GetConsoleScreenBufferInfo(hStdOut, &curInf);
                if (bSuccess) {
                    curPos = curInf.dwCursorPosition;
                    curPos.X += 1;
                    bSuccess = SetConsoleCursorPosition(hStdOut, curPos);
                }
                if (bSuccess) tty->term_cursor_ix += 1;
            }
            break;
*/

    terrn = 0;
/*	fprintf(stdout, " termHandleKey() abcde\b\b  \b\bf\n"); fflush(stdout);*/

    switch (rdch) {
        case TERMKEY_LEFT:
            if (tty->term_cursor_ix > 0) {
                if (tty->term_cursor_ix  == tty->term_inbuf_len) {
                    terrn = tty->termSetCursor(tty, 1);
                }
                strcpy(outbuf, "\b");
                terrn = tty->termWriteChars(tty, outbuf, 1);
            }
            break;

        case TERMKEY_RIGHT:
            if (tty->term_cursor_ix < tty->term_inbuf_len) {
                terrn = tty->termWriteChars(tty, tty->term_inbuf + tty->term_cursor_ix, 1);
                if (tty->term_cursor_ix  == tty->term_inbuf_len) {
                    terrn = tty->termSetCursor(tty, 0);
                }
            }
            break;

        case TERMKEY_LEFT_DEL:
        	if (tty->term_cursor_ix > 0) {
                ilen = tty->term_inbuf_len - tty->term_cursor_ix;
                olen = 2 * ilen + 4;
                obix = 0;
                obuf = New(char, olen);
                obuf[obix++] = '\b';
                if (ilen > 0) {
                    memcpy(obuf + 1, tty->term_inbuf + tty->term_cursor_ix, ilen);
                    obix += ilen;
                }
                obuf[obix++] = ' ';
                memset(obuf + obix, '\b', ilen + 1);
                obix += ilen + 1;
                /* delete from mem */
                if (ilen > 0) {
                    memmove(tty->term_inbuf + tty->term_cursor_ix - 1, tty->term_inbuf + tty->term_cursor_ix, ilen);
                }
                tty->term_inbuf_len -= 1;

                terrn = tty->termWriteChars(tty, obuf, obix);
                Free(obuf);
            }
            break;

        case TERMKEY_RIGHT_DEL:
            if (tty->term_cursor_ix < tty->term_inbuf_len) {
                ilen = tty->term_inbuf_len - tty->term_cursor_ix - 1;
                olen = 2 * ilen + 4;
                obix = 0;
                obuf = New(char, olen);
                if (ilen > 0) {
                    memcpy(obuf, tty->term_inbuf + tty->term_cursor_ix + 1, ilen);
                    obix += ilen;
                }
                obuf[obix++] = ' ';
                memset(obuf + obix, '\b', ilen + 1);
                obix += ilen + 1;
                /* delete from mem */
                if (ilen > 0) {
                    memmove(tty->term_inbuf + tty->term_cursor_ix, tty->term_inbuf + tty->term_cursor_ix + 1, ilen);
                }
                tty->term_inbuf_len -= 1;

                terrn = tty->termWriteChars(tty, obuf, obix);
                Free(obuf);
                if (tty->term_cursor_ix  == tty->term_inbuf_len) {
                    terrn = tty->termSetCursor(tty, 0);
                }
            }
            break;

        case TERMKEY_HOME:
            if (tty->term_cursor_ix > 0) {
                olen = tty->term_cursor_ix;
                obix = 0;
                obuf = New(char, olen);
                memset(obuf, '\b', olen);
                obix += olen;

                terrn = tty->termWriteChars(tty, obuf, obix);
                Free(obuf);
                terrn = tty->termSetCursor(tty, 1);
            }
            break;

        case TERMKEY_END:
            if (tty->term_cursor_ix < tty->term_inbuf_len) {
                ilen = tty->term_inbuf_len - tty->term_cursor_ix;
                terrn = tty->termWriteChars(tty, tty->term_inbuf + tty->term_cursor_ix, ilen);
                terrn = tty->termSetCursor(tty, 0);
            }
            break;

        case TERMKEY_UP:
            if (!(tty->term_flags & TERMFLAG_NO_HISTORY)) {
                if (!tty->term_hist_ix) {
                    terrn = tty->termUpdateLine(tty, "");
                    tty->term_hist_adv = 0;
                } else {
                    tty->term_hist_ix -= 1;
                    tty->term_hist_adv = 1;
                    terrn = tty->termUpdateLine(tty, tty->term_hist[tty->term_hist_ix]);
                }
                terrn = tty->termSetCursor(tty, 0);
            }
            break;

        case TERMKEY_DOWN:
            if (!(tty->term_flags & TERMFLAG_NO_HISTORY)) {
                if (tty->term_hist_adv) tty->term_hist_ix += 1;
                if (tty->term_hist_ix >= tty->term_hist_len) {
                    terrn = tty->termUpdateLine(tty, "");
                    tty->term_hist_ix  = tty->term_hist_len;
                    tty->term_hist_adv = 0;
                } else {
                    tty->term_hist_adv = 1;
                    terrn = tty->termUpdateLine(tty, tty->term_hist[tty->term_hist_ix]);
                }
                terrn = tty->termSetCursor(tty, 0);
            }
            break;

        default:
            ch = (char)rdch;
            if (ch >= ' ' && !(rdch & TERMKEY_MASK)) {
                ilen = tty->term_inbuf_len - tty->term_cursor_ix;
                if (!ilen) {    /* append char */
                    tty->term_inbuf[tty->term_cursor_ix] = ch;
                    terrn = tty->termWriteChars(tty, &ch, 1);
                    tty->term_inbuf_len = tty->term_cursor_ix;
                } else {        /* insert char */
                    olen = 2 * ilen + 4;
                    obix = 0;
                    obuf = New(char, olen);
                    obuf[obix++] = ch;

                    memcpy(obuf + obix, tty->term_inbuf + tty->term_cursor_ix, ilen);
                    obix += ilen;

                    memset(obuf + obix, '\b', ilen);
                    obix += ilen;

                    /* insert into mem */
                    memmove(tty->term_inbuf + tty->term_cursor_ix + 1, tty->term_inbuf + tty->term_cursor_ix, ilen);
                    tty->term_inbuf[tty->term_cursor_ix] = ch;
                    tty->term_inbuf_len += 1;

                    terrn = tty->termWriteChars(tty, obuf, obix);
                    Free(obuf);
                }
            }
            break;
    }

    return (terrn);
}
/***************************************************************/
int termReadLine(struct term * tty, char * prompt, char ** inbuf, int * inlen)
{
    int terrn;
    int terrn2;
    int done;
    int rdch;

    terrn = 0;
    (*inbuf) = NULL;
    (*inlen) = 0;

    tty->term_screen_cursor_x       = 0;

    terrn = tty->termBeginReadLine(tty);
    terrn = tty->termWriteChars(tty, prompt, (int)strlen(prompt));
/*
    fprintf(stdout, "xyz\b\x1B[K"); fflush(stdout);
    fprintf(stdout, "xyz\x1B[D \x1B[D"); fflush(stdout);
*/

    tty->term_inbuf_len             = 0;
    tty->term_cursor_ix             = 0;

    done  = 0;

    while (!done && !terrn) {
        terrn = tty->termReadChar(tty, &rdch);
        if (!terrn) {
            if (tty->term_cursor_ix >= tty->term_inbuf_max) {
                tty->term_inbuf_max += 128;
                tty->term_inbuf =
                    Realloc(tty->term_inbuf, char, tty->term_inbuf_max);
            }
            if (rdch == TERMKEY_ENTER) {
                done = 1;
                tty->term_inbuf[tty->term_inbuf_len] = '\0';
                (*inbuf) = tty->term_inbuf;
                (*inlen) = tty->term_inbuf_len;
                terrn = tty->termWriteChars(tty, "\n", 1);
            } else if (rdch == TERMKEY_INTR) {
                done = 1;
                tty->term_inbuf[tty->term_inbuf_len] = '\0';
                terrn = tty->termWriteChars(tty, "\n", 1);
                terrn = TERMREAD_INTR;
            } else if (rdch == TERMKEY_EOF) {
                done = 1;
                tty->term_inbuf[tty->term_inbuf_len] = '\0';
                (*inbuf) = tty->term_inbuf;
                (*inlen) = tty->term_inbuf_len;
                terrn = tty->termWriteChars(tty, "\n", 1);
                if (!terrn) terrn = TERMREAD_EOF;
            } else if (rdch) {
                terrn = tty->termHandleKey(tty, rdch);
            }
        }            
    }
    terrn2 = tty->termEndReadLine(tty);
    if (!terrn) terrn = terrn2;

    return (terrn);
}
/***************************************************************/
int termReadLineContinuation(struct term * tty, char * prompt, char ** inbuf, int * inlen)
{
    int terrn;
    char * svbuf;
    char * nbuf;
    int nlen;


    if (!(*inlen)) {
        terrn = tty->termReadLine(tty, prompt, inbuf, inlen);
    } else {
        nbuf = NULL;
        svbuf = New(char, (*inlen) + 1);
        memcpy(svbuf, *inbuf, (*inlen));

        terrn = tty->termReadLine(tty, prompt, &nbuf, &nlen);
        if (!terrn) {
            nlen += (*inlen);
            if (nlen + 1 >= tty->term_inbuf_max) {
                tty->term_inbuf_max = nlen + 64;
                tty->term_inbuf =
                    Realloc(tty->term_inbuf, char, tty->term_inbuf_max);
            }
            memmove(tty->term_inbuf + (*inlen), tty->term_inbuf, tty->term_inbuf_len + 1);
            memcpy(tty->term_inbuf, svbuf, (*inlen));
            tty->term_inbuf_len             = nlen;
            (*inlen)                        = nlen;
        }
        Free(svbuf);
    }

    return (terrn);
}
/***************************************************************/
int termAddHistoryLine(struct term * tty, const char * inbuf)
{
    int terrn;

    terrn = 0;
    if (!inbuf || !inbuf[0] ||
            (tty->term_hist_ix == tty->term_hist_len && tty->term_hist_len > 0 &&
            !strcmp(tty->term_hist[tty->term_hist_ix - 1], inbuf))) {
        /* do nothing */
    } else if (tty->term_hist_ix < tty->term_hist_len &&
            !strcmp(tty->term_hist[tty->term_hist_ix], inbuf)) {
        tty->term_hist_ix += 1;
    } else {
        if (tty->term_hist_len == tty->term_hist_max) {
            if (!tty->term_hist_max) tty->term_hist_max = 64; /* chunk */
            else tty->term_hist_max *= 2;
            tty->term_hist = Realloc(tty->term_hist, char *, tty->term_hist_max);
        } 
        tty->term_hist[tty->term_hist_len] = Strdup(inbuf);
        tty->term_hist_len += 1;
        tty->term_hist_ix = tty->term_hist_len;
    }
    tty->term_hist_adv = 0;

    return (terrn);
}
/***************************************************************/
int termSetFlags(struct term * tty, int tFlags)
{
    int prev_flags;

    prev_flags = (tty->term_flags & TERMFLAG_MASK);

    tty->term_flags |= (tFlags & TERMFLAG_MASK);

    return (prev_flags);
}
/***************************************************************/
int termClearFlags(struct term * tty, int tFlags)
{
    int prev_flags;

    prev_flags = (tty->term_flags & TERMFLAG_MASK);

    tty->term_flags ^= (prev_flags & tFlags);

    return (prev_flags);
}
/***************************************************************/
/*
int termSetCheckInterrupt(struct term * tty, int (*intrChecker)(void))
{
    int terrn;

    terrn = 0;

    tty->termIntrChecker            = intrChecker;

    return (terrn);
}
*/
/***************************************************************/
int term_init(struct term * tty) {

    int terrn;

    terrn = 0;

    tty->term_inbuf                 = NULL;
    tty->term_inbuf_len             = 0;
    tty->term_inbuf_max             = 0;
    tty->term_cursor_ix             = 0;
    tty->term_cursor_info1          = 0;
    tty->term_cursor_info2          = 0;

    tty->term_hist                  = NULL;
    tty->term_hist_adv              = 0;
    tty->term_hist_ix               = 0;
    tty->term_hist_len              = 0;
    tty->term_hist_max              = 0;

    tty->term_old_in_lmode           = 0;
    tty->term_old_out_lmode          = 0;
    tty->term_new_in_lmode           = 0;
    tty->term_new_out_lmode          = 0;

    tty->term_sighndlr               = NULL;
    /* tty->termIntrChecker            = NULL; */

    tty->term_errmsg[0]              = '\0';

 #if IS_WINDOWS
    tty->termInit                   = termWinInit;
    tty->termShut                   = termWinShut;
    tty->termBeginReadLine          = termWinBeginReadLine;
    tty->termEndReadLine            = termWinEndReadLine;
    tty->termReadChar               = termWinReadChar;
    tty->termWriteChars             = termWinWriteChars;
    tty->termSetCursor              = termWinSetCursor;
    tty->termSetErrMsg              = termWinSetErrMsg;
 #else

    memset(tty->term_esc_code, 0, TERM_NUM_ESC_CDS * TERM_ESC_CD_MAX);

    tty->termInit                   = termUxInit;
    tty->termShut                   = termUxShut;
    tty->termBeginReadLine          = termUxBeginReadLine;
    tty->termEndReadLine            = termUxEndReadLine;
    tty->termReadChar               = termUxReadChar;
    tty->termWriteChars             = termUxWriteChars;
    tty->termSetCursor              = termUxSetCursor;
    tty->termSetErrMsg              = termUxSetErrMsg;
#endif

    tty->termReadLine               = termReadLine;
    tty->termReadLineContinuation   = termReadLineContinuation;
    tty->termHandleKey              = termHandleKey;
    tty->termAddHistoryLine         = termAddHistoryLine;
    tty->termSetFlags               = termSetFlags;
    tty->termClearFlags             = termClearFlags;
    tty->termUpdateLine             = termUpdateLine;
    /* tty->termSetCheckInterrupt      = termSetCheckInterrupt; */

    terrn = tty->termInit(tty);

    return (terrn);
}
/***************************************************************/
void free_term(struct term * tty) {

    int ii;
    int terrn;

    if (tty->term_hist) {
        for (ii = 0; ii < tty->term_hist_len; ii++) Free(tty->term_hist[ii]);
        Free(tty->term_hist);
    }

    Free(tty->term_inbuf);

    terrn = tty->termShut(tty);

    CLOSE(tty->term_in_fnum);
    CLOSE(tty->term_out_fnum);

    Free(tty);
}
/***************************************************************/
int term_get_term_type_flags(int term_flags) {
    
    char * term;
    int term_type_flags;
    
    term_type_flags = 0;
    term = getenv("TERM_PROGRAM");

    if (term_flags & TERMFLAG_VERBOSE) {
        if (term) {
            fprintf(stdout, "Using TERM_PROGRAM=%s\n", term);
        } else {
            fprintf(stdout, "TERM_PROGRAM not set\n");
        }
        fflush(stdout);
    }

    if (term) {
        if (!strcmp(term, "Apple_Terminal")) {
            term_type_flags |= TERMTYP_FLAG_DEL_LEFT_DEL;
            term_type_flags |= TERMTYP_FLAG_ESC_RIGHT_DEL;
            term_type_flags |= TERMTYP_FLAG_EOL_WRAP_NEXT;
            
        } else if (!strcmp(term, "XCode_Terminal")) {
            term_type_flags |= TERMTYP_FLAG_DEL_LEFT_DEL;
            term_type_flags |= TERMTYP_FLAG_XCODE_RIGHT_DEL;
            term_type_flags |= TERMTYP_FLAG_ECHO; /* XCode_Terminal always echos */
        }
    }
    
    return (term_type_flags);
}
/***************************************************************/
struct term * new_term(int in_fnum, int out_fnum, int term_flags) {

    struct term * tty;
    int terrn;

    tty = New(struct term, 1);

    tty->term_in_fnum                  = DUP(in_fnum);
    tty->term_out_fnum                 = DUP(out_fnum);
    tty->term_flags                    = term_flags;

    tty->term_type_flags                = term_get_term_type_flags(term_flags);

    terrn = term_init(tty);
    if (terrn) {
        free_term(tty);
        tty = NULL;
    }

    return (tty);
}
/***************************************************************/
char * term_errmsg(struct term * tty, int terrn) {

    static char terrmsg[TERM_ERRMSG_MAX];

    if (tty && tty->term_errmsg[0]) {
        strcpy(terrmsg, tty->term_errmsg);
    } else {
        switch (terrn) {
            case TERRN_INIT   :
                strcpy(terrmsg, "Error initializing terminal.");
                break;

            default   :
                sprintf(terrmsg, "Unrecognized terminal error number %d", terrn);
                break;
        }
    }

    return (terrmsg);
}
/***************************************************************/


