#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cmath>
#include <chrono>
#include <algorithm>
#include <random>
#include <iostream>
#include <vector>
#include <sstream>
#include <unordered_map>
using namespace std;

int compute_score_count = 0;
int reorder_single_count = 0;

typedef unordered_map<string, unordered_map<string, double>> dist_matrix;
typedef unordered_map<string, vector<pair<string, double>>> ordered_dist_matrix;

double random_double(double lower_bound, double upper_bound) {
    random_device rd;
    uniform_real_distribution<double> dist(lower_bound, upper_bound);
    default_random_engine re(rd());
    return dist(re);
}

int random_int(int lower_bound, int upper_bound) {
    random_device rd;
    uniform_int_distribution<int> dist(lower_bound, upper_bound);
    default_random_engine re(rd());
    return dist(re);
}

class Parcel {
public:
    string store_name;
    int load_index = 0;
    double vol_1 = 0, vol_2 = 0;

    Parcel() = default;
    Parcel(string store_name, int load_index, double vol_1, double vol_2) : store_name(store_name), load_index(load_index), vol_1(vol_1), vol_2(vol_2) {}

    bool operator==(const Parcel& other) const {
        return (this->store_name == other.store_name) && (this->load_index == other.load_index);
    }

};

struct ParcelHash {
    size_t operator()(const Parcel& p) const {
        return hash<string>()(p.store_name) ^ hash<int>()(p.load_index);
    }
};

class Route {
public:
    vector<Parcel> loads;
    double total_vol_1, total_vol_2;

    Route() {
        total_vol_1 = total_vol_2 = 0.0;
    }

    void add(const Parcel& p) {
        loads.emplace_back(p);
        total_vol_1 += p.vol_1;
        total_vol_2 += p.vol_2;
    }

    void remove(int idx) {
        if (idx < 0 || idx >= this->loads.size()) {
            cout << "Invalid index for removing Parcel from Route!" << endl;
            exit(0);
        }
        this->total_vol_1 -= this->loads[idx].vol_1;
        this->total_vol_2 -= this->loads[idx].vol_2;
        this->loads.erase(this->loads.begin() + idx);
    }

    string to_string(const string& depot) {
        stringstream out;
        out << "{" << endl;
        out << "    \"route\": [\"" << depot << "\"";
        for (Parcel load : loads) {
            out << ", \"" << load.store_name << "\"";
        }
        int k = (int)ceil(max(total_vol_1, total_vol_2));
        out << "], " << endl;
        out << "    \"k\" : " << k << ", " << endl;
        out << "    \"loads\": [";
        bool first = true;
        for (const Parcel& load : loads) {
            if (first) {
                first = false;
            } else {
                out << ", ";
            }
            out << "{\"node\": \"" << load.store_name << "\", \"load_idx_in_node\" : " << load.load_index << "}";
        }
        out << "]" << endl;
        out << "}" << endl;
        return out.str();
    }
};

class Solution {
public:
    vector<Route> gene;
    double score = 0.0;

    Solution() = default;
    explicit Solution(const vector<Route>& gene) : gene(gene) {
    }
};

class Solver {
public:

    string depot;
    string parcel_filename;
    string time_filename;
    string output_filename;
    vector<Parcel> parcels;
    dist_matrix distance_matrix;
    ordered_dist_matrix ordered_distance_matrix;
    unordered_map<string, vector<Parcel>> store_to_parcels;
    unordered_map<string, Route> route_map; // store_name --> Route mapping
    vector<string> store_names;




    Solver(const string& depot, const string& parcel_filename, const string& time_filename,
            const string& output_filename): depot(depot), parcel_filename(parcel_filename),
            time_filename(time_filename), output_filename(output_filename) {

        cout << "loading parcels..." << endl;
        load_parcels(parcel_filename);
        cout << "loading time matrix..." << endl;
        distance_matrix = load_time(time_filename);

        // order neighbors by distance
        for (const auto& map1 : distance_matrix) {
            string source = map1.first;
            vector<pair<string, double>> v;
            ordered_distance_matrix.insert({source, v});
            for (const auto& map2 :map1.second) {
                ordered_distance_matrix[source].emplace_back(map2);
            }
            sort(ordered_distance_matrix[source].begin(), ordered_distance_matrix[source]
            .end(), [](pair<string, double> a, pair<string, double> b) {
                return a.second < b.second;
            });
        }


        // build store to parcels mapping
        for (const Parcel& p : parcels) {
            if (store_to_parcels.find(p.store_name) == store_to_parcels.end()) {
                vector<Parcel> v;
                store_to_parcels.insert({p.store_name, v});
            }
            store_to_parcels[p.store_name].emplace_back(p);
        }

        // build the store_name -> Route mapping for close neighbors greedy
        unordered_map<string, Route> route_map; // store_name --> Route mapping
        vector<string> store_names;

        for (const auto& pair: this->store_to_parcels) {
            if (pair.first != this->depot) {
                store_names.emplace_back(pair.first);
            }
            Route route;
            for (const Parcel& p : pair.second) {
                route.add(p);
            }
            route_map.insert({pair.first, route});
        }
        this->route_map = route_map;
        this->store_names = store_names;
    }

    double compute_route_time(const Route& route) {
        double total_time = 0.0;
        for (size_t i = 0; i < route.loads.size() - 1; ++i) {
            string src = route.loads[i].store_name;
            string dest = route.loads[i + 1].store_name;
            total_time += this->distance_matrix[src][dest];
        }
        total_time += this->distance_matrix[this->depot][route.loads[0].store_name];
        total_time += this->distance_matrix[route.loads[route.loads.size() - 1].store_name][this->depot];
        return total_time;
    }

    double compute_route_cost(const Route& route) {
        double route_time = compute_route_time(route);
        int route_truck_size = ceil(max(route.total_vol_1, route.total_vol_2));
        return route_time * route_truck_size;
    }

    double compute_score(const Solution& sol) {
        compute_score_count++;
        auto start = chrono::high_resolution_clock::now();
        double total_cost = 0.0;
        for (const Route& route : sol.gene) {
            total_cost += compute_route_cost(route);
        }
        auto end = chrono::high_resolution_clock::now();
        double time_taken = chrono::duration_cast<chrono::nanoseconds>(end - start).count();
//        cout << "compute score took " << time_taken << " nanoseconds" << endl;
        return total_cost;
    }

    Solution generate_solution() {
        return close_neighbors_combine_greedy(10);
    }

    // determine if two routes cost less when concatenated
    bool should_combine(const Route& route1, const Route& route2) {
        string route1_end = route1.loads[route1.loads.size() - 1].store_name;
        string route2_start = route2.loads[route2.loads.size() - 1].store_name;
        double old_transition_cost = this->distance_matrix[route1_end][this->depot] +
                this->distance_matrix[this->depot][route2_start];
        double new_transition_cost = this->distance_matrix[route1_end][route2_start];

        double route1_time = compute_route_time(route1);
        double route2_time = compute_route_time(route2);
        double old_cost = route1_time * ceil(max(route1.total_vol_1, route1.total_vol_2)) +
                route2_time * ceil(max(route2.total_vol_1, route2.total_vol_2));
        double new_time = route1_time + route2_time - old_transition_cost + new_transition_cost;
        double new_truck_size = ceil(max(route1.total_vol_1 + route2.total_vol_1, route1
        .total_vol_2 + route2.total_vol_2));
        double new_cost = new_time * new_truck_size;
        return new_cost < old_cost;
    }

    /* first build a route for every store, then look at up to
     * a certain number of neighbors' routes and see if they are
     * better combined together
     */
    Solution close_neighbors_combine_greedy(int number_of_neighbors) {
        unordered_set<string> used_stores;


        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        auto rng = default_random_engine(seed);
        shuffle(this->store_names.begin(), this->store_names.end(), rng);

        Solution sol;
        for (const string& store_name: this->store_names) {
            if (used_stores.find(store_name) != used_stores.end()) {
                continue;
            }
            used_stores.insert(store_name);

            // build a route by adding all parcels from a store at a time
            Route route = this->route_map[store_name];
            int neighbors_count = this->ordered_distance_matrix[store_name].size() - 1;
            for (int i = 1; i < min(number_of_neighbors + 1, neighbors_count); ++i) {
                string neighbor = this->ordered_distance_matrix[store_name][i].first;
                Route neighbor_route = this->route_map[neighbor];
                if (neighbor == this->depot || used_stores.find(neighbor) != used_stores.end()) {
                    continue;
                } else if (should_combine(route, neighbor_route)) {
                    for (const Parcel& p : neighbor_route.loads) {
                        route.add(p);
                    }
                    used_stores.insert(neighbor);
                }
            }
            sol.gene.emplace_back(route);
        }
        return sol;
    }


    dist_matrix load_time(const string& filename) {
        dist_matrix dist;
        FILE* in = fopen(filename.c_str(), "r");
        int depth = 0, inside = 0;
        string src, name, value_buffer;
        for (char ch; (ch = fgetc(in)) != EOF;) {
            if (ch == '{') {
                ++ depth;
            } else if (ch == '}') {
                -- depth;
            } else if (ch == '"') {
                inside ^= 1;
                if (inside == 1) {
                    name = "";
                    value_buffer = "";
                } else {
                    if (depth == 1) {
                        src = name;
                    }
                }
            } else if (inside == 1) {
                name += ch;
            } else if (isdigit(ch) || ch == '.') {
                value_buffer += ch;
            } else if (ch == ',') {
                double value;
                stringstream sin(value_buffer);
                sin >> value;
                dist[src][name] = value;
            }
        }
        fclose(in);
        return dist;
    }

    void load_parcels(const string& filename) {
        cout << "opening parcels file..." << endl;
        FILE* in = fopen(filename.c_str(), "r");
        int depth = 0;
        unsigned int inside = 0;
        string src, name, value_buffer;
        Parcel p;
        cout << "start reading parcel file..." << endl;
        for (char ch; (ch = fgetc(in)) != EOF;) {
            if (ch == '{') {
                ++ depth;
            } else if (ch == '}') {
                -- depth;
            } else if (ch == '"') {
                inside ^= 1;
                if (inside == 1) {
                    name = "";
                    value_buffer = "";
                } else {
                    if (depth == 1) {
                        p.store_name = name;
                        p.load_index = 0;
                    }
                }
            } else if (inside == 1) {
                name += ch;
            } else if (isdigit(ch) || ch == '.' || ch == 'e' || ch == 'E' || ch == '-') {
                value_buffer += ch;
            } else if (ch == ',' || ch == ']' || ch == '}') {
                if (value_buffer != "") {
                    stringstream sin(value_buffer);
                    if (name == "vol_1") {
                        sin >> p.vol_1;
                    } else if (name == "vol_2") {
                        sin >> p.vol_2;
                        this->parcels.emplace_back(p);
                        ++ p.load_index;
                    } else {
                        assert(false);
                    }
                    value_buffer = "";
                }
            }
        }
        fclose(in);
    }

    void write_to_file(const Solution& sol) {
        vector<Route> routes = sol.gene;
        FILE *out = fopen(this->output_filename.c_str(), "w");
        fprintf(out, "[");
        bool first = true;
        for (Route r : routes) {
            if (first) {
                first = false;
            } else {
                fprintf(out, ", ");
            }
            fprintf(out, "%s", r.to_string(this->depot).c_str());
        }
        fprintf(out, "]");
        fclose(out);
    }

    void swap(Route& route, int idx1, int idx2) {
        auto start = chrono::high_resolution_clock::now();
        std::swap(route.loads[idx1], route.loads[idx2]);
    }

    void reorder_single_route(Route& route) {
        Route new_route = route;

        // take the stop closest to depot to be the first
        double min_time = this->distance_matrix[this->depot][new_route.loads[0].store_name];
        int min_index = 0;
        for (int i = 1; i < new_route.loads.size(); ++i) {
            double new_time = this->distance_matrix[this->depot][new_route.loads[i].store_name];
            if (new_time < min_time) {
                min_time = new_time;
                min_index = i;
            }
        }
        swap(new_route, 0, min_index);

        // greedily build the route by choose the next closest stop
        int curr_index = 0;
        while (curr_index < max(0, (int)new_route.loads.size() - 2)) {
            min_time = this->distance_matrix[new_route.loads[curr_index].store_name][new_route.loads[curr_index + 1].store_name];
            min_index = curr_index + 1;
            for (int j = curr_index + 2; j < new_route.loads.size(); ++j) {
                double new_time = this->distance_matrix[new_route.loads[curr_index]
                                                        .store_name][new_route.loads[j].store_name];
                if (new_time < min_time) {
                    min_time = new_time;
                    min_index = j;
                }
            }
            swap(new_route, curr_index + 1, min_index);
            curr_index++;
        }

        // check if the ordered route is better
        double old_cost = compute_route_cost(route);
        double new_cost = compute_route_cost(new_route);
        if (new_cost < old_cost) {
            std::swap(new_route, route);
        }
    }

    void reorder_routes(Solution& sol) {
        for (Route& route : sol.gene) {
            reorder_single_route(route);
        }
    }

    // flips the segment [start, end] in route
    void flip(Route& route, int start, int end) {
        while (start < end) {
            std::swap(route.loads[start], route.loads[end]);
            start++;
            end--;
        }
    }


//    // 2-opt improvement of a single route
//    void two_opt(Route& route, int rounds) {
//
//        if (route.loads.size() <= 2) {
//            return;
//        }
//        auto start_time = chrono::high_resolution_clock::now();
//        for (int i = 0 ; i < rounds; ++i) {
//            int len = route.loads.size();
//            int idx1 = random_int(0, len - 1);
//            int idx2 = random_int(0, len - 1);
//            while (idx2 == idx1) {
//                idx2 = random_int(0, len - 1);
//            }
//
//            int start = min(idx1, idx2);
//            int end = max(idx1, idx2);
//            if (start == 0 && end == len - 1) {
//                continue;
//            }
//
//
//            // check if flipping the segment [idx1, idx2] gives a better result
//            double old_cost = 0.0, new_cost = 0.0;
//            if (start > 0) {
//                old_cost += this->distance_matrix[route.loads[start - 1].store_name][route.loads[start].store_name];
//                new_cost += this->distance_matrix[route.loads[start - 1].store_name][route.loads[end].store_name];
//            }
//            if (end < len - 1) {
//                old_cost += this->distance_matrix[route.loads[end].store_name][route
//                        .loads[end + 1].store_name];
//                new_cost += this->distance_matrix[route.loads[start].store_name][route
//                        .loads[end + 1].store_name];
//            }
//            if (new_cost < old_cost) {
//                flip(route, start, end);
//            }
//        }
//        auto end_time = chrono::high_resolution_clock::now();
//        double time_taken = chrono::duration_cast<chrono::nanoseconds>(end_time - start_time)
//                .count();
////        cout << "2 opt took " << time_taken << " nanoseconds" << endl;
//    }
//
//    void two_opt_routes(Solution& sol, int rounds) {
//        for (int i = 0; i < sol.gene.size(); ++i) {
//            two_opt(sol.gene[i], rounds);
//        }
//    }




    // determine if two routes should be combined
    // if so, store the combined route into route1
    bool should_two_routes_combine(Route& route1, Route& route2) {
        double old_cost = compute_route_cost(route1) + compute_route_cost(route2);
        Route combined_route;
        for (const Parcel& p : route1.loads) {
            combined_route.add(p);
        }
        for (const Parcel& p : route2.loads) {
            combined_route.add(p);
        }
        double new_cost = compute_route_cost(combined_route);
        if (new_cost < old_cost) {
            route1 = combined_route;
            return true;
        } else {
            return false;
        }
    }


    void combine_routes(Solution& sol, int count) {
        for (int i = 0; i < count; ++i) {
            int idx1 = random_int(0, sol.gene.size() - 1);
            int idx2 = random_int(0, sol.gene.size() - 1);
            while (idx2 == idx1) {
                idx2 = random_int(0, sol.gene.size() - 1);
            }


            if (should_two_routes_combine(sol.gene[idx1], sol.gene[idx2])) {
                sol.gene.erase(sol.gene.begin() + idx2);
            }
        }
    }


    // determine if the original_route should be split into [0, cut_index]
    // and [cut_index, end]. If so, store the first half in original_route
    // , second half in split_route
    bool should_split(Route& original_route, int cut_index, Route& split_route) {
        double old_cost = compute_route_cost(original_route);
        Route first_half, second_half;
        for (int i = 0; i <= cut_index; ++i) {
            first_half.add(original_route.loads[i]);
        }
        for (int i = cut_index + 1; i < original_route.loads.size(); ++i) {
            second_half.add(original_route.loads[i]);
        }
        double new_cost = compute_route_cost(first_half) + compute_route_cost(second_half);
        if (new_cost < old_cost) {
            original_route = first_half;
            split_route = second_half;
            return true;
        } else {
            return false;
        }

    }

    // for each route in the solution, try to split it into two
    void split_routes(Solution& sol, int rounds) {
        for (int j = 0; j < rounds; ++j) {
            for (int i = sol.gene.size() - 1; i >= 0; --i) {
                if (sol.gene[i].loads.size() < 2) {
                    continue;
                }
                int cut_index = random_int(0, sol.gene[i].loads.size() - 2);
                Route split_route;
                if (should_split(sol.gene[i], cut_index, split_route)) {
                    sol.gene.emplace_back(split_route);
                }
            }
        }
    }



    // recompute score after swapping
    void swap_parcels(Solution& solution, int number_of_swaps) {
        for (int i = 0; i < number_of_swaps; ++i) {
            // choose which two routes to swap
            int r_idx1 = random_int(0, solution.gene.size() - 1);
            int r_idx2 = random_int(0, solution.gene.size() - 1);
            while (r_idx2 == r_idx1) {
                r_idx2 = random_int(0, solution.gene.size() - 1);
            }

            Route new_route1 = solution.gene[r_idx1];
            Route new_route2 = solution.gene[r_idx2];

            // choose which two parcels to swap
            int p_idx1 = random_int(0, new_route1.loads.size() - 1);
            int p_idx2 = random_int(0, new_route2.loads.size() - 1);
            Parcel p1 = new_route1.loads[p_idx1];
            Parcel p2 = new_route2.loads[p_idx2];
            new_route1.add(p2);
            new_route2.add(p1);
            new_route1.remove(p_idx1);
            new_route2.remove(p_idx2);

            // reorder parcels within the routes
            reorder_single_route(new_route1);
            reorder_single_route(new_route2);

            // check if the result off swapping is better
            double old_cost = compute_route_cost(solution.gene[r_idx1]) +
                              compute_route_cost(solution.gene[r_idx2]);
            double new_cost = compute_route_cost(new_route1) + compute_route_cost(new_route2);

            if (new_cost < old_cost) {
                std::swap(solution.gene[r_idx1], new_route1);
                std::swap(solution.gene[r_idx2], new_route2);
            }
        }
        solution.score = compute_score(solution);
    }
};






