/* hour */
struct {
		int hour;
		int yday;
} lastsaved;

/* Function prototypes */
int save_check (int);

int save_data (void);

int load_data (void);

int load_buffer (void);

int save_buffer (void);

int save_buffertxt (void);

int load_lights (void);

int save_lights (void);

int load_pump (void);

int save_pump (void);
