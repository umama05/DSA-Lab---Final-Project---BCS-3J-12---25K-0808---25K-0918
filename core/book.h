//book.h - data model (shelf, review, userstate, book)
#pragma once

#include <string>

enum class Shelf {
  None = 0,
  WantToRead = 1,
  CurrentlyReading = 2,
  Read = 3
};

inline Shelf shelfFromInt(int v) {
  switch (v) {
    case 1: return Shelf::WantToRead;
    case 2: return Shelf::CurrentlyReading;
    case 3: return Shelf::Read;
    default: return Shelf::None;
  }
}

inline const char* shelfName(Shelf s) {
  switch (s) {
    case Shelf::WantToRead: return "Want to Read";
    case Shelf::CurrentlyReading: return "Currently Reading";
    case Shelf::Read: return "Read";
    default: return "No shelf";
  }
}

//user 1-5 star rating
struct Review {
  int rating = 0;
  std::string text;
  long long stamp = 0;

  bool empty() const {
    return rating == 0 && text.empty();
  }
};

//user addition to catalog entry
struct UserState {
  Shelf shelf = Shelf::None;
  int currentPage = 0;
  Review review;
};

struct Book {
  //catalog data
  int id = 0;
  std::string title;
  std::string author;
  std::string genre;
  int year = 0;
  int pages = 0;
  std::string description;

  //user data
  Shelf shelf = Shelf::None;
  int currentPage = 0;
  Review review;

  int rating() const {
    return review.rating;
  }
};








