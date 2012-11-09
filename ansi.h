/* screen size */
#define COLUMNS	80
#define ROWS	25

/* ANSI colors */
#define BLACK	30
#define RED		31
#define GREEN	32
#define BROWN	33
#define BLUE	34
#define PURPLE	35
#define CYAN	36
#define GREY	37

/* ANSI attributes */
#define NORMAL	0
#define BRIGHT	1
#define BLINK	5

/* use octal for ANSI sequences 
 * for terminals that support it */
#define TL		'+'
#define TR		'+'
#define BL		'+'
#define BR		'+'
#define VLINE	'|'
#define HLINE	'-'

/* Function prototypes */
void fcolor (int, int);

void bcolor (int);

void cls ();

void pos (int, int);

void bar (int);

void box (int, int, int, int, int, int, int, char);
