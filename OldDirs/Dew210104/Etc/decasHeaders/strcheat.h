/***************************************************************/
/*  strcheat.h -                                               */
/***************************************************************/
#ifndef STRCHEAT_H_INCLUDED
#define STRCHEAT_H_INCLUDED 1

struct stratcheat {   /* strcheat_ */
    count_t     strcheat_max_opportunities;
    count_t     strcheat_slots;
};
/***************************************************************/
int strat_play_cheat_init(struct game * gm,
        struct playinfo * pli,
        struct stratcheat * strcheat);
int find_opportunity_cheat(struct gstate * gst,
        struct opportunitylist * opl,
        struct stratcheat * strcheat,
        count_t player);
/***************************************************************/
#endif  /* STRCHEAT_H_INCLUDED */
/***************************************************************/
