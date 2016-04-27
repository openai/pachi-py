THIS IS UNUSED
assert False


cdef int count_liberties(board* b, int c):
    assert MAX_LIBERTY_COUNT <= GROUP_REFILL_LIBS
    return board_group_info(b, group_at(b, c)).libs


cdef int MAX_LIBERTY_COUNT = 4


# "player" is either S_BLACK or S_WHITE
# "move" is a Pachi coord_t
cdef class Transition:
    cdef public int color
    cdef public PyPachiBoard state
    cdef public int move
    cdef public PyPachiBoard next_state
    def __init__(self, int color, PyPachiBoard state, int move, PyPachiBoard next_state):
        assert color == 0 or color == 1
        self.color = color
        self.state = state
        self.move = move
        self.next_state = next_state
    def __richcmp__(Transition self, Transition other, int op):
        if op != 2 and op != 3: return NotImplemented
        cdef bint eq = self.color == other.color and self.state == other.state and self.move == other.move and self.next_state == other.next_state
        return eq if op == 2 else not eq



cdef class PyRolloutResult:
    cdef RolloutResult r
    def __cinit__(self, RolloutResult r_):
        self.r = r_
    property black_final_score:
        def __get__(self):
            return self.r.black_final_score
    property white_final_score:
        def __get__(self):
            return self.r.white_final_score
    property black_resigned:
        def __get__(self):
            return self.r.black_resigned
    property white_resigned:
        def __get__(self):
            return self.r.white_resigned
    property winner:
        def __get__(self):
            return color2player(Winner(self.r))

cdef object _opponent_policy = None
cdef coord_t _opponent_policy_wrapper(PachiBoardPtr b, stone color):
    cdef int action = _opponent_policy(wrap_board(b), color2player(color))
    return action2coord(b.get().pachiboard(), action)

cdef object collect_transitions(const vector[PachiBoardPtr]& bhist, const vector[move]& mhist):
    transitions = []
    cdef int i, a
    for i in range(mhist.size()):
        a = coord2action(bhist[i].get().pachiboard(), mhist[i].coord)
        if a == RESIGN_ACTION_ID: break # don't expose resigns
        transitions.append(Transition(
            player=color2player(mhist[i].color),
            state=wrap_board(bhist[i]),
            action=a,
            next_state=wrap_board(bhist[i+1])
        ))
    return transitions

def rollout_against_pachi(
        PyPachiBoard init_b, int starting_player,
        int pachi_player, string pachi_engine_type, string pachi_engine_arg, string pachi_timestr,
        object opponent_policy_func):
    cdef vector[PachiBoardPtr] bhist
    cdef vector[move] mhist
    global _opponent_policy; _opponent_policy = opponent_policy_func
    cdef PyRolloutResult result = PyRolloutResult(RolloutAgainstPachi(
        init_b._bptr,
        player2color(starting_player),
        player2color(pachi_player), pachi_engine_type, pachi_engine_arg, pachi_timestr,
        _opponent_policy_wrapper,
        &bhist, &mhist))
    assert bhist.size() == mhist.size() + 1
    return result, collect_transitions(bhist, mhist)



### Serialization of transition lists ###
def save_games(games):
    cdef int out_size = 0
    for transitions in games:
        out_size += 2 + 2*len(transitions)

    cdef np.ndarray[np.uint8_t,ndim=1] out = np.empty(out_size, np.uint8)
    cdef PyPachiBoard init_state
    cdef Transition t
    cdef unsigned int pos = 0
    for transitions in games:
        init_state = transitions[0].state
        assert init_state.empty, "only supports games starting from the empty state"
        assert 0 <= init_state.size() < 256
        assert 0 <= len(transitions) < 256
        out[pos] = init_state.size()
        out[pos+1] = len(transitions)
        pos += 2
        for t in transitions:
            assert 0 <= t.player < 256
            assert 0 <= t.action < 256
            out[pos] = t.player
            out[pos+1] = t.action
            pos += 2
    assert pos == out_size

    return out

def reconstruct_games(np.uint8_t[:] data):
    cdef int i, gamelen, boardsize, pos = 0
    cdef vector[np.uint8_t] players, actions
    cdef PyPachiBoard b, b2

    games = []
    while pos < len(data):
        # read board size, game length, and actions
        boardsize = data[pos]
        gamelen = data[pos+1]
        players.clear(); actions.clear()
        for i in range(gamelen):
            players.push_back(data[pos+2 + 2*i])
            actions.push_back(data[pos+2 + 2*i+1])
        pos += 2 + 2*gamelen

        # replay
        transitions = []
        b = CreateBoard(boardsize)
        for i in range(gamelen):
            b2 = b.play(actions[i], players[i])
            transitions.append(Transition(
                player=players[i], state=b, action=actions[i], next_state=b2))
            b = b2
        games.append(transitions)

        if len(games) % 1000 == 0:
            print '[%.2f%%]' % (100.*float(pos)/len(data))

    assert pos == len(data)
    return games

## Deserialization from SGF ###
cdef move move_from_gtp(board* b, char* colorstr, char* coordstr):
    cdef move m
    m.color = str2stone(colorstr)
    cdef coord_t *c = str2coord(coordstr, board_size(b))
    m.coord = deref(c)
    coord_done(c)
    return m
cdef PyPachiBoard play_move(PyPachiBoard b, move m):
    return wrap_board(Play(b._bptr, m))
cdef PyPachiBoard play_gtp(PyPachiBoard b, char* colorstr, char* coordstr):
   return play_move(b, move_from_gtp(b._b.pachiboard(), colorstr, coordstr))

from pygo.sgflib import SGFParser
def ReplaySGF(const char* filename):
    # Adapted from sgf2gtp.py in pachi

    with open(filename, 'r') as f:
        sgfdata = f.read()
    col = SGFParser(sgfdata).parse()
    assert len(col) == 1

    gametree = col[0]

    class UnknownNode(Exception):
        pass

    def get_atr(node, atr):
        try:
            return node.data[atr].data[0]
        except KeyError:
            return None

    def get_setup(node, atr):
        try:
            return node.data[atr].data[:]
        except KeyError:
            return None

    def col2num(column, board_size):
        a, o, z = map(ord, ['a', column, 'z'])
        if a <= o <= z:
            return a + board_size - o
        raise Exception( "Wrong column character: '%s'"%(column,) )

    def is_pass_move(coord, board_size):
        # the pass move is represented either by [] ( = empty coord )
        # OR by [tt] (for boards <= 19 only)
        return len(coord) == 0 or ( board_size <= 19 and coord == 'tt' )

    def record_step(PyPachiBoard b, object coord):
        cdef move m
        if is_pass_move(coord, bsize):
            m = move_from_gtp(b._b.pachiboard(), player_next, 'pass')
        else:
            x, y = coord
            # The reason for this incredibly weird thing is that
            # the GTP protocol excludes `i` in the coordinates
            # (do you see any purpose in this??)
            if x >= 'i':
                x = chr(ord(x)+1)
            y = str(col2num(y, bsize))
            m = move_from_gtp(b._b.pachiboard(), player_next, x+y)

        cdef PyPachiBoard next_b = play_move(b, m)
        return Transition(
            m.color,
            b,
            m.coord,
            next_b
        )

    # cursor for tree traversal
    c = gametree.cursor()
    # first node is the header
    header = c.node

    handicap = get_atr(header, 'HA')
    bsize = int(get_atr(header, 'SZ') or 19)
    # assert bsize == 19

    komi = get_atr(header, 'KM')
    player_next, player_other = 'B', 'W'
    setup_black = get_setup(header, 'AB')
    setup_white = get_setup(header, 'AW')
    ruleset = get_atr(header, 'RU')

    cdef PyPachiBoard b = CreateBoard(bsize)

    # if komi:
    #     b._pachi_board.komi = float(komi)
    if handicap and handicap != '0':
        #board_handicap(b._pachi_board, int(handicap), NULL)
        # The dataset seems to have explicit AB/AW for handicap stones
        # so don't use Pachi's add-handicap method here
        player_next, player_other = player_other, player_next
    if setup_black:
        for item in setup_black:
            x, y = item
            if x >= 'i':
                x = chr(ord(x)+1)
            y = str(col2num(y, bsize))
            b = play_gtp(b, 'B', x+y)
    if setup_white:
        for item in setup_white:
            x, y = item
            if x >= 'i':
                x = chr(ord(x)+1)
            y = str(col2num(y, bsize))
            b = play_gtp(b, 'W', x+y)
    if ruleset:
        if ruleset.lower() == 'nz':
            # board_set_rules only takes new_zealand
            ruleset = 'new_zealand'
        if not board_set_rules(b._b.pachiboard(), ruleset):
            print 'Unrecognized ruleset %s for %s, defaulting to Pachi default' % (ruleset, filename)

    # walk the game tree forward
    game_transitions = []
    while True:
        # sgf2gtp.pl ignores n = 0
        if c.atEnd:
            break
        c.next()

        coord = get_atr(c.node, player_next)
        if coord != None:
            game_transitions.append(record_step(b, coord))
            b = game_transitions[-1].next_state
        else:
            # MAYBE white started?
            # or one of the players plays two time in a row
            player_next, player_other = player_other, player_next
            coord = get_atr(c.node, player_next)
            if coord != None:
                game_transitions.append(record_step(b, coord))
                b = game_transitions[-1].next_state
            elif len(c.node) != 0:
                # TODO handle weird sgf files better
                raise UnknownNode

        player_next, player_other = player_other, player_next
    return game_transitions
