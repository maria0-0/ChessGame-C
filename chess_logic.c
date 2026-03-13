#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include "chess_logic.h"

char captured_pieces[32] = ""; // Store captured pieces
int promotion_pending = 0;
int promotion_x = -1, promotion_y = -1;
int promotion_is_white = 0;
int is_white_turn = 1; // 1 for white's turn, 0 for black's turn
int game_over = 0; // Define game over state
char *unicode_pieces[128];
char last_error[128]= "";
// Piece representation
char board[SIZE][SIZE] = {
    {'r', 'n', 'b', 'q', 'k', 'b', 'n', 'r'}, // Black pieces
    {'p', 'p', 'p', 'p', 'p', 'p', 'p', 'p'}, // Black pawns
    {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '}, // Empty row
    {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '}, // Empty row
    {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '}, // Empty row
    {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '}, // Empty row
    {'P', 'P', 'P', 'P', 'P', 'P', 'P', 'P'}, // White pawns
    {'R', 'N', 'B', 'Q', 'K', 'B', 'N', 'R'}  // White pieces
};

// Castling rights
int white_can_castle_kingside = 1;
int white_can_castle_queenside = 1;
int black_can_castle_kingside = 1;
int black_can_castle_queenside = 1;


// AI mode variables
//int ai_mode = 0;  // Start in human vs human mode
//int ai_plays_black = 1;  // AI defaults to playing black

// En passant tracking
int en_passant_target_x = -1;
int en_passant_target_y = -1;

// PGN storage
PGNMove move_history[MAX_MOVES];
int move_count = 0;

void init_unicode_pieces() {
    unicode_pieces['P'] = "♙"; unicode_pieces['p'] = "♟";
    unicode_pieces['R'] = "♖"; unicode_pieces['r'] = "♜";
    unicode_pieces['N'] = "♘"; unicode_pieces['n'] = "♞";
    unicode_pieces['B'] = "♗"; unicode_pieces['b'] = "♝";
    unicode_pieces['Q'] = "♕"; unicode_pieces['q'] = "♛";
    unicode_pieces['K'] = "♔"; unicode_pieces['k'] = "♚";
    unicode_pieces[' '] = "·";
}

// Function to initialize the game
void initialize_board() {
    // Reset the board to starting position
    char starting_position[SIZE][SIZE] = {
        {'r', 'n', 'b', 'q', 'k', 'b', 'n', 'r'}, // Black pieces
        {'p', 'p', 'p', 'p', 'p', 'p', 'p', 'p'}, // Black pawns
        {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '}, // Empty row
        {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '}, // Empty row
        {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '}, // Empty row
        {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '}, // Empty row
        {'P', 'P', 'P', 'P', 'P', 'P', 'P', 'P'}, // White pawns
        {'R', 'N', 'B', 'Q', 'K', 'B', 'N', 'R'}  // White pieces
    };
    
    // Copy starting position to board
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            board[i][j] = starting_position[i][j];
        }
    }
    
    // Initialize other game state
    is_white_turn = 1;
    captured_pieces[0] = '\0';
    promotion_pending = 0;
    promotion_x = -1;
    promotion_y = -1;
    last_error[0] = '\0';
    
    // Initialize Unicode pieces
    init_unicode_pieces();
    
    // Reset castling rights
    white_can_castle_kingside = 1;
    white_can_castle_queenside = 1;
    black_can_castle_kingside = 1;
    black_can_castle_queenside = 1;
    
    // Reset en passant
    en_passant_target_x = -1;
    en_passant_target_y = -1;

    // Reset move history
    move_count = 0;
    memset(move_history, 0, sizeof(move_history));
}

void print_board() {
    printf("\n    a  b  c  d  e  f  g  h\n");
    printf("  -------------------------\n");
    for (int i = 0; i < SIZE; i++) {
        printf("%d |", SIZE - i); // Print row numbers (8 to 1)
        for (int j = 0; j < SIZE; j++) {
            printf(" %s ", unicode_pieces[(int)board[i][j]]);
        }
        printf("| %d\n", SIZE - i); // Print row numbers again
    }
    printf("  -------------------------\n");
    printf("    a  b  c  d  e  f  g  h\n\n");
}

// Helper function to check if a square is under attack
int is_square_attacked(int x, int y, int by_white) {
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            char piece = board[i][j];
            if (piece == ' ') continue;
            if (by_white == isupper(piece)) {
                if (is_valid_move(i, j, x, y)) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

int is_valid_move(int sx, int sy, int dx, int dy) {
    // Check if coordinates are within bounds
    if (sx < 0 || sx >= SIZE || sy < 0 || sy >= SIZE ||
        dx < 0 || dx >= SIZE || dy < 0 || dy >= SIZE) {
        return 0;
    }

    char piece = board[sx][sy];
    if (piece == ' ') {
        return 0;  // No piece to move
    }

    // Check if moving to the same square
    if (sx == dx && sy == dy) {
        return 0;
    }

    // Check if capturing own piece
    if ((isupper(piece) && isupper(board[dx][dy])) ||
        (islower(piece) && islower(board[dx][dy]))) {
        return 0;
    }

    int dx_abs = abs(dx - sx);
    int dy_abs = abs(dy - sy);

    // Handle pawn moves
    if (toupper(piece) == 'P') {
        int forward = (piece == 'P') ? -1 : 1;  // White moves up (-1), Black moves down (+1)
        
        // Normal move forward
        if (sy == dy && board[dx][dy] == ' ') {
            if (dx == sx + forward) {
                return 1;  // One square forward
            }
            // First move can be two squares
            if ((piece == 'P' && sx == 6 && dx == 4) ||
                (piece == 'p' && sx == 1 && dx == 3)) {
                if (board[sx + forward][sy] == ' ') {
                    return 1;
                }
            }
        }
        // Capture diagonally
        else if (dx == sx + forward && dy_abs == 1) {
            if (board[dx][dy] != ' ' || (dx == en_passant_target_x && dy == en_passant_target_y)) {
                return 1;
            }
        }
        return 0;
    }

    // Handle knight moves
    if (toupper(piece) == 'N') {
        return (dx_abs == 2 && dy_abs == 1) || (dx_abs == 1 && dy_abs == 2);
    }

    // Handle bishop moves
    if (toupper(piece) == 'B') {
        if (dx_abs != dy_abs) {
            return 0;
        }
    }

    // Handle rook moves
    if (toupper(piece) == 'R') {
        if (dx != sx && dy != sy) {
            return 0;
        }
    }

    // Handle queen moves (combination of bishop and rook)
    if (toupper(piece) == 'Q') {
        if (dx_abs != dy_abs && dx != sx && dy != sy) {
            return 0;
        }
    }

    // Handle king moves
    if (toupper(piece) == 'K') {
        // Normal king move
        if (dx_abs <= 1 && dy_abs <= 1) {
            return 1;
        }
        // Castling
        if (dx == sx && dy_abs == 2) {
            // Check if castling is allowed
            if (piece == 'K') {
                if (!white_can_castle_kingside && dy > sy) return 0;
                if (!white_can_castle_queenside && dy < sy) return 0;
            } else {
                if (!black_can_castle_kingside && dy > sy) return 0;
                if (!black_can_castle_queenside && dy < sy) return 0;
            }
            
            // Check if path is clear
            int rook_y = dy > sy ? 7 : 0;
            int step = dy > sy ? 1 : -1;
            
            // Check if rook is in place
            if (board[sx][rook_y] != (piece == 'K' ? 'R' : 'r')) {
                return 0;
            }
            
            // Check if path is clear
            for (int y = sy + step; dy > sy ? y < rook_y : y > rook_y; y += step) {
                if (board[sx][y] != ' ') {
                    return 0;
                }
            }
            
            // Check if king passes through check
            for (int y = sy; dy > sy ? y <= dy : y >= dy; y += step) {
                char temp = board[sx][y];
                board[sx][y] = piece;
                board[sx][sy] = ' ';
                if (is_king_in_check(piece)) {
                    board[sx][sy] = piece;
                    board[sx][y] = temp;
                    return 0;
                }
                board[sx][sy] = piece;
                board[sx][y] = temp;
            }
            return 1;
        }
    }

    // Check if path is clear for sliding pieces (bishop, rook, queen)
    int step_x = (dx == sx) ? 0 : (dx - sx) / dx_abs;
    int step_y = (dy == sy) ? 0 : (dy - sy) / dy_abs;
    
    int x = sx + step_x;
    int y = sy + step_y;
    
    while (x != dx || y != dy) {
        if (board[x][y] != ' ') {
            return 0;  // Path is blocked
        }
        x += step_x;
        y += step_y;
    }

    return 1;
}

int is_king_in_check(char king) {
    int king_x = -1, king_y = -1;

    // Find the king's position
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (board[i][j] == king) {
                king_x = i;
                king_y = j;
                break;
            }
        }
    }

    // Check if the king is attacked by any opponent piece
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            char piece = board[i][j];

            // Skip empty squares and friendly pieces
            if (piece == ' ' || (islower(king) && islower(piece)) || (isupper(king) && isupper(piece))) {
                continue;
            }

            // Only validate moves for pieces that could attack the king
            if (is_valid_move(i, j, king_x, king_y)) {
                return 1; // King is in check
            }
        }
    }

    return 0; // King is safe
}

// Helper function to get piece name
char get_piece_char(char piece) {
    switch(toupper(piece)) {
        case 'P': return ' ';  // Pawns don't have a letter in PGN
        case 'N': return 'N';
        case 'B': return 'B';
        case 'R': return 'R';
        case 'Q': return 'Q';
        case 'K': return 'K';
        default: return ' ';
    }
}

// Convert coordinate notation to PGN notation
void convert_to_pgn_notation(char *pgn_move, const char *coord_move) {
    printf("Debug: Converting coordinate move %s to PGN\n", coord_move);
    
    int sx = SIZE - (coord_move[1] - '0');
    int sy = coord_move[0] - 'a';
    int dx = SIZE - (coord_move[3] - '0');
    int dy = coord_move[2] - 'a';
    
    char piece = board[sx][sy];
    int pos = 0;
    
    // Handle castling
    if ((piece == 'K' || piece == 'k') && abs(dy - sy) == 2) {
        strcpy(pgn_move, dy > sy ? "O-O" : "O-O-O");
        return;
    }
    
    // Always add piece letter, using 'P' for pawns
    switch(toupper(piece)) {
        case 'P': pgn_move[pos++] = 'P'; break;
        case 'N': pgn_move[pos++] = 'N'; break;
        case 'B': pgn_move[pos++] = 'B'; break;
        case 'R': pgn_move[pos++] = 'R'; break;
        case 'Q': pgn_move[pos++] = 'Q'; break;
        case 'K': pgn_move[pos++] = 'K'; break;
    }
    
    // Add source square for disambiguation
    pgn_move[pos++] = coord_move[0];  // Source file
    pgn_move[pos++] = coord_move[1];  // Source rank
    

    
    // Determine if this is a capture
    int is_capture = 0;
    char target_piece = board[dx][dy];
    
    // Regular capture - there's an opponent's piece at the destination
    if (target_piece != ' ' && 
        ((isupper(piece) && islower(target_piece)) || 
         (islower(piece) && isupper(target_piece)))) {
        is_capture = 1;
    }
    
    // En passant capture
    if (toupper(piece) == 'P' && sy != dy && 
        dx == en_passant_target_x && dy == en_passant_target_y) {
        is_capture = 1;
    }
    if(is_capture){
        pgn_move[pos++] = 'x';
    }   


    
    // Add destination square
    pgn_move[pos++] = coord_move[2];  // Destination file
    pgn_move[pos++] = coord_move[3];  // Destination rank
    
    // Add pawn promotion
    if ((piece == 'P' && dx == 0) || (piece == 'p' && dx == 7)) {
        pgn_move[pos++] = '=';
        pgn_move[pos++] = 'Q';  // Default to queen promotion
    }
    
    // Add check or checkmate symbol
    char temp = board[dx][dy];
    board[dx][dy] = piece;
    board[sx][sy] = ' ';
    
    if (is_king_in_check(is_white_turn ? 'k' : 'K')) {
        // Check if it's checkmate
        int is_mate = 1;
        for (int i = 0; i < SIZE && is_mate; i++) {
            for (int j = 0; j < SIZE && is_mate; j++) {
                char test_piece = board[i][j];
                if ((is_white_turn && islower(test_piece)) || (!is_white_turn && isupper(test_piece))) {
                    for (int di = 0; di < SIZE && is_mate; di++) {
                        for (int dj = 0; dj < SIZE && is_mate; dj++) {
                            if (is_valid_move(i, j, di, dj)) {
                                char temp2 = board[di][dj];
                                board[di][dj] = test_piece;
                                board[i][j] = ' ';
                                if (!is_king_in_check(is_white_turn ? 'k' : 'K')) {
                                    is_mate = 0;
                                }
                                board[i][j] = test_piece;
                                board[di][dj] = temp2;
                            }
                        }
                    }
                }
            }
        }
        pgn_move[pos++] = is_mate ? '#' : '+';
    }
    
    // Restore the board
    board[sx][sy] = piece;
    board[dx][dy] = temp;
    
    pgn_move[pos] = '\0';
    printf("Debug: Converted to PGN notation: %s\n", pgn_move);
}

// Helper function to check if a move needs disambiguation
int needs_disambiguation(int sx, int sy, int dx, int dy) {
    char piece = board[sx][sy];
    if (piece == ' ') return 0;
    
    // Check if any other piece of the same type can move to the same square
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if ((i != sx || j != sy) && board[i][j] == piece) {
                if (is_valid_move(i, j, dx, dy)) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

void add_to_pgn(const char *move, const char *full_move) {
    printf("Debug: add_to_pgn called with move: %s, full_move: %s\n", move, full_move);
    
    if (move_count >= MAX_MOVES) {
        printf("Debug: Maximum moves reached (%d)\n", MAX_MOVES);
        return;
    }
    
    // Get source and destination coordinates
    int sx = SIZE - (full_move[1] - '0');
    int sy = full_move[0] - 'a';
    int dx = SIZE - (full_move[3] - '0');
    int dy = full_move[2] - 'a';
    
    printf("Debug: Move coordinates: from (%d,%d) to (%d,%d)\n", sx, sy, dx, dy);
    
    // Validate coordinates
    if (sx < 0 || sx >= SIZE || sy < 0 || sy >= SIZE ||
        dx < 0 || dx >= SIZE || dy < 0 || dy >= SIZE) {
        printf("Debug: Invalid coordinates detected\n");
        return;
    }
    
    // Store the move
    PGNMove *pgn_move = &move_history[move_count];
    strncpy(pgn_move->move, move, sizeof(pgn_move->move) - 1);
    pgn_move->move[sizeof(pgn_move->move) - 1] = '\0';
    pgn_move->number = (move_count / 2) + 1;
    
    printf("Debug: Added move %d to history: %s\n", move_count + 1, pgn_move->move);
    move_count++;
}

void save_game(const char *filename) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        snprintf(last_error, sizeof(last_error), "Could not open file for saving: %s", strerror(errno));
        return;
    }
    
    // Write standard PGN headers
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    
    fprintf(file, "[Event \"Chess Game\"]\n");
    fprintf(file, "[Site \"Local Game\"]\n");
    fprintf(file, "[Date \"%04d.%02d.%02d\"]\n", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
    fprintf(file, "[Round \"1\"]\n");
    fprintf(file, "[White \"Player 1\"]\n");
    fprintf(file, "[Black \"Player 2\"]\n");
    fprintf(file, "[Result \"%s\"]\n", game_over ? (is_white_turn ? "0-1" : "1-0") : "*");
    
    // Add piece notation tags
    fprintf(file, "[PieceN \"Knight\"]\n");
    fprintf(file, "[PieceB \"Bishop\"]\n");
    fprintf(file, "[PieceR \"Rook\"]\n");
    fprintf(file, "[PieceQ \"Queen\"]\n");
    fprintf(file, "[PieceK \"King\"]\n");
    fprintf(file, "[PieceP \"Pawn\"]\n");
    fprintf(file, "\n");
    
    // Write moves
    int move_number = 1;
    for (int i = 0; i < move_count; i++) {
        // Write move number at start of white's move
        if (i % 2 == 0) {
            fprintf(file, "%d.", move_number++);
        }
        
        // Write the move with proper spacing
        fprintf(file, " %s", move_history[i].move);
        
        // Add newline after black's move or if it's the last move
        if (i % 2 == 1 || i == move_count - 1) {
            fprintf(file, "\n");
        }
    }
    
    // Write game result
    fprintf(file, "\n%s\n", game_over ? (is_white_turn ? "0-1" : "1-0") : "*");
    
    fclose(file);
    snprintf(last_error, sizeof(last_error), "Game saved successfully to %s", filename);
}

// Convert PGN move to coordinate notation
int convert_pgn_to_coordinate(const char *pgn_move, char *coord_move) {
    // Skip any leading spaces
    while (*pgn_move == ' ') {
        pgn_move++;
    }
    
    // Handle castling first
    if (strcmp(pgn_move, "O-O") == 0) {
        strcpy(coord_move, is_white_turn ? "e1g1" : "e8g8");
        return 1;
    }
    if (strcmp(pgn_move, "O-O-O") == 0) {
        strcpy(coord_move, is_white_turn ? "e1c1" : "e8c8");
        return 1;
    }

    size_t len = strlen(pgn_move);
    
    if (len < 2) {
        return 0;
    }

    // Handle direct coordinate notation (e.g., "e2e4", "f2f4")
    if (len == 4 && 
        pgn_move[0] >= 'a' && pgn_move[0] <= 'h' &&
        pgn_move[1] >= '1' && pgn_move[1] <= '8' &&
        pgn_move[2] >= 'a' && pgn_move[2] <= 'h' &&
        pgn_move[3] >= '1' && pgn_move[3] <= '8') {
        
        int src_x = '8' - pgn_move[1];
        int src_y = pgn_move[0] - 'a';
        int dst_x = '8' - pgn_move[3];
        int dst_y = pgn_move[2] - 'a';
        
        // Verify the move is valid
        if (is_valid_move(src_x, src_y, dst_x, dst_y)) {
            // Make sure this move doesn't leave the king in check
            char piece = board[src_x][src_y];
            char temp = board[dst_x][dst_y];
            board[dst_x][dst_y] = piece;
            board[src_x][src_y] = ' ';
            int king_in_check = is_king_in_check(is_white_turn ? 'K' : 'k');
            board[src_x][src_y] = piece;
            board[dst_x][dst_y] = temp;
            
            if (!king_in_check) {
                strcpy(coord_move, pgn_move);
                return 1;
            }
        }
        return 0;
    }

    // Handle new PGN format with piece type (e.g., "Pe2e4", "Nb1c3")
    if (len == 5 && isupper(pgn_move[0])) {
        //char piece_type = pgn_move[0];
        char src_file = pgn_move[1];
        char src_rank = pgn_move[2];
        char dst_file = pgn_move[3];
        char dst_rank = pgn_move[4];
        
        if (src_file >= 'a' && src_file <= 'h' &&
            src_rank >= '1' && src_rank <= '8' &&
            dst_file >= 'a' && dst_file <= 'h' &&
            dst_rank >= '1' && dst_rank <= '8') {
            
            sprintf(coord_move, "%c%c%c%c", src_file, src_rank, dst_file, dst_rank);
            return 1;
        }
        return 0;
    }

    // Get destination square for old PGN format
    char dst_file = pgn_move[len - 2];
    char dst_rank = pgn_move[len - 1];
    
    if (dst_file < 'a' || dst_file > 'h' || dst_rank < '1' || dst_rank > '8') {
        return 0;
    }

    int dst_x = '8' - dst_rank;
    int dst_y = dst_file - 'a';

    // If the move starts with a lowercase letter (a-h) and is 2 characters long,
    // it could be either a pawn move or a piece move to that square
    if (len == 2 && pgn_move[0] >= 'a' && pgn_move[0] <= 'h') {
        int src_y = pgn_move[0] - 'a';  // Source file is same as destination file
        
        // First try to find any piece that can move to this square
        const char pieces[] = {'N', 'B', 'R', 'Q', 'K'};  // Try pieces in order of likelihood
        for (int i = 0; i < 5; i++) {
            char piece = is_white_turn ? pieces[i] : tolower(pieces[i]);
            for (int rank = 0; rank < SIZE; rank++) {
                for (int file = 0; file < SIZE; file++) {
                    if (board[rank][file] == piece && is_valid_move(rank, file, dst_x, dst_y)) {
                        // Make sure this move doesn't leave the king in check
                        char temp = board[dst_x][dst_y];
                        board[dst_x][dst_y] = piece;
                        board[rank][file] = ' ';
                        int king_in_check = is_king_in_check(is_white_turn ? 'K' : 'k');
                        board[rank][file] = piece;
                        board[dst_x][dst_y] = temp;
                        
                        if (!king_in_check) {
                            sprintf(coord_move, "%c%c%c%c", 
                                    (char)('a' + file), '8' - rank,
                                    dst_file, dst_rank);
                            return 1;
                        }
                    }
                }
            }
        }
        
        // If no piece move found, try pawn moves
        int start_rank = is_white_turn ? 6 : 1;  // Starting rank for pawns
        int direction = is_white_turn ? -1 : 1;  // Direction of pawn movement
        
        // Try one-square move first
        int src_x = dst_x - direction;
        if (src_x >= 0 && src_x < SIZE && 
            board[src_x][src_y] == (is_white_turn ? 'P' : 'p') &&
            board[dst_x][dst_y] == ' ' &&
            is_valid_move(src_x, src_y, dst_x, dst_y)) {
            sprintf(coord_move, "%c%c%c%c", 
                    (char)('a' + src_y), '8' - src_x,
                    dst_file, dst_rank);
            return 1;
        }
        
        // Try two-square move if from starting position
        if ((is_white_turn && dst_x == 4) || (!is_white_turn && dst_x == 3)) {
            if (board[start_rank][src_y] == (is_white_turn ? 'P' : 'p') &&
                board[dst_x][dst_y] == ' ' &&
                board[dst_x - direction][dst_y] == ' ' &&
                is_valid_move(start_rank, src_y, dst_x, dst_y)) {
                sprintf(coord_move, "%c%c%c%c", 
                        (char)('a' + src_y), '8' - start_rank,
                        dst_file, dst_rank);
                return 1;
            }
        }
        
        return 0;
    }

    // Handle piece moves and captures
    char piece = 'P';  // Default to pawn
    size_t pos = 0;
    
    // Parse piece if present
    if (isupper(pgn_move[0]) && pgn_move[0] != 'O') {
        piece = pgn_move[0];
        pos++;
    }
    
    // Skip any 'x' in the notation
    while (pos < len && pgn_move[pos] == 'x') {
        pos++;
    }

    // For pawns with captures or explicit source file
    if (piece == 'P' && pos < len - 2) {
        char src_file = pgn_move[pos];
        if (src_file >= 'a' && src_file <= 'h') {
            int src_y = src_file - 'a';
            int src_x = dst_x + (is_white_turn ? 1 : -1);  // One square behind in direction of movement
            
            if (src_x >= 0 && src_x < SIZE && 
                board[src_x][src_y] == (is_white_turn ? 'P' : 'p') &&
                is_valid_move(src_x, src_y, dst_x, dst_y)) {
                sprintf(coord_move, "%c%c%c%c", 
                        src_file, '8' - src_x,
                        dst_file, dst_rank);
                return 1;
            }
        }
        return 0;
    }

    // For other pieces
    for (int rank = 0; rank < SIZE; rank++) {
        for (int file = 0; file < SIZE; file++) {
            if (board[rank][file] == (is_white_turn ? piece : tolower(piece))) {
                if (is_valid_move(rank, file, dst_x, dst_y)) {
                    // Check if this move would leave the king in check
                    char temp = board[dst_x][dst_y];
                    board[dst_x][dst_y] = board[rank][file];
                    board[rank][file] = ' ';
                    int king_in_check = is_king_in_check(is_white_turn ? 'K' : 'k');
                    board[rank][file] = board[dst_x][dst_y];
                    board[dst_x][dst_y] = temp;
                    
                    if (!king_in_check) {
                        sprintf(coord_move, "%c%c%c%c", 
                                (char)('a' + file), '8' - rank,
                                dst_file, dst_rank);
                        return 1;
                    }
                }
            }
        }
    }

    return 0;
}


void load_pgn(const char *filename) {
    printf("Debug: Loading PGN from file: %s\n", filename);
    
    // Reset the game state
    initialize_board();
    move_count = 0;
    is_white_turn = 1;
    
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Debug: Failed to open file: %s\n", strerror(errno));
        snprintf(last_error, sizeof(last_error), "Could not open file: %s", strerror(errno));
        return;
    }
    
    printf("Debug: File opened successfully\n");
    
    char line[MAX_PGN_LINE];
    int in_header = 1;
    int moves_processed = 0;
    
    while (fgets(line, sizeof(line), file)) {
        // Remove trailing whitespace and newlines
        char *end = line + strlen(line) - 1;
        while (end > line && isspace(*end)) {
            *end-- = '\0';
        }
        
        printf("Debug: Processing line: '%s'\n", line);
        
        // Skip empty lines
        if (strlen(line) == 0) {
            continue;
        }
        
        // Check if we're still in the header section
        if (in_header) {
            if (line[0] != '[') {
                in_header = 0;
            } else {
                continue;  // Skip header lines
            }
        }
        
        if (!in_header) {
            char *token = strtok(line, " ");
            while (token) {
                printf("Debug: Processing token: '%s'\n", token);
                
                // Skip move numbers (e.g., "1.", "2.", etc.)
                if (isdigit(token[0])) {
                    char *dot = strchr(token, '.');
                    if (dot) {
                        token = strtok(NULL, " ");
                        continue;
                    }
                }
                
                // Skip game result markers
                if (strcmp(token, "1-0") == 0 || strcmp(token, "0-1") == 0 || 
                    strcmp(token, "1/2-1/2") == 0 || strcmp(token, "*") == 0) {
                    token = strtok(NULL, " ");
                    continue;
                }
                
                // Process the move
                printf("Debug: Converting move: '%s'\n", token);
                char coord_move[5];
                if (convert_pgn_to_coordinate(token, coord_move)) {
                    printf("Debug: Successfully converted to coordinate move: %s\n", coord_move);
                    
                    // Make the move
                    move_piece(coord_move);
                    
                    if (last_error[0] == '\0') {
                        moves_processed++;
                        printf("Debug: Move %d applied successfully\n", moves_processed);
                    } else {
                        printf("Debug: Error applying move: %s\n", last_error);
                        fclose(file);
                        return;
                    }
                } else {
                    printf("Debug: Failed to convert move: %s\n", token);
                    snprintf(last_error, sizeof(last_error), "Could not convert move %s", token);
                    fclose(file);
                    return;
                }
                
                token = strtok(NULL, " ");
            }
        }
    }
    
    fclose(file);
    
    if (moves_processed == 0) {
        snprintf(last_error, sizeof(last_error), "No valid moves were processed from the file");
    } else {
        snprintf(last_error, sizeof(last_error), "Game loaded successfully (%d moves processed)", moves_processed);
    }
    
    printf("Debug: Finished loading PGN, processed %d moves\n", moves_processed);
}

void load_game(const char *filename) {
    load_pgn(filename);
}


void move_piece(char *move) {
    // Convert chess notation to board coordinates
    int sx = SIZE - (move[1] - '0');     // Source row
    int sy = move[0] - 'a';              // Source column
    int dx = SIZE - (move[3] - '0');     // Destination row
    int dy = move[2] - 'a';              // Destination column

    // Validate coordinates are within bounds
    if (sx < 0 || sx >= SIZE || sy < 0 || sy >= SIZE ||
        dx < 0 || dx >= SIZE || dy < 0 || dy >= SIZE) {
        snprintf(last_error, sizeof(last_error), "Invalid coordinates.");
        return;
    }

    char moving_piece = board[sx][sy];
    
    // Check if the source square is empty
    if (moving_piece == ' ') {
        snprintf(last_error, sizeof(last_error), "No piece at source square.");
        return;
    }

    // Validate turn
    if ((is_white_turn && !isupper(moving_piece)) || (!is_white_turn && !islower(moving_piece))) {
        snprintf(last_error, sizeof(last_error), "Invalid move! It's %s's turn.", is_white_turn ? "White" : "Black");
        return;
    }

    // Validate piece movement
    if (!is_valid_move(sx, sy, dx, dy)) {
        snprintf(last_error, sizeof(last_error), "Invalid move! Does not follow chess rules.");
        return;
    }

    // For pawns, ensure they only move forward
    if (toupper(moving_piece) == 'P') {
        int forward = is_white_turn ? -1 : 1;  // White pawns move up (-1), black pawns move down (+1)
        if ((dx - sx) * forward <= 0) {  // Check if moving in wrong direction
            snprintf(last_error, sizeof(last_error), "Pawns can only move forward.");
            return;
        }
    }

    // Store the original board state in case we need to undo the move
    char original_dest_piece = board[dx][dy];
    char original_src_piece = board[sx][sy];
    int original_is_white_turn = is_white_turn;

    // Handle castling
    if ((moving_piece == 'K' || moving_piece == 'k') && abs(dy - sy) == 2) {
        int rook_y = dy > sy ? 7 : 0;
        int new_rook_y = dy > sy ? dy - 1 : dy + 1;
        
        // Move the rook
        board[dx][new_rook_y] = board[sx][rook_y];
        board[sx][rook_y] = ' ';
    }

    // Handle en passant capture
    if ((moving_piece == 'P' || moving_piece == 'p') && 
        dx == en_passant_target_x && dy == en_passant_target_y) {
        board[sx][dy] = ' ';  // Remove the captured pawn
    }

    // Make the move
    board[dx][dy] = moving_piece;
    board[sx][sy] = ' ';

    // Check if the move leaves the king in check
    if (is_king_in_check(is_white_turn ? 'K' : 'k')) {
        // Undo the move if it leaves the king in check
        board[sx][sy] = original_src_piece;
        board[dx][dy] = original_dest_piece;
        is_white_turn = original_is_white_turn;
        snprintf(last_error, sizeof(last_error), "Move leaves the king in check! Move undone.");
        return;
    }

    // Update castling rights
    if (moving_piece == 'K') {
        white_can_castle_kingside = white_can_castle_queenside = 0;
    } else if (moving_piece == 'k') {
        black_can_castle_kingside = black_can_castle_queenside = 0;
    } else if (moving_piece == 'R') {
        if (sx == 7 && sy == 0) white_can_castle_queenside = 0;
        if (sx == 7 && sy == 7) white_can_castle_kingside = 0;
    } else if (moving_piece == 'r') {
        if (sx == 0 && sy == 0) black_can_castle_queenside = 0;
        if (sx == 0 && sy == 7) black_can_castle_kingside = 0;
    }

    // Set en passant target for two-square pawn moves
    en_passant_target_x = -1;
    en_passant_target_y = -1;
    if ((moving_piece == 'P' && sx == 6 && dx == 4) ||
        (moving_piece == 'p' && sx == 1 && dx == 3)) {
        en_passant_target_x = (sx + dx) / 2;
        en_passant_target_y = sy;
    }

    // Handle pawn promotion
    if ((moving_piece == 'P' && dx == 0) || (moving_piece == 'p' && dx == 7)) {
        promotion_pending = 1;
        promotion_x = dx;
        promotion_y = dy;
        promotion_is_white = is_white_turn;
        snprintf(last_error, sizeof(last_error), "Pawn promoted! Press Q/R/B/N.");
        return;
    }

    // Convert move to PGN notation and add to history
    char pgn_move[10];
    convert_to_pgn_notation(pgn_move, move);
    add_to_pgn(pgn_move, move);

    // Update game state
    last_error[0] = '\0';
    is_white_turn = !is_white_turn;
}
int is_checkmate_or_stalemate(char king) {
    // First check if the king is in check
    if (!is_king_in_check(king)) {
        return 0;  // Not checkmate if not in check
    }

    // Try every possible move for all pieces of the current player
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            char piece = board[i][j];
            
            // Skip empty squares and opponent's pieces
            if (piece == ' ' || (isupper(king) && islower(piece)) || (islower(king) && isupper(piece))) {
                continue;
            }
            
            // Try moving this piece to every possible square
            for (int dx = 0; dx < SIZE; dx++) {
                for (int dy = 0; dy < SIZE; dy++) {
                    if (is_valid_move(i, j, dx, dy)) {
                        // Make the move temporarily
                        char temp_dest = board[dx][dy];
                        board[dx][dy] = piece;
                        board[i][j] = ' ';
                        
                        // Check if this move gets us out of check
                        int still_in_check = is_king_in_check(king);
                        
                        // Undo the move
                        board[i][j] = piece;
                        board[dx][dy] = temp_dest;
                        
                        // If we found a legal move that gets us out of check, it's not checkmate
                        if (!still_in_check) {
                            return 0;
                        }
                    }
                }
            }
        }
    }
    
    // If we get here, no legal moves were found to get out of check
    return 1;  // Checkmate!
}
// Helper function to evaluate board position (simple version)
static int evaluate_position(void) {
    int score = 0;
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            switch (board[i][j]) {
                case 'P': score += 10; break;
                case 'p': score -= 10; break;
                case 'N': case 'B': score += 30; break;
                case 'n': case 'b': score -= 30; break;
                case 'R': score += 50; break;
                case 'r': score -= 50; break;
                case 'Q': score += 90; break;
                case 'q': score -= 90; break;
                case 'K': score += 900; break;
                case 'k': score -= 900; break;
            }
        }
    }
    return score;
}

// Function to generate all valid moves for the current position
static int generate_valid_moves(char moves[][5], int max_moves) {
    int move_count = 0;
    
    for (int sx = 0; sx < SIZE && move_count < max_moves; sx++) {
        for (int sy = 0; sy < SIZE && move_count < max_moves; sy++) {
            char piece = board[sx][sy];
            // Skip empty squares and opponent's pieces
            if (piece == ' ' || (is_white_turn && islower(piece)) || (!is_white_turn && isupper(piece))) {
                continue;
            }
            
            // Try all possible destinations
            for (int dx = 0; dx < SIZE && move_count < max_moves; dx++) {
                for (int dy = 0; dy < SIZE && move_count < max_moves; dy++) {
                    if (is_valid_move(sx, sy, dx, dy)) {
                        // Test if move leaves king in check
                        char temp = board[dx][dy];
                        board[dx][dy] = board[sx][sy];
                        board[sx][sy] = ' ';
                        int king_in_check = is_king_in_check(is_white_turn ? 'K' : 'k');
                        board[sx][sy] = board[dx][dy];
                        board[dx][dy] = temp;
                        
                        if (!king_in_check) {
                            sprintf(moves[move_count], "%c%c%c%c",
                                    (char)('a' + sy), '8' - sx,
                                    (char)('a' + dy), '8' - dx);
                            move_count++;
                        }
                    }
                }
            }
        }
    }
    return move_count;
}

int make_ai_move(void) {
    /*if (!ai_mode || (ai_plays_black && is_white_turn) || (!ai_plays_black && !is_white_turn)) {
        return 0;  // Not AI's turn
    }*/
    
    char valid_moves[256][5];  // Buffer for storing valid moves
    int move_count = generate_valid_moves(valid_moves, 256);
    
    if (move_count == 0) {
        return 0;  // No valid moves
    }
    
    // Simple AI: Choose a random move from valid moves
    int best_move_idx = 0;
    int best_score = -99999;
    
    for (int i = 0; i < move_count; i++) {
        // Make the move temporarily
        int sx = '8' - valid_moves[i][1];
        int sy = valid_moves[i][0] - 'a';
        int dx = '8' - valid_moves[i][3];
        int dy = valid_moves[i][2] - 'a';
        
        char temp = board[dx][dy];
        board[dx][dy] = board[sx][sy];
        board[sx][sy] = ' ';
        
        // Evaluate the position
        int score = evaluate_position();
        /*if (!ai_plays_black) {
            score = -score;  // Invert score if AI plays white
        }
        */
        // Add some randomness to avoid repetitive play
        score += rand() % 10;
        
        // Keep track of best move
        if (score > best_score) {
            best_score = score;
            best_move_idx = i;
        }
        
        // Undo the move
        board[sx][sy] = board[dx][dy];
        board[dx][dy] = temp;
    }
    
    // Make the best move
    move_piece(valid_moves[best_move_idx]);
    return 1;
}