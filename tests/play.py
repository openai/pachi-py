import argparse
import pachi_py

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--pachi_vs_random', action='store_true')
    args = parser.parse_args()

    if args.pachi_vs_random:
        moves = pachi_py.play(pachi_py.random_policy, pachi_py.make_pachi_policy(pachi_threads=2, pachi_timestr='_2400'), board_size=9)
    else:
        def user_policy_fn(curr_state, prev_state, prev_action):
            print curr_state.board
            coordstr = raw_input('%s> ' % pachi_py.color_to_str(curr_state.color))
            return curr_state.board.str_to_coord(coordstr)
        moves = pachi_py.play(user_policy_fn, pachi_py.make_pachi_policy(), board_size=9)

    print
    final_board = moves[-1][2].board
    print 'Final board state:'
    print final_board
    score = final_board.official_score
    print 'Final score:', score
    print 'White wins!' if score > 0 else 'Black wins!'

if __name__ == '__main__':
    main()
