#ifndef CHESS_GUI_H
#define CHESS_GUI_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800
#define BOARD_SIZE 600
#define SQUARE_SIZE (BOARD_SIZE / 8)
#define BUTTON_HEIGHT 50
#define BUTTON_WIDTH 200
#define BUTTON_PADDING 20
#define MAX_BUTTONS 10

typedef struct {
    SDL_Rect rect;
    const char *text;
    int active;
} Button;

extern Button buttons[MAX_BUTTONS];

// Global variables
extern int ai_enabled;
extern int ai_thinking;

// Function declarations
void initialize_gui(void);
void cleanup_gui(void);
void render_game(SDL_Renderer *renderer, TTF_Font *font);
void handle_event(SDL_Event *event);
void initialize_buttons(void);
int handle_button_click(SDL_Event *event);
void render_board(SDL_Renderer *renderer);
void render_pieces(SDL_Renderer *renderer);
void render_buttons(SDL_Renderer *renderer, TTF_Font *font);
void render_button(SDL_Renderer *renderer, TTF_Font *font, Button *button);
void render_error_message(SDL_Renderer *renderer, TTF_Font *font);
void render_game_over(SDL_Renderer *renderer, TTF_Font *font);
void handle_mouse_click(int x, int y);
void handle_promotion(SDL_Event *event);
void start_game(void);
void save_current_game(void);
void load_saved_game(void);
void back_to_menu(void);
void initialize_playback(void);
void play_next_move(void);
void move_piece(char *move);
void render_text(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, SDL_Color color);
void render_chessboard(SDL_Renderer *renderer);
void handle_mouse_drag(SDL_Event *event, char *move);
void handle_ai_promotion(void);
void check_game_over(void);

#endif // CHESS_GUI_H 