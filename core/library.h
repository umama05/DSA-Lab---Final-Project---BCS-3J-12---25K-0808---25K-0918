//  library.h  -  combines all data structures
#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "book.h"
#include "hashtable.h"
#include "linkedlist.h"
#include "stack.h"

enum class SortKey { Title, Author, Rating, Year };
enum class SortAlgorithm { MergeSort, InsertionSort };

struct SortStats {
    std::size_t items = 0;        
    std::size_t comparisons = 0;  
};

struct UndoAction {
    int bookId = 0;
    UserState before;   
    std::string label;  
};

class Library {
public:
    static constexpr std::size_t kMaxReviewBytes = 4000;

    Library() = default;
    Library(const Library&) = delete;
    Library& operator=(const Library&) = delete;

    int addBook(Book book, std::string* error = nullptr);
    
    bool removeBook(int id);

    Book* findBook(int id);
    const Book* findBook(int id) const;
    std::size_t bookCount() const { return books_.size(); }

    std::vector<Book*> allBooks();              
    std::vector<const Book*> allBooks() const;  
    std::vector<std::string> genres() const;    


    std::vector<Book*> query(const std::string& text, const std::string& genre, SortKey key,
        bool ascending, SortAlgorithm algorithm, SortStats* stats = nullptr);


    const LinkedList<Book*>& shelf(Shelf s) const;
    const LinkedList<Book*>& reviewFeed() const { return reviewFeed_; }


    bool updateUserData(int id, Shelf shelf, int currentPage, int rating, const std::string& review,
        std::string* undoLabel = nullptr);
    bool moveToShelf(int id, Shelf shelf, std::string* undoLabel = nullptr);
    bool setRating(int id, int rating, std::string* undoLabel = nullptr);


    bool canUndo() const { return !undo_.empty(); }
    std::string undoLabel() const;  
    bool undo(std::string* message = nullptr);
    void clearUndo() { undo_.clear(); }


    bool restoreUserState(int id, const UserState& state);  
    void rebuildReviewFeed();
    void resetUserData();  

private:
    std::vector<LinkedList<Book*>*> shelfLists();
    LinkedList<Book*>* listFor(Shelf s);
    const LinkedList<Book*>* listFor(Shelf s) const;
    void setShelfMembership(Book* b, Shelf s, bool toFront);
    long long nextStamp();
    bool isDuplicate(const Book& b) const;

    HashTable<Book> books_;
    LinkedList<Book*> order_;
    LinkedList<Book*> wantToRead_;
    LinkedList<Book*> reading_;
    LinkedList<Book*> read_;
    LinkedList<Book*> reviewFeed_;
    Stack<UndoAction> undo_;

    int nextId_ = 1;
    long long lastStamp_ = 0;
};
