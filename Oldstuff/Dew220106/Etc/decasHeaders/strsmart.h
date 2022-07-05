/***************************************************************/
/*  strsmart.h -                                               */
/***************************************************************/
#ifndef STRSMART_H_INCLUDED
#define STRSMART_H_INCLUDED 1

struct stratsmart {   /* strsmart_ */
    count_t     strsmart_filler;
};

/***************************************************************/
int strat_play_smart_init(struct game * gm,
        struct playinfo * pli,
        struct stratsmart * strsmart);
int strat_play_smart_shut(struct game * gm, struct stratsmart * strsmart);
void strat_smart_init(struct stratsmart * strsmart);
void strat_smart_shut(struct stratsmart * strsmart);
index_t find_opportunity_smart(struct gstate * gst,
        struct opportunitylist * opl,
        struct stratsmart * strsmart,
        count_t player);
/***************************************************************/
#endif  /* STRSMART_H_INCLUDED */
/***************************************************************/
