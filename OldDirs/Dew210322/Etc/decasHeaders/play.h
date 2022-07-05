/***************************************************************/
/*  play.h -                                                   */
/***************************************************************/
#define PLAY_DURATION_TURN          1   /* 1 player gets one play */
#define PLAY_DURATION_ROUND         2   /* Each player plays 4 turns (until out of cards) */
#define PLAY_DURATION_GAME          3   /* Each player plays 6 round (full game) */
#define PLAY_DURATION_POINT_MATCH   4   /* Play games until 1 player gets X points */
#define PLAY_DURATION_GAME_MATCH    5   /* Play X games */

/* These two approximated from experimentation */
#define CHEAT_MAX_OPPORTUNITIES   14
#define CHEAT_MAX_SLOTS           9

struct playinfo {   /* pli_ */
    count_t         pli_player;
    int             pli_msglevel;
    int             pli_rgen;
    int             pli_rseed;
    void *          pli_rg;
    count_t         pli_play_duration;
    int             pli_play_num_duration;
    int             pli_play_matches;
    struct strategy * pli_strat[MAX_PLAYERS];
};

/***************************************************************/
int play_prompt(struct globals * glb, struct gstate * gst, count_t player, int playflags);
int play_play(struct game * gm,
            struct playinfo * pli);
void show_opportunityitem_played(
    struct gstate * gst,
    struct opportunityitem * opi,
    count_t player,
    count_t siflags,
    count_t depth);
int find_opportunity_menu(struct gstate * gst,
        struct opportunitylist * opl,
        count_t player);
point_t calc_card_iota(card_t card);
void play_opportunity(struct gstate * gst, struct opportunityitem * opi, count_t player);
void create_remaining_deck_from_cards(struct gstate * gst, struct deck * remdk, count_t player);
void calc_opponent_gstate(
    struct gstate * opp_gst,
	card_t      * g_hand,
    count_t       g_nhand,
	struct slot * g_board,
    count_t       g_nboard,
    count_t       g_last_take,
    count_t       g_nturns,
    count_t player);

/***************************************************************/

