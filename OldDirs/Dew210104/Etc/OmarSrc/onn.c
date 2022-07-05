/***************************************************************/
/* onn.c                                                       */
/***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <math.h>

#include "config.h"
#include "cmemh.h"
#include "onn.h"
#include "randnum.h"
#include "util.h"

#ifdef PYTHON_SOURCE
################################################################
# From: https://github.com/mattm/simple-neural-network/blob/master/neural-network.py
################################################################
import random
import math

#
# Shorthand:
#   "pd_" as a variable prefix means "partial derivative"
#   "d_" as a variable prefix means "derivative"
#   "_wrt_" is shorthand for "with respect to"
#   "w_ho" and "w_ih" are the index of weights from hidden to output layer neurons and input to hidden layer neurons respectively
#
# Comment references:
#
# [1] Wikipedia article on Backpropagation
#   http://en.wikipedia.org/wiki/Backpropagation#Finding_the_derivative_of_the_error
# [2] Neural Networks for Machine Learning course on Coursera by Geoffrey Hinton
#   https://class.coursera.org/neuralnets-2012-001/lecture/39
# [3] The Back Propagation Algorithm
#   https://www4.rgu.ac.uk/files/chapter3%20-%20bp.pdf

class NeuralNetwork:
    LEARNING_RATE = 0.5

    def __init__(self, num_inputs, num_hidden, num_outputs, hidden_layer_weights = None, hidden_layer_bias = None, output_layer_weights = None, output_layer_bias = None):
        self.num_inputs = num_inputs

        self.hidden_layer = NeuronLayer(num_hidden, hidden_layer_bias)
        self.output_layer = NeuronLayer(num_outputs, output_layer_bias)

        self.init_weights_from_inputs_to_hidden_layer_neurons(hidden_layer_weights)
        self.init_weights_from_hidden_layer_neurons_to_output_layer_neurons(output_layer_weights)

    def init_weights_from_inputs_to_hidden_layer_neurons(self, hidden_layer_weights):
        weight_num = 0
        for h in range(len(self.hidden_layer.neurons)):
            for i in range(self.num_inputs):
                if not hidden_layer_weights:
                    self.hidden_layer.neurons[h].weights.append(random.random())
                else:
                    self.hidden_layer.neurons[h].weights.append(hidden_layer_weights[weight_num])
                weight_num += 1

    def init_weights_from_hidden_layer_neurons_to_output_layer_neurons(self, output_layer_weights):
        weight_num = 0
        for o in range(len(self.output_layer.neurons)):
            for h in range(len(self.hidden_layer.neurons)):
                if not output_layer_weights:
                    self.output_layer.neurons[o].weights.append(random.random())
                else:
                    self.output_layer.neurons[o].weights.append(output_layer_weights[weight_num])
                weight_num += 1

    def inspect(self):
        print('------')
        print('* Inputs: {}'.format(self.num_inputs))
        print('------')
        print('Hidden Layer')
        self.hidden_layer.inspect()
        print('------')
        print('* Output Layer')
        self.output_layer.inspect()
        print('------')

    def feed_forward(self, inputs):
        hidden_layer_outputs = self.hidden_layer.feed_forward(inputs)
        return self.output_layer.feed_forward(hidden_layer_outputs)

    # Uses online learning, ie updating the weights after each training case
    def train(self, training_inputs, training_outputs):
        self.feed_forward(training_inputs)

        # 1. Output neuron deltas
        pd_errors_wrt_output_neuron_total_net_input = [0] * len(self.output_layer.neurons)
        for o in range(len(self.output_layer.neurons)):

            //# ∂E/∂zⱼ
            pd_errors_wrt_output_neuron_total_net_input[o] = self.output_layer.neurons[o].calculate_pd_error_wrt_total_net_input(training_outputs[o])

        # 2. Hidden neuron deltas
        pd_errors_wrt_hidden_neuron_total_net_input = [0] * len(self.hidden_layer.neurons)
        for h in range(len(self.hidden_layer.neurons)):

            # We need to calculate the derivative of the error with respect to the output of each hidden layer neuron
            # dE/dyⱼ = Σ ∂E/∂zⱼ * ∂z/∂yⱼ = Σ ∂E/∂zⱼ * wᵢⱼ
            d_error_wrt_hidden_neuron_output = 0
            for o in range(len(self.output_layer.neurons)):
                d_error_wrt_hidden_neuron_output += pd_errors_wrt_output_neuron_total_net_input[o] * self.output_layer.neurons[o].weights[h]

            //# ∂E/∂zⱼ = dE/dyⱼ * ∂zⱼ/∂
            pd_errors_wrt_hidden_neuron_total_net_input[h] = d_error_wrt_hidden_neuron_output * self.hidden_layer.neurons[h].calculate_pd_total_net_input_wrt_input()

        # 3. Update output neuron weights
        for o in range(len(self.output_layer.neurons)):
            for w_ho in range(len(self.output_layer.neurons[o].weights)):

                //# ∂Eⱼ/∂wᵢⱼ = ∂E/∂zⱼ * ∂zⱼ/∂wᵢⱼ
                pd_error_wrt_weight = pd_errors_wrt_output_neuron_total_net_input[o] * self.output_layer.neurons[o].calculate_pd_total_net_input_wrt_weight(w_ho)

                //# Δw = α * ∂Eⱼ/∂wᵢ
                self.output_layer.neurons[o].weights[w_ho] -= self.LEARNING_RATE * pd_error_wrt_weight

        # 4. Update hidden neuron weights
        for h in range(len(self.hidden_layer.neurons)):
            for w_ih in range(len(self.hidden_layer.neurons[h].weights)):

                //# ∂Eⱼ/∂wᵢ = ∂E/∂zⱼ * ∂zⱼ/∂wᵢ
                pd_error_wrt_weight = pd_errors_wrt_hidden_neuron_total_net_input[h] * self.hidden_layer.neurons[h].calculate_pd_total_net_input_wrt_weight(w_ih)

                # Δw = α * ∂Eⱼ/∂wᵢ
                self.hidden_layer.neurons[h].weights[w_ih] -= self.LEARNING_RATE * pd_error_wrt_weight

    def calculate_total_error(self, training_sets):
        total_error = 0
        for t in range(len(training_sets)):
            training_inputs, training_outputs = training_sets[t]
            self.feed_forward(training_inputs)
            for o in range(len(training_outputs)):
                total_error += self.output_layer.neurons[o].calculate_error(training_outputs[o])
        return total_error

class NeuronLayer:
    def __init__(self, num_neurons, bias):

        # Every neuron in a layer shares the same bias
        self.bias = bias if bias else random.random()

        self.neurons = []
        for i in range(num_neurons):
            self.neurons.append(Neuron(self.bias))

    def inspect(self):
        print('Neurons:', len(self.neurons))
        for n in range(len(self.neurons)):
            print(' Neuron', n)
            for w in range(len(self.neurons[n].weights)):
                print('  Weight:', self.neurons[n].weights[w])
            print('  Bias:', self.bias)

    def feed_forward(self, inputs):
        outputs = []
        for neuron in self.neurons:
            outputs.append(neuron.calculate_output(inputs))
        return outputs

    def get_outputs(self):
        outputs = []
        for neuron in self.neurons:
            outputs.append(neuron.output)
        return outputs

class Neuron:
    def __init__(self, bias):
        self.bias = bias
        self.weights = []

    def calculate_output(self, inputs):
        self.inputs = inputs
        self.output = self.squash(self.calculate_total_net_input())
        return self.output

    def calculate_total_net_input(self):
        total = 0
        for i in range(len(self.inputs)):
            total += self.inputs[i] * self.weights[i]
        return total + self.bias

    # Apply the logistic function to squash the output of the neuron
    # The result is sometimes referred to as 'net' [2] or 'net' [1]
    def squash(self, total_net_input):
        return 1 / (1 + math.exp(-total_net_input))

    # Determine how much the neuron's total input has to change to move closer to the expected output
    #
    # Now that we have the partial derivative of the error with respect to the output (∂E/∂yⱼ) and
    # the derivative of the output with respect to the total net input (dyⱼ/dzⱼ) we can calculate
    # the partial derivative of the error with respect to the total net input.
    # This value is also known as the delta (δ) [1]
    # δ = ∂E/∂zⱼ = ∂E/∂yⱼ * dyⱼ/dzⱼ
    #
    def calculate_pd_error_wrt_total_net_input(self, target_output):
        return self.calculate_pd_error_wrt_output(target_output) * self.calculate_pd_total_net_input_wrt_input();

    # The error for each neuron is calculated by the Mean Square Error method:
    def calculate_error(self, target_output):
        return 0.5 * (target_output - self.output) ** 2

    # The partial derivate of the error with respect to actual output then is calculated by:
    # = 2 * 0.5 * (target output - actual output) ^ (2 - 1) * -1
    # = -(target output - actual output)
    #
    # The Wikipedia article on backpropagation [1] simplifies to the following, but most other learning material does not [2]
    # = actual output - target output
    #
    # Alternative, you can use (target - output), but then need to add it during backpropagation [3]
    #
    # Note that the actual output of the output neuron is often written as yⱼ and target output as tⱼ so:
    # = ∂E/∂yⱼ = -(tⱼ - yⱼ)
    def calculate_pd_error_wrt_output(self, target_output):
        return -(target_output - self.output)

    # The total net input into the neuron is squashed using logistic function to calculate the neuron's output:
    # yⱼ = φ = 1 / (1 + e^(-zⱼ))
    # Note that where ⱼ represents the output of the neurons in whatever layer we're looking at and ᵢ represents the layer below it
    #
    # The derivative (not partial derivative since there is only one variable) of the output then is:
    # dyⱼ/dzⱼ = yⱼ * (1 - yⱼ)
    def calculate_pd_total_net_input_wrt_input(self):
        return self.output * (1 - self.output)

    # The total net input is the weighted sum of all the inputs to the neuron and their respective weights:
    # = zⱼ = netⱼ = x₁w₁ + x₂w₂ ...
    #
    # The partial derivative of the total net input with respective to a given weight (with everything else held constant) then is:
    # = ∂zⱼ/∂wᵢ = some constant + 1 * xᵢw₁^(1-0) + some constant ... = xᵢ
    def calculate_pd_total_net_input_wrt_weight(self, index):
        return self.inputs[index]

###

# Blog post example:

nn = NeuralNetwork(2, 2, 2, hidden_layer_weights=[0.15, 0.2, 0.25, 0.3], hidden_layer_bias=0.35, output_layer_weights=[0.4, 0.45, 0.5, 0.55], output_layer_bias=0.6)
for i in range(10000):
    nn.train([0.05, 0.1], [0.01, 0.99])
    print(i, round(nn.calculate_total_error([[[0.05, 0.1], [0.01, 0.99]]]), 9))

# XOR example:

# training_sets = [
#     [[0, 0], [0]],
#     [[0, 1], [1]],
#     [[1, 0], [1]],
#     [[1, 1], [0]]
# ]

# nn = NeuralNetwork(len(training_sets[0][0]), 5, len(training_sets[0][1]))
# for i in range(10000):
#     training_inputs, training_outputs = random.choice(training_sets)
#     nn.train(training_inputs, training_outputs)
#     print(i, nn.calculate_total_error(training_sets))
################################################################

/***************************************************************/
/* Globals                                                     */

FILE * g_outfile = NULL;
#endif

/***************************************************************/
/* #define NN_LEARNING_RATE_DEFAULT    0.5 */

struct dlist { /* dl_ */
    int dl_num_vals;
    ONN_FLOAT * dl_vals;
};
/***************************************************************/
struct dlist * dl_new(int num_vals, ONN_FLOAT * vals)
{
    struct dlist * dl;
    int ii;

    dl = New(struct dlist, 1);
    dl->dl_num_vals = num_vals;
    if (!num_vals) {
        dl->dl_vals = NULL;
    } else {
        dl->dl_vals = New(ONN_FLOAT, num_vals);
        for (ii = 0; ii < num_vals; ii++) dl->dl_vals[ii] = vals[ii];
    }

    return (dl);
}
/***************************************************************/
struct dlist * dl_new_count(int num_vals, ONN_FLOAT val)
{
    struct dlist * dl;
    int ii;

    dl = New(struct dlist, 1);
    dl->dl_num_vals = num_vals;
    if (!num_vals) {
        dl->dl_vals = NULL;
    } else {
        dl->dl_vals = New(ONN_FLOAT, num_vals);
        for (ii = 0; ii < num_vals; ii++) dl->dl_vals[ii] = val;
    }

    return (dl);
}
/***************************************************************/
void dl_free(struct dlist * dl)
{
    Free(dl->dl_vals);
    Free(dl);
}
/***************************************************************/
void dl_print(struct dlist * dl)
{
    int ii;
    printf("[");
    for (ii = 0; ii < dl->dl_num_vals; ii++) {
        if (ii) printf(",");
        printf("%f", dl->dl_vals[ii]);
    }
    printf("]\n");
}
/***************************************************************/
void dl_append(struct dlist * dl, ONN_FLOAT val)
{
    dl->dl_vals = Realloc(dl->dl_vals, ONN_FLOAT, dl->dl_num_vals + 1);
    dl->dl_vals[dl->dl_num_vals] = val;
    dl->dl_num_vals += 1;
}
/***************************************************************/
struct neuron * ne_new(void)
{
    struct neuron *  ne;

    ne = New(struct neuron, 1);

    ne->ne_num_weights  = 0;
    ne->ne_weights      = NULL;
    ne->ne_prev_delta   = NULL;
    ne->ne_output       = 0.0;
    ne->ne_num_inputs   = 0;
    ne->ne_inputs       = NULL;
    ne->ne_bias         = 0.0;

    return (ne);
}
/***************************************************************/
struct neuron * ne_new_init(ONN_FLOAT bias)
{
    struct neuron *  ne;

    ne = ne_new();
    ne->ne_bias         = bias;

    return (ne);
}
/***************************************************************/
void ne_free(struct neuron *  ne)
{
    if (!ne) return;

    Free(ne->ne_weights);
    Free(ne->ne_prev_delta);
    Free(ne->ne_inputs);
    Free(ne);
}
/***************************************************************/
void ne_copy(struct neuron *  tgt_ne, struct neuron * src_ne)
{
    int ii;

    tgt_ne->ne_num_weights = src_ne->ne_num_weights;

    if (!src_ne->ne_num_inputs) {
        tgt_ne->ne_inputs     = NULL;
    } else {
        tgt_ne->ne_inputs = New(ONN_FLOAT, src_ne->ne_num_inputs);
        for (ii = 0; ii < src_ne->ne_num_inputs; ii++) {
            tgt_ne->ne_inputs[ii] = src_ne->ne_inputs[ii];
        }
    }

    if (!src_ne->ne_num_weights) {
        tgt_ne->ne_weights     = NULL;
        tgt_ne->ne_prev_delta  = NULL;
    } else {
        tgt_ne->ne_weights = New(ONN_FLOAT, src_ne->ne_num_weights);
        tgt_ne->ne_prev_delta = New(ONN_FLOAT, src_ne->ne_num_weights);
        for (ii = 0; ii < src_ne->ne_num_weights; ii++) {
            tgt_ne->ne_weights[ii] = src_ne->ne_weights[ii];
            tgt_ne->ne_prev_delta[ii] = src_ne->ne_prev_delta[ii];
        }
    }
}
/***************************************************************/
void ne_append_weight(struct neuron *  ne, ONN_FLOAT weight)
{
    ne->ne_weights = Realloc(ne->ne_weights, ONN_FLOAT, ne->ne_num_weights + 1);
    ne->ne_weights[ne->ne_num_weights] = weight;
    ne->ne_prev_delta = Realloc(ne->ne_prev_delta, ONN_FLOAT, ne->ne_num_weights + 1);
    ne->ne_prev_delta[ne->ne_num_weights] = 0.0;
    ne->ne_num_weights += 1;
}
/***************************************************************/
void nl_inspect(struct neural_layer *  nl)
{
    int nn;
    int ww;

    printf("Neurons: %d\n", nl->nl_num_neurons);
    for (nn = 0; nn < nl->nl_num_neurons; nn++) {
        printf(" Neuron: %d: output=%f, bias=%f\n", nn,
            nl->nl_neurons[nn]->ne_output, nl->nl_neurons[nn]->ne_bias);
        for (ww = 0; ww < nl->nl_neurons[nn]->ne_num_weights; ww++) {
            printf("  Weight: %f\n", nl->nl_neurons[nn]->ne_weights[ww]);
        }
        for (ww = 0; ww < nl->nl_neurons[nn]->ne_num_inputs; ww++) {
            printf("  Input: %f\n", nl->nl_neurons[nn]->ne_inputs[ww]);
        }
        printf("  Bias: %f\n", nl->nl_bias);
    }
}
/***************************************************************/
void nl_init(struct neural_layer *  nl,
    int num_neurons,
    ONN_FLOAT bias)
{
    int ii;

    nl->nl_num_neurons  = num_neurons;
    nl->nl_bias         = bias;
    nl->nl_neurons      = New(struct neuron *, num_neurons);

    for (ii = 0; ii < num_neurons; ii++) {
        nl->nl_neurons[ii] = ne_new_init(bias);
    }
}
/***************************************************************/
struct neural_layer * nl_new(void)
{
    struct neural_layer *  nl;

    nl = New(struct neural_layer, 1);

    return (nl);
}
/***************************************************************/
struct neural_layer * nl_new_init(
    int num_neurons,
    ONN_FLOAT bias)
{
    struct neural_layer *  nl;

    nl = nl_new();

    nl_init(nl, num_neurons, bias);

    return (nl);
}
/***************************************************************/
void nl_free(struct neural_layer *  nl)
{
    int ii;

    if (!nl) return;

    if (nl->nl_neurons) {
        for (ii = 0; ii < nl->nl_num_neurons; ii++) {
            ne_free(nl->nl_neurons[ii]);
        }
        Free(nl->nl_neurons);
    }
    Free(nl);
}
/***************************************************************/
struct neural_layer * nl_new_copy(struct neural_layer *  nl)
{
    struct neural_layer *  n_nl;
    int ii;

    if (!nl) return (NULL);

    n_nl = nl_new_init(nl->nl_num_neurons, nl->nl_bias);

    for (ii = 0; ii < nl->nl_num_neurons; ii++) {
        ne_copy(n_nl->nl_neurons[ii], n_nl->nl_neurons[ii]);
    }

    return (n_nl);
}
/***************************************************************/
struct neural_network * nn_dup(struct neural_network * nn)
{
/*
struct neural_network {
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
--
*/
    struct neural_network *  n_nn;
    int ii;

    n_nn = nn_new();
    n_nn->nn_info = Strdup("nn_dup");
    //if (nn->nn_info) n_nn->nn_info = Strdup(nn->nn_info);

    n_nn->nn_total_error      = nn->nn_total_error;
    n_nn->nn_learning_rate    = nn->nn_learning_rate;
    n_nn->nn_momentum         = nn->nn_momentum;
    n_nn->nn_num_epochs       = nn->nn_num_epochs;
    n_nn->nn_num_inputs       = nn->nn_num_inputs;
    n_nn->nn_num_outputs      = nn->nn_num_outputs;

    n_nn->nn_elapsed_time     = NULL;
    n_nn->nn_inputs           = NULL;
    n_nn->nn_hidden_layer     = nl_new_copy(nn->nn_hidden_layer);
    n_nn->nn_output_layer     = nl_new_copy(nn->nn_output_layer);

    if (nn->nn_elapsed_time) n_nn->nn_info = Strdup(nn->nn_elapsed_time);

    if (nn->nn_num_inputs) {
        n_nn->nn_inputs  = New(ONN_FLOAT, nn->nn_num_inputs);
        for (ii = 0; ii < nn->nn_num_inputs; ii++) {
            n_nn->nn_inputs[ii] = nn->nn_inputs[ii];
        }
    }

    return (n_nn);
}
/***************************************************************/
ONN_FLOAT ne_squash(ONN_FLOAT total_net_input)
{
/*
    # Apply the logistic function to squash the output of the neuron
    # The result is sometimes referred to as 'net' [2] or 'net' [1]
    def squash(self, total_net_input):
        return 1 / (1 + math.exp(-total_net_input))
*/
    ONN_FLOAT ret;

    ret = 1.0 / (1.0 + exp(-total_net_input));
    /* printf("ne_squash(%8.4f)=%16.12f\n", total_net_input, ret); */

    return (ret);
}
/***************************************************************/
ONN_FLOAT ne_calculate_pd_error_wrt_output(struct neuron *  ne, ONN_FLOAT target_output)
{
/*
    # The partial derivate of the error with respect to actual output then is calculated by:
    # = 2 * 0.5 * (target output - actual output) ^ (2 - 1) * -1
    # = -(target output - actual output)
    #
    # The Wikipedia article on backpropagation [1] simplifies to the following, but most other learning material does not [2]
    # = actual output - target output
    #
    # Alternative, you can use (target - output), but then need to add it during backpropagation [3]
    #
    # Note that the actual output of the output neuron is often written as yⱼ and target output as tⱼ so:
    # = ∂E/∂yⱼ = -(tⱼ - yⱼ)
    def calculate_pd_error_wrt_output(self, target_output):
        return -(target_output - self.output)
*/
    ONN_FLOAT pd_err;

    pd_err = -(target_output - ne->ne_output);

    return (pd_err);
}
/***************************************************************/
ONN_FLOAT ne_calculate_pd_total_net_input_wrt_weight(struct neuron *  ne, int index)
{
/*
    # The total net input is the weighted sum of all the inputs to the neuron and their respective weights:
    # = zⱼ = netⱼ = x₁w₁ + x₂w₂ ...
    #
    # The partial derivative of the total net input with respective to a given weight (with everything else held constant) then is:
    # = ∂zⱼ/∂wᵢ = some constant + 1 * xᵢw₁^(1-0) + some constant ... = xᵢ
    def calculate_pd_total_net_input_wrt_weight(self, index):
        return self.inputs[index]
*/
    return (ne->ne_inputs[index]);
}
/***************************************************************/
ONN_FLOAT ne_calculate_pd_total_net_input_wrt_input(struct neuron *  ne)
{
/*
    # The total net input into the neuron is squashed using logistic function to calculate the neuron's output:
    # yⱼ = φ = 1 / (1 + e^(-zⱼ))
    # Note that where ⱼ represents the output of the neurons in whatever layer we're looking at and ᵢ represents the layer below it
    #
    # The derivative (not partial derivative since there is only one variable) of the output then is:
    # dyⱼ/dzⱼ = yⱼ * (1 - yⱼ)
    def calculate_pd_total_net_input_wrt_input(self):
        return self.output * (1 - self.output)
*/
    ONN_FLOAT pd_tot;

    pd_tot = ne->ne_output * (1.0 - ne->ne_output);

    return (pd_tot);
}
/***************************************************************/
ONN_FLOAT ne_calculate_pd_error_wrt_total_net_input(struct neuron *  ne, ONN_FLOAT target_output)
{
/*
    # Determine how much the neuron's total input has to change to move closer to the expected output
    #
    # Now that we have the partial derivative of the error with respect to the output (∂E/∂yⱼ) and
    # the derivative of the output with respect to the total net input (dyⱼ/dzⱼ) we can calculate
    # the partial derivative of the error with respect to the total net input.
    # This value is also known as the delta (δ) [1]
    # δ = ∂E/∂zⱼ = ∂E/∂yⱼ * dyⱼ/dzⱼ
    #
    def calculate_pd_error_wrt_total_net_input(self, target_output):
        return self.calculate_pd_error_wrt_output(target_output) * self.calculate_pd_total_net_input_wrt_input();
*/
    ONN_FLOAT pd_err;
    ONN_FLOAT pd_tot;

    pd_err = ne_calculate_pd_error_wrt_output(ne, target_output);
    pd_tot = ne_calculate_pd_total_net_input_wrt_input(ne);

    return (pd_err * pd_tot);
}
/***************************************************************/
ONN_FLOAT ne_calculate_total_net_input(struct neuron *  ne)
{
/*
    def calculate_total_net_input(self):
        total = 0
        for i in range(len(self.inputs)):
            total += self.inputs[i] * self.weights[i]
        return total + self.bias
*/
    ONN_FLOAT total;
    int ii;

    total = 0.0;
    for (ii = 0; ii < ne->ne_num_inputs; ii++) {
        total += ne->ne_inputs[ii] * ne->ne_weights[ii];
    }
    total += ne->ne_bias;

    return (total);
}
/***************************************************************/
void ne_copy_inputs(struct neuron *  ne,
    int num_inputs,
    ONN_FLOAT * inputs)
{
    int ii;

    if (ne->ne_num_inputs != num_inputs) {
        if (!num_inputs) {
            Free(ne->ne_inputs);
            ne->ne_inputs     = NULL;
        } else {
            ne->ne_inputs = Realloc(ne->ne_inputs, ONN_FLOAT, num_inputs);
        }
        ne->ne_num_inputs = num_inputs;
    }

    for (ii = 0; ii < num_inputs; ii++) {
        ne->ne_inputs[ii] = inputs[ii];
    }
}
/***************************************************************/
ONN_FLOAT ne_calculate_output(struct neuron *  ne,
    int num_inputs,
    ONN_FLOAT * inputs)
{
/*
    def calculate_output(self, inputs):
        self.inputs = inputs
        self.output = self.squash(self.calculate_total_net_input())
        return self.output
*/
    ONN_FLOAT tot_net;

    ne_copy_inputs(ne, num_inputs, inputs);

    tot_net = ne_calculate_total_net_input(ne);

    ne->ne_output = ne_squash(tot_net);

    return (ne->ne_output);
}
/***************************************************************/
struct dlist * nl_feed_forward(struct neural_layer *  nl,
    int num_inputs,
    ONN_FLOAT * inputs)
{
/*
    def feed_forward(self, inputs):
        outputs = []
        for neuron in self.neurons:
            outputs.append(neuron.calculate_output(inputs))
        return outputs
*/
    struct dlist * dl;
    struct neuron *  ne;
    int ii;
    ONN_FLOAT out;

    dl = dl_new(0, NULL);
    for (ii = 0; ii < nl->nl_num_neurons; ii++) {
        ne = nl->nl_neurons[ii];
        out = ne_calculate_output(ne, num_inputs, inputs);
        dl_append(dl, out);
    }

    return (dl);
}
/***************************************************************/
struct dlist * nn_feed_forward(struct neural_network *  nn,
    int num_inputs,
    ONN_FLOAT * inputs)
{
/*
    def feed_forward(self, inputs):
        hidden_layer_outputs = self.hidden_layer.feed_forward(inputs)
        return self.output_layer.feed_forward(hidden_layer_outputs)
*/
    struct dlist * h_dl;
    struct dlist * o_dl;

    h_dl = nl_feed_forward(nn->nn_hidden_layer, num_inputs, inputs);
    o_dl = nl_feed_forward(nn->nn_output_layer, h_dl->dl_num_vals, h_dl->dl_vals);
    dl_free(h_dl);

    return (o_dl);
}
/***************************************************************/
void nn_init_weights_from_inputs_to_hidden_layer_neurons(struct neural_network *  nn,
    int num_hidden_layer_weights,
    ONN_FLOAT * hidden_layer_weights)
{
/*
    def init_weights_from_inputs_to_hidden_layer_neurons(self, hidden_layer_weights):
        weight_num = 0
        for h in range(len(self.hidden_layer.neurons)):
            for i in range(self.num_inputs):
                if not hidden_layer_weights:
                    self.hidden_layer.neurons[h].weights.append(random.random())
                else:
                    self.hidden_layer.neurons[h].weights.append(hidden_layer_weights[weight_num])
                weight_num += 1
*/
    int weight_num;
    int hh;
    int ii;

    weight_num = 0;
    for (hh = 0; hh < nn->nn_hidden_layer->nl_num_neurons; hh++) {
        for (ii = 0; ii < nn->nn_num_inputs; ii++) {
            ne_append_weight(nn->nn_hidden_layer->nl_neurons[hh], hidden_layer_weights[weight_num]);
            weight_num += 1;
        }
    }
}
/***************************************************************/
void nn_init_weights_from_hidden_layer_neurons_to_output_layer_neurons(struct neural_network *  nn,
    int num_output_layer_weights,
    ONN_FLOAT * output_layer_weights)
{
/*
    def init_weights_from_hidden_layer_neurons_to_output_layer_neurons(self, output_layer_weights):
    weight_num = 0
    for o in range(len(self.output_layer.neurons)):
        for h in range(len(self.hidden_layer.neurons)):
            if not output_layer_weights:
                self.output_layer.neurons[o].weights.append(random.random())
            else:
                self.output_layer.neurons[o].weights.append(output_layer_weights[weight_num])
            weight_num += 1
*/
    int weight_num;
    int oo;
    int hh;

    weight_num = 0;
    for (oo = 0; oo < nn->nn_output_layer->nl_num_neurons; oo++) {
        for (hh = 0; hh < nn->nn_hidden_layer->nl_num_neurons; hh++) {
            ne_append_weight(nn->nn_output_layer->nl_neurons[oo], output_layer_weights[weight_num]);
            weight_num += 1;
        }
    }
}
/***************************************************************/
void nn_inspect(struct neural_network *  nn)
{
    int ii;

    printf("------\n");
    printf("* Inputs: %d ", nn->nn_num_inputs);
    for (ii = 0; ii < nn->nn_num_inputs; ii++) printf("%f ", nn->nn_inputs[ii]);
    printf("------\n");
    printf("Hidden Layer\n");
    nl_inspect(nn->nn_hidden_layer);
    printf("------\n");
    printf("* Output Layer\n");
    nl_inspect(nn->nn_output_layer);
    printf("------\n");
}
/***************************************************************/
void nn_init(struct neural_network *  nn,
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
    ONN_FLOAT momentum)
{
/*
    def __init__(self, num_inputs, num_hidden, num_outputs, hidden_layer_weights = None, hidden_layer_bias = None, output_layer_weights = None, output_layer_bias = None):
        self.num_inputs = num_inputs

        self.hidden_layer = NeuronLayer(num_hidden, hidden_layer_bias)
        self.output_layer = NeuronLayer(num_outputs, output_layer_bias)

        self.init_weights_from_inputs_to_hidden_layer_neurons(hidden_layer_weights)
        self.init_weights_from_hidden_layer_neurons_to_output_layer_neurons(output_layer_weights)
*/
    nn->nn_learning_rate = learning_rate;
    nn->nn_momentum      = momentum;
    nn->nn_num_inputs    = num_inputs;
    nn->nn_num_outputs   = num_outputs;
    nn->nn_inputs        = New(ONN_FLOAT, num_inputs);
    nn->nn_hidden_layer  = nl_new_init(num_hidden, hidden_layer_bias);
    nn->nn_output_layer  = nl_new_init(num_outputs, output_layer_bias);

    nn_init_weights_from_inputs_to_hidden_layer_neurons(nn, num_hidden_layer_weights, hidden_layer_weights);
    nn_init_weights_from_hidden_layer_neurons_to_output_layer_neurons(nn, num_output_layer_weights, output_layer_weights);
}
/***************************************************************/
void nnt_randomize_layer_weights(void * rvp,
    int num_layer_weights,
    ONN_FLOAT * layer_weights)
{
    int ii;

    for (ii = 0; ii < num_layer_weights; ii++) {
        layer_weights[ii] = (randfltnext(rvp) * 2.0) - 1.0;
    }
}
/***************************************************************/
void nn_randomize_weights(
    int num_hidden_layer_weights,
    ONN_FLOAT * hidden_layer_weights,
    int num_output_layer_weights,
    ONN_FLOAT * output_layer_weights)
{
    void * rvp;

    rvp = randfltseed(RANDNUM_GENFLT_5, 100);
    nnt_randomize_layer_weights(rvp, num_hidden_layer_weights, hidden_layer_weights);
    nnt_randomize_layer_weights(rvp, num_output_layer_weights, output_layer_weights);
    randfltfree(rvp);
}
/***************************************************************/
struct neural_network * nn_new_default(
    int num_inputs,
    int num_hidden,
    int num_outputs)
{
    struct neural_network *  nn;
    int num_hidden_layer_weights;
    int num_output_layer_weights;
    ONN_FLOAT * hidden_layer_weights;
    ONN_FLOAT hidden_layer_bias;
    ONN_FLOAT * output_layer_weights;
    ONN_FLOAT output_layer_bias;
    ONN_FLOAT learning_rate;
    ONN_FLOAT momentum;

    nn = nn_new();
    nn->nn_info = Strdup("NNInit");

    num_hidden_layer_weights = num_inputs  * num_hidden;
    num_output_layer_weights = num_outputs * num_hidden;

    hidden_layer_weights    = New(ONN_FLOAT, num_hidden_layer_weights);
    hidden_layer_bias       = 0.1;
    output_layer_weights    = New(ONN_FLOAT, num_output_layer_weights);
    output_layer_bias       = 0.1;
    learning_rate           = 0.5;
    momentum                = 0.0;

    nn_randomize_weights(
        num_hidden_layer_weights    , hidden_layer_weights  ,
        num_output_layer_weights    , output_layer_weights  );

    nn_init(nn,
        num_inputs                  , num_hidden            , num_outputs,
        num_hidden_layer_weights    , hidden_layer_weights  , hidden_layer_bias ,
        num_output_layer_weights    , output_layer_weights  , output_layer_bias ,
        learning_rate               , momentum);

    Free(hidden_layer_weights);
    Free(output_layer_weights);

    return (nn);
}
/***************************************************************/
void nn_train(struct neural_network *  nn,
    int num_training_inputs,
    ONN_FLOAT * training_inputs,
    int num_training_outputs,
    ONN_FLOAT * training_outputs)
{
/*
    def train(self, training_inputs, training_outputs):
        self.feed_forward(training_inputs)

        # 1. Output neuron deltas
        pd_errors_wrt_output_neuron_total_net_input = [0] * len(self.output_layer.neurons)
        for o in range(len(self.output_layer.neurons)):

            # ∂E/∂zⱼ
            pd_errors_wrt_output_neuron_total_net_input[o] = self.output_layer.neurons[o].calculate_pd_error_wrt_total_net_input(training_outputs[o])

        # 2. Hidden neuron deltas
        pd_errors_wrt_hidden_neuron_total_net_input = [0] * len(self.hidden_layer.neurons)
        for h in range(len(self.hidden_layer.neurons)):

            # We need to calculate the derivative of the error with respect to the output of each hidden layer neuron
            # dE/dyⱼ = Σ ∂E/∂zⱼ * ∂z/∂yⱼ = Σ ∂E/∂zⱼ * wᵢⱼ
            d_error_wrt_hidden_neuron_output = 0
            for o in range(len(self.output_layer.neurons)):
                d_error_wrt_hidden_neuron_output += pd_errors_wrt_output_neuron_total_net_input[o] * self.output_layer.neurons[o].weights[h]

            # ∂E/∂zⱼ = dE/dyⱼ * ∂zⱼ/∂
            pd_errors_wrt_hidden_neuron_total_net_input[h] = d_error_wrt_hidden_neuron_output * self.hidden_layer.neurons[h].calculate_pd_total_net_input_wrt_input()

        # 3. Update output neuron weights
        for o in range(len(self.output_layer.neurons)):
            for w_ho in range(len(self.output_layer.neurons[o].weights)):

                # ∂Eⱼ/∂wᵢⱼ = ∂E/∂zⱼ * ∂zⱼ/∂wᵢⱼ
                pd_error_wrt_weight = pd_errors_wrt_output_neuron_total_net_input[o] * self.output_layer.neurons[o].calculate_pd_total_net_input_wrt_weight(w_ho)

                # Δw = α * ∂Eⱼ/∂wᵢ
                self.output_layer.neurons[o].weights[w_ho] -= self.LEARNING_RATE * pd_error_wrt_weight

        # 4. Update hidden neuron weights
        for h in range(len(self.hidden_layer.neurons)):
            for w_ih in range(len(self.hidden_layer.neurons[h].weights)):

                # ∂Eⱼ/∂wᵢ = ∂E/∂zⱼ * ∂zⱼ/∂wᵢ
                pd_error_wrt_weight = pd_errors_wrt_hidden_neuron_total_net_input[h] * self.hidden_layer.neurons[h].calculate_pd_total_net_input_wrt_weight(w_ih)

                # Δw = α * ∂Eⱼ/∂wᵢ
                self.hidden_layer.neurons[h].weights[w_ih] -= self.LEARNING_RATE * pd_error_wrt_weight
*/
/*
    def train(self, training_inputs, training_outputs):
        self.feed_forward(training_inputs)
*/
    int hh;
    int oo;
    int w_ho;
    int w_ih;
    struct dlist * o_dl;
    struct neuron *  h_ne;
    struct neuron *  o_ne;
    struct dlist * pd_errors_wrt_output_neuron_total_net_input;
    struct dlist * pd_errors_wrt_hidden_neuron_total_net_input;
    ONN_FLOAT d_error_wrt_hidden_neuron_output;
    ONN_FLOAT pd_error_wrt_weight;
    ONN_FLOAT delta;
/*
    def train(self, training_inputs, training_outputs):
        self.feed_forward(training_inputs)
*/
    o_dl = nn_feed_forward(nn, num_training_inputs, training_inputs);
    dl_free(o_dl);
/*
        # 1. Output neuron deltas
        pd_errors_wrt_output_neuron_total_net_input = [0] * len(self.output_layer.neurons)
        for o in range(len(self.output_layer.neurons)):

            # ∂E/∂zⱼ
            pd_errors_wrt_output_neuron_total_net_input[o] = self.output_layer.neurons[o].calculate_pd_error_wrt_total_net_input(training_outputs[o])
*/
    pd_errors_wrt_output_neuron_total_net_input = dl_new_count(nn->nn_output_layer->nl_num_neurons, 0.0);
    for (oo = 0; oo < nn->nn_output_layer->nl_num_neurons; oo++) {
        o_ne = nn->nn_output_layer->nl_neurons[oo];
        pd_errors_wrt_output_neuron_total_net_input->dl_vals[oo] =
            ne_calculate_pd_error_wrt_total_net_input(o_ne, training_outputs[oo]);
    }
/*
        # 2. Hidden neuron deltas
        pd_errors_wrt_hidden_neuron_total_net_input = [0] * len(self.hidden_layer.neurons)
        for h in range(len(self.hidden_layer.neurons)):

            # We need to calculate the derivative of the error with respect to the output of each hidden layer neuron
            # dE/dyⱼ = Σ ∂E/∂zⱼ * ∂z/∂yⱼ = Σ ∂E/∂zⱼ * wᵢⱼ
            d_error_wrt_hidden_neuron_output = 0
            for o in range(len(self.output_layer.neurons)):
                d_error_wrt_hidden_neuron_output += pd_errors_wrt_output_neuron_total_net_input[o] * self.output_layer.neurons[o].weights[h]

            # ∂E/∂zⱼ = dE/dyⱼ * ∂zⱼ/∂
            pd_errors_wrt_hidden_neuron_total_net_input[h] = d_error_wrt_hidden_neuron_output * self.hidden_layer.neurons[h].calculate_pd_total_net_input_wrt_input()
*/
    pd_errors_wrt_hidden_neuron_total_net_input = dl_new_count(nn->nn_hidden_layer->nl_num_neurons, 0.0);
    for (hh = 0; hh < nn->nn_hidden_layer->nl_num_neurons; hh++) {
        h_ne = nn->nn_hidden_layer->nl_neurons[hh];
        d_error_wrt_hidden_neuron_output = 0.0;
        for (oo = 0; oo < nn->nn_output_layer->nl_num_neurons; oo++) {
            o_ne = nn->nn_output_layer->nl_neurons[oo];
            d_error_wrt_hidden_neuron_output +=
                pd_errors_wrt_output_neuron_total_net_input->dl_vals[oo] *
                o_ne->ne_weights[hh];
        }
        pd_errors_wrt_hidden_neuron_total_net_input->dl_vals[hh] =
            d_error_wrt_hidden_neuron_output *
            ne_calculate_pd_total_net_input_wrt_input(h_ne);
    }

/*
        # 3. Update output neuron weights
        for o in range(len(self.output_layer.neurons)):
            for w_ho in range(len(self.output_layer.neurons[o].weights)):

                # ∂Eⱼ/∂wᵢⱼ = ∂E/∂zⱼ * ∂zⱼ/∂wᵢⱼ
                pd_error_wrt_weight = pd_errors_wrt_output_neuron_total_net_input[o] * self.output_layer.neurons[o].calculate_pd_total_net_input_wrt_weight(w_ho)

                # Δw = α * ∂Eⱼ/∂wᵢ
                self.output_layer.neurons[o].weights[w_ho] -= self.LEARNING_RATE * pd_error_wrt_weight

        ** Java **
            double deltaWeight = -learningRate * partialDerivative;
            double newWeight = con.getWeight() + deltaWeight;
            con.setDeltaWeight(deltaWeight);
            con.setWeight(newWeight + momentum * con.getPrevDeltaWeight());

*/
    for (oo = 0; oo < nn->nn_output_layer->nl_num_neurons; oo++) {
        o_ne = nn->nn_output_layer->nl_neurons[oo];
        for (w_ho = 0; w_ho < o_ne->ne_num_weights; w_ho++) {
            pd_error_wrt_weight = 
                pd_errors_wrt_output_neuron_total_net_input->dl_vals[oo] *
                ne_calculate_pd_total_net_input_wrt_weight(o_ne, w_ho);
            delta = nn->nn_learning_rate * pd_error_wrt_weight;
            o_ne->ne_weights[w_ho] -=
                (delta + o_ne->ne_prev_delta[w_ho] * nn->nn_momentum);
            o_ne->ne_prev_delta[w_ho] = delta;
        }
    }
    dl_free(pd_errors_wrt_output_neuron_total_net_input);

/*
        # 4. Update hidden neuron weights
        for h in range(len(self.hidden_layer.neurons)):
            for w_ih in range(len(self.hidden_layer.neurons[h].weights)):

                # ∂Eⱼ/∂wᵢ = ∂E/∂zⱼ * ∂zⱼ/∂wᵢ
                pd_error_wrt_weight = pd_errors_wrt_hidden_neuron_total_net_input[h] * self.hidden_layer.neurons[h].calculate_pd_total_net_input_wrt_weight(w_ih)

                # Δw = α * ∂Eⱼ/∂wᵢ
                self.hidden_layer.neurons[h].weights[w_ih] -= self.LEARNING_RATE * pd_error_wrt_weight
*/
    for (hh = 0; hh < nn->nn_hidden_layer->nl_num_neurons; hh++) {
        h_ne = nn->nn_hidden_layer->nl_neurons[hh];
        for (w_ih = 0; w_ih < h_ne->ne_num_weights; w_ih++) {
            pd_error_wrt_weight = 
                pd_errors_wrt_hidden_neuron_total_net_input->dl_vals[hh] *
                ne_calculate_pd_total_net_input_wrt_weight(h_ne, w_ih);
            delta = nn->nn_learning_rate * pd_error_wrt_weight;
            h_ne->ne_weights[w_ih] -= 
                (delta + h_ne->ne_prev_delta[w_ih] * nn->nn_momentum);
            h_ne->ne_prev_delta[w_ih] = delta;
        }
    }
    dl_free(pd_errors_wrt_hidden_neuron_total_net_input);
}
/***************************************************************/
ONN_FLOAT ne_calculate_error(struct neuron *  ne, ONN_FLOAT target_output)
{
/*
    # The error for each neuron is calculated by the Mean Square Error method:
    def calculate_error(self, target_output):
        return 0.5 * (target_output - self.output) ** 2
*/
    ONN_FLOAT err;
    ONN_FLOAT diff;

    diff = target_output - ne->ne_output;
    err = 0.5 * diff * diff;

    return (err);
}
/***************************************************************/
ONN_FLOAT nn_calculate_total_error(struct neural_network *  nn,
    int num_training_inputs,
    ONN_FLOAT * training_inputs,
    int num_training_outputs,
    ONN_FLOAT * training_outputs)
{
/*
    def calculate_total_error(self, training_sets):
        total_error = 0
        for t in range(len(training_sets)):
            training_inputs, training_outputs = training_sets[t]
            self.feed_forward(training_inputs)
            for o in range(len(training_outputs)):
                total_error += self.output_layer.neurons[o].calculate_error(training_outputs[o])
        return total_error
*/
    ONN_FLOAT total_error;
    int oo;
    struct neuron *  o_ne;
    struct dlist * o_dl;

    total_error = 0.0;
    o_dl = nn_feed_forward(nn, num_training_inputs, training_inputs);
    dl_free(o_dl);

    for (oo = 0; oo < nn->nn_output_layer->nl_num_neurons; oo++) {
        o_ne = nn->nn_output_layer->nl_neurons[oo];
        total_error += ne_calculate_error(o_ne, training_outputs[oo]);
    }

    return (total_error);
}
/***************************************************************/
int nn_forward(struct neural_network *  nn,
    int num_training_inputs,
    ONN_FLOAT * training_inputs,
    int num_outputs,
    ONN_FLOAT * outputs)
{
/*
*/
    struct dlist * o_dl;
    int ii;
    int nout;

    o_dl = nn_feed_forward(nn, num_training_inputs, training_inputs);
    nout = o_dl->dl_num_vals;
    if (nout > num_outputs) nout = num_outputs;

    for (ii = 0; ii < nout; ii++) outputs[ii] = o_dl->dl_vals[ii];
    dl_free(o_dl);

    return (nout);
}
/***************************************************************/
struct neural_network * nn_new(void)
{
    struct neural_network *  nn;

    nn = New(struct neural_network, 1);
    nn->nn_total_error      = 0.0;
    nn->nn_learning_rate    = 0.0;
    nn->nn_momentum         = 0.0;
    nn->nn_num_epochs       = 0;
    nn->nn_num_inputs       = 0;
    nn->nn_num_outputs      = 0;

    nn->nn_info             = NULL;
    nn->nn_elapsed_time     = NULL;
    nn->nn_inputs           = NULL;
    nn->nn_hidden_layer     = NULL;
    nn->nn_output_layer     = NULL;

    return (nn);
}
/***************************************************************/
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
    ONN_FLOAT momentum)
{
    struct neural_network *  nn;

    nn = nn_new();

    nn_init(nn,
        num_inputs                  , num_hidden            , num_outputs,
        num_hidden_layer_weights    , hidden_layer_weights  , hidden_layer_bias ,
        num_output_layer_weights    , output_layer_weights  , output_layer_bias ,
        learning_rate               , momentum);

    return (nn);
}
/***************************************************************/
void nn_free(struct neural_network *  nn)
{
    if (!nn) return;

    nl_free(nn->nn_hidden_layer);
    nl_free(nn->nn_output_layer);

    Free(nn->nn_inputs);
    Free(nn->nn_info);
    Free(nn->nn_elapsed_time);
    Free(nn);
}
/***************************************************************/
void  nn_set_elapsed_time(struct neural_network *  nn,
        const char * elap_tim,
        const char * time_fmt)
{
    char * new_time;

    new_time = add_string_timestamps(elap_tim, nn->nn_elapsed_time, time_fmt);
    if (new_time) {
        if (nn->nn_elapsed_time) Free(nn->nn_elapsed_time);

        nn->nn_elapsed_time = new_time;
    }
}
/***************************************************************/
void nl_set_bias(struct neural_layer *  nl, ONN_FLOAT new_bias)
{
    int nn;

    for (nn = 0; nn < nl->nl_num_neurons; nn++) {
        nl->nl_neurons[nn]->ne_bias = new_bias;
    }
    nl->nl_bias = new_bias;
}
/***************************************************************/
int nn_set_config(struct neural_network * nn, int cfgcmd, void * cfgparm)
{
    double dblval;
    int nnerr = 0;

    switch (cfgcmd) {
        case NNCFG_HIDDEN_LAYER_BIAS:
            dblval = *((double*)cfgparm);
            nl_set_bias(nn->nn_hidden_layer, dblval);
            break;
        case NNCFG_OUTPUT_LAYER_BIAS:
            dblval = *((double*)cfgparm);
            nl_set_bias(nn->nn_output_layer, dblval);
            break;
        case NNCFG_LEARNING_RATE:
            dblval = *((double*)cfgparm);
            nn->nn_learning_rate = dblval;
            break;
        case NNCFG_MOMENTUM:
            dblval = *((double*)cfgparm);
            nn->nn_momentum = dblval;
            break;
        default:
           nnerr = 1101;
           break;
    }

    return (nnerr);
}
/***************************************************************/
#ifdef DENN
/***************************************************************/
void nn_main(void)
{
/*
nn = NeuralNetwork(2, 2, 2, hidden_layer_weights=[0.15, 0.2, 0.25, 0.3], hidden_layer_bias=0.35, output_layer_weights=[0.4, 0.45, 0.5, 0.55], output_layer_bias=0.6)
for i in range(10000):
    nn.train([0.05, 0.1], [0.01, 0.99])
    print(i, round(nn.calculate_total_error([[[0.05, 0.1], [0.01, 0.99]]]), 9))
*/
    struct neural_network *  nn;
    ONN_FLOAT hidden_layer_weights[] = {0.15, 0.2, 0.25, 0.3};
    ONN_FLOAT output_layer_weights[] = {0.4, 0.45, 0.5, 0.55};
    ONN_FLOAT inputs[] = {0.05, 0.1};
    ONN_FLOAT outputs[] = {0.01, 0.99};
    ONN_FLOAT total_error;
    int ii;

    nn = nn_new_init(2, 2, 2, 4, hidden_layer_weights, 0.35, 4, output_layer_weights, 0.6);

    for (ii = 0; ii < 4; ii++) {
        nn_train(nn, 2, inputs, 2, outputs);
        total_error = nn_calculate_total_error(nn, 2, inputs, 2, outputs);
        printf("%d %6.4f\n", ii, total_error);
    }

    nn_free(nn);
}
/***************************************************************/
/***************************************************************/
void showusage(char * prognam) {

    printf("%s - v%s\n", PROGRAM_NAME, VERSION);
    printf("\n");
    printf("Usage: %s [options] <config file name>\n", prognam);
    printf("\n");
    printf("Options:\n");
    printf("  -pause            -p      Pause when done.\n");
    printf("  -version                  Show program version.\n");
    printf("\n");
}
/***************************************************************/
int set_error (char * fmt, ...) {

    va_list args;

    va_start (args, fmt);
    vfprintf(stderr, fmt, args);
    va_end (args);
    fprintf(stderr, "\n");

    return (-1);
}
/***************************************************************/
FILE * get_outfile(void) {

    return (g_outfile);
}
/***************************************************************/
void set_outfile(FILE * outfile) {

    g_outfile    = outfile;
}
/***************************************************************/
void printout (char * fmt, ...) {

    va_list args;
    FILE * outf;

    outf = get_outfile();
    if (!outf) outf = stdout;

    va_start (args, fmt);
    vfprintf(outf, fmt, args);
    va_end (args);
}
/***************************************************************/
struct arginfo * init(void) {

    struct arginfo * args;

    args = New(struct arginfo, 1);
    args->arg_flags = 0;

    return (args);
}
/***************************************************************/
void shut(struct arginfo * args) {

    Free(args);
}
/***************************************************************/
int process(struct arginfo * args) {

    int estat = 0;

    nn_main();

    return (estat);
}
/***************************************************************/
int getargs(int argc, char *argv[], struct arginfo * args) {

    int ii;
    int estat = 0;

    for (ii = 1; !estat && ii < argc; ii++) {
        if (argv[ii][0] == '-') {
            if (!Stricmp(argv[ii], "-p") || !Stricmp(argv[ii], "-pause")) {
                args->arg_flags |= ARG_FLAG_PAUSE;
            } else if (!Stricmp(argv[ii], "-?") || !Stricmp(argv[ii], "-help")) {
                args->arg_flags |= ARG_FLAG_HELP | ARG_FLAG_DONE;
            } else if (!Stricmp(argv[ii], "-version")) {
                args->arg_flags |= ARG_FLAG_VERSION | ARG_FLAG_DONE;

            } else {
                estat = set_error("Unrecognized option %s", argv[ii]);
            }
        }
    }

    return (estat);
}
/***************************************************************/
int main(int argc, char *argv[]) {

    int estat;
    struct arginfo * args;

    if (argc <= 1) {
        showusage(argv[0]);
        return (0);
    }

    args = init();

    estat = getargs(argc, argv, args);

    if (args->arg_flags & ARG_FLAG_HELP) {
        showusage(argv[0]);
    }

    if (args->arg_flags & ARG_FLAG_VERSION) {
        printf("%s - %s\n", PROGRAM_NAME, VERSION);
    }

    if (!estat && !(args->arg_flags & ARG_FLAG_DONE)) {
        estat = process(args);
    }

    shut(args);

#if CMEM_TYPE == 2
    print_mem_stats("stdout");
#endif

    if (args->arg_flags & ARG_FLAG_PAUSE) {
        char inbuf[100];
        printf("Press ENTER to continue...");
        fgets(inbuf, sizeof(inbuf), stdin);
    }

    return (estat);
}
/***************************************************************/
#endif
/***************************************************************/
