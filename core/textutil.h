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

// converts string to an integer
inline int toIntOr(const std::string& s, int fallback) {
    std::string t = trim(s);
    if (t.empty()) return fallback;
    char* end = nullptr;
    errno = 0;
    long v = std::strtol(t.c_str(), &end, 10);
    if (end == t.c_str() || *end != '\0' || errno == ERANGE || v > INT_MAX || v < INT_MIN) return fallback;
    return static_cast<int>(v);
}

// converts a string to a long long integer
inline long long toLongLongOr(const std::string& s, long long fallback) {
    std::string t = trim(s);
    if (t.empty()) return fallback;
    char* end = nullptr;
    errno = 0;
    long long v = std::strtoll(t.c_str(), &end, 10);
    if (end == t.c_str() || *end != '\0' || errno == ERANGE) return fallback;
    return v;
}

// converts a double value to a string with one decimal place
inline std::string fmt1(double v) {
    char buf[32];
    std::snprintf(buf, sizeof buf, "%.1f", v);
    return buf;
}

} 

