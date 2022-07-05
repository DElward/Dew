#ifndef PBNH_INCLUDED
#define PBNH_INCLUDED
/***************************************************************/
/* pbn.h                                                       */
/***************************************************************/

/*** BEGIN Generated in Excel 11/22/2018 8:19 ************************/
#define PBN_TAG_ID_ACTIONTABLE         1
#define PBN_TAG_ID_ANNOTATOR           2
#define PBN_TAG_ID_ANNOTATORNA         3
#define PBN_TAG_ID_APPLICATION         4
#define PBN_TAG_ID_AUCTION             5
#define PBN_TAG_ID_AUCTIONTIMETABLE    6
#define PBN_TAG_ID_BIDSYSTEMEW         7
#define PBN_TAG_ID_BIDSYSTEMNS         8
#define PBN_TAG_ID_BOARD               9
#define PBN_TAG_ID_COMPETITION         10
#define PBN_TAG_ID_CONTRACT            11
#define PBN_TAG_ID_DATE                12
#define PBN_TAG_ID_DEAL                13
#define PBN_TAG_ID_DEALID              14
#define PBN_TAG_ID_DEALER              15
#define PBN_TAG_ID_DECLARER            16
#define PBN_TAG_ID_DESCRIPTION         17
#define PBN_TAG_ID_EAST                18
#define PBN_TAG_ID_EASTNA              19
#define PBN_TAG_ID_EASTTYPE            20
#define PBN_TAG_ID_EVENT               21
#define PBN_TAG_ID_EVENTDATE           22
#define PBN_TAG_ID_EVENTSPONSOR        23
#define PBN_TAG_ID_FRENCHMP            24
#define PBN_TAG_ID_GENERATOR           25
#define PBN_TAG_ID_HIDDEN              26
#define PBN_TAG_ID_HOMETEAM            27
#define PBN_TAG_ID_INSTANTSCORETABLE   28
#define PBN_TAG_ID_MODE                29
#define PBN_TAG_ID_NORTH               30
#define PBN_TAG_ID_NORTHNA             31
#define PBN_TAG_ID_NORTHTYPE           32
#define PBN_TAG_ID_NOTE                33
#define PBN_TAG_ID_OPTIMUMPLAYTABLE    34
#define PBN_TAG_ID_OPTIMUMRESULTTABLE  35
#define PBN_TAG_ID_OPTIMUMSCORE        36
#define PBN_TAG_ID_PAIREW              37
#define PBN_TAG_ID_PAIRNS              38
#define PBN_TAG_ID_PLAY                39
#define PBN_TAG_ID_PLAYTIMETABLE       40
#define PBN_TAG_ID_RESULT              41
#define PBN_TAG_ID_ROOM                42
#define PBN_TAG_ID_ROUND               43
#define PBN_TAG_ID_SCORE               44
#define PBN_TAG_ID_SCOREIMP            45
#define PBN_TAG_ID_SCOREMP             46
#define PBN_TAG_ID_SCOREPERCENTAGE     47
#define PBN_TAG_ID_SCORERUBBER         48
#define PBN_TAG_ID_SCORERUBBERHISTORY  49
#define PBN_TAG_ID_SCORETABLE          50
#define PBN_TAG_ID_SCORING             51
#define PBN_TAG_ID_SECTION             52
#define PBN_TAG_ID_SITE                53
#define PBN_TAG_ID_SOUTH               54
#define PBN_TAG_ID_SOUTHNA             55
#define PBN_TAG_ID_SOUTHTYPE           56
#define PBN_TAG_ID_STAGE               57
#define PBN_TAG_ID_TABLE               58
#define PBN_TAG_ID_TERMINATION         59
#define PBN_TAG_ID_TIME                60
#define PBN_TAG_ID_TIMECALL            61
#define PBN_TAG_ID_TIMECARD            62
#define PBN_TAG_ID_TIMECONTROL         63
#define PBN_TAG_ID_TOTALSCORETABLE     64
#define PBN_TAG_ID_UTCDATE             65
#define PBN_TAG_ID_UTCTIME             66
#define PBN_TAG_ID_VISITTEAM           67
#define PBN_TAG_ID_VULNERABLE          68
#define PBN_TAG_ID_WEST                69
#define PBN_TAG_ID_WESTNA              70
#define PBN_TAG_ID_WESTTYPE            71

#define PBN_TAG_NAME_ACTIONTABLE         "ActionTable"
#define PBN_TAG_NAME_ANNOTATOR           "Annotator"
#define PBN_TAG_NAME_ANNOTATORNA         "AnnotatorNA"
#define PBN_TAG_NAME_APPLICATION         "Application"
#define PBN_TAG_NAME_AUCTION             "Auction"
#define PBN_TAG_NAME_AUCTIONTIMETABLE    "AuctionTimeTable"
#define PBN_TAG_NAME_BIDSYSTEMEW         "BidSystemEW"
#define PBN_TAG_NAME_BIDSYSTEMNS         "BidSystemNS"
#define PBN_TAG_NAME_BOARD               "Board"
#define PBN_TAG_NAME_COMPETITION         "Competition"
#define PBN_TAG_NAME_CONTRACT            "Contract"
#define PBN_TAG_NAME_DATE                "Date"
#define PBN_TAG_NAME_DEAL                "Deal"
#define PBN_TAG_NAME_DEALID              "DealId"
#define PBN_TAG_NAME_DEALER              "Dealer"
#define PBN_TAG_NAME_DECLARER            "Declarer"
#define PBN_TAG_NAME_DESCRIPTION         "Description"
#define PBN_TAG_NAME_EAST                "East"
#define PBN_TAG_NAME_EASTNA              "EastNA"
#define PBN_TAG_NAME_EASTTYPE            "EastType"
#define PBN_TAG_NAME_EVENT               "Event"
#define PBN_TAG_NAME_EVENTDATE           "EventDate"
#define PBN_TAG_NAME_EVENTSPONSOR        "EventSponsor"
#define PBN_TAG_NAME_FRENCHMP            "FrenchMP"
#define PBN_TAG_NAME_GENERATOR           "Generator"
#define PBN_TAG_NAME_HIDDEN              "Hidden"
#define PBN_TAG_NAME_HOMETEAM            "HomeTeam"
#define PBN_TAG_NAME_INSTANTSCORETABLE   "InstantScoreTable"
#define PBN_TAG_NAME_MODE                "Mode"
#define PBN_TAG_NAME_NORTH               "North"
#define PBN_TAG_NAME_NORTHNA             "NorthNA"
#define PBN_TAG_NAME_NORTHTYPE           "NorthType"
#define PBN_TAG_NAME_NOTE                "Note"
#define PBN_TAG_NAME_OPTIMUMPLAYTABLE    "OptimumPlayTable"
#define PBN_TAG_NAME_OPTIMUMRESULTTABLE  "OptimumResultTable"
#define PBN_TAG_NAME_OPTIMUMSCORE        "OptimumScore"
#define PBN_TAG_NAME_PAIREW              "PairEW"
#define PBN_TAG_NAME_PAIRNS              "PairNS"
#define PBN_TAG_NAME_PLAY                "Play"
#define PBN_TAG_NAME_PLAYTIMETABLE       "PlayTimeTable"
#define PBN_TAG_NAME_RESULT              "Result"
#define PBN_TAG_NAME_ROOM                "Room"
#define PBN_TAG_NAME_ROUND               "Round"
#define PBN_TAG_NAME_SCORE               "Score"
#define PBN_TAG_NAME_SCOREIMP            "ScoreIMP"
#define PBN_TAG_NAME_SCOREMP             "ScoreMP"
#define PBN_TAG_NAME_SCOREPERCENTAGE     "ScorePercentage"
#define PBN_TAG_NAME_SCORERUBBER         "ScoreRubber"
#define PBN_TAG_NAME_SCORERUBBERHISTORY  "ScoreRubberHistory"
#define PBN_TAG_NAME_SCORETABLE          "ScoreTable"
#define PBN_TAG_NAME_SCORING             "Scoring"
#define PBN_TAG_NAME_SECTION             "Section"
#define PBN_TAG_NAME_SITE                "Site"
#define PBN_TAG_NAME_SOUTH               "South"
#define PBN_TAG_NAME_SOUTHNA             "SouthNA"
#define PBN_TAG_NAME_SOUTHTYPE           "SouthType"
#define PBN_TAG_NAME_STAGE               "Stage"
#define PBN_TAG_NAME_TABLE               "Table"
#define PBN_TAG_NAME_TERMINATION         "Termination"
#define PBN_TAG_NAME_TIME                "Time"
#define PBN_TAG_NAME_TIMECALL            "TimeCall"
#define PBN_TAG_NAME_TIMECARD            "TimeCard"
#define PBN_TAG_NAME_TIMECONTROL         "TimeControl"
#define PBN_TAG_NAME_TOTALSCORETABLE     "TotalScoreTable"
#define PBN_TAG_NAME_UTCDATE             "UTCDate"
#define PBN_TAG_NAME_UTCTIME             "UTCTime"
#define PBN_TAG_NAME_VISITTEAM           "VisitTeam"
#define PBN_TAG_NAME_VULNERABLE          "Vulnerable"
#define PBN_TAG_NAME_WEST                "West"
#define PBN_TAG_NAME_WESTNA              "WestNA"
#define PBN_TAG_NAME_WESTTYPE            "WestType"
/*** END Generated in Excel 11/22/2018 8:19 **************************/

#define ACTIONTABLE_FIELDS \
    "Rank;" \
    "Call;" \
    "Card;" \
    "Declarer;" \
    "Contract;" \
    "Percentage;"

#define AUCTIONTIMETABLE_FIELDS \
    "Time;" \
    "TotalTime;"

#define INSTANTSCORETABLE_FIELDS \
    "Rank;" \
    "ScoreRange_NS;" \
    "ScoreRange_EW;" \
    "IMP_NS;" \
    "IMP_EW;" \
    "Percentage_NS;" \
    "Percentage_EW;"

#define OPTIMUMPLAYTABLE_FIELDS \
    "S;" \
    "H;" \
    "D;" \
    "C;"

#define OPTIMUMRESULTTABLE_FIELDS \
    "Declarer;" \
    "Denomination;" \
    "Result;"

#define PLAYTIMETABLE_FIELDS \
    "Time;" \
    "TotalTime;"

#define SCORETABLE_FIELDS \
    "Rank;" \
    "PairId_NS;" \
    "PairId_EW;" \
    "Names_NS;" \
    "Names_EW;" \
    "Contract;" \
    "Declarer;" \
    "Lead;" \
    "Result;" \
    "Score_NS;" \
    "Score_EW;" \
    "IMP_NS;" \
    "IMP_EW;" \
    "MP_NS;" \
    "MP_EW;" \
    "Percentage_NS;" \
    "Percentage_EW;" \
    "Multiplicity;"
/* "Lead" is OKBridge only */

#define TOTALSCORETABLE_FIELDS \
    "Rank;" \
    "PairId;" \
    "Names;" \
    "TotalScore;" \
    "TotalIMP;" \
    "TotalMP;" \
    "TotalPercentage;" \
    "MeanScore;" \
    "MeanIMP;" \
    "MeanMP;" \
    "MeanPercentage;" \
    "NrBoards;" \
    "Multiplicity;"

struct pbn_note { /* pbnn_ */
    int pbnn_note_id;
    char *pbnn_note_text;
};

struct pbn_note_list { /* pbnl_ */
    int pbnl_nnote;
    struct pbn_note ** pbnl_anote;
};

struct pbn_comment { /* pbnc_ */
    int pbnc_text_len;
    char *pbnc_comment_text;
};

struct pbn_comment_list { /* pbcl_ */
    int pbcl_ncomment;
    struct pbn_comment ** pbcl_acomment;
};

struct pbn_auction_table { /* pbat_ */
    int pbat_nbids;
    int pbat_bid[MAX_BIDS];
    char * pbat_annote[MAX_BIDS];
    struct pbn_note_list * pbat_notelist;
};

struct pbn_play_table { /* pbpt_ */
    int pbpt_ntricks;
    int pbpt_pix;
    int pbpt_trick[13][4];
    char * pbpt_play_note_nums[13][4];
    struct pbn_note_list * pbpt_notelist;
};

struct pbn_table_col_desc { /* pbntcd_ */
    char * pbntcd_col_name;
    int    pbntcd_col_width;
    char   pbntcd_col_justify;
};

struct pbn_table { /* pbnt_ */
    int                             pbnt_ncols;
    struct pbn_table_col_desc **    pbnt_pbntcd;
    int                             pbnt_nrows;
    char ***                        pbnt_arows; /* pbnt_arows[row][col] */
};

/*** BEGIN Generated in Excel 11/28/2018 17:30 ************************/
struct pbnrec { /* pbr_ */
    char *                 pbr_file_name;
    int                    pbr_file_line_number;
    int                    pbr_note_tagid;

    char *                 pbr_tag_actiontable;
    char *                 pbr_tag_annotator;
    char *                 pbr_tag_annotatorna;
    char *                 pbr_tag_application;
    char *                 pbr_tag_auction;
    char *                 pbr_tag_auctiontimetable;
    char *                 pbr_tag_bidsystemew;
    char *                 pbr_tag_bidsystemns;
    char *                 pbr_tag_board;
    char *                 pbr_tag_competition;
    char *                 pbr_tag_contract;
    char *                 pbr_tag_date;
    char *                 pbr_tag_deal;
    char *                 pbr_tag_dealid;
    char *                 pbr_tag_dealer;
    char *                 pbr_tag_declarer;
    char *                 pbr_tag_description;
    char *                 pbr_tag_east;
    char *                 pbr_tag_eastna;
    char *                 pbr_tag_easttype;
    char *                 pbr_tag_event;
    char *                 pbr_tag_eventdate;
    char *                 pbr_tag_eventsponsor;
    char *                 pbr_tag_frenchmp;
    char *                 pbr_tag_generator;
    char *                 pbr_tag_hidden;
    char *                 pbr_tag_hometeam;
    char *                 pbr_tag_instantscoretable;
    char *                 pbr_tag_mode;
    char *                 pbr_tag_north;
    char *                 pbr_tag_northna;
    char *                 pbr_tag_northtype;
    char *                 pbr_tag_note;
    char *                 pbr_tag_optimumplaytable;
    char *                 pbr_tag_optimumresulttable;
    char *                 pbr_tag_optimumscore;
    char *                 pbr_tag_pairew;
    char *                 pbr_tag_pairns;
    char *                 pbr_tag_play;
    char *                 pbr_tag_playtimetable;
    char *                 pbr_tag_result;
    char *                 pbr_tag_room;
    char *                 pbr_tag_round;
    char *                 pbr_tag_score;
    char *                 pbr_tag_scoreimp;
    char *                 pbr_tag_scoremp;
    char *                 pbr_tag_scorepercentage;
    char *                 pbr_tag_scorerubber;
    char *                 pbr_tag_scorerubberhistory;
    char *                 pbr_tag_scoretable;
    char *                 pbr_tag_scoring;
    char *                 pbr_tag_section;
    char *                 pbr_tag_site;
    char *                 pbr_tag_south;
    char *                 pbr_tag_southna;
    char *                 pbr_tag_southtype;
    char *                 pbr_tag_stage;
    char *                 pbr_tag_table;
    char *                 pbr_tag_termination;
    char *                 pbr_tag_time;
    char *                 pbr_tag_timecall;
    char *                 pbr_tag_timecard;
    char *                 pbr_tag_timecontrol;
    char *                 pbr_tag_totalscoretable;
    char *                 pbr_tag_utcdate;
    char *                 pbr_tag_utctime;
    char *                 pbr_tag_visitteam;
    char *                 pbr_tag_vulnerable;
    char *                 pbr_tag_west;
    char *                 pbr_tag_westna;
    char *                 pbr_tag_westtype;

    struct pbn_comment_list * pbr_comment_list;

    /* Custom tables */
    struct pbn_auction_table * pbr_auction_table;
    struct pbn_play_table * pbr_play_table;

    /* Generic tables */
    struct pbn_table *     pbr_action_table;
    struct pbn_table *     pbr_auction_time_table;
    struct pbn_table *     pbr_instant_score_table;
    struct pbn_table *     pbr_optimum_play_table;
    struct pbn_table *     pbr_optimum_result_table;
    struct pbn_table *     pbr_play_time_table;
    struct pbn_table *     pbr_score_table;
    struct pbn_table *     pbr_total_score_table;
};
/*** END Generated in Excel 11/20/2018 12:24 **************************/

struct pbnlist { /* pbl_ */
    int                 pbl_npbr;
    struct pbnrec **    pbl_apbr;
};
#define PBN_MAX_TOKE       256
#define PBN_MAX_LINE       1024

struct pbnglob { /* pbg_ */
    struct frec * pbg_frec;
    int         pbg_buf_ix;
    int         pbg_buf_prev_ix;
    char *      pbg_toke;
    char *      pbg_buf;
    int         pbg_game_done;
    int         pbg_estat;
    struct omarglobals * pbg_og;
    struct pbnrec * pbg_current_pbr;
};
/***************************************************************/
void svare_init_internal_pbn_class(struct svarexec * svx);

/***************************************************************/
#endif /* PBNH_INCLUDED */
