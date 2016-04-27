import numpy as np
cimport numpy as np
cimport cython
from libc.stdlib cimport malloc, free
from libc.stdio cimport FILE
from libcpp.vector cimport vector
from libcpp.string cimport string
from cython.operator cimport dereference as deref
from cpython.ref cimport PyObject


##### Wrapper API declarations #####
# These must go first before Pachi imports, because they use extern "C" on Pachi headers
cdef extern from "ptr.hpp":
    cppclass ptr[T]:
        ptr()
        T* get()
        void reset(T*)
        void assign(const ptr[T]&)

# Exceptions wrappers for C++ code
# see http://stackoverflow.com/a/29002414
class IllegalMove(RuntimeError):
    pass
class PachiEngineError(RuntimeError):
    pass
cdef public PyObject* _PyIllegalMove = <PyObject*> IllegalMove
cdef public PyObject* _PyPachiEngineError = <PyObject*> PachiEngineError
cdef extern from "exceptions.hpp":
    void raise_py_error()

# C++ wrapper for Pachi
cdef extern from "goutil.hpp":
    enum stone:
        S_NONE
        S_BLACK
        S_WHITE
        S_OFFBOARD
        S_MAX
    ctypedef int coord_t

    cppclass PachiBoard:
        board* pachiboard()
        ptr[PachiBoard] clone()
        int size()
    ctypedef ptr[PachiBoard] PachiBoardPtr
    PachiBoardPtr CreatePachiBoard(int size)

    void GetLegalMoves(PachiBoardPtr b, stone color, bint filter_suicides, vector[coord_t]* out)
    bint IsTerminal(PachiBoardPtr)
    float FastScore(PachiBoardPtr)
    float OfficialScore(PachiBoardPtr)
    string ToString(PachiBoardPtr)
    void PlayInPlace(PachiBoardPtr b, const move& m) except +raise_py_error
    PachiBoardPtr Play(PachiBoardPtr, const move&) except +raise_py_error
    PachiBoardPtr PlayRandom(PachiBoardPtr b, stone color, coord_t *coord)


    # Coordinate representation conversion
    # x/y are Pachi coordinates
    # i/j are the encoding used for training
    int i_from_xy(board* b, int x, int y)
    int j_from_xy(board* b, int x, int y)
    stone board_atij(board* b, int i, int j)
    int coord_ij(board* b, int i, int j)
    int i_from_coord(board* b, coord_t c)
    int j_from_coord(board* b, coord_t c)

    cppclass PachiEngine:
        PachiEngine(PachiBoardPtr b, const string& engine_type, string arg) except +raise_py_error
        PachiBoardPtr get_curr_board()
        coord_t genmove(stone curr_color, const string& timestr) except +raise_py_error
        void notify(coord_t move_coord, stone move_color)


##### Pachi API declarations #####

cdef extern from "stone.h":
    enum stone:
        S_NONE
        S_BLACK
        S_WHITE
        S_OFFBOARD
        S_MAX

    stone str2stone(char *str)
    stone pachi_stone_other "stone_other" (stone s)

cdef extern from "move.h":
    ctypedef int coord_t

    # void coord_done(coord_t *c)

    # # Return coordinate string in a dynamically allocated buffer. Thread-safe.
    char *coord2str(coord_t c, board *b)
    coord_t *str2coord(char *str, int board_size)

    struct move:
        coord_t coord
        stone color

    # Special moves
    coord_t pass_coord "pass"
    coord_t resign_coord "resign"

cdef extern from "board.h":
    int GROUP_REFILL_LIBS
    struct group:
        int libs
    ctypedef coord_t group_t

    struct board:
        pass
    int board_size(board*) # includes padding, so returns 19 + 2
    # stone board_at(board* b, int c)
    # stone board_atxy(board* b, int x, int y)
    # group_t group_at(board* b, int c)
    # group board_group_info(board* b, int g)

    void board_handicap(board *board, int stones, FILE *f)
    bint board_set_rules(board *board, char *name)

cdef extern from "random.h":
    void fast_srandom(unsigned long seed)


# Internal utility functions


cdef enum Channel:
    CHAN_CELL_BLACK = 0
    CHAN_CELL_WHITE
    CHAN_CELL_EMPTY
cdef int _NUM_FEATURE_CHANNELS = 3
cdef void encode_board(board* b, np.ndarray[np.int_t, ndim=3] out_x):
    cdef int s = board_size(b) - 2
    assert out_x.shape[0] == _NUM_FEATURE_CHANNELS and out_x.shape[1] == out_x.shape[2] == s
    out_x[...] = 0

    cdef unsigned int i, j
    cdef stone curr_stone
    for i in range(s):
        for j in range(s):
            curr_stone = board_atij(b, i, j)
            # Cell color
            if curr_stone == S_BLACK:
                out_x[<unsigned int>CHAN_CELL_BLACK,i,j] = 1
            elif curr_stone == S_WHITE:
                out_x[<unsigned int>CHAN_CELL_WHITE,i,j] = 1
            elif curr_stone == S_NONE:
                out_x[<unsigned int>CHAN_CELL_EMPTY,i,j] = 1
            else:
                assert False

# Python board wrapper
cdef PyPachiBoard wrap_board(PachiBoardPtr b):
    return PyPachiBoard()._set(b)

cdef class PyPachiBoard:
    cdef PachiBoardPtr _bptr
    cdef PachiBoard* _b # for convenience. set to self._bptr.get()
    cdef int _size

    cdef _set(self, PachiBoardPtr b):
        self._bptr.assign(b)
        self._b = self._bptr.get()
        self._size = self._b.size()
        return self

    def __repr__(self):
        return ToString(self._bptr)

    def clone(self):
        return wrap_board(self._bptr.get().clone())

    def get_stones(self, stone color):
        cdef np.ndarray[np.int_t, ndim=2] stones = np.empty((self._size*self._size, 2), dtype=np.int)
        cdef int i, j
        cdef unsigned int num_stones = 0
        for i in range(self._size):
            for j in range(self._size):
                if board_atij(self._b.pachiboard(), i, j) == color:
                    stones[num_stones,0] = i
                    stones[num_stones,1] = j
                    num_stones += 1
        return stones[:num_stones,:]

    def get_legal_coords(self, stone color, bint filter_suicides=False):
        cdef vector[coord_t] legal_coords
        GetLegalMoves(self._bptr, color, filter_suicides, &legal_coords)
        return legal_coords

    def encode(self, np.ndarray[np.int_t, ndim=3] out_x=None):
        if out_x is None:
            out_x = np.zeros((NUM_FEATURE_CHANNELS, self._size, self._size), dtype=int)
        encode_board(self._b.pachiboard(), out_x)
        return out_x

    def play(self, int coord, stone color):
        assert color == S_BLACK or color == S_WHITE
        cdef move m
        m.color = color
        m.coord = coord
        return wrap_board(Play(self._bptr, m))

    def play_random(self, stone color, bint return_action=False):
        cdef coord_t c
        cdef PyPachiBoard out = wrap_board(PlayRandom(self._bptr, color, &c))
        if not return_action: return out
        return out, c

    def play_inplace(self, int coord, stone color):
        assert color == S_BLACK or color == S_WHITE
        cdef move m
        m.color = color
        m.coord = coord
        PlayInPlace(self._bptr, m)

    property size:
        def __get__(self):
            return self._size
    property num_stones:
        def __get__(self):
            cdef int i, j, n = 0
            for i in range(self._size):
                for j in range(self._size):
                    if board_atij(self._b.pachiboard(), i, j) != S_NONE:
                        n += 1
            return n
    property empty:
        def __get__(self):
            return self.num_stones == 0
    property black_stones:
        def __get__(self):
            return self.get_stones(S_BLACK)
    property white_stones:
        def __get__(self):
            return self.get_stones(S_WHITE)
    property is_terminal:
        def __get__(self):
            return IsTerminal(self._bptr)
    property fast_score:
        def __get__(self):
            return FastScore(self._bptr)
    property official_score:
        def __get__(self):
            return OfficialScore(self._bptr)

    def __richcmp__(PyPachiBoard self, PyPachiBoard other, int op):
        # TODO: use the C++ operator==
        if op != 2 and op != 3: return NotImplemented
        if other._size != self._size: return False
        cdef int i, j
        for i in range(self._size):
            for j in range(self._size):
                if board_atij(self._b.pachiboard(), i, j) != board_atij(other._b.pachiboard(), i, j):
                    return op == 3
        return op == 2

    def __hash__(self):
        # Not sure if this is the best hash implementation, but it seems to work
        cdef long h = 0
        cdef int i, j
        for i in range(self._size):
            for j in range(self._size):
                h = 101*h + board_atij(self._b.pachiboard(), i, j)
        return h

    def __getitem__(self, idx):
        if len(idx) != 2:
            raise IndexError('Must provide 2 indices')
        cdef int i = idx[0]
        cdef int j = idx[1]
        if not (0 <= i < self._size and 0 <= j < self._size):
            raise IndexError('Coordinates %d,%d out of range for board size %d' % (i, j, self._size))
        return board_atij(self._b.pachiboard(), i, j)

    ### Utilities ###

    def str_to_coord(self, char *s):
        cdef coord_t *cptr = str2coord(s, board_size(self._b.pachiboard()))
        cdef coord_t c = deref(cptr)
        free(cptr)
        # Sanity checking
        if c == pass_coord or c == resign_coord: return c
        if not (0 <= i_from_coord(self._b.pachiboard(), c) < self._size and
                0 <= j_from_coord(self._b.pachiboard(), c) < self._size):
            raise RuntimeError('Invalid coordinate %s' % s)
        return c

    def coord_to_str(self, coord_t c):
        cdef char* tmpstr = coord2str(c, self._b.pachiboard())
        cdef string s
        s += tmpstr
        free(tmpstr)
        return s

    def coord_to_ij(self, coord_t c):
        cdef int i = i_from_coord(self._b.pachiboard(), c)
        cdef int j = j_from_coord(self._b.pachiboard(), c)
        return i, j

    def ij_to_coord(self, int i, int j):
        return coord_ij(self._b.pachiboard(), i, j)


cdef class PyPachiEngine:
    cdef PachiEngine* _engine

    def __cinit__(self, PyPachiBoard b, const string& engine_type, const string& arg):
        self._engine = new PachiEngine(b._bptr, engine_type, arg)

    def __dealloc__(self):
        del self._engine

    @property
    def curr_board(self):
        return wrap_board(self._engine.get_curr_board())

    def genmove(self, stone curr_color, const string& timestr):
        return self._engine.genmove(curr_color, timestr)

    def notify(self, coord_t move_coord, stone move_color):
        self._engine.notify(move_coord, move_color)


##### Exposed constants #####
NUM_FEATURE_CHANNELS = _NUM_FEATURE_CHANNELS
WHITE = S_WHITE
BLACK = S_BLACK
EMPTY = S_NONE
PASS_COORD = pass_coord
RESIGN_COORD = resign_coord


##### Exposed functions #####
cpdef PyPachiBoard CreateBoard(int size):
    return wrap_board(CreatePachiBoard(size))

def pachi_srand(unsigned long seed):
    fast_srandom(seed)

def stone_other(stone s):
    return pachi_stone_other(s)

def color_to_str(stone s):
    if s == S_BLACK: return "black"
    elif s == S_WHITE: return "white"
    return "INVALID"
