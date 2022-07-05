/***************************************************************/
/*  demi.h                                                     */
/***************************************************************/
#ifndef DEMIH_INCLUDED
#define DEMIH_INCLUDED

#define DEMI_TESTING    1

typedef char int8;
typedef unsigned char uint8;
typedef float float32;
typedef double float64;

#define DSTAT_WORKER_SHUTDOWN               16

#define DEMIERR_NOT_STARTED                 (-7400)
#define DEMIERR_CHANNEL_UNSUPPORTED         (-7401)
#define DEMIERR_TOKEN_OVERFLOW              (-7402)
#define DEMIERR_MISSING_CLOSE_QUOTE         (-7403)
#define DEMIERR_INVALID_BACKSLASH_ESCAPE    (-7404)
#define DEMIERR_EXPECTING_NUMBER            (-7405)
#define DEMIERR_UNDEFINED_VARIABLE          (-7406)
#define DEMIERR_EXP_CONST_VARIABLE          (-7407)
#define DEMIERR_UNDEFINED_VALUE_TYPE        (-7408)
#define DEMIERR_NULL_VALUE_TYPE             (-7409)
#define DEMIERR_EXP_CLOSE_PAREN             (-7410)
#define DEMIERR_UNEXP_CHAR                  (-7411)
#define DEMIERR_DTYPE_MISMATCH              (-7412)
#define DEMIERR_BAD_OP_DTYPE                (-7413)
#define DEMIERR_DUPLICATE_CONST             (-7414)
#define DEMIERR_EXP_END_OF_LINE             (-7415)
#define DEMIERR_MSG_LENGTH_SHORT            (-7416)
#define DEMIERR_MSG_LENGTH_IMPROPER         (-7417)
#define DEMIERR_UNRECOGNIZED_WORKER_MSG     (-7418)
#define DEMIERR_UNRECOGNIZED_BOSS_MSG       (-7419)
#define DEMIERR_CONFIG_MISMATCH             (-7420)
#define DEMIERR_DUPLICATE_TYPE              (-7421)
#define DEMIERR_SEND_LENGTH_MISMATCH        (-7422)
#define DEMIERR_RECV_INT_OVFL               (-7423)
#define DEMIERR_RECV_CSTR_OVFL              (-7424)
#define DEMIERR_RECV_VAL_OVFL               (-7425)
#define DEMIERR_RECV_LENGTH_MISMATCH        (-7426)
#define DEMIERR_TYPE_NOT_SUPPORTED          (-7427)
#define DEMIERR_TYPE_EXP_CLOSE_PAREN        (-7428)
#define DEMIERR_TYPE_NOT_UNSIGNED           (-7429)
#define DEMIERR_UNDEFINED_TYPE              (-7430)
#define DEMIERR_UNDEFINED_SIMPLE_TYPE       (-7431)
#define DEMIERR_EXP_INT_ARRAY_BOUNDS        (-7432)
#define DEMIERR_STRUCT_EMPTY                (-7433)
#define DEMIERR_BAD_FIELD_NAME              (-7434)
#define DEMIERR_BAD_FIELD_SEPARATOR         (-7435)
#define DEMIERR_TYPE_SIZE_MISMATCH          (-7436)
#define DEMIERR_TYPE_UNSUPPORTED_ARRAY      (-7437)
#define DEMIERR_FUNC_IN_TYPE_UNRECOGNIZED   (-7438)
#define DEMIERR_FUNC_OUT_TYPE_UNRECOGNIZED  (-7439)
#define DEMIERR_FUNC_IN_TYPE_NOT_TYPE       (-7440)
#define DEMIERR_FUNC_OUT_TYPE_NOT_TYPE      (-7441)
#define DEMIERR_FUNC_NOT_FUNC               (-7442)
#define DEMIERR_DUPLICATE_FUNC              (-7443)
#define DEMIERR_DUPLICATE_FUNC_REGISTRATION (-7444)
#define DEMIERR_PTR_ARRAY_STRUCT            (-7445)
#define DEMIERR_PTR_ARRAY_ARRAY             (-7446)
#define DEMIERR_PRINT_ERR                   (-7447)
#define DEMIERR_ERR_MSGLEN                  (-7448)
#define DEMIERR_WORKER_ERROR                (-7449)
#define DEMIERR_MAX_ARRAY_DIMENSIONS        (-7450)
#define DEMIERR_UNRECOGNIZED_TYPE           (-7451)
#define DEMIERR_UNSUPPORTED_TYPE            (-7452)
#define DEMIERR_UNIMPLEMENTED_INST          (-7453)

#define DEMCHAN_UNSUPPORTED                 (-7501)

#define DEMITYP_NONE        0
#define DEMITYP_CHAR        1
#define DEMITYP_INT8        2
#define DEMITYP_WCHAR       3
#define DEMITYP_INT16       4
#define DEMITYP_INT32       5
#define DEMITYP_INT64       6
#define DEMITYP_FLOAT32     7
#define DEMITYP_FLOAT64     8
#define DEMITYP_VOID        9
#define DEMITYP_STRUCT      16
#define DEMITYP_ARRAY       17
#define DEMITYP_POINTER     18

#define DEMITYP_SIMPLE_MASK (0x0F)

#define DEMI_DEFAULT_CHANNEL_TYPE   DEMI_CHANNEL_DIRECT
#define DEMI_MAX_CHANNELS   1
#define DEMI_MAX_CONST_SIZE 255
#define DEMI_ERRMSG_SIZE    128
#define DEMI_MAX_TOKE_SIZE  127

#define DEMI_CFGIX_ENDIAN           0   /* 0=little endian, 1=big endian */
#define DEMI_CFGIX_ALIGN            1   /* 4=4-byte aligned, etc. */
#define DEMI_CFGIX_CHAR_BYTES       2
#define DEMI_CFGIX_SHORT_BYTES      3
#define DEMI_CFGIX_INT_BYTES        4
#define DEMI_CFGIX_LONG_BYTES       5
#define DEMI_CFGIX_WCHAR_BYTES      6
#define DEMI_CFGIX_PTR_BYTES        7
#define DEMI_CFGIX_FLOAT_BYTES      8
#define DEMI_CFGIX_DOUBLE_BYTES     9
#define DEMI_CFGIX_LONG_LONG_BYTES  10
#define DEMI_CFGIX_AVAIL_1          11  /* Available for something else */
#define DEMI_CFG__SIZE              12

#define DEMI_CONST_MIN              3

#define DEMI_ITMTYP_CONST           1
#define DEMI_ITMTYP_NAMED_TYPE      2
#define DEMI_ITMTYP_FUNC            3

typedef unsigned short DINST;   /* 16 bits */

struct demiprog {    /* demp_ */
    int     demp_ninst;
    int     demp_xinst;
    DINST * demp_ainst;
};

struct demifield {    /* demf_ */
    char * demf_name;
    int    demf_offset;
    int    demf_aligned_size;
    struct demityp * demf_dmityp;
};

struct demityp_struct {    /* demts_ */
    int     demts_num_fields;
    int     demts_max_fields;
    int     demts_total_size;
    struct demifield ** demts_fields;
};

typedef int (*DEMI_CALLABLE_FUNCTION)(
    void * funcin,
    void * funcout);

struct demifunc {    /* demu_ */
    char * demu_name;
    int demu_func_index;
    struct deminamedtyp * demu_in_demnt;
    struct deminamedtyp * demu_out_demnt;
    DEMI_CALLABLE_FUNCTION demu_func;
};

/* array types */
//#define DEMTA_ARRAY_TYPE_SINGLE_ITEM    1
//#define DEMTA_ARRAY_TYPE_NULL_TERM      2   /* null terminated array */
#define DEMTA_ARRAY_TYPE_FIXED_SIZE     4
//#define DEMTA_ARRAY_TYPE_VAR_EXP        5   /* Variable with length as an expression */
//#define DEMTA_ARRAY_TYPE_VOID_PTR       6

/* pointer types */
#define DEMTP_PTR_TYPE_SINGLE_ITEM      1
#define DEMTP_PTR_TYPE_NULL_TERM        2   /* null terminated array */
//#define DEMTA_PTR_TYPE_FIXED_SIZE       4
//#define DEMTA_PTR_TYPE_VAR_EXP          5   /* Variable with length as an expression */
#define DEMTP_PTR_TYPE_VOID_PTR         6

struct demityp_array {    /* demta_ */
    int     demta_array_type;
    int     demta_num_elements;
    int     demta_array_stride;
    int     demta_total_size;
    struct demityp * demta_dmityp;
};

struct demityp_ptr {    /* demtp_ */
    int     demtp_ptr_type;
    char    demtp_ptrdat;   /* either '*' or '^', '*' indicates need free() */
    int     demtp_array_stride;
    struct demityp * demtp_dmityp;
};

#define DEMT_TYP_FLAG_UNSIGNED      1
#define DEMT_TYP_FLAG_VARIABLE_SIZE 2

#define DEMT_DISP_FLAG_NO_NL        1

struct demityp {    /* demt_ */
    int    demt_simp_typ;
    int    demt_typ_flags;
    int    demt_typ_size;
    union {
        struct demityp_struct * demt_demts;
        struct demityp_array * demt_demta;
        struct demityp_ptr * demt_demtp;
        void * demt_ptr;
    } demt_u;
};

struct deminamedtyp {    /* demnt_ */
    struct demityp *    demnt_demt;
    char *              demnt_name;
    struct demiitem *   demnt_named_demiitem;
    struct demiprog *   demnt_demp_pack;
    struct demiprog *   demnt_demp_unpack;
    struct demimemory * demnt_pack_tgtdemmem;
    struct demimemory * demnt_unpack_tgtdemmem;
};

#define FUNC_REF_BASE       64
#define FUNC_INDEX_TO_REF(ii) ((ii) + FUNC_REF_BASE)
#define FUNC_REF_TO_INDEX(ir) ((ir) - FUNC_REF_BASE)

struct demiitem {    /* demm_ */
    char * demm_item_nam;
    int    demm_item_typ;
    union {
        struct deminamedtyp * demm_demnt;
        struct demival * demm_demv;
        struct demifunc * demm_demu;
        void * demm_ptr;
    } demm_u;
};

#define DEMI_CHANNEL_DIRECT         0   /* Direct calls, no threading */
#define DEMI_CHANNEL_SHARED_MEMORY  1   /* Threads using shared memory to communicate */
#define DEMI_CHANNEL_PIPES          2   /* Threads using pipes to communicate */
#define DEMI_CHANNEL_UDP            3   /* Processes using UDP to communicate */
#define DEMI_CHANNEL_TCP            4   /* Processes using TCP to communicate */

#define DMTW_CFG                    17
#define DMTW_CONST                  18
#define DMTW_TYPE                   19
#define DMTW_FUNC_DEFINE            20
#define DMTW_FUNC_CALL              21
#define DMTW_SHUTDOWN               22

#define DMTB_OK                     33
#define DMTB_ERR                    34
#define DMTB_FUNC_RESULT            35
#define DMTB_SHUTTING_DOWN          36

#define DEMC_MSG_HEADER_LEN         3
#define DEMC_MIN_MESSAGE_LENGTH     DEMC_MSG_HEADER_LEN
#define DEMC_MSG_HEADER_LEN         3
#define DEMC_ERR_MIN_LEN            4

#define DEMIMEMORY_DEFAULT_SIZE     16
#define DEMI_PKT_HEAD_LEN           3
#define DEMI_PKT_TAIL_LEN           0

#define MAX_ARRAY_DIMENSIONS        8

struct demimemory {    /* demmem_ */
    int demmem_size;
    int demmem_length;
    uchar * demmem_ptr;
};

struct demi_worker {    /* demw_ */
    unsigned char demw_boss_cfg[DEMI_CFG__SIZE];
    int demw_channel_type;
    struct demichannel * demw_channel;
    struct demirec * demw_demi;
};

struct demichan_direct {    /* demcd_ */
    struct demi_worker * demcd_demw;
};

struct demichannel {    /* demc_ */
    int demc_channel_type;
    int demc_pkt_send_sn;
    int demc_pkt_recv_sn;
    struct demimemory * demc_recv_demmem;
    struct demimemory * demc_send_demmem;
    union {
        struct demichan_direct * demc_demcd;
        void * demc_ptr;
    } demc_u;
};

#define DEMIVAL_TYP_BOOL        1
#define DEMIVAL_TYP_CHAR        2 /* unused */
#define DEMIVAL_TYP_INT         3
#define DEMIVAL_TYP_STR         4

struct demival {    /* demv_ */
    int    demt_val_typ;
    union {
        int    demv_bool;
        /* char   demv_char; - unused */
        int    demv_int;
        char * demv_str;
        void * demv_ptr;
    } demv_u;
};

struct demirec {    /* demi_ works the same on both sides */
    int demi_dstat;
    int demi_flags;
    int demi_nfuncs;
    int demi_xfuncs;
    struct demifunc ** demi_funcs;
    char * demi_errmsg;
    struct dbtree * demi_itemtree;   /* struct demiitem * */
    uchar demi_cfg[DEMI_CFG__SIZE];
};

#define DEMS_FLAG_STARTED       1

struct calc_opportunity_iota_best_ARGS {
    struct gstate * gst;
    struct opportunityitem * opi;
    count_t player;
    count_t depth;
};

struct calc_opportunity_iota_best_OUT {
    int opiota;
};

struct demi_boss {    /* demb_ only for the master side */
    int demb_flags;
    int demb_channel_type;
    int demb_max_channels; 
    int demb_nchannels;
    int demb_nfuncs;
    struct demimemory * demb_send_buffer;
    struct demichannel ** demb_channel;
    struct demirec * demb_demi;
};

int demi_set_error(struct demirec * demi, int dstat, const char * fmt, ...);

struct demi_boss * demb_new_demi_boss(void);
void demi_get_errmsg(struct demirec * demi, char * errmsg, int errmsgsz);
char * demb_get_errmsg_ptr(struct demi_boss * demb);

void demb_shut(struct demi_boss * demb);
int demb_start(struct demi_boss * demb, const char * funccfg);
int demb_define_type(
    struct demi_boss * demb,
    const char * typname,
    const char * typdesc,
    size_t typsize);
int demb_define_func(
    struct demi_boss * demb,
    const char * funcname,
    const char * funcinargs,
    const char * funcoutargs,
    void ** vpdemu);
int demb_define_const(
    struct demi_boss * demb,
    const char * constname,
    const char * fmt, ...);
int demb_call_function(
    struct demi_boss * demb,
    void * vdemu,
    void * funcinargs,
    void * funcoutrtn);
int demb_wait_all_funcs(struct demi_boss * demb, int wait_flags);

/***************************************************************/
#endif /* DEMIH_INCLUDED */
/***************************************************************/
/*
struct gstate {
	card_t              gst_hand[MAX_PLAYERS][CARDS_PER_PLAYER];
    count_t             gst_nhand[MAX_PLAYERS];
	struct slot         gst_board[MAX_SLOTS_IN_BOARD];
    count_t             gst_last_take;
    count_t             gst_nboard;
    count_t             gst_nturns;
    count_t             gst_flags;
	card_t              gst_taken[MAX_PLAYERS][FIFTY_TWO];
    count_t             gst_ntaken[MAX_PLAYERS];
};
struct opportunityitem {
	count_t             opi_type;
	count_t             opi_cardix;
	count_t             opi_player;
	count_t             opi_bdlvalue;
    bitmask_t           opi_slotmask;
#if IS_DEBUG
	int                 opi_opiota;
#endif
};
point_t calc_opportunity_iota_best(
    struct gstate * gst,
    struct opportunityitem * opi,
    count_t player,
    count_t depth)
dstat = demb_define_type(demi, "count_t", "int8", sizeof(count_t));
dstat = demb_define_type(demi, "point_t", "int16", sizeof(point_t));
dstat = demb_define_type(demi, "bitmask_t", "uint16", sizeof(bitmask_t));
dstat = demb_define_type(demi, "card_t", "int8", sizeof(card_t));
*/