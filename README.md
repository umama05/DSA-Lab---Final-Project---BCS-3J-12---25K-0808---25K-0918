# BookVerse - Book Tracking & Review System

DSA-Lab---Final-Project---BCS-3J-12---25K-0808---25K-0918

## Project layout
```
core/       data structures and logic 
widgets/    reusable Qt widgets (book cards, detail panel, charts, covers)
dialogs/    Add Book dialog
resources/  stylesheet and icons
data/       starter catalog 
```

## Build
Requirements: C++17, Qt 6 with the Widgets module, CMake 3.16+.

```
cmake -S . -B build -DCMAKE_PREFIX_PATH=<path to your Qt, e.g. C:/Qt/6.x.x/msvc2022_64>
cmake --build build
```
