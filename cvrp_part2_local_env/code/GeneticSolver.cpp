#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <vector>
#include <queue>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include "Solver.cpp"
using namespace std;


bool solution_compare(const Solution& a, const Solution& b) {
    return a.score < b.score;
}



class GeneticSolver: public Solver {
public:
    vector<Solution> population;

    int population_size;
    int elite_size;
    int initialization_rounds;
    int improvement_rounds;
    double mutation_rate;
    int number_of_mutations;
    double immigrant_rate;

    GeneticSolver(const string &depot, const string &parcel_filename, const string &
    time_filename, const string &output_filename, int population_size, int elite_size, int
                  initialization_rounds, int improvement_rounds, double mutation_rate,
                  int number_of_mutations,
                  double immigrant_rate) : Solver(depot, parcel_filename, time_filename,
                                                  output_filename),
                                           population_size(population_size), elite_size(elite_size),
                                           initialization_rounds(initialization_rounds),
                                           improvement_rounds(improvement_rounds), mutation_rate
                                                   (mutation_rate),
                                           number_of_mutations(number_of_mutations),
                                           immigrant_rate(immigrant_rate) {
    }

    struct SolutionCompareType {
        bool operator()(const Solution& a, const Solution& b){
            return a.score < b.score;
        }
    };


    // generates initialization_rounds many solutions, then choose
    // the best population_size of them, ranks them from fittest to
    // the most unfit
    void generate_population() {
        vector<Solution> result;
        result.reserve(this->population_size);
        priority_queue<Solution, vector<Solution>, SolutionCompareType> max_heap;
        for (int i = 0; i < this->initialization_rounds; ++i) {
            Solution sol = generate_solution();
            sol.score = compute_score(sol);
            if (max_heap.size() < this->population_size) {
                max_heap.push(sol);
            } else if (sol.score < max_heap.top().score) {
                max_heap.pop();
                max_heap.push(sol);
            }
        }
        while (!max_heap.empty()) {
            result.emplace_back(max_heap.top());
            max_heap.pop();
        }
        reverse(result.begin(), result.end());
//        for (Solution sol : result) {
//            cout << "score: " << sol.score << endl;
//        }
        this->population = result;
    }
    // The best elite_size many individuals are selected directly.
    // For the rest in the population, each one is selected with
    // probability based on its score
    vector<Solution> select(const vector<Solution>& ranked_population) {
        vector<double> roulette_wheel = {0.0};
        roulette_wheel.reserve(1 + ranked_population.size());
        double highest_score = ranked_population[ranked_population.size() - 1].score + 500;
        double cumulative_score = 0.0;
        for (const Solution& sol : ranked_population) {
            cumulative_score += (highest_score - sol.score);
            roulette_wheel.emplace_back(cumulative_score);
        }

        // the ith individual in the population will be selected
        // if the number drawn is in [roulette_wheel[i - 1], roulette_wheel[i]]
        vector<Solution> selected;
        unordered_set<int> selected_index;
        for (int i = 0; i < this->elite_size; ++i) {
            selected.emplace_back(ranked_population[i]);
            selected_index.insert(i);
        }

        // ??? how many parents?
        for  (int i = this->elite_size; i < ranked_population.size(); ++i) {
            int random = rand() % (int)ceil(roulette_wheel[roulette_wheel.size() - 1]);
            for (int j = 0; j < roulette_wheel.size() - 1; ++j) {
                if (random >= roulette_wheel[j] && random < roulette_wheel[j + 1]) {
                    if (selected_index.find(j) != selected_index.end()) {
                        break;
                    } else {
                        selected.emplace_back(ranked_population[i]);
                    }
                }
            }
        }
        // we don't need to sort here because the elites are already at the front
//        sort(selected.begin(), selected.end(), solution_compare);
//        for (const Solution& sol: selected) {
//            cout << "selected score: " << sol.score << endl;
//        }
        return selected;
    }

    // take the first n routes from parent1, then put in the rest
    // of the parcels in the order of parent2
    // parents are Solution objects
    Solution OX_breed(const Solution& parent1, const Solution& parent2) {
        vector<Route> child_gene;
        unordered_set<Parcel, ParcelHash> added_parcels;

        int cut_index = rand() % parent1.gene.size();
//        cout << "chosen cut index is " << cut_index << "/" << parent1.gene.size() << endl;
        for (int i = 0; i <= cut_index; ++i) {
            child_gene.emplace_back(parent1.gene[i]);
            for (const Parcel& p : parent1.gene[i].loads) {
                added_parcels.insert(p);
            }
        }

        for (const Route& parent_route : parent2.gene) {
            Route child_route;
            for (const Parcel& p : parent_route.loads) {
                if (added_parcels.find(p) == added_parcels.end()) {
                    child_route.add(p);
                    added_parcels.insert(p);
                }
            }
            if (!child_route.loads.empty()) {
                child_gene.emplace_back(child_route);
            }
        }

        return Solution(child_gene);
    }

    struct ParentsHasher {
        size_t operator()(const vector<int>& a) const {
            return hash<int>()(a[0]) ^ hash<int>()(a[1]);
        }
    };

    // elite_size of ranked_selected goes directly into the
    // next generation, others are children of the selected parents
    void breed_population(const vector<Solution>& ranked_selected) {
        vector<Solution> next_generation;
        next_generation.reserve(this->population_size);
//        unordered_set<vector<int>, ParentsHasher> used_parents;
        for (int i = 0; i < this->elite_size; ++i) {
            next_generation.emplace_back(ranked_selected[i]);
        }

        while (next_generation.size() < this->population_size) {
            int idx1 = rand() % ranked_selected.size();
            int idx2 = rand() % ranked_selected.size();
            while (idx2 == idx1) {
                idx2 = rand() % ranked_selected.size();
            }

//            // check if this pair of parents have been used
//            vector<int> parent_indices = {idx1, idx2};
//            if (used_parents.find(parent_indices) != used_parents.end()) {
//                continue;
//            }

//            cout << "chosen parents are (" << idx1 << ", " << idx2 << ")" << endl;
            Solution child = OX_breed(ranked_selected[idx1], ranked_selected[idx2]);
            next_generation.emplace_back(child);
        }
        this->population = next_generation;
    }

    void mutate(Solution& solution) {
        swap_parcels(solution, this->number_of_mutations);
    }

    void mutate_population(vector<Solution>& next_generation) {
        // only mutate those non-elites
        for (int i = this->elite_size; i < next_generation.size(); ++i) {
            double draw = random_double(0.0, 1.0);
            if (draw < this->mutation_rate) {
                mutate(next_generation[i]);
            }
        }

        // no need to sort here either
        // sort(next_generation.begin(), next_generation.end(), solution_compare);
    }

    // replace some unfit individuals with newly_generated immigrants
    void introduce_immigrants() {
        int number_of_immigrants = ceil(this->immigrant_rate * this->population_size);
        for (int i = this->population_size - number_of_immigrants; i < this->population_size; ++i) {
            this->population[i] = generate_solution();
        }
        sort(this->population.begin(), this->population.end(), solution_compare);
    }

    void improve_population(int number_of_rounds) {
        for (Solution& sol : this->population) {
            combine_routes(sol, 5 * number_of_rounds);
//            split_routes(sol, 5 * number_of_rounds);
            swap_parcels(sol, 10 * number_of_rounds);
            reorder_routes(sol);
        }
    }


    void evolve() {

        introduce_immigrants();
        vector<Solution> ranked_selected = select(this->population);
        breed_population(ranked_selected);
        mutate_population(this->population);
        improve_population(this->improvement_rounds);

        // compute scores and sort
        for (Solution& sol : this->population) {
            if (sol.score == 0) {
                sol.score = compute_score(sol);
            }
        }
        sort(this->population.begin(), this->population.end(), solution_compare);
    }



};





