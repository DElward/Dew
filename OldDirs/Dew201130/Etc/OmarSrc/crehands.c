/***************************************************************/
/* crehands.c                                                  */
/***************************************************************/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <math.h>

#include "omar.h"
#include "bid.h"
#include "onn.h"
#include "crehands.h"
#include "natural.h"
#include "util.h"
#include "randnum.h"
#include "json.h"
#include "handrec.h"

/***************************************************************/
/*     -createhands Functions                                  */
/***************************************************************/
/***************************************************************/
struct createhands * new_createhands(void) {

    struct createhands * crh;

    crh = New(struct createhands, 1);

    crh->crh_handtype     = NULL;
    crh->crh_numhands     = 0;
    crh->crh_outfname     = NULL;
    crh->crh_handno       = 50; /* random number seed */

    return (crh);
}
/***************************************************************/
void free_createhands(struct createhands * crh) {

    Free(crh->crh_handtype);
    Free(crh->crh_outfname);

    Free(crh);
}
/***************************************************************/
int get_createhands(struct omarglobals * og,
        struct createhands * crh,
        int * argix,
        int argc,
        char *argv[])
{
/*
struct createhands {
    char * crh_handtype;
    int    crh_numhands;
    char * crh_outfname;
};
*/
    int estat = 0;
    int nerr;

    if ((*argix) + 3 > argc) {
        estat = set_error(og, "Insufficient arguments for -createhands.");
    } else {
        if (argv[*argix][0] == '-') {
            estat = set_error(og, "Expecting handtype in -createhands. Found: %s",
                argv[*argix]);
        } else {
            crh->crh_handtype = Strdup(argv[*argix]);
            (*argix)++;
        }
    }

    if (!estat) {
        if (argv[*argix][0] == '-') {
            estat = set_error(og, "Expecting number of hands in -createhands. Found: %s",
                argv[*argix]);
        } else {
            nerr = char_to_integer(argv[*argix], &(crh->crh_numhands));
            if (nerr) {
                estat = set_error(og, "Bad number of hands in -createhands. Found: %s",
                    argv[*argix]);
            }
            (*argix)++;
        }
    }

    if (!estat) {
        if (argv[*argix][0] == '-') {
            estat = set_error(og, "Expecting handtype in -createhands. Found: %s",
                argv[*argix]);
        } else {
            crh->crh_outfname = Strdup(argv[*argix]);
            (*argix)++;
        }
    }

    if (!estat && argv[*argix][0] != '-') {
        nerr = char_to_integer(argv[*argix], &(crh->crh_handno));
        if (nerr) {
            estat = set_error(og, "Bad hand number in -createhands. Found: %s",
                argv[*argix]);
        }
        (*argix)++;
        crh->crh_handno--;
    }

    return (estat);
}
/***************************************************************/
/*@*/void calc_best_one_suit(HAND * hand,
                     IHANDINFO * ihandinfo,
                     int *bid_type,
                     int *bid_level,
                     int *bid_suit)
{
/*
** Bid 1 of a suit. Which one?
*/
    int one_two_diff;

    (*bid_type)  = BID;
    (*bid_level) = 1;

    if (ihandinfo->ordered_suit_lengths[3] >= 5) {
        one_two_diff = ihandinfo->ordered_suit_lengths[3] -
                       ihandinfo->ordered_suit_lengths[2];
        if (one_two_diff == 0) {
            /* 5-5 and 6-6 bids higher ranking suit */
            if (ihandinfo->ordered_suits[3] > ihandinfo->ordered_suits[2]) {
                (*bid_suit)  = ihandinfo->ordered_suits[3];
            } else {
                (*bid_suit)  = ihandinfo->ordered_suits[2];
            }
        } else if (ihandinfo->ordered_suits[3] > ihandinfo->ordered_suits[2] ||
            one_two_diff > 1 ||
            ihandinfo->ordered_suits[2] >= HEARTS ||
            ihandinfo->value[REQ_PTS] >= 17) {
            (*bid_suit)  = ihandinfo->ordered_suits[3];
        } else {
            (*bid_suit)  = ihandinfo->ordered_suits[2];
        }
    } else {    /* longest suit is 4 cards */
        if (hand->length[DIAMONDS] == 4 ||
            hand->length[DIAMONDS] > hand->length[CLUBS]) {
            (*bid_suit)  = DIAMONDS;
        } else {
            (*bid_suit)  = CLUBS;
        }
    }
}
/***************************************************************/
/*@*/void calc_two_notrump(HAND * hand,
                     IHANDINFO * ihandinfo,
                     int *bid_type,
                     int *bid_level,
                     int *bid_suit)
{
/*
** Have 20-21 hcp, no singleton or void
*/
    if (hand->length[HEARTS] >= 5 ||
        hand->length[SPADES] >= 5) {
        calc_best_one_suit(hand, ihandinfo,
                bid_type, bid_level, bid_suit);
    } else {
        (*bid_type)  = BID;
        (*bid_level) = 2;
        (*bid_suit) = NOTRUMP;
    }
}
/***************************************************************/
/*@*/void calc_three_notrump(HAND * hand,
                     IHANDINFO * ihandinfo,
                     int *bid_type,
                     int *bid_level,
                     int *bid_suit)
{
/*
** Have 25-27 hcp, no singleton or void
*/
    if (hand->length[HEARTS] >= 5 ||
        hand->length[SPADES] >= 5) {
        (*bid_type)  = BID;
        (*bid_level) = 2;
        (*bid_suit) = CLUBS;
    } else {
        (*bid_type)  = BID;
        (*bid_level) = 3;
        (*bid_suit)  = NOTRUMP;
    }
}
/***************************************************************/
/*@*/void calc_maximum_opening_bid(HAND * hand,
                     IHANDINFO * ihandinfo,
                     int *bid_type,
                     int *bid_level,
                     int *bid_suit)
{
/*
** Have >= 20 pts
*/
    if (ihandinfo->value[REQ_HCP] >= 20 &&
        ihandinfo->value[REQ_HCP] <= 21 &&
        ihandinfo->ordered_suit_lengths[0] >= 2) {
        calc_two_notrump(hand, ihandinfo,
                bid_type, bid_level, bid_suit);
    } else if (ihandinfo->value[REQ_HCP] >= 25 &&
        ihandinfo->value[REQ_HCP] <= 27 &&
        ihandinfo->ordered_suit_lengths[0] >= 2) {
        calc_three_notrump(hand, ihandinfo,
                bid_type, bid_level, bid_suit);
    } else if (ihandinfo->value[REQ_PTS] >= 23 ||
        (ihandinfo->value[REQ_PTS] >=  21 &&
         ihandinfo->value[REQ_TRICKS] >= 9)) {
        (*bid_type)  = BID;
        (*bid_level) = 2;
        (*bid_suit) = CLUBS;
    } else {
        calc_best_one_suit(hand, ihandinfo,
                bid_type, bid_level, bid_suit);
    }
}
/***************************************************************/
/*@*/void calc_one_notrump(HAND * hand,
                     IHANDINFO * ihandinfo,
                     int *bid_type,
                     int *bid_level,
                     int *bid_suit)
{
/*
** Have 15-17 hcp, no singleton or void
*/
    if (hand->length[HEARTS] > 5 ||
        ihandinfo->value[REQ_SHCP_HEARTS] > 7 ||
        hand->length[SPADES] > 5 ||
        ihandinfo->value[REQ_SHCP_SPADES] > 7) {
        calc_best_one_suit(hand, ihandinfo,
                bid_type, bid_level, bid_suit);
    } else {
        (*bid_type)  = BID;
        (*bid_level) = 1;
        (*bid_suit) = NOTRUMP;
    }
}
/***************************************************************/
/*@*/void calc_opening_bid(HAND * hand,
                     IHANDINFO * ihandinfo,
                     int *bid_type,
                     int *bid_level,
                     int *bid_suit)
{
/*
** Have >= 14 pts
*/
    if (ihandinfo->value[REQ_PTS] >= 20) {
        calc_maximum_opening_bid(hand, ihandinfo,
                bid_type, bid_level, bid_suit);
    } else if (ihandinfo->value[REQ_HCP] >= 15 &&
               ihandinfo->value[REQ_HCP] <= 17 &&
               ihandinfo->ordered_suit_lengths[0] >= 2) {
        calc_one_notrump(hand, ihandinfo,
                bid_type, bid_level, bid_suit);
    } else {
        calc_best_one_suit(hand, ihandinfo,
                bid_type, bid_level, bid_suit);
    }
}
/***************************************************************/
/*@*/void calc_opening_preempt(HAND * hand,
                     IHANDINFO * ihandinfo,
                     int *bid_type,
                     int *bid_level,
                     int *bid_suit)
{
/*
** Have 7+ piece suit, < 14 pts
*/
    if (ihandinfo->value[REQ_HCP] >= 4 &&
        ihandinfo->value[REQ_HCP] <= 9) {
        (*bid_type)  = BID;
        (*bid_level) = 3;
        (*bid_suit)  = ihandinfo->ordered_suits[3];
    }
}
/***************************************************************/
/*@*/void calc_minimum_opening(HAND * hand,
                     IHANDINFO * ihandinfo,
                     int *bid_type,
                     int *bid_level,
                     int *bid_suit)
{
/*
** Have 12 or 13 pts
*/
    if (ihandinfo->value[REQ_PTS] == 12 &&
        ihandinfo->value[REQ_QT] >= 25) {
        calc_best_one_suit(hand, ihandinfo,
                bid_type, bid_level, bid_suit);
    } else if (ihandinfo->value[REQ_QT] >= 20 ||
               ihandinfo->ordered_suit_lengths[0] == 0) {
        calc_best_one_suit(hand, ihandinfo,
                bid_type, bid_level, bid_suit);
    }
}
/***************************************************************/
/*@*/void calc_opening_weak_two(HAND * hand,
                     IHANDINFO * ihandinfo,
                     int *bid_type,
                     int *bid_level,
                     int *bid_suit)
{
/*
** Have 6 piece suit, no void, < 14 pts
*/
    if (ihandinfo->value[REQ_HCP] >= 5 &&
        ihandinfo->ordered_suits[3] != CLUBS &&
        (ihandinfo->ordered_suits[2] <= DIAMONDS ||
         ihandinfo->ordered_suit_lengths[2] < 4)) {
        /* Make sure long suit has 4+ HCP */
        if (ihandinfo->value[REQ_SHCP_CLUBS + ihandinfo->ordered_suits[3]] >= 4) {
            (*bid_type)  = BID;
            (*bid_level) = 2;
            (*bid_suit)  = ihandinfo->ordered_suits[3];
        }
    }
    
    if ((*bid_type) == PASS &&
        ihandinfo->value[REQ_PTS] >= 12) {
        calc_minimum_opening(hand, ihandinfo,
                bid_type, bid_level, bid_suit);
    }
}
/***************************************************************/
/*@*/void calculate_opening_bid(HAND * hand,
                     IHANDINFO * ihandinfo,
                     int *bid_type,
                     int *bid_level,
                     int *bid_suit)
{
/*
** longest_suit length  = handinfo->ordered_suit_lengths[3]
** longest_suit         = handinfo->ordered_suits[3]
** shortest_suit length = handinfo->ordered_suit_lengths[0]
** shortest_suit        = handinfo->ordered_suits[0]
*/

    (*bid_type)  = PASS;
    (*bid_level) = 0;
    (*bid_suit)  = 0;

    if (ihandinfo->value[REQ_PTS] >= 14) {
        calc_opening_bid(hand, ihandinfo,
                bid_type, bid_level, bid_suit);
    } else if ((ihandinfo->value[REQ_PTS] == 12 ||
                ihandinfo->value[REQ_PTS] == 13) &&
                (ihandinfo->ordered_suit_lengths[3] <= 5 ||
                 ihandinfo->ordered_suit_lengths[0] == 0)) {
        calc_minimum_opening(hand, ihandinfo,
                bid_type, bid_level, bid_suit);
    } else if (ihandinfo->ordered_suit_lengths[3] == 6) {
        if (ihandinfo->ordered_suit_lengths[0] > 0) {
            calc_opening_weak_two(hand, ihandinfo,
                    bid_type, bid_level, bid_suit);
        }
    } else if (ihandinfo->ordered_suit_lengths[3] > 6) {
        calc_opening_preempt(hand, ihandinfo,
                bid_type, bid_level, bid_suit);
    }
}
/***************************************************************/
int write_createhands(struct omarglobals * og,
        struct createhands * crh,
        HAND * hand,
        IHANDINFO * ihandinfo,
        int ebid,
        struct frec * outf)
{
/*
**
**  h S         , H AK632   , D AKQT43  , C T9      , ; ; P
*/
    int estat = 0;
    int bid_type;
    int bid_level;
    int bid_suit;
    int suit;
    int spaces;
    int ii;
    int cardix;

    estat = frec_printf(outf, "h ");

    for (suit = 3; suit >= 0; suit--) {
        estat = frec_printf(outf, "%c ", display_suit[suit]);
        for (cardix = 0; cardix < hand->length[suit]; cardix++) {
            estat = frec_printf(outf, "%c", display_rank[hand->cards[suit][cardix]]);
        }
        spaces = 8 - hand->length[suit];
        for (ii = 0; ii < spaces; ii++) estat = frec_printf(outf, " ");
        if (suit) estat = frec_printf(outf, ", ");
    }

    decode_bid(ebid, &bid_type, &bid_level, &bid_suit);

    estat = frec_printf(outf, "; ; ");
    if (bid_type == PASS) {
        estat = frec_printf(outf, "P ");
    } else {
        estat = frec_printf(outf, "%c%c", bid_level + '0', display_suit[bid_suit]);
    }
    estat = frec_printf(outf, "; %d\n", crh->crh_handno);

/*
    for (ii = 0; ii < 4; ii++) {
        estat = frec_printf(outf, "%c%c ",
               ihandinfo->ordered_suit_lengths[ii] + '0',
                   display_suit[ihandinfo->ordered_suits[ii]]);
    }
    estat = frec_printf(outf, "\n");
*/

    return (estat);
}
/***************************************************************/
/*@*/void make_ihandinfo(HAND * hand, IHANDINFO * ihandinfo) {

    int req;
    int suit;
    int ii;
    HANDINFO handinfo;
    int pslen[4];
    int which[4];

    make_handinfo(hand, &handinfo);

    memset(ihandinfo, 0, sizeof(IHANDINFO));
    ihandinfo->hand = hand;

    for (req = 0; req < NUM_REQUIREMENTS; req++) {
        if (req == REQ_QT) {
            ihandinfo->value[req] = (int)(handinfo.value[req] * 10.0);
        } else {
            ihandinfo->value[req] = (int)(handinfo.value[req]);
        }
    }

    for (suit = 3; suit >= 0; suit--) {
       pslen[suit] = hand->length[suit];
       which[suit] = suit;
    }
/* Sort by suit length */
    sort_int4(pslen, which);

    for (ii = 0; ii < 4; ii++) {
       ihandinfo->ordered_suit_lengths[ii] = hand->length[which[ii]];
       ihandinfo->ordered_suits[ii] = which[ii];
    }
}
/***************************************************************/
int process_one_createhands_open(struct omarglobals * og,
        struct createhands * crh,
        struct frec * outf)
{
/*
**
*/
    int estat = 0;
    int done;
    int player;
    int bid_type;
    int bid_level;
    int bid_suit;
    int ebid;
    DECK deck;
    HANDS hands;
    IHANDINFO ihandinfo;

    done = 0;
    while (!done) {
        crh->crh_handno++;
        deal(crh->crh_handno, &deck);
        sort_hands(&deck, &hands);
        player = 0;
        while (!done && player < 4) {
            make_ihandinfo(&(hands[player]), &ihandinfo);
            calculate_opening_bid(&(hands[player]), &ihandinfo,
                &bid_type, &bid_level, &bid_suit);
            if (bid_type != PASS || player == 3 ||
                (crh->crh_handno % 7) == 0) {
                done = 1;
            } else {
                if (ihandinfo.value[REQ_HCP] >= 10) done = 1;
                else player++;
            }
        }
    }

    ebid = encode_bid(bid_type, bid_level, bid_suit);
    estat = write_createhands(og, crh, &(hands[player]), &ihandinfo, ebid, outf);

    return (estat);
}
/***************************************************************/
int process_createhands_open(struct omarglobals * og,
        struct createhands * crh,
        struct frec * outf)
{
/*
**
*/
    int estat = 0;
    int ii;

    for (ii = 0; !estat && ii < crh->crh_numhands; ii++) {
        estat = process_one_createhands_open(og, crh, outf);
    }

    return (estat);
}
/***************************************************************/
int process_createhands(struct omarglobals * og,
        struct createhands * crh)
{
/*
**
*/
    int estat = 0;
    struct frec * outf;

    outf = frec_open(crh->crh_outfname, "w", "output");
    if (!outf) {
        estat = set_error(og, "Error opening file for output. File: %s",
            crh->crh_outfname);
    }

    if (!Stricmp(crh->crh_handtype, "open")) {
        estat = process_createhands_open(og, crh, outf);
    } else {
        estat = set_error(og, "Expecting OPEN handtype in -createhands. Found: %s",
            crh->crh_handtype);
    }

    frec_close(outf);

    return (estat);
}
/***************************************************************/
/***************************************************************/
/*     -nntest Functions                                  */
/***************************************************************/
/***************************************************************/
struct nnhands * new_nnhands(void) {

    struct nnhands * nnh;

    nnh = New(struct nnhands, 1);

    nnh->nnh_fname     = NULL;

    return (nnh);
}
/***************************************************************/
void free_nnhands(struct nnhands * nnh) {

    Free(nnh->nnh_fname);

    Free(nnh);
}
/***************************************************************/
int get_nnhands(struct omarglobals * og,
        struct nnhands * nnh,
        int * argix,
        int argc,
        char *argv[])
{
/*
struct nnhands {
    char * nnh_fname;
    int    nnh_method;
};
*/
    int estat = 0;

    if ((*argix) + 1 > argc) {
        estat = set_error(og, "Insufficient arguments for -nntest.");
    } else {
        if (argv[*argix][0] == '-') {
            estat = set_error(og, "Expecting handtype in -nntest. Found: %s",
                argv[*argix]);
        } else {
            nnh->nnh_fname = Strdup(argv[*argix]);
            (*argix)++;
        }
    }

    return (estat);
}
/***************************************************************/
struct nntesthand { /* nnth_ */
    HAND nnth_hand;
    int  nnth_enc_open_bid;
    int nnth_num_inputs;
    int nnth_num_outputs;
    ONN_FLOAT * nnth_inputs;
    ONN_FLOAT * nnth_outputs;
};
/***************************************************************/
struct nntesthand * new_nntesthand(void) {

    struct nntesthand * nnth;

    nnth = New(struct nntesthand, 1);

    return (nnth);
}
/***************************************************************/
void free_nntesthand(struct nntesthand * nnth) {

    Free(nnth);
}
/***************************************************************/
void free_nntesthandlist(struct nntesthand ** nnthlist) {

    int ii;

    if (nnthlist) {
        for (ii = 0; nnthlist[ii]; ii++) {
            free_nntesthand(nnthlist[ii]);
        }
        Free(nnthlist);
    }
}
/***************************************************************/
int nntest_handle_hand(struct omarglobals * og,
            struct nntesthand * nnth,
            char * bidbuf)
{
/*
** See also: handle_hand()
*/

    int estat = 0;
    int ptr;
    int ok;
    int bt;
    int bl;
    int bs;
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

    ok = parse_hand(og, bidbuf, &ptr, &(nnth->nnth_hand));
    if (!ok) {
        estat = set_error(og, "Invalid hand");
        return (estat);
    }
    ptr++; /* Skip ; */

    while (ok) {
        next_bid(og, bidbuf, &ptr, &bt, &bl, &bs);
        if (bt == NONE) ok = 0;
    }

    ptr++;
    next_bid(og, bidbuf, &ptr, &bt, &bl, &bs);  /* Get answer */
    nnth->nnth_enc_open_bid = encode_bid(bt, bl, bs);

    if (bidbuf[ptr] == ';') {
        ptr++;
        while (isspace(bidbuf[ptr])) ptr++;
    }

    return (estat);
}
/***************************************************************/
/*@*/struct nntesthand ** nntest_get_hands(struct omarglobals * og, struct frec * nntest_fr)
{
/*
** See also: analyze_bidfile()
*/

    int estat = 0;
    int ok;
    int done;
    int len;
    int ptr;
    int comment;
    int num_nnth;
    struct nntesthand * nnth;
    struct nntesthand ** nnthlist;
    char bidbuf[TOKEN_MAX];

    num_nnth = 0;
    nnthlist = NULL;
    comment = 0;
    done = 0;
    while (!done) {

        ok = frec_gets(bidbuf, sizeof(bidbuf), nntest_fr);
        if (ok < 0) {
            done = 1;
            if (!nntest_fr->feof) {
                estat = set_error(og, "Error reading file.");
            }
        } else {
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
                else if (bidbuf[ptr] == 'h' || bidbuf[ptr] == 'H') {
                    nnth = new_nntesthand();
                    estat = nntest_handle_hand(og, nnth, &(bidbuf[ptr]));
                    if (estat) {
                        free_nntesthand(nnth);
                    } else {
                        nnthlist = Realloc(nnthlist, struct nntesthand*, num_nnth + 2);
                        nnthlist[num_nnth++] = nnth;
                        nnthlist[num_nnth] = NULL;
                    }
                } else {
                    omarerr(og, "Don't understand: '%s'\n", bidbuf);
                }
            }
        }
    }

    if (estat) {
        free_nntesthandlist(nnthlist);
        nnthlist = NULL;
    }

    return (nnthlist);
}
/***************************************************************/
struct neural_network * nnt_create_mm_nn(struct omarglobals * og, struct nninit * nni)
{
/*
nn = NeuralNetwork(2, 2, 2, hidden_layer_weights=[0.15, 0.2, 0.25, 0.3], hidden_layer_bias=0.35, output_layer_weights=[0.4, 0.45, 0.5, 0.55], output_layer_bias=0.6)
for i in range(10000):
    nn.train([0.05, 0.1], [0.01, 0.99])
    print(i, round(nn.calculate_total_error([[[0.05, 0.1], [0.01, 0.99]]]), 9))
*/
    struct neural_network *  nn;
    ONN_FLOAT hidden_layer_weights[] = {0.15, 0.2, 0.25, 0.3};
    ONN_FLOAT output_layer_weights[] = {0.4, 0.45, 0.5, 0.55};

    nn = nn_new_init(2, 2, 2, 4, hidden_layer_weights, 0.35, 4, output_layer_weights,
     0.6, 0.5, 0.0);
    nn->nn_info = Strdup(NN_INFO_MM);

    return (nn);
}
/***************************************************************/
struct jsonvalue * nnt_neuron_to_jsonvalue(struct neuron * ne)
{
/*
struct neuron {
    int         ne_num_weights;
    ONN_FLOAT * ne_weights;
    ONN_FLOAT   ne_output;
    ONN_FLOAT   ne_bias;
    int         ne_num_inputs;
    ONN_FLOAT * ne_inputs;
};
*/
    struct jsonvalue * jv;
    struct jsonvalue * njv;
    int ii;

    jv = jsonp_new_jsonvalue(VTAB_VAL_OBJECT);
    jv->jv_val.jv_jo = jsonp_new_jsonobject();

    jsonobject_put_int(jv->jv_val.jv_jo, "ne_num_weights", ne->ne_num_weights);
    jsonobject_put_int(jv->jv_val.jv_jo, "ne_num_inputs", ne->ne_num_inputs);
    jsonobject_put_double(jv->jv_val.jv_jo, "ne_output", ne->ne_output);
    jsonobject_put_double(jv->jv_val.jv_jo, "ne_bias", ne->ne_bias);

    njv = jsonp_new_jsonvalue(VTAB_VAL_ARRAY);
    njv->jv_val.jv_ja = jsonp_new_jsonarray();

    if (ne->ne_weights) {
        for (ii = 0; ii < ne->ne_num_weights; ii++) {
            jsonarray_put_double(njv->jv_val.jv_ja, ne->ne_weights[ii]);
        }
        jsonobject_put_jv(jv->jv_val.jv_jo, "ne_weights", njv);
    }

    njv = jsonp_new_jsonvalue(VTAB_VAL_ARRAY);
    njv->jv_val.jv_ja = jsonp_new_jsonarray();

    if (ne->ne_inputs) {
        for (ii = 0; ii < ne->ne_num_inputs; ii++) {
            jsonarray_put_double(njv->jv_val.jv_ja, ne->ne_inputs[ii]);
        }
        jsonobject_put_jv(jv->jv_val.jv_jo, "ne_inputs", njv);
    }

    return (jv);
}
/***************************************************************/
struct jsonvalue * nnt_neural_layer_to_jsonvalue(struct neural_layer * nl)
{
    struct jsonvalue * jv;
    struct jsonvalue * njv;
    struct jsonvalue * ijv;
    int ii;

    jv = jsonp_new_jsonvalue(VTAB_VAL_OBJECT);
    if (!nl) return (jv);

    jv->jv_val.jv_jo = jsonp_new_jsonobject();

    jsonobject_put_int(jv->jv_val.jv_jo, "nl_num_neurons", nl->nl_num_neurons);
    jsonobject_put_double(jv->jv_val.jv_jo, "nl_bias", nl->nl_bias);

    njv = jsonp_new_jsonvalue(VTAB_VAL_ARRAY);
    njv->jv_val.jv_ja = jsonp_new_jsonarray();

    if (nl->nl_neurons) {
        for (ii = 0; ii < nl->nl_num_neurons; ii++) {
            ijv = nnt_neuron_to_jsonvalue(nl->nl_neurons[ii]);
            jsonarray_put_jv(njv->jv_val.jv_ja, ijv);
        }
        jsonobject_put_jv(jv->jv_val.jv_jo, "nl_neurons", njv);
    }

    return (jv);
}
/***************************************************************/
struct jsonvalue * nnt_neural_network_to_jsonvalue(struct neural_network * nn)
{
    struct jsonvalue * jv;
    struct jsonvalue * nljv;

    jv = jsonp_new_jsonvalue(VTAB_VAL_OBJECT);
    jv->jv_val.jv_jo = jsonp_new_jsonobject();

    if (nn->nn_info) jsonobject_put_string(jv->jv_val.jv_jo, "nn_info", nn->nn_info);
    if (nn->nn_elapsed_time) jsonobject_put_string(jv->jv_val.jv_jo, "nn_elapsed_time", nn->nn_elapsed_time);
    jsonobject_put_double(jv->jv_val.jv_jo, "nn_learning_rate", nn->nn_learning_rate);
    jsonobject_put_double(jv->jv_val.jv_jo, "nn_momentum", nn->nn_momentum);
    jsonobject_put_int(jv->jv_val.jv_jo, "nn_num_inputs", nn->nn_num_inputs);
    jsonobject_put_int(jv->jv_val.jv_jo, "nn_num_outputs", nn->nn_num_outputs);
    jsonobject_put_double(jv->jv_val.jv_jo, "nn_total_error", nn->nn_total_error);
    jsonobject_put_int(jv->jv_val.jv_jo, "nn_num_epochs", nn->nn_num_epochs);

    if (nn->nn_hidden_layer) {
        nljv = nnt_neural_layer_to_jsonvalue(nn->nn_hidden_layer);
        jsonobject_put_jv(jv->jv_val.jv_jo, "nn_hidden_layer", nljv);
    }

    if (nn->nn_output_layer) {
        nljv = nnt_neural_layer_to_jsonvalue(nn->nn_output_layer);
        jsonobject_put_jv(jv->jv_val.jv_jo, "nn_output_layer", nljv);
    }

    return (jv);
}
/***************************************************************/
struct jsontree * nnt_neural_network_to_jsontree(struct neural_network *  nn)
{
    struct jsontree * jt;

    jt = jsonp_new_jsontree();
    jt->jt_jv = nnt_neural_network_to_jsonvalue(nn);

    return (jt);
}
/***************************************************************/
int nnt_save(struct omarglobals * og,
        struct neural_network *  nn,
        const char * savfname)
{
    int estat = 0;
    struct jsontree * jt;
    FILE * savf;
    int pjtflags;

    jt = nnt_neural_network_to_jsontree(nn);
    if (jt) {
        savf = fopen(savfname, "w");
        if (!savf) {
            int ern = ERRNO;
            estat = set_error(og, "Error opening file %s: %m", savfname, ern);
        } else {
            pjtflags = PJT_FLAG_INDENT | PJT_FLAG_LF | PJT_FLAG_PRETTY;
            estat = print_jsontree(savf, jt, pjtflags);
            fclose(savf);
        }
        free_jsontree(jt);
    }

    return (estat);
}
/***************************************************************/
#define RANDINT_TESTS 0
#define RANDFLT_TESTS 0

#if RANDINT_TESTS || RANDFLT_TESTS
int knuthint_main(void);
int knuthflt_main(void);
long knuthint_next(void);
double knuthflt_next(void);
void ran_start(long seed);
void ranf_start(long seed);
#define ABS(n) ((n)<0?-(n):(n))
void random_test(void)
{
/*
KnuthInt       1  273825514  273825514
KnuthInt       2  706022089  706022089
KnuthInt       3   88634575   88634575
KnuthInt   40000  773071064  773071064

KnuthFlt       1 0.76193708364178114323 0.76193708364178114323
KnuthFlt       2 0.57077179216726103839 0.57077179216726103839
KnuthFlt       3 0.64501894629920220048 0.64501894629920220048
KnuthFlt   40000 0.34875447909314916117 0.34875447909314916117

NumRec1Flt     1 0.87221868656213330517
NumRec1Flt     2 0.85522014873811047497
NumRec1Flt     3 0.95498113006119667023
NumRec1Flt 40000 0.17798810972738457470
*/
    int ntests;
    int nerrs;
    int ii;
#if RANDINT_TESTS
    void * vi;
    long nnk;
    long nnr;
#endif
#if RANDFLT_TESTS
    void * vf;
    void * vn;
    double ddk;
    double ddr;
#endif

    nerrs = 0;
    ntests = 40000;
    ntests = 3;

#if RANDINT_TESTS
    /**** Integer tests *****/
    vi = randintseed(RANDNUM_GENINT_2, 0);
    ran_start(316277L);        /* sqrt(10) = 3.16227766 */

    //knuthint_main();
    printf("---- Starting integer tests.\n");
    for (ii = 0; ii < ntests; ii++) {
        nnk = knuthint_next();
        nnr = randintnext(vi);
        if (ntests <= 10 || ii + 1 == ntests)
            printf("KnuthInt   %5d %10ld %10ld\n", ii+1, nnk, nnr);
        if (nnk != nnr) nerrs++;
    }
    randintfree(vi);

    printf("\n");
    printf("----\n");
#endif
    printf("\n");

#if RANDFLT_TESTS
    /**** Floating tests *****/
    vf = randfltseed(RANDNUM_GENFLT_2, 0);
    ranf_start(316277L);        /* sqrt(10) = 3.16227766 */

    for (ii = 0; ii < ntests; ii++) {
        ddk = knuthflt_next();
        ddr = randfltnext(vf);
        if (ntests <= 10 || ii + 1 == ntests)
            printf("KnuthFlt   %5d %.20f %.20f\n", ii + 1, ddk, ddr);
        //if (ABS(ddk - ddr) > 0.000000001) nerrs++;
        if (ddk != ddr) nerrs++;
    }
    printf("\n");
    randfltfree(vf);

    printf("---- NUMREC ran1\n");
    /* Numrec 4=0.29515362358426378009 */
    vn = randfltseed(RANDNUM_GENFLT_5, 0);
    for (ii = 0; ii < ntests; ii++) {
        ddr = randfltnext(vf);
        if (ntests <= 10 || ii + 1 == ntests)
            printf("NumRec1Flt %5d %.20f\n", ii + 1, ddr);
    }
    randfltfree(vn);
#endif
    if (!nerrs) {
        printf("No errors in %d tests.\n", ntests);
    } else {
        printf("******** %d errors in %d tests.\n", nerrs, ntests);;
    }
}
#endif
/***************************************************************/
struct nntesthand ** cat_nnthlist(struct nntesthand ** nnthlist1, struct nntesthand ** nnthlist2)
{
    int nnthlist_len1;
    int nnthlist_len2;
    int ii;
    struct nntesthand ** nnthlist;

    if (!nnthlist1) return (nnthlist2);
    if (!nnthlist2) return (nnthlist1);

    nnthlist_len1 = 0;
    for (ii = 0; nnthlist1[ii]; ii++) nnthlist_len1++;
    if (!nnthlist_len1) { Free(nnthlist1); return (nnthlist2); }

    nnthlist_len2 = 0;
    for (ii = 0; nnthlist2[ii]; ii++) nnthlist_len2++;
    if (!nnthlist_len2) { Free(nnthlist2); return (nnthlist1); }

    nnthlist = New(struct nntesthand *, nnthlist_len1 + nnthlist_len2 + 1);
    for (ii = 0; nnthlist1[ii]; ii++) nnthlist[ii] = nnthlist1[ii];
    for (ii = 0; nnthlist2[ii]; ii++) nnthlist[nnthlist_len1 + ii] = nnthlist2[ii];

    nnthlist[nnthlist_len1 + nnthlist_len2] = NULL;
    Free(nnthlist1);
    Free(nnthlist2);

    return (nnthlist);
}
/***************************************************************/
int process_nnhands(struct omarglobals * og,
        struct nnhands * nnh)
{
/*
**
*/
    int estat = 0;
    struct frec * nntest_fr;
    struct nntesthand ** nnthlist;

#if RANDINT_TESTS || RANDFLT_TESTS
    random_test(); return (0);
#endif

    nnthlist = NULL;

    /* Filename of "*" means delete all hands */
    if (!strcmp(nnh->nnh_fname, "*")) {
        free_nntesthandlist(og->og_nnthlist);
        og->og_nnthlist = NULL;
        return (0);
    }

    nntest_fr = frec_open(nnh->nnh_fname, "r", "NN test file");
    if (!nntest_fr) {
        estat = set_error(og, "Error opening file for input. File: %s",
            nnh->nnh_fname);
    }

    if (!estat) {
        nnthlist = nntest_get_hands(og, nntest_fr);
    }

    if (nnthlist) {
        og->og_nnthlist = cat_nnthlist(og->og_nnthlist, nnthlist);
    }

    frec_close(nntest_fr);

    return (estat);
}
/***************************************************************/
struct neural_network * nnt_create_new_nn(
        struct omarglobals * og,
        struct nninit * nni)
{
/*
nn = NeuralNetwork(2, 2, 2, hidden_layer_weights=[0.15, 0.2, 0.25, 0.3], hidden_layer_bias=0.35, output_layer_weights=[0.4, 0.45, 0.5, 0.55], output_layer_bias=0.6)
for i in range(10000):
    nn.train([0.05, 0.1], [0.01, 0.99])
    print(i, round(nn.calculate_total_error([[[0.05, 0.1], [0.01, 0.99]]]), 9))

    struct neural_network * nn_new_init(
    int num_inputs,
    int num_hidden,
    int num_outputs,
    int num_hidden_layer_weights,
    ONN_FLOAT * hidden_layer_weights,
    ONN_FLOAT hidden_layer_bias,
    int num_output_layer_weights,
    ONN_FLOAT * output_layer_weights,
    ONN_FLOAT output_layer_bias,
    ONN_FLOAT learning_rate);
*/
    struct neural_network *  nn;
    int num_hidden_layer_weights;
    int num_output_layer_weights;
    ONN_FLOAT * hidden_layer_weights;
    ONN_FLOAT * output_layer_weights;

    num_hidden_layer_weights = nni->nni_num_inputs  * nni->nni_num_hidden_neurons;
    num_output_layer_weights = nni->nni_num_outputs * nni->nni_num_hidden_neurons;

    hidden_layer_weights = New(ONN_FLOAT, num_hidden_layer_weights);
    output_layer_weights = New(ONN_FLOAT, num_output_layer_weights);

    if (nni->nni_init == NNI_INIT_RANDOM) {
        nn_randomize_weights(num_hidden_layer_weights, hidden_layer_weights,
                             num_output_layer_weights, output_layer_weights);
    }

    nn = nn_new_init(
        nni->nni_num_inputs     , nni->nni_num_hidden_neurons, nni->nni_num_outputs,
        num_hidden_layer_weights, hidden_layer_weights       , nni->nni_hidden_layer_bias,
        num_output_layer_weights, output_layer_weights       , nni->nni_output_layer_bias,
        nni->nni_learning_rate  , nni->nni_momentum);

    switch (nni->nni_method) {
        case NNT_METHOD_OPENING:
            nn->nn_info = Strdup(NN_INFO_OPENING);
            break;
        case NNT_METHOD_HANDREC_SUITED:
            nn->nn_info = Strdup(NN_INFO_HANDREC_SUITED);
            break;
        case NNT_METHOD_HANDREC_NOTRUMP:
            nn->nn_info = Strdup(NN_INFO_HANDREC_NOTRUMP);
            break;
        default:
            break;
    }

    return (nn);
}
/***************************************************************/
int process_nninit(struct omarglobals * og,
        struct nninit * nni,
        struct nndata * nnd)
{
/*
**
*/
    int estat = 0;

    if (nnd->nnd_nn) nn_free(nnd->nnd_nn);

    switch (nni->nni_method) {
        case NNT_METHOD_MM:
            nnd->nnd_nn = nnt_create_mm_nn(og, nni);
            break;
        case NNT_METHOD_OPENING:
        case NNT_METHOD_HANDREC_SUITED:
        case NNT_METHOD_HANDREC_NOTRUMP:
            nnd->nnd_nn = nnt_create_new_nn(og, nni);
            break;
        default:
            break;

    }

    return (estat);
}
/***************************************************************/
int process_nnsave(struct omarglobals * og,
        const char * savfame,
        struct nndata * nnd)
{
/*
**
*/
    int estat = 0;

    if (!nnd || !nnd->nnd_nn) {
        estat = set_error(og, "Neural net not set.");
        return (estat);
    }

    estat = nnt_save(og, nnd->nnd_nn, savfame);

    return (estat);
}
/***************************************************************/
struct nninit * new_nninit(void) {

    struct nninit * nni;

    nni = New(struct nninit, 1);
    nni->nni_method             = 0;
    nni->nni_num_inputs         = 0;
    nni->nni_num_hidden_neurons = 0;
    nni->nni_num_outputs        = 0;
    nni->nni_init               = 0;
    nni->nni_learning_rate      = 0.0;
    nni->nni_momentum           = 0.0;

    return (nni);
}
/***************************************************************/
void free_nninit(struct nninit * nni) {

    Free(nni);
}
/***************************************************************/
struct nndata * new_nndata(void) {

    struct nndata * nnd;

    nnd = New(struct nndata, 1);
    nnd->nnd_nn = NULL;

    return (nnd);
}
/***************************************************************/
void free_nndata(struct nndata * nnd) {

    if (!nnd) return;

    nn_free(nnd->nnd_nn);
    Free(nnd);
}
/***************************************************************/
int get_nn_train_method(const char * method_name) {

    int train_method;

    train_method = 0;

    if (!Stricmp(method_name, NN_INFO_MM)) {
        train_method    = NNT_METHOD_MM;
    } else if (!Stricmp(method_name, NN_INFO_OPENING)) {
        train_method    = NNT_METHOD_OPENING;
    } else if (!Stricmp(method_name, NN_INFO_HANDREC_SUITED)) {
        train_method    = NNT_METHOD_HANDREC_SUITED;
    } else if (!Stricmp(method_name, NN_INFO_HANDREC_NOTRUMP)) {
        train_method    = NNT_METHOD_HANDREC_NOTRUMP;
    }

    return (train_method);
}
/***************************************************************/
int get_nninit(struct omarglobals * og,
        struct nninit * nni,
        int * argix,
        int argc,
        char *argv[])
{
/*
struct nninit {
    char * nni_fname;
    int    nni_method;
};
*/
    int estat = 0;
    int nerr;

    if ((*argix) + 1 > argc) {
        estat = set_error(og, "Insufficient arguments for -nninit.");
    } else {
        if (argv[*argix][0] == '-') {
            estat = set_error(og, "Expecting handtype in -nninit. Found: %s",
                argv[*argix]);
        } else {
            nni->nni_method = get_nn_train_method(argv[*argix]);
            if (nni->nni_method) {
                (*argix)++;
            } else {
                estat = set_error(og, "Bad neural network type in -nninit. Found: %s",
                    argv[*argix]);
            }
        }

        if (!estat &&
            (nni->nni_method == NNT_METHOD_OPENING ||
             nni->nni_method == NNT_METHOD_HANDREC_SUITED ||
             nni->nni_method == NNT_METHOD_HANDREC_NOTRUMP)) {
            if ((*argix) + 5 > argc) {
                estat = set_error(og, "Insufficient arguments for -nninit opening.");
            } else {
                nerr = char_to_integer(argv[*argix], &(nni->nni_num_inputs));
                if (nerr) {
                    estat = set_error(og, "Bad number of inputs in -nninit. Found: %s",
                        argv[*argix]);
                }
                (*argix)++;
            }

            if (!estat) {
                nerr = char_to_integer(argv[*argix], &(nni->nni_num_hidden_neurons));
                if (nerr) {
                    estat = set_error(og, "Bad number of hidden neurons in -nninit. Found: %s",
                        argv[*argix]);
                }
                (*argix)++;
            }

            if (!estat) {
                nerr = char_to_integer(argv[*argix], &(nni->nni_num_outputs));
                if (nerr) {
                    estat = set_error(og, "Bad number of hidden neurons in -nninit. Found: %s",
                        argv[*argix]);
                }
                (*argix)++;
            }

            if (!estat) {
                nerr = string_to_double(argv[*argix], &(nni->nni_learning_rate));
                if (nerr) {
                    estat = set_error(og, "Bad learning rate in -nninit. Found: %s",
                        argv[*argix]);
                }
                (*argix)++;
            }

            if (!estat) {
                nerr = string_to_double(argv[*argix], &(nni->nni_momentum));
                if (nerr) {
                    estat = set_error(og, "Bad momentum in -nninit. Found: %s",
                        argv[*argix]);
                }
                (*argix)++;
            }

            if (!estat) {
                if (!Stricmp(argv[*argix], "RANDOM")) {
                    nni->nni_init = NNI_INIT_RANDOM;
                } else {
                    estat = set_error(og, "Bad neuron initialization in -nninit. Found: %s",
                        argv[*argix]);
                }
                (*argix)++;
            }
                
            nni->nni_hidden_layer_bias = NN_OPENING_DEFAULT_HIDDEN_LAYER_BIAS;
            nni->nni_output_layer_bias = NN_OPENING_DEFAULT_OUTPUT_LAYER_BIAS;
        }
    }

    return (estat);
}
/***************************************************************/
int get_nn_jsontree(
    struct omarglobals * og,
    const char * file_name,
    struct jsontree ** pjt)
{
    int estat = 0;
    int hterr;
    char errmsg[128];

    hterr = jsonp_get_jsontree(file_name, pjt,  errmsg, sizeof(errmsg));
    if (hterr) {
        estat = set_error(og, "Invalid JSON file. Error: %s", errmsg);
    }

    return (estat);
}
/***************************************************************/
int nnt_jsonobject_to_neuron(struct omarglobals * og,
        struct jsonobject * jo,
        struct neuron ** pne)
{
/*
struct neuron {
    int         ne_num_weights;
    ONN_FLOAT * ne_weights;
    ONN_FLOAT * ne_prev_delta;
    ONN_FLOAT   ne_output;
    ONN_FLOAT   ne_bias;
    int         ne_num_inputs;
    ONN_FLOAT * ne_inputs;
};
*/
    int estat = 0;
    int nerr;
    int ii;
    struct neuron * ne;
    struct jsonarray * n_ja;

    (*pne) = NULL;

    ne = ne_new();

    nerr = jsonobject_get_double(jo, "ne_bias", &(ne->ne_bias));
    if (nerr) {
        estat = set_error(og, "Bad %s number in neural network in JSON file.",
            "ne_bias");
    }

    if (!estat) {
        nerr = jsonobject_get_integer(jo, "ne_num_weights", &(ne->ne_num_weights));
        if (nerr) {
            estat = set_error(og, "Bad %s number in neural network in JSON file.",
                "ne_num_weights");
        }
    }

    if (!estat) {
        nerr = jsonobject_get_integer(jo, "ne_num_inputs", &(ne->ne_num_inputs));
        if (nerr) {
            estat = set_error(og, "Bad %s number in neural network in JSON file.",
                "ne_num_inputs");
        }
    }

    /* ne_weights */
    if (!estat && ne->ne_num_weights) {
        n_ja = jsonobject_get_jsonarray(jo, "ne_weights");
        if (!n_ja) {
            estat = set_error(og, "Cannot find %s in neural network in JSON file.",
                "ne_weights");
        } else {
            if (n_ja->ja_nvals != ne->ne_num_weights) {
                estat = set_error(og, "Expecting %d items in %s, but found %d in neural network in JSON file.",
                     ne->ne_num_weights,  "ne_weights",  n_ja->ja_nvals);
            } else {
                ne->ne_weights = New(ONN_FLOAT, ne->ne_num_weights);
                ne->ne_prev_delta = New(ONN_FLOAT, ne->ne_num_weights);
                for (ii = 0; !estat && ii < ne->ne_num_weights; ii++) {
                    ne->ne_prev_delta[ii] = 0.0;
                    nerr = jsonarray_get_double(n_ja, ii, &(ne->ne_weights[ii]));
                    if (nerr) {
                        estat = set_error(og, "Error getting item %d from %s in neural network in JSON file.",
                            ii, "ne_weights");
                    }
                }
            }
        }
    }

    /* ne_inputs */
    if (!estat && ne->ne_num_inputs) {
        n_ja = jsonobject_get_jsonarray(jo, "ne_inputs");
        if (!n_ja) {
            estat = set_error(og, "Cannot find %s in neural network in JSON file.",
                "ne_inputs");
        } else {
            if (n_ja->ja_nvals != ne->ne_num_inputs) {
                estat = set_error(og, "Expecting %d items in %s, but found %d in neural network in JSON file.",
                     ne->ne_num_inputs,  "ne_inputs",  n_ja->ja_nvals);
            } else {
                ne->ne_inputs = New(ONN_FLOAT, ne->ne_num_inputs);
                for (ii = 0; !estat && ii < ne->ne_num_inputs; ii++) {
                    nerr = jsonarray_get_double(n_ja, ii, &(ne->ne_inputs[ii]));
                    if (nerr) {
                        estat = set_error(og, "Error getting item %d from %s in neural network in JSON file.",
                            ii, "ne_inputs");
                    }
                }
            }
        }
    }

    if (estat) {
        ne_free(ne);
    } else {
        (*pne) = ne;
    }

    return (estat);
}
/***************************************************************/
int nnt_jsonobject_to_neural_layer(struct omarglobals * og,
        struct jsonobject * jo,
        struct neural_layer ** pnl)
{
/*
struct neural_layer {
    int         nl_num_neurons;
    ONN_FLOAT   nl_bias;
    struct neuron ** nl_neurons;
};
*/
    int estat = 0;
    int nerr;
    int ii;
    struct neural_layer * nl;
    struct jsonarray * ne_ja;
    struct jsonobject * ne_jo;

    (*pnl) = NULL;

    nl = nl_new();

    nerr = jsonobject_get_double(jo, "nl_bias", &(nl->nl_bias));
    if (nerr) {
        estat = set_error(og, "Bad %s number in neural network in JSON file.",
            "nl_bias");
    }

    nerr = jsonobject_get_integer(jo, "nl_num_neurons", &(nl->nl_num_neurons));
    if (nerr) {
        estat = set_error(og, "Bad %s number in neural network in JSON file.",
            "nl_num_neurons");
    }

    /* nl_neurons */
    if (!estat) {
        ne_ja = jsonobject_get_jsonarray(jo, "nl_neurons");
        if (!ne_ja) {
            estat = set_error(og, "Cannot find %s in neural network in JSON file.",
                "nl_neurons");
        } else {
            if (ne_ja->ja_nvals != nl->nl_num_neurons) {
                estat = set_error(og, "Expecting %d items in %s, but found %d in neural network in JSON file.",
                     nl->nl_num_neurons,  "nl_num_neurons",  ne_ja->ja_nvals);
            } else {
                nl->nl_neurons = New(struct neuron *, nl->nl_num_neurons);
                for (ii = 0; !estat && ii < nl->nl_num_neurons; ii++) {
                    ne_jo = jsonarray_get_jsonobject(ne_ja, ii);
                    if (!ne_jo) {
                        estat = set_error(og, "Error getting item %d from %s in neural network in JSON file.",
                            ii, "nl_neurons");
                    } else {
                        estat = nnt_jsonobject_to_neuron(og, ne_jo, &(nl->nl_neurons[ii]));
                    }
                }
            }
        }
    }

    if (estat) {
        nl_free(nl);
    } else {
        (*pnl) = nl;
    }

    return (estat);
}
/***************************************************************/
int nnt_jsonobject_to_neural_network(struct omarglobals * og,
        struct jsonobject * jo,
        struct neural_network ** pnn)
{
/*
struct neural_network {
    char * nn_info;
    ONN_FLOAT nn_learning_rate;
    int nn_num_inputs;
    int nn_num_outputs;
    int nn_num_epochs;
    ONN_FLOAT nn_total_error;
    ONN_FLOAT   * nn_inputs;
    struct neural_layer * nn_hidden_layer;
    struct neural_layer * nn_output_layer;
};
*/
    int estat = 0;
    int nerr;
    struct neural_network * nn;
    struct jsonobject * nljo;

    (*pnn) = NULL;

    nn = nn_new();

    nn->nn_info = jsonobject_get_string(jo, "nn_info");
    nn->nn_elapsed_time = jsonobject_get_string(jo, "nn_elapsed_time");

    if (!estat) {
        nerr = jsonobject_get_double(jo, "nn_learning_rate", &(nn->nn_learning_rate));
        if (nerr) {
            estat = set_error(og, "Bad %s number in neural network in JSON file.",
                "nn_learning_rate");
        }
    }

    if (!estat) {
        nerr = jsonobject_get_double(jo, "nn_momentum", &(nn->nn_momentum));
        if (nerr) {
            estat = set_error(og, "Bad %s number in neural network in JSON file.",
                "nn_momentum");
        }
    }

    if (!estat) {
        nerr = jsonobject_get_integer(jo, "nn_num_inputs", &(nn->nn_num_inputs));
        if (nerr) {
            estat = set_error(og, "Bad %s number in neural network in JSON file.",
                "nn_num_inputs");
        }
    }

    if (!estat) {
        nerr = jsonobject_get_integer(jo, "nn_num_outputs", &(nn->nn_num_outputs));
        if (nerr) {
            estat = set_error(og, "Bad %s number in neural network in JSON file.",
                "nn_num_outputs");
        }
    }

    if (!estat) {
        nerr = jsonobject_get_double(jo, "nn_total_error", &(nn->nn_total_error));
        if (nerr) {
            estat = set_error(og, "Bad %s number in neural network in JSON file.",
                "nn_total_error");
        }
    }

    if (!estat) {
        nerr = jsonobject_get_integer(jo, "nn_num_epochs", &(nn->nn_num_epochs));
        if (nerr) {
            estat = set_error(og, "Bad %s number in neural network in JSON file.",
                "nn_num_epochs");
        }
    }

    if (!estat) {
        nljo = jsonobject_get_jsonobject(jo, "nn_hidden_layer");
        if (!nljo) {
            estat = set_error(og, "Cannot find %s in neural network in JSON file.",
                "nn_hidden_layer");
        } else {
            estat = nnt_jsonobject_to_neural_layer(og, nljo, &(nn->nn_hidden_layer));
        }
    }

    if (!estat) {
        nljo = jsonobject_get_jsonobject(jo, "nn_output_layer");
        if (!nljo) {
            estat = set_error(og, "Cannot find %s in neural network in JSON file.",
                "nn_output_layer");
        } else {
            estat = nnt_jsonobject_to_neural_layer(og, nljo, &(nn->nn_output_layer));
        }
    }

    if (estat) {
        nn_free(nn);
    } else {
        (*pnn) = nn;
    }

    return (estat);
}
/***************************************************************/
int nnt_load(struct omarglobals * og,
        struct nndata * nnd,
        const char * loadfame)
{
/*
**
*/
    int estat = 0;
    struct jsontree * jt;

    estat = get_nn_jsontree(og, loadfame, &jt);
    if (!estat) {
        if (!jt || !jt->jt_jv || jt->jt_jv->jv_vtyp != VTAB_VAL_OBJECT || !jt->jt_jv->jv_val.jv_jo) {
            estat = set_error(og, "Invalid neural network in JSON file.");
        }
    }

    if (!estat) {
        estat = nnt_jsonobject_to_neural_network(og, jt->jt_jv->jv_val.jv_jo, &(nnd->nnd_nn));
        free_jsontree(jt);
    }

    return (estat);
}
/***************************************************************/
int process_nnload(struct omarglobals * og,
        const char * loadfame,
        struct nndata * nnd)
{
/*
**
*/
    int estat = 0;

    if (nnd->nnd_nn) nn_free(nnd->nnd_nn);

    estat = nnt_load(og, nnd, loadfame);

    return (estat);
}
/***************************************************************/
#ifdef UNUSED
int process_nntest_hands(struct omarglobals * og,
        struct nnhands * nnh,
        struct nntesthand ** nnthlist)
{
    int estat = 0;
    int ii;

    struct neural_network *  nn;

    nn = nnt_create_mm_nn();
    nnt_test(nn);
    estat = nnt_save(og, nn, "CON");

    if (nnthlist) {
        for (ii = 0; nnthlist[ii]; ii++) {
        }
        printf("process_nntest_hands() ii=%d\n", ii);
    }
    nn_free(nn);

    return (estat);
}
#endif
/***************************************************************/
void nnt_train(struct neural_network *  nn,
    int num_training_inputs,
    ONN_FLOAT * training_inputs,
    int num_training_outputs,
    ONN_FLOAT * training_outputs,
    int max_epochs)
{
    int ii;
    ONN_FLOAT total_error;

    for (ii = 0; ii < max_epochs; ii++) {
        nn_train(nn,
            num_training_inputs, training_inputs, num_training_outputs, training_outputs);
        total_error = nn_calculate_total_error(nn,
            num_training_inputs, training_inputs, num_training_outputs, training_outputs);
        nn->nn_total_error = total_error;
        nn->nn_num_epochs += 1;
    }
}
/***************************************************************/
int nnt_train_mm(struct omarglobals * og,
        struct neural_network *  nn,
        int max_epochs)
{
/*
**
*/
    int estat = 0;
    ONN_FLOAT inputs[] = {0.05, 0.1};
    ONN_FLOAT outputs[] = {0.01, 0.99};

    nnt_train(nn, 2, inputs, 2, outputs, max_epochs);

    return (estat);
}
/***************************************************************/
int nnt_verify_nntesthand_2_outputs(struct omarglobals * og,
            struct nntesthand * nnth)
{
/*
**  Outputs:
**      0 - Bid level / 8, 0=pass
**      1 - Bid suit / 8, clubs/pass=0, diamonds=1,hearts=2,spades=3,notrump=4
*/
    int estat = 0;
    int outix;
    int bid_type;
    int bid_level;
    int bid_suit;

    if (nnth->nnth_num_outputs == 2 && nnth->nnth_outputs) {
        return (0);
    }

    nnth->nnth_num_outputs = 2;
    if (nnth->nnth_outputs) Free(nnth->nnth_outputs);
    nnth->nnth_outputs = New(ONN_FLOAT, nnth->nnth_num_outputs);

    outix = 0;

    decode_bid(nnth->nnth_enc_open_bid, &bid_type, &bid_level, &bid_suit);
    if (bid_type != BID) {      /* i.e. bid_type == PASS */
        nnth->nnth_outputs[outix++] = 0.0;
        nnth->nnth_outputs[outix++] = 0.0;
    } else {
        nnth->nnth_outputs[outix++] = (double)bid_level / 8.0;
        nnth->nnth_outputs[outix++] = (double)bid_suit / 8.0;
    }

    return (estat);
}
/***************************************************************/
int nnt_verify_nntesthand_outputs(struct omarglobals * og,
            struct nntesthand ** nnthlist,
                int num_outputs)
{
    int estat = 0;
    int ii;

    if (num_outputs != 2) {
        estat = set_error(og, "Number of outputs must be 2.");
        return (estat);
    }

    for (ii = 0; !estat && nnthlist[ii]; ii++) {
        estat = nnt_verify_nntesthand_2_outputs(og, nnthlist[ii]);
    }

    return (estat);
}
/***************************************************************/
void nnt_setup_hand_8_suit_inputs(
                HAND * hand,
                double * inputs,
                int input_player,
                int input_suit,
                int player_suit,
                int num_highcards)
{
/*
**  Inputs:
**      0 - Number of clubs / 16
**      1 - Number of diamonds / 16
**      2 - Number of hearts / 16
**      3 - Number of spades / 16
**      4 - Number of hex card points in clubs / 16
**      5 - Number of hex card points in diamonds / 16
**      6 - Number of hex card points in hearts / 16
**      7 - Number of hex card points in spades / 16
**      Hex card points: Ace=8, King=4, Queen=2, Jack=1
*/
    int cardix;
    int hexpts;
    int inix;

    inix = (input_player * 8) + input_suit;

    inputs[inix] = (double)(hand->length[player_suit]) / 16.0;

    hexpts = 0;
    for (cardix = 0;
        cardix < hand->length[player_suit] && hand->cards[player_suit][cardix] >= (15 - num_highcards);
        cardix++) {
        hexpts += 1 << (hand->cards[player_suit][cardix] - (15 - num_highcards));
    }
    inputs[inix + 4] = (double)(hexpts) / (double)(1 << num_highcards);
}
/***************************************************************/
void nnt_setup_hand_8_inputs(
                HAND * hand,
                int num_inputs,
                double * inputs,
                int num_highcards)
{
/*
**  Inputs:
**      0 - Number of clubs / 16
**      1 - Number of diamonds / 16
**      2 - Number of hearts / 16
**      3 - Number of spades / 16
**      4 - Number of hex card points in clubs / 16
**      5 - Number of hex card points in diamonds / 16
**      6 - Number of hex card points in hearts / 16
**      7 - Number of hex card points in spades / 16
**      Hex card points: Ace=8, King=4, Queen=2, Jack=1
*/
    int suit;

    for (suit = 3; suit >= 0; suit--) {
        nnt_setup_hand_8_suit_inputs(hand, inputs, 0, suit, suit, num_highcards);
    }
}
/***************************************************************/
int nnt_verify_nntesthand_inputs(struct omarglobals * og,
            struct nntesthand ** nnthlist,
                int num_inputs)
{
    int estat = 0;
    int ii;
    struct nntesthand * nnth;

    if (num_inputs != 8) {
        estat = set_error(og, "Number of inputs must be 8.");
        return (estat);
    }

    for (ii = 0; !estat && nnthlist[ii]; ii++) {
        nnth = nnthlist[ii];
        if (nnth->nnth_num_inputs != 8 || !nnth->nnth_inputs) {
            nnth->nnth_num_inputs = 8;
            if (nnth->nnth_inputs) Free(nnth->nnth_inputs);
            nnth->nnth_inputs = New(ONN_FLOAT, nnth->nnth_num_inputs);

            nnt_setup_hand_8_inputs(&(nnth->nnth_hand), nnth->nnth_num_inputs, nnth->nnth_inputs, NUMBER_OF_HIGH_CARDS);
        }
    }

    return (estat);
}
/***************************************************************/
void nnt_train_nntesthand(
        struct neural_network *  nn,
        struct nntesthand * nnth,
        int max_epochs)
{
/*
**
*/
    nnt_train(nn,
        nnth->nnth_num_inputs, nnth->nnth_inputs,
        nnth->nnth_num_outputs, nnth->nnth_outputs, max_epochs);
}
/***************************************************************/
void nnt_train_nntesthandlist(
        struct neural_network *  nn,
            struct nntesthand ** nnthlist,
        int max_epochs)
{
/*
**
*/
    int ii;

    for (ii = 0; nnthlist[ii]; ii++) {
        nnt_train_nntesthand(nn, nnthlist[ii], 1);
    }
}
/***************************************************************/
int nnt_train_opening(struct omarglobals * og,
        struct neural_network *  nn,
            struct nntesthand ** nnthlist,
        int max_epochs)
{
/*
**
*/
    int estat = 0;
    int ii;

    if (!nnthlist || !nnthlist[0]) {
        estat = set_error(og, "No hands specified with -nnhands.");
        return (estat);
    }

    estat = nnt_verify_nntesthand_inputs(og, nnthlist, nn->nn_num_inputs);

    if (!estat) {
        estat = nnt_verify_nntesthand_outputs(og, nnthlist, nn->nn_num_outputs);
    }

    if (!estat) {
        for (ii = 0; ii < max_epochs; ii++) {
            nnt_train_nntesthandlist(nn, nnthlist, 1);
        }
    }

    return (estat);
}
/***************************************************************/
int count_suit_hcp(HAND * hand, int suit)
{
/*
S Q4
H 8753
D QJ
C Q8732
*/
    int hcp = 0;
    int cardix;

    for (cardix = 0; cardix < hand->length[suit]; cardix++) {
        if (hand->cards[suit][cardix] > 10) {
            hcp += hand->cards[suit][cardix] - 10;
        }
    }

    return (hcp);
}
/***************************************************************/
int count_hand_hcp(HAND * hand)
{
/*
S Q4
H 8753
D QJ
C Q8732
*/
    int hcp = 0;
    int suit;

    for (suit = SPADES; suit >= CLUBS; suit--) {
        hcp += count_suit_hcp(hand, suit);
    }

    return (hcp);
}
/***************************************************************/
void nnt_new_handrec_104_inputs(
            struct handrec * hr,
            int num_inputs,
            double * inputs,
            int declarer,
            int trump_suit)
{
/*
**  Inputs for suit contract:
**       0 -  25 2-bit owner of each card in trump suit
**      26 -  51 2-bit owner of each card in suit 2 (clubs or diamonds)
**      52 -  77 2-bit owner of each card in suit 3 (diamonds or hearts)
**      52 - 103 2-bit owner of each card in suit 4 (hearts or spades)
**
**  Inputs for notrump:
**       0 -  25 2-bit owner of each card in clubs
**      26 -  51 2-bit owner of each card in diamonds
**      52 -  77 2-bit owner of each card in hearts
**      52 - 103 2-bit owner of each card in spades
**
**  Owner
**     [0] [1]
**      0   0   Declarer
**      0   1   Opening leader
**      1   0   Dummy
**      1   0   Defender
**
*/
    int suit;
    int player;
    int playerix;
    int cardix;
    int card;
    int inbase;
    int inix;

    inbase = 0;
    if (trump_suit == CLUBS || trump_suit == NOTRUMP) {
        for (suit = 0; suit < 4; suit++) {
            for (playerix = 0; playerix < 4; playerix++) {
                player = (playerix + declarer) & 3;
                for (cardix = 0; cardix < hr->hr_hands[player].length[suit]; cardix++) {
                    card = hr->hr_hands[player].cards[suit][cardix] - 2;
                    inix = inbase + (card * 2);
                    inputs[inix    ] = (double)(player & 1);
                    inputs[inix + 1] = (double)((player >> 1) & 1);
                }
            }
            inbase += 26;
        }
    } else {
        for (player = 0; player < 4; player++) {
            for (cardix = 0; cardix < hr->hr_hands[player].length[trump_suit]; cardix++) {
                card = hr->hr_hands[player].cards[trump_suit][cardix] - 2;
                inix = inbase + (card * 2);
                inputs[inix    ] = (double)(player & 1);
                inputs[inix + 1] = (double)((player >> 1) & 1);
            }
        }
        inbase += 26;
        for (suit = 0; suit < 4; suit++) {
            if (suit != trump_suit) {
                for (player = 0; player < 4; player++) {
                    for (cardix = 0; cardix < hr->hr_hands[player].length[suit]; cardix++) {
                        card = hr->hr_hands[player].cards[suit][cardix] - 2;
                        inix = inbase + (card * 2);
                        inputs[inix    ] = (double)(player & 1);
                        inputs[inix + 1] = (double)((player >> 1) & 1);
                    }
                }
                inbase += 26;
            }
        }
    }
}
/***************************************************************/
void innt_calc_contract(
            struct handrec * hr,
            int trump_suit)
{
    int declarer;
    int tricks;
    int declarer2;
    int tricks2;
    int player;

    declarer  = NULL_PLAYER;
    declarer2 = NULL_PLAYER;
    for (player = 0; player < 4; player++) {
        hr->hr_hri[player].hri_declarer[trump_suit] = NULL_PLAYER;
        hr->hr_hri[player].hri_tricks_taken[trump_suit] = -1;
    }

    if (trump_suit == NOTRUMP) {
        if (count_hand_hcp(&(hr->hr_hands[0])) + count_hand_hcp(&(hr->hr_hands[2])) >= 20) {
            declarer = hr->hr_makeable.mkbl_contract[0][trump_suit].cont_declarer;
            tricks = hr->hr_makeable.mkbl_contract[0][trump_suit].cont_level + 6;
            declarer2 = hr->hr_makeable.mkbl_contract[2][trump_suit].cont_declarer;
            tricks2 = hr->hr_makeable.mkbl_contract[2][trump_suit].cont_level + 6;
        }
        if (declarer == NULL_PLAYER && declarer2 == NULL_PLAYER) {
            declarer = hr->hr_makeable.mkbl_contract[1][trump_suit].cont_declarer;
            tricks = hr->hr_makeable.mkbl_contract[1][trump_suit].cont_level + 6;
            declarer2 = hr->hr_makeable.mkbl_contract[3][trump_suit].cont_declarer;
            tricks2 = hr->hr_makeable.mkbl_contract[3][trump_suit].cont_level + 6;
        }
    } else {
        if (hr->hr_hands[0].length[trump_suit] + hr->hr_hands[2].length[trump_suit] > 6) {
            declarer = hr->hr_makeable.mkbl_contract[0][trump_suit].cont_declarer;
            tricks = hr->hr_makeable.mkbl_contract[0][trump_suit].cont_level + 6;
            declarer2 = hr->hr_makeable.mkbl_contract[2][trump_suit].cont_declarer;
            tricks2 = hr->hr_makeable.mkbl_contract[2][trump_suit].cont_level + 6;
        } else {
            declarer = hr->hr_makeable.mkbl_contract[1][trump_suit].cont_declarer;
            tricks = hr->hr_makeable.mkbl_contract[1][trump_suit].cont_level + 6;
            declarer2 = hr->hr_makeable.mkbl_contract[3][trump_suit].cont_declarer;
            tricks2 = hr->hr_makeable.mkbl_contract[3][trump_suit].cont_level + 6;
        }
    }

    if (declarer != NULL_PLAYER) {
        hr->hr_hri[declarer].hri_declarer[trump_suit] = declarer;
        hr->hr_hri[declarer].hri_tricks_taken[trump_suit] = tricks;
        hr->hr_hri[declarer].hri_econtract[trump_suit] =
            encode_contract(tricks - 6, trump_suit, BID, declarer);
    }
    if (declarer2 != NULL_PLAYER) {
        hr->hr_hri[declarer2].hri_declarer[trump_suit] = declarer2;
        hr->hr_hri[declarer2].hri_tricks_taken[trump_suit] = tricks2;
        hr->hr_hri[declarer2].hri_econtract[trump_suit] =
            encode_contract(tricks2 - 6, trump_suit, BID, declarer2);
    }
}
/***************************************************************/
int nnt_verify_handrec_104_inputs(struct omarglobals * og,
            struct handrec * hr)
{
/*
*/
    int estat = 0;
    int player;
    int suit;

    for (suit = CLUBS; suit <= NOTRUMP; suit++) {
        innt_calc_contract(hr, suit);
    }

    for (player = 0; player < 4; player++) {
        for (suit = CLUBS; suit <= NOTRUMP; suit++) {
            if (hr->hr_hri[player].hri_num_inputs[suit] != 104 || !hr->hr_hri[player].hri_inputs[suit]) {
                hr->hr_hri[player].hri_num_inputs[suit] = 104;

                if (hr->hr_hri[player].hri_declarer[suit] == player) {
                    if (hr->hr_hri[player].hri_inputs[suit]) Free(hr->hr_hri[player].hri_inputs[suit]);
                    hr->hr_hri[player].hri_inputs[suit] = New(ONN_FLOAT, hr->hr_hri[player].hri_num_inputs[suit]);
                    nnt_new_handrec_104_inputs(hr, hr->hr_hri[player].hri_num_inputs[suit], hr->hr_hri[player].hri_inputs[suit],
                        player, suit);
                }
            }
        }
    }

    return (estat);
}
/***************************************************************/
#ifdef UNUSED
void nnt_new_handrec_32_inputs_ssort_all(
            struct handrec * hr,
            int num_inputs,
            double * inputs,
            int declarer,
            int trump_suit,
            int num_highcards)
{
/*
**  Inverse: nnt_show_bid_opening() & nnt_show_play_handrec()
**
**  Inputs:
**      0 - Declarer number of trump suit / 16
**      1 - Declarer number of suit 2 / 16
**      2 - Declarer number of suit 3 / 16
**      4 - Declarer number of suit 4 / 16
**      4 - Declarer number of hex card points in trump suit / 16
**      5 - Declarer number of hex card points in suit 2 / 16
**      6 - Declarer number of hex card points in suit 3 / 16
**      7 - Declarer number of hex card points in suit 4 / 16
**      8-15 Same as 0-7, but for declarer's LHO
**      16-24 Same as 0-7, but for declarer's partner
**      25-31 Same as 0-7, but for declarer's RHO
**
**      Hex card points (HXP): Ace=8, King=4, Queen=2, Jack=1
**
*/
    int player;
    int playerix;
    int suit;
    int suitno;

    if (trump_suit == CLUBS || trump_suit == NOTRUMP) {
        for (playerix = 0; playerix < 4; playerix++) {
            player = (playerix + declarer) & 3;
            for (suit = 0; suit < 4; suit++) {
                nnt_setup_hand_8_suit_inputs(&(hr->hr_hands[player]), inputs,
                    player, suit, suit, num_highcards);
            }
        }
    } else {
        /* trump suit */
        for (playerix = 0; playerix < 4; playerix++) {
            player = (playerix + declarer) & 3;
            nnt_setup_hand_8_suit_inputs(&(hr->hr_hands[player]), inputs,
                player, 0, trump_suit, num_highcards);
        }
        /* suits 2,3,4 */
        for (playerix = 0; playerix < 4; playerix++) {
            player = (playerix + declarer) & 3;
            suitno = 0;
            for (suit = 0; suit < 4; suit++) {
                if (suit != trump_suit) {
                    suitno++;
                    nnt_setup_hand_8_suit_inputs(&(hr->hr_hands[player]), inputs,
                        player, suitno, suit, num_highcards);
                }
            }
        }
    }
}
#endif
/***************************************************************/
void nnt_new_handrec_32_inputs(
            struct handrec * hr,
            int num_inputs,
            double * inputs,
            int declarer,
            int trump_suit,
            int num_highcards,
            int * suit_order)
{
/*
**
Total error = 1.74323e-005
Epochs      = 14400
Time        = 00:00:03
**
*/
    int player;
    int playerix;
    int suit;

    for (playerix = 0; playerix < 4; playerix++) {
        player = (playerix + declarer) & 3;
        for (suit = 0; suit < 4; suit++) {
            nnt_setup_hand_8_suit_inputs(&(hr->hr_hands[player]), inputs,
                player, suit, suit_order[suit], num_highcards);
        }
    }
}
/***************************************************************/
int nnt_calc_suit_val(HAND * hand, int suit)
{
    int sval;

    sval = 100 * hand->length[suit] + count_suit_hcp(hand, suit);

    return (sval);
}
/***************************************************************/
void nnt_calc_suit_order_32(
            struct handrec * hr,
            int num_inputs,
            double * inputs,
            int declarer,
            int trump_suit,
            int num_highcards,
            int suit_sorting,
            int * suit_order)
{
/*
**
Total error = 1.74323e-005
Epochs      = 14400
Time        = 00:00:03
**
*/
    int suit;
    int suitno;
    int suit_val[4];

    switch (suit_sorting) {
        case SSORT_ALL:
            /* trump suit is suit_order[0] */
            if (trump_suit == CLUBS || trump_suit == NOTRUMP) {
                for (suit = 0; suit < 4; suit++) {
                    suit_order[suit] = suit;
                }
            } else {
                suit_order[0] = trump_suit;
                suitno = 0;
                for (suit = 0; suit < 4; suit++) {
                    if (suit != trump_suit) {
                        suitno++;
                        suit_order[suitno] = suit;
                    }
                }
            }
            break;
        case SSORT_LONGEST:
            /* trump suit is suit_order[3] */
            for (suit = 0; suit < 4; suit++) {
                suit_order[suit] = suit;
                suit_val[suit]  = nnt_calc_suit_val(&(hr->hr_hands[declarer]), suit);
                suit_val[suit] += nnt_calc_suit_val(&(hr->hr_hands[PARTNER(declarer)]), suit);
                if (suit == trump_suit) suit_val[suit] += 10000;
            }
            sort_int4(suit_val, suit_order);
            break;
        default:
            break;
    }
}
/***************************************************************/
int nnt_verify_handrec_32_inputs(struct omarglobals * og,
            struct handrec * hr,
            int suit_sorting)
{
/*

                    switch (suit_sorting) {
                        case SSORT_ALL:
                            nnt_new_handrec_32_inputs_ssort_all(hr, hr->hr_hri[player].hri_num_inputs[suit],
                                hr->hr_hri[player].hri_inputs[suit],
                                player, suit, NUMBER_OF_HIGH_CARDS);
                            break;
                        case SSORT_LONGEST:
                            nnt_new_handrec_32_inputs(hr, hr->hr_hri[player].hri_num_inputs[suit],
                                hr->hr_hri[player].hri_inputs[suit],
                                player, suit, NUMBER_OF_HIGH_CARDS);
                            break;
                        default:
                            break;
                    }


*/
    int estat = 0;
    int player;
    int suit;
    int suit_order[4];

    for (suit = CLUBS; suit <= NOTRUMP; suit++) {
        innt_calc_contract(hr, suit);
    }

    for (player = 0; player < 4; player++) {
        for (suit = CLUBS; suit <= NOTRUMP; suit++) {
            if (hr->hr_hri[player].hri_num_inputs[suit] != 32 || !hr->hr_hri[player].hri_inputs[suit]) {
                hr->hr_hri[player].hri_num_inputs[suit] = 32;
                if (hr->hr_hri[player].hri_declarer[suit] == player) {
                    if (hr->hr_hri[player].hri_inputs[suit]) Free(hr->hr_hri[player].hri_inputs[suit]);
                    hr->hr_hri[player].hri_inputs[suit] = New(ONN_FLOAT, hr->hr_hri[player].hri_num_inputs[suit]);

                    nnt_calc_suit_order_32(hr, hr->hr_hri[player].hri_num_inputs[suit],
                        hr->hr_hri[player].hri_inputs[suit],
                        player, suit, NUMBER_OF_HIGH_CARDS, suit_sorting, suit_order);

                    nnt_new_handrec_32_inputs(hr, hr->hr_hri[player].hri_num_inputs[suit],
                        hr->hr_hri[player].hri_inputs[suit],
                        player, suit, NUMBER_OF_HIGH_CARDS, suit_order);
                }
            }
        }
    }

    return (estat);
}
/***************************************************************/
int nnt_verify_handrec_inputs(struct omarglobals * og,
                    struct handrec ** ahandrec,
                    int suit_sorting,
                    int num_inputs)
{
    int estat = 0;
    int ii;

    if (num_inputs != 32 && num_inputs != 104) {
        estat = set_error(og, "Number of inputs must be 32 or 104.");
        return (estat);
    }

    if (num_inputs == 104) {
        for (ii = 0; !estat && ahandrec[ii]; ii++) {
            estat = nnt_verify_handrec_104_inputs(og, ahandrec[ii]);
        }
    } else {
        for (ii = 0; !estat && ahandrec[ii]; ii++) {
            estat = nnt_verify_handrec_32_inputs(og, ahandrec[ii], suit_sorting);
        }
    }

    return (estat);
}
/***************************************************************/
void nnt_new_handrec_1_outputs(
            struct handrec * hr,
            int num_outputs,
            double * outputs,
            int declarer,
            int trump_suit)
{
/*
**  Outputs:
**      0 - (number of tricks) / 16
*/
    outputs[0] = (double)(hr->hr_hri[declarer].hri_tricks_taken[trump_suit]) / 16.0;
}
/***************************************************************/
int nnt_verify_handrec_1_outputs(struct omarglobals * og,
            struct handrec * hr)
{
/*
**  Outputs:
**      0 - Bid level / 8, 0=pass
**      1 - Bid suit / 8, clubs/pass=0, diamonds=1,hearts=2,spades=3,notrump=4
*/
    int estat = 0;
    int player;
    int suit;

    for (player = 0; player < 4; player++) {
        for (suit = CLUBS; suit <= NOTRUMP; suit++) {
            if (hr->hr_hri[player].hri_num_outputs[suit] != 1 || !hr->hr_hri[player].hri_outputs[suit]) {
                if (hr->hr_hri[player].hri_declarer[suit] == player) {
                    hr->hr_hri[player].hri_num_outputs[suit] = 1;
                    if (hr->hr_hri[player].hri_outputs[suit]) Free(hr->hr_hri[player].hri_outputs[suit]);
                    hr->hr_hri[player].hri_outputs[suit] = New(ONN_FLOAT, hr->hr_hri[player].hri_num_outputs[suit]);
                    nnt_new_handrec_1_outputs(hr, hr->hr_hri[player].hri_num_outputs[suit], hr->hr_hri[player].hri_outputs[suit], 
                        player, suit);
                }
            }
        }
    }

    return (estat);
}
/***************************************************************/
int nnt_verify_handrec_outputs(struct omarglobals * og,
                struct handrec ** ahandrec,
                int suit_sorting,
                int num_outputs)
{
    int estat = 0;
    int ii;

    if (num_outputs != 1) {
        estat = set_error(og, "Number of outputs must be 1.");
        return (estat);
    }

    for (ii = 0; !estat && ahandrec[ii]; ii++) {
        estat = nnt_verify_handrec_1_outputs(og, ahandrec[ii]);
    }

    return (estat);
}
/***************************************************************/
void nnt_train_play(struct neural_network *  nn,
    int num_training_inputs,
    ONN_FLOAT * training_inputs,
    int num_training_outputs,
    ONN_FLOAT * training_outputs,
    int max_epochs)
{
    nnt_train(nn,
        num_training_inputs, training_inputs,
        num_training_outputs, training_outputs, max_epochs);
}
/***************************************************************/
void nnt_arrange_inputs(
    int num_training_inputs,
    ONN_FLOAT * training_inputs_in,
    ONN_FLOAT * training_inputs_out,
    int outorder)
{
    int player;
    int suit;
    int srcix;
    int tgtix;
    int outxref[24][4] = {
        { 0, 1, 2, 3 }, { 0, 1, 3, 2 }, { 0, 2, 1, 3 }, { 0, 2, 3, 1 }, { 0, 3, 1, 2 }, { 0, 3, 2, 1 },
        { 1, 0, 2, 3 }, { 1, 0, 3, 2 }, { 1, 2, 0, 3 }, { 1, 2, 3, 0 }, { 1, 3, 0, 2 }, { 1, 3, 2, 0 },
        { 2, 1, 0, 3 }, { 2, 1, 3, 0 }, { 2, 0, 1, 3 }, { 2, 0, 3, 1 }, { 2, 3, 1, 0 }, { 2, 3, 0, 1 },
        { 3, 1, 2, 0 }, { 3, 1, 0, 2 }, { 3, 2, 1, 0 }, { 3, 2, 0, 1 }, { 3, 0, 1, 2 }, { 3, 0, 2, 1 }
    };

    for (player = 0; player < 4; player++ ) {
        for (suit = 0; suit < 4; suit++ ) {
            srcix = player * 8 + suit;
            tgtix = player * 8 + outxref[outorder][suit];
            training_inputs_out[tgtix] = training_inputs_in[srcix];
            training_inputs_out[tgtix + 4] = training_inputs_in[srcix + 4];
        }
    }
}
/***************************************************************/
void nnt_train_handrec_for_suit_ssort_all(struct omarglobals * og,
        struct neural_network *  nn,
        struct handrec * hr,
        int declarer,
        int trump_suit,
        int max_epochs)
{
/*
**
    nnt_train_play(nn,
        hr->hr_num_inputs[trump_suit], hr->hr_inputs[trump_suit],
        hr->hr_num_outputs[trump_suit], hr->hr_outputs[trump_suit], max_epochs);
*/
    int num_training_inputs;
    int ii;
    ONN_FLOAT training_inputs[32];

    num_training_inputs = hr->hr_hri[declarer].hri_num_inputs[trump_suit];

    for (ii = 0; ii < 6; ii++) {
        nnt_arrange_inputs(num_training_inputs, hr->hr_hri[declarer].hri_inputs[trump_suit],
            training_inputs, ii);
        if (og->og_flags & OG_FLAG_DEBUG) {
            nnt_show_play_handrec(og, trump_suit, hr->hr_hri[declarer].hri_econtract[trump_suit],
                hr->hr_hri[declarer].hri_num_inputs[trump_suit], training_inputs);
        }
        nnt_train_play(nn,
            num_training_inputs, training_inputs,
            hr->hr_hri[declarer].hri_num_outputs[trump_suit], hr->hr_hri[declarer].hri_outputs[trump_suit], max_epochs);
    }
}
/***************************************************************/
void nnt_train_handrec_for_suit_ssort_longest(struct omarglobals * og,
        struct neural_network *  nn,
        struct handrec * hr,
        int declarer,
        int trump_suit,
        int max_epochs)
{
/*
**
*/
    if (og->og_flags & OG_FLAG_DEBUG) {
        nnt_show_play_handrec(og, trump_suit, hr->hr_hri[declarer].hri_econtract[trump_suit],
            hr->hr_hri[declarer].hri_num_inputs[trump_suit], hr->hr_hri[declarer].hri_inputs[trump_suit]);
    }
    nnt_train_play(nn,
        hr->hr_hri[declarer].hri_num_inputs[trump_suit] , hr->hr_hri[declarer].hri_inputs[trump_suit],
        hr->hr_hri[declarer].hri_num_outputs[trump_suit], hr->hr_hri[declarer].hri_outputs[trump_suit], max_epochs);
}
/***************************************************************/
void nnt_train_handrec_for_each_suit(struct omarglobals * og,
        struct neural_network *  nn,
        struct handrec ** ahandrec,
        int suit_sorting,
        int max_epochs)
{
/*
**
*/
    int player;
    int suit;
    int ii;

    switch (suit_sorting) {
        case SSORT_ALL:
            for (ii = 0; ahandrec[ii]; ii++) {
                for (player = 0; player < 4; player++) {
                    for (suit = CLUBS; suit <= SPADES; suit++) {
                        if (ahandrec[ii]->hr_hri[player].hri_declarer[suit] == player) {
                            nnt_train_handrec_for_suit_ssort_all(og, nn, ahandrec[ii], player, suit, max_epochs);
                        }
                    }
                }
            }
            break;
        case SSORT_LONGEST:
            for (ii = 0; ahandrec[ii]; ii++) {
                for (player = 0; player < 4; player++) {
                    for (suit = CLUBS; suit <= SPADES; suit++) {
                        if (ahandrec[ii]->hr_hri[player].hri_declarer[suit] == player) {
                            nnt_train_handrec_for_suit_ssort_longest(og, nn, ahandrec[ii], player, suit, max_epochs);
                        }
                    }
                }
            }
            break;
        default:
            break;
    }
}
/***************************************************************/
int nnt_verify_handrec_inputs_outputs(struct omarglobals * og,
        struct neural_network *  nn,
        struct handrec ** ahandrec,
        int suit_sorting)
{
/*
**
*/
    int estat = 0;

    if (!ahandrec || !ahandrec[0]) {
        estat = set_error(og, "No hands specified with -handrec.");
        return (estat);
    }

    if (!estat) {
        estat = nnt_verify_handrec_inputs(og, ahandrec, suit_sorting, nn->nn_num_inputs);
    }

    if (!estat) {
        estat = nnt_verify_handrec_outputs(og, ahandrec, suit_sorting, nn->nn_num_outputs);
    }

    return (estat);
}
/***************************************************************/
int nnt_train_handrec_suited(struct omarglobals * og,
        struct neural_network *  nn,
        struct handrec ** ahandrec,
        int suit_sorting,
        int max_epochs)
{
/*
**
*/
    int estat = 0;
    int ii;

    estat = nnt_verify_handrec_inputs_outputs(og, nn, ahandrec, suit_sorting);

    if (!estat) {
        for (ii = 0; ii < max_epochs; ii++) {
            nnt_train_handrec_for_each_suit(og, nn, ahandrec, suit_sorting, 1);
        }
    }

    return (estat);
}
/***************************************************************/
void nnt_train_handrec_for_notrump_ssuit_all(struct omarglobals * og,
        struct neural_network *  nn,
        struct handrec * hr,
        int max_epochs)
{
/*
**
    nnt_train_play(nn,
        hr->hr_num_inputs[trump_suit], hr->hr_inputs[trump_suit],
        hr->hr_num_outputs[trump_suit], hr->hr_outputs[trump_suit], max_epochs);
*/
    int num_training_inputs;
    int player;
    int ii;
    ONN_FLOAT training_inputs[32];

    for (player = 0; player < 4; player++) {
        if (hr->hr_hri[player].hri_declarer[NOTRUMP] == player) {
            num_training_inputs = hr->hr_hri[player].hri_num_inputs[NOTRUMP];

            for (ii = 0; ii < 24; ii++) {
                nnt_arrange_inputs(num_training_inputs, hr->hr_hri[player].hri_inputs[NOTRUMP],
                    training_inputs, ii);
                if (og->og_flags & OG_FLAG_DEBUG) {
                    nnt_show_play_handrec(og, NOTRUMP, hr->hr_hri[player].hri_econtract[NOTRUMP],
                        hr->hr_hri[player].hri_num_inputs[NOTRUMP], training_inputs);
                }
                nnt_train_play(nn,
                    num_training_inputs, training_inputs,
                    hr->hr_hri[player].hri_num_outputs[NOTRUMP], hr->hr_hri[player].hri_outputs[NOTRUMP], max_epochs);
            }
        }
    }
}
/***************************************************************/
void nnt_train_handrec_for_notrump_ssuit_longest(struct omarglobals * og,
        struct neural_network *  nn,
        struct handrec * hr,
        int max_epochs)
{
/*
**
    nnt_train_play(nn,
        hr->hr_num_inputs[trump_suit], hr->hr_inputs[trump_suit],
        hr->hr_num_outputs[trump_suit], hr->hr_outputs[trump_suit], max_epochs);
*/
    int player;

    for (player = 0; player < 4; player++) {
        if (hr->hr_hri[player].hri_declarer[NOTRUMP] == player) {
            if (og->og_flags & OG_FLAG_DEBUG) {
                nnt_show_play_handrec(og, NOTRUMP, hr->hr_hri[player].hri_econtract[NOTRUMP],
                    hr->hr_hri[player].hri_num_inputs[NOTRUMP], hr->hr_hri[player].hri_inputs[NOTRUMP]);
            }
            nnt_train_play(nn,
                hr->hr_hri[player].hri_num_inputs[NOTRUMP] , hr->hr_hri[player].hri_inputs[NOTRUMP],
                hr->hr_hri[player].hri_num_outputs[NOTRUMP], hr->hr_hri[player].hri_outputs[NOTRUMP], max_epochs);
        }
    }
}
/***************************************************************/
int nnt_train_handrec_notrump(struct omarglobals * og,
        struct neural_network *  nn,
        struct handrec ** ahandrec,
        int suit_sorting,
        int max_epochs)
{
/*
**
*/
    int estat = 0;
    int ii;
    int jj;

    estat = nnt_verify_handrec_inputs_outputs(og, nn, ahandrec, suit_sorting);
    if (estat) return (estat);

    switch (suit_sorting) {
        case SSORT_ALL:
            for (ii = 0; ii < max_epochs; ii++) {
                for (jj = 0; ahandrec[jj]; jj++) {
                    nnt_train_handrec_for_notrump_ssuit_all(og, nn, ahandrec[jj], 1);
                }
            }
            break;
        case SSORT_LONGEST:
            for (ii = 0; ii < max_epochs; ii++) {
                for (jj = 0; ahandrec[jj]; jj++) {
                    nnt_train_handrec_for_notrump_ssuit_longest(og, nn, ahandrec[jj], 1);
                }
            }
            break;
        default:
            break;
    }

    return (estat);
}
/***************************************************************/
int nnt_train_handrec(struct omarglobals * og,
        struct neural_network *  nn,
        struct handrec ** ahandrec,
        int suit_sorting,
        int train_method,
        int max_epochs)
{
/*
**
*/
    int estat = 0;
    struct proctimes pt_start;
    struct proctimes pt_stop;
    struct proctimes pt_result;
    char tsfmt[40];
    char timbuf[40];

    start_cpu_timer(&pt_start);

    switch (train_method) {
        case NNT_METHOD_MM:
            estat = nnt_train_mm(og, nn, max_epochs);
            break;
        case NNT_METHOD_OPENING:
            estat = nnt_train_opening(og, nn, og->og_nnthlist, max_epochs);
            break;
        case NNT_METHOD_HANDREC_SUITED:
            estat = nnt_train_handrec_suited(og, nn, ahandrec, suit_sorting, max_epochs);
            break;
        case NNT_METHOD_HANDREC_NOTRUMP:
            estat = nnt_train_handrec_notrump(og, nn, ahandrec, suit_sorting, max_epochs);
            break;
        default:
            break;

    }
    stop_cpu_timer(&pt_start, &pt_stop, &pt_result);
    strcpy(tsfmt, "%nD %H:%M:%S");
    format_timestamp(&(pt_result.pt_wall_time), tsfmt, timbuf, sizeof(timbuf));
    nn_set_elapsed_time(nn, timbuf, tsfmt);

    return (estat);
}
/***************************************************************/
int process_nntrain(struct omarglobals * og,
        struct nntrain * nnt,
        struct nndata * nnd)
{
/*
**
*/
    int estat = 0;
    struct proctimes pt_start;
    struct proctimes pt_stop;
    struct proctimes pt_result;
    char tsfmt[40];
    char timbuf[40];

    if (!nnd || !nnd->nnd_nn) {
        estat = set_error(og, "No neural net for -nntrain.");
        return (estat);
    }
    start_cpu_timer(&pt_start);

    switch (nnt->nnt_method) {
        case NNT_METHOD_MM:
            estat = nnt_train_mm(og, nnd->nnd_nn, nnt->nnt_max_epochs);
            break;
        case NNT_METHOD_OPENING:
            estat = nnt_train_opening(og, nnd->nnd_nn, og->og_nnthlist, nnt->nnt_max_epochs);
            break;
        case NNT_METHOD_HANDREC_SUITED:
            estat = nnt_train_handrec_suited(og, nnd->nnd_nn,
                og->og_ahandrec, SSORT_ALL, nnt->nnt_max_epochs);
            break;
        case NNT_METHOD_HANDREC_NOTRUMP:
                estat = nnt_train_handrec_notrump(og, nnd->nnd_nn,
                    og->og_ahandrec, SSORT_ALL, nnt->nnt_max_epochs);
            break;
        default:
            break;

    }
    stop_cpu_timer(&pt_start, &pt_stop, &pt_result);
    strcpy(tsfmt, "%nD %H:%M:%S");
    format_timestamp(&(pt_result.pt_wall_time), tsfmt, timbuf, sizeof(timbuf));

    if (!estat) {
        nn_set_elapsed_time(nnd->nnd_nn, timbuf, tsfmt);
        omarout(og, "Elapsed=%s, epochs=%d, Training error = %20.16f\n",
            timbuf, nnd->nnd_nn->nn_num_epochs, nnd->nnd_nn->nn_total_error);
    }

    return (estat);
}
/***************************************************************/
struct nntrain * new_nntrain(void) {

    struct nntrain * nnt;

    nnt = New(struct nntrain, 1);
    nnt->nnt_method     = 0;
    nnt->nnt_max_epochs = 0;

    return (nnt);
}
/***************************************************************/
void free_nntrain(struct nntrain * nnt) {

    Free(nnt);
}
/***************************************************************/
int get_nntrain(struct omarglobals * og,
        struct nntrain * nnt,
        int * argix,
        int argc,
        char *argv[])
{
/*
struct nntrain {
    int nnt_method;
};
*/
    int estat = 0;
    int nerr;

    if ((*argix) + 2 > argc) {
        estat = set_error(og, "Insufficient arguments for -nntrain.");
    } else {
        if (argv[*argix][0] == '-') {
            estat = set_error(og, "Expecting handtype in -nntrain. Found: %s",
                argv[*argix]);
        } else {
            nnt->nnt_method = get_nn_train_method(argv[*argix]);
            if (nnt->nnt_method) {
                (*argix)++;
            } else {
                estat = set_error(og, "Bad neural network type in -nntrain. Found: %s",
                    argv[*argix]);
            }
        }
    }

    if (!estat) {
        if (argv[*argix][0] == '-') {
            estat = set_error(og, "Expecting number of epochs in -nntrain. Found: %s",
                argv[*argix]);
        } else {
            nerr = char_to_integer(argv[*argix], &(nnt->nnt_max_epochs));
            if (nerr) {
                estat = set_error(og, "Bad number of epochs in -nntrain. Found: %s",
                    argv[*argix]);
            }
            (*argix)++;
        }
    }

    if (!estat && 
        (nnt->nnt_method == NNT_METHOD_HANDREC_SUITED || nnt->nnt_method == NNT_METHOD_HANDREC_NOTRUMP)) {
        if ((*argix) + 1 > argc) {
            estat = set_error(og, "Insufficient arguments for -nntrain handrec.");
        } else {
            if (!Stricmp(argv[*argix], "SUITED")) {
                nnt->nnt_method = NNT_METHOD_HANDREC_SUITED;
            } else if (!Stricmp(argv[*argix], "NOTRUMP")) {
                nnt->nnt_method = NNT_METHOD_HANDREC_NOTRUMP;
            } else {
                estat = set_error(og, "Bad training type in -nninit. Found: %s",
                    argv[*argix]);
            }
            (*argix)++;
        }
    }

    return (estat);
}
/***************************************************************/
/*@*/void decode_floating2_bid(ONN_FLOAT fbid0,
                     ONN_FLOAT fbid1,
                     int *bid_type,
                     int *bid_level,
                     int *bid_suit) {
/*
**  Outputs:
**      fbid0 - Bid level / 8, 0=pass
**      fbid1 - Bid suit / 8, clubs/pass=0, diamonds=1,hearts=2,spades=3,notrump=4
*/

    *bid_level = (int)floor(8.0 * fbid0 + 0.25);
    *bid_suit = (int)floor(8.0 * fbid1 + 0.5);

    if (*bid_level < 0) *bid_level = 0;
    else if (*bid_level > 7) *bid_level = 7;

    if (*bid_suit < 0) *bid_suit = 0;
    else if (*bid_suit > 4) *bid_suit = 4;

    if (*bid_level == 0) {
        *bid_type = PASS;
        *bid_suit = 0;
    } else {
        *bid_type = BID;
    }
}
/***************************************************************/
int nnt_get_opening_output(
        struct neural_network *  nn,
        struct nntesthand * nnth,
        int num_outputs,
        ONN_FLOAT * outputs)
{
/*
**
*/
    int nout;

    nout = nn_forward(nn, nnth->nnth_num_inputs, nnth->nnth_inputs,
        num_outputs, outputs);

    return (nout);
}
/***************************************************************/
int nnt_bid_nntesthand(struct omarglobals * og,
        struct nntesthand * nnth,
            int num_outputs,
            ONN_FLOAT * outputs)
{
/*
**
*/
    int is_correct;
    int result_bid_type;
    int result_bid_level;
    int result_bid_suit;
    int expect_bid_type;
    int expect_bid_level;
    int expect_bid_suit;

    is_correct = 0;

    decode_floating2_bid(outputs[0], outputs[1],
        &result_bid_type, &result_bid_level, &result_bid_suit);

    decode_bid(nnth->nnth_enc_open_bid,
        &expect_bid_type, &expect_bid_level, &expect_bid_suit);

    /* print_bid(og, nnth->nnth_enc_open_bid); */

    if ((result_bid_type  == expect_bid_type) &&
        (result_bid_level == result_bid_level) &&
        (result_bid_suit  == expect_bid_suit)) {
        is_correct = 1;
    }

    return (is_correct);
}
/***************************************************************/
void nnt_show_bid_opening(struct omarglobals * og,
        struct nntesthand * nnth,
        int num_inputs,
        ONN_FLOAT * inputs,
        int num_outputs,
        ONN_FLOAT * outputs)
{
    int suit;
    int cardix;
    int inix;
    int hxp;
    HAND hand;

    memset(&hand, 0, sizeof(HAND));

    inix = 0;
    for (suit = 0; suit < 4; suit++) {
        hand.length[suit] = (int)(inputs[inix] * 16.0);
        inix++;
    }
    for (suit = 0; suit < 4; suit++) {
        cardix = 0;
        hxp = (int)(inputs[inix] * 16.0);
        inix++;
        if (hxp & 8) hand.cards[suit][cardix++] = 14;
        if (hxp & 4) hand.cards[suit][cardix++] = 13;
        if (hxp & 2) hand.cards[suit][cardix++] = 12;
        if (hxp & 1) hand.cards[suit][cardix++] = 11;
        while (cardix < hand.length[suit]) {
            hand.cards[suit][cardix++] = 1;
        }
    }

    omarout(og, "======\n");
    print_hand(og, &hand);
    omarout(og, "\n");
    omarout(og, "======\n");
}
/***************************************************************/
int nnt_bid_opening(struct omarglobals * og,
        struct neural_network *  nn,
            struct nntesthand ** nnthlist)
{
/*
**
*/
    int estat = 0;
    int ii;
    int is_correct;
    int nhands;
    int ncorrect;
    int nout;
    struct nntesthand * nnth;
    ONN_FLOAT outputs[2];

    if (!nnthlist || !nnthlist[0]) {
        estat = set_error(og, "No hands specified with -nnhands.");
        return (estat);
    }

    estat = nnt_verify_nntesthand_inputs(og, nnthlist, nn->nn_num_inputs);

    if (!estat) {
        estat = nnt_verify_nntesthand_outputs(og, nnthlist, nn->nn_num_outputs);
    }

    ncorrect = 0;
    nhands = 0;

    if (!estat) {
        for (ii = 0; nnthlist[ii]; ii++) {
            nnth = nnthlist[ii];
            nout = nnt_get_opening_output(nn, nnth, 2, outputs);
            if (nout == 2) {
                is_correct = nnt_bid_nntesthand(og, nnth, nout, outputs);
                nhands++;
                if (is_correct) ncorrect++;
                if (og->og_flags & OG_FLAG_VERBOSE) {
                    nnt_show_bid_opening(og, nnthlist[ii],
                        nnth->nnth_num_inputs, nnth->nnth_inputs, nout, outputs);
                }
            }
        }
    }

    omarout(og, "Bidding got %d correct out of %d, for %5.2f%%\n",
        ncorrect, nhands, 100.0 * (double)ncorrect / (double)nhands);

    return (estat);
}
/***************************************************************/
int process_nnbid(struct omarglobals * og,
        struct nnbid * nnb,
        struct nndata * nnd)
{
/*
**
*/
    int estat = 0;

    estat = nnt_bid_opening(og, nnd->nnd_nn, og->og_nnthlist);

    return (estat);
}
/***************************************************************/
struct nnbid * new_nnbid(void) {

    struct nnbid * nnb;

    nnb = New(struct nnbid, 1);
    nnb->nnb_method     = 0;

    return (nnb);
}
/***************************************************************/
void free_nnbid(struct nnbid * nnb) {

    Free(nnb);
}
/***************************************************************/
int get_nnbid(struct omarglobals * og,
        struct nnbid * nnb,
        int * argix,
        int argc,
        char *argv[])
{
/*
struct nnbid {
    int nnb_method;
};
*/
    int estat = 0;

    return (estat);
}
/***************************************************************/
int nnt_get_play_output(
        struct neural_network *  nn,
        int num_inputs,
        ONN_FLOAT * inputs,
        int num_outputs,
        ONN_FLOAT * outputs)
{
/*
**
*/
    int nout;

    nout = nn_forward(nn, num_inputs, inputs,
        num_outputs, outputs);

    return (nout);
}
/***************************************************************/
#if 0
int nnt_check_play_output_1(
        ONN_FLOAT output,
        ONN_FLOAT exp_output)
{
/*
**
*/
    int is_correct;

    is_correct = 0;
    if (fabs(output * 16.0 - exp_output * 16.0) < 0.5) is_correct = 1;

    return (is_correct);
}
#endif
/***************************************************************/
int nnt_check_play_tricks_diff(
        ONN_FLOAT output,
        ONN_FLOAT exp_output)
{
/*
**
*/
    int tricks_diff;
    double tdiff;

    tdiff = exp_output * 16.0 - output * 16.0;

    tricks_diff = (int)(floor(tdiff + 0.5));

    return (tricks_diff);
}
/***************************************************************/
void nnt_show_play_handrec(struct omarglobals * og,
        int trump_suit,
        int econtract,
        int num_inputs,
        ONN_FLOAT * inputs)
{
/*
**  Inverse: nnt_new_handrec_32_inputs()
*/
    int playerix;
    int player;
    int suit;
    int cardix;
    int cardval;
    int inix;
    ONN_FLOAT hxp;
    CONTRACT contract;
    HANDS hands;
    int num_highcards;

    num_highcards = NUMBER_OF_HIGH_CARDS;

    memset(&contract, 0, sizeof(CONTRACT));
    memset(&hands, 0, sizeof(HANDS));

    if (econtract) {
        decode_contract(econtract,
            &(contract.cont_level),
            &(contract.cont_suit),
            &(contract.cont_doubled),
            &(contract.cont_declarer));
    }
/*
    inix = 0;
    for (playerix = 0; playerix < 4; playerix++) {
        for (suit = 0; suit < 4; suit++) {
            printf("Player: %d, suit: %d, len=%d, hxd=%d\n",
                playerix, suit,
                (int)(inputs[inix + suit] * 16.0),
                (int)(inputs[inix + suit + 4] * 16.0));
        }
        printf("\n");
        inix += 8;
    }
*/

    inix = 0;
    for (playerix = 0; playerix < 4; playerix++) {
        player = (playerix + contract.cont_declarer) & 3;
        for (suit = 3; suit >= 0; suit--) {
            hands[player].length[suit] = (int)(inputs[inix] * 16.0);
            inix++;
        }
        for (suit = 3; suit >= 0; suit--) {
            cardix = 0;
            cardval = 14;
            hxp = inputs[inix++];
            while (hxp > 0.0 && cardval >= (15 - num_highcards)) {
                hxp *= 2.0;
                if (hxp >= 1.0) {
                    hands[player].cards[suit][cardix++] = cardval;
                    hxp -= 1.0;
                }
                cardval--;
            }
            while (cardix < hands[player].length[suit]) {
                hands[player].cards[suit][cardix++] = 1;
            }
        }
    }

    print_bid_suit(og, "====== %s\n", trump_suit);
    print_hands(og, &hands);
    omarout(og, "\n");
    if (econtract) {
        omarout(og, "    Contract: ");
        print_contract(og, &contract);
    }
    omarout(og, "\n");
    omarout(og, "======\n");
}
/***************************************************************/
void init_playstats(struct playstats * pst)
{
    pst->pst_nhands     = 0;
    pst->pst_ncorrect   = 0;
    pst->pst_nplus1     = 0;
    pst->pst_nminus1     = 0;
}
/***************************************************************/
void show_playstats(struct omarglobals * og, struct playstats * pst)
{
    omarout(og, "Play got %d close out of %d, for %5.2f%%\n",
        pst->pst_nplus1 + pst->pst_nminus1, pst->pst_nhands,
        100.0 * (double)(pst->pst_nplus1 + pst->pst_nminus1) / (double)pst->pst_nhands);
    omarout(og, "Play got %d correct out of %d, for %5.2f%%\n",
        pst->pst_ncorrect, pst->pst_nhands, 100.0 * (double)pst->pst_ncorrect / (double)pst->pst_nhands);
}
/***************************************************************/
void nnt_play_suited_handrec_ssort_all(struct omarglobals * og,
        struct neural_network *  nn,
        struct handrec * hr,
        struct playstats * pst)
{
    int player;
    int suit;
    double output;
    int nout;
    int tricks_diff;
    int ii;
    ONN_FLOAT training_inputs[32];

    for (player = 0; player < 4; player++) {
        for (suit = CLUBS; suit <= SPADES; suit++) {
            if (hr->hr_hri[player].hri_declarer[suit] == player) {
                if (hr->hr_hri[player].hri_num_inputs[suit] == 32 && hr->hr_hri[player].hri_inputs[suit]) {
                    for (ii = 0; ii < 6; ii++) {
                        nnt_arrange_inputs(hr->hr_hri[player].hri_num_inputs[suit], hr->hr_hri[player].hri_inputs[suit],
                            training_inputs, ii);
                        nout = nnt_get_play_output(nn,
                            hr->hr_hri[player].hri_num_inputs[suit], training_inputs, 1, &output);
                        if (nout == 1) {
                            tricks_diff = nnt_check_play_tricks_diff(output, hr->hr_hri[player].hri_outputs[suit][0]);
                            if (tricks_diff == 0)       pst->pst_ncorrect += 1;
                            else if (tricks_diff == 1)  pst->pst_nplus1   += 1;
                            else if (tricks_diff == -1) pst->pst_nminus1  += 1;
                            pst->pst_nhands += 1;
                            if ((og->og_flags & OG_FLAG_DEBUG) ||
                                (!ii && (og->og_flags & OG_FLAG_VERBOSE))) {
                                nnt_show_play_handrec(og, suit,
                                    hr->hr_hri[player].hri_econtract[suit],
                                    hr->hr_hri[player].hri_num_inputs[suit], hr->hr_hri[player].hri_inputs[suit]);
                            }
                        }
                    }
                }
            }
        }
    }
}
/***************************************************************/
void nnt_play_suited_handrec_ssort_longest(struct omarglobals * og,
        struct neural_network *  nn,
        struct handrec * hr,
        struct playstats * pst)
{
    int player;
    int suit;
    double output;
    int nout;
    int tricks_diff;

    for (player = 0; player < 4; player++) {
        for (suit = CLUBS; suit <= SPADES; suit++) {
            if (hr->hr_hri[player].hri_declarer[suit] == player) {
                if (hr->hr_hri[player].hri_num_inputs[suit] == 32 && hr->hr_hri[player].hri_inputs[suit]) {
                        nout = nnt_get_play_output(nn,
                            hr->hr_hri[player].hri_num_inputs[suit], hr->hr_hri[player].hri_inputs[suit], 1, &output);
                        if (nout == 1) {
                            tricks_diff = nnt_check_play_tricks_diff(output, hr->hr_hri[player].hri_outputs[suit][0]);
                            if (tricks_diff == 0)       pst->pst_ncorrect += 1;
                            else if (tricks_diff == 1)  pst->pst_nplus1   += 1;
                            else if (tricks_diff == -1) pst->pst_nminus1  += 1;
                            pst->pst_nhands += 1;
                            if (og->og_flags & OG_FLAG_VERBOSE) {
                                nnt_show_play_handrec(og, suit,
                                    hr->hr_hri[player].hri_econtract[suit],
                                    hr->hr_hri[player].hri_num_inputs[suit], hr->hr_hri[player].hri_inputs[suit]);
                            }
                        }
                }
            }
        }
    }
}
/***************************************************************/
int nnt_play_all_suited_handrecs(struct omarglobals * og,
        struct neural_network *  nn,
        struct handrec ** ahandrec,
        int suit_sorting,
        struct playstats * pst)
{
/*
**
*/
    int estat = 0;
    int ii;

    estat = nnt_verify_handrec_inputs_outputs(og, nn, ahandrec, suit_sorting);
    if (estat) return (estat);

    switch (suit_sorting) {
        case SSORT_ALL:
            for (ii = 0; ahandrec[ii]; ii++) {
                nnt_play_suited_handrec_ssort_all(og, nn, ahandrec[ii], pst);
            }
            break;
        case SSORT_LONGEST:
            for (ii = 0; ahandrec[ii]; ii++) {
                nnt_play_suited_handrec_ssort_longest(og, nn, ahandrec[ii], pst);
            }
            break;
        default:
            break;
    }

    return (estat);
}
/***************************************************************/
void nnt_play_notrump_handrec_ssuit_all(struct omarglobals * og,
        struct neural_network *  nn,
        struct handrec * hr,
        struct playstats * pst)
{
    double output;
    int nout;
    int player;
    int ii;
    int tricks_diff;
    ONN_FLOAT training_inputs[32];

    for (player = 0; player < 4; player++) {
        if (hr->hr_hri[player].hri_declarer[NOTRUMP] != NULL_PLAYER) {
            if (hr->hr_hri[player].hri_num_inputs[NOTRUMP] == 32 && hr->hr_hri[player].hri_inputs[NOTRUMP]) {
                for (ii = 0; ii < 24; ii++) {
                    nnt_arrange_inputs(hr->hr_hri[player].hri_num_inputs[NOTRUMP], hr->hr_hri[player].hri_inputs[NOTRUMP],
                        training_inputs, ii);
                    nout = nnt_get_play_output(nn,
                        hr->hr_hri[player].hri_num_inputs[NOTRUMP], training_inputs, 1, &output);
                    if (nout == 1) {
                        tricks_diff = nnt_check_play_tricks_diff(output, hr->hr_hri[player].hri_outputs[NOTRUMP][0]);
                        if (tricks_diff == 0)       pst->pst_ncorrect += 1;
                        else if (tricks_diff == 1)  pst->pst_nplus1   += 1;
                        else if (tricks_diff == -1) pst->pst_nminus1  += 1;
                        pst->pst_nhands += 1;
                        if ((og->og_flags & OG_FLAG_DEBUG) ||
                            (!ii && (og->og_flags & OG_FLAG_VERBOSE))) {
                            nnt_show_play_handrec(og, NOTRUMP,
                                hr->hr_hri[player].hri_econtract[NOTRUMP],
                                hr->hr_hri[player].hri_num_inputs[NOTRUMP], hr->hr_hri[player].hri_inputs[NOTRUMP]);
                        }
                    }
                }
            }
        }
    }
}
/***************************************************************/
void nnt_play_notrump_handrec_ssuit_longest(struct omarglobals * og,
        struct neural_network *  nn,
        struct handrec * hr,
        struct playstats * pst)
{
    double output;
    int nout;
    int player;
    int tricks_diff;

    for (player = 0; player < 4; player++) {
        if (hr->hr_hri[player].hri_declarer[NOTRUMP] != NULL_PLAYER) {
            if (hr->hr_hri[player].hri_num_inputs[NOTRUMP] == 32 && hr->hr_hri[player].hri_inputs[NOTRUMP]) {
                nout = nnt_get_play_output(nn,
                    hr->hr_hri[player].hri_num_inputs[NOTRUMP], hr->hr_hri[player].hri_inputs[NOTRUMP], 1, &output);
                if (nout == 1) {
                    tricks_diff = nnt_check_play_tricks_diff(output, hr->hr_hri[player].hri_outputs[NOTRUMP][0]);
                    if (tricks_diff == 0)       pst->pst_ncorrect += 1;
                    else if (tricks_diff == 1)  pst->pst_nplus1   += 1;
                    else if (tricks_diff == -1) pst->pst_nminus1  += 1;
                    pst->pst_nhands += 1;
                    pst->pst_nhands += 1;
                    if (og->og_flags & OG_FLAG_DEBUG) {
                        nnt_show_play_handrec(og, NOTRUMP,
                            hr->hr_hri[player].hri_econtract[NOTRUMP],
                            hr->hr_hri[player].hri_num_inputs[NOTRUMP], hr->hr_hri[player].hri_inputs[NOTRUMP]);
                    }
                }
            }
        }
    }
}
/***************************************************************/
int nnt_play_all_notrump_handrecs(struct omarglobals * og,
        struct neural_network *  nn,
        struct handrec ** ahandrec,
        int suit_sorting,
        struct playstats * pst)
{
/*
**
*/
    int estat = 0;
    int ii;

    estat = nnt_verify_handrec_inputs_outputs(og, nn, ahandrec, suit_sorting);
    if (estat) return (estat);

    switch (suit_sorting) {
        case SSORT_ALL:
            for (ii = 0; ahandrec[ii]; ii++) {
                nnt_play_notrump_handrec_ssuit_all(og, nn, ahandrec[ii], pst);
            }
            break;
        case SSORT_LONGEST:
            for (ii = 0; ahandrec[ii]; ii++) {
                nnt_play_notrump_handrec_ssuit_longest(og, nn, ahandrec[ii], pst);
            }
            break;
        default:
            break;
    }

    return (estat);
}
/***************************************************************/
int process_nnplay(struct omarglobals * og,
        struct nnplay * nnp,
        struct nndata * nnd)
{
/*
**
*/
    int estat = 0;
    struct playstats pst;

    init_playstats(&pst);

    if (nnp->nnp_play_type == NNT_METHOD_HANDREC_SUITED) {
        estat = nnt_play_all_suited_handrecs(og, nnd->nnd_nn, og->og_ahandrec,
            SSORT_ALL, &pst);
    } else {
        estat = nnt_play_all_notrump_handrecs(og, nnd->nnd_nn, og->og_ahandrec,
            SSORT_ALL, &pst);
    }
    show_playstats(og, &pst);

    return (estat);
}
/***************************************************************/
int nnt_play_handrec(struct omarglobals * og,
        struct neural_network *  nn,
        struct handrec ** ahandrec,
        int suit_sorting,
        int train_method,
        struct playstats * pst)
{
/*
**
*/
    int estat = 0;

    if (train_method == NNT_METHOD_HANDREC_SUITED) {
        estat = nnt_play_all_suited_handrecs(og, nn, ahandrec, suit_sorting, pst);
    } else {
        estat = nnt_play_all_notrump_handrecs(og, nn, ahandrec, suit_sorting, pst);
    }

    return (estat);
}
/***************************************************************/
struct nnplay * new_nnplay(void) {

    struct nnplay * nnp;

    nnp = New(struct nnplay, 1);
    nnp->nnp_play_type     = 0;

    return (nnp);
}
/***************************************************************/
void free_nnplay(struct nnplay * nnp) {

    Free(nnp);
}
/***************************************************************/
int get_nnplay(struct omarglobals * og,
        struct nnplay * nnp,
        int * argix,
        int argc,
        char *argv[])
{
/*
struct nnplay {
    int nnp_method;
};
*/
    int estat = 0;

    if ((*argix) + 1 > argc) {
        estat = set_error(og, "Insufficient arguments for -nnplay.");
    } else {
        if (!Stricmp(argv[*argix], "SUITED")) {
            nnp->nnp_play_type = NNT_METHOD_HANDREC_SUITED;
        } else if (!Stricmp(argv[*argix], "NOTRUMP")) {
            nnp->nnp_play_type = NNT_METHOD_HANDREC_NOTRUMP;
        } else {
            estat = set_error(og, "Bad training type in -nnplay. Found: %s",
                argv[*argix]);
        }
        (*argix)++;
    }

    return (estat);
}
/***************************************************************/

