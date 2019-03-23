#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#include "sudoku.h"

/* transform board coordinates (i, j) to array index */
static inline int pos_index(int i, int j)
{
	return (i + j * MAX_COLUMNS);
}

/* Returns the value in a given board position */
static inline int value_get(struct board *b, int i, int j)
{
	return b->value[pos_index(i, j)];
}

static inline void value_set(struct board *b, int i, int j, int val)
{
	b->value[pos_index(i, j)] = val;
}

/* Checks if a value is already present in a given column */
static int check_column_for_val(struct board *b, int i, int val)
{
	int l;

	for (l = 0; l < MAX_ROWS; l++)
		if (value_get(b, i, l) == val)
			return 1;

	return 0;
}

/* Checks if a value is already present in a given row */
static int check_row_for_val(struct board *b, int j, int val)
{
	int k;

	for (k = 0; k < MAX_COLUMNS; k++)
	     if (value_get(b, k, j) == val)
			return 1;

	return 0;
}

/* Checks if a value is already present in a given square */
static int check_square_for_val(struct board *b, int i, int j, int val)
{
	int m = (i / SQUARES_PER_ROW) * SQUARES_PER_ROW;
	int n = (j / SQUARES_PER_COLUMN) * SQUARES_PER_COLUMN;

	int k, l;

	for (l = 0; l < SQUARE_SIZE; l++)
		for (k = 0; k < SQUARE_SIZE; k++)
			if (value_get(b, m + k, n + l) == val)
				return 1;

	return 0;
}

static inline int __candidate_clear(struct board *b, int i, int j, int val)
{
	struct candidate_list *c = &b->candidates[pos_index(i, j)];

	if (!c->ref[val - 1]) {
		if (!c->n)
			return -1;

		c->n--;

		(b->row_candidates[j].count[val - 1])--;
		(b->column_candidates[i].count[val - 1])--;
		(b->square_candidates[i / 3 + (j / 3) * 3].count[val - 1])--;
	}

	c->ref[val - 1]++;

	return 0;
}

static inline int __candidate_add(struct board *b, int i, int j, int val)
{
	struct candidate_list *c = &b->candidates[pos_index(i, j)];

	/* If reference count is already 0, then there is a bug */
	if (!c->ref[val - 1]) {
		printf("error (%d, %d), %d\n", i, j, val);
		return -1;
	}

	/* Decrease reference count */
	c->ref[val - 1]--;

	/* If the reference count drops to 0, then the candidate is actually added */
	if (!c->ref[val - 1]) {
		c->n++;

		b->row_candidates[j].count[val - 1]++;
		b->column_candidates[i].count[val - 1]++;
		b->square_candidates[i / 3 + (j / 3) * 3].count[val - 1]++;
	}

	return 0;
}

static void candidate_clear_column(struct board *b, int j, int val)
{
	int l;

	for (l = 0; l < MAX_ROWS; l++)
		__candidate_clear(b, j, l, val);
}

static void candidate_clear_row(struct board *b, int i, int val)
{
	int k;

	for (k = 0; k < MAX_COLUMNS; k++)
		__candidate_clear(b, k, i, val);
}

static void candidate_clear_square(struct board *b, int i, int j, int val)
{
	int m = (i / SQUARES_PER_ROW) * SQUARES_PER_ROW;
	int n = (j / SQUARES_PER_COLUMN) * SQUARES_PER_COLUMN;
	int k, l;

	for (l = 0; l < SQUARE_SIZE; l++)
		for (k = 0; k < SQUARE_SIZE; k++)
			__candidate_clear(b, m + k, n + l, val);
}

static int candidate_clear(struct board *b, int i, int j, int val)
{
	struct candidate_list *c = &b->candidates[pos_index(i, j)];
	int d;

	if (c->ref[val - 1]) {
		printf("error (%d, %d) %d not possible\n", i, j, val);
		return -1;
	}

	for (d = 1; d <= MAX_DIGITS; d++)
		if (d != val)
			__candidate_clear(b, i, j, d);

	candidate_clear_row(b, j, val);
	candidate_clear_column(b, i, val);
	candidate_clear_square(b, i, j, val);

	return 0;
}

static void candidate_add_column(struct board *b, int j, int val)
{
	int l;

	for (l = 0; l < MAX_ROWS; l++)
		__candidate_add(b, j, l, val);
}

static void candidate_add_row(struct board *b, int i, int val)
{
	int k;

	for (k = 0; k < MAX_COLUMNS; k++)
		__candidate_add(b, k, i, val);
}

static void candidate_add_square(struct board *b, int i, int j, int val)
{
	int m = (i / SQUARES_PER_ROW) * SQUARES_PER_ROW;
	int n = (j / SQUARES_PER_COLUMN) * SQUARES_PER_COLUMN;
	int k, l;

	for (l = 0; l < SQUARE_SIZE; l++)
		for (k = 0; k < SQUARE_SIZE; k++)
			__candidate_add(b, m + k, n + l, val);
}

/* Updates all candidate lists after a given value is removed from a board position */
/* This will affect the entire row, column and square */
static int candidate_add(struct board *b, int i, int j, int val)
{
	struct candidate_list *c = &b->candidates[pos_index(i, j)];
	int d;

	for (d = 1; d <= MAX_DIGITS; d++)
		if (d != val)
			__candidate_add(b, i, j, d);

	candidate_add_row(b, j, val);
	candidate_add_column(b, i, val);
	candidate_add_square(b, i, j, val);

	if (c->ref[val - 1]) {
		printf("error (%d, %d) %d not possible\n", i, j, val);
		return -1;
	}

	return 0;
}

static int candidate_get_next(struct board *b, int pos, int value)
{
	struct candidate_list *c = &b->candidates[pos];
	int d;

	if ((value < 0) || (value > MAX_DIGITS))
		return -1;

	for (d = value; d < MAX_DIGITS; d++)
		if (!c->ref[d])
			return d + 1;

	return -1;
}

static int find_least_candidates(struct board *b, int *pos)
{
	int i, best_pos = -2;
	int n = MAX_DIGITS + 1;

	for (i = *pos; i < MAX_POS; i++) {
		struct candidate_list *c = &b->candidates[i];

		/* already fill */
		if (b->value[i])
			continue;

		/* no possible candidate */
		if (!c->n)
			return -1;

		/* single candidate */
		if (c->n == 1) {
			*pos = i;
			return 1;
		}

		if (c->n < n) {
			n = c->n;
			best_pos = i;
		}
	}

	for (i = 0; i < *pos; i++) {
		struct candidate_list *c = &b->candidates[i];

		/* already fill */
		if (b->value[i])
			continue;

		/* no possible candidate */
		if (!c->n)
			return -1;

		/* single candidate */
		if (c->n == 1) {
			*pos = i;
			return 1;
		}

		if (c->n < n) {
			n = c->n;
			best_pos = i;
		}
	}

	*pos = best_pos;

	return n;
}


static int find_row_least_candidates(struct board *b, int *_pos, int *value)
{
	int i, j, pos, val;

	for (j = 0; j < MAX_ROWS; j++)
		for (val = 0; val < MAX_DIGITS; val++) {
			if (b->row_candidates[j].count[val] == 1)
				goto found;
		}

	return -1;

found:
	for (i = 0; i < MAX_COLUMNS; i++) {
		pos = pos_index(i, j);
		if (!b->candidates[pos].ref[val]) {
			*_pos = pos;
			*value = val + 1;
			return 1;
		}
	}

	printf("error (%d, %d) %d not possible\n", i, j, val);

	return -1;
}

static int find_column_least_candidates(struct board *b, int *_pos, int *value)
{
	int i, j, pos, val;

	for (i = 0; i < MAX_COLUMNS; i++)
		for (val = 0; val < MAX_DIGITS; val++) {
			if (b->column_candidates[i].count[val] == 1)
				goto found;
		}

	return -1;

found:
	for (j = 0; j < MAX_ROWS; j++) {
		pos = pos_index(i, j);
		if (!b->candidates[pos].ref[val]) {
			*_pos = pos;
			*value = val + 1;
			return 1;
		}
	}

	printf("error (%d, %d) %d not possible\n", i, j, val);

	return -1;
}

static void board_print(struct board *b)
{
	int i, j;

	for (j = 0; j < MAX_ROWS;j ++) {
		for (i = 0; i < MAX_COLUMNS; i++) {
			if (!value_get(b, i, j))
				printf("__ ");
			else
				printf("%2d ", value_get(b, i, j));

			if (!((i + 1)% 3))
				printf("   ");
		}

		if (!((j + 1)% 3))
			printf("\n");

		printf("\n");
	}

	printf("\n");
}


static void board_print_candidates(struct board *b)
{
	int i, j, val;

	for (j = 0; j < MAX_ROWS; j++) {
		for (i = 0; i < MAX_COLUMNS; i++) {
			int pos = pos_index(i, j);
			struct candidate_list *c = &b->candidates[pos];

			for (val = 0; val < MAX_DIGITS; val++) {
				if (b->value[pos])
					printf("X");
				else if (!c->ref[val])
					printf("%1d", val + 1);
				else
					printf("_");
			}

			printf("(%d)", c->n);

			if (!((i + 1)% 3))
				printf("   ");
			else
				printf(" ");
		}

		if (!((j + 1)% 3))
			printf("\n");

		printf("\n");
	}

	printf("\n");

	for (j = 0; j < MAX_ROWS; j++) {
		for (val = 0; val < MAX_DIGITS; val++)
			printf("%d", b->row_candidates[j].count[val]);

		printf("\n");
	}

	printf("\n");

	for (j = 0; j < MAX_COLUMNS; j++) {
		for (val = 0; val < MAX_DIGITS; val++)
			printf("%d", b->column_candidates[j].count[val]);

		printf("\n");
	}

	printf("\n");

	for (j = 0; j < MAX_SQUARES; j++) {
		for (val = 0; val < MAX_DIGITS; val++)
			printf("%d", b->square_candidates[j].count[val]);

		printf("\n");
	}

	printf("\n");
}

static void board_init(struct board *b)
{
	int i, j;

	for (i = 0; i < MAX_POS; i++) {
		b->value[i] = 0;

		for (j = 0; j < MAX_DIGITS; j++) {
			b->candidates[i].ref[j] = 0;
		}

		b->candidates[i].n = MAX_DIGITS;
	}

	for (j = 0; j < MAX_DIGITS; j++) {

		for (i = 0; i < MAX_ROWS; i++)
			b->row_candidates[i].count[j] = MAX_DIGITS;

		for (i = 0; i < MAX_COLUMNS; i++)
			b->column_candidates[i].count[j] = MAX_DIGITS;

		for (i = 0; i < MAX_SQUARES; i++)
			b->square_candidates[i].count[j] = MAX_DIGITS;
	}

	b->empty = MAX_ROWS * MAX_COLUMNS;

	b->move_n = 0;
	b->steps = 0;
	b->solutions = 0;
}

/* Sets a value in a given board position, updating all lookup tables */
static int board_set_val(struct board *b, int i, int j, int val)
{
	if (value_get(b, i, j))
		return -1;

	value_set(b, i, j, val);

	if (candidate_clear(b, i, j, val) < 0)
		return -1;

	b->empty--;

	return 0;
}

/* Clears a value in a given board position, updating all lookup tables */
static int board_clear_val(struct board *b, int i, int j)
{
	int val = value_get(b, i, j);

	if (!val)
		return -1;

	value_set(b, i, j, 0);

	if (candidate_add(b, i, j, val) < 0)
		return -1;

	b->empty++;

	return val;
}

static int board_backtrack(struct board *b, int *pos)
{
	struct move *m;
	int val;

	b->move_n--;

	m = &b->move_list[b->move_n];

	val = board_clear_val(b, m->i, m->j);

	*pos = pos_index(m->i, m->j);

	return val;
}

static int board_solve(struct board *b)
{
	int pos;
	struct move *m;
	int i, j;
	int val;
	int rc;

	while (1) {
//		board_print(b);

//		board_print_candidates(b);

		/* forward move */
		pos = 0;
		rc = find_least_candidates(b, &pos);
		if (rc < 0) {
	backtrack:
			do {
				if (!b->move_n) {
					printf("no solution\n");
					rc = -1;
					goto end;
				}

				/* backtrack */
				val = board_backtrack(b, &pos);
				val = candidate_get_next(b, pos, val);
			} while (val < 0);
		} else {
			val = candidate_get_next(b, pos, 0);
		}

		m = &b->move_list[b->move_n];
		i = pos % MAX_COLUMNS;
		j = pos / MAX_COLUMNS;

		board_set_val(b, i, j, val);
		m->i = i;
		m->j = j;

		b->move_n++;
		if (!b->solutions)
			b->steps++;

		printf("[%u, %u] (%d, %d) = %d\n", b->move_n, b->steps, i, j, val);

		if (!b->empty) {
			b->solutions++;
			goto backtrack;
			//break;
		}
	}
end:
	return rc;
}

static int board_setup(struct board *b, int *table_test)
{
	int i;

	for (i = 0; i < MAX_POS; i++)
		if (table_test[i])
			if (board_set_val(b, i % MAX_COLUMNS, i / MAX_COLUMNS, table_test[i]) < 0)
				return -1;

	return 0;
}

#if 0
int table_test[MAX_ROWS * MAX_COLUMNS] = {	0, 7, 0, 8, 0, 2, 0, 6, 0,
						4, 3, 0, 1, 0, 6, 0, 7, 9,
						0, 0, 6, 9, 0, 3, 8, 0, 0,
						6, 5, 9, 0, 0, 0, 7, 1, 4,
						0, 0, 0, 0, 0, 0, 0, 0, 0,
						7, 4, 3, 0, 0, 0, 9, 8, 2,
						0, 0, 7, 4, 0, 5, 1, 0, 0,
						5, 8, 0, 6, 0, 9, 0, 3, 7,
						0, 9, 0, 3, 0, 7, 0, 2, 0};
#elif 0
int table_test[MAX_ROWS * MAX_COLUMNS] = {	0, 0, 5, 9, 0, 4, 2, 0, 0,
						0, 0, 0, 0, 1, 0, 0, 0, 0,
						4, 0, 0, 8, 0, 3, 0, 0, 6,
						2, 0, 9, 3, 0, 6, 7, 0, 5,
						0, 5, 0, 0, 0, 0, 0, 8, 0,
						6, 0, 1, 7, 0, 2, 4, 0, 9,
						1, 0, 0, 6, 0, 5, 0, 0, 3,
						0, 0, 0, 0, 7, 0, 0, 0, 0,
						0, 0, 4, 1, 0, 8, 5, 0, 0};
#else
int table_test[MAX_ROWS * MAX_COLUMNS] = {	0, 0, 5, 0, 0, 1, 0, 0, 0,
						0, 0, 6, 0, 0, 0, 0, 9, 7,
						0, 0, 0, 2, 9, 0, 0, 0, 0,
						0, 5, 0, 0, 4, 0, 2, 0, 0,
						0, 1, 0, 9, 0, 7, 0, 0, 0,
						0, 0, 0, 1, 6, 0, 4, 0, 0,
						3, 0, 0, 0, 0, 5, 0, 0, 0,
						0, 0, 0, 0, 0, 0, 6, 3, 0,
						0, 2, 1, 0, 0, 9, 0, 0, 0};
#endif

int unsolveable_29[MAX_ROWS * MAX_COLUMNS] = {
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

int arto_inkala2[MAX_ROWS * MAX_COLUMNS] = {
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


int table_diabolic1[MAX_ROWS * MAX_COLUMNS] = {	0, 0, 0, 9, 0, 0, 0, 0, 0,
						2, 0, 8, 0, 4, 5, 0, 0, 6,
						6, 0, 0, 0, 0, 0, 0, 0, 0,
						0, 0, 0, 0, 3, 0, 0, 0, 0,
						8, 0, 0, 1, 0, 9, 0, 0, 2,
						9, 1, 5, 0, 0, 0, 0, 0, 0,
						0, 0, 3, 0, 0, 0, 2, 0, 5,
						0, 0, 0, 0, 9, 7, 0, 0, 0,
						0, 4, 0, 6, 0, 0, 7, 0, 0};


int table_diabolic2[MAX_ROWS * MAX_COLUMNS] = {	0, 8, 0, 0, 0, 0, 0, 0, 9,
						0, 0, 0, 0, 6, 8, 0, 1, 0,
						0, 5, 0, 3, 0, 0, 0, 7, 0,
						0, 0, 0, 0, 2, 0, 0, 0, 4,
						0, 0, 0, 6, 0, 1, 0, 0, 0,
						0, 0, 5, 0, 0, 4, 0, 3, 0,
						0, 4, 6, 2, 0, 0, 0, 0, 3,
						5, 3, 0, 0, 0, 0, 0, 9, 0,
						0, 9, 0, 0, 0, 0, 0, 0, 5};


int table_diabolic3[MAX_ROWS * MAX_COLUMNS] = {	9, 0, 0, 0, 0, 0, 0, 4, 7,
						0, 3, 0, 0, 0, 0, 0, 0, 6,
						0, 4, 5, 0, 2, 9, 0, 0, 3,
						0, 0, 0, 0, 0, 0, 0, 0, 2,
						0, 0, 6, 0, 9, 0, 7, 0, 0,
						1, 0, 0, 0, 3, 8, 0, 0, 0,
						0, 0, 0, 0, 0, 3, 0, 0, 0,
						0, 5, 2, 0, 0, 0, 6, 0, 0,
						0, 0, 0, 4, 0, 1, 0, 0, 0};

int table_diabolic4[MAX_ROWS * MAX_COLUMNS] = {	9, 0, 0, 0, 0, 0, 0, 4, 7,
						0, 0, 0, 0, 0, 0, 0, 0, 6,
						0, 0, 5, 0, 2, 9, 0, 0, 3,
						0, 0, 0, 0, 0, 0, 0, 0, 2,
						0, 0, 6, 0, 9, 0, 7, 0, 0,
						1, 0, 0, 0, 3, 8, 0, 0, 0,
						0, 0, 0, 0, 0, 3, 0, 0, 0,
						0, 5, 2, 0, 0, 0, 6, 0, 0,
						0, 0, 0, 4, 0, 1, 0, 0, 0};

int main(int argc, char *argv[])
{
	struct board b;

	board_init(&b);

	board_setup(&b, arto_inkala2);

	board_solve(&b);

	printf("solutions: %d, steps: %d\n", b.solutions, b.steps);
	board_print(&b);

	return 0;
}