#include "storage.h"

#include "csv.h"
#include "sorting.h"
#include "textutil.h"

LoadReport loadCatalogText(const std::string& csvText, Library& lib) {
    LoadReport report;
    const std::vector<csv::Row> rows = csv::parse(csvText);

    for (std::size_t i = 0; i < rows.size(); ++i) {
        const csv::Row& r = rows[i];
        if (i == 0 && !r.empty() && text::toLower(text::trim(r[0])) == "id") continue;  // header row

        if (r.size() < 6) {
            ++report.skipped;
            report.warnings.push_back("catalog row " + std::to_string(i + 1) + " has too few columns");
            continue;
        }
        Book b;
        b.id = text::toIntOr(r[0], 0);
        b.title = r[1];
        b.author = r[2];
        b.genre = r[3];
        b.year = text::toIntOr(r[4], 0);
        b.pages = text::toIntOr(r[5], 0);
        if (r.size() > 6) b.description = r[6];

        std::string err;
        if (lib.addBook(b, &err)) {
            ++report.loaded;
        } else {
            ++report.skipped;
            report.warnings.push_back("catalog row " + std::to_string(i + 1) + " skipped: " + err);
        }
    }
    return report;
}

LoadReport loadUserDataText(const std::string& csvText, Library& lib) {
    LoadReport report;
    const std::vector<csv::Row> rows = csv::parse(csvText);

    for (std::size_t i = 0; i < rows.size(); ++i) {
        const csv::Row& r = rows[i];
        if (i == 0 && !r.empty() && text::toLower(text::trim(r[0])) == "id") continue;  // header row

        if (r.size() < 4) {
            ++report.skipped;
            continue;
        }
        UserState st;
        const int id = text::toIntOr(r[0], 0);
        st.shelf = shelfFromInt(text::toIntOr(r[1], 0));
        st.currentPage = text::toIntOr(r[2], 0);
        st.review.rating = text::toIntOr(r[3], 0);
        if (r.size() > 4) st.review.text = r[4];
        if (r.size() > 5) st.review.stamp = text::toLongLongOr(r[5], 0);

        if (lib.restoreUserState(id, st)) {
            ++report.loaded;
        } else {
            ++report.skipped;  // book no longer in the catalog
            report.warnings.push_back("user data row " + std::to_string(i + 1) + " refers to an unknown book");
        }
    }
    lib.rebuildReviewFeed();
    lib.clearUndo();
    return report;
}

std::string catalogToText(const Library& lib) {
    std::string out = csv::line({"id", "title", "author", "genre", "year", "pages", "description"});
    std::vector<const Book*> books = lib.allBooks();
    mergeSort(books, [](const Book* a, const Book* b) { return a->id < b->id; });
    for (const Book* b : books)
        out += csv::line({std::to_string(b->id), b->title, b->author, b->genre, std::to_string(b->year),
                          std::to_string(b->pages), b->description});
    return out;
}

std::string userDataToText(const Library& lib) {
    std::string out = csv::line({"id", "shelf", "currentPage", "rating", "review", "reviewedAt"});

    auto writeBook = [&](const Book* b) {
        out += csv::line({std::to_string(b->id), std::to_string(static_cast<int>(b->shelf)),
                          std::to_string(b->currentPage), std::to_string(b->review.rating), b->review.text,
                          std::to_string(b->review.stamp)});
    };

    for (Shelf s : {Shelf::WantToRead, Shelf::CurrentlyReading, Shelf::Read})
        for (const Book* b : lib.shelf(s)) writeBook(b);
    
    for (const Book* b : lib.allBooks())
        if (b->shelf == Shelf::None && !b->review.empty()) writeBook(b);
    return out;
}




