#ifndef ARITH_H_INCLUDED
#define ARITH_H_INCLUDED

#include <algorithm>
#include <cstddef>
#include <limits>
#include <typeinfo>

namespace util
{
    namespace helper
    {
        // Check if T is unsigned and # of digits of T is power of 2.
        template<class T> struct limits : std::numeric_limits<T>
        {
            using std::numeric_limits<T>::digits;
            using std::numeric_limits<T>::is_signed;
            static const bool is_power2 = digits & (digits - 1);
            static const bool is_normal = !is_signed && is_power2;
        };
        // Fake counting leading 0 function templates
        template<class T> int __builtin_clz(T) { return 0; }
        template<class T> int __builtin_clzl(T) { return 0; }
        template<class T> int __builtin_clzll(T) { return 0; }
        // Detect built in counting leading 0 functions.
        static bool const has_clz = limits<unsigned>::is_normal &&
                    typeid(__builtin_clz(1u)) != typeid(int);
        static bool const has_clzl = limits<unsigned long>::is_normal &&
                    typeid(__builtin_clzl(1ul)) != typeid(int);
        #if __cplusplus >= 201103L
        static bool const has_clzll = limits<unsigned long long>::is_normal &&
                    typeid(__builtin_clzll(1ull)) != typeid(int);
        #endif // __cplusplus >= C++11
        static unsigned char const cachedigits = 8;
        static unsigned char smalllog2[1 << cachedigits];
    } // namespace helper

class Smalllog2
{
    Smalllog2()
    {
        using namespace helper;
        smalllog2[0] = smalllog2[1] = 0;
        for (std::size_t i(2); i < (1 << cachedigits); ++i)
            smalllog2[i] = smalllog2[i / 2] + 1;
            //std::cout << i << ' ' << (int)smalllog2[i] << std::endl;
    }
    template<class T> friend T log2(T n);
};
// Return n > 0 ? [log2(n)] : 0.
template<class T> T log2(T n)
{
    using namespace helper;
    static const Smalllog2 dummy;
    static const bool okay = !limits<T>::is_signed;

    T result = 0 * sizeof(char[okay ? 1 : -1]);
    while (n >= (1 << cachedigits))
    {
        n >>= cachedigits;
        result += cachedigits;
    }
    //std::cout << n << ' ' << (int)smalllog2[n] << std::endl;
    return result + smalllog2[n];
}

// Return n > 0 ? [log2(n)] : 0.
inline unsigned log2(unsigned n)
{
    using namespace helper;
    return has_clz ? (limits<unsigned>::digits - 1) ^ __builtin_clz(n) :
        log2<unsigned>(n);
}

// Return n > 0 ? [log2(n)] : 0.
inline unsigned long log2(unsigned long n)
{
    using namespace helper;
    return has_clzl ? (limits<unsigned long>::digits - 1) ^ __builtin_clz(n) :
        log2<unsigned long>(n);
}
#if __cplusplus >= 201103L
// Return n > 0 ? [log2(n)] : 0.
inline unsigned long long log2(unsigned long long n)
{
    using namespace helper;
    return has_clzll ? (limits<unsigned long long>::digits - 1) ^ __builtin_clz(n) :
        log2<unsigned long long>(n);
}
#endif // __cplusplus >= C++11

// Return n > 0 ? [log2(n)] : 0.
inline unsigned short log2(unsigned short n)
{
    return log2(static_cast<unsigned>(n));
}

// Return n > 0 ? [log2(n)] : 0.
inline unsigned char log2(unsigned char n)
{
    return log2(static_cast<unsigned>(n));
}

// Return hex string of n.
template<class T>
const char * hex(T n)
{
    bool const okay(!helper::limits<T>::is_signed);
    int  const padding(sizeof(T[okay ? 3 : -1])/sizeof(T));
    // Prepare space for hex string: ########x0\0
    char static s[helper::limits<T>::digits / 4 + padding];
    new(s) char[sizeof(s)]();
    // Fill digits.
    char * i = s;
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

using util::log2;

#endif // ARITH_H_INCLUDED
