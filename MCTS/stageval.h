#ifndef STAGEVAL_H_INCLUDED
#define STAGEVAL_H_INCLUDED

#include <cstddef>

// Stage
typedef std::size_t stage_t;

// Evaluation (value, sure?)
typedef std::pair<double, bool> Eval;

// Predefined game values
enum WDL { LOSS = -1, DRAW = 0, WIN = 1 };

#endif // STAGEVAL_H_INCLUDED
