/***************************************************************/
/* natural.h                                                   */
/***************************************************************/

void deal(long handno, DECK *deck);
void sort_hand(int* deck, HAND *hand);
void sort_hands(DECK *deck, HANDS *hands);

/***************************************************************/
int find_natural_bid(struct omarglobals * og,
                          SEQINFO *  si,
                          AUCTION *  auction,
                          ANALYSIS * analysis,
                          HANDINFO * handinfo);
void analyze_natural_bid(struct omarglobals * og, 
                      AUCTION *  auction,
                      int        bidno,
                      ANALYSIS * analysis);
void calculate_makeable(struct omarglobals * og, 
    int dealer,
    HANDS * hands,
    struct makeable * mkbl);

/* 05/29/2018 */
void show_natural_bids(struct omarglobals * og,
                            SEQINFO *  si,
                            AUCTION *  auction,
                            ANALYSIS * analysis,
                            HANDINFO * handinfo,
                            struct neural_network *  nn_suited,
                            struct neural_network *  nn_notrump);
/***************************************************************/
