/**
 * @file bitonic_loops.cpp - Bitonic sort done with nested loops, but still sequential--not parallel.
 * @author Kevin Lundeen
 * @see "Seattle University, CPSC5600, Winter 2020"
 */

#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <chrono>
#include <algorithm>
#include <future>

using namespace std;

const int ORDER = 4;       // for bigger arrays make this bigger, but take out the printing below
const int N = 1 << ORDER;  // must be power of 2
typedef vector<int> Data;

/**
 * The bitonic sorter class.
 * @tparam Container is any comparable object
 */
template<typename Container>
class Bitonic {
public:
    Bitonic(Container *data) : data(data), n(data->size()) {}

    void sort() {
        cout << "k\tj\ti\ti^j\ti&k" << endl;

        // k is size of the pieces, starting at pairs and doubling up until we get to the whole array
        // k also determines if we want ascending or descending for each section of i's
        // corresponds to 1<<d in textbook
        for (int k = 2; k <= n; k *= 2) { // k is one bit, marching to the left
            cout << fourbits(k) << "\t";

            // j is the distance between the first and second halves of the merge
            // corresponds to 1<<p in textbook
            for (int j = k / 2; j > 0; j /= 2) {  // j is one bit, marching from k to the right
                if (j != k / 2)
                    cout << "    \t";
                cout << fourbits(j) << "\t";

                // i is the merge element
                for (int i = 0; i < n; i++) {
                    if (i != 0)
                        cout << "    \t    \t";

                    cout << fourbits(i) << "\t";
                    int test = i & k;
                    cout << test;
                    int ixj = i ^j;  // xor: all the bits that are on in one and off in the other
                    cout << fourbits(ixj) << "\t" << fourbits(i & k) << endl;

                    // only compare if ixj is to the right of i
                    if (ixj > i) {
                        if ((i & k) == 0 && (*data)[i] > (*data)[ixj])
                            std::swap((*data)[i], (*data)[ixj]);
                        if ((i & k) != 0 && (*data)[i] < (*data)[ixj])
                            std::swap((*data)[i], (*data)[ixj]);
                    }
                }
            }
        }
    }

    /**
     * Debug helper. Shows the current state of the data array.
     * @param o        where to print
     * @param start    where to start in array
     * @param end      first one not to print in array (-1 for all)
     * @param label    optional suffix on the printout line
     */
    void dump(ostream &o, int start = 0, int end = -1, string label = "") {
        if (end == -1)
            end = n;
        o << "[" << start << ":" << end << "] ";
        for (int i = start; i < end; i++)
            o << (*data)[i] << " ";
        o << label << endl;
    }

    /**
     * Helper for printing out bits. Converts the last four bits of the given number to a string of 0's and 1's.
     * @param n number to convert to a string (only last four bits are observed)
     * @return four-character string of 0's and 1's
     */
    static string fourbits(int n) {
        string ret = /*to_string(n) + */(n > 15 ? "/1" : "/");
        for (int bit = 3; bit >= 0; bit--)
            ret += (n & 1 << bit) ? "1" : "0";
        return ret;
    }

private:
    Container *data;
    int n;
};


void fillRandom(Data &v, int lo, int hi) {
    uniform_int_distribution<int> dist(lo, hi);
    random_device rd;
    mt19937 source(rd());
    for (int i = v.size() - 1; i >= 0; i--)
        v[i] = dist(source);
}

int main() {
    Data data(N, 0);

    // try STL's sort first for comparison
    fillRandom(data, 0, N);
    auto start1 = chrono::steady_clock::now();
    sort(data.begin(), data.end());
    // for (int i = 0; i < N; i++) {
    //     cout << data[i] << " ";
    // }
    auto end1 = chrono::steady_clock::now();
    auto elapsed1 = chrono::duration<double, milli>(end1 - start1).count();
    cout << "default sort (probably quicksort) in " << elapsed1 << "ms" << endl;

    fillRandom(data, 0, N);

    // start timer
    auto start = chrono::steady_clock::now();

    Bitonic<Data> bitonic(&data);
    bitonic.sort();

    // stop timer
    auto end = chrono::steady_clock::now();
    auto elapsed = chrono::duration<double, milli>(end - start).count();

    //bitonic.dump(cout);
    int check = -1;
    for (int elem: data) {
        if (elem < check) {
            cout << "FAILED RESULT at " << check << endl;
            break;
        }
        check = elem;
    }
    cout << "in " << elapsed << "ms" << endl;
    return 0;
}
