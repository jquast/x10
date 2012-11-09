/* Number of lights */
#define LIGHTS 5
#define ON 1
#define OFF 0
/* Temporary structure for editing fields */
struct ltype_str {
		char name[13];
		char housecode[3];
		char devicecode[4];
		char ontime[8];
		char offtime[8];
		char decay[7];
};

/* structure for light data */
struct ltype {
		char name[13];
		char housecode;
		char devicecode;
		struct tm ontime;
		struct tm offtime;
		float decay;
		int state;
		int retry;
} light[LIGHTS];

/* function prototypes */
int light_on (int);

int light_off (int);

int light_check_on (int);

int light_check_off (int);

int lights_check ();

void decay(int);

void lights_clear (int);

int light_duration (int);

void lights_view (int);

struct ltype_str light_2str (struct ltype);

struct ltype str2_light (struct ltype_str);
