# Reproducibility Repository

This repository contains the simulation and analysis code used to reproduce the results presented in the paper.

The repository is organized into two main folders corresponding to the two network conditions considered in the study:

```text
.
├── A Random network/
│   ├── simulation.cpp
│   ├── Analysis.ipynb
│   ├── ActivityInTime.txt
│   ├── EtaInTime.txt
│   └── Parameters.txt
│
└── B Modular network/
    ├── simulation.cpp
    ├── Analysis.ipynb
    ├── ActivityInTime.txt
    ├── BlockNodesNumberM8E5I45.txt
    └── ModularAdjListM8E5I45.txt
```

### `A Random network`

Contains the C++ simulation, analysis notebook, and an example simulation output for the random-network condition.

### `B Modular network`

Contains the C++ simulation, analysis notebook, the network-structure files required by the simulation, and an example simulation output for the modular-network condition.

Each folder contains its own README with details on the model, simulation parameters, required files, and instructions for running the simulations and analyses.
