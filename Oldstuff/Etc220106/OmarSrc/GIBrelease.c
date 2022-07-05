#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#ifdef WIN32
#include <time.h>
#else
#include <sys/times.h>
struct tms timer;
clock_t t1;
#endif

/***********************************************************************
** 03/20/2020 DTE
** When run with arguments: -qchv test.txt
** Output:

                    S  K J 5
                    H  A K Q 3
                    D  ---
                    C  J 6 5
S  Q 9 8                                S  10 7 6
H  2                                    H  J 10 9 8
D  J 10 9 8                             D  3
C  8                                    C  K 10
                    S  A 4 2
                    H  7 6 5 4
                    D  5
                    C  A 2

North to play to DQ; S trumps
Value 8 after 615859 nodes and 0.45 sec

 South can take 8 tricks.

 *******
 Contents of test.txt:

q 9 8 - 2 - q j t 9 8 - 8 - 
k j 5 - a k q 3 - - j 6 5 - 
t 7 6 - j t 9 8 - 3 - k t - 
a 4 2 - 7 6 5 4 - 5 - a 2 - 
0
0
2 q

***********************************************************************/

#define MAXLINE 100 
#define MAXARGS 10 

typedef char bool;
typedef char result;

enum { NIL, T };

typedef int goal;

#define max(x,y) (((x) > (y)) ? (x) : (y))
#define min(x,y) (((x) < (y)) ? (x) : (y))

#define nibble(i,n) (((i) >> (4 * (n))) & 0XF)

#define dotimes(x,y) for ((x) = 0 ; (x) < (y) ; (x)++)
#define suit_loop dotimes(suit,4)
#define player_loop dotimes(who,4)
#define rank_loop dotimes(rank,in_play[suit])
#define hand_loop dotimes(hand,4)

typedef struct struct_sit {
  unsigned char sin_play[4];
  unsigned short scounts[4];
  long scards[4];
  char splayer;
  char ssuit_played[4], srank_played[4];
  char sled, sleader;
  char swinning_suit, swinning_rank, swinning_player;
  bool smaybe_win;
} sit;

#define in_play (current_sit.sin_play)
#define counts (current_sit.scounts)
#define cards (current_sit.scards)
#define player (current_sit.splayer)
#define suit_played (current_sit.ssuit_played)
#define rank_played (current_sit.srank_played)
#define led (current_sit.sled)
#define leader (current_sit.sleader)
#define winning_suit (current_sit.swinning_suit)
#define winning_rank (current_sit.swinning_rank)
#define winning_player (current_sit.swinning_player)
#define maybe_win (current_sit.smaybe_win)

#define abs_player(who) (((who) + leader) & 3)
#define rel_player(who) (((who) - leader) & 3)
#define true_player abs_player(player)
#define true_partner (true_player ^ 2)
#define maximizing (true_player & 1)
#define card_count (in_play[0] + in_play[1] + in_play[2] + in_play[3])
#define adj_count (card_count - player)
#define tricks_left (card_count >> 2)
#define need_balance(g) ((g) == (maximizing ? tricks_left : 0))
#define get_owner(suit,rank) ((cards[suit] >> (rank) * 2) & 3)
#define ip(suit,who) (nibble(counts[suit],who))

enum { SAME, DIFF, EARLIER, LATER };

#define card_check(suit,lo,hi,rel,action) \
{ \
  long cds = cards[suit] >> (2 * (lo)); \
  int rank; \
  for (rank = (lo) ; rank < (hi) ; rank++) \
    { \
      if ((rel == LATER) ? (rel_player(cds & 3) > player) : \
         ((rel == EARLIER) ? (rel_player(cds & 3) < player) : \
         ((rel == DIFF) ? ((cds & 3) != true_player) : \
         ((cds & 3) == true_player)))) \
        { action; } \
      (cds >>= 2); \
    } \
}

typedef char move;

#define move_suit(move) ((move) & 3)
#define move_rank(move) ((move) >> 2)
#define extended_rank(move) (move_rank(move) & 0XF)
#define possible_winner(move) ((move) >> 6)

#define make_move(suit,rank) (((rank) << 2) + (suit))

typedef struct struct_cache {
  result ans;
} cache;

typedef struct struct_conclusion {
  cache c;
} conclusion;

#define plural(num) (((num)==1) ? "" : "s") 
char *print_conclusion(conclusion *c);      

void print_sit(void);             

void construct_situation(void); 

int successors(move succ[], goal g);
                                
int play_suit(int suit, move *moves); 
void advance(move m);          

int analyze(void);             

void expand(conclusion *c, goal g);
void adv_expand(conclusion *c, move m, goal g);
void compute(conclusion *c, goal g);
int get_arg_line(void);
bool process_line(int argc, char **myargv);
void do_work(void);

bool show_times;
bool display_long;
int clock_limit;
FILE *infile;

sit current_sit;
int trumps;
char left[4][13];
bool useleft;

char line[MAXLINE], *argptrs[MAXARGS];
bool show_hand, terminate = NIL;

#ifdef ORIGINAL
void gib_main(void)
{
  char **myargv;
  bool first;
  
  int argc = get_arg_line();
  myargv = argptrs;
  
  for (first = T ; !terminate ; first = NIL)
    {
      if (!first)
        {
          argc = get_arg_line();
          myargv = argptrs;
        }
      if (!process_line(argc,myargv)) continue;
      do_work();
    }
}
#else
void gib_main(int argc, char ** myargv)
{
  /* char bbuf[100]; */
  bool first;
    
  for (first = T ; !terminate ; first = NIL)
    {
      if (!first)
        {
          argc = get_arg_line();
          myargv = argptrs;
        }
      if (!process_line(argc,myargv)) continue;
      do_work();
    }

    /* printf("Continue...");   */
    /* gets(bbuf);              */
}
#endif

int get_arg_line()
{
  int numargs = 1, i = 0;

  printf("Enter argument line: ");
  gets(line);
  line[strlen(line)+1] = 0;
  while (line[i])
    {
      while (line[i] && isspace(line[i])) i++;
      if (line[i] == 0) break;
      argptrs[numargs++] = line + i;
      while (line[i] && !isspace(line[i])) i++;
      line[i++] = 0;
    }
  return numargs;
}

bool process_line(int argc, char **myargv)
{
  bool get_limit = NIL;
  char ch;

  show_hand = terminate = NIL;
  show_times = NIL;
  display_long = NIL;
  clock_limit = 0;
  infile = stdin;

  while (--argc > 0)
    if (get_limit)
      {
        if (!sscanf(*++myargv,"%d",&clock_limit))
          {
            fprintf(stderr,"Bad CPU limit\n");
            return NIL;
          }
        get_limit = NIL;
      }
    else if ((ch = (*++myargv)[0]) == '-')
      while (ch = *++myargv[0])
        switch (ch)
          {
          case 'c': show_times = T; break;
          case 'h': show_hand = T; break;
          case 'l': get_limit = T; break;
          case 'q': terminate = T; break;
          case 'v': display_long = T; break;
          default:
            fprintf(stderr,"Unrecognized switch: -%c\n",ch);
            return NIL;
          }
    else if (infile != stdin)
      {
        fprintf(stderr,"Too many files specified.\n");
        return NIL;
      }
    else if ((infile = fopen(myargv[0],"r")) == NULL)
      {
        fprintf(stderr,"Can't open %s\n",myargv[0]);
        return NIL;
      }
  return T;
}

void do_work()
{
  int ans;

  useleft = T;
  construct_situation();
  if (infile != stdin) fclose(infile);
  if (show_hand) print_sit();
  useleft = NIL;
  ans = analyze();
  if (display_long)
    printf("\n South can take %d trick%s.\n",ans,plural(ans));
  else if (!show_times) printf("%X",ans);
}
char *player_names[4] = {"West","North","East","South"};
char *suit_names[4] = {"S","H","D","C"};
char *card_names[13] = {"A","K","Q","J","10","9","8","7","6","5","4","3","2"};

void info_to_deal(char deal[4][4][13]);
void print_deal(char deal[4][4][13]);
void gib_print_hand(char hand[4][13], int h);
void gib_print_hands(char hand1[4][13], int h1, char hand2[4][13], int h2);
char *process_suit(char *s, char c[], int who, int suit);
void gib_print_trick(void);
bool bgets(char *line);
int read_hand(char deal[4][4][13], char held[4][4], char psuit[52],
              char prank[52]);
int find_rank (char s);
void construct_left(char deal[4][4][13], char held[4][4]);
int ncomp(const void *x, const void *y);
void find_holders(char deal[4][4][13], char held[4][4], char holders[4][13]);
void construct_sit(char held[4][4], char holders[4][13]);
void play_cards(int cds, char psuit[52], char prank[52]);

void print_sit()
{
  char deal[4][4][13];

  printf("\n");
  info_to_deal(deal);
  print_deal(deal);
  gib_print_trick();
  printf("; %s trumps\n",(trumps >= 0) ? suit_names[trumps] : "no");
}

void info_to_deal(char deal[4][4][13])
{
  int suit, rank, owner;
  char cts[4][4] = { 0 };

  suit_loop rank_loop
    {
      owner = get_owner(suit,rank);
      deal[owner][suit][cts[owner][suit]++] = rank;
    }
}

void print_deal(char deal[4][4][13])
{
  gib_print_hand(deal[1],1);
  gib_print_hands(deal[0],0,deal[2],2);
  gib_print_hand(deal[3],3);
}

void gib_print_hand(char hand[4][13], int h)
{
  char s[MAXLINE];
  int suit;

  suit_loop
    printf("%20s%s  %s\n","",suit_names[suit],
           process_suit(s,hand[suit],h,suit));
}

void gib_print_hands(char hand1[4][13], int h1, char hand2[4][13], int h2)
{
  char s[MAXLINE];
  int suit, j;

  suit_loop
    {
      printf("%s  %s",suit_names[suit],process_suit(s,hand1[suit],h1,suit));
      for (j = 37 - (s[0] ? (int)strlen(s) : 3); j > 0 ; j--) printf(" ");
      printf("%s  %s\n",suit_names[suit],process_suit(s,hand2[suit],h2,suit));
    }
}

#define display_card(suit,rank) \
  (card_names[useleft ? left[suit][rank] : rank]) 

char *empty_suit = "---";

char *process_suit(char *s, char c[], int who, int suit)
{
  int i;
  bool played = ((rel_player(who) < player) && 
                 (suit_played[who] == suit));

  s[0] = 0;
  dotimes(i,ip(suit,who))
    if (!(played && (rank_played[who] == c[i])))
      {
        strcat(s,display_card(suit,c[i]));
        strcat(s," ");
      }
  return s[0] ? s : empty_suit;
}

void gib_print_trick()
{
  int who, abs;

  printf("\n%s to %s",player_names[true_player],player ? "play to " : "lead");
  dotimes(who,player)
    {
      abs = abs_player(who);
      printf("%s%s%s",
             (((who == 0) || (suit_played[abs] != led)) ?
              suit_names[suit_played[abs]] : ""),
             display_card(suit_played[abs],rank_played[abs]),
             ((who == player - 1) ? "" : ", "));
    }
}

void construct_situation()
{
  char deal[4][4][13], held[4][4], holders[4][13];
  int cds;
  char psuit[52], prank[52];

  memset(&current_sit,0,sizeof(sit));
  cds = read_hand(deal,held,psuit,prank);
  construct_left(deal,held);
  find_holders(deal,held,holders);
  construct_sit(held,holders);
  play_cards(cds,psuit,prank);
}

bool bgets(char *line)
{
  if (!fgets(line,MAXLINE,infile)) return NIL;
  *strchr(line,'\n') = 0;
  return T;
}  
 
int read_hand(char deal[4][4][13], char held[4][4], char psuit[52],
              char prank[52])
{
  char line[MAXLINE], *piece;
  int suit, hand, num, ldr, cds = 0;

  hand_loop
    {
      suit = 0;
      bgets(line);
      num = 0;
      piece = strtok(line," ");
      do
        {
          if (piece[0] == '-')
            {
              held[hand][suit++] = num;
              num = 0;
            }
          else deal[hand][suit][num++] = find_rank(piece[0]);
        }
      while (piece = strtok(NULL," "));
    }
  fscanf(infile,"%d%d",&ldr,&trumps);
  leader = ldr;                 
  bgets(line);                  
  while (bgets(line))
    {
      psuit[cds] = line[0] - '0';
      prank[cds++] = find_rank(line[2]);
    } 
  return cds;
}

int find_rank (char s)
{
  int j = 0;

  if ((s = toupper(s)) == 'T') return 4;
  while (s != card_names[j][0]) j++;
  return j;
}

void construct_left(char deal[4][4][13], char held[4][4])
{
  int hand, suit, card;
  char *ptr;

  suit_loop
    {
      in_play[suit] = 0;
      ptr = left[suit];
      hand_loop
        {
          dotimes(card,held[hand][suit])
            *ptr++ = deal[hand][suit][card];
          in_play[suit] += held[hand][suit];
        }
      qsort(left[suit],in_play[suit],sizeof(char),ncomp);
    }
}

int ncomp(const void *x, const void *y)
{
  return *((char *) x) - *((char *) y);
}

void find_holders(char deal[4][4][13], char held[4][4], char holders[4][13])
{
  int suit, who, card, c;

  suit_loop player_loop
    dotimes(card,held[who][suit])
      {
        c = 0;
        while (left[suit][c] != deal[who][suit][card]) c++;
        holders[suit][c] = who;
      }
}

void construct_sit(char held[4][4], char holders[4][13])
{
  int suit, rank, hand;

  suit_loop
    {
      rank_loop cards[suit] += ((long) holders[suit][rank]  << (rank * 2));
      hand_loop counts[suit] += 
        ((unsigned short) held[hand][suit] << (hand * 4)); 
    }
}

void play_cards(int cds, char psuit[52], char prank[52])
{
  int num, rank;

  dotimes(num,cds)
    { 
      rank = 0;
      while (left[psuit[num]][rank] != prank[num]) rank++;
      advance(make_move(psuit[num],rank));
    }
}
void collect_trick(void);
void remove_cards(void);
void remove_card (int suit, int rank);

void advance(move m)
{
  int suit = move_suit(m);
  
  if (!player) maybe_win = possible_winner(m);
  m &= 0X3F;             
  if (player == 0) led = suit;
  if (player == 0 || ((suit == trumps) && (winning_suit != trumps)) ||
      ((suit == winning_suit) && (move_rank(m) < winning_rank)))
    {
      winning_player = true_player;
      winning_suit = suit;
      winning_rank = move_rank(m);
    }
  suit_played[true_player] = suit;
  rank_played[true_player] = move_rank(m);
  if (++player == 4) collect_trick();
}

void collect_trick()
{
  player = 0;
  remove_cards();
  leader = winning_player;
}

void remove_cards()
{
  int who, q, suit, rank;

  player_loop
    {
      suit = suit_played[who];
      rank = rank_played[who];
      counts[suit] -= (1 << (4 * who));
      dotimes(q,who)
        if ((suit_played[q] == suit) && (rank_played[q] < rank_played[who]))
          rank--;
      remove_card(suit,rank);
      in_play[suit]--;
    }
}

long mask [] = {
  0X3FFFFFF, 0X3FFFFFC, 0X3FFFFF0, 0X3FFFFC0, 0X3FFFF00, 0X3FFFC00,
  0X3FFF000, 0X3FFC000, 0X3FF0000, 0X3FC0000, 0X3F00000, 0X3C00000,
  0X3000000, 0
  };

void remove_card (int suit, int rank)
{
  long temp = cards[suit] & mask[rank+1];

  cards[suit] &= ~mask[rank];
  cards[suit] += temp >> 2;
}  
int play_any_suit(move succ[], goal g);
int discard_order(int order[4]);
int mcomp(const void *x, const void *y);
int winners(int suit,int losers);
int lead_order(int order[4], goal g);
bool compute_losers(int losers[4], goal g);
bool attain_goal(int losers[4], goal g, int total, int taken, int suit);

int successors(move succ[], goal g)
{
  return player ?
    ip(led,true_player) ? play_suit(led,succ) : play_any_suit(succ,g) :
      card_count ? play_any_suit(succ,g) : 0;
}

int play_any_suit(move succ[], goal g)
{
  int legal = 0, num, suit, order[4], ct;
  move temp;

  ct = player ? discard_order(order) : lead_order(order,g);
  dotimes(suit,ct)
    {
      if (suit) temp = succ[legal + ct - 1 - suit];
      num = play_suit(order[suit],&succ[legal + ct - 1 - suit]);
      succ[suit] = succ[legal + ct - 1 - suit];
      if (suit && (legal + ct - 1 - suit != suit))
        succ[legal + ct - 1 - suit] = temp;
      legal += num;
    }
  return legal;
}

int discard_order(int order[4])
{
  int suit, ct = 0;

  suit_loop
    if (ip(suit,true_player))
      {
        if (suit == trumps)
          if (winning_player == true_partner) order[ct] = 0;
          else if (winning_suit != trumps) order[ct] = 0X40;
          else
            {
              order[ct] = 0;
              card_check(suit,0,winning_rank,SAME,
                        { order[ct] = 0X40; break; });
            }
        else order[ct] = 
          4 * (ip(suit,true_player) + ip(suit,true_partner));
        order[ct++] += suit;
      }
  qsort(order,ct,sizeof(int),mcomp);
  dotimes(suit,ct) order[suit] &= 3;
  return ct;
}

int mcomp(const void *x, const void *y)
{
  return *((int *) y) - *((int *) x);
}

int winners(int suit,int losers)
{
  int ans = 0, lost = 0, rank, longer;
  int most = max(ip(suit,true_player),ip(suit,true_partner));
  long cds = cards[suit];

  rank_loop
    {
      if ((cds & 1) == maximizing) ans++;
      else if (lost >= losers) break;
      else lost++;
      cds >>= 2;
    }
  if (ans + lost >= most) return most - lost;
  if (trumps >= 0)
    {
      longer = ip(suit,true_player) > ip(suit,true_partner) ?
        true_player : true_partner;
      if ((ans + lost < ip(suit,longer)) && 
          (ans + lost >= ip(suit,longer ^ 2)))
        ans += min(ip(trumps,longer ^ 2),ip(suit,longer) - ans - lost);
    }
  return ans;
}

#define COMB_LEN 2
#define NUM_WON 6
#define NUM_LOST 10
#define DRAW_TRUMPS 13

int lead_order(int order[4], goal g)
{
  int suit, ct = 0;
  int losers[4];
  bool chance = compute_losers(losers,maximizing ? g : tricks_left - g + 1);

  suit_loop
    if (ip(suit,true_player))
      {
        if (suit == trumps && maximizing && (counts[trumps] & 0XF0F))
          order[ct] = 1 << DRAW_TRUMPS;
        else
          {
            order[ct] = 
              (ip(suit,true_player) + ip(suit,true_partner)) << COMB_LEN;
            if (chance)
              order[ct] += (losers[suit] << NUM_LOST) + 
                (winners(suit,losers[suit]) << NUM_WON);
          }
        order[ct++] += suit;
      }
  qsort(order,ct,sizeof(int),mcomp);
  dotimes(suit,ct) order[suit] &= 3;
  return ct;
}

bool compute_losers(int losers[4], goal g)
{
  int total;

  dotimes(total,min(4,tricks_left - g + 1))
    if (attain_goal(losers,g,total,0,0)) return T;
  return NIL;
}

bool attain_goal(int losers[4], goal g, int total, int taken, int suit)
{
  if (suit == 4) return (taken >= g);
  dotimes(losers[suit],total)
    if (attain_goal(losers,g,total - losers[suit],
                    taken + winners(suit,losers[suit]),suit + 1))
      return T;
  return NIL;
}
int play_all_ranks(int suit, int *ranks);
move *collect_move(move *place, int which, int num, int *temp,
                   int n_poss, int n_loss, int suit);
move *collect_moves(move *place, int *ranks, int first, int next,
                    int suit, bool winner);
int maybe_winner(int owner, int suit, bool pard);
bool automatic_winner(int suit, int who);
int necessary_winner(int owner, int suit, bool pard);
int get_pard_status(int suit);

enum { LOSING, MAYBE, WINNING, NOTHING, ERROR };

enum { PLOSE, PMAYBE, PWIN };

int ruff_heuristics[3] = { MAYBE, WINNING, LOSING };

int heuristics[4][3][3] = {
  WINNING, MAYBE, LOSING,      
  MAYBE, LOSING, WINNING,      
  LOSING, WINNING, MAYBE,      
  WINNING, MAYBE, LOSING,      
  MAYBE, LOSING, WINNING,      
  LOSING, WINNING, MAYBE,      
  WINNING, MAYBE, LOSING,      
  MAYBE, WINNING, LOSING,      
  LOSING, MAYBE, WINNING,      
  WINNING, LOSING, NOTHING,    
  ERROR, ERROR, ERROR,         
  LOSING, WINNING, NOTHING     
  };

int play_suit(int suit, move *moves)
{
  int ranks[13];
  int pard,num,poss,necc;
  int n_poss = -1, n_loss;
  bool ruffing;
  int i;
  
  if (!(ruffing = (player && suit == trumps && suit != led)))
    pard = get_pard_status(suit);
  num = play_all_ranks(suit,ranks);
  necc = necessary_winner(true_player,suit,NIL);
  poss = (player == 3) ? necc : maybe_winner(true_player,suit,NIL);
  n_loss = num;
  dotimes(i,num)
    {
      if (ranks[i] > poss)
        {
          n_loss = i;
          break;
        }
      if (n_poss < 0 && ranks[i] > necc) n_poss = i;
    }
  if (n_poss < 0) n_poss = n_loss;
  dotimes(i,NOTHING)
    moves = 
      collect_move(moves,
                   ruffing ? ruff_heuristics[i] :
                     heuristics[player][pard][i],
                   num,ranks,n_poss,n_loss,suit);
  moves = collect_moves(moves,ranks,n_poss,n_loss,suit,T);
  moves = collect_moves(moves,ranks,0,n_poss,suit,T);
  moves = collect_moves(moves,ranks,n_loss,num,suit,NIL);
  return num;
}

int play_all_ranks(int suit, int *ranks)
{
  int count = 0, rank;
  long cd = cards[suit] << 2;
  
  rank_loop
   {
      if ((((cd >> 2) & 3) == true_player) &&
          (!rank || (((cd & 3) != ((cd >> 2) & 3)))))
        ranks[count++] = rank;
      cd >>= 2;
    }
  return count;
}

#define make_extended_move(suit,rank,winner) \
  (((winner) << 6) + make_move(suit,rank))

move *collect_move(move *place, int which, int num, int *ranks,
                   int n_poss, int n_loss, int suit)
{
  int first, next;
  
  switch (which) {
  case NOTHING: return place;
  case WINNING: first = 0; next = n_poss; break;
  case MAYBE: first = n_poss; next = n_loss; break;
  case LOSING: first = n_loss; next = num; break;
  default: fprintf(stderr,"movegen error!\n"); exit(1);
  }
  if (first != next)
    *(place++) = make_extended_move(suit,ranks[next - 1],which != LOSING);
  return place;
}

move *collect_moves(move *place, int *ranks, int first, int next,
                    int suit, bool winner)
{
  int rank;
  
  for (rank = first ; rank < next - 1; rank++)
    *(place++) = make_extended_move(suit,ranks[rank],winner);
  return place;
}

#define check_suit \
  if (player && suit != trumps && suit != winning_suit) return -1;

#define might_play(who,rank) \
  (((rank) == rank_played[who]) || (rel_player(who) >= player))
  
#define can_beat(who) { beaten[who] = 1; count++; }
  
int maybe_winner(int owner, int suit, bool pard)
{
  char beaten[4] = { 0 };
  int count = 0;
  int who, rank;

  check_suit;
  player_loop
    if (who != owner && automatic_winner(suit,who)) can_beat(who);
  if (pard && !beaten[owner ^ 2]) can_beat(owner ^ 2);
  for (rank = in_play[suit] - 1 ; rank >= 0 ; rank--)
    if ((who = get_owner(suit,rank)) == owner)
      {
        if (count == 3) return rank;
      }
    else if (!beaten[who] && might_play(who,rank)) can_beat(who);
  return -1;
}

bool automatic_winner(int suit, int who)
{
  if (rel_player(who) < player) return suit != suit_played[who];
  else return 
    !ip(suit,who) || (player && suit == trumps && suit != led);
}

int necessary_winner(int owner, int suit, bool pard)
{
  int who, rank, winner = -1;
  int limit = (player && suit == led) ? winning_rank : in_play[suit];
  long cds = cards[suit];
  
  check_suit;
  dotimes(rank,limit)
    {
      who = cds & 3;
      cds >>= 2;
      if (who == owner) winner = rank;
      else if (pard && who == (owner ^ 2)) continue;
      else if (rel_player(who) > player)
        {
          if (suit == led || !ip(led,who)) break;
        }
      else if (suit_played[who] != suit) continue;
      else if (rank_played[who] == rank) break;
    }
  return winner;
}
  
int get_pard_status(int suit)
{
  if (player > 1)
    {
      if (winning_player != true_partner) return PLOSE;
      if (player == 3) return PWIN;
      if (!maybe_win) return PLOSE;
      if (maybe_winner(abs_player(3),suit,NIL) < 0) return PWIN;
      return PMAYBE;
    }
  if (maybe_winner(true_partner,suit,T) < 0) return PLOSE;
  if (necessary_winner(true_partner,suit,T) < 0) return PMAYBE;
  return PWIN;
}
float get_run_time(void);
void check_timer(void);
void do_kids(conclusion *c, int legal, move succ[], goal g);
bool winning_option(conclusion *c, int legal, conclusion ckid[], move succ[],
                    goal g);

unsigned long step_count;
float start_time;

float get_run_time()
{
#ifdef WIN32
  return ((float) clock())/CLOCKS_PER_SEC - start_time;
#else
  times(&timer);
  return (timer.tms_utime / 60.0) - start_time;
#endif
}

void check_timer()
{
  if ((step_count++ & 0XFFF) || !clock_limit ||
      get_run_time() < clock_limit)
    return;
  printf((show_times || display_long) ? "Time limit exceeded.\n" : "*");
  exit(0);
}

conclusion terminal;

int analyze()
{
  conclusion c;
  int lo = 0, hi = tricks_left + 1, mid;
  
  memset(&terminal,0,sizeof(conclusion));
  step_count = 0;
  start_time = 0.0;
  start_time = get_run_time();
  while (hi - lo > 1)
    {
      mid = (hi + lo)/2;
      expand(&c,mid);
      if (c.c.ans) lo = mid;
      else hi = mid;
    }
  if (show_times)
    printf("Value %d after %ld nodes and %.2f sec\n",
           lo,step_count,
           get_run_time());
  return lo;
}

void expand(conclusion *c, goal g)
{
  if (g <= 0)
    {
      *c = terminal;
      c->c.ans = T;
    }
  else if (g > tricks_left) *c = terminal;
  else compute(c,g);
}

void compute(conclusion *c, goal g)
{
  move succ[14];

  check_timer();
  do_kids(c,successors(succ,g),succ,g);
}

void do_kids(conclusion *c, int legal, move succ[], goal g)
{
  conclusion ckid[13];

  if (winning_option(c,legal,ckid,succ,g)) return;
  c->c.ans = !maximizing;
}

bool winning_option(conclusion *c, int legal, conclusion ckid[], move succ[],
                    goal g)
{
  int i;
                 
  dotimes(i,legal)
    {
      adv_expand(&ckid[i],succ[i],g);
      if (maximizing == ckid[i].c.ans)
        {
          *c = ckid[i];
          return T;
        }
    }
  return NIL;
}

void adv_expand(conclusion *c, move m, goal g)
{
  sit safe = current_sit;

  advance(m);
  if (leader & !player) g--;
  expand(c,g);
  current_sit = safe;
}
