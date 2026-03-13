CC = gcc
CFLAGS = -Wall -Wextra
LDFLAGS = -L/usr/local/lib
LDLIBS = -lSDL2 -lSDL2_ttf -lSDL2_image

SRCS = chess_gui.c chess_logic.c chess_ai.c
OBJS = $(SRCS:.c=.o)
TARGET = chess_game

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean 