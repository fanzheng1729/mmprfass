#ifndef MCTS_H_INCLUDED
#define MCTS_H_INCLUDED

#include "MCTS1.h"
#if __cplusplus >= 201103L
#include "MCTS2.h"
#else
#define Tree2 Tree
#define MCTS2 MCTS
#endif

#endif // MCTS_H_INCLUDED
