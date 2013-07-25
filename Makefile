CFLAGS=-Wall -O2 -g
CC=gcc

sudoku: sudoku.c sudoku.h
	$(CC) $(CFLAGS) $< -o $@
