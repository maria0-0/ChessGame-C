#ifndef CHESS_AI_H
#define CHESS_AI_H

// Structure to store a move
typedef struct {
    int from_x, from_y;
    int to_x, to_y;
    int score;
} Move;

// Function declarations
Move find_best_move();
void make_move(Move move);
void make_ai_move(void);
void move_to_notation(Move move, char* notation);
void test_ai();
void print_chess_board();
void play_ai_vs_ai(int num_moves);
void play_human_vs_ai(int human_is_white);

#endif // CHESS_AI_H 