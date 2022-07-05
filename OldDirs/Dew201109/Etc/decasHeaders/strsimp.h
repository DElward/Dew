/***************************************************************/
/*  strsimp.h -                                               */
/***************************************************************/
#ifndef STRSIMP_H_INCLUDED
#define STRSIMP_H_INCLUDED 1

#define SIMP_ADJ_ELEMENTS       14
#define SIMP_ADJ_ELEMENTS_M3    11

struct strsimpstats {   /* strsimpst_ */
    int         strsimpst_points_for;
    int         strsimpst_num_matches;
    int         strsimpst_match_avg;
    int         strsimpst_gen;
    point_t   * strsimpst_pool_list4;
    point_t   * strsimpst_pool_list3;
    point_t   * strsimpst_pool_list2;
};

struct stratsimple {   /* strsimp_ */
    int                     strsimp_pool_size;
    int                     strsimp_pool_gen;
    int                     strsimp_pool_index;
    int                     strsimp_matches_per_gen;
    int                     strsimp_next_gen_size;
    int                     strsimp_match_num;
    point_t               * strsimp_card_adj4;
    point_t               * strsimp_card_adj3;
    point_t               * strsimp_card_adj2;
    struct strsimpstats  ** strsimp_stats;
};

/***************************************************************/
/***************************************************************/
int parse_simple_strategy_subtype(struct stratsimple * strsimp,
        const char *line,
        int * lix,
        char * stoke);

void strat_simple_init(struct stratsimple * strsimp);
int strat_match_simple_init(struct game * gm, struct stratsimple * strsimp);
int strat_match_simple_begin(struct game * gm, struct stratsimple * strsimp);
int strat_match_simple_end(struct game * gm,
                    struct stratsimple * strsimp,
                    struct gscore * gsc,
                    count_t player);
int strat_match_simple_shut(struct game * gm, struct stratsimple * strsimp);
int strat_play_simple_init(struct game * gm,
        struct playinfo * pli,
        struct stratsimple * strsimp);
int strat_play_simple_shut(struct game * gm, struct stratsimple * strsimp);
void strat_simple_shut(struct stratsimple * strsimp);
int find_opportunity_simple(struct gstate * gst,
        struct opportunitylist * opl,
        struct stratsimple * strsimp,
        count_t player);

/***************************************************************/
#endif  /* STRSIMP_H_INCLUDED */
/***************************************************************/
