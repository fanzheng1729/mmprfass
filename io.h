#ifndef IO_H_INCLUDED
#define IO_H_INCLUDED

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <map>
#include <string>
#include <vector>

struct strview;
std::ostream & operator<<(std::ostream & out, strview str);
template<class T>
std::ostream & operator<<(std::ostream & out, const std::vector<T> & v)
{
    std::copy(v.begin(), v.end(), std::ostream_iterator<T>(out, " "));
    return out << std::endl;
}
inline std::ostream & operator<<
    (std::ostream & out, const std::vector<bool> & v)
{
    std::copy(v.begin(), v.end(), std::ostream_iterator<bool>(out));
    return out << std::endl;
}
struct CNFClauses;
std::ostream & operator<<(std::ostream & out, const CNFClauses & cnf);
template<class Key, class T>
std::ostream & operator<<(std::ostream & out, const std::map<Key, T> & map)
{
    for (typename std::map<Key, T>::const_iterator
         iter(map.begin()); iter != map.end(); ++iter)
    {
        out << iter->first << ' ';
    }
    out << std::endl;

    return out;
}

// Return true if n <= lim. Otherwise print a message and return false.
bool is1stle2nd(std::size_t const n, std::size_t const lim,
                const char * const s1, const char * const s2);

template<class T>
bool unexpected(bool const condition, const char * const type, const T & value)
{
    if (condition)
        std::cerr << "Unexpected " << type << ": " << value << std::endl;

    return condition;
}

// Print the number and the label of an assertion.
void printass(std::size_t number, strview label, std::size_t count = 0);
template<class Assref>
void printass(Assref const & assref, std::size_t count = 0)
{
    printass(assref.second.number, assref.first, count);
}

// Ask a question and get its answer.
inline std::string ask(const char * question)
{
    std::string answer;
    std::cout << question;
    std::cin >> answer;
    return answer;
}

// Ask a y/n question and get its answer.
inline bool askyn(const char * question)
{
    std::string const & answer(ask(question));
    if (answer.empty()) return false;
    unsigned char const c(answer[0]);
    return std::tolower(c) == 'y';
}

#endif // IO_H_INCLUDED
