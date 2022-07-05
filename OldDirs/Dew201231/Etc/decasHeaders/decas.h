/***************************************************************/
/*  decas - Copy program                                       */
/***************************************************************/
#include <limits.h>

#define VERSION "0.00"
/*
Version Date     Description
0.00    12-23-16 Began work

Date     Enhancement request
*/
/***************************************************************/
#define PROGRAM_NAME "decas"
/***************************************************************/

#define ERRNO       (errno)
#ifdef _Windows
#define GETERRNO    (GetLastError())
#else
#define GETERRNO    ERRNO
#endif

#ifdef _DEBUG
    #define IS_DEBUG    1
#else
    #define IS_DEBUG    0
#endif

#if IS_DEBUG
    #define DESSERT(c,m) {if (!(c)) program_abort(m);}
#else
    #define DESSERT(c,m)
#endif
/***************************************************************/

#define COPT_MAXPIP         0   /* Keep totals only up to max pip in hand */
#define COPT_TRAIL_INFO     0   /* Print info about maximum trail in cheat */

/***************************************************************/
#define OPT_PAUSE       1
#define OPT_HELP        2
#define OPT_VERSION     4
#define OPT_DONE        8

#define ESTAT_END           1
#define ESTAT_FOPEN         (-2800)
#define ESTAT_FGETS         (-2801)
#define ESTAT_FBUF          (-2802)
#define ESTAT_BADSTMT       (-2803)
#define ESTAT_PARMDEPTH     (-2804)
#define ESTAT_PARMFILE      (-2805)
#define ESTAT_PARMFGETS     (-2806)
#define ESTAT_PARM          (-2807)
#define ESTAT_PARMEXPFNAME  (-2808)
#define ESTAT_PARMBAD       (-2809)
#define ESTAT_BADNUM        (-2810)
#define ESTAT_HASCARDS      (-2811)
#define ESTAT_NODECK        (-2812)
#define ESTAT_BADRGEN       (-2813)
#define ESTAT_EXPEOL        (-2814)
#define ESTAT_OUT_OF_CARDS  (-2815)
#define ESTAT_MANY_CARDS    (-2816)
#define ESTAT_BADPLAYER     (-2817)
#define ESTAT_EXPCOLON      (-2818)
#define ESTAT_BADCARD       (-2819)
#define ESTAT_BADPLAYERNO   (-2820)
#define ESTAT_BADCARDCOUNT  (-2821)
#define ESTAT_BADBUILDSTART (-2822)
#define ESTAT_OUT_OF_SLOTS  (-2823)
#define ESTAT_EXPCLOSE      (-2824)
#define ESTAT_BUILDFACE     (-2825)
#define ESTAT_BUILDTOTAL    (-2825)
#define ESTAT_BUILDCARDS    (-2826)
#define ESTAT_BUILDMISMATCH (-2827)
#define ESTAT_BADSHOW       (-2828)
#define ESTAT_BADPROMPT     (-2829)
#define ESTAT_BADPLAY       (-2829)
#define ESTAT_UNSUPPSTRAT   (-2830)
#define ESTAT_BADSTRAT      (-2831)
#define ESTAT_BADNUMBER     (-2832)
#define ESTAT_BADDURATION   (-2833)
#define ESTAT_EXPGAMES      (-2834)
#define ESTAT_EXPPOINTS     (-2835)
#define ESTAT_UNSUPPDUR     (-2836)
#define ESTAT_NOCARDS       (-2837)
#define ESTAT_BADMSGS       (-2838)
#define ESTAT_EXPDOT        (-2839)
#define ESTAT_BADSTRATINFO  (-2840)
#define ESTAT_EXPEQUAL      (-2841)
#define ESTAT_NORGEN        (-2842)
#define ESTAT_FOPENOUT      (-2843)
#define ESTAT_FWRITEOUT     (-2844)
#define ESTAT_START_BEST    (-2845)
#define ESTAT_WAIT_BEST     (-2846)
#define ESTAT_DEMI_SETUP    (-2847)
#define ESTAT_CALL_BEST     (-2848)

/***************************************************************/
/*
** Globals
*/
/***************************************************************/

#define LINE_MAX_SIZE   1024
#define LINE_MAX_LEN    (LINE_MAX_SIZE - 1)

#define TOKE_MAX_SIZE   PATH_MAXIMUM
#define TOKE_MAX_LEN    (TOKE_MAX_SIZE - 1)

#define STOKE_MAX_SIZE  16  /* small token */

#define MSGOUT_SIZE       LINE_MAX_SIZE

/***************************************************************/
int set_error(int estat, const char * fmt, ...);

typedef unsigned char card_t;
typedef unsigned char count_t;
typedef unsigned char boolean_t;
typedef uint16 bitmask_t;
typedef uint32 bigbitmask_t;
typedef int16 point_t;
typedef uint16 index_t;

#define FIFTY_TWO           52
#define THIRTEEN            13
#define ACE                 1
#define TEN                 10
#define JACK                11
#define KING                13
#define TWO_OF_SPADES       11
#define TEN_OF_DIAMONDS     41
#define ACE_OF_CLUBS        4
#define KING_OF_SPADES      55

#define SUIT_CLUBS          0
#define SUIT_DIAMONDS       1
#define SUIT_HEARTS         2
#define SUIT_SPADES         3
#define LAST_TURN           48
#define LAST_ROUND          6
#define GAME_OVER_ROUND     (LAST_ROUND + 1)
#define FIRST_TURN_IN_LAST_ROUND 40

#define MAX_PLAYERS         2
#define CARDS_PER_PLAYER    4
#define MAX_CARDS_IN_BOARD  16
#define MAX_TOTLIST         31
#define MAX_CARDS_IN_SLOT   16
#define MAX_SLOTS_IN_BOARD  16
#define CARDS_DEALT_PER_PLAYER      4
#define CARDS_DEALT_INIT_BOARD      4
#define MAX_COMBOS                  256 /* enough? */
#if IS_DEBUG
    #define DEBUG_MAX_OPPORTUNITES    100
#endif

#define PLAYER_EAST         0
#define PLAYER_WEST         1
#define PLAYER_NOBODY       (MAX_PLAYERS + 1)
#define PLAYER_OPPONENT(p)  (((p) + 1) & 1)
#define PLAYER_NAME(p)\
    ((p)==PLAYER_EAST?"East":((p)==PLAYER_WEST?"West":"?name?"))
#define CARD_TO_INDEX(c) ((c) - 4)
#define INDEX_TO_CARD(x) ((x) + 4)

#define BITFOR(wv,wi,ix) for (wv=(wi),ix=0;wv;wv>>=1,ix++) if (wv&1)
#define POW2FOR(wv,wi,ix) for (wv=(1 << (wi)),ix=1;ix < wv;ix++)
#define CPARRAY(at,nt,as,ns) {memcpy(at,as,ns*sizeof(as[0]));nt=ns;}

#define SLTYPE_NONE             0
#define SLTYPE_SINGLE           1
#define SLTYPE_BUILD            2
#define SLTYPE_DOUBLED_BUILD    3

#define FLAG_PLAYER_EAST        1
#define FLAG_PLAYER_WEST        2
#define FLAG_BOARD              4
#define FLAG_BOARD_SHOW_OWNER   8
#define FLAG_PLAYER_TAKEN_EAST  16
#define FLAG_PLAYER_TAKEN_WEST  32
#define FLAG_PLAYER_DECK        64
#define GET_PLAYER_FLAG(p) ((p)?FLAG_PLAYER_WEST:FLAG_PLAYER_EAST)

#define PBUF_SIZE               128

#define GET_SUIT(c) ((c) & 3)
#define GET_PIP(c) ((c) >> 2)
#define CARD_POINTS(c)\
    (((((c)>>2)==ACE)||((c)==TWO_OF_SPADES))?1:(((c)==TEN_OF_DIAMONDS)?2:0))

#define IOTA_PER_POINT      630
#define IOTA_PER_SPADE      90
#define IOTA_PER_CARD       70
/* #define BUILD_IOTA_PENALTY  60 */

#define COPY_GSTATE(tgst,sgst) memcpy(tgst,sgst,sizeof(struct gstate))

#define PLAY_DEFAULT_RGEN           1
#define PLAY_DEFAULT_RSEED          1

struct deck { /* dk_ */
	count_t             dk_ncards;
	card_t              dk_card[FIFTY_TWO];
};


struct slot { /* sl_ */
	count_t             sl_type;
	count_t             sl_value;
	count_t             sl_owner;
	count_t             sl_ncard;
	card_t              sl_card[MAX_CARDS_IN_SLOT];
};

struct gscore { /* gsc_ */
    int gsc_tot_points[MAX_PLAYERS];
    int gsc_games_won[MAX_PLAYERS];
    int gsc_games_lost[MAX_PLAYERS];
    int gsc_games_tied[MAX_PLAYERS];
};

/* #define GST_FLAG_NO_CHECK   1 */

struct gstate { /* gst_ */
	card_t              gst_hand[MAX_PLAYERS][CARDS_PER_PLAYER];
    count_t             gst_nhand[MAX_PLAYERS];
	struct slot         gst_board[MAX_SLOTS_IN_BOARD];
    count_t             gst_last_take;
    count_t             gst_nboard;
    count_t             gst_nturns;
    count_t             gst_flags;
	card_t              gst_taken[MAX_PLAYERS][FIFTY_TWO];
    count_t             gst_ntaken[MAX_PLAYERS];
};

struct game { /* gm_ */
	count_t             gm_dealer;
	count_t             gm_round;
    struct deck *       gm_dk;
    void *              gm_rg;
    struct gstate       gm_gst;
    int                 gm_rgen;
    int                 gm_rseed;
    void *              gm_erg;
};

struct globals { /* glb_ */
    struct game *       glb_gm;
	int                 glb_flags;
	int                 glb_nparms;
	char              **glb_parms;
};

struct frec { /* fr_ */
    FILE * fr_ref;
    char * fr_buf;
    char * fr_name;
    char * fr_desc;
    int    fr_bufsize;
    int    fr_buflen;
    int    fr_linenum;
    int    fr_eof;
};
/***************************************************************/
#define MSGLEVEL_OFF                0
#if IS_DEBUG
#define MSGLEVEL_DEBUG_SHOWBEFORE   1
#define MSGLEVEL_DEBUG_SHOWAFTER    2
#define MSGLEVEL_DEBUG_SHOWDEALS    4
#define MSGLEVEL_DEBUG_SHOWOPS      8
#endif
#define MSGLEVEL_DEBUG              16
#define MSGLEVEL_VERBOSE            (MSGLEVEL_DEBUG     << 1)
#define MSGLEVEL_DEFAULT            (MSGLEVEL_VERBOSE   << 1)
#define MSGLEVEL_TERSE              (MSGLEVEL_DEFAULT   << 1)
#define MSGLEVEL_QUIET              (MSGLEVEL_TERSE     << 1)

#define MSGLEVEL_QUIET_MASK     MSGLEVEL_QUIET
#define MSGLEVEL_TERSE_MASK     (MSGLEVEL_TERSE     | MSGLEVEL_QUIET_MASK)
#define MSGLEVEL_DEFAULT_MASK   (MSGLEVEL_DEFAULT   | MSGLEVEL_TERSE_MASK)
#define MSGLEVEL_VERBOSE_MASK   (MSGLEVEL_VERBOSE   | MSGLEVEL_DEFAULT_MASK)
#define MSGLEVEL_DEBUG_MASK     (MSGLEVEL_DEBUG     | MSGLEVEL_VERBOSE_MASK)
/***************************************************************/
void program_abort(const char * msg);
void gettoke(const char * bbuf, int * bbufix, char *toke, int tokemax);
int deck_card_pr(char * buf, card_t * cards, int ncards, char delim);
int deck_slot_pr(char * buf, struct slot * sl, char sdelim, int pflags);
void gstate_print(int msglevel, struct gstate * gst, int pflags);
int shuffle_parm(const char *shuf, int * rgen, int * rseed);
void strmcpy(char * tgt, const char * src, size_t tmax);
int stoi (const char * from, int *num);
int stoui (const char * from, int *num);
int is_msglevel(int msglevel);
void msgout(int msglevel, const char * fmt, ...);
int deal_cards(struct deck * dk, card_t * cards, count_t * ccount, count_t ncards);
void qdeal_cards(struct deck * dk, card_t * cards, count_t * ccount, count_t ncards);
int game_deal_next_round(struct game * gm);
void deck_shuffle(void * rg, struct deck * dk);
int game_deal_first_round(struct game * gm);
int deck_board_pr(char * buf, struct slot board[], int nsl, int pflags);
void deck_reset_shuffle(void * rg, struct deck * dk);
void show_cardlist(int msglevel, const char * pfx, card_t * cards, count_t ncards);
void card_show(
    int msglevel,
	struct slot * g_board,
    count_t       g_nboard,
	card_t      * g_hand,
    count_t       g_nhand,
    count_t       g_player);
void init_opportunitylist(struct opportunitylist * opl);
void free_opportunities(struct opportunitylist * opl);
#if IS_DEBUG
void dbg_set_max_opportunites(int maxop);
#endif
/***************************************************************/

