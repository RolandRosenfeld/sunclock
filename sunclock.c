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
       3.20  11/20/00  Drastic GUI and features improvements, now hopefully
                       sunclock looks nice...
*/

#include <unistd.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <sys/stat.h>
#include <string.h>

#include "sunclock.h"
#include "langdef.h"
#include <X11/xpm.h>

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

extern char *   salloc();
extern void     makePixmap();

extern char **get_dir_list();
extern int dup_strcmp();
extern void free_dirlist();

extern int      fill_mode;
/*
 * bits in flags
 */

#define	S_FAKE		01	/* date is fake, don't use actual time */
#define	S_ANIMATE	02	/* do animation based on increment */
#define	S_DIRTY		04	/* pixmap -> window copy required */

char share_i18n[] = SHAREDIR"/i18n/Sunclock.**";
char app_default[] = APPDEF"/Sunclock";
char *share_maps_dir = SHAREDIR"/earthmaps/";
char image_dir[1024];

char *BigFont_name = BIGFONT;
char *SmallFont_name = SMALLFONT;

char *freq_filter = ".xpm";
char **dirtable;

char language[4] = "";

char *	ProgName;
char *  Command = NULL;
char *rc_file = NULL;

Color	TextBgColor, TextFgColor, DirColor,
        LandColor, WaterColor, ArcticColor, 
	CityColor0, CityColor1, CityColor2,
	MarkColor1, MarkColor2, LineColor, TropicColor, SunColor;

char *		Display_name = "";

char *          MapXpm_file = NULL;
char *          MapXpm_sel = NULL;
char *          ClockXpm_file = NULL;
char *          ClockXpm_sel = NULL;

Display *	dpy;
int		scr;
Colormap        cmap;

Pixel           *pixel_dark=0;
unsigned long	black;
unsigned long	white;

Atom		wm_delete_window;
Atom		wm_protocols;

XFontStruct *	SmallFont;
XFontStruct *	BigFont;

GC		Store_gc;
GC              Invert_gc;
GC		BigFont_gc;
GC		DirFont_gc;
GC		SmallFont_gc;

Window		Map=0;
Window		Menu=0;
Window		Selfile=0;
Pixmap		Mappix;

XImage          *MapXIM=0, *ClockXIM=0, *CurXIM;
XpmAttributes   MapAttrib, ClockAttrib, *Attributes;


char            *ImageData = 0, *CurData;

struct geom	MapGeom =   { 0, 30,  30, 720, 360, 320, 160, 20 };
struct geom	ClockGeom = { 0, 30,  30, 128,  64,  48,  24, 20 };
struct geom	MenuGeom =  { 0, 30, 430, 700,  40, 700,  40,  0 };
struct geom	SelGeom =   { 0, 30,  40, 600, 180, 450,  80,  0 };

struct earthmap	Earthmap;

int             radius[4] = {0, 2, 3, 5};

int             spot_size = 2;
int             dotted = 0;
int		placement = -1;
int             mono = 0;
int             invert = 0;
int             leave;
int             pix_type;
int      	chwidth;
int		num_lines;
int		color_scale = 16;
int		shading;
int		shading_level = 1;
int		range;
int		allocation_level;

int             horiz_shift = 0;
int             vert_shift = 12;
int		label_shift = 0;
int		selfile_shift = 0;

int             horiz_drift = 0;
int             vert_drift =  0;

int             former_width;
int             former_height;
int             did_resize = 0;
int             num_table_entries;

int             time_count = PRECOUNT;
int		force_proj = 0;
int             first_drawall = 1;
int             not_firsttime = 0;
int             last_hint = -1;
int             hours_shown = 0;

int             do_map = 0;
int             do_selfile = 0;
int             do_hint = 0;
int		do_menu = 0;
int             do_memory = 0;
int             do_private = 0;

int		do_sunpos = 1;
int		do_cities = 1;

int		do_parall = 0;
int		do_tropics = 0;
int		do_merid = 0;
int		do_bottom = 0;

int		clock_mode = 0;
int             deg_mode = 0;
int             dms_mode = 0;
double		darkness = 0.5;
double		atm_refraction = ATM_REFRACTION;
double		atm_diffusion = ATM_DIFFUSION;

int             local_day_old = -1;
int             solar_day_old = -1;

char            map_mode = LEGALTIME;
char		CityInit[80] = "";

long		time_jump = 0;
long		time_progress = 60;
long		time_progress_init = 0;

double		sun_long;
double		sun_decl;

/* Records to hold extra marks 1 and 2 */

City pos1, pos2, *cities = NULL;
Mark mark1, mark2;

void
usage()
{
     fprintf(stderr, "%s: version %s, %s\nUsage:\n"
     "%s [-help] [-listmenu] [-version] [-language name]\n"
     SP"[-display name] [-bigfont name] [-smallfont name] \n"
     SP"[-mono] [-invert] [-private] [-usememory]\n"
     SP"[-placement (random, fixed, center, NW, NE, SW, SE)]\n"
     SP"[-rcfile file] [-sharedir directory]\n"
     SP"[-mapimage file.xpm(.gz)] [-clockimage file.xpm(.gz)]\n"
     SP"[-clock] [-date] [-seconds] [-command string]\n"
     SP"[-map] [-mapgeom +x+y] [-mapmode * <L,C,S,D,E>] [-decimal] [-dms]\n"
     SP"[-city name] [-position latitude longitude]\n"
     SP"[-jump number[s,m,h,d,M,Y]] [-progress number[s,m,h,d,M,Y]]\n"
     SP"[-night] [-nonight] [-shading mode=0,1,2,3,4]\n"
     SP"[-darkness value<=1.0] [-colorscale number<=64]\n"
     SP"[-diffusion value] [-refraction value]\n"
     SP"[-menu] [-nomenu] [-horizshift h (map<->menu)] [-vertshift v]\n"
     SP"[-coastlines] [-contour] [-landfill] [-fillmode number=0,1,2]\n"
     SP"[-sun] [-nosun] [-cities] [-nocities] [-meridians] [-nomeridians]\n"
     SP"[-parallels] [-noparallels] [-tropics] [-notropics]\n"
     SP"[-spotsize size(0,1,2,3)] [-dottedlines] [-plainlines]\n"
     SP"[-landcolor color] [-watercolor color] [-arcticcolor color]\n"
     SP"[-citycolor0 color] [-citycolor1 color] [-citycolor2 color]\n"
     SP"[-markcolor1 color] [-markcolor2 color] [-suncolor color]\n"
     SP"[-linecolor color] [-tropiccolor color] [-dircolor color]\n"
     SP"[-textbgcolor color] [-textfgcolor color]\n\n", 
        ProgName, VERSION, COPYRIGHT, ProgName);
}

void
listmenu()
{
        int i;

	fprintf(stderr, "%s\n\n", ShortHelp);
	for (i=0; i<N_OPTIONS; i++)
	fprintf(stderr, "%s %c : %s\n", Label[L_KEY], Option[2*i], Help[i]);
        fprintf(stderr, "\n");
}

void
initValues()
{
        strcpy(image_dir, share_maps_dir);
	strcpy(TextBgColor.name, "Grey92");
	strcpy(TextFgColor.name, "Black");
	strcpy(DirColor.name, "Blue");
	strcpy(LandColor.name, "Chartreuse2");
	strcpy(WaterColor.name, "RoyalBlue");
	strcpy(ArcticColor.name, "LemonChiffon");
	strcpy(CityColor0.name, "Orange");
	strcpy(CityColor1.name, "Red");
	strcpy(CityColor2.name, "Red3");
	strcpy(MarkColor1.name, "Pink1");
	strcpy(MarkColor2.name, "Pink2");
	strcpy(LineColor.name, "White");
	strcpy(TropicColor.name, "White");
	strcpy(SunColor.name, "Yellow");
	mark1.city = NULL;
	mark1.status = 0;
	mark2.city = NULL;
	mark2.status = 0;
}

void
needMore(argc, argv)
register int			argc;
register char **		argv;
{
	if (argc == 1) {
		fprintf(stderr, "%s: option `%s' requires an argument\n\n",
			ProgName, *argv);
		usage();
		if (leave) 
                        exit(1);
		else
		   fprintf(stderr, 
                        "Error in config file %s. "RECOVER, *argv);
	}
}

void
getGeom(s, g)
register char *			s;
register struct geom *		g;
{
	register int		mask;

	mask = XParseGeometry(s, &g->x, &g->y, &g->width, &g->height);
	if (mask == 0) {
		fprintf(stderr, "%s: `%s' is a bad geometry specification\n",
			ProgName, s);
		exit(1);
	}
	if (g->width<g->w_mini) g->width = g->w_mini;
        if (g->height<g->h_mini) g->height = g->h_mini;
	g->mask = mask;
	if ( placement<=RANDOM && (mask & ( XValue | YValue)) ) 
	   placement = FIXED;
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

char *
RCAlloc(s)
char * s;
{
       char *t;
       if (leave || !s) return s;
       t = (char *) salloc((strlen(s)+1)*sizeof(char));
       strcpy(t, s);
       return t;
}

int
parseArgs(argc, argv)
register int			argc;
register char **		argv;
{
	int	opt;

	while (--argc > 0) {
		++argv;
		if (strcasecmp(*argv, "-display") == 0) {
			needMore(argc, argv);
			Display_name = RCAlloc(*++argv); 
			--argc;
		} 
		else if (strcasecmp(*argv, "-bigfont") == 0) {
			needMore(argc, argv);
			BigFont_name = RCAlloc(*++argv); 
			--argc;
		} 
		else if (strcasecmp(*argv, "-smallfont") == 0) {
			needMore(argc, argv);
			SmallFont_name = RCAlloc(*++argv); 
			--argc;
		} 
		else if (strcasecmp(*argv, "-language") == 0) {
			needMore(argc, argv);
			strncpy(language, *++argv, 2);
			--argc;
		} 
                else if (strcasecmp(*argv, "-rcfile") == 0) {
			needMore(argc, argv);
			++argv;  /* already done in checkRCfile */
			--argc;
		}
                else if (strcasecmp(*argv, "-sharedir") == 0) {
			needMore(argc, argv);
			share_maps_dir = RCAlloc(*++argv);
			strncpy(image_dir, *argv, 1020);
			--argc;
		}
                else if (strcasecmp(*argv, "-mapimage") == 0) {
			needMore(argc, argv);
			MapXpm_file = RCAlloc(*++argv);
			--argc;
		}
                else if (strcasecmp(*argv, "-clockimage") == 0) {
			needMore(argc, argv);
			ClockXpm_file = RCAlloc(*++argv);
			--argc;
		}
		else if (strcasecmp(*argv, "-clockgeom") == 0) {
			needMore(argc, argv);
			getGeom(*++argv, &ClockGeom);
			--argc;
		}
		else if (strcasecmp(*argv, "-mapgeom") == 0) {
			needMore(argc, argv);
			getGeom(*++argv, &MapGeom);
			--argc;
		}
		else if (strcasecmp(*argv, "-mapmode") == 0) {
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
		else if (strcasecmp(*argv, "-spotsize") == 0) {
			needMore(argc, argv);
			spot_size = atoi(*++argv);
                        if (spot_size<0) spot_size = 0;
                        if (spot_size>3) spot_size = 3;
                        --argc;
		}
		else if (strcasecmp(*argv, "-fillmode") == 0) {
			needMore(argc, argv);
			fill_mode = atoi(*++argv);
                        if (fill_mode<0) fill_mode = 0;
                        if (fill_mode>3) fill_mode = 3;
                        --argc;
		}
		else if (strcasecmp(*argv, "-darkness") == 0) {
			needMore(argc, argv);
			darkness = 1.0 - atof(*++argv);
                        if (darkness<0.0) darkness = 0.0;
                        if (darkness>1.0) darkness = 1.0;
                        --argc;
		}
		else if (strcasecmp(*argv, "-diffusion") == 0) {
			needMore(argc, argv);
			atm_diffusion = atof(*++argv);
                        if (atm_diffusion<0.0) atm_diffusion = 0.0;
                        --argc;
		}
		else if (strcasecmp(*argv, "-refraction") == 0) {
			needMore(argc, argv);
			atm_refraction = atof(*++argv);
                        if (atm_refraction<0.0) atm_refraction = 0.0;
                        --argc;
		}
		else if (strcasecmp(*argv, "-colorscale") == 0) {
			needMore(argc, argv);
			color_scale = atoi(*++argv);
                        if (color_scale<=0) color_scale = 1;
                        if (color_scale>COLORSTEPS) color_scale = COLORSTEPS;
                        --argc;
		}
		else if (strcasecmp(*argv, "-landcolor") == 0) {
			needMore(argc, argv);
			strncpy(LandColor.name, *++argv, COLORLENGTH);
			--argc;
		}
		else if (strcasecmp(*argv, "-watercolor") == 0) {
			needMore(argc, argv);
			strncpy(WaterColor.name, *++argv, COLORLENGTH);
			--argc;
		}
		else if (strcasecmp(*argv, "-arcticcolor") == 0) {
			needMore(argc, argv);
			strncpy(ArcticColor.name, *++argv, COLORLENGTH);
			--argc;
		}
 		else if (strcasecmp(*argv, "-textbgcolor") == 0) {
			needMore(argc, argv);
			strncpy(TextBgColor.name, *++argv, COLORLENGTH);
			--argc;
		}
		else if (strcasecmp(*argv, "-textfgcolor") == 0) {
			needMore(argc, argv);
			strncpy(TextFgColor.name, *++argv, COLORLENGTH);
			--argc;
		}
		else if (strcasecmp(*argv, "-dircolor") == 0) {
			needMore(argc, argv);
			strncpy(DirColor.name, *++argv, COLORLENGTH);
			--argc;
		}
		else if (strcasecmp(*argv, "-citycolor0") == 0) {
			needMore(argc, argv);
			strncpy(CityColor0.name, *++argv, COLORLENGTH);
			--argc;
		}
		else if (strcasecmp(*argv, "-citycolor1") == 0) {
			needMore(argc, argv);
			strncpy(CityColor1.name, *++argv, COLORLENGTH);
			--argc;
		}
		else if (strcasecmp(*argv, "-citycolor2") == 0) {
			needMore(argc, argv);
			strncpy(CityColor2.name, *++argv, COLORLENGTH);
			--argc;
		}
		else if (strcasecmp(*argv, "-markcolor1") == 0) {
			needMore(argc, argv);
			strncpy(MarkColor1.name, *++argv, COLORLENGTH);
			--argc;
		}
		else if (strcasecmp(*argv, "-markcolor2") == 0) {
			needMore(argc, argv);
			strncpy(MarkColor2.name, *++argv, COLORLENGTH);
			--argc;
		}
		else if (strcasecmp(*argv, "-linecolor") == 0) {
			needMore(argc, argv);
			strncpy(LineColor.name, *++argv, COLORLENGTH);
			--argc;
		}
		else if (strcasecmp(*argv, "-tropiccolor") == 0) {
			needMore(argc, argv);
			strncpy(TropicColor.name, *++argv, COLORLENGTH);
			--argc;
		}
		else if (strcasecmp(*argv, "-suncolor") == 0) {
			needMore(argc, argv);
			strncpy(SunColor.name, *++argv, COLORLENGTH);
			--argc;
		}
		else if (strcasecmp(*argv, "-position") == 0) {
			needMore(argc, argv);
			pos1.lat = atof(*++argv);
	                --argc;
			needMore(argc, argv);
			pos1.lon = atof(*++argv);
			mark1.city = &pos1;
			--argc;
		}
		else if (strcasecmp(*argv, "-city") == 0) {
			needMore(argc, argv);
			strncpy(CityInit, *++argv, 79);
	                map_mode = COORDINATES;
			--argc;
		}
		else if (strcasecmp(*argv, "-placement") == 0) {
			needMore(argc, argv);
			if (strcasecmp(*++argv, "random")==0)
                           placement = RANDOM;
			if (strcasecmp(*argv, "fixed")==0) {
                           placement = FIXED;
	                   MapGeom.mask = XValue | YValue | 
                                          WidthValue | HeightValue;
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
		else if (strcasecmp(*argv, "-horizshift") == 0) {
			needMore(argc, argv);
                        horiz_shift = atoi(*++argv);
			--argc;
		}
		else if (strcasecmp(*argv, "-vertshift") == 0) {
			needMore(argc, argv);
                        vert_shift = atoi(*++argv);
			if (vert_shift<6) vert_shift = 6;
			--argc;
		}
		else if (strcasecmp(*argv, "-command") == 0) {
			needMore(argc, argv);
			Command = *++argv;
			--argc;
		}
		else if (strcasecmp(*argv, "-shading") == 0) {
			needMore(argc, argv);
			shading_level = atoi(*++argv);
			if (shading_level < 0) shading_level = 0;
			if (shading_level > 4) shading_level = 4;
			--argc;
		}
		else if ((opt = (strcasecmp(*argv, "-progress") == 0)) ||
                         (strcasecmp(*argv, "-jump") == 0)) {
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
                           time_progress = abs(value); 
                           time_progress_init = time_progress;
			} else 
			   time_jump = value;
			--argc;
		}
		else if (strcasecmp(*argv, "-mono") == 0) {
                        spot_size = 3;
                        mono = 1;
			invert = 1;
			fill_mode = 1;
                }
		else if (strcasecmp(*argv, "-invert") == 0) {
		        invert = 1;
			fill_mode = 1;
		}
		else if (strcasecmp(*argv, "-private") == 0)
		        do_private = 1;
		else if (strcasecmp(*argv, "-usememory") == 0)
                        do_memory = 1;
		else if (strcasecmp(*argv, "-clock") == 0)
			do_map = 0;
		else if (strcasecmp(*argv, "-map") == 0)
			do_map = 1;
		else if (strcasecmp(*argv, "-menu") == 0)
			do_menu = 1;
		else if (strcasecmp(*argv, "-nomenu") == 0)
			do_menu = 0;
		else if (strcasecmp(*argv, "-coastlines") == 0)
		        fill_mode = 0;
		else if (strcasecmp(*argv, "-contour") == 0)
		        fill_mode = 1;
		else if (strcasecmp(*argv, "-landfill") == 0)
		        fill_mode = 2;
		else if (strcasecmp(*argv, "-dottedlines") == 0)
		        dotted = 0;
		else if (strcasecmp(*argv, "-plainlines") == 0)
		        dotted = 1;
		else if (strcasecmp(*argv, "-date") == 0)
                        clock_mode = 0;
		else if (strcasecmp(*argv, "-seconds") == 0)
                        clock_mode = 1;
		else if (strcasecmp(*argv, "-decimal") == 0)
                        deg_mode = dms_mode = 0;
		else if (strcasecmp(*argv, "-dms") == 0)
                        deg_mode = dms_mode = 1;
		else if (strcasecmp(*argv, "-parallels") == 0) {
			do_parall = 1;
		}
		else if (strcasecmp(*argv, "-cities") == 0) {
			do_cities = 1;
		}
		else if (strcasecmp(*argv, "-night") == 0) {
			shading_level = 1;
		}
		else if (strcasecmp(*argv, "-nonight") == 0) {
			shading_level = 0;
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
		else if (strcasecmp(*argv, "-listmenu") == 0) {
		        listmenu();
			if (leave) 
			  exit(0);
			else
			  return(0);
		}
		else if (strcasecmp(*argv, "-version") == 0) {
			fprintf(stderr, "%s: version %s, %s\n",
				ProgName, VERSION, COPYRIGHT);
			if (leave) 
			  exit(0);
			else
			  return(0);
		} else {
                  fprintf(stderr, "%s: unknown option !!\n\n", *argv);
                  if (leave) {
		    usage();
	            exit(1);
		  }
		  else {
                    fprintf(stderr, RECOVER);
		    return(0);
		  }
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
	     leave = 0;
	     i = parseArgs(i+1, args-1);
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

    fname = share_i18n;

    i = strlen(share_i18n);
       
    if (!*language && getenv("LANG"))
       strncpy(language, getenv("LANG"), 2);
    if (!*language)
       strcpy (language,"en");

    for (j=0; j<=1; j++) share_i18n[i+j-2] = tolower(language[j]);

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
             "Unable to open language in %s\n", share_i18n);

    /* Done */

    return(0);
}

int
getPlacement(win, x, y, w, h)
Window win;
int *x, *y;
unsigned int *w, *h;
{
   int xp, yp;
   unsigned int b, d, n;
   Window junk, root, parent, *children;

   XQueryTree(dpy, win, &root, &parent, &children, &n);
	
   if (!parent) return 1;
	
   XGetGeometry(dpy, parent, &root, x, y, w, h, &b, &d);
   XTranslateCoordinates(dpy, win, parent, 0, 0, x, y, &junk);
   if (2*(*x) < *w) *w -= 2*(*x);
   if ((*x)+(*y) < *h) *h -= (*x) + (*y);
   XTranslateCoordinates(dpy, win, root, 0, 0, x, y, &junk);
   XTranslateCoordinates(dpy, parent, root, 0, 0, &xp, &yp, &junk);
   horiz_drift = *x - xp;
   vert_drift = *y - yp;
   return 0;
}

void
checkGeom(bool)
int bool;
{
	int a, b;
	struct geom * Geom = (do_map)? &MapGeom : &ClockGeom;     

        a = DisplayWidth(dpy, scr) - Geom->width - horiz_drift - 5;
	b = DisplayHeight(dpy, scr) - Geom->height - vert_drift - 5;
        if (Geom->x > a) Geom->x = a;
        if (Geom->y > b) Geom->y = b;
	if (Geom->x < 0) Geom->x = 5;
	if (Geom->y < 0) Geom->y = 5;
        if (bool) Geom->mask = XValue | YValue | WidthValue | HeightValue;
}

void
adjustGeom(which)
int which;
{
        int x, y, dx=0, dy=0;
        unsigned int w, h;
	struct geom * Geom;

        if (getPlacement(Map, &x, &y, &w, &h)) return;

	if (which) {
	   if (placement == CENTER) 
              dx = (MapGeom.width - ClockGeom.width)/2; else
           if (placement >= NW) {
	      if (placement & 1) 
                 dx = 0; else dx = MapGeom.width - ClockGeom.width;
           }

           if (placement == CENTER) 
                 dy = (MapGeom.height - ClockGeom.height)/2; else
           if (placement >= NW) {
	      if (placement <= NE) 
                 dy = 0; else dy = MapGeom.height - ClockGeom.height;
           }
	}

        if (placement) {
	    if (do_map) {
               Geom = &MapGeom; 
	       dx = -dx;
	       dy = -dy;
	    } else
               Geom = &ClockGeom;
	    if (placement >= CENTER) {
	        Geom->x = x - dx - horiz_drift;
	        Geom->y = y - dy - vert_drift;
		checkGeom(1);
	    }
	}
}

/*
 * Free resources.
 */

void
shutDown(all)
int all;
{
        if (Earthmap.wtab) {
           free(Earthmap.wtab);
	   Earthmap.wtab = 0;
	}
	if (Earthmap.wtab1) {
           free(Earthmap.wtab1);
           Earthmap.wtab1 = 0;
	}
        if (invert) {
           XFreeGC(dpy, Invert_gc);
	   XFreeGC(dpy, Store_gc);
	   XFreePixmap(dpy, Mappix);
	} else {
	   if (ImageData) free(ImageData);
	   ImageData = 0;
           if (pixel_dark) free(pixel_dark);
	   pixel_dark = 0;
        }
	if (all>=2) goto abort;
        if (!mono) {
	   XFreeGC(dpy, CityColor0.gc);
	   XFreeGC(dpy, CityColor1.gc);
	   XFreeGC(dpy, CityColor2.gc);
	   XFreeGC(dpy, MarkColor1.gc);
	   XFreeGC(dpy, MarkColor2.gc);
	   XFreeGC(dpy, LineColor.gc);
	   XFreeGC(dpy, TropicColor.gc);
	   XFreeGC(dpy, SunColor.gc);
           XFreeGC(dpy, DirFont_gc);
	}
	XFreeGC(dpy, BigFont_gc);
	XFreeGC(dpy, SmallFont_gc);
	XDestroyWindow(dpy, Map);
	hours_shown = 0;
	first_drawall = 1;
	if (all || !do_memory) {
	   if (MapXIM) {
              XDestroyImage(MapXIM); MapXIM = 0;
              XpmFreeAttributes(&MapAttrib);
	   }
	   if (ClockXIM) {
	      XDestroyImage(ClockXIM); ClockXIM = 0;
              XpmFreeAttributes(&ClockAttrib);
	   }
	}
	if (all) {
	   if (dirtable) free(dirtable);
	   XFreeFont(dpy, BigFont);
	   XFreeFont(dpy, SmallFont);
	 abort:
           XCloseDisplay(dpy);
	   exit(0);
	}
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
	struct geom *           Geom = NULL;

        if (num<=1) {
	    win = Map;
	    xsh.flags = PSize | PMinSize;
            if (num) Geom = &MapGeom; else Geom=&ClockGeom;
	    if (Geom->mask & (XValue | YValue)) {
		xsh.x = Geom->x;
		xsh.y = Geom->y;
                xsh.flags |= USPosition; 
	    }
	    xsh.width = Geom->width;
	    xsh.height = Geom->height + Geom->strip;
	    if (pix_type) {
                xsh.max_width = xsh.min_width = xsh.width;
                xsh.max_height = xsh.min_height = xsh.height;
                xsh.flags |= PMaxSize;
	    } else {
                xsh.min_width = Geom->w_mini;
                xsh.min_height = Geom->h_mini;
	    }
	} else 
	if (num>=2) {
	    if (num==2) {
	      win = Menu;
	      Geom = &MenuGeom;
	    }
	    if (num==3) {
	      win = Selfile;
	      Geom = &SelGeom;
	    }
	    xsh.flags = PSize | USPosition;
	    xsh.x = Geom->x;
	    xsh.y = Geom->y;
	    xsh.width = Geom->width;
	    xsh.height = Geom->height;
	    if (num==3) {
	      xsh.flags |= PMinSize;
	      xsh.min_width = Geom->w_mini;
              xsh.min_height = Geom->h_mini;
	    }
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
	char *instance[4] =     { "clock", "map", "menu", "selector" };

	mask = ExposureMask | ButtonPressMask | KeyPressMask;

        switch(num) {

	   case 0:
	   case 1:
		win = Map;
		mask |= ResizeRedirectMask;
		break;

	   case 2:
		win = Menu;
		mask |= PointerMotionMask;
		break;

	   case 3:
	        win = Selfile;
		mask |= ResizeRedirectMask;
		break;
	}

	if (!win) return;

        sprintf(name, "%s %s", ProgName, VERSION);
        XSetIconName(dpy, win, name);
	xch.res_name = ProgName;
	xch.res_class = "Sunclock";
	XSetClassHint(dpy, win, &xch);
        sprintf(name, "%s / %s", ProgName, instance[num]);
	XStoreName(dpy, win, name);
       	XSelectInput(dpy, win, mask);
	XSetWMProtocols(dpy, win, &wm_delete_window, 1);
}

void
fixGeometry(g)
register struct geom *		g;
{
	if (g->mask & XNegative)
		g->x = DisplayWidth(dpy, scr) - g->width + g->x;
	if (g->mask & YNegative)
		g->y = DisplayHeight(dpy, scr) - g->height - g->strip + g->y;
}

void
createWindow(num)
int num;
{
	XSetWindowAttributes	xswa;
	Window			root = RootWindow(dpy, scr);
	struct geom *		Geom = NULL;
        Window *		win = NULL;
	register int		mask;

	xswa.background_pixel = (mono)? white : TextBgColor.pix;
	xswa.border_pixel = (mono)? black : TextFgColor.pix;
	xswa.backing_store = WhenMapped;

	mask = CWBackPixel | CWBorderPixel | CWBackingStore;

        switch (num) {

	   case 0:
	        win = &Map;
		Geom = &ClockGeom;
		break;

	   case 1:
	        win = &Map;
                Geom = &MapGeom;
		break;

	   case 2:
	        if (Menu) win = NULL; else win = &Menu;
		Geom = &MenuGeom;
		break;

	   case 3:
	        if (Selfile) win = NULL; else win = &Selfile;
		Geom = &SelGeom;
		break;
	}

	if (num<=1) fixGeometry(Geom);

        if (win) {
	     *win = XCreateWindow(dpy, root,
		      Geom->x, Geom->y,
		      Geom->width, Geom->height+Geom->strip, 0,
		      CopyFromParent, InputOutput, 
		      CopyFromParent, mask, &xswa);
	     XSetWindowColormap(dpy, *win, cmap);
	}
}

/*
 *  Make and map windows in order specified by user, set the hints
 */

void
createWindows()
{
	int i;

        createWindow(do_map);
        setAllHints(do_map);
        for (i=2; i<=3; i++) createWindow(i);
        for (i=0; i<=3; i++) if (i!=1-do_map) setProtocols(i);

        XMapWindow(dpy, Map);
	XFlush(dpy);
	usleep(TIMESTEP);
	if (do_map && not_firsttime) 
           XMoveWindow(dpy, Map, MapGeom.x, MapGeom.y);
	if (!do_map && not_firsttime) 
	   XMoveWindow(dpy, Map, ClockGeom.x, ClockGeom.y);
	not_firsttime = 1;
}

void
createGCs()
{
	XGCValues		gcv;

	if (invert) {
           gcv.background = white;
           gcv.foreground = black;
	   Store_gc = XCreateGC(dpy, Map, GCForeground | GCBackground, &gcv);
           gcv.function = GXinvert;
           Invert_gc = XCreateGC(dpy, Mappix, 
                             GCForeground | GCBackground | GCFunction, &gcv);
	}

        if (!mono) {
	   gcv.background = TextBgColor.pix;
	   gcv.foreground = TextFgColor.pix;
	}

	gcv.font = SmallFont->fid;
	SmallFont_gc = XCreateGC(dpy, Map, GCForeground | GCBackground | GCFont, &gcv);
	gcv.font = BigFont->fid;
	BigFont_gc = XCreateGC(dpy, Map, GCForeground | GCBackground | GCFont, &gcv);
        Earthmap.gc = (do_map)? BigFont_gc : SmallFont_gc;
        
        if (mono) {
	   DirFont_gc = BigFont_gc;
	   return;
	}

	gcv.foreground = DirColor.pix;
	gcv.font = BigFont->fid;
	DirFont_gc = XCreateGC(dpy, Map, GCForeground | GCBackground | GCFont, &gcv);

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

	s = XAllocNamedColor(dpy, cmap, name, &c, &e);
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
	black = BlackPixel(dpy, scr);
	white = WhitePixel(dpy, scr);

        if (mono) return;

        LandColor.pix   = getColor(LandColor.name, white);
        WaterColor.pix  = getColor(WaterColor.name, black);
        ArcticColor.pix = getColor(ArcticColor.name, black);

        TextBgColor.pix = getColor(TextBgColor.name, white);
        TextFgColor.pix = getColor(TextFgColor.name, black);
        DirColor.pix    = getColor(DirColor.name, black);
        CityColor0.pix  = getColor(CityColor0.name, black);
        CityColor1.pix  = getColor(CityColor1.name, black);
        CityColor2.pix  = getColor(CityColor2.name, black);
	MarkColor1.pix  = getColor(MarkColor1.name, black);
	MarkColor2.pix  = getColor(MarkColor2.name, black);
	LineColor.pix   = getColor(LineColor.name, black);
	TropicColor.pix = getColor(TropicColor.name, black);
	SunColor.pix    = getColor(SunColor.name, white);
}

void
getFonts()
{
	BigFont = XLoadQueryFont(dpy, BigFont_name);
	if (BigFont == (XFontStruct *)NULL) {
		fprintf(stderr, "%s: can't open font `%s', using `%s'\n",
			ProgName, BigFont_name, FAILFONT);
		BigFont = XLoadQueryFont(dpy, FAILFONT);
		if (BigFont == (XFontStruct *)NULL) {
		   fprintf(stderr, "%s: can't open font `%s', giving up\n",
				ProgName, FAILFONT);
		   exit(1);
		}
	}
	SmallFont = XLoadQueryFont(dpy, SmallFont_name);
	if (SmallFont == (XFontStruct *)NULL) {
		fprintf(stderr, "%s: can't open font `%s', using `%s'\n",
			ProgName, SmallFont_name, FAILFONT);
		SmallFont = XLoadQueryFont(dpy, FAILFONT);
		if (SmallFont == (XFontStruct *)NULL) {
			fprintf(stderr, "%s: can't open font `%s', giving up\n",
				ProgName, FAILFONT);
			exit(1);
		}
	}

        MapGeom.strip = BigFont->max_bounds.ascent + 
                       BigFont->max_bounds.descent + 6;

        chwidth = MapGeom.strip + 5;

        ClockGeom.strip = SmallFont->max_bounds.ascent +
	                     SmallFont->max_bounds.descent + 4;

        MenuGeom.width = MENU_WIDTH * MapGeom.strip;
        MenuGeom.height = 2 * MapGeom.strip;

        SelGeom.width = SEL_WIDTH * MapGeom.strip;
        SelGeom.height = (11 + 4*SEL_HEIGHT)*MapGeom.strip/5;
}

void
clearStrip()
{
        XClearArea(dpy, Map, 0, MapGeom.height, 
                 MapGeom.width, MapGeom.strip, True);

	time_count = PRECOUNT;
	if (do_bottom) --do_bottom;
}

void
clearSelfile()
{
	XClearArea(dpy, Selfile, 0, MapGeom.strip+1, 
                SelGeom.width-2, SelGeom.height-MapGeom.strip-2, True);
}

void
exposeMap()
{
        XClearArea(dpy, Map, Earthmap.width-1, Earthmap.height-1, 1, 1, True);
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
        sinapp = sin(dtr(SUN_APPRADIUS + atm_refraction));

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

        /* time_count = PRECOUNT;
	   force_proj = 1; */
	last_hint = -1;

	if (!cptr) return;

  	gtime = Earthmap.time + time_jump;

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
winprint(gtime)
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

	if (!do_map) {
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

		return (s+label_shift);
        }

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
	   if (ltp.tm_mday != local_day_old) setDayParams(mark1.city);
	   local_day_old = ltp.tm_mday;
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
	     if (stp.tm_mday != solar_day_old) setDayParams(mark1.city);
	     solar_day_old = stp.tm_mday;
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

void
makeMapContext()
{
	if (do_map) {
	    Earthmap.width = MapGeom.width;
	    Earthmap.height = MapGeom.height;
	    Earthmap.textx = 4;
	    Earthmap.texty = MapGeom.height + BigFont->max_bounds.ascent + 3;
	} else {
	    Earthmap.width = ClockGeom.width;
	    Earthmap.height = ClockGeom.height;
	    Earthmap.textx = 4;
	    Earthmap.texty = ClockGeom.height + SmallFont->max_bounds.ascent + 2;
	}

	Earthmap.flags = S_DIRTY;
	Earthmap.noon = -1;
	Earthmap.wtab = (short *)salloc((int)(Earthmap.height * sizeof (short *)));
	Earthmap.wtab1 = (short *)salloc((int)(Earthmap.height * sizeof (short *)));
	Earthmap.increm = 0L;
	Earthmap.time = 0L;
	Earthmap.timeout = 0;
	Earthmap.projtime = -1L;
	Earthmap.text[0] = '\0';
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
    GC gc = CityColor0.gc;
    Window w;

    if (mode < 0) return;
    if (!mono && !do_cities && mode <= 2) return;

    if (mode == 0) { gc = CityColor0.gc; --mode; }
    if (mode == 1)   gc = CityColor1.gc;
    if (mode == 2)   gc = CityColor2.gc;
    if (mode == 3)   gc = MarkColor1.gc;
    if (mode == 4)   gc = MarkColor2.gc;
    if (mode == 5)   gc = SunColor.gc;

    checkMono(&w, &gc);

    ilat = Earthmap.height - (lat + 90) * (Earthmap.height / 180.0);
    ilon = (180.0 + lon) * (Earthmap.width / 360.0);

    rad = radius[spot_size];

    if (mode == 5)
       {
       int rad=6, diag=4;
       XFillArc(dpy, w, gc, ilon-rad/2, ilat-rad/2, rad, rad, 0, 360 * 64);
       XDrawLine(dpy, w, gc, ilon, ilat-rad, ilon, ilat+rad);
       XDrawLine(dpy, w, gc, ilon-rad, ilat, ilon+rad, ilat);
       XDrawLine(dpy, w, gc, ilon-diag, ilat-diag, ilon+diag, ilat+diag);
       XDrawLine(dpy, w, gc, ilon-diag, ilat+diag, ilon+diag, ilat-diag);
       return;
       }

    if (spot_size == 1)
       XDrawArc(dpy, w, gc, ilon-rad, ilat-rad, 2*rad, 2*rad, 0, 360 * 64);
    if (spot_size == 2)
       XFillArc(dpy, w, gc, ilon-rad, ilat-rad, 2*rad, 2*rad, 0, 360 * 64);  
    if (spot_size == 3)
       {
       XDrawArc(dpy, w, gc, ilon-rad, ilat-rad, 2*rad, 2*rad, 0, 360 * 64);
       XFillArc(dpy, w, gc, ilon-3, ilat-3, 6, 6, 0, 360 * 64);
       }
}

void
drawCities()
{
City *c;
        if (!do_map) return; 
        for (c = cities; c; c = c->next)
	    placeSpot(c->mode, c->lat, c->lon);
}

void
drawMarks()
{
        if (mono || !do_map) return; 

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
draw_parallel(gc, lat, step, thickness)
GC gc;
double lat;
int step;
int thickness;
{
        Window w;
	int ilat, i0, i1, i;

        if (!do_map) return; 

        checkMono(&w, &gc);

	ilat = Earthmap.height - (lat + 90) * (Earthmap.height / 180.0);
	i0 = Earthmap.width/2;
	i1 = 1+i0/step;
	for (i=-i1; i<i1; i+=1)
  	  XDrawLine(dpy, w, gc, i0+step*i-thickness, ilat, 
                                i0+step*i+thickness, ilat);
}

void
draw_parallels()
{
	int     i;

        for (i=-8; i<=8; i++) if (i!=0 || !do_tropics)
	  draw_parallel(LineColor.gc, i*10.0, 3, dotted);
}

void
draw_tropics()
{
	static  double val[5] = { -66.55, -23.45, 0.0, 23.45, 66.55 };
	int     i;

        if (mono && do_parall)
	  draw_parallel(LineColor.gc, 0.0, 3, dotted);

        for (i=0; i<5; i++)
	  draw_parallel(TropicColor.gc, val[i], 3, 1);
}	


/*
 * draw_meridian() - Draw a meridian line
 */

void
draw_meridian(gc, lon, step,thickness)
GC gc;
double lon;
int step;
int thickness;
{
        Window w;
	int ilon, i0, i1, i;

        if (!do_map) return; 
	
        checkMono(&w, &gc);

	ilon = (180.0 + lon) * (Earthmap.width / 360.0);
	i0 = Earthmap.height/2;
	i1 = 1+i0/step;
	for (i=-i1; i<i1; i+=1)
	  XDrawLine(dpy, w, gc, ilon, i0+step*i-thickness, 
                                ilon, i0+step*i+thickness);
}

void
draw_meridians()
{
	int     i;

        for (i=-11; i<=11; i++)
	  draw_meridian(LineColor.gc, i*15.0, 3, dotted);
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
drawBottomline()
{
        Window w = Map;
	GC gc = BigFont_gc;

        checkMono(&w, &gc);
        XDrawLine(dpy, w, gc, 0, Earthmap.height-1, 
	           Earthmap.width-1, Earthmap.height-1);
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
	if (!mono)
	  drawBottomline();
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
        
        if (!do_map) return; 
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
projillum(wtab, dec)
short *wtab;
double dec;
{
	int i, ftf = True, ilon, ilat, lilon = 0, lilat = 0, xt;
	double m, x, y, z, th, lon, lat, s, c;

	/* Clear unoccupied cells in width table */

	if (shading_level)
   	   for (i = 0; i < Earthmap.height; i++)
		wtab[i] = -1;
	else {
   	   for (i = 0; i < Earthmap.height; i++)
		wtab[i] = Earthmap.width;
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

		ilat = Earthmap.height 
                       - (lat + 90.0) * (Earthmap.height / 180.0);
		ilon = lon * (Earthmap.width / 180.0);

		if (ftf) {

			/* First time.  Just save start co-ordinate. */

			lilon = ilon;
			lilat = ilat;
			ftf = False;
		} else {

			/* Trace out the line and set the width table. */

			if (lilat == ilat) {
				wtab[(Earthmap.height - 1) - ilat] = (ilon == 0)? 1 : ilon;
			} else {
				m = ((double) (ilon - lilon)) / (ilat - lilat);
				for (i = lilat; i != ilat; i += sgn(ilat - lilat)) {
					xt = lilon + floor((m * (i - lilat)) + 0.5);
					wtab[(Earthmap.height - 1) - i] = (xt == 0)? 1 : xt;
				}
			}
			lilon = ilon;
			lilat = ilat;
		}
	}

	/* Now tweak the widths to generate full illumination for
	   the correct pole. */

	if (dec < 0.0) {
		ilat = Earthmap.height - 1;
		lilat = -1;
	} else {
		ilat = 0;
		lilat = 1;
	}

	for (i = ilat; i != Earthmap.height / 2; i += lilat) {
		if (wtab[i] != -1) {
			while (True) {
				wtab[i] = Earthmap.width;
				if (i == ilat)
					break;
				i -= lilat;
			}
			break;
		}
	}
}

void
SwitchPixel(x, y, t)
register int x;
register int y;
register int t;
{
        int i;
 	Pixel p;

        CurXIM->data = ImageData;
	p = XGetPixel(CurXIM, x, y);
        CurXIM->data = CurData;
        if (t>=0) {
	  for (i=0; i<Attributes->npixels; i++)
	    if (Attributes->pixels[i] == p) {     
	        XPutPixel(CurXIM, x, y, pixel_dark[i+t*Attributes->npixels]);
		break;
	    }
        } else {
	  XPutPixel(CurXIM, x, y, p);
	}
}

/*  XSPAN  --  Complement a span of pixels.  Called with line in which
	       pixels are contained, leftmost pixel in the  line,  and
	       the number of pixels to update.  Handles wrap-around at 
               the right edge of the screen.
               
               The 'state' parameter is a flag with first  3  bits set 
               as follows:
	       bit 0: (0/1) don't use/use progressive shading at  left 
                      border of line
	       bit 1: (0/1) don't use/use progressive shading at right
                      border of line
	       bit 2: (0/1) draw line in dark/light (or invert).
*/

int
setpix(i, color)
register int i;
register int color;
{
  int j;

  if (!shading) return 0;
  j = ((i + shading) * color_scale)/shading - color_scale/2;
  if (j < 0) j = 0;
  if (j > color_scale) j = color_scale;
  if (!color) j = color_scale-j;
  if (j == color_scale) j = -1;
  return j;
}

int
setlight(v)
double v;
{
  int j;

  j = (int) (0.5*(1+v)*color_scale);
  if (j >= color_scale) j = color_scale - 1;
  return j;
}

void
xspan(pline, leftp, npix, state)
register int	pline;
register int	leftp;
register int	npix;
register int	state;
{
        int i, a, b, c, shift, color;

	if (!npix) return;

        leftp = (leftp + Earthmap.width) % Earthmap.width;
	color = state/4;

	if (shading_level >= 2) {
	   if (npix == Earthmap.width) {
	      for (i=0; i<Earthmap.width; i++)
		 SwitchPixel(i, pline, -color);
	      return;
	      }
           c = (npix - Earthmap.width)/2;
           if (range <= -c) 
	      c = -range;
           if ((state & 3)==0)
	      a = b = c;
	   else {
              switch((state & 3)) {
	         case 1:
	           a = npix-c;
	           b = c;
                   break;
	         case 2:
	           b = npix-c;
	           a = c;
	           break;
	         case 3:
	           if (range>npix/2) 
                      a = b = npix/2;
		   else
	              a = b = range;
	           break;
	      }
	   }
	   shift = leftp+Earthmap.width;
           if (state & 1)
	      for (i=c; i<a; i++)
	        SwitchPixel((i+shift)%Earthmap.width, pline, setpix(i, color));
	   else
	      for (i=c; i<a; i++)
	        SwitchPixel((i+shift)%Earthmap.width, pline, -color);
	   shift = npix+leftp+Earthmap.width - 1;
           if (state & 2)
	      for (i=c; i<b; i++)
	        SwitchPixel((shift-i)%Earthmap.width, pline, setpix(i, color));
	   else
	      for (i=c; i<b; i++)
                SwitchPixel((shift-i)%Earthmap.width, pline, -color);
	   shift = leftp+Earthmap.width;
	   for (i=a; i<npix-b; i++)
	      SwitchPixel((i+shift)%Earthmap.width, pline, -color);
	   return;
	}

	if (leftp + npix > Earthmap.width) {
	  if (invert) {
	    XDrawLine(dpy, Mappix, Invert_gc, 0, pline,
		               (leftp + npix) - (Earthmap.width + 1), pline);
 	    XDrawLine(dpy, Mappix, Invert_gc, leftp, pline, 
                                              Earthmap.width - 1, pline);
	  } else {
            for (i=0; i<(leftp + npix) - Earthmap.width; i++)
	        SwitchPixel(i, pline, -color);
            for (i=leftp; i<Earthmap.width; i++)
	        SwitchPixel(i, pline, -color);
	    }
	}
	else {
	  if (invert)
	     XDrawLine(dpy, Mappix, Invert_gc, leftp, pline,
			  leftp + (npix - 1), pline);
	  else {
             for (i=leftp; i<leftp + npix ; i++)
	        SwitchPixel(i, pline, -color);
	  }
	}
}

void
setShade()
{
	if (shading_level == 2)
           Earthmap.shade = 
	      Earthmap.width * (SUN_APPRADIUS + atm_refraction)/180.0;
	if (shading_level >= 3)
           Earthmap.shade = 
	      Earthmap.width * (SUN_APPRADIUS + atm_diffusion)/180.0;
}

int
ShadingLevel(t)
short t;
{
         int max, val, res;
         double quot;
       
         if (t<0) return 0;
         if (t>=Earthmap.width) return 0;
	 if (t>Earthmap.width/2) 
	    val = Earthmap.width - t;
	 else
	    val = t;
	 max = val/2;
	 if (!val) return 0;
	 quot = ((double)Earthmap.width)/(double)(2*val);
         res = (int) (0.5 + (Earthmap.shade * quot));
	 if (res>max) res = max;
	 return res;
}

/*  MOVETERM  --  Update illuminated portion of the globe.  */

void
moveterm(wtab, noon, otab, onoon)
short *wtab, *otab;
int noon, onoon;
{
      int i, j, ol, oh, nl, nh;
      double c, cp, sp;

      for (i = 0; i < Earthmap.height; i++) {

	 if (shading_level >= 2) {
            shading = ShadingLevel(wtab[i]);
	    j = ShadingLevel(otab[i]);
	    if (j<=shading) 
               range = (shading+1)/2;
	    else
               range = (j+1)/2;
	 }

	     /* If line is off in new width table but is set in
	        the old table, clear it. */

	 if (wtab[i] < 0) {
	    if (otab[i] >= 0)
	       xspan(i, ((onoon-otab[i]/2)+Earthmap.width)%Earthmap.width,
                        otab[i], 0);
	 } else {

	     /* Line is on in new width table.  If it was off in
	        the old width table, just draw it. */

	    if (otab[i] < 0) {
	       xspan(i, ((noon-wtab[i]/2)+Earthmap.width)%Earthmap.width,
                        wtab[i], 7);
	    } else {
	     /* If both the old and new spans were the entire
	       screen, they're equivalent. */
  	       if (otab[i] == wtab[i] && wtab[i] == Earthmap.width) continue;

	     /* The line was on in both the old and new width
	        tables.  We must adjust the difference in the span.  */

  	       ol = ((onoon - otab[i]/2) + Earthmap.width) % Earthmap.width;
	       oh = ol + otab[i];
	       nl = ((noon - wtab[i]/2) + Earthmap.width) % Earthmap.width;
	       nh = nl + wtab[i];

	     /* Check whether old or new line spans the entire screen */

               if (wtab[i] == Earthmap.width) {
		  xspan(i, oh, Earthmap.width - otab[i], 4);
                  goto done;
	       }
               if (otab[i] == Earthmap.width) {
		  xspan(i, nh, (int) Earthmap.width - wtab[i], 3);
		  goto done;
	       }
	      
	     /* If spans are disjoint, erase old span and set new span. */

	       if (oh <= nl || nh <= ol) {
	          xspan(i, ol, oh - ol, 0);
		  xspan(i, nl, nh - nl, 7);
	       } else {

               /* Clear portion(s) of old span that extend 
                  beyond end of new span. */
	          if (ol < nl)
	             xspan(i, ol, nl - ol, 2);
	          if (oh > nh)
	             xspan(i, nh, oh - nh, 1);

	       /* Extend existing (possibly trimmed) span to
		  correct new length. */
	          if (nl < ol)
	             xspan(i, nl, ol - nl, 5);
	          if (nh > oh)
	             xspan(i, oh, nh - oh, 6);
	       }
	    }
	 }
       done:
	 otab[i] = wtab[i];
      }

      if (shading_level == 4) {
	 for (i = 0; i < Earthmap.height; i++) if (wtab[i] >= 0) {
	    nl = noon - wtab[i]/2 + Earthmap.width;
	    sp = M_PI*(double)(Earthmap.height/2-i)/((double) Earthmap.height);
	    cp = cos(sp)*cos(dtr(sun_decl));
            sp = sin(sp)*sin(dtr(sun_decl));
            for (j=0; j<wtab[i]; j++) {
  	     c = cos(2.0*M_PI*(double)(j-wtab[i]/2)/((double) Earthmap.width));
	     SwitchPixel((j+nl)%Earthmap.width, i, setlight(c*cp+sp));
	    }
	 }
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

	if (hours_shown) return;
        clearStrip();
	for (i=0; i<24; i++) {
	    sprintf(s, "%d", i); 
   	    XDrawImageString(dpy, Map, BigFont_gc, 
              ((i*Earthmap.width)/24 + 2*Earthmap.width - chwidth*strlen(s)/8 +
               (int)(sun_long*Earthmap.width/360.0))%Earthmap.width +1,
              BigFont->max_bounds.ascent + Earthmap.height + 3, s, strlen(s));
	}
        hours_shown = 1;
}

/* --- */
/*  UPDIMAGE  --  Update current displayed image.  */

void
updateImage()
{
	register int		i;
	int			xl;
	short *			wtab_swap;

	/* If this is a full repaint of the window, force complete
	   recalculation. */

	if (Earthmap.noon < 0) {
		Earthmap.projtime = 0;
		if (invert)
		for (i = 0; i < Earthmap.height; i++)
			Earthmap.wtab1[i] = -1;
		  else
		for (i = 0; i < Earthmap.height; i++)
			Earthmap.wtab1[i] = Earthmap.width;
	}

	if (Earthmap.flags & S_FAKE) {
		if (Earthmap.flags & S_ANIMATE)
			Earthmap.time += Earthmap.increm;
		if (Earthmap.time < 0)
			Earthmap.time = 0;
	} else
		time(&Earthmap.time);
	
	if (mono && !first_drawall) drawSun();

  	(void) sunParams(Earthmap.time + time_jump, &sun_long, &sun_decl, NULL);
	xl = sun_long * (Earthmap.width / 360.0);
	sun_long -= 180.0;

        if (mono) {
	  drawSun();
	  if (first_drawall) {
	    drawAll();
	    drawBottomline();
	    first_drawall = 0;
	  }
	}

	/* Projecting the illumination curve  for the current seasonal
           instant is costly.  If we're running in real time, only  do
	   it every PROJINT seconds.  */

	if ((Earthmap.flags & S_FAKE)
	 || Earthmap.projtime < 0
	 || (Earthmap.time - Earthmap.projtime) > PROJINT 
         || force_proj) {
		projillum(Earthmap.wtab, sun_decl);
		wtab_swap = Earthmap.wtab;
		Earthmap.wtab = Earthmap.wtab1;
		Earthmap.wtab1 = wtab_swap;
		Earthmap.projtime = Earthmap.time;
	}

	/* If the subsolar point has moved at least one pixel, update
	   the illuminated area on the screen.	*/

	if ((Earthmap.flags & S_FAKE) || Earthmap.noon != xl || force_proj) {
		moveterm(Earthmap.wtab1, xl, Earthmap.wtab, Earthmap.noon);
		Earthmap.noon = xl;
		Earthmap.flags |= S_DIRTY;
		force_proj = 0;
                if (map_mode == EXTENSION) show_hours();
	}
}

void
showText()
{
	XDrawImageString(dpy, Map, Earthmap.gc, Earthmap.textx,
			 Earthmap.texty, Earthmap.text, strlen(Earthmap.text));
}

void
showImage()
{
	register char *		p;
	time_t                  gtime;

        gtime = Earthmap.time + time_jump;

	p = winprint(gtime);

	if (Earthmap.flags & S_DIRTY) {
	   if (invert)
	       XCopyPlane(dpy, Mappix, Map, Store_gc, 
                    0, 0, Earthmap.width, Earthmap.height, 0, 0, 1);
	   else
               XPutImage(dpy, Map, BigFont_gc, CurXIM, 0, 0, 0, 0,
                        Earthmap.width, Earthmap.height);

	   Earthmap.flags &= ~S_DIRTY;
	   if (!mono) {
	       drawAll();
	       drawSun();
	   }
	}
	strcpy(Earthmap.text, p);
	showText();
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

        cx = (180.0 + cptr->lon) * (Earthmap.width / 360.0);
	cy = Earthmap.height - (cptr->lat + 90.0) * (Earthmap.height / 180.0);

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
	   updateMap();
	}
	break;

      case DISTANCES:
        if (mark2.city) mark2.city->mode = 0;
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
	  pos1.lat = 90.0-((double)y/(double)Earthmap.height)*180.0 ;
          pos1.lon = ((double)x/(double)Earthmap.width)*360.0-180.0 ;
	  mark1.city= &pos1;
	}
        updateMap();
        break;

      case SOLARTIME:
        if (mark1.city) 
           mark1.city->mode = 0;
        if (cptr) {
          mark1.city= cptr;
          mark1.city->mode = 1;
        } else {
          pos1.name = Label[L_POINT];
	  pos1.lat = 90.0-((double)y/(double)Earthmap.height)*180.0 ;
          pos1.lon = ((double)x/(double)Earthmap.width)*360.0-180.0 ;
	  mark1.city = &pos1;
	}
        updateMap();
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
initMenu()
{
	char s[2];
	char *ptr[2] = { Label[L_ESCAPE], Label[L_CONTROLS] };
	int i, j, b, d;

        s[1]='\0';
	d = (5*chwidth)/12;
	for (i=0; i<N_OPTIONS; i++) {
	      b = (Option[2*i+1]==';');
	      for (j=(i+1)*chwidth-b; j<=(i+1)*chwidth+b; j++)
	          XDrawLine(dpy, Menu, BigFont_gc, j, 0, j, MapGeom.strip);
	      s[0]=Option[2*i];
	      XDrawImageString(dpy, Menu, BigFont_gc, d+i*chwidth, 
                  BigFont->max_bounds.ascent + 3, s, 1);
	}
	for (i=0; i<=1; i++)
	   XDrawImageString(dpy, Menu, BigFont_gc, d +(1-i)*N_OPTIONS*chwidth,
                     BigFont->max_bounds.ascent + i*MapGeom.strip + 3, 
		     ptr[i], strlen(ptr[i]));
        XDrawLine(dpy, Menu, BigFont_gc, 0, MapGeom.strip, 
                        MENU_WIDTH * MapGeom.strip, MapGeom.strip);
}


void
PopMenu()
{
	int    w, h, a, b;
	
	if (!do_map) return;

	do_menu = 1 - do_menu;

        if (!do_menu) 
	  {
	  XUnmapWindow(dpy, Menu);
	  return;
	  }

        last_hint = -1;
	if (!getPlacement(Map, &MapGeom.x, &MapGeom.y, &w, &h)) {
	   MenuGeom.x = MapGeom.x + horiz_shift - horiz_drift;
	   /* To center: + (MapGeom.width - MenuGeom.width)/2; */
	   a = MapGeom.y + MapGeom.height + MapGeom.strip + vert_shift;
           b = MapGeom.y - MenuGeom.height - vert_shift - 2*vert_drift;
           if (b < (int) MenuGeom.height ) b = MenuGeom.height;
           if (a > (int) DisplayHeight(dpy,scr) 
                   - 2*MenuGeom.height -vert_drift -20)
              a = b;
	   MenuGeom.y = (placement<=NE)? a : b;              
	}
        setAllHints(2);
        XMoveWindow(dpy, Menu, MenuGeom.x, MenuGeom.y);
	if (do_selfile<=1)
           XMapRaised(dpy, Menu);
	else
           XMapWindow(dpy, Menu);
        XMoveWindow(dpy, Menu, MenuGeom.x, MenuGeom.y);
}

void
initSelfile()
{
	int i, d, p, q, h, skip;
	char *s;
	char *banner[9] = { "home", "share", "  /", "  .", "", "", 
	                    Label[L_ESCAPE], "", Label[L_VECTMAP] };

	d = chwidth/4;

	if (do_selfile==1) {

	  for (i=0; i<9; i++)
             XDrawImageString(dpy, Selfile, BigFont_gc, d+2*i*chwidth,
                BigFont->max_bounds.ascent + 3, banner[i], strlen(banner[i]));
 
          for (i=1; i<=8; i++) if (i!=7) {
	     h = 2*i*chwidth;
             XDrawLine(dpy, Selfile, BigFont_gc, h, 0, h, MapGeom.strip);
	  }

	  /* Drawing small triangular icons */
	  p = MapGeom.strip/4;
	  q = 3*MapGeom.strip/4;
	  h = 9*chwidth;
	  for (i=0; i<=q-p; i++)
	      XDrawLine(dpy,Selfile, BigFont_gc,
                    h-i, p+i, h+i, p+i);
	  h = 11*chwidth;
	  for (i=0; i<= q-p; i++)
	      XDrawLine(dpy,Selfile, BigFont_gc, h-i, q-i, h+i, q-i);

	  h = MapGeom.strip;
          XDrawLine(dpy, Selfile, BigFont_gc, 0, h, SelGeom.width, h);
	}
	
        do_selfile = 2;

        XDrawImageString(dpy, Selfile, DirFont_gc,
                d, BigFont->max_bounds.ascent + MapGeom.strip + 3, 
                image_dir, strlen(image_dir));

        h = 2*MapGeom.strip;
        XDrawLine(dpy, Selfile, BigFont_gc, 0, h, SelGeom.width, h);

	dirtable = get_dir_list(image_dir, &num_table_entries);
	if (dirtable)
           qsort(dirtable, num_table_entries, sizeof(char *), dup_strcmp);
	else {
	   char error[] = "Directory inexistent or inaccessible !!!";
           XDrawImageString(dpy, Selfile, DirFont_gc, d, 3*MapGeom.strip,
		     error, strlen(error));
	   return;
	}

	skip = (4*MapGeom.strip)/5;
	num_lines = (SelGeom.height-2*MapGeom.strip)/skip;
        for (i=0; i<num_table_entries-selfile_shift; i++) 
	  if (i<num_lines) {
	  s = dirtable[i+selfile_shift];
	  h = (s[strlen(s)-1]=='/');
          XDrawImageString(dpy, Selfile, h? DirFont_gc : BigFont_gc,
              d, BigFont->max_bounds.ascent + 
              2*MapGeom.strip + i*skip + 3, 
              s, strlen(s));
	}

}

void
PopSelfile()
{
        int a, b, w, h;
	struct geom * Geom;

	if (do_selfile)
            do_selfile = 0;
	else
	    do_selfile = 1;

        if (!do_selfile) 
	  {
	  XUnmapWindow(dpy, Selfile);
	  if (dirtable) free_dirlist(dirtable);
	  dirtable = NULL;
	  return;
	  }

	selfile_shift = 0;

	if (do_map) Geom=&MapGeom; else Geom=&ClockGeom;

	if (!getPlacement(Map, &Geom->x, &Geom->y, &w, &h)) {
	   SelGeom.x = Geom->x + horiz_shift - horiz_drift;
	   /* + (Geom->width - SelGeom.width)/2; */
	   a = Geom->y + Geom->height + Geom->strip + vert_shift;
           if (do_map && do_menu) 
               a += MenuGeom.height + vert_drift + vert_shift;
           b = Geom->y - SelGeom.height - vert_shift - 2*vert_drift;
           if (b < (int) MenuGeom.strip ) b = MenuGeom.height;
           if (a > (int) DisplayHeight(dpy,scr) 
                   - SelGeom.height - vert_drift -20)
              a = b;
	   SelGeom.y = (placement<=NE)? a : b;              
	}

        setAllHints(3);
        XMoveWindow(dpy, Selfile, SelGeom.x, SelGeom.y);
        XMapRaised(dpy, Selfile);
        XMoveWindow(dpy, Selfile, SelGeom.x, SelGeom.y);
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
	if (num >=4 && num <=6)
	    sprintf(more, " (%s)", Label[L_DEGREE]);
	if (num >=9 && num <=12) {
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
		  Label[L_PROGRESS], (num==6)? '-':'+', prog_str, 
                  Label[L_TIMEJUMP], time_jump/86400.0,
		  Label[L_DAYS]);
	}
	if (Option[2*num] == 'H') {
	    sscanf(RELEASE, "%d %d %d", &i, &j, &k);
	    sprintf(more, " (%s %s, %d %s %d, %s)", 
                      ProgName, VERSION, i, Month_name[j-1], k, COPYRIGHT);
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
              BigFont->max_bounds.ascent + MapGeom.strip + 3, 
              hint, strlen(hint));
}

void
createPixmap()
{
   struct geom * Geom;
   char        Xpm_file[1024]="";

   if (invert) {
     makeMapContext();
     makePixmap();
     Mappix = XCreatePixmapFromBitmapData(dpy, RootWindow(dpy, scr),
          Earthmap.bits, Earthmap.width,
          Earthmap.height, 0, 1, 1);
     free(Earthmap.bits);
     pix_type = 0;
     return;
   }

   CurXIM=0;

   if (do_map) {
      Attributes = &MapAttrib;
      Geom = &MapGeom;
      if (MapXpm_file!=NULL) {
	if (*MapXpm_file != '/' && *MapXpm_file != '.' )
	  sprintf(Xpm_file, "%s%s", image_dir, MapXpm_file);
	else
	  strcpy(Xpm_file, MapXpm_file);
      }
      if (MapXIM) CurXIM = MapXIM;
   } else {
      Attributes = &ClockAttrib;
      Geom = &ClockGeom;
      if (ClockXpm_file!=NULL) {
	if (*ClockXpm_file != '/' && *ClockXpm_file != '.' )
	  sprintf(Xpm_file, "%s%s", image_dir, ClockXpm_file);
	else
	  strcpy(Xpm_file, ClockXpm_file);
      }
      if (ClockXIM) CurXIM = ClockXIM;
   }

   if (CurXIM) {
      CurData = CurXIM->data;
      makeMapContext();
      return;
   }

   if (*Xpm_file) {
      Attributes->valuemask = XpmColormap | XpmReturnPixels;
      Attributes->colormap = cmap;
      if (XpmReadFileToImage(dpy, Xpm_file, &CurXIM, NULL, Attributes)) {
	 struct stat buf;
	 stat(Xpm_file, &buf);
         if (!S_ISREG(buf.st_mode))
            fprintf(stderr, "File %s doesn't seem to exist !!\n", Xpm_file);
         else {
            fprintf(stderr, "Cannot read XPM file %s,\nalthough it exists !!\n"
		    "Colormap possibly full. Retry with -private option.\n",
                    Xpm_file);
	 }
         if (do_map) MapXIM=0; else ClockXIM=0;
      } else {
         Geom->width = Attributes->width;
         Geom->height = Attributes->height;
         if (do_map) MapXIM=CurXIM; else ClockXIM=CurXIM;   
         pix_type = 1;
         makeMapContext();
	 return;
      }
   }

   /* Otherwise (no pixmap file specified) use default vector map  */
   makeMapContext();
   makePixmap();
   Attributes->valuemask = XpmColormap | XpmReturnPixels;
   Attributes->colormap = cmap;
   XpmCreateImageFromData(dpy, Earthmap.data, &CurXIM, NULL, Attributes);
   free(Earthmap.data[0]);
   free(Earthmap.data);
   if (CurXIM==0) {
     fprintf(stderr, "Cannot create map. Colormap possibly full. "
                     "Retry with -private option.\n");
     shutDown(2);
   }
   if (do_map) MapXIM=CurXIM; else ClockXIM=CurXIM;
   pix_type = 0;
}

void
setDarkPixels(a, b)
int a, b;
{
     int i, j, k, full=1, depth;
     XColor color, colorp, approx;
     unsigned int red[256], green[256], blue[256];
     char name[15];
     double steps[COLORSTEPS];

     if (invert || a==b) return;

     for (j=0; j<color_scale; j++) 
          steps[j] = 1.0 -
              (1.0 - ((double)j)/((double) color_scale))*(1.0 - darkness);

     for (j=a; j<b; j++) {
          for (i=0; i<Attributes->npixels; i++) { 
	      color.pixel = Attributes->pixels[i];
              XQueryColor(dpy, cmap, &color);
              k = i + j * Attributes->npixels;
              colorp.red = (unsigned int) ((double) color.red * steps[j]);
              colorp.green = (unsigned int) ((double) color.green * steps[j]);
              colorp.blue = (unsigned int) ((double) color.blue * steps[j]);
	      colorp.flags = DoRed | DoGreen | DoBlue;
	      if (XAllocColor(dpy, cmap, &colorp))
                 pixel_dark[k] = colorp.pixel;
	      else {
		 if (full) {
		   int l;
		   printf("%d\n", color.pixel);
                   fprintf(stderr, 
                      "Colormap full: cannot allocate color series %d/%d!\n"
		      "Trying to allocate other sufficiently close color.\n",
		      "You'll get a pretty ugly shading anyway...\n",
                      j, color_scale);
		   full = 0;
		   depth = DefaultDepth(dpy, scr);
		   for (l=0; l<256; l++) {
		      approx.pixel = l;
		      XQueryColor(dpy, cmap, &approx);
		      red[l] = approx.red;
		      green[l] = approx.red;
		      blue[l] = approx.blue;
		   }
		 }
		 if (depth == 8) {
		    int delta, delta0 = 1000000000, l, l0 = 0;
		    for (l=0; l<256; l++) {
		        int u, v, w;
		        u = ((int)colorp.red - (int)red[l])/16;
                        v = ((int)colorp.green - (int)green[l])/16;
                        w = ((int) colorp.blue - (int)blue[l])/16;
			delta = u*u+v*v+w*w;
			if (delta < delta0) { delta0 = delta; l0 = l;}
		    }
		    pixel_dark[k] = l0;
		 } else
		 pixel_dark[k] = black;
	      }
	  }
     }
}

void 
createWorkImage()
{
   int size;
   if (CurXIM) {
     XPutImage(dpy, Map, BigFont_gc, CurXIM, 0, 0, 0, 0,
                        Earthmap.width, Earthmap.height);
     XFlush(dpy);
     pixel_dark = (Pixel *) 
          salloc(color_scale * (Attributes->npixels) * sizeof(Pixel));
     allocation_level = (shading_level>=2)?color_scale:1;
     setDarkPixels(0, allocation_level);
     size = CurXIM->width*CurXIM->height*(CurXIM->bitmap_pad/8);
     ImageData = (char *)salloc(size);
     memcpy(ImageData, CurXIM->data, size); 
     CurData = CurXIM->data;
   }
}

void
checkAuxilWins()
{
      if (do_selfile) 
	 do_selfile = 2;
      if (do_menu) {
	     if (do_map) {
	        do_menu = 0;
   	        PopMenu();
	     } else
                XUnmapWindow(dpy, Menu);
      } 
      if (do_selfile) {
	  do_selfile = 0;
	  PopSelfile();
      }
}

void 
buildMap()
{
   createPixmap();
   checkGeom(0);
   createWindows();
   createGCs();
   createWorkImage();
   checkAuxilWins();
   setShade();
}

void RaiseAndFocus(win)
Window win;
{
     XFlush(dpy);
     XRaiseWindow(dpy, win);
     XSetInputFocus(dpy, win, RevertToPointerRoot, CurrentTime);
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

	/* printf("Test: <%c> %u\n", key, key); */
	if (win==Selfile) {
	   switch(key) {
	     case '\033':
                if (do_selfile) 
	          PopSelfile();
		return;
	     case 'U':
	        if (selfile_shift == 0) return;
	        selfile_shift -= num_lines/2;
	        if (selfile_shift <0) selfile_shift = 0;
		break;
	     case 'V':
	        if (num_table_entries-selfile_shift<num_lines) return;
	        selfile_shift += num_lines/2;
		break;
	     case 'R':
	        if (selfile_shift == 0) return;
	        selfile_shift -= 1;
		break;
	     case 'T':
	        if (num_table_entries-selfile_shift<num_lines) return;
	        selfile_shift += 1;
		break;
	     case 'P':
	        if (selfile_shift == 0) return;
	        selfile_shift = 0;
		break;
	     case 'W':
	        if (num_table_entries-selfile_shift<num_lines) return;
	        selfile_shift = num_table_entries - num_lines+2;
		break;
	     case 'h':
	     case 'q':
	     case 'w':
	        goto general;
	     default :
	        return;
	   }
	   clearSelfile();
	   return;
	}
	 
 general:
        switch(key) {
	   case '\011':
	     deg_mode = 1 -deg_mode;
	     break;
	   case '\033':
	     if (do_menu) PopMenu();
	     break;
	   case '<':
	     if (did_resize) {
	        if (do_map) {
		  MapGeom.width = former_width;
		  MapGeom.height = former_height;
		} else {
		  ClockGeom.width = former_width;
		  ClockGeom.height = former_height;
		}
                if (!invert) {
                  XDestroyImage(CurXIM);
		  CurXIM=0;
		  if (do_map) MapXIM = 0; else ClockXIM = 0;
	    	}
                adjustGeom(0);
                shutDown(0);
	        buildMap();
		did_resize = 0;
	     }
             break;
	   case 'P':
	     label_shift = 0;
	     break;
	   case 'W':
	     label_shift = 50;
	     clearStrip();
	     break;
	   case 'U': 
	     if (label_shift>0)
               --label_shift;
	     break;
	   case 'V': 
	     if (label_shift<50)
	       ++label_shift;
	     break;
	   case 'a': 
	     time_jump += time_progress;
	     force_proj = 1;
	     last_hint = -1;
	     break;
	   case 'b': 
	     time_jump -= time_progress;
	     force_proj = 1;
	     last_hint = -1;
	     break;
	   case 'c': 
	     if (map_mode != COORDINATES) 
	       deg_mode = dms_mode;
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
	       deg_mode = dms_mode;
	     else
	       deg_mode = 1 - deg_mode;
	     map_mode = DISTANCES;
	     break;
	   case 'e': 
	     map_mode = EXTENSION;
	     old_mode = EXTENSION;
	     hours_shown = 0;
	     show_hours();
	     break;
	   case 'f':
	     if (!do_selfile)
	        PopSelfile();
	     else
                RaiseAndFocus(Selfile);
	     break;
	   case 'g': 
	     if (!do_map) break;
	     if (!do_menu)
	       PopMenu();
             else {
	       last_hint = -1; 
               if (time_progress_init < 0) {
                 time_progress_init = -time_progress_init;
		 time_progress = 60;
		 break;
	       }
	       if (time_progress == 60) time_progress = 3600;
	         else
	       if (time_progress == 3600) time_progress = 86400;
	         else
	       if (time_progress == 86400) time_progress = 604800;
	         else
	       if (time_progress == 604800) time_progress = 2592000;
       	         else 
	       if (time_progress == 2592000) {
		 if (time_progress_init) {
		    time_progress = time_progress_init;
                    time_progress_init = -time_progress_init;
		 } else
	            time_progress = 60;
	       } else
		    time_progress = 60;
	     }
	     break;
	   case 'h':
	     if (!do_menu) 
	        PopMenu();
	     else
	        if (do_map)
	           RaiseAndFocus(Menu);
	     break;
	   case 'i': 
	     if (do_map && do_menu) PopMenu();
             XIconifyWindow(dpy, Map, scr);
	     break;
	   case 'u':
             if (mono && do_cities) drawCities();
	     do_cities = 1 - do_cities;
             if (mono && do_cities) drawCities();
             exposeMap();
	     break;
	   case 'l':
	     map_mode = LEGALTIME;
             if (mark1.city) mark1.city->mode = 0;
             if (mark2.city) mark2.city->mode = 0;
             if (mark1.city == &pos1 || mark2.city == &pos2) updateMap();
             mark1.city = NULL;
             mark2.city = NULL;
	     break;
	   case 'm':
             do_merid = 1 - do_merid;
             if (mono) draw_meridians();
	     exposeMap();
	     break;
	   case 'n':
	     if (invert || (color_scale == 1))
	        shading_level = 1 - shading_level;
	     else {
	        int size;
	        shading_level = (shading_level+1) % 5;
                if (shading_level>=2) {
                   setDarkPixels(allocation_level, color_scale);
		   allocation_level = color_scale;
		}
		for (i = 0; i < Earthmap.height; i++)
		    Earthmap.wtab1[i] = Earthmap.width;
                size = CurXIM->width*CurXIM->height*(CurXIM->bitmap_pad/8);
                memcpy(CurXIM->data, ImageData, size); 
                setShade();
	     }
	     force_proj = 1;
	     exposeMap();
	     break;
	   case 'o':
             do_sunpos = 1 - do_sunpos;
	     if (mono) draw_sun();
	     exposeMap();
             break;
	   case 'p':
             do_parall = 1 - do_parall;
	     if (mono) draw_parallels();
	     exposeMap();
	     break;
	   case 'q': 
	     shutDown(1);
	   case 's': 
	     if (map_mode != SOLARTIME) 
	       deg_mode = dms_mode;
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
             exposeMap();
	     break;
	   case ' ':
	   case 'w': 
	     do_map = 1 - do_map; 
	     adjustGeom(1);
             shutDown(0);
             buildMap();
	     did_resize = 0;
	     break;
	   case 'r':
	     clearStrip();
	     updateMap();
	     break;
	   case 'x':
	     if (Command) system(Command);
	     break;
	   case 'z':
	     time_jump = 0;
	     force_proj = 1;
	     last_hint = -1;
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
	if (y>MapGeom.strip) return;
        click_pos = x/chwidth;
	if (click_pos >= N_OPTIONS) click_pos = N_OPTIONS;
        showMenuHint(click_pos);
}

void freeXpmfiles(bool)
int * bool;
{
        if (do_map) {
	   if (MapXpm_sel || MapXpm_file) *bool = 1;
	   if (MapXpm_sel) free(MapXpm_sel);
           MapXpm_sel = NULL;
	   MapXpm_file = NULL;
        } else {
	   if (ClockXpm_sel || ClockXpm_file) *bool = 1;
	   if (ClockXpm_sel) free(ClockXpm_sel);
           ClockXpm_sel = NULL;
	   ClockXpm_file = NULL;
	}
}

void 
processSelAction(a, b)
int a;
int b;
{
	int  bool;

        if (b <= MapGeom.strip) {
	  a = a/(2*chwidth);
	  if (a==0 && getenv("HOME"))
	     sprintf(image_dir, "%s/", getenv("HOME")); 
	  if (a==1)
	     strcpy(image_dir, share_maps_dir);
	  if (a==2)
	     strcpy(image_dir, "/"); 
	  if (a==3 && getcwd(NULL,1024))
	     sprintf(image_dir, "%s/", getcwd(NULL,1024));
	  if (a<=3)
	     selfile_shift = 0;
	  if (a==4) {
	     if (selfile_shift == 0) return;
	     selfile_shift -= num_lines/2;
	     if (selfile_shift <0) selfile_shift = 0;
	  }
	  if (a==5) {
	     if (num_table_entries-selfile_shift<num_lines) return;
	     selfile_shift += num_lines/2;
	  }
	  if (a>=6 && a<=7) {
	     XUnmapWindow(dpy, Selfile);
	     do_selfile = 0;
	     return;
	  }
	  if (a>=8) {
	      bool=0;
	      freeXpmfiles(&bool);
	      if (bool) {
		 adjustGeom(0);
	         shutDown(0);
	         buildMap();
		 if (do_selfile)
                    RaiseAndFocus(Selfile);
	      }
	      return;
	  }
	  clearSelfile();
	  return;
	}
        if (b <= 2*MapGeom.strip) {
	  selfile_shift = 0;
	  clearSelfile();
	  return;
	}
	b = (b-2*MapGeom.strip-3)/(4*MapGeom.strip/5) + selfile_shift;
	if (b<num_table_entries) {
	   char *s;
	   s = dirtable[b];
	   if (s==NULL || *s=='\0') return;
	   if (a > XTextWidth (BigFont, s, strlen(s))+chwidth/4) return;
	   b = strlen(s)-1;
           if (s[b] == '/') {
	      int l;
	      if (!strcmp(s, "../")) {
	        l=strlen(image_dir)-1;
		if (l==0) return;
                image_dir[l--] = '\0';
	        while (l>=0 && image_dir[l] != '/')
		   image_dir[l--] = '\0';
		s = "";
	      }
              strcat(image_dir, s);
	      l=strlen(image_dir);
              if (image_dir[l-1] != '/') {
                 image_dir[l] = 'l';
                 image_dir[++l] = '\0';
	      }
	      if (dirtable) free_dirlist(dirtable);
	      dirtable = NULL;
	      selfile_shift = 0;
	      num_table_entries=0;
              clearSelfile();
	      return;
	   } else {
	      char *f, *Xpm_file;
	      Xpm_file = (do_map)? MapXpm_file : ClockXpm_file;
	      f = (char *)
                salloc((strlen(image_dir)+strlen(s)+2)*sizeof(char));
	      sprintf(f, "%s%s", image_dir, s);
	      if (!Xpm_file || strcmp(f, Xpm_file)) {
	         freeXpmfiles(&bool);
		 adjustGeom(0);
	         shutDown(0);
		 if (do_map) 
                    MapXpm_sel = MapXpm_file = f;
		 else
                    ClockXpm_sel = ClockXpm_file = f;
	         buildMap();
		 if (do_map && do_menu) {
		   do_menu = 0;
		   PopMenu();
		 }
		 if (do_selfile)
                   RaiseAndFocus(Selfile);
	      } else free(f);
	   }
	}
}

void
processClick(win, x, y, button)
Window  win;
int	x, y;
int     button;
{
char	key;
int	click_pos;

        RaiseAndFocus(win);
 
	if (win==Map && !do_map) {
	   if (button==1) {
	      if (y>ClockGeom.height) {
	         clock_mode = 1-clock_mode;
	         updateMap();
	      } else
		 processKey(win, 'x');
	   }
	   if (button==2)
	      PopSelfile();
	   if (button==3)
	      processKey(win, 'w');
	   return;
        }

        /* Click on bottom strip of map */
        if (win==Map && do_map) {
          if(y >= Earthmap.height) {
	     PopMenu();
             return;
	  }

	  if (button==2) {
	     PopSelfile();
	     return;
	  }
	  if (button==3) {
	     processKey(win, 'w');
	     return;
	  }

          time_count = PRECOUNT;

          /* Erase bottom strip to clean-up spots overlapping bottom strip */
	  if (do_bottom) clearStrip();

          /* Set the timezone, marks, etc, on a button press */
          processPoint(win, x, y);

          /* if spot overlaps bottom strip, set flag */
 	  if (y >= Earthmap.height - radius[spot_size]) {
	    if (map_mode == SOLARTIME)
              do_bottom = 1;
	    if (map_mode == DISTANCES)
              do_bottom = 2;
	  }
	}

        if (win == Menu) {
	   if (y>MapGeom.strip) return;
           click_pos = x/chwidth;
	   if (click_pos >= N_OPTIONS) {
	      PopMenu();
	      return;
	   }
           key = tolower(Option[2*click_pos]);
	   processKey(win, key);
	}

        if (win == Selfile)
	   processSelAction(x, y);
}

void 
processResize(win)
Window  win;
{
	   int x, y, w, h;
	   struct geom *Geom;

	   if (win == Map) {
	      if (getPlacement(win, &x, &y, &w, &h)) return;
              Geom = (do_map)? &MapGeom: &ClockGeom;
              former_width = Geom->width;
	      former_height = Geom->height;
              h -= Geom->strip;
              if (w==Geom->width && h==Geom->height) return;
	      if (w<Geom->w_mini) w = Geom->w_mini;
	      if (h<Geom->h_mini) h = Geom->h_mini;
	      Geom->width = w;
	      Geom->height = h;
              if (!invert) {
                 XDestroyImage(CurXIM);
                 CurXIM=0;
	         if (do_map) MapXIM = 0; else ClockXIM = 0;
    	      }
              adjustGeom(0);
              shutDown(0);
	      buildMap();
	      did_resize = 1;
	
	      if (do_menu) {
	         XFlush(dpy);
	         usleep(TIMESTEP);
	         do_menu = 0;
	         PopMenu();
	      }
	      return;
	   }
	   
	   if (win == Selfile) {
	      if (getPlacement(win, &x, &y, &w, &h)) return;
	      Geom = &SelGeom;
              if (w==Geom->width && h==Geom->height) return;
	      if (w<Geom->w_mini) w = Geom->w_mini;
	      if (h<Geom->h_mini) h = Geom->h_mini;
	      Geom->width = w;
	      Geom->height = h;
	      XDestroyWindow(dpy, Selfile);
              Selfile = 0;
	      do_selfile = 0;
	      createWindow(3);
	      setProtocols(3);
	      PopSelfile();
	   }
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

	if (--Earthmap.timeout <= 0 && (time_count==0)) {
		updateImage();
		showImage();
		Earthmap.timeout = 1;
                if (mono) pulseMarks();
	}
}

void
doExpose(w)
register Window			w;
{
	if (w == Map) {
	   updateImage();
	   Earthmap.flags |= S_DIRTY;
	   showImage();
	}

        if (w == Menu) 
	   initMenu();

        if (w == Selfile) 
           initSelfile();
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
	                        if (map_mode == EXTENSION) 
                                        show_hours();
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
					if (ev.xexpose.window==Selfile)
					   PopSelfile();
					else
					   shutDown(1);
				}
				break;

			case KeyPress:
                                key = XKeycodeToKeysym(dpy,ev.xkey.keycode,0);
				processKey(ev.xexpose.window, key & 255);
                                break;

			case ButtonPress:
			        processClick(ev.xexpose.window, 
                                             ev.xbutton.x, ev.xbutton.y,
					     ev.xbutton.button);
				break;

			case MotionNotify:
			        processMotion(ev.xexpose.window, 
                                              ev.xbutton.x, ev.xbutton.y);
			        break;

			case ResizeRequest: 
			        processResize(ev.xexpose.window);
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
		return;
            }

	if (mark1.city == &pos1) {
		map_mode = SOLARTIME;
		mark1.city = NULL;
		setTZ(NULL);
		doTimeout();
		mark1.city = &pos1;
                if (mono) pulseMarks();
		pos1.name = Label[L_POINT];
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

	leave = 1;
	(void) parseArgs(argc, argv);

        /* Correct bad setting of placement and shading_level parameters */
	if (placement<0) placement = NW;
	if (invert && (shading_level>=2)) shading_level = 1;

	dpy = XOpenDisplay(Display_name);
	if (dpy == (Display *)NULL) {
		fprintf(stderr, "%s: can't open display `%s'\n", ProgName,
			Display_name);
		exit(1);
	}

	scr = DefaultScreen(dpy);
        wm_protocols = XInternAtom(dpy, "WM_PROTOCOLS", False);
        wm_delete_window = XInternAtom(dpy, "WM_DELETE_WINDOW", False);

        if (do_private && DefaultDepth(dpy,scr)<=8)
	    cmap = XCreateColormap(dpy, RootWindow(dpy, scr), 
	                           DefaultVisual(dpy, scr), AllocNone);
	else
            cmap = DefaultColormap(dpy, scr);

	getColors();
	getFonts();
        buildMap();
	checkLocation(CityInit);
	if (do_map) do_menu = 1 - do_menu;
	PopMenu();
	eventLoop();
	exit(0);
}
