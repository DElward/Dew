/**************************************************************/
/* strat.c                                                    */
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
#include "strbest.h"
#include "strcheat.h"
#include "strsimp.h"
#include "strsmart.h"
#include "strat.h"
#include "randnum.h"
#include "play.h"

struct strategy {   /* strat_ */
    count_t     strat_strattype;
    union {
        struct stratsimple strat_strsimp;
        struct stratsmart  strat_strsmart;
        struct stratbest   strat_strbest;
        struct stratcheat  strat_strcheat;
    } strat_u;
};

/***************************************************************/
/***************************************************************/
struct strategy * strategy_new(struct globals * glb) {

    struct strategy * strat;

    strat = New(struct strategy, 1);

    strat->strat_strattype      = STRATTYPE_NONE;

    return (strat);
}
/***************************************************************/
int parse_strategy(struct strategy * strat,
        const char *line, int * lix, char * stoke)
{
    int estat = 0;

    if (!strcmp(stoke, "SIMPLE")) {
        strat->strat_strattype      = STRATTYPE_SIMPLE;
        strat_init(strat);
        estat = parse_simple_strategy_subtype(&(strat->strat_u.strat_strsimp), line, lix, stoke);
    } else if (!strcmp(stoke, "MENU")) {
        strat->strat_strattype      = STRATTYPE_MENU;
        strat_init(strat);
        gettoke(line, lix, stoke, STOKE_MAX_SIZE);
    } else if (!strcmp(stoke, "SMART")) {
        strat->strat_strattype      = STRATTYPE_SMART;
        strat_init(strat);
        gettoke(line, lix, stoke, STOKE_MAX_SIZE);
    } else if (!strcmp(stoke, "BEST")) {
        strat->strat_strattype      = STRATTYPE_BEST;
        strat_init(strat);
        estat = parse_best_strategy_subtype(&(strat->strat_u.strat_strbest), line, lix, stoke);
    } else if (!strcmp(stoke, "CHEAT")) {
        strat->strat_strattype      = STRATTYPE_CHEAT;
        strat_init(strat);
        gettoke(line, lix, stoke, STOKE_MAX_SIZE);
    } else {
        estat = set_error(ESTAT_BADSTRAT,"Unrecognized strategy: %s", stoke);
    }

    if (!estat) {
        estat = strat_start(strat);
    }

    return (estat);
}
/**************************************************************/
char * get_strategy_string(struct strategy * strat, int pflags)
{
    static char stratstr[64];

    /* ! = uniomplemented */

    switch (strat->strat_strattype) {
        case STRATTYPE_DUMB:
            strcpy(stratstr, "Dumb!");
            break;
        case STRATTYPE_SIMPLE:
            strcpy(stratstr, "Simple");
            break;
        case STRATTYPE_BEST:
            if (strat->strat_u.strat_strbest.strbest_shuf_rgen < 0) {
                strcpy(stratstr, "Best");
            } else {
                sprintf(stratstr, "Best(%d.%d)", 
                    strat->strat_u.strat_strbest.strbest_shuf_rgen,
                    strat->strat_u.strat_strbest.strbest_shuf_rseed);
            }
            break;
        case STRATTYPE_SMART:
            strcpy(stratstr, "Smart");
            break;
        case STRATTYPE_CHEAT:
            strcpy(stratstr, "Cheat");
            break;
        case STRATTYPE_PROMPT:
            strcpy(stratstr, "Prompt!");
            break;
        case STRATTYPE_MENU:
            strcpy(stratstr, "Menu");
            break;
        default:
            sprintf(stratstr, "?%d?", strat->strat_strattype);
            break;
    }

    return (stratstr);
}
/**************************************************************/
void print_strategy(char * stratbuf, int stratbufmax, struct strategy * strat)
{
    char * stratstr;

    stratstr = get_strategy_string(strat, 0);

    strmcpy(stratbuf, stratstr, stratbufmax);
}
/***************************************************************/
int strat_match_init(struct game * gm, struct strategy * strat)
{
/*
** Called once before the beginning of all matches
*/
    int estat = 0;

    switch (strat->strat_strattype) {
        case STRATTYPE_SIMPLE:
            estat = strat_match_simple_init(gm, &(strat->strat_u.strat_strsimp));
            break;
        case STRATTYPE_MENU:
            break;
        case STRATTYPE_BEST:
            break;
        case STRATTYPE_CHEAT:
            break;
        default:
            break;
    }

    return (estat);
}
/***************************************************************/
int strat_match_begin(struct game * gm, struct strategy * strat)
{
/*
** Called before each match has started
*/
    int estat = 0;

    switch (strat->strat_strattype) {
        case STRATTYPE_SIMPLE:
            estat = strat_match_simple_begin(gm, &(strat->strat_u.strat_strsimp));
            break;
        case STRATTYPE_MENU:
            break;
        case STRATTYPE_BEST:
            break;
        case STRATTYPE_CHEAT:
            break;
        default:
            break;
    }

    return (estat);
}
/***************************************************************/
int strat_match_end(struct game * gm,
                    struct strategy * strat,
                    struct gscore * gsc,
                    count_t player)
{
/*
** Called after each match has completed
*/
    int estat = 0;

    switch (strat->strat_strattype) {
        case STRATTYPE_SIMPLE:
            estat = strat_match_simple_end(gm, &(strat->strat_u.strat_strsimp), gsc, player);
            break;
        case STRATTYPE_MENU:
            break;
        case STRATTYPE_BEST:
            break;
        case STRATTYPE_CHEAT:
            break;
        default:
            break;
    }

    return (estat);
}
/***************************************************************/
int strat_match_shut(struct game * gm, struct strategy * strat)
{
/*
** Called once all matches are complete
*/
    int estat = 0;

    switch (strat->strat_strattype) {
        case STRATTYPE_SIMPLE:
            estat = strat_match_simple_shut(gm, &(strat->strat_u.strat_strsimp));
            break;
        case STRATTYPE_MENU:
            break;
        case STRATTYPE_BEST:
            break;
        case STRATTYPE_CHEAT:
            break;
        default:
            break;
    }

    return (estat);
}
/***************************************************************/
int strat_play_init(struct game * gm, struct playinfo * pli, struct strategy * strat)
{
/*
** Called once before any play has begun
*/
    int estat = 0;

    switch (strat->strat_strattype) {
        case STRATTYPE_SIMPLE:
            estat = strat_play_simple_init(gm, pli, &(strat->strat_u.strat_strsimp));
            break;
        case STRATTYPE_SMART:
            estat = strat_play_smart_init(gm, pli, &(strat->strat_u.strat_strsmart));
            break;
        case STRATTYPE_MENU:
            break;
        case STRATTYPE_BEST:
            estat = strat_play_best_init(gm, pli, &(strat->strat_u.strat_strbest));
            break;
        case STRATTYPE_CHEAT:
            estat = strat_play_cheat_init(gm, pli, &(strat->strat_u.strat_strcheat));
            break;
        default:
            break;
    }

    return (estat);
}
/***************************************************************/
int strat_play_shut(struct game * gm, struct strategy * strat)
{
/*
** Called once after all play has completed
*/
    int estat = 0;

    switch (strat->strat_strattype) {
        case STRATTYPE_SIMPLE:
            estat = strat_play_simple_shut(gm, &(strat->strat_u.strat_strsimp));
            break;
        case STRATTYPE_SMART:
            estat = strat_play_smart_shut(gm, &(strat->strat_u.strat_strsmart));
            break;
        case STRATTYPE_MENU:
            break;
        case STRATTYPE_BEST:
            break;
        case STRATTYPE_CHEAT:
            break;
        default:
            break;
    }

    return (estat);
}
/***************************************************************/
void strat_init(struct strategy * strat)
{
/*
** Called once to initialize the strategy
*/
    switch (strat->strat_strattype) {
        case STRATTYPE_SIMPLE:
            strat_simple_init(&(strat->strat_u.strat_strsimp));
            break;
        case STRATTYPE_MENU:
            break;
        case STRATTYPE_SMART:
            strat_smart_init(&(strat->strat_u.strat_strsmart));
            break;
        case STRATTYPE_BEST:
            strat_best_init(&(strat->strat_u.strat_strbest));
            break;
        case STRATTYPE_CHEAT:
            break;
        default:
            break;
    }
}
/***************************************************************/
int strat_start(struct strategy * strat)
{
/*
** Called once to start the strategy after knowing configuration parameters
*/
    int estat = 0;

    switch (strat->strat_strattype) {
        case STRATTYPE_SIMPLE:
            break;
        case STRATTYPE_MENU:
            break;
        case STRATTYPE_SMART:
            break;
        case STRATTYPE_BEST:
            estat = strat_start_best(&(strat->strat_u.strat_strbest));
            break;
        case STRATTYPE_CHEAT:
            break;
        default:
            break;
    }

    return (estat);
}
/***************************************************************/
void strat_shut(struct strategy * strat)
{
/*
** Called once to shut/free the strategy
*/
    switch (strat->strat_strattype) {
        case STRATTYPE_SIMPLE:
            strat_simple_shut(&(strat->strat_u.strat_strsimp));
            break;
        case STRATTYPE_SMART:
            strat_smart_shut(&(strat->strat_u.strat_strsmart));
            break;
        case STRATTYPE_MENU:
            break;
        case STRATTYPE_BEST:
            strat_best_shut(&(strat->strat_u.strat_strbest));
            break;
        case STRATTYPE_CHEAT:
            break;
        default:
            break;
    }

    Free(strat);
}
/**************************************************************/
int find_opportunity(struct gstate * gst,
        struct opportunitylist * opl,
        struct strategy * strat,
        count_t player,
        int * p_estat)
{
    int opix;

    switch (strat->strat_strattype) {
        case STRATTYPE_SIMPLE:
            opix = find_opportunity_simple(gst, opl, &(strat->strat_u.strat_strsimp), player);
            break;
        case STRATTYPE_MENU:
            opix = find_opportunity_menu(gst, opl, player);
            break;
        case STRATTYPE_SMART:
            opix = find_opportunity_smart(gst, opl, &(strat->strat_u.strat_strsmart), player);
            break;
        case STRATTYPE_BEST:
            opix = find_opportunity_best(gst, opl, &(strat->strat_u.strat_strbest), player, p_estat);
            break;
        case STRATTYPE_CHEAT:
            opix = find_opportunity_cheat(gst, opl, &(strat->strat_u.strat_strcheat), player);
            break;
        default:
            (*p_estat) = set_error(ESTAT_UNSUPPSTRAT,"Unsupported strategy: %d", strat->strat_strattype);
            break;
    }

    return (opix);
}
/**************************************************************/

