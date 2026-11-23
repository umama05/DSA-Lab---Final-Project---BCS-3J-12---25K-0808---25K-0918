#include "statistics.h"

#include "sorting.h"

namespace {
    void bump(std::vector<std::pair<std::string, int>>& counts, const std::string& genre) {
        for (auto& entry : counts)
            if (entry.first == genre) {
                ++entry.second;
                return;
            }
        counts.emplace_back(genre, 1);
    }
}  // namespace

LibraryStats computeStats(const Library& lib) {
    LibraryStats s;
    s.totalBooks = static_cast<int>(lib.bookCount());
    s.wantCount = static_cast<int>(lib.shelf(Shelf::WantToRead).size());
    s.readingCount = static_cast<int>(lib.shelf(Shelf::CurrentlyReading).size());
    s.readCount = static_cast<int>(lib.shelf(Shelf::Read).size());

    std::vector<std::pair<std::string, int>> readGenres;
    for (const Book* b : lib.shelf(Shelf::Read)) {
        s.pagesRead += b->pages;
        bump(readGenres, b->genre);
    }
    for (const Book* b : lib.shelf(Shelf::CurrentlyReading)) s.pagesRead += b->currentPage;

    std::vector<std::pair<std::string, int>> ratedGenres;
    long long ratingSum = 0;
    for (const Book* b : lib.allBooks()) {
        int r = b->rating();
        if (r < 1) continue;
        ++s.ratedCount;
        ratingSum += r;
        ++s.ratingHistogram[r];
        bump(ratedGenres, b->genre);
    }
    if (s.ratedCount > 0) s.averageRating = static_cast<double>(ratingSum) / s.ratedCount;

    s.genreCounts = !readGenres.empty() ? readGenres : ratedGenres;
    mergeSort(s.genreCounts, [](const std::pair<std::string, int>& a, const std::pair<std::string, int>& b) {
        if (a.second != b.second) return a.second > b.second;
        return a.first < b.first;
        });
    if (!s.genreCounts.empty()) s.favoriteGenre = s.genreCounts.front().first;
    return s;
}



