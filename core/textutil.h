//  textutil.h  -  string helpers 

#pragma once

#include <cctype>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace text {

// removes whitespace from the beginning and end of a string
inline std::string trim(const std::string& s) {
    std::size_t b = 0, e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return s.substr(b, e - b);
}

// converts all ASCII characters in a string to lowercase
inline std::string toLower(const std::string& s) {
    std::string out = s;
    for (char& c : out)
        if (static_cast<unsigned char>(c) < 128) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
    return out;
}

// splits string into individual words using whitespace as a separator
inline std::vector<std::string> tokenize(const std::string& s) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : s) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (!cur.empty()) out.push_back(cur), cur.clear();
        } 
        else {
            cur += c;
        }
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}
