#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <SDL2/SDL_image.h>
#include <errno.h>
#include <unistd.h>
#include "chess_ai.h"
#include "chess_logic.h"

#define SIZE 8
#define TILE_SIZE 80
#define WINDOW_SIZE (SIZE * TILE_SIZE)
#define WINDOW_HEIGHT (WINDOW_SIZE + 300) // Add extra space for status
#define BUTTON_WIDTH 200
#define BUTTON_HEIGHT 50
#define BUTTON_PADDING 20
#define NUM_MENU_BUTTONS 7

// Game modes
typedef enum {
    MODE_MENU,
    MODE_AI_VS_AI,
    MODE_HUMAN_VS_AI_WHITE,
    MODE_HUMAN_VS_AI_BLACK,
    MODE_PLAYING,
    MODE_PLAYBACK  // Add specific mode for playback
} GameMode;

// Button structure
typedef struct {
    SDL_Rect rect;
    const char* text;
    int active;
} Button;

// Global variables
extern int promotion_pending;
extern int promotion_x, promotion_y;
extern int promotion_is_white;
extern char board[SIZE][SIZE];
extern char last_error[128];
extern int is_white_turn;
extern char *unicode_pieces[128];
extern char captured_pieces[32];

// Global variables for AI and game state
SDL_Texture *piece_textures[128];
int is_check = 0;
int ai_enabled = 1;
int ai_thinking = 0;
GameMode current_mode = MODE_MENU;
int playback_mode = 0;
int current_playback_move = 0;
PGNMove playback_moves[MAX_MOVES];
int total_playback_moves = 0;

// Button declarations
Button buttons[NUM_MENU_BUTTONS] = {
    {.text = "AI vs AI", .active = 1},
    {.text = "Play as White vs AI", .active = 1},
    {.text = "Play as Black vs AI", .active = 1},
    {.text = "Save Game", .active = 0},
    {.text = "Load Game", .active = 1},
    {.text = "Back to Menu", .active = 0},
    {.text = "Next Move", .active = 0}  // For playback mode
};

// Forward declarations
void render_text(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, SDL_Color color);
//void make_ai_move();
void render_chessboard(SDL_Renderer *renderer);
void init_unicode_pieces();
void move_piece(char *move);
void initialize_buttons(int window_width, int window_height);
void render_menu(SDL_Renderer *renderer, TTF_Font *font);
int handle_menu_click(int x, int y);
void start_pgn_playback(const char *filename);
void play_next_move(void);

// Initialize button positions
void initialize_buttons(int window_width, int window_height) {
    // Menu buttons layout
    int menu_start_y = (window_height - (4 * (BUTTON_HEIGHT + BUTTON_PADDING))) / 2;
    
    // Initialize all buttons as inactive first
    for (int i = 0; i < NUM_MENU_BUTTONS; i++) {
        buttons[i].active = 0;
    }
    
    // Set up menu buttons (first 3 buttons + Load Game)
    for (int i = 0; i < 3; i++) {
        buttons[i].rect.x = (window_width - BUTTON_WIDTH) / 2;
        buttons[i].rect.y = menu_start_y + i * (BUTTON_HEIGHT + BUTTON_PADDING);
        buttons[i].rect.w = BUTTON_WIDTH;
        buttons[i].rect.h = BUTTON_HEIGHT;
        buttons[i].active = 1;
    }
    
    // Set up Load Game button
    buttons[4].rect.x = (window_width - BUTTON_WIDTH) / 2;
    buttons[4].rect.y = menu_start_y + 3 * (BUTTON_HEIGHT + BUTTON_PADDING);
    buttons[4].rect.w = BUTTON_WIDTH;
    buttons[4].rect.h = BUTTON_HEIGHT;
    buttons[4].active = 1;
    
    // Game buttons layout (initially inactive)
    int bottom_section_y = WINDOW_SIZE + 100;
    
    // Save Game button
    buttons[3].rect.x = 50;
    buttons[3].rect.y = bottom_section_y;
    buttons[3].rect.w = BUTTON_WIDTH;
    buttons[3].rect.h = BUTTON_HEIGHT;
    
    // Back to Menu button
    buttons[5].rect.x = (window_width - BUTTON_WIDTH) / 2;
    buttons[5].rect.y = bottom_section_y;
    buttons[5].rect.w = BUTTON_WIDTH;
    buttons[5].rect.h = BUTTON_HEIGHT;
    
    // Next Move button (for playback)
    buttons[6].rect.x = window_width - BUTTON_WIDTH - 50;
    buttons[6].rect.y = bottom_section_y;
    buttons[6].rect.w = BUTTON_WIDTH;
    buttons[6].rect.h = BUTTON_HEIGHT;
}

// Render a button
void render_button(SDL_Renderer *renderer, TTF_Font *font, Button *button) {
    if (!button->active) return;
    
    // Draw button background
    SDL_SetRenderDrawColor(renderer, 181, 136, 99, 255);
    SDL_RenderFillRect(renderer, &button->rect);
    
    // Draw button border
    SDL_SetRenderDrawColor(renderer, 240, 217, 181, 255);
    SDL_RenderDrawRect(renderer, &button->rect);
    
    // Draw button text
    SDL_Color text_color = {255, 255, 255, 255};
    SDL_Surface *surface = TTF_RenderText_Blended(font, button->text, text_color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    
    SDL_Rect text_rect = {
        button->rect.x + (button->rect.w - surface->w) / 2,
        button->rect.y + (button->rect.h - surface->h) / 2,
        surface->w,
        surface->h
    };
    
    SDL_RenderCopy(renderer, texture, NULL, &text_rect);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

// Render the menu
void render_menu(SDL_Renderer *renderer, TTF_Font *font) {
    // Clear the screen
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
    SDL_RenderClear(renderer);
    
    // Draw title
    SDL_Color title_color = {255, 255, 255, 255};
    render_text(renderer, font, "Chess Game", 
                (WINDOW_SIZE - 200) / 2, 
                50, title_color);
    
    // Calculate total height of all buttons
    int total_height = (BUTTON_HEIGHT + BUTTON_PADDING) * 4;  // Only show 4 buttons in menu
    int start_y = (WINDOW_SIZE - total_height) / 2;
    
    // Draw active buttons
    int button_count = 0;
    for (size_t i = 0; i < NUM_MENU_BUTTONS; i++) {
        if (buttons[i].active) {
            buttons[i].rect.x = (WINDOW_SIZE - BUTTON_WIDTH) / 2;
            buttons[i].rect.y = start_y + button_count * (BUTTON_HEIGHT + BUTTON_PADDING);
            buttons[i].rect.w = BUTTON_WIDTH;
            buttons[i].rect.h = BUTTON_HEIGHT;
            render_button(renderer, font, &buttons[i]);
            button_count++;
        }
    }
}

// Handle menu clicks
int handle_menu_click(int x, int y) {
        SDL_Point click = {x, y};

    // Check each button
    for (int i = 0; i < NUM_MENU_BUTTONS; i++) {
        if (!buttons[i].active) continue;
        
        
         if (SDL_PointInRect(&click, &buttons[i].rect)) {
            printf("Button clicked: %d\n", i);
        
            switch (i) {
                case 0: // AI vs AI
                    current_mode = MODE_AI_VS_AI;
                    ai_enabled = 1;
                    initialize_board();
                    buttons[0].active = buttons[1].active = buttons[2].active = buttons[4].active = 0;
                    buttons[3].active = buttons[5].active = 1;
                    return 1;
                    
                case 1: // Play as White vs AI
                    current_mode = MODE_HUMAN_VS_AI_WHITE;
                    ai_enabled = 1;
                    initialize_board();
                    buttons[0].active = buttons[1].active = buttons[2].active = buttons[4].active = 0;
                    buttons[3].active = buttons[5].active = 1;
                    return 1;
                    
                case 2: // Play as Black vs AI
                    current_mode = MODE_HUMAN_VS_AI_BLACK;
                    ai_enabled = 1;
                    initialize_board();
                    // Make first move for AI (White)
                    make_ai_move();
                    buttons[0].active = buttons[1].active = buttons[2].active = buttons[4].active = 0;
                    buttons[3].active = buttons[5].active = 1;
                    return 1;
                    
                case 3: // Save Game
                    save_game("chess_game.pgn");
                    return 1;
                    
                case 4: // Load Game
                    {
                        
                        start_pgn_playback("chess_game.pgn");
                        return 1;
                    }
                    
                case 5: // Back to Menu
                    current_mode = MODE_MENU;
                    playback_mode = 0;
                    ai_enabled = 0;
                    ai_thinking = 0;
                    initialize_board();
                    buttons[0].active = buttons[1].active = buttons[2].active = buttons[4].active = 1;
                    buttons[3].active = buttons[5].active = buttons[6].active = 0;
                    return 1;
                    
                case 6: // Next Move
                    if (playback_mode) {
                        play_next_move();
                        
                    }return 1;
                   
            }
        }
    }
    return 0;
}

// filepath: /Users/mariapetrulescu/ChessGame/chess_gui.c
void load_piece_textures(SDL_Renderer *renderer) {
    const char *piece_files[128] = {0};
    piece_files['P'] = "assets/white_pawn.png";
    piece_files['R'] = "assets/white_rook.png";
    piece_files['N'] = "assets/white_knight.png";
    piece_files['B'] = "assets/white_bishop.png";
    piece_files['Q'] = "assets/white_queen.png";
    piece_files['K'] = "assets/white_king.png";
    piece_files['p'] = "assets/black_pawn.png";
    piece_files['r'] = "assets/black_rook.png";
    piece_files['n'] = "assets/black_knight.png";
    piece_files['b'] = "assets/black_bishop.png";
    piece_files['q'] = "assets/black_queen.png";
    piece_files['k'] = "assets/black_king.png";

    for (int i = 0; i < 128; i++) {
        if (piece_files[i]) {
            SDL_Surface *surface = IMG_Load(piece_files[i]);
            if (!surface) {
                printf("Failed to load image %s: %s\n", piece_files[i], IMG_GetError());
                continue;
            }
            piece_textures[i] = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_FreeSurface(surface);
        }
    }
}

void free_piece_textures() {
    for (int i = 0; i < 128; i++) {
        if (piece_textures[i]) {
            SDL_DestroyTexture(piece_textures[i]);
            piece_textures[i] = NULL;
        }
    }
}

// filepath: /Users/mariapetrulescu/ChessGame/chess_gui.c
void render_text(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, SDL_Color color) {
   // printf("Rendering text: %s at (%d, %d)\n", text, x, y);

    SDL_Surface *surface = TTF_RenderText_Blended(font, text, color);
    if (!surface) {
        printf("TTF_RenderText_Blended Error: %s\n", TTF_GetError());
        return;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        printf("SDL_CreateTextureFromSurface Error: %s\n", SDL_GetError());
        SDL_FreeSurface(surface);
        return;
    }

    SDL_Rect dst = {x, y, surface->w, surface->h};
    //printf("Text dimensions: width=%d, height=%d\n", surface->w, surface->h);

    SDL_RenderCopy(renderer, texture, NULL, &dst);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

void render_chessboard(SDL_Renderer *renderer) {
    for (int y = 0; y < SIZE; y++) {
        for (int x = 0; x < SIZE; x++) {
            // Draw the tile
            SDL_Rect tile = {
                .x = x * TILE_SIZE,
                .y = y * TILE_SIZE,
                .w = TILE_SIZE,
                .h = TILE_SIZE
            };
            if ((x + y) % 2 == 0) {
                SDL_SetRenderDrawColor(renderer, 240, 217, 181, 255); // Light tile
            } else {
                SDL_SetRenderDrawColor(renderer, 181, 136, 99, 255); // Dark tile
            }
            SDL_RenderFillRect(renderer, &tile);

            // Draw the piece
            char piece = board[y][x];
            if (piece != ' ' && piece_textures[(int)piece]) {
                SDL_Rect dst = {
                    .x = x * TILE_SIZE,
                    .y = y * TILE_SIZE,
                    .w = TILE_SIZE,
                    .h = TILE_SIZE
                };
                SDL_RenderCopy(renderer, piece_textures[(int)piece], NULL, &dst);
            }
        }
    }
}

void handle_mouse_drag(SDL_Event *event, char *move) {
    static int selected_x = -1, selected_y = -1;
    static int dragging = 0;

    // Don't allow moves if game is over
    if (game_over || playback_mode) {
        return;
    }

    // Don't allow moves while AI is thinking
    if (ai_thinking) {
        return;
    }

    // Handle mouse events
    if (event->type == SDL_MOUSEBUTTONDOWN) {
        int tile_x = event->button.x / TILE_SIZE;
        int tile_y = event->button.y / TILE_SIZE;

        // Only handle clicks within the board
        if (tile_x >= 0 && tile_x < SIZE && tile_y >= 0 && tile_y < SIZE) {
            char piece = board[tile_y][tile_x];
            
            // Check if it's a valid piece to move
            if ((is_white_turn && isupper(piece)) || (!is_white_turn && islower(piece))) {
                // Check if it's the correct player's turn in AI mode
                if (ai_enabled) {
                    if ((current_mode == MODE_HUMAN_VS_AI_WHITE && !is_white_turn) ||
                        (current_mode == MODE_HUMAN_VS_AI_BLACK && is_white_turn) ||
                        current_mode == MODE_AI_VS_AI) {
                        return;
                    }
                }
                
                selected_x = tile_x;
                selected_y = tile_y;
                dragging = 1;
                printf("Selected piece %c at %c%d\n", piece, 'a' + selected_x, 8 - selected_y);
            }
        }
    } else if (event->type == SDL_MOUSEBUTTONUP && dragging) {
        int tile_x = event->button.x / TILE_SIZE;
        int tile_y = event->button.y / TILE_SIZE;

        // Only handle drops within the board
        if (tile_x >= 0 && tile_x < SIZE && tile_y >= 0 && tile_y < SIZE) {
            if (selected_x != tile_x || selected_y != tile_y) {  // Only if the destination is different
                snprintf(move, 5, "%c%d%c%d", 
                    'a' + selected_x,
                    8 - selected_y,
                    'a' + tile_x,
                    8 - tile_y
                );
                printf("Move: %s\n", move);
            }
        }
        selected_x = -1;
        selected_y = -1;
        dragging = 0;
    }
}


void handle_promotion(SDL_Event *event) {
    if (!promotion_pending) return;
    
    if (event->type == SDL_KEYDOWN) {
        char promoted_piece = 0;
        
        switch (event->key.keysym.sym) {
            case SDLK_q:
                promoted_piece = promotion_is_white ? 'Q' : 'q';
                break;
            case SDLK_r:
                promoted_piece = promotion_is_white ? 'R' : 'r';
                break;
            case SDLK_b:
                promoted_piece = promotion_is_white ? 'B' : 'b';
                break;
            case SDLK_n:
                promoted_piece = promotion_is_white ? 'N' : 'n';
                break;
            default:
                return; // Not a valid promotion key
        }
        
        // Apply the promotion
        board[promotion_x][promotion_y] = promoted_piece;
        promotion_pending = 0;
        
        // Update game state
        is_white_turn = !is_white_turn;
        last_error[0] = '\0';
        
        printf("Promoted pawn to %c\n", promoted_piece);
    }
}

void handle_ai_promotion() {
    if (promotion_pending) {
        // AI always promotes to queen
        char promoted_piece = promotion_is_white ? 'Q' : 'q';
        board[promotion_x][promotion_y] = promoted_piece;
        promotion_pending = 0;
        
        // Update game state
        is_white_turn = !is_white_turn;
        last_error[0] = '\0';
    }
}

void check_game_over() {
    char king = is_white_turn ? 'K' : 'k';
    printf("Debug: Checking game over. Current turn: %s\n", is_white_turn ? "White" : "Black");
    
    if (is_king_in_check(king)) {
        printf("Debug: King is in check\n");
        if (is_checkmate_or_stalemate(king)) {
            printf("Debug: Checkmate detected!\n");
            game_over = 1;
            snprintf(last_error, sizeof(last_error), "Checkmate! %s wins!", is_white_turn ? "Black" : "White");
            
            // Deactivate AI moves
            ai_enabled = 0;
        } else {
            printf("Debug: King is in check but has legal moves\n");
        }
    } else {
        printf("Debug: King is not in check\n");
    }
}




void render_game(SDL_Renderer *renderer, TTF_Font *font) {
    // Clear the screen
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
    SDL_RenderClear(renderer);
    
    // Render the chessboard
    render_chessboard(renderer);
    
    // Render game status
    if (last_error[0] != '\0') {
        SDL_Color error_color = {255, 0, 0, 255};
        render_text(renderer, font, last_error, 10, WINDOW_SIZE + 50, error_color);
    }
    
    // Render all active buttons
    for (int i = 0; i < NUM_MENU_BUTTONS; i++) {
        if (buttons[i].active) {
            render_button(renderer, font, &buttons[i]);
        }
    }
}

// Add this function to handle PGN playback
void start_pgn_playback(const char *filename) {
    printf("Debug: Starting PGN playback from file: %s\n", filename);
    
    // Reset the game state
    initialize_board();
    playback_mode = 1;
    current_playback_move = 0;
    total_playback_moves = 0;
    
    // Set appropriate game mode and button states
    current_mode = MODE_PLAYBACK;  // Use specific playback mode
    ai_enabled = 0;  // Disable AI during playback
    
    // Initialize buttons
    for (int i = 0; i < NUM_MENU_BUTTONS; i++) {
        buttons[i].active = 0;  // Deactivate all buttons first
    }
    buttons[5].active = 1;  // Activate Back to Menu
    buttons[6].active = 1;  // Activate Next Move
    
    // Try to get absolute path
    char abs_path[1024];
    const char *path_to_try = filename;
    
    if (getcwd(abs_path, sizeof(abs_path)) != NULL) {
        strncat(abs_path, "/", sizeof(abs_path) - strlen(abs_path) - 1);
        strncat(abs_path, filename, sizeof(abs_path) - strlen(abs_path) - 1);
        path_to_try = abs_path;
    }
    
    printf("Debug: Attempting to open PGN file at: %s\n", path_to_try);
    
    // Load the PGN file
    FILE *file = fopen(path_to_try, "r");
    if (!file) {
        // Try home directory if file not found
        const char *home = getenv("HOME");
        if (home) {
            snprintf(abs_path, sizeof(abs_path), "%s/%s", home, filename);
            printf("Debug: File not found, trying home directory: %s\n", abs_path);
            file = fopen(abs_path, "r");
        }
        
        if (!file) {
            printf("Debug: Failed to open PGN file: %s\n", strerror(errno));
            snprintf(last_error, sizeof(last_error), "Could not open file for playback: %s", strerror(errno));
            return;
        }
    }
    
    printf("Debug: Successfully opened PGN file\n");
    printf("Debug: Initial board state:\n");
    print_board();
    
    // Skip header tags and collect moves
    char line[MAX_PGN_LINE];
    int in_header = 1;
    int line_number = 0;
    
    while (fgets(line, sizeof(line), file)) {
        line_number++;
        printf("Debug: Reading line %d: %s", line_number, line);
        
        // Skip empty lines
        if (line[0] == '\n' || line[0] == '\r') {
            continue;
        }
        
        // Process headers
        if (in_header) {
            if (line[0] != '[') {
                in_header = 0;
            } else {
                continue;
            }
        }
        
        // Process moves
        char *token = strtok(line, " \t\n");
        while (token) {
            printf("Debug: Processing token: %s\n", token);
            
            // Skip move numbers, dots, and result markers
            if (isdigit(token[0]) || token[0] == '.' || strcmp(token, "*") == 0) {
                token = strtok(NULL, " \t\n");
                continue;
            }
            
            // Skip comments
            if (token[0] == '{') {
                while (token && !strstr(token, "}")) {
                    token = strtok(NULL, " \t\n");
                }
                token = strtok(NULL, " \t\n");
                continue;
            }
            
            // Clean up the move token
            char clean_move[10] = {0};
            int j = 0;
            for (int i = 0; token[i] && j < 9; i++) {
                char c = token[i];
                if (isalnum(c) || c == '-' || c == '=' || c == 'O' || c == 'x') {
                    clean_move[j++] = c;
                }
            }
            clean_move[j] = '\0';
            
            if (clean_move[0]) {
                printf("Debug: Adding move %d: %s (original: %s)\n", 
                       total_playback_moves + 1, clean_move, token);
                strncpy(playback_moves[total_playback_moves].move, clean_move, 
                       sizeof(playback_moves[0].move) - 1);
                playback_moves[total_playback_moves].move[sizeof(playback_moves[0].move) - 1] = '\0';
                total_playback_moves++;
            }
            
            token = strtok(NULL, " \t\n");
        }
    }
    
    fclose(file);
    
    printf("Debug: Loaded %d moves for playback. Moves:\n", total_playback_moves);
    for (int i = 0; i < total_playback_moves; i++) {
        printf("Move %d: %s\n", i + 1, playback_moves[i].move);
    }
    
    if (total_playback_moves == 0) {
        printf("Debug: No moves were loaded from the PGN file\n");
        snprintf(last_error, sizeof(last_error), "No moves found in PGN file");
        playback_mode = 0;
        buttons[6].active = 0;  // Deactivate Next Move button
        return;
    }
    
    // Update status
    snprintf(last_error, sizeof(last_error), "Loaded %d moves for playback. Press Next Move to start.", 
             total_playback_moves);
}

// Add this function to handle the next move in playback
void play_next_move() {
    printf("Debug: play_next_move called. Current move: %d, Total moves: %d\n", 
           current_playback_move, total_playback_moves);
    
    if (!playback_mode || current_playback_move >= total_playback_moves) {
        if (current_playback_move >= total_playback_moves) {
            snprintf(last_error, sizeof(last_error), "Playback complete!");
            playback_mode = 0;
            buttons[6].active = 0;  // Deactivate Next Move button
        }
        return;
    }
    
    // Print the current board state
    printf("Debug: Current board state before move:\n");
    print_board();
    
    printf("Debug: Attempting to play move: %s\n", playback_moves[current_playback_move].move);
    
    // Clean up the move string - remove any annotations
    char clean_move[10] = {0};
    int j = 0;
    for (int i = 0; playback_moves[current_playback_move].move[i] && j < 9; i++) {
        char c = playback_moves[current_playback_move].move[i];
        if (isalnum(c) || c == '-' || c == '=' || c == 'O') {
            clean_move[j++] = c;
        }
    }
    clean_move[j] = '\0';
    
    printf("Debug: Cleaned move: %s\n", clean_move);
    printf("Debug: Current turn: %s\n", is_white_turn ? "White" : "Black");
    
    char coord_move[5];
    if (convert_pgn_to_coordinate(clean_move, coord_move)) {
        printf("Debug: Successfully converted to coordinate move: %s\n", coord_move);
        move_piece(coord_move);
        current_playback_move++;
        snprintf(last_error, sizeof(last_error), "Move %d/%d: %s", 
                current_playback_move, total_playback_moves, 
                clean_move);
    } else {
        printf("Debug: Failed to convert move %s (original: %s)\n", 
               clean_move, playback_moves[current_playback_move].move);
        snprintf(last_error, sizeof(last_error), "Error: Could not convert move %s", 
                clean_move);
    }
    
    // Print the board state after the move attempt
    printf("Debug: Board state after move attempt:\n");
    print_board();
}

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL initialization failed: %s\n", SDL_GetError());
        return 1;
    }
    printf("SDL initialized successfully\n");
    
    if (TTF_Init() < 0) {
        printf("TTF initialization failed: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }
    printf("TTF initialized successfully\n");
    
    int img_flags = IMG_INIT_PNG;
    if (!(IMG_Init(img_flags) & img_flags)) {
        printf("SDL_image initialization failed: %s\n", IMG_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    printf("SDL_image initialized successfully\n");

    SDL_Window *window = SDL_CreateWindow("Chess Game",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        WINDOW_SIZE, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    printf("Window created successfully\n");

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    printf("Renderer created successfully\n");

    printf("Attempting to open font file: /System/Library/Fonts/Helvetica.ttc\n");
    TTF_Font *font = TTF_OpenFont("/System/Library/Fonts/Helvetica.ttc", 18);
    if (!font) {
        printf("TTF_OpenFont Error: %s\n", TTF_GetError());
        printf("Current working directory: ");
        system("pwd");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    printf("Font loaded successfully\n");

    initialize_board();
    init_unicode_pieces();
    load_piece_textures(renderer);
    initialize_buttons(WINDOW_SIZE, WINDOW_HEIGHT);

    SDL_Event event;
    int quit = 0;
    char move[5] = {0};
    Uint32 last_ai_move_time = 0;
    const Uint32 AI_MOVE_DELAY = 500;
    
    while (!quit) {
        Uint32 current_time = SDL_GetTicks();

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    quit = 1;
                    break;
                    
                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEBUTTONUP: {
                    // For mouse down events, check if it's on a button
                    if (event.type == SDL_MOUSEBUTTONDOWN) {
                        int button_clicked = 0;
                        
                        // Check if the click was on any active button
                        for (int i = 0; i < NUM_MENU_BUTTONS; i++) {
                            if (buttons[i].active) {
                                SDL_Point click = {event.button.x, event.button.y};
                                if (SDL_PointInRect(&click, &buttons[i].rect)) {
                                    button_clicked = 1;
                                    handle_menu_click(event.button.x, event.button.y);
                                    break;
                                }
                            }
                        }
                        
                        // If it's not a button click, handle it as a potential piece movement
                        if (!button_clicked && current_mode != MODE_MENU) {
                            handle_mouse_drag(&event, move);
                        }
                    } else if (event.type == SDL_KEYDOWN) {
                        // Handle keyboard input for promotion
                        if (promotion_pending) {
                            handle_promotion(&event);
                        }
                    } else {
                        // For mouse up events, always try to handle as piece movement
                        if (current_mode != MODE_MENU) {
                            handle_mouse_drag(&event, move);
                        }
                    }
                    
                    // Process any move that was generated by the mouse events
                    if (move[0] != '\0') {
                        printf("Attempting move: %s\n", move);
                        move_piece(move);
                        move[0] = '\0';
                        
                        check_game_over();
                        
                        // Make AI move if it's enabled and it's AI's turn
                        if (!game_over && ai_enabled) {
                            if (current_mode == MODE_HUMAN_VS_AI_WHITE) {
                                if (!is_white_turn) {  // AI plays black
                                    printf("AI (Black) making move\n");
                                    make_ai_move();
                                }
                            } else if (current_mode == MODE_HUMAN_VS_AI_BLACK) {
                                if (is_white_turn) {  // AI plays white
                                    printf("AI (White) making move\n");
                                    make_ai_move();
                                } 
                            }
                        }
                    }
                    break;
                }
            }
        }

        // Handle AI vs AI mode
        if (current_mode == MODE_AI_VS_AI && ai_enabled && !ai_thinking && !game_over &&
            (current_time - last_ai_move_time >= AI_MOVE_DELAY)) {
            printf("AI vs AI - making move for %s\n", is_white_turn ? "White" : "Black");
            ai_thinking = 1;
            
            // Make the AI move for the current player
            Move ai_move = find_best_move();
            if (ai_move.from_x != -1) {  // If a valid move was found
                make_move(ai_move);
                // Switch turns
                is_white_turn = !is_white_turn;
                last_ai_move_time = current_time;
                
                // Check for game over after each move
                check_game_over();
            } else {
                printf("No valid moves for %s!\n", is_white_turn ? "White" : "Black");
            }
            
            ai_thinking = 0;
        }

        // Render the appropriate screen
        if (current_mode == MODE_MENU) {
            render_menu(renderer, font);
        } else {
            render_game(renderer, font);
        }
        
        SDL_RenderPresent(renderer);
    }

    free_piece_textures();
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();

    return 0;
}