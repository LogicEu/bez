# bez Makefile

NAME=bez
SRC=$(NAME).c

CC=gcc
STD=-std=c99
OPT=-O2
WFLAGS=-Wall -Wextra -pedantic
INC=-Iinclude
LIB=-lglfw

OS=$(shell uname -s)
ifeq ($(OS),Darwin)
    LIB+=-framework OpenGL
else
    LIB+=-lGL -lGLEW
endif

CFLAGS=$(STD) $(OPT) $(WFLAGS) $(INC) $(LIB)

$(NAME): $(SRC)
	$(CC) $^ -o $@ $(CFLAGS)

.PHONY: clean

clean:
	$(RM) $(NAME)

