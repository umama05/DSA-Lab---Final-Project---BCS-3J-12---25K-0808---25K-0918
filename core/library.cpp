#include "library.h"
#include "sorting.h"
#include "textutil.h"
#include <algorithm>
#include <ctime>

//  catalog (hash table + insertion-ordered list)
bool Library::isDuplicate(const Book& b) const {
    const std::string t = text::toLower(b.title);
    const std::string a = text::toLower(b.author);
    for (const Book* other : order_)  // linear scan: fine for a personal library
        if (text::toLower(other->title) == t && text::toLower(other->author) == a) return true;
    return false;
}

int Library::addBook(Book book, std::string* error) {
    auto fail = [&](const char* msg) {
        if (error) *error = msg;
        return 0;
    };

    book.title = text::trim(book.title);
    book.author = text::trim(book.author);
    book.genre = text::trim(book.genre);
    book.description = text::trim(book.description);

    if (book.title.empty()) return fail("Please enter a title.");
    if (book.author.empty()) return fail("Please enter an author.");
    if (book.genre.empty()) return fail("Please choose or type a genre.");
    if (book.pages < 1) return fail("A book needs at least one page.");
    if (book.year < 1 || book.year > 2100) return fail("Please enter a valid publication year.");
    if (isDuplicate(book)) return fail("That title by that author is already in the catalog.");

    if (book.id <= 0) book.id = nextId_;
    else if (books_.contains(book.id)) return fail("A book with this ID already exists.");

    book.shelf = Shelf::None;
    book.currentPage = 0;
    book.review = Review();

    Book* stored = books_.insert(book.id, book); 
    order_.pushBack(stored);
    if (book.id >= nextId_) nextId_ = book.id + 1;
    return book.id;
}

bool Library::removeBook(int id) {
    Book* b = books_.find(id);
    if (!b) return false;
    setShelfMembership(b, Shelf::None, false);
    reviewFeed_.remove(b);
    order_.remove(b);
    undo_.clear(); 
    books_.remove(id);
    return true;
}

Book* Library::findBook(int id) { return books_.find(id); }
const Book* Library::findBook(int id) const { return books_.find(id); }

std::vector<Book*> Library::allBooks() { return order_.toVector(); }

std::vector<const Book*> Library::allBooks() const {
    std::vector<const Book*> out;
    out.reserve(order_.size());
    for (const Book* b : order_) out.push_back(b);
    return out;
}

std::vector<std::string> Library::genres() const {
    std::vector<std::string> out;
    for (const Book* b : order_) {
        bool seen = false;
        for (const std::string& g : out)
            if (g == b->genre) {
                seen = true;
                break;
            }
        if (!seen) out.push_back(b->genre);
    }
    mergeSort(out, [](const std::string& x, const std::string& y) { return text::toLower(x) < text::toLower(y); });
    return out;
}

//  search / filter / sort
std::vector<Book*> Library::query(const std::string& searchText, const std::string& genre, SortKey key,
                                  bool ascending, SortAlgorithm algorithm, SortStats* stats) {
    const std::vector<std::string> tokens = text::tokenize(text::toLower(searchText));

    // 1) linear search + linear filter
    std::vector<Book*> result;
    for (Book* b : order_) {
        if (!genre.empty() && b->genre != genre) continue;
        if (!tokens.empty()) {
            const std::string hay = text::toLower(b->title) + "\n" + text::toLower(b->author);
            bool all = true;
            for (const std::string& t : tokens)
                if (hay.find(t) == std::string::npos) {
                    all = false;
                    break;
                }
            if (!all) continue;
        }
        result.push_back(b);
    }

    // 2) sort with the chosen algorithm
    auto before = [key](const Book* a, const Book* b) {
        switch (key) {
            case SortKey::Title: return text::toLower(a->title) < text::toLower(b->title);
            case SortKey::Author: return text::toLower(a->author) < text::toLower(b->author);
            case SortKey::Rating: return a->rating() < b->rating();
            case SortKey::Year: return a->year < b->year;
        }
        return false;
    };
    auto less = [&](const Book* a, const Book* b) { return ascending ? before(a, b) : before(b, a); };

    std::size_t comparisons = (algorithm == SortAlgorithm::MergeSort) ? mergeSort(result, less)
                                                                      : insertionSort(result, less);
    if (stats) {
        stats->items = result.size();
        stats->comparisons = comparisons;
    }
    return result;
}

//  shelves (linked lists)
LinkedList<Book*>* Library::listFor(Shelf s) {
    switch (s) {
        case Shelf::WantToRead: return &wantToRead_;
        case Shelf::CurrentlyReading: return &reading_;
        case Shelf::Read: return &read_;
        default: return nullptr;
    }
}
const LinkedList<Book*>* Library::listFor(Shelf s) const {
    switch (s) {
        case Shelf::WantToRead: return &wantToRead_;
        case Shelf::CurrentlyReading: return &reading_;
        case Shelf::Read: return &read_;
        default: return nullptr;
    }
}

const LinkedList<Book*>& Library::shelf(Shelf s) const {
    static const LinkedList<Book*> kEmpty;
    const LinkedList<Book*>* l = listFor(s);
    return l ? *l : kEmpty;
}

void Library::setShelfMembership(Book* b, Shelf s, bool toFront) {
    if (LinkedList<Book*>* old = listFor(b->shelf)) old->remove(b);
    b->shelf = s;
    if (LinkedList<Book*>* now = listFor(s)) {
        if (toFront) now->pushFront(b); 
        else now->pushBack(b);          
    }
}

long long Library::nextStamp() {
    long long now = static_cast<long long>(std::time(nullptr));
    if (now <= lastStamp_) now = lastStamp_ + 1;  
    lastStamp_ = now;
    return now;
}

//  user data changes (undoable)
bool Library::updateUserData(int id, Shelf shelf, int page, int rating, const std::string& reviewText,
                             std::string* undoLabel) {
    Book* b = books_.find(id);
    if (!b) return false;

    rating = std::max(0, std::min(5, rating));
    std::string review = text::trim(reviewText);
    if (review.size() > kMaxReviewBytes) review.resize(kMaxReviewBytes);
    page = std::max(0, std::min(b->pages, page));
    switch (shelf) {
        case Shelf::Read:
            page = b->pages;
            break;
        case Shelf::CurrentlyReading:
            if (page >= b->pages) {  
                shelf = Shelf::Read;
                page = b->pages;
            }
            break;
        default:
            page = 0; 
            break;
    }

    const bool shelfChanged = b->shelf != shelf;
    const bool pageChanged = b->currentPage != page;
    const bool ratingChanged = b->review.rating != rating;
    const bool textChanged = b->review.text != review;
    if (!shelfChanged && !pageChanged && !ratingChanged && !textChanged) return false;

    UndoAction action;
    action.bookId = id;
    action.before.shelf = b->shelf;
    action.before.currentPage = b->currentPage;
    action.before.review = b->review;

    if (shelfChanged) setShelfMembership(b, shelf, true);
    b->currentPage = page;
    if (ratingChanged || textChanged) {
        b->review.rating = rating;
        b->review.text = review;
        reviewFeed_.remove(b);
        if (b->review.empty()) {
            b->review.stamp = 0;
        } else {
            b->review.stamp = nextStamp();
            reviewFeed_.pushFront(b);  
        }
    }

    const std::string quoted = "'" + b->title + "'";
    if (shelfChanged && !ratingChanged && !textChanged) {
        action.label = shelf == Shelf::None ? "Removed " + quoted + " from your shelves"
                                            : "Moved " + quoted + " to " + shelfName(shelf);
    } else if (!shelfChanged && pageChanged && !ratingChanged && !textChanged) {
        action.label = "Updated progress on " + quoted;
    } else if (ratingChanged && !shelfChanged && !pageChanged && !textChanged) {
        action.label = rating == 0 ? "Cleared the rating of " + quoted
                                   : "Rated " + quoted + " " + std::to_string(rating) + " stars";
    } else if (textChanged && !shelfChanged && !pageChanged && !ratingChanged) {
        action.label = "Edited your review of " + quoted;
    } else {
        action.label = "Updated " + quoted;
    }

    if (undoLabel) *undoLabel = action.label;
    undo_.push(action);
    return true;
}

bool Library::moveToShelf(int id, Shelf shelf, std::string* undoLabel) {
    const Book* b = books_.find(id);
    if (!b) return false;
    return updateUserData(id, shelf, b->currentPage, b->review.rating, b->review.text, undoLabel);
}

bool Library::setRating(int id, int rating, std::string* undoLabel) {
    const Book* b = books_.find(id);
    if (!b) return false;
    return updateUserData(id, b->shelf, b->currentPage, rating, b->review.text, undoLabel);
}

//  undo (stack)
std::string Library::undoLabel() const {
    const UndoAction* top = undo_.peek();
    return top ? top->label : std::string();
}

bool Library::undo(std::string* message) {
    UndoAction action;
    if (!undo_.pop(action)) return false;
    Book* b = books_.find(action.bookId);
    if (!b) return false;

    setShelfMembership(b, action.before.shelf, true);
    b->currentPage = action.before.currentPage;
    b->review = action.before.review;
    rebuildReviewFeed(); 

    if (message) *message = "Undid: " + action.label;
    return true;
}

//  loading helpers
bool Library::restoreUserState(int id, const UserState& st) {
    Book* b = books_.find(id);
    if (!b) return false;

    Shelf shelf = st.shelf;
    int page = std::max(0, std::min(b->pages, st.currentPage));
    if (shelf == Shelf::Read) page = b->pages;
    else if (shelf != Shelf::CurrentlyReading) page = 0;

    setShelfMembership(b, shelf, false);
    b->currentPage = page;
    b->review = st.review;
    b->review.rating = std::max(0, std::min(5, b->review.rating));
    if (b->review.text.size() > kMaxReviewBytes) b->review.text.resize(kMaxReviewBytes);
    if (b->review.empty()) b->review.stamp = 0;
    else if (b->review.stamp <= 0) b->review.stamp = nextStamp();
    if (b->review.stamp > lastStamp_) lastStamp_ = b->review.stamp;
    return true;
}

void Library::rebuildReviewFeed() {
    reviewFeed_.clear();
    std::vector<Book*> reviewed;
    for (Book* b : order_)
        if (!b->review.empty()) reviewed.push_back(b);
    // merge sort
    mergeSort(reviewed, [](const Book* a, const Book* b) { return a->review.stamp > b->review.stamp; });
    for (Book* b : reviewed) reviewFeed_.pushBack(b);
}

void Library::resetUserData() {
    for (Book* b : order_) {
        b->shelf = Shelf::None;
        b->currentPage = 0;
        b->review = Review();
    }
    wantToRead_.clear();
    reading_.clear();
    read_.clear();
    reviewFeed_.clear();
    undo_.clear();
}
