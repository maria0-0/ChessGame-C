# ♟️ Chess Engine & GUI in C

A fully functional Chess Engine and Graphical User Interface (GUI) developed in C. This project was created as part of the "Programming Techniques" curriculum at Politehnica University of Timișoara (AC).

## 🚀 Technical Highlights
- **Engine Logic:** Complete implementation of chess rules, including castling, en passant, and pawn promotion.
- **AI Strategy:** Features a chess AI based on the **Minimax algorithm** with **Alpha-Beta Pruning** and positional evaluation tables.
- **Graphics:** Built using **SDL2** for hardware-accelerated rendering. 
- **Standards:** Support for **PGN** (Portable Game Notation) for saving and loading game sessions.

## 🛠️ Tech Stack
- **Language:** C (Standard C11)
- **Library:** SDL2 (Video, Image, TTF) 
- **Build System:** Makefile

## 🏗️ Architecture
- `chess_logic.c`: Core move validation and board state management.
- `chess_ai.c`: AI implementation (Evaluation functions and search algorithms).
- `chess_gui.c`: Event handling and graphical representation.
