//  storage.h  -  Saving / loading the library as CSV TEXT.

#pragma once

#include <string>
#include <vector>

#include "library.h"

struct LoadReport {
    int loaded = 0;
    int skipped = 0;
    std::vector<std::string> warnings;
};

LoadReport loadCatalogText(const std::string& csvText, Library& lib);
LoadReport loadUserDataText(const std::string& csvText, Library& lib);  

std::string catalogToText(const Library& lib);
std::string userDataToText(const Library& lib);
