#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <unistd.h>  // for sleep
#include "chess_ai.h"

#define SIZE 8
#define MAX_DEPTH 3  // Adjust this value to control how many moves ahead the AI looks
#define INF 1000000
#define CHECK_PENALTY -500  // Large penalty for being in check
#define CHECKMATE_SCORE -100000  // Very large penalty for checkmate

// Static evaluation weights
#define DOUBLED_PAWN_PENALTY -10
#define ISOLATED_PAWN_PENALTY -20
#define PASSED_PAWN_BONUS 30
#define BISHOP_PAIR_BONUS 30
#define ROOK_ON_OPEN_FILE_BONUS 20
#define ROOK_ON_SEMI_OPEN_FILE_BONUS 10
#define CONNECTED_ROOKS_BONUS 15
#define CENTER_CONTROL_BONUS 10
#define KING_SHIELD_BONUS 10
#define MOBILITY_BONUS 5

// Center squares definition
const int center_squares[4][2] = {{3,3}, {3,4}, {4,3}, {4,4}};
const int extended_center_squares[16][2] = {
    {2,2}, {2,3}, {2,4}, {2,5},
    {3,2}, {3,3}, {3,4}, {3,5},
    {4,2}, {4,3}, {4,4}, {4,5},
    {5,2}, {5,3}, {5,4}, {5,5}
};

// External declarations
extern char board[SIZE][SIZE];
extern int is_white_turn;
extern char last_error[128];
extern int is_valid_move(int sx, int sy, int dx, int dy);
extern int is_king_in_check(char king);  // Add this external declaration

// Forward declarations
int generate_moves(Move* moves);

// Piece values for evaluation
const int PAWN_VALUE = 100;
const int KNIGHT_VALUE = 320;
const int BISHOP_VALUE = 330;
const int ROOK_VALUE = 500;
const int QUEEN_VALUE = 900;
const int KING_VALUE = 20000;

// Simple piece values for naive evaluation
#define NAIVE_PAWN_VALUE 100
#define NAIVE_KNIGHT_VALUE 300
#define NAIVE_BISHOP_VALUE 300
#define NAIVE_ROOK_VALUE 500
#define NAIVE_QUEEN_VALUE 900
#define NAIVE_KING_VALUE 20000

// Piece square tables for positional evaluation
const int pawn_table[SIZE][SIZE] = {
    {0,   0,   0,   0,   0,   0,   0,   0},
    {50,  50,  50,  50,  50,  50,  50,  50},
    {10,  10,  20,  30,  30,  20,  10,  10},
    {5,   5,  10,  25,  25,  10,   5,   5},
    {0,   0,   0,  20,  20,   0,   0,   0},
    {5,  -5, -10,   0,   0, -10,  -5,   5},
    {5,  10,  10, -20, -20,  10,  10,   5},
    {0,   0,   0,   0,   0,   0,   0,   0}
};

const int knight_table[SIZE][SIZE] = {
    {-50, -40, -30, -30, -30, -30, -40, -50},
    {-40, -20,   0,   0,   0,   0, -20, -40},
    {-30,   0,  10,  15,  15,  10,   0, -30},
    {-30,   5,  15,  20,  20,  15,   5, -30},
    {-30,   0,  15,  20,  20,  15,   0, -30},
    {-30,   5,  10,  15,  15,  10,   5, -30},
    {-40, -20,   0,   5,   5,   0, -20, -40},
    {-50, -40, -30, -30, -30, -30, -40, -50}
};

const int bishop_table[SIZE][SIZE] = {
    {-20, -10, -10, -10, -10, -10, -10, -20},
    {-10,   0,   0,   0,   0,   0,   0, -10},
    {-10,   0,   5,  10,  10,   5,   0, -10},
    {-10,   5,   5,  10,  10,   5,   5, -10},
    {-10,   0,  10,  10,  10,  10,   0, -10},
    {-10,  10,  10,  10,  10,  10,  10, -10},
    {-10,   5,   0,   0,   0,   0,   5, -10},
    {-20, -10, -10, -10, -10, -10, -10, -20}
};

const int rook_table[SIZE][SIZE] = {
    { 0,  0,  0,  0,  0,  0,  0,  0},
    { 5, 10, 10, 10, 10, 10, 10,  5},
    {-5,  0,  0,  0,  0,  0,  0, -5},
    {-5,  0,  0,  0,  0,  0,  0, -5},
    {-5,  0,  0,  0,  0,  0,  0, -5},
    {-5,  0,  0,  0,  0,  0,  0, -5},
    {-5,  0,  0,  0,  0,  0,  0, -5},
    { 0,  0,  0,  5,  5,  0,  0,  0}
};

const int queen_table[SIZE][SIZE] = {
    {-20, -10, -10, -5, -5, -10, -10, -20},
    {-10,   0,   0,  0,  0,   0,   0, -10},
    {-10,   0,   5,  5,  5,   5,   0, -10},
    { -5,   0,   5,  5,  5,   5,   0,  -5},
    {  0,   0,   5,  5,  5,   5,   0,  -5},
    {-10,   5,   5,  5,  5,   5,   0, -10},
    {-10,   0,   5,  0,  0,   0,   0, -10},
    {-20, -10, -10, -5, -5, -10, -10, -20}
};

const int king_middle_table[SIZE][SIZE] = {
    {-30, -40, -40, -50, -50, -40, -40, -30},
    {-30, -40, -40, -50, -50, -40, -40, -30},
    {-30, -40, -40, -50, -50, -40, -40, -30},
    {-30, -40, -40, -50, -50, -40, -40, -30},
    {-20, -30, -30, -40, -40, -30, -30, -20},
    {-10, -20, -20, -20, -20, -20, -20, -10},
    { 20,  20,   0,   0,   0,   0,  20,  20},
    { 20,  30,  10,   0,   0,  10,  30,  20}
};

const int king_end_table[SIZE][SIZE] = {
    {-50, -40, -30, -20, -20, -30, -40, -50},
    {-30, -20, -10,   0,   0, -10, -20, -30},
    {-30, -10,  20,  30,  30,  20, -10, -30},
    {-30, -10,  30,  40,  40,  30, -10, -30},
    {-30, -10,  30,  40,  40,  30, -10, -30},
    {-30, -10,  20,  30,  30,  20, -10, -30},
    {-30, -30,   0,   0,   0,   0, -30, -30},
    {-50, -30, -30, -30, -30, -30, -30, -50}
};

// Function to count material on the board (used to determine game phase)
int count_material() {
    int material = 0;
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            switch (board[i][j]) {
                case 'Q': case 'q': material += QUEEN_VALUE; break;
                case 'R': case 'r': material += ROOK_VALUE; break;
                case 'B': case 'b': material += BISHOP_VALUE; break;
                case 'N': case 'n': material += KNIGHT_VALUE; break;
                case 'P': case 'p': material += PAWN_VALUE; break;
            }
        }
    }
    return material;
}

// Function to check if the current position is checkmate
int is_checkmate() {
    Move possible_moves[256];
    int move_count = generate_moves(possible_moves);
    
    // If there are no legal moves and the king is in check, it's checkmate
    return move_count == 0 && is_king_in_check(is_white_turn ? 'K' : 'k');
}

// Function to count pawns in a file
int count_pawns_in_file(int file, int is_white) {
    int count = 0;
    char pawn = is_white ? 'P' : 'p';
    for (int rank = 0; rank < SIZE; rank++) {
        if (board[rank][file] == pawn) {
            count++;
        }
    }
    return count;
}

// Function to check if a pawn is isolated
int is_isolated_pawn(int file, int is_white) {
    char pawn = is_white ? 'P' : 'p';
    // Check left file
    int left_file_has_pawn = 0;
    if (file > 0) {
        for (int r = 0; r < SIZE; r++) {
            if (board[r][file-1] == pawn) {
                left_file_has_pawn = 1;
                break;
            }
        }
    }
    // Check right file
    int right_file_has_pawn = 0;
    if (file < SIZE-1) {
        for (int r = 0; r < SIZE; r++) {
            if (board[r][file+1] == pawn) {
                right_file_has_pawn = 1;
                break;
            }
        }
    }
    return !left_file_has_pawn && !right_file_has_pawn;
}

// Function to check if a pawn is passed
int is_passed_pawn(int rank, int file, int is_white) {
    char enemy_pawn = is_white ? 'p' : 'P';
    int direction = is_white ? -1 : 1;
    
    // Check if there are any enemy pawns ahead in the same or adjacent files
    for (int f = file-1; f <= file+1; f++) {
        if (f < 0 || f >= SIZE) continue;
        for (int r = rank + direction; r >= 0 && r < SIZE; r += direction) {
            if (board[r][f] == enemy_pawn) {
                return 0;
            }
        }
    }
    return 1;
}

// Function to evaluate pawn structure
int evaluate_pawn_structure(int is_white) {
    int score = 0;
    char pawn = is_white ? 'P' : 'p';
    
    for (int file = 0; file < SIZE; file++) {
        int pawns_in_file = count_pawns_in_file(file, is_white);
        
        // Penalty for doubled pawns
        if (pawns_in_file > 1) {
            score += (pawns_in_file - 1) * DOUBLED_PAWN_PENALTY;
        }
        
        // Check each pawn in the file
        for (int rank = 0; rank < SIZE; rank++) {
            if (board[rank][file] == pawn) {
                // Penalty for isolated pawns
                if (is_isolated_pawn(file, is_white)) {
                    score += ISOLATED_PAWN_PENALTY;
                }
                
                // Bonus for passed pawns
                if (is_passed_pawn(rank, file, is_white)) {
                    score += PASSED_PAWN_BONUS;
                    // Extra bonus for more advanced passed pawns
                    score += (is_white ? (7 - rank) : rank) * 5;
                }
            }
        }
    }
    
    return score;
}

// Function to count bishops
int count_bishops(int is_white) {
    int count = 0;
    char bishop = is_white ? 'B' : 'b';
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (board[i][j] == bishop) {
                count++;
            }
        }
    }
    return count;
}

// Function to check if a rook is on an open file
int is_rook_on_open_file(int file) {
    // Check if there are any pawns in the file
    for (int r = 0; r < SIZE; r++) {
        if (board[r][file] == 'P' || board[r][file] == 'p') {
            return 0;
        }
    }
    return 1;
}

// Function to check if a rook is on a semi-open file
int is_rook_on_semi_open_file(int file, int is_white) {
    char friendly_pawn = is_white ? 'P' : 'p';
    char enemy_pawn = is_white ? 'p' : 'P';
    
    int has_friendly_pawn = 0;
    int has_enemy_pawn = 0;
    
    for (int r = 0; r < SIZE; r++) {
        if (board[r][file] == friendly_pawn) has_friendly_pawn = 1;
        if (board[r][file] == enemy_pawn) has_enemy_pawn = 1;
    }
    
    return !has_friendly_pawn && has_enemy_pawn;
}

// Function to evaluate rook placement
int evaluate_rooks(int is_white) {
    int score = 0;
    char rook = is_white ? 'R' : 'r';
    int rook_files[SIZE] = {0};  // Track which files have rooks
    
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (board[i][j] == rook) {
                rook_files[j] = 1;
                
                // Bonus for rook on open file
                if (is_rook_on_open_file(j)) {
                    score += ROOK_ON_OPEN_FILE_BONUS;
                }
                // Bonus for rook on semi-open file
                else if (is_rook_on_semi_open_file(j, is_white)) {
                    score += ROOK_ON_SEMI_OPEN_FILE_BONUS;
                }
            }
        }
    }
    
    // Check for connected rooks
    for (int f = 0; f < SIZE-1; f++) {
        if (rook_files[f] && rook_files[f+1]) {
            score += CONNECTED_ROOKS_BONUS;
        }
    }
    
    return score;
}

// Function to evaluate center control
int evaluate_center_control(int is_white) {
    int score = 0;
    
    // Evaluate control of center squares
    for (int i = 0; i < 4; i++) {
        int rank = center_squares[i][0];
        int file = center_squares[i][1];
        char piece = board[rank][file];
        
        if (piece != ' ') {
            if ((is_white && isupper(piece)) || (!is_white && islower(piece))) {
                score += CENTER_CONTROL_BONUS;
            }
        }
    }
    
    // Evaluate control of extended center
    for (int i = 0; i < 16; i++) {
        int rank = extended_center_squares[i][0];
        int file = extended_center_squares[i][1];
        char piece = board[rank][file];
        
        if (piece != ' ') {
            if ((is_white && isupper(piece)) || (!is_white && islower(piece))) {
                score += CENTER_CONTROL_BONUS / 2;  // Half bonus for extended center
            }
        }
    }
    
    return score;
}

// Function to evaluate king safety
int evaluate_king_safety(int is_white) {
    int score = 0;
    char king = is_white ? 'K' : 'k';
    char pawn = is_white ? 'P' : 'p';
    
    // Find king position
    int king_rank = -1, king_file = -1;
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (board[i][j] == king) {
                king_rank = i;
                king_file = j;
                break;
            }
        }
        if (king_rank != -1) break;
    }
    
    // Check pawn shield
    if (king_rank != -1) {
        int shield_rank = is_white ? king_rank + 1 : king_rank - 1;
        if (shield_rank >= 0 && shield_rank < SIZE) {
            // Check three files in front of king
            for (int f = king_file-1; f <= king_file+1; f++) {
                if (f >= 0 && f < SIZE) {
                    if (board[shield_rank][f] == pawn) {
                        score += KING_SHIELD_BONUS;
                    }
                }
            }
        }
    }
    
    return score;
}

// Function to evaluate piece mobility
int evaluate_mobility(int is_white) {
    int score = 0;
    Move possible_moves[256];
    
    // Store current turn
    int original_turn = is_white_turn;
    is_white_turn = is_white;
    
    // Generate all possible moves
    int move_count = generate_moves(possible_moves);
    
    // Restore original turn
    is_white_turn = original_turn;
    
    // Add mobility bonus
    score += move_count * MOBILITY_BONUS;
    
    return score;
}

// Function for naive material-only evaluation
int evaluate_naive() {
    int score = 0;
    
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            switch (board[i][j]) {
                // White pieces (positive values)
                case 'P': score += NAIVE_PAWN_VALUE; break;
                case 'N': score += NAIVE_KNIGHT_VALUE; break;
                case 'B': score += NAIVE_BISHOP_VALUE; break;
                case 'R': score += NAIVE_ROOK_VALUE; break;
                case 'Q': score += NAIVE_QUEEN_VALUE; break;
                case 'K': score += NAIVE_KING_VALUE; break;
                
                // Black pieces (negative values)
                case 'p': score -= NAIVE_PAWN_VALUE; break;
                case 'n': score -= NAIVE_KNIGHT_VALUE; break;
                case 'b': score -= NAIVE_BISHOP_VALUE; break;
                case 'r': score -= NAIVE_ROOK_VALUE; break;
                case 'q': score -= NAIVE_QUEEN_VALUE; break;
                case 'k': score -= NAIVE_KING_VALUE; break;
            }
        }
    }
    
    return score;
}

// Function to evaluate the current board position
int evaluate_position() {
    // Check for checkmate
    if (is_checkmate()) {
        return is_white_turn ? CHECKMATE_SCORE : -CHECKMATE_SCORE;
    }
    
    // Use naive evaluation
    return evaluate_naive();
}

// Function to make a move on the board
void make_move(Move move) {
    board[move.to_x][move.to_y] = board[move.from_x][move.from_y];
    board[move.from_x][move.from_y] = ' ';
}

// Function to undo a move
void undo_move(Move move, char captured_piece) {
    board[move.from_x][move.from_y] = board[move.to_x][move.to_y];
    board[move.to_x][move.to_y] = captured_piece;
}

// Function to check if a move is legal (including check considerations)
int is_legal_move(Move move) {
    char captured = board[move.to_x][move.to_y];
    int legal = 1;
    
    // Make the move
    make_move(move);
    
    // If the move leaves our king in check, it's not legal
    if (is_king_in_check(is_white_turn ? 'K' : 'k')) {
        legal = 0;
    }
    
    // Undo the move
    undo_move(move, captured);
    
    return legal;
}

// Function to generate all possible moves for the current position
int generate_moves(Move* moves) {
    int move_count = 0;
    
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            char piece = board[i][j];
            
            // Skip empty squares and opponent's pieces
            if (piece == ' ' || (is_white_turn && islower(piece)) || (!is_white_turn && isupper(piece))) {
                continue;
            }
            
            // Try all possible destinations
            for (int dx = 0; dx < SIZE; dx++) {
                for (int dy = 0; dy < SIZE; dy++) {
                    if (is_valid_move(i, j, dx, dy)) {
                        Move move = {i, j, dx, dy, 0};
                        // Only add legal moves that don't leave king in check
                        if (is_legal_move(move)) {
                            moves[move_count] = move;
                            move_count++;
                        }
                    }
                }
            }
        }
    }
    
    return move_count;
}

// Minimax function with alpha-beta pruning
int minimax(int depth, int alpha, int beta, int maximizing_player) {
    // Check for checkmate at current position
    if (is_checkmate()) {
        return is_white_turn ? CHECKMATE_SCORE : -CHECKMATE_SCORE;
    }
    
    if (depth == 0) {
        return evaluate_position();
    }
    
    Move possible_moves[256];
    int move_count = generate_moves(possible_moves);
    
    // If no legal moves are available but not checkmate, it's a stalemate
    if (move_count == 0) {
        return 0;
    }
    
    if (maximizing_player) {
        int max_eval = -INF;
        for (int i = 0; i < move_count; i++) {
            char captured = board[possible_moves[i].to_x][possible_moves[i].to_y];
            make_move(possible_moves[i]);
            is_white_turn = !is_white_turn;
            
            int eval = minimax(depth - 1, alpha, beta, 0);
            
            is_white_turn = !is_white_turn;
            undo_move(possible_moves[i], captured);
            
            max_eval = (eval > max_eval) ? eval : max_eval;
            alpha = (eval > alpha) ? eval : alpha;
            if (beta <= alpha)
                break;
        }
        return max_eval;
    } else {
        int min_eval = INF;
        for (int i = 0; i < move_count; i++) {
            char captured = board[possible_moves[i].to_x][possible_moves[i].to_y];
            make_move(possible_moves[i]);
            is_white_turn = !is_white_turn;
            
            int eval = minimax(depth - 1, alpha, beta, 1);
            
            is_white_turn = !is_white_turn;
            undo_move(possible_moves[i], captured);
            
            min_eval = (eval < min_eval) ? eval : min_eval;
            beta = (eval < beta) ? eval : beta;
            if (beta <= alpha)
                break;
        }
        return min_eval;
    }
}

// Function to find the best move using minimax
Move find_best_move() {
    Move possible_moves[256];
    int move_count = generate_moves(possible_moves);
    Move best_move = {-1, -1, -1, -1, -INF};
    
    // If in check, increase search depth to find escape moves
    int current_depth = is_king_in_check(is_white_turn ? 'K' : 'k') ? MAX_DEPTH + 1 : MAX_DEPTH;
    
    for (int i = 0; i < move_count; i++) {
        char captured = board[possible_moves[i].to_x][possible_moves[i].to_y];
        make_move(possible_moves[i]);
        is_white_turn = !is_white_turn;
        
        int score = minimax(current_depth - 1, -INF, INF, 0);
        possible_moves[i].score = score;
        
        is_white_turn = !is_white_turn;
        undo_move(possible_moves[i], captured);
        
        if (score > best_move.score) {
            best_move = possible_moves[i];
            best_move.score = score;
        }
    }
    
    return best_move;
}

// Function to convert a move to chess notation
void move_to_notation(Move move, char* notation) {
    notation[0] = 'a' + move.from_y;
    notation[1] = '8' - move.from_x;
    notation[2] = 'a' + move.to_y;
    notation[3] = '8' - move.to_x;
    notation[4] = '\0';
}

// Function to print the chess board
void print_chess_board() {
    printf("\n   a b c d e f g h\n");
    printf("  -----------------\n");
    for (int i = 0; i < SIZE; i++) {
        printf("%d |", 8 - i);
        for (int j = 0; j < SIZE; j++) {
            printf("%c ", board[i][j] == ' ' ? '.' : board[i][j]);
        }
        printf("| %d\n", 8 - i);
    }
    printf("  -----------------\n");
    printf("   a b c d e f g h\n\n");
}

// Function to parse chess notation (e.g., "e2e4") into a Move
Move parse_move(const char* notation) {
    Move move;
    move.from_y = notation[0] - 'a';
    move.from_x = '8' - notation[1];
    move.to_y = notation[2] - 'a';
    move.to_x = '8' - notation[3];
    move.score = 0;
    return move;
}

// Function to check if a move is valid
int is_move_valid(Move move) {
    // Check if coordinates are within bounds
    if (move.from_x < 0 || move.from_x >= SIZE || move.from_y < 0 || move.from_y >= SIZE ||
        move.to_x < 0 || move.to_x >= SIZE || move.to_y < 0 || move.to_y >= SIZE) {
        return 0;
    }
    
    // Check if the move is valid according to chess rules
    return is_valid_move(move.from_x, move.from_y, move.to_x, move.to_y);
}

// Function to get a valid move from human player
Move get_human_move() {
    char notation[5];
    Move move;
    int valid = 0;
    
    while (!valid) {
        printf("Enter your move (e.g., e2e4): ");
        if (scanf("%4s", notation) != 1) {
            // Clear input buffer
            while (getchar() != '\n');
            printf("Invalid input format. Please use format like 'e2e4'.\n");
            continue;
        }
        
        move = parse_move(notation);
        if (is_move_valid(move)) {
            valid = 1;
        } else {
            printf("Invalid move. Please try again.\n");
        }
    }
    
    return move;
}

// Function to play AI vs AI game
void play_ai_vs_ai(int num_moves) {
    int moves = 0;
    char notation[5];
    
    printf("\nStarting AI vs AI game (max %d moves)...\n", num_moves);
    print_chess_board();
    
    while (moves < num_moves) {
        // Get AI move
        Move ai_move = find_best_move();
        if (ai_move.from_x == -1) {
            printf("Game over - no valid moves available.\n");
            break;
        }
        
        // Make the move
        char captured = board[ai_move.to_x][ai_move.to_y];
        make_move(ai_move);
        
        // Convert move to notation and display
        move_to_notation(ai_move, notation);
        printf("%s moves %s", is_white_turn ? "Black" : "White", notation);
        if (captured != ' ') {
            printf(" (captures %c)", captured);
        }
        printf("\n");
        
        print_chess_board();
        
        // Check for checkmate or stalemate
        Move possible_moves[256];
        int move_count = generate_moves(possible_moves);
        if (move_count == 0) {
            if (is_king_in_check(is_white_turn ? 'K' : 'k')) {
                printf("Checkmate! %s wins!\n", is_white_turn ? "Black" : "White");
            } else {
                printf("Stalemate! Game is a draw.\n");
            }
            break;
        }
        
        is_white_turn = !is_white_turn;
        moves++;
        
        // Add a small delay to make the game watchable
        sleep(1);
    }
    
    if (moves >= num_moves) {
        printf("Game ended after %d moves.\n", moves);
    }
}

// Function to play Human vs AI game
void play_human_vs_ai(int human_is_white) {
    char notation[5];
    is_white_turn = 1;  // White always starts
    
    printf("\nStarting Human vs AI game...\n");
    printf("You are playing %s\n", human_is_white ? "White" : "Black");
    print_chess_board();
    
    while (1) {
        Move move;
        
        if (is_white_turn == human_is_white) {
            // Human's turn
            move = get_human_move();
        } else {
            // AI's turn
            printf("AI is thinking...\n");
            move = find_best_move();
            if (move.from_x == -1) {
                printf("Game over - AI has no valid moves.\n");
                break;
            }
            move_to_notation(move, notation);
            printf("AI moves: %s\n", notation);
        }
        
        // Make the move
        make_move(move);
        print_chess_board();
        
        // Check for checkmate or stalemate
        Move possible_moves[256];
        int move_count = generate_moves(possible_moves);
        if (move_count == 0) {
            if (is_king_in_check(is_white_turn ? 'K' : 'k')) {
                printf("Checkmate! %s wins!\n", is_white_turn ? "Black" : "White");
            } else {
                printf("Stalemate! Game is a draw.\n");
            }
            break;
        }
        
        is_white_turn = !is_white_turn;
    }
}

// Function to run AI tests
void test_ai() {
    printf("\nChess AI Testing Menu:\n");
    printf("1. Play AI vs AI\n");
    printf("2. Play as White vs AI\n");
    printf("3. Play as Black vs AI\n");
    printf("Enter choice (1-3): ");
    
    int choice;
    if (scanf("%d", &choice) != 1) {
        printf("Invalid input. Exiting.\n");
        return;
    }
    
    switch (choice) {
        case 1:
            play_ai_vs_ai(50);  // Play up to 50 moves
            break;
        case 2:
            play_human_vs_ai(1);  // Human plays White
            break;
        case 3:
            play_human_vs_ai(0);  // Human plays Black
            break;
        default:
            printf("Invalid choice. Exiting.\n");
    }
} 