#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <vector>
#include <sstream>
#include <unordered_map>
using namespace std;

typedef unordered_map<string, unordered_map<string, double> > DIST_MATRIX;

struct Parcel
{
    string store_name;
    int load_index;
    double vol_1, vol_2;

    Parcel() {}
    Parcel(string store_name, int load_index, double vol_1, double vol_2) : store_name(store_name), load_index(load_index), vol_1(vol_1), vol_2(vol_2) {}
};

struct Route
{
    vector<Parcel> loads;
    double total_vol_1, total_vol_2;

    Route() {
        total_vol_1 = total_vol_2 = 0;
    }

    void add(Parcel p) {
        loads.push_back(p);
        total_vol_1 += p.vol_1;
        total_vol_2 += p.vol_2;
    }

    string to_string(string depot) {
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
        for (Parcel load : loads) {
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

DIST_MATRIX load_time(string filename)
{
    DIST_MATRIX dist;
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
// fprintf(stderr, "%s --> %s: %s\n", src.c_str(), name.c_str(), value_buffer.c_str());
            double value;
            stringstream sin(value_buffer);
            sin >> value;
            dist[src][name] = value;
        }
    }
    fclose(in);
    return dist;
}

vector<Parcel> load_parcels(string filename)
{
    vector<Parcel> parcels;
    FILE* in = fopen(filename.c_str(), "r");
    int depth = 0, inside = 0;
    string src, name, value_buffer;
    Parcel p;
    for (char ch; (ch = fgetc(in)) != EOF;) {
// fprintf(stderr, "%c", ch);
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
                    parcels.push_back(p);
                    ++ p.load_index;
                } else {
                    assert(false);
                }
                value_buffer = "";
            }
        }
    }
    fclose(in);
    return parcels;
}

int main(int argc, char* argv[])
{
    const string DEPOT = argv[1];
    const string TIME_FILE = argv[2];
    const string PICKUP_FILE = argv[3];
    const string OUTPUT_ROUTES = argv[4];

    DIST_MATRIX dist = load_time(TIME_FILE);
    vector<Parcel> parcels = load_parcels(PICKUP_FILE);

    double max_size = 0, total_vol_1 = 0, total_vol_2 = 0;
    for (Parcel p : parcels) {
        max_size = max(max_size, p.vol_1);
        max_size = max(max_size, p.vol_2);
        total_vol_1 += p.vol_1;
        total_vol_2 += p.vol_2;
    }

    fprintf(stderr, "max size = %.6f\n", max_size);
    fprintf(stderr, "total vol 1 = %.6f\n", total_vol_1);
    fprintf(stderr, "total vol 2 = %.6f\n", total_vol_2);

    vector<Route> routes;
    int truck_cap = int(max_size) + 1;
    for (int i = 0; i < parcels.size(); ++ i) {
        int j = i;
        Route r;
        while (j < parcels.size() &&
               r.total_vol_1 + parcels[j].vol_1 <= truck_cap &&
               r.total_vol_2 + parcels[j].vol_2 <= truck_cap) {
            r.add(parcels[j++]);
        }
        routes.push_back(r);
        i = j - 1;
    }

    FILE* out = fopen(OUTPUT_ROUTES.c_str(), "w");
    fprintf(out, "[");
    bool first = true;
    for (Route r : routes) {
        if (first) {
            first = false;
        } else {
            fprintf(out, ", ");
        }
        fprintf(out, "%s", r.to_string(DEPOT).c_str());
    }
    fprintf(out, "]");
    fclose(out);

    return 0;
}
