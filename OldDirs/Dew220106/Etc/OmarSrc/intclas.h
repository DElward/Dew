#ifndef INTCLASH_INCLUDED
#define INTCLASH_INCLUDED
/***************************************************************/
/* intclas.h                                                   */
/***************************************************************/
/***************************************************************/

#if IS_DEBUG
    #define IS_TESTING_CLASS   0
#else
    #define IS_TESTING_CLASS   0
#endif

/***********************************************************************/
/*    **** NOTE: Make sure to update struct internal_class_def         */
/***********************************************************************/
#define SVAR_CLASS_ID_HAND_RECORD_LIST      16388
#define SVAR_CLASS_ID_INTEGER               16389
#define SVAR_CLASS_ID_NEURAL_NET            16390
#define SVAR_CLASS_ID_TESTING               16391
#define SVAR_CLASS_ID_THREAD                16392
#define SVAR_CLASS_ID_THREAD_HANDLE         16393
#define SVAR_CLASS_ID_WIRE                  16394
#define SVAR_CLASS_ID_AUCTION               16395
#define SVAR_CLASS_ID_ANALYSIS              16396
#define SVAR_CLASS_ID_CONVCARD              16397
#define SVAR_CLASS_ID_HAND                  16398
#define SVAR_CLASS_ID_HANDS                 16399
#define SVAR_CLASS_ID_BIDDING               16400
#define SVAR_CLASS_ID_PBN                   16401

#define SVAR_CLASS_NAME_ANALYSIS            "Analysis"
#define SVAR_CLASS_NAME_AUCTION             "Auction"
#define SVAR_CLASS_NAME_BIDDING             "Bidding"
#define SVAR_CLASS_NAME_CONVCARD            "ConvCard"
#define SVAR_CLASS_NAME_HAND                "Hand"
#define SVAR_CLASS_NAME_HAND_RECORD_LIST    "HandRecordList"
#define SVAR_CLASS_NAME_HANDS               "Hands"
#define SVAR_CLASS_NAME_INTEGER             "Integer"
#define SVAR_CLASS_NAME_NEURAL_NET          "NeuralNet"
#define SVAR_CLASS_NAME_PBN                 "PBN"
#define SVAR_CLASS_NAME_TESTING             "Testing"
#define SVAR_CLASS_NAME_THREAD              "Thread"
#define SVAR_CLASS_NAME_THREAD_HANDLE       "ThreadHandle"
#define SVAR_CLASS_NAME_WIRE                "Wire"

struct jsinteger {   /* jsi_ */
    int jsi_value;
};

struct jsdouble {   /* jsd_ */
    double jsd_value;
};

struct auction_class {   /* auc_ */
    struct auction * auc_auction;
};

struct analysis_class {   /* ans_ */
    struct analysis_info  * ans_aninfo;
    struct seqinfo        * ans_seqinfo;
};

struct hand_class {   /* hnd_ */
    struct handinforec    * hnd_handinfo;
    struct hand           * hnd_hand;
};

struct hands_class {   /* hns_ */
    struct handinforec    * hns_handinfo;
    struct hand           * hns_hand;
    int                     hns_hand_mask;
};

#if 0
struct bid_class {   /* bidc_ */
    int bidc_encoded_bid;
};
#endif

void svare_init_internal_classes(struct svarexec * svx);
void svare_init_internal_functions(struct svarexec * svx);

int svare_check_arguments(struct svarexec * svx,
    const char * func_name,
    int nsvv,
    struct svarvalue ** asvv,
    const char * argmask);

int svare_add_int_class(struct svarexec * svx,
    const char * vnam,
    struct svarvalue * svv);
void svare_new_int_class_method(struct svarvalue * svv,
    const char * func_name,
    SVAR_INTERNAL_FUNCTION ifuncptr);
struct svarvalue * svare_new_int_class(struct svarexec * svx, int class_id);

/***************************************************************/
#endif /* INTCLASH_INCLUDED */
