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

       3.10  10/03/00  Menu window and a lot of new improvements as well.
       3.11  10/08/00  Many clean-ups and bug fixes.

*/

#include <unistd.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <string.h>

#include "sunclock.h"
#include "langdef.h"

#ifdef	DEFINE_BITS
#include "clock_icon.xbm"
#include "map_icon.xbm"
#endif

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
extern long	jdate();

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

struct geom {
	int	mask;
	int	x;
	int	y;
};

/*
 * bits in s_flags
 */

#define	S_FAKE		01	/* date is fake, don't use actual time */
#define	S_ANIMATE	02	/* do animation based on increment */
#define	S_DIRTY		04	/* pixmap -> window copy required */
#define	S_CLOCK		010	/* this is the clock window */

char share_file[] = SHAREDIR"/Sunclock.**";
char app_default[] = APPDEF"/Sunclock";
char language[4] = "";

char *	ProgName;
char *  Command = NULL;
char *rc_file = NULL;

Color	BgColor, FgColor, TextBgColor, TextFgColor,
	CityColor0, CityColor1, CityColor2,
	MarkColor1, MarkColor2, LineColor, TropicColor, SunColor;

char *		Display_name = "";
Display *	dpy;
int		scr;

Atom		wm_delete_window;
Atom		wm_protocols;

XFontStruct *	SmallFont;
XFontStruct *	BigFont;

GC		Store_gc;
GC		Invert_gc;
GC		BigFont_gc;
GC		SmallFont_gc;

Window		Clock;
Window		Map;
Window		Menu;
Pixmap		Clockpix;
Pixmap		Mappix;
struct geom	ClockGeom = { 0, 20, 20 };
struct geom	MapGeom = { 0, 30, 30 };

struct sunclock *	Current;

int             radius[4] = {0, 2, 3, 5};

int             spot_size = 2;
int		placement = -1;
int             mono = 0;
int		map_height;
int		clock_strip_height;
int		map_strip_height;
int		clock_height;

int             horiz_shift = 0;
int             vert_shift = 12;
int		label_shift = 0;

int             horiz_drift = 0;
int             vert_drift =  0;

int             time_count = PRECOUNT;
int		force_proj = 0;
int             first_time = 1;
int             last_hint = -1;
int             switched = 1;

int             do_map = 0;
int             do_hint = 0;
int		do_menu = 1;

int		do_night = 1;
int		do_sunpos = 1;
int		do_cities = 1;

int		do_parall = 0;
int		do_tropics = 0;
int		do_merid = 0;
int		do_bottom = 0;

int		clock_mode = 0;
int             deg_mode = 0;

char            map_mode = LEGALTIME;
char		CityInit[80] = "";

long		time_jump = 0;
long		time_progress = 60;
long		time_progress_init = 0;

double		sun_long = 0.0;
double		sun_decl = 200.0;

/* Records to hold extra marks 1 and 2 */

City pos1, pos2, *cities = NULL;
Mark mark1, mark2;

void
usage()
{
  	int	i;

	fprintf(stderr, "%s: version %s\nUsage:\n"
        "%s [-help] [-version] [-mono]\n"
        SP"[-display name] [-rcfile file] [-command string] [-language name]\n"
        SP"[-menu] [-nomenu] [-horizshift h (map<->menu)] [-vertshift v]\n"
	SP"[-clock] [-clockgeom +x+y] [-date] [-seconds]\n"
        SP"[-map] [-mapgeom +x+y] [-mapmode * <L,C,S,D,E>]\n"
        SP"[-placement (random, fixed, center, NW, NE, SW, SE)]\n"
        SP"[-jump number[s,m,h,d,M,Y]] [-progress number[s,m,h,d,M,Y]]\n"
        SP"[-spot size(0,1,2,3)] [-city name] [-position latitude longitude]\n"
        SP"[-night] [-cities] [-sunpos] [-meridians] [-parallels] [-tropics]\n"
        SP"[-nonight] [-nocities] [-nosunpos]\n"
        SP"[-nomeridians] [-noparallels] [-notropics]\n"
        SP"[-bg color] [-fg color] [-textbg color] [-textfg color]\n"
        SP"[-citycolor0 color] [-citycolor1 color] [-citycolor2 color]\n"
        SP"[-markcolor1 color] [-markcolor2 color]\n"
        SP"[-linecolor color] [-tropiccolor color] [-suncolor color]\n"
          "\n%s\n", 
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
	strcpy(TextBgColor.name, "Grey92");
	strcpy(TextFgColor.name, "Black");
	strcpy(CityColor0.name, "Orange");
	strcpy(CityColor1.name, "Red");
	strcpy(CityColor2.name, "Red3");
	strcpy(MarkColor1.name, "Green");
	strcpy(MarkColor2.name, "Green3");
	strcpy(LineColor.name, "Blue");
	strcpy(TropicColor.name, "Purple");
	strcpy(SunColor.name, "Gold");
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
checkRCfile(argc, argv)
register int			argc;
register char **		argv;
{
	int i;

	for (i=1; i<argc-1; i++) {
           if (strcasecmp(argv[i], "-rcfile") == 0)
	      rc_file = argv[i+1];
           if (strcasecmp(argv[i], "-language") == 0)
	      strncpy(language, argv[i+1], 2);
	}
	   
}

int
parseArgs(argc, argv, cond)
register int			argc;
register char **		argv;
register int			cond;
{
	int	opt;

	while (--argc > 0) {
		++argv;
		if (strcasecmp(*argv, "-display") == 0 && argc>1) {
			needMore(argc, argv);
			Display_name = *++argv;
			--argc;
		} 
		if (strcasecmp(*argv, "-language") == 0 && argc>1) {
			needMore(argc, argv);
			strncpy(language, *++argv, 2);
			--argc;
		} 
                else if (strcasecmp(*argv, "-rcfile") == 0 && argc>1) {
			needMore(argc, argv);
			++argv;  /* already done in checkRCfile */
			--argc;
		}
		else if (strcasecmp(*argv, "-clockgeom") == 0 && argc>1) {
			needMore(argc, argv);
			getGeom(*++argv, &ClockGeom);
			if (placement <= RANDOM) placement = FIXED;
	                if (!MapGeom.mask && placement <= FIXED) 
                           getGeom(*argv, &MapGeom);
			--argc;
		}
		else if (strcasecmp(*argv, "-mapgeom") == 0 && argc>1) {
			needMore(argc, argv);
			placement = FIXED;
			getGeom(*++argv, &MapGeom);
			if (placement <= RANDOM) placement = FIXED;
	                if (!ClockGeom.mask && placement <= FIXED) 
	                   getGeom(*argv, &ClockGeom);
			--argc;
		}
		else if (strcasecmp(*argv, "-mapmode") == 0 && argc>1) {
			needMore(argc, argv);
                        if (!strcasecmp(*++argv, "c")) map_mode = COORDINATES;
                        if (!strcasecmp(*argv, "d")) map_mode = DISTANCES;
                        if (!strcasecmp(*argv, "e")) map_mode = EXTENSION;
                        if (!strcasecmp(*argv, "l")) {
			  *CityInit = '\0';
			  map_mode = LEGALTIME;
			}
                        if (!strcasecmp(*argv, "s")) map_mode = SOLARTIME;
			--argc;
		}
		else if (strcasecmp(*argv, "-spot") == 0 && argc>1) {
			needMore(argc, argv);
			spot_size = atoi(*++argv);
                        if (spot_size<0) spot_size = 0;
                        if (spot_size>3) spot_size = 3;
                        --argc;
		}
		else if (strcasecmp(*argv, "-bg") == 0 && argc>1) {
			needMore(argc, argv);
			strcpy(BgColor.name, *++argv);
			--argc;
		}
		else if (strcasecmp(*argv, "-fg") == 0 && argc>1) {
			needMore(argc, argv);
			strcpy(FgColor.name, *++argv);
			--argc;
		}
		else if (strcasecmp(*argv, "-textbg") == 0 && argc>1) {
			needMore(argc, argv);
			strcpy(TextBgColor.name, *++argv);
			--argc;
		}
		else if (strcasecmp(*argv, "-textfg") == 0 && argc>1) {
			needMore(argc, argv);
			strcpy(TextFgColor.name, *++argv);
			--argc;
		}
		else if (strcasecmp(*argv, "-position") == 0 && argc>1) {
			needMore(argc, argv);
			pos1.lat = atof(*++argv);
	                --argc;
			needMore(argc, argv);
			pos1.lon = atof(*++argv);
			mark1.city = &pos1;
			--argc;
		}
		else if (strcasecmp(*argv, "-city") == 0 && argc>1) {
			needMore(argc, argv);
			strcpy(CityInit, *++argv);
	                map_mode = COORDINATES;
			--argc;
		}
		else if (strcasecmp(*argv, "-citycolor0") == 0 && argc>1) {
			needMore(argc, argv);
			strcpy(CityColor0.name, *++argv);
			--argc;
		}
		else if (strcasecmp(*argv, "-citycolor1") == 0 && argc>1) {
			needMore(argc, argv);
			strcpy(CityColor1.name, *++argv);
			--argc;
		}
		else if (strcasecmp(*argv, "-citycolor2") == 0 && argc>1) {
			needMore(argc, argv);
			strcpy(CityColor2.name, *++argv);
			--argc;
		}
		else if (strcasecmp(*argv, "-markcolor1") == 0 && argc>1) {
			needMore(argc, argv);
			strcpy(MarkColor1.name, *++argv);
			--argc;
		}
		else if (strcasecmp(*argv, "-markcolor2") == 0 && argc>1) {
			needMore(argc, argv);
			strcpy(MarkColor2.name, *++argv);
			--argc;
		}
		else if (strcasecmp(*argv, "-linecolor") == 0 && argc>1) {
			needMore(argc, argv);
			strcpy(LineColor.name, *++argv);
			--argc;
		}
		else if (strcasecmp(*argv, "-tropiccolor") == 0 && argc>1) {
			needMore(argc, argv);
			strcpy(TropicColor.name, *++argv);
			--argc;
		}
		else if (strcasecmp(*argv, "-suncolor") == 0 && argc>1) {
			needMore(argc, argv);
			strcpy(SunColor.name, *++argv);
			--argc;
		}
		else if (strcasecmp(*argv, "-placement") == 0 && argc>1) {
			needMore(argc, argv);
			if (strcasecmp(*++argv, "random")==0)
                           placement = RANDOM;
			if (strcasecmp(*argv, "fixed")==0) {
                           placement = FIXED;
	                   MapGeom.mask = XValue | YValue;
	                   ClockGeom.mask = XValue | YValue;
			}
			if (strcasecmp(*argv, "center")==0)
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
		else if (strcasecmp(*argv, "-horizshift") == 0 && argc>1) {
			needMore(argc, argv);
                        horiz_shift = atoi(*++argv);
			--argc;
		}
		else if (strcasecmp(*argv, "-vertshift") == 0 && argc>1) {
			needMore(argc, argv);
                        vert_shift = atoi(*++argv);
			if (vert_shift<6) vert_shift = 6;
			--argc;
		}
		else if (strcasecmp(*argv, "-command") == 0 && argc>1) {
			needMore(argc, argv);
			Command = *++argv;
			--argc;
		}
		else if (((opt = (strcasecmp(*argv, "-progress") == 0)) ||
                          (strcasecmp(*argv, "-jump") == 0)) && argc>1) {
		        char *str, c;
			long value;
			needMore(argc, argv);
			value = atol(str=*++argv);
			c = str[strlen(str)-1];
                        if (c>='0' && c<='9') c='s';
			switch(c) {
			  case 's': break;
			  case 'm': value *= 60 ; break;
			  case 'h': value *= 3600 ; break;
			  case 'd': value *= 86400 ; break;
			  case 'M': value *= 2592000 ; break;
			  case 'y':
			  case 'Y': value *= 31536000 ; break;
			  default : c = ' '; break;
			}
			if (c == ' ') usage();
			if (opt) {
                           time_progress = value; 
                           time_progress_init = value; 
			} else 
			   time_jump = value;
			--argc;
		}
		else if (strcasecmp(*argv, "-clock") == 0)
			do_map = 0;
		else if (strcasecmp(*argv, "-map") == 0)
			do_map = 1;
		else if (strcasecmp(*argv, "-menu") == 0)
			do_menu = 0;
		else if (strcasecmp(*argv, "-nomenu") == 0)
			do_menu = 1;
		else if (strcasecmp(*argv, "-mono") == 0) {
                        strcpy(TextBgColor.name, "White");
                        spot_size = 3;
			mono = 1;
		}
		else if (strcasecmp(*argv, "-date") == 0)
                        clock_mode = 0;
		else if (strcasecmp(*argv, "-seconds") == 0)
                        clock_mode = 1;
		else if (strcasecmp(*argv, "-parallels") == 0) {
			do_parall = 1;
		}
		else if (strcasecmp(*argv, "-cities") == 0) {
			do_cities = 1;
		}
		else if (strcasecmp(*argv, "-night") == 0) {
			do_night = 1;
		}
		else if (strcasecmp(*argv, "-nonight") == 0) {
			do_night = 0;
		}
		else if (strcasecmp(*argv, "-sunpos") == 0) {
			do_sunpos = 1;
		}
		else if (strcasecmp(*argv, "-nomeridians") == 0) {
			do_merid = 0;
		}
		else if (strcasecmp(*argv, "-notropics") == 0) {
			do_tropics = 0;
		}
		else if (strcasecmp(*argv, "-noparallels") == 0) {
			do_parall = 0;
		}
		else if (strcasecmp(*argv, "-nocities") == 0) {
			do_cities = 0;
		}
		else if (strcasecmp(*argv, "-nosunpos") == 0) {
			do_sunpos = 0;
		}
		else if (strcasecmp(*argv, "-meridians") == 0) {
			do_merid = 1;
		}
		else if (strcasecmp(*argv, "-tropics") == 0) {
			do_tropics = 1;
		}
		else if (strcasecmp(*argv, "-version") == 0) {
			fprintf(stderr, "%s: version %s\n",
				ProgName, VERSION);
			if (cond) 
			  exit(0);
			else
			  return(0);
		} else {
                  if (cond)
		    usage();
		  else
		    return(1);
		}
	}
	return(0);
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
    char option[3][128]; /* Pointers to options */
    char *args[3];      /* Pointers to options */
    char *city, *lat, *lon, *tz; /* Information about a place */
    City *crec;		/* Pointer to new city record */
    int  first_step=1;  /* Are we parsing options in rc file ? */
    int  i, j;		/* indices */

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
     * Get the path to the rc file
     */

    if (rc_file) fname = rc_file;
        else
    if ((fname = tildepath("~/.sunclockrc")) == NULL) {
        fprintf(stderr, 
             "Unable to get path to ~/.sunclockrc\n");
        return(1);
    }

    
    /* Open the RC file */

    if ((rc = fopen(fname, "r")) == NULL) {
        char *def_name = app_default;
        if ((rc = fopen(def_name, "r")) == NULL) {
          fprintf(stderr, 
             "Unable to find %s or %s\n", fname, def_name);
	  return(1);
	} else {
	  if (fname == rc_file)
          fprintf(stderr, 
             "Unable to find %s, returning to app-default\n   %s\n", 
             fname, def_name);
	  fname = app_default;
	} 
    }

    /* Read and parse lines from the file */

    option[0][0] = '-';
    for (i=0; i<=2; i++) args[i] = option[i];

    while (fgets(buf, 128, rc)) {

        /* Look for blank lines or comments */

        if ((buf[0] == '#') || (buf[0] == '\0')) continue;

        if (strstr(buf, "[Cities]")) {
           first_step = 0;
	   continue;
	}

        if (strstr(buf, "[Options]")) {
	   first_step = 1;
	   continue;
	}

	if (first_step) {
	   i = sscanf(buf, "%s %s %s\n", option[0]+1, option[1], option[2]);
           j = strlen(option[0])-1;
	   if (i==1 || option[0][j]==':') {
           fflush(stdout);
	     if (option[0][j]==':') option[0][j]='\0';
	     i = parseArgs(i+1, args-1, 0);
	   }
	   if (i>0) {
	     fprintf(stderr, 
                     "Error in %s at line %s in [options] !!\n\n", 
                     fname, option[0]+1);
	     usage();
	   }
	   continue;
	}

	/* Get the city name looking for blank lines and comments */

	if (((city = strtok(buf, ":\n")) == NULL))
	    continue;

	/* Get the latitude, longitude and timezone */

	if ((lat = strtok(NULL, " 	\n")) == NULL) {
	    fprintf(stderr, "Error in %s for city %s\n", fname, city);
	    continue;
	}

	if ((lon = strtok(NULL, " 	\n")) == NULL) {
	    fprintf(stderr, "Error in %s for city %s\n", fname, city);
	    continue;
	}

	if ((tz = strtok(NULL, " 	\n")) == NULL) {
	    fprintf(stderr, "Error in %s for city %s\n", fname, city);
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

    /*
     * Read language files
     */

    fname = share_file;

    i = strlen(share_file);
       
    if (!*language && getenv("LANG"))
       strncpy(language, getenv("LANG"), 2);
    if (!*language)
       strcpy (language,"en");

    for (j=0; j<=1; j++) share_file[i+j-2] = tolower(language[j]);

    if ((rc = fopen(fname, "r")) != NULL) {
      int j=0, k=0, l=0, m=0, n=0, p;
      while (fgets(buf, 128, rc)) {
        if (buf[0] != '#') {
	        p = strlen(buf)-1;
        	if (p>=0 && buf[p]=='\n') buf[p] = '\0';
	        if (j<7) { strcpy(Day_name[j], buf); j++; } else
        	if (k<12) { strcpy(Month_name[k], buf); k++; } else
		if (l<L_END) { strcpy(Label[l], buf); l++; } else 
		if (m<=N_OPTIONS) { strcpy(Help[m], buf); m++; } else 
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
             "Unable to open language in %s\n", share_file);

    /* Done */

    return(0);
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
	XFreeGC(dpy, SunColor.gc);
	XFreeGC(dpy, BigFont_gc);
	XFreeGC(dpy, SmallFont_gc);
	XFreeFont(dpy, BigFont);
	XFreeFont(dpy, SmallFont);
	XFreePixmap(dpy, Mappix);
	XFreePixmap(dpy, Clockpix);
	XDestroyWindow(dpy, Map);
	XDestroyWindow(dpy, Clock);
	XCloseDisplay(dpy);
	exit(0);
}


/*
 * Set up stuff the window manager will want to know.  Must be done
 * before mapping window, but after creating it.
 */

void
setAllHints(num)
int				num;
{
	XSizeHints		xsh;
	Window			win = 0;

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

	   case 2:
		win = Menu;
		xsh.x = MapGeom.x + horiz_shift;
		xsh.y = (placement<=NE)? 
	           MapGeom.y + map_height + vert_shift : 
                   MapGeom.y - 2*map_strip_height - vert_shift;
		xsh.flags |= USPosition;
		xsh.width = xsh.min_width = xsh.max_width = map_icon_width;
		xsh.height = xsh.min_height = xsh.max_height =
                              2*map_strip_height;
		break;
	}

	if (!win) return;
	XSetNormalHints(dpy, win, &xsh);
}

void
setProtocols(num)
int				num;
{
	XClassHint		xch;
	Window			win = 0;
	int                     mask;
        char 			name[80];	/* Used to change icon name */

	mask = ExposureMask | ButtonPressMask | KeyPressMask;

        switch(num) {
	   case 0:
		win = Clock;
		break;

	   case 1:
		win = Map;
		break;

	   case 2:
		win = Menu;
		mask |= PointerMotionMask;
		break;
	}

	if (!win) return;

        sprintf(name, "%s %s", ProgName, VERSION);
	xch.res_name = ProgName;
	xch.res_class = "Sunclock";
        XSetIconName(dpy, win, name);
	XSetClassHint(dpy, win, &xch);
	XStoreName(dpy, win, ProgName);
       	XSelectInput(dpy, win, mask);
	XSetWMProtocols(dpy, win, &wm_delete_window, 1);
}

/*
 *  Routine used to adjust the relative placement of clock and map windows
 */

void
AdjustGeom(num)
int num;
{
        Window root = RootWindow(dpy, scr);
        Window win;

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
	  if (num) {
	    if (placement >= CENTER) {
      	      XTranslateCoordinates(dpy, Clock, root, 0, 0, 
                                       &ClockGeom.x, &ClockGeom.y, &win);
	      MapGeom.x = ClockGeom.x - dx - horiz_drift;
	      MapGeom.y = ClockGeom.y - dy - vert_drift;
              MapGeom.mask = XValue | YValue;
	    }
	  } else {
	    if (placement >= CENTER) {
      	      XTranslateCoordinates(dpy, Map, root, 0, 0, 
                                       &MapGeom.x, &MapGeom.y, &win);
	      ClockGeom.x = MapGeom.x + dx - horiz_drift;
	      ClockGeom.y = MapGeom.y + dy - vert_drift;
              ClockGeom.mask = XValue | YValue;
	    }
	  } 
	}
}

void
MoveWindow(num)
int num;
{
        if (num)
           XMoveWindow(dpy, Map, MapGeom.x, MapGeom.y);
        else
           XMoveWindow(dpy, Clock, ClockGeom.x, ClockGeom.y);
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
			     clock_icon_width, clock_height, 0,
			     CopyFromParent, InputOutput, 
			     CopyFromParent, mask, &xswa);
		break;

	   case 1:
		fixGeometry(&MapGeom, map_icon_width, map_height);
	        Map = XCreateWindow(dpy, root,
			     MapGeom.x, MapGeom.y,
			     map_icon_width, map_height, 0,
			     0, InputOutput, 
			     CopyFromParent, mask, &xswa);
		break;

           case 2:
	        Menu = XCreateWindow(dpy, root,
			     MapGeom.x + horiz_shift, 
                             MapGeom.y + map_height + vert_shift,
			     map_icon_width, 2*map_strip_height, 0,
			     0, InputOutput, 
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
	int i;
	struct geom * WinGeom;
        Window win;

        wm_protocols = XInternAtom(dpy, "WM_PROTOCOLS", False);
        wm_delete_window = XInternAtom(dpy, "WM_DELETE_WINDOW", False);

        map_strip_height = BigFont->max_bounds.ascent + 
                       BigFont->max_bounds.descent + 4;
	map_height = map_icon_height + map_strip_height;

        clock_strip_height = SmallFont->max_bounds.ascent +
	                     SmallFont->max_bounds.descent + 2;
	clock_height = clock_icon_height + clock_strip_height;

        for (i=0; i<=2; i++) {
	   createWindow(i);
	   setProtocols(i);
	   XSetCommand(dpy, (i==0)? Clock:((i==1)?Map:Menu), argv, argc);
	}

        setAllHints(do_map);
	AdjustGeom(do_map);
	win = (do_map)? Map:Clock;
        XMapWindow(dpy, win);
	if (!placement) return;
	XFlush(dpy);
        usleep(3*TIMESTEP);
	AdjustGeom(1-do_map);
	WinGeom = (do_map)? &MapGeom : &ClockGeom;
	horiz_drift = WinGeom->x;
	vert_drift = WinGeom->y;
	XMoveWindow(dpy, win, WinGeom->x, WinGeom->y);
	XFlush(dpy);
        usleep(3*TIMESTEP);
	AdjustGeom(1-do_map);
	horiz_drift = WinGeom->x - horiz_drift;
	vert_drift = WinGeom->y - vert_drift;
        /* Used for debugging this !#@ of window placement */
	/* printf("Placement: %d Drift %d %d\n", 
           placement, horiz_drift, vert_drift); */
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

	gcv.foreground = SunColor.pix;
	gcv.background = SunColor.pix;
	gcv.font = BigFont->fid;
	SunColor.gc = XCreateGC(dpy, Map, GCForeground | GCBackground | GCFont, &gcv);
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
	SunColor.pix = getColor(SunColor.name, white);
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
                 map_icon_width, map_strip_height, True);

	time_count = PRECOUNT;
	if (do_bottom) --do_bottom;
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

/*
 *  Set the timezone of selected location
 */

void
setTZ(cptr)
City	*cptr;
{
#ifndef linux
	char buf[80];
#endif

   	if (cptr && cptr->tz && do_map) {
#ifdef linux
           setenv("TZ", cptr->tz, 1);
#else
	   sprintf(buf, "TZ=%s", cptr->tz);
 	   putenv(buf);
#endif
	   } 
	else
#ifdef linux
           unsetenv("TZ");
#else
	   {
	   /* This is supposed to reset timezone to default localzone */
	   strcpy(buf, "TZ");
	   /* Another option would be to set LOCALZONE adequately and put:
           strcpy(buf, "TZ="LOCALZONE); */
	   putenv(buf);
	   }
#endif
	tzset();
}

/*
 *   Sets sun position (longitude, declination) and returns
 *   solartime at given city
 *   CAUTION: sunlong is in fact given as longitude+180 in interval 0..360
 */

time_t
sunParams(gtime, sunlong, sundec, city)
time_t gtime;
double *sunlong;
double *sundec;
City *city;
{
	struct tm		ctp, stp;
	time_t			stime;
	double                  jt, gt;
	double			sunra, sunrv;
	long                    diff;

	ctp = *gmtime(&gtime);
	jt = jtime(&ctp);
	sunpos(jt, False, &sunra, sundec, &sunrv, sunlong);
	gt = gmst(jt);
        *sunlong = fixangle(180.0 + (sunra - (gt * 15))); 
	
	if (city) {
           stime = (long) ((city->lon - *sunlong)*240.0);
           stp = *gmtime(&stime);
           diff = stp.tm_sec-ctp.tm_sec
                  +60*(stp.tm_min-ctp.tm_min)+3600*(stp.tm_hour-ctp.tm_hour);
	   if (city->lon>0.0) while(diff<-40000) diff += 86400;
	   if (city->lon<0.0) while(diff>40000) diff -= 86400;
	   stime = gtime+diff;
	} else
	   stime = 0;
	return(stime);
}

char *
num2str(value, string)
double value;
char *string;
{
	int eps, d, m, s;

	if (deg_mode) {
 	  if (value<0) {
	    value = -value; 
	    eps = -1;
	  } else
	    eps = 1;
	  d = (int) value;
	  value = 60 * (value - d);
	  m = (int) value;
	  value = 60 * (value - m);	  
	  s = (int) value;
	  sprintf(string, "%d°%02d'%02d\"", eps*d, m, s);
	} else
	  sprintf(string, "%.3f", value); 
	return string;
}

char *
bigtprint(gtime)
time_t gtime;
{
	register struct tm 	ltp;
	register struct tm 	gtp;
	register struct tm 	stp;
	time_t                  stime;
 	int			i, l;
	static char		s[128];
	char		slat[20], slon[20], slatp[20], slonp[20];
        double		dist;
#ifdef NEW_CTIME
	struct timeb		tp;

	if (ftime(&tp) == -1) {
		fprintf(stderr, "%s: ftime failed: ", ProgName);
		perror("");
		exit(1);
	}
#endif

	if (!do_map) return "";

        switch(map_mode) {

	case LEGALTIME:
           gtp = *gmtime(&gtime);
           setTZ(mark1.city);
	   ltp = *localtime(&gtime);
	   sprintf(s,
		" %s %02d:%02d:%02d %s %s %02d %s %04d    %s %02d:%02d:%02d UTC %s %02d %s %04d",
                Label[L_LEGALTIME], ltp.tm_hour, ltp.tm_min,
		ltp.tm_sec,
#ifdef NEW_CTIME
		ltp.tm_zone,
#else
		tzname[ltp.tm_isdst],
#endif
		Day_name[ltp.tm_wday], ltp.tm_mday,
		Month_name[ltp.tm_mon], 1900 + ltp.tm_year ,
                Label[L_GMTTIME],
		gtp.tm_hour, gtp.tm_min,
		gtp.tm_sec, Day_name[gtp.tm_wday], gtp.tm_mday,
		Month_name[gtp.tm_mon], 1900 + gtp.tm_year);
	   break;

	case COORDINATES:
           setTZ(mark1.city);
	   ltp = *localtime(&gtime);
           if ((mark1.city) && mark1.full)
	   sprintf(s,
		" %s (%s,%s)  %02d:%02d:%02d %s %s %02d %s %04d   %s %02d:%02d:%02d   %s %02d:%02d:%02d",
                mark1.city->name, 
                num2str(mark1.city->lat, slat), num2str(mark1.city->lon, slon),
		ltp.tm_hour, ltp.tm_min, ltp.tm_sec,
#ifdef NEW_CTIME
		ltp.tm_zone,
#else
		tzname[ltp.tm_isdst],
#endif
		Day_name[ltp.tm_wday], ltp.tm_mday,
		Month_name[ltp.tm_mon], 1900 + ltp.tm_year ,
                Label[L_SUNRISE],
		mark1.sr.tm_hour, mark1.sr.tm_min, mark1.sr.tm_sec,
                Label[L_SUNSET], 
		mark1.ss.tm_hour, mark1.ss.tm_min, mark1.ss.tm_sec);
	        else
           if ((mark1.city) && !mark1.full)
	   sprintf(s,
		" %s (%s,%s)  %02d:%02d:%02d %s %s %02d %s %04d   %s %02d:%02d:%02d",
                mark1.city->name, 
                num2str(mark1.city->lat, slat), num2str(mark1.city->lon, slon),
		ltp.tm_hour, ltp.tm_min, ltp.tm_sec,
#ifdef NEW_CTIME
		ltp.tm_zone,
#else
		tzname[ltp.tm_isdst],
#endif
		Day_name[ltp.tm_wday], ltp.tm_mday,
		Month_name[ltp.tm_mon], 1900 + ltp.tm_year ,
                Label[L_DAYLENGTH],
		mark1.dl.tm_hour, mark1.dl.tm_min, mark1.dl.tm_sec);
	        else
                  sprintf(s," %s", Label[L_CLICKCITY]);
	        break;

	case SOLARTIME:
	   if (mark1.city) {
	     double junk;
	     stime = sunParams(gtime, &junk, &junk, mark1.city);
	     stp = *gmtime(&stime);
	     sprintf(s, " %s (%s,%s)  %s %02d:%02d:%02d   %s %02d %s %04d   %s %02d:%02d:%02d", 
                  mark1.city->name, 
                  num2str(mark1.city->lat,slat), num2str(mark1.city->lon,slon),
                  Label[L_SOLARTIME],
                  stp.tm_hour, stp.tm_min, stp.tm_sec,
                  Day_name[stp.tm_wday], stp.tm_mday,
		  Month_name[stp.tm_mon], 1900 + stp.tm_year,
                  Label[L_DAYLENGTH], 
 		  mark1.dl.tm_hour, mark1.dl.tm_min, mark1.dl.tm_sec);
           } else
                  sprintf(s," %s", Label[L_CLICKLOC]);
	   break;

	case DISTANCES:
	   if(mark1.city && mark2.city) {
             dist = sin(dtr(mark1.city->lat)) * sin(dtr(mark2.city->lat))
                    + cos(dtr(mark1.city->lat)) * cos(dtr(mark2.city->lat))
                           * cos(dtr(mark1.city->lon-mark2.city->lon));
             dist = acos(dist);
             sprintf(s, " %s (%s,%s) --> %s (%s,%s)     "
                      "%d km  =  %d miles", 
               mark2.city->name, 
               num2str(mark2.city->lat,slatp), num2str(mark2.city->lon, slonp),
               mark1.city->name, 
               num2str(mark1.city->lat, slat), num2str(mark1.city->lon, slon),
               (int)(EARTHRADIUS_KM*dist), (int)(EARTHRADIUS_ML*dist));
	   } else
	     sprintf(s, " %s", Label[L_CLICK2LOC]);
	   break;

	case EXTENSION:
	   return "";
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
smalltprint(gtime)
time_t gtime;
{
        register struct tm	ltp;
	int			i, l;
	static char		s[80];

	if (do_map) return "";

	setTZ(NULL);
	ltp = *localtime(&gtime);

	if (clock_mode)
	   sprintf(s, "%02d:%02d:%02d %s", 
                ltp.tm_hour, ltp.tm_min, ltp.tm_sec,
#ifdef NEW_CTIME
		ltp.tm_zone);
#else
		tzname[ltp.tm_isdst]);
#endif
        else
	   sprintf(s, "%02d:%02d %s %02d %s %02d", ltp.tm_hour, ltp.tm_min,
		Day_name[ltp.tm_wday], ltp.tm_mday, Month_name[ltp.tm_mon],
		ltp.tm_year % 100);

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
makeMapContexts()
{
	register struct sunclock * s;

	s = makeMapContext(map_icon_width, map_icon_height, Map, Mappix,
		     BigFont_gc, bigtprint, 4,
		     map_icon_height + BigFont->max_bounds.ascent + 2);
	Current = s;

	s = makeMapContext(clock_icon_width, clock_icon_height, 
		     Clock, Clockpix, SmallFont_gc, smalltprint, 4,
		     clock_icon_height + SmallFont->max_bounds.ascent + 1);
	Current->s_next = s;
	s->s_flags |= S_CLOCK;
	s->s_next = Current;
}

void
SwitchWindows()
{
	time_count = PRECOUNT;

        do_map = 1-do_map;
        AdjustGeom(do_map);
	XUnmapWindow(dpy,(do_map)?Clock:Map);
        setAllHints(do_map);
	MoveWindow(do_map);
        XMapWindow(dpy, (do_map)?Map:Clock);
        MoveWindow(do_map);
	if (do_map) switched = 1;
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
    int rad;
    GC gc;
    Window w;

    if (mode < 0) return;
    if (!do_cities && mode <= 2) return;

    if (mode == 0) { gc = CityColor0.gc; --mode; }
    if (mode == 1)   gc = CityColor1.gc;
    if (mode == 2)   gc = CityColor2.gc;
    if (mode == 3)   gc = MarkColor1.gc;
    if (mode == 4)   gc = MarkColor2.gc;
    if (mode == 5)   gc = SunColor.gc;

    checkMono(&w, &gc);

    ilat = map_icon_height - (lat + 90) * (map_icon_height / 180.0);
    ilon = (180.0 + lon) * (map_icon_width / 360.0);

    rad = radius[spot_size];
    if (spot_size == 1)
       XDrawArc(dpy, w, gc, ilon-rad, ilat-rad, 2*rad, 2*rad, 0, 360 * 64);
    if (spot_size == 2)
       XFillArc(dpy, w, gc, ilon-rad, ilat-rad, 2*rad, 2*rad, 0, 360 * 64);    
    if (spot_size == 3)
        {
        XDrawArc(dpy, w, gc, ilon-rad, ilat-rad, 2*rad, 2*rad, 0, 360 * 64);
        XFillArc(dpy, w, gc, ilon-3, ilat-3, 6, 6, 0, 360 * 64);
        }
    if (mode == 5)
        {
        rad = 6 + 2*mono;
	XDrawLine(dpy, w, gc, ilon, ilat-rad, ilon, ilat+rad);
	XDrawLine(dpy, w, gc, ilon-rad, ilat, ilon+rad, ilat);
	}
}

void
drawCities()
{
City *c;
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

/*
 * draw_sun() - Draw Sun at position where it stands at zenith
 */

void
draw_sun()
{
	placeSpot(5, sun_decl, sun_long);
}

void
drawSun()
{
	if (do_sunpos)
  	  draw_sun();
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
	drawSun();
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

	if (do_night)
   	   for (i = 0; i < ydots; i++)
		wtab[i] = -1;
	else {
   	   for (i = 0; i < ydots; i++)
		wtab[i] = ydots;
	   return;
	}

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

/*
 *  Produce bottom strip of hours
 */

void
show_hours()
{
	int i;
	char s[128];

        clearStrip();
	for (i=0; i<24; i++) {
	    sprintf(s, "%d", i); 
   	    XDrawImageString(dpy, Map, BigFont_gc, 
              ((i*map_icon_width)/24 + 2*map_icon_width - CHWID*strlen(s)/8 +
               (int)(sun_long*map_icon_width/360.0))%map_icon_width +1,
              BigFont->max_bounds.ascent + map_icon_height + 2, 
              s, strlen(s));
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
	short *			wtab_swap;

	/* If this is a full repaint of the window, force complete
	   recalculation. */

        if (mono && first_time) {
	  drawAll();
	  first_time = 0;
	}

	if (s->s_noon < 0) {
		s->s_projtime = 0;
		for (i = 0; i < s->s_height; i++) {
			s->s_wtab1[i] = -1;
		}
	}

	if (s->s_flags & S_FAKE) {
		if (s->s_flags & S_ANIMATE)
			s->s_time += s->s_increm;
		if (s->s_time < 0)
			s->s_time = 0;
	} else
		time(&s->s_time);
	
  	(void) sunParams(s->s_time + time_jump, &sun_long, &sun_decl, NULL);
	xl = sun_long * (s->s_width / 360.0);
	sun_long -= 180.0;

	/* Projecting the illumination curve  for the current seasonal
           instant is costly.  If we're running in real time, only  do
	   it every PROJINT seconds.  */

	if ((s->s_flags & S_FAKE)
	 || s->s_projtime < 0
	 || (s->s_time - s->s_projtime) > PROJINT 
         || force_proj) {
		projillum(s->s_wtab, s->s_width, s->s_height, sun_decl);
		wtab_swap = s->s_wtab;
		s->s_wtab = s->s_wtab1;
		s->s_wtab1 = wtab_swap;
		s->s_projtime = s->s_time;
	}

	/* If the subsolar point has moved at least one pixel, update
	   the illuminated area on the screen.	*/

	if ((s->s_flags & S_FAKE) || s->s_noon != xl || force_proj) {
		moveterm(s->s_wtab1, xl, s->s_wtab, s->s_noon, s->s_width,
			 s->s_height, s->s_pixmap);
                if (mono && do_sunpos) draw_sun();
		s->s_noon = xl;
		s->s_flags |= S_DIRTY;
		force_proj = 0;
                if (map_mode == EXTENSION) show_hours();
                if (mono && do_sunpos) draw_sun();
	}
}

void
showText(s)
register struct sunclock *	s;
{
	XDrawImageString(dpy, s->s_window, s->s_gc, s->s_textx,
			 s->s_texty, s->s_text, strlen(s->s_text));
}

void
showImage(s)
register struct sunclock *	s;
{
	register char *		p;
	time_t                  gtime;

        gtime = s->s_time + time_jump;

	p = (*s->s_tfunc)(gtime);

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
dayLength(gtime, lat)
time_t  gtime;
double	lat;
{
	double			duration;
	double			sundec, junk;
	double                  sinsun, sinpos, sinapp, num;

        sinpos = sin(dtr(lat));

        /* Get Sun declination */
        (void) sunParams(gtime, &junk, &sundec, NULL);
        sinsun = sin(dtr(sundec));

        /* Correct for the sun apparent diameter and atmospheric diffusion */
        sinapp = sin(dtr(SUN_APPRADIUS + ATM_CORRECTION));

        num = 1 - sinsun*sinsun - sinpos*sinpos - sinapp*sinapp
                - 2*sinsun*sinpos*sinapp;
        if (num<=0) {
           if (sinsun*sinpos>0) 
	     duration = 24.0;
	   else
	     duration = 0.0;
        } else
             duration = 12.0 + 24.0*atan((sinsun*sinpos+sinapp)/sqrt(num))/PI;

        return duration*3600;
}

void
setDayParams(cptr)
City *cptr;
{
        struct tm               gtm, ltm;
	double                  duration, junk;
	time_t			gtime, stime, sr, ss, dl;

        time_count = PRECOUNT;
	force_proj = 1;
        last_hint = -1; 

	if (!cptr) return;

  	gtime = Current->s_time + time_jump;

        /* Get local time at given location */
	setTZ(cptr);
        ltm = *localtime(&gtime);

        /* Get solar time at given location */
        stime = sunParams(gtime, &junk, &junk, cptr);

	/* Go to time when it is noon in solartime at cptr */
        gtime += 43200 - (stime % 86400);
        gtm = *gmtime(&gtime);

        if (gtm.tm_mday < ltm.tm_mday) gtime +=86400;
        if (gtm.tm_mday > ltm.tm_mday) gtime -=86400;
       
        /* Iterate, just in case of a day shift */
        stime = sunParams(gtime, &junk, &junk, cptr);
        gtime += 43200 - (stime % 86400);

        /* get day length at that time and location*/
        duration = dayLength(gtime, cptr->lat);
  
	/* compute sunrise and sunset in legal time */
	sr = gtime - (time_t)(0.5*duration);
	ss = gtime + (time_t)(0.5*duration);
        dl = ss-sr;
        mark1.full = 1;
	if (dl<=0) {dl = 0; mark1.full = 0;}
	if (dl>86380) {dl=86400; mark1.full = 0;}

	mark1.dl = *gmtime(&dl);
	mark1.dl.tm_hour += (mark1.dl.tm_mday-1) * 24;

	setTZ(cptr);
	mark1.sr = *localtime(&sr);
	mark1.ss = *localtime(&ss);
}

/*
 * processPoint() - This is kind of a cheesy way to do it but it works. What happens
 *                  is that when a different city is picked, the TZ environment
 *                  variable is set to the timezone of the city and then tzset().
 *                  is called to reset the system.
 */

void
processPoint(win, x, y)
Window win;
int x, y;      /* Screen co-ordinates of mouse */
{
    /*
     * Local Variables
     */

    City *cptr;    /* Used to search for a city */
    int cx, cy;    /* Screen coordinates of the city */

    /* Loop through the cities until on close to the pointer is found */

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
      case EXTENSION:
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

void
drawSeparator(rank, width)
int	rank, width;
{
	int j;

	for (j=CHWID*rank-width+CHWID; j<=CHWID*rank+CHWID+width; j++)
	     XDrawLine(dpy, Menu, BigFont_gc, j, 0, j, map_strip_height);
}

void
PopMenu()
{
	int    x, y;
	
	if (!do_map) {
	  do_menu = 0;
	  return;
	}

	do_menu = 1 - do_menu;

        if (!do_menu) 
	  {
	  XUnmapWindow(dpy, Menu);
	  return;
	  }

        last_hint = -1;
        AdjustGeom(0);
        setAllHints(2);
	x = MapGeom.x + horiz_shift - horiz_drift;
	y = (placement<=NE)? 
	    MapGeom.y + map_height + vert_shift: 
            MapGeom.y - 2*map_strip_height - vert_shift -2*vert_drift;
        XMoveWindow(dpy, Menu, x, y);
        XMapWindow(dpy, Menu);
        XMoveWindow(dpy, Menu, x, y);
}

void
showMenuHint(num)
int num;
{
	char hint[128], more[128];
	int i,j,k,l;

	if (num == last_hint) return;
        last_hint = num;
        sprintf(hint, " %s", Help[num]); 
        *more = '\0';
	if (num<=2)
	    sprintf(more, " (%s)", Label[L_DEGREE]);
	if (num >=5 && num <=8) {
	    char prog_str[30];
            if (time_progress == 60) 
                sprintf(prog_str, "1 %s", Label[L_MIN]);
	    else
	    if (time_progress == 3600)
                sprintf(prog_str, "1 %s", Label[L_HOUR]);
	    else
	    if (time_progress == 86400) 
                sprintf(prog_str, "1 %s", Label[L_DAY]);
	    else
	    if (time_progress == 604800) 
                sprintf(prog_str, "7 %s", Label[L_DAYS]);
	    else
	    if (time_progress == 2592000) 
                sprintf(prog_str, "30 %s", Label[L_DAYS]);
      	    else
                sprintf(prog_str, "%ld %s", time_progress, Label[L_SEC]);
            sprintf(more, " ( %s %c%s   %s %.3f %s )", 
		  Label[L_PROGRESS], (num==5)? '-':'+', prog_str, 
                  Label[L_TIMEJUMP], time_jump/86400.0,
		  Label[L_DAYS]);
	}
	if (Option[2*num] == 'H') {
	    sscanf(RELEASE, "%d %d %d", &i, &j, &k);
	    sprintf(more, " ( sunclock %s, %d %s %d )", 
                        VERSION, i, Month_name[j-1], k);
	}
	if (Option[2*num] == 'X') {
	    sprintf(more, " : %s", (Command)?Command:"(null)");
	}
        if (*more) strncat(hint, more, 120 - strlen(hint));
        l = strlen(hint);
	if (l<120) {
	    for (i=l; i<120; i++) hint[i] = ' ';
	    hint[120] = '\0';
	}
	XDrawImageString(dpy, Menu, BigFont_gc, 4, 
              BigFont->max_bounds.ascent + map_strip_height + 2, 
              hint, strlen(hint));
}

/*
 *  Process key events in eventLoop
 */

void
processKey(win, key)
Window  win;
char	key;
{
	int i, old_mode;

        time_count = PRECOUNT;

	old_mode = map_mode;

        switch(key) {
	   case '\033':
	     if (do_menu) PopMenu();
	     break;
	   case 'P':
	     label_shift = 0;
	     break;
	   case 'W':
	     label_shift = 30;
	     break;
	   case 'Q': 
	     if (label_shift<30)
             ++label_shift;
	     break;
	   case 'S': 
	     if (label_shift>0)
	       --label_shift;
	     break;
	   case 'a': 
	     time_jump += time_progress;
	     setDayParams(mark1.city);
	     break;
	   case 'b': 
	     time_jump -= time_progress;
	     setDayParams(mark1.city);
	     break;
	   case 'c': 
	     if (map_mode != COORDINATES) 
	       deg_mode = 0;
	     else
	       deg_mode = 1 - deg_mode;
	     map_mode = COORDINATES;
	     if (mark1.city == &pos1 || mark2.city == &pos2) updateMap();
             if (mark1.city == &pos1) mark1.city = NULL;
	     if (mark1.city)
               setDayParams(mark1.city);
             if (mark2.city) mark2.city->mode = 0;
             mark2.city = NULL;
	     break;
	   case 'd': 
	     if (map_mode != DISTANCES) 
	       deg_mode = 0;
	     else
	       deg_mode = 1 - deg_mode;
	     map_mode = DISTANCES;
	     break;
	   case 'e': 
	     map_mode = EXTENSION;
	     old_mode = EXTENSION;
	     show_hours();
	     break;
	   case 'g': 
	     if (!do_map) break;
	     if (!do_menu)
	       PopMenu();
             else {
	       last_hint = -1; 
	       if (time_progress == 60) time_progress = 3600;
	         else
	       if (time_progress == 3600) time_progress = 86400;
	         else
	       if (time_progress == 86400) time_progress = 604800;
	         else
	       if (time_progress == 604800) time_progress = 2592000;
       	         else 
	       if (time_progress == 2592000) {
		 if (time_progress_init)
		    time_progress = time_progress_init;
		 else
	            time_progress = 60;
	       } else
		    time_progress = 60;
	     }
	     break;
	   case 'h':
	     if (!do_menu) PopMenu();
	     break;
	   case 'i': 
	     if (do_map && do_menu) PopMenu();
             XIconifyWindow(dpy, (do_map)? Map:Clock, scr);
	     break;
	   case 'u':
	     if (mono && do_cities) drawCities();
	     do_cities = 1 - do_cities;
	     if (mono && do_cities) drawCities();
             if (mono || !do_cities) exposeMap();
	   case 'l':
	     map_mode = LEGALTIME;
             if (mark1.city == &pos1 || mark2.city == &pos2) updateMap();
             if (mark1.city) mark1.city->mode = 0;
             if (mark2.city) mark2.city->mode = 0;
             mark1.city = NULL;
             mark2.city = NULL;
	     break;
	   case 'm':
             do_merid = 1 - do_merid;
	     if (mono) draw_meridians();
	     if (mono || !do_merid) exposeMap();
	     break;
	   case 'n':
	     do_night = 1 -do_night;
	     force_proj = 1;
	     exposeMap();
	     break;
	   case 'o':
             do_sunpos = 1 - do_sunpos;
	     if (mono) draw_sun();
	     if (mono || !do_sunpos) exposeMap();
             break;
	   case 'p':
             do_parall = 1 - do_parall;
	     if (mono) draw_parallels();
	     if (mono || !do_parall) exposeMap();
	     exposeMap();
	     break;
	   case 'q': 
	     shutDown();
	   case 's': 
	     if (map_mode != SOLARTIME) 
	       deg_mode = 0;
	     else
	       deg_mode = 1 - deg_mode;
	     map_mode = SOLARTIME;
             if (mark2.city == &pos2) exposeMap();
             if (mark2.city) mark2.city->mode = 0;
             mark2.city = NULL;
	     if (mark1.city)
	       setDayParams(mark1.city);
	     break;
	   case 't':
             do_tropics = 1 - do_tropics;
	     if (mono) draw_tropics();
             if (mono || !do_tropics) exposeMap();
	     break;
	   case ' ':
	   case 'w':
	     if (do_map && do_menu) PopMenu();
	     SwitchWindows();
	   case 'r':
	     clearStrip();
	     updateMap();
	     break;
	   case 'x':
	     if (Command) system(Command);
	     break;
	   case 'y':
	     AdjustGeom(1-do_map);
	     setAllHints(1-do_map);
	     break;
	   case 'z':
	     time_jump = 0;
	     setDayParams(mark1.city);
	     break;
	   case '*':
	     deg_mode = 1 -deg_mode;
	     break;
           default:
	     if (!do_map) {
	       clock_mode = 1-clock_mode;
	       updateMap();
	     }
	     break ;
	}

        if (old_mode == EXTENSION && map_mode != old_mode) clearStrip();

	if (do_menu) {
          for (i=0; i<N_OPTIONS; i++)
	      if (key == Option[2*i]+32) showMenuHint(i);
	}
}

/*
 *  Process mouse events in eventLoop
 */

/*
 *  Process mouse motion events in eventLoop
 */

void
processMotion(win, x, y)
Window  win;
int	x, y;
{
int	click_pos;

	if (win != Menu) return;
	if (y>map_strip_height) return;
        click_pos = x/CHWID;
	if (click_pos >= N_OPTIONS) click_pos = N_OPTIONS;
        showMenuHint(click_pos);
}

void
processClick(win, x, y)
Window  win;
int	x, y;
{
char	key;
int	click_pos;

	if (win == Clock) {
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
        if (win == Map) {
          if(y > map_icon_height) {
	    PopMenu();
            return;
	  }

          /* Erase bottom strip to clean-up spots overlapping bottom strip */
	  if (do_bottom) clearStrip();

          /* Set the timezone, marks, etc, on a button press */
          processPoint(win, x, y);

          /* if spot overlaps bottom strip, set flag */
 	  if (y >= map_icon_height - radius[spot_size]) {
	    if (map_mode == SOLARTIME)
              do_bottom = 1;
	    if (map_mode == DISTANCES)
              do_bottom = 2;
	  }
	}

        if (win == Menu) {
	   if (y>map_strip_height) return;
           click_pos = x/CHWID;
	   if (click_pos >= N_OPTIONS) {
	      PopMenu();
	      return;
	   }
           key = Option[2*click_pos]+32;
	   processKey(win, key);
	}
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
	char s[128];
	int i;

        if (w == Menu) 
          {
          sprintf(s, "%s        %s", ListOptions, Label[L_ESCAPE]);
	  XDrawImageString(dpy, Menu, BigFont_gc, 4, 
              BigFont->max_bounds.ascent + 2, s, strlen(s));
          sprintf(s, " %s", Label[L_CONTROLS]);
	  XDrawImageString(dpy, Menu, BigFont_gc, 4, 
              BigFont->max_bounds.ascent + map_strip_height+ 2, s, strlen(s));
	  for (i=0; i<N_OPTIONS; i++)
	      drawSeparator(i, (Option[2*i+1]==';'));
          XDrawLine(dpy, Menu, BigFont_gc, 0, map_strip_height, 
                                         map_icon_width, map_strip_height);
	  return;
	  }

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
	                        if (map_mode == EXTENSION && switched) 
                                        show_hours();
				switched = 0;
				if (ev.xexpose.count == 0)
					doExpose(ev.xexpose.window);
				break;

			case ClientMessage:
				if (ev.xclient.message_type == wm_protocols &&
				    ev.xclient.format == 32 &&
				    ev.xclient.data.l[0] == wm_delete_window) {
					if (ev.xexpose.window==Menu)
					   PopMenu();
					else
					   shutDown();
				}
				break;

			case KeyPress:
                                key = XKeycodeToKeysym(dpy,ev.xkey.keycode,0);
				processKey(ev.xexpose.window, key & 255);
                                break;

			case ButtonPress:
			        processClick(ev.xexpose.window, 
                                             ev.xbutton.x, ev.xbutton.y);
				break;

			case MotionNotify:
			        processMotion(ev.xexpose.window, 
                                              ev.xbutton.x, ev.xbutton.y);
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

	if (*CityInit)
        for (c = cities; c; c = c->next)
	    if (!strcasecmp(c->name, CityInit)) {
		mark1.city = c;
                if (mono) mark1.status = -1;
		c->mode = 1;
	        doExpose(Map);
		setDayParams(c);
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
		setDayParams(&pos1);
        }	
}

int
main(argc, argv)
int				argc;
register char **		argv;
{
	char *			p;

	ProgName = *argv;

	/* Set default values */
        initValues();

	/* Read the configuation file */

        checkRCfile(argc, argv);
	if (readrc()) exit(1);

	if ((p = strrchr(ProgName, '/'))) ProgName = ++p;

	(void) parseArgs(argc, argv, 1);
	if (placement<0) placement = NW;

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
	PopMenu();
	eventLoop();
	exit(0);
}
