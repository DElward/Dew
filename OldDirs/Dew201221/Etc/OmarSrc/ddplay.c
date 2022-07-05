#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "ddplay.h"

extern int optionflags;
extern const char display_card[];
extern const char display_suit[];
extern const char display_player[];

void ddplay_print_hands(HANDS *phands, int flags);

/***************************************************************/
PLAYER winner(HANDS *phands, TRICK trick, PLAYER leader) {

    PLAYER who;
    PLAYER maxix;
    CARD card;
    CARD maxcard;
    SUIT suit;

    suit = GETSUIT(trick[leader]);
    maxcard = (*phands)[leader].cards[suit][GETCARD(trick[leader])];
    maxix = leader;

    who = LHO(leader);
    while (who != leader) {
        if (GETSUIT(trick[who]) == suit) {
            card = (*phands)[who].cards[suit][GETCARD(trick[who])];
            if (card > maxcard) {
                maxix = who;
                maxcard = card;
            }
        }
        who = LHO(who);
    }

    return (maxix);
}
/***************************************************************/
void remove_trick(HANDS *phands, TRICK  trick, HANDS *ph) {

    PLAYER player;
    SUIT   suit;
    NCARDS ix;
    NCARDS len;
    NCARDS cardix;

    for (player = WEST; player <= SOUTH; player++) {
        for (suit = CLUBS; suit <= SPADES; suit++) {
            len = (*phands)[player].length[suit];
            if (GETSUIT(trick[player]) == suit) {
                cardix = GETCARD(trick[player]);
                for (ix = 0; ix < len; ix++) {
                    if (ix < cardix) {
                        (*ph)[player].cards[suit][ix] = (*phands)[player].cards[suit][ix];
                    } else if (ix > cardix) {
                        (*ph)[player].cards[suit][ix-1] = (*phands)[player].cards[suit][ix];
                    }
                }
                (*ph)[player].length[suit] = len - 1;
            } else {
                (*ph)[player].length[suit] = len;
                for (ix = 0; ix < len; ix++) {
                    (*ph)[player].cards[suit][ix] = (*phands)[player].cards[suit][ix];
                }
            }
        }
    }
}
/***************************************************************/
void copy_hands(HANDS *phands, HANDS *ph) {

    PLAYER player;
    SUIT   suit;
    NCARDS ix;
    NCARDS len;

    for (player = WEST; player <= SOUTH; player++) {
        for (suit = CLUBS; suit <= SPADES; suit++) {
            len = (*phands)[player].length[suit];
            (*ph)[player].length[suit] = len;
            for (ix = 0; ix < len; ix++) {
                (*ph)[player].cards[suit][ix] = (*phands)[player].cards[suit][ix];
            }
        }
    }
}
/***************************************************************/
NTRICKS what_card_to_play(HANDS *phands,
                           HANDS    *ph,
                           PLAYER   leader,
                           PLAYER   turn,
                           SUIT     suit_led,
                           TRICK    trick,
                           CARD*    card) {
/*
** Returns number of tricks won by NS
*/
    NCARDS  try_ix;
    NTRICKS try_max;
    NTRICKS tricks;
    CARD    best_card;
    CARD    try_best;
    SUIT    losuit;
    SUIT    hisuit;
    SUIT    suit;
    PLAYER who_won;

    if (leader == turn) {
        losuit = CLUBS;
        hisuit = SPADES;
    } else {
        if ((*phands)[turn].length[suit_led]) {
            losuit = suit_led;
            hisuit = suit_led;
        } else {
            losuit = CLUBS;
            hisuit = SPADES;
        }
    }
    try_best = MAKECARD(NILCARD, NILSUIT);
    if (turn & 1) try_max = -1;
    else          try_max = 14;

    for (suit = losuit; suit <= hisuit; suit++) {
        if (leader == turn) suit_led = suit;

        try_ix = (*phands)[turn].length[suit] - 1;

        while (try_ix >= 0) {
            trick[turn] = MAKECARD(try_ix, suit);
            if (LHO(turn) != leader) {
                tricks = what_card_to_play(phands, ph, leader, LHO(turn), suit_led,
                                            trick, &best_card);
                if ( (turn & 1) && (tricks > try_max) ||
                    !(turn & 1) && (tricks < try_max)) {
                    try_max  = tricks;
                    try_best = trick[turn];
                }
            } else {

                who_won = winner(phands, trick, leader);
                tricks = (who_won & 1);

                if (CARDSINHAND0(*phands) > 1) {
                    remove_trick(phands, trick, ph);
                    tricks += count_winners(ph, who_won, &best_card);
                }

                if ( (turn & 1) && (tricks > try_max) ||
                    !(turn & 1) && (tricks < try_max)) {
                    try_max  = tricks;
                    try_best = trick[turn];
                }
            }
            try_ix--;
        }
    }
    *card = try_best;

    return (try_max);
}
/***************************************************************/
NTRICKS count_winners(HANDS *phands, PLAYER leader, CARD *best_card) {

    NTRICKS m;
    TRICK  trick;
    HANDS  ph;

    gstats.st_count_winners++;

    if (optionflags & OPTION_SHOW) {
        fprintf(stdout, "Leader: %c\n", display_player[leader]);
        ddplay_print_hands(phands, 3);
    }

    m = what_card_to_play(phands, &ph, leader, leader, 0, trick, best_card);

    return (m);
}
/***************************************************************/
void print_play(HANDS *phands, PLAYER leader, CARD card) {

    NTRICKS m;
    TRICK  trick;
    HANDS  ph;
    PLAYER player;
    PLAYER who_won;
    CARD best_card;
    SUIT suit_led;
    HANDS hands;
    NCARDS ncards;

    who_won = leader;
    best_card = card;
    copy_hands(phands, &hands);
    ncards = CARDSINHAND0(hands);

    print_trick(&hands, trick, who_won, 2);

    while (ncards > 0) {

        player = who_won;
        suit_led = GETSUIT(best_card);
        trick[player] = best_card;
        player = LHO(player);

        while (player != who_won) {
            m = what_card_to_play(&hands, &ph, who_won, player, suit_led, trick, &best_card);
            trick[player] = best_card;
            player = LHO(player);
        }

        print_trick(&hands, trick, who_won, 1);
        who_won = winner(&hands, trick, leader);
        remove_trick(&hands, trick, &ph);

        ncards = CARDSINHAND0(hands);

        if (ncards > 0) {
            copy_hands(&ph, &hands);
            count_winners(&hands, who_won, &best_card);
        }
    }
}
/***************************************************************/
NTRICKS what_card_to_play_ab(HANDS *phands,
                           HANDS    *ph,
                           PLAYER   leader,
                           PLAYER   turn,
                           SUIT     suit_led,
                           TRICK    trick,
                           CARD*    card,
                           NTRICKS  minns,
                           NTRICKS  maxns,
                           int depth) {
/*
** Returns number of tricks won by NS
*/
    NCARDS  try_ix;
    NTRICKS try_max;
    NTRICKS tricks;
    CARD    best_card;
    CARD    try_best;
    SUIT    losuit;
    SUIT    hisuit;
    SUIT    suit;
    PLAYER  who_won;
    NCARDS  ncards;
    NTRICKS mn;
    NTRICKS mx;

    if (leader == turn) {
        losuit = CLUBS;
        hisuit = SPADES;
    } else {
        if ((*phands)[turn].length[suit_led]) {
            losuit = suit_led;
            hisuit = suit_led;
        } else {
            losuit = CLUBS;
            hisuit = SPADES;
        }
    }
    try_best = MAKECARD(NILCARD, NILSUIT);
    if (turn & 1) try_max = 0;
    else          try_max = 13;

    ncards = CARDSINHAND0(*phands);

    for (suit = losuit; suit <= hisuit; suit++) {
        if (leader == turn) suit_led = suit;

        try_ix = (*phands)[turn].length[suit] - 1;

        while (try_ix >= 0) {
            trick[turn] = MAKECARD(try_ix, suit);
            if (LHO(turn) != leader) {
                tricks = what_card_to_play_ab(phands, ph, leader, LHO(turn), suit_led,
                                            trick, &best_card, minns, maxns, depth);
                if (turn & 1) { /* maximizing */
                    if (tricks > try_max) {
                        try_max = tricks;
                        try_best = trick[turn];
                        minns = try_max;
                    }
                } else { /* minimizing */
                    if (tricks < try_max) {
                        try_max  = tricks;
                        try_best = trick[turn];
                        maxns = try_max;
                    }
                }
            } else {
                who_won = winner(phands, trick, leader);
                tricks = (who_won & 1);

                if (ncards > 1) {
                    mn = minns;
                    mx = maxns;

                    remove_trick(phands, trick, ph);
                    tricks += count_winners_ab(ph, who_won, &best_card,
                                mn, mx, depth+1);
                    if (turn & 1) { /* maximizing */
                        if (tricks > try_max) {
                            try_max = tricks;
                            try_best = trick[turn];
                            minns = try_max;
                        }
                    } else { /* minimizing */
                        if (tricks < try_max) {
                            try_max  = tricks;
                            try_best = trick[turn];
                            maxns = try_max;
                        }
                    }

                    if (depth < 0) {
                        printf("%d. %c%c = %d tricks\n", depth,
                            display_card[(*phands)[turn].cards[suit][try_ix]],
                            display_suit[suit], tricks);
                    }
                }
            }

            try_ix--;
        }
    }
    *card = try_best;

    return (try_max);
}
/***************************************************************/
NTRICKS count_winners_ab(HANDS *phands, PLAYER leader, CARD *best_card,
                        NTRICKS minns, NTRICKS maxns, int depth) {

    NTRICKS m;
    TRICK  trick;
    HANDS  ph;

    gstats.st_count_winners++;

    if (optionflags & OPTION_SHOW) {
        fprintf(stdout, "Leader: %c\n", display_player[leader]);
        ddplay_print_hands(phands, 3);
    }

    m = what_card_to_play_ab(phands, &ph, leader, leader, 0, trick, best_card,
                            minns, maxns, depth);

    return (m);
}
/***************************************************************/
void print_play_ab(HANDS *phands, PLAYER leader, CARD card) {

    NTRICKS m;
    TRICK  trick;
    HANDS  ph;
    PLAYER player;
    PLAYER who_won;
    CARD best_card;
    SUIT suit_led;
    HANDS hands;
    NCARDS ncards;

    who_won = leader;
    best_card = card;
    copy_hands(phands, &hands);
    ncards = CARDSINHAND0(hands);

    print_trick(&hands, trick, who_won, 2);

    while (ncards > 0) {

        player = who_won;
        suit_led = GETSUIT(best_card);
        trick[player] = best_card;
        player = LHO(player);

        while (player != who_won) {
            m = what_card_to_play_ab(&hands, &ph, who_won, player,
                        suit_led, trick, &best_card,
                        (NTRICKS)(-1), (NTRICKS)(ncards + 1), 0);
            trick[player] = best_card;
            player = LHO(player);
        }

        print_trick(&hands, trick, who_won, 1);
        who_won = winner(&hands, trick, leader);
        remove_trick(&hands, trick, &ph);

        ncards = CARDSINHAND0(ph);

        if (ncards > 0) {
            copy_hands(&ph, &hands);
            count_winners_ab(&hands, who_won, &best_card,
                (NTRICKS)(-1), (NTRICKS)(ncards + 1), 0);
        }
    }
}
/***************************************************************/
NTRICKS playdd(HANDS *phands, PLAYER leader, CARD *best_card) {

    NTRICKS m;
    NCARDS ncards;

    ncards = CARDSINHAND0(*phands);
    m = count_winners_ab(phands, leader, best_card,
        (NTRICKS)(-1), (NTRICKS)(ncards + 1), 0);

    /* m = count_winners(phands, leader, best_card); */

    if (optionflags & OPTION_SHOWPLAY) {
        fprintf(stdout, "Leader: %c\n", display_player[leader]);
        print_play_ab(phands, leader, *best_card);
    }

    return (m);
}
/***************************************************************/
