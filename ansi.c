/*
   $Id: ansi.c,v 1.1 2005/08/20 17:34:21 dingo Exp $
   */
#include <main.h>
#include <ansi.h>

/* change foreground color */
void fcolor (int color, int attr) { printf ("\33[%d;%dm", attr, color); }

/* change background color */
void bcolor (int color) { printf ("\33[%dm", color+10); }

/* clear screen */
void cls () { puts ("\33[H\33[J"); fflush(stdout); }

/* move cursor position */
void pos (int x, int y) {
	/* move the cursor to position x, y */
	if(x <= 0 || x > COLUMNS+1 || y <= 0 || y >= ROWS)
		/* bad coordinate */
		printf ("!%i,%i", x, y);
	else
		printf ("\33[%d;%dH", y, x);
	fflush(stdout);
	return;
}

/* draw color bar on ROW y */
void bar (int y) {
	register int x;
	pos(1, y);
	for(x=0; x < 80; x++)
		putchar(' ');
	fflush(stdout);
}

/* draw box
 * location: COLUMN x, ROW y, width w, height h
 * attributes: foregroundcolor fc, backgroundcolor bc, attribute at
 * insides: (char) fill
 */
void box (int x, int y, int w, int h, int fc, int bc, int at, char fill) {
	int col, row;
	fcolor (fc, at); bcolor(bc);
	for(col = x; col < x + w; col++)
		for (row = y; row < y + h; row++) {
			pos (col, row);
			if (col == x || col == x+w-1) {
				if (row == y && col == x)
					putchar (TL);
				else if (row == y+h-1 && col == x)
					putchar (BL);
				else if (row == y)
					putchar (TR);
				else if (row == y+h-1)
					putchar (BR);
				else
					putchar (VLINE);
			} else if (row == y || row == y+h-1)
				putchar (HLINE);
			else
				putchar (fill);
		}
	fcolor (GREY, NORMAL); bcolor(BLACK);
}
