#ifndef NRANGE_H_INCLUDED
#define NRANGE_H_INCLUDED

#include <algorithm>
#include <cstddef>
#include <set>
#include <utility>
#include "iter.h"
#include "../util.h"

// Interval of numbers, first = begin (inc.), second = end (exc.)
typedef std::pair<std::size_t, std::size_t> NInterval;
// Range = set of intervals
typedef std::set<NInterval> NRange;

// Check if an interval is empty.
inline bool isempty(NInterval ninterval)
{
    return ninterval.first >= ninterval.second;
}
// Filter for empty intervals
bool (*const emptyfilter)(NInterval) = isempty;

// Check if a range is empty.
inline bool isempty(NRange const & nrange)
{
    return util::all_of(nrange.begin(), nrange.end(), emptyfilter);
}

// Check if a number is in an interval.
inline bool isnumberin(std::size_t n, NInterval ninterval)
{
    return ninterval.first <= n && n < ninterval.second;
}

// Check if a number is in a range.
inline bool isnumberin(std::size_t n, NRange const & nrange)
{
    for (NRange::const_iterator iter
         (nrange.begin()); iter != nrange.end(); ++iter)
        if (isnumberin(n, *iter)) return true;

    return false;
}

// Count the numbers in an interval. Return 0 if empty.
inline std::size_t numbercount(NInterval ninterval)
{
    return isempty(ninterval) ? 0 : ninterval.second - ninterval.first;
}

// Count the numbers in a range. Return 0 if empty.
inline std::size_t numbercount(NRange const & nrange)
{
    std::size_t n(0);
    for (NRange::const_iterator iter
         (nrange.begin()); iter != nrange.end(); ++iter)
        n += numbercount(*iter);

    return n;
}

// Remove empty intervals from a range.
inline void trim(NRange & nrange, bool trimmed = false)
{
    if (!trimmed) util::erase_if(nrange, emptyfilter);
}

// Simplify a range to a disjoint union of intervals.
inline void simplify(NRange & nrange, bool trimmed = false)
{
    trim(nrange, trimmed);

    for (NRange::iterator iter(nrange.begin()); iter != nrange.end(); )
    {
        // Find the end of the interval starting from iter->first.
        std::size_t end(iter->second);
        NRange::const_iterator iter2(iter);
        for (++iter2; iter2 != nrange.end() && iter2->first <= iter->second; ++iter2)
            if (iter2->second > end)
                end = iter2->second;
        // Adjust the first interval.
        NInterval ninterval(iter->first, end);
        nrange.erase(iter++);
        nrange.insert(ninterval);
        // Remove useless intervals.
        while (iter != iter2)
            nrange.erase(iter++);
    }
}
inline NRange simplify(NRange const & nrange)
{
    NRange result;
    std::remove_copy_if(nrange.begin(), nrange.end(), end_inserter(result),
                        emptyfilter);
    simplify(result, true);
    return result;
}

// Find the intersection of two intervals.
inline NInterval operator&(NInterval ninterval1, NInterval ninterval2)
{
    return NInterval(std::max(ninterval1.first, ninterval2.first),
                     std::min(ninterval1.second, ninterval2.second));
}

// Find the intersection of two ranges.
inline NRange & operator&=(NRange & nrange1, NRange const & nrange2)
{
    simplify(nrange1);

    for (NRange::iterator iter
         (nrange1.begin()); iter != nrange1.end(); ++iter)
    {
        NInterval ninterval(*iter);
        nrange1.erase(iter++);
        for (NRange::const_iterator iter2
             (nrange2.begin()); iter2 != nrange2.end(); ++iter2)
            nrange1.insert(ninterval & *iter2);
    }

    return nrange1;
}
inline NRange operator&(NRange const & nrange1, NRange const & nrange2)
{
    NRange result(simplify(nrange1));
    result &= nrange2;
    return result;
}

// Find the union of two ranges.
inline NRange & operator|=(NRange & nrange1, NRange const & nrange2)
{
    simplify(nrange1);
    std::remove_copy_if(nrange2.begin(), nrange2.end(), end_inserter(nrange1),
                        emptyfilter);
    simplify(nrange1, true);
    return nrange1;
}
inline NRange operator|(NRange const & nrange1, NRange const & nrange2)
{
    NRange result(simplify(nrange1));
    result |= nrange2;
    return result;
}

#endif // NRANGE_H_INCLUDED
