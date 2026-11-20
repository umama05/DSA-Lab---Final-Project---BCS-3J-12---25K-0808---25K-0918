//  statistics.h  -  Reading statistics (counters + traversal).

#pragma once

#include <string>
#include <utility>
#include <vector>

#include "library.h"

struct LibraryStats {
    int totalBooks = 0;
    int wantCount = 0;
    int readingCount = 0;
    int readCount = 0;
    long long pagesRead = 0;  

    int ratedCount = 0;
    double averageRating = 0.0;  
    int ratingHistogram[6] = {0, 0, 0, 0, 0, 0}; 

    std::string favoriteGenre;
    std::vector<std::pair<std::string, int>> genreCounts;
};

LibraryStats computeStats(const Library& lib);


