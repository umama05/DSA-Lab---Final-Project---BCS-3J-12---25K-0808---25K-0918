//  csv.h  -  basic csv reader & writer

#pragma once

#include <string>
#include <vector>  

namespace csv {

using Row = std::vector<std::string>;

// parses a csv doc from memory, skips blank lines
inline std::vector<Row> parse(const std::string& text) {
    std::vector<Row> rows;
    Row row;
    std::string field;
    bool inQuotes = false;

    std::size_t i = 0;
    if (text.size() >= 3 && static_cast<unsigned char>(text[0]) == 0xEF &&
        static_cast<unsigned char>(text[1]) == 0xBB && static_cast<unsigned char>(text[2]) == 0xBF)
        i = 3;  

    auto endRow = [&]() {
        row.push_back(field);
        field.clear();
        bool blank = (row.size() == 1 && row[0].empty());
        if (!blank) rows.push_back(row);
        row.clear();
    };

    for (; i < text.size(); ++i) {
        char c = text[i];
        if (inQuotes) {
            if (c == '"') {
                if (i + 1 < text.size() && text[i + 1] == '"') {
                    field += '"';
                    ++i;
                } else {
                    inQuotes = false;
                }
            } else {
                field += c;
            }
        } else if (c == '"') {
            inQuotes = true;
        } else if (c == ',') {
            row.push_back(field);
            field.clear();
        } else if (c == '\r') {
            
        } else if (c == '\n') {
            endRow();
        } else {
            field += c;
        }
    }
    if (!field.empty() || !row.empty()) endRow();
    return rows;
}

inline std::string escape(const std::string& s) {
    bool needsQuotes = s.find_first_of(",\"\r\n") != std::string::npos;
    if (!needsQuotes) return s;
    std::string out = "\"";
    for (char c : s) {
        if (c == '"') out += "\"\"";
        else out += c;
    }
    out += '"';
    return out;
}

inline std::string line(const Row& row) {
    std::string out;
    for (std::size_t i = 0; i < row.size(); ++i) {
        if (i) out += ',';
        out += escape(row[i]);
    }
    out += '\n';
    return out;
}

}  
