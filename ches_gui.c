#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 640
#define TILE_SIZE (WINDOW_WIDTH / 8)
// Chessboard state (0 = empty, positive = white pieces, negative = black pieces)
int chessboard[8][8] = {0};
SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
// Initialize chessboard with starting positions
void initialize_chessboard() {
    // Black pieces
    chessboard[0][0] = -2; // Rook
    chessboard[0][1] = -3; // Knight
    chessboard[0][2] = -4; // Bishop
    chessboard[0][3] = -5; // Queen
    chessboard[0][4] = -6; // King
    chessboard[0][5] = -4; // Bishop
    chessboard[0][6] = -3; // Knight
    chessboard[0][7] = -2; // Rook
    for (int i = 0; i < 8; i++) {
        chessboard[1][i] = -1; // Pawns
    }

    // White pieces
    chessboard[7][0] = 2; // Rook
    chessboard[7][1] = 3; // Knight
    chessboard[7][2] = 4; // Bishop
    chessboard[7][3] = 5; // Queen
    chessboard[7][4] = 6; // King
    chessboard[7][5] = 4; // Bishop
    chessboard[7][6] = 3; // Knight
    chessboard[7][7] = 2; // Rook
    for (int i = 0; i < 8; i++) {
        chessboard[6][i] = 1; // Pawns
    }
}
// Chessboard colors
SDL_Color light_tile = {240, 217, 181, 255}; // Light brown
SDL_Color dark_tile = {181, 136, 99, 255};  // Dark brown

// Chessboard state (0 = empty, 1 = white piece, 2 = black piece)


// Drag-and-drop state
int dragging = 0;
int selected_row = -1, selected_col = -1;
int mouse_x = 0, mouse_y = 0;

// Initialize SDL
int init_sdl() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 0;
    }

    window = SDL_CreateWindow("Chess Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return 0;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        return 0;
    }

    return 1;
}

// Draw the chessboard
// Draw the chessboard and pieces
void draw_chessboard() {
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            SDL_Rect tile = {col * TILE_SIZE, row * TILE_SIZE, TILE_SIZE, TILE_SIZE};
            if ((row + col) % 2 == 0) {
                SDL_SetRenderDrawColor(renderer, light_tile.r, light_tile.g, light_tile.b, light_tile.a);
            } else {
                SDL_SetRenderDrawColor(renderer, dark_tile.r, dark_tile.g, dark_tile.b, dark_tile.a);
            }
            SDL_RenderFillRect(renderer, &tile);

            // Draw pieces
            if (chessboard[row][col] != 0) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // Default to white
                if (chessboard[row][col] < 0) {
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black pieces
                }
                SDL_Rect piece = {col * TILE_SIZE + TILE_SIZE / 4, row * TILE_SIZE + TILE_SIZE / 4, TILE_SIZE / 2, TILE_SIZE / 2};
                SDL_RenderFillRect(renderer, &piece);
            }
        }
    }

    // Draw the dragged piece
    if (dragging && selected_row != -1 && selected_col != -1) {
        SDL_SetRenderDrawColor(renderer, chessboard[selected_row][selected_col] > 0 ? 255 : 0, 255, 255, 255);
        SDL_Rect piece = {mouse_x - TILE_SIZE / 4, mouse_y - TILE_SIZE / 4, TILE_SIZE / 2, TILE_SIZE / 2};
        SDL_RenderFillRect(renderer, &piece);
    }
}

// Handle mouse events for drag-and-drop
void handle_mouse_event(SDL_Event *event) {
    if (event->type == SDL_MOUSEBUTTONDOWN) {
        int col = event->button.x / TILE_SIZE;
        int row = event->button.y / TILE_SIZE;

        if (chessboard[row][col] != 0) { // Select a piece
            dragging = 1;
            selected_row = row;
            selected_col = col;
        }
    } else if (event->type == SDL_MOUSEBUTTONUP) {
        if (dragging) {
            int col = event->button.x / TILE_SIZE;
            int row = event->button.y / TILE_SIZE;

            // Move the piece
            if (row >= 0 && row < 8 && col >= 0 && col < 8) {
                chessboard[row][col] = chessboard[selected_row][selected_col];
                chessboard[selected_row][selected_col] = 0;
            }

            dragging = 0;
            selected_row = -1;
            selected_col = -1;
        }
    } else if (event->type == SDL_MOUSEMOTION) {
        mouse_x = event->motion.x;
        mouse_y = event->motion.y;
    }
}

// Main function
int main(int argc, char *argv[]) {
    if (!init_sdl()) {
        return 1;
    }

    // Initialize chessboard with starting positions
    initialize_chessboard();

    int running = 1;
    SDL_Event event;

    while (running) {
        // Handle events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }
            handle_mouse_event(&event);
        }

        // Render the chessboard
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        draw_chessboard();
        SDL_RenderPresent(renderer);
    }

    // Clean up
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}