#ifndef NIM_H_INCLUDED
#define NIM_H_INCLUDED

#include <cstddef>
#include <functional>
#include <numeric>
#include <utility>
#include <vector>
#include "../util/arith.h"

// An array of N piles of stones
template<std::size_t N>
struct Nim
{
    std::size_t piles[N];
    // Move: (# of pile, # of stones)
    typedef std::pair<std::size_t, std::size_t> Move;
    // Vector of moves
    typedef std::vector<Move> Moves;
    // Constructor
    Nim(std::size_t const * p) { std::copy(p, p + N, piles); }
    // Return the number of stones left.
    std::size_t sum() const
    { return std::accumulate(piles, piles + N, static_cast<std::size_t>(0)); }
    // XOR of all piles
    std::size_t value() const
    { return std::accumulate(piles, piles + N, static_cast<std::size_t>(0),
                             std::bit_xor<std::size_t>()); }
    // Return if it is a win for the 1st player.
    bool win() const { return value() != 0; }
    // Return all the legal moves.
    Moves moves(bool = true) const
    {
        Moves result;
        for (std::size_t pile(0); pile < N; ++pile)
            for (std::size_t n(1); n <= piles[pile]; ++n)
                result.push_back(Move(pile, n));

        return result;
    }
    bool legal(Move move, bool) const
    {
        return move.first < N && move.second <= piles[move.first];
    }
    void play(Move move, bool) { piles[move.first] -= move.second; }
    // Return winning move if it is a win. Otherwise return (0, 0).
    Move winningmove() const
    {
        std::size_t const v(value());
        if (v == 0u) return Move(0, 0);
        // The highest set bit in v
        std::size_t const n(1u << util::log2(v));
        // Find a pile whose same bit is set.
        std::size_t pile(0);
        for ( ; pile < N; ++pile)
            if (piles[pile] & n != 0) break;

        return Move(pile, piles[pile] ^ n);
    }
};

#endif // NIM_H_INCLUDED
