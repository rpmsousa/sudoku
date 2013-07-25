#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#include "sudoku.h"

/*
Board layout

(0, 0)		----->		(MAX_COLUMNS, 0)
|				|
|				|
|				|
|				|
v				v
(0, MAX_ROWS)	----->		(MAX_COLUMNS, MAX_ROWS)

*/

static inline int pos(int x, int y)
{
	return x + y * MAX_COLUMNS;

}

static inline int square_x(int x)
{
	return (x / SQUARES_PER_ROW) * SQUARES_PER_ROW;
}

static inline int square_y(int y)
{
	return (y / SQUARES_PER_COLUMN) * SQUARES_PER_COLUMN;
}

static inline int square_pos(int x, int y)
{
	return (x / SQUARES_PER_ROW) + (y / SQUARES_PER_COLUMN) * SQUARES_PER_ROW;
}

/* Returns the value in a given board position */
static inline int board_get_pos(struct board *board, int pos)
{
	return board->value[pos];
}

/* Returns the value in a given board position */
static inline int board_get_xy(struct board *board, int x, int y)
{
	return board_get_pos(board, pos(x, y));
}

#if 1
void move_print(struct board *board)
{
	struct move *move = board->cur_move;

	printf("%ld: (%d, %d)=%d %d %x\n", move - &board->move_list[0], move->x, move->y, board->value[pos(move->x, move->y)], move->val, move->tried);
	printf("%3x %3x %3x %3x\n", move->row_pos, move->column_pos, move->square_pos, move->xy_candidate);
}


void board_print(struct board *board)
{
	int x, y;

	for (y = 0; y < MAX_ROWS; y++) {
		for (x = 0; x < MAX_COLUMNS; x++) {
			if (!board->value[pos(x, y)])
				printf("_ ");
			else
				printf("%1d ", board->value[pos(x, y)]);

			if (!((x + 1) % 3))
				printf("  ");
		}

		if (!((y + 1) % 3))
			printf("\n");

		printf("\n");
	}

	printf("\n");
}

void board_print_candidate_list(struct board *board)
{
	struct candidate_list *candidates;
	int x, y, d;

	for (y = 0; y < MAX_ROWS; y++) {
		for (x = 0; x < MAX_COLUMNS; x++) {
			candidates = board->candidates + pos(x ,y);

			printf("%2d: ", candidates->n);
			for (d = 1; d <= MAX_DIGITS; d++)
				if (candidates->list & (1 << d))
					printf("%1d ", d);
				else
					printf("  ");

			if (!((x + 1) % 3))
				printf("   ");
		}

		if (!((y + 1) % 3))
			printf("\n");

		printf("\n");
	}

	printf("\n");
}

void board_print_digit_positions(struct pos_digit *plist, int val)
{
	int x, y;

	plist += val - 1; 

	for (y = 0; y < MAX_ROWS; y++) {
		for (x = 0; x < MAX_COLUMNS; x++) {
			if (!plist->pos[pos(x, y)])
				printf("_ ");
			else
				printf("%1d ", val);

			if (!((x + 1) % 3))
				printf("  ");
		}

		printf("%1d", plist->row_n[y]);

		if (!((y + 1) % 3))
			printf("\n");

		printf("\n");
	}

	for (x = 0; x < MAX_COLUMNS; x++) {

		printf("%1d ", plist->column_n[x]);

		if (!((x + 1) % 3))
			printf("  ");
	}

	printf("\n\n");
}
#else
static inline void move_print(struct board *board) { }

static inline void board_print(struct board *board) { }

static inline void board_print_candidate_list(struct board *board) { }

static inline void board_print_digit_positions(struct pos_digit *plist, int val) { }
#endif

/* Checks if a value is already present in a given column */
int check_column_for_val(struct board *board, int x, int val)
{
	int y;

	for (y = 0; y < MAX_ROWS; y++)
		if (board->value[pos(x, y)] == val)
			return 1;

	return 0;
}

/* Checks if a value is already present in a given row */
int check_row_for_val(struct board *board, int y, int val)
{
	int x;

	for (x = 0; x < MAX_COLUMNS; x++)
		if (board->value[pos(x, y)] == val)
			return 1;

	return 0;
}

/* Checks if a value is already present in a given square */
int check_square_for_val(struct board *board, int x, int y, int val)
{
	int sx = square_x(x);
	int sy = square_y(y);

	for (x = 0; x < SQUARE_SIZE; x++)
		for (y = 0; y < SQUARE_SIZE; y++)
			if (board->value[pos(sx + x, sy + y)] == val)
				return 1;

	return 0;
}

void digit_pos_remove(struct board *board, int x, int y, int val)
{
	struct pos_digit *plist = board->plist + val - 1;

	plist->pos[pos(x, y)] = 0;

	plist->row_n[y]--;
	plist->column_n[x]--;
	plist->square_n[square_pos(x, y)]--;
}

void digit_pos_add(struct board *board, int x, int y, int val)
{
	struct pos_digit *plist = board->plist + val - 1;

	plist->pos[pos(x, y)] = 1;

	plist->row_n[y]++;
	plist->column_n[x]++;
	plist->square_n[square_pos(x, y)]++;
}

int candidate_remove(struct board *board, int x, int y, int val)
{
	struct candidate_list *candidates = board->candidates + pos(x, y);

	if (candidates->list & (1 << val)) {
		candidates->list &= ~(1 << val);
		candidates->n--;

		digit_pos_remove(board, x, y, val);

		return 1;
	}

	return 0;
}

void candidate_add(struct board *board, int x, int y, int val)
{
	struct candidate_list *candidates = board->candidates + pos(x, y);

	if (!(candidates->list & (1 << val))) {
		candidates->list |= (1 << val);
		candidates->n++;

		digit_pos_add(board, x, y, val);
	}
}

/* Remove all candidates with a given value from a column */
/* Return list of positions changed, for backtracking */
int candidate_remove_from_column(struct board *board, int x, int val)
{
	int column_pos = 0;
	int y;

	for (y = 0; y < MAX_ROWS; y++)
		column_pos |= candidate_remove(board, x, y, val) << y;

	return column_pos;
}

/* Remove all candidates with a given value from a row */
/* Return list of positions changed, for backtracking */
int candidate_remove_from_row(struct board *board, int y, int val)
{
	int row_pos = 0;
	int x;

	for (x = 0; x < MAX_COLUMNS; x++)
		row_pos |= candidate_remove(board, x, y, val) << x;

	return row_pos;
}

/* Remove all candidates with a given value from a square */
/* Return list of positions changed, for backtracking */
int candidate_remove_from_square(struct board *board, int x, int y, int val)
{
	int square_pos = 0;
	int sx = square_x(x);
	int sy = square_y(y);

	for (y = 0; y < SQUARE_SIZE; y++)
		for (x = 0; x < SQUARE_SIZE; x++)
			square_pos |= candidate_remove(board, sx + x, sy + y, val) << (x + y * SQUARE_SIZE);

	return square_pos;
}

/* Remove all candidates from a position */
/* Return list of candidates removed, for backtracking */
int candidate_remove_from_xy(struct board *board, int x, int y)
{
	int xy_candidates = 0;
	int d;

	for (d = 1; d <= MAX_DIGITS; d++)
		xy_candidates |= candidate_remove(board, x, y, d) << d;

	return xy_candidates;
}

void candidate_add_to_column(struct board *board, int x, int column_pos, int val)
{
	int y;

	for (y = 0; y < MAX_ROWS; y++) {
		if (column_pos & (1 << y))
			candidate_add(board, x, y, val);
	}
}

void candidate_add_to_row(struct board *board, int y, int row_pos, int val)
{
	int x;

	for (x = 0; x < MAX_COLUMNS; x++) {
		if (row_pos & (1 << x))
			candidate_add(board, x, y, val);
	}
}

void candidate_add_to_square(struct board *board, int x, int y, int square_pos, int val)
{
	int sx = square_x(x);
	int sy = square_y(y);

	for (x = 0; x < SQUARE_SIZE; x++)
		for (y = 0; y < SQUARE_SIZE; y++) {
			if (square_pos & (1 << (x + y * SQUARE_SIZE)))
				candidate_add(board, sx + x, sy +y, val);
		}
}

void candidate_add_to_xy(struct board *board, int x, int y, int xy_candidates)
{
	int d;

	for (d = 1; d <= MAX_DIGITS; d++)
		if (xy_candidates & (1 << d))
			candidate_add(board, x, y, d);
}

/* Sets a value in a given board position, updating all lookup tables */
struct move *board_new_move(struct board *board, int x, int y, int val)
{
	struct move *move = board->cur_move;
	int old_val = board->value[pos(x, y)];

	if (old_val) {
		printf("board position(%d, %d) occupied (%d)\n", x, y, old_val);
		return move;
	}

	if (val < 1 || val > MAX_DIGITS) {
		printf("board position(%d, %d) value (%d) invalid\n", x, y, old_val);
		return move;
	}

	/* Record current state for backtrack */
	move->x = x;
	move->y = y;
	move->val = val;

	/* Update lookup tables and keep track of modifications */
	move->row_pos = candidate_remove_from_row(board, y, val);
	move->column_pos = candidate_remove_from_column(board, x, val);
	move->square_pos = candidate_remove_from_square(board, x, y, val);
	move->xy_candidate = candidate_remove_from_xy(board, x, y);

	board->plist[val - 1].row[y] = 1;
	board->plist[val - 1].column[x] = 1;
	board->plist[val - 1].square[square_pos(x, y)] = 1;

	board->value[pos(x, y)] = val;

	board->empty--;

	move_print(board);

	board_print_digit_positions(board->plist, val);

	board->cur_move++;

	return board->cur_move;
}

struct move *board_revert_move(struct board *board)
{
	struct move *move = board->cur_move - 1;
	int x = move->x;
	int y = move->y;
	int val = board->value[pos(x, y)];

	move_print(board);

	candidate_add_to_row(board, y, move->row_pos, val);
	candidate_add_to_column(board, x, move->column_pos, val);
	candidate_add_to_square(board, x, y, move->square_pos, val);
	candidate_add_to_xy(board, x, y, move->xy_candidate);

	board->plist[val - 1].row[y] = 0;
	board->plist[val - 1].column[x] = 0;
	board->plist[val - 1].square[square_pos(x, y)] = 0;

	board->value[pos(x, y)] = 0;

	board->empty++;

	board->cur_move->tried = 0;

	board->cur_move--;

	return move;
}

#define POS_FOR_DIGIT

/* Returns the position on the board with less possibilities */
int board_get_next_move(struct board *board)
{
	struct move *cur_move = board->cur_move;
	struct candidate_list *candidates;
	struct pos_digit *plist;
	int x, y, s, sx, sy;
	int n = MAX_DIGITS + 1;
	int d;
	int found = 0;

	if (!cur_move->tried) {

		/* Search the board position with least candidates (and still empty) */
		/* If a board position with 0 candidates is still emtpy return error */
		for (y = 0; y < MAX_ROWS; y++)
			for (x = 0; x < MAX_COLUMNS; x++) {
				if (board_get_xy(board, x, y) != 0)
					continue;

				candidates = board->candidates + pos(x, y);

				/* No more possible moves for a given position */
				if (!candidates->n) {
					printf("No move for (%d, %d)\n", x, y);
					return -1;
				}

				if (candidates->n < n) {
					n = candidates->n;

					cur_move->x = x;
					cur_move->y = y;
					cur_move->type = M_TYPE_POSITION;

					found = 1;
				}
			}


		/* Search all the digits to find the one with least possible positions */
		for (d = 1; d <= MAX_DIGITS; d++) {
			plist = board->plist + d - 1;

			for (y = 0; y < MAX_ROWS; y++) {
				if (plist->row[y])
					continue;

				if (!plist->row_n[y]) {
					printf("No row(%d) position for %d\n", y, d);
					return -1;
				}

#ifdef POS_FOR_DIGIT
				if (plist->row_n[y] < n) {
					n = plist->row_n[y];

					cur_move->y = y;
					cur_move->val = d;
					cur_move->type = M_TYPE_ROW;

					found = 1;
				}
#endif
			}

			for (x = 0; x < MAX_COLUMNS; x++) {
				if (plist->column[x])
					continue;

				if (!plist->column_n[x]) {
					printf("No column(%d) position for %d\n", x, d);
					return -1;
				}
#ifdef POS_FOR_DIGIT
				if (plist->column_n[x] < n) {
					n = plist->column_n[x];

					cur_move->x = x;
					cur_move->val = d;
					cur_move->type = M_TYPE_COLUMN;

					found = 1;
				}
#endif
			}

			for (s = 0; s < MAX_SQUARES; s++) {
				if (plist->square[s])
					continue;

				if (!plist->square_n[s]) {
					printf("No square(%d) position for %d\n", s, d);
					return -1;
				}

				if (plist->square_n[s] < n) {
					n = plist->square_n[s];

					cur_move->x = (s % SQUARE_SIZE) * SQUARE_SIZE;
					cur_move->y = (s / SQUARE_SIZE) * SQUARE_SIZE;
					cur_move->val = d;
					cur_move->type = M_TYPE_SQUARE;

					found = 1;
				}
			}
		}

		if (!found)
			return -1;
	}


	switch (cur_move->type) {
	/* Search for the candidate with the least possible positions */
	case M_TYPE_POSITION:
		candidates = board->candidates + pos(cur_move->x, cur_move->y);

		found = 0;
		n = MAX_DIGITS + 1;
		for (d = 1; d <= MAX_DIGITS; d++) {
			plist = board->plist + d - 1;

			if ((candidates->list & (1 << d)) && !(cur_move->tried & (1 << d))) {
				if (plist->row_n[cur_move->y] < n) {
					n = plist->row_n[cur_move->y];

					cur_move->val = d;
					found = 1;
				}

				if (plist->column_n[cur_move->x] < n) {
					n = plist->column_n[cur_move->x];

					cur_move->val = d;
					found = 1;
				}

				if (plist->square_n[square_pos(cur_move->x, cur_move->y)] < n) {
					n = plist->square_n[square_pos(cur_move->x, cur_move->y)];

					cur_move->val = d;
					found = 1;
				}
			}
		}

		if (!found)
			return -1;

		cur_move->tried |= 1 << cur_move->val;
		break;

	/* Search for the position with the least possible candidates */
	case M_TYPE_ROW:
		plist = board->plist + cur_move->val - 1;

		found = 0;
		n = MAX_DIGITS + 1;

		for (x = 0; x < MAX_COLUMNS; x++) {
			candidates = board->candidates + pos(x, cur_move->y);

			if (plist->pos[pos(x, cur_move->y)] && !(cur_move->tried & (1 << x))) {
				if (candidates->n < n) {
					n = candidates->n;

					cur_move->x = x;
					found = 1;
				}
			}
		}
		
		if (!found)
			return -1;

		cur_move->tried |= 1 << cur_move->x;
		printf("Best row digit (%d, %d) = %d %d\n", cur_move->x, cur_move->y, cur_move->val, n);

		break;

	/* Search for the position with the least possible candidates */
	case M_TYPE_COLUMN:
		plist = board->plist + cur_move->val - 1;

		found = 0;
		n = MAX_DIGITS + 1;

		for (y = 0; y < MAX_ROWS; y++) {
			candidates = board->candidates + pos(cur_move->x, y);

			if (plist->pos[pos(cur_move->x, y)] && !(cur_move->tried & (1 << y))) {
				if (candidates->n < n) {
					n = candidates->n;

					cur_move->y = y;
					found = 1;
				}
			}
		}

		if (!found)
			return -1;

		cur_move->tried |= 1 << cur_move->y;
		printf("Best column digit (%d, %d) = %d %d\n", cur_move->x, cur_move->y, cur_move->val, n);

		break;

	/* Search for the position with the least possible candidates */
	case M_TYPE_SQUARE:
		plist = board->plist + cur_move->val - 1;

		found = 0;
		n = MAX_DIGITS + 1;

		sx = square_x(cur_move->x);
		sy = square_y(cur_move->y);

		for (y = 0; y < SQUARE_SIZE; y++)
			for (x = 0; x < SQUARE_SIZE; x++) {
				candidates = board->candidates + pos(sx + x, sy + y);

				if (plist->pos[pos(sx + x, sy + y)] && !(cur_move->tried & (1 << (x + y * SQUARE_SIZE)))) {
					if (candidates->n < n) {
						n = candidates->n;

						cur_move->x = sx + x;
						cur_move->y = sy + y;
						found = 1;
					}
				}
			}

		if (!found)
			return -1;

		cur_move->tried |= 1 << ((cur_move->x - square_x(cur_move->x)) + (cur_move->y - square_y(cur_move->y)) * SQUARE_SIZE);
		printf("Best square digit (%d, %d) = %d %d\n", cur_move->x, cur_move->y, cur_move->val, n);

		break;
	}

	printf("%1d: (%1d, %1d)=%d %x %ld\n", n, cur_move->x, cur_move->y, cur_move->val, cur_move->tried, board->cur_move - board->move_list);

	return 0;
}


void candidates_init(struct candidate_list *candidates)
{
	int pos, d;

	for (pos = 0; pos < MAX_POS; pos++) {

		for (d = 1; d <= MAX_DIGITS; d++) {
			candidates->list |= 1 << d;
			candidates->n++;
		}

		candidates++;
	}
}

void plist_init(struct pos_digit *plist)
{
	int d, x, y, s;

	for (d = 0; d < MAX_DIGITS; d++, plist++) {

		for (y = 0; y < MAX_ROWS; y++)
			for (x = 0; x < MAX_COLUMNS; x++)
				plist->pos[pos(x, y)] = 1;

		for (y = 0; y < MAX_ROWS; y++) {
			plist->row_n[y] = MAX_COLUMNS;
			plist->row[y] = 0;
		}

		for (x = 0; x < MAX_COLUMNS; x++) {
			plist->column_n[x] = MAX_ROWS;
			plist->column[x] = 0;
		}

		for (s = 0; s < MAX_SQUARES; s++) {
			plist->square_n[s] = MAX_SQUARES;
			plist->square[s] = 0;
		}
	}
}

void board_setup(struct board *board, int *init)
{
	int x, y;

	for (y = 0; y < MAX_ROWS; y++)
		for (x = 0; x < MAX_COLUMNS; x++) {
			board_new_move(board, x, y, init[pos(x, y)]);
		}
}

void board_init(struct board *board)
{
	memset(board, 0, sizeof(struct board));

	board->empty = MAX_COLUMNS * MAX_ROWS;

	board->cur_move = &board->move_list[0];

	candidates_init(board->candidates);

	plist_init(board->plist);
}

void board_solve(struct board *board)
{
	struct move *cur_move = board->cur_move;
	int total_moves = 0;

	cur_move->tried = 0;

	while (board->empty) {

		if (board_get_next_move(board) >= 0) {
			total_moves++;

			cur_move = board_new_move(board, cur_move->x, cur_move->y, cur_move->val);

			fflush(stdout);
		} else {

			/* backtrack */

			cur_move = board_revert_move(board);
		}

		board_print(board);
		board_print_candidate_list(board);
	}


	printf("Total moves: %d\n", total_moves);
}

int init1[] = {
	0, 0, 0,	0, 0, 0,	0, 0, 6,
	0, 0, 0,	0, 1, 0,	5, 9, 4,
	0, 0, 0,	2, 0, 4,	7, 0, 0,

	0, 3, 0,	0, 0, 8,	0, 1, 0,
	5, 0, 2,	0, 0, 0,	9, 0, 8,
	0, 7, 0,	9, 0, 0,	0, 4, 0,

	0, 0, 9,	3, 0, 7,	0, 0, 0,
	1, 5, 4,	0, 6, 0,	0, 0, 0,
	7, 0, 0,	0, 0, 0,	0, 0, 0
};

int init2[] = {
	1, 0, 0,	0, 0, 0,	0, 0, 0,
	0, 0, 0,	0, 0, 0,	0, 0, 0,
	0, 0, 0,	0, 0, 0,	0, 0, 0,

	0, 0, 0,	0, 0, 0,	0, 0, 0,
	0, 0, 0,	0, 0, 0,	0, 0, 0,
	0, 0, 0,	0, 0, 0,	0, 0, 0,

	0, 0, 0,	0, 0, 0,	0, 0, 0,
	0, 0, 0,	0, 0, 0,	0, 0, 0,
	0, 0, 0,	0, 0, 0,	0, 0, 0
};

int init3[] = {
	0, 7, 0,	8, 0, 2,	0, 6, 0,
	4, 3, 0,	1, 0, 6,	0, 7, 9,
	0, 0, 6,	9, 0, 3,	8, 0, 0,

	6, 5, 9,	0, 0, 0,	7, 1, 4,
	0, 0, 0,	0, 0, 0,	0, 0, 0,
	7, 4, 3,	0, 0, 0,	9, 8, 2,

	0, 0, 7,	4, 0, 5,	1, 0, 0,
	5, 8, 0,	6, 0, 9,	0, 3, 7,
	0, 9, 0,	3, 0, 7,	0, 2, 0
};

int init4[] = {
	0, 0, 5,	9, 0, 4,	2, 0, 0,
	0, 0, 0,	0, 1, 0,	0, 0, 0,
	4, 0, 0,	8, 0, 3,	0, 0, 6,

	2, 0, 9,	3, 0, 6,	7, 0, 5,
	0, 5, 0,	0, 0, 0,	0, 8, 0,
	6, 0, 1,	7, 0, 2,	4, 0, 9,

	1, 0, 0,	6, 0, 5,	0, 0, 3,
	0, 0, 0,	0, 7, 0,	0, 0, 0,
	0, 0, 4,	1, 0, 8,	5, 0, 0
};

/* http://www.sudokuwiki.org/sudoku.htm */

int hard_17_clues[] = {
	0, 0, 2,	0, 9, 0,	3, 0, 0,
	8, 0, 5,	0, 0, 0,	0, 0, 0,
	1, 0, 0,	0, 0, 0,	0, 0, 0,

	0, 9, 0,	0, 6, 0,	0, 4, 0,
	0, 0, 0,	0, 0, 0,	0, 5, 8,
	0, 0, 0,	0, 0, 0,	0, 0, 1,

	0, 7, 0,	0, 0, 0,	2, 0, 0,
	3, 0, 0,	5, 0, 0,	0, 0, 0,
	0, 0, 0,	1, 0, 0,	0, 0, 0
};

int arto_inkala[] = {
	8, 0, 0,	0, 0, 0,	0, 0, 0,
	0, 0, 3,	6, 0, 0,	0, 0, 0,
	0, 7, 0,	0, 9, 0,	2, 0, 0,

	0, 5, 0,	0, 0, 7,	0, 0, 0,
	0, 0, 0,	0, 4, 5,	7, 0, 0,
	0, 0, 0,	1, 0, 0,	0, 3, 0,

	0, 0, 1,	0, 0, 0,	0, 6, 8,
	0, 0, 8,	5, 0, 0,	0, 1, 0,
	0, 9, 0,	0, 0, 0,	4, 0, 0
};

int arto_inkala2[] = {
	9, 0, 0,	0, 0, 0,	0, 0, 0,
	0, 0, 4,	7, 0, 0,	0, 0, 0,
	0, 8, 0,	0, 1, 0,	3, 0, 0,

	0, 6, 0,	0, 0, 8,	0, 0, 0,
	0, 0, 0,	0, 5, 6,	8, 0, 0,
	0, 0, 0,	2, 0, 0,	0, 4, 0,

	0, 0, 2,	0, 0, 0,	0, 7, 9,
	0, 0, 9,	6, 0, 0,	0, 2, 0,
	0, 1, 0,	0, 0, 0,	5, 0, 0
};

int unsolveable_29[] = {
	0, 0, 2,	8, 0, 0,	0, 0, 0,
	0, 3, 0,	0, 6, 0,	0, 0, 7,
	1, 0, 0,	0, 0, 0,	0, 4, 0,

	6, 0, 0,	0, 9, 0,	0, 0, 0,
	0, 5, 0,	6, 0, 0,	0, 0, 9,
	0, 0, 0,	0, 5, 7,	0, 6, 0,

	0, 0, 0,	3, 0, 0,	1, 0, 0,
	0, 7, 0,	0, 0, 6,	0, 0, 8,
	4, 0, 0,	0, 0, 0,	0, 2, 0
};

int main(int argc, char *argv[])
{
	struct board my_board;

	board_init(&my_board);
//	board_setup(&my_board, hard_17_clues);

//	board_setup(&my_board, unsolveable_29);
	board_setup(&my_board, arto_inkala2);
//	board_init(&my_board, init2);

	board_print(&my_board);
	board_print_candidate_list(&my_board);

	board_solve(&my_board);

	return 0;
}
