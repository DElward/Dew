#define SRAND srand
#define RAND  rand

typedef float FLOATING;

/* Mini functions */
#define SUIT(card) (card & 3)
#define RANK(card) (card >> 2)
#define DECLARER(player) ((player) & 3)
#define PARTNER(player) (((player) + 2) & 3)
#define LHO(player) (((player) + 1) & 3)
#define RHO(player) (((player) + 3) & 3)
#define US(player) ((player) & 1)
#define THEM(player) (((player) + 1) & 1)
#define MIN(i,j) ((i)<(j)?(i):(j))
#define MAX(i,j) ((i)>(j)?(i):(j))
#define MAKECARD(rank,suit) ((rank) << 2 | (suit))

/* Constants */
#define MAX_BIDS 317     /* 1 + (3 * 15) * 7 + 1 = 317 */

typedef int DECK[52];

typedef struct hand {
    int length[4];       /* Number of cards in each suit */
    int cards[4][13];    /* Cards for each suit */
} HAND;

typedef HAND HANDS[4];

typedef struct {
    int length[4];       /* Length of suit for each player */
    int player[15];      /* Player holding each card */
} SSUIT;   /* Sorted suit */

typedef struct {
    int num[4];          /* Number of winners for each player */
    int len;             /* Number of winning tricks */
    int who[13];         /* Winner of each trick */
    int which[13];       /* Card that won trick */
} WINNERS;

typedef struct CONTRACT { /* cont_ */
    int cont_level;
    int cont_suit;           /* enum e_bid_suit    */
    int cont_doubled;        /* enum e_bid_special */
    int cont_declarer;       /* enum e_player */
} CONTRACT;

typedef WINNERS WINLIST[4];

/* Constants */
enum e_face_card        {TEN=10, JACK=11, QUEEN=12, KING=13, ACE=14};
enum e_pair             {EAST_WEST, NORTH_SOUTH};
enum e_bid_suit         {CLUBS, DIAMONDS, HEARTS, SPADES, NOTRUMP};
enum e_bid_special      {NONE, BID, PASS, DOUBLE, REDOUBLE, NOBID, BIDQUERY};
enum e_player           {WEST, NORTH, EAST, SOUTH, NULL_PLAYER=7};
enum e_vulnerability    {VUL_NONE, VUL_NS, VUL_EW, VUL_BOTH};
#define ALLPASS_LEVEL   8
#define AUCTION_DONE_LEVEL   8

#define UNKNOWN_PLAY_CARD   1
#define QUERY_PLAY_CARD     2
#define DONE_PLAY_CARD      3

/*
**  DEALER[] = {NORTH, EAST, SOUTH, WEST};
**  NSVUL[]  = { 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0 };
**  EWVUL[]  = { 0, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 0, 1 };
*/

enum SUIT {
    NO_SUIT,
    CLUB_SUIT,
    DIAMOND_SUIT,
    HEART_SUIT,
    SPADE_SUIT,
    NT_SUIT,
    SUIT_SUIT
};

typedef struct auction {    /* au_ */
    int au_dealer;
    int au_vulnerability;
    int au_nbids;
    int au_bid[MAX_BIDS];
} AUCTION;

struct desclist {
    struct biddesc * bdesc;
    struct desclist * nextlist;
};
enum {
    SUIT_SET,
    FORCE_SET,
    AGREE_SET,
    ZZZZ_SET
};
enum {
    PTS_REQ,
    HCP_REQ,
    DPTS_REQ,
    TRICKS_REQ,
    QT_REQ,
    SLEN_REQ,
    SHCP_REQ,
    ZZZ_REQ
};

enum REQUIREMENT {
    REQ_PTS,            /* 00 */
    REQ_HCP,            /* 01 */
    REQ_DPTS,           /* 02 */
    REQ_TRICKS,         /* 03 */
    REQ_QT,             /* 04 */
    REQ_SLEN_CLUBS,     /* 05 */
    REQ_SLEN_DIAMONDS,  /* 06 */
    REQ_SLEN_HEARTS,    /* 07 */
    REQ_SLEN_SPADES,    /* 08 */
    REQ_SHCP_CLUBS,     /* 09 */
    REQ_SHCP_DIAMONDS,  /* 10 */
    REQ_SHCP_HEARTS,    /* 11 */
    REQ_SHCP_SPADES,    /* 12 */
    NUM_REQUIREMENTS    /* 13 */
};

typedef struct {
    FLOATING lovalue;
    FLOATING hivalue;
} REQUIREMENT;

enum FORCE {NO_FORCE, NON_FORCING, FORCING, GAME_FORCE};

typedef struct {
    enum SUIT suit[4];  /* SUIT1, SUIT2, ... */
    enum SUIT agree;    /* agreed upon trump suit */
    enum FORCE forcing;
    REQUIREMENT requirement[NUM_REQUIREMENTS];
} REQINFO;

typedef struct {
    REQINFO * headreqinfo;
    REQINFO * tailreqinfo;
} REQINFOLIST;

typedef REQINFOLIST REQLIST[4];

typedef struct analysis_info {  /* ani_ */
    struct desclist * desc[MAX_BIDS];
    REQLIST ani_ril;
} ANALYSIS;

struct sysdesc {
    char * sysname;
    long   nbids;
    struct biddesc ** bids;
    struct sysdesc *  nextsys;
};

struct convcard {
    struct sysdesc *ccs[2];
};

typedef struct seqinfo {
    int    longestseq;
    struct biddesc * headdesc;
    struct biddesc * taildesc;
    struct sysdesc * headsys;
    struct sysdesc * tailsys;
    struct convcard * cc;
} SEQINFO;

enum BIDTYPE {
    NO_BIDTYPE,
    BEGIN_BIDTYPE,
    PASS_BIDTYPE,
    DBL_BIDTYPE,
    RDBL_BIDTYPE,
    LEVEL_BIDTYPE,
    ANY_BIDTYPE,
    DESC_BIDTYPE,
    SEXP_BIDTYPE,
    ZZZZ_BIDTYPE
};
enum SUITTYPE {
    NO_SUITTYPE,
    CLUBS_SUITTYPE,
    DIAMONDS_SUITTYPE,
    HEARTS_SUITTYPE,
    SPADES_SUITTYPE,
    NOTRUMP_SUITTYPE,
    MINOR_SUITTYPE,
    MAJOR_SUITTYPE,
    TRUMP_SUITTYPE,
    ANY_SUITTYPE,
    ZZZZ_SUITTYPE
};
enum {SUIT0, SUIT1, SUIT2, SUIT3, SUIT4};
enum {NOONE, MY, LHO, PARD, RHO, UNBID};
struct suitexp {
    int relop;
    int suitspec;
};
union defu {
    int               stype;
    struct biddesc  * desc;
    struct suitexp  * sexp;
};
struct defulist {
    union defu        bu;
    union defu      * next;
};
struct biddef {
    enum BIDTYPE      bidtype;
    int               bidlevel;
    union defu        bu;
    struct biddef   * anddef;
    struct biddef   * next;      /* Next biddef in bidseq */
};
struct program {
    int proglen;
    int progmax;
    int progptr;
    char * buffer;
};
struct bidlist {
    struct program * req_prog;
    struct program * set_prog;
    struct program * score_prog;
    struct program * test_prog;
    struct bidlist * nextlist;
};
struct biddesc {
    char * bidname;
    int              nbseqs;
    struct bidseq ** bidseqs;
    struct biddesc * nextdesc;
    struct bidlist * blist;
    struct bidlist * btail;
};
struct bidseq {
    struct biddef * bd;
    int nbids;
    struct bidseq * nextseq;
};
enum RELOP {NA, EQ, LT, LE, GT, GE, NE, IN};
enum KEYWD {NO_KEYWD   , PTS_KEYWD  , HCP_KEYWD  , SLEN_KEYWD, SHCP_KEYWD,
            TRICKS_KEYWD, QT_KEYWD, DPTS_KEYWD};

#define STACK_SIZE  16

typedef struct {
    int dtype;
} OPER;
typedef struct {
    int err;
    int tos;
    OPER oper[STACK_SIZE];
} OPSTACK;
enum {NO_DTYPE, FLOAT_DTYPE, BOOL_DTYPE, SUIT_DTYPE};
enum {
    NO_OP,
    BF_OP,
    BFDEL_OP,
    BR_OP,
    BT_OP,
    ADD_OP,
    DIV_OP,
    END_OP,
    EQ_OP,
    GE_OP,
    GT_OP,
    LE_OP,
    LOADI_OP,
    LOADS_OP,
    LT_OP,
    MOD_OP,
    MUL_OP,
    NE_OP,
    IN_OP,
    NEG_OP,
    NOT_OP,
    SUB_OP,

    WEST_OP,
    NORTH_OP,
    EAST_OP,
    SOUTH_OP,

    DPTS_FUNC,
    HCP_FUNC,
    LONGEST_FUNC,
    PTS_FUNC,
    QT_FUNC,
    SHCP_FUNC,
    SLEN_FUNC,
    SNUM_FUNC,
    SUPPORT_FUNC,
    TOP_FUNC,
    TRICKS_FUNC,
    MAKES_FUNC,
    ZZZZ_OP
};

typedef struct handinforec {
    int trace;
    HAND * hand;
    FLOATING value[NUM_REQUIREMENTS];
} HANDINFO;

typedef struct {
    int trace;
    HAND * hand;
    int value[NUM_REQUIREMENTS];
    int ordered_suit_lengths[4];
    int ordered_suits[4];
} IHANDINFO;

typedef struct {
    FLOATING score;
    int bidtype;
    int bidlevel;
    int bidsuit;
} BIDSCORE;

#define NULL_SCORE (-9999)

extern char display_suit[];
extern char display_player[];
extern char display_rank[];

/***************************************************************/
int encode_bid(int bid_type, int bid_level, int bid_suit);
void decode_bid(int bid,
                int *bid_type,
                int *bid_level,
                int *bid_suit);
int bid_is_pass(int bid);
int encode_contract(
    int c_level,
    int c_suit,           /* enum e_bid_suit    */
    int c_doubled,        /* enum e_bid_special */
    int c_declarer);      /* enum e_player */
void decode_contract(int econtract,
                     int *c_level,
                     int *c_suit,
                     int *c_doubled,
                     int *c_declarer);
int alldigits(char * buf, int buflen);
struct biddesc * find_desc(SEQINFO *si, char * descname);
struct sysdesc * find_sys(SEQINFO *si, char * sysname);
void app_prog(struct program * prog, char * ptr, int len);
void append_prog(struct program * prog, int n);
void append_prog_int(struct program * prog, int n);
void append_prog_double(struct program * prog, FLOATING n);
void update_prog_int(struct program * prog, int p, int n);
int fetch_prog(struct program * prog);
int fetch_prog_int(struct program * prog);
FLOATING fetch_prog_double(struct program * prog);
int interp_suitspec(int suitspec, int bidder, REQLIST * reqinfo);
int is_num(char * buf);
int is_func(char * token);
struct biddesc * newdesc(SEQINFO * si, char * seqname);
struct biddesc *  store_seq(SEQINFO * si,
                            char * seqname,
                            int    which,
                            int    nbseqs,
                            struct bidseq ** bidseqs);
void make_bid(AUCTION * auction, int bt, int bl, int bs);
int desc_matches(struct biddesc * desc,
                      struct desclist * list);
#ifdef PUT_BACK
/*@*/int def_matches(struct biddef * bd,
                     AUCTION *  auction,
                     int        bidix,
                     ANALYSIS * analysis);
#endif
int seq_matches(struct bidseq * seq,
                AUCTION *  auction,
                int        bidix,
                ANALYSIS * analysis);
void apply_req(struct program * prog,
               REQUIREMENT * req);
void apply_req_prog(struct program * prog,
                    int bid,
                    int bidder,
                    REQLIST * reqinfo);
void apply_requirements(struct desclist * desc,
                        int bid,
                        int bidder,
                        REQLIST * reqinfo);
void apply_set(struct program *prog,
               int bid,
               int bidder,
               REQLIST * reqinfo);
void apply_set_prog(struct program * prog,
                    int bid,
                    int bidder,
                    REQLIST * reqinfo);
void apply_sets(struct desclist * desc,
                int bid,
                int bidder,
                REQLIST * reqinfo);
void make_handinfo(HAND * hand, HANDINFO * handinfo);
int count_top(HAND * hand, int suit, int topn);
void sort_int4(int k[], int r[]);
int find_longest(HAND * hand, int longestn);
ANALYSIS * new_analysis(void);
void reset_analysis(ANALYSIS * analysis);
void free_analysis(ANALYSIS * analysis);
void next_avail_bid(AUCTION * auction, int *bt, int *bl, int *bs, int *dbl);

int check_requirements(HANDINFO * handinfo, REQINFOLIST * ril);
int calc_support(HAND * hand, int trumpsuit);
int parse_force(char * token);
int calc_unbid(int unbid, REQLIST * reqinfo);

int score_making(int bid, int vul);
int tab(char *buf, int num);
void print_hands(struct omarglobals * og, HANDS *hands);
char * hands_to_chars(HANDS *hands);
void shuffle(long handno, DECK *deck);
void sort_suit(HANDS *hands, int suit, SSUIT *ssuit);
void find_suit_winners(SSUIT *ssuit, WINNERS *winners);
int  count_trump_contract(int trump_suit,
                          int declarer,
                          HANDS *hands,
                          WINLIST *winlist);
void sort_int4(int k[], int r[]);
int count_nt_contract(int declarer,
                      HANDS *hands,
                      WINLIST *winlist);
int best_suit_contract(HANDS *hands, int declarer);
int best_nt_contract(HANDS *hands, int declarer);
void best_contract(HANDS *hands, CONTRACT * contract);
void shuffle3(long handno, int deck[]);
void init3(HAND* hand, int deck[]);
void deal3(HAND* hand,
           int deck[],
           int declarer,
           long dealno,
           HANDS *hands);
void pause(char * text);

/* updated 03/04/2018 */
void print_bid(struct omarglobals * og, int bid);
int parse_bids(struct omarglobals * og, struct frec * biddef_fr, SEQINFO * si);
void print_seqinfo(struct omarglobals * og, SEQINFO * si);
void analyze_bidfile(struct omarglobals * og, struct frec * bidtest_fr, SEQINFO *si);

#define EXPFLAG_HANDREC     1
#define EXPFLAG_HANDINFO    2

void parse_exp(
        struct omarglobals * og,
        struct infile * inf,
        char * token,
        struct program *prog,
        OPSTACK * ops,
        int expflags);
FLOATING run_prog(
        struct omarglobals * og,
        struct program * prog,
        AUCTION * auction,
        ANALYSIS * analysis,
        HANDINFO * handinfo,
        struct handrec * hr);
int next_card(
        struct omarglobals * og,
        const char * bidbuf,
        int *ptr,
        int *card,
        int *suit,
        int * current_suit);
int next_bid(
        struct omarglobals * og,
        const char * bidbuf,
        int *ptr,
        int *bt,
        int *bl,
        int *bs);
void print_player(struct omarglobals * og, int player);
int parse_hand(
struct omarglobals * og,
    const char * bidbuf,
    int * ptr,
    HAND * hand);
void print_hand(struct omarglobals * og, HAND * hand);
char * hand_to_chars(HAND * hand);
char * auction_to_chars(AUCTION * auction);
void print_auction(struct omarglobals * og, AUCTION * auction);
void init_cc(struct omarglobals * og, SEQINFO * si);
void analyze_auction(
                    struct omarglobals * og,
                    SEQINFO *  si,
                    AUCTION *  auction,
                    ANALYSIS * analysis,
                    int anat);
int find_best_bid(struct omarglobals * og,
                       SEQINFO *  si,
                       AUCTION *  auction,
                       ANALYSIS * analysis,
                       HANDINFO * handinfo);
void print_ebid(struct omarglobals * og, int ebid);
char * ebid_to_chars(int ebid);
void free_cc(SEQINFO * si);
void analyze_bid(
                    struct omarglobals * og,
                    SEQINFO *  si,
                    AUCTION *  auction,
                    int        bidno,
                    ANALYSIS * analysis,
                    int anat);
void print_score_prog(FILE * outf, struct program * prog);
void print_req_prog(FILE * outf, struct program * prog);

/* 03/03/2018 */
int next_pair(const char * bidbuf, int *ptr, int *pair);
int next_player(const char * bidbuf, int *ptr, int *player);
void print_vulnerability(struct omarglobals * og, int vulnerability);
void get_vulnerability(char * vulstr, int vulnerability);
AUCTION * new_auction(int dealer, int vulnerability);
void make_ebid(AUCTION * auction, int ebid);
void print_bid_suit(struct omarglobals * og, const char * fmt, int bidsuit);
void print_display_player(struct omarglobals * og, int player);
void print_contract(struct omarglobals * og, CONTRACT * contract);

/* 04/01/2018 */
SEQINFO * new_seqinfo(void);
void free_seqinfo(SEQINFO * si);

void print_analysis(struct omarglobals * og,
        AUCTION * auction,
        ANALYSIS * analysis);
void print_req_name(struct omarglobals * og,
                            const char * fmt,
                            int req);
void print_player_fmt(struct omarglobals * og, const char * fmt, int player);

/* 06/01/2018 */
int is_bid_permitted(AUCTION *  auction, int ebid);
void next_legal_bid(AUCTION *  auction,     /* in  */
                    CONTRACT * contract,    /* out */
                    int      * double_bid); /* out */
void current_contract(AUCTION *  auction,   /* in  */
                      CONTRACT *contract,   /* out */
                      int * more_bids);     /* out */
void free_program(struct program * prog);
struct infile * infile_new_chars(const char * istr);
void next_token(struct infile * inf, char * toke);
void infile_free(struct infile * inf, int close_fr);

/***************************************************************/
