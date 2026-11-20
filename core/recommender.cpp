#include "recommender.h"

#include <algorithm>
#include "sorting.h"
#include "textutil.h"

namespace {

struct GenreProfile {
    std::string genre;
    double weight = 0.0;
    int ratedCount = 0;
    int ratingSum = 0;
    int readCount = 0;
};

struct AuthorProfile {
    std::string author;
    int bestRating = 0;
    std::string bestTitle;
};

double ratingWeight(int rating) {
    switch (rating) {
        case 5: return 3.0;
        case 4: return 1.5;
        case 3: return 0.25;
        case 2: return -1.0;
        case 1: return -2.0;
        default: return 0.0;
    }
}

GenreProfile& genreEntry(std::vector<GenreProfile>& v, const std::string& genre) {
    for (GenreProfile& g : v)
        if (g.genre == genre) return g;
    v.push_back(GenreProfile());
    v.back().genre = genre;
    return v.back();
}

AuthorProfile& authorEntry(std::vector<AuthorProfile>& v, const std::string& author) {
    for (AuthorProfile& a : v)
        if (a.author == author) return a;
    v.push_back(AuthorProfile());
    v.back().author = author;
    return v.back();
}

}  

std::vector<Recommendation> recommend(const Library& lib, std::size_t maxResults) {
    const std::vector<const Book*> books = lib.allBooks();

    std::vector<GenreProfile> genres;
    std::vector<AuthorProfile> authors;
    bool hasSignal = false;

    for (const Book* b : books) {
        const bool rated = b->rating() > 0;
        const bool finished = b->shelf == Shelf::Read;
        if (!rated && !finished) continue;
        hasSignal = true;

        GenreProfile& g = genreEntry(genres, b->genre);
        if (rated) {
            g.weight += ratingWeight(b->rating());
            ++g.ratedCount;
            g.ratingSum += b->rating();
        } else {
            g.weight += 0.5; 
        }
        if (finished) ++g.readCount;

        if (rated) {
            AuthorProfile& a = authorEntry(authors, b->author);
            if (b->rating() > a.bestRating) {
                a.bestRating = b->rating();
                a.bestTitle = b->title;
            }
        }
    }
    if (!hasSignal) return {}; 

    double bestWeight = 0.0;
    for (const GenreProfile& g : genres) bestWeight = std::max(bestWeight, g.weight);

    struct Scored {
        const Book* book;
        double score;
        std::string reason;
    };
    std::vector<Scored> scored;

    for (const Book* b : books) {
        const bool candidate = (b->shelf == Shelf::None || b->shelf == Shelf::WantToRead) && b->rating() == 0;
        if (!candidate) continue;

        double score = 0.0;
        std::vector<std::string> reasons;

        const GenreProfile* gp = nullptr;
        for (const GenreProfile& g : genres)
            if (g.genre == b->genre) gp = &g;
        if (gp) {
            if (gp->weight > 0.0 && bestWeight > 0.0) {
                score += 70.0 * gp->weight / bestWeight;
                std::string r = "You enjoy " + b->genre;
                if (gp->ratedCount > 0)
                    r += " (avg " + text::fmt1(static_cast<double>(gp->ratingSum) / gp->ratedCount) + " stars over " +
                         std::to_string(gp->ratedCount) + (gp->ratedCount == 1 ? " book)" : " books)");
                else
                    r += " (you've read " + std::to_string(gp->readCount) + ")";
                reasons.push_back(r);
            } else if (gp->weight < 0.0) {
                score -= 40.0;  
            }
        }

        for (const AuthorProfile& a : authors) {
            if (a.author == b->author && a.bestRating >= 4) {
                score += (a.bestRating == 5) ? 25.0 : 15.0;
                reasons.push_back("Same author as '" + a.bestTitle + "' (" + std::to_string(a.bestRating) + " stars)");
            }
        }

        if (b->shelf == Shelf::WantToRead) {
            score += 10.0;
            reasons.push_back("Already on your Want to Read shelf");
        }

        if (score <= 0.0) continue;

        std::string reason;
        for (std::size_t i = 0; i < reasons.size(); ++i) reason += (i ? "  \xC2\xB7  " : "") + reasons[i];
        scored.push_back({b, std::min(100.0, score), reason});
    }

    mergeSort(scored, [](const Scored& a, const Scored& b) {
        if (a.score != b.score) return a.score > b.score;
        return text::toLower(a.book->title) < text::toLower(b.book->title);
    });

    std::vector<Recommendation> out;
    for (std::size_t i = 0; i < scored.size() && i < maxResults; ++i) {
        Recommendation r;
        r.book = scored[i].book;
        r.matchPercent = static_cast<int>(scored[i].score + 0.5);
        r.reason = scored[i].reason;
        out.push_back(r);
    }
    return out;
}
