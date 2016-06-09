#include "goutil.hpp"
#include "exceptions.hpp"

#include <sstream>
#include <stdexcept>
#include <iostream>
#include <algorithm>

extern "C" {
#include "stone.h"
#include "engine.h"
#include "uct/uct.h"
#include "uct/internal.h"
#include "montecarlo/montecarlo.h"
#include "random/random.h"
#include "timeinfo.h"
#include "fbook.h"
#include "playout.h"
#include "ownermap.h"
#include "mq.h"
}

void GetLegalMoves(PachiBoardPtr b, stone color, bool filter_suicides, std::vector<coord_t>* out) {
    out->clear(); out->push_back(pass);
    board* pb = b->pachiboard();
    foreach_free_point(pb) {
        assert(board_at(pb, c) == S_NONE);
        bool valid = filter_suicides ? board_is_valid_play_no_suicide(pb, color, c) : board_is_valid_play(pb, color, c);
        if (valid) {
            out->push_back(c);
        }
    } foreach_free_point_end;
}

bool IsTerminal(PachiBoardPtr b) {
    board* pb = b->pachiboard();
    // last move is a resign
    // if (b->num_moves() >= 1 && is_resign(b->get_last_move(0).coord)) {
    if (pb->moves > 0 && is_resign(pb->last_move.coord)) {
        return true;
    }
    // last two moves are passes
    // if (b->num_moves() >= 2 && is_pass(b->get_last_move(0).coord) && is_pass(b->get_last_move(1).coord)) {
    if (pb->moves > 0 && is_pass(pb->last_move.coord) && pb->last_move.color != S_NONE && is_pass(pb->last_move2.coord) && pb->last_move2.color != S_NONE) {
        return true;
    }
    return false;
}

// RAII for board_ownermap
struct OwnerMap {
    board_ownermap ownermap;
    OwnerMap(board *b) {
        ownermap.map = (sig_atomic_t (*)[S_MAX]) malloc(board_size2(b) * sizeof(ownermap.map[0]));
        memset(ownermap.map, 0, board_size2(b) * sizeof(ownermap.map[0]));
        board_ownermap_fill(&ownermap, b);
    }
    ~OwnerMap() { free(ownermap.map); }
};
typedef ptr<OwnerMap> OwnerMapPtr;

/* How big proportion of ownermap counts must be of one color to consider
 * the point sure. */
static move_queue GetDeadGroups(board *b, OwnerMapPtr ownermap) {
    /* Make sure enough playouts are simulated to get a reasonable dead group list. */
    // while (u->ownermap.playouts < GJ_MINGAMES)
    //     uct_playout(u, b, color, u->t);
    if (!ownermap) {
        ownermap.reset(new OwnerMap(b));
    }

    move_queue mq = { .moves = 0 };
    gj_state gs_array[board_size2(b)];
    struct group_judgement gj = { .thres = GJ_THRES, .gs = gs_array };
    board_ownermap_judge_groups(b, &ownermap->ownermap, &gj);
    groups_of_status(b, &gj, GS_DEAD, &mq);

    return mq;
}

float OfficialScore(PachiBoardPtr b) {
    move_queue mq = GetDeadGroups(b->pachiboard(), OwnerMapPtr());
    return board_official_score(b->pachiboard(), &mq);
}


static std::string trim(const std::string &s) {
    auto notspace = [](char c){ return !std::isspace(c); };
    auto a = std::find_if(s.begin(), s.end(), notspace);
    auto b = std::find_if(s.rbegin(), std::string::const_reverse_iterator(a), notspace).base();
    return std::string(a, b);
}
std::string ToString(PachiBoardPtr b) {
    char *s = board_print(b->pachiboard(), NULL);
    std::string out(s);
    free(s); // s is constructed with strdup, so we need to free it here
    return trim(out);
}


static inline bool moves_equal(const move& m1, const move& m2) {
    return m1.coord == m2.coord && m1.color == m2.color;
}
void PlayInPlace(PachiBoardPtr b, const move& m) {
    board* pb = b->pachiboard();

    if (board_play(pb, const_cast<move*>(&m)) < 0) {
        char* tmp = coord2str(m.coord, pb);
        std::stringstream ss;
        ss << "Illegal move by " << (m.color == S_BLACK ? "black" : "white") << " at " << tmp << ". Current board:\n";
        ss << ToString(b);
        free(tmp);
        throw IllegalMove(ss.str());
    }
}

PachiBoardPtr Play(PachiBoardPtr b, const move& m) {
    ptr<PachiBoard> new_state = b->clone();
    PlayInPlace(new_state, m);
    return new_state;
}

void PlayRandomInPlace(PachiBoardPtr b, stone color, coord_t *coord) {
    board_play_random(b->pachiboard(), color, coord, NULL, NULL);
}

PachiBoardPtr PlayRandom(PachiBoardPtr b, stone color, coord_t *coord) {
    ptr<PachiBoard> new_state = b->clone();
    PlayRandomInPlace(new_state, color, coord);
    return new_state;
}


PachiEngine::PachiEngine(PachiBoardPtr bptr, const std::string& engine_type, std::string arg) : m_engine_type(engine_type), m_board(bptr) {
    auto engine_init_fn = engine_random_init;
    if (engine_type == "random") {
        engine_init_fn = engine_random_init;
    } else if (engine_type == "montecarlo") {
        engine_init_fn = engine_montecarlo_init;
    } else if (engine_type == "uct") {
        engine_init_fn = engine_uct_init;
        // seems to leak memory when pondering is on
        if (arg != "") { arg += ","; }
        arg += "pondering=0";
    } else {
        throw PachiEngineError("engine not supported: " + engine_type);
    }

    char* tmp_arg = arg == "" ? nullptr : strdup(arg.c_str());
    m_engine = engine_init_fn(tmp_arg, m_board->pachiboard());
    if (tmp_arg != nullptr) { free(tmp_arg); }
}

coord_t PachiEngine::genmove(stone curr_color, const std::string& timestr) {
    // Set pachi timing options (max sims, etc.)
    time_info ti;
    ti.period = time_info::TT_NULL;
    if (timestr != "" && !time_parse(&ti, const_cast<char*>(timestr.c_str()))) {
        std::stringstream ss;
        ss << "Invalid timekeeping specification for Pachi: " << timestr << '\n';
        ss << "Format:\n";
        ss << "*   =NUM - fixed number of simulations per move\n";
        ss << "*   NUM - number of seconds to spend per move (can be floating_t)\n";
        ss << "*   _NUM - number of seconds to spend per game\n";
        throw PachiEngineError(ss.str());
    }

    // Play
    coord_t* c = m_engine->genmove(m_engine, m_board->pachiboard(), &ti, curr_color, false);
    coord_t out = *c;
    coord_done(c);
    return out;
}

void PachiEngine::notify(coord_t move_coord, stone move_color) {
    if (!m_engine->notify_play) { return; }
    move m = { move_coord, move_color };
    m_engine->notify_play(m_engine, m_board->pachiboard(), &m, NULL);
}

PachiEngine::~PachiEngine() {
    if (m_engine->stop) { m_engine->stop(m_engine); }
    if (m_engine->done) { m_engine->done(m_engine); }
    if (m_engine->data) { free(m_engine->data); }
    free(m_engine);
}
