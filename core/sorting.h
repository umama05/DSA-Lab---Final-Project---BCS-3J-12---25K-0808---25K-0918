//  sorting.h  -  merge sort & insertion sort

#pragma once

#include <cstddef>
#include <vector>

template <typename T, typename Less>
std::size_t insertionSort(std::vector<T>& a, Less less) {
    std::size_t comparisons = 0;
    for (std::size_t i = 1; i < a.size(); ++i) {
        T key = a[i];
        std::size_t j = i;
        while (j > 0) {
            ++comparisons;
            if (less(key, a[j - 1])) {
                a[j] = a[j - 1];
                --j;
            } else {
                break;
            }
        }
        a[j] = key;
    }
    return comparisons;
}

namespace detail {
template <typename T, typename Less>
void mergeSortRange(std::vector<T>& a, std::vector<T>& tmp, std::size_t lo, std::size_t hi,
                    Less& less, std::size_t& comparisons) {
    if (hi - lo < 2) return;
    std::size_t mid = lo + (hi - lo) / 2;
    mergeSortRange(a, tmp, lo, mid, less, comparisons);
    mergeSortRange(a, tmp, mid, hi, less, comparisons);

    std::size_t i = lo, j = mid, k = lo;
    while (i < mid && j < hi) {
        ++comparisons;
        if (less(a[j], a[i])) tmp[k++] = a[j++];
        else tmp[k++] = a[i++];
    }
    while (i < mid) tmp[k++] = a[i++];
    while (j < hi) tmp[k++] = a[j++];
    for (k = lo; k < hi; ++k) a[k] = tmp[k];
}
}  

template <typename T, typename Less>
std::size_t mergeSort(std::vector<T>& a, Less less) {
    std::vector<T> tmp(a.size());
    std::size_t comparisons = 0;
    detail::mergeSortRange(a, tmp, 0, a.size(), less, comparisons);
    return comparisons;
}
