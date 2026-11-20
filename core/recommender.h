//  recommender.h  -  score-based book recommendations.

#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "library.h"

struct Recommendation {
    const Book* book = nullptr;
    int matchPercent = 0;  
    std::string reason;
};

std::vector<Recommendation> recommend(const Library& lib, std::size_t maxResults = 10);
