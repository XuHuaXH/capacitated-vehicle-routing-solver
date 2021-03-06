#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cmath>
#include <ctime>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <unordered_map>
#include "GeneticSolver.cpp"
using namespace std;


void write_to_file(string s) {
    ofstream outfile;
    outfile.open("results.txt", ios_base::app);
    outfile << s << endl;
    outfile.close();
}





int main(int argc, char* argv[])
{
    const string DEPOT = argv[1];
    const string TIME_FILE = argv[2];
    const string PICKUP_FILE = argv[3];
    const string OUTPUT_ROUTES = argv[4];

    GeneticSolver solver(DEPOT, PICKUP_FILE, TIME_FILE, OUTPUT_ROUTES, 12, 5, 50, 1, 0.03, 0, 0.3);

    vector<Parcel> parcels = solver.parcels;

    double max_size = 0, total_vol_1 = 0, total_vol_2 = 0;
    for (const Parcel& p : parcels) {
        max_size = max(max_size, p.vol_1);
        max_size = max(max_size, p.vol_2);
        total_vol_1 += p.vol_1;
        total_vol_2 += p.vol_2;
    }

    fprintf(stderr, "max size = %.6f\n", max_size);
    fprintf(stderr, "total vol 1 = %.6f\n", total_vol_1);
    fprintf(stderr, "total vol 2 = %.6f\n", total_vol_2);


    solver.generate_population();

    time_t start_time = time(nullptr);
    time_t last_time = start_time;
    double last_score = solver.population[0].score;

    for (int i = 0; i < 8001; ++i) {
        if (i == 0) {
            write_to_file(to_string(solver.population[0].score));
        }

        if (i > 0 && i % 10 == 0) {
            solver.mutation_rate += 0.1;
        }

        if (i > 0 && i % 30 == 0) {
            solver.number_of_mutations += 100;
        }

        solver.evolve();


        time_t curr_time = time(nullptr);
        if (curr_time - start_time > 500) {
            solver.write_to_file(solver.population[0]);
            write_to_file(to_string(solver.population[0].score));
            return 0;
        }
        if (curr_time > last_time) {
            double curr_score = solver.population[0].score;
            cout << "improvement: " << (last_score - curr_score) << " per second" << endl;
            last_time = curr_time;
            last_score = curr_score;
        }
        cout << "the best score for generation " << i << " is: " << solver.population[0].score <<
        endl;
    }

    solver.write_to_file(solver.population[0]);


    return 0;
}
