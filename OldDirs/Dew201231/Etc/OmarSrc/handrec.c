/***************************************************************/
/* handrec.h                                                   */
/***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include "config.h"
#include "omar.h"
#include "json.h"
#include "bid.h"
#include "dir.h"
#include "util.h"
#include "handrec.h"
#include "natural.h"

struct resultrec { /* rerec_ */
    int     rerec_ns_result;
    int     rerec_npairs;
    int     rerec_pct;
    int     rerec_mp_pct;
};

/***************************************************************/
static int get_handrec_jsontree(
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
void init_contract(CONTRACT * contract)
{
    contract->cont_level    = 0;
    contract->cont_suit     = 0;
    contract->cont_doubled  = 0;
    contract->cont_declarer = NULL_PLAYER;
}
/***************************************************************/
void init_makeable(struct makeable * mkbl)
{
    int declarer;

    mkbl->mkbl_ns_optimum = NULL_SCORE;

    for (declarer = WEST; declarer <= SOUTH; declarer++) {
        init_contract(&(mkbl->mkbl_contract[declarer][CLUBS]));
        init_contract(&(mkbl->mkbl_contract[declarer][DIAMONDS]));
        init_contract(&(mkbl->mkbl_contract[declarer][HEARTS]));
        init_contract(&(mkbl->mkbl_contract[declarer][SPADES]));
        init_contract(&(mkbl->mkbl_contract[declarer][NOTRUMP]));
    }
}
/***************************************************************/
void init_results(struct results * rslt)
{
    rslt->rslt_nresultrec = 0;
    rslt->rslt_aresultrec = NULL;
}
/***************************************************************/
void init_handrec(struct handrec * hr)
{
    int player;
    int suit;

    hr->hr_estat = 0;
    hr->hr_fname = NULL;
    hr->hr_url = NULL;
    hr->hr_acbl_board_no = 0;
    hr->hr_dealer = 0;
    hr->hr_vulnerability = 0;

    memset(hr->hr_hands, 0, sizeof(HANDS));
    init_makeable(&(hr->hr_makeable));
    init_results(&(hr->hr_results));

    for (player = 0; player < 4; player++) {
        for (suit = CLUBS; suit <= NOTRUMP; suit++) {
            hr->hr_hri[player].hri_num_inputs[suit] = 0;
            hr->hr_hri[player].hri_num_outputs[suit] = 0;
            hr->hr_hri[player].hri_inputs[suit] = NULL;
            hr->hr_hri[player].hri_outputs[suit] = NULL;
            hr->hr_hri[player].hri_declarer[suit] = NULL_PLAYER;
            hr->hr_hri[player].hri_tricks_taken[suit] = -1;
        }
    }
}
/***************************************************************/
struct handrec * new_handrec(void)
{
    struct handrec * hr;

    hr = New(struct handrec, 1);
    init_handrec(hr);

    return (hr);
}
/***************************************************************/
int jsonstring_to_hand(
        struct omarglobals * og,
        struct handrec * hr,
        HAND * hand,
        int suit,
        struct jsonstring * suit_jb)
{
/*
** See also parse_hand()
*/
    int estat = 0;
    int ok;
    int card;
    int card_suit;
    int hix;
    int ncards;
    int current_suit = 0;

    hand->length[suit] = 0;
    current_suit = suit;

    if (suit_jb->jb_buf[0] != '-') {    /* void */
        hix = 0;
        ok = 1;
        ncards = 0;
        while (ok && ncards < 13) {
            ok = next_card(og, suit_jb->jb_buf, &hix, &card, &card_suit, &current_suit);
            if (ok < 0) {
                estat = set_error(og, "Illegal card.\n");
                ok = 0;
            } else if (ok) {
                if (hand->length[card_suit] &&
                    hand->cards[card_suit][hand->length[card_suit]-1] <= card) {
                    estat = set_error(og, "Card out of order within suit.\n");
                    ok = 0;
                } else {
                    hand->cards[card_suit][hand->length[card_suit]++] = card;
                    ncards++;
                }
            }
        }
    }

    return (estat);
}
/***************************************************************/
int jsonarray_to_hand(
        struct omarglobals * og,
        struct handrec * hr,
        HAND * hand,
        struct jsonarray * hand_ja)
{
    int estat = 0;
    struct jsonstring * suit_jb;

    if (hand_ja->ja_nvals != 4) {
        estat = set_error(og, "Did not find 4 suits. Found %d\nin file: %s",
            hand_ja->ja_nvals, hr->hr_fname);
    }

    /* Spades */
    if (!estat) {
        suit_jb = jsonarray_get_jsonstring(hand_ja, 0);
        if (!suit_jb) {
            estat = set_error(og, "Cannot find suit %s\nin file: %s", "SPADES", hr->hr_fname);
        } else {
            estat = jsonstring_to_hand(og, hr, hand, SPADES, suit_jb);
        }
    }

    /* Hearts */
    if (!estat) {
        suit_jb = jsonarray_get_jsonstring(hand_ja, 1);
        if (!suit_jb) {
            estat = set_error(og, "Cannot find suit %s\nin file: %s", "HEARTS", hr->hr_fname);
        } else {
            estat = jsonstring_to_hand(og, hr, hand, HEARTS, suit_jb);
        }
    }

    /* Diamonds */
    if (!estat) {
        suit_jb = jsonarray_get_jsonstring(hand_ja, 2);
        if (!suit_jb) {
            estat = set_error(og, "Cannot find suit %s\nin file: %s", "DIAMONDS", hr->hr_fname);
        } else {
            estat = jsonstring_to_hand(og, hr, hand, DIAMONDS, suit_jb);
        }
    }

    /* Clubs */
    if (!estat) {
        suit_jb = jsonarray_get_jsonstring(hand_ja, 3);
        if (!suit_jb) {
            estat = set_error(og, "Cannot find suit %s\nin file: %s", "CLUBS", hr->hr_fname);
        } else {
            estat = jsonstring_to_hand(og, hr, hand, CLUBS, suit_jb);
        }
    }

    return (estat);
}
/***************************************************************/
int jsonarray_to_resultrec(
        struct omarglobals * og,
        struct handrec * hr,
        struct resultrec * rerec,
        struct jsonarray * ritem_ja)
{
/*
**  N/S result, # pairs, PCT, MP%
*/

    int estat = 0;
    int nerr;

    rerec->rerec_ns_result = 0;
    rerec->rerec_npairs    = 0;
    rerec->rerec_pct       = 0;
    rerec->rerec_mp_pct    = 0;

    /* N/S result */
    if (!estat) {
        nerr = jsonarray_get_integer(ritem_ja, 0, &(rerec->rerec_ns_result));
        if (nerr) estat = set_error(og, "Bad %s number\nin file: %s", "N/S result", hr->hr_fname);
    }

    /* # pairs */
    if (!estat) {
        nerr = jsonarray_get_integer(ritem_ja, 1, &(rerec->rerec_npairs));
        if (nerr) estat = set_error(og, "Bad %s number\nin file: %s", "# pairs", hr->hr_fname);
    }

    /* Pct */
    if (!estat) {
        nerr = jsonarray_get_integer(ritem_ja, 2, &(rerec->rerec_pct));
        if (nerr) estat = set_error(og, "Bad %s number\nin file: %s", "Pct", hr->hr_fname);
    }

    /* MP Pct */
    if (!estat) {
        nerr = jsonarray_get_integer(ritem_ja, 3, &(rerec->rerec_mp_pct));
        if (nerr) estat = set_error(og, "Bad %s number\nin file: %s", "MP Pct", hr->hr_fname);
    }

    return (estat);
}
/***************************************************************/
int jsonarray_to_results(
        struct omarglobals * og,
        struct handrec * hr,
        struct results * results,
        struct jsonarray * results_ja)
{
    int estat = 0;
    int ii;
    struct jsonarray * ritem_ja;

    if (!results_ja->ja_nvals) return (0);

    results->rslt_nresultrec = results_ja->ja_nvals;
    results->rslt_aresultrec = New(struct resultrec*, results->rslt_nresultrec);
    memset(results->rslt_aresultrec, 0, results->rslt_nresultrec * sizeof(struct resultrec*));

    for (ii = 0; ii < results_ja->ja_nvals; ii++) {
        ritem_ja = jsonarray_get_jsonarray(results_ja, ii);
        if (!ritem_ja) {
            estat = set_error(og, "Cannot find result #%d\nin file: %s", ii, hr->hr_fname);
        } else {
            results->rslt_aresultrec[ii] = New(struct resultrec, 1);
            estat = jsonarray_to_resultrec(og, hr, results->rslt_aresultrec[ii], ritem_ja);
        }
    }

    return (estat);
}
/***************************************************************/
int char_to_player(char dlrchar)
{
    int dlr;

    switch (dlrchar) {
        case 'n': case 'N': dlr = NORTH; break;
        case 'e': case 'E': dlr = EAST;  break;
        case 's': case 'S': dlr = SOUTH; break;
        case 'w': case 'W': dlr = WEST;  break;
        default:            dlr = (-1);  break;
    }

    return (dlr);
}
/***************************************************************/
int thecommongame_get_dealer(
    struct omarglobals * og,
    struct handrec * hr,
    struct jsonobject * jo)
{
    int estat = 0;
    int dlr;
    struct jsonstring * dealer_jb;

    hr->hr_dealer = 0;
    dealer_jb = jsonobject_get_jsonstring(jo, "dealer");
    if (!dealer_jb) {
        estat = set_error(og, "Cannot find %s\nin file: %s", "dealer", hr->hr_fname);
    } else if (!dealer_jb->jb_bytes) {
        estat = set_error(og, "%s empty\nin file: %s", "dealer", hr->hr_fname);
    } else {
        dlr = char_to_player(dealer_jb->jb_buf[0]);
        if (dlr < 0) {
            estat = set_error(og, "Unrecognized %s (%s)\nin file: %s",
                dealer_jb->jb_buf, "dealer", hr->hr_fname);
        } else {
            hr->hr_dealer = dlr;
        }
    }

    return (estat);
}
/***************************************************************/
int chars_to_vulnerability(const char * vulstr)
{
    int vul;

    if (!Stricmp(vulstr, "none")) {
        vul = VUL_NONE;
    } else if (!Stricmp(vulstr, "ns") || !Stricmp(vulstr, "n-s")) {
        vul = VUL_NS;
    } else if (!Stricmp(vulstr, "ew") || !Stricmp(vulstr, "e-w")) {
        vul = VUL_EW;
    } else if (!Stricmp(vulstr, "both")) {
        vul = VUL_BOTH;
    } else {
        vul = (-1);
    }

    return (vul);
}
/***************************************************************/
int thecommongame_get_vulnerability(
        struct omarglobals * og, 
        struct handrec * hr,
        struct jsonobject * jo)
{
    int estat = 0;
    int vul;
    struct jsonstring * vulnerability_jb;

    hr->hr_vulnerability = 0;
    vulnerability_jb = jsonobject_get_jsonstring(jo, "vulnerability");
    if (!vulnerability_jb) {
        estat = set_error(og, "Cannot find %s\nin file: %s", "vulnerability", hr->hr_fname);
    } else if (!vulnerability_jb->jb_bytes) {
        estat = set_error(og, "%s empty\nin file: %s", "vulnerability", hr->hr_fname);
    } else {
        vul = chars_to_vulnerability(vulnerability_jb->jb_buf);
        if (vul < 0) {
            estat = set_error(og, "Unrecognized %s (%s)\nin file: %s",
                vulnerability_jb->jb_buf, "vulnerability", hr->hr_fname);
        } else {
            hr->hr_vulnerability = vul;
        }
    }

    return (estat);
}
/***************************************************************/
int thecommongame_parse_makeable(struct omarglobals * og,
        struct handrec * hr,
        struct makeable * mkbl,
        const char * mkblbuf)
{
/*
** e.g. mkblbuf = "NS 5N; NS 4S; NS 4H; NS 5C; EW 2D; Optimal Contract +460"
*/

    int estat = 0;
    int mix;
    int prev_mix;
    int pair;
    int ok;
    int bt;
    int bl;
    int bs;
    int nmakeable;
    int nerr;
    int declarer;
    char toke[SMTOKESIZE];
 
    if (og->og_flags & OG_FLAG_VERBOSE) {
        omarout(og, "Handrec makeable = \"%s\"\n", mkblbuf);
    }

    mix = 0;
    nmakeable = 0;
    ok = 1;
    init_makeable(mkbl);

    while (ok) {
        prev_mix = mix;
        ok = next_pair(mkblbuf, &mix, &pair);
        if (ok) {
            declarer = (pair == NORTH_SOUTH)?NORTH:EAST;
        } else {
            pair = -1;
            mix = prev_mix;
            ok = next_player(mkblbuf, &mix, &declarer);
        }

        if (ok) {
            ok = next_bid(og, mkblbuf, &mix, &bt, &bl, &bs);
            if (!ok) declarer = -1;

            while (declarer >= 0) {
                if (mkbl->mkbl_contract[declarer][bs].cont_level) {
                    estat = set_error(og, "Duplicate contract in makeable: %s\nin file: %s",
                        mkblbuf, hr->hr_fname);
                    ok = 0;
                } else {
                    mkbl->mkbl_contract[declarer][bs].cont_level    = bl;
                    mkbl->mkbl_contract[declarer][bs].cont_suit     = bs;
                    mkbl->mkbl_contract[declarer][bs].cont_doubled  = bt;
                    mkbl->mkbl_contract[declarer][bs].cont_declarer = declarer;
                    nmakeable++;
                }
                if (pair >= 0) {
                    declarer = (pair == NORTH_SOUTH)?SOUTH:WEST;
                    pair = -1;
                } else {
                    declarer = -1;
                }
            }
        }
        if (ok) {
            get_toke_char(mkblbuf, &mix, toke, SMTOKESIZE, 0);
            if (toke[0] != ';') {
                ok = 0;
            }
        }
    }

    if (!estat) {
        mix = prev_mix;
        get_toke_char(mkblbuf, &mix, toke, SMTOKESIZE, 0);
        if (strcmp(toke, "Optimal")) {
            estat = set_error(og, "Expecting 'Optimal' in makeable: %s\nin file: %s",
            mkblbuf, hr->hr_fname);
        } else {
            get_toke_char(mkblbuf, &mix, toke, SMTOKESIZE, 0);
            if (strcmp(toke, "Contract")) {
                estat = set_error(og, "Expecting 'Contract' in makeable: %s\nin file: %s",
                mkblbuf, hr->hr_fname);
            } else {
                get_toke_char(mkblbuf, &mix, toke, SMTOKESIZE, 0);
                nerr = char_to_integer(toke, &(mkbl->mkbl_ns_optimum));
                if (nerr) {
                    estat = set_error(og,
                    "Expecting number for 'Optimal Contract' in makeable: %s\nin file: %s",
                        mkblbuf, hr->hr_fname);
                    /* omarout(og, "URL: %s\n", hr->hr_url); */
                }
            }
        }
    }

    return (estat);
}
/***************************************************************/
int thecommongame_get_makeable(struct omarglobals * og,
        struct handrec * hr,
        struct makeable * mkbl,
        struct jsonobject * jo)
{
    int estat = 0;
    struct jsonstring * makeable_jb;

    makeable_jb = jsonobject_get_jsonstring(jo, "makeable");
    if (!makeable_jb) {
        estat = set_error(og, "Cannot find %s\nin file: %s", "makeable", hr->hr_fname);
    } else if (!makeable_jb->jb_bytes) {
        estat = set_error(og, "%s empty\nin file: %s", "makeable", hr->hr_fname);
    } else {
        estat = thecommongame_parse_makeable(og, hr, mkbl, makeable_jb->jb_buf);
    }

    return (estat);
}
/***************************************************************/
int thecommongame_get_url(struct omarglobals * og,
        struct handrec * hr,
        struct jsonobject * jo)
{
    int estat = 0;
    struct jsonstring * url_jb;

    url_jb = jsonobject_get_jsonstring(jo, "url");
    if (!url_jb) {
        estat = set_error(og, "Cannot find %s\nin file: %s", "url", hr->hr_fname);
    } else if (!url_jb->jb_bytes) {
        estat = set_error(og, "%s empty\nin file: %s", "url", hr->hr_fname);
    } else {
        hr->hr_url = Strdup(url_jb->jb_buf);
    }

    return (estat);
}
/***************************************************************/
int thecommongame_jsontree_to_handrec(
        struct omarglobals * og,
        struct handrec * hr,
        struct jsonobject * jo)
{
    int estat = 0;
    struct jsonobject * hands_jo;
    struct jsonarray * hand_ja;
    struct jsonarray * results_ja;

    estat = thecommongame_get_url(og, hr, jo);

    if (!estat) {
        estat = thecommongame_get_dealer(og, hr, jo);
    }

    if (!estat) {
        estat = thecommongame_get_vulnerability(og, hr, jo);
    }

    if (!estat) {
        estat = thecommongame_get_makeable(og, hr, &(hr->hr_makeable), jo);
    }

    if (!estat) {
        hands_jo = jsonobject_get_jsonobject(jo, "hands");
        if (!hands_jo) {
            estat = set_error(og, "Cannot find %s\nin file: %s", "hands", hr->hr_fname);
        }
    }

    /* North */
    if (!estat) {
        hand_ja = jsonobject_get_jsonarray(hands_jo, "NORTH");
        if (!hand_ja) {
            estat = set_error(og, "Cannot find player %s\nin file: %s", "NORTH", hr->hr_fname);
        } else {
            estat = jsonarray_to_hand(og, hr, &(hr->hr_hands[NORTH]), hand_ja);
        }
    }

    /* East */
    if (!estat) {
        hand_ja = jsonobject_get_jsonarray(hands_jo, "EAST");
        if (!hand_ja) {
            estat = set_error(og, "Cannot find player %s\nin file: %s", "EAST", hr->hr_fname);
        } else {
            estat = jsonarray_to_hand(og, hr, &(hr->hr_hands[EAST]), hand_ja);
        }
    }

    /* South */
    if (!estat) {
        hand_ja = jsonobject_get_jsonarray(hands_jo, "SOUTH");
        if (!hand_ja) {
            estat = set_error(og, "Cannot find player %s\nin file: %s", "SOUTH", hr->hr_fname);
        } else {
            estat = jsonarray_to_hand(og, hr, &(hr->hr_hands[SOUTH]), hand_ja);
        }
    }

    /* West */
    if (!estat) {
        hand_ja = jsonobject_get_jsonarray(hands_jo, "WEST");
        if (!hand_ja) {
            estat = set_error(og, "Cannot find player %s\nin file: %s", "WEST", hr->hr_fname);
        } else {
            estat = jsonarray_to_hand(og, hr, &(hr->hr_hands[WEST]), hand_ja);
        }
    }

    /* results */
    if (!estat) {
        results_ja = jsonobject_get_jsonarray(jo, "results");
        if (!results_ja) {
            estat = set_error(og, "Cannot find %s\nin file: %s", "results", hr->hr_fname);
        } else {
            estat = jsonarray_to_results(og, hr, &(hr->hr_results), results_ja);
        }
    }

    return (estat);
}
/***************************************************************/
struct handrec * parse_handrec(struct omarglobals * og, const char * hrfname)
{
    struct handrec * hr;
    struct jsontree * jt;

    hr = new_handrec();
    hr->hr_fname = Strdup(hrfname);

    if (og->og_flags & OG_FLAG_VERBOSE) {
        omarout(og, "Processing file: %s\n", hr->hr_fname);
    }

    hr->hr_estat = get_handrec_jsontree(og, hrfname, &jt);

    if (!hr->hr_estat && jt) {
        if (jt->jt_jv && jt->jt_jv->jv_vtyp == VTAB_VAL_OBJECT) {
            jsonobject_decode_base64(jt->jt_jv->jv_val.jv_jo, "analysis");
            /* jsonobject_decode_base64(jt->jt_jv->jv_val.jv_jo, "makeable"); */
            jsonobject_decode_base64(jt->jt_jv->jv_val.jv_jo, "url");

            if (0 && og->og_flags & OG_FLAG_VERBOSE) {
                print_jsontree(get_outfile(og), jt, 0);
            }

            hr->hr_estat = thecommongame_jsontree_to_handrec(og, hr, jt->jt_jv->jv_val.jv_jo);

            if (hr->hr_estat) {
                if (!(og->og_flags & OG_FLAG_QUIET)) {
                    omarout(og, "*Error\nin file: %s\n", hrfname);
                }
                /* print_jsontree(get_outfile(), jt, 0);  */
                free_handrec(hr);
                hr = NULL;
            }
        } else {
            hr->hr_estat = set_error(og, "Invalid object\nin file: %s", hrfname);
        }

        free_jsontree(jt);
    }

    return (hr);
}
/***************************************************************/
void print_results(struct omarglobals * og, struct results * results)
{
    int ii;
    struct resultrec * rerec;

    omarout(og, "Number of results = %d\n", results->rslt_nresultrec);

    for (ii = 0; ii < results->rslt_nresultrec; ii++) {
        rerec = results->rslt_aresultrec[ii];
        omarout(og, "%-4d %4d %3d %3d\n",
            rerec->rerec_ns_result,
            rerec->rerec_npairs,
            rerec->rerec_pct,
            rerec->rerec_mp_pct);
    }
}
/***************************************************************/
void print_contract(struct omarglobals * og, CONTRACT * contract)
{
    int ebid;

    ebid = encode_bid(contract->cont_doubled, contract->cont_level, contract->cont_suit);

    print_display_player(og, contract->cont_declarer);
    omarout(og, " ");
    print_ebid(og, ebid);
}
/***************************************************************/
void print_makeable_contracts(struct omarglobals * og, struct makeable * mkbl)
{
    int declarer;
    int suit;
    int nmakeable;

    nmakeable = 0;

    for (declarer = WEST; declarer <= SOUTH; declarer++) {
        for (suit = CLUBS; suit <= NOTRUMP; suit++) {
            if (mkbl->mkbl_contract[declarer][suit].cont_declarer != NULL_PLAYER) {
                if (nmakeable) omarout(og, "; ");
                print_contract(og, &(mkbl->mkbl_contract[declarer][suit]));
                nmakeable++;
            }
        }
    }
}
/***************************************************************/
void print_makeable(struct omarglobals * og, struct makeable * mkbl)
{
    print_makeable_contracts(og, mkbl);
    omarout(og, "\n");
    omarout(og, "Optimal contract = %+d\n", mkbl->mkbl_ns_optimum);
}
/***************************************************************/
void print_handrec(struct omarglobals * og, struct handrec * hr)
{
    print_hands(og, &(hr->hr_hands));
    omarout(og, "Dealer: ");
    print_player(og, hr->hr_dealer);
    omarout(og, "   ");
    print_vulnerability(og, hr->hr_vulnerability);
    omarout(og, "\n");
    print_makeable(og, &(hr->hr_makeable));
    /* print_results(&(hr->hr_results)); */
    omarout(og, "\n");
}
/***************************************************************/
void free_results(struct results * rslt)
{
    int ii;

    if (!rslt) return;

    for (ii = 0; ii < rslt->rslt_nresultrec; ii++) {
        Free(rslt->rslt_aresultrec[ii]);
    }
    Free(rslt->rslt_aresultrec);
}
/***************************************************************/
void free_handrec_data(struct handrec * hr)
{
    int player;
    int suit;

    if (!hr) return;

    free_results(&(hr->hr_results));
    Free(hr->hr_fname);
    Free(hr->hr_url);

    for (player = 0; player < 4; player++) {
        for (suit = CLUBS; suit <= NOTRUMP; suit++) {
            Free(hr->hr_hri[player].hri_inputs[suit]);
            Free(hr->hr_hri[player].hri_outputs[suit]);
        }
    }
}
/***************************************************************/
void free_handrec(struct handrec * hr)
{
    if (!hr) return;

    free_handrec_data(hr);

    Free(hr);
}
/***************************************************************/
void add_handrec(struct omarglobals * og, struct handrec * hr)
{

    if (og->og_nhandrec >= og->og_xhandrec) {
        if (!og->og_xhandrec) og->og_xhandrec = 16;
        else og->og_xhandrec *= 2;
        og->og_ahandrec = Realloc(og->og_ahandrec, struct handrec *, og->og_xhandrec + 1);
    }
    og->og_ahandrec[og->og_nhandrec] = hr;
    og->og_nhandrec += 1;
    og->og_ahandrec[og->og_nhandrec] = NULL;
}
/***************************************************************/
/*@*/void parse_handrec_prog(
            struct omarglobals * og,
            struct program * prog,
            const char * hrreq) {

    struct infile * inf;
    OPSTACK * ops;
    char token[TOKEN_MAX];

    ops = New(OPSTACK, 1);

    inf = infile_new_chars(hrreq);

    next_token(inf, token);

    if (!strcmp(token, "SEL")) {
        next_token(inf, token);
        parse_exp(og, inf, token, prog, ops, EXPFLAG_HANDREC);
        if (!omargeterr(og) &&
            !ops->err && (ops->oper[ops->tos-1].dtype != BOOL_DTYPE)) {
            omarerr(og, "SEL requires boolean datatype.\n");
        } else {
            append_prog(prog, END_OP);
        }
    } else {
        omarerr(og, "Expecting SEL.  Found '%s'.\n", token);
        next_token(inf, token);
    }

    if (!omargeterr(og) && token[0]) {
        omarerr(og, "Expecting end of line.  Found '%s'.\n", token);
    }

    infile_free(inf, 0);
    Free(ops);

#if 0
    if (!omargeterr(og)) {
        print_score_prog(get_outfile(og), prog);
    }
#endif
}
/***************************************************************/
/*@*/int run_handrec_prog(
            struct omarglobals * og,
            struct handrec * hr,
            struct program * prog)
{
    int hr_sel;
    AUCTION * auction = NULL;
    ANALYSIS * analysis = NULL;
    HANDINFO handinfo[4];
    int ii;

    for (ii = 0; ii < 4; ii++) {
        make_handinfo(&(hr->hr_hands[ii]), &(handinfo[ii]));
    }

    hr_sel = (int)run_prog(og, prog, auction, analysis, handinfo, hr);
    if (omargeterr(og)) hr_sel = 0;

    return (hr_sel);
}
/***************************************************************/
struct handrec ** parse_handrecdir(struct omarglobals * og,
    const char * hrdirname,
    const char * hrreq)
{
    struct handrec ** hrl;
    struct handrec * hr;
    char ** filenames;
    int ii;
    int sel;
    int nhrl;
    int xhrl;
    char ** badfiles;
    int nbadfiles;
    int estat = 0;
    struct program * sel_prog;

    hrl = NULL;
    hr = NULL;
    sel_prog = NULL;
    nhrl = 0;
    xhrl = 0;
    badfiles = NULL;
    nbadfiles = 0;

    if (hrreq) {
        sel_prog = New(struct program, 1);
        parse_handrec_prog(og, sel_prog, hrreq);
        estat = omargeterr(og);
    }

    if (!estat) {
        estat = get_filenames(hrdirname, &filenames);
    }

    if (!estat && filenames) {
        for (ii = 0; !estat && filenames[ii]; ii++) {
            /* omarout(og, "%s\n", filenames[ii]); */
            hr = parse_handrec(og, filenames[ii]);
            /* estat = hr->hr_estat; */

            if (hr && sel_prog) {
                sel = run_handrec_prog(og, hr, sel_prog);
                if (!sel) {
                    free_handrec(hr);
                    hr = NULL;
                }
            }

            if (hr) {
                /* new stuff */
                if (nhrl >= xhrl) {
                    if (!xhrl) xhrl = 16;
                    else xhrl *= 2;
                    hrl = Realloc(hrl, struct handrec *, xhrl + 1);
                }
                hrl[nhrl] = hr;
                nhrl += 1;
                hrl[nhrl] = NULL;
                /* **** */
            } else if (!sel_prog) {
                badfiles = Realloc(badfiles, char*, nbadfiles + 2);
                badfiles[nbadfiles] = Strdup(filenames[ii]);
                nbadfiles++;
                badfiles[nbadfiles] = NULL;
            }
        }
        /* omarout(og, "Files proccessed = %d\n", ii); */
        if (nbadfiles) omarout(og, "Files with errors = %d\n", nbadfiles);
        free_charlist(filenames);
    }

    for (ii = 0; ii < nbadfiles; ii++) {
        omarout(og, "%s\n", badfiles[ii]);
    }
    free_charlist(badfiles);
    free_program(sel_prog);

    return (hrl);
}
/***************************************************************/
void copy_hand(struct hand * tgt_hand, struct hand * src_hand) {

    int suit;
    int ii;

    for (suit=0; suit<4; suit++) {
        tgt_hand->length[suit] = src_hand->length[suit];
        for (ii=0; ii < src_hand->length[suit]; ii++) {
            tgt_hand->cards[suit][ii] = src_hand->cards[suit][ii];
        }
    }
}
/***************************************************************/
void make_next_bid(struct omarglobals * og,
            struct seqinfo * si,
            AUCTION * auction,
            ANALYSIS * analysis,
            HAND * hand,
            HANDINFO * handinfo) {

    int bestbid;
    int trace;
    int bidno;

    trace = 1;

    if (trace) {
        print_hand(og, hand);
    }

    bestbid = find_best_bid(og, si, auction, analysis, handinfo);
    if (!bestbid) {
        omarout(og, "* No bids available *\n");
    } else {
        bidno = auction->au_nbids;
        make_ebid(auction, bestbid);
        analyze_bid(og, si, auction, bidno, analysis, 0);
    }
}
/***************************************************************/
void bid_handrec_hand(struct omarglobals * og,
            struct handrec * hr,
            struct seqinfo * si) {

    AUCTION * auction;
    ANALYSIS * analysis;
    HANDINFO handinfo;
    int ok;
    HAND hand;
    int trace;
    int player;
    int done;
    int nbids;

    player = hr->hr_dealer;
    trace = 1;
    ok = 1;
    print_hands(og, &(hr->hr_hands));

    auction = new_auction(hr->hr_dealer, hr->hr_vulnerability);

    analysis = new_analysis();
    init_cc(og, si);
    analyze_auction(og, si, auction, analysis, 0);

    nbids = 0;
    done = 0;
    while (!done) {
        copy_hand(&hand, &(hr->hr_hands[player]));
        make_handinfo(&hand, &handinfo);
        handinfo.trace = trace;

        print_hand(og, &hand);
        omarout(og, "\n");

        make_next_bid(og, si, auction, analysis, &hand, &handinfo);
        
        player = LHO(player);
        nbids++;
        if (nbids > 4) done = 1;
    }
    print_auction(og, auction);

    Free(analysis);
    free_cc(si);

    Free(auction);
}
/***************************************************************/
void bid_handrec(struct omarglobals * og, struct handrec * hr, struct seqinfo * si)
{
    bid_handrec_hand(og, hr, si);
}
/***************************************************************/
void check_contract(struct omarglobals * og, struct handrec * hr) {

    struct makeable mkbl;
    
    calculate_makeable(og, hr->hr_dealer, &(hr->hr_hands), &(mkbl));

    print_makeable(og, &(mkbl));
}
/***************************************************************/
void check_contracts(struct omarglobals * og) {

    int ii;

    for (ii = 0; ii < og->og_nhandrec; ii++) {
        check_contract(og, og->og_ahandrec[ii]);
    }
}
/***************************************************************/
int calculate_suit_makes(struct makeable * mkbl,
                int declarer,
                int suit)
{
    int nmakes = 0;

    if (mkbl->mkbl_contract[declarer][suit].cont_declarer != NULL_PLAYER) {
        nmakes = mkbl->mkbl_contract[declarer][suit].cont_level;
    }

    return (nmakes);
}
/***************************************************************/
int calculate_makes(struct omarglobals * og,
                struct handrec * hr,
                int player,
                int suit)
{
    int nmakes = 0;

    nmakes = calculate_suit_makes(&(hr->hr_makeable), player, suit);

#if 0
    if (nmakes) {
        print_handrec(og, hr);
    }
#endif

    return (nmakes);
}
/***************************************************************/
