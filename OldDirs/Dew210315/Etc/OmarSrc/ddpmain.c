#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include "ddplay.h"

int optionflags;
/***************************************************************/
/*
const char display_suit[]   = "CDHSN567";
const char display_player[] = "WNES";
*/
const char display_card[]   = "0x23456789TJQKA@";
extern const char display_suit[];
extern const char display_player[];

/***************************************************************/
void gettoke (char * bbuf,
                   int  * bbufix,
                   char *toke,
                   int  tokemax) {
/*
** Reads next token from bbuf, starting from bbufix.
** upshifted
*/
    int tokelen;
    int done;
    char ch;

    toke[0] = '\0';

    while (bbuf[*bbufix] &&
           isspace(bbuf[*bbufix])) (*bbufix)++;
    if (!bbuf[*bbufix]) return;

    tokelen = 0;
    done = 0;

    while (!done) {
        ch = bbuf[*bbufix];

        if (isalpha(ch) || isdigit(ch)) {
            if (tokelen + 1 < tokemax) toke[tokelen++] = toupper(ch);
            (*bbufix)++;
        } else if (ch == '+'  ||
                   ch == '-'  ||
                   ch == '.'  ||
                   ch == '@' ||
                   ch == '$'  ||
                   ch == '_') {
            if (tokelen + 1 < tokemax) toke[tokelen++] = ch;
            (*bbufix)++;
        } else {
            if (!tokelen) {
                if (tokelen + 1 < tokemax) toke[tokelen++] = ch;
                (*bbufix)++;
            }
            done = 1;
        }
    }
    toke[tokelen] = '\0';
}
/***************************************************************/
int ddplay_tab(char *buf, int num) {

   int len;
   int ii;

   len = (int)strlen(buf);
   if (len < num) {
       for (ii=len;ii<num;ii++) buf[ii]=' ';
       buf[num]=0;
       return (num);
   } else {
       return (len);
   }
}
/***************************************************************/
void ddplay_print_hands(HANDS *phands, int flags) {

    PLAYER player;
    SUIT suit;
    NCARDS cardix;
    char buf[80];
    int  len;

    if (flags & 2) {
        fprintf(stdout, "Iteration #%ld\n", gstats.st_count_winners);
    }

    player = NORTH;
    for (suit=SPADES; suit>=CLUBS; suit--) {
        buf[0] = 0;
        len = ddplay_tab(buf, 10);
        buf[len++] = display_suit[suit];
        buf[len++] = ' ';
        for (cardix=0; cardix < (*phands)[player].length[suit]; cardix++) {
            buf[len++] = display_card[(*phands)[player].cards[suit][cardix]];
        }
        buf[len++] = '\0';
        fprintf(stdout, "%s\n", buf);
    }

    for (suit=SPADES; suit>=CLUBS; suit--) {
        buf[0] = 0;
        len = ddplay_tab(buf, 4);
        player = WEST;
        while (player <= EAST) {
            buf[len++] = display_suit[suit];
            buf[len++] = ' ';
            for (cardix=0; cardix < (*phands)[player].length[suit]; cardix++) {
                buf[len++] = display_card[(*phands)[player].cards[suit][cardix]];
            }
            buf[len++] = '\0';
            len = ddplay_tab(buf, 16);
            player += 2;
        }
        fprintf(stdout, "%s\n", buf);
    }

    player = SOUTH;
    for (suit=SPADES; suit>=CLUBS; suit--) {
        buf[0] = 0;
        len = ddplay_tab(buf, 10);
        buf[len++] = display_suit[suit];
        buf[len++] = ' ';
        for (cardix=0; cardix < (*phands)[player].length[suit]; cardix++) {
            buf[len++] = display_card[(*phands)[player].cards[suit][cardix]];
        }
        buf[len++] = '\0';
        fprintf(stdout, "%s\n", buf);
    }
    fprintf(stdout, "\n");

    if (flags & 1) {
        fprintf(stdout, "Continue...");
        fgets(buf, sizeof(buf), stdin);
        if (buf[0] == 'Q' || buf[0] == 'q') exit (1);
    }
}
/***************************************************************/
void print_trick (HANDS *phands, TRICK trick, PLAYER leader, int flags) {
/*
** flags:
**    1 - Suppress header
**    2 - Suppress trick
**    4 - Show winner of trick
*/

    PLAYER player;
    char buf[100];
    int  len;
    CARD card;

    if (!(flags & 1)) {
        len = 0;
        for (player = WEST; player <= SOUTH; player++) {
            buf[len++] = ' ';
            buf[len++] = display_player[player];
            buf[len++] = ' ';
            buf[len++] = ' ';
        }
        buf[len] = '\0';
        fprintf(stdout, "%s\n", buf);
    }

    if (!(flags & 2)) {
        len = 0;
        for (player = WEST; player <= SOUTH; player++) {
            buf[len++] = ' ';
            card = (*phands)[player].cards[GETSUIT(trick[player])][GETCARD(trick[player])];
            buf[len++] = display_card[card];
            buf[len++] = display_suit[GETSUIT(trick[player])];
            if (player == leader) buf[len++] = '*';
            else                  buf[len++] = ' ';
        }
        buf[len] = '\0';
        if (flags & 4) {
            fprintf(stdout, "%s - %c\n", buf, display_player[winner(phands, trick, leader)]);
        } else {
            fprintf(stdout, "%s\n", buf);
        }
    }
}
/***************************************************************/
void test(void) {
/*
**         S
**         H AQT
**         D K2
**         C 2
**  S 4            S A
**  H KJ9          H 87
**  D 4            D 73
**  C A            C 3
**         S 2
**         H 32
**         D A6
**         C 4
**
** S to lead
*/

    NTRICKS w;
    CARD    card;

    HANDS hands = {
        { /* West */
            {1, 1, 3, 1}, {
                { A, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                { 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                { K, J, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                { 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            }},
        { /* North */
            {1, 2, 3, 0}, {
                { 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                { K, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                { A, Q, T, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            }},
        { /* East */
            {1, 2, 2, 1}, {
                { 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                { 7, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                { 8, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                { A, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            }},
        { /* South */
            {0, 3, 2, 1}, {
                { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                { A, 6, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                { 3, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                { 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            }}
    };

    gstats.st_start_time  = (float)clock();
    ddplay_print_hands(&hands, 0);
    w = playdd(&hands, SOUTH, &card);
    gstats.st_total_time = (float)clock() - gstats.st_start_time;
    fprintf(stdout, "winners=%d\n", w);

}
/***************************************************************/
int parseoptions(int argc, char *argv[], int *argix) {

    int ok;
    int done;
    int ii;

    optionflags = 0;
    ok = 1;
    done = 0;
    ii = 0;
    while (!done && (ii < argc)) {
        if (!strcmp(argv[ii], "+pause")) {
            optionflags |= OPTION_PAUSE;
        } else if (!strcmp(argv[ii], "+show")) {
            optionflags |= OPTION_SHOW;
        } else if (!strcmp(argv[ii], "+showplay")) {
            optionflags |= OPTION_SHOWPLAY;
        } else if (argv[ii][0] == '-') {
            ok = 0;
            done = 1;
        } else {
            done = 1;
        }
        ii++;
    }
    *argix = ii;

    return (ok);
}
/***************************************************************/
void init_stats(void) {

    gstats.st_count_winners = 0;
    gstats.st_start_time    = 0;
    gstats.st_total_time    = 0;
};
/***************************************************************/
void show_stats(void) {

    fprintf(stdout, "\n");
    fprintf(stdout, "Statistics:\n");
    fprintf(stdout, "count winners      = %ld\n", gstats.st_count_winners);
    fprintf(stdout, "Execution time     = %8.3f\n",
            gstats.st_total_time / CLOCKS_PER_SEC);
    fprintf(stdout, "\n");
};
/***************************************************************/
int ddplay_main(int argc, char *argv[]) {

    int ok;
    int argix;
    
    ok = parseoptions(argc, argv, &argix);
    if (!ok) {
        fprintf(stdout, "%s usage:", argv[0]);
        fprintf(stdout, "   +pause          Pause when done.\n");
        fprintf(stdout, "   +show           Show hands as they are evaluated.\n");
        fprintf(stdout, "   +showplay       Show best play of hands.\n");
        return (1);
    }

    init_stats();
    test();
    show_stats();

    if (optionflags & OPTION_PAUSE) {
        char bbuf[100];

        fprintf(stdout, "Continue...");
        fgets(bbuf, sizeof(bbuf), stdin);
    }

    return (0);
}
/***************************************************************/
