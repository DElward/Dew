/**************************************************************/
/* play.c                                                     */
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
#include "strat.h"
#include "randnum.h"
#include "play.h"
#include "when.h"

#if COPT_TRAIL_INFO
static int gti_max_trail_slots          = 0;
static int gti_max_trail_opportunities  = 0;
#endif

/**************************************************************/
void slot_copy(struct slot * sl_tgt, struct slot * sl_src)
{
    sl_tgt->sl_type     = sl_src->sl_type;
    sl_tgt->sl_value    = sl_src->sl_value;
    sl_tgt->sl_owner    = sl_src->sl_owner;
    CPARRAY(sl_tgt->sl_card, sl_tgt->sl_ncard, sl_src->sl_card, sl_src->sl_ncard);
}
/**************************************************************/
void deck_copy(struct deck * dk_tgt, struct deck * dk_src)
{
    CPARRAY(dk_tgt->dk_card, dk_tgt->dk_ncards, dk_src->dk_card, dk_src->dk_ncards);
}
/**************************************************************/
void play_compact_slots(struct gstate * gst)
{
    count_t slotix_tgt;
    count_t slotix_src;

    slotix_tgt = 0;
    while (slotix_tgt < gst->gst_nboard && gst->gst_board[slotix_tgt].sl_ncard) {
        slotix_tgt++;
    }

    slotix_src = slotix_tgt + 1;
    while (slotix_src < gst->gst_nboard) {
        /* skip over slots not in use */
        while (slotix_src < gst->gst_nboard &&
                !gst->gst_board[slotix_src].sl_ncard) slotix_src++;
        /* copy slots in use */
        while (slotix_src < gst->gst_nboard &&
                gst->gst_board[slotix_src].sl_ncard) {
            slot_copy(&(gst->gst_board[slotix_tgt]), &(gst->gst_board[slotix_src]));
            slotix_tgt++;
            slotix_src++;
        }
    }
    gst->gst_nboard = slotix_tgt;
}
/**************************************************************/
void play_opportunity_trail(
        struct gstate * gst,
        struct opportunityitem * opi,
        count_t player)
{
    struct slot * sl;

    DESSERT(gst->gst_nboard < MAX_SLOTS_IN_BOARD, "Out of slots.");

    sl = &(gst->gst_board[gst->gst_nboard]);
    sl->sl_card[0] = gst->gst_hand[player][opi->opi_cardix];
    sl->sl_ncard   = 1;
    sl->sl_owner   = PLAYER_NOBODY;
    sl->sl_value   = GET_PIP(sl->sl_card[0]);
    sl->sl_type    = SLTYPE_SINGLE;
    gst->gst_nboard += 1;
}
/**************************************************************/
void play_opportunity_build(
        struct gstate * gst,
        struct opportunityitem * opi,
        count_t player,
        count_t bldtype)
{
    struct slot * sl_tgt;
    struct slot * sl_src;
    count_t slotix;
    bitmask_t slotmask;
    int ii;

    sl_tgt = NULL;
    sl_src = NULL;
    BITFOR(slotmask,opi->opi_slotmask,slotix) {
        if (!sl_tgt) {
            sl_tgt = &(gst->gst_board[slotix]);
            DESSERT(sl_tgt->sl_ncard < MAX_CARDS_IN_SLOT, "No room for build card played.");
            sl_tgt->sl_card[sl_tgt->sl_ncard] = gst->gst_hand[player][opi->opi_cardix];
            sl_tgt->sl_ncard   += 1;
            sl_tgt->sl_owner   = player;
            sl_tgt->sl_value   = opi->opi_bdlvalue;
            sl_tgt->sl_type    = bldtype;
        } else {
            sl_src = &(gst->gst_board[slotix]);
            DESSERT(sl_tgt->sl_ncard + sl_src->sl_ncard < MAX_CARDS_IN_SLOT, "No room for build cards.");
            for (ii = 0; ii < sl_src->sl_ncard; ii++) {
                sl_tgt->sl_card[sl_tgt->sl_ncard] = sl_src->sl_card[ii];
                sl_tgt->sl_ncard   += 1;
            }
            sl_src->sl_ncard = 0;
        }
    }
    if (sl_src) play_compact_slots(gst);
}
/**************************************************************/
void play_opportunity_take(
        struct gstate * gst,
        struct opportunityitem * opi,
        count_t player)
{
    struct slot * sl_src;
    count_t slotix;
    bitmask_t slotmask;
    int ii;

    gst->gst_taken[player][gst->gst_ntaken[player]] = gst->gst_hand[player][opi->opi_cardix];
    gst->gst_ntaken[player] += 1;
    gst->gst_last_take = player;

    BITFOR(slotmask,opi->opi_slotmask,slotix) {
        sl_src = &(gst->gst_board[slotix]);
        for (ii = 0; ii < sl_src->sl_ncard; ii++) {
            gst->gst_taken[player][gst->gst_ntaken[player]] = sl_src->sl_card[ii];
            gst->gst_ntaken[player] += 1;
        }
        sl_src->sl_ncard = 0;
    }
    play_compact_slots(gst);
}
/**************************************************************/
void last_take_gets_remaining_cards(struct gstate * gst)
{
    int slotix;
    int ii;
    struct slot * sl;
    count_t last_player;

    last_player = gst->gst_last_take;
    if (last_player != PLAYER_NOBODY) {
        for (slotix = 0; slotix < gst->gst_nboard; slotix++) {
            sl = &(gst->gst_board[slotix]);
            for (ii = 0; ii < sl->sl_ncard; ii++) {
                gst->gst_taken[last_player][gst->gst_ntaken[last_player]] = sl->sl_card[ii];
                gst->gst_ntaken[last_player] += 1;
            }
            sl->sl_ncard;
        }
        gst->gst_nboard = 0;
    }
}
/**************************************************************/
int check_build_integrity(struct gstate * gst, count_t player)
{
    int builds_ok;
    int slotix;
    int cardix;
    int found;
    struct slot * sl;

    builds_ok = 1;

    for (slotix = 0; builds_ok && slotix < gst->gst_nboard; slotix++) {
        sl = &(gst->gst_board[slotix]);
        if (sl->sl_owner == player) {
            found = 0;
            for (cardix = 0; !found && cardix < gst->gst_nhand[sl->sl_owner]; cardix++) {
                if (GET_PIP(gst->gst_hand[sl->sl_owner][cardix]) == sl->sl_value) {
                    found = 1;
                }
            }
            builds_ok = found;
        }
    }

    if (!builds_ok) {
        msgout(MSGLEVEL_QUIET, "******** Build problem *******\n");
        gstate_print(MSGLEVEL_QUIET, gst, FLAG_PLAYER_EAST | FLAG_PLAYER_WEST | FLAG_BOARD);
    }
    return (builds_ok);
}
/**************************************************************/
void play_opportunity(struct gstate * gst, struct opportunityitem * opi, count_t player)
{

    DESSERT(opi->opi_cardix < gst->gst_nhand[player], "Bad card index.");

    switch (opi->opi_type) {
        case OPTYP_TRAIL             :
            play_opportunity_trail(gst, opi, player);
            break;

        case OPTYP_TAKE              :
            play_opportunity_take(gst, opi, player);
            break;

        case OPTYP_BUILD_SINGLE      :
            play_opportunity_build(gst, opi, player, SLTYPE_BUILD);
            break;

        case OPTYP_BUILD_DOUBLE      :
            play_opportunity_build(gst, opi, player, SLTYPE_DOUBLED_BUILD);
            break;

        case OPTYP_BUILD_MORE_DOUBLE :
            play_opportunity_build(gst, opi, player, SLTYPE_DOUBLED_BUILD);
            break;

        default                      :
            DESSERT(1, "Bad opportunity type.");
            break;
    }

    /* delete card played from hand */
    gst->gst_nhand[player] -= 1;
    memmove(&(gst->gst_hand[player][opi->opi_cardix]),
            &(gst->gst_hand[player][opi->opi_cardix + 1]),
            sizeof(card_t) * (gst->gst_nhand[player] - opi->opi_cardix));

#if IS_DEBUG
    DESSERT(check_build_integrity(gst, player), "Build problem.");
#endif

    gst->gst_nturns += 1;
    if (gst->gst_nturns == LAST_TURN) {
        last_take_gets_remaining_cards(gst);
    }
}
/***************************************************************/
void show_opportunityitem_played(
    struct gstate * gst,
    struct opportunityitem * opi,
    count_t player,
    count_t siflags,
    count_t depth)
{
    char oppbuf[PBUF_SIZE];

    if (siflags & 1) {
        show_opportunityitem(oppbuf, opi, gst->gst_board, gst->gst_hand[player]);
        msgout(MSGLEVEL_TERSE, "%d. Before opportunity played by %s: %s\n",
            depth, PLAYER_NAME(player), oppbuf);
    }
    if (siflags & 2) {
        gstate_print(MSGLEVEL_TERSE, gst,
            FLAG_PLAYER_EAST | FLAG_PLAYER_WEST | FLAG_BOARD | FLAG_BOARD_SHOW_OWNER);
    }
}
/**************************************************************/
void create_remaining_deck_from_cards(struct gstate * gst, struct deck * remdk, count_t player)
{
    int ix;
    int ii;
    unsigned char cards[FIFTY_TWO];

    memset(cards, 0, sizeof(cards));

    /* use cards taken */
    for (ix = 0; ix < MAX_PLAYERS; ix++) {
        for (ii = 0; ii < gst->gst_ntaken[ix]; ii++) {
            cards[CARD_TO_INDEX(gst->gst_taken[ix][ii])] += 1;
        }
    }

    /* use cards on board */
    for (ix = 0; ix < gst->gst_nboard; ix++) {
        for (ii = 0; ii < gst->gst_board[ix].sl_ncard; ii++) {
            cards[CARD_TO_INDEX(gst->gst_board[ix].sl_card[ii])] += 1;
        }
    }

    /* use player's own cards */
    for (ii = 0; ii < gst->gst_nhand[player]; ii++) {
        cards[CARD_TO_INDEX(gst->gst_hand[player][ii])] += 1;
    }

    /* make deck using unseen cards */
    remdk->dk_ncards = 0;
    for (ii = 0; ii < FIFTY_TWO; ii++) {
        if (!cards[ii]) {
            remdk->dk_card[remdk->dk_ncards++] = INDEX_TO_CARD(ii);
        }
    }
}
/**************************************************************/
int find_opportunity_menu(struct gstate * gst, struct opportunitylist * opl, count_t player)
{
    int pflags;
    int done;
    int opix;
    int ilen;
    char inbuf[PBUF_SIZE];

    if (player == PLAYER_EAST) pflags = FLAG_PLAYER_EAST;
    else  pflags = FLAG_PLAYER_WEST;

    done = 0;
    while (!done) {
        msgout(MSGLEVEL_TERSE, "\n");
        gstate_print(MSGLEVEL_TERSE, gst, FLAG_BOARD);
        msgout(MSGLEVEL_TERSE, "\n");
        gstate_print(MSGLEVEL_TERSE, gst, pflags);
        msgout(MSGLEVEL_TERSE, "\n");
        show_opportunities(
            opl,
            gst->gst_board,
            gst->gst_nboard,
            gst->gst_hand[player],
            gst->gst_nhand[player],
            player);
        msgout(MSGLEVEL_TERSE, "Which?");
        fgets(inbuf, sizeof(inbuf), stdin);
        ilen = IStrlen(inbuf);
        while (ilen > 0 && isspace(inbuf[ilen-1])) ilen--;
        inbuf[ilen] = '\0';
        if (ilen) {
            if (!strcmp(inbuf, "//")) {
                exit(1);
            } else if (stoi(inbuf, &opix)) {
                msgout(MSGLEVEL_TERSE, "'%s' is not a valid number.\n", inbuf);
            } else if (opix < 1 || opix > opl->opl_nopi) {
                msgout(MSGLEVEL_TERSE, "'%s' is not a number between 1 and %d.\n",
                    inbuf, opl->opl_nopi);
            } else {
                done = 1;
            }
        }
    }

    return (opix-1);
}
/**************************************************************/
void show_opportunity_played(struct playinfo * pli,
            struct gstate * gst,
            struct opportunityitem * opi,
            count_t player)
{
/*
** ** Only get here when: pli->pli_msglevel <= MSGLEVEL_VERBOSE
*/
    char oppbuf[PBUF_SIZE];

    show_opportunityitem(oppbuf, opi,
        gst->gst_board, gst->gst_hand[player]);
    msgout(MSGLEVEL_DEFAULT, "Opportunity played by %s: %s\n", PLAYER_NAME(player), oppbuf);

    /* point_t opiota; */
    /* point_t calc_opportunity_iota_simple(struct gstate * gst, struct opportunityitem * opi, count_t player); */
    /* opiota = calc_opportunity_iota_simple(gst, opi, player); */
    /* printf("Opportunity iota = %d\n", (int)opiota); */
}
/**************************************************************/
point_t calc_card_iota(card_t card)
{
    point_t ciota;

    ciota = CARD_POINTS(card) * IOTA_PER_POINT + IOTA_PER_CARD;
    if (GET_SUIT(card) == SUIT_SPADES)  ciota += IOTA_PER_SPADE;

    return (ciota);
}
/**************************************************************/
void calc_opponent_gstate(
    struct gstate * opp_gst,
	card_t      * g_hand,
    count_t       g_nhand,
	struct slot * g_board,
    count_t       g_nboard,
    count_t       g_last_take,
    count_t       g_nturns,
    count_t player)
{
/*
** See also: gstate_init()
*/
    int ii;

    opp_gst->gst_nhand[player] = g_nhand;
    memcpy(opp_gst->gst_hand[player], g_hand, sizeof(card_t) * g_nhand);

    opp_gst->gst_nboard = g_nboard;
    for (ii = 0; ii < g_nboard; ii++) {
        slot_copy(&(opp_gst->gst_board[ii]), &(g_board[ii]));
    }

    opp_gst->gst_last_take = g_last_take;
    opp_gst->gst_nturns    = g_nturns;
    opp_gst->gst_flags     = 0;

    /* Do not use taken */
    opp_gst->gst_ntaken[player]   = 0;
    opp_gst->gst_ntaken[PLAYER_OPPONENT(player)] = 0;

    opp_gst->gst_nhand[PLAYER_OPPONENT(player)] = 0;
}
/**************************************************************/
/**************************************************************/
int make_play(struct playinfo * pli, struct gstate * gst, struct strategy * strat, count_t player)
{
    int estat = 0;
    int opix;
    struct opportunitylist opl;

    if (!gst->gst_nhand[player]) {
        estat = set_error(ESTAT_NOCARDS,"No cards for player %s", PLAYER_NAME(player));
    } else {
        init_opportunitylist(&opl);
        game_opportunities(
            &opl,
            gst->gst_board,
            gst->gst_nboard,
            gst->gst_hand[player],
            gst->gst_nhand[player],
            player);

        opix = find_opportunity(gst, &opl, strat, player, &estat);
    }

    if (!estat) {
#if COPT_TRAIL_INFO
    if (opl.opl_opi[opix]->opi_type == OPTYP_TRAIL &&
        opl.opl_opi[0]->opi_type    != OPTYP_TRAIL) {
        if (gst->gst_nboard > gti_max_trail_slots) {
            gti_max_trail_slots = gst->gst_nboard;
            msgout(MSGLEVEL_TERSE, "Maximum trail slots         = %d\n", gti_max_trail_slots);
        }
        if (opl.opl_nopi > gti_max_trail_opportunities) {
            gti_max_trail_opportunities = opl.opl_nopi;
            msgout(MSGLEVEL_TERSE, "Maximum trail opportunities = %d\n", gti_max_trail_opportunities);
        }
    }
#endif

        if (is_msglevel(MSGLEVEL_VERBOSE)) {
            gstate_print(MSGLEVEL_TERSE, gst,
                FLAG_PLAYER_EAST | FLAG_PLAYER_WEST | FLAG_BOARD | FLAG_BOARD_SHOW_OWNER);
            msgout(MSGLEVEL_VERBOSE, "%s: ", get_strategy_string(strat, 0));
        }
        if (is_msglevel(MSGLEVEL_DEFAULT)) {
            show_opportunity_played(pli, gst, opl.opl_opi[opix], player);
        }
        play_opportunity(gst, opl.opl_opi[opix], player);
    }
    free_opportunities(&opl);

    return (estat);
}
/**************************************************************/
void show_missing_cards(struct gstate * gst)
{
    int plix;
    int ii;
    count_t crd;
    count_t ncards;
    count_t cardlist[FIFTY_TWO];
    count_t cards[FIFTY_TWO];

    memset(cards, 0, sizeof(cards));

    for (plix = 0; plix < MAX_PLAYERS; plix++) {
        for (ii = 0; ii < gst->gst_ntaken[plix]; ii++) {
            crd = gst->gst_taken[plix][ii] - 4;
            if (crd < 0 || crd > 51) {
                msgout(MSGLEVEL_TERSE, "Card out of range: %d\n", (int)crd);
            } else {
                cards[crd] += 1;
            }
        }
    }

    ncards = 0;
    for (ii = 0; ii < FIFTY_TWO; ii++) {
        if (!cards[ii]) {
            cardlist[ncards++] = (ii + 4);
        }
    }
    show_cardlist(MSGLEVEL_TERSE, "Missing cards: ", cardlist, ncards);

    ncards = 0;
    for (ii = 0; ii < FIFTY_TWO; ii++) {
        if (cards[ii] > 1) {
            cardlist[ncards++] = (ii + 4);
        }
    }
    if (ncards) {
        show_cardlist(MSGLEVEL_TERSE, "Extra cards: ", cardlist, ncards);
    }
}
/**************************************************************/
void calc_player_scores(struct gstate * gst, count_t * player_score)
{
    int plix;
    int ii;
    count_t tot;
    count_t nsp;
    count_t nspades[MAX_PLAYERS];

    if (gst->gst_ntaken[0] + gst->gst_ntaken[1] != FIFTY_TWO) {
        show_missing_cards(gst);
    }
    DESSERT(gst->gst_ntaken[0] + gst->gst_ntaken[1] == FIFTY_TWO, "52 cards not taken.");

    for (plix = 0; plix < MAX_PLAYERS; plix++) {
        tot = 0;
        nsp = 0;
        for (ii = 0; ii < gst->gst_ntaken[plix]; ii++) {
            if (GET_SUIT(gst->gst_taken[plix][ii]) == SUIT_SPADES) nsp++;
            tot += CARD_POINTS(gst->gst_taken[plix][ii]);
        }
        nspades[plix] = nsp;
        player_score[plix] = tot;
    }
    DESSERT(nspades[0] + nspades[1] == THIRTEEN, "13 spades not taken.");

    /* **** Only works for two players */
    if (nspades[PLAYER_EAST] > nspades[PLAYER_WEST]) {
        player_score[PLAYER_EAST] += 1;
    } else {
        player_score[PLAYER_WEST] += 1;
    }

    if (gst->gst_ntaken[PLAYER_EAST] > gst->gst_ntaken[PLAYER_WEST]) {
        player_score[PLAYER_EAST] += 3;
    } else if (gst->gst_ntaken[PLAYER_EAST] < gst->gst_ntaken[PLAYER_WEST]) {
        player_score[PLAYER_WEST] += 3;
    }
}
/**************************************************************/
void increment_game_scores(struct gstate * gst, struct gscore * gsc)
{
    count_t player_score[MAX_PLAYERS];
    count_t player;
    int diff;

    calc_player_scores(gst, player_score);
    for (player = 0; player < MAX_PLAYERS; player++) {
        gsc->gsc_tot_points[player] += player_score[player];
        diff = player_score[player] - player_score[PLAYER_OPPONENT(player)];
        if (diff > 0)       gsc->gsc_games_won[player]   += 1;
        else if (diff < 0)  gsc->gsc_games_lost[player]  += 1;
        else                gsc->gsc_games_tied[player]  += 1;
    }
}
/**************************************************************/
#if COPT_TRAIL_INFO
void print_trail_info(void)
{
    extern void print_trail_opportunity_info(void);

    msgout(MSGLEVEL_TERSE, "Final maximum trail slots           = %d\n", gti_max_trail_slots);
    msgout(MSGLEVEL_TERSE, "Final maximum trail opportunities   = %d\n", gti_max_trail_opportunities);

    print_trail_opportunity_info();
}
#endif
/**************************************************************/
void print_game_scores(struct playinfo * pli, struct gscore * gsc)
{
    count_t player;
    char stratbuf[80];

    msgout(MSGLEVEL_TERSE, "%6s %-16s %6s %5s %5s %5s\n",
            "Player", "Strategy", "Points", "W", "L", "T");
    for (player = 0; player < MAX_PLAYERS; player++) {
        print_strategy(stratbuf, sizeof(stratbuf), pli->pli_strat[player]);
        msgout(MSGLEVEL_TERSE, "%6s %-16s %6d %5d %5d %5d\n",
                PLAYER_NAME(player),
                stratbuf,
                gsc->gsc_tot_points[player],
                gsc->gsc_games_won[player],
                gsc->gsc_games_lost[player],
                gsc->gsc_games_tied[player]);
    }

#if COPT_TRAIL_INFO
    print_trail_info();
#endif
}
/**************************************************************/
int game_shuffle_and_deal(struct game * gm) {

    int estat;

    deck_reset_shuffle(gm->gm_erg, gm->gm_dk);
    estat = game_deal_first_round(gm);

    return (estat);
}
/**************************************************************/
int play_duration_turn(struct game * gm,
            struct playinfo * pli,
            int nturns)
{
    int estat = 0;
    int nump;
    count_t player;
    count_t player_score[MAX_PLAYERS];

    player = pli->pli_player;

    if (!gm->gm_dk->dk_ncards) {
        if (gm->gm_gst.gst_nhand[player] == 0) {
            estat = game_shuffle_and_deal(gm);
        }
    } else if (gm->gm_dk->dk_ncards == FIFTY_TWO) {
        estat = game_deal_first_round(gm);
    }

    for (nump = 0; !estat && nump < nturns; nump++) {
        if (gm->gm_gst.gst_nhand[player] == 0) {
            msgout(MSGLEVEL_VERBOSE, "\n");
            estat = game_deal_next_round(gm);
            if (estat) return (estat);
        }
        estat = make_play(pli, &(gm->gm_gst), pli->pli_strat[player], player);
        player = PLAYER_OPPONENT(player);
    }

    if (!estat) {
        if (gm->gm_round == GAME_OVER_ROUND) {
            calc_player_scores(&(gm->gm_gst), player_score);
            msgout(MSGLEVEL_DEFAULT, "%s score %d\n",
                PLAYER_NAME(PLAYER_EAST), player_score[PLAYER_EAST]);
            msgout(MSGLEVEL_DEFAULT, "%s score %d\n",
                PLAYER_NAME(PLAYER_WEST), player_score[PLAYER_WEST]);
        } else {
            if (is_msglevel(MSGLEVEL_VERBOSE)) {
                gstate_print(MSGLEVEL_TERSE, &(gm->gm_gst),
                    FLAG_PLAYER_TAKEN_EAST | FLAG_PLAYER_TAKEN_WEST);
            }
        }
    }

    return (estat);
}
/**************************************************************/
int play_games(struct game * gm,
            struct playinfo * pli,
            int ngames,
            struct gscore * gsc)
{
    int estat = 0;
    int gnum;
    count_t player;

    for (player = 0; player < MAX_PLAYERS; player++) {
        estat = strat_match_begin(gm, pli->pli_strat[player]);
    }

    for (gnum = 0; !estat && gnum < ngames; gnum++) {
        msgout(MSGLEVEL_DEFAULT, "**** Starting game %d\n", gnum + 1);
        if (gnum) {
            estat = game_shuffle_and_deal(gm);
        }
        /* gstate_print(MSGLEVEL_VERBOSE, &(gm->gm_gst),            */
        /*    FLAG_PLAYER_EAST | FLAG_PLAYER_WEST | FLAG_BOARD);    */
        show_cardlist(MSGLEVEL_DEBUG, "  Deck: ", gm->gm_dk->dk_card, gm->gm_dk->dk_ncards);

        player = PLAYER_OPPONENT(gm->gm_dealer);

        while (!estat && gm->gm_round <= LAST_ROUND) {
            while (!estat && gm->gm_gst.gst_nhand[player]) {
                estat = make_play(pli, &(gm->gm_gst), pli->pli_strat[player], player);
                player = PLAYER_OPPONENT(player);
            }
            if (!estat && gm->gm_round < LAST_ROUND) {
                estat = game_deal_next_round(gm);
                msgout(MSGLEVEL_VERBOSE, "\n");
            } else {
                gm->gm_round = GAME_OVER_ROUND;
            }
        }

        if (!estat && gm->gm_dk->dk_ncards == 0) {
            if (is_msglevel(MSGLEVEL_DEBUG)) {
                gstate_print(MSGLEVEL_DEBUG, &(gm->gm_gst),
                    FLAG_PLAYER_EAST | FLAG_PLAYER_WEST | FLAG_BOARD |
                    FLAG_PLAYER_TAKEN_EAST | FLAG_PLAYER_TAKEN_WEST);
            }

            increment_game_scores(&(gm->gm_gst), gsc);
        }
        gm->gm_dealer = PLAYER_OPPONENT(gm->gm_dealer);
    }

    if (!estat) {
        for (player = 0; player < MAX_PLAYERS; player++) {
            estat = strat_match_end(gm, pli->pli_strat[player], gsc, player);
        }
    }

    return (estat);
}
/**************************************************************/
int play_duration_game(struct game * gm,
            struct playinfo * pli,
            int ngames)
{
    int estat = 0;
    struct gscore gsc;
    struct proctimes pt_start ;     /* 12/05/2019 */
    struct proctimes pt_end   ;     /* 12/05/2019 */
    struct proctimes pt_result;     /* 12/05/2019 */
   
    start_cpu_timer(&pt_start, 0);    /* 12/05/2019 */

    memset(&gsc, 0, sizeof(struct gscore));

    gm->gm_dealer = pli->pli_player;

    if (!gm->gm_dk->dk_ncards) {
        estat = game_shuffle_and_deal(gm);
    } else if (gm->gm_dk->dk_ncards == FIFTY_TWO) {
        estat = game_deal_first_round(gm);
    }

    if (!estat) {
        estat = play_games(gm, pli, ngames, &gsc);
    }
    stop_cpu_timer(&pt_start, &pt_end, &pt_result);    /* 12/05/2019 */

    print_game_scores(pli, &gsc);

    printf("Elapsed time for %d games: %d.%03d seconds\n",
        ngames, pt_result.pt_wall_time.ts_second, pt_result.pt_wall_time.ts_microsec / 1000);    /* 12/05/2019 */

    return (estat);
}
/**************************************************************/
int play_duration_game_match(struct game * gm,
            struct playinfo * pli,
            int ngames_in_match,
            int nmatches)
{
    int estat = 0;
    int mnum;
    count_t player;
    struct gscore gsc;

    for (player = 0; player < MAX_PLAYERS; player++) {
        estat = strat_match_init(gm, pli->pli_strat[player]);
    }

    gm->gm_dealer = pli->pli_player;

    if (!gm->gm_dk->dk_ncards) {
        estat = game_shuffle_and_deal(gm);
    } else if (gm->gm_dk->dk_ncards == FIFTY_TWO) {
        estat = game_deal_first_round(gm);
    }

    for (mnum = 0; !estat && mnum < nmatches; mnum++) {
        memset(&gsc, 0, sizeof(struct gscore));
        estat = play_games(gm, pli, ngames_in_match, &gsc);
    }

    for (player = 0; !estat &&player < MAX_PLAYERS; player++) {
        estat = strat_match_shut(gm, pli->pli_strat[player]);
    }

    return (estat);
}
/**************************************************************/
int play_play(struct game * gm, struct playinfo * pli)
{
    int estat = 0;
    int num;
    count_t player;

    for (player = 0; player < MAX_PLAYERS; player++) {
        estat = strat_play_init(gm, pli, pli->pli_strat[player]);
    }

    switch (pli->pli_play_duration) {
        case PLAY_DURATION_TURN:
            num = pli->pli_play_num_duration;
            estat = play_duration_turn(gm, pli, num);
            break;
        case PLAY_DURATION_ROUND:
            num = 8 * pli->pli_play_num_duration;
            if (gm->gm_gst.gst_nhand[PLAYER_EAST] || gm->gm_gst.gst_nhand[PLAYER_WEST]) {
                num -= 8;
                num += gm->gm_gst.gst_nhand[PLAYER_EAST];
                num += gm->gm_gst.gst_nhand[PLAYER_WEST];
            }
            estat = play_duration_turn(gm, pli, num);
            break;
        case PLAY_DURATION_GAME:
            estat = play_duration_game(gm, pli, pli->pli_play_num_duration);
            break;
        case PLAY_DURATION_GAME_MATCH:
            estat = play_duration_game_match(gm, pli,
                pli->pli_play_num_duration,  pli->pli_play_matches);
            break;
        default:
            estat = set_error(ESTAT_UNSUPPDUR,"Unsupported duration: %d", pli->pli_play_duration);
            break;
    }

    for (player = 0; !estat &&player < MAX_PLAYERS; player++) {
        estat = strat_play_shut(gm, pli->pli_strat[player]);
    }

    return (estat);
}
/**************************************************************/
