#include <unordered_set>
#include <vector>
using namespace std;

struct Hasher {
    size_t operator()(const vector<int>& a) const {
        return hash<int>()(a[0]) ^ hash<int>()(a[1]);
    }
};

int main() {
    unordered_set<vector<int>, Hasher> used;
    vector<int> a = {1, 2};
    used.insert(a);
}