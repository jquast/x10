/* buffer window */
#define BUF_Y 2
#define BUF_HEIGHT 15

/* spaces between light switches */
#define BUTTONPADD 14

/* pump diagram area */
#define PUMP_Y 18
#define PUMP_X 65
#define PUMP_WIDTH 13

/* Function prototypes */
void reset_stdin();

struct termios set_stdin();

void refresh_main_major ();

void refresh_main_minor (char *);

int field_input (int, int, void *, int);

void program_light (int);

void program_pump ();

void draw_buf ();
void popbox(char *, int);
