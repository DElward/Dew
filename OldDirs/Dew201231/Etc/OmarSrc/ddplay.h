/***************************************************************/
/*
** ddplay.h
*/
/***************************************************************/
#define MAX(x,y) ((x)>(y)?(x):(y))
#define MIN(x,y) ((x)<(y)?(x):(y))
/***************************************************************/
typedef signed char byt;
typedef unsigned char uby;
/***************************************************************/
typedef struct {
    uby length[4];       /* Number of cards in each suit */
    uby cards[4][13];    /* Cards for each suit */
} HAND;

typedef HAND HANDS[4];
typedef byt PLAYER;
typedef byt SUIT;
typedef byt PIP;
typedef byt NCARDS;
typedef byt NTRICKS;
typedef byt NSUITS;
typedef uby CARD;  /* (PIP << 4) & SUIT */
typedef CARD TRICK[4];

#define PARTNER(player) (PLAYER)((player + 2) & 3)
#define LHO(player) (PLAYER)((player + 1) & 3)
#define RHO(player) (PLAYER)((player + 3) & 3)
#define MAKECARD(pip, suit) (((pip) << 4) | (suit))
#define GETCARD(card) ((card) >> 4)
#define GETSUIT(card) ((card) & 3)
#define CARDSINHAND0(hands) ((hands)[0].length[0] + (hands)[0].length[1] + \
                             (hands)[0].length[2] + (hands)[0].length[3])

#define A           14
#define K           13
#define Q           12
#define J           11
#define T           10
#define WEST        0
#define NORTH       1
#define EAST        2
#define SOUTH       3
#define CLUBS       0
#define DIAMONDS    1
#define HEARTS      2
#define SPADES      3
#define NOTRUMP     4
#define NILSUIT     7
#define NILCARD     15
/***************************************************************/
/* Constant arrays */
#define OPTION_PAUSE    1
#define OPTION_SHOW     2
#define OPTION_SHOWPLAY 4
/***************************************************************/
struct struct_stat {
    long  st_count_winners;
    float st_start_time;
    float st_total_time;
} gstats;
/***************************************************************/
/* main.c */
void print_trick (HANDS *phands, TRICK trick, PLAYER leader, int flags);
void print_hands(HANDS *phands, int flags);
/***************************************************************/
/* ddplay.c */
NTRICKS count_winners(HANDS *phands, PLAYER leader, CARD *best_card);
NTRICKS playdd(HANDS *phands, PLAYER leader, CARD *best_card);
PLAYER winner(HANDS *phands, TRICK trick, PLAYER leader);
NTRICKS count_winners_ab(HANDS *phands, PLAYER leader, CARD *best_card,
                NTRICKS minew, NTRICKS minns, int depth);
/***************************************************************/
/*
Transpostion table entry (16 bytes):

bits   0 -   3  number of spades
       4 -   7  number of hearts
       8 -  11  number of diamonds
      12 -  15  number of clubs
      16 -  41  location of spades 
      42 -  67  location of hearts
      68 -  93  location of diamonds
      94 - 119  location of clubs
     120 - 123  lead
     124 - 127  number of tricks

     bits   0-123 comprise the key
     bits 124-127 comprise the data
*/
