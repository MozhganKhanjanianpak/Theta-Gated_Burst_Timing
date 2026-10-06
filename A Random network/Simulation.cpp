/*
 * ================================================================
 * Simulation of activity dynamics in a random network
 * ================================================================
 *
 * This code implements the discrete-time network model described
 * in the manuscript:
 *
 *   "Theta-Gated Burst Timing and Competition in Modular 
 *         Excitatory–Inhibitory Networks"
 *
 * The network is a directed Erdős–Rényi random network in which
 * nodes are divided into excitatory and inhibitory populations.
 * The synaptic activity has a finite lifetime, with different
 * durations for excitatory and inhibitory connections.
 *
 * External activation is temporally modulated by a sinusoidal
 * theta drive:
 *
 *   eta(t) = eta_0 + eta_A * sin(phi)
 *
 * where
 *
 *   phi = 2*pi*(t mod T_theta)/T_theta.
 *
 * The simulation starts with a random set of active neurons
 * determined by the external activation probability at t = 0.
 * Subsequently, neuronal states are updated from synaptic input
 * and stochastic external activation.
 *
 * Network type:
 *   Directed Erdős–Rényi random network
 *
 * Output files:
 *   ActivityInTime.txt
 *       Time step and fraction of active neurons (rho).
 *
 *   EtaInTime.txt
 *       Time step and instantaneous external activation
 *       probability eta(t).
 *
 *   Parameters.txt
 *       Main model parameters used in the simulation.
 *
 * ================================================================
 */


#include <iostream>
#include <vector>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <string>
#include <stdexcept>

using namespace std;


/* ================================================================
 * Model and simulation parameters
 * ================================================================
 *
 * N:
 *   Total number of neurons.
 *
 * p:
 *   Connection probability of the Erdős–Rényi network.
 *
 * E:
 *   Fraction of excitatory neurons. The first N*E neurons are
 *   excitatory and the remaining neurons are inhibitory.
 *
 * W0E, W0I:
 *   Synaptic weights of excitatory and inhibitory connections.
 *
 * TE, TI:
 *   Lifetime of an active excitatory or inhibitory synapse,
 *   measured in simulation time steps.
 *
 * D:
 *   Threshold for activation by synaptic input.
 *
 * tmax:
 *   Total number of simulation time steps.
 *
 * eta_0:
 *   Baseline external activation probability.
 *
 * eta_A:
 *   Amplitude of the theta modulation.
 *
 * T_theta:
 *   Period of the sinusoidal theta modulation, in simulation
 *   time steps.
 *
 * The instantaneous external activation probability is
 *
 *   eta(t) = eta_0 + eta_A * sin(phi).
 *
 * ================================================================
 */

#define N       2000
#define p       0.1
#define E       0.8

#define W0E     1
#define W0I     -4

#define TE      5
#define TI      7

#define D       8

#define tmax    25000

#define eta_0   0.0025
#define eta_A   0.002
#define T_theta 200


/*
 * Heaviside step function.
 *
 * H(z) = 1 for z > 0
 * H(z) = 0 otherwise.
 *
 * This definition is used for thresholding synaptic and
 * external inputs.
 */
#define H(z) ((z > 0) ? 1 : 0)


const double PI = 3.141592653589793;


/* ================================================================
 * Synaptic connection
 * ================================================================
 *
 * Only existing directed connections are stored.
 *
 * target:
 *   Index of the postsynaptic neuron.
 *
 * weight:
 *   Synaptic weight. Positive for excitatory and negative for
 *   inhibitory connections.
 *
 * lifetime:
 *   Remaining lifetime of the active synapse. A value of zero
 *   indicates that the synapse is currently inactive.
 *
 * ================================================================
 */

struct Link {
    int target;
    double weight;
    int lifetime;
};


/* ================================================================
 * Network and neuronal state
 * ================================================================
 */

/*
 * Adjacency list representation of the directed network.
 *
 * adj[i] contains all outgoing connections originating from
 * neuron i.
 */
vector<Link> adj[N];


/*
 * node_state[i]:
 *   Current state of neuron i.
 *
 *   0 -> inactive
 *   1 -> active
 *
 * node_input[i]:
 *   Total synaptic input received by neuron i at the current
 *   time step.
 */
int node_state[N] = {0};
int node_input[N] = {0};


/* ================================================================
 * Main simulation
 * ================================================================
 */

int main()
{
    /* ------------------------------------------------------------
     * Check the external activation parameters.
     *
     * Since
     *
     *   eta(t) = eta_0 + eta_A*sin(phi),
     *
     * the activation probability must remain within [0, 1]
     * throughout the theta cycle. Therefore:
     *
     *   eta_A <= eta_0
     *   eta_0 + eta_A <= 1
     * ------------------------------------------------------------
     */

    if ((eta_A > eta_0) || (eta_A + eta_0 > 1)) {
        cerr << "Error: eta_0 and/or eta_A ranges are not valid!\n";
        return 1;
    }


    /* ------------------------------------------------------------
     * Output files
     * ------------------------------------------------------------
     *
     * ActivityInTime.txt:
     *   time    rho(t)
     *
     * EtaInTime.txt:
     *   time    eta(t)
     *
     * Parameters.txt:
     *   Model parameters, one parameter per line.
     * ------------------------------------------------------------
     */

    ofstream output("ActivityInTime.txt");
    ofstream output_eta("EtaInTime.txt");
    ofstream output_parameters("Parameters.txt");


    /* ------------------------------------------------------------
     * Initialize the pseudo-random number generator.
     *
     * A time-dependent seed is used so that independent executions
     * generate different network realizations and stochastic
     * activation sequences.
     * ------------------------------------------------------------
     */

    srand(time(NULL));


    /* ------------------------------------------------------------
     * Number of excitatory neurons
     * ------------------------------------------------------------
     *
     * The first N_E neurons are excitatory.
     * The remaining N - N_E neurons are inhibitory.
     * ------------------------------------------------------------
     */

    int N_E = N * E;


    int t = 0;


    /* ------------------------------------------------------------
     * Save the main model parameters.
     *
     * The current one-parameter-per-line format is retained so
     * that it remains compatible with the analysis code.
     * ------------------------------------------------------------
     */

    output_parameters << W0E << endl;
    output_parameters << W0I << endl;
    output_parameters << TE << endl;
    output_parameters << TI << endl;
    output_parameters << D << endl;
    output_parameters << eta_0 << endl;
    output_parameters << eta_A << endl;
    output_parameters << T_theta << endl;

    output_parameters.close();


    /* ============================================================
     * 1. Construct the directed Erdős–Rényi network
     * ============================================================
     *
     * For every ordered pair of distinct neurons (i,j), a directed
     * connection i -> j is created with probability p.
     *
     * The type of a connection is determined by its source neuron:
     *
     *   source neuron i < N_E
     *       -> excitatory connection with weight W0E
     *
     *   source neuron i >= N_E
     *       -> inhibitory connection with weight W0I
     *
     * Self-connections are excluded.
     * ============================================================
     */

    int total_link = 0;

    for (int i = 0; i < N; i++) {

        for (int j = 0; j < N; j++) {

            /* No self-connections. */
            if (i == j)
                continue;


            /* Draw a random number uniformly from [0,1]. */
            double r = rand() / double(RAND_MAX);


            /* Create the directed connection with probability p. */
            if (r < p) {

                Link link;

                link.target = j;
                link.lifetime = 0;


                /*
                 * Assign the synaptic weight according to the
                 * type of the source neuron.
                 */
                if (i < N_E)
                    link.weight = W0E;      // Excitatory
                else
                    link.weight = W0I;      // Inhibitory


                /*
                 * Store the connection in the adjacency list
                 * of its source neuron.
                 */
                adj[i].push_back(link);
            }
        }
    }


    /* ============================================================
     * 2. Initial activation at t = 0
     * ============================================================
     *
     * The initial external activation probability is evaluated
     * from the phase of the theta drive at t = 0.
     * ============================================================
     */

    double phi = 2 * PI * (t % T_theta) / T_theta;

    double eta_t = eta_0 + eta_A * sin(phi);


    /*
     * Number of initially active excitatory and inhibitory neurons.
     *
     * The expected fraction of active neurons in both populations
     * is determined by eta_t.
     */
    int N_E_0 = N_E * eta_t;
    int N_I_0 = (N - N_E) * eta_t;


    /* ------------------------------------------------------------
     * Random activation of excitatory neurons
     * ------------------------------------------------------------
     *
     * Randomly select N_E_0 distinct excitatory neurons and
     * activate them.
     * ------------------------------------------------------------
     */

    int N_E_0_counter = 0;

    while (N_E_0_counter < N_E_0) {

        int rand_E_node = rand() % N_E;

        if (node_state[rand_E_node] == 0) {

            node_state[rand_E_node] = 1;

            N_E_0_counter++;
        }
    }


    /* ------------------------------------------------------------
     * Random activation of inhibitory neurons
     * ------------------------------------------------------------
     *
     * Randomly select N_I_0 distinct inhibitory neurons and
     * activate them.
     *
     * Inhibitory neurons occupy indices N_E ... N-1.
     * ------------------------------------------------------------
     */

    int N_I_0_counter = 0;

    while (N_I_0_counter < N_I_0) {

        int rand_I_node = rand() % (N - N_E);

        /*
         * Map the random index to the inhibitory-neuron range.
         */
        int inhibitory_node = N - rand_I_node - 1;


        if (node_state[inhibitory_node] == 0) {

            node_state[inhibitory_node] = 1;

            N_I_0_counter++;
        }
    }


    /* ------------------------------------------------------------
     * Record network activity at t = 0
     * ------------------------------------------------------------
     *
     * rho(t) is the fraction of active neurons:
     *
     *   rho(t) = N_active(t) / N
     * ------------------------------------------------------------
     */

    double active = 0;

    for (int i = 0; i < N; i++)
        active += node_state[i];


    output << "0\t" << active / N << endl;


    /* Record the external activation probability at t = 0. */
    output_eta << "0\t" << eta_t << endl;


    /* ============================================================
     * 3. Network dynamics
     * ============================================================
     *
     * The simulation proceeds for t = 1,...,tmax.
     *
     * Each time step consists of:
     *
     *   (1) Update synaptic lifetimes
     *   (2) Compute total synaptic input to each neuron
     *   (3) Update neuronal states
     *   (4) Record network activity
     * ============================================================
     */

    for (t = 1; t <= tmax; t++) {


        /* ========================================================
         * (1) Update synaptic lifetimes
         * ========================================================
         *
         * If an active synapse already has a non-zero lifetime,
         * its lifetime decreases by one.
         *
         * If an inactive synapse has lifetime zero and its source
         * neuron was active at prevous time step, the synapse becomes 
         * active and its lifetime is set to TE or TI depending on the
         * type of its source neuron.
         *
         * Thus, activation of a neuron initiates transmission
         * along all of its outgoing connections.
         * ========================================================
         */

        for (int i = 0; i < N; i++) {

            /*
             * Determine the lifetime of synapses originating
             * from neuron i.
             */
            int Tmax = (i < N_E) ? TE : TI;


            for (auto &link : adj[i]) {

                /*
                 * term1:
                 *   Decrease the lifetime of an already active
                 *   synapse by one.
                 *
                 * term2:
                 *   If the synapse is inactive and the source
                 *   neuron was active, start a new synaptic
                 *   lifetime of Tmax.
                 */
                int term1 = H(link.lifetime) *
                            (link.lifetime - 1);

                int term2 = (1 - H(link.lifetime)) *
                            Tmax *
                            node_state[i];


                link.lifetime = term1 + term2;
            }
        }


        /* ========================================================
         * (2) Compute synaptic input
         * ========================================================
         *
         * First reset the total input received by every neuron.
         * ========================================================
         */

        for (int i = 0; i < N; i++)
            node_input[i] = 0;


        /*
         * Each active synapse contributes its weight to the
         * postsynaptic target.
         */
        for (int i = 0; i < N; i++) {

            for (auto &link : adj[i]) {

                node_input[link.target] +=
                    H(link.lifetime) * link.weight;
            }
        }


        /* ========================================================
         * (3) Update neuronal states
         * ========================================================
         *
         * First compute the phase and external activation
         * probability for the current time step.
         * ========================================================
         */

        phi = 2 * PI * (t % T_theta) / T_theta;

        eta_t = eta_0 + eta_A * sin(phi);


        for (int i = 0; i < N; i++) {

            /*
             * Activation by synaptic input.
             *
             * A neuron becomes active if its total synaptic
             * input reaches or exceeds the threshold D.
             *
             * H(node_input[i] - D) implements this threshold.
             */
            int term1 = H(node_input[i] - D);


            /*
             * Activation by external input.
             *
             * External activation is considered only when the
             * neuron was not activated by synaptic input.
             *
             * A random number r is drawn for each neuron and
             * the neuron receives external activation when
             *
             *   r < eta_t.
             */
            int term2 = 0;

            double r = rand() / double(RAND_MAX);

            term2 = (1 - H(node_input[i] - D)) *
                    H(eta_t - r);


            /*
             * The new neuronal state is determined by either
             * synaptic or external activation.
             */
            node_state[i] = term1 + term2;
        }


        /* ========================================================
         * (4) Record network activity
         * ========================================================
         *
         * Compute the fraction of active neurons:
         *
         *   rho(t) = N_active(t) / N
         * ========================================================
         */

        active = 0;

        for (int i = 0; i < N; i++)
            active += node_state[i];


        /* Record rho(t). */
        output << t << "\t" << active / N << endl;


        /* Record eta(t). */
        output_eta << t << "\t" << eta_t << endl;
    }


    /* ============================================================
     * 4. Close output files
     * ============================================================
     */

    output.close();
    output_eta.close();


    return 0;
}