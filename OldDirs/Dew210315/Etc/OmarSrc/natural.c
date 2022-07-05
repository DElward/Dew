/***************************************************************/
/* natural.c                                                   */
/***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "config.h"
#include "omar.h"
#include "bid.h"
#include "handrec.h"
#include "util.h"
#include "onn.h"
#include "crehands.h"
#include "natural.h"

/***************************************************************/

typedef int STATS[8][5];
typedef FLOATING FSTATS[8][5];
typedef struct {
    int bid;
    FLOATING score;
} STATREC;
typedef STATREC SSTATS[38];

struct nstats { /* nst_ */
    int nst_stats[8][5];
};

struct nstatrec { /* nsr_ */
    int             nsr_bid;
    int             nsr_count;
    int             nsr_total_count;
    FLOATING        nsr_avg_score;
    FLOATING        nsr_wtd_score;
};

struct nsstats { /* nss_ */
    int             nss_nstats;
    struct nstatrec nss_stats[38];
};

struct nfstats { /* nfs_ */
    int             nfs_ncontracts;
    int             nfs_count[8][5];
    int             nfs_total_count[8][5];
    FLOATING        nfs_avg_score[8][5];
    FLOATING        nfs_wtd_score[8][5];
};

/***************************************************************/
/*@*/void shuffle(long handno, DECK *deck) {

    register int ii;
    register int r;
    register int c;

    SRAND(handno);

    for (ii = 0; ii < 51; ii++) {
       r = RAND();
       r = ii + (r % (52 - ii));
       c = (*deck)[ii];
       (*deck)[ii] = (*deck)[r];
       (*deck)[r]  = c;
    }
}
/***************************************************************/
/*@*/void deal(long handno, DECK *deck) {

    register int  ii;

    for (ii=0; ii<52; ii++) (*deck)[ii] = ii;

    shuffle(handno, deck);

}
/***************************************************************/
/*@*/void sort_hand(int* deck, HAND *hand) {

    register int suit;
    register int rank;
    register int ii;
    register int jj;
    register int *cp;

    for (suit=0; suit<4; suit++) hand->length[suit] = 0;

    for (ii=0; ii<13; ii++) {
        suit = SUIT(deck[ii]);
        rank = RANK(deck[ii]);
        jj = hand->length[suit]++;
        cp = &(hand->cards[suit][jj]);
        while (jj && *(cp - 1) < rank) {
            *cp = *(cp - 1);
            cp--;
            jj--;
        }
        *cp = rank;
    }
}
/***************************************************************/
/*@*/void sort_hands(DECK *deck, HANDS *hands) {

    register int player;
    register int cardix;

    cardix = 0;
    for (player=0; player<4; player++) {
         sort_hand(&((*deck)[cardix]), &((*hands)[player]));
         cardix += 13;
    }
}
/***************************************************************/
/*@*/void sort_suit(HANDS *hands, int suit, SSUIT *ssuit) {

    register int playerix;
    register int cardix;
    register int len;

    for (playerix = 0; playerix < 4; playerix++) {
        len = (*hands)[playerix].length[suit];
        ssuit->length[playerix] = len;

        for (cardix = 0; cardix < len; cardix++) {
            ssuit->player[(*hands)[playerix].cards[suit][cardix]] = playerix;
        }
    }
}
/***************************************************************/
/*@*/void find_suit_winners(SSUIT *ssuit, WINNERS *winners) {

    int flags[13] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    register int player;
    register int card;
    register int round;
    register int lho;
    register int winner;

    round = 1;
    winners->len = 0;
    memset(winners->num, 0, sizeof(winners->num));

    card = ACE;
    player = ssuit->player[card];
    winners->who[winners->len] = player;
    winners->which[winners->len] = card;
    winners->len++;
    winners->num[player] = 1;

    while (card > 2) {
        card--;
        player = ssuit->player[card];
        lho = LHO(player);
        if (ssuit->length[player] > round) {
            if (ssuit->length[lho] <= round) winner = 1;
            else if (ssuit->length[RHO(player)] < round) winner = 1;
            else {
                register int ii = card + 1;

                winner = 1;
                while (winner && ii <= 14) {
                    if (ssuit->player[ii] == lho && !flags[ii]) winner = 0;
                    else ii++;
                }
                if (!winner) flags[ii] = 1;
            }

            if (winner) {
                winners->who[winners->len] = player;
                winners->which[winners->len] = card;
                winners->len++;
                winners->num[player] += 1;
                round++;
            }
        }
    }
}
/***************************************************************/
/*@*/int  count_trump_contract(int trump_suit,
                               int declarer,
                               HANDS *hands,
                               WINLIST *winlist) {

    int suit;
    int dummy;
    int winners;
    int trick;
    int on_lead;
    int ix;
    int rho;
    int lho;
    int slen[4][4];
    int winix[4];
    int player;
    int winner;  /* player that won trick */
    int max;
    int n;

    dummy = PARTNER(declarer);
    lho = LHO(declarer);
    rho = RHO(declarer);
    trick = 0;

    for (suit = 3; suit >= 0; suit--) {
        winix[suit] = 0;
        for (player = 3; player >= 0; player--) {
            slen[player][suit] = (*hands)[player].length[suit];
        }
    }

/* Play each trick */
    trick = 0;
    winners = 0;
    on_lead = lho;

    while (trick < 13) {
        if (on_lead == lho || on_lead == rho) {
            max = 0;
            for (ix = 3; ix >= 0; ix--) {
                if (slen[on_lead][ix]) {
                    n = MAX((*hands)[lho].length[ix],
                            (*hands)[rho].length[ix]);
                    if (n > max) {
                        suit = ix;
                        max = n;
                    }
                }
            }
        } else {
            if ((slen[lho][trump_suit] || slen[rho][trump_suit]) &&
                 slen[on_lead][trump_suit]) suit = trump_suit;
            else {
                max = 0;
                for (ix = 3; ix >= 0; ix--) {
                    if (ix != trump_suit && slen[on_lead][ix]) {
                        n = MAX((*hands)[declarer].length[ix],
                                (*hands)[dummy].length[ix]);
                        if (n > max) {
                            suit = ix;
                            max = n;
                        }
                    }
                }
                if (!max) suit = trump_suit;
            }
        }

        if (winix[suit] < (*winlist)[suit].len) {
            winner = (*winlist)[suit].who[winix[suit]];
            winix[suit]++;
        } else {
            winner = on_lead;
        }

        player = on_lead;
        slen[player][suit]--;

        player = LHO(player);
        if (slen[player][suit]) slen[player][suit]--;
        else if (slen[player][trump_suit] &&
                 winner != PARTNER(player) &&
                 (!slen[LHO(player)][trump_suit] ||
                   slen[LHO(player)][suit])) {
            winner = player;
            slen[player][trump_suit]--;
        } else {
            max = 0;
            n = 0;
            for (ix = 3; ix >= 0; ix--) {
                if (ix != trump_suit && slen[player][ix] >= max) {
                    max = slen[player][ix];
                    n = ix;
                }
            }
            if (!max) {
                n = trump_suit;
                winner = player;
            }
            slen[player][n]--;
        }

        player = LHO(player);
        if (slen[player][suit]) slen[player][suit]--;
        else if (slen[player][trump_suit] &&
                 winner != PARTNER(player) &&
                 (!slen[LHO(player)][trump_suit] ||
                   slen[LHO(player)][suit])) {
            winner = player;
            slen[player][trump_suit]--;
        } else {
            max = 0;
            n = 0;
            for (ix = 3; ix >= 0; ix--) {
                if (ix != trump_suit && slen[player][ix] >= max) {
                    max = slen[player][ix];
                    n = ix;
                }
            }
            if (!max) {
                n = trump_suit;
                winner = player;
            }
            slen[player][n]--;
        }

        player = LHO(player);
        if (slen[player][suit]) slen[player][suit]--;
        else if (slen[player][trump_suit] &&
                 winner != PARTNER(player)) {
            winner = player;
            slen[player][trump_suit]--;
        } else {
            max = 0;
            n = 0;
            for (ix = 3; ix >= 0; ix--) {
                if (ix != trump_suit && slen[player][ix] >= max) {
                    max = slen[player][ix];
                    n = ix;
                }
            }
            if (!max) {
                n = trump_suit;
                winner = player;
            }
            slen[player][n]--;
        }
#if 0
printf("[");
for (player = 0; player < 4; player++) {
 if (player) omarout(og, "][");
 for (suit = 3; suit >= 0; suit--) {
  omarout(og, "%d", slen[player][suit]);
  if (suit) omarout(og, ",");
 }
}
printf("] ");
#endif

        if (winner == declarer || winner == dummy) {
            winners++;
        }
        on_lead = winner;
        trick++;
    }

    return (winners);
}
/***************************************************************/
/*@*/void sort_int4(int k[], int r[]) {

    register int a = k[0];
    register int b = k[1];
    register int c = k[2];
    register int d = k[3];
    register int t;

    if (b < a) {t=b;b=a;a=t;t=r[1];r[1]=r[0];r[0]=t;}
    if (d < c) {t=d;d=c;c=t;t=r[3];r[3]=r[2];r[2]=t;}

    if (c < a) {
        if (d < a)      {t=r[0];r[0]=r[2];r[2]=t;t=r[1];r[1]=r[3];r[3]=t;}
        else if (d < b) {t=r[0];r[0]=r[2];r[2]=r[3];r[3]=r[1];r[1]=t;}
        else            {t=r[0];r[0]=r[2];r[2]=r[1];r[1]=t;}
    } else {
        if (b < c)      ;
        else if (b < d) {t=r[1];r[1]=r[2];r[2]=t;}
        else            {t=r[1];r[1]=r[2];r[2]=r[3];r[3]=t;}
    }
}
/***************************************************************/
#ifdef UNUSED
void test_sort_int4(void) {

    int nerrs;
    int ntot;
    int ii;
    int jj;
    int ok;
    int int4key[4];
    int int4dat[4];
    int int4list[24][4] = {
        { 0, 1, 2, 3 }, { 0, 1, 3, 2 }, { 0, 2, 1, 3 }, { 0, 2, 3, 1 }, { 0, 3, 1, 2 }, { 0, 3, 2, 1 },
        { 1, 0, 2, 3 }, { 1, 0, 3, 2 }, { 1, 2, 0, 3 }, { 1, 2, 3, 0 }, { 1, 3, 0, 2 }, { 1, 3, 2, 0 },
        { 2, 1, 0, 3 }, { 2, 1, 3, 0 }, { 2, 0, 1, 3 }, { 2, 0, 3, 1 }, { 2, 3, 1, 0 }, { 2, 3, 0, 1 },
        { 3, 1, 2, 0 }, { 3, 1, 0, 2 }, { 3, 2, 1, 0 }, { 3, 2, 0, 1 }, { 3, 0, 1, 2 }, { 3, 0, 2, 1 }
    };

    ntot  = 0;
    nerrs = 0;

    for (ii = 0; ii < 24; ii++) {
        for (jj = 0; jj < 4; jj++) {
            int4key[jj] = int4list[ii][jj];
            int4dat[jj] = int4key[jj];
        }
        sort_int4(int4key, int4dat);

        ok = 1;
        for (jj = 0; ok && jj < 4; jj++) {
            if (int4dat[jj] != jj) ok = 0;
        }
        if (!ok) {
            nerrs++;
            printf("** Sort error on order %d: int4dat [0]=%d,[1]=%d,[2]=%d,[3]=%d\n",
                ii, int4dat[0], int4dat[1], int4dat[2], int4dat[3]);
        }
        ntot++;
    }
    if (!nerrs) {
        printf("No sort4 errors out of %d.\n", ntot);
    } else {
        printf("** %d sort4 errors out of %d.\n", nerrs, ntot);
    }
}
#endif
/***************************************************************/
/*@*/int count_nt_contract(int declarer,
                           HANDS *hands,
                           WINLIST *winlist) {

    int suit;
    int dummy;
    int winners;
    int trick;
    int us_suit;
    int us_ix;
    int them_suit;
    int them_ix;
    int on_lead;
    int ix;
    int pslen[4];
    int which[4];
    int us;
    int them;

    us = US(declarer);
    them = THEM(declarer);
    dummy = PARTNER(declarer);

    for (suit = 3; suit >= 0; suit--) {
       pslen[suit] = (*hands)[declarer].length[suit] +
                     (*hands)[dummy].length[suit];
       which[suit] = suit;
    }
/* Sort by suit length */
    sort_int4(pslen, which);

/* Play each trick */
    trick = 0;
    winners = 0;
    /* losers = 0; */
    them_suit = 0;
    them_ix = 0;
    us_suit = 3;
    us_ix = 0;
    on_lead = them;

    while (trick < 13) {
        if (on_lead == them) {
            suit = which[them_suit];
            while (them_ix >= (*winlist)[suit].len) {
               them_suit++;
               suit = which[them_suit];
               them_ix = 0;
            }
            ix = them_ix;
            them_ix++;
        } else {
            suit = which[us_suit];
            while (us_ix >= (*winlist)[suit].len) {
                us_suit--;
                suit = which[us_suit];
                us_ix = 0;
            }
            ix = us_ix;
            us_ix++;
        }

        if (((*winlist)[suit].who[ix] == declarer) ||
            ((*winlist)[suit].who[ix] == dummy)) {
            winners++;
            on_lead = us;
        } else {
            /* losers++; */
            on_lead = them;
        }
        trick++;
    }

    return (winners);
}
/***************************************************************/
/*@*/int best_suit_contract(HANDS *hands, int declarer) {

    SSUIT ssuit;
    WINLIST winlist;
    int suit;
    int tricks;
    int mtricks;
    int msuit;
    int mval;
    int dummy;
    int val;

    mval = 0;
    dummy = PARTNER(declarer);

    for (suit = 3; suit >= 0; suit--) {
        sort_suit(hands, suit, &ssuit);
        find_suit_winners(&ssuit, &(winlist[suit]));
    }

    for (suit = 3; suit >= 0; suit--) {
        if (((*hands)[declarer].length[suit] +
             (*hands)[dummy].length[suit]) > 6) {
            tricks = count_trump_contract(suit, declarer, hands, &winlist);
            if (suit >= HEARTS) val = tricks * 3;
            else                val = tricks * 2;
            if (val > mval) {
                mval = val;
                mtricks = tricks;
                msuit = suit;
            }
        }
    }

    if (mtricks>6) return encode_bid(BID, mtricks-6, msuit);
    else         return (0);
}
/***************************************************************/
/*@*/int best_nt_contract(HANDS *hands, int declarer) {

    SSUIT ssuit;
    WINLIST winlist;
    int suit;
    int dummy;
    int tricks;

    dummy = PARTNER(declarer);

    tricks = 0;
    for (suit = 3; suit >= 0; suit--) {
        sort_suit(hands, suit, &ssuit);
        find_suit_winners(&ssuit, &(winlist[suit]));
        tricks += winlist[suit].num[declarer] +
                  winlist[suit].num[dummy];
    }

    if (tricks < 7) return (0);

    tricks = count_nt_contract(declarer, hands, &winlist);

    if (tricks>6) return encode_bid(BID, tricks-6, NOTRUMP);
    else         return (0);
}
/***************************************************************/
/*@*/void best_contract(HANDS *hands, CONTRACT *contract) {

    int ew;
    int ewn;
    int ews;
    int ns;
    int nsn;
    int nss;
    int typ;
    int lev;
    int suit;

    ewn = best_nt_contract(hands, EAST);
    nsn = best_nt_contract(hands, NORTH);

    ews = best_suit_contract(hands, EAST);
    nss = best_suit_contract(hands, NORTH);

    if (ewn && (score_making(ewn, 0) >= score_making(ews, 0))) {
        ew = ewn;
    } else {
        ew = ews;
    }

    if (nsn && (score_making(nsn, 0) >= score_making(nss, 0))) {
        ns = nsn;
    } else {
        ns = nss;
    }

    if (ew > ns) {
       decode_bid(ew, &typ, &lev, &suit);
       contract->cont_level = lev;
       contract->cont_suit  = suit;
       contract->cont_doubled = 0;
       contract->cont_declarer = EAST;
    } else {
       decode_bid(ns, &typ, &lev, &suit);
       contract->cont_level = lev;
       contract->cont_suit  = suit;
       contract->cont_doubled = 0;
       contract->cont_declarer = NORTH;
    }
}
/***************************************************************/
/*@*/void shuffle3(long handno, int deck[]) {

    register int ii;
    register int r;
    register int c;

    SRAND(handno);

    for (ii = 0; ii < 38; ii++) {
       r = ii + (RAND() % (39 - ii));
       c = deck[ii];
       deck[ii] = deck[r];
       deck[r]  = c;
    }
}
/***************************************************************/
/*@*/void init3(HAND* hand, int deck[]) {

    int suit;
    int ix;
    int len;
    int rank;
    int dix;

    dix = 0;
    for (suit = 3; suit >= 0; suit--) {
        ix = 0;
        len = hand->length[suit];
        for (rank = 14; rank >= 2; rank--) {
            if (ix < len && rank == hand->cards[suit][ix]) ix++;
            else {
                deck[dix] = (rank << 2) + suit;
                dix++;
            }
        }
    }
}
/***************************************************************/
/*@*/void deal3(HAND* hand,
                int deck[],
                int declarer,
                long dealno,
                HANDS *hands) {

    int player;
    int cardix;

    shuffle3(dealno, deck);

    memcpy(&((*hands)[declarer]), hand, sizeof(HAND));

    cardix = 0;
    for (player=0; player<4; player++) {
        if (player != declarer) {
            sort_hand(&(deck[cardix]), &((*hands)[player]));
            cardix += 13;
        }
    }
}
/***************************************************************/
/*
** STATS[0][0] = Pass
** STATS[0][1] = Double
** STATS[0][2] = Redouble
*/
/***************************************************************/
/*@*/void sort_stats(FSTATS fstats, SSTATS sstats) {

    int level;
    int suit;
    int six;
    int ii;
    int jj;
    STATREC srtmp;
    
    six = 0;
    
    sstats[six].bid = encode_bid(PASS, 0, 0);
    sstats[six].score = fstats[0][0];
    six++;
    
    sstats[six].bid = encode_bid(DOUBLE, 0, 0);
    sstats[six].score = fstats[0][1];
    six++;
    
    sstats[six].bid = encode_bid(REDOUBLE, 0, 0);
    sstats[six].score = fstats[0][2];
    six++;
    
    for (level = 1; level <= 7; level++) {
        for (suit = NOTRUMP; suit >= CLUBS; suit--) {
            sstats[six].bid = encode_bid(BID, level, suit);
            sstats[six].score = fstats[level][suit];
            six++;
        }
    }

    for (ii = 0; ii < 37; ii++) {
        for (jj = ii+1; jj < 38; jj++) {
            if (sstats[ii].score < sstats[jj].score) {
                srtmp.bid = sstats[ii].bid;
                srtmp.score = sstats[ii].score;
                sstats[ii].bid = sstats[jj].bid;
                sstats[ii].score = sstats[jj].score;
                sstats[jj].bid = srtmp.bid;
                sstats[jj].score = srtmp.score;
            }
        }
    }
}
/***************************************************************/
/*@*/void print_stats(struct omarglobals * og, STATS stats) {

    int level;
    int suit;
    int n;
    FSTATS fstats;
    FLOATING ncont;
    SSTATS sstats;

    ncont = 0;
    /* Count number of contracts in stats */
    for (level = 1; level <= 7; level++) {
        for (suit = NOTRUMP; suit >= CLUBS; suit--) {
            ncont += (FLOATING)stats[level][suit];
        }
    }

    /* */
    memset(fstats, 0, sizeof(fstats));
    for (level = 7; level >= 1; level--) {
        for (suit = NOTRUMP; suit >= CLUBS; suit--) {
            if (level < 7) {
                fstats[level][suit] = fstats[level+1][suit] +
                   (FLOATING)(stats[level][suit]);
            } else {
                fstats[level][suit] =
                    (FLOATING)(stats[level][suit]);
            }
        }
    }

    /* Calculate probability of making contract */
    for (level = 1; level <= 7; level++) {
        for (suit = NOTRUMP; suit >= CLUBS; suit--) {
            fstats[level][suit] /= ncont;
        }
    }

    /* Multiply probability of making contract * score for making */
    for (level = 1; level <= 7; level++) {
        for (suit = NOTRUMP; suit >= CLUBS; suit--) {
    fstats[level][suit] *=
                   (FLOATING)score_making(encode_bid(BID, level, suit), 0);
        }
    }

    sort_stats(fstats, sstats);

    for (n = 0; n < 8; n++) {
        omarout(og, "%d. ", n+1);
        print_bid(og, sstats[n].bid);
        omarout(og, " %g\n", sstats[n].score);
    }
    
}
/***************************************************************/
/*@*/int find_natural_bid(struct omarglobals * og,
                          SEQINFO *  si,
                          AUCTION *  auction,
                          ANALYSIS * analysis,
                          HANDINFO * handinfo) {

    HANDS hands;
    int init_deck[39];
    int deck[39];
    long dealno;
    int found;
    int tofind;
    int bidder;
    int fits;
    HANDINFO handinfos[4];
    int p;
    CONTRACT contract;
    STATS stats;

    memset(&stats, 0, sizeof(stats));

    init3(handinfo->hand, init_deck);

    bidder = (auction->au_dealer + auction->au_nbids) & 3;
    dealno = 100;
    found = 0;
    tofind = 50;
    while (found < tofind) {
        memcpy(deck, init_deck, sizeof(deck));
        deal3(handinfo->hand, deck, bidder, dealno, &hands);

        p = PARTNER(bidder);
        make_handinfo(&(hands[p]), &(handinfos[p]));
        fits = check_requirements(&(handinfos[p]),
                    &(analysis->ani_ril[p]));
        if (fits) {
            p = LHO(bidder);
            make_handinfo(&(hands[p]), &(handinfos[p]));
            fits = check_requirements(&(handinfos[p]),
                        &(analysis->ani_ril[p]));
        }
        if (fits) {
            p = RHO(bidder);
            make_handinfo(&(hands[p]), &(handinfos[p]));
            fits = check_requirements(&(handinfos[p]),
                        &(analysis->ani_ril[p]));
        }

        if (fits) {
            best_contract(&hands, &contract);
            if (contract.cont_declarer == bidder ||
                contract.cont_declarer == PARTNER(bidder)) {
                stats[contract.cont_level][contract.cont_suit]++;
            }
            found++;
        }
        dealno++;
    }

    print_stats(og, stats);

    return (0);
}
/***************************************************************/
/*@*/void init4(int deck[]) {

    int suit;
    int rank;
    int dix;

    dix = 0;
    for (suit = 3; suit >= 0; suit--) {
        for (rank = 12; rank >= 0; rank--) {
            deck[dix] = (rank << 2) + suit;
            dix++;
        }
    }
}
/***************************************************************/
/*@*/void shuffle4(long handno, int deck[]) {

    register int ii;
    register int r;
    register int c;

    SRAND(handno);

    for (ii = 0; ii < 51; ii++) {
       r = ii + (RAND() % (52 - ii));
       c = deck[ii];
       deck[ii] = deck[r];
       deck[r]  = c;
    }
}
/***************************************************************/
/*@*/void deal4(int deck[],
                long dealno,
                HANDS *hands) {

    int player;
    int cardix;

    shuffle4(dealno, deck);

    cardix = 0;
    for (player=0; player<4; player++) {
        sort_hand(&(deck[cardix]), &((*hands)[player]));
        cardix += 13;
    }
}
/***************************************************************/
/*@*/void analyze_natural_bid(struct omarglobals * og, 
                      AUCTION *  auction,
                      int        bidno,
                      ANALYSIS * analysis) {

    HANDS hands;
    int init_deck[52];
    int deck[52];
    long dealno;
    int found;
    int tofind;
    int bidder;
    int fits;
    HANDINFO handinfos[4];
    int p;
    CONTRACT contract;
    STATS stats;
    int perm[24][4] = {
    {0,1,2,3}, {0,1,3,2}, {0,2,1,3}, {0,2,3,1}, {0,3,1,2}, {0,3,2,1},
    {1,0,2,3}, {1,0,3,2}, {1,2,0,3}, {1,2,3,0}, {1,3,0,2}, {1,3,2,0},
    {2,1,0,3}, {2,1,3,0}, {2,0,1,3}, {2,0,3,1}, {2,3,1,0}, {2,3,0,1},
    {3,1,2,0}, {3,1,0,2}, {3,2,1,0}, {3,2,0,1}, {3,0,1,2}, {3,0,2,1}
    };
    int permix;

    memset(&stats, 0, sizeof(stats));

    init4(init_deck);

    bidder = (auction->au_dealer + bidno) & 3;
    dealno = 100;
    found = 0;
    tofind = 2000;
    while (found < tofind) {
        memcpy(deck, init_deck, sizeof(deck));
        deal4(deck, dealno, &hands);

        for (p = 0; p < 4; p++) {
            make_handinfo(&(hands[p]), &(handinfos[p]));
        };

        for (permix = 0; permix < 24; permix++) {

            p = perm[permix][0];
            fits = check_requirements(&(handinfos[p]),
                &(analysis->ani_ril[0]));
                
            if (fits) {
                p = perm[permix][1];
                fits = check_requirements(&(handinfos[p]),
                    &(analysis->ani_ril[1]));
            }

                            
            if (fits) {
                p = perm[permix][2];
                fits = check_requirements(&(handinfos[p]),
                    &(analysis->ani_ril[2]));
            }
                
            if (fits) {
                p = perm[permix][3];
                fits = check_requirements(&(handinfos[p]),
                    &(analysis->ani_ril[3]));
            }

            if (fits) {
#if PRINTHANDS
                int bid;
#endif
                HANDS phands;
                int ii;
                
                found++;
                if (!(found & 31)) {printf("*");fflush(stdout);}

                for (ii = 0; ii < 4; ii++) {
                    memcpy(&(phands[ii]), &(hands[perm[permix][ii]]), sizeof(HAND));
                }
                best_contract(&phands, &contract);
                if (contract.cont_declarer == bidder ||
                    contract.cont_declarer == PARTNER(bidder)) {
                    stats[contract.cont_level][contract.cont_suit]++;
                    

#if PRINTHANDS
                    /* Print hands */
                    omarout(og, "%d.  ", found);
                    bid = encode_bid(BID, contract.level, contract.suit);
                    print_bid(bid);
                    omarout(og, "\n");
                    print_hands(&phands);
                    pause("-?");
#endif
                }
            }
        }
        dealno++;
    }
    
    omarout(og, "\n");
    print_stats(og, stats);
}
/***************************************************************/
/* 2018                                                        */
/***************************************************************/
void calculate_contract_makeable(struct omarglobals * og,
    int declarer,
    int suit,
    HANDS * hands,
    WINLIST * winlist,
    CONTRACT * contract)
{
    int tricks;

    if (suit == NOTRUMP) {
        tricks = 0;
        for (suit = 3; suit >= 0; suit--) {
            tricks += (*winlist)[suit].num[declarer] +
                      (*winlist)[suit].num[PARTNER(declarer)];
        }
        if (tricks > 6) {
            tricks = count_nt_contract(declarer, hands, winlist);
        }
    } else {
        tricks = count_trump_contract(suit, declarer, hands, winlist);
    }
    if (tricks > 6) {
       contract->cont_level = tricks - 6;
       contract->cont_suit  = suit;
       contract->cont_doubled = BID;
       contract->cont_declarer = declarer;
    }

}
/***************************************************************/
void calculate_makeable_contracts(struct omarglobals * og,
    HANDS * hands,
    struct makeable * mkbl)
{
    int declarer;
    int suit;
    WINLIST winlist;
    SSUIT ssuit;

    for (suit = 3; suit >= 0; suit--) {
        sort_suit(hands, suit, &ssuit);
        find_suit_winners(&ssuit, &(winlist[suit]));
    }

    for (declarer = WEST; declarer <= SOUTH; declarer++) {
        for (suit = CLUBS; suit <= NOTRUMP; suit++) {
            calculate_contract_makeable(og, declarer, suit,
                hands, &winlist,
                &(mkbl->mkbl_contract[declarer][suit]));
        }
    }
}
/***************************************************************/
void calculate_makeable(struct omarglobals * og, 
    int dealer,
    HANDS * hands,
    struct makeable * mkbl)
{
    init_makeable(mkbl);

    print_hands(og, hands);
    calculate_makeable_contracts(og, hands, mkbl);
}
/***************************************************************/
/*@*/void print_handinfo(struct omarglobals * og, int player, HANDINFO * handinfo) {

    int ii;

    print_player_fmt(og, "%-5s hand info:\n", player);
    print_hand(og, handinfo->hand);

    for (ii = 0; ii < NUM_REQUIREMENTS; ii++) {
        print_req_name(og, "%-20s: ", ii);
        omarout(og, "%g\n", handinfo->value[ii]);
    }
}
/***************************************************************/
/**** New NN routines                                          */
/***************************************************************/
static void copy_hands_to_handrec(
    struct handrec * handrec,
    HANDS *hands)
{
    int ii;

    for (ii = 0; ii < 4; ii++) {
        copy_hand(&((handrec->hr_hands)[ii]), &((*hands)[ii]));
    }
}
/***************************************************************/
void suit_contract_nn(   struct handrec * hr,
                        int declarer,
                        struct neural_network *  nn,
                        struct nstats * nst)
{
    int nout;
    double output;
    int tricks;
    int suit;

    for (suit = CLUBS; suit <= SPADES; suit++) {
        if (hr->hr_hri[declarer].hri_num_inputs[suit] == 32 && hr->hr_hri[declarer].hri_inputs[suit]) {
            nout = nnt_get_play_output(nn,
                hr->hr_hri[declarer].hri_num_inputs[suit], hr->hr_hri[declarer].hri_inputs[suit], 1, &output);
            tricks = (int)(floor(output * 16.0 + 0.5));
            if (tricks > 6 && tricks <= 13) {
                nst->nst_stats[tricks - 6][suit] += 1;
            }
#if 0
    switch (suit) {
        case CLUBS    : printf("Clubs");    break;
        case DIAMONDS : printf("Diamonds"); break;
        case HEARTS   : printf("Hearts");   break;
        case SPADES   : printf("Spades");   break;
        case NOTRUMP  : printf("Notrump");     break;
    }
            printf(" Tricks=%d\n", tricks);
#endif
        }
    }
}
/***************************************************************/
void setup_handrec_1_outputs(struct omarglobals * og,
            struct handrec * hr)
{
/*
**
*/
    int player;
    int suit;

    for (player = 0; player < 4; player++) {
        for (suit = CLUBS; suit <= NOTRUMP; suit++) {
            hr->hr_hri[player].hri_num_outputs[suit] = 1;
            if (hr->hr_hri[player].hri_outputs[suit]) Free(hr->hr_hri[player].hri_outputs[suit]);
            hr->hr_hri[player].hri_outputs[suit] = New(ONN_FLOAT, hr->hr_hri[player].hri_num_outputs[suit]);
            nnt_new_handrec_1_outputs(hr, hr->hr_hri[player].hri_num_outputs[suit], hr->hr_hri[player].hri_outputs[suit], 
                player, suit);
        }
    }
}
/***************************************************************/
void setup_suit_handrec_32_inputs(struct omarglobals * og,
            struct handrec * hr,
            int declarer,
            int suit_sorting)
{
    int suit;
    int partner;
    int suit_order[4];

    partner = PARTNER(declarer);

    for (suit = CLUBS; suit <= SPADES; suit++) {
        hr->hr_hri[declarer].hri_declarer[suit] = NULL_PLAYER;
        hr->hr_hri[declarer].hri_tricks_taken[suit] = -1;
        hr->hr_hri[partner].hri_declarer[suit] = NULL_PLAYER;
        hr->hr_hri[partner].hri_tricks_taken[suit] = -1;
    }

    for (suit = CLUBS; suit <= NOTRUMP; suit++) {
        if (hr->hr_hands[declarer].length[suit] + hr->hr_hands[partner].length[suit] > 6) {
            hr->hr_hri[declarer].hri_num_inputs[suit] = 32;
            if (hr->hr_hri[declarer].hri_inputs[suit]) Free(hr->hr_hri[declarer].hri_inputs[suit]);
            hr->hr_hri[declarer].hri_inputs[suit] = New(ONN_FLOAT, hr->hr_hri[declarer].hri_num_inputs[suit]);

            nnt_calc_suit_order_32(hr, hr->hr_hri[declarer].hri_num_inputs[suit],
                hr->hr_hri[declarer].hri_inputs[suit],
                declarer, suit, NUMBER_OF_HIGH_CARDS, suit_sorting, suit_order);

            nnt_new_handrec_32_inputs(hr, hr->hr_hri[declarer].hri_num_inputs[suit],
                hr->hr_hri[declarer].hri_inputs[suit],
                declarer, suit, NUMBER_OF_HIGH_CARDS, suit_order);

#if 0
            nnt_show_play_handrec(og, suit, 0,
                hr->hr_hri[declarer].hri_num_inputs[suit], hr->hr_hri[declarer].hri_inputs[suit]);
#endif
        }
    }
}
/***************************************************************/
void best_suit_contract_nn(struct omarglobals * og,
                        HANDS *hands,
                        int declarer,
                        struct neural_network *  nn_suited,
                        struct nstats * nst)
{

    struct handrec hr;

    /* print_player_fmt(og, "Declarer: %-5s\n", declarer); */
    /* print_hands(og, hands); */
    init_handrec(&hr);
    copy_hands_to_handrec(&hr, hands);
    setup_suit_handrec_32_inputs(og, &hr, declarer, SSORT_LONGEST);
    setup_handrec_1_outputs(og, &hr);
    suit_contract_nn(&hr, declarer, nn_suited, nst);
    free_handrec_data(&hr);
}
/***************************************************************/
void setup_notrump_handrec_32_inputs(struct omarglobals * og,
            struct handrec * hr,
            int declarer,
            int suit_sorting)
{
    int suit;
    int partner;
    int suit_order[4];

    partner = PARTNER(declarer);

    for (suit = CLUBS; suit <= SPADES; suit++) {
        hr->hr_hri[declarer].hri_declarer[suit] = NULL_PLAYER;
        hr->hr_hri[declarer].hri_tricks_taken[suit] = -1;
        hr->hr_hri[partner].hri_declarer[suit] = NULL_PLAYER;
        hr->hr_hri[partner].hri_tricks_taken[suit] = -1;
    }

    for (suit = NOTRUMP; suit <= NOTRUMP; suit++) {
        if (count_hand_hcp(&(hr->hr_hands[declarer])) + count_hand_hcp(&(hr->hr_hands[partner])) >= 20) {
            hr->hr_hri[declarer].hri_num_inputs[suit] = 32;
            if (hr->hr_hri[declarer].hri_inputs[suit]) Free(hr->hr_hri[declarer].hri_inputs[suit]);
            hr->hr_hri[declarer].hri_inputs[suit] = New(ONN_FLOAT, hr->hr_hri[declarer].hri_num_inputs[suit]);

            nnt_calc_suit_order_32(hr, hr->hr_hri[declarer].hri_num_inputs[suit],
                hr->hr_hri[declarer].hri_inputs[suit],
                declarer, suit, NUMBER_OF_HIGH_CARDS, suit_sorting, suit_order);

            nnt_new_handrec_32_inputs(hr, hr->hr_hri[declarer].hri_num_inputs[suit],
                hr->hr_hri[declarer].hri_inputs[suit],
                declarer, suit, NUMBER_OF_HIGH_CARDS, suit_order);

#if 0
            nnt_show_play_handrec(og, suit, 0,
                hr->hr_hri[declarer].hri_num_inputs[suit], hr->hr_hri[declarer].hri_inputs[suit]);
#endif
        }
    }
}
/***************************************************************/
void notrump_contract_nn(   struct handrec * hr,
                        int declarer,
                        struct neural_network *  nn,
                        struct nstats * nst)
{
    int nout;
    double output;
    int tricks;
    int suit;

    for (suit = NOTRUMP; suit <= NOTRUMP; suit++) {
        if (hr->hr_hri[declarer].hri_num_inputs[suit] == 32 && hr->hr_hri[declarer].hri_inputs[suit]) {
            nout = nnt_get_play_output(nn,
                hr->hr_hri[declarer].hri_num_inputs[suit], hr->hr_hri[declarer].hri_inputs[suit], 1, &output);
            tricks = (int)(floor(output * 16.0 + 0.5));
            if (tricks > 6 && tricks <= 13) {
                nst->nst_stats[tricks - 6][suit] += 1;
            }
            /* printf(" Tricks=%d\n", tricks); */
        }
    }
}
/***************************************************************/
void best_nt_contract_nn(struct omarglobals * og,
                        HANDS *hands,
                        int declarer,
                        struct neural_network *  nn_notrump,
                        struct nstats * nst)
{
    struct handrec hr;

    /* print_player_fmt(og, "Declarer: %-5s\n", declarer); */
    /* print_hands(og, hands); */
    init_handrec(&hr);
    copy_hands_to_handrec(&hr, hands);
    setup_notrump_handrec_32_inputs(og, &hr, declarer, SSORT_LONGEST);
    setup_handrec_1_outputs(og, &hr);
    notrump_contract_nn(&hr, declarer, nn_notrump, nst);
    free_handrec_data(&hr);
}
/***************************************************************/
void best_contract_nn(struct omarglobals * og,
                        HANDS *hands,
                        CONTRACT *contract,
                        struct neural_network *  nn_suited,
                        struct neural_network *  nn_notrump,
                        struct nstats * nst)
{
    best_suit_contract_nn(og, hands, NORTH, nn_suited, nst);
    best_nt_contract_nn(og, hands, NORTH, nn_notrump, nst);
}
/***************************************************************/
/*@*/void sort_fstats(struct nsstats * nss, struct nfstats * nfs)
{
/*
struct nstatrec {
    int             nsr_bid;
    int             nsr_count;
    int             nsr_total_count;
    FLOATING        nsr_avg_score;
    FLOATING        nsr_wtd_score;
};

struct nsstats {
    int             nss_nstats;
    struct nstatrec nss_stats[38];
};

struct nfstats {
    int             nfs_ncontracts;
    int             nfs_count[8][5];
    int             nfs_total_count[8][5];
    FLOATING        nfs_avg_score[8][5];
    FLOATING        nfs_wtd_score[8][5];
};
*/

    int level;
    int suit;
    int ii;
    int jj;
    struct nstatrec nsrtmp;
    
    nss->nss_nstats = 0;
    
    nss->nss_stats[nss->nss_nstats].nsr_bid         = encode_bid(PASS, 0, 0);
    nss->nss_stats[nss->nss_nstats].nsr_count       = nfs->nfs_count[0][0];
    nss->nss_stats[nss->nss_nstats].nsr_total_count = nfs->nfs_total_count[0][0];
    nss->nss_stats[nss->nss_nstats].nsr_avg_score   = nfs->nfs_avg_score[0][0];
    nss->nss_stats[nss->nss_nstats].nsr_wtd_score   = nfs->nfs_wtd_score[0][0];
    nss->nss_nstats++;
    
    nss->nss_stats[nss->nss_nstats].nsr_bid = encode_bid(DOUBLE, 0, 0);
    nss->nss_stats[nss->nss_nstats].nsr_count       = nfs->nfs_count[0][1];
    nss->nss_stats[nss->nss_nstats].nsr_total_count = nfs->nfs_total_count[0][1];
    nss->nss_stats[nss->nss_nstats].nsr_avg_score   = nfs->nfs_avg_score[0][1];
    nss->nss_stats[nss->nss_nstats].nsr_wtd_score   = nfs->nfs_wtd_score[0][1];
    nss->nss_nstats++;
    
    nss->nss_stats[nss->nss_nstats].nsr_bid = encode_bid(REDOUBLE, 0, 0);
    nss->nss_stats[nss->nss_nstats].nsr_count       = nfs->nfs_count[0][2];
    nss->nss_stats[nss->nss_nstats].nsr_total_count = nfs->nfs_total_count[0][2];
    nss->nss_stats[nss->nss_nstats].nsr_avg_score   = nfs->nfs_avg_score[0][2];
    nss->nss_stats[nss->nss_nstats].nsr_wtd_score   = nfs->nfs_wtd_score[0][2];
    nss->nss_nstats++;
    
    for (level = 1; level <= 7; level++) {
        for (suit = NOTRUMP; suit >= CLUBS; suit--) {
            nss->nss_stats[nss->nss_nstats].nsr_bid = encode_bid(BID, level, suit);
            nss->nss_stats[nss->nss_nstats].nsr_count       = nfs->nfs_count[level][suit];
            nss->nss_stats[nss->nss_nstats].nsr_total_count = nfs->nfs_total_count[level][suit];
            nss->nss_stats[nss->nss_nstats].nsr_avg_score   = nfs->nfs_avg_score[level][suit];
            nss->nss_stats[nss->nss_nstats].nsr_wtd_score   = nfs->nfs_wtd_score[level][suit];
            nss->nss_nstats++;
        }
    }

    for (ii = 0; ii < nss->nss_nstats - 1; ii++) {
        for (jj = ii+1; jj < nss->nss_nstats; jj++) {
            if (nss->nss_stats[ii].nsr_wtd_score < nss->nss_stats[jj].nsr_wtd_score) {
                memcpy(&nsrtmp, &(nss->nss_stats[ii]), sizeof(struct nstatrec));
                memcpy(&(nss->nss_stats[ii]), &(nss->nss_stats[jj]), sizeof(struct nstatrec));
                memcpy(&(nss->nss_stats[jj]), &nsrtmp, sizeof(struct nstatrec));
            }
        }
    }
}
/***************************************************************/
/*@*/void calc_nsstats(struct nsstats * nss, struct nstats * nst)
{

/*
struct nsstats {
    int             nss_nstats;
    struct nstatrec nss_stats[38];
};

struct nfstats {
    int             nfs_ncontracts;
    int             nfs_count[8][5];
    int             nfs_total_count[8][5];
    FLOATING        nfs_avg_score[8][5];
    FLOATING        nfs_wtd_score[8][5];
};
*/
    int level;
    int suit;
    FLOATING fncont;
    struct nfstats nfs;

    memset(nss, 0, sizeof(struct nsstats));
    memset(&nfs, 0, sizeof(struct nfstats));

    nfs.nfs_ncontracts = 0;

    /* nfs.nfs_score[0][0] = PASS       */
    /* nfs.nfs_score[0][1] = DOUBLE     */
    /* nfs.nfs_score[0][2] = REDOUBLE   */

    /* Count number of contracts in stats */
    for (level = 0; level <= 7; level++) {
        for (suit = NOTRUMP; suit >= CLUBS; suit--) {
            nfs.nfs_ncontracts += nst->nst_stats[level][suit];
            nfs.nfs_count[level][suit] = nst->nst_stats[level][suit];
            nfs.nfs_total_count[level][suit] = nst->nst_stats[level][suit];
        }
    }
    if (!nfs.nfs_ncontracts) return;

    /* */
    for (suit = NOTRUMP; suit >= CLUBS; suit--) {
        nfs.nfs_total_count[7][suit] = nst->nst_stats[7][suit];
    }
    for (level = 6; level >= 1; level--) {
        for (suit = NOTRUMP; suit >= CLUBS; suit--) {
            nfs.nfs_total_count[level][suit] =
                nfs.nfs_total_count[level+1][suit] +
                nst->nst_stats[level][suit];
        }
    }

    fncont = (FLOATING)(nfs.nfs_ncontracts);
    /* Calculate probability of making contract */
    for (level = 1; level <= 7; level++) {
        for (suit = NOTRUMP; suit >= CLUBS; suit--) {
            nfs.nfs_avg_score[level][suit] = 
                (FLOATING)(nfs.nfs_total_count[level][suit]) / fncont;
        }
    }

    /* Multiply probability of making contract * score for making */
    for (level = 1; level <= 7; level++) {
        for (suit = NOTRUMP; suit >= CLUBS; suit--) {
            nfs.nfs_wtd_score[level][suit] =
                    nfs.nfs_avg_score[level][suit] *
                   (FLOATING)score_making(encode_bid(BID, level, suit), 0);
        }
    }

    sort_fstats(nss, &nfs);

#if 0
    for (level = 0; level <= 7; level++) {
        for (suit = NOTRUMP; suit >= CLUBS; suit--) {
            printf("%6.3f ", nfs.nfs_score[level][suit]);
        }
        printf("\n");
    }
#endif
    
}
/***************************************************************/
/*@*/void print_nstats(struct omarglobals * og,
                            AUCTION *  auction,
                            struct nstats * nst)
{
/*
struct nstatrec {
    int             nsr_bid;
    int             nsr_count;
    int             nsr_total_count;
    FLOATING        nsr_avg_score;
    FLOATING        nsr_wtd_score;
};

struct nsstats {
    int             nss_nstats;
    struct nstatrec nss_stats[38];
};
 N Bid    Count TCnt Avg    Wtd
 1. 4H     111  111   0.13  56.10
 2. 2H     320  320   0.39  42.36
 3. 1H     391  391   0.47  37.64
 4. 3H     213  213   0.26  35.88


 */
    int n;
    int limit;
    int bid_permitted;
    struct nsstats nss;

    calc_nsstats(&nss, nst);

    printf("%2s %-4s %6s %6s %6s %6s\n",
        "N",
        "Bid",
        "Count",
        "TCount",
        "Avg%",
        "Wtd");

    limit = MIN(nss.nss_nstats, 12);

    for (n = 0; n < limit; n++) {
        if (nss.nss_stats[n].nsr_total_count > 0) {
            bid_permitted = is_bid_permitted(auction, nss.nss_stats[n].nsr_bid);
            if (bid_permitted) {
                omarout(og, "%2d. ", n+1);
                print_bid(og, nss.nss_stats[n].nsr_bid);
                omarout(og, " %4d"  , nss.nss_stats[n].nsr_count);
                omarout(og, " %6d"  , nss.nss_stats[n].nsr_total_count);
                omarout(og, " %6.2f", nss.nss_stats[n].nsr_avg_score * 100.0);
                omarout(og, " %6.2f", nss.nss_stats[n].nsr_wtd_score);
                omarout(og, "\n");
            }
        }
    }
}
/***************************************************************/
void show_natural_bids(struct omarglobals * og,
                            SEQINFO *  si,
                            AUCTION *  auction,
                            ANALYSIS * analysis,
                            HANDINFO * handinfo,
                            struct neural_network *  nn_suited,
                            struct neural_network *  nn_notrump)
{
/*
** See also: find_natural_bid()
*/
    HANDS hands;
    int init_deck[39];
    int deck[39];
    long dealno;
    int found;
    int tofind;
    int bidder;
    int fits;
    HANDINFO handinfos[4];
    int p;
    CONTRACT contract;
    struct nstats nst;

    memset(&nst, 0, sizeof(struct nstats));

    init3(handinfo->hand, init_deck);

    bidder = (auction->au_dealer + auction->au_nbids) & 3;
    dealno = 100;
    found = 0;
    tofind = 400; /* 50 */
    while (found < tofind) {
        memcpy(deck, init_deck, sizeof(deck));
        deal3(handinfo->hand, deck, bidder, dealno, &hands);

        p = PARTNER(bidder);
        make_handinfo(&(hands[p]), &(handinfos[p]));
        fits = check_requirements(&(handinfos[p]),
                    &(analysis->ani_ril[p]));
        if (fits) {
            p = LHO(bidder);
            make_handinfo(&(hands[p]), &(handinfos[p]));
            fits = check_requirements(&(handinfos[p]),
                        &(analysis->ani_ril[p]));
        }
        if (fits) {
            p = RHO(bidder);
            make_handinfo(&(hands[p]), &(handinfos[p]));
            fits = check_requirements(&(handinfos[p]),
                        &(analysis->ani_ril[p]));
        }

        if (fits) {
            best_contract_nn(og, &hands, &contract, nn_suited, nn_notrump, &nst);
            found++;
        }
        dealno++;
    }
    printf("\n");

    print_nstats(og, auction, &nst);
}
/***************************************************************/
