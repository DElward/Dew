/***************************************************************/
/* onn.h                                                       */
/***************************************************************/

typedef double ONN_FLOAT;

struct neuron {   /* ne_ */
    int         ne_num_weights;
    ONN_FLOAT * ne_weights;
    ONN_FLOAT * ne_prev_delta;
    ONN_FLOAT   ne_output;
    ONN_FLOAT   ne_bias;
    int         ne_num_inputs;
    ONN_FLOAT * ne_inputs;
};
struct neural_layer {   /* nl_ */
    int         nl_num_neurons;
    ONN_FLOAT   nl_bias;
    struct neuron ** nl_neurons;
};
struct neural_network {   /* nn_ */
    char * nn_info;
    char * nn_elapsed_time;
    ONN_FLOAT nn_learning_rate;
    ONN_FLOAT nn_momentum;
    int nn_num_inputs;
    int nn_num_outputs;
    int nn_num_epochs;
    ONN_FLOAT nn_total_error;
    ONN_FLOAT   * nn_inputs;
    struct neural_layer * nn_hidden_layer;
    struct neural_layer * nn_output_layer;
};

/************************/
/**  nn_set_config     **/
/************************/

#define NNCFG_HIDDEN_LAYER_BIAS     1
#define NNCFG_OUTPUT_LAYER_BIAS     2
#define NNCFG_LEARNING_RATE         3
#define NNCFG_MOMENTUM              4

int nn_set_config(struct neural_network * nn, int cfgcmd, void * cfgparm);
/***************************************************************/
struct neural_network * nn_new(void);
struct neural_network * nn_new_init(
    int num_inputs,
    int num_hidden,
    int num_outputs,
    int num_hidden_layer_weights,
    ONN_FLOAT * hidden_layer_weights,
    ONN_FLOAT hidden_layer_bias,
    int num_output_layer_weights,
    ONN_FLOAT * output_layer_weights,
    ONN_FLOAT output_layer_bias,
    ONN_FLOAT learning_rate,
    ONN_FLOAT momentum);
void nn_free(struct neural_network *  nn);
struct neural_network * nn_dup(struct neural_network * nn);
void nn_train(struct neural_network *  nn,
    int num_training_inputs,
    ONN_FLOAT * training_inputs,
    int num_training_outputs,
    ONN_FLOAT * training_outputs);
ONN_FLOAT nn_calculate_total_error(struct neural_network *  nn,
    int num_training_inputs,
    ONN_FLOAT * training_inputs,
    int num_training_outputs,
    ONN_FLOAT * training_outputs);
void nl_free(struct neural_layer *  nl);
struct neural_layer * nl_new(void);
struct neuron * ne_new(void);
void ne_free(struct neuron *  ne);

int nn_forward(struct neural_network *  nn,
    int num_training_inputs,
    ONN_FLOAT * training_inputs,
    int num_outputs,
    ONN_FLOAT * outputs);
void  nn_set_elapsed_time(struct neural_network *  nn,
        const char * elap_tim,
        const char * time_fmt);
void nn_randomize_weights(
    int num_hidden_layer_weights,
    ONN_FLOAT * hidden_layer_weights,
    int num_output_layer_weights,
    ONN_FLOAT * output_layer_weights);
struct neural_network * nn_new_default(
    int num_inputs,
    int num_hidden,
    int num_outputs);

/***************************************************************/

