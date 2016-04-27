#pragma once

#include "ptr.hpp"
using smart::ptr;

#include <stdio.h>
#include <vector>
#include <string>
#include <map>

// Pachi includes
extern "C" {
#include "move.h"
#include "board.h"
#include "engine.h"
#include "random.h"
}

// Minimal wrapper classes for Pachi structures
class PachiBoard {
    board* m_board;

public:
    explicit PachiBoard(int size) {
        m_board = board_init(NULL);
        board_resize(m_board, size);
        board_clear(m_board);
    }
    explicit PachiBoard(const PachiBoard& b) {
        m_board = (board*) malloc(sizeof(board));
        board_copy(m_board, b.m_board);
    }
    explicit PachiBoard(board* b, bool copy) {
        if (copy) {
            m_board = (board*) malloc(sizeof(board));
            board_copy(m_board, b);
        } else {
            m_board = b; // take ownership
        }
    }
    ~PachiBoard() { board_done(m_board); }
    const board* pachiboard() const { return m_board; }
    board* pachiboard() { return m_board; }

    ptr<PachiBoard> clone() { return ptr<PachiBoard>(new PachiBoard(*this)); }
    int size() { return board_size(m_board) - 2; }

    friend bool operator==(const PachiBoard& l, const PachiBoard& r);
};
inline bool operator==(const PachiBoard& l, const PachiBoard& r) {
    if (board_size(l.m_board) != board_size(r.m_board)) {
        return false;
    }
    foreach_point(l.m_board) {
        if (board_at(l.m_board, c) != board_at(r.m_board, c)) {
            return false;
        }
    } foreach_point_end;
    return true;
}
typedef ptr<PachiBoard> PachiBoardPtr;


// Exposed through Cython
inline PachiBoardPtr CreatePachiBoard(int size) { return PachiBoardPtr(new PachiBoard(size)); }
void GetLegalMoves(PachiBoardPtr b, stone color, bool filter_suicides, std::vector<coord_t>* out);
inline std::vector<coord_t> GetLegalMoves(PachiBoardPtr b, stone color, bool filter_suicides) {
    std::vector<coord_t> out;
    GetLegalMoves(b, color, filter_suicides, &out);
    return out;
}
bool IsTerminal(PachiBoardPtr b);
inline float FastScore(PachiBoardPtr b) { return board_fast_score(b->pachiboard()); }
float OfficialScore(PachiBoardPtr b);
std::string ToString(PachiBoardPtr b);
void PlayInPlace(PachiBoardPtr b, const move& m);
PachiBoardPtr Play(PachiBoardPtr b, const move& m);
inline PachiBoardPtr Play(PachiBoardPtr b, coord_t coord, stone color) {
    move m = {coord, color};
    return Play(b, m);
}
PachiBoardPtr PlayRandom(PachiBoardPtr b, stone color, coord_t *coord);

inline int i_from_xy(board* b, int x, int y) { return board_size(b)-2-y; }
inline int j_from_xy(board* b, int x, int y) { return x-1; }
inline stone board_atij(board* b, int i, int j) { return board_atxy(b, j+1, board_size(b)-2-i); }
inline int coord_ij(board* b, int i, int j) { return coord_xy(b, j+1, board_size(b)-2-i); }
inline int i_from_coord(board* b, coord_t c) { return i_from_xy(b, coord_x(c, b), coord_y(c, b)); }
inline int j_from_coord(board* b, coord_t c) { return j_from_xy(b, coord_x(c, b), coord_y(c, b)); }

// Wrapper for Pachi engines. Frees engine when destroyed.
class PachiEngine {
    engine* m_engine;
    const std::string m_engine_type;
    PachiBoardPtr m_board; // Stores the current board for the game. Must stay alive during the lifetime of the engine.

public:
    PachiEngine(PachiBoardPtr b, const std::string& engine_type, std::string arg);
    ~PachiEngine();

    PachiBoardPtr get_curr_board() { return m_board; }
    coord_t genmove(stone curr_color, const std::string& timestr);
    void notify(coord_t move_coord, stone move_color);
};
