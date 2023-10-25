# bez Makefile

NAME=bez
CC=gcc
STD=-std=c99
OPT=-O2
WFLAGS=-Wall -Wextra -pedantic
INC=-Iinclude -Isrc
LIB=-lglfw

BINDIR=bin
SRCDIR=src

SRC=$(wildcard $(SRCDIR)/*.c)
OBJS=$(patsubst $(SRCDIR)/%.c,$(BINDIR)/%.o,$(SRC))

OS=$(shell uname -s)
ifeq ($(OS),Darwin)
    LIB+=-framework OpenGL
else
    LIB+=-lm -lGL -lGLEW
endif

CFLAGS=$(STD) $(OPT) $(WFLAGS)

.PHONY: clean

$(NAME): $(OBJS)
	$(CC) $^ -o $@ $(CFLAGS) $(LIB)

$(BINDIR)/%.o: $(SRCDIR)/%.c $(BINDIR)
	$(CC) -c $< -o $@ $(CFLAGS) $(INC)

$(BINDIR):
	mkdir -p $@

clean:
	$(RM) $(NAME)
	$(RM) -r $(BINDIR)

