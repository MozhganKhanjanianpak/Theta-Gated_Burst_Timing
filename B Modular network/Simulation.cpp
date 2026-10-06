/////////////////////////////////////////////////////////////
// Modular network simulation
//
// The external input is sinusoidally modulated at theta
// frequency:
//
//     eta(t) = eta_0 + eta_A * sin(phi_theta)
//
// The network consists of M modules. External input is
// distributed according to the spatial concentration
// parameter F and the ambiguity parameter alpha.
//
// Modules g1 and g2 are the two target modules.
// alpha = 0 corresponds to a single unambiguous target
// (module g1).
/////////////////////////////////////////////////////////////

#include <iostream>
#include <vector>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <cmath>

using namespace std;


// ==========================================================
// Model parameters
// ==========================================================

#define N       2000        // Total number of neurons
#define M       8           // Number of modules

#define W0E     1           // Excitatory synaptic weight
#define W0I    -4           // Inhibitory synaptic weight

#define TE      5           // Excitatory synaptic lifetime
#define TI      7           // Inhibitory synaptic lifetime
#define D       8           // Activation threshold

#define F       0.4         // Spatial concentration of external input

#define alpha   0.5         // Ambiguity parameter in [0, 1]
// For a single target, set alpha = 0.

#define tmax    25000       // Number of simulation time steps

#define eta_0   0.0015      // Mean external activation probability
#define eta_A   0.001       // Amplitude of theta modulation

#define T_theta 200         // Theta period (ms)


// Heaviside step function:
// H(z) = 1 for z > 0, and 0 otherwise.
#define H(z) ((z > 0) ? 1 : 0)


const double PI = 3.141592653589793;


// ==========================================================
// Link structure
//
// Only existing connections are stored in the adjacency
// lists. Each link contains:
//
//   target   : target neuron
//   weight   : excitatory or inhibitory synaptic weight
//   lifetime : remaining lifetime of the active synapse
// ==========================================================

struct Link {
    int target;
    double weight;
    int lifetime;
};


// Sparse adjacency list
vector<Link> adj[N];


// State variables
int node_state[N] = {0};
int node_input[N] = {0};


// Information about the modules.
//
// For each module:
//   [0] = module label
//   [1] = number of neurons
//   [2] = starting neuron index
//   [3] = ending neuron index
//
// Row 0 stores general information about the network,
// including the number of modules.
int block_node_number[M + 1][4] = {0};


// ==========================================================
// Main simulation
// ==========================================================

int main()
{
    // ------------------------------------------------------
    // Validate model parameters
    // ------------------------------------------------------

    if ((eta_A > eta_0) || (eta_A + eta_0 > 1)) {
        cerr << "Error: eta_0 and/or eta_A values are invalid.\n";
        return 1;
    }

    if ((F < 0) || (F > 1)) {
        cerr << "Error: F must be in the range [0, 1].\n";
        return 1;
    }

    if ((alpha < 0) || (alpha > 1)) {
        cerr << "Error: alpha must be in the range [0, 1].\n";
        return 1;
    }


    // Random seed for stochastic external activation
    srand(time(NULL));


    // ------------------------------------------------------
    // Variables controlling the external input
    // ------------------------------------------------------

    double eta_g_1;
    double eta_g_2;
    double eta_m;

    double r;

    int t = 0;


    // ======================================================
    // Load module structure
    // ======================================================

    //
    // The file contains:
    //
    //   number of modules
    //
    // followed by one line for each module containing:
    //
    //   module label
    //   number of neurons
    //   first neuron index
    //   last neuron index
    //

    ifstream input_file_blocks(
        "./BlockNodesNumberM8E5I45.txt"
    );

    if (!input_file_blocks) {
        cerr << "Error: Could not open "
             << "BlockNodesNumberM8E5I45.txt\n";
        return 1;
    }


    int num_modules;
    int unused_block_parameter;

    input_file_blocks
        >> num_modules
        >> unused_block_parameter
        >> unused_block_parameter
        >> unused_block_parameter;

    block_node_number[0][0] = num_modules;


    for (int module = 0; module < num_modules; module++) {

        int module_label;
        int num_neurons;
        int first_neuron;
        int last_neuron;

        input_file_blocks
            >> module_label
            >> num_neurons
            >> first_neuron
            >> last_neuron;

        block_node_number[module + 1][0] = module_label;
        block_node_number[module + 1][1] = num_neurons;
        block_node_number[module + 1][2] = first_neuron;
        block_node_number[module + 1][3] = last_neuron;
    }

    input_file_blocks.close();


    // Number of neurons in the two target modules
    int N_g_1 = block_node_number[1][1];
    int N_g_2 = block_node_number[2][1];

    // Last neuron index of each target module
    int end_g_1 = block_node_number[1][3];
    int end_g_2 = block_node_number[2][3];


    // Prevent unused-variable warnings while keeping these
    // quantities available as explicit model information.
    (void)N_g_1;
    (void)N_g_2;


    // ======================================================
    // Load modular network
    // ======================================================

    ifstream input_file(
        "./ModularAdjListM8E5I45.txt"
    );

    if (!input_file) {
        cerr << "Error: Could not open "
             << "ModularAdjListM8E5I45.txt\n";
        return 1;
    }


    //
    // Network file header:
    //
    //   first value  : network/module metadata
    //   second value : total number of links
    //   third value  : additional metadata
    //
    int network_parameter_1;
    int total_link;
    int network_parameter_3;

    input_file
        >> network_parameter_1
        >> total_link
        >> network_parameter_3;


    // Read all existing directed links
    for (int c = 0; c < total_link; c++) {

        int source;
        int target;
        int link_type;

        input_file
            >> source
            >> target
            >> link_type;

        Link link;

        link.target = target;
        link.lifetime = 0;

        // Positive links are excitatory; negative links
        // are inhibitory.
        if (link_type > 0) {
            link.weight = W0E;
        }
        else {
            link.weight = W0I;
        }

        adj[source].push_back(link);
    }

    input_file.close();


    // ======================================================
    // Output file
    // ======================================================

    ofstream output("./ActivityInTime.txt");

    if (!output) {
        cerr << "Error: Could not create ActivityInTime.txt\n";
        return 1;
    }


    output
        << "##############################################################"
        << endl;

    output
        << "#Parameters: D = " << D
        << "\tT_theta = " << T_theta
        << "\tAlpha = " << alpha
        << endl;

    output
        << "#Eta_0 = " << eta_0
        << "\tEta_A = " << eta_A
        << "\tF = " << F
        << endl;

    output
        << "##############################################################"
        << endl;


    // ======================================================
    // Initialize neuron states and synaptic lifetimes
    // ======================================================

    for (int i = 0; i < N; i++) {

        node_state[i] = 0;

        for (auto &link : adj[i]) {
            link.lifetime = 0;
        }
    }


    // ======================================================
    // Initial activation at t = 0
    // ======================================================

    // Theta phase and external activation probability
    double phi =
        2 * PI * (t % T_theta) / T_theta;

    double eta_t =
        eta_0 + eta_A * sin(phi);


    // ------------------------------------------------------
    // Distribute the external input
    //
    // g1 and g2 are the two target modules.
    // eta_m is the input probability for non-target modules.
    // ------------------------------------------------------

    eta_g_1 =
        eta_t *
        (1 + (M - 1 - M * alpha / 2) * F);

    eta_g_2 =
        eta_t *
        (1 + (-1 + M * alpha / 2) * F);

    eta_m =
        eta_t * (1 - F);


    // ------------------------------------------------------
    // Stochastic external activation
    // ------------------------------------------------------

    for (int i = 0; i < N; i++) {

        r = rand() / double(RAND_MAX);

        node_state[i] =
            H(end_g_1 - i) *
            H(eta_g_1 - r)

            +

            (1 - H(end_g_1 - i)) *
            H(end_g_2 - i) *
            H(eta_g_2 - r)

            +

            (1 - H(end_g_1 - i)) *
            (1 - H(end_g_2 - i)) *
            H(eta_m - r);
    }


    // ======================================================
    // Output activity at t = 0
    // ======================================================

    double active = 0;

    output << t << "\t";

    for (int module = 0; module < num_modules; module++) {

        active = 0;

        for (
            int i = block_node_number[module][3];
            i < block_node_number[module + 1][3];
            i++
        ) {
            active += node_state[i];
        }

        output << active / N << "\t";
    }

    output << endl;


    // ======================================================
    // Network dynamics
    // ======================================================

    for (t = 1; t <= tmax; t++) {


        // --------------------------------------------------
        // (1) Update synaptic lifetimes
        // --------------------------------------------------

        for (int i = 0; i < N; i++) {

            for (auto &link : adj[i]) {

                // Excitatory and inhibitory connections have
                // different active lifetimes.
                int Tmax =
                    (link.weight > 0) ? TE : TI;


                // Existing active connection:
                // decrease its remaining lifetime.
                int term1 =
                    H(link.lifetime) *
                    (link.lifetime - 1);


                // Inactive connection:
                // activate it if the source neuron was active.
                int term2 =
                    (1 - H(link.lifetime)) *
                    Tmax *
                    node_state[i];


                link.lifetime =
                    term1 + term2;
            }
        }


        // --------------------------------------------------
        // (2) Compute total synaptic input to each neuron
        // --------------------------------------------------

        for (int i = 0; i < N; i++) {
            node_input[i] = 0;
        }


        for (int i = 0; i < N; i++) {

            for (auto &link : adj[i]) {

                node_input[link.target] +=
                    H(link.lifetime) * link.weight;
            }
        }


        // --------------------------------------------------
        // (3) Update neuron states
        // --------------------------------------------------

        // Current theta phase and external activation
        // probability.
        phi =
            2 * PI * (t % T_theta) / T_theta;

        eta_t =
            eta_0 + eta_A * sin(phi);


        // External input probabilities for the target
        // and non-target modules.
        eta_g_1 =
            eta_t *
            (1 + (M - 1 - M * alpha / 2) * F);

        eta_g_2 =
            eta_t *
            (1 + (-1 + M * alpha / 2) * F);

        eta_m =
            eta_t * (1 - F);


        for (int i = 0; i < N; i++) {

            // ----------------------------------------------
            // Activation due to synaptic input
            // ----------------------------------------------

            int term1 =
                H(node_input[i] - D);


            // ----------------------------------------------
            // Activation due to external input
            //
            // External activation is applied only when the
            // neuron is not already activated by synaptic
            // input.
            // ----------------------------------------------

            r = rand() / double(RAND_MAX);

            int term2 =
                (1 - H(node_input[i] - D))
                *
                (
                    H(end_g_1 - i) *
                    H(eta_g_1 - r)

                    +

                    (1 - H(end_g_1 - i)) *
                    H(end_g_2 - i) *
                    H(eta_g_2 - r)

                    +

                    (1 - H(end_g_1 - i)) *
                    (1 - H(end_g_2 - i)) *
                    H(eta_m - r)
                );


            // Final state of the neuron
            node_state[i] =
                term1 + term2;
        }


        // --------------------------------------------------
        // (4) Record activity in each module
        // --------------------------------------------------

        active = 0;

        output << t << "\t";

        for (int module = 0; module < num_modules; module++) {

            active = 0;

            for (
                int i = block_node_number[module][3];
                i < block_node_number[module + 1][3];
                i++
            ) {
                active += node_state[i];
            }

            output << active / N << "\t";
        }

        output << endl;
    }


    output.close();

    return 0;
}