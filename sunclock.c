/*****************************************************************************
 *
 * Sun clock version 3.xx by Jean-Pierre Demailly
 *
 * Is derived from the previous versions whose notices appear below.
 * See CHANGES for more explanation on the (quite numerous changes and
 * improvements). Version 3.xx is now published under the GPL.
 */

/*****************************************************************************
 *
 * Sun clock.  X11 version by John Mackin.
 *
 * This program was derived from, and is still in part identical with, the
 * Suntools Sun clock program whose author's comment appears immediately
 * below.  Please preserve both notices.
 *
 * The X11R3/4 version of this program was written by John Mackin, at the
 * Basser Department of Computer Science, University of Sydney, Sydney,
 * New South Wales, Australia; <john@cs.su.oz.AU>.  This program, like
 * the one it was derived from, is in the public domain: `Love is the
 * law, love under will.'
 */

/*****************************************************************************

	Sun clock

	Designed and implemented by John Walker in November of 1988.

	Version for the Sun Workstation.

    The algorithm used to calculate the position of the Sun is given in
    Chapter 18 of:

    "Astronomical  Formulae for Calculators" by Jean Meeus, Third Edition,
    Richmond: Willmann-Bell, 1985.  This book can be obtained from:

       Willmann-Bell
       P.O. Box 35025
       Richmond, VA  23235
       USA
       Phone: (804) 320-7016

    This program was written by:

       John Walker
       Autodesk, Inc.
       2320 Marinship Way
       Sausalito, CA  94965
       USA
       Fax:   (415) 389-9418
       Voice: (415) 332-2344 Ext. 2829
       Usenet: {sun,well,uunet}!acad!kelvin
	   or: kelvin@acad.uu.net

    modified for interactive maps by

	Stephen Martin
	Fujitsu Systems Business of Canada
	smartin@fujitsu.ca

    This  program is in the public domain: "Do what thou wilt shall be the
    whole of the law".  I'd appreciate  receiving  any  bug  fixes  and/or
    enhancements,  which  I'll  incorporate  in  future  versions  of  the
    program.  Please leave the original attribution information intact	so
    that credit and blame may be properly apportioned.

    Revision history:

	1.0  12/21/89  Initial version.
	      8/24/89  Finally got around to submitting.

	1.1   8/31/94  Version with interactive map.
	1.2  10/12/94  Fixes for HP and Solaris, new icon bitmap
	1.3  11/01/94  Timezone now shown in icon

    next minor revisions by Jean-Pierre Demailly (demailly@ujf-grenoble.fr)
	1.4  04/03/99  Change in city management
	1.5  15/03/99  Color support
	1.6  28/03/99  Iconic stuff fixed
	1.7  17/08/99  Calculation of city distances
	1.8  20/08/99  Fixed small bug in mono mode
	1.9  21/01/00  Iconic stuff fixed again (didn't work for all WMHints!)
	2.0  21/04/00  Added XFlush in doTimeout to force update
        2.1  06/06/00  Updated latlong.txt with a better source 

    major rewrite of GUI (Jean-Pierre Demailly, demailly@ujf-grenoble.fr)
       3.00  08/26/00  Many bug corrections, GUI improvements (keyboard and
                       mouse controls), new functions, resulting in a much 
                       more powerful program.

       3.01  09/01/00  Additional command line improvements and a lot of
       		       compilation fixes.

*/

#include <unistd.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <string.h>

#include "sunclock.h"
#include "langdef.h"

/* 
 *  external routines
 */

extern long	time();
#ifdef NEW_CTIME
extern char *	timezone();
#endif

extern double	jtime();
extern double	gmst();
extern void	sunpos();

struct sunclock {
	int		s_width;	/* size of pixmap */
	int		s_height;
	Window		s_window;	/* associated window */
	Pixmap		s_pixmap;	/* and pixmap */
	int		s_flags;	/* see below */
	int		s_noon;		/* position of noon */
	short *		s_wtab1;	/* current width table (?) */
	short *		s_wtab;		/* previous width table (?) */
	long		s_increm;	/* increment for fake time */
	long		s_time;		/* time - real or fake, see flags */
	GC		s_gc;		/* GC for writing text into window */
	char *		(*s_tfunc)();	/* function to return the text */
	char		s_text[128];	/* and the current text that's there */
	int		s_textx;	/* where to draw the text */
	int		s_texty;	/* where to draw the text */
	long		s_projtime;	/* last time we projected illumination */
	int		s_timeout;	/* time until next image update */
	struct sunclock * s_next;	/* pointer to next clock context */
};

/* Color record */
typedef struct Color {
	char		name[50];
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

/*
 * bits in s_flags
 */

#define	S_FAKE		01	/* date is fake, don't use actual time */
#define	S_ANIMATE	02	/* do animation based on increment */
#define	S_DIRTY		04	/* pixmap -> window copy required */
#define	S_CLOCK		010	/* this is the clock window */

struct sunclock *	makeMapContext();
Bool			evpred();

char app_default[] = APPDEF"/Sunclock***";

struct geom {
	int	mask;
	int	x;
	int	y;
};

char *	ProgName;
char *  Command = NULL;

Color	BgColor, FgColor, TextBgColor, TextFgColor,
	CityColor0, CityColor1, CityColor2,
	MarkColor1, MarkColor2, LineColor, TropicColor;

char *		Display_name = "";
Display *	dpy;
int		scr;

XFontStruct *	SmallFont;
XFontStruct *	BigFont;

GC		Store_gc;
GC		Invert_gc;
GC		BigFont_gc;
GC		SmallFont_gc;

Window		Clock;
Window		Map;
Pixmap		Clockpix;
Pixmap		Mappix;
struct geom	ClockGeom = { 0, 20, 20 };
struct geom	MapGeom = { 0, 30, 30 };

struct sunclock *	Current;

int             spot_size = 2;
int		placement = -1;
int             mono = 0;
int		map_height;
int		clock_height;
int		label_shift = 0;

int             time_count = PRECOUNT;
int		force_proj = 0;
int		click_pos;

int             do_map = 0;
int		do_help = 0;
int		do_parall = 0;
int		do_tropics = 0;
int		do_merid = 0;
int		do_nocities = 0;

int             progress_mode = 0;
int		clock_mode = 0;
char            map_mode = LEGALTIME;
char *		CityInit = NULL;

long		local_shift = 0;
long		global_shift = 0;
long		time_progress = 1;

/* Records to hold extra marks 1 and 2 */

City pos1, pos2, *cities = NULL;
Mark mark1, mark2;

void
usage()
{
  	int	i;

	fprintf(stderr, "%s: version %s\nUsage:\n"
         "%s [-display dispname] [-cmd command]\n"
         SP"[-version] [-help] [-mono]\n"
	 SP"[-clock] [-clockgeom +x+y] [-seconds]\n"
         SP"[-map] [-mapgeom +x+y] [-mapmode * <c,d,l,s>]\n"
         SP"[-placement (random, fixed, center, NW, NE, SW, SE)]\n"
         SP"[-spot size(0,1,2,3)] [-shift timeshift(sec)]\n"
         SP"[-city name] [-position latitude longitude]\n"
         SP"[-meridians] [-parallels] [-tropics] [-nocities]\n"
         SP"[-bg color] [-fg color] [-textbg color] [-textfg color]\n"
         SP"[-citycolor0 color] [-citycolor1 color] [-citycolor2 color]\n"
         SP"[-markcolor1 color] [-markcolor2 color]\n"
         SP"[-linecolor color] [-tropiccolor color]"
           "\n\n%s\n", 
         ProgName, VERSION, ProgName, ShortHelp);

	for (i=0; i<N_OPTIONS; i++)
	fprintf(stderr, "%s %c : %s\n", Label[L_KEY], Option[2*i], Help[i]);
        fprintf(stderr, "\n");
	exit(1);
}

void
initValues()
{
	strcpy(BgColor.name, "White");
	strcpy(FgColor.name, "Black");
	strcpy(TextBgColor.name, "Thistle1");
	strcpy(TextFgColor.name, "Black");
	strcpy(CityColor0.name, "Orange");
	strcpy(CityColor1.name, "Red");
	strcpy(CityColor2.name, "Red3");
	strcpy(MarkColor1.name, "Green");
	strcpy(MarkColor2.name, "Green3");
	strcpy(LineColor.name, "Blue");
	strcpy(TropicColor.name, "Purple");
	mark1.city = NULL;
	mark1.status = 0;
	mark2.city = NULL;
	mark2.status = 0;
}

char *
salloc(nbytes)
register int			nbytes;
{
	register char *		p;

	p = malloc((unsigned)nbytes);
	if (p == (char *)NULL) {
		fprintf(stderr, "%s: out of memory\n", ProgName);
		exit(1);
	}
	return (p);
}

/*
 * readrc() - Read the user's ~/.sunclockrc file and app-defaults
 */

int 
readrc()
{
    /*
     * Local Variables
     */

    char *fname;	/* Path to .sunclockrc file */
    FILE *rc;		/* File pointer for rc file */
    char buf[128];	/* Buffer to hold input lines */
    char language[4];   /* String to hold language identifier */
    char *city, *lat, *lon, *tz; /* Information about a place */
    City *crec;		/* Pointer to new city record */
    int  i;		/* index */

    /*
     * External Functions
     */

    char *tildepath();	/* Returns path to ~/<file> */

    /*
     * Initialize ListOptions
     */

    for (i=0; i<N_OPTIONS; i++) {
      ListOptions[4*i+1] = Option[2*i];
      ListOptions[4*i] = ListOptions[4*i+2] = ListOptions[4*i+3] = ' ';
    }
    ListOptions[4*N_OPTIONS] = '\0';

    /*
     * Read language files
     */

    fname = app_default;

    i = strlen(app_default);
    app_default[i-3] = '_';

    if (getenv("LANG")) {
       strncpy(language, getenv("LANG"), 2);
    } else {
       strcpy (language,"en");
    }
    app_default[i-2] = language[0];
    app_default[i-1] = language[1];

    if ((rc = fopen(fname, "r")) != NULL) {
      int j=0, k=0, l=0, m=0, n=0, p;
      while (fgets(buf, 128, rc)) {
        if (buf[0] != '#') {
	        p = strlen(buf)-1;
        	if (p>=0 && buf[p]=='\n') buf[p] = '\0';
	        if (j<7) { strcpy(Day_name[j], buf); j++; } else
        	if (k<12) { strcpy(Month_name[k], buf); k++; } else
		if (l<L_END) { strcpy(Label[l], buf); l++; } else 
		if (m<N_OPTIONS) { strcpy(Help[m], buf); m++; } else 
                {
                   if (n==0) *ShortHelp = '\0';
		   strcat(ShortHelp, buf); 
                   strcat(ShortHelp, "\n"); 
                   n++;
                }
	}
      }
      fclose(rc);
    } else
        fprintf(stderr, 
             "Unable to open language in %s\n", app_default);

    /*
     * Get the path to the rc file
     */

    app_default[i-3] = '\0';

    if ((fname = tildepath("~/.sunclockrc")) == NULL) {
        fprintf(stderr, 
             "Unable to get path to ~/.sunclockrc\n");
        return(1);
    }

    
    /* Open the RC file */

    if ((rc = fopen(fname, "r")) == NULL) {
        fname = app_default;
        if ((rc = fopen(fname, "r")) == NULL)
	  {
          fprintf(stderr, 
             "Unable to find ~/.sunclockrc or app-default Sunclock\n");
	  return(1);
	  }
    }

    /* Read and parse lines from the file */

    while (fgets(buf, 128, rc)) {

	/* Get the city name looking for blank lines and comments */

	if (((city = strtok(buf, ":\n")) == NULL) ||
	    (city[0] == '#') || (city[0] == '\0'))
	    continue;

	/* Get the latitude, longitude and timezone */

	if ((lat = strtok(NULL, " 	\n")) == NULL) {
	    fprintf(stderr, "Error in .sunclockrc for city %s\n", city);
	    continue;
	}

	if ((lon = strtok(NULL, " 	\n")) == NULL) {
	    fprintf(stderr, "Error in .sunclockrf for city %s\n", city);
	    continue;
	}

	if ((tz = strtok(NULL, " 	\n")) == NULL) {
	    fprintf(stderr, "Error in .sunclockrc for city %s\n", city);
	    continue;
	}

	/* Create the record for the city */

	if ((crec = (City *) calloc(1, sizeof(City))) == NULL) {
	    fprintf(stderr, "Memory allocation failure\n");
	    return(1);
	}

	/* Set up the record */

	crec->name = strdup(city);
	crec->lat = atof(lat);
	crec->lon = atof(lon);
	crec->mode = 0;
	crec->tz = strdup(tz);

	/* Link it into the list */

	crec->next = cities;
	cities = crec;
    }

    /* Done */

    return(0);
}

void
needMore(argc, argv)
register int			argc;
register char **		argv;
{
	if (argc == 1) {
		fprintf(stderr, "%s: option `%s' requires an argument\n",
			ProgName, *argv);
		usage();
	}
}

void
getGeom(s, g)
register char *			s;
register struct geom *		g;
{
	register int		mask;
	unsigned int		width;
	unsigned int		height;

	mask = XParseGeometry(s, &g->x, &g->y, &width, &height);
	if (mask == 0) {
		fprintf(stderr, "%s: `%s' is a bad geometry specification\n",
			ProgName, s);
		exit(1);
	}
	if ((mask & WidthValue) || (mask & HeightValue))
		fprintf(stderr, "%s: warning: width/height in geometry `%s' ignored\n",
			ProgName, s);
	g->mask = mask;
}

void
parseArgs(argc, argv)
register int			argc;
register char **		argv;
{
	while (--argc > 0) {
		++argv;
		if (strcmp(*argv, "-display") == 0 && argc>1) {
			needMore(argc, argv);
			Display_name = *++argv;
			--argc;
		}
		else if (strcmp(*argv, "-clockgeom") == 0 && argc>1) {
			needMore(argc, argv);
			getGeom(*++argv, &ClockGeom);
			if (placement <= RANDOM) placement = FIXED;
	                if (!MapGeom.mask && placement <= FIXED) 
                           getGeom(*argv, &MapGeom);
			--argc;
		}
		else if (strcmp(*argv, "-mapgeom") == 0 && argc>1) {
			needMore(argc, argv);
			placement = FIXED;
			getGeom(*++argv, &MapGeom);
			if (placement <= RANDOM) placement = FIXED;
	                if (!ClockGeom.mask && placement <= FIXED) 
	                   getGeom(*argv, &ClockGeom);
			--argc;
		}
		else if (strcmp(*argv, "-mapmode") == 0 && argc>1) {
			needMore(argc, argv);
                        if (!strcasecmp(*++argv, "c")) map_mode = COORDINATES;
                        if (!strcasecmp(*argv, "d")) map_mode = DISTANCES;
                        if (!strcasecmp(*argv, "l")) map_mode = LEGALTIME;
                        if (!strcasecmp(*argv, "s")) map_mode = SOLARTIME;
			--argc;
		}
		else if (strcmp(*argv, "-spot") == 0 && argc>1) {
			needMore(argc, argv);
			spot_size = atoi(*++argv);
                        if (spot_size<0) spot_size = 0;
                        if (spot_size>3) spot_size = 3;
                        --argc;
		}
		else if (strcmp(*argv, "-bg") == 0 && argc>1) {
			needMore(argc, argv);
			strcpy(BgColor.name, *++argv);
			--argc;
		}
		else if (strcmp(*argv, "-fg") == 0 && argc>1) {
			needMore(argc, argv);
			strcpy(FgColor.name, *++argv);
			--argc;
		}
		else if (strcmp(*argv, "-textbg") == 0 && argc>1) {
			needMore(argc, argv);
			strcpy(TextBgColor.name, *++argv);
			--argc;
		}
		else if (strcmp(*argv, "-textfg") == 0 && argc>1) {
			needMore(argc, argv);
			strcpy(TextFgColor.name, *++argv);
			--argc;
		}
		else if (strcmp(*argv, "-position") == 0 && argc>1) {
			needMore(argc, argv);
			pos1.lat = atof(*++argv);
	                --argc;
			needMore(argc, argv);
			pos1.lon = atof(*++argv);
			mark1.city = &pos1;
			--argc;
		}
		else if (strcmp(*argv, "-city") == 0 && argc>1) {
			needMore(argc, argv);
			CityInit = *++argv;
			--argc;
		}
		else if (strcmp(*argv, "-citycolor0") == 0 && argc>1) {
			needMore(argc, argv);
			strcpy(CityColor0.name, *++argv);
			--argc;
		}
		else if (strcmp(*argv, "-citycolor1") == 0 && argc>1) {
			needMore(argc, argv);
			strcpy(CityColor1.name, *++argv);
			--argc;
		}
		else if (strcmp(*argv, "-citycolor2") == 0 && argc>1) {
			needMore(argc, argv);
			strcpy(CityColor2.name, *++argv);
			--argc;
		}
		else if (strcmp(*argv, "-markcolor1") == 0 && argc>1) {
			needMore(argc, argv);
			strcpy(MarkColor1.name, *++argv);
			--argc;
		}
		else if (strcmp(*argv, "-markcolor2") == 0 && argc>1) {
			needMore(argc, argv);
			strcpy(MarkColor2.name, *++argv);
			--argc;
		}
		else if (strcmp(*argv, "-linecolor") == 0 && argc>1) {
			needMore(argc, argv);
			strcpy(LineColor.name, *++argv);
			--argc;
		}
		else if (strcmp(*argv, "-tropiccolor") == 0 && argc>1) {
			needMore(argc, argv);
			strcpy(TropicColor.name, *++argv);
			--argc;
		}
		else if (strcmp(*argv, "-placement") == 0 && argc>1) {
			needMore(argc, argv);
			if (strcmp(*++argv, "random")==0)
                           placement = RANDOM;
			if (strcmp(*argv, "fixed")==0) {
                           placement = FIXED;
	                   MapGeom.mask = XValue | YValue;
	                   ClockGeom.mask = XValue | YValue;
			}
			if (strcmp(*argv, "center")==0)
                           placement = CENTER;
			if (strcasecmp(*argv, "nw")==0)
                           placement = NW;
			if (strcasecmp(*argv, "ne")==0)
                           placement = NE;
			if (strcasecmp(*argv, "sw")==0)
                           placement = SW;
			if (strcasecmp(*argv, "se")==0)
                           placement = SE;
			--argc;
		}
		else if (strcmp(*argv, "-cmd") == 0 && argc>1) {
			needMore(argc, argv);
			Command = *++argv;
			--argc;
		}
		else if (strcmp(*argv, "-shift") == 0 && argc>1) {
			needMore(argc, argv);
			global_shift= atol(*++argv);
			--argc;
		}
		else if (strcmp(*argv, "-clock") == 0)
			do_map = 0;
		else if (strcmp(*argv, "-map") == 0)
			do_map = 1;
		else if (strcmp(*argv, "-mono") == 0) {
                        strcpy(TextBgColor.name, "White");
                        spot_size = 3;
			mono = 1;
		}
		else if (strcmp(*argv, "-seconds") == 0)
                        clock_mode = 1;
		else if (strcmp(*argv, "-parallels") == 0) {
			do_parall = 1;
		}
		else if (strcmp(*argv, "-meridians") == 0) {
			do_merid = 1;
		}
		else if (strcmp(*argv, "-tropics") == 0) {
			do_tropics = 1;
		}
		else if (strcmp(*argv, "-nocities") == 0) {
			do_nocities = spot_size;
			spot_size = 0;
		}
		else if (strcmp(*argv, "-version") == 0) {
			fprintf(stderr, "%s: version %s\n",
				ProgName, VERSION);
			exit(0);
		}
		else
			usage();
	}

	if (placement<0)
	  placement = NW;
}

/*
 * Free resources.
 */

void
shutDown()
{
	XFreeGC(dpy, Store_gc);
	XFreeGC(dpy, Invert_gc);
	XFreeGC(dpy, CityColor0.gc);
	XFreeGC(dpy, CityColor1.gc);
	XFreeGC(dpy, CityColor2.gc);
	XFreeGC(dpy, MarkColor1.gc);
	XFreeGC(dpy, MarkColor2.gc);
	XFreeGC(dpy, LineColor.gc);
	XFreeGC(dpy, TropicColor.gc);
	XFreeGC(dpy, BigFont_gc);
	XFreeGC(dpy, SmallFont_gc);
	XFreeFont(dpy, BigFont);
	XFreeFont(dpy, SmallFont);
	XFreePixmap(dpy, Mappix);
	XFreePixmap(dpy, Clockpix);
	XDestroyWindow(dpy, Map);
	XDestroyWindow(dpy, Clock);
	XCloseDisplay(dpy);
}


/*
 * Set up stuff the window manager will want to know.  Must be done
 * before mapping window, but after creating it.
 */

void
setAllHints(argc, argv, num)
int				argc;
char **				argv;
int				num;
{
	XClassHint		xch;
	XSizeHints		xsh;
	Window			win = 0;
        char 			name[80];	/* Used to change icon name */

	xsh.flags = PSize | PMinSize | PMaxSize;

        switch(num) {
	  case 0:
		win = Clock;
		if (ClockGeom.mask & (XValue | YValue)) {
			xsh.x = ClockGeom.x;
			xsh.y = ClockGeom.y;
			xsh.flags |= USPosition;
		}
		xsh.width = xsh.min_width = xsh.max_width = clock_icon_width;
		xsh.height = xsh.min_height = xsh.max_height = clock_height;
		break;

	   case 1:
		win = Map;
		if (MapGeom.mask & (XValue | YValue)) {
			xsh.x = MapGeom.x;
			xsh.y = MapGeom.y;
			xsh.flags |= USPosition;
		}
		xsh.width = xsh.min_width = xsh.max_width = map_icon_width;
		xsh.height = xsh.min_height = xsh.max_height = map_height;
		break;
	}

	if (!win) return;

	XSetNormalHints(dpy, win, &xsh);

        sprintf(name, "%s %s", ProgName, VERSION);
	xch.res_name = ProgName;
	xch.res_class = "Sunclock";
        XSetIconName(dpy, win, name);
	XSetClassHint(dpy, win, &xch);
	XStoreName(dpy, win, ProgName);
	XSetCommand(dpy, win, argv, argc);
       	XSelectInput(dpy, win, ExposureMask | ButtonPressMask | KeyPressMask);

}

/*
 *  Routine used to adjust the relative placement of clock and map windows
 */

void
AdjustGeom()
{
	Window			root = RootWindow(dpy, scr);
        Window			win;
	int			dx = 0, dy = 0;

        if (placement == CENTER) 
           dx = (map_icon_width - clock_icon_width)/2; else
        if (placement >= NW) {
	   if(placement & 1) 
             dx = 0; else dx = map_icon_width - clock_icon_width;
        }

        if (placement == CENTER) dy = (map_height - clock_height)/2; else
        if (placement >= NW) {
	   if (placement <= NE) 
             dy = 0; else dy = map_height - clock_height;
        }

        if (placement) {
	  if (do_map) {
	    if (placement >= CENTER) {
              XTranslateCoordinates(dpy, Map, root, 0, 0, 
                                       &MapGeom.x, &MapGeom.y, &win);
	      ClockGeom.x = MapGeom.x + dx;
	      ClockGeom.y = MapGeom.y + dy;
	    }
	  } else {
	    if (placement >= CENTER) {
              XTranslateCoordinates(dpy, Clock, root, 0, 0, 
                                       &ClockGeom.x, &ClockGeom.y, &win);
	      MapGeom.x = ClockGeom.x - dx;
	      MapGeom.y = ClockGeom.y - dy;
	    }
	  } 
	}
}

void
fixGeometry(g, w, h)
register struct geom *		g;
register int			w;
register int			h;
{
	if (g->mask & XNegative)
		g->x = DisplayWidth(dpy, scr) - w + g->x;
	if (g->mask & YNegative)
		g->y = DisplayHeight(dpy, scr) - h + g->y;
}

void
createWindow(num)
int num;
{
	XSetWindowAttributes	xswa;
	Window			root = RootWindow(dpy, scr);
	register int		mask;

	xswa.background_pixel = TextBgColor.pix;
	xswa.border_pixel = FgColor.pix;
	xswa.backing_store = WhenMapped;
	mask = CWBackPixel | CWBorderPixel | CWBackingStore;

        switch (num) {

           case 0:
		fixGeometry(&ClockGeom, clock_icon_width, clock_height);
		Clock = XCreateWindow(dpy, root,
			     ClockGeom.x, ClockGeom.y,
			     clock_icon_width, clock_height, 1, 
			     CopyFromParent, InputOutput, 
			     CopyFromParent, mask, &xswa);
		break;

	   case 1:
		fixGeometry(&MapGeom, map_icon_width, map_height);
	        Map = XCreateWindow(dpy, root,
			     MapGeom.x, MapGeom.y,
			     map_icon_width, map_height, 3,
			     CopyFromParent, InputOutput, 
			     CopyFromParent, mask, &xswa);
		break;
	}
}

/*
 *  Make and map windows in order specified by user, set the hints
 */

void
createWindows(argc, argv)
int				argc;
register char **		argv;
{
	map_height = map_icon_height + BigFont->max_bounds.ascent +
	     BigFont->max_bounds.descent +2 ;
	clock_height = clock_icon_height + SmallFont->max_bounds.ascent +
	     SmallFont->max_bounds.descent + 2;

	createWindow(do_map);
	setAllHints(argc, argv, do_map);
	XMapWindow(dpy, (do_map)? Map:Clock);
        AdjustGeom();
	createWindow(1-do_map);
	setAllHints(argc, argv, 1-do_map);
}

void
makeGCs()
{
	XGCValues		gcv;

	gcv.background = BgColor.pix;
	gcv.foreground = FgColor.pix;
	gcv.function = GXinvert;
	Invert_gc = XCreateGC(dpy, Mappix, GCForeground | GCBackground | GCFunction, &gcv);

	gcv.background = BgColor.pix;
	gcv.foreground = FgColor.pix;
	Store_gc = XCreateGC(dpy, Map, GCForeground | GCBackground, &gcv);

	gcv.foreground = CityColor0.pix;
	gcv.background = CityColor0.pix;
	CityColor0.gc = XCreateGC(dpy, Map, GCForeground | GCBackground, &gcv);

	gcv.foreground = CityColor1.pix;
	gcv.background = CityColor1.pix;
	CityColor1.gc = XCreateGC(dpy, Map, GCForeground | GCBackground, &gcv);

	gcv.foreground = CityColor2.pix;
	gcv.background = CityColor2.pix;
	CityColor2.gc = XCreateGC(dpy, Map, GCForeground | GCBackground, &gcv);

	gcv.foreground = MarkColor1.pix;
	gcv.background = MarkColor1.pix;
	MarkColor1.gc = XCreateGC(dpy, Map, GCForeground | GCBackground, &gcv);

	gcv.foreground = MarkColor2.pix;
	gcv.background = MarkColor2.pix;
	MarkColor2.gc = XCreateGC(dpy, Map, GCForeground | GCBackground, &gcv);

	gcv.foreground = LineColor.pix;
	gcv.background = LineColor.pix;
	LineColor.gc = XCreateGC(dpy, Map, GCForeground | GCBackground, &gcv);

	gcv.foreground = TropicColor.pix;
	gcv.background = TropicColor.pix;
	TropicColor.gc = XCreateGC(dpy, Map, GCForeground | GCBackground, &gcv);

	gcv.foreground = TextFgColor.pix;
	gcv.background = TextBgColor.pix;
	gcv.font = BigFont->fid;
	BigFont_gc = XCreateGC(dpy, Map, GCForeground | GCBackground | GCFont, &gcv);
	gcv.font = SmallFont->fid;
	SmallFont_gc = XCreateGC(dpy, Map, GCForeground | GCBackground | GCFont, &gcv);
}

unsigned long 
getColor(name, other)
char *           name;
unsigned long    other;
{

	XColor			c;
	XColor			e;
	register Status		s;

	s = XAllocNamedColor(dpy, DefaultColormap(dpy, scr), name, &c, &e);
	if (s != (Status)1) {
		fprintf(stderr, "%s: warning: can't allocate color `%s'\n",
			ProgName, name);
		return(other);
	}
	else
		return(c.pixel);
}

void
getColors()
{
	unsigned long  black = BlackPixel(dpy, scr);
	unsigned long  white = WhitePixel(dpy, scr);

        BgColor.pix = getColor(BgColor.name, white);
        FgColor.pix = getColor(FgColor.name, black);

        TextBgColor.pix = getColor(TextBgColor.name, white);
        TextFgColor.pix = getColor(TextFgColor.name, black);
        CityColor0.pix = getColor(CityColor0.name, black);
        CityColor1.pix = getColor(CityColor1.name, black);
        CityColor2.pix = getColor(CityColor2.name, black);
	MarkColor1.pix = getColor(MarkColor1.name, black);
	MarkColor2.pix = getColor(MarkColor2.name, black);
	LineColor.pix = getColor(LineColor.name, black);
	TropicColor.pix = getColor(TropicColor.name, black);
}

void
getFonts()
{
	BigFont = XLoadQueryFont(dpy, BIGFONT);
	if (BigFont == (XFontStruct *)NULL) {
		fprintf(stderr, "%s: can't open font `%s', using `%s'\n",
			ProgName, BIGFONT, FAILFONT);
		BigFont = XLoadQueryFont(dpy, FAILFONT);
		if (BigFont == (XFontStruct *)NULL) {
			fprintf(stderr, "%s: can't open font `%s', giving up\n",
				ProgName, FAILFONT);
			exit(1);
		}
	}
	SmallFont = XLoadQueryFont(dpy, SMALLFONT);
	if (SmallFont == (XFontStruct *)NULL) {
		fprintf(stderr, "%s: can't open font `%s', using `%s'\n",
			ProgName, SMALLFONT, FAILFONT);
		SmallFont = XLoadQueryFont(dpy, FAILFONT);
		if (SmallFont == (XFontStruct *)NULL) {
			fprintf(stderr, "%s: can't open font `%s', giving up\n",
				ProgName, FAILFONT);
			exit(1);
		}
	}
}

void
clearStrip()
{
        XClearArea(dpy, Map, 0, map_icon_height, 
                 map_icon_width, map_height-map_icon_height, True);

	time_count = PRECOUNT;
}

void
exposeMap()
{
        if (do_map)
           XClearArea(dpy, Map, map_icon_width-1, map_height-1, 1, 1, True);
	else
           XClearArea(dpy, Clock, 
              clock_icon_width-1, clock_height-1, 1, 1, True);
}

void
updateMap()
{
	time_count = PRECOUNT;
        exposeMap();
}

char *
bigtprint(ltp, gmtp)
register struct tm *		ltp;
register struct tm *		gmtp;
{
 	int			i, l;
	static char		s[128];
        static double           dist;
#ifdef NEW_CTIME
	struct timeb		tp;

	if (ftime(&tp) == -1) {
		fprintf(stderr, "%s: ftime failed: ", ProgName);
		perror("");
		exit(1);
	}
#endif

	if (progress_mode) {
	  sprintf(s, " G   %s %ld %s   %s %.3f %s  | %s", 
		  Label[L_PROGRESS], 
                  (time_progress>=1440)? time_progress/1440 : 1, 
                  (time_progress>=1440)? Label[L_DAYS] : 
                  ((time_progress>=60)? Label[L_HOUR] : Label[L_MIN]),
                  Label[L_GLOBALSHIFT], global_shift/86400.0,
		  Label[L_DAYS], Label[L_ENTER]);
          l = strlen(s);
	  if (l<98) {
	    for (i=l; i<98; i++) s[i] = ' ';
	    s[98] = '\0';
	  }
          strcat(s, Label[L_CANCEL]);
	} else
	if (do_help == 1)
	  sprintf(s, "%s   %s", ListOptions, Label[L_CONTROLS]);
	else
        if (do_help == 2) {
          sprintf(s, " %c   %s", 
              Option[2*click_pos], Help[click_pos]); 
          l = strlen(s);
	  if (l<98) {
	    for (i=l; i<98; i++) s[i] = ' ';
	    s[98] = '\0';
	  }
          strcat(s, Label[L_CANCEL]);
	}
	else

        switch(map_mode) {

	case LEGALTIME:
	   sprintf(s,
		" %s %02d:%02d:%02d %s %s %02d %s %04d    %s %02d:%02d:%02d UTC %s %02d %s %04d",
                Label[L_LEGALTIME], ltp->tm_hour, ltp->tm_min,
		ltp->tm_sec,
#ifdef NEW_CTIME
		ltp->tm_zone,
#else
		tzname[ltp->tm_isdst],
#endif
		Day_name[ltp->tm_wday], ltp->tm_mday,
		Month_name[ltp->tm_mon], 1900 + ltp->tm_year ,
                Label[L_GMTTIME],
		gmtp->tm_hour, gmtp->tm_min,
		gmtp->tm_sec, Day_name[gmtp->tm_wday], gmtp->tm_mday,
		Month_name[gmtp->tm_mon], 1900 + gmtp->tm_year);
	   break;

	case COORDINATES:
           if ((mark1.city) && mark1.full)
	   sprintf(s,
		" %s (%.2f,%.2f)  %02d:%02d:%02d %s %s %02d %s %04d   %s %02d:%02d:%02d   %s %02d:%02d:%02d",
                mark1.city->name, mark1.city->lat, mark1.city->lon,
		ltp->tm_hour, ltp->tm_min, ltp->tm_sec,
#ifdef NEW_CTIME
		ltp->tm_zone,
#else
		tzname[ltp->tm_isdst],
#endif
		Day_name[ltp->tm_wday], ltp->tm_mday,
		Month_name[ltp->tm_mon], 1900 + ltp->tm_year ,
                Label[L_SUNRISE],
		mark1.sr.tm_hour, mark1.sr.tm_min, mark1.sr.tm_sec,
                Label[L_SUNSET], 
		mark1.ss.tm_hour, mark1.ss.tm_min, mark1.ss.tm_sec);
	        else
           if ((mark1.city) && !mark1.full)
	   sprintf(s,
		" %s (%.2f,%.2f)  %02d:%02d:%02d %s %s %02d %s %04d   %s %02d:%02d:%02d",
                mark1.city->name, mark1.city->lat, mark1.city->lon,
		ltp->tm_hour, ltp->tm_min, ltp->tm_sec,
#ifdef NEW_CTIME
		ltp->tm_zone,
#else
		tzname[ltp->tm_isdst],
#endif
		Day_name[ltp->tm_wday], ltp->tm_mday,
		Month_name[ltp->tm_mon], 1900 + ltp->tm_year ,
                Label[L_DAYLENGTH],
		mark1.dl.tm_hour, mark1.dl.tm_min, mark1.dl.tm_sec);
	        else
                  sprintf(s," %s", Label[L_CLICKCITY]);
	        break;

	case SOLARTIME:
           if (mark1.city)
	   sprintf(s, " %s (%.2f,%.2f)  %s %02d:%02d:%02d   %s %02d %s %04d   %s %02d:%02d:%02d", 
                  mark1.city->name, mark1.city->lat, mark1.city->lon,
                  Label[L_SOLARTIME],
                  gmtp->tm_hour, gmtp->tm_min, gmtp->tm_sec,
                  Day_name[gmtp->tm_wday], gmtp->tm_mday,
		  Month_name[gmtp->tm_mon], 1900 + gmtp->tm_year,
                  Label[L_DAYLENGTH], 
 		  mark1.dl.tm_hour, mark1.dl.tm_min, mark1.dl.tm_sec);
           else
                  sprintf(s," %s", Label[L_CLICKLOC]);
	   break;

	case DISTANCES:
	   if(mark1.city && mark2.city) {
             dist = sin(dtr(mark1.city->lat)) * sin(dtr(mark2.city->lat))
                    + cos(dtr(mark1.city->lat)) * cos(dtr(mark2.city->lat))
                           * cos(dtr(mark1.city->lon-mark2.city->lon));
             dist = acos(dist);
             sprintf(s, " %s (%.2f %.2f) --> %s (%.2f %.2f)     "
                      "%d km  =  %d miles", 
               mark2.city->name, mark2.city->lat, mark2.city->lon, 
               mark1.city->name, mark1.city->lat, mark1.city->lon,
               (int)(EARTHRADIUS_KM*dist), (int)(EARTHRADIUS_ML*dist));
	   } else
	     sprintf(s, " %s", Label[L_CLICK2LOC]);
	   break;

	default:
	   break;
	}

        l = strlen(s);
	if (l<125) {
	  for (i=l; i<125; i++) s[i] = ' ';
	  s[125] = '\0';
	}
	return (s+label_shift);
}

char *
smalltprint(ltp, gmtp)
register struct tm *		ltp;
register struct tm *		gmtp;
{
	int			i, l;
	static char		s[80];

	if (clock_mode)
	   sprintf(s, "%02d:%02d:%02d %s", 
                ltp->tm_hour, ltp->tm_min, ltp->tm_sec,
#ifdef NEW_CTIME
		ltp->tm_zone);
#else
		tzname[ltp->tm_isdst]);
#endif
        else
	   sprintf(s, "%02d:%02d %s %02d %s %02d", ltp->tm_hour, ltp->tm_min,
		Day_name[ltp->tm_wday], ltp->tm_mday, Month_name[ltp->tm_mon],
		ltp->tm_year % 100);

        l = strlen(s);
	if (l<72) {
	  for (i=l; i<72; i++) s[i] = ' ';
	  s[72] = '\0';
	}

	return (s);
}

void
makePixmaps()
{
	Mappix = XCreatePixmapFromBitmapData(dpy, RootWindow(dpy, scr),
				 map_icon_bits, map_icon_width,
				 map_icon_height, 0, 1, 1);

	Clockpix = XCreatePixmapFromBitmapData(dpy, RootWindow(dpy, scr),
				 clock_icon_bits, clock_icon_width,
				 clock_icon_height, 0, 1, 1);
}

void
makeMapContexts()
{
	register struct sunclock * s;

	s = makeMapContext(map_icon_width, map_icon_height, Map, Mappix,
		     BigFont_gc, bigtprint, 0,
		     map_icon_height + BigFont->max_bounds.ascent + 1);
	Current = s;

	s = makeMapContext(clock_icon_width, clock_icon_height, 
		     Clock, Clockpix, SmallFont_gc, smalltprint, 6,
		     clock_icon_height + SmallFont->max_bounds.ascent + 1);
	Current->s_next = s;
	s->s_flags |= S_CLOCK;
	s->s_next = Current;
}

struct sunclock *
makeMapContext(wid, ht, win, pix, gc, fun, txx, txy)
int				wid;
int				ht;
Window				win;
Pixmap				pix;
GC				gc;
char *				(*fun)();
int				txx;
int				txy;
{
	register struct sunclock * s;

	s = (struct sunclock *)salloc(sizeof (struct sunclock));
	s->s_width = wid;
	s->s_height = ht;
	s->s_window = win;
	s->s_pixmap = pix;
	s->s_flags = S_DIRTY;
	s->s_noon = -1;
	s->s_wtab = (short *)salloc((int)(ht * sizeof (short *)));
	s->s_wtab1 = (short *)salloc((int)(ht * sizeof (short *)));
	s->s_increm = 0L;
	s->s_time = 0L;
	s->s_gc	= gc;
	s->s_tfunc = fun;
	s->s_timeout = 0;
	s->s_projtime = -1L;
	s->s_text[0] = '\0';
	s->s_textx = txx;
	s->s_texty = txy;

	return (s);
}	

void
SwitchWindows()
{
	XSizeHints		xsh;

	time_count = PRECOUNT;

        if (do_map == 0) 
	  {
          AdjustGeom();
          XUnmapWindow(dpy, Clock);
          xsh.x = MapGeom.x;
          xsh.y = MapGeom.y;
          xsh.flags = USPosition;
      	  if (placement) XSetNormalHints(dpy, Map, &xsh);
          XMapWindow(dpy, Map);
	  if (placement) XMoveWindow(dpy, Map, MapGeom.x, MapGeom.y);
	  }
        if (do_map == 1) 
	  {
          AdjustGeom();
          XUnmapWindow(dpy, Map);
          xsh.x = ClockGeom.x;
          xsh.y = ClockGeom.y;
          xsh.flags = USPosition;
      	  if (placement) XSetNormalHints(dpy, Clock, &xsh);
          XMapWindow(dpy, Clock);
	  if (placement) XMoveWindow(dpy, Clock, ClockGeom.x, ClockGeom.y);
          }
        do_map = 1 - do_map;
}

void
drawSeparator(rank, width)
int	rank, width;
{
	int j;

	for (j=CHWID*rank-width+CHWID-4; j<=CHWID*rank+CHWID-4+width; j++)
	     XDrawLine(dpy, Map, BigFont_gc, j,map_icon_height, j,map_height);
}

void
drawSeparators()
{
	int	i;
        
	if (do_help == 1) {
 	  for (i=0; i<N_OPTIONS; i++)
	      drawSeparator(i, (Option[2*i+1]==';'));
	}

	if (do_help == 2 || progress_mode) {
 	  for (i=0; i<=1; i++)
	      drawSeparator(23*i, i);
	}
}

/*
 *  Select Window and GC for monochrome mode
 */ 

void
checkMono(pw, pgc)
Window *pw;
GC     *pgc;
{
	if (mono) { 
           *pw = Mappix ; 
           *pgc = Invert_gc; 
        } else
	   *pw = Map;
}

/*
 * placeSpot() - Put a spot (city or mark) on the map.
 */

void
placeSpot(mode, lat, lon)
int    mode;
double lat, lon;		/* Latitude and longtitude of the city */
{
    /*
     * Local Variables
     */

    int ilat, ilon; 		/* Screen coordinates of the city */
    GC gc;
    Window w;

    if (mode < 0) return;
    if (mode == 0) { gc = CityColor0.gc; --mode; }
    if (mode == 1)   gc = CityColor1.gc;
    if (mode == 2)   gc = CityColor2.gc;
    if (mode == 3)   gc = MarkColor1.gc;
    if (mode == 4)   gc = MarkColor2.gc;

    checkMono(&w, &gc);

    ilat = map_icon_height - (lat + 90) * (map_icon_height / 180.0);
    ilon = (180.0 + lon) * (map_icon_width / 360.0);

    if (spot_size == 1)
       XDrawArc(dpy, w, gc, ilon-2, ilat-2, 4, 4, 0, 360 * 64);
    if (spot_size == 2)
       XFillArc(dpy, w, gc, ilon-3, ilat-3, 6, 6, 0, 360 * 64);    
    if (spot_size == 3)
        {
        XDrawArc(dpy, w, gc, ilon-5, ilat-5, 10, 10, 0, 360 * 64);
        XFillArc(dpy, w, gc, ilon-3, ilat-3, 6, 6, 0, 360 * 64);
        }
}

void
drawCities()
{
City *c;
        if (spot_size) 
        for (c = cities; c; c = c->next)
	    placeSpot(c->mode, c->lat, c->lon);
}

void
drawMarks()
{
        if (mono) return;

        /* code for color mode */
        if (mark1.city == &pos1)
	  placeSpot(3, mark1.city->lat, mark1.city->lon);
        if (mark2.city == &pos2)
	  placeSpot(4, mark2.city->lat, mark2.city->lon);
}

/*
 * draw_parallel() - Draw a parallel line
 */

void
draw_parallel(gc, lat)
GC gc;
double lat;
{
	Window w;
	int ilat;

        checkMono(&w, &gc);

	ilat = map_icon_height - (lat + 90) * (map_icon_height / 180.0);

	XDrawLine(dpy, w, gc, 0, ilat, map_icon_width-1, ilat);
}

void
draw_parallels()
{
	int     i;

        for (i=-8; i<=8; i++)
	  draw_parallel(LineColor.gc, i*10.0);
}

void
draw_tropics()
{
	static  double val[5] = { -66.55, -23.45, 0.0, 23.45, 66.55 };
	int     i;

        for (i=0; i<5; i++)
	  draw_parallel(TropicColor.gc, val[i]);
}

/*
 * draw_meridian() - Draw a meridian line
 */

void
draw_meridian(gc, lon)
GC gc;
double lon;
{
	Window w;
	int ilon;

        checkMono(&w, &gc);

	ilon = (180.0 + lon) * (map_icon_width / 360.0);

	XDrawLine(dpy, w, gc, ilon, 0, ilon, map_icon_height-1);
}

void
draw_meridians()
{
	int     i;

        for (i=-11; i<=11; i++)
	  draw_meridian(LineColor.gc, i*15.0);
}

void
drawLines()
{
	if (do_merid)
	  draw_meridians();
	if (do_parall)
	  draw_parallels();
	if (do_tropics)
	  draw_tropics();
}

void
drawAll()
{
        drawLines();
        drawCities();
	drawMarks();
}

void
pulseMarks()
{
int     done = 0;
	if (mark1.city && mark1.status<0) {
           if (mark1.pulse) {
	     placeSpot(0, mark1.save_lat, mark1.save_lon);
	     done = 1;
	   }
	   mark1.save_lat = mark1.city->lat;
	   mark1.save_lon = mark1.city->lon;
           if (mark1.city == &pos1) {
	      done = 1;
              placeSpot(0, mark1.save_lat, mark1.save_lon);
	      mark1.pulse = 1;
	   } else
	      mark1.pulse = 0;
	   mark1.status = 1;
	}
	else
        if (mark1.status>0) {
	   if (mark1.city|| mark1.pulse) {
              placeSpot(0, mark1.save_lat, mark1.save_lon);
	      mark1.pulse = 1-mark1.pulse;
	      done = 1;
	   }
	   if (mark1.city == NULL) mark1.status = 0;
	}

	if (mark2.city && mark2.status<0) {
           if (mark2.pulse) {
	     placeSpot(0, mark2.save_lat, mark2.save_lon);
	     done = 1;
	   }
	   mark2.save_lat = mark2.city->lat;
	   mark2.save_lon = mark2.city->lon;
           if (mark2.city == &pos2) {
              placeSpot(0, mark2.save_lat, mark2.save_lon);
	      done = 1;
	      mark2.pulse = 1;
	   } else
	      mark2.pulse = 0;
	   mark2.status = 1;
	}
	else
        if (mark2.status>0) {
	   if (mark2.city || mark2.pulse) {
              placeSpot(0, mark2.save_lat, mark2.save_lon);
              mark2.pulse = 1 - mark2.pulse;
	      done = 1;
	   }
	   if (mark2.city == NULL) mark2.status = 0;
	}
	if (done) exposeMap();
}

/*  PROJILLUM  --  Project illuminated area on the map.  */

void
projillum(wtab, xdots, ydots, dec)
short *wtab;
int xdots, ydots;
double dec;
{
	int i, ftf = True, ilon, ilat, lilon = 0, lilat = 0, xt;
	double m, x, y, z, th, lon, lat, s, c;

	/* Clear unoccupied cells in width table */

	for (i = 0; i < ydots; i++)
		wtab[i] = -1;

	/* Build transformation for declination */

	s = sin(-dtr(dec));
	c = cos(-dtr(dec));

	/* Increment over a semicircle of illumination */

	for (th = -(PI / 2); th <= PI / 2 + 0.001;
	    th += PI / TERMINC) {

		/* Transform the point through the declination rotation. */

		x = -s * sin(th);
		y = cos(th);
		z = c * sin(th);

		/* Transform the resulting co-ordinate through the
		   map projection to obtain screen co-ordinates. */

		lon = (y == 0 && x == 0) ? 0.0 : rtd(atan2(y, x));
		lat = rtd(asin(z));

		ilat = ydots - (lat + 90.0) * (ydots / 180.0);
		ilon = lon * (xdots / 360.0);

		if (ftf) {

			/* First time.  Just save start co-ordinate. */

			lilon = ilon;
			lilat = ilat;
			ftf = False;
		} else {

			/* Trace out the line and set the width table. */

			if (lilat == ilat) {
				wtab[(ydots - 1) - ilat] = ilon == 0 ? 1 : ilon;
			} else {
				m = ((double) (ilon - lilon)) / (ilat - lilat);
				for (i = lilat; i != ilat; i += sgn(ilat - lilat)) {
					xt = lilon + floor((m * (i - lilat)) + 0.5);
					wtab[(ydots - 1) - i] = xt == 0 ? 1 : xt;
				}
			}
			lilon = ilon;
			lilat = ilat;
		}
	}

	/* Now tweak the widths to generate full illumination for
	   the correct pole. */

	if (dec < 0.0) {
		ilat = ydots - 1;
		lilat = -1;
	} else {
		ilat = 0;
		lilat = 1;
	}

	for (i = ilat; i != ydots / 2; i += lilat) {
		if (wtab[i] != -1) {
			while (True) {
				wtab[i] = xdots / 2;
				if (i == ilat)
					break;
				i -= lilat;
			}
			break;
		}
	}
}

/*  XSPAN  --  Complement a span of pixels.  Called with line in which
	       pixels are contained, leftmost pixel in the  line,  and
	       the   number   of   pixels   to	 complement.   Handles
	       wrap-around at the right edge of the screen.  */

void
xspan(pline, leftp, npix, xdots, p)
register int			pline;
register int			leftp;
register int			npix;
register int			xdots;
register Pixmap			p;
{
	leftp = leftp % xdots;

	if (leftp + npix > xdots) {
		XDrawLine(dpy, p, Invert_gc, leftp, pline, xdots - 1, pline);
		XDrawLine(dpy, p, Invert_gc, 0, pline,
			  (leftp + npix) - (xdots + 1), pline);
	}
	else
		XDrawLine(dpy, p, Invert_gc, leftp, pline,
			  leftp + (npix - 1), pline);
}

/*  MOVETERM  --  Update illuminated portion of the globe.  */

void
moveterm(wtab, noon, otab, onoon, xdots, ydots, pixmap)
short *wtab, *otab;
int noon, onoon, xdots, ydots;
Pixmap pixmap;
{
	int i, ol, oh, nl, nh;

	for (i = 0; i < ydots; i++) {

		/* If line is off in new width table but is set in
		   the old table, clear it. */

		if (wtab[i] < 0) {
			if (otab[i] >= 0) {
				xspan(i, ((onoon - otab[i]) + xdots) % xdots,
				    otab[i] * 2, xdots, pixmap);
			}
		} else {

			/* Line is on in new width table.  If it was off in
			   the old width table, just draw it. */

			if (otab[i] < 0) {
				xspan(i, ((noon - wtab[i]) + xdots) % xdots,
				    wtab[i] * 2, xdots, pixmap);
			} else {

				/* If both the old and new spans were the entire
				   screen, they're equivalent. */

				if (otab[i] == wtab[i] && wtab[i] == (xdots / 2))
					continue;

				/* The line was on in both the old and new width
				   tables.  We must adjust the difference in the
				   span.  */

				ol =  ((onoon - otab[i]) + xdots) % xdots;
				oh = (ol + otab[i] * 2) - 1;
				nl =  ((noon - wtab[i]) + xdots) % xdots;
				nh = (nl + wtab[i] * 2) - 1;

				/* If spans are disjoint, erase old span and set
				   new span. */

				if (oh < nl || nh < ol) {
					xspan(i, ol, (oh - ol) + 1, xdots, pixmap);
					xspan(i, nl, (nh - nl) + 1, xdots, pixmap);
				} else {
					/* Clear portion(s) of old span that extend
					   beyond end of new span. */
					if (ol < nl) {
						xspan(i, ol, nl - ol, xdots, pixmap);
						ol = nl;
					}
					if (oh > nh) {
						xspan(i, nh + 1, oh - nh, xdots, pixmap);
						oh = nh;
					}
					/* Extend existing (possibly trimmed) span to
					   correct new length. */
					if (nl < ol) {
						xspan(i, nl, ol - nl, xdots, pixmap);
					}
					if (nh > oh) {
						xspan(i, oh + 1, nh - oh, xdots, pixmap);
					}
				}
			}
		}
		otab[i] = wtab[i];
	}
}

/* --- */
/*  UPDIMAGE  --  Update current displayed image.  */

void
updateImage(s)
register struct sunclock *	s;
{
	register int		i;
	int			xl;
	struct tm *		ct;
	double			jt;
	double			sunra;
	double			sundec;
	double			sunrv;
	double			sunlong;
	double			gt;
	short *			wtab_swap;
	time_t                  gtime;

	/* If this is a full repaint of the window, force complete
	   recalculation. */

	if (s->s_noon < 0) {
		s->s_projtime = 0;
		for (i = 0; i < s->s_height; i++) {
			s->s_wtab1[i] = -1;
		}
	    if (mono && do_map) drawAll();
	}

	if (s->s_flags & S_FAKE) {
		if (s->s_flags & S_ANIMATE)
			s->s_time += s->s_increm;
		if (s->s_time < 0)
			s->s_time = 0;
	} else
		time(&s->s_time);
	
  	gtime = s->s_time + global_shift;
	ct = gmtime(&gtime);
	jt = jtime(ct);
	sunpos(jt, False, &sunra, &sundec, &sunrv, &sunlong);
	gt = gmst(jt);

	/* Projecting the illumination curve  for the current seasonal
           instant is costly.  If we're running in real time, only  do
	   it every PROJINT seconds.  */

	if ((s->s_flags & S_FAKE)
	 || s->s_projtime < 0
	 || (s->s_time - s->s_projtime) > PROJINT 
         || force_proj) {
		projillum(s->s_wtab, s->s_width, s->s_height, sundec);
		wtab_swap = s->s_wtab;
		s->s_wtab = s->s_wtab1;
		s->s_wtab1 = wtab_swap;
		s->s_projtime = s->s_time;
	}

	sunlong = fixangle(180.0 + (sunra - (gt * 15)));
	xl = sunlong * (s->s_width / 360.0);

	/* If the subsolar point has moved at least one pixel, update
	   the illuminated area on the screen.	*/

	if ((s->s_flags & S_FAKE) || s->s_noon != xl || force_proj) {
		moveterm(s->s_wtab1, xl, s->s_wtab, s->s_noon, s->s_width,
			 s->s_height, s->s_pixmap);
		s->s_noon = xl;
		s->s_flags |= S_DIRTY;
		force_proj = 0;
	}
}

void
showText(s)
register struct sunclock *	s;
{
	XDrawImageString(dpy, s->s_window, s->s_gc, s->s_textx,
			 s->s_texty, s->s_text, strlen(s->s_text));
        drawSeparators();
}

/*
 *  Set the timezone of selected location
 */

void
setTZ(cptr)
City	*cptr;
{
	char buf[80];

	if (cptr && do_map)
	        sprintf(buf, "TZ=%s", cptr->tz);
	else
	        strcpy(buf, "TZ");
	putenv(buf);
	tzset();
}

void
showImage(s)
register struct sunclock *	s;
{
	register char *		p;
	register struct tm *	gmtp;
	struct tm		lt;
	time_t                  ltime, gtime;

        ltime = s->s_time + global_shift;

	setTZ(mark1.city);
	lt = *localtime(&ltime);

        gtime = ltime + local_shift;
	gmtp = gmtime(&gtime);
 
	p = (*s->s_tfunc)(&lt, gmtp);

	if (s->s_flags & S_DIRTY) {
		XCopyPlane(dpy, s->s_pixmap, s->s_window, Store_gc, 0, 0,
			   s->s_width, s->s_height, 0, 0, 1);
		if (s->s_flags & S_CLOCK)
			XClearArea(dpy, s->s_window, 0, s->s_height + 1,
				   0, 0, False);
		s->s_flags &= ~S_DIRTY;
	}
	strcpy(s->s_text, p);
	showText(s);
	if (!mono) drawAll();
}

/* get "daylength" at a location of latitude lat, when it is noon at
   GMT time gt at that location */

double
dayLength(gt, lat)
double	gt, lat;
{
	double			duration;
	double			sunra;
	double			sundec;
	double			sunrv;
	double			sunlong;
	double                  sinsun, sinpos, num;

        /* Get sun position */

	sunpos(gt, False, &sunra, &sundec, &sunrv, &sunlong);       
        sinpos = sin(dtr(lat));

        /* Correct for the sun apparent diameter */

        if (lat>0) sundec += 0.5 * SUN_APPDIAM * sinpos;
        if (lat<=0) sundec -= 0.5 * SUN_APPDIAM * sinpos;

        sinsun = sin(dtr(sundec));

        if (sinsun==0 || sinpos==0)
	  duration = 12.0 + SUN_APPDIAM/(15.0 * cos(dtr(lat)));
	else {
          num = 1 - sinsun*sinsun - sinpos*sinpos;
          if (num<=0) {
	    if (sinsun*sinpos>0) 
	      duration = 24.0;
	    else
	      duration = 0.0;
	  } else
	      duration = 12.0 + 24.0*atan(sinsun*sinpos/sqrt(num))/PI
			      + SUN_APPDIAM/(15.0 * cos(dtr(lat)));
	}
	return duration*3600;
}

void
setDayParams(cptr)
City *cptr;
{
        struct tm *             gtm;
	struct tm *             ltm;
	double			gt, lt;
	double                  duration, shift, corr;
	time_t			sr, ss, dl, ct;

        time_count = PRECOUNT;
	force_proj = 1;

	if (!cptr) return;

  	ct = Current->s_time + global_shift;

        /* GMT time in julian days (and fraction of day) */
        gtm = gmtime(&ct);
	gt = jtime(gtm);

	/* legal time in julian days (and fraction of day) */
	setTZ(cptr);
        ltm = localtime(&ct);
	lt = jtime(ltm);
        /* Difference, in sec, between legal time and solar time */
        shift = (lt -gt)*86400.0-cptr->lon*240.0;

        /* Correct GMT time so that we are at noon in solar time */ 
        corr = ltm->tm_hour/24.0-0.5+(ltm->tm_min)/1440.0 + 
              (ltm->tm_sec - shift)/86400.0;
        gt  = gt - corr;

        /* get day length at that time and location*/
        duration = dayLength(gt, cptr->lat);

        /* improve approximation by reiterating calculation with
	   gt = approx sunrise or approx sunset */
	sr = 43200 - 0.5*dayLength(gt-5.78704E-6*duration, cptr->lat) + shift;
	ss = 43200 + 0.5*dayLength(gt+5.78704E-6*duration, cptr->lat) + shift;
        dl = ss-sr;
        mark1.full = 1;
	if (dl<=0) {dl = 0; mark1.full = 0;}
	if (dl>86380) {dl=86400; mark1.full = 0;}

	mark1.sr = *gmtime(&sr);
	mark1.ss = *gmtime(&ss);
	mark1.dl = *gmtime(&dl);
	mark1.dl.tm_hour += (mark1.dl.tm_mday-1) * 24;
}

/*
 * processPoint() - This is kind of a cheesy way to do it but it works. What happens
 *                  is that when a different city is picked, the TZ environment
 *                  variable is set to the timezone of the city and then tzset().
 *                  is called to reset the system.
 */

void
processPoint(x, y)
int x, y;      /* Screen co-ordinates of mouse */
{
    /*
     * Local Variables
     */

    City *cptr;    /* Used to search for a city */
    int cx, cy;    /* Screen coordinates of the city */

    /* Loop through the cities until on close to the pointer is found */

    local_shift = 0;
    
    for (cptr = cities; cptr; cptr = cptr->next) {

        /* Convert the latitude and longitude of the cities to integer */

        cx = (180.0 + cptr->lon) * (map_icon_width / 360.0);
	cy = map_icon_height - (cptr->lat + 90.0) * (map_icon_height / 180.0);

	/* Check to see if we are close enough */

	if ((cx >= x - 3) && (cx <= x + 3) && (cy >= y - 3) && (cy <= y + 3 ))
           break;
    }

    if (map_mode == LEGALTIME) {
      if (cptr)
      	map_mode = COORDINATES;
      else
	map_mode = SOLARTIME;
    }

    switch(map_mode) {

      case COORDINATES:
	if (cptr) {
   	   if (mark1.city) mark1.city->mode = 0;
           mark1.city = cptr;
           cptr->mode = 1;
	}
	break;

      case DISTANCES:
        if (mark2.city) mark2.city->mode = 0;
        if (mark2.city == &pos2) updateMap();
	if (mark1.city == &pos1) {
	    pos2 = pos1;
	    mark2.city = &pos2;
	} else
            mark2.city = mark1.city;
        if (mark2.city) mark2.city->mode = 2;
        if (cptr) {
          mark1.city = cptr;
          mark1.city->mode = 1;
        } else {
          pos1.name = Label[L_POINT];
	  pos1.lat = 90.0-((double)y/(double)map_icon_height)*180.0 ;
          pos1.lon = ((double)x/(double)map_icon_width)*360.0-180.0 ;
	  mark1.city= &pos1;
	}
        break;

      case SOLARTIME:
        if (mark1.city) 
           mark1.city->mode = 0;
        if (mark1.city== &pos1) updateMap();
        if (cptr) {
          mark1.city= cptr;
          mark1.city->mode = 1;
        } else {
          pos1.name = Label[L_POINT];
	  pos1.lat = 90.0-((double)y/(double)map_icon_height)*180.0 ;
          pos1.lon = ((double)x/(double)map_icon_width)*360.0-180.0 ;
	  mark1.city = &pos1;
	}
        local_shift = (long) (mark1.city->lon * 240.0);
        break;

      default:
	break;
    }

    setDayParams(mark1.city);
    
    if (mono) {
      if (mark1.city) mark1.status = -1;
      if (mark2.city) mark2.status = -1;
    }
}

/*
 *  Process key events in eventLoop
 */

void
processKey(key)
char	key;
{
	int i;

        time_count = PRECOUNT;

        if (progress_mode) {
           if (key == '\015' || key == '\033') {
	     clearStrip();
	     progress_mode = 0;
	     return;
	   }
           if (key != 'g' && key !='a' && key != 'b' && key != 'z') return;
	}

        if (do_help) {
	   clearStrip();
	   ++do_help;
           i = 0;
           while (i<N_OPTIONS && Option[2*i] != key-32) ++i;
           if (i < N_OPTIONS) {
              if (do_help == 2) {
		click_pos = i;
	        return;
	      }
              if (do_help == 3 && click_pos!= i) {
		do_help = 0;
	        return;
	      }
	      do_help = 0;
	   }
	   else {
	      do_help = 0;
	      return;
	   }
	}

        switch(key) {
	   case 'Q': 
	     if (label_shift<20)
             ++label_shift;
	     break;
	   case 'S': 
	     if (label_shift>0)
	       --label_shift;
	     break;
	   case 'a': 
	     global_shift += (time_progress * 60);
	     setDayParams(mark1.city);
	     break;
	   case 'b': 
	     global_shift -= (time_progress * 60);
	     setDayParams(mark1.city);
	     break;
	   case 'c': 
	     local_shift = 0;
	     map_mode = COORDINATES;
	     if (mark1.city == &pos1 || mark2.city == &pos2) updateMap();
             if (mark1.city == &pos1) mark1.city = NULL;
	     if (mark1.city)
               setDayParams(mark1.city);
             if (mark2.city) mark2.city->mode = 0;
             mark2.city = NULL;
	     break;
	   case 'd': 
	     map_mode = DISTANCES;
	     break;
	   case 'g': 
	     if (!do_map) break;
	     if (progress_mode) {
		     if (time_progress == 1) time_progress = 60;
		       else
		     if (time_progress == 60) time_progress = 1440;
		       else
		     if (time_progress == 1440) time_progress = 10080;
		       else
		     if (time_progress == 10080) time_progress = 43200;
	       	       else
		     if (time_progress == 43200) time_progress = 1;
	     } else {
		     progress_mode = 1;
	     	     do_help = 0;
	     }
	     break;
	   case 'h': 
             ++do_help;
	     break;
	   case 'i': 
             XIconifyWindow(dpy, (do_map)? Map:Clock, scr);
	     break;
	   case 'u':
	     if (mono)
	       drawCities();
	     else {
               if (do_nocities) {
	         spot_size = do_nocities;
	         do_nocities = 0;
	       } else {
	         do_nocities = spot_size;
	         spot_size = 0;
	       }
	     }
             updateMap();
	   case 'l':
	     map_mode = LEGALTIME;
	     local_shift = 0;
             if (mark1.city == &pos1 || mark2.city == &pos2) updateMap();
             if (mark1.city) mark1.city->mode = 0;
             if (mark2.city) mark2.city->mode = 0;
             mark1.city = NULL;
             mark2.city = NULL;
	     break;
	   case 'm':
             do_merid = 1 - do_merid;
	     if (mono) draw_meridians();
	     exposeMap();
	     break;
	   case 'p':
             do_parall = 1 - do_parall;
	     if (mono) draw_parallels();
	     exposeMap();
	     break;
	   case 'q': 
	     shutDown();
	     exit(0);
	   case 's': 
	     map_mode = SOLARTIME;
             if (mark2.city == &pos2) updateMap();
             if (mark2.city) mark2.city->mode = 0;
             mark2.city = NULL;
	     if (mark1.city) {
	       local_shift = (long)(mark1.city->lon*240.0);
	       setDayParams(mark1.city);
	     } else
	       local_shift = 0;
	     break;
	   case 't':
             do_tropics = 1 - do_tropics;
	     if (mono) draw_tropics();
             updateMap();
	     break;
	   case ' ':
	   case 'w':
	     SwitchWindows();
	   case 'r':
	     updateMap();
	     break;
	   case 'x':
	     if (Command) system(Command);
	     break;
	   case 'z':
	     global_shift = 0;
	     setDayParams(mark1.city);
	     break;
        default:
	     if (!do_map) {
	       clock_mode = 1-clock_mode;
	       updateMap();
	     }
	     break ;
	}
}

/*
 *  Process mouse events in eventLoop
 */

void
processMouse(x, y)
int	x, y;
{
char	key;

	if (do_map == 0) {
	   /* Show help on click */
	   if (y <= clock_icon_height)
	        SwitchWindows();
	   else {
                /* Click on bottom strip of clock */
	        clock_mode = 1-clock_mode;
	        updateMap();
	   }
           return;
	}

        time_count = PRECOUNT;

        /* Click on bottom strip of map */
        if (do_map == 1 && y > map_icon_height) {
	   if (progress_mode) {
              clearStrip();
	      if (x <= 24*CHWID -4) {
		processKey('g');
	        return;
	      }
	      progress_mode = 0;
	      return;
	   }
	   ++do_help;
	   if (do_help == 2) {
	      click_pos = (x+3)/CHWID;
	      clearStrip();
	      if (click_pos>=N_OPTIONS) {
		 do_help = 0;
	      } else
		usleep(20*TIMESTEP);
	   } else
	   if (do_help == 3) {
	      do_help = 0;
	      clearStrip();
	      if (x <= 24*CHWID - 4) {
	         key = Option[2*click_pos]+32;
		 processKey(key);
	      }
	   }
           return;
	}

	/* Set the timezone, marks, etc, on a button press */

        if (do_map && !do_help) processPoint(x, y);
}

void
setTimeout(s)
register struct sunclock *	s;
{
	long			t;

	if (s->s_flags & S_CLOCK & !force_proj) {
		time(&t);
		setTZ(mark1.city);
		s->s_timeout = 60 - localtime(&t)->tm_sec;
	}
	else
		s->s_timeout = 1;
}

/*
 * Got an expose event for window w.  Do the right thing if it's not
 * currently the one we're displaying.
 */

void
doTimeout()
{
	if (QLength(dpy))
		return;		/* ensure events processed first */

        time_count = (time_count+1) % TIMECOUNT;

	if (--Current->s_timeout <= 0 && (time_count==0)) {
		updateImage(Current);
		showImage(Current);
		setTimeout(Current);
                if (mono) pulseMarks();
	}
}

void
doExpose(w)
register Window			w;
{
	if (w != Current->s_window) {
		Current = Current->s_next;
		if (w != Current->s_window) {
			fprintf(stderr,
				"%s: expose event for unknown window, id = 0x%08lx\n",
				ProgName, w);
			exit(1);
		}
		setTimeout(Current);
	}
	updateImage(Current);
	Current->s_flags |= S_DIRTY;
	showImage(Current);
}

/*
 * Someone is sure to wonder why the event loop is coded this way, without
 * using select().  The answer is that this was developed on a System V
 * kernel, which has select() but the call has bugs; so, I was inspired
 * to make it portable to systems without select().  The slight delay in
 * expose event processing that results from using sleep(1) rather than
 * alarm() is a fine payoff for not having to worry about interrupted
 * system calls.
 *
 * I've got to use XCheckIfEvent with a degenerate predicate rather than
 * XCheckMaskEvent with a mask of -1L because the latter won't collect all
 * types of events, notably ClientMessage and Selection events.  Sigh.
 */

Bool
evpred(d, e, a)
register Display *		d;
register XEvent *		e;
register char *			a;
{
	return (True);
}

void
eventLoop()
{
	XEvent			ev;
	int			key;

	for (;;) {
		if (XCheckIfEvent(dpy, &ev, evpred, (char *)0))
			switch(ev.type) {
		
			case Expose:
				if (ev.xexpose.count == 0)
					doExpose(ev.xexpose.window);
				break;

			case KeyPress:
                                key = XKeycodeToKeysym(dpy,ev.xkey.keycode,0);
				processKey(key & 255);
                                break;

			case ButtonPress:
			        processMouse(ev.xbutton.x, ev.xbutton.y);
				break;

			} else {
 		        usleep(TIMESTEP);
			doTimeout();
		}
	}
}

void checkLocation(name)
char *	name;
{
City *c;
int  i; 
	if (CityInit)
        for (c = cities; c; c = c->next)
	    if (!strcasecmp(c->name, CityInit)) {
		map_mode = COORDINATES;
		mark1.city = NULL;
		i = do_map;
		do_map = 1;
		setTZ(c);
		doTimeout();
		mark1.city = c;
                if (mono) mark1.status = -1;
		c->mode = 1;
		setDayParams(c);
		do_map = i;
		return;
            }

	if (mark1.city == &pos1) {
		map_mode = SOLARTIME;
		mark1.city = NULL;
		setTZ(NULL);
		doTimeout();
		mark1.city = &pos1;
                if (mono) mark1.status = -1;
		pos1.name = Label[L_POINT];
	        local_shift = (long) (pos1.lon * 240.0);
		setDayParams(&pos1);
        }	
}

int
main(argc, argv)
int				argc;
register char **		argv;
{
	char *			p;

	/* Read the ~/.sunclockrc file */

	if (readrc())
	    exit(1);

        initValues();

	ProgName = *argv;
	if ((p = strrchr(ProgName, '/')))
		ProgName = ++p;
	parseArgs(argc, argv);

	dpy = XOpenDisplay(Display_name);
	if (dpy == (Display *)NULL) {
		fprintf(stderr, "%s: can't open display `%s'\n", ProgName,
			Display_name);
		exit(1);
	}
	scr = DefaultScreen(dpy);

	getColors();
	getFonts();
	makePixmaps();
	createWindows(argc, argv);
	makeGCs();
	makeMapContexts();
	checkLocation(CityInit);
	eventLoop();
	exit(0);
}
