cmake_minimum_required(VERSION 3.16)
project(BookVerse VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/core ${CMAKE_SOURCE_DIR}/widgets ${CMAKE_SOURCE_DIR}/dialogs)

add_library(bookverse_core STATIC
    core/library.cpp
    core/recommender.cpp
    core/statistics.cpp
    core/storage.cpp
    core/book.h core/csv.h core/hashtable.h core/library.h core/linkedlist.h core/recommender.h
    core/sorting.h core/stack.h core/statistics.h core/storage.h core/textutil.h)
target_include_directories(bookverse_core PUBLIC core)

set(BOOKVERSE_GUI_SOURCES
    mainwindow.cpp mainwindow.h mainwindow.ui
    dialogs/addbookdialog.cpp dialogs/addbookdialog.h dialogs/addbookdialog.ui
    widgets/barchart.cpp widgets/barchart.h
    widgets/bookdelegate.cpp widgets/bookdelegate.h
    widgets/bookdetailpanel.cpp widgets/bookdetailpanel.h widgets/bookdetailpanel.ui
    widgets/coverstore.cpp widgets/coverstore.h
    widgets/guiutil.cpp widgets/guiutil.h
    widgets/hintlistwidget.cpp widgets/hintlistwidget.h
    widgets/starrating.cpp widgets/starrating.h
    resources.qrc)

add_executable(BookVerse WIN32 main.cpp ${BOOKVERSE_GUI_SOURCES})
target_link_libraries(BookVerse PRIVATE bookverse_core Qt${QT_VERSION_MAJOR}::Widgets)

if(MSVC)
    target_compile_options(bookverse_core PRIVATE /utf-8 /W3)
    target_compile_options(BookVerse PRIVATE /utf-8 /W3)
endif()


