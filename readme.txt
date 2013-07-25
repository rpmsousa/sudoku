Sudoku solver

Track, for each board position, all candidate values.
Track, for each value, all possible positions.

At each move check if there is still a solution otherwise backtrack.
Chose move position based on:

Position with least possible candidates

or

Value with least possible positions in a row, column or square

Each move keeps track of the board state and lookup tables


TODO:
Improve next move choice algorithm so that:
- The number of steps taken to solve a puzzle is reduced.
- Solver is stable relative to board changes that lead to identical puzzles
(like re-mapping all values, rotations, horizontal/vertical columns shifts, ...)

- Don't stop on first solution and check how many solutions exist.


TODO TODO:
Add ability to create puzzles and measure their difficulty.
One idea is to use genetic algorithms to create puzzles
(Not sure if mathematical results for sudoku already have recipes to create
hard puzzles...)
