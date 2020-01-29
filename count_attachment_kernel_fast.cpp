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
                "# and entries after @column are considered node-ids that obtain edges at that time step."
             << endl;
        exit(1);
    }

    size_t const column = stoull(argv[1]);

    uint64_t t = 0;             // time (= number of lines read).
    double N = 0;               // cumulative number of nodes at time t.
    double E = 0;               // cumulative number of edges at time t.
    vector<double> n;           // number of nodes in each class at time t.
    vector<uint64_t> k;         // class of each node at time t.
    vector<double> A;           // attachment kernel.
    vector<vector<uint64_t>> s; // active/inactive switching time points of each class
    vector<double> m_history;   // number of edges added at time t.

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
        m_history.push_back(tok.size() - column);
        E += m_history.back();

        double N_tmp = N;

        // Preprocess.
        for (auto it = tok.begin() + column; it != tok.end(); ++it) {

            uint64_t const ni = stoull(*it);
            nodes.push_back(ni);

            if (k.size() <= ni) k.resize(ni + 1, 0);

            uint64_t const& ki = k[ni];
            ++m[ki];
        }

        // Update attachment kernel.
        for (auto const& ni : nodes) {

            uint64_t const& ki = k[ni];

            if (ki == 0) {
                ++N_tmp; // add a new node.
            } else {
                A[ki] += m[ki] * N / n[ki];
            }
        }

        N = N_tmp;

        // Update node's class.
        for (auto const& ni : nodes) {

            uint64_t const& ki = ++k[ni];

            // Extend sizes of some arrays if needed.
            if (ki >= n.size()) {
                size_t const sz = ki + 2;
                n.resize(sz, 0);
                A.resize(sz, 0);
                s.resize(sz, vector<uint64_t>());
            }
        }

        // Update The number of nodes in each class
        // and, in addition, record time points to switch the activity state of each class.
        for (auto const& [k, mk] : m) {
            if (k > 0) {
                n[k] -= mk;
                if (n[k] == 0) s[k].push_back(t); // inactive time period of k-th class started.
            }
            if (n[k + 1] == 0) s[k + 1].push_back(t); // active time period of (k+1)-th class started.
            n[k + 1] += mk;
        }
    }

    vector<double> w(A.size(), 0); // normalization factor for each class.

    // s[k] should be [ t_a, t_i, t_a, t_i, ...], where 't_a' and 't_i' are a time point
    // at which k-th class is activated and inactivated, respectively.
    for (uint64_t k = 1; k < A.size(); ++k) {
        auto const& sk = s[k];
        for (uint64_t i = 1; i < sk.size(); ++i) {
            if (i % 2) {
                // Sum up only active time periods (= t_i - t_a).
                for (uint64_t tt = sk[i - 1]; tt < sk[i]; ++tt) {
                    w[k] += m_history[tt];
                }
                // If the number of edges added is temporally constant, say 'm',
                // the above loop can be simplified as follows:
                // w[k] += m * (sk[i] - sk[i - 1]);
            }
        }

        if (sk.size() % 2 == 1) {
            // This is the case the array ends with 't_a', such as [ t_a, t_i, ..., t_i, t_a ].
            // The time period after the last 't_a' is considered an active state.
            for (uint64_t tt = sk.back(); tt < t; ++tt) {
                w[k] += m_history[tt];
            }
            // simple case of constant 'm':
            // w[k] += m * (t - sk.back());
        }
    }

    // Output the result.
    
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
