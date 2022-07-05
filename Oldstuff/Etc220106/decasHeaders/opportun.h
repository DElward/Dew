/***************************************************************/
/*  opportun.h -                                               */
/***************************************************************/

#define OPTYP_TRAIL             1
#define OPTYP_TAKE              2
#define OPTYP_BUILD_SINGLE      3
#define OPTYP_BUILD_DOUBLE      4
#define OPTYP_BUILD_MORE_DOUBLE 5

struct opportunityitem { /* opi_ */
	count_t             opi_type;
	count_t             opi_cardix;
	count_t             opi_player;
	count_t             opi_bdlvalue;
    bitmask_t           opi_slotmask;
#if IS_DEBUG
	int                 opi_opiota;
#endif
};

struct opportunitylist { /* opl_ */
    struct opportunityitem ** opl_opi;
	int                       opl_nopi;
	int                       opl_xopi;
#if IS_DEBUG
	int                       opl_emsg;
#endif
};
/***************************************************************/
int show_opportunityitem(
    char * slotbuf,
    struct opportunityitem * opi,
	struct slot * g_board,
	card_t      * g_hand);
void game_opportunities(
    struct opportunitylist * opl,
	struct slot * g_board,
    count_t       g_nboard,
	card_t      * g_hand,
    count_t       g_nhand,
    count_t       g_player);
void show_opportunities(
    struct opportunitylist * opl,
	struct slot * g_board,
    count_t       g_nboard,
	card_t      * g_hand,
    count_t       g_nhand,
    count_t       g_player);

/***************************************************************/

