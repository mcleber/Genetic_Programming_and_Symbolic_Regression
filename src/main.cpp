/**
 * @file main.cpp
 * @brief Genetic Programming implementation in C++23.
 *
 * This program evolves mathematical expression trees
 * to approximate the target function: f(x) = x^2 + x + 1.
 *
 * Koza operators implemented:
 * - Subtree crossover: swaps entire subtrees between two parents.
 * - Subtree mutation: replaces a subtree with a randomly generated one.
 *
 * Features:
 * - Uses std::unique_ptr to manage tree nodes.
 * - Supports binary operators: +, -, *, /
 * - Supports variables (x) and small integer constants.
 * - Implements: evaluation, fitness (with parsimony pressure),
 *   crossover, mutation, elitism, and hierarchical printing.
 *
 * BUILD & RUN
 * cd path/to/programacao_genetica
 * mkdir build && cd build
 * cmake ..
 * cmake --build .
 * ./programacao_genetica
 *
 * @author Cleber Moretti
 * @date 30 sep 2025
 */

#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <cmath>
#include <format>
#include <ranges>
#include <fstream>
#include <unordered_map>
#include <execution>
#include <numeric>

/// Alias for the variable map used during expression evaluation.
/// Example: {"x": 2.0} means x = 2 in the current evaluation context.
using VarMap = std::unordered_map<std::string, double>;

// ====================================================================
// Node -- expression tree node
// ====================================================================
/**
 * @struct Node
 * @brief Represents a node in a mathematical expression tree.
 *
 * Each node can be:
 * - An internal (operator) node: "+", "-", "*", "/"
 * - A terminal (leaf) node: "x" or a constant (e.g. "1", "2")
 *
 * Children are managed via std::unique_ptr to prevent memory leaks
 * and to allow safe deep copies via clone_tree().
 */
struct Node
{
    std::string value;                     ///< Operator or terminal value
    std::unique_ptr<Node> left  = nullptr; ///< Left child (nullptr for leaves)
    std::unique_ptr<Node> right = nullptr; ///< Right child (nullptr for leaves)

    /// Constructs a node with the given value.
    explicit Node(std::string val) : value(std::move(val)) {}
};

// ====================================================================
// evaluate -- recursive expression evaluator
// ====================================================================
/**
 * @brief Evaluates the expression tree for a given set of variable values.
 *
 * Traverses the tree recursively:
 * - Leaf nodes return the variable value from `vars` or a parsed constant.
 * - Internal nodes apply the binary operator to their evaluated children.
 *
 * Division by zero (or near-zero) is protected: if the right operand is
 * smaller than 1e-9, the left operand is returned as a neutral fallback.
 *
 * @param node Root of the expression tree (or current subtree in recursion).
 * @param vars Map of variable names to their current values, e.g. {"x": 1.5}.
 * @return The numeric result of evaluating the expression.
 */
double evaluate(const std::unique_ptr<Node>& node, const VarMap& vars)
{
    if (!node)
        return 0.0;

    // Terminal node: variable or numeric constant
    if (!node->left && !node->right)
    {
        auto it = vars.find(node->value);
        if (it != vars.end())
            return it->second; // known variable (e.g. "x")

        try
        {
            return std::stod(node->value); // numeric constant (e.g. "3")
        }
        catch (...)
        {
            return 0.0; // unknown token -- treated as 0
        }
    }

    double l = evaluate(node->left,  vars);
    double r = evaluate(node->right, vars);

    if (node->value == "+") return l + r;
    if (node->value == "-") return l - r;
    if (node->value == "*") return l * r;

    // Division with zero-guard
    if (std::abs(r) < 1e-9)
        return l; // neutral fallback instead of inf/nan

    return l / r;
}

// ====================================================================
// random_tree -- random expression tree generator
// ====================================================================
/**
 * @brief Generates a random expression tree using the full/grow method.
 *
 * At each internal level, a random binary operator (+, -, *, /) is chosen.
 * At depth == 0 (leaf level), either the variable "x" or a random integer
 * constant in [1, 5] is generated with equal probability.
 *
 * @param depth Maximum remaining depth before forcing a terminal node.
 *              Default is 2, producing trees with up to 7 nodes.
 * @return Unique pointer to the root of the generated tree.
 */
std::unique_ptr<Node> random_tree(int depth = 2)
{
    // Force terminal node at maximum depth
    if (depth == 0)
    {
        if (std::rand() % 2)
            return std::make_unique<Node>("x");

        // Random integer constant: "1" to "5"
        return std::make_unique<Node>(std::to_string(std::rand() % 5 + 1));
    }

    // Internal node: pick a random binary operator
    std::vector<std::string> ops = { "+", "-", "*", "/" };
    auto node = std::make_unique<Node>(ops[std::rand() % ops.size()]);
    node->left  = random_tree(depth - 1);
    node->right = random_tree(depth - 1);
    return node;
}

// ====================================================================
// tree_size -- node counter
// ====================================================================
/**
 * @brief Recursively counts the total number of nodes in the tree.
 *
 * Used to:
 * 1. Compute average tree size per generation (bloat monitoring).
 * 2. Apply parsimony pressure in the fitness function.
 *
 * @param node Root of the subtree to measure.
 * @return Total node count (internal + terminal nodes).
 */
std::size_t tree_size(const std::unique_ptr<Node>& node)
{
    if (!node)
        return 0;

    return 1 + tree_size(node->left) + tree_size(node->right);
}

// ====================================================================
// fitness -- evaluation with parsimony pressure
// ====================================================================
/**
 * @brief Computes the fitness of an expression tree against f(x) = x² + x + 1.
 *
 * Fitness = -(sum of absolute errors) - (alpha * tree_size)
 *
 * The negative sign makes this a maximisation problem where 0 is the best
 * achievable score (perfect fit with zero nodes, which is impossible in
 * practice -- the real best is a small, accurate tree).
 *
 * The parsimony term (alpha * size) penalises large trees, counteracting
 * the natural tendency of GP to accumulate introns (code bloat).
 *
 * Test points cover a range of negative, zero, and positive values so
 * that the evolved expression must generalise beyond any single region.
 *
 * @param tree  Root of the expression tree to evaluate.
 * @param alpha Parsimony coefficient (default = 0.03).
 *              Increase to favour smaller trees; decrease for accuracy.
 * @return Fitness value. Higher (closer to 0) is better.
 */
double fitness(const std::unique_ptr<Node>& tree, double alpha = 0.03)
{
    double err = 0.0;

    // Diverse test points: negative, fractional, zero, and positive
    std::vector<double> test = { -5, -3, -2, -1, -0.5, 0, 0.5, 1, 2, 3, 5 };

    for (double x : test)
    {
        VarMap vars = { {"x", x} };

        double y_true = x * x + x + 1.0;
        double y_pred = evaluate(tree, vars);

        err += std::abs(y_true - y_pred);
    }

    // Parsimony pressure: penalise unnecessarily large trees
    double penalty = alpha * static_cast<double>(tree_size(tree));

    return -err - penalty;
}

// ====================================================================
// clone_tree -- deep copy
// ====================================================================
/**
 * @brief Creates a full, independent deep copy of the expression tree.
 *
 * Because std::unique_ptr does not allow copying, every time a tree
 * must be duplicated (elitism, crossover child construction) this
 * function must be used explicitly.
 *
 * @param node Root of the tree to clone.
 * @return New tree with identical structure and values, sharing no memory
 *         with the original.
 */
std::unique_ptr<Node> clone_tree(const std::unique_ptr<Node>& node)
{
    if (!node) return nullptr;

    auto copy   = std::make_unique<Node>(node->value);
    copy->left  = clone_tree(node->left);
    copy->right = clone_tree(node->right);
    return copy;
}

// ====================================================================
// collect_nodes -- pointer harvester for crossover/mutation
// ====================================================================
/**
 * @brief Collects raw pointers to every unique_ptr<Node> in the tree.
 *
 * Stores addresses of the unique_ptr slots (not the Node* raw pointers),
 * so that a selected node can be replaced in-place simply by assigning
 * to *ptr without needing to know the parent.
 *
 * @param node  Current node (pass root on the initial call).
 * @param nodes Output vector; grows as the tree is traversed.
 */
void collect_nodes(
    std::unique_ptr<Node>&               node,
    std::vector<std::unique_ptr<Node>*>& nodes)
{
    if (!node) return;

    nodes.push_back(&node);
    collect_nodes(node->left,  nodes);
    collect_nodes(node->right, nodes);
}

// ====================================================================
// crossover -- Koza subtree crossover
// ====================================================================
/**
 * @brief Classic subtree crossover (Koza, 1992).
 *
 * Produces one offspring by:
 *  1. Cloning parent A to form the child.
 *  2. Selecting a random cut point in the child.
 *  3. Selecting a random subtree (preferably an internal node)
 *     from a clone of parent B.
 *  4. Replacing the child's subtree at the cut point with the
 *     selected subtree from B.
 *
 * The bias toward internal donor nodes (up to 10 attempts) follows
 * the standard Koza 90/10 rule, which favours functional crossover
 * over leaf swaps to promote meaningful recombination.
 *
 * @param a First parent (not modified).
 * @param b Second parent (not modified).
 * @return New child tree resulting from the crossover.
 */
std::unique_ptr<Node> crossover(
    const std::unique_ptr<Node>& a,
    const std::unique_ptr<Node>& b)
{
    if (!a || !b) return nullptr;

    // Step 1: child starts as a full clone of parent A
    auto child = clone_tree(a);

    // Step 2: collect all nodes from child and from a clone of B
    std::vector<std::unique_ptr<Node>*> child_nodes;
    collect_nodes(child, child_nodes);

    auto donor = clone_tree(b);
    std::vector<std::unique_ptr<Node>*> donor_nodes;
    collect_nodes(donor, donor_nodes);

    if (donor_nodes.empty())
        return child; // B is empty -- return child unchanged

    // Step 3: pick a random cut point in the child
    auto* cut_point = child_nodes[std::rand() % child_nodes.size()];

    // Step 4: prefer an internal node from B (up to 10 attempts)
    std::unique_ptr<Node>* donor_point = nullptr;
    bool found = false;

    for (int attempt = 0; attempt < 10; ++attempt)
    {
        auto* candidate = donor_nodes[std::rand() % donor_nodes.size()];

        if ((*candidate)->left || (*candidate)->right)
        {
            donor_point = candidate; // internal node found
            found = true;
            break;
        }
    }

    // Fallback: accept any node if no internal node was found
    if (!found)
        donor_point = donor_nodes[std::rand() % donor_nodes.size()];

    // Step 5: replace cut point with a clone of the chosen donor subtree
    *cut_point = clone_tree(*donor_point);

    return child;
}

// ====================================================================
// mutate -- Koza subtree mutation
// ====================================================================
/**
 * @brief Classic subtree mutation (Koza, 1992).
 *
 * With probability `prob`, selects a random node in the tree and
 * replaces the entire subtree rooted there with a freshly generated
 * random tree of depth 2.
 *
 * This differs from point mutation (which only changes a node's value)
 * in that it simultaneously changes both the structure and the values
 * of the replaced subtree, introducing greater structural diversity.
 *
 * @param tree Root of the tree to mutate (modified in-place).
 * @param prob Probability that mutation is applied (default = 0.4 = 40%).
 */
void mutate(std::unique_ptr<Node>& tree, double prob = 0.4)
{
    if (!tree) return;

    // Stochastic gate: skip mutation with probability (1 - prob)
    if ((std::rand() / static_cast<double>(RAND_MAX)) >= prob)
        return;

    // Collect all nodes and pick a random mutation point
    std::vector<std::unique_ptr<Node>*> nodes;
    collect_nodes(tree, nodes);

    auto* mutation_point = nodes[std::rand() % nodes.size()];

    // Replace the subtree at the chosen point with a new random tree
    *mutation_point = random_tree(2);
}

// ====================================================================
// tree_to_string -- infix expression serialiser
// ====================================================================
/**
 * @brief Converts the expression tree to a fully parenthesised infix string.
 *
 * Examples:
 *  - A leaf "x"    --> "x"
 *  - (+, x, 1)    --> "(x+1)"
 *  - (+, (*, x, x), x) --> "((x*x)+x)"
 *
 * @param node Root of the tree (or current subtree in recursion).
 * @return String representation of the expression.
 */
std::string tree_to_string(const std::unique_ptr<Node>& node)
{
    if (!node) return "";
    if (!node->left && !node->right) return node->value; // leaf

    return "("
         + tree_to_string(node->left)
         + node->value
         + tree_to_string(node->right)
         + ")";
}

// ====================================================================
// print_tree -- hierarchical (rotated) tree printer
// ====================================================================
/**
 * @brief Prints the tree rotated 90° counter-clockwise.
 *
 * Right subtree is printed first (appears above in the terminal),
 * then the current node, then the left subtree (appears below).
 * Indentation (4 spaces per level) represents depth.
 *
 * This layout makes it easy to read the tree structure visually:
 * the left-most printed line is the deepest right-most node.
 *
 * @param node  Current node to print.
 * @param level Current depth level (0 for root).
 */
void print_tree(const std::unique_ptr<Node>& node, int level = 0)
{
    if (!node) return;

    print_tree(node->right, level + 1);
    std::cout << std::string(level * 4, ' ') << node->value << "\n";
    print_tree(node->left,  level + 1);
}

// ====================================================================
// main -- evolutionary loop
// ====================================================================
int main()
{
    std::srand(std::time(nullptr)); // seed RNG with current time

    constexpr int POP_SIZE    = 200; // population size
    constexpr int GENERATIONS = 100; // number of evolutionary cycles

    // Open CSV for logging; stores one row per generation
    std::ofstream csv("evolution_fitness.csv");
    csv << "generation,best_fitness,average_fitness,average_size\n";

    // ----------------------------------------------------------------
    // Initialise population with random trees
    // ----------------------------------------------------------------
    std::vector<std::unique_ptr<Node>> population;
    population.reserve(POP_SIZE);

    for (int i = 0; i < POP_SIZE; ++i)
        population.push_back(random_tree());

    // ================================================================
    // Main evolutionary loop
    // ================================================================
    for (int gen = 0; gen < GENERATIONS; ++gen)
    {
        // ------------------------------------------------------------
        // Step 1: Parallel fitness evaluation
        // ------------------------------------------------------------
        // Each individual is evaluated independently, making this
        // embarrassingly parallel -- ideal for std::execution::par_unseq.

        std::vector<std::pair<double, int>> scored(POP_SIZE);
        std::vector<int> indices(POP_SIZE);
        std::iota(indices.begin(), indices.end(), 0); // fill 0..POP_SIZE-1

        std::for_each(
            std::execution::par_unseq,
            indices.begin(), indices.end(),
            [&](int i)
            {
                scored[i] = { fitness(population[i]), i };
            }
        );

        // ------------------------------------------------------------
        // Step 2: Rank individuals by fitness (best = highest value)
        // ------------------------------------------------------------
        // FIX: removed duplicate std::sort that was here in the original.
        // A single sort in descending order is sufficient.
        std::sort(scored.rbegin(), scored.rend());

        // ------------------------------------------------------------
        // Step 3: Compute generation statistics
        // ------------------------------------------------------------
        double sum_fit  = 0.0;
        double sum_size = 0.0;

        for (auto& [f, i] : scored)
        {
            sum_fit  += f;
            sum_size += static_cast<double>(tree_size(population[i]));
        }

        double avg_fit  = sum_fit  / POP_SIZE;
        double avg_size = sum_size / POP_SIZE;

        // Log to CSV
        csv << std::format("{},{:.4f},{:.4f},{:.1f}\n",
                           gen,
                           scored[0].first,
                           avg_fit,
                           avg_size);

        // Print summary to console
        std::cout << std::format(
            "Generation {:2}: Best fitness = {:.2f}, "
            "Average fitness = {:.2f}, "
            "Average size = {:.1f}\n",
            gen, scored[0].first, avg_fit, avg_size);

        std::cout << "Expression: "
                  << tree_to_string(population[scored[0].second]) << "\n";
        std::cout << "Hierarchical tree:\n";
        print_tree(population[scored[0].second]);
        std::cout << std::string(60, '-') << "\n";

        // ------------------------------------------------------------
        // Step 4: Build the next generation
        // ------------------------------------------------------------
        /**
         * Three-stage reproduction strategy:
         *
         * A) Elitism: the single best individual is copied unchanged.
         *    This guarantees that the best solution found so far is
         *    never lost to crossover or mutation.
         *
         * B) Top-30% reproduction: the 60 best individuals are cloned
         *    into the new population, providing a high-quality gene pool
         *    for the crossover step that follows.
         *    NOTE: the elite individual is also included in the top-30%,
         *    so it appears twice -- once from A, once from B. This is
         *    intentional: it slightly biases the gene pool toward the
         *    current best without dominating the population.
         *
         * C) Crossover + mutation: the remaining slots are filled with
         *    children produced by selecting two parents at random from
         *    the entire (current) population, crossing them over, and
         *    then applying subtree mutation with probability 0.4.
         *    Using the full population (not just the top-30%) for parent
         *    selection preserves genetic diversity and helps avoid
         *    premature convergence.
         */

        constexpr int ELITE_N = 1; // individuals preserved unchanged

        std::vector<std::unique_ptr<Node>> new_pop;
        new_pop.reserve(POP_SIZE);

        // A) Elitism
        for (int i = 0; i < ELITE_N; ++i)
            new_pop.push_back(clone_tree(population[scored[i].second]));

        // B) Top-30% reproduction
        int top_k = static_cast<int>(POP_SIZE * 0.3);
        for (int i = 0; i < top_k; ++i)
            new_pop.push_back(clone_tree(population[scored[i].second]));

        // C) Crossover + mutation to fill remaining slots
        while (new_pop.size() < static_cast<std::size_t>(POP_SIZE))
        {
            // Random parent selection from the full population
            const auto& a = population[std::rand() % POP_SIZE];
            const auto& b = population[std::rand() % POP_SIZE];

            auto child = crossover(a, b);
            mutate(child);

            new_pop.push_back(std::move(child));
        }

        // Replace the old population with the new one
        population = std::move(new_pop);
    }

    csv.close();
    return 0;
}