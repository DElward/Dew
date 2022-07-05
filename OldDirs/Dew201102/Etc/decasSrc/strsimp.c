/**************************************************************/
/* strsimp.c                                                  */
/**************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>
#include <errno.h>
#include <math.h>

#include "types.h"
#include "decas.h"
#include "opportun.h"
#include "strsimp.h"
#include "randnum.h"
#include "play.h"

/***************************************************************/
int parse_simple_strategy_subtype(struct stratsimple * strsimp,
        const char *line,
        int * lix,
        char * stoke)
{
    int estat = 0;
    int num;
    int done;

    done = 0;

    do {
        gettoke(line, lix, stoke, STOKE_MAX_SIZE);
        if (strcmp(stoke, ";")) {
            done = 1;
        } else {
            gettoke(line, lix, stoke, STOKE_MAX_SIZE);
            if (!strcmp(stoke, ";")) {
                /* ok */
            } else  if (!strcmp(stoke, "POOL")) {
                gettoke(line, lix, stoke, STOKE_MAX_SIZE);
                if (strcmp(stoke, "=")) {
                    estat = set_error(ESTAT_EXPEQUAL,
                        "Expecting equal sign (=). Found: %s", stoke);
                } else {
                    gettoke(line, lix, stoke, STOKE_MAX_SIZE);
                    if (stoui(stoke, &num)) {
                        estat = set_error(ESTAT_BADNUMBER,"Invalid number: %s", stoke);
                    } else {
                        strsimp->strsimp_pool_size  = num;
                    }
                }
            } else {
                estat = set_error(ESTAT_BADSTRATINFO,
                    "Unrecognized simple strategy info: %s", stoke);
            }
        }
    } while (!estat && !done);

    return (estat);
}
/**************************************************************/
point_t calc_opportunity_iota_simple(struct gstate * gst,
        struct opportunityitem * opi,
        struct stratsimple * strsimp,
        count_t player)
{
    point_t opiota;
    card_t card_played;
    bitmask_t slotmask;
    count_t slotix;
    count_t opponent;
    count_t opponent_build_value;
    int ii;
    card_t pip;
    point_t * card_adj = NULL;

    card_played = gst->gst_hand[player][opi->opi_cardix];
    opponent = PLAYER_OPPONENT(player);
    opponent_build_value = 0;

    if (opi->opi_type == OPTYP_TRAIL) {
        opiota = -calc_card_iota(card_played);
        pip = GET_PIP(card_played);
        switch (gst->gst_nhand[player]) {
            case 4: opiota -= strsimp->strsimp_card_adj4[pip];     break;
            case 3: opiota -= strsimp->strsimp_card_adj3[pip];     break;
            case 2: opiota -= strsimp->strsimp_card_adj2[pip];     break;
            default:
                break;
        }
    } else {
        opiota = 0;
        BITFOR(slotmask,opi->opi_slotmask,slotix) {
            if (gst->gst_board[slotix].sl_owner == opponent) {
                opponent_build_value = gst->gst_board[slotix].sl_value;
            }
            for (ii = 0; ii < gst->gst_board[slotix].sl_ncard; ii++) {
                opiota += calc_card_iota(gst->gst_board[slotix].sl_card[ii]);
            }
        }
        opiota += calc_card_iota(card_played);
        if (opi->opi_type != OPTYP_TAKE) {
            if (opponent_build_value == opi->opi_bdlvalue) {
                opiota = -opiota;
            } else {
                switch (gst->gst_nhand[player]) {
                    case 4: opiota -= strsimp->strsimp_card_adj4[0];     break;
                    case 3: opiota -= strsimp->strsimp_card_adj3[0];     break;
                    case 2: opiota -= strsimp->strsimp_card_adj2[0];     break;
                    default:
                        DESSERT(0, "Build with 1 card.");
                        break;
                }
                /* Add card you will play later. */
                for (ii = 0; ii < gst->gst_nhand[player]; ii++) {
                    if (ii != opi->opi_cardix &&
                        GET_PIP(gst->gst_hand[player][ii]) == opi->opi_bdlvalue) {
                        opiota += calc_card_iota(gst->gst_hand[player][ii]);
                    }
                }
                /*
                ** To make it more important to keep valuable cards in your hand
                ** when you have two or more cards of the build value,
                ** we're going to subtract 1/4 of the value.  e.g. If you have
                ** both the 3S and 3D, and you're going to build using a 3,
                ** it's better to build using the 3D.
                */
                if (GET_PIP(card_played) == opi->opi_bdlvalue) {
                    opiota -= calc_card_iota(card_played) >> 2;
                }
            }
        }
    }

    return (opiota);
}
/**************************************************************/
int find_opportunity_simple(struct gstate * gst,
        struct opportunitylist * opl,
        struct stratsimple * strsimp,
        count_t player)
{
    int opix;
    int best_opix;
    int best_iota;
    int opiota;

    best_iota = calc_opportunity_iota_simple(gst, opl->opl_opi[0], strsimp, player);
    best_opix = 0;

    for (opix = 1; opix < opl->opl_nopi; opix++) {
        opiota = calc_opportunity_iota_simple(gst, opl->opl_opi[opix], strsimp, player);
        if (opiota > best_iota) {
            best_iota = opiota;
            best_opix = opix;
        }
    }

    return (best_opix);
}
/**************************************************************/
//----------------------------------------------------------------
// From Numerical Recipes in C (NUMREC):
//
//  /* note #undef's at end of file */
//  #define SWAP(a,b) temp=(a);(a)=(b);(b)=temp;
//     
//  float select(unsigned long k, unsigned long n, float arr[])
//  {
//      unsigned long i,ir,j,l,mid;
//      float a,temp;
//     
//      l=1;
//      ir=n;
//      for (;;) {
//          if (ir <= l+1) {
//              if (ir == l+1 && arr[ir] < arr[l]) {
//                  SWAP(arr[l],arr[ir])
//              }
//              return arr[k];
//          } else {
//              mid=(l+ir) >> 1;
//              SWAP(arr[mid],arr[l+1])
//              if (arr[l] > arr[ir]) {
//                  SWAP(arr[l],arr[ir])
//              }
//              if (arr[l+1] > arr[ir]) {
//                  SWAP(arr[l+1],arr[ir])
//              }
//              if (arr[l] > arr[l+1]) {
//                  SWAP(arr[l],arr[l+1])
//  
//              }
//              i=l+1;
//              j=ir;
//              a=arr[l+1];
//              for (;;) {
//                  do i++; while (arr[i] < a);
//                  do j--; while (arr[j] > a);
//                  if (j < i) break;
//                  SWAP(arr[i],arr[j])
//              }
//              arr[l+1]=arr[j];
//              arr[j]=a;
//              if (j >= k) ir=j-1;
//              if (j <= k) l=i;
//          }
//      }
//  }
//  #undef SWAP
//----------------------------------------------------------------


/***************************************************************/
void strat_simple_calc_averages(struct stratsimple * strsimp)
{
    int poolix;
    double avg;

    for (poolix = 0; poolix < strsimp->strsimp_pool_size; poolix++) {
        if (strsimp->strsimp_stats[poolix]->strsimpst_num_matches == 0) {
            avg = 0.0;
        } else {
            avg = (double)(strsimp->strsimp_stats[poolix]->strsimpst_points_for) /
                  (double)(strsimp->strsimp_stats[poolix]->strsimpst_num_matches);
        }
        strsimp->strsimp_stats[poolix]->strsimpst_match_avg = (int)floor(avg + 0.5);
    }
}
/***************************************************************/
//      void qsoptsyms(struct optsymbol ** y, int l, int r) {
//      /*
//      ** Quick sort of symbol struct:  y - data
//      **
//      ** to call:  qssyms(y, 0, nyms - 1);
//      */
//          int i;
//          int j;
//          char * v;
//          struct optsymbol * u;
//      
//          if (r > l) {
//              v = y[r]->optsym; i = l - 1; j = r;
//              for (;;) {
//                  while (i < j && strcmp(y[++i]->optsym, v) < 0) ; /* use < 0 for asc, > 0 for desc */
//                  while (i < j && strcmp(y[--j]->optsym, v) > 0) ; /* use > 0 for asc, < 0 for desc */
//                  if (i >= j) break;
//                  u = y[i]; y[i] = y[j]; y[j] = u;
//              }
//              u = y[i]; y[i] = y[r]; y[r] = u;
//              qsoptsyms(y, l, i-1);
//              qsoptsyms(y, i+1, r);
//          }
//      }
//      /***************************************************************/
//      void sortoptsymbols(void) {
//      
//          int ii;
//          int nn;
//      
//          qsoptsyms(optsymlist, 0, noptsyms - 1);
//      
//          /* Remove duplicate option symbols */
//          nn = 0;
//          for (ii = 1; ii < noptsyms; ii++) {
//              if (!strcmp(optsymlist[ii]->optsym, optsymlist[nn]->optsym)) {
//                  Free(optsymlist[ii]);
//              } else {
//                  nn++;
//                  if (ii != nn) optsymlist[nn] = optsymlist[ii];
//              }
//          }
//          if (noptsyms != nn) {
//              noptsyms = nn;
//              optsymlist = Realloc(optsymlist, struct optsymbol *, noptsyms);
//          }
//      }
/***************************************************************/
void strat_simple_quicksort_averages(struct strsimpstats ** strsimpst, int l, int r)
{
    int i;
    int j;
    int v;
    struct strsimpstats * u;

    if (r > l) {
        v = strsimpst[r]->strsimpst_match_avg; i = l - 1; j = r;
        for (;;) {
            while (i < j && strsimpst[++i]->strsimpst_match_avg > v) ; /* use < v for asc, > v for desc */
            while (i < j && strsimpst[--j]->strsimpst_match_avg < v) ; /* use > v for asc, < v for desc */
            if (i >= j) break;
            u = strsimpst[i]; strsimpst[i] = strsimpst[j]; strsimpst[j] = u;
        }
        u = strsimpst[i]; strsimpst[i] = strsimpst[r]; strsimpst[r] = u;
        strat_simple_quicksort_averages(strsimpst, l, i-1);
        strat_simple_quicksort_averages(strsimpst, i+1, r);
    }
}
/***************************************************************/
void strat_simple_sort_by_averages(struct stratsimple * strsimp)
{
    strat_simple_quicksort_averages(strsimp->strsimp_stats, 0, strsimp->strsimp_pool_size - 1);
}
/***************************************************************/
void strat_simple_show_strsimpstats(struct strsimpstats * strsimpst, count_t player)
{
    int elix;
    int ncards;

    for (ncards = 2; ncards <= 4; ncards++) {
        printf("%s,%d,%5d,%3d,%3d,%3d,",
            PLAYER_NAME(player),
            ncards,
            strsimpst->strsimpst_points_for,
            strsimpst->strsimpst_num_matches,
            strsimpst->strsimpst_match_avg,
            strsimpst->strsimpst_gen);

        for (elix = 0; elix < SIMP_ADJ_ELEMENTS; elix++) {
            if (elix) printf(",");
            switch (ncards) {
                case 2: printf("%2d", strsimpst->strsimpst_pool_list2[elix]); break;
                case 3: printf("%2d", strsimpst->strsimpst_pool_list3[elix]); break;
                case 4: printf("%2d", strsimpst->strsimpst_pool_list4[elix]); break;
                default: break;
            }
        }
        printf("\n");
    }
}
/***************************************************************/
void strat_simple_show_stratsimple(struct stratsimple * strsimp, count_t player)
{
    int poolix;

    /* printf("Results for %s:\n", PLAYER_NAME(player)); */
    printf("\n");
    printf("\n");

    for (poolix = 0; poolix < strsimp->strsimp_pool_size; poolix++) {
        strat_simple_show_strsimpstats(strsimp->strsimp_stats[poolix], player);
    }
}
/***************************************************************/
void strat_simple_init_pool_item(struct game * gm, struct stratsimple * strsimp, point_t * pool_item)
{
    int elix;

    for (elix = 0; elix < SIMP_ADJ_ELEMENTS; elix++) {
        pool_item[elix] = randbetween(gm->gm_erg, 5, 95);
    }
    pool_item[11] = 10;
    pool_item[12] = pool_item[11];
    pool_item[13] = pool_item[11];
}
/***************************************************************/
void strat_simple_clear_stat_item(struct strsimpstats * strsimpst)
{
    strsimpst->strsimpst_points_for     = 0;
    strsimpst->strsimpst_num_matches    = 0;
    strsimpst->strsimpst_match_avg      = 0;
}
/***************************************************************/
void strat_simple_init_stat_item(struct game * gm, struct stratsimple * strsimp, struct strsimpstats * strsimpst)
{
    strat_simple_clear_stat_item(strsimpst);
    strsimpst->strsimpst_gen            = strsimp->strsimp_pool_gen;

    strsimpst->strsimpst_pool_list2 = New(point_t, SIMP_ADJ_ELEMENTS);
    strat_simple_init_pool_item(gm, strsimp, strsimpst->strsimpst_pool_list2);

    strsimpst->strsimpst_pool_list3 = New(point_t, SIMP_ADJ_ELEMENTS);
    strat_simple_init_pool_item(gm, strsimp, strsimpst->strsimpst_pool_list3);

    strsimpst->strsimpst_pool_list4 = New(point_t, SIMP_ADJ_ELEMENTS);
    strat_simple_init_pool_item(gm, strsimp, strsimpst->strsimpst_pool_list4);
}
/***************************************************************/
void strat_simple_merge_pool_items(struct game * gm,
    point_t * tgt_pool_item,
    point_t * src_pool_item0,
    point_t * src_pool_item1)
{
    int elix;

    for (elix = 0; elix < SIMP_ADJ_ELEMENTS_M3; elix++) {
        tgt_pool_item[elix] = (src_pool_item0[elix] + src_pool_item1[elix]) / 2 + randbetween(gm->gm_erg, -2, 2);
    }
}
/***************************************************************/
void strat_simple_copy_pool_item(point_t * tgt_pool_item, point_t * src_pool_item)
{
    memcpy(tgt_pool_item, src_pool_item, sizeof(point_t) * SIMP_ADJ_ELEMENTS);
}
/***************************************************************/
void strat_simple_create_next_gen(struct game * gm, struct stratsimple * strsimp)
{
    int poolix;
    int psize;
    int ix0;
    int ix1;

    strsimp->strsimp_pool_gen += 1;

    for (poolix = 0; poolix < strsimp->strsimp_pool_size; poolix++) {
        strat_simple_clear_stat_item(strsimp->strsimp_stats[poolix]);
    }

    psize = strsimp->strsimp_next_gen_size;
    while (psize < strsimp->strsimp_pool_size) {
        ix0 = randbetween(gm->gm_erg, 0, strsimp->strsimp_next_gen_size - 1);
        ix1 = randbetween(gm->gm_erg, 0, strsimp->strsimp_next_gen_size - 1);
        if (ix0 != ix1) {
            strat_simple_merge_pool_items(gm,
                strsimp->strsimp_stats[psize]->strsimpst_pool_list2,
                strsimp->strsimp_stats[ix0]->strsimpst_pool_list2,
                strsimp->strsimp_stats[ix1]->strsimpst_pool_list2);
            strat_simple_merge_pool_items(gm,
                strsimp->strsimp_stats[psize]->strsimpst_pool_list3,
                strsimp->strsimp_stats[ix0]->strsimpst_pool_list3,
                strsimp->strsimp_stats[ix1]->strsimpst_pool_list3);
            strat_simple_merge_pool_items(gm,
                strsimp->strsimp_stats[psize]->strsimpst_pool_list4,
                strsimp->strsimp_stats[ix0]->strsimpst_pool_list4,
                strsimp->strsimp_stats[ix1]->strsimpst_pool_list4);
            strsimp->strsimp_stats[psize]->strsimpst_gen = strsimp->strsimp_pool_gen;
            psize++;
        }
    }

    if (randbetween(gm->gm_erg, 0, 3) == 0) {
        ix0 = strsimp->strsimp_pool_size - 1;
        strat_simple_init_pool_item(gm, strsimp, strsimp->strsimp_stats[ix0]->strsimpst_pool_list2);
        strat_simple_init_pool_item(gm, strsimp, strsimp->strsimp_stats[ix0]->strsimpst_pool_list3);
        strat_simple_init_pool_item(gm, strsimp, strsimp->strsimp_stats[ix0]->strsimpst_pool_list4);
    }
}
/***************************************************************/
void strat_simple_end_generation(struct game * gm, struct stratsimple * strsimp, count_t player)
{
    strat_simple_calc_averages(strsimp);
    strat_simple_sort_by_averages(strsimp);
    strat_simple_show_stratsimple(strsimp, player);

    strat_simple_create_next_gen(gm, strsimp);

    strsimp->strsimp_match_num = 0;
}
/**************************************************************/
int strat_match_simple_init(struct game * gm, struct stratsimple * strsimp)
{
/*
** Called once for simple strategy before the beginning of all matches
*/
    int estat = 0;

    return (estat);
}
/***************************************************************/
int strat_match_simple_begin(struct game * gm, struct stratsimple * strsimp)
{
/*
** Called before each simple strategy match has started
*/
    int estat = 0;
    int poolix;

    if (strsimp->strsimp_pool_size) {
        poolix = randbetween(gm->gm_erg, 0, strsimp->strsimp_pool_size - 1);
        strsimp->strsimp_pool_index = poolix;
        strat_simple_copy_pool_item(strsimp->strsimp_card_adj2, strsimp->strsimp_stats[poolix]->strsimpst_pool_list2);
        strat_simple_copy_pool_item(strsimp->strsimp_card_adj3, strsimp->strsimp_stats[poolix]->strsimpst_pool_list3);
        strat_simple_copy_pool_item(strsimp->strsimp_card_adj4, strsimp->strsimp_stats[poolix]->strsimpst_pool_list4);
    }

    strsimp->strsimp_match_num += 1;

    return (estat);
}
/***************************************************************/
int strat_match_simple_end(struct game * gm,
                    struct stratsimple * strsimp,
                    struct gscore * gsc,
                    count_t player)
{
/*
** Called after each simple strategy match has completed
*/
    int estat = 0;
    int poolix;

    if (strsimp->strsimp_pool_size) {
        poolix = strsimp->strsimp_pool_index;
        strsimp->strsimp_stats[poolix]->strsimpst_points_for = gsc->gsc_tot_points[player];
        strsimp->strsimp_stats[poolix]->strsimpst_num_matches += 1;

        if (strsimp->strsimp_match_num == strsimp->strsimp_matches_per_gen) {
            strat_simple_end_generation(gm, strsimp, player);
        }
    }

    return (estat);
}
/***************************************************************/
void strat_simple_shut(struct stratsimple * strsimp)
{
/*
** Called once to shut/free the simple strategy
*/
}
/***************************************************************/
int strat_match_simple_shut(struct game * gm, struct stratsimple * strsimp)
{
/*
** Called once for simple strategy all matches are complete
*/
    int estat = 0;

    return (estat);
}
/***************************************************************/
int strat_play_simple_init(struct game * gm, struct playinfo * pli, struct stratsimple * strsimp)
{
/*
** Called once for simple strategy before any play has begun
*/
    int estat = 0;
    int poolix;
    
    /* Calculated 03/03/2017  - Bld   A   2   3   4   5   6   7   8   9   T   J   Q   K   - */
    point_t init_card_adj2[] = { 36, 37, 41, 40, 32, 45, 31, 48, 51, 43, 46, 10, 10, 10,  0};
    point_t init_card_adj3[] = { 42, 35, 35, 55, 31, 49, 39, 35, 26, 38, 40, 10, 10, 10,  0};
    point_t init_card_adj4[] = { 27, 35, 40, 41, 44, 33, 39, 32, 52, 62, 46, 10, 10, 10,  0};

    /* 03/03/2017 using: play dealer east, west simple;pool=16, east simple;pool=16, for 262144 matches of 128 games, msgs terse */

    strsimp->strsimp_card_adj2 = New(point_t, SIMP_ADJ_ELEMENTS);
    strat_simple_copy_pool_item(strsimp->strsimp_card_adj2, init_card_adj2);

    strsimp->strsimp_card_adj3 = New(point_t, SIMP_ADJ_ELEMENTS);
    strat_simple_copy_pool_item(strsimp->strsimp_card_adj3, init_card_adj3);

    strsimp->strsimp_card_adj4 = New(point_t, SIMP_ADJ_ELEMENTS);
    strat_simple_copy_pool_item(strsimp->strsimp_card_adj4, init_card_adj4);

    if (strsimp->strsimp_pool_size) {
        strsimp->strsimp_matches_per_gen = strsimp->strsimp_pool_size * strsimp->strsimp_pool_size;
        strsimp->strsimp_next_gen_size   = strsimp->strsimp_pool_size / 2;

        strsimp->strsimp_stats = New(struct strsimpstats*, strsimp->strsimp_pool_size);
        for (poolix = 0; poolix < strsimp->strsimp_pool_size; poolix++) {
            strsimp->strsimp_stats[poolix] = New(struct strsimpstats, 1);
            strat_simple_init_stat_item(gm, strsimp, strsimp->strsimp_stats[poolix]);
        }
    }

    return (estat);
}
/***************************************************************/
void strat_simple_free_stat_item(struct strsimpstats * strsimpst)
{
    Free(strsimpst->strsimpst_pool_list2);
    Free(strsimpst->strsimpst_pool_list3);
    Free(strsimpst->strsimpst_pool_list4);
    Free(strsimpst);
}
/***************************************************************/
int strat_play_simple_shut(struct game * gm, struct stratsimple * strsimp)
{
/*
** Called once for simple strategy after all play has completed
*/
    int estat = 0;
    int poolix;

    if (strsimp->strsimp_pool_size) {
        for (poolix = 0; poolix < strsimp->strsimp_pool_size; poolix++) {
            strat_simple_free_stat_item(strsimp->strsimp_stats[poolix]);
        }
        Free(strsimp->strsimp_stats);
    }

    Free(strsimp->strsimp_card_adj2);
    Free(strsimp->strsimp_card_adj3);
    Free(strsimp->strsimp_card_adj4);

    return (estat);
}
/***************************************************************/
void strat_simple_init(struct stratsimple * strsimp)
{
/*
** Called once to initialize the simple strategy
*/
    strsimp->strsimp_card_adj2          = NULL;
    strsimp->strsimp_card_adj3          = NULL;
    strsimp->strsimp_card_adj4          = NULL;
    strsimp->strsimp_pool_gen           = 0;
    strsimp->strsimp_match_num          = 0;
    strsimp->strsimp_matches_per_gen    = 0;
    strsimp->strsimp_pool_size          = 0;
    strsimp->strsimp_pool_index         = 0;
    strsimp->strsimp_next_gen_size      = 0;
}
/**************************************************************/
