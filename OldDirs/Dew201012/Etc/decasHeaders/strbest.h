/***************************************************************/
/*  strbest.h -                                               */
/***************************************************************/
#ifndef STRBEST_H_INCLUDED
#define STRBEST_H_INCLUDED 1

#define BEST_DEFAULT_MIN_TRIES      16

#define STRBEST_SHOW_FLAG_SHOWTOTS      1
#define STRBEST_SHOW_FLAG_SHOWEACHTOT   2

struct stratbest {   /* strbest_ */
    void *      strbest_erg;
    int         strbest_rgen;
    int         strbest_rseed;
    void *      strbest_rg;
    int         strbest_show_flag;
    int         strbest_mintries;
    int         strbest_shuf_rgen;
    int         strbest_shuf_rseed;
    void *      strbest_shuf_rg;
    struct demi_boss * strbest_demb;
    void *      strbest_demf_calc_opportunity_iota_best;
};

/***************************************************************/
int parse_best_strategy_subtype(struct stratbest * strbest,
        const char *line,
        int * lix,
        char * stoke);
int strat_play_best_init(struct game * gm,
        struct playinfo * pli,
        struct stratbest * strbest);
void strat_best_init(struct stratbest * strbest);
void strat_best_shut(struct stratbest * strbest);
index_t find_opportunity_best(struct gstate * gst,
        struct opportunitylist * opl,
        struct stratbest * strbest,
        count_t player,
        int * p_estat);
int strat_start_best(struct stratbest * strbest);
/***************************************************************/
#endif  /* STRBEST_H_INCLUDED */
/***************************************************************/
