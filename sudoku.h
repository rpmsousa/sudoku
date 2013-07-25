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


/* list of possible positions for a given digit */

struct pos_digit {
	unsigned char pos[MAX_POS];

	int row_n[MAX_ROWS];
	int column_n[MAX_COLUMNS];
	int square_n[MAX_SQUARES];

	int row[MAX_ROWS];
	int column[MAX_COLUMNS];
	int square[MAX_SQUARES];
};

/* list of possible candidates for a given position */

struct candidate_list {
	unsigned short list;	/* Bitmask of possible candidates */
	unsigned int n;		/* Total number of possible candidates */
};

struct move {
	int x;
	int y;
	int val;	/* current value */
	int tried;	/* Bitmask of already tried values */
	int type;	/* Type of current move */

	int row_pos;
	int column_pos;
	int square_pos;
	int xy_candidate;
};

struct board {
	unsigned char value[MAX_POS];

	/* for each position track which digits are still possible */
	struct candidate_list candidates[MAX_POS];

	/* for each digit track possible positions in each column */
	struct pos_digit plist[MAX_DIGITS];

	struct move move_list[MAX_MOVES];
	struct move *cur_move;

	unsigned int empty;
};

#endif /*  _SUDOKU_H_ */
