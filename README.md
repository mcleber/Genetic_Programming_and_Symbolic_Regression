# Genetic Programming and Symbolic Regression

<!-- >> 🚧 This repository is a work in progress. -->

### A brief introduction to Genetic Programming and Symbolic Regression with C++23.

![IDE](https://img.shields.io/badge/IDE-VS%20Code-blue)
![Language](https://img.shields.io/badge/Language-C%2B%2B23-orange)
![Language](https://img.shields.io/badge/Language-Python-orange)
![License](https://img.shields.io/badge/License-GPL--3.0-blue)

In this tutorial-oriented implementation, the algorithm automatically evolves mathematical expression trees to approximate the target function `f(x) = x² + x + 1` without prior knowledge of its underlying equation.

---

## Overview

Genetic Programming (GP) is an Evolutionary Computation technique that evolves programs
or expressions to solve a problem, inspired by natural selection and the principles of
genetics. The algorithm starts with a population of randomly generated expression trees
and, over successive generations, applies selection, crossover, and mutation to discover
the equation that best represents the observed data, without prior knowledge of the
target formula.

The project covers all essential components of a functional Genetic Programming system:

| Component                 | Description                                                                                 |
| ------------------------- | ------------------------------------------------------------------------------------------- |
| Tree Representation       | Operators (+, -, *, /) as internal nodes; `x` and constants as leaf nodes.                  |
| Population Initialization | Random generation of expression trees (depth = 2).                                          |
| Fitness Evaluation        | Absolute error over x ∈ {-5,-3,-2,-1,-0.5, 0, 0.5, 1, 2, 3, 5} with parsimony size penalty. |
| Selection                 | Top **30%** by fitness with elitism (1 individual preserved intact).                        |
| Crossover                 | Koza subtree crossover — biased toward internal (operator) donor nodes.                     |
| Mutation                  | Koza subtree mutation — replaces a random subtree with a new random tree (prob = 0.40).     |
| Bloat Control             | Parsimony pressure: `fitness -= α × tree_size` (α = 0.03).                                  |
| Parallelism               | `std::execution::par_unseq` for parallel fitness evaluation (requires TBB on Linux).        |
| Export & Visualization    | CSV export (`evolution_fitness.csv`) and annotated Matplotlib plots.                        |


> Complete documentation, including detailed explanations of each component and the fully commented source code, is available both in the PDF located in the `docs/` directory and in the source files contained in `src/`. The PDF is currently available only in Portuguese.

---

## Symbolic Regression

Symbolic Regression is the most classical application of Genetic Programming. Unlike linear or polynomial regression, no equation structure is assumed in advance — the algorithm starts from scratch and discovers both the form and the parameters of the expression automatically.

In this tutorial, the target function is `f(x) = x² + x + 1`. The algorithm has no knowledge of this equation. It only receives pairs `(x, f(x))` and evolves expression trees that reproduce the same values — simulating the real-world scenario: observed data, unknown law.

### How the Tree Represents an Equation

Each internal node of the tree is an operator (`+`, `-`, `*`) and each leaf is a variable (`x`) or a numeric constant. Evaluation follows a post-order traversal: children are evaluated recursively and the operator at the root is applied. The expression `x² + x + 1` as a tree:

```text
       +
      / \
     +   1
    / \
   *   x
  / \
 x   x
```

### Fitness Calculation

Fitness measures how well a tree approximates the target function. It is computed as the negative sum of absolute errors over five values of `x`:

| Term                          | Role                               |
| ----------------------------- | ---------------------------------- |
| `y_true = x² + x + 1`         | Target function output             |
| `y_pred = evaluate(tree, x)`  | Tree output                        |
| `err += abs(y_true - y_pred)` | Absolute error per test point      |
| `penalty = 0.03 × tree_size`  | Parsimony pressure (bloat control) |
| `return -err - penalty`       | Higher (closer to 0) is better     |


`fitness = 0` means zero error — the tree found the exact equation. The more negative, the worse the approximation.

### Results

With `POP_SIZE = 200` and `GENERATIONS = 100`, the algorithm consistently converges to
the exact solution by generation ~15–59 (depending on random seed):

```
Generation 59: Best fitness = -0.21   Expression: ((x+1)+(x*x))
```

All expressions with near-zero fitness are algebraically equivalent to x² + x + 1.
The cleanest form found:

```
(x*x) + (x+1)  =  x² + x + 1  ✓
```

### The Bloat Problem

Bloat is the uncontrolled growth of trees across generations. Two main causes:

- **Growth pressure from crossover**: statistically more likely for offspring to be
  larger than smaller — most nodes live in deep subtrees, so crossover tends to insert
  material rather than remove it.
- **Neutral subtrees (introns)**: subexpressions such as `x*1`, `x+0`, or `a-a` do
  not change the expression value but occupy space. Since they do not hurt fitness,
  selection never removes them.

This implementation controls bloat via **parsimony pressure** (`α × tree_size` subtracted
from fitness). The bloat peak (~40 nodes average) occurs around generation 35; after that
parsimony eliminates bloated individuals faster than crossover creates them, stabilising
the average size around 15–20 nodes.

---

## Repository Structure

```text
Genetic_Programming_and_Symbolic_Regression/
|
├── src/
|   ├── main.cpp
│   └── plot_fitness.py
|
├── docs/
│   └── Tutorial_Programacao_Genetica_Regressao_Simbolica_Cpp23.pdf
|
├── CMakeLists.txt
|
├── .editorconfig
├── .gitignore
├── License
└── README.md
```

---

## Build and Run

```bash
cd Genetic_Programming_and_Symbolic_Regression
mkdir build && cd build
cmake ..
cmake --build .
./programacao_genetica
```

to save the output in the build/ directory:

```bash
./programacao_genetica > output.txt
```

and to open the file:

```bash
cat output.txt
```

> Requires a compiler with **C++23** support and **Intel TBB** (for parallel execution).

```bash
# Ubuntu / Debian
sudo apt install libtbb-dev
```

> Tested with GCC 13+.

---

## Evolution Visualization

After running the program, generate the annotated plot:

```bash
python3 ../src/plot_evolution.py
```

The script reads `evolution_fitness.csv` (generated automatically by the C++ program)
and creates `evolution_annotated.png`. All annotation positions are computed dynamically
from the data, so the chart is correct regardless of which run produced the CSV.

Requires pandas and matplotlib:

```bash
pip install pandas matplotlib
```

The chart contains two panels:

1. **Fitness Evolution** — best fitness, average fitness, and ideal reference (0).
  Annotations mark: convergence point, bloat crisis, and stabilisation.
2. **Bloat Monitoring** — average tree size per generation.
  Annotations mark: initial growth, peak size, sharp decline, and stable plateau.

---

## Additional Notes

This project was developed for educational purposes during my initial studies in Evolutionary Computation and Genetic Programming. Its primary goal is to emphasize conceptual clarity rather than performance or the completeness expected from a general-purpose library.

If you find any errors, inaccuracies, or opportunities for improvement, please feel free to open an *issue* or get in touch. Contributions and corrections are always welcome.
