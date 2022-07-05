/***************************************************************/
/* handrec.h                                                   */
/***************************************************************/

struct makeable { /* mkbl_ */
    int         mkbl_ns_optimum;
    CONTRACT    mkbl_contract[4][5];    /* 4 declarers, 5 suits */
};

struct results { /* rslt_ */
    int                 rslt_nresultrec;
    struct resultrec ** rslt_aresultrec;
};

struct handrecinfo { /* hri_ */
    /* nn inputs/outputs for CLUBS, DIAMONDS, HEARTS, SPADES, NOTRUMP */
    int         hri_num_inputs[5];
    int         hri_num_outputs[5];
    double *    hri_inputs[5];
    double *    hri_outputs[5];
    int         hri_declarer[5];     /* enum e_player */
    int         hri_econtract[5];
    int         hri_tricks_taken[5];
};

struct handrec { /* hr_ */
    int     hr_estat;
    char *  hr_fname;
    char *  hr_url;
    int     hr_acbl_board_no;
    int     hr_dealer;
    int     hr_vulnerability;
    struct hand   hr_hands[4];
    struct makeable hr_makeable;
    struct results hr_results;
    struct handrecinfo hr_hri[4]; /* one for each declarer */
};

/***************************************************************/
struct handrec * parse_handrec(struct omarglobals * og, const char * hrfname);
struct handrec ** parse_handrecdir(struct omarglobals * og,
    const char * hrfname,
    const char * hrreq);
void print_handrec(struct omarglobals * og, struct handrec * hr);
void bid_handrec(struct omarglobals * og, struct handrec * hr, struct seqinfo * si);
void free_handrec_data(struct handrec * hr);
void free_handrec(struct handrec * hr);
void add_handrec(struct omarglobals * og, struct handrec * hr);
void check_contracts(struct omarglobals * og);
void init_makeable(struct makeable * mkbl);
void copy_hand(struct hand * tgt_hand, struct hand * src_hand);
int chars_to_vulnerability(const char * vulstr);
int char_to_player(char dlrchar);
void init_handrec(struct handrec * hr);
int calculate_makes(struct omarglobals * og,
                struct handrec * hr,
                int player,
                int suit);
/***************************************************************/
