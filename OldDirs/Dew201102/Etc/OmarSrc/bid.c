/***************************************************************/
/* bid.c                                                       */
/***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "config.h"
#include "omar.h"
#include "bid.h"
#include "util.h"
#include "natural.h"
#include "handrec.h"

#define PRINTHANDS 0

char display_rank[] = "0x23456789TJQKA";
char display_suit[] = "CDHSN";
char display_player[] = "WNES";

#if 0
  To do:
    merge apply_requirements & apply_sets()
#endif

struct infile { /* inf_ */
    struct frec * inf_fr;
    char        * inf_buf;
    int           inf_bufsz;
    int           inf_buflen;
    int           inf_bufptr;
};

/***************************************************************/
struct infile * infile_new(struct frec * inf_fr)
{
    struct infile * inf;

    inf = New(struct infile, 1);
    inf->inf_fr     = inf_fr;
    inf->inf_bufsz  = 256;
    inf->inf_buf    = New(char, inf->inf_bufsz);
    inf->inf_bufptr = 0;
    inf->inf_buflen = 0;

    return (inf);
}
/***************************************************************/
struct infile * infile_new_chars(const char * istr)
{
    struct infile * inf;
    int ilen;

    ilen = IStrlen(istr);

    inf = New(struct infile, 1);
    inf->inf_fr     = NULL;
    inf->inf_bufsz  = ilen + 1;
    inf->inf_buf    = New(char, inf->inf_bufsz);
    inf->inf_bufptr = 0;
    inf->inf_buflen = ilen;
    strcpy(inf->inf_buf, istr);

    return (inf);
}
/***************************************************************/
void infile_free(struct infile * inf, int close_fr)
{
    if (inf->inf_fr && close_fr) frec_close(inf->inf_fr);

    Free(inf->inf_buf);
    Free(inf);
}
/***************************************************************/
/*@*/int duplicate_score_making(int made, int bid_level, int bid_suit, int doubled, int vul) {

    int pts;
    int bonus;
    int pts_per[5] = {20, 20, 30, 30, 30};
    int ft[5]      = {0, 0, 0, 0, 10};

    pts = ft[bid_suit] + made * pts_per[bid_suit];
    if (doubled) pts = pts * (doubled * 2);
    if (pts < 100) {
        bonus = 50;
    } else {
        if (vul) bonus = 500; else bonus = 300;
        if (bid_level == 6) {
            if (vul) bonus += 500; else bonus += 300;
        } else if (bid_level == 7) {
            if (vul) bonus += 1500; else bonus += 1000;
        }
    }
    if (doubled) bonus += 50 * doubled; /* for the insult */
    pts += bonus;

    return (pts);
}
/***************************************************************/
/*@*/int duplicate_score_down(int nundertricks, int bid_suit, int doubled, int vul) {

    int pts;

    if (doubled == 0) {
        if (vul) pts =  50 * nundertricks;
        else     pts = 200 * nundertricks - 100;
    } else {
        if (vul) {
            pts = 300 * nundertricks - 100;
        } else {
            if (nundertricks <= 3) {
                pts = 200 * nundertricks - 100;
            } else {
                pts = 300 * nundertricks - 400;
            }
        }
        if (doubled == 2) pts *= 2;
    }

    return (pts);
}
/***************************************************************/
/*@*/int duplicate_score(int trickstaken, int encbid, int doubled, int vul) {
/*
** See: https://en.wikipedia.org/wiki/Bridge_scoring
**
** doubled:
**      0 - not doubled
**      1 - doubled
**      2 - redoubled
*/
    int bid_type;
    int bid_level;
    int bid_suit;
    int pts;
    int overunder;

    decode_bid(encbid, &bid_type, &bid_level, &bid_suit);

    if (bid_type != BID || bid_level < 1 || bid_level > 7 || bid_suit < 0|| bid_suit > 4) {
        pts = 0;
    } else {
        overunder = trickstaken - (bid_level + 6);
        if (overunder < 0) {
            pts = duplicate_score_down(-overunder, bid_suit, doubled, vul);
        } else {
            pts = duplicate_score_making(trickstaken - 6, bid_level, bid_suit, doubled, vul);
        }
    }

    return (pts);
}
/***************************************************************/
/* original bd.c                                               */
/***************************************************************/
/*@*/int score_making(int bid, int vul) {

    int bid_type;
    int bid_level;
    int bid_suit;
    int pts;
    int bonus;
    int pts_per[5] = {20, 20, 30, 30, 30};
    int y[5]       = {0, 0, 0, 0, 10};

    decode_bid(bid, &bid_type, &bid_level, &bid_suit);

    if (bid_type != BID || !bid_level) return (0);
    else {
        pts = y[bid_suit] + bid_level * pts_per[bid_suit];
        if (pts < 100)  bonus = 50;
        else {
            bonus = (vul?500:300);
            if (bid_level == 6) bonus += (vul?750:500);
            else if (bid_level == 7) bonus += (vul?1500:1000);
        }

        return (pts + bonus);
    }
}
/***************************************************************/
/*@*/int tab(char *buf, int num) {

   int len;
   int ii;

   len = IStrlen(buf);
   if (len < num) {
       for (ii=len;ii<num;ii++) buf[ii]=' ';
       buf[num]=0;
       return (num);
   } else {
       return (len);
   }
}
/***************************************************************/
/*
void print_hands(struct omarglobals * og, HANDS *hands) {

    int player;
    int suit;
    int card;
    char buf[80];
    int  len;

    player = 1;
    for (suit=3; suit>=0; suit--) {
        buf[0] = 0;
        len = tab(buf, 10);
        buf[len++] = display_suit[suit];
        buf[len++] = ' ';
        for (card=0; card < (*hands)[player].length[suit]; card++) {
            buf[len++] = display_rank[(*hands)[player].cards[suit][card]];
        }
        buf[len++] = '\0';
        omarout(og, "%s\n", buf);
    }

    for (suit=3; suit>=0; suit--) {
        buf[0] = 0;
        len = tab(buf, 4);
        player = 0;
        while (player <= 2) {
            buf[len++] = display_suit[suit];
            buf[len++] = ' ';
            for (card=0; card < (*hands)[player].length[suit]; card++) {
                buf[len++] = display_rank[(*hands)[player].cards[suit][card]];
            }
            buf[len++] = '\0';
            len = tab(buf, 16);
            player += 2;
        }
        omarout(og, "%s\n", buf);
    }

    player = 3;
    for (suit=3; suit>=0; suit--) {
        buf[0] = 0;
        len = tab(buf, 10);
        buf[len++] = display_suit[suit];
        buf[len++] = ' ';
        for (card=0; card < (*hands)[player].length[suit]; card++) {
            buf[len++] = display_rank[(*hands)[player].cards[suit][card]];
        }
        buf[len++] = '\0';
        omarout(og, "%s\n", buf);
    }
}
*/
/***************************************************************/
char * hands_to_chars(HANDS *hands) {

    int player;
    int suit;
    int card;
    char * handstr;
    int handlen;
    int handmax;
    char ch;

    handstr = NULL;
    handlen = 0;
    handmax = 0;

    player = 1;
    for (suit=3; suit>=0; suit--) {
        append_chars_rept(&handstr, &handlen, &handmax, ' ', 10);
        ch = display_suit[suit];
        append_chars(&handstr, &handlen, &handmax, &ch, 1);
        append_chars(&handstr, &handlen, &handmax, " ", 1);
        for (card=0; card < (*hands)[player].length[suit]; card++) {
            ch = display_rank[(*hands)[player].cards[suit][card]];
            append_chars(&handstr, &handlen, &handmax, &ch, 1);
        }
        append_chars(&handstr, &handlen, &handmax, "\n", 1);
    }

    for (suit=3; suit>=0; suit--) {
        append_chars_rept(&handstr, &handlen, &handmax, ' ', 4);
        player = 0;
        while (player <= 2) {
            ch = display_suit[suit];
            append_chars(&handstr, &handlen, &handmax, &ch, 1);
            append_chars(&handstr, &handlen, &handmax, " ", 1);
            for (card=0; card < (*hands)[player].length[suit]; card++) {
                ch = display_rank[(*hands)[player].cards[suit][card]];
                append_chars(&handstr, &handlen, &handmax, &ch, 1);
            }
            append_chars_rept(&handstr, &handlen, &handmax, ' ',
                16 - (*hands)[player].length[suit]);
            player += 2;
        }
        append_chars(&handstr, &handlen, &handmax, "\n", 1);
    }

    player = 3;
    for (suit=3; suit>=0; suit--) {
        append_chars_rept(&handstr, &handlen, &handmax, ' ', 10);
        ch = display_suit[suit];
        append_chars(&handstr, &handlen, &handmax, &ch, 1);
        append_chars(&handstr, &handlen, &handmax, " ", 1);
        for (card=0; card < (*hands)[player].length[suit]; card++) {
            ch = display_rank[(*hands)[player].cards[suit][card]];
            append_chars(&handstr, &handlen, &handmax, &ch, 1);
        }
        append_chars(&handstr, &handlen, &handmax, "\n", 1);
    }

    return (handstr);
}
/***************************************************************/
void print_hands(struct omarglobals * og, HANDS *hands) {

    char * hands_str;

    hands_str = hands_to_chars(hands);

    omarout(og, "%s\n", hands_str);
    Free(hands_str);
}
/***************************************************************/
/*@*/int encode_contract(
    int c_level,
    int c_suit,         /* enum e_bid_suit    */
    int c_bid_type,     /* enum e_bid_special */
    int c_declarer)     /* enum e_player */
{
    int econtract;  /* 11 bits */

    econtract =
        (c_declarer << 9) |   /* 2 bits */
        (c_bid_type << 6) |   /* 3 bits */
        (c_level    << 3) |   /* 3 bits */
        (c_suit         ) ;   /* 3 bits */

    return (econtract);
}
/***************************************************************/
/*@*/void decode_contract(int econtract,
                     int *c_level,
                     int *c_suit,
                     int *c_bid_type,
                     int *c_declarer) {

    (*c_declarer) = (econtract >> 9) & 3;
    (*c_bid_type) = (econtract >> 6) & 7;
    (*c_level)    = (econtract >> 3) & 7;
    (*c_suit)     = (econtract     ) & 7;
}
/***************************************************************/
void current_contract(AUCTION *  auction,   /* in  */
                      CONTRACT *contract,   /* out */
                      int * more_bids)      /* out */
{
/*
**  Returns:
**      Contract with
**      contract->doubled = PASS    - 1-3 passes at start, more passes available
**      contract->doubled = NOBID   - 4 passes at start, passed out
*/
    int bidix;
    int npass;
    int bid_type;
    int bid_level;
    int bid_suit;

    contract->cont_level     = NONE;
    contract->cont_suit      = 0;
    contract->cont_doubled   = 0;
    contract->cont_declarer  = 0;
    (*more_bids)             = 0;

    npass = 0;
    bidix = auction->au_nbids - 1;
    while (bidix >= 0 && contract->cont_level == NONE) {
        decode_bid(auction->au_bid[bidix], &bid_type, &bid_level, &bid_suit);
        if (bid_type == PASS) {
            npass++;
            if (npass == 3) (*more_bids) = 0;
            bidix--;
        } else if (bid_type == DOUBLE) {
            contract->cont_doubled = DOUBLE;
            npass = 0;
            (*more_bids) = 1;
            bidix--;
        } else if (bid_type == REDOUBLE) {
            contract->cont_doubled = REDOUBLE;
            npass = 0;
            (*more_bids) = 1;
            bidix--;
        } else {
            contract->cont_level     = bid_level;
            contract->cont_suit      = bid_suit;
            contract->cont_declarer  = DECLARER(bidix + auction->au_dealer);
            npass               = 0;
            (*more_bids)        = 1;
            if (contract->cont_doubled == 0) contract->cont_doubled = BID;
        }
    }

    if (contract->cont_level == NONE) {
        if (npass < 3 || bidix < 0) {
            contract->cont_doubled = PASS;
            (*more_bids)      = 1;
        } else {
            contract->cont_doubled = NOBID;
            (*more_bids)      = 0;
        }
    }
}
/***************************************************************/
void next_legal_bid(AUCTION *  auction,     /* in  */
                    CONTRACT * contract,    /* out */
                    int      * double_bid)  /* out */
{
/*
**  Returns:
**      Contract with
**      contract->doubled = PASS    - Bidding at 7NT
**      contract->doubled = NOBID   - Bidding done
*/
    int more_bids;
    int bidder;
    CONTRACT curr_cont;

    contract->cont_level     = NONE;
    contract->cont_suit      = 0;
    contract->cont_doubled   = 0;
    contract->cont_declarer  = 0;
    (*double_bid)       = 0;
    bidder              = DECLARER(auction->au_nbids + auction->au_dealer);

    current_contract(auction, &curr_cont, &more_bids);

    if (!more_bids) {
        contract->cont_level     = NONE;
        contract->cont_doubled   = NOBID;
    } else {
        if (curr_cont.cont_level == NONE) {
            contract->cont_level     = 1;
            contract->cont_suit      = CLUBS;
            contract->cont_doubled   = BID;
            contract->cont_declarer  = bidder;
        } else if (contract->cont_suit == NOTRUMP) {
            if (curr_cont.cont_level == 7) {
                contract->cont_level     = NONE;
                contract->cont_doubled   = PASS;
            } else {
                contract->cont_level     = curr_cont.cont_level + 1;
                contract->cont_suit      = CLUBS;
                contract->cont_doubled   = BID;
                contract->cont_declarer  = bidder;
            }
        } else {
            contract->cont_level     = curr_cont.cont_level;
            contract->cont_suit      = curr_cont.cont_suit + 1;
            contract->cont_doubled   = BID;
            contract->cont_declarer  = bidder;
        }

        if (curr_cont.cont_doubled == BID) {
            if (curr_cont.cont_declarer == RHO(bidder) ||
                curr_cont.cont_declarer == LHO(bidder)) {
                (*double_bid)       = DOUBLE;
            }
        } else if (curr_cont.cont_doubled == DOUBLE) {
            if (curr_cont.cont_declarer == bidder ||
                curr_cont.cont_declarer == PARTNER(bidder)) {
                (*double_bid)       = REDOUBLE;
            }
        }
    }
}
/***************************************************************/
int is_bid_permitted(AUCTION *  auction, int ebid)
{
    int bid_type;
    int bid_level;
    int bid_suit;
    int double_bid;
    int is_permitted;
    CONTRACT next_cont;

    is_permitted = 0;

    decode_bid(ebid, &bid_type, &bid_level, &bid_suit);
    next_legal_bid(auction, &next_cont, &double_bid);

    switch (bid_type) {
        case NONE:
        case NOBID:
            if (next_cont.cont_level == NONE) is_permitted = 1;
            break;
        case PASS:
            if (next_cont.cont_doubled != NOBID) is_permitted = 1;
            break;
        case DOUBLE:
            if (double_bid == DOUBLE) is_permitted = 1;
            break;
        case REDOUBLE:
            if (double_bid == REDOUBLE) is_permitted = 1;
            break;
        case BID:
            if (next_cont.cont_level > 0) {
                if (bid_level > next_cont.cont_level) {
                    if (bid_level <= 7) is_permitted = 1;
                } else if (bid_level > next_cont.cont_level) {
                    if (bid_suit > next_cont.cont_suit) is_permitted = 1;
                }
            }
            break;
        default:
            break;
    }

    return (is_permitted);
}
/***************************************************************/
/*@*/int encode_bid(int bid_type, int bid_level, int bid_suit) {
/*
** Result:
**       PASS       -> 2
**       DOUBLE     -> 3
**       REDOUBLE   -> 4
**       1C         -> 8
**       1D         -> 9
**       1H         -> 10
**       1S         -> 11
**       1N         -> 12
**       2C         -> 16
**
**       2S         -> 19
**       2N         -> 20
**       3C         -> 24
**
**       3N         -> 28
**
**       6N         -> 52
**
**       7N         -> 60
*/
    if (bid_type == BID) return ((bid_level << 3) + bid_suit);
    else                 return (bid_type);
}
/***************************************************************/
/*@*/void decode_bid(int bid,
                     int *bid_type,
                     int *bid_level,
                     int *bid_suit) {

    *bid_level = bid >> 3;
    if (*bid_level) {
        *bid_type = BID;
        *bid_suit = bid & 7;
    } else {
        *bid_type = bid;
        *bid_suit = 0;
    }
}
/***************************************************************/
int bid_is_pass(int bid)
{
    int is_pass;

    is_pass = 0;
    if (bid == PASS) is_pass = 1;

    return (is_pass);
}
/***************************************************************/
/*@*/void print_bid(struct omarglobals * og, int bid) {

    int bt;
    int bl;
    int bs;

    decode_bid(bid, &bt, &bl, &bs);

    switch (bt) {
        case NONE    : omarout(og, "None "); break;
        case PASS    : omarout(og, "Pass "); break;
        case DOUBLE  : omarout(og, "Dbl  "); break;
        case REDOUBLE: omarout(og, "Rdbl "); break;
        case BID     :
            omarout(og, "%c%c   ", (char)(bl) + '0', display_suit[bs]);
            break;
        default      : omarout(og, "??   "); break;
    }
}
/***************************************************************/
/*@*/int alldigits(char * buf, int buflen) {

    int all;
    int ii;

    all = 1;
    for (ii = 0; all && ii < buflen; ii++) {
        if (!isdigit(buf[ii])) all = 0;
    }

    return (all);
}
/***************************************************************/
#if 0
void get_cwd(char *cwd, int cwdmax) {

    if (!_getcwd(cwd, cwdmax)) {
        cwd[0] = '\0';
    }
}
/***************************************************************/
/*@*/char * current_dir(void) {
/*
** Can be called from Windows while testing.
*/

    char * out;
    static char cwd[256];

    get_cwd(cwd, sizeof(cwd));
    out = cwd;

    return (out);
}
#endif
/***************************************************************/
/*@*/void next_token(struct infile * inf, char * toke) {

    int tokelen;
    int ok;
    char c;

    while (inf->inf_bufptr < inf->inf_buflen && isspace(inf->inf_buf[inf->inf_bufptr])) inf->inf_bufptr++;
    if (inf->inf_bufptr < inf->inf_buflen-1 &&
        inf->inf_buf[inf->inf_bufptr] == '/' &&
        inf->inf_buf[inf->inf_bufptr+1] == '/') inf->inf_bufptr = inf->inf_buflen;

    while (inf->inf_bufptr >= inf->inf_buflen) {

        toke[0] = '\0';

        ok = frec_gets(inf->inf_buf, inf->inf_bufsz, inf->inf_fr);
        if (ok < 0) return ;

        inf->inf_buflen = IStrlen(inf->inf_buf);
        inf->inf_bufptr = 0;

        if (inf->inf_buflen && inf->inf_buf[0] == '#') inf->inf_buflen = 0;

        while (inf->inf_bufptr < inf->inf_buflen && isspace(inf->inf_buf[inf->inf_bufptr])) inf->inf_bufptr++;
        if (inf->inf_bufptr < inf->inf_buflen-1 &&
            inf->inf_buf[inf->inf_bufptr] == '/' &&
            inf->inf_buf[inf->inf_bufptr+1] == '/') inf->inf_bufptr = inf->inf_buflen;
    }

    tokelen = 0;
    c = inf->inf_buf[inf->inf_bufptr];
    while ((c >= 'A' && c <= 'Z') ||
           (c >= 'a' && c <= 'z') ||
           (c >= '0' && c <= '9') ||
           (c == '_')            ) {
        toke[tokelen++] = toupper(c);
        inf->inf_bufptr++;
        c = inf->inf_buf[inf->inf_bufptr];
    }

    if (c == '.' && alldigits(toke, tokelen)) {
        toke[tokelen++] = c;
        inf->inf_bufptr++;
        c = inf->inf_buf[inf->inf_bufptr];
        while (c >= '0' && c <= '9') {
            toke[tokelen++] = c;
            inf->inf_bufptr++;
            c = inf->inf_buf[inf->inf_bufptr];
        }
    }

    if (!tokelen) {
        toke[tokelen++] = c;
        inf->inf_bufptr++;
        c = inf->inf_buf[inf->inf_bufptr];
        if ((toke[0] == '-' && c == '>') ||
            (toke[0] == '<' && (c == '>' || c == '=')) ||
            (toke[0] == '>' && c == '=')) {
            toke[tokelen++] = c;
            inf->inf_bufptr++;
            c = inf->inf_buf[inf->inf_bufptr];
        }
    }

    toke[tokelen] = '\0';
}
/***************************************************************/
/*@*/struct biddesc * find_desc(SEQINFO *si, char * descname) {

    struct biddesc * bd;

    bd = si->headdesc;

    while (bd) {
       if (!strcmp(bd->bidname, descname)) return (bd);
       else bd = bd->nextdesc;
    }
    return (0);
}
/***************************************************************/
/*@*/struct sysdesc * find_sys(SEQINFO *si, char * sysname) {

    struct sysdesc * sd;

    sd = si->headsys;

    while (sd) {
       if (!strcmp(sd->sysname, sysname)) return (sd);
       else sd = sd->nextsys;
    }
    return (0);
}
/***************************************************************/
/*@*/int parse_relop(char * token) {

    int relop;

         if (!strcmp(token, "<"))  relop = LT;
    else if (!strcmp(token, "<=")) relop = LE;
    else if (!strcmp(token, "="))  relop = EQ;
    else if (!strcmp(token, ">"))  relop = GT;
    else if (!strcmp(token, ">=")) relop = GE;
    else if (!strcmp(token, "<>")) relop = NE;
    else if (!strcmp(token, "IN")) relop = IN;
    else                           relop = NA;

    return (relop);
}
/***************************************************************/
/*@*/void print_relop(struct omarglobals * og, int relop) {

    switch (relop) {
        case NA : omarout(og, "[]"); break;
        case LT : omarout(og, "<");  break;
        case LE : omarout(og, "<="); break;
        case EQ : omarout(og, "=");  break;
        case GT : omarout(og, ">");  break;
        case GE : omarout(og, ">="); break;
        case NE : omarout(og, "<>"); break;
        case IN : omarout(og, "IN"); break;
        default        : omarout(og, "?(%d)",relop);  break;
    }
}
/***************************************************************/
/*@*/int  parse_absolute_player(char * token) {

    int who;

         if (!strcmp(token, "WEST"))  who = WEST;
    else if (!strcmp(token, "NORTH")) who = NORTH;
    else if (!strcmp(token, "EAST"))  who = EAST;
    else if (!strcmp(token, "SOUTH")) who = SOUTH;
    else                              who = NULL_PLAYER;

    return (who);
}
/***************************************************************/
/*@*/int  parse_player(char * token) {

    int who;

         if (!strcmp(token, "MY"))    who = MY;
    else if (!strcmp(token, "PARD"))  who = PARD;
    else if (!strcmp(token, "RHO"))   who = RHO;
    else if (!strcmp(token, "LHO"))   who = LHO;
    else if (!strcmp(token, "UNBID")) who = UNBID;
    else                              who = NOONE;

    return (who);
}
/***************************************************************/
/*@*/void print_relative_player(struct omarglobals * og, int player) {

    switch (player) {
        case MY:    omarout(og, "MY");    break;
        case LHO:   omarout(og, "LHO");   break;
        case PARD:  omarout(og, "PARD");  break;
        case RHO:   omarout(og, "RHO");   break;
        case UNBID: omarout(og, "UNBID"); break;
        default:    omarout(og, "?(%d)", player); break;
    }
}
/***************************************************************/
/*@*/int parse_designator(char * token) {

    int suit;

         if (!strcmp(token, "SUIT1")) suit = SUIT1;
    else if (!strcmp(token, "SUIT2")) suit = SUIT2;
    else if (!strcmp(token, "SUIT3")) suit = SUIT3;
    else if (!strcmp(token, "SUIT4")) suit = SUIT4;
    else                              suit = SUIT0;

    return (suit);
}
/***************************************************************/
/*@*/void print_designator(struct omarglobals * og, int designator) {

    switch (designator) {
        case SUIT0: omarout(og, "SUIT0"); break;
        case SUIT1: omarout(og, "SUIT1"); break;
        case SUIT2: omarout(og, "SUIT2"); break;
        case SUIT3: omarout(og, "SUIT3"); break;
        case SUIT4: omarout(og, "SUIT4"); break;
        default:   omarout(og, "?(%d)", designator); break;
    }
}
/***************************************************************/
/*@*/int parse_suit(char * token) {

    int suit;

         if (!strcmp(token, "CLUBS"))    suit = CLUB_SUIT;
    else if (!strcmp(token, "DIAMONDS")) suit = DIAMOND_SUIT;
    else if (!strcmp(token, "HEARTS"))   suit = HEART_SUIT;
    else if (!strcmp(token, "SPADES"))   suit = SPADE_SUIT;
    else if (!strcmp(token, "NT"))       suit = NT_SUIT;
    else if (!strcmp(token, "SUIT"))     suit = SUIT_SUIT;
    else                                 suit = NO_SUIT;

    return (suit);
}
/***************************************************************/
/*@*/void print_suit(struct omarglobals * og, int suit) {

    switch (suit) {
        case NO_SUIT      : omarout(og, "NO_SUIT");  break;
        case CLUB_SUIT    : omarout(og, "CLUBS");    break;
        case DIAMOND_SUIT : omarout(og, "DIAMONDS"); break;
        case HEART_SUIT   : omarout(og, "HEARTS");   break;
        case SPADE_SUIT   : omarout(og, "SPADES");   break;
        case SUIT_SUIT    : omarout(og, "SUIT");     break;
        default:   omarout(og, "?(suit%d)", suit); break;
    }
}
/***************************************************************/
/*@*/void print_bid_suit(struct omarglobals * og, const char * fmt, int bidsuit) {

    switch (bidsuit) {
        case CLUBS    : omarout(og, fmt, "CLUBS");    break;
        case DIAMONDS : omarout(og, fmt, "DIAMONDS"); break;
        case HEARTS   : omarout(og, fmt, "HEARTS");   break;
        case SPADES   : omarout(og, fmt, "SPADES");   break;
        case NOTRUMP  : omarout(og, fmt, "NOTRUMP");     break;
        default:   omarout(og, "?(bidsuit=%d)", bidsuit); break;
    }
}
/***************************************************************/
/*@*/void parse_playersuit(struct omarglobals * og,
            struct infile * inf,
            char * token,
            int * who,
            int * suit) {

    *who = parse_player(token);
    if (*who == NOONE) {
        omarerr(og, "Expecting MY,PARD,RHO, or LHO.  Found '%s'\n", token);
        *who = MY;
    }

    next_token(inf, token);
    if (strcmp(token,".")) {
        omarerr(og, "Expecting '.'.  Found '%s'\n", token);
    }

    next_token(inf, token);
    *suit = parse_designator(token);
    if (*suit == SUIT0) {
        omarerr(og, "Expecting SUIT1, SUIT2, ...  Found '%s'\n", token);
    }
}
/***************************************************************/
/*@*/void print_playersuit(struct omarglobals * og, int player, int suit) {

    print_player(og, player);
    omarout(og, ".");
    print_designator(og, suit);
}
/***************************************************************/
/*@*/int parse_suitspec(struct omarglobals * og, struct infile * inf, char * token) {

    int suit;
    int suitspec;

    suit = parse_suit(token);
    if (suit == NO_SUIT) {
        int player;

        parse_playersuit(og, inf, token, &player, &suit);
        suitspec = (player << 3) | suit;
    } else {
        suitspec = suit;
    }

    return (suitspec);
}
/***************************************************************/
/*@*/void print_suitspec(struct omarglobals * og, int suitspec) {

    int player;
    int suit;

    player = (suitspec >> 3);
    if (player == NOONE) {
        print_suit(og, suitspec & 7);
    } else {
        suit = suitspec & 7;
        print_playersuit(og, player, suit);
    }
}
/***************************************************************/
/*@*/void print_suitspec_outf(FILE * outf, int suitspec) {

    int player;
    int suit;

    player = (suitspec >> 3);
    suit = suitspec & 7;
    if (player == NOONE) {
        switch (suit) {
            case NO_SUIT      : fprintf(outf, "NO_SUIT");  break;
            case CLUB_SUIT    : fprintf(outf, "CLUBS");    break;
            case DIAMOND_SUIT : fprintf(outf, "DIAMONDS"); break;
            case HEART_SUIT   : fprintf(outf, "HEARTS");   break;
            case SPADE_SUIT   : fprintf(outf, "SPADES");   break;
            case NT_SUIT      : fprintf(outf, "NT");       break;
            case SUIT_SUIT    : fprintf(outf, "SUIT");     break;
            default: 
                fprintf(outf, "?(%d)", suit);
                break;
        }
    } else {
        switch (player) {
            case MY    : fprintf(outf, "MY");  break;
            case LHO   : fprintf(outf, "LHO"); break;
            case PARD  : fprintf(outf, "PARD"); break;
            case RHO   : fprintf(outf, "RHO"); break;
            case UNBID : fprintf(outf, "UNBID"); break;
            default    :
                fprintf(outf, "?(%d",player);
                break;
        }
        fprintf(outf, ".");
        switch (suit) {
            case SUIT0: fprintf(outf, "SUIT0"); break;
            case SUIT1: fprintf(outf, "SUIT1"); break;
            case SUIT2: fprintf(outf, "SUIT2"); break;
            case SUIT3: fprintf(outf, "SUIT3"); break;
            case SUIT4: fprintf(outf, "SUIT4"); break;
            default:
                fprintf(outf, "?(%d)", suit);
                break;
        }
    }
}
/***************************************************************/
/*@*/void parse_suitexp(
            struct omarglobals * og,
            struct infile * inf,
            struct biddef * bd,
            char * token) {

    int relop;
    int suitspec;

    next_token(inf, token);
    if (!strcmp(token, "SUIT")) {
        next_token(inf, token);
        relop = parse_relop(token);
        if (relop == NA) {
            omarerr(og, "Expecting relational operator.  Found '%s'\n", token);
        }
        next_token(inf, token);
    } else relop = EQ;

    suitspec = parse_suitspec(og, inf, token);

    bd->bu.sexp = New(struct suitexp, 1);
    bd->bu.sexp->relop    = relop;
    bd->bu.sexp->suitspec = suitspec;

    next_token(inf, token);
}
/***************************************************************/
/*@*/void print_suitexp(struct omarglobals * og, struct suitexp * sexp) {

    omarout(og, "SUIT");
    print_relop(og, sexp->relop);

    print_suitspec(og, sexp->suitspec);
}
/***************************************************************/
/*@*/struct biddef *  parse_biddef(struct omarglobals * og,
        struct infile * inf,
        SEQINFO *si,
        const char * bidname,
        char * token) {

    struct biddef * bd;

    bd = New(struct biddef, 1);
    bd->bidtype = NO_BIDTYPE;

         if (!strcmp(token, "B")) bd->bidtype = BEGIN_BIDTYPE;
    else if (!strcmp(token, "P")) bd->bidtype = PASS_BIDTYPE;
    else if (!strcmp(token, "@")) bd->bidtype = ANY_BIDTYPE;
    else if (!strcmp(token, "X")) bd->bidtype = DBL_BIDTYPE;
    else if (!strcmp(token, "XX")) bd->bidtype = RDBL_BIDTYPE;
    else if (token[0] >= '1' && token[0] <= '7' && token[1] == '\0') {
        bd->bidtype = SEXP_BIDTYPE;
        bd->bidlevel = (int)(token[0] - '0');
        next_token(inf, token);
        if (strcmp(token, "[")) {
            omarerr(og, "Expecting ] in %s.  Found '%s'\n", bidname, token);
        }
        parse_suitexp(og, inf, bd, token);
        if (strcmp(token, "]")) {
            omarerr(og, "Expecting ] in %s.  Found '%s'\n", bidname, token);
        }
    } else if (token[0] >= '1' && token[0] <= '7') {
        bd->bidtype = LEVEL_BIDTYPE;
        bd->bidlevel = (int)(token[0] - '0');
             if (!strcmp(&(token[1]), "C")) bd->bu.stype = CLUBS_SUITTYPE;
        else if (!strcmp(&(token[1]), "D")) bd->bu.stype = DIAMONDS_SUITTYPE;
        else if (!strcmp(&(token[1]), "H")) bd->bu.stype = HEARTS_SUITTYPE;
        else if (!strcmp(&(token[1]), "S")) bd->bu.stype = SPADES_SUITTYPE;
        else if (!strcmp(&(token[1]), "N")) bd->bu.stype = NOTRUMP_SUITTYPE;
        else if (!strcmp(&(token[1]), "I")) bd->bu.stype = MINOR_SUITTYPE;
        else if (!strcmp(&(token[1]), "A")) bd->bu.stype = MAJOR_SUITTYPE;
        else if (!strcmp(&(token[1]), "T")) bd->bu.stype = TRUMP_SUITTYPE;
        else if (!strcmp(&(token[1]), "U")) bd->bu.stype = ANY_SUITTYPE;
        else {
            omarerr(og, "Bad suit type in in %s.  Found '%s'\n", bidname, token);
            next_token(inf, token);
            return (0);
        }
    } else if (!strcmp(token,"(")) {
        struct biddef * tail;

        next_token(inf, token);
        Free(bd);   /* 04/02/2018 */
        bd = parse_biddef(og, inf, si, bidname, token);
        tail = bd;

        while (!strcmp(token, "&")) {
           next_token(inf, token);
           tail->anddef = parse_biddef(og, inf, si, bidname, token);
           tail = tail->anddef;
        }

        if (strcmp(token,")")) {
            omarerr(og, "Expecting ) in %s.  Found '%s'.\n", bidname, token);
        }
    } else {
        bd->bidtype = DESC_BIDTYPE;
        bd->bu.desc = find_desc(si, token);
        if (!bd->bu.desc) {
            omarerr(og, "Bad suit description in %s.  Found '%s'\n", bidname, token);
            next_token(inf, token);
            return (0);
        }
    }

    next_token(inf, token);

    return (bd);
}
/***************************************************************/
/*@*/void print_biddef(struct omarglobals * og, struct biddef * bd) {

     char buf[256];
     char nbuf[10];

     buf[0] = '\0';

     switch (bd->bidtype) {
         case NO_BIDTYPE     : strcat(buf, "No");         break;
         case BEGIN_BIDTYPE  : strcat(buf, "B");          break;
         case PASS_BIDTYPE   : strcat(buf, "P");          break;
         case DBL_BIDTYPE    : strcat(buf, "X");          break;
         case RDBL_BIDTYPE   : strcat(buf, "XX");         break;
         case ANY_BIDTYPE    : strcat(buf, "@");          break;
         case LEVEL_BIDTYPE  :
             sprintf(nbuf, "%c", (char)(bd->bidlevel + '0'));
             strcat(buf, nbuf);
             switch (bd->bu.stype) {
                 case NO_SUITTYPE       : strcat(buf, "No");break;
                 case CLUBS_SUITTYPE    : strcat(buf, "C"); break;
                 case DIAMONDS_SUITTYPE : strcat(buf, "D"); break;
                 case HEARTS_SUITTYPE   : strcat(buf, "H"); break;
                 case SPADES_SUITTYPE   : strcat(buf, "S"); break;
                 case NOTRUMP_SUITTYPE  : strcat(buf, "N"); break;
                 case MINOR_SUITTYPE    : strcat(buf, "I"); break;
                 case MAJOR_SUITTYPE    : strcat(buf, "A"); break;
                 case TRUMP_SUITTYPE    : strcat(buf, "T"); break;
                 case ANY_SUITTYPE      : strcat(buf, "U"); break;
                 default                : strcat(buf, "?"); break;
             }
             break;
         case DESC_BIDTYPE   :
             strcat(buf, bd->bu.desc->bidname);
             break;
         case SEXP_BIDTYPE   :
             omarout(og, "%d[", bd->bidlevel);
             print_suitexp(og, bd->bu.sexp);
             omarout(og, "]");
             buf[0] = '\0'; /* So nothing else will print */
             break;
         default             : strcat(buf, "?");          break;
     }
     omarout(og, "%s", buf);
}
/***************************************************************/
/*@*/void print_deflist(struct omarglobals * og, struct biddef * bd) {

    if (!bd->anddef) print_biddef(og, bd);
    else {
        omarout(og, "(");
        while (bd) {
            print_biddef(og, bd);
            bd = bd->anddef;
            if (bd) omarout(og, "&");
        }
        omarout(og, ")");
    }
}
/***************************************************************/
/*@*/FLOATING get_constant(struct infile * inf, char * token) {

    FLOATING n;

    n = (FLOATING)atof(token);
    next_token(inf, token);

    return (n);
}
/***************************************************************/
/*@*/void app_prog(struct program * prog, char * ptr, int len) {

    if (prog->proglen + len > prog->progmax) {
        prog->progmax = prog->proglen + len + 8;
        prog->buffer  = Realloc(prog->buffer, char, prog->progmax);
    }

    memcpy(prog->buffer + prog->proglen, ptr, len);
    prog->proglen += len;
}
/***************************************************************/
/*@*/void append_prog(struct program * prog, int n) {

    char c;

    if (n < 0 || n > 255) {
        omarerr(NULL, "Internal error:  append_prog item too large.\n");
    }

    c = n & 255;

    app_prog(prog, &c, 1);
}
/***************************************************************/
/*@*/void append_prog_int(struct program * prog, int n) {

    app_prog(prog, (char*)(&n), sizeof(n));
}
/***************************************************************/
/*@*/void append_prog_float(struct program * prog, FLOATING n) {

    FLOATING f;

    f = n;  /* Amazing! */
    app_prog(prog, (char*)(&f), sizeof(f));
}
/***************************************************************/
/*@*/void update_prog_int(struct program * prog, int p, int n) {

    if (p < 0 || p + (int)sizeof(n) > prog->proglen) {
        omarerr(NULL, "Range error on update_prog_int\n");
    }

    memcpy(prog->buffer + p, &n, sizeof(n));
}
/***************************************************************/
/*@*/int fetch_prog(struct program * prog) {

    register unsigned char c;

    if (prog->progptr >= prog->proglen) {
        omarerr(NULL, "Attempt to fetch past end of program.\n");
        omarerr(NULL, "progptr=%d,proglen=%d\n", prog->progptr, prog->proglen);
    }

    c = *(prog->buffer + prog->progptr);
    prog->progptr++;

    return ((int)c);
}
/***************************************************************/
/*@*/int fetch_prog_int(struct program * prog) {

    int ii;


    if (prog->progptr + (int)sizeof(ii) > prog->proglen) {
        omarerr(NULL, "Attempt to fetch int past end of program.\n");
        omarerr(NULL, "progptr=%d,proglen=%d\n", prog->progptr, prog->proglen);
    }

    memcpy(&ii, prog->buffer + prog->progptr, sizeof(ii));
    prog->progptr += sizeof(ii);

    return (ii);
}
/***************************************************************/
/*@*/FLOATING fetch_prog_float(struct program * prog) {

    FLOATING f;


    if (prog->progptr + (int)sizeof(f) > prog->proglen) {
        omarerr(NULL, "Attempt to fetch FLOATING past end of program.\n");
        omarerr(NULL, "progptr=%d,proglen=%d\n", prog->progptr, prog->proglen);
    }

    memcpy(&f, prog->buffer + prog->progptr, sizeof(f));
    prog->progptr += sizeof(f);
    return (f);
}
/***************************************************************/
/*@*/int calc_unbid(int unbid, REQLIST * reqinfo) {

    int player;
    int suitix;
    int suit;
    int bidsuits[4] = {0, 0, 0, 0};

    for (player = 0; player < 4; player++) {
        for (suitix = 0; suitix < 4; suitix++) {
            suit = (*reqinfo)[player].headreqinfo->suit[suitix];
            if (suit != NO_SUIT) bidsuits[suit - CLUB_SUIT] = 1;
        }
    }

    suitix = 3;
    while (suitix >= 0 && unbid) {
        if (!bidsuits[suitix]) unbid--;
        if (unbid) suitix--;
    }

    if (unbid) return (NO_SUIT);
    else return (suitix + CLUB_SUIT);
}
/***************************************************************/
/*@*/void parse_requirement(struct omarglobals * og,
        struct infile * inf,
        struct program * prog,
        char * token) {

    FLOATING num;
    int   relop;

    next_token(inf, token);
    relop = parse_relop(token);
    next_token(inf, token);
    if (relop == NA) {
        omarerr(og, "Expecting relational operator.  Found '%s'\n", token);
    } else {
        append_prog(prog, relop);
        num = get_constant(inf, token);
        append_prog_float(prog, num);

        if (relop == IN) {
            if (strcmp(token, ",")) {
                omarerr(og, 
                "Expecting , to separate IN operands.  Found '%s'.\n",
                token);
            }
            next_token(inf, token);
            num = get_constant(inf, token);
            append_prog_float(prog, num);
        }
    }
}
/***************************************************************/
/*@*/void parse_req(struct omarglobals * og,
        struct infile * inf,
        struct program * prog,
        char *token) {

    int keywd;
    int suitspec;

         if (!strcmp(token, "PTS"))     keywd = PTS_KEYWD;
    else if (!strcmp(token, "HCP"))     keywd = HCP_KEYWD;
    else if (!strcmp(token, "DPTS"))    keywd = DPTS_KEYWD;
    else if (!strcmp(token, "TRICKS"))  keywd = TRICKS_KEYWD;
    else if (!strcmp(token, "QT"))      keywd = QT_KEYWD;
    else if (!strcmp(token, "SLEN"))    keywd = SLEN_KEYWD;
    else if (!strcmp(token, "SHCP"))    keywd = SHCP_KEYWD;
    else {
        omarerr(og, "Unrecognized requirement: '%s'\n", token);
        return;
    }

    switch (keywd) {
        case PTS_KEYWD:
            append_prog(prog, PTS_REQ);
            parse_requirement(og, inf, prog, token);
            break;
        case HCP_KEYWD:
            append_prog(prog, HCP_REQ);
            parse_requirement(og, inf, prog, token);
            break;
        case DPTS_KEYWD:
            append_prog(prog, DPTS_REQ);
            parse_requirement(og, inf, prog, token);
            break;
        case TRICKS_KEYWD:
            append_prog(prog, TRICKS_REQ);;
            parse_requirement(og, inf, prog, token);
            break;
        case QT_KEYWD:
            append_prog(prog, QT_REQ);
            parse_requirement(og, inf, prog, token);
            break;
        case SLEN_KEYWD:
            append_prog(prog, SLEN_REQ);
            next_token(inf, token);
            if (strcmp(token,"(")) {
                omarerr(og, "Expecting (.  Found '%s'\n", token);
            }
            next_token(inf, token);
            suitspec = parse_suitspec(og, inf, token);
            append_prog(prog, suitspec);
            next_token(inf, token);
            if (strcmp(token,")")) {
                omarerr(og, "Expecting ).  Found '%s'\n", token);
            }
            parse_requirement(og, inf, prog, token);
            break;
        case SHCP_KEYWD:
            append_prog(prog, SHCP_REQ);
            next_token(inf, token);
            if (strcmp(token,"(")) {
                omarerr(og, "Expecting (.  Found '%s'\n", token);
            }
            next_token(inf, token);
            suitspec = parse_suitspec(og, inf, token);
            append_prog(prog, suitspec);
            next_token(inf, token);
            if (strcmp(token,")")) {
                omarerr(og, "Expecting ).  Found '%s'\n", token);
            }
            parse_requirement(og, inf, prog, token);
            break;
        default:
            break;
    }
}
/***************************************************************/
/*@*/void print_req_prog(FILE * outf, struct program * prog) {

    int   op;
    int   relop;
    int   suitspec;
    FLOATING num;
    FLOATING num2;

    prog->progptr = 0;
    while (prog->progptr < prog->proglen) {
        op    = fetch_prog(prog);

        fprintf(outf, "REQ ");
        switch (op) {
            case PTS_REQ    : fprintf(outf, "PTS ");     break;
            case HCP_REQ    : fprintf(outf, "HCP ");     break;
            case DPTS_REQ   : fprintf(outf, "DPTS ");    break;
            case TRICKS_REQ : fprintf(outf, "TRICKS ");  break;
            case QT_REQ     : fprintf(outf, "QT ");      break;
            case SLEN_REQ:
                fprintf(outf, "SLEN(");
                suitspec = fetch_prog(prog);
                print_suitspec_outf(outf, suitspec);
                fprintf(outf, ") ");
                break;
            case SHCP_REQ:
                fprintf(outf, "SHCP(");
                suitspec = fetch_prog(prog);
                print_suitspec_outf(outf, suitspec);
                fprintf(outf, ") ");
                break;
            default        : fprintf(outf, "?(%d)",op);  break;
        }

        relop = fetch_prog(prog);
        /* print_relop(og, relop); */
        switch (relop) {
            case NA : fprintf(outf, "[]"); break;
            case LT : fprintf(outf, "<");  break;
            case LE : fprintf(outf, "<="); break;
            case EQ : fprintf(outf, "=");  break;
            case GT : fprintf(outf, ">");  break;
            case GE : fprintf(outf, ">="); break;
            case NE : fprintf(outf, "<>"); break;
            case IN : fprintf(outf, "IN"); break;
            default        : fprintf(outf, "?(%d)",relop);  break;
        }

        num   = fetch_prog_float(prog);
        if (relop == IN) {
            num2 = fetch_prog_float(prog);
            fprintf(outf, " %g,%g; ", (double)num, (double)num2);
        } else {
            fprintf(outf, " %g; ", (double)num);
        }
    }
}
/***************************************************************/
/*@*/int parse_force(char * token) {

    int force;

         if (!strcmp(token, "NONFORCING"))  force = NON_FORCING;
    else if (!strcmp(token, "FORCING"))     force = FORCING;
    else if (!strcmp(token, "GAMEFORCE"))   force = GAME_FORCE;
    else                                    force = NO_FORCE;

    return (force);
}
/***************************************************************/
/*@*/void print_force(struct omarglobals * og, int force) {

    switch (force) {
        case NO_FORCE       : omarout(og, "NO_FORCE"); break;
        case NON_FORCING    : omarout(og, "NONFORCING"); break;
        case FORCING        : omarout(og, "FORCING");    break;
        case GAME_FORCE     : omarout(og, "GAMEFORCE");  break;
        default:   omarout(og, "?(%d)", force); break;
    }
}
/***************************************************************/
/*@*/void parse_set(struct omarglobals * og,
        struct infile * inf,
        struct program * prog,
        char *token) {

    int suit;
    int suitspec;
    int force;

    suit = parse_designator(token);
    if (suit != SUIT0) {

        next_token(inf, token);
        if (strcmp(token, "=")) {
            omarerr(og, "Expecting =.  Found '%s'\n", token);
        }

        append_prog(prog, SUIT_SET);
        append_prog(prog, suit);
        next_token(inf, token);
        suitspec = parse_suitspec(og, inf, token);
        append_prog(prog, suitspec);
    } else {
        force = parse_force(token);
        if (force == NO_FORCE) {
            omarerr(og, "Expecting SUIT1, SUIT2, ...  Found '%s'\n", token);
        } else {
            append_prog(prog, FORCE_SET);
            append_prog(prog, force);
        }
    }
    next_token(inf, token);
}
/***************************************************************/
/*@*/void print_set(struct omarglobals * og, struct program *prog) {

    int settype;
    int suit;
    int suitspec;
    int force;

    settype = fetch_prog(prog);
    if (settype == SUIT_SET) {
        suit = fetch_prog(prog);
        print_designator(og, suit);
        omarout(og, " = ");
        suitspec = fetch_prog(prog);
        print_suitspec(og, suitspec);
    } else if (settype == FORCE_SET) {
        force = fetch_prog(prog);
        print_force(og, force);
    }
}
/***************************************************************/
/*@*/void print_set_prog(struct omarglobals * og, struct program * prog) {

    prog->progptr = 0;
    while (prog->progptr < prog->proglen) {
        omarout(og, "SET ");
        print_set(og, prog);
        omarout(og, ";");
    }
}
/***************************************************************/
/*@*/int is_num(char * buf) {

    int dots;
    int ii;

    dots = 0;
    for (ii = 0; buf[ii]; ii++) {
        if (buf[ii] < '0' || buf[ii] > '9') {
            if (buf[ii] == '.') {
                if (!dots) dots++;
                else return (0);
            } else return (0);
        }
    }

    return (1);
}
/***************************************************************/
/*@*/int is_func(char * token) {

    int func;

         if (!strcmp(token, "DPTS"))    func = DPTS_FUNC;
    else if (!strcmp(token, "HCP"))     func = HCP_FUNC;
    else if (!strcmp(token, "LONGEST")) func = LONGEST_FUNC;
    else if (!strcmp(token, "PTS"))     func = PTS_FUNC;
    else if (!strcmp(token, "QT"))      func = QT_FUNC;
    else if (!strcmp(token, "SHCP"))    func = SHCP_FUNC;
    else if (!strcmp(token, "SLEN"))    func = SLEN_FUNC;
    else if (!strcmp(token, "SNUM"))    func = SNUM_FUNC;
    else if (!strcmp(token, "SUPPORT")) func = SUPPORT_FUNC;
    else if (!strcmp(token, "TOP"))     func = TOP_FUNC;
    else if (!strcmp(token, "TRICKS"))  func = TRICKS_FUNC;
    else if (!strcmp(token, "MAKES"))   func = MAKES_FUNC;
    else                                func = NO_OP;

    return (func);
}
/***************************************************************/
/*@*/void parse_func(struct omarglobals * og,
                     struct infile * inf,
                     int func,
                     char * token,
                     struct program * prog,
                     OPSTACK * ops,
                        int expflags) {

    int nbparms;

    switch (func) {
        case HCP_FUNC     : nbparms = 0; break;
        case DPTS_FUNC    : nbparms = 0; break;
        case LONGEST_FUNC : nbparms = 1; break;
        case PTS_FUNC     : nbparms = 0; break;
        case QT_FUNC      : nbparms = 0; break;
        case SHCP_FUNC    : nbparms = 1; break;
        case SLEN_FUNC    : nbparms = 1; break;
        case SNUM_FUNC    : nbparms = 1; break;
        case SUPPORT_FUNC : nbparms = 1; break;
        case TOP_FUNC     : nbparms = 1; break;
        case TRICKS_FUNC  : nbparms = 0; break;
        case MAKES_FUNC   : nbparms = 1; break;
        default           : nbparms = 0; break;
    }

    next_token(inf, token);
    if (nbparms) {
        if (strcmp(token, "(")) {
            omarerr(og, "Expecting ( after function name.  Found '%s'\n",
                   token);
            ops->err = 1;
        } else {
            next_token(inf, token);
            parse_exp(og, inf, token, prog, ops, expflags);
            if (!ops->err) {
                if (func == LONGEST_FUNC) {
                    if (ops->oper[ops->tos-1].dtype != FLOAT_DTYPE) {
                        omarerr(og, "Expecting NUM type expression.\n");
                        ops->err = 1;
                    }
                } else {
                    if (ops->oper[ops->tos-1].dtype != SUIT_DTYPE) {
                        omarerr(og, "Expecting SUIT type expression.\n");
                        ops->err = 1;
                    }
                }
            }
        }

        if (!ops->err && func == TOP_FUNC) {
            if (strcmp(token, ",")) {
                omarerr(og, "Expecting , between parameters.  Found '%s'\n",
                        token);
                ops->err = 1;
            } else {
                next_token(inf, token);
                parse_exp(og, inf, token, prog, ops, expflags);
                if (!ops->err) {
                    if (ops->oper[ops->tos-1].dtype != FLOAT_DTYPE) {
                        omarerr(og, "Expecting NUM type expression.\n");
                        ops->err = 1;
                    }
                }
            }
        }

        if (!ops->err) {
            if (strcmp(token, ")")) {
                omarerr(og, "Expecting ) after parameters.  Found '%s'\n",
                       token);
                ops->err = 1;
            } else {
                next_token(inf, token);
            }
        }
    }

    if (!ops->err) {
        switch (func) {
            case DPTS_FUNC    :
                append_prog(prog, DPTS_FUNC);
                ops->oper[ops->tos++].dtype = FLOAT_DTYPE;
                break;
            case HCP_FUNC     :
                append_prog(prog, HCP_FUNC);
                ops->oper[ops->tos++].dtype = FLOAT_DTYPE;
                break;
            case LONGEST_FUNC :
                append_prog(prog, LONGEST_FUNC);
                ops->oper[ops->tos-1].dtype = SUIT_DTYPE;
                break;
            case PTS_FUNC     :
                append_prog(prog, PTS_FUNC);
                ops->oper[ops->tos++].dtype = FLOAT_DTYPE;
                break;
            case QT_FUNC      :
                append_prog(prog, QT_FUNC);
                ops->oper[ops->tos++].dtype = FLOAT_DTYPE;
                break;
            case SHCP_FUNC    :
                append_prog(prog, SHCP_FUNC);
                ops->oper[ops->tos-1].dtype = FLOAT_DTYPE;
                break;
            case SLEN_FUNC    :
                append_prog(prog, SLEN_FUNC);
                ops->oper[ops->tos-1].dtype = FLOAT_DTYPE;
                break;
            case SNUM_FUNC    :
                append_prog(prog, SNUM_FUNC);
                ops->oper[ops->tos-1].dtype = FLOAT_DTYPE;
                break;
            case SUPPORT_FUNC     :
                append_prog(prog, SUPPORT_FUNC);
                ops->oper[ops->tos-1].dtype = FLOAT_DTYPE;
                break;
            case TOP_FUNC     :
                append_prog(prog, TOP_FUNC);
                ops->tos--;
                ops->oper[ops->tos-1].dtype = FLOAT_DTYPE;
                break;
            case TRICKS_FUNC      :
                append_prog(prog, TRICKS_FUNC);
                ops->oper[ops->tos++].dtype = FLOAT_DTYPE;
                break;
            case MAKES_FUNC    :
                append_prog(prog, MAKES_FUNC);
                ops->oper[ops->tos-1].dtype = FLOAT_DTYPE;
                break;
            default           :
                break;
        }
    }
}
/***************************************************************/
/*@*/void parse_factor(struct omarglobals * og,
        struct infile * inf,
        char * token,
        struct program *prog,
        OPSTACK * ops,
        int expflags) {

    int func;

    if (!strcmp(token, "(")) {
        next_token(inf, token);
        parse_exp(og, inf, token, prog, ops, expflags);
        if (!ops->err && strcmp(token, ")")) {
            omarerr(og, "Expecting matching (.  Found '%s'\n", token);
            ops->err = 1;
            return;
        }
        next_token(inf, token);
    } else if (is_num(token)) {
        FLOATING num;

        append_prog(prog, LOADI_OP);
        ops->oper[ops->tos++].dtype = FLOAT_DTYPE;
        num = (FLOATING)atof(token);
        append_prog_float(prog, num);
        next_token(inf, token);
    } else if (!strcmp(token, "FALSE")) {
        append_prog(prog, LOADI_OP);
        ops->oper[ops->tos++].dtype = BOOL_DTYPE;
        append_prog_float(prog, (FLOATING)0);
        next_token(inf, token);
    } else if (!strcmp(token, "TRUE")) {
        append_prog(prog, LOADI_OP);
        ops->oper[ops->tos++].dtype = BOOL_DTYPE;
        append_prog_float(prog, (FLOATING)1);
        next_token(inf, token);
    } else if ((func = parse_suit(token)) != NO_SUIT) {
        FLOATING fsuit;

        append_prog(prog, LOADS_OP);
        ops->oper[ops->tos++].dtype = SUIT_DTYPE;
        fsuit = (FLOATING)func;
        append_prog_float(prog, fsuit);
        next_token(inf, token);
    } else if ((func = parse_player(token)) != NOONE) {
        next_token(inf, token);
        if (strcmp(token,".")) {
            omarerr(og, "Expecting '.'.  Found '%s'\n", token);
            ops->err = 1;
        } else {
            int designator;
            int suit;
            FLOATING fsuit;

            next_token(inf, token);
            designator = parse_designator(token);
            if (designator == SUIT0) {
                omarerr(og, "Expecting SUIT1, ...  Found '%s'\n", token);
                ops->err = 1;
            } else {
                suit = (func << 3) | designator;
                fsuit = (FLOATING)suit;
                ops->oper[ops->tos++].dtype = SUIT_DTYPE;
                append_prog(prog, LOADS_OP);
                append_prog_float(prog, fsuit);
                next_token(inf, token);
            }
        }
    } else if ((func = parse_absolute_player(token)) != NULL_PLAYER) {
        next_token(inf, token);
        if (strcmp(token,"[")) {
            omarerr(og, "Expecting '['.  Found '%s'\n", token);
            ops->err = 1;
        } else if (!(expflags & EXPFLAG_HANDREC) ||
                    (expflags & EXPFLAG_HANDINFO)) {
            omarerr(og, "Player '%s' not permitted.\n", token);
            ops->err = 1;
        } else {
            append_prog(prog, WEST_OP + func);
            next_token(inf, token);
            parse_exp(og, inf, token, prog, ops, expflags | EXPFLAG_HANDINFO);
            if (!ops-> err && (ops->oper[ops->tos-1].dtype != FLOAT_DTYPE)) {
                omarerr(og, "Numeric datatype required.\n");
                ops->err = 1;
                return;
            }
            if (strcmp(token,"]")) {
                omarerr(og, "Expecting ']'.  Found '%s'\n", token);
                ops->err = 1;
            } else {
                 next_token(inf, token);
            }
        }
    } else if ((func = is_func(token))) {
        if ((expflags & EXPFLAG_HANDREC) && !(expflags & EXPFLAG_HANDINFO)) {
            omarerr(og, "No hand defined.  Found '%s'\n", token);
            ops->err = 1;
        } else {
            parse_func(og, inf, func, token, prog, ops, expflags);
        }
    } else {
        omarerr(og, "Expecting num, function, or (exp).  Found '%s'\n", token);
        ops->err = 1;
    }
}
/***************************************************************/
/*@*/void parse_unary(
        struct omarglobals * og,
        struct infile * inf,
        char * token,
        struct program *prog,
        OPSTACK * ops,
        int expflags) {

    int m;

    if ((m=!strcmp(token, "-")) || !strcmp(token,"+")) {
        next_token(inf, token);
        parse_unary(og, inf, token, prog, ops, expflags);
        if (!ops->err) {
            if (ops->oper[ops->tos-1].dtype != FLOAT_DTYPE) {
                omarerr(og, "Datatype mismatch on unary +/-.\n");
                ops->err = 1;
            } else {
                if (m) append_prog(prog, NEG_OP);
            }
        }
    } else if (!strcmp(token, "NOT")) {
        next_token(inf, token);
        parse_unary(og, inf, token, prog, ops, expflags);
        if (!ops->err) {
            if (ops->oper[ops->tos-1].dtype != BOOL_DTYPE) {
                omarerr(og, "Datatype mismatch on NOT.\n");
                ops->err = 1;
            } else {
                append_prog(prog, NOT_OP);
            }
        }
    } else {
        parse_factor(og, inf, token, prog, ops, expflags);
    }
}
/***************************************************************/
/*@*/void parse_term(
        struct omarglobals * og,
        struct infile * inf,
        char * token,
        struct program *prog,
        OPSTACK * ops,
        int expflags) {

    int m;
    int d;

    parse_unary(og, inf, token, prog, ops, expflags);

    while (!ops->err &&
           ((m=!strcmp(token, "*")) ||
            (d=!strcmp(token, "/")) ||
            (!strcmp(token, "MOD")))) {
        next_token(inf, token);
        parse_unary(og, inf, token, prog, ops, expflags);
        if (!ops->err) {
            if (ops->oper[ops->tos-1].dtype != FLOAT_DTYPE ||
                ops->oper[ops->tos-2].dtype != FLOAT_DTYPE) {
                omarerr(og, "Datatype mismatch on multiply.\n");
                ops->err = 1;
            } else {
                if (m)      append_prog(prog, MUL_OP);
                else if (d) append_prog(prog, DIV_OP);
                else        append_prog(prog, MOD_OP);
                ops->tos--;
            }
        }
    }
}
/***************************************************************/
/*@*/void parse_sum(
        struct omarglobals * og,
        struct infile * inf,
        char * token,
        struct program *prog,
        OPSTACK * ops,
        int expflags) {

    int p;

    parse_term(og, inf, token, prog, ops, expflags);

    while (!ops->err && ((p=!strcmp(token, "+")) || !strcmp(token, "-"))) {
        next_token(inf, token);
        parse_term(og, inf, token, prog, ops, expflags);
        if (!ops->err) {
            if (ops->oper[ops->tos-1].dtype != FLOAT_DTYPE ||
                ops->oper[ops->tos-2].dtype != FLOAT_DTYPE) {
                omarerr(og, "Datatype mismatch on add.\n");
                ops->err = 1;
            } else {
                if (p) append_prog(prog, ADD_OP);
                else   append_prog(prog, SUB_OP);
                ops->tos--;
            }
        }
    }
}
/***************************************************************/
/*@*/void parse_compare(
        struct omarglobals * og,
        struct infile * inf,
        char * token,
        struct program *prog,
        OPSTACK * ops,
        int expflags) {

    int r;

    parse_sum(og, inf, token, prog, ops, expflags);

    while (!ops->err && ((r=parse_relop(token)) != NA)) {
        next_token(inf, token);
        parse_sum(og, inf, token, prog, ops, expflags);
        if (!ops->err) {
            if (ops->oper[ops->tos-1].dtype !=
                ops->oper[ops->tos-2].dtype) {
                omarerr(og, "Datatype mismatch on compare.\n");
                ops->err = 1;
            } else {
                switch (r) {
                    case LT : append_prog(prog, LT_OP); break;
                    case LE : append_prog(prog, LE_OP); break;
                    case EQ : append_prog(prog, EQ_OP); break;
                    case GE : append_prog(prog, GE_OP); break;
                    case GT : append_prog(prog, GT_OP); break;
                    case NE : append_prog(prog, NE_OP); break;
                    case IN :
                        if (strcmp(token, ",")) {
                            omarerr(og, 
                            "Expecting , to separate IN operands.  Found '%s'.\n",
                            token);
                        }
                        next_token(inf, token);
                        parse_sum(og, inf, token, prog, ops, expflags);
                        if (!ops->err) {
                            if (ops->oper[ops->tos-1].dtype !=
                                ops->oper[ops->tos-2].dtype) {
                                omarerr(og, "Datatype mismatch on compare.\n");
                                ops->err = 1;
                            }
                        }
                        append_prog(prog, IN_OP);
                        ops->tos--;
                        break;
                    default :
                        omarerr(og, "Compare error, r=%d\n", r);
                        break;
                }
                ops->tos--;
                ops->oper[ops->tos-1].dtype = BOOL_DTYPE;
            }
        }
    }
}
/***************************************************************/
/*@*/void parse_and(
        struct omarglobals * og,
        struct infile * inf,
        char * token,
        struct program *prog,
        OPSTACK * ops,
        int expflags) {

    int lab;

    parse_compare(og, inf, token, prog, ops, expflags);

    while (!ops->err && !strcmp(token, "AND")) {
        append_prog(prog, BF_OP);
        lab = prog->proglen;
        append_prog_int(prog, 0);
        next_token(inf, token);
        parse_compare(og, inf, token, prog, ops, expflags);
        if (!ops->err) {
            if (ops->oper[ops->tos-1].dtype != BOOL_DTYPE ||
                ops->oper[ops->tos-2].dtype != BOOL_DTYPE) {
                omarerr(og, "Datatype mismatch on and.\n");
                ops->err = 1;
            } else {
                ops->tos--;
            }
        }
        update_prog_int(prog, lab, prog->proglen);
    }
}
/***************************************************************/
/*@*/void parse_or(
        struct omarglobals * og,
        struct infile * inf,
        char * token,
        struct program *prog,
        OPSTACK * ops,
        int expflags) {

    int lab;

    parse_and(og, inf, token, prog, ops, expflags);

    while (!ops->err && !strcmp(token, "OR")) {
        append_prog(prog, BT_OP);
        lab = prog->proglen;
        append_prog_int(prog, 0);
        next_token(inf, token);
        parse_and(og, inf, token, prog, ops, expflags);
        if (!ops->err) {
            if (ops->oper[ops->tos-1].dtype != BOOL_DTYPE ||
                ops->oper[ops->tos-2].dtype != BOOL_DTYPE) {
                omarerr(og, "Datatype mismatch on and.\n");
                ops->err = 1;
            } else {
                ops->tos--;
            }
        }
        update_prog_int(prog, lab, prog->proglen);
    }
}
/***************************************************************/
/*@*/void parse_ifexp(
        struct omarglobals * og,
        struct infile * inf,
        char * token,
        struct program *prog,
        OPSTACK * ops,
        int expflags) {

    int lab1;
    int lab2;
    int dtyp;

    if (strcmp(token, "IF")) {
        parse_or(og, inf, token, prog, ops, expflags);
    } else {
        next_token(inf, token);
        parse_exp(og, inf, token, prog, ops, expflags);
        if (!ops->err) {
            if (ops->oper[ops->tos-1].dtype != BOOL_DTYPE) {
                omarerr(og, "Datatype mismatch on IF.\n");
                ops->err = 1;
            } else if (strcmp(token, "THEN")) {
                omarerr(og, "Expecting THEN after IF expression.\n");
                ops->err = 1;
            } else {
                append_prog(prog, BFDEL_OP);
                lab1 = prog->proglen;
                append_prog_int(prog, 0);
                ops->tos--;
                next_token(inf, token);
                parse_exp(og, inf, token, prog, ops, expflags);
            }
        }
        if (!ops->err) {
            if (strcmp(token, "ELSE")) {
                omarerr(og, "Expecting ELSE after IF expression.\n");
                ops->err = 1;
            } else {
                append_prog(prog, BR_OP);
                lab2 = prog->proglen;
                append_prog_int(prog, 0);
                update_prog_int(prog, lab1, prog->proglen);
                dtyp = ops->oper[ops->tos-1].dtype;
                ops->tos--;
                next_token(inf, token);
                parse_exp(og, inf, token, prog, ops, expflags);
            }
        }
        if (!ops->err) {
            if (ops->oper[ops->tos-1].dtype != dtyp) {
                omarerr(og, "Datatype conflict on IF.\n");
                ops->err = 1;
            }
            update_prog_int(prog, lab2, prog->proglen);
        }
    }
}
/***************************************************************/
/*@*/void parse_exp(
        struct omarglobals * og,
        struct infile * inf,
        char * token,
        struct program *prog,
        OPSTACK * ops,
        int expflags) {

    parse_ifexp(og, inf, token, prog, ops, expflags);
}
/***************************************************************/
/*@*/void parse_score(
        struct omarglobals * og,
        struct infile * inf,
        struct program * prog,
        char *token) {

    OPSTACK * ops;

    ops = New(OPSTACK, 1);
    parse_exp(og, inf, token, prog, ops, 0);
    append_prog(prog, END_OP);
    if (!ops-> err && (ops->oper[ops->tos-1].dtype != FLOAT_DTYPE)) {
        omarerr(og, "SCORE requires numeric datatype.\n");
        ops->err = 1;
        return;
    }

    Free(ops);
}
/***************************************************************/
/*@*/void parse_test(
        struct omarglobals * og,
        struct infile * inf,
        struct program * prog,
        char *token) {

    OPSTACK * ops;
    int lab = 0;

    ops = New(OPSTACK, 1);
    if (prog->proglen) {
        prog->proglen--; /* Delete END */
        append_prog(prog, BF_OP);
        lab = prog->proglen;
        append_prog_int(prog, 0);
    }

    parse_exp(og, inf, token, prog, ops, 0);
    if (!ops->err && (ops->oper[ops->tos-1].dtype != BOOL_DTYPE)) {
        omarerr(og, "TEST requires boolean datatype.\n");
        ops->err = 1;
        return;
    }

    if (lab) update_prog_int(prog, lab, prog->proglen);

    append_prog(prog, END_OP);

    Free(ops);
}
/***************************************************************/
/*@*/void print_score_prog(FILE * outf, struct program * prog) {

    int op;
    int ii;
    int suitspec;
    FLOATING num;

    prog->progptr = 0;
    while (prog->progptr < prog->proglen) {
        fprintf(outf, "%4d. ", prog->progptr);
        op = fetch_prog(prog);
        switch (op) {
            case ADD_OP      : fprintf(outf, "ADD   "); break;
            case BF_OP       :
                ii = fetch_prog_int(prog);
                fprintf(outf, "BF    %d", ii);
                break;
            case BFDEL_OP       :
                ii = fetch_prog_int(prog);
                fprintf(outf, "BFDEL %d", ii);
                break;
            case BR_OP       :
                ii = fetch_prog_int(prog);
                fprintf(outf, "BR    %d", ii);
                break;
            case BT_OP       :
                ii = fetch_prog_int(prog);
                fprintf(outf, "BT    %d", ii);
                break;
            case DIV_OP      : fprintf(outf, "DIV   "); break;
            case EQ_OP       : fprintf(outf, "EQ    "); break;
            case END_OP      : fprintf(outf, "END   "); break;
            case GE_OP       : fprintf(outf, "GE    "); break;
            case GT_OP       : fprintf(outf, "GT    "); break;
            case LE_OP       : fprintf(outf, "LE    "); break;
            case LOADI_OP    :
                num = fetch_prog_float(prog);
                fprintf(outf, "LOADI %g", num);
                break;
            case LOADS_OP    :
                num = fetch_prog_float(prog);
                fprintf(outf, "LOADS ");
                suitspec = (int)num;
                print_suitspec_outf(outf, suitspec);
                break;
            case LT_OP       : fprintf(outf, "LT    "); break;
            case MOD_OP      : fprintf(outf, "MOD   "); break;
            case MUL_OP      : fprintf(outf, "MUL   "); break;
            case NE_OP       : fprintf(outf, "NE    "); break;
            case IN_OP       : fprintf(outf, "IN    "); break;
            case NEG_OP      : fprintf(outf, "NEG   "); break;
            case NOT_OP      : fprintf(outf, "NOT   "); break;
            case SUB_OP      : fprintf(outf, "SUB   "); break;

            case WEST_OP      : fprintf(outf, "WEST  "); break;
            case NORTH_OP     : fprintf(outf, "NORTH "); break;
            case EAST_OP      : fprintf(outf, "EAST  "); break;
            case SOUTH_OP     : fprintf(outf, "SOUTH "); break;

            case DPTS_FUNC    : fprintf(outf, "DPTS ");    break;
            case HCP_FUNC     : fprintf(outf, "HCP ");     break;
            case LONGEST_FUNC : fprintf(outf, "LONGEST "); break;
            case PTS_FUNC     : fprintf(outf, "PTS ");     break;
            case QT_FUNC      : fprintf(outf, "QT ");      break;
            case SHCP_FUNC    : fprintf(outf, "SHCP ");    break;
            case SLEN_FUNC    : fprintf(outf, "SLEN ");    break;
            case SNUM_FUNC    : fprintf(outf, "SNUM ");    break;
            case SUPPORT_FUNC : fprintf(outf, "SUPPORT "); break;
            case TOP_FUNC     : fprintf(outf, "TOP ");     break;
            case TRICKS_FUNC  : fprintf(outf, "TRICKS ");  break;
            case MAKES_FUNC   : fprintf(outf, "MAKES ");   break;
            default          : fprintf(outf, "?(%d)", op); break;
        }
        fprintf(outf, "\n");
    }
}
/***************************************************************/
/*@*/struct bidlist * parse_bidlist(
            struct omarglobals * og,
            struct infile * inf,
            char *token) {

    struct bidlist * bl;
    int done;


    if (strcmp(token, "{")) {
        omarerr(og, "Expecting { to begin bid requirements.  Found '%s'.\n",
               token);
        return (0);
    }

    bl = New(struct bidlist, 1);

    next_token(inf, token);
    done = 0;
    while (token[0] && !done) {
        if (!strcmp(token, "}"))      done = 1;
        else if (!strcmp(token, ";")) {
            next_token(inf, token);
        } else {
            if (!strcmp(token, "REQ")) {
                next_token(inf, token);
                if (!bl->req_prog) {
                    bl->req_prog = New(struct program, 1);
                }
                parse_req(og, inf, bl->req_prog, token);
            } else if (!strcmp(token, "SET")) {
                next_token(inf, token);
                if (!bl->set_prog) {
                    bl->set_prog = New(struct program, 1);
                }
                parse_set(og, inf, bl->set_prog, token);
            } else if (!strcmp(token, "SCORE")) {
                next_token(inf, token);
                if (!bl->score_prog) {
                    bl->score_prog = New(struct program, 1);
                    parse_score(og, inf, bl->score_prog, token);
                } else {
                    omarerr(og, "Only one SCORE permitted in desc.\n");
                }
            } else if (!strcmp(token, "TEST")) {
                next_token(inf, token);
                if (!bl->test_prog) {
                    bl->test_prog = New(struct program, 1);
                }
                parse_test(og, inf, bl->test_prog, token);
            } else {
                omarerr(og, "Expecting REQ.  Found '%s'.\n", token);
                next_token(inf, token);
            }
            if (strcmp(token, ";")) {
                omarerr(og, "Expecting ;.  Found '%s'.\n", token);
            }
        }
    }

    next_token(inf, token);

    return (bl);
}
/***************************************************************/
/*@*/void parse_bidinfo(struct omarglobals * og,
        struct infile * inf,
        struct biddesc * bd,
        char *token) {

    int done;
    struct bidlist * bl;

    done = 0;
    next_token(inf, token);
    if (!strcmp(token, ";")) {
        done = 1;
    }

    while (!done) {
         bl = parse_bidlist(og, inf, token);
         if (!bd->blist) bd->blist = bl;
         if (bd->btail) bd->btail->nextlist = bl;
         bd->btail = bl;

         if (!strcmp(token, ";")) done = 1;
         else if (!strcmp(token, ",")) next_token(inf, token);
         else {
            omarerr(og, "Expecting , or ;.  Found '%s'\n", token);
            done = 1;
         }
    }
    next_token(inf, token);
}
/***************************************************************/
/*@*/struct bidseq * parse_bidseq(struct omarglobals * og,
        struct infile * inf,
        SEQINFO * si,
        const char * bidname,
        char *token) {

    struct bidseq * bs;
    struct biddef * bt;
    struct biddef * b;

    bs = New(struct bidseq, 1);
    bt = parse_biddef(og, inf, si, bidname, token);
    bs->bd = bt;
    bs->nbids = 1;

    while (bt && !strcmp(token, "-")) {
        next_token(inf, token);
        b = parse_biddef(og, inf, si, bidname, token);
        bt->next = b;
        bt       = b;
        bs->nbids++;
    }

    if (!bt) return (0);

    return (bs);

}
/***************************************************************/
/*@*/void print_bidseq(struct omarglobals * og, struct bidseq * bs) {

    struct biddef * b;

    b = bs->bd;
    if (!b) return;

    while (b) {
        if (b != bs->bd) omarout(og, " - ");
        print_deflist(og, b);
        b = b->next;
    }
}
/***************************************************************/
/*@*/struct biddesc * newdesc(SEQINFO * si, char * seqname) {

    struct biddesc * bd;

    bd = New(struct biddesc, 1);
    bd->bidname = Strdup(seqname);

    if (!si->headdesc) {
        si->headdesc = bd;
    } else {
        si->taildesc->nextdesc = bd;
    }
    si->taildesc = bd;

    return (bd);
}
/***************************************************************/
/*@*/struct biddesc *  store_seq(SEQINFO * si,
                                 char * seqname,
                                 int    which,
                                 int    nbseqs,
                                 struct bidseq ** bidseqs) {

    struct biddesc * sdesc;
    int ii;

    sdesc = newdesc(si, seqname);
    sdesc->bidseqs = bidseqs;
    sdesc->nbseqs  = nbseqs;

    for (ii = 0; ii < nbseqs; ii++) {
#ifdef OLD_WAY
        bs = New(struct bidseq, 1);
        memcpy(bs, bidseqs[ii], sizeof(struct bidseq));
        bs->seqdesc = sdesc;

        if (!si->headseq) {
            si->headseq = bs;
        } else {
            si->tailseq->nextseq = bs;
        }
        si->tailseq = bs;
#endif

/* Keep track of maximum sequence length */
        if (bidseqs[ii]->nbids > si->longestseq) {
            si->longestseq = bidseqs[ii]->nbids;
        }
    }

    return (sdesc);
}
/***************************************************************/
/*@*/void print_bidlist(struct omarglobals * og, struct bidlist * blist) {

    struct bidlist * bl;

    bl = blist;

    while (bl) {

        omarout(og, " {");

        if (bl->req_prog) {
            print_req_prog(get_outfile(og), bl->req_prog);
        }

        if (bl->set_prog) {
            print_set_prog(og, bl->set_prog);
        }

        if (bl->score_prog) {
            omarout(og, "SCORE\n");
            print_score_prog(get_outfile(og), bl->score_prog);
        }

        if (bl->test_prog) {
            omarout(og, "TEST\n");
            print_score_prog(get_outfile(og), bl->test_prog);
        }

        omarout(og, " }");
        bl = bl->nextlist;
        if (bl) omarout(og, ",");
    }
}
/***************************************************************/
/*@*/void print_biddesc(struct omarglobals * og, struct biddesc * bd) {

    struct bidseq * bs;
    int ii;

    omarout(og, "BID %s = ", bd->bidname);
    for (ii = 0; ii < bd->nbseqs; ii++) {
        bs = bd->bidseqs[ii];
        print_bidseq(og, bs);
        if (ii+1 < bd->nbseqs) omarout(og, " , ");
    }
    omarout(og, " :\n");

    print_bidlist(og, bd->blist);
    omarout(og, ";\n");
}
/***************************************************************/
/*@*/void print_desclist(struct omarglobals * og, struct desclist * dl) {

    while (dl) {
        omarout(og, "%s", dl->bdesc->bidname);
        dl = dl->nextlist;
        if (dl) omarout(og, ",");
    }
}
/***************************************************************/
/*@*/void print_seqinfo(struct omarglobals * og, SEQINFO * si) {

    struct biddesc *bd;

    bd = si->headdesc;

    while (bd) {
        print_biddesc(og, bd);
        omarout(og, "\n");
        bd = bd->nextdesc;
    }
    omarout(og, "\n");
}
/***************************************************************/
void free_program(struct program * prog) {

    if (!prog) return;

    Free(prog->buffer);
    Free(prog);
}
/***************************************************************/
/*@*/void free_bidlist(struct bidlist * blist) {

    struct bidlist * bl;
    struct bidlist * next_bl;

    bl = blist;

    while (bl) {
        next_bl = bl->nextlist;
        if (bl->req_prog) {
            free_program(bl->req_prog);
        }

        if (bl->set_prog) {
            free_program(bl->set_prog);
        }

        if (bl->score_prog) {
            free_program(bl->score_prog);
        }

        if (bl->test_prog) {
            free_program(bl->test_prog);
        }
        Free(bl);
        bl = next_bl;
    }
}
/***************************************************************/
/*@*/void free_suitexp(struct suitexp * sexp)
{
/*
struct suitexp {
    int relop;
    int suitspec;
};
*/
    Free(sexp);
}
/***************************************************************/
/*@*/void free_biddef(struct biddef * bd)
{
/*
union defu {
    int               stype;
    struct biddesc  * desc;
    struct suitexp  * sexp;
};
struct biddef {
    enum BIDTYPE      bidtype;
    int               bidlevel;
    union defu        bu;
    struct biddef   * anddef;
    struct biddef   * next;      // Next biddef in bidseq 
};
*/
    if (bd->bidtype == SEXP_BIDTYPE) {
        free_suitexp(bd->bu.sexp);
    }

    Free(bd);
}
/***************************************************************/
/*@*/void free_deflist(struct biddef * bd)
{
/*
struct biddef {
    enum BIDTYPE      bidtype;
    int               bidlevel;
    union defu        bu;
    struct biddef   * anddef;
    struct biddef   * next;      // Next biddef in bidseq 
};
*/

    struct biddef * next_bd;

    if (!bd->anddef) {
        free_biddef(bd);
    } else {
        while (bd) {
            next_bd = bd->anddef;
            free_biddef(bd);
            bd = next_bd;
        }
    }
}
/***************************************************************/
/*@*/void free_bidseq(struct bidseq * bs)
{
/*
struct bidseq {
    struct biddef * bd;
    int nbids;
    struct bidseq * nextseq;
};
print_bidseq
*/

    struct biddef * bd;
    struct biddef * next_bd;

    bd = bs->bd;

    while (bd) {
        next_bd = bd->next;
        free_deflist(bd);
        bd = next_bd;
    }
    Free(bs);
}
/***************************************************************/
/*@*/void free_biddesc(struct biddesc * bd) {

    struct bidseq * bs;
    int ii;

    Free(bd->bidname);
    for (ii = 0; ii < bd->nbseqs; ii++) {
        bs = bd->bidseqs[ii];
        free_bidseq(bs);
    }
    Free(bd->bidseqs);

    free_bidlist(bd->blist);
    Free(bd);
}
/***************************************************************/
void free_sysdesc(struct sysdesc * sys)
{
/*
struct sysdesc {
    char * sysname;
    long   nbids;
    struct biddesc ** bids;
    struct sysdesc *  nextsys;
};
*/

    struct sysdesc * sd;
    struct sysdesc * next_sd;

    sd = sys;

    while (sd) {
        next_sd = sd->nextsys;
        Free(sd->sysname);
        Free(sd->bids);
        Free(sd);
        sd = next_sd;
    }
}
/***************************************************************/
/*@*/SEQINFO * new_seqinfo(void) {

    SEQINFO * si;

    si = New(SEQINFO, 1);

    return (si);
}
/***************************************************************/
void free_seqinfo(SEQINFO * si)
{
/*
typedef struct seqinfo {
    int    longestseq;
    struct biddesc * headdesc;
    struct biddesc * taildesc;
    struct sysdesc * headsys;
    struct sysdesc * tailsys;
    struct convcard * cc;
} SEQINFO;
*/

    struct biddesc *bd;
    struct biddesc * next_bd;
    /* struct omarglobals * og = get_omarglobals(); */

    if (!si) return;

    bd = si->headdesc;

    while (bd) {
        next_bd = bd->nextdesc;
        free_biddesc(bd);
        bd = next_bd;
    }
    free_sysdesc(si->headsys);
    Free(si);
}
/***************************************************************/
void free_desclist(struct desclist * dlist)
{
/*
struct desclist {
    struct biddesc * bdesc;
    struct desclist * nextlist;
};
*/

    struct desclist * dl;
    struct desclist * next_dl;

if (!dlist) return;

    dl = dlist;

    while (dl) {
        next_dl = dl->nextlist;
        Free(dl);
        dl = next_dl;
    }
}
/***************************************************************/
void free_reqinfo(REQINFO * reqinfo)
{
/*
typedef struct {
    enum SUIT suit[4];  // SUIT1, SUIT2, ...
    enum SUIT agree;    // agreed upon trump suit
    enum FORCE forcing;
    REQUIREMENT requirement[NUM_REQUIREMENTS];
} REQINFO;
*/

    Free(reqinfo);
}
/***************************************************************/
/*@*/void free_reqinfolist(REQINFOLIST * ril)
{
/*
typedef struct {
    REQINFO * headreqinfo;
    REQINFO * tailreqinfo;
} REQINFOLIST;
*/

    free_reqinfo(ril->headreqinfo);
}
/***************************************************************/
void free_analysis(ANALYSIS * analysis)
{
/*
typedef struct {
    struct desclist * desc[MAX_BIDS];
    REQLIST ril;
} ANALYSIS;
*/

    int ii;

    for (ii = 0; ii < MAX_BIDS; ii++) {
        free_desclist(analysis->desc[ii]);
    }

    for (ii = 0; ii < 4; ii++) {
        free_reqinfolist(&(analysis->ani_ril[ii]));
    }

    Free(analysis);
}
/***************************************************************/
/*@*/void new_analysis_reqinfo_lists(ANALYSIS * analysis) {

    int player;
    int ii;
    REQINFO * ri;

    for (player = 0; player < 4; player++) {
        ri = New(REQINFO, 1);
        analysis->ani_ril[player].headreqinfo = ri;
        analysis->ani_ril[player].tailreqinfo = ri;
        for (ii = 0; ii < NUM_REQUIREMENTS; ii++) {
            ri->requirement[ii].lovalue = 0;
            ri->requirement[ii].hivalue = 99;
        }
    }
}
/***************************************************************/
void reset_analysis(ANALYSIS * analysis)
{
/*
typedef struct {
    struct desclist * desc[MAX_BIDS];
    REQLIST ril;
} ANALYSIS;
*/

    int ii;

    for (ii = 0; ii < 4; ii++) {
        free_reqinfolist(&(analysis->ani_ril[ii]));
    }

    new_analysis_reqinfo_lists(analysis);

}
/***************************************************************/
void free_auction(AUCTION * auction)
{
/*
typedef struct auction {
    int au_dealer;
    int au_vulnerability;
    int au_nbids;
    int au_bid[MAX_BIDS];
} AUCTION;
*/

    Free(auction);
}
/***************************************************************/
/*@*/void print_system(struct omarglobals * og, struct sysdesc * sd) {

    long ii;
    int col;
    int len;

    omarout(og, "SYS %s = ", sd->sysname);
    col = 999;

    for (ii = 0; ii < sd->nbids; ii++) {
        len = IStrlen(sd->bids[ii]->bidname);
        if (col + len > 64) { omarout(og, "\n    "); col = 5; }
        if (ii + 1 < sd->nbids) omarout(og, "%s, ", sd->bids[ii]->bidname);
        else                    omarout(og, "%s;" , sd->bids[ii]->bidname);
        col += len + 2;
    }
    omarout(og, "\n");
}
/***************************************************************/
/*@*/void print_systems(struct omarglobals * og, SEQINFO * si) {

    struct sysdesc * sd;

    sd = si->headsys;
    while (sd) {
        print_system(og, sd);
        sd = sd->nextsys;
    }
}
/***************************************************************/
/*@*/int parse_biddesc(struct omarglobals * og,
                        struct infile * inf,
                        SEQINFO *si,
                        char * token) {

    int  ostat = 0;
    struct bidseq * bs;
    struct bidseq ** bidseqs;
    struct biddesc * bd;
    struct sysdesc * sd;
    int    nseq;
    char   bidname[TOKEN_MAX];
    int    done;

    bs = 0;
    nseq = 0;
    next_token(inf, bidname);

    next_token(inf, token);
    if (strcmp(token, "=")) {
        ostat = omarerr(og, "Expecting = after bid name.  Found '%s'.\n", token);
        next_token(inf, token);
    } else {
        if (!strlen(bidname) || !isalpha(bidname[0])) {
            ostat = omarerr(og, "Invalid bid name %s.\n", bidname);
        }
        bd = find_desc(si, bidname);
        if (bd) {
            ostat = omarerr(og, "Duplicate bid description: %s\n", bidname);
        }
        sd = find_sys(si, bidname);
        if (sd) {
            ostat = omarerr(og, "Bid description %s has same name as system\n", bidname);
        }
        done = 0;
        next_token(inf, token);
        while (!done) {
            bs = parse_bidseq(og, inf, si, bidname, token);

            if (bs) {
                if (!nseq) bidseqs = New(struct bidseq *, 1);
                else bidseqs = Realloc(bidseqs, struct bidseq *, nseq + 1);
                *(bidseqs + nseq) = bs;
                nseq++;
            }

            if (!strcmp(token, ",")) {
                next_token(inf, token);
            } else if (!strcmp(token, ":")) {
                bd = store_seq(si, bidname, 3, nseq, bidseqs);
                parse_bidinfo(og, inf, bd, token);
                done = 1;
            } else {
                ostat = omarerr(og, "Expecting colon, or comma in %s.  Found '%s'\n",
                    bidname, token);
                next_token(inf, token);
            }
        }
    }

    return (ostat);
}
/***************************************************************/
/*@*/void add_bid_to_sys(struct sysdesc * sys,
                         struct biddesc * bd) {

    int found;
    long ii;

    found = 0;
    for (ii = 0; !found && ii < sys->nbids; ii++) {
       if (!strcmp(sys->bids[ii]->bidname, bd->bidname)) found = 1;
    }

    if (!found) {
       if (!sys->nbids) sys->bids = New(struct biddesc*, 1);
       else sys->bids = Realloc(sys->bids, struct biddesc*, sys->nbids + 1);

       sys->bids[sys->nbids++] = bd;
    }
}
/***************************************************************/
/*@*/void add_sys_to_sys(struct sysdesc * sys,
                         struct sysdesc * sd) {

    long ii;

    for (ii = 0; ii < sd->nbids; ii++) {
        add_bid_to_sys(sys, sd->bids[ii]);
    }
}
/***************************************************************/
/*@*/void parse_sysdesc(struct omarglobals * og,
                        struct infile * inf,
                        SEQINFO *si,
                        char * token) {

    char   sysname[256];
    struct sysdesc * sys;
    struct sysdesc * sd;
    struct biddesc * bd;
    int    done;

    next_token(inf, sysname);

    next_token(inf, token);
    if (strcmp(token, "=")) {
        omarerr(og, "Expecting = after sys name.  Found '%s'.\n", token);
        next_token(inf, token);
    } else {
        if (!strlen(sysname) || !isalpha(sysname[0])) {
            omarerr(og, "Invalid system name %s.\n", sysname);
        }
        sd = find_sys(si, sysname);
        if (sd) {
            omarerr(og, "Duplicate system description: %s\n", sysname);
        }
        bd = find_desc(si, sysname);
        if (bd) {
            omarerr(og, "System description %s has same name as bid.\n", sysname);
        }

        sys = New(struct sysdesc, 1);
        sys->sysname = Strdup(sysname);
        sys->nbids   = 0;
        sys->bids    = 0;
        sys->nextsys = 0;

        next_token(inf, token);
        if (!strcmp(token, ";")) done = 1;
        else                     done = 0;
        while (!done) {
            if (!strlen(token) || !isalpha(token[0])) {
                omarerr(og, "Invalid system reference %s.\n", sysname);
            }
            sd = find_sys(si, token);
            if (sd) add_sys_to_sys(sys, sd);
            else {
                bd = find_desc(si, token);
                if (!bd) {
                    omarerr(og, "System reference %s not found.\n", token);
                } else {
                    add_bid_to_sys(sys, bd);
                }
            }
            next_token(inf, token);
            if (!strcmp(token, ";")) done = 1;
            else if (!strcmp(token, ",")) next_token(inf, token);
            else {
                omarerr(og, "Expecting comma or semicolon.  Found %s\n",
                        token);
            }
        }

        /**** Insert sys into SEQINFO * si ****/
        if (!si->headsys) {
            si->headsys = sys;
        } else {
            si->tailsys->nextsys = sys;
        }
        si->tailsys = sys;
    }
    next_token(inf, token);
}
/***************************************************************/
/*@*/int parse_bids(struct omarglobals * og, struct frec * biddef_fr, SEQINFO * si) {

    int  ostat = 0;
    int  done;
    struct infile * inf;
    char token[TOKEN_MAX];

    inf = infile_new(biddef_fr);

    next_token(inf, token);
    done = (!token[0]);
    while (!done) {
        if (!token[0] || !strcmp(token,"END")) {
            done = 1;
        } else if (!strcmp(token, "BID")) {
            ostat = parse_biddesc(og, inf, si, token);
        } else if (!strcmp(token, "SYS")) {
            parse_sysdesc(og, inf, si, token);
        } else {
            ostat = omarerr(og, "Expecting BID, or END.  Found '%s'.\n", token);
            next_token(inf, token);
        }
    }
    infile_free(inf, 0);

    return (ostat);
}
/***************************************************************/
/*@*/void print_player(struct omarglobals * og, int player) {

    switch (player) {
        case WEST  : omarout(og, "WEST");  break;
        case NORTH : omarout(og, "NORTH"); break;
        case EAST  : omarout(og, "EAST");  break;
        case SOUTH : omarout(og, "SOUTH"); break;
        default    : omarout(og, "?(player=%d",player); break;
    }
}
/***************************************************************/
/*@*/void print_display_player(struct omarglobals * og, int player) {

    omarout(og, "%c", display_player[player]);
}
/***************************************************************/
/*@*/void get_vulnerability(char * vulstr, int vulnerability) {

    switch (vulnerability) {
        case VUL_NONE   : strcpy(vulstr, "NONE")   ; break;
        case VUL_NS     : strcpy(vulstr, "N-S")    ; break;
        case VUL_EW     : strcpy(vulstr, "E-W")    ; break;
        case VUL_BOTH   : strcpy(vulstr, "BOTH")   ; break;
        default    : sprintf(vulstr, "?(%d)", vulnerability&255); break;
    }
}
/***************************************************************/
/*@*/void print_vulnerability(struct omarglobals * og, int vulnerability) {

    char vulstr[8];

    get_vulnerability(vulstr, vulnerability);

    omarout(og, "%s", vulstr);
}
/***************************************************************/
/*@*/void print_req_name(struct omarglobals * og,
                            const char * fmt,
                            int req) {

    switch (req) {
        case REQ_PTS           : omarout(og, fmt, "PTS");            break;
        case REQ_HCP           : omarout(og, fmt, "HCP");            break;
        case REQ_DPTS          : omarout(og, fmt, "DPTS");           break;
        case REQ_TRICKS        : omarout(og, fmt, "TRICKS");         break;
        case REQ_QT            : omarout(og, fmt, "QT");             break;
        case REQ_SLEN_CLUBS    : omarout(og, fmt, "SLEN(CLUBS)");    break;
        case REQ_SLEN_DIAMONDS : omarout(og, fmt, "SLEN(DIAMONDS)"); break;
        case REQ_SLEN_HEARTS   : omarout(og, fmt, "SLEN(HEARTS)");   break;
        case REQ_SLEN_SPADES   : omarout(og, fmt, "SLEN(SPADES)");   break;
        case REQ_SHCP_CLUBS    : omarout(og, fmt, "SHCP(CLUBS)");    break;
        case REQ_SHCP_DIAMONDS : omarout(og, fmt, "SHCP(DIAMONDS)"); break;
        case REQ_SHCP_HEARTS   : omarout(og, fmt, "SHCP(HEARTS)");   break;
        case REQ_SHCP_SPADES   : omarout(og, fmt, "SHCP(SPADES)");   break;
        default                :                           break;
    }
}
/***************************************************************/
/*@*/void print_requirement(struct omarglobals * og,
                            const char * fmt,
                            int req,
                            REQUIREMENT * requirement) {


    if (requirement->lovalue != 0 ||
        requirement->hivalue != 99) {
        print_req_name(og, fmt, req);

        omarout(og, " [%g..%g]; ", requirement->lovalue, requirement->hivalue);
        omarout(og, "\n");
        omarout(og, "    ");
    }
}
/***************************************************************/
/*@*/void print_reqinfo(struct omarglobals * og, REQINFO * reqinfo) {

    int ii;

    for (ii = 0; ii < 4; ii++) {
        if (reqinfo->suit[ii]) {
            print_designator(og, SUIT1 + ii);
            omarout(og, " = ");
            print_suit(og, reqinfo->suit[ii]);
            omarout(og, "\n");
            omarout(og, "    ");
         }
    }

    if (reqinfo->forcing != NO_FORCE) {
        print_force(og, reqinfo->forcing);
        omarout(og, ";\n");
        omarout(og, "    ");
    }

    for (ii = 0; ii < NUM_REQUIREMENTS; ii++) {
        print_requirement(og, "%s", ii,  &(reqinfo->requirement[ii]));
    }
}
/***************************************************************/
/*@*/void print_reqinfolist(struct omarglobals * og, REQINFOLIST * ril) {

    print_reqinfo(og, ril->headreqinfo);
}
/***************************************************************/
char * ebid_to_chars(int ebid) {

    char * bidstr;
    int bidlen;
    int bidmax;
    char ch;
    int bt;
    int bl;
    int bs;

    bidstr = NULL;
    bidlen = 0;
    bidmax = 0;

    decode_bid(ebid, &bt, &bl, &bs);

    if (bt == PASS)          append_chars(&bidstr, &bidlen, &bidmax, "P", 1);
    else if (bt == DOUBLE)   append_chars(&bidstr, &bidlen, &bidmax, "X", 1);
    else if (bt == REDOUBLE) append_chars(&bidstr, &bidlen, &bidmax, "XX", 2);
    else if (bt == NOBID)    append_chars(&bidstr, &bidlen, &bidmax, "0", 1);
    else if (bt == BID) {
        ch = bl + '0';
        append_chars(&bidstr, &bidlen, &bidmax, &ch, 1);
        ch = display_suit[bs];
        append_chars(&bidstr, &bidlen, &bidmax, &ch, 1);
    } else append_chars(&bidstr, &bidlen, &bidmax, "?", 1);

    return (bidstr);
}
/***************************************************************/
/*
/void print_ebid(struct omarglobals * og, int ebid) {

    int bt;
    int bl;
    int bs;

    decode_bid(ebid, &bt, &bl, &bs);
    if (bt == PASS) omarout(og, "P ");
    else if (bt == DOUBLE) omarout(og, "X ");
    else if (bt == REDOUBLE) omarout(og, "XX");
    else if (bt == NOBID)    omarout(og, "0");
    else if (bt == BID) omarout(og, "%d%c", bl, display_suit[bs]);
    else omarout(og, "?(%d)", ebid);
}
*/
/***************************************************************/
/*@*/void print_ebid(struct omarglobals * og, int ebid) {

    char * bidstr;

    bidstr = ebid_to_chars(ebid);

    omarout(og, "%-2s", bidstr);
    Free(bidstr);
}
/***************************************************************/
char * auction_to_chars(AUCTION * auction)
{

    char * austr;
    int bidder;
    int bidno;
    int bt;
    int bl;
    int bs;
    char ch;
    int aulen;
    int aumax;

    austr = NULL;
    aulen = 0;
    aumax = 0;

    for (bidder = 0; bidder < 4; bidder++) {
        append_chars(&austr, &aulen, &aumax, " ", 1);
        ch = display_player[bidder];
        append_chars(&austr, &aulen, &aumax, &ch, 1);
        append_chars(&austr, &aulen, &aumax, " ", 1);
    }
    append_chars(&austr, &aulen, &aumax, "\n", 1);

    for (bidder = 0; bidder < auction->au_dealer; bidder++) {
        append_chars(&austr, &aulen, &aumax, "   ", 3);
    }

    bidder = auction->au_dealer;
    for (bidno = 0; bidno < auction->au_nbids; bidno++) {
        append_chars(&austr, &aulen, &aumax, " ", 1);
        decode_bid(auction->au_bid[bidno], &bt, &bl, &bs);
        if (bt == PASS) append_chars(&austr, &aulen, &aumax, "P ", 2);
        else if (bt == DOUBLE) append_chars(&austr, &aulen, &aumax, "X ", 2);
        else if (bt == REDOUBLE) append_chars(&austr, &aulen, &aumax, "XX", 2);
        else if (bt == BID) {
            ch = '0' + bl;
            append_chars(&austr, &aulen, &aumax, &ch, 1);
            ch = display_suit[bs];
            append_chars(&austr, &aulen, &aumax, &ch, 1);
        } else append_chars(&austr, &aulen, &aumax, "??", 2);
        bidder++;
        if (bidder >= 4) {
            append_chars(&austr, &aulen, &aumax, "\n", 1);
            bidder = 0;
        }
    }

    return (austr);
}
/***************************************************************/
/*@*/void print_auction(struct omarglobals * og, AUCTION * auction) {

    char * austr;

    austr = auction_to_chars(auction);

    omarout(og, "%s\n", austr);
    Free(austr);
}
/***************************************************************/
/* original br.c                                               */
/***************************************************************/

/***************************************************************/
/*
void print_hand(struct omarglobals * og, HAND * hand) {

    int suit;
    int cardix;

    for (suit = 3; suit >= 0; suit--) {
        omarout(og, "%c ", display_suit[suit]);
        for (cardix = 0; cardix < hand->length[suit]; cardix++) {
            omarout(og, "%c", display_rank[hand->cards[suit][cardix]]);
        }
        omarout(og, "\n");
    }
}
*/
/***************************************************************/
char * hand_to_chars(HAND * hand) {

    int suit;
    int cardix;
    char * handstr;
    int handlen;
    int handmax;
    char ch;

    handstr = NULL;
    handlen = 0;
    handmax = 0;

    for (suit = 3; suit >= 0; suit--) {
        ch = display_suit[suit];
        append_chars(&handstr, &handlen, &handmax, &ch, 1);
        append_chars(&handstr, &handlen, &handmax, " ", 1);

        for (cardix = 0; cardix < hand->length[suit]; cardix++) {
            ch = display_rank[hand->cards[suit][cardix]];
            append_chars(&handstr, &handlen, &handmax, &ch, 1);
        }
        append_chars(&handstr, &handlen, &handmax, "\n", 1);
    }

    return (handstr);
}
/***************************************************************/
/*@*/void print_hand(struct omarglobals * og, HAND * hand) {

    char * handstr;

    handstr = hand_to_chars(hand);

    omarout(og, "%s\n", handstr);
    Free(handstr);
}
/***************************************************************/
/*@*/void make_handinfo(HAND * hand, HANDINFO * handinfo) {

    int suit;
    int cardix;
    int shcp[4];
    int hcp;
    FLOATING dpts;
    FLOATING adjust;
    int pts;
    int req;
    FLOATING qt;
    int losers;
    int top;

    memset(handinfo, 0, sizeof(HANDINFO));
    handinfo->hand = hand;

    hcp = 0;
    dpts = 0;
    adjust = 0;
    for (suit = 0; suit < 4; suit++) {
        shcp[suit] = 0;
        for (cardix = 0;
            cardix < hand->length[suit] && hand->cards[suit][cardix] >= 11;
            cardix++) {
            hcp += (hand->cards[suit][cardix] - 10);
            shcp[suit] += (hand->cards[suit][cardix] - 10);

            /* Adjust for unprotected honors */
            if (hand->length[suit] < 3) {
                if (hand->cards[suit][cardix] < 15 - hand->length[suit]) {
                    adjust--;
                }
            }
        }

        if (hand->length[suit] < 3) {
            dpts += 3 - hand->length[suit];
        }
    }

    /* Calculate number of losers */
    losers = 0;
    for (suit = 0; suit < 4; suit++) {
        top = 15 - hand->length[suit];
        if (top < 12) top = 12;
        for (cardix = 0; cardix < hand->length[suit] && cardix < 3; cardix++) {
            if (hand->cards[suit][cardix] < top) losers++;
        }
    }

    /* Calculate number of quick tricks */
    qt = 0;
    for (suit = 0; suit < 4; suit++) {
        if (hand->length[suit] && hand->cards[suit][0] == 14) {
            qt += 1.0;  /* A */
            if (hand->length[suit] > 1) {
                if (hand->cards[suit][1] == 13) qt += 1.0;      /* AK */
                else if (hand->cards[suit][1] == 12) qt += 0.5; /* AQ */
            }
        } else if (hand->length[suit] > 1 && hand->cards[suit][0] == 13) {
            qt += 0.5;  /* K */
            if (hand->cards[suit][1] == 12) qt += 0.5;          /* KQ */
        }
    }

    for (req = 0; req < NUM_REQUIREMENTS; req++) {
        switch (req) {
            case REQ_PTS            :
                pts = hcp + (int)floor(dpts + adjust + 0.5);
                handinfo->value[req] = (FLOATING)pts;
                break;
            case REQ_HCP            :
                handinfo->value[req] = (FLOATING)hcp;
                break;
            case REQ_DPTS           :
                handinfo->value[req] = dpts;
                break;
            case REQ_TRICKS         :
                handinfo->value[req] = (FLOATING)(13 - losers);
                break;
            case REQ_QT         :
                handinfo->value[req] = qt;
                break;
            case REQ_SLEN_CLUBS     :
                handinfo->value[req] = (FLOATING)(hand->length[0]);
                break;
            case REQ_SLEN_DIAMONDS  :
                handinfo->value[req] =(FLOATING)( hand->length[1]);
                break;
            case REQ_SLEN_HEARTS    :
                handinfo->value[req] = (FLOATING)(hand->length[2]);
                break;
            case REQ_SLEN_SPADES    :
                handinfo->value[req] = (FLOATING)(hand->length[3]);
                break;
            case REQ_SHCP_CLUBS     :
                handinfo->value[req] = (FLOATING)shcp[0];
                break;
            case REQ_SHCP_DIAMONDS  :
                handinfo->value[req] = (FLOATING)shcp[1];
                break;
            case REQ_SHCP_HEARTS    :
                handinfo->value[req] = (FLOATING)shcp[2];
                break;
            case REQ_SHCP_SPADES    :
                handinfo->value[req] = (FLOATING)shcp[3];
                break;
            default                 :
                break;
        }
    }
}
/***************************************************************/
/*@*/void make_bid(AUCTION * auction, int bt, int bl, int bs) {

    auction->au_bid[auction->au_nbids++] = encode_bid(bt, bl, bs);
}
/***************************************************************/
/*@*/void make_ebid(AUCTION * auction, int ebid) {

    int bt;
    int bl;
    int bs;

    decode_bid(ebid, &bt, &bl, &bs);
    auction->au_bid[auction->au_nbids++] = encode_bid(bt, bl, bs);
}
/***************************************************************/
/*@*/int interp_suitspec(int suitspec, int bidder, REQLIST * reqinfo) {

    int player;
    int suit;

    player = (suitspec >> 3);
    suit = suitspec & 7;

    if (player) {
        if (player == UNBID) {
            suit = calc_unbid(suit, reqinfo);
        } else {
            switch (player) {
                case MY   : player = bidder;         ; break;
                case LHO  : player = (bidder + 1) & 3; break;
                case PARD : player = (bidder + 2) & 3; break;
                case RHO  : player = (bidder + 3) & 3; break;
                default   : return (NO_SUIT)         ;
            }
            suit = (*reqinfo)[player].headreqinfo->suit[suit - CLUB_SUIT];
        }
    }

    return (suit);
}
/***************************************************************/
/*@*/int desc_matches(struct biddesc * desc,
                      struct desclist * list) {

    int matches;
    struct desclist * dl;

    dl = list;
    matches = 0;
    while (dl && !matches) {
       if (!strcmp(desc->bidname, dl->bdesc->bidname)) matches = 1;
       else dl = dl->nextlist;
    }

    return (matches);
}
/***************************************************************/
/*@*/int def_matches(struct biddef * bd,
                     AUCTION *  auction,
                     int        bidix,
                     ANALYSIS * analysis) {

    int matches;
    int bt;
    int bl;
    int bs;

    matches = 0;

    if (bidix < 0) {
        if (bd->bidtype == BEGIN_BIDTYPE) return (1);
        else                              return (0);
    }

    decode_bid(auction->au_bid[bidix], &bt, &bl, &bs);

    switch (bd->bidtype) {
        case NO_BIDTYPE     :
            break;
        case BEGIN_BIDTYPE  :
            break;
        case PASS_BIDTYPE   :
            if (bt == PASS) matches = 1;
            break;
        case DBL_BIDTYPE    :
            if (bt == DOUBLE) matches = 1;
            break;
        case RDBL_BIDTYPE   :
            if (bt == REDOUBLE) matches = 1;
            break;
        case ANY_BIDTYPE    :
            matches = 1;
            break;
        case LEVEL_BIDTYPE  :
            if (bd->bidlevel == bl) switch (bd->bu.stype) {
                case NO_SUITTYPE       :
                    break;
                case CLUBS_SUITTYPE    :
                    if (bs == CLUBS)    matches = 1;
                    break;
                case DIAMONDS_SUITTYPE :
                    if (bs == DIAMONDS) matches = 1;
                    break;
                case HEARTS_SUITTYPE   :
                    if (bs == HEARTS)   matches = 1;
                    break;
                case SPADES_SUITTYPE   :
                    if (bs == SPADES)   matches = 1;
                    break;
                case NOTRUMP_SUITTYPE  :
                    if (bs == NOTRUMP)  matches = 1;
                    break;
                case MINOR_SUITTYPE    :
                    if (bs == CLUBS || bs == DIAMONDS) matches = 1;
                    break;
                case MAJOR_SUITTYPE    :
                    if (bs == HEARTS || bs == SPADES)  matches = 1;
                    break;
                case TRUMP_SUITTYPE    :
                    if (bs != NOTRUMP)                 matches = 1;
                    break;
                case ANY_SUITTYPE      :
                    matches = 1;
                    break;
                default                :
                    break;
            }
            break;
        case DESC_BIDTYPE   :
            if (desc_matches(bd->bu.desc, analysis->desc[bidix])) {
                matches = 1;
            }
            break;
        case SEXP_BIDTYPE   :
            if (bd->bidlevel == bl && bs != NOTRUMP) {
                int suit;

                suit = interp_suitspec(bd->bu.sexp->suitspec,
                       (auction->au_dealer + bidix) & 3,
                       &(analysis->ani_ril));

                if (suit != NO_SUIT) {
                    bs += CLUB_SUIT;
                    switch (bd->bu.sexp->relop) {
                        case LT: if (bs <  suit) matches = 1; break;
                        case LE: if (bs <= suit) matches = 1; break;
                        case EQ: if (bs == suit) matches = 1; break;
                        case GE: if (bs >= suit) matches = 1; break;
                        case GT: if (bs >  suit) matches = 1; break;
                        case NE: if (bs != suit) matches = 1; break;
                        default:                              break;
                    }
                }
            }
        default             :
            break;
    }
    return (matches);
}
/***************************************************************/
/*@*/int deflist_matches(struct biddef * bd,
                         AUCTION *  auction,
                         int        bidix,
                         ANALYSIS * analysis) {

    int matches;

    matches = 1;
    while (bd && matches) {
        matches = def_matches(bd, auction, bidix, analysis);
        bd = bd->anddef;
    }

    return (matches);
}
/***************************************************************/
/*@*/int seq_matches(struct bidseq * seq,
                     AUCTION *  auction,
                     int        bidix,
                     ANALYSIS * analysis) {

    int ii;
    int matches;
    struct biddef * bd;

    matches = 1;
    bd = seq->bd;
    for (ii = 0; matches && ii < seq->nbids; ii++) {
        if (!deflist_matches(bd, auction, bidix + ii, analysis)) {
            matches = 0;
        } else bd = bd->next;
    }

    return (matches);
}
/***************************************************************/
/*@*/void apply_req(struct program * prog,
                    REQUIREMENT * req) {

    int relop;
    FLOATING num;

    relop = fetch_prog(prog);
    num   = fetch_prog_float(prog);
    if (relop == EQ || relop == GT || relop == GE || relop == IN) {
        if (num > req->lovalue) {
            req->lovalue = num;
        }
    }
    if (relop == IN) num = fetch_prog_float(prog);
    if (relop == EQ || relop == LT || relop == LE || relop == IN) {
        if (num < req->hivalue) {
            req->hivalue = num;
        }
    }
}
/***************************************************************/
/*@*/void skip_req(struct program * prog,
                    REQUIREMENT * req) {

    int relop;
    FLOATING num;

    relop = fetch_prog(prog);
    num   = fetch_prog_float(prog);
    if (relop == IN) num = fetch_prog_float(prog);
}
/***************************************************************/
/*@*/void apply_req_prog(struct program * prog,
                         int bid,
                         int bidder,
                         REQLIST * reqinfo) {
    int   op;
    int   suit;
    int   suitspec;
    REQINFO * ri;

    if (!prog) return;

    prog->progptr = 0;
    ri = (*reqinfo)[bidder].headreqinfo;
    while (prog->progptr < prog->proglen) {
        op    = fetch_prog(prog);

        switch (op) {
            case PTS_REQ :
                apply_req(prog, &(ri->requirement[REQ_PTS]));
                break;
            case HCP_REQ :
                apply_req(prog, &(ri->requirement[REQ_HCP]));
                break;
            case DPTS_REQ :
                apply_req(prog, &(ri->requirement[REQ_DPTS]));
                break;
            case TRICKS_REQ :
                apply_req(prog, &(ri->requirement[REQ_TRICKS]));
                break;
            case QT_REQ :
                apply_req(prog, &(ri->requirement[REQ_QT]));
                break;
            case SLEN_REQ:
                suitspec = fetch_prog(prog);
                suit = interp_suitspec(suitspec, bidder, reqinfo);
                if (suit == SUIT_SUIT) {
                    int bt;
                    int bl;

                    decode_bid(bid, &bt, &bl, &suit);
                    apply_req(prog, &(ri->requirement[REQ_SLEN_CLUBS+suit]));
                } else if (suit != NO_SUIT) {
                    apply_req(prog,
                        &(ri->requirement[REQ_SLEN_CLUBS-CLUB_SUIT+suit]));
                } else {
                    skip_req(prog,
                        &(ri->requirement[REQ_SLEN_CLUBS-CLUB_SUIT+suit]));
                }
                break;
            case SHCP_REQ:
                suitspec = fetch_prog(prog);
                suit = interp_suitspec(suitspec, bidder, reqinfo);
                if (suit == SUIT_SUIT) {
                    int bt;
                    int bl;

                    decode_bid(bid, &bt, &bl, &suit);
                    apply_req(prog, &(ri->requirement[REQ_SHCP_CLUBS+suit]));
                } else if (suit != NO_SUIT) {
                    apply_req(prog,
                        &(ri->requirement[REQ_SHCP_CLUBS-CLUB_SUIT+suit]));
                } else {
                    skip_req(prog,
                        &(ri->requirement[REQ_SLEN_CLUBS-CLUB_SUIT+suit]));
                }
                break;
            default        :   break;
        }
    }
}
/***************************************************************/
/*@*/void apply_requirements(struct desclist * desc,
                             int bid,
                             int bidder,
                             REQLIST * reqinfo) {

    struct desclist * dl;
    struct bidlist * bl;

    dl = desc;
    while (dl) {
        bl = dl->bdesc->blist;
        while (bl) {
            apply_req_prog(dl->bdesc->blist->req_prog,
                           bid, bidder, reqinfo);
            bl = bl->nextlist;
        }
        dl = dl->nextlist;
    }
}
/***************************************************************/
/*@*/void apply_set(struct program *prog,
                    int bid,
                    int bidder,
                    REQLIST * reqinfo) {

    int settype;

    settype = fetch_prog(prog);
    if (settype == SUIT_SET) {
        int designator;
        int suit;
        int suitspec;

        designator = fetch_prog(prog) - SUIT1;
        suitspec = fetch_prog(prog);
        suit = interp_suitspec(suitspec, bidder, reqinfo);
        if (suit == SUIT_SUIT) {
            int bt;
            int bl;

            decode_bid(bid, &bt, &bl, &suit);
            (*reqinfo)[bidder].headreqinfo->suit[designator] =
                suit + CLUB_SUIT;
        } else if (suit != NO_SUIT) {
            (*reqinfo)[bidder].headreqinfo->suit[designator] =
                suit + CLUB_SUIT;
        }
    } else {
        int force;

        force = fetch_prog(prog);
        (*reqinfo)[bidder].headreqinfo->forcing = force;
    }
}
/***************************************************************/
/*@*/void apply_set_prog(struct program * prog,
                         int bid,
                         int bidder,
                         REQLIST * reqinfo) {

    if (!prog) return;

    prog->progptr = 0;
    while (prog->progptr < prog->proglen) {
        apply_set(prog, bid, bidder, reqinfo);
    }
}
/***************************************************************/
/*@*/void apply_sets(struct desclist * desc,
                     int bid,
                     int bidder,
                     REQLIST * reqinfo) {

    struct desclist * dl;
    struct bidlist * bl;

    /* Reset onetime bid info */
    if ((*reqinfo)[bidder].headreqinfo->forcing == FORCING) {
        (*reqinfo)[bidder].headreqinfo->forcing = NO_FORCE;
    }

    dl = desc;
    while (dl) {
        bl = dl->bdesc->blist;
        while (bl) {
            apply_set_prog(dl->bdesc->blist->set_prog,
                           bid, bidder, reqinfo);
            bl = bl->nextlist;
        }
        dl = dl->nextlist;
    }
}
/***************************************************************/
/*@*/void analyze_bid(
                    struct omarglobals * og,
                    SEQINFO *  si,
                    AUCTION *  auction,
                    int        bidno,
                    ANALYSIS * analysis,
                    int anat) {

    int lastbidno;
    int bn;
    int len;
    int six;
    int bix;
    struct desclist * desc;
    struct desclist * d;
    struct bidseq  * seq;
    struct biddesc * bd;
    struct sysdesc * cc;

/* Set convention card for bid */
    cc = si->cc->ccs[(auction->au_dealer + bidno) & 1];

    lastbidno = MAX(bidno - si->longestseq + 1, -1);
    bn = bidno;
    desc = NULL;

    while (bn >= lastbidno) {
        len = bidno - bn + 1;

/* Search for bid sequences of length len */
        for (bix = 0; bix < cc->nbids; bix++) {
            bd = cc->bids[bix];
            for (six = 0; six < bd->nbseqs; six++) {
                seq = bd->bidseqs[six];
                if (seq->nbids == len) {
                    if (seq_matches(seq, auction, bn, analysis)) {
                        d = New(struct desclist, 1);
                        d->bdesc = bd;
                        d->nextlist = desc;
                        desc = d;
                    }
                }
            }
        }
        bn--;
    }

    /* 04/02/2018: Added free_desclist() */
    if (analysis->desc[bidno]) {
        free_desclist(analysis->desc[bidno]);
    }
    analysis->desc[bidno] = desc;

    if (desc) {
        apply_requirements(desc, auction->au_bid[bidno],
                           (auction->au_dealer + bidno) & 3,
                           &(analysis->ani_ril));

        apply_sets(desc, auction->au_bid[bidno],
                   (auction->au_dealer + bidno) & 3,
                   &(analysis->ani_ril));
    }

    if (!desc && anat) {
	    int typ;
	    int lev;
	    int sut;
	
        decode_bid(auction->au_bid[bidno], &typ, &lev, &sut);

        if (typ != PASS || anat > 1) {
		    analyze_natural_bid(og, auction, bidno, analysis);
	    }
    }
}
/***************************************************************/
/*@*/void analyze_auction(
                    struct omarglobals * og,
                    SEQINFO *  si,
                    AUCTION *  auction,
                    ANALYSIS * analysis,
                    int anat) {

    int ii;

    reset_analysis(analysis);

    for (ii = 0; ii < auction->au_nbids; ii++) {
        analyze_bid(og, si, auction, ii, analysis, anat);
    }
}
/***************************************************************/
/*@*/void print_player_fmt(struct omarglobals * og, const char * fmt, int player) {

    switch (player) {
        case WEST  : omarout(og, fmt, "West "); break;
        case NORTH : omarout(og, fmt, "North"); break;
        case EAST  : omarout(og, fmt, "East "); break;
        case SOUTH : omarout(og, fmt, "South"); break;
        default    : omarout(og, fmt, "?(player=%d",player); break;
    }
}
/***************************************************************/
/*@*/void print_analysis(struct omarglobals * og,
        AUCTION * auction,
        ANALYSIS * analysis) {

    int ii;

    for (ii = 0; ii < auction->au_nbids; ii++) {
        print_player_fmt(og, "%-5s", (ii + auction->au_dealer) & 3);
        omarout(og, " ");
        print_bid(og, auction->au_bid[ii]);
        omarout(og, "(");
        if (analysis->desc[ii]) print_desclist(og, analysis->desc[ii]);
        else                    omarout(og, "Natural");
        omarout(og, ")\n");
    }
    omarout(og, "\n");

    for (ii = 0; ii < 4; ii++) {
        print_player_fmt(og, "%-5s", ii);
        omarout(og, "\n");
        omarout(og, "    ");
        print_reqinfolist(og, &(analysis->ani_ril[ii]));
        omarout(og, "\n");
    }
}
/***************************************************************/
/*@*/FLOATING score_desclist(
                        struct omarglobals * og,
                        struct desclist * dl,
                        AUCTION *  auction,
                        ANALYSIS * analysis,
                        HANDINFO * handinfo) {

    struct bidlist * bl;
    FLOATING bestscore;
    FLOATING score;

    bestscore = 0;
    while (dl) {
        bl = dl->bdesc->blist;
        while (bl) {
            if (bl->score_prog) {
                score = run_prog(og, bl->score_prog, auction, analysis, handinfo, NULL);
                if (score > bestscore) bestscore = score;
            }
            bl = bl->nextlist;
        }
        dl = dl->nextlist;
    }

    return (bestscore);
}
/***************************************************************/
/*@*/int test_desclist(
                        struct omarglobals * og,
                        struct desclist * dl,
                        AUCTION *  auction,
                        ANALYSIS * analysis,
                        HANDINFO * handinfo) {

    struct bidlist * bl;
    int ok;

    ok = 1;
    while (dl && ok) {
        bl = dl->bdesc->blist;
        while (bl && ok) {
            if (bl->test_prog) {
                ok = (int)run_prog(og, bl->test_prog, auction, analysis, handinfo, NULL);
            }
            bl = bl->nextlist;
        }
        dl = dl->nextlist;
    }

    return (ok);
}
/***************************************************************/
/*@*/void next_avail_bid(AUCTION * auction, int *bt, int *bl, int *bs, int *dbl)
 {

    int bidno;
    int typ;
    int lev;
    int sut;
    int done;
    int declarer;
    int bidder;

    bidno = auction->au_nbids - 1;
    done = 0;
    *dbl = 0;

    while (!done && bidno >= 0) {
        decode_bid(auction->au_bid[bidno], &typ, &lev, &sut);
        if (typ == PASS) bidno--;
        else if (typ == DOUBLE) {
            *dbl = 1;
            bidno--;
        } else if (typ == REDOUBLE) {
            *dbl = 2;
            bidno--;
        } else {
            if (sut == NOTRUMP) {
                if (lev == 7) *bt = NONE;
                else {
                    *bt = BID;
                    *bl = lev + 1;
                    *bs = CLUBS;
                }
            } else {
                *bt = BID;
                *bl = lev;
                *bs = sut + 1;
            }
            declarer = (auction->au_dealer + bidno) & 3;
            bidder = (auction->au_dealer + auction->au_nbids) & 3;
            if ((declarer & 1) == (bidder & 1)) {
                if (*dbl == 1) *dbl = REDOUBLE;
                else           *dbl = NONE;
            } else {
                if (*dbl == 0) *dbl = DOUBLE;
                else           *dbl = NONE;
            }
            done = 1;
        }
    }

    if (bidno < 0) {
        *bt = BID;
        *bl = 1;
        *bs = CLUBS;
    }
}
/***************************************************************/
/*@*/int check_requirements(HANDINFO * handinfo, REQINFOLIST * ril) {

    int req;
    int matches;

    matches = 1;
    req = 0;
    while (matches && req < NUM_REQUIREMENTS) {
        if (handinfo->value[req]<ril->headreqinfo->requirement[req].lovalue ||
            handinfo->value[req]>ril->headreqinfo->requirement[req].hivalue) {
            matches = 0;
        } else {
            req++;
        }
    }

    return (matches);
}
/***************************************************************/
/*@*/void try_bid(struct omarglobals * og,
                  BIDSCORE * bestbid,
                  int btyp,
                  int blev,
                  int bsut,
                  SEQINFO *  si,
                  AUCTION *  auction,
                  ANALYSIS * analysis,
                  HANDINFO * handinfo) {

    REQINFO save_reqinfo;
    int bidno;
    FLOATING bidscore;
    int test;
    int bidder;

    bidder = (auction->au_dealer + auction->au_nbids) & 3;
    memcpy(&save_reqinfo,
           analysis->ani_ril[bidder].headreqinfo, sizeof(REQINFO));

    bidno = auction->au_nbids;
    make_bid(auction, btyp, blev, bsut);
    analyze_bid(og, si, auction, bidno, analysis, 0);
    if (analysis->desc[bidno]) {

        test = check_requirements(handinfo, &(analysis->ani_ril[bidder]));

        if (test) {
            test = test_desclist(og, analysis->desc[bidno],
                        auction, analysis, handinfo);
        }

        if (test) {
            bidscore = score_desclist(og, analysis->desc[bidno],
                            auction, analysis, handinfo);

            if (handinfo->trace) {
                print_ebid(og, auction->au_bid[bidno]);
                omarout(og, " = %g\n", bidscore);
            }

            if (bidscore > bestbid->score) {
                bestbid->score = bidscore;
                bestbid->bidtype = btyp;
                bestbid->bidlevel = blev;
                bestbid->bidsuit = bsut;
            }
        }
    }

    auction->au_nbids = bidno;  /* Back out last bid */

    memcpy(analysis->ani_ril[bidder].headreqinfo,
           &save_reqinfo, sizeof(REQINFO));
}
/***************************************************************/
/*@*/int find_best_bid(struct omarglobals * og,
                       SEQINFO *  si,
                       AUCTION *  auction,
                       ANALYSIS * analysis,
                       HANDINFO * handinfo) {

    int typ;
    int level;
    int suit;
    int dbl;
    BIDSCORE bestbid;
    int bb;

    bestbid.score = 0;
    bestbid.bidtype = NONE;

    try_bid(og, &bestbid, PASS, 0, 0, si, auction, analysis, handinfo);

    next_avail_bid(auction, &typ, &level, &suit, &dbl);
    if (dbl != NONE) {
        try_bid(og, &bestbid, dbl, 0, 0, si, auction, analysis, handinfo);
    }

    while (typ == BID) {

        try_bid(og, &bestbid, typ, level, suit, si, auction, analysis, handinfo);

        if (suit == NOTRUMP) {
            level++;
            if (level > 7) typ = NONE;
            else suit = CLUBS;
        } else suit++;
    }

    if (bestbid.score > 0) {
        bb = encode_bid(bestbid.bidtype, bestbid.bidlevel, bestbid.bidsuit);
    } else {
        bb = encode_bid(NOBID, 0, 0);
    }

    return (bb);
}
/***************************************************************/
/*@*/int next_bid(
        struct omarglobals * og,
        const char * bidbuf,
        int *ptr,
        int *bt,
        int *bl,
        int *bs) {

    char c;
    int is_bid;

    *bt = NONE;
    is_bid = 1;

    while (isspace(bidbuf[*ptr]) ||
           (bidbuf[*ptr] == '-') ||
           (bidbuf[*ptr] == ',')) (*ptr)++;

    c = bidbuf[*ptr];
    (*ptr)++;
    if (!c || c == ';') return (0);

    if (c >= '1' && c <= '7') {
        *bt = BID;
        *bl = c - '0';
        c = bidbuf[*ptr];
        (*ptr)++;
             if (c == 'c' || c == 'C') *bs = CLUBS;
        else if (c == 'd' || c == 'D') *bs = DIAMONDS;
        else if (c == 'h' || c == 'H') *bs = HEARTS;
        else if (c == 's' || c == 'S') *bs = SPADES;
        else if (c == 'n' || c == 'N') *bs = NOTRUMP;
        else {
           omarerr(og, "Unrecognized suit '%c'\n", c);
           is_bid = 0;
        }
    } else if (c == 'x' || c == 'X') {
        c = bidbuf[*ptr];
        if (c == 'x' || c == 'X') {
            *bt = REDOUBLE;
            (*ptr)++;
        } else *bt = DOUBLE;
    } else if (c == 'p' || c == 'P') *bt = PASS;
    else {
        omarerr(og, "Unrecognized bid '%c'\n", c);
        is_bid = 0;
    }

    while (isspace(bidbuf[*ptr]) ||
           (bidbuf[*ptr] == '-') ||
           (bidbuf[*ptr] == ',')) (*ptr)++;

    return (is_bid);
}
/***************************************************************/
/*@*/int next_card(
        struct omarglobals * og,
        const char * bidbuf,
        int *ptr,
        int *card,
        int *suit,
        int * current_suit) {

    char c;
    int is_suit;

    while (isspace(bidbuf[*ptr]) ||
           (bidbuf[*ptr] == ',')) (*ptr)++;

    c = toupper(bidbuf[*ptr]);
    (*ptr)++;
    if (!c || c == ';') return (0);

    is_suit = 1;
    while (is_suit) {
        switch (c) {
            case 'C' : *current_suit = CLUBS;    is_suit = 1; break;
            case 'D' : *current_suit = DIAMONDS; is_suit = 1; break;
            case 'H' : *current_suit = HEARTS;   is_suit = 1; break;
            case 'S' : *current_suit = SPADES;   is_suit = 1; break;
            default  :                           is_suit = 0; break;
        }
        if (is_suit) {
            while (isspace(bidbuf[*ptr]) ||
                   (bidbuf[*ptr] == ',')) (*ptr)++;

            c = toupper(bidbuf[*ptr]);
            (*ptr)++;
            if (!c || c == ';') return (0);
         }
    }

    switch (c) {
        case 'A' : *card = 14; break;
        case 'K' : *card = 13; break;
        case 'Q' : *card = 12; break;
        case 'J' : *card = 11; break;
        case 'T' : *card = 10; break;
        default  :
            if (c >= '2' && c <= '9') *card = c - '0';
            else {
                omarerr(og, "Unrecognized card '%c'\n", c);
                return (-1);
            }
            break;
    }

    *suit = *current_suit;

    return (1);
}
/***************************************************************/
/*@*/int next_pair(const char * bidbuf, int *ptr, int *pair) {
/*
** 03/03/2018
*/

    char c;
    int is_pair;

    while (isspace(bidbuf[*ptr]) ||
           (bidbuf[*ptr] == ',')) (*ptr)++;

    c = toupper(bidbuf[*ptr]);
    (*ptr)++;
    if (!c || c == ';') return (0);

    is_pair = 0;
    if (c == 'N' && toupper(bidbuf[*ptr] == 'S')) {
        *pair = NORTH_SOUTH;
        is_pair = 1;
        (*ptr)++;
    } else if (c == 'E' && toupper(bidbuf[*ptr] == 'W')) {
        *pair = EAST_WEST;
        is_pair = 1;
        (*ptr)++;
    }

    return (is_pair);
}
/***************************************************************/
/*@*/int next_player(const char * bidbuf, int *ptr, int *player) {
/*
** 03/03/2018
*/

    char c;
    int is_player;

    while (isspace(bidbuf[*ptr]) ||
           (bidbuf[*ptr] == ',')) (*ptr)++;

    c = toupper(bidbuf[*ptr]);
    (*ptr)++;
    if (!c || c == ';') return (0);

    switch (c) {
        case 'N' : *player = NORTH; is_player = 1; break;
        case 'E' : *player = EAST;  is_player = 1; break;
        case 'S' : *player = SOUTH; is_player = 1; break;
        case 'W' : *player = WEST;  is_player = 1; break;
        default  :                  is_player = 0; break;
    }

    return (is_player);
}
/***************************************************************/
/*@*/void handle_desc(struct omarglobals * og,
        SEQINFO * si,
        char * bidbuf) {

    struct biddesc * bd;
    char desc[256];
    int  dlen;
    int ii;

    ii = 1;
    for (ii = 1;isspace(bidbuf[ii]);ii++) ;

    dlen = 0;
    while ((bidbuf[ii] >= 'a' && bidbuf[ii] <= 'z') ||
           (bidbuf[ii] >= 'A' && bidbuf[ii] <= 'Z') ||
           (bidbuf[ii] >= '0' && bidbuf[ii] <= '9') ||
           (bidbuf[ii] == '_')                     ) {
        desc[dlen] = toupper(bidbuf[ii]);
        dlen++;
        ii++;
    }

    if (dlen) {
        desc[dlen] = '\0';
        bd = find_desc(si, desc);
        if (!bd) omarerr(og, "Can't find desc '%s'\n", desc);
        else {
            print_biddesc(og, bd);
            omarerr(og, "\n");
        }
    }
}
/***************************************************************/
/*@*/void handle_sys(struct omarglobals * og,
        SEQINFO * si,
        char * bidbuf) {

    struct sysdesc * sd;
    char sys[256];
    int  slen;
    int ii;

    ii = 1;
    for (ii = 1;isspace(bidbuf[ii]);ii++) ;

    slen = 0;
    while ((bidbuf[ii] >= 'a' && bidbuf[ii] <= 'z') ||
           (bidbuf[ii] >= 'A' && bidbuf[ii] <= 'Z') ||
           (bidbuf[ii] >= '0' && bidbuf[ii] <= '9') ||
           (bidbuf[ii] == '_')                     ) {
        sys[slen] = toupper(bidbuf[ii]);
        slen++;
        ii++;
    }

    if (slen) {
        sys[slen] = '\0';
        sd = find_sys(si, sys);
        if (!sd) omarerr(og, "Can't find sys '%s'\n", sys);
        else {
            print_system(og, sd);
            omarerr(og, "\n");
        }
    }
}
/***************************************************************/
/*@*/ANALYSIS * new_analysis(void) {

    ANALYSIS * analysis;

    analysis = New(ANALYSIS, 1);

    new_analysis_reqinfo_lists(analysis);

    return (analysis);
}
/***************************************************************/
/*@*/AUCTION * new_auction(int dealer, int vulnerability) {

    AUCTION * auction;

    auction = New(AUCTION, 1);

    auction->au_dealer = dealer;
    auction->au_vulnerability = vulnerability;

    return (auction);
}
/***************************************************************/
/*@*/void init_cc(struct omarglobals * og, SEQINFO * si) {

    struct sysdesc * sd;

    si->cc = New(struct convcard, 1);

    sd = find_sys(si, "CCEW");
    if (!sd) {
        omarerr(og, "Unable to find convention card for EAST-WEST card.\n");
        sd = find_sys(si, "STANDARD_AMERICAN");
        if (!sd) {
            omarerr(og, "Unable to find STANDARD_AMERICAN convention card.\n");
        } else {
            omarout(og, "Using STANDARD_AMERICAN.\n");
        }
    }
    si->cc->ccs[0] = sd;    /* EAST_WEST */

    sd = find_sys(si, "CCNS");
    if (!sd) {
        omarerr(og, "Unable to find convention card for NORTH-SOUTH card.\n");
        sd = find_sys(si, "STANDARD_AMERICAN");
        if (!sd) {
            omarerr(og, "Unable to find STANDARD_AMERICAN convention card.\n");
        } else {
            omarout(og, "Using STANDARD_AMERICAN.\n");
        }
    }
    si->cc->ccs[1] = sd;    /* NORTH_SOUTH */
}
/***************************************************************/
/*@*/void free_cc(SEQINFO * si) {

    Free(si->cc);
    si->cc = NULL;
}
/***************************************************************/
/*@*/void handle_auction(struct omarglobals * og,
        SEQINFO * si,
        char * bidbuf) {

    AUCTION * auction;
    ANALYSIS * analysis;
    int ptr;
    int bt;
    int bl;
    int bs;
    int sdone;
    int anat;

    auction = new_auction(0, VUL_NONE);
    ptr = 1;
    anat = 0;
    while (isspace(bidbuf[ptr])) ptr++;
    if (bidbuf[ptr] == '*') {
        anat = 1;
        ptr++;
    }
    sdone = 0;

    while (!sdone) {
        next_bid(og, bidbuf, &ptr, &bt, &bl, &bs);
        if (bt == NONE) sdone = 1;
        else make_bid(auction, bt, bl, bs);
    }

    if (auction->au_nbids) {
        analysis = new_analysis();
        init_cc(og, si);
        analyze_auction(og, si, auction, analysis, anat);
        print_analysis(og, auction, analysis);
        free_analysis(analysis);
        free_cc(si);
    }
    Free(auction);
}
/***************************************************************/
/*@*/int parse_hand(
        struct omarglobals * og,
        const char * bidbuf,
        int * ptr,
        HAND * hand) {
/*
** See also jsonstring_to_hand()
*/

    int ok;
    int ncards;
    int card;
    int suit;
    int current_suit = 0;

    for (suit = 0; suit < 4; suit++) hand->length[suit] = 0;

    ok = 1;
    ncards = 0;
    while (ok && ncards < 13) {
        ok = next_card(og, bidbuf, ptr, &card, &suit, &current_suit);
        if (ok) {
            if (hand->length[suit] &&
                hand->cards[suit][hand->length[suit]-1] <= card) {
                omarerr(og, "Card out of order within suit.\n");
                ok = 0;
             } else {
                hand->cards[suit][hand->length[suit]++] = card;
                ncards++;
            }
        }
    }

    if (!ok) {
        omarerr(og, "Less than 13 cards in hand\n");
        return (0);
    }

    ok = next_card(og, bidbuf, ptr, &card, &suit, &current_suit);
    if (ok) {
        omarerr(og, "More than 13 cards in hand\n");
        return (0);
    }

    return (1);
}
/***************************************************************/
/*@*/void handle_hand(struct omarglobals * og,
            SEQINFO * si,
            char * bidbuf)
{
/*
** See also: nntest_handle_hand()
*/

    AUCTION * auction;
    ANALYSIS * analysis;
    HANDINFO handinfo;
    int ptr;
    int ok;
    HAND hand;
    int bt;
    int bl;
    int bs;
    int bestbid;
    int trace;
    int quiet;

    trace = 0;
    if (og->og_flags & OG_FLAG_TRACE) trace = 1;
    quiet = 1;
    ok = 1;
    ptr = 1;
    while (isspace(bidbuf[ptr])) ptr++;
    if (bidbuf[ptr] == '*') {
        trace = 1;
        ptr++;
    } else if (bidbuf[ptr] == '.') {
        quiet = 0;
        ptr++;
    }

    ok = parse_hand(og, bidbuf, &ptr, &hand);
    if (!ok) return;
    ptr++; /* Skip ; */

    make_handinfo(&hand, &handinfo);
    handinfo.trace = trace;

    auction = new_auction(0, VUL_NONE);

    while (ok) {
        next_bid(og, bidbuf, &ptr, &bt, &bl, &bs);
        if (bt == NONE) ok = 0;
        else make_bid(auction, bt, bl, bs);
    }

    ptr++;
    next_bid(og, bidbuf, &ptr, &bt, &bl, &bs);  /* Get answer */

    if (bidbuf[ptr] == ';') {
        ptr++;
        while (isspace(bidbuf[ptr])) ptr++;
    }

    if (auction->au_nbids || bt != NONE) {

        if (trace) {
            omarout(og, "(%s) -\n", &(bidbuf[ptr]));
            print_hand(og, &hand);
            print_auction(og, auction);
        }

        analysis = new_analysis();
        init_cc(og, si);
        analyze_auction(og, si, auction, analysis, 0);

        bestbid = find_best_bid(og, si, auction, analysis, &handinfo);
        if (bestbid == NOBID) {
            omarout(og, "(%s) * No bids available *\n", &(bidbuf[ptr]));
        } else {
            int recbid;;

            recbid = encode_bid(bt, bl, bs);
            if (bestbid != recbid) {
        bestbid = find_best_bid(og, si, auction, analysis, &handinfo);
                if (!trace) {
                    print_hand(og, &hand);
                    print_auction(og, auction);
                }
                omarout(og, "(%s) Best bid found is ", &(bidbuf[ptr]));
                print_ebid(og, bestbid);
                omarout(og, ", but recommended bid is ");
                print_ebid(og, recbid);
                omarout(og, ".\n");
            } else if (trace || !quiet) {
                omarout(og, "(%s) ", &(bidbuf[ptr]));
                print_ebid(og, bestbid);
                omarout(og, " bid is correct.\n");
            }
        }

        free_analysis(analysis);
        free_cc(si);
    }
    free_auction(auction);
}
/***************************************************************/
/*@*/void handle_natural(struct omarglobals * og, SEQINFO * si, char * bidbuf) {

    AUCTION * auction;
    ANALYSIS * analysis;
    HANDINFO handinfo;
    int ptr;
    int ok;
    HAND hand;
    int bt;
    int bl;
    int bs;
    int bestbid;
    int trace;
    int recbid;

    trace = 0;
    ok = 1;
    ptr = 1;
    while (isspace(bidbuf[ptr])) ptr++;
    if (bidbuf[ptr] == '*') {
        trace = 1;
        ptr++;
    }

    ok = parse_hand(og, bidbuf, &ptr, &hand);
    if (!ok) return;
    ptr++; /* Skip ; */

    make_handinfo(&hand, &handinfo);
    handinfo.trace = trace;

    auction = new_auction(0, VUL_NONE);

    while (ok) {
        next_bid(og, bidbuf, &ptr, &bt, &bl, &bs);
        if (bt == NONE) ok = 0;
        else make_bid(auction, bt, bl, bs);
    }

    ptr++;
    next_bid(og, bidbuf, &ptr, &bt, &bl, &bs);  /* Get answer */

    if (bidbuf[ptr] == ';') {
        ptr++;
        while (isspace(bidbuf[ptr])) ptr++;
    }

    if (auction->au_nbids) {

        if (trace) {
            omarout(og, "(%s) -\n", &(bidbuf[ptr]));
            print_hand(og, &hand);
            print_auction(og, auction);
        }

        analysis = new_analysis();
        init_cc(og, si);
        analyze_auction(og, si, auction, analysis, 0);

        bestbid = find_natural_bid(og, si, auction, analysis, &handinfo);
        if (!bestbid) {
            omarout(og, "(%s) * No natural bid found *\n", &(bidbuf[ptr]));
            return;
        }

        recbid = encode_bid(bt, bl, bs);
        if (bestbid != recbid) {
            if (!trace) {
                print_hand(og, &hand);
                print_auction(og, auction);
            }
            omarout(og, "(%s) Best bid found is ", &(bidbuf[ptr]));
            print_ebid(og, bestbid);
            omarout(og, ", but recommended bid is ");
            print_ebid(og, recbid);
            omarout(og, ".\n");
        } else {
            omarout(og, "(%s) ", &(bidbuf[ptr]));
            print_ebid(og, bestbid);
            omarout(og, " bid is correct.\n");
        }

        Free(analysis);
        free_cc(si);
    }

    Free(auction);
}
/***************************************************************/
/*@*/void pause(char * text) {

    char bbuf[256];
    int ptr;

    if (!text[0]) fputs("-->", stdout);
    else {
        if (text[0] == ' ') text++;
        fputs(text, stdout);
    }

    fgets(bbuf, sizeof(bbuf), stdin);
    ptr = 0;
    while (isspace(bbuf[ptr])) ptr++;
    if (bbuf[ptr] == 'e' || bbuf[ptr] == 'E') exit (0);
}
/***************************************************************/
/*@*/void echo(char * text) {

    if (text[0] == ' ') text++;

    fputs(text, stdout);
    fputs("\n", stdout);
}
/***************************************************************/
/*@*/void analyze_bidfile(struct omarglobals * og, struct frec * bidtest_fr, SEQINFO *si)
{
/*
** See also: nntest_get_hands()
*/

    char bidbuf[TOKEN_MAX];
    int ok;
    int done;
    int len;
    int ptr;
    int comment;

    comment = 0;
    done = 0;
    while (!done) {

        ok = frec_gets(bidbuf, sizeof(bidbuf), bidtest_fr);
        if (ok < 0) return;

        len = IStrlen(bidbuf);
        while (len && isspace(bidbuf[len-1])) len--;
        bidbuf[len] = '\0';

        ptr = 0;
        while (ptr < len && isspace(bidbuf[ptr])) ptr++;

        if (comment) {
            while (ptr < len && bidbuf[ptr] != '}') ptr++;
            if (ptr < len) {
                comment = 0;
                ptr++;
                while (ptr < len && isspace(bidbuf[ptr])) ptr++;
            }
        }

        if (!comment) {
            if (ptr >= len || bidbuf[ptr] == '#') ;
            else if (bidbuf[ptr] == 'e' || bidbuf[ptr] == 'E') done = 1;
            else if (bidbuf[ptr] == '{') comment = 1;
            else if (bidbuf[ptr] == 'd' || bidbuf[ptr] == 'D') {
                handle_desc(og, si, &(bidbuf[ptr]));
            } else if (bidbuf[ptr] == 'a' || bidbuf[ptr] == 'A') {
                handle_auction(og, si, &(bidbuf[ptr]));
            } else if (bidbuf[ptr] == 'h' || bidbuf[ptr] == 'H') {
                handle_hand(og, si, &(bidbuf[ptr]));
            } else if (bidbuf[ptr] == 'n' || bidbuf[ptr] == 'N') {
                handle_natural(og, si, &(bidbuf[ptr]));
            } else if (bidbuf[ptr] == 'p' || bidbuf[ptr] == 'P') {
                pause(&(bidbuf[ptr+1]));
            } else if (bidbuf[ptr] == 'q' || bidbuf[ptr] == 'Q') {
                echo(&(bidbuf[ptr+1]));
            } else if (bidbuf[ptr] == 's' || bidbuf[ptr] == 'S') {
                handle_sys(og, si, &(bidbuf[ptr]));
            } else {
                omarerr(og, "Don't understand: '%s'\n", bidbuf);
            }
        }
    }
}
/***************************************************************/
/*@*/int find_longest(HAND * hand, int longestn) {

    int lens[4];
    int suits[4];
    int ii;

    for (ii = 3; ii>= 0; ii--) {
        lens[ii] = hand->length[ii];
        suits[ii] = ii;
    }

    sort_int4(lens, suits);

    return (suits[4-longestn]);
}
/***************************************************************/
/*@*/int calc_support(HAND * hand, int trumpsuit) {

    int suit;
    int cardix;
    int pts;
    int shcp;
    int ii;

    pts = 0;
    for (suit = 0; suit < 4; suit++) {
        if (suit != trumpsuit) ii = (3 - hand->length[suit]) * 2 - 1;
        else ii = (hand->length[suit] - 4);
        if (ii > 0) pts += ii;

        shcp = 0;
        for (cardix = 0;
            cardix < hand->length[suit] && hand->cards[suit][cardix] >= 11;
            cardix++) {
            shcp += (hand->cards[suit][cardix] - 10);
        }
        pts += shcp;
        if (suit == trumpsuit && shcp) pts += 1;
    }

    return (pts);
}
/***************************************************************/
/*@*/int count_top(HAND * hand, int suit, int topn) {

    register int ii;
    register int n;
    register int c;
    register int top;

    top = 0;
    ii = 0;
    c = 13 - topn;
    if (topn > hand->length[suit]) n = hand->length[suit];
    else n = topn;

    while (ii < n && hand->cards[suit][ii] >= c) {
        top++;
        ii++;
    }

    return (top);
}
/***************************************************************/
/*@*/FLOATING run_prog(
                    struct omarglobals * og,
                    struct program * prog,
                    AUCTION * auction,
                    ANALYSIS * analysis,
                    HANDINFO * phandinfo,
                    struct handrec * hr) {

    int op;
    int lab;
    int suitspec;
    FLOATING num;
    int ii;
    int tos;
    FLOATING stack[STACK_SIZE];
    int done;
    int player;
    HANDINFO * handinfo;

    done = 0;
    tos = 0;
    player = NULL_PLAYER;
    handinfo = phandinfo;

    prog->progptr = 0;
    while (!done) {
        op = fetch_prog(prog);
        switch (op) {
            case ADD_OP      :
                stack[tos-2] = stack[tos-2] + stack[tos-1];
                tos--;
                break;
            case BF_OP       :
                lab = fetch_prog_int(prog);
                if (!stack[tos-1]) prog->progptr = lab;
                else tos--;
                break;
            case BFDEL_OP       :
                lab = fetch_prog_int(prog);
                if (!stack[tos-1]) prog->progptr = lab;
                tos--;
                break;
            case BR_OP       :
                lab = fetch_prog_int(prog);
                prog->progptr = lab;
                break;
            case BT_OP       :
                lab = fetch_prog_int(prog);
                if (stack[tos-1]) prog->progptr = lab;
                else tos--;
                break;
            case DIV_OP      :
                stack[tos-2] = stack[tos-2] / stack[tos-1];
                tos--;
                break;
            case EQ_OP       :
                stack[tos-2] = (FLOATING)(stack[tos-2] == stack[tos-1]);
                tos--;
                break;
            case END_OP      :
                done = 1;
                break;
            case GE_OP       :
                stack[tos-2] = (FLOATING)(stack[tos-2] >= stack[tos-1]);
                tos--;
                break;
            case GT_OP       :
                stack[tos-2] = (FLOATING)(stack[tos-2] > stack[tos-1]);
                tos--;
                break;
            case LE_OP       :
                stack[tos-2] = (FLOATING)(stack[tos-2] <= stack[tos-1]);
                tos--;
                break;
            case LOADI_OP    :
                num = fetch_prog_float(prog);
                stack[tos++] = num;
                break;
            case LOADS_OP    :
                num = fetch_prog_float(prog);
                suitspec = (int)num;
                if (auction && analysis) {
                    suitspec = interp_suitspec(suitspec,
                           (auction->au_dealer + auction->au_nbids - 1) & 3,
                           &(analysis->ani_ril));
                    if (suitspec == SUIT_SUIT) {
                        int bt;
                        int bl;
                        int bs;

                        decode_bid(auction->au_bid[auction->au_nbids-1],&bt, &bl, &bs);
                        if (bt == BID && bs != NOTRUMP) suitspec = bs + CLUB_SUIT;
                        else suitspec = NO_SUIT;
                    }
                }
                stack[tos++] = (FLOATING)suitspec;
                break;
            case LT_OP       :
                stack[tos-2] = (FLOATING)(stack[tos-2] < stack[tos-1]);
                tos--;
                break;
            case MOD_OP      :
                stack[tos-2] = (FLOATING)fmod(stack[tos-2], stack[tos-1]);
                tos--;
                break;
            case MUL_OP      :
                stack[tos-2] = stack[tos-2] * stack[tos-1];
                tos--;
                break;
            case NE_OP       :
                stack[tos-2] = (FLOATING)(stack[tos-2] != stack[tos-1]);
                tos--;
                break;
            case IN_OP       :
                stack[tos-3] = (FLOATING)((stack[tos-3] >= stack[tos-2]) && (stack[tos-3] <= stack[tos-1]));
                tos -= 2;
                break;
            case NEG_OP      :
                stack[tos-1] = -stack[tos-1];
                break;
            case NOT_OP      :
                stack[tos-1] = (FLOATING)(stack[tos-1]?1:0);
                break;
            case SUB_OP      :
                stack[tos-2] = stack[tos-2] - stack[tos-1];
                tos--;
                break;

            case WEST_OP:
            case NORTH_OP:
            case EAST_OP:
            case SOUTH_OP:
                if (hr) {
                    player = op - WEST_OP;
                    handinfo = phandinfo + player;
                }
                break;

            case DPTS_FUNC    :
                num = handinfo->value[REQ_DPTS];
                stack[tos++] = num;
                break;
            case HCP_FUNC     :
                num = handinfo->value[REQ_HCP];
                stack[tos++] = num;
                break;
            case LONGEST_FUNC :
                ii = (int)stack[tos-1];
                if (ii >= 1 && ii <= 4) {
                    num =
                    (FLOATING)(find_longest(handinfo->hand,ii) + CLUB_SUIT);
                } else {
                    num = 0;
                }
                stack[tos-1] = num;
                break;
            case PTS_FUNC     :
                num = handinfo->value[REQ_PTS];
                stack[tos++] = num;
                break;
            case QT_FUNC      :
                num = handinfo->value[REQ_QT];
                stack[tos++] = num;
                break;
            case SHCP_FUNC    :
                suitspec = (int)stack[tos-1];
                if (suitspec >= CLUB_SUIT && suitspec <= SPADE_SUIT) {
                    num =
                    handinfo->value[suitspec - CLUB_SUIT + REQ_SHCP_CLUBS];
                } else {
                    num = 0;
                }
                stack[tos-1] = num;
                break;
            case SLEN_FUNC    :
                suitspec = (int)stack[tos-1];
                if (suitspec >= CLUB_SUIT && suitspec <= SPADE_SUIT) {
                    num =
                    handinfo->value[suitspec - CLUB_SUIT + REQ_SLEN_CLUBS];
                } else {
                    num = 0;
                }
                stack[tos-1] = num;
                break;
            case SNUM_FUNC    :
                suitspec = (int)stack[tos-1];
                if (suitspec >= CLUB_SUIT && suitspec <= SPADE_SUIT) {
                    num = (FLOATING)suitspec;
                } else {
                    num = 0;
                }
                stack[tos-1] = num;
                break;
            case SUPPORT_FUNC     :
                suitspec = (int)stack[tos-1];
                if (suitspec >= CLUB_SUIT && suitspec <= SPADE_SUIT) {
                    num = (FLOATING)calc_support(handinfo->hand, suitspec - CLUB_SUIT);
                } else {
                    num = 0;
                }
                stack[tos-1] = num;
                break;
            case TOP_FUNC     :
                suitspec = (int)stack[tos-2];
                num = 0;
                if (suitspec >= CLUB_SUIT && suitspec <= SPADE_SUIT) {
                    ii = (int)stack[tos-1];
                    if (ii >= 1 && ii <= 13) {
                        num = (FLOATING)count_top(handinfo->hand,
                                suitspec-CLUB_SUIT, ii);
                    }
                }
                tos--;
                stack[tos-1] = num;
                break;
            case TRICKS_FUNC      :
                num = handinfo->value[REQ_TRICKS];
                stack[tos++] = num;
                break;
            case MAKES_FUNC      :
                if (hr && player != NULL_PLAYER) {
                    suitspec = (int)stack[tos-1];
                    if (suitspec >= CLUB_SUIT && suitspec <= NT_SUIT) {
                        suitspec -= CLUB_SUIT;
                    } else {
                        suitspec = CLUB_SUIT;
                    }
                    num = (FLOATING)calculate_makes(og, hr, player, suitspec);
                    stack[tos-1] = num;
                }
                break;
            default          :
                omarerr(og, "Unrecognized op: ?(%d)", op);
                break;
        }
    }

    if (tos != 1) {
        omarerr(og, "** Stack not clean, tos=%d\n", tos);
    }

    return (stack[tos-1]);
}
/***************************************************************/


