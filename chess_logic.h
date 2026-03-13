#ifndef CHESS_LOGIC_H
#define CHESS_LOGIC_H

#define SIZE 8
#define MAX_MOVES 1000
#define MAX_PGN_LINE 1024

// Game state variables
extern char board[SIZE][SIZE];
extern int is_white_turn;
extern char last_error[128];
extern int promotion_pending;
extern int promotion_x, promotion_y;
extern int promotion_is_white;
extern int game_over;

// Castling rights
extern int white_can_castle_kingside;
extern int white_can_castle_queenside;
extern int black_can_castle_kingside;
extern int black_can_castle_queenside;

// En passant tracking
extern int en_passant_target_x;
extern int en_passant_target_y;

// PGN move storage
typedef struct {
    char move[10];     // Store moves like "e4", "Nf3", "O-O", etc.
    char full_move[20]; // Store full coordinate notation like "e2e4"
    int number;        // Move number
    char piece;        // The piece that moved
    int capture;       // Whether it was a capture
    int check;         // Whether it gave check
    int checkmate;     // Whether it was checkmate
} PGNMove;

extern PGNMove move_history[MAX_MOVES];
extern int move_count;

// Function declarations
void initialize_board(void);
int is_valid_move(int sx, int sy, int dx, int dy);
void move_piece(char *move);
int is_king_in_check(char king);
int is_checkmate_or_stalemate(char king);
void print_board(void);

// PGN-related functions
void add_to_pgn(const char *move, const char *full_move);
void save_pgn(const char *filename);
void load_pgn(const char *filename);
const char *get_pgn_move_string(int move_index);
void convert_to_pgn_notation(char *pgn_move, const char *coord_move);
int convert_pgn_to_coordinate(const char *pgn_move, char *coord_move);
int needs_disambiguation(int sx, int sy, int dx, int dy);
void save_game(const char *filename);
void load_game(const char *filename);
void init_unicode_pieces();

#endif // CHESS_LOGIC_H 