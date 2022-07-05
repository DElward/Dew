/**************************************************************/
/* strbest.c                                                  */
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
#include "strbest.h"
#include "randnum.h"
#include "play.h"
#include "demi.h"

#if DEMI_TESTING
    extern int demi_test_all(struct demi_boss * demb);
    int demi_test_register_callable_functions(void * vdemi,
        int(*register_func)(void * vd, const char * funcnam, DEMI_CALLABLE_FUNCTION func),
        const char * funccfg);
#endif

/***************************************************************/
int parse_best_strategy_subtype(struct stratbest * strbest,
        const char *line,
        int * lix,
        char * stoke)
{
    int estat = 0;
    int num;
    int done;
    int plix;

    done = 0;

    do {
        gettoke(line, lix, stoke, STOKE_MAX_SIZE);
        if (strcmp(stoke, ";")) {
            done = 1;
        } else {
            plix = (*lix);
            gettoke(line, lix, stoke, STOKE_MAX_SIZE);
            if (!strcmp(stoke, ";")) {
                (*lix) = plix;
            } else if (!strcmp(stoke, "SHOWTOTS")) {
                strbest->strbest_show_flag |= STRBEST_SHOW_FLAG_SHOWTOTS;
            } else if (!strcmp(stoke, "SHOWEACHTOT")) {
                strbest->strbest_show_flag |= STRBEST_SHOW_FLAG_SHOWEACHTOT;
            } else if (!strcmp(stoke, "MINTRIES")) {
                gettoke(line, lix, stoke, STOKE_MAX_SIZE);
                if (strcmp(stoke, "=")) {
                    estat = set_error(ESTAT_EXPEQUAL,
                        "Expecting equal sign (=). Found: %s", stoke);
                } else {
                    gettoke(line, lix, stoke, STOKE_MAX_SIZE);
                    if (stoui(stoke, &num)) {
                        estat = set_error(ESTAT_BADNUMBER,"Invalid number: %s", stoke);
                    } else {
                        strbest->strbest_mintries  = num;
                    }
                }
            } else if (!strcmp(stoke, "SHUFFLE")) {
                gettoke(line, lix, stoke, STOKE_MAX_SIZE);
                if (strcmp(stoke, "=")) {
                    estat = set_error(ESTAT_EXPEQUAL,
                        "Expecting equal sign (=). Found: %s", stoke);
                } else {
                    gettoke(line, lix, stoke, STOKE_MAX_SIZE);
                    estat = shuffle_parm(stoke, &(strbest->strbest_shuf_rgen), &(strbest->strbest_shuf_rseed));
                }
            } else {
                estat = set_error(ESTAT_BADSTRATINFO,
                    "Unrecognized best strategy info: %s", stoke);
            }
        }
    } while (!estat && !done);

    return (estat);
}
/***************************************************************/
void strat_best_init_demi(struct stratbest * strbest)
{
/*
** Called once to initialize the best strategy demi
*/
    strbest->strbest_demb              = demb_new_demi_boss();
    strbest->strbest_demf_calc_opportunity_iota_best = NULL;
}
/***************************************************************/
void strat_best_init(struct stratbest * strbest)
{
/*
** Called once to initialize the best strategy
*/
    strbest->strbest_erg                = NULL;
    strbest->strbest_rg                 = NULL;
    strbest->strbest_rgen               = PLAY_DEFAULT_RGEN;
    strbest->strbest_rseed              = PLAY_DEFAULT_RSEED;
    strbest->strbest_show_flag          = 0;
    strbest->strbest_mintries           = BEST_DEFAULT_MIN_TRIES;
    strbest->strbest_shuf_rgen         = -1;
    strbest->strbest_shuf_rseed        = -1;
    strbest->strbest_shuf_rg           = NULL;

    strat_best_init_demi(strbest);
}
/***************************************************************/
void strat_best_shut(struct stratbest * strbest)
{
/*
** Called once to shut/free the simple strategy
*/
    if (strbest->strbest_rg) randintfree(strbest->strbest_rg);
    if (strbest->strbest_demb) demb_shut(strbest->strbest_demb);
}
/**************************************************************/
int strat_play_best_init(struct game * gm, struct playinfo * pli, struct stratbest * strbest)
{
/*
** Called once for simple strategy before the beginning of all matches
*/
    int estat = 0;

    if (strbest->strbest_rg) {
        strbest->strbest_erg = strbest->strbest_rg;
    } else if (gm->gm_erg) {
        strbest->strbest_erg = gm->gm_erg;
    } else if (strbest->strbest_shuf_rgen < 0) {
        strbest->strbest_rg = randintseed(strbest->strbest_rgen, strbest->strbest_rseed);
        strbest->strbest_erg = strbest->strbest_rg;
    }

    return (estat);
}
/**************************************************************/
point_t calc_best_opponent_opportunity_iota_best(
    struct gstate * gst,
    struct opportunityitem * opi,
    /* struct stratbest * strbest, */
    count_t player,
    count_t depth);

point_t calc_opportunity_iota_best(
    struct gstate * gst,
    struct opportunityitem * opi,
    /* struct stratbest * strbest, */
    count_t player,
    count_t depth)
{
    point_t opiota;
    card_t card_played;
    bitmask_t slotmask;
    count_t slotix;
    int ii;
    point_t opp_best_iota;

    card_played = gst->gst_hand[player][opi->opi_cardix];
    opiota = 0;

    if (opi->opi_type == OPTYP_TAKE) {
        BITFOR(slotmask,opi->opi_slotmask,slotix) {
            for (ii = 0; ii < gst->gst_board[slotix].sl_ncard; ii++) {
                opiota += calc_card_iota(gst->gst_board[slotix].sl_card[ii]);
            }
        }
        opiota += calc_card_iota(card_played);
    }

    if (gst->gst_nhand[PLAYER_OPPONENT(player)]) {
        opp_best_iota = calc_best_opponent_opportunity_iota_best(gst, opi,
            /* strbest, */ player, depth);
        opiota -= opp_best_iota;
    }

    return (opiota);
}
/**************************************************************/
void prune_opportunities_best(struct gstate * gst,
        struct opportunitylist * opl,
        /* struct stratbest * strbest, */
        count_t player)
{
    if (opl->opl_opi[0]->opi_type                 != OPTYP_TRAIL &&
        opl->opl_opi[opl->opl_nopi - 1]->opi_type == OPTYP_TRAIL) {
        if (gst->gst_nboard > CHEAT_MAX_SLOTS ||
            opl->opl_nopi > CHEAT_MAX_OPPORTUNITIES) {
            while (opl->opl_opi[opl->opl_nopi - 1]->opi_type == OPTYP_TRAIL) {
                opl->opl_nopi -= 1;
            }
        }
    }
}
/**************************************************************/
point_t calc_best_opponent_opportunity_iota_best(
    struct gstate * gst,
    struct opportunityitem * opi,
    /* struct stratbest * strbest, */
    count_t player,
    count_t depth)
{
    struct opportunitylist opp_opl;
    struct gstate opp_gst;
    point_t opp_best_iota;
    point_t opp_opiota;
    int opp_best_opix;
    int opp_opix;
    count_t opponent;

#if IS_DEBUG
    if (is_msglevel(MSGLEVEL_DEBUG_SHOWBEFORE)) {
        show_opportunityitem_played(gst, opi, player, 3, depth);
    }
#endif

    COPY_GSTATE(&opp_gst, gst);
    play_opportunity(&opp_gst, opi, player);

#if IS_DEBUG
    if (is_msglevel(MSGLEVEL_DEBUG_SHOWAFTER)) {
        msgout(MSGLEVEL_TERSE, "%d. After:\n", depth);
        gstate_print(MSGLEVEL_TERSE, &opp_gst,
            FLAG_PLAYER_EAST | FLAG_PLAYER_WEST | FLAG_BOARD | FLAG_BOARD_SHOW_OWNER);
    }
#endif

    opponent = PLAYER_OPPONENT(player);
    init_opportunitylist(&opp_opl);
    game_opportunities(
        &opp_opl,
        opp_gst.gst_board,
        opp_gst.gst_nboard,
        opp_gst.gst_hand[opponent],
        opp_gst.gst_nhand[opponent],
        opponent);
    prune_opportunities_best(&opp_gst, &opp_opl, /* strbest, */ opponent);

    DESSERT(opp_opl.opl_nopi > 0, "No opportunities after pruning.");
    opp_best_iota = calc_opportunity_iota_best(&opp_gst, opp_opl.opl_opi[0], /* strbest, */ opponent, depth + 1);
    opp_best_opix = 0;

    for (opp_opix = 1; opp_opix < opp_opl.opl_nopi; opp_opix++) {
        opp_opiota = calc_opportunity_iota_best(&opp_gst, opp_opl.opl_opi[opp_opix], /* strbest, */ opponent, depth + 1);
        if (opp_opiota > opp_best_iota) {
            opp_best_iota = opp_opiota;
            opp_best_opix = opp_opix;
        }
    }
    free_opportunities(&opp_opl);

#if IS_DEBUG
    if (is_msglevel(MSGLEVEL_DEBUG_SHOWBEFORE)) {
        msgout(MSGLEVEL_TERSE, "%d. Best iota for %s = %d\n",
            depth, PLAYER_NAME(opponent), opp_best_iota);
    }
#endif

    return (opp_best_iota);
}
/**************************************************************/
boolean_t calc_card_read_matches_best(
    struct gstate * opp_gst,
	struct slot * g_board,
    count_t       g_nboard,
	card_t      * opp_hand,
    count_t       opp_nhand,
    /* struct stratbest * strbest, */
    count_t opponent)
{
    boolean_t hand_matches;
    count_t slotix;
    count_t cardix;
	count_t val;

    hand_matches = 1;
    for (slotix = 0; hand_matches && slotix < g_nboard; slotix++) {
        if (g_board[slotix].sl_owner == opponent) {
            val = g_board[slotix].sl_value;
            hand_matches = 0;
            for (cardix = 0; !hand_matches && cardix < opp_nhand; cardix++) {
                if (GET_PIP(opp_hand[cardix]) == val) hand_matches = 1;
            }
        }
    }

    return (hand_matches);
}
/***************************************************************/
void quicksort_int_keydata(int * keyint, int * datint, int l, int r)
{
    int i;
    int j;
    int v;
    int u;

    if (r > l) {
        v = keyint[r];
        i = l - 1;
        j = r;
        for (;;) {
            while (i < j && keyint[++i] > v) ; /* use < v for asc, > v for desc */
            while (i < j && keyint[--j] < v) ; /* use > v for asc, < v for desc */
            if (i >= j) break;
            u = keyint[i]; keyint[i] = keyint[j]; keyint[j] = u;
            u = datint[i]; datint[i] = datint[j]; datint[j] = u;
        }
        u = keyint[i]; keyint[i] = keyint[r]; keyint[r] = u;
        u = datint[i]; datint[i] = datint[r]; datint[r] = u;
        quicksort_int_keydata(keyint, datint, l, i-1);
        quicksort_int_keydata(keyint, datint, i+1, r);
    }
}
/***************************************************************/
void show_opportunity_iotas(
    struct opportunitylist * opl,
    int   * iota_vals,
    int   * iota_index,
	struct slot * g_board,
    count_t       g_nboard,
	card_t      * g_hand,
    count_t       g_nhand,
    count_t       g_player)
{
    int ii;
    struct opportunityitem * opi;
    int sblen;
    char slotbuf[PBUF_SIZE];

    msgout(MSGLEVEL_TERSE, "Opportunities for %s\n", PLAYER_NAME(g_player));
    for (ii = 0; ii < opl->opl_nopi; ii++) {
        if (iota_index) opi = opl->opl_opi[iota_index[ii]];
        else            opi = opl->opl_opi[ii];
        sprintf(slotbuf, "%3d %6d ", ii, iota_vals[ii]);
        sblen = IStrlen(slotbuf);
        sblen += show_opportunityitem(slotbuf + sblen,
            opi, g_board, g_hand);
        msgout(MSGLEVEL_TERSE, "%s\n", slotbuf);
    }
    msgout(MSGLEVEL_TERSE, "\n");
}
/**************************************************************/
index_t strat_best_showtots(
    struct gstate * opp_gst,
    struct opportunitylist * opl,
    int * tot_iota,
    count_t player)
{
    index_t best_opix;
    int   * iota_vals;
    int   * iota_index;
    index_t opix;

    iota_vals  = New(int, opl->opl_nopi);
    iota_index = New(int, opl->opl_nopi);
    for (opix = 0; opix < opl->opl_nopi; opix++) {
        iota_vals[opix]  = tot_iota[opix];
        iota_index[opix] = opix;
    }
    quicksort_int_keydata(iota_vals, iota_index, 0, opl->opl_nopi - 1);
    gstate_print(MSGLEVEL_TERSE, opp_gst,
        GET_PLAYER_FLAG(player) | FLAG_BOARD | FLAG_BOARD_SHOW_OWNER);
    show_opportunity_iotas(opl, iota_vals, iota_index,
        opp_gst->gst_board,
        opp_gst->gst_nboard,
        opp_gst->gst_hand[player],
        opp_gst->gst_nhand[player],
        player);

    best_opix = iota_index[0];
    
    Free(iota_index);
    Free(iota_vals);

    return (best_opix);
}
/**************************************************************/
void calc_opportunity_iota_best_demi(
    struct gstate * gst,
    struct opportunityitem * opi,
    struct stratbest * strbest,
    count_t player,
    count_t depth,
    int * p_opiota,
    int * p_estat)
{
/*
** 12/08/2019
*/
    int dstat;
    struct calc_opportunity_iota_best_ARGS args;

    (*p_estat) = 0;
    if (!strbest->strbest_demb) {
        (*p_opiota) = calc_opportunity_iota_best(gst, opi, /* strbest, */ player, 0);
    } else {
        args.gst    = gst;
        args.opi    = opi;
        args.player = player;
        args.depth  = depth;
        dstat = demb_call_function(strbest->strbest_demb,
            strbest->strbest_demf_calc_opportunity_iota_best,
            &args, p_opiota);
        if (dstat) {
            (*p_estat) = set_error(ESTAT_CALL_BEST,"Error calling calc_opportunity_iota_best(): %s",
                demb_get_errmsg_ptr(strbest->strbest_demb));
        }
    }
}
/**************************************************************/
index_t calc_best_opportunity_index_best(
    struct gstate * opp_gst,
    struct opportunitylist * opl,
    struct deck * remdk,    /* not shuffled */
	card_t      * g_hand,
    count_t       g_nhand,
    count_t       opp_nhand,
    struct stratbest * strbest,
    count_t player,
    int * p_estat)
{
    index_t best_opix;
    int best_iota;
    int dstat;
    index_t opix;
    index_t trialix;
    struct opportunityitem * opi;
    struct deck playdk;
    int * tot_iota;
    count_t opponent;
    boolean_t hand_matches;
    int * opp_iota_list;
    int mintries;

    best_opix = 0;
    opponent = PLAYER_OPPONENT(player);

    tot_iota = New(int, opl->opl_nopi);
    memset(tot_iota, 0, sizeof(int) * opl->opl_nopi);

    opp_iota_list = New(int, opl->opl_nopi);

    if (strbest->strbest_shuf_rgen >= 0) {
        if (strbest->strbest_shuf_rg) {
            randintfree(strbest->strbest_shuf_rg);
            strbest->strbest_shuf_rseed++;
        }
        strbest->strbest_shuf_rg = randintseed(strbest->strbest_shuf_rgen, strbest->strbest_shuf_rseed);
        strbest->strbest_erg = strbest->strbest_shuf_rg;
    }

    mintries = strbest->strbest_mintries;
    if (opp_gst->gst_nturns >= FIRST_TURN_IN_LAST_ROUND) {
        mintries = 1;
    }

#if 0
    gstate_print(MSGLEVEL_TERSE, opp_gst,
        FLAG_PLAYER_EAST | FLAG_PLAYER_WEST | FLAG_BOARD | FLAG_BOARD_SHOW_OWNER |
        FLAG_PLAYER_TAKEN_EAST | FLAG_PLAYER_TAKEN_WEST | FLAG_PLAYER_DECK);
#endif

    for (trialix = 0; trialix < mintries; trialix++) {
        if (opp_nhand) {
            hand_matches = 0;
            while (!hand_matches) {
                CPARRAY(playdk.dk_card, playdk.dk_ncards, remdk->dk_card, remdk->dk_ncards);
                deck_shuffle(strbest->strbest_erg, &playdk);

                qdeal_cards(&playdk,
                    opp_gst->gst_hand[opponent],
                    &(opp_gst->gst_nhand[opponent]), opp_nhand);
                hand_matches = calc_card_read_matches_best(opp_gst, 
                    opp_gst->gst_board, opp_gst->gst_nboard,
                    opp_gst->gst_hand[opponent], opp_gst->gst_nhand[opponent],
                    /* strbest, */ opponent);
            }
        }

#if 0
        printf("======= calc_best_opportunity_index_best()\n");
        gstate_print(opp_gst, FLAG_PLAYER_EAST | FLAG_PLAYER_WEST | FLAG_BOARD | FLAG_BOARD_SHOW_OWNER);
            show_opportunities(
                    opl,
                    opp_gst->gst_board,
                    opp_gst->gst_nboard,
                    opp_gst->gst_hand[player],
                    opp_gst->gst_nhand[player],
                    player);
#endif
#if IS_DEBUG
    if (is_msglevel(MSGLEVEL_DEBUG_SHOWDEALS)) {
        msgout(MSGLEVEL_TERSE, "################################ Trial deal:\n");
        gstate_print(MSGLEVEL_TERSE, opp_gst,
            GET_PLAYER_FLAG(player) | FLAG_BOARD | FLAG_BOARD_SHOW_OWNER);
        show_cardlist(MSGLEVEL_TERSE, "  Deck: ", playdk.dk_card, playdk.dk_ncards);
    }
#endif

        for (opix = 0; !(*p_estat) && opix < opl->opl_nopi; opix++) {
            opi = opl->opl_opi[opix];
            calc_opportunity_iota_best_demi(opp_gst, opi, strbest, player, 0, &(opp_iota_list[opix]), p_estat);
        }

        if (strbest->strbest_demb) {
            dstat = demb_wait_all_funcs(strbest->strbest_demb, 0);
            if (dstat) {
                (*p_estat) = set_error(ESTAT_WAIT_BEST,"Error waiting on strategy best: %s",
                    demb_get_errmsg_ptr(strbest->strbest_demb));
            }
        }


        for (opix = 0; opix < opl->opl_nopi; opix++) {
            tot_iota[opix] += opp_iota_list[opix];
        }

        if (strbest->strbest_show_flag & STRBEST_SHOW_FLAG_SHOWEACHTOT) {
            strat_best_showtots(opp_gst, opl, opp_iota_list, player);
        }
    }

    best_iota = tot_iota[0];
    best_opix = 0;
    for (opix = 1; opix < opl->opl_nopi; opix++) {
        if (tot_iota[opix] > best_iota) {
            best_iota = tot_iota[opix];
            best_opix = opix;
        }
    }

    if (strbest->strbest_show_flag & STRBEST_SHOW_FLAG_SHOWTOTS) {
        opix = strat_best_showtots(opp_gst, opl, tot_iota, player);
        if (tot_iota[opix] != tot_iota[best_opix]) {
            msgout(MSGLEVEL_TERSE, "******************************** Best index=%d\n", best_opix);
            show_opportunity_iotas(opl, tot_iota, NULL,
                opp_gst->gst_board,
                opp_gst->gst_nboard,
                opp_gst->gst_hand[player],
                opp_gst->gst_nhand[player],
                player);
        }
    }

    Free(tot_iota);
    Free(opp_iota_list);

    return (best_opix);
}
/**************************************************************/
index_t find_opportunity_best(struct gstate * gst,
        struct opportunitylist * opl,
        struct stratbest * strbest,
        count_t player,
        int * p_estat)
{
    index_t best_opix;
    struct deck remdk;
    struct gstate opp_gst;

    create_remaining_deck_from_cards(gst, &remdk, player);

    calc_opponent_gstate(&opp_gst,
        gst->gst_hand[player], gst->gst_nhand[player],
        gst->gst_board, gst->gst_nboard, gst->gst_last_take,
        gst->gst_nturns,
        player);

    best_opix = calc_best_opportunity_index_best(&opp_gst, opl, &remdk,
        gst->gst_hand[player], gst->gst_nhand[player],
        gst->gst_nhand[PLAYER_OPPONENT(player)], strbest, player, p_estat);

    return (best_opix);
}
/**************************************************************/
int strat_best_declare_dtypes(struct demi_boss * demb)
{
/*
** 12/10/2019
*/
    int estat = 0;
    int dstat;

    dstat = 0;

    if (!dstat) dstat = demb_define_const(demb, "MAX_PLAYERS", "%d", MAX_PLAYERS);
    if (!dstat) dstat = demb_define_const(demb, "CARDS_PER_PLAYER", "%d", CARDS_PER_PLAYER);
    if (!dstat) dstat = demb_define_const(demb, "MAX_SLOTS_IN_BOARD", "%d", MAX_SLOTS_IN_BOARD);
    if (!dstat) dstat = demb_define_const(demb, "FIFTY_TWO", "%d", FIFTY_TWO);
    if (!dstat) dstat = demb_define_const(demb, "MAX_CARDS_IN_SLOT", "%d", MAX_CARDS_IN_SLOT);

    if (!dstat) dstat = demb_define_type(demb, "count_t", "int8", sizeof(count_t));
    if (!dstat) dstat = demb_define_type(demb, "point_t", "int16", sizeof(point_t));
    if (!dstat) dstat = demb_define_type(demb, "bitmask_t", "uint16", sizeof(bitmask_t));
    if (!dstat) dstat = demb_define_type(demb, "card_t", "int8", sizeof(card_t));

    if (!dstat) {
        dstat = demb_define_type(demb, "slot",
            "{"
                "sl_type  : count_t;"
                "sl_value : count_t;"
                "sl_owner : count_t;"
                "sl_ncard : count_t;"
                "sl_card  : card_t[MAX_CARDS_IN_SLOT];"
            "}", sizeof(struct slot));
    }

    if (!dstat) {
        dstat = demb_define_type(demb, "gstate",
            "{"
            "gst_hand        : card_t[MAX_PLAYERS][CARDS_PER_PLAYER];"
                "gst_nhand       : count_t[MAX_PLAYERS];"
                "gst_board       : slot[MAX_SLOTS_IN_BOARD];"
                "gst_last_take   : count_t;"
                "gst_nboard      : count_t;"
                "gst_nturns      : count_t;"
                "gst_flags       : count_t;"
                "gst_taken       : card_t[MAX_PLAYERS][FIFTY_TWO];"
                "gst_ntaken      : count_t[MAX_PLAYERS];"
            "}", sizeof(struct gstate));
    }

    if (!dstat) {
        dstat = demb_define_type(demb, "opportunityitem",
            "{"
                "opi_type        : count_t;"
                "opi_cardix      : count_t;"
                "opi_player      : count_t;"
                "opi_bdlvalue    : count_t;"
                "opi_slotmask    : bitmask_t;"
#if IS_DEBUG
                "opi_opiota        : int;"
#endif
            "}", sizeof(struct opportunityitem));
    }

    if (!dstat) {
        dstat = demb_define_type(demb, "calc_opportunity_iota_best_ARGS",
            "{"
                "gst    : *gstate;"
                "opi    : *opportunityitem;"
                "player : count_t;"
                "depth  : count_t;"
            "}", sizeof(struct calc_opportunity_iota_best_ARGS));
    }

    if (!dstat) {
        dstat = demb_define_type(demb, "calc_opportunity_iota_best_OUT",
            "{"
                "opiota  : int;"
            "}", sizeof(struct calc_opportunity_iota_best_OUT));
    }

    if (dstat) {
        estat = set_error(ESTAT_DEMI_SETUP,"Error initializing strategy best: %s",
            demb_get_errmsg_ptr(demb));
    }

    return (estat);
}
/**************************************************************/
int demi_calc_opportunity_iota_best(void * funcin, void * funcout)
{
/*
** 01/05/2020
*/
    int dstat;
    struct calc_opportunity_iota_best_ARGS * args;
    struct calc_opportunity_iota_best_OUT * frtn;

    dstat = 0;

    args = (struct calc_opportunity_iota_best_ARGS *)funcin;
    frtn = (struct calc_opportunity_iota_best_OUT *)funcout;

    frtn->opiota = calc_opportunity_iota_best(args->gst, args->opi, args->player, args->depth);

    return (dstat);
}
/**************************************************************/
int demi_register_callable_functions(void * vdemi,
    int (*register_func)(void * vd, const char * funcnam, DEMI_CALLABLE_FUNCTION func),
    const char * funccfg)
{
/*
** 01/05/2020
*/
    int dstat;

    dstat = register_func(vdemi, "calc_opportunity_iota_best", demi_calc_opportunity_iota_best);

#if DEMI_TESTING
    if (!dstat) {
        dstat = demi_test_register_callable_functions(vdemi, register_func, funccfg);
    }
#endif

    return (dstat);
}
/**************************************************************/
int strat_best_declare_funcs(struct demi_boss * demb,
    void ** vp_calc_opportunity_iota_best)
{
/*
** 01/04/2020
*/
    int estat = 0;
    int dstat;

    dstat = demb_define_func(demb,
        "calc_opportunity_iota_best",
        "calc_opportunity_iota_best_ARGS",
        "calc_opportunity_iota_best_OUT",
        vp_calc_opportunity_iota_best);

    return (estat);
}
/**************************************************************/
void continue_exit(void)
{
    char xbuf[80];

    printf("Continue...");
    fgets(xbuf, sizeof(xbuf), stdin);
    exit(1);
}
/**************************************************************/
int strat_start_best(struct stratbest * strbest)
{
/*
** 12/10/2019
*/
    int estat = 0;
    int dstat;

    if (!strbest->strbest_demb) return (0);

    dstat = demb_start(strbest->strbest_demb, NULL);
    if (dstat) {
        estat = set_error(ESTAT_START_BEST,"Error starting strategy best: %s",
            demb_get_errmsg_ptr(strbest->strbest_demb));
    }

#if DEMI_TESTING
    if (!estat) {
        estat = demi_test_all(strbest->strbest_demb);
    }
continue_exit();
#endif

    if (!estat) {
        estat = strat_best_declare_dtypes(strbest->strbest_demb);
    }

    if (!estat) {
        estat = strat_best_declare_funcs(strbest->strbest_demb,
            &(strbest->strbest_demf_calc_opportunity_iota_best));
    }

    return (estat);
}
/**************************************************************/
