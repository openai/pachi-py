import pachi_py
import numpy as np

def make_random_board(size):
    b = pachi_py.CreateBoard(size)
    c = pachi_py.BLACK
    for _ in range(0, 50):
        b = b.play(np.random.choice(b.get_legal_coords(c)), c)
        c = pachi_py.stone_other(c)
    return b

def test_board_sizes():
    for s in [9, 19]:
        b = pachi_py.CreateBoard(s)
        for player in [pachi_py.BLACK, pachi_py.WHITE]:
            assert len(b.get_legal_coords(player)) == s*s + 1, \
                'Starting board should have size**2 + 1 legal moves'

def test_board():
    b = make_random_board(19)
    assert all(b[ij[0],ij[1]] == pachi_py.BLACK for ij in b.black_stones)
    assert all(b[ij[0],ij[1]] == pachi_py.WHITE for ij in b.white_stones)

def test_illegal_move():
    b = pachi_py.CreateBoard(9).play(14, pachi_py.WHITE)
    try:
        b.play(14, pachi_py.BLACK)
    except pachi_py.IllegalMove:
        return
    assert False, 'IllegalMove exception should have been raised'
