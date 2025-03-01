#ifndef ARITH_H_INCLUDED
#define ARITH_H_INCLUDED

#include <algorithm>
#include <limits>
#if __cplusplus >= 202002L
#include <bit>
#endif // __cplusplus >= 202002L

namespace util
{
    namespace helper
    {
        // Check if T is unsigned and # of digits of T is power of 2.
        template<class T> struct limits : std::numeric_limits<T>
        {
            using std::numeric_limits<T>::digits;
            using std::numeric_limits<T>::is_signed;
        };
        static unsigned char const cachedigits = 8;
        static unsigned char smalllog2[1 << cachedigits];
    } // namespace helper

class Smalllog2
{
    Smalllog2()
    {
        using namespace helper;
        smalllog2[0] = smalllog2[1] = 0;
        for (unsigned i(2); i < (1 << cachedigits); ++i)
            smalllog2[i] = smalllog2[i / 2] + 1;
            //std::cout << i << ' ' << (int)smalllog2[i] << std::endl;
    }
    template<class T> friend T log2(T n);
};

// Return n > 0 ? [log2(n)] : 0.
template<class T> T log2(T n)
{
#if __cplusplus >= 202002L
    return !!n * (std::bit_width(n) - 1);
#endif // __cplusplus >= 202002L
    using namespace helper;
    static const Smalllog2 init;
    static const bool okay = !limits<T>::is_signed;

    T result = 0 * sizeof(char[okay ? 1 : -1]);
    while (n >= (1 << cachedigits))
    {
        n >>= cachedigits;
        result += cachedigits;
    }
    //std::cout << n << ' ' << static_cast<int>(smalllog2[n]) << std::endl;
    return result + smalllog2[n];
}

// Return hex string of n.
template<class T>
const char * hex(T n)
{
    bool const okay(!helper::limits<T>::is_signed);
    int  const padding(sizeof(T[okay ? 3 : -1])/sizeof(T));
    // Clean space for hex string: ########x0\0
    char static s[helper::limits<T>::digits / 4 + padding];
    new(s) char[sizeof(s)]();
    // Fill digits.
    char * i(s);
    for ( ; n > 0; n /= 16, ++i)
    {
        T r(n % 16);
        *i = r < 10 ? r + '0' : r - 10 + 'A';
    }
    if (i == s) *i++ = '0';
    // Add "x0".
    *i++ = 'x', *i++ = '0';
    // Reverse the string.
    std::reverse(s, i);

    return s;
}
} // namespace util

#endif // ARITH_H_INCLUDED
