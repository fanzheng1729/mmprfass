#ifndef SECT_H_INCLUDED
#define SECT_H_INCLUDED

#include <cstddef>
#include <iosfwd>
#include <map>
#include <string>
#include <vector>

// Section number
typedef std::vector<std::size_t> Sectionnumber;
// Section level count
typedef Sectionnumber::size_type Sectionlevel;

// Data of a section
struct Section
{
    std::string title;
    // Position in tokens
    std::size_t tokenpos;
    // # assertions before the section
    std::size_t assnumber;
};

// Map: Section # -> position of begin of section in the token/assertion list
struct Sections : std::map<Sectionnumber, Section>
{
// Print the contents.
    void print(Sectionlevel const level = 0) const;
// Find the section whose title begins with str.
    const_iterator find(const char * str) const;
// Parse section header ($4.4.1/Headings).
    Sections(struct Comments const & comments);
};

std::ostream & operator<<(std::ostream & out, const Sectionnumber & sn);
std::ostream & operator<<(std::ostream & out, const Section & sect);

#endif // SECT_H_INCLUDED
