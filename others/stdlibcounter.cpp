// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Jussi Pakkanen

#include<vector>
#include<string>
#include<unordered_map>
#include<fstream>
#include<iostream>
#include<sstream>
#include<algorithm>

struct WordCount {
    const std::string *s;
    size_t count;

    int operator<=>(const WordCount &o) const {
        auto diff = (int64_t) o.count - (int64_t) count;
        if(diff != 0) {
            return diff;
        }
        return (int64_t)o.s->size() - (int64_t) s->size();
    }
};

int main(int argc, char **argv) {
    if(argc != 2) {
        std::cout << argv[0] << " <input file>\n";
        return 0;
    }
    std::string line;
    std::ifstream ifile(argv[1]);
    std::string word;
    std::unordered_map<std::string, size_t> counts;
    while(std::getline(ifile, line)) {
        std::stringstream linestream(line);
        while(std::getline(linestream, word, ' ')) {
            ++counts[word];
        }
    }
    std::vector<WordCount> stats;
    stats.reserve(counts.size());
    for(const auto &[key, value]: counts) {
        stats.emplace_back(WordCount(&key, value));
    }
    std::sort(stats.begin(), stats.end());
    for(const auto &i: stats) {
        std::cout << i.count << " " << *i.s << "\n";
    }
    return 0;
}
