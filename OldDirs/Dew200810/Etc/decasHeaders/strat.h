/***************************************************************/
/*  strat.h -                                                  */
/***************************************************************/
#define STRATTYPE_NONE      0
#define STRATTYPE_DUMB      1
#define STRATTYPE_SIMPLE    2
#define STRATTYPE_SMART     3
#define STRATTYPE_BEST      4
#define STRATTYPE_CHEAT     5

/* ask human strategies */
#define STRATTYPE_MENU      17
#define STRATTYPE_PROMPT    18

/***************************************************************/
struct strategy * strategy_new(struct globals * glb);
char * get_strategy_string(struct strategy * strat, int pflags);
void print_strategy(char * stratbuf, int stratbufmax, struct strategy * strat);

int parse_strategy(struct strategy * strat,
        const char *line, int * lix, char * stoke);

int strat_match_init(struct game * gm, struct strategy * strat);
int strat_match_begin(struct game * gm, struct strategy * strat);
int strat_match_end(struct game * gm,
                    struct strategy * strat,
                    struct gscore * gsc,
                    count_t player);
int strat_match_shut(struct game * gm, struct strategy * strat);

int strat_play_init(struct game * gm, struct playinfo * pli, struct strategy * strat);
int strat_play_shut(struct game * gm, struct strategy * strat);

void strat_init(struct strategy * strat);
void strat_shut(struct strategy * strat);
int strat_start(struct strategy * strat);

int find_opportunity(struct gstate * gst,
        struct opportunitylist * opl,
        struct strategy * strat,
        count_t player,
        int * p_estat);

/***************************************************************/
