/***************************************************************/
/* crehands.h                                                  */
/***************************************************************/

#define NUMBER_OF_HIGH_CARDS    4

struct createhands { /* crh_ */
    char * crh_handtype;
    int    crh_numhands;
    char * crh_outfname;
    long   crh_handno;
};

struct nnhands { /* nnh_ */
    char * nnh_fname;
    int    nnh_method;
};

struct nninit { /* nni_ */
    int nni_method;
    int nni_num_inputs;
    int nni_num_hidden_neurons;
    int nni_num_outputs;
    int nni_init;
    double nni_hidden_layer_bias;
    double nni_output_layer_bias;
    double nni_learning_rate;
    double nni_momentum;
};

struct nntrain { /* nnt_ */
    int nnt_method;
    int nnt_max_epochs;
};

struct nnplay { /* nnb_ */
    int nnp_play_type;
};

struct nnbid { /* nnp_ */
    int nnb_method;
};

struct nndata { /* nnd_ */
    struct neural_network *  nnd_nn;
};

#define NNT_METHOD_MM               1
#define NNT_METHOD_OPENING          2
#define NNT_METHOD_HANDREC_SUITED   3
#define NNT_METHOD_HANDREC_NOTRUMP  4

#define NN_INFO_MM              "MMTest"
#define NN_INFO_OPENING         "Opening"
#define NN_INFO_HANDREC_SUITED  "HandrecSuited"
#define NN_INFO_HANDREC_NOTRUMP "HandrecNotrump"

#define SSORT_ALL           1
#define SSORT_LONGEST       2

#define NNI_INIT_RANDOM     1

#define NN_OPENING_DEFAULT_HIDDEN_LAYER_BIAS    0.1
#define NN_OPENING_DEFAULT_OUTPUT_LAYER_BIAS    0.1

/***************************************************************/
struct playstats { /* pst_ */
    int pst_nhands;
    int pst_ncorrect;
    int pst_nplus1;
    int pst_nminus1;
};

/***************************************************************/

struct createhands * new_createhands(void);
void free_createhands(struct createhands * crh);
int get_createhands(struct omarglobals * og,
        struct createhands * crh,
        int * argix,
        int argc,
        char *argv[]);
int process_createhands(struct omarglobals * og,
        struct createhands * crh);

struct nnhands * new_nnhands(void);
void free_nnhands(struct nnhands * nnt);
int get_nnhands(struct omarglobals * og,
        struct nnhands * nnt,
        int * argix,
        int argc,
        char *argv[]);
int process_nnhands(struct omarglobals * og,
        struct nnhands * nnt);
void free_nntesthandlist(struct nntesthand ** nnthlist);

struct nninit * new_nninit(void);
void free_nninit(struct nninit * crh);
int get_nninit(struct omarglobals * og,
        struct nninit * nni,
        int * argix,
        int argc,
        char *argv[]);
int process_nninit(struct omarglobals * og,
        struct nninit * nni,
        struct nndata * nnd);

int process_nnsave(struct omarglobals * og,
        const char * savfame,
        struct nndata * nnd);
struct nndata * new_nndata(void);
void free_nndata(struct nndata * nnd);

int process_nnload(struct omarglobals * og,
        const char * loadfame,
        struct nndata * nnd);

int process_nntrain(struct omarglobals * og,
        struct nntrain * nnt,
        struct nndata * nnd);
struct nntrain * new_nntrain(void);
void free_nntrain(struct nntrain * nnt);
int get_nntrain(struct omarglobals * og,
        struct nntrain * nnt,
        int * argix,
        int argc,
        char *argv[]);

int process_nnbid(struct omarglobals * og,
        struct nnbid * nnt,
        struct nndata * nnd);
struct nnbid * new_nnbid(void);
void free_nnbid(struct nnbid * nnt);
int get_nnbid(struct omarglobals * og,
        struct nnbid * nnt,
        int * argix,
        int argc,
        char *argv[]);

int process_nnplay(struct omarglobals * og,
        struct nnplay * nnp,
        struct nndata * nnd);
struct nnplay * new_nnplay(void);
void free_nnplay(struct nnplay * nnp);
int get_nnplay(struct omarglobals * og,
        struct nnplay * nnp,
        int * argix,
        int argc,
        char *argv[]);
struct neural_network * nnt_create_new_nn(
        struct omarglobals * og,
        struct nninit * nni);

/* for xeq files */
int nnt_train_handrec(struct omarglobals * og,
            struct neural_network *  nn,
            struct handrec ** ahandrec,
            int suit_sorting,
            int train_method,
            int max_epochs);
int nnt_play_handrec(struct omarglobals * og,
        struct neural_network *  nn,
        struct handrec ** ahandrec,
        int suit_sorting,
        int train_method,
        struct playstats * pst);
void show_playstats(struct omarglobals * og, struct playstats * pst);
void init_playstats(struct playstats * pst);

int nnt_save(struct omarglobals * og,
        struct neural_network *  nn,
        const char * savfname);
int nnt_jsonobject_to_neural_network(struct omarglobals * og,
        struct jsonobject * jo,
        struct neural_network ** pnn);
int get_nn_jsontree(
    struct omarglobals * og,
    const char * file_name,
    struct jsontree ** pjt);
int nnt_verify_handrec_32_inputs(struct omarglobals * og,
            struct handrec * hr,
            int suit_sorting);
int nnt_verify_handrec_1_outputs(struct omarglobals * og,
            struct handrec * hr);
int nnt_get_play_output(
        struct neural_network *  nn,
        int num_inputs,
        ONN_FLOAT * inputs,
        int num_outputs,
        ONN_FLOAT * outputs);
void nnt_calc_suit_order_32(
            struct handrec * hr,
            int num_inputs,
            double * inputs,
            int declarer,
            int trump_suit,
            int num_highcards,
            int suit_sorting,
            int * suit_order);
void nnt_new_handrec_32_inputs(
            struct handrec * hr,
            int num_inputs,
            double * inputs,
            int declarer,
            int trump_suit,
            int num_highcards,
            int * suit_order);
void nnt_new_handrec_1_outputs(
            struct handrec * hr,
            int num_outputs,
            double * outputs,
            int declarer,
            int trump_suit);
void nnt_show_play_handrec(struct omarglobals * og,
        int trump_suit,
        int econtract,
        int num_inputs,
        ONN_FLOAT * inputs);
int count_hand_hcp(HAND * hand);

/***************************************************************/
