/**************************************************************/
/* decas.c                                                    */
/**************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>
#include <errno.h>

#include "types.h"
#include "decas.h"
#include "opportun.h"
#include "play.h"
#include "strat.h"
#include "randnum.h"

/*
**  Plan:
**      Write game_dblbld_opportunities(), called when player
**      has two cards that are the same (a pair).
**
**      Use dblbldlist in 
**          game_take_opportunities()
**          game_build_opportunities()
**          game_build_double_opportunities()
**
**      Test more than 8 cards on board. More than 16? 
**
**  Issues:
**      game_build_double_opportunities() may put cards in order
**      that doesn't add.
**      For example, build doubled tens may be: 8, 7, 3, 2.
**      Is this a problem? For printing, yes.
**
**  Done
**      Allow hands and board to be read from input file.
*/

char suit_list[] = "CDHS";
char pip_list[] = "@A23456789TJQK";
char player_list[] = "EW";
int g_do_pause;

int             global_msglevel = 0;
FILE *          global_outfile  = NULL;

/***************************************************************/
void program_abort(const char * msg) {

    if (global_outfile) {
        fprintf(global_outfile, "**** ABORT: %s\n", msg);
        fflush(global_outfile);
        fclose(global_outfile);
    }

    fprintf(stdout, "**** ABORT: %s\n", msg);
    fflush(stdout);

 	if (g_do_pause) {
		char inbuf[100];
		printf("Press ENTER to continue...");
		fgets(inbuf, sizeof(inbuf), stdin);
	}

   exit(1);
}
/***************************************************************/
void showusage(char * prognam) {

	printf("%s - v%s\n", PROGRAM_NAME, VERSION);
	printf("\n");
	printf("Usage: %s [options]\n", prognam);
	printf("\n");
	printf("Options:\n");
	printf("  -help              Show extended help.\n");
	printf("  -parms <file>      Use parm file <file>.\n");
	printf("  -pause             Pause when done.\n");
	printf("  -version           Show program version.\n");
	printf("\n");
}
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
int set_error(int estat, const char * fmt, ...) {

	va_list args;
    char msgbuf[MSGOUT_SIZE];

	va_start(args, fmt);
	vsprintf(msgbuf, fmt, args);
	va_end(args);

    fprintf(stderr, "%s (DEERR %d)\n", msgbuf, estat);
    if (global_outfile) fprintf(global_outfile, "%s (DEERR %d)\n", msgbuf, estat);

	return (estat);
}
/***************************************************************/
int set_error_f(int estat, struct frec * fr, const char * fmt, ...) {

	va_list args;
    char msgbuf[MSGOUT_SIZE];

	va_start(args, fmt);
	vsprintf(msgbuf, fmt, args);
	va_end(args);

    fprintf(stderr, "%s\n", msgbuf);
	fprintf(stderr, "Error in file %s, line #%d (DEERR %d)\n",
        fr->fr_name, fr->fr_linenum, estat);

    if (global_outfile) {
        fprintf(global_outfile, "%s\n", msgbuf);
	    fprintf(global_outfile, "Error in file %s, line #%d (DEERR %d)\n",
            fr->fr_name, fr->fr_linenum, estat);
    }

	return (estat);
}
/***************************************************************/
int is_msglevel(int msglevel)
{
    return (msglevel & global_msglevel);
}
/***************************************************************/
void msgout(int msglevel, const char * fmt, ...) {

	va_list args;
    FILE * outf;
    char msgbuf[MSGOUT_SIZE];

    if (!is_msglevel(msglevel)) return;

    outf = global_outfile;
    if (!outf) outf = stdout;

	va_start(args, fmt);
	vsprintf(msgbuf, fmt, args);
	va_end(args);

    fprintf(outf, "%s", msgbuf);
}
/***************************************************************/
void deck_init(struct deck * dk, count_t ncards) {

    int ii;

	dk->dk_ncards = ncards;
    for (ii = 0; ii < ncards; ii++) dk->dk_card[ii] = (count_t)(ii + 4);
}
/***************************************************************/
void gstate_init(struct gstate * gst)
{
/*
** See also: calc_opponent_gstate()
*/

    gst->gst_last_take          = PLAYER_NOBODY;
    gst->gst_nhand[PLAYER_EAST] = 0;
    gst->gst_nhand[PLAYER_WEST] = 0;
    gst->gst_nboard             = 0;
    gst->gst_nturns             = 0;
    gst->gst_flags              = 0;
    gst->gst_ntaken[PLAYER_EAST] = 0;
    gst->gst_ntaken[PLAYER_WEST] = 0;
}
/***************************************************************/
struct deck * deck_new(void) {

	struct deck * dk;

	dk = New(struct deck, 1);

	dk->dk_ncards = 0;
    memset(dk->dk_card, 0, sizeof(card_t) * FIFTY_TWO);

	return (dk);
}
/***************************************************************/
void game_init(struct game * gm)
{
    gm->gm_dealer   = PLAYER_EAST;
    gm->gm_round    = 0;
    gm->gm_rgen     = PLAY_DEFAULT_RGEN;
    gm->gm_rseed    = PLAY_DEFAULT_RSEED;
    gm->gm_rg       = NULL;
    gm->gm_erg      = NULL;

    gstate_init(&(gm->gm_gst));
}
/***************************************************************/
struct game * game_new(void) {

	struct game * gm;

	gm = New(struct game, 1);
	gm->gm_dk = deck_new();
    deck_init(gm->gm_dk, 0);

    game_init(gm);

	return (gm);
}
/***************************************************************/
void game_set_rgen(struct game * gm, int rgen, int rseed) {

    gm->gm_rgen  = rgen;
    gm->gm_rseed = rseed;

    if (gm->gm_rg) randintfree(gm->gm_rg);
    gm->gm_rg = randintseed(rgen, rseed);
}
/***************************************************************/
void game_new_glb(struct globals * glb) {

    if (!glb->glb_gm) {
        glb->glb_gm = game_new();
    } else {
        game_init(glb->glb_gm);
        deck_init(glb->glb_gm->gm_dk, 0);
    }
}
/***************************************************************/
struct globals * globals_new(void) {

	struct globals * glb;

	glb = New(struct globals, 1);

	glb->glb_gm    = NULL;
	glb->glb_flags = 0;
	glb->glb_nparms = 0;
	glb->glb_parms  = NULL;

	return (glb);
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
void deck_free(struct deck * dk) {

	Free(dk);
}
/***************************************************************/
void game_free(struct game * gm) {

	if (!gm) return;

    deck_free(gm->gm_dk);
	Free(gm);
}
/***************************************************************/
void globals_free(struct globals * glb) {

	game_free(glb->glb_gm);
	free_parms(glb);
	Free(glb);
}
/***************************************************************/
void shut(void) {

#ifdef _Windows
	shut_com();
#endif
    if (global_outfile) {
        fflush(global_outfile);
        fclose(global_outfile);
    }
}
/***************************************************************/
void get_toke_quote(const char * bbuf, int * bbufix, char *toke, int tokemax) {
	/*
	** Parse quotation
	*/
	char quote;
	int tokelen;
	int done;
	char ch;

	tokelen = 0;
	toke[0] = '\0';

	ch = bbuf[*bbufix];
	(*bbufix)++;
	quote = ch;
	toke[tokelen++] = ch;
	done = 0;

	while (!done) {
		ch = bbuf[*bbufix];

		if (!ch) {
			done = 1;
		} else if (ch == quote) {
			if (tokelen + 1 < tokemax) toke[tokelen++] = ch;
			(*bbufix)++;
			if (bbuf[*bbufix] == quote) (*bbufix) += 1;
			else done = 1;
		} else {
			if (tokelen + 1 < tokemax) toke[tokelen++] = ch;
			(*bbufix)++;
		}
	}
	toke[tokelen] = '\0';
}
/***************************************************************/
void gettoke(const char * bbuf, int * bbufix, char *toke, int tokemax) {
	/*
	** Reads next token from bbuf, starting from bbufix.
	** upshifted
	*/
	int tokelen;
	int done;
	char ch;

	toke[0] = '\0';

	while (bbuf[*bbufix] && isspace(bbuf[*bbufix])) (*bbufix)++;
	if (!bbuf[*bbufix]) return;

	if (bbuf[*bbufix] == '\"') {
		get_toke_quote(bbuf, bbufix, toke, tokemax);
		return;
	}

	tokelen = 0;
	done = 0;

	while (!done) {
		ch = bbuf[*bbufix];

		if (isalpha(ch) || isdigit(ch)) {
			if (tokelen + 1 < tokemax) toke[tokelen++] = (char)toupper(ch);
			(*bbufix)++;
		} else if (ch == '#' ||
			ch == '$' ||
			ch == '.' ||
			ch == '_') {
			if (tokelen + 1 < tokemax) toke[tokelen++] = ch;
			(*bbufix)++;
		} else {
			if (!tokelen) {
				if (tokelen + 1 < tokemax) toke[tokelen++] = ch;
				(*bbufix)++;
			}
			done = 1;
		}
	}
	toke[tokelen] = '\0';
}
/***************************************************************/
/*@*/int stoint32 (const char * from, int32 *dest) {
/*
**  Convert from string to signed 32 bit binary
**  Range is: -2147483648 to +2147483647 with leading sign permitted
**  Returns:  0 if successful       ** Opposite of Warehouse stol()
**            1 if error
*/
    int ii;
    int jj;
    int len;
    int32 nn;
    const int32 max_div_10 = 214748364L;
    const char *ptr;
    char sign;

    *dest = 0;
    ptr = from;
    sign = ptr[0];
    if (sign == '-' || sign == '+') ptr++;
    else sign = '+';
    len = strlen(ptr);
    if (!len) return (1);  /* Don't allow "", "+", or "-" */

    nn = 0;
    jj = 0;
    while (jj<len && nn<max_div_10) {
        ii = ptr[jj] - '0';
        if (ii < 0 || ii > 9) return (1);
        nn = nn * 10 + ii;
        jj++;
    }

    if (jj<len && nn==max_div_10) {
        ii = 0;
        if (jj < len) ii = ptr[jj] - '0';
        if (ii < 0 || ii > 9) return (1);
        if (ii <= 7 && sign != '-') {
            nn = nn * 10 + ii;
            jj++;
        } else if (ii <= 8 && sign == '-') {
            nn = nn * (-10) - ii;
            jj++;
        }
    } else if (sign == '-') {
        nn = -nn;
    }

    if (jj<len) return (1);

    *dest = nn;

    return (0);
}
/***************************************************************/
/*@*/int stoi (const char * from, int *num) {

    int32 num32;
    int out;

    out = stoint32(from, &num32);
    *num = num32;

    return (out);
}
/***************************************************************/
/*@*/int stoui (const char * from, int *num) {

    int32 num32;
    int out;

    out = stoint32(from, &num32);
    *num = num32;
    if (!out && num32 < 0) out = 2;

    return (out);
}
/***************************************************************/
/*@*/int istrcmp(char * b1, char * b2) {
	/*
	** Case insensitive strcmp
	*/
	int ii;

	for (ii = 0; b1[ii]; ii++) {
		if (tolower(b1[ii]) < tolower(b2[ii])) return (-1);
		else if (tolower(b1[ii]) > tolower(b2[ii])) return (1);
	}
	if (b2[ii]) return (-1);
	else        return (0);
}
/***************************************************************/
/*@*/int imemicmp(const char * b1, const char * b2, size_t blen) {
    /*
     ** Case insensitive memcmp
     */
    size_t ii;

    for (ii = 0; ii < blen; ii++) {
        if (tolower(b1[ii]) < tolower(b2[ii])) return (-1);
        else if (tolower(b1[ii]) > tolower(b2[ii])) return (1);
    }
    return (0);
}
/***************************************************************/
int frec_read_line(char * line, int linemax, struct frec * fr) {

    int ii;
    int lix;
    int done;
    int estat;

    lix = 0;
    done = 0;

    while (!done) {
        if (!fgets(fr->fr_buf, fr->fr_bufsize, fr->fr_ref)) {
            int en = ERRNO;
            done = 1;
            if (ferror(fr->fr_ref) || !feof(fr->fr_ref)) {
                estat = set_error(ESTAT_FGETS, "Error #%d reading file: %s\n%s",
                    en, fr->fr_name, strerror(en));
                lix = (-1);
            } else {
                fr->fr_eof = 1;
                lix = (-1);
            }
        } else {
            fr->fr_linenum += 1;
            ii = strlen(fr->fr_buf);
            while (ii > 0 && isspace(fr->fr_buf[ii - 1])) ii--;
            if (linemax - lix <= ii) {
                estat = set_error_f(ESTAT_FBUF, fr, "Buffer overflow reading file");
                lix = (-1);
                done = 1;
            } else {
                done = 1;
                memcpy(line + lix, fr->fr_buf, ii);
                lix += ii;
                line[lix] = '\0';
            }
        }
    }

    return (lix);
}
/***************************************************************/
int frec_close(struct frec * fr) {

    fclose(fr->fr_ref);
    Free(fr->fr_name);
    Free(fr->fr_desc);
    Free(fr->fr_buf);
    Free(fr);

    return (0);
}
/***************************************************************/
struct frec * frec_open(const char * fname, const char * fmode, const char * fdesc) {

    FILE * fref;
    struct frec * fr;

    fref = fopen(fname, fmode);
    if (!fref) {
        int ern = ERRNO;
        set_error(ESTAT_FOPEN, "Error #%d opening %s: %s\n%s",
            ern, fdesc, fname, strerror(ern));
        fr = NULL;
    } else {
        fr = New(struct frec, 1);
        fr->fr_name = Strdup(fname);
        fr->fr_desc = Strdup(fdesc);
        fr->fr_bufsize = LINE_MAX_SIZE + 2;
        fr->fr_buf = New(char, fr->fr_bufsize);
        fr->fr_linenum = 0;
        fr->fr_buflen = 0;
        fr->fr_eof = 0;
        fr->fr_ref = fref;
    }

    return (fr);
}
/***************************************************************/
int deck_card_pr(char * buf, card_t * cards, int ncards, char delim) {

    int ii;
    int blen;
    int suit;
    int pip;

    blen = 0;
    for (ii = 0; ii < ncards; ii++) {
        if (cards[ii] < ACE_OF_CLUBS || cards[ii] > KING_OF_SPADES) {
            buf[blen++] = '*';
            buf[blen++] = '$';
        } else {
            suit = GET_SUIT(cards[ii]);
            pip = GET_PIP(cards[ii]);
            buf[blen++] = pip_list[pip];
            buf[blen++] = suit_list[suit];
        }
        if (delim) buf[blen++] = delim;
    }
    buf[blen] = '\0';

    return (blen);
}
/***************************************************************/
#if IS_DEBUG
void dedug_slot_pr(struct slot * sl) {

    int ii;
    int blen;
    char buf[PBUF_SIZE];

    blen = 0;
    buf[blen++] = '{';
    for (ii = 0; ii < sl->sl_ncard; ii++) {
        blen += deck_card_pr(buf + blen, &(sl->sl_card[ii]), 1, ' ');
    }
    buf[blen++] = '}';
    buf[blen] = '\0';
    msgout(MSGLEVEL_QUIET, "  Board for missed doubled grouping of %c: %s\n",
        pip_list[sl->sl_value], buf);
}
#endif
/***************************************************************/
void goup_doubled_build(struct slot * sl)
{
/*
** Board for missed doubled grouping of T: {7S 2C 2D 4C 6C AH 8C }
**
**      Finds: 6C+2C+2D first, the cannot use 7S or 8C because they require a 2
**
**  Proposed solution:
**      1. Sort ascending
**      2. Count ii down to 1
**
**      That way, the bigger cards should get matched first
**          e.g. 8C+2C, 7S+AH+2D, 6C+4C
*/

    bitmask_t ii;
    bitmask_t jj;
    bitmask_t totbitmask;
    bitmask_t grp_mask;
    bitmask_t grp_pow2;
    count_t slotix;
    count_t tot;
    card_t tcard;
	count_t grp_ncard;
	card_t  grp_card[MAX_CARDS_IN_SLOT];

    /* test if already grouped */
    tot = 0;
    for (slotix = 0; slotix < sl->sl_ncard; slotix++) {
        tot += GET_PIP(sl->sl_card[slotix]);
        if (tot == sl->sl_value) tot = 0;
    }

    if (tot) {
        /* sort sl->sl_card */
        for (ii = 0; ii < sl->sl_ncard - 1; ii++) {
            for (jj = ii + 1; jj < sl->sl_ncard; jj++) {
                if (sl->sl_card[jj] < sl->sl_card[ii]) {
                    tcard = sl->sl_card[ii];
                    sl->sl_card[ii] = sl->sl_card[jj];
                    sl->sl_card[jj] = tcard;
                }
            }
        }

        tot = 0;
        for (slotix = 0; slotix < sl->sl_ncard; slotix++) {
            tot += GET_PIP(sl->sl_card[slotix]);
            if (tot == sl->sl_value) tot = 0;
        }

        grp_ncard = 0;
        grp_mask  = 0;
        grp_pow2  = (1 << sl->sl_ncard) - 1;

        for (ii = grp_pow2; grp_mask != grp_pow2 && ii > 0; ii--) {
            tot = 0;
            BITFOR(totbitmask,ii,slotix) {
                tot += GET_PIP(sl->sl_card[slotix]);
            }
 
            if (tot == sl->sl_value && ((ii | grp_mask) == (ii ^ grp_mask))) {
                BITFOR(totbitmask,ii,slotix) {
                    grp_card[grp_ncard++] = sl->sl_card[slotix];
                }
                grp_mask |= ii;
            }
        }
#if IS_DEBUG
        if (sl->sl_ncard !=  grp_ncard) {
            dedug_slot_pr(sl);
        }
#endif
        DESSERT(sl->sl_ncard ==  grp_ncard, "Missed doubled grouping.");
        CPARRAY(sl->sl_card, sl->sl_ncard, grp_card, grp_ncard);
    }
}
/***************************************************************/
int deck_slot_pr(char * buf, struct slot * sl, char sdelim, int pflags)
{
    int blen;
    int ii;
    int bldval;
    int ngrp;
    char delim;
    char tbuf[PBUF_SIZE];

    blen = 0;
    switch (sl->sl_type) {
        case SLTYPE_SINGLE:
            if (sl->sl_ncard == 1) {
                blen += deck_card_pr(buf + blen, &(sl->sl_card[0]), 1, sdelim);
            } else if (sl->sl_ncard == 0) {
                buf[blen++] = '*';
                buf[blen++] = ' ';
            } else {
                sprintf(tbuf, "**Expecting 1 card, not %d**", sl->sl_ncard);
                strcpy(buf + blen, tbuf);
                blen += IStrlen(tbuf);
            }
            break;

        case SLTYPE_BUILD:
            bldval = 0;
            if (pflags & FLAG_BOARD_SHOW_OWNER) {
                if (sl->sl_owner == PLAYER_EAST) buf[blen++] = 'E';
                else if (sl->sl_owner == PLAYER_WEST) buf[blen++] = 'W';
            }
            buf[blen++] = '[';
            if (sl->sl_ncard == 0) buf[blen++] = ']';
            for (ii = 0; ii < sl->sl_ncard; ii++) {
                bldval += GET_PIP(sl->sl_card[ii]);
                delim = '+';
                if (ii + 1 == sl->sl_ncard) delim = ']';
                blen += deck_card_pr(buf + blen, &(sl->sl_card[ii]), 1, delim);
            }
            if (bldval != sl->sl_value) {
                sprintf(tbuf, "**Expecting build value %d, not %d**",
                    sl->sl_value, bldval);
                strcpy(buf + blen, tbuf);
                blen += IStrlen(tbuf);
            }
            buf[blen++] = sdelim;
            break;

        case SLTYPE_DOUBLED_BUILD:
            goup_doubled_build(sl);
            bldval = 0;
            ngrp = 0;
            if (pflags & FLAG_BOARD_SHOW_OWNER) {
                if (sl->sl_owner == PLAYER_EAST) buf[blen++] = 'E';
                else if (sl->sl_owner == PLAYER_WEST) buf[blen++] = 'W';
            }
            buf[blen++] = '[';
            if (sl->sl_ncard == 0) buf[blen++] = ']';
            for (ii = 0; ii < sl->sl_ncard; ii++) {
                bldval += GET_PIP(sl->sl_card[ii]);
                delim = '+';
                if (ii + 1 == sl->sl_ncard) {
                    delim = ']';
                    ngrp++;
                } else if (bldval == sl->sl_value) {
                    delim = ',';
                    ngrp++;
                    bldval = 0;
                }
                blen += deck_card_pr(buf + blen, &(sl->sl_card[ii]), 1, delim);
            }
            if (bldval != sl->sl_value) {
                sprintf(tbuf, "**Expecting doubled build value %d, not %d**",
                    sl->sl_value, bldval);
                strcpy(buf + blen, tbuf);
                blen += IStrlen(tbuf);
            } else if (ngrp <= 1) {
                sprintf(tbuf, "**Expecting doubled build**");
                strcpy(buf + blen, tbuf);
                blen += IStrlen(tbuf);
            }
            buf[blen++] = sdelim;
            break;

        default:
            sprintf(tbuf, "**Bad slot type: %d**", sl->sl_type);
            strcpy(buf + blen, tbuf);
            blen += IStrlen(tbuf);
            break;
    }

    buf[blen] = '\0';

    return (blen);
}
/***************************************************************/
int deck_board_pr(char * buf, struct slot board[], int nsl, int pflags) {

    int ii;
    int blen;

    blen = 0;
    for (ii = 0; ii < nsl; ii++) {
        blen += deck_slot_pr(buf + blen, &(board[ii]), ' ', pflags);
    }
    buf[blen] = '\0';

    return (blen);
}
/***************************************************************/
void card_show(
    int msglevel,
	struct slot * g_board,
    count_t       g_nboard,
	card_t      * g_hand,
    count_t       g_nhand,
    count_t       g_player)
{
    int pflags;
    char buf[PBUF_SIZE];

    if (g_board) {
        pflags = FLAG_BOARD | FLAG_BOARD_SHOW_OWNER;
        deck_board_pr(buf, g_board, g_nboard, pflags);
        msgout(msglevel, "  Board: %s\n", buf);
    }

    if (g_hand) {
        deck_card_pr(buf, g_hand, g_nhand, ' ');
        msgout(msglevel, "  %s : %s\n", PLAYER_NAME(g_player), buf);
    }
}
/***************************************************************/
void qdeal_cards(struct deck * dk, card_t * cards, count_t * ccount, count_t ncards) {

    int ii;

    dk->dk_ncards -= ncards;
    for (ii = 0; ii < ncards; ii++) {
        cards[ii] = dk->dk_card[dk->dk_ncards + ii];
    }
    (*ccount) = ncards;
}
/***************************************************************/
int deal_cards(struct deck * dk, card_t * cards, count_t * ccount, count_t ncards) {

    int estat = 0;

    if (dk->dk_ncards < ncards) {
        estat = set_error(ESTAT_OUT_OF_CARDS, "No more cards.");
    } else if ((*ccount) + ncards > CARDS_PER_PLAYER) {
        estat = set_error(ESTAT_MANY_CARDS, "Too many cards.");
    } else {
        qdeal_cards(dk, cards, ccount, ncards);
    }

    return (estat);
}
/***************************************************************/
int deal_board(struct deck * dk, struct slot * sl, count_t * ccount, count_t ncards) {

    int estat = 0;
    int ii;
    card_t card;

    if (dk->dk_ncards < ncards) {
        estat = set_error(ESTAT_OUT_OF_CARDS, "No more cards for board.");
    } else if ((*ccount) + ncards > MAX_SLOTS_IN_BOARD) {
        estat = set_error(ESTAT_MANY_CARDS, "Too many cards in board.");
    } else {
        dk->dk_ncards -= ncards;
        for (ii = 0; ii < ncards; ii++) {
            card = dk->dk_card[dk->dk_ncards + ii];
            sl[(*ccount) + ii].sl_card[0] = card;
            sl[(*ccount) + ii].sl_ncard = 1;
            sl[(*ccount) + ii].sl_owner = PLAYER_NOBODY;
            sl[(*ccount) + ii].sl_type  = SLTYPE_SINGLE;
            sl[(*ccount) + ii].sl_value = GET_PIP(card);
        } 
        (*ccount)    += ncards;
    }

    return (estat);
}
/***************************************************************/
void show_cardlist(int msglevel, const char * pfx, card_t * cards, count_t ncards)
{
    count_t showmx;
    count_t cardix;
    count_t nshow;
    int blen;
    char buf[PBUF_SIZE];

    showmx = 20;
    cardix = 0;
    while (cardix < ncards) {
        strcpy(buf, pfx);
        blen = IStrlen(buf);
        nshow = ncards - cardix;
        if (nshow > showmx) nshow = showmx;
        deck_card_pr(buf + blen, cards + cardix, nshow, ' ');
        msgout(msglevel, "%s\n", buf);
        cardix += nshow;
    }
}
/***************************************************************/
void gstate_print(int msglevel, struct gstate * gst, int pflags) {

    char buf[PBUF_SIZE];

    if (pflags & FLAG_PLAYER_EAST) {
        deck_card_pr(buf, gst->gst_hand[PLAYER_EAST], gst->gst_nhand[PLAYER_EAST], ' ');
        msgout(msglevel, "  %s : %s\n", PLAYER_NAME(PLAYER_EAST), buf);
    }

    if (pflags & FLAG_PLAYER_WEST) {
        deck_card_pr(buf, gst->gst_hand[PLAYER_WEST], gst->gst_nhand[PLAYER_WEST], ' ');
        msgout(msglevel, "  %s : %s\n", PLAYER_NAME(PLAYER_WEST), buf);
    }

    if (pflags & FLAG_BOARD) {
        deck_board_pr(buf, gst->gst_board, gst->gst_nboard, pflags);
        msgout(msglevel, "  Board: %s\n", buf);
    }

    if (pflags & FLAG_PLAYER_TAKEN_EAST) {
        show_cardlist(msglevel, "  East taken: ", gst->gst_taken[PLAYER_EAST], gst->gst_ntaken[PLAYER_EAST]);
    }

    if (pflags & FLAG_PLAYER_TAKEN_WEST) {
        show_cardlist(msglevel, "  West taken: ", gst->gst_taken[PLAYER_WEST], gst->gst_ntaken[PLAYER_WEST]);
    }
}
/***************************************************************/
int game_deal_next_round(struct game * gm) {

    int estat = 0;

    if (gm->gm_gst.gst_nhand[PLAYER_EAST]) {
        estat = set_error(ESTAT_HASCARDS, "Player East cards.");
    } if (gm->gm_gst.gst_nhand[PLAYER_WEST]) {
        estat = set_error(ESTAT_HASCARDS, "Player West cards.");
    }

    gm->gm_round += 1;
    msgout(MSGLEVEL_VERBOSE, "**** Dealing round %d\n", gm->gm_round);

    estat = deal_cards(gm->gm_dk,
        gm->gm_gst.gst_hand[PLAYER_EAST],
        &(gm->gm_gst.gst_nhand[PLAYER_EAST]), CARDS_DEALT_PER_PLAYER);

    if (!estat) {
        estat = deal_cards(gm->gm_dk,
            gm->gm_gst.gst_hand[PLAYER_WEST],
            &(gm->gm_gst.gst_nhand[PLAYER_WEST]), CARDS_DEALT_PER_PLAYER);
    }

    return (estat);
}
/***************************************************************/
int game_deal_first_round(struct game * gm) {

    int estat = 0;

    gm->gm_round = 0;
    gstate_init(&(gm->gm_gst));

    estat = game_deal_next_round(gm);

    if (!estat) {
        estat = deal_board(gm->gm_dk,
            gm->gm_gst.gst_board,
            &(gm->gm_gst.gst_nboard), CARDS_DEALT_INIT_BOARD);
    }

    return (estat);
}
/***************************************************************/
int process_deal(struct globals * glb, const char *line, int * lix, char * stoke) {

    int estat = 0;

    if (!glb->glb_gm) {
        estat = set_error(ESTAT_NODECK,"Game unititialized.");
    } else {
        estat = game_deal_first_round(glb->glb_gm);
        gettoke(line, lix, stoke, STOKE_MAX_SIZE);
    }

    return (estat);
}
/***************************************************************/
int process_end(struct globals * glb, const char *line, int * lix, char * stoke) {

    int estat;

    estat = ESTAT_END;

    return (estat);
}
/***************************************************************/
count_t text_to_player(const char * stoke) {

	count_t player;
    int slen;

    player = PLAYER_NOBODY;
    slen = IStrlen(stoke);
    if (slen) {
        if (!Memicmp(stoke, "EAST", slen)) {
            player = PLAYER_EAST;
        } else if (!Memicmp(stoke, "WEST", slen)) {
            player = PLAYER_WEST;
        }
    }

    return (player);
}
/***************************************************************/
void deck_shuffle(void * rg, struct deck * dk) {

    int ii;
    int jj;
    int lmt;
    count_t tt;

    DESSERT(rg != NULL, "No random generator.");

    lmt = dk->dk_ncards - 1;

    for (ii = 0; ii < lmt; ii++) {
        jj = randbetween(rg, ii, dk->dk_ncards - 1);
        tt = dk->dk_card[ii];
        dk->dk_card[ii] = dk->dk_card[jj];
        dk->dk_card[jj] = tt;
    }
}
/***************************************************************/
void deck_reset_shuffle(void * rg, struct deck * dk) {

    deck_init(dk, FIFTY_TWO);
    deck_shuffle(rg, dk);
}
/***************************************************************/
int shuffle_parm(const char *shuf, int * rgen, int * rseed) {

    int estat = 0;
    int ix;
    int shix;
    char stoke[STOKE_MAX_SIZE + 2];

    (*rgen) = 0;
    (*rseed) = 0;
    shix = 0;
    ix = 0;
    while (ix < STOKE_MAX_SIZE && isdigit(shuf[shix])) {
        stoke[ix++] = shuf[shix++];
    }
    stoke[ix] = '\0';

    if (stoi(stoke, rgen)) {
        estat = set_error(ESTAT_BADNUM,"Unrecognized number: %s", stoke);
    } else if (shuf[shix]) {
        if (shuf[shix] != '.') {
            estat = set_error(ESTAT_EXPDOT, 
                    "Expecting dot (.). Found : %c", stoke);
        } else {
            shix++;
            ix = 0;
            while (ix < STOKE_MAX_SIZE && isdigit(shuf[shix])) {
                stoke[ix++] = shuf[shix++];
            }
            stoke[ix] = '\0';
            if (stoi(stoke, rseed)) {
                estat = set_error(ESTAT_BADNUM,"Unrecognized number: %s", stoke);
            } else if (shuf[shix]) {
                estat = set_error(ESTAT_BADNUM,"Unrecognized number: %s%s",
                    stoke, shuf + shix);
            }
        }
    }

    return (estat);
}
/***************************************************************/
struct playinfo * new_playinfo(struct globals * glb) {

    struct playinfo * pli;
    int ii;

    pli = New(struct playinfo, 1);
    pli->pli_player                 = PLAYER_EAST;
    pli->pli_play_duration          = PLAY_DURATION_TURN;
    pli->pli_msglevel               = global_msglevel;
    pli->pli_play_num_duration      = 1;
    pli->pli_play_matches           = 0;
    pli->pli_rgen                   = PLAY_DEFAULT_RGEN;
    pli->pli_rseed                  = PLAY_DEFAULT_RSEED;
    pli->pli_rg                     = NULL;

    for (ii = 0; ii < MAX_PLAYERS; ii++) {
        pli->pli_strat[ii] = strategy_new(glb);
    }

    return (pli);
}
/***************************************************************/
void free_playinfo(struct playinfo * pli) {

    count_t player;

    for (player = 0; player < MAX_PLAYERS; player++) {
        strat_shut(pli->pli_strat[player]);
    }
    if (pli->pli_rg) randintfree(pli->pli_rg);

    Free(pli);
}
/***************************************************************/
int parse_msglevel(struct playinfo * pli,
        const char *line, int * lix, char * stoke, int * msglevel)
{
    int estat = 0;

    (*msglevel) = 0;

    if (!strcmp(stoke, "VERBOSE")) {
        (*msglevel) = MSGLEVEL_VERBOSE_MASK;
        gettoke(line, lix, stoke, STOKE_MAX_SIZE);
    } else if (!strcmp(stoke, "DEFAULT")) {
        (*msglevel) = MSGLEVEL_DEFAULT_MASK;
        gettoke(line, lix, stoke, STOKE_MAX_SIZE);
    } else if (!strcmp(stoke, "TERSE")) {
        (*msglevel) = MSGLEVEL_TERSE_MASK;
        gettoke(line, lix, stoke, STOKE_MAX_SIZE);
    } else if (!strcmp(stoke, "QUIET")) {
        (*msglevel) = MSGLEVEL_QUIET_MASK;
        gettoke(line, lix, stoke, STOKE_MAX_SIZE);
    } else if (!strcmp(stoke, "OFF")) {
        (*msglevel) = MSGLEVEL_OFF;
        gettoke(line, lix, stoke, STOKE_MAX_SIZE);

#if IS_DEBUG
    } else if (!strcmp(stoke, "DEBUG")) {
        (*msglevel) = MSGLEVEL_DEBUG_MASK;
        gettoke(line, lix, stoke, STOKE_MAX_SIZE);
        while (!estat && !strcmp(stoke, ";")) {
            gettoke(line, lix, stoke, STOKE_MAX_SIZE);
            if (!strcmp(stoke, "SHOWAFTER")) {
                (*msglevel) |= MSGLEVEL_DEBUG_SHOWAFTER;
                gettoke(line, lix, stoke, STOKE_MAX_SIZE);
            } else if (!strcmp(stoke, "SHOWBEFORE")) {
                (*msglevel) |= MSGLEVEL_DEBUG_SHOWBEFORE;
                gettoke(line, lix, stoke, STOKE_MAX_SIZE);
            } else if (!strcmp(stoke, "SHOWDEALS")) {
                (*msglevel) |= MSGLEVEL_DEBUG_SHOWDEALS;
                gettoke(line, lix, stoke, STOKE_MAX_SIZE);
            } else if (!strcmp(stoke, "SHOWOPS")) {
                (*msglevel) |= MSGLEVEL_DEBUG_SHOWOPS;
                gettoke(line, lix, stoke, STOKE_MAX_SIZE);
            } else {
                estat = set_error(ESTAT_BADMSGS,"Unrecognized DEBUG message level: %s", stoke);
            }
        }
#endif

    } else {
        estat = set_error(ESTAT_BADMSGS,"Unrecognized message level: %s", stoke);
    }

    return (estat);
}
/***************************************************************/
int parse_duration(struct playinfo * pli,
        const char *line, int * lix, char * stoke)
{
    int estat = 0;
    int num;
    int ii;

    num = 0;
    ii = 0;
    while (isdigit(stoke[ii])) {
        num = (num * 10) + (stoke[ii] - '0');
        ii++;
    }

    if (stoke[ii]) {
        estat = set_error(ESTAT_BADNUMBER,"Invalid number: %s", stoke);
    } else {
        gettoke(line, lix, stoke, STOKE_MAX_SIZE);
        if (!strcmp(stoke, "TURN") || !strcmp(stoke, "TURNS")) {
            pli->pli_play_duration      = PLAY_DURATION_TURN;
            pli->pli_play_num_duration  = num;
            gettoke(line, lix, stoke, STOKE_MAX_SIZE);

        } else if (!strcmp(stoke, "ROUND") || !strcmp(stoke, "ROUNDS")) {
            pli->pli_play_duration      = PLAY_DURATION_ROUND;
            pli->pli_play_num_duration   = num;
            gettoke(line, lix, stoke, STOKE_MAX_SIZE);

        } else if (!strcmp(stoke, "GAME") || !strcmp(stoke, "GAMES")) {
            pli->pli_play_duration      = PLAY_DURATION_GAME;
            pli->pli_play_num_duration   = num;
            gettoke(line, lix, stoke, STOKE_MAX_SIZE);

        } else if (!strcmp(stoke, "MATCH") || !strcmp(stoke, "MATCHES")) {
            gettoke(line, lix, stoke, STOKE_MAX_SIZE);
            if (!strcmp(stoke, "OF")) {
                pli->pli_play_duration  = PLAY_DURATION_GAME_MATCH;
                pli->pli_play_matches   = num;
                gettoke(line, lix, stoke, STOKE_MAX_SIZE);
                num = 0;
                ii = 0;
                while (isdigit(stoke[ii])) {
                    num = (num * 10) + (stoke[ii] - '0');
                    ii++;
                }
                if (stoke[ii]) {
                    estat = set_error(ESTAT_BADNUMBER,"Invalid number: %s", stoke);
                } else {
                    pli->pli_play_num_duration   = num;
                    gettoke(line, lix, stoke, STOKE_MAX_SIZE);
                    if (!strcmp(stoke, "GAME") || !strcmp(stoke, "GAMES")) {
                        gettoke(line, lix, stoke, STOKE_MAX_SIZE);
                    } else {
                        estat = set_error(ESTAT_EXPGAMES,"Expecting GAMES. Found: %s", stoke);
                    }
                }
            } else if (!strcmp(stoke, "TO")) {
                pli->pli_play_duration  = PLAY_DURATION_POINT_MATCH;
                pli->pli_play_matches   = num;
                gettoke(line, lix, stoke, STOKE_MAX_SIZE);
                num = 0;
                ii = 0;
                while (isdigit(stoke[ii])) {
                    num = (num * 10) + (stoke[ii] - '0');
                    ii++;
                }
                if (stoke[ii]) {
                    estat = set_error(ESTAT_BADNUMBER,"Invalid number: %s", stoke);
                } else {
                    pli->pli_play_num_duration   = num;
                    gettoke(line, lix, stoke, STOKE_MAX_SIZE);
                    if (!strcmp(stoke, "POINT") || !strcmp(stoke, "POINTS")) {
                        gettoke(line, lix, stoke, STOKE_MAX_SIZE);
                    } else {
                        estat = set_error(ESTAT_EXPPOINTS,"Expecting POINTS. Found: %s", stoke);
                    }
                }
            } else {
                estat = set_error(ESTAT_BADDURATION,"Unrecognized duration: %s", stoke);
            }

        } else {
            estat = set_error(ESTAT_BADSTRAT,"Unrecognized strategy: %s", stoke);
        }
    }

    return (estat);
}
/***************************************************************/
int process_play(struct globals * glb, const char *line, int * lix, char * stoke)
{
/*
**  Syntax:
**
**      PLAY [DEALER player] | [TOPLAY player] 
**           [player strategy]
**           [FOR number duration]
**           [MSGS msglevel]
**           [SHUFFLE rgen [.rseed] ]
**
**          player          ::= EAST | WEST
**          strategy        ::= MENU | simple-strategy | best-strategy | CHEAT
**          number          ::= digit | digit ...
**          duration        ::= TURN[S] | ROUND[S] | GAME[S] |
**                              MATCH[ES] TO number POINTS |
**                              MATCH[ES] OF number GAME[S]
**          msglevel        ::= debug-msglevel | VERBOSE | DEFAULT | TERSE | QUIET | OFF
**
**          best-strategy   ::= BEST
**                              [ ; SHOWTOTS ]*
**                              [ ; SHOWEACHTOT ]*
**                              [ ; MINTRIES=minimum-tries ]*
**
**          simple-strategy ::= SIMPLE 
**                              [ ; POOL=pool-size ]*
**
**          debug-msglevel  ::= DEBUG
**                              [ ; SHOWAFTER ]*
**                              [ ; SHOWBEFORE ]*
**                              [ ; SHOWDEALS ]*
**
**          * = subject to change
**
**  Examples:
**      play dealer east, west simple, east best, for 3 turns, shuffle 1.4, msgs verbose 
**
*/
    int estat = 0;
	count_t player;
    int done;
    boolean_t set_msglevel;
    int save_global_msglevel;
    struct playinfo * pli;

    pli = new_playinfo(glb);
    set_msglevel = 0;

    gettoke(line, lix, stoke, STOKE_MAX_SIZE);
    done = 0;
    while (!estat && !done) {
        if (!stoke[0]) {
            done = 1;
        } else if (!strcmp(stoke, ",")) {
            gettoke(line, lix, stoke, STOKE_MAX_SIZE);
        } else if (!strcmp(stoke, "DEALER")) {
            gettoke(line, lix, stoke, STOKE_MAX_SIZE);
            player = text_to_player(stoke);
            if (player == PLAYER_NOBODY) {
                estat = set_error(ESTAT_BADPLAYER,"Invalid dealer: %s", stoke);
            } else {
                pli->pli_player = PLAYER_OPPONENT(player);
                gettoke(line, lix, stoke, STOKE_MAX_SIZE);
            }
        } else if (!strcmp(stoke, "TOPLAY")) {
            gettoke(line, lix, stoke, STOKE_MAX_SIZE);
            player = text_to_player(stoke);
            if (player == PLAYER_NOBODY) {
                estat = set_error(ESTAT_BADPLAYER,"Invalid player: %s", stoke);
            } else {
                pli->pli_player = player;
                gettoke(line, lix, stoke, STOKE_MAX_SIZE);
            }
        } else if (!strcmp(stoke, "FOR")) {
            gettoke(line, lix, stoke, STOKE_MAX_SIZE);
            estat = parse_duration(pli, line, lix, stoke);
        } else if (!strcmp(stoke, "SHUFFLE")) {
            gettoke(line, lix, stoke, STOKE_MAX_SIZE);
            estat = shuffle_parm(stoke, &(pli->pli_rgen), &(pli->pli_rseed));
            if (!estat) {
                pli->pli_rg = randintseed(pli->pli_rgen, pli->pli_rseed);
                if (!pli->pli_rg) {
                    estat = set_error(ESTAT_BADRGEN,"Unrecognized generator: %d", pli->pli_rgen);
                } else {
                    gettoke(line, lix, stoke, STOKE_MAX_SIZE);
                }
            }
        } else if (!strcmp(stoke, "MSGS")) {
            gettoke(line, lix, stoke, STOKE_MAX_SIZE);
            estat = parse_msglevel(pli, line, lix, stoke, &(pli->pli_msglevel));
            if (!estat) set_msglevel = 1;
        } else {    /* strategy for "EAST" or "WEST" */
            player = text_to_player(stoke);
            if (player == PLAYER_NOBODY) {
                estat = set_error(ESTAT_BADPLAYER,"Invalid player: %s", stoke);
            } else {
                gettoke(line, lix, stoke, STOKE_MAX_SIZE);
                estat = parse_strategy(pli->pli_strat[player],
                            line, lix, stoke);
            }
        }
    }

    if (!estat) {
        if (!glb->glb_gm) {
            game_new_glb(glb);
            game_set_rgen(glb->glb_gm, glb->glb_gm->gm_rgen, glb->glb_gm->gm_rseed);
            deck_init(glb->glb_gm->gm_dk, FIFTY_TWO);
        }
    }

    if (!estat) {
        if (pli->pli_rg) {
            glb->glb_gm->gm_erg = pli->pli_rg;
            deck_shuffle(glb->glb_gm->gm_erg, glb->glb_gm->gm_dk);
        } else if (glb->glb_gm->gm_rg) {
            glb->glb_gm->gm_erg = glb->glb_gm->gm_rg;
        }
    }

    if (!estat) {
        if (set_msglevel) {
            save_global_msglevel = global_msglevel;
            global_msglevel = pli->pli_msglevel;
        }
        estat = play_play(glb->glb_gm, pli);
        if (!estat) {
            gettoke(line, lix, stoke, STOKE_MAX_SIZE);
        }
        if (set_msglevel) {
            global_msglevel = save_global_msglevel;
        }
    }
    free_playinfo(pli);

    return (estat);
}
/***************************************************************/
#ifdef UNUSED
int process_prompt(struct globals * glb, const char *line, int * lix, char * stoke) {

    int estat = 0;
	count_t player;
    int playflags;
    int done;

    playflags = 0;
    gettoke(line, lix, stoke, STOKE_MAX_SIZE);
    player = text_to_player(stoke);
    if (player == PLAYER_NOBODY) {
        estat = set_error(ESTAT_BADPLAYER,"Invalid player: %s", stoke);
    } else {
        done = 0;
        while (!estat && !done) {
            gettoke(line, lix, stoke, STOKE_MAX_SIZE);
            if (!stoke[0]) {
                done = 1;
            } else if (!strcmp(stoke, "MENU")) {
                playflags |= PLAYFLAG_MENU;
            } else {
                estat = set_error(ESTAT_BADPROMPT,"Invalid PROMPT: %s", stoke);
            }
        }
    }

    if (!glb->glb_gm) {
        estat = set_error(ESTAT_NODECK,"Game unititialized.");
    } else {
        estat = play_prompt(glb, &(glb->glb_gm->gm_gst), player, playflags);
        if (!estat) {
            gettoke(line, lix, stoke, STOKE_MAX_SIZE);
        }
    }

    return (estat);
}
#endif
/***************************************************************/
int process_show(struct globals * glb, const char *line, int * lix, char * stoke) {

    int estat = 0;
    int pflags;
    int done;

    pflags = 0;

    done = 0;
    while (!estat && !done) {
        gettoke(line, lix, stoke, STOKE_MAX_SIZE);
        if (!stoke[0]) {
            done = 1;
        } else if (!strcmp(stoke, "EAST")) {
            pflags |= FLAG_PLAYER_EAST;
        } else if (!strcmp(stoke, "WEST")) {
            pflags |= FLAG_PLAYER_WEST;
        } else if (!strcmp(stoke, "DECK")) {
            pflags |= FLAG_PLAYER_DECK;
        } else if (!strcmp(stoke, "ETAKEN")) {
            pflags |= FLAG_PLAYER_TAKEN_EAST;
        } else if (!strcmp(stoke, "WEST")) {
            pflags |= FLAG_PLAYER_WEST;
        } else if (!strcmp(stoke, "WTAKEN")) {
            pflags |= FLAG_PLAYER_TAKEN_WEST;
        } else if (!strcmp(stoke, "BOARD")) {
            pflags |= FLAG_BOARD;
            pflags |= FLAG_BOARD_SHOW_OWNER;
        } else {
            estat = set_error(ESTAT_BADSHOW,"Invalid SHOW: %s", stoke);
        }
    }

    if (glb->glb_gm && pflags) {
        gstate_print(MSGLEVEL_TERSE, &(glb->glb_gm->gm_gst), pflags);
        if (pflags & FLAG_PLAYER_DECK) {
            show_cardlist(MSGLEVEL_TERSE, "  Deck: ",
                glb->glb_gm->gm_dk->dk_card, glb->glb_gm->gm_dk->dk_ncards);
        }

        gettoke(line, lix, stoke, STOKE_MAX_SIZE);
    }

    return (estat);
}
/***************************************************************/
void deck_print(struct deck * dk) {

    int ii;
    char buf[80];

    for (ii = 0; ii < 4; ii++) {
        deck_card_pr(buf, dk->dk_card + ii * 13, 13, ' ');
        printf("  %s\n", buf);
    }
}
/***************************************************************/
#if IS_DEBUG
static int dbg_g_max_opportunites = 0;

void dbg_set_max_opportunites(int maxop)
{
    dbg_g_max_opportunites = maxop;
}
#endif
/***************************************************************/
void init_opportunitylist(struct opportunitylist * opl)
{
    opl->opl_opi  = NULL;
    opl->opl_nopi = 0;
    opl->opl_xopi = 0;

#if IS_DEBUG
    if (!dbg_g_max_opportunites) dbg_set_max_opportunites(DEBUG_MAX_OPPORTUNITES);
    opl->opl_emsg = dbg_g_max_opportunites;
#endif
}
/***************************************************************/
void free_opportunities(struct opportunitylist * opl)
{
    int ii;

    for (ii = 0; ii < opl->opl_nopi; ii++) {
        Free(opl->opl_opi[ii]);
    }
    Free(opl->opl_opi);
}
/***************************************************************/
int process_opportunities(struct globals * glb, const char *line, int * lix, char * stoke) {

    int estat = 0;
	count_t player;
    struct opportunitylist opl;

    if (!glb->glb_gm) {
        estat = set_error(ESTAT_NODECK,"Game unititialized.");
    } else {
        gettoke(line, lix, stoke, STOKE_MAX_SIZE);
        player = text_to_player(stoke);
        if (player == PLAYER_NOBODY) {
            estat = set_error(ESTAT_BADPLAYER,"Invalid player: %s", stoke);
        } else {
            init_opportunitylist(&opl);
            game_opportunities(
                &opl,
                glb->glb_gm->gm_gst.gst_board,
                glb->glb_gm->gm_gst.gst_nboard,
                glb->glb_gm->gm_gst.gst_hand[player],
                glb->glb_gm->gm_gst.gst_nhand[player],
                player);

            show_opportunities(
                    &opl,
                    glb->glb_gm->gm_gst.gst_board,
                    glb->glb_gm->gm_gst.gst_nboard,
                    glb->glb_gm->gm_gst.gst_hand[player],
                    glb->glb_gm->gm_gst.gst_nhand[player],
                    player);

            gettoke(line, lix, stoke, STOKE_MAX_SIZE);
            free_opportunities(&opl);
        }
    }

    return (estat);
}
/***************************************************************/
int process_outfile(struct globals * glb, const char *line, int * lix, char * stoke) {

    int estat = 0;
    int rtn;
    int ern;
    FILE * outf;
    time_t uxtm;                                                                  
    struct tm *timeptr;                                                           
    char datbuf[64];
    char fname[LINE_MAX_SIZE];

    while (isspace(line[*lix])) (*lix)++;

    if (global_outfile) fclose(global_outfile);
    global_outfile = NULL;

    if (line[*lix]) {
        strcpy(fname, line + (*lix));
        outf = fopen(fname, "w");
        if (!outf) {
            ern = ERRNO;
            estat = set_error(ESTAT_FOPENOUT, "Error #%d opening %s: %s\n%s",
                ern, "output file", fname, strerror(ern));
        } else {
            rtn = fprintf(outf, "%s\n",
                "================================================================");
            if (rtn < 0) {
                ern = ERRNO;
                estat = set_error(ESTAT_FWRITEOUT, "Error #%d writing to %s: %s\n%s",
                    ern, "output file", fname, strerror(ern));
            } else {
                global_outfile = outf;
                                                                                
                uxtm = time(NULL);                                                            
                timeptr = localtime(&uxtm);                                                   
                strftime(datbuf,sizeof(datbuf)-1,"%Y/%m/%d %H:%M:%S", timeptr);
                fprintf(outf, "%s\n", datbuf);

                /* Done with this line */
                while (line[*lix]) (*lix)++;
                stoke[0] = '\0';
            }
        }
    }


    return (estat);
}
/***************************************************************/
int process_shuffle(struct globals * glb, const char *line, int * lix, char * stoke) {

    int estat = 0;
    int rgen;
    int rseed = 0;

    gettoke(line, lix, stoke, STOKE_MAX_SIZE);
    if (!stoke[0]) {
        if (!glb->glb_gm) {
            estat = set_error(ESTAT_NODECK,"Deck unititialized.");
        } else {
            deck_shuffle(glb->glb_gm->gm_rg, glb->glb_gm->gm_dk);
        }
    } else {
        estat = shuffle_parm(stoke, &rgen, &rseed);

        if (!estat) {
            if (!glb->glb_gm) game_new_glb(glb);
            game_set_rgen(glb->glb_gm, rgen, rseed);
            if (!glb->glb_gm->gm_rg) {
                estat = set_error(ESTAT_BADRGEN,"Unrecognized generator: %d", rgen);
            }
        }

        if (!estat) {
            gettoke(line, lix, stoke, STOKE_MAX_SIZE);

            deck_init(glb->glb_gm->gm_dk, FIFTY_TWO);
            deck_shuffle(glb->glb_gm->gm_rg, glb->glb_gm->gm_dk);
        }
    }

    return (estat);
}
/***************************************************************/
card_t parse_card(const char * stoke) {

    card_t card;
    char * cptr;
    count_t pip;
    count_t suit;

    card = 0;
    pip = 0;
    if (stoke[0] && stoke[1] && !stoke[2]) {
        cptr = strchr(pip_list, stoke[0]);
        if (cptr) {
            pip = (count_t)(cptr - pip_list);
            cptr = strchr(suit_list, stoke[1]);
            if (cptr) {
                suit = (count_t)(cptr - suit_list);
                if (pip >= ACE && pip <= KING) {
                    card = (pip << 2) | (suit & 3);
                }
            }
        }
    }

    return (card);
}
/***************************************************************/
int add_board_card(struct gstate * gst, card_t card) {

    int estat = 0;
    count_t pip;
    struct slot * sl;

    pip = GET_PIP(card);
    if (pip < ACE || pip > KING) {
        estat = set_error(ESTAT_BADCARD,"Invalid card number. Found: %d", (int)card);
    } else if (gst->gst_nboard >= MAX_SLOTS_IN_BOARD) {
        estat = set_error(ESTAT_OUT_OF_SLOTS,"Out of slots for card on board.");
    } else {
        sl = &(gst->gst_board[gst->gst_nboard]);
        sl->sl_card[0] = card;
        sl->sl_ncard   = 1;
        sl->sl_owner   = PLAYER_NOBODY;
        sl->sl_type    = SLTYPE_SINGLE;
        sl->sl_value   = pip;
        gst->gst_nboard += 1;
    }

    return (estat);
}
/***************************************************************/
int process_build(struct gstate * gst,
    count_t build_owner,
    const char *line,
    int * lix,
    char * stoke) {

    int estat = 0;
    int done;
    count_t adding;
    struct slot * sl = NULL;
    card_t build_value;
    card_t card = 0;
    card_t pip = 0;

    if (gst->gst_nboard >= MAX_SLOTS_IN_BOARD) {
        estat = set_error(ESTAT_OUT_OF_SLOTS,"Out of slots for build on board.");
    } else {
        sl = &(gst->gst_board[gst->gst_nboard]);
        gettoke(line, lix, stoke, STOKE_MAX_SIZE);
        card = parse_card(stoke);
        if (!card) {
            estat = set_error(ESTAT_BADCARD,"Invalid board card. Found: %s", stoke);
        } else {
            pip = GET_PIP(card);
            if (pip >= JACK) {
                estat = set_error(ESTAT_BUILDFACE,
                    "Cannot build with face cards. Found: %s", stoke);
            }
        }
    }

    if (!estat) {
        sl->sl_card[0] = card;
        sl->sl_ncard   = 1;
        sl->sl_owner   = build_owner;
        sl->sl_type    = SLTYPE_SINGLE;
        sl->sl_value   = pip;

        build_value = pip;
        done = 0;
        while (!estat && !done) {
            gettoke(line, lix, stoke, STOKE_MAX_SIZE);
            if (!stoke[0]) {
                estat = set_error(ESTAT_EXPCLOSE,"Expecting ] to end build.");
            } else if (stoke[0] == ']') {
                if (!build_value) {
                    sl->sl_type = SLTYPE_DOUBLED_BUILD;
                    gettoke(line, lix, stoke, STOKE_MAX_SIZE);
                    if (!stoke[0] || stoke[1]) {
                    } else if (stoke[0] == 'A') {
                        sl->sl_value = 1;
                    } else if (stoke[0] == 'T') {
                        sl->sl_value = 10;
                    } else if (stoke[0] >= '2' && stoke[0] <= '9') {
                        sl->sl_value = stoke[0] - '0';
                    } else {
                        estat = set_error(ESTAT_BUILDTOTAL,
                            "Bad build value. Found: %s", stoke);
                    }
                } else if (sl->sl_type == SLTYPE_DOUBLED_BUILD) {
                    if (build_value != sl->sl_value) {
                        estat = set_error(ESTAT_BUILDMISMATCH,
                            "Doubled build values do not match. Found: %d and %d",
                                (int)build_value, (int)sl->sl_value);
                    }
                }
                done = 1;
            } else if ((stoke[0] == '+') || (stoke[0] == ',')) {
                adding = 1;
                if (stoke[0] == ',') adding = 0;

                gettoke(line, lix, stoke, STOKE_MAX_SIZE);
                card = parse_card(stoke);
                if (!card) {
                    estat = set_error(ESTAT_BADCARD,"Invalid board card. Found: %s", stoke);
                } else {
                    pip = GET_PIP(card);
                    if (sl->sl_ncard >= MAX_CARDS_IN_SLOT) {
                        estat = set_error(ESTAT_BUILDCARDS,
                            "Too many cards in build. Found: %d", (int)MAX_CARDS_IN_SLOT);
                    } else {
                        if (adding) {   /* stoke[0] == '+' */
                            build_value += pip;
                            if (build_value > TEN) {
                                estat = set_error(ESTAT_BUILDTOTAL,
                                    "Cannot build to more than 10. Found: %s", stoke);
                            } else {
                                if (sl->sl_type != SLTYPE_DOUBLED_BUILD) {
                                    sl->sl_type = SLTYPE_BUILD;
                                    sl->sl_value = build_value;
                                }
                            }
                        } else {    /* stoke[0] == ',' */
                            if (sl->sl_type == SLTYPE_DOUBLED_BUILD) {
                                if (build_value != sl->sl_value) {
                                    estat = set_error(ESTAT_BUILDMISMATCH,
                                        "Doubled build values do not match. Found: %d and %d",
                                            (int)build_value, (int)sl->sl_value);
                                }
                            } else {
                                sl->sl_type = SLTYPE_DOUBLED_BUILD;
                                sl->sl_value = build_value;
                            }
                            build_value = pip;
                        }
                        sl->sl_card[sl->sl_ncard] = card;
                        sl->sl_ncard += 1;
                    }
                }
            } else if ((stoke[0] == ':')) {
                build_value = 0;
                gettoke(line, lix, stoke, STOKE_MAX_SIZE);
                card = parse_card(stoke);
                if (!card) {
                    estat = set_error(ESTAT_BADCARD,"Invalid board card. Found: %s", stoke);
                } else {
                    pip = GET_PIP(card);
                    if (sl->sl_ncard >= MAX_CARDS_IN_SLOT) {
                        estat = set_error(ESTAT_BUILDCARDS,
                            "Too many cards in build. Found: %d", (int)MAX_CARDS_IN_SLOT);
                    } else {
                        sl->sl_card[sl->sl_ncard] = card;
                        sl->sl_ncard += 1;
                    }
                }
            } else {
                estat = set_error(ESTAT_BADBUILDSTART,
                        "Expecting card or [. Found: %s", stoke);
            }
        }

        gst->gst_nboard += 1;
    }

    return (estat);
}
/***************************************************************/
int process_board_cards(struct gstate * gst, const char *line, int * lix, char * stoke) {

    int estat = 0;
    int done;
    card_t card;
    count_t build_owner;

    gettoke(line, lix, stoke, STOKE_MAX_SIZE);
    if (strcmp(stoke, ":")) {
        estat = set_error(ESTAT_EXPCOLON,"Expecting colon. Found: %s", stoke);
    } else {
        done = 0;
        while (!estat && !done) {
            gettoke(line, lix, stoke, STOKE_MAX_SIZE);
            if (!stoke[0]) {
                done = 1;
            } else if (!stoke[1]) {
                build_owner = PLAYER_NOBODY;
                if (stoke[0] == 'E') {
                    build_owner = PLAYER_EAST;
                    gettoke(line, lix, stoke, STOKE_MAX_SIZE);
                } else if (stoke[0] == 'W') {
                    build_owner = PLAYER_WEST;
                    gettoke(line, lix, stoke, STOKE_MAX_SIZE);
                }
                if (stoke[0] != '[') {
                    estat = set_error(ESTAT_BADBUILDSTART,
                            "Expecting card or [. Found: %s", stoke);
                } else {
                    estat = process_build(gst, build_owner, line, lix, stoke);
                }
            } else {
                card = parse_card(stoke);
                if (!card) {
                    estat = set_error(ESTAT_BADCARD,"Invalid board card. Found: %s", stoke);
                } else {
                    estat = add_board_card(gst, card);
                }
            }
        }
    }


    return (estat);
}
/***************************************************************/
int process_board(struct globals * glb, const char *line, int * lix, char * stoke) {

    int estat = 0;

    if (!glb->glb_gm) glb->glb_gm = game_new();

    estat = process_board_cards(&(glb->glb_gm->gm_gst), line, lix, stoke);

    if (!estat) {
        if (!glb->glb_gm->gm_round) glb->glb_gm->gm_round = 1; /* prevent shuffle */
    }

    return (estat);
}
/***************************************************************/
int add_deck_card(struct deck * dk, card_t card) {

    int estat = 0;

    if (card < ACE_OF_CLUBS || card > KING_OF_SPADES) {
        estat = set_error(ESTAT_BADCARD,"Invalid card number. Found: %d", (int)card);
    } else if (dk->dk_ncards >= FIFTY_TWO) {
        estat = set_error(ESTAT_BADCARDCOUNT,"Too many cards for deck");
    } else {
        dk->dk_card[dk->dk_ncards] = card;
        dk->dk_ncards += 1;
    }

    return (estat);
}
/***************************************************************/
int process_deck(struct globals * glb, const char *line, int * lix, char * stoke) {

    int estat = 0;
    int done;
    card_t card;

    gettoke(line, lix, stoke, STOKE_MAX_SIZE);
    if (strcmp(stoke, ":")) {
        estat = set_error(ESTAT_EXPCOLON,"Expecting colon. Found: %s", stoke);
    } else {
        done = 0;
        while (!estat && !done) {
            gettoke(line, lix, stoke, STOKE_MAX_SIZE);
            if (!stoke[0]) {
                done = 1;
            } else {
                card = parse_card(stoke);
                if (!card) {
                    estat = set_error(ESTAT_BADCARD,"Invalid card. Found: %s", stoke);
                } else {
                    estat = add_deck_card(glb->glb_gm->gm_dk, card);
                }
            }
        }
    }

    return (estat);
}
/***************************************************************/
int add_player_card(struct gstate * gst, count_t player, card_t card) {

    int estat = 0;

    if (card < ACE_OF_CLUBS || card > KING_OF_SPADES) {
        estat = set_error(ESTAT_BADCARD,"Invalid card number. Found: %d", (int)card);
    } else if (player >= MAX_PLAYERS) {
        estat = set_error(ESTAT_BADPLAYERNO,"Invalid player number. Found: %d", (int)player);
    } else if (gst->gst_nhand[player] >= CARDS_PER_PLAYER) {
        estat = set_error(ESTAT_BADCARDCOUNT,"Too many cards for player", CARDS_PER_PLAYER);
    } else {
        gst->gst_hand[player][gst->gst_nhand[player]] = card;
        gst->gst_nhand[player] += 1;
    }

    return (estat);
}
/***************************************************************/
int process_player_cards(struct gstate * gst, const char *line, int * lix, char * stoke,
            count_t player) {

    int estat = 0;
    int done;
    card_t card;

    gettoke(line, lix, stoke, STOKE_MAX_SIZE);
    if (strcmp(stoke, ":")) {
        estat = set_error(ESTAT_EXPCOLON,"Expecting colon. Found: %s", stoke);
    } else {
        done = 0;
        while (!estat && !done) {
            gettoke(line, lix, stoke, STOKE_MAX_SIZE);
            if (!stoke[0]) {
                done = 1;
            } else {
                card = parse_card(stoke);
                if (!card) {
                    estat = set_error(ESTAT_BADCARD,"Invalid card. Found: %s", stoke);
                } else {
                    estat = add_player_card(gst, player, card);
                }
            }
        }
    }

    return (estat);
}
/***************************************************************/
int process_east(struct globals * glb, const char *line, int * lix, char * stoke) {

    int estat = 0;

    if (!glb->glb_gm) glb->glb_gm = game_new();

    estat = process_player_cards(&(glb->glb_gm->gm_gst), line, lix, stoke, PLAYER_EAST);

    return (estat);
}
/***************************************************************/
int process_init(struct globals * glb, const char *line, int * lix, char * stoke) {

    int estat = 0;

    if (!glb->glb_gm) glb->glb_gm = game_new();

    game_init(glb->glb_gm);
    gettoke(line, lix, stoke, STOKE_MAX_SIZE);

    return (estat);
}
/***************************************************************/
int process_west(struct globals * glb, const char *line, int * lix, char * stoke) {

    int estat = 0;

    if (!glb->glb_gm) glb->glb_gm = game_new();

    estat = process_player_cards(&(glb->glb_gm->gm_gst), line, lix, stoke, PLAYER_WEST);

    return (estat);
}
/***************************************************************/
int process_in_line(struct globals * glb, const char *line, int * lix) {

    int estat = 0;
    char stoke[STOKE_MAX_SIZE];

    gettoke(line, lix, stoke, STOKE_MAX_SIZE);
    if (!strcmp(stoke, "BOARD")) {
        estat = process_board(glb, line, lix, stoke);
    } else if (!strcmp(stoke, "DEAL")) {
        estat = process_deal(glb, line, lix, stoke);
    } else if (!strcmp(stoke, "DECK")) {
        estat = process_deck(glb, line, lix, stoke);
    } else if (!strcmp(stoke, "EAST")) {
        estat = process_east(glb, line, lix, stoke);
    } else if (!strcmp(stoke, "END")) {
        estat = process_end(glb, line, lix, stoke);
    } else if (!strcmp(stoke, "INIT")) {
        estat = process_init(glb, line, lix, stoke);
    } else if (!strcmp(stoke, "PLAY")) {
        estat = process_play(glb, line, lix, stoke);
#ifdef UNUSED
    } else if (!strcmp(stoke, "PROMPT")) {
        estat = process_prompt(glb, line, lix, stoke);
#endif
    } else if (!strcmp(stoke, "OUTFILE")) {
        estat = process_outfile(glb, line, lix, stoke);
    } else if (!strcmp(stoke, "OPPORTUNITIES")) {
        estat = process_opportunities(glb, line, lix, stoke);
    } else if (!strcmp(stoke, "SHOW")) {
        estat = process_show(glb, line, lix, stoke);
    } else if (!strcmp(stoke, "SHUFFLE")) {
        estat = process_shuffle(glb, line, lix, stoke);
    } else if (!strcmp(stoke, "WEST")) {
        estat = process_west(glb, line, lix, stoke);
    } else {
        estat = set_error(ESTAT_BADSTMT,"Unrecognized statement: %s", stoke);
    }

    if (!estat && stoke[0]) {
        estat = set_error(ESTAT_EXPEOL,"Expecting end of line. Found: %s", stoke);
    }

    return (estat);
}
/***************************************************************/
int process_in_parm(struct globals * glb, const char *parm) {

    int estat = 0;
    int done;
    int lix;
    struct frec * in_fr;
    char line[LINE_MAX_SIZE];

    in_fr = frec_open(parm, "r", "input file");
    if (in_fr) {

        done = 0;
        while (!done && !estat) {
            lix = frec_read_line(line, sizeof(line), in_fr);
            if (lix < 0) {
                if (in_fr->fr_eof) done = 1;
                else               estat = -1;
            } else {
                lix = 0;
                while (line[lix] && isspace(line[lix])) lix++;
                if (line[lix] && line[lix] != '#') {
                    estat = process_in_line(glb, line, &lix);
                }
            }
        }

        frec_close(in_fr);
    }

    return (estat);
}
/***************************************************************/
int process_parms(struct globals * glb, char ** aparm, int nparm) {

    int estat = 0;
    int ii;

    for (ii = 0; !estat && ii < nparm; ii++) {
        estat = process_in_parm(glb, aparm[ii]);
    }

    return (estat);
}
/***************************************************************/
int process(struct globals * glb) {

	int estat = 0;

	estat = process_parms(glb, glb->glb_parms, glb->glb_nparms);

	return (estat);
}
/***************************************************************/
char * parse_command_line(const char * cmd, int * pargc, const char * arg0, int flags) {

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
        arg0len = strlen(arg0) + 1;
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
int getargs(struct globals * glb, int argc, char *argv[]);
/***************************************************************/
int parse_parmfile(struct globals * glb, const char * parmfname) {

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
				ix = strlen(fbuf);
				while (ix > 0 && isspace(fbuf[ix - 1])) ix--;
				fbuf[ix] = '\0';
				ix = 0;
				while (isspace(fbuf[ix])) ix++;
				if (fbuf[ix] && fbuf[ix] != '#') {
					parmbuf = parse_command_line(fbuf, &parmc, "", 0);
					if (parmbuf && parmc) {
						argv = (char**)parmbuf;
						estat = getargs(glb, parmc, argv);
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
int getargs(struct globals * glb, int argc, char *argv[]) {

	int ii;
	int estat = 0;

	for (ii = 1; !estat && ii < argc; ii++) {
		if (argv[ii][0] == '-') {

			if (!istrcmp(argv[ii], "-pause")) {
				glb->glb_flags |= OPT_PAUSE;

			} else if (!istrcmp(argv[ii], "-help")) {
				glb->glb_flags |= OPT_HELP | OPT_DONE;

			} else if (!istrcmp(argv[ii], "-version")) {
				glb->glb_flags |= OPT_VERSION | OPT_DONE;

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

	if (glb->glb_flags & OPT_HELP) {
		showusage(argv[0]);
	}

	return (estat);
}
/***************************************************************/
int main(int argc, char * argv[])
{
	int estat;
	struct globals * glb;
	int err_num = 0;

	if (argc <= 1) {
		showusage(argv[0]);
		return (0);
	}

    global_msglevel = MSGLEVEL_DEFAULT_MASK;

	glb = globals_new();

	estat = getargs(glb, argc, argv);

	g_do_pause = (glb->glb_flags & OPT_PAUSE);

	if (!estat && !(glb->glb_flags & OPT_DONE)) {
		estat = process(glb);
	}

	if (glb->glb_flags & OPT_VERSION) {
		printf("%s - %s\n", PROGRAM_NAME, VERSION);
	}

    if (estat < 0) err_num = 1;

	globals_free(glb);
	shut();

	if (g_do_pause) {
		char inbuf[100];
		printf("Press ENTER to continue...");
		fgets(inbuf, sizeof(inbuf), stdin);
	}

	return (err_num);
}
/**************************************************************/
