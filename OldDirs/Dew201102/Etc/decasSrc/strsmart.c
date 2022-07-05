/**************************************************************/
/* strsmart.c                                                 */
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
#include "strsmart.h"
#include "play.h"

/***************************************************************/
int strat_play_smart_init(struct game * gm, struct playinfo * pli, struct stratsmart * strsmart)
{
/*
** Called once for smart strategy before any play has begun
*/
    int estat = 0;

    return (estat);
}
/**************************************************************/
point_t calc_take_opportunity_iota(
    struct gstate * gst,
    struct opportunityitem * opi,
    count_t player)
{
    point_t opiota;
    card_t card_played;
    bitmask_t slotmask;
    count_t slotix;
    int ii;

    card_played = gst->gst_hand[player][opi->opi_cardix];
    opiota = 0;

    BITFOR(slotmask,opi->opi_slotmask,slotix) {
        for (ii = 0; ii < gst->gst_board[slotix].sl_ncard; ii++) {
            opiota += calc_card_iota(gst->gst_board[slotix].sl_card[ii]);
        }
    }
    opiota += calc_card_iota(card_played);

    return (opiota);
}
/**************************************************************/
point_t calc_build_opportunity_iota(
    struct gstate * gst,
    struct opportunityitem * opi,
    count_t player)
{
    point_t opiota;
    card_t card_played;
    bitmask_t slotmask;
    count_t slotix;
    int ii;

    card_played = gst->gst_hand[player][opi->opi_cardix];
    opiota = 0;

    BITFOR(slotmask,opi->opi_slotmask,slotix) {
        for (ii = 0; ii < gst->gst_board[slotix].sl_ncard; ii++) {
            opiota += calc_card_iota(gst->gst_board[slotix].sl_card[ii]);
        }
    }
    opiota += calc_card_iota(card_played);

    return (opiota);
}
/**************************************************************/
point_t calc_opponent_opportunityitem_iota_smart(
    struct gstate * gst,
    struct opportunityitem * opi,
    point_t opponent_hand_bits,
    point_t avoid_bits,
    struct stratsmart * strsmart,
    count_t player)
{
    point_t opiota;
    count_t pip;
    card_t card_played;

    opiota = 0;
    card_played = gst->gst_hand[player][opi->opi_cardix];
    pip = GET_PIP(card_played);

    switch (opi->opi_type) {
        case OPTYP_TRAIL             :
            if (pip & (avoid_bits | opponent_hand_bits)) {
                opiota = - calc_card_iota(card_played);
            }
            break;

        case OPTYP_TAKE              :
            opiota = calc_take_opportunity_iota(gst, opi, player);
            break;

        case OPTYP_BUILD_SINGLE      :
            if (pip & (avoid_bits | opponent_hand_bits)) {
                opiota = - calc_card_iota(card_played);
            } else {
                opiota = calc_take_opportunity_iota(gst, opi, player);
            }
            break;

        case OPTYP_BUILD_DOUBLE      :
        case OPTYP_BUILD_MORE_DOUBLE :
            if (pip & opponent_hand_bits) {
                opiota = - calc_card_iota(card_played);
            } else {
                opiota = calc_take_opportunity_iota(gst, opi, player);
            }
            break;

        default                      :
            DESSERT(1, "Bad opportunity type.");
            break;
    }

    return (opiota);
}
/**************************************************************/
index_t calc_opponent_opportunity_smart(
    struct gstate * gst,
    struct opportunitylist * opl,
    point_t opponent_hand_bits,
    point_t avoid_bits,
    struct stratsmart * strsmart,
    count_t player,
    point_t * best_iota)
{
    point_t opiota;
    index_t opix;
    index_t best_opix;

    opiota = 0;
    (*best_iota) = 0;

    for (opix = 0; opix < opl->opl_nopi; opix++) {
        opiota = calc_opponent_opportunityitem_iota_smart(gst,
            opl->opl_opi[opix], opponent_hand_bits, avoid_bits, strsmart, player);
        if (!opix || (opiota > (*best_iota))) {
            (*best_iota) = opiota;
            best_opix = opix;
        }
    }

    return (best_opix);
}
/**************************************************************/
boolean_t player_has_card(
    card_t card_pip,
	card_t      * g_hand,
    count_t       g_nhand)
{
    boolean_t has_card;
    count_t cardix;

    has_card = 0;
    for (cardix = 0; !has_card && cardix < g_nhand; cardix++) {
        if (card_pip == GET_PIP(g_hand[cardix])) has_card = 1;
    }

    return (has_card);
}
/**************************************************************/
#if 0
index_t calc_best_opportunity_index_build_smart(
    struct gstate * gst,
    struct gstate * opp_gst,
    struct opportunitylist * opl,
    struct deck * remdk,
    struct deck * opponent_build_dk,
    struct stratsmart * strsmart,
    count_t player)
{
    index_t best_opix;
    index_t opix;
    point_t opiota;
    point_t build_card_iota;
	count_t build_value;
    point_t opp_iota;
    count_t opponent;
    count_t remix0;
    count_t opp_nhand;
    int total_opponent_iota;
    int opportunity_iota;
    int opportunity_iota_best;
    point_t hand_bits;
    point_t avoid_bits;
    count_t pip;
    count_t cardix;
    count_t val;
    count_t slotix;
    count_t opbdkix;
    card_t card_played;
    struct opportunityitem * opi;
    struct gstate work_gst;
    struct opportunitylist opp_opl;

    best_opix = 0;
    opportunity_iota_best = 0;
    opportunity_iota = 0;
    build_value = 0;
    opponent = PLAYER_OPPONENT(player);
    opp_nhand = gst->gst_nhand[opponent] - 1;

    for (opix = 0; opix < opl->opl_nopi; opix++) {
        opi = opl->opl_opi[opix];
        opiota = 0;
        build_card_iota = 0;
        card_played = gst->gst_hand[player][opi->opi_cardix];

        calc_opponent_gstate(&work_gst,
            gst->gst_hand[player], gst->gst_nhand[player],
            gst->gst_board, gst->gst_nboard, gst->gst_last_take,
            gst->gst_nturns,
            player);

        switch (opi->opi_type) {
            case OPTYP_TRAIL             :
                break;

            case OPTYP_TAKE              :
                opiota = calc_take_opportunity_iota(&work_gst, opi, player);
                break;

            case OPTYP_BUILD_SINGLE      :
            case OPTYP_BUILD_DOUBLE      :
            case OPTYP_BUILD_MORE_DOUBLE :
                build_value = opi->opi_bdlvalue;
                opiota = calc_build_opportunity_iota(&work_gst, opi, player);
                for (cardix = 0; cardix < work_gst.gst_nhand[player]; cardix++) {
                    if (GET_PIP(work_gst.gst_hand[player][cardix]) == build_value) {
                        build_card_iota += calc_card_iota(work_gst.gst_hand[player][cardix]);
                    }
                }
                break;

            default                      :
                DESSERT(1, "Bad opportunity type.");
                break;
        }

        play_opportunity(&work_gst, opi, player);

        hand_bits = 0;
        avoid_bits = 0;
        for (cardix = 0; cardix < work_gst.gst_nhand[player]; cardix++) {
            pip = GET_PIP(work_gst.gst_hand[player][cardix]);
            hand_bits |= (1 << pip);
            if (pip <= TEN) {
                for (slotix = 0; slotix < work_gst.gst_nboard; slotix++) {
                    val = work_gst.gst_board[slotix].sl_value;
                    if ((pip + val) <= TEN &&
                        work_gst.gst_board[slotix].sl_type != SLTYPE_DOUBLED_BUILD) {
                        avoid_bits |= (1 << (pip + val));
                    }
                }
            }
        }

        total_opponent_iota = 0;
        for (opbdkix = 0; !opbdkix || opbdkix < opponent_build_dk->dk_ncards; opbdkix++) {
            work_gst.gst_hand[opponent][0] = opponent_build_dk->dk_card[opbdkix];
            work_gst.gst_nhand[opponent] = 1;

            switch (opp_nhand) {
                case 0:
                    break;
                case 1:
                    init_opportunitylist(&opp_opl);
                    game_opportunities(
                        &opp_opl,
                        work_gst.gst_board,
                        work_gst.gst_nboard,
                        work_gst.gst_hand[opponent],
                        work_gst.gst_nhand[opponent],
                        opponent);
                    opp_iota = (*best_iota)(
                        &work_gst,
                        &opp_opl,
                        hand_bits,
                        avoid_bits,
                        strsmart,
                        opponent);
                    total_opponent_iota = opp_iota;
                    opportunity_iota += opiota;
                    break;
                case 2:
                case 3:
                case 4:
                    opportunity_iota = 0;
                    for (remix0 = 0; remix0 < remdk->dk_ncards; remix0++) {
                        work_gst.gst_hand[opponent][1] = remdk->dk_card[remix0];
                        work_gst.gst_nhand[opponent] = 2;
                        init_opportunitylist(&opp_opl);
                        game_opportunities(
                            &opp_opl,
                            work_gst.gst_board,
                            work_gst.gst_nboard,
                            work_gst.gst_hand[opponent],
                            work_gst.gst_nhand[opponent],
                            opponent);
                        opp_iota = (*best_iota)(
                            &work_gst,
                            &opp_opl,
                            hand_bits,
                            avoid_bits,
                            strsmart,
                            opponent);
                        total_opponent_iota = opp_iota;
                        opportunity_iota += opiota;
                    }
                    break;
                default:
                    /* should not happen */
                    break;
            }

            switch (opi->opi_type) {
                case OPTYP_TRAIL             :
                    break;

                case OPTYP_TAKE              :
                    break;

                case OPTYP_BUILD_SINGLE      :
                case OPTYP_BUILD_DOUBLE      :
                case OPTYP_BUILD_MORE_DOUBLE :
                    if (player_has_card(build_value,
                            work_gst.gst_hand[opponent],
                            work_gst.gst_nhand[opponent])) {                                
                        opportunity_iota -= (opiota + opiota);
                    } else {
                        opportunity_iota += opiota + build_card_iota;
                    }
                    break;

                default                      :
                    DESSERT(1, "Bad opportunity type.");
                    break;
            }
        }

#if IS_DEBUG
	    opl->opl_opi[opix]->opi_opiota = opportunity_iota;
#endif

        if (!opix || (opportunity_iota > opportunity_iota_best)) {
            opportunity_iota_best = opportunity_iota;
            best_opix = opix;
        }
    }

    return (best_opix);
}
#endif
/**************************************************************/
point_t check_best_opponent_opportunity_smart(
            struct gstate * work_gst,
            struct opportunitylist * opp_opl,
            point_t opponent_hand_bits,
            point_t avoid_bits,
            struct stratsmart * strsmart,
            count_t opponent)
{
/*
** 12/01/2019
*/
    index_t best_opp_opix;
    point_t best_opp_iota;

    init_opportunitylist(opp_opl);
    game_opportunities(
        opp_opl,
        work_gst->gst_board,
        work_gst->gst_nboard,
        work_gst->gst_hand[opponent],
        work_gst->gst_nhand[opponent],
        opponent);
    best_opp_opix = calc_opponent_opportunity_smart(
        work_gst,
        opp_opl,
        opponent_hand_bits,
        avoid_bits,
        strsmart,
        opponent,
        &best_opp_iota);

    return (best_opp_iota);
}
/**************************************************************/
boolean_t check_build_for_opponents_card(
            struct gstate * work_gst,
            struct opportunityitem * opi,
	        count_t build_value,
            count_t opponent)
{
/*
** 12/01/2019
*/
    boolean_t opp_has_build_card;

    opp_has_build_card = 0;

    switch (opi->opi_type) {
        case OPTYP_TRAIL             :
            break;

        case OPTYP_TAKE              :
            break;

        case OPTYP_BUILD_SINGLE      :
        case OPTYP_BUILD_DOUBLE      :
        case OPTYP_BUILD_MORE_DOUBLE :
            opp_has_build_card = player_has_card(build_value,
                    work_gst->gst_hand[opponent],
                    work_gst->gst_nhand[opponent]);
            break;

        default                      :
            DESSERT(1, "Bad opportunity type.");
            break;
    }

    return (opp_has_build_card);
}
/**************************************************************/
index_t calc_best_opportunity_index_build_smart(
    struct gstate * gst,
    struct gstate * opp_gst,
    struct opportunitylist * opl,
    struct deck * remdk,
    struct deck * opponent_build_dk,
    count_t * opponent_builds, /* 11/30/2019 */
    struct stratsmart * strsmart,
    count_t player)
{
    index_t best_opix;
    index_t opix;
    point_t opiota;
    point_t build_card_iota;
	count_t build_value;
    point_t best_opp_iota;
    count_t opponent;
    count_t remix0;
    int opportunity_iota;
    int opportunity_iota_best;
    point_t hand_bits;
    point_t avoid_bits;
    count_t pip;
    count_t cardix;
    count_t val;
    count_t slotix;
    count_t opbdkix0;
    count_t opbdkix1;
    card_t card_played;
    boolean_t opp_has_build_card;
    struct opportunityitem * opi;
    struct gstate work_gst;
    struct opportunitylist opp_opl;

    best_opix = 0;
    opportunity_iota_best = 0;
    opportunity_iota = 0;
    build_value = 0;
    build_card_iota = 0;
    opponent = PLAYER_OPPONENT(player);

    for (opix = 0; opix < opl->opl_nopi; opix++) {
        opi = opl->opl_opi[opix];

        switch (opi->opi_type) {
            case OPTYP_TRAIL             :
                opiota = 0;
                break;

            case OPTYP_TAKE              :
                opiota = calc_take_opportunity_iota(gst, opi, player);
                break;

            case OPTYP_BUILD_SINGLE      :
            case OPTYP_BUILD_DOUBLE      :
            case OPTYP_BUILD_MORE_DOUBLE :
                build_value = opi->opi_bdlvalue;
                opiota = calc_build_opportunity_iota(gst, opi, player);
                for (cardix = 0; cardix < gst->gst_nhand[player]; cardix++) {
                    if (GET_PIP(gst->gst_hand[player][cardix]) == build_value) {
                        build_card_iota += calc_card_iota(gst->gst_hand[player][cardix]);
                    }
                }
                break;

            default                      :
                DESSERT(1, "Bad opportunity type.");
                break;
        }

        if (gst->gst_nhand[opponent]) {
            card_played = gst->gst_hand[player][opi->opi_cardix];

            calc_opponent_gstate(&work_gst,
                gst->gst_hand[player], gst->gst_nhand[player],
                gst->gst_board, gst->gst_nboard, gst->gst_last_take,
                gst->gst_nturns,
                player);

            play_opportunity(&work_gst, opi, player);

            hand_bits = 0;
            avoid_bits = 0;
            for (cardix = 0; cardix < work_gst.gst_nhand[player]; cardix++) {
                pip = GET_PIP(work_gst.gst_hand[player][cardix]);
                hand_bits |= (1 << pip);
                if (pip <= TEN) {
                    for (slotix = 0; slotix < work_gst.gst_nboard; slotix++) {
                        val = work_gst.gst_board[slotix].sl_value;
                        if ((pip + val) <= TEN &&
                            work_gst.gst_board[slotix].sl_type != SLTYPE_DOUBLED_BUILD) {
                            avoid_bits |= (1 << (pip + val));
                        }
                    }
                }
            }

            opportunity_iota = 0;

            if (gst->gst_nhand[opponent] == 1) {
                for (opbdkix0 = 0; opbdkix0 < opponent_build_dk->dk_ncards; opbdkix0++) {
                    work_gst.gst_hand[opponent][0] = opponent_build_dk->dk_card[opbdkix0];
                    work_gst.gst_nhand[opponent] = 1;

                    best_opp_iota = check_best_opponent_opportunity_smart(
                        &work_gst,
                        &opp_opl,
                        hand_bits,
                        avoid_bits,
                        strsmart,
                        opponent);
                    opportunity_iota += opiota - best_opp_iota;
                }
            } else if (opponent_builds[1] == 0) {
                for (opbdkix0 = 0; opbdkix0 < opponent_build_dk->dk_ncards; opbdkix0++) {
                    work_gst.gst_hand[opponent][0] = opponent_build_dk->dk_card[opbdkix0];
                    work_gst.gst_nhand[opponent] = 1;

                    for (remix0 = 0; remix0 < remdk->dk_ncards; remix0++) {
                        work_gst.gst_hand[opponent][1] = remdk->dk_card[remix0];
                        work_gst.gst_nhand[opponent] = 2;

                        best_opp_iota = check_best_opponent_opportunity_smart(
                            &work_gst,
                            &opp_opl,
                            hand_bits,
                            avoid_bits,
                            strsmart,
                            opponent);
                        opportunity_iota += opiota - best_opp_iota;
                        opp_has_build_card = check_build_for_opponents_card(
                                    &work_gst,
                                    opi,
	                                build_value,
                                    opponent);
                        if (opp_has_build_card) {                                
                            opportunity_iota -= (opiota + opiota);
                        } else {
                            opportunity_iota += opiota + build_card_iota;
                        }
                    }
                }
            } else {    /* 12/01/2019 - opponent_builds[1] != 0, opponent has 2 builds */
                for (opbdkix0 = 0; opbdkix0 < opponent_build_dk->dk_ncards; opbdkix0++) {
                    if (GET_PIP(opponent_build_dk->dk_card[opbdkix0]) == opponent_builds[0]) {
                        work_gst.gst_hand[opponent][0] = opponent_build_dk->dk_card[opbdkix0];
                        work_gst.gst_nhand[opponent] = 1;

                        for (opbdkix1 = 0; opbdkix1 < opponent_build_dk->dk_ncards; opbdkix1++) {
                            if (GET_PIP(opponent_build_dk->dk_card[opbdkix1]) == opponent_builds[1]) {
                                /* if there are 2 builds, opponent has 2 cards */
                                work_gst.gst_hand[opponent][1] = opponent_build_dk->dk_card[opbdkix1];
                                work_gst.gst_nhand[opponent] = 2;

                                best_opp_iota = check_best_opponent_opportunity_smart(
                                    &work_gst,
                                    &opp_opl,
                                    hand_bits,
                                    avoid_bits,
                                    strsmart,
                                    opponent);
                                opportunity_iota += opiota - best_opp_iota;
                                opp_has_build_card = check_build_for_opponents_card(
                                            &work_gst,
                                            opi,
	                                        build_value,
                                            opponent);
                                if (opp_has_build_card) {                                
                                    opportunity_iota -= (opiota + opiota);
                                } else {
                                    opportunity_iota += opiota + build_card_iota;
                                }
                            }
                        }
                    }
                }
            }
        }

#if IS_DEBUG
	    opl->opl_opi[opix]->opi_opiota = opportunity_iota;
#endif

        if (!opix || (opportunity_iota > opportunity_iota_best)) {
            opportunity_iota_best = opportunity_iota;
            best_opix = opix;
        }
    }


    return (best_opix);
}
/**************************************************************/
index_t calc_best_opportunity_index_smart(
    struct gstate * gst,
    struct gstate * opp_gst,
    struct opportunitylist * opl,
    struct deck * remdk,
    struct deck * opponent_build_dk,
    struct stratsmart * strsmart,
    count_t player)
{
    index_t best_opix;
    index_t opix;
    point_t opiota;
    point_t build_card_iota;
	count_t build_value;
    point_t best_opp_iota;
    count_t opponent;
    count_t remix0;
    int opportunity_iota;
    int opportunity_iota_best;
    point_t hand_bits;
    point_t avoid_bits;
    count_t pip;
    count_t cardix;
    count_t val;
    count_t slotix;
    card_t card_played;
    boolean_t opp_has_build_card;
    struct opportunityitem * opi;
    struct gstate work_gst;
    struct opportunitylist opp_opl;

    best_opix = 0;
    opportunity_iota_best = 0;
    opportunity_iota = 0;
    build_value = 0;
    build_card_iota = 0;
    opponent = PLAYER_OPPONENT(player);

    for (opix = 0; opix < opl->opl_nopi; opix++) {
        opi = opl->opl_opi[opix];

        switch (opi->opi_type) {
            case OPTYP_TRAIL             :
                opiota = 0;
                break;

            case OPTYP_TAKE              :
                opiota = calc_take_opportunity_iota(gst, opi, player);
                break;

            case OPTYP_BUILD_SINGLE      :
            case OPTYP_BUILD_DOUBLE      :
            case OPTYP_BUILD_MORE_DOUBLE :
                build_value = opi->opi_bdlvalue;
                opiota = calc_build_opportunity_iota(gst, opi, player);
                for (cardix = 0; cardix < gst->gst_nhand[player]; cardix++) {
                    if (GET_PIP(gst->gst_hand[player][cardix]) == build_value) {
                        build_card_iota += calc_card_iota(gst->gst_hand[player][cardix]);
                    }
                }
                break;

            default                      :
                DESSERT(1, "Bad opportunity type.");
                break;
        }

        if (gst->gst_nhand[opponent]) {
            card_played = gst->gst_hand[player][opi->opi_cardix];

            calc_opponent_gstate(&work_gst,
                gst->gst_hand[player], gst->gst_nhand[player],
                gst->gst_board, gst->gst_nboard, gst->gst_last_take,
                gst->gst_nturns,
                player);


            play_opportunity(&work_gst, opi, player);

            hand_bits = 0;
            avoid_bits = 0;
            for (cardix = 0; cardix < work_gst.gst_nhand[player]; cardix++) {
                pip = GET_PIP(work_gst.gst_hand[player][cardix]);
                hand_bits |= (1 << pip);
                if (pip <= TEN) {
                    for (slotix = 0; slotix < work_gst.gst_nboard; slotix++) {
                        val = work_gst.gst_board[slotix].sl_value;
                        if ((pip + val) <= TEN &&
                            work_gst.gst_board[slotix].sl_type != SLTYPE_DOUBLED_BUILD) {
                            avoid_bits |= (1 << (pip + val));
                        }
                    }
                }
            }

            opportunity_iota = 0;
            for (remix0 = 0; remix0 < remdk->dk_ncards; remix0++) {
                work_gst.gst_hand[opponent][0] = remdk->dk_card[remix0];
                work_gst.gst_nhand[opponent] = 1;

                best_opp_iota = check_best_opponent_opportunity_smart(
                    &work_gst,
                    &opp_opl,
                    hand_bits,
                    avoid_bits,
                    strsmart,
                    opponent);
                opportunity_iota += opiota - best_opp_iota;

                opp_has_build_card = check_build_for_opponents_card(
                            &work_gst,
                            opi,
	                        build_value,
                            opponent);
                if (opp_has_build_card) {                                
                    opportunity_iota -= (opiota + opiota);
                } else {
                    opportunity_iota += opiota + build_card_iota;
                }
            }
        }

#if IS_DEBUG
	    opl->opl_opi[opix]->opi_opiota = opportunity_iota;
#endif

        if (!opix || (opportunity_iota > opportunity_iota_best)) {
            opportunity_iota_best = opportunity_iota;
            best_opix = opix;
        }
    }


    return (best_opix);
}
/**************************************************************/
void create_remaining_deck_with_builds(struct gstate * gst,
        struct deck * remdk,
        struct deck * opponent_build_dk,
        count_t  * opponent_builds, /* 11/30/2019 */
        count_t player)
{
    count_t opponent;
    count_t slotix;
    count_t val;
    count_t srcix;
    count_t tgtix;
    card_t  rem_card[FIFTY_TWO]; /* 11/30/2019 - Fixed this function. It was bad. */

    opponent = PLAYER_OPPONENT(player);

    opponent_builds[0] = 0;   /* first build value by opponent */
    opponent_builds[1] = 0;   /* second build value by opponent */

    for (slotix = 0; slotix < gst->gst_nboard; slotix++) {
        val = gst->gst_board[slotix].sl_value;
        /* Check builds by player. If not 0, then cannot trail */
        if (gst->gst_board[slotix].sl_owner == opponent) {
            if (!opponent_builds[0]) opponent_builds[0] = val;
            else opponent_builds[1] = val;
        }
    }

    create_remaining_deck_from_cards(gst, remdk, player);

    opponent_build_dk->dk_ncards = 0;
    if (opponent_builds[0]) {
        memcpy(rem_card, remdk->dk_card, sizeof(card_t) * remdk->dk_ncards); /* 11/30/2019 */
        tgtix = 0;
        for (srcix = 0; srcix < remdk->dk_ncards; srcix++) {
            val = GET_PIP(rem_card[srcix]);
            if (val == opponent_builds[0] ||
                val == opponent_builds[1]) {
                opponent_build_dk->dk_card[opponent_build_dk->dk_ncards] =
                    rem_card[srcix];
                opponent_build_dk->dk_ncards += 1;
            } else {
                remdk->dk_card[tgtix] = rem_card[srcix];
                tgtix++;
            }
        }
        remdk->dk_ncards = tgtix;
    }
}
/**************************************************************/
index_t find_opportunity_smart(struct gstate * gst,
        struct opportunitylist * opl,
        struct stratsmart * strsmart,
        count_t player)
{
    index_t best_opix;
    struct deck remdk;
    struct deck opponent_build_dk;
    struct gstate opp_gst;
    count_t opponent_builds[2];

    create_remaining_deck_with_builds(gst, &remdk, &opponent_build_dk, opponent_builds, player);

    calc_opponent_gstate(&opp_gst,
        gst->gst_hand[player], gst->gst_nhand[player],
        gst->gst_board, gst->gst_nboard, gst->gst_last_take,
        gst->gst_nturns,
        player);

    if (opponent_build_dk.dk_ncards) {
        best_opix = calc_best_opportunity_index_build_smart(gst, &opp_gst, opl,
            &remdk, &opponent_build_dk, opponent_builds,
            strsmart, player);
    } else {
        best_opix = calc_best_opportunity_index_smart(gst, &opp_gst, opl,
            &remdk, &opponent_build_dk,
            strsmart, player);
    }

#if IS_DEBUG
    if (is_msglevel(MSGLEVEL_DEBUG_SHOWOPS)) {
        show_opportunities(
                opl,
                opp_gst.gst_board,
                opp_gst.gst_nboard,
                opp_gst.gst_hand[player],
                opp_gst.gst_nhand[player],
                player);
    }
#endif

    return (best_opix);
}
/**************************************************************/
int strat_play_smart_shut(struct game * gm, struct stratsmart * strsmart)
{
/*
** Called once for smart strategy after all play has completed
*/
    int estat = 0;

    return (estat);
}
/***************************************************************/
void strat_smart_shut(struct stratsmart * strsmart)
{
/*
** Called once to shut/free the smart strategy
*/
}
/***************************************************************/
void strat_smart_init(struct stratsmart * strsmart)
{
/*
** Called once to initialize the smart strategy
*/
    strsmart->strsmart_filler          = 0;
}
/***************************************************************/
