/***************************************************************/
/* pbn.c                                                       */
/***************************************************************/
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <math.h>

#include "omar.h"
#include "xeq.h"
#include "util.h"
#include "bid.h"
#include "pbn.h"
#include "intclas.h"

#define IS_PBN_END(c) ((c) == '\0' || (c) == '[' || (c) == ';')


#define PBN_NAG_ZERO    100
#define PBN_NAG_MAX     27

#define PBN_NAG_BID_BANG                1
#define PBN_NAG_BID_QUESTION            2
#define PBN_NAG_BID_BANG_BANG           3
#define PBN_NAG_BID_QUESTION_QUESTION   4
#define PBN_NAG_BID_BANG_QUESTION       5
#define PBN_NAG_BID_QUESTION_BANG       6

#define PBN_NAG_PLAY_BANG               7
#define PBN_NAG_PLAY_QUESTION           8
#define PBN_NAG_PLAY_BANG_BANG          9
#define PBN_NAG_PLAY_QUESTION_QUESTION  10
#define PBN_NAG_PLAY_BANG_QUESTION      11
#define PBN_NAG_PLAY_QUESTION_BANG      12
/*
pbn_v21.txt
    3.5.3  Auction suffix
         !  good call
         ?  poor call
        !!  very good call
        ??  very poor call
        !?  speculative call
        ?!  questionable call

    pbn_eng_v10.nag
        0 'no annotation'
        1 good call
        2 poor call
        3 very good call
        4 very poor call
        5 speculative call
        6 questionable call
        7 good card
        8 poor card
        9 very good card
        10 very poor card
        11 speculative card
        12 questionable card
*/
/***************************************************************/
/*     PBN Functions                                           */
/***************************************************************/
/*** BEGIN Generated in Excel 11/22/2018 8:22 ************************/
struct pbn_tag_def_def {   /* ptd_ */
    char *          ptd_tag_name;
    int             ptd_tag_id;
} pbntag_list[] = {
{ PBN_TAG_NAME_ACTIONTABLE         ,   PBN_TAG_ID_ACTIONTABLE         } , 
{ PBN_TAG_NAME_ANNOTATOR           ,   PBN_TAG_ID_ANNOTATOR           } , 
{ PBN_TAG_NAME_ANNOTATORNA         ,   PBN_TAG_ID_ANNOTATORNA         } , 
{ PBN_TAG_NAME_APPLICATION         ,   PBN_TAG_ID_APPLICATION         } , 
{ PBN_TAG_NAME_AUCTION             ,   PBN_TAG_ID_AUCTION             } , 
{ PBN_TAG_NAME_AUCTIONTIMETABLE    ,   PBN_TAG_ID_AUCTIONTIMETABLE    } , 
{ PBN_TAG_NAME_BIDSYSTEMEW         ,   PBN_TAG_ID_BIDSYSTEMEW         } , 
{ PBN_TAG_NAME_BIDSYSTEMNS         ,   PBN_TAG_ID_BIDSYSTEMNS         } , 
{ PBN_TAG_NAME_BOARD               ,   PBN_TAG_ID_BOARD               } , 
{ PBN_TAG_NAME_COMPETITION         ,   PBN_TAG_ID_COMPETITION         } , 
{ PBN_TAG_NAME_CONTRACT            ,   PBN_TAG_ID_CONTRACT            } , 
{ PBN_TAG_NAME_DATE                ,   PBN_TAG_ID_DATE                } , 
{ PBN_TAG_NAME_DEAL                ,   PBN_TAG_ID_DEAL                } , 
{ PBN_TAG_NAME_DEALID              ,   PBN_TAG_ID_DEALID              } , 
{ PBN_TAG_NAME_DEALER              ,   PBN_TAG_ID_DEALER              } , 
{ PBN_TAG_NAME_DECLARER            ,   PBN_TAG_ID_DECLARER            } , 
{ PBN_TAG_NAME_DESCRIPTION         ,   PBN_TAG_ID_DESCRIPTION         } , 
{ PBN_TAG_NAME_EAST                ,   PBN_TAG_ID_EAST                } , 
{ PBN_TAG_NAME_EASTNA              ,   PBN_TAG_ID_EASTNA              } , 
{ PBN_TAG_NAME_EASTTYPE            ,   PBN_TAG_ID_EASTTYPE            } , 
{ PBN_TAG_NAME_EVENT               ,   PBN_TAG_ID_EVENT               } , 
{ PBN_TAG_NAME_EVENTDATE           ,   PBN_TAG_ID_EVENTDATE           } , 
{ PBN_TAG_NAME_EVENTSPONSOR        ,   PBN_TAG_ID_EVENTSPONSOR        } , 
{ PBN_TAG_NAME_FRENCHMP            ,   PBN_TAG_ID_FRENCHMP            } , 
{ PBN_TAG_NAME_GENERATOR           ,   PBN_TAG_ID_GENERATOR           } , 
{ PBN_TAG_NAME_HIDDEN              ,   PBN_TAG_ID_HIDDEN              } , 
{ PBN_TAG_NAME_HOMETEAM            ,   PBN_TAG_ID_HOMETEAM            } , 
{ PBN_TAG_NAME_INSTANTSCORETABLE   ,   PBN_TAG_ID_INSTANTSCORETABLE   } , 
{ PBN_TAG_NAME_MODE                ,   PBN_TAG_ID_MODE                } , 
{ PBN_TAG_NAME_NORTH               ,   PBN_TAG_ID_NORTH               } , 
{ PBN_TAG_NAME_NORTHNA             ,   PBN_TAG_ID_NORTHNA             } , 
{ PBN_TAG_NAME_NORTHTYPE           ,   PBN_TAG_ID_NORTHTYPE           } , 
{ PBN_TAG_NAME_NOTE                ,   PBN_TAG_ID_NOTE                } , 
{ PBN_TAG_NAME_OPTIMUMPLAYTABLE    ,   PBN_TAG_ID_OPTIMUMPLAYTABLE    } , 
{ PBN_TAG_NAME_OPTIMUMRESULTTABLE  ,   PBN_TAG_ID_OPTIMUMRESULTTABLE  } , 
{ PBN_TAG_NAME_OPTIMUMSCORE        ,   PBN_TAG_ID_OPTIMUMSCORE        } , 
{ PBN_TAG_NAME_PAIREW              ,   PBN_TAG_ID_PAIREW              } , 
{ PBN_TAG_NAME_PAIRNS              ,   PBN_TAG_ID_PAIRNS              } , 
{ PBN_TAG_NAME_PLAY                ,   PBN_TAG_ID_PLAY                } , 
{ PBN_TAG_NAME_PLAYTIMETABLE       ,   PBN_TAG_ID_PLAYTIMETABLE       } , 
{ PBN_TAG_NAME_RESULT              ,   PBN_TAG_ID_RESULT              } , 
{ PBN_TAG_NAME_ROOM                ,   PBN_TAG_ID_ROOM                } , 
{ PBN_TAG_NAME_ROUND               ,   PBN_TAG_ID_ROUND               } , 
{ PBN_TAG_NAME_SCORE               ,   PBN_TAG_ID_SCORE               } , 
{ PBN_TAG_NAME_SCOREIMP            ,   PBN_TAG_ID_SCOREIMP            } , 
{ PBN_TAG_NAME_SCOREMP             ,   PBN_TAG_ID_SCOREMP             } , 
{ PBN_TAG_NAME_SCOREPERCENTAGE     ,   PBN_TAG_ID_SCOREPERCENTAGE     } , 
{ PBN_TAG_NAME_SCORERUBBER         ,   PBN_TAG_ID_SCORERUBBER         } , 
{ PBN_TAG_NAME_SCORERUBBERHISTORY  ,   PBN_TAG_ID_SCORERUBBERHISTORY  } , 
{ PBN_TAG_NAME_SCORETABLE          ,   PBN_TAG_ID_SCORETABLE          } , 
{ PBN_TAG_NAME_SCORING             ,   PBN_TAG_ID_SCORING             } , 
{ PBN_TAG_NAME_SECTION             ,   PBN_TAG_ID_SECTION             } , 
{ PBN_TAG_NAME_SITE                ,   PBN_TAG_ID_SITE                } , 
{ PBN_TAG_NAME_SOUTH               ,   PBN_TAG_ID_SOUTH               } , 
{ PBN_TAG_NAME_SOUTHNA             ,   PBN_TAG_ID_SOUTHNA             } , 
{ PBN_TAG_NAME_SOUTHTYPE           ,   PBN_TAG_ID_SOUTHTYPE           } , 
{ PBN_TAG_NAME_STAGE               ,   PBN_TAG_ID_STAGE               } , 
{ PBN_TAG_NAME_TABLE               ,   PBN_TAG_ID_TABLE               } , 
{ PBN_TAG_NAME_TERMINATION         ,   PBN_TAG_ID_TERMINATION         } , 
{ PBN_TAG_NAME_TIME                ,   PBN_TAG_ID_TIME                } , 
{ PBN_TAG_NAME_TIMECALL            ,   PBN_TAG_ID_TIMECALL            } , 
{ PBN_TAG_NAME_TIMECARD            ,   PBN_TAG_ID_TIMECARD            } , 
{ PBN_TAG_NAME_TIMECONTROL         ,   PBN_TAG_ID_TIMECONTROL         } , 
{ PBN_TAG_NAME_TOTALSCORETABLE     ,   PBN_TAG_ID_TOTALSCORETABLE     } , 
{ PBN_TAG_NAME_UTCDATE             ,   PBN_TAG_ID_UTCDATE             } , 
{ PBN_TAG_NAME_UTCTIME             ,   PBN_TAG_ID_UTCTIME             } , 
{ PBN_TAG_NAME_VISITTEAM           ,   PBN_TAG_ID_VISITTEAM           } , 
{ PBN_TAG_NAME_VULNERABLE          ,   PBN_TAG_ID_VULNERABLE          } , 
{ PBN_TAG_NAME_WEST                ,   PBN_TAG_ID_WEST                } , 
{ PBN_TAG_NAME_WESTNA              ,   PBN_TAG_ID_WESTNA              } , 
{ PBN_TAG_NAME_WESTTYPE            ,   PBN_TAG_ID_WESTTYPE            } , 
{ NULL      , 0     } };
/*** END Generated in Excel 11/22/2018 8:22 **************************/
/***************************************************************/
static int pbn_count_tags(void)
{
/*
struct pbn_tag_def_def {
    char *          ptd_tag_name;
    int             ptd_tag_id;
} pbntag_list[] = {
*/
    int tcount = 0;

    while (pbntag_list[tcount].ptd_tag_name) {
#if IS_DEBUG
        if (tcount) {
            if (strcmp(pbntag_list[tcount - 1].ptd_tag_name,
                       pbntag_list[tcount].ptd_tag_name) > 0) {
                printf(
        "**** count_html_entities(): entities not in order at %d: %s\n",
                    tcount, pbntag_list[tcount].ptd_tag_name);
            }
        }
#endif
        tcount++;
    }

    return (tcount);
}
/***************************************************************/
static int pbn_find_tag_id_index(const char * tagnam)
{
    static int num_pbntags = 0;
    int lo;
    int hi;
    int mid;

    if (!num_pbntags) {
        num_pbntags = pbn_count_tags();
    }

    lo = 0;
    hi = num_pbntags - 1;
    while (lo <= hi) {
        mid = (lo + hi) / 2;
        if (strcmp(tagnam, pbntag_list[mid].ptd_tag_name) <= 0)
                                             hi = mid - 1;
        else                                 lo = mid + 1;
    }

    if (lo == num_pbntags ||
        strcmp(tagnam, pbntag_list[lo].ptd_tag_name)) lo = -1;

    return (lo);
}
/***************************************************************/
static int pbn_find_tag_by_name(const char * tagnam)
{
    int iix;
    int cid;

    iix = pbn_find_tag_id_index(tagnam);

    cid = 0;
    if (iix >= 0) cid = pbntag_list[iix].ptd_tag_id;

    return (cid);
}
/***************************************************************/
static void free_pbnglob(struct pbnglob * pbg)
{
    if (!pbg) return;

    if (pbg->pbg_frec) frec_close(pbg->pbg_frec);
    Free(pbg->pbg_toke);
    Free(pbg->pbg_buf);
    Free(pbg);
}
/***************************************************************/
static struct pbnglob * new_pbnglob(struct omarglobals * og)
{
    struct pbnglob * pbg;

    pbg = New(struct pbnglob, 1);
    pbg->pbg_estat              = 0;
    pbg->pbg_frec               = NULL;
    pbg->pbg_buf_ix             = 0;
    pbg->pbg_buf_prev_ix        = 0;
    pbg->pbg_buf                = New(char, PBN_MAX_LINE);
    pbg->pbg_toke               = New(char, PBN_MAX_TOKE);
    pbg->pbg_buf[0]             = '\0';
    pbg->pbg_toke[0]            = '\0';
    pbg->pbg_game_done          = 0;
    pbg->pbg_og                 = og;
    pbg->pbg_current_pbr        = NULL;

    return (pbg);
}
/***************************************************************/
static struct pbn_note * new_pbn_note(void)
{
/*
struct pbn_note {
    int pbnn_note_id;
    char *pbnn_note_text;
};
*/
    struct pbn_note * pbnn;

    pbnn = New(struct pbn_note, 1);
    pbnn->pbnn_note_id = 0;
    pbnn->pbnn_note_text = NULL;

    return (pbnn);
}
/***************************************************************/
static void free_pbn_note(struct pbn_note * pbnn)
{
    Free(pbnn->pbnn_note_text);
    Free(pbnn);
}
/***************************************************************/
static struct pbn_note_list * new_pbn_note_list(void)
{
/*
struct pbn_note_list {
    int pbnl_nnote;
    struct pbn_note ** pbnl_anote;
};
*/
    struct pbn_note_list * pbnl;

    pbnl = New(struct pbn_note_list, 1);
    pbnl->pbnl_nnote = 0;
    pbnl->pbnl_anote = NULL;

    return (pbnl);
}
/***************************************************************/
static void free_pbn_note_list(struct pbn_note_list * pbnl)
{
    int ii;

    if (!pbnl) return;

    for (ii = 0; ii < pbnl->pbnl_nnote; ii++) {
        free_pbn_note(pbnl->pbnl_anote[ii]);
    }
    Free(pbnl->pbnl_anote);

    Free(pbnl);
}
/***************************************************************/
static struct pbn_comment * new_pbn_comment(void)
{
/*
struct pbn_comment {
    int pbnc_text_len;
    char *pbnc_comment_text;
};
*/
    struct pbn_comment * pbnc;

    pbnc = New(struct pbn_comment, 1);
    pbnc->pbnc_text_len = 0;
    pbnc->pbnc_comment_text = NULL;

    return (pbnc);
}
/***************************************************************/
static void free_pbn_comment(struct pbn_comment * pbnc)
{
    Free(pbnc->pbnc_comment_text);
    Free(pbnc);
}
/***************************************************************/
static struct pbn_comment_list * new_pbn_comment_list(void)
{
/*
struct pbn_comment_list {
    int pbcl_ncomment;
    struct pbn_comment ** pbcl_acomment;
};
*/
    struct pbn_comment_list * pbcl;

    pbcl = New(struct pbn_comment_list, 1);
    pbcl->pbcl_ncomment = 0;
    pbcl->pbcl_acomment = NULL;

    return (pbcl);
}
/***************************************************************/
static void free_pbn_comment_list(struct pbn_comment_list * pbcl)
{
    int ii;

    if (!pbcl) return;

    for (ii = 0; ii < pbcl->pbcl_ncomment; ii++) {
        free_pbn_comment(pbcl->pbcl_acomment[ii]);
    }
    Free(pbcl->pbcl_acomment);

    Free(pbcl);
}
/***************************************************************/
static void add_comment_to_comment_list(struct pbn_comment_list * pbcl, struct pbn_comment * pbnc)
{
/*
struct pbn_comment_list {
    int pbcl_ncomment;
    struct pbn_comment ** pbcl_acomment;
};
*/
    pbcl->pbcl_acomment = Realloc(pbcl->pbcl_acomment, struct pbr *, pbcl->pbcl_ncomment + 1);
    pbcl->pbcl_acomment[pbcl->pbcl_ncomment] = pbnc;
    pbcl->pbcl_ncomment += 1;
}
/***************************************************************/
static void free_pbn_auction_table(struct pbn_auction_table * pbat)
{
    int ii;

    if (!pbat) return;

    for (ii = 0; ii < pbat->pbat_nbids; ii++) {
        if (pbat->pbat_annote[ii]) Free(pbat->pbat_annote[ii]);
    }
    free_pbn_note_list(pbat->pbat_notelist);

    Free(pbat);
}
/***************************************************************/
static struct pbn_auction_table * new_pbn_auction_table(void)
{
    struct pbn_auction_table * pbat;

    pbat = New(struct pbn_auction_table, 1);
    pbat->pbat_nbids = 0;
    pbat->pbat_notelist = NULL;

    return (pbat);
}
/***************************************************************/
static void free_pbn_play_table(struct pbn_play_table * pbpt)
{
    int ii;
    int jj;

    if (!pbpt) return;

    for (ii = 0; ii < pbpt->pbpt_ntricks; ii++) {
        for (jj = 0; jj < 4; jj++) {
            if (pbpt->pbpt_play_note_nums[ii][jj]) {
                Free(pbpt->pbpt_play_note_nums[ii][jj]);
            }
        }
    }
    free_pbn_note_list(pbpt->pbpt_notelist);

    Free(pbpt);
}
/***************************************************************/
static struct pbn_play_table * new_pbn_play_table(void)
{
    struct pbn_play_table * pbpt;

    pbpt = New(struct pbn_play_table, 1);
    pbpt->pbpt_ntricks = 0;
    pbpt->pbpt_pix     = 0;
    pbpt->pbpt_notelist = NULL;

    return (pbpt);
}
/***************************************************************/
static struct pbn_table * new_pbn_table(void)
{
/*
struct pbn_play_table {
    int pbpt_ntricks;
    int pbpt_pix;
    int pbpt_trick[13][4];
    char * pbpt_play_note_nums[13][4];
    struct pbn_note_list * pbpt_notelist;
};
*/
    struct pbn_table * pbnt;

    pbnt = New(struct pbn_table, 1);
    pbnt->pbnt_ncols = 0;
    pbnt->pbnt_pbntcd = NULL;
    pbnt->pbnt_nrows = 0;
    pbnt->pbnt_arows = NULL;

    return (pbnt);
}
/***************************************************************/
static struct pbn_table_col_desc * new_pbn_table_col_desc(
    const char * col_name,
    int    col_width,
    char   col_justify)
{
/*
--
struct pbn_table_col_desc {
    char * pbntcd_col_name;
    int    pbntcd_col_index;
    int    pbntcd_col_width;
};
--
*/
    struct pbn_table_col_desc * pbntcd;

    pbntcd = New(struct pbn_table_col_desc, 1);
    pbntcd->pbntcd_col_name  = NULL;
    pbntcd->pbntcd_col_width = col_width;
    pbntcd->pbntcd_col_justify = col_justify;
    if (col_name) pbntcd->pbntcd_col_name = Strdup(col_name);

    return (pbntcd);
}
/***************************************************************/
static void free_pbn_table_col_desc(struct pbn_table_col_desc * pbntcd)
{
    Free(pbntcd->pbntcd_col_name);
    Free(pbntcd);
}
/***************************************************************/
static void add_table_col_desc(struct pbn_table * pbnt, struct pbn_table_col_desc * pbntcd)
{
/*
--
struct pbn_table {
    int                             pbnt_ncols;
    struct pbn_table_col_desc **    pbnt_pbntcd;
    int                             pbnt_nrows;
    char ***                        pbnt_arows;
};
--
*/
    pbnt->pbnt_pbntcd = Realloc(pbnt->pbnt_pbntcd, struct pbr *, pbnt->pbnt_ncols + 1);
    pbnt->pbnt_pbntcd[pbnt->pbnt_ncols] = pbntcd;
    pbnt->pbnt_ncols += 1;
}
/***************************************************************/
static void free_pbn_table(struct pbn_table * pbnt)
{
    int ii;
    int jj;

    if (!pbnt) return;

    for (ii = 0; ii < pbnt->pbnt_ncols; ii++) {
        free_pbn_table_col_desc(pbnt->pbnt_pbntcd[ii]);
    }
    Free(pbnt->pbnt_pbntcd);

    for (ii = 0; ii < pbnt->pbnt_nrows; ii++) {
        for (jj = 0; jj < pbnt->pbnt_ncols; jj++) {
            Free(pbnt->pbnt_arows[ii][jj]);
        }
        Free(pbnt->pbnt_arows[ii]);
    }
    Free(pbnt->pbnt_arows);

    Free(pbnt);
}
/***************************************************************/
/*** BEGIN Generated in Excel 11/20/2018 12:37 ************************/
static void free_pbnrec(struct pbnrec * pbr)
{
    Free(pbr->pbr_file_name);
    Free(pbr->pbr_tag_actiontable);
    Free(pbr->pbr_tag_annotator);
    Free(pbr->pbr_tag_annotatorna);
    Free(pbr->pbr_tag_application);
    Free(pbr->pbr_tag_auction);
    Free(pbr->pbr_tag_bidsystemew);
    Free(pbr->pbr_tag_bidsystemns);
    Free(pbr->pbr_tag_board);
    Free(pbr->pbr_tag_competition);
    Free(pbr->pbr_tag_contract);
    Free(pbr->pbr_tag_date);
    Free(pbr->pbr_tag_deal);
    Free(pbr->pbr_tag_dealid);
    Free(pbr->pbr_tag_dealer);
    Free(pbr->pbr_tag_declarer);
    Free(pbr->pbr_tag_description);
    Free(pbr->pbr_tag_east);
    Free(pbr->pbr_tag_eastna);
    Free(pbr->pbr_tag_easttype);
    Free(pbr->pbr_tag_event);
    Free(pbr->pbr_tag_eventdate);
    Free(pbr->pbr_tag_eventsponsor);
    Free(pbr->pbr_tag_frenchmp);
    Free(pbr->pbr_tag_generator);
    Free(pbr->pbr_tag_hidden);
    Free(pbr->pbr_tag_hometeam);
    Free(pbr->pbr_tag_mode);
    Free(pbr->pbr_tag_north);
    Free(pbr->pbr_tag_northna);
    Free(pbr->pbr_tag_northtype);
    Free(pbr->pbr_tag_note);
    Free(pbr->pbr_tag_optimumscore);
    Free(pbr->pbr_tag_pairew);
    Free(pbr->pbr_tag_pairns);
    Free(pbr->pbr_tag_play);
    Free(pbr->pbr_tag_result);
    Free(pbr->pbr_tag_room);
    Free(pbr->pbr_tag_round);
    Free(pbr->pbr_tag_score);
    Free(pbr->pbr_tag_scoreimp);
    Free(pbr->pbr_tag_scoremp);
    Free(pbr->pbr_tag_scorepercentage);
    Free(pbr->pbr_tag_scorerubber);
    Free(pbr->pbr_tag_scorerubberhistory);
    Free(pbr->pbr_tag_scoretable);
    Free(pbr->pbr_tag_scoring);
    Free(pbr->pbr_tag_section);
    Free(pbr->pbr_tag_site);
    Free(pbr->pbr_tag_south);
    Free(pbr->pbr_tag_southna);
    Free(pbr->pbr_tag_southtype);
    Free(pbr->pbr_tag_stage);
    Free(pbr->pbr_tag_table);
    Free(pbr->pbr_tag_termination);
    Free(pbr->pbr_tag_time);
    Free(pbr->pbr_tag_timecall);
    Free(pbr->pbr_tag_timecard);
    Free(pbr->pbr_tag_timecontrol);
    Free(pbr->pbr_tag_totalscoretable);
    Free(pbr->pbr_tag_utcdate);
    Free(pbr->pbr_tag_utctime);
    Free(pbr->pbr_tag_visitteam);
    Free(pbr->pbr_tag_vulnerable);
    Free(pbr->pbr_tag_west);
    Free(pbr->pbr_tag_westna);
    Free(pbr->pbr_tag_westtype);

    free_pbn_comment_list(pbr->pbr_comment_list);

    free_pbn_auction_table(pbr->pbr_auction_table);
    free_pbn_play_table(pbr->pbr_play_table);

    free_pbn_table(pbr->pbr_action_table);
    free_pbn_table(pbr->pbr_auction_time_table);
    free_pbn_table(pbr->pbr_instant_score_table);
    free_pbn_table(pbr->pbr_optimum_play_table);
    free_pbn_table(pbr->pbr_optimum_result_table);
    free_pbn_table(pbr->pbr_play_time_table);
    free_pbn_table(pbr->pbr_score_table);
    free_pbn_table(pbr->pbr_total_score_table);

    Free(pbr);
}
/*** END Generated in Excel 11/20/2018 12:37 **************************/
/***************************************************************/
/*** BEGIN Generated in Excel 11/20/2018 12:34 ************************/
static struct pbnrec * new_pbnrec(struct pbnglob * pbg, const char * pbn_fname, int pbn_line_num)
{
    struct pbnrec * pbr;

    pbr = New(struct pbnrec, 1);
    pbr->pbr_file_name               = NULL;
    if (pbn_fname) pbr->pbr_file_name = Strdup(pbn_fname);
    pbr->pbr_file_line_number        = pbn_line_num;
    pbr->pbr_note_tagid              = 0;
    pbr->pbr_comment_list            = NULL;

    pbg->pbg_current_pbr = pbr;


    pbr->pbr_tag_annotator           = NULL;
    pbr->pbr_tag_annotatorna         = NULL;
    pbr->pbr_tag_application         = NULL;
    pbr->pbr_tag_auction             = NULL;
    pbr->pbr_tag_bidsystemew         = NULL;
    pbr->pbr_tag_bidsystemns         = NULL;
    pbr->pbr_tag_board               = NULL;
    pbr->pbr_tag_competition         = NULL;
    pbr->pbr_tag_contract            = NULL;
    pbr->pbr_tag_date                = NULL;
    pbr->pbr_tag_deal                = NULL;
    pbr->pbr_tag_dealid              = NULL;
    pbr->pbr_tag_dealer              = NULL;
    pbr->pbr_tag_declarer            = NULL;
    pbr->pbr_tag_description         = NULL;
    pbr->pbr_tag_east                = NULL;
    pbr->pbr_tag_eastna              = NULL;
    pbr->pbr_tag_easttype            = NULL;
    pbr->pbr_tag_event               = NULL;
    pbr->pbr_tag_eventdate           = NULL;
    pbr->pbr_tag_eventsponsor        = NULL;
    pbr->pbr_tag_frenchmp            = NULL;
    pbr->pbr_tag_generator           = NULL;
    pbr->pbr_tag_hidden              = NULL;
    pbr->pbr_tag_hometeam            = NULL;
    pbr->pbr_tag_mode                = NULL;
    pbr->pbr_tag_north               = NULL;
    pbr->pbr_tag_northna             = NULL;
    pbr->pbr_tag_northtype           = NULL;
    pbr->pbr_tag_note                = NULL;
    pbr->pbr_tag_optimumscore        = NULL;
    pbr->pbr_tag_pairew              = NULL;
    pbr->pbr_tag_pairns              = NULL;
    pbr->pbr_tag_play                = NULL;
    pbr->pbr_tag_result              = NULL;
    pbr->pbr_tag_room                = NULL;
    pbr->pbr_tag_round               = NULL;
    pbr->pbr_tag_score               = NULL;
    pbr->pbr_tag_scoreimp            = NULL;
    pbr->pbr_tag_scoremp             = NULL;
    pbr->pbr_tag_scorepercentage     = NULL;
    pbr->pbr_tag_scorerubber         = NULL;
    pbr->pbr_tag_scorerubberhistory  = NULL;
    pbr->pbr_tag_scoretable          = NULL;
    pbr->pbr_tag_scoring             = NULL;
    pbr->pbr_tag_section             = NULL;
    pbr->pbr_tag_site                = NULL;
    pbr->pbr_tag_south               = NULL;
    pbr->pbr_tag_southna             = NULL;
    pbr->pbr_tag_southtype           = NULL;
    pbr->pbr_tag_stage               = NULL;
    pbr->pbr_tag_table               = NULL;
    pbr->pbr_tag_termination         = NULL;
    pbr->pbr_tag_time                = NULL;
    pbr->pbr_tag_timecall            = NULL;
    pbr->pbr_tag_timecard            = NULL;
    pbr->pbr_tag_timecontrol         = NULL;
    pbr->pbr_tag_totalscoretable     = NULL;
    pbr->pbr_tag_utcdate             = NULL;
    pbr->pbr_tag_utctime             = NULL;
    pbr->pbr_tag_visitteam           = NULL;
    pbr->pbr_tag_vulnerable          = NULL;
    pbr->pbr_tag_west                = NULL;
    pbr->pbr_tag_westna              = NULL;

    /* Custom tables */
    pbr->pbr_auction_table           = NULL;
    pbr->pbr_play_table              = NULL;

    /* Generic tables */
    pbr->pbr_action_table            = NULL;
    pbr->pbr_auction_time_table      = NULL;
    pbr->pbr_instant_score_table     = NULL;
    pbr->pbr_optimum_play_table      = NULL;
    pbr->pbr_optimum_result_table    = NULL;
    pbr->pbr_play_time_table         = NULL;
    pbr->pbr_score_table             = NULL;
    pbr->pbr_total_score_table       = NULL;

    return (pbr);
}
/*** END Generated in Excel 11/20/2018 12:34 **************************/
/***************************************************************/
static struct pbnlist * new_pbnlist(void)
{
    struct pbnlist * pbl;

    pbl = New(struct pbnlist, 1);
    pbl->pbl_npbr = 0;
    pbl->pbl_apbr = NULL;

    return (pbl);
}
/***************************************************************/
static void free_pbnlist(struct pbnlist * pbl)
{
    int ii;

    for (ii = 0; ii < pbl->pbl_npbr; ii++) {
        free_pbnrec(pbl->pbl_apbr[ii]);
    }
    Free(pbl->pbl_apbr);
    Free(pbl);
}
/***************************************************************/
static int svare_method_PBNNew(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)
{
    int svstat = 0;
    struct pbnlist * pbl;

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "<" SVAR_CLASS_NAME_PBN ">");

    if (!svstat) {
        pbl = new_pbnlist();
        svare_store_int_object(svx, asvv[0]->svv_val.svv_svnc, rsvv, pbl);
    }

    return (svstat);
}
/***************************************************************/
static int svare_method_PBNDelete(struct svarexec * svx,   /* NULL for destructor */
    const char * func_name,     /* NULL for destructor */
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)    /* NULL for destructor */
{
    int svstat = 0;
    struct pbnlist * pbl;

    pbl = (struct pbnlist *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
    free_pbnlist(pbl);

    return (svstat);
}
/***************************************************************/
static int pbn_set_estat(struct pbnglob * pbg, int estat)
{
    pbg->pbg_estat = estat;

    return (pbg->pbg_estat);
}
/***************************************************************/
static int pbn_get_estat(struct pbnglob * pbg, int ignwarn)
{
    int estat;

    estat = pbg->pbg_estat;

    return (estat);
}
/***************************************************************/
int pbn_set_error (struct pbnglob * pbg, const char * fmt, ...) {

    va_list args;
    int estat;

    va_start (args, fmt);
    estat = omarerr_v(pbg->pbg_og, fmt, args);
    va_end (args);
    pbn_set_estat(pbg, estat);

    return (pbg->pbg_estat);
}
/***************************************************************/
int pbn_set_warning (struct pbnglob * pbg, const char * fmt, ...) {

    va_list args;
    int estat;

    va_start (args, fmt);
    estat = omarwarn_v(pbg->pbg_og, fmt, args);
    va_end (args);
    /* pbn_set_estat(pbg, estat); */

    return (pbg->pbg_estat);
}
/***************************************************************/
#if 0
static void get_pbn_toke_quote(struct pbnglob * pbg) {
/*
** Parse quotation
*/
    char quote;
    int tokelen;
    int done;
    char ch;

    tokelen = 0;
    pbg->pbg_toke[0] = '\0';

    ch = pbg->pbg_buf[pbg->pbg_buf_ix];
    (pbg->pbg_buf_ix)++;
    quote = ch;
    pbg->pbg_toke[tokelen++] = ch;
    done = 0;

    while (!done) {
        ch = pbg->pbg_buf[pbg->pbg_buf_ix];

        if (ch == '\\') {
            (pbg->pbg_buf_ix)++;
            ch = pbg->pbg_buf[pbg->pbg_buf_ix];
            if (!ch) {
                done = 1;
            } else {
                if (tokelen + 1 < PBN_MAX_TOKE) pbg->pbg_toke[tokelen++] = ch;
                (pbg->pbg_buf_ix)++;
            }
        } else if (ch == quote) {
            if (tokelen + 1 < PBN_MAX_TOKE) pbg->pbg_toke[tokelen++] = ch;
            (pbg->pbg_buf_ix)++;
            done = 1;
        } else if (ch) {
            if (tokelen + 1 < PBN_MAX_TOKE) pbg->pbg_toke[tokelen++] = ch;
            (pbg->pbg_buf_ix)++;
        } else {
            done = 1;
        }
    }

    pbg->pbg_toke[tokelen] = '\0';
}
/***************************************************************/
/*@*/void get_pbn_toke_str(struct pbnglob * pbg) {

    int tokelen;
    int done;
    char ch;

    tokelen = 0;
    pbg->pbg_toke[tokelen] = '\0';
    while (isspace(pbg->pbg_buf[pbg->pbg_buf_ix])) {
        pbg->pbg_buf_ix += 1;
    }
    pbg->pbg_buf_prev_ix        = pbg->pbg_buf_ix;
    if (!pbg->pbg_buf[pbg->pbg_buf_ix]) return;

    done = 0;
    ch = pbg->pbg_buf[pbg->pbg_buf_ix];
    if (ch == '\"') {
        get_pbn_toke_quote(pbg);
        return;
    } else {
        while (!done) {
            ch = pbg->pbg_buf[pbg->pbg_buf_ix];

            if (isalpha(ch) || isdigit(ch)) {
                if (tokelen + 1 < PBN_MAX_TOKE) pbg->pbg_toke[tokelen++] = ch;
                (pbg->pbg_buf_ix)++;
            } else if (ch == '_' || ch == '/') {
                if (tokelen + 1 < PBN_MAX_TOKE) pbg->pbg_toke[tokelen++] = ch;
                (pbg->pbg_buf_ix)++;
            } else {
                if (!tokelen) {
                if (tokelen + 1 < PBN_MAX_TOKE) pbg->pbg_toke[tokelen++] = ch;
                    (pbg->pbg_buf_ix)++;
                }
                done = 1;
            }
        }
        pbg->pbg_toke[tokelen] = '\0';
    }
}
#else
/***************************************************************/
static void get_pbn_toke_quote(const char * buf, int * bufix, char * toke, int tokesz) {
/*
** Parse quotation
*/
    char quote;
    int tokelen;
    int done;
    char ch;

    tokelen = 0;
    toke[0] = '\0';

    ch = buf[*bufix];
    (*bufix)++;
    quote = ch;
    toke[tokelen++] = ch;
    done = 0;

    while (!done) {
        ch = buf[*bufix];

        if (ch == '\\') {
            (*bufix)++;
            ch = buf[*bufix];
            if (!ch) {
                done = 1;
            } else if (ch == '\\' || ch == '\"') {
                if (tokelen + 1 < tokesz) toke[tokelen++] = ch;
                (*bufix)++;
            } else {
                if (tokelen + 1 < tokesz) toke[tokelen++] = '\\';
            }
        } else if (ch == quote) {
            if (tokelen + 1 < tokesz) toke[tokelen++] = ch;
            (*bufix)++;
            done = 1;
        } else if (ch) {
            if (tokelen + 1 < tokesz) toke[tokelen++] = ch;
            (*bufix)++;
        } else {
            done = 1;
        }
    }

    toke[tokelen] = '\0';
}
/***************************************************************/
/*@*/int get_pbn_toke_str(const char * buf, int * bufix, char * toke, int tokesz) {

    int tokelen;
    int done;
    int buf_prev_ix;
    char ch;

    tokelen = 0;
    toke[tokelen] = '\0';
    while (isspace(buf[*bufix])) {
        *bufix += 1;
    }
    buf_prev_ix        = *bufix;

    done = 0;
    ch = buf[*bufix];
    if (ch == '\"') {
        get_pbn_toke_quote(buf, bufix, toke, tokesz);
    } else if (ch) {
        while (!done) {
            ch = buf[*bufix];

            if (isalpha(ch) || isdigit(ch)) {
                if (tokelen + 1 < tokesz) toke[tokelen++] = ch;
                (*bufix)++;
            /* nondelimiting specials */
            } else if (ch == '_' || ch == '/') {
                if (tokelen + 1 < tokesz) toke[tokelen++] = ch;
                (*bufix)++;
            /* special cases */
            } else if (ch == '=' || ch == '!' || ch == '?') {
                if (tokelen) {
                    if (toke[0] == '=' || toke[0] == '!' || toke[0] == '?') {
                        if (tokelen + 1 < tokesz) toke[tokelen++] = ch;
                        (*bufix)++;
                    }
                    done = 1;
                } else {
                    if (tokelen + 1 < tokesz) toke[tokelen++] = ch;
                    (*bufix)++;
                }
            } else if (ch == '$') {
                if (tokelen) {
                    done = 1;
                } else {
                    if (tokelen + 1 < tokesz) toke[tokelen++] = ch;
                    (*bufix)++;
                }
            } else {
                if (!tokelen) {
                    if (tokelen + 1 < tokesz) toke[tokelen++] = ch;
                    (*bufix)++;
                }
                done = 1;
            }
        }
        toke[tokelen] = '\0';
    }

    return (buf_prev_ix);
}
#endif
/***************************************************************/
/*@*/int get_pbn_toke_str_white(
    const char * buf,
    int * bufix,
    char * toke,
    int tokesz,
    char delim) {

    int tokelen;
    int done;
    int buf_prev_ix;
    char ch;

    tokelen = 0;
    toke[tokelen] = '\0';
    while (isspace(buf[*bufix])) {
        *bufix += 1;
    }
    buf_prev_ix        = *bufix;

    done = 0;
    while (!done) {
        ch = buf[*bufix];

        if (!ch || isspace(ch) || ch == delim) {
            if (!tokelen && ch == delim) {
                if (tokelen + 1 < tokesz) toke[tokelen++] = ch;
                (*bufix)++;
            }
            done = 1;
        } else {
            if (tokelen + 1 < tokesz) toke[tokelen++] = ch;
            (*bufix)++;
        }
    }
    toke[tokelen] = '\0';

    return (buf_prev_ix);
}
/***************************************************************/
/*@*/int get_pbn_read_line(struct pbnglob * pbg)
{
    int estat = 0;
    int ok;

    ok = frec_gets(pbg->pbg_buf, PBN_MAX_LINE, pbg->pbg_frec);
    if (ok < 0) {
        if (pbg->pbg_frec->feof) {
            estat = pbn_set_estat(pbg, ESTAT_EOF);
        } else {
            estat = pbn_set_error(pbg, "Error reading file.");
        }
    } else {
        pbg->pbg_buf_ix = 0;
    }

    return (estat);
}
/***************************************************************/
/*@*/void get_pbn_toke_line(struct pbnglob * pbg)
{
    int estat = 0;
    int done;

    done = 0;
    while (!done) {
        estat = get_pbn_read_line(pbg);
        if (estat) {
            done = 1;
        } else if (pbg->pbg_buf[0] == '%') {
            /* comment */
        } else {
            while (isspace(pbg->pbg_buf[pbg->pbg_buf_ix])) pbg->pbg_buf_ix += 1;
            if (!pbg->pbg_buf[pbg->pbg_buf_ix]) {
                /* end of game */
                done = 1;
                pbg->pbg_game_done = 1;
            } else {
                done = 1;
            }
        }
    }
}
/***************************************************************/
/*@*/void get_pbn_comment(struct pbnglob * pbg)
{
    int estat;
    char cend;
    char clen;
    int stix;
    int done;
    char *comment_text;
    struct pbn_comment * pbnc;
    struct pbn_comment_list * pbcl;

    pbcl = NULL;
    if (pbg->pbg_current_pbr) {
        if (!pbg->pbg_current_pbr->pbr_comment_list) {
            pbg->pbg_current_pbr->pbr_comment_list = new_pbn_comment_list();
        }
        pbcl = pbg->pbg_current_pbr->pbr_comment_list;
    }

    cend = '\0';
    if (pbg->pbg_buf[pbg->pbg_buf_ix] == '{') cend = '}';
    stix = pbg->pbg_buf_ix;
    pbg->pbg_buf_ix += 1;

    done = 0;
    while (!done) {
        while (pbg->pbg_buf[pbg->pbg_buf_ix] &&
               pbg->pbg_buf[pbg->pbg_buf_ix] != cend) {
            pbg->pbg_buf_ix += 1;
        }

        if (pbcl) {
            clen = pbg->pbg_buf_ix - stix;
            if (pbg->pbg_buf[pbg->pbg_buf_ix]) clen++;
            comment_text = New(char, clen + 1);
            memcpy(comment_text, pbg->pbg_buf + stix, clen);
            comment_text[clen] = '\0';

            pbnc = new_pbn_comment();
            pbnc->pbnc_comment_text = comment_text;
            pbnc->pbnc_text_len     = clen;
            add_comment_to_comment_list(pbcl, pbnc);
        }

        if (pbg->pbg_buf[pbg->pbg_buf_ix]) {
            pbg->pbg_buf_ix += 1;
            done = 1;
        } else {
            estat = get_pbn_read_line(pbg);
            if (!cend || estat) {
                done = 1;
            } else {
                stix = 0;
            }
        }
    }


}
/***************************************************************/
/*@*/void get_pbn_toke(struct pbnglob * pbg)
{
    int done;
    int end_at_blank_line;

    end_at_blank_line = 0;
    done = 0;
    pbg->pbg_toke[0] = '\0';
    while (!done) {
        if (pbg->pbg_game_done) {
            done = 1;
        } else {
            while (isspace(pbg->pbg_buf[pbg->pbg_buf_ix])) pbg->pbg_buf_ix += 1;
            if (pbg->pbg_buf[pbg->pbg_buf_ix] == '{' ||
                pbg->pbg_buf[pbg->pbg_buf_ix] == ';') {
                get_pbn_comment(pbg);
                end_at_blank_line = 0;
            } else if (!pbg->pbg_buf[pbg->pbg_buf_ix]) {
                if (end_at_blank_line) {
                    done = 1;
                } else {
                    get_pbn_toke_line(pbg);
                    end_at_blank_line = 1;
                }
            } else {
                pbg->pbg_buf_prev_ix = get_pbn_toke_str(pbg->pbg_buf, &(pbg->pbg_buf_ix), pbg->pbg_toke, PBN_MAX_TOKE);
                done = 1;
            }
        }
    }
}
/***************************************************************/
//  /*@*/void get_pbn_toke_white(struct pbnglob * pbg, char delim)
//  {
//      int estat = 0;
//  
//      pbg->pbg_toke[0] = '\0';
//      if (!pbg->pbg_game_done) {
//          pbg->pbg_buf_prev_ix = get_pbn_toke_str_white(pbg->pbg_buf, &(pbg->pbg_buf_ix), pbg->pbg_toke, PBN_MAX_TOKE, delim);
//          if (!pbg->pbg_toke[0]) {
//              get_pbn_toke_line(pbg);
//              pbg->pbg_buf_prev_ix = get_pbn_toke_str_white(pbg->pbg_buf, &(pbg->pbg_buf_ix), pbg->pbg_toke, PBN_MAX_TOKE, delim);
//          }
//      }
//  }
/***************************************************************/
/*@*/void unget_pbn_toke(struct pbnglob * pbg) {
    pbg->pbg_buf_ix = pbg->pbg_buf_prev_ix;
    pbg->pbg_toke[0] = '\0';
}
#if 0
/***************************************************************/
/*@*/void handle_auction(struct omarglobals * og,
        SEQINFO * si,
        char * bidbuf) {

    AUCTION * auction;
    ANALYSIS * analysis;
    int ptr;
    int bt;
    int bl;
    int bs;
    int sdone;
    int anat;

    auction = new_auction(0, VUL_NONE);
    ptr = 1;
    anat = 0;
    while (isspace(bidbuf[ptr])) ptr++;
    if (bidbuf[ptr] == '*') {
        anat = 1;
        ptr++;
    }
    sdone = 0;

    while (!sdone) {
        next_bid(og, bidbuf, &ptr, &bt, &bl, &bs);
        if (bt == NONE) sdone = 1;
        else make_bid(auction, bt, bl, bs);
    }

    if (auction->au_nbids) {
        analysis = new_analysis();
        init_cc(og, si);
        analyze_auction(og, si, auction, analysis, anat);
        print_analysis(og, auction, analysis);
        free_analysis(analysis);
        free_cc(si);
    }
    Free(auction);
}
#endif
/***************************************************************/
/*@*/int parse_pbn_annotes(
        struct pbnglob * pbg,
        char ** annotes,
        int is_play)
{
    char c;
    int ix;
    int num;
    int note_ok;
    int done;
    int note_num;
    int nannotes;

    note_ok = 1;
    done = 0;
    nannotes = 0;
    (*annotes) = NULL;

    while (!done && note_ok) {
        ix = 0;
        note_num = 0;
        get_pbn_toke(pbg);
        c = pbg->pbg_toke[ix];
        if (c == '=') {
            num = 0;
            ix++;
            while (isdigit(pbg->pbg_toke[ix])) {
                num = (num * 10) + (pbg->pbg_toke[ix] - '0');
                ix++;
            }
            if (num < 1 || num > PBN_NAG_ZERO) {
                pbn_set_error(pbg, "Note number out of range '%d'\n", num);
                note_ok = 0;
            } else if (pbg->pbg_toke[ix] != '=' || pbg->pbg_toke[ix+1]) {
                pbn_set_error(pbg, "Expecting '=' to terminate note reference '%s'\n", pbg->pbg_toke);
                note_ok = 0;
            } else {
                note_num = num;
            }
        } else if (c == '!') {
            ix++;
            c = pbg->pbg_toke[ix];
            if (c == '\0') {
                if (is_play) note_num = PBN_NAG_PLAY_BANG;
                else         note_num = PBN_NAG_BID_BANG;
            } else if (c == '?' && pbg->pbg_toke[ix+1] == '\0') {
                if (is_play) note_num = PBN_NAG_PLAY_BANG_QUESTION;
                else         note_num = PBN_NAG_BID_BANG_QUESTION;
            } else if (c == '!' && pbg->pbg_toke[ix+1] == '\0') {
                if (is_play) note_num = PBN_NAG_PLAY_BANG_BANG;
                else         note_num = PBN_NAG_BID_BANG_BANG;
            } else {
                pbn_set_error(pbg, "Unrecognized suffix '%s'\n", pbg->pbg_toke);
                note_ok = 0;
            }
        } else if (c == '?') {
            ix++;
            c = pbg->pbg_toke[ix];
            note_num = PBN_NAG_ZERO;
            if (c == '\0') {
                if (is_play) note_num += PBN_NAG_PLAY_QUESTION;
                else         note_num += PBN_NAG_BID_QUESTION;
            } else if (c == '?' && pbg->pbg_toke[ix+1] == '\0') {
                if (is_play) note_num += PBN_NAG_PLAY_QUESTION_QUESTION;
                else         note_num += PBN_NAG_BID_QUESTION_QUESTION;
            } else if (c == '!' && pbg->pbg_toke[ix+1] == '\0') {
                if (is_play) note_num += PBN_NAG_PLAY_QUESTION_BANG;
                else         note_num += PBN_NAG_BID_QUESTION_BANG;
            } else {
                pbn_set_error(pbg, "Unrecognized suffix '%s'\n", pbg->pbg_toke);
                note_ok = 0;
            }
        } else if (c == '$') {
            num = 0;
            ix++;
            while (isdigit(pbg->pbg_toke[ix])) {
                num = (num * 10) + (pbg->pbg_toke[ix] - '0');
                ix++;
            }
            if (num < 1 || num > PBN_NAG_MAX) {
                pbn_set_error(pbg, "Note NAG number out of range '%d'\n", num);
                note_ok = 0;
            } else if (pbg->pbg_toke[ix+1]) {
                pbn_set_error(pbg, "Expecting end of NAG. Found '%s'\n", pbg->pbg_toke);
                note_ok = 0;
            } else {
                note_num = num + PBN_NAG_ZERO;
            }
        } else {
            unget_pbn_toke(pbg);
            done = 1;
        }

        if (!done && note_ok) {
            (*annotes) = Realloc(*annotes, char, nannotes + 2);
            (*annotes)[nannotes] = (char)note_num;
            nannotes++;
            (*annotes)[nannotes] = '\0';
        }
    }

    return (note_ok);
}
/***************************************************************/
/*@*/int pbnat_next_bid(
        struct pbnglob * pbg,
        int *bt,
        int *bl,
        int *bs,
        char ** annotes) {

    char c;
    int is_bid;
    int ix;

    *bt = NONE;
    *annotes = NULL;
    is_bid = 1;
    ix = 0;
    get_pbn_toke(pbg);
    if (!pbg->pbg_toke[0]) return (0);

    c = pbg->pbg_toke[ix++];
    if (IS_PBN_END(c)) {
        unget_pbn_toke(pbg);
        return (0);
    }

    if (c >= '1' && c <= '7') {
        *bt = BID;
        *bl = c - '0';
        c = pbg->pbg_toke[ix++];

             if (c == 'c' || c == 'C') *bs = CLUBS;
        else if (c == 'd' || c == 'D') *bs = DIAMONDS;
        else if (c == 'h' || c == 'H') *bs = HEARTS;
        else if (c == 's' || c == 'S') *bs = SPADES;
        else if (c == 'n' || c == 'N') {
            *bs = NOTRUMP;
            c = pbg->pbg_toke[ix];
            if (c == 't' || c == 'T') {
                ix++;
            } else if (c) {
               pbn_set_error(pbg, "Unrecognized bid suit '%c'\n", c);
               is_bid = 0;
            }
        } else {
           pbn_set_error(pbg, "Unrecognized bid suit '%c'\n", c);
           is_bid = 0;
        }
    } else if (c == 'x' || c == 'X') {
        c = pbg->pbg_toke[ix];
        if (c == 'x' || c == 'X') {
            *bt = REDOUBLE;
            ix++;
        } else if (!c) {
            *bt = DOUBLE;
        } else {
            pbn_set_error(pbg, "Unrecognized PGN bid '%s'\n", pbg->pbg_toke);
            is_bid = 0;
        }
    } else if (c == 'p' || c == 'P') {
        *bt = PASS;
        c = pbg->pbg_toke[ix];
        if (c == 'a' || c == 'A') {
            ix++;
            c = pbg->pbg_toke[ix];
            if (c == 's' || c == 'S') {
                ix++;
                c = pbg->pbg_toke[ix];
                if (c == 's' || c == 'S') {
                    ix++;
                } else {
                    pbn_set_error(pbg, "Unrecognized PGN bid '%s'\n", pbg->pbg_toke);
                    is_bid = 0;
                }
            } else {
                pbn_set_error(pbg, "Unrecognized PGN bid '%s'\n", pbg->pbg_toke);
                is_bid = 0;
            }
        } else if (c) {
            pbn_set_error(pbg, "Unrecognized PGN bid '%s'\n", pbg->pbg_toke);
            is_bid = 0;
        }
    } else if (c == 'a' || c == 'A') {
        *bl = ALLPASS_LEVEL;
        *bt = PASS;
        c = pbg->pbg_toke[ix];
        if (c == 'p' || c == 'P') {
            ix++;
            c = pbg->pbg_toke[ix];
            if (c) {
                pbn_set_error(pbg, "Unrecognized PGN bid '%s'\n", pbg->pbg_toke);
                is_bid = 0;
            }
        } else {
            pbn_set_error(pbg, "Unrecognized PGN bid '%s'\n", pbg->pbg_toke);
            is_bid = 0;
        }
    } else if (c == '-') {
        *bt = NOBID;
    } else if (c == '+') {
        *bt = BIDQUERY;
    } else if (c == '*') {
        *bl = AUCTION_DONE_LEVEL;
        *bt = 0;
    } else {
        pbn_set_error(pbg, "Unrecognized PGN bid '%s'\n", pbg->pbg_toke);
        is_bid = 0;
    }

    c = pbg->pbg_toke[ix++];
    if (c) {
        pbn_set_error(pbg, "Extra characters in PGN bid '%s'\n", pbg->pbg_toke);
        is_bid = 0;
    }

    /* Get notes and annotations */
    if (is_bid) {
        is_bid = parse_pbn_annotes(pbg, annotes, 0);
    }

    return (is_bid);
}
/***************************************************************/
/*@*/void pbnat_make_bid(struct pbn_auction_table * pbat, int bt, int bl, int bs, char * annote) {

    if (pbat->pbat_nbids < MAX_BIDS) {
        pbat->pbat_bid[pbat->pbat_nbids] = encode_bid(bt, bl, bs);
        pbat->pbat_annote[pbat->pbat_nbids] = annote;
        pbat->pbat_nbids += 1;
    }
}
/***************************************************************/
int parse_pbn_auction_table(struct pbnglob * pbg, struct pbnrec * pbr)
{
/*
** 3.5  Auction section
*/

    int estat = 0;
    int bt;
    int bl;
    int bs;
    int np;
    int is_bid;
    int bidix;
    int atdone;
    char * annote;

    atdone = 0;
    while (!atdone) {
        is_bid = pbnat_next_bid(pbg, &bt, &bl, &bs, &annote);
        if (!is_bid) {
            atdone = 1;
        } else if (bl == AUCTION_DONE_LEVEL) {
            atdone = 1;
        } else if (bl == ALLPASS_LEVEL) {
            np = 3;
            if (pbr->pbr_auction_table->pbat_nbids == 0) {
                np = 4;
            } else {
                bidix = pbr->pbr_auction_table->pbat_nbids - 1;
                while (np > 0 && bidix >= 0 &&
                    bid_is_pass(pbr->pbr_auction_table->pbat_bid[bidix])) {
                    np--;
                    bidix--;
                }
            }
            while (np) {
                pbnat_make_bid(pbr->pbr_auction_table, PASS, 0, 0, annote);
                np--;
            }
            atdone = 1;
        } else {
            pbnat_make_bid(pbr->pbr_auction_table, bt, bl, bs, annote);
        }
    }

    return (estat);
}
/***************************************************************/
/*@*/void pbnat_make_play(struct pbn_play_table * pbpt, int rank, int suit) {

    if (pbpt->pbpt_ntricks < 13) {
        pbpt->pbpt_trick[pbpt->pbpt_ntricks][pbpt->pbpt_pix] =
            MAKECARD(rank, suit);
        pbpt->pbpt_pix += 1;
        if (pbpt->pbpt_pix > 3) {
            pbpt->pbpt_ntricks += 1;
            pbpt->pbpt_pix = 0;
        }
    }
}
/***************************************************************/
/*@*/int pbnat_next_card(
        struct pbnglob * pbg,
        int *rank,
        int *suit) {

    char c;
    int is_card;
    int is_special;
    int ix;

    (*rank) = 0;
    (*suit) = 0;
    is_card = 1;
    is_special = 0;
    ix = 0;
    get_pbn_toke(pbg);
    if (!pbg->pbg_toke[0]) return (0);

    c = toupper(pbg->pbg_toke[ix++]);
    if (IS_PBN_END(c)) {
        unget_pbn_toke(pbg);
        return (0);
    }

    switch (c) {
        case 'C' : (*suit) = CLUBS;    break;
        case 'D' : (*suit) = DIAMONDS; break;
        case 'H' : (*suit) = HEARTS;   break;
        case 'S' : (*suit) = SPADES;   break;
        case '-' :
            (*suit) = UNKNOWN_PLAY_CARD;
            is_special = 1;
            break;
        case '+' :
            (*suit) = QUERY_PLAY_CARD;
            is_special = 1;
            break;
        case '*' :
            (*suit) = DONE_PLAY_CARD;
            is_special = 1;
            break;
        default  :
            pbn_set_error(pbg, "Unrecognized card suit  '%c'\n", c);
            is_card = 0;
            break;
    }

    if (is_card && !is_special) {
        c = toupper(pbg->pbg_toke[ix++]);
        switch (c) {
            case 'A' : (*rank) = 14; break;
            case 'K' : (*rank) = 13; break;
            case 'Q' : (*rank) = 12; break;
            case 'J' : (*rank) = 11; break;
            case 'T' : (*rank) = 10; break;
            default  :
                if (c >= '2' && c <= '9') (*rank) = c - '0';
                else {
                   pbn_set_error(pbg, "Unrecognized card  '%c'\n", c);
                   is_card = 0;
                }
                break;
        }
    }

    c = pbg->pbg_toke[ix++];
    if (c) {
        pbn_set_error(pbg, "Extra characters in PGN card '%s'\n", pbg->pbg_toke);
        is_card = 0;
    }

    return (is_card);
}
/***************************************************************/
int parse_pbn_play_table(struct pbnglob * pbg, struct pbnrec * pbr)
{
/*
** 3.6  Play section
*/

    int estat = 0;
    int suit;
    int rank;
    int is_card;
    int ptdone;

    ptdone = 0;
    while (!ptdone) {
        is_card = pbnat_next_card(pbg, &rank, &suit);
        if (!is_card) {
            ptdone = 1;
        } else {
            if (!rank) {
                if (suit == QUERY_PLAY_CARD || suit == QUERY_PLAY_CARD) {
                    ptdone = 1;
                }
            }
            pbnat_make_play(pbr->pbr_play_table, rank, suit);
        }
    }

    return (estat);
}
/***************************************************************/
int findstr(char ** strlist, int nstrs, const char * str) {

    int lo;
    int hi;
    int mid;

    lo = 0;
    hi = nstrs - 1;
    while (lo <= hi) {
        mid = (lo + hi) / 2;
        if (strcmp(str, strlist[mid]) <= 0) hi = mid - 1;
        else                                lo = mid + 1;
    }

    if (lo == nstrs || strcmp(str, strlist[lo])) lo = -1;

    return (lo);
}
/***************************************************************/
void qsortstrs(char ** strlist, int l, int r) {
/*
** Quick sort of char ** strlist
**
** to call:  qsortstrs(strlist, 0, nstrs - 1);
*/
    int i;
    int j;
    char * v;
    char * u;

    if (r > l) {
        v = strlist[r]; i = l - 1; j = r;
        for (;;) {
            while (i < j && strcmp(strlist[++i], v) < 0) ; /* use < 0 for asc, > 0 for desc */
            while (i < j && strcmp(strlist[--j], v) > 0) ; /* use > 0 for asc, < 0 for desc */
            if (i >= j) break;
            u = strlist[i]; strlist[i] = strlist[j]; strlist[j] = u;
        }
        u = strlist[i]; strlist[i] = strlist[r]; strlist[r] = u;
        qsortstrs(strlist, l, i-1);
        qsortstrs(strlist, i+1, r);
    }
}
/***************************************************************/
int parse_pbn_table_col_desc(struct pbnglob * pbg, 
                            struct pbn_table * pbnt,
                            char ** permitted_strlist,
                            int npermitted,
                            const char * col_desc)
{
    int estat = 0;
    int strix;
    int colix;
    int iwidth;
    int wix;
    char justify;
    char sorted;
    struct pbn_table_col_desc * pbntcd;
    char col_name[64];
    char col_width[64];

    iwidth = 0;
    justify = '\0';
    sorted = '\0';

    colix = 0;
    get_pbn_toke_str_white(col_desc, &(colix), col_name, sizeof(col_name), '\\');
    if (!col_name[0]) {
        /* done */
    } else if (col_name[0] == ';') {
        /* ignore */
    } else {
        if (col_name[0] == '+' || col_name[0] == '-') {
            sorted = col_name[0];
            memmove(col_name, col_name + 1, IStrlen(col_name));
        }
        strix = findstr(permitted_strlist, npermitted, col_name);
        if (strix < 0) {
            /* pbn_set_warning(pbg, "Unexpected PGN table column: %s", col_name); */
        }
        get_pbn_toke_str_white(col_desc, &(colix), col_width, sizeof(col_width), '\\');
        wix = 0;
        if (col_width[wix] == '\\') {
            get_pbn_toke_str_white(col_desc, &(colix), col_width, sizeof(col_width), 0);
            while (isdigit(col_width[wix])) {
                iwidth = (iwidth * 10) + (col_width[wix] - '0');
                wix++;
            }
            if (col_width[wix] == 'L' || col_width[wix] == 'R') {
                justify = col_width[wix];
                wix++;
            }
        }
        if (col_width[wix]) {
            estat = pbn_set_error(pbg, "Invalid PGN column description: %s", col_desc);
        } else {
            pbntcd = new_pbn_table_col_desc(col_name, iwidth, justify);
            add_table_col_desc(pbnt, pbntcd);
        }
    }

    return (estat);
}
/***************************************************************/
int parse_pbn_table_columns(struct pbnglob * pbg, 
                            struct pbn_table * pbnt,
                            const char * permitted_cols,
                            const char * actual_cols)
{
/*
** 5.2  Description of tables
*/

    int estat = 0;
    char ** permitted_strlist;
    int npermitted;
    int xpermitted;
    int done;
    int ix;

    permitted_strlist = NULL;
    npermitted = 0;
    xpermitted = 0;

    ix = 0;
    done = 0;
    while (!done) {
        get_pbn_toke_str(permitted_cols, &(ix), pbg->pbg_toke, PBN_MAX_TOKE);
        if (!pbg->pbg_toke[0]) {
            done = 1;
        } else if (pbg->pbg_toke[0] == ';') {
            /* ignore */
        } else {
            if (npermitted == xpermitted) {
                if (!xpermitted) xpermitted = 8;
                else             xpermitted *= 2;
                permitted_strlist = Realloc(permitted_strlist, char*, xpermitted);
            }
            permitted_strlist[npermitted] = strdupx(pbg->pbg_toke, 0);
            npermitted++;
            get_pbn_toke_str(permitted_cols, &(ix), pbg->pbg_toke, PBN_MAX_TOKE);
            if (!pbg->pbg_toke[0]) {
                done = 1;
            } else if (pbg->pbg_toke[0] != ';') {
                estat = pbn_set_error(pbg, "Unexpected PGN delimiter '%s' in: %s", pbg->pbg_toke, permitted_cols);
                done = 1;
            }
        }
    }

    if (!estat) {
        qsortstrs(permitted_strlist, 0, npermitted - 1);
        /* for (ix = 0; ix < npermitted; ix++) printf("%2d. %s\n", ix, permitted_strlist[ix]); */


        ix = 0;
        done = 0;
        while (!done) {
            get_pbn_toke_str_white(actual_cols, &(ix), pbg->pbg_toke, PBN_MAX_TOKE, ';');
            if (!pbg->pbg_toke[0]) {
                done = 1;
            } else if (pbg->pbg_toke[0] == ';') {
                /* ignore */
            } else {
                estat = parse_pbn_table_col_desc(pbg, pbnt, permitted_strlist, npermitted, pbg->pbg_toke);
                if (estat) {
                    done = 1;
                } else {
                    get_pbn_toke_str_white(actual_cols, &(ix), pbg->pbg_toke, PBN_MAX_TOKE, ';');
                    if (!pbg->pbg_toke[0]) {
                        done = 1;
                    } else if (pbg->pbg_toke[0] != ';') {
                        estat = pbn_set_error(pbg, "Expecting semicolon as PGN delimiter. Found: %s", pbg->pbg_toke);
                        done = 1;
                    }
                }
            }
        }
    }
    for (ix = 0; ix < npermitted; ix++) {
        Free(permitted_strlist[ix]);
    }
    Free(permitted_strlist);

    return (estat);
}
/***************************************************************/
int parse_pbn_table_rows(struct pbnglob * pbg, struct pbn_table * pbnt)
{
/*
** 5.2  Description of tables
*/

    int estat = 0;
    int col_ix;
    int done;
    int rowdone;

    done = 0;

    pbg->pbg_buf_prev_ix = get_pbn_toke_str_white(pbg->pbg_buf, &(pbg->pbg_buf_ix), pbg->pbg_toke, PBN_MAX_TOKE, 0);
    if (pbg->pbg_toke[0]) {
        estat = pbn_set_error(pbg, "Expecting start of PGN table. Found '%s'", pbg->pbg_toke);
        done = 1;
    } else {
        get_pbn_toke_line(pbg);
        pbg->pbg_buf_prev_ix = get_pbn_toke_str_white(pbg->pbg_buf, &(pbg->pbg_buf_ix), pbg->pbg_toke, PBN_MAX_TOKE, 0);
        if (IS_PBN_END(pbg->pbg_toke[0])) done = 1;
    }

    while (!done) {
        pbnt->pbnt_arows = Realloc(pbnt->pbnt_arows, char**, pbnt->pbnt_nrows + 1);
        pbnt->pbnt_arows[pbnt->pbnt_nrows] = New(char*, pbnt->pbnt_ncols);
        col_ix = 0;
        rowdone = 0;
        while (!rowdone && col_ix < pbnt->pbnt_ncols) {
            pbnt->pbnt_arows[pbnt->pbnt_nrows][col_ix] = strdupx(pbg->pbg_toke, 0);
            pbg->pbg_buf_prev_ix = get_pbn_toke_str_white(pbg->pbg_buf, &(pbg->pbg_buf_ix), pbg->pbg_toke, PBN_MAX_TOKE, 0);
            if (IS_PBN_END(pbg->pbg_toke[0])) {
                rowdone = 1;
            } else {
                col_ix++;
            }
        }
        pbnt->pbnt_nrows += 1;
        if (!rowdone) {
            pbn_set_warning(pbg, "Extra data in PGN table: %s", pbg->pbg_buf + pbg->pbg_buf_ix);
            pbg->pbg_toke[0] = '\0';
        }
        if (pbg->pbg_toke[0]) {
            done = 1;
        } else {
            get_pbn_toke_line(pbg);
            pbg->pbg_buf_prev_ix = get_pbn_toke_str_white(pbg->pbg_buf, &(pbg->pbg_buf_ix), pbg->pbg_toke, PBN_MAX_TOKE, 0);
            if (IS_PBN_END(pbg->pbg_toke[0])) done = 1;
        }
    }
    unget_pbn_toke(pbg);

    return (estat);
}
/***************************************************************/
int parse_pbn_generic_table(struct pbnglob * pbg,
        struct pbnrec * pbr,
        char ** pbn_tag,
        struct pbn_table ** pbnt,
        const char * permitted_cols,
        int tflags)
{
/*
** tflags: unused
*/
    int estat = 0;

    if (*pbn_tag) {
        estat = pbn_set_error(pbg, "Duplicate PGN tag [%s in file: %s", pbg->pbg_toke, pbr->pbr_file_name);
    } else {
        get_pbn_toke(pbg);
        if (pbg->pbg_toke[0] == '\"') {
            (*pbn_tag) = strdupx(pbg->pbg_toke, STRCPYX_REMOVE_QUOTES);
            get_pbn_toke(pbg);
        }
        if (pbg->pbg_toke[0] != ']') {
            estat = pbn_set_error(pbg, "Found \'%s\' when expecting ] for end of PGN tag in file: %s",
                pbg->pbg_toke, pbr->pbr_file_name);
        } else {
            (*pbnt) = new_pbn_table();
            estat = parse_pbn_table_columns(pbg, *pbnt, permitted_cols, *pbn_tag);
            if (!estat) {
                estat = parse_pbn_table_rows(pbg, *pbnt);
            }
        }

    }

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_generic(struct pbnglob * pbg, struct pbnrec * pbr, char ** pbn_tag, int tflags)
{
/*
** tflags: unused
*/
    int estat = 0;

    if (*pbn_tag) {
        estat = pbn_set_error(pbg, "Duplicate PGN tag [%s in file: %s", pbg->pbg_toke, pbr->pbr_file_name);
    } else {
        get_pbn_toke(pbg);
        if (pbg->pbg_toke[0] != '\"') {
            estat = pbn_set_error(pbg, "Expecting quoted string for PGN tag [%s in file: %s",
                pbg->pbg_toke, pbr->pbr_file_name);
        } else {
            (*pbn_tag) = strdupx(pbg->pbg_toke, STRCPYX_REMOVE_QUOTES);
            get_pbn_toke(pbg);
            if (pbg->pbg_toke[0] != ']') {
                estat = pbn_set_error(pbg, "Found \'%s\' when expecting ] for end of PGN tag in file: %s",
                    pbg->pbg_toke, pbr->pbr_file_name);
            }
        }
    }

    return (estat);
}
/*** BEGIN Generated in Excel 11/20/2018 15:25 ************************/
/***************************************************************/
int parse_pbn_tag_actiontable(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = parse_pbn_generic_table(pbg, pbr,
        &(pbr->pbr_tag_actiontable), &(pbr->pbr_action_table), ACTIONTABLE_FIELDS, 0);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_annotator(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = pbn_set_error(pbg, "Unsupported PGN tag [%s in file: %s", pbg->pbg_toke, pbr->pbr_file_name);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_annotatorna(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = pbn_set_error(pbg, "Unsupported PGN tag [%s in file: %s", pbg->pbg_toke, pbr->pbr_file_name);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_application(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = pbn_set_error(pbg, "Unsupported PGN tag [%s in file: %s", pbg->pbg_toke, pbr->pbr_file_name);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_auction(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    if (pbr->pbr_auction_table) {
        estat = pbn_set_error(pbg, "Duplicate PGN tag [%s in file: %s", pbg->pbg_toke, pbr->pbr_file_name);
    } else {
        get_pbn_toke(pbg);
        if (pbg->pbg_toke[0] == '\"') {
            pbr->pbr_tag_auction = strdupx(pbg->pbg_toke, STRCPYX_REMOVE_QUOTES);
            get_pbn_toke(pbg);
        }
        if (pbg->pbg_toke[0] != ']') {
            estat = pbn_set_error(pbg, "Found \'%s\' when expecting ] for end of PGN tag in file: %s",
                pbg->pbg_toke, pbr->pbr_file_name);
        } else {
            pbr->pbr_auction_table = new_pbn_auction_table();
            estat = parse_pbn_auction_table(pbg, pbr);
        }

    }
    if (!estat) pbr->pbr_note_tagid = PBN_TAG_ID_AUCTION;

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_auctiontimetable(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = pbn_set_error(pbg, "Unsupported PGN tag [%s in file: %s", pbg->pbg_toke, pbr->pbr_file_name);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_bidsystemew(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = pbn_set_error(pbg, "Unsupported PGN tag [%s in file: %s", pbg->pbg_toke, pbr->pbr_file_name);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_bidsystemns(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = pbn_set_error(pbg, "Unsupported PGN tag [%s in file: %s", pbg->pbg_toke, pbr->pbr_file_name);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_board(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = parse_pbn_tag_generic(pbg, pbr, &(pbr->pbr_tag_board), 0);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_competition(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = parse_pbn_tag_generic(pbg, pbr, &(pbr->pbr_tag_competition), 0);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_contract(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = parse_pbn_tag_generic(pbg, pbr, &(pbr->pbr_tag_contract), 0);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_date(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = parse_pbn_tag_generic(pbg, pbr, &(pbr->pbr_tag_date), 0);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_deal(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = parse_pbn_tag_generic(pbg, pbr, &(pbr->pbr_tag_deal), 0);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_dealer(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = parse_pbn_tag_generic(pbg, pbr, &(pbr->pbr_tag_dealer), 0);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_dealid(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = pbn_set_error(pbg, "Unsupported PGN tag [%s in file: %s", pbg->pbg_toke, pbr->pbr_file_name);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_declarer(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = parse_pbn_tag_generic(pbg, pbr, &(pbr->pbr_tag_declarer), 0);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_description(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = parse_pbn_tag_generic(pbg, pbr, &(pbr->pbr_tag_description), 0);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_east(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = parse_pbn_tag_generic(pbg, pbr, &(pbr->pbr_tag_east), 0);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_eastna(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = pbn_set_error(pbg, "Unsupported PGN tag [%s in file: %s", pbg->pbg_toke, pbr->pbr_file_name);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_easttype(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = pbn_set_error(pbg, "Unsupported PGN tag [%s in file: %s", pbg->pbg_toke, pbr->pbr_file_name);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_event(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = parse_pbn_tag_generic(pbg, pbr, &(pbr->pbr_tag_event), 0);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_eventdate(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = pbn_set_error(pbg, "Unsupported PGN tag [%s in file: %s", pbg->pbg_toke, pbr->pbr_file_name);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_eventsponsor(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = pbn_set_error(pbg, "Unsupported PGN tag [%s in file: %s", pbg->pbg_toke, pbr->pbr_file_name);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_frenchmp(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = pbn_set_error(pbg, "Unsupported PGN tag [%s in file: %s", pbg->pbg_toke, pbr->pbr_file_name);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_generator(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = pbn_set_error(pbg, "Unsupported PGN tag [%s in file: %s", pbg->pbg_toke, pbr->pbr_file_name);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_hidden(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = pbn_set_error(pbg, "Unsupported PGN tag [%s in file: %s", pbg->pbg_toke, pbr->pbr_file_name);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_hometeam(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = parse_pbn_tag_generic(pbg, pbr, &(pbr->pbr_tag_hometeam), 0);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_instantscoretable(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = pbn_set_error(pbg, "Unsupported PGN tag [%s in file: %s", pbg->pbg_toke, pbr->pbr_file_name);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_mode(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = parse_pbn_tag_generic(pbg, pbr, &(pbr->pbr_tag_mode), 0);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_north(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = parse_pbn_tag_generic(pbg, pbr, &(pbr->pbr_tag_north), 0);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_northna(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = pbn_set_error(pbg, "Unsupported PGN tag [%s in file: %s", pbg->pbg_toke, pbr->pbr_file_name);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_northtype(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = pbn_set_error(pbg, "Unsupported PGN tag [%s in file: %s", pbg->pbg_toke, pbr->pbr_file_name);

    return (estat);
}
/***************************************************************/
static void add_note_to_notelist(struct pbn_note_list * pbnl, struct pbn_note * pbnn)
{
/*
struct pbn_note_list {
    int pbnl_nnote;
    struct pbn_note ** pbnl_anote;
};
*/
    pbnl->pbnl_anote = Realloc(pbnl->pbnl_anote, struct pbr *, pbnl->pbnl_nnote + 1);
    pbnl->pbnl_anote[pbnl->pbnl_nnote] = pbnn;
    pbnl->pbnl_nnote += 1;
}
/***************************************************************/
int parse_pbn_note(struct pbnglob * pbg, struct pbnrec * pbr, struct pbn_note_list * notelist)
{
    int estat = 0;
    int ix;
    int ii;
    int note_id;
    struct pbn_note * pbnn;

    pbnn = NULL;
    if (pbg->pbg_toke[0] == '\"') {
        pbnn = new_pbn_note();
        pbnn->pbnn_note_text = strdupx(pbg->pbg_toke, STRCPYX_REMOVE_QUOTES);
        note_id = 0;
        ix = 0;
        get_pbn_toke_str(pbnn->pbnn_note_text, &(ix), pbg->pbg_toke, PBN_MAX_TOKE);
        ii = 0;
        while (isdigit(pbg->pbg_toke[ii])) {
            note_id = (note_id * 10) + (pbg->pbg_toke[ii] - '0');
            ii++;
        }
        if (note_id < 1 || note_id > PBN_NAG_ZERO || pbg->pbg_toke[ii]) {
            estat = pbn_set_error(pbg, "Invalid PGN note in file: %s", pbg->pbg_toke, pbr->pbr_file_name);
        } else {
            pbnn->pbnn_note_id = note_id;
        }
    }
    get_pbn_toke(pbg);
    if (pbg->pbg_toke[0] != ']') {
        estat = pbn_set_error(pbg, "Found \'%s\' when expecting ] for end of PGN tag in file: %s",
            pbg->pbg_toke, pbr->pbr_file_name);
    }

    if (estat) {
        free_pbn_note(pbnn);
    } else {
        add_note_to_notelist(notelist, pbnn);
    }

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_note(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;
    struct pbn_note_list * notelist;

    notelist = NULL;
    get_pbn_toke(pbg);

    switch (pbr->pbr_note_tagid) {
        case PBN_TAG_ID_AUCTION:
            if (pbr->pbr_auction_table) {
                if (!pbr->pbr_auction_table->pbat_notelist) {
                    pbr->pbr_auction_table->pbat_notelist = new_pbn_note_list();
                }
                notelist = pbr->pbr_auction_table->pbat_notelist;
            }
            break;
        case PBN_TAG_ID_PLAY:
            if (pbr->pbr_play_table) {
                if (!pbr->pbr_play_table->pbpt_notelist) {
                    pbr->pbr_play_table->pbpt_notelist = new_pbn_note_list();
                }
                notelist = pbr->pbr_play_table->pbpt_notelist;
            }
            break;
        default:
            break;
    }

    if (!notelist) {
        estat = pbn_set_error(pbg, "Unmoored PGN note in file: %s", pbg->pbg_toke, pbr->pbr_file_name);
    } else {
        estat = parse_pbn_note(pbg, pbr, notelist);
    }

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_optimumplaytable(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = pbn_set_error(pbg, "Unsupported PGN tag [%s in file: %s", pbg->pbg_toke, pbr->pbr_file_name);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_optimumresulttable(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = pbn_set_error(pbg, "Unsupported PGN tag [%s in file: %s", pbg->pbg_toke, pbr->pbr_file_name);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_optimumscore(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = pbn_set_error(pbg, "Unsupported PGN tag [%s in file: %s", pbg->pbg_toke, pbr->pbr_file_name);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_pairew(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = pbn_set_error(pbg, "Unsupported PGN tag [%s in file: %s", pbg->pbg_toke, pbr->pbr_file_name);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_pairns(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = pbn_set_error(pbg, "Unsupported PGN tag [%s in file: %s", pbg->pbg_toke, pbr->pbr_file_name);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_play(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    if (pbr->pbr_play_table) {
        estat = pbn_set_error(pbg, "Duplicate PGN tag [%s in file: %s", pbg->pbg_toke, pbr->pbr_file_name);
    } else {
        get_pbn_toke(pbg);
        if (pbg->pbg_toke[0] == '\"') {
            pbr->pbr_tag_play = strdupx(pbg->pbg_toke, STRCPYX_REMOVE_QUOTES);
            get_pbn_toke(pbg);
        }
        if (pbg->pbg_toke[0] != ']') {
            estat = pbn_set_error(pbg, "Found \'%s\' when expecting ] for end of PGN tag in file: %s",
                pbg->pbg_toke, pbr->pbr_file_name);
        } else {
            pbr->pbr_play_table = new_pbn_play_table();
            estat = parse_pbn_play_table(pbg, pbr);
        }

    }
    if (!estat) pbr->pbr_note_tagid = PBN_TAG_ID_PLAY;

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_playtimetable(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = pbn_set_error(pbg, "Unsupported PGN tag [%s in file: %s", pbg->pbg_toke, pbr->pbr_file_name);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_result(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = parse_pbn_tag_generic(pbg, pbr, &(pbr->pbr_tag_result), 0);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_room(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = parse_pbn_tag_generic(pbg, pbr, &(pbr->pbr_tag_room), 0);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_round(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = parse_pbn_tag_generic(pbg, pbr, &(pbr->pbr_tag_round), 0);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_score(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = parse_pbn_tag_generic(pbg, pbr, &(pbr->pbr_tag_score), 0);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_scoreimp(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = parse_pbn_tag_generic(pbg, pbr, &(pbr->pbr_tag_scoreimp), 0);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_scoremp(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = pbn_set_error(pbg, "Unsupported PGN tag [%s in file: %s", pbg->pbg_toke, pbr->pbr_file_name);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_scorepercentage(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = pbn_set_error(pbg, "Unsupported PGN tag [%s in file: %s", pbg->pbg_toke, pbr->pbr_file_name);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_scorerubber(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = pbn_set_error(pbg, "Unsupported PGN tag [%s in file: %s", pbg->pbg_toke, pbr->pbr_file_name);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_scorerubberhistory(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = pbn_set_error(pbg, "Unsupported PGN tag [%s in file: %s", pbg->pbg_toke, pbr->pbr_file_name);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_scoretable(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = parse_pbn_generic_table(pbg, pbr,
        &(pbr->pbr_tag_scoretable), &(pbr->pbr_score_table), SCORETABLE_FIELDS, 0);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_scoring(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = parse_pbn_tag_generic(pbg, pbr, &(pbr->pbr_tag_scoring), 0);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_section(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = pbn_set_error(pbg, "Unsupported PGN tag [%s in file: %s", pbg->pbg_toke, pbr->pbr_file_name);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_site(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = parse_pbn_tag_generic(pbg, pbr, &(pbr->pbr_tag_site), 0);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_south(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = parse_pbn_tag_generic(pbg, pbr, &(pbr->pbr_tag_south), 0);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_southna(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = pbn_set_error(pbg, "Unsupported PGN tag [%s in file: %s", pbg->pbg_toke, pbr->pbr_file_name);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_southtype(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = pbn_set_error(pbg, "Unsupported PGN tag [%s in file: %s", pbg->pbg_toke, pbr->pbr_file_name);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_stage(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = parse_pbn_tag_generic(pbg, pbr, &(pbr->pbr_tag_stage), 0);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_table(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = pbn_set_error(pbg, "Unsupported PGN tag [%s in file: %s", pbg->pbg_toke, pbr->pbr_file_name);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_termination(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = pbn_set_error(pbg, "Unsupported PGN tag [%s in file: %s", pbg->pbg_toke, pbr->pbr_file_name);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_time(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = pbn_set_error(pbg, "Unsupported PGN tag [%s in file: %s", pbg->pbg_toke, pbr->pbr_file_name);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_timecall(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = pbn_set_error(pbg, "Unsupported PGN tag [%s in file: %s", pbg->pbg_toke, pbr->pbr_file_name);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_timecard(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = pbn_set_error(pbg, "Unsupported PGN tag [%s in file: %s", pbg->pbg_toke, pbr->pbr_file_name);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_timecontrol(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = pbn_set_error(pbg, "Unsupported PGN tag [%s in file: %s", pbg->pbg_toke, pbr->pbr_file_name);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_totalscoretable(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = parse_pbn_generic_table(pbg, pbr,
        &(pbr->pbr_tag_totalscoretable), &(pbr->pbr_total_score_table), TOTALSCORETABLE_FIELDS, 0);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_utcdate(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = pbn_set_error(pbg, "Unsupported PGN tag [%s in file: %s", pbg->pbg_toke, pbr->pbr_file_name);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_utctime(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = pbn_set_error(pbg, "Unsupported PGN tag [%s in file: %s", pbg->pbg_toke, pbr->pbr_file_name);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_visitteam(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = parse_pbn_tag_generic(pbg, pbr, &(pbr->pbr_tag_visitteam), 0);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_vulnerable(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = parse_pbn_tag_generic(pbg, pbr, &(pbr->pbr_tag_vulnerable), 0);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_west(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = parse_pbn_tag_generic(pbg, pbr, &(pbr->pbr_tag_west), 0);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_westna(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = pbn_set_error(pbg, "Unsupported PGN tag [%s in file: %s", pbg->pbg_toke, pbr->pbr_file_name);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag_westtype(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = pbn_set_error(pbg, "Unsupported PGN tag [%s in file: %s", pbg->pbg_toke, pbr->pbr_file_name);

    return (estat);
}
/*** END Generated in Excel 11/20/2018 11:53 **************************/
/***************************************************************/
int parse_pbn_tag_unrecognized(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;

    estat = pbn_set_error(pbg, "Unrecognized PGN tag [%s in file: %s", pbg->pbg_toke, pbr->pbr_file_name);

    return (estat);
}
/***************************************************************/
int parse_pbn_tag(struct pbnglob * pbg, struct pbnrec * pbr)
{
    int estat = 0;
    int pbntagid;

    pbntagid = pbn_find_tag_by_name(pbg->pbg_toke);

    /*** BEGIN Generated in Excel 11/20/2018 15:17 ************************/
    switch (pbntagid) {
        case PBN_TAG_ID_ACTIONTABLE:
            estat = parse_pbn_tag_actiontable(pbg, pbr);
            break;
        case PBN_TAG_ID_ANNOTATOR:
            estat = parse_pbn_tag_annotator(pbg, pbr);
            break;
        case PBN_TAG_ID_ANNOTATORNA:
            estat = parse_pbn_tag_annotatorna(pbg, pbr);
            break;
        case PBN_TAG_ID_APPLICATION:
            estat = parse_pbn_tag_application(pbg, pbr);
            break;
        case PBN_TAG_ID_AUCTION:
            estat = parse_pbn_tag_auction(pbg, pbr);
            break;
        case PBN_TAG_ID_AUCTIONTIMETABLE:
            estat = parse_pbn_tag_auctiontimetable(pbg, pbr);
            break;
        case PBN_TAG_ID_BIDSYSTEMEW:
            estat = parse_pbn_tag_bidsystemew(pbg, pbr);
            break;
        case PBN_TAG_ID_BIDSYSTEMNS:
            estat = parse_pbn_tag_bidsystemns(pbg, pbr);
            break;
        case PBN_TAG_ID_BOARD:
            estat = parse_pbn_tag_board(pbg, pbr);
            break;
        case PBN_TAG_ID_COMPETITION:
            estat = parse_pbn_tag_competition(pbg, pbr);
            break;
        case PBN_TAG_ID_CONTRACT:
            estat = parse_pbn_tag_contract(pbg, pbr);
            break;
        case PBN_TAG_ID_DATE:
            estat = parse_pbn_tag_date(pbg, pbr);
            break;
        case PBN_TAG_ID_DEAL:
            estat = parse_pbn_tag_deal(pbg, pbr);
            break;
        case PBN_TAG_ID_DEALID:
            estat = parse_pbn_tag_dealid(pbg, pbr);
            break;
        case PBN_TAG_ID_DEALER:
            estat = parse_pbn_tag_dealer(pbg, pbr);
            break;
        case PBN_TAG_ID_DECLARER:
            estat = parse_pbn_tag_declarer(pbg, pbr);
            break;
        case PBN_TAG_ID_DESCRIPTION:
            estat = parse_pbn_tag_description(pbg, pbr);
            break;
        case PBN_TAG_ID_EAST:
            estat = parse_pbn_tag_east(pbg, pbr);
            break;
        case PBN_TAG_ID_EASTNA:
            estat = parse_pbn_tag_eastna(pbg, pbr);
            break;
        case PBN_TAG_ID_EASTTYPE:
            estat = parse_pbn_tag_easttype(pbg, pbr);
            break;
        case PBN_TAG_ID_EVENT:
            estat = parse_pbn_tag_event(pbg, pbr);
            break;
        case PBN_TAG_ID_EVENTDATE:
            estat = parse_pbn_tag_eventdate(pbg, pbr);
            break;
        case PBN_TAG_ID_EVENTSPONSOR:
            estat = parse_pbn_tag_eventsponsor(pbg, pbr);
            break;
        case PBN_TAG_ID_FRENCHMP:
            estat = parse_pbn_tag_frenchmp(pbg, pbr);
            break;
        case PBN_TAG_ID_GENERATOR:
            estat = parse_pbn_tag_generator(pbg, pbr);
            break;
        case PBN_TAG_ID_HIDDEN:
            estat = parse_pbn_tag_hidden(pbg, pbr);
            break;
        case PBN_TAG_ID_HOMETEAM:
            estat = parse_pbn_tag_hometeam(pbg, pbr);
            break;
        case PBN_TAG_ID_INSTANTSCORETABLE:
            estat = parse_pbn_tag_instantscoretable(pbg, pbr);
            break;
        case PBN_TAG_ID_MODE:
            estat = parse_pbn_tag_mode(pbg, pbr);
            break;
        case PBN_TAG_ID_NORTH:
            estat = parse_pbn_tag_north(pbg, pbr);
            break;
        case PBN_TAG_ID_NORTHNA:
            estat = parse_pbn_tag_northna(pbg, pbr);
            break;
        case PBN_TAG_ID_NORTHTYPE:
            estat = parse_pbn_tag_northtype(pbg, pbr);
            break;
        case PBN_TAG_ID_NOTE:
            estat = parse_pbn_tag_note(pbg, pbr);
            break;
        case PBN_TAG_ID_OPTIMUMPLAYTABLE:
            estat = parse_pbn_tag_optimumplaytable(pbg, pbr);
            break;
        case PBN_TAG_ID_OPTIMUMRESULTTABLE:
            estat = parse_pbn_tag_optimumresulttable(pbg, pbr);
            break;
        case PBN_TAG_ID_OPTIMUMSCORE:
            estat = parse_pbn_tag_optimumscore(pbg, pbr);
            break;
        case PBN_TAG_ID_PAIREW:
            estat = parse_pbn_tag_pairew(pbg, pbr);
            break;
        case PBN_TAG_ID_PAIRNS:
            estat = parse_pbn_tag_pairns(pbg, pbr);
            break;
        case PBN_TAG_ID_PLAY:
            estat = parse_pbn_tag_play(pbg, pbr);
            break;
        case PBN_TAG_ID_PLAYTIMETABLE:
            estat = parse_pbn_tag_playtimetable(pbg, pbr);
            break;
        case PBN_TAG_ID_RESULT:
            estat = parse_pbn_tag_result(pbg, pbr);
            break;
        case PBN_TAG_ID_ROOM:
            estat = parse_pbn_tag_room(pbg, pbr);
            break;
        case PBN_TAG_ID_ROUND:
            estat = parse_pbn_tag_round(pbg, pbr);
            break;
        case PBN_TAG_ID_SCORE:
            estat = parse_pbn_tag_score(pbg, pbr);
            break;
        case PBN_TAG_ID_SCOREIMP:
            estat = parse_pbn_tag_scoreimp(pbg, pbr);
            break;
        case PBN_TAG_ID_SCOREMP:
            estat = parse_pbn_tag_scoremp(pbg, pbr);
            break;
        case PBN_TAG_ID_SCOREPERCENTAGE:
            estat = parse_pbn_tag_scorepercentage(pbg, pbr);
            break;
        case PBN_TAG_ID_SCORERUBBER:
            estat = parse_pbn_tag_scorerubber(pbg, pbr);
            break;
        case PBN_TAG_ID_SCORERUBBERHISTORY:
            estat = parse_pbn_tag_scorerubberhistory(pbg, pbr);
            break;
        case PBN_TAG_ID_SCORETABLE:
            estat = parse_pbn_tag_scoretable(pbg, pbr);
            break;
        case PBN_TAG_ID_SCORING:
            estat = parse_pbn_tag_scoring(pbg, pbr);
            break;
        case PBN_TAG_ID_SECTION:
            estat = parse_pbn_tag_section(pbg, pbr);
            break;
        case PBN_TAG_ID_SITE:
            estat = parse_pbn_tag_site(pbg, pbr);
            break;
        case PBN_TAG_ID_SOUTH:
            estat = parse_pbn_tag_south(pbg, pbr);
            break;
        case PBN_TAG_ID_SOUTHNA:
            estat = parse_pbn_tag_southna(pbg, pbr);
            break;
        case PBN_TAG_ID_SOUTHTYPE:
            estat = parse_pbn_tag_southtype(pbg, pbr);
            break;
        case PBN_TAG_ID_STAGE:
            estat = parse_pbn_tag_stage(pbg, pbr);
            break;
        case PBN_TAG_ID_TABLE:
            estat = parse_pbn_tag_table(pbg, pbr);
            break;
        case PBN_TAG_ID_TERMINATION:
            estat = parse_pbn_tag_termination(pbg, pbr);
            break;
        case PBN_TAG_ID_TIME:
            estat = parse_pbn_tag_time(pbg, pbr);
            break;
        case PBN_TAG_ID_TIMECALL:
            estat = parse_pbn_tag_timecall(pbg, pbr);
            break;
        case PBN_TAG_ID_TIMECARD:
            estat = parse_pbn_tag_timecard(pbg, pbr);
            break;
        case PBN_TAG_ID_TIMECONTROL:
            estat = parse_pbn_tag_timecontrol(pbg, pbr);
            break;
        case PBN_TAG_ID_TOTALSCORETABLE:
            estat = parse_pbn_tag_totalscoretable(pbg, pbr);
            break;
        case PBN_TAG_ID_UTCDATE:
            estat = parse_pbn_tag_utcdate(pbg, pbr);
            break;
        case PBN_TAG_ID_UTCTIME:
            estat = parse_pbn_tag_utctime(pbg, pbr);
            break;
        case PBN_TAG_ID_VISITTEAM:
            estat = parse_pbn_tag_visitteam(pbg, pbr);
            break;
        case PBN_TAG_ID_VULNERABLE:
            estat = parse_pbn_tag_vulnerable(pbg, pbr);
            break;
        case PBN_TAG_ID_WEST:
            estat = parse_pbn_tag_west(pbg, pbr);
            break;
        case PBN_TAG_ID_WESTNA:
            estat = parse_pbn_tag_westna(pbg, pbr);
            break;
        case PBN_TAG_ID_WESTTYPE:
            estat = parse_pbn_tag_westtype(pbg, pbr);
            break;
        default:
            estat = parse_pbn_tag_unrecognized(pbg, pbr);
            break;
    }
    /*** END Generated in Excel 11/20/2018 15:17 **************************/

    return (estat);
}
/***************************************************************/
/* (struct pbnglob * pbg, struct pbnrec * pbr) */
int parse_pbnrec(
    struct pbnglob * pbg,
    struct pbnrec * pbr)
{
    int estat = 0;

    get_pbn_toke(pbg);
    estat = pbn_get_estat(pbg, 1);

    /* process pbg->pbg_toke */
    while (!estat && pbg->pbg_toke[0]) {
        if (pbg->pbg_toke[0] == '[') {
            get_pbn_toke(pbg);
            if (!pbg->pbg_toke[0]) {
                estat = pbn_set_error(pbg, "Unexpected end of PGN file: %s", pbr->pbr_file_name);
            } else if (pbg->pbg_toke[0] == ']') {
                /* We'll allow [] */
            } else {
                estat = parse_pbn_tag(pbg, pbr);
                if (!estat) {
                    get_pbn_toke(pbg);
                    estat = pbn_get_estat(pbg, 1);
                }
            }
        } else {
            estat = pbn_set_error(pbg, "Unrecognized PBN text '%s'\n", pbg->pbg_toke);
        }
    }

    return (estat);
}
/***************************************************************/
static void cat_pbnrec_to_pbnlist(struct pbnlist * pbl, struct pbnrec * pbr)
{
    pbl->pbl_apbr = Realloc(pbl->pbl_apbr, struct pbr *, pbl->pbl_npbr + 2);
    pbl->pbl_apbr[pbl->pbl_npbr] = pbr;
    pbl->pbl_npbr += 1;
    pbl->pbl_apbr[pbl->pbl_npbr] = NULL;
}
/***************************************************************/
int parse_pbnfile(struct omarglobals * og,
    struct pbnlist * pbl,
    const char * pbnfname)
{
    int estat = 0;
    int done;
    struct pbnrec * pbr;
    struct pbnglob * pbg;

    pbg = new_pbnglob(og);
    pbr = new_pbnrec(pbg, pbnfname, 0);

    if (og->og_flags & OG_FLAG_VERBOSE) {
        omarout(og, "Processing file: %s\n", pbr->pbr_file_name);
    }

    pbg->pbg_frec = frec_open(pbr->pbr_file_name, "r", "PBN file");
    if (!pbg->pbg_frec) {
        estat = pbn_set_error(pbg, "Error opening PBN file: %s",
            pbr->pbr_file_name);
    }

    if (!estat) {
        done = 0;
        while (!estat && !done) {
            estat = parse_pbnrec(pbg, pbr);
            if (estat) {
                if (estat == ESTAT_EOF) {
                    estat = pbn_set_estat(pbg, 0);
                    done = 1;
                } else {
                    estat = pbn_set_error(pbg, "Error parsing PBN file: %s",
                        pbr->pbr_file_name);
                }
                free_pbnrec(pbr);
            } else {
                cat_pbnrec_to_pbnlist(pbl, pbr);
                pbg->pbg_game_done = 0;
                pbr = new_pbnrec(pbg, pbnfname, pbg->pbg_frec->flinenum);
            }
        }
    }
    free_pbnglob(pbg);

    return (estat);
}
/***************************************************************/
static int svare_method_PBN_ReadFile(struct svarexec * svx,   /* NULL for destructor */
    const char * func_name,     /* NULL for destructor */
    int nsvv,
    struct svarvalue ** asvv,
    struct svarvalue * rsvv)    /* NULL for destructor */
{
    int svstat = 0;
    int estat;
    struct pbnlist * pbl;
    char * fname;
    struct omarglobals * og = (struct omarglobals *)(svx->svx_gdata);

    svstat = svare_check_arguments(svx, func_name, nsvv, asvv,
        "{" SVAR_CLASS_NAME_PBN "}s");

    if (!svstat) {
        pbl = (struct pbnlist *)(asvv[0]->svv_val.svv_svvi.svvi_svviv->svviv_val_ptr);
        fname = asvv[1]->svv_val.svv_chars.svvc_val_chars;
        estat = parse_pbnfile(og, pbl, fname);
        if (estat) {
            svstat = svare_set_error(svx, SVARERR_FUNCTION_FAILED,
                    "Function %s(%s) returned an error %d",
                    func_name, fname, estat);
        }
    }

    return (svstat);
}
/***************************************************************/
void svare_init_internal_pbn_class(struct svarexec * svx)
{
    struct svarvalue * svv;

    /* PBN */
    svv = svare_new_int_class(svx, SVAR_CLASS_ID_PBN);
    svare_new_int_class_method(svv, ".new"              , svare_method_PBNNew);
    svare_new_int_class_method(svv, ".delete"           , svare_method_PBNDelete);
    svare_new_int_class_method(svv, "PBNReadFile"       , svare_method_PBN_ReadFile);
    svare_add_int_class(svx, SVAR_CLASS_NAME_PBN, svv);
}
/***************************************************************/
