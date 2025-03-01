#ifndef GOM_H_INCLUDED
#define GOM_H_INCLUDED

#include <algorithm>
#include <iostream>
#include <utility>
#include <vector>

// M * N Gomoko, with K stones in a row as a win.
template<std::size_t M, std::size_t N, std::size_t K>
struct Gom
{
    // The board, 1 = our stone, -1 = their stone, 0 = vacant
    int board[M][N];
    // Move = (row, column)
    typedef std::pair<std::size_t, std::size_t> Move;
    // Vector of moves
    typedef std::vector<Move> Moves;
    // Constructor
    Gom(int * p = NULL)
    {
        if (!p)
            new(board) int[M][N]();
        else
            std::copy(p, p + M*N, &board[0][0]);
    }
    // Return if the board is full.
    bool full() const { return std::find(board[0], board[M], 0) == board[M]; }
    friend std::ostream & operator<<(std::ostream & out, Gom const& gom)
    {
        for (std::size_t i(0); i < M; ++i)
        {
            for (std::size_t j(0); j < N; ++j)
            {
                int who(gom.board[i][j]);
                out << (who == 1 ? 'O' : who == 0 ? ' ' :
                    who == -1 ? 'X' : '?');
            }
            out << std::endl;
        }

        return out;
    }
    // Check if there is a run of K stones from begin to end with step apart.
    // Return 1 if there is ours, -1 if there is theirs, and 0 if there is non.
    static int run(const int * begin, const int * end, std::size_t step)
    {
        std::size_t count(0);
        int who(0);

        for ( ; begin < end && count < K; begin += step)
        {
            if (*begin == 0) // Reset.
                count = 0;
            else if (count == 0) // Start counting.
            {
                who = *begin;
                ++count;
            }
            else if (*begin == who) // Continue counting.
                ++count;
            else // Reset.
                count = 0;
        }

        return count == K ? who : 0;
    }
    // Check if there is a collection of runs.
    // Return 1 if there is ours, -1 if there is theirs, and 0 if there is non.
    static int run(const int * begin, const int * end, std::size_t step,
                   long beginstep, long endstep, std::size_t count)
    {
        for ( ; count > 0; --count)
        {
            int const who(run(begin, end, step));
            if (who != 0) return who;

            begin += beginstep;
            end += endstep;
        }

        return 0;
    }
    // Return 1 if we won, -1 if they won. Otherwise return 0.
    int winner() const
    {
        int who(0);
        // Check rows.
        who = run(&board[0][0], &board[1][0], 1, N, N, M);
        if (who != 0) return who;
        // Check columns.
        who = run(&board[0][0], &board[M][0], N, 1, 1, N);
        if (who != 0) return who;
        // No winner.
        return 0;
    }
    // Return all the legal moves.
    Moves moves(bool = true) const
    {
        Moves result;
        for (std::size_t i(0); i < M; ++i)
            for (std::size_t j(0); j < N; ++j)
                if (board[i][j] == 0)
                    result.push_back(Move(i, j));

        return result;
    }
    bool legal(Move move, bool) const
    {
        return move.first < M && move.second < N && board[move.first][move.second] == 0;
    }
    void play(Move move, bool ourturn)
    {
        board[move.first][move.second] = ourturn ? 1 : -1;
    }
};

#endif // GOM_H_INCLUDED
