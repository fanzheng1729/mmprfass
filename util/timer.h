#ifndef TIMER_H_INCLUDED
#define TIMER_H_INCLUDED

#include <ctime>
struct Timer
{
    std::clock_t start;
    void reset() { start = std::clock(); }
    Timer() { reset(); }
    operator double() const { return resolution() * (std::clock() - start); }
    static double resolution() { return 1./CLOCKS_PER_SEC; }
};

#endif // TIMER_H_INCLUDED
