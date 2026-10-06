# Modular network

This folder contains the simulation code, analysis notebook, network-structure files, and an example simulation output for the **modular-network condition** described in the paper.

The network consists of **8 modules** with a total of **2000 nodes**. The network is generated beforehand, and its modular structure and node assignments are provided as input files to the simulation code.

The external input is modulated by a theta-frequency oscillation and its spatial distribution across the network is controlled by the parameter \(F\). In addition, the ambiguity between the two target modules is controlled by the parameter \(\alpha\).

---

## Files

### Simulation

* `simulation.cpp`
  C++ code for running the neuronal-network simulation.

### Analysis

* `Analysis.ipynb`
  Jupyter Notebook used to analyze the simulation output and generate the corresponding results and figures.

### Network structure

The simulation reads the pre-generated modular network structure from the following files:

* `BlockNodesNumberM8E5I45.txt`
  Contains the node assignments and boundaries of the network modules.

* `ModularAdjListM8E5I45.txt`
  Contains the adjacency list defining the directed network connections and their excitatory/inhibitory types.

These files are required for running `simulation.cpp`.

### Example simulation output

* `ActivityInTime.txt`
  Example output from a simulation run. It contains the activity of the eight modules over time and can be used directly with `Analysis.ipynb`, without running the C++ simulation first.

---

## Model and simulation

The modular network contains:

* **2000 nodes**
* **8 modules**
* Excitatory and inhibitory synaptic interactions
* Long-lived inhibitory synaptic activity
* Theta-modulated external input

The main simulation parameters include:

| Parameter                             |     Value |
| ------------------------------------- | --------: |
| Number of nodes \(N\)                 |      2000 |
| Number of modules \(M\)               |         8 |
| Excitatory synaptic weight \(W_E\)    |        +1 |
| Inhibitory synaptic weight \(W_I\)    |        −4 |
| Excitatory lifetime \(T_E\)           |         5 |
| Inhibitory lifetime \(T_I\)           |         7 |
| Activation threshold \(D\)            |         8 |
| Simulation duration                   | 25,000 ms |
| Baseline input \(\eta_0\)             |    0.0015 |
| Theta-modulation amplitude \(\eta_A\) |     0.001 |
| Theta period \(T_\theta\)             |    200 ms |
| Spatial input concentration \(F\)     |       0.4 |
| Target ambiguity \(\alpha\)           |       0.5 |

The values of \(F\) and \(\alpha\) can be modified in `simulation.cpp` to investigate different spatial-input and target-ambiguity conditions.

---

## External input

The external activation probability is modulated periodically at theta frequency:

$$
\eta(t) = \eta_0 + \eta_A \sin(\phi),
$$

where the phase advances according to the theta period \(T_\theta\).

The parameter \(F\) controls the spatial concentration of a fixed external-input budget on the target modules. The parameter \(\alpha\) determines how the input directed toward the two target modules is distributed between them.

Thus, the modular-network simulations can be used to investigate how theta-modulated and spatially structured inputs affect the emergence and temporal ordering of network bursts.

---

## Running the simulation

A C++11-compatible compiler is required.

Compile the simulation with:

```bash
g++ -O3 -std=c++11 simulation.cpp -o simulation
```

Then run:

```bash
./simulation
```

Before running the simulation, make sure that the two network-structure files are located in the same directory as `simulation.cpp`:

```text
BlockNodesNumberM8E5I45.txt
ModularAdjListM8E5I45.txt
```

The simulation will read these files and generate:

```text
ActivityInTime.txt
```

This file contains the time series of the activity of the eight modules.

---

## Running the analysis

The repository includes an example `ActivityInTime.txt`, so the analysis can be performed without first running the C++ simulation.

Launch Jupyter Notebook from this directory:

```bash
jupyter notebook
```

Then open:

```text
Analysis.ipynb
```

The notebook performs the burst detection and analyzes the temporal responses of the two target modules. Among other quantities, it calculates:

* first-burst onset times for the two target modules and the whole network,
* winner classification of the two target modules,
* first-response probabilities,
* winner-switching rate,
* winner sequences across theta cycles,
* temporal lag between the two target modules,
* distributions of temporal differences.

The notebook also produces the figures used to characterize the temporal organization of the network response.

---

## Python requirements

The analysis requires Python and the following packages:

```text
numpy
scipy
matplotlib
jupyter
```

They can be installed with:

```bash
pip install numpy scipy matplotlib jupyter
```

---

## Reproducibility

The provided network-structure files define the modular network used by the simulation. The included `ActivityInTime.txt` provides an example simulation output that allows the analysis notebook to be run directly.

Because the simulation uses stochastic external activation, a newly generated simulation will generally not produce exactly the same time series as the included example output.

To reproduce the exact analysis shown in the repository, the provided example `ActivityInTime.txt` can therefore be used directly.

---

## Folder structure

The folder is organized as follows:

```text
Modular network/
│
├── simulation.cpp
├── Analysis.ipynb
├── ActivityInTime.txt
├── BlockNodesNumberM8E5I45.txt
└── ModularAdjListM8E5I45.txt
```

The two network-structure files are required by the simulation, while the example `ActivityInTime.txt` allows the analysis to be reproduced without rerunning the simulation.

```
```
