/*
 * Sun clock definitions.
 */

#define XK_MISCELLANY
#define XK_LATIN1

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysymdef.h>
#include <X11/keysym.h>
#include <X11/xpm.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include "version.h"

#define MENU_WIDTH 38
#define SEL_WIDTH 28
#define SEL_HEIGHT 10
#define MAXSHORT 32767

#define abs(x) ((x) < 0 ? (-(x)) : x)			  /* Absolute value */
#define sgn(x) (((x) < 0) ? -1 : ((x) > 0 ? 1 : 0))	  /* Extract sign */
#define dtr(x) ((x) * (PI / 180.0))			  /* Degree->Radian */
#define rtd(x) ((x) / (PI / 180.0))			  /* Radian->Degree */
#define fixangle(a) ((a) - 360.0 * (floor((a) / 360.0)))  /* Fix angle	  */

#define PI 3.14159265358979323846
#define ZFACT 1.2

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
#define TIMESTEP  10000

enum {RANDOM=0, FIXED, CENTER, NW, NE, SW, SE};

/* Geometry structure */

typedef struct Geometry {
	int	mask;
	int	x;
	int	y;
	unsigned int	width;
	unsigned int	height;
        unsigned int    w_mini;
        unsigned int    h_mini;
} Geometry;

typedef struct Flags {
  /* Status flags */
        short mono;                     /* 0=color 1=invert 2=B&W */
        short fillmode;                 /* 0=coastlines 1=contour 2=landfill */
        short dotted;                   /* use dotted lines ? */
        short spotsize;                 /* size of spots repr. cities */
        short colorscale;               /* number of colors in shading */
        short darkness;                 /* level of darkness in shading */
        short map_mode;                 /* are we in C, D, E, L, S mode? */
        short clock_mode;               /* clock mode */
        short progress;                 /* special progress time ?*/
        short shading;                  /* shading mode */
        short dms;                      /* degree, minute, second mode */
        short sunpos;                   /* is Sun to be shown ? */
        short cities;                   /* are cities to be shown ? */
        short meridian;                 /* are meridian to be shown ? */
        short parallel;                 /* are parallel to be shown ? */
        short tropics;                  /* are tropics to be shown ? */
  /* Internal switches */
        short update;                   /* update image (=-1 full update) */
        short firsttime;                /* is it first window mapping ? */
        short bottom;                   /* bottom strip to be cleaned */
        short hours_shown;              /* hours in extension mode shown? */
} Flags;

typedef struct ZoomSettings {
        double          fx;             /* zoom factor along width */
        double          fy;             /* zoom factor along height */
        double          fdx;            /* translation factor along width */
        double          fdy;            /* translation factor along height */
        int             mode;           /* zoom behaviour mode=0,1,2 */
        int             width;          /* width of full extent zoomed area */
        int             height;         /* height of full extent zoomed area */
        int             dx;             /* translation along width */
        int             dy;             /* translation along height */
} ZoomSettings;

typedef struct GClist {
        GC mapstore;
        GC mapinvert;
        GC menufont;
        GC clockfont;
        GC optionfont;
        GC dirfont;
        GC imagefont;
        GC choice;
        GC change;
        GC zoombg;
        GC zoomfg;
        GC citycolor0;
        GC citycolor1;
        GC citycolor2;
        GC markcolor1;
        GC markcolor2;
        GC linecolor;
        GC meridiancolor;
        GC parallelcolor;
        GC tropiccolor;
        GC suncolor;
        GC mooncolor;
} GClist;

typedef struct Pixlist {
        Pixel black;
        Pixel white;
        Pixel textbgcolor;
        Pixel textfgcolor;
        Pixel mapbgcolor;
        Pixel mapfgcolor;
        Pixel zoombgcolor;
        Pixel zoomfgcolor;
        Pixel optionbgcolor;
        Pixel optionfgcolor;
        Pixel caretcolor;
        Pixel dircolor;
        Pixel imagecolor;
        Pixel changecolor;
        Pixel choicecolor;
        Pixel citycolor0;
        Pixel citycolor1;
        Pixel citycolor2;
        Pixel markcolor1;
        Pixel markcolor2;
        Pixel linecolor;
        Pixel meridiancolor;
        Pixel parallelcolor;
        Pixel tropiccolor;
        Pixel suncolor;
        Pixel mooncolor;
} Pixlist;

typedef struct GraphicData {
        Colormap        cmap;           /* window (private?) colormap */  
        short links;                    /* how many other Windows linked ? */
        short usedcolors;               /* number of colors used */
        int   mapstrip;                 /* height of strip for map */
        int   clockstrip;               /* height of strip for clock */
        int   charspace;                /* menu char spacing */
        int   precedence;               /* ordinal number of creation */
        XFontStruct *   menufont;       /* menu font structure */
        XFontStruct *   clockfont;      /* clock font structure */
        GClist          gclist;         /* window GCs */  
        Pixlist         pixlist;        /* special color pixels */  
} GraphicData;

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

/* Sundata structure */
typedef struct Sundata {
        Window          win;            /* window id */
        GraphicData *   gdata;          /* associated graphical data */
        int             wintype;        /* is window map or clock ? */
        int             hstrip;         /* height of bottom strip */
        Geometry        geom;           /* geometry */
	Geometry        prevgeom;       /* previous geometry */
        ZoomSettings    zoom;           /* Zoom settings of window */
        ZoomSettings    newzoom;        /* New zoom settings */
	Flags		flags;		/* window behavioral flags */
        char *          clock_img_file; /* name of clock xpm file */
        char *          map_img_file;   /* name of map xpm file */
	char *		bits;           /* pointer to char * bitmap bits */
        short *         tr1;            /* pointer to day/night transition 1 */
        short *         tr2;            /* pointer to day/night transition 2 */
        int             south;          /* color code (0 / -1) at South pole */
        double *        wave;           /* pointer to sine, cosine values */
        Pixmap          pix;            /* pixmap */
        XImage *        xim;            /* ximage of map */ 
        char *          ximdata;        /* ximage data copy*/ 
        unsigned char * daypixel;       /* pointer to day pixels */
        unsigned char * nightpixel;     /* pointer to night pixels */
        int             ncolors;        /* number of colors in day pixels */
	long		time;		/* time - real or fake, see flags */
	long		projtime;	/* last time projected illumination */
        long            jump;           /* time jump (in sec) */
        double          sundec;         /* Sun declination */
        double          sunlon;         /* Sun longitude */
        double          shadefactor;    /* shading factor */
        double          shadescale;     /* shading rescale factor */
        double          fnoon;          /* position of noon, double float */
	int		noon;		/* position of noon, integer */
        int             local_day;      /* previous local day */
        int             solar_day;      /* previous solar day */
	int		count;	        /* number of time iterations */
        struct City     pos1;           /* first position */
        struct City     pos2;           /* second position */
        struct Mark     mark1;          /* first mark */
        struct Mark     mark2;          /* second mark */
        struct Sundata *next;           /* pointer to next structure */
} Sundata;

/* Which OS are we using ? */

#if defined(linux) || defined(__linux) || defined(__linux__)
#define _OS_LINUX_
#elif defined(hpux) || defined(__hpux) || defined(__hpux__)
#define _OS_HPUX_
#endif
