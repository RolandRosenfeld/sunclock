/*
 * Sun clock definitions.
 */

#define XK_MISCELLANY
#define XK_LATIN1

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysymdef.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include "version.h"

#define MENU_WIDTH 38
#define SEL_WIDTH 32
#define SEL_HEIGHT 10

#define abs(x) ((x) < 0 ? (-(x)) : x)			  /* Absolute value */
#define sgn(x) (((x) < 0) ? -1 : ((x) > 0 ? 1 : 0))	  /* Extract sign */
#define dtr(x) ((x) * (PI / 180.0))			  /* Degree->Radian */
#define rtd(x) ((x) / (PI / 180.0))			  /* Radian->Degree */
#define fixangle(a) ((a) - 360.0 * (floor((a) / 360.0)))  /* Fix angle	  */

#define PI 3.14159265358979323846

#define COLORSTEPS  128            /* maximum number of colors for shading */
#define COLORLENGTH 48             /* maximum length of color names */

#define TERMINC  100		   /* Circle segments for terminator */

#define PROJINT  (60 * 10)	   /* Frequency of seasonal recalculation
				      in seconds. */

#define RECOVER         "Trying to recover.\n"

#define	FAILFONT	"fixed"
#define EARTHRADIUS_KM  6378.125
#define EARTHRADIUS_ML  3963.180
#define SUN_APPRADIUS   0.266      /* Sun apparent radius, in degrees */
#define ATM_REFRACTION  0.100      /* Atmospheric refraction, in degrees */
#define ATM_DIFFUSION   3.0        /* Atmospheric diffusion, in degrees */

#define SP		"         "

#define COORDINATES 'c'
#define DISTANCES 'd'
#define EXTENSION 'e'
#define LEGALTIME 'l'
#define SOLARTIME 's'

#define TIMECOUNT 25
#define PRECOUNT  TIMECOUNT-1
#define TIMESTEP  10000

enum {RANDOM=0, FIXED, CENTER, NW, NE, SW, SE};

/* sunmap structure */
struct earthmap {
	int		width;		/* width of pixmap */
	int		height;         /* height of pixmap */
	char **		data;           /* pointer to char ** pixmap data */
	char *		bits;           /* pointer to char * bitmap bits */
	int		flags;		/* see below */
	int		noon;		/* position of noon */
	short *		nwtab;		/* current width table (?) */
	short *		owtab;		/* previous width table (?) */
	long		increm;		/* increment for fake time */
	long		time;		/* time - real or fake, see flags */
	GC		gc;		/* GC for writing text into window */
	char		text[128];	/* and the current text that's there */
	int		textx;		/* where to draw the text */
	int		texty;		/* where to draw the text */
	long		projtime;	/* last time we projected illumination */
	int		timeout;	/* time until next image update */
};

/* Color record */
typedef struct Color {
	char		name[COLORLENGTH+1];
	unsigned long	pix;
	GC		gc;
} Color;

/* Records to hold cities */

typedef struct City {
    char *name;		/* Name of the city */
    double lat, lon;	/* Latitude and longitude of city */
    char *tz;		/* Timezone of city */
    int mode;
    struct City *next;	/* Pointer to next record */
} City;

/* Records to hold marks */
typedef struct Mark {
    City *city;
    double save_lat, save_lon;
    int  status, pulse, full;
    struct tm sr, ss, dl;
} Mark;

/* Geometry structure */

typedef struct geom {
	int	mask;
	int	x;
	int	y;
	unsigned int	width;
	unsigned int	height;
        unsigned int     w_mini;
        unsigned int     h_mini;
        int     strip;
} geom;

