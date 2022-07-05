/**************************************************************/
/* strcheat.c                                                 */
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
#include "strcheat.h"
#include "play.h"

/***************************************************************/
int strat_play_cheat_init(struct game * gm,
        struct playinfo * pli,
        struct stratcheat * strcheat)
{
/*
** Called once for cheat strategy before any play has begun
*/
    int estat = 0;

    strcheat->strcheat_max_opportunities = CHEAT_MAX_OPPORTUNITIES;
    strcheat->strcheat_slots             = CHEAT_MAX_SLOTS;

    return (estat);
}
/**************************************************************/
void prune_opportunities_cheat(struct gstate * gst,
        struct opportunitylist * opl,
        struct stratcheat * strcheat,
        count_t player)
{
    if (opl->opl_opi[0]->opi_type                 != OPTYP_TRAIL &&
        opl->opl_opi[opl->opl_nopi - 1]->opi_type == OPTYP_TRAIL) {
        if (gst->gst_nboard > strcheat->strcheat_slots ||
            opl->opl_nopi > strcheat->strcheat_max_opportunities) {
            while (opl->opl_opi[opl->opl_nopi - 1]->opi_type == OPTYP_TRAIL) {
                opl->opl_nopi -= 1;
            }
        }
    }
}
/**************************************************************/
point_t calc_opportunity_iota_cheat(
    struct gstate * gst,
    struct opportunityitem * opi,
    struct stratcheat * strcheat,
    count_t player);

point_t calc_best_opponent_opportunity_iota_cheat(
    struct gstate * gst,
    struct opportunityitem * opi,
    struct stratcheat * strcheat,
    count_t player)
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
        show_opportunityitem_played(gst, opi, player, 3, 0);
    }
#endif

    COPY_GSTATE(&opp_gst, gst);
    play_opportunity(&opp_gst, opi, player);

#if IS_DEBUG
    if (is_msglevel(MSGLEVEL_DEBUG_SHOWAFTER)) {
        msgout(MSGLEVEL_TERSE, "After:\n");
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
    prune_opportunities_cheat(&opp_gst, &opp_opl, strcheat, opponent);

#if IS_DEBUG
    if (opp_opl.opl_nopi == 0) {
        msgout(MSGLEVEL_QUIET, "No opportunities for %s\n", PLAYER_NAME(opponent));
        gstate_print(MSGLEVEL_QUIET, gst, FLAG_PLAYER_EAST | FLAG_PLAYER_WEST | FLAG_BOARD | FLAG_BOARD_SHOW_OWNER);
        free_opportunities(&opp_opl);
        init_opportunitylist(&opp_opl);
        game_opportunities(
            &opp_opl,
            opp_gst.gst_board,
            opp_gst.gst_nboard,
            opp_gst.gst_hand[opponent],
            opp_gst.gst_nhand[opponent],
            opponent);
}
#endif

    opp_best_iota = calc_opportunity_iota_cheat(&opp_gst, opp_opl.opl_opi[0], strcheat, opponent);
    opp_best_opix = 0;

    for (opp_opix = 1; opp_opix < opp_opl.opl_nopi; opp_opix++) {
        opp_opiota = calc_opportunity_iota_cheat(&opp_gst, opp_opl.opl_opi[opp_opix], strcheat, opponent);
        if (opp_opiota > opp_best_iota) {
            opp_best_iota = opp_opiota;
            opp_best_opix = opp_opix;
        }
    }
    free_opportunities(&opp_opl);

    return (opp_best_iota);
}
/**************************************************************/
point_t calc_opportunity_iota_cheat(
    struct gstate * gst,
    struct opportunityitem * opi,
    struct stratcheat * strcheat,
    count_t player)
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
        opp_best_iota = calc_best_opponent_opportunity_iota_cheat(gst, opi, strcheat, player);
        opiota -= opp_best_iota;
    }

    return (opiota);
}
/**************************************************************/
int find_opportunity_cheat(struct gstate * gst,
        struct opportunitylist * opl,
        struct stratcheat * strcheat,
        count_t player)
{
    int opix;
    int best_opix;
    int best_iota;
    int opiota;

    prune_opportunities_cheat(gst, opl, strcheat, player);

    best_iota = calc_opportunity_iota_cheat(gst, opl->opl_opi[0], strcheat, player);
    best_opix = 0;

    for (opix = 1; opix < opl->opl_nopi; opix++) {
        opiota = calc_opportunity_iota_cheat(gst, opl->opl_opi[opix], strcheat, player);
        if (opiota > best_iota) {
            best_iota = opiota;
            best_opix = opix;
        }
    }

    return (best_opix);
}
/**************************************************************/
