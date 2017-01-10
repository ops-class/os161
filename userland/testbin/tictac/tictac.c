/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * These are some constants we use in our program.
 * EMPTY is used to indicate empty spaces in the board.
 * X_MARKER and O_MARKER are used to indicate where each
 *	players has moved
 * DIM indicates the size of the board.  For now
 * we assume a conventional 3x3 playing board.
 *
 * This should work once the basic system calls are complete.
 */

#include <stdio.h>
#include <unistd.h>

#define NEWLINE 012
#define	EMPTY		0
#define X_PLAYER	1
#define	O_PLAYER	2
#define X_MARKER	1
#define	O_MARKER	2
#define DIM		3
#define	DIMCHAR		"2"
#define MAXSTRING	100

typedef enum { FALSE, TRUE } bool;

/* Function Declarations */
bool ask_yesno(const char *msg);
bool do_move(int player);
void initialize_board(void);
bool is_win(int x, int y);
int  read_string(char *buf, int length);
void print_board(void);
void print_instructions(void);
bool win_column(int y, int marker);
bool win_diag_left(int x, int y, int marker);
bool win_diag_right(int x, int y, int marker);
bool win_row(int x, int marker);
bool Strcmp(const char *a, const char *b);


/*
 * The board is gloabally defined.
 */
int board[DIM][DIM];

/* Console I/O routines */

int
main(void)
{
	bool win = FALSE;
	int move, max_moves;
	int player;

	print_instructions();
	max_moves = DIM * DIM;	/* Maximum number of moves in a game */

	while (TRUE) {
		initialize_board();
		for (move = 1; move <= max_moves; move++) {
			player = move % 2 == 0 ? 2 : 1;
			win = do_move(player);
			print_board();
			if (win) {
				printf("Player %d, you WON!\n\n", player);
				break;		/* out of for loop */
			}
		}
		/*
		 * If we got here by falling through the loop, then it is a
		 * tie game.
		 */
		if (!win)
			printf("Tie Game!\n\n");
		if (!ask_yesno("Do you wish to play again?"))
			break;			/* out of while loop */
	}
	return 0;
}

/*
 * print_instructions
 * Displays the instructions for the game.
 * Input
 *	None
 * Output
 *	None
 * Error
 *	None
 */
void
print_instructions(void)
{
	printf("Welcome to tic-tac-toe!\n");
	printf("Player 1 always plays X and player 2 always play O\n");
	printf("Good luck!\n\n\n");
}

void
/*
 * print_board
 * Display the DIM by DIM board.
 * Input
 *	None.
 * Output
 *	None.
 * Errors
 *	None.
 */
print_board(void)
{
	int i, j;

	/* Print labels across the top */
	printf("\n    0  1  2\n");

	for (i = 0; i < DIM; i++) {
		/* Print row labels */
		printf(" %d ", i);
		for (j = 0; j < DIM; j++) {
			switch (board[i][j]) {
				case EMPTY: printf("   "); break;
				case X_MARKER: printf(" X "); break;
				case O_MARKER: printf(" O "); break;
				default: printf("???"); break;
			}
		}
		printf("\n");
	}
	printf("\n");
}

/*
 * ask_yesno (taken from histo.c)
 * This function prints out the message and asks the user to respond
 * with either a yes or a no.  It continues asking the questions until
 * a valid reply is encountered and returns TRUE/FALSE indicating
 * the answer (True for yes, false for no).
 *
 * Input
 *	Question to be asked.
 * Output
 *	TRUE if response is yes
 *	FALSE if response is no
 * Error
 *	None
 */
bool
ask_yesno(const char *msg)
{
	char answer[MAXSTRING];

	while (TRUE) {
		printf("%s [yes/no] ", msg);
		if (read_string(answer, MAXSTRING) < 0)
			return(FALSE);
		if (Strcmp(answer, "yes"))
			return(TRUE);
		else if (Strcmp(answer, "no"))
			return(FALSE);
		else
			printf("Please answer either yes or no\n");
	}
}

/*
 * do_move
 * Processes one player move.  The player enters a row and column
 * and we have to determine if the square is valid (on the board)
 * and that there is no mark already there.  We continue to ask
 * for row/column pairs until we receive a valid combination.
 * Then we mark the board, check for a win, and return.
 *
 * Input
 *	player		Indicates which player (1 or 2) is moving
 * Output
 *	TRUE if this move won the game.
 *	FALSE if this move did not win the game.
 * Error
 *	None
 */
bool
do_move(int player)
{
	int x, y;
	char answer[MAXSTRING];
	char cx;

	printf("Player %d (%c), your move\n", player,
	       player == X_PLAYER ? 'X' : 'O');

	while (TRUE) {
		printf("Which row [0-%d]: ", DIM-1);
		if (read_string(answer, MAXSTRING) < 0)
			return(FALSE);
		cx = answer[0];
		x = cx - '0';
		if (x < 0 || x >= DIM) {
			printf("Invalid row; must be >= 0 and < %d\n", DIM-1);
			continue;
		}
		printf("Which column [0-%d]: ", DIM-1);
		if (read_string(answer, MAXSTRING) < 0)
			return(FALSE);
		cx = answer[0];
		y = cx - '0';
		if (y < 0 || y >= DIM) {
			printf("Invalid column; must be >= 0 and < %d\n",
				DIM-1);
			continue;
		}

		if (board[x][y] != EMPTY) {
			printf("That location is occupied; please try again\n");
			print_board();
		} else
			break;
	}
	board[x][y] = player == X_PLAYER ? X_MARKER : O_MARKER;

	return(is_win(x, y));

}

/*
 * is_win
 * Checks if the move into position x, y created a tic-tac-toe.
 * There are four possible ways to win -- horizontal, vertical,
 * and the two diagonals; check each one using a separate routine.
 *
 * Four routines for checking the wins are:
 * win_row	The horizontal spots are all the same as this current mark.
 * win_column	The vertical spots are all the same as this current mark.
 * win_diag_left	The diagonal spots from left to right are all the
 * 			same as the current mark.
 * win_diag_right	The diagonal spots from right to left are all the
 * 			same as the current mark.
 *
 * Input (for all routines)
 *	x x coordinate
 *	y y coordinate
 *	marker the value just placed on the board.
 * Output
 *	TRUE if player won
 *	FALSE otherwise
 */
bool
is_win(int x, int y)
{
	int marker;

	marker = board[x][y];

	/*
	 * Note that C "short circuit evaluation".  As soon as any one
	 * of these functions returns TRUE, we know that the expression
	 * is true.  Therefore, we can return TRUE without executing
	 * any of the other routines.
	 */
	return(win_row(x, marker) || win_column(y, marker) ||
	    win_diag_left(x, y, marker) || win_diag_right(x, y, marker));
}

/*
 * Four helper functions for determining a win.
 */
bool
win_column(int y, int marker)
{
	int i;
	for (i = 0; i < DIM; i++)
		if (board[i][y] != marker)
			return(FALSE);
	return(TRUE);
}

bool
win_row(int x, int marker)
{
	int i;
	for (i = 0; i < DIM; i++)
		if (board[x][i] != marker)
			return(FALSE);
	return(TRUE);
}

bool
win_diag_left(int x, int y, int marker)
{
	int i;

	/* Check that move is on the diagonal */
	if (x != y)
		return(FALSE);

	for (i = 0; i < DIM; i++)
		if (board[i][i] != marker)
			return(FALSE);
	return(TRUE);
}

bool
win_diag_right(int x, int y, int marker)
{
	int i;

	/* Check that move is on the diagonal */
	if (x + y != DIM - 1)
		return(FALSE);
	for (i = 0; i < DIM; i++)
		if (board[i][DIM - 1 - i] != marker)
			return(FALSE);
	return(TRUE);
}

void
initialize_board(void)
{
	int i, j;

	for (i = 0; i < DIM; i++)
		for (j = 0; j < DIM; j++)
			board[i][j] = EMPTY;
}

int
read_string(char *buf, int length)
{
	int	char_read;
	int	i;

	i = 0;
	while ((char_read = getchar()) != EOF && char_read != NEWLINE &&
	    i < length) {
		buf[i] = (char) char_read;
		i++;
		putchar(char_read);
	}

	if (char_read == EOF)
		return(-1);

	/*
	 * If the input overflows the buffer, just cut it short
	 * at length - 1 characters.
	 */
	if (i >= length)
		i--;
	buf[i] = 0;
	return(i);
}

bool
Strcmp(const char *a, const char *b)
{
	if (a == NULL)
		return(b == NULL);
	if (b == NULL)
		return(FALSE);

	while (*a && *b)
		if (*a++ != *b++)
			return(FALSE);

	return(*a == *b);

}
