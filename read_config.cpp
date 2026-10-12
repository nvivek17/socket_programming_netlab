// #include "read_config.h"
#include <vector>
#include <fstream>
#include <sstream>
#include <cassert>
#include <cctype>
#include <iostream>

using namespace std;

void parse_matrix(char* filepath, vector<vector<pair<int, int>>>& adj) {

    ifstream file(filepath);
    if (!file) {
        cout << "cannot open config file: " << filepath << '\n';
        exit(-1);
    }

    int n = -1;
    int row_no = 1;
    string line;
    while (getline(file, line)) {
        
        if (line.empty()) continue;
        if (line[0] == '#') continue;

        bool whitespace_line = true;
        for (char c : line) {
            if (!isspace(c) && c != '\r') whitespace_line = false;
        }
        if (whitespace_line) continue;
        
        if (row_no >= n + 1 && n != -1) {
            cout << "extra rows at end" << '\n';
            exit(-1);
        }

        // first row
        istringstream iss(line);
        vector<int> nums;
        int x;
        while (iss >> x) nums.push_back(x);

        if (row_no == 1) n = nums.size();
        adj.resize(n + 1);

        assert(nums.size() == n + 1 - row_no);

        for (int j = 1; j <= n + 1 - row_no; j++) {
            int cost = nums[j - 1];
            if (cost >= 0) {
                cout << row_no << " " << j + row_no - 1 << " " << nums[j - 1] << '\n';
                adj[row_no].push_back({j, cost});
                adj[j].push_back({row_no, cost});
            }
        }

        row_no++;
    }

    if (row_no <= n) {
        cout << "too few rows \n" << '\n';
    }
}