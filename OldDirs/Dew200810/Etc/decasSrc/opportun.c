/**************************************************************/
/*  opportun.c -                                               */
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

extern char suit_list[];
extern char pip_list[];
extern char player_list[];

#if COPT_TRAIL_INFO
static int gti_max_combos         = 0;
static int gti_max_opportunities  = 0;
#endif

/***************************************************************/
void totlist_to_buf(
	char * buf,
    count_t * totlist,
    count_t   ntotlist,
    count_t * cardlistix,
	struct slot * g_board)
{
    int blen;
    int ii;
    int jj;
    int bitmask;
    int cardix;
    int slotix;
    card_t card;

    blen = 0;
    for (ii = 0; ii < ntotlist; ii++) {
        BITFOR(bitmask,totlist[ii],cardix) {
            slotix = cardlistix[cardix];
            for (jj = 0; jj < g_board[slotix].sl_ncard; jj++) {
                card = g_board[slotix].sl_card[jj];
                blen += deck_card_pr(buf + blen, &card, 1, '+');
            }
        }
        if (blen) blen--;
        buf[blen++] = ',';
        buf[blen++] = ' ';
    }
    if (blen) blen--;
    if (blen) blen--;
    buf[blen] = '\0';
}
/***************************************************************/
int show_opportunityitem(
    char * slotbuf,
    struct opportunityitem * opi,
	struct slot * g_board,
	card_t      * g_hand)
{
    int slotix;
    int sblen;
    char optyp[32];
    bitmask_t ii;

    sblen = deck_card_pr(slotbuf, g_hand + opi->opi_cardix, 1, ' ');

    switch (opi->opi_type) {
        case OPTYP_TRAIL             :
            strcpy(optyp, "Trail");
            break;
        case OPTYP_TAKE              :
            strcpy(optyp, "Take ");
            break;
        case OPTYP_BUILD_SINGLE      :
            sprintf(optyp, "Build %c ", pip_list[opi->opi_bdlvalue]);
            break;
        case OPTYP_BUILD_DOUBLE      :
            sprintf(optyp, "Bld Dbl %c ", pip_list[opi->opi_bdlvalue]);
            break;
        case OPTYP_BUILD_MORE_DOUBLE :
            sprintf(optyp, "More Dbl %c ", pip_list[opi->opi_bdlvalue]);
            break;
        default                      :
            strcpy(optyp, "?op?");
            break;
    }
    strcpy(slotbuf + sblen, optyp);

    sblen += IStrlen(optyp);
    BITFOR(ii,opi->opi_slotmask,slotix) {
        sblen += deck_slot_pr(slotbuf + sblen, &(g_board[slotix]), ' ', 0);
    }
    slotbuf[sblen] = '\0';

    return (sblen);
}
/***************************************************************/
void show_opportunities(
    struct opportunitylist * opl,
	struct slot * g_board,
    count_t       g_nboard,
	card_t      * g_hand,
    count_t       g_nhand,
    count_t       g_player)
{
    int ii;
    int jj;
    int oix;
    int blen;
    int ncols;
    int cdiff;
    int limit;
    int width;
    int colwidth;
    char buf[PBUF_SIZE];
    int sblen;
    char slotbuf[PBUF_SIZE];

    width = 78;
    ncols = 1;
    if (opl->opl_nopi > 8) ncols = 2;
    limit = (opl->opl_nopi + ncols - 1) / ncols;
    cdiff = limit;
    colwidth = width / ncols;

    msgout(MSGLEVEL_TERSE, "Opportunities for %s\n", PLAYER_NAME(g_player));
    for (ii = 0; ii < limit; ii++) {
        blen = 0;
        for (jj = 0; jj < ncols; jj++) {
            oix = ii + jj * cdiff;
            if (oix < opl->opl_nopi) {
                while (blen < jj * colwidth) buf[blen++] = ' ';
                sprintf(slotbuf, "%4d ", oix + 1);
#if IS_DEBUG
                if (is_msglevel(MSGLEVEL_DEBUG_SHOWOPS)) {
                    sprintf(slotbuf, "%4d %8d ", oix + 1, opl->opl_opi[oix]->opi_opiota);
                }
#endif
                sblen = IStrlen(slotbuf);
                sblen += show_opportunityitem(slotbuf + sblen,
                    opl->opl_opi[oix], g_board, g_hand);
                memcpy(buf + blen, slotbuf, sblen);
                blen += sblen;
            }
        }
        buf[blen] = '\0';
        msgout(MSGLEVEL_TERSE, "%s\n", buf);
    }
}
/***************************************************************/
void print_totlist(
	struct slot * g_board,
    count_t       g_nboard,
	card_t        pip,
    bitmask_t *   totlist,
    count_t       ntotlist)
{
    int ii;
    bitmask_t totbits;
    int blen;
    int slotix;
    char buf[PBUF_SIZE];

    msgout(MSGLEVEL_TERSE, "Totals for %d\n", pip);
    for (ii = 0; ii < ntotlist; ii++) {
        sprintf(buf, "%d. ", ii+1);
        blen = IStrlen(buf);
        BITFOR(totbits,totlist[ii],slotix) {
            blen += deck_slot_pr(buf + blen, &(g_board[slotix]), ' ', 0);
        }
        msgout(MSGLEVEL_TERSE, "%s\n", buf);
    }
}
/***************************************************************/
void game_combos(
    bitmask_t *   totlist,
    count_t   ntotlist,
	bitmask_t   dblbldmask,
    bitmask_t * atotcombos,
    count_t *   ntotcombos) 
{
/*
** totlist contains a list of bitmasks where the slots
** add up to the number we are matching.
** For example is we are matching a 10 and
**      The board is             : 2D AD AS 9S 8S 2C
**      The bits of totlist are  :  1  2  4  8 16 32
**      totlist[0] = 10 AD+9S    :  0  1  0  1  0  0
**      totlist[1] = 12 AS+9S    :  0  0  1  1  0  0
**      totlist[2] = 17 2D+8S    :  1  0  0  0  1  0
**      totlist[3] = 22 AD+AS+8S :  0  1  1  0  1  0
**      totlist[4] = 48 8S+2C    :  0  0  0  0  1  1
**      ntotlist=5
**
**      Then we have to look at the totlist elements to see
**      which ones can be combined. Two elements can be combined
**      if they do not share any slots.  For example, totlist[0]
**      can be combined with totlist[4], but totlist[0] cannot be
**      combined with totlist[1] because they share the 9S.
**
*/
    bigbitmask_t ii;
    bitmask_t bitmask;
    bigbitmask_t totbitmask;
    bigbitmask_t totpow2;
    bitmask_t ormask;
    count_t totlistix;
    count_t hasdups;

    (*ntotcombos) = 0;

    POW2FOR(totpow2,ntotlist,ii) {
        ormask = 0;
        hasdups = 0;
        BITFOR(totbitmask,ii,totlistix) {
            bitmask = totlist[totlistix];
            /*
            ** totlist elements do not use the same element
            ** if the bitmask OR equals the XOR.
            */
            if ((bitmask | ormask) == (bitmask ^ ormask)) {
                ormask |= bitmask;
            } else {
                hasdups = 1;
                totbitmask = 0; /* stop loop */
            }
        }
 
        if (!hasdups) {
            if ((*ntotcombos) >= MAX_COMBOS) {
                ii = 0;
            }
            DESSERT((*ntotcombos) < MAX_COMBOS, "Out of combos");
            atotcombos[*ntotcombos] = ormask;
            (*ntotcombos) += 1;

#if COPT_TRAIL_INFO
    if ((*ntotcombos) > gti_max_combos) {
        gti_max_combos = (*ntotcombos);
        msgout(MSGLEVEL_TERSE, "Maximum combos              = %d\n", gti_max_combos);
    }
#endif

            if (dblbldmask) {
                DESSERT((*ntotcombos) < MAX_COMBOS, "Out of doubled combos");
                atotcombos[*ntotcombos] = ormask | dblbldmask;
                (*ntotcombos) += 1;
            }
        }
    }

    if (dblbldmask) {
        DESSERT((*ntotcombos) < MAX_COMBOS, "Out of doubled combos at end");
        atotcombos[*ntotcombos] = dblbldmask;
        (*ntotcombos) += 1;
    }
}
/***************************************************************/
int check_build_used(
	struct slot * g_board,
    count_t       g_nboard,
    count_t       bdlvalue,
    count_t       g_player,
    bitmask_t     slotmask,
    count_t       player_build) 
{
/*
** Makes certain that if a player has a build, then you must
** take the build if you take with the build card.  For example:
** 
**   Board: JD [3H+5H] 9H AC 7D
** 
**   WEST : 9D 8C 2D
** 
** Opportunities for West
**    1 9D Take 9H                           7 8C Build 9 AC
**    2 9D Take [3H+5H] AC                   8 2D Bld Dbl 9 9H 7D
**    3 9D Take [3H+5H] 9H AC                9 2D Bld Dbl 9 [3H+5H] AC 7D
**    4 8C Take [3H+5H]                     10 2D Bld Dbl 9 [3H+5H] 9H AC 7D
**    5 8C Take [3H+5H] AC 7D               11 2D Build 9 7D
**    6 8C Bld Dbl 9 9H AC
**
**  NOTE: The opportunity "8C Take AC 7D" is not listed.
*/
    count_t build_used;
    count_t slotix;
    bitmask_t bitmask;

    build_used = 0;

    if (bdlvalue != player_build) {
        build_used = 1;
    } else {
        BITFOR(bitmask,slotmask,slotix) {
            if (g_board[slotix].sl_owner == g_player &&
                g_board[slotix].sl_value == player_build) {
                bitmask = 0;
                build_used = 1;
            }
        }
    }

    return (build_used);
}
/***************************************************************/
int check_player_builds_used(
	struct slot * g_board,
    count_t       g_nboard,
    card_t        card_played,
    count_t       bdlvalue,
    count_t       g_player,
    bitmask_t     slotmask,
    count_t *     player_builds,
    count_t *     opponent_builds) 
{
    count_t build_used;

    build_used = 1;
    if (player_builds[0]) {
        build_used = check_build_used(g_board, g_nboard,
            GET_PIP(card_played), g_player, slotmask, player_builds[0]);
        if (build_used) {
            build_used = check_build_used(g_board, g_nboard,
                    bdlvalue, g_player, slotmask, player_builds[0]);
        }
        if (build_used && player_builds[1]) {
            build_used = check_build_used(g_board, g_nboard,
                    GET_PIP(card_played), g_player, slotmask, player_builds[1]);
            if (build_used) {
                build_used = check_build_used(g_board, g_nboard,
                        bdlvalue, g_player, slotmask, player_builds[1]);
            }
        }
    }

    if (build_used) {
        /*
        -- Double builds not allowed if an opponent build already exists with
        -- the same value.
        */
        if (bdlvalue == opponent_builds[0] ||
            bdlvalue == opponent_builds[1]) build_used = 0;
    }
    return (build_used);
}
/***************************************************************/
int new_opportunity(struct opportunitylist * opl,
	count_t   oi_type,
	count_t   oi_cardix,
	count_t   oi_player,
	count_t   oi_bdlvalue,
    bitmask_t oi_slotmask)
{
    struct opportunityitem  * p_opi;
    int ovfl = 0;

    p_opi = New(struct opportunityitem, 1);

    p_opi->opi_type     = oi_type;
    p_opi->opi_cardix   = oi_cardix;
    p_opi->opi_player   = oi_player;
    p_opi->opi_bdlvalue = oi_bdlvalue;
    p_opi->opi_slotmask   = oi_slotmask;
#if IS_DEBUG
	p_opi->opi_opiota     = 0;
#endif

    if (opl->opl_nopi == opl->opl_xopi) {
        if (!opl->opl_xopi) opl->opl_xopi = 16;
        else                opl->opl_xopi *= 2;
        opl->opl_opi = Realloc(opl->opl_opi, struct opportunityitem*, opl->opl_xopi);
    }

#if IS_DEBUG
    if (opl->opl_nopi == opl->opl_emsg) {
        ovfl = 1;
        msgout(MSGLEVEL_TERSE, "Opportunites at %d\n", opl->opl_nopi);
        dbg_set_max_opportunites(opl->opl_xopi);
        opl->opl_emsg = opl->opl_xopi;
    }
#endif

    opl->opl_opi[opl->opl_nopi] = p_opi;
    opl->opl_nopi += 1;

    return (ovfl);
}
/**************************************************************/
void game_take_opportunities(
    struct opportunitylist * opl,
	struct slot * g_board,
    count_t       g_nboard,
    count_t   g_player,
	card_t      * g_hand,
    count_t       g_nhand,
    count_t   g_cardix,
    bitmask_t * atotcombos,
    count_t     ntotcombos,
    count_t *   player_builds) 
{
    count_t ii;
    count_t build_used;
    /* struct opportunityitem  * p_opi; */

    for (ii = 0; ii < ntotcombos; ii++) {
        build_used = 1;
        if (player_builds[0]) {
            build_used = check_build_used(g_board, g_nboard,
                GET_PIP(g_hand[g_cardix]), g_player, atotcombos[ii], player_builds[0]);
            if (build_used && player_builds[1]) {
                build_used = check_build_used(g_board, g_nboard,
                    GET_PIP(g_hand[g_cardix]), g_player, atotcombos[ii], player_builds[1]);
            }
        }

        if (build_used) {
            /*
            DESSERT(opl->opl_nopi < MAX_OPPORTUNITES, "Out of take opportunities");
            p_opi = &(opl->opl_opi[opl->opl_nopi]);
            p_opi->opi_cardix = g_cardix;
            p_opi->opi_player = g_player;
            p_opi->opi_bdlvalue = 0;
            p_opi->opi_type = OPTYP_TAKE;
            p_opi->opi_slotmask   = atotcombos[ii];
            opl->opl_nopi += 1;
            */
#if IS_DEBUG
            if (new_opportunity(opl, OPTYP_TAKE, g_cardix, g_player, 0, atotcombos[ii])) {
                card_show(MSGLEVEL_DEFAULT, g_board, g_nboard, g_hand, g_nhand, g_player);
            }
#else
            new_opportunity(opl, OPTYP_TAKE, g_cardix, g_player, 0, atotcombos[ii]);
#endif

#if COPT_TRAIL_INFO
    if (opl->opl_nopi > gti_max_opportunities) {
        gti_max_opportunities = opl->opl_nopi;
        msgout(MSGLEVEL_TERSE, "Maximum opportunities (7)   = %d\n", gti_max_opportunities);
    }
#endif
        }
    }
}
/***************************************************************/
void game_dblbld_opportunities(
    struct opportunitylist * opl,
	struct slot * g_board,
    count_t       g_nboard,
    count_t   g_player,
	card_t      * g_hand,
    count_t       g_nhand,
    count_t   g_cardix,
    bitmask_t * atotcombos,
    count_t     ntotcombos,
	bitmask_t * dblbldlist,
    count_t *   player_builds,
    count_t *   opponent_builds)
{
    count_t ii;
    count_t bdlvalue;
    count_t build_used;
    count_t build_type;
    /* struct opportunityitem  * p_opi; */

    bdlvalue = GET_PIP(g_hand[g_cardix]);

    for (ii = 0; ii < ntotcombos; ii++) {
        if ((atotcombos[ii] | dblbldlist[bdlvalue-1]) != (atotcombos[ii] ^ dblbldlist[bdlvalue-1])) {
            /* skip this one, it is a duplicate */
        } else {
            build_used = check_player_builds_used(g_board, g_nboard,
                g_hand[g_cardix], bdlvalue, g_player, atotcombos[ii] | dblbldlist[bdlvalue-1],
                player_builds, opponent_builds);
            if (build_used) {
            /*
                DESSERT(opl->opl_nopi < MAX_OPPORTUNITES, "Out of build opportunities");
                p_opi = &(opl->opl_opi[opl->opl_nopi]);
                p_opi->opi_cardix   = g_cardix;
                p_opi->opi_player   = g_player;
                p_opi->opi_bdlvalue = bdlvalue;
                if (dblbldlist[bdlvalue-1]) p_opi->opi_type     = OPTYP_BUILD_MORE_DOUBLE;
                else                        p_opi->opi_type     = OPTYP_BUILD_DOUBLE;

                p_opi->opi_slotmask = atotcombos[ii] | dblbldlist[bdlvalue-1];
                opl->opl_nopi += 1;
            */
            if (dblbldlist[bdlvalue-1]) build_type     = OPTYP_BUILD_MORE_DOUBLE;
            else                        build_type     = OPTYP_BUILD_DOUBLE;
#if IS_DEBUG
            if (new_opportunity(opl, build_type, g_cardix, g_player, bdlvalue, atotcombos[ii])) {
                card_show(MSGLEVEL_DEFAULT, g_board, g_nboard, g_hand, g_nhand, g_player);
            }
#else
            new_opportunity(opl, build_type, g_cardix, g_player, bdlvalue, atotcombos[ii]);
#endif
                /* printf("3. Double build %d\n", opl->opl_nopi); */

#if COPT_TRAIL_INFO
    if (opl->opl_nopi > gti_max_opportunities) {
        gti_max_opportunities = opl->opl_nopi;
        msgout(MSGLEVEL_TERSE, "Maximum opportunities (3)   = %d\n", gti_max_opportunities);
    }
#endif
            }
        }
    }

    if (dblbldlist[bdlvalue - 1]) {
            /*
        DESSERT(opl->opl_nopi < MAX_OPPORTUNITES, "Out of build more opportunities");
        p_opi = &(opl->opl_opi[opl->opl_nopi]);
        p_opi->opi_cardix   = g_cardix;
        p_opi->opi_player   = g_player;
        p_opi->opi_bdlvalue = bdlvalue;
        p_opi->opi_type     = OPTYP_BUILD_MORE_DOUBLE;

        p_opi->opi_slotmask   = dblbldlist[bdlvalue-1];
        opl->opl_nopi += 1;
            */
        /* printf("4. Double build %d\n", opl->opl_nopi); */
#if IS_DEBUG
        if (new_opportunity(opl, OPTYP_BUILD_MORE_DOUBLE, g_cardix, g_player, bdlvalue, dblbldlist[bdlvalue-1])) {
            card_show(MSGLEVEL_DEFAULT, g_board, g_nboard, g_hand, g_nhand, g_player);
        }
#else
        new_opportunity(opl, OPTYP_BUILD_MORE_DOUBLE, g_cardix, g_player, bdlvalue, dblbldlist[bdlvalue-1]);
#endif

#if COPT_TRAIL_INFO
    if (opl->opl_nopi > gti_max_opportunities) {
        gti_max_opportunities = opl->opl_nopi;
        msgout(MSGLEVEL_TERSE, "Maximum opportunities (4)   = %d\n", gti_max_opportunities);
    }
#endif
    }
}
/***************************************************************/
void game_build_double_opportunities(
    struct opportunitylist * opl,
	struct slot * g_board,
    count_t       g_nboard,
    count_t   g_player,
	card_t      * g_hand,
    count_t       g_nhand,
    count_t   g_largecardix,
    count_t   g_smallcardix,
    bitmask_t * difftotlist,
    count_t   ndifftotlist,
	bitmask_t   largedblbldmask,
    bitmask_t * alargetotcombos,
    count_t     nlargetotcombos,
    count_t *   player_builds,
    count_t *   opponent_builds)
{
    bitmask_t   lbitmask;
    bitmask_t   dbitmask;
    count_t     build_used;
    count_t     ii;
    count_t     jj;
    count_t     isdup;
    count_t     difftotlistix;
    count_t     bdlvalue;
    /* struct opportunityitem  * p_opi; */
    bitmask_t   bldbitmask;
    bitmask_t abuilds[MAX_COMBOS];
    count_t   nbuilds;
    count_t   build_type;

/* Hand 0.2:
**  EAST : 7D KC JC QC
**  WEST : TD JD 8C 6H
**  Board: 2D AD AS 9S 8S 2C
**          1  2  4  8 16 32
*/
    bdlvalue = GET_PIP(g_hand[g_largecardix]);
    nbuilds = 0;

    for (ii = 0; ii < nlargetotcombos; ii++) {
        lbitmask = alargetotcombos[ii];

        for (difftotlistix = 0; difftotlistix < ndifftotlist; difftotlistix++) {
            dbitmask = difftotlist[difftotlistix];

            bldbitmask = (lbitmask | dbitmask);
            if (bldbitmask == (lbitmask ^ dbitmask)) {
                isdup = 0;
                for (jj = 0; !isdup && jj < nbuilds; jj++) {
                    if (bldbitmask == abuilds[jj]) {
                        isdup = 1;
                    }
                }
                if (!isdup) {
                    /* The following if prevents two doubled builds of the same value */
                    if (!(largedblbldmask) || (bldbitmask & largedblbldmask)) {
                        DESSERT(nbuilds < MAX_COMBOS, "Out of room in build list");
                        abuilds[nbuilds++] = bldbitmask;
                    }
                }
            }
        }
    }

    for (ii = 0; ii < nbuilds; ii++) {
        build_used = check_player_builds_used(g_board, g_nboard,
            g_hand[g_smallcardix], bdlvalue, g_player, abuilds[ii],
            player_builds, opponent_builds);
        if (build_used) {
            /*
            DESSERT(opl->opl_nopi < MAX_OPPORTUNITES, "Out of dbl build opportunities");
            p_opi = &(opl->opl_opi[opl->opl_nopi]);
            p_opi->opi_cardix   = g_smallcardix;
            p_opi->opi_player   = g_player;
            p_opi->opi_bdlvalue = bdlvalue;

            if (abuilds[ii] & largedblbldmask) p_opi->opi_type = OPTYP_BUILD_MORE_DOUBLE;
            else p_opi->opi_type = OPTYP_BUILD_DOUBLE;

            p_opi->opi_slotmask   = abuilds[ii];
            opl->opl_nopi += 1;
            */
            if (abuilds[ii] & largedblbldmask) build_type     = OPTYP_BUILD_MORE_DOUBLE;
            else                               build_type     = OPTYP_BUILD_DOUBLE;
            /* printf("1. Double build %d\n", opl->opl_nopi); */
#if IS_DEBUG
            if (new_opportunity(opl, build_type, g_smallcardix, g_player, bdlvalue, abuilds[ii])) {
                card_show(MSGLEVEL_DEFAULT, g_board, g_nboard, g_hand, g_nhand, g_player);
            }
#else
            new_opportunity(opl, build_type, g_smallcardix, g_player, bdlvalue, abuilds[ii]);
#endif

#if COPT_TRAIL_INFO
    if (opl->opl_nopi > gti_max_opportunities) {
        gti_max_opportunities = opl->opl_nopi;
        msgout(MSGLEVEL_TERSE, "Maximum opportunities (1)   = %d\n", gti_max_opportunities);
    }
#endif
        }
    }
}
/***************************************************************/
void game_build_opportunities(
    struct opportunitylist * opl,
	struct slot * g_board,
    count_t       g_nboard,
    count_t   g_player,
	card_t      * g_hand,
    count_t       g_nhand,
    count_t   g_largecardix,
    count_t   g_smallcardix,
    bitmask_t * difftotlist,
    count_t   ndifftotlist,
	bitmask_t   largedblbldmask,
    bitmask_t * alargetotcombos,
    count_t nlargetotcombos,
    count_t *   player_builds,
    count_t *   opponent_builds)
{
    count_t difftotlistix;
    count_t bdlvalue;
    count_t build_used;
    count_t build_type;
    bitmask_t slotmask;
    /* struct opportunityitem  * p_opi; */

    bdlvalue = GET_PIP(g_hand[g_largecardix]);

    if (nlargetotcombos) {
        game_build_double_opportunities(opl, g_board, g_nboard, g_player, g_hand, g_nhand,
            g_largecardix ,
            g_smallcardix, difftotlist, ndifftotlist,
            largedblbldmask, alargetotcombos, nlargetotcombos,
            player_builds, opponent_builds);
    }

    for (difftotlistix = 0; difftotlistix < ndifftotlist; difftotlistix++) {
        slotmask = difftotlist[difftotlistix] | largedblbldmask;
        build_used = 1;
        if (player_builds[0]) {
            build_used = check_build_used(g_board, g_nboard,
                GET_PIP(g_hand[g_smallcardix]), g_player, slotmask, player_builds[0]);
            if (build_used && player_builds[1]) {
                build_used = check_build_used(g_board, g_nboard,\
                     GET_PIP(g_hand[g_smallcardix]), g_player, slotmask, player_builds[1]);
            }
        }
        if (build_used) {
            /*
            -- Single builds not allowed if a build already exists with
            -- the same value.
            */
            if (bdlvalue == player_builds[0] ||
                bdlvalue == player_builds[1] ||
                bdlvalue == opponent_builds[0] ||
                bdlvalue == opponent_builds[1]) build_used = 0;
        }
        if (build_used) {
            /*
            DESSERT(opl->opl_nopi < MAX_OPPORTUNITES, "Out of build opportunities");
            p_opi = &(opl->opl_opi[opl->opl_nopi]);
            p_opi->opi_cardix   = g_smallcardix;
            p_opi->opi_player   = g_player;
            p_opi->opi_bdlvalue = bdlvalue;

            if (largedblbldmask) p_opi->opi_type = OPTYP_BUILD_MORE_DOUBLE;
            else p_opi->opi_type = OPTYP_BUILD_SINGLE;
            p_opi->opi_slotmask = slotmask;
            opl->opl_nopi += 1;
            */
            if (largedblbldmask) build_type     = OPTYP_BUILD_MORE_DOUBLE;
            else                 build_type     = OPTYP_BUILD_SINGLE;
            /* printf("2. Double build more %d\n", opl->opl_nopi); */
#if IS_DEBUG
            if (new_opportunity(opl, build_type, g_smallcardix, g_player, bdlvalue, slotmask)) {
                card_show(MSGLEVEL_DEFAULT, g_board, g_nboard, g_hand, g_nhand, g_player);
            }
#else
            new_opportunity(opl, build_type, g_smallcardix, g_player, bdlvalue, slotmask);
#endif

#if COPT_TRAIL_INFO
    if (opl->opl_nopi > gti_max_opportunities) {
        gti_max_opportunities = opl->opl_nopi;
        msgout(MSGLEVEL_TERSE, "Maximum opportunities (2)   = %d\n", gti_max_opportunities);
    }
#endif
        }
    }
}
/***************************************************************/
int game_trail_opportunities(
    struct opportunitylist * opl,
    count_t   g_player,
    count_t   g_cardix)
{
    int ovfl = 0;
    /* struct opportunityitem  * p_opi; */

            /*
    DESSERT(opl->opl_nopi < MAX_OPPORTUNITES, "Out of trail opportunities");
    p_opi = &(opl->opl_opi[opl->opl_nopi]);
    p_opi->opi_slotmask   = 0;
    p_opi->opi_cardix   = g_cardix;
    p_opi->opi_player   = g_player;
    p_opi->opi_bdlvalue = 0;
    p_opi->opi_type     = OPTYP_TRAIL;

    opl->opl_nopi += 1;
            */
    ovfl = new_opportunity(opl, OPTYP_TRAIL, g_cardix, g_player, 0, 0);

#if COPT_TRAIL_INFO
    if (opl->opl_nopi > gti_max_opportunities) {
        gti_max_opportunities = opl->opl_nopi;
        msgout(MSGLEVEL_TERSE, "Maximum opportunities (5)   = %d\n", gti_max_opportunities);
    }
#endif

    return (ovfl);
}
/***************************************************************/
void game_take_face_card_opportunities(
    struct opportunitylist * opl,
	struct slot * g_board,
    count_t       g_nboard,
	card_t      * g_hand,
    count_t       g_nhand,
    count_t       g_player)
{
    count_t cardix;
    count_t slotix;
    card_t  pip;
    /* struct opportunityitem  * p_opi; */

    for (cardix = 0; cardix < g_nhand; cardix++) {
        pip = GET_PIP(g_hand[cardix]);
        if (pip >= JACK) {
            for (slotix = 0; slotix < g_nboard; slotix++) {
                if (g_board[slotix].sl_type == SLTYPE_SINGLE) {
                    if (GET_PIP(g_board[slotix].sl_card[0]) == pip) {
            /*
                        DESSERT(opl->opl_nopi < MAX_OPPORTUNITES, "Out of take face card opportunities");
                        p_opi = &(opl->opl_opi[opl->opl_nopi]);
                        p_opi->opi_cardix = cardix;
                        p_opi->opi_player = g_player;
                        p_opi->opi_bdlvalue = 0;
                        p_opi->opi_type = OPTYP_TAKE;
                        p_opi->opi_slotmask = (1 << slotix);
                        opl->opl_nopi += 1;
            */
#if IS_DEBUG
            if (new_opportunity(opl, OPTYP_TAKE, cardix, g_player, 0, (1 << slotix))) {
                card_show(MSGLEVEL_DEFAULT, g_board, g_nboard, g_hand, g_nhand, g_player);
            }
#else
            new_opportunity(opl, OPTYP_TAKE, cardix, g_player, 0, (1 << slotix));
#endif

#if COPT_TRAIL_INFO
    if (opl->opl_nopi > gti_max_opportunities) {
        gti_max_opportunities = opl->opl_nopi;
        msgout(MSGLEVEL_TERSE, "Maximum opportunities (6)   = %d\n", gti_max_opportunities);
    }
#endif
                    }
                }
            }
        }
    }
}
/***************************************************************/
void game_opportunities(
    struct opportunitylist * opl,
	struct slot * g_board,
    count_t       g_nboard,
	card_t      * g_hand,
    count_t       g_nhand,
    count_t       g_player)
{
    bitmask_t   ncardspow2;
    bitmask_t   bits;
    bitmask_t   bitmask;
    bitmask_t   slotbits;
    count_t ncards;
    count_t slotix;
    count_t cardix;
    count_t card2ix;
    count_t valtot;
    count_t player_builds[2];
    count_t opponent_builds[2];
	count_t opponent;
    count_t pipix;
    count_t pip2ix;
    count_t pipdiffix;
	count_t val;
#if COPT_MAXPIP
    card_t  maxpip;
#endif
	count_t totcounts[TEN];
	bitmask_t dblbldlist[THIRTEEN];
	bitmask_t totlist[TEN][MAX_TOTLIST];
    card_t  cardlist[MAX_CARDS_IN_BOARD];
    count_t cardlistix[MAX_CARDS_IN_BOARD];
    bitmask_t atotcombos[CARDS_PER_PLAYER][MAX_COMBOS];
    count_t ntotcombos[CARDS_PER_PLAYER];
	card_t  largebuilds[TEN]; /* if have more than 1 large, only build with small 1 time */
    /* char buf[80]; */

    /*
    **  dblbldlist      Bitmask of slots that have doubled builds by any player
    **  cardlist        List of slot values for single builds and A-T slots
    **  cardlistix      Slot indexes for cards in cardlist
    **  ncards          Number cards in cardlist and cardlistix
    **  totlist[v]      List of slot bitmasks where the slots total to v
    **  totcounts[v]    Length of each list in totlist[v]
    **  atotcombos[c]   List of totlist bitmasks that can be taken by card c, including dbl builds
    **  ntotcombos[c]   Length of each list in atotcombos[c]
    */
    opl->opl_nopi = 0;

    /* Check for builds */
    /* gather board values into cardlist */
    memset(dblbldlist, 0, sizeof(dblbldlist));
    player_builds[0] = 0;   /* first build value by player */
    player_builds[1] = 0;   /* second build value by player */
    opponent_builds[0] = 0;   /* first build value by opponent */
    opponent_builds[1] = 0;   /* second build value by opponent */
    opponent = PLAYER_OPPONENT(g_player);
    ncards = 0;
    for (slotix = 0; slotix < g_nboard; slotix++) {
        val = g_board[slotix].sl_value;
        /* Check builds by player. If not 0, then cannot trail */
        if (g_board[slotix].sl_owner == g_player) {
            if (!player_builds[0]) player_builds[0] = val;
            else player_builds[1] = val;
        } else if (g_board[slotix].sl_owner == opponent) {
            if (!opponent_builds[0]) opponent_builds[0] = val;
            else opponent_builds[1] = val;
        }

        if (g_board[slotix].sl_type == SLTYPE_DOUBLED_BUILD) {
            dblbldlist[val-1] |= (1 << slotix);
        } else if (val <= TEN) {
            DESSERT(ncards < MAX_CARDS_IN_BOARD, "Too many cards on board");
            cardlist[ncards] = val;
            cardlistix[ncards] = slotix;
            ncards++;
        }
    }

#if COPT_MAXPIP
    maxpip = 0;
    for (cardix = 0; cardix < g_nhand; cardix++) {
        pipix = GET_PIP(g_hand[cardix]);
        if (pipix <= TEN && pipix > maxpip) maxpip = pipix;
    }
#endif

    memset(totcounts, 0, sizeof(totcounts));
    memset(totlist  , 0, sizeof(totlist));
    if (ncards) {
        POW2FOR(ncardspow2,ncards,bits) {
            valtot = 0;
            slotbits = 0;
            BITFOR(bitmask,bits,cardix) {
                valtot += cardlist[cardix];
                slotbits |= (1 << cardlistix[cardix]);
                if (valtot > TEN) {
                    bitmask = 0; /* stop loop */
                }
            }

#if COPT_MAXPIP
            if (valtot <= TEN && valtot <= maxpip) {
#else
            if (valtot <= TEN) {
#endif
                valtot -= 1;
#if IS_DEBUG
                if (totcounts[valtot] >= MAX_TOTLIST) {
                    msgout(MSGLEVEL_DEFAULT, "**** Out of total counts for %c ****\n", pip_list[valtot]);
                    card_show(MSGLEVEL_DEFAULT, g_board, g_nboard, g_hand, g_nhand, g_player);
                }
#endif
                if (totcounts[valtot] < MAX_TOTLIST) {
                    /* DESSERT(totcounts[valtot] < MAX_TOTLIST, "Out of total counts"); */
                    totlist[valtot][totcounts[valtot]] = slotbits;
                    totcounts[valtot] += 1;
                }
            }
        }
    }

    /* Opportunities for taking */
    for (cardix = 0; cardix < g_nhand; cardix++) {
        ntotcombos[cardix] = 0;
        pipix = GET_PIP(g_hand[cardix]) - 1;
        if (pipix < TEN && (totcounts[pipix] || dblbldlist[pipix])) {
            /* print_totlist(g_board, g_nboard, pipix + 1, totlist[pipix], totcounts[pipix]); */
            game_combos(totlist[pipix], totcounts[pipix], dblbldlist[pipix],
                atotcombos[cardix], &(ntotcombos[cardix]));

            game_take_opportunities(opl, g_board, g_nboard, g_player, g_hand, g_nhand, cardix,
                atotcombos[cardix], ntotcombos[cardix], player_builds);
        }
    }

    /* Opportunities for taking face cards */
    game_take_face_card_opportunities(
            opl,
            g_board,
            g_nboard,
            g_hand,
            g_nhand,
            g_player);

    memset(largebuilds, 0, sizeof(largebuilds));
    /* Opportunities for building */
    for (cardix = 0; cardix < g_nhand - 1; cardix++) {
        pipix = GET_PIP(g_hand[cardix]) - 1;
        if (pipix < TEN) {
            for (card2ix = cardix + 1; card2ix < g_nhand; card2ix++) {
                pip2ix = GET_PIP(g_hand[card2ix]) - 1;
                if (pip2ix < TEN) {
                    if (pipix > pip2ix) {
                        pipdiffix = pipix - pip2ix - 1;
                        if (totcounts[pipdiffix] && !(largebuilds[pipix] & (1 << card2ix))) {
        /* print_totlist(g_board, g_nboard, pipix + 1, totlist[pipix], totcounts[pipix]); */
                            game_build_opportunities(opl, g_board, g_nboard, g_player, g_hand, g_nhand,
                                cardix ,
                                card2ix, totlist[pipdiffix], totcounts[pipdiffix],
                                dblbldlist[pipix],
                                atotcombos[cardix], ntotcombos[cardix],
                                player_builds, opponent_builds);
                            largebuilds[pipix] |= (1 << card2ix);
                        }
                    } else if (pip2ix > pipix) {
                        pipdiffix = pip2ix - pipix - 1;
                        if (totcounts[pipdiffix] && !(largebuilds[pip2ix] & (1 << cardix))) {
                            game_build_opportunities(opl, g_board, g_nboard, g_player, g_hand, g_nhand,
                                card2ix,
                                cardix , totlist[pipdiffix], totcounts[pipdiffix],
                                dblbldlist[pip2ix],
                                atotcombos[card2ix], ntotcombos[card2ix],
                                player_builds, opponent_builds);
                            largebuilds[pip2ix] |= (1 << cardix);
                        }
                    } else {
                        game_dblbld_opportunities(opl, g_board, g_nboard, g_player,
                            g_hand, g_nhand, cardix,
                            atotcombos[cardix], ntotcombos[cardix], dblbldlist,
                            player_builds, opponent_builds);

                        game_dblbld_opportunities(opl, g_board, g_nboard, g_player,
                            g_hand, g_nhand, card2ix,
                            atotcombos[card2ix], ntotcombos[card2ix], dblbldlist,
                            player_builds, opponent_builds);
                    }
                }
            }
        }
    }

    /* Opportunities for trailing */
    if (!player_builds[0]) {
        for (cardix = 0; cardix < g_nhand; cardix++) {
#if IS_DEBUG
            if (game_trail_opportunities(opl, g_player, cardix)) {
                card_show(MSGLEVEL_DEFAULT, g_board, g_nboard, g_hand, g_nhand, g_player);
            }
#else
            game_trail_opportunities(opl, g_player, cardix);
#endif
        }
    }

#if IS_DEBUG
    if (!opl->opl_nopi) {
        msgout(MSGLEVEL_TERSE, "No opportunities for %s\n", PLAYER_NAME(g_player));
        card_show(MSGLEVEL_TERSE, g_board, g_nboard, g_hand, g_nhand, g_player);
        program_abort("No opportunities.");
    }
#endif

/*
    printf("Totals:\n");
    for (cardix = 0; cardix < TEN; cardix++) {
        if (totcounts[cardix]) {
            totlist_to_buf(buf, totlist[cardix], totcounts[cardix], cardlistix, g_board);
            printf("  %2d: %s\n", cardix + 1, buf);
        }
    }
*/
}
/**************************************************************/
#if COPT_TRAIL_INFO
void print_trail_opportunity_info(void)
{
    msgout(MSGLEVEL_TERSE, "Final maximum combos                = %d\n", gti_max_combos);
    msgout(MSGLEVEL_TERSE, "Final maximum opportunities         = %d\n", gti_max_opportunities);
}
#endif
/**************************************************************/
