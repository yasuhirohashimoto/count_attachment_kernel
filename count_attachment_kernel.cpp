#include <boost/algorithm/string.hpp>
#include <fstream>
#include <iostream>
#include <limits>
#include <unordered_map>
#include <vector>

using namespace std;

int main(int argc, char** argv)
{
    if (argc < 2) {
        cerr << "usage: cat file(tsv) | a.out column\n"
                "# In @file(tsv), each line corresponds to one time step,"
                "# and entries after @column are considered node-ids that obtain edges."
             << endl;
        exit(1);
    }

    size_t const column = stoull(argv[1]);

    uint64_t t = 0;                // time (= number of lines read).
    double N = 0;                  // cumulative number of nodes at time t.
    double E = 0;                  // cumulative number of edges at time t.
    vector<double> n;              // number of nodes in k-th class at time t.
    vector<uint64_t> k;            // class of each node at time t.
    vector<double> A;              // attachment kernel.
    vector<double> w(A.size(), 0); // normalization factor for each class.

    for (string line; getline(cin, line);) {

        boost::trim(line);
        if (line.empty() || line[0] == '#') continue; // skip empty lines or lines starting with a '#'.

        ++t;

        if (t % 10000 == 0) {
            cerr << t << " (" << A.size() << ")" << endl;
        }

        vector<string> tok;
        boost::algorithm::split(tok, line, [](char c) { return c == '\t'; });
        assert(column < tok.size());

        vector<uint64_t> nodes;            // selected nodes.
        unordered_map<uint64_t, double> m; // number of selections of each class.
        double const mt = tok.size() - column;
		E += mt;

        double N_tmp = N;

        // preprocessing.
        for (auto it = tok.begin() + column; it != tok.end(); ++it) {

            uint64_t const ni = stoull(*it);
            nodes.push_back(ni);

            if (k.size() <= ni) k.resize(ni + 1, 0);

            uint64_t const& ki = k[ni];
            ++m[ki];
        }

        // Attachment kernel is updated.
        for (auto const& ni : nodes) {

            uint64_t const& ki = k[ni];

            if (ki == 0) {
                ++N_tmp; // add a new node.
            } else {
                A[ki] += m[ki] * N / n[ki];
            }
        }

        N = N_tmp;

        // Classes to which each node belongs are updated.
        for (auto const& ni : nodes) {

            uint64_t const& ki = ++k[ni];

            // In addition, the sizes of some arrays are extended if needed.
            if (ki >= n.size()) {
                size_t const sz = ki + 2;
                n.resize(sz, 0);
                A.resize(sz, 0);
                w.resize(sz, 0);
            }
        }

        // Normalization factor is updated.
        for (size_t k = 0; k < A.size(); ++k) {
            if (n[k] > 0) w[k] += mt;
        }

        // The number of nodes in each class is updated.
        for (auto const& [k, mk] : m) {
            if (k > 0) n[k] -= mk;
            n[k + 1] += mk;
        }
    }

    cout.precision(numeric_limits<double>::digits10);

    cout << "# T : " << t << "\n"
         << "# N : " << N << "\n"
         << "# E : " << E << "\n\n"
         << "# column1: k\n"
         << "# column2: A_k (with Thong Pham's correction)\n"
         << "# column3: A_k (without correction)\n"
         << endl;

    double const A1 = A[1] / w[1];

    for (size_t k = 1; k < A.size(); ++k) {
        if (w[k] > 0) {
            cout << k
                 << "\t" << A[k] / (A1 * w[k])
                 << "\t" << A[k] / A[1]
                 << endl;
        }
    }

    return 0;
}
