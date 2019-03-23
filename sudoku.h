#ifndef _SUDOKU_H_
#define _SUDOKU_H_

#define MAX_COLUMNS	9
#define MAX_ROWS	MAX_COLUMNS
#define SQUARE_SIZE	3 /* sqrt(MAX_COLUMNS) */

#define MAX_POS		(MAX_COLUMNS * MAX_ROWS)

#define SQUARES_PER_ROW		(MAX_COLUMNS / SQUARE_SIZE)
#define SQUARES_PER_COLUMN	(MAX_ROWS / SQUARE_SIZE)

#define MAX_SQUARES	(SQUARES_PER_ROW * SQUARES_PER_COLUMN)
#define MAX_DIGITS	MAX_COLUMNS

#define MAX_MOVES	(MAX_COLUMNS * MAX_ROWS)

#define M_TYPE_POSITION		1
#define M_TYPE_ROW		2
#define M_TYPE_COLUMN		3
#define M_TYPE_SQUARE		4


struct candidate_count {
	unsigned char count[MAX_DIGITS];
};

/* list of possible candidates for a given position */
struct candidate_list {
	unsigned char ref[MAX_DIGITS];
	unsigned int n;		/* Total number of possible candidates */
};

struct move {
	int i;
	int j;
};

struct board {
	unsigned char value[MAX_POS];

	/* for each position track which values are still possible */
	struct candidate_list candidates[MAX_POS];

	/* for each value track possible positions in each column, row, square */
	struct candidate_count row_candidates[MAX_ROWS];
	struct candidate_count column_candidates[MAX_COLUMNS];
	struct candidate_count square_candidates[MAX_SQUARES];


	struct move move_list[MAX_MOVES];
	unsigned int move_n;

	unsigned int empty;	/* remaining empty positions */
	unsigned int steps;	/* steps taken to solve */
	unsigned int solutions;	/* number of possible solution */
};

#endif /*  _SUDOKU_H_ */
